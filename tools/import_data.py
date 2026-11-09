"""Import Data/*.csv into the DataTables under /Game/Pawpocalypse/Data, then prove every table holds its CSV (docs/06 §4, §9).

    py tools/import_data.py            # import every table, save, then the parity check (editor closed)
    py tools/import_data.py --check    # parity only, nothing written: the L3 gate "edited a CSV, forgot the import"
    py tools/import_data.py --self-test  # the parity check must catch one changed cell (a scratch copy of the CSVs)

Run with plain Python, it launches the headless editor with THIS file as the script (KB U/17: UnrealEditor-Cmd,
-run=pythonscript, absolute forward-slash path, -SCCProvider=None, editor closed; it refuses while any Unreal editor
runs, the designer's included) and turns the PAW_ lines of the log into an exit code. Inside the editor it does the work.

Tables are generated from CSV and never edited (CLAUDE.md), so the import refills them whole. Row structs are
Source/Pawpocalypse/Data/PawDataRows.h (tools/gen_paw_rows.py): one FString per column, so a table holds the CSV's TEXT
and PawCore parses it (FPawDataAdapter). Parity compares, per table: the row names with the CSV ids, in order (the deck
and the achievement roster are ordered by `_godot_file`, which travels as a column), and every cell's text.
"""
import csv
import io
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "Data")
NOT_TABLES = {"CodeConstantsMap.csv", "InputMap.csv", "AudioEvents.csv", "AudioMusic.csv", "TextOverrides.csv"}  # InputMap:
# Enhanced Input assets; Audio*: the audio bank (tools/import_audio.py); TextOverrides: the string tables' English
ASSET_DIR = "/Game/Pawpocalypse/Data"
EDITOR = r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
UPROJECT = os.path.join(ROOT, "Pawpocalypse.uproject")


def struct_name(csv_name):
    # tools/gen_paw_rows.py's naming, without the F (a USTRUCT's script path drops it).
    return "Paw%sRow" % (csv_name[:-4] if csv_name.endswith("Data") else csv_name)


def read_csv(path):
    with open(path, newline="", encoding="utf-8") as f:
        rows = list(csv.reader(f))
    return rows[0], rows[1:]


def table_csvs(data_dir):
    return [(fn[:-4], os.path.join(data_dir, fn)) for fn in sorted(os.listdir(data_dir))
            if fn.endswith(".csv") and fn not in NOT_TABLES]


# --------------------------------------------------------------------------------------------------------------------
# Inside the editor
# --------------------------------------------------------------------------------------------------------------------

def in_editor():
    import unreal

    mode = os.environ.get("PAW_IMPORT_MODE", "import")
    data_dir = os.environ.get("PAW_IMPORT_DATA", DATA)
    lib = unreal.DataTableFunctionLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    say = unreal.log_warning
    failed = False

    for name, path in table_csvs(data_dir):
        asset = "%s/DT_%s" % (ASSET_DIR, name)
        struct = unreal.load_object(None, "/Script/Pawpocalypse.%s" % struct_name(name))
        if struct is None:
            say("PAW_IMPORT FAIL %s: no row struct %s (run py tools/gen_paw_rows.py and build)" % (name, struct_name(name)))
            failed = True
            continue
        header, rows = read_csv(path)
        if mode == "import":
            table = unreal.EditorAssetLibrary.load_asset(asset) if unreal.EditorAssetLibrary.does_asset_exist(asset) else None
            if table is None:
                factory = unreal.DataTableFactory()
                factory.set_editor_property("struct", struct)
                table = tools.create_asset("DT_%s" % name, ASSET_DIR, unreal.DataTable, factory)
            # The importer matches the header to property names; `_godot_file` is the property `godot_file`.
            out = io.StringIO()
            w = csv.writer(out, lineterminator="\n")
            w.writerow(["godot_file" if c == "_godot_file" else c for c in header])
            w.writerows(rows)
            if table is None or not lib.fill_data_table_from_csv_string(table, out.getvalue(), struct):
                say("PAW_IMPORT FAIL %s: the DataTable import failed" % name)
                failed = True
                continue
            if not unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False):
                say("PAW_IMPORT FAIL %s: could not save %s" % (name, asset))
                failed = True
                continue
            say("PAW_IMPORT ok %s %d rows" % (name, len(rows)))

        # Parity: the asset as saved (load it fresh) against the CSV.
        table = unreal.EditorAssetLibrary.load_asset(asset) if unreal.EditorAssetLibrary.does_asset_exist(asset) else None
        if table is None:
            say("PAW_PARITY FAIL %s: %s does not exist (run py tools/import_data.py)" % (name, asset))
            failed = True
            continue
        problems = []
        names = [str(n) for n in lib.get_data_table_row_names(table)]
        ids = [r[0] for r in rows]
        # Row names are FNames: compare case-insensitively (KB U/05 §3), the ids themselves are lower case by rule.
        if [n.lower() for n in names] != ids:
            problems.append("row names %s... differ from the CSV ids %s..." % (names[:4], ids[:4]))
        else:
            for col, column in enumerate(header[1:], start=1):
                prop = "godot_file" if column == "_godot_file" else column
                cells = list(lib.get_data_table_column_as_string(table, prop))
                want = [r[col] for r in rows]
                if cells != want:
                    bad = next((i for i, (a, b) in enumerate(zip(cells, want)) if a != b), min(len(cells), len(want)))
                    got = cells[bad] if bad < len(cells) else "<missing>"
                    exp = want[bad] if bad < len(want) else "<missing>"
                    problems.append("column %s row %s: table %r, CSV %r" % (column, ids[bad] if bad < len(ids) else bad, got, exp))
        if problems:
            failed = True
            for p in problems[:5]:
                say("PAW_PARITY FAIL %s: %s" % (name, p))
        else:
            say("PAW_PARITY ok %s %d rows x %d columns" % (name, len(rows), len(header) - 1))

    say("PAW_DONE %s" % ("FAIL" if failed else "ok"))


