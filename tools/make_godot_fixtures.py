"""Regenerate PawCore's Godot ground-truth fixtures by running the REAL Godot code.

PawCore's L1 tests compare against what Godot itself computes, not against what we think it computes:
  amounts  Amounts.short over 4,403 values, English and a 10000-grouping language  -> Tests/Data/amounts_godot.tsv
  combat   CombatResolver over 5,000 random CombatStats snapshots                  -> Tests/Data/combat_godot.tsv (first 2,000)
  stats    GameState._rebuild_stats() over 3,000 random progress states            -> Tests/Data/stats_godot.tsv (first 1,000)
  tree     SkillTreeManager (sells, prices, rivals, gates, purchase, grant), 3,000 states -> Tests/Data/tree_godot.tsv
  tree_layout SkillTreeUI's layout (branch order, hex seating, separation, normalising, bounds, canvas) and nav_pick on the
           shipping tree and 400 random trees -> Tests/Data/tree_layout_godot.tsv
  tree_text SkillTreeUI._effect_text() for every branch at every level, and its number expressions on 2,000 values
           -> Tests/Data/tree_text_godot.tsv
  jobs     GameState's jobs (held, run lock, Union Rules, job terms, picker), 3,000 states -> Tests/Data/jobs_godot.tsv
  job_picker the real JobPicker (job_picker.gd): what it offers, the default row, Union Rules' second ask, and the run
           state after each press, 3,000 states -> Tests/Data/job_picker_godot.tsv (first 1,000)
  save     the real SaveManager: save_game() from a random live state (the file read back), load_game() of a random
           save over a random live state, 3,000 rows -> Tests/Data/save_godot.tsv (first 1,000)
  victory  the real HUD's VictoryScreen: format_duration, when GameState.mark_boss_defeated() raises the ending, the line
           it shows and the pause flag, 3,000 states -> Tests/Data/victory_godot.tsv (first 1,000); its look -> log
  progress GameState's mutations (kills, waves, travel, bosses, bait, shop, Reawaken, clocks, new game), 3,000 states x
           12 ops -> Tests/Data/progress_godot.tsv
  achievements AchievementManager (earn, claim, claim all, load, new game), 3,000 states x 10 ops
           -> Tests/Data/achievements_godot.tsv
  draw     CardManager's deal (XP gain, draw, offer, reroll, choose, banish) from seeded global RNG, 3,000 states x 8 ops
           -> Tests/Data/draw_godot.tsv
  economy  GameState's economy (rewards, waves, gates, Reawaken, bait, Legacy), 3,000 states -> Tests/Data/economy_godot.tsv
           (first 1,000)
  field_rules the realm's pure rules (GameState field getters, spawn cadence, TierData.pick_enemy,
           TierController.playfield_rect, a real Enemy node's activate/take_damage), 3,000 states
           -> Tests/Data/field_rules_godot.tsv (first 1,000)
  kittens  the kittens' pure rules (GameState kitten getters, the pool size, a real Kitten node's orbit slot,
           _pick_target, _is_overcrowded and one hunt() over real Enemy nodes), 3,000 states
           -> Tests/Data/kittens_godot.tsv (first 1,000)
  aura     the aura's and zones' pure rules (GameState aura/Snare/Unicorn getters, the aura cards' values, aura, poop and
           player DPS, a real Enemy node's apply_slow), 3,000 states -> Tests/Data/aura_godot.tsv (first 1,000)
  pounce   the Pounce's pure rules (GameState Pounce/Reflex/Daredevil getters, the Pounce cards' values, pounce cooldown and
           damage), 3,000 states -> Tests/Data/pounce_godot.tsv (first 1,000)
  extras   the paw's and the kill's extras and the Prism (their GameState getters, Catnip Bomb and Scavenge card values),
           3,000 states -> Tests/Data/extras_godot.tsv (first 1,000)
  boss     a real Boss node: take_damage (shield, pierce, phases, the kill), heal (the cap), the shield cycle, 3,000
           bosses x up to 12 operations -> Tests/Data/boss_godot.tsv (first 1,000)
The full combat, stats and economy sets go to build/*_full.tsv for the long tests (docs/09).

The Godot repo is never written: the project is copied (without .git) to build/godot_fixtures/project, the scripts in
tools/godot_fixtures/ are dropped into the copy, and Godot runs headless with APPDATA and LOCALAPPDATA pointed at a
sandbox, so even an autoload that saves touches nothing of the designer's. The Godot repo's `git status` is checked
before and after.

Doubles leave Godot as the encoded Variant (`var_to_bytes(...).hex_encode()`): exact, but Godot encodes a float in 32
bits when that loses nothing (flag bit 16 of the header clear), so the decoder reads the header. They are written as
C hex floats (`float.hex`), which strtod reads back exactly.

Usage:
  py tools/make_godot_fixtures.py                 # regenerate into the repo
  py tools/make_godot_fixtures.py --check         # regenerate into build/godot_fixtures/out and require byte-identity
  py tools/make_godot_fixtures.py --only stats    # one fixture
  py tools/make_godot_fixtures.py --reuse-copy    # keep build/godot_fixtures/project (only when the Godot repo is unchanged)
Godot: --godot-exe (default C:/Program Files/Godot/Godot.exe), --godot (default C:/Games/pawpocalypse).
"""

