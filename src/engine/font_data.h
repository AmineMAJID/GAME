#pragma once
// Généré automatiquement par tools/gen_font.py — ne pas éditer.
#include <cstdint>
namespace fe {
struct GlyphData { uint32_t cp; int16_t x, y, w, h; int16_t adv; };
struct Font {
  int height; int ascent; int atlasW; int atlasH;
  const GlyphData* glyphs; int glyphCount;
  const uint8_t* atlas;
};
extern const Font FONT_SMALL;
extern const Font FONT_BIG;
}
