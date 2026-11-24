"""The soak (docs/09 §5, L6; phase 8's exit): `paw.Soak` in a packaged Development build plays a whole game.

Launches the INNER exe of a packaged build (as tools/packaged_bench.py) on L_Realm with no saves (-NoSave: a fresh game, the
designer's own game untouched), windowed at 1280x800, the frame capped at 60 (a player's frame, not the bench's flat out),
and runs `paw.Soak <game-minutes> <speed>` (Source/Pawpocalypse/Dev/PawSoak.cpp: the UI's own presses, a report, quit).
The build's Saved folder is cleared first and left for reading (report: Saved/PawSoak/report.txt; the log).

    py tools/packaged_soak.py [--speed 5] [--minutes 240] [--fps N] [--build build/package] [--timeout 7200] [--arg=-x ...]
    --speed 5: the pace (docs/09 §5, "trust 5x for pace"); --speed 20: stability.
Exit 0 = PAW_SOAK PASS (the game won, no Error line, no stall); 1 = FAIL or no verdict; 2 = no build, or one is running.
The report is copied to build/soak/<stamp>-<speed>x.txt.
"""
import argparse
import datetime
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", default=os.path.join(ROOT, "build", "package"))
    ap.add_argument("--speed", type=float, default=5.0)
    ap.add_argument("--minutes", type=float, default=240.0, help="the game-minute limit")
    ap.add_argument("--fps", type=int, default=None, help="the frame cap (0 = none; default 60 up to 7x, else none: the "
                    "simulation catches up at most 8 steps of 1/60 s a frame, so 20x needs more than 60 frames a second)")
    ap.add_argument("--timeout", type=float, default=7200.0, help="real seconds before the run is called hung")
    ap.add_argument("--arg", action="append", help="an extra command-line argument (--arg=-ini:...), repeatable "
                    "(tools/packaged_pso.py: the PSO validation)")
    args = ap.parse_args()
    if args.fps is None:
        args.fps = 60 if args.speed <= 7 else 0

    game = os.path.join(args.build, "Windows", "Pawpocalypse")
    exe = os.path.join(game, "Binaries", "Win64", "Pawpocalypse.exe")
    if not os.path.isfile(exe):
        print("packaged_soak: no packaged build at %s (package a Development build first)" % exe)
        return 2
    running = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Pawpocalypse.exe"], capture_output=True, text=True).stdout
    if "Pawpocalypse.exe" in running:
        print("packaged_soak: a packaged Pawpocalypse.exe is running; close it first (this never closes it for you)")
        return 2
    saved = os.path.join(game, "Saved")
    shutil.rmtree(saved, ignore_errors=True)

    cmd = [exe, "/Game/Pawpocalypse/Maps/L_Realm", "-NoSave", "-windowed", "-ResX=1280", "-ResY=800", "-log", "-stdout",
           "-ExecCmds=r.VSync 0,t.MaxFPS %d,paw.Soak %g %g" % (args.fps, args.minutes, args.speed)] + (args.arg or [])
    print("packaged_soak: %gx, at most %g game minutes, frame cap %d (timeout %.0f s)" % (args.speed, args.minutes, args.fps, args.timeout), flush=True)
    lines, verdict = [], None
    try:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")
        deadline = datetime.datetime.now() + datetime.timedelta(seconds=args.timeout)
        for line in p.stdout:
            if "PAW_SOAK" in line:
                text = line.split("PAW_SOAK", 1)[1].strip()
                lines.append(text)
                print("  " + text, flush=True)
                if text.startswith("PASS") or text.startswith("FAIL"):
                    verdict = text
            if datetime.datetime.now() > deadline:
                p.kill()
                print("packaged_soak: timed out after %.0f s" % args.timeout)
                break
        p.wait(timeout=120)
    except subprocess.TimeoutExpired:
        p.kill()
    out = os.path.join(ROOT, "build", "soak")
    os.makedirs(out, exist_ok=True)
    report = os.path.join(out, "%s-%gx.txt" % (datetime.datetime.now().strftime("%Y%m%d_%H%M%S"), args.speed))
    with open(report, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print("packaged_soak: %s (report %s; the build's log in %s)" % (verdict or "no verdict", report, os.path.join(saved, "Logs")))
    return 0 if verdict == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
