#include "sim.h"
#include <fstream>
#include <sstream>

namespace fv {

// ============================================================
// Sérialisation texte (format lisible, versionné, robuste)
// ============================================================

static const char* SAVE_MAGIC = "FARMVALE-SAVE";
static const int SAVE_VERSION = 1;

static void writeBag(std::ofstream& f, const char* name, const vector<ItemStack>& bag) {
    f << name << " " << bag.size() << "\n";
    for (auto& s : bag) {
        if (s.empty()) f << "-\n";
        else f << s.id << " " << s.q << " " << s.n << "\n";
    }
}

static void readBag(std::ifstream& f, vector<ItemStack>& bag) {
    string name;
    int count = 0;
    f >> name >> count;
    bag.assign(count > 0 ? count : 0, ItemStack());
    string line;
    std::getline(f, line); // fin de ligne
    for (int i = 0; i < count; i++) {
        std::getline(f, line);
        if (line == "-" || line.empty()) continue;
        std::istringstream ss(line);
        string id; int q, n;
        ss >> id >> q >> n;
        bag[i] = ItemStack{id, q, n};
    }
}

bool Sim::saveToFile(const string& path) const {
    std::ofstream f(path);
    if (!f) return false;
    f << SAVE_MAGIC << " " << SAVE_VERSION << "\n";
    f << "seed " << st.seed << "\n";
    f << "day " << st.day << "\n";
    f << "minutes " << st.minutes << "\n";
    f << "weather " << st.weather << "\n";
    f << "money " << st.money << "\n";
    f << "energy " << st.energy << "\n";
    f << "map " << st.mapId << "\n";
    f << "pos " << st.px << " " << st.py << " " << st.facing << "\n";
    f << "sel " << sel << "\n";
    f << "canLevel " << st.canLevel << "\n";
    f << "energyUp " << st.energyUp << "\n";
    f << "earningsToday " << st.earningsToday << "\n";
    f << "nextAnimalId " << st.nextAnimalId << "\n";
    f << "animalCounter " << st.animalCounter << "\n";
    f << "skills";
    for (int i = 0; i < SK_COUNT; i++) f << " " << st.skills[i];
    f << "\n";
    f << "stats " << st.statMoneyEarned << " " << st.statCropsHarvested << " " << st.statAnimalsRaised
      << " " << st.statFishCaught << " " << st.statDaysPlayed << " " << (int)st.reachedWinter << "\n";

    writeBag(f, "hotbar", st.hotbar);
    writeBag(f, "backpack", st.backpack);
    writeBag(f, "storage", st.storage);

    f << "plots " << st.plots.size() << "\n";
    for (auto& kv : st.plots) {
        int x = (int)(kv.first & 0xFFFFFFFF), y = (int)(kv.first >> 32);
        const Plot& p = kv.second;
        f << "plot " << x << " " << y << " " << p.crop << " " << p.plantedDay << " "
          << p.progress << " " << (int)p.watered << " " << (int)p.fert << " " << (int)p.dead << "\n";
    }

    f << "animals " << st.animals.size() << "\n";
    for (auto& a : st.animals) {
        f << "animal " << a.id << " " << a.type << " " << a.x << " " << a.y << " "
          << a.age << " " << a.friendship << " " << a.happiness << " "
          << (int)a.fedToday << " " << (int)a.pettedToday << " "
          << (int)a.pregnant << " " << a.gestation << " " << a.lastBredDay << " "
          << a.ready << " " << a.produceAcc << " " << (int)a.wasFed << " " << (int)a.wasPetted << "\n";
    }

    f << "nodes " << st.nodes.size() << "\n";
    for (auto& kv : st.nodes) {
        int x = (int)(kv.first & 0xFFFFFFFF), y = (int)(kv.first >> 32);
        const Node& n = kv.second;
        f << "node " << x << " " << y << " " << n.kind << " " << n.a << " " << n.b << "\n";
    }

    f << "quests " << st.quests.size() << "\n";
    for (auto& q : st.quests) {
        f << "quest " << q.id << "|" << q.text << "|" << q.kind << "|" << q.arg << " "
          << q.target << " " << q.progress << " " << q.rewardMoney << " "
          << q.rewardSkill << " " << q.rewardXp << " " << (int)q.done << "\n";
    }
    return (bool)f;
}

bool Sim::loadFromFile(const string& path) {
    std::ifstream f(path);
    if (!f) return false;
    string magic, key;
    int version = 0;
    f >> magic >> version;
    if (magic != SAVE_MAGIC || version != SAVE_VERSION) return false;

    GameState ns;
    f >> key >> ns.seed;
    f >> key >> ns.day;
    f >> key >> ns.minutes;
    f >> key >> ns.weather;
    f >> key >> ns.money;
    f >> key >> ns.energy;
    f >> key >> ns.mapId;
    f >> key >> ns.px >> ns.py >> ns.facing;
    f >> key >> sel;
    f >> key >> ns.canLevel;
    f >> key >> ns.energyUp;
    f >> key >> ns.earningsToday;
    f >> key >> ns.nextAnimalId;
    f >> key >> ns.animalCounter;
    f >> key;
    for (int i = 0; i < SK_COUNT; i++) f >> ns.skills[i];
    int winter = 0;
    f >> key >> ns.statMoneyEarned >> ns.statCropsHarvested >> ns.statAnimalsRaised
      >> ns.statFishCaught >> ns.statDaysPlayed >> winter;
    ns.reachedWinter = winter != 0;
    ns.energyMax = ns.energyMaxBase();

    readBag(f, ns.hotbar);
    readBag(f, ns.backpack);
    readBag(f, ns.storage);

    int count = 0;
    f >> key >> count;
    for (int i = 0; i < count; i++) {
        int x, y, watered, fert, dead;
        string crop;
        Plot p;
        f >> key >> x >> y >> crop >> p.plantedDay >> p.progress >> watered >> fert >> dead;
        p.crop = crop;
        p.watered = watered != 0;
        p.fert = fert != 0;
        p.dead = dead != 0;
        ns.plots[tileKey(x, y)] = p;
    }

    f >> key >> count;
    ns.animals.resize(count);
    for (int i = 0; i < count; i++) {
        Animal& a = ns.animals[i];
        int fed, petted, pregnant, wasFed, wasPetted;
        f >> key >> a.id >> a.type >> a.x >> a.y >> a.age >> a.friendship >> a.happiness
          >> fed >> petted >> pregnant >> a.gestation >> a.lastBredDay >> a.ready >> a.produceAcc
          >> wasFed >> wasPetted;
        a.fedToday = fed != 0;
        a.pettedToday = petted != 0;
        a.pregnant = pregnant != 0;
        a.wasFed = wasFed != 0;
        a.wasPetted = wasPetted != 0;
    }

    f >> key >> count;
    for (int i = 0; i < count; i++) {
        int x, y;
        Node n;
        f >> key >> x >> y >> n.kind >> n.a >> n.b;
        ns.nodes[tileKey(x, y)] = n;
    }

    f >> key >> count;
    ns.quests.resize(count);
    string line;
    std::getline(f, line);
    for (int i = 0; i < count; i++) {
        std::getline(f, line);
        Quest& q = ns.quests[i];
        // format: quest id|texte|kind|arg target progress rewardMoney rewardSkill rewardXp done
        std::istringstream ss(line);
        string tok;
        ss >> tok; // "quest"
        std::getline(ss, q.id, '|');
        std::getline(ss, q.text, '|');
        std::getline(ss, q.kind, '|');
        std::getline(ss, q.arg, ' ');
        // retire l'espace initiale de arg si présente
        if (!q.arg.empty() && q.arg[0] == ' ') q.arg = q.arg.substr(1);
        int done = 0;
        ss >> q.target >> q.progress >> q.rewardMoney >> q.rewardSkill >> q.rewardXp >> done;
        q.done = done != 0;
    }

    if (!f) return false;

    // On remplace l'état et on reconstruit le monde SANS re-peupler la nature
    st = ns;
    rng = Rng(st.seed * 2654435761u + 12345);
    world.build(st, false);
    return true;
}

} // namespace fv
