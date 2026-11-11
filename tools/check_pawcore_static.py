"""L0 static gates for PawCore (docs/09 §1, CLAUDE.md hard rules). Seconds, no build.

  no-float      `double` for every economy/damage value: the word `float` does not appear in PawCore code (comments
                and string literals are stripped first, so "float64" in a comment is fine; doctest is third-party). ONE
                exception, FLOAT_IS_THE_SPEC: PcBackdrop, which reproduces Godot's float32 Vector2 / RandomNumberGenerator
                (real_t) bit for bit; no economy or damage value lives there.
  engine-free   PawCore includes only the C++ standard library, its own Pc*.h headers and the vendored doctest; the one
                exception is Private/PawCoreModule.cpp, the Unreal module glue.
  test-guard    every .cpp under Tests/ is wrapped in `#if defined(PAWCORE_STANDALONE_TESTS)` or `PAWCORE_SIM_CLI`
                (UBT compiles every .cpp in the module folder; an unguarded test would land in the game).
  generated     PcEnums.h and PcDataRows.h are exactly what tools/gen_pc_enums.py / gen_pc_data.py write (`--check`), the
                game's Data/PawDataRows.h what tools/gen_paw_rows.py writes, UI/PawTokens.h what tools/gen_paw_tokens.py
                writes (the Godot palette), ST_Data.csv what tools/gen_data_strings.py writes (the data names) and
                ST_Tree.csv / PcTreeWording.h / PawTreeTables.h what tools/gen_tree_tables.py writes (skill_tree_ui.gd),
                Realm/PawFxLook.h what tools/gen_fx_look.py writes (the field effects' look, restyle D3).
  ui-rules      the game's UI code (Source/Pawpocalypse/UI, tests excepted) holds no colour value but the generated
                PawTokens.h (no FLinearColor(/FColor( literal, no FromHex/FromSRGBColor), no hard-coded player text
                (FText::FromString, LOCTEXT, NSLOCTEXT; AsCultureInvariant only in PawText.cpp, for numbers); every string-
                table key it names exists in ST_UI.csv, and every ST_UI key is named somewhere (docs/05 §7, docs/08 §4).
  data-export   Data/*.csv is exactly what tools/export_godot_data.py exports from the Godot project today (until
                parity Godot is the spec: a difference means someone edited a CSV, or Godot changed and nobody re-exported).
                Covers Data/CodeConstants.csv too: the export fails when a Godot code constant has no row in the reviewed
                Data/CodeConstantsMap.csv, or a mapped one is gone (tools/export_code_constants.py).

`--self-test` plants one violation per gate in a scratch copy and requires each gate to catch it.
Usage: py tools/check_pawcore_static.py [--self-test] [--skip-export]
"""

import argparse
import filecmp
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CORE = ROOT / "Source" / "PawCore"
UI = ROOT / "Source" / "Pawpocalypse" / "UI"
ST_UI = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings" / "ST_UI.csv"
UI_KEY = re.compile(r'TEXT\("((?:TAB|PAGE|SHOP|ZONE|WAVE|BOSS|REALMS|BAIT|REAWAKEN|CATCHES|AWARDS|JOBS|PICK|TREE|PROMPT|CEREMONY|JOBPICK|VICTORY|DURATION|TITLE|PAUSE|SETTINGS|CONTROLS|INPUT|KEY|HUD|QUICK)_[a-z0-9_]+)"\)')
# A colour built from numbers: FLinearColor(1, 0, 0), FColor Name{255, ...}, FromHex(...); not a function returning one.
UI_COLOUR = re.compile(r"\bF(?:Linear)?Color\s*(?:\w+\s*)?[({]\s*[-+]?[\d.]|FromHex|FromSRGBColor")
UI_TEXT = re.compile(r"FText::FromString\s*\(|\bLOCTEXT\s*\(|\bNSLOCTEXT\s*\(")
GODOT = "C:/Games/pawpocalypse"
STD_HEADERS = {
    "algorithm", "array", "cassert", "cctype", "cerrno", "charconv", "chrono", "cinttypes", "climits", "cmath", "cstddef",
    "cstdint", "cstdio", "cstdlib", "cstring", "fstream", "functional", "initializer_list", "iomanip", "ios", "iostream",
    "iterator", "limits", "map", "memory", "numeric", "optional", "random", "set", "sstream", "string", "string_view",
    "system_error", "tuple", "type_traits", "unordered_map", "unordered_set", "utility", "vector",
}


