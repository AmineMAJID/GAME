#include "sim.h"
#include <cmath>
#include <algorithm>

namespace fv {

// Portée d'interaction (en tuiles)
static constexpr float REACH = 1.7f;

static bool inReach(const GameState& st, int tx, int ty) {
    float dx = (tx * TILE + TILE / 2.0f) - st.px;
    float dy = (ty * TILE + TILE / 2.0f) - st.py;
    return std::sqrt(dx * dx + dy * dy) <= REACH * TILE;
}

// ------------------------------------------------------------
// Action contextuelle (touche Espace / clic souris)
// ------------------------------------------------------------
bool Sim::actionAt(int tx, int ty) {
    if (st.mapId == "home" && world.home.o(tx, ty) == T_NONE && world.home.g(tx, ty) != T_POND) {
        // à l'intérieur, seules les structures sont interactives
    }
    if (!inReach(st, tx, ty)) {
        toast("Trop loin…", 2);
        return false;
    }
    MapData& m = world.current(st);
    if (!m.inBounds(tx, ty)) return false;
    uint8_t obj = m.o(tx, ty);

    // 1) Structures interactives
    if (obj == T_DOOR || obj == T_BED || obj == T_CHEST || obj == T_BOARD || obj == T_COUNTER || obj == T_BIN)
        return useStructure(tx, ty);

    // 2) Animaux à proximité
    int ai = animalNear(st.px, st.py, REACH * TILE);
    if (ai >= 0) {
        const ItemStack& s = selected();
        const ItemDef* sd = s.empty() ? nullptr : itemDef(s.id);
        if (sd && (sd->type == IT_FEED || s.id == "corn")) return feedAnimal(ai, s.id);
        if (sd && sd->type == IT_TOOL) {
            if (s.id == "tool_pail" || s.id == "tool_shears") return collectAnimal(ai, s.id);
            if (s.id == "tool_axe" || s.id == "tool_scythe") { /* on pète/tonte par défaut */ }
        }
        return petAnimal(ai);
    }

    // 3) Parcelles
    Plot* p = plotAt(tx, ty);
    const ItemStack& s = selected();
    const ItemDef* sd = s.empty() ? nullptr : itemDef(s.id);
    if (p && !p->crop.empty() && !p->dead) {
        const CropDef* c = cropDef(p->crop);
        if (c && p->progress >= c->days) return harvestAt(tx, ty);
    }
    if (sd && sd->type == IT_SEED) return plantAt(tx, ty);
    if (sd && s.id == "fertilizer") return fertilizeAt(tx, ty);
    if (sd && s.id == "tool_can") return waterAt(tx, ty);

    // 4) Nature
    Node* n = nodeAt(tx, ty);
    if (n && n->kind == "tree" && sd && s.id == "tool_axe") return chopAt(tx, ty);
    if (n && n->kind == "bush" && n->a == 1) return forageAt(tx, ty);

    // 5) Sol
    uint8_t g = m.g(tx, ty);
    if (g == T_GRASS) {
        if (world.tillable(st, tx, ty)) {
            if (sd && s.id == "tool_hoe") return tillAt(tx, ty);
            if (sd && sd->type == IT_SEED) return plantAt(tx, ty); // labour + plante en un geste
            if (sd && s.id == "tool_scythe") return scytheAt(tx, ty);
        } else if (sd && s.id == "tool_scythe" && !n) {
            return scytheAt(tx, ty);
        }
    }
    if (g == T_POND && sd && s.id == "tool_rod") {
        if (spendEnergy(ENERGY_FISH)) {
            events.push_back({"fish_start", "", 0});
            return true;
        }
        return false;
    }

    // 6) Swing d'outil sur le vide
    if (sd && sd->type == IT_TOOL) {
        if (spendEnergy(ENERGY_SWING)) {
            events.push_back({"sfx", "swing", 0});
            return true;
        }
    }
    return false;
}

// ------------------------------------------------------------
// Labour
// ------------------------------------------------------------
bool Sim::tillAt(int tx, int ty) {
    MapData& m = world.current(st);
    if (!m.inBounds(tx, ty)) return false;
    if (m.g(tx, ty) != T_GRASS || !world.tillable(st, tx, ty)) {
        toast("On ne peut pas labourer ici.", 2);
        return false;
    }
    if (!spendEnergy(ENERGY_TILL)) return false;
    m.setG(tx, ty, T_SOIL);
    events.push_back({"sfx", "till", 0});
    return true;
}

// ------------------------------------------------------------
// Plantation
// ------------------------------------------------------------
bool Sim::plantAt(int tx, int ty) {
    if (st.mapId != "farm") { toast("On ne plante que dehors.", 2); return false; }
    const ItemStack& s = selected();
    const ItemDef* sd = itemDef(s.id);
    if (!sd || sd->type != IT_SEED) return false;
    const CropDef* c = cropDef(sd->crop);
    if (!c) return false;

    MapData& m = world.current(st);
    if (!m.inBounds(tx, ty)) return false;

    // Labour automatique si le joueur a la houe
    if (m.g(tx, ty) == T_GRASS) {
        if (!world.tillable(st, tx, ty)) { toast("On ne peut pas planter ici.", 2); return false; }
        if (countItem("tool_hoe") < 1) { toast("Il vous faut une houe.", 3); return false; }
        if (!spendEnergy(ENERGY_TILL)) return false;
        m.setG(tx, ty, T_SOIL);
        events.push_back({"sfx", "till", 0});
    }
    if (m.g(tx, ty) != T_SOIL) { toast("La terre doit être labourée.", 2); return false; }

    // Nettoie une culture morte
    Plot* p = plotAt(tx, ty);
    if (p && p->dead) st.plots.erase(tileKey(tx, ty)), p = nullptr;
    if (p && !p->crop.empty()) { toast("Il y a déjà une culture ici.", 2); return false; }

    if (!c->growsInSeason(st.season())) {
        toast(string(c->name) + " ne pousse pas en " + seasonName(st.season()) + ".", 3);
        return false;
    }
    if (!spendEnergy(ENERGY_PLANT)) return false;
    removeItem(s.id, 0, 1);
    st.plots[tileKey(tx, ty)] = Plot{c->id, st.day, 0.0f, false, false, false};
    events.push_back({"sfx", "plant", 0});
    emit("plant", "", 1);
    return true;
}

// ------------------------------------------------------------
// Arrosage (avec zone selon le niveau de l'arrosoir)
// ------------------------------------------------------------
bool Sim::waterAt(int tx, int ty) {
    if (st.mapId != "farm") return false;
    int watered = 0;
    auto tryWater = [&](int x, int y) {
        Plot* p = plotAt(x, y);
        if (p && !p->crop.empty() && !p->dead && !p->watered) {
            p->watered = true;
            watered++;
        }
    };
    tryWater(tx, ty);
    if (st.canLevel >= 2) {
        // 1x3 dans la direction du joueur
        int dx = 0, dy = 0;
        if (st.facing == FACING_LEFT) dx = -1;
        else if (st.facing == FACING_RIGHT) dx = 1;
        else if (st.facing == FACING_UP) dy = -1;
        else dy = 1;
        tryWater(tx + dx, ty + dy);
        tryWater(tx + 2 * dx, ty + 2 * dy);
    }
    if (st.canLevel >= 3) {
        for (int y = ty - 1; y <= ty + 1; y++)
            for (int x = tx - 1; x <= tx + 1; x++)
                tryWater(x, y);
    }
    if (watered == 0) {
        // Arroser la terre sèche la réhumidifie aussi (utile)
        MapData& m = world.current(st);
        if (m.inBounds(tx, ty) && m.g(tx, ty) == T_SOIL) {
            events.push_back({"sfx", "water", 0});
            return spendEnergy(ENERGY_WATER);
        }
        toast("Rien à arroser ici.", 2);
        return false;
    }
    if (!spendEnergy(ENERGY_WATER)) return false;
    events.push_back({"sfx", "water", 0});
    emit("water", "", watered);
    return true;
}

// ------------------------------------------------------------
// Engrais
// ------------------------------------------------------------
bool Sim::fertilizeAt(int tx, int ty) {
    if (st.mapId != "farm") return false;
    Plot* p = plotAt(tx, ty);
    bool soilOnly = false;
    MapData& m = world.current(st);
    if (!p && m.inBounds(tx, ty) && m.g(tx, ty) == T_SOIL) soilOnly = true;
    if (!p && !soilOnly) { toast("Rien à engraisser ici.", 2); return false; }
    if (p && (p->fert || p->dead)) { toast("Déjà traité.", 2); return false; }
    if (countItem("fertilizer") < 1) { toast("Il vous faut de l'engrais.", 3); return false; }
    removeItem("fertilizer", 0, 1);
    if (p) p->fert = true;
    events.push_back({"sfx", "plant", 0});
    toast("Engrais appliqué : pousse plus vite et meilleure qualité.", 1);
    return true;
}

// ------------------------------------------------------------
// Récolte
// ------------------------------------------------------------
bool Sim::harvestAt(int tx, int ty) {
    Plot* p = plotAt(tx, ty);
    if (!p || p->crop.empty() || p->dead) return false;
    const CropDef* c = cropDef(p->crop);
    if (!c || p->progress < c->days) { toast("Ce n'est pas encore mûr.", 2); return false; }
    if (!spendEnergy(ENERGY_HARVEST)) return false;

    int farmLv = st.skillLevel(SK_FARMING);
    int yield = 1;
    if (rng.chance(0.25f + farmLv * 0.03f)) yield = 2;
    int q = 0;
    qualityRoll(SK_FARMING, p->fert, q);

    int got = addItem(c->productId, q, yield);
    if (got < yield) toast("Inventaire plein !", 2);

    st.statCropsHarvested += (uint64_t)yield;
    addXp(SK_FARMING, 10);
    events.push_back({"sfx", "harvest", 0});
    emit("harvest", p->crop, yield);

    if (st.statCropsHarvested == 1) events.push_back({"ach", AchievementIds::FIRST_HARVEST, 0});
    if (st.statCropsHarvested >= 100) events.push_back({"ach", AchievementIds::GREEN_THUMB, 0});

    string cropName = c->name;
    if (c->regrow > 0) {
        // Repousse : on conserve la parcelle
        p->progress = (float)(c->days - c->regrow);
        toast(string("Récolté : ") + qualityName(q) + cropName + " x" + std::to_string(yield), 1);
    } else {
        toast(string("Récolté : ") + qualityName(q) + cropName + " x" + std::to_string(yield), 1);
        st.plots.erase(tileKey(tx, ty));
    }
    return true;
}

// ------------------------------------------------------------
// Coupe de bois
// ------------------------------------------------------------
bool Sim::chopAt(int tx, int ty) {
    Node* n = nodeAt(tx, ty);
    if (!n || n->kind != "tree") return false;
    if (!spendEnergy(ENERGY_CHOP)) return false;
    n->a--;
    events.push_back({"sfx", "chop", 0});
    if (n->a <= 0) {
        n->kind = "stump";
        n->a = st.day + 10; // repousse dans 10 jours
        n->b = 0;
        int wood = rng.chance(0.25f) ? 2 : 1;
        addItem("wood", 0, wood);
        addXp(SK_FORAGING, 10);
        emit("wood", "", wood);
        toast(string("Arbre abattu : +") + std::to_string(wood) + " bois", 1);
    }
    return true;
}

// ------------------------------------------------------------
// Fauchage (foin)
// ------------------------------------------------------------
bool Sim::scytheAt(int tx, int ty) {
    MapData& m = world.current(st);
    if (!m.inBounds(tx, ty)) return false;
    if (nodeAt(tx, ty)) { toast("Quelque chose bloque ici.", 2); return false; }
    if (m.g(tx, ty) != T_GRASS) { toast("Pas d'herbe à faucher ici.", 2); return false; }
    if (!spendEnergy(ENERGY_SCYTHE)) return false;
    int hay = rng.chance(0.30f) ? 2 : 1;
    addItem("hay", 0, hay);
    addXp(SK_FORAGING, 2);
    events.push_back({"sfx", "scythe", 0});
    return true;
}

// ------------------------------------------------------------
// Cueillette (buissons)
// ------------------------------------------------------------
bool Sim::forageAt(int tx, int ty) {
    Node* n = nodeAt(tx, ty);
    if (!n || n->kind != "bush" || n->a == 0) { toast("Pas de baies mûres ici.", 2); return false; }
    if (!spendEnergy(ENERGY_FORAGE)) return false;
    n->a = 0;
    n->b = st.day + 3;
    int berries = rng.chance(0.30f) ? 2 : 1;
    addItem("berry", 0, berries);
    addXp(SK_FORAGING, 10);
    events.push_back({"sfx", "forage", 0});
    return true;
}

// ------------------------------------------------------------
// Animaux : nourriture / caresse / récolte
// ------------------------------------------------------------
int Sim::animalNear(float px, float py, float range) const {
    int best = -1;
    float bestD = range;
    for (size_t i = 0; i < st.animals.size(); i++) {
        const Animal& a = st.animals[i];
        float dx = a.x - px, dy = a.y - py;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d < bestD) { bestD = d; best = (int)i; }
    }
    return best;
}

