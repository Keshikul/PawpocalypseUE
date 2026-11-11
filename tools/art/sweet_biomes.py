#!/usr/bin/env python3
"""The twelve biome floors and their thirty-six props, in the sweet palette.

WHY THIS IS A SEPARATE TOOL FROM make_sweet_art.py. Everything that tool draws
is a CHARACTER: one object, centred in its own box, seen against whatever is
behind it. A biome floor is the opposite - it is a surface with no subject, it
has to repeat forever without showing a join, and its entire job is to be
IGNORED. The mice are what the player is aiming at; a floor interesting enough
to look at costs exactly the attention the mice need.

WHAT WAS THERE BEFORE. Nothing, for the classic set. BiomeBackdrop drew each
biome as a vertical GRADIENT WASH between two dark colours - tier_1 was
#367519 down to #163807 - plus a scatter of low-alpha polygons. Two problems
with that, and the second is the interesting one:

  * The designer ruled out background gradients on 2026-09-10 ("we do not need
    a gradient in the backgrounds because it just makes stuff look worse"), and
    palette.py has said "One flat field, one flat detail. NO GRADIENTS" ever
    since.
  * THE GAME'S FLOORS AND THE PALETTE'S FLOORS WERE DIFFERENT COLOURS. Every
    creature in the game was checked for contrast against palette.BACKDROPS,
    where tier_1 is pale mint #A3F593. The game was drawing dark green. The
    checks were true about a floor the player never saw.

Generating a floor texture fixes both at once, because BiomeBackdrop already
says "an image floor replaces the wash outright rather than sitting on top of
it" - so a backdrop existing is what turns the gradient off.

TWO FORMATS, ON PURPOSE.
  * FLOORS ARE PNG. They tile, and ci/check_textures.gd measures the seam by
    opening PNGs out of a style's backdrops folder. An SVG floor would be
    untestable by the gate that exists to catch exactly the mistake floors
    make.
  * PROPS ARE SVG, like every other object in the game, because they are drawn
    at a size the scene picks.

FLOORS CARRY NO INK OUTLINE. Every creature in this game is outlined, and that
is what keeps forty of them legible at once. Outlining the floor too would put
the same weight of line under the thing it is supposed to sit behind. Floor
detail is flat, low-contrast, and close in value to the field.

    py tools/sweet_biomes.py                 # sheets -> build/, writes nothing
    py tools/sweet_biomes.py --write         # writes assets/art/
    py tools/sweet_biomes.py --seams         # just the seam report
"""

from __future__ import annotations

import argparse
import math
import pathlib
import random
import sys

try:
    from PIL import Image, ImageDraw
except ImportError as exc:                      # pragma: no cover
    sys.exit("needs Pillow: py -m pip install pillow (%s)" % exc)

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import palette as PAL
import sweet_shapes as S

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
# Since the 2026-09-11 restructure these are plain game art, not a style.
# PORT (Unreal, 2026-11-05): copied from the Godot repo's tools/, unchanged except the output paths: the art goes to
# SourceArt/svg/<Godot's assets/art folder>, or to $PAW_ART_ROOT when set (tools/art/check_art.py).
import os
STYLE = pathlib.Path(os.environ.get("PAW_ART_ROOT", ROOT / "SourceArt" / "svg"))

TILE = 512              # floors, square, seamless
PROP = 128              # props, the same box every other object uses

# tier id -> palette key. The two naming schemes have to meet somewhere and this
# is the only place they do.
TIERS = {
    "tier_1": "tier_1_backyard", "tier_2": "tier_2_alley",
    "tier_3": "tier_3_park", "tier_4": "tier_4_rooftops",
    "tier_5": "tier_5_sewers", "tier_6": "tier_6_turf",
    "tier_7": "tier_7_portal", "tier_8": "tier_8_mythic",
    "tier_9": "tier_9_kitchen", "tier_10": "tier_10_vets",
    "tier_11": "tier_11_factory", "tier_12": "tier_12_litterbox",
}


def _rgb(hex_colour):
    return tuple(int(hex_colour[i:i + 2], 16) for i in (1, 3, 5))


def _rgb_hex(c):
    """_mix works in hex; this exists so a caller can pass either."""
    return c


def _mix(a, b, t):
    ra, rb = _rgb(a), _rgb(b)
    return tuple(int(round(ra[i] + (rb[i] - ra[i]) * t)) for i in range(3))


# ---------------------------------------------------------------- floors ----
# EVERY FLOOR IS DRAWN THROUGH wrapped(), which is the whole seamlessness
# story: a shape near an edge is drawn again at the opposite edge, so the tile's
# left column continues into its own right column. There is no post-process and
# no blending - the tile is seamless because it was drawn seamless.

def wrapped(draw_one, x, y, reach):
    """Call draw_one at (x, y) and at every wrap of it that the tile can see."""
    for dx in (-TILE, 0, TILE):
        for dy in (-TILE, 0, TILE):
            if abs(x + dx - TILE / 2) < TILE / 2 + reach and \
               abs(y + dy - TILE / 2) < TILE / 2 + reach:
                draw_one(x + dx, y + dy)


