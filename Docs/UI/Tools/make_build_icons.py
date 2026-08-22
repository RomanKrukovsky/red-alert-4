# -*- coding: utf-8 -*-
"""Schematic build-card icons.

The cards used photoreal unit renders left from the retired roster, so a
barracks card showed a rifleman and a refinery card showed a tank. The reference
uses small schematic silhouettes instead, which also stay readable at card size.
One neutral sheet is drawn per structure and tinted per direction at runtime.
"""
from PIL import Image, ImageDraw

S = 128
FG = (226, 232, 240, 255)
DIM = (226, 232, 240, 140)

def new():
    return Image.new('RGBA', (S, S), (0, 0, 0, 0))

def headquarters():
    im = new(); d = ImageDraw.Draw(im)
    d.polygon([(20, 100), (20, 52), (64, 26), (108, 52), (108, 100)], outline=FG, width=4)
    d.rectangle([52, 66, 76, 100], outline=FG, width=3)
    d.line([(64, 26), (64, 10)], fill=FG, width=3)
    d.ellipse([58, 4, 70, 16], outline=FG, width=3)
    return im

def power():
    im = new(); d = ImageDraw.Draw(im)
    d.rectangle([26, 46, 102, 102], outline=FG, width=4)
    d.polygon([(70, 52), (48, 78), (62, 78), (56, 96), (80, 68), (66, 68)], fill=FG)
    d.line([(40, 46), (40, 24)], fill=FG, width=4)
    d.line([(88, 46), (88, 24)], fill=FG, width=4)
    return im

def refinery():
    im = new(); d = ImageDraw.Draw(im)
    d.rectangle([20, 62, 108, 104], outline=FG, width=4)
    d.ellipse([32, 30, 66, 64], outline=FG, width=4)
    d.rectangle([76, 34, 96, 64], outline=FG, width=4)
    d.line([(20, 84), (108, 84)], fill=DIM, width=2)
    return im

def barracks():
    im = new(); d = ImageDraw.Draw(im)
    d.polygon([(18, 100), (18, 60), (64, 34), (110, 60), (110, 100)], outline=FG, width=4)
    d.rectangle([54, 74, 74, 100], outline=FG, width=3)
    d.line([(30, 72), (98, 72)], fill=DIM, width=2)
    return im

def factory():
    im = new(); d = ImageDraw.Draw(im)
    d.polygon([(16, 102), (16, 62), (46, 78), (46, 62), (76, 78), (76, 62), (112, 62), (112, 102)],
              outline=FG, width=4)
    d.rectangle([88, 30, 104, 62], outline=FG, width=3)
    return im

def radar():
    im = new(); d = ImageDraw.Draw(im)
    d.rectangle([44, 84, 84, 106], outline=FG, width=4)
    d.line([(64, 84), (64, 58)], fill=FG, width=4)
    d.arc([26, 18, 102, 94], start=200, end=340, fill=FG, width=5)
    d.line([(64, 58), (94, 34)], fill=FG, width=3)
    return im

def emp():
    im = new(); d = ImageDraw.Draw(im)
    d.rectangle([48, 76, 80, 106], outline=FG, width=4)
    d.line([(64, 76), (64, 44)], fill=FG, width=4)
    for r, w in ((16, 4), (28, 3), (40, 2)):
        d.arc([64 - r, 44 - r, 64 + r, 44 + r], start=200, end=340, fill=FG, width=w)
    return im

def silo():
    im = new(); d = ImageDraw.Draw(im)
    d.rectangle([28, 70, 100, 106], outline=FG, width=4)
    d.polygon([(64, 14), (78, 46), (78, 70), (50, 70), (50, 46)], outline=FG, width=4)
    d.line([(28, 88), (100, 88)], fill=DIM, width=2)
    return im

ICONS = {
    'HQ': headquarters, 'Power': power, 'Refinery': refinery, 'Barracks': barracks,
    'Factory': factory, 'Radar': radar, 'Emp': emp, 'Silo': silo,
}
for name, fn in ICONS.items():
    out = '/tmp/T_RA4_Icon_%s.png' % name
    fn().save(out)
    print(out)
