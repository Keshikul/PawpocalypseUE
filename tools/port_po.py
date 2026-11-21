"""Carry Godot's translations over to the port's string tables (phase 7, docs/WORKLIST-LOC-2026-11-18.md).

    py tools/port_po.py [--godot C:/Games/pawpocalypse] [--check]

Reads Godot's gettext files (<godot>/locale/<lang>.po: msgid = Godot's English, msgctxt for data words) and the port's string
tables (Content/Pawpocalypse/Localization/Strings/ST_UI.csv, ST_Data.csv, ST_Tree.csv). For every key whose English source is
still Godot's English (the same words, the same {placeholders}), the language's translation is carried over, in the context
Godot gave it where it gave one (a key's prefix names it: UPG_ upgrade, CARD_ card, ...; an ST_UI row's comment "ctx <name>").

Writes, per Unreal culture, Content/Pawpocalypse/Localization/Translations/<culture>/<Table>.csv ("Key","Text","Source"), LF:
Source is the English the translation was made for. A key whose English changes later no longer matches its row's Source:
tools/loc_status.py lists it as stale, and only it is translated again (the designer, 2026-11-18: "if we make changes later
make sure that we don't have to retranslate everything and just redo things we changed").

THE FILES ARE THE TRANSLATIONS (2026-11-20): rows come from Godot and from the port's own translators
(tools/loc_status.py --apply), and a run never loses one. Per key, in the English table's order:
1. the file's row, when its Source is today's English and its {placeholders} are the English's (a translation in use: the
   port's own, or one carried earlier);
2. else Godot's translation of today's English, when there is one (carried);
3. else the file's row as it is (stale or broken: kept, so the translator sees the old English beside the new);
4. else none (missing). A row whose key left the English tables is dropped.
Dutch and Danish have no Godot file: their files are written once they have a row.

Never carried: a line the port changed (D15's plain English and every string new to the port: no Godot translation was made
for those words), a translation whose {placeholders} differ from the English, and a plural (Godot's tr_n) in a language with
more than two forms (the port's two keys cannot hold Czech or Polish "few": ICU plural patterns, later). Each is listed in
build/loc/port_po.md. Godot's own files are only read. --check: exit 1 when a run would change a file (something Godot
could carry is not carried, a row's key is gone, a file is not in the written form).
"""
import argparse
import csv
import io
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STRINGS = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings"
OUT = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Translations"
REPORT = ROOT / "build" / "loc" / "port_po.md"
TABLES = ("ST_UI", "ST_Data", "ST_Tree")

# Godot's locale -> the Unreal culture (Steam's language in brackets). es_MX was Godot's Latin American Spanish: es-419.
CULTURES = {
    "cs": "cs", "de": "de", "es": "es", "es_MX": "es-419", "fr": "fr", "hu": "hu", "it": "it", "ja": "ja", "ko": "ko",
    "pl": "pl", "pt": "pt-PT", "pt_BR": "pt-BR", "ru": "ru", "sv": "sv", "th": "th", "tr": "tr", "uk": "uk", "vi": "vi",
    "zh_Hans": "zh-Hans", "zh_Hant": "zh-Hant",
}
# The store's languages Godot never had (designer, 2026-11-18: "Add dutch and danish"): every line is the port's own.
NEW_CULTURES = ("da", "nl")
# A data key's prefix -> Godot's msgctxt for those words.
PREFIX_CTXT = {"UPG": "upgrade", "CARD": "card", "ACH": "achievement", "JOB": "job", "TIER": "tier", "BOSS": "boss",
               "SKILL": "skillbranch", "EVO": "evolution", "TREE": "skillbranch"}
TOKEN = re.compile(r"\{[A-Za-z_][A-Za-z0-9_]*\}")


def unquote(s: str) -> str:
    out, i = [], 0
    while i < len(s):
        c = s[i]
        if c == "\\" and i + 1 < len(s):
            n = s[i + 1]
            out.append({"n": "\n", "t": "\t", '"': '"', "\\": "\\"}.get(n, n))
            i += 2
            continue
        out.append(c)
        i += 1
    return "".join(out)


