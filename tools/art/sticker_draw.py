"""Sticker Box drawing (D18, S5; batch 1 drawn the night of 2026-11-17): the approved Bestiary's primitives in the
generator grammar.

The Bestiary (docs/WORKLIST-STICKER-2026-11-17.md, the approved concept) draws each creature from a handful of primitives:
an ellipse, a circle, a path, a tube (a stroked path with an ink edge), a rounded rect, and a group that moves, scales and
turns its contents while keeping every ink line one width. This module is those primitives, emitting the grammar's parts
(sweet_shapes: "ell", "rect", "tri" for any polygon, "line" for a stroke), so a creature can be transcribed from the
concept line for line and still go through the grammar's tools (the fit, the SVG writer's shape kinds, look_lint).

What differs from sweet_shapes, and why (each is a question for S5's lint, not a fault here):
  * Every outlined part carries its own stroke width ("sw"): the body's ink is 3.6 units, an eye's 3.0, a beak's 2.1
    (the concept's lineW / eyeLW). The grammar sets one width on the group.
  * A drawing is made in the Bestiary's units (feet on y = 0, facing left) and fitted to the 128 box at the end, the ink
    scaling with it (the concept's rule: the line is part of the drawing, so a small sprite has a thin line).
  * The white die-cut rim is NOT in the SVG (look_lint's flat rule: no filters). It is made when rastered (E2): the SVG
    says how wide (data-rim) and tools/pack_atlas.py dilates the alpha by a disc of that radius, filled RIM_COLOUR.
"""

from __future__ import annotations

import contextlib
import math
import re

INK = "#2A2131"
LINE_W = 3.6          # the concept's lineW: every outline
EYE_LW = 3.0          # the concept's eyeLW: eyes, sweat, small hard parts
RIM = 4.3             # the die-cut rim, outside the ink, in drawing units (the concept's feMorphology radius)
RIM_COLOUR = "#FFFDF6"
SIZE = 128
FIT_MARGIN = 3.0

_parts: list = []
_stack = [(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)]   # affine (a, b, c, d, e, f): x' = a x + c y + e, y' = b x + d y + f
_lw = [LINE_W]


def r2(v):
    """The concept's r2(): two decimals, halves up (JS Math.round), so a recorded figure computes what the concept did."""
    return math.floor(v * 100 + 0.5) / 100


def hex2rgb(h):
    h = h.lstrip("#")
    return [int(h[i:i + 2], 16) for i in (0, 2, 4)]


def mix(a, b, t):
    """The concept's mix(): a toward b by t, rounded per channel (JS Math.round: half up)."""
    A, B = hex2rgb(a), hex2rgb(b)
    return "#" + "".join("%02x" % max(0, min(255, math.floor(v + (w - v) * t + 0.5))) for v, w in zip(A, B))


# ---- transforms ----
def _m():
    return _stack[-1]


def _apply(m, x, y):
    a, b, c, d, e, f = m
    return (a * x + c * y + e, b * x + d * y + f)


def _scale(m):
    a, b, c, d, _e, _f = m
    return math.sqrt(abs(a * d - b * c))


def _rotated(m):
    a, b, c, d, _e, _f = m
    return abs(b) > 1e-9 or abs(c) > 1e-9 or abs(a - d) > 1e-9


def _push(tx, ty, s, rot, flip):
    a, b, c, d, e, f = _m()
    cr, sr = math.cos(math.radians(rot)), math.sin(math.radians(rot))
    sx = -s if flip else s
    # local = T(tx, ty) R(rot) S(sx, s)
    la, lb, lc, ld = cr * sx, sr * sx, -sr * s, cr * s
    na, nb = a * la + c * lb, b * la + d * lb
    nc, nd = a * lc + c * ld, b * lc + d * ld
    ne, nf = a * tx + c * ty + e, b * tx + d * ty + f
    _stack.append((na, nb, nc, nd, ne, nf))


@contextlib.contextmanager
def at(tx, ty, s=1.0, rot=0.0, flip=False):
    """The concept's G() as a block: translate, rotate (degrees), scale (mirrored when flip) what is drawn inside."""
    _push(tx, ty, s, rot, flip)
    try:
        yield
    finally:
        _stack.pop()


