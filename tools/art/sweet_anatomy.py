"""The anatomy pass (restyle R5, 2026-11-09): enemies with bodies, not blobs.

Designer, 2026-11-09: "most enemies look like indistinct blobs. I like the general style of our stuff but it needs a bit
more anatomy and shape." The style stays the grammar's (sweet_shapes: the union trick, one ink, flat fills, big panicked
eyes, chubby); what changes is the body plan. The old grammar drew every animal as a horizontal bean, a head circle and
foot nubs. Here each one is built from its parts:

  * LIMBS, TAILS, NECKS AND SNOUTS ARE TUBES (sweet_shapes.tube): tapered, fused into the silhouette, so a leg is a leg
    from hip to foot and a snout narrows to the nose.
  * A BODY HAS A FRONT AND A BACK: chest and hips are separate masses (a pear, a hunch), not one ellipse.
  * INNER CONTOURS: one or two short ink curves (a thigh, a shoulder, a cheek) give the mass form inside the outline.
    Thinner than the outline (CONTOUR), so the silhouette still reads first.
  * A SPECIES CUE YOU CAN SEE AT 56 PX: the rat's long bare tail and incisors, the mouse's big ears and long hind feet,
    the gristle's brute shoulders and scars, the moth's real wings, the vole's spade paws, the hamster's cheeks.

Every drawing faces left with its feet on y ~118, in the grammar's 128 box (make_sweet_art fits it to the box).

    py tools/art/review_anatomy.py     # the review sheet, today's beside these
"""

from __future__ import annotations

import math
import pathlib
import random
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import palette as PAL       # noqa: E402
import sweet_shapes as S    # noqa: E402

E = PAL.ENEMIES
INK = S.INK
CONTOUR = 2.0                               # an inner contour: thinner than the outline
PINK = PAL.STACKLANDS["coral"][0]           # bare skin: tails, inner ears, noses, toes
PINK_DEEP = PAL.STACKLANDS["coral"][1]
TOOTH = PAL.EYE                             # incisors and claws: the eye-white


def contour(p0, p1, p2, w=CONTOUR):
    return S.line(S.bez(p0, p1, p2, 12), INK, w)


def nose(x, y, r=3.4, colour=PINK_DEEP):
    return S.ell(x, y, r, r * 0.8, colour)



# --- the rig (designer, 2026-11-09: "would it be possible to have wings as a separate layer so they can be animated?
# Just a basic wing flap rotation. Also could you do the same for the legs on other enemies?") ---------------------------
# A drawing marks its moving parts with limb(): each becomes its own layer, drawn behind the body (back) or over it, and
# turned about its pivot by the game (Realm view). The motions:
#   swing  amp x sin(2 pi (hz t + phase)): a leg stepping (a pair half a cycle apart), a spread wing flapping;
#   lift   amp x (1 - cos(2 pi (hz t + phase))) / 2: a folded wing lifting off the back and settling (0 at rest).
# amp is in degrees, clockwise on screen when positive (the drawing faces left; the game mirrors it). At t = 0 every
# layer is at rest, and the static drawing (icons, cards, the lint) is that rest pose: the back layers, the body, the
# front layers (make_sweet_art writes it so, and SourceArt/svg/rigs/<stem>__<layer>.svg beside it).

STEP_HZ = 2.0          # two steps a second: the walkers' legs (the hz loop together in 2.5 s: the review GIF)
STEP_AMP = 14.0
FLAP_HZ = 3.2          # a flier's spread wings
FLAP_AMP = 22.0
LIFT_HZ = 0.8          # a folded wing on a walker: a slow lift and settle
LIFT_AMP = -12.0       # the tip up (the drawing faces left, the wing hinges at its front)


def limb(name, pivot, parts, motion="swing", amp=STEP_AMP, hz=STEP_HZ, phase=0.0, back=True, depth=None):
    """Marks `parts` as the rig layer `name`, turned about `pivot` (a point in the 128 box, where it meets the body).

    `depth`: its draw depth among the drawing's layers (back < 0 < front; the game spaces them 0.12 of a draw place
    apart and has room for three each side). By default each layer has its own, counted out from the body. Layers that
    never touch may share one (a dog's two far legs, a cockroach's six): look_lint's `rig` rule checks they never overlap,
    at rest or at either end of their turn."""
    meta = {"name": name, "pivot": pivot, "motion": motion, "amp": amp, "hz": hz, "phase": phase, "back": back,
            "depth": depth}
    for q in parts:
        q["rig"] = meta
    return parts


def leg(name, pivot, parts, phase=0.0, back=True, amp=STEP_AMP, depth=None):
    return limb(name, pivot, parts, "swing", amp, STEP_HZ, phase, back, depth)


def depths(layers):
    """Each layer's draw depth (rig_layers' order): its own `depth`, or counted out from the body (0)."""
    body = [n for n, _m, _p in layers].index("body")
    out = []
    for k, (_n, meta, _p) in enumerate(layers):
        d = k - body if meta is None or meta.get("depth") is None else meta["depth"]
        if meta is not None and (d == 0 or (d < 0) != meta["back"]):
            raise ValueError("layer %s: depth %s does not match back=%s" % (_n, d, meta["back"]))
        out.append(d)
    return out


def rig_layers(parts):
    """The layers of a rigged drawing, in draw order: [(name, meta or None, parts)], the body's meta None; [] if the
    drawing has no rig."""
    if not any("rig" in q for q in parts):
        return []
    names, metas, groups, body = [], {}, {}, []
    for q in parts:
        r = q.get("rig")
        if r is None:
            body.append(q)
            continue
        if r["name"] not in metas:
            names.append(r["name"]); metas[r["name"]] = r; groups[r["name"]] = []
        groups[r["name"]].append(q)
    back = [(n, metas[n], groups[n]) for n in names if metas[n]["back"]]
    front = [(n, metas[n], groups[n]) for n in names if not metas[n]["back"]]
    return back + [("body", None, body)] + front


def rest_pose(parts):
    """The drawing as the game shows it at rest: the back layers, the body, the front layers."""
    layers = rig_layers(parts)
    return [q for _n, _m, ps in layers for q in ps] if layers else parts


# --- the mouse: big round ears, a pointed snout, long hind feet, a thin bare tail ----------------------------------

def mouse(fur=None, mark=None, rig=True):
    fur = fur or E["mouse"][0]
    mark = mark or E["mouse"][1]
    parts = []
    # The tail first, behind everything, its own outline: thin, bare, curling up.
    parts += S.fuse([S.tube(S.bez((106, 104), (128, 112), (122, 80)), 5.0, 2.4, PINK)])
    # The far ear behind the head.
    parts += S.fuse([S.ell(58, 46, 15, 16, fur)])
    parts.append(S.ell(58, 47, 9, 10, PINK, s=False))
    body = [
        S.ell(88, 92, 27, 24, fur),                                   # hips, the mass
        S.ell(62, 96, 22, 19, fur),                                   # chest
        S.ell(44, 72, 20, 18, fur),                                   # cranium
        S.tube(S.bez((40, 76), (28, 80), (18, 86)), 24, 9, fur),      # snout
    ]
    legs = [                                                          # (rig layer, pivot, blobs, phase)
        ("leg_hind", (92, 106), [S.ell(98, 115, 15, 5.5, fur)], 0.5),                 # the long hind foot
        ("leg_far", (68, 102), [S.tube([(68, 106), (70, 117)], 9, 8, fur), S.ell(71, 118, 6.5, 3.6, fur)], 0.5),
        ("leg_near", (56, 100), [S.tube([(56, 104), (53, 117)], 9, 8, fur), S.ell(51, 118, 6.5, 3.6, fur)], 0.0),
    ]
    if rig:
        for name, pivot, blobs, phase in legs:
            parts += leg(name, pivot, S.fuse(blobs), phase)
    else:
        body += [b for _n, _p, blobs, _ph in legs for b in blobs]
    parts += S.fuse(body)
    parts.append(S.ell(64, 104, 18, 10, S.tint(fur, 0.6), s=False))   # belly
    parts.append(contour((80, 113), (72, 94), (92, 78)))             # the thigh
    parts += S.fuse([S.ell(34, 50, 16, 17, fur)])                    # the near ear, in front of the crown
    parts.append(S.ell(34, 51, 10, 11, PINK, s=False))
    parts += S.whiskers(22, 86, 20)
    parts += S.eyes(34, 70, 9, 10, asym=1.22)
    parts.append(nose(16, 86))
    parts.append(S.curve(20, 92, 25, 95, 30, 92, w=1.6))
    return parts


# --- the rat: hunched, a long wedge snout with incisors, small ears, a thick bare segmented tail ----------------------

def rat(fur=None, mark=None, legs=True):
    fur = fur or E["rat"][0]
    mark = mark or E["rat"][1]
    parts = []
    tail = S.bez((108, 108), (132, 122), (140, 102), 18)
    parts += S.fuse([S.tube(tail, 8.0, 2.8, PINK)])
    for i in (5, 10):                                           # the tail's rings
        (x0, y0), (x1, y1) = tail[i - 1], tail[i + 1]
        nx, ny = -(y1 - y0), x1 - x0
        d = math.hypot(nx, ny) or 1
        h = (8.0 + (2.6 - 8.0) * i / 17) / 2 + 0.6
        x, y = tail[i]
        parts.append(S.line([(x + nx / d * h, y + ny / d * h), (x - nx / d * h, y - ny / d * h)], INK, 1.4))
    parts += S.fuse([S.ell(56, 60, 9, 9, fur)])                      # far ear
    parts.append(S.ell(56, 61, 5, 5, PINK, s=False))
    body = [
        S.ell(88, 88, 30, 27, fur),                                   # the hunch
        S.ell(60, 98, 24, 18, fur),                                   # shoulders, low
        S.ell(42, 86, 17, 15, fur),                                   # skull, carried low
        S.tube(S.bez((36, 88), (22, 92), (8, 98)), 22, 7, fur),       # the long wedge snout
    ]
    if legs:
        parts += leg("leg_hind", (94, 106), S.fuse([S.ell(100, 115, 16, 5.5, fur)]), 0.5)
        parts += leg("leg_far", (66, 104), S.fuse([S.tube([(66, 108), (68, 117)], 9, 8, fur), S.ell(69, 118, 7, 3.6, fur)]), 0.5)
        parts += leg("leg_near", (52, 100), S.fuse([S.tube([(52, 104), (48, 117)], 9, 8, fur), S.ell(46, 118, 7, 3.6, fur)]), 0.0)
    parts += S.fuse(body)
    parts.append(S.ell(64, 107, 20, 8, S.tint(fur, 0.55), s=False))
    parts.append(S.ell(96, 76, 14, 9, mark, s=False))                 # a dark saddle on the hunch
    parts.append(contour((82, 114 if legs else 108), (72, 92), (94, 70)))   # the haunch
    parts.append(contour((48, 100), (54, 92), (64, 90)))             # the shoulder
    parts += S.fuse([S.ell(44, 66, 10, 10, fur)])                    # near ear, small
    parts.append(S.ell(44, 67, 6, 6, PINK, s=False))
    parts.append(S.rect(13, 104, 4.4, 7, TOOTH, r=1.2))               # incisors
    parts += S.whiskers(14, 98, 20)
    parts += S.eyes(33, 80, 8.5, 9.5, asym=1.25)
    parts.append(nose(7, 98, 3.2))
    return parts


