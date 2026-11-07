#!/usr/bin/env python3
"""Regenerate the shipping placeholder SVGs from the sweet grammar.

THIS REPLACES make_art.py AS THE GENERATOR OF RECORD, and the difference is the
whole point of the file. `make_art.py` drew 18 assets under names like
`mouse_common`; `rename_art_assets.py` then fanned those 18 out across the 135
filenames the game actually loads. The result is that **135 files on disk hold
only 44 distinct images.** All twelve job icons are the same picture. All twelve
gristle variants, plus the factory rat, the robot mouse and the cockroach, share
one blob. The six bosses are one image. Player stages 4-7 are one image. The
twenty-eight cards are seven images repeated four times each.

So this is not a restyle. Most of these assets have never been drawn, and the
job is to give each of the 135 names its own drawing.

HOW IT RUNS. One family at a time, because 135 designs reviewed in one go is 135
designs reviewed badly:

    py tools/make_sweet_art.py --list
    py tools/make_sweet_art.py --batch player            # contact sheet only
    py tools/make_sweet_art.py --batch player --write    # write the SVGs

`--batch X` renders build/sweet_batch_X.png and writes nothing. That sheet shows
every asset at full size AND at the size it is actually played at, because the
48px test in docs/pawpocalypse-enemy-shape-bible.md is the one that matters and
the one most easily skipped. Only `--write` touches assets/.

THE THREE THINGS A WRITE MUST NOT CHANGE, because each is load-bearing
elsewhere: the filename (ArtStyle matches by basename), the on-disk pixel size
(the .import sidecars and the .tscn uids hang off the path, and the game fits
art to a target size assuming these dimensions), and the file's continued
existence (a missing file is a silent fallback to another style, which no gate
catches). This tool only ever overwrites in place.
"""

from __future__ import annotations

import argparse
import math
import pathlib
import sys

try:
    from PIL import Image, ImageDraw
except ImportError as exc:                      # pragma: no cover
    sys.exit("needs Pillow: py -m pip install pillow (%s)" % exc)

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import palette as PAL
import sweet_shapes as S
import sweet_evolutions as EVO

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
# ART LIVES IN A FOLDER NAMED FOR WHAT IT IS, since the 2026-09-11 restructure.
# Before that everything drawn here went to one "placeholder" directory, which
# was accurate when make_art.py drew eighteen shapes and stopped being accurate
# when this set became the game's art.
# PORT (Unreal, 2026-11-05): copied from the Godot repo's tools/, unchanged except the output paths: the art goes to
# SourceArt/svg/<Godot's assets/art folder>, or to $PAW_ART_ROOT when set (tools/art/check_art.py).
import os
ART = os.environ.get("PAW_ART_ROOT", "SourceArt/svg").replace("\\", "/")


def art(filename):
    """Route a filename to its folder by what it IS. One function rather than a
    constant per family, so adding an asset never means picking a path."""
    for prefix, folder in (("enemy_", "creatures"), ("boss_", "bosses"),
                           ("player_", "player"), ("kitten_", "player"),
                           ("ui_", "ui"), ("card_", "cards"),
                           ("skill_", "skills"), ("job_", "jobs"),
                           ("hat_", "jobs"),
                           ("prop_", "props")):
        if filename.startswith(prefix):
            return "%s/%s/%s" % (ART, folder, filename)
    raise ValueError("no folder for %r - add its prefix to art()" % filename)

# Every asset is (relative path, pixel size on disk, callable -> parts).
# The pixel size is READ OFF THE EXISTING FILES, never chosen here.


def _player():
    """Stages 1-3 are the cat plus something; 4-7 are the cat on a mount, and
    they are the ones that ship at 256 because a unicorn does not fit in 128
    units at the same visual weight as a cat."""
    e = EVO.EVOLUTIONS()
    return [
        (art("player_cat_base.svg"), 128, lambda: EVO.the_cat()),
        (art("player_cat_stage_1.svg"), 128, lambda: e["stage_1_tiny_kitten"]),
        (art("player_cat_stage_2.svg"), 128, lambda: e["stage_2_armored_tomcat"]),
        (art("player_cat_stage_3.svg"), 128, lambda: e["stage_3_cyber_cat"]),
        (art("player_cat_stage_4.svg"), 256, lambda: e["stage_4_rainbow_cavalry"]),
        (art("player_cat_stage_5.svg"), 256, lambda: e["stage_5_alicorn"]),
        (art("player_cat_stage_6.svg"), 256, lambda: e["stage_6_void_unicorn"]),
        (art("player_cat_stage_7.svg"), 256, lambda: e["stage_7_the_last_unicorn"]),
        (art("kitten_basic.svg"), 128, lambda: S.creature(S.CAST()["kitten"])),
    ]



# ------------------------------------------------------------ biome tokens ---
# SWITCHED OFF (designer, 2026-09-14: "some of the enemies seem to have icons
# next to them? Like a mouse has a leaf next to it - those should not be
# present"). The drawing functions and their colour checks are kept - a
# token is one flag away from coming back - but no creature is drawn with
# one. Biome copies of a creature are now identical, which is what the
# designer looked at and preferred.
BIOME_TOKENS = False

# A BIOME TOKEN IS HOW TWO COPIES OF ONE CREATURE DIFFER. enemy_backyard_mouse
# and enemy_kitchen_mouse are the same animal in the same palette - tinting them
# apart was tried and rejected (see palette.py) - so what separates them is a
# small piece of the place, sat at the creature's feet and overlapping the body
# so it reads as attached rather than stuck on.
#
# They are deliberately SMALL. Two copies of a creature are never on screen at
# the same time, so the token does not have to win a fight; it only has to make
# the biome's mouse feel like it belongs there.

# BESIDE THE CREATURE, NOT UNDER IT. The first placement tucked the token at the
# feet where the body covered it, so at 48px the backyard mouse and the kitchen
# mouse were the same picture - which is the entire thing the token exists to
# prevent. It sits clear of the body outline now, and it is half again as big,
# because a cue that cannot be seen at the size the game is played at is not a
# cue. The mouse's leftmost edge is its head at x=23, so x=18 breaks the
# silhouette rather than hiding inside it.
TOKEN_AT = (18.0, 100.0)
TOKEN_SCALE = 1.55

# TOKEN COLOURS ARE CHECKED, because the first set was not and three of the five
# were invisible on the exact ground they are meant to decorate: a green clover
# on green grass (21 apart), a butter crumb on a butter kitchen floor (18), a
# grey bolt on a warm factory floor (21). Hardcoding a colour into a drawing
# function is how art dodges the palette rules, so these live in a table that
# check_tokens() walks on every batch render.
# Each token is a Stacklands family (palette.py), checked against its floor
# below like every other colour.
TOKEN_COLOURS = {
    "clover": (PAL.header("green"), "tier_1_backyard"),
    "crumb":  (PAL.face("orange"), "tier_9_kitchen"),
    "leaf":   (PAL.header("green"), "tier_3_park"),
    "drip":   (PAL.face("green"), "tier_5_sewers"),
    "bolt":   (PAL.header("slate"), "tier_11_factory"),
    "can":    (PAL.face("red"), "tier_2_alley"),
}
LEAF_DARK = PAL.header("green")


def check_no_background_fills(items):
    """Refuse to write art that "cuts" a shape using a backdrop colour.

    THIS BUG HAS APPEARED THREE TIMES in one job - the signature mouse's ear
    notch, and twice in the icon set - and it is invisible in review precisely
    when the contact sheet happens to use the same background the colour was
    sampled from. It is never correct: these sprites move between twelve
    backdrops and sit on UI panels, so a "hole" painted in one biome's field
    colour is a coloured blob everywhere else. This grammar has no subtraction.
    If a shape needs a bite out of it, draw a different shape.
    """
    banned = {}
    for b, pair in PAL.BACKDROPS.items():
        banned[pair["field"].upper()] = b + " field"
        banned[pair["detail"].upper()] = b + " detail"
    bad = []
    for rel, px, fn in items:
        for part in fn():
            fill = part.get("fill")
            if isinstance(fill, str) and fill.upper() in banned:
                bad.append("%s paints with %s (%s) - background colours are not holes"
                           % (pathlib.Path(rel).stem, fill, banned[fill.upper()]))
    return bad


def check_tokens():
    bad = []
    for name, (colour, biome) in TOKEN_COLOURS.items():
        d = PAL.distance(colour, PAL.BACKDROPS[biome]["field"])
        if d < PAL.MIN_FROM_GROUND:
            bad.append("token %s (%s) is %.0f from %s (min %.0f)"
                       % (name, colour, d, biome, PAL.MIN_FROM_GROUND))
    return bad


def _clover():
    x, y = TOKEN_AT
    k = TOKEN_SCALE
    return S.fuse([S.ell(x - 5 * k, y, 5.5 * k, 5 * k, TOKEN_COLOURS["clover"][0]),
                   S.ell(x + 5 * k, y, 5.5 * k, 5 * k, TOKEN_COLOURS["clover"][0]),
                   S.ell(x, y - 6 * k, 5.5 * k, 5 * k, TOKEN_COLOURS["clover"][0])]) + [
        S.line([(x, y + 3 * k), (x, y + 11 * k)], LEAF_DARK, 2.4)]


def _crumb():
    x, y = TOKEN_AT
    k = TOKEN_SCALE
    return S.fuse([S.tri([(x - 8 * k, y + 6 * k), (x + 2 * k, y - 7 * k),
                          (x + 9 * k, y + 6 * k)], TOKEN_COLOURS["crumb"][0])]) + [
        S.ell(x + 1, y + 1, 3.0, 2.8, S.tint(TOKEN_COLOURS["crumb"][0], 0.45), s=False),
        S.ell(x - 4, y + 6, 2.4, 2.2, S.tint(TOKEN_COLOURS["crumb"][0], 0.45), s=False)]


def _leaf():
    x, y = TOKEN_AT
    k = TOKEN_SCALE
    return S.fuse([S.ell(x, y, 10 * k, 6 * k, TOKEN_COLOURS["leaf"][0])]) + [
        S.line([(x - 9 * k, y), (x + 9 * k, y)], LEAF_DARK, 2.2)]


def _drip():
    x, y = TOKEN_AT
    k = TOKEN_SCALE
    return S.fuse([S.ell(x, y + 2, 6 * k, 7.5 * k, TOKEN_COLOURS["drip"][0]),
                   S.ell(x, y - 5 * k, 3.2 * k, 4 * k, TOKEN_COLOURS["drip"][0])])


def _bolt_token():
    x, y = TOKEN_AT
    k = TOKEN_SCALE
    return S.fuse([S.rect(x, y, 13 * k, 12 * k, TOKEN_COLOURS["bolt"][0], r=3)]) + [
        S.ell(x, y, 3.4 * k, 3.4 * k, S.shade(TOKEN_COLOURS["bolt"][0], 0.3), s=False)]


def _can():
    """A crushed drink can for the alley.

    THE BUG THIS FIXES: the alley dog was handed _bolt_token(), which is the FACTORY
    token. Its colour had been checked against tier_11's warm floor and never
    against tier_2's pale blue one, so a token whose entire job is to say "you
    are in the alley" was saying "factory" in a colour nobody had validated for
    the place it actually stands. Tokens are per-biome; reusing one across
    biomes silently defeats both the identification and the colour check."""
    x, y = TOKEN_AT
    k = TOKEN_SCALE
    c = TOKEN_COLOURS["can"][0]
    return S.fuse([S.rect(x, y, 11 * k, 16 * k, c, r=3),
                   S.rect(x + 3 * k, y - 4 * k, 9 * k, 7 * k, c, r=2)]) + [
        S.line([(x - 4 * k, y + 3), (x + 4 * k, y + 3)], S.shade(c, 0.3), 2.0)]


def _sparkle(pts):
    out = []
    for cx, cy, r in pts:
        out.append(S.tri([(cx - r, cy), (cx, cy - r * 2.2), (cx + r, cy)], PAL.EYE, s=False))
        out.append(S.tri([(cx - r, cy), (cx, cy + r * 2.2), (cx + r, cy)], PAL.EYE, s=False))
    return out


def _spec(name, **over):
    spec = dict(S.CAST()[name])
    spec.update(over)
    return spec


def _mouse(token=None, **over):
    parts = S.creature(_spec("mouse", **over))
    return parts + (token() if token and BIOME_TOKENS else [])


def _rat(token=None, **over):
    parts = S.creature(_spec("rat", **over))
    return parts + (token() if token and BIOME_TOKENS else [])


def _flying_mouse():
    """Mouse plus moth wings.

    THE SAME MISTAKE AS THE UNICORN, MADE TWICE. The first pair sat at (58,62)
    and (74,50), which is inside the body, so the mouse covered them completely
    and the flier was indistinguishable from the common mouse. A wing has to
    BREAK THE OUTLINE. The body tops out around y=64 and ends near x=110, so
    these climb up and back out of both. Rooted under the body, tips in clear
    air - that is what makes it a wing and not a placard."""
    w = "#E4D2F2"
    far = S.fuse([S.ell(88, 52, 25, 16, w), S.ell(102, 36, 20, 13, w),
                  S.ell(112, 22, 15, 10, w)])
    # BEHIND THE HEAD, 2026-09-30. At (76,62) r21 the near wing reached back to
    # x=55 and sat on the face - the head is centred at x=48, r25 - covering
    # the right eye. From x=76 it clears the head and still breaks the back.
    near = S.fuse([S.ell(96, 58, 20, 13, S.shade(w, 0.12)),
                   S.ell(106, 44, 16, 10, S.shade(w, 0.12))])
    body = S.creature(S.CAST()["mouse"])
    return far + body + near


def _golden_mouse():
    g = _spec("mouse", fur=PAL.ENEMIES["golden_mouse"][0],
              mark=PAL.ENEMIES["golden_mouse"][1])
    return S.creature(g) + _sparkle([(96, 52, 4.5), (26, 60, 3.2), (74, 36, 3.8)])


def _signature_mouse():
    """The biome's named mouse. Bigger head, a NOTCHED EAR and a brow scar - all
    silhouette, because this one has to read as 'that one is different' at the
    same 48px as the forty around it."""
    parts = S.creature(_spec("mouse", eye_r=12, whisker_len=34))
    # A COWLICK, NOT A NOTCH. The first version cut the ear by painting a
    # triangle in tier_1's field colour. That is not a notch, it is a green
    # wedge: it only disappears on the one backdrop it was sampled from, and
    # this sprite is used in every biome. Never punch a hole with a background
    # colour in art that moves between backgrounds. The cue is additive now -
    # three fur spikes on the crown, which join the silhouette and read as a
    # scruffy elder at any size, on any ground.
    fur = PAL.ENEMIES["mouse"][0]
    parts += S.fuse([S.tri([(36, 50), (30, 26), (48, 44)], fur),
                     S.tri([(46, 46), (46, 20), (58, 42)], fur),
                     S.tri([(56, 46), (64, 24), (66, 48)], fur)])
    parts.append(S.line([(60, 54), (72, 46)], S.INK, 2.4))
    return parts


def _ghost_mouse():
    """No feet (in the spec) and a tail that frays into three strands instead of
    ending in a tip."""
    parts = S.creature(S.CAST()["ghost_mouse"])
    c = PAL.ENEMIES["ghost_mouse"][0]
    for dy in (-7, 0, 7):
        parts.append(S.curve(108, 100 + dy, 122, 92 + dy, 128, 104 + dy, w=4.0, colour=c))
    return parts


def _void_mouse():
    parts = S.creature(S.CAST()["void_mouse"])
    return parts + _sparkle([(78, 84, 3.0), (96, 96, 2.4), (62, 100, 2.6)]) + [
        S.ell(88, 74, 2.0, 2.0, PAL.EYE, s=False),
        S.ell(70, 68, 1.6, 1.6, PAL.EYE, s=False)]


def _bat():
    """The flier for biomes 4-12. Two wing fans of three triangles each, the
    same construction as the flying mouse's, on a round body with no tail and
    tall pointed ears - the silhouette says bat before the colour does."""
    c, m = PAL.ENEMIES["bat"]
    parts = S.fuse([S.tri([(14, 70), (30, 40), (44, 66)], c),
                    S.tri([(28, 74), (38, 34), (52, 64)], c),
                    S.tri([(42, 76), (54, 40), (58, 70)], c),
                    S.tri([(114, 70), (98, 40), (84, 66)], c),
                    S.tri([(100, 74), (90, 34), (76, 64)], c),
                    S.tri([(86, 76), (74, 40), (70, 70)], c),
                    S.ell(64, 74, 26, 24, c),
                    S.tri([(46, 58), (50, 30), (60, 56)], c),
                    S.tri([(68, 56), (78, 30), (82, 58)], c)])
    parts += [S.tri([(48, 58), (52, 40), (58, 57)], S.tint(m, 0.2), s=False),
              S.tri([(70, 57), (76, 40), (80, 58)], S.tint(m, 0.2), s=False),
              S.ell(64, 84, 12, 8, m, s=False)]
    parts += S.eyes(64, 70, 11, 9, asym=1.2)
    parts += [S.tri([(56, 86), (59, 94), (62, 86)], PAL.EYE, s=False),
              S.tri([(66, 86), (69, 94), (72, 86)], PAL.EYE, s=False)]
    return parts


