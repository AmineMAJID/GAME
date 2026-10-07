#!/usr/bin/env python3
# ============================================================
# FarmVale — génère les artworks de la page Steam (capsules)
# Tailles officielles Steam :
#   header_capsule.png   460×215  (en-tête page magasin)
#   main_capsule.png     616×353  (capsule principale)
#   small_capsule.png    231×87   (petite capsule, liste de jeux)
#   vertical_capsule.png 374×448  (bibliothèque Steam)
#   library_hero.png     1920×620 (fond bibliothèque, optionnel)
#   library_logo.png     960×360  (logo transparent, optionnel)
# Usage : python3 tools/gen_art.py
# ============================================================
import os
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.join(os.path.dirname(__file__), "..")
OUT = os.path.join(ROOT, "steam")
os.makedirs(OUT, exist_ok=True)

FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
if not os.path.exists(FONT_PATH):
    FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"


def font(size):
    return ImageFont.truetype(FONT_PATH, size)


def lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def draw_scene(img, W, H, with_title=True, title_scale=1.0):
    """Dessine la scène de ferme FarmVale dans l'image (W×H)."""
    d = ImageDraw.Draw(img)
    # --- Ciel dégradé
    horizon = int(H * 0.52)
    for y in range(horizon):
        t = y / max(1, horizon)
        d.line([(0, y), (W, y)], fill=lerp((62, 105, 165), (158, 200, 233), t))
    # --- Soleil
    sun_x, sun_y, sun_r = int(W * 0.82), int(H * 0.16), int(H * 0.075)
    for r in range(sun_r + 14, sun_r, -4):
        d.ellipse([sun_x - r, sun_y - r, sun_x + r, sun_y + r], fill=(255, 224, 120))
    d.ellipse([sun_x - sun_r, sun_y - sun_r, sun_x + sun_r, sun_y + sun_r], fill=(255, 238, 160))
    # --- Nuages
    for i, (cx, cy, s) in enumerate([(0.12, 0.10, 1.0), (0.38, 0.18, 0.8), (0.62, 0.08, 1.1), (0.93, 0.24, 0.7)]):
        cx, cy, s = int(cx * W), int(cy * H), s * H * 0.045
        for dx, dy, r in [(-s, 0, s), (0, -s * 0.7, s * 1.2), (s, 0, s), (0, 0, s * 0.9)]:
            d.ellipse([cx + dx - r, cy + dy - r, cx + dx + r, cy + dy + r], fill=(245, 248, 252))
    # --- Champ
    d.rectangle([0, horizon, W, H], fill=(96, 168, 74))
    # sillons de culture
    row_h = max(4, H // 60)
    y = horizon + row_h
    while y < H:
        d.line([(0, y), (W, y)], fill=(78, 142, 58), width=max(1, row_h // 3))
        # plants (blé) sur le sillon
        step = max(8, W // 40)
        for x in range(step // 2, W, step):
            d.ellipse([x - 2, y - row_h // 2 - 3, x + 2, y - row_h // 2 + 3], fill=(222, 184, 90))
            d.line([(x, y - row_h // 2 + 3), (x, y - row_h // 2 + 7)], fill=(70, 120, 50), width=1)
        y += row_h * 2
    # --- Maison (à gauche)
    hx, hy = int(W * 0.10), int(H * 0.30)
    hw, hh = int(W * 0.16), int(H * 0.20)
    # murs
    d.rectangle([hx, hy + hh // 3, hx + hw, hy + hh], fill=(238, 226, 196))
    # toit (trapèze rouge)
    d.polygon([(hx - hw // 8, hy + hh // 3), (hx + hw + hw // 8, hy + hh // 3),
               (hx + hw * 3 // 4, hy), (hx + hw // 4, hy)], fill=(178, 62, 48))
    # porte + fenêtre
    d.rectangle([hx + hw // 2 - 8, hy + hh - hh // 3, hx + hw // 2 + 8, hy + hh], fill=(110, 72, 40))
    d.rectangle([hx + 8, hy + hh // 2 - 6, hx + 22, hy + hh // 2 + 8], fill=(255, 230, 140))
    # --- Arbre (à droite de la maison)
    tx, ty = int(W * 0.32), int(H * 0.26)
    trunk_w, trunk_h = int(H * 0.018), int(H * 0.10)
    d.rectangle([tx - trunk_w // 2, ty + int(H * 0.10), tx + trunk_w // 2, ty + int(H * 0.10) + trunk_h], fill=(110, 74, 42))
    for dx, dy, r in [(-int(H * 0.05), 0, int(H * 0.07)), (int(H * 0.04), -int(H * 0.02), int(H * 0.06)),
                      (0, -int(H * 0.06), int(H * 0.075))]:
        d.ellipse([tx + dx - r, ty + dy - r, tx + dx + r, ty + dy + r], fill=(52, 128, 62))
    # --- Vache (petite, devant la maison)
    vx, vy = int(W * 0.20), int(H * 0.56)
    vs = H * 0.035
    d.ellipse([vx - vs, vy - vs * 0.6, vx + vs, vy + vs * 0.6], fill=(250, 250, 250))  # corps
    d.ellipse([vx + vs * 0.7, vy - vs * 0.8, vx + vs * 1.2, vy - vs * 0.2], fill=(250, 250, 250))  # tête
    d.ellipse([vx - vs * 0.5, vy - vs * 0.4, vx - vs * 0.1, vy + vs * 0.1], fill=(60, 60, 60))  # tache
    d.ellipse([vx + vs * 0.85, vy - vs * 0.65, vx + vs * 0.95, vy - vs * 0.55], fill=(30, 30, 30))  # œil
    # --- Poule
    px, py = int(W * 0.30), int(H * 0.60)
    ps = H * 0.02
    d.ellipse([px - ps, py - ps, px + ps, py + ps], fill=(255, 255, 255))
    d.ellipse([px + ps * 0.6, py - ps * 1.2, px + ps * 1.1, py - ps * 0.6], fill=(255, 255, 255))
    d.polygon([(px + ps * 1.0, py - ps * 0.9), (px + ps * 1.4, py - ps * 0.8), (px + ps * 1.0, py - ps * 0.7)], fill=(240, 160, 40))
    # --- Titre
    if with_title:
        title = "FarmVale"
        sub = "La Ferme des Quatre Saisons"
        ts = max(10, int(H * 0.115 * title_scale))
        ss = max(8, int(H * 0.045 * title_scale))
        f1, f2 = font(ts), font(ss)
        # ombre du titre
        d.text((W // 2 + 3, horizon - int(H * 0.16) + 3), title, font=f1, fill=(30, 40, 30), anchor="mm")
        d.text((W // 2, horizon - int(H * 0.16)), title, font=f1, fill=(255, 244, 200), anchor="mm")
        d.text((W // 2, horizon - int(H * 0.16) + ts // 2 + ss // 2 + 4), sub, font=f2, fill=(255, 214, 110), anchor="mm")


def make(name, w, h, with_title=True, title_scale=1.0, bg_solid=None):
    # Rendu 2× plus grand puis réduction LANCZOS (plus net)
    img = Image.new("RGB", (w * 2, h * 2), (0, 0, 0))
    draw_scene(img, w * 2, h * 2, with_title, title_scale)
    img = img.resize((w, h), Image.LANCZOS)
    path = os.path.join(OUT, name)
    img.save(path)
    print(f"  {name}  {w}×{h}")


def make_logo():
    """Logo transparent (library_logo) : juste le titre sur fond transparent."""
    W, H = 960, 360
    img = Image.new("RGBA", (W * 2, H * 2), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    f1 = font(int(H * 2 * 0.28))
    f2 = font(int(H * 2 * 0.10))
    d.text((W, H * 0.42), "FarmVale", font=f1, fill=(255, 244, 200, 255), anchor="mm")
    d.text((W, H * 0.68), "La Ferme des Quatre Saisons", font=f2, fill=(255, 214, 110, 255), anchor="mm")
    # liseré sombre pour la lisibilité
    img = img.resize((W, H), Image.LANCZOS)
    path = os.path.join(OUT, "library_logo.png")
    img.save(path)
    print(f"  library_logo.png  {W}×{H} (transparent)")


print("Génération des artworks Steam -> steam/")
make("header_capsule.png", 460, 215)
make("main_capsule.png", 616, 353)
make("small_capsule.png", 231, 87, title_scale=0.85)
make("vertical_capsule.png", 374, 448, title_scale=0.9)
make("library_hero.png", 1920, 620, title_scale=1.15)
make_logo()
print("Terminé. Uploade ces fichiers dans Steamworks -> Store Page -> Artwork.")
