#include "game.h"
#include "game_ui.h"
#include <cmath>

namespace fv {

using fe::rgb;

// Palette monde
static const Color C_GRASS   = rgb(86, 145, 66);
static const Color C_GRASS_D = rgb(70, 118, 54);
static const Color C_GRASS_L = rgb(108, 168, 84);
static const Color C_PATH    = rgb(196, 164, 116);
static const Color C_PATH_D  = rgb(168, 138, 94);
static const Color C_SAND    = rgb(226, 208, 164);
static const Color C_WATER   = rgb(64, 128, 178);
static const Color C_WATER_L = rgb(110, 175, 220);
static const Color C_SOIL    = rgb(122, 84, 52);
static const Color C_SOIL_D  = rgb(96, 64, 40);
static const Color C_SOIL_WET= rgb(84, 56, 36);
static const Color C_BLUE_L  = rgb(127, 184, 232);
static const Color C_WOOD    = rgb(139, 90, 43);
static const Color C_WOOD_D  = rgb(96, 60, 30);
static const Color C_WOOD_L  = rgb(176, 123, 63);
static const Color C_LEAF    = rgb(52, 120, 60);
static const Color C_LEAF_D  = rgb(38, 88, 44);
static const Color C_LEAF_L  = rgb(88, 158, 80);
static const Color C_WALL    = rgb(228, 214, 186);
static const Color C_ROOF    = rgb(172, 62, 54);
static const Color C_ROOF_D  = rgb(130, 44, 38);
static const Color C_FLOOR   = rgb(176, 128, 78);
static const Color C_FLOOR_D = rgb(150, 106, 62);
static const Color C_WHITE   = rgb(245, 245, 240);
static const Color C_BLACK   = rgb(28, 26, 32);
static const Color C_SKIN    = rgb(240, 200, 160);
static const Color C_HAIR    = rgb(90, 60, 40);
static const Color C_SHIRT   = rgb(70, 130, 140);
static const Color C_PANTS   = rgb(70, 80, 130);
static const Color C_RED     = rgb(200, 60, 60);
static const Color C_ORANGE  = rgb(230, 140, 50);
static const Color C_GOLD    = rgb(235, 200, 100);
static const Color C_PINK    = rgb(240, 170, 180);
static const Color C_GRAY    = rgb(120, 120, 128);
static const Color C_CREAM   = rgb(240, 236, 224);

static inline int clampi(int v, int a, int b) { return v < a ? a : (v > b ? b : v); }

int Game::tileHash(int x, int y) const {
    uint32_t h = (uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)sim.st.seed * 83492791u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (int)(h & 0x7FFFFFFF);
}

// ------------------------------------------------------------
// Rendu du monde complet
// ------------------------------------------------------------
void Game::renderWorld() {
    MapData& m = sim.world.current(sim.st);
    fb.clear(rgb(20, 24, 30));

    int tx0 = std::max(0, camX / TILE - 1);
    int ty0 = std::max(0, camY / TILE - 1);
    int tx1 = std::min(m.w - 1, (camX + FB_W) / TILE + 1);
    int ty1 = std::min(m.h - 1, (camY + FB_H) / TILE + 1);

    renderGround();

    // Entités triées par Y pour la profondeur
    struct Ent { int y; int kind; int x; int ty; const Animal* a; const Node* n; };
    vector<Ent> ents;

    // Parcelles (culture) — uniquement sur la carte ferme
    if (sim.st.mapId == "farm") {
        for (int ty = ty0; ty <= ty1; ty++) {
            for (int tx = tx0; tx <= tx1; tx++) {
                const Plot* p = sim.plotAt(tx, ty);
                if (p && !p->crop.empty()) {
                    int sx = tx * TILE - camX, sy = ty * TILE - camY;
                    renderPlot(tx, ty, *p, sx, sy);
                }
            }
        }
    }

    // Fences (basse profondeur)
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (m.o(tx, ty) == T_FENCE)
                renderFence(tx * TILE - camX, ty * TILE - camY);

    // Entités dynamiques (coordonnées écran pour un tri cohérent)
    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            if (sim.st.mapId == "farm") {
                auto it = sim.st.nodes.find(tileKey(tx, ty));
                if (it != sim.st.nodes.end())
                    ents.push_back({ty * TILE - camY + 30, 0, tx, ty, nullptr, &it->second});
            }
            uint8_t o = m.o(tx, ty);
            if (o == T_BOARD || o == T_BIN || (o == T_STALL && tx == 36 && ty == 3))
                ents.push_back({ty * TILE - camY + 30, 1, tx, ty, nullptr, nullptr});
        }
    }
    // Animaux
    for (auto& a : sim.st.animals) {
        int ax = (int)a.x - camX, ay = (int)a.y - camY;
        if (ax < -40 || ax > FB_W + 40 || ay < -40 || ay > FB_H + 40) continue;
        ents.push_back({(int)a.y - camY, 2, ax, 0, &a, nullptr});
    }
    // Joueur
    ents.push_back({(int)sim.st.py - camY, 3, 0, 0, nullptr, nullptr});

    std::sort(ents.begin(), ents.end(), [](const Ent& a, const Ent& b) { return a.y < b.y; });

    for (auto& e : ents) {
        if (e.kind == 0) {
            int sx = e.x * TILE - camX, sy = e.ty * TILE - camY;
            if (e.n->kind == "tree" || e.n->kind == "stump") renderTree(sx, sy, *e.n, e.x, e.ty);
            else renderBush(sx, sy, *e.n);
        } else if (e.kind == 1) {
            int sx = e.x * TILE - camX, sy = e.ty * TILE - camY;
            uint8_t o = m.o(e.x, e.ty);
            if (o == T_STALL) renderStall(sx, sy);
            else if (o == T_BOARD) renderBoard(sx, sy);
            else renderBin(sx, sy);
        } else if (e.kind == 2) {
            renderAnimal(*e.a, e.x, e.y);
        } else {
            renderPlayer((int)sim.st.px - camX, (int)sim.st.py - camY);
        }
    }

    // Maison (toujours dessinée par-dessus selon sa position en Y)
    if (sim.st.mapId == "farm") {
        renderHouse(5 * TILE - camX, 2 * TILE - camY);
        renderStructures();
    } else {
        renderBed(2 * TILE - camX, 2 * TILE - camY);
        renderChest(6 * TILE - camX, 2 * TILE - camY);
    }

    renderHearts();
    renderWeather();
    renderLighting();
}

