#!/usr/bin/env python3
"""pawsim — closed-loop economy simulator for Pawpocalypse.

Numbers Doc §11 and the v3 Design & Numbers Review both ask for a closed-loop
simulation; this is it. The review's headline results (a 56-minute exploit
clear, a 5,470x patient-run multiplier) came from a script that was never
committed, so this rebuilds it as a permanent, re-runnable tool.

DESIGN RULE: this script parses the real .tres files in /data. It does not
re-type any balance constant. That is deliberate — the review flags
`TierData.enemy_hp` duplicating `EnemyData.hp` as guaranteed drift, and a
simulator with its own copy of the numbers would be exactly the same bug at a
larger scale. If a number matters here, it is read from /data.

The damage formula is mirrored from CombatResolver.aura_dps()/kitten_dps().
That IS a duplication of logic, and it is the one unavoidable one — Python
cannot call GDScript. `verify_formula_mirror()` documents each term against its
source line so a drift is at least greppable.

Usage:
    python tools/pawsim.py                     # both policies, summary tables
    python tools/pawsim.py --policy exploit    # reset-spam (the F1 exploit)
    python tools/pawsim.py --policy patient    # honest play
    python tools/pawsim.py --verbose           # per-tier timeline
"""

from __future__ import annotations

import argparse
import math
import re
import statistics
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Copied from the Godot project (tools/pawsim.py at 350b2f5) for the port (docs/06 §7). The .tres loader reads the Godot
# project READ-ONLY at PAWSIM_GODOT_ROOT; the CSV loader (--data csv) reads this repo's Data/*.csv.
import os
REPO_ROOT = Path(os.environ.get("PAWSIM_GODOT_ROOT", "C:/Games/pawpocalypse"))
DATA_DIR = REPO_ROOT / "data"

# Simulation resolution. 1.0s is plenty — every rate in the game is per-second
# or per-minute, and finer steps change results by well under a percent.
DT = 1.0
MAX_SIM_SECONDS = 24 * 3600

# Review §"Sim assumptions": perfect targeting, signature invuln ignored when
# hunting commons (optimistic), greedy upgrade buying, kills capped by spawn
# rate. Kept identical so results stay comparable to the review's.
SIGNATURE_INVULN_IGNORED = True

# Void-kitten adds heal the boss unless intercepted. The sim has no spatial
# model, so it cannot know how many the player actually catches — and pretending
# it can would be worse than stating an assumption. ADD_LEAK_FRACTION is that
# assumption, exposed as --add-leak so the sensitivity can be checked rather
# than argued about.
#
# 0.25 is deliberately pessimistic-ish: a player who ignores adds entirely faces
# a far longer fight than this, and one who plays them perfectly faces a shorter
# one. Sizing boss HP against "a quarter get through" means a careless player is
# punished and an attentive one is rewarded, which is the whole point of the
# mechanic.
## Separate default constant so argparse can read it without tripping Python's
## "used prior to global declaration" rule inside main().
ADD_LEAK_DEFAULT = 0.25

# Golden Mice flee, so the player does not catch all of them. Same honesty
# problem as add leakage: the sim has no spatial model and cannot know the real
# rate, so the assumption is named and exposed as --golden-catch rather than
# buried. 0.5 is deliberately middling — a player who ignores them entirely
# still gets the run the boss curve was tuned for, just slower.
GOLDEN_CATCH_DEFAULT = 0.5
GOLDEN_CATCH_RATE = GOLDEN_CATCH_DEFAULT
ADD_LEAK_FRACTION = ADD_LEAK_DEFAULT

# Share of FLIERS the player actually kills (v4 Phase 8, 2026-08-19).
#
# A flier takes nothing from ground damage zones and crosses at 2.1x speed, so
# unlike every other pool spawn it is NOT reliably killed by leaving the aura
# where it is. Modelling it as a certain kill would make the whole verb read as
# free income, and its reward premium is priced as exactly 1/this — so the two
# have to agree or the simulator pays for misses that never happen.
#
# The same number is asserted in ci/check_resources.gd as FLIER_CATCH_RATE.
FLIER_CATCH_RATE = 0.45

# GameState.MAX_GOLDEN_CHANCE. A golden chance at 1.0 would mean the spawn pool
# never rolls and every enemy in the game is a Golden Mouse.
MAX_GOLDEN_CHANCE = 0.35


# --------------------------------------------------------------------------
# v4: Catches (per-run card picks)
# --------------------------------------------------------------------------
#
# v4 Phase 1 deals the player three cards on every Catches level and lets them
# keep one. None of it is built yet. This models the TRACK — how many picks a
# run contains — so Phase 1 has a tuning surface rather than a guess, which is
# the same reason v3 built the economy simulator before touching the economy.
#
# The plan's target is 10-15 picks in a 20-40 minute run.
#
# XP is one point per mouse killed, and Catches level N costs
# CATCHES_BASE * CATCHES_GROWTH^N. Both are on the CLI because the honest
# answer to "what curve gives twelve picks a run?" is a sweep, not an opinion.
#
# The interesting question this is built to answer: kills per run GROW across a
# game (spawn rate and DPS both compound), while the XP curve is fixed, so
# picks per run will drift upward unless the curve outruns the kill count.
# Whether that drift is a problem or a feature is a Phase 1 decision — this
# just makes it visible.
# Calibrated by sweep, 2026-08-14 at base 8.0 / growth 1.75: a median of 13 picks
# holding in a 10-16 band. GROWTH sets the SPREAD (kills per run grow ~35x over
# a game, so the drift is log_GROWTH(35)); BASE slides the whole band.
#
# RAISED 2026-08-18 to 24.0 / 1.68 on the designer's instruction that "XP gain
# should be slower in the beginning" — at base 8 the first card arrived after
# eight mice. These mirror CardManager.LEVEL_COST_BASE / LEVEL_COST_GROWTH and
# check_cards.gd asserts the two agree.
#
# The sweep above has NOT been re-run against them. That is deliberate: the
# designer asked for every mechanical change first and one economy pass over all
# of them together, so this band is currently unverified rather than calibrated.
CATCHES_BASE_DEFAULT = 24.0
CATCHES_GROWTH_DEFAULT = 1.68
CATCHES_BASE = CATCHES_BASE_DEFAULT
CATCHES_GROWTH = CATCHES_GROWTH_DEFAULT


def whole_catnip(amount: float) -> float:
    """Mirrors GameState.whole_catnip(): earned payouts are whole, never below 1.

    Added 2026-08-17 after the designer reported that killing four mice did not
    move the money. A Tier 1 common pays 0.33 against a HUD that floors the bank,
    so the first four kills of a new game displayed 0, 0, 0, 1.
    """
    if amount <= 0.0:
        return 0.0
    return max(1.0, round(amount))


def catches_levels_for_points(xp: float) -> int:
    """How many Catches levels `xp` buys from zero within one run.

    Closed form rather than a loop: the cost of the first N levels is the
    geometric series BASE * (GROWTH^N - 1) / (GROWTH - 1), so N falls out of a
    logarithm. At a million ticks a run, the loop version showed up in profiles.
    """
    if xp <= 0.0 or CATCHES_BASE <= 0.0:
        return 0
    if CATCHES_GROWTH <= 1.0:
        return int(xp / CATCHES_BASE)
    inner = 1.0 + xp * (CATCHES_GROWTH - 1.0) / CATCHES_BASE
    return int(math.log(inner, CATCHES_GROWTH))


# Total Stardust the achievement roster pays. Read from /data rather than typed,
# same rule as every other constant here.
ACHIEVEMENT_STARDUST_TOTAL = 0.0


# --------------------------------------------------------------------------
# .tres parsing
# --------------------------------------------------------------------------

_EXT_RE = re.compile(r'\[ext_resource\s+.*?path="res://([^"]+)".*?id="([^"]+)"\]')
_KV_RE = re.compile(r"^(\w+)\s*=\s*(.+)$")
_ARRAY_RE = re.compile(r"Array\[\w+\]\(\[(.*)\]\)")
_EXTREF_ID_RE = re.compile(r'ExtResource\("([^"]+)"\)')
_EXTREF_RE = re.compile(r'ExtResource\("([^"]+)"\)')


_DICT_ENTRY_RE = re.compile(r'"([^"]+)"\s*:\s*\[([^\]]*)\]')


def _parse_dict_block(text: str) -> dict:
    """A Godot Dictionary of string -> float array, flattened to one line.

    Only the shape SkillBranchData.extra_effects uses. Anything else in the
    block is ignored rather than guessed at — a silently wrong value here would
    show up as a balance number nobody could trace.
    """
    out: dict = {}
    for key, body in _DICT_ENTRY_RE.findall(text):
        body = body.strip()
        out[key] = [float(x) for x in body.split(",")] if body else []
    return out


def parse_tres(path: Path) -> dict:
    """Minimal Godot .tres reader — enough for the flat resources in /data.

    Returns a dict of property name -> value. ExtResource references are
    resolved to absolute Paths so callers can recurse into them.
    """
    text = path.read_text(encoding="utf-8")
    ext_map = {rid: REPO_ROOT / rel for rel, rid in _EXT_RE.findall(text)}

    out: dict = {}
    in_resource = False
    # A Godot Dictionary literal spans lines, which this line-based reader
    # cannot see. When one opens, swallow lines until the braces balance and
    # hand the whole block to _parse_dict_block(). Added 2026-08-21 for
    # SkillBranchData.extra_effects, the v6 multi-channel ladders — without it
    # Claws' multiplicative half and Legacy's head-start channels are invisible
    # to the simulator, which would then model a weaker game than ships.
    pending_key = ""
    pending: list = []
    depth = 0

    for raw in text.splitlines():
        line = raw.strip()

        if pending_key:
            pending.append(line)
            depth += line.count("{") - line.count("}")
            if depth <= 0:
                out[pending_key] = _parse_dict_block(" ".join(pending))
                pending_key = ""
                pending = []
            continue

        if line.startswith("[resource]"):
            in_resource = True
            continue
        if line.startswith("["):
            in_resource = False
            continue
        if not in_resource or not line:
            continue

        m = _KV_RE.match(line)
        if not m:
            continue
        key, val = m.group(1), m.group(2).strip()

        if val.startswith("{"):
            pending_key = key
            pending = [val]
            depth = val.count("{") - val.count("}")
            if depth <= 0:
                out[key] = _parse_dict_block(val)
                pending_key = ""
                pending = []
            continue

        arr = _ARRAY_RE.match(val)
        if arr:
            body = arr.group(1).strip()
            if not body:
                out[key] = []
            elif "ExtResource(" in body:
                # An array of resource references — TierData.spawn_pool since
                # 2026-08-18. Resolved element-wise the same way a scalar
                # reference is, so callers can recurse without caring which
                # shape the field had.
                out[key] = [ext_map[rid] for rid in _EXTREF_ID_RE.findall(body)]
            elif '"' in body:
                out[key] = [s.strip().strip('"') for s in body.split(",")]
            else:
                out[key] = [float(s) for s in body.split(",")]
            continue

        ref = _EXTREF_RE.match(val)
        if ref:
            resolved = ext_map.get(ref.group(1))
            # id "1" is always the script itself; skip it.
            if resolved is not None and resolved.suffix == ".tres":
                out[key] = resolved
            continue

        if val.startswith('"'):
            out[key] = val.strip('"')
        elif val in ("true", "false"):
            # Without this, `is_signature = false` parsed to the STRING "false",
            # which is truthy — every enemy would have read as a signature.
            out[key] = (val == "true")
        else:
            try:
                out[key] = float(val)
            except ValueError:
                out[key] = val

    return out


# --------------------------------------------------------------------------
# Data model — field names mirror the GDScript Resource classes
# --------------------------------------------------------------------------

MULTIPLICATIVE, ADDITIVE = 0, 1


@dataclass
class Upgrade:
    """scripts/data/upgrade_data.gd"""

    id: str
    base_cost: float
    cost_growth: float
    effect_per_level: float
    effect_type: int
    cap: float
    cap_per_floor: float = 0.0
    cap_per_reawaken: float = 0.0

    def cost_for_level(self, level: int) -> float:
        return self.base_cost * self.cost_growth**level

    def effect_total(self, level: int) -> float:
        # GameState._effect_total()
        if self.effect_type == MULTIPLICATIVE:
            return self.effect_per_level**level
        return self.effect_per_level * level

    def effective_cap(self, floors: int = 0, reawakens: int = 0) -> float:
        # GameState.effective_cap() — the ceiling rises with the deepest biome
        # reached this run (2026-08-19) AND with Reawaken count (Step 7,
        # 2026-08-25). The second term was authored in /data and this model did
        # not read it, so every measurement of it came back "no change" — the
        # same way achievement_stardust() sat here uncalled.
        if self.cap < 0.0:
            return -1.0
        return (self.cap
                + self.cap_per_floor * max(floors, 0)
                + self.cap_per_reawaken * max(reawakens, 0))

    def at_cap(self, level: int, floors: int = 0, reawakens: int = 0) -> bool:
        # GameState.is_upgrade_at_cap()
        if self.cap < 0.0:
            return False
        return self.effect_total(level) >= self.effective_cap(floors, reawakens)

    def clamped(self, level: int, floors: int = 0, reawakens: int = 0) -> float:
        v = self.effect_total(level)
        return min(v, self.effective_cap(floors, reawakens)) if self.cap >= 0.0 else v


@dataclass
class Branch:
    """scripts/data/skill_branch_data.gd"""

    id: str
    level_costs: list
    level_effects: list
    # Reawakens required before the tree will sell this branch at all. Read from
    # /data like everything else here — the ladder was added 2026-08-20 and a
    # simulator that did not know about it would report a tree the real game
    # refuses to sell.
    required_reawakens: int = 0
    # v6 (2026-08-21). Unlocks are bought once and hand over an ability;
    # ladders are bought repeatedly and move a number. `parent` is the branch
    # this one grows out of, and when `parent_is_required` the tree refuses to
    # sell this until the parent is owned.
    is_unlock: bool = False
    parent: str = ""
    parent_is_required: bool = False
    # Channel name -> per-level values, for the ladders that merged two or three
    # old branches into one node.
    extra_effects: dict = field(default_factory=dict)

    def max_level(self) -> int:
        return len(self.level_costs)

    def cost_for_next_level(self, level: int) -> float:
        if level >= self.max_level():
            return math.inf
        return self.level_costs[level]

    def effect_at_level(self, level: int) -> float:
        if level <= 0:
            return 0.0
        return self.level_effects[min(level, len(self.level_effects)) - 1]

    def extra_at_level(self, channel: str, level: int) -> float:
        values = self.extra_effects.get(channel) or []
        if level <= 0 or not values:
            return 0.0
        return float(values[min(level, len(values)) - 1])


@dataclass
class Enemy:
    id: str
    hp: float
    reward_catnip: float
    is_signature: bool = False
    is_flier: bool = False


@dataclass
class Tier:
    id: str
    index: int
    spawn_rate_per_min: float
    required_evolution_stage: int
    # (Enemy, weight) drawn from several enemy tiers — mirrors
    # TierData.spawn_pool / spawn_weights. A biome stopped being "one common plus
    # one signature" on 2026-08-18; it is a mix now, mostly its own tier, with
    # leftovers from below and a warning shot from above.
    pool: list
    boss: dict | None
    golden_chance: float = 0.0
    golden: Enemy | None = None
    # Catnip that must have been earned THIS RUN before the biome opens, and the
    # flat multiplier on every reward paid inside it (2026-08-19). Last in the
    # dataclass because they carry defaults and `pool`/`boss` do not.
    catnip_to_unlock: float = 0.0
    reward_multiplier: float = 1.0

    def _pool_mean(self, value) -> float:
        """Weighted mean of value(enemy) over the pool. Goldens excluded — they
        are rolled separately and mostly escape."""
        total = sum(w for _e, w in self.pool)
        if total <= 0.0:
            return 0.0
        return sum(value(e) * w for e, w in self.pool) / total

    @property
    def signature_fraction(self) -> float:
        """Share of pool spawns that are signatures. Catches pays them double, so
        this is what the XP rate rides on. Mirrors TierData.signature_fraction()."""
        return self._pool_mean(lambda e: 1.0 if e.is_signature else 0.0)

    @property
    def avg_hp(self) -> float:
        # Uncaught goldens still cost the damage spent chasing them, but the sim
        # has no chase model, so they are counted at their catch rate only.
        g = self.golden_chance if self.golden is not None else 0.0
        ordinary = self._pool_mean(lambda e: e.hp)
        if g <= 0.0:
            return ordinary
        return (1.0 - g) * ordinary + g * self.golden.hp * GOLDEN_CATCH_RATE

    @property
    def avg_catnip(self) -> float:
        # Golden is rolled first and wins outright (TierController), so it takes
        # its share off the top and the pool splits what is left.
        g = self.golden_chance if self.golden is not None else 0.0
        ordinary = self._pool_mean(lambda e: e.reward_catnip)
        if g <= 0.0:
            return ordinary
        # Only the CAUGHT fraction pays; the rest flee and time out.
        return (1.0 - g) * ordinary + g * self.golden.reward_catnip * GOLDEN_CATCH_RATE