def floor(tier):
    key = TIERS[tier]
    field = PAL.BACKDROPS[key]["field"]
    detail = PAL.BACKDROPS[key]["detail"]
    # A third tone, halfway between, so a floor has two levels of texture
    # without ever approaching the contrast a creature has.
    faint = _mix(field, detail, 0.45)
    img = Image.new("RGB", (TILE, TILE), _rgb(field))
    d = ImageDraw.Draw(img)
    rng = random.Random("floor:" + tier)
    dark = _rgb(detail)

    def blade(x, y, s_=1.0, colour=None):
        d.line([(x, y), (x + rng.uniform(-9, 9) * s_, y - 34 * s_)],
               fill=colour or faint, width=max(2, int(5 * s_)))

    def dot(x, y, r, colour):
        d.ellipse([x - r, y - r, x + r, y + r], fill=colour)

    def brick(x, y, w, h, colour):
        d.rounded_rectangle([x - w / 2, y - h / 2, x + w / 2, y + h / 2],
                            radius=6, fill=colour)

    def ring(x, y, r, colour, wd=6):
        d.ellipse([x - r, y - r, x + r, y + r], outline=colour, width=wd)

    def star(x, y, r, colour):
        pts = []
        for i in range(10):
            a = i * math.pi / 5 - math.pi / 2
            rr = r if i % 2 == 0 else r * 0.42
            pts.append((x + math.cos(a) * rr, y + math.sin(a) * rr))
        d.polygon(pts, fill=colour)

    def paw(x, y, r, colour):
        dot(x, y + r * 0.45, r * 0.78, colour)
        for ax, ay in ((-0.95, -0.6), (-0.34, -0.95), (0.34, -0.95), (0.95, -0.6)):
            dot(x + ax * r, y + ay * r, r * 0.3, colour)

    # A UNIVERSAL GRAIN PASS, before any motif.
    #
    # It is there for looks - a perfectly flat colour field bands on a large
    # screen - but the reason it is MANDATORY is the seam test. That test scores
    # the edge mismatch as a MULTIPLE of the texture's own neighbour-to-neighbour
    # contrast, so a floor with a perfectly flat interior has a denominator of
    # nearly zero and any edge difference at all, however invisible, divides into
    # it and explodes. tier_5's ripples scored 9.0 that way with a join no human
    # could see: its middle row happened to fall between two arcs.
    #
    # Grain everywhere means the denominator is real everywhere, which makes the
    # ratio mean what it says.
    grain = _mix(_rgb_hex(field), _rgb_hex(detail), 0.16)
    for _ in range(2600):
        gx, gy = rng.uniform(0, TILE), rng.uniform(0, TILE)
        gr = rng.uniform(1.6, 3.4)
        wrapped(lambda px, py, gr=gr: dot(px, py, gr, grain), gx, gy, 6)

    # TWO RULES, BOTH LEARNED BY MEASURING A BROKEN TILE.
    #
    #  1. NO RANDOMNESS INSIDE A WRAPPED CALLBACK. wrapped() calls its function
    #     up to nine times for the nine positions a shape can occupy, so an
    #     rng call inside it gives every copy DIFFERENT values - the copy at the
    #     left edge stops matching the copy at the right edge and the wrap is
    #     undone by the very code meant to create it. tier_1's grass measured a
    #     vertical seam 7372x the inner contrast because of exactly this. Every
    #     random value is drawn OUTSIDE and closed over.
    #
    #  2. EVERY GRID PITCH MUST DIVIDE THE TILE. A 52px pitch across 512px is
    #     9.85 cells: the pattern restarts mid-cell at the edge and the join is
    #     a hard line no amount of wrapping can fix. tier_4 (52) and tier_11
    #     (104) both failed on this. Pitches are divisors of TILE now.
    if tier == "tier_1":                       # grass
        for _ in range(340):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            sc, lean = rng.uniform(0.6, 1.2), rng.uniform(-9, 9)
            wrapped(lambda px, py, sc=sc, lean=lean: d.line(
                [(px, py), (px + lean * sc, py - 34 * sc)],
                fill=faint, width=max(2, int(5 * sc))), x, y, 46)
    elif tier == "tier_2":                     # cobbles, running bond
        for row in range(8):                   # 64 divides 512
            for col in range(8):
                x = col * 64 + (32 if row % 2 else 0)
                y = row * 64 + 32
                wrapped(lambda px, py: brick(px, py, 56, 52, faint), x, y, 44)
    elif tier == "tier_3":                     # leaves
        for _ in range(180):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            r = rng.uniform(9, 18)
            wrapped(lambda px, py, r=r: d.ellipse(
                [px - r * 1.7, py - r, px + r * 1.7, py + r], fill=faint), x, y, 40)
    elif tier == "tier_4":                     # roof tiles
        for row in range(8):                   # 64 divides 512
            for col in range(8):
                x = col * 64 + (32 if row % 2 else 0)
                y = row * 64 + 32
                wrapped(lambda px, py: d.pieslice(
                    [px - 32, py - 30, px + 32, py + 34], 0, 180, fill=faint), x, y, 46)
    elif tier == "tier_5":                     # ripples
        # SCATTERED, NOT RANKED. A strict 32px row grid measured 2.65 on the
        # vertical seam - over the gate's limit of 2.0 - because a rigidly
        # periodic texture makes the seam test's denominator depend on exactly
        # where the sampled row falls between rows. Free placement plus wrapping
        # is seamless by construction and has no preferred row at all.
        for _ in range(58):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            w_ = rng.uniform(52, 78)
            wrapped(lambda px, py, w_=w_: d.arc(
                [px - w_, py - w_ * 0.32, px + w_, py + w_ * 0.32], 0, 180,
                fill=faint, width=7), x, y, 100)
    elif tier == "tier_6":                     # paw prints
        for _ in range(60):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            r = rng.uniform(12, 20)
            wrapped(lambda px, py, r=r: paw(px, py, r, faint), x, y, 46)
    elif tier == "tier_7":                     # rings
        for _ in range(40):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            r = rng.uniform(18, 52)
            wrapped(lambda px, py, r=r: ring(px, py, r, faint), x, y, 62)
    elif tier == "tier_8":                     # stars
        for _ in range(120):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            r = rng.uniform(6, 16)
            wrapped(lambda px, py, r=r: star(px, py, r, faint), x, y, 24)
    elif tier == "tier_9":                     # kitchen tiles
        for row in range(8):                   # 64 divides 512
            for col in range(8):
                wrapped(lambda px, py: brick(px, py, 56, 56, faint),
                        col * 64 + 32, row * 64 + 32, 44)
    elif tier == "tier_10":                    # fur wisps
        for _ in range(260):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            a = rng.uniform(0, 6.283)
            wrapped(lambda px, py, a=a: d.line(
                [(px, py), (px + math.cos(a) * 26, py + math.sin(a) * 26)],
                fill=faint, width=4), x, y, 34)
    elif tier == "tier_11":                    # plate and rivets
        for row in range(4):                   # 128 divides 512
            for col in range(4):
                x, y = col * 128 + 64, row * 128 + 64
                wrapped(lambda px, py: brick(px, py, 116, 116, faint), x, y, 84)
                for ox, oy in ((-46, -46), (46, -46), (-46, 46), (46, 46)):
                    wrapped(lambda px, py, ox=ox, oy=oy: dot(px + ox, py + oy, 5, dark),
                            x, y, 84)
    else:                                      # tier_12, litter granules
        for _ in range(900):
            x, y = rng.uniform(0, TILE), rng.uniform(0, TILE)
            r = rng.uniform(3, 8)
            c = faint if rng.random() < 0.75 else dark
            wrapped(lambda px, py, r=r, c=c: dot(px, py, r, c), x, y, 12)
    return img



# ----------------------------------------------------------------- props ----
# THE NAMES ARE NOT MINE TO CHOOSE. BiomeBackdrop.BIOME_PROPS has declared these
# thirty-six stems since 2026-08-29 - two scatter props and one edge prop per
# biome - and the famine style already ships art for every one of them. A prop
# under any other name is a prop the game will not load.
#
# SCATTER vs EDGE is a real distinction, not a label. Scatter props are strewn
# across the floor under the fight, so they are SMALL and LOW and must never be
# mistakable for a target. Edge props frame the screen from outside the play
# area, so they can be tall and heavy - the framing is kept in props rather than
# painted into the floor precisely so it survives an aspect-ratio change.
#
# ARCHITECTURE USES rect, LIVING THINGS USE ELLIPSES. The same rule the first
# prop pass learned the hard way: a fence built from ellipses is a row of
# arches, and paving built from ellipses is a cloud.

INK = S.INK