// ------------------------------------------------------------
// Sol
// ------------------------------------------------------------
void Game::renderGround() {
    MapData& m = sim.world.current(sim.st);
    int tx0 = std::max(0, camX / TILE - 1);
    int ty0 = std::max(0, camY / TILE - 1);
    int tx1 = std::min(m.w - 1, (camX + FB_W) / TILE + 1);
    int ty1 = std::min(m.h - 1, (camY + FB_H) / TILE + 1);
    double t = plat->timeMs() / 1000.0;

    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            int sx = tx * TILE - camX, sy = ty * TILE - camY;
            uint8_t g = m.g(tx, ty);
            int h = tileHash(tx, ty);
            switch (g) {
                case T_GRASS: {
                    fe::rect(fb, sx, sy, TILE, TILE, C_GRASS);
                    // herbe haute (petites touffes)
                    for (int i = 0; i < 3; i++) {
                        int hx = sx + (h >> (i * 5)) % 28 + 2;
                        int hy = sy + (h >> (i * 7 + 3)) % 28 + 2;
                        fe::px(fb, hx, hy, C_GRASS_D);
                        fe::px(fb, hx + 1, hy, C_GRASS_L);
                        fe::px(fb, hx, hy + 1, C_GRASS_D);
                    }
                    // fleurs rares (deterministes)
                    if (h % 23 == 0) {
                        int fx = sx + (h >> 8) % 24 + 4;
                        int fy = sy + (h >> 13) % 24 + 4;
                        Color fc = (h % 2) ? rgb(255, 255, 240) : rgb(255, 220, 90);
                        fe::px(fb, fx, fy, fc);
                        fe::px(fb, fx + 1, fy, fc);
                        fe::px(fb, fx, fy + 1, fc);
                        fe::px(fb, fx + 1, fy + 1, rgb(230, 200, 60));
                    }
                    break;
                }
                case T_PATH: {
                    fe::rect(fb, sx, sy, TILE, TILE, C_PATH);
                    for (int i = 0; i < 2; i++) {
                        int hx = sx + (h >> (i * 6)) % 26 + 3;
                        int hy = sy + (h >> (i * 8 + 2)) % 26 + 3;
                        fe::rect(fb, hx, hy, 3, 2, C_PATH_D);
                    }
                    break;
                }
                case T_SAND: {
                    fe::rect(fb, sx, sy, TILE, TILE, C_SAND);
                    for (int i = 0; i < 2; i++) {
                        int hx = sx + (h >> (i * 6)) % 26 + 3;
                        int hy = sy + (h >> (i * 8 + 2)) % 26 + 3;
                        fe::px(fb, hx, hy, rgb(205, 185, 140));
                        fe::px(fb, hx + 1, hy + 1, rgb(240, 225, 190));
                    }
                    break;
                }
                case T_POND: {
                    fe::rect(fb, sx, sy, TILE, TILE, C_WATER);
                    // reflets animés
                    float sh = (float)std::sin(t * 2.0 + tx * 1.3 + ty * 0.7);
                    fe::hline(fb, sx + 4 + (int)(sh * 3), sx + 14 + (int)(sh * 3), sy + 8 + (h % 3) * 7, C_WATER_L);
                    fe::hline(fb, sx + 16 - (int)(sh * 2), sx + 26 - (int)(sh * 2), sy + 20 + (h % 2) * 6, rgb(90, 155, 200));
                    break;
                }
                case T_SOIL: {
                    const Plot* p = sim.plotAt(tx, ty);
                    Color c = (p && p->watered) ? C_SOIL_WET : C_SOIL;
                    fe::rect(fb, sx, sy, TILE, TILE, c);
                    // sillons
                    for (int i = 1; i < 4; i++)
                        fe::hline(fb, sx + 2, sx + TILE - 3, sy + i * 8, C_SOIL_D);
                    if (p && p->watered) {
                        fe::hline(fb, sx + 4, sx + 12, sy + 4, rgb(120, 90, 60));
                        fe::hline(fb, sx + 18, sx + 27, sy + 25, rgb(120, 90, 60));
                    }
                    break;
                }
                case T_FLOOR: {
                    fe::rect(fb, sx, sy, TILE, TILE, C_FLOOR);
                    // planches
                    fe::hline(fb, sx, sx + TILE - 1, sy, C_FLOOR_D);
                    if ((tx + ty) % 2 == 0) fe::vline(fb, sx + 16, sy + 1, sy + TILE - 1, C_FLOOR_D);
                    break;
                }
                default:
                    fe::rect(fb, sx, sy, TILE, TILE, C_GRASS);
                    break;
            }
        }
    }
}

