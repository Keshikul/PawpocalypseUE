"""Look-lint: the field's drawings follow the look spec (docs/WORKLIST-RESTYLE-2026-11-07.md §4 and §6 step 2; R5).

    py tools/art/look_lint.py               # lint the field's drawings in SourceArt/svg; exit 1 on a new or stale fault
    py tools/art/look_lint.py --self-test   # every rule FAILS on a fault planted into a copy (nothing else is written)
    py tools/art/look_lint.py --known       # print the KNOWN table as the art stands (to shrink it after a redraw)

The field's drawings are SourceArt/svg/creatures, bosses, player (the cat's stages and base, the weapons, the kitten) and
props (the ground shadow is a PNG and is not drawn). The rules:

  palette  palette.py's own rules pass (registers, distance from the cat, every token clears every ground it ships on).
  colour   every fill and stroke is a palette.py token, or a token shaded toward INK or tinted toward the grammar's cream
           by sweet_shapes.tint()/shade(): the generators are run in this process with tint() recorded, and a derived
           colour counts only when its base and its target are themselves allowed (a tinted literal is still a literal).
  ink      every outlined shape is outlined in INK, at the grammar's one width (sweet_shapes.STROKE, set once on the
           group; no shape sets its own). Lines (whiskers, toes, manes) are marks, not outlines: the colour rule only.
  flat     no gradient, pattern, filter, mask, clip, image or opacity anywhere.
  ground   a creature's body (its largest fill on a 128 px raster) clears the field of every floor it ships on by
           palette.MIN_FROM_GROUND (TierData's spawn pools, golden enemies and bosses; the generic boss, its adds, the
           cat's stages and the kitten: all twelve floors). palette.py checks the tokens; this checks what is DRAWN.
  unique   no shade used twice (designer, 2026-11-09): the drawn bodies of two different characters that ship (the
           enemy kinds, the bosses, the cat, the kitten) are palette.UNIQUE_MIN apart; palette.py's rule 8 checks the tokens.
  crossing no outline crosses itself: a polygon whose edges cross fills as a bow-tie (2026-11-10: every tube's round
           end was built backwards and drew a "fan" at the tip of each arm, leg, neck and tail; designer: "the point is
           flipped so the outline doesn't continue but looks like a fan at the end").
  eyes     drawn at 56 px (the smallest sprite_target_px), a creature's eye whites cover at least EYE_MIN_PX56 pixels:
           the cast's smallest when the lint was written, rounded down, so a redraw never shrinks eyes below what reads.

STICKER BOX (D18; 2026-11-18). A drawing whose SVG says data-look="sticker" (tools/art/sticker_art.py: the approved
Bestiary's cast, recorded) is held to the concept's rules where they differ from the grammar's:
  ink      each outlined shape carries its own width (the concept's line, eye and small-part widths), none wider than the
           drawing's line (data-ink), all in INK.
  colour   the concept's colour table (palette.STICKER_CREATURES) and mix() of it, recorded as derivations.
  eyes     Sticker's eye white (#FFFFFF); the floor is EYE_MIN_PX56_STICKER, the approved cast's smallest, rounded down.
  unique   within a realm, between enemies (UNIQUE_APPROVED: the pairs the designer approved as drawn).

KNOWN is a ratchet, not an allowance: the faults the art had when the lint was written (2026-11-09), which R5's redraw
removes. A fault not in it fails; an entry the art no longer produces fails as stale, so the table only ever shrinks.
"""

from __future__ import annotations

import argparse
import csv
import io
import math
import pathlib
import re
import shutil
import sys

HERE = pathlib.Path(__file__).resolve().parent
ROOT = HERE.parent.parent
sys.path.insert(0, str(HERE))
import palette as PAL      # noqa: E402
import sweet_shapes as S   # noqa: E402

SVG = ROOT / "SourceArt" / "svg"
FAMILIES = ("creatures", "bosses", "player", "props")
EYE_MIN_PX56 = 64.0        # 2026-11-09: the seagull's 65.8 px, the smallest in the cast, rounded down
# Sticker Box (D18, 2026-11-18): a Sticker drawing's eyes are measured at the smallest size the game draws IT (display_px:
# an enemy at its sprite_target_px, a boss at 0.9 of its 256, the add at 45, the cat at 175, a kitten at 96), not at one
# 56 px for all: the concept sizes its eyes to the drawing (a boss's are small in a big body). The floor is the approved
# cast's smallest when the rule was written, rounded down (the bat, 27.7 px at 56): the designer approved the cast as
# drawn ("The enemies all seem good"), and a redraw never shrinks eyes below what reads.
EYE_MIN_PX_STICKER = 27.0
EYE_PX = 56                # the smallest sprite_target_px in Data/EnemyData.csv
HEX = re.compile(r"#[0-9A-Fa-f]{6}\b")
CREAM_TARGET = "#FBF7EC"   # sweet_shapes.tint()'s default direction: a target, never a fill

