# -*- coding: utf-8 -*-
"""Scarlet Horizon title backdrop.

Replaces the retired artwork that showed a Soviet star over a burning Moscow.
The frame states the premise in one image: five directions holding different
stretches of one horizon, with the scarlet line running between them. Drawing
order matters - ground, then skyline, then the horizon line on top, so the line
is never buried by the silhouettes.
"""
import random
from PIL import Image, ImageDraw, ImageFilter

W, H = 1920, 1080
HORIZON = 596
random.seed(20450)

BANDS = [
    (0.00, (92, 40, 150)),   # Eurasian plum
    (0.26, (26, 76, 184)),   # Atlantic cobalt
    (0.50, (20, 122, 84)),   # Eastern jade
    (0.72, (18, 130, 148)),  # Pacific turquoise
    (0.90, (168, 116, 40)),  # Independent amber
    (1.00, (168, 116, 40)),
]

def band(t):
    for i in range(len(BANDS) - 1):
        a, ca = BANDS[i]
        b, cb = BANDS[i + 1]
        if a <= t <= b:
            k = 0.0 if b == a else (t - a) / (b - a)
            k = k * k * (3 - 2 * k)
            return tuple(int(ca[c] + (cb[c] - ca[c]) * k) for c in range(3))
    return BANDS[-1][1]

img = Image.new('RGB', (W, H), (3, 4, 8))
d = ImageDraw.Draw(img)

# Sky: the band colour is strongest at the horizon and fades into overcast.
for y in range(HORIZON):
    v = y / HORIZON
    f = 0.06 + 0.94 * (v ** 2.6)
    for x in range(0, W, 4):
        r, g, b = band(x / W)
        d.rectangle([x, y, x + 3, y],
                    fill=(int(r * f) + 6, int(g * f) + 8, int(b * f) + 14))

# Ground plane, dark and slightly warm-neutral.
d.rectangle([0, HORIZON, W, H], fill=(5, 6, 10))

# Wet ground carrying a soft wash of the same directions.
wash = Image.new('RGB', (W, H), (0, 0, 0))
wd = ImageDraw.Draw(wash)
for y in range(HORIZON, H):
    k = (1.0 - (y - HORIZON) / float(H - HORIZON)) ** 2.2
    for x in range(0, W, 8):
        r, g, b = band(x / W)
        wd.rectangle([x, y, x + 7, y], fill=(int(r * k), int(g * k), int(b * k)))
img = Image.blend(img, wash.filter(ImageFilter.GaussianBlur(13)), 0.42)
d = ImageDraw.Draw(img)

# City glow, then the silhouettes that sit in front of it.
glow = Image.new('RGB', (W, H), (0, 0, 0))
gd = ImageDraw.Draw(glow)
for _ in range(420):
    x = random.randint(0, W)
    hgt = random.randint(14, 96)
    wdt = random.randint(5, 18)
    r, g, b = band(x / W)
    gd.rectangle([x, HORIZON - hgt, x + wdt, HORIZON],
                 fill=(min(255, r + 90), min(255, g + 90), min(255, b + 90)))
img = Image.blend(img, glow.filter(ImageFilter.GaussianBlur(9)), 0.40)
d = ImageDraw.Draw(img)

x = -20
while x < W + 20:
    wdt = random.randint(24, 92)
    hgt = random.randint(12, 84)
    d.rectangle([x, HORIZON - hgt, x + wdt, HORIZON + 2], fill=(4, 5, 9))
    x += wdt + random.randint(3, 22)

# The horizon itself, drawn last so nothing covers it.
for i, a in ((0, 1.00), (1, 0.92), (2, 0.66), (3, 0.44), (5, 0.24), (8, 0.12), (13, 0.05)):
    col = (int(60 + 195 * a), int(18 + 46 * a), int(20 + 48 * a))
    for s in ((-1, 1) if i else (0,)):
        d.line([(0, HORIZON + s * i), (W, HORIZON + s * i)], fill=col)

# Vignette keeps live text on a calm field without crushing the frame.
vig = Image.new('L', (W, H), 0)
ImageDraw.Draw(vig).ellipse(
    [-int(W * 0.34), -int(H * 0.50), int(W * 1.34), int(H * 1.50)], fill=255)
img = Image.composite(img, Image.new('RGB', (W, H), (4, 5, 10)),
                      vig.filter(ImageFilter.GaussianBlur(160)))

img.save('/tmp/T_RA4_TitleBackdrop.png')
print('title backdrop written', img.size)
