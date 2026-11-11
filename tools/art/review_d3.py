#!/usr/bin/env python3
"""Restyle R5, decision D3: the review sheet for the aura and the paw strike (docs/WORKLIST-RESTYLE-2026-11-07.md §3).

Writes build/review/d3_aura_strike.png. Writes nothing the game loads.

Everything is drawn at the game's own scale (1080 logical pixels high, a field pixel = a screen pixel at 1080p) from the
game's own art and numbers:
  - the floors: SourceArt/svg/backdrops/backdrop_tier_N.png tiled at Pc::Backdrop::TileScale (0.5);
  - the cat: player_cat_base.svg fitted to CatPx (175); the enemies: their EnemyData texture at sprite_target_px;
  - the shadows: the atlas's ground_shadow, placed as PawFieldLook.h's Shadow() does;
  - the aura: base_aura_radius 77 x the Aura Radius upgrade (x3 at its cap): drawn at 150 (mid-run);
  - the swipe: PawVfx.cpp's Swipe geometry (reach 62 x 0.88, the arc's span and lead, the claws), line for line;
  - the paws: the game's two cursor drawings, ui_cursor_paw_open.svg and ui_cursor_paw_closed.svg.

    py tools/art/review_d3.py
"""

from __future__ import annotations

import csv
import io
import math
import pathlib
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import fx_look as FX  # noqa: E402  (the approved values: what the game draws)
import palette as PAL  # noqa: E402

import resvg_py  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
SVG = ROOT / "SourceArt" / "svg"
SS = 2  # supersampling

PLUM = PAL.PLUM
INK = PAL.INK


def rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def rgba(h, a):
    return rgb(h) + (int(round(a * 255)),)


def f2hex(r, g, b):
    return "#%02X%02X%02X" % (round(r * 255), round(g * 255), round(b * 255))


# --- the game's numbers ---------------------------------------------------------------------------------------------
CAT_PX = 175.0
TILE_SCALE = 0.5
SHADOW_ALPHA, SHADOW_WIDTH, SHADOW_FEET = 0.7, 0.8, 0.46
COMBAT = next(csv.DictReader(open(ROOT / "Data" / "CombatConstantsData.csv", encoding="utf-8")))
BASE_AURA = float(COMBAT["base_aura_radius"])
PAW_REACH = float(COMBAT["base_paw_reach"])
AURA_R = 150.0
# PawVfx.cpp (vfx_swipe.gd)
SWIPE_SEG, ARC_SPAN, REACH_FRAC, SWIPE_W, SWIPE_EDGE = 20, 1.55, 0.88, 9.0, 4.0
CLAWS, CLAW_LEN, CLAW_GAP, CLAW_W, CLAW_AT = 3, 26.0, 9.0, 4.5, 0.35
SWIPE_COL = f2hex(0.996, 0.965, 0.918)
CRIT_COL, CRIT_A, CRIT_SCALE = f2hex(1.0, 0.42, 0.72), 0.95, 1.5
# aura_ring.gd
AURA_GOLD = f2hex(0.965, 0.761, 0.361)

ENEMIES = {r["id"]: r for r in csv.DictReader(open(ROOT / "Data" / "EnemyData.csv", encoding="utf-8"))}


def svg_img(path, longest):
    """An SVG rasterised so its longest edge is `longest` px (the game fits sprites by the texture's longest edge)."""
    longest = max(1, int(round(longest * SS)))
    probe = Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_path=str(path), width=256)))).convert("RGBA")
    w, h = probe.size
    if w >= h:
        pw, ph = longest, max(1, round(longest * h / w))
    else:
        pw, ph = max(1, round(longest * w / h)), longest
    return Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_path=str(path), width=pw, height=ph)))).convert("RGBA")


def enemy_img(eid):
    r = ENEMIES[eid]
    rel = r["texture_path"].replace("res://assets/art/", "")
    return svg_img(SVG / rel, float(r["sprite_target_px"])), float(r["sprite_target_px"])


def shadow_img():
    for r in csv.DictReader(open(ROOT / "SourceArt" / "png" / "atlas" / "field.csv", encoding="utf-8")):
        if r["id"] == "ground_shadow":
            page = Image.open(ROOT / "SourceArt" / "png" / "atlas" / ("field_%s.png" % r["page"])).convert("RGBA")
            x, y, w, h = int(r["x"]), int(r["y"]), int(r["w"]), int(r["h"])
            return page.crop((x, y, x + w, y + h))
    raise SystemExit("no ground_shadow in the atlas: run py tools/pack_atlas.py")