def G(tx, ty, s, rot, fn, flip=False):
    """The concept's G(): the parts fn() adds, moved as at() moves them."""
    with at(tx, ty, s, rot, flip):
        fn()


# ---- path flattening (M L C Q A Z, absolute, as the concept writes them) ----
_TOK = re.compile(r"[MLCQAZ]|-?\d*\.?\d+(?:e-?\d+)?")


def _arc(x1, y1, rx, ry, phi, large, sweep, x2, y2, n=10):
    """SVG arc to points (the endpoint parameterisation, SVG 1.1 F.6.5)."""
    if rx == 0 or ry == 0:
        return [(x2, y2)]
    cp, sp = math.cos(math.radians(phi)), math.sin(math.radians(phi))
    dx, dy = (x1 - x2) / 2, (y1 - y2) / 2
    x1p, y1p = cp * dx + sp * dy, -sp * dx + cp * dy
    rx, ry = abs(rx), abs(ry)
    lam = x1p ** 2 / rx ** 2 + y1p ** 2 / ry ** 2
    if lam > 1:
        rx, ry = rx * math.sqrt(lam), ry * math.sqrt(lam)
    num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p
    den = rx * rx * y1p * y1p + ry * ry * x1p * x1p
    co = math.sqrt(max(0.0, num / den)) * (-1 if large == sweep else 1)
    cxp, cyp = co * rx * y1p / ry, -co * ry * x1p / rx
    cx, cy = cp * cxp - sp * cyp + (x1 + x2) / 2, sp * cxp + cp * cyp + (y1 + y2) / 2
    def ang(ux, uy, vx, vy):
        a = math.atan2(ux * vy - uy * vx, ux * vx + uy * vy)
        return a
    t1 = ang(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry)
    dt = ang((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry)
    if not sweep and dt > 0:
        dt -= 2 * math.pi
    elif sweep and dt < 0:
        dt += 2 * math.pi
    out = []
    for i in range(1, n + 1):
        t = t1 + dt * i / n
        out.append((cx + rx * math.cos(t) * cp - ry * math.sin(t) * sp, cy + rx * math.cos(t) * sp + ry * math.sin(t) * cp))
    return out


def flatten(d, steps=10):
    """Path data -> (points, closed). One subpath (the concept's paths are single)."""
    toks = _TOK.findall(d)
    pts, i, cmd, closed = [], 0, None, False
    def num():
        nonlocal i
        v = float(toks[i])
        i += 1
        return v
    while i < len(toks):
        t = toks[i]
        if t in "MLCQAZ":
            cmd = t
            i += 1
            if cmd == "Z":
                closed = True
                continue
        if cmd == "M" or cmd == "L":
            pts.append((num(), num()))
        elif cmd == "C":
            p0 = pts[-1]
            p1, p2, p3 = (num(), num()), (num(), num()), (num(), num())
            for k in range(1, steps + 1):
                u = k / steps
                v = 1 - u
                pts.append((v ** 3 * p0[0] + 3 * v * v * u * p1[0] + 3 * v * u * u * p2[0] + u ** 3 * p3[0],
                            v ** 3 * p0[1] + 3 * v * v * u * p1[1] + 3 * v * u * u * p2[1] + u ** 3 * p3[1]))
        elif cmd == "Q":
            p0 = pts[-1]
            p1, p2 = (num(), num()), (num(), num())
            for k in range(1, steps + 1):
                u = k / steps
                v = 1 - u
                pts.append((v * v * p0[0] + 2 * v * u * p1[0] + u * u * p2[0], v * v * p0[1] + 2 * v * u * p1[1] + u * u * p2[1]))
        elif cmd == "A":
            rx, ry, phi, large, sweep, x, y = num(), num(), num(), num(), num(), num(), num()
            x1, y1 = pts[-1]
            pts += _arc(x1, y1, rx, ry, phi, int(large), int(sweep), x, y)
        else:
            raise ValueError("flatten: %r" % t)
    # drop exact repeats (a closing point on the start)
    out = []
    for p in pts:
        if not out or abs(p[0] - out[-1][0]) > 1e-6 or abs(p[1] - out[-1][1]) > 1e-6:
            out.append(p)
    if closed and len(out) > 2 and abs(out[0][0] - out[-1][0]) < 1e-6 and abs(out[0][1] - out[-1][1]) < 1e-6:
        out.pop()
    return out, closed


# ---- the primitives ----
def _sw(o):
    """The concept's lwOf(): the outline's width in absolute units (a group's scale never changes it)."""
    if o and "lw" in o:
        return o["lw"]
    if o and o.get("noLine"):
        return 0.0
    return _lw[-1]


def _shape(part, fill, o):
    ink = fill == "INK"
    part["fill"] = INK if ink else fill
    sw = 0.0 if ink else _sw(o)
    part["s"] = sw > 0
    part["sw"] = sw
    _parts.append(part)


def _oval_pts(cx, cy, rx, ry, rot=0.0, n=36):
    cr, sr = math.cos(math.radians(rot)), math.sin(math.radians(rot))
    return [(cx + rx * math.cos(2 * math.pi * i / n) * cr - ry * math.sin(2 * math.pi * i / n) * sr,
             cy + rx * math.cos(2 * math.pi * i / n) * sr + ry * math.sin(2 * math.pi * i / n) * cr) for i in range(n)]


def E(cx, cy, rx, ry, fill, o=None):
    m = _m()
    if _rotated(m) and abs(rx - ry) > 1e-9:
        _shape({"k": "tri", "pts": [_apply(m, x, y) for x, y in _oval_pts(cx, cy, rx, ry)]}, fill, o)
        return
    s = _scale(m)
    x, y = _apply(m, cx, cy)
    _shape({"k": "ell", "cx": x, "cy": y, "rx": rx * s, "ry": ry * s}, fill, o)


def C(cx, cy, r, fill, o=None):
    E(cx, cy, r, r, fill, o)


def E2(cx, cy, rx, ry, rot, fill, o=None):
    m = _m()
    _shape({"k": "tri", "pts": [_apply(m, x, y) for x, y in _oval_pts(cx, cy, rx, ry, rot)]}, fill, o)


def P(d, fill, o=None):
    pts, _closed = flatten(d)
    m = _m()
    _shape({"k": "tri", "pts": [_apply(m, x, y) for x, y in pts]}, fill, o)


def R(x, y, w, h, rx, fill, o=None):
    m = _m()
    if not _rotated(m):
        s = _scale(m)
        cx, cy = _apply(m, x + w / 2, y + h / 2)
        _shape({"k": "rect", "cx": cx, "cy": cy, "w": w * s, "h": h * s, "r": rx * s}, fill, o)
        return
    pts = []
    for qx, qy, a0 in ((x + w - rx, y + rx, -90), (x + w - rx, y + h - rx, 0), (x + rx, y + h - rx, 90), (x + rx, y + rx, 180)):
        for k in range(7):
            t = math.radians(a0 + 90 * k / 6)
            pts.append((qx + rx * math.cos(t), qy + rx * math.sin(t)))
    _shape({"k": "tri", "pts": [_apply(m, px, py) for px, py in pts]}, fill, o)


def GLASS(x, y, w, h, rx, op, fill, o=None):
    """A see-through rounded rect: the cyber cat's visor, which the concept draws at fill-opacity op over the eyes (the
    one see-through part of the approved cast; look_lint's flat rule allows a Sticker drawing's fill-opacity)."""
    R(x, y, w, h, rx, fill, o)
    _parts[-1]["op"] = op


def T(d, w, fill, o=None):
    """A tube: the stroke in its colour over an ink stroke 2 x the outline wider (the concept's T)."""
    pts, _closed = flatten(d)
    m = _m()
    s = _scale(m)
    ap = [_apply(m, x, y) for x, y in pts]
    ink = fill == "INK"
    lw = 0.0 if ink else _sw(o)
    if lw:
        _parts.append({"k": "line", "pts": ap, "fill": INK, "w": w * s + 2 * lw})
    _parts.append({"k": "line", "pts": ap, "fill": INK if ink else fill, "w": w * s})


def RING(cx, cy, rx, ry, w, col):
    """The concept's RING(): an ellipse drawn as a band of width w in col over an ink band (the halo)."""
    m = _m()
    s = _scale(m)
    pts = [_apply(m, x, y) for x, y in _oval_pts(cx, cy, rx, ry)]
    pts.append(pts[0])
    lw = _lw[-1]
    if lw:
        _parts.append({"k": "line", "pts": pts, "fill": INK, "w": w * s + 2 * lw})
    _parts.append({"k": "line", "pts": pts, "fill": col, "w": w * s})


NO = {"noLine": True}   # the concept's {noLine:true}: a fill with no outline

# ---- the concept's face parts ----
EYE = "#FFFFFF"
# The drop shape: the slime rat's melting blobs (no enemy has a tear drop: designer, 2026-11-17).
DROP = "M0 -9 C 3 -4, 6 -1, 6 2.5 C 6 6, 3 8, 0 8 C -3 8, -6 6, -6 2.5 C -6 -1, -3 -4, 0 -9 Z"


def eye(x, y, r, px=0.0, py=0.0):
    """A panicked eye: a white with the eye's ink, the pupil pushed off-centre (the two eyes look different ways)."""
    C(x, y, r, EYE, {"lw": EYE_LW})
    C(r2(x + px * r * 0.42), r2(y + py * r * 0.42), r2(r * 0.34), "INK")


def bossEyes(x1, y1, x2, y2, r):
    """A boss's eyes: sure of itself, pupils forward, brows down (the concept's bossEyes)."""
    C(x1, y1, r, EYE, {"lw": EYE_LW})
    C(x2, y2, r, EYE, {"lw": EYE_LW})
    C(r2(x1 - r * .3), r2(y1 + r * .12), r2(r * .38), "INK")
    C(r2(x2 - r * .3), r2(y2 + r * .12), r2(r * .38), "INK")
    T("M%s %s L%s %s" % (r2(x1 - r * 1.15), r2(y1 - r * 1.2), r2(x1 + r * .75), r2(y1 - r * .5)), 3.8, "INK")
    T("M%s %s L%s %s" % (r2(x2 - r * .75), r2(y2 - r * .5), r2(x2 + r * 1.15), r2(y2 - r * 1.2)), 3.8, "INK")


def star4(x, y, r):
    k = r * 0.2
    return ("M%s %s Q%s %s %s %s Q%s %s %s %s Q%s %s %s %s Q%s %s %s %s Z"
            % (x, y - r, r2(x + k), r2(y - k), x + r, y, r2(x + k), r2(y + k), x, y + r, r2(x - k), r2(y + k), x - r, y,
               r2(x - k), r2(y - k), x, y - r))


def spark(x, y, r, col):
    P(star4(x, y, r), col, {"lw": EYE_LW * 0.7})


def blob(x, y, s, col):
    G(x, y, s, 0, lambda: P(DROP, col))


ICON_LW = 2.6         # the concept's icon line (its icon(): fig(..., {lw: 2.6})), no rim


def draw(fn, lw=LINE_W):
    """Runs a figure function; returns its parts (in the concept's units). lw: the outline a part takes by default (the
    concept's lineW for creatures, ICON_LW for icons)."""
    global _parts
    _parts = []
    _lw.append(lw)
    try:
        fn()
    finally:
        _lw.pop()
    out, _parts = _parts, []
    return out


# ---- the fit to the 128 box, and the SVG ----
def extent(parts, rim=RIM):
    x0 = y0 = 1e9
    x1 = y1 = -1e9
    for p in parts:
        pad = (p["w"] / 2 if p["k"] == "line" else p.get("sw", 0) / 2) + rim
        if p["k"] == "ell":
            xs, ys = (p["cx"] - p["rx"], p["cx"] + p["rx"]), (p["cy"] - p["ry"], p["cy"] + p["ry"])
        elif p["k"] == "rect":
            xs, ys = (p["cx"] - p["w"] / 2, p["cx"] + p["w"] / 2), (p["cy"] - p["h"] / 2, p["cy"] + p["h"] / 2)
        else:
            xs = (min(x for x, _ in p["pts"]), max(x for x, _ in p["pts"]))
            ys = (min(y for _, y in p["pts"]), max(y for _, y in p["pts"]))
        x0, x1 = min(x0, xs[0] - pad), max(x1, xs[1] + pad)
        y0, y1 = min(y0, ys[0] - pad), max(y1, ys[1] + pad)
    return x0, y0, x1, y1


def frame(*part_lists):
    """(k, dx, dy): the one transform that puts every drawing given, and its rim, in the 128 box (ground mode: the lowest
    foot on the bottom margin). Several drawings share it when something is placed on all of them (the cat's stages)."""
    boxes = [extent(p) for p in part_lists]
    x0, y0 = min(b[0] for b in boxes), min(b[1] for b in boxes)
    x1, y1 = max(b[2] for b in boxes), max(b[3] for b in boxes)
    k = (SIZE - 2 * FIT_MARGIN) / max(x1 - x0, y1 - y0)
    return k, SIZE / 2 - k * (x0 + x1) / 2, (SIZE - FIT_MARGIN) - k * y1


def square(*part_lists, rim=RIM, margin=FIT_MARGIN):
    """(k, dx, dy): the drawings and their rims centred in the 128 box (an overlay: a weapon drawn over the cat; an icon,
    with no rim and its own margin)."""
    boxes = [extent(p, rim) for p in part_lists]
    x0, y0 = min(b[0] for b in boxes), min(b[1] for b in boxes)
    x1, y1 = max(b[2] for b in boxes), max(b[3] for b in boxes)
    k = (SIZE - 2 * margin) / max(x1 - x0, y1 - y0)
    return k, SIZE / 2 - k * (x0 + x1) / 2, SIZE / 2 - k * (y0 + y1) / 2


def fit(parts, at=None):
    """(parts in the 128 box, k): the drawing AND its rim fill the box (ground mode: feet on the bottom margin), the ink
    scaled with it; or placed by a given (k, dx, dy) from frame() or square()."""
    k, dx, dy = at or frame(parts)
    out = []
    for p in parts:
        q = dict(p)
        if p["k"] == "ell":
            q.update(cx=p["cx"] * k + dx, cy=p["cy"] * k + dy, rx=p["rx"] * k, ry=p["ry"] * k)
        elif p["k"] == "rect":
            q.update(cx=p["cx"] * k + dx, cy=p["cy"] * k + dy, w=p["w"] * k, h=p["h"] * k, r=p["r"] * k)
        else:
            q["pts"] = [(x * k + dx, y * k + dy) for x, y in p["pts"]]
        if p["k"] == "line":
            q["w"] = p["w"] * k
        else:
            q["sw"] = p.get("sw", 0) * k
        out.append(q)
    return out, k


def to_svg(parts, px=SIZE, k=None, rim=RIM, ink=LINE_W):
    """sweet_shapes.to_svg's shape kinds and format, each outlined shape with its own stroke-width. With k (fit()'s scale)
    the root carries data-look="sticker", data-rim (the die-cut rim's radius, which tools/pack_atlas.py draws when it
    rasters: E2) and data-ink (the widest outline, which tools/art/look_lint.py holds every outline to), in box units."""
    look = ' data-look="sticker" data-rim="%.3f" data-ink="%.3f"' % (rim * k, ink * k) if k else ""
    o = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d"%s>' % (SIZE, SIZE, px, px, look),
         '<g stroke-linejoin="round" stroke-linecap="round">']
    for p in parts:
        st = ('stroke="%s" stroke-width="%.2f"' % (INK, p["sw"])) if p.get("s") else 'stroke="none"'
        if "op" in p:
            st = 'fill-opacity="%s" %s' % (p["op"], st)
        if p["k"] == "ell":
            o.append('<ellipse cx="%.2f" cy="%.2f" rx="%.2f" ry="%.2f" fill="%s" %s/>' % (p["cx"], p["cy"], p["rx"], p["ry"], p["fill"], st))
        elif p["k"] == "rect":
            o.append('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" rx="%.2f" fill="%s" %s/>'
                     % (p["cx"] - p["w"] / 2, p["cy"] - p["h"] / 2, p["w"], p["h"], p["r"], p["fill"], st))
        elif p["k"] == "tri":
            o.append('<polygon points="%s" fill="%s" %s/>' % (" ".join("%.2f,%.2f" % xy for xy in p["pts"]), p["fill"], st))
        else:
            o.append('<polyline points="%s" fill="none" stroke="%s" stroke-width="%.2f"/>'
                     % (" ".join("%.2f,%.2f" % xy for xy in p["pts"]), p["fill"], p["w"]))
    return "\n".join(o + ["</g>", "</svg>"]) + "\n"
