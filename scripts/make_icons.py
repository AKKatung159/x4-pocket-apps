#!/usr/bin/env python3
"""Writes apps/<id>/icon.bmp: the hub icon of every app, 32x32 1-bit, in the solid style of the
X4 home icons (crosspoint-x4 scripts/personal/home_icons.py): drawn 8x supersampled on a 48-unit
grid, then thresholded. Needs Pillow. Run it after changing a design; the BMPs are committed."""
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

SIZE, S = 32, 8
APPS = Path(__file__).resolve().parents[1] / "apps"


def canvas():
    im = Image.new("L", (SIZE * S, SIZE * S), 0)
    return im, ImageDraw.Draw(im), SIZE * S / 48


def finish(im):
    return im.resize((SIZE, SIZE), Image.LANCZOS).point(lambda v: 255 if v >= 128 else 0)


def font(px):
    try:
        return ImageFont.load_default(size=px)
    except TypeError:  # Pillow < 10.1
        return ImageFont.load_default()


def g2048():  # four solid tiles with the corner one knocked out to a "2"
    im, d, u = canvas()
    for x, y in ((3, 3), (25, 3), (3, 25), (25, 25)):
        d.rounded_rectangle([x * u, y * u, (x + 20) * u, (y + 20) * u], 4 * u, fill=255)
    d.text((13 * u, 13 * u), "2", font=font(int(19 * u)), fill=0, anchor="mm")
    d.text((35 * u, 35 * u), "4", font=font(int(19 * u)), fill=0, anchor="mm")
    return finish(im)


def minesweeper():  # a mine: solid disc, four short spikes and a fuse-free knocked-out glint
    im, d, u = canvas()
    c = 24 * u
    for i in range(4):
        a = i * math.pi / 4
        dx, dy = 21 * u * math.cos(a), 21 * u * math.sin(a)
        d.line([(c - dx, c - dy), (c + dx, c + dy)], fill=255, width=int(4 * u))
    d.ellipse([c - 15 * u, c - 15 * u, c + 15 * u, c + 15 * u], fill=255)
    d.ellipse([c - 9 * u, c - 9 * u, c - 1 * u, c - 1 * u], fill=0)
    return finish(im)


def nonogram():  # a 4x4 picross grid: the picture's cells solid, the empty ones outlined
    im, d, u = canvas()
    picture = ("#..#", "####", ".##.", "#..#")
    cell, x0 = 11, 2
    for r, row in enumerate(picture):
        for c, ch in enumerate(row):
            x, y = (x0 + c * cell) * u, (x0 + r * cell) * u
            box = [x + u, y + u, x + (cell - 1) * u, y + (cell - 1) * u]
            if ch == "#":
                d.rectangle(box, fill=255)
            else:
                d.rectangle(box, outline=255, width=int(2 * u))
    return finish(im)


def star(d, cx, cy, r_out, r_in, fill):
    pts = []
    for i in range(10):
        a = -math.pi / 2 + i * math.pi / 5
        r = r_out if i % 2 == 0 else r_in
        pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    d.polygon(pts, fill=fill)


def tarot():  # an upright card with a star knocked out
    im, d, u = canvas()
    d.rounded_rectangle([10 * u, 2 * u, 38 * u, 46 * u], 4 * u, fill=255)
    star(d, 24 * u, 22 * u, 11 * u, 4.5 * u, 0)
    d.rectangle([15 * u, 37 * u, 33 * u, 40 * u], fill=0)
    return finish(im)


def flashcards():  # two solid stacked cards, a gap knocked out between them, text lines on the front
    im, d, u = canvas()
    d.rounded_rectangle([14 * u, 3 * u, 46 * u, 29 * u], 4 * u, fill=255)
    d.rounded_rectangle([0 * u, 14 * u, 38 * u, 45 * u], 6 * u, fill=0)  # the gap
    d.rounded_rectangle([3 * u, 17 * u, 35 * u, 42 * u], 4 * u, fill=255)
    d.rectangle([9 * u, 25 * u, 29 * u, 29 * u], fill=0)
    d.rectangle([9 * u, 33 * u, 22 * u, 36 * u], fill=0)
    return finish(im)


def stats():  # three solid rising bars on a base line (the home stats icon)
    im, d, u = canvas()
    d.rounded_rectangle([5 * u, 26 * u, 15 * u, 42 * u], 2 * u, fill=255)
    d.rounded_rectangle([19 * u, 16 * u, 29 * u, 42 * u], 2 * u, fill=255)
    d.rounded_rectangle([33 * u, 6 * u, 43 * u, 42 * u], 2 * u, fill=255)
    d.rectangle([3 * u, 43 * u, 45 * u, 46 * u], fill=255)
    return finish(im)


def dashboard():  # a gauge: solid half-disc, a knocked-out needle and hub
    im, d, u = canvas()
    d.pieslice([2 * u, 8 * u, 46 * u, 52 * u], 180, 360, fill=255)
    d.pieslice([10 * u, 16 * u, 38 * u, 44 * u], 180, 360, fill=0)
    d.line([(24 * u, 30 * u), (35 * u, 14 * u)], fill=255, width=int(4 * u))
    d.ellipse([19 * u, 25 * u, 29 * u, 35 * u], fill=255)
    d.rounded_rectangle([2 * u, 36 * u, 46 * u, 44 * u], 3 * u, fill=255)
    return finish(im)


ICONS = {"2048": g2048, "minesweeper": minesweeper, "nonogram": nonogram, "tarot": tarot,
         "flashcards": flashcards, "stats": stats, "dashboard": dashboard}

if __name__ == "__main__":
    for app, draw in ICONS.items():
        mask = draw()  # white = shape
        icon = Image.eval(mask, lambda v: 255 - v).convert("1")  # black ink on white
        icon.save(APPS / app / "icon.bmp")
        print("wrote", APPS / app / "icon.bmp")