def _ghost():
    """A sheet ghost. A tall dome that ends in three scallops, so it reads as
    cloth rather than as a body; no feet, no tail, nothing a mouse has."""
    c, m = PAL.ENEMIES["ghost"]
    parts = S.fuse([S.ell(64, 62, 34, 38, c),
                    S.rect(64, 90, 68, 30, c, r=2),
                    S.ell(42, 106, 12, 12, c), S.ell(64, 110, 12, 12, c), S.ell(86, 106, 12, 12, c)])
    parts += [S.ell(52, 80, 7, 5, m, s=False), S.ell(76, 80, 7, 5, m, s=False)]
    parts += S.eyes(64, 62, 13, 10, asym=1.25)
    parts.append(S.ell(64, 80, 5, 6, S.INK, s=False))
    return parts


def _imp():
    """A flame-shaped spirit: a teardrop body with two horn tips and a flicker
    on top, gold eyes. The Mythic Realm's common, replacing the void mouse."""
    c, m = PAL.ENEMIES["imp"]
    parts = S.fuse([S.ell(64, 82, 30, 30, c),
                    S.tri([(38, 70), (64, 8), (90, 70)], c),
                    S.tri([(52, 40), (58, 16), (66, 44)], c)])
    parts += [S.ell(64, 96, 14, 9, S.tint(c, 0.35), s=False),
              S.tri([(40, 78), (28, 60), (46, 66)], m, s=False),
              S.tri([(88, 78), (100, 60), (82, 66)], m, s=False)]
    parts += S.eyes(64, 74, 12, 9, asym=1.2)
    parts.append(S.curve(56, 92, 64, 98, 72, 92, w=2.4))
    return parts


def _ant():
    """Three beads in a row, six legs, two antennae. The Kitchen's common:
    a pantry has ants, and an ant is nothing like a mouse from any angle."""
    c, m = PAL.ENEMIES["ant"]
    parts = S.fuse([S.ell(34, 84, 18, 15, c), S.ell(64, 86, 15, 13, c), S.ell(96, 82, 21, 17, c)])
    parts.append(S.ell(96, 90, 11, 6, S.tint(c, 0.35), s=False))
    for x in (46, 62, 78):
        parts.append(S.line([(x, 96), (x - 8, 116)], S.INK, 2.2))
        parts.append(S.line([(x, 96), (x + 8, 116)], S.INK, 2.2))
    parts.append(S.curve(24, 72, 10, 50, 14, 40, w=2.0))
    parts.append(S.curve(38, 70, 36, 46, 44, 38, w=2.0))
    parts += [S.ell(14, 40, 3, 3, m), S.ell(44, 38, 3, 3, m)]
    parts += S.eyes(34, 80, 8, 7, asym=1.2)
    return parts


def _nightlife():
    return [
        (art("enemy_kitchen_ant.svg"), 128, _ant),
        (art("enemy_bat.svg"), 128, _bat),
        (art("enemy_portal_ghost.svg"), 128, _ghost),
        (art("enemy_mythic_imp.svg"), 128, _imp),
    ]


def _robot_mouse():
    """SQUARE EVERYWHERE. The bible gives the robot mouse 'square blobs, not
    round' as its one cue, and it is the only creature in the game built from
    rectangles - which is also why sweet_shapes grew a rect primitive."""
    c, m = PAL.ENEMIES["robot_mouse"]
    body = S.fuse([S.rect(80, 92, 52, 40, c, r=5), S.rect(48, 68, 44, 40, c, r=5),
                   S.rect(28, 46, 17, 17, c, r=3), S.rect(68, 46, 17, 17, c, r=3),
                   S.rect(60, 118, 13, 12, c, r=2), S.rect(94, 118, 13, 12, c, r=2)])
    parts = body + [
        S.rect(28, 46, 8, 8, m, r=2, s=False), S.rect(68, 46, 8, 8, m, r=2, s=False),
        S.rect(76, 100, 34, 16, S.tint(c, 0.55), r=3, s=False),
        S.line([(110, 86), (126, 74)], S.INK, 2.6),
        S.ell(126, 72, 4.0, 4.0, m),
    ]
    parts += S.eyes(48, 66, 13, 11, asym=1.22)
    parts.append(S.rect(48, 84, 14, 5, m, r=1, s=False))
    return parts


def _slime_rat():
    parts = S.creature(S.CAST()["slime_rat"])
    c = PAL.ENEMIES["slime_rat"][0]
    for x, r in ((54, 6.0), (78, 7.5), (100, 5.5)):
        parts.append(S.ell(x, 112, r, r * 1.5, c))
        parts.append(S.ell(x, 122, r * 0.6, r * 0.8, c))
    return parts


def _vermin():
    return [
        (art("enemy_mouse.svg"), 128, lambda: _mouse()),
        (art("enemy_backyard_mouse.svg"), 128, lambda: _mouse(_clover)),
        (art("enemy_mouse_signature.svg"), 128, _signature_mouse),
        (art("enemy_mouse_flying.svg"), 128, _flying_mouse),
        (art("enemy_mouse_golden.svg"), 128, _golden_mouse),
        # The ghost mouse and the void mouse left the game on 2026-09-14 - no
        # mouse past biome 3. Their drawings stay below; see _nightlife().
        (art("enemy_factory_robot_mouse.svg"), 128, _robot_mouse),
        (art("enemy_rat.svg"), 128, lambda: _rat()),
        (art("enemy_park_rat.svg"), 128, lambda: _rat(_leaf)),
        (art("enemy_sewer_rat.svg"), 128, lambda: _rat(_drip)),
        (art("enemy_factory_rat.svg"), 128, lambda: _rat(_bolt_token)),
        (art("enemy_turf_slime_rat.svg"), 128, _slime_rat),
    ]



# --------------------------------------------------------------- gristle ----
# THE HARDEST ASSET IN THE GAME, because there are twelve of it and it is the
# one creature with no shape of its own. The bible calls it "not an animal -
# meat", lumpy and asymmetric, and gives it NO cue at all. So the usual answer
# (give each biome variant a different silhouette feature) has nothing to attach
# to, and the previous answer (ship one blob twelve times) is what this whole
# job exists to undo.
#
# TWO THINGS VARY, AND BOTH ARE HONEST TO WHAT GRISTLE IS.
#
#   1. MASS AND MENACE CLIMB WITH THE TIER. Gristle is the one enemy the player
#      meets in every biome for fifty hours, so the tier_12 gristle being half
#      again the size of the tier_1 one, and spikier, is the only progression
#      the player can actually see in a creature that never changes.
#   2. THE LUMPS ARE SEEDED BY BIOME NAME. Formless is the point: a creature
#      with no correct arrangement can be rearranged freely, and a deterministic
#      seed means the backyard gristle is the same backyard gristle every run
#      without anyone authoring twelve blob clusters by hand.

GRISTLE_TIERS = ["tier_1_backyard", "tier_2_alley", "tier_3_park",
                 "tier_4_rooftops", "tier_5_sewers", "tier_6_turf",
                 "tier_7_portal", "tier_8_mythic", "tier_9_kitchen",
                 "tier_10_vets", "tier_11_factory", "tier_12_litterbox"]


def _gristle(biome):
    """THE FIRST VERSION CAME OUT CUTE, which for this creature is the one
    unacceptable result. It had a round head-ball perched on a round body with
    big high eyes, and twelve adorable chicks is not what "not an animal - meat"
    means. Three things were wrong and all three were structural:

      * THE HEAD WAS A SEPARATE SPHERE. Gristle has no head. It has an END that
        happens to have eyes in it. A distinct ball on top of a distinct body is
        the universal read for "animal", so the mass is now ONE low wide lump
        with the eyes set into its front, no neck, no join.
      * THE EYES WERE TOO BIG AND TOO HIGH. Large eyes high on a round form is
        the exact recipe for a baby bird. They are small now, set low and deep
        in the mass, and still the widest mismatch in the game at 1.35.
      * IT STOOD TALL. Meat sags. The silhouette is wider than it is high now,
        which is also what keeps it from colliding with the vole in tier_2 -
        the vole is a squat ROUND potato, this is a squat WIDE slab.
    """
    import math
    import random
    i = GRISTLE_TIERS.index(biome)
    t = i / 11.0                       # 0 at the backyard, 1 in the litterbox
    rng = random.Random("gristle:" + biome)

    fur, mark = PAL.ENEMIES["gristle"]
    grow = 0.84 + 0.34 * t
    cx, cy = 70.0, 96.0

    # One low wide slab, then lumps that break its edge. Wider than tall.
    lumps = [S.ell(cx, cy, 42 * grow, 22 * grow, fur)]
    for _ in range(4 + int(round(4 * t))):
        a = rng.uniform(0, 6.283)
        r = rng.uniform(0.6, 1.0)
        lumps.append(S.ell(cx + 36 * grow * r * math.cos(a),
                           cy + 17 * grow * r * math.sin(a),
                           rng.uniform(10, 20) * grow,
                           rng.uniform(8, 15) * grow, fur))

    # Stubby feet, inside the union so the thing sits on the ground rather than
    # floating above a stray ellipse - which is what the first version did.
    for fx in (52, 84):
        lumps.append(S.ell(fx, 114, 9 * grow, 6, fur))

    # Spikes climb with the tier: none in the backyard, a ridge by the litterbox.
    for _ in range(int(round(6 * t))):
        a = rng.uniform(3.4, 6.1)
        bx = cx + 34 * grow * math.cos(a)
        by = cy + 18 * grow * math.sin(a)
        lumps.append(S.tri([(bx - 7, by + 4), (bx + rng.uniform(-7, 7), by - 20 * grow),
                            (bx + 7, by + 4)], fur))
    parts = S.fuse(lumps)

    parts.append(S.ell(cx - 2, cy + 9, 27 * grow, 11 * grow, S.tint(fur, 0.55), s=False))
    for _ in range(3 + int(round(3 * t))):
        parts.append(S.ell(cx + rng.uniform(-30, 30) * grow,
                           cy + rng.uniform(-14, 12) * grow,
                           rng.uniform(4, 9), rng.uniform(4, 7), mark, s=False))

    # Eyes set INTO the front of the mass - small, low, and badly matched.
    ex, ey = cx - 30 * grow, cy - 8 * grow
    parts += S.eyes(ex, ey, 10, 6.5, asym=1.35, pupil=0.30)
    parts.append(S.ell(ex - 2, ey + 13, 3.4, 2.6, S.shade(fur, 0.55), s=False))
    parts.append(S.curve(ex - 9, ey + 17, ex - 1, ey + 21, ex + 8, ey + 16, w=1.8))
    return parts


def _gristles():
    out = []
    for b in GRISTLE_TIERS:
        short = b.split("_", 2)[2]
        name = {"backyard": "backyard", "alley": "alley", "park": "park",
                "rooftops": "rooftop", "sewers": "sewer", "turf": "turf",
                "portal": "portal", "mythic": "mythic", "kitchen": "kitchen",
                "vets": "vet", "factory": "factory", "litterbox": "litterbox"}[short]
        out.append((art("enemy_%s_gristle.svg" % name), 128,
                    (lambda bb: (lambda: _gristle(bb)))(b)))
    out.append((art("enemy_alley_vole.svg"), 128,
                lambda: S.creature(S.CAST()["vole"])))
    return out



# ----------------------------------------------------------------- birds ----
# Four cues, no two alike, none of them colour: goose = NECK, pigeon = NO NECK,
# seagull = WINGS OUT, parrot = HOOKED BEAK + CREST, crow = CORNERS.


def _goose(token=None):
    parts = S.creature(S.CAST()["goose"])
    return parts + (token() if token and BIOME_TOKENS else [])


def _parrot():
    """Crest and hook. Both are added here rather than in the spec because a
    crest is a fan of triangles and a hook is a curve, and the spec grammar
    speaks in blobs."""
    fur, mark = PAL.ENEMIES["parrot"]
    crest = S.fuse([S.tri([(42, 40), (34, 12), (52, 34)], mark),
                    S.tri([(50, 36), (50, 8), (62, 32)], mark),
                    S.tri([(58, 38), (68, 16), (68, 40)], mark)])
    parts = crest + S.creature(S.CAST()["parrot"])
    beak = S.fuse([S.tri([(34, 46), (12, 56), (34, 62)], PAL.GOLD)])
    parts += beak
    parts.append(S.curve(20, 54, 24, 64, 32, 62, w=2.4))
    return parts


def _crow():
    """Folded wings as hard triangles - the one bird outline with corners."""
    fur, mark = PAL.ENEMIES["crow"]
    parts = S.fuse([S.tri([(58, 70), (104, 66), (86, 104)], fur),
                    S.tri([(62, 78), (112, 88), (74, 106)], fur)])
    parts = parts + S.creature(S.CAST()["crow"])
    parts += S.fuse([S.tri([(34, 50), (6, 58), (34, 64)], S.shade(fur, 0.45))])
    parts.append(S.tri([(96, 92), (128, 104), (94, 104)], mark, s=True))
    return parts


def _birds():
    return [
        (art("enemy_goose.svg"), 128, lambda: _goose()),
        (art("enemy_park_goose.svg"), 128, lambda: _goose(_leaf)),
        (art("enemy_rooftop_pigeon.svg"), 128,
         lambda: S.creature(S.CAST()["pigeon"])),
        (art("enemy_rooftop_seagull.svg"), 128,
         lambda: S.creature(S.CAST()["seagull"])),
        (art("enemy_vet_parrot.svg"), 128, _parrot),
        (art("enemy_mythic_crow.svg"), 128, _crow),
    ]



# ------------------------------------------------------ bugs and odd ones ----


def _moth(token=None):
    parts = S.creature(S.CAST()["moth"])
    return parts + (token() if token and BIOME_TOKENS else [])


def _squirrel():
    """The bible gives the squirrel a plume tail LARGER THAN ITS BODY arcing over
    the back. The spec's `behind` tail was a modest curl, which is a squirrel
    with a tail rather than a tail with a squirrel."""
    fur, mark = PAL.ENEMIES["squirrel"]
    plume = S.fuse([S.ell(104, 88, 20, 22, fur), S.ell(110, 60, 19, 20, fur),
                    S.ell(100, 34, 17, 17, fur), S.ell(80, 22, 15, 14, fur)])
    return plume + S.creature(S.CAST()["squirrel"])


def _sludge_cat():
    """Sags: drips off the bottom, and the ears droop rather than stand."""
    parts = S.creature(S.CAST()["sludge_cat"])
    c = PAL.ENEMIES["sludge_cat"][0]
    for x, r in ((58, 6.5), (82, 8.0), (104, 5.5)):
        parts.append(S.ell(x, 114, r, r * 1.6, c))
        parts.append(S.ell(x, 124, r * 0.55, r * 0.75, c))
    return parts


def _oddities():
    return [
        (art("enemy_dog.svg"), 128, lambda: S.creature(S.CAST()["dog"])),
        (art("enemy_alley_dog.svg"), 128,
         lambda: S.creature(S.CAST()["dog"]) + _can()),
        (art("enemy_portal_hound.svg"), 128,
         lambda: S.creature(S.CAST()["hound"])),
        (art("enemy_moth.svg"), 128, lambda: _moth()),
        (art("enemy_backyard_moth.svg"), 128, lambda: _moth(_clover)),
        (art("enemy_kitchen_cockroach.svg"), 128,
         lambda: S.creature(S.CAST()["cockroach"])),
        (art("enemy_turf_toad.svg"), 128, lambda: S.creature(S.CAST()["toad"])),
        (art("enemy_sewer_crocodile.svg"), 128,
         lambda: S.creature(S.CAST()["crocodile"])),
        (art("enemy_vet_hamster.svg"), 128,
         lambda: S.creature(S.CAST()["hamster"])),
        (art("enemy_squirrel.svg"), 128, _squirrel),
        (art("enemy_litterbox_dust_bunny.svg"), 128,
         lambda: S.creature(S.CAST()["dust_bunny"])),
        (art("enemy_litterbox_sludge_cat.svg"), 128, _sludge_cat),
    ]



# ----------------------------------------------------------------- bosses ---
# BOSSES GET TWO CUES, not one. They appear alone, with the music and the health
# bar, and a creature that only differs from a common enemy by size has not
# earned that. All seven ship at 256px on disk.


def _tyrant():
    """Goose NECK plus twice the MASS. Drawn as a goose whose body is scaled up
    around the neck root rather than as a new animal - the player has fought
    geese for two biomes and the joke is that this is simply a bigger one."""
    b, m = PAL.BOSSES["park_tyrant"]
    body = S.fuse([S.ell(76, 96, 46, 30, b), S.ell(58, 70, 18, 32, b),
                   S.ell(48, 40, 15, 28, b), S.ell(44, 20, 19, 15, b),
                   S.ell(96, 118, 12, 7, b), S.ell(62, 120, 12, 7, b)])
    parts = body
    parts.append(S.ell(78, 104, 32, 17, S.tint(b, 0.55), s=False))
    parts += S.fuse([S.tri([(30, 18), (6, 24), (30, 30)], PAL.GOLD)])
    parts += S.eyes(44, 16, 9, 8, asym=1.3)
    for i in range(3):
        parts.append(S.ell(52 + i * 6, 44 + i * 14, 6, 5, m, s=False))
    return parts


def _champion():
    """THE CAT SILHOUETTE, WRONG. Same animal the player is, in another coat,
    with a split ear and a scar. Both are ADDITIVE - the split is drawn as two
    tips rather than by punching a notch in the background colour, which is the
    mistake the signature mouse made."""
    b, m = PAL.BOSSES["rival_champion"]
    spec = dict(S.CAST()["player_cat"])
    spec["fur"] = b
    spec["mark"] = m
    parts = S.creature(spec)
    # ONE extra prong, not two. The cat spec already draws a pair of ears; adding
    # two more gave the champion four, which reads as a mistake rather than as a
    # torn ear. A single prong beside the existing left ear forks it.
    parts += S.fuse([S.tri([(22, 32), (12, 8), (30, 26)], b)])
    parts.append(S.line([(36, 40), (52, 56)], S.INK, 3.0))
    parts.append(S.line([(40, 36), (44, 44)], S.INK, 2.2))
    return parts


