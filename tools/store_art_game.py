"""The Steam store, library and event images drawn from the GAME's own art, posed after the key art (designer,
2026-11-25: "recreate the store images using our in game enemies and player. Pose the main character if you can using
in game assets or redo them. Try to get as close as you can to the concept images").

    py tools/store_art_game.py [--logo recreated|original] [--out DIR] [--only NAME ...] [--psd]

THE CONCEPT (tools/store_art.py's key art, Gemini 2026-11-03): the logo across the top, the cat on the unicorn leaping in
the middle on a rainbow with a glow round it, a hooded crowd on the left, an army of armed kittens on the right, a purple
sky over a sage-green field. HERE every creature is the game's own drawing (SourceArt/svg, the Sticker Box cast, each
with its white die-cut rim as the game draws it):
  - the hero: the stage-4 rider (the cat on the unicorn), POSED: its legs turned about their hips into a leap, the
    mount tilted nose-up and flipped to face right, the game's lance (the stage-4 weapon) over the cat's shoulder;
  - the left crowd: the game's enemies, flipped to face the hero, back rows smaller and higher;
  - the right crowd: kittens (the game's kitten, some golden as Golden Kittens are) with the cat's swords and lances and
    the jobs' hats;
  - the sky and field, the rainbow arc and trail, the glow and sparkles: painted here in the game's rainbow colours.
The logo: the recreation in Boogaloo (Drive Logo/recreated; no AI art anywhere) or, with --logo original, the key
art's (Drive Logo/gemini). Out (default build/steam/store_art_game): upload/<name>.png at Steam's 13 sizes, checks/
(the small capsule at 120x45 and 184x69, a sheet); with --psd, psd/<name>.psd, each step its own layer (tools/
store_psd.py; read back and compared with the PNG). Drive keeps them in Promotional/Steam/store-images/game-art.
"""

import argparse
import csv
import io
import math
import sys
import xml.etree.ElementTree as ET
from functools import lru_cache
from pathlib import Path

import numpy as np
import resvg_py
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage

sys.path.insert(0, str(Path(__file__).resolve().parent))
from store_psd import Layers, psd_matches, save_psd  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
SVG = ROOT / "SourceArt" / "svg"
OUT = ROOT / "build" / "steam" / "store_art_game"
DRIVE = Path(r"G:\My Drive\Games\Pawpocalypse\Promotional")
LOGOS = {"recreated": DRIVE / "Logo" / "recreated" / "pawpocalypse_logo_recreated.png",
         "original": DRIVE / "Logo" / "gemini" / "logo_3840.png"}
NS = "http://www.w3.org/2000/svg"
ET.register_namespace("", NS)
RIM = (0xFF, 0xFD, 0xF6)   # sticker_draw.RIM_COLOUR
INK = (0x2A, 0x21, 0x31)
RAINBOW = ["#FF6B6B", "#FFAE42", "#FFE15C", "#7DDB6A", "#58B6FF", "#A87BFF"]   # the mane's four + green and violet
SIZES = {
    "main_capsule": (1232, 706), "header_capsule": (920, 430), "library_header": (920, 430),
    "small_capsule": (462, 174), "vertical_capsule": (748, 896), "library_capsule": (600, 900),
    "library_hero": (3840, 1240), "library_logo": (1280, 192), "page_background": (1438, 810),
    "event_cover": (800, 450), "event_header": (1920, 622), "app_icon": (184, 184), "shortcut_icon": (256, 256),
}
# The left crowd, front to back by how much each reads at a glance (silhouettes the player meets often first).
ENEMIES = ["enemy_park_goose", "enemy_rat", "enemy_park_gristle", "enemy_portal_ghost", "enemy_squirrel", "enemy_mythic_crow",
           "enemy_alley_dog", "enemy_factory_robot_mouse", "enemy_litterbox_dust_bunny", "enemy_mythic_imp",
           "enemy_kitchen_cockroach", "enemy_litterbox_sludge_cat", "enemy_mouse", "enemy_turf_toad", "enemy_vet_hamster",
           "enemy_portal_hound", "enemy_sewer_crocodile", "enemy_kitchen_ant", "enemy_goose", "enemy_vet_parrot",
           "enemy_factory_gristle", "enemy_dog", "enemy_backyard_mouse", "enemy_sewer_rat"]
