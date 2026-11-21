"""Bake the UI's Widget Blueprints (and the button styles' Blueprints) that do not exist yet.

Runs INSIDE the editor (headless), editor closed:
    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=C:/abs/path/tools/bake_widgets.py -nullrhi -unattended -nop4
        -SCCProvider=None -stdout
or, from a shell: py tools/bake_widgets.py --run   (finds the editor, refuses while one of this project's editors runs).

Every class in PawUi::BakedClasses (Source/Pawpocalypse/UI/PawWidgets.cpp) without an asset under
/Game/Pawpocalypse/UI/Widgets gets one; an existing asset is NEVER baked over: it owns its layout (designer, 2026-11-04).
To rebuild one from code, delete its .uasset first. Prints one line per class; exits non-zero on any FAILED.

GRAFT new parts of a class's code layout into its existing asset (UPawWidgetBaker::GraftParts: everything else in the asset
stays as the designer left it):
    py tools/bake_widgets.py --run --graft "PawHudScreen=add:DockLegend;add:PouncePrompt" [--graft "PawMainMenu=swap:HintText>HintPrompts"]
RESTYLE a part (its brush, padding, bar style or size from the code layout; nothing else of it): "PawJobPicker=restyle:Panel".
WRAPAT a text's wrap width and auto-wrap from the code layout (2026-11-20, the language pass): "PawMainMenu=wrapat:CreditsText".
LIST an asset's widget names (to find a part baked without a name): "PawHudScreen=names:".

RECOLOUR every baked asset after a palette change: `py tools/bake_widgets.py --run --recolour`. The assets saved their colours
as values when baked. Since D18 (Sticker Box) they move by RULE (UPawWidgetBaker::RecolourByRule; the rules are
gen_paw_tokens.ui_rules: a kept ink, property rules, then a map per role), because plum folded roles into shared values. An
inventory is taken first (--colours) and a pre-flight refuses the run, before anything is saved, while a colour the palette
retired is placed by no rule. A colour that is not one the palette changed since tools/art/palette_widgets_snapshot.json (the
palette they carry) stays, alpha kept, so anything the designer set by hand stays. On success the snapshot is rewritten to
today's palette. Pawpocalypse.UI.Widgets.Palette fails while one is missed.

REFONT every baked asset to the game's font (restyle R2; UPawWidgetBaker::Refont): `py tools/bake_widgets.py --run --refont`,
after `py tools/import_fonts.py --run`. Every font a baked asset saved that is an engine font (Roboto) becomes F_Paw, its
typeface name, size and outline kept. Pawpocalypse.UI.Widgets.Fonts fails while one is missed.

TEXT FLOOR (restyle R4b; UPawWidgetBaker::TextFloor): `py tools/bake_widgets.py --run --textfloor`. Every font size a baked
asset saved under 12 pt (16 px at 1080p) and every piece's TextPx / LabelPx under 16 rise to the floor (a keycap's label
exempt: PawUi::UnderTextFloor). Pawpocalypse.UI.Widgets.TextFloor fails while one is under it.
COLOUR INVENTORY (the Sticker Box restyle S1; UPawWidgetBaker::ColourReport): `py tools/bake_widgets.py --run --colours`.
Every colour every baked asset saves, with its widget, property path and role, to build/widget_colours.tsv. Changes nothing.
"""

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EDITOR_CMD = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"


def grafts(argv):
    out = []
    for i, a in enumerate(argv):
        if a == "--graft" and i + 1 < len(argv):
            out.append(argv[i + 1])
        elif a.startswith("--graft="):
            out.append(a[len("--graft="):])
    return out


def inside_editor():
    import unreal
    # The grafts arrive in PAW_GRAFTS ('|'-separated): arguments after -script= do not survive Windows quoting.
    report = unreal.PawWidgetBaker.bake_widgets()
    if os.environ.get("PAW_RECOLOUR"):
        report += "\n" + unreal.PawWidgetBaker.recolour_by_rule(os.environ.get("PAW_RULES", ""))
    if os.environ.get("PAW_REFONT"):
        report += "\n" + unreal.PawWidgetBaker.refont()
    if os.environ.get("PAW_TEXTFLOOR"):
        report += "\n" + unreal.PawWidgetBaker.text_floor()
    if os.environ.get("PAW_COLOURS"):
        report += "\n" + unreal.PawWidgetBaker.colour_report()
    for g in [g for g in os.environ.get("PAW_GRAFTS", "").split("|") if g]:
        cls, _, ops = g.partition("=")
        report += "\n" + unreal.PawWidgetBaker.graft_parts(cls, ops)
    for line in report.splitlines():
        unreal.log_warning("BAKE " + line)
    if "FAILED" in report:
        unreal.log_error("BAKE FAILED")
    else:
        unreal.log_warning("BAKE DONE")


