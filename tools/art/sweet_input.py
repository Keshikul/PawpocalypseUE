#!/usr/bin/env python3
"""Button glyphs for the prompts, in the sweet grammar (controller plan 9).

Written by tools/sweet_ui.py --write, into assets/art/ui/input/<family>/, one
folder per family so a console port swaps a folder rather than code. This file
only DRAWS; sweet_ui owns rendering, the contact sheet and the write.

THE DESIGNER'S CALL (2026-09-30): our own glyphs, not a third-party pack.
Kenney's CC0 Input Prompts were the reference for what reads at 32 px - a
single bold label on a simple silhouette - and nothing was copied from them.

NAMED BY POSITION. `face_down` is the bottom face button on every pad: A on
an Xbox, the cross on a PlayStation, B on a Nintendo. Godot's joypad buttons
are positional too, so scripts/systems/input_glyphs.gd maps a binding to a
position once and the family only chooses the folder. PAD_GLYPHS there and
PAD_POSITIONS here are the same list; the input gate checks every file.

LETTERS ARE STROKES, not <text>. Godot's SVG rasteriser has no fonts, so a
text element would draw nothing. Each letter is a few ink polylines on a
10 x 14 grid (`_glyph`), which is also what keeps them as heavy as the
outline everything else wears.

KEYS ARE NOT HERE. A keycap is drawn at run time around the key's live label
(AZERTY, remaps). Only the arrow keys are images, because the game's font has
no arrow characters; they and the mouse live in the keyboard folder.
"""

from __future__ import annotations

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import palette as PAL
import sweet_shapes as S

INK = S.INK
PAPER = PAL.UI["button"]
PAPER_DIM = PAL.STACKLANDS["sand_detail"]
SLOT = PAL.UI["slot"]
GOLD = PAL.GOLD

# Xbox's four colours, as the nearest card families: green A, red B, blue X,
# yellow Y. The letter stays ink on all four, like every letter in the game.
XBOX_FACE = {"face_down": PAL.face("green"), "face_right": PAL.face("red"),
             "face_left": PAL.header("blue"), "face_up": PAL.header("yellow")}
# PlayStation's symbols carry the colour; the disc stays paper.
PS_SYMBOL = {"face_down": PAL.header("blue"), "face_right": PAL.face("red"),
             "face_left": PAL.header("coral"), "face_up": PAL.header("green")}

PAD_POSITIONS = [
    "face_down", "face_right", "face_left", "face_up",
    "shoulder_l", "shoulder_r", "trigger_l", "trigger_r",
    "dpad", "dpad_up", "dpad_down", "dpad_left", "dpad_right",
    "stick_l", "stick_r", "stick_l_click", "stick_r_click",
    "start", "select",
]
KEYBOARD_GLYPHS = ["mouse_left", "mouse_right", "mouse_middle", "mouse_move",
                   "arrow_up", "arrow_down", "arrow_left", "arrow_right"]
FAMILIES = ["xbox", "playstation", "steam_deck"]

# The label each family prints on each position.
LABELS = {
    "xbox": {"face_down": "A", "face_right": "B", "face_left": "X", "face_up": "Y",
             "shoulder_l": "LB", "shoulder_r": "RB", "trigger_l": "LT", "trigger_r": "RT",
             "stick_l": "L", "stick_r": "R", "stick_l_click": "LS", "stick_r_click": "RS"},
    "steam_deck": {"face_down": "A", "face_right": "B", "face_left": "X", "face_up": "Y",
                   "shoulder_l": "L1", "shoulder_r": "R1", "trigger_l": "L2", "trigger_r": "R2",
                   "stick_l": "L", "stick_r": "R", "stick_l_click": "L3", "stick_r_click": "R3"},
    "playstation": {"shoulder_l": "L1", "shoulder_r": "R1", "trigger_l": "L2", "trigger_r": "R2",
                    "stick_l": "L", "stick_r": "R", "stick_l_click": "L3", "stick_r_click": "R3"},
}


# ------------------------------------------------------------ stroke letters --
# Each letter is a list of polylines on a 10 wide x 14 tall grid, y down.

def _arc(cx, cy, rx, ry, t0, t1, n=14):
    return [(cx + rx * math.cos(math.radians(t0 + (t1 - t0) * i / n)),
             cy + ry * math.sin(math.radians(t0 + (t1 - t0) * i / n)))
            for i in range(n + 1)]


def _bowl(y0, y1, w):
    """A B/R/P bowl from the stem at x=0: out along y0, round, back along y1."""
    r = (y1 - y0) / 2.0
    return [(0, y0)] + _arc(w - r, y0 + r, r, r, -90, 90) + [(0, y1)]


