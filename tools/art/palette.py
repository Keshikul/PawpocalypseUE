#!/usr/bin/env python3
"""The Pawpocalypse colour lookup table, and the rules it has to obey.

THE PLUM RESTYLE (designer, 2026-11-07: "Plum everywhere"; D14, docs/WORKLIST-RESTYLE-2026-11-07.md). The Unreal look left
Godot. INK is now Plum #2A1A33, not Stacklands black: every outline and every letter. The PLUM family below is the menus'
palette (panels, rows, text, focus, buy, reward, threat), checked by rule 6 (contrast, WCAG 2 ratios). The Stacklands
table stays as the measured record the light grounds, the card families and the creatures are still built from.

WHY THIS IS A PROGRAM AND NOT A LIST. Every colour here is checked against
rules that can be measured rather than argued about, and the checks run every
time the table is touched. A palette kept as a list of hexes drifts the moment
someone adds "just one more" mid-grey.

THE SCHEME IS STACKLANDS' (designer, 2026-09-14: "Look at game Stacklands. I
want the color scheme to be the same"). Nothing below was eyeballed off a
memory of that game: seven 1920x1080 Steam screenshots were sampled - the
median of a flat region for every surface and every card family, the darkest
2% of a panel for the ink, a 24-colour quantisation of all seven for the
dominant tones - and the STACKLANDS table is those numbers verbatim. Everything
the game draws is either one of them or a derivation written down beside it.
docs/pawpocalypse-colour-reference.md carries the same tables with the
reasoning; `py tools/palette.py --md` prints them so the two cannot drift.

WHAT STACKLANDS ACTUALLY DOES, read off the samples:

  * INK IS BLACK. #1A1A1A on every outline, every letter, every card frame.
    The previous palette insisted on a dark purple ink; that is gone.
  * SURFACES ARE PAPER. Panels are cream (#FFF9E3), boards are sand (#F7F2DC)
    or meadow (#B3E5B2), and detail on a surface is the SAME HUE at a lower
    value - meadow #B3E5B2 carries its bush lines at #98BE98, sand carries its
    doodles at #E5D4B7. One field, one detail, no gradients. The old rule
    survives because Stacklands obeys it.
  * THERE IS A DARK REGISTER, and it is not black. The Cardopedia backdrop is
    the world dimmed to V44-49 at S21-24 (#58705A / #607E60): desaturated,
    still coloured, nowhere near the old #141220. The designer wants the space
    biomes and the Sewers dark; this is what dark looks like in that game.
  * IDENTITY IS A CARD FAMILY: a FACE colour and a HEADER colour, the header
    more saturated or darker. Eleven families, and every creature, boss, tab
    and card flavour in this game is now one of them.
  * TWO MORE THINGS AND NOTHING ELSE: alert red #FF7055 and disabled grey
    #C7C1AB. That is the whole vocabulary.

THE RULES, as numbers

  1. EVERY GROUND IS IN A REGISTER. Light is V >= 86, dark is V <= 50. The band
     between is where Stacklands puts only its outer-world vignette (#9AC09B,
     V75) - never a surface a thing has to be read against. A ground in the
     band is a ground that will swallow half the cast.
  2. NO CREATURE MAY LOOK LIKE THE CAT. Enforced on ACCENTS: every enemy and
     boss accent must be at least MIN_FROM_CAT from CAT_ACCENT in CIE76. The
     cat's accent is the black-ish "dark" family, so no enemy uses it.
  3. EVERY CREATURE MUST SEPARATE FROM ITS GROUND. Body and accent both at
     least MIN_FROM_GROUND from the field they stand on, checked for the
     combinations that actually ship (BIOME_CREATURES), not the cross product.
  4. THE CAT SEPARATES FROM EVERY GROUND, both registers. It is the one
     mid-value warm grey in the game (V62) precisely so it clears the paper
     above it and the night below it.

    py tools/palette.py            # print the table and run the checks
    py tools/palette.py --json     # machine-readable, for the recolour pass
    py tools/palette.py --gd       # write scripts/systems/palette.gd
    py tools/palette.py --md       # the tables, for the reference doc
"""

from __future__ import annotations

import argparse
import colorsys
import json
import pathlib
import sys

# --- the rules, as numbers ---------------------------------------------------

LIGHT_V_MIN = 86.0        # a light-register ground is at least this bright
DARK_V_MAX = 50.0         # a dark-register ground is at most this bright
MIN_FROM_CAT = 26.0       # CIE76 between any creature accent and the cat's
MIN_FROM_GROUND = 22.0    # CIE76 between a creature colour and its field
UNIQUE_MIN = 16.0         # CIE76 between any two characters' bodies (rule 8: no shade used twice)
ACCENT_MIN = 10.0         # CIE76 between an accent and every other body and accent


def hsv(h_deg: float, s_pct: float, v_pct: float) -> str:
    r, g, b = colorsys.hsv_to_rgb(h_deg / 360.0, s_pct / 100.0, v_pct / 100.0)
    return "#%02X%02X%02X" % (round(r * 255), round(g * 255), round(b * 255))


def to_hsv(hex_colour: str) -> tuple[float, float, float]:
    r = int(hex_colour[1:3], 16) / 255.0
    g = int(hex_colour[3:5], 16) / 255.0
    b = int(hex_colour[5:7], 16) / 255.0
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    return h * 360.0, s * 100.0, v * 100.0


def _lab(hex_colour: str) -> tuple[float, float, float]:
    """sRGB -> CIE Lab (D65). Written out rather than pulled in: three formulae
    against a dependency the recolour pass would also have to carry."""
    def lin(c: float) -> float:
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    r = lin(int(hex_colour[1:3], 16) / 255.0)
    g = lin(int(hex_colour[3:5], 16) / 255.0)
    b = lin(int(hex_colour[5:7], 16) / 255.0)
    x = (0.4124 * r + 0.3576 * g + 0.1805 * b) / 0.95047
    y = (0.2126 * r + 0.7152 * g + 0.0722 * b) / 1.00000
    z = (0.0193 * r + 0.1192 * g + 0.9505 * b) / 1.08883
    def f(t: float) -> float:
        return t ** (1.0 / 3.0) if t > 0.008856 else (7.787 * t) + (16.0 / 116.0)
    fx, fy, fz = f(x), f(y), f(z)
    return 116.0 * fy - 16.0, 500.0 * (fx - fy), 200.0 * (fy - fz)


def distance(a: str, b: str) -> float:
    la, aa, ba = _lab(a)
    lb, ab, bb = _lab(b)
    return ((la - lb) ** 2 + (aa - ab) ** 2 + (ba - bb) ** 2) ** 0.5


