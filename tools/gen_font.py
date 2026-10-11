#!/usr/bin/env python3
"""Génère src/engine/font_data.h/.cpp : atlas de police bitmap pour le moteur logiciel.
Utilise DejaVu Sans Bold (présent sur le système) -> rendu propre et lisible en pixel-art.
Usage: python3 tools/gen_font.py
"""
from PIL import Image, ImageDraw, ImageFont
import os, sys

FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
CHARS = (
    "".join(chr(c) for c in range(32, 127))
    + "àâäçéèêëîïôöùûüœæÀÂÄÇÉÈÊËÎÏÔÖÙÛÜŒÆ’€°·★♥♪►▼▲►—«»’“”…%"
)
# dédoublonne en gardant l'ordre
seen = set()
CHARS = [c for c in CHARS if not (c in seen or seen.add(c))]

def build(size, name, out_prefix):
    font = ImageFont.truetype(FONT_PATH, size)
    pad = 2
    # mesure
    glyphs = []
    max_h = 0
    for ch in CHARS:
        bbox = font.getbbox(ch)
        w = max(1, bbox[2] - bbox[0])
        h = max(1, bbox[3] - bbox[1])
        glyphs.append([ch, w, h, bbox[0], bbox[1]])
        max_h = max(max_h, h)
    # atlas : simple packing en lignes
    atlas_w = 1024
    x = pad; y = pad; row_h = 0
    for g in glyphs:
        ch, w, h, x0, y0 = g
        if x + w + pad > atlas_w:
            x = pad; y += row_h + pad; row_h = 0
        g.extend([x, y])  # positions dans l'atlas
        x += w + pad
        row_h = max(row_h, h)
    atlas_h = y + row_h + pad
    img = Image.new("L", (atlas_w, atlas_h), 0)
    d = ImageDraw.Draw(img)
    for g in glyphs:
        ch, w, h, x0, y0, ax, ay = g
        d.text((ax - x0, ay - y0), ch, font=font, fill=255)
    px = img.load()
    atlas = bytearray()
    for yy in range(atlas_h):
        for xx in range(atlas_w):
            atlas.append(px[xx, yy])
    adv = {ch: font.getlength(ch) for ch in CHARS}
    ascent, descent = font.getmetrics()
    return glyphs, atlas, atlas_w, atlas_h, ascent, max_h, adv

def c_array_u8(name, data):
    lines = [f"static const uint8_t {name}[] = {{"]
    for i in range(0, len(data), 20):
        lines.append("  " + ",".join(str(b) for b in data[i:i+20]) + ",")
    lines.append("};")
    return "\n".join(lines)

def emit(size, name, guard):
    glyphs, atlas, aw, ah, ascent, max_h, adv = build(size, name, None)
    h = []
    h.append(f"// Généré automatiquement par tools/gen_font.py — ne pas éditer. Taille demandée: {size}px")
    h.append(f"static const int {name}_HEIGHT = {ascent + max_h};")
    h.append(f"static const int {name}_ASCENT = {ascent};")
    h.append(f"static const GlyphData {name}_GLYPHS[] = {{")
    for ch, w, gh, x0, y0, ax, ay in glyphs:
        cp = ord(ch)
        h.append(f"  {{0x{cp:X}, {ax}, {ay}, {w}, {gh}, {int(round(adv[ch]))}}},")
    h.append("};")
    cpp = []
    cpp.append(f"static const uint8_t {name}_ATLAS[] = {{")
    for i in range(0, len(atlas), 20):
        cpp.append("  " + ",".join(str(b) for b in atlas[i:i+20]) + ",")
    cpp.append("};")
    return "\n".join(h), "\n".join(cpp), len(glyphs), aw, ah

small_h, small_cpp, n1, aw1, ah1 = emit(14, "FONT_SMALL", None)
big_h, big_cpp, n2, aw2, ah2 = emit(26, "FONT_BIG", None)

hdr = f"""#pragma once
// Généré automatiquement par tools/gen_font.py — ne pas éditer.
#include <cstdint>
namespace fe {{
struct GlyphData {{ uint32_t cp; int16_t x, y, w, h; int16_t adv; }};
struct Font {{
  int height; int ascent; int atlasW; int atlasH;
  const GlyphData* glyphs; int glyphCount;
  const uint8_t* atlas;
}};
extern const Font FONT_SMALL;
extern const Font FONT_BIG;
}}
"""

cpp = f"""// Généré automatiquement par tools/gen_font.py — ne pas éditer.
#include "font_data.h"
namespace fe {{
{small_h}
{big_h}
{small_cpp}
{big_cpp}
const Font FONT_SMALL = {{ FONT_SMALL_HEIGHT, FONT_SMALL_ASCENT, {aw1}, {ah1}, FONT_SMALL_GLYPHS, {n1}, FONT_SMALL_ATLAS }};
const Font FONT_BIG   = {{ FONT_BIG_HEIGHT,   FONT_BIG_ASCENT,   {aw2}, {ah2}, FONT_BIG_GLYPHS,   {n2}, FONT_BIG_ATLAS }};
}}
"""

os.makedirs("src/engine", exist_ok=True)
with open("src/engine/font_data.h", "w") as f:
    f.write(hdr)
with open("src/engine/font_data.cpp", "w") as f:
    f.write(cpp)
print(f"OK: {n1} glyphes (small {aw1}x{ah1}), {n2} glyphes (big {aw2}x{ah2})")

if __name__ == "__main__":
    pass
