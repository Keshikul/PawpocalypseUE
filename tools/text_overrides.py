"""The port's own English where it differs from Godot's (the designer's text pass, 2026-11-09).

The designer: "all the text should just be to the point informative information" (plain words, real numbers, one word per
idea; CLAUDE.md, tools/text_audit.py). Every changed line is in Data/TextOverrides.csv:

    table, key, text, godot, note
      table   ST_UI, ST_Data or ST_Tree
      key     the string table key
      text    the English the game shows
      godot   the English Godot shows (for ST_Data / ST_Tree: when the line was written; the generators fail when Godot's
              has changed since: review it). The engine's Godot-parity tests put these words back for their run
              (PawTextTesting.h), so they still compare against Godot's recorded screens.
      note    why (a fact checked in the code, a finding), optional

ST_Data / ST_Tree: tools/gen_data_strings.py and tools/gen_tree_tables.py write `text` instead of Godot's English. They fail
loudly when a line's key is not one they generate, when Godot's English is no longer `godot`, or when `text` drops or adds
a {token}. ST_UI is written by hand: `--check` fails when a line's `text` is not what ST_UI.csv says (change both).
The Godot project is not changed: its translation gate (ci/check_loc.py) wants every English string translated in every
shipped language, so ~200 changed strings there would break its build. A changed line is untranslated here too (docs/08).

    py tools/text_overrides.py --check       # the ST_UI lines agree with ST_UI.csv (the generators check the others)
    py tools/text_overrides.py --self-test   # each check bites on a planted fault
"""
import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FILE = ROOT / "Data" / "TextOverrides.csv"
TABLES = ("ST_UI", "ST_Data", "ST_Tree")
ST_UI = ROOT / "Content" / "Pawpocalypse" / "Localization" / "Strings" / "ST_UI.csv"
TOKEN = re.compile(r"\{[^}]*\}|<[^>]*>")


def tokens(text):
    return sorted(TOKEN.findall(text))


def load(table, path=FILE):
    """{key: row} for one table."""
    if not path.exists():
        return {}
    out = {}
    with open(path, encoding="utf-8", newline="") as f:
        for n, row in enumerate(csv.DictReader(f), start=2):
            if row["table"] not in TABLES:
                raise SystemExit("%s line %d: table %r is not one of %s" % (path.name, n, row["table"], TABLES))
            if row["table"] != table:
                continue
            if row["key"] in out:
                raise SystemExit("%s line %d: %s twice" % (path.name, n, row["key"]))
            out[row["key"]] = row
    return out


def apply(table, rows, path=FILE):
    """rows: [key, godot English, comment] as generated; returns them with each overridden line's English replaced."""
    lines = load(table, path)
    problems, out, used = [], [], set()
    for key, english, comment in rows:
        line = lines.get(key)
        if line is None:
            out.append([key, english, comment])
            continue
        used.add(key)
        if line["godot"] != english:
            problems.append("%s: Godot's English changed (was %r, now %r): review the line" % (key, line["godot"], english))
        if tokens(line["text"]) != tokens(english):
            problems.append("%s: tokens %s, Godot's line has %s" % (key, tokens(line["text"]), tokens(english)))
        out.append([key, line["text"], comment + " | English: Data/TextOverrides.csv"])
    for key in sorted(set(lines) - used):
        problems.append("%s: not a key of %s" % (key, table))
    if problems:
        raise SystemExit("Data/TextOverrides.csv (%s):\n  " % table + "\n  ".join(problems))
    return out


def check_ui(path=FILE, table_csv=ST_UI):
    """The hand-written table: each ST_UI line's `text` is what ST_UI.csv says, with Godot's {tokens}."""
    with open(table_csv, encoding="utf-8", newline="") as f:
        now = {r["Key"]: r["SourceString"] for r in csv.DictReader(f)}
    problems = []
    for key, line in load("ST_UI", path).items():
        if key not in now:
            problems.append("%s: not a key of ST_UI" % key)
        elif now[key] != line["text"]:
            problems.append("%s: ST_UI.csv says %r, the line says %r: change both" % (key, now[key], line["text"]))
        if tokens(line["text"]) != tokens(line["godot"]):
            problems.append("%s: tokens %s, Godot's line has %s" % (key, tokens(line["text"]), tokens(line["godot"])))
    if problems:
        raise SystemExit("Data/TextOverrides.csv (ST_UI):\n  " + "\n  ".join(problems))
    return 0


def self_test():
    """Each check must bite on a planted fault; a clean file must pass."""
    import tempfile
    rows = [["CARD_a_desc", "Golden enemies turn up {v}x as often.", "c"], ["CARD_b_desc", "Plain.", "c"]]
    head = "table,key,text,godot,note\n"
    good = 'ST_Data,CARD_a_desc,Golden enemies spawn x{v} as often.,Golden enemies turn up {v}x as often.,\n'
    cases = [
        ("clean", good, None),
        ("Godot changed", good.replace("turn up", "show up"), "changed"),
        ("a lost token", good.replace("x{v} as", "more"), "tokens"),
        ("an invented token", good.replace("as often.,", "as often {c}.,", 1), "tokens"),
        ("an unknown key", good + "ST_Data,CARD_zz_desc,Hi.,Hi.,\n", "not a key"),
        ("a key twice", good + good, "twice"),
        ("an unknown table", good + "ST_Cards,CARD_a_desc,Hi.,Hi.,\n", "not one of"),
    ]
    ok = True
    with tempfile.TemporaryDirectory() as d:
        for name, body, want in cases:
            p = Path(d) / "TextOverrides.csv"
            p.write_text(head + body, encoding="utf-8")
            try:
                out = apply("ST_Data", rows, p)
                got = None if out[0][1] == "Golden enemies spawn x{v} as often." and out[1][1] == "Plain." else "wrong output"
            except SystemExit as e:
                got = str(e)
            hit = (got is None) if want is None else (got is not None and want in got)
            print("  %-18s %s" % (name, "ok" if hit else "MISSED: %s" % got))
            ok &= hit
        ui = Path(d) / "ST_UI.csv"
        ui.write_text('"Key","SourceString","Comment"\n"PICK_subtitle","Take 1 card.",""\n', encoding="utf-8")
        line = "ST_UI,PICK_subtitle,Take 1 card.,Take one.,\n"
        for name, body, want in [("ui clean", line, None),
                                 ("ui table differs", line.replace("Take 1 card.,", "Take 2 cards.,", 1), "change both"),
                                 ("ui unknown key", line.replace("PICK_subtitle", "PICK_zz"), "not a key"),
                                 ("ui token", line.replace("Take one.", "Take {n}."), "tokens")]:
            p = Path(d) / "TextOverrides.csv"
            p.write_text(head + body, encoding="utf-8")
            try:
                got = None if check_ui(p, ui) == 0 else "nonzero"
            except SystemExit as e:
                got = str(e)
            hit = (got is None) if want is None else (got is not None and want in got)
            print("  %-18s %s" % (name, "ok" if hit else "MISSED: %s" % got))
            ok &= hit
    print("text_overrides --self-test: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    import sys
    if "--self-test" in sys.argv:
        sys.exit(self_test())
    if "--check" in sys.argv:
        check_ui()
        print("text_overrides --check: the ST_UI lines agree with ST_UI.csv")
        sys.exit(0)
    print(__doc__)