FLIERS = ["enemy_bat", "enemy_mouse_flying", "enemy_backyard_moth", "enemy_rooftop_seagull", "enemy_moth"]
ENEMY_RIM = False   # the designer, 2026-11-25: "remove the white background from the enemies"
KITTEN_RIM = False  # and from the kitten army ("the enemies on the right should not have a white background")
WEAPONS = ["player_weapon_stage_2", "player_weapon_stage_4", "player_weapon_stage_2", "player_weapon_stage_5"]
HATS = ["hat_daredevil", "hat_engineer", "hat_magician", "hat_ratcatcher", "hat_town_crier", "hat_blacksmith", None, None]


def hexrgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (1, 3, 5))


# --- drawing the game's art ---------------------------------------------------------------------------------------

def rim_px(svg_text, width):
    import re
    m = re.search(r'data-rim="([\d.]+)"', svg_text[:600])
    v = re.search(r'viewBox="([-\d.]+) ([-\d.]+) ([\d.]+) ', svg_text[:600])
    return float(m.group(1)) * width / float(v.group(3)) if m and v else 0.0


def die_cut(img, radius):
    """The drawing over its white die-cut rim (pack_atlas.die_cut, by a distance transform: fast at any size)."""
    if radius <= 0:
        return img
    pad = int(radius) + 3
    a = np.pad(np.asarray(img)[:, :, 3].astype(np.float32) / 255.0, pad)
    d = ndimage.distance_transform_edt(a < 0.5)
    rim = np.clip(radius + 0.5 - d, 0.0, 1.0)
    big = Image.new("RGBA", (img.width + 2 * pad, img.height + 2 * pad), RIM + (0,))
    big.putalpha(Image.fromarray((rim * 255 + 0.5).astype(np.uint8)))
    big.alpha_composite(img, (pad, pad))
    return big


@lru_cache(maxsize=512)
def sprite(stem, width, flip=False, rim=True):
    """A field drawing at `width` px, with its white die-cut rim unless rim=False, optionally mirrored. Cached."""
    folder = next(f for p, f in (("enemy_", "creatures"), ("boss_", "bosses"), ("player_", "player"), ("kitten_", "player"),
                                 ("hat_", "jobs"), ("prop_", "props")) if stem.startswith(p))
    text = (SVG / folder / (stem + ".svg")).read_text(encoding="utf-8")
    img = Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_string=text, width=width, height=width)))).convert("RGBA")
    if rim:
        img = die_cut(img, rim_px(text, width))
    return img.transpose(Image.FLIP_LEFT_RIGHT) if flip else img