def strip_comments_and_strings(text):
    out, i, n = [], 0, len(text)
    while i < n:
        if text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
        elif text[i] in "\"'":
            q = text[i]
            j = i + 1
            while j < n and text[j] != q:
                j += 2 if text[j] == "\\" else 1
            out.append(q + q)
            i = j + 1
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def sources(core):
    return [p for p in core.rglob("*") if p.suffix in (".h", ".cpp") and "ThirdParty" not in p.parts]


# Files whose float IS the specification: Godot's real_t (Vector2, Rect2, randf) reproduced exactly (phase 6 slice 2,
# fixture `backdrop`: the same layout in double drifts off Godot's, sabotage backdrop_blob_in_double). Nothing else, ever.
FLOAT_IS_THE_SPEC = {"Public/PcBackdrop.h", "Private/PcBackdrop.cpp"}


def check_no_float(core):
    bad = []
    for p in sources(core):
        if p.relative_to(core).as_posix() in FLOAT_IS_THE_SPEC:
            continue
        code = strip_comments_and_strings(p.read_text(encoding="utf-8"))
        for no, line in enumerate(code.splitlines(), 1):
            if re.search(r"\bfloat\b", line):
                bad.append("%s:%d uses float" % (p.relative_to(core), no))
    return bad


def check_engine_free(core):
    bad = []
    for p in sources(core):
        if p.name == "PawCoreModule.cpp":
            continue
        for no, line in enumerate(p.read_text(encoding="utf-8").splitlines(), 1):
            m = re.match(r'\s*#\s*include\s*([<"])([^>"]+)[>"]', line)
            if not m:
                continue
            name = m.group(2)
            ok = (m.group(1) == "<" and name in STD_HEADERS) or \
                 (m.group(1) == '"' and (re.fullmatch(r"Pc\w+\.h", name) or name == "ThirdParty/doctest/doctest.h"))
            if not ok:
                bad.append("%s:%d includes %s%s" % (p.relative_to(core), no, m.group(1), name))
    return bad


def check_test_guard(core):
    bad = []
    for p in (core / "Tests").glob("*.cpp"):
        first = next((l for l in p.read_text(encoding="utf-8").splitlines() if l.strip() and not l.lstrip().startswith("//")), "")
        if not re.fullmatch(r"#if defined\((PAWCORE_STANDALONE_TESTS|PAWCORE_SIM_CLI)\)", first.strip()):
            bad.append("%s does not start with the standalone guard" % p.relative_to(core))
    return bad


def check_generated():
    bad = []
    for script in ("gen_pc_enums.py", "gen_pc_data.py"):
        r = subprocess.run([sys.executable, str(ROOT / "tools" / script), "--godot", GODOT, "--check"], capture_output=True, text=True)
        if r.returncode != 0:
            bad.append("%s --check: %s" % (script, (r.stdout + r.stderr).strip()[-300:]))
    # The game's DataTable row structs, from the Data/*.csv headers (tools/gen_paw_rows.py).
    for script, args in (("gen_paw_rows.py", ["--check"]), ("gen_paw_tokens.py", ["--check"]),
                         ("gen_data_strings.py", ["--check"]), ("gen_tree_tables.py", ["--godot", GODOT, "--check"]),
                         ("gen_input_defaults.py", ["--check"]), ("fonts.py", ["--check"]),
                         ("gen_fx_look.py", ["--check"])):
        r = subprocess.run([sys.executable, str(ROOT / "tools" / script)] + args, capture_output=True, text=True)
        if r.returncode != 0:
            bad.append("%s --check: %s" % (script, (r.stdout + r.stderr).strip()[-300:]))
    return bad