def _foreman():
    """Gristle MASS plus hard angular PLATES. The plates are rectangles on a
    creature made entirely of lumps, which is the whole read: something has been
    bolted to the meat."""
    b, m = PAL.BOSSES["factory_foreman"]
    lumps = [S.ell(70, 94, 46, 28, b), S.ell(38, 84, 22, 20, b),
             S.ell(100, 86, 24, 22, b), S.ell(74, 68, 22, 18, b),
             S.ell(52, 116, 12, 7, b), S.ell(88, 116, 12, 7, b)]
    parts = S.fuse(lumps)
    parts.append(S.ell(68, 102, 32, 13, S.tint(b, 0.5), s=False))
    parts += S.fuse([S.rect(44, 62, 34, 16, "#B9BFC9", r=2),
                     S.rect(96, 64, 28, 14, "#B9BFC9", r=2)])
    parts.append(S.rect(70, 92, 40, 12, "#8A93A2", r=2))
    for x in (54, 70, 86):
        parts.append(S.ell(x, 92, 2.6, 2.6, S.shade("#8A93A2", 0.3), s=False))
    # Bigger and clear of the plating. At 8px on a mass this size the eyes
    # disappeared entirely and the foreman read as furniture, not a creature.
    parts += S.eyes(42, 80, 15, 11, asym=1.35, pupil=0.3)
    parts.append(S.ell(42, 98, 4.0, 3.0, S.shade(b, 0.5), s=False))
    return parts


def _litter_king():
    """Round, plus THE ONLY CROWN IN THE GAME."""
    b, m = PAL.BOSSES["litter_king"]
    parts = S.fuse([S.ell(66, 92, 44, 34, b), S.ell(66, 56, 30, 26, b),
                    S.ell(46, 120, 13, 7, b), S.ell(88, 120, 13, 7, b)])
    parts.append(S.ell(66, 100, 32, 20, S.tint(b, 0.55), s=False))
    crown = S.fuse([S.rect(66, 26, 46, 12, PAL.GOLD, r=2),
                    S.tri([(46, 28), (44, 4), (58, 26)], PAL.GOLD),
                    S.tri([(58, 26), (66, 0), (74, 26)], PAL.GOLD),
                    S.tri([(74, 26), (88, 4), (86, 28)], PAL.GOLD)])
    parts += crown
    for x in (52, 66, 80):
        parts.append(S.ell(x, 26, 3.0, 3.0, m, s=False))
    parts += S.eyes(66, 54, 14, 11, asym=1.3)
    parts.append(S.ell(66, 70, 4.0, 3.0, S.shade(b, 0.5), s=False))
    return parts


def _void_god():
    """NO FIXED SILHOUETTE - the bible's words. The outline itself should be
    wrong, so this is the one creature with no symmetry, no ground contact and
    no repeated part: spikes of different lengths at uneven angles, and eyes
    that do not pair."""
    import math
    import random
    b, m = PAL.BOSSES["void_god"]
    rng = random.Random("void_god")
    lumps = [S.ell(66, 76, 38, 34, b)]
    for i in range(9):
        a = i * 0.698 + rng.uniform(-0.2, 0.2)
        reach = rng.uniform(34, 58)
        bx, by = 66 + reach * math.cos(a) * 0.95, 76 + reach * math.sin(a) * 0.8
        lumps.append(S.tri([(66 + 18 * math.cos(a - 0.5), 76 + 18 * math.sin(a - 0.5)),
                            (bx, by),
                            (66 + 18 * math.cos(a + 0.5), 76 + 18 * math.sin(a + 0.5))], b))
    parts = S.fuse(lumps)
    for _ in range(6):
        parts.append(S.ell(66 + rng.uniform(-26, 26), 76 + rng.uniform(-24, 24),
                           rng.uniform(4, 9), rng.uniform(4, 8), m, s=False))
    # Three eyes, no two alike and none in a pair.
    for ex, ey, er in ((52, 64, 12), (80, 74, 8), (64, 92, 6)):
        parts.append(S.ell(ex, ey, er, er, PAL.EYE))
        parts.append(S.ell(ex + er * 0.1, ey + er * 0.1, er * 0.3, er * 0.3, S.INK, s=False))
    for sx, sy, sr in ((30, 40, 2.6), (104, 48, 2.2), (44, 110, 2.4), (96, 104, 2.0)):
        parts.append(S.ell(sx, sy, sr, sr, PAL.EYE, s=False))
    return parts


def _boss_generic():
    """boss.tscn's default texture: a plain heavy lump with a boss's proportions
    and none of anyone's cues, so that if it ever shows up in play it reads as
    'a boss whose art did not load' rather than as a specific character."""
    b, m = PAL.BOSSES["generic"]
    parts = S.fuse([S.ell(66, 92, 44, 32, b), S.ell(66, 54, 28, 25, b),
                    S.ell(46, 120, 13, 7, b), S.ell(88, 120, 13, 7, b)])
    parts.append(S.ell(66, 100, 32, 18, S.tint(b, 0.55), s=False))
    parts += S.eyes(66, 52, 13, 10, asym=1.3)
    parts.append(S.ell(66, 68, 4.0, 3.0, S.shade(b, 0.5), s=False))
    parts.append(S.curve(56, 74, 66, 80, 76, 74, w=2.2))
    return parts


def _boss_add():
    """The minions a boss summons. Deliberately a LESSER version of the same
    idea - small, round, two stubby legs - because an add that reads as
    important steals the boss's moment."""
    b, m = PAL.BOSSES["add"]
    parts = S.fuse([S.ell(64, 82, 30, 26, b), S.ell(50, 112, 10, 6, b),
                    S.ell(78, 112, 10, 6, b)])
    parts.append(S.ell(64, 88, 20, 12, S.tint(b, 0.55), s=False))
    parts += S.eyes(64, 78, 11, 8, asym=1.3)
    parts.append(S.ell(64, 92, 3.2, 2.4, S.shade(b, 0.5), s=False))
    return parts


def _bosses():
    return [
        (art("boss_park_tyrant.svg"), 256, _tyrant),
        (art("boss_rival_champion.svg"), 256, _champion),
        (art("boss_factory_foreman.svg"), 256, _foreman),
        (art("boss_litter_king.svg"), 256, _litter_king),
        (art("boss_void_god.svg"), 256, _void_god),
        (art("boss_generic.svg"), 256, _boss_generic),
        (art("boss_add.svg"), 256, _boss_add),
    ]



# ------------------------------------------------------- currency icons -----
# THE SHAPES ARE NOT UP FOR REDESIGN. A player who has spent forty hours reading
# "leaf = catnip, star = stardust, fish = bait" does not benefit from a cleverer
# icon; changing the silhouette of a currency is a usability change wearing an
# art change's clothes. So these keep make_art.py's three identities exactly and
# only change HOW they are drawn - fused silhouette, one ink outline, one shade.
#
# The colours follow palette.py rather than the old file: catnip is GOLD there
# (the palette names it "catnip gold") and stardust is the purple STARDUST.

BAIT = PAL.MATERIALS["bait"]


def _icon_catnip():
    """ONE bold pointed leaf with a vein.

    The first version was a sprig of three lobes around a stem. At full size it
    was a pretty plant; at 32px - which is the only size this is ever seen, in
    the HUD beside a number - it collapsed into a gold plus sign. A currency
    icon has exactly one job at one size, so this is a single leaf: a body
    ellipse fused to a point, which still reads as a leaf when it is 32 pixels
    of gold.
    """
    g = PAL.GOLD
    parts = S.fuse([S.ell(64, 68, 27, 32, g),
                    S.tri([(40, 52), (64, 8), (88, 52)], g),
                    S.tri([(46, 88), (64, 116), (82, 88)], g)])
    parts.append(S.line([(64, 108), (64, 26)], S.shade(g, 0.42), 4.0))
    for y, dx in ((84, 20), (68, 23), (52, 19)):
        parts.append(S.line([(64, y + 8), (64 - dx, y - 4)], S.shade(g, 0.42), 2.6))
        parts.append(S.line([(64, y + 8), (64 + dx, y - 4)], S.shade(g, 0.42), 2.6))
    return parts


def _icon_stardust():
    """An eight-point star. Built as two squashed diamonds plus a core so the
    union has the star's outline without needing a polygon primitive."""
    c = PAL.STARDUST
    pts = S.fuse([S.tri([(64, 8), (48, 64), (80, 64)], c),
                  S.tri([(64, 120), (48, 64), (80, 64)], c),
                  S.tri([(8, 64), (64, 48), (64, 80)], c),
                  S.tri([(120, 64), (64, 48), (64, 80)], c),
                  S.ell(64, 64, 22, 22, c)])
    parts = pts
    parts.append(S.ell(64, 64, 12, 12, S.tint(c, 0.6), s=False))
    parts.append(S.ell(58, 58, 4.0, 4.0, PAL.EYE, s=False))
    return parts


def _icon_bait():
    """A fish: body, tail wedge, one eye. The tail is fused so the fish is one
    shape, which is what stops it reading as a balloon with a flag."""
    parts = S.fuse([S.ell(58, 64, 38, 25, BAIT),
                    S.tri([(96, 64), (124, 42), (124, 86)], BAIT)])
    parts.append(S.ell(54, 70, 26, 13, S.tint(BAIT, 0.55), s=False))
    parts.append(S.curve(84, 50, 92, 64, 84, 78, w=2.4))
    parts.append(S.ell(34, 56, 6.5, 6.5, PAL.EYE))
    parts.append(S.ell(34, 57, 2.4, 2.4, S.INK, s=False))
    return parts


def _icons():
    return [
        (art("ui_icon_catnip.svg"), 128, _icon_catnip),
        (art("ui_icon_stardust.svg"), 128, _icon_stardust),
        (art("ui_icon_bait.svg"), 128, _icon_bait),
        (art("ui_cursor_paw_open.svg"), 64, lambda: _cursor_paw(False)),
        (art("ui_cursor_paw_closed.svg"), 64, lambda: _cursor_paw(True)),
    ]


# THE PAW CURSOR (designer, 2026-10-01: "can we make a paw cursor that clutches
# fingers when pressed?"). Shown over the menus only - over the field the cat
# IS the cursor and the pointer hides. Open: four toes spread. Pressed: the
# toes pulled in tight and the claws out, because a cat closing its paw on a
# button is not a gentle animal. The hotspot is the top of the middle toes
# (InputDevice.PAW_HOTSPOT), so both drawings keep their toes on one line.
def _cursor_paw(closed):
    pad = PAL.MATERIALS["paper"]
    bean = PAL.CAT_NOSE
    if not closed:
        out = S.fuse([S.ell(64, 84, 36, 30, pad)]) + [S.ell(64, 84, 36, 30, pad)]
        for dx, dy, r in ((-40, 50, 14), (-15, 34, 15), (15, 34, 15), (40, 50, 14)):
            out += S.fuse([S.ell(64 + dx, dy, r, r * 1.1, pad)]) + [S.ell(64 + dx, dy, r, r * 1.1, pad)]
        out += [S.ell(64, 90, 18, 13, bean, s=False)]
        for dx, dy in ((-40, 52), (-15, 36), (15, 36), (40, 52)):
            out += [S.ell(64 + dx, dy, 6, 6, bean, s=False)]
        return out
    out = []
    # Claws first, so the toes overlap their roots.
    for dx in (-30, -10, 10, 30):
        out += S.fuse([S.tri([(64 + dx - 5, 44), (64 + dx + 5, 44), (64 + dx + 1, 22)], PAL.EYE)])
        out += [S.tri([(64 + dx - 5, 44), (64 + dx + 5, 44), (64 + dx + 1, 22)], PAL.EYE)]
    out += S.fuse([S.ell(64, 80, 38, 32, pad)]) + [S.ell(64, 80, 38, 32, pad)]
    for dx in (-30, -10, 10, 30):
        out += S.fuse([S.ell(64 + dx, 52, 12, 11, pad)]) + [S.ell(64 + dx, 52, 12, 11, pad)]
    out += [S.ell(64, 88, 18, 12, bean, s=False)]
    return out



# ======================================================= THE FLAT ICONS ======
# A DIFFERENT PROBLEM FROM CREATURES, and worth saying why before any of them
# are drawn. A creature is seen in motion, at 48px, against a coloured ground,
# forty at a time, and its job is to be TOLD APART from other creatures. An icon
# is seen still, at 32-64px, on a UI panel, one at a time, and its job is to be
# RECOGNISED - to say "this is the one that burns things" before the player has
# read the label.
#
# So the rules invert. Creatures need silhouette variety above all; icons need
# ONE unmistakable object each, drawn as large as the frame allows, in no more
# than three colours. Detail is the enemy - everything here is checked at the
# size it is used, which for the 64px families is half the drawing space.
#
# All icons are drawn in the same 128-unit space as everything else and emitted
# at whatever pixel size their family ships at, so none of the drawing code has
# to know that skills are 64 and cards are 256.

# Every material is a Stacklands family (palette.MATERIALS), so a skill icon's
# steel is the stone card's slate and its wood is the berry card's orange.
METAL = PAL.MATERIALS["metal"]
METAL_DARK = PAL.MATERIALS["metal_dark"]
WOOD = PAL.MATERIALS["wood"]
WOOD_DARK = PAL.MATERIALS["wood_dark"]
CLOTH = PAL.MATERIALS["cloth"]
CLOTH_DARK = PAL.MATERIALS["cloth_dark"]
WARM = PAL.MATERIALS["warm"]
PAPER_W = PAL.MATERIALS["paper"]
FLAME = PAL.MATERIALS["flame"]
LEAF_G = PAL.MATERIALS["leaf"]


def _burst(cx, cy, r, colour, n=8):
    """A radiating star burst - the icon set's "impact" mark."""
    import math
    out = []
    for i in range(n):
        a = i * (6.283 / n)
        out.append(S.tri([(cx + math.cos(a + 0.28) * r * 0.34,
                           cy + math.sin(a + 0.28) * r * 0.34),
                          (cx + math.cos(a) * r, cy + math.sin(a) * r),
                          (cx + math.cos(a - 0.28) * r * 0.34,
                           cy + math.sin(a - 0.28) * r * 0.34)], colour))
    return S.fuse(out)


# ------------------------------------------------------------- the hats -----
# A hat is worn BY THE CAT IN THE WORLD (JobData.hat_path), so it is the hat
# alone - no head under it, nothing else in the frame - and it is drawn wide and
# low so it reads as sitting ON something.

def _hat_blacksmith():
    return (S.fuse([S.rect(64, 76, 92, 18, WOOD_DARK, r=5),
                    S.rect(64, 54, 58, 28, WOOD, r=6)])
            + [S.ell(44, 50, 13, 11, METAL), S.ell(84, 50, 13, 11, METAL),
               S.rect(64, 50, 22, 6, METAL_DARK, r=2)])


def _hat_daredevil():
    return (S.fuse([S.ell(64, 62, 44, 38, WARM), S.rect(64, 84, 84, 14, WARM, r=5)])
            + [S.rect(64, 44, 14, 44, PAPER_W, r=3),
               S.rect(64, 84, 84, 8, S.shade(WARM, 0.3), r=3)])


def _hat_engineer():
    return (S.fuse([S.ell(64, 66, 40, 32, PAL.GOLD), S.rect(64, 84, 92, 14, PAL.GOLD, r=5)])
            + [S.rect(64, 46, 12, 30, S.shade(PAL.GOLD, 0.28), r=3),
               S.rect(64, 84, 92, 7, S.shade(PAL.GOLD, 0.28), r=3)])


def _hat_magician():
    return (S.fuse([S.rect(64, 48, 56, 68, PAL.STARDUST, r=5),
                    S.rect(64, 88, 100, 16, PAL.STARDUST, r=6)])
            + [S.rect(64, 72, 58, 12, PAL.GOLD, r=2)]
            + _burst(64, 26, 15, PAL.GOLD, 5))


def _hat_town_crier():
    return (S.fuse([S.tri([(10, 82), (64, 34), (118, 82)], CLOTH),
                    S.rect(64, 84, 106, 16, CLOTH, r=7)])
            + [S.rect(64, 84, 106, 8, CLOTH_DARK, r=3)]
            # A FEATHER, not an egg. The first one was a lone upright ellipse
            # floating beside the hat with nothing joining it, which at 64px
            # read as an egg someone had left there. It is raked now, tapered to
            # a quill, and rooted INSIDE the crown so it is part of the hat.
            + S.fuse([S.ell(96, 46, 8, 17, PAPER_W), S.ell(104, 30, 6, 12, PAPER_W),
                      S.ell(88, 62, 7, 13, PAPER_W)])
            + [S.line([(84, 72), (108, 24)], S.shade(PAPER_W, 0.3), 2.6)])


def _hat_unemployed():
    """A nightcap: a cone that FLOPS, with a bobble on the end. The first version
    was a rounded rectangle with a lump beside it and read as neither a hat nor
    anything else - a droop needs a taper and a tip, not a rounded box."""
    return (S.fuse([S.rect(64, 86, 84, 22, LEAF_G, r=9),
                    S.tri([(30, 82), (70, 22), (86, 84)], LEAF_G),
                    S.ell(84, 30, 15, 13, LEAF_G),
                    S.ell(104, 44, 11, 10, LEAF_G)])
            + [S.ell(110, 50, 12, 11, S.tint(LEAF_G, 0.55))]
            + [S.rect(64, 92, 86, 9, S.shade(LEAF_G, 0.28), r=4)])


