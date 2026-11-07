"""Generate the Enhanced Input assets from Data/InputMap.csv (Godot's input map; docs/05 §3).

Runs INSIDE the editor (headless), editor closed:
    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=C:/abs/path/tools/create_input_assets.py -nullrhi -unattended -nop4
        -SCCProvider=None -stdout
or, from a shell: py tools/create_input_assets.py --run   (finds the editor, refuses while one of this project's editors runs).

Writes IA_<Action> per Godot action under /Game/Pawpocalypse/Input/Actions and IMC_Pawpocalypse (every event) under
/Game/Pawpocalypse/Input, through PawpocalypseEditor UPawInputAssetBuilder. Generated, never edited by hand (re-export the CSV
from Godot, then re-run this): Pawpocalypse.Input.Map checks the assets equal the CSV. Exits non-zero on any FAILED.
"""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EDITOR_CMD = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"


def inside_editor():
    import unreal
    report = unreal.PawInputAssetBuilder.build_input_assets(str((ROOT / "Data" / "InputMap.csv").resolve()))
    for line in report.splitlines():
        unreal.log_warning("INPUT " + line)
    if "FAILED" in report:
        unreal.log_error("INPUT FAILED")
    else:
        unreal.log_warning("INPUT DONE")


def from_shell():
    sys.path.insert(0, str(ROOT / "tools"))
    import unreal_procs
    if unreal_procs.blocking():
        print("create_input_assets: an Unreal process of this project is running; close it first")
        return 2
    script = (ROOT / "tools" / "create_input_assets.py").as_posix()
    cmd = [EDITOR_CMD, str(ROOT / "Pawpocalypse.uproject"), "-run=pythonscript", "-script=" + script, "-nullrhi",
           "-unattended", "-nop4", "-SCCProvider=None", "-stdout", "-NoSave"]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    lines = [l for l in r.stdout.splitlines() if "INPUT " in l]
    for l in lines:
        print(l[l.index("INPUT "):])
    if not any("INPUT DONE" in l for l in lines):
        log = ROOT / "build" / "create_input_assets.log"
        log.parent.mkdir(exist_ok=True)
        log.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
        print("create_input_assets: FAILED (log: %s)" % log)
        return 1
    return 0


if __name__ == "__main__":
    if "--run" in sys.argv:
        sys.exit(from_shell())
    inside_editor()