def posed_rider_svg(front=(50.0, 75.0), back=(-32.0, -20.0), tilt=12.0, tail=-14.0, lance=True):
    """The stage-4 rider leaping: legs turned about their hips, the mount tilted nose-up, flipped to face right, the
    stage-4 lance over the cat's shoulder (PawFieldLook.h WeaponOffsetMounted*). The parts are found by their place in
    the drawing and asserted, so a redrawn rider stops this tool instead of being bent wrong."""
    root = ET.parse(SVG / "player" / "player_cat_stage_4.svg").getroot()
    g = root.find("{%s}g" % NS)
    kids = list(g)
    tags = [k.tag.split("}")[1] for k in kids]
    rects = [i for i, t in enumerate(tags) if t == "rect"]
    if len(rects) != 6 or tags[:6] != ["polyline"] * 6:
        raise SystemExit("store_art_game: player_cat_stage_4.svg is drawn differently now; re-fit posed_rider_svg")
    far_front, far_back, near_front, near_back, hoof_front, hoof_back = rects

    def hip(el):
        return float(el.get("x")) + float(el.get("width")) / 2, float(el.get("y")) + 2.0

    def wrap(ids, angle, pivot):
        grp = ET.Element("{%s}g" % NS, {"transform": "rotate(%.2f %.2f %.2f)" % (angle, pivot[0], pivot[1])})
        for i in ids:
            grp.append(kids[i])
        return grp
    groups = {far_front: wrap([far_front], front[1], hip(kids[far_front])),
              far_back: wrap([far_back], back[1], hip(kids[far_back])),
              near_front: wrap([near_front, hoof_front], front[0], hip(kids[near_front])),
              near_back: wrap([near_back, hoof_back], back[0], hip(kids[near_back])),
              0: wrap(list(range(6)), tail, (92.0, 80.0))}
    skip = set(range(1, 6)) | {hoof_front, hoof_back}
    for k in kids:
        g.remove(k)
    whole = ET.Element("{%s}g" % NS, {"transform": "translate(128 0) scale(-1 1) rotate(%.2f 64 80)" % tilt})
    for i, k in enumerate(kids):
        if i in groups:
            whole.append(groups[i])
        elif i not in skip:
            whole.append(k)
    if lance:
        w = ET.parse(SVG / "player" / "player_weapon_stage_4.svg").getroot()
        size = 0.559 * 128 * 1.25
        cx, cy = 64 + 0.223 * 128, 64 - 0.144 * 128
        wg = ET.Element("{%s}g" % NS, {"transform": "translate(%.2f %.2f) scale(%.4f)" % (cx - size / 2, cy - size / 2, size / 128)})
        for k in list(w):
            wg.append(k)
        whole.insert(0, wg)   # behind the cat and the mount, as the game draws the lance over the shoulder
    g.append(whole)
    root.set("viewBox", "-20 -16 168 160")
    root.set("width", "336")
    root.set("height", "320")
    root.set("data-rim", "3.224")
    return ET.tostring(root, encoding="unicode")


@lru_cache(maxsize=16)
def rider(width):
    text = posed_rider_svg()
    h = round(width * 160 / 168)
    img = Image.open(io.BytesIO(bytes(resvg_py.svg_to_bytes(svg_string=text, width=width, height=h)))).convert("RGBA")
    img = die_cut(img, 3.224 * width / 168)
    return img.crop(img.getchannel("A").getbbox())


