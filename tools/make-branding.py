#!/usr/bin/env python3
"""Genera gli asset dell'identità visiva di NicoOS 27 Extreme.

Scrittore PNG basato solo sulla libreria standard (niente PIL, niente
ImageMagick): uno sfondo scuro "extreme", la tessera del logo NicoOS e lo
splash di avvio BIOS/SYSLINUX. Output deterministico, così le ISO sono
riproducibili.

Esegui dalla radice del repository:  python3 tools/make-branding.py
"""
import math
import os
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROFILE = os.path.join(ROOT, "archiso")

ACCENT = (0x00, 0xD9, 0xFF)
ACCENT2 = (0x7A, 0x5C, 0xFF)
BG_TOP = (0x05, 0x07, 0x0D)
BG_BOTTOM = (0x0A, 0x14, 0x20)


def write_png(path, width, height, pixels):
    """pixels: bytearray di terne RGB, in ordine per righe."""
    raw = bytearray()
    stride = width * 3
    for y in range(height):
        raw.append(0)  # tipo di filtro 0
        raw += pixels[y * stride:(y + 1) * stride]

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as fh:
        fh.write(png)
    print(f"  {os.path.relpath(path, ROOT)}  {width}x{height}  {len(png) // 1024} KiB")


def clamp(v, lo=0, hi=255):
    return lo if v < lo else hi if v > hi else int(v)


def mix(a, b, t):
    return (clamp(a[0] + (b[0] - a[0]) * t),
            clamp(a[1] + (b[1] - a[1]) * t),
            clamp(a[2] + (b[2] - a[2]) * t))


def glow(px, py, cx, cy, radius, colour, strength):
    """Bagliore radiale additivo, restituisce il contributo (r, g, b)."""
    d = math.hypot(px - cx, py - cy) / radius
    if d >= 1.0:
        return (0, 0, 0)
    falloff = (1.0 - d) ** 2.2 * strength
    return (colour[0] * falloff, colour[1] * falloff, colour[2] * falloff)


def wallpaper(path, w=1920, h=1080, c1=ACCENT, c2=ACCENT2, top=BG_TOP, bottom=BG_BOTTOM):
    px = bytearray(w * h * 3)
    cx1, cy1 = w * 0.74, h * 0.22
    cx2, cy2 = w * 0.18, h * 0.88
    for y in range(h):
        t = y / (h - 1)
        base = mix(top, bottom, t ** 0.85)
        row = y * w * 3
        for x in range(w):
            g1 = glow(x, y, cx1, cy1, w * 0.55, c1, 0.34)
            g2 = glow(x, y, cx2, cy2, w * 0.5, c2, 0.22)
            r = base[0] + g1[0] + g2[0]
            g = base[1] + g1[1] + g2[1]
            b = base[2] + g1[2] + g2[2]
            # griglia tecnica tenue
            if x % 64 == 0 or y % 64 == 0:
                r += 3
                g += 5
                b += 7
            # linea d'accento all'orizzonte
            if abs(y - int(h * 0.62)) <= 1:
                fade = 1.0 - abs(x - w / 2) / (w / 2)
                r += c1[0] * 0.45 * fade
                g += c1[1] * 0.45 * fade
                b += c1[2] * 0.45 * fade
            i = row + x * 3
            px[i] = clamp(r)
            px[i + 1] = clamp(g)
            px[i + 2] = clamp(b)
    write_png(path, w, h, px)


# --- piccolo font a blocchi 5x7 (solo i glifi servono a splash/logo) -------
FONT = {
    "N": ("10001", "11001", "10101", "10011", "10001", "10001", "10001"),
    "I": ("11111", "00100", "00100", "00100", "00100", "00100", "11111"),
    "C": ("01110", "10001", "10000", "10000", "10000", "10001", "01110"),
    "O": ("01110", "10001", "10001", "10001", "10001", "10001", "01110"),
    "S": ("01111", "10000", "10000", "01110", "00001", "00001", "11110"),
    "2": ("01110", "10001", "00001", "00010", "00100", "01000", "11111"),
    "7": ("11111", "00001", "00010", "00100", "01000", "01000", "01000"),
    "E": ("11111", "10000", "10000", "11110", "10000", "10000", "11111"),
    "X": ("10001", "10001", "01010", "00100", "01010", "10001", "10001"),
    "T": ("11111", "00100", "00100", "00100", "00100", "00100", "00100"),
    "R": ("11110", "10001", "10001", "11110", "10100", "10010", "10001"),
    "M": ("10001", "11011", "10101", "10101", "10001", "10001", "10001"),
    "L": ("10000", "10000", "10000", "10000", "10000", "10000", "11111"),
    "V": ("10001", "10001", "10001", "10001", "10001", "01010", "00100"),
    "D": ("11110", "10001", "10001", "10001", "10001", "10001", "11110"),
    "K": ("10001", "10010", "10100", "11000", "10100", "10010", "10001"),
    "P": ("11110", "10001", "10001", "11110", "10000", "10000", "10000"),
    "A": ("01110", "10001", "10001", "11111", "10001", "10001", "10001"),
    "U": ("10001", "10001", "10001", "10001", "10001", "10001", "01110"),
    "G": ("01110", "10001", "10000", "10111", "10001", "10001", "01111"),
    "H": ("10001", "10001", "10001", "11111", "10001", "10001", "10001"),
    "B": ("11110", "10001", "10001", "11110", "10001", "10001", "11110"),
    "W": ("10001", "10001", "10001", "10101", "10101", "11011", "10001"),
    "F": ("11111", "10000", "10000", "11110", "10000", "10000", "10000"),
    "Y": ("10001", "10001", "01010", "00100", "00100", "00100", "00100"),
    "-": ("00000", "00000", "00000", "11111", "00000", "00000", "00000"),
    " ": ("00000",) * 7,
}


