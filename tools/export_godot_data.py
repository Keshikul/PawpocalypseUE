"""Export the Godot project's /data .tres resources to CSV, one file per Resource class.

This is the ONE-TIME bridge from the Godot project to the Unreal project, and the
re-runnable check that the two still agree while both exist. The CSVs it writes
under Data/ become the Unreal project's source of truth (imported into DataTables
by a headless editor script, read directly by pawsim and the C++ sim library).

    py tools/export_godot_data.py --godot C:/Games/pawpocalypse --out Data
    py tools/export_godot_data.py --godot C:/Games/pawpocalypse --out Data --catalogue docs/appendix-a-content-catalogue.md

Standard library only. Conventions (docs/07-data-pipeline.md):
  - one row per resource, first column `id` (lower-case string id, never an FName)
  - lists joined with ';' (CSV tools leave it alone), dictionaries as JSON
  - a reference to another resource is written as THAT resource's id
  - enum fields are written as the enum NAME, never the Godot index: Godot serialises
    enums by index, Unreal must never inherit that trap (docs/04 section 6)
  - Color(r, g, b, a) as 'r;g;b;a'
"""
from __future__ import annotations

import argparse
import csv
import json
import os
import re
import sys
from collections import defaultdict

RE_EXT = re.compile(r'\[ext_resource type="(\w+)"[^\]]*path="([^"]+)"[^\]]*id="([^"]+)"\]')
RE_HEADER = re.compile(r'\[gd_resource type="Resource" script_class="(\w+)"')
RE_KEYVAL = re.compile(r'^([A-Za-z_][A-Za-z0-9_/]*)\s*=\s*(.*)$')
RE_ENUM_START = re.compile(r'^enum\s+(\w+)\s*\{(.*)$')
RE_EXPORT = re.compile(r'^@export(?:_\w+)?(?:\([^)]*\))?\s+var\s+(\w+)\s*:\s*([\w.]+)')


def res_path_to_fs(godot_root: str, res_path: str) -> str:
    return os.path.join(godot_root, res_path.replace("res://", "").replace("/", os.sep))


def parse_enums(gd_text: str) -> dict[str, list[str]]:
    """enum Name { A, B = 3, C } -> {'Name': ['A', 'B'... indexed by value]}. Handles multi-line
    bodies and comments. Values are placed at their explicit index; gaps stay ''."""
    enums: dict[str, list[str]] = {}
    lines = gd_text.splitlines()
    i = 0
    while i < len(lines):
        m = RE_ENUM_START.match(lines[i].strip())
        if not m:
            i += 1
            continue
        name, rest = m.group(1), m.group(2)
        body = rest
        while "}" not in body and i + 1 < len(lines):
            i += 1
            body += "\n" + lines[i]
        body = body.split("}")[0]
        body = re.sub(r"#[^\n]*", "", body)
        values: dict[int, str] = {}
        nxt = 0
        for item in body.replace("\n", ",").split(","):
            item = item.strip()
            if not item:
                continue
            if "=" in item:
                k, v = item.split("=", 1)
                nxt = int(v.strip())
                item = k.strip()
            values[nxt] = item
            nxt += 1
        size = max(values) + 1 if values else 0
        enums[name] = [values.get(n, "") for n in range(size)]
        i += 1
    return enums


def parse_export_types(gd_text: str) -> dict[str, str]:
    out = {}
    for line in gd_text.splitlines():
        m = RE_EXPORT.match(line.strip())
        if m:
            out[m.group(1)] = m.group(2).split(".")[-1]
    return out


def split_top_level(s: str) -> list[str]:
    """Split on commas not nested inside (), [], {} or quotes."""
    parts, depth, cur, quote = [], 0, [], False
    for ch in s:
        if ch == '"' and (not cur or cur[-1] != "\\"):
            quote = not quote
        if not quote:
            if ch in "([{":
                depth += 1
            elif ch in ")]}":
                depth -= 1
            elif ch == "," and depth == 0:
                parts.append("".join(cur).strip())
                cur = []
                continue
        cur.append(ch)
    if "".join(cur).strip():
        parts.append("".join(cur).strip())
    return parts