# Prop colours per biome. DELIBERATELY OUTSIDE THE CREATURE PALETTE - forty
# enemies move over this ground and a prop the colour of a mouse is a prop the
# player flinches at - and each one is checked against its own floor below.
# Every prop colour is a Stacklands family face or header (palette.py), picked
# so the pair reads as the material and clears its own floor. The alley's bin
# and the rooftops' vent are the slate FACE rather than the header because the
# header is the colour of a pale cool floor - measured 17 and 20 from them.
PROP_COLOURS = {
    "tier_1":  (PAL.header("green"), PAL.header("slate")),   # leaf, stone
    "tier_2":  (PAL.face("red"), PAL.face("slate")),         # brick, metal
    "tier_3":  (PAL.header("green"), PAL.face("orange")),    # foliage, bark
    "tier_4":  (PAL.face("slate"), PAL.face("coral")),       # steel, clay
    "tier_5":  (PAL.header("slate"), PAL.header("green")),   # pipe, water
    "tier_6":  (PAL.face("orange"), PAL.face("red")),        # timber, bowl
    "tier_7":  (PAL.header("blue"), PAL.header("yellow")),   # shard, gold
    "tier_8":  (PAL.header("yellow"), PAL.header("blue")),   # gold, crystal
    "tier_9":  (PAL.face("orange"), PAL.header("slate")),    # crust, appliance
    "tier_10": (PAL.header("slate"), PAL.header("blue")),    # steel, cloth
    "tier_11": (PAL.header("slate"), PAL.header("orange")),  # machine, kibble
    "tier_12": (PAL.header("cream"), PAL.header("green")),   # litter, clump
}


def check_prop_colours():
    """Same rule the creatures live under: a prop has to be visible on the floor
    it stands on. This has caught three colours already in this project - a
    green clover on green grass, a butter crumb on a butter kitchen floor, a
    grey bolt on a warm factory floor - so it runs before anything is written."""
    bad = []
    for tier, pair in PROP_COLOURS.items():
        ground = PAL.BACKDROPS[TIERS[tier]]["field"]
        for c in pair:
            dist = PAL.distance(c, ground)
            if dist < PAL.MIN_FROM_GROUND:
                bad.append("%s prop %s is %.0f from its floor %s (min %.0f)"
                           % (tier, c, dist, ground, PAL.MIN_FROM_GROUND))
    return bad


# R5, THE PROPS REDRAWN (designer approved 2026-11-10). The note the enemies
# got applies here too: "most look like indistinct blobs... it needs a bit more
# anatomy and shape". Each prop has a silhouette that says what it is, a second
# level of drawing (a facet, a seam, a grain, a bolt) at a contour weight lighter
# than the outline, and one light. The 36 stems, their ideas and their colours
# (PROP_COLOURS, palette.PROPS) stayed. Scatter props (78 px on screen) stay SMALL
# and LOW, never a target; the edge props (190 px) carry more. Nothing in a prop
# may read as a face or a creature: a print on the bowl read as a mouth, a swirl
# in the ring as a face, strands of fur as a dead bird; all three were taken out.
C = PROP_COLOURS
PP = PAL.PROPS
LINE = 2.2                  # a contour inside a prop: lighter than the outline (3.1)


def r_(cx, cy, w, h, c, rr=3.0, s=True):
    return S.rect(cx, cy, w, h, c, r=rr, s=s)


def poly(pts, c, s=True):
    return S.tri(pts, c, s)


def ink(pts, w=LINE):
    return S.line(pts, INK, w)


def arc(p0, p1, p2, w=LINE, colour=None):
    return S.line(S.bez(p0, p1, p2, 12), colour or INK, w)


def light(cx, cy, rx, ry, c, amount=0.4, angle=0.0):
    return S.oval(cx, cy, rx, ry, angle, S.tint(c, amount), s=False)


def dots(pts, r, c):
    return [S.ell(x, y, r, r, c, s=False) for x, y in pts]


def ring_pts(cx, cy, r, n, a0=0.0, jitter=None):
    out = []
    for i in range(n):
        a = a0 + 2 * math.pi * i / n
        rr = r * (jitter[i % len(jitter)] if jitter else 1.0)
        out.append((cx + math.cos(a) * rr, cy + math.sin(a) * rr))
    return out


# --- tier 1, the Backyard: leaf, stone ---------------------------------------------------------------------------------

def grass():
    a, b = C["tier_1"]
    blades = [S.tube(S.bez((x0, 96), (x0 + bend * 0.4, 70), (x0 + bend, top)), 9, 1.5, a)
              for x0, bend, top in ((46, -14, 52), (56, -4, 38), (66, 6, 44), (76, 16, 56), (52, 22, 62))]
    parts = S.fuse([S.ell(62, 96, 24, 7, a)] + blades)
    parts += [S.tube(S.bez((57, 94), (55, 70), (53, 42)), 3, 1, S.tint(a, 0.35), s=False)]   # one blade catches the light
    return parts


def rock():
    a, b = C["tier_1"]
    body = [poly([(30, 100), (34, 76), (50, 62), (74, 64), (88, 80), (90, 100)], b),
            S.ell(96, 98, 12, 9, b)]                                                       # a pebble that broke off it
    parts = S.fuse(body)
    parts += [poly([(38, 76), (52, 66), (72, 68), (66, 80), (44, 82)], S.tint(b, 0.35), s=False)]   # the flat top
    parts += [ink([(66, 80), (72, 92), (68, 100)]), ink([(44, 82), (40, 92)])]                      # its facets
    parts += [ink([(56, 70), (60, 76)], 1.8)]                                                      # a crack
    return parts


def bush():
    a, b = C["tier_1"]
    lobes = [(36, 82, 22, 20), (60, 64, 24, 22), (88, 70, 24, 22), (100, 92, 18, 16), (64, 92, 34, 18), (28, 98, 16, 12)]
    parts = S.fuse([S.ell(x, y, rx, ry, a) for x, y, rx, ry in lobes])
    parts += [S.ell(64, 104, 40, 8, S.shade(a, 0.25), s=False)]                     # the shade under it
    parts += [arc((40, 78), (50, 70), (60, 74)), arc((70, 70), (80, 60), (92, 66)), arc((80, 90), (90, 82), (100, 86))]
    parts += [light(56, 58, 10, 6, a, 0.4, -0.3), light(30, 78, 7, 5, a, 0.35, -0.3)]
    parts += S.fuse([S.ell(x, y, 3.6, 3.6, PP["flowers"][0]) for x, y in ((48, 86), (76, 80), (94, 96))])   # berries
    return parts


# --- tier 2, the Alley: brick, metal -----------------------------------------------------------------------------------

def brick():
    a, b = C["tier_2"]
    body = [r_(52, 94, 56, 20, a, 3), poly([(84, 92), (106, 92), (108, 104), (100, 108), (84, 106)], a)]
    parts = S.fuse(body)
    parts += [r_(52, 88, 50, 6, S.tint(a, 0.3), 2, False)]                           # the top face
    parts += [ink([(38, 86), (38, 103)]), ink([(66, 86), (66, 103)])]               # its frogs (the brick's marks)
    parts += [ink([(28, 100), (34, 96)], 1.6)]                                      # a chip off the corner
    parts += [poly([(87, 94), (104, 94), (102, 98), (88, 98)], S.tint(a, 0.3), s=False), ink([(100, 96), (102, 104)], 1.6)]
    return parts


def bag():
    a, b = C["tier_2"]
    c = PP["bag"][1]
    body = [S.ell(64, 92, 38, 26, c), S.ell(62, 70, 28, 22, c), S.ell(64, 50, 11, 10, c),
            poly([(52, 44), (58, 22), (64, 40)], c), poly([(64, 40), (74, 24), (78, 44)], c)]   # the tied ears
    parts = S.fuse(body)
    parts += [S.line([(52, 54), (76, 54)], S.shade(c, 0.35), 5.0)]                 # the tie
    parts += [arc((42, 74), (52, 92), (46, 108)), arc((80, 70), (76, 88), (86, 104)), arc((60, 62), (62, 72), (58, 80))]
    parts += [poly([(88, 96), (98, 90), (96, 102)], S.shade(c, 0.4))]               # a tear
    parts += [light(48, 84, 9, 6, c, 0.28, -0.4)]
    return parts