def slime_rat():
    """The rat, melting: no legs, drips where the feet were (the old slime rat's idea, on the new rat)."""
    fur, mark = E["slime_rat"]
    parts = rat(fur, mark, legs=False)
    for x, r in ((56, 6.0), (78, 7.5), (98, 5.5)):
        parts += S.fuse([S.ell(x, 110, r, r * 1.5, fur), S.ell(x, 120, r * 0.6, r * 0.8, fur)])
    return parts


def flying_mouse():
    """The mouse with wings: the flier is its own enemy and its own colour (lilac); its wings are its body tinted pale,
    rooted at the shoulder and breaking the outline up and back (the old flier's rule: a wing that stays inside the body
    reads as a placard)."""
    fur, mark = E["flying_mouse"]
    wing = S.tint(fur, 0.55)
    far = S.fuse([S.oval(84, 48, 24, 13, -0.75, wing)])
    near = S.fuse([S.oval(98, 58, 22, 12, -0.45, S.shade(wing, 0.10))])
    near.append(contour((86, 66), (96, 56), (112, 50), 1.6))
    far = limb("wing_far", (68, 63), far, "swing", FLAP_AMP, FLAP_HZ, 0.0, back=True)
    near = limb("wing_near", (80, 66), near, "swing", FLAP_AMP, FLAP_HZ, 0.0, back=False)
    return far + mouse(fur, mark, rig=False) + near             # a flier: its legs hang still


# --- the gristle: a brute. Big shoulders, knuckles on the ground, a small sunk head, scars and a plaster ------------
# Still "meat" in mass (the old gristle's idea) but with a body: it leans on its arms like a gorilla, its back rises
# to a ridge of spikes that climbs with the tier, and its lumps grow with the tier. Seeded by biome, as before.

def gristle(t=0.0, seed="backyard"):
    rng = random.Random("gristle:" + seed)
    fur, mark = E["gristle"]
    g = 0.9 + 0.22 * t
    parts = []
    # the far arm, behind
    parts += leg("arm_far", (46, 92), S.fuse([S.tube(S.bez((46, 92), (40, 104), (38, 114)), 16 * g, 13 * g, fur),
                                             S.ell(38, 116, 10 * g, 5.5, fur)]), 0.5, amp=10.0)
    parts += leg("leg_hind", (103, 104), S.fuse([S.tube([(104, 108), (106, 116)], 12, 11, fur), S.ell(108, 117, 9, 4.5, fur)]), 0.5)
    parts += leg("arm_near", (62, 94), S.fuse([S.tube(S.bez((62, 92), (58, 106), (60, 115)), 18 * g, 15 * g, fur),
                                             S.ell(60, 116, 12 * g, 6, fur)]), 0.0, amp=10.0)   # knuckles down
    lumps = [
        S.ell(78, 82, 34 * g, 28 * g, fur),                           # shoulders and back: the mass is up front
        S.ell(100, 98, 22 * g, 18 * g, fur),                          # small hips
        S.ell(48, 86, 21, 19, fur),                                   # the head, sunk low between the shoulders
    ]
    for _ in range(2 + int(round(3 * t))):                            # lumps on the back
        a = rng.uniform(math.pi * 1.05, math.pi * 1.9)
        lumps.append(S.ell(78 + 30 * g * math.cos(a), 82 + 24 * g * math.sin(a), rng.uniform(8, 13), rng.uniform(7, 10), fur))
    n = 3 + int(round(5 * t))                                         # the ridge: three spikes in the backyard, eight late
    for k in range(n):
        a = math.pi * (1.15 + 0.6 * (k + 0.5) / n)
        bx, by = 80 + 30 * g * math.cos(a), 82 + 25 * g * math.sin(a)
        tip = 9 + 9 * t + rng.uniform(-2, 3)
        lumps.append(S.tri([(bx - 6, by + 3), (bx + math.cos(a) * tip, by + math.sin(a) * tip), (bx + 6, by + 3)], fur))
    parts += S.fuse(lumps)
    parts.append(S.ell(70, 100, 20 * g, 9, S.tint(fur, 0.55), s=False))      # the belly
    for _ in range(2 + int(round(2 * t))):                                    # blotches
        parts.append(S.ell(84 + rng.uniform(-14, 18), 76 + rng.uniform(-10, 12), rng.uniform(4, 8), rng.uniform(4, 6), mark, s=False))
    parts.append(contour((92, 112), (88, 98), (100, 88)))            # the hip
    # scars across the shoulder
    parts.append(S.line([(80, 62), (92, 74)], INK, 2.4))
    parts.append(S.line([(82, 70), (88, 66)], INK, 1.8))
    parts.append(S.line([(86, 74), (92, 70)], INK, 1.8))
    # the face
    parts += S.eyes(42, 84, 8, 7.5, asym=1.3)
    parts.append(nose(30, 94, 3.4))
    parts.append(S.curve(31, 100, 39, 103, 47, 99, w=1.8))
    parts.append(S.tri([(37, 101.5), (39, 106.5), (41.5, 101.8)], TOOTH))   # an underbite fang
    return parts


# --- the moth: front on. A fat fuzzy body, two pairs of real wings with eyespots, feathered antennae, six legs --------

def moth():
    fur, mark = E["moth"]
    parts = []
    for side in (-1, 1):
        w = S.fuse([S.oval(64 + side * 30, 96, 20, 14, side * 0.55, fur)])                 # hindwing, down and out
        w.append(S.ell(64 + side * 34, 99, 6, 5, S.tint(mark, 0.35), s=False))
        w += S.fuse([S.oval(64 + side * 34, 62, 32, 21, -side * 0.45, fur)])               # forewing, up and out
        w.append(S.ell(64 + side * 42, 56, 10, 9, S.tint(fur, 0.55), s=False))             # an eyespot
        w.append(S.ell(64 + side * 42, 56, 5, 4.5, mark, s=False))
        w.append(contour((64 + side * 14, 76), (64 + side * 30, 68), (64 + side * 50, 66), 1.6))   # a vein
        # front on: both wings rise together, so mirrored (the left one turns clockwise to rise)
        parts += limb("wing_l" if side < 0 else "wing_r", (64 + side * 10, 82), w, "swing", -side * 18.0, FLAP_HZ)
    for side in (-1, 1):                                              # six legs, short, from under the thorax
        for y, dx in ((96, 14), (102, 15), (108, 13)):
            parts.append(S.line([(64 + side * 8, y), (64 + side * dx, y + 6), (64 + side * (dx + 2), y + 10)], INK, 2.0))
    body = [S.ell(64, 104, 13, 17, fur),                              # abdomen
            S.ell(64, 84, 16, 14, fur)]                               # thorax
    for k in range(7):                                                # fuzz round the thorax
        a = math.pi * (0.85 + 1.3 * k / 6)
        body.append(S.ell(64 + 16 * math.cos(a), 84 + 9 * math.sin(a), 5, 4.5, fur))
    body.append(S.ell(64, 62, 18, 16, fur))                           # the head, big: the eyes are the style
    parts += S.fuse(body)
    parts.append(S.ell(64, 108, 8, 10, S.tint(fur, 0.55), s=False))
    for y in (100, 108, 115):                                         # the abdomen's bands
        parts.append(S.curve(55, y - 1, 64, y + 3, 73, y - 1, w=1.6))
    for side in (-1, 1):                                              # feathered antennae
        pts = S.bez((64 + side * 7, 48), (64 + side * 12, 32), (64 + side * 26, 26), 10)
        parts.append(S.line(pts, INK, 1.8))
        for (x, y) in pts[3:9]:
            parts.append(S.line([(x, y), (x + side * 3.5, y + 4.5)], INK, 1.3))
    parts += S.eyes(64, 62, 9.5, 9.5, asym=1.25)
    return parts


# --- the vole: a potato with no ears (its cue, kept) that digs: a blunt snout, spade paws, a stub of tail ---------

def vole():
    fur, mark = E["vole"]
    parts = []
    parts += S.fuse([S.tube([(110, 104), (120, 100)], 7, 5, fur)])
    body = [
        S.ell(78, 90, 32, 28, fur),
        S.ell(50, 90, 23, 21, fur),                                   # head, no neck
        S.tube(S.bez((34, 96), (26, 98), (20, 100)), 16, 10, fur),    # a short blunt snout
    ]

    def toes(cx):                                                     # broad digging paws, toes spread
        return [S.line([(cx + k * 3.4, 112), (cx + k * 4.6 - 2, 119)], INK, 1.6) for k in (-1, 0, 1)]
    parts += leg("leg_hind", (94, 108), S.fuse([S.ell(98, 115, 11, 5, fur)]), 0.5)
    parts += leg("arm_far", (64, 104), S.fuse([S.tube([(64, 108), (60, 114)], 11, 10, fur),            # digging arms,
                                             S.oval(58, 116.5, 10, 5, -0.1, fur)]) + toes(58), 0.5)   # spade paws
    parts += leg("arm_near", (50, 102), S.fuse([S.tube([(50, 106), (44, 114)], 11, 10, fur),
                                              S.oval(41, 116, 10, 5, -0.15, fur)]) + toes(41), 0.0)
    parts += S.fuse(body)
    parts.append(S.ell(78, 104, 24, 9, S.tint(fur, 0.55), s=False))
    parts.append(contour((86, 114), (80, 98), (96, 84)))
    parts += S.whiskers(24, 98, 16)
    parts += S.eyes(42, 85, 9.5, 9, asym=1.22)
    parts.append(nose(19, 100, 3.6))
    return parts


# --- the hamster: sits up. Cheek pouches stuffed, paws held to the chest round a seed, tiny ears ---------------------

