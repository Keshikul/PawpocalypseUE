"""The screenshot tour, both games (docs/11 phase 3 exit: every screen against the Godot shots).

The SAME steps and state on each side (a new game in tier_1 with 1000 Catnip; the six tabs; the tree browsed; a card hand; a
Reawaken with three jobs held: the ceremony at its hold, the spend step, the job picker; the ending at 4200 s):
  Godot   tools/godot_fixtures/tour_probe.gd in the scratch copy (build/godot_fixtures/project), sandboxed APPDATA, rendered
          fullscreen with opengl3 (NOT --headless: the dummy rasteriser writes blank PNGs; fullscreen, because a 1920x1080
          window on a 1920x1080 desktop is shrunk and widens the canvas)            -> build/tour/godot/<shot>.png
  Unreal  the editor build with -game and the paw.* tour steps (Dev/PawShot.cpp)    -> build/tour/ue/<shot>.png
Then each pair side by side (Godot left)                                            -> build/tour/compare/<shot>.png

    py tools/godot_tour.py [--skip-godot] [--skip-ue] [--set screens|biomes|fonts] [--res 1280x800]

--set fonts (restyle R2, 2026-11-07; Unreal only): each language's font sheet, the culture made current in the game and
sample lines from Godot's locale/<lang>.po drawn in F_Paw (paw.FontSheet): the language picker's names, then the menu words
and the longest lines -> build/tour/ue/font_<culture>.png and one contact sheet, build/look/fonts_in_game.png. The glyph
gate is tools/check_font_coverage.py; this shows Slate drawing them (the per-language CJK face, Thai, no boxes).
--res: the Unreal window's size (default 1920x1080).

--set biomes (phase 6 slice 2): each of the 12 biomes after travelling there, the cat parked at (1250, 540): the backdrop
compared (the mice differ: each game rolls its own). Godot: tools/godot_fixtures/biome_probe.gd; Unreal: paw.Travel.

Both runs open a window on the desktop for a minute or two. The Godot repo is never written; the editor must be closed (UBT
first after a C++ change).
"""
import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import make_godot_fixtures as mgf  # noqa: E402

OUT = ROOT / "build" / "tour"
BIOME_SHOTS = ["biome_%d" % n for n in range(1, 13)] + ["title"]
SHOTS = ["tab_upgrades", "tab_reawaken", "tab_realms", "tab_catches", "tab_awards", "tab_jobs", "tree_browse", "card_pick",
         "ceremony", "tree_spend", "unicorn", "job_picker", "victory"]
EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
UPROJECT = ROOT / "Pawpocalypse.uproject"
# The Unreal tour: each step waits 30 frames after the one before (paw.Wait changes that once); frames are capped at 60
# so a wait is real time (uncapped, the editor build ran ~500 fps and the ceremony shot landed 0.06 s in).
UE_STEPS = (["t.MaxFPS 60", "paw.Cheat.Catnip 1000"]
            + sum([["paw.Tab %d" % (i + 1), "paw.Wait 60", "paw.Shot tour_%s" % t] for i, t in enumerate(SHOTS[:6])], [])
            + ["paw.Tab 1", "paw.Tree", "paw.Wait 90", "paw.Shot tour_tree_browse", "paw.Leave",
               "paw.Hand", "paw.Wait 60", "paw.Shot tour_card_pick", "paw.Take", "paw.Wait 30",
               "paw.Reawaken jobs", "paw.Wait 30", "paw.Shot tour_ceremony", "paw.Wait 120", "paw.Shot tour_tree_spend",
               "paw.Cheat.Stardust 100000", "paw.Buy unlock_unicorn", "paw.Wait 96", "paw.Shot tour_unicorn", "paw.Wait 180",
               "paw.Leave", "paw.Wait 60", "paw.Shot tour_job_picker",
               "paw.Victory 4200", "paw.Wait 60", "paw.Shot tour_victory quit"])


def godot_side(exe, probe="tour_probe.gd"):
    project = mgf.WORK / "project"
    mgf.copy_project(Path("C:/Games/pawpocalypse"), project, True)
    shutil.copy2(mgf.SCRIPTS / probe, project / "tour_probe.gd")
    (project / "tour_probe.tscn").write_text('[gd_scene load_steps=2 format=3]\n\n[ext_resource type="Script" '
                                             'path="res://tour_probe.gd" id="1"]\n\n[node name="TourProbe" type="Node"]\n'
                                             'script = ExtResource("1")\n', encoding="utf-8")
    sandbox = OUT / "appdata"
    if sandbox.exists():
        shutil.rmtree(sandbox)
    for sub in ("Roaming", "Local"):
        (sandbox / sub).mkdir(parents=True)
    env = dict(os.environ, APPDATA=str(sandbox / "Roaming"), LOCALAPPDATA=str(sandbox / "Local"))
    r = subprocess.run([exe, "--path", str(project), "--rendering-driver", "opengl3", "--fullscreen", "res://tour_probe.tscn"],
                       capture_output=True, text=True, env=env, timeout=600)
    (OUT / "godot.log").write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
    if "TOUR_PROBE done" not in r.stdout or "SCRIPT ERROR" in r.stderr:
        raise SystemExit("the Godot tour failed; log: %s" % (OUT / "godot.log"))
    found = list(sandbox.rglob("port_tour"))
    if not found:
        raise SystemExit("no Godot shots under %s" % sandbox)
    dest = OUT / "godot"
    dest.mkdir(parents=True, exist_ok=True)
    for png in found[0].glob("*.png"):
        shutil.copy2(png, dest / png.name)
    shutil.rmtree(sandbox, ignore_errors=True)