# --------------------------------------------------------------------------------------------------------------------
# Outside: launch the headless editor
# --------------------------------------------------------------------------------------------------------------------

def editor_running():
    # Any Unreal process of this project, or one naming no project (tools/unreal_procs.py); another project's run is not ours.
    from unreal_procs import blocking
    return bool(blocking())


def run_editor(mode, data_dir):
    if editor_running():
        print("import_data: an Unreal editor is running; close it first (this never closes it for you)")
        return None
    env = dict(os.environ, PAW_IMPORT_MODE=mode, PAW_IMPORT_DATA=data_dir)
    script = os.path.abspath(__file__).replace("\\", "/")
    cmd = [EDITOR, UPROJECT, "-run=pythonscript", "-script=" + script, "-nullrhi", "-unattended", "-nop4",
           "-SCCProvider=None", "-stdout"]
    r = subprocess.run(cmd, capture_output=True, text=True, env=env, encoding="utf-8", errors="replace")
    log = os.path.join(ROOT, "build", "import_data_%s.log" % mode)
    os.makedirs(os.path.dirname(log), exist_ok=True)
    with open(log, "w", encoding="utf-8") as f:
        f.write(r.stdout + r.stderr)
    # -stdout echoes the log into stdout as well: keep each PAW_ line once, in order.
    lines = list(dict.fromkeys(l[l.index("PAW_"):] for l in (r.stdout + r.stderr).splitlines() if "PAW_" in l and "Warning" in l))
    # The importer's own complaints (a column with no property, a malformed row) are failures too.
    problems = [l.strip() for l in (r.stdout + r.stderr).splitlines() if "LogDataTable" in l and ("Warning" in l or "Error" in l)]
    return lines, problems, log


def verdict(result):
    if result is None:
        return 2
    lines, problems, log = result
    for l in lines:
        print(l)
    for p in problems:
        print("DataTable importer: " + p)
    ok = "PAW_DONE ok" in lines and not problems
    print("import_data: %s (log: %s)" % ("PASS" if ok else "FAIL", log))
    return 0 if ok else 1


def self_test():
    """The parity check must catch one changed cell: a scratch copy of Data with one HP nudged, checked (not imported)."""
    import shutil
    scratch = os.path.join(ROOT, "build", "import_data_selftest")
    shutil.rmtree(scratch, ignore_errors=True)
    shutil.copytree(DATA, scratch)
    p = os.path.join(scratch, "EnemyData.csv")
    text = open(p, encoding="utf-8", newline="").read()
    with open(p, "w", encoding="utf-8", newline="") as f:
        f.write(text.replace("143.364", "143.365", 1))
    result = run_editor("check", scratch)
    if result is None:
        return 2
    caught = any(l.startswith("PAW_PARITY FAIL EnemyData") for l in result[0])
    print("%s  a changed EnemyData cell" % ("CAUGHT  " if caught else "MISSED  "))
    return 0 if caught else 1


def main():
    if "--self-test" in sys.argv:
        return self_test()
    return verdict(run_editor("check" if "--check" in sys.argv else "import", DATA))


try:
    import unreal  # noqa: F401
    in_editor()
except ImportError:
    if __name__ == "__main__":
        sys.exit(main())