bool Sim::feedAnimal(int idx, const string& feedId) {
    if (idx < 0 || idx >= (int)st.animals.size()) return false;
    Animal& a = st.animals[idx];
    const AnimalDef* d = animalDef(a.type);
    if (!d) return false;
    bool ok = (feedId == d->feedItem) || (!d->feedAlt.empty() && feedId == d->feedAlt);
    if (!ok) { toast("Cet animal ne mange pas ça.", 3); return false; }
    if (a.fedToday) { toast("Déjà nourri aujourd'hui.", 2); return false; }
    if (countItem(feedId) < 1) { toast("Plus de nourriture.", 3); return false; }
    if (!spendEnergy(ENERGY_FEED)) return false;
    removeItem(feedId, 0, 1);
    a.fedToday = true;
    a.happiness = std::min(100, a.happiness + 10);
    float mult = 1.0f + st.skillLevel(SK_ANIMAL) * 0.05f;
    a.friendship = std::min(100, (int)(a.friendship + 5 * mult));
    addXp(SK_ANIMAL, 5);
    events.push_back({"sfx", "feed", 0});
    emit("feed", "", 1);
    toast(string("Miam ! ") + d->name + " adore.", 1);
    return true;
}

bool Sim::petAnimal(int idx) {
    if (idx < 0 || idx >= (int)st.animals.size()) return false;
    Animal& a = st.animals[idx];
    if (a.pettedToday) { toast("Déjà câliné aujourd'hui.", 2); return false; }
    if (!spendEnergy(ENERGY_PET)) return false;
    a.pettedToday = true;
    float mult = 1.0f + st.skillLevel(SK_ANIMAL) * 0.05f;
    a.friendship = std::min(100, (int)(a.friendship + 15 * mult));
    a.happiness = std::min(100, a.happiness + 5);
    addXp(SK_ANIMAL, 5);
    events.push_back({"sfx", "pet", 0});
    events.push_back({"hearts", std::to_string(a.id), 0});
    return true;
}

