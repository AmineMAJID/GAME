#pragma once
// ============================================================
// FarmVale — catalogue des objets, cultures, animaux, poissons
// ============================================================
#include "types.h"
#include "constants.h"

namespace fv {

enum ItemType { IT_TOOL, IT_SEED, IT_CROP, IT_PRODUCT, IT_RESOURCE, IT_FISH, IT_FEED };

struct ItemDef {
    string id;
    string name;
    ItemType type;
    int icon = 0;        // id d'icône (voir icons.cpp)
    int sell = 0;        // prix de vente de base (qualité 0)
    int stack = MAX_STACK;
    string desc;
    string crop;         // pour IT_SEED : culture associée
};

struct CropDef {
    string id;
    string name;
    int icon;
    uint8_t seasons;     // bitmask (1<<Season)
    int days;            // jours pour pousser
    int regrow;          // jours entre deux récoltes (0 = une seule)
    string seedId;
    string productId;
    bool growsInSeason(int s) const { return (seasons >> s) & 1; }
};

struct AnimalDef {
    string id;
    string name;
    int icon;
    int buy;
    int sell;
    int adultAge;        // jours pour devenir adulte
    string feedItem;     // aliment de base
    string feedAlt;      // aliment alternatif accepté (vide = aucun)
    string produceItem;
    int produceEvery;    // tous les N jours
    string largeItem;    // version "grande" (amitié élevée)
    int largeMinFriend;
    int gestation;       // jours de gestation
    int breedCooldown;   // jours min entre deux reproductions
    float scale;         // taille d'affichage
};

struct FishDef {
    string id;
    string name;
    int icon;
    int sell;
    float difficulty;    // 0..2 (vitesse du mini-jeu)
    uint8_t seasons;     // bitmask
};

const ItemDef* itemDef(const string& id);
const CropDef* cropDef(const string& id);
const AnimalDef* animalDef(const string& id);
const FishDef* fishDef(const string& id);

int itemSellPrice(const string& id, int q);   // prix selon la qualité
int seedPrice(const string& seedId);

// Recherche d'un ItemStack dans une liste (pour hotbar/outils)
int findItem(vector<ItemStack>& bag, const string& id);

// --------- Listes pour le magasin ---------
struct ShopEntry {
    string itemId;   // objet (outil/graine/ressource) ou animal:("animal:"+id)
    int price;
    string label;
    int icon;
};
vector<ShopEntry> shopCatalog();

// --------- Upgrade d'arrosoir ---------
struct CanUpgrade { int level; int price; const char* name; int aoe; };
extern const CanUpgrade CAN_UPGRADES[3];

// --------- Cibles de quête ---------
struct QuestTemplate {
    string kind;
    string arg;      // culture cible ou ""
    int count;
    int rewardMoney;
    int rewardSkill;
    int rewardXp;
};
extern const QuestTemplate QUEST_POOL[];
extern const int QUEST_POOL_COUNT;

} // namespace fv