# The biome set: a moment for the shaders (a first run compiles them: the floor shows black until they land), then each biome.
UE_BIOME_STEPS = (["t.MaxFPS 60", "paw.Cheat.UnlockTiers", "paw.Cat 1250 540", "paw.Wait 300"]
                  + sum([["paw.Travel %d" % n, "paw.Wait 90", "paw.Shot tour_biome_%d" % n] for n in range(1, 13)], [])
                  + ["paw.Wait 5", "paw.Shot tour_end quit"])


UE_TITLE_STEPS = ["t.MaxFPS 60", "paw.Wait 180", "paw.Shot tour_title quit"]

# The menus (restyle R4, 2026-11-08: until then the title, pause and settings shots in build/tour/ue were left over from
# older runs, nothing retook them): the title and the three settings sections on L_Title, the pause menu in the realm.
MENU_TITLE_SHOTS = ["title", "settings_audio", "settings_video", "settings_controls"]
UE_MENU_TITLE_STEPS = (["t.MaxFPS 60", "paw.Wait 180", "paw.Shot tour_title"]
                       + sum([["paw.Settings %s" % s, "paw.Wait 30", "paw.Shot tour_settings_%s" % s]
                              for s in ("audio", "video", "controls")], []))
UE_MENU_TITLE_STEPS[-1] += " quit"
UE_PAUSE_STEPS = ["t.MaxFPS 60", "paw.Wait 120", "paw.Press Escape", "paw.Wait 30", "paw.Shot tour_pause quit"]


RES = (1920, 1080)
UI_SCALE = 0.0
UE_DEST = "ue"


def ue_side(shots_list=None, steps=None, level="/Game/Pawpocalypse/Maps/L_Realm"):
    shots_list = shots_list or SHOTS
    steps = steps or UE_STEPS
    shots = ROOT / "Saved" / "PawShots"
    for s in shots_list:
        (shots / ("tour_%s.png" % s)).unlink(missing_ok=True)
        (OUT / UE_DEST / ("%s.png" % s)).unlink(missing_ok=True) # never leave an old shot to be mistaken for a new one
    if UI_SCALE:
        steps = ["paw.UiScale %g" % UI_SCALE] + list(steps)
    cmd = [EDITOR, str(UPROJECT), level, "-game", "-windowed", "-ResX=%d" % RES[0], "-ResY=%d" % RES[1],
           "-unattended", "-NoSave", "-nosplash", "-nosound", "-SCCProvider=None", "-ExecCmds=" + ",".join(steps), "-log=PawTour.log"]
    subprocess.run(cmd, timeout=900)
    dest = OUT / UE_DEST
    dest.mkdir(parents=True, exist_ok=True)
    for s in shots_list:
        src = shots / ("tour_%s.png" % s)
        if src.exists():
            shutil.copy2(src, dest / ("%s.png" % s))


FONT_MSGIDS = ["Continue", "New Game", "Settings", "Quit", "Upgrades", "Skill Tree", "Reawaken", "Buy", "Paused", "Resume"]


def font_lines():
    """culture -> the lines its sheet shows (Godot's translations; the port's own arrive in phase 7)."""
    sys.path.insert(0, str(ROOT / "tools"))
    import check_font_coverage as cfc
    import fonts
    locale = Path("C:/Games/pawpocalypse/locale")
    names = cfc.native_names()
    out = {}
    for po in [None] + sorted(locale.glob("*.po")):
        ids = cfc.po_strings(locale / "messages.pot" if po is None else po, "msgid")
        strs = ids if po is None else cfc.po_strings(po, "msgstr")
        pairs = {i: t for i, t in zip(ids, strs) if i and t and "%" not in t and "{" not in t and "[" not in t}
        culture = "en" if po is None else fonts.culture(po.stem)
        words = [pairs[m] for m in FONT_MSGIDS if m in pairs]
        longest = sorted(pairs.values(), key=len, reverse=True)[:3]
        out[culture] = ["%s  %s" % (culture, " · ".join(words[:4]))] + [" · ".join(names[i:i + 8]) for i in range(0, len(names), 8)] \
            + [" · ".join(words[4:])] + longest
    return out


