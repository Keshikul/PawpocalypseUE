"""The Steam store screenshots: the real game, scripted, the same scenes every run (the night of 2026-11-17).

    py tools/store_shots.py [--res 1920x1080] [--only NAME ...]

One run of the game (the editor build with -game, as the screenshot tour), a FRESH sandboxed save folder
(build/steam/shots/saves: never the designer's saves), the debug readout off, frames capped at 60. Each scene is set up by
paw.* steps in the tour queue; cheats run in order through paw.Do (queued). The shots land in Saved/PawShots/store_<name>.png
and are copied to build/steam/shots/<name>.png, each with the step list that made it (shots.txt).

Valve's rules (Steam/05): real gameplay only, 1920x1080 or more, 16:9, no marketing text; the HUD stays on.
"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import unreal_procs  # noqa: E402

EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
OUT = ROOT / "build" / "steam" / "shots"
SAVES = OUT / "saves"

# Every level up deals a card hand and holds the game until a card is taken (the first run, 2026-11-18: six of nine shots
# were a hand, and the Reawaken never started). CLEAR takes whatever hands are open (paw.Take with none open does nothing).
CLEAR = ["paw.Take", "paw.Wait 10"] * 10

# (name, what it shows, the steps before the shot). The cat sits where the dock leaves the field clear. The boss comes
# first, on a fresh game: the unlock cheat marks every boss beaten, and a beaten boss is not summoned again (the
# third run, 2026-11-18); at full strength the cat would kill it the frame it arrives.
SCENES = [
    ("01_backyard_hunt", "the first realm: the paw on a crowd, the Upgrades tab open",
     ["paw.Do paw.Cheat.Catnip 2500", "paw.Tab 1", "paw.Cat 760 560", "paw.Wait 720"] + CLEAR + ["paw.Wait 10"]),
    # The boss arrives about where the other scenes park the cat (2026-11-18: the cat stood on the Tyrant), so here the
    # cat waits low on the left.
    ("02_park_boss", "the Park Tyrant called with Bait (paw.Boss travels to the first boss realm on a fresh game)",
     CLEAR + ["paw.Boss", "paw.Cat 330 820", "paw.Wait 150"] + CLEAR + ["paw.Wait 10"]),
    ("03_park_aura_kittens", "the Park with the aura, kittens and every upgrade bought; the cat evolved",
     ["paw.Do paw.Cheat.UnlockStages", "paw.Travel 3", "paw.Do paw.Cheat.MaxUpgrades", "paw.Cat 760 560", "paw.Wait 420"]
     + CLEAR + ["paw.Wait 10"]),
    ("04_card_pick", "a level up: three cards",
     CLEAR + ["paw.Hand", "paw.Wait 60"]),
    ("05_realms_tab", "the Realms tab: twelve realms",
     CLEAR + ["paw.Tab 3", "paw.Wait 40"] + CLEAR + ["paw.Wait 10"]),
    ("06_reawaken", "the Reawaken ceremony",
     CLEAR + ["paw.Tab 1", "paw.Reawaken jobs", "paw.Wait 30"]),
    ("07_skill_tree", "the skill tree (the spend step)",
     ["paw.Wait 120"]),
    ("08_unicorn", "the Unicorn bought: the cat climbs on",
     ["paw.Do paw.Cheat.Stardust 100000", "paw.Buy unlock_unicorn", "paw.Wait 96"]),
    ("09_mythic_rider", "a late realm on the unicorn after a Reawaken (the upgrades reset: few level ups)",
     ["paw.Wait 180", "paw.Leave", "paw.Wait 60", "paw.Press Enter", "paw.Wait 60",
      "paw.Do paw.Cheat.UnlockStages", "paw.Travel 8", "paw.Cat 760 560", "paw.Wait 360"]
     + CLEAR + ["paw.Wait 10"]),
]


def steps_for(scenes):
    steps = ["t.MaxFPS 60", "paw.DebugHud 0"]
    for name, _what, pre in scenes:
        steps += pre + ["paw.Shot store_%s" % name]
    steps[-1] += " quit"
    return steps


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--res", default="1920x1080")
    ap.add_argument("--only", nargs="*")
    args = ap.parse_args(argv)
    found = unreal_procs.blocking()
    if found:
        print("store_shots: an Unreal process of this project is running; close it first")
        unreal_procs.print_blocking(found)
        return 2
    w, h = (int(v) for v in args.res.split("x"))
    scenes = [s for s in SCENES if not args.only or s[0] in args.only]
    shots = ROOT / "Saved" / "PawShots"
    for name, _w, _p in scenes:
        (shots / ("store_%s.png" % name)).unlink(missing_ok=True)
        (OUT / ("%s.png" % name)).unlink(missing_ok=True)   # never an old shot mistaken for a new one
    if not args.only:   # a whole run: no shot of a scene that is no longer in the list either (a renamed scene's old file)
        for old in OUT.glob("*.png"):
            old.unlink()
    if SAVES.exists():
        shutil.rmtree(SAVES)
    SAVES.mkdir(parents=True)
    steps = steps_for(scenes)
    cmd = [EDITOR, str(ROOT / "Pawpocalypse.uproject"), "/Game/Pawpocalypse/Maps/L_Realm", "-game", "-windowed",
           "-ResX=%d" % w, "-ResY=%d" % h, "-unattended", "-NoSave", "-nosplash", "-nosound", "-SCCProvider=None", "-culture=en",
           "-PawSaveDir=%s" % SAVES, "-ExecCmds=" + ",".join(steps), "-log=PawStoreShots.log"]
    subprocess.run(cmd, timeout=1800)
    OUT.mkdir(parents=True, exist_ok=True)
    lines, missing = [], []
    for name, what, pre in scenes:
        src = shots / ("store_%s.png" % name)
        if src.exists():
            shutil.copy2(src, OUT / ("%s.png" % name))
            lines.append("%s  %s\n    steps: %s" % (name, what, ", ".join(pre)))
        else:
            missing.append(name)
    (OUT / "shots.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("store_shots: %d of %d shots -> %s%s" % (len(scenes) - len(missing), len(scenes), OUT,
                                                   ("; MISSING " + ", ".join(missing)) if missing else ""))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
