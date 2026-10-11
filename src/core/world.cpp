#include "world.h"
#include "rng.h"
#include "items.h"

namespace fv {

void World::build(GameState& st, bool scatter) {
    genFarm(st, scatter);
    genHome();
}

MapData& World::current(GameState& st) { return st.mapId == "home" ? home : farm; }
const MapData& World::current(const GameState& st) const { return st.mapId == "home" ? home : farm; }

bool World::tillable(const GameState& st, int x, int y) const {
    if (!farm.inBounds(x, y)) return false;
    if (farm.g(x, y) != T_GRASS) return false;
    if (inForest(x, y) || inPasture(x, y) || inPondZone(x, y)) return false;
    if (farm.o(x, y) != T_NONE) return false;
    if (st.nodes.count(tileKey(x, y))) return false; // arbre/buisson dessus
    return true;
}

bool World::solidAt(const GameState& st, const MapData& m, float px, float py) const {
    // Boîte du joueur : ~14x12 px centrée sur (px, py) au niveau des pieds
    const float hx = 7.0f, hy = 6.0f;
    const float pts[5][2] = {
        {px - hx, py - hy}, {px + hx, py - hy}, {px, py - hy},
        {px - hx, py + 2}, {px + hx, py + 2},
    };
    for (auto& p : pts) {
        int tx = (int)(p[0] / TILE), ty = (int)(p[1] / TILE);
        if (!m.inBounds(tx, ty)) return true;
        if (m.solid(tx, ty)) return true;
        // Nature dynamique (arbre / buisson / souche) = solide
        auto it = st.nodes.find(tileKey(tx, ty));
        if (it != st.nodes.end() && m.id == "farm") return true;
    }
    return false;
}

// ------------------------------------------------------------
// Génération de la ferme
// ------------------------------------------------------------
void World::genFarm(GameState& st, bool scatter) {
    farm.init(MAP_W, MAP_H, "farm");

    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            farm.setG(x, y, T_GRASS);

    // --- Étang (bas droite) : anneau de sable puis eau ---
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            float dx = (x - 41.0f) / 7.2f;
            float dy = (y - 24.0f) / 5.2f;
            float d = dx * dx + dy * dy;
            if (d <= 1.0f) farm.setG(x, y, T_POND);
            else if (d <= 1.45f) farm.setG(x, y, T_SAND);
        }
    }

    // --- Chemins de terre ---
    auto pathV = [&](int x, int y0, int y1) { for (int y = y0; y <= y1; y++) if (farm.g(x, y) == T_GRASS) farm.setG(x, y, T_PATH); };
    auto pathH = [&](int y, int x0, int x1) { for (int x = x0; x <= x1; x++) if (farm.g(x, y) == T_GRASS) farm.setG(x, y, T_PATH); };
    pathV(8, 7, 20);        // de la maison au portail du pâturage
    pathH(8, 8, 37);        // vers le marché
    pathV(37, 6, 8);        // vers le comptoir
    pathH(19, 20, 30);      // sentier vers la forêt

    // --- Maison (5..10, 2..6) ---
    for (int y = 2; y <= 6; y++) {
        for (int x = 5; x <= 10; x++) {
            bool edge = (x == 5 || x == 10 || y == 2 || y == 6);
            farm.setG(x, y, T_FLOOR);
            farm.setO(x, y, edge ? T_WALL : T_NONE);
        }
    }
    farm.setO(8, 6, T_DOOR); // porte d'entrée

    // --- Panneau des quêtes / caisse d'expédition ---
    farm.setO(13, 4, T_BOARD);
    farm.setO(10, 7, T_BIN);

    // --- Stand du marché (36..39, 3..4) + comptoir ---
    for (int y = 3; y <= 4; y++)
        for (int x = 36; x <= 39; x++)
            farm.setO(x, y, T_STALL);
    farm.setO(37, 5, T_COUNTER);
    farm.setO(38, 5, T_COUNTER);

    // --- Clôture du pâturage (8..26, 20..28), portail en (8,20) ---
    for (int x = 8; x <= 26; x++) {
        if (x != 8) farm.setO(x, 20, T_FENCE);
        farm.setO(x, 28, T_FENCE);
    }
    for (int y = 21; y <= 27; y++) {
        farm.setO(8, y, T_FENCE);
        farm.setO(26, y, T_FENCE);
    }
    farm.setG(8, 20, T_PATH); // portail (passable)

    if (scatter) scatterNature(st, st.seed);
}