SHADOW = shadow_img()


def font(px, black=False):
    name = "MPLUSRounded1c-Black.ttf" if black else "MPLUSRounded1c-ExtraBold.ttf"
    return ImageFont.truetype(str(ROOT / "SourceArt" / "fonts" / name), int(px * SS))


# --- drawing on a supersampled layer --------------------------------------------------------------------------------
class Cell:
    """One field view, w x h field px, drawn at SS x."""

    def __init__(self, w, h, tier):
        self.w, self.h = w, h
        self.img = Image.new("RGBA", (w * SS, h * SS))
        tile = Image.open(SVG / "backdrops" / ("backdrop_tier_%d.png" % tier)).convert("RGBA")
        t = int(512 * TILE_SCALE * SS)
        tile = tile.resize((t, t), Image.LANCZOS)
        for y in range(0, h * SS, t):
            for x in range(0, w * SS, t):
                self.img.paste(tile, (x, y))

    def over(self, fn):
        layer = Image.new("RGBA", self.img.size)
        fn(ImageDraw.Draw(layer))
        self.img = Image.alpha_composite(self.img, layer)

    def sprite(self, im, cx, cy, flip=False):
        if flip:
            im = im.transpose(Image.FLIP_LEFT_RIGHT)
        self.img.alpha_composite(im, (int(cx * SS - im.width / 2), int(cy * SS - im.height / 2)))

    def creature(self, im, px, cx, cy, flip=False):
        sw = px * SHADOW_WIDTH * SS
        sh = sw * SHADOW.height / SHADOW.width
        s = SHADOW.resize((max(1, int(sw)), max(1, int(sh))), Image.LANCZOS)
        a = s.getchannel("A").point(lambda v: int(v * SHADOW_ALPHA))
        s.putalpha(a)
        self.img.alpha_composite(s, (int(cx * SS - s.width / 2), int((cy + px * SHADOW_FEET) * SS - s.height / 2)))
        self.sprite(im, cx, cy, flip)

    def done(self):
        return self.img.resize((self.w, self.h), Image.LANCZOS)

    def zoomed(self, cx, cy, w, h):
        """The w x h field px around (cx, cy) at 2x: the supersampled pixels themselves."""
        return self.img.crop((int((cx - w / 2) * SS), int((cy - h / 2) * SS), int((cx + w / 2) * SS), int((cy + h / 2) * SS)))


def P(x, y):
    return (x * SS, y * SS)


def arc_pts(cx, cy, r, a0, a1, n):
    return [P(cx + math.cos(a0 + (a1 - a0) * i / n) * r, cy + math.sin(a0 + (a1 - a0) * i / n) * r) for i in range(n + 1)]


def poly(d, pts, col, w):
    d.line(pts, fill=col, width=max(1, int(round(w * SS))), joint="curve")
    r = w * SS / 2
    for p in (pts[0], pts[-1]):
        d.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill=col)


def dashes(cx, cy, r, dash, gap):
    n = max(1, int(round(2 * math.pi * r / (dash + gap))))
    step = 2 * math.pi / n
    span = step * dash / (dash + gap)
    return [(i * step, i * step + span) for i in range(n)]


# --- the aura options -----------------------------------------------------------------------------------------------
def aura_today(c, x, y, r):
    c.over(lambda d: d.ellipse([*P(x - r, y - r), *P(x + r, y + r)], fill=rgba(AURA_GOLD, 0.14)))
    c.over(lambda d: poly(d, arc_pts(x, y, r, 0, 2 * math.pi, 96), rgba(AURA_GOLD, 0.72), 2.5))


def aura_concept(c, x, y, r):
    pink = "#FF6FA8"
    c.over(lambda d: d.ellipse([*P(x - r, y - r), *P(x + r, y + r)], fill=rgba(pink, 0.13)))

    def ring(d):
        for a0, a1 in dashes(x, y, r, 16, 12):
            poly(d, arc_pts(x, y, r, a0, a1, 6), rgba(pink, 1.0), 5)
    c.over(ring)