def hamster():
    fur, mark = E["hamster"]
    parts = []
    body = [
        S.ell(70, 92, 30, 28, fur),                                   # sitting body, a pear
        S.ell(64, 60, 24, 21, fur),                                   # head, up
        S.ell(42, 68, 13, 12, fur), S.ell(86, 68, 13, 12, fur),       # the stuffed cheeks
        S.ell(52, 116, 11, 5, fur), S.ell(86, 116, 11, 5, fur),       # hind feet, flat
        S.ell(46, 42, 7, 7, fur), S.ell(80, 42, 7, 7, fur),           # small ears
        S.tube([(98, 104), (106, 108)], 6, 4, fur),                   # a stub of tail
    ]
    parts += S.fuse(body)
    parts.append(S.ell(46, 42, 4, 4, PINK, s=False))
    parts.append(S.ell(80, 42, 4, 4, PINK, s=False))
    parts.append(S.ell(70, 98, 21, 18, S.tint(fur, 0.6), s=False))   # the bib
    parts.append(S.ell(64, 64, 15, 9, S.tint(fur, 0.6), s=False))    # the muzzle
    parts.append(contour((40, 62), (38, 72), (46, 80)))              # cheek lines
    parts.append(contour((88, 62), (90, 72), (82, 80)))
    parts += S.fuse([S.ell(64, 86, 7, 9, PAL.STACKLANDS["yellow"][1])])      # the seed
    parts += S.fuse([S.ell(55, 86, 6, 5, fur), S.ell(73, 86, 6, 5, fur)])    # the paws on it
    parts += S.eyes(64, 54, 11, 7.5, asym=1.2)
    parts.append(nose(64, 64, 3))
    parts += S.whiskers(64, 66, 22)
    parts.append(S.curve(60, 70, 64, 73, 68, 70, w=1.6))
    return parts


# --- the birds (R5 batch 3). Each keeps its cue: the goose's NECK, the pigeon's NO NECK, the seagull's WINGS OUT, the
# parrot's HOOKED BEAK AND CREST, the crow's CORNERS. What they gain: a teardrop body, a folded wing with feather lines,
# tail feathers, legs with toes, a beak in two parts. -----------------------------------------------------------------

BEAK = PAL.GOLD                                # a yellow beak (the seagull's, the parrot's)


def bird_feet(xs, y0, colour, toes=3, w=3.6):
    """Thin legs from the body to the ground, toes forward."""
    out = []
    for x in xs:
        out.append(S.line([(x, y0), (x - 1, 116)], INK, w + 2.4))
        out.append(S.line([(x, y0), (x - 1, 116)], colour, w))
        for k in range(toes):
            dx = -7 + k * 4.5
            out.append(S.line([(x - 1, 116), (x - 1 + dx, 120)], INK, 2.2))
    return out


def feathers(cx, cy, n, length, angle, spread=4.0):
    """Feather lines on a folded wing: n short ink strokes, parallel, toward the wing's tip."""
    out = []
    ca, sa = math.cos(angle), math.sin(angle)
    for k in range(n):
        x, y = cx + k * spread * -sa, cy + k * spread * ca
        out.append(S.line([(x, y), (x + ca * length, y + sa * length)], INK, 1.6))
    return out


def goose():
    fur, beak = E["goose"]
    parts = []
    for k, x in enumerate((76, 60)):                                  # orange legs and webbed feet, behind
        parts += leg("leg_far" if k == 0 else "leg_near", (x, 100), S.fuse([S.tube([(x, 102), (x - 2, 115)], 5, 4.5, beak),
                     S.tri([(x - 12, 120), (x - 2, 113), (x + 5, 120)], beak)]), 0.5 * (1 - k))
    body = [
        S.oval(76, 90, 34, 21, -0.12, fur),                           # the body, a teardrop
        S.tri([(98, 80), (124, 70), (106, 96)], fur),                 # the tail, up
        S.tube(S.bez((56, 84), (40, 64), (46, 36)), 17, 12, fur),     # THE NECK
        S.oval(42, 28, 14, 12, -0.15, fur),                           # the head
    ]
    parts += S.fuse(body)
    parts.append(S.oval(68, 98, 22, 9, -0.1, S.tint(fur, 0.6), s=False))      # the breast
    wing = S.fuse([S.oval(84, 86, 24, 12, -0.18, S.shade(fur, 0.08))]) + feathers(76, 82, 3, 14, -0.18)   # folded wing
    parts += limb("wing", (62, 89), wing, "lift", LIFT_AMP, LIFT_HZ, back=False)
    parts += S.fuse([S.tube([(32, 28), (14, 32)], 11, 6, beak)])             # the bill
    parts.append(S.line([(30, 31), (17, 33)], INK, 1.4))
    parts += S.eyes(41, 25, 6.5, 7.2, asym=1.2)
    return parts


def pigeon():
    fur, ring = E["pigeon"]
    parts = []
    parts += S.fuse([S.tri([(94, 88), (124, 82), (122, 104)], S.shade(fur, 0.15))])   # the tail fan, behind
    parts += leg("leg_far", (78, 100), bird_feet((78,), 104, PINK), 0.5)
    parts += leg("leg_near", (62, 100), bird_feet((62,), 104, PINK), 0.0)
    body = [
        S.ell(74, 88, 32, 26, fur),                                   # the body
        S.ell(52, 82, 24, 26, fur),                                   # the puffed chest
        S.ell(46, 54, 18, 17, fur),                                   # the head, straight on the chest: NO NECK
    ]
    parts += S.fuse(body)
    parts.append(S.oval(50, 74, 18, 8, 0.25, ring, s=False))          # the sheen on the neck
    parts.append(S.ell(50, 92, 15, 14, S.tint(fur, 0.35), s=False))    # the chest
    # the folded wing, in the body's own grey: a shaded wing was the pigeon's largest colour and too near the mouse and the
    # foreman (look_lint unique); its outline and two bars draw it
    wing = S.fuse([S.oval(82, 84, 26, 16, -0.12, fur)])
    wing.append(S.line([(74, 80), (92, 77)], INK, 2.4))               # two wing bars
    wing.append(S.line([(76, 88), (96, 85)], INK, 2.4))
    parts += limb("wing", (58, 86), wing, "lift", LIFT_AMP, LIFT_HZ, back=False)
    parts += S.fuse([S.tri([(31, 54), (20, 59), (31, 62)], S.shade(fur, 0.5))])     # the beak
    parts.append(S.ell(33, 55, 3.4, 2.4, S.tint(fur, 0.7), s=False))           # the cere
    parts += S.eyes(44, 50, 7, 7.5, asym=1.2)
    return parts


def gull_wing(root, ctrl, tip, fill, tip_fill, side):
    """A gull's wing spread out sideways: broad at the shoulder, an arched leading edge, tapering to a dark tip, with
    three feather lines along the trailing edge."""
    spine = S.bez(root, ctrl, tip, 16)
    out = S.fuse([S.tube(spine, 26, 7, fill)])
    out += S.fuse([S.tube(spine[11:], 13, 6, tip_fill)])
    for k in (4, 7, 10):
        (x0, y0), (x1, y1) = spine[k - 1], spine[k + 1]
        nx, ny = -(y1 - y0), x1 - x0
        d = math.hypot(nx, ny) or 1
        if ny < 0:
            nx, ny = -nx, -ny
        x, y = spine[k]
        out.append(S.line([(x + nx / d * 2, y + ny / d * 2), (x + nx / d * 9 - side * 3, y + ny / d * 9)], INK, 1.6))
    return out


def seagull():
    """WINGS OUT, near front on (the old gull's cue): the wings spread sideways behind the body, each a broad arched
    tube to a dark tip."""
    fur, mark = E["seagull"]
    tip = S.shade(fur, 0.55)
    # the wings flap mirrored (front on): the right one turns anticlockwise to rise
    parts = limb("wing_r", (76, 84), gull_wing((76, 84), (100, 56), (124, 64), S.shade(fur, 0.06), tip, 1),
                 "swing", -FLAP_AMP * 0.8, FLAP_HZ * 0.75)
    parts += limb("wing_l", (52, 84), gull_wing((52, 84), (28, 56), (4, 64), S.shade(fur, 0.06), tip, -1),
                  "swing", FLAP_AMP * 0.8, FLAP_HZ * 0.75)
    body = [
        S.oval(64, 92, 22, 18, 0.0, fur),                             # the body, a short barrel
        S.tri([(56, 104), (64, 120), (72, 104)], fur),                # the tail, down
        S.ell(62, 66, 16, 15, fur),                                   # the head
        S.ell(56, 108, 5, 4, fur), S.ell(70, 108, 5, 4, fur),         # feet tucked
    ]
    parts += S.fuse(body)
    parts.append(S.ell(64, 96, 13, 11, S.tint(fur, 0.55), s=False))
    parts += S.fuse([S.tube([(52, 70), (34, 74)], 9, 5, BEAK)])
    parts.append(S.ell(39, 75, 2.2, 1.8, PAL.STACKLANDS["red"][0], s=False))   # the red spot on the bill
    parts += S.eyes(62, 62, 7, 7.2, asym=1.2)
    return parts


def parrot():
    fur, wing = E["parrot"]
    parts = []
    for k, (x1, y1) in enumerate(((100, 124), (110, 120), (92, 126))):    # long tail feathers
        parts += S.fuse([S.tube(S.bez((76, 98), (86, 112), (x1, y1)), 11, 6, wing if k != 1 else fur)])
    crest = [S.tri([(48, 34), (40, 8), (58, 28)], wing), S.tri([(56, 30), (60, 4), (68, 30)], wing),
             S.tri([(64, 32), (78, 14), (74, 38)], wing)]
    parts += S.fuse(crest)                                            # THE CREST
    body = [
        S.oval(66, 80, 23, 30, 0.18, fur),                            # sitting upright
        S.ell(56, 48, 19, 18, fur),                                   # the head
    ]
    parts += S.fuse(body)
    parts.append(S.oval(58, 88, 12, 18, 0.18, S.tint(fur, 0.55), s=False))
    w = S.fuse([S.oval(76, 82, 13, 26, 0.25, wing)]) + feathers(70, 96, 3, 10, 1.9, 4.5)   # the folded wing, its own green
    parts += limb("wing", (82, 58), w, "lift", LIFT_AMP, LIFT_HZ, back=False)
    parts += leg("leg_far", (72, 104), bird_feet((72,), 106, S.shade(fur, 0.4), toes=2), 0.5, back=False)
    parts += leg("leg_near", (58, 104), bird_feet((58,), 106, S.shade(fur, 0.4), toes=2), 0.0, back=False)
    hook = [(42, 42), (30, 44), (24, 52), (26, 62), (30, 58), (34, 52), (42, 54)]     # THE HOOKED BEAK
    parts += S.fuse([S.tri(hook, BEAK)])
    parts += S.fuse([S.tri([(40, 56), (32, 58), (38, 62)], S.shade(BEAK, 0.35))])  # the lower mandible
    parts += S.eyes(54, 44, 8.5, 8.5, asym=1.2)
    return parts


