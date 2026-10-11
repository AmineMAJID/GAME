#include "game.h"
#include "game_ui.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

namespace fv {

using fe::rgb;

static const Color C_GOLD_UI = rgb(232, 193, 90);
static const char* WEEK_DAYS[7] = { "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche" };
static const char* HOTBAR_KEYS = "1234567890-=";

// ------------------------------------------------------------
// Frame principale
// ------------------------------------------------------------
void Game::render() {
    if (screen == Screen::TITLE) {
        renderTitle();
        if (menu != Menu::NONE) {
            fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(10, 8, 14), 150);
            renderMenus();
        }
    } else {
        renderWorld();
        renderHud();
        if (screen == Screen::PAUSED) {
            renderPauseMenu();
        } else if (menu != Menu::NONE) {
            fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(10, 8, 14), 150);
            renderMenus();
        }
        if (fishing) renderFishingGame();
    }
    if (botMode && !pendingShot.empty()) {
        takeScreenshot(pendingShot);
        pendingShot.clear();
        if (shotsTaken >= 10) plat->requestQuit();
    }
}

// ------------------------------------------------------------
// HUD en jeu
// ------------------------------------------------------------
void Game::renderHud() {
    const auto& in = plat->input;

    // Barre du haut
    fe::blendRect(fb, 0, 0, FB_W, 34, rgb(15, 13, 20), 190);
    fe::hline(fb, 0, FB_W - 1, 34, UI_BORDER);

    // Argent
    fe::drawIcon(fb, 8, 9, IC_COIN, 0);
    fe::drawTextShadow(fb, 30, 12, std::to_string(sim.st.money), fe::FONT_SMALL, C_GOLD_UI);

    // Énergie
    int ex = 90, ew = 150;
    fe::drawIcon(fb, ex - 22, 9, IC_BOLT, 0);
    fe::rect(fb, ex, 12, ew, 12, UI_SLOT);
    fe::rectOutline(fb, ex, 12, ew, 12, UI_BORDER);
    float ef = sim.st.energy / sim.st.energyMaxBase();
    Color ec = ef > 0.5f ? UI_GOOD : (ef > 0.25f ? UI_WARN : UI_BAD);
    fe::rect(fb, ex + 2, 14, (int)((ew - 4) * ef), 8, ec);
    fe::drawTextShadow(fb, ex + ew + 6, 12, std::to_string((int)sim.st.energy) + "/" + std::to_string(sim.st.energyMaxBase()), fe::FONT_SMALL, UI_TEXT);

    // Date / heure / saison (centre)
    char buf[128];
    snprintf(buf, sizeof(buf), "%s %d · %s · %02d:%02d",
             seasonName(sim.st.season()), sim.st.dayOfSeason(), WEEK_DAYS[sim.st.weekDay()],
             sim.st.hour(), sim.st.minute());
    fe::drawTextCentered(fb, FB_W / 2, 12, buf, fe::FONT_SMALL, UI_TEXT);

    // Météo (droite)
    int wx = FB_W - 150;
    int wIcon = IC_SUN;
    switch (sim.st.weather) {
        case W_SUN: wIcon = IC_SUN; break;
        case W_CLOUD: wIcon = IC_CLOUD; break;
        case W_RAIN: wIcon = IC_RAIN; break;
        case W_STORM: wIcon = IC_STORM; break;
        case W_SNOW: wIcon = IC_SNOW; break;
    }
    fe::drawIcon(fb, wx, 9, wIcon, 0);
    fe::drawTextShadow(fb, wx + 22, 12, weatherName(sim.st.weather), fe::FONT_SMALL, UI_TEXT);

    // Info objet sélectionné (au-dessus de la hotbar)
    ItemStack& sel = sim.selected();
    if (!sel.empty()) {
        const ItemDef* def = itemDef(sel.id);
        if (def) {
            string label = qualityName(sel.q) + def->name + (sel.n > 1 ? " x" + std::to_string(sel.n) : "");
            int w = fe::textWidth(label, fe::FONT_SMALL) + 12;
            fe::drawTextCentered(fb, FB_W / 2, FB_H - 78, label, fe::FONT_SMALL, UI_ACCENT);
            (void)w;
        }
    }

    renderHotbar();
    renderToasts();
    renderAnimalInfo();

    // Aide contextuelle
    fe::drawTextShadow(fb, 8, FB_H - 18, "Espace: agir · B: sac · E: manger · O: sauvegarder · M: musique · F: plein écran · Échap: pause",
                       fe::FONT_SMALL, UI_DIM);
    (void)in;
}

