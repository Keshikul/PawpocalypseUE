"""The pad-only playthrough and the focus tour (docs/09 L4 `padplay`; Godot ci/check_padplay.gd): runs paw.PadPlay
(Source/Pawpocalypse/Dev/PawPadPlay.cpp) in the editor build with -game, in a real window (Slate's focus needs one), on a
FRESH sandboxed save folder (build/padplay/saves: never the designer's saves or settings), and reports its checks.

    py tools/padplay_tour.py            # needs a current editor build (UBT); refuses while this project's editor runs

Exit 0 = every check passed; 1 = a check failed or the run did not finish; 2 = an editor of this project is running.
"""

import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
UPROJECT = ROOT / "Pawpocalypse.uproject"
SAVES = ROOT / "build" / "padplay" / "saves"
LOG = ROOT / "Saved" / "Logs" / "PawPadPlay.log"


def main() -> int:
    sys.path.insert(0, str(ROOT / "tools"))
    import unreal_procs
    if unreal_procs.blocking():
        print("padplay_tour: an Unreal process of this project is running; close it first")
        return 2
    shutil.rmtree(SAVES, ignore_errors=True)
    SAVES.mkdir(parents=True)
    LOG.unlink(missing_ok=True)
    cmd = [EDITOR, str(UPROJECT), "-game", "-windowed", "-ResX=1600", "-ResY=900", "-unattended", "-nosplash", "-nosound",
           "-SCCProvider=None", "-PawSaveDir=%s" % SAVES, "-ExecCmds=t.MaxFPS 60,paw.PadPlay", "-log=PawPadPlay.log"]
    try:
        subprocess.run(cmd, timeout=900)
    except subprocess.TimeoutExpired:
        print("padplay_tour: FAIL - the run did not finish in 15 minutes")
        return 1
    if not LOG.exists():
        print("padplay_tour: FAIL - no log at %s" % LOG)
        return 1
    lines = [l for l in LOG.read_text(encoding="utf-8", errors="replace").splitlines() if "PADPLAY" in l]
    for l in lines:
        m = re.search(r"PADPLAY (.*)$", l)
        print(m.group(1) if m else l)
    result = [l for l in lines if "PADPLAY RESULT" in l]
    if not result:
        print("padplay_tour: FAIL - the run ended without a result (log: %s)" % LOG)
        return 1
    return 0 if "RESULT PASS" in result[-1] else 1


if __name__ == "__main__":
    sys.exit(main())