// ------------------------------------------------------------
// Parcelle (inutile visuellement ici, le sol est déjà dessiné)
// ------------------------------------------------------------
void Game::renderPlot(int tx, int ty, const Plot& p, int sx, int sy) {
    (void)tx; (void)ty;
    renderCropSprite(sx, sy, p.crop, p.progress, p.dead, false);
    // engrais : petites pépites dorées au sol
    if (p.fert) {
        fe::px(fb, sx + 5, sy + 26, C_GOLD);
        fe::px(fb, sx + 25, sy + 24, C_GOLD);
        fe::px(fb, sx + 15, sy + 28, rgb(200, 160, 60));
    }
}

// ------------------------------------------------------------
// Sprite de culture (16x16 -> 32x32, en 4 étapes)
// ------------------------------------------------------------
void Game::renderCropSprite(int sx, int sy, const string& crop, float progress, bool dead, bool ready) {
    const CropDef* c = cropDef(crop);
    if (!c) return;
    float frac = progress / c->days; // 0..1+
    if (dead) {
        // plant fané
        fe::vline(fb, sx + 16, sy + 14, sy + 28, rgb(120, 100, 60));
        fe::px(fb, sx + 14, sy + 18, rgb(100, 85, 50));
        fe::px(fb, sx + 18, sy + 18, rgb(100, 85, 50));
        fe::px(fb, sx + 16, sy + 12, rgb(90, 75, 45));
        return;
    }
    int cx = sx + 16;
    float sway = (float)std::sin(plat->timeMs() / 700.0 + sx) * 1.2f;
    bool mature = frac >= 1.0f;

    if (crop == "wheat") {
        int h = 6 + (int)(frac * 18);
        fe::vline(fb, cx, sy + 30 - h, sy + 30, C_LEAF_D);
        if (frac > 0.4f) {
            for (int i = 0; i < 3; i++) {
                int gy = sy + 30 - h + 4 + i * 5;
                fe::rect(fb, cx - 3, gy, 2, 3, mature ? C_GOLD : C_LEAF_L);
                fe::rect(fb, cx + 1, gy, 2, 3, mature ? C_GOLD : C_LEAF_L);
            }
        }
        if (mature) {
            for (int i = 0; i < 4; i++) {
                int gy = sy + 30 - h + i * 4;
                fe::px(fb, cx - 1, gy, C_GOLD);
                fe::px(fb, cx + 1, gy, C_GOLD);
                fe::px(fb, cx, gy - 2, C_GOLD);
            }
        }
    } else if (crop == "potato") {
        int bush = 4 + (int)(frac * 12);
        fe::circle(fb, cx - 4, sy + 30 - bush / 2, bush / 2 + 2, C_LEAF);
        fe::circle(fb, cx + 4, sy + 30 - bush / 2 + 1, bush / 2 + 1, C_LEAF_D);
        fe::circle(fb, cx, sy + 30 - bush / 2 - 2, bush / 2, C_LEAF_L);
        if (mature) {
            fe::circle(fb, cx - 5, sy + 28, 2, rgb(200, 165, 110));
            fe::circle(fb, cx + 5, sy + 28, 2, rgb(200, 165, 110));
            fe::circle(fb, cx, sy + 29, 2, rgb(220, 185, 130));
        }
    } else if (crop == "strawberry") {
        fe::circle(fb, cx - 5, sy + 26, 5, C_LEAF);
        fe::circle(fb, cx + 5, sy + 26, 5, C_LEAF_D);
        fe::circle(fb, cx, sy + 24, 6, C_LEAF_L);
        if (frac > 0.75f) {
            fe::circle(fb, cx - 5, sy + 27, 2, C_RED);
            fe::circle(fb, cx + 5, sy + 27, 2, C_RED);
            fe::circle(fb, cx, sy + 29, 2, C_RED);
        }
    } else if (crop == "corn") {
        int h = 6 + (int)(frac * 22);
        fe::vline(fb, cx + (int)sway, sy + 30 - h, sy + 30, C_LEAF_D);
        if (frac > 0.3f) {
            fe::rect(fb, cx - 6, sy + 30 - h + 8, 5, 3, C_LEAF);
            fe::rect(fb, cx + 2, sy + 30 - h + 12, 5, 3, C_LEAF);
        }
        if (mature) {
            fe::rect(fb, cx - 2, sy + 30 - h + 2, 5, 10, C_GOLD);
            fe::rect(fb, cx - 2, sy + 30 - h + 2, 5, 2, rgb(180, 140, 50));
            fe::px(fb, cx + 3, sy + 30 - h, C_LEAF_L);
        }
    } else if (crop == "tomato") {
        fe::vline(fb, cx, sy + 30 - (int)(frac * 14), sy + 30, C_LEAF_D);
        fe::circle(fb, cx - 6, sy + 26, 4, C_LEAF);
        fe::circle(fb, cx + 6, sy + 24, 4, C_LEAF);
        if (frac > 0.6f) {
            fe::circle(fb, cx - 4, sy + 25, 2, C_RED);
            fe::circle(fb, cx + 4, sy + 27, 2, C_RED);
            if (mature) fe::circle(fb, cx + 1, sy + 22, 2, C_RED);
        }
    } else if (crop == "sunflower") {
        int h = 6 + (int)(frac * 20);
        fe::vline(fb, cx + (int)sway, sy + 30 - h, sy + 30, C_LEAF_D);
        if (frac > 0.3f) {
            fe::rect(fb, cx - 5, sy + 30 - h + 10, 4, 2, C_LEAF);
            fe::rect(fb, cx + 2, sy + 30 - h + 14, 4, 2, C_LEAF);
        }
        if (mature) {
            int fy = sy + 30 - h;
            for (int i = 0; i < 8; i++) {
                float a = i * 0.7854f;
                fe::px(fb, cx + 1 + (int)(7 * std::cos(a)), fy + (int)(7 * std::sin(a)), C_GOLD);
            }
            fe::circle(fb, cx + 1, fy, 4, rgb(90, 60, 30));
            fe::px(fb, cx, fy - 1, C_BLACK);
            fe::px(fb, cx + 2, fy - 1, C_BLACK);
        }
    } else if (crop == "pumpkin") {
        fe::circle(fb, cx - 6, sy + 27, 5, C_LEAF);
        fe::circle(fb, cx + 6, sy + 27, 5, C_LEAF_D);
        fe::circle(fb, cx, sy + 25, 6, C_LEAF_L);
        if (mature) {
            fe::circle(fb, cx, sy + 26, 7, C_ORANGE);
            fe::vline(fb, cx, sy + 19, sy + 33, rgb(180, 100, 30));
            fe::vline(fb, cx - 4, sy + 21, sy + 31, rgb(200, 120, 45));
            fe::vline(fb, cx + 4, sy + 21, sy + 31, rgb(200, 120, 45));
            fe::rect(fb, cx - 1, sy + 16, 3, 4, C_LEAF_D);
        }
    } else if (crop == "kale") {
        int h = 5 + (int)(frac * 12);
        fe::circle(fb, cx - 5, sy + 30 - h / 2, 5, C_LEAF);
        fe::circle(fb, cx + 5, sy + 30 - h / 2 + 1, 5, C_LEAF_D);
        fe::circle(fb, cx, sy + 30 - h / 2 - 2, 6, C_LEAF_L);
        fe::vline(fb, cx, sy + 30 - h + 4, sy + 30, rgb(150, 190, 140));
    }

    // étincelle si prêt à récolter
    if (mature) {
        float tw = (float)std::fmod(plat->timeMs() / 500.0, 1.0);
        int sx2 = sx + 24 + (int)(tw * 6);
        int sy2 = sy + 6 - (int)(tw * 6);
        fe::px(fb, sx2, sy2, C_GOLD);
        fe::px(fb, sx2 + 1, sy2, C_GOLD);
        fe::px(fb, sx2, sy2 + 1, C_GOLD);
    }
    (void)ready;
}