def read_po(path: Path):
    """[(ctxt, msgid, msgid_plural, [msgstr...], nplurals)]"""
    entries, cur, field = [], {}, None
    nplurals = 2
    text = path.read_text(encoding="utf-8")
    m = re.search(r"nplurals=(\d+)", text)
    if m:
        nplurals = int(m.group(1))

    def flush():
        if "msgid" in cur and cur["msgid"]:
            strs = [cur.get("msgstr[%d]" % i, "") for i in range(nplurals)] if "msgid_plural" in cur else [cur.get("msgstr", "")]
            entries.append((cur.get("msgctxt", ""), cur["msgid"], cur.get("msgid_plural"), strs))
        cur.clear()

    for line in text.splitlines():
        line = line.rstrip("\r")
        if not line.strip() or line.startswith("#"):
            if not line.strip() and cur:
                flush()
            continue
        m = re.match(r'^(msgctxt|msgid_plural|msgid|msgstr\[\d+\]|msgstr)\s+"(.*)"$', line)
        if m:
            if m.group(1) in ("msgctxt",) or (m.group(1) == "msgid" and "msgid" in cur):
                if m.group(1) == "msgctxt" and "msgid" in cur:
                    flush()
                elif m.group(1) == "msgid" and "msgid" in cur and "msgstr" in "".join(cur.keys()):
                    flush()
            field = m.group(1)
            cur[field] = unquote(m.group(2))
            continue
        m = re.match(r'^"(.*)"$', line)
        if m and field:
            cur[field] = cur.get(field, "") + unquote(m.group(1))
    if cur:
        flush()
    return entries, nplurals


def read_table(name: str):
    rows = []
    with open(STRINGS / (name + ".csv"), encoding="utf-8", newline="") as f:
        for r in csv.DictReader(f):
            rows.append((r["Key"], r["SourceString"], r.get("Comment", "")))
    return rows


def ctxt_of(table: str, key: str, comment: str) -> str:
    if table == "ST_UI":
        m = re.search(r"\bctx ([a-z][a-z ]*?)(?:[:;,)]|$)", comment)
        return m.group(1).strip() if m else ""
    return PREFIX_CTXT.get(key.split("_", 1)[0], "")


def tokens_match(text: str, eng: str) -> bool:
    return sorted(TOKEN.findall(text)) == sorted(TOKEN.findall(eng))


def read_rows(path: Path):
    """A culture's file as it stands: {key: (text, source)} (none: {})."""
    if not path.exists():
        return {}
    with open(path, encoding="utf-8", newline="") as f:
        return {r["Key"]: (r["Text"], r["Source"]) for r in csv.DictReader(f)}


def merge(eng: str, have, carried):
    """The row a key gets, and why (the module docstring's order): ((text, source) or None, "own" | "godot" | "kept" | "missing")."""
    if have is not None and have[1] == eng and tokens_match(have[0], eng):
        return have, ("godot" if carried == have[0] else "own")
    if carried is not None:
        return (carried, eng), "godot"
    if have is not None:
        return have, "kept"
    return None, "missing"


def godot_lines(godot: Path, lang: str):
    """Godot's translations of one language: by (msgctxt, English) and by English alone."""
    by_ctx, by_id = {}, {}
    if lang is None:
        return by_ctx, by_id
    entries, nplurals = read_po(godot / "locale" / (lang + ".po"))
    for ctxt, msgid, plural, strs in entries:
        forms = {msgid: (strs[0], False)}
        if plural is not None:
            forms[plural] = (strs[-1], nplurals > 2)
        for eng, (tr, bMany) in forms.items():
            if not tr:
                continue
            by_ctx[(ctxt, eng)] = (tr, bMany)
            by_id.setdefault(eng, set()).add((tr, bMany))
    return by_ctx, by_id


def build(godot: Path, overlay=None):
    """Every culture's files as a run writes them. overlay: {culture: {table: {key: (text, source)}}}, rows that replace the
    files' own before the merge (tools/loc_status.py --apply)."""
    tables = {t: read_table(t) for t in TABLES}
    files, report = {}, []
    totals = {t: len(rows) for t, rows in tables.items()}
    langs = {culture: lang for lang, culture in CULTURES.items()}
    langs.update({culture: None for culture in NEW_CULTURES})
    for culture in sorted(langs):
        by_ctx, by_id = godot_lines(godot, langs[culture])
        missing, bad_tokens, many_forms = [], [], []
        written, counts = {}, {}
        for table, rows in tables.items():
            have = read_rows(OUT / culture / (table + ".csv"))
            have.update(((overlay or {}).get(culture) or {}).get(table) or {})
            out, why_count = [], {"own": 0, "godot": 0, "kept": 0, "missing": 0}
            for key, eng, comment in rows:
                ctxt = ctxt_of(table, key, comment)
                hit = by_ctx.get((ctxt, eng)) or by_ctx.get(("", eng))
                if hit is None and len(by_id.get(eng, ())) == 1:
                    hit = next(iter(by_id[eng]))
                carried = None
                if hit is None:
                    missing.append((table, key, eng))
                elif hit[1]:
                    many_forms.append((table, key, eng))
                elif not tokens_match(hit[0], eng):
                    bad_tokens.append((table, key, eng, hit[0]))
                else:
                    carried = hit[0]
                row, why = merge(eng, have.get(key), carried)
                why_count[why] += 1
                if row is not None:
                    out.append((key, row[0], row[1]))
            buf = io.StringIO(newline="")
            w = csv.writer(buf, quoting=csv.QUOTE_ALL, lineterminator="\n")
            w.writerow(["Key", "Text", "Source"])
            w.writerows(out)
            written[OUT / culture / (table + ".csv")] = buf.getvalue().encode("utf-8")
            counts[table] = why_count
            report.append((culture, table, len(out), totals[table], why_count))
        # A language Godot never had gets files once it has a row.
        if langs[culture] is not None or any(c["own"] + c["kept"] for c in counts.values()):
            files.update(written)
        report.append((culture, None, (missing, bad_tokens, many_forms), None, None))
    return files, report, tables


