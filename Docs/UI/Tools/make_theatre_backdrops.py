# -*- coding: utf-8 -*-
"""Battle theatre backdrops, one per direction.

The HUD screens still sat on retired art: a red Soviet command centre stood in
for the Eurasian and independent theatres, and a Chronolegion citadel stood in
for the Pacific. Each direction gets its own field instead, built from the same
palette the selection screens use, so a HUD reads as its own theatre before any
label is read.
"""
import random
from PIL import Image, ImageDraw, ImageFilter

W, H = 1920, 1080

# name: (sky, ground, accent, horizon_y, silhouette kind)
THEATRES = {
    'Eurasian':    ((44, 24, 78), (12, 8, 20), (150, 96, 210), 430, 'industrial'),
    'Atlantic':    ((16, 40, 92), (6, 12, 26), (86, 148, 232), 470, 'naval'),
    'Eastern':     ((26, 62, 44), (8, 16, 12), (120, 200, 140), 415, 'industrial'),
    'Pacific':     ((14, 62, 74), (5, 14, 18), (96, 208, 216), 455, 'islands'),
    'Independent': ((78, 52, 22), (16, 12, 6), (216, 168, 84), 440, 'ridges'),
}

def build(name, sky, ground, accent, horizon, kind, seed):
    random.seed(seed)
    img = Image.new('RGB', (W, H), ground)
    d = ImageDraw.Draw(img)

    for y in range(horizon):
        k = (y / horizon) ** 1.7
        d.line([(0, y), (W, y)], fill=(
            int(sky[0] * (0.30 + 1.05 * k)),
            int(sky[1] * (0.30 + 1.05 * k)),
            int(sky[2] * (0.30 + 1.05 * k))))

    # Horizon haze in the direction's accent.
    haze = Image.new('RGB', (W, H), (0, 0, 0))
    hd = ImageDraw.Draw(haze)
    hd.rectangle([0, horizon - 90, W, horizon + 30], fill=accent)
    img = Image.blend(img, haze.filter(ImageFilter.GaussianBlur(70)), 0.30)
    d = ImageDraw.Draw(img)

    # Skyline: each theatre has its own silhouette rhythm.
    x = -40
    while x < W + 40:
        if kind == 'industrial':
            w, h = random.randint(30, 110), random.randint(30, 170)
            d.rectangle([x, horizon - h, x + w, horizon + 6], fill=(ground[0] + 3, ground[1] + 3, ground[2] + 5))
            if random.random() < 0.35:
                d.rectangle([x + w // 3, horizon - h - random.randint(30, 90), x + w // 3 + 8, horizon - h],
                            fill=(ground[0] + 4, ground[1] + 4, ground[2] + 6))
            x += w + random.randint(6, 30)
        elif kind == 'naval':
            w, h = random.randint(90, 260), random.randint(12, 40)
            d.rectangle([x, horizon - h, x + w, horizon + 4], fill=(ground[0] + 4, ground[1] + 5, ground[2] + 8))
            x += w + random.randint(40, 150)
        elif kind == 'islands':
            w, h = random.randint(120, 320), random.randint(30, 110)
            d.polygon([(x, horizon + 4), (x + w // 2, horizon - h), (x + w, horizon + 4)],
                      fill=(ground[0] + 4, ground[1] + 6, ground[2] + 8))
            x += w + random.randint(30, 120)
        else:  # ridges
            w, h = random.randint(160, 380), random.randint(50, 150)
            d.polygon([(x, horizon + 4), (x + w // 3, horizon - h), (x + w, horizon + 4)],
                      fill=(ground[0] + 5, ground[1] + 4, ground[2] + 3))
            x += w // 2 + random.randint(20, 90)

    # Ground wash so the lower HUD panels sit on something, not on flat black.
    wash = Image.new('RGB', (W, H), (0, 0, 0))
    wd = ImageDraw.Draw(wash)
    for y in range(horizon, H):
        k = (1.0 - (y - horizon) / float(H - horizon)) ** 2.0
        wd.line([(0, y), (W, y)], fill=(int(accent[0] * k * 0.5), int(accent[1] * k * 0.5), int(accent[2] * k * 0.5)))
    img = Image.blend(img, wash.filter(ImageFilter.GaussianBlur(24)), 0.42)

    # The shared scarlet horizon line, thin and never a bloc colour.
    d = ImageDraw.Draw(img)
    for i, a in ((0, 1.0), (1, 0.55), (2, 0.28), (4, 0.12)):
        col = (int(70 + 175 * a), int(20 + 34 * a), int(22 + 36 * a))
        for s in ((-1, 1) if i else (0,)):
            d.line([(0, horizon + s * i), (W, horizon + s * i)], fill=col)

    vig = Image.new('L', (W, H), 0)
    ImageDraw.Draw(vig).ellipse([-W // 3, -H // 3, W + W // 3, H + H // 3], fill=255)
    img = Image.composite(img, Image.new('RGB', (W, H), tuple(max(0, c - 2) for c in ground)),
                          vig.filter(ImageFilter.GaussianBlur(170)))

    out = '/tmp/T_RA4_Theatre_%s.png' % name
    img.save(out)
    return out

for i, (name, args) in enumerate(THEATRES.items()):
    print(build(name, *args, seed=900 + i))
