#pragma once
// ============================================================
// FarmVale — monde : cartes, génération, zones, collisions
// ============================================================
#include "types.h"
#include "constants.h"

namespace fv {

struct World {
    MapData farm;
    MapData home;

    // Construit les cartes statiques ; si scatter=true, peuple aussi st.nodes
    // (arbres/buissons vivants, persistés dans la sauvegarde).
    void build(GameState& st, bool scatter);

    MapData& current(GameState& st);
    const MapData& current(const GameState& st) const;

    // Zones spéciales de la carte ferme
    static bool inForest(int x, int y)   { return x >= 0 && x <= 6 && y >= 12 && y < MAP_H; }
    static bool inPasture(int x, int y)  { return x >= 9 && x <= 25 && y >= 21 && y <= 27; }
    static bool inPondZone(int x, int y) { return x >= 33 && x <= 47 && y >= 18 && y < MAP_H; }

    // Une tuile d'herbe labourable ? (prend en compte la nature dynamique)
    bool tillable(const GameState& st, int x, int y) const;

    // Collision au niveau pixel (boîte du joueur)
    bool solidAt(const GameState& st, const MapData& m, float px, float py) const;

    // Positions d'apparition
    static constexpr int FARM_SPAWN_TX = 8, FARM_SPAWN_TY = 9;   // à côté de la maison
    static constexpr int PASTURE_CX = 17, PASTURE_CY = 24;
    static constexpr int HOME_DOOR_TX = 10, HOME_DOOR_TY = 13;   // sortie maison
    static constexpr int FARM_DOOR_TX = 8, FARM_DOOR_TY = 7;     // retour ferme

private:
    void genFarm(GameState& st, bool scatter);
    void genHome();
    void scatterNature(GameState& st, uint64_t seed);
};

// Fabrique un état de nouvelle partie
void newGameState(GameState& st, uint64_t seed);

} // namespace fv
