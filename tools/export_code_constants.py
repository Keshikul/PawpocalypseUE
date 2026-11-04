"""Export the gameplay constants that live in Godot CODE into Data/CodeConstants.csv (docs/06 §5).

The reviewed list is Data/CodeConstantsMap.csv (`godot_name, destination, literal, note`): one row per constant
tools/list_godot_code_constants.py finds (appendix C), plus the few non-numeric or literal ones the rules need. Its
`destination` says where each one lives in the port:

    code_constants:<id>   a gameplay rule: exported here, read by PawCore (PcCodeConstants) and pawsim
    presentation          owned by the C++ class that draws it, with the Godot reason as the comment
    palette               the generated colour tokens
    input / save / audio / dev   owned by that game-module system

The export FAILS (nothing written) when the map and the Godot code disagree: a Godot constant with no map row, a map row
whose constant is gone, an id used twice, a gameplay row that is not a number or a list, a literal row whose code text
does not occur exactly once. So nothing is silently dropped, and a constant changed in Godot reaches the CSV on the next
export (tools/check_pawcore_static.py's data-export gate fails until it does).

Values are written as Python prints them: ints as ints, floats in shortest round-trip form (`1.0 / 60.0` becomes
0.016666666666666666), lists `;`-joined like every other CSV here.

    py tools/export_code_constants.py --godot C:/Games/pawpocalypse --out Data
(tools/export_godot_data.py runs this too, so one export writes every Data/*.csv.)
"""
from __future__ import annotations

import argparse
import csv
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from list_godot_code_constants import RE_CONST, RE_EXPORT, reason_above, scan  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_MAP = os.path.join(ROOT, "Data", "CodeConstantsMap.csv")
DESTINATIONS = {"presentation", "palette", "input", "save", "audio", "dev", "data"}
RE_ID = re.compile(r"^[a-z][a-z0-9_]*$")
RE_INT = re.compile(r"^[-+]?\d+$")
RE_EXPR = re.compile(r"^[-+0-9.eE*/() ]+$")
RE_LIST = re.compile(r'^\[\s*("[^"]*"\s*(,\s*"[^"]*"\s*)*)?\]$')
RE_LAST_NUMBER = re.compile(r"[-+]?\d+(?:\.\d+)?(?:e[-+]?\d+)?(?!.*\d)", re.I)


def number_text(godot_value: str) -> str | None:
    """A Godot numeric literal or constant expression as the CSV writes it; None if it is not one."""
    v = godot_value.strip()
    if RE_INT.match(v):
        return str(int(v))
    if not RE_EXPR.match(v):
        return None
    try:
        return repr(float(eval(v, {"__builtins__": {}}, {})))  # digits and + - * / ( ) only (RE_EXPR)
    except (SyntaxError, ZeroDivisionError, TypeError, ValueError):
        return None


def godot_const(godot: str, rel: str, name: str) -> tuple[str, str] | None:
    """(value text, reason) of `const NAME` (or the numeric `@export var name` default) in one Godot script, or None."""
    with open(os.path.join(godot, rel), encoding="utf-8") as f:
        lines = f.read().splitlines()
    for i, line in enumerate(lines, 1):
        m = RE_CONST.match(line) or RE_EXPORT.match(line)
        if m and m.group(1) == name:
            return m.group(2).strip(), reason_above(lines, i, (m.group(3) or "").strip())
    return None


def scene_overrides(godot: str, name: str) -> list[str]:
    """Scenes that set a property called `name`: an @export default the CSV exports would not be what the game uses."""
    hits = []
    for d, _, files in os.walk(godot):
        if ".godot" in d.split(os.sep):
            continue
        for fn in files:
            if fn.endswith(".tscn"):
                with open(os.path.join(d, fn), encoding="utf-8", errors="replace") as f:
                    if any(line.startswith(name + " = ") for line in f):
                        hits.append(os.path.relpath(os.path.join(d, fn), godot).replace(os.sep, "/"))
    return hits


