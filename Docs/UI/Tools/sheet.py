# -*- coding: utf-8 -*-
import sys
from PIL import Image, ImageDraw
paths = sys.argv[2:]
out = sys.argv[1]
COLS, CELL_W = 2, 620
thumbs = []
for p in paths:
    im = Image.open(p).convert('RGB')
    h = int(im.height * CELL_W / im.width)
    thumbs.append((p.split('/')[-1], im.resize((CELL_W, h), Image.LANCZOS)))
CELL_H = max(t[1].height for t in thumbs)
LABEL = 26
rows = (len(thumbs) + COLS - 1) // COLS
sheet = Image.new('RGB', (COLS*CELL_W + (COLS+1)*8, rows*(CELL_H+LABEL) + (rows+1)*8), (12,12,14))
d = ImageDraw.Draw(sheet)
for i, (name, im) in enumerate(thumbs):
    r, c = divmod(i, COLS)
    x = 8 + c*(CELL_W+8); y = 8 + r*(CELL_H+LABEL+8)
    d.text((x+4, y+4), name, fill=(255,220,120))
    sheet.paste(im, (x, y+LABEL))
sheet.save(out, quality=88)
print(out, sheet.size)