def check_ui(ui_dir, st_ui):
    import csv
    bad = []
    with open(st_ui, encoding="utf-8", newline="") as f:
        table = {row["Key"] for row in csv.DictReader(f)}
    named = set()
    for path in sorted(list(ui_dir.glob("*.cpp")) + list(ui_dir.glob("*.h"))):
        raw = path.read_text(encoding="utf-8")
        named |= set(UI_KEY.findall(raw))
        if path.name in ("PawTokens.h", "PawTreeTables.h") or path.name.endswith("Tests.cpp"):
            continue
        code = strip_comments_and_strings(raw)
        for n, line in enumerate(code.splitlines(), 1):
            if UI_COLOUR.search(line):
                bad.append("%s:%d holds a colour value (use PawTokens)" % (path.name, n))
            if UI_TEXT.search(line):
                bad.append("%s:%d hard-codes player text (use PawText)" % (path.name, n))
            if "AsCultureInvariant" in line and path.name != "PawText.cpp":
                bad.append("%s:%d culture-invariant text outside PawText.cpp" % (path.name, n))
            # Godot prints numbers with str() / "%d" / "%.Nf": never grouped, never localised (PawText Count, Fixed).
            if re.search(r"FText::As(?:Number|Percent|Currency)\s*\(", line):
                bad.append("%s:%d formats a number the engine's way (Godot's is PawText::Count / Fixed)" % (path.name, n))
            # The restyle's font (R2): text is F_Paw (PawUi::PawFont, UMG's default); never the engine's Roboto.
            if "GetDefaultFontStyle" in line:
                bad.append("%s:%d uses the engine's default font (use PawUi::PawFont)" % (path.name, n))
        for n, line in enumerate(raw.splitlines(), 1):
            if "/Engine/EngineFonts" in line and not line.lstrip().startswith("//"):
                bad.append("%s:%d names an engine font (use PawUi::PawFont)" % (path.name, n))
    # The string tables load loose at run time: one saved with CRLF doubles every line break inside a quoted value
    # (measured 2026-11-06: a cell's CR LF reads as two breaks). LF only.
    for table_csv in sorted(st_ui.parent.glob("ST_*.csv")):
        if b"\r" in table_csv.read_bytes():
            bad.append("%s has CR line endings (LF only: a CRLF string table doubles its line breaks)" % table_csv.name)
    for key in sorted(named - table):
        bad.append("ST_UI has no key %s" % key)
    for key in sorted(table - named):
        bad.append("ST_UI key %s is named by no UI source" % key)
    return bad


def check_data_export(data_dir):
    out = ROOT / "build" / "check_export"
    if out.exists():
        shutil.rmtree(out)
    r = subprocess.run([sys.executable, str(ROOT / "tools" / "export_godot_data.py"), "--godot", GODOT, "--out", str(out),
                        "--code-map", str(data_dir / "CodeConstantsMap.csv")], capture_output=True, text=True)
    if r.returncode != 0:
        return ["export_godot_data.py failed: " + (r.stdout + r.stderr).strip()[-300:]]
    bad = []
    # CodeConstantsMap.csv is the reviewed SOURCE of the CodeConstants export (tools/export_code_constants.py), not output;
    # TextOverrides.csv is the port's own English (tools/text_overrides.py), not exported either.
    names = sorted(({p.name for p in out.glob("*.csv")} | {p.name for p in data_dir.glob("*.csv")})
                   - {"CodeConstantsMap.csv", "TextOverrides.csv"})
    for name in names:
        a, b = out / name, data_dir / name
        if not (a.exists() and b.exists() and filecmp.cmp(a, b, shallow=False)):
            bad.append("Data/%s differs from a fresh export of the Godot project" % name)
    return bad


