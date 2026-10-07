#include "sim.h"

namespace fv {

// Trouve le prix d'un article du catalogue
static int shopPrice(const string& id) {
    for (auto& e : shopCatalog())
        if (e.itemId == id) return e.price;
    const ItemDef* d = itemDef(id);
    return d ? d->sell : 0;
}

// ------------------------------------------------------------
// Achat
// ------------------------------------------------------------
bool Sim::buy(const string& shopId) {
    // Améliorations
    if (shopId == "upg_can") {
        if (st.canLevel >= 3) { toast("Arrosoir déjà au maximum.", 2); return false; }
        int price = CAN_UPGRADES[st.canLevel].price;
        if (st.money < price) { toast("Pas assez d'argent.", 3); return false; }
        st.money -= price;
        st.canLevel++;
        events.push_back({"sfx", "coin", 0});
        toast("Arrosoir amélioré !", 1);
        return true;
    }
    if (shopId == "upg_energy") {
        if (st.energyUp >= 4) { toast("Énergie déjà au maximum.", 2); return false; }
        static const int prices[4] = {1000, 2500, 5000, 10000};
        int price = prices[st.energyUp];
        if (st.money < price) { toast("Pas assez d'argent.", 3); return false; }
        st.money -= price;
        st.energyUp++;
        st.energyMax = st.energyMaxBase();
        events.push_back({"sfx", "coin", 0});
        toast("Énergie max augmentée (+25) !", 1);
        return true;
    }

    // Animaux
    if (shopId.rfind("animal:", 0) == 0) {
        string type = shopId.substr(7);
        const AnimalDef* d = animalDef(type);
        if (!d) return false;
        if ((int)st.animals.size() >= MAX_ANIMALS) { toast("Le pâturage est plein !", 3); return false; }
        if (st.money < d->buy) { toast("Pas assez d'argent.", 3); return false; }
        st.money -= d->buy;
        Animal a;
        a.id = st.nextAnimalId++;
        a.type = type;
        a.x = (World::PASTURE_CX * TILE + TILE / 2) + rng.range(-60, 60);
        a.y = (World::PASTURE_CY * TILE + TILE / 2) + rng.range(-40, 40);
        a.age = d->adultAge; // animaux achetés adultes
        a.friendship = 10;
        a.happiness = 100;
        a.dir = rng.uf() * 6.28f;
        a.moveTimer = 1.0f;
        st.animals.push_back(a);
        st.animalCounter++;
        events.push_back({"sfx", "coin", 0});
        if (st.animalCounter == 1) events.push_back({"ach", AchievementIds::FIRST_ANIMAL, 0});
        toast(string("Bienvenue à votre ") + d->name + " !", 1);
        return true;
    }

    // Objets
    const ItemDef* def = itemDef(shopId);
    if (!def) return false;
    int price = shopPrice(shopId);
    if (price <= 0 && def->type != IT_TOOL) { toast("Cet objet n'est pas en vente.", 3); return false; }
    if (def->type == IT_TOOL && countItem(shopId) >= 1) { toast("Outil déjà possédé.", 2); return false; }
    if (st.money < price) { toast("Pas assez d'argent.", 3); return false; }
    st.money -= price;
    addItem(shopId, 0, 1);
    events.push_back({"sfx", "coin", 0});
    toast(string("Acheté : ") + def->name, 1);
    return true;
}

// ------------------------------------------------------------
// Vente d'une pile
// ------------------------------------------------------------
int Sim::sellStack(vector<ItemStack>& bag, int idx, int n) {
    if (idx < 0 || idx >= (int)bag.size()) return 0;
    ItemStack& s = bag[idx];
    if (s.empty()) return 0;
    const ItemDef* def = itemDef(s.id);
    if (!def || def->sell <= 0) { toast("Cet objet ne se revend pas.", 3); return 0; }
    if (n <= 0 || n > s.n) n = s.n;
    int total = itemSellPrice(s.id, s.q) * n;
    s.n -= n;
    if (s.n <= 0) s.clear();
    addMoney(total, true);
    events.push_back({"sfx", "coin", 0});
    return total;
}

// ------------------------------------------------------------
// Caisse d'expédition : vend tout (sauf les outils)
// ------------------------------------------------------------
int Sim::shipAll() {
    int total = 0;
    for (auto* bag : {&st.hotbar, &st.backpack}) {
        for (size_t i = 0; i < bag->size(); i++) {
            ItemStack& s = (*bag)[i];
            if (s.empty()) continue;
            const ItemDef* def = itemDef(s.id);
            if (!def || def->type == IT_TOOL) continue;
            int price = itemSellPrice(s.id, s.q) * s.n;
            total += price;
            s.clear();
        }
    }
    if (total > 0) {
        addMoney(total, true);
        events.push_back({"sfx", "coin", 0});
    }
    return total;
}

// ------------------------------------------------------------
// Vente d'un animal
// ------------------------------------------------------------
int Sim::sellAnimal(int idx) {
    if (idx < 0 || idx >= (int)st.animals.size()) return 0;
    const AnimalDef* d = animalDef(st.animals[idx].type);
    int price = d ? d->sell : 0;
    toast(string("Vendu pour ") + std::to_string(price) + " pièces.", 1);
    st.animals.erase(st.animals.begin() + idx);
    addMoney(price, true);
    events.push_back({"sfx", "coin", 0});
    return price;
}

} // namespace fv
