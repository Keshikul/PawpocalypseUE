"""The screenshot tour of this game: every screen, every biome or every language's font sheet, shot in the editor build.

Was the Unreal half of `godot_tour.py` (which also shot the first try, Godot, beside it); since D22 the Godot half is gone
(2026-11-24, HANDOFF-LOG (145)) and the tour is this game's own. It opens a `-game` window for a minute or two, with
`-NoSave` (the designer's saves and settings untouched) and the paw.* tour steps (Source/Pawpocalypse/Dev/PawShot.cpp).

    py tools/screen_tour.py [--set screens|biomes|fonts] [--res 1280x800] [--uiscale 1.5] [--culture de|pseudo]

--set screens (default): a new game in tier_1 with 1000 Catnip; the six dock tabs; the tree browsed; a card hand; a
          Reawaken with three jobs held (the ceremony at its hold, the spend step, the Unicorn, the job picker); the ending
          at 4200 s; then the title, the three settings pages and the pause menu        -> build/tour/<dest>/<shot>.png
--set biomes: each of the 12 biomes after travelling there, the cat parked at (1250, 540), and the title.
--set fonts: each language's font sheet (paw.FontSheet on L_Title): the language picker's names, the menu words and the
          longest lines of its own translation files, drawn in F_Paw -> build/tour/<dest>/font_<culture>.png and one contact
          sheet, build/look/fonts_in_game.png. The glyph gate is tools/check_font_coverage.py; this shows Slate drawing them.
<dest> is `ue` at 1920x1080 and UI scale 1; otherwise ue_<res>_<scale>[_<culture>], never over the 1080p set.

Refuses while an Unreal process of this project runs (tools/unreal_procs.py): never two runs at once, never the
designer's editor. Build the editor target first after a C++ change (the tour runs the editor build's DLLs).
"""
import argparse
import csv
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

OUT = ROOT / "build" / "tour"
BIOME_SHOTS = ["biome_%d" % n for n in range(1, 13)]
SHOTS = ["tab_upgrades", "tab_reawaken", "tab_realms", "tab_catches", "tab_awards", "tab_jobs", "tree_browse", "card_pick",
         "ceremony", "tree_spend", "unicorn", "job_picker", "victory"]
EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
UPROJECT = ROOT / "Pawpocalypse.uproject"
# Each step waits 30 frames after the one before (paw.Wait changes that once); frames are capped at 60 so a wait is real
# time (uncapped, the editor build ran ~500 fps and the ceremony shot landed 0.06 s in).
UE_STEPS = (["t.MaxFPS 60", "paw.Cheat.Catnip 1000"]
            + sum([["paw.Tab %d" % (i + 1), "paw.Wait 60", "paw.Shot tour_%s" % t] for i, t in enumerate(SHOTS[:6])], [])
            + ["paw.Tab 1", "paw.Tree", "paw.Wait 90", "paw.Shot tour_tree_browse", "paw.Leave",
               "paw.Hand", "paw.Wait 60", "paw.Shot tour_card_pick", "paw.Take", "paw.Wait 30",
               "paw.Reawaken jobs", "paw.Wait 30", "paw.Shot tour_ceremony", "paw.Wait 120", "paw.Shot tour_tree_spend",
               "paw.Cheat.Stardust 100000", "paw.Buy unlock_unicorn", "paw.Wait 96", "paw.Shot tour_unicorn", "paw.Wait 180",
               "paw.Leave", "paw.Wait 60", "paw.Shot tour_job_picker",
               "paw.Victory 4200", "paw.Wait 60", "paw.Shot tour_victory quit"])
# A moment for the shaders (a first run compiles them: the floor shows black until they land), then each biome.
UE_BIOME_STEPS = (["t.MaxFPS 60", "paw.Cheat.UnlockTiers", "paw.Cat 1250 540", "paw.Wait 300"]
                  + sum([["paw.Travel %d" % n, "paw.Wait 90", "paw.Shot tour_biome_%d" % n] for n in range(1, 13)], [])
                  + ["paw.Wait 5", "paw.Shot tour_end quit"])
UE_TITLE_STEPS = ["t.MaxFPS 60", "paw.Wait 180", "paw.Shot tour_title quit"]
# The menus (restyle R4, 2026-11-08: until then nothing retook them): the title and the three settings sections on L_Title,
# the pause menu in the realm.
MENU_TITLE_SHOTS = ["title", "settings_audio", "settings_video", "settings_controls"]
UE_MENU_TITLE_STEPS = (["t.MaxFPS 60", "paw.Wait 180", "paw.Shot tour_title"]
                       + sum([["paw.Settings %s" % s, "paw.Wait 30", "paw.Shot tour_settings_%s" % s]
                              for s in ("audio", "video", "controls")], []))
UE_MENU_TITLE_STEPS[-1] += " quit"
UE_PAUSE_STEPS = ["t.MaxFPS 60", "paw.Wait 120", "paw.Press Escape", "paw.Wait 30", "paw.Shot tour_pause quit"]

RES = (1920, 1080)
UI_SCALE = 0.0
DEST = "ue"
CULTURE = "en"      # --culture: the language shown; "pseudo": English made pseudo (-PseudoLoc)