def run_all(core, data_dir, skip_export):
    gates = [("no-float", lambda: check_no_float(core)), ("engine-free", lambda: check_engine_free(core)),
             ("test-guard", lambda: check_test_guard(core)), ("generated", check_generated),
             ("ui-rules", lambda: check_ui(UI, ST_UI))]
    if not skip_export:
        gates.append(("data-export", lambda: check_data_export(data_dir)))
    results = {}
    for name, fn in gates:
        results[name] = fn()
    return results


def self_test():
    """One planted violation per gate, each in a scratch copy (or, for the generated headers, a restored edit)."""
    scratch = ROOT / "build" / "static_selftest"
    caught = {}

    def fresh():
        if scratch.exists():
            shutil.rmtree(scratch)
        shutil.copytree(CORE, scratch / "PawCore")
        return scratch / "PawCore"

    core = fresh()
    assert not check_no_float(core) and not check_engine_free(core) and not check_test_guard(core), "the clean copy fails"
    with open(core / "Private" / "PcStats.cpp", "a", encoding="utf-8") as f:
        f.write("\nnamespace Pc { static float Planted = 0.0f; }\n")
    caught["no-float"] = bool(check_no_float(core))
    core = fresh()
    with open(core / "Private" / "PcStats.cpp", "a", encoding="utf-8") as f:
        f.write('\n#include "CoreMinimal.h"\n')
    caught["engine-free"] = bool(check_engine_free(core))
    core = fresh()
    t = core / "Tests" / "PcStatsTests.cpp"
    t.write_text("static int Planted = 0; // code before the guard\n" + t.read_text(encoding="utf-8"), encoding="utf-8")
    caught["test-guard"] = bool(check_test_guard(core))
    shutil.rmtree(scratch, ignore_errors=True)

    header = CORE / "Public" / "PcEnums.h"
    original = header.read_bytes()
    try:
        header.write_bytes(original + b"\n// planted\n")
        caught["generated"] = bool(check_generated())
    finally:
        header.write_bytes(original)
    rows = ROOT / "Source" / "Pawpocalypse" / "Data" / "PawDataRows.h"
    original = rows.read_bytes()
    try:
        rows.write_bytes(original + b"\n// planted\n")
        caught["generated: PawDataRows.h"] = bool(check_generated())
    finally:
        rows.write_bytes(original)

    tokens = UI / "PawTokens.h"
    original = tokens.read_bytes()
    try:
        tokens.write_bytes(original + b"\n// planted\n")
        caught["generated: PawTokens.h"] = bool(check_generated())
    finally:
        tokens.write_bytes(original)

    for planted in (UI / "PawTreeTables.h", UI / "PawFontSpec.h", CORE / "Public" / "PcTreeWording.h",
                    ROOT / "Source" / "Pawpocalypse" / "Realm" / "PawFxLook.h",
                    ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings" / "ST_Tree.csv"):
        original = planted.read_bytes()
        try:
            planted.write_bytes(original + b"\n// planted\n")
            caught["generated: " + planted.name] = bool(check_generated())
        finally:
            planted.write_bytes(original)

    # The UI rules, each planted in a scratch copy of the UI folder and the table.
    ui_copy = ROOT / "build" / "static_selftest_ui"
    def fresh_ui():
        if ui_copy.exists():
            shutil.rmtree(ui_copy)
        shutil.copytree(UI, ui_copy / "UI")
        shutil.copy(ST_UI, ui_copy / "ST_UI.csv")
        return ui_copy / "UI", ui_copy / "ST_UI.csv"
    u, s = fresh_ui()
    assert not check_ui(u, s), "the clean UI copy fails"
    with open(u / "PawUiKit.cpp", "a", encoding="utf-8") as f:
        f.write("\nstatic const FLinearColor Planted(1.0f, 0.0f, 0.0f);\n")
    caught["ui-rules: colour literal"] = bool(check_ui(u, s))
    u, s = fresh_ui()
    with open(u / "PawUiKit.cpp", "a", encoding="utf-8") as f:
        f.write('\nstatic FText Planted() { return FText::FromString(TEXT("Buy")); }\n')
    caught["ui-rules: hard-coded text"] = bool(check_ui(u, s))
    u, s = fresh_ui()
    with open(u / "PawUiKit.cpp", "a", encoding="utf-8") as f:
        f.write('\nstatic FText Planted(int64 N) { return FText::AsNumber(N); }\n')
    caught["ui-rules: engine number format"] = bool(check_ui(u, s))
    u, s = fresh_ui()
    with open(u / "PawUiKit.cpp", "a", encoding="utf-8") as f:
        f.write('\nstatic FSlateFontInfo Planted() { return FCoreStyle::GetDefaultFontStyle("Bold", 10); }\n')
    caught["ui-rules: engine default font"] = bool(check_ui(u, s))
    u, s = fresh_ui()
    with open(u / "PawUiKit.cpp", "a", encoding="utf-8") as f:
        f.write('\nstatic const TCHAR* Planted = TEXT("/Engine/EngineFonts/Roboto");\n')
    caught["ui-rules: engine font path"] = bool(check_ui(u, s))
    u, s = fresh_ui()
    with open(u / "PawUiKit.cpp", "a", encoding="utf-8") as f:
        f.write('\nstatic const TCHAR* Planted = TEXT("SHOP_no_such_line");\n')
    caught["ui-rules: missing key"] = bool(check_ui(u, s))
    u, s = fresh_ui()
    with open(s, "a", encoding="utf-8") as f:
        f.write('"ZONE_never_named","Nobody says this",""\n')
    caught["ui-rules: unused key"] = bool(check_ui(u, s))
    shutil.rmtree(ui_copy, ignore_errors=True)

    data_copy = ROOT / "build" / "static_selftest_data"
    if data_copy.exists():
        shutil.rmtree(data_copy)
    shutil.copytree(ROOT / "Data", data_copy)
    p = data_copy / "UpgradeData.csv"
    p.write_text(p.read_text(encoding="utf-8").replace("2.4364", "2.4365", 1), encoding="utf-8")
    caught["data-export"] = bool(check_data_export(data_copy))
    shutil.rmtree(data_copy, ignore_errors=True)
    # A Godot code constant edited in the CSV, and one dropped from the reviewed map, are each a failed export.
    shutil.copytree(ROOT / "Data", data_copy)
    p = data_copy / "CodeConstants.csv"
    p.write_bytes(p.read_bytes().replace(b"max_wave,64,", b"max_wave,65,", 1))
    caught["data-export: code constant edited"] = bool(check_data_export(data_copy))
    shutil.rmtree(data_copy, ignore_errors=True)
    shutil.copytree(ROOT / "Data", data_copy)
    p = data_copy / "CodeConstantsMap.csv"
    p.write_bytes(b"".join(l for l in p.read_bytes().splitlines(True) if b":LEVY_EVERY," not in l))
    caught["data-export: constant unmapped"] = bool(check_data_export(data_copy))
    shutil.rmtree(data_copy, ignore_errors=True)

    clean = run_all(CORE, ROOT / "Data", False)
    for name, ok in caught.items():
        print("%s  %s" % ("CAUGHT  " if ok else "MISSED  ", name))
    clean_ok = not any(clean.values())
    print("%s  the real sources" % ("PASS    " if clean_ok else "FAIL    "))
    return 0 if all(caught.values()) and clean_ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--skip-export", action="store_true", help="skip the Godot re-export comparison")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    results = run_all(CORE, ROOT / "Data", args.skip_export)
    failed = 0
    for name, bad in results.items():
        print("%s  %s" % ("PASS" if not bad else "FAIL", name))
        for b in bad[:20]:
            print("      " + b)
        failed += bool(bad)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
