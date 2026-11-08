"""Makes the game's font faces and builds the composite font F_Paw (restyle R2, 2026-11-07; docs/07 §5).

    py tools/import_fonts.py --run        (headless; editor closed)

SourceArt/fonts/<file> -> /Game/Pawpocalypse/Fonts/FF_<name> (UFontFace; a .ttc is ONE asset, its faces picked by index),
each with its loading policy, then F_Paw rewritten whole: both by UPawFontBuilder (PawpocalypseEditor) from the generated
UI/PawFontSpec.h (run `py tools/fonts.py` first when the table changed). Not through the font import factory: it runs
UFontFace::PostEditChangeProperty, which flushes Slate's font cache without checking Slate exists and asserts headless
(UE 5.8.3; KB U/09 §5). A face whose file is unchanged is kept. Test: Pawpocalypse.UI.Fonts.Composite; gate:
tools/check_font_coverage.py.
"""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EDITOR_CMD = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
FACES = ("FF_MPlusRounded1c_ExtraBold", "FF_MPlusRounded1c_Black", "FF_NotoSansCJK_Medium", "FF_NotoSansThai_Medium")


def inside_editor():
    import unreal
    log = unreal.log_warning
    failed = False
    for line in unreal.PawFontBuilder.build_faces().splitlines():
        log("IMPORT face " + line)
        failed |= "FAILED" in line
    if not failed:
        result = unreal.PawFontBuilder.build_font()
        log("IMPORT " + result)
        failed |= result.startswith("FAILED")
    for p in unreal.EditorAssetLibrary.list_assets("/Game/Pawpocalypse/Fonts", recursive=False):
        if p.split(".")[-1] not in FACES + ("F_Paw",):
            log("IMPORT stray (not in tools/fonts.py): " + p)
    log("IMPORT FAILED" if failed else "IMPORT DONE")


def from_shell():
    sys.path.insert(0, str(ROOT / "tools"))
    import fonts
    import unreal_procs
    if set(fonts.FACES) != set(FACES):
        print("import_fonts: FACES here differs from tools/fonts.py")
        return 1
    if unreal_procs.blocking():
        print("import_fonts: an Unreal process of this project is running; close it first")
        return 2
    script = (ROOT / "tools" / "import_fonts.py").as_posix()
    cmd = [EDITOR_CMD, str(ROOT / "Pawpocalypse.uproject"), "-run=pythonscript", "-script=%s" % script,
           "-nullrhi", "-unattended", "-nop4", "-SCCProvider=None", "-stdout"]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    lines = [l[l.index("IMPORT "):] for l in r.stdout.splitlines() if "IMPORT " in l and "LogInit" not in l]
    for l in dict.fromkeys(lines):
        print(l)
    if "IMPORT DONE" not in lines:
        log = ROOT / "build" / "import_fonts.log"
        log.parent.mkdir(exist_ok=True)
        log.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
        print("import_fonts: FAILED (log: %s)" % log)
        return 1
    return 0


if __name__ == "__main__":
    if "--run" in sys.argv[1:]:
        sys.exit(from_shell())
    inside_editor()