def fonts_side():
    lines = font_lines()
    shots = ROOT / "Saved" / "PawShots"
    shots.mkdir(parents=True, exist_ok=True)
    steps = ["t.MaxFPS 60"]
    for culture, ls in lines.items():
        (shots / ("fontsheet_%s.txt" % culture)).write_text("\n".join(l.replace("\n", " ") for l in ls), encoding="utf-8")
        steps += ["paw.FontSheet %s" % culture, "paw.Wait 20", "paw.Shot tour_font_%s" % culture]
    steps[-1] += " quit"
    names = ["font_%s" % c for c in lines]
    ue_side(names, steps, "/Game/Pawpocalypse/Maps/L_Title")
    from PIL import Image, ImageDraw
    got = [OUT / UE_DEST / ("%s.png" % n) for n in names]
    got = [g for g in got if g.exists()]
    cols, w, h = 4, 640, 360
    sheet = Image.new("RGB", (cols * w, ((len(got) + cols - 1) // cols) * h), (18, 10, 23))
    for i, g in enumerate(got):
        sheet.paste(Image.open(g).convert("RGB").resize((w, h)), ((i % cols) * w, (i // cols) * h))
    dest = ROOT / "build" / "look" / "fonts_in_game.png"
    dest.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(dest)
    print("fonts: %d of %d languages shot; %s" % (len(got), len(names), dest))


def compare(shots_list=None):
    from PIL import Image, ImageDraw
    shots_list = shots_list or SHOTS
    dest = OUT / "compare"
    dest.mkdir(parents=True, exist_ok=True)
    missing = []
    for s in shots_list:
        g, u = OUT / "godot" / ("%s.png" % s), OUT / UE_DEST / ("%s.png" % s)
        if not (g.exists() and u.exists()):
            missing.append(s)
            continue
        a = Image.open(g).convert("RGB").resize((960, 540))
        b = Image.open(u).convert("RGB").resize((960, 540))
        sheet = Image.new("RGB", (1920, 560), (255, 255, 255))
        sheet.paste(a, (0, 20))
        sheet.paste(b, (960, 20))
        d = ImageDraw.Draw(sheet)
        d.text((8, 4), "Godot  %s" % s, fill=(0, 0, 0))
        d.text((968, 4), "Unreal  %s" % s, fill=(0, 0, 0))
        sheet.save(dest / ("%s.png" % s))
    print("compare: %d pairs in %s%s" % (len(shots_list) - len(missing), dest, ("; missing " + ", ".join(missing)) if missing else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot-exe", default="C:/Program Files/Godot/Godot.exe")
    ap.add_argument("--skip-godot", action="store_true")
    ap.add_argument("--skip-ue", action="store_true")
    ap.add_argument("--set", choices=["screens", "biomes", "fonts"], default="screens")
    ap.add_argument("--res", default="1920x1080")
    ap.add_argument("--uiscale", type=float, default=0.0, help="Slate's application scale (restyle R4h: 1.5 = the 150 %% check); "
                    "the shots go to build/tour/ue_<res>_<scale>, never over the 1080p set")
    args = ap.parse_args()
    global RES, UI_SCALE, UE_DEST
    RES = tuple(int(v) for v in args.res.lower().split("x"))
    UI_SCALE = args.uiscale
    if UI_SCALE or RES != (1920, 1080):
        UE_DEST = "ue_%dx%d_%s" % (RES[0], RES[1], ("%g" % UI_SCALE if UI_SCALE else "1"))
    if args.set == "fonts":
        fonts_side()
        return
    biomes = args.set == "biomes"
    shots_list = BIOME_SHOTS if biomes else SHOTS
    OUT.mkdir(parents=True, exist_ok=True)
    before = mgf.godot_status(Path("C:/Games/pawpocalypse"))
    if not args.skip_godot:
        godot_side(args.godot_exe, "biome_probe.gd" if biomes else "tour_probe.gd")
    if mgf.godot_status(Path("C:/Games/pawpocalypse")) != before:
        raise SystemExit("the Godot repo changed during the tour")
    if not args.skip_ue:
        ue_side(shots_list, UE_BIOME_STEPS if biomes else UE_STEPS)
        if biomes:
            ue_side(["title"], UE_TITLE_STEPS, "/Game/Pawpocalypse/Maps/L_Title")
        else:
            ue_side(MENU_TITLE_SHOTS, UE_MENU_TITLE_STEPS, "/Game/Pawpocalypse/Maps/L_Title")
            ue_side(["pause"], UE_PAUSE_STEPS)
            stale = [s for s in MENU_TITLE_SHOTS + ["pause"] if not (ROOT / "Saved" / "PawShots" / ("tour_%s.png" % s)).exists()]
            if stale:
                print("tour: NOT SHOT (the copies in build/tour/ue are old): " + ", ".join(stale))
    compare(shots_list)


if __name__ == "__main__":
    main()