def mix(a: str, b: str, t: float) -> str:
    """Straight RGB mix, for a derivation that has to stay between two named
    colours (a puddle between a field and its detail, a hover between two
    creams)."""
    ra, ga, ba = (int(a[i:i + 2], 16) for i in (1, 3, 5))
    rb, gb, bb = (int(b[i:i + 2], 16) for i in (1, 3, 5))
    return "#%02X%02X%02X" % (round(ra + (rb - ra) * t), round(ga + (gb - ga) * t),
                              round(ba + (bb - ba) * t))


# --- STACKLANDS, measured --------------------------------------------------
#
# Median RGB of a flat region in a Steam screenshot (1920x1080), 2026-09-14.
# The "where" column in the reference doc names the shot and the region. These
# are the source; nothing below invents a colour that is not derived from one.

STACKLANDS = {
    # paper and boards
    "paper":          "#FFF9E3",   # quest panel, tooltip, every dialog
    "paper_band":     "#FFF6CF",   # a panel's section header row
    "paper_bright":   "#FFFDED",   # the brightest cream in the quantisation
    "sand":           "#F7F2DC",   # island board; the dominant colour overall
    "sand_detail":    "#E5D4B7",   # doodles on the sand
    "meadow":         "#B3E5B2",   # the meadow board
    "meadow_detail":  "#98BE98",   # bush and grass lines on the meadow
    "world_green":    "#9AC09B",   # the world outside the board (a vignette)
    "night_green":    "#607E60",   # Cardopedia backdrop, light ray
    "night_deep":     "#58705A",   # Cardopedia backdrop, dark ray
    "ocean":          "#AFD1EC",   # the sea beside the island
    # ink and text
    "ink":            "#1A1A1A",   # every outline and every letter
    "disabled":       "#C7C1AB",   # a completed quest's text
    "alert":          "#FF7055",   # the exclamation badge
    # the eleven card families: (face, header)
    "red":            ("#D86061", "#EC7B7B"),   # rat, seagull - enemies
    "coral":          ("#F6BBAD", "#F8A08C"),   # shed, house, campfire - buildings
    "orange":         ("#E89956", "#F4A968"),   # berry, banana - food
    "yellow":         ("#FFEFB4", "#F2C65B"),   # coin, shell - money
    "blue":           ("#6B81B9", "#8596C3"),   # idea
    "green":          ("#AEDC84", "#70954F"),   # cod - fresh things
    "slate":          ("#6B7485", "#9BA2AD"),   # stone, stick, flint, wood
    "dark":           ("#474641", "#63625D"),   # tree, rock, driftwood
    "grey":           ("#F6F6F6", "#CDCDCD"),   # corpse
    "cream":          ("#F8F1DE", "#EEE2AC"),   # villager
    "black":          ("#1A1A1A", "#3A3A3A"),   # booster packs
    # the villager's icon disc
    "disc":           "#F0E7B3",
}

FAMILY_NAMES = ["red", "coral", "orange", "yellow", "blue", "green",
                "slate", "dark", "grey", "cream", "black"]
FAMILIES = {name: STACKLANDS[name] for name in FAMILY_NAMES}


def face(family: str) -> str:
    return FAMILIES[family][0]


def header(family: str) -> str:
    return FAMILIES[family][1]


# --- the plum restyle (2026-11-07) ------------------------------------------
#
# Chosen on the concept canvas (the boards "Original, plum menus", "Main menu (plum)", "Skill tree (plum)"). Roles:
# base = panels and the ink; deep = panel edges and the solid drop shadows; raised = rows and inactive tabs on a panel;
# bone = text on plum and card bodies (never cream); lilac = secondary text; butter = focus, the selected thing, numbers;
# catnip = buying and Catnip; stardust = Stardust and rewards only; crimson = threat, bosses, can't-afford.

PLUM = {
    "base":     "#2A1A33",
    "deep":     "#120A17",
    "raised":   "#3E2A4A",
    "bone":     "#F6EEF8",
    "lilac":    "#DCCFE4",
    "butter":   "#FFD23F",
    "catnip":   "#5FD068",
    "stardust": "#B98CFF",
    "crimson":  "#D2263F",
}
MIN_TEXT_CONTRAST = 4.5      # WCAG 2 AA for body text
MIN_LARGE_CONTRAST = 3.0     # WCAG 2 AA for 24 px and up, and for a focus ring against its panel

# The booster-pack family was Stacklands' ink; it follows the ink (nothing draws it today).
STACKLANDS_BLACK = STACKLANDS["black"]
FAMILIES["black"] = (PLUM["base"], PLUM["raised"])

# --- core ------------------------------------------------------------------

INK = PLUM["base"]
EYE = STACKLANDS["grey"][0]          # eye whites are the corpse card's white
GOLD = header("yellow")              # Catnip is the coin card's header
STARDUST = header("blue")            # Stardust is the idea card's header
ALERT = STACKLANDS["alert"]
DISABLED = STACKLANDS["disabled"]

# --- the player --------------------------------------------------------------
#
# THE CAT IS THE ONE MID-VALUE THING IN THE GAME. Stacklands has no cat; its
# villager is cream, and a cream cat vanishes on the cream kitchen and the
# cream corner of every panel. Two registers of ground - paper above V86,
# night below V50 - leave exactly one place a body can stand clear of both,
# and that is V60-70, which is where the tabby goes. Its accent is the dark
# family, so its stripes are the only near-black fill in the cast and no
# enemy may use that family for an accent (rule 2).

CAT_BODY = hsv(32, 16, 62)
CAT_ACCENT = face("dark")
CAT_NOSE = header("coral")

PLAYER = {
    "cat_body": CAT_BODY,
    "cat_accent": CAT_ACCENT,
    "cat_nose": CAT_NOSE,
    "kitten_body": "#BD966A",          # tan (R5, 2026-11-09): its own colour, clear of every floor (cream vanished on the Kitchen)
    "kitten_accent": "#A07D5F",
    "unicorn_body": face("grey"),
}


def body(h, s=30.0, v=95.0):
    """Kept for callers; a body in this palette is a family face, and the few
    that are not are written out as hsv() beside a reason."""
    return hsv(h, s, v)


# --- enemies -----------------------------------------------------------------
#
# ONE COLOUR PER ENEMY (designer, 2026-11-09: "make sure that every enemy has it's own color and player has own color
# too. So no same shade is used twice"). Until then (body, accent) were Stacklands family faces and headers, eleven
# families for some thirty characters, so one red was the rat, the seagull, the parrot, the hound and the imp. Now each
# enemy kind, each boss and the player (the cat's body and stripes, the kitten) has a body of its own, at least
# UNIQUE_MIN apart from every other (rule 8), inside a hue/saturation/value window that keeps it what it was (the rat
# red, the mouse blue, the crow dark, the goose white), and still clear of every floor it ships on (rule 3). The accent
# is the body shaded toward INK or tinted toward cream, except two: the goose's orange beak, and the dog's brown patches
# (no shade of its body was free), at least ACCENT_MIN from every other colour here. A kind's biome copies share its colour: they are the
# same enemy. Chosen by a search over those windows (2026-11-09); change one by hand and rule 8 says if it collides.

