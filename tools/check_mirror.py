"""THE MIRROR GATE (docs/06 §8): PawCore against pawsim.

Godot's `ci/check_mirror.py` compared the game with pawsim on one fixture at every evolution stage, because a mirror
sampled at one stage would have passed throughout the 9.30x kitten drift (the bad term was `x ** (stage - 1)`, exactly
1.0 at stage 1). This does the same with PawCore in the game's seat, and more of it:

  * Godot's fixture (same levels, same branches) at every boss count, so this gate fails where that one would have;
  * thousands of seeded random states over the shop lines and branches pawsim models (damage, kittens, income, caps
    that grow per floor and per Reawaken, the Reawaken multiplier);
  * every upgrade's price at levels 0..80 and every branch's price at every level.

PawCore's half is `build/pawcore_tests/pawcore_sim.exe` (built by tools/build_pawcore_tests.ps1); it prints exact hex
floats. pawsim reads the same `Data/*.csv` (`load_game_data_csv`, proven byte-identical to its .tres loader by
tools/check_pawsim_parity.py). Both sides are float64 doing the same algebra in a different order, so the tolerance is
1e-9 relative, as in Godot.

`--self-test` proves the gate can fail: it points PawCore at sabotaged copies of the data (one number changed each)
and requires every one to be caught. A gate that has never failed has never been tested.

Usage:
  py tools/check_mirror.py                 # the gate (2,000 random states)
  py tools/check_mirror.py --random 200000 # the long run
  py tools/check_mirror.py --self-test     # the gate must catch each sabotage
Exit 0 = agree; 1 = disagree (or a sabotage went uncaught); 2 = could not run.
"""

import argparse
import csv
import io
import math
import os
import random
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import pawsim  # noqa: E402

SIM_EXE = ROOT / "build" / "pawcore_tests" / "pawcore_sim.exe"
WORK = ROOT / "build" / "mirror"
REL_TOL = 1e-9

# Must match FIXTURE / FIXTURE_BRANCHES in the Godot repo's ci/check_mirror.gd. Duplicated, not shared: the two sides
# are meant to be independent, and a shared fixture file is one more thing that can agree with itself.
GODOT_FIXTURE = {"paw_damage": 40, "kitten_power": 40, "attack_speed": 20, "kitten_count": 10, "currency_find": 25}
GODOT_FIXTURE_BRANCHES = {"unlock_aura": 1, "aura_damage": 3, "instinct": 2}

# What the random states vary: the lines and branches pawsim's damage, kitten and income model reads. Anything else
# (jobs, cards, the tree's crit and speed branches) pawsim does not model, so a random level there would compare two
# different games.
RANDOM_UPGRADES = ["paw_damage", "kitten_power", "attack_speed", "kitten_count", "currency_find", "shred", "crit_chance"]
RANDOM_BRANCHES = ["unlock_aura", "aura_damage", "instinct", "claws", "vitality", "unlock_unicorn", "unicorn", "unlock_poop",
                   "fortune", "legacy"]

# The sabotages for --self-test: (file, row id, column, new value). Each changes one number PawCore reads; pawsim keeps
# the real data, so the gate must see a difference.
SABOTAGES = [
    ("UpgradeData", "paw_damage", "effect_per_level", "x1.0001"),
    ("UpgradeData", "kitten_count", "effect_per_level", "+1"),
    ("UpgradeData", "currency_find", "cost_growth", "x1.0000001"),
    ("SkillBranchData", "aura_damage", "level_effects", "first+0.01"),
    ("EvolutionData", "stage_4", "weapon_multiplier", "x1.001"),
    ("CombatConstantsData", None, "base_kitten_damage_per_sec", "x1.001"),
    ("ReawakenConstantsData", None, "reawaken_mult_exponent", "x1.001"),
    ("ReawakenConstantsData", None, "reawaken_stardust_coefficient", "x1.01"),
    ("ReawakenConstantsData", None, "reawaken_min_gain_fraction", "x1.01"),
    ("ReawakenConstantsData", None, "reawaken_catnip_exponent", "x1.01"),
    ("SkillBranchData", "legacy", "level_effects", "first+0.01"),
    ("TierData", "tier_4", "golden_chance", "x1.01"),
]


def fmt_levels(d):
    return ",".join("%s:%d" % (k, v) for k, v in sorted(d.items()))


