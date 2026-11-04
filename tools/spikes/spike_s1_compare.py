"""SPIKE S1 (docs/04 §8): compare the runs in Saved/Spikes/S1. Throwaway.

For every <run>.png/.txt pair: the colour verdict and timings from the .txt, plus 4x nearest-neighbour crops of one static
sprite (edges at rest) and of the moving band (ghosting), laid out in Saved/Spikes/S1/compare.png.

  py tools/spikes/spike_s1_compare.py [run ...]     # default: every run found
"""
import pathlib
import re
import sys

from PIL import Image, ImageDraw

DIR = pathlib.Path(__file__).resolve().parents[2] / "Saved" / "Spikes" / "S1"
ZOOM = 4
# Field-pixel boxes (1920x1080 field = viewport at the S1 resolution): a static sprite in the bottom grid, and a slice
# of the band where the movers fly.
STATIC_BOX = (120, 770, 280, 890)
MOVER_BOX = (700, 380, 900, 520)


def edge_blend(img, k, ox):
    """Static grid: share of sprite/background boundary pixels that are blends (neither ~black nor full sprite colour)."""
    im = img.crop((int(ox + 120 * k), int(770 * k), int(ox + 1800 * k), int(1060 * k)))
    px = im.load()
    edge = blend = 0
    for y in range(1, im.height - 1):
        for x in range(1, im.width - 1):
            nb = [max(px[x + 1, y]), max(px[x - 1, y]), max(px[x, y + 1]), max(px[x, y - 1])]
            if min(nb) < 8 and max(nb) > 60:
                edge += 1
                if 8 <= max(px[x, y]) <= 0.85 * max(nb):
                    blend += 1
    return 100.0 * blend / max(edge, 1)


def faint_share(img, k, ox):
    """Mover band: share of lit pixels that are faint (max channel 8..60). Smear and ghost trails are faint pixels."""
    im = img.crop((int(ox + 200 * k), int(220 * k), int(ox + 1720 * k), int(700 * k)))
    faint = lit = 0
    for c in im.getdata():
        m = max(c)
        if m >= 8:
            lit += 1
            faint += 1 if m <= 60 else 0
    return 100.0 * faint / max(lit, 1)


def main():
    # Each argument is <run>_<capture|screen>; offscreen runs only have a valid _capture, windowed runs a _screen.
    runs = sys.argv[1:]
    rows = []
    for run in runs:
        txt = (DIR / (run + ".txt")).read_text().splitlines()
        head = dict(re.findall(r"(\w+)=([\w.x]+)", txt[0]))
        colour = txt[1]
        print("%-12s %s | frame %s ms (max %s) | gpu %s ms" % (run, colour, head.get("frame_ms_avg"), head.get("frame_ms_max"), head.get("gpu_ms_avg")))
        img = Image.open(DIR / (run + ".png")).convert("RGB")
        k = img.height / 1080.0
        ox = img.width / 2.0 - 960.0 * k  # field height = viewport height; width centred (16:10 shows less width)

        def crop(box):
            b = (int(ox + box[0] * k), int(box[1] * k), int(ox + box[2] * k), int(box[3] * k))
            c = img.crop(b)
            return c.resize((c.width * ZOOM, c.height * ZOOM), Image.NEAREST)

        print("%-12s edges at rest: %.1f%% blended | movers: %.1f%% faint (smear)" % (run, edge_blend(img, k, ox), faint_share(img, k, ox)))
        rows.append((run, colour.split(" -> ")[-1], crop(STATIC_BOX), crop(MOVER_BOX)))

    if not rows:
        return
    label_h = 28
    w = max(r[2].width for r in rows) + max(r[3].width for r in rows) + 30
    h = sum(max(r[2].height, r[3].height) + label_h + 10 for r in rows)
    sheet = Image.new("RGB", (w, h), (255, 255, 255))
    draw = ImageDraw.Draw(sheet)
    y = 0
    for run, verdict, a, b in rows:
        draw.text((6, y + 6), "%s   colour %s" % (run, verdict), fill=(0, 0, 0))
        sheet.paste(a, (0, y + label_h))
        sheet.paste(b, (a.width + 30, y + label_h))
        y += max(a.height, b.height) + label_h + 10
    out = DIR / "compare.png"
    sheet.save(out)
    print("wrote", out)


if __name__ == "__main__":
    main()
