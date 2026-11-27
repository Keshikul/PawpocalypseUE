"""The game's Windows icon (the exe, its taskbar button and window): SourceArt/icon/Pawpocalypse.ico, every size Windows asks
for, from the key art's square icon (D26: the first try's key art; tools/store_art.py writes it as upload/shortcut_icon.png,
the same picture as Steam's client icon). Source/Pawpocalypse.Target.cs points UBT at the .ico (WindowsPlatform.
ApplicationIcon): without it a build wears the engine's Default.ico (playtest 2026-11-26, note 2).

Usage:  py tools/make_app_icon.py [--src PNG]   # writes SourceArt/icon/Pawpocalypse.ico (default source: build/steam/store_art/upload/shortcut_icon.png)
        py tools/make_app_icon.py --check       # the .ico is there and holds every size, the largest 256 px
        py tools/make_app_icon.py --self-test   # the check fails on an icon missing a size and on a missing file

The project folder Build/ is the same folder as the gitignored build/ on Windows (names are case-blind), so the engine's
own place for it (Build/Windows/Application.ico) would never be committed: hence SourceArt/icon and the target setting.
"""

import argparse
import io
import sys
import tempfile
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ICON = ROOT / "SourceArt" / "icon" / "Pawpocalypse.ico"
SOURCE = ROOT / "build" / "steam" / "store_art" / "upload" / "shortcut_icon.png"
SIZES = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]


def write(src, out):
    im = Image.open(src).convert("RGBA")
    if im.width != im.height or im.width < 256:
        raise SystemExit("%s: the icon's source must be square and at least 256 px (got %dx%d)" % (src, im.width, im.height))
    out.parent.mkdir(parents=True, exist_ok=True)
    im.resize((256, 256), Image.LANCZOS).save(out, format="ICO", sizes=[(s, s) for s in SIZES])


def problems(path):
    if not path.is_file():
        return ["%s is missing (run py tools/make_app_icon.py)" % path]
    try:
        with Image.open(path) as im:
            have = sorted({s[0] for s in im.info.get("sizes", set())})
    except Exception as e:  # noqa: BLE001 - any unreadable file is the finding
        return ["%s cannot be read as an icon: %s" % (path, e)]
    missing = [s for s in SIZES if s not in have]
    return ["%s lacks the size(s) %s (has %s)" % (path, missing, have)] if missing else []


def self_test():
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        src = tmp / "src.png"
        Image.new("RGB", (256, 256), (200, 120, 60)).save(src)
        good = tmp / "good.ico"
        write(src, good)
        if problems(good):
            failures.append("a complete icon was refused: %s" % problems(good))
        partial = tmp / "partial.ico"
        Image.open(src).convert("RGBA").save(partial, format="ICO", sizes=[(16, 16), (32, 32)])
        if not problems(partial):
            failures.append("an icon without 256 px passed")
        if not problems(tmp / "none.ico"):
            failures.append("a missing icon passed")
    for f in failures:
        print("SELF-TEST FAIL: " + f)
    print("make_app_icon self-test: %s" % ("PASS" if not failures else "FAIL"))
    return 1 if failures else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--src", type=Path, default=SOURCE)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if args.check:
        found = problems(ICON)
        for p in found:
            print("FAIL: " + p)
        print("app icon: %s" % ("FAIL" if found else "PASS (%s)" % ICON.relative_to(ROOT)))
        return 1 if found else 0
    if not args.src.is_file():
        raise SystemExit("%s is missing: run py tools/store_art.py first, or give --src" % args.src)
    write(args.src, ICON)
    found = problems(ICON)
    print("wrote %s (%d bytes)%s" % (ICON.relative_to(ROOT), ICON.stat().st_size, "" if not found else ": " + "; ".join(found)))
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
