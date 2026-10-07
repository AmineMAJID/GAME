#include "icons.h"
#include "draw.h"
#include <cmath>

namespace fe {

// Palette
static const Color C_WOOD = rgb(139, 90, 43), C_WOOD_D = rgb(92, 58, 30), C_WOOD_L = rgb(176, 123, 63);
static const Color C_GREEN = rgb(62, 155, 79), C_GREEN_D = rgb(44, 112, 56), C_GREEN_L = rgb(111, 207, 111);
static const Color C_GOLD = rgb(232, 193, 90), C_GOLD_D = rgb(184, 145, 47);
static const Color C_RED = rgb(214, 69, 69), C_RED_D = rgb(160, 40, 40);
static const Color C_BLUE = rgb(74, 144, 217), C_BLUE_L = rgb(127, 184, 232), C_BLUE_D = rgb(40, 90, 150);
static const Color C_WHITE = rgb(242, 242, 242), C_GRAY = rgb(154, 160, 166), C_DARK = rgb(58, 63, 68);
static const Color C_SKIN = rgb(240, 200, 160);
static const Color C_ORANGE = rgb(232, 134, 46), C_ORANGE_D = rgb(180, 90, 30);
static const Color C_CREAM = rgb(245, 240, 232);
static const Color C_BLACK = rgb(30, 30, 36);
static const Color C_PINK = rgb(240, 170, 180);
static const Color C_BROWN = rgb(120, 80, 50);

struct IconCtx {
    Framebuffer& fb;
    int ox, oy;
    void P(int x, int y, Color c) { px(fb, ox + x, oy + y, c); }
    void R(int x, int y, int w, int h, Color c) { rect(fb, ox + x, oy + y, w, h, c); }
    void H(int x0, int x1, int y, Color c) { hline(fb, ox + x0, ox + x1, oy + y, c); }
    void V(int x, int y0, int y1, Color c) { vline(fb, ox + x, oy + y0, oy + y1, c); }
    void circle(int cx, int cy, int r, Color c) { fe::circle(fb, ox + cx, oy + cy, r, c); }
    void circleOutline(int cx, int cy, int r, Color c) { fe::circleOutline(fb, ox + cx, oy + cy, r, c); }
};

static void drawTool(IconCtx& c, int id) {
    switch (id) {
        case 1: // Houe
            c.R(3, 2, 3, 9, C_WOOD);       // manche
            c.R(2, 1, 5, 3, C_GRAY);       // tête
            c.R(8, 9, 5, 2, C_WOOD);
            c.R(8, 11, 3, 3, C_WOOD);
            break;
        case 2: // Arrosoir
            c.R(3, 6, 8, 7, C_BLUE);       // réservoir
            c.R(3, 5, 8, 2, C_BLUE_D);
            c.R(11, 4, 3, 3, C_BLUE_D);    // bec
            c.R(1, 5, 2, 3, C_BLUE_D);     // anse
            c.P(14, 8, C_BLUE_L); c.P(14, 10, C_BLUE_L); c.P(13, 12, C_BLUE_L); // gouttes
            break;
        case 3: // Hache
            c.R(4, 2, 3, 11, C_WOOD);      // manche
            c.R(7, 2, 6, 5, C_GRAY);       // tête
            c.R(7, 2, 2, 5, C_WHITE);
            break;
        case 4: // Faux
            c.R(11, 2, 2, 12, C_WOOD);     // manche
            c.R(2, 10, 9, 2, C_GRAY);      // lame
            c.R(2, 10, 9, 1, C_WHITE);
            break;
        case 5: // Canne à pêche
            c.R(3, 1, 2, 12, C_WOOD_D);    // canne
            c.R(5, 2, 8, 1, C_DARK);       // ligne
            c.P(13, 3, C_DARK); c.P(13, 5, C_DARK); c.P(13, 7, C_DARK);
            c.R(12, 8, 3, 2, C_RED);        // bouchon
            break;
        case 6: // Seau
            c.R(3, 5, 10, 8, C_GRAY);      // seau
            c.R(3, 5, 10, 2, C_WHITE);
            c.R(2, 4, 12, 2, C_DARK);      // anse
            c.R(5, 8, 6, 3, C_BLUE_L);     // lait
            break;
        case 7: // Tondeuse
            c.R(2, 7, 9, 5, C_GRAY);       // corps
            c.R(2, 7, 9, 1, C_WHITE);
            c.R(11, 5, 3, 4, C_DARK);      // poignée
            c.R(4, 12, 2, 2, C_DARK); c.R(8, 12, 2, 2, C_DARK); // roues
            break;
    }
}

static void drawSeed(IconCtx& c, int id) {
    // sachet de graines + symbole de la culture
    c.R(3, 2, 10, 12, C_CREAM);
    c.R(3, 2, 10, 2, C_WOOD_L);
    c.R(3, 12, 10, 2, C_WOOD_L);
    c.R(5, 5, 6, 7, C_WOOD_L);
    Color inner = C_GOLD;
    switch (id) {
        case 10: inner = C_GOLD; break;                    // blé
        case 11: inner = rgb(222, 184, 135); break;        // patate
        case 12: inner = C_RED; break;                     // fraise
        case 13: inner = C_GOLD; break;                    // maïs
        case 14: inner = C_RED; break;                     // tomate
        case 15: inner = C_GOLD; break;                    // tournesol
        case 16: inner = C_ORANGE; break;                  // citrouille
        case 17: inner = C_GREEN; break;                   // kale
    }
    c.circle(8, 8, 2, inner);
    c.P(6, 6, C_DARK); c.P(10, 6, C_DARK);
}

static void drawCrop(IconCtx& c, int id) {
    switch (id) {
        case 20: // blé
            c.R(7, 3, 2, 10, C_GOLD);
            for (int i = 0; i < 4; i++) { c.R(5 - i, 4 + i, 2, 2, C_GOLD_D); c.R(9 + i, 4 + i, 2, 2, C_GOLD_D); }
            c.R(7, 13, 2, 2, C_GREEN_D);
            break;
        case 21: // patate
            c.circle(8, 9, 4, rgb(222, 184, 135));
            c.circle(7, 8, 1, rgb(240, 210, 170));
            c.P(6, 7, C_DARK); c.P(10, 11, C_DARK);
            c.R(7, 3, 2, 3, C_GREEN);
            break;
        case 22: // fraise
            c.R(7, 2, 2, 3, C_GREEN);          // feuilles
            c.circle(8, 10, 4, C_RED);
            c.P(6, 9, C_GOLD); c.P(10, 9, C_GOLD); c.P(8, 12, C_GOLD); // graines
            break;
        case 23: // maïs
            c.R(5, 6, 6, 7, C_GOLD);           // épi
            c.R(7, 3, 2, 4, C_GOLD_D);
            c.R(3, 7, 2, 5, C_GREEN); c.R(11, 7, 2, 5, C_GREEN); // feuilles
            break;
        case 24: // tomate
            c.circle(8, 10, 4, C_RED);
            c.circle(6, 8, 1, rgb(240, 120, 110));
            c.R(7, 2, 2, 4, C_GREEN);
            c.P(8, 4, C_GREEN_D);
            break;
        case 25: // tournesol
            for (int i = 0; i < 8; i++) {
                float a = i * 0.7854f;
                c.P(8 + (int)(5 * std::cos(a)), 8 + (int)(5 * std::sin(a)), C_GOLD);
            }
            c.circle(8, 8, 3, C_WOOD_D);
            c.P(7, 7, C_BLACK); c.P(9, 7, C_BLACK);
            break;
        case 26: // citrouille
            c.circle(8, 9, 5, C_ORANGE);
            c.circle(8, 9, 5, C_ORANGE);
            c.V(8, 4, 14, C_ORANGE_D);
            c.R(7, 2, 2, 3, C_GREEN_D);
            break;
        case 27: // kale
            c.R(6, 4, 4, 9, C_GREEN);
            c.R(3, 6, 3, 5, C_GREEN_D); c.R(10, 6, 3, 5, C_GREEN_D);
            c.V(8, 5, 12, C_GREEN_L);
            break;
    }
}

static void drawProduct(IconCtx& c, int id) {
    switch (id) {
        case 30: // œuf
            c.circle(8, 9, 4, C_CREAM);
            c.circle(7, 8, 2, C_WHITE);
            break;
        case 31: // lait
            c.R(5, 3, 6, 3, C_WHITE);          // bouchon
            c.R(4, 6, 8, 8, C_BLUE_L);         // pot
            c.R(4, 9, 8, 4, C_WHITE);          // étiquette
            c.R(5, 10, 6, 1, C_BLUE);
            break;
        case 32: // laine
            c.circle(6, 8, 4, C_CREAM);
            c.circle(10, 8, 4, C_CREAM);
            c.circle(8, 6, 4, C_CREAM);
            c.circle(8, 10, 4, C_CREAM);
            c.circle(8, 8, 3, C_WHITE);
            break;
    }
}

static void drawResource(IconCtx& c, int id) {
    switch (id) {
        case 40: // foin
            c.R(4, 10, 2, 4, C_GOLD);
            c.R(7, 8, 2, 6, C_GOLD);
            c.R(10, 10, 2, 4, C_GOLD);
            c.R(3, 12, 10, 2, C_GOLD_D);
            c.P(4, 9, C_GOLD_D); c.P(7, 7, C_GOLD_D); c.P(10, 9, C_GOLD_D);
            break;
        case 41: // bois
            c.R(3, 4, 4, 9, C_WOOD);           // bûche 1
            c.R(9, 6, 4, 7, C_WOOD);           // bûche 2
            c.R(3, 4, 4, 2, C_WOOD_L); c.R(9, 6, 4, 2, C_WOOD_L);
            c.circle(5, 8, 1, C_WOOD_D); c.circle(11, 9, 1, C_WOOD_D);
            break;
        case 42: // baies
            c.R(7, 2, 2, 4, C_GREEN_D);        // branche
            c.circle(5, 9, 3, C_RED);
            c.circle(11, 10, 3, C_RED);
            c.circle(8, 12, 3, C_RED_D);
            c.P(4, 8, C_WHITE); c.P(10, 9, C_WHITE);
            break;
        case 43: // engrais
            c.R(6, 2, 4, 3, C_WOOD_D);         // bouchon
            c.R(4, 5, 8, 9, C_WOOD_L);         // sac
            c.R(5, 8, 6, 4, C_WOOD);
            c.P(6, 9, C_GREEN); c.P(9, 10, C_GREEN); c.P(7, 11, C_GREEN);
            break;
    }
}

static void drawFish(IconCtx& c, int id) {
    Color body = C_GRAY;
    if (id == 51) body = rgb(120, 180, 120);
    if (id == 52) body = rgb(200, 170, 90);
    if (id == 53) body = rgb(90, 130, 90);
    c.R(2, 7, 8, 4, body);                 // corps
    c.R(10, 6, 4, 6, body);                // queue... triangle simplifié
    c.P(10, 6, body); c.P(11, 7, body); c.P(12, 8, body); c.P(11, 9, body); c.P(10, 10, body);
    c.P(4, 8, C_BLACK);                    // œil
    c.R(5, 6, 2, 1, body == C_GRAY ? C_WHITE : C_WHITE); // nageoire
}

static void drawAnimal(IconCtx& c, int id) {
    switch (id) {
        case 60: // poule
            c.circle(7, 9, 4, C_WHITE);        // corps
            c.circle(11, 6, 2, C_WHITE);       // tête
            c.R(9, 4, 2, 2, C_RED);            // crête
            c.P(13, 6, C_ORANGE);              // bec
            c.P(11, 6, C_BLACK);               // œil
            c.R(5, 13, 1, 2, C_ORANGE); c.R(8, 13, 1, 2, C_ORANGE);
            break;
        case 61: // vache
            c.R(2, 6, 12, 6, C_WHITE);         // corps
            c.R(3, 7, 3, 3, C_BLACK); c.R(9, 9, 3, 3, C_BLACK); // taches
            c.R(10, 3, 4, 4, C_WHITE);         // tête
            c.R(9, 2, 2, 2, C_GRAY); c.R(13, 2, 2, 2, C_GRAY);   // oreilles
            c.R(11, 5, 2, 1, C_PINK);          // museau
            c.P(11, 4, C_BLACK); c.P(13, 4, C_BLACK);
            c.R(4, 12, 2, 3, C_GRAY); c.R(10, 12, 2, 3, C_GRAY); // pattes
            break;
        case 62: // mouton
            c.circle(6, 8, 4, C_CREAM);
            c.circle(10, 8, 4, C_CREAM);
            c.circle(8, 6, 4, C_CREAM);
            c.circle(8, 10, 4, C_CREAM);
            c.circle(8, 8, 3, C_WHITE);
            c.R(11, 7, 3, 3, C_GRAY);          // tête
            c.P(12, 8, C_BLACK);
            c.R(5, 12, 2, 3, C_DARK); c.R(9, 12, 2, 3, C_DARK);
            break;
    }
}

static void drawUi(IconCtx& c, int id) {
    switch (id) {
        case IC_HEART:
            c.circle(5, 6, 3, C_RED); c.circle(10, 6, 3, C_RED);
            c.R(3, 7, 10, 6, C_RED); c.P(8, 12, C_RED); c.P(7, 11, C_RED); c.P(9, 11, C_RED);
            break;
        case IC_COIN:
            c.circle(8, 8, 6, C_GOLD);
            c.circle(8, 8, 4, C_GOLD_D);
            c.circle(8, 8, 2, C_GOLD);
            break;
        case IC_BOLT:
            c.P(9, 2, C_GOLD); c.P(7, 2, C_GOLD); c.P(8, 3, C_GOLD);
            c.R(6, 4, 3, 5, C_GOLD); c.P(5, 9, C_GOLD); c.P(6, 9, C_GOLD);
            c.R(4, 10, 3, 4, C_GOLD); c.P(4, 14, C_GOLD);
            break;
        case IC_CLOCK:
            c.circleOutline(8, 8, 6, C_DARK);
            c.R(7, 4, 2, 5, C_DARK); c.R(7, 8, 4, 2, C_DARK);
            break;
        case IC_SUN:
            c.circle(8, 8, 4, C_GOLD);
            for (int i = 0; i < 8; i++) {
                float a = i * 0.7854f;
                c.P(8 + (int)(7 * std::cos(a)), 8 + (int)(7 * std::sin(a)), C_GOLD);
            }
            break;
        case IC_CLOUD:
            c.circle(5, 9, 3, C_WHITE); c.circle(9, 7, 4, C_WHITE); c.circle(12, 10, 2, C_WHITE);
            c.R(3, 10, 11, 3, C_WHITE);
            break;
        case IC_RAIN:
            c.circle(5, 6, 3, C_GRAY); c.circle(9, 4, 4, C_GRAY); c.circle(12, 7, 2, C_GRAY);
            c.R(3, 7, 11, 3, C_GRAY);
            c.P(4, 11, C_BLUE_L); c.P(8, 12, C_BLUE_L); c.P(12, 11, C_BLUE_L);
            c.P(4, 14, C_BLUE); c.P(8, 15, C_BLUE); c.P(12, 14, C_BLUE);
            break;
        case IC_STORM:
            c.circle(5, 5, 3, C_DARK); c.circle(9, 4, 4, C_DARK); c.circle(12, 6, 2, C_DARK);
            c.R(3, 6, 11, 3, C_DARK);
            c.P(9, 9, C_GOLD); c.P(7, 10, C_GOLD); c.P(8, 11, C_GOLD);
            c.R(6, 12, 2, 3, C_GOLD); c.P(5, 15, C_GOLD);
            break;
        case IC_SNOW:
            c.circle(5, 6, 3, C_WHITE); c.circle(9, 4, 4, C_WHITE); c.circle(12, 7, 2, C_WHITE);
            c.R(3, 7, 11, 3, C_WHITE);
            c.P(4, 11, C_BLUE_L); c.P(8, 12, C_BLUE_L); c.P(12, 11, C_BLUE_L);
            c.P(6, 14, C_BLUE_L); c.P(10, 15, C_BLUE_L);
            break;
        case IC_STAR:
            for (int i = 0; i < 5; i++) {
                float a = -1.5708f + i * 1.2566f;
                float a2 = a + 0.6283f;
                c.P(8 + (int)(6 * std::cos(a)), 8 + (int)(6 * std::sin(a)), C_GOLD);
                c.P(8 + (int)(2.5f * std::cos(a2)), 8 + (int)(2.5f * std::sin(a2)), C_GOLD);
            }
            break;
        case IC_QUEST:
            c.R(3, 2, 10, 12, C_CREAM);
            c.R(3, 2, 10, 2, C_WOOD_L);
            c.R(5, 6, 2, 4, C_DARK); c.P(6, 11, C_DARK);
            c.R(9, 6, 3, 1, C_GRAY); c.R(9, 9, 3, 1, C_GRAY);
            break;
        case IC_SLEEP:
            drawText(c.fb, c.ox + 2, c.oy + 3, "zZ", FONT_SMALL, C_BLUE_D);
            break;
        case IC_ARROW_R:
            c.R(3, 7, 7, 2, C_DARK);
            c.P(10, 5, C_DARK); c.P(11, 6, C_DARK); c.P(12, 7, C_DARK);
            c.P(11, 8, C_DARK); c.P(10, 9, C_DARK);
            break;
        case IC_ARROW_D:
            c.R(7, 3, 2, 7, C_DARK);
            c.P(5, 10, C_DARK); c.P(6, 11, C_DARK); c.P(7, 12, C_DARK);
            c.P(8, 11, C_DARK); c.P(9, 10, C_DARK);
            break;
        case IC_BAG:
            c.R(3, 6, 10, 8, C_WOOD_L);
            c.R(3, 6, 10, 2, C_WOOD);
            c.R(5, 3, 6, 3, C_WOOD_D);
            c.R(6, 9, 4, 3, C_WOOD_D);
            break;
        case IC_CHEST:
            c.R(2, 6, 12, 8, C_WOOD);
            c.R(2, 6, 12, 2, C_WOOD_L);
            c.R(7, 8, 2, 3, C_GOLD);
            break;
        case IC_PAUSE:
            c.R(4, 3, 3, 10, C_DARK);
            c.R(9, 3, 3, 10, C_DARK);
            break;
        case IC_NOTE:
            c.circle(5, 11, 2, C_DARK);
            c.R(7, 3, 2, 9, C_DARK);
            c.R(7, 3, 5, 2, C_DARK);
            break;
        case IC_MUTE:
            c.R(3, 6, 4, 4, C_DARK);
            c.P(7, 5, C_DARK); c.P(8, 6, C_DARK); c.P(8, 10, C_DARK); c.P(7, 11, C_DARK);
            c.R(10, 4, 2, 2, C_RED); c.R(9, 6, 2, 2, C_RED); c.R(10, 8, 2, 2, C_RED); c.R(9, 10, 2, 2, C_RED);
            break;
        case IC_CHECK:
            c.R(3, 8, 3, 3, C_GREEN);
            c.R(6, 10, 2, 3, C_GREEN);
            c.R(8, 6, 2, 2, C_GREEN); c.R(10, 4, 2, 2, C_GREEN); c.R(12, 2, 2, 2, C_GREEN);
            break;
        case IC_CROSS:
            for (int i = 0; i < 3; i++) {
                c.P(4 + i, 4 + i, C_RED); c.P(5 + i, 4 + i, C_RED);
                c.P(11 - i, 4 + i, C_RED); c.P(10 - i, 4 + i, C_RED);
                c.P(4 + i, 11 - i, C_RED); c.P(5 + i, 11 - i, C_RED);
                c.P(11 - i, 11 - i, C_RED); c.P(10 - i, 11 - i, C_RED);
            }
            break;
        case IC_FISH:
            c.R(2, 7, 8, 4, C_BLUE);
            c.P(10, 6, C_BLUE); c.P(11, 7, C_BLUE); c.P(12, 8, C_BLUE); c.P(11, 9, C_BLUE); c.P(10, 10, C_BLUE);
            c.P(4, 8, C_WHITE);
            break;
        case IC_LEAF:
            c.circle(6, 6, 4, C_GREEN);
            c.circle(10, 10, 4, C_GREEN);
            c.R(4, 4, 8, 2, C_GREEN_L);
            c.V(8, 4, 12, C_GREEN_L);
            break;
        case IC_HOME:
            c.R(2, 8, 12, 6, C_WOOD);
            c.P(8, 2, C_RED_D); c.P(7, 3, C_RED_D); c.P(9, 3, C_RED_D);
            c.R(4, 4, 8, 4, C_RED_D);
            c.R(7, 10, 3, 4, C_WOOD_D);
            break;
    }
}

void drawIcon(Framebuffer& fb, int x, int y, int id, int q) {
    IconCtx c{fb, x, y};
    if (id >= 1 && id <= 7) drawTool(c, id);
    else if (id >= 10 && id <= 17) drawSeed(c, id);
    else if (id >= 20 && id <= 27) drawCrop(c, id);
    else if (id >= 30 && id <= 32) drawProduct(c, id);
    else if (id >= 40 && id <= 43) drawResource(c, id);
    else if (id >= 50 && id <= 53) drawFish(c, id);
    else if (id >= 60 && id <= 62) drawAnimal(c, id);
    else if (id >= 100) drawUi(c, id);
    // Liseré de qualité
    if (q == 1) rectOutline(fb, x - 1, y - 1, 18, 18, rgb(192, 200, 210));
    if (q == 2) rectOutline(fb, x - 1, y - 1, 18, 18, rgb(255, 215, 80));
}

} // namespace fe