def text_mask(text):
    glyphs = [FONT.get(c, FONT[" "]) for c in text.upper()]
    width = sum(5 for _ in glyphs) + (len(glyphs) - 1)
    mask = [[0] * width for _ in range(7)]
    xoff = 0
    for g in glyphs:
        for y in range(7):
            for x in range(5):
                if g[y][x] == "1":
                    mask[y][xoff + x] = 1
        xoff += 6
    return mask, width, 7


def draw_text(px, w, h, text, scale, origin_x, origin_y, colour, alpha=1.0):
    mask, mw, mh = text_mask(text)
    for my in range(mh):
        for mx in range(mw):
            if not mask[my][mx]:
                continue
            for dy in range(scale):
                for dx in range(scale):
                    x = origin_x + mx * scale + dx
                    y = origin_y + my * scale + dy
                    if 0 <= x < w and 0 <= y < h:
                        i = (y * w + x) * 3
                        px[i] = clamp(px[i] * (1 - alpha) + colour[0] * alpha)
                        px[i + 1] = clamp(px[i + 1] * (1 - alpha) + colour[1] * alpha)
                        px[i + 2] = clamp(px[i + 2] * (1 - alpha) + colour[2] * alpha)


def fill_rect(px, w, h, x0, y0, x1, y1, colour, alpha=1.0):
    for y in range(max(0, y0), min(h, y1)):
        for x in range(max(0, x0), min(w, x1)):
            i = (y * w + x) * 3
            px[i] = clamp(px[i] * (1 - alpha) + colour[0] * alpha)
            px[i + 1] = clamp(px[i + 1] * (1 - alpha) + colour[1] * alpha)
            px[i + 2] = clamp(px[i + 2] * (1 - alpha) + colour[2] * alpha)


def splash(path, w=640, h=480):
    px = bytearray(w * h * 3)
    for y in range(h):
        t = y / (h - 1)
        base = mix((0x04, 0x06, 0x0C), (0x08, 0x12, 0x1E), t)
        for x in range(w):
            i = (y * w + x) * 3
            px[i], px[i + 1], px[i + 2] = base
    fill_rect(px, w, h, 0, 0, w, 3, ACCENT, 0.9)
    fill_rect(px, w, h, 0, h - 3, w, h, ACCENT2, 0.7)
    fill_rect(px, w, h, 80, 150, 560, 152, ACCENT, 0.55)
    draw_text(px, w, h, "NICOOS", 10, 80, 176, ACCENT)
    draw_text(px, w, h, "27 EXTREME", 5, 84, 258, (0xE6, 0xF5, 0xFF))
    draw_text(px, w, h, "DESKTOP LIVE + INSTALLER", 2, 84, 306, (0x6F, 0x8B, 0xA6))
    write_png(path, w, h, px)


def logo(path, size=256):
    px = bytearray(size * size * 3)
    for y in range(size):
        for x in range(size):
            i = (y * size + x) * 3
            # tessera quadrata con angoli arrotondati
            r = 48
            dx = max(r - x, x - (size - 1 - r), 0)
            dy = max(r - y, y - (size - 1 - r), 0)
            inside = math.hypot(dx, dy) <= r
            if not inside:
                continue
            d = math.hypot(x - size * 0.35, y - size * 0.3) / size
            base = mix((0x0C, 0x16, 0x24), (0x05, 0x08, 0x10), min(1.0, d * 1.6))
            px[i], px[i + 1], px[i + 2] = base
    # bordo d'accento
    for y in range(size):
        for x in range(size):
            r = 48
            dx = max(r - x, x - (size - 1 - r), 0)
            dy = max(r - y, y - (size - 1 - r), 0)
            dist = math.hypot(dx, dy)
            if 47 <= dist <= 48:
                i = (y * size + x) * 3
                px[i], px[i + 1], px[i + 2] = ACCENT
    draw_text(px, size, size, "N", 19, 40, 52, ACCENT)
    fill_rect(px, size, size, 40, 196, 216, 199, ACCENT2, 0.9)
    write_png(path, size, size, px)


# Sfondi alternativi che accompagnano quello predefinito. Vengono scritti in
# ./wallpapers, la cartella da cui build.sh li impacchetta in
# /usr/share/nicoos/wallpapers (una delle cartelle cercate dal selettore di
# sfondi della shell).
WALLPAPERS = (
    ("viola",    (0x7A, 0x5C, 0xFF), (0xFF, 0x3C, 0xA8)),
    ("ambra",    (0xFF, 0xB0, 0x3C), (0xFF, 0x5C, 0x3C)),
    ("verde",    (0x2C, 0xE6, 0xA8), (0x00, 0x9C, 0xFF)),
    ("ghiaccio", (0x9C, 0xD8, 0xFF), (0x3C, 0x6C, 0xFF)),
)


def main():
    print("Generazione degli asset di branding NicoOS:")
    wallpaper(os.path.join(PROFILE, "airootfs/usr/share/backgrounds/nicoos/nicoos-wallpaper.png"))
    for name, c1, c2 in WALLPAPERS:
        wallpaper(os.path.join(ROOT, "wallpapers", f"nicoos-{name}.png"), c1=c1, c2=c2)
    logo(os.path.join(PROFILE, "airootfs/usr/share/nicoos/branding/nicoos-logo.png"))
    logo(os.path.join(PROFILE, "airootfs/etc/calamares/branding/nicoos/logo.png"))
    logo(os.path.join(PROFILE, "airootfs/usr/share/sddm/themes/nicoos/logo.png"))
    splash(os.path.join(PROFILE, "syslinux/splash.png"))


if __name__ == "__main__":
    main()
