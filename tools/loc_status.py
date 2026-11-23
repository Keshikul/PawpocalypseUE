"""Where each language stands (phase 7): per culture and string table, the lines translated, missing, and STALE.

    py tools/loc_status.py [--culture de] [--list] [--self-test]

Reads the English tables (Content/Pawpocalypse/Localization/Strings/ST_*.csv) and the translations
(Content/Pawpocalypse/Localization/Translations/<culture>/<Table>.csv: "Key","Text","Source"). A line is:
- translated: a row whose Source is today's English and whose {placeholders} are the English's;
- STALE: a row whose Source is not today's English (the English changed since it was translated: translate only it again;
  --list prints the old English beside the new, so a translator changes the part that changed);
- missing: no row (new English, or a line tools/port_po.py could not carry);
- not needed: English with no letters but a multiplier's x (x5, {n}/{max}): it reads the same in every language.
A row whose {placeholders} differ from the English counts as missing and is printed (a broken format string would show raw).

The cultures are the store's 23 languages (docs/WORKLIST-LOC-2026-11-18.md; Dutch and Danish added 2026-11-18).

    py tools/loc_status.py --todo [--culture de]          # build/loc/todo/<culture>.csv: the work file, one row per line to translate
    py tools/loc_status.py --apply build/loc/todo/de.csv  # its filled Translation column into the culture's files

--apply: every row with a Translation goes into Translations/<culture>/<Table>.csv, its Source stamped with the English
the translator SAW (the work file's English column), not today's: a line whose English changed after the work file was made
arrives stale and is listed again (KB Unreal/20 §3). All or nothing: nothing is written while a row is refused (no such
line, a {placeholder} lost or added, a key twice). The files are then written by tools/port_po.py's merge, so a later
port_po run keeps every row. --todo never overwrites a work file that holds a translation not yet applied.
--self-test: plants an English edit, a dropped placeholder and a missing row in memory, and checks each is reported; checks
--apply's refusals, port_po's merge order, the work file's guard, and an apply into a copy of the German files.
"""
import argparse
import csv
import re
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import port_po  # noqa: E402  (the files' one writer)

ROOT = Path(__file__).resolve().parent.parent
STRINGS = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings"
TRANS = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Translations"
TABLES = ("ST_UI", "ST_Data", "ST_Tree")
CULTURES = ("cs", "da", "de", "es", "es-419", "fr", "hu", "it", "ja", "ko", "nl", "pl", "pt-BR", "pt-PT", "ru", "sv", "th", "tr",
            "uk", "vi", "zh-Hans", "zh-Hant")
TOKEN = re.compile(r"\{[A-Za-z_][A-Za-z0-9_]*\}")
LETTER = re.compile(r"[^\W\d_]", re.UNICODE)


def read_csv(path: Path, key_col: str, cols):
    if not path.exists():
        return {}
    with open(path, encoding="utf-8", newline="") as f:
        return {r[key_col]: tuple(r[c] for c in cols) for r in csv.DictReader(f)}


def english():
    return {t: {k: v[0] for k, v in read_csv(STRINGS / (t + ".csv"), "Key", ("SourceString",)).items()} for t in TABLES}


def translations(culture, base=None):
    return {t: read_csv((base or TRANS) / culture / (t + ".csv"), "Key", ("Text", "Source")) for t in TABLES}


WORK_COLUMNS = ["Table", "Key", "Why", "English", "OldEnglish", "Comment", "Glossary", "Translation"]


def read_work(path: Path):
    with open(path, encoding="utf-8-sig", newline="") as f:
        reader = csv.DictReader(f)
        lost = [c for c in ("Table", "Key", "English", "Translation") if c not in (reader.fieldnames or [])]
        if lost:
            raise SystemExit("loc_status --apply: %s has no %s column" % (path, ", ".join(lost)))
        return list(reader)