def bin_():
    a, b = C["tier_2"]
    body = [poly([(30, 50), (98, 50), (92, 116), (36, 116)], b),                   # a tapered bin
            r_(64, 46, 82, 12, b, 5)]
    parts = S.fuse(body)
    for x in (44, 56, 72, 84):                                                      # the ribs
        parts += [ink([(x, 58), (x + (x - 64) * 0.06, 110)])]
    parts += [r_(64, 46, 82, 5, S.shade(b, 0.28), 2, False), ink([(30, 52), (98, 52)])]
    parts += S.fuse([r_(64, 36, 22, 8, b, 4)])                                      # the lid's handle
    parts += [poly([(36, 72), (42, 68), (44, 80), (38, 84)], S.shade(b, 0.18), s=False)]   # a dent
    parts += [light(80, 80, 4, 18, b, 0.35)]
    return parts


# --- tier 3, the Park: foliage, bark -----------------------------------------------------------------------------------

def leaf(cx, cy, length, width, angle, c):
    ca, sa = math.cos(angle), math.sin(angle)
    tip = (cx + ca * length, cy + sa * length)
    base = (cx - ca * length * 0.2, cy - sa * length * 0.2)
    nx, ny = -sa * width, ca * width
    pts = [base] + [(cx + ca * length * t + nx * math.sin(math.pi * t) * 1.0, cy + sa * length * t + ny * math.sin(math.pi * t))
                    for t in (0.15, 0.35, 0.55, 0.75, 0.9)] + [tip] \
        + [(cx + ca * length * t - nx * math.sin(math.pi * t), cy + sa * length * t - ny * math.sin(math.pi * t))
           for t in (0.9, 0.75, 0.55, 0.35, 0.15)]
    return poly(pts, c), (base, tip)


def leaves():
    a, b = C["tier_3"]
    shapes, ribs = [], []
    for cx, cy, ang, col in ((40, 92, -0.3, a), (64, 98, 0.15, S.tint(a, 0.15)), (58, 80, -0.9, S.shade(a, 0.12))):
        p, rib = leaf(cx, cy, 34, 11, ang, col)
        shapes.append(p); ribs.append(rib)
    parts = []
    for p in shapes:
        parts += S.fuse([p])
    for (x0, y0), (x1, y1) in ribs:
        parts += [ink([(x0, y0), (x0 + (x1 - x0) * 0.85, y0 + (y1 - y0) * 0.85)], 1.8)]
    return parts


def flower(cx, cy, r, petal, centre):
    out = S.fuse([S.ell(cx + math.cos(a) * r * 0.75, cy + math.sin(a) * r * 0.75, r * 0.55, r * 0.55, petal)
                  for a in (i * 2 * math.pi / 5 - math.pi / 2 for i in range(5))])
    out += [S.ell(cx, cy, r * 0.38, r * 0.38, centre)]
    return out


def flowers():
    a, b = C["tier_3"]
    stem = S.shade(a, 0.28)
    parts = [S.line([(44, 76), (48, 106)], stem, 3.2), S.line([(66, 60), (64, 106)], stem, 3.2),
             S.line([(86, 76), (80, 106)], stem, 3.2)]
    lf, _ = leaf(56, 96, 18, 6, -0.6, a)
    lf2, _ = leaf(72, 98, 18, 6, -2.4, a)
    parts += S.fuse([lf, lf2])
    parts += flower(44, 72, 13, PP["flowers"][0], PP["flowers"][1])
    parts += flower(66, 56, 14, PP["flowers"][1], C["tier_3"][1])
    parts += flower(86, 72, 13, PP["flowers"][2], PP["flowers"][1])
    return parts


def tree():
    a, b = C["tier_3"]
    trunk = [poly([(54, 116), (58, 66), (70, 66), (76, 116)], b),
             poly([(46, 118), (56, 104), (60, 118)], b), poly([(70, 118), (74, 104), (84, 118)], b),   # the roots
             S.tube(S.bez((62, 82), (50, 70), (42, 62)), 7, 4, b)]                                       # a branch
    canopy = [S.ell(40, 52, 26, 22, a), S.ell(84, 48, 28, 24, a), S.ell(62, 30, 28, 22, a), S.ell(64, 60, 30, 16, a),
              S.ell(100, 64, 14, 12, a), S.ell(26, 66, 12, 10, a)]
    parts = S.fuse(trunk) + S.fuse(canopy)
    parts += [ink([(64, 112), (65, 84)], 1.8), ink([(70, 104), (71, 92)], 1.8)]                  # bark
    parts += [S.ell(64, 70, 26, 6, S.shade(a, 0.25), s=False)]                                   # the canopy's underside
    parts += [arc((30, 54), (40, 44), (52, 48)), arc((72, 44), (84, 34), (98, 42)), arc((52, 30), (62, 22), (72, 26))]
    parts += [light(54, 22, 10, 6, a, 0.4, -0.3), light(28, 46, 7, 5, a, 0.35, -0.3)]
    parts += [S.ell(78, 86, 4, 5, S.shade(b, 0.45))]                                              # a knot hole
    return parts


# --- tier 4, the Rooftops: steel, clay ---------------------------------------------------------------------------------

def vent():
    a, b = C["tier_4"]
    parts = S.fuse([r_(64, 88, 70, 44, a, 5)])
    for y in (74, 86, 98):                                                          # the louvres, each lit on top
        parts += [r_(64, y, 54, 7, S.shade(a, 0.3), 2, False), S.line([(38, y - 3), (90, y - 3)], S.tint(a, 0.35), 1.6)]
    parts += dots([(33, 70), (95, 70), (33, 106), (95, 106)], 2.2, S.shade(a, 0.45))  # the screws
    return parts


def tiles():
    a, b = C["tier_4"]
    parts = []
    for row, (y, xs) in enumerate(((70, (50, 78)), (86, (36, 64, 92)))):          # two rows of curved clay tiles
        for x in xs:
            parts += S.fuse([poly([(x - 13, y - 10), (x + 13, y - 10), (x + 13, y + 6), (x, y + 12), (x - 13, y + 6)], b)])
            parts += [arc((x - 9, y - 4), (x, y + 4), (x + 9, y - 4), 1.8, S.tint(b, 0.4))]
    parts += S.fuse([poly([(80, 102), (94, 98), (98, 106), (86, 110)], b)])         # a broken piece
    return parts


