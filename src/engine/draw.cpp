#include "draw.h"
#include <cmath>

namespace fe {

void px(Framebuffer& fb, int x, int y, Color c) {
    if (fb.in(x, y)) fb.at(x, y) = c;
}

void rect(Framebuffer& fb, int x, int y, int w, int h, Color c) {
    int x0 = std::max(0, x), y0 = std::max(0, y);
    int x1 = std::min(fb.w, x + w), y1 = std::min(fb.h, y + h);
    for (int yy = y0; yy < y1; yy++) {
        uint32_t* row = &fb.px[(size_t)yy * fb.w];
        for (int xx = x0; xx < x1; xx++) row[xx] = c;
    }
}

void rectOutline(Framebuffer& fb, int x, int y, int w, int h, Color c) {
    hline(fb, x, x + w - 1, y, c);
    hline(fb, x, x + w - 1, y + h - 1, c);
    vline(fb, x, y, y + h - 1, c);
    vline(fb, x + w - 1, y, y + h - 1, c);
}

void hline(Framebuffer& fb, int x0, int x1, int y, Color c) {
    if (y < 0 || y >= fb.h) return;
    if (x0 > x1) std::swap(x0, x1);
    x0 = std::max(0, x0); x1 = std::min(fb.w - 1, x1);
    uint32_t* row = &fb.px[(size_t)y * fb.w];
    for (int x = x0; x <= x1; x++) row[x] = c;
}

void vline(Framebuffer& fb, int x, int y0, int y1, Color c) {
    if (x < 0 || x >= fb.w) return;
    if (y0 > y1) std::swap(y0, y1);
    y0 = std::max(0, y0); y1 = std::min(fb.h - 1, y1);
    for (int y = y0; y <= y1; y++) fb.px[(size_t)y * fb.w + x] = c;
}

void circle(Framebuffer& fb, int cx, int cy, int r, Color c) {
    for (int y = -r; y <= r; y++) {
        int dx = (int)std::sqrt((float)(r * r - y * y));
        hline(fb, cx - dx, cx + dx, cy + y, c);
    }
}

void circleOutline(Framebuffer& fb, int cx, int cy, int r, Color c) {
    for (int y = -r; y <= r; y++) {
        int dx = (int)std::sqrt((float)(r * r - y * y));
        px(fb, cx - dx, cy + y, c);
        px(fb, cx + dx, cy + y, c);
    }
    for (int x = -r; x <= r; x++) {
        int dy = (int)std::sqrt((float)(r * r - x * x));
        px(fb, cx + x, cy - dy, c);
        px(fb, cx + x, cy + dy, c);
    }
}

void blendPx(Framebuffer& fb, int x, int y, Color c, int alpha) {
    if (!fb.in(x, y) || alpha <= 0) return;
    if (alpha >= 255) { fb.at(x, y) = c; return; }
    uint32_t& d = fb.at(x, y);
    int inv = 255 - alpha;
    int r = (int)((c >> 16 & 255) * alpha + (d >> 16 & 255) * inv) / 255;
    int g = (int)((c >> 8 & 255) * alpha + (d >> 8 & 255) * inv) / 255;
    int b = (int)((c & 255) * alpha + (d & 255) * inv) / 255;
    d = rgb(r, g, b);
}

void blendRect(Framebuffer& fb, int x, int y, int w, int h, Color c, int alpha) {
    int x0 = std::max(0, x), y0 = std::max(0, y);
    int x1 = std::min(fb.w, x + w), y1 = std::min(fb.h, y + h);
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++)
            blendPx(fb, xx, yy, c, alpha);
}

// ------------------------------------------------------------
// Texte
// ------------------------------------------------------------
uint32_t nextCodepoint(const std::string& s, size_t& i) {
    unsigned char c = (unsigned char)s[i++];
    if (c < 0x80) return c;
    if ((c & 0xE0) == 0xC0 && i < s.size()) {
        uint32_t cp = c & 0x1F;
        cp = (cp << 6) | ((unsigned char)s[i++] & 0x3F);
        return cp;
    }
    if ((c & 0xF0) == 0xE0 && i + 1 < s.size()) {
        uint32_t cp = c & 0x0F;
        cp = (cp << 6) | ((unsigned char)s[i++] & 0x3F);
        cp = (cp << 6) | ((unsigned char)s[i++] & 0x3F);
        return cp;
    }
    return c; // repli
}

static const GlyphData* findGlyph(const Font& font, uint32_t cp) {
    for (int i = 0; i < font.glyphCount; i++)
        if (font.glyphs[i].cp == cp) return &font.glyphs[i];
    return nullptr;
}

int textWidth(const std::string& utf8, const Font& font) {
    int w = 0;
    size_t i = 0;
    while (i < utf8.size()) {
        uint32_t cp = nextCodepoint(utf8, i);
        const GlyphData* g = findGlyph(font, cp);
        w += g ? g->adv : font.height / 2;
    }
    return w;
}

void drawText(Framebuffer& fb, int x, int y, const std::string& utf8, const Font& font, Color c) {
    size_t i = 0;
    int cx = x;
    while (i < utf8.size()) {
        uint32_t cp = nextCodepoint(utf8, i);
        const GlyphData* g = findGlyph(font, cp);
        if (!g) { cx += font.height / 2; continue; }
        for (int gy = 0; gy < g->h; gy++) {
            for (int gx = 0; gx < g->w; gx++) {
                uint8_t a = font.atlas[(size_t)(g->y + gy) * font.atlasW + (g->x + gx)];
                if (a > 8) blendPx(fb, cx + gx, y + gy, c, a);
            }
        }
        cx += g->adv;
    }
}

void drawTextShadow(Framebuffer& fb, int x, int y, const std::string& utf8, const Font& font, Color c) {
    drawText(fb, x + 1, y + 1, utf8, font, rgb(20, 16, 24));
    drawText(fb, x, y, utf8, font, c);
}

void drawTextCentered(Framebuffer& fb, int cx, int y, const std::string& utf8, const Font& font, Color c) {
    drawText(fb, cx - textWidth(utf8, font) / 2, y, utf8, font, c);
}

} // namespace fe
