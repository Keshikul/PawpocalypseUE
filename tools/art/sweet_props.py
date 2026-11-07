#!/usr/bin/env python3
"""Backyard props (tier_1), read off content/art/reference/sweet/bg_backyard.png.

WHY ONE BIOME AND NOT ALL TWELVE. Props are where a biome stops being a coloured
rectangle and becomes a place, and the vocabulary is different for each one - a
backyard is fences, pots and clover; the sewers are pipes and puddles. Doing one
properly is worth more than twelve sketched, and the backyard is the one every
player sees first.

WHAT THE REFERENCE ACTUALLY CONTAINS, in the order it reads: a plank fence along
the top, a pale stone edge along the bottom and right, terracotta planter
troughs and round pots with flowers, a blue watering can, a stack of grey
stones, a tipped paper bag, a white tub, clover tufts and grass strokes.

TWO RULES THAT COME FROM THE REFERENCE RATHER THAN FROM TASTE.

  * PROPS NEVER USE A CREATURE'S ACCENT COLOUR. Forty enemies move over this
    ground. A prop the same colour as a mouse is a prop the player will flinch
    at. Terracotta, stone grey and fence cream are deliberately outside the
    creature palette.
  * PROPS ARE FLATTER THAN CREATURES. No belly patches, no highlights, one
    shade at most. They sit still, so any detail on them is detail that is
    read forty times a minute and never changes.

Scatter props are 64px; the fence and paving are 64-wide tiles meant to repeat
along an edge, which is why they are drawn to butt against their own copies.

    py tools/sweet_props.py            # sheet -> build/sweet_props.png
    py tools/sweet_props.py --svg DIR  # write .svg files instead
"""

from __future__ import annotations

import argparse
import pathlib
import sys

try:
    from PIL import Image, ImageDraw
except ImportError as exc:                      # pragma: no cover
    sys.exit("needs Pillow: py -m pip install pillow (%s)" % exc)

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import palette as PAL
import sweet_shapes as S

INK = S.INK
P = 64                      # scatter props
TILE = 64                   # edge tiles

# Outside the creature palette on purpose - see the rules above.
TERRA = "#D99873"
TERRA_DARK = "#B9775A"
SOIL = "#8A6450"
STONE = "#DCD8CE"
STONE_DARK = "#BDB8AC"
FENCE = "#F0E3C2"
FENCE_DARK = "#D8C79E"
TIN = "#A8CADD"
TIN_DARK = "#86A9C2"
PAPER = "#EFE2C4"
TUB = "#F2F0E8"
LEAF = "#8FBF6B"
LEAF_DARK = "#6E9E52"
FLOWER_A = "#F2D27C"        # butter yellow
FLOWER_B = "#C9A2D6"        # muted lilac, not a candy pink
GRASS = PAL.BACKDROPS["tier_1_backyard"]["detail"]


def leafy(cx, cy, r, colour=LEAF):
    """A three-lobed clover, the reference's most repeated scatter element."""
    return S.fuse([S.ell(cx - r * 0.62, cy, r * 0.68, r * 0.62, colour),
                   S.ell(cx + r * 0.62, cy, r * 0.68, r * 0.62, colour),
                   S.ell(cx, cy - r * 0.62, r * 0.68, r * 0.62, colour)]) + [
        S.line([(cx, cy + r * 0.3), (cx, cy + r * 1.5)], INK, 2.0)]


def flower(cx, cy, r, petal=FLOWER_A):
    out = []
    for i in range(5):
        a = i * 1.2566
        out.append(S.ell(cx + r * 1.05 * __import__("math").cos(a),
                         cy + r * 1.05 * __import__("math").sin(a),
                         r * 0.62, r * 0.62, petal))
    out = S.fuse(out)
    out.append(S.ell(cx, cy, r * 0.5, r * 0.5, "#F6E9A8", s=False))
    return out


