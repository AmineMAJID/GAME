#include "sim.h"
#include <algorithm>

namespace fv {

Sim::Sim() : world(), rng(1) {}

void Sim::newGame(uint64_t seed) {
    newGameState(st, seed);
    rng = Rng(st.seed * 2654435761u + 12345);
    world.build(st, true);
    rollWeather();
    generateDailyQuests();
    toast("Bienvenue à FarmVale ! Bonne première journée.", 1);
}

void Sim::giveStarter() {}

// ------------------------------------------------------------
// Utilitaires
// ------------------------------------------------------------
void Sim::toast(const string& msg, int kind) {
    // Un message identique rafraîchit le toast existant au lieu de s'empiler
    for (auto& t : toasts) {
        if (t.msg == msg) { t.ttl = 3.5f; t.kind = kind; return; }
    }
    toasts.push_back({msg, 3.5f, kind});
    if (toasts.size() > 6) toasts.erase(toasts.begin());
}

void Sim::addMoney(int amount, bool earned) {
    st.money += amount;
    if (earned) {
        st.earningsToday += amount;
        st.statMoneyEarned += (uint64_t)amount;
        if (st.money >= 50000)
            events.push_back({"ach", AchievementIds::RICH, 0});
    }
}

void Sim::addXp(int skill, int amount) {
    if (skill < 0 || skill >= SK_COUNT) return;
    int before = st.skillLevel(skill);
    st.skills[skill] += amount;
    int after = st.skillLevel(skill);
    if (after > before) {
        toast(string("Niveau ") + skillName(skill) + " : " + std::to_string(after) + " !", 1);
        events.push_back({"sfx", "levelup", 0});
        if (skill == SK_FARMING && after >= 10)
            events.push_back({"ach", AchievementIds::FARMER_10, 0});
    }
}

bool Sim::spendEnergy(float e) {
    if (st.energy < e) {
        toast("Trop fatigué…", 2);
        return false;
    }
    st.energy -= e;
    if (st.energy < 0) st.energy = 0;
    return true;
}

Plot* Sim::plotAt(int tx, int ty) {
    auto it = st.plots.find(tileKey(tx, ty));
    return it == st.plots.end() ? nullptr : &it->second;
}
const Plot* Sim::plotAt(int tx, int ty) const {
    auto it = st.plots.find(tileKey(tx, ty));
    return it == st.plots.end() ? nullptr : &it->second;
}

Node* Sim::nodeAt(int tx, int ty) {
    auto it = st.nodes.find(tileKey(tx, ty));
    return it == st.nodes.end() ? nullptr : &it->second;
}

bool Sim::qualityRoll(int skill, bool fert, int& q) const {
    int lv = st.skillLevel(skill);
    float gold = 0.04f + lv * 0.02f + (fert ? 0.10f : 0.0f);
    float silver = 0.12f + lv * 0.03f + (fert ? 0.10f : 0.0f);
    float r = rng.uf();
    if (r < gold) { q = 2; return true; }
    if (r < gold + silver) { q = 1; return true; }
    q = 0;
    return true;
}

// ------------------------------------------------------------
// Inventaire
// ------------------------------------------------------------
int Sim::addItem(const string& id, int q, int n) {
    const ItemDef* def = itemDef(id);
    if (!def || n <= 0) return 0;
    int left = n;
    // D'abord dans les piles existantes (hotbar puis sac)
    for (auto* bag : {&st.hotbar, &st.backpack}) {
        for (auto& s : *bag) {
            if (left <= 0) break;
            if (s.sameKind(ItemStack{id, q, 1}) && s.n < def->stack) {
                int add = def->stack - s.n;
                if (add > left) add = left;
                s.n += add;
                left -= add;
            }
        }
    }
    // Puis dans les cases vides
    for (auto* bag : {&st.hotbar, &st.backpack}) {
        for (auto& s : *bag) {
            if (left <= 0) break;
            if (s.empty()) {
                int add = def->stack < left ? def->stack : left;
                s = ItemStack{id, q, add};
                left -= add;
            }
        }
    }
    return n - left;
}

bool Sim::removeItem(const string& id, int q, int n) {
    int left = n;
    for (auto* bag : {&st.hotbar, &st.backpack}) {
        for (auto& s : *bag) {
            if (left <= 0) break;
            if (!s.empty() && s.id == id && s.q == q) {
                int take = s.n < left ? s.n : left;
                s.n -= take;
                left -= take;
                if (s.n <= 0) s.clear();
            }
        }
    }
    return left == 0;
}

int Sim::countItem(const string& id) const {
    int total = 0;
    for (auto* bag : {&st.hotbar, &st.backpack})
        for (auto& s : *bag)
            if (!s.empty() && s.id == id) total += s.n;
    return total;
}

void Sim::selectSlot(int i) {
    if (i >= 0 && i < (int)st.hotbar.size()) sel = i;
}

ItemStack& Sim::selected() { return st.hotbar[sel]; }
const ItemStack& Sim::selected() const { return st.hotbar[sel]; }

bool Sim::moveStack(vector<ItemStack>& from, int i, vector<ItemStack>& to, int j) {
    if (i < 0 || i >= (int)from.size() || j < 0 || j >= (int)to.size()) return false;
    ItemStack& a = from[i];
    ItemStack& b = to[j];
    if (a.empty()) return false;
    if (b.empty()) {
        b = a;
        a.clear();
        return true;
    }
    if (a.sameKind(b)) {
        const ItemDef* def = itemDef(a.id);
        int maxS = def ? def->stack : MAX_STACK;
        int add = maxS - b.n;
        if (add > a.n) add = a.n;
        b.n += add;
        a.n -= add;
        if (a.n <= 0) a.clear();
        return true;
    }
    std::swap(a, b);
    return true;
}

void Sim::sortBag(vector<ItemStack>& bag) {
    std::sort(bag.begin(), bag.end(), [](const ItemStack& a, const ItemStack& b) {
        if (a.empty() && b.empty()) return false;
        if (a.empty()) return false;
        if (b.empty()) return true;
        if (a.id != b.id) return a.id < b.id;
        return a.q < b.q;
    });
}

} // namespace fv
