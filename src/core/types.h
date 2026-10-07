#pragma once
// ============================================================
// FarmVale — types de base du jeu
// ============================================================
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include "constants.h"

namespace fv {

using std::string;
using std::vector;

// --------- Une pile d'objets (inventaire) ---------
struct ItemStack {
    string id;
    int q = 0;   // qualité (0..2)
    int n = 0;   // quantité
    bool empty() const { return n <= 0 || id.empty(); }
    void clear() { id.clear(); q = 0; n = 0; }
    bool sameKind(const ItemStack& o) const { return !empty() && id == o.id && q == o.q; }
};

// --------- Carte (ferme ou maison) ---------
struct MapData {
    int w = 0, h = 0;
    string id;
    vector<uint8_t> ground;  // Tile de sol
    vector<uint8_t> obj;     // Tile d'objet (T_NONE si vide)

    void init(int w_, int h_, const string& id_) {
        w = w_; h = h_; id = id_;
        ground.assign((size_t)w * h, T_GRASS);
        obj.assign((size_t)w * h, T_NONE);
    }
    uint8_t g(int x, int y) const { return ground[(size_t)y * w + x]; }
    uint8_t o(int x, int y) const { return obj[(size_t)y * w + x]; }
    void setG(int x, int y, uint8_t t) { ground[(size_t)y * w + x] = t; }
    void setO(int x, int y, uint8_t t) { obj[(size_t)y * w + x] = t; }
    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
    // Un objet est-il solide (collision) ?
    static bool objSolid(uint8_t t) {
        switch (t) {
            case T_WALL: case T_TREE: case T_BUSH: case T_FENCE:
            case T_STALL: case T_COUNTER: case T_BOARD: case T_BIN:
            case T_CHEST: case T_STUMP:
                return true;
            default:
                return false;
        }
    }
    bool solid(int x, int y) const {
        if (!inBounds(x, y)) return true;
        uint8_t gg = g(x, y);
        if (gg == T_POND) return true;
        uint8_t oo = o(x, y);
        return objSolid(oo);
    }
};

// --------- Parcelle de culture ---------
struct Plot {
    string crop;        // id de la culture (vide = jachère)
    int plantedDay = 0;
    float progress = 0; // "jours" de croissance accumulés
    bool watered = false;
    bool fert = false;
    bool dead = false;  // desséchée
};

// --------- Animal ---------
struct Animal {
    int id = 0;
    string type;        // chicken / cow / sheep
    float x = 0, y = 0; // position en pixels (tuile*32 + 16)
    int age = 0;        // en jours (0 = bébé nouveau-né)
    int friendship = 0; // 0..100
    int happiness = 100;
    bool fedToday = false;
    bool pettedToday = false;
    bool pregnant = false;
    int gestation = 0;
    int lastBredDay = -100;
    int ready = 0;      // produits prêts à récolter (œufs/lait/laine)
    int produceAcc = 0; // accumulateur de jours pour la production
    bool wasFed = false;    // nourri la veille (pour le cycle journalier)
    bool wasPetted = false;
    float dir = 0;      // direction de déplacement (radians)
    float moveTimer = 0;
    bool sleeping = false;
    float bob = 0;      // phase d'animation
    bool isAdult() const;
};

// --------- Nœud dynamique sur la carte (arbre, buisson, souche) ---------
struct Node {
    string kind;  // "tree" | "bush" | "stump"
    int a = 0;    // tree: pv restants ; bush: a des baies ; stump: jour de repousse
    int b = 0;    // bush/stump: jour de repousse ; tree: total de pv
};

// --------- Quête ---------
struct Quest {
    string id;
    string text;
    string kind;   // harvest / water / feed / earn / fish / wood / plant
    string arg;    // ex: id de culture
    int target = 0;
    int progress = 0;
    int rewardMoney = 0;
    int rewardSkill = -1;
    int rewardXp = 0;
    bool done = false;
};

// --------- État complet de la partie ---------
struct GameState {
    int version = 1;
    uint64_t seed = 1;
    int day = 1;             // jour absolu (1 = premier jour)
    int minutes = DAY_START_MIN;
    int weather = W_SUN;
    int money = 500;
    float energy = ENERGY_MAX_BASE;
    int energyMax = (int)ENERGY_MAX_BASE;

    string mapId = "farm";
    float px = 0, py = 0;     // position du joueur en pixels
    int facing = FACING_DOWN;

    vector<ItemStack> hotbar;    // HOTBAR_SLOTS
    vector<ItemStack> backpack;  // BACKPACK_SLOTS
    vector<ItemStack> storage;   // STORAGE_SLOTS (coffre)

    std::unordered_map<uint64_t, Plot> plots;
    vector<Animal> animals;
    std::unordered_map<uint64_t, Node> nodes;
    vector<Quest> quests;

    int skills[SK_COUNT] = {0, 0, 0, 0};  // XP
    int earningsToday = 0;
    int canLevel = 1;        // niveau de l'arrosoir (1..3)
    int energyUp = 0;        // améliorations d'énergie achetées

    // stats (succès + fin de partie)
    uint64_t statMoneyEarned = 0;
    uint64_t statCropsHarvested = 0;
    uint64_t statAnimalsRaised = 0;
    uint64_t statFishCaught = 0;
    uint64_t statDaysPlayed = 0;
    bool reachedWinter = false;

    int nextAnimalId = 1;
    int animalCounter = 0;   // animaux achetés/total pour stats

    // helpers temps
    int season() const { return (day - 1) / DAYS_PER_SEASON % SEASON_COUNT; }
    int dayOfSeason() const { return (day - 1) % DAYS_PER_SEASON + 1; }
    int weekDay() const { return (day - 1) % 7; }
    int hour() const { return minutes / 60; }
    int minute() const { return minutes % 60; }
    bool isNight() const { return minutes >= 20 * 60 || minutes < 6 * 60; }
    int skillLevel(int s) const {
        int lv = skills[s] / SKILL_XP_PER_LEVEL + 1;
        return lv > SKILL_MAX_LEVEL ? SKILL_MAX_LEVEL : lv;
    }
    int energyMaxBase() const { return (int)ENERGY_MAX_BASE + energyUp * 25; }
};

// Clé de hash pour les maps de tuiles
inline uint64_t tileKey(int x, int y) {
    return ((uint64_t)(uint32_t)y << 32) | (uint32_t)x;
}

} // namespace fv
