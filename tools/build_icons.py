#!/usr/bin/env python3
"""Builds the class icons for the SD card from the artwork in assets/src/.
Run from the project root:   python3 tools/build_icons.py SD_CARD
Needs: pip install pillow numpy scipy

Writes <SD>/grimoire/icons/<class>_{t,w,b}.bin and <SD>/neon/icons/<class>_{t,w,b}.bin
  _t  36x36   tile icon (full brightness, pre-blended on the tile color)
  _w  34x34   dim watermark for the compact tiles
  _b 160x160  dim, desaturated background for the class screen
Raw RGB565 little-endian, no header. Black in the artwork becomes the app background.
"""
import sys, os, re
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

SD = sys.argv[1] if len(sys.argv) > 1 else 'SD_CARD'
SRC = 'assets/src/'
BG = np.array([14, 12, 24]) / 255.0       # C_BG
PANEL = np.array([36, 30, 58]) / 255.0    # C_PANEL
T, W, B = 36, 34, 160

# class colors, same table as src/main.cpp (index = order in classes.txt)
CLASS_RGB = [(200,90,200),(240,220,120),(90,190,90),(240,200,70),(70,170,110),(230,90,70),(150,90,220),
             (80,140,240),(240,130,60),(90,210,220),(230,110,160),(170,200,80),(120,130,240),(220,80,120),(100,220,170),(200,160,110)]

def load(name):
    im = Image.open(SRC + name).convert('RGB')
    fl = 0.16 if name.endswith('.jpg') else 0.06   # the artwork's 'black' is not pure black: lift it so no box shows
    if True:
        a = np.asarray(im).astype(float) / 255.0
        a = np.clip((a - fl) / (1 - fl), 0, 1)
        im = Image.fromarray((a * 255).astype(np.uint8))
    return im

def components(im, thr=70, grow=12, minsz=60):
    a = np.asarray(im).max(2)
    lab, n = ndi.label(ndi.binary_dilation(a > thr, iterations=grow))
    boxes = []
    for s in ndi.find_objects(lab):
        h, w = s[0].stop - s[0].start, s[1].stop - s[1].start
        if h > minsz and w > minsz:
            boxes.append((s[1].start, s[0].start, s[1].stop, s[0].stop))
    return boxes

def grid_sort(boxes, rows):
    boxes = sorted(boxes, key=lambda b: (b[1] + b[3]) / 2)
    per = len(boxes) // rows
    out = []
    for r in range(rows):
        out += sorted(boxes[r * per:(r + 1) * per], key=lambda b: b[0])
    return out

def square(im, box, pad=0.06):
    """crop exactly the box (so neighbouring artwork never leaks in) and centre it on a black square"""
    x0, y0, x1, y1 = [int(v) for v in box]
    c = im.crop((x0, y0, x1, y1))
    s = int(max(c.size) * (1 + pad * 2))
    out = Image.new('RGB', (s, s), (0, 0, 0))
    out.paste(c, ((s - c.size[0]) // 2, (s - c.size[1]) // 2))
    return out

def cells(im, cols, rows, inset=0):
    w, h = im.size
    cw, ch = w / cols, h / rows
    return [im.crop((int(c * cw) + inset, int(r * ch) + inset, int((c + 1) * cw) - inset, int((r + 1) * ch) - inset))
            for r in range(rows) for c in range(cols)]

def trim(im, thr=60):
    a = np.asarray(im).max(2)
    ys, xs = np.where(a > thr)
    if len(xs) == 0: return im
    return square(im, (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1), 0.06)

def tint_white(im, rgb):
    a = np.asarray(im).astype(float) / 255.0
    lum = a.max(2)[..., None]
    return Image.fromarray((np.clip(lum * (np.array(rgb) / 255.0), 0, 1) * 255).astype(np.uint8))

def mix_tint(im, rgb, k=0.5):
    a = np.asarray(im).astype(float) / 255.0
    lum = a.max(2)[..., None]
    t = lum * (np.array(rgb) / 255.0)
    return Image.fromarray((np.clip(a * (1 - k) + t * k, 0, 1) * 255).astype(np.uint8))

def composite(im, size, bg, intensity=1.0, sat=1.0):
    im = im.resize((size, size), Image.LANCZOS)
    p = np.asarray(im).astype(float) / 255.0
    g = p.mean(2, keepdims=True)
    p = g + (p - g) * sat
    a = np.clip(p.max(2, keepdims=True) * 1.3, 0, 1)
    out = p * intensity + bg * (1 - a * intensity)
    return np.clip(out, 0, 1)

def rgb565(arr):
    v = (arr * 255 + 0.5).astype(np.uint16)
    return (((v[..., 0] >> 3) << 11) | ((v[..., 1] >> 2) << 5) | (v[..., 2] >> 3)).astype('<u2').tobytes()

def write(root, cls, tile_src, bg_src):
    d = os.path.join(SD, root, 'icons'); os.makedirs(d, exist_ok=True)
    n = cls.lower()
    open(os.path.join(d, n + '_t.bin'), 'wb').write(rgb565(composite(tile_src, T, PANEL)))
    open(os.path.join(d, n + '_w.bin'), 'wb').write(rgb565(composite(tile_src, W, PANEL, 0.30, 0.8)))
    open(os.path.join(d, n + '_b.bin'), 'wb').write(rgb565(composite(bg_src, B, BG, 0.30, 0.55)))

# ---- D&D 2024: flat image for tiles, detailed one for backgrounds (4 x 3 grids, same order) ----
DND = ['Fighter', 'Barbarian', 'Paladin', 'Cleric', 'Rogue', 'Ranger', 'Wizard', 'Sorcerer', 'Warlock', 'Druid', 'Bard', 'Monk']
flat = load('dnd_flat.png'); detail = load('dnd_detail.png')
fb = grid_sort(components(flat), 3)
assert len(fb) == 12, len(fb)
flat_icons = [square(flat, b) for b in fb]
detail_icons = [trim(c) for c in cells(detail, 4, 3)]
SRD_ORDER = ['Bard', 'Cleric', 'Druid', 'Paladin', 'Ranger', 'Sorcerer', 'Warlock', 'Wizard']   # order in grimoire classes.txt
for n, ti, bi in zip(DND, flat_icons, detail_icons):
    if n in SRD_ORDER:                      # recolor to the tile frame color so icon and frame match
        col = CLASS_RGB[SRD_ORDER.index(n)]
        ti = tint_white(ti, col)
        bi = mix_tint(bi, col, 0.5)
    write('grimoire', n, ti, bi)

# ---- Neon Odyssey: white silhouettes (tile, tinted per class) + soft neon art (background) ----
NEON = ['Cryptist', 'Disciple', 'Enforcer', 'Esper', 'Gridrunner', 'Icon', 'Juggernaut', 'Machinist', 'Oracle',
        'Primalist', 'Scoundrel', 'Soldier', 'Tracer']
sil = [trim(c, 120) for c in cells(load('neon_icons.jpg'), 4, 4, inset=8)][:13]
bgs = [square(load('neon_bg.jpg'), b) for b in grid_sort(components(load('neon_bg.jpg')), 4)]   # 12, no Oracle
assert len(bgs) == 12, len(bgs)
extra = load('neon_extra.jpg')
eye = square(extra, (1109, 279, 1324, 499))                                        # the eye = Oracle
bgs.insert(NEON.index('Oracle'), eye)
for i, n in enumerate(NEON):
    col = CLASS_RGB[i]
    write('neon', n, tint_white(sil[i], col), mix_tint(bgs[i], col, 0.5))
print('icons written to', SD)
