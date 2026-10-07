#pragma once
// ============================================================
// FarmVale — primitives de dessin (framebuffer logiciel)
// ============================================================
#include "engine.h"
#include "font_data.h"
#include <string>

namespace fe {

// Couleurs utilitaires
inline Color rgb(int r, int g, int b) { return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b; }
inline Color lerp(Color a, Color b, float t) {
    int r = (int)((a >> 16 & 255) + ((b >> 16 & 255) - (a >> 16 & 255)) * t);
    int g = (int)((a >> 8 & 255) + ((b >> 8 & 255) - (a >> 8 & 255)) * t);
    int bl = (int)((a & 255) + ((b & 255) - (a & 255)) * t);
    return rgb(r, g, bl);
}

void px(Framebuffer& fb, int x, int y, Color c);
void rect(Framebuffer& fb, int x, int y, int w, int h, Color c);
void rectOutline(Framebuffer& fb, int x, int y, int w, int h, Color c);
void hline(Framebuffer& fb, int x0, int x1, int y, Color c);
void vline(Framebuffer& fb, int x, int y0, int y1, Color c);
void circle(Framebuffer& fb, int cx, int cy, int r, Color c);
void circleOutline(Framebuffer& fb, int cx, int cy, int r, Color c);
void blendRect(Framebuffer& fb, int x, int y, int w, int h, Color c, int alpha); // alpha 0..255
void blendPx(Framebuffer& fb, int x, int y, Color c, int alpha);

// Texte (UTF-8). Retourne la largeur en pixels.
int textWidth(const std::string& utf8, const Font& font);
void drawText(Framebuffer& fb, int x, int y, const std::string& utf8, const Font& font, Color c);
void drawTextShadow(Framebuffer& fb, int x, int y, const std::string& utf8, const Font& font, Color c);
// Texte centré dans un rectangle
void drawTextCentered(Framebuffer& fb, int cx, int y, const std::string& utf8, const Font& font, Color c);

// Décodage UTF-8 : retourne le prochain codepoint et avance l'index
uint32_t nextCodepoint(const std::string& s, size_t& i);

} // namespace fe
