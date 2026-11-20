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

Never carried: a line the port changed (D15's plain English and every string new to the port: no Godot translation was made
for those words), a translation whose {placeholders} differ from the English, and a plural (Godot's tr_n) in a language with
more than two forms (the port's two keys cannot hold Czech or Polish "few": ICU plural patterns, later). Each is listed in
build/loc/port_po.md. Godot's own files are only read. --check: exit 1 when the written files differ from a fresh run.
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


def build(godot: Path):
    tables = {t: read_table(t) for t in TABLES}
    files, report = {}, []
    totals = {t: len(rows) for t, rows in tables.items()}
    for lang, culture in sorted(CULTURES.items(), key=lambda kv: kv[1]):
        entries, nplurals = read_po(godot / "locale" / (lang + ".po"))
        by_ctx, by_id = {}, {}
        for ctxt, msgid, plural, strs in entries:
            forms = {msgid: (strs[0], False)}
            if plural is not None:
                forms[plural] = (strs[-1] if nplurals <= 2 else strs[-1], nplurals > 2)
            for eng, (tr, bMany) in forms.items():
                if not tr:
                    continue
                by_ctx[(ctxt, eng)] = (tr, bMany)
                by_id.setdefault(eng, set()).add((tr, bMany))
        missing, bad_tokens, many_forms = [], [], []
        for table, rows in tables.items():
            out = []
            for key, eng, comment in rows:
                ctxt = ctxt_of(table, key, comment)
                hit = by_ctx.get((ctxt, eng)) or by_ctx.get(("", eng))
                if hit is None and len(by_id.get(eng, ())) == 1:
                    hit = next(iter(by_id[eng]))
                if hit is None:
                    missing.append((table, key, eng))
                    continue
                tr, bMany = hit
                if bMany:
                    many_forms.append((table, key, eng))
                    continue
                if sorted(TOKEN.findall(tr)) != sorted(TOKEN.findall(eng)):
                    bad_tokens.append((table, key, eng, tr))
                    continue
                out.append((key, tr, eng))
            buf = io.StringIO(newline="")
            w = csv.writer(buf, quoting=csv.QUOTE_ALL, lineterminator="\n")
            w.writerow(["Key", "Text", "Source"])
            w.writerows(out)
            files[OUT / culture / (table + ".csv")] = buf.getvalue().encode("utf-8")
            report.append((culture, table, len(out), totals[table]))
        report.append((culture, None, (missing, bad_tokens, many_forms), None))
    return files, report, tables


def write_report(report, tables):
    lines = ["# port_po: Godot's translations carried over", "",
             "Per culture: the lines carried / the lines in the table. The rest are translated by hand (changed or new English).", "",
             "| Culture | " + " | ".join(TABLES) + " | All |", "|---|" + "---|" * (len(TABLES) + 1)]
    per = {}
    for culture, table, a, b in report:
        if table:
            per.setdefault(culture, {})[table] = (a, b)
    for culture, t in per.items():
        carried = sum(a for a, _ in t.values())
        total = sum(b for _, b in t.values())
        lines.append("| %s | %s | %d / %d (%.0f %%) |" % (culture, " | ".join("%d / %d" % t[x] for x in TABLES), carried, total,
                                                            100.0 * carried / max(total, 1)))
    first = next(r for r in report if r[1] is None)
    missing, bad, many = first[2]
    lines += ["", "## Not carried (the first culture's list; every culture's is the same lines, give or take)", "",
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
        stray = [p for p in OUT.rglob("*.csv") if p not in files and p.parent.name in CULTURES.values()]
        for p in bad + stray:
            print("port_po --check: %s differs from a fresh run" % p.relative_to(ROOT))
        print("port_po --check: %s" % ("OK" if not bad and not stray else "STALE"))
        return 1 if bad or stray else 0
    for p, b in files.items():
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(b)
    per = write_report(report, tables)
    for culture, t in per.items():
        carried = sum(x for x, _ in t.values())
        total = sum(y for _, y in t.values())
        print("%-8s %4d / %d carried" % (culture, carried, total))
    print("port_po: %d files -> %s; report %s" % (len(files), OUT.relative_to(ROOT), REPORT.relative_to(ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