# "<file> <rule> <subject> [<subject> ...]": the faults the art had on 2026-11-09 (R5 removes them; `--known` reprints).
# colour: the literals the generators drew before the restyle (the mounts' coats and manes, the armour's steels, five
# props). ground: the pale coats (the armoured cat's largest fill is its pale belly; the unicorns') on the light floors;
# the void unicorn on the dark. Removed 2026-11-09 by the one-colour palette: the Park Tyrant on its Park, the cream
# kitten on four floors, the flying mouse's literal wings.
KNOWN = """
"""
# Emptied 2026-11-18: the old cat's pale coats on the light floors went with the old cat (D18).

# APPROVED (D18, 2026-11-18): bodies closer to a floor than MIN_FROM_GROUND as the designer approved them. The Bestiary
# stands each realm's cast, and the cat at that realm's stage, on the realm's ground ("The enemies all seem good"); in
# Sticker Box the ink line and the white rim, not the body, part a creature from its floor (E2's question about the pale
# Kitchen and Vet floors is still open). The same format and the same ratchet as KNOWN: a pair not listed fails, a listed
# pair the art no longer has fails as stale, so the table only shrinks. Closest: the add on the Mythic floor (5), the
# Litter King on the Litterbox (7), the standing cat on the Alley (11).
APPROVED = """
boss_add.svg ground tier_7_portal tier_8_mythic
boss_litter_king.svg ground tier_12_litterbox
enemy_bat.svg ground tier_7_portal tier_8_mythic
enemy_factory_rat.svg ground tier_11_factory
enemy_factory_robot_mouse.svg ground tier_11_factory
enemy_park_rat.svg ground tier_2_alley
enemy_rooftop_pigeon.svg ground tier_4_rooftops tier_5_sewers
enemy_rooftop_seagull.svg ground tier_4_rooftops
enemy_sewer_rat.svg ground tier_5_sewers
enemy_vet_hamster.svg ground tier_10_vets
player_cat_base.svg ground tier_11_factory tier_2_alley tier_5_sewers tier_7_portal
player_cat_stage_1.svg ground tier_11_factory tier_2_alley tier_5_sewers tier_7_portal
player_cat_stage_2.svg ground tier_11_factory tier_2_alley tier_5_sewers tier_7_portal
player_cat_stage_3.svg ground tier_11_factory tier_2_alley tier_5_sewers tier_7_portal
player_cat_stage_4.svg ground tier_10_vets tier_9_kitchen
player_cat_stage_5.svg ground tier_10_vets tier_9_kitchen
player_cat_stage_7.svg ground tier_12_litterbox tier_6_turf
"""



def parse_known(text: str) -> set:
    keys = set()
    for line in text.strip().splitlines():
        f, rule, *subjects = line.split()
        keys |= {(f, rule, s) for s in subjects}
    return keys


def format_known(keys) -> str:
    by = {}
    for f, rule, s in keys:
        by.setdefault((f, rule), []).append(s)
    out = []
    for (f, rule), subjects in sorted(by.items()):
        subjects.sort()
        line = "%s %s" % (f, rule)
        for s in subjects:
            if len(line) + 1 + len(s) > 120:
                out.append(line)
                line = "%s %s" % (f, rule)
            line += " " + s
        out.append(line)
    return "\n".join(out)


# --- what is allowed -----------------------------------------------------------------------------------------------

def _hexes(obj, out):
    if isinstance(obj, dict):
        for v in obj.values():
            _hexes(v, out)
    elif isinstance(obj, (list, tuple)):
        for v in obj:
            _hexes(v, out)
    elif isinstance(obj, str) and HEX.fullmatch(obj):
        out.add(obj.upper())
    return out


def tokens() -> set:
    return _hexes(PAL.as_dict(), set()) | {PAL.INK.upper(), PAL.EYE.upper()}


def derivations() -> list:
    """Every (base, target, result) the generators derive with sweet_shapes.tint() (shade() calls it), recorded while
    every batch and prop is drawn in this process. Nothing is written."""
    edges = []
    real = S.tint

    def spy(c, amount, toward=CREAM_TARGET):
        out = real(c, amount, toward)
        edges.append((c.upper(), toward.upper(), out.upper()))
        return out

    S.tint = spy
    import sticker_cast as SC
    import sticker_draw as SD
    real_mix = SD.mix

    def mix_spy(a, b, t):
        out = real_mix(a, b, t)
        edges.append((a.upper(), b.upper(), out.upper()))
        return out

    SD.mix = SC.mix = mix_spy
    try:
        import make_sweet_art as M
        import sweet_biomes as B
        for batch in M.BATCHES.values():
            for _rel, _px, fn in batch():
                fn()
        B.PROPS()
        for _stems, fn in SC.CAST.values():
            SD.draw(fn)
        for _stem, fn in SC.STANDING + SC.STANDING_WEAPONS + SC.MOUNTED + SC.MOUNTED_WEAPONS:
            SD.draw(fn)
    finally:
        S.tint = real
        SD.mix = SC.mix = real_mix
    return edges