bool Sim::collectAnimal(int idx, const string& toolId) {
    if (idx < 0 || idx >= (int)st.animals.size()) return false;
    Animal& a = st.animals[idx];
    const AnimalDef* d = animalDef(a.type);
    if (!d) return false;
    if (!a.isAdult()) { toast("C'est encore un bébé.", 2); return false; }
    string need = (a.type == "sheep") ? "tool_shears" : "tool_pail";
    if (toolId != need) {
        toast(a.type == "sheep" ? "Il faut une tondeuse." : "Il vous faut un seau.", 3);
        return false;
    }
    if (a.ready <= 0) { toast("Rien à récolter pour l'instant.", 2); return false; }
    if (!spendEnergy(a.type == "sheep" ? ENERGY_SHEAR : ENERGY_MILK)) return false;

    string item = d->produceItem;
    if (!d->largeItem.empty() && a.friendship >= d->largeMinFriend) item = d->largeItem;
    int n = a.ready;
    addItem(item, 0, n);
    a.ready = 0;
    a.happiness = std::min(100, a.happiness + 5);
    addXp(SK_ANIMAL, 10);
    events.push_back({"sfx", "collect", 0});
    toast(string("Récolté : ") + itemDef(item)->name + " x" + std::to_string(n), 1);
    return true;
}