import argparse
import filecmp
import os
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SCRIPTS = ROOT / "tools" / "godot_fixtures"
WORK = ROOT / "build" / "godot_fixtures"
TEST_DATA = ROOT / "Source" / "PawCore" / "Tests" / "Data"
GENERATED_ON = "2026-10-28"  # the date stamped in the headers; kept fixed so a re-run reproduces the files byte for byte

VARIANT_FLOAT = re.compile(r"^03000[01]00[0-9a-f]{8}([0-9a-f]{8})?$")


def decode(hex_text):
    b = bytes.fromhex(hex_text)
    header = struct.unpack("<I", b[:4])[0]
    if header & 0xFFFF != 3:
        raise ValueError("not a float Variant: " + hex_text)
    return struct.unpack("<d", b[4:12])[0] if (header >> 16) & 1 else struct.unpack("<f", b[4:8])[0]


def godot_status(godot):
    r = subprocess.run(["git", "-C", str(godot), "status", "--porcelain"], capture_output=True, text=True)
    return r.stdout


def copy_project(godot, dest, reuse):
    """A scratch copy WITH .godot (the import cache: without it every texture an autoload touches re-imports)."""
    if not (reuse and (dest / "project.godot").exists()):
        if dest.exists():
            shutil.rmtree(dest)
        shutil.copytree(godot, dest, ignore=shutil.ignore_patterns(".git"))
    for name in ("amounts_fixture.gd", "combat_fixture.gd", "stats_fixture.gd", "stats_fixture.tscn", "economy_fixture.gd",
                 "economy_fixture.tscn", "cards_fixture.gd", "cards_fixture.tscn", "tree_fixture.gd", "tree_fixture.tscn",
                 "jobs_fixture.gd", "jobs_fixture.tscn", "progress_fixture.gd", "progress_fixture.tscn",
                 "achievements_fixture.gd", "achievements_fixture.tscn", "draw_fixture.gd", "draw_fixture.tscn",
                 "field_rules_fixture.gd", "field_rules_fixture.tscn", "kittens_fixture.gd", "kittens_fixture.tscn",
                 "aura_fixture.gd", "aura_fixture.tscn", "pounce_fixture.gd", "pounce_fixture.tscn",
                 "extras_fixture.gd", "extras_fixture.tscn", "boss_fixture.gd", "boss_fixture.tscn",
                 "tree_layout_fixture.gd", "tree_layout_fixture.tscn", "tree_text_fixture.gd", "tree_text_fixture.tscn",
                 "job_picker_fixture.gd", "job_picker_fixture.tscn", "victory_fixture.gd", "victory_fixture.tscn",
                 "save_fixture.gd", "save_fixture.tscn",
                 "amounts_values.txt"):
        shutil.copy2(SCRIPTS / name, dest / name)