// ------------------------------------------------------------
// Arbre / souche
// ------------------------------------------------------------
void Game::renderTree(int sx, int sy, const Node& n, int tx, int ty) {
    if (n.kind == "stump") {
        fe::rect(fb, sx + 11, sy + 20, 10, 10, C_WOOD_D);
        fe::rect(fb, sx + 12, sy + 21, 8, 8, C_WOOD);
        fe::circleOutline(fb, sx + 16, sy + 24, 3, C_WOOD_D);
        return;
    }
    float sway = (float)std::sin(plat->timeMs() / 900.0 + tx * 2 + ty) * 1.5f;
    // tronc
    fe::rect(fb, sx + 13, sy + 16, 6, 16, C_WOOD_D);
    fe::rect(fb, sx + 14, sy + 16, 2, 16, C_WOOD_L);
    // couronne (3 cercles)
    int cy = sy + 10;
    int cx = sx + 16 + (int)sway;
    fe::circle(fb, cx - 8, cy + 2, 9, C_LEAF_D);
    fe::circle(fb, cx + 8, cy + 2, 9, C_LEAF_D);
    fe::circle(fb, cx, cy - 5, 10, C_LEAF);
    fe::circle(fb, cx - 4, cy - 8, 6, C_LEAF_L);
    fe::circle(fb, cx + 6, cy - 2, 5, C_LEAF_L);
}

// ------------------------------------------------------------
// Buisson (avec ou sans baies)
// ------------------------------------------------------------
void Game::renderBush(int sx, int sy, const Node& n) {
    fe::circle(fb, sx + 9, sy + 22, 7, C_LEAF_D);
    fe::circle(fb, sx + 23, sy + 22, 7, C_LEAF_D);
    fe::circle(fb, sx + 16, sy + 17, 8, C_LEAF);
    fe::circle(fb, sx + 12, sy + 14, 4, C_LEAF_L);
    if (n.a == 1) {
        fe::circle(fb, sx + 11, sy + 20, 2, C_RED);
        fe::circle(fb, sx + 21, sy + 22, 2, C_RED);
        fe::circle(fb, sx + 16, sy + 26, 2, rgb(170, 40, 40));
    }
}

