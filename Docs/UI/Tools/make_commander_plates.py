# -*- coding: utf-8 -*-
"""Commander portrait plates, one per direction.

The retired art showed a Soviet marshal and a US president, which both belongs to
the removed world and contradicts the names the screens print (Volkova, Reed).
Until photographed or painted portraits exist, each direction gets a deliberate
plate: an operations-room field in its own colour with a silhouette and rank bar.
A plate that reads as designed UI is honest; wrong-world art is not.
"""
import random
from PIL import Image, ImageDraw, ImageFilter

W, H = 720, 900

DIRECTIONS = {
    'Eurasian':    ((86, 40, 140), (150, 96, 210)),
    'Atlantic':    ((22, 62, 150), (86, 148, 232)),
    'Eastern':     ((18, 104, 72), (96, 190, 128)),
    'Pacific':     ((16, 112, 128), (86, 200, 210)),
    'Independent': ((140, 96, 34), (216, 168, 84)),
}

def plate(name, base, accent, seed):
    random.seed(seed)
    img = Image.new('RGB', (W, H), (5, 6, 10))
    d = ImageDraw.Draw(img)

    # Operations room behind the figure: a soft field plus console glows.
    for y in range(H):
        k = (y / H) ** 1.4
        d.line([(0, y), (W, y)], fill=(
            int(base[0] * (0.34 + 0.95 * (1 - k))),
            int(base[1] * (0.34 + 0.95 * (1 - k))),
            int(base[2] * (0.34 + 0.95 * (1 - k)))))
    glow = Image.new('RGB', (W, H), (0, 0, 0))
    gd = ImageDraw.Draw(glow)
    for _ in range(90):
        x = random.randint(0, W); y = random.randint(int(H * 0.18), int(H * 0.72))
        gd.rectangle([x, y, x + random.randint(6, 26), y + random.randint(3, 10)],
                     fill=(min(255, accent[0] + 40), min(255, accent[1] + 40), min(255, accent[2] + 40)))
    img = Image.blend(img, glow.filter(ImageFilter.GaussianBlur(14)), 0.48)
    d = ImageDraw.Draw(img)

    # Figure: shoulders and head as a clean silhouette, no face and no insignia
    # that could read as a real force's emblem.
    cx = W // 2
    d.ellipse([cx - 108, 214, cx + 108, 452], fill=(9, 10, 15))
    d.polygon([(cx - 250, H), (cx - 196, 470), (cx - 96, 418),
               (cx + 96, 418), (cx + 196, 470), (cx + 250, H)], fill=(9, 10, 15))
    # Rank bar on the shoulder, in the direction's accent.
    for i in range(3):
        d.rectangle([cx + 108 + i * 22, 520 + i * 6, cx + 122 + i * 22, 566 + i * 6], fill=accent)
        d.rectangle([cx - 122 - i * 22, 520 + i * 6, cx - 108 - i * 22, 566 + i * 6], fill=accent)

    # Rim light so the silhouette separates from the field.
    rim = Image.new('RGB', (W, H), (0, 0, 0))
    rd = ImageDraw.Draw(rim)
    rd.ellipse([cx - 112, 210, cx + 112, 456], outline=accent, width=3)
    rd.line([(cx - 200, 468), (cx - 98, 414)], fill=accent, width=3)
    rd.line([(cx + 200, 468), (cx + 98, 414)], fill=accent, width=3)
    img = Image.blend(img, rim.filter(ImageFilter.GaussianBlur(5)), 0.55)

    # Vignette keeps the caption legible wherever the screen puts it.
    vig = Image.new('L', (W, H), 0)
    ImageDraw.Draw(vig).ellipse([-W // 3, -H // 4, W + W // 3, H + H // 4], fill=255)
    img = Image.composite(img, Image.new('RGB', (W, H), (4, 5, 9)),
                          vig.filter(ImageFilter.GaussianBlur(150)))
    out = '/tmp/T_RA4_Commander_%s.png' % name
    img.save(out)
    return out

for i, (name, (base, accent)) in enumerate(DIRECTIONS.items()):
    print(plate(name, base, accent, 700 + i))
