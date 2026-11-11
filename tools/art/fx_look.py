"""The field effects' look, restyle decision D3 (designer, 2026-11-09: "Recommended", on the canvas board "Review: D3, the
aura and the paw strike"; docs/WORKLIST-RESTYLE-2026-11-07.md §2).

  aura   D: the inside lifted with white at 10 %, a Bone ring with a Plum ink edge (Godot's gold ring and fill are gone)
  strike S3: the clutching paw (ui_cursor_paw_closed's drawing, claws out) rides the tip of a shorter arc
  crit   Butter (Godot's pink is the reward colour)

One source for the review sheet (tools/art/review_d3.py) and the game (tools/gen_fx_look.py -> Realm/PawFxLook.h), so what
was approved is what ships. Timing, sizes of the arc and the claws, and when each part shows stay Godot's (vfx_swipe.gd).
"""

import palette as PAL

AURA_LIFT = "#FFFFFF"
AURA_LIFT_ALPHA = 0.10
AURA_RING = PAL.PLUM["bone"]
AURA_RING_WIDTH = 3.0        # px, the Bone line
AURA_INK_EDGE = 2.0          # px of Plum each side of it
AURA_INK_ALPHA = 0.85        # as the swipe's ink edge

CRIT = PAL.PLUM["butter"]    # crits and earned crits (an earned crit stays twice as wide, as in Godot)

STRIKE_PAW_PX = 46.0         # the drawing's 128-unit box, in field px (the cat is 175)
STRIKE_ARC_TRAIL = 0.6       # the arc's length behind the paw, a share of Godot's
STRIKE_PAW_FADE_GAIN = 1.6   # the paw's opacity = min(1, the swipe's fade x this): solid for the first 37 % of the sweep
