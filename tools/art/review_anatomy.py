#!/usr/bin/env python3
"""Restyle R5, the anatomy pass's review sheets (tools/art/sweet_anatomy.py; docs/WORKLIST-RESTYLE-2026-11-07.md §6).

Draws the batch in sweet_anatomy.SPECIES (its title, crowd floor and notes: sweet_anatomy.BATCH). Batch 2 (mouse, rat,
gristle, moth, vole, hamster) and later batches are written once approved; the sheet is made per batch.

Writes build/review/anatomy.png and the new drawings as SVGs under build/review/anatomy/ (for the lint). Writes nothing
the game loads.

At the game's scale (1080p): each species today beside its new drawing on the floor it ships on, at its EnemyData
sprite_target_px, with the atlas shadow; then 2x; then a crowd on the batch's floor, today against new. Every new drawing is
run through look_lint's rules (colour, ink, flat, eyes, ground) and the result is printed on the sheet.

    py tools/art/review_anatomy.py
"""

from __future__ import annotations

import io
import pathlib
import sys

from PIL import Image, ImageDraw

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import palette as PAL          # noqa: E402
import sweet_shapes as S       # noqa: E402
import make_sweet_art as M     # noqa: E402
import sweet_anatomy as A      # noqa: E402
import review_d3 as R          # noqa: E402  (floors, shadows, fonts, the game's sizes)
import look_lint as L          # noqa: E402

import resvg_py                # noqa: E402

ROOT = HERE.parent.parent
SS = R.SS
PLUM = PAL.PLUM
OUT = ROOT / "build" / "review" / "anatomy"

def floor_of(stem: str) -> int:
    """The first floor (tier number) the drawing ships on (TierData, via look_lint.grounds)."""
    floors = L.grounds().get(stem)
    if not floors:
        raise SystemExit("%s ships on no floor" % stem)
    return min(int(f.split("_")[1]) for f in floors)


def floor_name(tier: int) -> str:
    import csv
    for r in csv.DictReader(open(ROOT / "Data" / "TierData.csv", encoding="utf-8")):
        if r["id"] == "tier_%d" % tier:
            return r["display_name"]
    return "tier %d" % tier


BOSS_SCALE = 0.9           # boss.tscn: its drawing at the SVG's own size (256) x 0.9 (pack_atlas.BOSS_SCALE)
ADD_PX = 15.0 * 1.5 * 2.0  # boss_add.gd: RADIUS x 1.5, a diameter


def folder_of(stem: str) -> str:
    return "bosses" if stem.startswith("boss_") else "creatures"


def size_of(stem: str) -> float:
    """The size the game draws the drawing at, 1080p (an enemy's sprite_target_px; a boss's 256 x 0.9; an add's 45)."""
    if stem == "boss_add":
        return ADD_PX
    if stem.startswith("boss_"):
        return 256.0 * BOSS_SCALE
    sizes = [float(r["sprite_target_px"]) for r in R.ENEMIES.values() if pathlib.PurePosixPath(r["texture_path"]).stem == stem]
    if not sizes:
        raise SystemExit("no EnemyData row draws %s" % stem)
    return max(sizes)


def new_svgs(edges: list) -> dict:
    """key -> the new drawing's SVG (fitted as make_sweet_art fits a creature), recording its tint() derivations."""
    real = S.tint

    def spy(c, amount, toward=L.CREAM_TARGET):
        out = real(c, amount, toward)
        edges.append((c.upper(), toward.upper(), out.upper()))
        return out

    S.tint = spy
    try:
        out = {}
        for key, (stem, fn, _) in A.SPECIES.items():
            (_, _, fitted), = M.fitted([("%s/%s.svg" % (folder_of(stem), stem), 128, fn)])
            out[key] = S.to_svg(fitted(), S.SIZE, px=128)
        return out
    finally:
        S.tint = real


def sprite_svg(svg: str, px: float) -> Image.Image:
    n = max(1, int(round(px * SS)))
    return Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_string=svg, width=n, height=n)))).convert("RGBA")


def today_svg(stem: str) -> str:
    return (ROOT / "SourceArt" / "svg" / folder_of(stem) / (stem + ".svg")).read_text(encoding="utf-8")


def wrap(s, px, width):
    f = R.font(px)
    out, line = [], ""
    for w in s.split():
        trial = (line + " " + w).strip()
        if f.getlength(trial) / SS > width and line:
            out.append(line)
            line = w
        else:
            line = trial
    return out + [line]


def lint(new: dict, edges: list) -> list:
    ok = L.allowed(edges)
    where = L.grounds()
    OUT.mkdir(parents=True, exist_ok=True)
    lines = []
    for key, svg in new.items():
        stem = A.SPECIES[key][0]
        path = OUT / (stem + ".svg")
        path.write_text(svg, encoding="utf-8", newline="\n")
        name = path.name
        bad = (L.colour_faults(name, svg, ok) + L.ink_faults(name, svg) + L.flat_faults(name, svg) + L.eye_faults(name, svg)
               + L.ground_faults(path, svg, where))
        eyes = L.eye_px56(svg)
        was = L.eye_px56(today_svg(stem))
        lines.append("%s: %s; eyes %.0f px at 56 px (today %.0f)" % (key, "lint clean" if not bad else "; ".join(b[3] for b in bad), eyes, was))
    return lines


