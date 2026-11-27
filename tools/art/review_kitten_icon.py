"""C6 (D-l): an icon for Extra Kittens (Vitality), a heart today, as a review sheet.

Options, composed from the game's own icons (SourceArt/png/icons, what the game draws):
  A  today: the heart (skill_vitality);
  B  Kitten Count's kitten face as it is (the shop line that also gives kittens);
  C  the kitten face with a small Stardust "+" sticker (the tree's currency: a kitten the tree gives);
  D  two kitten faces, a family (the achievement "Big Family" would follow).
Each on a tree node (the dark die-cut plate, 84 px, the Compass's) and at the dock row's 40 px on cream.

Usage:  py tools/art/review_kitten_icon.py        -> build/review/c6_kitten_icon.png
"""
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
ICONS = ROOT / "SourceArt" / "png" / "icons"
OUT = ROOT / "build" / "review" / "c6_kitten_icon.png"

INK = (42, 33, 49, 255)            # #2A2131
NODE = (30, 24, 38, 255)           # PawTreeTables NodeDark #1E1826
RIM = (255, 253, 246, 235)         # NodeRim
SKY = (52, 42, 62, 255)            # the tree's background, near it
CREAM = (246, 238, 222, 255)
STARDUST = (185, 140, 255, 255)    # palette "stardust" #B98CFF


def icon(name: str) -> Image.Image:
    return Image.open(ICONS / f"{name}.png").convert("RGBA")


def plus_badge(size: int) -> Image.Image:
    b = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(b)
    d.ellipse((2, 2, size - 3, size - 3), fill=STARDUST, outline=INK, width=max(3, size // 9))
    t, m = max(3, size // 7), size // 2
    arm = size * 0.26
    d.rectangle((m - arm, m - t / 2, m + arm, m + t / 2), fill=(255, 253, 246, 255))
    d.rectangle((m - t / 2, m - arm, m + t / 2, m + arm), fill=(255, 253, 246, 255))
    return b


def option_c() -> Image.Image:
    base = icon("ui_icon_upgrade_kitten_count").copy()
    base.alpha_composite(plus_badge(52), (128 - 56, 128 - 58))
    return base


def option_d() -> Image.Image:
    k = icon("ui_icon_upgrade_kitten_count")
    out = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    out.alpha_composite(k.resize((92, 92), Image.LANCZOS), (34, 6))
    out.alpha_composite(k.resize((84, 84), Image.LANCZOS), (2, 42))
    return out


OPTIONS = [
    ("A  today (heart)", lambda: icon("skill_vitality")),
    ("B  the shop's kitten", lambda: icon("ui_icon_upgrade_kitten_count")),
    ("C  kitten + Stardust", option_c),
    ("D  two kittens", option_d),
]


def node(img: Image.Image, px: int = 84) -> Image.Image:
    pad = 14
    t = Image.new("RGBA", (px + 2 * pad, px + 2 * pad), SKY)
    d = ImageDraw.Draw(t)
    d.ellipse((pad + 4, pad + 6, pad + px + 4, pad + px + 6), fill=(4, 2, 6, 217))      # hard shadow
    d.ellipse((pad - 3, pad - 3, pad + px + 3, pad + px + 3), fill=RIM)                  # die-cut rim
    d.ellipse((pad, pad, pad + px, pad + px), fill=NODE, outline=INK, width=4)
    ic = img.resize((int(px * 0.66), int(px * 0.66)), Image.LANCZOS)
    t.alpha_composite(ic, ((t.width - ic.width) // 2, (t.height - ic.height) // 2))
    return t


def row(img: Image.Image, px: int = 40) -> Image.Image:
    t = Image.new("RGBA", (px + 16, px + 16), CREAM)
    t.alpha_composite(img.resize((px, px), Image.LANCZOS), (8, 8))
    return t


def main() -> int:
    try:
        f = ImageFont.truetype("arialbd.ttf", 14)
    except OSError:
        f = ImageFont.load_default()
    colw, W, H = 190, 190 * len(OPTIONS) + 20, 330
    sheet = Image.new("RGBA", (W, H), CREAM)
    d = ImageDraw.Draw(sheet)
    for i, (label, make) in enumerate(OPTIONS):
        x = 10 + i * colw
        img = make()
        d.text((x + 6, 10), label, fill=INK, font=f)
        sheet.alpha_composite(img, (x + 30, 36))
        sheet.alpha_composite(node(img), (x + 10, 176))
        sheet.alpha_composite(row(img), (x + 130, 196))
    d.text((10, H - 22), "Top: 128 px. Below: on a tree node (84 px, dark plate) and in a dock row (40 px).", fill=INK, font=f)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    sheet.resize((W * 2, H * 2), Image.LANCZOS).convert("RGB").save(OUT)
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
