"""The Steam store, library and event images from the key art's layers (designer, 2026-11-25: "This is the logo that i
want. It's from back in godot. Also the capsule layout and colors. Can you change everything according to that?").

    py tools/store_art.py [--src DIR] [--out DIR] [--psd]

THE ART. The key art is the first try's (2026-11-03): Gemini drew the full 16:9 capsule, a square and a 9:16 version of
it, the background with no title or character, and the character on green screen (keyed to a transparent layer); the
logo is cut out of the full capsule. No font: the lettering is drawn. AI-generated: the Steam page's AI disclosure says
so. The layers live in Drive (`Promotional/Steam/store-images/key-art/source`, PNG; --src):
  capsule_16x9.png 2720x1568, square_1x1.png 2048, portrait_9x16.png 1536x2752 (the composed pictures),
  background.png 3840x2160 and character.png 4096 RGBA (upscaled 1.4x and 2x), logo.png 3840x576 RGBA.

THE LAYOUT is the capsule's: the logo across the top with a paw print at each end, the cat on the unicorn in the middle
on its rainbow, the hooded crowd on the left and the armed kittens on the right, the purple sky over the green field.
Where Steam's shape is near one of the composed pictures, it is cut from that picture (the main capsule and the event
cover from the 16:9, the vertical and library capsules from the 9:16, the icons from the square); the wide and small
shapes are put together from the layers in the same arrangement. The small capsule shows the lettering without the paw
prints, so the name nearly fills it (Steam: readable at 120x45). The library hero has no text (Steam's rule); the
library logo is the logo alone, transparent.

Out (default build/steam/store_art): upload/<name>.png at Steam's sizes (the 13 of the 2026-11-18 set), checks/ (the
small capsule at 120x45 and 184x69, a contact sheet of the set). PNG only (designer, 2026-11-25).
"""

import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
from store_psd import Layers, psd_matches, save_psd  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
SRC = Path(r"G:\My Drive\Games\Pawpocalypse\Promotional\Steam\store-images\key-art\source")
OUT = ROOT / "build" / "steam" / "store_art"

SIZES = {
    "main_capsule": (1232, 706), "header_capsule": (920, 430), "library_header": (920, 430),
    "small_capsule": (462, 174), "vertical_capsule": (748, 896), "library_capsule": (600, 900),
    "library_hero": (3840, 1240), "library_logo": (1280, 192), "page_background": (1438, 810),
    "event_cover": (800, 450), "event_header": (1920, 622), "app_icon": (184, 184), "shortcut_icon": (256, 256),
}

# In the layers' own pixels. The character's figure: the cat's ear tips at y 927, the unicorn's hooves at about y 3686,
# across x 524-3755 (its glow and sparkle trail run to the bottom). The logo: a paw print 0-280, the lettering 316-3524,
# a paw print 3564-3840.
FIG_TOP, FIG_FEET, FIG_CX = 927.0, 3686.0, 2140.0
LETTERS = (300, 0, 3540, 576)


def cover(img, w, h, cx=0.5, cy=0.5):
    """img cut to w:h around (cx, cy) as fractions (clamped inside it), scaled to w x h."""
    iw, ih = img.size
    if iw / ih > w / h:
        cw, ch = ih * w / h, ih
    else:
        cw, ch = iw, iw * h / w
    x0 = min(max(cx * iw - cw / 2, 0), iw - cw)
    y0 = min(max(cy * ih - ch / 2, 0), ih - ch)
    return img.crop((round(x0), round(y0), round(x0 + cw), round(y0 + ch))).resize((w, h), Image.LANCZOS)


def put(canvas, layer, x, y, width):
    """layer scaled to width, its top-left at (x, y) (may hang off the canvas)."""
    k = width / layer.width
    scaled = layer.resize((max(1, round(layer.width * k)), max(1, round(layer.height * k))), Image.LANCZOS)
    x, y = round(x), round(y)
    box = (max(0, -x), max(0, -y), min(scaled.width, canvas.width - x), min(scaled.height, canvas.height - y))
    if box[0] < box[2] and box[1] < box[3]:
        canvas.alpha_composite(scaled.crop(box), (max(0, x), max(0, y)))


def logo_at(canvas, logo, cx, top, width):
    put(canvas, logo, cx - width / 2, top, width)


def rider_at(canvas, char, cx, top, feet):
    """The cat on the unicorn with its ear tips at y top and the hooves at y feet, centred on x cx."""
    k = (feet - top) / (FIG_FEET - FIG_TOP)
    put(canvas, char, cx - FIG_CX * k, top - FIG_TOP * k, char.width * k)


def composed(bg, char, logo, w, h, logo_w, logo_top, fig_top, fig_feet, cx=None, bg_cy=0.5):
    """The capsule's arrangement on a w x h canvas, as layers (the PSD's): the background, the rider, the logo on top."""
    L = Layers(w, h)
    L.add("background", cover(bg, w, h, cy=bg_cy))
    cx = w / 2 if cx is None else cx
    rider_at(L.new("cat on the unicorn"), char, cx, fig_top, fig_feet)
    if logo is not None:
        logo_at(L.new("logo"), logo, w / 2, logo_top, logo_w)
    return L