def check_work(rows, eng):
    """A work file's filled rows -> ({table: {key: (text, source seen)}}, refused [(table, key, why)], stale [(table, key)]).
    Source is the English the translator saw: when today's differs, the row arrives stale (listed again, never shown)."""
    accepted, refused, stale, seen_keys = {}, [], [], set()
    for r in rows:
        text = r.get("Translation") or ""
        if not text.strip():
            continue
        table, key, seen = r.get("Table", ""), r.get("Key", ""), r.get("English", "")
        if table not in TABLES:
            refused.append((table, key, "no table %r" % table))
        elif key not in eng[table]:
            refused.append((table, key, "no such line in today's %s" % table))
        elif (table, key) in seen_keys:
            refused.append((table, key, "the key is in the work file twice"))
        elif sorted(TOKEN.findall(text)) != sorted(TOKEN.findall(seen)):
            refused.append((table, key, "placeholders %s, the English has %s" % (
                " ".join(sorted(TOKEN.findall(text))) or "none", " ".join(sorted(TOKEN.findall(seen))) or "none")))
        else:
            accepted.setdefault(table, {})[key] = (text, seen)
            if seen != eng[table][key]:
                stale.append((table, key))
        seen_keys.add((table, key))
    return accepted, refused, stale


def unapplied(rows, trans):
    """A work file's translations that the culture's files do not hold (the --todo guard)."""
    return [(r["Table"], r["Key"]) for r in rows if (r.get("Translation") or "").strip()
            and (trans.get(r.get("Table"), {}).get(r.get("Key")) or (None,))[0] != r["Translation"]]


def apply_rows(culture, accepted, godot: Path):
    """The accepted rows into the culture's files, through port_po's merge (every culture's files are written as a
    port_po run writes them). Returns the rows the merge did not keep as given."""
    files, report, tables = port_po.build(godot, overlay={culture: accepted})
    port_po.write(files)
    port_po.write_report(report, tables)
    held = translations(culture, port_po.OUT)
    return [(t, k) for t, rows in accepted.items() for k, row in rows.items() if held[t].get(k) != row]


def apply(path: Path, culture, godot: Path):
    culture = culture or path.stem
    if culture not in CULTURES:
        print("loc_status --apply: %r is not one of the cultures (%s); name it with --culture" % (culture, " ".join(CULTURES)))
        return 1
    eng = english()
    accepted, refused, stale = check_work(read_work(path), eng)
    for t, k, why in refused:
        print("  REFUSED %s %s: %s" % (t, k, why))
    if refused:
        print("loc_status --apply: %d rows refused; nothing written" % len(refused))
        return 1
    n = sum(len(rows) for rows in accepted.values())
    replaced = apply_rows(culture, accepted, godot)
    for t, k in stale:
        print("  STALE ON ARRIVAL %s %s: its English changed after the work file was made (stamped with the English seen)" % (t, k))
    for t, k in replaced:
        print("  NOT KEPT %s %s: stale on arrival, and Godot has a translation of today's English" % (t, k))
    s = status(eng, translations(culture))
    need = len(s["translated"]) + len(s["missing"]) + len(s["stale"]) + len(s["tokens"])
    print("loc_status --apply: %s: %d rows applied (%d stale on arrival); now %d / %d translated, %d missing, %d stale, %d broken"
          % (culture, n, len(stale), len(s["translated"]), need, len(s["missing"]), len(s["stale"]), len(s["tokens"])))
    return 0


def unescape(cell: str) -> str:
    """A string table cell as the engine reads it (FString::ReplaceEscapedCharWithChar: \\n, \\r, \\t, \\', \\", \\\\)."""
    return re.sub(r"\\(.)", lambda m: {"n": "\n", "r": "\r", "t": "\t"}.get(m.group(1), m.group(1)), cell)


def needs(eng: str) -> bool:
    # A multiplier (x5, x10) is a mark, not a word. Read as the engine reads it: the n of an escaped line break is no letter
    # (2026-11-20: "{rank}\n\n{offline}" was counted as words here and not by PawText::NeedsWords).
    bare = re.sub(r"\bx\d+\b", "", TOKEN.sub("", unescape(eng)))
    return bool(LETTER.search(bare))