// ------------------------------------------------------------
// Animal
// ------------------------------------------------------------
void Game::renderAnimal(const Animal& a, int ax, int ay) {
    const AnimalDef* d = animalDef(a.type);
    if (!d) return;
    float scale = a.isAdult() ? d->scale : d->scale * 0.6f;
    bool sleep = a.sleeping;
    // ombre
    fe::circle(fb, ax, ay + 2, (int)(8 * scale), rgb(0, 0, 0) & 0x00000000); // placeholder
    fe::blendRect(fb, ax - 8, ay, 16, 4, C_BLACK, 60);

    float bob = sleep ? 0 : (float)std::sin(a.bob) * 1.5f;
    int bx = ax, by = ay - 8 + (int)bob;
    int s = (int)(scale * 16);

    if (a.type == "chicken") {
        if (sleep) {
            fe::circle(fb, bx, by + 2, s / 3, C_WHITE);
            fe::circle(fb, bx + 6, by, s / 5, C_WHITE);
            fe::rect(fb, bx - 8, by + 2, 16, 3, C_WHITE);
        } else {
            fe::circle(fb, bx, by + 2, s / 3, C_WHITE);          // corps
            fe::circle(fb, bx + 6, by - 3, s / 5, C_WHITE);     // tête
            fe::rect(fb, bx + 4, by - 6, 3, 2, C_RED);          // crête
            fe::px(fb, bx + 9, by - 3, C_ORANGE);               // bec
            fe::px(fb, bx + 6, by - 4, C_BLACK);                // œil
            fe::rect(fb, bx - 3, by + 5, 2, 3, C_ORANGE);       // pattes
            fe::rect(fb, bx + 2, by + 5, 2, 3, C_ORANGE);
            fe::circle(fb, bx - 3, by + 1, s / 6, rgb(220, 220, 215)); // aile
        }
    } else if (a.type == "cow") {
        if (sleep) {
            fe::rect(fb, bx - 10, by, 22, 7, C_WHITE);
            fe::rect(fb, bx + 8, by - 3, 6, 5, C_WHITE);
            fe::rect(fb, bx + 9, by - 1, 4, 2, C_PINK);
        } else {
            fe::rect(fb, bx - 9, by - 4, 18, 10, C_WHITE);      // corps
            fe::rect(fb, bx - 7, by - 2, 4, 3, C_BLACK);        // tache
            fe::rect(fb, bx + 3, by + 2, 4, 3, C_BLACK);        // tache
            fe::rect(fb, bx + 5, by - 3, 3, 2, C_BLACK);        // tache
            fe::rect(fb, bx + 6, by - 9, 7, 7, C_WHITE);        // tête
            fe::rect(fb, bx + 5, by - 11, 2, 3, C_GRAY);        // oreille
            fe::rect(fb, bx + 12, by - 11, 2, 3, C_GRAY);       // oreille
            fe::rect(fb, bx - 2, by - 13, 2, 3, C_CREAM);       // cornes
            fe::rect(fb, bx + 9, by - 13, 2, 3, C_CREAM);
            fe::rect(fb, bx + 7, by - 5, 5, 3, C_PINK);         // museau
            fe::px(fb, bx + 8, by - 6, C_BLACK);
            fe::px(fb, bx + 11, by - 6, C_BLACK);
            fe::rect(fb, bx - 7, by + 6, 2, 4, C_GRAY);         // pattes
            fe::rect(fb, bx + 5, by + 6, 2, 4, C_GRAY);
            fe::vline(fb, bx - 10, by - 2, by + 4, C_GRAY);      // queue
        }
    } else { // mouton
        if (sleep) {
            fe::circle(fb, bx - 3, by + 2, 6, C_CREAM);
            fe::circle(fb, bx + 4, by + 2, 6, C_CREAM);
            fe::circle(fb, bx + 9, by, 4, C_GRAY);
        } else {
            fe::circle(fb, bx - 5, by, 6, C_CREAM);
            fe::circle(fb, bx + 5, by, 6, C_CREAM);
            fe::circle(fb, bx, by - 3, 7, C_WHITE);
            fe::circle(fb, bx + 8, by - 2, 4, C_GRAY);          // tête
            fe::px(fb, bx + 9, by - 3, C_BLACK);
            fe::rect(fb, bx - 4, by + 5, 2, 4, C_GRAY);         // pattes
            fe::rect(fb, bx + 3, by + 5, 2, 4, C_GRAY);
        }
    }

    // indicateur "production prête"
    if (a.ready > 0 && !sleep) {
        int iy = by - 14;
        fe::rect(fb, bx - 5, iy - 3, 10, 9, UI_PANEL);
        fe::rectOutline(fb, bx - 5, iy - 3, 10, 9, UI_ACCENT);
        int icon = a.type == "chicken" ? 30 : (a.type == "cow" ? 31 : 32);
        fe::drawIcon(fb, bx - 4, iy - 2, icon, 0);
    }

    // petit "z" si endormi
    if (sleep) {
        int zy = by - 12 - (int)(std::fmod(plat->timeMs() / 800.0, 1.0) * 6);
        fe::drawText(fb, bx + 10, zy, "z", fe::FONT_SMALL, C_BLUE_L);
    }
}