def make_fixtures(g, n_random, seed):
    """[(name, upgrades, branches, bosses, reawakens, deepest, stardust, lifetime Catnip ever)]"""
    boss_count = sum(1 for t in g.tiers if t.boss)
    out = []
    # Godot's fixture owns 10 Kitten Count levels: 10 kittens at one a level, but 20 since D17 E4 (two a buy), past the cap
    # of a run in biome 1. Kept inside the cap like the random states below (the KNOWN DIVERGENCE): 5 levels, 10 kittens.
    kc = g.upgrades["kitten_count"]
    godot_up = dict(GODOT_FIXTURE)
    godot_up["kitten_count"] = min(godot_up["kitten_count"], int(kc.effective_cap(0, 0) / kc.effect_per_level))
    for cleared in range(boss_count + 1):
        out.append(("godot_%d" % cleared, dict(godot_up), dict(GODOT_FIXTURE_BRANCHES), cleared, 0, 0, 0.0, 0.0))
    rng = random.Random(seed)
    for i in range(n_random):
        up = {u: (rng.randint(0, 60) if rng.random() < 0.8 else 0) for u in RANDOM_UPGRADES}
        br = {}
        for b in RANDOM_BRANCHES:
            if rng.random() < 0.6:
                br[b] = rng.randint(1, g.branches[b].max_level())
        stardust = 10.0 ** rng.uniform(0.0, 6.0) if rng.random() < 0.85 else 0.0
        reawakens, deepest = rng.randint(0, 15), rng.randint(0, 11)
        # KNOWN DIVERGENCE, unreachable in play: the game's get_kitten_count() reads the UNCAPPED _effect_total and
        # pawsim's kitten_count() clamps to the cap. The shop never sells past the cap (is_upgrade_at_cap), so the two
        # agree on every state a player can reach; past it they differ (measured 2026-10-28: 944 of 2,000 random
        # states). Kept inside the cap here so the gate tests what is real; reported to the designer, not "fixed".
        kc = g.upgrades["kitten_count"]
        up["kitten_count"] = min(up["kitten_count"], int(kc.effective_cap(deepest, reawakens) / kc.effect_per_level))
        cleared = rng.randint(0, boss_count)
        catnip_ever = float(math.floor(10.0 ** rng.uniform(0.0, 16.0))) if rng.random() < 0.9 else 0.0
        out.append(("rand_%d" % i, up, br, cleared, reawakens, deepest, stardust, catnip_ever))
    return out


def write_fixture_file(path, fixtures):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# written by tools/check_mirror.py\n")
        for name, up, br, cleared, reawakens, deepest, stardust, catnip_ever in fixtures:
            f.write("name=%s|upgrades=%s|branches=%s|bosses=%d|reawakens=%d|deepest=%d|stardust=%s|catnip_ever=%s\n"
                    % (name, fmt_levels(up), fmt_levels(br), cleared, reawakens, deepest, stardust.hex(), catnip_ever.hex()))