def crow():
    fur, mark = E["crow"]
    parts = []
    parts += S.fuse([S.tri([(92, 84), (126, 94), (98, 106)], S.shade(fur, 0.15))])   # the wedge tail, a corner
    parts += leg("leg_far", (76, 100), bird_feet((76,), 104, S.shade(fur, 0.6)), 0.5)
    parts += leg("leg_near", (60, 100), bird_feet((60,), 104, S.shade(fur, 0.6)), 0.0)
    body = [
        S.oval(70, 84, 30, 21, 0.08, fur),
        S.tube(S.bez((52, 78), (44, 70), (44, 62)), 22, 20, fur),
        S.ell(44, 58, 17, 16, fur),
        S.tri([(36, 68), (32, 82), (46, 76)], fur),                   # ruffled throat: corners
        S.tri([(44, 74), (42, 88), (54, 80)], fur),
    ]
    parts += S.fuse(body)
    wing = [(54, 74), (100, 70), (116, 94), (80, 100)]                # THE CORNERS: a hard folded wing
    w = S.fuse([S.tri(wing, S.shade(fur, 0.12))]) + feathers(76, 80, 3, 16, 0.3, 5.0)
    parts += limb("wing", (56, 75), w, "lift", LIFT_AMP, LIFT_HZ, back=False)
    parts += S.fuse([S.tube([(32, 58), (8, 64)], 13, 4, S.shade(fur, 0.55))])      # the thick beak
    parts.append(S.line([(30, 61), (12, 64)], INK, 1.4))
    parts += S.eyes(44, 54, 7, 7.5, asym=1.25)
    return parts


# --- R5 batch 4: the dog and the hound, the ant and the cockroach. Each keeps its cue (the dog CHUNKY with FLOPPY EARS
# and its TONGUE out, the hound LEAN AND LONG, the ant THREE SEGMENTS, the cockroach FLAT with long antennae) and gains
# legs from hip to paw, rigged as drawn. ------------------------------------------------------------------------------

def paw_leg(name, hip, knee, foot, w0, w1, fur, phase, back=True, paw=(8.5, 4.2), depth=None):
    """A leg from the hip through the knee to a paw on the ground, its own outlined shape, turned at the hip."""
    spine = S.bez(hip, knee, foot, 12)
    return leg(name, hip, S.fuse([S.tube(spine, w0, w1, fur), S.ell(foot[0] - 2, foot[1] + 1.5, paw[0], paw[1], fur)]), phase,
               back, depth=depth)


# --- the dog: CHUNKY, FLOPPY EARS, TONGUE OUT. A deep chest, a short back, a round rump, a big head ---------------------

def dog(fur=None, mark=None):
    fur = fur or E["dog"][0]
    mark = mark or E["dog"][1]
    parts = []
    parts += S.fuse([S.tube(S.bez((100, 80), (118, 74), (116, 56)), 10, 7, fur)])          # the tail, up
    parts += paw_leg("leg_far_hind", (92, 96), (100, 106), (94, 116), 13, 10, S.shade(fur, 0.08), 0.0, depth=-2)
    parts += paw_leg("leg_far_front", (58, 98), (58, 108), (56, 116), 12, 10, S.shade(fur, 0.08), 0.5, depth=-2)
    parts += S.fuse([S.oval(56, 50, 10, 17, 0.35, mark)])                                    # the far ear
    body = [
        S.ell(62, 88, 25, 21, fur),                                   # the deep chest
        S.ell(92, 86, 21, 19, fur),                                   # the rump
        S.ell(78, 88, 18, 16, fur),                                   # the back between
        S.tube([(48, 66), (58, 82)], 26, 26, fur),                    # the neck
        S.ell(40, 56, 23, 21, fur),                                   # the head, big
        S.tube(S.bez((28, 64), (20, 68), (13, 70)), 22, 16, fur),     # the muzzle, short and blunt
    ]
    parts += paw_leg("leg_near_hind", (98, 96), (106, 106), (100, 116), 14, 11, fur, 0.5, depth=-1)
    parts += paw_leg("leg_near_front", (62, 100), (60, 110), (60, 116), 13, 11, fur, 0.0, depth=-1)
    parts += S.fuse(body)
    parts.append(S.ell(66, 98, 20, 11, S.tint(fur, 0.6), s=False))   # the chest
    parts.append(S.ell(26, 70, 12, 8, S.tint(fur, 0.6), s=False))    # the muzzle, pale
    parts.append(S.ell(96, 80, 9, 7, mark, s=False))                  # a patch on the rump
    parts.append(contour((86, 104), (84, 90), (98, 78)))             # the haunch
    parts += S.fuse([S.tube([(18, 76), (20, 90)], 8, 9, PINK)])      # THE TONGUE, out
    parts.append(S.line([(19, 80), (19.5, 87)], PINK_DEEP, 1.6))
    parts += S.fuse([S.oval(52, 62, 9, 19, 0.3, mark)])              # FLOPPY EAR, the near one, hanging
    parts += S.eyes(36, 52, 9.5, 10, asym=1.22)
    parts.append(nose(11, 69, 4.2, INK))
    parts.append(S.curve(14, 76, 22, 79, 28, 74, w=1.8))
    return parts


# --- the hound (the Portal): LEAN AND LONG. A deep narrow chest, a tucked waist, long legs, a long snout ---------------

def hound():
    fur, mark = E["hound"]
    parts = []
    parts += S.fuse([S.tube(S.bez((104, 64), (124, 70), (124, 90)), 7, 3.5, fur)])         # a long thin tail, down
    parts += paw_leg("leg_far_hind", (98, 74), (112, 94), (100, 116), 11, 6.5, S.shade(fur, 0.1), 0.0, paw=(7, 3.6), depth=-2)
    parts += paw_leg("leg_far_front", (58, 78), (55, 98), (54, 116), 10, 6.5, S.shade(fur, 0.1), 0.5, paw=(7, 3.6), depth=-2)
    parts += S.fuse([S.oval(50, 30, 7, 16, 0.4, mark)])                                     # the far ear
    body = [
        S.oval(62, 68, 20, 17, 0.1, fur),                             # the deep narrow chest
        S.tube(S.bez((70, 70), (84, 74), (98, 66)), 22, 15, fur),     # the tucked waist
        S.ell(100, 66, 13, 12, fur),                                  # the haunch
        S.tube([(48, 46), (56, 62)], 17, 20, fur),                    # the neck
        S.ell(40, 34, 16, 15, fur),                                   # the skull
        S.tube(S.bez((30, 40), (18, 44), (6, 48)), 17, 9, fur),       # the long snout
    ]
    parts += paw_leg("leg_near_hind", (102, 74), (116, 94), (104, 116), 12, 7.5, fur, 0.5, paw=(7.5, 3.8), depth=-1)
    parts += paw_leg("leg_near_front", (62, 80), (62, 98), (60, 116), 11, 7.5, fur, 0.0, paw=(7.5, 3.8), depth=-1)
    parts += S.fuse(body)
    parts.append(S.oval(60, 76, 13, 7, 0.2, S.tint(fur, 0.55), s=False))      # the chest
    for k in range(3):                                                         # ribs: it is hungry
        parts.append(contour((70 + k * 6, 60), (72 + k * 6, 67), (70 + k * 6, 74), 1.6))
    parts.append(contour((96, 74), (94, 64), (106, 58)))             # the haunch
    parts += S.fuse([S.tube([(10, 52), (12, 62)], 6, 7, PINK)])      # the tongue
    parts += S.fuse([S.oval(46, 44, 6, 18, 0.3, mark)])              # the long ear, hanging
    parts += S.eyes(36, 30, 8, 8.5, asym=1.25)
    parts.append(nose(5, 47, 3.6, INK))
    parts.append(S.curve(8, 52, 16, 54, 24, 50, w=1.6))
    return parts


# --- the ant: THREE SEGMENTS. Head, thorax, a big gaster; six jointed legs from the thorax; elbowed antennae, mandibles --

def ant_leg(name, hip, knee, foot, phase, back):
    pts = [hip, knee, foot]
    return leg(name, hip, [S.line(pts, INK, 5.6), S.line(pts, E["ant"][0] if not back else S.shade(E["ant"][0], 0.2), 2.6)],
               phase, back, amp=18.0, depth=-1 if back else 1)                # each side's three share a depth


def ant():
    fur, mark = E["ant"]
    parts = []
    # the far legs behind, the near ones in front; an ant walks on alternating tripods
    for k, (hx, kx, fx) in enumerate(((52, 44, 36), (62, 64, 62), (70, 84, 92))):
        parts += ant_leg("leg_far_%d" % k, (hx + 2, 92), (kx + 3, 98), (fx + 4, 112), 0.5 * ((k + 1) % 2), True)
    body = [
        S.ell(30, 76, 15, 13, fur),                                   # the head
        S.oval(60, 86, 16, 9, -0.15, fur),                            # the thorax
        S.ell(70, 88, 5, 5, fur),                                     # the waist node
        S.oval(96, 82, 24, 18, -0.25, fur),                           # THE GASTER, big
    ]
    parts += S.fuse(body)
    parts.append(S.oval(98, 88, 14, 7, -0.25, S.tint(fur, 0.35), s=False))
    for k in range(3):                                                # the gaster's bands
        parts.append(contour((88 + k * 8, 68 + k * 1.5), (92 + k * 8, 82), (88 + k * 8, 97 - k * 1.5), 1.6))
    for k, (hx, kx, fx) in enumerate(((52, 44, 36), (62, 64, 62), (70, 84, 92))):
        parts += ant_leg("leg_near_%d" % k, (hx, 92), (kx, 100), (fx, 116), 0.5 * (k % 2), False)
    parts.append(S.line([(24, 66), (22, 50), (8, 44)], INK, 2.4))   # elbowed antennae
    parts.append(S.line([(32, 64), (36, 48), (24, 38)], INK, 2.4))
    parts += S.fuse([S.tri([(18, 84), (8, 88), (16, 92)], S.shade(fur, 0.4))])    # mandibles
    parts += S.eyes(28, 74, 7.5, 7.5, asym=1.2)
    return parts


# --- the cockroach: FLAT AND FAST, from above (a roach is its back): a shield over the head, wing cases with a seam, long
# antennae swept forward, six spiny legs splayed out both sides, scuttling ----------------------------------------------

def roach_leg(name, hip, knee, foot, phase):
    fur = E["cockroach"][0]
    pts = [hip, knee, foot]
    out = [S.line(pts, INK, 6.4), S.line(pts, S.shade(fur, 0.15), 3.2)]
    (x0, y0), (x1, y1) = knee, foot
    for t in (0.4, 0.75):                                             # spines on the shin
        x, y = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        out.append(S.line([(x, y), (x + 4, y + (3 if y1 > y0 else -3))], INK, 1.4))
    return leg(name, hip, out, phase, True, amp=16.0, depth=-1)       # the six never touch: one depth


