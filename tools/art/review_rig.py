#!/usr/bin/env python3
"""Restyle R5, the rig's review sheet: the enemies' wings and legs moving (tools/art/sweet_anatomy.py limb(); designer,
2026-11-09: "would it be possible to have wings as a separate layer so they can be animated? Just a basic wing flap
rotation. Also could you do the same for the legs on other enemies?").

Reads what ships: SourceArt/svg/rigs/<stem>__<layer>.svg (make_sweet_art writes them; build_art.py), each layer turned about
its pivot exactly as the Realm view turns it (sweet_anatomy: swing = amp sin, lift = amp (1 - cos) / 2). Each creature on the
floor it ships on at its EnemyData size (1x), and beside it at 2x. Writes build/review/rig.gif (one loop, 2.5 s: every hz in
the rig is a multiple of 0.4) and build/review/rig.png (rest | the first extreme | the second, 2x). Writes nothing the game
loads.

    py tools/art/review_rig.py            # what ships (SourceArt/svg/rigs)
    py tools/art/review_rig.py --batch    # the anatomy batch under review (sweet_anatomy.SPECIES), from its drawings:
                                          # build/review/rig_batch.gif
"""

from __future__ import annotations

import io
import math
import pathlib
import re
import sys

from PIL import Image, ImageDraw

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import palette as PAL          # noqa: E402
import sweet_anatomy as A      # noqa: E402
import sweet_shapes as S       # noqa: E402
import make_sweet_art as M     # noqa: E402
import review_d3 as R          # noqa: E402  (floors, shadows, fonts)
import review_anatomy as RA    # noqa: E402  (floor_of, floor_name, size_of)

import resvg_py                # noqa: E402

ROOT = HERE.parent.parent
RIGS = ROOT / "SourceArt" / "svg" / "rigs"
OUT = ROOT / "build" / "review"
SS = R.SS
PLUM = PAL.PLUM
LOOP = 2.5          # seconds: STEP_HZ 2, FLAP_HZ 3.2, LIFT_HZ 0.8, the gull's 2.4 all loop in it
FPS = 20

SHOW = ["enemy_backyard_mouse", "enemy_factory_rat", "enemy_mouse_flying", "enemy_backyard_gristle",
        "enemy_litterbox_gristle", "enemy_backyard_moth", "enemy_alley_vole", "enemy_park_goose",
        "enemy_rooftop_pigeon", "enemy_rooftop_seagull", "enemy_vet_parrot", "enemy_mythic_crow"]


DRAWINGS = {}       # stem -> drawing function, for --batch: the layers come from the drawing, not from SourceArt


def layers(stem: str) -> list:
    """[(order, name, svg, rig dict or None)], back to front."""
    if stem in DRAWINGS:
        (_, _, fitted), = M.fitted([("%s/%s.svg" % (RA.folder_of(stem), stem), 128, DRAWINGS[stem])])
        split = A.rig_layers(fitted())
        out = []
        for (name, meta, ps), order in zip(split, A.depths(split)):
            rig = None if meta is None else {"pivot": meta["pivot"], "motion": meta["motion"], "amp": meta["amp"],
                                             "hz": meta["hz"], "phase": meta["phase"]}
            out.append((order, name, S.to_svg(ps, S.SIZE, px=128), rig))
        return out
    out = []
    for f in RIGS.glob(stem + "__*.svg"):
        svg = f.read_text(encoding="utf-8")
        head = svg[:svg.index(">")]
        a = dict(re.findall(r'data-([a-z-]+)="([^"]*)"', head))
        if a["rig-of"] != stem:
            continue
        rig = None
        if a["layer"] != "body":
            px, py = (float(v) for v in a["pivot"].split())
            rig = {"pivot": (px, py), "motion": a["motion"], "amp": float(a["amp"]), "hz": float(a["hz"]),
                   "phase": float(a["phase"])}
        out.append((int(a["order"]), a["layer"], svg, rig))
    if not out:
        raise SystemExit("no rig for %s (run tools/art/build_art.py)" % stem)
    return sorted(out, key=lambda o: o[0])


def angle(rig, t: float) -> float:
    """Degrees, clockwise on screen (the Realm view's formula)."""
    if rig is None:
        return 0.0
    a = 2.0 * math.pi * (rig["hz"] * t + rig["phase"])
    if rig["motion"] == "lift":
        return rig["amp"] * (1.0 - math.cos(a)) / 2.0
    return rig["amp"] * math.sin(a)