def run_godot(exe, project, args, marker, log_path):
    env = dict(os.environ)
    sandbox = WORK / "appdata"
    for sub in ("Roaming", "Local"):
        (sandbox / sub).mkdir(parents=True, exist_ok=True)
    env["APPDATA"] = str(sandbox / "Roaming")
    env["LOCALAPPDATA"] = str(sandbox / "Local")
    cmd = [str(exe), "--headless", "--path", str(project)] + args
    r = subprocess.run(cmd, capture_output=True, text=True, env=env, timeout=1800)
    log_path.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
    if marker not in r.stdout:
        raise SystemExit("Godot did not finish (%s missing); log: %s" % (marker, log_path))


def convert_amounts(raw, out_dir):
    rows = []
    for line in open(raw, encoding="utf-8"):
        h, en, zz = line.rstrip("\n").split("\t")
        rows.append((decode(h), en, zz))
    rows.sort(key=lambda t: t[0])
    with open(out_dir / "amounts_godot.tsv", "w", encoding="utf-8", newline="\n") as f:
        f.write("# Godot 4.7.1 Amounts.short (scripts/systems/amounts.gd) ground truth, generated %s by running the Godot code.\n" % GENERATED_ON)
        f.write("# columns: exact double (C hex float), English, 10000-grouping with comma decimal (suffixes W Y Z J G X)\n")
        for v, en, zz in rows:
            f.write("%s\t%s\t%s\n" % (float.hex(v), en, zz))
    return len(rows)


def convert_cell(cell):
    """One encoded double, or a `;`-joined list of them (every part must be one), to C hex floats; anything else as is."""
    if VARIANT_FLOAT.match(cell):
        return float.hex(decode(cell))
    parts = cell.split(";")
    if len(parts) > 1 and all(VARIANT_FLOAT.match(p) for p in parts):
        return ";".join(float.hex(decode(p)) for p in parts)
    return cell


def convert_tokens(cell):
    """Every encoded double anywhere in a cell built of `|`, `,`, `;`, `:` and `=` separated parts (op logs, digests)."""
    return "".join(float.hex(decode(t)) if VARIANT_FLOAT.match(t) else t for t in re.split(r"([|,;:=])", cell))


def convert_table(raw, path, comment_lines, take, tokens=False):
    lines = open(raw, encoding="utf-8").read().splitlines()
    header = lines[0][2:].split("\t")  # the Godot side writes its header as "# col\tcol..."
    rows = lines[1:] if take is None else lines[1:1 + take]
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        for c in comment_lines:
            f.write(c + "\n")
        f.write("\t".join(header) + "\n")
        for line in rows:
            cells = line.split("\t")
            if len(cells) != len(header):
                raise SystemExit("%s: a row has %d cells, the header %d" % (raw, len(cells), len(header)))
            f.write("\t".join((convert_tokens if tokens else convert_cell)(c) for c in cells) + "\n")
    return len(rows)


COMBAT_HEAD = ("# Godot 4.7.1 CombatResolver ground truth (scripts/autoload/combat_resolver.gd + shipping combat_constants.tres), generated %s\n"
               "# by running the Godot code on random CombatStats; exact doubles as C hex floats. %s")
CARDS_HEAD = ("# Godot 4.7.1 CardManager ground truth (the real autoloads, scratch copy of the project), generated\n"
              "# %s on random states; doubles as C hex floats. %s")
ECONOMY_HEAD = ("# Godot 4.7.1 GameState economy ground truth (the real autoloads, scratch copy of the project), generated\n"
                "# %s on random states; doubles as C hex floats. %s")
TREE_HEAD = ("# Godot 4.7.1 SkillTreeManager ground truth (the real autoloads, scratch copy of the project), generated\n"
             "# %s on random states; doubles as C hex floats. %s")
JOB_PICKER_HEAD = ("# Godot 4.7.1 JobPicker ground truth (the real job_picker.gd and autoloads, scratch copy of the project), generated\n"
                   "# %s on random states; doubles as C hex floats. %s")
JOBS_HEAD = ("# Godot 4.7.1 GameState job functions ground truth (the real autoloads, scratch copy of the project), generated\n"
             "# %s on random states; doubles as C hex floats. %s")
