#pragma once
// ============================================================
// FarmVale — widgets UI en mode immédiat (partagés par les menus)
// ============================================================
#include "../core/sim.h"
#include "../engine/engine.h"
#include "../engine/draw.h"
#include "../engine/icons.h"
#include "../engine/audio.h"

namespace fv {

using fe::Framebuffer;
using fe::InputState;
using fe::Color;
using fe::rgb;
using fe::IC_HEART; using fe::IC_COIN; using fe::IC_BOLT; using fe::IC_CLOCK;
using fe::IC_SUN; using fe::IC_CLOUD; using fe::IC_RAIN; using fe::IC_STORM; using fe::IC_SNOW;
using fe::IC_STAR; using fe::IC_QUEST; using fe::IC_SLEEP; using fe::IC_ARROW_R; using fe::IC_ARROW_D;
using fe::IC_BAG; using fe::IC_CHEST; using fe::IC_PAUSE; using fe::IC_NOTE; using fe::IC_MUTE;
using fe::IC_CHECK; using fe::IC_CROSS; using fe::IC_FISH; using fe::IC_LEAF; using fe::IC_HOME;

// Couleurs UI
static const Color UI_PANEL   = rgb(42, 37, 48);
static const Color UI_PANEL_L = rgb(58, 52, 66);
static const Color UI_BORDER  = rgb(90, 82, 104);
static const Color UI_ACCENT  = rgb(232, 193, 90);
static const Color UI_TEXT    = rgb(240, 236, 228);
static const Color UI_DIM     = rgb(160, 152, 168);
static const Color UI_GOOD    = rgb(111, 207, 111);
static const Color UI_WARN    = rgb(232, 160, 69);
static const Color UI_BAD     = rgb(214, 69, 69);
static const Color UI_SLOT    = rgb(28, 25, 32);
static const Color UI_SLOT_HL = rgb(70, 62, 84);

struct Ui {
    Framebuffer& fb;
    const InputState& in;
    fe::Audio* audio = nullptr;

    Ui(Framebuffer& f, const InputState& i, fe::Audio* a) : fb(f), in(i), audio(a) {}

    bool hover(int x, int y, int w, int h) const {
        return in.mouseX >= x && in.mouseX < x + w && in.mouseY >= y && in.mouseY < y + h;
    }
    bool clicked(int x, int y, int w, int h) const {
        return in.mousePressed && hover(x, y, w, h);
    }

    void panel(int x, int y, int w, int h, const char* title = nullptr) {
        fe::rect(fb, x, y, w, h, UI_PANEL);
        fe::rect(fb, x, y, w, 2, UI_PANEL_L);
        fe::rectOutline(fb, x, y, w, h, UI_BORDER);
        fe::rectOutline(fb, x + 1, y + 1, w - 2, h - 2, rgb(20, 18, 24));
        if (title) {
            fe::rect(fb, x + 8, y - 8, fe::textWidth(title, fe::FONT_SMALL) + 16, 18, UI_PANEL);
            fe::rectOutline(fb, x + 8, y - 8, fe::textWidth(title, fe::FONT_SMALL) + 16, 18, UI_BORDER);
            fe::drawText(fb, x + 16, y - 4, title, fe::FONT_SMALL, UI_ACCENT);
        }
    }

    bool button(int x, int y, int w, int h, const std::string& label, bool enabled = true) {
        bool hov = enabled && hover(x, y, w, h);
        fe::rect(fb, x, y, w, h, hov ? UI_SLOT_HL : UI_SLOT);
        fe::rectOutline(fb, x, y, w, h, hov ? UI_ACCENT : UI_BORDER);
        Color tc = enabled ? (hov ? UI_ACCENT : UI_TEXT) : UI_DIM;
        fe::drawTextCentered(fb, x + w / 2, y + (h - fe::FONT_SMALL.height) / 2 + 1, label, fe::FONT_SMALL, tc);
        if (hov && in.mousePressed && audio) audio->sfx(fe::Sfx::Button);
        return clicked(x, y, w, h) && enabled;
    }

    // Case d'inventaire : dessine l'objet, retourne true si cliquée
    bool itemSlot(int x, int y, int size, const ItemStack& s, bool selected = false) {
        fe::rect(fb, x, y, size, size, UI_SLOT);
        fe::rectOutline(fb, x, y, size, size, selected ? UI_ACCENT : UI_BORDER);
        if (!s.empty()) {
            const ItemDef* def = itemDef(s.id);
            int icon = def ? def->icon : 0;
            int pad = (size - 16) / 2;
            fe::drawIcon(fb, x + pad, y + pad, icon, s.q);
            if (s.n > 1) {
                string n = std::to_string(s.n);
                int w = fe::textWidth(n, fe::FONT_SMALL);
                fe::drawTextShadow(fb, x + size - w - 2, y + size - fe::FONT_SMALL.height - 1, n, fe::FONT_SMALL, UI_TEXT);
            }
        }
        if (hover(x, y, size, size) && audio) { /* son au clic géré par l'appelant */ }
        return clicked(x, y, size, size);
    }

    bool closeButton(int x, int y) {
        bool hov = hover(x, y, 18, 18);
        fe::rect(fb, x, y, 18, 18, hov ? UI_BAD : UI_SLOT);
        fe::rectOutline(fb, x, y, 18, 18, UI_BORDER);
        fe::drawTextCentered(fb, x + 9, y + 3, "X", fe::FONT_SMALL, UI_TEXT);
        return clicked(x, y, 18, 18);
    }

    void tooltip(int x, int y, const std::string& text) {
        int w = fe::textWidth(text, fe::FONT_SMALL) + 12;
        int h = fe::FONT_SMALL.height + 8;
        int tx = std::min(x, fb.w - w - 4);
        int ty = std::min(y, fb.h - h - 4);
        fe::rect(fb, tx, ty, w, h, UI_PANEL);
        fe::rectOutline(fb, tx, ty, w, h, UI_ACCENT);
        fe::drawText(fb, tx + 6, ty + 4, text, fe::FONT_SMALL, UI_TEXT);
    }
};

} // namespace fv
