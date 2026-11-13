#!/usr/bin/env python3
"""Genera gli asset dell'identità visiva di NicoOS 27 Extreme.

Scrittore PNG basato solo sulla libreria standard (niente PIL, niente
ImageMagick): sfondo, splash di avvio BIOS/SYSLINUX e i formati del logo
ufficiale (nicoos27extreme.png) usati da Calamares, SDDM e Plymouth. I colori
primari sono il rosso del logo e il nero. Output deterministico, così le ISO
sono riproducibili.

Esegui dalla radice del repository:  python3 tools/make-branding.py
"""
import glob
import math
import os
import re
import struct
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROFILE = os.path.join(ROOT, "archiso")

# Colori primari di NicoOS: il rosso del logo ufficiale (nicoos27extreme.png,
# #e3272b) e il nero. Nessuna tinta blu resta negli asset generati.
ACCENT = (0xE3, 0x27, 0x2B)
ACCENT2 = (0xFF, 0xFF, 0xFF)
BG_TOP = (0xE3, 0x27, 0x2B)
BG_BOTTOM = (0x3A, 0x0C, 0x0F)
ACCENT_DARK = (0xB4, 0x1E, 0x22)


def write_png(path, width, height, pixels, alpha=False):
    """pixels: bytearray di terne RGB (o quaterne RGBA con alpha=True)."""
    channels = 4 if alpha else 3
    colour_type = 6 if alpha else 2  # 6 = RGBA, 2 = RGB
    raw = bytearray()
    stride = width * channels
    for y in range(height):
        raw.append(0)  # tipo di filtro 0
        raw += pixels[y * stride:(y + 1) * stride]

    def chunk(tag, data):
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, colour_type, 0, 0, 0))
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
                r += 6
                g += 6
                b += 6
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
        base = mix((0x08, 0x08, 0x08), (0x16, 0x16, 0x16), t)
        for x in range(w):
            i = (y * w + x) * 3
            px[i], px[i + 1], px[i + 2] = base
    fill_rect(px, w, h, 0, 0, w, 3, ACCENT, 0.9)
    fill_rect(px, w, h, 0, h - 3, w, h, ACCENT_DARK, 0.9)
    fill_rect(px, w, h, 80, 150, 560, 152, ACCENT, 0.55)
    draw_text(px, w, h, "NICOOS", 10, 80, 176, ACCENT)
    draw_text(px, w, h, "27 EXTREME", 5, 84, 258, (0xFF, 0x6B, 0x6B))
    draw_text(px, w, h, "DESKTOP LIVE + INSTALLER", 2, 84, 306, (0xB4, 0x1E, 0x22))
    write_png(path, w, h, px)


def read_png(path):
    """Legge un PNG RGB/RGBA a 8 bit (filtri 0-4): restituisce (w, h, RGBA)."""
    with open(path, "rb") as fh:
        data = fh.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: non è un PNG")

    pos, idat = 8, bytearray()
    width = height = depth = ctype = 0
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        tag = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        if tag == b"IHDR":
            width, height, depth, ctype = struct.unpack(">IIBB", payload[:10])
        elif tag == b"IDAT":
            idat += payload
        pos += 12 + length

    channels = {0: 1, 2: 3, 4: 2, 6: 4}[ctype]
    if depth != 8:
        raise ValueError(f"{path}: profondità {depth} bit non supportata")

    raw = zlib.decompress(bytes(idat))
    stride = width * channels
    rows = []
    prev = bytearray(stride)
    i = 0
    for _ in range(height):
        ftype = raw[i]
        i += 1
        line = bytearray(raw[i:i + stride])
        i += stride
        if ftype == 1:
            for x in range(channels, stride):
                line[x] = (line[x] + line[x - channels]) & 0xFF
        elif ftype == 2:
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 0xFF
        elif ftype == 3:
            for x in range(stride):
                left = line[x - channels] if x >= channels else 0
                line[x] = (line[x] + ((left + prev[x]) >> 1)) & 0xFF
        elif ftype == 4:
            for x in range(stride):
                left = line[x - channels] if x >= channels else 0
                up = prev[x]
                ul = prev[x - channels] if x >= channels else 0
                p = left + up - ul
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - ul)
                pred = left if (pa <= pb and pa <= pc) else (up if pb <= pc else ul)
                line[x] = (line[x] + pred) & 0xFF
        rows.append(line)
        prev = line

    rgba = bytearray(width * height * 4)
    for y, line in enumerate(rows):
        for x in range(width):
            src = x * channels
            dst = (y * width + x) * 4
            if channels >= 3:
                rgba[dst:dst + 3] = line[src:src + 3]
                rgba[dst + 3] = line[src + 3] if channels == 4 else 255
            else:
                v = line[src]
                rgba[dst] = rgba[dst + 1] = rgba[dst + 2] = v
                rgba[dst + 3] = line[src + 1] if channels == 2 else 255
    return width, height, rgba