def chimney():
    a, b = C["tier_4"]
    body = [r_(62, 82, 50, 68, b, 3), r_(62, 46, 62, 14, b, 3), r_(54, 34, 14, 14, S.shade(a, 0.05), 2),
            r_(72, 34, 14, 14, S.shade(a, 0.05), 2)]                                # two pots on the cap
    parts = S.fuse(body)
    for i, y in enumerate((60, 72, 84, 96, 108)):                                   # the brick courses
        parts += [ink([(38, y), (86, y)], 1.8)]
        for x in ((50, 74) if i % 2 == 0 else (40, 62, 84)):
            if 38 < x < 86:
                parts += [ink([(x, y), (x, y + 12 if y < 108 else 116)], 1.8)]
    parts += [r_(62, 46, 62, 5, S.shade(b, 0.3), 2, False)]
    parts += [S.ell(54, 28, 5, 2.4, INK, s=False), S.ell(72, 28, 5, 2.4, INK, s=False)]   # the pots' mouths
    return parts


# --- tier 5, the Sewers: pipe, water -----------------------------------------------------------------------------------

def pipe():
    a, b = C["tier_5"]
    parts = S.fuse([r_(64, 88, 76, 26, a, 4), r_(28, 88, 12, 36, a, 3), r_(100, 88, 12, 36, a, 3)])   # a pipe and its flanges
    parts += [S.line([(30, 80), (98, 80)], S.tint(a, 0.4), 3.0)]
    parts += dots([(28, 76), (28, 100), (100, 76), (100, 100)], 2.0, S.shade(a, 0.45))   # bolts
    parts += [S.ell(112, 92, 5, 9, S.shade(a, 0.45))]                               # the open end
    parts += S.fuse([S.ell(114, 108, 3, 4.2, b)])                                   # a drip of sewer water
    return parts


def puddle5():
    a, b = C["tier_5"]
    parts = S.fuse([poly(ring_pts(62, 94, 36, 16, 0, (1, .92, 1.05, .9, 1, 1.08, .95, 1)), b), S.ell(92, 90, 16, 10, b)])
    # flatten: the puddle lies on the ground
    for p in parts:
        if p["k"] == "tri":
            p["pts"] = [(x, 94 + (y - 94) * 0.42) for x, y in p["pts"]]
    parts += [S.ell(56, 92, 14, 4.5, S.tint(b, 0.4), s=False)]
    parts += [S.line(S.bez((70, 96), (80, 92), (90, 96), 10), S.tint(b, 0.5), 1.8)]   # a ripple
    parts += S.fuse([S.ell(84, 86, 3.4, 3.4, S.tint(b, 0.25))])                      # a bubble
    return parts


def grate():
    a, b = C["tier_5"]
    parts = S.fuse([r_(64, 86, 80, 56, a, 6)])
    parts += [r_(64, 86, 64, 40, S.shade(a, 0.55), 3, False)]                       # the dark under it
    parts += [S.ell(64, 100, 26, 6, S.shade(b, 0.2), s=False)]                      # the water down there
    for x in (40, 52, 64, 76, 88):
        parts += [r_(x, 86, 6, 42, a, 2)]
    parts += [ink([(30, 66), (98, 66)], 1.6)]
    parts += dots([(30, 64), (98, 64), (30, 108), (98, 108)], 2.2, S.shade(a, 0.45))
    parts += [S.line([(34, 62), (94, 62)], S.tint(a, 0.4), 1.6)]
    parts += S.fuse([S.tube([(46, 108), (46, 114), (47, 119)], 5, 3, b)])           # ooze through the bars
    return parts


# --- tier 6, the Rival Cat Turf: timber, bowl --------------------------------------------------------------------------

def bowl():
    a, b = C["tier_6"]
    parts = S.fuse([poly([(28, 82), (100, 82), (90, 108), (38, 108)], b), S.ell(64, 82, 37, 12, b)])
    parts += [S.ell(64, 82, 29, 7.5, S.shade(b, 0.42), s=False)]                    # the inside
    parts += S.fuse([S.ell(x, y, 4.4, 3.4, C["tier_11"][1]) for x, y in ((54, 80), (64, 78), (72, 81), (60, 83))])   # food
    parts += [S.line([(36, 94), (92, 94)], S.tint(b, 0.4), 2.4)]                    # a band
    return parts


def post():
    a, b = C["tier_6"]
    parts = S.fuse([r_(64, 110, 52, 12, a, 4), r_(64, 72, 26, 70, a, 4), r_(64, 38, 38, 10, a, 4)])
    rope = S.tint(a, 0.45)
    parts += [r_(64, 74, 26, 52, rope, 3, False)]
    for y in range(52, 100, 6):                                                     # the rope wound round it
        parts += [ink([(52, y + 4), (76, y)], 1.6)]
    parts += [poly([(54, 70), (58, 64), (56, 78)], S.shade(rope, 0.3), s=False), poly([(70, 82), (74, 76), (72, 90)], S.shade(rope, 0.3), s=False)]   # shredded
    parts += [S.line([(78, 40), (88, 52), (86, 62)], INK, 1.6)]                     # a string
    parts += S.fuse([S.ell(86, 66, 5, 5, PP["flowers"][0])])                       # and its pompom
    return parts


def fence():
    a, b = C["tier_6"]
    pickets = [poly([(x - 9, 116), (x - 9, 46), (x, 34), (x + 9, 46), (x + 9, 116)], a) for x in (30, 64, 98)]
    parts = S.fuse(pickets)
    parts += S.fuse([r_(64, 60, 108, 9, S.shade(a, 0.2), 3), r_(64, 96, 108, 9, S.shade(a, 0.2), 3)])
    for x in (30, 64, 98):
        parts += [ink([(x - 3, 70), (x - 2, 86)], 1.6), ink([(x + 3, 104), (x + 3, 112)], 1.6)]   # grain
        parts += dots([(x, 60), (x, 96)], 1.8, INK)                                   # nails
        parts += [S.line([(x - 5, 48), (x - 5, 54)], S.tint(a, 0.4), 2.0)]
    return parts


# --- tier 7, the Portal Zone: shard, gold ------------------------------------------------------------------------------

def shard():
    a, b = C["tier_7"]
    parts = S.fuse([poly([(48, 108), (60, 34), (78, 108)], a), poly([(70, 108), (86, 62), (94, 108)], a),
                    poly([(36, 108), (42, 80), (54, 108)], a)])
    parts += [poly([(60, 34), (64, 108), (78, 108)], S.shade(a, 0.18), s=False),     # the dark facet
              poly([(86, 62), (88, 108), (94, 108)], S.shade(a, 0.18), s=False)]
    parts += [S.line([(58, 48), (54, 98)], S.tint(a, 0.45), 2.6), S.line([(84, 72), (80, 100)], S.tint(a, 0.45), 2.2)]
    parts += [S.ell(64, 110, 30, 4, S.shade(a, 0.3), s=False)]
    return parts


def ring():
    a, b = C["tier_7"]
    outer = ring_pts(64, 80, 34, 28)
    parts = S.fuse([poly(outer, b)])
    parts += [S.ell(64, 80, 22, 22, INK, s=False)]                                  # the hole (a portal, rimmed)
    parts += [S.ell(64, 80, 19, 19, S.shade(a, 0.45), s=False)]
    parts += [S.line([(64 + math.cos(k * 0.35) * (2 + k * 0.42), 80 + math.sin(k * 0.35) * (2 + k * 0.42)) for k in range(36)],
                     S.tint(a, 0.2), 2.2)]                                          # the portal's spiral
    parts += [S.line([(x, y) for x, y in ring_pts(64, 80, 28, 10)[6:10]], S.tint(b, 0.45), 2.6)]   # the gold catching light
    for i in range(8):                                                              # notches round the band
        an = i * math.pi / 4 + 0.2
        parts += [ink([(64 + math.cos(an) * 24, 80 + math.sin(an) * 24), (64 + math.cos(an) * 33, 80 + math.sin(an) * 33)], 1.6)]
    return parts