def allowed(edges) -> set:
    """The tokens, closed under the recorded derivations whose base and target are both allowed."""
    ok = tokens()
    grew = True
    while grew:
        grew = False
        for base, toward, out in edges:
            if out not in ok and base in ok and (toward in ok or toward == CREAM_TARGET):
                ok.add(out)
                grew = True
    return ok


# --- the rules -------------------------------------------------------------------------------------------------------
# Each returns faults as (file, rule, subject, message); (file, rule, subject) is the key KNOWN lists.

def field_files(root: pathlib.Path = SVG) -> list:
    return [p for fam in FAMILIES for p in sorted((root / fam).glob("*.svg"))]


def colours_of(text: str) -> set:
    return {m.group(1).upper() for m in re.finditer(r'(?:fill|stroke)="(#[0-9A-Fa-f]{6})"', text)}


def colour_faults(name: str, text: str, ok: set) -> list:
    return [(name, "colour", c, "%s is not a palette token or a token's shade or tint" % c)
            for c in sorted(colours_of(text) - ok)]


SHAPE = re.compile(r"<(ellipse|rect|polygon|circle|path)\b([^>]*)/?>")


def grammar_ink_faults(name: str, text: str) -> list:
    bad = []
    g = re.search(r'<g\b[^>]*stroke-width="([\d.]+)"', text)
    if not g or abs(float(g.group(1)) - S.STROKE) > 1e-6:
        bad.append((name, "ink", "width", "the outline width is %s, the grammar's is %.1f"
                    % (g.group(1) if g else "unset", S.STROKE)))
    for m in SHAPE.finditer(text):
        attrs = m.group(2)
        st = re.search(r'stroke="([^"]+)"', attrs)
        if st and st.group(1) != "none" and st.group(1).upper() != PAL.INK.upper():
            bad.append((name, "ink", st.group(1).upper(), "a %s is outlined in %s, not INK %s"
                        % (m.group(1), st.group(1), PAL.INK)))
        if "stroke-width" in attrs:
            bad.append((name, "ink", "own-width", "a %s sets its own outline width" % m.group(1)))
    return bad


def sticker_ink_faults(name: str, text: str) -> list:
    """A Sticker drawing (data-look="sticker"; D18): every outline is INK, each shape carries its own width (the concept's
    line, eye and small-part widths), and none is wider than the drawing's line (data-ink, the line at the drawing's fit)."""
    bad = []
    m = re.search(r'data-ink="([\d.]+)"', text)
    if not m:
        return [(name, "ink", "data-ink", "a Sticker drawing without data-ink")]
    widest = float(m.group(1))
    for m in SHAPE.finditer(text):
        attrs = m.group(2)
        st = re.search(r'stroke="([^"]+)"', attrs)
        if not st or st.group(1) == "none":
            continue
        if st.group(1).upper() != PAL.INK.upper():
            bad.append((name, "ink", st.group(1).upper(), "a %s is outlined in %s, not INK %s" % (m.group(1), st.group(1), PAL.INK)))
        w = re.search(r'stroke-width="([\d.]+)"', attrs)
        if not w:
            bad.append((name, "ink", "unset", "a %s of a Sticker drawing has no outline width" % m.group(1)))
        elif float(w.group(1)) > widest + 0.0051:   # widths are written to 2 decimals, data-ink to 3
            bad.append((name, "ink", "wider", "a %s's outline (%s) is wider than the drawing's line %.3f"
                        % (m.group(1), w.group(1), widest)))
    return bad


def ink_faults(name: str, text: str) -> list:
    if is_sticker(text):
        return sticker_ink_faults(name, text)
    return grammar_ink_faults(name, text)


FLAT_BANNED = ("opacity", "Gradient", "<pattern", "filter", "<mask", "mask=", "clip-path", "<clipPath", "<image", "blur",
               "rgba(", "hsla(")


def flat_faults(name: str, text: str) -> list:
    if is_sticker(text):    # the approved cyber cat's visor is see-through (sticker_draw.GLASS): fill-opacity only
        text = re.sub(r' fill-opacity="[\d.]+"', "", text)
    return [(name, "flat", w, "not flat (%s)" % w) for w in FLAT_BANNED if w in text]


POLYGON = re.compile(r'<polygon points="([^"]+)"')


def _side(a, b, c) -> float:
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])


def _cross(p, q, r, s, eps=1e-9) -> bool:
    """Segments pq and rs cross properly (touching at a point or running along each other is not a crossing)."""
    d1, d2, d3, d4 = _side(r, s, p), _side(r, s, q), _side(p, q, r), _side(p, q, s)
    return ((d1 > eps and d2 < -eps) or (d1 < -eps and d2 > eps)) and ((d3 > eps and d4 < -eps) or (d3 < -eps and d4 > eps))


