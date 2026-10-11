#include "game.h"
#include <cmath>

namespace fv {

// ------------------------------------------------------------
// Choix du poisson selon saison / météo / heure / compétence
// ------------------------------------------------------------
void Game::pickFish() {
    float roach = 40, trout = 25, carp = 25, pike = 10;
    int s = sim.st.season();
    if (s == WINTER) { trout = 0; carp = 5; pike = 30; roach = 30; }
    if (s == SPRING) { pike = 5; }
    if (s == AUTUMN) { trout = 10; }
    if (sim.isRaining()) carp *= 3.0f;
    if (sim.st.isNight()) pike *= 2.5f;
    float skill = (float)sim.st.skillLevel(SK_FISHING);
    pike += skill * 2.0f;
    trout += skill * 1.5f;

    float total = roach + trout + carp + pike;
    float r = sim.rng.uf() * total;
    const char* id = "fish_roach";
    if ((r -= roach) < 0) id = "fish_roach";
    else if ((r -= trout) < 0) id = "fish_trout";
    else if ((r -= carp) < 0) id = "fish_carp";
    else id = "fish_pike";
    fishCatchId = id;

    fishQuality = 0;
    float qr = sim.rng.uf();
    if (qr < 0.03f + skill * 0.02f) fishQuality = 2;
    else if (qr < 0.12f + skill * 0.05f) fishQuality = 1;
}

// ------------------------------------------------------------
// Bot de démonstration (mode headless : joue et prend des captures)
// ------------------------------------------------------------
void Game::botUpdate(float dt) {
    botTimer += dt;
    if (botTimer < 0.12f) return;
    botTimer = 0;

    // Le bot a de l'énergie et un peu de capital (démonstration)
    sim.st.energy = sim.st.energyMaxBase();
    if (sim.st.money < 8000) sim.st.money = 8000;

    auto gotoTile = [&](int x, int y) {
        sim.st.mapId = "farm"; // dormir nous renvoie à la maison : on revient dehors
        sim.st.px = x * TILE + TILE / 2.0f;
        sim.st.py = y * TILE + TILE / 2.0f;
    };
    auto waterField = [&](int x0, int x1, int y0, int y1) {
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                gotoTile(x, y);
                int canSlot = findItem(sim.st.hotbar, "tool_can");
                if (canSlot >= 0) { sim.sel = canSlot; sim.actionAt(x, y); }
            }
    };
    auto careAnimals = [&]() {
        if (sim.countItem("hay") < 100) sim.addItem("hay", 0, 50); // stock de démo
        for (size_t i = 0; i < sim.st.animals.size(); i++) {
            sim.feedAnimal((int)i, "hay");
            sim.petAnimal((int)i);
            if (sim.st.animals[i].ready > 0)
                sim.collectAnimal((int)i, sim.st.animals[i].type == "sheep" ? "tool_shears" : "tool_pail");
        }
    };

    int day = sim.st.day;