ENEMIES = {
    "mouse":        ("#7795C7", "#60709B"),   # periwinkle
    "flying_mouse": ("#BA9FD6", "#8F77A5"),   # lilac: the flier is its own enemy
    "golden_mouse": ("#F0C63E", "#B5923B"),   # gold, the reward
    "rat":          ("#C25D59", "#94494E"),   # brick red
    "vole":         ("#D68C60", "#A26A52"),   # clay
    "hamster":      ("#E6A34C", "#AE7A44"),   # honey
    "gristle":      ("#CCAD45", "#9B8140"),   # mustard
    "moth":         ("#F0B5A3", "#B58681"),   # peach
    "dog":          ("#E0C89D", "#916C41"),   # biscuit, brown patches
    "goose":        ("#E1E2F5", "#FA6E28"),   # lavender white, an orange beak
    "ghost":        ("#C8F4FA", "#D7F5F6"),   # ice
    "pigeon":       ("#A09AB8", "#7D7490"),   # dove grey
    "seagull":      ("#E66653", "#EC9281"),   # coral red
    "parrot":       ("#4BB84F", "#2D78C2"),   # parrot green; its wing, crest and tail macaw blue (R5: the wing is its largest colour)
    "crow":         ("#5D8550", "#8CA77F"),   # moss
    "toad":         ("#88BD60", "#AACE8A"),   # pea green
    "crocodile":    ("#5FA388", "#8EBCA6"),   # jade
    "sludge_cat":   ("#B6CC5E", "#8C9751"),   # bile
    "bat":          ("#6467B2", "#5A599B"),   # indigo
    "imp":          ("#C24663", "#943955"),   # raspberry
    "hound":        ("#99533D", "#78423A"),   # rust
    "slime_rat":    ("#84A9BD", "#99B7C5"),   # slate blue
    "robot_mouse":  ("#93C7C2", "#749397"),   # tin-toy teal: no steel grey was free (the cat, its belly, the Foreman, the pigeon)
    "ant":          ("#BD6039", "#D29376"),   # terracotta
    "cockroach":    ("#6B4C31", "#BCAC9A"),   # chestnut
    "dust_bunny":   ("#DABFE0", "#E2CCE3"),   # mauve fluff
    # Drawn but shipped by no tier (UNSHIPPED): kept as they were, outside the one-colour rule.
    "squirrel":     FAMILIES["orange"],
    "ghost_mouse":  (face("grey"), header("blue")),     # white with blue marks - the grey header vanished on the coral turf
    "void_mouse":   (header("slate"), face("blue")),    # slate body clears the mythic purple; blue face marks
}

BOSSES = {
    "park_tyrant":     ("#408F3B", "#396C39"),   # forest green: it clears its own Park
    "rival_champion":  ("#CC7B35", "#9B5E34"),   # ginger
    "factory_foreman": ("#727F8F", "#969EA7"),   # gunmetal
    "litter_king":     ("#D1C67D", "#9F9267"),   # litter sand
    "void_god":        ("#373F8A", "#333572"),   # deep indigo
    "generic":         ("#C72C46", "#982740"),   # crimson: threat
    "add":             ("#DB5E90", "#A64A74"),   # rose: its adds
}


# --- grounds -----------------------------------------------------------------
#
# One flat field, one flat detail of the same hue, no gradients - which is
# exactly how the meadow and the sand are built in the reference, so those two
# biomes ARE the reference colours. Every other light biome is a hue shift of
# the same recipe (field S10-26 V86-95, detail same hue, +8-14 S, -14 V), and
# every dark biome is the Cardopedia register (S22-40, V32-45) in its own hue.
#
# WHICH BIOMES ARE DARK (designer, 2026-09-14: "Later space biomes should have
# darker backgrounds. Sewers also not light"): the Sewers, the three space
# realms - Portal Zone, Mythic Realm, Litterbox Dimension - and the Pet Food
# Factory, which is a night shift. Seven light, five dark.
#
# "patch" is the large soft darker blotch BiomeBackdrop lays over the tiled
# floor to break the repeat (designer: "the tiled backgrounds are too tiled";
# "some darker spots overlays or large puddles"); "puddle" is the same idea
# with a lighter rim, for the floors that are wet.


def _light(h, s, v, wet=False):
    field = hsv(h, s, v)
    detail = hsv(h, min(s + 12, 100), v - 14)
    return {"field": field, "detail": detail,
            "patch": mix(field, detail, 0.55),
            "puddle": hsv(h, min(s + 6, 100), v - 6) if wet else "",
            "register": "light"}


def _dark(h, s, v, wet=False):
    field = hsv(h, s, v)
    detail = hsv(h, min(s + 6, 100), v - 8)
    return {"field": field, "detail": detail,
            "patch": mix(field, detail, 0.7),
            "puddle": hsv(h, max(s - 4, 0), v + 8) if wet else "",
            "register": "dark"}


BACKDROPS = {
    "tier_1_backyard":   dict(_light(119, 22, 90), field=STACKLANDS["meadow"], detail=STACKLANDS["meadow_detail"]),
    "tier_2_alley":      _light(215, 10, 87, wet=True),     # wet pavement
    "tier_3_park":       _light(92, 28, 90),
    "tier_4_rooftops":   dict(_light(207, 26, 92), field=STACKLANDS["ocean"]),
    "tier_5_sewers":     _dark(160, 26, 40, wet=True),
    "tier_6_turf":       _light(18, 26, 94),
    "tier_7_portal":     _dark(228, 30, 42),
    "tier_8_mythic":     _dark(276, 30, 40),
    "tier_9_kitchen":    dict(_light(49, 11, 97), field=STACKLANDS["sand"], detail=STACKLANDS["sand_detail"]),
    "tier_10_vets":      _light(165, 16, 90, wet=True),     # mopped floor
    "tier_11_factory":   _dark(28, 30, 38),                 # deeper than the other night floors: the cat is warm grey and this is the warm one
    "tier_12_litterbox": _dark(258, 34, 32, wet=True),      # the puddle is litter clumping
}
# The meadow's patch and the sand's patch are re-derived from the reference
# tones, not from the hsv() approximation the dict started from.
for _key in ("tier_1_backyard", "tier_4_rooftops", "tier_9_kitchen"):
    _t = BACKDROPS[_key]
    _t["patch"] = mix(_t["field"], _t["detail"], 0.55)

