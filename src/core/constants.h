#pragma once
// ============================================================
// FarmVale — constantes de game design
// ============================================================
#include <cstdint>

namespace fv {

// --- Rendu / monde ---
constexpr int TILE = 32;          // taille d'une tuile en pixels
constexpr int VIEW_W = 30;        // tuiles visibles en largeur
constexpr int VIEW_H = 19;        // tuiles visibles en hauteur
constexpr int FB_W = TILE * VIEW_W;   // 960
constexpr int FB_H = TILE * VIEW_H;   // 608

constexpr int MAP_W = 48;
constexpr int MAP_H = 30;
constexpr int HOME_W = 20;
constexpr int HOME_H = 14;

// --- Temps ---
constexpr int DAY_START_MIN = 6 * 60;    // 6h00
constexpr int DAY_END_MIN   = 26 * 60;   // 2h00 du lendemain
constexpr int MINUTES_PER_DAY = 24 * 60;
constexpr float REAL_SECONDS_PER_GAME_HOUR = 12.0f; // 1 jour de jeu ≈ 4 minutes réelles
constexpr int DAYS_PER_SEASON = 28;
constexpr int SEASON_COUNT = 4;

// --- Saisons ---
enum Season { SPRING = 0, SUMMER = 1, AUTUMN = 2, WINTER = 3 };
inline const char* seasonName(int s) {
    static const char* n[4] = { "Printemps", "Été", "Automne", "Hiver" };
    return n[s & 3];
}
inline const char* seasonKey(int s) {
    static const char* n[4] = { "spring", "summer", "autumn", "winter" };
    return n[s & 3];
}

// --- Météo ---
enum Weather { W_SUN = 0, W_CLOUD = 1, W_RAIN = 2, W_STORM = 3, W_SNOW = 4 };
inline const char* weatherName(int w) {
    static const char* n[5] = { "Ensoleillé", "Nuageux", "Pluie", "Orage", "Neige" };
    return n[w];
}

// --- Tuiles ---
enum Tile : uint8_t {
    T_NONE = 0,
    T_GRASS, T_PATH, T_SAND, T_POND,
    T_SOIL,                 // terre labourée (inoccupe)
    T_WALL, T_FLOOR, T_DOOR, T_BED, T_CHEST,
    T_STALL, T_COUNTER, T_BOARD, T_BIN,
    T_FENCE, T_TREE, T_BUSH, T_STUMP,
};

// --- Énergie / actions ---
constexpr float ENERGY_MAX_BASE = 100.0f;
constexpr float ENERGY_TILL = 2.0f;
constexpr float ENERGY_WATER = 1.0f;
constexpr float ENERGY_PLANT = 1.0f;
constexpr float ENERGY_HARVEST = 2.0f;
constexpr float ENERGY_CHOP = 4.0f;
constexpr float ENERGY_SCYTHE = 2.0f;
constexpr float ENERGY_FORAGE = 1.0f;
constexpr float ENERGY_FEED = 1.0f;
constexpr float ENERGY_PET = 1.0f;
constexpr float ENERGY_MILK = 4.0f;
constexpr float ENERGY_SHEAR = 4.0f;
constexpr float ENERGY_FISH = 5.0f;
constexpr float ENERGY_SWING = 1.0f;

// --- Joueur ---
constexpr float PLAYER_SPEED = 92.0f;      // pixels / seconde
constexpr int HOTBAR_SLOTS = 12;
constexpr int BACKPACK_SLOTS = 24;
constexpr int STORAGE_SLOTS = 24;
constexpr int MAX_STACK = 99;

// --- Animaux ---
constexpr int MAX_ANIMALS = 16;
constexpr int MAX_READY_PRODUCTS = 3;
constexpr int BREED_MIN_FRIENDSHIP = 60;

// --- Skills ---
constexpr int SKILL_MAX_LEVEL = 10;
constexpr int SKILL_XP_PER_LEVEL = 100;
enum Skill { SK_FARMING = 0, SK_ANIMAL = 1, SK_FORAGING = 2, SK_FISHING = 3, SK_COUNT };
inline const char* skillName(int s) {
    static const char* n[4] = { "Agriculture", "Élevage", "Cueillette", "Pêche" };
    return n[s];
}

// --- Direction ---
enum Facing { FACING_DOWN = 0, FACING_UP = 1, FACING_LEFT = 2, FACING_RIGHT = 3 };

// --- Qualités ---
constexpr int QUALITY_COUNT = 3; // 0 normal, 1 argent, 2 or
inline const char* qualityName(int q) {
    static const char* n[3] = { "", "Argent ", "Or " };
    return n[q];
}
inline float qualityPriceMult(int q) {
    static const float m[3] = { 1.0f, 1.25f, 1.5f };
    return m[q];
}

// --- Cibles Steam (succès) ---
struct AchievementIds {
    static constexpr const char* FIRST_HARVEST = "ACH_FIRST_HARVEST";
    static constexpr const char* GREEN_THUMB = "ACH_GREEN_THUMB";       // 100 récoltes
    static constexpr const char* FIRST_ANIMAL = "ACH_FIRST_ANIMAL";
    static constexpr const char* FIRST_BREED = "ACH_FIRST_BREED";
    static constexpr const char* FARMER_10 = "ACH_FARMER_10";           // niveau 10 agriculture
    static constexpr const char* FISHERMAN = "ACH_FISHERMAN";           // 50 poissons
    static constexpr const char* RICH = "ACH_RICH";                     // 50 000 pièces
    static constexpr const char* SEASONS = "ACH_ALL_SEASONS";          // survivre à l'hiver
};

} // namespace fv