def cockroach():
    fur, mark = E["cockroach"]
    parts = []
    cy = 78
    for side in (-1, 1):                                              # side -1: the far side (up), 1: the near (down)
        for k, (hx, kx, fx, ky, fy) in enumerate(((52, 42, 30, 14, 30), (68, 70, 64, 18, 38), (84, 98, 116, 16, 36))):
            parts += roach_leg("leg_%s_%d" % ("l" if side < 0 else "r", k), (hx, cy + side * 6),
                               (kx, cy + side * ky), (fx, cy + side * fy), 0.5 * ((k + (side > 0)) % 2))
    for side in (-1, 1):                                              # antennae, long, swept forward and out
        parts.append(S.line(S.bez((30, cy + side * 4), (10, cy + side * 10), (2, cy + side * 34), 10), INK, 2.2))
    body = [
        S.oval(76, cy, 40, 22, 0.0, fur),                             # the body, a flat oval
        S.ell(34, cy, 11, 11, fur),                                   # the head, under the shield
    ]
    parts += S.fuse(body)
    parts += S.fuse([S.oval(84, cy, 32, 19, 0.0, S.shade(fur, 0.12))])        # the wing cases
    parts.append(S.line([(54, cy), (116, cy)], INK, 1.8))            # their seam
    parts += S.fuse([S.oval(48, cy, 13, 17, 0.0, mark)])              # THE SHIELD over the head, pale
    parts.append(S.ell(48, cy, 6, 9, S.shade(mark, 0.3), s=False))    # its dark mark
    parts += S.eyes(31, cy, 7.5, 7, asym=1.2)
    return parts


# --- R5 batch 5: the toad, crocodile, bat, imp, dust bunny, sludge cat and robot mouse (the ghost stays today's). Each keeps its cue and
# gains a body plan; what moves is rigged as drawn (legs, wings, arms, ears, a wind-up key). ---------------------------

# --- the toad (the Turf): SQUAT AND WARTY. A wide low body, a big flat head with a wide mouth, eyes on top, a folded hind
# leg you can see, front arms planted ----------------------------------------------------------------------------------

def toad():
    fur, mark = E["toad"]
    parts = []
    parts += paw_leg("arm_far", (54, 96), (50, 106), (48, 116), 11, 9, S.shade(fur, 0.1), 0.5, paw=(8, 3.6), depth=-1)
    body = [
        S.oval(78, 94, 38, 22, -0.05, fur),                           # the wide low body
        S.oval(44, 84, 26, 19, 0.05, fur),                            # the flat head
        S.ell(36, 66, 10, 9, fur), S.ell(54, 64, 10, 9, fur),         # eye bumps on top
        S.oval(100, 100, 18, 13, -0.35, fur),                         # the folded hind leg's thigh
        S.oval(102, 116, 17, 4.5, 0.0, fur),                          # its long foot, flat
    ]
    parts += S.fuse(body)
    parts += paw_leg("arm_near", (46, 98), (40, 108), (38, 116), 12, 10, fur, 0.0, paw=(9, 4), back=False, depth=1)
    parts.append(S.oval(66, 104, 26, 9, 0.0, S.tint(fur, 0.55), s=False))    # the pale belly
    for (x, y, r) in ((74, 80, 4.5), (88, 78, 3.5), (98, 86, 4), (64, 76, 3), (84, 90, 3)):     # warts
        parts.append(S.ell(x, y, r, r * 0.85, mark, s=False))
    parts.append(contour((90, 112), (88, 98), (104, 90)))             # the thigh
    parts.append(S.curve(20, 88, 38, 96, 60, 90, w=2.2))              # the wide mouth
    parts += S.eyes(45, 64, 8.5, 9, asym=1.15)
    return parts


# --- the crocodile (the Sewers): LONG AND LOW, TEETH. A long snout with a row of teeth, scutes down the back, a long
# tail, four short legs splayed ------------------------------------------------------------------------------------------

def crocodile():
    fur, mark = E["crocodile"]
    parts = []
    parts += paw_leg("leg_far_front", (50, 92), (44, 102), (42, 114), 9, 8, S.shade(fur, 0.1), 0.5, paw=(7, 3.4), depth=-2)
    parts += paw_leg("leg_far_hind", (86, 92), (94, 102), (92, 114), 10, 8, S.shade(fur, 0.1), 0.0, paw=(7, 3.4), depth=-2)
    body = [
        S.tube(S.bez((4, 90), (20, 88), (34, 84)), 12, 18, fur),      # THE LONG SNOUT
        S.oval(42, 82, 15, 12, 0.0, fur),                             # the head
        S.oval(70, 86, 30, 13, 0.0, fur),                             # the long body
        S.tube(S.bez((96, 86), (114, 90), (126, 78)), 18, 4, fur),    # the tail
    ]
    ridge = [S.tri([(x - 4, 76), (x, 68), (x + 4, 76)], fur) for x in (56, 66, 76, 86)]
    ridge += [S.tri([(x - 3, 82), (x, 76), (x + 3, 82)], fur) for x in (100, 110)]
    parts += S.fuse(body + ridge)                                     # scutes down the back
    for x in (8, 14, 20, 26):                                         # THE TEETH, over the lip
        parts.append(S.tri([(x - 2.2, 93), (x, 98), (x + 2.2, 93)], TOOTH))
    parts.append(S.line([(4, 93), (34, 91)], INK, 1.6))               # the jaw line
    parts.append(S.oval(66, 94, 24, 5, 0.0, S.tint(fur, 0.5), s=False))       # the pale belly
    for x in (60, 72, 84):                                            # the back's plates
        parts.append(S.line([(x, 80), (x + 2, 90)], INK, 1.4))
    parts += paw_leg("leg_near_front", (46, 94), (40, 104), (36, 116), 10, 9, fur, 0.0, paw=(8, 3.6), depth=-1)
    parts += paw_leg("leg_near_hind", (82, 94), (90, 104), (88, 116), 11, 9, fur, 0.5, paw=(8, 3.6), depth=-1)
    parts.append(nose(5, 87, 2.4, INK))
    parts += S.eyes(42, 72, 7, 7.5, asym=1.2)
    return parts


# --- the bat (the flier): WINGS SPREAD. Front on: membrane wings with finger bones and a scalloped edge, big ears, fangs,
# tiny feet ------------------------------------------------------------------------------------------------------------

def bat_wing(side, fur):
    """One wing, side -1 left, 1 right: a membrane from the shoulder to a tip up and out, its trailing edge scalloped between
    three finger tips, the fingers drawn as bones."""
    c = 64
    sh = (c + side * 12, 70)
    tip = (c + side * 60, 36)
    fingers = [(c + side * 58, 66), (c + side * 44, 76), (c + side * 28, 84)]
    pts = [sh, (c + side * 34, 50), tip]
    prev = tip
    for f in fingers:                                                 # scallops: each a dip between two finger tips
        mid = ((prev[0] + f[0]) / 2 - side * 2, (prev[1] + f[1]) / 2 + 6)
        pts += [mid, f]
        prev = f
    pts += [((prev[0] + sh[0]) / 2, prev[1] + 2), (sh[0], sh[1] + 12)]
    if side > 0:
        pts = pts[::-1]
    out = S.fuse([S.tri(pts, fur)])          # the body's own colour: a paler membrane was its largest colour, too near the mouse
    for f in [tip] + fingers:                                         # the finger bones
        out.append(S.line([sh, f], S.shade(fur, 0.35), 2.0))
    return out


def bat():
    fur, mark = E["bat"]
    parts = []
    parts += limb("wing_l", (52, 72), bat_wing(-1, fur), "swing", FLAP_AMP, FLAP_HZ)
    parts += limb("wing_r", (76, 72), bat_wing(1, fur), "swing", -FLAP_AMP, FLAP_HZ)
    body = [
        S.ell(64, 84, 15, 17, fur),                                   # the body
        S.ell(64, 64, 17, 15, fur),                                   # the head
        S.tri([(50, 58), (44, 32), (60, 52)], fur),                   # BIG EARS
        S.tri([(78, 58), (84, 32), (68, 52)], fur),
        S.ell(58, 102, 4, 4, fur), S.ell(70, 102, 4, 4, fur),         # tiny feet
    ]
    parts += S.fuse(body)
    parts.append(S.tri([(50, 54), (47, 40), (56, 52)], S.tint(PINK, 0.2), s=False))
    parts.append(S.tri([(78, 54), (81, 40), (72, 52)], S.tint(PINK, 0.2), s=False))
    parts.append(S.ell(64, 88, 9, 10, S.tint(fur, 0.45), s=False))    # the furry chest
    parts.append(S.tri([(58, 72), (60, 78), (62, 72)], TOOTH))        # fangs
    parts.append(S.tri([(66, 72), (68, 78), (70, 72)], TOOTH))
    parts.append(nose(64, 68, 2.4, S.shade(fur, 0.5)))
    parts += S.eyes(64, 60, 8, 8, asym=1.2)
    return parts


# --- the imp (the Mythic Realm): A LITTLE DEVIL. Horns, a pointed ear, a small bat wing, a tail with a spade, goat legs.
# The wing is three rounded panels fanned from the shoulder (designer, 2026-11-09: "The imp seems to have problems with a
# wing": the first was one concave polygon, whose grown outline broke and tore as it turned) --------------------------

def imp():
    fur, mark = E["imp"]
    parts = []
    parts += S.fuse([S.tube(S.bez((80, 98), (112, 104), (110, 74)), 6, 3, fur),
                     S.tri([(104, 72), (110, 62), (118, 74), (110, 78)], fur)])   # the tail and its spade
    shoulder = (72, 72)
    panels = [(86, 58, 15, 7, -0.85), (92, 66, 15, 6.5, -0.45), (92, 76, 13, 5.5, -0.05)]
    wing = S.fuse([S.oval(cx, cy, rx, ry, a, mark) for cx, cy, rx, ry, a in panels])
    for cx, cy, rx, ry, a in panels:                                  # the finger bones, shoulder to each panel's tip
        wing.append(S.line([shoulder, (cx + math.cos(a) * rx * 0.8, cy + math.sin(a) * rx * 0.8)], S.shade(mark, 0.4), 1.6))
    parts += limb("wing", shoulder, wing, "swing", -14.0, 2.4, back=True)
    parts += paw_leg("leg_far", (70, 96), (76, 108), (70, 116), 9, 6, S.shade(fur, 0.12), 0.5, paw=(6, 3.2), depth=-2)
    parts += paw_leg("leg_near", (58, 96), (64, 108), (58, 116), 10, 6.5, fur, 0.0, paw=(6.5, 3.4), depth=-1)
    body = [
        S.ell(64, 88, 18, 16, fur),                                   # the body
        S.ell(58, 62, 19, 17, fur),                                   # the head
        S.tri([(42, 54), (40, 36), (52, 48)], mark),                  # HORNS
        S.tri([(64, 48), (72, 32), (72, 50)], mark),
        S.tri([(72, 58), (88, 50), (76, 68)], fur),                   # a pointed ear, at the back of the head
    ]
    parts += S.fuse(body)
    parts.append(S.tri([(75, 59), (84, 54), (77, 64)], S.tint(PINK, 0.3), s=False))
    parts.append(S.ell(62, 92, 10, 9, S.tint(fur, 0.5), s=False))
    parts.append(S.curve(42, 72, 50, 78, 58, 72, w=2.0))              # a grin
    parts.append(S.tri([(46, 74), (48, 79), (50, 74.5)], TOOTH))
    parts += S.eyes(52, 60, 8, 8.5, asym=1.25)
    return parts