def inked_dashes(colour, w=4.0, edge=2.0, dash=18, gap=12):
    def draw(c, x, y, r):
        segs = dashes(x, y, r, dash, gap)
        c.over(lambda d: [poly(d, arc_pts(x, y, r, a0, a1, 6), rgba(INK, 0.85), w + edge * 2) for a0, a1 in segs])
        c.over(lambda d: [poly(d, arc_pts(x, y, r, a0, a1, 6), rgba(colour, 1.0), w) for a0, a1 in segs])
    return draw


def aura_lift(c, x, y, r):
    c.over(lambda d: d.ellipse([*P(x - r, y - r), *P(x + r, y + r)], fill=rgba(FX.AURA_LIFT, FX.AURA_LIFT_ALPHA)))
    pts = arc_pts(x, y, r, 0, 2 * math.pi, 96)
    c.over(lambda d: poly(d, pts, rgba(INK, FX.AURA_INK_ALPHA), FX.AURA_RING_WIDTH + FX.AURA_INK_EDGE * 2))
    c.over(lambda d: poly(d, pts, rgba(FX.AURA_RING, 1.0), FX.AURA_RING_WIDTH))


AURAS = [
    ("A. Today (Godot's gold)", "ring 2.5 px at 72 %, fill 14 %", aura_today),
    ("B. The concept", "pink dashes, fill 13 %", aura_concept),
    ("C. Bone dashes, no fill", "Bone 4 px, Plum ink edge", inked_dashes(PLUM["bone"])),
    ("D. White lift + ring", "fill white 10 %, Bone ring, Plum edge", aura_lift),
    ("E. Crimson dashes, no fill", "Crimson 4 px, Plum ink edge", inked_dashes(PLUM["crimson"])),
]

BIOMES = [  # tier, name, the enemies that ship there (inside the aura, outside it)
    (1, "Backyard (light)", "backyard_mouse", "backyard_moth"),
    (9, "The Neighbour's Kitchen (light)", None, None),
    (5, "Sewers (dark)", "sewer_squirrel", "sewer_flying_mouse"),
    (7, "Portal Zone (dark)", "portal_rat", "portal_goose"),
]


def kitchen_ids():
    ks = [k for k, r in ENEMIES.items() if r["tier"] == "9" and r["is_signature"].lower() != "true" and "gristle" not in k
          and "golden" not in k]
    return ks[0], ks[1]


CAT = svg_img(SVG / "player" / "player_cat_base.svg", CAT_PX)

AW, AH = 400, 380


def aura_cell(tier, inside, outside, fn):
    c = Cell(AW, AH, tier)
    cx, cy = AW / 2, AH / 2 + 8
    fn(c, cx, cy, AURA_R)
    ia, ipx = enemy_img(inside)
    oa, opx = enemy_img(outside)
    c.creature(ia, ipx, cx - 105, cy + 60)
    c.creature(oa, opx, cx + 120, cy - 120, flip=True)
    c.creature(CAT, CAT_PX, cx, cy)
    return c.done()


# --- the strike options ---------------------------------------------------------------------------------------------
PAW_OPEN = svg_img(SVG / "ui" / "ui_cursor_paw_open.svg", FX.STRIKE_PAW_PX)
PAW_CLOSED = svg_img(SVG / "ui" / "ui_cursor_paw_closed.svg", FX.STRIKE_PAW_PX)


