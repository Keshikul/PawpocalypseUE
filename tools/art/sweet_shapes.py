#!/usr/bin/env python3
"""Fused-silhouette creatures for the placeholder set, built to the sweet refs.

Companion to docs/pawpocalypse-enemy-shape-bible.md, which is the design side of
this file. If the two disagree, the bible is the intent and this is the bug.

THE UNION TRICK. Draw every blob EXPANDED by the line width and filled with ink,
then draw every blob again at true size with its own fill. The ink survives only
where no fill covers it, which is exactly the outer boundary of the union. That
is how eight overlapping ellipses become one animal with one continuous outline
and no internal seams.

(The obvious version - stroke each blob, then fill over it - erases the outline
entirely, because a stroke is drawn INSIDE the shape's edge.)

EARS ARE DERIVED FROM THE HEAD, NEVER PLACED BY HAND. Every creature declares a
`head` circle and ears are computed from its centre and radius at an angle. The
first version hand-placed ear triangles in absolute coordinates per creature,
which is exactly why they looked scattered - each pair was eyeballed against a
head it did not know about.

EYES ARE DELIBERATELY UNEQUAL. One eye is drawn larger than the other, because
matched circles read as mechanical. The larger one is on the far side, so the
face reads as slightly turned toward the player.

COST. None at runtime: Godot rasterises SVG once at import into a
CompressedTexture2D, so this is paid at build time and every creature is still
one sprite draw.

    py tools/sweet_shapes.py            # contact sheet -> build/sweet_shapes.png
    py tools/sweet_shapes.py --svg DIR  # write .svg files instead
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

SIZE = 128
INK = PAL.INK
EYE_WHITE = PAL.EYE
STROKE = 3.1
SS = 4


def tint(c, amount, toward="#FBF7EC"):
    r, g, b = (int(c[i:i + 2], 16) for i in (1, 3, 5))
    tr, tg, tb = (int(toward[i:i + 2], 16) for i in (1, 3, 5))
    m = lambda a, t: int(round(a + (t - a) * amount))
    return "#%02X%02X%02X" % (m(r, tr), m(g, tg), m(b, tb))


def shade(c, amount):
    return tint(c, amount, PAL.INK)


def ell(cx, cy, rx, ry, fill, s=True):
    return {"k": "ell", "cx": cx, "cy": cy, "rx": rx, "ry": ry, "fill": fill, "s": s}


def rect(cx, cy, w, h, fill, r=0.0, s=True):
    """A rounded rectangle. ADDED FOR PROPS, 2026-09-11.

    The creature grammar is deliberately all-round, and props built out of it
    came out wrong in a way that was obvious once drawn: a fence became a row of
    arches, paving became clouds, a paper bag became an egg. Architecture has
    flat edges and corners; animals do not. One primitive fixes all of it.
    """
    return {"k": "rect", "cx": cx, "cy": cy, "w": w, "h": h, "r": r,
            "fill": fill, "s": s}


def tri(pts, fill, s=True):
    return {"k": "tri", "pts": pts, "fill": fill, "s": s}


def line(pts, colour, w=2.0):
    return {"k": "line", "pts": pts, "fill": colour, "w": w}


def curve(x1, y1, cx, cy, x2, y2, w=6.0, colour=None):
    pts = []
    for i in range(23):
        t = i / 22.0
        pts.append(((1 - t) ** 2 * x1 + 2 * (1 - t) * t * cx + t * t * x2,
                    (1 - t) ** 2 * y1 + 2 * (1 - t) * t * cy + t * t * y2))
    return line(pts, colour or INK, w)


def bez(p0, p1, p2, n=14):
    """A quadratic curve's points: a tube's spine."""
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]) for t in (i / (n - 1) for i in range(n))]


def _tube_pts(spine, w0, w1):
    """The outline of a tube along `spine`, w0 wide at its start tapering to w1, with round caps."""
    n = len(spine)
    left, right = [], []
    for i, (x, y) in enumerate(spine):
        a, b = spine[max(i - 1, 0)], spine[min(i + 1, n - 1)]
        dx, dy = b[0] - a[0], b[1] - a[1]
        d = max(math.hypot(dx, dy), 1e-6)
        nx, ny = -dy / d, dx / d
        h = (w0 + (w1 - w0) * i / (n - 1)) / 2.0
        left.append((x + nx * h, y + ny * h))
        right.append((x - nx * h, y - ny * h))

    # A cap runs from the edge the outline arrives on (the left, a0 = heading + 90 deg) round the tip to the edge it
    # leaves by. Until 2026-11-10 it started from the other edge, so each end crossed itself into a bow-tie: a "fan"
    # instead of a round end (designer: "the point is flipped so the outline doesn't continue but looks like a fan").
    def cap(c, r, a0):
        return [(c[0] + math.cos(a0 - math.pi * k / 6) * r, c[1] + math.sin(a0 - math.pi * k / 6) * r) for k in range(1, 6)]
    (x0, y0), (x1, y1) = spine[0], spine[1]
    (xa, ya), (xb, yb) = spine[-2], spine[-1]
    end = cap(spine[-1], w1 / 2.0, math.atan2(yb - ya, xb - xa) + math.pi / 2)
    start = cap(spine[0], w0 / 2.0, math.atan2(y0 - y1, x0 - x1) + math.pi / 2)
    return left + end + right[::-1] + start


def tube(spine, w0, w1, fill, s=True):
    """A limb, tail, neck or snout: a tapered tube along `spine` (R5, 2026-11-09: the anatomy pass). A polygon that
    knows its spine, so fuse() grows it by exactly the line width all round instead of from its centroid."""
    return {"k": "tri", "pts": _tube_pts(spine, w0, w1), "fill": fill, "s": s, "spine": (spine, w0, w1)}


def _oval_pts(cx, cy, rx, ry, angle, n=28):
    ca, sa = math.cos(angle), math.sin(angle)
    return [(cx + math.cos(2 * math.pi * i / n) * rx * ca - math.sin(2 * math.pi * i / n) * ry * sa,
             cy + math.cos(2 * math.pi * i / n) * rx * sa + math.sin(2 * math.pi * i / n) * ry * ca) for i in range(n)]


def oval(cx, cy, rx, ry, angle, fill, s=True):
    """A tilted ellipse (a wing, a leaning head), as a polygon that fuse() grows by the line width (R5 anatomy)."""
    return {"k": "tri", "pts": _oval_pts(cx, cy, rx, ry, angle), "fill": fill, "s": s, "oval": (cx, cy, rx, ry, angle)}


def fuse(blobs):
    out = []
    for b in blobs:
        c = dict(b); c["s"] = False; c["fill"] = INK
        if "spine" in c:
            sp, w0, w1 = c["spine"]
            c["pts"] = _tube_pts(sp, w0 + STROKE * 2, w1 + STROKE * 2)
        elif "oval" in c:
            cx, cy, rx, ry, a = c["oval"]
            c["pts"] = _oval_pts(cx, cy, rx + STROKE, ry + STROKE, a)
        elif c["k"] == "ell":
            c["rx"] += STROKE; c["ry"] += STROKE
        elif c["k"] == "rect":
            c["w"] += STROKE * 2; c["h"] += STROKE * 2; c["r"] += STROKE
        else:
            gx = sum(p[0] for p in c["pts"]) / len(c["pts"])
            gy = sum(p[1] for p in c["pts"]) / len(c["pts"])
            grown = []
            for px, py in c["pts"]:
                dx, dy = px - gx, py - gy
                d = max(math.hypot(dx, dy), 0.001)
                grown.append((px + dx / d * STROKE * 1.5, py + dy / d * STROKE * 1.5))
            c["pts"] = grown
        out.append(c)
    for b in blobs:
        c = dict(b); c["s"] = False; out.append(c)
    return out


def ear_shapes(head, kind, size=0.45, spread=42.0, lift=1.0):
    """Ears built from the head circle: `spread` is degrees either side of
    vertical, `size` is a fraction of the head radius."""
    hx, hy, hr = head
    outer, inner = [], []
    for side in (-1, 1):
        a = math.radians(-90 + side * spread)
        bx, by = hx + math.cos(a) * hr * 0.84, hy + math.sin(a) * hr * 0.84
        er = hr * size
        if kind == "round":
            outer.append(ell(bx, by, er, er, None))
            inner.append(ell(bx, by, er * 0.52, er * 0.52, None))
        elif kind in ("cat", "tuft"):
            reach = 2.1 if kind == "cat" else 2.8
            tipx = bx + math.cos(a) * er * reach * lift
            tipy = by + math.sin(a) * er * reach * lift
            px, py = -math.sin(a), math.cos(a)
            outer.append(tri([(bx + px * er, by + py * er), (tipx, tipy),
                              (bx - px * er, by - py * er)], None))
            if kind == "cat":
                inner.append(tri([(bx + px * er * 0.5, by + py * er * 0.5),
                                  ((tipx + bx) / 2.0, (tipy + by) / 2.0),
                                  (bx - px * er * 0.5, by - py * er * 0.5)], None))
        elif kind == "floppy":
            outer.append(ell(bx + side * er * 0.35, by + er * 1.05, er * 0.70,
                             er * 1.45, None))
            inner.append(ell(bx + side * er * 0.35, by + er * 1.15, er * 0.36,
                             er * 0.86, None))
    return outer, inner


# --- the faces (R5, the restyle; docs/WORKLIST-RESTYLE-2026-11-07.md §4 "Faces") ------------------------------------
# Designer, 2026-11-09 (review batch 1): the cat keeps its face ("Keep today's cats looks"); the enemies panic ("Panic for
# enemies but without the drop"): worried brows, tiny pupils, no shine. Bosses keep "today" (their S.eyes calls say so)
# until their own pass. The cat's face is the player cat's, stage 1's and the kitten's (spec "face_kind": "cat").
PREY_FACE = "panic"


def eyes(cx, cy, spread, r, asym=1.22, pupil=0.24, face=None):
    face = face or PREY_FACE
    out = []
    for side, scale in ((-1, 1.0), (1, asym)):
        er = r * scale
        ex = cx + side * spread
        if face == "panic":
            er *= 1.08
            out.append(ell(ex, cy, er, er, EYE_WHITE))
            out.append(ell(ex, cy + er * 0.08, er * 0.13, er * 0.13, INK, s=False))
            # Worried brows: the inner end raised.
            out.append(line([(ex - side * er * 0.25, cy - er * 1.50), (ex + side * er * 0.80, cy - er * 1.16)], INK, 2.8))
            continue
        out.append(ell(ex, cy, er, er, EYE_WHITE))
        out.append(ell(ex, cy + er * 0.06, er * pupil, er * pupil, INK, s=False))
        out.append(ell(ex + er * 0.30, cy - er * 0.36, er * 0.15, er * 0.15,
                       EYE_WHITE, s=False))
    return out


def whiskers(cx, cy, length):
    out = []
    for side in (-1, 1):
        for dy in (-4.0, 0.0, 4.0):
            out.append(line([(cx + side * 7, cy + dy * 0.4),
                             (cx + side * length, cy + dy)], INK, 1.5))
    return out


def creature(spec):
    fur = spec["fur"]
    mark = spec.get("mark", shade(fur, 0.30))
    belly_c = spec.get("belly_colour", tint(fur, 0.60))
    hx, hy, hr = spec["head"]
    parts = []

    for group in spec.get("behind", []):
        parts += fuse([ell(b[0], b[1], b[2], b[3], fur) for b in group])

    blobs = [ell(b[0], b[1], b[2], b[3], fur) for b in spec["blobs"]]
    blobs.append(ell(hx, hy, hr, hr, fur))
    if spec.get("snout"):
        blobs.append(tri(spec["snout"], fur))
    blobs += [ell(f[0], f[1], f[2], f[3], fur) for f in spec.get("feet", [])]

    inner = []
    if spec.get("ears"):
        outer, inner = ear_shapes(spec["head"], spec["ears"][0], *spec["ears"][1:])
        for o in outer:
            o["fill"] = fur
            blobs.append(o)
    parts += fuse(blobs)

    if spec.get("belly"):
        b = spec["belly"]
        parts.append(ell(b[0], b[1], b[2], b[3], belly_c, s=False))
    for m in spec.get("marks", []):
        parts.append(ell(m[0], m[1], m[2], m[3], mark, s=False))
    for i in inner:
        i["fill"] = tint(mark, 0.40); i["s"] = False
        parts.append(i)

    for tx, ty in spec.get("toes", []):
        parts.append(line([(tx, ty), (tx, ty + 5)], INK, 1.4))
        parts.append(line([(tx + 4.5, ty), (tx + 4.5, ty + 5)], INK, 1.4))
    for a in spec.get("antennae", []):
        parts.append(curve(a[0], a[1], a[2], a[3], a[4], a[5], w=1.8))
        parts.append(ell(a[4], a[5], 2.2, 2.2, mark, s=False))

    fx, fy = spec.get("face", (hx, hy))
    kind = "today" if spec.get("face_kind") == "cat" else spec.get("face_style", PREY_FACE)
    if spec.get("whiskers", False):
        parts += whiskers(fx, fy + hr * 0.55, spec.get("whisker_len", 30))
    parts += eyes(fx, fy - hr * 0.06, spec.get("eye_spread", hr * 0.48),
                  spec.get("eye_r", hr * 0.46), spec.get("eye_asym", 1.22),
                  spec.get("pupil", 0.24), face=kind)

    nose = spec.get("nose", (fx, fy + hr * 0.55))
    parts.append(ell(nose[0], nose[1], 3.2, 2.5, shade(fur, 0.55), s=False))
    parts.append(curve(nose[0] - 6, nose[1] + 3, nose[0], nose[1] + 7,
                       nose[0] + 6, nose[1] + 3, w=1.6))
    if spec.get("tongue"):
        parts.append(ell(nose[0] + 1, nose[1] + 8, 3.0, 2.4, PAL.CAT_NOSE))
    return parts


def to_svg(parts, size=SIZE, px=None):
    """`size` is the DRAWING space; `px` is the pixel size written into the
    width/height attributes. They are separate because the shipping assets come
    in three on-disk sizes (64, 128, 256) while the grammar only ever draws in
    one 128-unit space. Emitting viewBox="0 0 128 128" with width="256" renders
    the same art at 256px, which keeps every asset's existing pixel dimensions
    without any of the drawing code having to know about them."""
    px = size if px is None else px
    o = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d">'
         % (size, size, px, px),
         '<g stroke-width="%.1f" stroke-linejoin="round" stroke-linecap="round">' % STROKE]
    for p in parts:
        st = 'stroke="%s"' % INK if p.get("s", True) else 'stroke="none"'
        if p["k"] == "ell":
            o.append('<ellipse cx="%.1f" cy="%.1f" rx="%.1f" ry="%.1f" fill="%s" %s/>'
                     % (p["cx"], p["cy"], p["rx"], p["ry"], p["fill"], st))
        elif p["k"] == "rect":
            o.append('<rect x="%.1f" y="%.1f" width="%.1f" height="%.1f" rx="%.1f" fill="%s" %s/>'
                     % (p["cx"] - p["w"] / 2, p["cy"] - p["h"] / 2, p["w"], p["h"],
                        p["r"], p["fill"], st))
        elif p["k"] == "tri":
            o.append('<polygon points="%s" fill="%s" %s/>'
                     % (" ".join("%.1f,%.1f" % xy for xy in p["pts"]), p["fill"], st))
        else:
            o.append('<polyline points="%s" fill="none" stroke="%s" stroke-width="%.1f"/>'
                     % (" ".join("%.1f,%.1f" % xy for xy in p["pts"]), p["fill"], p["w"]))
    return "\n".join(o + ["</g>", "</svg>"])


def to_svg_wh(parts, vw, vh, pw=None, ph=None):
    """A NON-SQUARE canvas, for UI.

    Everything else this grammar draws is a character in a square box. UI is
    not: a button is 256x64, a bar is 64x16, a separator is 64x8, and those
    ratios are fixed by the Theme's 9-patch margins rather than by taste. So the
    viewBox and the pixel size are each a pair here.
    """
    pw = vw if pw is None else pw
    ph = vh if ph is None else ph
    body = to_svg(parts, vw, px=pw)
    head = ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" '
            'width="%d" height="%d">' % (vw, vh, pw, ph))
    first_close = body.index(">") + 1
    return head + body[first_close:]


def to_image_wh(parts, vw, vh):
    im = Image.new("RGBA", (vw * SS, vh * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    _paint(d, parts)
    return im.resize((vw, vh), Image.LANCZOS)


def _paint(d, parts):
    for p in parts:
        outline = INK if p.get("s", True) else None
        w = max(int(STROKE * SS), 1)
        if p["k"] == "ell":
            d.ellipse([(p["cx"] - p["rx"]) * SS, (p["cy"] - p["ry"]) * SS,
                       (p["cx"] + p["rx"]) * SS, (p["cy"] + p["ry"]) * SS],
                      fill=p["fill"], outline=outline, width=w)
        elif p["k"] == "rect":
            d.rounded_rectangle([(p["cx"] - p["w"] / 2) * SS, (p["cy"] - p["h"] / 2) * SS,
                                 (p["cx"] + p["w"] / 2) * SS, (p["cy"] + p["h"] / 2) * SS],
                                radius=max(p["r"] * SS, 0), fill=p["fill"],
                                outline=outline, width=w)
        elif p["k"] == "tri":
            d.polygon([(x * SS, y * SS) for x, y in p["pts"]], fill=p["fill"],
                      outline=outline, width=w)
        else:
            d.line([(x * SS, y * SS) for x, y in p["pts"]], fill=p["fill"],
                   width=max(int(p["w"] * SS), 1), joint="curve")

def to_image(parts, size=SIZE):
    im = Image.new("RGBA", (size * SS, size * SS), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    _paint(d, parts)
    return im.resize((size, size), Image.LANCZOS)


def CAST():
    E = PAL.ENEMIES
    cat = PAL.CAT_BODY
    return {
        "player_cat": {
            "fur": cat, "mark": shade(cat, 0.42),
            "head": (48, 56, 30),
            "blobs": [(72, 86, 43, 36), (66, 98, 41, 24), (94, 80, 30, 31)],
            "ears": ("cat", 0.44, 40, 1.0),
            "feet": [(46, 116, 8, 6), (64, 118, 8, 6), (90, 117, 8, 6), (106, 115, 8, 6)],
            "toes": [(42, 113), (60, 115)],
            "belly": (68, 98, 33, 23),
            "marks": [(106, 64, 5, 11), (110, 78, 5, 12), (106, 92, 5, 11)],
            "behind": [[(116, 94, 9, 8), (123, 78, 8, 12), (116, 62, 9, 8)]],
            "whiskers": True, "whisker_len": 32, "tongue": True, "eye_r": 14, "face_kind": "cat",
        },
        "kitten": {
            "fur": PAL.PLAYER["kitten_body"], "mark": shade(PAL.PLAYER["kitten_accent"], 0.3),
            "head": (54, 70, 23),
            "blobs": [(72, 94, 31, 26), (88, 90, 22, 23)],
            "ears": ("cat", 0.46, 40, 1.0),
            "feet": [(58, 116, 6, 5), (74, 117, 6, 5), (90, 115, 6, 5)],
            "belly": (70, 100, 23, 15),
            "behind": [[(102, 100, 7, 6), (109, 88, 6, 9)]],
            "whiskers": True, "whisker_len": 22, "eye_r": 11, "face_kind": "cat",
        },
        "mouse": {
            "fur": E["mouse"][0], "mark": E["mouse"][1],
            "head": (48, 68, 25),
            "blobs": [(74, 92, 36, 28), (92, 88, 24, 24)],
            "ears": ("round", 0.86, 46, 1.0),
            "feet": [(56, 116, 7, 5), (76, 117, 7, 5), (96, 115, 7, 5)],
            "belly": (72, 100, 27, 17),
            "behind": [[(112, 100, 8, 7), (124, 88, 7, 9)]],
            "whiskers": True, "whisker_len": 30, "eye_r": 11,
        },
        # --- batch 2, 2026-09-11 ---------------------------------------
        # The ghost keeps mouse geometry exactly and removes the FEET. A mouse
        # that does not touch the ground is the cheapest possible "this one is
        # not alive", and it costs no colour, which the shape bible requires.
        "ghost_mouse": {
            "fur": E["ghost_mouse"][0], "mark": E["ghost_mouse"][1],
            "head": (48, 68, 25),
            "blobs": [(74, 92, 36, 28), (92, 88, 24, 24)],
            "ears": ("round", 0.86, 46, 1.0),
            "feet": [],
            "belly": (72, 100, 27, 17),
            "behind": [[(112, 100, 8, 7), (124, 88, 7, 9)]],
            "whiskers": True, "whisker_len": 30, "eye_r": 11,
        },
        # Same mouse, darkest body in the cast, and a starfield laid over it.
        "void_mouse": {
            "fur": E["void_mouse"][0], "mark": E["void_mouse"][1],
            "head": (48, 68, 25),
            "blobs": [(74, 92, 36, 28), (92, 88, 24, 24)],
            "ears": ("round", 0.86, 46, 1.0),
            "feet": [(56, 116, 7, 5), (76, 117, 7, 5), (96, 115, 7, 5)],
            "belly": (72, 100, 27, 17),
            "behind": [[(112, 100, 8, 7), (124, 88, 7, 9)]],
            "whiskers": True, "whisker_len": 30, "eye_r": 11,
        },
        # Rat mass, and the melt happens in make_sweet_art as added drips rather
        # than in the spec, because a drip is not a body part.
        "slime_rat": {
            "fur": E["slime_rat"][0], "mark": E["slime_rat"][1],
            "head": (42, 78, 22),
            "blobs": [(78, 92, 44, 26), (104, 88, 22, 22)],
            "snout": [(36, 74), (8, 86), (36, 94)],
            "ears": ("round", 0.42, 52, 1.0),
            "feet": [],
            "belly": (78, 99, 32, 15),
            "behind": [[(120, 100, 9, 7), (128, 86, 7, 10)]],
            "whiskers": True, "whisker_len": 26, "eye_r": 9, "nose": (24, 84),
        },
        # A potato with a face. The bible gives the vole NO EARS as its cue -
        # absence is as identifiable as an appendage, and the vole is the proof.
        # Squat, head low, and deliberately the roundest thing in the cast so
        # nothing about it suggests a mouse.
        "vole": {
            "fur": E["vole"][0], "mark": E["vole"][1],
            "head": (52, 84, 24),
            "blobs": [(78, 94, 38, 26), (98, 92, 22, 21)],
            "ears": None,
            "feet": [(60, 116, 8, 5), (84, 117, 8, 5), (104, 115, 7, 5)],
            "belly": (76, 102, 30, 16),
            "behind": [[(116, 102, 7, 6)]],
            "whiskers": True, "whisker_len": 24, "eye_r": 9, "eye_spread": 12,
        },
        "rat": {
            "fur": E["rat"][0], "mark": E["rat"][1],
            "head": (42, 78, 22),
            # The snout is a WEDGE, not a third round blob. The bible's known-gaps
            # list has said since 2026-09-10 that the rat's head merges into its
            # body and costs the pointed-snout cue; a round blob on the front of
            # a round head cannot read as a point, so it is a triangle now.
            "blobs": [(78, 92, 44, 26), (104, 88, 22, 22)],
            "snout": [(36, 74), (8, 86), (36, 94)],
            "ears": ("round", 0.42, 52, 1.0),
            "feet": [(56, 114, 7, 5), (80, 115, 7, 5), (102, 113, 7, 5)],
            "belly": (78, 99, 32, 15),
            "behind": [[(120, 100, 9, 7), (128, 86, 7, 10)]],
            "whiskers": True, "whisker_len": 34, "eye_r": 9, "nose": (24, 84),
        },
        "dog": {
            "fur": E["dog"][0], "mark": E["dog"][1],
            "head": (48, 60, 31),
            "blobs": [(74, 90, 41, 30), (94, 86, 28, 27)],
            # Was 0.60/62 and the bible's known-gaps list called these "too small,
            # they merge into the head - the strongest cue in the design is the
            # weakest in the build". Bigger and pushed further round the skull.
            "ears": ("floppy", 0.86, 72, 1.0),
            "feet": [(50, 116, 9, 6), (70, 118, 9, 6), (94, 116, 9, 6)],
            "belly": (72, 100, 30, 19),
            "marks": [(104, 74, 6, 12)],
            "behind": [[(116, 78, 9, 8), (122, 64, 8, 10)]],
            "eye_r": 14, "tongue": True,
        },
        "goose": {
            "fur": E["goose"][0], "mark": E["goose"][1],
            "head": (44, 26, 15),
            "blobs": [(78, 96, 36, 24), (62, 74, 13, 26), (54, 50, 11, 22)],
            "ears": None,
            "feet": [(68, 118, 8, 5), (90, 118, 8, 5)],
            "belly": (80, 102, 24, 13),
            "marks": [(30, 28, 9, 5)],
            "eye_r": 7, "eye_spread": 7, "nose": (28, 28), "eye_asym": 1.3,
        },
        # THE PIGEON IS THE GOOSE'S OPPOSITE AND THAT IS ITS WHOLE CUE. The
        # goose is neck; the pigeon has NONE - the head sits straight on the
        # body with no join at all. Built by putting the head circle low and
        # overlapping the body so the fused union has no waist.
        "pigeon": {
            "fur": E["pigeon"][0], "mark": E["pigeon"][1],
            "head": (48, 68, 22),
            "blobs": [(74, 90, 38, 28), (96, 84, 22, 22)],
            "ears": None,
            "feet": [(64, 118, 8, 5), (88, 118, 8, 5)],
            "belly": (72, 98, 28, 16),
            "marks": [(52, 86, 8, 7)],
            "eye_r": 8, "eye_spread": 10, "nose": (26, 68), "eye_asym": 1.24,
        },
        # WINGS HELD OUT, never folded - the only bird in the game that is wider
        # than it is tall. The wings are in `behind` so they join the union.
        "seagull": {
            "fur": E["seagull"][0], "mark": E["seagull"][1],
            "head": (54, 58, 19),
            "blobs": [(74, 88, 30, 24)],
            "behind": [[(26, 74, 26, 11), (10, 66, 14, 7)],
                       [(112, 72, 26, 11), (126, 64, 14, 7)]],
            "ears": None,
            "feet": [(66, 114, 7, 5), (86, 114, 7, 5)],
            "belly": (74, 94, 21, 14),
            "eye_r": 8, "eye_spread": 9, "nose": (34, 60), "eye_asym": 1.26,
        },
        # A CREST, and the only one in the game. Beak is added by make_sweet_art
        # because a hook is a curve, not a blob.
        "parrot": {
            "fur": E["parrot"][0], "mark": E["parrot"][1],
            "head": (50, 52, 21),
            "blobs": [(74, 88, 28, 26), (92, 80, 20, 22)],
            "ears": None,
            "feet": [(68, 118, 7, 5), (88, 118, 7, 5)],
            "belly": (72, 94, 20, 16),
            "marks": [(90, 76, 7, 9)],
            "eye_r": 8, "eye_spread": 9, "eye_asym": 1.3,
        },
        # Angular. Folded wings drawn as triangles rather than lobes, so the crow
        # is the one bird whose outline has CORNERS - that is what keeps it apart
        # from the seagull without using colour.
        "crow": {
            "fur": E["crow"][0], "mark": E["crow"][1],
            "head": (48, 56, 20),
            "blobs": [(76, 86, 32, 24)],
            "ears": None,
            "feet": [(66, 116, 7, 5), (88, 116, 7, 5)],
            "belly": (74, 92, 22, 13),
            "eye_r": 7, "eye_spread": 9, "eye_asym": 1.3,
        },
        # FLAT AND WIDE - the lowest silhouette in the game, and the only
        # creature drawn head-on. Six legs, and the antennae are most of its
        # width, which is what stops it reading as a beetle-shaped pebble.
        "cockroach": {
            "fur": E["cockroach"][0], "mark": E["cockroach"][1],
            "head": (64, 74, 17),
            "blobs": [(64, 96, 40, 20), (64, 84, 30, 14)],
            "ears": None,
            "feet": [(28, 108, 9, 4), (46, 114, 9, 4), (82, 114, 9, 4),
                     (100, 108, 9, 4), (30, 92, 8, 4), (98, 92, 8, 4)],
            "belly": (64, 100, 26, 9),
            "antennae": [(56, 62, 30, 40, 14, 30), (72, 62, 98, 40, 114, 30)],
            "eye_r": 8, "eye_spread": 8, "eye_asym": 1.2,
        },
        # LONG SNOUT, longer and heavier than the rat. The snout is a wedge for
        # the same reason the rat's is - a round blob cannot read as a point.
        "crocodile": {
            "fur": E["crocodile"][0], "mark": E["crocodile"][1],
            "head": (54, 84, 21),
            "blobs": [(86, 96, 40, 22), (112, 92, 20, 18)],
            "snout": [(52, 68), (2, 84), (52, 98)],
            "ears": None,
            "feet": [(66, 116, 9, 5), (92, 117, 9, 5), (112, 114, 8, 5)],
            "belly": (86, 102, 30, 12),
            "marks": [(96, 86, 6, 7), (78, 84, 5, 6)],
            "eye_r": 8, "eye_spread": 11, "nose": (12, 82), "eye_asym": 1.28,
        },
        # CHEEK POUCHES: the face is wider than it is tall, and nothing else in
        # the cast is. Achieved with two blobs either side of the head that join
        # the union, not with a wider head circle - a wide circle is just a big
        # head, pouches are a shape.
        "hamster": {
            "fur": E["hamster"][0], "mark": E["hamster"][1],
            "head": (54, 66, 24),
            "blobs": [(74, 94, 34, 26), (30, 70, 15, 13), (78, 70, 15, 13)],
            "ears": ("round", 0.34, 44, 1.0),
            "feet": [(60, 116, 8, 5), (84, 116, 8, 5)],
            "belly": (72, 100, 26, 15),
            "whiskers": True, "whisker_len": 22, "eye_r": 9, "eye_spread": 13,
        },
        # THE ONLY CREATURE WITHOUT A CLEAN EDGE. Its cue is a BROKEN outline,
        # so the fuzz blobs are deliberately many, small and irregular; it also
        # has no legs and floats.
        "dust_bunny": {
            "fur": E["dust_bunny"][0], "mark": E["dust_bunny"][1],
            "head": (62, 78, 26),
            "blobs": [(62, 94, 30, 22), (38, 84, 16, 15), (88, 84, 16, 15),
                      (50, 66, 13, 12), (78, 64, 14, 13), (62, 106, 18, 12)],
            "ears": None, "feet": [],
            "belly": (62, 96, 20, 12),
            "eye_r": 9, "eye_spread": 11, "eye_asym": 1.3,
        },
        # The cat shape, SAGGING. It has to read as the player and be wrong,
        # which is the one deliberate exception to "nothing else uses cat ears".
        "sludge_cat": {
            "fur": E["sludge_cat"][0], "mark": E["sludge_cat"][1],
            "head": (50, 70, 27),
            "blobs": [(76, 96, 40, 26), (96, 92, 26, 22)],
            "ears": ("cat", 0.40, 40, 0.7),
            "feet": [],
            "belly": (74, 102, 30, 15),
            "whiskers": True, "whisker_len": 26, "eye_r": 11, "eye_asym": 1.3,
        },
        # The portal's dog. Same floppy-ear cue as the alley dog - they never
        # share a biome - but leaner and taller, and warm where its biome is not.
        "hound": {
            "fur": E["hound"][0], "mark": E["hound"][1],
            "head": (46, 54, 27),
            "blobs": [(76, 88, 36, 26), (98, 82, 24, 24)],
            "ears": ("floppy", 0.72, 64, 1.0),
            "feet": [(56, 118, 8, 6), (78, 118, 8, 6), (100, 116, 8, 6)],
            "belly": (74, 98, 26, 16),
            "behind": [[(118, 74, 8, 8), (124, 60, 7, 10)]],
            "eye_r": 11, "tongue": True,
        },
        "squirrel": {
            "fur": E["squirrel"][0], "mark": E["squirrel"][1],
            "head": (44, 70, 24),
            "blobs": [(60, 94, 27, 25), (74, 96, 20, 20)],
            "ears": ("tuft", 0.34, 38, 1.0),
            "feet": [(46, 116, 7, 5), (66, 116, 7, 5)],
            "belly": (58, 100, 19, 14),
            "behind": [[(100, 88, 17, 21), (106, 62, 15, 17), (96, 42, 12, 11)]],
            "whiskers": True, "whisker_len": 22, "eye_r": 11,
        },
        "toad": {
            "fur": E["toad"][0], "mark": E["toad"][1],
            "head": (66, 82, 26),
            "blobs": [(66, 100, 44, 22), (44, 84, 22, 19), (88, 84, 22, 19)],
            "ears": None,
            "feet": [(32, 112, 11, 6), (58, 114, 9, 5), (86, 114, 9, 5), (100, 112, 11, 6)],
            "belly": (66, 106, 31, 12),
            "marks": [(94, 96, 5, 6), (42, 98, 5, 6)],
            "eye_r": 13, "eye_spread": 17, "pupil": 0.20, "eye_asym": 1.15,
            "nose": (66, 96),
        },
        "moth": {
            "fur": E["moth"][0], "mark": E["moth"][1],
            "head": (58, 62, 20),
            "blobs": [(62, 88, 20, 24)],
            "behind": [[(30, 78, 24, 28)], [(96, 78, 24, 28)]],
            "ears": None, "feet": [],
            "belly": (62, 92, 13, 16),
            "antennae": [(50, 48, 40, 30, 34, 28), (66, 48, 76, 30, 82, 28)],
            "eye_r": 10, "eye_asym": 1.28,
        },
        "gristle": {
            "fur": E["gristle"][0], "mark": E["gristle"][1],
            "head": (58, 74, 22),
            "blobs": [(70, 94, 34, 24), (44, 90, 20, 18), (92, 88, 18, 20),
                      (78, 76, 16, 14)],
            "ears": None,
            "feet": [(52, 114, 8, 5), (78, 115, 8, 5)],
            "belly": (68, 100, 24, 14),
            "marks": [(90, 84, 6, 7), (46, 86, 5, 6)],
            "eye_r": 10, "eye_spread": 11, "eye_asym": 1.35, "pupil": 0.28,
        },
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--svg", metavar="DIR")
    args = ap.parse_args()
    people = dict((k, creature(v)) for k, v in CAST().items())

    if args.svg:
        out = pathlib.Path(args.svg); out.mkdir(parents=True, exist_ok=True)
        for name, parts in people.items():
            (out / (name + ".svg")).write_text(to_svg(parts), encoding="utf-8")
            print("  %s.svg" % name)
        return 0

    cols, cell, small = 5, 212, 48
    rows = (len(people) + cols - 1) // cols
    ground = PAL.BACKDROPS["tier_1_backyard"]["field"]
    shadow = PAL.BACKDROPS["tier_1_backyard"]["detail"]
    sheet = Image.new("RGB", (cols * cell, rows * cell + 40), ground)
    d = ImageDraw.Draw(sheet)
    for i, (name, parts) in enumerate(people.items()):
        cx, cy = (i % cols) * cell, (i // cols) * cell + 30
        d.ellipse([cx + 36, cy + 124, cx + 132, cy + 140], fill=shadow)
        big = to_image(parts)
        sheet.paste(big, (cx + 14, cy + 6), big)
        tiny = to_image(parts).resize((small, small), Image.LANCZOS)
        sheet.paste(tiny, (cx + 152, cy + 24), tiny)
        d.text((cx + 14, cy + 150), "%s   right: %dpx" % (name, small), fill=PAL.INK)
    d.text((14, 8), "one dominant silhouette cue each - see the enemy shape bible",
           fill=PAL.INK)
    out = pathlib.Path("build/sweet_shapes.png")
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out)
    print("wrote %s (%dx%d)" % (out, sheet.width, sheet.height))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