# --- the dust bunny (the Litterbox): A BALL OF FLUFF WITH EARS. Fuzz all round, two long ears (one flopped), tiny feet ---

def dust_bunny():
    fur, mark = E["dust_bunny"]
    parts = []
    parts += limb("ear_far", (72, 46), S.fuse([S.oval(80, 26, 7, 20, 0.35, S.shade(fur, 0.06))]), "lift", -10.0, 0.8, 0.5)
    parts += leg("foot_far", (74, 106), S.fuse([S.ell(78, 114, 9, 5, S.shade(fur, 0.1))]), 0.5, depth=-1)
    body = [S.ell(64, 80, 32, 30, fur)]
    for k in range(16):                                               # the fuzz, all round
        a = 2 * math.pi * k / 16
        body.append(S.ell(64 + 32 * math.cos(a), 80 + 30 * math.sin(a), 7, 6.5, fur))
    parts += S.fuse(body)
    for (x, y) in ((78, 62), (86, 80), (74, 98), (50, 96), (90, 96)):            # lint caught in it
        parts.append(S.curve(x - 4, y, x, y - 3, x + 4, y, w=1.4, colour=S.shade(fur, 0.3)))
    parts += leg("foot_near", (52, 106), S.fuse([S.ell(48, 114, 10, 5.5, fur)]), 0.0, back=False, depth=1)
    parts += limb("ear_near", (54, 50), S.fuse([S.tube(S.bez((54, 50), (40, 34), (24, 40)), 13, 10, fur)]),
                  "lift", 10.0, 0.8, 0.0, back=False)                 # the near ear, flopped forward
    parts.append(S.tube(S.bez((50, 48), (40, 38), (30, 42)), 5, 4, S.tint(PINK, 0.3), s=False))
    parts.append(S.ell(56, 92, 14, 8, S.tint(fur, 0.5), s=False))
    parts.append(nose(46, 84, 2.6))
    parts += S.eyes(50, 74, 9, 9.5, asym=1.2)
    return parts


# --- the sludge cat (the Litterbox): A CAT, MELTING. REDONE 2026-11-10 (designer: "Redo sludge cat"; take A approved:
# "The puddle sludge cat is good"). Half cat, half puddle: the head and chest stand up out of a wide slick, the back half
# has already gone; two front legs reach forward and drag it along (they step); the tail curls up out of the far end of
# the puddle; one ear stands, the other is sliding off; drips off the chin; bubbles ------------------------------------

def puddle(cx, y, rx, ry, fur, lumps):
    """A puddle on the ground with a lumpy rim: one oval and the lumps that break its edge."""
    return [S.oval(cx, y, rx, ry, 0.0, fur)] + [S.ell(x, yy, r, r * 0.6, fur) for x, yy, r in lumps]


def sludge_cat():
    fur, mark = E["sludge_cat"]
    parts = []
    parts += paw_leg("leg_far", (46, 90), (32, 98), (24, 110), 10, 9, S.shade(fur, 0.1), 0.5, paw=(7.5, 3.8), depth=-1)
    body = puddle(74, 111, 46, 9, fur, ((34, 113, 9), (58, 117, 11), (96, 117, 12), (116, 112, 8)))
    body += [
        S.oval(76, 98, 26, 13, -0.15, fur),                           # the back half, already sinking into the slick
        S.oval(52, 82, 15, 21, 0.3, fur),                             # the chest, standing up out of it
        S.ell(42, 56, 20, 17, fur),                                   # the head
        S.tri([(26, 48), (22, 26), (40, 42)], fur),                   # the ear that stands
        S.tri([(54, 46), (82, 64), (60, 60)], fur),                   # the ear sliding off the back of the head
        S.ell(80, 67, 3.2, 4.4, fur),                                 # ... and the drip it is losing
        S.ell(34, 72, 5, 6, fur),                                     # the chin, running
        S.tube(S.bez((98, 104), (122, 92), (110, 70)), 9, 7, fur),    # the tail, curling up out of the puddle
        S.ell(109, 77, 3.4, 5, fur),                                  # a drip down the tail
    ]
    parts += S.fuse(body)
    parts += S.fuse([S.ell(34, 81, 2.8, 3.6, fur)])                   # a drop falling off the chin
    parts += S.fuse([S.ell(81, 76, 2.2, 2.8, fur)])                   # and off the ear
    parts.append(S.oval(80, 111, 30, 3.2, 0.0, S.tint(fur, 0.5), s=False))      # the puddle's sheen
    parts.append(S.oval(70, 92, 12, 4, -0.35, S.tint(fur, 0.45), s=False))      # the back's sheen
    for (x, y, r) in ((84, 100, 4), (66, 96, 2.6), (100, 110, 3), (48, 112, 2.4)):   # bubbles in the sludge
        parts.append(S.ell(x, y, r, r, mark, s=False))
    parts.append(S.ell(92, 82, 3.2, 3.2, S.tint(fur, 0.35)))           # one bubble risen out of it, about to pop
    parts += paw_leg("leg_near", (54, 94), (42, 104), (34, 114), 11, 10, fur, 0.0, paw=(8, 4), back=False, depth=1)
    parts.append(S.tri([(28, 46), (25, 32), (36, 42)], S.tint(PINK, 0.3), s=False))   # the standing ear's inside
    parts += S.whiskers(30, 64, 16)
    parts.append(nose(26, 60, 2.8))
    parts.append(S.curve(26, 68, 31, 66, 37, 69, w=1.8))              # a sagging mouth
    parts += S.eyes(40, 53, 9, 9.5, asym=1.25)
    return parts


# --- the robot mouse (the Factory): TIN AND BOXY. A riveted box body, disc ears with bolts, a nose light, wheels for feet,
# a wind-up key in its back that turns, a wire tail with a plug --------------------------------------------------------

def robot_mouse():
    fur, mark = E["robot_mouse"]
    parts = []
    parts += S.fuse([S.tube(S.bez((104, 98), (124, 100), (122, 82)), 4, 4, mark), S.ell(122, 80, 4, 4, mark)])    # the wire tail
    key = [S.rect(96, 56, 5, 14, mark, r=1.5), S.ell(96, 44, 9, 6, mark), S.ell(96, 44, 3.5, 2.5, fur)]
    parts += limb("key", (96, 62), S.fuse(key[:2]) + [key[2]], "swing", 25.0, 1.2, back=True)   # THE WIND-UP KEY
    parts += S.fuse([S.ell(68, 46, 13, 13, mark)])                    # the far ear
    body = [
        S.rect(76, 86, 56, 34, fur, r=10),                            # the box body
        S.rect(40, 74, 34, 28, fur, r=9),                             # the head, a box too
        S.tri([(24, 66), (8, 78), (24, 86)], fur),                    # the snout, a wedge
    ]
    parts += S.fuse(body)
    parts += S.fuse([S.ell(46, 52, 13, 13, mark)])                    # the near ear, a disc
    parts.append(S.ell(46, 52, 4, 4, fur, s=False))                    # its bolt
    for x in (58, 74, 90):                                            # rivets
        parts.append(S.ell(x, 74, 1.8, 1.8, S.shade(fur, 0.35), s=False))
        parts.append(S.ell(x, 98, 1.8, 1.8, S.shade(fur, 0.35), s=False))
    parts.append(S.rect(78, 88, 30, 10, S.tint(fur, 0.5), r=3, s=False))      # a panel
    parts.append(S.ell(8, 78, 3.4, 3.4, PAL.STACKLANDS["red"][0]))                     # the nose light
    for x in (56, 96):                                                # wheels for feet
        parts += S.fuse([S.ell(x, 108, 9, 9, mark)])
        parts.append(S.ell(x, 108, 3, 3, fur, s=False))
    parts += S.eyes(38, 70, 8.5, 9, asym=1.2)
    return parts


# --- R5 bosses (review). Bosses keep their TWO cues (make_sweet_art's boss notes) and the confident face. The Rival
# Champion is a cat (designer: "Keep today's cats looks") and the generic boss is plain on purpose: both stay today's.

B = PAL.BOSSES
FACE = "today"


# --- BOSSES CARRY MORE DETAIL (designer, 2026-11-09: "make bosses more detailed than the rest because they will be larger
# in size"): a boss is drawn at 256 x 0.9 = 230 px, three to four times an enemy, so each gets a second level of drawing
# that an enemy at 64 px could not hold. REDONE 2026-11-10 (designer: "Redo bosses"): the first drafts were enemies with
# extra lines; each boss is now a POSE and a presence, its two cues kept.

GOLD = PAL.GOLD


def scallops(x0, x1, y, n, w=1.4, colour=None, depth=3.0):
    out = []
    step = (x1 - x0) / n
    for k in range(n):
        a = x0 + k * step
        out.append(S.curve(a, y, a + step / 2, y + depth, a + step, y, w=w, colour=colour))
    return out


def brow(x0, y0, x1, y1, w=3.4):
    return S.line([(x0, y0), (x1, y1)], INK, w)


def feather_wing(root, angle, length, width, fill, tiers=3):
    """A raised wing: tiers of long ovals fanned from the root, the outer tier longest, each with a feather line; the tips
    break into points. angle: radians, the wing's direction (screen: 0 right, -pi/2 up)."""
    out = []
    shapes = []
    for k in range(tiers):
        a = angle + (k - (tiers - 1) / 2) * 0.32
        L = length * (0.72 + 0.14 * k)
        cx, cy = root[0] + math.cos(a) * L * 0.5, root[1] + math.sin(a) * L * 0.5
        shapes.append(S.oval(cx, cy, L * 0.5, width * (1 - 0.15 * k), a, fill))
        tip = (root[0] + math.cos(a) * L * 1.02, root[1] + math.sin(a) * L * 1.02)
        shapes.append(S.tri([(tip[0] - math.sin(a) * 4, tip[1] + math.cos(a) * 4), (tip[0] + math.cos(a) * 9, tip[1] + math.sin(a) * 9),
                             (tip[0] + math.sin(a) * 4, tip[1] - math.cos(a) * 4)], fill))
    out += S.fuse(shapes)
    for k in range(tiers):
        a = angle + (k - (tiers - 1) / 2) * 0.32
        L = length * (0.72 + 0.14 * k)
        out.append(S.line([(root[0] + math.cos(a) * L * 0.25, root[1] + math.sin(a) * L * 0.25),
                           (root[0] + math.cos(a) * L * 0.9, root[1] + math.sin(a) * L * 0.9)], S.shade(fill, 0.3), 1.6))
    return out


