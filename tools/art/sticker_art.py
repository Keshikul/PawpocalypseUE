"""The Sticker Box cast into the game's art (D18, S5; 2026-11-18).

    py tools/art/sticker_art.py OUT_DIR      # every recorded character as an SVG named as the game's file (a review copy)

make_sweet_art.py runs the grammar's batches first, then write_into() puts each character of the approved Bestiary
(tools/art/sticker_cast.py, recorded from the concept by sticker_record.js) over the grammar's drawing of the same file, at
the size the grammar wrote it. The SVG says it is a Sticker drawing (data-look), how wide its die-cut rim is (data-rim: the
rim is made when rastered, tools/pack_atlas.py, E2) and how wide its outline (data-ink: look_lint holds every outline to it).

A Sticker drawing is one still sprite: its rig layers (the grammar's moving wings and legs, R5) are removed with it, until
the drawings come with their own layers (E3, open).

THE CAT. The game draws the cat's hat and weapon as their own sprites over it, placed by constants (PawFieldLook.h: an
offset from the cat's centre and a width, as shares of the cat's sprite), the standing stages (1-3) by one set and the
mounted (4-7) by another. So each group of stages shares one frame (sticker_draw.frame), the weapons are cut out of the
concept's figures (sticker_record.js) and drawn in their own square of that frame, and the constants are worked out from
the drawings here: overlays() gives them, and the art build stops when PawFieldLook.h says otherwise (the number to write
is in the message). The ten job hats are the approved ones (2026-11-18, tools/art/concept_hats.js, recorded as
sticker_cast.JOB_HATS), drawn in the Tiny Kitten's frame: they share one square (as the weapons do), the standing stages
wear it where the concept puts it, the mounted ones through the rider's transform (riderFig draws catFig at
G(22, -63, 0.6)). A job's icon is its hat alone (the designer chose option B: "Not the top hats").
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import re  # noqa: E402

from sticker_draw import ICON_LW, SIZE, draw, fit, frame, square, to_svg  # noqa: E402
import sticker_cast as SC  # noqa: E402


def svg_of(fn, px: int) -> str:
    parts, k = fit(draw(fn))
    return to_svg(parts, px, k)


def stems() -> dict:
    """The game's file stem -> (the character's name in sticker_cast, its figure)."""
    out = {}
    for name, (files, fn) in SC.CAST.items():
        for stem in files:
            out[stem] = (name, fn)
    return out


# Where the Tiny Kitten's frame (the hats' units) sits in each group's frame: the standing cat is it; the rider's cat is
# catFig at G(22, -63, 0.6) inside riderFig (tx, ty, scale).
HAT_FRAME_STANDING = (0.0, 0.0, 1.0)
HAT_FRAME_MOUNTED = (22.0, -63.0, 0.6)


def hat_parts() -> list:
    return [draw(fn) for fn in SC.JOB_HATS.values()]


def hat_square() -> tuple:
    """(k, dx, dy): the one square every hat shares (so one set of constants places any of them), rims included."""
    return square(*hat_parts())


def _group(cats, weapons, hat_frame):
    """The frame of a group of cat stages, its weapons' square, and the overlay constants (x, y, width as shares of the
    cat's sprite from its centre, the art facing left) for the weapon and the hat."""
    cat_parts = [draw(fn) for _stem, fn in cats]
    weapon_parts = [draw(fn) for _stem, fn in weapons]
    cat_at, weapon_at = frame(*cat_parts), square(*weapon_parts)
    k, dx, dy = cat_at
    side, cx, cy = SIZE / k, (SIZE / 2 - dx) / k, (SIZE / 2 - dy) / k          # the cat's sprite, in concept units
    kw, dxw, dyw = weapon_at
    wside, wx, wy = SIZE / kw, (SIZE / 2 - dxw) / kw, (SIZE / 2 - dyw) / kw   # the weapon's square
    kh, dxh, dyh = hat_square()
    tx, ty, ts = hat_frame                                                    # the hats' square, in this group's frame
    hside, hx, hy = ts * SIZE / kh, tx + ts * (SIZE / 2 - dxh) / kh, ty + ts * (SIZE / 2 - dyh) / kh
    hat = ((hx - cx) / side, (hy - cy) / side, hside / side)
    weapon = ((wx - cx) / side, (wy - cy) / side, wside / side)
    return cat_parts, cat_at, weapon_parts, weapon_at, {"hat": hat, "weapon": weapon}


