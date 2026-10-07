#include "game.h"
#include "game_ui.h"
#include "achievements.h"
#include "../engine/image.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace fv {

using fe::Key;

// ------------------------------------------------------------
// Initialisation
// ------------------------------------------------------------
void Game::init(IPlatform* platform, bool bot, int botDays_, const string& shotPrefix_) {
    plat = platform;
    botMode = bot;
    botDays = botDays_;
    shotPrefix = shotPrefix_;
    fb.resize(FB_W, FB_H);
    audio.init(platform);
    audio.master = 0.85f;
    hasSave = saveExists();
    steam::init();
    plat->setTitle("FarmVale — La Ferme des Quatre Saisons");
    if (bot) {
        // En mode bot (headless), on démarre directement une partie
        sim.newGame(20261007u);
        screen = Screen::PLAYING;
        menu = Menu::NONE;
    }
}

string Game::savePath() {
    return plat->saveDir + "/farmvale_save.txt";
}

bool Game::saveExists() {
    FILE* f = fopen(savePath().c_str(), "r");
    if (!f) return false;
    fclose(f);
    return true;
}

void Game::saveGame() {
    if (sim.saveToFile(savePath())) {
        sim.toast("Partie sauvegardée.", 1);
    } else {
        sim.toast("Échec de la sauvegarde.", 3);
    }
}

void Game::loadGame() {
    if (sim.loadFromFile(savePath())) {
        screen = Screen::PLAYING;
        menu = Menu::NONE;
        sim.timePaused = false;
        sim.toast("Partie chargée.", 1);
        audio.sfx(fe::Sfx::Open);
    } else {
        sim.toast("Aucune sauvegarde trouvée.", 3);
    }
}

// ------------------------------------------------------------
// Boucle principale
// ------------------------------------------------------------
void Game::run() {
    double last = plat->timeMs();
    while (plat->poll()) {
        frameCount++;
        double now = plat->timeMs();
        float dt = (float)((now - last) / 1000.0);
        last = now;
        if (dt > 0.1f) dt = 0.1f;
        if (dt <= 0) dt = 0.001f;

        update(dt);
        render();
        plat->present(fb);
        plat->input.endFrame();

        double elapsed = plat->timeMs() - now;
        if (elapsed < 16.0) plat->sleepMs((int)(16.0 - elapsed));
    }
    steam::shutdown();
}

// ------------------------------------------------------------
// Update global
// ------------------------------------------------------------
void Game::update(float dt) {
    audio.update(plat->timeMs());
    switch (screen) {
        case Screen::TITLE:   updateTitle(dt); break;
        case Screen::PLAYING: updatePlaying(dt); break;
        case Screen::PAUSED:  break;
    }
    // toasts
    for (auto& t : sim.toasts) t.ttl -= dt;
    if (!sim.toasts.empty() && sim.toasts.front().ttl <= 0) sim.toasts.erase(sim.toasts.begin());
    if (lightning > 0) lightning -= dt * 2.5f;
    if (lightningCooldown > 0) lightningCooldown -= dt;
}

void Game::updateTitle(float dt) {
    // Le monde du fond tourne doucement (on simule un peu de temps)
    static float acc = 0;
    acc += dt;
    if (acc > 0.5f) { acc = 0; sim.advanceTime(0.5f); }
    updatePlayingCameraOnly(dt);
}