LETTERS = {
    "A": [[(0, 14), (5, 0), (10, 14)], [(2.1, 9), (7.9, 9)]],
    "B": [[(0, 0), (0, 14)], _bowl(0, 7, 8.4), _bowl(7, 14, 10)],
    "X": [[(0.5, 0), (9.5, 14)], [(9.5, 0), (0.5, 14)]],
    "Y": [[(0, 0), (5, 7), (10, 0)], [(5, 7), (5, 14)]],
    "L": [[(0.5, 0), (0.5, 14), (9.5, 14)]],
    "R": [[(0, 14), (0, 0)], _bowl(0, 7.4, 9.4), [(4.2, 7.4), (10, 14)]],
    "T": [[(0, 0), (10, 0)], [(5, 0), (5, 14)]],
    "S": [_arc(5, 3.6, 4.8, 3.6, -25, -270) + _arc(5, 10.4, 4.8, 3.6, -90, 155)],
    "Z": [[(0.5, 0), (9.5, 0), (0.5, 14), (9.5, 14)]],
    "1": [[(1.8, 3.2), (5.4, 0), (5.4, 14)], [(1.8, 14), (9, 14)]],
    "2": [_arc(5, 4.2, 4.8, 4.2, 180, 385) + [(0.3, 14), (10, 14)]],
    "3": [_arc(5, 3.6, 4.6, 3.6, 205, 450), _arc(5, 10.4, 5, 3.6, -90, 160)],
}


def _glyph(text, cx, cy, h, colour=INK, w=None):
    """`text` centred on (cx, cy), `h` units tall, as ink strokes."""
    k = h / 14.0
    gap = 3.4
    width = len(text) * 10 + (len(text) - 1) * gap
    x0 = cx - width * k / 2.0
    y0 = cy - h / 2.0
    w = w if w is not None else max(h * 0.22, 5.0)
    out = []
    for i, ch in enumerate(text):
        ox = x0 + i * (10 + gap) * k
        for stroke in LETTERS[ch]:
            out.append(S.line([(ox + x * k, y0 + y * k) for x, y in stroke], colour, w))
    return out


def _outlined_stroke(pts, colour, w):
    """A coloured stroke with an ink edge: the ink under it, a size larger."""
    return [S.line(pts, INK, w + S.STROKE * 2), S.line(pts, colour, w)]


# ------------------------------------------------------------------- shapes --

def _face(fill, label=None, symbol=None, symbol_colour=None):
    parts = S.fuse([S.ell(64, 64, 52, 52, fill)])
    if label:
        parts += _glyph(label, 64, 64, 50)
    if symbol:
        parts += _ps_symbol(symbol, symbol_colour)
    return parts


def _ps_symbol(position, colour):
    w = 11.0
    if position == "face_down":          # cross
        return (_outlined_stroke([(42, 42), (86, 86)], colour, w)
                + _outlined_stroke([(86, 42), (42, 86)], colour, w))
    if position == "face_right":         # circle
        return _outlined_stroke(_arc(64, 64, 24, 24, -90, 290, 38), colour, w)
    if position == "face_left":          # square
        return _outlined_stroke([(42, 42), (86, 42), (86, 86), (42, 86), (42, 42)], colour, w)
    return _outlined_stroke([(64, 38), (90, 84), (38, 84), (64, 38)], colour, w)   # triangle


def _shoulder(label):
    return (S.fuse([S.rect(64, 70, 112, 60, PAPER, r=22)])
            + [S.rect(64, 46, 84, 6, PAPER_DIM, r=3, s=False)]
            + _glyph(label, 64, 72, 34))


def _trigger(label):
    return (S.fuse([S.rect(64, 70, 84, 100, PAPER, r=30)])
            + [S.rect(64, 30, 50, 6, PAPER_DIM, r=3, s=False)]
            + _glyph(label, 64, 76, 34))


def _dpad(lit=None):
    arms = {"up": (64, 34), "down": (64, 94), "left": (34, 64), "right": (94, 64)}
    parts = S.fuse([S.rect(64, 64, 38, 112, PAPER, r=8),
                    S.rect(64, 64, 112, 38, PAPER, r=8)])
    parts.append(S.rect(64, 64, 34, 34, PAPER, r=4, s=False))
    if lit:
        x, y = arms[lit]
        parts.append(S.rect(x, y, 30, 30, GOLD, r=5, s=False))
    for name, (x, y) in arms.items():
        a = {"up": -90, "down": 90, "left": 180, "right": 0}[name]
        tip = (x + math.cos(math.radians(a)) * 9, y + math.sin(math.radians(a)) * 9)
        l = (x + math.cos(math.radians(a + 120)) * 8, y + math.sin(math.radians(a + 120)) * 8)
        r = (x + math.cos(math.radians(a - 120)) * 8, y + math.sin(math.radians(a - 120)) * 8)
        parts.append(S.tri([tip, l, r], INK, s=False))
    return parts