def overlays() -> dict:
    """The overlay constants PawFieldLook.h must hold: {"standing"|"mounted": {"hat"|"weapon": (x, y, width)}}."""
    return {"standing": _group(SC.STANDING, SC.STANDING_WEAPONS, HAT_FRAME_STANDING)[4],
            "mounted": _group(SC.MOUNTED, SC.MOUNTED_WEAPONS, HAT_FRAME_MOUNTED)[4]}


FIELD_LOOK = pathlib.Path(__file__).resolve().parent.parent.parent / "Source" / "Pawpocalypse" / "Realm" / "PawFieldLook.h"
HEADER_NAMES = {("standing", "hat"): ("HatOffsetX", "HatOffsetY", "HatWidth"),
                ("mounted", "hat"): ("HatOffsetMountedX", "HatOffsetMountedY", "HatWidthMounted"),
                ("standing", "weapon"): ("WeaponOffsetX", "WeaponOffsetY", "WeaponWidth"),
                ("mounted", "weapon"): ("WeaponOffsetMountedX", "WeaponOffsetMountedY", "WeaponWidthMounted")}


def header_faults() -> list:
    """Where PawFieldLook.h's overlay constants differ from the drawings' (to 3 decimals)."""
    text = FIELD_LOOK.read_text(encoding="utf-8")
    bad = []
    got = overlays()
    for (group, what), names in HEADER_NAMES.items():
        for name, value in zip(names, got[group][what]):
            m = re.search(r"\b%s = (-?[\d.]+)" % name, text)
            if not m or abs(float(m.group(1)) - round(value, 3)) > 1e-9:
                bad.append("%s = %.3f (PawFieldLook.h has %s)" % (name, value, m.group(1) if m else "none"))
    return bad


def player_svgs(px_of) -> dict:
    """stem -> SVG for the cat's stages and weapons (px_of: stem -> the size the grammar wrote it)."""
    out = {}
    for cats, weapons, hat_frame in ((SC.STANDING, SC.STANDING_WEAPONS, HAT_FRAME_STANDING),
                                     (SC.MOUNTED, SC.MOUNTED_WEAPONS, HAT_FRAME_MOUNTED)):
        cat_parts, cat_at, weapon_parts, weapon_at, _o = _group(cats, weapons, hat_frame)
        for (stem, _fn), parts in zip(cats, cat_parts):
            placed, k = fit(parts, cat_at)
            out[stem] = to_svg(placed, px_of(stem), k)
        for (stem, _fn), parts in zip(weapons, weapon_parts):
            placed, k = fit(parts, weapon_at)
            out[stem] = to_svg(placed, px_of(stem), k)
    return out


HAT_PX = 128    # the hats' sprites (the grammar drew them at 64; the new hats have stripes and stars to keep)


def hat_svgs() -> dict:
    """stem -> SVG: each job's hat in the hats' shared square (the field's sprite, hat_<id>)."""
    at = hat_square()
    out = {}
    for (job, _fn), parts in zip(SC.JOB_HATS.items(), hat_parts()):
        placed, k = fit(parts, at)
        out["hat_" + job] = to_svg(placed, HAT_PX, k)
    return out


def job_icon_svgs() -> dict:
    """stem -> SVG: each job's icon, its hat alone at the icon line (option B, 2026-11-18), over the grammar's job_<id>."""
    return {"job_" + job: icon_svg(fn) for job, fn in SC.JOB_HATS.items()}


