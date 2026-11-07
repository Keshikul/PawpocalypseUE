"""Builds SourceArt/svg, the game's whole drawn art set, from the generators in tools/art (docs/07 §2).

    py tools/art/build_art.py                 # regenerate SourceArt/svg
    py tools/art/build_art.py --check         # regenerate into build/art/check; fail on any difference from SourceArt/svg

The generators began as the Godot repo's tools/ (the "sweet" grammar), copied unchanged but for their output paths (each
file's PORT note). Since 2026-11-07 they are this project's own: the Unreal look no longer follows Godot (designer; docs/
WORKLIST-RESTYLE-2026-11-07.md R0), so --check compares with the committed SourceArt/svg only, never with Godot's assets/art.

SourceArt/svg mirrors Godot's assets/art: <folder>/<stem>.svg, where folder is creatures, bosses, player, cards,
skills, jobs, props, ui (ui/input/<family> the glyphs), plus the generated PNGs: the 12 backdrop floors (backdrops/) and the
ground shadow (props/ground_shadow.png, make_ground_shadow.py's soft variant). Four UI drawings were made by hand in Godot
(app_icon, ui_menu_bug / discord / language): their sources are tools/art/hand/ui. The Boba photos are not drawn: phase 6's
Boba slice brings them.

Never hand-edit a file under SourceArt/svg: change its generator, then run this (docs/07 §2.1).
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
ART = ROOT / "tools" / "art"
DEST = ROOT / "SourceArt" / "svg"


def generate(out: Path) -> None:
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    env = dict(os.environ, PAW_ART_ROOT=out.as_posix())
    runs = [
        ["make_sweet_art.py", "--all", "--write", "--new"],
        ["sweet_ui.py", "--write"],
        ["sweet_biomes.py", "--write"],
        ["make_ground_shadow.py", "--out", str(out / "_shadow")],
    ]
    for args in runs:
        r = subprocess.run([sys.executable, str(ART / args[0])] + args[1:], cwd=str(ROOT), env=env, capture_output=True,
            text=True, encoding="utf-8", errors="replace")
        if r.returncode != 0:
            raise SystemExit("build_art: %s failed:\n%s%s" % (args[0], r.stdout[-2000:], r.stderr[-2000:]))
    shutil.move(str(out / "_shadow" / "ground_shadow_soft.png"), str(out / "props" / "ground_shadow.png"))
    shutil.rmtree(out / "_shadow")
    for f in (ART / "hand").rglob("*.svg"):
        rel = f.relative_to(ART / "hand")
        (out / rel.parent).mkdir(parents=True, exist_ok=True)
        shutil.copyfile(f, out / rel)


def tree(root: Path, skip=()) -> dict:
    out = {}
    for p in root.rglob("*"):
        if p.is_file() and p.suffix in (".svg", ".png"):
            rel = p.relative_to(root).as_posix()
            if not any(rel.startswith(s) for s in skip):
                out[rel] = p.read_bytes().replace(b"\r\n", b"\n")
    return out


def compare(name: str, want: dict, got: dict) -> list:
    bad = ["%s: missing %s" % (name, k) for k in sorted(set(want) - set(got))]
    bad += ["%s: extra %s" % (name, k) for k in sorted(set(got) - set(want))]
    bad += ["%s: differs %s" % (name, k) for k in sorted(set(want) & set(got)) if want[k] != got[k]]
    return bad


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()
    if not args.check:
        generate(DEST)
        print("build_art: %d files -> %s" % (len(tree(DEST)), DEST.relative_to(ROOT)))
        return 0
    work = ROOT / "build" / "art" / "check"
    generate(work)
    got = tree(work)
    bad = compare("SourceArt/svg", tree(DEST), got)
    # The floors must tile without a seam at any aspect (the realm repeats them across a 32:9 view): sweet_biomes.py's own
    # seam gate (each floor's join no louder than twice its own grain).
    r = subprocess.run([sys.executable, str(ART / "sweet_biomes.py"), "--seams"], cwd=str(ROOT), capture_output=True, text=True,
                       encoding="utf-8", errors="replace")
    print("  floor seams: %s" % (r.stdout.strip().splitlines()[-1] if r.stdout.strip() else "no output"))
    if r.returncode != 0:
        bad.append("a floor has a seam (sweet_biomes.py --seams)")
    for b in bad[:30]:
        print("  " + b)
    print("build_art --check: %d files, %s" % (len(got), "identical" if not bad else "%d DIFFERENCES" % len(bad)))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
