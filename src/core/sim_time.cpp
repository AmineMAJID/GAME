#include "sim.h"

namespace fv {

// ------------------------------------------------------------
// Avancement du temps
// ------------------------------------------------------------
void Sim::advanceTime(float dtSeconds) {
    if (timePaused) return;
    float gameMinutes = dtSeconds * (60.0f / REAL_SECONDS_PER_GAME_HOUR) * gameSpeed;
    st.minutes += (int)gameMinutes;
    // sous-minutes accumulées en float serait mieux, mais un pas entier suffit
    if (st.minutes >= DAY_END_MIN) {
        passOut();
    }
}

void Sim::passOut() {
    toast("Vous vous êtes évanoui d'épuisement…", 2);
    int penalty = st.money / 10;
    if (penalty > 1000) penalty = 1000;
    st.money -= penalty;
    advanceDay();
    // On se réveille dans la maison
    st.mapId = "home";
    st.px = (float)(World::HOME_DOOR_TX * TILE + TILE / 2);
    st.py = (float)((World::HOME_DOOR_TY - 1) * TILE + TILE / 2);
    st.energy = st.energyMaxBase() * 0.6f;
    events.push_back({"sfx", "wakeup", 0});
}

void Sim::sleepNow() {
    advanceDay();
    st.mapId = "home";
    st.px = (float)(World::HOME_DOOR_TX * TILE + TILE / 2);
    st.py = (float)((World::HOME_DOOR_TY - 1) * TILE + TILE / 2);
    st.energy = (float)st.energyMaxBase();
    events.push_back({"sfx", "sleep", 0});
    events.push_back({"autosave", "", 0});
}

void Sim::rollWeather() {
    Rng r(st.seed ^ ((uint64_t)st.day * 0x9E3779B97F4A7C15ull));
    int s = st.season();
    float x = r.uf();
    if (s == WINTER) {
        st.weather = x < 0.45f ? W_SNOW : (x < 0.75f ? W_SUN : W_CLOUD);
    } else {
        if (x < 0.55f) st.weather = W_SUN;
        else if (x < 0.70f) st.weather = W_CLOUD;
        else if (x < 0.90f) st.weather = W_RAIN;
        else st.weather = W_STORM;
    }
}

// ------------------------------------------------------------
// Changement de jour : le cœur du cycle de la ferme
// ------------------------------------------------------------
void Sim::advanceDay() {
    st.day++;
    st.minutes = DAY_START_MIN;
    st.statDaysPlayed++;
    rollWeather();
    if (st.season() == WINTER && !st.reachedWinter) {
        st.reachedWinter = true;
        events.push_back({"ach", AchievementIds::SEASONS, 0});
        toast("L'hiver est arrivé… les cultures ne pousseront plus.", 2);
    }

    bool rain = isRaining();
    int farmLv = st.skillLevel(SK_FARMING);
    float growthMult = 1.0f + farmLv * 0.02f;

    // --- Cultures ---
    for (auto& kv : st.plots) {
        Plot& p = kv.second;
        if (p.dead || p.crop.empty()) continue;
        const CropDef* c = cropDef(p.crop);
        if (!c) { p.dead = true; continue; }
        bool watered = p.watered || rain;
        if (st.season() == WINTER) {
            // Le gel tue les cultures
            p.dead = true;
            p.progress = 0;
            continue;
        }
        if (!watered) {
            // 25% de chance de sécheresse
            if (rng.chance(0.25f)) {
                p.dead = true;
            }
        } else {
            p.progress += (1.0f + (p.fert ? 0.33f : 0.0f)) * growthMult;
        }
        p.watered = false; // à ré-arroser aujourd'hui
    }

    // --- Nature : repousse ---
    for (auto& kv : st.nodes) {
        Node& n = kv.second;
        if (n.kind == "stump" && st.day >= n.a) {
            int x = (int)(kv.first & 0xFFFFFFFF), y = (int)(kv.first >> 32);
            n = Node{"tree", 3, 3};
            (void)x; (void)y;
        } else if (n.kind == "bush" && n.a == 0 && st.day >= n.b) {
            n.a = 1; // les baies repoussent
        }
    }

    // --- Animaux ---
    dailyAnimals();
    checkBreeding();

    // --- Quêtes du jour ---
    st.earningsToday = 0;
    generateDailyQuests();

    events.push_back({"newday", std::to_string(st.day), 0});
}

} // namespace fv
