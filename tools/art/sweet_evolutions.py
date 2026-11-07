#!/usr/bin/env python3
"""The seven player evolutions and the unicorn mount, composed from one cat.

THE RULE THIS OBEYS (designer, 2026-09-08): "Cat generations should be the
original cat with extra pieces on top such as armor." So there is exactly one
cat in this file - the same spec sweet_shapes.py draws for the player - and
every stage is that cat plus something. No stage redraws the animal.

That is also what makes the ladder read as a ladder. If each stage were drawn
fresh, stage 5 would be a different character wearing stage 4's name; built
additively, the player sees their own cat gaining a helmet, then a visor, then
a mount, then wings.

STAGES 4-7 ADD A MOUNT rather than changing the cat, because the reference
(content/art/reference/sweet/evo4_rainbow_cavalry.png) is a tall long-legged
unicorn with the SAME small round cat sitting on a saddle blanket on its back.
The cat does not grow; the thing under it does.

    py tools/sweet_evolutions.py            # sheet -> build/sweet_evolutions.png
    py tools/sweet_evolutions.py --svg DIR  # write .svg files instead
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

INK = S.INK
EYE = S.EYE_WHITE
SIZE = S.SIZE

# A PASTEL rainbow, not a saturated one, and with the red end pulled to coral so
# it never lands in the cotton-candy pink the designer ruled out.
RAINBOW = ["#EE9384", "#F3B87C", "#F0D989", "#A9D18C", "#8FC7D6", "#9AA4DC", "#BE9BD3"]

STEEL = "#B9BFC9"
STEEL_DARK = "#8A93A2"
GOLD = PAL.GOLD
HOOF = "#5E5566"


# --- composition -------------------------------------------------------------

def xform(parts, dx=0.0, dy=0.0, scale=1.0):
    """Move and scale a finished part list. This is what lets one cat be reused
    at four sizes on four mounts without redrawing it."""
    out = []
    for p in parts:
        q = dict(p)
        if q["k"] == "ell":
            q["cx"] = q["cx"] * scale + dx
            q["cy"] = q["cy"] * scale + dy
            q["rx"] *= scale
            q["ry"] *= scale
        elif q["k"] == "rect":
            # Rects were added for props (sweet_shapes.rect) after this was
            # written, and fell through to the polyline branch: a KeyError on
            # "pts" the first time anything with a rect was moved.
            q["cx"] = q["cx"] * scale + dx
            q["cy"] = q["cy"] * scale + dy
            q["w"] *= scale
            q["h"] *= scale
            q["r"] *= scale
        elif q["k"] == "tri":
            q["pts"] = [(x * scale + dx, y * scale + dy) for x, y in q["pts"]]
        else:
            q["pts"] = [(x * scale + dx, y * scale + dy) for x, y in q["pts"]]
            q["w"] = max(q["w"] * scale, 0.9)
        out.append(q)
    return out


def the_cat(seated=False):
    """The one cat. `seated` tucks the legs away, because a cat on a saddle does
    not need feet dangling through the blanket."""
    spec = dict(S.CAST()["player_cat"])
    if seated:
        spec = dict(spec)
        spec["feet"] = []
        spec["toes"] = []
    return S.creature(spec)


# --- the unicorn mount -------------------------------------------------------

def unicorn(coat="#F3EFE4", mane=RAINBOW, tail=RAINBOW, horn=GOLD, wings=None,
            starfield=False, blanket=None):
    """Tall, long-legged, long-necked. Deliberately NOT a round blob - it is the
    one silhouette in the game built from verticals, which is why a mounted
    stage reads instantly against everything else.

    THE SADDLE IS LAID HERE, between the body and the near wing (designer,
    2026-09-30: "the unicorn has a rainbow seat that is on top of wings"). It
    used to be appended by the caller after the whole mount, so it lay on top
    of the near wing - a blanket over a wing is a wing that is not attached to
    anything. `blanket` is the saddle's colours; None means no saddle."""
    parts = []

    # tail first, behind the body
    for i, c in enumerate(tail):
        off = i * 2.6
        parts.append(S.curve(104, 62 + off * 0.35, 126, 68 + off,
                             116, 100 + off * 0.6, w=5.0, colour=c))

    # THE WING FIX, 2026-09-11. The first pair was drawn at the flanks, behind
    # the body, at the height of the barrel - so the body ate them and stages 5
    # and 7 lost the one cue that is supposed to name them. A wing has to break
    # the BACK LINE (the body tops out at y=50) or it is not a wing, it is a
    # smudge. Far wing high and back; near wing lower and forward, drawn after
    # the body so it overlaps it. Feathers step up-right, which is also the only
    # way to get a raked wing out of unrotatable ellipses.
    FEATHER = ((0, 0, 19, 12), (11, -13, 17, 11), (20, -26, 14, 9),
               (27, -37, 11, 7))
    far_wing, near_wing = [], []
    if wings:
        fan = [S.ell(96 + ox, 50 + oy, rx, ry, S.shade(wings, 0.14))
               for ox, oy, rx, ry in FEATHER]
        far_wing = S.fuse(fan)
        fan = [S.ell(84 + ox * 0.86, 66 + oy * 0.86, rx * 0.86, ry * 0.86, wings)
               for ox, oy, rx, ry in FEATHER]
        near_wing = S.fuse(fan)
    parts += far_wing

    legs = []
    for lx, ly in ((34, 96), (52, 100), (84, 100), (104, 96)):
        legs.append(S.ell(lx, ly, 6.5, 24, coat))
    body = [S.ell(70, 74, 36, 24, coat), S.ell(46, 66, 16, 18, coat)]
    neck = [S.ell(36, 48, 11, 20, coat)]
    head = [S.ell(28, 28, 17, 13, coat), S.ell(14, 32, 9, 7, coat)]
    ears = [S.tri([(30, 18), (28, 4), (38, 16)], coat),
            S.tri([(40, 16), (44, 3), (48, 18)], coat)]
    parts += S.fuse(legs + body + neck + head + ears)
    # The starfield speckles the COAT, so it goes on before anything that sits
    # on the coat. Drawn last, as it was, the dots landed on the saddle and the
    # near wing of stage 7.
    if starfield:
        for sx, sy, sr in ((58, 40, 2.2), (92, 52, 1.8), (40, 84, 2.0),
                           (100, 34, 2.4), (76, 26, 1.6)):
            parts.append(S.ell(sx, sy, sr, sr, EYE, s=False))
    if blanket is not None:
        parts += saddle(blanket)
    parts += near_wing

    # hooves
    for lx in (34, 52, 84, 104):
        parts.append(S.ell(lx, 118, 7, 5, HOOF, s=False))

    # horn - the cue nothing else in the game has
    parts += S.fuse([S.tri([(20, 16), (6, -6), (30, 10)], horn)])
    for i in range(3):
        parts.append(S.line([(14 - i * 3, 8 - i * 5), (24 - i * 3, 6 - i * 5)],
                            S.shade(horn, 0.35), 1.6))

    # mane, a strand per colour down the neck
    for i, c in enumerate(mane):
        x = 40 + i * 2.2
        parts.append(S.curve(x, 18 + i * 1.2, x + 14, 34 + i * 2,
                             x + 6, 58 + i * 2.2, w=5.2, colour=c))

    parts.append(S.ell(16, 34, 6, 4.5, "#E9B9BE", s=False))          # muzzle
    parts += S.eyes(26, 26, 8, 6.5, asym=1.3, pupil=0.26)
    return parts