# ------------------------------------------------------------- the jobs -----
# The job icon is a LIST icon, so it is the job's tool rather than a cat wearing
# the hat - a row of six near-identical cats in six hats is a row of six cats.

def _job_blacksmith():
    """A hammer. The description is 'what you can touch gets flattened'."""
    return (S.fuse([S.rect(70, 40, 66, 30, METAL, r=6), S.rect(46, 40, 22, 40, METAL, r=5)])
            + [S.rect(70, 40, 66, 10, METAL_DARK, r=3)]
            + S.fuse([S.rect(64, 88, 16, 54, WOOD, r=5)])
            + [S.rect(64, 104, 16, 8, WOOD_DARK, r=2)])


def _job_daredevil():
    """A crater. 'Commits absolutely... then you stand in the crater.'"""
    return (_burst(64, 66, 50, FLAME)
            + S.fuse([S.ell(64, 86, 42, 18, S.shade(FLAME, 0.35))])
            + [S.ell(64, 86, 26, 10, S.shade(FLAME, 0.55), s=False)])


def _job_engineer():
    """A clipboard. 'Turns up with a clipboard and three options.'"""
    return (S.fuse([S.rect(64, 70, 72, 92, PAPER_W, r=6)])
            + [S.rect(64, 26, 34, 16, METAL_DARK, r=4)]
            + [S.rect(56, 52, 44, 8, METAL_DARK, r=3),
               S.rect(56, 72, 44, 8, METAL_DARK, r=3),
               S.rect(56, 92, 44, 8, METAL_DARK, r=3)])


def _job_magician():
    """A wand and sparks. 'Whatever is up that sleeve burns everything nearby.'"""
    return (S.fuse([S.rect(58, 76, 16, 76, PAL.STARDUST, r=5)])
            + [S.rect(58, 104, 16, 18, PAPER_W, r=3)]
            + _burst(74, 32, 26, PAL.GOLD, 6)
            + [S.ell(30, 60, 5, 5, PAL.GOLD, s=False),
               S.ell(100, 74, 4, 4, PAL.GOLD, s=False)])


def _job_town_crier():
    """A bell. 'Bellows across the whole yard.'"""
    return (S.fuse([S.ell(64, 62, 38, 40, PAL.GOLD), S.rect(64, 84, 82, 18, PAL.GOLD, r=6),
                    S.rect(64, 24, 14, 14, PAL.GOLD, r=4)])
            + [S.ell(64, 100, 11, 11, S.shade(PAL.GOLD, 0.35))]
            + [S.line([(26, 44), (12, 32)], S.INK, 3.0),
               S.line([(102, 44), (116, 32)], S.INK, 3.0)])


def _job_unemployed():
    """A cushion. The cat reviewed the positions and declined all of them."""
    return (S.fuse([S.rect(64, 74, 88, 52, CLOTH, r=18)])
            + [S.ell(64, 74, 30, 18, CLOTH_DARK, s=False)]
            + [S.ell(24, 52, 7, 7, CLOTH_DARK, s=False),
               S.ell(104, 52, 7, 7, CLOTH_DARK, s=False),
               S.ell(24, 96, 7, 7, CLOTH_DARK, s=False),
               S.ell(104, 96, 7, 7, CLOTH_DARK, s=False)])


def _jobs():
    return [
        (art("hat_blacksmith.svg"), 64, _hat_blacksmith),
        (art("hat_daredevil.svg"), 64, _hat_daredevil),
        (art("hat_engineer.svg"), 64, _hat_engineer),
        (art("hat_magician.svg"), 64, _hat_magician),
        (art("hat_town_crier.svg"), 64, _hat_town_crier),
        (art("hat_unemployed.svg"), 64, _hat_unemployed),
        (art("job_blacksmith.svg"), 64, _job_blacksmith),
        (art("job_daredevil.svg"), 64, _job_daredevil),
        (art("job_engineer.svg"), 64, _job_engineer),
        (art("job_magician.svg"), 64, _job_magician),
        (art("job_town_crier.svg"), 64, _job_town_crier),
        (art("job_unemployed.svg"), 64, _job_unemployed),
    ]


# -------------------------------------------------------- nine lives --------
# The layer is gone (2026-09-14); three of its rule icons are skill icons now.

def _rule_curiosity():
    """A fan of cards. 'One more card in every hand, for good.'"""
    out = []
    for dx, dy, c in ((-26, 8, S.shade(PAPER_W, 0.18)), (0, 0, PAPER_W),
                      (26, 8, S.shade(PAPER_W, 0.18))):
        out += S.fuse([S.rect(64 + dx, 70 + dy, 44, 62, c, r=6)])
    out.append(S.ell(64, 66, 13, 13, PAL.STARDUST, s=False))
    return out


def _rule_feral_instinct():
    """A claw mark. 'The kittens inherit a tenth of your crit chance.'"""
    out = []
    for dx in (-24, 0, 24):
        out += S.fuse([S.tri([(64 + dx - 10, 30), (64 + dx + 9, 34),
                              (64 + dx + 1, 100)], WARM)])
    return out


def _rule_muscle_memory():
    """A STAIRCASE. 'Every trunk starts at its first rung after a Reawaken.'

    The first version stacked three centred bars of decreasing width, which is
    not a staircase - it is a wedding cake. Steps have to step SIDEWAYS as well
    as up, so each tread is offset."""
    out = []
    for x, y in ((36, 98), (64, 74), (92, 50)):
        out += S.fuse([S.rect(x, y, 40, 24, METAL, r=4)])
    for x, y in ((36, 98), (64, 74), (92, 50)):
        out.append(S.rect(x, y - 8, 40, 7, METAL_DARK, r=3))
    return out


def _rule_second_helping():
    """Two coins. 'Reawakening pays out twice.'"""
    return (S.fuse([S.ell(48, 74, 32, 32, PAL.GOLD)])
            + [S.ell(48, 74, 18, 18, S.shade(PAL.GOLD, 0.3), s=False)]
            + S.fuse([S.ell(86, 56, 26, 26, PAL.GOLD)])
            + [S.ell(86, 56, 14, 14, S.shade(PAL.GOLD, 0.3), s=False)])


def _rule_long_nap():
    """THREE Zs, not a crescent moon.

    The crescent was cut by drawing a disc in tier_8's FIELD COLOUR over a
    circle - the third time in this job that a shape has been "subtracted" with
    a background colour, and the third time it would have shipped a coloured
    blob instead of a hole. On a UI panel that disc is not transparent, it is
    lilac. There is no subtraction in this grammar; if a shape needs a bite
    taken out of it, draw a different shape. check_no_background_fills() now
    refuses to write any asset that tries it.

    ORPHANED: this rule went with offline accrual on 2026-09-10 and nothing
    loads this file. Regenerated so the set stays whole.
    """
    out = []
    for x, y, w in ((44, 96, 34), (68, 64, 30), (90, 34, 24)):
        out += S.fuse([S.rect(x, y - w * 0.30, w, 9, PAL.STARDUST, r=2),
                       S.rect(x, y + w * 0.30, w, 9, PAL.STARDUST, r=2)])
        out.append(S.line([(x + w * 0.40, y - w * 0.26), (x - w * 0.40, y + w * 0.26)],
                          PAL.STARDUST, 8.0))
    return out


def _rules():
    """The three Nine Lives rules that became skill nodes on 2026-09-14, drawn
    at skill size. Feral Instinct took over the whisker_trade node (and its
    icon file name - the id is what saves key on), so its icon lives under
    that name."""
    d = ART + "/skills/"
    return [
        (d + "skill_curiosity.svg", 64, _rule_curiosity),
        (d + "skill_whisker_trade.svg", 64, _rule_feral_instinct),
        (d + "skill_second_helping.svg", 64, _rule_second_helping),
    ]



# ------------------------------------------------- the icon vocabulary ------
# Twenty-eight cards and twenty-six skills is fifty-four drawings, and drawing
# fifty-four unrelated things is how an icon set stops looking like a set. So
# there is a VOCABULARY of about a dozen marks, and each icon is a composition
# of two or three of them. A player learns the vocabulary once - claw means
# damage, ring means aura, coin means catnip, flame means burn - and then reads
# every new card faster than they can read its text.
#
# _ring() is worth reading as the pattern for everything in here: a ring is
# BEADS FUSED INTO A TORUS, never a disc with a smaller disc painted over it.
# See check_no_background_fills().

AURA_C = PAL.MATERIALS["aura"]
AURA_D = PAL.MATERIALS["aura_dark"]
CLAW_C = PAL.MATERIALS["claw"]
BOMB_C = PAL.MATERIALS["bomb"]
RAIN = PAL.MATERIALS["rainbow"]


def _ring(cx, cy, r, colour, beads=16, w=9.0, squash=1.0):
    import math
    out = []
    for i in range(beads):
        a = i * (2.0 * math.pi / beads)
        out.append(S.ell(cx + math.cos(a) * r, cy + math.sin(a) * r * squash,
                         w * 0.62, w * 0.62 * squash, colour))
    return S.fuse(out)


def _claws(colour=CLAW_C, n=3, cx=64, top=26, bot=104, spread=24):
    out = []
    for i in range(n):
        dx = (i - (n - 1) / 2.0) * spread
        out += S.fuse([S.tri([(cx + dx - 9, top), (cx + dx + 8, top + 5),
                              (cx + dx + 1, bot)], colour)])
    return out


def _flame(cx, cy, h, colour=FLAME):
    return S.fuse([S.ell(cx, cy, h * 0.42, h * 0.40, colour),
                   S.tri([(cx - h * 0.36, cy), (cx + h * 0.06, cy - h * 1.05),
                          (cx + h * 0.36, cy - h * 0.1)], colour)])


def _coin(cx, cy, r, colour=None):
    c = colour or PAL.GOLD
    return S.fuse([S.ell(cx, cy, r, r, c)]) + [
        S.ell(cx, cy, r * 0.54, r * 0.54, S.shade(c, 0.3), s=False)]


def _paw(cx, cy, r, colour=CLAW_C):
    out = S.fuse([S.ell(cx, cy + r * 0.42, r * 0.92, r * 0.74, colour)])
    for dx, dy, s_ in ((-1.02, -0.72, 0.34), (-0.38, -1.02, 0.36),
                       (0.38, -1.02, 0.36), (1.02, -0.72, 0.34)):
        out += S.fuse([S.ell(cx + dx * r, cy + dy * r, r * s_, r * s_, colour)])
    return out


def _bolt(cx, cy, h, colour=PAL.GOLD):
    return S.fuse([S.tri([(cx - h * 0.34, cy), (cx + h * 0.16, cy - h * 0.98),
                          (cx + h * 0.04, cy - h * 0.06)], colour),
                   S.tri([(cx + h * 0.34, cy - h * 0.06),
                          (cx - h * 0.14, cy + h * 0.98),
                          (cx - h * 0.02, cy + h * 0.04)], colour)])


def _rainbow(cx, cy, r, w=7.0):
    out = []
    for i, c in enumerate(RAIN):
        out.append(S.curve(cx - r + i * w * 0.72, cy,
                           cx, cy - r * 1.5 + i * w * 0.9,
                           cx + r - i * w * 0.72, cy, w=w, colour=c))
    return out


def _cardshape(cx, cy, colour=PAPER_W, w=44, h=62, r=6):
    return S.fuse([S.rect(cx, cy, w, h, colour, r=r)])


def _drop(cx, cy, r, colour=AURA_C):
    return S.fuse([S.ell(cx, cy, r, r * 1.16, colour),
                   S.tri([(cx - r * 0.7, cy - r * 0.4), (cx, cy - r * 2.0),
                          (cx + r * 0.7, cy - r * 0.4)], colour)])


def _bomb(cx, cy, r, colour=BOMB_C):
    return (S.fuse([S.ell(cx, cy + r * 0.2, r, r, colour),
                    S.rect(cx + r * 0.5, cy - r * 0.82, r * 0.5, r * 0.44, colour, r=2)])
            + [S.curve(cx + r * 0.7, cy - r * 1.0, cx + r * 1.5, cy - r * 1.5,
                       cx + r * 1.1, cy - r * 2.0, w=4.0, colour=S.INK)]
            + _burst(cx + r * 1.1, cy - r * 2.1, r * 0.5, PAL.GOLD, 5))


def _arrow(cx, cy, r, colour=AURA_D, down=False):
    k = -1.0 if down else 1.0
    return S.fuse([S.rect(cx, cy, r * 0.5, r * 1.5, colour, r=2),
                   S.tri([(cx - r * 0.8, cy - k * r * 0.5), (cx, cy - k * r * 1.4),
                          (cx + r * 0.8, cy - k * r * 0.5)], colour)])


# ----------------------------------------------------------- the cards ------

def _cards():
    d = ART + "/cards/"
    C = {}

    # --- pounce -------------------------------------------------------------
    C["chain_pounce"] = lambda: (_paw(38, 82, 20) + _paw(66, 60, 20) + _paw(96, 40, 20))
    C["peer_pressure"] = lambda: (_paw(64, 44, 22)
                                  + _paw(30, 96, 14) + _paw(64, 104, 14) + _paw(98, 96, 14))
    C["scorched_landing"] = lambda: (S.fuse([S.ell(64, 100, 46, 18, S.shade(FLAME, 0.4))])
                                     + _flame(34, 96, 32) + _flame(64, 100, 38)
                                     + _flame(94, 96, 32) + _paw(64, 42, 22))
    C["feral_focus"] = lambda: (_ring(64, 100, 20, AURA_D, beads=10, w=7, squash=0.42)
                                + _flame(64, 86, 44) + _paw(64, 40, 22))
    C["bloodlust"] = lambda: (_paw(46, 74, 26)
                              + _arrow(98, 70, 24, AURA_D, down=True))

    # --- aura ---------------------------------------------------------------
    C["close_quarters"] = lambda: (_ring(64, 68, 26, AURA_C, w=14)
                                   + _burst(64, 68, 20, AURA_D, 8))
    C["ember_trail"] = lambda: (_ring(64, 62, 34, AURA_C, w=9)
                                + _flame(30, 108, 34) + _flame(64, 112, 28)
                                + _flame(98, 108, 34))
    C["hot_borders"] = lambda: (_ring(64, 68, 44, FLAME, w=14)
                                + _ring(64, 68, 20, AURA_C, w=8))
    C["slow_burn"] = lambda: (_ring(64, 68, 44, FLAME, w=13)
                              + _flame(64, 84, 40))
    C["thunderclap"] = lambda: (_ring(64, 70, 42, AURA_C, w=10) + _bolt(64, 70, 46))
    C["rigor"] = lambda: (_ring(64, 70, 42, AURA_C, w=10)
                          + _arrow(64, 74, 26, AURA_D, down=True))

    # --- economy ------------------------------------------------------------
    C["bounty"] = lambda: (_coin(64, 74, 34) + _burst(64, 74, 20, PAL.STARDUST, 6))
    C["compound_interest"] = lambda: (_coin(42, 92, 24) + _coin(66, 72, 24)
                                      + _coin(90, 52, 24))
    C["gold_rush"] = lambda: (_coin(40, 84, 26) + _coin(80, 84, 26)
                              + _coin(60, 46, 26))
    C["hoarder"] = lambda: (S.fuse([S.rect(64, 88, 84, 48, WOOD, r=8)])
                            + [S.rect(64, 70, 88, 12, WOOD_DARK, r=4)]
                            + _coin(64, 40, 22))
    C["scavenger"] = lambda: (_coin(64, 80, 28)
                              + _arrow(64, 36, 22, LEAF_G)
                              + [S.ell(24, 96, 8, 8, PAL.GOLD, s=False),
                                 S.ell(104, 96, 8, 8, PAL.GOLD, s=False)])
    C["notoriety"] = lambda: (_cardshape(64, 72, colour=PAPER_W, w=56, h=72)
                              + S.fuse([S.tri([(44, 58), (38, 24), (58, 52)], CLAW_C),
                                        S.tri([(56, 54), (56, 18), (70, 50)], CLAW_C),
                                        S.tri([(68, 54), (82, 22), (84, 56)], CLAW_C)])
                              + [S.rect(64, 84, 34, 9, PAL.STARDUST, r=3),
                                 S.rect(64, 100, 34, 9, PAL.STARDUST, r=3)])

    # --- kittens ------------------------------------------------------------
    C["giblets"] = lambda: (_burst(64, 70, 46, CLAW_C, 10)
                            + S.fuse([S.ell(64, 70, 20, 18, S.shade(CLAW_C, 0.3))]))
    C["gilded_kitten"] = lambda: (xform_parts(S.creature(S.CAST()["kitten"]), 0, 0, 0.9)
                                  + _burst(96, 34, 20, PAL.GOLD, 6))
    C["lone_hunter"] = lambda: (xform_parts(S.creature(S.CAST()["kitten"]), -8, 0, 0.82)
                                + _coin(104, 34, 18))

    # --- the deck -----------------------------------------------------------
    C["dilated"] = lambda: (_cardshape(30, 76) + _cardshape(64, 68)
                            + _cardshape(98, 76) + _burst(64, 26, 18, PAL.STARDUST, 5))
    C["fixation"] = lambda: (_cardshape(50, 72) + _cardshape(80, 72)
                             + [S.ell(65, 72, 14, 14, PAL.STARDUST, s=False)])
    C["not_that_one"] = lambda: (_cardshape(64, 72)
                                 + [S.line([(30, 36), (98, 108)], CLAW_C, 11.0),
                                    S.line([(98, 36), (30, 108)], CLAW_C, 11.0)])
    C["spite"] = lambda: (_cardshape(64, 74, w=38, h=52)
                          + _ring(64, 74, 48, PAL.STARDUST, beads=15, w=9)
                          + _arrow(102, 46, 18, PAL.STARDUST))

    # --- bombs and rainbows -------------------------------------------------
    C["catnip_bomb"] = lambda: _bomb(58, 82, 30)
    C["stampede"] = lambda: (_bomb(48, 84, 24)
                             + [S.line([(84, 46), (118, 46)], AURA_D, 7.0),
                                S.line([(84, 66), (118, 66)], AURA_D, 7.0),
                                S.line([(84, 86), (118, 86)], AURA_D, 7.0)])
    C["rainbow_trail"] = lambda: _rainbow(64, 96, 52)
    C["lingering_rainbow"] = lambda: (_rainbow(64, 88, 48)
                                      + [S.ell(64, 108, 40, 12, RAIN[4], s=True)])

    return [(d + "card_%s.svg" % k, 256, v) for k, v in sorted(C.items())]


