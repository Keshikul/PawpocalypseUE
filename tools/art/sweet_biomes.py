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


def _r(cx, cy, w, h, c, rr=3.0, s_=True):
    return S.rect(cx, cy, w, h, c, r=rr, s=s_)


def PROPS():
    P_ = {}
    for tier, (a, b) in PROP_COLOURS.items():
        da, db = S.shade(a, 0.28), S.shade(b, 0.28)
        t = tier.replace("tier_", "")

        if tier == "tier_1":
            P_["prop_tier1_grass"] = S.fuse([S.ell(64, 92, 26, 10, a)]) + [
                S.line([(48, 92), (40, 56)], da, 5.0),
                S.line([(58, 90), (56, 46)], da, 5.0),
                S.line([(70, 90), (76, 50)], da, 5.0),
                S.line([(80, 92), (90, 60)], da, 5.0)]
            P_["prop_tier1_rock"] = S.fuse([S.ell(58, 86, 30, 22, b),
                                            S.ell(84, 92, 20, 16, b)]) + [
                S.ell(52, 80, 12, 7, S.tint(b, 0.4), s=False)]
            P_["prop_tier1_bush"] = S.fuse([S.ell(48, 80, 30, 28, a),
                                            S.ell(80, 74, 28, 26, a),
                                            S.ell(64, 56, 26, 24, a),
                                            _r(64, 104, 70, 20, a, 8)]) + [
                S.ell(50, 74, 11, 8, S.tint(a, 0.35), s=False)]
        elif tier == "tier_2":
            P_["prop_tier2_brick"] = S.fuse([_r(64, 86, 62, 26, a, 4)]) + [
                S.line([(36, 86), (92, 86)], da, 3.0)]
            # A BULGING SACK WITH A GATHERED, TIED TOP. Drawn as a rounded
            # rectangle with a small cap it was a tombstone - straight sides and
            # a domed top is a headstone, not a bin bag. A sack is widest near
            # the bottom and pinches where it is tied.
            P_["prop_tier2_bag"] = S.fuse([S.ell(64, 92, 36, 30, "#6E6A63"),
                                           S.ell(64, 70, 28, 22, "#6E6A63"),
                                           S.ell(64, 50, 13, 12, "#6E6A63"),
                                           S.tri([(52, 44), (64, 24), (76, 44)], "#6E6A63")]) + [
                S.line([(50, 56), (78, 56)], S.shade("#6E6A63", 0.35), 5.0),
                S.ell(52, 88, 12, 8, S.tint("#6E6A63", 0.28), s=False)]
            P_["prop_tier2_bin"] = S.fuse([_r(64, 84, 64, 64, b, 6),
                                           _r(64, 48, 76, 16, b, 5)]) + [
                _r(64, 48, 76, 7, db, 3, False),
                _r(64, 84, 54, 8, db, 3, False)]
        elif tier == "tier_3":
            P_["prop_tier3_leaves"] = S.fuse([S.ell(48, 88, 20, 12, a),
                                              S.ell(78, 92, 18, 11, a),
                                              S.ell(64, 76, 17, 10, a)]) + [
                S.line([(30, 88), (66, 88)], da, 2.4)]
            P_["prop_tier3_flowers"] = (
                S.fuse([S.ell(44, 72, 13, 13, "#E8B0C4"), S.ell(64, 60, 13, 13, "#F0D27C"),
                        S.ell(84, 72, 13, 13, "#B49AD6")])
                + [S.line([(44, 78), (46, 104)], da, 3.0),
                   S.line([(64, 66), (64, 104)], da, 3.0),
                   S.line([(84, 78), (82, 104)], da, 3.0)])
            P_["prop_tier3_tree"] = S.fuse([_r(64, 100, 22, 44, b, 5),
                                            S.ell(44, 56, 30, 26, a),
                                            S.ell(84, 52, 30, 26, a),
                                            S.ell(64, 34, 28, 24, a)]) + [
                S.line([(64, 108), (64, 70)], db, 3.0)]
        elif tier == "tier_4":
            P_["prop_tier4_vent"] = S.fuse([_r(64, 86, 66, 42, a, 5)]) + [
                _r(64, 74, 54, 7, da, 2, False), _r(64, 86, 54, 7, da, 2, False),
                _r(64, 98, 54, 7, da, 2, False)]
            P_["prop_tier4_tiles"] = S.fuse([_r(46, 88, 40, 26, b, 3),
                                             _r(86, 88, 40, 26, b, 3),
                                             _r(64, 66, 40, 26, b, 3)]) + [
                S.line([(26, 76), (102, 76)], db, 2.4)]
            P_["prop_tier4_chimney"] = S.fuse([_r(64, 84, 48, 76, b, 4),
                                               _r(64, 44, 62, 18, b, 4)]) + [
                _r(64, 44, 62, 8, db, 3, False),
                S.line([(50, 60), (50, 116)], db, 3.0)]
        elif tier == "tier_5":
            P_["prop_tier5_pipe"] = S.fuse([_r(64, 88, 84, 30, a, 14)]) + [
                S.ell(24, 88, 8, 15, da), S.ell(104, 88, 8, 15, da)]
            P_["prop_tier5_puddle"] = S.fuse([S.ell(60, 92, 38, 17, b),
                                              S.ell(86, 88, 20, 11, b)]) + [
                S.ell(50, 88, 13, 5, S.tint(b, 0.45), s=False)]
            P_["prop_tier5_grate"] = S.fuse([_r(64, 86, 78, 54, a, 5)]) + [
                S.line([(40, 62), (40, 110)], da, 4.0),
                S.line([(56, 62), (56, 110)], da, 4.0),
                S.line([(72, 62), (72, 110)], da, 4.0),
                S.line([(88, 62), (88, 110)], da, 4.0)]
        elif tier == "tier_6":
            # A BOWL NEEDS A RIM AND A BODY. One ellipse plus a bar read as a
            # flat lozenge; the depth comes from a wide rim ellipse sitting on a
            # narrower tapered body, with the inside shaded darker than the out.
            P_["prop_tier6_bowl"] = S.fuse([S.ell(64, 96, 30, 16, b),
                                            _r(64, 106, 36, 12, b, 6)]) + [
                S.ell(64, 80, 38, 15, b, s=True),
                S.ell(64, 80, 30, 10, S.shade(b, 0.42), s=False)]
            P_["prop_tier6_post"] = S.fuse([_r(64, 84, 24, 68, a, 4),
                                            _r(64, 46, 36, 14, a, 5)]) + [
                S.line([(64, 60), (64, 112)], da, 3.0)]
            P_["prop_tier6_fence"] = S.fuse([_r(30, 78, 18, 74, a, 4),
                                             _r(64, 78, 18, 74, a, 4),
                                             _r(98, 78, 18, 74, a, 4)]) + [
                _r(64, 60, 104, 9, da, 3, False), _r(64, 96, 104, 9, da, 3, False)]
        elif tier == "tier_7":
            P_["prop_tier7_shard"] = S.fuse([S.tri([(46, 104), (66, 32), (82, 104)], a)]) + [
                S.line([(66, 40), (60, 100)], S.tint(a, 0.4), 3.0)]
            P_["prop_tier7_ring"] = _ring_parts(64, 80, 34, b, 14)
            P_["prop_tier7_arch"] = (_ring_parts(64, 96, 44, a, 16, half=True)
                                     + [_r(20, 110, 20, 22, a, 3),
                                        _r(108, 110, 20, 22, a, 3)])
        elif tier == "tier_8":
            P_["prop_tier8_star"] = _star_parts(64, 76, 34, a)
            P_["prop_tier8_crystal"] = S.fuse([S.tri([(48, 104), (58, 34), (70, 104)], b),
                                               S.tri([(68, 104), (84, 52), (92, 104)], b)]) + [
                S.line([(58, 44), (54, 98)], S.tint(b, 0.45), 3.0)]
            P_["prop_tier8_rock"] = S.fuse([S.ell(54, 88, 32, 24, "#9E93B5"),
                                            S.ell(84, 82, 24, 22, "#9E93B5"),
                                            S.tri([(70, 84), (78, 40), (92, 82)], a)]) + [
                S.ell(48, 82, 12, 7, "#B5ABC9", s=False)]
        elif tier == "tier_9":
            P_["prop_tier9_crumbs"] = S.fuse([S.ell(52, 94, 11, 8, a)]) + [
                S.ell(74, 90, 8, 6, a, s=True), S.ell(64, 102, 6, 5, a, s=True),
                S.ell(88, 98, 5, 4, a, s=True)]
            P_["prop_tier9_toaster"] = S.fuse([_r(64, 84, 72, 50, b, 8)]) + [
                _r(52, 60, 26, 7, db, 2, False), _r(80, 60, 26, 7, db, 2, False),
                S.ell(96, 92, 7, 7, "#C25E4E", s=True)]
            P_["prop_tier9_shelf"] = S.fuse([_r(64, 58, 100, 16, a, 3),
                                             _r(30, 92, 14, 52, a, 3),
                                             _r(98, 92, 14, 52, a, 3)]) + [
                _r(64, 58, 100, 7, da, 2, False)]
        elif tier == "tier_10":
            # A CLUMP, not an oval with three legs. Three whiskers off a smooth
            # ellipse read as an insect; a shed fur tuft is an irregular fused
            # mass with a broken edge, which is the dust bunny's trick applied
            # to something that is not alive.
            P_["prop_tier10_fur"] = S.fuse([S.ell(60, 92, 22, 13, "#D6C9B5"),
                                            S.ell(78, 88, 15, 10, "#D6C9B5"),
                                            S.ell(48, 86, 13, 9, "#D6C9B5"),
                                            S.ell(68, 80, 12, 8, "#D6C9B5")]) + [
                S.line([(40, 84), (30, 74)], "#B5A894", 2.6),
                S.line([(88, 84), (100, 76)], "#B5A894", 2.6),
                S.ell(56, 88, 9, 5, "#EDE4D6", s=False)]
            P_["prop_tier10_carrier"] = S.fuse([_r(64, 88, 78, 52, b, 8),
                                                _r(64, 56, 30, 12, b, 5)]) + [
                _r(84, 88, 30, 34, S.shade(b, 0.3), 4, True),
                S.line([(76, 74), (76, 102)], db, 2.4),
                S.line([(92, 74), (92, 102)], db, 2.4)]
            P_["prop_tier10_chair"] = S.fuse([_r(64, 88, 76, 22, a, 5),
                                              _r(64, 56, 76, 44, a, 6),
                                              _r(34, 108, 12, 24, a, 3),
                                              _r(94, 108, 12, 24, a, 3)]) + [
                _r(64, 56, 60, 30, S.shade(a, 0.25), 5, False)]
        elif tier == "tier_11":
            P_["prop_tier11_kibble"] = S.fuse([S.ell(54, 92, 12, 10, b)]) + [
                S.ell(76, 88, 10, 9, b, s=True), S.ell(66, 102, 9, 8, b, s=True)]
            P_["prop_tier11_hopper"] = S.fuse([S.tri([(24, 48), (104, 48), (64, 100)], a),
                                               _r(64, 108, 26, 22, a, 3)]) + [
                _r(64, 52, 84, 10, da, 3, False)]
            P_["prop_tier11_conveyor"] = S.fuse([_r(64, 84, 108, 26, a, 12)]) + [
                S.ell(24, 84, 11, 11, da), S.ell(64, 84, 11, 11, da),
                S.ell(104, 84, 11, 11, da),
                _r(64, 108, 16, 22, da, 3, True)]
        else:
            P_["prop_tier12_clump"] = S.fuse([S.ell(58, 90, 26, 19, b),
                                              S.ell(80, 86, 18, 15, b)]) + [
                S.ell(52, 84, 10, 6, S.shade(b, 0.3), s=False)]
            P_["prop_tier12_puddle"] = S.fuse([S.ell(62, 94, 36, 15, a),
                                               S.ell(88, 90, 17, 9, a)]) + [
                S.ell(52, 90, 12, 5, S.tint(a, 0.4), s=False)]
            P_["prop_tier12_scoop"] = S.fuse([S.ell(52, 88, 30, 24, a),
                                              _r(92, 60, 46, 14, a, 6)]) + [
                S.ell(52, 88, 19, 15, S.shade(a, 0.3), s=False),
                S.line([(76, 62), (110, 56)], da, 3.0)]
    return P_


def _ring_parts(cx, cy, r, colour, w, half=False):
    out = []
    n = 18
    for i in range(n):
        a = math.pi + i * (math.pi / (n - 1)) if half else i * (2.0 * math.pi / n)
        out.append(S.ell(cx + math.cos(a) * r, cy + math.sin(a) * r,
                         w * 0.62, w * 0.62, colour))
    return S.fuse(out)


def _star_parts(cx, cy, r, colour):
    out = []
    for i in range(5):
        a = i * (2.0 * math.pi / 5) - math.pi / 2
        out.append(S.tri([(cx + math.cos(a - 0.34) * r * 0.4,
                           cy + math.sin(a - 0.34) * r * 0.4),
                          (cx + math.cos(a) * r, cy + math.sin(a) * r),
                          (cx + math.cos(a + 0.34) * r * 0.4,
                           cy + math.sin(a + 0.34) * r * 0.4)], colour))
    return S.fuse(out)


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
