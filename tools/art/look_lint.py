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
EYE_PX = 56                # the smallest sprite_target_px in Data/EnemyData.csv
HEX = re.compile(r"#[0-9A-Fa-f]{6}\b")
CREAM_TARGET = "#FBF7EC"   # sweet_shapes.tint()'s default direction: a target, never a fill

# "<file> <rule> <subject> [<subject> ...]": the faults the art had on 2026-11-09 (R5 removes them; `--known` reprints).
# colour: the literals the generators drew before the restyle (the mounts' coats and manes, the armour's steels, five
# props). ground: the pale coats (the armoured cat's largest fill is its pale belly; the unicorns') on the light floors;
# the void unicorn on the dark. Removed 2026-11-09 by the one-colour palette: the Park Tyrant on its Park, the cream
# kitten on four floors, the flying mouse's literal wings.
KNOWN = """
player_cat_stage_2.svg ground tier_10_vets tier_2_alley tier_6_turf tier_9_kitchen
player_cat_stage_3.svg ground tier_10_vets tier_2_alley tier_6_turf tier_9_kitchen
player_cat_stage_4.svg ground tier_10_vets tier_2_alley tier_6_turf tier_9_kitchen
player_cat_stage_5.svg ground tier_10_vets tier_2_alley tier_6_turf tier_9_kitchen
player_cat_stage_6.svg ground tier_12_litterbox tier_7_portal tier_8_mythic
player_cat_stage_7.svg ground tier_10_vets tier_2_alley tier_6_turf tier_9_kitchen
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
    try:
        import make_sweet_art as M
        import sweet_biomes as B
        for batch in M.BATCHES.values():
            for _rel, _px, fn in batch():
                fn()
        B.PROPS()
    finally:
        S.tint = real
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


def ink_faults(name: str, text: str) -> list:
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


FLAT_BANNED = ("opacity", "Gradient", "<pattern", "filter", "<mask", "mask=", "clip-path", "<clipPath", "<image", "blur",
               "rgba(", "hsla(")


def flat_faults(name: str, text: str) -> list:
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


ELL = re.compile(r'<ellipse cx="([\d.]+)" cy="([\d.]+)" rx="([\d.]+)" ry="([\d.]+)" fill="(#\w+)" stroke="([#\w]+)"/>')


def eye_px56(text: str) -> float:
    """The eye whites' area at 56 px, from the geometry: an eye is sweet_shapes.eyes()'s EYE ellipse outlined in INK
    with its INK pupil drawn next, inside it (not a raster count: a ghost's white body is the colour of an eye)."""
    box = float(re.search(r'viewBox="0 0 ([\d.]+)', text).group(1))
    es = [(float(a), float(b), float(c), float(d), f.upper(), s.upper()) for a, b, c, d, f, s in ELL.findall(text)]
    area = 0.0
    for e, nxt in zip(es, es[1:] + [None]):
        if e[4] == PAL.EYE.upper() and e[5] == PAL.INK.upper() and nxt and nxt[4] == PAL.INK.upper() \
                and nxt[5] == "NONE" and math.hypot(nxt[0] - e[0], nxt[1] - e[1]) < max(e[2], e[3]):
            area += math.pi * e[2] * e[3]
    return area * (EYE_PX / box) ** 2


def eye_faults(name: str, text: str) -> list:
    if not is_creature(name):
        return []
    a = eye_px56(text)
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
        if a == 255 and c in drawn and c not in (PAL.INK.upper(), PAL.EYE.upper()):
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
        return "gristle"
    if stem in RATS:
        return "rat"
    if stem.startswith("player_cat_"):
        return "the cat"
    return PAIRS.get(stem, stem)


def unique_faults(files, where) -> list:
    """No shade used twice (designer, 2026-11-09): the drawn bodies of two different characters that ship are at least
    palette.UNIQUE_MIN apart. A kind's biome copies share theirs."""
    bodies = []
    for p in files:
        if is_creature(p.name) and p.stem in where:
            b = body_colour(p, colours_of(p.read_text(encoding="utf-8")))
            if b:
                bodies.append((p, species_of(p.stem), b))
    bad = []
    for i in range(len(bodies)):
        for j in range(i + 1, len(bodies)):
            (pa, sa, ba), (pb, sb, bb) = bodies[i], bodies[j]
            d = PAL.distance(ba, bb)
            if sa != sb and d < PAL.UNIQUE_MIN:
                bad.append((pa.name, "unique", pb.name, "its body %s is %.1f from %s's %s (min %.0f): the same shade"
                            % (ba, d, pb.name, bb, PAL.UNIQUE_MIN)))
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
    stale = ["%s: KNOWN lists %s %s, which the art no longer has (remove it: the table only shrinks)" % k
             for k in sorted(known - keys)]
    return new + stale


# --- the self-test ---------------------------------------------------------------------------------------------------

def shrink_eyes(text: str) -> str:
    def one(m):
        cx, cy, rx, ry, f, s = m.groups()
        if f.upper() == PAL.EYE.upper() and s.upper() == PAL.INK.upper():
            rx, ry = "%.1f" % (float(rx) * 0.8), "%.1f" % (float(ry) * 0.8)
        return '<ellipse cx="%s" cy="%s" rx="%s" ry="%s" fill="%s" stroke="%s"/>' % (cx, cy, rx, ry, f, s)
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
    mouse = work / "creatures" / "enemy_backyard_mouse.svg"
    body = PAL.ENEMIES["mouse"][0].upper()
    ink = 'stroke="%s"/>' % PAL.INK
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
        ("eyes shrunk", work / "creatures" / "enemy_rooftop_seagull.svg", shrink_eyes, "its eyes cover"),
        ("a body lost on its floor", mouse,
         lambda t: t.replace(body, PAL.BACKDROPS["tier_1_backyard"]["detail"].upper()), "the tier_1_backyard floor"),
        ("two enemies in one shade", mouse, lambda t: t.replace(body, PAL.ENEMIES["rat"][0].upper()), "the same shade"),
        ("touching legs at one depth", work / "rigs" / "enemy_alley_dog__leg_far_front.svg",
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
    fake = ("enemy_backyard_mouse.svg", "colour", "#123456")
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
    keep = PAL.ENEMIES["mouse"]
    PAL.ENEMIES["mouse"] = (PAL.BACKDROPS["tier_1_backyard"]["field"], keep[1])
    try:
        got = [b for b in verdict(faults(files, ok, where, work), known) if b.startswith("palette.py")]
    finally:
        PAL.ENEMIES["mouse"] = keep
    print("self-test: %-30s %s" % ("a token on its own floor", ("CAUGHT  " + got[0]) if got else "MISSED"))
    missed += 0 if got else 1
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
    known = parse_known(KNOWN)
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
    print("look_lint: %d drawings, %d allowed colours; %d known faults in %d files (R5 removes them); smallest eyes "
          "%.1f px at %d px (%s); %s" % (len(files), len(ok), len(known), len({k[0] for k in known}), eyes[0], EYE_PX,
                                         eyes[1], "PASS" if not bad else "%d FAULT(S)" % len(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