def xform_parts(parts, dx, dy, scale):
    """EVO.xform under a local name, so the icon code does not have to reach
    into the evolutions module for the one thing it borrows."""
    return EVO.xform(parts, dx, dy, scale)



# ---------------------------------------------------------- the skills -----
# SIXTY-FOUR PIXELS, which is half what the cards get, so these are the most
# constrained drawings in the game: ONE mark, or two if the second is large.
# Anything finer than about eight drawing units disappears.
#
# TWO CONVENTIONS DO MOST OF THE WORK.
#   * A skill and its upgrade share a mark. Aura, Aura Power and Aura Reach are
#     all the ring - a player should see at a glance that three nodes belong to
#     one trunk, because that is literally how the tree is laid out.
#   * An "unlock_" node is its own mark PLUS A PADLOCK. Unlocks are a different
#     kind of thing from levels and they should not have to be read to be told
#     apart.


def _padlock(cx=96, cy=96, r=20, colour=METAL):
    # THE SHACKLE IS AN ARCH, not a filled ellipse. Drawn solid it read as a head
    # on a body and the padlock became a tiny grey person. An arch needs a hole,
    # and there is no subtraction here, so it is a half-ring of fused beads -
    # the same trick as the halo.
    import math
    arch = []
    for i in range(9):
        a = math.pi + i * (math.pi / 8.0)
        arch.append(S.ell(cx + math.cos(a) * r * 0.62,
                          cy - r * 0.34 + math.sin(a) * r * 0.62,
                          r * 0.17, r * 0.17, colour))
    return (S.fuse(arch)
            + S.fuse([S.rect(cx, cy + r * 0.30, r * 1.6, r * 1.25, colour, r=4)])
            + [S.ell(cx, cy + r * 0.22, r * 0.24, r * 0.24, S.shade(colour, 0.45), s=False),
               S.rect(cx, cy + r * 0.60, r * 0.18, r * 0.42, S.shade(colour, 0.45), r=1)])


def _horn(cx=64, cy=96, h=76, colour=None):
    c = colour or PAL.GOLD
    out = S.fuse([S.tri([(cx - h * 0.22, cy), (cx, cy - h), (cx + h * 0.22, cy)], c)])
    for i in range(3):
        y = cy - h * (0.24 + i * 0.22)
        w = h * 0.18 * (1.0 - i * 0.25)
        out.append(S.line([(cx - w, y), (cx + w, y - h * 0.05)], S.shade(c, 0.35), 3.4))
    return out


def _heart(cx, cy, r, colour=CLAW_C):
    return S.fuse([S.ell(cx - r * 0.48, cy - r * 0.34, r * 0.6, r * 0.58, colour),
                   S.ell(cx + r * 0.48, cy - r * 0.34, r * 0.6, r * 0.58, colour),
                   S.tri([(cx - r * 1.0, cy - r * 0.1), (cx, cy + r * 0.98),
                          (cx + r * 1.0, cy - r * 0.1)], colour)])


def _prism(cx=58, cy=76, h=62, colour=AURA_C):
    out = S.fuse([S.tri([(cx - h * 0.52, cy + h * 0.42), (cx, cy - h * 0.56),
                         (cx + h * 0.52, cy + h * 0.42)], colour)])
    for i, c in enumerate(RAIN[:5]):
        out.append(S.line([(cx + h * 0.34, cy + h * 0.04),
                           (126, 40 + i * 15)], c, 6.0))
    return out


def _eye(cx=64, cy=70, r=40, colour=AURA_C):
    return (S.fuse([S.ell(cx, cy, r, r * 0.66, colour)])
            + [S.ell(cx, cy, r * 0.46, r * 0.46, PAL.EYE),
               S.ell(cx, cy, r * 0.22, r * 0.22, S.INK, s=False)])


def _net(colour=AURA_D):
    out = []
    for i in range(4):
        t = 22 + i * 28
        out.append(S.line([(t, 26), (t - 14, 108)], colour, 5.0))
        out.append(S.line([(18, t), (110, t - 10)], colour, 5.0))
    return out


def _weight(colour=METAL_DARK):
    return (S.fuse([S.rect(64, 84, 88, 54, colour, r=8),
                    S.rect(64, 44, 30, 20, colour, r=6)])
            + [S.ell(64, 88, 26, 18, S.shade(colour, 0.3), s=False)])


def _funnel(colour=AURA_C):
    return (S.fuse([S.tri([(16, 30), (112, 30), (64, 92)], colour),
                    S.rect(64, 104, 20, 34, colour, r=4)])
            + [S.rect(64, 36, 92, 12, S.shade(colour, 0.3), r=4)])


def _bars():
    out = []
    for i, c in enumerate(RAIN[:5]):
        out += S.fuse([S.rect(64, 34 + i * 17, 84, 13, c, r=4)])
    return out


def _skills():
    d = ART + "/skills/"
    K = {}
    K["aura"] = lambda: _ring(64, 70, 44, AURA_C, w=15)
    K["aura_damage"] = lambda: _ring(64, 70, 44, AURA_C, w=14) + _claws(CLAW_C, 1, 64, 38, 100, 0)
    K["aura_reach"] = lambda: (_ring(64, 70, 30, AURA_C, w=12)
                               + _arrow(64, 22, 18, AURA_D)
                               + _arrow(64, 118, 18, AURA_D, down=True))
    K["claws"] = lambda: _claws(CLAW_C, 3)
    K["critical_mass"] = lambda: _burst(64, 70, 52, PAL.GOLD, 10)
    K["deadweight"] = lambda: _weight()
    K["fortune"] = lambda: _coin(64, 72, 42)
    K["funnel"] = lambda: _funnel()
    K["instinct"] = lambda: _eye()
    K["legacy"] = lambda: (xform_parts(S.creature(S.CAST()["kitten"]), 0, -4, 0.86)
                           + _burst(104, 30, 18, PAL.GOLD, 5))
    K["overkill"] = lambda: _burst(64, 70, 50, CLAW_C, 9)
    K["overkill_splash"] = lambda: (_burst(56, 64, 40, CLAW_C, 9)
                                    + _drop(104, 96, 15, CLAW_C)
                                    + _drop(24, 104, 12, CLAW_C))
    K["play_with_food"] = lambda: (_paw(44, 62, 30)
                                   + xform_parts(S.creature(S.CAST()["mouse"]), 46, 44, 0.46))
    K["pounce"] = lambda: _paw(64, 70, 44)
    K["pounce_power"] = lambda: _paw(60, 80, 36) + _arrow(64, 26, 20, PAL.GOLD)
    K["prism"] = lambda: _prism()
    K["reflex"] = lambda: _bolt(64, 66, 84)
    K["refraction"] = lambda: (_prism(54, 80, 54)
                               + [S.line([(6, 46), (40, 70)], PAL.EYE, 6.0)])
    K["snare"] = lambda: _net()
    K["spectrum"] = lambda: _bars()
    K["unicorn"] = lambda: _horn()
    # A soft-serve swirl in three rainbow bands, with a spiteful little
    # curl on top: the Unicorn's limb split in two (designer, 2026-10-01),
    # and the second node needed its own picture.
    K["rainbow_poop"] = lambda: (S.fuse([S.ell(64, 98, 46, 18, RAIN[0])]) + [S.ell(64, 98, 46, 18, RAIN[0])]
                                 + S.fuse([S.ell(64, 76, 36, 16, RAIN[2])]) + [S.ell(64, 76, 36, 16, RAIN[2])]
                                 + S.fuse([S.ell(64, 56, 25, 14, RAIN[4])]) + [S.ell(64, 56, 25, 14, RAIN[4])]
                                 + S.fuse([S.tri([(56, 48), (74, 46), (70, 24)], RAIN[5])])
                                 + [S.tri([(56, 48), (74, 46), (70, 24)], RAIN[5])])
    K["unicorn_power"] = lambda: _horn(64, 100, 70) + _burst(64, 22, 22, PAL.STARDUST, 6)
    K["vitality"] = lambda: _heart(64, 66, 46)
    K["full_spectrum"] = lambda: _ring(64, 68, 42, RAIN[4], w=14) + _bars()[:6]
    K["unlock_full_spectrum"] = lambda: _bars() + _padlock()
    K["unlock_prism"] = lambda: _prism(50, 66, 54) + _padlock()
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]




# ------------------------------------------------- weapons and whiskers -----
# WHAT WAS ACTUALLY MISSING, found by walking every ArtStyle.texture_for() stem
# during the 2026-09-11 restructure. The famine set had these and the sweet set
# did not, so removing styles would have removed them from the game:
#
#   player_weapon_stage_2..7   the overlay drawn in the cat's paw
#
# STAGE 1 IS DELIBERATELY ABSENT and must stay that way. The art backlog records
# it as dropped on purpose: stage 1 is the bare paw, the ABSENCE of a weapon
# rather than a picture of one, and get_weapon_texture() returning null for it
# is the correct behaviour, not a gap.
#
# A weapon is drawn as a DIAGONAL held in a paw, scaled against the cat's width,
# so each one reads as a silhouette at a fraction of the cat's size. The theme
# follows the stage: armour, then circuitry, then the mount's cavalry lance, and
# gold at the end.


def _haft(x1, y1, x2, y2, colour, w=11.0):
    return S.fuse([S.ell((x1 + x2) / 2, (y1 + y2) / 2, 0.1, 0.1, colour)])[:0] + [
        S.line([(x1, y1), (x2, y2)], S.INK, w + 5.0),
        S.line([(x1, y1), (x2, y2)], colour, w)]


def _weapon_2():
    """Armoured Tomcat: a mace. Blunt, heavy, all shoulder."""
    return (_haft(34, 108, 76, 44, WOOD)
            + S.fuse([S.ell(84, 34, 24, 24, METAL)])
            + [S.ell(84, 34, 11, 11, METAL_DARK, s=False)]
            + [S.tri([(84, 2), (92, 24), (76, 24)], METAL, s=True),
               S.tri([(114, 34), (94, 42), (94, 26)], METAL, s=True),
               S.tri([(54, 34), (74, 26), (74, 42)], METAL, s=True)])


def _weapon_3():
    """Cyber Cat: a blaster. The only weapon with a straight barrel."""
    return (S.fuse([S.rect(70, 62, 76, 26, METAL, r=6),
                    S.rect(44, 92, 24, 40, METAL_DARK, r=5)])
            + [S.rect(84, 62, 40, 10, "#7FD4E8", r=3),
               S.ell(110, 62, 12, 15, "#7FD4E8", s=True),
               S.ell(110, 62, 5, 7, PAL.EYE, s=False)])


def _weapon_4():
    """Rainbow Cavalry: a LANCE, because the stage is cavalry and a lance is the
    one weapon that says mounted before it says anything else."""
    return (_haft(20, 112, 84, 36, WOOD, 9.0)
            + S.fuse([S.tri([(78, 44), (122, 8), (96, 54)], METAL)])
            + [S.ell(74, 48, 15, 11, PAL.GOLD, s=True)]
            + [S.ell(74, 48, 7, 5, S.shade(PAL.GOLD, 0.3), s=False)])


def _weapon_5():
    """Alicorn: a feathered spear - the wing motif carried into the weapon."""
    # The feathers sit ALONG the haft as a fletching, not as two clouds beside
    # it. Tapered, overlapping, and rooted on the shaft.
    return (_haft(24, 110, 80, 40, PAPER_W, 9.0)
            + S.fuse([S.tri([(72, 48), (104, 6), (92, 52)], "#CFE4F5")])
            + S.fuse([S.tri([(46, 88), (28, 72), (56, 74)], "#CFE4F5"),
                      S.tri([(38, 98), (18, 84), (48, 86)], "#CFE4F5")]))


def _weapon_6():
    """Void Unicorn: a scythe. The only curved blade in the set."""
    # A SWEEP, not a paddle. The blade was one fat ellipse and read as a
    # lollipop; a scythe is a long thin curve that TAPERS to a point, so it is
    # built from three narrowing triangles fanned off the haft's top.
    return (_haft(30, 116, 66, 40, "#4A4266", 10.0)
            + S.fuse([S.tri([(62, 46), (118, 30), (74, 22)], "#B9A6E8"),
                      S.tri([(74, 24), (120, 30), (104, 12)], "#B9A6E8"),
                      S.tri([(60, 40), (86, 18), (68, 16)], "#B9A6E8")])
            + [S.line([(66, 34), (112, 28)], "#D8CBF2", 3.0)])


def _weapon_7():
    """The Last Unicorn: a gold greatsword, the widest blade in the game."""
    return (S.fuse([S.rect(66, 62, 30, 104, PAL.GOLD, r=6),
                    S.tri([(48, 20), (66, 0), (84, 20)], PAL.GOLD)])
            + S.fuse([S.rect(66, 104, 72, 16, S.shade(PAL.GOLD, 0.3), r=5)])
            + [S.rect(66, 122, 20, 20, WOOD, r=4),
               S.ell(66, 62, 9, 34, S.tint(PAL.GOLD, 0.5), s=False)])


def _icon_whiskers():
    """The Nine Lives currency. Three whiskers off a muzzle dot - it existed only
    as a famine PNG, so the sweet set would have lost it with the styles."""
    parts = S.fuse([S.ell(64, 78, 13, 10, PAL.CAT_NOSE)])
    for side in (-1, 1):
        for i, dy in enumerate((-16.0, 0.0, 14.0)):
            parts.append(S.curve(64 + side * 12, 74 + dy * 0.3,
                                 64 + side * 36, 66 + dy,
                                 64 + side * 56, 60 + dy * 1.5,
                                 w=5.0, colour=S.INK))
    parts.append(S.ell(60, 74, 4, 3, PAL.EYE, s=False))
    return parts


def _missing():
    return [
        (art("player_weapon_stage_2.svg"), 128, _weapon_2),
        (art("player_weapon_stage_3.svg"), 128, _weapon_3),
        (art("player_weapon_stage_4.svg"), 128, _weapon_4),
        (art("player_weapon_stage_5.svg"), 128, _weapon_5),
        (art("player_weapon_stage_6.svg"), 128, _weapon_6),
        (art("player_weapon_stage_7.svg"), 128, _weapon_7),
    ]



# ------------------------------------------ the 2026-09-12 jobs ------------
# Four jobs added with the skill tree expansion. Each needs an ICON for the
# roster and a HAT for the cat to wear, and the two are drawn differently for
# the reason the first six were: a hat is worn in the world so it is the hat
# alone, an icon is read in a list so it is the job's TOOL.


def _hat_cat_burglar():
    """A flat cap with the brim pulled down. The only hat in the set that hides
    part of the face, which is the whole idea."""
    return (S.fuse([S.ell(64, 58, 40, 26, "#4A4E5C"),
                    S.rect(64, 78, 88, 16, "#3A3E4A", r=6)])
            + [S.rect(64, 78, 88, 7, "#2E313B", r=3, s=False),
               S.ell(48, 52, 12, 8, S.tint("#4A4E5C", 0.28), s=False)])


def _hat_sphinx():
    """A nemes headdress: the striped cloth that falls either side of the face.
    Read as Egyptian instantly, and nothing else in the set has side panels."""
    return (S.fuse([S.ell(64, 50, 38, 30, PAL.GOLD),
                    S.tri([(26, 46), (18, 106), (44, 96)], PAL.GOLD),
                    S.tri([(102, 46), (110, 106), (84, 96)], PAL.GOLD)])
            + [S.rect(64, 30, 66, 9, "#3F72C4", r=2, s=False),
               S.rect(64, 44, 62, 9, "#3F72C4", r=2, s=False),
               S.line([(30, 64), (36, 98)], "#3F72C4", 5.0),
               S.line([(98, 64), (92, 98)], "#3F72C4", 5.0)])


def _hat_tax_collector():
    """A green eyeshade. Small, unmistakable, and the only translucent-looking
    thing in the set."""
    return (S.fuse([S.rect(64, 52, 74, 18, "#7A6A4E", r=5),
                    S.ell(64, 74, 44, 22, "#4E9E6A")])
            + [S.ell(64, 74, 34, 15, S.tint("#4E9E6A", 0.35), s=False),
               S.rect(64, 46, 78, 7, "#5E5238", r=3, s=False)])