@lru_cache(maxsize=64)
def kitten(width, weapon, hat, golden):
    """A kitten of the cat's army: the game's kitten, a weapon held up in front, maybe a job's hat, maybe golden."""
    k = sprite("kitten_basic", width, rim=KITTEN_RIM)
    canvas = Image.new("RGBA", (k.width + width, k.height + width // 2), (0, 0, 0, 0))
    ox, oy = width // 2, width // 2
    if weapon:
        w = sprite(weapon, round(width * 0.62), rim=KITTEN_RIM).rotate(28, resample=Image.BICUBIC, expand=True)
        canvas.alpha_composite(w, (ox + round(width * 0.02) - w.width // 2 + round(width * 0.12), oy + round(width * 0.38) - w.height // 2))
    canvas.alpha_composite(k, (ox, oy))
    if hat:
        h = sprite(hat, round(width * 0.62), rim=KITTEN_RIM)
        canvas.alpha_composite(h, (ox + round(width * 0.36) - h.width // 2, oy + round(width * 0.17) - h.height // 2))
    if golden:   # Golden Kitten's tint (PawFieldLook.h GoldenKittenTint), on the kitten only, not the rim
        arr = np.asarray(canvas).astype(np.float32)
        warm = np.array([1.12, 0.98, 0.55])
        notrim = (np.abs(arr[..., :3] - np.array(RIM)).sum(-1) > 30)[..., None]
        arr[..., :3] = np.where(notrim, np.clip(arr[..., :3] * warm, 0, 255), arr[..., :3])
        canvas = Image.fromarray(arr.astype(np.uint8), "RGBA")
    return canvas.crop(canvas.getchannel("A").getbbox())


# --- painting the scene -------------------------------------------------------------------------------------------

def sky_reach(W, H, horizon):
    """0 in the purple sky, 1 on the open field: the purple reaches far down and fades into the field over 45% of the
    height (the designer, 2026-11-25: "Make the purple reach further down in a feathered way")."""
    y = np.linspace(0, 1, H)[:, None]
    x = np.linspace(0, 1, W)[None, :]
    wave = horizon + 0.03 * np.sin(x * 7.0 + 1.3) + 0.015 * np.sin(x * 17.0)
    ff = np.clip((y - wave + 0.04) / 0.45, 0, 1)
    return ff * ff * (3 - 2 * ff)


def mist(canvas, horizon, strength=0.62, seed=11):
    """The purple mist laid OVER the crowds (designer: "The purple should be over the enemies so they are darkened when
    they are in the purple mist. So the player pops out more"): the sky's own colour, as thick as the sky is purple,
    a little cloudy. The hero, its trail and the logo are drawn after it, so they stay clear."""
    W, H = canvas.size
    ff = sky_reach(W, H, horizon)
    y = np.linspace(0, 1, H)[:, None]
    t = np.clip(y / max(horizon, 1e-3), 0, 1)
    col = np.array(hexrgb("#24163A")) * (1 - t[..., None]) + np.array(hexrgb("#4E3670")) * t[..., None]
    rng = np.random.default_rng(seed)
    cloud = ndimage.gaussian_filter(rng.random((max(8, H // 8), max(8, W // 8))), 3)
    cloud = (cloud - cloud.min()) / (np.ptp(cloud) + 1e-6)
    cloud = np.asarray(Image.fromarray((cloud * 255).astype(np.uint8)).resize((W, H), Image.BICUBIC), np.float32) / 255
    a = np.clip((1 - ff) * strength * (0.8 + 0.4 * cloud), 0, 1)
    layer = np.dstack([np.broadcast_to(col, (H, W, 3)), a * 255]).astype(np.uint8)
    canvas.alpha_composite(Image.fromarray(layer, "RGBA"))


def background(W, H, horizon, glow_xy, seed=3):
    """The key art's ground: a dusky purple sky with darker swirls over a sage-green field, lighter where the hero is."""
    rng = np.random.default_rng(seed)
    y = np.linspace(0, 1, H)[:, None]
    x = np.linspace(0, 1, W)[None, :]
    sky_top, sky_low = np.array(hexrgb("#24163A")), np.array(hexrgb("#4E3670"))   # darker (designer, 2026-11-25)
    field_far, field_near = np.array(hexrgb("#6E8F7C")), np.array(hexrgb("#5D8466"))
    t = np.clip(y / max(horizon, 1e-3), 0, 1)
    sky = sky_top * (1 - t[..., None]) + sky_low * t[..., None]
    # the horizon: a soft wavy edge
    ff = sky_reach(W, H, horizon)
    u = np.clip((y - horizon) / max(1 - horizon, 1e-3), 0, 1)
    field = field_far * (1 - u[..., None]) + field_near * u[..., None]
    img = sky * (1 - ff[..., None]) + field * ff[..., None]
    # swirls in the sky: darker and lighter bands that wander
    for k, (amp, freq, ph, col, a) in enumerate(((0.05, 5.0, 0.4, "#170C28", 0.4), (0.04, 7.5, 2.0, "#5E4382", 0.25),
                                                 (0.03, 11.0, 4.1, "#2E1D48", 0.35))):
        band_y = horizon * (0.25 + 0.22 * k) + amp * np.sin(x * freq + ph) + 0.02 * np.sin(x * 23 + k)
        d = np.abs(y - band_y)
        m = np.clip(1 - d / (0.035 + 0.01 * k), 0, 1) * (1 - ff) * a
        img = img * (1 - m[..., None]) + np.array(hexrgb(col)) * m[..., None]
    # the light where the hero stands
    gx, gy = glow_xy
    r = np.sqrt(((x - gx) * W / H) ** 2 + (y - gy) ** 2)
    light = np.clip(1 - r / 0.75, 0, 1) ** 2 * 0.5
    img = img * (1 - light[..., None]) + np.array(hexrgb("#BFE3B4")) * light[..., None]
    # a faint noise so large gradients do not band
    img = img + (rng.random((H, W, 1)) - 0.5) * 6
    return Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), "RGB").convert("RGBA")


def scatter_props(canvas, horizon, seed, density=1.0):
    """Grass tufts, flowers and leaves on the field, small and soft so they sit behind the crowds."""
    W, H = canvas.size
    rng = np.random.default_rng(seed)
    n = int(26 * density * W / H)
    for _ in range(n):
        yy = rng.uniform(horizon + 0.04, 0.98)
        xx = rng.uniform(0.0, 1.0)
        size = int(H * (0.035 + 0.05 * (yy - horizon) / (1 - horizon)))
        stem = rng.choice(["prop_tier1_grass", "prop_tier1_grass", "prop_tier3_flowers", "prop_tier3_leaves"])
        s = sprite(stem, max(8, size)).copy()
        s.putalpha(Image.fromarray((np.asarray(s)[..., 3] * 0.55).astype(np.uint8)))
        canvas.alpha_composite(s, (int(xx * W - s.width / 2), int(yy * H - s.height / 2)))


def ring(W, H, cx, cy, r, width, colour, alpha, blur):
    yy, xx = np.mgrid[0:H, 0:W]
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    m = np.clip(1 - np.abs(d - r) / (width / 2), 0, 1) * (yy < cy + r * 0.05)
    layer = Image.new("RGBA", (W, H), hexrgb(colour) + (0,))
    layer.putalpha(Image.fromarray((m * alpha * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(blur)))
    return layer


def rainbow_arc(canvas, cx, cy, r, band, alpha=0.55):
    W, H = canvas.size
    for i, c in enumerate(reversed(RAINBOW)):
        canvas.alpha_composite(ring(W, H, cx, cy, r - i * band, band * 1.05, c, alpha, band * 0.35))


def rainbow_trail(canvas, start, end, width, alpha=0.8):
    """A ribbon of the rainbow's bands along a curve from `start` (under the hooves) to `end` (off the bottom)."""
    W, H = canvas.size
    layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    (x0, y0), (x1, y1) = start, end
    pts = []
    for i in range(41):
        t = i / 40
        cx = x0 + (x1 - x0) * t + math.sin(t * math.pi) * width * 1.6
        cy = y0 + (y1 - y0) * t
        pts.append((cx, cy, t))
    n = len(RAINBOW)
    for b, c in enumerate(RAINBOW):
        off = (b - (n - 1) / 2) * width / n
        line = [(px + off * (0.4 + 0.6 * t), py) for px, py, t in pts]
        d.line(line, fill=hexrgb(c) + (int(255 * alpha),), width=max(2, int(width / n * 1.15)), joint="curve")
    layer = layer.filter(ImageFilter.GaussianBlur(width * 0.04))
    # fade out toward the bottom
    a = np.asarray(layer).astype(np.float32)
    fade = np.clip(1.15 - (np.arange(H) - y0) / max(1, (y1 - y0)), 0.15, 1)[:, None]
    a[..., 3] *= fade
    canvas.alpha_composite(Image.fromarray(a.astype(np.uint8), "RGBA"))


def glow(canvas, cx, cy, r, colour="#FFFFFF", alpha=0.6):
    W, H = canvas.size
    yy, xx = np.mgrid[0:H, 0:W]
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / r
    m = np.clip(1 - d, 0, 1) ** 1.6 * alpha
    layer = Image.new("RGBA", (W, H), hexrgb(colour) + (0,))
    layer.putalpha(Image.fromarray((m * 255).astype(np.uint8)))
    canvas.alpha_composite(layer)


def sparkles(canvas, box, n, size, seed):
    rng = np.random.default_rng(seed)
    d = ImageDraw.Draw(canvas)
    x0, y0, x1, y1 = box
    for _ in range(n):
        x, y = rng.uniform(x0, x1), rng.uniform(y0, y1)
        s = size * rng.uniform(0.4, 1.0)
        col = hexrgb(rng.choice(["#FFFFFF", "#FFF3B0", "#FFE15C", "#BDE7FF"])) + (230,)
        d.polygon([(x, y - s), (x + s * 0.28, y - s * 0.28), (x + s, y), (x + s * 0.28, y + s * 0.28), (x, y + s),
                   (x - s * 0.28, y + s * 0.28), (x - s, y), (x - s * 0.28, y - s * 0.28)], fill=col)


def crowd(canvas, side, top, bottom, x_in, size, seed, count, enemies=True, sky=None):
    """A crowd massed against one edge: rows from `top` (small, far) to `bottom` (big, near), thinning toward x_in.
    side -1: the left edge (the game's enemies, flipped to face the hero); +1: the right (the kitten army)."""
    W, H = canvas.size
    rng = np.random.default_rng(seed)
    placed = []
    sizes = {}
    if enemies:
        for r in csv.DictReader(open(ROOT / "Data" / "EnemyData.csv", encoding="utf-8")):
            s = r["texture_path"].rsplit("/", 1)[-1][:-4]
            sizes[s] = max(sizes.get(s, 0), float(r["sprite_target_px"]))
    for i in range(count):
        v = rng.uniform(0, 1) ** 0.9
        y = top + (bottom - top) * v
        depth = 0.42 + 0.8 * v
        reach = x_in * (0.6 + 0.4 * v) * rng.uniform(0.0, 1.0) ** 0.75
        x = reach if side < 0 else W - reach
        if enemies:
            stem = ENEMIES[i % len(ENEMIES)]
            k = (sizes.get(stem, 70.0) / 70.0) ** 0.6
            placed.append((y, x, stem, int(size * depth * k), None))
        else:
            placed.append((y, x, "kitten", int(size * depth), i))
    if sky:
        sx0, sy0, sx1, sy1 = sky
        for j in range(max(2, count // 6)):
            stem = FLIERS[j % len(FLIERS)]
            placed.append((sy0 + (sy1 - sy0) * rng.uniform(), (sx0 + (sx1 - sx0) * rng.uniform()), stem,
                           int(size * rng.uniform(0.45, 0.7)), None))
    for y, x, stem, s, i in sorted(placed, key=lambda p: p[0]):
        if stem == "kitten":
            img = kitten(max(12, s), WEAPONS[i % len(WEAPONS)] if i % 5 != 4 else None, HATS[i % len(HATS)], i % 7 == 3)
        else:
            img = sprite(stem, max(12, s), flip=side < 0, rim=ENEMY_RIM)
        canvas.alpha_composite(img, (int(x - img.width / 2), int(y - img.height * 0.8)))


@lru_cache(maxsize=4)
def logo_image(which):
    return Image.open(LOGOS[which]).convert("RGBA")


def put_logo(canvas, which, cx, top, width):
    lg = logo_image(which)
    k = width / lg.width
    lg = lg.resize((round(lg.width * k), round(lg.height * k)), Image.LANCZOS)
    canvas.alpha_composite(lg, (round(cx - lg.width / 2), round(top)))


def scene(W, H, logo, hero_h, hero_cy, logo_w=0.62, logo_top=0.03, horizon=0.36, crowd_size=0.16, crowd_in=0.30,
          crowd_count=56, hero_cx=0.5, seed=5, arc=True, trail=True, fliers=True):
    """The key art's arrangement on a W x H canvas, every size in shares of H (logo_w of W), as named layers (the PSD's)."""
    cx, cy = W * hero_cx, H * hero_cy
    L = Layers(W, H)
    L.add("sky and field", background(W, H, horizon, (hero_cx, hero_cy)))
    scatter_props(L.new("grass and flowers"), horizon, seed)
    if arc:
        rainbow_arc(L.new("rainbow arc"), cx, cy + hero_h * H * 0.3, hero_h * H * 0.66, hero_h * H * 0.05, alpha=0.7)
    glow(L.new("glow (wide)"), cx, cy, hero_h * H * 0.75, "#FFFFFF", 0.55)
    # the fliers stay in the sky under the logo, never over it
    logo_bottom = H * logo_top + (W * logo_w * logo_image(logo).height / logo_image(logo).width if logo else 0)
    sky_top = max(H * horizon * 0.3, logo_bottom + H * 0.06)
    sky = (W * 0.02, sky_top, W * crowd_in * 1.1, max(sky_top + 1, H * horizon * 0.95)) if crowd_count and fliers else None
    crowd(L.new("enemies"), -1, H * (horizon + 0.02), H * 1.05, W * crowd_in, H * crowd_size, seed, crowd_count, sky=sky)
    crowd(L.new("kitten army"), +1, H * (horizon + 0.02), H * 1.05, W * crowd_in, H * crowd_size, seed + 1,
          int(crowd_count * 0.8), enemies=False)
    mist(L.new("purple mist"), horizon)
    r = rider(max(64, int(hero_h * H * 1.15)))
    if trail:
        rainbow_trail(L.new("rainbow trail"), (cx - r.width * 0.12, cy + r.height * 0.3), (cx - r.width * 0.35, H * 1.1),
                      r.width * 0.22)
    glow(L.new("glow (hero)"), cx, cy, hero_h * H * 0.42, "#FFF6DA", 0.5)
    if hero_h > 0.01:
        L.new("rider").alpha_composite(r, (int(cx - r.width / 2), int(cy - r.height / 2)))
        sparkles(L.new("sparkles"), (cx - r.width * 0.7, cy - r.height * 0.6, cx + r.width * 0.7, cy + r.height * 0.75), 26,
                 H * 0.018, seed)
    if logo:
        put_logo(L.new("logo"), logo, W / 2, H * logo_top, W * logo_w)
    return L


# --- the 13 images ------------------------------------------------------------------------------------------------

def build(logo, only=None):
    out = {}

    def want(n):
        return not only or n in only
    if want("main_capsule"):
        out["main_capsule"] = scene(1232, 706, logo, hero_h=0.74, hero_cy=0.62, logo_w=0.66)
    if want("event_cover"):
        out["event_cover"] = scene(800, 450, logo, hero_h=0.62, hero_cy=0.6, logo_w=0.66)
    for n in ("header_capsule", "library_header"):
        if want(n):
            out[n] = scene(920, 430, logo, hero_h=0.74, hero_cy=0.62, logo_w=0.62, crowd_in=0.3, crowd_count=64)
    if want("event_header"):
        out["event_header"] = scene(1920, 622, logo, hero_h=0.72, hero_cy=0.62, logo_w=0.5, crowd_in=0.34, crowd_count=96)
    if want("vertical_capsule"):
        out["vertical_capsule"] = scene(748, 896, logo, hero_h=0.5, hero_cy=0.48, logo_w=0.9, logo_top=0.035,
                                        horizon=0.3, crowd_size=0.13, crowd_in=0.42, crowd_count=70)
    if want("library_capsule"):
        out["library_capsule"] = scene(600, 900, logo, hero_h=0.42, hero_cy=0.45, logo_w=0.92, logo_top=0.035,
                                       horizon=0.28, crowd_size=0.11, crowd_in=0.44, crowd_count=80)
    if want("library_hero"):
        out["library_hero"] = scene(3840, 1240, None, hero_h=0.75, hero_cy=0.52, horizon=0.3, crowd_size=0.17,
                                    crowd_in=0.33, crowd_count=110)
    if want("small_capsule"):
        s = scene(462, 174, None, hero_h=0.0001, hero_cy=0.5, horizon=0.35, crowd_size=0.3, crowd_in=0.2,
                  crowd_count=10, arc=False, trail=False, fliers=False)
        s.add("dim", Image.new("RGBA", s.size, (40, 20, 50, 60)))
        lg = logo_image(logo)
        letters = lg.crop((int(lg.width * 0.082), 0, int(lg.width * 0.918), lg.height)) if logo == "original" else \
            lg.crop((int(lg.width * 0.095), 0, int(lg.width * 0.905), lg.height))
        k = 440 / letters.width
        letters = letters.resize((440, round(letters.height * k)), Image.LANCZOS)
        s.new("logo lettering").alpha_composite(letters, (11, (174 - letters.height) // 2))
        out["small_capsule"] = s
    if want("page_background"):
        p = scene(1438, 810, None, hero_h=0.0001, hero_cy=0.5, arc=False, trail=False, crowd_count=30).flat()
        p = p.filter(ImageFilter.GaussianBlur(4))
        out["page_background"] = Layers.single("background (blurred, dimmed)",
                                               Image.blend(Image.new("RGBA", p.size, (20, 14, 30, 255)), p, 0.5))
    if want("library_logo"):
        lg = logo_image(logo)
        out["library_logo"] = lg.resize((1280, round(lg.height * 1280 / lg.width)), Image.LANCZOS)
        if out["library_logo"].height != 192:
            c = Image.new("RGBA", (1280, 192), (0, 0, 0, 0))
            t = out["library_logo"]
            t = t.resize((round(t.width * 192 / t.height), 192), Image.LANCZOS) if t.height > 192 else t
            c.alpha_composite(t, ((1280 - t.width) // 2, (192 - t.height) // 2))
            out["library_logo"] = c
        out["library_logo"] = Layers.single("logo", out["library_logo"])
    if want("app_icon") or want("shortcut_icon"):
        icon = Layers(512, 512)
        icon.add("sky and field", background(512, 512, 0.6, (0.5, 0.55)))
        rainbow_arc(icon.new("rainbow arc"), 256, 400, 250, 26, alpha=0.85)
        glow(icon.new("glow"), 256, 300, 260, "#FFF6DA", 0.55)
        cat = sprite("player_cat_stage_1", 1400)
        k = 1400 / 128.0
        head = cat.crop((int(0 * k), int(18 * k), int(96 * k), int(114 * k)))   # the head, ears and whiskers (head at 44, 62)
        head = head.resize((470, 470), Image.LANCZOS)
        icon.new("cat").alpha_composite(head, ((512 - 470) // 2 + 40, 512 - 470 + 10))
        if want("app_icon"):
            out["app_icon"] = icon.resized(184, 184)
        if want("shortcut_icon"):
            out["shortcut_icon"] = icon.resized(256, 256)
    return out


def checks(images, out):
    d = out / "checks"
    d.mkdir(parents=True, exist_ok=True)
    if "small_capsule" in images:
        small = images["small_capsule"].convert("RGB")
        for w, h in ((120, 45), (184, 69)):
            small.resize((w, h), Image.LANCZOS).resize((w * 4, h * 4), Image.NEAREST).save(d / ("small_capsule_%dx%d.png" % (w, h)))
    tiles = []
    for name, im in images.items():
        t = im.convert("RGBA")
        k = min(420 / t.height, 900 / t.width)
        t = t.resize((max(1, round(t.width * k)), max(1, round(t.height * k))), Image.LANCZOS)
        tile = Image.new("RGBA", (t.width, t.height + 28), (90, 90, 90, 255))
        tile.alpha_composite(t, (0, 28))
        ImageDraw.Draw(tile).text((6, 6), "%s %dx%d" % (name, *im.size), fill=(255, 255, 255, 255))
        tiles.append(tile)
    rows, row, width = [], [], 0
    for t in tiles:
        if row and width + t.width > 2400:
            rows.append(row)
            row, width = [], 0
        row.append(t)
        width += t.width + 12
    rows.append(row)
    sheet = Image.new("RGB", (2400, sum(max(t.height for t in r) + 12 for r in rows)), (30, 30, 30))
    y = 0
    for r in rows:
        x = 0
        for t in r:
            sheet.paste(t.convert("RGB"), (x, y))
            x += t.width + 12
        y += max(t.height for t in r) + 12
    sheet.save(d / "sheet.png")


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--logo", choices=sorted(LOGOS), default="recreated")
    ap.add_argument("--out", type=Path, default=OUT)
    ap.add_argument("--only", nargs="*")
    ap.add_argument("--psd", action="store_true", help="also a layered PSD of each image (out/psd; needs psd-tools)")
    args = ap.parse_args(argv)
    stacks = build(args.logo, args.only)
    up = args.out / "upload"
    up.mkdir(parents=True, exist_ok=True)
    images = {}
    for name, L in stacks.items():
        im = L.flat()
        if im.size != SIZES[name]:
            raise SystemExit("store_art_game: %s is %s, Steam wants %s" % (name, im.size, SIZES[name]))
        (im if name == "library_logo" else im.convert("RGB")).save(up / (name + ".png"))
        images[name] = im
        if args.psd:
            path = args.out / "psd" / (name + ".psd")
            save_psd(path, L)
            ok, diff = psd_matches(path, im)
            print("  %-17s %2d layers  PSD %s (mean difference %.2f)" % (name, len(L.layers), "ok" if ok else "DIFFERS", diff))
            if not ok:
                return 1
    checks(images, args.out)
    print("store_art_game: %d images -> %s%s" % (len(images), up, " (+ psd)" if args.psd else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