def run_pawcore(data_dir, fixture_path, out_path):
    if not SIM_EXE.exists():
        print("FAIL  %s missing: run tools/build_pawcore_tests.ps1 first" % SIM_EXE)
        sys.exit(2)
    r = subprocess.run([str(SIM_EXE), "--data", str(data_dir), "--fixtures", str(fixture_path), "--out", str(out_path)],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return None, r.stderr.strip()
    rows, prices = [], []
    with open(out_path, encoding="utf-8") as f:
        text = f.read()
    head, _, tail = text.partition("#prices\n")
    rows = list(csv.DictReader(io.StringIO(head), delimiter="\t"))
    prices = list(csv.DictReader(io.StringIO(tail), delimiter="\t"))
    return (rows, prices), ""


def pawsim_state(g, boss_indices, fx):
    name, up, br, cleared, reawakens, deepest, stardust, catnip_ever = fx
    s = pawsim.State(g=g)
    for k, v in up.items():
        s.upgrade_levels[k] = v
    for k, v in br.items():
        s.branch_levels[k] = v
    s.bosses_defeated = set(boss_indices[:cleared])
    s.reawaken_count = reawakens
    s._deepest_biome = deepest + 1  # pawsim keeps the 1-based biome; floors_reached() = that - 1
    s._stardust_granted_ever = stardust
    s.lifetime_catnip_ever = catnip_ever
    return s


class Gate:
    def __init__(self):
        self.checks = 0
        self.failures = []
        self.exact = 0
        self.worst = 0.0

    def check(self, name, ok, detail=""):
        self.checks += 1
        if not ok:
            self.failures.append(name + (("  --  " + detail) if detail else ""))

    def close(self, name, pawcore, sim):
        self.checks += 1
        if pawcore == sim:
            self.exact += 1
            return
        scale = max(abs(pawcore), abs(sim))
        rel = abs(pawcore - sim) / scale if scale > 0 and math.isfinite(scale) else math.inf
        self.worst = max(self.worst, rel)
        if not rel <= REL_TOL:
            ratio = pawcore / sim if sim else math.inf
            self.failures.append("%s  --  PawCore %r vs pawsim %r (PawCore/pawsim = %.6fx)" % (name, pawcore, sim, ratio))


def compare(g, fixtures, report):
    rows, prices = report
    gate = Gate()
    boss_indices = sorted(t.index for t in g.tiers if t.boss)
    gate.check("PawCore emitted one row per fixture", len(rows) == len(fixtures), "%d rows for %d fixtures" % (len(rows), len(fixtures)))
    duty = g.combat["poop_duration_sec"] / g.combat["poop_cooldown_sec"]
    poop_compared = 0
    for fx, row in zip(fixtures, rows):
        tag = fx[0]
        gate.check("%s: row order" % tag, row["name"] == tag, "PawCore row is %s" % row["name"])
        s = pawsim_state(g, boss_indices, fx)
        h = lambda k: float.fromhex(row[k])  # noqa: E731
        gate.check("%s: evolution stage" % tag, s.evolution_stage() == int(row["evolution_stage"]),
                   "pawsim %d vs PawCore %s" % (s.evolution_stage(), row["evolution_stage"]))
        gate.check("%s: kitten count" % tag, s.kitten_count() == int(row["kitten_count"]),
                   "pawsim %d vs PawCore %s" % (s.kitten_count(), row["kitten_count"]))
        gate.close("%s: paw DPS" % tag, h("paw_dps"), s.paw_dps())
        gate.close("%s: aura DPS" % tag, h("aura_dps"), s.aura_dps())
        gate.close("%s: player DPS" % tag, h("player_dps"), s.player_dps())
        gate.close("%s: kitten DPS each" % tag, h("kitten_dps_each"), s.kitten_dps_each())
        gate.close("%s: currency find" % tag, h("currency_find"), s.currency_find())
        gate.close("%s: Reawaken multiplier" % tag, h("reawaken_multiplier"), s.reawaken_multiplier())
        # D17 E3: what the Reawaken multiplies Catnip from kills by (pawsim's income reads it).
        gate.close("%s: Reawaken Catnip multiplier" % tag, h("reawaken_catnip_multiplier"), s.reawaken_catnip_multiplier())
        # The game's poop_dps() is instantaneous and ungated (TierController gates and times it); pawsim's is sustained
        # over the duty cycle and zero until the ability is owned.
        if s.poop_ability_unlocked():
            poop_compared += 1
            gate.close("%s: poop DPS (sustained)" % tag, h("poop_dps") * duty, s.poop_dps_sustained())
        # The economy pawsim mirrors (PcEconomy): the Stardust grant and its gate, Legacy, Fortune, golden chances.
        gate.close("%s: pending Stardust" % tag, h("pending_stardust"), s.pending_stardust())
        gate.close("%s: Reawaken requirement" % tag, h("reawaken_requirement"), s.reawaken_requirement())
        gate.close("%s: Catnip carryover" % tag, h("catnip_carryover"), s.catnip_carryover())
        gate.check("%s: starting upgrade level" % tag, s.starting_upgrade_level() == int(row["starting_level"]),
                   "pawsim %d vs PawCore %s" % (s.starting_upgrade_level(), row["starting_level"]))
        gate.check("%s: starting biome" % tag, s.starting_biome_index() == int(row["starting_biome"]),
                   "pawsim %d vs PawCore %s" % (s.starting_biome_index(), row["starting_biome"]))
        gate.close("%s: Fortune yield" % tag, h("fortune_yield"), s.fortune_yield())
        golden = [float.fromhex(x) for x in row["golden"].split(";")]
        gate.check("%s: a golden chance per biome" % tag, len(golden) == len(g.tiers))
        for t, chance in zip(g.tiers, golden):
            gate.close("%s: golden chance in %s" % (tag, t.id), chance, s.golden_chance(t))

    # ANTI-VACUITY: the comparisons above must have had something to compare.
    godot_rows = [r for r in rows if r["name"].startswith("godot_")]
    gate.check("Godot's fixture buys the aura (else its aura comparison is 0 against 0)",
               bool(godot_rows) and all(float.fromhex(r["aura_dps"]) > 0.0 for r in godot_rows))
    gate.check("paw DPS moves with the evolution stage (else the stage sweep is inert)",
               len(godot_rows) > 1 and float.fromhex(godot_rows[-1]["paw_dps"]) > float.fromhex(godot_rows[0]["paw_dps"]))
    gate.check("kitten DPS does NOT move with the evolution stage (the 2026-08-28 drift)",
               len({r["kitten_dps_each"] for r in godot_rows}) == 1)
    if len(fixtures) > 100:
        gate.check("the random states reach the poop comparison", poop_compared > 0)

    # Prices.
    n_up = n_br = 0
    for p in prices:
        kind, pid, level, cost = p["kind"], p["id"], int(p["level"]), float.fromhex(p["cost"])
        if kind == "upgrade":
            n_up += 1
            gate.close("price of %s at level %d" % (pid, level), cost, g.upgrades[pid].cost_for_level(level))
        else:
            n_br += 1
            sim = g.branches[pid].cost_for_next_level(level)
            gate.close("price of branch %s at level %d" % (pid, level), cost, -1.0 if math.isinf(sim) else sim)
    gate.check("every upgrade priced", n_up == 81 * len(g.upgrades), "%d prices for %d upgrades" % (n_up, len(g.upgrades)))
    gate.check("every branch priced", n_br == sum(b.max_level() + 1 for b in g.branches.values()), "%d branch prices" % n_br)
    return gate


def run_gate(data_dir, n_random, seed, quiet=False):
    g = pawsim.load_game_data_csv()
    fixtures = make_fixtures(g, n_random, seed)
    WORK.mkdir(parents=True, exist_ok=True)
    fixture_path = WORK / "fixtures.txt"
    write_fixture_file(fixture_path, fixtures)
    report, err = run_pawcore(data_dir, fixture_path, WORK / "pawcore_report.tsv")
    if report is None:
        if not quiet:
            print("FAIL  pawcore_sim failed: %s" % err)
        return None
    return compare(g, fixtures, report)


def sabotage_copy(spec, dest):
    cls, row_id, column, change = spec
    if dest.exists():
        shutil.rmtree(dest)
    shutil.copytree(ROOT / "Data", dest)
    path = dest / ("%s.csv" % cls)
    with open(path, encoding="utf-8", newline="") as f:
        rows = list(csv.reader(f))
    header = rows[0]
    col = header.index(column)
    hit = 0
    for r in rows[1:]:
        if row_id is not None and r[header.index("id")] != row_id:
            continue
        old = r[col]
        if change.startswith("x"):
            r[col] = repr(float(old) * float(change[1:]))
        elif change.startswith("+"):
            r[col] = repr(float(old) + float(change[1:]))
        elif change.startswith("first+"):
            parts = old.split(";")  # list cells are `;`-separated (docs/06 §2)
            parts[0] = repr(float(parts[0]) + float(change[len("first+"):]))
            r[col] = ";".join(parts)
        hit += 1
    if hit != 1:
        raise SystemExit("sabotage %s matched %d rows, expected 1" % (spec,))
    with open(path, "w", encoding="utf-8", newline="") as f:
        csv.writer(f, lineterminator="\n").writerows(rows)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--random", type=int, default=2000, help="random states besides Godot's fixture")
    ap.add_argument("--seed", type=int, default=20261028)
    ap.add_argument("--self-test", action="store_true", help="the gate must catch every sabotage in SABOTAGES")
    args = ap.parse_args()

    if args.self_test:
        uncaught = []
        for spec in SABOTAGES:
            sabotage_copy(spec, WORK / "sabotage")
            gate = run_gate(WORK / "sabotage", 300, args.seed, quiet=True)
            caught = gate is None or bool(gate.failures)
            print("%s  %-60s %s" % ("CAUGHT  " if caught else "MISSED  ", " ".join(str(x) for x in spec),
                                    ("%d failures" % len(gate.failures)) if gate else "pawcore_sim refused the data"))
            if not caught:
                uncaught.append(spec)
        shutil.rmtree(WORK / "sabotage", ignore_errors=True)
        # And the real data must pass, or "caught" means nothing.
        gate = run_gate(ROOT / "Data", 300, args.seed)
        clean = gate is not None and not gate.failures
        print("%s  the real data" % ("PASSES  " if clean else "FAILS   "))
        if uncaught or not clean:
            print("FAIL  self-test: %d sabotage(s) uncaught, real data %s" % (len(uncaught), "passes" if clean else "FAILS"))
            return 1
        print("PASS  self-test: all %d sabotages caught, the real data passes" % len(SABOTAGES))
        return 0

    gate = run_gate(ROOT / "Data", args.random, args.seed)
    if gate is None:
        return 2
    print("MIRROR_ASSERTIONS %d" % gate.checks)
    if gate.failures:
        for f in gate.failures[:60]:
            print("FAIL  " + f)
        if len(gate.failures) > 60:
            print("FAIL  ... and %d more" % (len(gate.failures) - 60))
        print("FAIL  %d of %d assertions failed" % (len(gate.failures), gate.checks))
        return 1
    print("PASS  %d assertions (%d values bit-identical, worst relative difference %.3g) - PawCore and pawsim agree"
          % (gate.checks, gate.exact, gate.worst))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