BIOME_CREATURES = {
    "tier_1_backyard": ["golden_mouse", "gristle", "moth", "mouse", "vole"],
    "tier_2_alley": ["dog", "flying_mouse", "golden_mouse", "gristle", "mouse", "rat", "vole"],
    "tier_3_park": ["flying_mouse", "golden_mouse", "goose", "gristle", "mouse", "pigeon", "rat", "vole"],
    "tier_4_rooftops": ["bat", "golden_mouse", "gristle", "pigeon", "rat", "seagull"],
    "tier_5_sewers": ["bat", "crocodile", "golden_mouse", "gristle", "pigeon", "rat", "slime_rat"],
    "tier_6_turf": ["bat", "ghost", "golden_mouse", "gristle", "rat", "slime_rat", "toad"],
    "tier_7_portal": ["bat", "ghost", "golden_mouse", "gristle", "hound", "imp", "slime_rat"],
    "tier_8_mythic": ["bat", "crow", "ghost", "golden_mouse", "gristle", "imp"],
    "tier_9_kitchen": ["ant", "bat", "cockroach", "golden_mouse", "gristle"],
    "tier_10_vets": ["bat", "golden_mouse", "gristle", "hamster", "parrot"],
    "tier_11_factory": ["bat", "golden_mouse", "gristle", "rat", "robot_mouse"],
    "tier_12_litterbox": ["bat", "dust_bunny", "golden_mouse", "gristle", "sludge_cat"],
}


# BIOME_CREATURES is derived from the tier .tres spawn pools and each entry's
# texture_path (2026-09-14), so it lists what ACTUALLY ships, tinted reuses
# included - which is how the khaki mythic squirrel was found.
# --- UI ----------------------------------------------------------------------
#
# The dock, every dialog and every tooltip are Stacklands panels: cream paper,
# a 3px black frame, black text. "We still have dark background behind
# upgrades menu" (designer, 2026-09-14) - not any more. An ENABLED button is
# the brightest cream on the panel because affordable is the state the player
# scans for; disabled is the grey text the reference uses for a finished quest.
# Each tab is a card family so the five read apart at a glance.

# PLUM EVERYWHERE (restyle R3, designer 2026-11-07): the panels are Plum, the text Bone, the rows and buttons Plum Raised,
# the wells and the pressed state Deep Plum. R3's shapes (2026-11-08): the plates' edges and their one solid shadow, straight
# down, are Deep Plum (spec: docs/WORKLIST-RESTYLE-2026-11-07.md §4; the chrome in sweet_ui.py, the frames the walker's
# EDGE role). The ink stays the ink of letters, the outline of labels over the field and a card's frame. The tab
# families, the currencies and the card families keep their colours; a card's body is Bone with ink text on it (WBP_CardFace keeps its ink: bake_widgets.py
# --recolour, KEEP_INK). Rule 7 checks every pair the plum screens draw.
PLUM_HOVER = mix(PLUM["raised"], PLUM["lilac"], 0.15)

UI = {
    "panel":           PLUM["base"],
    "panel_band":      PLUM_HOVER,                # (shares the hover tone: one old cream, one new plum)
    "panel_frame":     PLUM["deep"],              # a plate's edge (R3's shapes)
    "text":            PLUM["bone"],
    "text_muted":      PLUM["lilac"],
    "text_disabled":   mix(PLUM["lilac"], PLUM["base"], 0.2),   # R4e: a row it cannot pay is still READ (the board's)
    "button":          PLUM["raised"],
    "button_hover":    PLUM_HOVER,
    "button_pressed":  PLUM["deep"],
    "button_disabled": mix(PLUM["base"], PLUM["raised"], 0.5),
    "button_frame":    PLUM["deep"],
    "slot":            PLUM["deep"],              # a recessed well on a panel
    "shadow":          PLUM["deep"],              # the plates' one solid shadow, straight down (R3's shapes)
    "badge":           ALERT,
    "focus":           PLUM["butter"],            # the keyboard/pad selection ring (R4a)
    "selected":        PLUM["butter"],            # the selected thing: the open tab, a settings section, a call (R4a)
    "selected_text":   INK,                       # a label on the selected plate
    "catnip":          GOLD,
    "stardust":        STARDUST,
    "tab_upgrades":    header("orange"),
    "tab_reawaken":    header("blue"),
    "tab_realms":      header("green"),
    "tab_catches":     header("yellow"),
    "tab_awards":      header("coral"),
    "boss_banner":     PLUM["crimson"],
    "boss_banner_text": PLUM["bone"],
    "field_label":     PLUM["bone"],              # the fill of a label over the live field (its outline is INK)
    "danger":          PLUM["crimson"],
    "good":            header("green"),
    "card_plate":      PLUM["bone"],
    "card_ink":        INK,                       # a card's text and frame on its Bone body (WBP_CardFace keeps the ink)
    "card_swarm":      header("green"),
    "card_pounce":     header("red"),
    "card_aura":       header("blue"),
    "card_deck":       header("yellow"),
    "card_take":       INK,
    "card_banish":     STACKLANDS["alert"],
}

# Colours Godot authored as literals in scenes and scripts rather than in its
# palette (gen_paw_tokens.py once read them out of the Godot repo). Since the
# Unreal look left Godot (designer, 2026-11-07) they live here, kept as the
# exact sRGB float strings Godot wrote: (r, g, b, a or None for opaque, where).
LITERALS = {
    "CEREMONY_FLASH":    ("0.972", "0.949", "1", None, "reawaken_ceremony.tscn Flash color"),
    "CEREMONY_HEADLINE": ("0.184", "0.129", "0.302", None, "reawaken_ceremony.tscn Headline font_color"),
    "CEREMONY_PAYOUT":   ("0.286", "0.208", "0.435", None, "reawaken_ceremony.tscn Payout font_color"),
    "VICTORY_DIM":       ("0.05", "0.04", "0.08", "0.88", "victory_screen.gd dim.color"),
}

# The skill-tree page is the one thing in the game that is not paper or
# night-green: the designer asked for it on 2026-09-14 - "purple dark with
# cloud like things floating around and maybe some stars but not too bright.
# It should not overpower the skill icons." It is the dark register's recipe
# (S30-40, V20-35) in violet, with the clouds one step up and the stars the
# paper cream at low alpha.
SKY = {
    "base":       hsv(262, 44, 20),
    # Toned down 2026-09-15 (designer: "the fog shapes behind the tree can be
    # toned down a bit. They stand out a bit much"): five points of value
    # above the base instead of ten, and the sky draws them at 0.35 alpha.
    "cloud":      hsv(266, 42, 25),
    # Six degrees of hue and one of value apart, on purpose (designer,
    # 2026-09-14: "the background extra shapes should have less color
    # variations among them"). The clouds still breathe between the two, but
    # every cloud reads as the same violet.
    "cloud_warm": hsv(272, 40, 26),
    "star":       STACKLANDS["paper"],
    "plate":      hsv(262, 30, 27),   # a node's plate, one step up from the sky
    "plate_edge": hsv(262, 24, 42),
}

