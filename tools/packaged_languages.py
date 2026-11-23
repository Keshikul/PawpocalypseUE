"""The packaged language check (docs/09 L5; phase 7, docs/WORKLIST-LOC-2026-11-18.md §3 step 3): every shipped language in the package.

The editor's tests prove the translations with the editor's files (every culture's ICU data, the loose translation files beside
the project); a package has only what was staged. This runs `paw.Languages` (Source/Pawpocalypse/Dev/PawLanguages.cpp) in a
packaged Development build, on the title, twice, launching the INNER exe (KB U/19 §2), windowed at 1280x800, with -NoSave:

  1. -culture=<start> and `paw.Languages start=<start>`: the game starts in that language and the engine shows it;
  2. `paw.Languages`: the title's list offers every language with a translation file; each is picked there as a player picks
     it and must be complete, current in the engine, every line of the string tables showing its translation, every
     character in a face of F_Paw that has it, and (Thai, Japanese, Chinese) a line break inside a run of letters.

Every culture folder of Content/Pawpocalypse/Localization/Translations, and English, must come back "ok". The build's own Saved
folder is cleared first; the reports, logs and screenshots are copied to build/loc/package/, with sheet.png (every language's
title with its list open, for the eye).

    py tools/packaged_languages.py [--build build/package] [--start ru]
Exit 0 = PASS; 1 = FAIL or no verdict; 2 = no packaged build, or a packaged copy is already running.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TRANSLATIONS = os.path.join(ROOT, "Content", "Pawpocalypse", "Localization", "Translations")
OUT = os.path.join(ROOT, "build", "loc", "package")


def expected_cultures():
    found = sorted(d for d in os.listdir(TRANSLATIONS) if os.path.isdir(os.path.join(TRANSLATIONS, d)))
    return ["en"] + found


def run(exe, game, extra, timeout):
    """One launch; returns the PAW_LANG lines and the error lines of its log."""
    shutil.rmtree(os.path.join(game, "Saved"), ignore_errors=True)
    cmd = [exe, "/Game/Pawpocalypse/Maps/L_Title", "-NoSave", "-windowed", "-ResX=1280", "-ResY=800", "-log", "-stdout"] + extra
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=timeout)
        text = r.stdout + r.stderr
    except subprocess.TimeoutExpired as e:
        text = e.stdout.decode("utf-8", "replace") if isinstance(e.stdout, bytes) else (e.stdout or "")
        print("packaged_languages: timed out after %.0f s" % timeout)
    log = os.path.join(game, "Saved", "Logs", "Pawpocalypse.log")
    if os.path.isfile(log):
        with open(log, encoding="utf-8", errors="replace") as f:
            text = f.read()
    lines = [m.group(1) for m in re.finditer(r"PAW_LANG (.*)", text)]
    errors = [l.strip() for l in text.splitlines() if re.search(r"Error: |Settings: the engine has no culture", l)]
    return lines, errors, log


def keep(game, name):
    """Copy a run's report, log and screenshots out of the build before the next run clears its Saved folder."""
    os.makedirs(OUT, exist_ok=True)
    saved = os.path.join(game, "Saved")
    report = os.path.join(saved, "PawLanguages", "report.txt")
    if os.path.isfile(report):
        shutil.copyfile(report, os.path.join(OUT, "report_%s.txt" % name))
    log = os.path.join(saved, "Logs", "Pawpocalypse.log")
    if os.path.isfile(log):
        shutil.copyfile(log, os.path.join(OUT, "log_%s.txt" % name))
    shots = os.path.join(saved, "PawLanguages")
    if os.path.isdir(shots):
        for f in os.listdir(shots):
            if f.endswith(".png"):
                shutil.copyfile(os.path.join(shots, f), os.path.join(OUT, f))


