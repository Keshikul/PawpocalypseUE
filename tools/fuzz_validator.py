"""LONG TEST C: validator fuzz. Thousands of random single-cell corruptions of Data/*.csv, each run through PawCore's
loader and validator (`pawcore_sim --validate`), each classified CAUGHT or UNCAUGHT.

Column types come from the generated row readers (Source/PawCore/Public/PcDataRows.h: `R.ReadDouble("hp", ...)`), so
every corruption is typed. Three classes:
  hostile  values no game can mean: nan/inf/empty/text in a number, an unknown enum name, a duplicate or upper-case id,
           a reference to an id that does not exist, a malformed list or ladder, a list shortened or lengthened.
           These SHOULD be rejected; every uncaught one is listed by file and column for an explanation (the report).
  wild     legal-looking but extreme: zero, negative, x1000, x0.001, a reversed list. Measured, not required.
  benign   x(1 +- 0.001), the size of a real balance edit. Measured: a rejection here is a tight invariant (fine) and is
           listed so it can be checked to be one on purpose.

Writes build/fuzz/report.txt. Usage:
  py tools/fuzz_validator.py --trials 3000          # the nightly (a few minutes)
  py tools/fuzz_validator.py --trials 30000         # the long run
Exit 0 = every hostile corruption caught or explained (EXPLAINED below); 1 = an unexplained one, or the clean baseline
does not reproduce (then nothing else means anything); 2 = could not run.
Found on its first run (2026-10-28): upper-case ids passed the validator -> rule ids.lower_case.
"""

import argparse
import collections
import csv
import io
import json
import math
import random
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "Data"
WORK = ROOT / "build" / "fuzz"
SIM = ROOT / "build" / "pawcore_tests" / "pawcore_sim.exe"
ROWS_H = ROOT / "Source" / "PawCore" / "Public" / "PcDataRows.h"

# Row struct -> CSV file (docs/06 §4: one CSV per Godot Resource class).
STRUCT_FILE = {
    "PcTier": "TierData", "PcEnemy": "EnemyData", "PcBoss": "BossData", "PcUpgrade": "UpgradeData",
    "PcSkillBranch": "SkillBranchData", "PcEvolution": "EvolutionData", "PcCard": "CardData", "PcJob": "JobData",
    "PcAchievement": "AchievementData", "PcCombatConstants": "CombatConstantsData",
    "PcReawakenConstants": "ReawakenConstantsData",
}

# Failures the shipped data already has (Tests/PcValidateTests.cpp KnownGodotDataIssues): not caused by a corruption.
KNOWN = {
    "RULE [achievement.target] achievement br_ferocity_5 counts levels of unknown branch 'ferocity'",
    "RULE [achievement.target] achievement br_mystique_6 counts levels of unknown branch 'mystique'",
}


# Uncaught HOSTILE classes that are harmless, with the reason. An uncaught hostile class NOT in this list fails the run:
# it is either explained here (with evidence) or becomes a validator rule.
EXPLAINED = {
    ("EvolutionData", "ability_ids", "dangling_element"):
        "dead field: evolution_data.gd declares ability_ids, no script reads it, every shipped row is empty",
    ("CardData", "level_values", "drop_last"):
        "on a card WITHOUT a cost a shorter ladder is a legal design edit (max level = ladder length); on a card with a "
        "cost, card.cost catches the length mismatch with cost_values",
}


def column_types():
    """{file: {column: reader}} from the generated ReadRow bodies."""
    text = ROWS_H.read_text(encoding="utf-8")
    out = {}
    for m in re.finditer(r"inline void ReadRow\(const CsvTable& T, size_t Row, (Pc\w+)& Out.*?\n\t}\n", text, re.S):
        cols = {}
        for r in re.finditer(r'R\.Read(\w+)(?:<[^>]*>)?\("(\w+)"', m.group(0)):
            cols[r.group(2)] = r.group(1)
        out[STRUCT_FILE[m.group(1)]] = cols
    return out


def read_csv(name):
    with open(DATA / ("%s.csv" % name), encoding="utf-8", newline="") as f:
        return list(csv.reader(f))


def write_csv(path, rows):
    with open(path, "w", encoding="utf-8", newline="") as f:
        csv.writer(f, lineterminator="\n").writerows(rows)


def validate(data_dir):
    r = subprocess.run([str(SIM), "--data", str(data_dir), "--validate"], capture_output=True, text=True)
    if r.returncode not in (0, 1):
        raise SystemExit("pawcore_sim crashed (exit %d): %s" % (r.returncode, r.stderr[-500:]))
    return [l for l in r.stdout.splitlines() if l and l not in KNOWN]


def number(v):
    try:
        return float(v)
    except ValueError:
        return None


def all_ids(tables):
    ids = set()
    for rows in tables.values():
        h = rows[0]
        if "id" in h:
            i = h.index("id")
            ids.update(r[i] for r in rows[1:])
    return ids