# Materials the card and skill icons are drawn in. These used to be ad-hoc
# hexes in make_sweet_art.py; each is now the family it obviously is.
# The backdrop props' own colours (sweet_biomes.py), named (R5, 2026-11-09). They were literal hexes in the prop drawings,
# the look lint's only colour faults outside the cast; naming them here makes them palette colours without changing a pixel.
# A prop is scenery, never a character: "one colour per character" does not count them.
PROPS = {
    "fur": ("#B5A894", "#D6C9B5", "#EDE4D6"),       # the Vet's fur clump: shadow, coat, light
    "bag": ("#564E52", "#6E6A63", "#959189"),       # the Alley's bin bag
    "flowers": ("#E8B0C4", "#F0D27C", "#B49AD6"),   # the Park's flowers: pink, yellow, lilac
    "rock": ("#9E93B5", "#B5ABC9"),                 # the Mythic Realm's rock and its light
    "toaster": ("#C25E4E",),                        # the Kitchen toaster's knob
}

# The cat's gear: its weapons, its armour and its mounts' coats (sweet_evolutions.py, make_sweet_art.py), named (R5,
# 2026-11-09). They were literal hexes; the designer keeps the cat's looks ("Keep today's cats looks"), so they are named
# exactly as drawn, not moved to nearer tokens: no pixel changes. Not characters (the cat's table is PLAYER).
GEAR = {
    "rainbow": ["#EE9384", "#F3B87C", "#F0D989", "#A9D18C", "#8FC7D6", "#9AA4DC", "#BE9BD3"],  # a pastel rainbow, red pulled to coral
    "steel": "#B9BFC9", "steel_dark": "#8A93A2",      # the armour (stages 2, 3) and the old Foreman's plates
    "hoof": "#5E5566",
    "unicorn_coat": "#F3EFE4", "unicorn_muzzle": "#E9B9BE",
    "ice": "#7FD4E8",                                 # stage 3's frost: the visor, the weapon
    "alicorn": ["#CFE4F5", "#B9D6EE", "#A8C8E4"],     # stage 5: wings and mane, pale to deep
    "nightmare_coat": "#6A5F86", "nightmare_mane": "#B9A6E8", "nightmare_mane_dark": "#8E7FC9",
    "nightmare_deep": "#6E5FA8", "nightmare_horn": "#D8CBF2",
    "halo_wings": "#EFCE86",                          # stage 7
    "haft": "#4A4266",                                # stage 6's weapon
}

MATERIALS = {
    "metal":      header("slate"),
    "metal_dark": face("slate"),
    "wood":       header("orange"),
    "wood_dark":  hsv(28, 62, 62),
    "cloth":      header("blue"),
    "cloth_dark": face("blue"),
    "warm":       header("coral"),
    "paper":      face("cream"),
    "flame":      header("orange"),
    "leaf":       face("green"),
    "leaf_dark":  header("green"),
    "aura":       header("blue"),
    "aura_dark":  face("blue"),
    "claw":       header("red"),
    "bomb":       header("slate"),
    "bait":       header("coral"),
    "bronze":     hsv(28, 52, 72),
    "silver":     header("grey"),
    "rainbow":    [header("red"), header("coral"), header("orange"), header("yellow"),
                   face("green"), header("blue"), face("blue")],
}


# --- variants ----------------------------------------------------------------
# Biome variants of a creature differ by SHAPE, not colour - see the shape
# bible. variant() stays the single place that answers "what colour is this
# creature here" so a future tint has one function to change.


def variant(creature: str, biome: str) -> tuple[str, str]:
    return ENEMIES[creature]


def variant_problems(pairs) -> list[str]:
    bad = []
    for creature, biome in pairs:
        body_c, accent = variant(creature, biome)
        for label, colour in (("body", body_c), ("accent", accent)):
            d = distance(colour, BACKDROPS[biome]["field"])
            if d < MIN_FROM_GROUND:
                bad.append("%s in %s: %s %s is %.0f from its ground %s (min %.0f)"
                           % (creature, biome, label, colour, d,
                              BACKDROPS[biome]["field"], MIN_FROM_GROUND))
    return bad


# --- the checks --------------------------------------------------------------


def check():
    problems = []

    # 1. Grounds sit in a register.
    for biome, tones in BACKDROPS.items():
        _, _, val = to_hsv(tones["field"])
        if tones["register"] == "light" and val < LIGHT_V_MIN:
            problems.append("OUT OF REGISTER: %s field %s is V%.0f, light needs >= %.0f"
                            % (biome, tones["field"], val, LIGHT_V_MIN))
        if tones["register"] == "dark" and val > DARK_V_MAX:
            problems.append("OUT OF REGISTER: %s field %s is V%.0f, dark needs <= %.0f"
                            % (biome, tones["field"], val, DARK_V_MAX))
        if distance(tones["field"], tones["detail"]) < 6.0:
            problems.append("FLAT: %s detail %s is indistinguishable from its field %s"
                            % (biome, tones["detail"], tones["field"]))

    # 2. No enemy accent reads as the cat's.
    for group, table in (("enemy", ENEMIES), ("boss", BOSSES)):
        for name, pair in table.items():
            d = distance(pair[1], CAT_ACCENT)
            if d < MIN_FROM_CAT:
                problems.append("TOO CLOSE TO THE CAT: %s_%s accent %s is %.1f away"
                                % (group, name, pair[1], d))

    # 3. Every creature clears the ground it ships on, body and accent both.
    problems += variant_problems([(c, b) for b, cs in BIOME_CREATURES.items() for c in cs])
    for b in BACKDROPS:
        for boss in ("generic", "add"):
            for label, colour in zip(("body", "accent"), BOSSES[boss]):
                d = distance(colour, BACKDROPS[b]["field"])
                if d < MIN_FROM_GROUND:
                    problems.append("boss_%s %s %s is %.0f from %s (min %.0f)"
                                    % (boss, label, colour, d, b, MIN_FROM_GROUND))

    # 4. The cat clears every ground.
    for b, tones in BACKDROPS.items():
        d = distance(CAT_BODY, tones["field"])
        if d < MIN_FROM_GROUND:
            problems.append("THE CAT IS LOST: body %s is %.0f from %s %s"
                            % (CAT_BODY, d, b, tones["field"]))

    # 5. Text on paper is readable; the five tab families are distinct.
    if distance(UI["text_muted"], UI["panel"]) < 35.0:
        problems.append("UNREADABLE: muted text %s on the panel %s" % (UI["text_muted"], UI["panel"]))
    tabs = [UI[k] for k in ("tab_upgrades", "tab_reawaken", "tab_realms", "tab_catches", "tab_awards")]
    for i in range(len(tabs)):
        for j in range(i + 1, len(tabs)):
            if distance(tabs[i], tabs[j]) < 20.0:
                problems.append("TABS COLLIDE: %s and %s" % (tabs[i], tabs[j]))

    # 6. The plum screens are readable (WCAG 2), focus shows, buy and threat differ in lightness.
    problems += plum_problems()

    # 8. No shade used twice (designer, 2026-11-09): every character its own body colour, every accent its own.
    problems += unique_problems()

    # 7. The UI on plum (R3): every text on every ground the screens draw it on.
    for fg, bg, need, why in UI_PAIRS:
        r = contrast(UI[fg], UI[bg])
        if r < need:
            problems.append("LOW CONTRAST: ui %s %s on %s %s is %.2f:1, %s needs %.1f" % (fg, UI[fg], bg, UI[bg], r, why, need))
    return problems