def _hat_ratcatcher():
    """A wide slouch hat with a feather - the Pied Piper silhouette, which is
    what the job is."""
    return (S.fuse([S.ell(64, 96, 56, 14, "#6E4A3A"),
                    S.ell(64, 58, 30, 26, "#6E4A3A")])
            + [S.rect(64, 76, 58, 9, "#4E3226", r=3, s=False)]
            + S.fuse([S.ell(98, 46, 7, 16, "#D6564E"),
                      S.ell(106, 32, 5, 11, "#D6564E")]))


def _job_cat_burglar():
    """A swag bag. The tool, not the thief."""
    return (S.fuse([S.ell(64, 80, 34, 30, "#4A4E5C"),
                    S.ell(64, 50, 15, 13, "#4A4E5C"),
                    S.tri([(50, 46), (64, 22), (78, 46)], "#4A4E5C")])
            + [S.line([(46, 58), (82, 58)], "#2E313B", 5.0),
               S.ell(64, 84, 15, 13, PAL.GOLD, s=True),
               S.ell(64, 84, 7, 7, S.shade(PAL.GOLD, 0.3), s=False)])


def _job_sphinx():
    """A riddle: a question mark cut from stone."""
    parts = S.fuse([S.rect(64, 96, 30, 26, "#C9B98F", r=4)])
    ring = []
    for i in range(11):
        a = math.pi * 1.15 - i * (math.pi * 1.35 / 10.0)
        ring.append(S.ell(64 + math.cos(a) * 26, 52 - math.sin(a) * 26, 8, 8, PAL.GOLD))
    parts = S.fuse(ring) + parts
    parts += S.fuse([S.ell(64, 78, 8, 8, PAL.GOLD)])
    parts.append(S.line([(52, 96), (76, 96)], S.shade("#C9B98F", 0.3), 2.4))
    return parts


def _job_tax_collector():
    """A ledger with a coin on it. Two marks, and the pairing is the joke."""
    return (S.fuse([S.rect(60, 72, 68, 82, PAPER_W, r=5)])
            + [S.rect(60, 42, 68, 12, "#7A6A4E", r=3),
               S.rect(56, 62, 44, 6, METAL_DARK, r=2, s=False),
               S.rect(56, 78, 44, 6, METAL_DARK, r=2, s=False),
               S.rect(56, 94, 30, 6, METAL_DARK, r=2, s=False)]
            + _coin(96, 96, 22))


def _job_ratcatcher():
    """A pipe, and three notes coming off it."""
    return (S.fuse([S.rect(54, 76, 16, 84, "#8A6A4A", r=6)])
            + [S.ell(54, 38, 11, 9, "#6E5238", s=True),
               S.ell(54, 62, 4, 4, "#5A4230", s=False),
               S.ell(54, 82, 4, 4, "#5A4230", s=False),
               S.ell(54, 102, 4, 4, "#5A4230", s=False)]
            + S.fuse([S.ell(92, 44, 8, 7, LEAF_G), S.rect(97, 34, 4, 20, LEAF_G, r=1)])
            + S.fuse([S.ell(104, 74, 7, 6, AURA_C), S.rect(108, 65, 4, 18, AURA_C, r=1)])
            + S.fuse([S.ell(88, 100, 6, 5, PAL.STARDUST),
                      S.rect(92, 92, 3, 16, PAL.STARDUST, r=1)]))


def _new_jobs():
    return [
        (art("hat_cat_burglar.svg"), 64, _hat_cat_burglar),
        (art("hat_sphinx.svg"), 64, _hat_sphinx),
        (art("hat_tax_collector.svg"), 64, _hat_tax_collector),
        (art("hat_ratcatcher.svg"), 64, _hat_ratcatcher),
        (art("job_cat_burglar.svg"), 64, _job_cat_burglar),
        (art("job_sphinx.svg"), 64, _job_sphinx),
        (art("job_tax_collector.svg"), 64, _job_tax_collector),
        (art("job_ratcatcher.svg"), 64, _job_ratcatcher),
    ]



# ------------------------------------------- job sub-tree ladders ----------
# Five nodes, one under each of five jobs. Each icon has to read at 56px inside
# a 96px square, and has to be tellable from its PARENT's icon at a glance -
# these sit directly below the job they belong to, so "another coin" or "another
# claw" would be a node the player cannot find twice.


def _skill_dent():
    """A plate with three gouges taken out of it. The gouges are drawn as dark
    STROKES on top, never as background-coloured shapes - a cut-out faked with
    the page colour stops being a cut-out the moment the icon sits on anything
    else, which this project has shipped by accident three times."""
    plate = S.fuse([S.rect(62, 74, 80, 72, METAL, r=10)])
    out = plate + [S.rect(62, 74, 62, 54, S.tint(METAL, 0.18), r=7, s=False)]
    # Two gouges, deliberately unequal. Even spacing is what made the first
    # version read as a grille.
    out.append(S.line([(38, 48), (74, 96)], S.shade(METAL, 0.5), 9.0))
    out.append(S.line([(70, 54), (88, 82)], S.shade(METAL, 0.5), 6.0))
    # A broken corner, drawn INSIDE the plate's own outline. Outside it, the
    # same triangle read as a pennant attached to the plate rather than a piece
    # taken off it.
    out.append(S.tri([(84, 38), (102, 38), (102, 58)], S.shade(METAL, 0.62), s=False))
    return out


def _skill_overtime():
    """A clock, hands past the hour. The only round face in the skill set, so
    it does not collide with the Engineer's clipboard above it."""
    return (_ring(64, 68, 40, "#6FA8C8", beads=18, w=11)
            + S.fuse([S.ell(64, 68, 27, 27, "#DCE8F0")])
            + [S.ell(64, 68, 5, 5, "#2E4A5C", s=False),
               S.line([(64, 68), (64, 46)], "#2E4A5C", 6.0),
               S.line([(64, 68), (84, 78)], "#2E4A5C", 5.0)])


def _skill_severance():
    """A coin handed upward. Unemployed's own node is a coin too, so this one
    is a coin PLUS the arrow - the pairing is the point."""
    return _coin(58, 84, 34) + _arrow(100, 46, 22, LEAF_G)


def _skill_traps():
    """Jaws closing over a net. The net says Snare, the jaws say this is what
    happens to whatever the Snare caught."""
    out = []
    # The net sits BEHIND and thin, so it says "snare" without competing with
    # the jaws for the eye.
    for i in range(3):
        u = 28 + i * 36
        out.append(S.line([(u, 22), (u - 8, 106)], S.tint(AURA_D, 0.4), 3.0))
        out.append(S.line([(16, u), (112, u - 8)], S.tint(AURA_D, 0.4), 3.0))
    jaw = "#9E6535"
    out += S.fuse([S.rect(64, 30, 92, 14, jaw, r=6)])
    out += S.fuse([S.rect(64, 112, 92, 14, jaw, r=6)])
    for x in (34, 54, 74, 94):
        out += S.fuse([S.tri([(x - 10, 36), (x, 62), (x + 10, 36)], jaw)])
        out += S.fuse([S.tri([(x - 10, 106), (x, 80), (x + 10, 106)], jaw)])
    return out


def _skill_compound():
    """Three coins, each larger than the last. Growth said as a shape rather
    than as an arrow, which every other growth icon in the set already uses."""
    return (_coin(30, 102, 17) + _coin(60, 86, 25) + _coin(94, 60, 32))


def _sub1():
    d = ART + "/skills/"
    K = {
        "dent": _skill_dent,
        "overtime": _skill_overtime,
        "severance": _skill_severance,
        "traps": _skill_traps,
        "compound": _skill_compound,
    }
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# ------------------------------------------ job sub-tree ladders, two ------
# The five that needed the game to start remembering something. Same rule as
# batch one: each has to be tellable at 48px from the job icon directly above
# it, so none of these may be a paw, a coin or a ring.


def _skill_momentum():
    """Three chevrons breaking forward, each one further along and brighter.
    Speed said as repetition rather than as a motion blur, which does not
    survive 48px."""
    out = []
    for i, (x, c) in enumerate(((30, "#8A5138"), (62, "#B5763F"), (94, "#E0954C"))):
        out += S.fuse([S.tri([(x - 16, 34), (x + 12, 70), (x - 16, 106)], c)])
    return out


def _skill_carry():
    """Rings spreading from a planted paw. The paw is DOWN - the node is about
    not moving, and a paw mid-swipe would say the opposite."""
    # The paw goes LAST, largest and darkest. The first version drew it small
    # in a tint of the same blue the rings use, and at 48px the whole icon
    # collapsed into one rosette with no paw in it at all.
    return (_ring(64, 74, 54, S.tint(AURA_C, 0.55), beads=22, w=6)
            + _ring(64, 74, 38, S.tint(AURA_C, 0.3), beads=16, w=6)
            + _paw(64, 74, 30, "#2F5C72"))


def _skill_misdirection():
    """Four arrows turned inward on a point. The only inward-pointing icon in
    the set, which is exactly what the node does."""
    out = []
    for x, y, dn in ((64, 22, True), (64, 114, False)):
        out += _arrow(x, y, 17, "#9A6FC0", down=dn)
    out += S.fuse([S.tri([(26, 50), (26, 86), (56, 68)], "#9A6FC0")])
    out += S.fuse([S.tri([(102, 50), (102, 86), (72, 68)], "#9A6FC0")])
    return out


def _skill_fence():
    """A coin behind a paling fence. The pun is the point and it happens to be
    the clearest silhouette available - nothing else in the set has uprights."""
    out = _coin(64, 62, 30)
    for x in (26, 48, 70, 92, 114):
        out += S.fuse([S.rect(x, 92, 13, 52, "#6E7486", r=3),
                       S.tri([(x - 7, 70), (x, 58), (x + 7, 70)], "#6E7486")])
    out.append(S.line([(16, 88), (116, 88)], S.shade("#6E7486", 0.35), 6.0))
    return out


def _skill_patience():
    """An hourglass with the sand nearly through, and a spark waiting at the
    bottom. Slowness, then the payoff."""
    glass = S.fuse([S.tri([(30, 22), (98, 22), (64, 66)], "#C9B98F"),
                    S.tri([(30, 116), (98, 116), (64, 72)], "#C9B98F")])
    return (glass
            + [S.rect(64, 20, 76, 11, "#8A7A52", r=4, s=False),
               S.rect(64, 118, 76, 11, "#8A7A52", r=4, s=False),
               S.tri([(46, 34), (82, 34), (64, 60)], PAL.GOLD, s=False),
               S.tri([(52, 110), (76, 110), (64, 92)], PAL.GOLD, s=False)])


def _sub2():
    d = ART + "/skills/"
    K = {
        "momentum": _skill_momentum,
        "carry": _skill_carry,
        "misdirection": _skill_misdirection,
        "fence": _skill_fence,
        "patience": _skill_patience,
    }
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# ------------------------------------------------ job sub-tree verbs -------
# The verbs. A verb icon shows the THING THAT HAPPENS, not the stat that moves,
# which is what keeps them apart from the ladders drawn above - those are all
# arrows, bars and growth, and these are all objects.


def _skill_pickpocket():
    """A coin leaving a pouch. The coin sits outside the pouch's mouth, which
    is the whole story in one shape."""
    # A POCKET, not a pouch. Two drafts of a round drawstring bag with a coin
    # on a string beside it read as a lit bomb at 48px - the sphere and the
    # spark are that silhouette and nothing about the colour rescued it. A
    # patch pocket has corners, so it cannot be mistaken for a bomb.
    pocket = S.fuse([S.rect(56, 84, 76, 72, "#4E6E92", r=8)])
    return (pocket
            + [S.rect(56, 54, 76, 13, "#3C566F", r=5, s=False),
               S.line([(24, 66), (88, 66)], "#32485E", 4.0),
               S.line([(40, 96), (72, 96)], S.tint("#4E6E92", 0.3), 4.0)]
            + _coin(92, 40, 22))


def _skill_long_lunch():
    """A mug, steaming. Reads at any size and nothing else in the set is a
    cylinder."""
    mug = S.fuse([S.rect(58, 84, 56, 60, "#D8DEE8", r=8),
                  S.ell(96, 78, 16, 15, "#D8DEE8")])
    return (mug
            + [S.ell(96, 78, 8, 7, S.tint("#D8DEE8", 0.55), s=False),
               S.rect(58, 66, 48, 12, "#9A6B4A", r=4, s=False)]
            + [S.line([(44, 44), (38, 30), (46, 18)], "#E8EEF4", 5.0),
               S.line([(62, 42), (68, 28), (60, 16)], "#E8EEF4", 5.0)])


def _skill_levy():
    """A wax seal stamped on paper. A tithe is a thing that is OWED and then
    settled, and a seal is the settling."""
    return (S.fuse([S.rect(58, 76, 70, 84, PAPER_W, r=5)])
            + [S.rect(54, 48, 46, 6, METAL_DARK, r=2, s=False),
               S.rect(54, 62, 46, 6, METAL_DARK, r=2, s=False)]
            + S.fuse([S.ell(84, 92, 30, 30, "#B84A46")])
            + [S.ell(84, 92, 17, 17, S.shade("#B84A46", 0.3), s=False)])


def _skill_infestation():
    """Three mice, one behind the other. The crowd is the effect, so the icon
    is a crowd."""
    return (xform_parts(S.creature(S.CAST()["mouse"]), 4, 36, 0.34)
            + xform_parts(S.creature(S.CAST()["mouse"]), 40, 44, 0.34)
            + xform_parts(S.creature(S.CAST()["mouse"]), 22, 4, 0.38))


def _skill_sleight_of_hand():
    """An empty top hat with a puff where the mouse was. The Magician's own
    node is a wand, so the hat keeps the pair apart."""
    hat = S.fuse([S.ell(64, 104, 52, 13, "#3A3346"),
                  S.rect(64, 72, 56, 58, "#3A3346", r=6)])
    return (hat
            + [S.rect(64, 92, 58, 12, "#7A4FA0", r=3, s=False)]
            + S.fuse([S.ell(30, 56, 11, 10, S.tint(PAL.STARDUST, 0.4)),
                      S.ell(20, 44, 7, 7, S.tint(PAL.STARDUST, 0.4))])
            + S.fuse([S.ell(100, 52, 10, 9, S.tint(PAL.STARDUST, 0.4)),
                      S.ell(110, 40, 6, 6, S.tint(PAL.STARDUST, 0.4))]))


def _sub3():
    d = ART + "/skills/"
    K = {
        "pickpocket": _skill_pickpocket,
        "long_lunch": _skill_long_lunch,
        "levy": _skill_levy,
        "infestation": _skill_infestation,
        "sleight_of_hand": _skill_sleight_of_hand,
    }
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# ------------------------------------------- job sub-tree verbs, two -------


def _skill_anvil_paw():
    """An anvil with a paw print struck into its face. The anvil carries the
    name and the paw says whose it is."""
    anvil = S.fuse([S.rect(64, 92, 92, 26, METAL_DARK, r=6),
                    S.rect(64, 66, 50, 30, METAL_DARK, r=5),
                    S.tri([(18, 56), (52, 50), (52, 74)], METAL_DARK)])
    return anvil + [S.rect(64, 108, 62, 12, S.shade(METAL_DARK, 0.3), r=4, s=False)] \
        + _paw(74, 66, 17, S.tint(METAL_DARK, 0.55))


def _skill_crash_landing():
    """A crater: a ring of thrown-up ground with flame in the middle. Round,
    but the only round icon in the set that is also on fire."""
    out = _ring(64, 80, 44, "#7A4A32", beads=14, w=13)
    out += S.fuse([S.ell(64, 82, 26, 20, "#D66A38")])
    out += S.fuse([S.tri([(50, 78), (64, 42), (78, 78)], "#F0A64A")])
    out.append(S.ell(64, 84, 10, 8, S.tint("#F0A64A", 0.5), s=False))
    return out


def _skill_echo():
    """Two rings, the second offset and fainter - the first one having moved
    on. The offset is the whole idea, so the rings must not be concentric."""
    return (_ring(46, 62, 34, S.tint(AURA_C, 0.55), beads=16, w=8)
            + _ring(82, 88, 34, AURA_C, beads=16, w=9))


def _skill_riddle():
    """A question mark over a paw. The Sphinx's own node is a question mark cut
    from stone, so this one is smaller and sits ON something."""
    out = _paw(48, 96, 26, S.tint(PAL.GOLD, 0.3))
    ink = "#2E2838"
    out.append(S.line([(66, 34), (86, 24), (102, 36), (92, 52), (82, 60), (82, 74)],
                      ink, 9.0))
    out.append(S.ell(82, 92, 7, 7, ink, s=False))
    return out


def _skill_blueprint():
    """A rolled plan with a drawn line on it. Paper again, like Levy - but
    Levy's is stamped and square and this one is a scroll."""
    sheet = S.fuse([S.rect(64, 70, 96, 88, "#3E6E9E", r=7)])
    out = sheet
    for i in range(3):
        y = 42 + i * 28
        out.append(S.line([(20, y), (108, y)], S.tint("#3E6E9E", 0.22), 2.0))
        x = 36 + i * 28
        out.append(S.line([(x, 28), (x, 112)], S.tint("#3E6E9E", 0.22), 2.0))
    # The drafted shape, bold and white over the grid.
    out.append(S.line([(38, 92), (38, 52), (86, 52), (86, 92)], "#F2F7FC", 6.0))
    out.append(S.line([(30, 92), (94, 92)], "#F2F7FC", 6.0))
    return out


