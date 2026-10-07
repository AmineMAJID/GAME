#pragma once
// ============================================================
// FarmVale — moteur minimal (abstraction plateforme)
// Framebuffer logiciel 0xRRGGBB + entrée + audio.
// Backends : Win32 (Steam), X11 (Linux), headless (tests).
// ============================================================
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>

namespace fe {

using Color = uint32_t; // 0xRRGGBB

// --- Touches (indépendant de la plateforme) ---
enum Key {
    K_NONE = 0,
    K_UP, K_DOWN, K_LEFT, K_RIGHT,
    K_W, K_A, K_S, K_D,
    K_SPACE, K_ENTER, K_ESC, K_TAB,
    K_B, K_E, K_M, K_F, K_P, K_O,
    K_1, K_2, K_3, K_4, K_5, K_6, K_7, K_8, K_9, K_0, K_MINUS, K_EQUALS,
    K_COUNT
};

struct InputState {
    bool down[K_COUNT] = {};
    bool pressed[K_COUNT] = {};
    bool released[K_COUNT] = {};
    int mouseX = 0, mouseY = 0;   // coordonnées framebuffer
    bool mouseDown = false;
    bool mousePressed = false;
    bool mouseReleased = false;
    int wheel = 0;
    void endFrame() {
        for (int i = 0; i < K_COUNT; i++) { pressed[i] = false; released[i] = false; }
        mousePressed = mouseReleased = false;
        wheel = 0;
    }
    bool anyPressed() const {
        for (int i = 0; i < K_COUNT; i++) if (pressed[i]) return true;
        return mousePressed;
    }
};

// --- Framebuffer logiciel ---
struct Framebuffer {
    int w = 0, h = 0;
    std::vector<uint32_t> px; // 0xRRGGBB, row-major
    void resize(int w_, int h_) { w = w_; h = h_; px.assign((size_t)w * h, 0); }
    void clear(uint32_t c) { std::fill(px.begin(), px.end(), c); }
    inline uint32_t& at(int x, int y) { return px[(size_t)y * w + x]; }
    inline uint32_t get(int x, int y) const { return px[(size_t)y * w + x]; }
    bool in(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
};

// --- Interface plateforme ---
struct IPlatform {
    virtual ~IPlatform() = default;
    virtual bool poll() = 0;                    // pompe les événements ; false = quitter
    virtual void present(const Framebuffer&) = 0; // affiche la frame
    virtual void setTitle(const std::string&) {}
    virtual void beep(int hz, int ms) {}        // bip simple (fallback)
    virtual void playPcm(const int16_t* data, int samples, int rate) {} // son synthétisé
    virtual double timeMs() = 0;
    virtual void sleepMs(int ms) {}
    virtual void setFullscreen(bool) {}
    virtual bool isFullscreen() const { return false; }
    virtual void requestQuit() {}
    InputState input;
    int windowScale = 2;
    std::string saveDir = ".";                   // dossier des sauvegardes
};

} // namespace fe