void World::scatterNature(GameState& st, uint64_t seed) {
    Rng rng(seed ^ 0x5EEDull);
    auto treeAt = [&](int x, int y) {
        st.nodes[tileKey(x, y)] = Node{"tree", 3, 3};   // a = pv, b = pv max
    };
    auto bushAt = [&](int x, int y) {
        st.nodes[tileKey(x, y)] = Node{"bush", 1, 0};   // a = a des baies
    };
    // Forêt (gauche, bas) : arbres denses + buissons
    for (int y = 12; y < MAP_H; y++) {
        for (int x = 0; x <= 6; x++) {
            if (farm.o(x, y) != T_NONE) continue;
            if (farm.g(x, y) != T_GRASS) continue;
            float r = rng.uf();
            if (r < 0.42f) treeAt(x, y);
            else if (r < 0.58f) bushAt(x, y);
        }
    }
    // Quelques arbres épars à droite
    for (int y = 2; y < 18; y++) {
        for (int x = 28; x < MAP_W; x++) {
            if (farm.o(x, y) != T_NONE || farm.g(x, y) != T_GRASS) continue;
            float r = rng.uf();
            if (r < 0.06f) treeAt(x, y);
            else if (r < 0.10f) bushAt(x, y);
        }
    }
    // Buissons près de l'étang
    for (int y = 18; y < MAP_H; y++) {
        for (int x = 28; x < 36; x++) {
            if (farm.o(x, y) != T_NONE || farm.g(x, y) != T_GRASS) continue;
            if (rng.uf() < 0.10f) bushAt(x, y);
        }
    }
}

// ------------------------------------------------------------
// Intérieur de la maison
// ------------------------------------------------------------
void World::genHome() {
    home.init(HOME_W, HOME_H, "home");
    for (int y = 0; y < HOME_H; y++) {
        for (int x = 0; x < HOME_W; x++) {
            bool edge = (x == 0 || y == 0 || x == HOME_W - 1 || y == HOME_H - 1);
            home.setG(x, y, T_FLOOR);
            home.setO(x, y, edge ? T_WALL : T_NONE);
        }
    }
    // Lit (2x2) en haut à gauche
    home.setO(2, 2, T_BED);
    home.setO(3, 2, T_BED);
    home.setO(2, 3, T_BED);
    home.setO(3, 3, T_BED);
    // Coffre de rangement
    home.setO(6, 2, T_CHEST);
    // Porte de sortie (bas)
    home.setO(HOME_DOOR_TX, HOME_DOOR_TY, T_DOOR);
}

// ------------------------------------------------------------
// Nouvelle partie
// ------------------------------------------------------------
void newGameState(GameState& st, uint64_t seed) {
    st = GameState();
    st.seed = seed ? seed : 1;
    st.day = 1;
    st.minutes = DAY_START_MIN;
    st.money = 500;
    st.energy = ENERGY_MAX_BASE;
    st.energyMax = (int)ENERGY_MAX_BASE;
    st.mapId = "farm";
    st.px = (float)(World::FARM_SPAWN_TX * TILE + TILE / 2);
    st.py = (float)(World::FARM_SPAWN_TY * TILE + TILE / 2);
    st.facing = FACING_DOWN;

    st.hotbar.resize(HOTBAR_SLOTS);
    st.backpack.resize(BACKPACK_SLOTS);
    st.storage.resize(STORAGE_SLOTS);
    st.hotbar[0] = {"tool_hoe", 0, 1};
    st.hotbar[1] = {"tool_can", 0, 1};
    st.hotbar[2] = {"seed_wheat", 0, 10};
    st.hotbar[3] = {"seed_potato", 0, 5};
    st.hotbar[4] = {"hay", 0, 5};
}

} // namespace fv