// ------------------------------------------------------------
// Joueur
// ------------------------------------------------------------
void Game::renderPlayer(int sx, int sy) {
    // ombre
    fe::blendRect(fb, sx - 7, sy - 2, 14, 4, C_BLACK, 70);
    float walk = (float)std::sin(walkPhase) * 2.0f;
    bool moving = walkPhase > 0.01f;
    int legL = moving ? (int)walk : 0;
    int legR = moving ? -(int)walk : 0;

    // jambes
    fe::rect(fb, sx - 4, sy - 8, 3, 8 + legL, C_PANTS);
    fe::rect(fb, sx + 1, sy - 8, 3, 8 + legR, C_PANTS);
    fe::rect(fb, sx - 5, sy - 1 + legL, 4, 2, C_BLACK); // chaussures
    fe::rect(fb, sx + 1, sy - 1 + legR, 4, 2, C_BLACK);
    // corps (chemise)
    fe::rect(fb, sx - 5, sy - 18, 10, 11, C_SHIRT);
    fe::rect(fb, sx - 5, sy - 18, 10, 2, rgb(90, 160, 170));
    // bras (animés pendant une action)
    int armSwing = (int)(actionAnim > 0 ? std::sin((0.25f - actionAnim) * 40) * 4 : 0);
    if (sim.st.facing == FACING_LEFT || sim.st.facing == FACING_RIGHT) {
        fe::rect(fb, sx - 7, sy - 16, 2, 7 + armSwing, C_SHIRT);
        fe::rect(fb, sx + 5, sy - 16, 2, 7 - armSwing, C_SHIRT);
        fe::rect(fb, sx - 7, sy - 9 + armSwing, 2, 2, C_SKIN); // mains
        fe::rect(fb, sx + 5, sy - 9 - armSwing, 2, 2, C_SKIN);
    } else {
        fe::rect(fb, sx - 6, sy - 16, 2, 7, C_SHIRT);
        fe::rect(fb, sx + 4, sy - 16, 2, 7, C_SHIRT);
        fe::rect(fb, sx - 6, sy - 9, 2, 2, C_SKIN);
        fe::rect(fb, sx + 4, sy - 9, 2, 2, C_SKIN);
    }
    // tête
    fe::rect(fb, sx - 4, sy - 26, 8, 8, C_SKIN);
    // cheveux / visage selon la direction
    if (sim.st.facing == FACING_UP) {
        fe::rect(fb, sx - 4, sy - 27, 8, 3, C_HAIR); // cheveux vu de dos
    } else if (sim.st.facing == FACING_DOWN) {
        fe::rect(fb, sx - 4, sy - 27, 8, 2, C_HAIR);
        fe::px(fb, sx - 2, sy - 23, C_BLACK); // yeux
        fe::px(fb, sx + 1, sy - 23, C_BLACK);
    } else {
        // profil
        int dir = sim.st.facing == FACING_RIGHT ? 1 : -1;
        fe::rect(fb, sx - 4, sy - 27, 8, 2, C_HAIR);
        fe::px(fb, sx - 4 + 7 * (dir > 0 ? 1 : 0) - (dir > 0 ? 0 : 3), sy - 23, C_BLACK);
    }
    // outil en main (si outil sélectionné)
    ItemStack& sel = sim.selected();
    if (!sel.empty()) {
        const ItemDef* def = itemDef(sel.id);
        if (def && def->type == IT_TOOL) {
            int tx = sx + (sim.st.facing == FACING_RIGHT ? 8 : -8);
            int ty = sy - 12 + armSwing;
            fe::drawIcon(fb, tx - 8, ty - 8, def->icon, 0);
        }
    }
}

// ------------------------------------------------------------
// Bâtiments et structures
// ------------------------------------------------------------
void Game::renderHouse(int sx, int sy) {
    // murs
    fe::rect(fb, sx, sy + 32, 6 * TILE, 4 * TILE, C_WALL);
    fe::rect(fb, sx, sy + 32, 6 * TILE, 4, rgb(200, 185, 155));
    // colombages
    for (int i = 0; i <= 6; i++)
        fe::vline(fb, sx + i * TILE - (i == 6 ? 1 : 0), sy + 32, sy + 6 * TILE, C_WOOD_L);
    fe::hline(fb, sx, sx + 6 * TILE - 1, sy + 3 * TILE + 16, C_WOOD_L);
    // toit
    for (int row = 0; row < 3; row++) {
        int w = 6 * TILE - row * 2 * 8;
        fe::rect(fb, sx + row * 8 + 4, sy + row * 12, w, 12, row % 2 ? C_ROOF : C_ROOF_D);
    }
    fe::rect(fb, sx + 4, sy, 6 * TILE - 8, 6, C_ROOF_D);
    // cheminée + fumée
    fe::rect(fb, sx + 5 * TILE - 8, sy - 8, 12, 16, C_ROOF_D);
    float f1 = (float)std::fmod(plat->timeMs() / 1400.0, 1.0);
    fe::blendRect(fb, sx + 5 * TILE - 6 + (int)(f1 * 10), sy - 14 - (int)(f1 * 16), 6, 6, C_WHITE, (int)(80 * (1 - f1)));
    // porte
    fe::rect(fb, sx + 3 * TILE - 8, sy + 4 * TILE + 8, 20, 24, C_WOOD_D);
    fe::rect(fb, sx + 3 * TILE - 6, sy + 4 * TILE + 10, 16, 20, C_WOOD);
    fe::px(fb, sx + 3 * TILE + 6, sy + 4 * TILE + 20, C_GOLD); // poignée
    // fenêtre (lumineuse la nuit)
    bool night = sim.st.isNight();
    Color win = night ? rgb(255, 210, 110) : rgb(120, 170, 200);
    fe::rect(fb, sx + TILE + 6, sy + 3 * TILE + 20, 20, 16, C_WOOD_D);
    fe::rect(fb, sx + TILE + 8, sy + 3 * TILE + 22, 16, 12, win);
    if (night) drawGlow(sx + TILE + 16, sy + 3 * TILE + 28, 70, rgb(255, 200, 90), 90);
    fe::vline(fb, sx + TILE + 15, sy + 3 * TILE + 22, sy + 3 * TILE + 33, C_WOOD_D);
}