    // Travail du jour selon le calendrier
    switch (day) {
        case 1: // défriche et plante un champ de blé 4x3
            for (int y = 12; y <= 14; y++)
                for (int x = 12; x <= 15; x++) {
                    gotoTile(x, y);
                    sim.sel = 0;
                    sim.actionAt(x, y);
                    sim.sel = 2;
                    sim.actionAt(x, y);
                    sim.sel = 1;
                    sim.actionAt(x, y);
                }
            break;
        case 2: // arrose + fait les courses
            waterField(12, 15, 12, 14);
            sim.buy("animal:chicken");
            sim.buy("animal:chicken");
            sim.buy("animal:cow");
            sim.buy("tool_pail");
            sim.buy("tool_axe");
            careAnimals();
            break;
        case 3: // arrose + plante des fraises
            waterField(12, 15, 12, 14);
            for (int y = 16; y <= 17; y++)
                for (int x = 12; x <= 15; x++) {
                    gotoTile(x, y);
                    int seedSlot = findItem(sim.st.hotbar, "seed_strawberry");
                    if (seedSlot < 0) { sim.buy("seed_strawberry"); seedSlot = findItem(sim.st.hotbar, "seed_strawberry"); }
                    if (seedSlot >= 0) { sim.sel = seedSlot; sim.actionAt(x, y); }
                    int canSlot = findItem(sim.st.hotbar, "tool_can");
                    if (canSlot >= 0) { sim.sel = canSlot; sim.actionAt(x, y); }
                }
            careAnimals();
            break;
        case 4: // arrose tout
            waterField(12, 15, 12, 14);
            waterField(12, 15, 16, 17);
            careAnimals();
            break;
        case 5: // achète un mouton + tondeuse
            sim.buy("animal:sheep");
            sim.buy("tool_shears");
            waterField(12, 15, 12, 14);
            waterField(12, 15, 16, 17);
            careAnimals();
            break;
        case 6: // arrose + récolte le blé mûr + expédie
            waterField(12, 15, 12, 14);
            waterField(12, 15, 16, 17);
            for (int y = 12; y <= 14; y++)
                for (int x = 12; x <= 15; x++) {
                    gotoTile(x, y);
                    Plot* p = sim.plotAt(x, y);
                    if (p && !p->crop.empty() && !p->dead) {
                        const CropDef* c = cropDef(p->crop);
                        if (c && p->progress >= c->days) sim.harvestAt(x, y);
                    }
                }
            careAnimals();
            break;
        case 7: // repos : on force la pluie pour la capture
            sim.st.weather = W_RAIN;
            waterField(12, 15, 12, 14);
            careAnimals();
            break;
        case 8: // dernier jour : capture de la famille
            careAnimals();
            break;
    }

    // Toujours nourrir les animaux
    careAnimals();

    // Captures programmées (le bot fixe l'heure/météo pour la photo)
    auto shoot = [&](int d, int hour, int weather, int ptx, int pty, int facing, const char* name) {
        if (sim.st.day == d && lastShotDay != d) {
            sim.st.minutes = hour * 60;
            sim.st.weather = weather;
            gotoTile(ptx, pty);
            sim.st.facing = facing;
            lastShotDay = d;
            pendingShot = name;
        }
    };
    shoot(4, 12, W_SUN,   13, 15, FACING_DOWN,  "01_ferme_midi");
    shoot(6, 18, W_CLOUD, 14, 13, FACING_RIGHT, "02_soir");
    shoot(7, 14, W_RAIN,  13, 13, FACING_DOWN,  "03_pluie");
    shoot(8, 22, W_SUN,   17, 24, FACING_LEFT,  "04_nuit_paturage");
    shoot(9, 10, W_SUN,   16, 23, FACING_UP,    "05_famille");

    // ---- Jour 10+ : captures du quartier, des menus et de l'intérieur ----
    if (sim.st.day >= 10) {
        if (lastShotDay != 10) {
            sim.st.minutes = 10 * 60;
            sim.st.weather = W_SUN;
            gotoTile(24, 4);
            sim.st.facing = FACING_RIGHT;
            lastShotDay = 10;
            pendingShot = "06_quartier_maison";
            menu = Menu::NONE;
        } else if (shotsTaken == 6) {
            menu = Menu::SHOP;
            pendingShot = "07_menu_magasin";
        } else if (shotsTaken == 7) {
            menu = Menu::INVENTORY;
            pendingShot = "08_menu_inventaire";
        } else if (shotsTaken == 8) {
            menu = Menu::QUESTS;
            pendingShot = "09_menu_quetes";
        } else if (shotsTaken == 9) {
            menu = Menu::NONE;
            sim.st.mapId = "home";
            sim.st.px = 4 * TILE + TILE / 2.0f;
            sim.st.py = 6 * TILE + TILE / 2.0f;
            sim.st.facing = FACING_DOWN;
            pendingShot = "10_interieur_maison";
        } else if (shotsTaken >= 10) {
            botMode = false;
        }
        return; // pas de dodo pendant les captures des menus
    }
    // Dodo : la journée de travail est finie (sauf si une capture est en attente)
    if (pendingShot.empty()) {
        sim.sleepNow();
    }
}

} // namespace fv