PROGRESS_HEAD = ("# Godot 4.7.1 GameState mutations ground truth (the real autoloads, scratch copy of the project), generated\n"
                 "# %s on random states, 12 random operations each; doubles as C hex floats. %s")
ACHIEVEMENTS_HEAD = ("# Godot 4.7.1 AchievementManager ground truth (the real autoloads, scratch copy of the project), generated\n"
                     "# %s on random states, 10 random operations each; doubles as C hex floats. %s")
FIELD_RULES_HEAD = ("# Godot 4.7.1 field rules ground truth (GameState field getters, TierData.pick_enemy, TierController.playfield_rect,\n"
                    "# a real Enemy node's activate/take_damage), generated %s on random states; doubles as C hex floats. %s")
KITTENS_HEAD = ("# Godot 4.7.1 kitten rules ground truth (GameState kitten getters, the pool size, a real Kitten node's orbit slot,\n"
                "# _pick_target, _is_overcrowded and one hunt() over real Enemy nodes), generated %s on random states; doubles as C hex floats. %s")
AURA_HEAD = ("# Godot 4.7.1 aura and zone rules ground truth (GameState aura getters, aura card values, aura/poop/player DPS,\n"
             "# a real Enemy node's apply_slow), generated %s on random states; doubles as C hex floats. %s")
POUNCE_HEAD = ("# Godot 4.7.1 Pounce rules ground truth (GameState Pounce getters, the Pounce cards' values, pounce cooldown and\n"
               "# damage), generated %s on random states; doubles as C hex floats. %s")
EXTRAS_HEAD = ("# Godot 4.7.1 paw/kill extras and Prism ground truth (GameState getters, Catnip Bomb and Scavenge card values),\n"
               "# generated %s on random states; doubles as C hex floats. %s")
BOSS_HEAD = ("# Godot 4.7.1 Boss node ground truth (take_damage, heal, the shield cycle), generated %s on random bosses\n"
             "# and operations; doubles as C hex floats. %s")
DRAW_HEAD = ("# Godot 4.7.1 CardManager deal ground truth (the real autoloads, scratch copy of the project), generated\n"
             "# %s on random states, 8 random seeded operations each; doubles as C hex floats. %s")
TREE_LAYOUT_HEAD = ("# Godot 4.7.1 SkillTreeUI layout ground truth (skill_tree_ui.gd, the script without its scene, scratch copy of\n"
                    "# the project), generated %s: row 0 the shipping tree, then random trees; doubles as C hex floats. %s")
TREE_TEXT_HEAD = ("# Godot 4.7.1 SkillTreeUI effect wording ground truth (skill_tree_ui.gd _effect_text and its number formats, the\n"
                  "# script without its scene, English), generated %s; doubles as C hex floats. %s")
VICTORY_HEAD = ("# Godot 4.7.1 VictoryScreen ground truth (the real hud.tscn, victory_screen.gd and autoloads, scratch copy of the\n"
                "# project), generated %s on random states; doubles as C hex floats. %s")
SAVE_HEAD = ("# Godot 4.7.1 SaveManager ground truth (the real save_manager.gd save_game() / load_game() and autoloads, scratch copy\n"
             "# of the project, sandboxed user://), generated %s by tools/godot_fixtures/save_fixture.gd. %s")