def crossings(pts) -> int:
    n, found = len(pts), 0
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        for j in range(i + 2, n - (1 if i == 0 else 0)):
            found += _cross(a, b, pts[j], pts[(j + 1) % n])
    return found


def crossing_faults(name: str, text: str) -> list:
    bad = []
    for k, m in enumerate(POLYGON.finditer(text)):
        pts = [tuple(map(float, xy.split(","))) for xy in m.group(1).split()]
        c = crossings(pts)
        if c:
            bad.append((name, "crossing", "polygon-%d" % k, "polygon %d's outline crosses itself %d time(s): it fills as a "
                        "bow-tie (a tube's end cap built backwards draws a fan)" % (k, c)))
    return bad


def is_creature(name: str) -> bool:
    return name.startswith(("enemy_", "boss_", "player_cat_", "kitten_"))


def is_sticker(text: str) -> bool:
    """A drawing of the approved Bestiary (tools/art/sticker_art.py), held to Sticker Box's rules (D18)."""
    return 'data-look="sticker"' in text[:400]


# A Sticker drawing's ellipse carries its own outline width (D18).
ELL = re.compile(r'<ellipse cx="([\d.]+)" cy="([\d.]+)" rx="([\d.]+)" ry="([\d.]+)" fill="(#\w+)" stroke="([#\w]+)"'
                 r'(?: stroke-width="[\d.]+")?/>')
EYE_WHITES = (PAL.EYE.upper(), PAL.STICKER_CREATURES["eye"].upper())   # the grammar's eye white, Sticker's


def eye_px56(text: str) -> float:
    """The eye whites' area at 56 px, from the geometry: an eye is sweet_shapes.eyes()'s EYE ellipse outlined in INK
    with its INK pupil drawn next, inside it (not a raster count: a ghost's white body is the colour of an eye)."""
    box = float(re.search(r'viewBox="0 0 ([\d.]+)', text).group(1))
    es = [(float(a), float(b), float(c), float(d), f.upper(), s.upper()) for a, b, c, d, f, s in ELL.findall(text)]
    area = 0.0
    # The pupil follows its white within a few shapes: the grammar draws white, pupil; the concept's cat and bosses draw
    # both whites, then both pupils (D18).
    for i, e in enumerate(es):
        if e[4] in EYE_WHITES and e[5] == PAL.INK.upper() and any(
                nxt[4] == PAL.INK.upper() and nxt[5] == "NONE" and math.hypot(nxt[0] - e[0], nxt[1] - e[1]) < max(e[2], e[3])
                for nxt in es[i + 1:i + 5]):
            area += math.pi * e[2] * e[3]
    return area * (EYE_PX / box) ** 2


_DISPLAY = {}


def boss_scale() -> float:
    """PawLook::BossScale (Source/Pawpocalypse/Realm/PawFieldLook.h), as tools/pack_atlas.py reads it."""
    import re
    text = (ROOT / "Source" / "Pawpocalypse" / "Realm" / "PawFieldLook.h").read_text(encoding="utf-8")
    return float(re.search(r"constexpr double BossScale = ([\d.]+);", text).group(1))


def display_px(stem: str) -> float:
    """The smallest size the game draws a drawing at, in px (tools/pack_atlas.py's sizes, without its pulse and squash)."""
    if not _DISPLAY:
        for r in csv.DictReader(open(ROOT / "Data" / "EnemyData.csv", newline="", encoding="utf-8")):
            s = pathlib.PurePosixPath(r["texture_path"]).stem
            _DISPLAY[s] = min(_DISPLAY.get(s, 1e9), float(r["sprite_target_px"]))
        for r in csv.DictReader(open(ROOT / "Data" / "BossData.csv", newline="", encoding="utf-8")):
            _DISPLAY[pathlib.PurePosixPath(r["texture_path"]).stem] = 256.0 * boss_scale()  # PawLook::BossScale
        _DISPLAY.update({"boss_generic": 256.0 * boss_scale(), "boss_add": 45.0, "kitten_basic": 96.0, "player_cat_base": 175.0})
        _DISPLAY.update({"player_cat_stage_%d" % i: 175.0 for i in range(1, 8)})
    return _DISPLAY.get(stem, float(EYE_PX))


def eye_faults(name: str, text: str) -> list:
    if not is_creature(name):
        return []
    a = eye_px56(text)
    if is_sticker(text):
        px = display_px(pathlib.PurePosixPath(name).stem)
        a *= (px / EYE_PX) ** 2
        if a < EYE_MIN_PX_STICKER:
            return [(name, "eyes", "small", "its eyes cover %.1f px at %.0f px, the smallest it is drawn; the floor is %.0f"
                     % (a, px, EYE_MIN_PX_STICKER))]
        return []
    if a < EYE_MIN_PX56:
        return [(name, "eyes", "small", "its eyes cover %.1f px at %d px; the floor is %.0f" % (a, EYE_PX, EYE_MIN_PX56))]
    return []