def write_report(report, tables):
    lines = ["# port_po: Godot's translations carried over, and the port's own", "",
             "Per culture: the rows in the files / the lines in the table; then how many are Godot's (carried), the port's own",
             "(tools/loc_status.py --apply) and kept stale or broken. tools/loc_status.py says what is left to translate.", "",
             "| Culture | " + " | ".join(TABLES) + " | All | Godot | Own | Stale kept |", "|---|" + "---|" * (len(TABLES) + 4)]
    per, why = {}, {}
    for culture, table, a, b, c in report:
        if table:
            per.setdefault(culture, {})[table] = (a, b)
            for k, n in c.items():
                why.setdefault(culture, {}).setdefault(k, 0)
                why[culture][k] += n
    for culture, t in per.items():
        rows = sum(a for a, _ in t.values())
        total = sum(b for _, b in t.values())
        w = why[culture]
        lines.append("| %s | %s | %d / %d (%.0f %%) | %d | %d | %d |" % (
            culture, " | ".join("%d / %d" % t[x] for x in TABLES), rows, total, 100.0 * rows / max(total, 1),
            w["godot"], w["own"], w["kept"]))
    first = next(r for r in report if r[1] is None and r[0] in CULTURES.values())
    missing, bad, many = first[2]
    lines += ["", "## Godot has no translation for (the first culture's list; every culture's is the same lines, give or take)", "",
              "- %d lines with no Godot translation (changed or new English)" % len(missing),
              "- %d translations whose {placeholders} differ from the English" % len(bad),
              "- %d plurals in a language with more than two forms" % len(many), ""]
    for table, key, eng in missing:
        lines.append("- `%s %s`: %s" % (table, key, eng.replace("\n", " / ")))
    for table, key, eng, tr in bad:
        lines.append("- PLACEHOLDERS `%s %s`: %s -> %s" % (table, key, eng.replace("\n", " / "), tr.replace("\n", " / ")))
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_bytes(("\n".join(lines) + "\n").encode("utf-8"))
    return per


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--godot", default="C:/Games/pawpocalypse")
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()
    files, report, tables = build(Path(a.godot))
    if a.check:
        bad = [p for p, b in files.items() if not p.exists() or p.read_bytes() != b]
        known = set(CULTURES.values()) | set(NEW_CULTURES)
        stray = [p for p in OUT.rglob("*.csv") if p not in files and p.parent.name in known]
        for p in bad + stray:
            print("port_po --check: %s differs from a run" % p.relative_to(ROOT))
        print("port_po --check: %s" % ("OK" if not bad and not stray else "STALE (py tools/port_po.py writes it; no row is lost)"))
        return 1 if bad or stray else 0
    write(files)
    per = write_report(report, tables)
    for culture, t in per.items():
        rows = sum(x for x, _ in t.values())
        total = sum(y for _, y in t.values())
        print("%-8s %4d / %d rows" % (culture, rows, total))
    print("port_po: %d files -> %s; report %s" % (len(files), OUT.relative_to(ROOT), REPORT.relative_to(ROOT)))
    return 0


def write(files):
    for p, b in files.items():
        if p.exists() and p.read_bytes() == b:
            continue
        p.parent.mkdir(parents=True, exist_ok=True)
        tmp = p.with_suffix(".csv.tmp")
        tmp.write_bytes(b)
        tmp.replace(p)


if __name__ == "__main__":
    sys.exit(main())
