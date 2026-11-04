"""Where does a Reawaken land in the REAL Godot game? (designer 2026-11-01: "Territory wins")

Runs tools/godot_fixtures/reawaken_travel_probe.gd in the scratch copy of the Godot project (build/godot_fixtures/project,
the same copy tools/make_godot_fixtures.py uses), headless, with APPDATA/LOCALAPPDATA in a sandbox, one Godot process per
scenario. Each run presses Reawaken on the real HUD, takes any card pick, closes the spend step's skill tree, and reports
where the player ended up. Required, per scenario:
  terr_deep     Legacy 5 (Territory -> biome 4), Reawaken in tier_5  -> spend step seen, then tier_4
  none          no Territory, Reawaken in tier_5                      -> spend step seen, then tier_1
  terr_shallow  Legacy 5, Reawaken in tier_2                          -> spend step seen, then tier_4
  terr_same     Legacy 5, Reawaken in tier_4                          -> spend step seen, then tier_4

The Godot repo is never written; its `git status` is compared before and after.

  py tools/check_godot_reawaken_travel.py            # the Godot working tree as it is
  py tools/check_godot_reawaken_travel.py --head     # negative test: hud.gd and tier_controller.gd from Godot's HEAD
                                                     # (before the decision) in the copy; must FAIL
Exit 0 when every scenario lands as required.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PROJECT = ROOT / "build" / "godot_fixtures" / "project"
PROBE = ROOT / "tools" / "godot_fixtures"
EXPECTED = {
    "terr_deep": "tier_4",
    "none": "tier_1",
    "terr_shallow": "tier_4",
    "terr_same": "tier_4",
}
SYNCED = ("scripts", "scenes", "data", "ci", "locale")
OVERLAID = ("scripts/systems/hud.gd", "scripts/systems/tier_controller.gd")
END = re.compile(r"^PROBE end spend_step_seen=(true|false) scene=(\S+) tree=\S+ biome=(\S*) ")


def godot_status(godot):
    return subprocess.run(["git", "-C", str(godot), "status", "--porcelain"], capture_output=True, text=True).stdout


def sync(godot):
    if not (PROJECT / "project.godot").exists():
        print("copying the Godot project (first run) ...")
        shutil.copytree(godot, PROJECT, ignore=shutil.ignore_patterns(".git"))
    for d in SYNCED:
        if (PROJECT / d).exists():
            shutil.rmtree(PROJECT / d)
        shutil.copytree(godot / d, PROJECT / d)
    shutil.copy2(godot / "project.godot", PROJECT / "project.godot")
    for name in ("reawaken_travel_probe.gd", "reawaken_travel_probe.tscn"):
        shutil.copy2(PROBE / name, PROJECT / name)


def overlay_head(godot):
    for rel in OVERLAID:
        text = subprocess.run(["git", "-C", str(godot), "show", "HEAD:" + rel], capture_output=True, check=True).stdout
        (PROJECT / rel).write_bytes(text)


def run(exe, scenario):
    sandbox = ROOT / "build" / "godot_probe_appdata" / scenario
    if sandbox.exists():
        shutil.rmtree(sandbox)
    for sub in ("Roaming", "Local"):
        (sandbox / sub).mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, APPDATA=str(sandbox / "Roaming"), LOCALAPPDATA=str(sandbox / "Local"))
    r = subprocess.run([exe, "--headless", "--path", str(PROJECT), "res://reawaken_travel_probe.tscn", "--", scenario],
                       capture_output=True, text=True, env=env, timeout=300)
    log = ROOT / "build" / "godot_probe_ci" / ("reawaken_" + scenario + ".log")
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
    for line in r.stdout.splitlines():
        m = END.match(line)
        if m:
            return m.group(1) == "true", m.group(3), log
    return None, None, log


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--godot", default="C:/Games/pawpocalypse")
    ap.add_argument("--godot-exe", default="C:/Program Files/Godot/Godot.exe")
    ap.add_argument("--head", action="store_true", help="negative test: the two travel scripts from Godot's HEAD")
    ap.add_argument("--only", choices=sorted(EXPECTED), action="append")
    args = ap.parse_args()
    godot = Path(args.godot)
    before = godot_status(godot)
    sync(godot)
    if args.head:
        overlay_head(godot)
    failures = 0
    for scenario in args.only or list(EXPECTED):
        spend, biome, log = run(args.godot_exe, scenario)
        ok = spend is True and biome == EXPECTED[scenario]
        failures += 0 if ok else 1
        print("%-13s %s  spend step %s, landed in %s (required: spend step, %s)  %s"
              % (scenario, "PASS" if ok else "FAIL", {True: "seen", False: "MISSED", None: "?"}[spend], biome or "?",
                 EXPECTED[scenario], "" if ok else log))
    if godot_status(godot) != before:
        print("FAIL: the Godot repo's git status changed during the run")
        failures += 1
    print("REAWAKEN TRAVEL: %s" % ("PASS" if failures == 0 else "FAIL (%d)" % failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