UNSHIPPED = {"squirrel", "ghost_mouse", "void_mouse"}     # drawn, but no tier spawns them


def characters() -> dict:
    """name -> (body, accent) for every character on the field: the enemies that ship, the bosses, the player."""
    out = {"enemy " + k: v for k, v in ENEMIES.items() if k not in UNSHIPPED}
    out.update({"boss " + k: v for k, v in BOSSES.items()})
    out["the cat"] = (CAT_BODY, CAT_ACCENT)
    out["the kitten"] = (PLAYER["kitten_body"], PLAYER["kitten_accent"])
    return out


def unique_problems() -> list[str]:
    bad = []
    chars = characters()
    names = sorted(chars)
    # The cat's pale belly is the largest colour on the armoured stages (2, 3): a player colour too. The tint is
    # sweet_shapes.tint()'s (body 60 % toward its cream), written out here so palette.py imports nothing.
    belly = mix(CAT_BODY, "#FBF7EC", 0.60)
    bodies = [(n, chars[n][0]) for n in names] + [("the cat's stripes", CAT_ACCENT), ("the cat's belly", belly)]
    for i in range(len(bodies)):
        for j in range(i + 1, len(bodies)):
            d = distance(bodies[i][1], bodies[j][1])
            if d < UNIQUE_MIN:
                bad.append("SAME SHADE: %s %s and %s %s are %.1f apart (min %.0f)"
                           % (bodies[i][0], bodies[i][1], bodies[j][0], bodies[j][1], d, UNIQUE_MIN))
    for n in names:
        if n == "the cat":
            continue
        acc = chars[n][1]
        others = [(m, chars[m][0]) for m in names if m != n] + [(m + "'s accent", chars[m][1]) for m in names
                                                                if m not in (n, "the cat")] + [("the cat's stripes", CAT_ACCENT)]
        for m, c in others:
            d = distance(acc, c)
            if d < ACCENT_MIN:
                bad.append("SAME SHADE: %s's accent %s and %s %s are %.1f apart (min %.0f)" % (n, acc, m, c, d, ACCENT_MIN))
    return bad


def _luminance(hex_colour: str) -> float:
    def lin(c: int) -> float:
        c = c / 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = (int(hex_colour[i:i + 2], 16) for i in (1, 3, 5))
    return 0.2126 * lin(r) + 0.7152 * lin(g) + 0.0722 * lin(b)


def contrast(a: str, b: str) -> float:
    """WCAG 2 contrast ratio, 1-21."""
    la, lb = sorted((_luminance(a), _luminance(b)), reverse=True)
    return (la + 0.05) / (lb + 0.05)


# (text, ground, minimum, why): the pairs the plum screens actually draw.
PLUM_PAIRS = [
    ("bone", "base", MIN_TEXT_CONTRAST, "text on a panel"),
    ("bone", "raised", MIN_TEXT_CONTRAST, "text on a row"),
    ("lilac", "base", MIN_TEXT_CONTRAST, "secondary text on a panel"),
    ("lilac", "raised", MIN_TEXT_CONTRAST, "secondary text on a row"),
    ("base", "butter", MIN_TEXT_CONTRAST, "a selected button's label"),
    ("base", "catnip", MIN_TEXT_CONTRAST, "a buy button's label"),
    ("base", "stardust", MIN_TEXT_CONTRAST, "a Stardust button's label"),
    ("bone", "crimson", MIN_TEXT_CONTRAST, "a boss banner's text"),
    ("butter", "base", MIN_LARGE_CONTRAST, "the focus ring on a panel"),
    ("butter", "raised", MIN_LARGE_CONTRAST, "the focus ring on a row"),
]


# (text, ground, minimum, why): the UI pairs on plum (rule 7). Disabled text is meant to recede: large-text contrast.
UI_PAIRS = [
    ("text", "panel", MIN_TEXT_CONTRAST, "text on a panel"),
    ("text", "button", MIN_TEXT_CONTRAST, "a button's label"),
    ("text", "button_hover", MIN_TEXT_CONTRAST, "a hovered button's label"),
    ("text", "button_pressed", MIN_TEXT_CONTRAST, "a pressed button's label"),
    ("text", "slot", MIN_TEXT_CONTRAST, "a keycap's label"),
    ("text_muted", "panel", MIN_TEXT_CONTRAST, "secondary text"),
    ("text_muted", "button", MIN_TEXT_CONTRAST, "a row's second line"),
    ("text_disabled", "button_disabled", MIN_TEXT_CONTRAST, "a locked row: dimmer than a live one, still read (R4e)"),
    ("good", "panel", MIN_LARGE_CONTRAST, "a good status line"),
    ("boss_banner_text", "boss_banner", MIN_TEXT_CONTRAST, "the boss banner"),
    ("card_ink", "card_plate", MIN_TEXT_CONTRAST, "a card's ink text on its body"),
    ("selected_text", "selected", MIN_TEXT_CONTRAST, "a label on the selected plate"),
    ("focus", "panel", MIN_LARGE_CONTRAST, "the focus ring on a panel"),
    ("focus", "button", MIN_LARGE_CONTRAST, "the focus ring beside a row"),
    ("catnip", "panel", MIN_LARGE_CONTRAST, "a Catnip amount"),
    ("stardust", "panel", MIN_LARGE_CONTRAST, "a Stardust amount"),
]


def plum_problems() -> list[str]:
    bad = []
    if INK != PLUM["base"]:
        bad.append("INK %s is not Plum base %s (plum everywhere, D14)" % (INK, PLUM["base"]))
    for fg, bg, need, why in PLUM_PAIRS:
        r = contrast(PLUM[fg], PLUM[bg])
        if r < need:
            bad.append("LOW CONTRAST: %s %s on %s %s is %.2f:1, %s needs %.1f" % (fg, PLUM[fg], bg, PLUM[bg], r, why, need))
    # Buy and threat must differ in lightness, not hue alone (red/green colour blindness).
    la, lb = sorted((_luminance(PLUM["catnip"]), _luminance(PLUM["crimson"])), reverse=True)
    if (la + 0.05) / (lb + 0.05) < 1.5:
        bad.append("COLOUR-BLIND: catnip and crimson differ by hue only")
    return bad