def swipe(c, at, angle, dist, t, stroke, stroke_a, width, paw=None, trail=1.0):
    """PawVfx.cpp's Swipe at sweep t; `paw` draws a paw at the arc's leading point, toes along the motion."""
    sweep, alpha = t, 1.0 - t
    R = max(PAW_REACH, 8.0) * REACH_FRAC
    lead = -ARC_SPAN * 0.5 + ARC_SPAN * sweep
    span = ARC_SPAN * 0.55 * trail
    start = lead - span

    def world(lx, ly):
        ca, sa = math.cos(angle), math.sin(angle)
        return (at[0] + lx * ca - ly * sa, at[1] + lx * sa + ly * ca)

    pts, echo = [], []
    for i in range(SWIPE_SEG + 1):
        a = start + span * i / SWIPE_SEG
        pts.append(P(*world(math.cos(a) * R, math.sin(a) * R)))
        b = start - span * 0.30 + span * i / SWIPE_SEG
        echo.append(P(*world(math.cos(b) * R * 0.78, math.sin(b) * R * 0.78)))
    w = width
    c.over(lambda d: poly(d, pts, rgba(INK, 0.85 * alpha), w + SWIPE_EDGE * 2))
    c.over(lambda d: poly(d, pts, rgba(stroke, stroke_a * alpha), w))
    if sweep >= CLAW_AT and dist > 0:
        grow = min(max((sweep - CLAW_AT) / 0.25, 0.0), 1.0)
        al = (1 / math.sqrt(2), -1 / math.sqrt(2))
        ac = (al[1], -al[0])
        cw = CLAW_W * w / SWIPE_W
        for i in range(CLAWS):
            k = CLAW_GAP * (i - (CLAWS - 1) * 0.5)
            ax = dist + ac[0] * k - al[0] * CLAW_LEN * 0.5
            ay = ac[1] * k - al[1] * CLAW_LEN * 0.5
            bx, by = ax + al[0] * CLAW_LEN * grow, ay + al[1] * CLAW_LEN * grow
            A, B = P(*world(ax, ay)), P(*world(bx, by))
            c.over(lambda d, A=A, B=B: poly(d, [A, B], rgba(INK, 0.85 * alpha), cw + SWIPE_EDGE))
            c.over(lambda d, A=A, B=B: poly(d, [A, B], rgba(stroke, stroke_a * alpha), cw))
    c.over(lambda d: poly(d, echo, rgba(stroke, stroke_a * alpha * 0.45), w * 0.55))
    if paw is not None:
        tip = world(math.cos(lead) * R, math.sin(lead) * R)
        # the motion's direction at the lead (increasing angle), in world space
        tx, ty = -math.sin(lead), math.cos(lead)
        ca, sa = math.cos(angle), math.sin(angle)
        wx, wy = tx * ca - ty * sa, tx * sa + ty * ca
        deg = math.degrees(math.atan2(wx, -wy))  # the drawing's toes point up (-y)
        im = paw.rotate(-deg, resample=Image.BICUBIC, expand=True)
        a = im.getchannel("A").point(lambda v: int(v * min(1.0, alpha * FX.STRIKE_PAW_FADE_GAIN)))
        im.putalpha(a)
        c.sprite(im, *tip)


SW, SH = 190, 160  # field px around the strike, shown at 2x
FRAMES = (0.25, 0.5, 0.75)


def strike_cell(t, paw, stroke, stroke_a, width, trail=1.0):
    c = Cell(400, 360, 1)
    cat = (200 + 40, 180 + 10)
    angle = math.radians(200)
    dist = 58.0
    vic = (cat[0] + math.cos(angle) * dist, cat[1] + math.sin(angle) * dist)
    m, mpx = enemy_img("backyard_mouse")
    c.creature(CAT, CAT_PX, *cat)
    c.creature(m, mpx, *vic)
    swipe(c, cat, angle, dist, t, stroke, stroke_a, width, paw=paw, trail=trail)
    return c.zoomed(cat[0] - 45, cat[1] - 20, SW, SH)


STRIKES = [
    ("S1. Today: the arc and three claws", "no paw drawn (vfx_swipe.gd)", None, 1.0),
    ("S2. The open paw rides the arc", "ui_cursor_paw_open at the arc's tip", PAW_OPEN, FX.STRIKE_ARC_TRAIL),
    ("S3. The clutching paw rides the arc", "ui_cursor_paw_closed (claws out) at the tip", PAW_CLOSED, FX.STRIKE_ARC_TRAIL),
]
CRITS = [
    ("crit today: pink", CRIT_COL, CRIT_A),
    ("crit in Butter", FX.CRIT, 1.0),
    ("crit in Crimson", PLUM["crimson"], 1.0),
]