def arch():
    a, b = C["tier_7"]
    parts = []
    n = 7
    for i in range(n):                                                              # the voussoirs
        a0, a1 = math.pi + i * math.pi / n, math.pi + (i + 1) * math.pi / n
        ro, ri = 50, 34
        pts = [(64 + math.cos(a0) * ri, 100 + math.sin(a0) * ri), (64 + math.cos(a0) * ro, 100 + math.sin(a0) * ro),
               (64 + math.cos(a1) * ro, 100 + math.sin(a1) * ro), (64 + math.cos(a1) * ri, 100 + math.sin(a1) * ri)]
        col = b if i == n // 2 else (a if i % 2 == 0 else S.tint(a, 0.15))        # the keystone in gold
        parts += S.fuse([poly(pts, col)])
    parts += S.fuse([r_(22, 108, 18, 20, a, 3), r_(106, 108, 18, 20, a, 3)])
    parts += [S.line(S.bez((36, 100), (64, 62), (92, 100), 16), S.tint(a, 0.55), 2.0)]   # the glow along its inside
    return parts


# --- tier 8, the Mythic Realm: gold, crystal ---------------------------------------------------------------------------

def star():
    a, b = C["tier_8"]
    pts = []
    for i in range(10):
        an = -math.pi / 2 + i * math.pi / 5
        rr = 34 if i % 2 == 0 else 15
        pts.append((64 + math.cos(an) * rr, 80 + math.sin(an) * rr))
    parts = S.fuse([poly(pts, a)])
    for i in range(0, 10, 2):                                                       # each point's ridge
        parts += [ink([(64, 80), pts[i]], 1.6)]
    parts += [poly([(64, 80), pts[0], pts[1]], S.tint(a, 0.4), s=False), poly([(64, 80), pts[8], pts[9]], S.tint(a, 0.25), s=False)]
    parts += [poly([(64, 80)] + [pts[i] for i in (4, 5, 6)], S.shade(a, 0.15), s=False)]
    for x, y, r in ((100, 50, 6), (28, 56, 4)):                                     # two glints
        parts += [S.line([(x - r, y), (x + r, y)], S.tint(a, 0.2), 2.0), S.line([(x, y - r), (x, y + r)], S.tint(a, 0.2), 2.0)]
    return parts


def crystal():
    a, b = C["tier_8"]
    shards = [poly([(46, 108), (48, 52), (60, 36), (70, 54), (68, 108)], b),
              poly([(66, 108), (74, 66), (86, 56), (94, 72), (90, 108)], b),
              poly([(30, 108), (34, 82), (44, 74), (50, 108)], b)]
    parts = S.fuse(shards)
    parts += [poly([(60, 36), (60, 108), (68, 108), (70, 54)], S.shade(b, 0.18), s=False),
              poly([(86, 56), (84, 108), (90, 108), (94, 72)], S.shade(b, 0.18), s=False)]
    parts += [ink([(48, 52), (60, 58), (70, 54)], 1.6), ink([(74, 66), (84, 70), (94, 72)], 1.6)]
    parts += [S.line([(52, 58), (52, 100)], S.tint(b, 0.5), 2.6)]
    parts += S.fuse([poly([(40, 112), (88, 112), (96, 106), (32, 106)], S.shade(b, 0.35))])   # the base it grows from
    return parts


def rock8():
    a, b = C["tier_8"]
    stone, light_ = PP["rock"]
    body = [poly([(22, 110), (26, 84), (44, 66), (72, 64), (96, 74), (108, 96), (104, 112)], stone)]
    parts = S.fuse(body)
    parts += [poly([(34, 84), (46, 70), (70, 68), (62, 82)], light_, s=False)]
    parts += [ink([(62, 82), (70, 104)]), ink([(62, 82), (34, 86)])]
    parts += S.fuse([poly([(74, 74), (84, 34), (94, 76)], a), poly([(88, 80), (102, 52), (106, 86)], a)])   # gold growing out
    parts += [S.line([(82, 44), (80, 70)], S.tint(a, 0.45), 2.2)]
    parts += [ink([(40, 96), (50, 92), (56, 100)], 1.6)]
    return parts


# --- tier 9, the Neighbour's Kitchen: crust, appliance -----------------------------------------------------------------

def crumbs():
    a, b = C["tier_9"]
    bits = [poly([(42, 86), (54, 82), (62, 90), (56, 100), (42, 98)], a), poly([(70, 84), (80, 82), (82, 92), (72, 94)], a),
            poly([(60, 102), (68, 100), (70, 108), (62, 110)], a), poly([(86, 98), (92, 96), (94, 102), (88, 104)], a)]
    parts = []
    for bt in bits:
        parts += S.fuse([bt])
    parts += [poly([(44, 88), (54, 84), (58, 88), (46, 92)], S.shade(a, 0.3), s=False)]    # the crust on the big one
    parts += dots([(50, 94), (76, 88)], 1.4, S.tint(a, 0.5))
    return parts


def shelf():
    a, b = C["tier_9"]
    parts = S.fuse([r_(26, 82, 10, 30, S.shade(a, 0.15), 2), r_(102, 82, 10, 30, S.shade(a, 0.15), 2), r_(64, 62, 100, 14, a, 3)])
    parts += [r_(64, 58, 100, 5, S.tint(a, 0.35), 2, False), ink([(28, 66), (100, 66)], 1.6)]
    parts += S.fuse([r_(44, 42, 20, 26, b, 5), r_(44, 28, 22, 6, S.shade(b, 0.3), 2)])          # a jar
    parts += [r_(44, 44, 12, 10, PP["flowers"][1], 2, False)]                                    # its label
    parts += S.fuse([poly([(70, 54), (90, 54), (88, 34), (72, 34)], PP["toaster"][0])])          # a mug
    parts += [S.line(S.bez((90, 38), (100, 44), (90, 50), 10), INK, 3.0)]
    return parts


def toaster():
    a, b = C["tier_9"]
    parts = S.fuse([r_(52, 50, 18, 20, a, 4), r_(76, 52, 18, 18, a, 4)])            # the toast, up
    parts += [r_(52, 46, 12, 10, S.tint(a, 0.4), 3, False), r_(76, 48, 12, 10, S.tint(a, 0.4), 3, False)]
    parts += S.fuse([r_(64, 84, 76, 52, b, 10)])
    parts += [r_(52, 62, 22, 5, INK, 2, False), r_(76, 62, 22, 5, INK, 2, False)]   # the slots
    parts += [S.line([(36, 74), (36, 100)], S.tint(b, 0.4), 3.0)]
    parts += [r_(98, 82, 8, 4, S.shade(b, 0.4), 2)]                                 # the lever
    parts += S.fuse([S.ell(92, 98, 5, 5, PP["toaster"][0])])                        # the knob
    parts += S.fuse([r_(40, 112, 10, 6, S.shade(b, 0.3), 2), r_(88, 112, 10, 6, S.shade(b, 0.3), 2)])   # feet
    return parts