def PROPS():
    """Every prop here that is ARCHITECTURE uses rect; every prop that is ALIVE
    uses ellipses. That split is the whole reason the first pass failed - a
    fence drawn from ellipses is a row of arches, and paving drawn from ellipses
    is a cloud."""
    out = {}

    # --- living scatter: round ----------------------------------------------
    # Five blades rising out of one small mound. The first version was three
    # loose strokes with nothing at the bottom, so it read as scratches on the
    # ground rather than as a plant growing out of it.
    out["grass_tuft"] = S.fuse([S.ell(32, 48, 15, 6, LEAF)]) + [
        S.line([(22, 48), (16, 28)], LEAF_DARK, 3.0),
        S.line([(27, 47), (24, 22)], LEAF_DARK, 3.0),
        S.line([(32, 47), (33, 18)], LEAF_DARK, 3.0),
        S.line([(37, 47), (41, 23)], LEAF_DARK, 3.0),
        S.line([(42, 48), (48, 30)], LEAF_DARK, 3.0),
    ]

    # Veins split the fused union back into three leaves. Without them the
    # three-lobe silhouette is just a round bush, which is the shape a tree has.
    out["clover"] = leafy(32, 34, 11) + [
        S.line([(32, 36), (24, 28)], LEAF_DARK, 1.8),
        S.line([(32, 36), (40, 28)], LEAF_DARK, 1.8),
        S.line([(32, 36), (32, 44)], LEAF_DARK, 1.8),
    ]

    out["flower_pot"] = (
        S.fuse([S.rect(32, 44, 26, 22, TERRA, r=4)])
        + [S.rect(32, 33, 30, 7, TERRA_DARK, r=2, s=True)]
        + [S.rect(32, 31, 24, 4, SOIL, r=1, s=False),
           S.line([(32, 31), (32, 18)], LEAF_DARK, 2.4)]
        + flower(32, 14, 6))

    out["planter_box"] = (
        S.fuse([S.rect(32, 44, 54, 20, TERRA, r=3)])
        + [S.rect(32, 35, 58, 7, TERRA_DARK, r=2, s=True)]
        + [S.rect(32, 33, 50, 4, SOIL, r=1, s=False)]
        + [S.line([(16, 33), (16, 22)], LEAF_DARK, 2.2),
           S.line([(32, 33), (32, 20)], LEAF_DARK, 2.2),
           S.line([(48, 33), (48, 22)], LEAF_DARK, 2.2)]
        + flower(16, 19, 5) + flower(32, 17, 5, FLOWER_B) + flower(48, 19, 5))

    out["sprout"] = (
        S.fuse([S.rect(32, 46, 16, 14, TERRA, r=3)])
        + [S.rect(32, 39, 19, 5, TERRA_DARK, r=2, s=True),
           S.line([(32, 38), (32, 28)], LEAF_DARK, 2.2)]
        + S.fuse([S.ell(26, 26, 6, 4, LEAF), S.ell(38, 26, 6, 4, LEAF)]))

    # --- objects: flat edges -------------------------------------------------
    # Body, a spout that TAPERS out of the body wall, a rose on the end, and a
    # handle arch over the top. The first version had the spout as a loose ink
    # curve floating off the side, which read as a crack.
    out["watering_can"] = (
        S.fuse([S.rect(34, 42, 32, 28, TIN, r=7),
                S.tri([(20, 34), (20, 44), (4, 26)], TIN),
                S.rect(34, 20, 26, 7, TIN, r=3)])
        + [S.rect(34, 28, 36, 6, TIN_DARK, r=2),
           S.rect(6, 24, 11, 8, TIN_DARK, r=3)]
        + [S.line([(34, 22), (34, 16)], INK, 2.0),
           S.line([(44, 36), (44, 52)], TIN_DARK, 2.0)])

    out["stone_stack"] = S.fuse([
        S.rect(22, 46, 26, 13, STONE, r=5),
        S.rect(44, 45, 22, 12, STONE, r=5),
        S.rect(33, 33, 28, 13, STONE, r=5),
    ]) + [S.line([(21, 39), (54, 39)], STONE_DARK, 2.0),
          S.line([(33, 51), (52, 51)], STONE_DARK, 2.0)]

    # A grocery bag is a box with a FOLDED-OVER TOP and creases down the front.
    # Drawing it as one rounded rectangle with a stripe made a picture frame.
    out["paper_bag"] = (
        S.fuse([S.rect(32, 44, 30, 30, PAPER, r=2),
                S.rect(32, 26, 34, 10, PAPER, r=2)])
        + [S.rect(32, 26, 34, 10, PAPER, r=2),
           S.line([(32, 21), (32, 31)], S.shade(PAPER, 0.22), 1.8),
           S.line([(22, 33), (22, 58)], S.shade(PAPER, 0.18), 1.8),
           S.line([(42, 33), (42, 58)], S.shade(PAPER, 0.18), 1.8)])

    out["tub"] = (
        S.fuse([S.rect(32, 44, 26, 26, TUB, r=3)])
        + [S.rect(32, 30, 30, 6, STONE_DARK, r=2, s=True)])

    # --- edge tiles: butt against their own copies ---------------------------
    # Planks with a small gap, and two rails crossing them. Drawn so the LAST
    # plank of one tile and the FIRST of the next leave the same gap as the
    # ones inside, which is the only thing that makes a tile a tile.
    out["fence_tile"] = S.fuse([
        S.rect(8, 34, 13, 44, FENCE, r=3), S.rect(24, 34, 13, 44, FENCE, r=3),
        S.rect(40, 34, 13, 44, FENCE, r=3), S.rect(56, 34, 13, 44, FENCE, r=3),
    ]) + [S.rect(32, 24, 64, 5, FENCE_DARK, r=1, s=False),
          S.rect(32, 46, 64, 5, FENCE_DARK, r=1, s=False)]

    # Running bond, two courses, both fully inside the 64px tile - the first
    # version put its bottom course at cy=58 with h=18, so a third of it was
    # sliced off by the canvas edge and the tile read as two blobs.
    out["paving_tile"] = S.fuse([
        S.rect(16, 24, 30, 20, STONE, r=3),
        S.rect(48, 24, 30, 20, STONE, r=3),
        S.rect(0, 48, 30, 20, STONE, r=3),
        S.rect(32, 48, 30, 20, STONE, r=3),
        S.rect(64, 48, 30, 20, STONE, r=3),
    ]) + [S.line([(32, 15), (32, 33)], STONE_DARK, 2.2),
          S.line([(16, 39), (16, 57)], STONE_DARK, 2.2),
          S.line([(48, 39), (48, 57)], STONE_DARK, 2.2),
          S.line([(2, 36), (62, 36)], STONE_DARK, 2.2)]

    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--svg", metavar="DIR")
    args = ap.parse_args()
    props = PROPS()

    if args.svg:
        out = pathlib.Path(args.svg); out.mkdir(parents=True, exist_ok=True)
        for name, parts in props.items():
            (out / (name + ".svg")).write_text(S.to_svg(parts, P), encoding="utf-8")
            print("  %s.svg" % name)
        return 0

    cols, cell = 6, 128
    rows = (len(props) + cols - 1) // cols
    field = PAL.BACKDROPS["tier_1_backyard"]["field"]
    sheet = Image.new("RGB", (cols * cell, rows * cell + 210), field)
    d = ImageDraw.Draw(sheet)
    for i, (name, parts) in enumerate(props.items()):
        cx, cy = (i % cols) * cell, (i // cols) * cell + 30
        img = S.to_image(parts, P).resize((104, 104), Image.LANCZOS)
        sheet.paste(img, (cx + 12, cy + 4), img)
        d.text((cx + 10, cy + 108), name, fill=PAL.INK)

    # the two tiles repeated, which is the only way to see whether they tile
    y = rows * cell + 52
    d.text((10, y - 18), "fence and paving, tiled - the point of a tile is the seam",
           fill=PAL.INK)
    for k in range(10):
        f = S.to_image(PROPS()["fence_tile"], P)
        sheet.paste(f, (10 + k * 64, y), f)
    for k in range(10):
        p = S.to_image(PROPS()["paving_tile"], P)
        sheet.paste(p, (10 + k * 64, y + 72), p)

    out = pathlib.Path("build/sweet_props.png")
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out)
    print("wrote %s (%dx%d)" % (out, sheet.width, sheet.height))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
