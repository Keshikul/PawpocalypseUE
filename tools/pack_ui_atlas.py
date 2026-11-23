"""The UI's atlas (phase 8, draw calls; HANDOFF-LOG (139)): Godot's 128 px icons (tools/raster_svg.py group `icons`,
SourceArt/png/icons) and the 56 px input glyphs (group `input`, SourceArt/png/input: the prompts' buttons and keys) packed
and the chrome plates (group `chrome`: the nine-patch buttons, panels, bars) onto one 2048 px page, so the HUD's icons,
glyphs and plates are one texture and Slate draws them in one batch.

    py tools/pack_ui_atlas.py           # SourceArt/png/ui_atlas/ui_<n>.png + ui.csv
    py tools/pack_ui_atlas.py --check   # a fresh pack into build/ui_atlas_check equals SourceArt/png/ui_atlas (exit 1 if not)

Why: Slate batches by texture. Every icon its own UTexture2D made every icon on the HUD its own draw call (11 upgrade rows, 6
tabs, 4 quick buttons, the currencies: Slate.ShowBatching and ProfileGPU on the package, 2026-11-23: the UI was 61 of the
frame's 84 draw calls). A brush whose resource is an atlas region (ISlateTextureAtlasInterface: UPawUiCell) shares its
page's shader resource, so every icon on the page batches with the others in its layer.

Only the 128 px icons go in (the HUD's, the cards', the tree's, the achievements') and the glyphs; the pictures drawn larger
(the title's cat and mouse, the save cards' and job picker's cats, the ceremony's rays and cat) stay their own textures
(DA_Icons.Textures). Each cell is the PNG 1:1 (the UI draws without mips), edge-extruded by 2 px so bilinear sampling at its
border reads the picture, never a neighbour; skyline-packed tallest first. tools/import_art.py --run icons imports the page as
T_Atlas_UI_<n> (the UI's texture settings) and every cell into DA_Icons (AtlasPages, AtlasNames, AtlasCellPage, AtlasUV);
PawUi::IconBrush and the prompts (PawGlyphs) draw a cell when the picture has one.

ui.csv: id (the PNG's stem), page, x, y, w, h (the cell in page pixels, without the extrusion), u0, v0, u1, v1.
"""
import argparse
import csv
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "SourceArt" / "png" / "icons"
GLYPHS = ROOT / "SourceArt" / "png" / "input"
CHROME = ROOT / "SourceArt" / "png" / "chrome"
OUT = ROOT / "SourceArt" / "png" / "ui_atlas"
PAGE = 2048
EXTRUDE = 2
ICON_PX = 128
GLYPH_PX = 56


def icons():
    """Every picture that goes in: the 128 px icons, the 56 px glyphs and the chrome plates (any size), by stem."""
    from PIL import Image
    out = {}
    for src, px in ((SRC, ICON_PX), (GLYPHS, GLYPH_PX), (CHROME, None)):
        for p in sorted(src.glob("*.png")):
            img = Image.open(p).convert("RGBA")
            if px is None or img.size == (px, px):
                if p.stem in out:
                    raise SystemExit("pack_ui_atlas: two pictures named %s" % p.stem)
                out[p.stem] = img
    return out


def extrude(img, e):
    """Copies the border pixels outward by e px (as tools/pack_atlas.py)."""
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
    """Skyline packing (bottom-left: each picture where its top is lowest, then leftmost), tallest first, then by name
    (stable). Denser than shelves for mixed sizes: the plates, icons and glyphs fit one page. Returns id -> (page, x, y)."""
    order = sorted(images, key=lambda n: (-images[n].height, -images[n].width, n))
    pos, page = {}, 0
    sky = [(0, PAGE, 0)]  # (x, width, y): the top of what is placed, left to right

    def fit(w, h):
        best = None
        for i, (x, _, _) in enumerate(sky):
            if x + w > PAGE:
                break
            top, span, j = 0, 0, i
            while span < w:
                top = max(top, sky[j][2])
                span += sky[j][1]
                j += 1
            if top + h <= PAGE and (best is None or (top, x) < (best[0], best[1])):
                best = (top, x)
        return best

    for n in order:
        w, h = images[n].width + 2 * EXTRUDE, images[n].height + 2 * EXTRUDE
        at = fit(w, h)
        if at is None:
            page, sky = page + 1, [(0, PAGE, 0)]
            at = fit(w, h)
        y, x = at
        pos[n] = (page, x + EXTRUDE, y + EXTRUDE)
        # The skyline: [x, x + w) now at y + h; segments under it cut away.
        out = []
        for sx, sw, sy in sky:
            for a, b in ((sx, min(sx + sw, x)), (max(sx, x + w), sx + sw)):
                if b > a:
                    out.append((a, b - a, sy))
        out.append((x, w, y + h))
        out.sort()
        merged = []
        for seg in out:
            if merged and merged[-1][2] == seg[2] and merged[-1][0] + merged[-1][1] == seg[0]:
                merged[-1] = (merged[-1][0], merged[-1][1] + seg[1], seg[2])
            else:
                merged.append(seg)
        sky = merged
    return pos


def build(out: Path) -> int:
    from PIL import Image
    images = icons()
    if not images:
        raise SystemExit("pack_ui_atlas: no icons in %s (py tools/raster_svg.py first)" % SRC)
    pos = pack(images)
    pages = [Image.new("RGBA", (PAGE, PAGE), (0, 0, 0, 0)) for _ in range(max(p for p, _, _ in pos.values()) + 1)]
    for n, (p, x, y) in pos.items():
        pages[p].paste(extrude(images[n], EXTRUDE), (x - EXTRUDE, y - EXTRUDE))
    out.mkdir(parents=True, exist_ok=True)
    for old in out.glob("ui_*.png"):
        old.unlink()
    for i, img in enumerate(pages):
        img.save(out / ("ui_%d.png" % i), optimize=False)
    with open(out / "ui.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["id", "page", "x", "y", "w", "h", "u0", "v0", "u1", "v1"])
        for n in sorted(pos):
            p, x, y = pos[n]
            iw, ih = images[n].size
            w.writerow([n, p, x, y, iw, ih, "%.8f" % (x / PAGE), "%.8f" % (y / PAGE),
                        "%.8f" % ((x + iw) / PAGE), "%.8f" % ((y + ih) / PAGE)])
    chrome = sum(1 for p in CHROME.glob("*.png"))
    glyphs = sum(1 for n, i in images.items() if n.startswith("input_"))
    print("pack_ui_atlas: %d icons, %d glyphs and %d chrome plates, %d page(s) of %d" % (len(images) - glyphs - chrome, glyphs,
                                                                                     chrome, len(pages), PAGE))
    return len(pages)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    if not args.check:
        build(OUT)
        return 0
    from PIL import Image
    work = ROOT / "build" / "ui_atlas_check"
    for old in work.glob("*") if work.exists() else []:
        old.unlink()
    build(work)
    bad = []
    names = sorted(p.name for p in work.iterdir())
    have = sorted(p.name for p in OUT.iterdir()) if OUT.exists() else []
    if names != have:
        bad.append("the files differ: %s vs %s" % (names, have))
    for n in names:
        a, b = work / n, OUT / n
        if not b.exists():
            continue
        same = (Image.open(a).tobytes() == Image.open(b).tobytes()) if n.endswith(".png") else a.read_bytes() == b.read_bytes()
        if not same:
            bad.append(n)
    print("pack_ui_atlas --check: %s" % ("identical" if not bad else "DIFFERENT: " + ", ".join(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
