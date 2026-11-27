"""C3 (playtest 2026-11-26, notes 17 and 20; D-h): the signatures' white edge, a review sheet at game size.

Cut from the field atlas (SourceArt/png/atlas, what the game draws), on each biome's own floor tone, beside the biome's
common for scale, four ways:
  A  today: drawn at sprite_target_px (95.2) with the white glow 1.24x behind, alpha 0.65-1.0 (PawFieldLook.h);
  B  a thinner, fainter glow: 1.10x, half the alpha;
  C  no glow, drawn at 80 px;
  D  no glow, 80 px, a different mark: a small gold sticker ring on the ground under it;
  and the designer's pick (2026-11-27, D31): B's glow in gold (PawTokens::GOLD).
The glow is drawn at its breathing midpoint. Each tile is 1:1 game pixels at 1080p; the sheet is shown at 2x.

Usage:  py tools/art/review_signatures.py        -> build/review/c3_signatures.png
"""
from __future__ import annotations

import csv
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
ATLAS = ROOT / "SourceArt" / "png" / "atlas"
OUT = ROOT / "build" / "review" / "c3_signatures.png"

INK = (42, 33, 49, 255)        # PawTokens INK #2A2131
GOLD = (242, 193, 78, 255)
# (signature cell, common cell, floor tone (PcBackdropTones tier_n base), label)
ROWS = [
    ("enemy_backyard_moth", "enemy_backyard_mouse", (154, 207, 108), "Backyard moth"),
    ("enemy_alley_dog", "enemy_alley_vole", (169, 176, 186), "Alley dog"),
    ("enemy_park_goose", "enemy_park_rat", (124, 196, 106), "Park goose"),
    ("enemy_sewer_crocodile", "enemy_sewer_rat", (111, 143, 136), "Sewer dog (croc art)"),
]
SIG_PX, COMMON_PX = 95.2, 64.4  # Data/EnemyData.csv sprite_target_px
GLOW_MID = (0.65 + 1.0) / 2.0   # PawFieldLook GlowAlphaMin / Max, mid-breath
COLS = [
    ("A  today", dict(px=SIG_PX, glow=1.24, alpha=GLOW_MID, ring=False)),
    ("B  thinner, fainter glow", dict(px=SIG_PX, glow=1.10, alpha=GLOW_MID / 2.0, ring=False)),
    ("C  no glow, 80 px", dict(px=80.0, glow=0.0, alpha=0.0, ring=False)),
    ("D  80 px, a gold ring", dict(px=80.0, glow=0.0, alpha=0.0, ring=True)),
    ("B gold: chosen (D31)", dict(px=SIG_PX, glow=1.10, alpha=GLOW_MID / 2.0, ring=False, colour=(242, 198, 91))),
]
TILE_W, TILE_H = 230, 160
SCALE = 2


def cells() -> dict:
    page = Image.open(ATLAS / "field_0.png").convert("RGBA")
    out = {}
    with open(ATLAS / "field.csv", encoding="utf-8", newline="") as f:
        for r in csv.DictReader(f):
            x, y, w, h = (int(r[k]) for k in ("x", "y", "w", "h"))
            out[r["id"]] = page.crop((x, y, x + w, y + h))
    return out


def scaled(img: Image.Image, px: float) -> Image.Image:
    n = max(1, round(px))
    return img.resize((n, n), Image.LANCZOS)  # the quad is the cell, drawn Size wide (PawRealmView: B.Width = Size)


def white(img: Image.Image, alpha: float, colour=(255, 255, 255)) -> Image.Image:
    a = img.getchannel("A").point(lambda v: int(v * alpha))
    w = Image.new("RGBA", img.size, colour + (0,))
    w.putalpha(a)
    return w


def paste_centre(dst: Image.Image, src: Image.Image, cx: float, cy: float):
    dst.alpha_composite(src, (round(cx - src.width / 2), round(cy - src.height / 2)))


def tile(sig: Image.Image, common: Image.Image, floor, spec) -> Image.Image:
    t = Image.new("RGBA", (TILE_W, TILE_H), floor + (255,))
    cy = TILE_H / 2 + 6
    paste_centre(t, scaled(common, COMMON_PX), 46, cy + 10)
    sx = 140
    body = scaled(sig, spec["px"])
    if spec["ring"]:
        d = ImageDraw.Draw(t)
        rw, rh = spec["px"] * 0.95, spec["px"] * 0.26
        by = cy + spec["px"] * 0.36
        d.ellipse((sx - rw / 2, by - rh / 2, sx + rw / 2, by + rh / 2), fill=GOLD, outline=INK, width=3)
    if spec["glow"] > 0:
        paste_centre(t, white(scaled(sig, spec["px"] * spec["glow"]), spec["alpha"], spec.get("colour", (255, 255, 255))), sx, cy)
    paste_centre(t, body, sx, cy)
    return t


def font(size: int):
    for name in ("arialbd.ttf", "arial.ttf", "DejaVuSans-Bold.ttf"):
        try:
            return ImageFont.truetype(name, size)
        except OSError:
            continue
    return ImageFont.load_default()


def main() -> int:
    c = cells()
    head_h, left_w, gap = 34, 180, 8
    W = left_w + len(COLS) * (TILE_W + gap)
    H = head_h + len(ROWS) * (TILE_H + gap) + 40
    sheet = Image.new("RGBA", (W, H), (246, 238, 222, 255))
    d = ImageDraw.Draw(sheet)
    f, fs = font(15), font(12)
    for i, (name, _spec) in enumerate(COLS):
        d.text((left_w + i * (TILE_W + gap) + 6, 9), name, fill=INK, font=f)
    for j, (sig_id, common_id, floor, label) in enumerate(ROWS):
        y = head_h + j * (TILE_H + gap)
        d.text((8, y + TILE_H / 2 - 9), label, fill=INK, font=f)
        for i, (_name, spec) in enumerate(COLS):
            sheet.alpha_composite(tile(c[sig_id], c[common_id], floor, spec), (left_w + i * (TILE_W + gap), y))
    d.text((8, H - 30), "1:1 at 1080p, shown 2x. Left in each tile: the biome's common (64 px) for scale. "
                        "The glow at mid-breath (alpha 0.83 today).", fill=INK, font=fs)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    sheet.resize((W * SCALE, H * SCALE), Image.NEAREST).convert("RGB").save(OUT)
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
