"""The ground shadow, as ONE engine-drawn asset rather than 38 baked ones.

WHY NOT PAINT IT INTO THE SPRITES
---------------------------------
Every current placeholder has a maroon ellipse painted under it, and that choice
has already cost real work twice:

  * The animation blueprints: a creature that tips or bobs drags its shadow with
    it, so the shadow rotates off the floor. A ground shadow that leaves the
    ground stops reading as a shadow.
  * The 3D trial: image-to-3D reconstructed the painted ellipse as an actual
    DISC OF GEOMETRY under the model. Several creatures came back standing on a
    plinth, and the negative prompt never fully suppressed it.

Both are the same root cause: a shadow belongs to the FLOOR, and the sprite is
not the floor.

WHAT AN ENGINE-DRAWN SHADOW BUYS
--------------------------------
  * One asset for all 38 sprites, so it can never drift in style between them.
  * It stays flat while the creature bobs, leans and squashes above it.
  * It scales with `sprite_target_px` (56-108px across the roster) for free.
  * FLIERS get it right: a flier's shadow should sit below and shrink with
    height. Painted in, a flier carries its shadow through the air.
  * It can be dimmed per biome — a sewer needs less shadow than a lawn — and
    turned off entirely without touching a single creature.
  * The handoff already warns that removing painted shadows from finished art is
    BANNED: it was tried, it ate 14% of angry_dog including both eyes. So the
    only safe time to not have them baked in is while the art is being redrawn.

So: draw the creatures with NO shadow, and let this sit under them.

Usage:  py tools/make_ground_shadow.py
"""

import argparse
import os

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "art_raw", "shadow")

# soft  - a diffuse contact shadow for most creatures
# tight - a harder, smaller pool for small fast vermin, which read better with
#         a crisper anchor point
VARIANTS = {
    "ground_shadow_soft":  dict(w=256, h=96, softness=1.6, peak=0.42),
    "ground_shadow_tight": dict(w=256, h=80, softness=2.6, peak=0.55),
}


def ellipse(w, h, softness, peak):
    """Pure black, alpha-only, soft radial falloff.

    Procedural rather than generated: a diffusion model cannot hold a clean
    radial gradient, and this is four lines of numpy. Same reasoning that put
    the UI buttons and plates in make_ui_placeholders.py instead of the LoRA.
    """
    yy, xx = np.mgrid[0:h, 0:w]
    nx = (xx - (w - 1) / 2.0) / ((w - 1) / 2.0)
    ny = (yy - (h - 1) / 2.0) / ((h - 1) / 2.0)
    r = np.sqrt(nx * nx + ny * ny)
    a = (np.clip(1.0 - r, 0.0, 1.0) ** softness * peak * 255).clip(0, 255)
    return Image.fromarray(
        np.dstack([np.zeros((h, w, 3), np.uint8), a.astype(np.uint8)]), "RGBA")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", default=OUT)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    for name, spec in VARIANTS.items():
        p = os.path.join(a.out, name + ".png")
        ellipse(**spec).save(p)
        print("wrote %-34s %dx%d" % (name + ".png", spec["w"], spec["h"]))
    print()
    print("Wiring, when the engine work happens:")
    print("  a Sprite2D BELOW the creature, z_index one less, NEVER rotated,")
    print("  width scaled to the creature's own width, alpha scaled by height")
    print("  for fliers. Draw the creatures themselves with no shadow at all.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