@dataclass
class GameData:
    combat: dict
    reawaken: dict
    upgrades: dict
    branches: dict
    tiers: list
    evolution: dict


def _achievement_stardust_total() -> float:
    total = 0.0
    for p in sorted((DATA_DIR / "achievements").glob("*.tres")):
        total += parse_tres(p).get("stardust_reward", 0.0)
    return total


## How far ahead a player will save for a Bait, in seconds of current income.
## Sized a little above the ten minutes the prices are authored against, so a
## price that drifts slightly still reads as a goal rather than a wall.
BAIT_SAVE_HORIZON_SEC = 900.0


def load_game_data() -> GameData:
    def enemy(p: Path) -> Enemy:
        d = parse_tres(p)
        # EnemyData.MovementKind.FLIER == 1. Absent on every enemy authored
        # before Phase 8, which is exactly why GROUND is index 0.
        return Enemy(d["id"], d["hp"], d["reward_catnip"],
                     bool(d.get("is_signature", False)),
                     int(d.get("movement", 0)) == 1)

    # Three of the eight biomes hold a boss (v4 Phase 2). The eight separate
    # ARENA realms this used to have to reach through were deleted with that
    # change, along with the boss_ledger_id indirection that pointed an arena
    # back at the biome it gated. A biome's boss is now simply its own.
    tiers = []
    # Discovered, not a hardcoded range. `for i in range(1, 9)` silently ignored
    # biomes 9-12 the moment they were authored, and the sim happily reported a
    # "final boss" that was no longer the final boss - the same class of quiet
    # wrongness as achievement_stardust() sitting uncalled.
    i = 0
    while True:
        i += 1
        path = DATA_DIR / f"tier_{i}.tres"
        if not path.exists():
            break
        d = parse_tres(path)
        boss = parse_tres(d["boss_data"]) if "boss_data" in d else None
        tiers.append(
            Tier(
                id=d["id"],
                index=i,
                spawn_rate_per_min=d["spawn_rate_per_min"],
                required_evolution_stage=int(d["required_evolution_stage"]),
                catnip_to_unlock=float(d.get("catnip_to_unlock", 0.0)),
                reward_multiplier=float(d.get("reward_multiplier", 1.0)),
                pool=list(zip(
                    [enemy(path) for path in d["spawn_pool"]],
                    [float(w) for w in d["spawn_weights"]],
                )),
                boss=boss,
                golden_chance=d.get("golden_chance", 0.0),
                golden=enemy(d["golden_enemy"]) if "golden_enemy" in d else None,
            )
        )

    global ACHIEVEMENT_STARDUST_TOTAL
    ACHIEVEMENT_STARDUST_TOTAL = _achievement_stardust_total()

    upgrades = {}
    for p in sorted(DATA_DIR.glob("upgrade_*.tres")):
        d = parse_tres(p)
        upgrades[d["id"]] = Upgrade(
            id=d["id"],
            base_cost=d["base_cost"],
            cost_growth=d["cost_growth"],
            effect_per_level=d["effect_per_level"],
            effect_type=int(d["effect_type"]),
            cap=d["cap"],
            cap_per_floor=d.get("cap_per_floor", 0.0),
            cap_per_reawaken=d.get("cap_per_reawaken", 0.0),
        )

    branches = {}
    for p in sorted((DATA_DIR / "skill_tree").glob("*.tres")):
        d = parse_tres(p)
        branches[d["id"]] = Branch(
            d["id"], d["level_costs"], d["level_effects"],
            int(d.get("required_reawakens", 0)),
            bool(d.get("is_unlock", False)),
            str(d.get("parent", "")),
            bool(d.get("parent_is_required", False)),
            dict(d.get("extra_effects", {}) or {}),
        )

    evolution = {}
    for p in sorted((DATA_DIR / "evolution").glob("*.tres")):
        d = parse_tres(p)
        evolution[int(d["stage"])] = d

    return GameData(
        combat=parse_tres(DATA_DIR / "combat_constants.tres"),
        reawaken=parse_tres(DATA_DIR / "reawaken_constants.tres"),
        upgrades=upgrades,
        branches=branches,
        tiers=tiers,
        evolution=evolution,
    )


# --------------------------------------------------------------------------
# Data/*.csv loader (the Unreal port, docs/06 §7). Same numbers, same order, same Python types as load_game_data():
# parse_tres() turns every number into a float, strings stay strings, bools are bools, missing keys stay missing, and
# files are visited in sorted-path order (dict order decides what the sim iterates and prints). This loader reproduces
# each of those, so the full-game report must come out byte-identical (tools/check_pawsim_parity.py).
# --------------------------------------------------------------------------

import csv
import json

CSV_DIR = Path(__file__).resolve().parent.parent / "Data"

# Columns the exporter writes as ';'-joined arrays (a one-element array has no ';', so the type cannot be guessed).
_CSV_LIST_COLUMNS = {
    "spawn_pool", "spawn_weights", "level_costs", "level_effects", "phase_hp_thresholds", "phase_vulnerable_sec",
    "phase_shield_sec", "ability_ids", "exclusive_with", "tint",
}
# Enums are written by NAME (docs/06 §3); these are the ones the sim reads as numbers, with the Godot enum order.
_CSV_ENUMS = {
    ("UpgradeData", "effect_type"): ["MULTIPLICATIVE", "ADDITIVE"],
    ("EnemyData", "movement"): ["GROUND", "FLIER"],
}


def _csv_scalar(text: str):
    if text in ("true", "false"):
        return text == "true"
    try:
        return float(text)
    except ValueError:
        return text


def _csv_rows(cls: str) -> list:
    """Rows of Data/<cls>.csv as parse_tres-shaped dicts, sorted by their Godot file path like sorted(glob(...))."""
    out = []
    with open(CSV_DIR / f"{cls}.csv", encoding="utf-8", newline="") as f:
        for row in csv.DictReader(f):
            d = {}
            for col, text in row.items():
                if col == "_godot_file" or text == "":
                    continue
                if (cls, col) in _CSV_ENUMS:
                    names = _CSV_ENUMS[(cls, col)]
                    if text not in names:
                        raise ValueError(f"{cls}.{col}: unknown enum name {text!r} (known: {names})")
                    d[col] = float(names.index(text))
                elif col == "extra_effects":
                    d[col] = {k: [float(x) for x in v] for k, v in json.loads(text).items()}
                elif col in _CSV_LIST_COLUMNS:
                    d[col] = [_csv_scalar(x) for x in text.split(";")]
                else:
                    d[col] = _csv_scalar(text)
            d["_godot_file"] = row["_godot_file"]
            out.append(d)
    # Windows Path ordering is case-insensitive; the .tres loader sorted Path objects.
    return sorted(out, key=lambda d: d["_godot_file"].lower())


def load_code_constants_csv() -> dict:
    """Data/CodeConstants.csv (docs/06 §5): the gameplay constants that live in Godot CODE, by id. The .tres loader has no
    such file, so with --data tres the hand-copied module constants above stand in for them (the Godot pawsim's way)."""
    with open(CSV_DIR / "CodeConstants.csv", newline="", encoding="utf-8") as f:
        return {row["id"]: row["value"] for row in csv.DictReader(f)}


def load_game_data_csv() -> GameData:
    # The cap is read from the same CSV PawCore reads, so the two cannot drift (the Godot pawsim copied it by hand).
    global MAX_GOLDEN_CHANCE
    MAX_GOLDEN_CHANCE = float(load_code_constants_csv()["max_golden_chance"])
    enemies = {d["id"]: d for d in _csv_rows("EnemyData")}
    bosses = {d["id"]: d for d in _csv_rows("BossData")}

    def enemy(eid: str) -> Enemy:
        d = enemies[eid]
        return Enemy(d["id"], d["hp"], d["reward_catnip"],
                     bool(d.get("is_signature", False)),
                     int(d.get("movement", 0)) == 1)

    def strip(d: dict) -> dict:
        return {k: v for k, v in d.items() if k != "_godot_file"}

    tier_rows = {d["_godot_file"]: d for d in _csv_rows("TierData")}
    tiers = []
    i = 0
    while True:
        i += 1
        d = tier_rows.get(f"data/tier_{i}.tres")
        if d is None:
            break
        tiers.append(
            Tier(
                id=d["id"],
                index=i,
                spawn_rate_per_min=d["spawn_rate_per_min"],
                required_evolution_stage=int(d["required_evolution_stage"]),
                catnip_to_unlock=float(d.get("catnip_to_unlock", 0.0)),
                reward_multiplier=float(d.get("reward_multiplier", 1.0)),
                pool=list(zip(
                    [enemy(e) for e in d["spawn_pool"]],
                    [float(w) for w in d["spawn_weights"]],
                )),
                boss=strip(bosses[d["boss_data"]]) if "boss_data" in d else None,
                golden_chance=d.get("golden_chance", 0.0),
                golden=enemy(d["golden_enemy"]) if "golden_enemy" in d else None,
            )
        )

    global ACHIEVEMENT_STARDUST_TOTAL
    ACHIEVEMENT_STARDUST_TOTAL = 0.0
    for d in _csv_rows("AchievementData"):
        ACHIEVEMENT_STARDUST_TOTAL += d.get("stardust_reward", 0.0)

    upgrades = {}
    for d in _csv_rows("UpgradeData"):
        upgrades[d["id"]] = Upgrade(
            id=d["id"],
            base_cost=d["base_cost"],
            cost_growth=d["cost_growth"],
            effect_per_level=d["effect_per_level"],
            effect_type=int(d["effect_type"]),
            cap=d["cap"],
            cap_per_floor=d.get("cap_per_floor", 0.0),
            cap_per_reawaken=d.get("cap_per_reawaken", 0.0),
        )

    branches = {}
    for d in _csv_rows("SkillBranchData"):
        branches[d["id"]] = Branch(
            d["id"], d["level_costs"], d["level_effects"],
            int(d.get("required_reawakens", 0)),
            bool(d.get("is_unlock", False)),
            str(d.get("parent", "")),
            bool(d.get("parent_is_required", False)),
            dict(d.get("extra_effects", {}) or {}),
        )

    evolution = {}
    for d in _csv_rows("EvolutionData"):
        evolution[int(d["stage"])] = strip(d)

    return GameData(
        combat=strip(_csv_rows("CombatConstantsData")[0]),
        reawaken=strip(_csv_rows("ReawakenConstantsData")[0]),
        upgrades=upgrades,
        branches=branches,
        tiers=tiers,
        evolution=evolution,
    )


# --------------------------------------------------------------------------
# Player state — mirrors GameState + SkillTreeManager
# --------------------------------------------------------------------------


