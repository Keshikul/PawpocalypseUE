"""SPIKE S3 (docs/04 §8, doc 07 §2) offline half: rasterise, pack, reference. Throwaway; the production tools/raster_svg.py and
tools/pack_atlas.py of phase 6 start from this.

Scope: the field sprites: every enemy texture (EnemyData.sprite_target_px), every boss (drawn at its native SVG size), the
cat evolution stages (Player.SPRITE_TARGET_PX 175) and the kitten (96). Godot sizes and scale factors from the Godot code:
sprites are fitted by their longest edge; the breathing pulse stretches height by up to 3 %, the cat's hop by up to 8 % more.

Raster variants (each sprite square, size rounded up to a multiple of 4):
  r1x    1x the largest on-screen size at 1080p
  r2x    2x the largest on-screen size at 1080p (doc 07's plan: 4K-sharp)
  godot  the SVG's native size, which is what Godot imports (svg/scale 1.0) and scales at draw time
Atlas pages: 2048 x 2048, shelf packing, cells padded and edge-extruded by EXTRUDE px (mip bleeding, doc 04 §9).
References: each sprite rasterised directly at its exact drawn size (1080p: target px; 4K: 2x), composited over black on the
same layout the Unreal capture uses, so a capture and its reference compare pixel for pixel.

  py tools/spikes/s3_raster_pack.py
Writes SourceArt/png/spike_s3/: <variant>/page_<n>.png, ref_1080.png, ref_2160.png, godot_like_1080.png, manifest.csv,
summary.txt.
"""
import csv
import io
import math
import os
import re

import resvg_py
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GODOT = "C:/Games/pawpocalypse"
OUT = os.path.join(ROOT, "SourceArt/png/spike_s3")
PAGE = 2048
EXTRUDE = 4
PULSE = 1.03          # SpriteLiveliness.PULSE_AMPLITUDE
CAT_HOP = 1.08        # Player.HOP_SQUASH 0.16 * 0.5
CAT_PX = 175.0        # Player.SPRITE_TARGET_PX
KITTEN_PX = 96.0      # Kitten.SPRITE_TARGET_PX


def godot_path(res):
    return os.path.join(GODOT, res.replace("res://", ""))


def svg_native(path):
    m = re.search(r'width="([\d.]+)"', open(path, encoding="utf-8").read(400))
    return float(m.group(1))


def sprites():
    """name -> (svg path, drawn px at 1080p, largest px at 1080p)."""
    out = {}
    for row in csv.DictReader(open(os.path.join(ROOT, "Data/EnemyData.csv"), encoding="utf-8")):
        p = godot_path(row["texture_path"])
        t = float(row["sprite_target_px"])
        name = os.path.basename(p)[:-4]
        if name not in out or out[name][1] < t:
            out[name] = (p, t, t * PULSE)
    for row in csv.DictReader(open(os.path.join(ROOT, "Data/BossData.csv"), encoding="utf-8")):
        p = godot_path(row["texture_path"])
        n = svg_native(p)
        out[os.path.basename(p)[:-4]] = (p, n, n * PULSE)
    for row in csv.DictReader(open(os.path.join(ROOT, "Data/EvolutionData.csv"), encoding="utf-8")):
        if row.get("texture_path"):
            p = godot_path(row["texture_path"])
            out[os.path.basename(p)[:-4]] = (p, CAT_PX, CAT_PX * PULSE * CAT_HOP)
    k = os.path.join(GODOT, "assets/art/player/kitten_basic.svg")
    out["kitten_basic"] = (k, KITTEN_PX, KITTEN_PX * PULSE)
    return dict(sorted(out.items()))


def ceil4(x):
    return int(math.ceil(x / 4.0) * 4)


def raster(path, size):
    return Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_path=path, width=size, height=size)))).convert("RGBA")


def extrude(img, e):
    """Copy the border pixels outward by e px so bilinear/mip sampling at the cell edge reads the sprite, not a neighbour."""
    w, h = img.size
    out = Image.new("RGBA", (w + 2 * e, h + 2 * e))
    out.paste(img, (e, e))
    out.paste(img.crop((0, 0, w, 1)).resize((w, e)), (e, 0))
    out.paste(img.crop((0, h - 1, w, h)).resize((w, e)), (e, h + e))
    side = out.crop((e, 0, e + 1, h + 2 * e))
    out.paste(side.resize((e, h + 2 * e)), (0, 0))
    side = out.crop((w + e - 1, 0, w + e, h + 2 * e))
    out.paste(side.resize((e, h + 2 * e)), (w + e, 0))
    return out