def build(src):
    cap = Image.open(src / "capsule_16x9.png").convert("RGB")
    sq = Image.open(src / "square_1x1.png").convert("RGB")
    port = Image.open(src / "portrait_9x16.png").convert("RGB")
    bg = Image.open(src / "background.png").convert("RGB")
    char = Image.open(src / "character.png").convert("RGBA")
    logo = Image.open(src / "logo.png").convert("RGBA")
    letters = logo.crop(LETTERS)
    out = {}
    # Cut from the composed pictures.
    # (these are one picture each: Gemini drew them whole, so their PSD has one layer)
    out["main_capsule"] = Layers.single("key art (16:9)", cover(cap, *SIZES["main_capsule"]))
    out["event_cover"] = Layers.single("key art (16:9)", cover(cap, *SIZES["event_cover"]))
    out["vertical_capsule"] = Layers.single("key art (9:16)", cover(port, *SIZES["vertical_capsule"], cy=0.355))
    out["library_capsule"] = Layers.single("key art (9:16)", cover(port, *SIZES["library_capsule"], cy=0.44))
    # The icons: the cat's face, from the square.
    face = sq.crop((470, 380, 1330, 1240))
    out["app_icon"] = Layers.single("key art (square)", face.resize(SIZES["app_icon"], Image.LANCZOS))
    out["shortcut_icon"] = Layers.single("key art (square)", face.resize(SIZES["shortcut_icon"], Image.LANCZOS))
    # Put together from the layers, in the capsule's arrangement.
    for name in ("header_capsule", "library_header"):
        out[name] = composed(bg, char, logo, 920, 430, logo_w=610, logo_top=12, fig_top=96, fig_feet=452, bg_cy=0.5)
    out["event_header"] = composed(bg, char, logo, 1920, 622, logo_w=1040, logo_top=22, fig_top=168, fig_feet=690)
    small = Layers(*SIZES["small_capsule"])
    small.add("background", cover(bg, *SIZES["small_capsule"], cy=0.42))
    small.add("dim", Image.new("RGBA", small.size, (40, 20, 50, 70)))
    logo_at(small.new("logo lettering"), letters, 231, 87 - 0.5 * 438 * letters.height / letters.width, 438)
    out["small_capsule"] = small
    hero = composed(bg, char, None, 3840, 1240, 0, 0, fig_top=90, fig_feet=1190, bg_cy=0.52)
    out["library_hero"] = hero
    out["library_logo"] = Layers.single("logo", logo.resize(SIZES["library_logo"], Image.LANCZOS))
    page = cover(bg, *SIZES["page_background"]).filter(ImageFilter.GaussianBlur(4))
    out["page_background"] = Layers.single("background (blurred, dimmed)", ImageEnhance.Brightness(page).enhance(0.45))
    return out


def checks(images, out):
    d = out / "checks"
    d.mkdir(parents=True, exist_ok=True)
    small = images["small_capsule"].convert("RGB")
    for w, h in ((120, 45), (184, 69)):
        small.resize((w, h), Image.LANCZOS).resize((w * 4, h * 4), Image.NEAREST).save(d / ("small_capsule_%dx%d.png" % (w, h)))
    # A contact sheet: every image scaled into a 420 px tall row, transparent ones over a mid grey.
    tiles = []
    for name, im in images.items():
        t = im.convert("RGBA")
        k = min(420 / t.height, 900 / t.width)
        t = t.resize((max(1, round(t.width * k)), max(1, round(t.height * k))), Image.LANCZOS)
        tile = Image.new("RGBA", (t.width, t.height + 28), (90, 90, 90, 255))
        tile.alpha_composite(t, (0, 28))
        ImageDraw.Draw(tile).text((6, 6), "%s %dx%d" % (name, *im.size), fill=(255, 255, 255, 255))
        tiles.append(tile)
    rows, row, width = [], [], 0
    for t in tiles:
        if row and width + t.width > 2400:
            rows.append(row)
            row, width = [], 0
        row.append(t)
        width += t.width + 12
    rows.append(row)
    sheet = Image.new("RGB", (2400, sum(max(t.height for t in r) + 12 for r in rows)), (30, 30, 30))
    y = 0
    for r in rows:
        x = 0
        for t in r:
            sheet.paste(t.convert("RGB"), (x, y))
            x += t.width + 12
        y += max(t.height for t in r) + 12
    sheet.save(d / "sheet.png")


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--src", type=Path, default=SRC)
    ap.add_argument("--out", type=Path, default=OUT)
    ap.add_argument("--psd", action="store_true", help="also a layered PSD of each image (out/psd; needs psd-tools)")
    args = ap.parse_args(argv)
    stacks = build(args.src)
    up = args.out / "upload"
    up.mkdir(parents=True, exist_ok=True)
    images = {}
    for name, L in stacks.items():
        im = L.flat()
        want = SIZES[name]
        if im.size != want:
            raise SystemExit("store_art: %s is %s, Steam wants %s" % (name, im.size, want))
        keep_alpha = name == "library_logo"
        (im if keep_alpha else im.convert("RGB")).save(up / (name + ".png"))
        images[name] = im
        if args.psd:
            path = args.out / "psd" / (name + ".psd")
            save_psd(path, L)
            ok, diff = psd_matches(path, im)
            print("  %-17s %2d layers  PSD %s (mean difference %.2f)" % (name, len(L.layers), "ok" if ok else "DIFFERS", diff))
            if not ok:
                return 1
    checks(images, args.out)
    print("store_art: %d images -> %s%s" % (len(images), up, " (+ psd)" if args.psd else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