# --- the Park Tyrant: goose NECK + twice the MASS, HISSING. The neck thrust out low and forward, the bill open on a tongue,
# both wings raised behind like a cloak, a wide stance. -----------------------------------------------------------------

def tyrant():
    fur, mark = B["park_tyrant"]
    pale = S.tint(fur, 0.55)
    parts = []
    parts += limb("wing_far", (84, 62), feather_wing((84, 62), -1.25, 66, 15, S.shade(fur, 0.12)), "lift", -10.0, 0.7, 0.5, depth=-3)
    for k, (hip, foot) in enumerate((((96, 96), (104, 120)), ((70, 98), (62, 120)))):
        x0, y0 = hip
        fx, fy = foot
        f = S.fuse([S.tube([hip, (fx, fy - 4)], 10, 8, GOLD), S.tri([(fx - 15, fy + 2), (fx, fy - 7), (fx + 9, fy + 2)], GOLD)])
        f += [S.line([(fx, fy - 3), (fx - 9, fy + 1)], S.shade(GOLD, 0.3), 1.4), S.line([(fx, fy - 3), (fx + 4, fy + 1)], S.shade(GOLD, 0.3), 1.4)]
        f += [S.tri([(fx - 17, fy + 1.5), (fx - 13, fy), (fx - 14, fy + 3.5)], INK), S.tri([(fx + 10, fy + 1.5), (fx + 6, fy), (fx + 7, fy + 3.5)], INK)]
        parts += leg("leg_far" if k == 0 else "leg_near", hip, f, 0.5 * (1 - k), depth=-2 if k == 0 else -1, amp=7.0)
    body = [
        S.ell(86, 80, 36, 28, fur),                                   # THE MASS
        S.ell(64, 84, 24, 22, fur),                                   # the chest, pushed forward
        S.tri([(110, 60), (126, 46), (122, 82)], fur),                # the tail, fanned up
        S.tri([(112, 66), (128, 62), (120, 86)], fur),
        S.tube(S.bez((60, 76), (40, 88), (24, 70)), 28, 20, fur),     # THE NECK, low and forward
        S.oval(20, 62, 17, 14, -0.3, fur),                            # the head, big
    ]
    parts += S.fuse(body)
    parts.append(S.ell(66, 92, 18, 13, pale, s=False))                # the breast
    for k, y in enumerate((86, 92, 98)):
        parts += scallops(52 + k * 2, 80 - k * 2, y, 5 - k, 1.3, S.shade(pale, 0.25))
    for k in range(4):                                                # bars down the neck
        x, y = 48 - k * 8, 82 - k * 3
        parts.append(S.curve(x - 3, y - 6, x - 1, y, x - 3, y + 6, w=2.0, colour=mark))
    parts.append(contour((96, 92), (92, 80), (102, 70)))             # the thigh
    parts += limb("wing_near", (76, 66), feather_wing((76, 66), -1.6, 60, 14, S.shade(fur, 0.05)), "lift", -10.0, 0.7, 0.0, back=False, depth=1)
    # the bill, OPEN: upper and lower, a tongue between, serrations
    parts += S.fuse([S.tri([(10, 66), (-2, 70), (-12, 80), (-8, 82), (8, 74)], S.shade(PINK_DEEP, 0.35))])   # the open mouth
    parts += S.fuse([S.tube(S.bez((12, 58), (-2, 54), (-14, 58)), 13, 6, GOLD)])                       # the upper bill
    parts += S.fuse([S.tube(S.bez((12, 72), (0, 76), (-10, 84)), 10, 5, S.shade(GOLD, 0.15))])         # the lower bill, dropped
    parts.append(S.tube(S.bez((6, 70), (-2, 72), (-8, 70)), 5, 3, PINK, s=False))                     # the tongue, up
    for x in (-8, -3, 2, 7):                                          # the serrated edge, a goose's teeth
        parts.append(S.tri([(x - 1.6, 61.5), (x, 65.5), (x + 1.6, 61.5)], TOOTH))
    parts.append(S.ell(-2, 55, 1.6, 1.1, S.shade(GOLD, 0.5), s=False))                                # a nostril
    parts += S.eyes(22, 56, 8, 8.8, asym=1.25, face=FACE)
    parts.append(brow(12, 44, 32, 52))
    return parts


# --- the Foreman: gristle MASS + bolted PLATES. Upright, a giant wrench over one shoulder, a hard hat with hazard stripes and
# a lamp, bolted plates on the shoulder, chest and knee, a thick tail with a steel band, fangs, a stitched scar --------

def foreman():
    fur, mark = B["factory_foreman"]
    steel, dark = PAL.MATERIALS["metal"], PAL.MATERIALS["metal_dark"]
    shine = S.tint(steel, 0.45)
    parts = []
    parts += S.fuse([S.tube(S.bez((96, 104), (122, 112), (124, 86)), 14, 6, fur)])          # the tail
    parts += S.fuse([S.rect(116, 104, 8, 12, steel, r=1.5)])                                 # its steel band
    parts += leg("leg_far", (88, 100), S.fuse([S.tube([(88, 100), (92, 114)], 18, 15, S.shade(fur, 0.1)),
                                              S.ell(94, 117, 12, 5, S.shade(fur, 0.1))]), 0.5, depth=-2, amp=6.0)
    body = [
        S.ell(70, 72, 34, 32, fur),                                   # the chest and shoulders: a wall
        S.ell(78, 98, 24, 18, fur),                                   # the belly and hips
        S.ell(44, 50, 18, 16, fur),                                   # the head, sunk between the shoulders
        S.tube(S.bez((34, 54), (24, 58), (16, 62)), 16, 9, fur),      # the snout
    ]
    for (x, y) in ((86, 42), (94, 48), (100, 56)):                    # fur bristling at the back of the neck
        body.append(S.tri([(x - 5, y + 4), (x + 3, y - 8), (x + 5, y + 4)], fur))
    parts += S.fuse(body)
    parts.append(S.ell(72, 92, 18, 12, S.tint(fur, 0.55), s=False))   # the belly
    parts.append(contour((60, 100), (72, 106), (86, 100), 1.6))
    parts += leg("leg_near", (70, 102), S.fuse([S.tube([(70, 102), (66, 114)], 20, 16, fur), S.ell(62, 117, 13, 5.5, fur)])
                 + S.fuse([S.rect(68, 108, 14, 10, steel, r=2)])
                 + [S.ell(64, 108, 1.6, 1.6, dark, s=False), S.ell(72, 108, 1.6, 1.6, dark, s=False)], 0.0, back=False, depth=1, amp=6.0)
    # the plates: a pauldron on the far shoulder, a chest plate, bolted
    parts += S.fuse([S.oval(90, 60, 16, 12, -0.4, steel), S.rect(66, 78, 26, 16, steel, r=3)])
    parts.append(S.line([(80, 52), (98, 62)], shine, 1.8))
    parts.append(S.line([(56, 72), (76, 72)], shine, 1.8))
    for (x, y) in ((84, 56), (96, 64), (56, 78), (76, 78), (56, 84), (76, 84)):
        parts.append(S.ell(x, y, 2.0, 2.0, dark, s=False))
    # the hard hat: a dome with a brim, hazard stripes, a lamp
    parts += S.fuse([S.ell(46, 36, 20, 13, GOLD), S.rect(46, 42, 46, 6, GOLD, r=3)])
    parts.append(S.line([(28, 38), (64, 38)], S.shade(GOLD, 0.35), 3.0))       # the hat's band
    parts += S.fuse([S.ell(30, 38, 5, 5, PAL.MATERIALS["paper"])])
    # the near arm: up, the hand on the wrench resting on the shoulder
    # the near arm: forward, the fist on a giant wrench stood on its head in front of him
    wrench = S.fuse([S.tube([(34, 86), (30, 108)], 9, 9, steel), S.ell(30, 112, 11, 9, steel)])
    wrench.append(S.tri([(24, 112), (36, 112), (30, 122)], S.shade(steel, 0.35), s=False))     # the jaw's gap
    wrench.append(S.line([(36, 90), (33, 105)], shine, 1.6))
    arm = S.fuse([S.tube(S.bez((60, 76), (48, 86), (36, 86)), 18, 15, fur), S.ell(34, 84, 11, 9, fur)])
    for k in range(3):
        arm.append(S.line([(29 + k * 4, 80), (29 + k * 4, 86)], INK, 1.4))
    parts += limb("wrench", (60, 76), wrench + arm, "swing", 3.0, 0.5, back=False, depth=2)
    parts += S.eyes(40, 50, 7.5, 7.5, asym=1.3, pupil=0.3, face=FACE)
    parts.append(brow(30, 40, 52, 46, 3.6))
    parts.append(nose(15, 61, 3.4, S.shade(fur, 0.5)))
    parts.append(S.curve(18, 66, 26, 70, 34, 66, w=1.8))
    parts.append(S.tri([(22, 67), (24, 72.5), (26, 67.5)], TOOTH))
    parts.append(S.tri([(28, 67.6), (30, 72), (32, 67.2)], TOOTH))
    parts.append(S.line([(100, 72), (106, 86)], INK, 2.0))           # a stitched scar
    for k in range(3):
        x, y = 101 + k * 2.2, 75 + k * 4.4
        parts.append(S.line([(x - 2.4, y + 1.4), (x + 2.4, y - 1.4)], INK, 1.2))
    return parts


# --- the Litter King: ROUND + THE ONLY CROWN. Enthroned on a throne of litter clumps, a robe with an ermine collar, a big
# jewelled crown, a sceptre he waves, a medal, little legs that swing ---------------------------------------------------