// ------------------------------------------------------------
// Update en jeu
// ------------------------------------------------------------
void Game::updatePlaying(float dt) {
    if (botMode) botUpdate(dt);

    const auto& in = plat->input;

    // --- Menus : le temps du jeu est en pause ---
    if (menu != Menu::NONE) {
        sim.timePaused = true;
    } else {
        sim.timePaused = false;
    }

    // --- Mouvement du joueur ---
    if (menu == Menu::NONE && screen == Screen::PLAYING) {
        float dx = 0, dy = 0;
        if (in.down[Key::K_LEFT] || in.down[Key::K_A]) dx -= 1;
        if (in.down[Key::K_RIGHT] || in.down[Key::K_D]) dx += 1;
        if (in.down[Key::K_UP] || in.down[Key::K_W]) dy -= 1;
        if (in.down[Key::K_DOWN] || in.down[Key::K_S]) dy += 1;
        if (dx != 0 || dy != 0) {
            float len = std::sqrt(dx * dx + dy * dy);
            dx /= len; dy /= len;
            MapData& m = sim.world.current(sim.st);
            float nx = sim.st.px + dx * PLAYER_SPEED * dt;
            float ny = sim.st.py + dy * PLAYER_SPEED * dt;
            if (!sim.world.solidAt(sim.st, m, sim.st.px, sim.st.py) || true) {
                if (!sim.world.solidAt(sim.st, m, nx, sim.st.py)) sim.st.px = nx;
                if (!sim.world.solidAt(sim.st, m, sim.st.px, ny)) sim.st.py = ny;
            }
            if (std::abs(dx) > std::abs(dy)) sim.st.facing = dx > 0 ? FACING_RIGHT : FACING_LEFT;
            else sim.st.facing = dy > 0 ? FACING_DOWN : FACING_UP;
            walkPhase += dt * 9.0f;
        }
        if (actionAnim > 0) actionAnim -= dt;

        // --- Interactions clavier ---
        handleInputPlaying();
    }

    // --- Simulation ---
    sim.advanceTime(dt);
    sim.updateAnimals(dt);

    // --- Pêche (mini-jeu) ---
    if (fishing) {
        if (fishWait > 0) {
            fishWait -= dt;
            if (fishWait <= 0) {
                // la touche mord !
                fishWait = -1;
                fishSpeed = 40 + sim.rng.uf() * 60;
                pickFish();
                audio.sfx(fe::Sfx::Bell);
                sim.toast("Ça mord ! Maintiens ESPACE pour ferrer.", 1);
            }
        } else {
            // mini-jeu : la cible oscille, le joueur monte/descend la barre
            fishTarget = 50 + std::sin(plat->timeMs() / 300.0) * 30;
            float up = in.down[Key::K_SPACE] ? 1.0f : 0.0f;
            fishBar += (up * 130.0f - 65.0f) * dt + (50.0f - fishBar) * 2.0f * dt;
            if (fishBar < 5) fishBar = 5;
            if (fishBar > 95) fishBar = 95;
            if (std::abs(fishBar - fishTarget) < 12) {
                fishProgress += dt * (0.5f + sim.st.skillLevel(SK_FISHING) * 0.05f);
            } else {
                fishProgress -= dt * 0.15f;
            }
            if (fishProgress < 0) fishProgress = 0;
            if (fishProgress >= 100 || in.pressed[Key::K_ESC]) {
                fishing = false;
                if (fishProgress >= 100) {
                    sim.catchFish(fishCatchId, fishQuality);
                } else {
                    sim.toast("Le poisson s'est échappé…", 2);
                }
            }
        }
    }

    // --- Événements de la sim (sons, ouvertures de menus, succès) ---
    processEvents();

    // --- Caméra ---
    updatePlayingCameraOnly(dt);

    // --- Particules météo ---
    MapData& m = sim.world.current(sim.st);
    int mapPxW = m.w * TILE, mapPxH = m.h * TILE;
    if (sim.st.weather == W_RAIN || sim.st.weather == W_STORM) {
        if ((int)rain.size() < 220) {
            Particle p;
            p.x = (float)(sim.rng.range(0, mapPxW + 200) - 100);
            p.y = (float)(sim.rng.range(-100, 0));
            p.vx = -60 - sim.rng.uf() * 40;
            p.vy = 420 + sim.rng.uf() * 160;
            p.len = 8 + (int)(sim.rng.uf() * 8);
            rain.push_back(p);
        }
        for (auto& p : rain) {
            p.x += p.vx * dt; p.y += p.vy * dt;
            if (p.y > mapPxH + 20) { p.y = -10; p.x = (float)(sim.rng.range(0, mapPxW)); }
        }
        while (rain.size() > 220) rain.erase(rain.begin());
    } else rain.clear();
    if (sim.st.weather == W_SNOW) {
        if ((int)snow.size() < 140) {
            Particle p;
            p.x = (float)(sim.rng.range(0, mapPxW + 100));
            p.y = (float)(sim.rng.range(-100, 0));
            p.vx = 10 + sim.rng.uf() * 20;
            p.vy = 30 + sim.rng.uf() * 25;
            p.len = 2;
            snow.push_back(p);
        }
        for (auto& p : snow) {
            p.x += (p.vx + std::sin(plat->timeMs() / 900.0 + p.y) * 14) * dt;
            p.y += p.vy * dt;
            if (p.y > mapPxH + 20) { p.y = -10; p.x = (float)(sim.rng.range(0, mapPxW)); }
        }
        while (snow.size() > 140) snow.erase(snow.begin());
    } else snow.clear();

    // Orage : éclairs aléatoires
    if (sim.st.weather == W_STORM) {
        if (lightningCooldown <= 0 && sim.rng.chance(0.008f * dt * 60)) {
            lightning = 1.0f;
            lightningCooldown = 2.0f + sim.rng.uf() * 5.0f;
            audio.beep(90, 120);
        }
    }

    // Cœurs
    for (auto& h : hearts) h.ttl -= dt;
    if (!hearts.empty() && hearts.front().ttl <= 0) hearts.erase(hearts.begin());
}

