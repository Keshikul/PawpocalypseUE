"""Rasterises the field's sprites and packs them into atlas pages (docs/07 §2, §2.2; docs/04 §5.1).

    py tools/pack_atlas.py [--check]

Reads SourceArt/svg (tools/art/build_art.py) and Data/*.csv; writes SourceArt/png/atlas/field_<n>.png and field.csv
(gitignored, regenerated; tools/import_art.py --run atlas imports them as T_Atlas_Field_<n> and DA_FieldAtlas).

WHICH SPRITES, AND HOW BIG. Each is rasterised SQUARE at 2x the largest size Godot draws it on a 1080p screen, rounded up to 4
(spike S3). Every drawing is a square SVG fitted by its longest edge, so the drawn size is one number:
  - each enemy texture: the largest EnemyData.sprite_target_px using it, x 1.03 (the breathing pulse, SpriteLiveliness);
  - each boss: its SVG's own size x 0.9 (boss.tscn's Visual scale; boss.gd never refits it); boss_generic is the scene's
    default; boss_add at RADIUS 15 x 1.5 x 2 (boss_add.gd _draw);
  - the cat's stages: Player.SPRITE_TARGET_PX 175 x 1.16 (the hop's crouch widens it by HOP_SQUASH 0.16); the weapons
    (WEAPON_WIDTH_FRACTION 0.405) and the hats (HAT_WIDTH_FRACTION 0.62) of that cat, x 1.16 too;
  - the kitten: Kitten.SPRITE_TARGET_PX 96 x 1.03;
  - the ground shadow: the generated PNG as it is (256 x 96; drawn at most 0.8 x the cat's 175 px);
  - the backdrop props (phase 6 slice 2, biome_backdrop.gd): a corner piece at EDGE_PX 190, the title garden's tree at
    LANDMARK_PX 230 x 1.15 (its largest roll), every other prop at PROP_PX 78 x 1.6 (the scatter's largest roll). Which
    prop plays which part is PcBackdrop's table; EDGE_PROPS / LANDMARK_PROPS below must agree with it, and
    Pawpocalypse.Realm.Look.Atlas checks every prop PcBackdrop can draw is here.
Pages are 2048 px, shelf-packed tallest first; each cell is edge-extruded by 4 px so bilinear and mip sampling at its border
read the sprite, never a neighbour (docs/04 §9).

field.csv: id (the art's stem: enemy_backyard_mouse, player_cat_stage_4, ground_shadow, ...), page, x, y, w, h (the cell in
texels, without the extrusion), u0, v0, u1, v1, native_w, native_h (the size Godot's texture had: the SVG's own width and
height, or the PNG's; the game fits some art by it).

--check: packs again into build/ and fails if a page or the manifest differs.
"""

import argparse
import csv
import io
import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SVG = ROOT / "SourceArt" / "svg"
OUT = ROOT / "SourceArt" / "png" / "atlas"
PAGE = 2048
EXTRUDE = 4

PULSE = 1.03          # SpriteLiveliness.PULSE_AMPLITUDE
CAT_PX = 175.0        # Player.SPRITE_TARGET_PX
CAT_SQUASH = 1.16     # 1 + Player.HOP_SQUASH: the crouch's width
KITTEN_PX = 96.0      # Kitten.SPRITE_TARGET_PX
BOSS_SCALE = 0.9      # boss.tscn Visual.scale
ADD_PX = 15.0 * 1.5 * 2.0  # BossAdd.RADIUS x 1.5, a diameter
WEAPON_FRACTION = 0.405    # Player.WEAPON_WIDTH_FRACTION (the mounted one is smaller)
HAT_FRACTION = 0.62        # Player.HAT_WIDTH_FRACTION (likewise)
PROP_PX = 78.0 * 1.6       # BiomeBackdrop.PROP_PX x the largest scatter scale
EDGE_PX = 190.0            # BiomeBackdrop.EDGE_PX (a border piece is 0.7 of it)
LANDMARK_PX = 230.0 * 1.15 # BiomeBackdrop.LANDMARK_PX x the largest landmark scale
EDGE_PROPS = {"prop_tier1_bush", "prop_tier2_bin", "prop_tier3_tree", "prop_tier4_chimney", "prop_tier5_grate",
              "prop_tier6_fence", "prop_tier7_arch", "prop_tier8_rock", "prop_tier9_shelf", "prop_tier10_chair",
              "prop_tier11_conveyor", "prop_tier12_scoop"}
LANDMARK_PROPS = {"prop_tier3_tree"}


def stem(res_path: str) -> str:
    return res_path.rsplit("/", 1)[-1].rsplit(".", 1)[0]


def svg_size(path: Path) -> float:
    return native(path)[0]


def native(path: Path):
    """The size Godot imports the file at (svg/scale 1.0: the SVG's width and height attributes; a PNG's pixels)."""
    if path.suffix == ".png":
        from PIL import Image
        return tuple(float(v) for v in Image.open(path).size)
    import re
    head = path.read_text(encoding="utf-8")[:400]
    return (float(re.search(r'\swidth="([\d.]+)"', head).group(1)), float(re.search(r'\sheight="([\d.]+)"', head).group(1)))


def ceil4(x: float) -> int:
    return int(math.ceil(x / 4.0) * 4)