def write_into(art_root, written: dict) -> None:
    """Over the grammar's drawing of each stem, at the size the grammar wrote it (written: stem -> (path, px))."""
    root = pathlib.Path(art_root)
    bad = header_faults()
    if bad:
        sys.exit("sticker_art: the cat's hat and weapon constants differ from the drawings; write these into "
                 "Source/Pawpocalypse/Realm/PawFieldLook.h (and its test, PawLookTests.cpp):\n  " + "\n  ".join(bad))

    def put(stem, svg):
        if stem not in written:
            sys.exit("sticker_art: the grammar wrote no %s to draw over" % stem)
        pathlib.Path(written[stem][0]).write_text(svg, encoding="utf-8", newline="\n")
        for layer in (root / "rigs").glob(stem + "__*.svg"):
            layer.unlink()

    for stem, (name, fn) in stems().items():
        put(stem, svg_of(fn, written[stem][1] if stem in written else SIZE))
    player = player_svgs(lambda stem: written[stem][1] if stem in written else SIZE)
    for stem, svg in player.items():
        put(stem, svg)
    for stem, fn in SC.CURSORS.items():
        put(stem, cursor_svg(fn, written[stem][1] if stem in written else 64))
    hats, icons = hat_svgs(), job_icon_svgs()
    for stem, svg in list(hats.items()) + list(icons.items()):
        put(stem, svg)
    print("  sticker_art: %d drawings from the approved Bestiary, the UI concept's kit and the approved hats (Sticker Box)"
          % (len(stems()) + len(player) + len(SC.CURSORS) + len(hats) + len(icons)))


# An icon keeps the margin the grammar's icons had (about 14 of the 128 box all round): the widgets that show them were
# sized for that.
ICON_MARGIN = 14.0


def icon_svg(fn) -> str:
    parts = draw(fn, ICON_LW)
    placed, k = fit(parts, square(parts, rim=0.0, margin=ICON_MARGIN))
    return to_svg(placed, SIZE, k, rim=0.0, ink=ICON_LW)


CURSOR_LW = 2.8      # the kit draws its paws at 2.8, with the rim (the paw is the cursor over menus)


def cursor_svg(fn, px: int) -> str:
    parts = draw(fn, CURSOR_LW)
    placed, k = fit(parts, square(parts))
    return to_svg(placed, px, k, ink=CURSOR_LW)


# The Reawaken ceremony (S4, 2026-11-18): the UI concept's scrCeremony, its background and its cat as two UI pictures.
# rays(800, 470, 32, '#7B5FD8', '#8B6BEF'), the halo (r 230, #B7A3E8 at .55) and its eight sparks (#FFF3A0) in the concept's
# 1600 x 900 frame, drawn 1920 x 1080; the cat as catFig({stage: 2}) with its sword, on its rim.
RAYS = (800.0, 470.0, 32, "#7B5FD8", "#8B6BEF")
HALO = (230.0, "#B7A3E8", 0.55)
SPARKS = ((520, 240, 16), (1080, 260, 22), (460, 640, 14), (1150, 620, 18), (690, 180, 10), (930, 170, 12), (600, 760, 10),
          (1020, 770, 12))
SPARK = "#FFF3A0"
# The Unicorn ceremony (S4, 2026-11-19): the UI concept's scrUnicorn background, rays(800, 500, 36, '#FF8F8F', '#FFC27A') and
# its four white sparks (no halo); the rainbow and the rider are the game's, painted by UPawUnicornCeremony.
UNICORN_RAYS = (800.0, 500.0, 36, "#FF8F8F", "#FFC27A")
UNICORN_SPARKS = ((480, 300, 18), (1120, 320, 22), (560, 560, 12), (1060, 560, 14))
UNICORN_SPARK = "#FFFFFF"


def ceremony_rays_svg(rays=RAYS, halo=HALO, sparks=SPARKS, spark=SPARK) -> str:
    import math
    from sticker_draw import EYE_LW, INK, r2, star4
    cx, cy, n, c1, c2 = rays
    o = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1600 900" width="1920" height="1080" data-raster="1920x1080">',
         '<rect width="1600" height="900" fill="%s"/>' % c1]
    for i in range(0, n, 2):
        a1, a2 = i / n * math.pi * 2, (i + 1) / n * math.pi * 2
        o.append('<path d="M%s %s L%s %s L%s %s Z" fill="%s"/>' % (cx, cy, r2(cx + 1400 * math.cos(a1)), r2(cy + 1400 * math.sin(a1)),
                                                                 r2(cx + 1400 * math.cos(a2)), r2(cy + 1400 * math.sin(a2)), c2))
    if halo:
        r, col, op = halo
        o.append('<circle cx="%s" cy="%s" r="%s" fill="%s" opacity="%s"/>' % (cx, cy, r, col, op))
    for x, y, rr in sparks:   # the concept's spark(): a star4 outlined at 0.7 of the eye line
        o.append('<path d="%s" fill="%s" stroke="%s" stroke-width="%s" stroke-linejoin="round"/>' % (star4(x, y, rr), spark, INK,
                                                                                                   r2(EYE_LW * 0.7)))
    return "\n".join(o + ["</svg>"]) + "\n"