void Game::renderStall(int sx, int ty0) {
    // poteaux + comptoir + auvent rayé
    fe::rect(fb, sx, ty0 + 24, 4 * TILE, 8, C_WOOD_D); // comptoir
    fe::rect(fb, sx, ty0 + 24, 4 * TILE, 3, C_WOOD_L);
    fe::vline(fb, sx + 4, ty0 + 8, ty0 + 24, C_WOOD_D);
    fe::vline(fb, sx + 4 * TILE - 8, ty0 + 8, ty0 + 24, C_WOOD_D);
    // auvent
    for (int i = 0; i < 4 * TILE; i += 8)
        fe::rect(fb, sx + i, ty0, 8, 10, (i / 8) % 2 ? C_RED : C_WHITE);
    fe::rect(fb, sx - 2, ty0 + 10, 4 * TILE + 4, 3, C_WOOD_D);
    // enseigne
    fe::drawTextShadow(fb, sx + 10, ty0 - 12, "MARCHÉ", fe::FONT_SMALL, C_GOLD);
}

void Game::renderBoard(int sx, int sy) {
    fe::rect(fb, sx + 13, sy + 8, 6, 24, C_WOOD_D); // poteau
    fe::rect(fb, sx, sy, 32, 22, C_WOOD);
    fe::rect(fb, sx + 2, sy + 2, 28, 18, C_CREAM);
    fe::hline(fb, sx + 5, sx + 26, sy + 7, C_GRAY);
    fe::hline(fb, sx + 5, sx + 26, sy + 11, C_GRAY);
    fe::hline(fb, sx + 5, sx + 20, sy + 15, C_GRAY);
    fe::drawTextShadow(fb, sx - 2, sy - 12, "QUÊTES", fe::FONT_SMALL, UI_ACCENT);
}


void Game::renderBin(int sx, int sy) {
    fe::rect(fb, sx + 4, sy + 12, 24, 18, C_WOOD);
    fe::rect(fb, sx + 4, sy + 12, 24, 4, C_WOOD_L);
    fe::rect(fb, sx + 2, sy + 9, 28, 5, C_WOOD_D); // couvercle
    fe::hline(fb, sx + 6, sx + 26, sy + 20, C_WOOD_D);
    fe::drawTextShadow(fb, sx + 1, sy - 2, "CAISSE", fe::FONT_SMALL, C_GOLD);
}

void Game::renderFence(int sx, int sy) {
    fe::rect(fb, sx + 14, sy + 8, 4, 24, C_WOOD_D); // poteau central
    fe::rect(fb, sx, sy + 12, TILE, 3, C_WOOD_L);  // rail haut
    fe::rect(fb, sx, sy + 20, TILE, 3, C_WOOD_L);  // rail bas
    fe::rect(fb, sx + 1, sy + 8, 3, 24, C_WOOD_D); // poteau gauche
}

void Game::renderBed(int sx, int sy) {
    fe::rect(fb, sx + 2, sy + 10, 2 * TILE - 4, 2 * TILE - 8, C_WOOD_D); // cadre
    fe::rect(fb, sx + 4, sy + 12, 2 * TILE - 8, 2 * TILE - 12, C_WHITE); // drap
    fe::rect(fb, sx + 4, sy + 12, 2 * TILE - 8, TILE - 6, rgb(90, 130, 200)); // couverture
    fe::rect(fb, sx + 6, sy + 13, 18, 10, C_WHITE); // oreiller
    fe::rectOutline(fb, sx + 2, sy + 10, 2 * TILE - 4, 2 * TILE - 8, C_WOOD);
}

void Game::renderChest(int sx, int sy) {
    fe::rect(fb, sx + 2, sy + 12, TILE - 4, 18, C_WOOD);
    fe::rect(fb, sx + 2, sy + 12, TILE - 4, 4, C_WOOD_L);
    fe::rect(fb, sx, sy + 8, TILE, 6, C_WOOD_D);
    fe::rect(fb, sx + 13, sy + 14, 6, 6, C_GOLD); // serrure
}