# --- tier 10, the Vet's Waiting Room: steel, cloth ---------------------------------------------------------------------

def fur():
    shadow, coat, lite = PP["fur"]
    body = [S.ell(60, 92, 22, 12, coat), S.ell(80, 88, 14, 10, coat), S.ell(46, 86, 13, 9, coat), S.ell(66, 80, 12, 8, coat)]
    body += [poly([(36, 86), (30, 80), (40, 82)], coat), poly([(90, 84), (98, 80), (92, 90)], coat),
             poly([(62, 74), (64, 68), (68, 74)], coat), poly([(74, 104), (80, 104), (76, 98)], coat)]   # tufts sticking out
    parts = S.fuse(body)
    parts += [S.ell(58, 88, 9, 4.5, lite, s=False)]
    parts += [arc((50, 96), (60, 92), (70, 98), 1.6), arc((72, 90), (80, 86), (88, 90), 1.6)]
    return parts


def carrier():
    a, b = C["tier_10"]
    parts = S.fuse([r_(64, 86, 84, 52, b, 9), r_(64, 52, 34, 12, a, 5)])            # the box, its handle
    parts += [r_(64, 64, 76, 5, S.shade(b, 0.25), 2, False)]                         # the lid's seam
    parts += S.fuse([r_(86, 88, 32, 34, S.shade(b, 0.35), 5)])                        # the door
    for x in (78, 86, 94):
        parts += [S.line([(x, 74), (x, 102)], a, 2.4)]                              # its bars
    parts += [S.line([(72, 88), (100, 88)], a, 2.0)]
    for y in (78, 88, 98):                                                          # the vents
        parts += [r_(42, y, 18, 4, S.shade(b, 0.4), 2, False)]
    return parts


def chair():
    a, b = C["tier_10"]
    parts = S.fuse([r_(32, 104, 6, 26, a, 2), r_(96, 104, 6, 26, a, 2)])            # the legs
    parts += S.fuse([r_(64, 56, 74, 46, b, 8), r_(64, 90, 82, 18, b, 6)])            # the back and the seat, cloth
    parts += [r_(64, 56, 56, 30, S.shade(b, 0.2), 6, False)]
    parts += [ink([(64, 44), (64, 70)], 1.6)]                                       # the seam
    parts += dots([(50, 50), (78, 50), (50, 62), (78, 62)], 2.0, S.shade(b, 0.45))   # buttons
    parts += [S.line([(28, 84), (100, 84)], S.tint(b, 0.4), 2.4)]
    parts += S.fuse([r_(64, 116, 72, 5, a, 2)])                                       # the rail
    return parts


# --- tier 11, the Pet Food Factory: machine, kibble --------------------------------------------------------------------

def kibble_bits(cx, cy, c, kinds):
    out = []
    for (x, y, k) in kinds:
        if k == "fish":
            out += S.fuse([S.ell(cx + x, cy + y, 9, 6, c), poly([(cx + x + 7, cy + y), (cx + x + 14, cy + y - 6), (cx + x + 14, cy + y + 6)], c)])
            out += dots([(cx + x - 4, cy + y - 1)], 1.4, INK)
        elif k == "bone":
            out += S.fuse([r_(cx + x, cy + y, 14, 5, c, 2), S.ell(cx + x - 8, cy + y - 3, 3.4, 3.4, c), S.ell(cx + x - 8, cy + y + 3, 3.4, 3.4, c),
                           S.ell(cx + x + 8, cy + y - 3, 3.4, 3.4, c), S.ell(cx + x + 8, cy + y + 3, 3.4, 3.4, c)])
        else:
            out += S.fuse([S.ell(cx + x, cy + y, 7, 6, c)])
            out += [S.ell(cx + x, cy + y, 2.2, 2.2, S.shade(c, 0.35), s=False)]
    return out


def kibble():
    a, b = C["tier_11"]
    return kibble_bits(64, 94, b, ((-14, -2, "fish"), (14, -6, "o"), (6, 8, "bone")))


def hopper():
    a, b = C["tier_11"]
    parts = S.fuse([S.ell(64, 44, 34, 9, b)])                                        # the kibble heaped in it
    parts += S.fuse([poly([(22, 48), (106, 48), (74, 94), (54, 94)], a), r_(64, 104, 22, 22, a, 3)])
    parts += [r_(64, 50, 88, 8, S.shade(a, 0.28), 3, False)]
    parts += dots([(30, 56), (98, 56), (44, 74), (84, 74)], 2.2, S.shade(a, 0.45))   # rivets
    parts += [S.line([(36, 58), (60, 90)], S.tint(a, 0.35), 2.4)]
    parts += S.fuse([S.ell(64, 120, 4.4, 3.4, b)])                                   # a piece falling out
    parts += [S.ell(56, 42, 3, 2.4, S.shade(b, 0.3), s=False), S.ell(72, 44, 3, 2.4, S.shade(b, 0.3), s=False)]
    return parts


def conveyor():
    a, b = C["tier_11"]
    parts = S.fuse([r_(40, 108, 10, 24, S.shade(a, 0.2), 2), r_(88, 108, 10, 24, S.shade(a, 0.2), 2)])   # legs
    parts += S.fuse([r_(64, 86, 112, 26, a, 13)])
    parts += [r_(64, 76, 96, 5, S.shade(a, 0.45), 2, False)]                          # the belt
    for x in (20, 42, 64, 86, 108):
        parts += S.fuse([S.ell(x, 90, 7, 7, S.shade(a, 0.25))])
        parts += [S.ell(x, 90, 1.8, 1.8, INK, s=False)]
    parts += kibble_bits(64, 66, b, ((-26, 0, "o"), (0, -2, "fish"), (26, 0, "bone")))
    return parts


# --- tier 12, the Litterbox Dimension: litter, clump -------------------------------------------------------------------

def clump():
    a, b = C["tier_12"]
    pts = ring_pts(64, 90, 26, 14, 0.2, (1, .86, 1.04, .92, 1.1, .9, 1, .84, 1.06, .95, 1, .88, 1.08, .93))
    parts = S.fuse([poly([(x, 90 + (y - 90) * 0.72) for x, y in pts], b), S.ell(90, 96, 10, 7, b)])
    parts += [poly([(46, 82), (60, 76), (72, 80), (58, 86)], S.tint(b, 0.3), s=False)]
    parts += dots([(50, 94), (62, 98), (76, 92), (70, 86), (56, 88), (90, 94)], 1.8, S.shade(b, 0.35))   # granules stuck to it
    parts += dots([(36, 104), (102, 106), (28, 98)], 1.8, S.shade(a, 0.1))          # loose litter round it
    return parts


def sand():
    a, b = C["tier_12"]
    parts = S.fuse([poly([(24, 104), (44, 88), (64, 82), (86, 88), (106, 104)], a)])   # a little dune of litter
    parts += [arc((38, 100), (52, 94), (66, 98), 1.8, S.shade(a, 0.3)), arc((60, 92), (74, 88), (88, 94), 1.8, S.shade(a, 0.3))]
    parts += [S.oval(58, 88, 12, 3, -0.15, S.tint(a, 0.5), s=False)]
    parts += dots([(18, 108), (112, 108), (30, 112), (98, 112)], 2.0, a)
    return parts