class Exporter:
    def __init__(self, godot_root: str):
        self.root = godot_root
        self.id_cache: dict[str, str] = {}
        self.script_cache: dict[str, tuple[dict, dict]] = {}

    def script_info(self, res_path: str):
        if res_path not in self.script_cache:
            with open(res_path_to_fs(self.root, res_path), encoding="utf-8") as f:
                text = f.read()
            self.script_cache[res_path] = (parse_enums(text), parse_export_types(text))
        return self.script_cache[res_path]

    def id_of(self, res_path: str) -> str:
        if res_path not in self.id_cache:
            rid = os.path.splitext(os.path.basename(res_path))[0]
            fs = res_path_to_fs(self.root, res_path)
            if fs.endswith(".tres") and os.path.exists(fs):
                with open(fs, encoding="utf-8") as f:
                    for line in f:
                        m = re.match(r'^id = "(.*)"', line)
                        if m:
                            rid = m.group(1)
                            break
            self.id_cache[res_path] = rid
        return self.id_cache[res_path]

    def convert(self, raw: str, ext: dict[str, str]):
        raw = raw.strip()
        if raw.startswith('"'):
            return json.loads(raw)
        if raw in ("true", "false"):
            return raw
        m = re.match(r'^ExtResource\("([^"]+)"\)$', raw)
        if m:
            return self.id_of(ext[m.group(1)])
        m = re.match(r'^Array\[[\w]+\]\(\[(.*)\]\)$', raw, re.S)
        if m:
            return ";".join(str(self.convert(p, ext)) for p in split_top_level(m.group(1)))
        if raw.startswith("[") and raw.endswith("]"):
            return ";".join(str(self.convert(p, ext)) for p in split_top_level(raw[1:-1]))
        m = re.match(r'^Color\((.*)\)$', raw)
        if m:
            return ";".join(p.strip() for p in m.group(1).split(","))
        if raw.startswith("{"):
            inner = raw[1:-1].strip()
            d = {}
            for item in split_top_level(inner):
                if not item:
                    continue
                k, v = item.split(":", 1)
                val = self.convert(v, ext)
                d[json.loads(k.strip())] = [float(x) for x in val.split(";")] if isinstance(val, str) and ";" in val else val
            return json.dumps(d, separators=(",", ":"))
        return raw

    def parse_tres(self, fs_path: str):
        with open(fs_path, encoding="utf-8") as f:
            text = f.read()
        hm = RE_HEADER.search(text)
        if not hm:
            return None
        cls = hm.group(1)
        ext = {m.group(3): m.group(2) for m in RE_EXT.finditer(text)}
        script_path = next((p for p in ext.values() if p.endswith(".gd")), None)
        body = text.split("[resource]", 1)[1]
        # join multi-line values (dictionaries, long arrays) onto their key line
        rows, cur = [], None
        for line in body.splitlines():
            if RE_KEYVAL.match(line) and (cur is None or self._balanced(cur)):
                if cur is not None:
                    rows.append(cur)
                cur = line
            elif cur is not None and line.strip():
                cur += "\n" + line
        if cur is not None:
            rows.append(cur)
        enums, types = self.script_info(script_path) if script_path else ({}, {})
        record = {}
        for row in rows:
            k, v = RE_KEYVAL.match(row.split("\n", 1)[0]).groups()
            if "\n" in row:
                v = v + "\n" + row.split("\n", 1)[1]
            if k == "script":
                continue
            val = self.convert(v, ext)
            enum_name = types.get(k)
            if enum_name in enums and re.fullmatch(r"-?\d+", str(val)):
                idx = int(val)
                names = enums[enum_name]
                val = names[idx] if 0 <= idx < len(names) and names[idx] else f"UNKNOWN_{idx}"
            record[k] = val
        return cls, record

    @staticmethod
    def _balanced(s: str) -> bool:
        return s.count("{") == s.count("}") and s.count("[") == s.count("]") and s.count("(") == s.count(")")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--godot", required=True, help="root of the Godot project (holds project.godot)")
    ap.add_argument("--out", required=True, help="folder for the CSVs")
    ap.add_argument("--catalogue", help="also write a markdown content catalogue here")
    args = ap.parse_args()

    ex = Exporter(args.godot)
    by_class: dict[str, list[dict]] = defaultdict(list)
    data_root = os.path.join(args.godot, "data")
    for dirpath, _, files in os.walk(data_root):
        for fn in sorted(files):
            if not fn.endswith(".tres"):
                continue
            parsed = ex.parse_tres(os.path.join(dirpath, fn))
            if parsed is None:
                continue
            cls, rec = parsed
            rec = {"id": rec.pop("id", os.path.splitext(fn)[0]), **rec}
            rec["_godot_file"] = os.path.relpath(os.path.join(dirpath, fn), args.godot).replace(os.sep, "/")
            by_class[cls].append(rec)

    os.makedirs(args.out, exist_ok=True)
    total = 0
    for cls, recs in sorted(by_class.items()):
        cols: list[str] = []
        for r in recs:
            for k in r:
                if k not in cols:
                    cols.append(k)
        cols.remove("_godot_file")
        cols.append("_godot_file")
        path = os.path.join(args.out, f"{cls}.csv")
        with open(path, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=cols)
            w.writeheader()
            for r in sorted(recs, key=lambda r: r["id"]):
                w.writerow(r)
        total += len(recs)
        print(f"{cls:28s} {len(recs):4d} rows -> {path}")
    print(f"TOTAL {total} resources in {len(by_class)} classes")

    if args.catalogue:
        write_catalogue(by_class, args.catalogue)
        print(f"catalogue -> {args.catalogue}")
    return 0


