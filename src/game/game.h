#pragma once
// ============================================================
// FarmVale — couche jeu : boucle, écrans, entrées, rendu, UI
// ============================================================
#include "../core/sim.h"
#include "../engine/engine.h"
#include "../engine/audio.h"
#include "../engine/draw.h"
#include "../engine/icons.h"
#include <string>
#include <vector>

namespace fv {

using fe::Framebuffer;
using fe::IPlatform;
using fe::Color;
using std::string;
using std::vector;

enum class Screen { TITLE, PLAYING, PAUSED };
enum class Menu { NONE, SHOP, INVENTORY, QUESTS, CHEST, HELP, SLEEP_CONFIRM, ANIMAL_INFO };

class Game {
public:
    void init(IPlatform* platform, bool botMode, int botDays, const string& shotPrefix);
    void run();

    // Capture headless de l'écran titre (une fois, puis quitte)
    bool captureTitleShot = false;
    bool titleShotDone = false;
    long frameCount = 0;

private:
    IPlatform* plat = nullptr;
    Framebuffer fb;
    fe::Audio audio;
    Sim sim;
    bool hasSave = false;
    bool botMode = false;
    int botDays = 8;
    string shotPrefix;
    int shotsTaken = 0;

    Screen screen = Screen::TITLE;
    Menu menu = Menu::NONE;

    // Caméra (pixels)
    int camX = 0, camY = 0;

    // Particules météo
    struct Particle { float x, y, vx, vy; int len; };
    vector<Particle> rain, snow;
    float lightning = 0;      // flash d'orage (0..1)
    float lightningCooldown = 0;

    // Cœurs au-dessus des animaux
    struct Heart { float x, y, ttl; };
    vector<Heart> hearts;

    // Mini-jeu de pêche
    bool fishing = false;
    float fishWait = 0;       // attente de la touche
    float fishBar = 50, fishTarget = 50, fishProgress = 0, fishSpeed = 0;
    string fishCatchId;
    int fishQuality = 0;

    // Animation joueur
    float walkPhase = 0;
    float actionAnim = 0;     // animation d'action (coup d'outil)

    // Inventaire : objet tenu à la souris
    bool holdingItem = false;
    ItemStack heldStack;
    vector<ItemStack>* heldFrom = nullptr;
    int heldIndex = -1;

    // Bot (mode headless)
    float botTimer = 0;
    int lastShotDay = -1;

    // --- Boucle ---
    void update(float dt);
    void render();
    void updatePlaying(float dt);
    void updateTitle(float dt);
    void updatePlayingCameraOnly(float dt);
    void handleInputPlaying();
    void processEvents();
    void playEventSfx(const Event& e);

    // Écran titre (simulation séparée pour le décor animé)
    Sim titleSim;
    bool titleSimReady = false;
    string pendingShot;

    // --- Rendu monde ---
    void renderWorld();
    void renderGround();
    void renderPlot(int tx, int ty, const Plot& p, int sx, int sy);
    void renderCropSprite(int sx, int sy, const string& crop, float frac, bool dead, bool ready);
    void renderTree(int sx, int sy, const Node& n, int tx, int ty);
    void renderBush(int sx, int sy, const Node& n);
    void renderAnimal(const Animal& a, int sxBase, int syBase);
    void renderPlayer(int sx, int sy);
    void renderStructures();
    void renderHouse(int sx, int sy);
    void renderStall(int sx, int sy);
    void renderBoard(int sx, int sy);
    void renderBin(int sx, int sy);
    void renderFence(int sx, int sy);
    void renderBed(int sx, int sy);
    void renderChest(int sx, int sy);
    void renderWeather();
    void renderLighting();
    void renderHearts();
    int camClampX() const;
    int camClampY() const;

    // --- Rendu UI ---
    void renderHud();
    void renderHotbar();
    void renderToasts();
    void renderFishingGame();
    void renderMenus();
    void renderTitle();
    void renderPauseMenu();
    void renderShop();
    void renderInventory();
    void renderQuests();
    void renderChest();
    void renderHelp();
    void renderSleepConfirm();
    void renderAnimalInfo();

    // --- Menus : actions ---
    void openMenu(Menu m);
    void closeMenu();
    void handleSlotClick(vector<ItemStack>& bag, int idx);
    void renderHeldItem();
    void doInteractTile(int tx, int ty);
    void doInteractFront();
    void tryEat();
    void saveGame();
    void loadGame();
    bool saveExists();
    string savePath();

    // --- Divers rendu ---
    void drawGlow(int cx, int cy, int radius, Color c, int alpha);
    int tileHash(int x, int y) const;

    // --- Bot (screenshots headless) ---
    void botUpdate(float dt);
    void takeScreenshot(const string& name);

    // --- Utilitaires UI ---
    void drawItemStack(int x, int y, const ItemStack& s, bool selected);
    int foodValue(const string& id) const;
    void pickFish();
};

} // namespace fv