void Game::renderStructures() {
    // Les structures statiques (murs) sont invisibles : le décor est déjà là.
    // On dessine juste les marqueurs d'interaction au sol.
    MapData& m = sim.world.current(sim.st);
    int tx0 = std::max(0, camX / TILE - 1);
    int ty0 = std::max(0, camY / TILE - 1);
    int tx1 = std::min(m.w - 1, (camX + FB_W) / TILE + 1);
    int ty1 = std::min(m.h - 1, (camY + FB_H) / TILE + 1);
    for (int ty = ty0; ty <= ty1; ty++) {
        for (int tx = tx0; tx <= tx1; tx++) {
            uint8_t o = m.o(tx, ty);
            if (o == T_NONE) continue;
            int sx = tx * TILE - camX, sy = ty * TILE - camY;
            if (o == T_WALL) {
                fe::rect(fb, sx, sy, TILE, TILE, C_WALL);
                fe::hline(fb, sx, sx + TILE - 1, sy + TILE / 2, C_WOOD_L);
            } else if (o == T_DOOR) {
                // marqueur de porte au sol
                fe::rect(fb, sx + 8, sy + 24, 16, 6, C_WOOD);
            } else if (o == T_COUNTER) {
                fe::rect(fb, sx, sy + 8, TILE, 20, C_WOOD);
                fe::rect(fb, sx, sy + 8, TILE, 4, C_WOOD_L);
                fe::drawTextShadow(fb, sx + 12, sy + 12, "$", fe::FONT_SMALL, C_GOLD);
            }
        }
    }
}

// ------------------------------------------------------------
// Météo
// ------------------------------------------------------------
void Game::renderWeather() {
    // Pluie
    for (auto& p : rain) {
        int sx = (int)p.x - camX, sy = (int)p.y - camY;
        if (sx < -20 || sx > FB_W + 20 || sy < -20 || sy > FB_H + 20) continue;
        for (int i = 0; i < p.len; i++)
            fe::px(fb, sx + (int)(p.vx * 0.004f * i), sy + i, rgb(180, 210, 240));
    }
    // Neige
    for (auto& p : snow) {
        int sx = (int)p.x - camX, sy = (int)p.y - camY;
        if (sx < -10 || sx > FB_W + 10 || sy < -10 || sy > FB_H + 10) continue;
        fe::rect(fb, sx, sy, 2, 2, C_WHITE);
    }
    // Voile nuageux
    if (sim.st.weather == W_CLOUD) fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(180, 190, 200), 28);
    if (sim.st.weather == W_RAIN) fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(60, 80, 110), 30);
    if (sim.st.weather == W_STORM) fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(30, 35, 60), 45);
    if (sim.st.weather == W_SNOW) fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(220, 230, 245), 25);
    // Éclair
    if (lightning > 0) {
        fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(255, 255, 240), (int)(160 * lightning));
    }
}

// ------------------------------------------------------------
// Éclairage jour/nuit + lueurs
// ------------------------------------------------------------
void Game::renderLighting() {
    int m = sim.st.minutes;
    // facteur d'obscurité 0 (jour) .. 1 (nuit profonde)
    float dark = 0;
    if (m >= 21 * 60 || m < 5 * 60) dark = 0.62f;
    else if (m >= 18 * 60 && m < 21 * 60) dark = (m - 18 * 60) / (3.0f * 60) * 0.62f;
    else if (m >= 5 * 60 && m < 6 * 60) dark = 0.62f - (m - 5 * 60) / 3600.0f * 0.62f;

    if (sim.st.weather == W_STORM) dark = std::min(0.8f, dark + 0.2f);
    if (sim.st.weather == W_SNOW) dark *= 0.7f;

    if (dark > 0.01f) {
        fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(16, 20, 48), (int)(dark * 255));
    }
    // teinte chaude au crépuscule / aube
    float warm = 0;
    if (m >= 18 * 60 && m < 20 * 60) warm = (m - 18 * 60) / 7200.0f;
    if (m >= 5 * 60 && m < 7 * 60) warm = (7 * 60 - m) / 7200.0f;
    if (warm > 0.01f)
        fe::blendRect(fb, 0, 0, FB_W, FB_H, rgb(255, 150, 80), (int)(warm * 45));

    // lueur du joueur la nuit (lanterne)
    if (dark > 0.2f) {
        int px = (int)sim.st.px - camX, py = (int)sim.st.py - camY;
        drawGlow(px, py - 10, (int)(60 * (0.4f + dark * 0.6f)), rgb(255, 220, 140), (int)(70 * dark));
    }
}

void Game::drawGlow(int cx, int cy, int radius, Color c, int alpha) {
    if (radius <= 0 || alpha <= 0) return;
    int r2 = radius * radius;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            int d2 = x * x + y * y;
            if (d2 > r2) continue;
            float f = 1.0f - (float)d2 / r2;
            f = f * f * f; // falloff doux
            fe::blendPx(fb, cx + x, cy + y, c, (int)(alpha * f));
        }
    }
}

// ------------------------------------------------------------
// Cœurs au-dessus des animaux
// ------------------------------------------------------------
void Game::renderHearts() {
    for (auto& h : hearts) {
        int sx = (int)h.x, sy = (int)h.y - (int)((1.6f - h.ttl) * 14);
        int a = (int)(std::min(1.0f, h.ttl) * 255);
        if (a <= 0) continue;
        // cœur simple
        fe::circle(fb, sx - 2, sy, 2, C_RED);
        fe::circle(fb, sx + 2, sy, 2, C_RED);
        fe::rect(fb, sx - 4, sy + 1, 8, 3, C_RED);
        fe::px(fb, sx, sy + 4, C_RED);
        fe::px(fb, sx - 1, sy + 3, C_RED);
        fe::px(fb, sx + 1, sy + 3, C_RED);
        (void)a;
    }
}

} // namespace fv