def sheet(cultures):
    """Every language's title with its list open, side by side (half size), for the designer's eye."""
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        return None
    tiles = [(c, os.path.join(OUT, "%s.png" % c)) for c in cultures if os.path.isfile(os.path.join(OUT, "%s.png" % c))]
    if not tiles:
        return None
    w, h, cols = 640, 400, 6
    rows = (len(tiles) + cols - 1) // cols
    out = Image.new("RGB", (cols * w, rows * (h + 24)), (24, 24, 24))
    draw = ImageDraw.Draw(out)
    for i, (culture, path) in enumerate(tiles):
        img = Image.open(path).convert("RGB").resize((w, h), Image.LANCZOS)
        x, y = (i % cols) * w, (i // cols) * (h + 24)
        out.paste(img, (x, y + 24))
        draw.text((x + 6, y + 5), culture, fill=(240, 240, 240))
    path = os.path.join(OUT, "sheet.png")
    out.save(path)
    return path


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")  # the lines hold every script
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build", default=os.path.join(ROOT, "build", "package"))
    ap.add_argument("--start", default="ru", help="the culture the first run starts in (-culture=)")
    ap.add_argument("--timeout", type=float, default=300.0)
    args = ap.parse_args()

    game = os.path.join(args.build, "Windows", "Pawpocalypse")
    exe = os.path.join(game, "Binaries", "Win64", "Pawpocalypse.exe")
    if not os.path.isfile(exe):
        print("packaged_languages: no packaged build at %s (package a Development build first)" % exe)
        return 2
    running = subprocess.run(["tasklist", "/FI", "IMAGENAME eq Pawpocalypse.exe"], capture_output=True, text=True).stdout
    if "Pawpocalypse.exe" in running:
        print("packaged_languages: a packaged Pawpocalypse.exe is running; close it first (this never closes it for you)")
        return 2
    shutil.rmtree(OUT, ignore_errors=True)
    expected = expected_cultures()
    failures = []

    print("run 1: the game started in '%s'" % args.start)
    lines, errors, _ = run(exe, game, ["-culture=%s" % args.start, "-ExecCmds=paw.Languages start=%s" % args.start], args.timeout)
    keep(game, "start")
    for line in lines:
        print("  " + line)
    verdict = [l for l in lines if l.startswith("PASS") or l.startswith("FAIL")]
    if not verdict or not verdict[-1].startswith("PASS"):
        failures.append("start: %s" % (verdict[-1] if verdict else "no verdict"))

    print("run 2: every language picked on the title")
    lines, errors2, _ = run(exe, game, ["-ExecCmds=paw.Languages"], args.timeout)
    keep(game, "every")
    for line in lines:
        print("  " + line)
    verdict = [l for l in lines if l.startswith("PASS") or l.startswith("FAIL")]
    if not verdict or not verdict[-1].startswith("PASS"):
        failures.append("every language: %s" % (verdict[-1][:300] if verdict else "no verdict"))
    seen = {}
    for line in lines:
        m = re.match(r"lang (\S+) culture=(\S+) .* (ok|FAIL .*)$", line)
        if m:
            seen[m.group(2)] = m.group(3)
    for culture in expected:
        if culture not in seen:
            failures.append("%s: not looked at in the package (no translations loaded for it?)" % culture)
        elif seen[culture] != "ok":
            failures.append("%s: %s" % (culture, seen[culture]))
        elif not os.path.isfile(os.path.join(OUT, "%s.png" % culture)):
            failures.append("%s: no screenshot" % culture)

    for e in sorted(set(errors + errors2))[:20]:
        print("  log: " + e)
    path = sheet(expected)
    print("reports, logs, screenshots: %s%s" % (OUT, ("; sheet: %s" % path) if path else ""))
    if failures:
        for f in failures:
            print("FAIL " + f)
        print("packaged_languages: FAIL (%d)" % len(failures))
        return 1
    print("packaged_languages: PASS (%d languages: %s)" % (len(expected), ", ".join(expected)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