void Game::updatePlayingCameraOnly(float dt) {
    (void)dt;
    MapData& m = sim.world.current(sim.st);
    int mapPxW = m.w * TILE, mapPxH = m.h * TILE;
    // On centre la carte quand elle est plus petite que l'écran (maison)
    int minX = std::min(0, mapPxW - FB_W);
    int maxX = std::max(0, mapPxW - FB_W);
    int minY = std::min(0, mapPxH - FB_H);
    int maxY = std::max(0, mapPxH - FB_H);
    camX = std::max(minX, std::min(maxX, (int)sim.st.px - FB_W / 2));
    camY = std::max(minY, std::min(maxY, (int)sim.st.py - FB_H / 2));
}

int Game::camClampX() const { return camX; }
int Game::camClampY() const { return camY; }

// ------------------------------------------------------------
// Entrées clavier en jeu
// ------------------------------------------------------------
void Game::handleInputPlaying() {
    const auto& in = plat->input;
    // Sélection de la hotbar
    const Key numKeys[12] = { Key::K_1, Key::K_2, Key::K_3, Key::K_4, Key::K_5, Key::K_6,
                              Key::K_7, Key::K_8, Key::K_9, Key::K_0, Key::K_MINUS, Key::K_EQUALS };
    for (int i = 0; i < 12; i++) {
        if (in.pressed[numKeys[i]]) {
            sim.selectSlot(i);
            audio.sfx(fe::Sfx::Select);
        }
    }
    // Interaction devant soi
    if (in.pressed[Key::K_SPACE] || in.pressed[Key::K_ENTER]) {
        if (fishing) { /* le mini-jeu utilise ESPACE */ }
        else doInteractFront();
    }
    // Souris : interaction sur la tuile cliquée
    if (in.mousePressed && !fishing) {
        MapData& m = sim.world.current(sim.st);
        int tx = (camX + in.mouseX) / TILE;
        int ty = (camY + in.mouseY) / TILE;
        if (m.inBounds(tx, ty) && in.mouseY > 40 && in.mouseY < FB_H - 60) {
            doInteractTile(tx, ty);
        }
    }
    // Manger
    if (in.pressed[Key::K_E]) tryEat();
    // Inventaire
    if (in.pressed[Key::K_B] || in.pressed[Key::K_TAB]) {
        if (menu == Menu::INVENTORY) closeMenu();
        else openMenu(Menu::INVENTORY);
    }
    // Pause
    if (in.pressed[Key::K_ESC] || in.pressed[Key::K_P]) {
        if (menu != Menu::NONE) closeMenu();
        else if (screen == Screen::PLAYING) { screen = Screen::PAUSED; audio.sfx(fe::Sfx::Open); }
    }
    // Plein écran
    if (in.pressed[Key::K_F]) plat->setFullscreen(!plat->isFullscreen());
    // Musique
    if (in.pressed[Key::K_M]) {
        audio.musicOn = !audio.musicOn;
        sim.toast(audio.musicOn ? "Musique : oui" : "Musique : non", 0);
    }
    // Sauvegarde rapide
    if (in.pressed[Key::K_O]) saveGame();
}

void Game::doInteractFront() {
    int tx = (int)(sim.st.px / TILE);
    int ty = (int)(sim.st.py / TILE);
    switch (sim.st.facing) {
        case FACING_UP: ty -= 1; break;
        case FACING_DOWN: ty += 1; break;
        case FACING_LEFT: tx -= 1; break;
        case FACING_RIGHT: tx += 1; break;
    }
    doInteractTile(tx, ty);
}

void Game::doInteractTile(int tx, int ty) {
    if (screen != Screen::PLAYING) return;
    if (sim.actionAt(tx, ty)) {
        actionAnim = 0.25f;
    }
}

int Game::foodValue(const string& id) const {
    if (id == "wheat") return 15;
    if (id == "potato") return 25;
    if (id == "strawberry") return 20;
    if (id == "corn") return 20;
    if (id == "tomato") return 20;
    if (id == "pumpkin") return 35;
    if (id == "sunflower") return 10;
    if (id == "kale") return 20;
    if (id == "berry") return 15;
    if (id == "egg") return 20;
    if (id == "egg_large") return 30;
    if (id == "milk") return 40;
    if (id == "milk_large") return 60;
    if (id == "hay") return 10;
    if (id == "fish_roach") return 25;
    if (id == "fish_trout") return 40;
    if (id == "fish_carp") return 45;
    if (id == "fish_pike") return 70;
    return 0;
}