def saddle(colours=RAINBOW):
    """The blanket the cat sits on. Concentric arcs, flattest at the top so the
    cat has somewhere to sit."""
    parts = []
    for i, c in enumerate(reversed(colours[:5])):
        parts.append(S.ell(70, 58, 24 - i * 3.4, 13 - i * 1.9, c, s=(i == 0)))
    return parts


# --- stage-specific pieces ---------------------------------------------------

def helmet():
    """A crowned dome, a brow band, a nose guard and three rivets.

    Two constraints fight here. The dome has to stop above y=40 or it covers
    the eyes, and it has to stay inside x 21..75 or it swallows the cat's ear
    tips at x 14 and 82 - and those ears are the cat's whole silhouette cue
    (see docs/pawpocalypse-enemy-shape-bible.md). So the ears poke out of the
    sides like ear holes, which is both the cue preserved and the joke."""
    parts = S.fuse([S.ell(48, 29, 27, 13, STEEL),
                    S.tri([(41, 19), (48, 0), (55, 19)], STEEL)])
    parts.append(S.rect(48, 38, 56, 8, STEEL_DARK, r=3))
    parts.append(S.rect(48, 47, 9, 15, STEEL_DARK, r=3))
    for rx in (30, 48, 66):
        parts.append(S.ell(rx, 38, 2.3, 2.3, S.shade(STEEL_DARK, 0.32), s=False))
    return parts


