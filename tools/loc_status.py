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
--self-test: plants an English edit, a dropped placeholder and a missing row in memory, and checks each is reported.
"""
import argparse
import csv
import re
import sys
from pathlib import Path

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


def translations(culture):
    return {t: read_csv(TRANS / culture / (t + ".csv"), "Key", ("Text", "Source")) for t in TABLES}


def needs(eng: str) -> bool:
    # A multiplier (x5, x10) is a mark, not a word.
    bare = re.sub(r"\bx\d+\b", "", TOKEN.sub("", eng))
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
    eng["ST_UI"] = {"A": "Buy {n} more", "B": "Close", "C": "x5", "D": "Save"}
    tr = {t: {} for t in TABLES}
    tr["ST_UI"] = {"A": ("Kaufe {n} mehr", "Buy {n} more"), "B": ("Schliessen", "Shut"), "D": ("Speichern {n}", "Save")}
    s = status(eng, tr)
    ok = (s["translated"] == [("ST_UI", "A")] and [x[1] for x in s["stale"]] == ["B"] and s["not_needed"] == [("ST_UI", "C")]
          and [x[1] for x in s["tokens"]] == ["D"] and s["missing"] == [])
    eng["ST_UI"]["E"] = "New line"
    s = status(eng, tr)
    ok = ok and [x[1] for x in s["missing"]] == ["E"]
    print("loc_status self-test:", "PASS (a changed English line is stale, only it; a lost placeholder and a new line are reported)"
          if ok else "FAIL")
    return 0 if ok else 1


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
        with open(path, "w", encoding="utf-8", newline="") as f:
            w = csv.writer(f, quoting=csv.QUOTE_ALL, lineterminator="\n")
            w.writerow(["Table", "Key", "Why", "English", "OldEnglish", "Comment", "Glossary", "Translation"])
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
    a = ap.parse_args()
    if a.self_test:
        return self_test()
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