def from_shell(argv):
    sys.path.insert(0, str(ROOT / "tools"))
    import unreal_procs
    found = unreal_procs.wait_blocking()
    if found:
        print("bake_widgets: an Unreal process of this project is running; close it first")
        unreal_procs.print_blocking(found)
        return 2
    env = dict(os.environ, PAW_GRAFTS="|".join(grafts(argv)))
    snapshot_update = None
    if "--recolour" in argv:
        import json
        import gen_paw_tokens
        # D18 (Sticker Box): by RULE (gen_paw_tokens.ui_rules: the kept inks, the property rules, the role maps, the art's
        # changed colours), after a pre-flight against an inventory taken now: every saved colour placed or kept, and no
        # colour the palette retired left unplaced. An unplaced colour stops the run before anything is saved.
        print("bake_widgets: the inventory first")
        if not run_editor(dict(os.environ, PAW_GRAFTS="", PAW_COLOURS="1")):
            return 1
        rules, _ambiguous = gen_paw_tokens.ui_rules(gen_paw_tokens.WIDGETS_SNAPSHOT)
        placed, _kept, unplaced = gen_paw_tokens.preflight(rules, ROOT / "build" / "widget_colours.tsv",
                                                            gen_paw_tokens.WIDGETS_SNAPSHOT)
        if unplaced:
            print("bake_widgets: REFUSED, %d saved colours no rule places:" % len(unplaced))
            for row in unplaced:
                print("  %s %s %s %s #%s" % (row["asset"], row["object"], row["path"], row["role"], row["colour"]))
            return 1
        if not placed:
            print("bake_widgets: the widgets already carry today's palette; nothing to recolour")
        else:
            env["PAW_RULES"] = gen_paw_tokens.rules_arg(rules)
            env["PAW_RECOLOUR"] = "1"
            print("bake_widgets: recolour %d saved colours by %d rules (pre-flight: none unplaced)" % (len(placed), len(rules)))
            snapshot_update = json.dumps(gen_paw_tokens.palette.as_dict(), indent=2) + "\n"
    if "--refont" in argv:
        env["PAW_REFONT"] = "1"
    if "--textfloor" in argv:
        env["PAW_TEXTFLOOR"] = "1"
    if "--colours" in argv:
        env["PAW_COLOURS"] = "1"
    if not run_editor(env):
        return 1
    if snapshot_update is not None:
        gen_paw_tokens.WIDGETS_SNAPSHOT.write_text(snapshot_update, encoding="utf-8", newline="\n")
        print("bake_widgets: the widgets now carry today's palette (%s)" % gen_paw_tokens.WIDGETS_SNAPSHOT.relative_to(ROOT))
    return 0


def run_editor(env):
    """One headless editor run of this script with env; True when it printed BAKE DONE (its log in build/ when not)."""
    script = (ROOT / "tools" / "bake_widgets.py").as_posix()
    cmd = [EDITOR_CMD, str(ROOT / "Pawpocalypse.uproject"), "-run=pythonscript", "-script=" + script, "-nullrhi",
           "-unattended", "-nop4", "-SCCProvider=None", "-stdout", "-NoSave"]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace", env=env)
    lines = [l for l in r.stdout.splitlines() if "BAKE " in l]
    for l in lines:
        print(l[l.index("BAKE "):])
    if not any("BAKE DONE" in l for l in lines):
        log = ROOT / "build" / "bake_widgets.log"
        log.parent.mkdir(exist_ok=True)
        log.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
        print("bake_widgets: FAILED (log: %s)" % log)
        return False
    return True


if __name__ == "__main__":
    if "--run" in sys.argv:
        sys.exit(from_shell(sys.argv[1:]))
    inside_editor()
