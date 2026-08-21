# -*- coding: utf-8 -*-
"""Scarlet Horizon wordmark: cold steel letters cut by the scarlet horizon line."""
from PIL import Image, ImageDraw, ImageFont

W, H = 1024, 320
FONT = 'Content/RA4UI/Fonts/RA4_RobotoCondensedSemiBold.ttf'
img = Image.new('RGBA', (W, H), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

big = ImageFont.truetype(FONT, 132)
mid = ImageFont.truetype(FONT, 96)
small = ImageFont.truetype(FONT, 26)

def centred(text, font, y, fill, spacing=0):
    widths = [d.textlength(ch, font=font) for ch in text]
    total = sum(widths) + spacing * (len(text) - 1)
    x = (W - total) / 2
    for ch, w in zip(text, widths):
        d.text((x, y), ch, font=font, fill=fill)
        x += w + spacing
    return total

# Steel gradient is faked with two offset passes: a dark base and a lighter face.
centred('SCARLET', big, 22, (86, 92, 104, 255), 14)
centred('SCARLET', big, 18, (226, 232, 240, 255), 14)
centred('HORIZON', mid, 170, (86, 92, 104, 255), 30)
centred('HORIZON', mid, 166, (198, 206, 218, 255), 30)

# The horizon itself: a thin scarlet line with a soft bloom, never a bloc colour.
for i, a in ((0, 210), (1, 150), (2, 90), (3, 45)):
    d.line([(150, 156 + i), (W - 150, 156 + i)], fill=(242, 46, 52, a))
    d.line([(150, 156 - i), (W - 150, 156 - i)], fill=(242, 46, 52, a))
d.ellipse([W/2 - 90, 150, W/2 + 90, 162], fill=(255, 150, 140, 34))

centred('АЛЬТЕРНАТИВНАЯ СОВРЕМЕННОСТЬ', small, 276, (150, 158, 172, 255), 8)

img.save('/tmp/T_RA4_Logo_source.png')
print('logo written 1024x320')
