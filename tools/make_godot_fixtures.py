"""Regenerate PawCore's Godot ground-truth fixtures by running the REAL Godot code.

The scratch copy gets the port's fixes to Godot's faults first (D16, tools/godot_fixes.py); the Godot repo is only read.

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
  title    the real title screen (main_menu.tscn): Continue / Load, the status line, the five slot cards, the leaderboard,
           the default selection, and format_when / format_duration / _kills_text, 3,000 states
           -> Tests/Data/title_godot.tsv (first 1,000); its look -> Tests/Data/title_look_godot.txt
  pause    the real pause menu (pause_menu.tscn): 2,000 rows x 13 presses (open, close, the Save page, a slot's two-press
           overwrite, the quit button's first press) and what it shows after each -> Tests/Data/pause_godot.tsv (first 600)
  settings the real SettingsManager reading a settings.cfg Godot wrote, and the real settings dialog showing it: 3,000 rows
           x 6 changes (sliders, boxes, lists, the section) -> Tests/Data/settings_godot.tsv (first 1,000); fit_to_screen
           and centred_position, 3,000 cases -> Tests/Data/settings_fit_godot.tsv
  input_axis what Godot's own Input makes of a stick / trigger value for every action bound to an axis (pressed, strength,
           raw strength), 409 values each -> Tests/Data/input_axis_godot.tsv (the engine test Pawpocalypse.Input.HalfAxis)
  input_glyphs the real InputGlyphs: every action's prompt in every family, the keycap label of every key, the {token}
           replacement of the game's prompt templates -> Tests/Data/input_glyphs_godot.tsv, input_keys_godot.tsv,
           input_prompts_godot.tsv (the engine test Pawpocalypse.Input.Glyphs)
  input_device the real InputDevice autoload's observe() (prompt family, cursor source, warp echo, press guard, toasts),
           700 rows x 10 steps with real time between some -> Tests/Data/input_device_godot.tsv; family_from_info, 3,000
           cases -> Tests/Data/input_family_godot.tsv
  input_bindings the real InputBindings (input_bindings.gd: rebind, swaps, refusals, reset_row, the saved diff, apply()
           with the swaps and the dead zone, binding_from) and settings.cfg's [bindings] (save / load, hand edits):
           3,000 rows x 8 ops -> Tests/Data/input_bindings_godot.tsv (first 300), input_binding_from_godot.tsv,
           input_bindings_cfg_godot.tsv (first 200), input_bindings_defaults_godot.tsv
  controls the real Controls page (controls_panel.gd in the real settings dialog): 1,500 rows from a recorded start x 12
           moves (a cell listening, events, the timeout, Reset, Reset all, the switches, the sliders, the prompt style, a
           direct bind) and the page after each -> Tests/Data/controls_godot.tsv (first 500), controls_words_godot.tsv
  backdrop the real BiomeBackdrop (biome_backdrop.gd): every prop and patch / puddle outline its _draw() makes, for the 12
           biomes and the title's garden at 7 viewport sizes and 4 effects budgets -> Tests/Data/backdrop_godot.tsv
  rainbow  the real rainbow_bar.gd, rainbow_frame.gd and skill_tree_sky.gd, compiled from their own source with only their
           draw calls (and the bar's global random draws) renamed to recorders: 30 bars x 20 frames, 40 frames x 10
           frames, 200 follows, the sky at 4 sizes x 4 budgets x 10 frames -> Tests/Data/rainbow_bar_godot.tsv,
           rainbow_frame_godot.tsv, rainbow_follow_godot.tsv, sky_layout_godot.tsv, sky_frames_godot.tsv
  unicorn  the real unicorn_ceremony.gd recompiled with its draw calls, random draws and sounds renamed to recorders, its
           tweens stepped by custom_step: 16 runs at 4 sizes, presses at random frames -> Tests/Data/unicorn_godot.tsv
  dock_repeat the real DockNavigator of the real HUD: hold-to-buy and the held row step, frame by frame on Godot's own action
           state, 600 sequences x 40 frames -> Tests/Data/dock_repeat_godot.tsv, dock_repeat_rows_godot.tsv
  juice    Godot's Tween easing (Tween.interpolate_value: linear, sine, quad, back, elastic x in, out, in_out, out_in) and the
           real UiJuice (ui_juice.gd) on a real Button, 160 sequences x 60 events / frames stepped by Tween.custom_step
           -> Tests/Data/juice_curves_godot.tsv, juice_seq_godot.tsv, juice_leans_godot.tsv
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
import godot_fixes  # tools/godot_fixes.py: the port's fixes to Godot's code (D16)
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
                 "save_fixture.gd", "save_fixture.tscn", "title_fixture.gd", "title_fixture.tscn", "pause_fixture.gd", "pause_fixture.tscn",
                 "settings_fixture.gd", "settings_fixture.tscn",
                 "input_axis_fixture.gd", "input_axis_fixture.tscn",
                 "input_device_fixture.gd", "input_device_fixture.tscn",
                 "input_glyphs_fixture.gd", "input_glyphs_fixture.tscn",
                 "input_bindings_fixture.gd", "input_bindings_fixture.tscn",
                 "controls_fixture.gd", "controls_fixture.tscn",
                 "dock_repeat_fixture.gd", "dock_repeat_fixture.tscn",
                 "backdrop_fixture.gd", "backdrop_fixture.tscn",
                 "juice_fixture.gd", "juice_fixture.tscn",
                 "rainbow_fixture.gd", "rainbow_fixture.tscn",
                 "unicorn_fixture.gd", "unicorn_fixture.tscn",
                 "amounts_values.txt"):
        shutil.copy2(SCRIPTS / name, dest / name)
    # The port's fixes to Godot's code (D16, tools/godot_fixes.py), on this scratch copy only: a fixture then shows Godot's
    # behaviour with the faults the port does not copy fixed, and every other row is still Godot's own.
    applied = godot_fixes.apply_all(dest)
    if applied:
        print("the port's fixes applied to the scratch copy: %s" % ", ".join(applied), flush=True)


def run_godot(exe, project, args, marker, log_path):
    env = dict(os.environ)
    sandbox = WORK / "appdata"
    # A fresh user:// for every run: a fixture that saves (settings.cfg, slots) must not leak into the next one. The settings
    # fixture's last random settings.cfg once set the input fixture's stick dead zone (2026-11-04).
    shutil.rmtree(sandbox, ignore_errors=True)
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
TITLE_HEAD = ("# Godot 4.7.1 title screen ground truth (the real main_menu.tscn, main_menu.gd, pause_menu.gd formatters and the\n"
              "# SaveManager slot files, scratch copy of the project, sandboxed user://), generated %s by\n"
              "# tools/godot_fixtures/title_fixture.gd; doubles as C hex floats; texts with \\n for a line break. %s")
PAUSE_HEAD = ("# Godot 4.7.1 pause menu ground truth (the real pause_menu.tscn / pause_menu.gd and the SaveManager slot files,\n"
              "# scratch copy of the project, sandboxed user://), generated %s by tools/godot_fixtures/pause_fixture.gd;\n"
              "# one line per press; texts with \\n for a line break. %s")
SETTINGS_HEAD = ("# Godot 4.7.1 settings ground truth (the real settings_manager.gd reading a settings.cfg written by Godot's ConfigFile,\n"
                 "# and the real settings_dialog.tscn / settings_dialog.gd showing it; scratch copy of the project, sandboxed user://),\n"
                 "# generated %s by tools/godot_fixtures/settings_fixture.gd; one line per load or change; doubles as C hex\n"
                 "# floats; texts with \\n for a line break. %s")
SAVE_HEAD = ("# Godot 4.7.1 SaveManager ground truth (the real save_manager.gd save_game() / load_game() and autoloads, scratch copy\n"
             "# of the project, sandboxed user://), generated %s by tools/godot_fixtures/save_fixture.gd. %s")
STATS_HEAD = ("# Godot 4.7.1 GameState._rebuild_stats() ground truth (the real autoloads, scratch copy of the project), generated\n"
              "# %s on random progress states; doubles as C hex floats. %s")


# Fixtures that record the WALL CLOCK (the title screen reads "now" for "last saved"): their clock columns cannot reproduce;
# every other cell must, byte for byte. The texts made FROM the clock ("2 min ago") are compared: they reproduce, because the
# fixture stamps each save at now minus an offset; a calendar date in them (a save older than a week) is compared as a date.
WALL_CLOCK = {"title_godot": ("_now", "_unix")}
# Fixtures whose steps wait REAL time (input_device: the mouse-quiet and warp-echo windows): a re-run can fall the other
# side of a window, so they are re-run but not compared. Their L1 test replays the clock each step recorded.
REAL_TIME = {"input_device_godot.tsv"}
DATE = re.compile(r"\d{4}-\d{2}-\d{2}")


# InputGlyphs.bbcode's markup, neutralised: an inline glyph is <g:stem>, a keycap <k:label> (PawGlyphs::Neutral).
BB_GLYPH = re.compile(r"\[img=\d+x\d+\][^\[]*/(input_[a-z_]+)\.svg\[/img\]")
BB_KEYCAP = re.compile(r"\[bgcolor=#[0-9a-fA-F]+\]\[color=#[0-9a-fA-F]+\]\[b\] (.*?) \[/b\]\[/color\]\[/bgcolor\]")


def write_glyph_tables(project, out_tests):
    sys.path.insert(0, str(ROOT / "tools"))
    from export_input_map import KEYS
    head = ("# Godot 4.7.1 InputGlyphs ground truth (scripts/systems/input_glyphs.gd, the real project.godot [input]), generated %s"
            % GENERATED_ON)
    written = []
    # Only the actions the port maps (Data/InputMap.csv): Godot's own ui_* editor actions (copy, file dialogs) are not the game's.
    import csv
    with open(ROOT / "Data" / "InputMap.csv", encoding="utf-8") as f:
        ours = {r["action"] for r in csv.DictReader(f)}
    raw = project / "input_glyphs_fixture.tsv"
    lines = open(raw, encoding="utf-8").read().splitlines()
    kept = [lines[0]] + [l for l in lines[1:] if l.split("\t")[0] in ours]
    if len({l.split("\t")[0] for l in kept[1:]}) != len(ours):
        raise SystemExit("input_glyphs: Godot has %s, the port maps %s" % (sorted({l.split("\t")[0] for l in kept[1:]}), sorted(ours)))
    open(raw, "w", encoding="utf-8", newline="\n").write("\n".join(kept) + "\n")
    n = convert_table(project / "input_glyphs_fixture.tsv", out_tests / "input_glyphs_godot.tsv",
                      [head, "# by tools/godot_fixtures/input_glyphs_fixture.gd. Family: 0 keyboard, 1 Xbox, 2 PlayStation, 3 Steam Deck,",
                       "# 4 generic. kind: none (no binding on that device), glyph (an image), keycap (a key drawn around its label),",
                       "# blank (a binding with no picture: shown as nothing)."], None)
    written.append(("input_glyphs_godot.tsv", out_tests, n))
    lines = open(project / "input_keys_fixture.tsv", encoding="utf-8").read().splitlines()
    with open(out_tests / "input_keys_godot.tsv", "w", encoding="utf-8", newline="\n") as f:
        f.write(head + "\n# by tools/godot_fixtures/input_glyphs_fixture.gd: InputGlyphs.key_label per physical keycode (headless: the US\n"
                "# label); ue_key = tools/export_input_map.py KEYS (blank: a key the port does not bind yet).\n")
        f.write("keycode\tlabel\tue_key\n")
        for line in lines[1:]:
            code, label = line.split("\t")
            f.write("%s\t%s\t%s\n" % (code, label, KEYS.get(int(code), "")))
    written.append(("input_keys_godot.tsv", out_tests, len(lines) - 1))
    lines = open(project / "input_prompts_fixture.tsv", encoding="utf-8").read().splitlines()
    with open(out_tests / "input_prompts_godot.tsv", "w", encoding="utf-8", newline="\n") as f:
        f.write(head + "\n# by tools/godot_fixtures/input_glyphs_fixture.gd: InputGlyphs.bbcode(template, family), the BBCode neutralised:\n"
                "# an inline glyph is <g:stem>, a keycap <k:label>.\n")
        f.write("id\tfamily\ttemplate\tprompt\n")
        for line in lines[1:]:
            pid, family, template, bb = line.split("\t")
            neutral = BB_KEYCAP.sub(lambda m: "<k:%s>" % m.group(1), BB_GLYPH.sub(lambda m: "<g:%s>" % m.group(1), bb))
            if "[" in neutral and "[" not in template:
                raise SystemExit("input_prompts: BBCode left over in %r" % neutral)
            f.write("%s\t%s\t%s\t%s\n" % (pid, family, template, neutral))
    written.append(("input_prompts_godot.tsv", out_tests, len(lines) - 1))
    return written


def same_but_clock(ref, new, suffixes):
    a = open(ref, encoding="utf-8").read().splitlines()
    b = open(new, encoding="utf-8").read().splitlines()
    if len(a) != len(b):
        return False
    header = None
    for x, y in zip(a, b):
        if x.startswith("#"):
            if x != y:
                return False
            continue
        if header is None:
            header = x.split("\t")
            if x != y:
                return False
            continue
        xs, ys = x.split("\t"), y.split("\t")
        if len(xs) != len(ys):
            return False
        for col, p, q in zip(header, xs, ys):
            # A save older than a week shows its calendar date: the date of (now - offset) moves with the day of the run.
            if p != q and not col.endswith(suffixes) and DATE.sub("<date>", p) != DATE.sub("<date>", q):
                return False
    return True


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--godot", default="C:/Games/pawpocalypse")
    ap.add_argument("--godot-exe", default="C:/Program Files/Godot/Godot.exe")
    ap.add_argument("--only", choices=["amounts", "combat", "stats", "economy", "cards", "tree", "jobs", "progress", "achievements", "draw", "field_rules", "kittens", "aura", "pounce", "extras", "boss", "tree_layout", "tree_text", "job_picker", "victory", "save", "title", "pause", "settings", "input_axis", "input_device", "input_glyphs", "input_bindings", "controls", "dock_repeat", "backdrop", "juice", "rainbow", "unicorn"], action="append")
    ap.add_argument("--reuse-copy", action="store_true")
    ap.add_argument("--check", action="store_true", help="write to build/godot_fixtures/out and require byte-identity")
    args = ap.parse_args()
    which = args.only or ["amounts", "combat", "stats", "economy", "cards", "tree", "jobs", "progress", "achievements", "draw", "field_rules", "kittens", "aura", "pounce", "extras", "boss", "tree_layout", "tree_text", "job_picker", "victory", "save", "title", "pause", "settings", "input_axis", "input_device", "input_glyphs", "input_bindings", "controls", "dock_repeat", "backdrop", "juice", "rainbow", "unicorn"]
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
    if "title" in which:
        run_godot(args.godot_exe, project, ["res://title_fixture.tscn"], "TITLE_FIXTURE done", WORK / "title.log")
        raw = project / "title_fixture.tsv"
        n = convert_table(raw, out_tests / "title_godot.tsv",
                          (TITLE_HEAD % (GENERATED_ON, "First 1,000 of 3,000 rows (the L1 test).")).split("\n"), 1000)
        written.append(("title_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "title_godot_full.tsv",
                          (TITLE_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("title_godot_full.tsv", out_build, n))
        look = []
        for line in (WORK / "title.log").read_text(encoding="utf-8").splitlines():
            if line.startswith("LOOK "):
                look.append(re.sub(r":<(\w+)#\d+>", r":<\1>", line[5:]))
        with open(out_tests / "title_look_godot.txt", "w", encoding="utf-8", newline="\n") as f:
            f.write("# Godot 4.7.1 title screen as drawn (the real main_menu.tscn, headless), generated %s by\n" % GENERATED_ON)
            f.write("# tools/godot_fixtures/title_fixture.gd. Colours are Godot Color() values (sRGB floats).\n")
            f.write("\n".join(look) + "\n")
        written.append(("title_look_godot.txt", out_tests, len(look)))
    if "pause" in which:
        run_godot(args.godot_exe, project, ["res://pause_fixture.tscn"], "PAUSE_FIXTURE done", WORK / "pause.log")
        raw = project / "pause_fixture.tsv"
        n = convert_table(raw, out_tests / "pause_godot.tsv",
                          (PAUSE_HEAD % (GENERATED_ON, "The first 600 rows' presses (the L1 test).")).split("\n"), 600 * 13)
        written.append(("pause_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "pause_godot_full.tsv",
                          (PAUSE_HEAD % (GENERATED_ON, "All 2,000 rows (the long test).")).split("\n"), None)
        written.append(("pause_godot_full.tsv", out_build, n))
    if "settings" in which:
        run_godot(args.godot_exe, project, ["res://settings_fixture.tscn"], "SETTINGS_FIXTURE done", WORK / "settings.log")
        raw = project / "settings_fixture.tsv"
        n = convert_table(raw, out_tests / "settings_godot.tsv",
                          (SETTINGS_HEAD % (GENERATED_ON, "The first 1,000 rows (the L1 test).")).split("\n"), 1000 * 7)
        written.append(("settings_godot.tsv", out_tests, n))
        n = convert_table(raw, out_build / "settings_godot_full.tsv",
                          (SETTINGS_HEAD % (GENERATED_ON, "All 3,000 rows (the long test).")).split("\n"), None)
        written.append(("settings_godot_full.tsv", out_build, n))
        n = convert_table(project / "settings_fit_fixture.tsv", out_tests / "settings_fit_godot.tsv",
                          ["# Godot 4.7.1 SettingsManager.fit_to_screen / centred_position ground truth, generated %s by" % GENERATED_ON,
                           "# tools/godot_fixtures/settings_fixture.gd: target t, usable u, decorations d -> fitted f; screen p/s -> centred c."],
                          None)
        written.append(("settings_fit_godot.tsv", out_tests, n))
    if "backdrop" in which:
        run_godot(args.godot_exe, project, ["res://backdrop_fixture.tscn"], "BACKDROP_FIXTURE done", WORK / "backdrop.log")
        n = convert_table(project / "backdrop_fixture.tsv", out_tests / "backdrop_godot.tsv",
                          ["# Godot 4.7.1 BiomeBackdrop ground truth (the real biome_backdrop.gd in a SubViewport of each size, every draw it",
                           "# makes recorded through a probe subclass: _blob and _draw_swaying), generated %s by" % GENERATED_ON,
                           "# tools/godot_fixtures/backdrop_fixture.gd; doubles as C hex floats. blobs: at_x,at_y,radius,x0:y0:...;",
                           "# props: kind (s scatter, l landmark, b border, e edge),stem,x,y,w,h,sway,phase,hz (w < 0: mirrored)."], None, tokens=True)
        written.append(("backdrop_godot.tsv", out_tests, n))
    if "juice" in which:
        run_godot(args.godot_exe, project, ["res://juice_fixture.tscn"], "JUICE_FIXTURE done", WORK / "juice.log")
        head = ["# Godot 4.7.1 UI motion ground truth (Tween.interpolate_value, and the real scripts/systems/ui_juice.gd on a real Button,",
                "# its tweens stepped by Tween.custom_step), generated %s by tools/godot_fixtures/juice_fixture.gd; doubles as C hex floats." % GENERATED_ON]
        n = convert_table(project / "juice_curves.tsv", out_tests / "juice_curves_godot.tsv",
                          head + ["# Every easing UiJuice uses (+ linear) x every ease, a 1 s tween from 0 by 1 at 401 points and UiJuice's hover at 41."], None)
        written.append(("juice_curves_godot.tsv", out_tests, n))
        n = convert_table(project / "juice_seq.tsv", out_tests / "juice_seq_godot.tsv",
                          head + ["# Random sequences of UiJuice's events and frames; after each line the Button's scale and rotation (radians)."], None)
        written.append(("juice_seq_godot.tsv", out_tests, n))
        n = convert_table(project / "juice_leans.tsv", out_tests / "juice_leans_godot.tsv",
                          head + ["# Each jitter step's lean (degrees, the global randf_range's draw), in order per sequence."], None)
        written.append(("juice_leans_godot.tsv", out_tests, n))
    if "rainbow" in which:
        run_godot(args.godot_exe, project, ["res://rainbow_fixture.tscn"], "RAINBOW_FIXTURE done", WORK / "rainbow.log")
        head = ["# Godot 4.7.1 ground truth for the rainbow bar, the rainbow frame and the skill tree's sky: the real",
                "# scripts/systems/rainbow_bar.gd, rainbow_frame.gd and skill_tree_sky.gd compiled from their own source with only their",
                "# draw calls (and the bar's global random draws) renamed to recorders, generated %s by" % GENERATED_ON,
                "# tools/godot_fixtures/rainbow_fixture.gd; doubles as C hex floats."]
        for raw, name, what in (
                ("rainbow_bar.tsv", "rainbow_bar_godot.tsv",
                 "# Random bars and frames: the Range's kept value, clock, accumulator, draws taken; sparks x,y,vx,vy,age,hue; bands x,w,r,g,b; circles x,y,radius,r,g,b,a."),
                ("rainbow_frame.tsv", "rainbow_frame_godot.tsv",
                 "# Random frames stepped by hand: the phase, then the one polyline (x:y:r:g:b per point; opaque)."),
                ("rainbow_follow.tsv", "rainbow_follow_godot.tsv",
                 "# A frame following a plate: the plate's global origin, size, lean and shown; where the frame put itself."),
                ("sky_layout.tsv", "sky_layout_godot.tsv",
                 "# The sky's _layout() per viewport size and effects budget: clouds x,y,radius,drift_x,drift_y,hue_period,phase,slow_amp,fast_amp; stars x,y,radius,alpha,period,phase."),
                ("sky_frames.tsv", "sky_frames_godot.tsv",
                 "# Then 10 frames each: the clock, cloud centres x:y, the base rect x,y,w,h,r,g,b,a, stars x,y,radius,r,g,b,a, clouds r:g:b:a:x0:y0:...")):
            n = convert_table(project / raw, out_tests / name, head + [what], None, tokens=True)
            written.append((name, out_tests, n))
    if "unicorn" in which:
        run_godot(args.godot_exe, project, ["res://unicorn_fixture.tscn"], "UNICORN_FIXTURE done", WORK / "unicorn.log")
        n = convert_table(project / "unicorn.tsv", out_tests / "unicorn_godot.tsv",
                          ["# Godot 4.7.1 ground truth for the Unicorn ceremony: the real scripts/systems/unicorn_ceremony.gd compiled from",
                           "# its own source with only its draw calls, global random draws and sounds renamed to recorders, its tweens stepped",
                           "# by custom_step after its _process, presses fed to its _input; generated %s by" % GENERATED_ON,
                           "# tools/godot_fixtures/unicorn_fixture.gd; doubles as C hex floats. One line per frame; bits: every 23rd",
                           "# (x,y,vx,vy,rot,spin,sx,sy,r,g,b,star,rain); arcs cx,cy,r,from,to,n,r,g,b,width,aa; shapes kind:r:g:b:a:width:x0:y0..."],
                          None, tokens=True)
        written.append(("unicorn_godot.tsv", out_tests, n))
    if "input_axis" in which:
        run_godot(args.godot_exe, project, ["res://input_axis_fixture.tscn"], "INPUT_AXIS_FIXTURE done", WORK / "input_axis.log")
        n = convert_table(project / "input_axis_fixture.tsv", out_tests / "input_axis_godot.tsv",
                          ["# Godot 4.7.1 Input ground truth for the axis-bound actions (the real project.godot [input], Input.parse_input_event",
                           "# then is_action_pressed / get_action_strength / get_action_raw_strength), generated %s by" % GENERATED_ON,
                           "# tools/godot_fixtures/input_axis_fixture.gd; doubles as C hex floats."], None)
        written.append(("input_axis_godot.tsv", out_tests, n))
        n = convert_table(project / "input_vector_fixture.tsv", out_tests / "input_vector_godot.tsv",
                          ["# Godot 4.7.1 Input.get_vector(steer_left, steer_right, steer_up, steer_down) on the right stick (the real",
                           "# project.godot [input]), generated %s by tools/godot_fixtures/input_axis_fixture.gd; Godot's axes (+Y down);" % GENERATED_ON,
                           "# doubles as C hex floats."], None)
        written.append(("input_vector_godot.tsv", out_tests, n))
    if "input_glyphs" in which:
        run_godot(args.godot_exe, project, ["res://input_glyphs_fixture.tscn"], "INPUT_GLYPHS_FIXTURE done", WORK / "input_glyphs.log")
        written += write_glyph_tables(project, out_tests)
    if "input_device" in which:
        run_godot(args.godot_exe, project, ["res://input_device_fixture.tscn"], "INPUT_DEVICE_FIXTURE done", WORK / "input_device.log")
        n = convert_table(project / "input_device_fixture.tsv", out_tests / "input_device_godot.tsv",
                          ["# Godot 4.7.1 InputDevice ground truth (the real input_device.gd observe(), warp_pointer, claim_stick,",
                           "# press_counts_for, joy connections, prompt styles), generated %s by tools/godot_fixtures/input_device_fixture.gd;" % GENERATED_ON,
                           "# `now` is the clock each step ran at (milliseconds); doubles as C hex floats."], None)
        written.append(("input_device_godot.tsv", out_tests, n))
        n = convert_table(project / "input_family_fixture.tsv", out_tests / "input_family_godot.tsv",
                          ["# Godot 4.7.1 InputDevice.family_from_info ground truth, generated %s by" % GENERATED_ON,
                           "# tools/godot_fixtures/input_device_fixture.gd. Family: 0 keyboard, 1 Xbox, 2 PlayStation, 3 Steam Deck, 4 generic."], None)
        written.append(("input_family_godot.tsv", out_tests, n))
        n = convert_table(project / "input_velocity_fixture.tsv", out_tests / "input_velocity_godot.tsv",
                          ["# Godot 4.7.1 Player.stick_velocity ground truth (the cat's speed for a steer deflection and a field width),",
                           "# generated %s by tools/godot_fixtures/input_device_fixture.gd; doubles as C hex floats." % GENERATED_ON], None)
        written.append(("input_velocity_godot.tsv", out_tests, n))
    if "input_bindings" in which:
        run_godot(args.godot_exe, project, ["res://input_bindings_fixture.tscn"], "INPUT_BINDINGS_FIXTURE done", WORK / "input_bindings.log")
        head = ("# Godot 4.7.1 InputBindings ground truth (scripts/systems/input_bindings.gd, the real project.godot [input], scratch copy",
                "# of the project, sandboxed user://), generated %s by tools/godot_fixtures/input_bindings_fixture.gd. An event is" % GENERATED_ON,
                "# k<physical keycode>, m<mouse button>, b<joy button>, a<joy axis><sign>; none is -. Result: 0 BOUND, 1 SWAPPED,",
                "# 2 REFUSED_SYSTEM, 3 REFUSED_TAKEN, 4 UNCHANGED. Saved: action=[{key:<i|f|s|b>value|...},...]; doubles as C hex floats.")
        rows = open(project / "input_bindings_fixture.tsv", encoding="utf-8").read().splitlines()
        # The first 300 random rows (every line of them: start, 8 ops, apply) for the L1 test; all of them for the long test.
        cut = next((i for i, l in enumerate(rows) if l.split("\t")[0] == "300"), len(rows))
        n = convert_table(project / "input_bindings_fixture.tsv", out_tests / "input_bindings_godot.tsv",
                          list(head) + ["# First 300 of 3,000 rows (the L1 test)."], cut - 1)
        written.append(("input_bindings_godot.tsv", out_tests, n))
        n = convert_table(project / "input_bindings_fixture.tsv", out_build / "input_bindings_godot_full.tsv",
                          list(head) + ["# All 3,000 rows (the long test)."], None)
        written.append(("input_bindings_godot_full.tsv", out_build, n))
        n = convert_table(project / "input_binding_from.tsv", out_tests / "input_binding_from_godot.tsv",
                          list(head) + ["# binding_from() on random raw events."], None)
        written.append(("input_binding_from_godot.tsv", out_tests, n))
        n = convert_table(project / "input_bindings_cfg.tsv", out_tests / "input_bindings_cfg_godot.tsv",
                          list(head) + ["# settings.cfg with random [bindings]: the text written, read back, the working state, the text written again;",
                                        "# texts with \\n for a line break. First 200 of 1,500 rows."], 200)
        written.append(("input_bindings_cfg_godot.tsv", out_tests, n))
        n = convert_table(project / "input_bindings_defaults.tsv", out_tests / "input_bindings_defaults_godot.tsv",
                          list(head) + ["# The project's map as InputBindings captured it, in InputMap.get_actions() order."], None)
        written.append(("input_bindings_defaults_godot.tsv", out_tests, n))
    if "controls" in which:
        run_godot(args.godot_exe, project, ["res://controls_fixture.tscn"], "CONTROLS_FIXTURE done", WORK / "controls.log")
        head = ("# Godot 4.7.1 Controls page ground truth (scripts/systems/controls_panel.gd in the real settings_dialog.tscn, with the",
                "# real SettingsManager, InputDevice and InputBindings; scratch copy, sandboxed user://), generated %s by" % GENERATED_ON,
                "# tools/godot_fixtures/controls_fixture.gd; one line per move; texts with \\n for a line break; doubles as C hex floats.",
                "# A cell is text|disabled|family drawn|shows anything; families: 0 keyboard, 1 Xbox, 2 PlayStation, 3 Steam Deck, 4 generic.")
        rows = open(project / "controls_fixture.tsv", encoding="utf-8").read().splitlines()
        cut = next((i for i, l in enumerate(rows) if l.split("\t")[0] == "500"), len(rows))
        n = convert_table(project / "controls_fixture.tsv", out_tests / "controls_godot.tsv",
                          list(head) + ["# First 500 of 1,500 rows (the L1 test)."], cut - 1)
        written.append(("controls_godot.tsv", out_tests, n))
        n = convert_table(project / "controls_fixture.tsv", out_build / "controls_godot_full.tsv",
                          list(head) + ["# All 1,500 rows (the long test)."], None)
        written.append(("controls_godot_full.tsv", out_build, n))
        n = convert_table(project / "controls_words.tsv", out_tests / "controls_words_godot.tsv", list(head[:2]) + ["# The page's words (English)."], None)
        written.append(("controls_words_godot.tsv", out_tests, n))
    if "dock_repeat" in which:
        run_godot(args.godot_exe, project, ["res://dock_repeat_fixture.tscn"], "DOCK_REPEAT_FIXTURE done", WORK / "dock_repeat.log")
        head = ["# Godot 4.7.1 DockNavigator ground truth (scripts/systems/dock_navigator.gd _tick_nav / _tick_press in the real hud.tscn,",
                "# Godot's own action state; scratch copy, sandboxed user://), generated %s by tools/godot_fixtures/dock_repeat_fixture.gd;" % GENERATED_ON,
                "# doubles as C hex floats."]
        n = convert_table(project / "dock_repeat_fixture.tsv", out_tests / "dock_repeat_godot.tsv", head, None)
        written.append(("dock_repeat_godot.tsv", out_tests, n))
        n = convert_table(project / "dock_repeat_rows.tsv", out_tests / "dock_repeat_rows_godot.tsv", head[:2] + ["# The Upgrades page's rows and whether each repeats."], None)
        written.append(("dock_repeat_rows_godot.tsv", out_tests, n))
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
            if name in REAL_TIME:
                print("RUN   %s (real time: re-run, not compared)" % name)
                continue
            masked = next((cols for prefix, cols in WALL_CLOCK.items() if name.startswith(prefix)), None)
            if masked:
                same = ref.exists() and same_but_clock(ref, folder / name, masked)
            else:
                same = ref.exists() and filecmp.cmp(ref, folder / name, shallow=False)
            print("%s  %s%s" % ("SAME" if same else "DIFF", name, " (wall-clock columns masked)" if masked and same else ""))
            bad += 0 if same else 1
        if bad:
            print("FAIL  %d fixture(s) do not reproduce" % bad)
            return 1
        print("PASS  every fixture reproduces byte for byte from the Godot code")
    return 0


if __name__ == "__main__":
    sys.exit(main())
