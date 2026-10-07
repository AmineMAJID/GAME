#include "items.h"
#include <unordered_map>

namespace fv {

// ---------------- Objets ----------------
static const std::unordered_map<string, ItemDef>& itemTable() {
    static const std::unordered_map<string, ItemDef> t = {
        // --- Outils ---
        {"tool_hoe",    {"tool_hoe",    "Houe",          IT_TOOL,    1,  0,   1, "Laboure la terre pour y planter des graines."}},
        {"tool_can",    {"tool_can",    "Arrosoir",      IT_TOOL,    2,  0,   1, "Arrose les cultures. Améliorable au marché."}},
        {"tool_axe",    {"tool_axe",    "Hache",         IT_TOOL,    3,  250, 1, "Coupe les arbres pour obtenir du bois."}},
        {"tool_scythe", {"tool_scythe", "Faux",          IT_TOOL,    4,  300, 1, "Coupe l'herbe haute pour obtenir du foin."}},
        {"tool_rod",    {"tool_rod",    "Canne à pêche", IT_TOOL,    5,  400, 1, "Pêche des poissons à l'étang."}},
        {"tool_pail",   {"tool_pail",   "Seau",          IT_TOOL,    6,  250, 1, "Traite les vaches (et récolte les œufs)."}},
        {"tool_shears", {"tool_shears", "Tondeuse",      IT_TOOL,    7,  300, 1, "Tond la laine des moutons."}},
        // --- Graines ---
        {"seed_wheat",     {"seed_wheat",     "Graines de blé",        IT_SEED, 10, 5,  MAX_STACK, "Culture de printemps. Pousse en 4 jours.", "wheat"}},
        {"seed_potato",    {"seed_potato",    "Graines de patate",     IT_SEED, 11, 10, MAX_STACK, "Culture de printemps. Pousse en 6 jours.", "potato"}},
        {"seed_strawberry",{"seed_strawberry","Graines de fraise",     IT_SEED, 12, 25, MAX_STACK, "Printemps. Repousse tous les 4 jours.", "strawberry"}},
        {"seed_corn",      {"seed_corn",      "Graines de maïs",       IT_SEED, 13, 12, MAX_STACK, "Été/automne. Repousse tous les 4 jours.", "corn"}},
        {"seed_tomato",    {"seed_tomato",    "Graines de tomate",     IT_SEED, 14, 15, MAX_STACK, "Été. Repousse tous les 3 jours.", "tomato"}},
        {"seed_sunflower", {"seed_sunflower", "Graines de tournesol",  IT_SEED, 15, 15, MAX_STACK, "Été. Jolie fleur qui se revend bien.", "sunflower"}},
        {"seed_pumpkin",   {"seed_pumpkin",   "Graines de citrouille", IT_SEED, 16, 30, MAX_STACK, "Automne. Grosses citrouilles." , "pumpkin"}},
        {"seed_kale",      {"seed_kale",      "Graines de chou kale",  IT_SEED, 17, 18, MAX_STACK, "Automne. Pousse en 6 jours.", "kale"}},
        // --- Récoltes ---
        {"wheat",      {"wheat",      "Blé",           IT_CROP, 20, 25,  MAX_STACK, ""}},
        {"potato",     {"potato",     "Pomme de terre",IT_CROP, 21, 30,  MAX_STACK, ""}},
        {"strawberry", {"strawberry", "Fraise",        IT_CROP, 22, 100, MAX_STACK, ""}},
        {"corn",       {"corn",       "Maïs",          IT_CROP, 23, 50,  MAX_STACK, ""}},
        {"tomato",     {"tomato",     "Tomate",        IT_CROP, 24, 60,  MAX_STACK, ""}},
        {"sunflower",  {"sunflower",  "Tournesol",     IT_CROP, 25, 80,  MAX_STACK, ""}},
        {"pumpkin",    {"pumpkin",    "Citrouille",    IT_CROP, 26, 120, MAX_STACK, ""}},
        {"kale",       {"kale",       "Chou kale",     IT_CROP, 27, 70,  MAX_STACK, ""}},
        // --- Produits animaux ---
        {"egg",        {"egg",        "Œuf",           IT_PRODUCT, 30, 40,  MAX_STACK, ""}},
        {"egg_large",  {"egg_large",  "Gros œuf",      IT_PRODUCT, 30, 60,  MAX_STACK, ""}},
        {"milk",       {"milk",       "Lait",          IT_PRODUCT, 31, 100, MAX_STACK, ""}},
        {"milk_large", {"milk_large", "Lait entier",   IT_PRODUCT, 31, 150, MAX_STACK, ""}},
        {"wool",       {"wool",       "Laine",         IT_PRODUCT, 32, 250, MAX_STACK, ""}},
        // --- Ressources ---
        {"hay",        {"hay",        "Foin",          IT_FEED,   40, 20,  MAX_STACK, "Nourriture pour les animaux (sauf poules: maïs accepté)."}},
        {"wood",       {"wood",       "Bois",          IT_RESOURCE, 41, 10, MAX_STACK, "Matériau de construction."}},
        {"berry",      {"berry",      "Baies",         IT_RESOURCE, 42, 30, MAX_STACK, "Cueillies dans les buissons."}},
        {"fertilizer", {"fertilizer", "Engrais",       IT_RESOURCE, 43, 50, MAX_STACK, "Accélère la pousse de 33% et améliore la qualité."}},
        // --- Poissons ---
        {"fish_roach", {"fish_roach", "Gardon",   IT_FISH, 50, 30,  MAX_STACK, "Commun, toute l'année."}},
        {"fish_trout", {"fish_trout", "Truite",   IT_FISH, 51, 75,  MAX_STACK, "Aime les eaux fraîches du printemps."}},
        {"fish_carp",  {"fish_carp",  "Carpe",    IT_FISH, 52, 90,  MAX_STACK, "Préfère la pluie."}},
        {"fish_pike",  {"fish_pike",  "Brochet",  IT_FISH, 53, 150, MAX_STACK, "Rare et vigoureux."}},
    };
    return t;
}

const ItemDef* itemDef(const string& id) {
    auto& t = itemTable();
    auto it = t.find(id);
    return it == t.end() ? nullptr : &it->second;
}

int itemSellPrice(const string& id, int q) {
    const ItemDef* d = itemDef(id);
    if (!d) return 0;
    int base = d->sell;
    if (q == 1) base = (int)(base * 1.25f + 0.5f);
    if (q == 2) base = (int)(base * 1.5f + 0.5f);
    return base;
}

int seedPrice(const string& seedId) {
    // les graines se revendent à la moitié de leur prix d'achat (arrondi)
    static const std::unordered_map<string, int> prices = {
        {"seed_wheat", 10}, {"seed_potato", 20}, {"seed_strawberry", 50},
        {"seed_corn", 25}, {"seed_tomato", 30}, {"seed_sunflower", 30},
        {"seed_pumpkin", 60}, {"seed_kale", 35},
    };
    auto it = prices.find(seedId);
    return it == prices.end() ? 10 : it->second;
}

int findItem(vector<ItemStack>& bag, const string& id) {
    for (size_t i = 0; i < bag.size(); i++)
        if (!bag[i].empty() && bag[i].id == id) return (int)i;
    return -1;
}

bool Animal::isAdult() const {
    const AnimalDef* d = animalDef(type);
    return d && age >= d->adultAge;
}

// ---------------- Cultures ----------------
static const std::unordered_map<string, CropDef>& cropTable() {
    static const std::unordered_map<string, CropDef> t = {
        {"wheat",      {"wheat",      "Blé",           20, (uint8_t)(1 << SPRING),                 4,  0, "seed_wheat",      "wheat"}},
        {"potato",     {"potato",     "Pomme de terre",21, (uint8_t)(1 << SPRING),                 6,  0, "seed_potato",     "potato"}},
        {"strawberry", {"strawberry", "Fraise",        22, (uint8_t)(1 << SPRING),                 8,  4, "seed_strawberry", "strawberry"}},
        {"corn",       {"corn",       "Maïs",          23, (uint8_t)((1 << SUMMER) | (1 << AUTUMN)), 8, 4, "seed_corn",    "corn"}},
        {"tomato",     {"tomato",     "Tomate",        24, (uint8_t)(1 << SUMMER),                 8,  3, "seed_tomato",     "tomato"}},
        {"sunflower",  {"sunflower",  "Tournesol",     25, (uint8_t)(1 << SUMMER),                 8,  0, "seed_sunflower",  "sunflower"}},
        {"pumpkin",    {"pumpkin",    "Citrouille",    26, (uint8_t)(1 << AUTUMN),                 10, 0, "seed_pumpkin",    "pumpkin"}},
        {"kale",       {"kale",       "Chou kale",     27, (uint8_t)(1 << AUTUMN),                 6,  0, "seed_kale",       "kale"}},
    };
    return t;
}

const CropDef* cropDef(const string& id) {
    auto& t = cropTable();
    auto it = t.find(id);
    return it == t.end() ? nullptr : &it->second;
}

// ---------------- Animaux ----------------
static const std::unordered_map<string, AnimalDef>& animalTable() {
    static const std::unordered_map<string, AnimalDef> t = {
        {"chicken", {"chicken", "Poule",  60, 500,  300,  3, "hay", "corn",
                     "egg", 1, "egg_large", 50, 3, 4, 0.75f}},
        {"cow",     {"cow",     "Vache",  61, 1500, 1000, 5, "hay", "",
                     "milk", 1, "milk_large", 50, 5, 6, 1.0f}},
        {"sheep",   {"sheep",   "Mouton", 62, 2000, 1500, 5, "hay", "",
                     "wool", 3, "wool", 0, 4, 6, 0.95f}},
    };
    return t;
}

const AnimalDef* animalDef(const string& id) {
    auto& t = animalTable();
    auto it = t.find(id);
    return it == t.end() ? nullptr : &it->second;
}

// ---------------- Poissons ----------------
static const std::unordered_map<string, FishDef>& fishTable() {
    static const std::unordered_map<string, FishDef> t = {
        {"fish_roach", {"fish_roach", "Gardon",  50, 30,  0.6f, (uint8_t)((1<<SPRING)|(1<<SUMMER)|(1<<AUTUMN))}},
        {"fish_trout", {"fish_trout", "Truite",  51, 75,  1.0f, (uint8_t)((1<<SPRING)|(1<<SUMMER))}},
        {"fish_carp",  {"fish_carp",  "Carpe",   52, 90,  1.2f, (uint8_t)((1<<SUMMER)|(1<<AUTUMN))}},
        {"fish_pike",  {"fish_pike",  "Brochet", 53, 150, 1.8f, (uint8_t)((1<<AUTUMN)|(1<<WINTER))}},
    };
    return t;
}

const FishDef* fishDef(const string& id) {
    auto& t = fishTable();
    auto it = t.find(id);
    return it == t.end() ? nullptr : &it->second;
}

// ---------------- Magasin ----------------
vector<ShopEntry> shopCatalog() {
    return {
        {"tool_axe",    750,  "Hache",         3},
        {"tool_scythe", 1000, "Faux",          4},
        {"tool_rod",    1250, "Canne à pêche", 5},
        {"tool_pail",   500,  "Seau",          6},
        {"tool_shears", 750,  "Tondeuse",      7},
        {"seed_wheat",     10, "Graines de blé",        10},
        {"seed_potato",    20, "Graines de patate",     11},
        {"seed_strawberry",50, "Graines de fraise",     12},
        {"seed_corn",      25, "Graines de maïs",       13},
        {"seed_tomato",    30, "Graines de tomate",     14},
        {"seed_sunflower", 30, "Graines de tournesol",  15},
        {"seed_pumpkin",   60, "Graines de citrouille", 16},
        {"seed_kale",      35, "Graines de chou kale",  17},
        {"hay",        25,  "Foin",          40},
        {"fertilizer", 50,  "Engrais",       43},
        {"animal:chicken",  500, "Poule",        60},
        {"animal:cow",     1500, "Vache",        61},
        {"animal:sheep",   2000, "Mouton",       62},
    };
}

const CanUpgrade CAN_UPGRADES[3] = {
    {1, 0,     "Arrosoir (1 case)",  1},
    {2, 2000,  "Arrosoir acier (1x3)", 3},
    {3, 5000,  "Arrosoir or (3x3)",   9},
};

// ---------------- Quêtes ----------------
const QuestTemplate QUEST_POOL[] = {
    {"harvest", "wheat",      5,  300, SK_FARMING,  40},
    {"harvest", "strawberry", 3,  500, SK_FARMING,  60},
    {"harvest", "pumpkin",    2,  600, SK_FARMING,  80},
    {"harvest", "corn",       4,  400, SK_FARMING,  50},
    {"plant",   "",           8,  250, SK_FARMING,  40},
    {"water",   "",           10, 200, SK_FARMING,  30},
    {"feed",    "",           3,  250, SK_ANIMAL,   40},
    {"earn",    "",           800, 400, -1,         0},
    {"fish",    "",           2,  300, SK_FISHING,  50},
    {"wood",    "",           5,  250, SK_FORAGING, 40},
};
const int QUEST_POOL_COUNT = (int)(sizeof(QUEST_POOL) / sizeof(QUEST_POOL[0]));

} // namespace fv