def litter_king():
    fur, mark = B["litter_king"]
    robe = PAL.MATERIALS["cloth"]
    trim = PAL.MATERIALS["paper"]
    clump = S.shade(fur, 0.25)
    parts = []
    rng = random.Random("throne")
    throne = [S.rect(68, 96, 72, 40, clump, r=10)]                    # the seat
    for k in range(6):                                                # the back: clumps piled high
        throne.append(S.ell(40 + k * 11 + rng.uniform(-2, 2), 30 + (abs(k - 2.5) * 6) + rng.uniform(-2, 2), 11, 10, clump))
    throne.append(S.rect(68, 52, 66, 46, clump, r=8))
    parts += S.fuse(throne)
    for _ in range(14):                                               # the litter's grain
        parts.append(S.ell(rng.uniform(36, 100), rng.uniform(36, 110), rng.uniform(1.4, 2.6), rng.uniform(1.2, 2.2),
                           S.shade(clump, 0.25), s=False))
    parts += S.fuse([S.oval(66, 84, 34, 30, 0.0, robe)])              # the robe, over the shoulders
    parts.append(S.line([(48, 70), (44, 108)], S.shade(robe, 0.25), 1.6))
    parts.append(S.line([(84, 70), (88, 108)], S.shade(robe, 0.25), 1.6))
    for k, x in enumerate((54, 78)):                                  # little legs off the seat, swinging
        parts += leg("leg_%d" % k, (x, 104), S.fuse([S.tube([(x, 104), (x - 2, 118)], 10, 9, fur), S.ell(x - 4, 120, 8, 4.5, fur)]),
                     0.5 * k, back=False, depth=1 + k, amp=10.0)
    body = [S.ell(66, 82, 26, 24, fur), S.ell(64, 50, 22, 20, fur)]   # the round body; the head
    parts += S.fuse(body)
    parts.append(S.ell(66, 88, 18, 14, S.tint(fur, 0.55), s=False))
    parts += S.fuse([S.tube(S.bez((40, 66), (64, 76), (90, 66)), 10, 10, trim)])            # the ermine collar
    for x, y in ((46, 70), (56, 73.5), (66, 74.5), (76, 73), (86, 69)):
        parts.append(S.tri([(x - 1.4, y - 1.6), (x, y + 2.2), (x + 1.4, y - 1.6)], INK))
    parts += S.fuse([S.ell(66, 86, 6, 6, GOLD)])                      # a medal
    parts.append(S.ell(66, 86, 2.6, 2.6, PAL.STACKLANDS["red"][0], s=False))
    crown = S.fuse([S.rect(64, 26, 46, 13, GOLD, r=2),
                    S.tri([(42, 28), (38, 2), (52, 24)], GOLD), S.tri([(54, 24), (64, -4), (74, 24)], GOLD),
                    S.tri([(76, 24), (90, 2), (86, 28)], GOLD)])
    parts += crown
    for x, y in ((38, 2), (64, -4), (90, 2)):
        parts += S.fuse([S.ell(x, y, 3.2, 3.2, GOLD)])
    for x, c in ((50, PAL.STACKLANDS["red"][0]), (64, PAL.STACKLANDS["blue"][0]), (78, PAL.STACKLANDS["green"][1])):
        parts += S.fuse([S.ell(x, 26, 3.6, 3.6, c)])
    parts.append(S.line([(44, 21), (84, 21)], S.shade(GOLD, 0.3), 1.2))
    sceptre = S.fuse([S.tube([(34, 92), (22, 46)], 6, 6, GOLD), S.ell(20, 40, 9, 9, GOLD)])
    sceptre += [S.line([(20, 34), (20, 46)], S.shade(GOLD, 0.35), 1.4), S.line([(14, 40), (26, 40)], S.shade(GOLD, 0.35), 1.4)]
    sceptre += S.fuse([S.ell(36, 90, 9, 8, fur)])
    sceptre += [S.line([(32 + k * 3.5, 87), (32 + k * 3.5, 92)], INK, 1.2) for k in range(3)]
    parts += limb("sceptre", (36, 90), sceptre, "swing", 10.0, 0.9, back=False, depth=3)
    parts.append(S.ell(48, 56, 5, 3.5, S.tint(PINK, 0.3), s=False))
    parts.append(S.ell(80, 56, 5, 3.5, S.tint(PINK, 0.3), s=False))
    parts += S.eyes(62, 46, 10, 9, asym=1.3, face=FACE)
    parts.append(S.ell(62, 58, 3.6, 2.8, S.shade(fur, 0.5), s=False))
    parts.append(S.curve(54, 63, 62, 67, 70, 63, w=2.0))
    return parts


# --- the Void God: NO FIXED SILHOUETTE. Floating: an irregular mass with one huge eye among many unpaired ones, a crown of
# horns of every length, a starfield inside it, a maw, five tentacles each on its own beat --------------------------

def void_god():
    fur, mark = B["void_god"]
    rng = random.Random("void_god")
    parts = []
    roots = (((44, 84), (28, 100), (14, 116), 11), ((58, 92), (54, 110), (60, 126), 13), ((74, 92), (88, 108), (82, 126), 10),
             ((88, 84), (108, 94), (122, 110), 9), ((96, 72), (118, 70), (126, 86), 7))
    for k, (root, ctrl, tip, w0) in enumerate(roots):
        spine = S.bez(root, ctrl, tip, 14)
        t = S.fuse([S.tube(spine, w0, 2.5, S.shade(fur, 0.07 * (k % 2)))])
        for n in range(3, 12, 2):
            x, y = spine[n]
            r = w0 * (1 - n / 14) * 0.24 + 0.6
            t.append(S.ell(x, y, r, r, S.tint(fur, 0.35), s=False))
        parts += limb("tentacle_%d" % k, root, t, "swing", (8 + 2.5 * k) * (1 if k % 2 else -1), 0.5 + 0.13 * k, 0.21 * k,
                      depth=-1 - (k % 2))
    lumps = [S.ell(66, 62, 36, 32, fur), S.ell(48, 72, 20, 18, fur), S.ell(86, 70, 18, 16, fur)]
    for i, (a, reach) in enumerate(((3.6, 46), (4.1, 58), (4.5, 50), (4.9, 64), (5.3, 44), (5.75, 54), (0.2, 40), (2.9, 42))):
        bx, by = 66 + reach * math.cos(a), 62 + reach * math.sin(a) * 0.85
        lumps.append(S.tri([(66 + 20 * math.cos(a - 0.32), 62 + 20 * math.sin(a - 0.32)), (bx, by),
                            (66 + 20 * math.cos(a + 0.32), 62 + 20 * math.sin(a + 0.32))], fur))       # horns, every length
    parts += S.fuse(lumps)
    for _ in range(18):                                               # the starfield inside it
        parts.append(S.ell(66 + rng.uniform(-30, 30), 62 + rng.uniform(-24, 26), rng.uniform(0.6, 1.4), rng.uniform(0.6, 1.4),
                           S.tint(fur, 0.6), s=False))
    for _ in range(4):
        parts.append(S.ell(66 + rng.uniform(-26, 26), 62 + rng.uniform(-20, 20), rng.uniform(5, 9), rng.uniform(4, 7), mark, s=False))
    for (x0, y0), (x1, y1), (x2, y2) in (((34, 48), (40, 56), (36, 64)), ((96, 46), (90, 54), (96, 60))):
        parts.append(S.line([(x0, y0), (x1, y1), (x2, y2)], S.tint(fur, 0.3), 1.4))
    maw = [(46, 82), (54, 76), (62, 81), (70, 75), (78, 81), (86, 77), (84, 88), (66, 94), (50, 90)]
    parts += S.fuse([S.tri(maw, INK)])
    for x in (52, 60, 68, 76, 82):
        parts.append(S.tri([(x - 2, 80), (x, 85.5), (x + 2, 80)], TOOTH))
    for x in (58, 66, 74):
        parts.append(S.tri([(x - 2, 92), (x, 87), (x + 2, 92)], TOOTH))
    parts.append(S.ell(64, 56, 16, 16, PAL.EYE))                       # THE EYE
    parts.append(S.ell(65.5, 57.5, 6.5, 6.5, INK, s=False))
    parts.append(S.ell(63, 54.5, 2.0, 2.0, PAL.EYE, s=False))         # its glint
    for ex, ey, er in ((38, 64, 6), (92, 56, 5), (82, 36, 3.6), (46, 40, 3.2), (100, 74, 2.8)):
        parts.append(S.ell(ex, ey, er, er, PAL.EYE))
        parts.append(S.ell(ex + er * 0.1, ey + er * 0.1, er * 0.32, er * 0.32, INK, s=False))
    for ex, ey in ((56, 32), (28, 74)):
        parts.append(S.curve(ex - 4, ey, ex, ey + 2.5, ex + 4, ey, w=1.6))
    for sx, sy, sr in ((10, 30, 2.6), (118, 26, 2.2), (14, 60, 2.0), (120, 52, 2.4), (30, 14, 1.8)):
        parts.append(S.ell(sx, sy, sr, sr, PAL.EYE, s=False))           # debris round it
    return parts


# --- the add: a LESSER version of the same idea, small and round, two stubby legs (now its own, stepping) --------------

def boss_add():
    fur, mark = B["add"]
    parts = []
    parts += leg("foot_far", (74, 100), S.fuse([S.ell(78, 112, 10, 6, S.shade(fur, 0.1))]), 0.5, depth=-1, amp=16.0)
    parts += S.fuse([S.ell(64, 82, 30, 26, fur)])
    parts.append(S.ell(64, 88, 20, 12, S.tint(fur, 0.55), s=False))
    parts += leg("foot_near", (54, 100), S.fuse([S.ell(50, 112, 10, 6, fur)]), 0.0, back=False, depth=1, amp=16.0)
    parts += S.eyes(64, 78, 11, 8, asym=1.3, face=FACE)
    parts.append(S.ell(64, 92, 3.2, 2.4, S.shade(fur, 0.5), s=False))
    return parts


SPECIES = {   # review key -> (today's file, the new drawing, the files it would replace). The bosses, redone; the imp
              # (rebuilt wing) is reviewed on its own board (HANDOFF-LOG (83)).
    "tyrant": ("boss_park_tyrant", tyrant, "boss_park_tyrant"),
    "foreman": ("boss_factory_foreman", foreman, "boss_factory_foreman"),
    "litter_king": ("boss_litter_king", litter_king, "boss_litter_king"),
    "void_god": ("boss_void_god", void_god, "boss_void_god"),
    "add": ("boss_add", boss_add, "boss_add"),
}

BATCH = {
    "title": "R5 bosses, redone",
    "crowd_floor": 11,              # the Factory: the Foreman's floor
    "notes": [
        "Redone (designer: Redo bosses): each a pose and a presence, the two cues kept. The Park Tyrant HISSING: the neck "
        "thrust out low, the bill open on a tongue and a serrated edge, both wings raised behind like a cloak, a wide stance. "
        "The Foreman upright, leaning on a giant wrench: a hard hat with a lamp, bolted plates on the shoulder, chest and knee, "
        "a banded tail, fangs, a stitched scar. The Litter King ENTHRONED on a throne of litter clumps: a robe, an ermine "
        "collar, a big jewelled crown, a sceptre he waves, a medal, little legs that swing. The Void God floating: one huge eye "
        "among many, a crown of horns of every length, a starfield inside it, a maw, five tentacles each on its own beat.",
        "Unchanged: the Rival Champion (a cat) and the generic boss (plain on purpose); the add keeps its look, its feet step.",
    ],
}
