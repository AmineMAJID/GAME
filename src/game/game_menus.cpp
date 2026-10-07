#include "game.h"
#include "game_ui.h"
#include <cmath>
#include <cstdlib>

namespace fv {

using fe::rgb;

static const Color C_GOLD_SHOP = rgb(232, 193, 90);

// ------------------------------------------------------------
// Clic sur un slot d'inventaire (saisie / déplacement d'objet)
// ------------------------------------------------------------
void Game::handleSlotClick(vector<ItemStack>& bag, int idx) {
    if (idx < 0 || idx >= (int)bag.size()) return;
    if (!holdingItem) {
        if (!bag[idx].empty()) {
            holdingItem = true;
            heldStack = bag[idx];
            heldFrom = &bag;
            heldIndex = idx;
            bag[idx].clear();
            audio.sfx(fe::Sfx::Select);
        }
    } else {
        if (bag[idx].empty()) {
            bag[idx] = heldStack;
            holdingItem = false;
            heldStack.clear();
            heldFrom = nullptr;
            heldIndex = -1;
        } else if (bag[idx].sameKind(heldStack)) {
            const ItemDef* def = itemDef(heldStack.id);
            int maxS = def ? def->stack : MAX_STACK;
            int add = maxS - bag[idx].n;
            if (add > heldStack.n) add = heldStack.n;
            bag[idx].n += add;
            heldStack.n -= add;
            if (heldStack.n <= 0) {
                holdingItem = false;
                heldStack.clear();
                heldFrom = nullptr;
                heldIndex = -1;
            }
        } else {
            std::swap(bag[idx], heldStack);
        }
        audio.sfx(fe::Sfx::Select);
    }
}

void Game::renderHeldItem() {
    if (holdingItem && !heldStack.empty()) {
        const ItemDef* def = itemDef(heldStack.id);
        int icon = def ? def->icon : 0;
        int mx = plat->input.mouseX, my = plat->input.mouseY;
        fe::rect(fb, mx - 12, my - 12, 24, 24, UI_PANEL);
        fe::drawIcon(fb, mx - 8, my - 8, icon, heldStack.q);
        if (heldStack.n > 1)
            fe::drawTextShadow(fb, mx + 8, my + 6, std::to_string(heldStack.n), fe::FONT_SMALL, UI_TEXT);
    }
}

// ------------------------------------------------------------
// Dispatch des menus
// ------------------------------------------------------------
void Game::renderMenus() {
    switch (menu) {
        case Menu::SHOP: renderShop(); break;
        case Menu::INVENTORY: renderInventory(); break;
        case Menu::QUESTS: renderQuests(); break;
        case Menu::CHEST: renderChest(); break;
        case Menu::HELP: renderHelp(); break;
        case Menu::SLEEP_CONFIRM: renderSleepConfirm(); break;
        default: break;
    }
    renderHeldItem();
}

// ------------------------------------------------------------
// Panneau de confirmation "dormir"
// ------------------------------------------------------------
void Game::renderSleepConfirm() {
    Ui ui(fb, plat->input, &audio);
    int w = 320, h = 130, x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    ui.panel(x, y, w, h, "Dodo");
    fe::drawTextCentered(fb, FB_W / 2, y + 34, "Aller dormir ?", fe::FONT_SMALL, UI_TEXT);
    fe::drawTextCentered(fb, FB_W / 2, y + 52, "La journée passera et votre énergie sera restaurée.", fe::FONT_SMALL, UI_DIM);
    if (ui.button(x + 40, y + 76, 100, 30, "Oui")) {
        closeMenu();
        sim.sleepNow();
    }
    if (ui.button(x + 180, y + 76, 100, 30, "Non")) {
        closeMenu();
    }
}

// ------------------------------------------------------------
// Magasin
// ------------------------------------------------------------
void Game::renderShop() {
    Ui ui(fb, plat->input, &audio);
    int w = 850, h = 500, x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    ui.panel(x, y, w, h, "Marché");
    if (ui.closeButton(x + w - 26, y + 6)) { closeMenu(); return; }

    // --- Colonne achats ---
    int cx = x + 16, cy = y + 30;
    fe::drawText(fb, cx, cy, "Achats", fe::FONT_SMALL, UI_ACCENT);
    cy += 22;
    vector<ShopEntry> catalog = shopCatalog();
    for (auto& e : catalog) {
        int rowY = cy;
        bool isAnimal = e.itemId.rfind("animal:", 0) == 0;
        // ligne
        fe::rect(fb, cx, rowY, 400, 22, (rowY / 22) % 2 ? UI_SLOT : UI_PANEL);
        fe::drawIcon(fb, cx + 4, rowY + 3, e.icon, 0);
        fe::drawText(fb, cx + 26, rowY + 5, e.label, fe::FONT_SMALL, UI_TEXT);
        fe::drawText(fb, cx + 200, rowY + 5, std::to_string(e.price) + " p", fe::FONT_SMALL, C_GOLD_SHOP);
        bool canBuy = sim.st.money >= e.price;
        if (isAnimal && (int)sim.st.animals.size() >= MAX_ANIMALS) canBuy = false;
        if (ui.button(cx + 320, rowY + 1, 74, 20, "Acheter", canBuy)) {
            sim.buy(e.itemId);
        }
        cy += 24;
    }

    // --- Colonne ventes + améliorations ---
    int rx = x + 440, ry = y + 30;
    fe::drawText(fb, rx, ry, "Vendre", fe::FONT_SMALL, UI_ACCENT);
    ry += 22;
    // rassemble les objets vendables
    struct SellRow { string id; int q; int n; int total; };
    vector<SellRow> rows;
    for (auto* bag : {&sim.st.hotbar, &sim.st.backpack}) {
        for (auto& s : *bag) {
            if (s.empty()) continue;
            const ItemDef* def = itemDef(s.id);
            if (!def || def->sell <= 0) continue;
            bool found = false;
            for (auto& r : rows)
                if (r.id == s.id && r.q == s.q) { r.n += s.n; r.total += itemSellPrice(s.id, s.q) * s.n; found = true; }
            if (!found) rows.push_back({s.id, s.q, s.n, itemSellPrice(s.id, s.q) * s.n});
        }
    }
    for (auto& r : rows) {
        const ItemDef* def = itemDef(r.id);
        int icon = def ? def->icon : 0;
        fe::rect(fb, rx, ry, 390, 22, (ry / 22) % 2 ? UI_SLOT : UI_PANEL);
        fe::drawIcon(fb, rx + 4, ry + 3, icon, r.q);
        fe::drawText(fb, rx + 26, ry + 5, string(qualityName(r.q)) + (def ? def->name : r.id) + " x" + std::to_string(r.n), fe::FONT_SMALL, UI_TEXT);
        fe::drawText(fb, rx + 220, ry + 5, std::to_string(r.total) + " p", fe::FONT_SMALL, C_GOLD_SHOP);
        if (ui.button(rx + 316, ry + 1, 68, 20, "Vendre")) {
            // vend toutes les piles correspondantes
            for (auto* bag : {&sim.st.hotbar, &sim.st.backpack}) {
                for (size_t i = 0; i < bag->size(); i++) {
                    ItemStack& s = (*bag)[i];
                    if (!s.empty() && s.id == r.id && s.q == r.q) sim.sellStack(*bag, (int)i, s.n);
                }
            }
        }
        ry += 24;
        if (ry > y + h - 120) break;
    }

    // --- Améliorations ---
    int uy = y + h - 108;
    fe::drawText(fb, rx, uy, "Améliorations", fe::FONT_SMALL, UI_ACCENT);
    if (sim.st.canLevel < 3) {
        const CanUpgrade& up = CAN_UPGRADES[sim.st.canLevel];
        fe::drawText(fb, rx, uy + 20, string(up.name) + " — " + std::to_string(up.price) + " p", fe::FONT_SMALL, UI_TEXT);
        if (ui.button(rx + 300, uy + 16, 86, 20, "Acheter", sim.st.money >= up.price)) sim.buy("upg_can");
    } else {
        fe::drawText(fb, rx, uy + 20, "Arrosoir : niveau max", fe::FONT_SMALL, UI_GOOD);
    }
    if (sim.st.energyUp < 4) {
        static const int prices[4] = {1000, 2500, 5000, 10000};
        int price = prices[sim.st.energyUp];
        fe::drawText(fb, rx, uy + 44, "Énergie max +25 — " + std::to_string(price) + " p", fe::FONT_SMALL, UI_TEXT);
        if (ui.button(rx + 300, uy + 40, 86, 20, "Acheter", sim.st.money >= price)) sim.buy("upg_energy");
    } else {
        fe::drawText(fb, rx, uy + 44, "Énergie : niveau max", fe::FONT_SMALL, UI_GOOD);
    }
    fe::drawText(fb, rx, uy + 70, "Solde: " + std::to_string(sim.st.money) + " pièces", fe::FONT_SMALL, C_GOLD_SHOP);
}


// ------------------------------------------------------------
// Inventaire
// ------------------------------------------------------------
void Game::renderInventory() {
    Ui ui(fb, plat->input, &audio);
    int w = 780, h = 440, x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    ui.panel(x, y, w, h, "Sac à dos");
    if (ui.closeButton(x + w - 26, y + 6)) { closeMenu(); return; }

    // Grille du sac (6x4) à gauche
    int gx = x + 20, gy = y + 40, slot = 44, gap = 6;
    for (int i = 0; i < BACKPACK_SLOTS; i++) {
        int sx = gx + (i % 6) * (slot + gap);
        int sy = gy + (i / 6) * (slot + gap);
        if (ui.itemSlot(sx, sy, slot, sim.st.backpack[i])) handleSlotClick(sim.st.backpack, i);
    }
    // Barre rapide sous la grille (petites cases, même largeur que la grille)
    int hx = gx, hy = gy + 4 * (slot + gap) + 34;
    fe::drawText(fb, hx, hy - 20, "Barre rapide", fe::FONT_SMALL, UI_ACCENT);
    int hslot = 24, hgap = 1; // 12 * 25 = 300 = largeur de la grille
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int sx = hx + i * (hslot + hgap);
        bool sel = (i == sim.sel);
        if (ui.itemSlot(sx, hy, hslot, sim.st.hotbar[i], sel)) handleSlotClick(sim.st.hotbar, i);
    }

