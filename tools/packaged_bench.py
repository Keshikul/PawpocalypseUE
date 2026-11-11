"""The packaged-build frame gate (docs/09 L5; docs/11 phase 2 exit): run `paw.Bench` in a packaged Development build.

Launches the INNER exe of a packaged build (KB U/19 §2: the outer stub spawns a child) on L_Realm, windowed at 1280x800 with
vsync and the frame cap off, runs `paw.Bench <seconds>` (Source/Pawpocalypse/Dev/PawBench.cpp: fill the field to the live cap,
measure, check the cooked drawing, screenshot, quit), and turns its PAW_BENCH lines into an exit code. The build's own Saved
folder is cleared first (and its report, screenshot and log are left there for reading); nothing outside the build is written.

    py tools/packaged_bench.py [--build build/package] [--seconds 20] [--res 1280x800]
Exit 0 = PAW_BENCH PASS; 1 = FAIL or no verdict; 2 = no packaged build, or a packaged copy is already running.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", default=os.path.join(ROOT, "build", "package"))
    ap.add_argument("--seconds", type=float, default=20.0)
    ap.add_argument("--res", default="1280x800")
    ap.add_argument("--timeout", type=float, default=300.0)
    ap.add_argument("--exec", default="", help="console commands run before the bench, comma-separated (e.g. paw.Rig 0)")
    args = ap.parse_args()

    game = os.path.join(args.build, "Windows", "Pawpocalypse")
    exe = os.path.join(game, "Binaries", "Win64", "Pawpocalypse.exe")
    if not os.path.isfile(exe):
        print("packaged_bench: no packaged build at %s (package a Development build first)" % exe)
        return 2
    running = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Pawpocalypse.exe"], capture_output=True, text=True).stdout
    if "Pawpocalypse.exe" in running:
        print("packaged_bench: a packaged Pawpocalypse.exe is running; close it first (this never closes it for you)")
        return 2
    saved = os.path.join(game, "Saved")
    shutil.rmtree(saved, ignore_errors=True)

    w, h = args.res.lower().split("x")
    cmd = [exe, "/Game/Pawpocalypse/Maps/L_Realm", "-NoSave", "-windowed", "-ResX=%s" % w, "-ResY=%s" % h, "-log", "-stdout",
           "-ExecCmds=r.VSync 0,t.MaxFPS 0,%spaw.Bench %g" % ((args.exec + ",") if args.exec else "", args.seconds)]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=args.timeout)
        text = r.stdout + r.stderr
    except subprocess.TimeoutExpired as e:
        text = (e.stdout or b"").decode("utf-8", "replace") if isinstance(e.stdout, bytes) else (e.stdout or "")
        print("packaged_bench: timed out after %.0f s" % args.timeout)
    log = os.path.join(saved, "Logs", "Pawpocalypse.log")
    if os.path.isfile(log):
        with open(log, encoding="utf-8", errors="replace") as f:
            text = f.read()
    lines = [m.group(1) for m in re.finditer(r"PAW_BENCH (.*)", text)]
    for line in lines:
        print(line)
    for line in text.splitlines():
        if re.search(r"Error: .*(Failed to find|Failed to load|LogPaw)", line):
            print("  " + line.strip())
    verdict = [l for l in lines if l.startswith("PASS") or l.startswith("FAIL")]
    shot = os.path.join(saved, "PawBench", "bench.png")
    print("screenshot: %s" % (shot if os.path.isfile(shot) else "none"))
    if not verdict:
        print("packaged_bench: FAIL, no verdict (log: %s)" % log)
        return 1
    print("packaged_bench: %s" % ("PASS" if verdict[-1].startswith("PASS") else "FAIL"))
    return 0 if verdict[-1].startswith("PASS") else 1


if __name__ == "__main__":
    sys.exit(main())