def status(eng_tables, trans_tables):
    out = {"translated": [], "stale": [], "missing": [], "tokens": [], "not_needed": []}
    for t in TABLES:
        for key, eng in eng_tables[t].items():
            if not needs(eng):
                out["not_needed"].append((t, key))
                continue
            row = trans_tables[t].get(key)
            if row is None:
                out["missing"].append((t, key, eng))
            elif row[1] != eng:
                out["stale"].append((t, key, row[1], eng))
            elif sorted(TOKEN.findall(row[0])) != sorted(TOKEN.findall(eng)):
                out["tokens"].append((t, key, eng, row[0]))
            else:
                out["translated"].append((t, key))
    return out


def self_test():
    eng = {t: {} for t in TABLES}
    eng["ST_UI"] = {"A": "Buy {n} more", "B": "Close", "C": "x5", "D": "Save", "F": "{a}\\n\\n{b}"}
    tr = {t: {} for t in TABLES}
    tr["ST_UI"] = {"A": ("Kaufe {n} mehr", "Buy {n} more"), "B": ("Schliessen", "Shut"), "D": ("Speichern {n}", "Save")}
    s = status(eng, tr)
    ok = (s["translated"] == [("ST_UI", "A")] and [x[1] for x in s["stale"]] == ["B"] and s["not_needed"] == [("ST_UI", "C"), ("ST_UI", "F")]
          and [x[1] for x in s["tokens"]] == ["D"] and s["missing"] == [])
    eng["ST_UI"]["E"] = "New line"
    s = status(eng, tr)
    ok = ok and [x[1] for x in s["missing"]] == ["E"]
    print("loc_status self-test:", "PASS (a changed English line is stale, only it; a lost placeholder and a new line are reported)"
          if ok else "FAIL")
    return 0 if ok and self_test_apply() else 1


