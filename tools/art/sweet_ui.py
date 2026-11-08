#!/usr/bin/env python3
"""The UI: 9-patch chrome, tab and quick icons, and the 41 achievement badges.

WHY THIS WAS THE LAST THING LEFT. Batches 1-11 redrew every creature, card,
skill, job and rule; sweet_biomes drew the floors and props. The side panel kept
its old art the whole time, because the UI textures were never part of a style
in the first place - scenes and the theme reference them by hard path, so they
survived the style removal untouched and simply moved folder. The designer spotted
it immediately: "the side panel still seems to be using old assets."

THE 9-PATCH RULES, which are not style choices and will break the game if broken:

  * ALL FOUR BUTTON STATES MUST SHARE ONE SILHOUETTE. ci/check_ui_theme.gd
    asserts it, and the reason is in tools/make_ui_placeholders.py's header: the
    four generated states once measured alpha bboxes of 1-255, 24-232, 28-228
    and 52-204, so the button visibly JUMPED on hover. Here every state is the
    same geometry function with a different fill.
  * THE CORNER RADIUS MUST BE SMALLER THAN THE THEME'S texture_margin. A 9-patch
    cuts the source at the margin and stretches what is inside; a corner that
    reaches past the cut gets stretched and smears.
  * THE MIDDLE MUST BE FLAT. It is the part that repeats. Anything textured
    there tiles visibly across a wide button.

PORT (Unreal, restyle R3's shapes, 2026-11-08): the plates are the plum spec's
(docs/WORKLIST-RESTYLE-2026-11-07.md §4): a Deep Plum edge, one solid Deep Plum
shadow straight down (panels 8 px, buttons 4), and a pressed button SINKS by its
shadow. So the pressed state's silhouette is the others' moved down by the
shadow (checked below), not the same box: the plate moving is the point. The
shadow lives inside the nine-patch's bottom margin, so the margins (and
PawUi::ChromeTable, which holds them) are read off these drawings now, not off
Godot's files; every corner and the shadow's stay inside the cut.

Sizes are READ OFF THE FILES BEING REPLACED, never chosen here - the theme's
margins were authored against them.

    py tools/sweet_ui.py                 # contact sheets -> build/, writes nothing
    py tools/sweet_ui.py --only input_   # a sheet of just the pieces named so
    py tools/sweet_ui.py --write         # writes assets/art/ui/

The button glyphs are drawn in tools/sweet_input.py and written here, into
assets/art/ui/input/<family>/ at 64 px (see that file's header).
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
import sweet_input as SI

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
# PORT (Unreal, 2026-11-05): copied from the Godot repo's tools/, unchanged except the output paths: the art goes to
# SourceArt/svg/<Godot's assets/art folder>, or to $PAW_ART_ROOT when set (tools/art/check_art.py).
import os
OUT = pathlib.Path(os.environ.get("PAW_ART_ROOT", ROOT / "SourceArt" / "svg")) / "ui"

INK = S.INK

# --- the UI palette ----------------------------------------------------------
# THE PANEL IS PAPER (designer, 2026-09-14: "we still have dark background
# behind upgrades menu"; and the whole scheme is Stacklands', whose every
# panel is cream with a black frame). It used to be the ink, on the argument
# that a dark panel would not compete with a pastel field. Stacklands settles
# that the other way: cream panel, black 3px frame, black text, and the
# frame is what keeps it apart from the board. All of these are palette.UI.
PANEL = PAL.UI["panel"]
PANEL_EDGE = PAL.UI["panel_frame"]
CREAM = PAL.UI["button"]
CREAM_HOVER = PAL.UI["button_hover"]
CREAM_PRESS = PAL.UI["button_pressed"]
CREAM_OFF = PAL.UI["button_disabled"]
SELECTED = PAL.UI["selected"]          # R4a: the held toggle (the open tab), Butter, sunk like the pressed plate
SLOT = PAL.UI["slot"]
# The light that icons are drawn in ON A PANEL. It was CREAM (the button colour) while the buttons were cream; since the
# plum flip the buttons are Plum Raised, and a gear or a door in that vanished on the plum panel (seen 2026-11-08).
LIGHT = PAL.UI["text"]
GOLD = PAL.GOLD
GOLD_DARK = S.shade(PAL.GOLD, 0.32)
TEAL = PAL.MATERIALS["aura"]
CLAW = PAL.MATERIALS["claw"]
LEAF = PAL.MATERIALS["leaf_dark"]
PLUM = PAL.header("blue")


EDGE = PAL.UI["panel_frame"]          # Deep Plum since R3's shapes
BUTTON_EDGE = PAL.UI["button_frame"]
SHADOW = PAL.UI["shadow"]
EDGE_W = 3.0                            # one ink weight: the grammar's stroke, rounded
GAP = 1.0                               # transparent all round, so the filtered edge never touches the cut


def plate(w, h, r, fill, shadow, edge=EDGE, sink=False):
    """A rounded plate with a Deep Plum edge and ONE solid shadow straight down. THE ONE SHAPE 9-PATCH CHROME IS ALLOWED
    TO BE. `sink`: the pressed state, the plate moved down onto its shadow (the same footprint's bottom)."""
    top = GAP + (shadow if sink else 0.0)
    bottom = h - GAP - (0.0 if sink else shadow)
    cx, cy, pw, ph = w / 2.0, (top + bottom) / 2.0, w - 2 * GAP, bottom - top
    parts = []
    if shadow and not sink:
        parts.append(S.rect(cx, cy + shadow, pw, ph, SHADOW, r=r, s=False))
    parts.append(S.rect(cx, cy, pw, ph, edge, r=r, s=False))
    parts.append(S.rect(cx, cy, pw - 2 * EDGE_W, ph - 2 * EDGE_W, fill, r=max(r - EDGE_W, 0.0), s=False))
    return parts


BUTTON_SHADOW = 4.0
PANEL_SHADOW = 8.0


# --------------------------------------------------------------- the chrome --

def CHROME():
    c = {}
    # Buttons: 256x64, margins 16. Radius 9 + the 4 px shadow + the gap stay inside the bottom cut (64 - 16 = 48: the
    # plate's corner starts at 59 - 9 = 50); the shortest button (34 px) still fits 16 + 16.
    for name, fill in (("normal", CREAM), ("hover", CREAM_HOVER),
                       ("pressed", CREAM_PRESS), ("disabled", CREAM_OFF)):
        c["ui_button_%s" % name] = ((256, 64), (lambda f, sink: (lambda: plate(
            256, 64, 9, f, BUTTON_SHADOW, BUTTON_EDGE, sink)))(fill, name == "pressed"))
    # R4a: the selected toggle (the open tab): the pressed plate's shape in Butter.
    c["ui_button_selected"] = ((256, 64), lambda: plate(256, 64, 9, SELECTED, BUTTON_SHADOW, BUTTON_EDGE, True))

    # Panel: margins 16 / 16 / 16 / 24 (the shortest wearer, a 42 px popup list, still fits), radius 12, the 8 px shadow.
    # Window: margins 24 / 24 / 24 / 32, radius 16. The small plate (the toast, the zone plate): margins 12 / 12 / 12 / 16,
    # radius 8, a 4 px shadow.
    c["ui_panel"] = ((128, 128), lambda: plate(128, 128, 12, PANEL, PANEL_SHADOW))
    c["ui_window"] = ((256, 256), lambda: plate(256, 256, 16, PANEL, PANEL_SHADOW))
    c["ui_plate_small"] = ((64, 48), lambda: plate(64, 48, 8, PANEL, BUTTON_SHADOW))
    c["ui_dock"] = ((128, 128), lambda: plate(128, 128, 12, PANEL, PANEL_SHADOW))
    c["ui_slot"] = ((96, 96), lambda: S.fuse([S.rect(48, 48, 88, 88, SLOT, r=12)]))
    c["ui_card_frame"] = ((160, 192), lambda: (
        S.fuse([S.rect(80, 96, 150, 182, CREAM, r=14)])
        + [S.rect(80, 96, 126, 158, S.shade(CREAM, 0.12), r=10, s=True)]))

    # Bars. 64x16 with a 6px radius - small enough that the theme can cut them.
    c["ui_bar_track"] = ((64, 16), lambda: S.fuse([S.rect(32, 8, 60, 12, SLOT, r=6)]))
    c["ui_bar_fill"] = ((64, 16), lambda: S.fuse([S.rect(32, 8, 60, 12, GOLD, r=6)]))
    c["ui_separator"] = ((64, 8), lambda: [S.rect(32, 4, 62, 3, GOLD_DARK, r=1, s=False)])

    # Skill-tree nodes.
    c["ui_skill_node"] = ((96, 96), lambda: S.fuse([S.ell(48, 48, 42, 42, CREAM)]))
    c["ui_skill_node_owned"] = ((96, 96), lambda: (
        S.fuse([S.ell(48, 48, 42, 42, GOLD)])
        + [S.ell(48, 48, 26, 26, S.tint(GOLD, 0.45), s=False)]))
    c["ui_skill_node_locked"] = ((96, 96), lambda: (
        S.fuse([S.ell(48, 48, 42, 42, SLOT)])
        + _padlock_parts(48, 50, 20, CREAM_OFF)))
    return c


def _padlock_parts(cx, cy, r, colour):
    arch = []
    for i in range(9):
        a = math.pi + i * (math.pi / 8.0)
        arch.append(S.ell(cx + math.cos(a) * r * 0.58,
                          cy - r * 0.40 + math.sin(a) * r * 0.58,
                          r * 0.16, r * 0.16, colour))
    return (S.fuse(arch)
            + S.fuse([S.rect(cx, cy + r * 0.30, r * 1.5, r * 1.2, colour, r=4)]))


# ---------------------------------------------------------------- the icons --
# Every icon is 128x128, the size the old ones were, and each is ONE object.


def _burst(cx, cy, r, colour, n=8):
    out = []
    for i in range(n):
        a = i * (6.283 / n)
        out.append(S.tri([(cx + math.cos(a + 0.3) * r * 0.34,
                           cy + math.sin(a + 0.3) * r * 0.34),
                          (cx + math.cos(a) * r, cy + math.sin(a) * r),
                          (cx + math.cos(a - 0.3) * r * 0.34,
                           cy + math.sin(a - 0.3) * r * 0.34)], colour))
    return S.fuse(out)


def _paw(cx, cy, r, colour=CLAW):
    out = S.fuse([S.ell(cx, cy + r * 0.42, r * 0.92, r * 0.74, colour)])
    for dx, dy, sc in ((-1.02, -0.72, 0.34), (-0.38, -1.02, 0.36),
                       (0.38, -1.02, 0.36), (1.02, -0.72, 0.34)):
        out += S.fuse([S.ell(cx + dx * r, cy + dy * r, r * sc, r * sc, colour)])
    return out


def _ring(cx, cy, r, colour, beads=16, w=10.0):
    out = []
    for i in range(beads):
        a = i * (6.283 / beads)
        out.append(S.ell(cx + math.cos(a) * r, cy + math.sin(a) * r,
                         w * 0.6, w * 0.6, colour))
    return S.fuse(out)


def _heart(cx, cy, r, colour=CLAW):
    return S.fuse([S.ell(cx - r * 0.48, cy - r * 0.34, r * 0.6, r * 0.58, colour),
                   S.ell(cx + r * 0.48, cy - r * 0.34, r * 0.6, r * 0.58, colour),
                   S.tri([(cx - r, cy - r * 0.1), (cx, cy + r * 0.98),
                          (cx + r, cy - r * 0.1)], colour)])


def _trophy(colour=GOLD):
    return (S.fuse([S.ell(64, 52, 34, 30, colour),
                    S.rect(64, 88, 18, 34, colour, r=4),
                    S.rect(64, 110, 56, 16, colour, r=5),
                    S.ell(28, 46, 12, 16, colour), S.ell(100, 46, 12, 16, colour)])
            + [S.ell(64, 50, 16, 14, S.shade(colour, 0.3), s=False)])


def _gear(colour=LIGHT):
    out = [S.ell(64, 64, 34, 34, colour)]
    for i in range(8):
        a = i * (6.283 / 8)
        out.append(S.rect(64 + math.cos(a) * 40, 64 + math.sin(a) * 40,
                          18, 18, colour, r=3))
    return S.fuse(out) + [S.ell(64, 64, 14, 14, PANEL, s=True)]


def _floppy(colour=TEAL):
    return (S.fuse([S.rect(64, 64, 84, 84, colour, r=8)])
            + [S.rect(64, 38, 44, 26, S.shade(colour, 0.3), r=3),
               S.rect(64, 94, 56, 32, S.tint(colour, 0.5), r=3),
               S.rect(76, 38, 12, 18, PANEL, r=2, s=False)])


def _door(colour=LIGHT):
    return (S.fuse([S.rect(64, 66, 72, 100, colour, r=8)])
            + [S.rect(64, 66, 52, 78, S.shade(colour, 0.14), r=5),
               S.ell(88, 70, 6, 6, GOLD_DARK)])


def _tree(colour=LEAF):
    return (S.fuse([S.rect(64, 96, 14, 44, "#B5793F", r=3),
                    S.ell(40, 56, 26, 24, colour), S.ell(88, 52, 26, 24, colour),
                    S.ell(64, 34, 26, 22, colour)])
            + [S.line([(64, 106), (64, 62)], "#8A5C30", 3.0)])


def _flag(colour=CLAW):
    return (S.fuse([S.rect(34, 68, 12, 92, "#B5793F", r=3),
                    S.tri([(40, 24), (104, 44), (40, 66)], colour)])
            + [S.ell(34, 114, 20, 8, S.shade("#B5793F", 0.2), s=False)])


def _cards(colour=LIGHT):
    out = []
    for dx, dy in ((-24, 8), (0, 0), (24, 8)):
        out += S.fuse([S.rect(64 + dx, 68 + dy, 44, 62, colour, r=6)])
    out.append(S.ell(64, 64, 13, 13, PLUM, s=False))
    return out


def _hat():
    return (S.fuse([S.rect(64, 52, 56, 62, PLUM, r=5),
                    S.rect(64, 90, 100, 16, PLUM, r=6)])
            + [S.rect(64, 74, 58, 12, GOLD, r=2)])


def _whisker_face():
    parts = S.fuse([S.ell(64, 76, 14, 11, PAL.CAT_NOSE)])
    for side in (-1, 1):
        for dy in (-16.0, 0.0, 14.0):
            parts.append(S.curve(64 + side * 12, 72 + dy * 0.3,
                                 64 + side * 34, 64 + dy,
                                 64 + side * 54, 58 + dy * 1.5,
                                 w=5.0, colour=LIGHT))
    return parts


def ICONS():
    i = {}
    # Tabs - each is the thing the tab is about, no text.
    i["ui_tab_upgrades"] = lambda: _paw(64, 68, 36)
    i["ui_tab_reawaken"] = lambda: _ring(64, 64, 38, TEAL) + _burst(64, 64, 22, GOLD, 6)
    i["ui_tab_realms"] = lambda: _flag()
    i["ui_tab_catches"] = lambda: _cards()
    i["ui_tab_awards"] = lambda: _trophy()
    i["ui_tab_jobs"] = lambda: _hat()
    # The quick bar along the bottom of the dock.
    i["ui_quick_settings"] = lambda: _gear()
    i["ui_quick_skills"] = lambda: _tree()
    i["ui_quick_save"] = lambda: _floppy()
    i["ui_quick_quit"] = lambda: _door()
    # Small marks.
    i["ui_close"] = lambda: [S.line([(36, 36), (92, 92)], LIGHT, 13.0),
                             S.line([(92, 36), (36, 92)], LIGHT, 13.0)]
    i["ui_tick"] = lambda: [S.line([(30, 68), (54, 92)], LEAF, 14.0),
                            S.line([(54, 92), (98, 36)], LEAF, 14.0)]
    i["ui_lock"] = lambda: _padlock_parts(64, 68, 34, CREAM_OFF)
    return i



# ------------------------------------------------- the achievement badges ---
# FORTY-ONE OF THEM, and they are read in a scrolling list where the eye moves
# down a column. So the design is a BADGE, not forty-one unrelated pictures:
# one rosette shape, a family symbol in the middle, and a rim colour for how far
# into that family the award is. A player scanning the list sees which family an
# award belongs to before they read a word of it, and how rare it is without
# comparing numbers.
#
# The names are not mine to choose - the HUD asks for "achievement_%s" % data.id
# and there are exactly these ids in /data.

BRONZE = PAL.MATERIALS["bronze"]
SILVER = PAL.MATERIALS["silver"]
TIER_COLOURS = [BRONZE, SILVER, PAL.GOLD]

# id prefix -> (family symbol, family colour). The trailing number is the tier.
ACHIEVEMENT_IDS = [
    "boss_1", "boss_2", "boss_3",
    "br_ferocity_5", "br_instinct_5", "br_mystique_6", "br_unicorn_1",
    "br_unicorn_5", "br_vitality_5",
    "catnip_1000", "catnip_100000", "catnip_10000000", "catnip_1000000000",
    "catnip_100000000000",
    "kills_1", "kills_100", "kills_1000", "kills_10000", "kills_50000",
    "kills_200000", "kills_1000000",
    "reawaken_1", "reawaken_2", "reawaken_3", "reawaken_5", "reawaken_8",
    "reawaken_12",
    "stage_2", "stage_3", "stage_4",
    "up_aura_radius_15", "up_currency_find_25", "up_kitten_count_10",
    "up_kitten_power_25", "up_kitten_speed_20", "up_overkill_15",
    "up_paw_damage_20", "up_paw_damage_60", "up_pounce_power_20",
    "up_shield_pierce_15", "up_spawn_rate_20",
]


def _family(aid):
    """(family key, sort value). The family is what the badge SHOWS; the sort
    value is what decides its rim, by rank within the family rather than by the
    raw number - 1,000,000,000 kills and 12 reawakens are both "the last one"."""
    head = aid.rsplit("_", 1)
    tail = head[1] if len(head) == 2 and head[1].isdigit() else "0"
    stem = head[0] if len(head) == 2 and head[1].isdigit() else aid
    return stem, int(tail)


def _rosette(colour, rim):
    """The one badge shape. Scalloped rim, flat centre to sit a symbol on."""
    beads = []
    for i in range(12):
        a = i * (6.283 / 12)
        beads.append(S.ell(64 + math.cos(a) * 40, 64 + math.sin(a) * 40, 16, 16, rim))
    return (S.fuse(beads + [S.ell(64, 64, 44, 44, rim)])
            + [S.ell(64, 64, 34, 34, colour, s=True)])


def _eye_mark(colour):
    return (S.fuse([S.ell(64, 64, 26, 17, colour)])
            + [S.ell(64, 64, 11, 11, PANEL, s=False),
               S.ell(64, 64, 5, 5, colour, s=False)])


def _shield(colour):
    return S.fuse([S.rect(64, 56, 40, 34, colour, r=6),
                   S.tri([(44, 66), (64, 92), (84, 66)], colour)])


def _coin(colour):
    return S.fuse([S.ell(64, 64, 24, 24, colour)]) + [
        S.ell(64, 64, 12, 12, S.shade(colour, 0.3), s=False)]


def _crown(colour):
    return S.fuse([S.rect(64, 76, 52, 16, colour, r=3),
                   S.tri([(40, 76), (38, 42), (54, 72)], colour),
                   S.tri([(54, 72), (64, 36), (74, 72)], colour),
                   S.tri([(74, 72), (90, 42), (88, 76)], colour)])


def _skull(colour):
    """THE BOSS FAMILY WAS TWO FANGS IN THE SAME COLOUR AS THE KILLS CLAWS, and
    at badge size three claws and two fangs are the same picture. A skull shares
    nothing with anything else in the set: it is the only closed round mass with
    holes in it."""
    return (S.fuse([S.ell(64, 58, 30, 28, colour),
                    S.rect(64, 86, 34, 22, colour, r=6)])
            + [S.ell(53, 56, 9, 10, PANEL, s=False),
               S.ell(75, 56, 9, 10, PANEL, s=False),
               S.rect(64, 70, 7, 8, PANEL, r=1, s=False),
               S.line([(52, 86), (52, 96)], PANEL, 3.0),
               S.line([(64, 86), (64, 96)], PANEL, 3.0),
               S.line([(76, 86), (76, 96)], PANEL, 3.0)])


def _horn(colour):
    out = S.fuse([S.tri([(52, 94), (64, 30), (76, 94)], colour)])
    for i in range(3):
        y = 84 - i * 18
        out.append(S.line([(56 + i * 3, y), (72 - i * 3, y - 4)],
                          S.shade(colour, 0.35), 3.0))
    return out


def _arrow(cx, cy, h, colour):
    """ONE arrow. _arrows() returns three and callers were SLICING that list to
    borrow a subset - out[:6], out[6:12] - which is a real bug waiting to fire:
    the list is fused parts, so an arbitrary slice can cut a shape's ink pass
    from its fill pass and leave half a drawing. Callers that want one arrow ask
    for one arrow."""
    return S.fuse([S.rect(cx, cy + h * 0.22, h * 0.26, h * 0.62, colour, r=2),
                   S.tri([(cx - h * 0.36, cy - h * 0.10),
                          (cx, cy - h * 0.56),
                          (cx + h * 0.36, cy - h * 0.10)], colour)])


def _arrows(colour):
    out = []
    for dx in (-22, 0, 22):
        out += _arrow(64 + dx, 66, 48, colour)
    return out


def _kittens(colour):
    out = []
    for dx, dy, r in ((-20, 8, 18), (18, 2, 20)):
        out += S.fuse([S.ell(64 + dx, 66 + dy, r, r * 0.9, colour),
                       S.tri([(64 + dx - r * 0.8, 66 + dy - r * 0.5),
                              (64 + dx - r * 0.5, 66 + dy - r * 1.5),
                              (64 + dx - r * 0.1, 66 + dy - r * 0.7)], colour),
                       S.tri([(64 + dx + r * 0.1, 66 + dy - r * 0.7),
                              (64 + dx + r * 0.5, 66 + dy - r * 1.5),
                              (64 + dx + r * 0.8, 66 + dy - r * 0.5)], colour)])
    return out


# family -> (symbol function, family colour)
FAMILIES = {
    "kills": (lambda c: _claws(c), CLAW),
    "catnip": (lambda c: _leaf(c), GOLD),
    "reawaken": (lambda c: _ring(64, 64, 26, c, 12, 9) + _burst(64, 64, 15, c, 6), TEAL),
    "stage": (_crown, PLUM),
    "boss": (_skull, LIGHT),
    "br_ferocity": (lambda c: _claws(c), CLAW),
    "br_instinct": (_eye_mark, TEAL),
    "br_mystique": (lambda c: S.fuse([S.tri([(38, 92), (64, 32), (90, 92)], c)]), PLUM),
    "br_unicorn": (_horn, GOLD),
    "br_vitality": (lambda c: _heart(64, 60, 30, c), CLAW),
    "up_paw_damage": (lambda c: _paw(64, 66, 26, c), CLAW),
    # A bare ring was indistinguishable from the reawaken badge, which is also a
    # teal ring. Reach is what this one is about, so it grows arrows outward.
    "up_aura_radius": (lambda c: _ring(64, 64, 28, c, 14, 9)
                       + _arrow(64, 30, 30, c)
                       + _arrow(64, 116, 30, c), TEAL),
    "up_currency_find": (_coin, GOLD),
    "up_kitten_count": (_kittens, LIGHT),
    "up_kitten_power": (lambda c: _kittens(c) + _burst(96, 34, 14, GOLD, 5), LIGHT),
    "up_kitten_speed": (lambda c: _kittens(c) + _arrow(100, 40, 34, TEAL), LIGHT),
    "up_overkill": (lambda c: _burst(64, 64, 36, c, 9), CLAW),
    "up_pounce_power": (lambda c: _paw(64, 80, 24, c) + _arrow(64, 34, 34, GOLD), CLAW),
    "up_shield_pierce": (_shield, TEAL),
    "up_spawn_rate": (_arrows, LEAF),
}


def _claws(colour):
    out = []
    for dx in (-20, 0, 20):
        out += S.fuse([S.tri([(64 + dx - 8, 34), (64 + dx + 7, 38),
                              (64 + dx, 94)], colour)])
    return out


def _leaf(colour):
    return (S.fuse([S.ell(64, 66, 22, 27, colour),
                    S.tri([(44, 54), (64, 26), (84, 54)], colour)])
            + [S.line([(64, 90), (64, 40)], S.shade(colour, 0.4), 3.4)])


def ACHIEVEMENTS():
    ranked = {}
    for aid in ACHIEVEMENT_IDS:
        fam, val = _family(aid)
        ranked.setdefault(fam, []).append((val, aid))
    tier_of = {}
    for fam, items in ranked.items():
        items.sort()
        n = len(items)
        for i, (_, aid) in enumerate(items):
            # Thirds of the family, so a one-entry family is gold and a seven
            # entry family climbs bronze -> silver -> gold.
            tier_of[aid] = min(2, int(i * 3 / max(n, 1)))

    out = {}
    for aid in ACHIEVEMENT_IDS:
        fam, _ = _family(aid)
        symbol, colour = FAMILIES[fam]
        rim = TIER_COLOURS[tier_of[aid]]

        def make(symbol=symbol, colour=colour, rim=rim):
            return _rosette(S.shade(PAL.INK, 0.05), rim) + symbol(colour)

        out["achievement_%s" % aid] = ((128, 128), make)
    return out


# --------------------------------------------------------------- rendering --

def all_pieces():
    out = {}
    for name, (size, fn) in CHROME().items():
        out[name] = (size, fn)
    for name, fn in ICONS().items():
        out[name] = ((128, 128), fn)
    out.update(ACHIEVEMENTS())
    for name, (_folder, fn) in SI.INPUT().items():
        out[name] = ((128, 128), fn)
    return out


def sheet(pieces, path, title):
    cols, cell = 6, 150
    rows = (len(pieces) + cols - 1) // cols
    img = Image.new("RGB", (cols * cell, rows * cell + 30), (236, 232, 224))
    d = ImageDraw.Draw(img)
    d.text((10, 8), title, fill=PAL.INK)
    for i, (name, (size, fn)) in enumerate(sorted(pieces.items())):
        cx, cy = (i % cols) * cell, (i // cols) * cell + 26
        # ON THE GROUND IT ACTUALLY SITS ON. Tab, quick and mark icons live on
        # the dark panel; reviewing cream art against a light card says nothing
        # about whether it reads. The chrome keeps a neutral card because it IS
        # the background in play.
        on_panel = name.startswith(("ui_tab_", "ui_quick_", "input_")) or name in (
            "ui_close", "ui_tick", "ui_lock")
        if on_panel:
            d.rectangle([cx + 4, cy, cx + cell - 4, cy + cell - 36],
                        fill=tuple(int(PANEL[k:k + 2], 16) for k in (1, 3, 5)))
        w, h = size
        tile = S.to_image_wh(fn(), w, h)
        scale = min((cell - 24) / float(w), (cell - 48) / float(h), 1.6)
        tile = tile.resize((max(1, int(w * scale)), max(1, int(h * scale))),
                           Image.LANCZOS)
        img.paste(tile, (cx + (cell - tile.width) // 2,
                         cy + (cell - 40 - tile.height) // 2), tile)
        d.text((cx + 8, cy + cell - 30), name.replace("ui_", ""), fill=PAL.INK)
        d.text((cx + 8, cy + cell - 18), "%dx%d" % (w, h), fill=PAL.INK)
    img.save(path)
    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--only", default="", help="sheet only the pieces with this prefix")
    args = ap.parse_args()
    pieces = all_pieces()
    input_folders = {name: folder for name, (folder, _fn) in SI.INPUT().items()}

    # THE SILHOUETTE RULE, checked here rather than discovered in the gate. Since R3's shapes the pressed plate sinks by
    # its shadow: its box is the others' with the top moved down by the shadow, nothing else.
    # At half alpha: the resampling leaves a faint halo to the canvas's edge.
    states = {n: S.to_image_wh(pieces["ui_button_%s" % n][1](), 256, 64).getchannel("A")
              .point(lambda a: 255 if a >= 128 else 0).getbbox()
              for n in ("normal", "hover", "pressed", "disabled")}
    rest = {states[n] for n in ("normal", "hover", "disabled")}
    x0, y0, x1, y1 = states["normal"]
    sunk = (x0, y0 + int(BUTTON_SHADOW), x1, y1)
    selected = S.to_image_wh(pieces["ui_button_selected"][1](), 256, 64).getchannel("A") \
        .point(lambda a: 255 if a >= 128 else 0).getbbox()
    if len(rest) != 1 or states["pressed"] != sunk or selected != sunk:
        print("BUTTON STATES DIFFER: %s, selected %s (pressed and selected should be %s)" % (states, selected, sunk))
        return 1
    print("three button states share one silhouette %s; pressed sinks by its shadow: %s" % (states["normal"], sunk))

    if args.write:
        OUT.mkdir(parents=True, exist_ok=True)
        for name, (size, fn) in pieces.items():
            w, h = size
            parts = fn()
            # A BADGE MAY NOT BE CROPPED (2026-09-30: achievement_up_aura_radius_15
            # ran 7 units past its box). Shrink-only, like make_sweet_art's
            # overlays: every other badge is untouched, and chrome never is -
            # its geometry is fixed by the Theme's 9-patch margins.
            if name.startswith("achievement_"):
                import make_sweet_art as M
                import sweet_evolutions as EVO
                k, dx, dy = M.fit_transform([parts], "overlay")
                parts = EVO.xform(parts, dx, dy, k)
            if name in input_folders:
                # One folder per family, written at 64 px (sweet_input.py).
                folder = OUT / "input" / input_folders[name]
                folder.mkdir(parents=True, exist_ok=True)
                (folder / (name + ".svg")).write_text(S.to_svg_wh(parts, w, h, 64, 64),
                                                      encoding="utf-8", newline="\n")
                continue
            # newline="\n": the repo stores LF; a Windows text-mode write is CRLF.
            (OUT / (name + ".svg")).write_text(S.to_svg_wh(parts, w, h),
                                               encoding="utf-8", newline="\n")
        print("wrote %d UI pieces to %s" % (len(pieces), OUT))
        return 0

    if args.only:
        pieces = {k: v for k, v in pieces.items() if k.startswith(args.only)}
    p = ROOT / "build" / ("sweet_ui%s.png" % (("_" + args.only.strip("_")) if args.only else ""))
    p.parent.mkdir(parents=True, exist_ok=True)
    img = sheet(pieces, p, "UI chrome and icons - on the panel colour they sit on")
    print("wrote %s (%dx%d) - review before --write" % (p, img.width, img.height))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
