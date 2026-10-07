#include "sim.h"
#include <cmath>

namespace fv {

// Limites du pâturage en pixels
static constexpr float PASTURE_X0 = 9 * TILE + 10;
static constexpr float PASTURE_X1 = 26 * TILE - 10;
static constexpr float PASTURE_Y0 = 21 * TILE + 10;
static constexpr float PASTURE_Y1 = 27 * TILE - 10;

// ------------------------------------------------------------
// IA des animaux (errance dans le pâturage)
// ------------------------------------------------------------
void Sim::updateAnimals(float dt) {
    bool night = st.isNight();
    for (auto& a : st.animals) {
        a.bob += dt * 7.0f;
        a.sleeping = night;
        if (night) continue;
        a.moveTimer -= dt;
        if (a.moveTimer <= 0) {
            a.dir = rng.uf() * 6.2831f;
            a.moveTimer = 1.0f + rng.uf() * 2.5f;
        }
        float speed = a.isAdult() ? 15.0f : 24.0f;
        float nx = a.x + std::cos(a.dir) * speed * dt;
        float ny = a.y + std::sin(a.dir) * speed * dt;
        // Reste dans le pâturage
        if (nx < PASTURE_X0 || nx > PASTURE_X1) { a.dir = 3.14159f - a.dir; nx = a.x; }
        if (ny < PASTURE_Y0 || ny > PASTURE_Y1) { a.dir = -a.dir; ny = a.y; }
        a.x = std::max(PASTURE_X0, std::min(PASTURE_X1, nx));
        a.y = std::max(PASTURE_Y0, std::min(PASTURE_Y1, ny));
    }
}

// ------------------------------------------------------------
// Cycle journalier des animaux
// ------------------------------------------------------------
void Sim::dailyAnimals() {
    for (auto& a : st.animals) {
        bool fed = a.fedToday;
        bool petted = a.pettedToday;
        a.wasFed = fed;
        a.wasPetted = petted;
        a.fedToday = false;
        a.pettedToday = false;
        a.age++;

        if (!fed) a.happiness -= 25;
        else a.happiness += 5;
        if (!petted) a.happiness -= 5;
        if (a.happiness < 0) a.happiness = 0;
        if (a.happiness > 100) a.happiness = 100;

        const AnimalDef* d = animalDef(a.type);
        if (!d) continue;

        // Gestation
        if (a.pregnant) {
            a.gestation--;
            if (a.gestation <= 0) {
                a.pregnant = false;
                a.lastBredDay = st.day;
                Animal baby;
                baby.id = st.nextAnimalId++;
                baby.type = a.type;
                baby.x = a.x + 14;
                baby.y = a.y + 10;
                if (baby.x > PASTURE_X1) baby.x = a.x - 14;
                baby.age = 0;
                baby.friendship = 20;
                baby.happiness = 100;
                baby.dir = rng.uf() * 6.28f;
                baby.moveTimer = 1.0f;
                st.animals.push_back(baby);
                st.statAnimalsRaised++;
                events.push_back({"hearts", std::to_string(a.id), 0});
                events.push_back({"sfx", "birth", 0});
                events.push_back({"ach", AchievementIds::FIRST_BREED, 0});
                toast("Un bébé " + d->name + " est né !", 1);
            }
            continue;
        }

        // Production
        if (a.isAdult() && fed && a.happiness >= 30) {
            a.produceAcc++;
            if (a.produceAcc >= d->produceEvery) {
                a.produceAcc = 0;
                if (a.ready < MAX_READY_PRODUCTS) a.ready++;
            }
        }
    }
}

// ------------------------------------------------------------
// Reproduction : deux adultes de même espèce, amitié élevée
// ------------------------------------------------------------
void Sim::checkBreeding() {
    if ((int)st.animals.size() >= MAX_ANIMALS) return;
    for (size_t i = 0; i < st.animals.size(); i++) {
        for (size_t j = i + 1; j < st.animals.size(); j++) {
            Animal& A = st.animals[i];
            Animal& B = st.animals[j];
            if (A.type != B.type) continue;
            const AnimalDef* d = animalDef(A.type);
            if (!d) continue;
            if (!A.isAdult() || !B.isAdult()) continue;
            if (A.pregnant || B.pregnant) continue;
            if (!A.wasFed || !B.wasFed) continue;
            if (A.friendship < BREED_MIN_FRIENDSHIP || B.friendship < BREED_MIN_FRIENDSHIP) continue;
            if (st.day - A.lastBredDay < d->breedCooldown) continue;
            if (st.day - B.lastBredDay < d->breedCooldown) continue;
            if (!rng.chance(0.7f)) continue;
            B.pregnant = true;
            B.gestation = d->gestation;
            A.lastBredDay = st.day;
            B.lastBredDay = st.day;
            events.push_back({"hearts", std::to_string(A.id), 0});
            events.push_back({"hearts", std::to_string(B.id), 0});
            events.push_back({"sfx", "breed", 0});
            toast("Deux " + d->name + "s se sont rapprochés… ♡", 1);
            return; // un couple par jour suffit
        }
    }
}

} // namespace fv