def shoot(shots_list, steps, level="/Game/Pawpocalypse/Maps/L_Realm"):
    """One -game run of `steps`; copies each tour_<shot>.png it took to build/tour/<DEST>/<shot>.png."""
    shots = ROOT / "Saved" / "PawShots"
    for s in shots_list:
        (shots / ("tour_%s.png" % s)).unlink(missing_ok=True)
        (OUT / DEST / ("%s.png" % s)).unlink(missing_ok=True)  # never leave an old shot to be mistaken for a new one
    if UI_SCALE:
        steps = ["paw.UiScale %g" % UI_SCALE] + list(steps)
    culture = ["-culture=en", "-PseudoLoc"] if CULTURE == "pseudo" else ["-culture=" + CULTURE]
    cmd = [EDITOR, str(UPROJECT), level, "-game", "-windowed", "-ResX=%d" % RES[0], "-ResY=%d" % RES[1],
           "-unattended", "-NoSave", "-nosplash", "-nosound", "-SCCProvider=None"] + culture + [
           "-ExecCmds=" + ",".join(steps), "-log=PawTour.log"]
    subprocess.run(cmd, timeout=900)
    dest = OUT / DEST
    dest.mkdir(parents=True, exist_ok=True)
    missing = []
    for s in shots_list:
        src = shots / ("tour_%s.png" % s)
        if src.exists():
            shutil.copy2(src, dest / ("%s.png" % s))
        else:
            missing.append(s)
    return missing


FONT_WORDS = ["Continue", "New Game", "Settings", "Quit", "Upgrades", "Skill Tree", "Reawaken", "Buy", "Paused", "Resume"]


def font_lines():
    """culture -> the lines its sheet shows: the picker's names, the menu words and the longest lines, from this game's own
    string tables (English) and translation files (Content/Pawpocalypse/Localization/Translations/<culture>/*.csv)."""
    import check_font_coverage as cfc
    names = cfc.native_names()
    loc = ROOT / "Content" / "Pawpocalypse" / "Localization"
    tables = {"en": [(r["SourceString"], r["SourceString"]) for f in sorted((loc / "Strings").glob("*.csv"))
                     for r in csv.DictReader(f.open(encoding="utf-8"))]}
    for folder in sorted(p for p in (loc / "Translations").iterdir() if p.is_dir()):
        tables[folder.name] = [(r["Source"], r["Text"]) for f in sorted(folder.glob("*.csv"))
                               for r in csv.DictReader(f.open(encoding="utf-8"))]
    out = {}
    for culture, rows in tables.items():
        pairs = {src: txt for src, txt in rows if src and txt and "{" not in txt and "<" not in txt}
        words = [pairs[w] for w in FONT_WORDS if w in pairs]
        longest = sorted(set(pairs.values()), key=len, reverse=True)[:3]
        out[culture] = (["%s  %s" % (culture, " · ".join(words[:4]))] + [" · ".join(names[i:i + 8]) for i in range(0, len(names), 8)]
                        + [" · ".join(words[4:])] + longest)
    return out


def fonts_set():
    lines = font_lines()
    shots = ROOT / "Saved" / "PawShots"
    shots.mkdir(parents=True, exist_ok=True)
    steps = ["t.MaxFPS 60"]
    for culture, ls in lines.items():
        (shots / ("fontsheet_%s.txt" % culture)).write_text("\n".join(l.replace("\n", " ") for l in ls), encoding="utf-8")
        steps += ["paw.FontSheet %s" % culture, "paw.Wait 20", "paw.Shot tour_font_%s" % culture]
    steps[-1] += " quit"
    names = ["font_%s" % c for c in lines]
    missing = shoot(names, steps, "/Game/Pawpocalypse/Maps/L_Title")
    from PIL import Image
    got = [OUT / DEST / ("%s.png" % n) for n in names if n not in missing]
    cols, w, h = 4, 640, 360
    sheet = Image.new("RGB", (cols * w, max(1, (len(got) + cols - 1) // cols) * h), (42, 33, 49))
    for i, g in enumerate(got):
        sheet.paste(Image.open(g).convert("RGB").resize((w, h)), ((i % cols) * w, (i // cols) * h))
    dest = ROOT / "build" / "look" / "fonts_in_game.png"
    dest.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(dest)
    print("tour: %d of %d languages shot; %s" % (len(got), len(names), dest))
    return 0 if not missing else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--set", choices=["screens", "biomes", "fonts"], default="screens")
    ap.add_argument("--res", default="1920x1080")
    ap.add_argument("--culture", default="en", help="the language shown (an Unreal culture), or pseudo (-PseudoLoc)")
    ap.add_argument("--uiscale", type=float, default=0.0, help="Slate's application scale (1.5 = the 150 %% check)")
    args = ap.parse_args()
    global RES, UI_SCALE, DEST, CULTURE
    RES = tuple(int(v) for v in args.res.lower().split("x"))
    UI_SCALE = args.uiscale
    CULTURE = args.culture
    if UI_SCALE or RES != (1920, 1080):
        DEST = "ue_%dx%d_%s" % (RES[0], RES[1], ("%g" % UI_SCALE if UI_SCALE else "1"))
    if CULTURE != "en":
        DEST += "_" + CULTURE
    from unreal_procs import print_blocking, wait_blocking
    found = wait_blocking()
    if found:
        print("tour: an Unreal process of this project is running; close it first (this never closes it for you)")
        print_blocking(found)
        return 2
    OUT.mkdir(parents=True, exist_ok=True)
    if args.set == "fonts":
        return fonts_set()
    if args.set == "biomes":
        missing = shoot(BIOME_SHOTS, UE_BIOME_STEPS) + shoot(["title"], UE_TITLE_STEPS, "/Game/Pawpocalypse/Maps/L_Title")
    else:
        missing = (shoot(SHOTS, UE_STEPS) + shoot(MENU_TITLE_SHOTS, UE_MENU_TITLE_STEPS, "/Game/Pawpocalypse/Maps/L_Title")
                   + shoot(["pause"], UE_PAUSE_STEPS))
    print("tour: %s -> %s%s" % (args.set, OUT / DEST, ("; NOT SHOT: " + ", ".join(missing)) if missing else ""))
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())