// ------------------------------------------------------------
// Hotbar
// ------------------------------------------------------------
void Game::renderHotbar() {
    int slots = (int)sim.st.hotbar.size();
    int sw = 34, gap = 4;
    int total = slots * sw + (slots - 1) * gap;
    int x0 = (FB_W - total) / 2;
    int y0 = FB_H - 48;
    for (int i = 0; i < slots; i++) {
        int x = x0 + i * (sw + gap);
        bool sel = (i == sim.sel);
        fe::rect(fb, x, y0, sw, sw, sel ? UI_SLOT_HL : UI_SLOT);
        fe::rectOutline(fb, x, y0, sw, sw, sel ? UI_ACCENT : UI_BORDER);
        if (sel) fe::rect(fb, x, y0 - 3, sw, 3, UI_ACCENT);
        const ItemStack& s = sim.st.hotbar[i];
        if (!s.empty()) {
            const ItemDef* def = itemDef(s.id);
            int icon = def ? def->icon : 0;
            fe::drawIcon(fb, x + 9, y0 + 9, icon, s.q);
            if (s.n > 1) {
                string n = std::to_string(s.n);
                fe::drawTextShadow(fb, x + sw - fe::textWidth(n, fe::FONT_SMALL) - 3, y0 + sw - fe::FONT_SMALL.height - 2, n, fe::FONT_SMALL, UI_TEXT);
            }
        }
        // numéro de raccourci
        char key[2] = { HOTBAR_KEYS[i], 0 };
        fe::drawText(fb, x + 3, y0 + 2, key, fe::FONT_SMALL, UI_DIM);
    }
}

// ------------------------------------------------------------
// Toasts (notifications)
// ------------------------------------------------------------
void Game::renderToasts() {
    int y = 44;
    for (auto& t : sim.toasts) {
        int w = fe::textWidth(t.msg, fe::FONT_SMALL) + 20;
        int x = FB_W - w - 10;
        Color accent = UI_TEXT;
        if (t.kind == 1) accent = UI_GOOD;
        if (t.kind == 2) accent = UI_WARN;
        if (t.kind == 3) accent = UI_BAD;
        int a = (int)std::min(1.0f, t.ttl) * 255;
        fe::rect(fb, x, y, w, 22, UI_PANEL);
        fe::rect(fb, x, y, 4, 22, accent);
        fe::rectOutline(fb, x, y, w, 22, UI_BORDER);
        fe::drawText(fb, x + 10, y + 5, t.msg, fe::FONT_SMALL, UI_TEXT);
        (void)a;
        y += 26;
    }
}

// ------------------------------------------------------------
// Panneau d'info d'un animal (survol souris)
// ------------------------------------------------------------
void Game::renderAnimalInfo() {
    const auto& in = plat->input;
    if (in.mouseY < 40 || in.mouseY > FB_H - 60) return;
    int best = sim.animalNear(camX + in.mouseX, camY + in.mouseY, 26);
    if (best < 0) return;
    Animal& a = sim.st.animals[best];
    const AnimalDef* d = animalDef(a.type);
    if (!d) return;

    int w = 190, h = 96;
    int x = std::max(4, std::min(FB_W - w - 4, in.mouseX + 14));
    int y = std::max(40, std::min(FB_H - h - 60, in.mouseY - h - 8));
    Ui ui(fb, in, &audio);
    ui.panel(x, y, w, h, d->name.c_str());

    fe::drawIcon(fb, x + 10, y + 16, d->icon, 0);
    // cœurs d'amitié
    for (int i = 0; i < 5; i++)
        fe::drawIcon(fb, x + 34 + i * 18, y + 16, IC_HEART, 0);
    for (int i = 0; i < 5; i++)
        if (a.friendship >= (i + 1) * 20)
            fe::rect(fb, x + 34 + i * 18, y + 16, 16, 16, rgb(0, 0, 0)); // masque (opaque = plein)
    // remplace : dessine des cœurs colorés par-dessus selon l'amitié
    for (int i = 0; i < 5; i++) {
        bool full = a.friendship >= (i + 1) * 20;
        bool half = !full && a.friendship >= i * 20 + 10;
        if (full || half) {
            // redessine le cœur avec la bonne couleur via le masque
        }
    }
    char line[128];
    snprintf(line, sizeof(line), "Âge: %d j · %s", a.age, a.isAdult() ? "adulte" : "bébé");
    fe::drawText(fb, x + 10, y + 40, line, fe::FONT_SMALL, UI_TEXT);
    snprintf(line, sizeof(line), "Bonheur: %d%%", a.happiness);
    fe::drawText(fb, x + 10, y + 54, line, fe::FONT_SMALL, UI_TEXT);
    if (a.pregnant) fe::drawText(fb, x + 10, y + 68, "Enceinte… ♡", fe::FONT_SMALL, UI_WARN);
    else if (a.ready > 0) fe::drawText(fb, x + 10, y + 68, "Production prête !", fe::FONT_SMALL, UI_GOOD);
    else fe::drawText(fb, x + 10, y + 68, a.fedToday ? "Nourri aujourd'hui" : "À nourrir", fe::FONT_SMALL, UI_DIM);
}

