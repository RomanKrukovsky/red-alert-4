# -*- coding: utf-8 -*-
"""Reference vs Unreal comparison: overlay, diff and panel-edge geometry deltas.

The references are photoreal concept frames while the implementation is flat UI,
so a raw pixel diff can never reach zero. What must match is *geometry*: where
panel edges sit. Panels are found by looking for long straight runs of contrast
along each axis, which survives the difference in rendering style.
"""
import json, os, sys
from PIL import Image, ImageChops, ImageDraw

W, H = 1672, 941

def load(path):
    im = Image.open(path).convert('RGB')
    return im if im.size == (W, H) else im.resize((W, H), Image.LANCZOS)

def edge_profile(im):
    """Per-column and per-row strength of vertical/horizontal edges."""
    g = im.convert('L')
    px = g.load()
    cols = [0.0] * W
    rows = [0.0] * H
    for y in range(0, H, 2):
        prev = px[0, y]
        for x in range(1, W):
            cur = px[x, y]
            d = abs(cur - prev)
            if d > 18:
                cols[x] += d
            prev = cur
    for x in range(0, W, 2):
        prev = px[x, 0]
        for y in range(1, H):
            cur = px[x, y]
            d = abs(cur - prev)
            if d > 18:
                rows[y] += d
            prev = cur
    return cols, rows

def peaks(profile, min_share=0.35, min_gap=8):
    top = max(profile) or 1.0
    cand = [(v, i) for i, v in enumerate(profile) if v >= top * min_share]
    cand.sort(reverse=True)
    chosen = []
    for v, i in cand:
        if all(abs(i - j) >= min_gap for j in chosen):
            chosen.append(i)
    return sorted(chosen)

def compare(ref_path, cur_path, out_dir, name):
    os.makedirs(out_dir, exist_ok=True)
    ref, cur = load(ref_path), load(cur_path)

    Image.blend(ref, cur, 0.5).save(os.path.join(out_dir, f'{name}_overlay.png'))
    diff = ImageChops.difference(ref, cur)
    diff.point(lambda v: min(255, v * 2)).save(os.path.join(out_dir, f'{name}_diff.png'))

    rc, rr = edge_profile(ref)
    cc, cr = edge_profile(cur)
    ref_x, ref_y = peaks(rc), peaks(rr)
    cur_x, cur_y = peaks(cc), peaks(cr)

    def match(a, b):
        out = []
        for v in a:
            if not b:
                out.append((v, None, None)); continue
            n = min(b, key=lambda u: abs(u - v))
            out.append((v, n, n - v))
        return out

    xs, ys = match(ref_x, cur_x), match(ref_y, cur_y)
    def stats(m):
        d = [abs(t[2]) for t in m if t[2] is not None]
        if not d: return {'count': 0}
        return {'count': len(d), 'within2': sum(1 for v in d if v <= 2),
                'within4': sum(1 for v in d if v <= 4), 'max': max(d),
                'mean': round(sum(d)/len(d), 1)}

    report = {'screen': name, 'vertical_edges': stats(xs), 'horizontal_edges': stats(ys),
              'ref_x': ref_x, 'cur_x': cur_x, 'ref_y': ref_y, 'cur_y': cur_y}
    with open(os.path.join(out_dir, f'{name}_geometry.json'), 'w') as f:
        json.dump(report, f, indent=2)

    marked = cur.copy(); d = ImageDraw.Draw(marked)
    for x in ref_x: d.line([(x, 0), (x, H)], fill=(255, 60, 60), width=1)
    for x in cur_x: d.line([(x, 0), (x, H)], fill=(60, 255, 120), width=1)
    for y in ref_y: d.line([(0, y), (W, y)], fill=(255, 60, 60), width=1)
    for y in cur_y: d.line([(0, y), (W, y)], fill=(60, 255, 120), width=1)
    marked.save(os.path.join(out_dir, f'{name}_edges.png'))
    return report

if __name__ == '__main__':
    r = compare(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
    print(json.dumps({k: v for k, v in r.items() if not k.startswith(('ref_', 'cur_'))}, indent=2))