    // --- Panneau du joueur (droite) ---
    int px = x + 20 + 6 * (slot + gap) + 40, py = y + 40;
    fe::drawText(fb, px, py, "Compétences", fe::FONT_SMALL, UI_ACCENT);
    py += 22;
    for (int s = 0; s < SK_COUNT; s++) {
        int lv = sim.st.skillLevel(s);
        int xp = sim.st.skills[s];
        int cur = xp % SKILL_XP_PER_LEVEL;
        fe::drawText(fb, px, py, skillName(s), fe::FONT_SMALL, UI_TEXT);
        fe::drawText(fb, px + 150, py, " niv " + std::to_string(lv), fe::FONT_SMALL, UI_ACCENT);
        // barre de progression
        fe::rect(fb, px, py + 14, 170, 6, UI_SLOT);
        if (lv < SKILL_MAX_LEVEL)
            fe::rect(fb, px, py + 14, (int)(170.0f * cur / SKILL_XP_PER_LEVEL), 6, UI_GOOD);
        py += 30;
    }
    py += 6;
    fe::drawText(fb, px, py, "Statistiques", fe::FONT_SMALL, UI_ACCENT);
    py += 20;
    char buf[128];
    snprintf(buf, sizeof(buf), "Jours vécus: %llu", (unsigned long long)sim.st.statDaysPlayed);
    fe::drawText(fb, px, py, buf, fe::FONT_SMALL, UI_TEXT); py += 18;
    snprintf(buf, sizeof(buf), "Récoltes: %llu", (unsigned long long)sim.st.statCropsHarvested);
    fe::drawText(fb, px, py, buf, fe::FONT_SMALL, UI_TEXT); py += 18;
    snprintf(buf, sizeof(buf), "Animaux élevés: %llu", (unsigned long long)sim.st.statAnimalsRaised);
    fe::drawText(fb, px, py, buf, fe::FONT_SMALL, UI_TEXT); py += 18;
    snprintf(buf, sizeof(buf), "Poissons pêchés: %llu", (unsigned long long)sim.st.statFishCaught);
    fe::drawText(fb, px, py, buf, fe::FONT_SMALL, UI_TEXT); py += 18;
    snprintf(buf, sizeof(buf), "Pièces gagnées: %llu", (unsigned long long)sim.st.statMoneyEarned);
    fe::drawText(fb, px, py, buf, fe::FONT_SMALL, UI_TEXT); py += 18;
    snprintf(buf, sizeof(buf), "Énergie max: %d", sim.st.energyMaxBase());
    fe::drawText(fb, px, py, buf, fe::FONT_SMALL, UI_TEXT); py += 24;
    fe::drawText(fb, px, py, "Astuce: sélectionnez de la nourriture", fe::FONT_SMALL, UI_DIM); py += 16;
    fe::drawText(fb, px, py, "et appuyez sur E pour manger (énergie).", fe::FONT_SMALL, UI_DIM);
}