def shoulder_plates():
    """Layered lames, not discs. One flat ellipse per shoulder read as a button
    stuck on the flank; two stacked with a dark lower edge reads as a pauldron
    because armour is made of overlapping plates and a button is not."""
    out = []
    for px, py in ((32, 80), (102, 74)):
        out += S.fuse([S.ell(px, py, 14.5, 10, STEEL),
                       S.ell(px, py + 9, 12.5, 7.5, STEEL)])
        out.append(S.ell(px, py + 9.5, 11, 4.6, STEEL_DARK, s=False))
    return out


def visor():
    """A band worn across the eyes, with a strap that goes round the skull -
    the first version floated in front of the face with nothing holding it."""
    parts = S.fuse([S.ell(48, 52, 27, 9.5, "#7FD4E8"),
                    S.ell(48, 52, 29, 4.5, STEEL_DARK)])
    parts.append(S.ell(57, 50, 7, 3.0, EYE, s=False))
    parts.append(S.line([(24, 46), (19, 28)], STEEL_DARK, 2.6))
    parts += S.fuse([S.ell(18, 24, 4.2, 4.2, "#7FD4E8")])
    return parts


def panel_lines():
    return [S.line([(78, 66), (78, 96)], S.shade(PAL.CAT_BODY, 0.3), 1.6),
            S.line([(64, 70), (96, 70)], S.shade(PAL.CAT_BODY, 0.3), 1.6)]


def halo():
    """Over the unicorn head, not the cat - the cat is tiny up there.

    Drawn BEHIND everything (see EVOLUTIONS) so the horn rises through the ring
    instead of colliding with it. The first version sat on top at the same
    height as the horn tip and the two read as one bent gold object.    A REAL RING, NOT A DISC WITH A LILAC LID. This drew its hole by painting a
    smaller ellipse in tier_8's FIELD colour, which is only invisible on the one
    backdrop the contact sheet happens to use. Stage 7 is the player at the end
    of the ladder and is seen in all twelve biomes, so everywhere except the
    mythic realm that "hole" was a solid lilac disc inside a gold rim. Caught by
    check_no_background_fills() after it had already shipped.

    There is no subtraction in this grammar. The ring is built from beads laid
    round an ellipse and fused, so the middle is genuinely empty.
    """
    import math
    beads = []
    for i in range(14):
        a = i * (2.0 * math.pi / 14.0)
        beads.append(S.ell(26 + math.cos(a) * 20.0, 4 + math.sin(a) * 5.6,
                           4.4, 3.0, GOLD))
    return S.fuse(beads)


# --- the ladder --------------------------------------------------------------