# --- the sheet ------------------------------------------------------------------------------------------------------
def main():
    pad, gap, label_w = 40, 16, 0
    k1, k2 = kitchen_ids()
    biomes = [(t, n, a or k1, b or k2) for t, n, a, b in BIOMES]

    aura_w = len(AURAS) * AW + (len(AURAS) - 1) * gap
    strike_cols = len(FRAMES) + len(CRITS)
    strike_w = strike_cols * SW * 2 + (strike_cols - 1) * gap
    W = pad * 2 + max(aura_w, strike_w)
    head_h, colhead_h, rowhead_h = 150, 70, 40
    aura_h = colhead_h + len(biomes) * (rowhead_h + AH + gap)
    strike_h = 110 + colhead_h + len(STRIKES) * (rowhead_h + SH * 2 + gap)
    cur_h = 300
    H = pad + head_h + aura_h + strike_h + cur_h + pad
    sheet = Image.new("RGB", (W * SS, H * SS), rgb(PLUM["base"]))
    d = ImageDraw.Draw(sheet)

    def text(x, y, s, px, col, black=False):
        d.text((x * SS, y * SS), s, font=font(px, black), fill=rgb(col))

    def paste2(im, x, y):
        """A crop of a supersampled cell (SS px per field px), shown at 2x: 2 sheet px per field px."""
        big = im.resize((im.width * 2, im.height * 2), Image.LANCZOS)
        sheet.paste(big.convert("RGB"), (x * SS, y * SS))

    def paste(im, x, y):
        big = im.resize((im.width * SS, im.height * SS), Image.NEAREST)
        sheet.paste(big.convert("RGB"), (x * SS, y * SS))

    y = pad
    text(pad, y, "D3: the aura and the paw strike", 44, PLUM["butter"], black=True)
    text(pad, y + 62, "At the game's scale (1080p): the real floors, cat (175 px), enemies and shadows. Aura radius %d px "
         "(mid-run; %d at the start, x3 at the upgrade's cap)." % (AURA_R, BASE_AURA), 22, PLUM["lilac"])
    text(pad, y + 96, "Pick one aura (A-E), one strike (S1-S3) and one crit colour. Recommended: aura D, strike S3, crit Butter.",
         22, PLUM["bone"])
    y += head_h

    # aura
    text(pad, y, "THE AURA", 30, PLUM["butter"], black=True)
    for i, (name, sub, _) in enumerate(AURAS):
        x = pad + i * (AW + gap)
        text(x, y + 38, name, 20, PLUM["bone"])
    y += colhead_h
    for tier, bname, ein, eout in biomes:
        text(pad, y + 6, bname, 22, PLUM["lilac"])
        y += rowhead_h
        for i, (_, _, fn) in enumerate(AURAS):
            paste(aura_cell(tier, ein, eout, fn), pad + i * (AW + gap), y)
        y += AH + gap

    # strike
    y += 30
    text(pad, y, "THE PAW STRIKE, zoomed 2x (0.24 s; the cat hits enemies within 62 px of its centre)", 30, PLUM["butter"], black=True)
    text(pad, y + 44, "The strike lands on the cat's own face in the game today too: the reach (62 px) is inside the cat (175 px).",
         20, PLUM["lilac"])
    y += 80
    heads = ["%d %% of the sweep" % int(t * 100) for t in FRAMES] + [c[0] for c in CRITS]
    for i, h in enumerate(heads):
        text(pad + i * (SW * 2 + gap), y + 30, h, 20, PLUM["bone"])
    y += colhead_h
    for name, sub, paw, trail in STRIKES:
        text(pad, y + 6, "%s  -  %s" % (name, sub), 22, PLUM["lilac"])
        y += rowhead_h
        for i, t in enumerate(FRAMES):
            paste2(strike_cell(t, paw, SWIPE_COL, 1.0, SWIPE_W, trail), pad + i * (SW * 2 + gap), y)
        for j, (_, col, a) in enumerate(CRITS):
            paste2(strike_cell(0.5, paw, col, a, SWIPE_W * CRIT_SCALE, trail), pad + (len(FRAMES) + j) * (SW * 2 + gap), y)
        y += SH * 2 + gap

    # the menu cursors, for reference (R6 owns them)
    y += 30
    text(pad, y, "THE MENU CURSORS TODAY (menus only; over the field the cat is the cursor). R6 redraws them.", 24,
         PLUM["butter"], black=True)
    y += 50
    for i, name in enumerate(("ui_cursor_paw_open", "ui_cursor_paw_closed")):
        im = svg_img(SVG / "ui" / (name + ".svg"), 64)
        sheet.paste(im, ((pad + i * 220) * SS, (y + 20) * SS), im)
        im2 = svg_img(SVG / "ui" / (name + ".svg"), 32)
        sheet.paste(im2, ((pad + i * 220 + 90) * SS, (y + 36) * SS), im2)
        text(pad + i * 220, y + 110, name.replace("ui_cursor_", ""), 18, PLUM["lilac"])

    out = ROOT / "build" / "review" / "d3_aura_strike.png"
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.resize((W, H), Image.LANCZOS).save(out)
    print("wrote %s (%dx%d)" % (out, W, H))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