// ------------------------------------------------------------
// Coffre de rangement (maison)
// ------------------------------------------------------------
void Game::renderChest() {
    Ui ui(fb, plat->input, &audio);
    int w = 560, h = 330, x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    ui.panel(x, y, w, h, "Coffre de rangement");
    if (ui.closeButton(x + w - 26, y + 6)) { closeMenu(); return; }

    int gx = x + 20, gy = y + 44, slot = 52, gap = 8;
    fe::drawText(fb, gx, y + 26, "Coffre (24 places) — cliquez pour déplacer", fe::FONT_SMALL, UI_DIM);
    for (int i = 0; i < STORAGE_SLOTS; i++) {
        int sx = gx + (i % 8) * (slot + gap);
        int sy = gy + (i / 8) * (slot + gap);
        if (ui.itemSlot(sx, sy, slot, sim.st.storage[i])) handleSlotClick(sim.st.storage, i);
    }
    int hy = gy + 3 * (slot + gap) + 16;
    fe::drawText(fb, gx, hy - 22, "Barre rapide", fe::FONT_SMALL, UI_ACCENT);
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int sx = gx + i * (slot + gap);
        if (ui.itemSlot(sx, hy, slot, sim.st.hotbar[i], i == sim.sel)) handleSlotClick(sim.st.hotbar, i);
    }
}