def ceremony_cat_svg(px: int = 512) -> str:
    parts = draw(SC.cat_stage_2) + draw(SC.weapon_stage_2)
    placed, k = fit(parts, square(parts))
    return to_svg(placed, px, k).replace("<svg ", "<svg data-raster=\"%dx%d\" " % (px, px), 1)


def write_ceremony(ui_dir) -> None:
    ui = pathlib.Path(ui_dir)
    (ui / "ui_icon_ceremony_rays.svg").write_text(ceremony_rays_svg(), encoding="utf-8", newline="\n")
    (ui / "ui_icon_ceremony_cat.svg").write_text(ceremony_cat_svg(), encoding="utf-8", newline="\n")
    (ui / "ui_icon_unicorn_rays.svg").write_text(ceremony_rays_svg(UNICORN_RAYS, None, UNICORN_SPARKS, UNICORN_SPARK),
                                                 encoding="utf-8", newline="\n")
    print("  sticker_art: the Reawaken ceremony's rays and cat, the Unicorn ceremony's rays (Sticker Box)")


# The card pick (S4, 2026-11-18): the UI concept's NEW starburst, starN(12, 34, 26) in its redSoft with a 3 px ink edge, round
# joins (pickCard). The word goes over it as text (the game's string, in every language), not in the picture.
CARD_NEW = (12, 34.0, 26.0, 3.0)


def card_new_svg() -> str:
    import json
    import math
    from sticker_draw import INK, r2
    n, ro, ri, lw = CARD_NEW
    red = json.loads((pathlib.Path(__file__).resolve().parent / "sticker_colours.json").read_text(encoding="utf-8"))["uiRedSoft"]
    pts = []
    for i in range(2 * n):   # the concept's starN: from straight up, outer and inner by turns
        a = -math.pi / 2 + i * math.pi / n
        rr = ri if i % 2 else ro
        pts.append(("M" if i == 0 else "L") + "%s %s" % (r2(rr * math.cos(a)), r2(rr * math.sin(a))))
    half = ro + lw / 2 + 0.5
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="%s %s %s %s" width="128" height="128" data-raster="128x128">\n'
            '<path d="%s Z" fill="%s" stroke="%s" stroke-width="%s" stroke-linejoin="round"/>\n</svg>\n'
            % (r2(-half), r2(-half), r2(2 * half), r2(2 * half), " ".join(pts), red, INK, lw))


# The title (S4, 2026-11-18): the UI concept's scrTitle puts the Tiny Kitten (catFig({stage: 1})) and three mice (mouseFig())
# on the garden, each on its rim. UI pictures like the ceremony's cat: the cat 512 px, a mouse 256 px.
TITLE_CAST = {"ui_icon_title_cat": ("cat_stage_1", 512), "ui_icon_title_mouse": ("mouse", 256)}


def title_cast_svg(fn_name: str, px: int) -> str:
    parts = draw(getattr(SC, fn_name))
    placed, k = fit(parts, square(parts))
    return to_svg(placed, px, k).replace("<svg ", "<svg data-raster=\"%dx%d\" " % (px, px), 1)


def write_title(ui_dir) -> None:
    ui = pathlib.Path(ui_dir)
    for stem, (fn_name, px) in TITLE_CAST.items():
        (ui / (stem + ".svg")).write_text(title_cast_svg(fn_name, px), encoding="utf-8", newline="\n")
    print("  sticker_art: the title's cat and mouse (Sticker Box)")


# The save slots (S4, 2026-11-18): the UI concept's slotCard shows the save's cat at its stage (catFig, or riderFig from
# stage 4), with its weapon, on its rim: one 256 px picture per stage.
SLOT_CAT_PX = 256