void Game::tryEat() {
    ItemStack& s = sim.selected();
    if (s.empty()) { sim.toast("Sélectionnez de la nourriture.", 2); return; }
    int val = foodValue(s.id);
    if (val <= 0) { sim.toast("Ce n'est pas comestible.", 3); return; }
    if (sim.st.energy >= sim.st.energyMaxBase()) { sim.toast("Vous n'avez pas faim.", 2); return; }
    sim.removeItem(s.id, s.q, 1);
    sim.st.energy = std::min((float)sim.st.energyMaxBase(), sim.st.energy + val);
    audio.sfx(fe::Sfx::Feed);
    const ItemDef* d = itemDef(s.id);
    sim.toast(string("Miam… +") + std::to_string(val) + " énergie (" + (d ? d->name : s.id) + ")", 1);
}

// ------------------------------------------------------------
// Événements de la sim
// ------------------------------------------------------------
void Game::processEvents() {
    for (auto& e : sim.events) {
        playEventSfx(e);
        if (e.type == "open_shop") openMenu(Menu::SHOP);
        else if (e.type == "open_chest") openMenu(Menu::CHEST);
        else if (e.type == "open_quests") openMenu(Menu::QUESTS);
        else if (e.type == "sleep_ui") openMenu(Menu::SLEEP_CONFIRM);
        else if (e.type == "fish_start") { fishing = true; fishWait = 1.2f + sim.rng.uf() * 2.0f; fishProgress = 0; }
        else if (e.type == "hearts") {
            int id = std::atoi(e.arg.c_str());
            for (auto& a : sim.st.animals) {
                if (a.id == id) {
                    hearts.push_back({a.x - camX, a.y - camY - 30, 1.6f});
                    break;
                }
            }
        } else if (e.type == "autosave") saveGame();
        else if (e.type == "ach") steam::unlock(e.arg.c_str());
        else if (e.type == "newday") {
            // petit son de cloche le matin
            audio.sfx(fe::Sfx::Bell);
        }
    }
    sim.events.clear();
}

void Game::playEventSfx(const Event& e) {
    if (e.type != "sfx") return;
    if (e.arg == "till") audio.sfx(fe::Sfx::Till);
    else if (e.arg == "water") audio.sfx(fe::Sfx::Water);
    else if (e.arg == "plant") audio.sfx(fe::Sfx::Plant);
    else if (e.arg == "harvest") audio.sfx(fe::Sfx::Harvest);
    else if (e.arg == "chop") audio.sfx(fe::Sfx::Chop);
    else if (e.arg == "scythe") audio.sfx(fe::Sfx::Scythe);
    else if (e.arg == "forage") audio.sfx(fe::Sfx::Forage);
    else if (e.arg == "feed") audio.sfx(fe::Sfx::Feed);
    else if (e.arg == "pet") audio.sfx(fe::Sfx::Pet);
    else if (e.arg == "collect") audio.sfx(fe::Sfx::Milk);
    else if (e.arg == "catch") audio.sfx(fe::Sfx::Catch);
    else if (e.arg == "coin") audio.sfx(fe::Sfx::Coin);
    else if (e.arg == "quest") audio.sfx(fe::Sfx::Quest);
    else if (e.arg == "levelup") audio.sfx(fe::Sfx::LevelUp);
    else if (e.arg == "door") audio.sfx(fe::Sfx::Door);
    else if (e.arg == "open") audio.sfx(fe::Sfx::Open);
    else if (e.arg == "swing") audio.sfx(fe::Sfx::Swing);
    else if (e.arg == "birth") audio.sfx(fe::Sfx::Birth);
    else if (e.arg == "breed") audio.sfx(fe::Sfx::Breed);
    else if (e.arg == "sleep") audio.sfx(fe::Sfx::Sleep);
    else if (e.arg == "wakeup") audio.sfx(fe::Sfx::Wakeup);
}

// ------------------------------------------------------------
// Menus
// ------------------------------------------------------------
void Game::openMenu(Menu m) {
    menu = m;
    audio.sfx(fe::Sfx::Open);
}

void Game::closeMenu() {
    // Rend l'objet tenu à sa place d'origine si besoin
    if (holdingItem && heldFrom && heldIndex >= 0 && heldIndex < (int)heldFrom->size()
        && (*heldFrom)[heldIndex].empty()) {
        (*heldFrom)[heldIndex] = heldStack;
    }
    menu = Menu::NONE;
    holdingItem = false;
    heldStack.clear();
    heldFrom = nullptr;
    heldIndex = -1;
}

void Game::takeScreenshot(const string& name) {
    string path = shotPrefix + name + ".bmp";
    fe::saveBMP(fb, path);
    printf("[screenshot] %s\n", path.c_str());
    shotsTaken++;
}

} // namespace fv