// ------------------------------------------------------------
// Quêtes
// ------------------------------------------------------------
void Game::renderQuests() {
    Ui ui(fb, plat->input, &audio);
    int w = 620, h = 300, x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    ui.panel(x, y, w, h, "Quêtes du jour");
    if (ui.closeButton(x + w - 26, y + 6)) { closeMenu(); return; }

    int qy = y + 44;
    if (sim.st.quests.empty()) {
        fe::drawTextCentered(fb, FB_W / 2, qy + 40, "Aucune quête aujourd'hui.", fe::FONT_SMALL, UI_DIM);
        return;
    }
    for (auto& q : sim.st.quests) {
        fe::drawIcon(fb, x + 20, qy + 2, IC_QUEST, 0);
        fe::drawText(fb, x + 44, qy + 4, q.text, fe::FONT_SMALL, q.done ? UI_GOOD : UI_TEXT);
        // barre de progression
        float f = q.target > 0 ? (float)q.progress / q.target : 0;
        if (f > 1) f = 1;
        fe::rect(fb, x + 44, qy + 22, 300, 8, UI_SLOT);
        fe::rect(fb, x + 44, qy + 22, (int)(300 * f), 8, q.done ? UI_GOOD : UI_ACCENT);
        char buf[64];
        snprintf(buf, sizeof(buf), "%d/%d", q.progress, q.target);
        fe::drawText(fb, x + 352, qy + 22, buf, fe::FONT_SMALL, UI_DIM);
        snprintf(buf, sizeof(buf), "+%d p", q.rewardMoney);
        fe::drawText(fb, x + 430, qy + 4, buf, fe::FONT_SMALL, C_GOLD_SHOP);
        if (q.rewardXp > 0)
            fe::drawText(fb, x + 430, qy + 20, "+" + std::to_string(q.rewardXp) + " XP", fe::FONT_SMALL, UI_GOOD);
        if (q.done) fe::drawText(fb, x + 500, qy + 4, "TERMINÉE", fe::FONT_SMALL, UI_GOOD);
        qy += 56;
    }
    fe::drawText(fb, x + 20, y + h - 30, "De nouvelles quêtes arrivent chaque matin.", fe::FONT_SMALL, UI_DIM);
}

