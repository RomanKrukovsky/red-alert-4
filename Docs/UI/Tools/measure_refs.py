# -*- coding: utf-8 -*-
"""Measure every reference on the 1672x941 canvas: panel edges, bands, free field."""
import json, sys, os
sys.path.insert(0, os.path.dirname(__file__))
from compare import load, edge_profile, peaks, W, H

out = {}
for n in range(1, 25):
    im = load(f'SCREENSHOTS/{n}.png')
    cols, rows = edge_profile(im)
    xs, ys = peaks(cols, 0.30, 10), peaks(rows, 0.30, 10)

    # Luminance bands tell how much of the frame is dark scene versus lit chrome.
    g = im.convert('L'); px = g.load()
    dark = sum(1 for y in range(0, H, 3) for x in range(0, W, 3) if px[x, y] < 42)
    total = len(range(0, H, 3)) * len(range(0, W, 3))

    out[n] = {
        'vertical_edges': xs,
        'horizontal_edges': ys,
        'left_rail_x': xs[0] if xs else None,
        'right_rail_x': xs[-1] if xs else None,
        'top_band_y': ys[0] if ys else None,
        'bottom_band_y': ys[-1] if ys else None,
        'dark_share': round(dark / total, 3),
    }
    print(n, 'V:', xs[:8], 'H:', ys[:8], 'dark:', out[n]['dark_share'])

os.makedirs('Docs/UI/ReferenceGeometry', exist_ok=True)
with open('Docs/UI/ReferenceGeometry/measured.json', 'w') as f:
    json.dump(out, f, indent=2)
print('written Docs/UI/ReferenceGeometry/measured.json')