def sprites() -> dict:
    """id -> (source file, raster size in px; None = the PNG as it is)."""
    def rows(name):
        return list(csv.DictReader(open(ROOT / "Data" / name, encoding="utf-8")))
    drawn = {}

    def want(s, px):
        drawn[s] = max(drawn.get(s, 0.0), px)
    for r in rows("EnemyData.csv"):
        want(stem(r["texture_path"]), float(r["sprite_target_px"]) * PULSE)
    for s in sorted({stem(r["texture_path"]) for r in rows("BossData.csv")} | {"boss_generic"}):
        want(s, svg_size(SVG / "bosses" / (s + ".svg")) * BOSS_SCALE)
    want("boss_add", ADD_PX)
    for r in rows("EvolutionData.csv"):
        want(stem(r["texture_path"]), CAT_PX * CAT_SQUASH)
    for p in sorted((SVG / "player").glob("player_weapon_stage_*.svg")):
        want(p.stem, CAT_PX * WEAPON_FRACTION * CAT_SQUASH)
    for r in rows("JobData.csv"):
        if r["hat_path"]:
            want(stem(r["hat_path"]), CAT_PX * HAT_FRACTION * CAT_SQUASH)
    want("kitten_basic", KITTEN_PX * PULSE)
    for p in sorted((SVG / "props").glob("prop_*.svg")):
        want(p.stem, LANDMARK_PX if p.stem in LANDMARK_PROPS else EDGE_PX if p.stem in EDGE_PROPS else PROP_PX)
    folders = {"enemy_": "creatures", "boss_": "bosses", "player_": "player", "kitten_": "player", "hat_": "jobs",
               "prop_": "props"}
    out = {}
    for s, px in sorted(drawn.items()):
        folder = next(f for p, f in folders.items() if s.startswith(p))
        src = SVG / folder / (s + ".svg")
        if not src.exists():
            raise SystemExit("pack_atlas: %s is missing (py tools/art/build_art.py)" % src)
        out[s] = (src, ceil4(2.0 * px))
    out["ground_shadow"] = (SVG / "props" / "ground_shadow.png", None)
    return out


def raster(src: Path, size):
    from PIL import Image
    if size is None:
        return Image.open(src).convert("RGBA")
    import resvg_py
    return Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_path=str(src), width=size, height=size)))).convert("RGBA")


def extrude(img, e):
    """Copies the border pixels outward by e px (spike S3)."""
    from PIL import Image
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


def pack(images: dict) -> dict:
    """Shelf packing, tallest first (then by id, so the result is stable). Returns id -> (page, x, y)."""
    order = sorted(images, key=lambda n: (-images[n].height, -images[n].width, n))
    pos, page, x, y, shelf = {}, 0, 0, 0, 0
    for n in order:
        w, h = images[n].width + 2 * EXTRUDE, images[n].height + 2 * EXTRUDE
        if w > PAGE or h > PAGE:
            raise SystemExit("pack_atlas: %s is larger than a page" % n)
        if x + w > PAGE:
            x, y, shelf = 0, y + shelf, 0
        if y + h > PAGE:
            page, x, y, shelf = page + 1, 0, 0, 0
        pos[n] = (page, x + EXTRUDE, y + EXTRUDE)
        x += w
        shelf = max(shelf, h)
    return pos


def build(out: Path) -> int:
    from PIL import Image
    spr = sprites()
    images = {n: raster(src, size) for n, (src, size) in spr.items()}
    pos = pack(images)
    pages = [Image.new("RGBA", (PAGE, PAGE), (0, 0, 0, 0)) for _ in range(max(p for p, _, _ in pos.values()) + 1)]
    for n, (p, x, y) in pos.items():
        pages[p].paste(extrude(images[n], EXTRUDE), (x - EXTRUDE, y - EXTRUDE))
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob("field_*.png"):
        old.unlink()
    for i, img in enumerate(pages):
        img.save(out / ("field_%d.png" % i), optimize=False)
    with open(out / "field.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["id", "page", "x", "y", "w", "h", "u0", "v0", "u1", "v1", "native_w", "native_h"])
        for n in sorted(pos):
            p, x, y = pos[n]
            iw, ih = images[n].size
            w.writerow([n, p, x, y, iw, ih, "%.8f" % (x / PAGE), "%.8f" % (y / PAGE), "%.8f" % ((x + iw) / PAGE),
                "%.8f" % ((y + ih) / PAGE), "%g" % native(spr[n][0])[0], "%g" % native(spr[n][0])[1]])
    texels = sum(i.width * i.height for i in images.values())
    print("pack_atlas: %d sprites, %d pages of %d (%.0f%% filled)" % (len(images), len(pages), PAGE,
        100.0 * texels / (len(pages) * PAGE * PAGE)))
    return len(pages)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    if not args.check:
        build(OUT)
        return 0
    from PIL import Image
    work = ROOT / "build" / "atlas_check"
    build(work)
    bad = []
    names = sorted(p.name for p in work.iterdir())
    if names != sorted(p.name for p in OUT.iterdir() if p.name.startswith("field")):
        bad.append("the files differ: %s" % names)
    for n in names:
        a, b = work / n, OUT / n
        if not b.exists():
            continue
        same = (Image.open(a).tobytes() == Image.open(b).tobytes()) if n.endswith(".png") else a.read_bytes() == b.read_bytes()
        if not same:
            bad.append(n)
    print("pack_atlas --check: %s" % ("identical" if not bad else "DIFFERENT: " + ", ".join(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