// ------------------------------------------------------------
// Aide
// ------------------------------------------------------------
void Game::renderHelp() {
    Ui ui(fb, plat->input, &audio);
    int w = 660, h = 440, x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    ui.panel(x, y, w, h, "Aide — Comment jouer");
    if (ui.closeButton(x + w - 26, y + 6)) { closeMenu(); return; }

    int ty = y + 36;
    auto line = [&](const char* txt, Color c) {
        fe::drawText(fb, x + 24, ty, txt, fe::FONT_SMALL, c);
        ty += 20;
    };
    line("CONTROLS", UI_ACCENT);
    line("  WASD / flèches : se déplacer", UI_TEXT);
    line("  Espace / clic souris : agir (contextuel)", UI_TEXT);
    line("  1..0, -, = : sélectionner la barre rapide", UI_TEXT);
    line("  B / Tab : inventaire   ·   E : manger", UI_TEXT);
    line("  O : sauvegarder   ·   M : musique   ·   F : plein écran", UI_TEXT);
    line("  Échap : fermer / pause", UI_TEXT);
    ty += 8;
    line("CULTURES", UI_ACCENT);
    line("  Houe : labourer l'herbe.  Graines : planter sur la terre labourée.", UI_TEXT);
    line("  Arrosez chaque jour (la pluie aide).  Engrais : pousse +33%.", UI_TEXT);
    line("  Récoltez quand l'étoile brille.  Qualité argent/or = prix x1.25/x1.5.", UI_TEXT);
    line("  Chaque culture a sa saison ; l'hiver, tout gèle.", UI_TEXT);
    ty += 8;
    line("ANIMAUX", UI_ACCENT);
    line("  Nourrissez-les (foin, maïs pour les poules) et câlinez-les chaque jour.", UI_TEXT);
    line("  Seau : œufs et lait.  Tondeuse : laine.  Amitié >= 60 : reproduction !", UI_TEXT);
    line("  Deux adultes de même espèce peuvent avoir un bébé.", UI_TEXT);
    ty += 8;
    line("AUTRES", UI_ACCENT);
    line("  Hache : couper les arbres (repoussent).  Faux : foin dans l'herbe.", UI_TEXT);
    line("  Canne à pêche à l'étang (mini-jeu).  Panneau : quêtes du jour.", UI_TEXT);
    line("  Lit : dormir (ou s'évanouir à 2h du matin…).  Caisse : tout vendre.", UI_TEXT);
}

// ------------------------------------------------------------
// Menu pause
// ------------------------------------------------------------
void Game::renderPauseMenu() {
    fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(8, 6, 12), 170);
    Ui ui(fb, plat->input, &audio);
    int w = 340, h = 420, x = (FB_W - w) / 2, y = (FB_H - h) / 2;
    ui.panel(x, y, w, h, "Pause");
    int bw = 260, bx = x + (w - bw) / 2, by = y + 36;
    auto btn = [&](const char* label, int dy) {
        if (ui.button(bx, by + dy, bw, 30, label)) return true;
        return false;
    };
    if (btn("Reprendre", 0)) { screen = Screen::PLAYING; }
    if (btn("Sauvegarder", 40)) saveGame();
    if (btn("Charger", 80)) loadGame();
    if (btn("Aide", 120)) { screen = Screen::PLAYING; openMenu(Menu::HELP); }
    if (btn(audio.musicOn ? "Musique : ON" : "Musique : OFF", 160)) audio.musicOn = !audio.musicOn;
    if (btn(plat->isFullscreen() ? "Fenêtré" : "Plein écran", 200)) plat->setFullscreen(!plat->isFullscreen());
    if (btn("Retour au titre", 240)) { screen = Screen::TITLE; menu = Menu::NONE; }
    if (btn("Quitter", 280)) { saveGame(); exit(0); }

    // Volume
    int vy = y + 320;
    fe::drawText(fb, bx, vy, "Volume", fe::FONT_SMALL, UI_TEXT);
    fe::rect(fb, bx, vy + 20, bw, 8, UI_SLOT);
    fe::rectOutline(fb, bx, vy + 20, bw, 8, UI_BORDER);
    int hx = bx + (int)(bw * audio.master);
    fe::rect(fb, hx - 5, vy + 16, 10, 16, UI_ACCENT);
    if (ui.hover(bx, vy + 14, bw, 20) && plat->input.mouseDown) {
        float f = (float)(plat->input.mouseX - bx) / bw;
        audio.master = std::max(0.0f, std::min(1.0f, f));
    }
}

} // namespace fv