class Posed:
    """One creature's layers rasterised once at n px, posed per frame."""

    def __init__(self, stem: str, n: int):
        self.n = n
        self.layers = []
        for _o, _name, svg, rig in layers(stem):
            im = Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_string=svg, width=n, height=n)))).convert("RGBA")
            self.layers.append((im, rig))

    def at(self, t: float) -> Image.Image:
        out = Image.new("RGBA", (self.n, self.n))
        k = self.n / 128.0
        for im, rig in self.layers:
            if rig is not None:
                im = im.rotate(-angle(rig, t), resample=Image.BICUBIC, center=(rig["pivot"][0] * k, rig["pivot"][1] * k))
            out.alpha_composite(im)
        return out


def main() -> int:
    global SHOW
    batch = "--batch" in sys.argv
    title = "R5: the rig, wings and legs moving"
    if batch:
        SHOW = [stem for stem, _fn, _covers in A.SPECIES.values()]
        DRAWINGS.update({stem: fn for stem, fn, _covers in A.SPECIES.values()})
        title = A.BATCH["title"] + ", moving"
    biggest = max(RA.size_of(stem) for stem in SHOW)
    cw, ch = max(150, int(biggest * 1.5)), max(120, int(biggest * 1.2))   # one view at 1x, field px; beside it 2x
    block = cw + 12 + 2 * cw
    per_row, pad, gap = (3 if biggest <= 120 else 2), 30, 24          # the bosses: two to a row
    rows = (len(SHOW) + per_row - 1) // per_row
    W = pad * 2 + per_row * block + (per_row - 1) * gap
    head = 96
    H = pad + head + rows * (2 * ch + 40) + pad

    cells = []
    for i, stem in enumerate(SHOW):
        tier = RA.floor_of(stem)
        px = RA.size_of(stem)
        bg1 = R.Cell(cw, ch, tier).img
        bg2 = R.Cell(2 * cw, 2 * ch, tier).img
        cells.append((stem, tier, px, bg1, bg2, Posed(stem, int(round(px * SS))), Posed(stem, int(round(2 * px * SS)))))

    def frame(t: float, label: bool = True) -> Image.Image:
        sheet = Image.new("RGB", (W * SS, H * SS), R.rgb(PLUM["deep"]))
        d = ImageDraw.Draw(sheet)

        def text(x, y, s, px, col, black=False):
            d.text((x * SS, y * SS), s, font=R.font(px, black), fill=R.rgb(col))
        if label:
            text(pad, pad, title, 32, PLUM["butter"], black=True)
            text(pad, pad + 46, "Each at its game size on its floor, then 2x. Legs step 2x a second; spread wings flap; "
                 "folded wings lift and settle.", 16, PLUM["lilac"])
        for i, (stem, tier, px, bg1, bg2, p1, p2) in enumerate(cells):
            r, c = divmod(i, per_row)
            x = pad + c * (block + gap)
            y = pad + head + r * (2 * ch + 40)
            for bg, posed, scale, ox in ((bg1, p1, 1, 0), (bg2, p2, 2, cw + 12)):
                cell = R.Cell.__new__(R.Cell)
                cell.w, cell.h, cell.img = bg.width // SS, bg.height // SS, bg.copy()
                cell.creature(posed.at(t), px * scale, cell.w / 2, cell.h * 0.48)
                sheet.paste(cell.img.convert("RGB"), ((x + ox) * SS, y * SS))
            if label:
                text(x, y + 2 * ch + 6, "%s  (%s, %d px)" % (stem.replace("enemy_", ""), RA.floor_name(tier), px), 14,
                     PLUM["lilac"])
        return sheet.resize((W, H), Image.LANCZOS)

    OUT.mkdir(parents=True, exist_ok=True)
    frames = [frame(k / FPS) for k in range(int(LOOP * FPS))]
    pal = frames[0].quantize(colors=255, method=Image.Quantize.MEDIANCUT)
    gif = [f.quantize(palette=pal, dither=Image.Dither.NONE) for f in frames]
    name = "rig_batch" if batch else "rig"
    gif[0].save(OUT / (name + ".gif"), save_all=True, append_images=gif[1:], duration=int(1000 / FPS), loop=0, optimize=True)
    # the static sheet: rest, then the times each walker's legs are at their extremes (a quarter and three quarters of a step)
    stills = [frame(0.0), frame(0.125), frame(0.375)]
    still = Image.new("RGB", (W, H * 3))
    for k, f in enumerate(stills):
        still.paste(f, (0, k * H))
    still.save(OUT / (name + ".png"))
    print("review_rig: build/review/%s.gif (%dx%d, %d frames), build/review/%s.png" % (name, W, H, len(frames), name))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
