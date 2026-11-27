"""C5 (playtest 2026-11-26, notes 33 and 42; D-c, D-n): the boss's size and the Summon button, a review sheet.

Top: the Park Tyrant beside the cat (stage 2, CatPx 175) on the Park's floor at BossScale 1.4 (today, 358 px), 1.8 and
2.2, cut from the field atlas (what the game draws), 1:1 at 1080p, the sheet shown at half size.
Bottom: a mock of the Summon button at the 480 px dock's width (Sticker Box: cream card, ink edge, hard ink shadow),
in three states: not strong enough yet, strong enough, beaten. Its words are a DRAFT (none of them is approved).

Usage:  py tools/art/review_boss_summon.py        -> build/review/c5_boss_summon.png
"""
from __future__ import annotations

import csv
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
ATLAS = ROOT / "SourceArt" / "png" / "atlas"
FONTS = ROOT / "SourceArt" / "fonts"
OUT = ROOT / "build" / "review" / "c5_boss_summon.png"

INK = (42, 33, 49, 255)
PANEL = (255, 248, 235, 255)       # UI_PANEL
BUTTON = (244, 231, 207, 255)      # UI_BUTTON
MUTED = (110, 98, 118, 255)        # UI_TEXT_MUTED
DANGER = (201, 51, 35, 255)        # UI_DANGER
GOOD = (78, 154, 46, 255)          # UI_GOOD
YELLOW = (242, 198, 91, 255)       # YELLOW_HEADER
PARK = (124, 196, 106, 255)        # PcBackdropTones tier_3
CAT_PX, BOSS_NATIVE = 175, 256
SCALES = (1.4, 1.8, 2.2)


def cells() -> dict:
    page = Image.open(ATLAS / "field_0.png").convert("RGBA")
    out = {}
    with open(ATLAS / "field.csv", encoding="utf-8", newline="") as f:
        for r in csv.DictReader(f):
            x, y, w, h = (int(r[k]) for k in ("x", "y", "w", "h"))
            out[r["id"]] = page.crop((x, y, x + w, y + h))
    return out


def font(name: str, size: int):
    try:
        return ImageFont.truetype(str(FONTS / name), size)
    except OSError:
        return ImageFont.load_default()


def sizes_panel(c: dict) -> Image.Image:
    cat = c["player_cat_stage_2"].resize((CAT_PX, CAT_PX), Image.LANCZOS)
    boss = c["boss_park_tyrant"]
    panels = []
    for k in SCALES:
        px = round(BOSS_NATIVE * k)
        p = Image.new("RGBA", (CAT_PX + px + 90, 600), PARK)
        ground = 560
        p.alpha_composite(cat, (30, ground - CAT_PX))
        p.alpha_composite(boss.resize((px, px), Image.LANCZOS), (CAT_PX + 60, ground - px))
        d = ImageDraw.Draw(p)
        d.text((16, 12), f"x{k}  ({px} px){'  today' if k == 1.4 else ''}", fill=INK, font=font("Nunito-Black.ttf", 30))
        panels.append(p)
    W = sum(p.width for p in panels) + 20 * (len(panels) - 1)
    out = Image.new("RGBA", (W, 600), PANEL)
    x = 0
    for p in panels:
        out.alpha_composite(p, (x, 0))
        x += p.width + 20
    return out


def portrait(c: dict, px: int) -> Image.Image:
    S = int(px / 0.32)  # the head is about a third of the drawing wide, left of centre
    boss = c["boss_park_tyrant"].resize((S, S), Image.LANCZOS)
    cx, cy = int(S * 0.24), int(S * 0.70)
    face = boss.crop((cx - px // 2, cy - px // 2, cx - px // 2 + px, cy - px // 2 + px))
    mask = Image.new("L", (px, px), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, px - 1, px - 1), fill=255)
    out = Image.new("RGBA", (px, px), (255, 236, 214, 255))
    out.alpha_composite(face)
    out.putalpha(mask)
    ring = Image.new("RGBA", (px, px), (0, 0, 0, 0))
    ImageDraw.Draw(ring).ellipse((1, 1, px - 2, px - 2), outline=INK, width=4)
    out.alpha_composite(ring)
    return out


def button(c: dict, state: str) -> Image.Image:
    W, H = 432, 118
    img = Image.new("RGBA", (W + 10, H + 10), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    face = {"weak": BUTTON, "ready": (255, 214, 196, 255), "beaten": (226, 220, 210, 255)}[state]
    edge = DANGER if state == "ready" else INK
    d.rounded_rectangle((6, 7, W + 6, H + 7), 16, fill=INK)                         # hard ink shadow
    d.rounded_rectangle((0, 0, W, H), 16, fill=face, outline=edge, width=4)
    img.alpha_composite(portrait(c, 84), (14, 17))
    big, small = font("Nunito-Black.ttf", 26), font("Nunito-Bold.ttf", 17)
    title = {"weak": "Summon Park Tyrant", "ready": "Summon Park Tyrant", "beaten": "Park Tyrant beaten"}[state]
    d.text((112, 14), title, fill=INK, font=big)
    if state == "beaten":
        d.text((112, 56), "Gone for good. Next: Rival Champion.", fill=MUTED, font=small)
        return img
    share = 0.42 if state == "weak" else 1.18
    bx0, bx1, by = 112, W - 20, 62
    d.rounded_rectangle((bx0, by, bx1, by + 18), 9, fill=(255, 255, 255, 255), outline=INK, width=3)
    fill_to = bx0 + (bx1 - bx0) * min(share, 1.0)
    d.rounded_rectangle((bx0 + 3, by + 3, max(bx0 + 12, fill_to - 3), by + 15), 6, fill=GOOD if share >= 1 else YELLOW)
    words = (f"Your damage: {share:.0%} of what it takes" if share < 1 else f"Strong enough ({share:.0%}). 120 s to win.")
    d.text((112, 86), words, fill=INK if share >= 1 else MUTED, font=small)
    return img


def main() -> int:
    c = cells()
    top = sizes_panel(c)
    top = top.resize((top.width // 2, top.height // 2), Image.LANCZOS)
    btns = [button(c, s) for s in ("weak", "ready", "beaten")]
    labels = ["Not strong enough yet", "Strong enough", "Beaten"]
    W = max(top.width, 3 * 460 + 20)
    H = top.height + 60 + 190 + 40
    sheet = Image.new("RGBA", (W, H), PANEL)
    d = ImageDraw.Draw(sheet)
    sheet.alpha_composite(top, (0, 0))
    y = top.height + 20
    d.text((10, y), "The Summon button, 480 px dock (1:1). Words are a draft.", fill=INK, font=font("Nunito-Black.ttf", 20))
    for i, (b, lab) in enumerate(zip(btns, labels)):
        x = 10 + i * 460
        d.text((x, y + 36), lab, fill=MUTED, font=font("Nunito-Bold.ttf", 16))
        sheet.alpha_composite(b, (x, y + 62))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    sheet.convert("RGB").save(OUT)
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