def _sub4():
    d = ART + "/skills/"
    K = {
        "anvil_paw": _skill_anvil_paw,
        "crash_landing": _skill_crash_landing,
        "echo": _skill_echo,
        "riddle": _skill_riddle,
        "blueprint": _skill_blueprint,
    }
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# --------------------------------------------- the jobless capstones -------
# Five keystones, one per trunk. These want to look HEAVIER than the nodes
# around them - they are the end of a line, not a rung on it - so each is built
# on one large central shape rather than on a cluster.


def _skill_butchers_rhythm():
    """A cleaver over a row of beats. The beats are what the node is about and
    the cleaver is whose beats they are."""
    # A WIDE FLAT RECTANGLE IS A TRAY, NOT A CLEAVER, which is what the first
    # draft read as at 48px. What makes a cleaver is the proportion - taller
    # than it is wide - plus a spine that is visibly thicker than the edge, and
    # a handle long enough to be a handle rather than a tab.
    blade = S.fuse([S.rect(50, 58, 58, 66, "#C4CAD6", r=4),
                    S.rect(88, 34, 30, 14, "#6E4A3A", r=5)])
    out = blade + [
        S.rect(50, 32, 60, 12, S.shade("#C4CAD6", 0.35), r=3, s=False),
        S.line([(22, 88), (78, 88)], S.tint("#C4CAD6", 0.6), 6.0),
    ]
    for i, x in enumerate((30, 56, 82)):
        c = PAL.GOLD if i == 2 else S.tint("#C4CAD6", 0.3)
        out += S.fuse([S.ell(x, 112, 10, 10, c)])
    return out


def _skill_standing_wave():
    """Three concentric rings of alternating weight - a pulse frozen. Echo is
    two rings SIDE BY SIDE, so concentric is what keeps these apart."""
    return (_ring(64, 70, 52, S.tint(AURA_C, 0.5), beads=22, w=7)
            + _ring(64, 70, 34, AURA_C, beads=16, w=11)
            + _ring(64, 70, 16, S.shade(AURA_C, 0.2), beads=9, w=8))


def _skill_sixth_sense():
    """An eye with a marker pin beside it. Instinct's own icon is a plain eye,
    so this one is the eye plus the thing it has spotted."""
    return (_eye(52, 66, 34)
            + S.fuse([S.tri([(102, 118), (90, 84), (114, 84)], "#E0B44A"),
                      S.ell(102, 76, 15, 15, "#E0B44A")])
            + [S.ell(102, 76, 7, 7, S.shade("#E0B44A", 0.35), s=False)])


def _skill_compound_interest():
    """A coin with a clock face on it. Fortune's Compound is coins STACKED;
    this one is a single coin that tells the time, because the difference
    between the two nodes is per-wave against per-minute."""
    return (_coin(64, 70, 46)
            + [S.ell(64, 70, 30, 30, S.tint(PAL.GOLD, 0.45), s=False),
               S.ell(64, 70, 4, 4, "#6E5218", s=False),
               S.line([(64, 70), (64, 48)], "#6E5218", 5.0),
               S.line([(64, 70), (82, 78)], "#6E5218", 4.0)])


def _skill_gridlock():
    """A net with a moth caught in it. Snare's own icon is the bare net, and
    the thing in it is the whole point of this one - fliers are what Gridlock
    reaches and nothing else does."""
    out = _net(S.shade(AURA_D, 0.15))
    out += xform_parts(S.creature(S.CAST()["moth"]), 8, 6, 0.5)
    return out


def _sub5():
    d = ART + "/skills/"
    K = {
        "butchers_rhythm": _skill_butchers_rhythm,
        "standing_wave": _skill_standing_wave,
        "sixth_sense": _skill_sixth_sense,
        "compound_interest": _skill_compound_interest,
        "gridlock": _skill_gridlock,
    }
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# ------------------------------------- job sub-tree rule-changers ----------


def _skill_hammerfall():
    """A hammer head with a shockwave running out to the right. The wave is
    what the node adds, so it gets the width."""
    head = S.fuse([S.rect(40, 62, 46, 42, METAL_DARK, r=5),
                   S.rect(40, 100, 14, 34, "#6E4A3A", r=5)])
    out = head + [S.rect(40, 46, 48, 11, S.shade(METAL_DARK, 0.3), r=3, s=False)]
    for i, x in enumerate((76, 94, 112)):
        out.append(S.line([(x, 44 + i * 4), (x, 80 - i * 4)],
                          S.tint(METAL_DARK, 0.35 + i * 0.12), 7.0 - i * 1.2))
    return out


def _skill_crowd_work():
    """A ring packed with mice. Standing Wave is rings alone and Carry is a
    ring round a paw, so this one is the ring round a CROWD."""
    out = _ring(64, 70, 50, S.tint(AURA_C, 0.45), beads=20, w=8)
    out += xform_parts(S.creature(S.CAST()["mouse"]), 45, 45, 0.3)
    out += xform_parts(S.creature(S.CAST()["mouse"]), 20, 66, 0.3)
    out += xform_parts(S.creature(S.CAST()["mouse"]), 68, 62, 0.3)
    return out


def _skill_cat_burglars_cut():
    """A coin split down the middle, the halves pulled slightly apart. A CUT,
    said as literally as the set allows, and no other icon is a broken shape."""
    left = S.fuse([S.ell(48, 70, 30, 38, PAL.GOLD)])
    right = S.fuse([S.ell(84, 74, 30, 38, S.shade(PAL.GOLD, 0.18))])
    return (left + right
            + [S.line([(66, 26), (60, 116)], "#6E5218", 5.0),
               S.ell(46, 70, 11, 13, S.shade(PAL.GOLD, 0.3), s=False)])


def _skill_inscrutable():
    """A stone face, eyes closed, giving nothing away. The Sphinx's node is a
    riddle mark and this is the sphinx itself refusing to answer."""
    face = S.fuse([S.rect(64, 72, 74, 84, "#C9B98F", r=10)])
    return (face
            + [S.rect(64, 34, 78, 13, S.shade("#C9B98F", 0.25), r=5, s=False),
               S.line([(36, 64), (58, 64)], "#5E5238", 6.0),
               S.line([(70, 64), (92, 64)], "#5E5238", 6.0),
               S.line([(46, 96), (82, 96)], "#5E5238", 5.0),
               S.ell(64, 78, 6, 9, S.shade("#C9B98F", 0.2), s=False)])


def _skill_pied_piper():
    """A pipe at the centre with mice circling it. Ratcatcher's own icon is the
    pipe alone; the ring around it is what this node adds."""
    out = S.fuse([S.rect(64, 70, 15, 62, "#8A6A4A", r=6)])
    out += [S.ell(64, 42, 10, 8, "#6E5238", s=True),
            S.ell(64, 66, 4, 4, "#5A4230", s=False),
            S.ell(64, 84, 4, 4, "#5A4230", s=False)]
    out += xform_parts(S.creature(S.CAST()["mouse"]), 8, 46, 0.28)
    out += xform_parts(S.creature(S.CAST()["mouse"]), 80, 46, 0.28)
    out += xform_parts(S.creature(S.CAST()["mouse"]), 44, 86, 0.28)
    return out


def _sub6():
    d = ART + "/skills/"
    K = {
        "hammerfall": _skill_hammerfall,
        "crowd_work": _skill_crowd_work,
        "cat_burglars_cut": _skill_cat_burglars_cut,
        "inscrutable": _skill_inscrutable,
        "pied_piper": _skill_pied_piper,
    }
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# --------------------------------- job sub-tree rule-changers, two ---------


def _skill_nine_lives_insurance():
    """A paw print stamped on a policy. Paper again, but Levy's is sealed in
    red and Blueprint's is a blue grid, so a paw is what keeps this one apart."""
    return (S.fuse([S.rect(62, 70, 84, 92, PAPER_W, r=6)])
            + [S.rect(58, 38, 56, 6, METAL_DARK, r=2, s=False),
               S.rect(58, 52, 56, 6, METAL_DARK, r=2, s=False),
               S.rect(50, 66, 40, 6, METAL_DARK, r=2, s=False)]
            + _paw(70, 96, 24, S.tint("#C4756A", 0.1)))


def _skill_grand_finale():
    """A kitten drawn as an outline with stars around it - present, but not
    really there. The only hollow creature in the whole set."""
    parts = xform_parts(S.creature(S.CAST()["kitten"]), 30, 34, 0.62)
    ghost = []
    for p in parts:
        q = dict(p)
        q["fill"] = "#B79BD8"
        ghost.append(q)
    return (ghost
            + S.fuse([S.ell(20, 34, 7, 7, PAL.STARDUST)])
            + S.fuse([S.ell(108, 46, 6, 6, PAL.STARDUST)])
            + S.fuse([S.ell(98, 106, 5, 5, PAL.STARDUST)]))


def _skill_union_rules():
    """Two hats on one peg. The jobs ARE hats in this game, so two of them is
    the clearest possible statement of the rule."""
    # TWO DARK HATS SIDE BY SIDE ARE ONE DARK LUMP at 48px. They have to differ
    # in VALUE, not only in hue - the first draft had navy against brown, which
    # are the same brightness and merged into a single silhouette.
    return (S.fuse([S.ell(36, 84, 28, 19, "#7E869A"),
                    S.rect(36, 100, 58, 12, "#616B80", r=5)])
            + S.fuse([S.ell(94, 40, 26, 18, "#4A3428"),
                      S.rect(94, 56, 54, 11, "#35241B", r=5)])
            + [S.line([(12, 116), (116, 116)], METAL_DARK, 6.0),
               S.line([(94, 66), (94, 112)], S.tint(METAL_DARK, 0.3), 4.0)])


def _skill_audit():
    """A stamp coming down on a coin. Levy is a seal already ON the paper;
    this one is the stamp itself, mid-swing, over money."""
    return (_coin(58, 96, 30)
            + S.fuse([S.rect(74, 46, 58, 26, "#4E7A52", r=6),
                      S.rect(74, 22, 22, 26, "#3A5C3E", r=5)])
            + [S.rect(74, 60, 52, 8, "#2E4A32", r=3, s=False)])


def _skill_prototype():
    """A card standing proud of two others, with a spark on it. The card shape
    is unique in the set and the spark says which one is the lucky one."""
    out = S.fuse([S.rect(36, 82, 40, 58, "#3E6E9E", r=5)])
    out += S.fuse([S.rect(60, 76, 40, 58, "#4E7AA8", r=5)])
    out += S.fuse([S.rect(88, 62, 44, 64, "#6FA8C8", r=6)])
    out += [S.rect(88, 62, 32, 48, S.tint("#6FA8C8", 0.35), r=4, s=False)]
    out += _burst(88, 62, 22, PAL.GOLD, 7)
    return out


def _sub7():
    d = ART + "/skills/"
    K = {
        "nine_lives_insurance": _skill_nine_lives_insurance,
        "grand_finale": _skill_grand_finale,
        "union_rules": _skill_union_rules,
        "audit": _skill_audit,
        "prototype": _skill_prototype,
    }
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# ------------------------------------------------------------- swarm ------


def _skill_swarm():
    """One mouse arriving at speed, with motion chevrons behind it.

    HAS TO BE TELLABLE FROM TWO NEIGHBOURS. Infestation is three mice and no
    chevrons; Momentum is three chevrons and no mouse. This is the pairing, and
    the pairing is the whole idea - not more of them standing around, more of
    them ARRIVING."""
    out = []
    # Chevrons brighten toward the mouse, so the eye reads them as motion
    # arriving rather than as three separate marks.
    for x, c in ((8, "#36632F"), (34, "#599A4C"), (60, "#84CC66")):
        out += S.fuse([S.tri([(x, 26), (x + 24, 68), (x, 110)], c)])
    out += xform_parts(S.creature(S.CAST()["mouse"]), 46, 30, 0.6)
    return out


def _sub8():
    d = ART + "/skills/"
    return [(d + "skill_swarm.svg", 64, _skill_swarm)]



# ------------------------------------------------ phase 6: automation -----
# These say "this happens WITHOUT YOU", so each is a familiar object with the
# hand taken out of it - a signpost that walks, a hook that baits itself, a
# card that deals itself.


def _skill_auto_travel():
    """A signpost with motion chevrons - the road moving, not the traveller."""
    post = S.fuse([S.rect(40, 74, 14, 76, "#6E5238"),
                   S.rect(52, 44, 62, 26, "#4E8ABF", r=5)])
    return (post
            + [S.tri([(83, 31), (105, 44), (83, 57)], "#4E8ABF", s=False)]
            + [S.line([(22, 96), (36, 96)], S.tint("#4E8ABF", 0.3), 5.0),
               S.line([(16, 110), (36, 110)], S.tint("#4E8ABF", 0.5), 5.0)])


def _skill_auto_bait():
    """A bell on a spring, ringing itself. Town Crier's node is a plain bell, so
    this one has the coil and the ring lines."""
    return (S.fuse([S.ell(64, 62, 30, 28, "#D08A3C"),
                    S.rect(64, 86, 54, 12, "#B0702C", r=4)])
            + [S.ell(64, 98, 8, 8, "#8A5620", s=False),
               S.rect(64, 32, 12, 16, "#8A5620", r=4, s=False)]
            + [S.line([(20, 46), (30, 56)], "#E8B871", 5.0),
               S.line([(108, 46), (98, 56)], "#E8B871", 5.0)])


def _skill_auto_card():
    """Two cards, the top one lifting off the deck by itself."""
    return (S.fuse([S.rect(56, 88, 52, 66, "#6A5A8A", r=6)])
            + [S.rect(56, 88, 38, 50, S.tint("#6A5A8A", 0.3), r=4, s=False)]
            + S.fuse([S.rect(76, 48, 50, 62, "#9A86C4", r=6)])
            + [S.rect(76, 48, 36, 46, S.tint("#9A86C4", 0.35), r=4, s=False)]
            + _arrow(76, 14, 14, PAL.STARDUST))


def _sub9():
    d = ART + "/skills/"
    K = {"auto_travel": _skill_auto_travel, "auto_bait": _skill_auto_bait,
         "auto_card": _skill_auto_card}
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# ------------------------------------ phase 6: thresholds and compounding --


def _skill_swarm_logic():
    """A dense block of mice. Crowd_work is a ring AROUND a crowd; this is the
    crowd with no ring, because the node is about the count and nothing else."""
    out = []
    for dx, dy in ((14, 22), (52, 22), (88, 26), (30, 56), (68, 56), (48, 88)):
        out += xform_parts(S.creature(S.CAST()["mouse"]), dx, dy, 0.26)
    return out


def _skill_last_stand():
    """One mouse, alone, in a circle of claw marks. The mirror of Swarm Logic
    and drawn as its opposite: emptiness, with something about to happen."""
    return (_ring(64, 70, 48, S.tint(CLAW_C, 0.4), beads=12, w=7)
            + xform_parts(S.creature(S.CAST()["mouse"]), 34, 40, 0.46)
            + _claws(CLAW_C, 3, 64, 18, 50, 26))


def _skill_overflow():
    """A brimming cup running over. Catnip spilling, which is what the node
    turns a wasted kill into."""
    # The mound of green sits ABOVE the rim so the cup is visibly too full,
    # and the drips run down the front rather than floating beside it.
    out = S.fuse([S.ell(64, 50, 34, 14, LEAF_G),
                  S.ell(48, 44, 14, 12, LEAF_G),
                  S.ell(80, 44, 14, 12, LEAF_G)])
    out += S.fuse([S.rect(64, 84, 62, 58, "#C9CFD9", r=6),
                   S.rect(64, 56, 72, 12, "#AEB6C4", r=5)])
    out += [S.rect(64, 56, 72, 12, "#AEB6C4", r=5, s=False)]
    out += S.fuse([S.ell(42, 78, 7, 13, LEAF_G), S.ell(42, 96, 5, 8, LEAF_G)])
    out += S.fuse([S.ell(86, 74, 6, 11, LEAF_G)])
    return out


def _skill_reputation():
    """A wanted poster with a cat on it. The only portrait in the set."""
    return (S.fuse([S.rect(64, 70, 84, 96, PAPER_W, r=5)])
            + [S.rect(64, 32, 74, 10, METAL_DARK, r=3, s=False)]
            + xform_parts(S.creature(S.CAST()["kitten"]), 32, 30, 0.52)
            + [S.rect(64, 106, 60, 8, METAL_DARK, r=3, s=False)])


def _skill_grudge_ledger():
    """Tally marks - four and a diagonal. Counting, said as plainly as it can
    be, and nothing else in the set is pure marks."""
    out = S.fuse([S.rect(64, 72, 86, 92, PAPER_W, r=5)])
    for x in (36, 52, 68, 84):
        out.append(S.line([(x, 46), (x, 96)], "#4A4258", 6.0))
    out.append(S.line([(28, 96), (92, 46)], "#B84A46", 6.0))
    return out


def _skill_body_count():
    """A stacked cairn of tiny skulls - a pile that keeps growing."""
    out = []
    for cx, cy, r in ((42, 98, 19), (86, 98, 19), (64, 52, 22)):
        # Cranium over a narrower jaw. That silhouette IS the recognition; a
        # circle with two dots on it is a bowling ball.
        out += S.fuse([S.ell(cx, cy - r * 0.2, r, r * 0.9, "#E2DCCE"),
                       S.rect(cx, cy + r * 0.62, r * 1.0, r * 0.6, "#E2DCCE", r=3)])
        out.append(S.ell(cx - r * 0.38, cy - r * 0.22, r * 0.26, r * 0.32,
                         "#3A3446", s=False))
        out.append(S.ell(cx + r * 0.38, cy - r * 0.22, r * 0.26, r * 0.32,
                         "#3A3446", s=False))
        out.append(S.line([(cx - r * 0.5, cy + r * 0.5), (cx + r * 0.5, cy + r * 0.5)],
                          "#B9B2A2", 2.6))
    return out


