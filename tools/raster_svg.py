"""Rasterise the game's SVG art to PNG (docs/07 §2): the input glyphs (phase 5 slice 3b) and the paw cursors (slice 5).

    py tools/raster_svg.py [--group input] [--check]

The SVGs are SourceArt/svg (tools/art/build_art.py writes them from the art generators, phase 6); each group is rasterised
with resvg_py to SourceArt/png/<group>/ (gitignored, regenerated), one PNG per SVG, named by its stem. tools/import_art.py
then imports the PNGs as T_<stem> textures. The field's sprites are not a group here: tools/pack_atlas.py rasterises them
into atlas pages.

The size (docs/07 §2.2, spike S3): 2x the largest size the art is drawn at on a 1080p screen, rounded up to 4. The glyphs are
drawn at most 28 px (InputGlyphs / PromptIcon icon_px 28: the Pounce prompt, the tutorial's GLYPH_PX), so 56 px squares; a
non-square drawing is fitted by its longest edge and centred.

The paw cursors (group `cursor`, input_device.gd PAW_TEXTURES) are drawn at Godot's PAW_PX, 40 px, and SHIP AS PNG FILES: the
engine loads a hardware cursor from a loose file under Content (UGameViewportClient::SetHardwareCursor), so they are written to
Content/Pawpocalypse/UI/Cursors (committed; staged by DefaultGame.ini), not SourceArt/png.

The chrome (group `chrome`, phase 6 slice 5a): the theme's nine-patch art (pawpocalypse_theme.tres StyleBoxTextures), at
the SVG's OWN size (Godot imports them at svg/scale=1.0 and draws the margins 1:1): a 256 x 64 button stays 256 x 64.
PawUi::Chrome (UI/PawUiKit.cpp) holds each one's margins. ui_dock is left out: Godot's dock overrides it (hud.tscn
StyleBoxFlat_panel), so nothing draws it.

--check: re-rasterises into build/ and fails if any PNG differs from its destination.
"""

import argparse
import io
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SVG = ROOT / "SourceArt" / "svg"
CHROME = ("ui_button_normal", "ui_button_hover", "ui_button_pressed", "ui_button_disabled", "ui_panel", "ui_window",
          "ui_bar_track", "ui_bar_fill", "ui_separator", "ui_plate_small", "ui_button_selected")
# Paths under SourceArt/svg (globs): the icons Godot draws.
ICONS = ("ui/ui_tab_*.svg", "ui/ui_icon_*.svg", "ui/ui_quick_*.svg", "ui/ui_menu_*.svg", "ui/ui_tick.svg",
         "ui/achievement_*.svg", "cards/*.svg", "skills/*.svg", "jobs/job_*.svg")
# group -> (the folder under SourceArt/svg, the raster size in px (None: the SVG's own size), the SVGs' pattern or their
# stems, where the PNGs go (None: SourceArt/png))
GROUPS = {
    "input": ("ui/input", 56, "*.svg", None),
    "cursor": ("ui", 40, "ui_cursor_*.svg", "Content/Pawpocalypse/UI/Cursors"),
    "chrome": ("ui", None, CHROME, None),
    "icons": ("", None, ICONS, None),
}


def png_dir(group: str) -> Path:
    dest = GROUPS[group][3]
    return ROOT / dest if dest else ROOT / "SourceArt" / "png" / group
VIEWBOX = re.compile(r'viewBox="\s*([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)\s*"')


def raster(svg_path: Path, size: int):
    import resvg_py
    from PIL import Image
    text = svg_path.read_text(encoding="utf-8")
    m = VIEWBOX.search(text)
    if not m:
        raise SystemExit("raster_svg: %s has no viewBox" % svg_path)
    w, h = float(m.group(3)), float(m.group(4))
    if size is None:  # the SVG's own size, 1:1
        rw, rh = round(w), round(h)
        return Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_path=str(svg_path), width=rw, height=rh)))).convert("RGBA")
    scale = size / max(w, h)
    rw, rh = max(1, round(w * scale)), max(1, round(h * scale))
    img = Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_path=str(svg_path), width=rw, height=rh)))).convert("RGBA")
    if (rw, rh) == (size, size):
        return img
    out = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    out.paste(img, ((size - rw) // 2, (size - rh) // 2))
    return out


def run(group: str, png_out: Path) -> int:
    folder, size, pattern, _ = GROUPS[group]
    src = SVG / folder
    if isinstance(pattern, tuple) and "/" in pattern[0]:
        svgs = [s for glob in pattern for s in sorted(SVG.glob(glob))]
    elif isinstance(pattern, tuple):
        svgs = [src / (stem + ".svg") for stem in pattern]
        missing = [s.name for s in svgs if not s.exists()]
        if missing:
            raise SystemExit("raster_svg: missing under %s: %s" % (src, ", ".join(missing)))
    else:
        svgs = sorted(src.rglob(pattern)) if pattern == "*.svg" else sorted(src.glob(pattern))
    if not svgs:
        raise SystemExit("raster_svg: no SVGs under %s (py tools/art/build_art.py)" % src)
    if png_out.exists():
        shutil.rmtree(png_out)
    png_out.mkdir(parents=True)
    stems = set()
    for s in svgs:
        if s.stem in stems:
            raise SystemExit("raster_svg: two SVGs named %s" % s.stem)
        stems.add(s.stem)
        raster(s, size).save(png_out / (s.stem + ".png"), optimize=False)
    return len(svgs)


def same_tree(a: Path, b: Path, pattern: str) -> list:
    fa = {p.relative_to(a).as_posix(): p for p in a.rglob(pattern)}
    fb = {p.relative_to(b).as_posix(): p for p in b.rglob(pattern)}
    bad = sorted(set(fa) ^ set(fb))
    if pattern.endswith(".png"):
        from PIL import Image
        for k in sorted(set(fa) & set(fb)):
            if Image.open(fa[k]).tobytes() != Image.open(fb[k]).tobytes():
                bad.append(k)
    else:
        bad += [k for k in sorted(set(fa) & set(fb)) if fa[k].read_bytes() != fb[k].read_bytes()]
    return bad


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--group", choices=sorted(GROUPS), action="append")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    bad = []
    for group in args.group or sorted(GROUPS):
        png_out = png_dir(group)
        if args.check:
            work = ROOT / "build" / "raster_check" / group
            n = run(group, work / "png")
            diff = same_tree(png_out, work / "png", "*.png")
            print("%s: %d SVGs, %s" % (group, n, "identical" if not diff else "DIFFERENT: " + ", ".join(diff[:8])))
            bad += diff
        else:
            n = run(group, png_out)
            size = GROUPS[group][1]
            print("%s: %d SVGs -> %s (%s)" % (group, n, png_out.relative_to(ROOT), "%d px" % size if size else "1:1"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