def brand_mark(path, size):
    """Scrive il logo ufficiale (./nicoos27extreme.png) in un quadrato `size`,
    centrato e con i bordi trasparenti.

    Il sorgente è 157x151 px: per la splash su schermi grandi serve più
    risoluzione, quindi lo si ingrandisce con il vicino più prossimo, che su un
    marchio a tinte piatte resta nitido invece di sfocare i bordi.
    """
    sw, sh, pixels = read_png(os.path.join(ROOT, "nicoos27extreme.png"))
    scale = min(size / sw, size / sh)
    tw, th = max(1, int(sw * scale + 0.5)), max(1, int(sh * scale + 0.5))
    off_x, off_y = (size - tw) // 2, (size - th) // 2
    out = bytearray(size * size * 4)  # trasparente
    for y in range(th):
        sy = min(sh - 1, int(y * sh / th))
        for x in range(tw):
            sx = min(sw - 1, int(x * sw / tw))
            src = (sy * sw + sx) * 4
            if pixels[src + 3] == 0:
                continue
            dst = ((off_y + y) * size + (off_x + x)) * 4
            out[dst:dst + 4] = pixels[src:src + 4]
    write_png(path, size, size, out, alpha=True)
    print(f"  {os.path.relpath(path, ROOT)}  {size}x{size}  (logo ufficiale)")


def plymouth_dot(path, size=64, colour=ACCENT):
    """Punto rosso (colore del logo) con bordo morbido.

    La schermata di avvio lo rimpicciolisce a 4-16 px: con i bordi sfumati
    l'anello di caricamento resta fluido a qualsiasi risoluzione invece di
    mostrare scalini.
    """
    px = bytearray(size * size * 4)
    centre = (size - 1) / 2
    radius = size / 2 - 1
    for y in range(size):
        for x in range(size):
            d = math.hypot(x - centre, y - centre)
            a = (radius - d) / 1.6  # bordo morbido di 1,6 px
            a = 0.0 if a < 0 else (1.0 if a > 1 else a)
            i = (y * size + x) * 4
            px[i], px[i + 1], px[i + 2] = colour
            px[i + 3] = clamp(a * 255)
    write_png(path, size, size, px, alpha=True)


def plymouth_theme():
    """Asset del tema Plymouth (schermata di avvio).

    Il logo che compare sotto l'anello mentre gira è brand_mark() del marchio
    ufficiale ./nicoos27extreme.png, lo stesso disegno usato da Calamares e dal
    greeter SDDM: si cambia quell'immagine e si rigenera, non logo.png nel tema
    (viene sovrascritto a ogni esecuzione).
    """
    theme = os.path.join(PROFILE, "airootfs/usr/share/plymouth/themes/nicoos")
    os.makedirs(theme, exist_ok=True)
    plymouth_dot(os.path.join(theme, "dot.png"))

    brand_mark(os.path.join(theme, "logo.png"), 512)

    # Fotogrammi di splash.mp4 per la fase video di Plymouth. Se ffmpeg non è
    # disponibile si tengono i fotogrammi già presenti nel repository.
    plymouth_video_frames(theme)


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
    # Il logo di Calamares, quello del greeter SDDM e l'icona dell'app sono lo
    # stesso marchio ufficiale, in tre risoluzioni.
    brand_mark(os.path.join(PROFILE, "airootfs/usr/share/nicoos/branding/nicoos-logo.png"), 256)
    brand_mark(os.path.join(PROFILE, "airootfs/etc/calamares/branding/nicoos/logo.png"), 512)
    brand_mark(os.path.join(PROFILE, "airootfs/usr/share/sddm/themes/nicoos/logo.png"), 512)
    splash(os.path.join(PROFILE, "syslinux/splash.png"))
    plymouth_theme()


