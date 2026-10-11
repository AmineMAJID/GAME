#include "sim.h"
#include <utility>

namespace fv {

// ------------------------------------------------------------
// Génération des quêtes du jour (3 quêtes aléatoires)
// ------------------------------------------------------------
void Sim::generateDailyQuests() {
    st.quests.clear();
    vector<int> idx;
    for (int i = 0; i < QUEST_POOL_COUNT; i++) idx.push_back(i);
    // Mélange de Fisher-Yates
    for (int i = (int)idx.size() - 1; i > 0; i--) {
        int j = rng.range(0, i);
        std::swap(idx[i], idx[j]);
    }
    for (int k = 0; k < 3 && k < (int)idx.size(); k++) {
        const QuestTemplate& t = QUEST_POOL[idx[k]];
        Quest q;
        q.id = t.kind + "_" + std::to_string(st.day) + "_" + std::to_string(k);
        q.kind = t.kind;
        q.arg = t.arg;
        q.target = t.count;
        q.rewardMoney = t.rewardMoney;
        q.rewardSkill = t.rewardSkill;
        q.rewardXp = t.rewardXp;
        if (t.kind == "harvest") {
            const CropDef* c = cropDef(t.arg);
            q.text = "Récolter " + std::to_string(t.count) + " " + (c ? c->name : t.arg);
        } else if (t.kind == "plant") {
            q.text = "Planter " + std::to_string(t.count) + " graines";
        } else if (t.kind == "water") {
            q.text = "Arroser " + std::to_string(t.count) + " parcelles";
        } else if (t.kind == "feed") {
            q.text = "Nourrir " + std::to_string(t.count) + " animaux";
        } else if (t.kind == "earn") {
            q.text = "Gagner " + std::to_string(t.count) + " pièces aujourd'hui";
            q.progress = st.earningsToday;
        } else if (t.kind == "fish") {
            q.text = "Pêcher " + std::to_string(t.count) + " poissons";
        } else if (t.kind == "wood") {
            q.text = "Abattre " + std::to_string(t.count) + " arbres";
        }
        st.quests.push_back(q);
    }
}

// ------------------------------------------------------------
// Notification d'événement -> progression des quêtes
// ------------------------------------------------------------
void Sim::emit(const string& kind, const string& arg, int amount) {
    for (auto& q : st.quests) {
        if (q.done) continue;
        bool match = (q.kind == kind);
        if (match && kind == "harvest" && !q.arg.empty() && q.arg != arg) match = false;
        if (!match) continue;
        if (kind == "earn") {
            q.progress = st.earningsToday;
        } else {
            q.progress += amount;
        }
        if (q.progress >= q.target) {
            q.done = true;
            addMoney(q.rewardMoney, true);
            if (q.rewardSkill >= 0 && q.rewardXp > 0) addXp(q.rewardSkill, q.rewardXp);
            toast("Quête terminée : " + q.text + "  (+" + std::to_string(q.rewardMoney) + " pièces)", 1);
            events.push_back({"sfx", "quest", 0});
        }
    }
}

} // namespace fv