def scoop():
    a, b = C["tier_12"]
    parts = S.fuse([S.tube([(78, 70), (96, 58), (116, 48)], 12, 10, a)])              # the handle
    parts += [S.ell(112, 50, 3.4, 2.6, INK, s=False)]                               # its hang hole
    parts += S.fuse([poly([(18, 74), (82, 66), (90, 104), (30, 114)], a)])           # the blade
    parts += [poly([(26, 80), (78, 72), (84, 100), (34, 108)], S.shade(a, 0.22), s=False)]
    for i in range(4):                                                              # its slots
        x0 = 36 + i * 12
        parts += [r_(x0 + 4, 90, 5, 22, INK, 2, False)]
    parts += S.fuse([S.ell(56, 76, 7, 5, b)])                                        # a clump caught in it
    parts += [S.line([(84, 64), (110, 50)], S.tint(a, 0.4), 2.2)]
    return parts


PROP_DRAW = {
    "prop_tier1_grass": grass, "prop_tier1_rock": rock, "prop_tier1_bush": bush,
    "prop_tier2_brick": brick, "prop_tier2_bag": bag, "prop_tier2_bin": bin_,
    "prop_tier3_leaves": leaves, "prop_tier3_flowers": flowers, "prop_tier3_tree": tree,
    "prop_tier4_vent": vent, "prop_tier4_tiles": tiles, "prop_tier4_chimney": chimney,
    "prop_tier5_pipe": pipe, "prop_tier5_puddle": puddle5, "prop_tier5_grate": grate,
    "prop_tier6_bowl": bowl, "prop_tier6_post": post, "prop_tier6_fence": fence,
    "prop_tier7_shard": shard, "prop_tier7_ring": ring, "prop_tier7_arch": arch,
    "prop_tier8_star": star, "prop_tier8_crystal": crystal, "prop_tier8_rock": rock8,
    "prop_tier9_crumbs": crumbs, "prop_tier9_shelf": shelf, "prop_tier9_toaster": toaster,
    "prop_tier10_fur": fur, "prop_tier10_carrier": carrier, "prop_tier10_chair": chair,
    "prop_tier11_kibble": kibble, "prop_tier11_hopper": hopper, "prop_tier11_conveyor": conveyor,
    "prop_tier12_clump": clump, "prop_tier12_puddle": sand, "prop_tier12_scoop": scoop,
}


def PROPS():
    return {stem: draw() for stem, draw in PROP_DRAW.items()}


def seam_ratio(img):
    """ci/check_textures.gd's own measure, reimplemented so a floor can be
    rejected here rather than three steps later in a Godot gate."""
    w, h = img.size
    px = img.load()

    def diff(a, b):
        return sum(abs(a[i] - b[i]) for i in range(3)) / 765.0

    eh = ih = 0.0
    for y in range(0, h, 4):
        eh += diff(px[0, y], px[w - 1, y])
        ih += diff(px[w // 2, y], px[w // 2 + 1, y])
    ev = iv = 0.0
    for x in range(0, w, 4):
        ev += diff(px[x, 0], px[x, h - 1])
        iv += diff(px[x, h // 2], px[x, h // 2 + 1])
    return ((eh / max(ih, 1e-4)), (ev / max(iv, 1e-4)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--seams", action="store_true")
    args = ap.parse_args()

    floors = {t: floor(t) for t in TIERS}

    print("seam ratios (the gate rejects above 2.0; 1.0 means the join is as "
          "quiet as the texture's own grain)")
    worst = 0.0
    for t, img in floors.items():
        hx, vy = seam_ratio(img)
        worst = max(worst, hx, vy)
        flag = "   <-- SEAM" if max(hx, vy) > 2.0 else ""
        print("  %-9s h=%5.2f  v=%5.2f%s" % (t, hx, vy, flag))
    print("worst: %.2f" % worst)
    if args.seams:
        return 0 if worst <= 2.0 else 1

    props = PROPS()
    bad = check_prop_colours()
    if bad:
        for b in bad:
            print("  PROP COLOUR PROBLEM: %s" % b)
        return 1
    print("%d props, all visible on their own floor" % len(props))

    if args.write:
        out = STYLE / "backdrops"
        out.mkdir(parents=True, exist_ok=True)
        for t, img in floors.items():
            img.save(out / ("backdrop_%s.png" % t))
        pout = STYLE / "props"
        pout.mkdir(parents=True, exist_ok=True)
        for name, parts in props.items():
            (pout / (name + ".svg")).write_text(S.to_svg(parts, S.SIZE, px=PROP),
                                                encoding="utf-8")
        print("wrote %d floors and %d props to %s" % (len(floors), len(props), STYLE))
        return 0

    # props sheet: each prop ON ITS OWN FLOOR, because that is the only place it
    # ever appears and a prop reviewed on a neutral card is a prop reviewed
    # against a background the game never shows.
    cols, cell = 6, 150
    rows = (len(props) + cols - 1) // cols
    ps = Image.new("RGB", (cols * cell, rows * cell + 30), (250, 248, 242))
    pd = ImageDraw.Draw(ps)
    pd.text((10, 8), "thirty-six props, each on the floor it actually stands on",
            fill=PAL.INK)
    for i, (name, parts) in enumerate(sorted(props.items())):
        tier = "tier_" + name.split("_")[1].replace("tier", "")
        cx, cy = (i % cols) * cell, (i // cols) * cell + 26
        patch = floors[tier].resize((cell - 12, cell - 34), Image.LANCZOS)
        ps.paste(patch, (cx + 6, cy + 2))
        img = S.to_image(parts, S.SIZE).resize((cell - 40, cell - 40), Image.LANCZOS)
        ps.paste(img, (cx + 20, cy + 6), img)
        pd.text((cx + 6, cy + cell - 28), name.replace("prop_", ""), fill=PAL.INK)
    pp = ROOT / "build" / "sweet_props_all.png"
    ps.save(pp)
    print("wrote %s (%dx%d)" % (pp, ps.width, ps.height))

    # contact sheet: each floor at a readable size, and one of them tiled 2x2
    cols, cell = 4, 200
    rows = (len(floors) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell, rows * cell + 30), (250, 248, 242))
    d = ImageDraw.Draw(sheet)
    d.text((10, 8), "the twelve floors, each shown as a 2x2 repeat so the seam "
                    "has somewhere to show", fill=PAL.INK)
    for i, (t, img) in enumerate(floors.items()):
        cx, cy = (i % cols) * cell, (i // cols) * cell + 26
        quad = Image.new("RGB", (TILE * 2, TILE * 2))
        for qx in (0, TILE):
            for qy in (0, TILE):
                quad.paste(img, (qx, qy))
        sheet.paste(quad.resize((cell - 16, cell - 40), Image.LANCZOS), (cx + 8, cy + 4))
        d.text((cx + 8, cy + cell - 32), "%s  %s" % (t, TIERS[t].split("_", 2)[2]),
               fill=PAL.INK)
    p = ROOT / "build" / "sweet_floors.png"
    p.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(p)
    print("wrote %s (%dx%d) - review before --write" % (p, sheet.width, sheet.height))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