def build(godot: str, map_path: str) -> tuple[list[dict], list[str]]:
    """The CodeConstants rows and the problems; rows are only good when problems is empty."""
    problems: list[str] = []
    with open(map_path, newline="", encoding="utf-8") as f:
        mapped = list(csv.DictReader(f))
    by_name: dict[str, dict] = {}
    for r in mapped:
        if r["godot_name"] in by_name:
            problems.append("map: %s listed twice" % r["godot_name"])
        by_name[r["godot_name"]] = r

    # Every constant appendix C lists has a reviewed destination.
    found = {"%s:%s" % (rel, name) for rel, _, name, _, _, _ in scan(godot)}
    for name in sorted(found - set(by_name)):
        problems.append("map: Godot constant %s has no row in %s" % (name, os.path.basename(map_path)))

    rows, ids = [], set()
    for r in mapped:
        dest, literal, godot_name = r["destination"].strip(), r["literal"], r["godot_name"]
        rel, _, name = godot_name.rpartition(":")
        if not os.path.isfile(os.path.join(godot, rel)):
            problems.append("map: %s names a file Godot does not have" % godot_name)
            continue
        if not dest.startswith("code_constants:"):
            if dest not in DESTINATIONS:
                problems.append("map: %s has unknown destination %r" % (godot_name, dest))
            elif literal:
                problems.append("map: %s has a literal but is not a code constant" % godot_name)
            elif godot_name not in found:
                problems.append("map: %s is not a constant in the Godot code any more" % godot_name)
            continue
        cid = dest.split(":", 1)[1]
        if not RE_ID.match(cid) or cid in ids:
            problems.append("map: %s: id %r is not a fresh lower-case id" % (godot_name, cid))
            continue
        ids.add(cid)
        if literal:
            with open(os.path.join(godot, rel), encoding="utf-8") as f:
                count = f.read().count(literal)
            number = RE_LAST_NUMBER.search(literal)
            if count != 1 or not number:
                problems.append("map: %s: literal %r occurs %d times in %s (must be once, with a number)"
                                % (godot_name, literal, count, rel))
                continue
            value, reason = number_text(number.group(0)), r["note"]
        else:
            got = godot_const(godot, rel, name)
            if got is None:
                problems.append("map: %s is not a constant in the Godot code any more" % godot_name)
                continue
            raw, reason = got
            value = number_text(raw)
            if value is None and RE_LIST.match(raw):
                value = ";".join(re.findall(r'"([^"]*)"', raw))
            if value is None:
                problems.append("map: %s = %s is neither a number nor a list of strings" % (godot_name, raw))
                continue
            if name.islower():
                overridden = scene_overrides(godot, name)
                if overridden:
                    problems.append("map: %s is an @export a scene overrides (%s): the default is not the game's value"
                                    % (godot_name, ", ".join(overridden)))
                    continue
        rows.append({"id": cid, "value": value, "reason": reason, "godot_name": godot_name})
    rows.sort(key=lambda r: r["id"])
    return rows, problems


def export(godot: str, out_dir: str, map_path: str = DEFAULT_MAP) -> int:
    rows, problems = build(godot, map_path)
    if problems:
        for p in problems:
            print("CodeConstants: " + p)
        print("CodeConstants: export FAILED, %d problem(s); nothing written" % len(problems))
        return 1
    os.makedirs(out_dir, exist_ok=True)
    path = os.path.join(out_dir, "CodeConstants.csv")
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=["id", "value", "reason", "godot_name"])
        w.writeheader()
        w.writerows(rows)
    print(f"{'CodeConstants':28s} {len(rows):4d} rows -> {path}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--godot", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--map", default=DEFAULT_MAP)
    a = ap.parse_args()
    return export(a.godot, a.out, a.map)


if __name__ == "__main__":
    raise SystemExit(main())