STATS_HEAD = ("# Godot 4.7.1 GameState._rebuild_stats() ground truth (the real autoloads, scratch copy of the project), generated\n"
              "# %s on random progress states; doubles as C hex floats. %s")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot", default="C:/Games/pawpocalypse")
    ap.add_argument("--godot-exe", default="C:/Program Files/Godot/Godot.exe")
    ap.add_argument("--only", choices=["amounts", "combat", "stats", "economy", "cards", "tree", "jobs", "progress", "achievements", "draw", "field_rules", "kittens", "aura", "pounce", "extras", "boss", "tree_layout", "tree_text", "job_picker", "victory", "save"], action="append")
    ap.add_argument("--reuse-copy", action="store_true")
    ap.add_argument("--check", action="store_true", help="write to build/godot_fixtures/out and require byte-identity")
    args = ap.parse_args()
    which = args.only or ["amounts", "combat", "stats", "economy", "cards", "tree", "jobs", "progress", "achievements", "draw", "field_rules", "kittens", "aura", "pounce", "extras", "boss", "tree_layout", "tree_text", "job_picker", "victory", "save"]
    godot = Path(args.godot)

    before = godot_status(godot)
    project = WORK / "project"
    print("copying %s -> %s (without .git) ..." % (godot, project), flush=True)
    copy_project(godot, project, args.reuse_copy)

    out_tests = WORK / "out" if args.check else TEST_DATA
    out_build = WORK / "out" if args.check else ROOT / "build"
    out_tests.mkdir(parents=True, exist_ok=True)
    out_build.mkdir(parents=True, exist_ok=True)
    written = []

    if "amounts" in which:
        run_godot(args.godot_exe, project, ["--script", "res://amounts_fixture.gd"], "AMOUNTS_FIXTURE", WORK / "amounts.log")
        n = convert_amounts(project / "amounts_fixture.tsv", out_tests)
        written.append(("amounts_godot.tsv", out_tests, n))
    if "combat" in which:
        run_godot(args.godot_exe, project, ["--script", "res://combat_fixture.gd"], "COMBAT_FIXTURE", WORK / "combat.log")
        raw = project / "combat_fixture.tsv"
        n = convert_table(raw, out_tests / "combat_godot.tsv",
                          (COMBAT_HEAD % (GENERATED_ON, "First 2,000 of 5,000 rows (the L1 test).")).split("\n"), 2000)
        written.append(("combat_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "combat_godot_full.tsv",
                          (COMBAT_HEAD % (GENERATED_ON, "All 5,000 rows (the long test).")).split("\n"), None)
        written.append(("combat_godot_full.tsv", out_build, n))
    if "stats" in which:
        # A SCENE, not --script: a --script SceneTree cannot reference autoload identifiers (GameState) at all.
        run_godot(args.godot_exe, project, ["res://stats_fixture.tscn"], "STATS_FIXTURE", WORK / "stats.log")
        raw = project / "stats_fixture.tsv"
        n = convert_table(raw, out_tests / "stats_godot.tsv",
                          (STATS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("stats_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "stats_godot_full.tsv",
                          (STATS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("stats_godot_full.tsv", out_build, n))

    if "economy" in which:
        run_godot(args.godot_exe, project, ["res://economy_fixture.tscn"], "ECONOMY_FIXTURE", WORK / "economy.log")
        raw = project / "economy_fixture.tsv"
        n = convert_table(raw, out_tests / "economy_godot.tsv",
                          (ECONOMY_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("economy_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "economy_godot_full.tsv",
                          (ECONOMY_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("economy_godot_full.tsv", out_build, n))
    if "cards" in which:
        run_godot(args.godot_exe, project, ["res://cards_fixture.tscn"], "CARDS_FIXTURE", WORK / "cards.log")
        raw = project / "cards_fixture.tsv"
        n = convert_table(raw, out_tests / "cards_godot.tsv",
                          (CARDS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("cards_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "cards_godot_full.tsv",
                          (CARDS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("cards_godot_full.tsv", out_build, n))
    if "tree" in which:
        run_godot(args.godot_exe, project, ["res://tree_fixture.tscn"], "TREE_FIXTURE", WORK / "tree.log")
        raw = project / "tree_fixture.tsv"
        n = convert_table(raw, out_tests / "tree_godot.tsv",
                          (TREE_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("tree_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "tree_godot_full.tsv",
                          (TREE_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("tree_godot_full.tsv", out_build, n))

    if "tree_layout" in which:
        run_godot(args.godot_exe, project, ["res://tree_layout_fixture.tscn"], "TREE_LAYOUT_FIXTURE", WORK / "tree_layout.log")
        raw = project / "tree_layout_fixture.tsv"
        n = convert_table(raw, out_tests / "tree_layout_godot.tsv",
                          (TREE_LAYOUT_HEAD % (GENERATED_ON, "All 401 trees.")).split("\n"), None)
        written.append(("tree_layout_godot.tsv", out_tests, n))

    if "tree_text" in which:
        run_godot(args.godot_exe, project, ["res://tree_text_fixture.tscn"], "TREE_TEXT_FIXTURE", WORK / "tree_text.log")
        raw = project / "tree_text_fixture.tsv"
        n = convert_table(raw, out_tests / "tree_text_godot.tsv",
                          (TREE_TEXT_HEAD % (GENERATED_ON, "Every branch and level; 2,000 values x 9 formats.")).split("\n"), None)
        written.append(("tree_text_godot.tsv", out_tests, n))

    if "jobs" in which:
        run_godot(args.godot_exe, project, ["res://jobs_fixture.tscn"], "JOBS_FIXTURE", WORK / "jobs.log")
        raw = project / "jobs_fixture.tsv"
        n = convert_table(raw, out_tests / "jobs_godot.tsv",
                          (JOBS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("jobs_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "jobs_godot_full.tsv",
                          (JOBS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("jobs_godot_full.tsv", out_build, n))
    if "job_picker" in which:
        run_godot(args.godot_exe, project, ["res://job_picker_fixture.tscn"], "JOB_PICKER_FIXTURE", WORK / "job_picker.log")
        raw = project / "job_picker_fixture.tsv"
        n = convert_table(raw, out_tests / "job_picker_godot.tsv",
                          (JOB_PICKER_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("job_picker_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "job_picker_godot_full.tsv",
                          (JOB_PICKER_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("job_picker_godot_full.tsv", out_build, n))
    if "victory" in which:
        run_godot(args.godot_exe, project, ["res://victory_fixture.tscn"], "VICTORY_FIXTURE done", WORK / "victory.log")
        raw = project / "victory_fixture.tsv"
        n = convert_table(raw, out_tests / "victory_godot.tsv",
                          (VICTORY_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("victory_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "victory_godot_full.tsv",
                          (VICTORY_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("victory_godot_full.tsv", out_build, n))
        # The screen's resolved look (the engine's default theme reaches it, not the game's): the LOOK lines of the log, with
        # the one non-deterministic part (an object id) cut. tools/gen_paw_tokens.py reads its colours from this file.
        look = []
        for line in (WORK / "victory.log").read_text(encoding="utf-8").splitlines():
            if line.startswith("LOOK "):
                look.append(re.sub(r":<(\w+)#\d+>", r":<\1>", line[5:]))
        with open(out_tests / "victory_look_godot.txt", "w", encoding="utf-8", newline="\n") as f:
            f.write("# Godot 4.7.1 VictoryScreen as drawn (the real hud.tscn, headless 1920x1920), generated %s by\n" % GENERATED_ON)
            f.write("# tools/godot_fixtures/victory_fixture.gd. Colours are Godot Color() values (sRGB floats).\n")
            f.write("\n".join(look) + "\n")
        written.append(("victory_look_godot.txt", out_tests, len(look)))
    if "save" in which:
        run_godot(args.godot_exe, project, ["res://save_fixture.tscn"], "SAVE_FIXTURE", WORK / "save.log")
        raw = project / "save_fixture.tsv"
        n = convert_table(raw, out_tests / "save_godot.tsv",
                          (SAVE_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("save_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "save_godot_full.tsv",
                          (SAVE_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("save_godot_full.tsv", out_build, n))
    if "progress" in which:
        run_godot(args.godot_exe, project, ["res://progress_fixture.tscn"], "PROGRESS_FIXTURE", WORK / "progress.log")
        raw = project / "progress_fixture.tsv"
        n = convert_table(raw, out_tests / "progress_godot.tsv",
                          (PROGRESS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("progress_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "progress_godot_full.tsv",
                          (PROGRESS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("progress_godot_full.tsv", out_build, n))
    if "achievements" in which:
        run_godot(args.godot_exe, project, ["res://achievements_fixture.tscn"], "ACHIEVEMENTS_FIXTURE", WORK / "achievements.log")
        raw = project / "achievements_fixture.tsv"
        n = convert_table(raw, out_tests / "achievements_godot.tsv",
                          (ACHIEVEMENTS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("achievements_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "achievements_godot_full.tsv",
                          (ACHIEVEMENTS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("achievements_godot_full.tsv", out_build, n))
    if "draw" in which:
        run_godot(args.godot_exe, project, ["res://draw_fixture.tscn"], "DRAW_FIXTURE", WORK / "draw.log")
        raw = project / "draw_fixture.tsv"
        n = convert_table(raw, out_tests / "draw_godot.tsv",
                          (DRAW_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("draw_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "draw_godot_full.tsv",
                          (DRAW_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("draw_godot_full.tsv", out_build, n))

    if "field_rules" in which:
        run_godot(args.godot_exe, project, ["res://field_rules_fixture.tscn"], "FIELD_RULES_FIXTURE", WORK / "field_rules.log")
        raw = project / "field_rules_fixture.tsv"
        n = convert_table(raw, out_tests / "field_rules_godot.tsv",
                          (FIELD_RULES_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("field_rules_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "field_rules_godot_full.tsv",
                          (FIELD_RULES_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("field_rules_godot_full.tsv", out_build, n))

    if "kittens" in which:
        run_godot(args.godot_exe, project, ["res://kittens_fixture.tscn"], "KITTENS_FIXTURE", WORK / "kittens.log")
        raw = project / "kittens_fixture.tsv"
        n = convert_table(raw, out_tests / "kittens_godot.tsv",
                          (KITTENS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("kittens_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "kittens_godot_full.tsv",
                          (KITTENS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("kittens_godot_full.tsv", out_build, n))

    if "aura" in which:
        run_godot(args.godot_exe, project, ["res://aura_fixture.tscn"], "AURA_FIXTURE", WORK / "aura.log")
        raw = project / "aura_fixture.tsv"
        n = convert_table(raw, out_tests / "aura_godot.tsv",
                          (AURA_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("aura_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "aura_godot_full.tsv",
                          (AURA_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("aura_godot_full.tsv", out_build, n))

    if "pounce" in which:
        run_godot(args.godot_exe, project, ["res://pounce_fixture.tscn"], "POUNCE_FIXTURE", WORK / "pounce.log")
        raw = project / "pounce_fixture.tsv"
        n = convert_table(raw, out_tests / "pounce_godot.tsv",
                          (POUNCE_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("pounce_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "pounce_godot_full.tsv",
                          (POUNCE_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("pounce_godot_full.tsv", out_build, n))

    if "extras" in which:
        run_godot(args.godot_exe, project, ["res://extras_fixture.tscn"], "EXTRAS_FIXTURE", WORK / "extras.log")
        raw = project / "extras_fixture.tsv"
        n = convert_table(raw, out_tests / "extras_godot.tsv",
                          (EXTRAS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("extras_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "extras_godot_full.tsv",
                          (EXTRAS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("extras_godot_full.tsv", out_build, n))

    if "boss" in which:
        run_godot(args.godot_exe, project, ["res://boss_fixture.tscn"], "BOSS_FIXTURE", WORK / "boss.log")
        raw = project / "boss_fixture.tsv"
        n = convert_table(raw, out_tests / "boss_godot.tsv",
                          (BOSS_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000, tokens=True)
        written.append(("boss_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "boss_godot_full.tsv",
                          (BOSS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None, tokens=True)
        written.append(("boss_godot_full.tsv", out_build, n))

    after = godot_status(godot)
    if after != before:
        print("FAIL  the Godot repo's git status changed during the run:\n" + after)
        return 1
    for name, folder, n in written:
        print("wrote %-24s %6d rows -> %s" % (name, n, folder))

    if args.check:
        bad = 0
        for name, folder, n in written:
            ref = (TEST_DATA if not name.endswith("_full.tsv") else ROOT / "build") / name
            same = ref.exists() and filecmp.cmp(ref, folder / name, shallow=False)
            print("%s  %s" % ("SAME" if same else "DIFF", name))
            bad += 0 if same else 1
        if bad:
            print("FAIL  %d fixture(s) do not reproduce" % bad)
            return 1
        print("PASS  every fixture reproduces byte for byte from the Godot code")
    return 0


if __name__ == "__main__":
    sys.exit(main())
