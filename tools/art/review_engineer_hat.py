"""C4 (playtest 2026-11-26, note 28): the Engineer's hard hat from the side, a review sheet.

Today's `hardhat` (tools/art/concept_hats.js) is drawn front-on, mirror-symmetric, on a cat drawn from the side. B is a
side view in the concept's own grammar (its P/E/T helpers, its ink line): a dome taller at the front, a short peak toward
the way the cat faces, a small lip at the back, the ridge running front to back. It stays inside today's hat square
(x -53..19, y -106..-68 in the concept's units), so PawFieldLook.h's hat constants stay.

The sheet runs the UI concept's code (tools/art/concept_ui_screens.html, as tools/art/sticker_record.js does) in node:
the Tiny Kitten facing left and right with each hat, and the hat alone (the job icon). concept_hats.js is NOT changed:
that waits for the designer's pick (then it is recorded again, sticker_record.js).

Usage:  py tools/art/review_engineer_hat.py        -> build/review/c4_engineer_hat.png
"""
from __future__ import annotations

import io
import subprocess
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ART = ROOT / "tools" / "art"
OUT = ROOT / "build" / "review" / "c4_engineer_hat.png"

JS = r"""
const fs = require('fs'), path = require('path');
const HERE = process.argv[1];
const uiHtml = fs.readFileSync(path.join(HERE, 'concept_ui_screens.html'), 'utf8');
const uiScript = uiHtml.slice(uiHtml.lastIndexOf('<script>') + 8, uiHtml.indexOf('/* ---------- the page ---------- */'));
const hatsJs = fs.readFileSync(path.join(HERE, 'concept_hats.js'), 'utf8');
const M = new Function(uiScript + '\n' + hatsJs + '\nLW = L.lineW;\nreturn {catFig, NEWHAT, P, E, T, mix};')();  // LW: the ink line fig() draws with
const {P, E, T, mix} = M;
const Y = '#FFC83D', shade = mix(Y, '#2A2131', .18), light = mix(Y, '#FFFFFF', .45);
const side = () =>
    P('M-43 -75 C -46 -104, 4 -108, 12 -76 Z', Y)                    // the dome, taller at the front
  + P('M-54 -71 C -46 -66, 8 -67, 17 -72.5 C 9 -78, -36 -79, -54 -71 Z', Y)  // the brim: a long peak in front, short behind
  + T('M-35 -86 C -28 -97, -6 -98, 5 -86', 5, light, {noLine: true}); // the ridge, front to back
const hats = {today: M.NEWHAT.hardhat, side};
const cell = (inner, flip) => '<g transform="' + (flip ? 'scale(-1,1)' : '') + '">' + inner + '</g>';
const out = {};
for (const [k, h] of Object.entries(hats)) {
  const fig = M.catFig({}) + h();
  out[k + '_left'] = cell(fig, false);
  out[k + '_right'] = cell(fig, true);
  out[k + '_icon'] = cell(h(), false);
}
process.stdout.write(JSON.stringify(out));
"""


def svg(inner: str, view: str, px: int) -> bytes:
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{view}" width="{px}" height="{px}">{inner}</svg>').encode()


def raster(svg_bytes: bytes) -> Image.Image:
    import resvg_py
    return Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_string=svg_bytes.decode())))).convert("RGBA")


def main() -> int:
    import json
    from PIL import ImageDraw, ImageFont
    res = subprocess.run(["node", "-e", JS, str(ART)], capture_output=True, text=True, check=True)
    parts = json.loads(res.stdout)
    try:
        f = ImageFont.truetype("arialbd.ttf", 18)
    except OSError:
        f = ImageFont.load_default()
    cream = (246, 238, 222, 255)
    W, H = 3 * 260 + 40, 2 * 300 + 70
    sheet = Image.new("RGBA", (W, H), cream)
    d = ImageDraw.Draw(sheet)
    for r, (k, label) in enumerate((("today", "A  today (front-on)"), ("side", "B  from the side"))):
        y = 20 + r * 310
        d.text((16, y), label, fill=(42, 33, 49, 255), font=f)
        cells = [(parts[k + "_left"], "-75 -125 130 130", "facing left"),
                 (parts[k + "_right"], "-55 -125 130 130", "facing right"),
                 (parts[k + "_icon"], "-62 -115 90 90", "the job icon")]
        for i, (inner, view, cap) in enumerate(cells):
            img = raster(svg(inner, view, 240))
            x = 20 + i * 260
            sheet.alpha_composite(img, (x, y + 30))
            d.text((x + 8, y + 274), cap, fill=(110, 98, 118, 255), font=f)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    sheet.convert("RGB").save(OUT)
    print(OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