def EVOLUTIONS():
    out = {}

    out["unicorn"] = unicorn()

    # THE PLAYER'S KITTEN IS NOT THE SUMMONED KITTEN, 2026-09-11. These were the
    # same drawing, which meant player_cat_stage_1.svg and kitten_basic.svg were
    # byte-identical - and kittens SWARM AROUND THE PLAYER, so the one pairing
    # in the game where two sprites must never be confused was the one pairing
    # that shared a sprite. Stage 1 keeps kitten PROPORTIONS (it is the bottom
    # of the ladder) but wears the player's own colours, so the player reads it
    # as themselves; the minion keeps the cream kitten palette.
    tiny = dict(S.CAST()["kitten"])
    tiny["fur"] = PAL.CAT_BODY
    tiny["mark"] = S.shade(PAL.CAT_BODY, 0.42)
    tiny["tongue"] = True
    out["stage_1_tiny_kitten"] = S.creature(tiny)

    out["stage_2_armored_tomcat"] = the_cat() + shoulder_plates() + helmet()

    out["stage_3_cyber_cat"] = (the_cat() + shoulder_plates() + panel_lines()
                                + visor())

    # From here the cat stops changing and the thing under it does.
    rider = xform(the_cat(seated=True), dx=58.0, dy=17.0, scale=0.33)
    out["stage_4_rainbow_cavalry"] = unicorn(blanket=RAINBOW) + rider

    out["stage_5_alicorn"] = (unicorn(wings="#CFE4F5",
                              mane=["#CFE4F5", "#B9D6EE", "#A8C8E4"],
                              tail=["#CFE4F5", "#B9D6EE", "#A8C8E4"],
                              blanket=["#A8C8E4", "#CFE4F5", "#B9D6EE", "#8FC7D6", "#A8C8E4"])
                             + rider)

    out["stage_6_void_unicorn"] = (
        unicorn(coat="#6A5F86", mane=["#B9A6E8"] * 3 + ["#8E7FC9"] * 4,
                tail=["#B9A6E8", "#8E7FC9", "#6E5FA8"], horn="#D8CBF2",
                starfield=True,
                blanket=["#8E7FC9", "#B9A6E8", "#6E5FA8", "#D8CBF2", "#8E7FC9"])
        + rider)

    out["stage_7_the_last_unicorn"] = (
        halo() + unicorn(wings="#EFCE86", starfield=True, blanket=RAINBOW) + rider)

    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--svg", metavar="DIR")
    args = ap.parse_args()
    people = EVOLUTIONS()

    if args.svg:
        out = pathlib.Path(args.svg); out.mkdir(parents=True, exist_ok=True)
        for name, parts in people.items():
            (out / (name + ".svg")).write_text(S.to_svg(parts), encoding="utf-8")
            print("  %s.svg" % name)
        return 0

    cols, cell, small = 4, 214, 48
    rows = (len(people) + cols - 1) // cols
    ground = PAL.BACKDROPS["tier_8_mythic"]["field"]
    shadow = PAL.BACKDROPS["tier_8_mythic"]["detail"]
    sheet = Image.new("RGB", (cols * cell, rows * cell + 40), ground)
    d = ImageDraw.Draw(sheet)
    for i, (name, parts) in enumerate(people.items()):
        cx, cy = (i % cols) * cell, (i // cols) * cell + 30
        d.ellipse([cx + 34, cy + 124, cx + 134, cy + 140], fill=shadow)
        big = S.to_image(parts)
        sheet.paste(big, (cx + 14, cy + 6), big)
        tiny = S.to_image(parts).resize((small, small), Image.LANCZOS)
        sheet.paste(tiny, (cx + 152, cy + 24), tiny)
        d.text((cx + 12, cy + 150), "%s  right: %dpx" % (name, small), fill=PAL.INK)
    d.text((14, 8), "one cat, seven stages - each is the cat plus something",
           fill=PAL.INK)
    out = pathlib.Path("build/sweet_evolutions.png")
    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out)
    print("wrote %s (%dx%d)" % (out, sheet.width, sheet.height))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