// ------------------------------------------------------------
// Mini-jeu de pêche
// ------------------------------------------------------------
void Game::renderFishingGame() {
    fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(8, 10, 18), 120);
    int cx = FB_W / 2, cy = FB_H / 2 - 40;
    Ui ui(fb, plat->input, &audio);
    ui.panel(cx - 110, cy - 90, 220, 180, "Pêche");

    if (fishWait > 0) {
        fe::drawTextCentered(fb, cx, cy - 10, "En attente d'une touche…", fe::FONT_SMALL, UI_TEXT);
        float t = 1.0f - fishWait / 3.0f;
        fe::rect(fb, cx - 60, cy + 20, 120, 8, UI_SLOT);
        fe::rect(fb, cx - 60, cy + 20, (int)(120 * t), 8, UI_ACCENT);
        return;
    }

    // barre verticale
    int bx = cx - 10, by = cy - 60, bh = 120;
    fe::rect(fb, bx, by, 20, bh, UI_SLOT);
    fe::rectOutline(fb, bx, by, 20, bh, UI_BORDER);
    // zone cible
    int tz = by + (int)((1.0f - (fishTarget + 12) / 100.0f) * bh);
    fe::rect(fb, bx + 2, tz, 16, (int)(24.0f / 100 * bh), UI_GOOD);
    // curseur poisson
    int fy = by + (int)((1.0f - fishBar / 100.0f) * bh);
    fe::rect(fb, bx + 2, fy - 4, 16, 8, UI_ACCENT);
    fe::drawTextCentered(fb, cx, by + bh + 8, "Maintiens ESPACE !", fe::FONT_SMALL, UI_TEXT);
    // progression
    fe::rect(fb, cx - 60, by + bh + 28, 120, 8, UI_SLOT);
    fe::rect(fb, cx - 60, by + bh + 28, (int)(120 * fishProgress / 100.0f), 8, C_GOLD_UI);
    const FishDef* fd = fishDef(fishCatchId);
    if (fd) fe::drawTextCentered(fb, cx, by + bh + 44, fd->name, fe::FONT_SMALL, UI_DIM);
}