def pack(cells, page):
    """Shelf packing, tallest first. cells: name -> size. Returns name -> (page, x, y) of the sprite's top-left."""
    order = sorted(cells, key=lambda n: -cells[n])
    pos, p, x, y, shelf = {}, 0, 0, 0, 0
    for n in order:
        s = cells[n] + 2 * EXTRUDE
        if x + s > page:
            x, y, shelf = 0, y + shelf, 0
        if y + s > page:
            p, x, y, shelf = p + 1, 0, 0, 0
        pos[n] = (p, x + EXTRUDE, y + EXTRUDE)
        x += s
        shelf = max(shelf, s)
    return pos, p + 1


def screen_layout(drawn):
    """Field-pixel rects (1080p) for the capture: shelf rows across 1920 x 1080 with an 8 px gap."""
    order = sorted(drawn, key=lambda n: -drawn[n])
    rects, x, y, row = {}, 8, 8, 0
    for n in order:
        s = int(round(drawn[n]))
        if x + s > 1912:
            x, y, row = 8, y + row + 8, 0
        rects[n] = (x, y, s)
        x += s + 8
        row = max(row, s)
    assert y + row <= 1080, "layout overflows 1080"
    return rects


def godot_like(path, native, drawn):
    """Godot's path, approximated: raster at native size (import scale 1.0), then trilinear-ish minification to the drawn size."""
    img = raster(path, int(native))
    while img.width >= 2 * drawn:          # mip chain by box halving
        img = img.resize((img.width // 2, img.height // 2), Image.BOX)
    return img.resize((drawn, drawn), Image.BILINEAR)


def main():
    spr = sprites()
    os.makedirs(OUT, exist_ok=True)
    variants = {
        "r1x": {n: ceil4(v[2]) for n, v in spr.items()},
        "r2x": {n: ceil4(2 * v[2]) for n, v in spr.items()},
        "godot": {n: int(svg_native(v[0])) for n, v in spr.items()},
    }
    rects = screen_layout({n: v[1] for n, v in spr.items()})
    summary, rows = [], []
    for var, sizes in variants.items():
        pos, pages = pack(sizes, PAGE)
        _, pages4k = pack(sizes, 4096)
        imgs = [Image.new("RGBA", (PAGE, PAGE), (0, 0, 0, 0)) for _ in range(pages)]
        for n, (p, x, y) in pos.items():
            imgs[p].paste(extrude(raster(spr[n][0], sizes[n]), EXTRUDE), (x - EXTRUDE, y - EXTRUDE))
        os.makedirs(os.path.join(OUT, var), exist_ok=True)
        for i, im in enumerate(imgs):
            im.save(os.path.join(OUT, var, "page_%d.png" % i))
        texels = sum(s * s for s in sizes.values())
        summary.append("%-6s sprites %d, texels %d (%.1f%% of one 2048 page), pages@2048 %d, pages@4096 %d"
                       % (var, len(sizes), texels, 100.0 * texels / PAGE / PAGE, pages, pages4k))
        for n, (p, x, y) in pos.items():
            sx, sy, ss = rects[n]
            rows.append([var, n, p, x, y, sizes[n], sx, sy, ss])
    for res, k in (("1080", 1), ("2160", 2)):
        ref = Image.new("RGB", (1920 * k, 1080 * k), (0, 0, 0))
        for n, (sx, sy, ss) in rects.items():
            ref.paste(raster(spr[n][0], ss * k), (sx * k, sy * k), raster(spr[n][0], ss * k))
        ref.save(os.path.join(OUT, "ref_%s.png" % res))
    gl = Image.new("RGB", (1920, 1080), (0, 0, 0))
    for n, (sx, sy, ss) in rects.items():
        g = godot_like(spr[n][0], svg_native(spr[n][0]), ss)
        gl.paste(g, (sx, sy), g)
    gl.save(os.path.join(OUT, "godot_like_1080.png"))
    with open(os.path.join(OUT, "manifest.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["variant", "name", "page", "x", "y", "size", "screen_x", "screen_y", "screen_size"])
        w.writerows(rows)
    open(os.path.join(OUT, "summary.txt"), "w").write("\n".join(summary) + "\n")
    print("\n".join(summary))


if __name__ == "__main__":
    main()