def _stick(label, clicked=False):
    parts = S.fuse([S.ell(64, 64, 54, 54, SLOT)])
    parts += S.fuse([S.ell(64, 64 if not clicked else 68, 38, 38, PAPER)])
    if clicked:
        # Pressed IN: the cap sits low in its well, with a ring under it.
        parts.append(S.ell(64, 68, 30, 30, PAPER_DIM, s=False))
        parts += _glyph(label, 64, 68, 28)
    else:
        parts += _glyph(label, 64, 64, 34)
    return parts


def _pill(inner):
    return S.fuse([S.rect(64, 64, 104, 62, PAPER, r=26)]) + inner


def _menu_lines():
    return [S.line([(46, y), (82, y)], INK, 7.0) for y in (52, 64, 76)]


def _view_squares():
    return [S.rect(58, 58, 24, 20, PAPER, r=3), S.rect(70, 70, 24, 20, PAPER, r=3)]


def _share_mark():
    return [S.line([(64, 80), (64, 46)], INK, 7.0),
            S.line([(52, 58), (64, 46), (76, 58)], INK, 7.0)]


def pad_glyph(family, position):
    lab = LABELS[family]
    if position.startswith("face_"):
        if family == "xbox":
            return _face(XBOX_FACE[position], lab[position])
        if family == "playstation":
            return _face(PAPER, symbol=position, symbol_colour=PS_SYMBOL[position])
        # The Deck's buttons are monochrome.
        return _face(PAPER, lab[position])
    if position.startswith("shoulder_"):
        return _shoulder(lab[position])
    if position.startswith("trigger_"):
        return _trigger(lab[position])
    if position == "dpad":
        return _dpad()
    if position.startswith("dpad_"):
        return _dpad(position[5:])
    if position in ("stick_l", "stick_r"):
        return _stick(lab[position])
    if position.endswith("_click"):
        return _stick(lab[position], clicked=True)
    if position == "start":
        return _pill(_menu_lines())
    if position == "select":
        if family == "playstation":
            return _pill(_share_mark())
        return _pill(_view_squares())
    raise KeyError(position)


# ----------------------------------------------------- keyboard and mouse --

def _mouse(lit):
    parts = S.fuse([S.rect(64, 66, 66, 100, PAPER, r=32)])
    if lit == "left":
        parts.append(S.rect(47, 38, 26, 34, GOLD, r=10, s=False))
    elif lit == "right":
        parts.append(S.rect(81, 38, 26, 34, GOLD, r=10, s=False))
    parts.append(S.line([(64, 18), (64, 56)], INK, S.STROKE))
    parts.append(S.line([(34, 56), (94, 56)], INK, S.STROKE))
    parts += S.fuse([S.rect(64, 38, 12, 22, GOLD if lit == "middle" else PAPER_DIM, r=6)])
    if lit == "move":
        for sx in (-1, 1):
            parts.append(S.tri([(64 + sx * 50, 66), (64 + sx * 40, 56), (64 + sx * 40, 76)],
                               INK, s=False))
    return parts


def _arrow_key(direction):
    parts = (S.fuse([S.rect(64, 64, 104, 104, PAPER, r=16)])
             + [S.rect(64, 104, 88, 10, PAPER_DIM, r=4, s=False)])
    a = {"up": -90, "down": 90, "left": 180, "right": 0}[direction]
    tip = (64 + math.cos(math.radians(a)) * 28, 58 + math.sin(math.radians(a)) * 28)
    l = (64 + math.cos(math.radians(a + 130)) * 26, 58 + math.sin(math.radians(a + 130)) * 26)
    r = (64 + math.cos(math.radians(a - 130)) * 26, 58 + math.sin(math.radians(a - 130)) * 26)
    parts.append(S.tri([tip, l, r], INK, s=False))
    return parts


def keyboard_glyph(name):
    if name.startswith("mouse_"):
        return _mouse(name[6:])
    return _arrow_key(name[6:])


# ------------------------------------------------------------------- the set --

def INPUT():
    """name -> (folder, draw fn). Every glyph is a 128-unit drawing written at
    64 px: prompts sit at 24-40 px, and UI art has no mipmaps (set_mipmaps.py),
    so a 128 px texture minified four times would shimmer."""
    out = {}
    for family in FAMILIES:
        for position in PAD_POSITIONS:
            out["input_%s_%s" % (family, position)] = (
                family, (lambda f=family, p=position: pad_glyph(f, p)))
    for name in KEYBOARD_GLYPHS:
        out["input_keyboard_%s" % name] = ("keyboard", (lambda n=name: keyboard_glyph(n)))
    return out
