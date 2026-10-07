// ============================================================
// FarmVale — test headless de la simulation
// Simule 60+ jours de jeu et vérifie que toutes les mécaniques
// fonctionnent : cultures, animaux, reproduction, économie,
// quêtes, sauvegarde/chargement.
// ============================================================
#include "../src/core/sim.h"
#include <cstdio>
#include <cmath>

using namespace fv;

static int failures = 0;
static int checks = 0;

#define CHECK(cond, msg) do { \
    checks++; \
    if (!(cond)) { failures++; printf("  [ÉCHEC] %s (ligne %d)\n", msg, __LINE__); } \
    else { printf("  [ok] %s\n", msg); } \
} while (0)

// Déplace le joueur sur une tuile (pour respecter la portée d'interaction)
static void gotoTile(Sim& sim, int x, int y) {
    sim.st.px = x * TILE + TILE / 2.0f;
    sim.st.py = y * TILE + TILE / 2.0f;
}

int main() {
    printf("=== Test de simulation FarmVale ===\n");

    Sim sim;
    sim.newGame(42);

    // --- Jour 1 : on laboure, plante, arrose ---
    printf("\n-- Boucle de culture (printemps) --\n");
    gotoTile(sim, 12, 12);
    sim.sel = 0; // houe
    CHECK(sim.actionAt(12, 12), "labour de la tuile (12,12)");
    CHECK(sim.world.farm.g(12, 12) == T_SOIL, "la tuile est labourée");
    sim.sel = 2; // graines de blé
    CHECK(sim.actionAt(12, 12), "plantation de blé");
    const Plot* p = sim.plotAt(12, 12);
    CHECK(p && p->crop == "wheat", "la parcelle contient du blé");
    sim.sel = 1; // arrosoir
    CHECK(sim.actionAt(12, 12), "arrosage");
    p = sim.plotAt(12, 12);
    CHECK(p && p->watered, "la parcelle est arrosée");

    // On plante une rangée de 6 blés (le joueur se déplace sur chaque tuile)
    for (int x = 12; x <= 17; x++) {
        gotoTile(sim, x, 12);
        if (x > 12) {
            sim.sel = 0;
            sim.actionAt(x, 12); // labour
        }
        sim.sel = 2;
        sim.actionAt(x, 12); // plante
        sim.sel = 1;
        sim.actionAt(x, 12); // arrose
    }
    CHECK(sim.countItem("seed_wheat") == 10 - 6, "6 graines de blé consommées");
    CHECK(sim.st.plots.size() == 6, "6 parcelles plantées");

    // --- Avancer le temps jusqu'à maturité (blé = 4 jours) ---
    // 4 jours d'arrosage = 4.08 jours de pousse (à la 5e action, on récolterait)
    for (int d = 0; d < 4; d++) {
        for (int x = 12; x <= 17; x++) {
            gotoTile(sim, x, 12);
            sim.sel = 1;
            sim.actionAt(x, 12); // ré-arrose chaque jour
        }
        sim.advanceDay();
    }
    p = sim.plotAt(12, 12);
    CHECK(p && p->progress >= 4.0f, "le blé a poussé (progress >= 4 jours)");

    // --- Récolte ---
    int wheatBefore = sim.countItem("wheat");
    gotoTile(sim, 12, 12);
    CHECK(sim.harvestAt(12, 12), "récolte du blé mûr");
    CHECK(sim.countItem("wheat") > wheatBefore, "du blé est dans l'inventaire");
    CHECK(sim.plotAt(12, 12) == nullptr, "la parcelle est vide après récolte (pas de repousse)");
    CHECK(sim.st.statCropsHarvested >= 1, "stat récoltes incrémentée");
    CHECK(sim.st.skills[SK_FARMING] >= 10, "XP agriculture gagnée");

    // --- Culture qui ne pousse pas hors saison ---
    printf("\n-- Saisons --\n");
    while (sim.st.day < 29) sim.advanceDay();
    CHECK(sim.st.season() == SUMMER, "on est en été au jour 29");
    gotoTile(sim, 20, 10);
    sim.sel = 0;
    sim.actionAt(20, 10);
    sim.sel = 2; // blé = printemps uniquement
    CHECK(!sim.actionAt(20, 10), "impossible de planter du blé en été");

    // --- Animaux ---
    printf("\n-- Animaux et reproduction --\n");
    sim.st.money = 10000;
    CHECK(sim.buy("animal:chicken"), "achat d'une poule");
    CHECK(sim.buy("animal:chicken"), "achat d'une seconde poule");
    CHECK(sim.st.money == 10000 - 1000, "argent débité (2 poules)");
    CHECK(sim.st.animals.size() == 2, "2 animaux dans le pâturage");

    // Il faut du foin et un seau
    sim.addItem("hay", 0, 100);
    CHECK(sim.buy("tool_pail"), "achat d'un seau");
    CHECK(sim.buy("tool_shears"), "achat d'une tondeuse");

    // Nourrir + caresser les deux poules pendant plusieurs jours
    for (int d = 0; d < 12; d++) {
        for (size_t i = 0; i < sim.st.animals.size(); i++) {
            sim.feedAnimal((int)i, "hay");
            sim.petAnimal((int)i);
        }
        // récupération des œufs quand prêts
        for (size_t i = 0; i < sim.st.animals.size(); i++) {
            if (sim.st.animals[i].ready > 0) sim.collectAnimal((int)i, "tool_pail");
        }
        sim.updateAnimals(0.1f);
        sim.advanceDay();
    }
    int eggs = sim.countItem("egg") + sim.countItem("egg_large");
    CHECK(eggs > 0, "des œufs ont été récoltés");
    CHECK(sim.st.animals.size() > 2, "reproduction : la famille s'est agrandie");
    CHECK(sim.st.statAnimalsRaised > 0, "au moins un bébé est né (stat)");
    CHECK(sim.st.skills[SK_ANIMAL] > 0, "XP élevage gagnée");

    // --- Économie ---
    printf("\n-- Économie --\n");
    CHECK(eggs > 0, "œufs en stock");
    int slot = findItem(sim.st.backpack, "egg");
    vector<ItemStack>* bag = &sim.st.backpack;
    if (slot < 0) { slot = findItem(sim.st.hotbar, "egg"); bag = &sim.st.hotbar; }
    if (slot < 0) { slot = findItem(sim.st.backpack, "egg_large"); bag = &sim.st.backpack; }
    if (slot < 0) { slot = findItem(sim.st.hotbar, "egg_large"); bag = &sim.st.hotbar; }
    CHECK(slot >= 0, "œufs localisés dans un sac");
    int money0 = sim.st.money;
    int n = bag && slot >= 0 ? (*bag)[slot].n : 0;
    int got = (bag && slot >= 0) ? sim.sellStack(*bag, slot, n) : 0;
    CHECK(got >= n * 40, "vente d'œufs rapporte de l'argent");
    CHECK(sim.st.money > money0, "argent augmenté après vente");

    int shipped = sim.shipAll();
    CHECK(shipped >= 0, "expédition via la caisse fonctionne");

    // Achat d'outils
    CHECK(sim.buy("tool_axe"), "achat d'une hache");
    CHECK(sim.buy("tool_axe") == false, "impossible d'acheter la hache en double");

    // --- Quêtes ---
    printf("\n-- Quêtes --\n");
    CHECK(sim.st.quests.size() == 3, "3 quêtes journalières générées");
    sim.st.earningsToday = 100000; // pour la quête "gagner de l'argent"
    sim.emit("plant", "", 100);
    sim.emit("water", "", 100);
    sim.emit("feed", "", 100);
    // une quête de récolte peut cibler n'importe quelle culture
    for (const char* cid : {"wheat", "potato", "strawberry", "corn", "tomato", "sunflower", "pumpkin", "kale"})
        sim.emit("harvest", cid, 100);
    sim.emit("fish", "", 100);
    sim.emit("wood", "", 100);
    sim.emit("earn", "", 100);
    bool allDone = true;
    for (auto& q : sim.st.quests) if (!q.done) allDone = false;
    CHECK(allDone, "les quêtes réagissent aux événements");

    // --- Pêche ---
    printf("\n-- Pêche --\n");
    uint64_t fish0 = sim.st.statFishCaught;
    sim.catchFish("fish_roach", 0);
    CHECK(sim.st.statFishCaught == fish0 + 1, "stat pêche incrémentée");
    CHECK(sim.countItem("fish_roach") == 1, "poisson dans l'inventaire");

    // --- Sauvegarde / chargement ---
    printf("\n-- Sauvegarde --\n");
    sim.st.money = 12345;
    sim.st.day = 77;
    CHECK(sim.saveToFile("build/test_save.txt"), "sauvegarde écrite");
    Sim sim2;
    CHECK(sim2.loadFromFile("build/test_save.txt"), "sauvegarde relue");
    CHECK(sim2.st.money == 12345, "argent restauré");
    CHECK(sim2.st.day == 77, "jour restauré");
    CHECK(sim2.st.animals.size() == sim.st.animals.size(), "animaux restaurés");
    CHECK(sim2.st.plots.size() == sim.st.plots.size(), "parcelles restaurées");
    CHECK(sim2.st.nodes.size() == sim.st.nodes.size(), "nature restaurée");
    CHECK(sim2.st.quests.size() == sim.st.quests.size(), "quêtes restaurées");
    CHECK(sim2.countItem("fish_roach") == sim.countItem("fish_roach"), "inventaire restauré");

    // Le monde rechargé ne doit pas régénérer la nature (arbres coupés = souches)
    printf("\n-- Persistance du monde --\n");
    Sim sim3;
    sim3.newGame(42);
    int treesBefore = 0;
    for (auto& kv : sim3.st.nodes) if (kv.second.kind == "tree") treesBefore++;
    CHECK(treesBefore > 10, "la forêt est peuplée d'arbres");
    sim3.st.energy = 100000;
    sim3.addItem("tool_axe", 0, 1);
    int axeSlot = findItem(sim3.st.hotbar, "tool_axe");
    for (auto& kv : sim3.st.nodes) {
        if (kv.second.kind == "tree") {
            int x = (int)(kv.first & 0xFFFFFFFF), y = (int)(kv.first >> 32);
            gotoTile(sim3, x, y);
            sim3.sel = axeSlot;
            for (int c = 0; c < 3; c++) sim3.actionAt(x, y);
        }
    }
    int stumps = 0;
    for (auto& kv : sim3.st.nodes) if (kv.second.kind == "stump") stumps++;
    CHECK(stumps == treesBefore, "tous les arbres abattus deviennent des souches");
    CHECK(sim3.saveToFile("build/test_save2.txt"), "sauvegarde 2 écrite");
    Sim sim4;
    CHECK(sim4.loadFromFile("build/test_save2.txt"), "sauvegarde 2 relue");
    int stumpsAfter = 0, treesAfter = 0;
    for (auto& kv : sim4.st.nodes) {
        if (kv.second.kind == "stump") stumpsAfter++;
        if (kv.second.kind == "tree") treesAfter++;
    }
    CHECK(stumpsAfter == stumps, "les souches persistent après rechargement");
    CHECK(treesAfter == 0, "les arbres coupés ne repoussent pas au chargement");

    // --- Longue simulation : 120 jours, le jeu doit rester stable ---
    printf("\n-- Stabilité (120 jours) --\n");
    Sim sim5;
    sim5.newGame(7);
    sim5.st.money = 50000;
    sim5.addItem("hay", 0, 500);
    sim5.buy("tool_pail");
    sim5.buy("tool_shears");
    for (int d = 0; d < 120; d++) {
        if (d == 5) sim5.buy("animal:cow");
        if (d == 6) sim5.buy("animal:sheep");
        if (d == 7) sim5.buy("animal:cow");
        gotoTile(sim5, 15, 15);
        // laboure / plante / arrose / récolte en continu sur une parcelle
        sim5.sel = 0;
        sim5.actionAt(15, 15);
        if (sim5.selected().id == "tool_hoe" || sim5.selected().empty()) {
            int seedSlot = findItem(sim5.st.hotbar, "seed_wheat");
            if (seedSlot >= 0) { sim5.sel = seedSlot; sim5.actionAt(15, 15); }
        }
        int canSlot = findItem(sim5.st.hotbar, "tool_can");
        if (canSlot >= 0) { sim5.sel = canSlot; sim5.actionAt(15, 15); }
        Plot* pp = sim5.plotAt(15, 15);
        if (pp && !pp->crop.empty() && !pp->dead) {
            const CropDef* c = cropDef(pp->crop);
            if (c && pp->progress >= c->days) sim5.harvestAt(15, 15);
        }
        // nourrit les animaux
        for (size_t i = 0; i < sim5.st.animals.size(); i++) {
            sim5.feedAnimal((int)i, "hay");
            sim5.petAnimal((int)i);
            if (sim5.st.animals[i].ready > 0)
                sim5.collectAnimal((int)i, sim5.st.animals[i].type == "sheep" ? "tool_shears" : "tool_pail");
        }
        if (d % 10 == 9) sim5.shipAll();
        sim5.updateAnimals(0.05f);
        sim5.advanceDay();
        if (sim5.st.energy < 5) sim5.st.energy = sim5.st.energyMaxBase();
    }
    CHECK(sim5.st.day == 121, "120 jours simulés sans crash");
    CHECK(sim5.st.statDaysPlayed == 120, "stat jours OK");
    CHECK((int)sim5.st.animals.size() <= MAX_ANIMALS, "population animale plafonnée");
    CHECK(sim5.st.money > 0, "économie positive");
    CHECK(sim5.st.statCropsHarvested > 0, "des récoltes ont eu lieu");
    CHECK(sim5.st.statAnimalsRaised > 0, "des naissances ont eu lieu");
    printf("  -> après 120 jours : %d animaux, %lld pièces, %llu récoltes, %llu naissances, %llu poissons\n",
           (int)sim5.st.animals.size(), (long long)sim5.st.money,
           (unsigned long long)sim5.st.statCropsHarvested,
           (unsigned long long)sim5.st.statAnimalsRaised,
           (unsigned long long)sim5.st.statFishCaught);

    printf("\n=== Résultat : %d vérifications, %d échec(s) ===\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