def _sub10():
    d = ART + "/skills/"
    K = {"swarm_logic": _skill_swarm_logic, "last_stand": _skill_last_stand,
         "overflow": _skill_overflow, "reputation": _skill_reputation,
         "grudge_ledger": _skill_grudge_ledger, "body_count": _skill_body_count}
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]



# ------------------------------------------- the three missing cards -------
# Card art is 128px, twice the skill icons, so these can carry more detail.


def _card_hoarders():
    """A kitten with a mouse in its mouth, hauling. The drag is the card."""
    out = xform_parts(S.creature(S.CAST()["kitten"]), 30, 34, 0.7)
    out += xform_parts(S.creature(S.CAST()["mouse"]), 78, 66, 0.36)
    out.append(S.line([(86, 78), (110, 88)], S.tint(CLAW_C, 0.4), 5.0))
    return out


def _card_understudy():
    """A kitten stepping out of a signature mouse's amber outline."""
    ghost = []
    for p in xform_parts(S.creature(S.CAST()["mouse"]), 14, 30, 0.62):
        q = dict(p)
        q["fill"] = S.tint(PAL.GOLD, 0.55)
        ghost.append(q)
    return ghost + xform_parts(S.creature(S.CAST()["kitten"]), 52, 46, 0.62)


def _card_ricochet():
    """One burst and three arrows leaving it - the spill reaching further."""
    out = _burst(40, 60, 30, CLAW_C, 8)
    for x, y in ((82, 34), (92, 66), (80, 98)):
        out += S.fuse([S.tri([(x, y - 10), (x + 26, y), (x, y + 10)], CLAW_C)])
    return out


def _sub11():
    d = ART + "/cards/"
    K = {"hoarders": _card_hoarders, "understudy": _card_understudy,
         "ricochet": _card_ricochet}
    return [(d + "card_%s.svg" % k, 128, v) for k, v in sorted(K.items())]



# ------------------------------------------- phase 6: the last jobless -----


def _skill_kitten_foreman():
    """A kitten with a clipboard. Engineer's node is the clipboard alone, so
    the pairing is what tells them apart - and the joke is the point."""
    return (xform_parts(S.creature(S.CAST()["kitten"]), 18, 34, 0.66)
            + S.fuse([S.rect(96, 74, 34, 44, PAPER_W, r=4),
                      S.rect(96, 50, 18, 8, METAL_DARK, r=2)])
            + [S.line([(84, 66), (108, 66)], METAL_DARK, 3.0),
               S.line([(84, 78), (108, 78)], METAL_DARK, 3.0)])


def _skill_second_opinion():
    """Two arrows chasing each other in a circle - a reroll, said the way every
    game says it, which is the point of a quality-of-life icon."""
    out = _ring(64, 70, 38, S.tint(AURA_C, 0.35), beads=14, w=7)
    out += S.fuse([S.tri([(96, 44), (116, 62), (92, 70)], AURA_D)])
    out += S.fuse([S.tri([(32, 96), (12, 78), (36, 70)], AURA_D)])
    return out


def _skill_whisker_trade():
    """Whiskers crossed over a coin - the currency and what it is worth."""
    return (_coin(64, 82, 30)
            # Thicker and darker than the first draft: near-white hairlines
            # vanished at 48px, which is the size the icon is actually read at.
            + [S.line([(12, 28), (60, 50)], "#6E6656", 8.0),
               S.line([(12, 48), (58, 58)], "#6E6656", 7.0),
               S.line([(116, 28), (68, 50)], "#6E6656", 8.0),
               S.line([(116, 48), (70, 58)], "#6E6656", 7.0)])


def _skill_cartography():
    """A folded map with a route on it. Nothing else in the set is a map."""
    out = S.fuse([S.rect(64, 70, 96, 76, PAPER_W, r=4)])
    out += [S.line([(40, 32), (40, 108)], S.shade(PAPER_W, 0.22), 3.0),
            S.line([(88, 32), (88, 108)], S.shade(PAPER_W, 0.22), 3.0)]
    out.append(S.line([(26, 96), (48, 70), (78, 84), (102, 46)], "#C4564E", 5.0))
    out += S.fuse([S.ell(102, 44, 7, 7, "#C4564E")])
    return out


def _skill_deep_pockets():
    """A pocket with coins spilling UP out of it - the same pocket shape as
    Pickpocket, filling rather than being emptied."""
    out = S.fuse([S.rect(60, 88, 76, 64, "#4E6E92", r=8)])
    out += [S.rect(60, 62, 76, 13, "#3C566F", r=5, s=False)]
    out += _coin(36, 40, 17)
    out += _coin(66, 28, 15)
    out += _coin(94, 42, 16)
    return out


def _sub12():
    d = ART + "/skills/"
    K = {"kitten_foreman": _skill_kitten_foreman,
         "second_opinion": _skill_second_opinion,
         "whisker_trade": _skill_whisker_trade,
         "cartography": _skill_cartography,
         "deep_pockets": _skill_deep_pockets}
    return [(d + "skill_%s.svg" % k, 64, v) for k, v in sorted(K.items())]


BATCHES = {
    "player": _player,
    "vermin": _vermin,
    "gristle": _gristles,
    "birds": _birds,
    "oddities": _oddities,
    "nightlife": _nightlife,
    "bosses": _bosses,
    "icons": _icons,
    "jobs": _jobs,
    "rules": _rules,
    "cards": _cards,
    "skills": _skills,
    "missing": _missing,
    "newjobs": _new_jobs,
    "sub1": _sub1,
    "sub2": _sub2,
    "sub3": _sub3,
    "sub4": _sub4,
    "sub5": _sub5,
    "sub6": _sub6,
    "sub7": _sub7,
    "sub8": _sub8,
    "sub9": _sub9,
    "sub10": _sub10,
    "sub11": _sub11,
    "sub12": _sub12,
}

# What each family is actually played at on screen. The sheet renders this size
# beside the full one so the 48px test cannot be skipped.
PLAY_SIZE = {"player": 90, "bosses": 120, "icons": 32,
             "jobs": 64, "rules": 64, "skills": 64, "cards": 128}


def sheet(name, items, path):
    play = PLAY_SIZE.get(name, 48)
    # The full render is 128 wide and the play-size render sits beside it, so a
    # cell narrower than 128 + play + margins silently overlaps the next column.
    cols = 3
    cell = 128 + play + 46
    rows = (len(items) + cols - 1) // cols
    ground = PAL.BACKDROPS["tier_1_backyard"]["field"]
    shadow = PAL.BACKDROPS["tier_1_backyard"]["detail"]
    img = Image.new("RGB", (cols * cell, rows * 196 + 34), ground)
    d = ImageDraw.Draw(img)
    d.text((12, 10), "batch: %s   %d assets   left: full   right: %dpx, the size it is played at"
           % (name, len(items), play), fill=PAL.INK)
    for i, (rel, px, fn) in enumerate(items):
        cx, cy = (i % cols) * cell, (i // cols) * 196 + 30
        d.ellipse([cx + 26, cy + 116, cx + 122, cy + 132], fill=shadow)
        big = S.to_image(fn(), S.SIZE)
        img.paste(big, (cx + 8, cy + 4), big)
        small = big.resize((play, play), Image.LANCZOS)
        img.paste(small, (cx + 136, cy + 40), small)
        d.text((cx + 8, cy + 142), pathlib.Path(rel).stem, fill=PAL.INK)
        d.text((cx + 8, cy + 156), "%dpx on disk" % px, fill=PAL.INK)
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)
    return img


# ------------------------------------------------------------------ fitting ---
# EVERY DRAWING FILLS ITS SQUARE (designer, 2026-09-30). Two faults, one cause.
# Fifty of 201 drawings ran past the 128-unit viewBox and were CROPPED - the
# unicorn's horn and wings, every rat's tail, the seagull's wingtips - and 72
# filled less than 80% of it and played SMALL, because the game fits a sprite
# by its texture's longest edge (enemy.gd, player.gd, kitten.gd): a drawing
# occupying half the box shows at half the authored size. The drawing code
# places shapes by eye in a fixed 128 space, so neither fault shows until the
# file is loaded. Fitting at write time fixes both without touching a single
# drawing function, and the contact sheet shows exactly what gets written.
#
# Three modes. CREATURES are bottom-aligned, because enemy.gd hangs the ground
# shadow at the bottom edge of the box - centred, a short wide rat would float
# above its own shadow. ICONS are centred. OVERLAYS (hats, weapons) are drawn
# to be sized against the cat by player.gd, so filling them would enlarge them
# on the cat: they are only shrunk, and only if something is cropped.
FIT_MARGIN = 3.0


def extent(parts):
    """The drawing's true bounds, stroke included - an ink outline that crosses
    the edge is cropped exactly like a fill."""
    x0 = y0 = 1e9
    x1 = y1 = -1e9
    for p in parts:
        k = p["k"]
        pad = S.STROKE / 2 if (k != "line" and p.get("s", True)) else 0.0
        if k == "ell":
            xs = (p["cx"] - p["rx"], p["cx"] + p["rx"])
            ys = (p["cy"] - p["ry"], p["cy"] + p["ry"])
        elif k == "rect":
            xs = (p["cx"] - p["w"] / 2, p["cx"] + p["w"] / 2)
            ys = (p["cy"] - p["h"] / 2, p["cy"] + p["h"] / 2)
        else:
            if k == "line":
                pad = p["w"] / 2
            xs = (min(x for x, _ in p["pts"]), max(x for x, _ in p["pts"]))
            ys = (min(y for _, y in p["pts"]), max(y for _, y in p["pts"]))
        x0, x1 = min(x0, xs[0] - pad), max(x1, xs[1] + pad)
        y0, y1 = min(y0, ys[0] - pad), max(y1, ys[1] + pad)
    return x0, y0, x1, y1


def _fit_step(box, mode):
    x0, y0, x1, y1 = box
    lo, hi = FIT_MARGIN, S.SIZE - FIT_MARGIN
    if mode == "overlay":
        c = S.SIZE / 2.0
        reach = max(c - x0, x1 - c, c - y0, y1 - c)
        k = min(1.0, (c - FIT_MARGIN) / reach)
        return k, c * (1 - k), c * (1 - k)
    k = (hi - lo) / max(x1 - x0, y1 - y0)
    dx = S.SIZE / 2.0 - k * (x0 + x1) / 2.0
    dy = (hi - k * y1) if mode == "ground" else (S.SIZE / 2.0 - k * (y0 + y1) / 2.0)
    return k, dx, dy


def fit_transform(parts_list, mode):
    """(scale, dx, dy), p' = p * scale + d, fitting the UNION of `parts_list`.

    Iterated, because stroke width does not scale with the drawing - the SVG
    sets it once on the group - so one pass lands a stroke's width off."""
    k, dx, dy = 1.0, 0.0, 0.0
    for _ in range(3):
        boxes = [extent(EVO.xform(p, dx, dy, k)) for p in parts_list]
        box = (min(b[0] for b in boxes), min(b[1] for b in boxes),
               max(b[2] for b in boxes), max(b[3] for b in boxes))
        k2, dx2, dy2 = _fit_step(box, mode)
        k, dx, dy = k * k2, dx * k2 + dx2, dy * k2 + dy2
    return k, dx, dy


def fit_mode(filename):
    stem = pathlib.Path(filename).stem
    if stem.startswith(("hat_", "player_weapon_")):
        return "overlay"
    if stem.startswith(("enemy_", "boss_", "kitten_", "player_cat_")):
        return "ground"
    return "centre"


def player_fit():
    """ONE transform for every cat stage, not one each. player.gd places the
    hat and the weapon as fractions of the cat texture, so each stage fitted on
    its own would move the head and paws out from under them."""
    stages = [fn() for rel, _px, fn in _player() if "player_cat_" in rel]
    return fit_transform(stages, "ground")


def overlay_constants():
    """player.gd's overlay constants, carried through player_fit().

    An offset is a fraction of the texture from its centre, so a point at
    offset f sits at 64 + 128 f and lands at k (64 + 128 f) + d. Widths scale
    by k. SPRITE_TARGET_PX grows by 1/k so the cat is the same size on screen."""
    k, dx, dy = player_fit()
    def off(fx, fy):
        return ((k * (64 + 128 * fx) + dx - 64) / 128,
                (k * (64 + 128 * fy) + dy - 64) / 128)
    return {
        "scale": k,
        "SPRITE_TARGET_PX": 154.0 / k,
        "HAT_HEAD_OFFSET": off(0.0, -0.34),
        "HAT_WIDTH_FRACTION": 0.52 * k,
        "WEAPON_OFFSET": off(-0.22, 0.16),
        "WEAPON_WIDTH_FRACTION": 0.46 * k,
        "WEAPON_OFFSET_MOUNTED": off(-0.02, -0.02),
        "WEAPON_WIDTH_FRACTION_MOUNTED": 0.40 * k,
    }


STICKER = "#FFF9E3"     # palette.py panel paper - the same cream as every plate
STICKER_GROW = 5.0


def sticker(parts, grow=STICKER_GROW):
    """A cream silhouette `grow` units wider than the drawing, laid BEHIND it.

    HATS GET ONE (designer, 2026-09-30: "make the hats big and very visible so
    they don't get lost in the chaos"). A hat is ink-outlined like everything
    else on the field, and on the dark late biomes, under forty mice, an ink
    edge against a dark floor is no edge at all. A paper rim is: it is the
    material of every panel in the game, so it reads as a label pinned to the
    cat rather than one more creature in the crowd."""
    back = []
    for p in parts:
        q = dict(p)
        q["fill"] = STICKER
        q["s"] = False
        if q["k"] == "ell":
            q["rx"] += grow
            q["ry"] += grow
        elif q["k"] == "rect":
            q["w"] += grow * 2
            q["h"] += grow * 2
            q["r"] += grow
        elif q["k"] == "tri":
            gx = sum(x for x, _ in q["pts"]) / len(q["pts"])
            gy = sum(y for _, y in q["pts"]) / len(q["pts"])
            pts = []
            for x, y in q["pts"]:
                d = max(math.hypot(x - gx, y - gy), 0.001)
                pts.append((x + (x - gx) / d * grow * 1.5, y + (y - gy) / d * grow * 1.5))
            q["pts"] = pts
        else:
            q["w"] = q["w"] + grow * 2
        back.append(q)
    return back + parts


def fitted(items):
    """Wrap every item's drawing function in its fit. Applied before the sheet
    AND the write, so what is reviewed is what ships."""
    shared = player_fit()
    out = []
    for rel, px, fn in items:
        if pathlib.Path(rel).stem.startswith("hat_"):
            fn = (lambda fn=fn: sticker(fn()))
        mode = fit_mode(rel)
        if "player_cat_" in rel:
            k, dx, dy = shared
        else:
            k, dx, dy = fit_transform([fn()], mode)
        out.append((rel, px, (lambda fn=fn, k=k, dx=dx, dy=dy: EVO.xform(fn(), dx, dy, k))))
    return out


def write_batch(items, allow_new=False):
    """The no-new-files guard exists so a typo in a filename cannot quietly
    produce an orphan asset that nothing loads and nobody notices. Genuinely
    new art is a deliberate act, so it gets a deliberate flag (--new) rather
    than a weakened guard."""
    for rel, px, fn in items:
        target = ROOT / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() and not allow_new:
            sys.exit("refusing to create a new asset: %s\n"
                     "  this tool only overwrites files the game already loads" % rel)
        # newline="\n": the repo stores LF, and a Windows text-mode write turned
        # every line of all 201 files into a CRLF change.
        target.write_text(S.to_svg(fn(), S.SIZE, px=px), encoding="utf-8", newline="\n")
        print("  %-52s %dpx" % (rel, px))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--batch")
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--all", action="store_true",
                    help="with --write: every batch in one go")
    ap.add_argument("--new", action="store_true",
                    help="permit creating files that do not exist yet")
    args = ap.parse_args()

    if args.all and args.write:
        for name, fn in BATCHES.items():
            items = fitted(fn())
            bad = check_tokens() + check_no_background_fills(items)
            if bad:
                sys.exit("batch %r: %s" % (name, "; ".join(bad)))
            write_batch(items, args.new)
        return 0

    if args.list or not args.batch:
        for k, fn in BATCHES.items():
            print("%-12s %d assets" % (k, len(fn())))
        return 0

    if args.batch not in BATCHES:
        sys.exit("unknown batch %r; try --list" % args.batch)
    items = fitted(BATCHES[args.batch]())
    if args.batch == "player":
        print("player.gd overlay constants for this fit (carry them over by hand):")
        for key, v in overlay_constants().items():
            print("  %-30s %s" % (key, ("(%.3f, %.3f)" % v) if isinstance(v, tuple) else "%.3f" % v))

    bad = check_tokens() + check_no_background_fills(items)
    if bad:
        for b in bad:
            print("  TOKEN PROBLEM: %s" % b)
        sys.exit("token colours violate the palette rules; fix before writing")

    if args.write:
        write_batch(items, args.new)
        print("wrote %d assets for batch %r" % (len(items), args.batch))
        return 0

    out = ROOT / "build" / ("sweet_batch_%s.png" % args.batch)
    img = sheet(args.batch, items, out)
    print("wrote %s (%dx%d) - review before --write" % (out, img.width, img.height))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