def _table(recs, cols, title, note=""):
    out = [f"## {title}", ""]
    if note:
        out += [note, ""]
    out.append("| " + " | ".join(cols) + " |")
    out.append("|" + "---|" * len(cols))
    for r in recs:
        cells = []
        for c in cols:
            v = str(r.get(c, "")).replace("|", "\\|").replace("\n", " ")
            if len(v) > 110:
                v = v[:107] + "..."
            cells.append(v)
        out.append("| " + " | ".join(cells) + " |")
    out.append("")
    return out


def write_catalogue(by_class, path):
    def tier_key(r):
        m = re.search(r"(\d+)$", r["id"])
        return int(m.group(1)) if m else 0

    lines = [
        "# Appendix A - Content catalogue (generated)",
        "",
        "> Generated by `tools/export_godot_data.py` from the Godot project's `/data`. **Do not edit by hand**:",
        "> re-run the exporter. The full rows (every field) are the CSVs in `Data/`; this page is the readable",
        "> subset. Enum fields are written by NAME.",
        "",
    ]
    g = by_class
    if "TierData" in g:
        lines += _table(sorted(g["TierData"], key=tier_key),
                        ["id", "display_name", "required_evolution_stage", "catnip_to_unlock", "spawn_rate_per_min",
                         "reward_multiplier", "golden_chance", "boss_data", "spawn_pool", "spawn_weights"], "Biomes (TierData)")
    if "BossData" in g:
        lines += _table(g["BossData"], ["id", "display_name", "total_hp", "reward_catnip", "time_limit_sec",
                                         "bait_cost_catnip", "bait_required", "kills_per_bait", "phase_hp_thresholds",
                                         "phase_vulnerable_sec", "phase_shield_sec"], "Bosses (BossData)")
    if "EvolutionData" in g:
        lines += _table(g["EvolutionData"], ["id", "stage", "display_name", "weapon_multiplier", "texture_path"],
                        "Evolution stages (EvolutionData)")
    if "UpgradeData" in g:
        lines += _table(g["UpgradeData"], ["id", "display_name", "effect_type", "target_stat", "base_cost", "cost_growth",
                                            "effect_per_level", "cap", "cap_per_floor", "cap_per_reawaken"],
                        "Shop lines (UpgradeData)")
    if "EnemyData" in g:
        lines += _table(sorted(g["EnemyData"], key=lambda r: (int(r.get("tier", 0)), r["id"])),
                        ["id", "tier", "hp", "reward_catnip", "movement", "is_signature", "invuln_vulnerable_sec",
                         "invuln_shield_sec", "flees_player", "speed_multiplier", "sprite_target_px"], "Enemies (EnemyData)")
    if "CardData" in g:
        lines += _table(g["CardData"], ["id", "display_name", "flavour", "effect", "level_values", "cost", "cost_values",
                                         "description"], "Cards (CardData)")
    if "SkillBranchData" in g:
        lines += _table(sorted(g["SkillBranchData"], key=lambda r: (r.get("parent", ""), r["id"])),
                        ["id", "display_name", "parent", "is_unlock", "effect_type", "level_costs", "level_effects",
                         "job_id", "gated_by_boss", "extra_effects"], "Skill tree nodes (SkillBranchData)")
    if "JobData" in g:
        lines += _table(g["JobData"], ["id", "display_name", "dropped_by_biome", "description"], "Jobs (JobData)",
                        "Every multiplier column is in `Data/JobData.csv`.")
    if "AchievementData" in g:
        lines += _table(g["AchievementData"], ["id", "display_name", "metric", "threshold", "stardust_reward",
                                                "requirement_text", "secret"], "Achievements (AchievementData)")
    for single in ("CombatConstantsData", "ReawakenConstantsData"):
        if single in g:
            rec = g[single][0]
            lines += [f"## {single}", "", "| field | value |", "|---|---|"]
            lines += [f"| {k} | {v} |" for k, v in rec.items() if k not in ("id", "_godot_file")]
            lines.append("")
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))


if __name__ == "__main__":
    sys.exit(main())