def grounds() -> dict:
    """Drawing stem -> the floors (palette.BACKDROPS keys) it ships on."""
    floor_of = {"_".join(k.split("_")[:2]): k for k in PAL.BACKDROPS}
    with open(ROOT / "Data" / "EnemyData.csv", newline="", encoding="utf-8") as f:
        enemy = {r["id"]: pathlib.PurePosixPath(r["texture_path"]).stem for r in csv.DictReader(f)}
    with open(ROOT / "Data" / "BossData.csv", newline="", encoding="utf-8") as f:
        boss = {r["id"]: pathlib.PurePosixPath(r["texture_path"]).stem for r in csv.DictReader(f)}
    out = {}
    with open(ROOT / "Data" / "TierData.csv", newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            floor = floor_of[r["id"]]
            for i in [i for i in r["spawn_pool"].split(";") if i] + ([r["golden_enemy"]] if r["golden_enemy"] else []):
                if i not in enemy:
                    raise SystemExit("look_lint: TierData %s spawns %r, which EnemyData does not have" % (r["id"], i))
                out.setdefault(enemy[i], set()).add(floor)
            if r["boss_data"]:
                out.setdefault(boss[r["boss_data"]], set()).add(floor)
    for stem in ("boss_generic", "boss_add", "kitten_basic", "player_cat_base") + \
            tuple("player_cat_stage_%d" % i for i in range(1, 8)):
        out[stem] = set(PAL.BACKDROPS)
    return out


def body_colour(path: pathlib.Path, drawn: set) -> str:
    """The largest fill on a 128 px raster (exact colours only: the antialiased edge counts for nothing)."""
    import resvg_py
    from PIL import Image, ImageChops
    png = bytes(resvg_py.svg_to_bytes(svg_path=str(path), width=128, height=128))
    counts = {}
    for n, (r, g, b, a) in Image.open(io.BytesIO(png)).convert("RGBA").getcolors(1 << 16):
        c = "#%02X%02X%02X" % (r, g, b)
        if a == 255 and c in drawn and c not in (PAL.INK.upper(),) + EYE_WHITES:
            counts[c] = counts.get(c, 0) + n
    return max(counts, key=counts.get) if counts else ""


def ground_faults(path: pathlib.Path, text: str, where: dict) -> list:
    if not is_creature(path.name) or path.stem not in where:
        return []          # a prop, or a variant no tier spawns: no floor to clear
    body = body_colour(path, colours_of(text))
    if not body:
        return [(path.name, "ground", "body", "no body colour found")]
    bad = []
    for floor in sorted(where[path.stem]):
        d = PAL.distance(body, PAL.BACKDROPS[floor]["field"])
        if d < PAL.MIN_FROM_GROUND:
            bad.append((path.name, "ground", floor, "its body %s is %.0f from the %s floor %s (min %.0f)"
                        % (body, d, floor, PAL.BACKDROPS[floor]["field"], PAL.MIN_FROM_GROUND)))
    return bad


RATS = ("enemy_rat", "enemy_park_rat", "enemy_sewer_rat", "enemy_factory_rat")
PAIRS = {"enemy_mouse": "mouse", "enemy_backyard_mouse": "mouse", "enemy_moth": "moth", "enemy_backyard_moth": "moth",
         "enemy_dog": "dog", "enemy_alley_dog": "dog", "enemy_goose": "goose", "enemy_park_goose": "goose"}


def species_of(stem: str) -> str:
    """Which character a drawing is: a kind's biome copies are one enemy (one colour); the cat's stages are the player."""
    if stem.endswith("_gristle"):
        return stem          # the approved Bestiary draws a gristle per tier, each its own colour (sticker_cast.gristle_N)
    if stem in RATS:
        return "rat"
    if stem.startswith("player_cat_"):
        return "the cat"
    return PAIRS.get(stem, stem)


# "No shade used twice" (designer, 2026-11-09) in Sticker Box (D18). The designer approved the Bestiary's cast as drawn
# (2026-11-18: "The enemies all seem good"): the die-cut rim, the outline and the silhouette tell its characters apart, and
# a shade may come back in another realm (the moth's wing is the add's body; the Void God's body is the void unicorn's).
# So the rule holds where two characters are on the field together: within one realm (its spawn pool, its golden enemy,
# its boss, and the boss's adds where a boss is), between enemies, never the cat (the cursor, the biggest thing there).
# These pairs share a realm and are closer than UNIQUE_MIN as the designer approved them; any other pair fails, and an
# entry the art no longer has fails as stale (the table only shrinks).
UNIQUE_APPROVED = {
    ("enemy_alley_vole.svg", "enemy_backyard_mouse.svg"),        # the Backyard spawns the Alley's vole (TierData)
    ("enemy_park_rat.svg", "enemy_rooftop_pigeon.svg"),          # the Park spawns the Rooftops' pigeon
    ("boss_add.svg", "enemy_bat.svg"),
    ("boss_add.svg", "enemy_mouse_flying.svg"),
    ("enemy_kitchen_cockroach.svg", "enemy_kitchen_gristle.svg"),
    ("enemy_rooftop_pigeon.svg", "enemy_rooftop_seagull.svg"),
}


def is_player(stem: str) -> bool:
    return stem.startswith(("player_", "kitten_"))


def unique_faults(files, where) -> list:
    """No shade used twice: the drawn bodies of two different enemies (bosses and adds included) that are on the field
    together are at least palette.UNIQUE_MIN apart (UNIQUE_APPROVED excepted). An add is where a boss is."""
    bosses = set().union(*[f for s, f in where.items() if s.startswith("boss_") and s not in ("boss_add", "boss_generic")])
    bodies = []
    for p in files:
        if is_creature(p.name) and p.stem in where and not is_player(p.stem):
            b = body_colour(p, colours_of(p.read_text(encoding="utf-8")))
            if b:
                floors = bosses if p.stem == "boss_add" else where[p.stem]
                bodies.append((p, species_of(p.stem), b, floors))
    bad, seen = [], set()
    for i in range(len(bodies)):
        for j in range(i + 1, len(bodies)):
            (pa, sa, ba, fa), (pb, sb, bb, fb) = bodies[i], bodies[j]
            d = PAL.distance(ba, bb)
            if sa == sb or d >= PAL.UNIQUE_MIN or not (fa & fb):
                continue
            pair = tuple(sorted((pa.name, pb.name)))
            seen.add(pair)
            if pair not in UNIQUE_APPROVED:
                bad.append((pa.name, "unique", pb.name, "its body %s is %.1f from %s's %s on the %s floor (min %.0f): "
                            "the same shade" % (ba, d, pb.name, bb, sorted(fa & fb)[0], PAL.UNIQUE_MIN)))
    names = {p.name for p, *_ in bodies}
    for pair in sorted(UNIQUE_APPROVED - seen):
        if set(pair) <= names:
            bad.append((pair[0], "unique", pair[1], "UNIQUE_APPROVED lists %s and %s, which are no longer the same "
                        "shade (remove it: the table only shrinks)" % pair))
    return bad


RIG_LOOP = 2.5          # seconds: every hz in the rig is a multiple of 0.4 (sweet_anatomy)
RIG_STEPS = 50
RIG_TOUCH_PX = 3        # overlapping pixels at 128 px that count as touching


def rig_faults(root: pathlib.Path = SVG) -> list:
    """The rig (R5): layers of one drawing that share a draw depth (sweet_anatomy.limb depth) never overlap, at any
    moment of a loop; overlapping masked layers at one depth would fight (and too close in depth drew black)."""
    import resvg_py
    from PIL import Image, ImageChops
    layers = {}
    for f in sorted((root / "rigs").glob("*__*.svg")):
        text = f.read_text(encoding="utf-8")
        a = dict(re.findall(r'data-([a-z-]+)="([^"]*)"', text[:text.index(">")]))
        if a.get("layer", "body") == "body":
            continue
        im = Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_string=text, width=128, height=128)))).getchannel("A")
        px, py = (float(v) for v in a["pivot"].split())
        layers.setdefault((a["rig-of"], int(a["order"])), []).append(
            (a["layer"], im, (px, py), a["motion"], float(a["amp"]), float(a["hz"]), float(a["phase"])))
    bad = []
    for (stem, order), group in sorted(layers.items()):
        for i in range(len(group)):
            for j in range(i + 1, len(group)):
                worst = (0, 0.0)
                for k in range(RIG_STEPS):
                    tt = RIG_LOOP * k / RIG_STEPS
                    masks = []
                    for name, im, piv, motion, amp, hz, ph in (group[i], group[j]):
                        a = 2 * math.pi * (hz * tt + ph)
                        deg = amp * (1 - math.cos(a)) / 2 if motion == "lift" else amp * math.sin(a)
                        masks.append(im.rotate(-deg, resample=Image.BILINEAR, center=piv).point(lambda v: 255 if v > 127 else 0))

                    n = ImageChops.multiply(masks[0], masks[1]).histogram()[255]   # both opaque
                    worst = max(worst, (n, tt))
                if worst[0] > RIG_TOUCH_PX:
                    bad.append((stem + ".svg", "rig", "%s+%s" % (group[i][0], group[j][0]),
                                "layers %s and %s share depth %d but overlap (%d px at 128 px, t %.2f s)"
                                % (group[i][0], group[j][0], order, worst[0], worst[1])))
    return bad


