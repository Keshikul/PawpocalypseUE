"""The field effects' look, restyle decision D3 (designer, 2026-11-09: "Recommended", on the canvas board "Review: D3, the
aura and the paw strike"; docs/WORKLIST-RESTYLE-2026-11-07.md §2).

  aura   Sticker Box (D18, 2026-11-18): the UI concept's ring, its halo yellow at 85 % over a 16 % fill of the same, 6 px
         (5 at the concept's 1600-wide mock), no ink edge. (D3 had a white lift and a Bone ring on a Plum edge.)
  strike Sticker Box (D18, 2026-11-18): the concept's swipe, a white arc with its ink edge and the three claw slashes in
         ink, at Godot's length; no paw. (D3's S3, the clutching paw riding a shorter arc, can be switched back on:
         STRIKE_PAW.)
  crit   Sticker's yellow (D3: Butter; Godot's pink is the reward colour)

One source for the review sheet (tools/art/review_d3.py) and the game (tools/gen_fx_look.py -> Realm/PawFxLook.h), so what
was approved is what ships. Timing, sizes of the arc and the claws, and when each part shows stay Godot's (vfx_swipe.gd).
"""

import palette as PAL

AURA_LIFT = PAL.STICKER_CREATURES["halo"]
AURA_LIFT_ALPHA = 0.16
AURA_RING = PAL.STICKER_CREATURES["halo"]
AURA_RING_ALPHA = 0.85
AURA_RING_WIDTH = 6.0        # px
AURA_INK_EDGE = 0.0          # none: the concept's ring has no ink edge
AURA_INK_ALPHA = 0.0

CRIT = PAL.STICKER["yellow"]  # crits and earned crits (an earned crit stays twice as wide, as in Godot)

STRIKE_PAW = False           # D18: the concept's swipe has no paw (D3's paw, drawn when True)
STRIKE_ARC = PAL.STICKER["white"]   # the concept's arc
STRIKE_SLASH_INK = True      # the concept's slashes are ink, with no colour inside
STRIKE_PAW_PX = 46.0         # the drawing's 128-unit box, in field px (the cat is 175)
STRIKE_ARC_TRAIL = 1.0       # the arc's length, a share of Godot's (D3 had 0.6, behind its paw)
STRIKE_PAW_FADE_GAIN = 1.6   # the paw's opacity = min(1, the swipe's fade x this): solid for the first 37 % of the sweep