def self_test_apply():
    fails = []

    def expect(what, got, want):
        if got != want:
            fails.append("%s: got %r, want %r" % (what, got, want))

    # --apply's refusals, and the Source stamped with the English seen.
    eng = {t: {} for t in TABLES}
    eng["ST_UI"] = {"A": "Buy {n} more", "B": "Close", "C": "Save now"}
    rows = [
        {"Table": "ST_UI", "Key": "A", "English": "Buy {n} more", "Translation": "Kaufe {n} mehr"},
        {"Table": "ST_UI", "Key": "B", "English": "Shut", "Translation": "Zu"},          # English changed since
        {"Table": "ST_UI", "Key": "C", "English": "Save now", "Translation": ""},        # not translated yet
        {"Table": "ST_UI", "Key": "Z", "English": "Gone", "Translation": "Weg"},
        {"Table": "ST_Nope", "Key": "A", "English": "Buy {n} more", "Translation": "x"},
        {"Table": "ST_UI", "Key": "A", "English": "Buy {n} more", "Translation": "Kaufe {n} mehr"},
    ]
    accepted, refused, stale = check_work(rows, eng)
    expect("accepted", accepted, {"ST_UI": {"A": ("Kaufe {n} mehr", "Buy {n} more"), "B": ("Zu", "Shut")}})
    expect("stale on arrival", stale, [("ST_UI", "B")])
    expect("refused", [(t, k) for t, k, _ in refused], [("ST_UI", "Z"), ("ST_Nope", "A"), ("ST_UI", "A")])
    _, refused, _ = check_work([{"Table": "ST_UI", "Key": "A", "English": "Buy {n} more", "Translation": "Kaufe mehr"}], eng)
    expect("a lost placeholder", len(refused), 1)
    _, refused, _ = check_work([{"Table": "ST_UI", "Key": "B", "English": "Close", "Translation": "Zu {n}"}], eng)
    expect("an added placeholder", len(refused), 1)

    # port_po's merge: a row in use > Godot's > a stale row kept > none.
    expect("own kept over Godot", port_po.merge("Close", ("Zu", "Close"), "Schliessen"), (("Zu", "Close"), "own"))
    expect("Godot's when they agree", port_po.merge("Close", ("Zu", "Close"), "Zu"), (("Zu", "Close"), "godot"))
    expect("Godot over stale", port_po.merge("Close", ("Zu", "Shut"), "Schliessen"), (("Schliessen", "Close"), "godot"))
    expect("Godot over broken", port_po.merge("Buy {n}", ("Kauf", "Buy {n}"), "Kauf {n}"), (("Kauf {n}", "Buy {n}"), "godot"))
    expect("stale kept", port_po.merge("Close", ("Zu", "Shut"), None), (("Zu", "Shut"), "kept"))
    expect("missing", port_po.merge("Close", None, None), (None, "missing"))

    # The work file's guard: one holding a translation the files lack is never overwritten.
    trans = {t: {} for t in TABLES}
    trans["ST_UI"] = {"A": ("Kaufe {n} mehr", "Buy {n} more")}
    expect("applied", unapplied(rows[:1] + rows[2:3], trans), [])
    expect("not applied", unapplied(rows[:2], trans), [("ST_UI", "B")])

    # An apply into a copy of the German files: a new line, a line replaced, a line stale on arrival; then a plain
    # port_po run changes nothing (no row is ever lost to it). No Godot file is read (D22).
    godot = None
    if True:
        real_out, real_trans, real_report = port_po.OUT, globals()["TRANS"], port_po.REPORT
        tmp = Path(tempfile.mkdtemp(prefix="loc_apply_"))
        try:
            shutil.copytree(real_out / "de", tmp / "de")
            port_po.OUT, port_po.REPORT = tmp, tmp / "port_po.md"
            globals()["TRANS"] = tmp
            eng = english()
            # The copy without three of its lines (German is complete since 2026-11-20): they are missing.
            no_godot = {("ST_UI", k) for k in sorted(eng["ST_UI"])[:3]}
            for t in TABLES:
                path = tmp / "de" / (t + ".csv")
                keep = [(k, v[0], v[1]) for k, v in read_csv(path, "Key", ("Text", "Source")).items() if (t, k) not in no_godot]
                with open(path, "w", encoding="utf-8", newline="") as f:
                    w = csv.writer(f, quoting=csv.QUOTE_ALL, lineterminator="\n")
                    w.writerow(["Key", "Text", "Source"])
                    w.writerows(keep)
            s = status(eng, translations("de"))
            new_t, new_k, new_e = s["missing"][0]
            old_t, old_k = s["translated"][0]
            old_e = eng[old_t][old_k]
            stale_t, stale_k, stale_e = s["missing"][1]
            work = [
                {"Table": new_t, "Key": new_k, "English": new_e, "Translation": "NEU " + " ".join(TOKEN.findall(new_e))},
                {"Table": old_t, "Key": old_k, "English": old_e, "Translation": "EIGEN " + " ".join(TOKEN.findall(old_e))},
                {"Table": stale_t, "Key": stale_k, "English": stale_e + " (old)", "Translation": "ALT " + " ".join(TOKEN.findall(stale_e))},
            ]
            accepted, refused, stale = check_work(work, eng)
            expect("apply refused", refused, [])
            expect("apply not kept", apply_rows("de", accepted, godot), [])
            after = status(eng, translations("de"))
            expect("new line translated", (new_t, new_k) in after["translated"], True)
            expect("a translated line replaced", translations("de")[old_t][old_k][0].startswith("EIGEN"), True)
            expect("stale on arrival listed stale", [(t, k, o) for t, k, o, _ in after["stale"]], [(stale_t, stale_k, stale_e + " (old)")])
            expect("counts", (len(after["translated"]), len(after["missing"])), (len(s["translated"]) + 1, len(s["missing"]) - 2))
            files, _, _ = port_po.build(godot)
            changed = [p.name for p, b in files.items() if p.parent.name == "de" and p.read_bytes() != b]
            expect("a later port_po run changes nothing", changed, [])
        finally:
            port_po.OUT, globals()["TRANS"], port_po.REPORT = real_out, real_trans, real_report
            shutil.rmtree(tmp, ignore_errors=True)

    for f in fails:
        print("  FAIL", f)
    print("loc_status --apply self-test:", "PASS (refusals, the Source seen, the merge order, the work file's guard, an apply "
          "into a copy of the German files kept by a later port_po run)" if not fails else "FAIL")
    return not fails


