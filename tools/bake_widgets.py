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
LIST an asset's widget names (to find a part baked without a name): "PawHudScreen=names:".

RECOLOUR every baked asset after a palette change (the restyle; UPawWidgetBaker::Recolour): `py tools/bake_widgets.py --run
--recolour`. The assets saved their colours as values when baked; every colour that is EXACTLY one the palette changed since
tools/art/palette_widgets_snapshot.json (the palette they carry) becomes today's, alpha kept; anything the designer set by hand
stays. On success the snapshot is rewritten to today's palette. Pawpocalypse.UI.Widgets.Palette fails while one is missed.

REFONT every baked asset to the game's font (restyle R2; UPawWidgetBaker::Refont): `py tools/bake_widgets.py --run --refont`,
after `py tools/import_fonts.py --run`. Every font a baked asset saved that is an engine font (Roboto) becomes F_Paw, its
typeface name, size and outline kept. Pawpocalypse.UI.Widgets.Fonts fails while one is missed.

TEXT FLOOR (restyle R4b; UPawWidgetBaker::TextFloor): `py tools/bake_widgets.py --run --textfloor`. Every font size a baked
asset saved under 12 pt (16 px at 1080p) and every piece's TextPx / LabelPx under 16 rise to the floor (a keycap's label
exempt: PawUi::UnderTextFloor). Pawpocalypse.UI.Widgets.TextFloor fails while one is under it.
"""

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EDITOR_CMD = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"


# Assets whose text sits on a light surface and keeps its ink through a recolour (a card's Bone body).
KEEP_INK = ("WBP_CardFace",)


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
        report += "\n" + unreal.PawWidgetBaker.recolour_by_role(os.environ.get("PAW_RECOLOUR_TEXT", ""),
                                                                os.environ.get("PAW_RECOLOUR_SURFACE", ""),
                                                                os.environ.get("PAW_RECOLOUR_EDGE", ""), ";".join(KEEP_INK))
    if os.environ.get("PAW_REFONT"):
        report += "\n" + unreal.PawWidgetBaker.refont()
    if os.environ.get("PAW_TEXTFLOOR"):
        report += "\n" + unreal.PawWidgetBaker.text_floor()
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
    if unreal_procs.blocking():
        print("bake_widgets: an Unreal process of this project is running; close it first")
        return 2
    script = (ROOT / "tools" / "bake_widgets.py").as_posix()
    env = dict(os.environ, PAW_GRAFTS="|".join(grafts(argv)))
    snapshot_update = None
    if "--recolour" in argv:
        import json
        import gen_paw_tokens
        # By ROLE (R3): the UI table's text colours and surfaces apart (one old ink, two new colours); every other changed
        # palette colour (the art's, never the UI's) in both.
        text, surface, edge = gen_paw_tokens.ui_role_pairs(gen_paw_tokens.WIDGETS_SNAPSHOT)
        for old, new, _ in gen_paw_tokens.colour_pairs(gen_paw_tokens.WIDGETS_SNAPSHOT, ui=False):
            text.setdefault(old, new)
            surface.setdefault(old, new)
            edge.setdefault(old, new)
        if not text and not surface and not edge:
            print("bake_widgets: the widgets already carry today's palette; nothing to recolour")
        else:
            env["PAW_RECOLOUR_TEXT"] = ";".join("%s>%s" % (o[1:], n[1:]) for o, n in sorted(text.items()))
            env["PAW_RECOLOUR_SURFACE"] = ";".join("%s>%s" % (o[1:], n[1:]) for o, n in sorted(surface.items()))
            env["PAW_RECOLOUR_EDGE"] = ";".join("%s>%s" % (o[1:], n[1:]) for o, n in sorted(edge.items()))
            env["PAW_RECOLOUR"] = "1"
            print("bake_widgets: recolour text %s" % ", ".join("%s->%s" % kv for kv in sorted(text.items())))
            print("bake_widgets: recolour surfaces %s" % ", ".join("%s->%s" % kv for kv in sorted(surface.items())))
            print("bake_widgets: recolour edges %s" % ", ".join("%s->%s" % kv for kv in sorted(edge.items())))
            snapshot_update = json.dumps(gen_paw_tokens.palette.as_dict(), indent=2) + "\n"
    if "--refont" in argv:
        env["PAW_REFONT"] = "1"
    if "--textfloor" in argv:
        env["PAW_TEXTFLOOR"] = "1"
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
        return 1
    if snapshot_update is not None:
        gen_paw_tokens.WIDGETS_SNAPSHOT.write_text(snapshot_update, encoding="utf-8", newline="\n")
        print("bake_widgets: the widgets now carry today's palette (%s)" % gen_paw_tokens.WIDGETS_SNAPSHOT.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    if "--run" in sys.argv:
        sys.exit(from_shell(sys.argv[1:]))
    inside_editor()