# Animazione di avvio Plymouth: 10 fotogrammi al secondo, cioè 0,1 s esatti
# per fotogramma. L'animazione ha la stessa durata del video perché i
# fotogrammi sono campionati a 10 fps: quelli intermedi di un sorgente a 30 fps
# non verrebbero mai mostrati, e occupare memoria per loro è solo costo.
PLYMOUTH_FPS = 10
PLYMOUTH_FRAME_SECONDS = 1 / PLYMOUTH_FPS
# Plymouth scala i fotogrammi alla risoluzione dello schermo a ogni cambio: oltre
# 1280 px di larghezza il PNG costa memoria e decodifica senza che si veda la
# differenza su una schermata di avvio.
PLYMOUTH_MAX_WIDTH = 1280


def plymouth_video_frames(theme):
    """Estrae i fotogrammi di ./splash.mp4 in theme/splashN.png per Plymouth.

    L'animazione è a PLYMOUTH_FPS: ogni fotogramma resta a schermo
    PLYMOUTH_FRAME_SECONDS (0,1 s) e la durata totale resta quella del video.
    I nomi partono da splash0.png, come li carica nicoos.script. Il conteggio
    deve stare anche in VIDEO_FRAMES (nicoos.script) e nell'attesa di
    nicoos-plymouth-fade-out.
    """
    import shutil, subprocess
    src = os.path.join(ROOT, "splash.mp4")
    if not os.path.exists(src):
        return
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        print("  plymouth video: ffmpeg non trovato, fotogrammi esistenti conservati")
        return
    # rimuove fotogrammi vecchi per evitare residui se il video è cambiato
    for old in glob.glob(os.path.join(theme, "splash*.png")):
        try:
            os.remove(old)
        except OSError:
            pass
    out_pattern = os.path.join(theme, "splash%d.png")
    # fps=10 campiona il video alla cadenza reale dell'animazione; scale limita
    # la larghezza; rgb24 (invece di rgba) tiene i PNG più piccoli e più
    # veloci da decodificare, e il fondo è comunque nero pieno.
    vf = (f"fps={PLYMOUTH_FPS},"
          f"scale='min({PLYMOUTH_MAX_WIDTH},iw)':-2:flags=lanczos,"
          f"format=rgb24")
    try:
        subprocess.run(
            [ffmpeg, "-y", "-hide_banner", "-loglevel", "error",
             "-i", src, "-vf", vf, "-start_number", "0", out_pattern],
            check=True,
        )
    except subprocess.CalledProcessError as e:
        print(f"  plymouth video: estrazione fallita ({e})")
        return
    frames = sorted(glob.glob(os.path.join(theme, "splash*.png")),
                    key=lambda p: int(os.path.basename(p)[6:-4]))
    n = len(frames)
    if frames:
        with open(frames[0], "rb") as fh:
            head = fh.read(24)
        width, height = struct.unpack(">II", head[16:24])
    else:
        width = height = 0
    print(f"  plymouth video: {n} fotogrammi {width}x{height} da splash.mp4"
          f" ({n * PLYMOUTH_FRAME_SECONDS:.1f} s a {PLYMOUTH_FPS} fps,"
          f" {PLYMOUTH_FRAME_SECONDS * 1000:.0f} ms ciascuno)")

    # Il tema carica un numero fisso di fotogrammi: se il video cambia e il
    # conteggio non viene aggiornato, la splash ne salta una parte.
    script = os.path.join(theme, "nicoos.script")
    try:
        with open(script, encoding="utf-8") as fh:
            match = re.search(r"^VIDEO_FRAMES\s*=\s*(\d+)\s*;", fh.read(), re.M)
    except OSError:
        match = None
    if match and int(match.group(1)) != n:
        print(f"  ATTENZIONE: nicoos.script ha VIDEO_FRAMES={match.group(1)},"
              f" i fotogrammi sono {n}: aggiorna il tema")


if __name__ == "__main__":
    main()