def faults(files, ok, where, root: pathlib.Path = SVG) -> list:
    bad = [("palette.py", "palette", p, p) for p in PAL.check()] + rig_faults(root)
    for p in files:
        text = p.read_text(encoding="utf-8")
        bad += colour_faults(p.name, text, ok) + ink_faults(p.name, text) + flat_faults(p.name, text)
        bad += crossing_faults(p.name, text)
        bad += eye_faults(p.name, text) + ground_faults(p, text, where)
    return bad + unique_faults(files, where)


def verdict(found, known) -> list:
    """The lines that fail: faults KNOWN does not list, and KNOWN entries the art no longer has."""
    keys = {f[:3] for f in found}
    new = ["%s: %s" % (f[0], f[3]) for f in found if f[:3] not in known]
    stale = ["%s: KNOWN or APPROVED lists %s %s, which the art no longer has (remove it: the table only shrinks)" % k
             for k in sorted(known - keys)]
    return new + stale


# --- the self-test ---------------------------------------------------------------------------------------------------

def shrink_eyes(text: str) -> str:
    def one(m):
        cx, cy, rx, ry, f, s = m.groups()
        if f.upper() in EYE_WHITES and s.upper() == PAL.INK.upper():
            rx, ry = "%.1f" % (float(rx) * 0.8), "%.1f" % (float(ry) * 0.8)
        width = re.search(r' stroke-width="[\d.]+"', m.group(0))
        return '<ellipse cx="%s" cy="%s" rx="%s" ry="%s" fill="%s" stroke="%s"%s/>' % (cx, cy, rx, ry, f, s,
                                                                                       width.group(0) if width else "")
    return ELL.sub(one, text)