def corruptions(reader, column, value, rows, row_index, col_index, ids, rng):
    """[(class, kind, new value)] for one cell."""
    out = []
    v = number(value)
    if reader in ("Double", "Int"):
        # An EMPTY cell is not hostile: it is how the exporter writes "not set in the .tres", i.e. the script's default
        # (the shipped data has 1,000+ of them, e.g. every JobData multiplier left at x1). Wild, like any other value.
        out += [("hostile", "nan", "nan"), ("hostile", "inf", "inf"), ("hostile", "text", "abc"),
                ("wild", "empty_is_default", ""), ("hostile", "overflow", "1e400")]
        if reader == "Int":
            out.append(("hostile", "fraction", "1.5"))
        if v is not None:
            fmt = (lambda x: str(int(round(x)))) if reader == "Int" else repr
            out += [("wild", "zero", fmt(0.0)), ("wild", "negative", fmt(-abs(v) if v else -1.0)),
                    ("wild", "x1000", fmt(v * 1000.0)), ("wild", "x0.001", fmt(v * 0.001))]
            if reader == "Double" and v != 0.0:
                out.append(("benign", "x1.001", repr(v * (1.0 + rng.choice([-1, 1]) * 0.001))))
            if reader == "Int":
                out.append(("benign", "plus1", str(int(v) + 1)))
    elif reader == "Bool":
        out += [("hostile", "text", "maybe"), ("hostile", "number", "2")]
    elif reader == "Enum":
        out += [("hostile", "unknown_name", "BOGUS"), ("hostile", "lower_case", value.lower() if value else "x"),
                ("hostile", "number", "1")]
    elif reader == "Color":
        out += [("hostile", "text", "notacolor"), ("hostile", "short", "1;0")]
    elif reader == "String":
        if column == "id" and len(rows) == 2:
            pass  # the singleton tables (CombatConstants, ReawakenConstants): their id is never looked up
        elif column == "id":
            other = [r[col_index] for i, r in enumerate(rows[1:], 1) if i != row_index]
            if other:
                out.append(("hostile", "duplicate_id", rng.choice(other)))
            out += [("hostile", "empty_id", ""), ("hostile", "upper_case_id", value.upper() + "X" if value else "X")]
        elif value in ids:
            out += [("hostile", "dangling_reference", "no_such_id"), ("wild", "empty_reference", "")]
        else:
            out += [("wild", "empty", ""), ("benign", "changed_text", value + " x")]
    elif reader == "DoubleList":
        parts = value.split(";") if value else []
        if parts:
            i = rng.randrange(len(parts))
            nan = list(parts); nan[i] = "nan"
            text = list(parts); text[i] = "abc"
            out += [("hostile", "element_nan", ";".join(nan)), ("hostile", "element_text", ";".join(text)),
                    ("hostile", "trailing_separator", value + ";")]
            if len(parts) > 1:
                out.append(("hostile", "drop_last", ";".join(parts[:-1])))
                out.append(("wild", "reversed", ";".join(reversed(parts))))
            out.append(("hostile", "append", value + ";" + parts[-1]))
            neg = list(parts); neg[i] = repr(-abs(float(parts[i])) or -1.0)
            out.append(("wild", "element_negative", ";".join(neg)))
            ben = list(parts); ben[i] = repr(float(parts[i]) * 1.001)
            out.append(("benign", "element_x1.001", ";".join(ben)))
        out.append(("wild", "emptied_is_default", ""))
    elif reader == "StringList":
        parts = value.split(";") if value else []
        out.append(("hostile", "dangling_element", ";".join(parts + ["no_such_id"])))
        if parts:
            out.append(("wild", "drop_element", ";".join(parts[:-1])))
    elif reader == "LadderMap":
        out += [("hostile", "broken_json", "{"), ("hostile", "not_a_map", "[1,2]")]
        try:
            ladder = json.loads(value) if value else {}
        except ValueError:
            ladder = {}
        if ladder:
            k = rng.choice(sorted(ladder))
            bad = dict(ladder); bad[k] = ["x"]
            out.append(("hostile", "channel_text", json.dumps(bad)))
            out.append(("wild", "drop_channel", json.dumps({c: v for c, v in ladder.items() if c != k})))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--trials", type=int, default=3000)
    ap.add_argument("--seed", type=int, default=20261028)
    args = ap.parse_args()
    if not SIM.exists():
        print("FAIL  %s missing: run tools/build_pawcore_tests.ps1" % SIM)
        return 2

    types = column_types()
    tables = {name: read_csv(name) for name in types}
    ids = all_ids(tables)
    fuzz_data = WORK / "data"
    if fuzz_data.exists():
        shutil.rmtree(fuzz_data)
    shutil.copytree(DATA, fuzz_data)

    # BASELINE: every table re-serialised by this script's CSV writer must still validate clean, or a "caught" below
    # could be the writer's quoting rather than the corruption.
    for name, rows in tables.items():
        write_csv(fuzz_data / ("%s.csv" % name), rows)
    base = validate(fuzz_data)
    if base:
        print("FAIL  the re-serialised clean data does not validate:\n  " + "\n  ".join(base[:10]))
        return 1

    # Every (file, row, column) cell the readers know about, so trials spread over every column, not every row.
    cells = []
    for name, rows in tables.items():
        header = rows[0]
        for column, reader in types[name].items():
            if column not in header:
                continue
            c = header.index(column)
            for r in range(1, len(rows)):
                cells.append((name, r, c, column, reader))
    by_column = collections.defaultdict(list)
    for cell in cells:
        by_column[(cell[0], cell[3])].append(cell)
    columns = sorted(by_column)

    rng = random.Random(args.seed)
    stats = collections.defaultdict(lambda: [0, 0])  # (class, file, column, kind) -> [tried, caught]
    uncaught_examples = {}
    caught_rules = collections.Counter()
    for t in range(args.trials):
        name, r, c, column, reader = rng.choice(by_column[columns[t % len(columns)]])
        rows = tables[name]
        options = corruptions(reader, column, rows[r][c], rows, r, c, ids, rng)
        if not options:
            continue
        klass, kind, new = rng.choice(options)
        if new == rows[r][c]:
            continue
        bad = [list(x) for x in rows]
        bad[r][c] = new
        path = fuzz_data / ("%s.csv" % name)
        write_csv(path, bad)
        try:
            failures = validate(fuzz_data)
        finally:
            write_csv(path, rows)
        key = (klass, name, column, kind)
        stats[key][0] += 1
        if failures:
            stats[key][1] += 1
            for f in failures:
                tag = re.match(r"(LOAD|RULE) (\[[\w.]+\])?", f)
                caught_rules[tag.group(0) if tag else f[:40]] += 1
        else:
            uncaught_examples.setdefault(key, "%s row %d (%s): %r -> %r" % (name, r, rows[r][0], rows[r][c], new))
        if (t + 1) % 500 == 0:
            print("  %d / %d trials" % (t + 1, args.trials), flush=True)

    lines = ["Validator fuzz: %d trials, seed %d, %d columns over %d files" % (args.trials, args.seed, len(columns), len(types)), ""]
    for klass in ("hostile", "wild", "benign"):
        keys = sorted(k for k in stats if k[0] == klass)
        tried = sum(stats[k][0] for k in keys)
        caught = sum(stats[k][1] for k in keys)
        lines.append("%-8s %6d tried, %6d caught (%.1f%%)" % (klass, tried, caught, 100.0 * caught / max(tried, 1)))
    lines += ["", "UNCAUGHT HOSTILE corruptions (each needs an explanation or a rule):"]
    unexplained = []
    for k in sorted(k for k in stats if k[0] == "hostile" and stats[k][1] < stats[k][0]):
        lines.append("  %-22s %-28s %-20s %4d of %4d uncaught   e.g. %s"
                     % (k[1], k[2], k[3], stats[k][0] - stats[k][1], stats[k][0], uncaught_examples.get(k, "")))
        why = EXPLAINED.get(k[1:])
        lines.append("      %s" % (("explained: " + why) if why else "UNEXPLAINED: add a validator rule or an explanation"))
        if not why:
            unexplained.append(k)
    lines += ["", "UNCAUGHT WILD corruptions (measured):"]
    for k in sorted(k for k in stats if k[0] == "wild" and stats[k][1] < stats[k][0]):
        lines.append("  %-22s %-28s %-20s %4d of %4d uncaught" % (k[1], k[2], k[3], stats[k][0] - stats[k][1], stats[k][0]))
    lines += ["", "CAUGHT BENIGN corruptions (a tight invariant, or a rule that is too strict):"]
    for k in sorted(k for k in stats if k[0] == "benign" and stats[k][1] > 0):
        lines.append("  %-22s %-28s %-20s %4d of %4d caught" % (k[1], k[2], k[3], stats[k][1], stats[k][0]))
    lines += ["", "What caught them (loader errors and rule tags):"]
    for tag, n in caught_rules.most_common():
        lines.append("  %6d  %s" % (n, tag))
    report = "\n".join(lines) + "\n"
    (WORK / "report.txt").write_text(report, encoding="utf-8")
    print(report)
    shutil.rmtree(fuzz_data, ignore_errors=True)
    if unexplained:
        print("FAIL  %d uncaught hostile corruption class(es) without an explanation" % len(unexplained))
        return 1
    print("PASS  every hostile corruption was caught or is explained")
    return 0


if __name__ == "__main__":
    sys.exit(main())