def slot_cat_svg(stage: int, px: int = SLOT_CAT_PX) -> str:
    parts = draw(getattr(SC, "cat_stage_%d" % stage))
    if stage >= 2:
        parts += draw(getattr(SC, "weapon_stage_%d" % stage))
    placed, k = fit(parts, square(parts))
    return to_svg(placed, px, k).replace("<svg ", "<svg data-raster=\"%dx%d\" " % (px, px), 1)


def write_slots(ui_dir) -> None:
    ui = pathlib.Path(ui_dir)
    for stage in range(1, 8):
        (ui / ("ui_icon_slot_cat_%d.svg" % stage)).write_text(slot_cat_svg(stage), encoding="utf-8", newline="\n")
    print("  sticker_art: the save slots' cats, stages 1-7 (Sticker Box)")


# The job picker (S4, 2026-11-18): the UI concept's scrJobPick shows the Tiny Kitten wearing each job's hat (the approved
# hats, sticker_cast.JOB_HATS), on its rim: one 256 px picture per job, and the bare kitten for "No job".
def job_cat_svg(hat_fn=None, px: int = SLOT_CAT_PX) -> str:
    parts = draw(SC.cat_stage_1) + (draw(hat_fn) if hat_fn else [])
    placed, k = fit(parts, square(parts))
    return to_svg(placed, px, k).replace("<svg ", "<svg data-raster=\"%dx%d\" " % (px, px), 1)


def write_job_cats(ui_dir) -> None:
    ui = pathlib.Path(ui_dir)
    for job, fn in SC.JOB_HATS.items():
        (ui / ("ui_icon_job_cat_%s.svg" % job)).write_text(job_cat_svg(fn), encoding="utf-8", newline="\n")
    (ui / "ui_icon_job_cat_none.svg").write_text(job_cat_svg(), encoding="utf-8", newline="\n")
    print("  sticker_art: the job picker's cats in their hats (Sticker Box)")


def write_card_pick(ui_dir) -> None:
    (pathlib.Path(ui_dir) / "ui_icon_card_new.svg").write_text(card_new_svg(), encoding="utf-8", newline="\n")
    print("  sticker_art: the card pick's NEW starburst (Sticker Box)")


# Icons the UI concept adds that the grammar never drew (new files, not drawn over one): D18 (S4) the controller toast's.
NEW_ICONS = {"ui_icon_pad"}


def write_icons(ui_dir) -> None:
    """The UI concept's kit icons (S6, first part) over the grammar's icons of the same files (sweet_ui.py calls this
    after it writes its own): the tabs, the quick bar, the menu's links, the currencies and the small marks."""
    ui = pathlib.Path(ui_dir)
    for stem, fn in SC.ICONS.items():
        target = ui / (stem + ".svg")
        if not target.exists() and stem not in NEW_ICONS:
            sys.exit("sticker_art: the grammar wrote no %s to draw over" % target)
        target.write_text(icon_svg(fn), encoding="utf-8", newline="\n")
    for stem, fn in SC.UPGRADE_ICONS.items():   # new files: the grammar drew no upgrade icons
        (ui / (stem + ".svg")).write_text(icon_svg(fn), encoding="utf-8", newline="\n")
    print("  sticker_art: %d icons from the UI concept's kit (Sticker Box)" % (len(SC.ICONS) + len(SC.UPGRADE_ICONS)))


def build(out_dir) -> list:
    out = pathlib.Path(out_dir)
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    for stem, (name, fn) in stems().items():
        parts, k = fit(draw(fn))
        (out / (stem + ".svg")).write_text(to_svg(parts, 128, k), encoding="utf-8", newline="\n")
        rows.append((name, stem, len(parts), k))
    return rows


if __name__ == "__main__":
    if sys.argv[1:] == ["--overlays"]:
        for group, o in overlays().items():
            for what, (x, y, w) in o.items():
                print("%-8s %-6s x %.3f  y %.3f  width %.3f" % (group, what, x, y, w))
        sys.exit(0)
    for row in build(sys.argv[1] if len(sys.argv) > 1 else "build/sticker_art"):
        print("%-16s %-30s %3d parts, k %.3f" % row)