def as_dict():
    return {
        "stacklands": STACKLANDS,
        "ink": INK, "eye": EYE, "gold": GOLD, "stardust": STARDUST,
        "alert": ALERT, "disabled": DISABLED,
        "player": PLAYER,
        "enemies": dict((k, {"body": v[0], "accent": v[1]}) for k, v in ENEMIES.items()),
        "bosses": dict((k, {"body": v[0], "accent": v[1]}) for k, v in BOSSES.items()),
        "backdrops": BACKDROPS,
        "ui": UI, "sky": SKY, "materials": MATERIALS, "props": PROPS, "gear": GEAR,
        "plum": PLUM,
    }


# --- the runtime mirror ------------------------------------------------------

# PORT: palette.gd is written to build/art/ (tools/art/check_art.py compares it with the Godot repo's).
GD_PATH = pathlib.Path(__file__).resolve().parent.parent.parent / "build" / "art" / "palette.gd"


def _q(hex_colour: str) -> str:
    return '"%s"' % hex_colour


def gdscript() -> str:
    """scripts/systems/palette.gd, generated. The HUD, the tree, the pick
    screen and the boss banner read their colours from here, so a colour in a
    .gd file is the SAME colour the art was drawn in. Regenerated by
    `py tools/palette.py --gd`; ci/check_mirror.py fails if it is stale."""
    out = []
    out.append("extends RefCounted")
    out.append("class_name Palette")
    out.append("## GENERATED by `py tools/palette.py --gd` - do not edit by hand.")
    out.append("##")
    out.append("## The runtime half of tools/palette.py: every colour a script draws")
    out.append("## comes from here, so the HUD is painted in the same Stacklands paper")
    out.append("## the art was generated in. See docs/pawpocalypse-colour-reference.md.")
    out.append("")
    out.append("const INK := Color(%s)" % _q(INK))
    out.append("const EYE := Color(%s)" % _q(EYE))
    out.append("const GOLD := Color(%s)" % _q(GOLD))
    out.append("const STARDUST := Color(%s)" % _q(STARDUST))
    out.append("const ALERT := Color(%s)" % _q(ALERT))
    out.append("const DISABLED := Color(%s)" % _q(DISABLED))
    out.append("")
    # One flat const per colour FIRST: a `const` in another script may only be
    # built from constant expressions, and indexing a Dictionary is not one -
    # `const X := Palette.UI["good"]` is a parse error, `Palette.UI_GOOD` is
    # fine. The dictionaries below are for lookups by name at runtime.
    out.append("## Flat constants, for use in other scripts' `const` declarations.")
    for name in FAMILY_NAMES:
        out.append("const %s_FACE := Color(%s)" % (name.upper(), _q(face(name))))
        out.append("const %s_HEADER := Color(%s)" % (name.upper(), _q(header(name))))
    for k, v in UI.items():
        out.append("const UI_%s := Color(%s)" % (k.upper(), _q(v)))
    for k, v in SKY.items():
        out.append("const SKY_%s := Color(%s)" % (k.upper(), _q(v)))
    for k, v in PLUM.items():
        out.append("const PLUM_%s := Color(%s)" % (k.upper(), _q(v)))
    out.append("")
    out.append("## The Stacklands card families, [face, header].")
    out.append("const FAMILIES := {")
    for name in FAMILY_NAMES:
        out.append('\t"%s": [Color(%s), Color(%s)],' % (name, _q(face(name)), _q(header(name))))
    out.append("}")
    out.append("")
    out.append("const UI := {")
    for k, v in UI.items():
        out.append('\t"%s": Color(%s),' % (k, _q(v)))
    out.append("}")
    out.append("")
    out.append("const SKY := {")
    for k, v in SKY.items():
        out.append('\t"%s": Color(%s),' % (k, _q(v)))
    out.append("}")
    out.append("")
    out.append("## Per biome, keyed by TierData.id: field, detail, patch, puddle (transparent = dry).")
    out.append("const BACKDROPS := {")
    for key, tones in BACKDROPS.items():
        tier = "tier_" + key.split("_")[1]
        puddle = "Color(%s)" % _q(tones["puddle"]) if tones["puddle"] else "Color(0, 0, 0, 0)"
        out.append('\t"%s": {"field": Color(%s), "detail": Color(%s), "patch": Color(%s), "puddle": %s, "dark": %s},'
                   % (tier, _q(tones["field"]), _q(tones["detail"]), _q(tones["patch"]), puddle,
                      "true" if tones["register"] == "dark" else "false"))
    out.append("}")
    out.append("")
    out.append("const PLAYER := {")
    for k, v in PLAYER.items():
        out.append('\t"%s": Color(%s),' % (k, _q(v)))
    out.append("}")
    out.append("")
    out.append("")
    out.append("static func family(name: String, part: int) -> Color:")
    out.append("\treturn (FAMILIES[name] as Array)[part]")
    out.append("")
    out.append("")
    out.append("static func ui(key: String) -> Color:")
    out.append("\treturn UI[key]")
    out.append("")
    out.append("")
    out.append("static func backdrop(tier_id: String) -> Dictionary:")
    out.append('\treturn BACKDROPS.get(tier_id, BACKDROPS["tier_1"])')
    return "\n".join(out)


# --- the reference tables ----------------------------------------------------