// ------------------------------------------------------------
// Structures (portes, lit, coffre, panneau, marché, caisse)
// ------------------------------------------------------------
bool Sim::useStructure(int tx, int ty) {
    MapData& m = world.current(st);
    if (!m.inBounds(tx, ty)) return false;
    uint8_t obj = m.o(tx, ty);
    switch (obj) {
        case T_DOOR:
            events.push_back({"sfx", "door", 0});
            if (st.mapId == "farm") {
                st.mapId = "home";
                st.px = (float)(World::HOME_DOOR_TX * TILE + TILE / 2);
                st.py = (float)((World::HOME_DOOR_TY - 1) * TILE + TILE - 4);
            } else {
                st.mapId = "farm";
                st.px = (float)(World::FARM_DOOR_TX * TILE + TILE / 2);
                st.py = (float)(World::FARM_DOOR_TY * TILE + TILE - 4);
            }
            return true;
        case T_BED:
            events.push_back({"sleep_ui", "", 0});
            return true;
        case T_CHEST:
            events.push_back({"open_chest", "", 0});
            events.push_back({"sfx", "open", 0});
            return true;
        case T_BOARD:
            events.push_back({"open_quests", "", 0});
            events.push_back({"sfx", "open", 0});
            return true;
        case T_COUNTER:
            events.push_back({"open_shop", "", 0});
            events.push_back({"sfx", "open", 0});
            return true;
        case T_BIN: {
            int total = shipAll();
            if (total > 0) toast(string("Expédié pour ") + std::to_string(total) + " pièces.", 1);
            else toast("Rien à expédier.", 2);
            return true;
        }
        default:
            return false;
    }
}

// ------------------------------------------------------------
// Pêche
// ------------------------------------------------------------
void Sim::catchFish(const string& fishId, int q) {
    const FishDef* f = fishDef(fishId);
    if (!f) return;
    addItem(fishId, q, 1);
    st.statFishCaught++;
    addXp(SK_FISHING, 15);
    events.push_back({"sfx", "catch", 0});
    emit("fish", "", 1);
    if (st.statFishCaught >= 50) events.push_back({"ach", AchievementIds::FISHERMAN, 0});
    toast(string("Pris : ") + qualityName(q) + f->name + " !", 1);
}


} // namespace fv