def main() -> int:
    edges = L.derivations()
    new = new_svgs(edges)
    notes = lint(new, edges)

    pad, gap = 40, 24
    biggest = max(size_of(stem) for stem, _fn, _c in A.SPECIES.values())
    cw, ch = max(300, int(biggest * 2.5)), max(190, int(biggest * 1.3))   # one cell at 1x (today | new); beside it 2x
    block = cw + 16 + 2 * cw
    per_row = 2 if biggest <= 120 else 1                              # the bosses: one to a row
    keys = list(A.SPECIES)
    rows = (len(keys) + per_row - 1) // per_row
    W = pad * 2 + per_row * block + (per_row - 1) * gap
    crowd_w, crowd_h = (W - 2 * pad - gap) // 2, 380
    H = pad + 170 + rows * (60 + 2 * ch + 30) + 80 + crowd_h + 60 + 60 + 30 * (len(notes) + 10) + pad
    sheet = Image.new("RGB", (W * SS, H * SS), R.rgb(PLUM["deep"]))
    d = ImageDraw.Draw(sheet)

    def text(x, y, s, px, col, black=False):
        d.text((x * SS, y * SS), s, font=R.font(px, black), fill=R.rgb(col))

    y = pad
    text(pad, y, A.BATCH["title"], 44, PLUM["butter"], black=True)
    for i, line in enumerate(wrap("Designer: \"most enemies look like indistinct blobs... it needs a bit more anatomy and shape.\" Same style "
                                  "(one ink, flat, big panicked eyes, chubby); new body plans: legs and tails as tapered tubes, snouts, "
                                  "chest and hips as separate masses, a thigh or shoulder line, one species cue that reads at 56 px. "
                                  "Each pair: today on the left, new on the right, on the floor it ships on, at its own size; then 2x.",
                                  20, W - 2 * pad)):
        text(pad, y + 60 + i * 28, line, 20, PLUM["lilac"])
    y += 170

    for r in range(rows):
        row = keys[r * per_row:(r + 1) * per_row]
        for i, key in enumerate(row):
            x = pad + i * (block + gap)
            stem, _, covers = A.SPECIES[key]
            text(x, y, key, 22, PLUM["bone"], black=True)
            text(x, y + 30, "today | new.  Replaces " + covers, 15, PLUM["lilac"])
        y += 60
        for i, key in enumerate(row):
            x = pad + i * (block + gap)
            stem = A.SPECIES[key][0]
            tier = floor_of(stem)
            px = size_of(stem)
            c = R.Cell(cw, ch, tier)
            c.creature(sprite_svg(today_svg(stem), px), px, cw * 0.27, ch * 0.5)
            c.creature(sprite_svg(new[key], px), px, cw * 0.73, ch * 0.5)
            sheet.paste(c.img.convert("RGB"), (x * SS, y * SS))                   # 1x: SS sheet px per field px
            text(x + 6, y + ch + 6, "1x: %s, %d px" % (floor_name(tier), px), 15, PLUM["lilac"])
            big = c.img.resize((2 * cw * SS, 2 * ch * SS), Image.LANCZOS)          # 2x: two sheet px per field px
            sheet.paste(big.convert("RGB"), ((x + cw + 16) * SS, y * SS))
        y += 2 * ch + 30
    # a crowd: the batch on its floor, at their sizes, today against new
    crowd_tier = A.BATCH["crowd_floor"]
    place = floor_name(crowd_tier)
    place = place[4:] if place.startswith("The ") else place          # "The Neighbour's Kitchen": not "THE THE"
    text(pad, y, "A CROWD ON THE %s at game size: today | new" % place.upper(), 26, PLUM["butter"], black=True)
    y += 50
    import random
    rng = random.Random(7)
    spots = [(rng.uniform(40, crowd_w - 40), rng.uniform(40, crowd_h - 40)) for _ in range(14)]
    mix = (list(A.SPECIES) * 14)[:14]
    for j, which in enumerate(("today", "new")):
        c = R.Cell(crowd_w, crowd_h, crowd_tier)
        cat = R.svg_img(ROOT / "SourceArt" / "svg" / "player" / "player_cat_stage_1.svg", R.CAT_PX)
        for (sx, sy), key in sorted(zip(spots, mix), key=lambda p: p[0][1]):
            stem = A.SPECIES[key][0]
            px = size_of(stem)
            svg = today_svg(stem) if which == "today" else new[key]
            c.creature(sprite_svg(svg, px), px, sx, sy, flip=sx > crowd_w / 2)
        c.creature(cat, R.CAT_PX, crowd_w / 2, crowd_h / 2)
        sheet.paste(c.img.convert("RGB"), ((pad + j * (crowd_w + gap)) * SS, y * SS))
    y += crowd_h + 40

    text(pad, y, "The lint on the new drawings, and what is next", 24, PLUM["butter"], black=True)
    y += 40
    more = notes + A.BATCH["notes"]
    for n in more:
        for line in wrap("- " + n, 18, W - 2 * pad):
            text(pad, y, line, 18, PLUM["lilac"])
            y += 27

    out = ROOT / "build" / "review" / "anatomy.png"
    img = sheet.resize((W, H), Image.LANCZOS)
    img = img.crop((0, 0, W, min(H, y + pad)))
    img.save(out)
    print("wrote %s (%dx%d)" % (out, img.width, img.height))
    for n in notes:
        print("  " + n)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
