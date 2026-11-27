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

# The aura (playtest 2026-11-26, note 31: "the aura shouldn't have a huge round outline. just feather is good"): the fill
# alone, its outer AURA_FEATHER of the radius fading to clear; no ring (width 0: not drawn). D18 had a 6 px ring at 85 %.
AURA_LIFT = PAL.STICKER_CREATURES["halo"]
AURA_LIFT_ALPHA = 0.18
AURA_FEATHER = 0.35          # the share of the radius that fades from AURA_LIFT_ALPHA to clear
AURA_RING = PAL.STICKER_CREATURES["halo"]
AURA_RING_ALPHA = 0.85
AURA_RING_WIDTH = 0.0        # px; 0: no ring
AURA_INK_EDGE = 0.0          # none: the concept's ring has no ink edge
AURA_INK_ALPHA = 0.0

# The ground zones cards leave (they were the first try's colours, hard-coded in PawVfx). Ember Trail burns red (playtest
# 2026-11-26, note 39: "ember trail is blue. it should be red"); Hammerfall's crater and Anvil Echo keep theirs.
ZONE_TRAIL = PAL.STICKER["badge"]
ZONE_CRATER = "#FF853D"
ZONE_ECHO = "#9EE0F0"

# Thunderclap's detonation (playtest 2026-11-26, note 32: "thunderclap doesn't have a visual effect?": the first try's 4 px
# pale ring faded as it grew and was gone inside the cat). A bold white ring on an ink edge sweeps out to the aura's edge
# and holds its strength for PULSE_HOLD of its life, over a quick flash of the aura's yellow.
PULSE_RING = PAL.STICKER["white"]
PULSE_RING_WIDTH = 10.0      # px
PULSE_INK_EDGE = 3.0         # px each side
PULSE_FLASH = PAL.STICKER_CREATURES["halo"]
PULSE_FLASH_ALPHA = 0.35
PULSE_SEC = 0.45
PULSE_HOLD = 0.6             # the share of its life at full strength

CRIT = PAL.STICKER["yellow"]  # crits and earned crits (an earned crit stays twice as wide, as in Godot)

STRIKE_PAW = False           # D18: the concept's swipe has no paw (D3's paw, drawn when True)
STRIKE_ARC = PAL.STICKER["white"]   # the concept's arc
STRIKE_SLASH_INK = True      # the concept's slashes are ink, with no colour inside
STRIKE_PAW_PX = 46.0         # the drawing's 128-unit box, in field px (the cat is 175)
STRIKE_ARC_TRAIL = 1.0       # the arc's length, a share of Godot's (D3 had 0.6, behind its paw)
STRIKE_PAW_FADE_GAIN = 1.6   # the paw's opacity = min(1, the swipe's fade x this): solid for the first 37 % of the sweep
