#pragma once
// ============================================================
// FarmVale — Sim : cœur de la simulation (logique pure, testable)
// Propre, sans aucune dépendance d'affichage.
// ============================================================
#include "types.h"
#include "world.h"
#include "rng.h"
#include "items.h"
#include "constants.h"

namespace fv {

struct Toast { string msg; float ttl = 3.2f; int kind = 0; }; // 0 info, 1 ok, 2 warn, 3 erreur
struct Event { string type; string arg; int amount = 0; };    // ex: {"sfx","harvest"}, {"ach","ACH_RICH"}

class Sim {
public:
    GameState st;
    World world;
    mutable Rng rng;
    vector<Toast> toasts;
    vector<Event> events;
    bool timePaused = false;     // menus ouverts
    float gameSpeed = 1.0f;      // multiplicateur de vitesse

    Sim();

    void newGame(uint64_t seed);
    bool loadFromFile(const string& path);
    bool saveToFile(const string& path) const;

    // ---- Temps ----
    void advanceTime(float dtSeconds);
    void advanceDay();
    void sleepNow();
    void passOut();
    void rollWeather();

    // ---- Inventaire ----
    int addItem(const string& id, int q, int n);       // retourne la quantité ajoutée
    bool removeItem(const string& id, int q, int n);
    int countItem(const string& id) const;
    void selectSlot(int i);
    ItemStack& selected();
    const ItemStack& selected() const;
    bool moveStack(vector<ItemStack>& from, int i, vector<ItemStack>& to, int j);
    void sortBag(vector<ItemStack>& bag);

    // ---- Économie ----
    bool buy(const string& shopId);   // "tool_axe" | "seed_wheat" | "animal:cow" | "upg_can" | "upg_energy"
    int sellStack(vector<ItemStack>& bag, int idx, int n);
    int shipAll();
    int sellAnimal(int idx);

    // ---- Actions sur le monde ----
    bool actionAt(int tx, int ty);    // action contextuelle (interagir)
    bool tillAt(int tx, int ty);
    bool plantAt(int tx, int ty);
    bool waterAt(int tx, int ty);
    bool fertilizeAt(int tx, int ty);
    bool harvestAt(int tx, int ty);
    bool chopAt(int tx, int ty);
    bool scytheAt(int tx, int ty);
    bool forageAt(int tx, int ty);
    int animalNear(float px, float py, float range) const;
    bool feedAnimal(int idx, const string& feedId);
    bool petAnimal(int idx);
    bool collectAnimal(int idx, const string& toolId);
    bool useStructure(int tx, int ty); // porte/lit/coffre/panneau/comptoir/caisse -> événements
    void catchFish(const string& fishId, int q);

    // ---- Quêtes ----
    void generateDailyQuests();
    void emit(const string& kind, const string& arg = "", int amount = 1);

    // ---- Animaux ----
    void updateAnimals(float dt);
    void dailyAnimals();
    void checkBreeding();

    // ---- Utilitaires ----
    void toast(const string& msg, int kind = 0);
    void addXp(int skill, int amount);
    void addMoney(int amount, bool earned = false);
    bool spendEnergy(float e);
    Plot* plotAt(int tx, int ty);
    const Plot* plotAt(int tx, int ty) const;
    Node* nodeAt(int tx, int ty);
    bool isRaining() const { return st.weather == W_RAIN || st.weather == W_STORM || st.weather == W_SNOW; }
    int hour() const { return st.hour(); }
    bool qualityRoll(int skill, bool fert, int& q) const; // 0/1/2

    int sel = 0;

private:
    void giveStarter();
};

} // namespace fv