// ------------------------------------------------------------
// Écran titre (scène animée dessinée à la main)
// ------------------------------------------------------------
void Game::renderTitle() {
    double t = plat->timeMs() / 1000.0;
    // ciel dégradé
    Color skyTop = rgb(70, 110, 170), skyBot = rgb(160, 200, 235);
    for (int y = 0; y < FB_H; y++) {
        float f = (float)y / FB_H * 0.55f;
        fe::hline(fb, 0, FB_W - 1, y, fe::lerp(skyTop, skyBot, f));
    }
    // soleil
    int sunX = 780 + (int)(std::sin(t * 0.2) * 30);
    int sunY = 90;
    for (int r = 46; r > 0; r -= 6)
        fe::circle(fb, sunX, sunY, r, r > 30 ? rgb(255, 220, 110) : rgb(255, 235, 150));
    // nuages qui dérivent
    for (int i = 0; i < 4; i++) {
        int cx = ((int)(t * (20 + i * 12)) + i * 260) % (FB_W + 120) - 60;
        int cy = 60 + i * 38;
        fe::circle(fb, cx, cy, 22, rgb(255, 255, 255));
        fe::circle(fb, cx + 24, cy - 8, 26, rgb(255, 255, 255));
        fe::circle(fb, cx + 50, cy + 2, 20, rgb(250, 250, 250));
        fe::rect(fb, cx - 10, cy + 8, 72, 14, rgb(255, 255, 255));
    }
    // collines
    fe::circle(fb, 120, FB_H + 60, 190, rgb(96, 160, 80));
    fe::circle(fb, 700, FB_H + 90, 230, rgb(84, 145, 70));
    fe::circle(fb, 1100, FB_H + 40, 200, rgb(104, 170, 88));
    // sol
    fe::rect(fb, 0, 380, FB_W, FB_H - 380, rgb(86, 145, 66));
    // champs labourés (lignes de sillons)
    for (int y = 420; y < FB_H; y += 26)
        fe::hline(fb, 0, FB_W - 1, y, rgb(110, 90, 60));
    // rangées de cultures
    for (int x = 40; x < FB_W - 40; x += 46) {
        for (int y = 430; y < FB_H - 20; y += 30) {
            int h = tileHash(x, y);
            fe::circle(fb, x + 8, y + 6, 5, (h % 3) ? rgb(62, 155, 79) : rgb(232, 193, 90));
            fe::circle(fb, x + 6, y + 4, 2, rgb(111, 207, 111));
        }
    }
    // maison
    int hx = 90, hy = 300;
    fe::rect(fb, hx, hy + 40, 130, 70, rgb(228, 214, 186));
    for (int row = 0; row < 3; row++)
        fe::rect(fb, hx + row * 12 + 6, hy + row * 14, 130 - row * 24, 14, row % 2 ? rgb(172, 62, 54) : rgb(150, 50, 44));
    fe::rect(fb, hx + 52, hy + 78, 28, 32, rgb(120, 75, 40));
    fe::rect(fb, hx + 14, hy + 62, 22, 18, rgb(255, 220, 130)); // fenêtre chaude
    // arbre
    fe::rect(fb, 640, 330, 10, 50, rgb(110, 70, 40));
    fe::circle(fb, 645, 320, 34, rgb(52, 120, 60));
    fe::circle(fb, 620, 330, 24, rgb(38, 88, 44));
    fe::circle(fb, 672, 332, 22, rgb(70, 140, 76));

    // titre
    fe::blendRect(fb, 0, 130, FB_W, 130, rgb(12, 10, 18), 110);
    fe::drawTextCentered(fb, FB_W / 2, 148, "FarmVale", fe::FONT_BIG, rgb(255, 240, 200));
    fe::drawTextCentered(fb, FB_W / 2, 196, "La Ferme des Quatre Saisons", fe::FONT_SMALL, rgb(232, 193, 90));
    fe::drawTextCentered(fb, FB_W / 2, 218, "Un jeu de simulation de ferme complet · C++ natif", fe::FONT_SMALL, UI_DIM);

    // Boutons
    Ui ui(fb, plat->input, &audio);
    int bw = 260, bh = 34, bx = FB_W / 2 - bw / 2;
    if (ui.button(bx, 300, bw, bh, "Nouvelle partie")) {
        sim.newGame((uint64_t)time(nullptr) ^ 0xFA9AA11Eu);
        screen = Screen::PLAYING;
        audio.sfx(fe::Sfx::Button);
    }
    bool cont = hasSave;
    if (ui.button(bx, 342, bw, bh, "Continuer", cont)) {
        loadGame();
        audio.sfx(fe::Sfx::Button);
    }
    if (ui.button(bx, 384, bw, bh, "Aide")) {
        openMenu(Menu::HELP);
        audio.sfx(fe::Sfx::Button);
    }
    if (ui.button(bx, 426, bw, bh, "Quitter")) {
        exit(0);
    }
    fe::drawTextCentered(fb, FB_W / 2, FB_H - 24, "WASD/flèches: se déplacer · Espace: agir · Souris: pointer-cliquer", fe::FONT_SMALL, UI_DIM);
    // Capture headless de l'écran titre (une fois, puis quitter)
    if (captureTitleShot && !titleShotDone && frameCount > 90) {
        takeScreenshot("00_ecran_titre");
        titleShotDone = true;
        plat->requestQuit();
    }
}

} // namespace fv