def write_todo(eng, only=None):
    """build/loc/todo/<culture>.csv: every line to translate (missing, stale, broken placeholders), with the English, the
    table's comment for the translator, the old English of a stale line, and the store glossary's terms it contains (so the
    game and the store page say Catnip, Stardust, Reawaken and the stage names the same way)."""
    import json
    comments = {t: {k: v[0] for k, v in read_csv(STRINGS / (t + ".csv"), "Key", ("Comment",)).items()} for t in TABLES}
    glossary_path = ROOT / "build" / "steam" / "research" / "glossary.json"
    glossary = json.loads(glossary_path.read_text(encoding="utf-8")) if glossary_path.exists() else {}
    out_dir = ROOT / "build" / "loc" / "todo"
    out_dir.mkdir(parents=True, exist_ok=True)
    for culture in CULTURES:
        if only and culture != only:
            continue
        s = status(eng, translations(culture))
        rows = [(t, k, "stale", e, old) for t, k, old, e in s["stale"]]
        rows += [(t, k, "placeholders", e, "") for t, k, e, _ in s["tokens"]]
        rows += [(t, k, "missing", e, "") for t, k, e in s["missing"]]
        # The glossary is keyed by Godot's locale names (tools/port_po.py CULTURES); a term with no entry stays English.
        godot = {"es-419": "es_MX", "pt-BR": "pt_BR", "pt-PT": "pt", "zh-Hans": "zh_Hans", "zh-Hant": "zh_Hant"}.get(culture, culture)
        terms = {en: tr for en, tr in glossary.get(godot, {}).items() if tr} if isinstance(glossary, dict) else {}
        path = out_dir / (culture + ".csv")
        held = unapplied(read_work(path), translations(culture)) if path.exists() else []
        if held:
            print("%-8s kept %s: %d translations in it are not applied yet (--apply it first)" % (culture, path.relative_to(ROOT), len(held)))
            continue
        with open(path, "w", encoding="utf-8", newline="") as f:
            w = csv.writer(f, quoting=csv.QUOTE_ALL, lineterminator="\n")
            w.writerow(WORK_COLUMNS)
            for t, k, why, e, old in rows:
                hits = ["%s = %s" % (en, tr) for en, tr in (terms.items() if isinstance(terms, dict) else []) if en.lower() in e.lower()]
                w.writerow([t, k, why, e, old, comments[t].get(k, ""), "; ".join(hits), ""])
        print("%-8s %4d lines -> %s" % (culture, len(rows), path.relative_to(ROOT)))
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--culture")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--self-test", action="store_true")
    ap.add_argument("--todo", action="store_true", help="write build/loc/todo/<culture>.csv: what to translate, with context")
    ap.add_argument("--apply", metavar="WORK_FILE", help="a filled work file into the culture's files (culture: --culture or the file's name)")
    ap.add_argument("--godot", default=None, help="Godot's repo, to carry its .po translations too (history: D22, none needed)")
    a = ap.parse_args()
    if a.self_test:
        return self_test()
    if a.apply:
        return apply(Path(a.apply), a.culture, Path(a.godot) if a.godot else None)
    eng = english()
    if a.todo:
        return write_todo(eng, a.culture)
    print("%-8s %10s %7s %7s %6s" % ("culture", "translated", "missing", "stale", "broken"))
    for culture in CULTURES:
        if a.culture and culture != a.culture:
            continue
        s = status(eng, translations(culture))
        need = len(s["translated"]) + len(s["missing"]) + len(s["stale"]) + len(s["tokens"])
        print("%-8s %4d / %-4d %7d %7d %6d" % (culture, len(s["translated"]), need, len(s["missing"]), len(s["stale"]), len(s["tokens"])))
        if a.list:
            for t, key, old, new in s["stale"]:
                print("  STALE %s %s\n    was: %s\n    now: %s" % (t, key, old.replace("\n", " / "), new.replace("\n", " / ")))
            for t, key, eng_s, tr in s["tokens"]:
                print("  PLACEHOLDERS %s %s: %s -> %s" % (t, key, eng_s.replace("\n", " / "), tr.replace("\n", " / ")))
            for t, key, eng_s in s["missing"]:
                print("  MISSING %s %s: %s" % (t, key, eng_s.replace("\n", " / ")))
    return 0


if __name__ == "__main__":
    sys.exit(main())