def markdown() -> str:
    def row(*cells):
        return "| " + " | ".join(str(c) for c in cells) + " |"

    def hs(c):
        h, s, v = to_hsv(c)
        return "H%.0f S%.0f V%.0f" % (h, s, v)

    L = []
    L.append("### Stacklands, measured")
    L.append("")
    L.append(row("name", "hex", "HSV", "what it is in Stacklands"))
    L.append(row("---", "---", "---", "---"))
    notes = {
        "paper": "quest panel, tooltip, every dialog", "paper_band": "a panel's section header row",
        "paper_bright": "brightest cream in the quantisation", "sand": "island board; 13.5% of all pixels",
        "sand_detail": "doodles on the sand", "meadow": "the meadow board", "meadow_detail": "bush and grass lines",
        "world_green": "the world outside the board (vignette)", "night_green": "Cardopedia backdrop, light ray",
        "night_deep": "Cardopedia backdrop, dark ray", "ocean": "the sea beside the island",
        "ink": "every outline and letter", "disabled": "a completed quest's text", "alert": "the exclamation badge",
        "disc": "the villager's icon disc",
    }
    for k, v in STACKLANDS.items():
        if isinstance(v, tuple):
            continue
        L.append(row(k, "`%s`" % v, hs(v), notes.get(k, "")))
    L.append("")
    L.append("### The eleven card families")
    L.append("")
    L.append(row("family", "face", "header", "Stacklands cards"))
    L.append(row("---", "---", "---", "---"))
    fam_notes = {
        "red": "Rat, Seagull - enemies", "coral": "Shed, House, Campfire - buildings",
        "orange": "Berry, Banana, Apple - food", "yellow": "Coin, Shell - money", "blue": "Idea",
        "green": "Cod - fresh things", "slate": "Stone, Stick, Flint, Wood", "dark": "Tree, Rock, Driftwood",
        "grey": "Corpse", "cream": "Villager, Militia, Fisher", "black": "booster packs",
    }
    for name in FAMILY_NAMES:
        f, h = FAMILIES[name]
        L.append(row(name, "`%s` %s" % (f, hs(f)), "`%s` %s" % (h, hs(h)), fam_notes[name]))
    L.append("")
    L.append("### Core")
    L.append("")
    L.append(row("role", "hex", "derived from"))
    L.append(row("---", "---", "---"))
    L.append(row("ink", "`%s`" % INK, "Stacklands ink"))
    L.append(row("eye white", "`%s`" % EYE, "grey face"))
    L.append(row("Catnip", "`%s`" % GOLD, "yellow header"))
    L.append(row("Stardust", "`%s`" % STARDUST, "blue header"))
    L.append(row("alert / badge", "`%s`" % ALERT, "Stacklands alert"))
    L.append(row("disabled", "`%s`" % DISABLED, "Stacklands disabled"))
    L.append("")
    L.append("### The player")
    L.append("")
    L.append(row("part", "hex", "HSV", "why"))
    L.append(row("---", "---", "---", "---"))
    why = {"cat_body": "V66 warm grey: the one mid-value body, clears both registers",
           "cat_accent": "dark face - the only near-black fill in the cast",
           "cat_nose": "coral header", "kitten_body": "tan, its own", "kitten_accent": "tan shaded",
           "unicorn_body": "grey face"}
    for k, v in PLAYER.items():
        L.append(row(k, "`%s`" % v, hs(v), why[k]))
    L.append("")
    L.append("### Biomes")
    L.append("")
    L.append(row("biome", "register", "field", "detail", "patch", "puddle", "creatures"))
    L.append(row("---", "---", "---", "---", "---", "---", "---"))
    for k, t in BACKDROPS.items():
        L.append(row(k, t["register"], "`%s` %s" % (t["field"], hs(t["field"])), "`%s`" % t["detail"],
                     "`%s`" % t["patch"], "`%s`" % t["puddle"] if t["puddle"] else "dry",
                     ", ".join(BIOME_CREATURES[k])))
    L.append("")
    L.append("### Enemies")
    L.append("")
    L.append(row("creature", "body", "accent", "family", "from cat accent"))
    L.append(row("---", "---", "---", "---", "---"))
    inv = {}
    for name in FAMILY_NAMES:
        inv[FAMILIES[name][0]] = name + " face"
        inv[FAMILIES[name][1]] = name + " header"
    for k, (b, a) in ENEMIES.items():
        L.append(row(k, "`%s`" % b, "`%s`" % a, "%s / %s" % (inv.get(b, "custom"), inv.get(a, "custom")),
                     "%.0f" % distance(a, CAT_ACCENT)))
    L.append("")
    L.append("### Bosses")
    L.append("")
    L.append(row("boss", "body", "accent", "family"))
    L.append(row("---", "---", "---", "---"))
    for k, (b, a) in BOSSES.items():
        L.append(row(k, "`%s`" % b, "`%s`" % a, "%s / %s" % (inv.get(b, "custom"), inv.get(a, "custom"))))
    L.append("")
    L.append("### UI")
    L.append("")
    L.append(row("role", "hex", "HSV"))
    L.append(row("---", "---", "---"))
    for k, v in UI.items():
        L.append(row(k, "`%s`" % v, hs(v)))
    L.append("")
    L.append("### The skill-tree sky")
    L.append("")
    L.append(row("role", "hex", "HSV"))
    L.append(row("---", "---", "---"))
    for k, v in SKY.items():
        L.append(row(k, "`%s`" % v, hs(v)))
    L.append("")
    L.append("### Icon materials")
    L.append("")
    L.append(row("material", "hex"))
    L.append(row("---", "---"))
    for k, v in MATERIALS.items():
        L.append(row(k, ", ".join("`%s`" % c for c in v) if isinstance(v, list) else "`%s`" % v))
    return "\n".join(L)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--gd", action="store_true", help="write scripts/systems/palette.gd")
    ap.add_argument("--md", action="store_true", help="print the reference tables")
    args = ap.parse_args()
    if args.json:
        print(json.dumps(as_dict(), indent=2))
        return 0
    if args.md:
        print(markdown())
        return 0
    if args.gd:
        problems = check()
        if problems:
            print("refusing to write palette.gd over %d problem(s)" % len(problems))
            for p in problems:
                print("  " + p)
            return 1
        GD_PATH.write_text(gdscript() + "\n", encoding="utf-8", newline="\n")
        print("wrote %s" % GD_PATH)
        return 0

    def row(name, colour, note=""):
        h, sat, val = to_hsv(colour)
        print("  %-24s %s  H%3.0f S%3.0f%% V%3.0f%%  %s" % (name, colour, h, sat, val, note))

    print("CORE")
    for k, v in (("ink", INK), ("eye", EYE), ("catnip gold", GOLD), ("stardust", STARDUST),
                 ("alert", ALERT), ("disabled", DISABLED)):
        row(k, v)
    print("")
    print("PLAYER")
    for k, v in PLAYER.items():
        row(k, v)
    print("")
    print("ENEMIES   body + accent, each a Stacklands family face/header")
    for k, pair in ENEMIES.items():
        row(k, pair[0], "accent %s  %.0f from cat accent" % (pair[1], distance(pair[1], CAT_ACCENT)))
    print("")
    print("BOSSES")
    for k, pair in BOSSES.items():
        row(k, pair[0], "accent %s" % pair[1])
    print("")
    print("BACKDROPS  (flat field + flat detail, no gradients; light >= V86, dark <= V50)")
    for k, tones in BACKDROPS.items():
        row(k, tones["field"], "%-5s detail %s patch %s puddle %s"
            % (tones["register"], tones["detail"], tones["patch"], tones["puddle"] or "-"))
    print("")
    print("UI")
    for k, v in UI.items():
        row(k, v)
    print("")
    print("SKY")
    for k, v in SKY.items():
        row(k, v)

    problems = check()
    print("")
    print("-" * 68)
    if problems:
        print("%d PROBLEM(S):" % len(problems))
        for p in problems:
            print("  " + p)
        return 1
    print("Passes: every ground in a register, no accent reads as the cat, nothing")
    print("vanishes into the ground it stands on, the cat clears all twelve floors.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
