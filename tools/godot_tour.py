"""The screenshot tour, both games (docs/11 phase 3 exit: every screen against the Godot shots).

The SAME steps and state on each side (a new game in tier_1 with 1000 Catnip; the six tabs; the tree browsed; a card hand; a
Reawaken with three jobs held: the ceremony at its hold, the spend step, the job picker; the ending at 4200 s):
  Godot   tools/godot_fixtures/tour_probe.gd in the scratch copy (build/godot_fixtures/project), sandboxed APPDATA, rendered
          fullscreen with opengl3 (NOT --headless: the dummy rasteriser writes blank PNGs; fullscreen, because a 1920x1080
          window on a 1920x1080 desktop is shrunk and widens the canvas)            -> build/tour/godot/<shot>.png
  Unreal  the editor build with -game and the paw.* tour steps (Dev/PawShot.cpp)    -> build/tour/ue/<shot>.png
Then each pair side by side (Godot left)                                            -> build/tour/compare/<shot>.png

    py tools/godot_tour.py [--skip-godot] [--skip-ue]

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
SHOTS = ["tab_upgrades", "tab_reawaken", "tab_realms", "tab_catches", "tab_awards", "tab_jobs", "tree_browse", "card_pick",
         "ceremony", "tree_spend", "job_picker", "victory"]
EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
UPROJECT = ROOT / "Pawpocalypse.uproject"
# The Unreal tour: each step waits 30 frames after the one before (paw.Wait changes that once); frames are capped at 60
# so a wait is real time (uncapped, the editor build ran ~500 fps and the ceremony shot landed 0.06 s in).
UE_STEPS = (["t.MaxFPS 60", "paw.Cheat.Catnip 1000"]
            + sum([["paw.Tab %d" % (i + 1), "paw.Wait 60", "paw.Shot tour_%s" % t] for i, t in enumerate(SHOTS[:6])], [])
            + ["paw.Tab 1", "paw.Tree", "paw.Wait 90", "paw.Shot tour_tree_browse", "paw.Leave",
               "paw.Hand", "paw.Wait 60", "paw.Shot tour_card_pick", "paw.Take", "paw.Wait 30",
               "paw.Reawaken jobs", "paw.Wait 30", "paw.Shot tour_ceremony", "paw.Wait 120", "paw.Shot tour_tree_spend",
               "paw.Leave", "paw.Wait 60", "paw.Shot tour_job_picker",
               "paw.Victory 4200", "paw.Wait 60", "paw.Shot tour_victory quit"])


def godot_side(exe):
    project = mgf.WORK / "project"
    mgf.copy_project(Path("C:/Games/pawpocalypse"), project, True)
    shutil.copy2(mgf.SCRIPTS / "tour_probe.gd", project / "tour_probe.gd")
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


def ue_side():
    shots = ROOT / "Saved" / "PawShots"
    for s in SHOTS:
        (shots / ("tour_%s.png" % s)).unlink(missing_ok=True)
    cmd = [EDITOR, str(UPROJECT), "/Game/Pawpocalypse/Maps/L_Realm", "-game", "-windowed", "-ResX=1920", "-ResY=1080",
           "-unattended", "-NoSave", "-nosplash", "-nosound", "-SCCProvider=None", "-ExecCmds=" + ",".join(UE_STEPS), "-log=PawTour.log"]
    subprocess.run(cmd, timeout=900)
    dest = OUT / "ue"
    dest.mkdir(parents=True, exist_ok=True)
    for s in SHOTS:
        src = shots / ("tour_%s.png" % s)
        if src.exists():
            shutil.copy2(src, dest / ("%s.png" % s))


def compare():
    from PIL import Image, ImageDraw
    dest = OUT / "compare"
    dest.mkdir(parents=True, exist_ok=True)
    missing = []
    for s in SHOTS:
        g, u = OUT / "godot" / ("%s.png" % s), OUT / "ue" / ("%s.png" % s)
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
    print("compare: %d pairs in %s%s" % (len(SHOTS) - len(missing), dest, ("; missing " + ", ".join(missing)) if missing else ""))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot-exe", default="C:/Program Files/Godot/Godot.exe")
    ap.add_argument("--skip-godot", action="store_true")
    ap.add_argument("--skip-ue", action="store_true")
    args = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    before = mgf.godot_status(Path("C:/Games/pawpocalypse"))
    if not args.skip_godot:
        godot_side(args.godot_exe)
    if mgf.godot_status(Path("C:/Games/pawpocalypse")) != before:
        raise SystemExit("the Godot repo changed during the tour")
    if not args.skip_ue:
        ue_side()
    compare()


if __name__ == "__main__":
    main()