def self_test(ok, where, known) -> int:
    """Each rule fails on one planted fault in a copy under build/look_lint; the copy, unplanted, passes."""
    work = ROOT / "build" / "look_lint" / "selftest"
    if work.exists():
        shutil.rmtree(work)
    for fam in FAMILIES + ("rigs",):
        shutil.copytree(SVG / fam, work / fam)
    files = field_files(work)
    if verdict(faults(files, ok, where, work), known):
        print("self-test: the unplanted copy already fails; run the lint first")
        return 1
    # The grammar's plants go on the grammar drawing that still ships (the generic boss: the enemies, bosses, cat and kitten
    # are Sticker since 2026-11-18), its eyes on the goose (the smallest grammar eyes left); the Sticker plants on the
    # Sticker mouse, its eyes on the bat (the approved cast's smallest), its shade on the Backyard's moth (one realm).
    mouse = work / "bosses" / "boss_generic.svg"
    body = body_colour(mouse, colours_of(mouse.read_text(encoding="utf-8")))
    ink = 'stroke="%s"/>' % PAL.INK
    sticker = work / "creatures" / "enemy_backyard_mouse.svg"
    fur = PAL.STICKER_CREATURES["mouseFur"].upper()
    moth = work / "creatures" / "enemy_backyard_moth.svg"
    moth_body = body_colour(moth, colours_of(moth.read_text(encoding="utf-8")))
    plants = [
        ("an off-palette fill", mouse, lambda t: t.replace('fill="%s"' % body, 'fill="#8596C4"', 1),
         "not a palette token"),
        ("a tint of a literal", mouse, lambda t: t.replace('fill="%s"' % body, 'fill="%s"' % S.tint("#123456", 0.3), 1),
         "not a palette token"),
        ("an outline not in INK", mouse, lambda t: t.replace(ink, 'stroke="#000000"/>', 1), "not INK"),
        ("a thinner outline", mouse, lambda t: t.replace('stroke-width="3.1"', 'stroke-width="2.4"', 1),
         "outline width is"),
        ("a shape with its own width", mouse, lambda t: t.replace("<ellipse ", '<ellipse stroke-width="5" ', 1),
         "its own outline width"),
        ("a body at half opacity", mouse, lambda t: t.replace("<ellipse ", '<ellipse opacity="0.5" ', 1), "not flat"),
        ("a gradient", mouse, lambda t: t.replace("<g ", '<defs><linearGradient id="g"/></defs><g ', 1), "not flat"),
        ("an outline crossing itself", mouse,
         lambda t: t.replace("</g>", '<polygon points="10,10 20,20 20,10 10,20" fill="%s" stroke="%s"/></g>'
                             % (body, PAL.INK), 1), "crosses itself"),
        ("eyes shrunk", work / "creatures" / "enemy_goose.svg", shrink_eyes, "its eyes cover"),
        ("a body lost on its floor", mouse,
         lambda t: t.replace(body, PAL.BACKDROPS["tier_1_backyard"]["detail"].upper()), "the tier_1_backyard floor"),
        ("a Sticker outline too wide", sticker,
         lambda t: re.sub(r'(<ellipse [^>]*stroke-width=")[\d.]+("/>)', r'\g<1>9.00\2', t, count=1),
         "wider than the drawing's line"),
        ("a Sticker outline not in INK", sticker,
         lambda t: re.sub(r'(<ellipse [^>]*)stroke="%s" stroke-width' % PAL.INK, r'\g<1>stroke="#000000" stroke-width', t,
                          count=1), "not INK"),
        ("an off-palette Sticker fill", sticker, lambda t: t.replace('fill="%s"' % fur, 'fill="#8596C4"', 1),
         "not a palette token"),
        ("Sticker eyes shrunk", work / "creatures" / "enemy_bat.svg", shrink_eyes, "its eyes cover"),
        ("two enemies of a realm in one shade", sticker, lambda t: t.replace(fur, moth_body), "the same shade"),
        ("touching legs at one depth", work / "rigs" / "enemy_dog__leg_far_front.svg",
         lambda t: t.replace('data-order="-2"', 'data-order="-1"', 1), "share depth -1 but overlap"),
    ]
    missed = 0
    for what, path, plant, want in plants:
        text = path.read_text(encoding="utf-8")
        planted = plant(text)
        if planted == text:
            print("self-test: %-30s DID NOT LAND (the plant changed nothing)" % what)
            missed += 1
            continue
        path.write_text(planted, encoding="utf-8", newline="\n")
        got = [b for b in verdict(faults(files, ok, where, work), known) if want in b]
        path.write_text(text, encoding="utf-8", newline="\n")
        print("self-test: %-30s %s" % (what, ("CAUGHT  " + got[0]) if got else "MISSED"))
        missed += 0 if got else 1
    # The ratchet, on a known colour of its own (the real KNOWN colours were all named in the palette, 2026-11-09): an entry
    # the art no longer has is reported; a colour known in one file is still a fault in another.
    fake = ("kitten_basic.svg", "colour", "#123456")
    got = [b for b in verdict(faults(files, ok, where, work), known | {fake}) if "which the art no longer has" in b]
    print("self-test: %-30s %s" % ("a known colour redrawn", ("CAUGHT  " + got[0]) if got else "MISSED"))
    missed += 0 if got else 1
    rat = work / "creatures" / "enemy_factory_rat.svg"
    text = rat.read_text(encoding="utf-8")
    rat.write_text(text.replace("<g ", '<g fill="#123456" ', 1), encoding="utf-8", newline="\n")
    try:
        got = [b for b in verdict(faults(files, ok, where, work), known | {fake}) if "#123456 is not a palette token" in b]
    finally:
        rat.write_text(text, encoding="utf-8", newline="\n")
    print("self-test: %-30s %s" % ("a known colour in a new file", ("CAUGHT  " + got[0]) if got else "MISSED"))
    missed += 0 if got else 1
    # Rule 3 checks the generic boss's tokens (the grammar's last creature on the field since D18).
    keep_boss = PAL.BOSSES["generic"]
    PAL.BOSSES["generic"] = (PAL.BACKDROPS["tier_1_backyard"]["field"], keep_boss[1])
    try:
        got = [b for b in verdict(faults(files, ok, where, work), known) if "boss_generic body" in b]
    finally:
        PAL.BOSSES["generic"] = keep_boss
    print("self-test: %-30s %s" % ("a token on its own floor", ("CAUGHT  " + got[0]) if got else "MISSED"))
    missed += 0 if got else 1
    keep = PAL.ENEMIES["mouse"]
    PAL.ENEMIES["mouse"] = (PAL.ENEMIES["bat"][0], keep[1])
    try:
        got = [b for b in verdict(faults(files, ok, where, work), known) if "SAME SHADE" in b]
    finally:
        PAL.ENEMIES["mouse"] = keep
    print("self-test: %-30s %s" % ("a palette body used twice", ("CAUGHT  " + got[0]) if got else "MISSED"))
    missed += 0 if got else 1
    if verdict(faults(files, ok, where, work), known):
        print("self-test: the copy did not come back clean")
        missed += 1
    shutil.rmtree(work)
    print("look_lint --self-test: %s" % ("every plant caught" if not missed else "%d NOT CAUGHT" % missed))
    return 1 if missed else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--known", action="store_true")
    args = ap.parse_args()
    ok = allowed(derivations())
    where = grounds()
    known = parse_known(KNOWN) | parse_known(APPROVED)
    if args.self_test:
        return self_test(ok, where, known)
    files = field_files()
    found = faults(files, ok, where)
    if args.known:
        print(format_known({f[:3] for f in found}))
        return 0
    bad = verdict(found, known)
    for b in bad:
        print("  " + b)
    eyes = min((eye_px56(p.read_text(encoding="utf-8")), p.name) for p in files if is_creature(p.name))
    print("look_lint: %d drawings, %d allowed colours; %d known or approved faults in %d files; smallest eyes "
          "%.1f px at %d px (%s); %s" % (len(files), len(ok), len(known), len({k[0] for k in known}), eyes[0], EYE_PX,
                                         eyes[1], "PASS" if not bad else "%d FAULT(S)" % len(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