@dataclass
class State:
    g: GameData
    catnip: float = 0.0   # earned from kills, buys the base upgrades
    stardust: float = 0.0 # earned from Reawaken, buys skill branches
    reawaken_count: int = 0
    lifetime_catnip_this_cycle: float = 0.0
    lifetime_catnip_ever: float = 0.0  # not in the shipped game — review fix 1
    upgrade_levels: dict = field(default_factory=dict)
    branch_levels: dict = field(default_factory=dict)
    bosses_defeated: set = field(default_factory=set)  # tier indices, 1-based
    # --- THE BAIT GATE (2026-08-27, taught to the sims 2026-08-28) ----------
    # Kills bank PER BIOME and pay for that biome's boss alone, so farming an
    # easy tier cannot buy a hard tier's fight. All three are cleared by a
    # Reawaken: the bait is re-earned every cycle, which IS the mechanic — a
    # static 40 measured against a player who keeps getting faster.
    #
    # `bait_earned` is DERIVED from the kill bank rather than stored, mirroring
    # GameState.bait_earned(). Two stored numbers that must agree is how they
    # stop agreeing.
    tier_kills_by_biome: dict = field(default_factory=dict)   # index -> kills
    bait_spent_by_biome: dict = field(default_factory=dict)   # index -> bait
    # PASSAGE re-locks every cycle; POWER never does. bosses_defeated above is
    # permanent and grants the evolution stage; this gates travel only.
    bosses_cleared_this_cycle: set = field(default_factory=set)
    # Every shop purchase this run: (upgrade id, cost, utility gain as a
    # FRACTION of the utility before it). The fraction is what "noticeable"
    # means as a number - see cadence_report().
    buy_log: list = field(default_factory=list)
    # Achievement Stardust already paid. achievement_stardust() is a CUMULATIVE
    # total, so crediting it needs the delta - see credit_achievement_stardust().
    _achievement_stardust_paid: float = 0.0

    def __post_init__(self):
        for k in self.g.upgrades:
            self.upgrade_levels.setdefault(k, 0)
        for k in self.g.branches:
            self.branch_levels.setdefault(k, 0)

    # --- GameState mirrors -------------------------------------------------

    def is_unicorn_unlocked(self) -> bool:
        return self.reawaken_count >= self.g.reawaken["unicorn_unlock_reawaken_count"]

    # Pounce and Overkill became SKILL TREE branches on 2026-08-18, spread across
    # the first three Reawakens. Two states, same split as GameState:
    # *_unlocked = the tree will sell it; *_owned = the player bought it, so the
    # ability works and its shop lines exist.
    def is_pounce_unlocked(self) -> bool:
        return self.reawaken_count >= self.g.reawaken["pounce_unlock_reawaken_count"]

    def is_pounce_owned(self) -> bool:
        return self.branch_levels.get("unlock_pounce", 0) > 0

    def is_overkill_unlocked(self) -> bool:
        return self.reawaken_count >= self.g.reawaken["overkill_unlock_reawaken_count"]

    def is_overkill_owned(self) -> bool:
        return self.branch_levels.get("unlock_overkill", 0) > 0

    # Mirrors GameState.BRANCH_GATED_UPGRADES / is_upgrade_visible(). A hidden
    # shop line is unbuyable, not merely unshown, so the buyer must not price it.
    BRANCH_GATED_UPGRADES = {
        "pounce_power": "unlock_pounce",
        "pounce_cooldown": "unlock_pounce",
        "overkill": "unlock_overkill",
    }

    def is_upgrade_visible(self, uid: str) -> bool:
        branch = self.BRANCH_GATED_UPGRADES.get(uid)
        return branch is None or self.branch_levels.get(branch, 0) > 0

    def evolution_stage(self) -> int:
        """Mirrors GameState.get_evolution_stage().

        Boss-gated: one stage per CONSECUTIVE boss cleared, walking the biomes
        that actually hold one. With three bosses this returns 1..4 (v4 Phase
        2). It used to count "tier 1..7 cleared in order", which encoded "every
        biome has a boss" as an integer range — with bosses only in biomes 3, 6
        and 8 that returns 1 forever.

        There is no reawaken path to a stage, so F2 is structurally impossible
        rather than merely patched — `--fix-f2` is retained only to reproduce
        the pre-fix behaviour for comparison runs.
        """
        if LEGACY_MYSTIQUE_GATE:
            if self.is_unicorn_unlocked():
                if not FIX_F2:
                    return 8
                if self.branch_levels["claws"] >= self.g.branches["claws"].max_level():
                    return 8
            eff = self.g.branches["claws"].effect_at_level(self.branch_levels["claws"])
            return int(eff) if eff > 0.0 else 1

        stage = 1
        for tier in self.g.tiers:
            if tier.boss is None:
                continue
            if tier.index not in self.bosses_defeated:
                break
            stage += 1
        return stage

    def kitten_count(self) -> int:
        u = self.g.upgrades["kitten_count"]
        vit = int(self.g.branches["vitality"].effect_at_level(self.branch_levels["vitality"]))
        return int(u.clamped(self.upgrade_levels["kitten_count"],
                             self.floors_reached(), self.reawaken_count)) + vit

    def reawaken_multiplier(self) -> float:
        """Mirrors GameState.get_reawaken_multiplier().

        A POWER LAW in Stardust ever earned since 2026-08-18. It used to be
        `mult_per_point ** softcap(stardust)`; soft-capping the exponent of an
        exponential does not bound it, and this function used to raise
        OverflowError on a long patient run — which is how the problem was
        found, not by inspection.
        """
        scale = self.g.reawaken.get("reawaken_mult_scale", 0.0)
        if scale <= 0.0:
            return 1.0
        return (1.0 + max(self._stardust_granted_ever, 0.0) / scale) ** self.g.reawaken[
            "reawaken_mult_exponent"
        ]

    # --- CombatResolver mirrors -------------------------------------------

    def _damage_scale(self) -> float:
        """Everything BOTH damage sources are scaled by.

        Mirrors CombatResolver._damage_scale(). Factored out for the same reason
        it is factored out there: the paw and the aura are two ways the same cat
        hits things, and a bonus that reached one but not the other would be a
        bug nobody could see.
        """
        stage = self.evolution_stage()
        return (
            (self.g.combat["evolution_damage_per_stage"] ** (stage - 1))
            * self.g.evolution[stage]["weapon_multiplier"]
            * (1.0 + self.g.branches["claws"].effect_at_level(self.branch_levels["claws"]))
            # multiplicative_damage_product — Mystique. Ferocity SUMS, Mystique
            # MULTIPLIES: separate brackets, per pawpocalypse.md Combat Rules.
            * (
                self.g.branches["claws"].extra_at_level("mult", self.branch_levels["claws"])
                if (self.branch_levels["claws"] > 0 and not LEGACY_MYSTIQUE_GATE)
                else 1.0
            )
            * self.reawaken_multiplier()
        )

    def _crit_term(self) -> float:
        """Expected-value crit, as the resolver folds it in.

        Crit was hardcoded to zero from v3 until v5 gave it a shop line back.
        It is rolled PER SWIPE in the game; here it is an average, which is what
        every balance comparison downstream wants.
        """
        crit = self.g.upgrades["crit_chance"].clamped(
            self.upgrade_levels["crit_chance"], self.floors_reached(),
            self.reawaken_count)
        return 1.0 + crit * (self.g.combat["crit_multiplier"] - 1.0)

    def paw_dps(self) -> float:
        """The cat's own swipes — CombatResolver.paw_dps().

        Discrete since v5: `base_attack_speed` swings a second, `base_paw_damage`
        each. That is what makes Attack Speed a stat at all — on the continuous
        rate the aura used to be, it was arithmetically Paw Damage again.

        Shred counts at HALF its ceiling, matching the resolver: a player
        sweeping a crowd never stacks and one committed to a single mouse always
        does, so the truth for a run sits between.
        """
        c = self.g.combat
        u = self.g.upgrades
        floors = self.floors_reached()
        speed = c["base_attack_speed"] * u["attack_speed"].clamped(
            self.upgrade_levels["attack_speed"], floors, self.reawaken_count)
        shred = 1.0 + u["shred"].clamped(self.upgrade_levels["shred"], floors,
                                     self.reawaken_count) \
            * (c["shred_max_stacks"] * 0.5)
        return (
            c["base_paw_damage"]
            # clamped(), matching GameState._rebuild_stats()'s _capped_effect.
            # Identical to effect_total() while paw_damage authors cap = -1.0,
            # and the difference between a future authored cap binding and being
            # silently ignored. BOTH sides were changed together on 2026-08-28:
            # changing one alone is how a mirror drifts.
            * u["paw_damage"].clamped(self.upgrade_levels["paw_damage"], floors,
                                      self.reawaken_count)
            * shred
            * self._damage_scale()
            * speed
            * self._crit_term()
        )

    def aura_owned(self) -> bool:
        """The aura is a PURCHASE — branch `unlock_aura` since v6."""
        return self.branch_levels.get("unlock_aura", 0) > 0

    def aura_dps(self) -> float:
        """Mirrors CombatResolver.aura_dps() — zero until the branch is bought."""
        if not self.aura_owned():
            return 0.0
        return (
            self.g.combat["base_damage_per_sec"]
            # clamped() for the reason in paw_dps() above — the aura reads the
            # same shop line, so it has to honour the same cap.
            * self.g.upgrades["paw_damage"].clamped(
                self.upgrade_levels["paw_damage"], self.floors_reached(),
                self.reawaken_count)
            * (self.g.branches["aura_damage"].effect_at_level(
                self.branch_levels["aura_damage"])
               if self.branch_levels.get("aura_damage", 0) > 0 else 1.0)
            * self._damage_scale()
            * self._crit_term()
        )

    def player_dps(self) -> float:
        """Paw and aura together — CombatResolver.player_dps().

        What Pounce, ground zones and edge burn are priced against. They used to
        be fractions of the aura, which WAS the whole player; leaving them there
        would have made every one worth nothing until the branch was bought.
        """
        return self.paw_dps() + self.aura_dps()

    def kitten_dps_each(self) -> float:
        """Mirrors CombatResolver.kitten_dps() (combat_resolver.gd:111).

        NO EVOLUTION MULTIPLIER, and the absence is the point. This function
        carried `evolution_damage_per_stage ** (stage - 1)` — reaching x128 by
        stage 7 — from the day it was written, and justified it by citing a
        rationale in the game function's header that has never contained it. All
        twelve revisions of combat_resolver.gd back to 10493a9 agree: the game
        has never scaled kitten damage by the evolution stage. The term was
        invented HERE, so this was a mirror that was wrong from birth rather
        than one that drifted away.

        Removed 2026-08-28; docs/pawpocalypse-code-review-2026-08-28.md section 1
        carries the measured blast radius (up to 9.30x on total DPS at stage 6).
        """
        u = self.g.upgrades
        return (
            self.g.combat["base_kitten_damage_per_sec"]
            # clamped(), matching _rebuild_stats()'s _capped_effect. See paw_dps().
            * u["kitten_power"].clamped(self.upgrade_levels["kitten_power"],
                                        self.floors_reached(), self.reawaken_count)
            * (1.0 + self.g.branches["instinct"].effect_at_level(self.branch_levels["instinct"]))
            * 1.0  # kitten_multiplicative_damage_product
            * self.reawaken_multiplier()
        )

    def poop_ability_unlocked(self) -> bool:
        """Mirrors GameState.is_poop_ability_unlocked() (game_state.gd:233)."""
        # Its own node since 2026-10-01: Unicorn, then Rainbow Poop beneath.
        return (self.is_unicorn_unlocked() and self.branch_levels["unlock_unicorn"] > 0
                and self.branch_levels.get("unlock_poop", 0) > 0)

    def poop_dps_sustained(self) -> float:
        """Mirrors CombatResolver.poop_dps() (combat_resolver.gd:52), amortised.

        The zone is not always up: TierController fires it on `poop_cooldown_sec`
        and it lasts `poop_duration_sec`, so sustained single-target contribution
        is the instantaneous DPS times that duty cycle. Nothing feeds
        `poop_power_multiplier` or `poop_additive_damage_sum` today — Unicorn is
        the only branch touching poop, via the multiplicative product.
        """
        if not self.poop_ability_unlocked():
            return 0.0
        duty = self.g.combat["poop_duration_sec"] / self.g.combat["poop_cooldown_sec"]
        # Poop is a FRACTION OF THE PLAYER'S TOTAL DPS, not a flat number -- as a
        # flat number it measured 0.0% of total DPS at every ten-minute sample of
        # a full run. v5 moved it from the aura to the whole player, since the
        # aura became one of two damage sources and a poop tracking half the
        # curve would halve itself. player_dps() already carries the Reawaken
        # multiplier, so it is deliberately not applied again here.
        return (
            self.player_dps()
            * self.g.combat["poop_aura_fraction"]
            * 1.0  # poop_power_multiplier — nothing targets it
            * (1.0 + 0.0)  # poop_additive_damage_sum — no source exists yet
            * (self.g.branches["unicorn"].effect_at_level(self.branch_levels["unicorn"])
               if self.branch_levels.get("unicorn", 0) > 0 else 1.0)
            * duty
        )

    def prism_yield_multiplier(self) -> float:
        """Spectrum's green status, as an expected value over all kills.

        The beam is one damage source among several, so only its SHARE of kills
        gets the bonus. Approximated by the beam's share of total DPS, which is
        what the aggregate model can honestly say - a per-kill attribution would
        need the field sim.
        """
        if self.branch_levels.get("unlock_prism", 0) <= 0:
            return 1.0
        lv = self.branch_levels.get("spectrum", 0)
        if lv <= 0:
            return 1.0
        # total_dps_before_shards(), NOT total_dps(): the latter reaches back
        # through prism_shard_dps() into this same stack and turns one damage
        # calculation into several.
        total = self.total_dps_before_shards()
        if total <= 0.0:
            return 1.0
        share = self.prism_beam_dps() / total
        bonus = self.g.branches["spectrum"].effect_at_level(lv) - 1.0
        return 1.0 + share * bonus

    def nine_life_card_bonus(self) -> int:
        """Curiosity: one more card in every hand, permanently. A skill-tree
        node since 2026-09-14 (the Nine Lives layer was cut)."""
        return 1 if self.branch_levels.get("curiosity", 0) > 0 else 0

    def achievement_stardust(self) -> float:
        """Stardust the achievement roster has paid out by now.

        Achievements pay Stardust (designer decision), and because the permanent
        multiplier keys off stardust EARNED, that is real power the boss curve
        has to be sized against. Modelled as a smooth ramp against run progress
        rather than per-achievement: the roster is thresholded on kills, Catnip,
        Reawakens, bosses and upgrade levels, all of which advance together, so
        a per-entry model would add a lot of machinery to land in the same
        place. Deliberately conservative — it pays out no FASTER than progress.
        """
        if ACHIEVEMENT_STARDUST_TOTAL <= 0.0:
            return 0.0
        cleared = len(self.bosses_defeated)
        fraction = cleared / float(len(self.g.tiers))
        return ACHIEVEMENT_STARDUST_TOTAL * fraction

    def credit_achievement_stardust(self) -> float:
        """Pay out whatever the roster owes since the last call, and bank it.

        achievement_stardust() has been in this file since the feature landed and
        NOTHING EVER CALLED IT, so every simulated run has been missing the
        roster's contribution to `stardust_granted_ever` - and that feeds the
        permanent multiplier, so the sim was understating late power. This is the
        project's own recurring bug: "if a system pays the player, assert the
        BALANCE moves" (pawpocalypse.md, Achievements).

        Credited to BOTH the spendable balance and granted-ever, matching
        GameState.grant_achievement_stardust().
        """
        owed = self.achievement_stardust() - self._achievement_stardust_paid
        if owed <= 0.0:
            return 0.0
        self._achievement_stardust_paid += owed
        self.stardust += owed
        self._stardust_granted_ever += owed
        return owed

    def pounce_dps_sustained(self) -> float:
        """Pounce (TierController._tick_pounce), amortised over its cooldown.

        Modelled as damage-on-cooldown with NO vulnerable-window bonus: the sim
        has no model of player timing, and crediting a x5 the player might miss
        would size every boss against skill this cannot verify. So the ability
        is worth strictly more in the real game than here, which is the safe
        direction for a pacing dial to be wrong in.
        """
        c = self.g.combat
        reduction = min(
            self.g.upgrades["pounce_cooldown"].effect_total(self.upgrade_levels["pounce_cooldown"]),
            self.g.upgrades["pounce_cooldown"].effective_cap(
                self.floors_reached(), self.reawaken_count),
        )
        cooldown = c.get("pounce_cooldown_sec", 0.0) * (1.0 - reduction)
        if cooldown <= 0.0:
            return 0.0
        # An unbought Pounce does not fire at all (TierController._tick_pounce
        # returns early), so it contributes no DPS rather than a small amount.
        if not self.is_pounce_owned():
            return 0.0
        power = self.g.upgrades["pounce_power"].effect_total(self.upgrade_levels["pounce_power"])
        if self.branch_levels.get("pounce", 0) > 0:
            power *= self.g.branches["pounce"].effect_at_level(self.branch_levels["pounce"])
        # REFLEX NO LONGER MULTIPLIES ANYTHING HERE, and it should not have.
        #
        # This read `extra_at_level("offline", ...)` — the branch's OFFLINE
        # EARNINGS channel, x1.5 — and multiplied pounce power by it. The game
        # never did that: _reflex_scale() spent that number on offline accrual
        # and nothing else, so the sim was over-rating Pounce by 50% against a
        # number that had nothing to do with damage.
        #
        # Offline accrual was removed on 2026-09-10 and the channel with it, so
        # the read would now return 0.0 and quietly do nothing. Deleted rather
        # than left dead. If Reflex should scale the burst by its OWN effect
        # (x1.1), that is a real change to the sim's numbers and a separate
        # decision — see the project rule about prices never moving.
        multiple = c.get("pounce_damage_seconds", 0.0) * power / cooldown
        # Mirrors CombatResolver.pounce_cap_scale(). The cap is on the PRODUCT of
        # the dials, so it lands here as a clamp on the sustained aura multiple
        # rather than on any one upgrade.
        cap = c.get("pounce_max_aura_multiple", 0.0)
        if cap > 0.0:
            multiple = min(multiple, cap)
        return self.player_dps() * multiple

    def prism_beam_dps(self) -> float:
        """GameState.get_prism_beam_dps() - a SHARE OF AURA DPS, never flat.

        Chaining is not modelled as extra damage, and that is correct rather
        than lazy: Refraction moves the same beam onto another target, so it
        raises COVERAGE, not output. It is priced in coverage() with the rest of
        the reach terms.
        """
        if self.branch_levels.get("unlock_prism", 0) <= 0:
            return 0.0
        return self.aura_dps() * self.g.branches["unlock_prism"].effect_at_level(
            self.branch_levels["unlock_prism"])

    def total_dps(self) -> float:
        """The whole damage stack, computed in ONE pass.

        Critical Mass is folded in here rather than living in its own method,
        because it is a SHARE of this total and a separate method meant every
        caller recomputed the stack two or three times over. The greedy buyer
        calls this once per candidate purchase per tick, so that turned a
        sixty-second verification run into a six-minute one.
        """
        pre = self.total_dps_before_shards()
        lv = self.branch_levels.get("critical_mass", 0)
        if lv <= 0 or self.branch_levels.get("unlock_prism", 0) <= 0 or pre <= 0.0:
            return pre
        # Ten beam kills detonate for a share of aura DPS against everything.
        # Credited against one target's worth, not the whole board: the model has
        # no board, and over-crediting an AoE is how one term ends up wearing the
        # whole curve (the Pounce lesson).
        beam_share = self.prism_beam_dps() / pre
        payload = self.aura_dps() * self.g.branches["critical_mass"].effect_at_level(lv)
        return pre + payload * beam_share / PRISM_SHARDS_PER_DETONATION

    def total_dps_before_shards(self) -> float:
        """total_dps() without the shard term, to keep the two from recursing."""
        return (
            self.player_dps()
            + self.kitten_count() * self.kitten_dps_each()
            + self.poop_dps_sustained()
            + self.pounce_dps_sustained()
            + self.prism_beam_dps()
        )

    # --- throughput --------------------------------------------------------

    def spawn_rate_per_sec(self, tier: Tier) -> float:
        """Mirrors TierController._spawn_rate_per_sec(): the biome's authored
        rate times the Spawn Rate shop line, and nothing else.

        The v4 Phase 4 power lift (authored pushed toward ambient DPS / avg HP,
        capped at 2x, with a capacity ceiling and an absolute floor) was taken
        out on 2026-09-14 at the designer's request: density belongs to the
        biome, not to the cat. The authored rates were doubled in the same pass
        because a log of this function over the whole run showed the lift
        pinned at 2.0x in every biome, so the pace of a player keeping up is
        unchanged. The sim does not model the Swarm node, Infestation or the
        Ratcatcher job; the game multiplies those in on top.
        """
        u = self.g.upgrades["spawn_rate"]
        mult = u.clamped(self.upgrade_levels["spawn_rate"],
                         self.floors_reached(), self.reawaken_count)
        return max(tier.spawn_rate_per_min * mult / 60.0, 1e-4)

    def overkill_fraction(self) -> float:
        """Mirrors GameState._rebuild_stats(): the shop line, plus the Overkill
        skill branch on top, both clamped to the line's own cap."""
        u = self.g.upgrades["overkill"]
        floors = self.floors_reached()
        frac = min(u.effect_total(self.upgrade_levels["overkill"]),
                   u.effective_cap(floors, self.reawaken_count))
        if self.is_overkill_owned():
            frac = min(
                frac + (self.g.branches["overkill"].effect_at_level(
                    self.branch_levels["overkill"])
                    if self.branch_levels.get("overkill", 0) > 0
                    else self.g.branches["unlock_overkill"].effect_at_level(
                        self.branch_levels["unlock_overkill"])),
                u.effective_cap(floors, self.reawaken_count),
            )
        return frac

    def overkill_bounty_multiple(self, tier: Tier, aura_on_boss: bool = False) -> float:
        """How many times its own reward a corpse pays back as Overkill bounty.

        Mirrors CombatResolver.overkill_bounty(). In the game the input is one
        killing blow's overshoot; here it is the aggregate the same quantity
        averages to — damage the player could spend per available mouse, over
        that mouse's HP, minus the one mouse-worth that actually killed it.
        That ratio IS the surplus the playtest measured at 9.4x, so this term is
        precisely "the wasted part of the surplus, converted".
        """
        frac = self.overkill_fraction()
        if frac <= 0.0:
            return 0.0
        killed = self.kill_rate_per_sec(tier, aura_on_boss)
        if killed <= 0.0:
            return 0.0
        dps = self.kitten_count() * self.kitten_dps_each() if aura_on_boss else self.total_dps()
        supported = dps / max(tier.avg_hp, 1e-9)
        surplus = supported / killed
        wasted = max(surplus - 1.0, 0.0)
        # Mirrors CombatResolver.overkill_bounty()'s two forms. Kept here rather
        # than only in pawsim2 because a stale mirror invalidates every run this
        # file produces, silently.
        exponent = self.g.combat.get("overkill_bounty_exponent", 0.0)
        if exponent > 0.0:
            return frac * min(wasted, wasted ** exponent)
        return frac * min(wasted, self.g.combat["overkill_bounty_max_multiple"])

    def currency_find(self) -> float:
        """Mirrors GameState.get_currency_find_multiplier()."""
        u = self.g.upgrades["currency_find"]
        # clamped(), matching that function's _capped_effect. See paw_dps().
        return u.clamped(self.upgrade_levels["currency_find"],
                         self.floors_reached(), self.reawaken_count)

    def fortune_yield(self) -> float:
        """GameState.get_fortune_yield_multiplier() — the Fortune branch (Phase 7).

        Guarded on level > 0 like the game does: effect_at_level(0) is the first
        authored entry, not 1.0, so an unowned branch would otherwise hand out a
        bonus nobody bought.
        """
        if self.branch_levels.get("fortune", 0) <= 0:
            return 1.0
        return self.g.branches["fortune"].effect_at_level(
            self.branch_levels["fortune"])

    def golden_chance(self, tier: Tier) -> float:
        """GameState.golden_chance_for() — the biome's own chance plus the Luck
        branch, clamped. A biome with no golden authored stays at zero."""
        if tier.golden is None or tier.golden_chance <= 0.0:
            return 0.0
        bonus = 0.0
        if self.branch_levels.get("fortune", 0) > 0:
            bonus = self.g.branches["fortune"].extra_at_level(
                "golden", self.branch_levels["fortune"])
        return min(tier.golden_chance + bonus, MAX_GOLDEN_CHANCE)

    def coverage(self, tier: "Tier | None" = None) -> float:
        """Share of spawns actually killed. 1.0 unless COVERAGE_MODEL is on.

        Driven by kill-capacity headroom (see COVERAGE_MODEL above), with a
        smaller term for the lines that literally extend reach. Takes the tier
        because headroom is per-biome: the same cat is saturated in biome 2 and
        struggling in biome 8.
        """
        if not COVERAGE_MODEL:
            return 1.0
        floors = self.floors_reached()
        rw = self.reawaken_count
        aura_m = 1.0
        if self.aura_owned():
            aura_m = self.g.upgrades["aura_radius"].clamped(
                self.upgrade_levels["aura_radius"], floors, rw)
            # The Aura Reach BRANCH multiplies the same radius, and this model
            # referenced it nowhere at all - so buying it changed no number the
            # ranker could see. It was unbuyable, not unattractive.
            reach_lv = self.branch_levels.get("aura_reach", 0)
            if reach_lv > 0:
                aura_m *= self.g.branches["aura_reach"].effect_at_level(reach_lv)
        kit_m = self.g.upgrades["kitten_speed"].clamped(
            self.upgrade_levels["kitten_speed"], floors, rw)
        reach = COVERAGE_PER_MULT * (max(aura_m - 1.0, 0.0) + max(kit_m - 1.0, 0.0))

        # THE AMBUSH TRUNK (v7). Without a term here the greedy buyer cannot see
        # it and would never buy it - the exact way `aura_reach` sat unbought in
        # every run under every ruleset because no term read it.
        #
        # Expressed as a share of the REMAINING gap to 1.0, not as a flat bonus
        # against a ceiling. A hard clamp made the whole trunk worthless the
        # instant the unlock was bought: coverage hit the cap, every further rung
        # scored exactly zero marginal gain, and the buyer bought `unlock_snare`
        # and then none of its three ladders.
        #
        # Calibrated against the field sim with the trunk owned:
        #   40% slow            +6.0pp at minute 60, +3.2pp at 180
        #   80% slow + 3 bounce +27.8pp / +16.8pp
        ambush = 0.0
        # REFRACTION. The beam jumping to the next enemy is reach, not output -
        # it kills things the cat never went near. Same bucket as aura radius.
        if self.branch_levels.get("unlock_prism", 0) > 0:
            fr = self.branch_levels.get("refraction", 0)
            if fr > 0:
                reach += PRISM_PER_CHAIN * self.g.branches["refraction"].effect_at_level(fr)

        if self.branch_levels.get("unlock_full_spectrum", 0) > 0:
            reach += PRISM_SWEEP_REACH

        snare_lv = self.branch_levels.get("unlock_snare", 0)
        if snare_lv > 0:
            slow = self.g.branches["unlock_snare"].effect_at_level(snare_lv)
            dw = self.branch_levels.get("deadweight", 0)
            if dw > 0:
                slow = self.g.branches["deadweight"].effect_at_level(dw)
            ambush = AMBUSH_PER_SLOW * slow
            fn = self.branch_levels.get("funnel", 0)
            if fn > 0:
                ambush += AMBUSH_PER_BOUNCE * self.g.branches["funnel"].effect_at_level(fn)

        if tier is None:
            tier = self.best_tier()
        spawn = self.spawn_rate_per_sec(tier)
        if spawn <= 0.0 or tier.avg_hp <= 0.0:
            return min(1.0, COVERAGE_MAX + reach_bonus)
        headroom = (self.total_dps() / tier.avg_hp) / spawn
        span = COVERAGE_MAX - COVERAGE_MIN
        z = max(-60.0, min(60.0, -COVERAGE_SLOPE * (headroom - COVERAGE_KNEE)))
        base = COVERAGE_MIN + span / (1.0 + math.exp(z))
        # Both bonuses close a share of what is still escaping, so neither can
        # push coverage past 1.0 and neither ever has zero marginal value.
        closed = 1.0 - math.exp(-(reach + ambush))
        return max(COVERAGE_MIN, min(1.0, base + (1.0 - base) * closed))

    def catnip_per_sec(self, tier: Tier, aura_on_boss: bool = False) -> float:
        """Kills are capped by spawn rate (review assumption).

        When the player is committed to the boss, aura damage is going into
        the boss, so ambient income is kitten-only — which is what
        TierController actually does: the boss spawns ALONGSIDE normal
        ambient mice (tier_controller.gd:16-21), kittens keep hunting the
        pooled Enemy array, and aura follows the cursor.

        OVERKILL IS DELIBERATELY NOT MODELLED HERE, and that is a real gap
        worth stating rather than papering over with a fudge factor.

        Overkill (v3 review F7) splashes a killing blow's wasted damage onto
        the nearest surviving mouse. Under this function's assumptions it is
        provably worth zero: `min(..., spawn_rate_per_sec)` says the player
        already kills every mouse that spawns, and Overkill cannot raise a cap
        that is about spawning rather than killing. The regime where it would
        pay — per-frame damage exceeding a mouse's whole HP pool while still
        being kill-limited rather than spawn-limited — needs a spawn rate above
        3,600/min, and the game's maximum is 324/min.

        Its real value is COVERAGE, which this model does not have. The header's
        "perfect targeting" assumption means every spawned mouse is assumed
        killed; the actual game lets mice wander out of `play_bounds` and
        despawn for no reward (tier_controller._on_enemy_despawned), and a
        splash can reach a mouse outside the aura radius. So the shipped game
        is somewhat FASTER than this sim reports, by an amount that needs
        playtest data to pin down. Sizing boss HP against this model is
        therefore mildly conservative, which is the safe direction to err.
        """
        return (self.kill_rate_per_sec(tier, aura_on_boss)
                * self.avg_whole_catnip(tier)
                * self.prism_yield_multiplier())

    def avg_whole_catnip(self, tier: Tier) -> float:
        """Expected Catnip per kill after GameState.whole_catnip() rounding.

        Rounding is PER KILL in the game, so it has to be applied per enemy type
        here and then averaged — not applied to `tier.avg_catnip`, which would
        round the average and get Tier 1 wrong in both directions at once.

        This is not a rounding detail. A Tier 1 common pays 0.33 and the floor of
        1 makes it pay 1, so Tier 1 income roughly TRIPLES; Tiers 2-8 barely move
        because their rewards already exceed 1. Mirrors game_state.whole_catnip().
        """
        cf = self.currency_find()
        g = self.golden_chance(tier)
        # Overkill pays a share of the damage that overshot the victim, priced
        # in that victim's OWN reward — so it rides the same per-enemy value the
        # rounding is applied to, rather than being added afterwards.
        bounty = 1.0 + self.overkill_bounty_multiple(tier)
        total = sum(w for _e, w in tier.pool)
        ordinary = 0.0
        if total > 0.0:
            # A flier is only killed FLIER_CATCH_RATE of the time; everything
            # else in the pool is a certain kill once the aura can chew it.
            ordinary = sum(
                whole_catnip(e.reward_catnip * bounty * cf)
                * w * (FLIER_CATCH_RATE if e.is_flier else 1.0)
                for e, w in tier.pool) / total
        # The deeper-is-richer term (TierData.reward_multiplier). Applied to the
        # whole payout, golden included, exactly as GameState.add_currency does.
        # Fortune multiplies the whole payout, exactly where GameState does.
        rm = getattr(tier, "reward_multiplier", 1.0) * self.fortune_yield()
        if g <= 0.0:
            return ordinary * rm
        return ((1.0 - g) * ordinary
                + g * whole_catnip(tier.golden.reward_catnip * cf) * GOLDEN_CATCH_RATE) * rm

    def kill_rate_per_sec(self, tier: Tier, aura_on_boss: bool = False) -> float:
        """Mice killed per second, capped by how fast they spawn.

        Extracted from catnip_per_sec (whose docstring carries the assumptions)
        so the v4 Catches can read the SAME number the income model does.
        Two independent estimates of the kill rate would drift apart the moment
        either one was tuned, and the drift would be silent.
        """
        dps = self.kitten_count() * self.kitten_dps_each() if aura_on_boss else self.total_dps()
        # Coverage applies to the SPAWN-bound branch only: it is about mice
        # walking off unrewarded, and a kill-bound player is already failing to
        # keep up rather than failing to reach. Same split pawsim2 uses.
        return min(dps / tier.avg_hp, self.spawn_rate_per_sec(tier) * self.coverage(tier))

    def utility(self, tier: Tier) -> float:
        """Purchase-ranking objective.

        Income alone is the wrong objective under boss gating. Ambient kills
        are spawn-capped almost immediately (Tier 1 caps at 20/min against a
        starting 0.96 kills/sec of damage), so once capped, a damage upgrade
        adds exactly zero catnip/sec and a pure-income ranker would never buy
        one — yet damage is precisely what opens the next tier now.

        The geometric mean keeps both terms load-bearing: a purchase has to
        improve income or damage without collapsing the other to score well.
        """
        return math.sqrt(max(self.catnip_per_sec(tier), 1e-9) * max(self.total_dps(), 1e-9))

    def unlocked_tiers(self) -> list:
        """Biomes the player may stand in right now.

        TWO gates, not one (2026-08-19). The evolution stage still opens blocks
        of three, and on top of that each biome has a `catnip_to_unlock`
        threshold measured against Catnip earned THIS RUN.

        The second gate is what the simulator was missing entirely: it used to
        jump 1 -> 3 -> 6 -> 8 the instant a boss died, because nothing stopped
        it, so every claim about how long a biome takes to open was a claim
        about a rule the sim did not implement.
        """
        stage = self.evolution_stage()
        # Territory admits its own biomes regardless of the Catnip gate. The
        # game does not merely PERMIT the player to be there, it puts them
        # there: `_on_reawakened` travels to `REALMS[start_index - 1]` without
        # consulting `is_biome_unlocked()`. Modelling the floor without this
        # leaves the simulated player standing somewhere the real one never is.
        floor_index = self.starting_biome_index()
        # THIRD gate as of 2026-08-27: passage past a boss biome re-locks every
        # cycle. Exempted below the Territory floor for the same reason the
        # Catnip gate is — `_on_reawakened` TRAVELS the player there outright
        # without consulting is_biome_unlocked(), so modelling the floor without
        # the exemption leaves the simulated player somewhere the real one is
        # standing.
        return [t for t in self.g.tiers
                if t.required_evolution_stage <= stage
                and (self.bosses_cleared_for_stage(t.required_evolution_stage)
                     or t.index <= floor_index)
                and (self.lifetime_catnip_this_cycle >= t.catnip_to_unlock
                     or t.index <= floor_index)]

    def floors_reached(self) -> int:
        """Deepest biome index entered this run, 0-based — GameState.floors_reached().

        A high-water mark: dropping back to farm somewhere easy must not
        confiscate ceilings already paid for.
        """
        return max(getattr(self, "_deepest_biome", 1) - 1, 0)

    def best_tier(self) -> Tier:
        """The biome a sensible player would be standing in.

        Deepest-unlocked used to be the right answer, because biomes unlocked
        one at a time and the newest was both the deepest and the only new
        choice. v4 Phase 2 unlocks them in BLOCKS of three, and deepest-unlocked
        then sends a brand-new player straight into biome 3, whose mice have
        orders of magnitude more HP than their damage can chew. That is not an
        opening any player would choose, so modelling it would make every
        number downstream a fiction about a game nobody plays.

        Income is the honest proxy: a player farms where the Catnip is.
        """
        # Territory sets a FLOOR on where a run starts, so biomes below it are
        # not places this player goes any more.
        choices = [t for t in self.unlocked_tiers() if t.index >= self.starting_biome_index()]
        if not choices:
            choices = self.unlocked_tiers()
        return max(choices, key=self.catnip_per_sec)

    def next_boss_tier(self):
        """The next boss the player could go and fight, or None.

        Separate from best_tier() for the same reason: the biome you FARM and
        the biome you FIGHT IN are no longer the same place. A player farms
        biome 2 and travels to biome 3 to spend their bait.

        Targets the next boss not cleared THIS CYCLE, not the next never-beaten
        one (2026-08-28). Passage re-locks on Reawaken, so a boss killed two
        cycles ago is standing in the road again and has to be re-baited.
        """
        for tier in self.unlocked_tiers():
            if (tier.boss is not None
                    and tier.index not in self.bosses_cleared_this_cycle):
                return tier
        return None

    def bait_cost(self, tier) -> float:
        """The CATNIP price of a summon. Not the gate — see is_bait_unlocked().

        Measured 2026-08-27 at the moment each fight is engaged, this had decayed
        to 8 seconds of income for the first boss and 0.00 min for every one
        after, which is why Bait exists.
        """
        return float(tier.boss.get("bait_cost_catnip", 0.0)) if tier.boss else 0.0

    # --- the Bait gate, mirroring GameState --------------------------------

    def bait_count_required(self, tier) -> int:
        """BossData.bait_required — 40 on every boss, and it never scales."""
        return int(tier.boss.get("bait_required", 0) or 0) if tier and tier.boss else 0

    def kills_per_bait(self, tier) -> int:
        """BossData.kills_per_bait. The per-biome difficulty lives here so the
        readout the player learns is always `x / 40`."""
        return max(int(tier.boss.get("kills_per_bait", 1) or 1), 1) if tier and tier.boss else 1

    def bank_kills(self, tier, n: float) -> None:
        """Kills bank in the biome they happened in — GameState.tier_kill_count,
        banked to tier_kills_by_biome on leaving and restored on arriving."""
        if tier is None or n <= 0.0:
            return
        self.tier_kills_by_biome[tier.index] = (
            self.tier_kills_by_biome.get(tier.index, 0.0) + n)

    def kills_banked(self, tier) -> float:
        return self.tier_kills_by_biome.get(tier.index, 0.0) if tier else 0.0

    def bait_earned(self, tier) -> int:
        """Mirrors GameState.bait_earned(): a DIVISOR on the kill bank, floored.

        Integer division on purpose — the game returns `kills / per` on ints, so
        39.9 baits' worth of kills is 39 baits, not 40.
        """
        if tier is None or not tier.boss:
            return 0
        return int(self.kills_banked(tier) // self.kills_per_bait(tier))

    def bait_held(self, tier) -> int:
        """Mirrors GameState.bait_held(): produced less spent, never negative."""
        if tier is None or not tier.boss:
            return 0
        return max(self.bait_earned(tier)
                   - int(self.bait_spent_by_biome.get(tier.index, 0)), 0)

    def is_bait_unlocked(self, tier) -> bool:
        """Mirrors GameState.is_bait_unlocked() — bait in hand, not waves done."""
        if tier is None or not tier.boss:
            return False
        return self.bait_held(tier) >= self.bait_count_required(tier)

    def spend_bait(self, tier) -> None:
        """Mirrors GameState.buy_bait()'s bait half. ALL-OR-NOTHING: the caller
        must have checked the Catnip too, because the game checks both before
        spending either — a summon refused for being one Catnip short used to
        eat forty bait."""
        if tier is None or not tier.boss:
            return
        self.bait_spent_by_biome[tier.index] = (
            self.bait_spent_by_biome.get(tier.index, 0)
            + self.bait_count_required(tier))

    def bosses_cleared_for_stage(self, required_stage: int) -> bool:
        """Mirrors GameState._bosses_cleared_for_stage().

        A biome wanting evolution stage N needs the first N-1 bosses beaten THIS
        CYCLE. Without this the whole bait economy is optional: bosses_defeated
        persists, so from the second cycle on a player could walk past every
        boss biome and never bait anything again.
        """
        boss_indices = [t.index for t in self.g.tiers if t.boss]
        for i in range(min(required_stage - 1, len(boss_indices))):
            if boss_indices[i] not in self.bosses_cleared_this_cycle:
                return False
        return True

    def mark_boss_defeated(self, index: int) -> None:
        """Mirrors GameState.mark_boss_defeated(): the permanent ledger AND this
        cycle's passage, together. The game records the cycle clear even when the
        boss was already permanently defeated — every rematch after the first
        would otherwise be a no-op that opened nothing."""
        self.bosses_defeated.add(index)
        self.bosses_cleared_this_cycle.add(index)

    def bait_farm_tier(self, boss_tier):
        """Where a player committed to the next boss has to stand.

        THE BEHAVIOURAL HEART OF THE 2026-08-27 CHANGE, and the reason this is
        not merely another gate on engagement: bait drops only in the boss's own
        biome, so a player who wants the fight must go and farm THERE, however
        much better the income is somewhere else. Before this the sim farmed the
        richest biome and the boss opened on a timer it never paid for.
        """
        if boss_tier is None or not boss_tier.boss:
            return None
        if self.is_bait_unlocked(boss_tier):
            return None
        return boss_tier

    # --- spending ----------------------------------------------------------

    def buy_upgrades(self, tier: Tier, reserve: float = 0.0) -> int:
        """Greedy: repeatedly buy the affordable upgrade with the best
        marginal catnip/sec gained per Catnip spent.

        `reserve` is Catnip the player is deliberately NOT spending, because
        they are saving for a boss's Bait (v4 Phase 2). Without it this buyer
        spends to zero every tick and could never accumulate a price worth
        ten minutes of income, so the bait would only ever become affordable
        once one tick of income covered it — which is to say, never as a
        decision, only as an eventual accident.
        """
        bought = 0
        while True:
            base = self.utility(tier)
            best, best_ratio, best_cost = None, 0.0, 0.0
            for uid, u in self.g.upgrades.items():
                lv = self.upgrade_levels[uid]
                if u.at_cap(lv, self.floors_reached(), self.reawaken_count):
                    continue
                # GameState.is_kitten_power_locked()
                if uid in ("kitten_power", "kitten_speed") and self.kitten_count() <= 0:
                    continue
                # GameState.is_upgrade_visible() — the line does not exist until
                # its skill-tree branch is bought.
                if not self.is_upgrade_visible(uid):
                    continue
                cost = u.cost_for_level(lv)
                if cost > self.catnip - reserve:
                    continue
                self.upgrade_levels[uid] += 1
                gain = self.utility(tier) - base
                self.upgrade_levels[uid] -= 1
                if gain <= 0.0:
                    continue
                ratio = gain / cost
                if ratio > best_ratio:
                    best, best_ratio, best_cost = uid, ratio, cost
            if best is None:
                return bought
            self.catnip -= best_cost
            self.upgrade_levels[best] += 1
            self.buy_log.append((best, best_cost, (best_ratio * best_cost) / max(base, 1e-12)))
            bought += 1

    def buy_branches(self) -> int:
        """Greedy on marginal catnip/sec per STARDUST.

        Under LEGACY_MYSTIQUE_GATE, Mystique is bought first unconditionally —
        it was the only thing that unlocked tiers and a tier jump dwarfs any
        damage node. With boss gating it is an ordinary damage branch and
        competes on merit like everything else.
        """
        bought = 0
        while True:
            if LEGACY_MYSTIQUE_GATE:
                myst = self.g.branches["claws"]
                mlv = self.branch_levels["claws"]
                if mlv < myst.max_level() and myst.cost_for_next_level(mlv) <= self.stardust:
                    self.stardust -= myst.cost_for_next_level(mlv)
                    self.branch_levels["claws"] += 1
                    bought += 1
                    continue

            tier = self.best_tier()
            base = self.utility(tier)
            best, best_ratio, best_cost = None, 0.0, 0.0
            for bid, b in self.g.branches.items():
                if bid == "claws" and LEGACY_MYSTIQUE_GATE:
                    continue
                if bid == "unlock_pounce" and not self.is_pounce_unlocked():
                    continue
                if bid == "unlock_overkill" and not self.is_overkill_unlocked():
                    continue
                if bid == "unicorn" and not self.is_unicorn_unlocked():
                    continue
                # The per-branch Reawaken ladder. Without this the sim buys
                # eleven branches on the first Reawaken that the game locks.
                if self.reawaken_count < b.required_reawakens:
                    continue
                # v6 prerequisites: a ladder cannot be bought before the
                # unlock it hangs off. SkillTreeManager._is_branch_purchasable().
                if not self.branch_prereq_met(bid):
                    continue
                lv = self.branch_levels[bid]
                if lv >= b.max_level():
                    continue
                cost = b.cost_for_next_level(lv)
                if cost > self.stardust:
                    continue
                self.branch_levels[bid] += 1
                gain = self.utility(tier) - base
                if gain <= 0 and lv == 0:
                    # A GATE (2026-10-01): a node that does nothing itself but
                    # opens the ones under it - the Unicorn, since it became
                    # its own limb with Rainbow Poop beneath. The greedy rate
                    # ranker sees zero and would never buy it, so it is priced
                    # as a bundle with its best child: the pair's gain over
                    # the pair's cost, bought one step at a time.
                    gain, cost = self._gate_bundle(bid, cost, tier, base)
                self.branch_levels[bid] -= 1
                ratio = (gain / cost) if gain > 0 else 0.0
                if ratio > best_ratio:
                    best, best_ratio, best_cost = bid, ratio, b.cost_for_next_level(lv)
            if best is None:
                # Nothing left that raises the RATE. Legacy is invisible to the
                # greedy ranker by construction — it changes where the next run
                # STARTS, not how fast this one goes, so utility() (a rate) sees
                # exactly zero gain and would never buy it however cheap it got.
                #
                # This is a deliberate heuristic, not an optimisation: spare
                # Stardust goes to the cheapest Legacy rung. It mirrors what a
                # player actually does with prestige currency once the damage
                # ladders are bought out, and without it the simulator would
                # report Legacy as worthless when what it really means is
                # "I cannot price deferred value".
                if self._buy_cheapest_legacy():
                    bought += 1
                    continue
                return bought
            self.stardust -= best_cost
            self.branch_levels[best] += 1
            self._note_unlock(best)
            bought += 1

    def _note_unlock(self, bid: str) -> None:
        """The Reawaken count at which an unlock was FIRST bought, for the
        runway line. Recorded at purchase because the old line assumed fixed
        Reawaken gates (1, 2, 3) that have all been 0 since 2026-08-23."""
        if not hasattr(self, "unlocked_at"):
            self.unlocked_at = {}
        if self.branch_levels.get(bid, 0) == 1 and bid not in self.unlocked_at:
            self.unlocked_at[bid] = self.reawaken_count

    def _gate_bundle(self, gate: str, gate_cost: float, tier, base: float):
        """(gain, cost) of `gate` bought together with its best child. Called
        with the gate's level already raised by one."""
        best_gain, best_cost = 0.0, gate_cost
        for cid, c in self.g.branches.items():
            if c.parent != gate or self.branch_levels[cid] > 0:
                continue
            if self.reawaken_count < c.required_reawakens:
                continue
            ccost = c.cost_for_next_level(0)
            if ccost < 0 or gate_cost + ccost > self.stardust:
                continue
            self.branch_levels[cid] += 1
            g = self.utility(tier) - base
            self.branch_levels[cid] -= 1
            if g > 0 and g / (gate_cost + ccost) > best_gain / best_cost:
                best_gain, best_cost = g, gate_cost + ccost
        return best_gain, best_cost

    def branch_prereq_met(self, bid: str) -> bool:
        """SkillTreeManager._is_branch_purchasable() — the v6 parent rule.

        A ladder hangs off the unlock that grants its ability and cannot be
        bought before it. Trunks are independent of each other on purpose,
        so only `parent_is_required` edges gate anything.
        """
        b = self.g.branches.get(bid)
        if b is None or not b.parent_is_required or not b.parent:
            return True
        return self.branch_levels.get(b.parent, 0) > 0

    LEGACY_BRANCHES = ("legacy",)

    def _buy_cheapest_legacy(self) -> bool:
        best, best_cost = None, math.inf
        for bid in self.LEGACY_BRANCHES:
            b = self.g.branches[bid]
            lv = self.branch_levels[bid]
            if lv >= b.max_level():
                continue
            # The Reawaken ladder applies here too. This is a SECOND buy path,
            # added because utility() cannot price Legacy, and it happily bought
            # gated branches the main ranker had correctly skipped — the sim
            # reported Legacy levels at two Reawakens against a gate of four.
            if self.reawaken_count < b.required_reawakens:
                continue
            # Same v6 prerequisite rule as the greedy ranker above. This is
            # the SECOND purchase path, and a rule in one and not the other
            # is how the simulator comes to disagree with the game.
            if not self.branch_prereq_met(bid):
                continue
            cost = b.cost_for_next_level(lv)
            if cost <= self.stardust and cost < best_cost:
                best, best_cost = bid, cost
        if best is None:
            return False
        self.stardust -= best_cost
        self.branch_levels[best] += 1
        self._note_unlock(best)
        return True

    # --- reawaken ----------------------------------------------------------

    def pending_stardust(self) -> float:
        """Mirrors GameState.get_pending_stardust() — review fix 1."""
        total = math.floor(self.g.reawaken["reawaken_stardust_coefficient"]
                           * math.sqrt(self.lifetime_catnip_ever))
        return max(0.0, total - self._stardust_granted_ever)

    def reawaken_requirement(self) -> float:
        """Mirrors GameState.get_reawaken_requirement().

        The gate is priced in Stardust, not Catnip. The old Catnip threshold was
        computed from a different quantity than the payout, so it unlocked while
        the reset was still a net loss -- Reawaken #2 unlocked paying 5 Stardust
        on a balance of 1,059, a +0% gain for wiping every base upgrade. See
        ReawakenConstantsData for the full measurement.
        """
        return max(
            self.g.reawaken["reawaken_min_stardust"],
            self._stardust_granted_ever * self.g.reawaken["reawaken_min_gain_fraction"],
        )

    def can_reawaken(self) -> bool:
        return self.pending_stardust() >= self.reawaken_requirement()

    def reawaken(self) -> tuple:
        """Mirrors GameState.reawaken().

        ONE grant now: Stardust. The old Catnip grant is gone — Catnip is what
        kills pay, and Stardust is what Reawakening pays.
        """
        p = self.g.reawaken
        if FIX_F1:
            # Review fix 1: split-invariant. Grant against lifetime-ever, minus
            # what has already been granted. Shares one implementation with the
            # gate (pending_stardust) so the two can never drift apart.
            sd = self.pending_stardust()
        else:
            sd = math.floor(p["reawaken_stardust_coefficient"] * math.sqrt(self.lifetime_catnip_this_cycle))

        self.stardust += sd
        self._stardust_granted_ever += sd
        # Legacy run accelerators (v4 Phase 5). Mirrors GameState.reawaken():
        # a share of Catnip survives, and every VISIBLE shop line restarts at a
        # floor level rather than at zero. Hidden lines get nothing, or Muscle
        # Memory would hand out levels of an ability the player cannot use.
        self.catnip = math.floor(self.catnip * self.catnip_carryover())
        floor_level = self.starting_upgrade_level()
        for k in self.upgrade_levels:
            self.upgrade_levels[k] = (
                floor_level if floor_level > 0 and self.is_upgrade_visible(k) else 0
            )
        self.lifetime_catnip_this_cycle = 0.0
        # THE BAIT IS RE-EARNED EVERY CYCLE (2026-08-27). Mirrors
        # GameState.reawaken(), which clears all three together. bosses_defeated
        # is deliberately NOT among them: the player keeps every evolution stage
        # and is never walked backwards, they just lose the right to skip the
        # fight.
        self.tier_kills_by_biome.clear()
        self.bait_spent_by_biome.clear()
        self.bosses_cleared_this_cycle.clear()
        self.reawaken_count += 1
        return sd, 0.0

    # --- Legacy accelerators, mirroring GameState ---------------------------

    def catnip_carryover(self) -> float:
        return min(max(self.g.branches["legacy"].effect_at_level(
            self.branch_levels["legacy"]), 0.0), 1.0)

    def starting_upgrade_level(self) -> int:
        return max(0, int(self.g.branches["legacy"].extra_at_level(
            "levels", self.branch_levels["legacy"])))

    def starting_biome_index(self) -> int:
        """Mirrors GameState.get_starting_biome_index() — clamped to what the
        boss ledger has actually unlocked, so Territory is a shortcut through
        earned ground rather than a key to unearned ground.

        THE CLAMP IS THE EVOLUTION STAGE GATE ALONE, and that is load-bearing.
        This used to go through `unlocked_tiers()`, which additionally requires
        `lifetime_catnip_this_cycle >= catnip_to_unlock` — and a Reawaken zeroes
        that counter, so Territory evaporated at the exact moment it exists to
        pay. The simulated player re-climbed from biome 1 after every reset
        while the real one is TRAVELLED to the Territory biome outright
        (`tier_controller.gd:1915`, which calls `_travel_to` and never consults
        the Catnip gate). That one line was most of a 24-minute "gap with
        nothing new": the whole re-climb was content the player had already had.

        The boss ledger survives a Reawaken — `reawaken()` clears
        `lifetime_catnip_this_cycle` and `deepest_biome_index`, never
        `bosses_defeated` — so the stage gate is the honest bound.
        """
        wanted = max(1, int(self.g.branches["legacy"].extra_at_level(
            "biome", self.branch_levels["legacy"])))
        stage = self.evolution_stage()
        deepest = max((t.index for t in self.g.tiers
                       if t.required_evolution_stage <= stage), default=1)
        return min(wanted, deepest)

    _stardust_granted_ever: float = 0.0


# Module-level switches so the mirrored formulas stay readable.
FIX_F1 = False
FIX_F2 = False
FIX_F3 = False

# --- COVERAGE (open defect 3, 2026-08-25) ----------------------------------
#
# This model assumes PERFECT TARGETING: every mouse that spawns is killed. The
# field simulator in pawsim2 measures 43% at biome 2 minute 60, so ambient income
# here is roughly twice what the arena delivers - and, worse for tuning, the two
# lines that BUY coverage (`aura_radius`, `kitten_speed`) improve nothing in this
# model at all, so the greedy buyer scores them at zero and never buys them. That
# is why `aura_reach` is bought in no run under any ruleset.
#
# Measured with `pawsim2 runtime --sweep-line <id> --seeds 7`, both levers move
# coverage by ~9 percentage points per unit of their multiplier:
#     aura_radius   1.0x -> 3.3x   43.4% -> 65.0%
#     kitten_speed  1.0x -> 3.3x   42.8% -> 63.7%
#
# DEFAULT OFF, deliberately, and the same way FIX_F1..F3 are: switching it on
# halves modelled income, which moves every number the economy was tuned against
# - boss HP, bait prices, the wave gates. It is here so the Ambush trunk can be
# priced against measured coverage instead of guessed, per the implementation
# plan's step 8; turning it on for the shipped economy is a re-tune, not a flag.
# ON by default since 2026-08-25. Perfect targeting is not a simplification,
# it is a known-false assumption worth ~2x of ambient income, and it made the
# three coverage lines unbuyable. Validated at eight operating points, mean
# error 3.4 points. `--perfect-targeting` restores the old behaviour for
# comparing against numbers measured before this date.
COVERAGE_MODEL = True
# REFIT 2026-08-26 against 72 field-sim measurements (was eight), collected at
# eleven operating points from minute 5 to 240 and spread across headroom by
# forcing `spawn_capacity_headroom` (a dial the spawner no longer has, since
# 2026-09-14; headroom now varies with the authored rate instead). Binned,
# chase play style:
#
#   headroom   <0.9   0.9-1.0  1.0-1.1  1.1-1.2  1.2-1.3  1.3-1.5
#   measured   64.2     66.2     55.1     69.8     77.5     86.6   (%)
#   headroom   1.5-2   2-3      3-5      5-10     10+
#   measured   97.5     94.4     98.3     97.5     97.8     (%)
#
#   mean |error|   shipped 12.7 pts  ->  refit 8.5 pts
#   bias early/late  -4.5/-20.6      ->  +6.0/-13.0
#
# READ THIS BEFORE TRUSTING A COVERAGE NUMBER: THE INPUT IS WRONG, AND THE REFIT
# DOES NOT FIX THAT. Coverage correlates with how CROWDED the arena is
# (`mean_alive`, r = -0.71) far better than with headroom (r = +0.34). At
# headroom ~= 1.0 - which is exactly where `spawn_capacity_headroom = 1.0` pins
# the whole early game - measured coverage runs from 35% to 99.6%. **No function
# of headroom alone can be closer than +/-32 points there.** Within one biome it
# tracks live count cleanly (biome 1: 16.1 alive -> 35%, 5.3 -> 67%, 3.3 -> 79%)
# and biome 3 shows the same shape shifted up as the player's reach grows, which
# is what coverage physically IS: crowding relative to reach. Headroom is a
# proxy that correlates late and fails early.
#
# The honest summary: this halves the error and removes a third of the late
# bias, and it still cannot tell a 35% opening from a 99% one. Worth having.
# Not a foundation to tune on. Replacing the input is real modelling work and
# is written up in the handoff rather than done here.
#
# Regenerate: tools/../scratch collect_coverage.py + fit_coverage.py, or see the
# handoff's 2026-08-26 entry. `check_sims` asserts the fit against the binned
# table above, so editing these four numbers without re-measuring turns it red.
COVERAGE_MIN = 0.45
COVERAGE_MAX = 0.97
# Logistic, not exponential-approach: the measured curve is FLAT below
# headroom ~1.15 and then rises very steeply through ~1.25. An
# exponential-approach fit missed by up to 18 points at the knee, which is
# exactly where a run spends its time.
COVERAGE_KNEE = 1.20      # headroom at the midpoint
COVERAGE_SLOPE = 8.0
# What the coverage LINES buy on top, per unit of multiplier. Small next to
# headroom, which is the honest finding, but non-zero is what makes the greedy
# buyer value aura_radius, aura_reach and kitten_speed at all instead of scoring
# them zero and never buying them.
COVERAGE_PER_MULT = 0.02
# Ambush, measured with the trunk owned (see coverage()).
AMBUSH_PER_SLOW = 0.28
AMBUSH_PER_BOUNCE = 0.215
# Per chain link. Smaller than a Funnel bounce: a bounce recovers a mouse that
# was leaving, a chain only reaches one that was already going to be reachable.
PRISM_PER_CHAIN = 0.06
# Full Spectrum stops choosing targets entirely. Worth more than several
# chain links, but it gives up focus, which the aggregate model cannot see.
PRISM_SWEEP_REACH = 0.25
# TierController.PRISM_SHARDS_PER_DETONATION.
PRISM_SHARDS_PER_DETONATION = 10.0

# Reproduce the pre-boss world (Mystique gates tiers, reawaken short-circuits
# to Stage 8) for before/after comparison runs.
LEGACY_MYSTIQUE_GATE = False

# --tune: print the boss HP that would produce the intended fight length at
# whatever DPS the player actually arrives with. Escalating, so each boss is a
# bigger event than the last, ending on the 8-minute finale the Numbers Doc
# always intended for the Void God (review F5).
TUNE = False
# Intended fight length per boss, in seconds. Only tiers 3, 6 and 8 have a boss
# since v4 Phase 2; the other entries are vestigial and kept only so a re-added
# boss has a sane default rather than falling through to 60.0.
#
# The three live targets were RAISED on 2026-08-18. They were written for an
# eight-boss game where a fight was one gate of seven; with three, each fight is
# a third of the whole progression and arrives after a deliberate Bait purchase,
# so a 45-second boss 1 would be an anticlimax for something the player spent
# ten minutes of income to summon.
TUNE_FIGHT_SEC = {1: 20.0, 2: 30.0, 3: 90.0, 4: 60.0, 5: 90.0, 6: 180.0, 7: 180.0, 8: 480.0}

# Per-tier HP overrides used by --autotune, so a sweep never has to rewrite
# /data mid-search. Empty means "use the shipped .tres values".
BOSS_HP_OVERRIDE: dict = {}

# Intended MINUTES per tier, arrival to arrival (Numbers Doc §2's 12->82 min
# budget shape: each biome should last longer than the last, totalling ~6.5h).
# A flat HP multiplier cannot produce this — it front-loads the run, because
# early boss HP is sized against tiny DPS while late DPS grows far faster than
# any uniform scale. Hence per-tier autotuning.
TARGET_TIER_MINUTES = {1: 15.0, 2: 22.0, 3: 30.0, 4: 40.0, 5: 52.0, 6: 65.0, 7: 78.0, 8: 88.0}


# --------------------------------------------------------------------------
# Boss
# --------------------------------------------------------------------------


def boss_uptime_factor(boss: dict) -> float:
    """Effective-HP multiplier from the invulnerability windows.

    Boss.gd alternates vulnerable/shield per phase; a phase's vulnerable
    uptime is vuln/(vuln+shield). Each phase covers a slice of total HP
    delimited by phase_hp_thresholds. Effective HP is the HP-weighted sum of
    each slice divided by its uptime.

    On shipped data this returns ~2.22, matching the review's "x2.2 -> 77.6M
    effective HP" (F5).
    """
    thresholds = list(boss["phase_hp_thresholds"])  # e.g. [0.67, 0.33]
    vuln = list(boss["phase_vulnerable_sec"])
    shield = list(boss["phase_shield_sec"])

    bounds = [1.0] + thresholds + [0.0]
    factor = 0.0
    for i in range(len(vuln)):
        slice_fraction = bounds[i] - bounds[i + 1]
        uptime = vuln[i] / (vuln[i] + shield[i])
        factor += slice_fraction / uptime
    return factor


def boss_total_hp(tier: Tier, scale: float = 1.0) -> float:
    """Authored HP, unless --autotune is currently overriding this tier."""
    return BOSS_HP_OVERRIDE.get(tier.index, tier.boss["total_hp"]) * scale


def boss_add_hp_factor(boss: dict, ttk_seconds: float) -> float:
    """Effective-HP multiplier from adds that reach the boss and heal it.

    An add lands `add_heal_fraction` of max HP, one arrives every
    `add_interval_sec`, and ADD_LEAK_FRACTION of them get through. Over a fight
    of `ttk_seconds` that is a straightforward addition to the pool the player
    has to chew through.

    Clamped: if leaked healing ever met or exceeded incoming damage the fight
    would be unwinnable and time-to-kill would go infinite, which would send
    --autotune chasing a boss HP that does not exist. The clamp turns that into
    a merely very long fight, which shows up as a bad number instead of a hang.
    """
    interval = boss.get("add_interval_sec", 0.0)
    heal = boss.get("add_heal_fraction", 0.0)
    if interval <= 0.0 or heal <= 0.0 or ttk_seconds <= 0.0:
        return 1.0
    leaked = (ttk_seconds / interval) * ADD_LEAK_FRACTION
    return min(1.0 + leaked * heal, 4.0)


def boss_effective_hp(tier: Tier, scale: float = 1.0) -> float:
    return boss_total_hp(tier, scale) * boss_uptime_factor(tier.boss)


# --------------------------------------------------------------------------
# Simulation
# --------------------------------------------------------------------------


@dataclass
class Result:
    policy: str
    cleared: bool
    total_seconds: float
    boss_seconds: float
    end_multiplier: float
    stardust: float
    reawaken_count: int
    tier_arrival: dict
    reawaken_log: list
    boss_log: list = field(default_factory=list)
    kitten_share_start: float = 0.0
    kitten_share_end: float = 0.0
    # Pounce's share of total DPS at the end of the game, and the biggest share
    # it ever reaches. v4 Phase 4 exists partly because this hit 86% — one term
    # that large distorts every other measurement in the sim, so it needs to be
    # a reported number rather than something re-derived by hand each time.
    pounce_share_end: float = 0.0
    pounce_share_peak: float = 0.0
    # Kills/sec the player's DAMAGE supports, over kills/sec the SPAWNER allows,
    # at the end. The "surplus" the playtest measured at 9.4x by minute 30.
    surplus_end: float = 0.0
    catnip_granted: float = 0.0
    catnip_unspent: float = 0.0
    branch_levels: dict = field(default_factory=dict)
    # Reawaken count at which each unlock was first bought (2026-10-01).
    unlocked_at: dict = field(default_factory=dict)
    ## v4: one entry per life, Reawaken to Reawaken. Empty under v3 assumptions
    ## only if the player never reset.
    runs: list = field(default_factory=list)
    # --- the fun audit (2026-08-19) ---------------------------------------
    # Everything else this simulator reports is about whether the numbers WORK.
    # These three are about whether there is anything to DO, which is a
    # different question and the one a demo gets judged on.
    #
    # dead_air:     longest stretch, in seconds, where the player could not
    #               afford ANY purchase — no upgrade, no branch. In an
    #               incremental this is the failure state: not losing, but
    #               watching a number rise with no decision attached to it.
    # drought:      longest stretch with no NEW thing — no biome, no boss, no
    #               evolution stage, no first level of a branch, no card level.
    #               Affording rung 7 of a ladder you have bought six of is not
    #               the same experience as unlocking something.
    # buys_per_min: purchases per minute in the first, middle and last third of
    #               the game. Incrementals are supposed to decelerate; the
    #               question is whether they decelerate to a stop.
    dead_air: float = 0.0
    dead_window: tuple = (0.0, 0.0)
    drought: float = 0.0
    drought_window: tuple = (0.0, 0.0)
    buys_per_min: tuple = (0.0, 0.0, 0.0)
    # (t, uid, cost, gain_fraction) for every shop purchase, in order.
    shop_buys: list = field(default_factory=list)


@dataclass
class RunRecord:
    """One life: Reawaken to Reawaken, or game start / game end at the edges.

    v4 makes the RUN the unit of design — 20-40 minutes, 10-15 card picks —
    where v3 only ever measured the whole 6.5-hour game. Everything needed to
    check those two targets lives here.
    """
    index: int
    start_seconds: float
    end_seconds: float
    deepest_tier: int
    kills: float
    ## Catches points earned this run. NOT the same as `kills`: a signature pays
    ## two points to a common's one (TierController._on_enemy_killed), because a
    ## signature takes longer to bring down and paying it the same would make
    ## hunting one a waste of the run's clock. Expected points per kill is
    ## therefore 1 + signature_fraction.
    catches: float
    card_picks: int
    stardust_gained: float

    @property
    def minutes(self) -> float:
        return (self.end_seconds - self.start_seconds) / 60.0


def simulate(
    g: GameData,
    policy: str,
    patient_max_reawakens: int = 2,
    verbose: bool = False,
    boss_engage_ttk_sec: float = 60.0,
    patient_stall_window: float = 600.0,
    patient_stall_growth: float = 0.01,
    tier_stall_window: float = 60.0,
    tier_stall_growth: float = 0.02,
    engage_tolerance: float = 3.0,
    boss_uptime_scale: float = 1.0,
) -> Result:
    s = State(g)
    t = 0.0
    tier_arrival: dict = {}
    reawaken_log: list = []
    boss_log: list = []

    # Every tier has a boss now, and killing it is the only way forward.
    # Each is farmed until income stalls, then engaged.
    boss_remaining = 0.0
    boss_engaged = False
    boss_seconds = 0.0
    boss_started_at = 0.0
    engaged_tier = None

    # Stall detection for the patient policy: income growth flattening out is
    # the honest signal that a run has nothing left to buy and it is time to
    # reset. Avoids hardcoding a bank target (which would make the patient
    # result circular).
    stall_window = patient_stall_window
    last_income = 0.0
    last_check = 0.0

    # Independent stall tracker for "have I outgrown this tier's farming?",
    # which is what triggers a boss attempt.
    tier_last_utility = 0.0
    tier_last_check = 0.0
    # Set when the tier is farmed out but its boss is still unbeatable —
    # the signal that only a Reawaken can break the wall.
    boss_blocked = False

    # --- v4: the run is the unit of design ---
    # v3 measured only the whole 6.5-hour game. v4's targets are per-RUN (20-40
    # minutes, 10-15 card picks), so a life has to be tracked as a first-class
    # span rather than reconstructed afterwards from the reawaken log.
    runs: list = []
    run_start = 0.0
    run_kills = 0.0
    run_catches = 0.0
    run_deepest = 1
    run_sd_before = 0.0

    def _close_run(end_t: float) -> None:
        runs.append(RunRecord(
            index=len(runs) + 1,
            start_seconds=run_start,
            end_seconds=end_t,
            deepest_tier=run_deepest,
            kills=run_kills,
            catches=run_catches,
            # Derived rather than counted: nothing spends picks yet, Phase 1
            # owns that. Mirrors CardManager.add_kill_catches().
            card_picks=catches_levels_for_points(run_catches),
            stardust_gained=s._stardust_granted_ever - run_sd_before,
        ))

    def _result(cleared: bool) -> Result:
        """Single construction site for both exits.

        These were two hand-duplicated Result(...) literals; a field added to
        one and forgotten in the other would have drifted silently, and the
        `runs` field added in v4 Phase 0 is exactly the kind that would.
        """
        _close_run(t)
        return Result(
            policy=policy,
            cleared=cleared,
            total_seconds=t,
            boss_seconds=boss_seconds,
            end_multiplier=s.reawaken_multiplier(),
            stardust=s.stardust,
            reawaken_count=s.reawaken_count,
            tier_arrival=tier_arrival,
            reawaken_log=reawaken_log,
            boss_log=boss_log,
            kitten_share_start=kitten_share_start,
            kitten_share_end=(s.kitten_count() * s.kitten_dps_each()) / max(s.total_dps(), 1e-9),
            pounce_share_end=s.pounce_dps_sustained() / max(s.total_dps(), 1e-9),
            pounce_share_peak=pounce_share_peak,
            surplus_end=_surplus(s, s.best_tier()),
            catnip_granted=s._stardust_granted_ever,
            catnip_unspent=s.stardust,
            branch_levels=dict(s.branch_levels),
            unlocked_at=dict(getattr(s, "unlocked_at", {})),
            runs=runs,
            dead_air=max(dead_air, t - _dead_since),
            dead_window=dead_window,
            drought_window=drought_window,
            drought=max(drought, t - _new_at),
            buys_per_min=_cadence(_buy_times, t),
            shop_buys=list(_buy_stamped),
        )

    def _cadence(times, ended_at) -> tuple:
        """Purchases per minute in the first, middle and last third of the game.

        Thirds of the CLOCK, not of the purchase list — the question is whether
        the late game still has decisions in it, and splitting by purchase count
        would make every game look evenly paced by construction.
        """
        if ended_at <= 0.0:
            return (0.0, 0.0, 0.0)
        third = ended_at / 3.0
        buckets = [0, 0, 0]
        for ts in times:
            buckets[min(int(ts / third), 2)] += 1
        return tuple(round(b / (third / 60.0), 1) for b in buckets)

    def _surplus(state, tier) -> float:
        """How many times more mice the player COULD kill than the spawner
        offers. 1.0 means damage and supply are in balance; 10 means nine of
        every ten points of damage buys nothing."""
        supported = state.total_dps() / max(tier.avg_hp, 1e-9)
        allowed = state.spawn_rate_per_sec(tier)
        return supported / max(allowed, 1e-9)

    kitten_share_start = 0.0
    pounce_share_peak = 0.0

    # --- fun audit bookkeeping ---------------------------------------------
    dead_air = 0.0            # longest run of seconds with nothing purchasable
    _dead_since = 0.0
    drought = 0.0             # longest run of seconds with nothing NEW
    _new_at = 0.0
    _seen_new = set()         # ("biome", 3), ("boss", 6), ("branch", "pounce")...
    _buy_times = []           # timestamp of every purchase, for the cadence split
    _buy_stamped = []         # (t, uid, cost, gain_fraction) per SHOP buy
    dead_window = (0.0, 0.0)  # WHEN the worst stretch was, which is the actionable part
    drought_window = (0.0, 0.0)
    _engaged_bosses = set()
    while t < MAX_SIM_SECONDS:
        # Sampled once, ten minutes in: "kittens feel weak" is unfalsifiable
        # without a number for how much of the damage they actually are.
        if kitten_share_start == 0.0 and t >= 600.0:
            kitten_share_start = (s.kitten_count() * s.kitten_dps_each()) / max(s.total_dps(), 1e-9)
        pounce_share_peak = max(
            pounce_share_peak, s.pounce_dps_sustained() / max(s.total_dps(), 1e-9))
        # THE BAIT GATE MOVES THE PLAYER, not just the boss (2026-08-28).
        # `best_tier()` answers "where is the income", which was the whole story
        # while a boss opened on a wave counter. Bait drops only in the boss's
        # own biome, so a player who wants the fight has to go and stand there
        # and grind it out — 9,600 kills for the first one. Farming elsewhere
        # buys nothing toward it.
        #
        # Committed means the fight is WINNABLE: an unreachable boss is not
        # somewhere a sensible player parks, they keep farming and come back.
        # That is the same `engage_tolerance` test the engagement block uses
        # below, hoisted so the decision about WHERE to stand can read it.
        _pending = s.next_boss_tier()
        tier = s.best_tier()
        if _pending is not None and not boss_engaged:
            _eff = boss_effective_hp(_pending, boss_uptime_scale)
            _d = s.total_dps()
            _ttk = (_eff / _d) if _d > 0 else math.inf
            if math.isfinite(_ttk):
                _ttk = (_eff * boss_add_hp_factor(_pending.boss, _ttk)) / _d
            if _ttk <= TUNE_FIGHT_SEC.get(_pending.index, 60.0) * engage_tolerance:
                _farm = s.bait_farm_tier(_pending)
                if _farm is not None and _farm in s.unlocked_tiers():
                    tier = _farm
        if tier.index not in tier_arrival:
            tier_arrival[tier.index] = t
            if verbose:
                print(
                    f"  [{t/60:7.1f} min] reached {tier.id} "
                    f"(stage {s.evolution_stage()}, mult {s.reawaken_multiplier():,.1f}x)"
                )
        # High-water mark, so per-floor upgrade ceilings survive a trip back to
        # an easier biome.
        s._deepest_biome = max(getattr(s, "_deepest_biome", 1), tier.index)

        # How long the next boss would take at the damage the player has right
        # now. Hoisted out of the engagement block below because the answer is
        # needed BEFORE spending: a player who can win the fight starts saving
        # for its bait instead of buying another upgrade, and that trade is the
        # entire mechanic v4 Phase 2 added.
        def boss_ttk(bt) -> float:
            eff = boss_effective_hp(bt, boss_uptime_scale)
            d = s.total_dps()
            if d <= 0:
                return math.inf
            naive = eff / d
            if math.isfinite(naive):
                eff *= boss_add_hp_factor(bt.boss, naive)
            return eff / d

        pending_boss = s.next_boss_tier()
        bait_reserve = 0.0
        if pending_boss is not None and not boss_engaged:
            if boss_ttk(pending_boss) <= TUNE_FIGHT_SEC.get(pending_boss.index, 60.0) * engage_tolerance:
                want = s.bait_cost(pending_boss)
                # Only save for a bait that is actually within reach. A price
                # further away than BAIT_SAVE_HORIZON_SEC of income is not a
                # target, it is a wall — the player keeps upgrading and comes
                # back when it is cheap.
                #
                # Without this bound the buyer freezes ALL spending the moment a
                # boss is winnable, which stops income growing, which pushes the
                # price further out of reach. Measured: it turned the final
                # bait into a near-linear dial on total game length (50M ->
                # 5h46m, 100M -> 9h53m, 200M -> 18h, 600M -> never finished),
                # which is a property of the frozen buyer, not of the game.
                if want <= s.catnip_per_sec(tier) * BAIT_SAVE_HORIZON_SEC:
                    bait_reserve = want

        bought = s.buy_upgrades(tier, reserve=bait_reserve)
        bought += s.buy_branches()

        # Dead air: a tick where the player could buy nothing. Measured as a
        # RUN of such ticks, not a count of them, because the complaint is never
        # "I had four idle seconds" — it is "nothing happened for six minutes".
        if bought > 0:
            _buy_times.extend([t] * bought)
            while len(_buy_stamped) < len(s.buy_log):
                _buy_stamped.append((t,) + tuple(s.buy_log[len(_buy_stamped)]))
            if t - _dead_since > dead_air:
                dead_air = t - _dead_since
                dead_window = (_dead_since, t)
            _dead_since = t

        # Drought: anything the player has never had before. A ladder rung they
        # have bought six of does not count; a branch's FIRST level does.
        _now_new = set()
        _now_new.add(("biome", tier.index))
        _now_new.add(("stage", s.evolution_stage()))
        _now_new.add(("cards", catches_levels_for_points(run_catches)))
        for _bid, _lv in s.branch_levels.items():
            if _lv > 0:
                _now_new.add(("branch", _bid))
        for _bi in s.bosses_defeated:
            _now_new.add(("boss", _bi))
        for _bi in _engaged_bosses:
            _now_new.add(("fighting", _bi))
        if not _now_new <= _seen_new:
            _seen_new |= _now_new
            if t - _new_at > drought:
                drought = t - _new_at
                drought_window = (_new_at, t)
            _new_at = t

        # Travel to the next boss and fight it once farming has stopped paying
        # — the player has taken what the biomes have to give and the boss is
        # the only way forward. An exploiter additionally waits until the kill
        # is a formality (`boss_engage_ttk_sec`), since resets are cheap.
        #
        # The boss biome is looked up separately from the FARMING biome (v4
        # Phase 2): three unlock at once now, so the place with the best income
        # is usually not the place with the fight in it.
        boss_tier = pending_boss
        if boss_tier is not None and not boss_engaged:
            util = s.utility(tier)
            if t - tier_last_check >= tier_stall_window:
                growth = (util / tier_last_utility - 1.0) if tier_last_utility > 0 else 1.0
                tier_last_utility, tier_last_check = util, t
                stalled = growth < tier_stall_growth

                eff_hp = boss_effective_hp(boss_tier, boss_uptime_scale)
                dps = s.total_dps()
                # Add healing scales with how long the fight runs, so it is
                # measured against the naive TTK and then folded back into the
                # pool the player actually has to chew through.
                naive_ttk = eff_hp / dps if dps > 0 else math.inf
                if math.isfinite(naive_ttk):
                    eff_hp *= boss_add_hp_factor(boss_tier.boss, naive_ttk)
                ttk = eff_hp / dps if dps > 0 else math.inf
                # A player attempts the boss when the fight looks winnable in
                # a sane amount of time — not merely because farming got dull.
                # If the tier is exhausted AND the boss is still out of reach,
                # the only remaining source of power is a Reawaken. That is
                # what gives reawaken a job under boss gating: it stopped
                # being the tier gate, so it has to be the wall-breaker.
                ready = ttk <= TUNE_FIGHT_SEC.get(boss_tier.index, 60.0) * engage_tolerance
                # The bait is the gate v4 Phase 2 added: a winnable boss the
                # player cannot pay for is a boss they do not fight yet. Without
                # this the sim would report a pace the game cannot deliver.
                bait = s.bait_cost(boss_tier)
                # ALL-OR-NOTHING, and the order mirrors GameState.buy_bait():
                # both halves are checked before either is spent. The game's
                # first version spent the Bait and then checked the Catnip, so a
                # summon one Catnip short destroyed forty bait.
                if ready and s.catnip >= bait and s.is_bait_unlocked(boss_tier):
                    s.spend_bait(boss_tier)
                    s.catnip -= bait
                    boss_engaged = True
                    # Engaging a boss is an EVENT. Counting only the kill made
                    # the drought metric charge the player for the fight itself
                    # — the Void God takes 13 minutes, which read as 13 minutes
                    # of nothing happening.
                    _engaged_bosses.add(boss_tier.index)
                    boss_remaining = eff_hp
                    boss_started_at = t
                    engaged_tier = boss_tier.index
                    if verbose:
                            print(
                                f"  [{t/60:7.1f} min] engaging {boss_tier.boss['display_name']} "
                                f"({eff_hp:,.0f} eff HP, dps {dps:,.0f}, projected {ttk:,.0f}s, "
                                f"bait {bait:,.0f}, income {s.catnip_per_sec(tier):,.4g}/s = {bait / max(s.catnip_per_sec(tier), 1e-9) / 60.0:,.2f} min)"
                            )
                    if TUNE:
                        # HP that WOULD produce the intended fight length at
                        # the DPS the player actually shows up with.
                        want = TUNE_FIGHT_SEC.get(boss_tier.index, 60.0)
                        uf = boss_uptime_factor(boss_tier.boss)
                        print(
                            f"    TUNE tier {boss_tier.index}: dps={dps:,.0f} "
                            f"uptime={uf:.2f} target={want}s "
                            f"-> total_hp = {want * dps / uf:,.0f}"
                        )
                elif stalled:
                    boss_blocked = True
            elif tier_last_utility == 0.0:
                tier_last_utility, tier_last_check = util, t

        income = s.catnip_per_sec(tier, aura_on_boss=boss_engaged)
        # Catches rides the same kill rate the income does, on purpose.
        _killed = s.kill_rate_per_sec(tier, boss_engaged) * DT
        run_kills += _killed
        # Banked where they happened. This is what pays for THIS biome's boss
        # and nothing else — the property that disqualified a plain kill count.
        s.bank_kills(tier, _killed)
        # A signature pays 2 points to a common's 1, so expected points per kill
        # is 1 + signature_fraction. Mirrors CardManager.add_kill_catches().
        run_catches += _killed * (1.0 + tier.signature_fraction)
        run_deepest = max(run_deepest, tier.index)
        s.catnip += income * DT
        s.lifetime_catnip_this_cycle += income * DT
        s.lifetime_catnip_ever += income * DT

        if boss_engaged:
            # Player aura AND kittens are both on the boss — kittens follow
            # the player in (Kitten.hunt_boss via TierController), which is
            # also why ambient income is zero during the fight.
            boss_remaining -= s.total_dps() * DT
            boss_seconds += DT
            if boss_remaining <= 0.0:
                # Records the permanent ledger AND this cycle's passage.
                s.mark_boss_defeated(engaged_tier)
                s.credit_achievement_stardust()
                boss_log.append((boss_started_at, t - boss_started_at, engaged_tier))
                if verbose:
                    killed = g.tiers[engaged_tier - 1].boss["display_name"]
                    print(
                        f"  [{t/60:7.1f} min] KILLED {killed} "
                        f"in {t - boss_started_at:,.0f}s -> stage {s.evolution_stage()}"
                    )
                boss_engaged = False
                tier_last_utility, tier_last_check = 0.0, t
                if engaged_tier == len(g.tiers):
                    t += DT
                    return _result(True)

        # --- Reawaken policy ---
        should = False
        if s.can_reawaken() and not boss_engaged:
            if policy == "exploit":
                # F1: reset the instant the fixed threshold is met, forever.
                should = True
            elif policy == "v4":
                # Same behaviour as `exploit`, opposite meaning — and that
                # inversion is the point of the 2026-08-14 gate rewrite.
                #
                # Under the old Catnip threshold, resetting the moment you were
                # allowed to was the EXPLOIT: the gate opened while the payout
                # was still worthless (#2 paid 5 Stardust on a balance of 1,059).
                # The gate is priced in Stardust now and only opens at
                # +reawaken_min_gain_fraction, so "reset when allowed" is simply
                # correct play. v4 assumes the player does exactly that, because
                # v4's whole run structure depends on it.
                should = True
            elif policy == "patient":
                # Wall-breaker: this tier is farmed out and its boss is still
                # unreachable, so reset for the multiplier and the Catnip.
                if boss_blocked:
                    should = True
                elif s.reawaken_count < patient_max_reawakens and t - last_check >= stall_window:
                    growth = (income / last_income - 1.0) if last_income > 0 else 1.0
                    should = growth < patient_stall_growth
                    last_income, last_check = income, t
        if should:
            boss_blocked = False

        if should:
            sd, cn = s.reawaken()
            reawaken_log.append((t, sd, cn, s.stardust))
            # Close the life AFTER the grant lands, so the run is credited with
            # the Stardust it actually paid out.
            _close_run(t)
            run_start, run_kills, run_catches, run_deepest = t, 0.0, 0.0, 1
            run_sd_before = s._stardust_granted_ever
            if verbose and len(reawaken_log) <= 12:
                print(
                    f"  [{t/60:7.1f} min] Reawaken #{s.reawaken_count}: "
                    f"+{sd:,.0f} stardust, +{cn:,.0f} catnip (total sd {s.stardust:,.0f})"
                )

        if policy == "patient" and last_income == 0.0:
            last_income, last_check = income, t

        t += DT

    return _result(False)


# --------------------------------------------------------------------------
# Reporting
# --------------------------------------------------------------------------


# What the designer asked for on 2026-09-14: "an upgrade every few seconds
# or minutes that actually gives noticeable change is better" than "always
# something to upgrade but it doesn't mean anything". NOTICEABLE is a number
# here: a shop buy that lifts income by at least this fraction.
NOTICEABLE_GAIN = 0.08


def cadence_report(r: "Result") -> None:
    buys = r.shop_buys
    if not buys or r.total_seconds <= 0:
        return
    ended = buys[-1][0]
    third = max(ended, 1.0) / 3.0
    print("  --- shop cadence (a buy is NOTICEABLE at >= %.0f%% income) ---" % (NOTICEABLE_GAIN * 100))
    print("  total shop buys      : %d over %.0f min  (%.1f/min)" % (len(buys), ended / 60.0, len(buys) / (ended / 60.0)))
    for i, label in enumerate(("early", "mid", "late")):
        chunk = [b for b in buys if min(int(b[0] / third), 2) == i]
        if len(chunk) < 2:
            print("  %-5s  no buys" % label)
            continue
        # A MOMENT is a second in which the player bought anything: the sim
        # re-levels the shop in one tick after a Reawaken, and the player does
        # the same thing over a minute of clicking. Gaps are between moments,
        # so a burst counts once; gains are per buy, because each click is a
        # decision the player reads.
        moments = sorted(set(b[0] for b in chunk))
        gaps = sorted(moments[k] - moments[k - 1] for k in range(1, len(moments))) or [0.0]
        gains = sorted(b[3] for b in chunk)
        med_gap = gaps[len(gaps) // 2]
        p90_gap = gaps[int(0.9 * len(gaps))]
        med_gain = gains[len(gains) // 2]
        share = sum(1 for g in gains if g >= NOTICEABLE_GAIN) / len(gains)
        print("  %-5s  %4d buys in %4d moments   gap median %4.0fs  p90 %4.0fs   gain median %5.1f%%   noticeable %3.0f%%"
              % (label, len(chunk), len(moments), med_gap, p90_gap, med_gain * 100.0, share * 100.0))
    per_line = {}
    for _, uid, _, g in buys:
        per_line.setdefault(uid, []).append(g)
    worst = sorted(per_line.items(), key=lambda kv: sorted(kv[1])[len(kv[1]) // 2])[:4]
    print("  least noticeable lines: " + ", ".join(
        "%s (%d buys, median %.1f%%)" % (uid, len(gs), 100.0 * sorted(gs)[len(gs) // 2]) for uid, gs in worst))


def fmt_hms(seconds: float) -> str:
    h, rem = divmod(int(seconds), 3600)
    m, sec = divmod(rem, 60)
    return f"{h}h {m:02d}m" if h else f"{m}m {sec:02d}s"


def report(r: Result, g: GameData) -> None:
    print(f"\n=== policy: {r.policy} ===")
    if r.cleared:
        print(f"  final boss dead at   : {fmt_hms(r.total_seconds)}  ({r.total_seconds/60:,.1f} min)")
    else:
        print(f"  DID NOT CLEAR within {fmt_hms(r.total_seconds)}")
    print(f"  boss fight duration  : {fmt_hms(r.boss_seconds)}")
    print(f"  Reawakens            : {r.reawaken_count}")
    print(f"  total stardust       : {r.stardust:,.0f}")
    print(f"  end multiplier       : {r.end_multiplier:,.1f}x")
    # How much of the player's damage the kittens actually are. Without this,
    # "kittens feel weak" is unfalsifiable.
    print(f"  kitten share of DPS  : {r.kitten_share_start:.1%} at 10 min -> {r.kitten_share_end:.1%} at the end")
    print(f"  pounce share of DPS  : peak {r.pounce_share_peak:.1%}, {r.pounce_share_end:.1%} at the end")
    print(f"  damage/spawn surplus : {r.surplus_end:,.1f}x at the end")
    if r.catnip_granted:
        # Review fix 6 needs the tree priced against real supply, not vibes.
        spent = r.catnip_granted - r.catnip_unspent
        print(f"  stardust earned/spent: {r.catnip_granted:,.0f} / {spent:,.0f}")
        levels = " ".join(
            f"{bid}:{lv}/{b.max_level()}"
            for bid, b in g.branches.items()
            for lv in [r.branch_levels.get(bid, 0)]
        )
        print(f"  branch levels        : {levels}")
    print("  --- fun audit ---")
    # Runway: complete lives remaining after each Reawaken-gated branch opens.
    # A branch that unlocks on the final Reawaken is content the player never
    # gets to use, however good it is.
    # MEASURED since 2026-10-01: the Reawaken each unlock was actually bought
    # on, from the purchase log. It used to subtract fixed gates (1, 2, 3)
    # that the game stopped having on 2026-08-23, so it reported the Unicorn
    # as bought after Reawaken 3 whatever it cost.
    _bought = getattr(r, "unlocked_at", {})
    _runway = []
    for _b in ("unlock_pounce", "unlock_overkill", "unlock_unicorn", "unlock_poop"):
        if _b in _bought:
            _runway.append("%s:%d" % (_b, max(r.reawaken_count - _bought[_b], 0)))
        else:
            _runway.append("%s:never" % _b)
    print("  runs after unlock    : %s  (of %d Reawakens)"
          % ("  ".join(_runway), r.reawaken_count))
    print(f"  longest dead air     : {r.dead_air/60:6.1f} min  "
          f"(min {r.dead_window[0]/60:.0f}-{r.dead_window[1]/60:.0f}, nothing affordable)")
    print(f"  longest drought      : {r.drought/60:6.1f} min  "
          f"(min {r.drought_window[0]/60:.0f}-{r.drought_window[1]/60:.0f}, nothing NEW)")
    print("  buys/min early|mid|late: %s" % " | ".join("%.1f" % b for b in r.buys_per_min))
    cadence_report(r)
    print("  tier arrivals        :")
    prev = 0.0
    for idx in sorted(r.tier_arrival):
        at = r.tier_arrival[idx]
        print(f"    tier {idx}  at {at/60:8.1f} min   (+{(at-prev)/60:6.1f} min in previous)")
        prev = at
    if r.boss_log:
        print("  boss kills           :")
        for started, dur, idx in r.boss_log:
            name = g.tiers[idx - 1].boss["display_name"]
            print(f"    tier {idx}  {name:<22} engaged {started/60:7.1f} min, fight {dur:6.0f}s")
    report_runs(r)


## v4's two headline targets, the way "6h30m / 100x" were v3's.
##
## A run is a life: Reawaken to Reawaken. The final entry is the tail of the
## game rather than a completed life, so it is reported but excluded from the
## medians — it ends when the Void God dies, not when the player judged a reset
## worthwhile, and including it drags the median toward an outlier.
RUN_MINUTES_TARGET = (20.0, 40.0)
RUN_PICKS_TARGET = (10, 15)


def report_runs(r: Result) -> None:
    if len(r.runs) < 2:
        return
    body = r.runs[:-1]
    mins = [run.minutes for run in body]
    picks = [run.card_picks for run in body]
    med_min = statistics.median(mins)
    med_picks = statistics.median(picks)

    def verdict(value: float, lo: float, hi: float) -> str:
        if value < lo:
            return "BELOW target"
        return "ABOVE target" if value > hi else "on target"

    lo_m, hi_m = RUN_MINUTES_TARGET
    lo_p, hi_p = RUN_PICKS_TARGET
    print(f"  --- v4 run structure ({len(body)} complete lives, tail excluded) ---")
    print(f"  run length median    : {med_min:5.1f} min   "
          f"(target {lo_m:.0f}-{hi_m:.0f})   {verdict(med_min, lo_m, hi_m)}")
    print(f"  run length range     : {min(mins):5.1f} - {max(mins):.1f} min")
    print(f"  card picks median    : {med_picks:5.0f}       "
          f"(target {lo_p}-{hi_p})     {verdict(med_picks, lo_p, hi_p)}")
    print(f"  card picks range     : {min(picks):5d} - {max(picks)}")
    print("  per run              :")
    for run in r.runs:
        tail = "  <- tail, not a completed life" if run is r.runs[-1] else ""
        print(f"    run {run.index:>2}  {run.minutes:6.1f} min  "
              f"biome {run.deepest_tier}  {run.kills:>10,.0f} kills  "
              f"{run.card_picks:>3d} picks  +{run.stardust_gained:>10,.0f} sd{tail}")


def tier_durations(r: Result, tier_count: int) -> dict:
    """Minutes spent per tier, arrival to arrival (last tier runs to the clear)."""
    out = {}
    for i in range(1, tier_count + 1):
        if i not in r.tier_arrival:
            continue
        start = r.tier_arrival[i]
        end = r.tier_arrival.get(i + 1, r.total_seconds)
        out[i] = (end - start) / 60.0
    return out


def autotune(g: GameData, iterations: int, damping: float, **kw) -> dict:
    """Search per-tier boss HP that reproduces TARGET_TIER_MINUTES.

    Each tier's duration is dominated by how long the player must farm before
    its boss becomes beatable, so boss HP is the pacing dial. Scale each
    tier's HP by (target/actual) with damping and re-run until it settles.
    Damped because the tiers are coupled: making Tier 3 longer leaves the
    player richer at Tier 4, which shifts Tier 4's timing too.
    """
    global BOSS_HP_OVERRIDE
    BOSS_HP_OVERRIDE = {t.index: t.boss["total_hp"] for t in g.tiers if t.boss}

    best, best_err = dict(BOSS_HP_OVERRIDE), math.inf
    for it in range(iterations):
        r = simulate(g, "patient", **kw)
        durations = tier_durations(r, len(g.tiers))

        err = 0.0
        for idx, target in TARGET_TIER_MINUTES.items():
            actual = durations.get(idx)
            if actual is None or actual <= 0:
                err += 10.0
                continue
            err += abs(math.log(actual / target))
        if err < best_err:
            best, best_err = dict(BOSS_HP_OVERRIDE), err

        total = r.total_seconds / 60.0
        print(
            f"  iter {it + 1:2d}: total {total:6.1f} min, "
            f"cleared={r.cleared}, log-err {err:6.3f}"
        )

        for idx, target in TARGET_TIER_MINUTES.items():
            actual = durations.get(idx)
            if actual is None or actual <= 0:
                # Never reached: the boss is far too strong, back it off hard.
                BOSS_HP_OVERRIDE[idx] = BOSS_HP_OVERRIDE.get(idx, 1.0) * 0.5
                continue
            ratio = (target / actual) ** damping
            BOSS_HP_OVERRIDE[idx] = max(1.0, BOSS_HP_OVERRIDE[idx] * ratio)

    BOSS_HP_OVERRIDE = best
    return best


def main() -> int:
    ap = argparse.ArgumentParser(description="Pawpocalypse closed-loop economy simulator")
    ap.add_argument("--policy", choices=["exploit", "patient", "v4", "both"], default="both",
                    help="v4 = reset whenever the Stardust-priced gate opens, which is "
                         "correct play under the new gate rather than an exploit")
    ap.add_argument("--catches-base", type=float, default=None,
                    help="XP cost of Catches level 1 (v4 card picks; default: the game's, card_level_cost_base)")
    ap.add_argument("--catches-growth", type=float, default=None,
                    help="geometric growth of Catches level cost (default: the game's, card_level_cost_growth)")
    ap.add_argument("--perfect-targeting", action="store_true",
                    help="assume every spawn is killed, as this model did before "
                         "2026-08-25. Known false - the field sim measures 38-92%% "
                         "depending on kill-capacity headroom - and it makes "
                         "aura_radius, aura_reach and kitten_speed unbuyable. Here "
                         "to reproduce numbers measured before coverage landed.")
    ap.add_argument("--fix-f1", action="store_true", help="split-invariant reawaken grants (review fix 1)")
    ap.add_argument("--fix-f2", action="store_true", help="gate Stage 8 on Mystique too (review fix 2)")
    ap.add_argument("--fix-f3", action="store_true",
                    help="scaling Reawaken threshold + multiplier soft cap (review fixes 3 & 4)")
    ap.add_argument("--fixed", action="store_true", help="shorthand for --fix-f1 --fix-f2 --fix-f3")
    ap.add_argument("--patient-max-reawakens", type=int, default=2)
    ap.add_argument("--boss-engage-ttk", type=float, default=60.0,
                    help="exploiter engages the boss once projected kill time drops below this (sec)")
    ap.add_argument("--patient-stall-window", type=float, default=600.0)
    ap.add_argument("--patient-stall-growth", type=float, default=0.01)
    ap.add_argument("--tier-stall-window", type=float, default=60.0,
                    help="how long a tier's utility must flatten before attempting its boss")
    ap.add_argument("--tier-stall-growth", type=float, default=0.02)
    ap.add_argument("--legacy-gate", action="store_true",
                    help="reproduce pre-boss progression (Mystique gates tiers) for comparison")
    ap.add_argument("--tune", action="store_true",
                    help="print the boss total_hp that would hit the intended fight length")
    ap.add_argument("--engage-tolerance", type=float, default=3.0,
                    help="attempt the boss once projected kill time is within this multiple of target")
    ap.add_argument("--golden-catch", type=float, default=GOLDEN_CATCH_DEFAULT,
                    help="fraction of Golden Mice the player is assumed to catch")
    ap.add_argument("--add-leak", type=float, default=ADD_LEAK_DEFAULT,
                    help="fraction of void-kitten adds assumed to reach the boss")
    ap.add_argument("--boss-hp-scale", type=float, default=1.0,
                    help="scale every boss's HP, to sweep pacing without editing /data")
    ap.add_argument("--autotune", action="store_true",
                    help="search per-tier boss HP that reproduces TARGET_TIER_MINUTES")
    ap.add_argument("--autotune-iterations", type=int, default=25)
    ap.add_argument("--autotune-damping", type=float, default=0.5)
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--data", choices=["tres", "csv"], default="tres",
                    help="tres: the Godot project's /data (read-only, PAWSIM_GODOT_ROOT); csv: this repo's Data/*.csv")
    args = ap.parse_args()

    global FIX_F1, FIX_F2, FIX_F3, LEGACY_MYSTIQUE_GATE, TUNE, ADD_LEAK_FRACTION
    global COVERAGE_MODEL
    global GOLDEN_CATCH_RATE, CATCHES_BASE, CATCHES_GROWTH
    # The game's curve unless swept: from Data/CodeConstants.csv with --data csv, else the copies above.
    code = load_code_constants_csv() if args.data == "csv" else {}
    CATCHES_BASE = args.catches_base if args.catches_base is not None else float(
        code.get("card_level_cost_base", CATCHES_BASE_DEFAULT))
    CATCHES_GROWTH = args.catches_growth if args.catches_growth is not None else float(
        code.get("card_level_cost_growth", CATCHES_GROWTH_DEFAULT))
    TUNE = args.tune

    # The v4 policy resets whenever the gate opens, which is only sane behaviour
    # BECAUSE grants are split-invariant (F1). Without F1 the grant is
    # sqrt(this cycle) and reset-spam pays many times over, so `--policy v4`
    # without `--fixed` would model a game nobody is building and quietly report
    # a plausible-looking number for it. Force the fixes on and say so.
    v4_selected = args.policy == "v4"
    if v4_selected and not (args.fixed or (args.fix_f1 and args.fix_f2 and args.fix_f3)):
        print("note: --policy v4 implies --fixed (v4 assumes split-invariant grants)")
    COVERAGE_MODEL = not args.perfect_targeting
    FIX_F1 = args.fix_f1 or args.fixed or v4_selected
    FIX_F2 = args.fix_f2 or args.fixed or v4_selected
    FIX_F3 = args.fix_f3 or args.fixed or v4_selected
    LEGACY_MYSTIQUE_GATE = args.legacy_gate
    ADD_LEAK_FRACTION = args.add_leak
    GOLDEN_CATCH_RATE = args.golden_catch

    g = load_game_data_csv() if args.data == "csv" else load_game_data()
    print(f"pawsim — loaded {len(g.tiers)} tiers, {len(g.upgrades)} upgrades, {len(g.branches)} branches")
    print(f"fixes active: F1={'on' if FIX_F1 else 'off'}  F2={'on' if FIX_F2 else 'off'}  F3={'on' if FIX_F3 else 'off'}"
          f"  coverage={'MEASURED' if COVERAGE_MODEL else 'perfect-targeting'}")
    print(f"boss effective-HP factor: {boss_uptime_factor(g.tiers[-1].boss):.2f}x "
          f"({g.tiers[-1].boss['total_hp']*boss_uptime_factor(g.tiers[-1].boss):,.0f} effective HP)")

    sim_kw = dict(
        patient_max_reawakens=args.patient_max_reawakens,
        verbose=False,
        boss_engage_ttk_sec=args.boss_engage_ttk,
        patient_stall_window=args.patient_stall_window,
        patient_stall_growth=args.patient_stall_growth,
        tier_stall_window=args.tier_stall_window,
        tier_stall_growth=args.tier_stall_growth,
        engage_tolerance=args.engage_tolerance,
        boss_uptime_scale=args.boss_hp_scale,
    )

    if args.autotune:
        print("\n--- autotuning boss HP against TARGET_TIER_MINUTES ---")
        result = autotune(g, args.autotune_iterations, args.autotune_damping, **sim_kw)
        print("\n  tuned boss HP:")
        for idx in sorted(result):
            name = g.tiers[idx - 1].boss["display_name"]
            print(f"    tier {idx}  {name:<22} total_hp = {result[idx]:,.0f}")
        print()

    policies = ["exploit", "patient"] if args.policy == "both" else [args.policy]
    for p in policies:
        if args.verbose:
            print(f"\n--- {p} timeline ---")
        report(
            simulate(
                g, p, args.patient_max_reawakens, args.verbose,
                args.boss_engage_ttk, args.patient_stall_window, args.patient_stall_growth,
                args.tier_stall_window, args.tier_stall_growth,
                args.engage_tolerance, args.boss_hp_scale,
            ),
            g,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
