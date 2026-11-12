"""econ_review - the economy review's measurements (docs/ECONOMY-REVIEW-2026-11-12.md).

pawsim (`--policy v4 --data csv`: Data/*.csv, the player who Reawakens as soon as the gate opens) with three additions:
  * WAVES. The game escalates a biome against its own kill bank (PcEconomy CurrentWave / WaveHpMultiplier /
    WaveRewardMultiplier: HP x1.35 and reward x1.28 a wave); pawsim has no waves. Here a biome's HP and reward follow
    its kill bank exactly as the game derives them (`--no-waves` switches it off).
  * LOGS. Every shop buy (time, line, gain), every tree level (its gain at purchase, all else held), every Reawaken, and
    a sample of the field every 10 s; the metrics the review uses are computed from them.
  * LEVERS. A proposal is '+'-joined levers applied IN MEMORY (Data/ is never written): see apply() and the review.

Usage:
    py tools/econ_review.py                                   # today's economy
    py tools/econ_review.py --proposal recommended            # the review's recommendation (E1-E6)
    py tools/econ_review.py --proposal pass+inc0.5 --label x  # any mix of levers
    py tools/econ_review.py --waits build/econ/today.json     # where the long waits are, per run
Writes build/econ/<label>.json. One run takes ~2.5 min; runs are deterministic.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
import sys
from pathlib import Path

PROJ = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJ / "tools"))
import pawsim as P  # noqa: E402

OUT = PROJ / "build" / "econ"

STATE = None
CLOCK = [0]
WAVES = [True]
MAX_WAVE = 64
TICKS: list = []      # (t, run, tier, wave, income, dps, mult, catnip)
BUYS: list = []       # (t, run, tier, uid, cost, gain)
TREE: list = []       # dict per branch level bought
REA: list = []        # dict per Reawaken
TREESTEP: list = []   # (t, run, util0, util1, dps0, dps1, inc0, inc1) per tree spend
RULES: dict = {}      # proposal switches read by the hooks


# ---------------------------------------------------------------------------------------------------------------------
# Waves (the game has them: PcEconomy CurrentWave / WaveHpMultiplier / WaveRewardMultiplier; pawsim does not)
# ---------------------------------------------------------------------------------------------------------------------

class WaveTier(P.Tier):
    @property
    def avg_hp(self):
        return self._base_hp * (self.whg ** self.wave())

    @property
    def reward_multiplier(self):
        return self._base_rm * (self.wrg ** self.wave())

    @reward_multiplier.setter
    def reward_multiplier(self, v):
        self._base_rm = v

    def wave(self) -> int:
        if not WAVES[0] or self.kpw <= 0 or STATE is None:
            return 0
        k = STATE.tier_kills_by_biome.get(self.index, 0.0)
        if k <= 0:
            return 0
        g = self.wkg
        w = k / self.kpw if abs(g - 1.0) < 1e-12 else math.log(1.0 + k * (g - 1.0) / self.kpw) / math.log(g)
        return max(0, min(int(w + 1e-9), MAX_WAVE))


def wave_tiers(g: P.GameData) -> None:
    rows = {r["id"]: r for r in csv.DictReader(open(PROJ / "Data" / "TierData.csv", encoding="utf-8"))}
    new = []
    for t in g.tiers:
        r = rows[t.id]
        nt = WaveTier.__new__(WaveTier)
        rm = t.__dict__.get("reward_multiplier", 1.0)
        d = dict(t.__dict__)
        d.pop("reward_multiplier", None)
        nt.__dict__.update(d)
        nt._base_rm = rm
        nt._base_hp = P.Tier.avg_hp.fget(t)
        nt.kpw = float(r["kills_per_wave"] or 0)
        nt.wkg = float(r["wave_kill_growth"] or 1)
        nt.whg = float(r["wave_hp_growth"] or 1)
        nt.wrg = float(r["wave_reward_growth"] or 1)
        new.append(nt)
    g.tiers[:] = new


# ---------------------------------------------------------------------------------------------------------------------
# Hooks
# ---------------------------------------------------------------------------------------------------------------------

_orig_post = P.State.__post_init__
_orig_bank = P.State.bank_kills
_orig_buy_up = P.State.buy_upgrades
_orig_buy_br = P.State.buy_branches
_orig_rea = P.State.reawaken
_orig_cps = P.State.catnip_per_sec


def _post(self):
    global STATE
    _orig_post(self)
    STATE = self


def _bank(self, tier, n):
    t = CLOCK[0]
    if t % 10 == 0 and tier is not None:
        cap = self.total_dps() / max(tier.avg_hp, 1e-9)
        allowed = self.spawn_rate_per_sec(tier) * self.coverage(tier)
        TICKS.append((t, self.reawaken_count, tier.index, tier.wave() if isinstance(tier, WaveTier) else 0,
                      self.catnip_per_sec(tier), self.total_dps(), self.reawaken_multiplier(), self.catnip,
                      cap / max(allowed, 1e-9)))
    CLOCK[0] += 1
    _orig_bank(self, tier, n)


def _buy_up(self, tier, reserve=0.0):
    n0 = len(self.buy_log)
    r = _orig_buy_up(self, tier, reserve)
    for e in self.buy_log[n0:]:
        BUYS.append((CLOCK[0], self.reawaken_count, tier.index, e[0], e[1], e[2]))
    return r


def _parts(self):
    tier = self.best_tier()
    return self.utility(tier), self.total_dps(), self.catnip_per_sec(tier)


def _buy_br(self):
    before = dict(self.branch_levels)
    pre = _parts(self) if self.stardust > 0 else None
    r = _orig_buy_br(self)
    if r:
        u1, d1, i1 = _parts(self)
        if pre:
            TREESTEP.append((CLOCK[0], self.reawaken_count, pre[0], u1, pre[1], d1, pre[2], i1))
        for bid, lv in self.branch_levels.items():
            lv0 = before.get(bid, 0)
            if lv <= lv0:
                continue
            # What this node's new levels are worth right now: everything else held.
            self.branch_levels[bid] = lv0
            u0, d0, i0 = _parts(self)
            self.branch_levels[bid] = lv
            TREE.append(dict(t=CLOCK[0], run=self.reawaken_count, id=bid, from_lv=lv0, to_lv=lv,
                             max_lv=self.g.branches[bid].max_level(),
                             cost=sum(self.g.branches[bid].cost_for_next_level(k) for k in range(lv0, lv)),
                             util_gain=(u1 / u0 - 1.0) if u0 > 0 else 0.0,
                             dps_gain=(d1 / d0 - 1.0) if d0 > 0 else 0.0,
                             inc_gain=(i1 / i0 - 1.0) if i0 > 0 else 0.0))
    return r


def _rea(self):
    rec = dict(t=CLOCK[0], n=self.reawaken_count + 1, mult_before=self.reawaken_multiplier())
    sd, cn = _orig_rea(self)
    rec.update(sd=sd, mult_after=self.reawaken_multiplier(), sd_ever=self._stardust_granted_ever)
    REA.append(rec)
    return sd, cn


def _cps(self, tier, aura_on_boss=False):
    v = _orig_cps(self, tier, aura_on_boss)
    k = RULES.get("income_mult_exp", 0.0)  # unused since D17 (E3 is pawsim's rule); kept 0
    if k:
        v *= self.reawaken_multiplier() ** k
    return v


_orig_bait_ok = P.State.is_bait_unlocked
BAITREADY: dict = {}  # boss tier index -> (t, seconds the fight would take at that moment), the FIRST time Bait is ready


def _bait_ok(self, tier):
    ok = _orig_bait_ok(self, tier)
    if ok and tier is not None and tier.index not in BAITREADY and tier.index not in self.bosses_defeated:
        eff = P.boss_effective_hp(tier, 1.0)
        d = self.total_dps()
        ttk = eff / d if d > 0 else float("inf")
        if ttk != float("inf"):
            ttk = eff * P.boss_add_hp_factor(tier.boss, ttk) / d
        BAITREADY[tier.index] = (CLOCK[0], ttk)
    return ok


P.State.is_bait_unlocked = _bait_ok
P.State.__post_init__ = _post
P.State.bank_kills = _bank
P.State.buy_upgrades = _buy_up
P.State.buy_branches = _buy_br
P.State.reawaken = _rea
P.State.catnip_per_sec = _cps


# ---------------------------------------------------------------------------------------------------------------------
# Levers (in memory only)
# ---------------------------------------------------------------------------------------------------------------------


def _pass(P):
    """A boss beaten in ANY cycle stops blocking passage (today: passage re-locks every Reawaken, so each run re-farms
    40 Bait for every boss already beaten)."""

    def bosses_cleared_for_stage(self, required_stage):
        idx = [t.index for t in self.g.tiers if t.boss]
        done = self.bosses_defeated | self.bosses_cleared_this_cycle
        return all(idx[i] in done for i in range(min(required_stage - 1, len(idx))))

    def next_boss_tier(self):
        for tier in self.unlocked_tiers():
            if tier.boss is not None and tier.index not in self.bosses_defeated \
                    and tier.index not in self.bosses_cleared_this_cycle:
                return tier
        return None

    P.State.bosses_cleared_for_stage = bosses_cleared_for_stage
    P.State.next_boss_tier = next_boss_tier


def _outclass(P, seconds):
    """A beaten boss the cat would kill within `seconds` lets it pass without a rematch."""
    orig_cleared = P.State.bosses_cleared_for_stage
    orig_next = P.State.next_boss_tier

    def _waved_through(self, tier):
        if tier.index not in self.bosses_defeated:
            return False
        eff = P.boss_effective_hp(tier, 1.0)
        d = self.total_dps()
        return d > 0 and eff / d <= seconds

    def bosses_cleared_for_stage(self, required_stage):
        idx = [t for t in self.g.tiers if t.boss]
        for i in range(min(required_stage - 1, len(idx))):
            t = idx[i]
            if t.index in self.bosses_cleared_this_cycle:
                continue
            if _waved_through(self, t):
                continue
            return False
        return True

    def next_boss_tier(self):
        for tier in self.unlocked_tiers():
            if tier.boss is not None and tier.index not in self.bosses_cleared_this_cycle \
                    and not _waved_through(self, tier):
                return tier
        return None

    P.State.bosses_cleared_for_stage = bosses_cleared_for_stage
    P.State.next_boss_tier = next_boss_tier


def _keepbait(P):
    """Bait already earned for a boss not yet beaten survives a Reawaken (today the kill bank, and the bait with it,
    is cleared, so a Reawaken throws away the progress toward the next boss). Waves still reset."""
    orig_bank = P.State.bank_kills
    orig_rea = P.State.reawaken

    def bank_kills(self, tier, n):
        orig_bank(self, tier, n)
        if tier is not None and n > 0 and tier.boss:
            bb = self.__dict__.setdefault("_bait_bank", {})
            bb[tier.index] = bb.get(tier.index, 0.0) + n

    def bait_earned(self, tier):
        if tier is None or not tier.boss:
            return 0
        return int(self.__dict__.get("_bait_bank", {}).get(tier.index, 0.0) // self.kills_per_bait(tier))

    def reawaken(self):
        spent = dict(self.bait_spent_by_biome)
        out = orig_rea(self)
        self.bait_spent_by_biome.update(spent)
        return out

    P.State.bank_kills = bank_kills
    P.State.bait_earned = bait_earned
    P.State.reawaken = reawaken


# D17 (2026-11-12) put E1-E6 and the Unicorn step INTO the game: `today` is that economy. `before_d17` puts the old rules
# and numbers back in memory (the review's "today" column: 406 min). The levers pass, keepbait, kit and unl0.1 are today's
# rules now (no-ops or the same numbers); they stay so the review's runs can be repeated.
PRESETS = {
    "today": "",
    "before_d17": "relock+inc0+uni1+kitold+unl0.02+hpold",
}

# Boss HP before D17 E6 (Data/BossData.csv on 2026-11-12, before the re-pricing).
_HP_BEFORE_D17 = {"park_tyrant": 2.93798e+06, "rival_champion": 3.64195e+09, "void_god": 3.49125e+10,
                  "factory_foreman": 2.07098e+12, "litter_king": 8.0768e+12}


def _relock(P):
    """Godot's rule before D17 (E1, E2 undone): a Reawaken clears this cycle's passage, the Bait spent and the Bait bank."""
    orig = P.State.reawaken

    def reawaken(self):
        out = orig(self)
        self.bosses_cleared_this_cycle.clear()
        self.bait_spent_by_biome.clear()
        self.bait_kills_by_biome.clear()
        self.__dict__.pop("_bait_bank", None)
        return out

    P.State.reawaken = reawaken


def apply(name, g, P, RULES):
    name = "+".join(PRESETS.get(x, x) for x in name.split("+"))
    if not name.strip("+"):
        return
    for lever in name.split("+"):
        if lever == "keepbait":
            _keepbait(P)
        elif lever == "pass":
            _pass(P)
        elif lever.startswith("outclass"):
            _outclass(P, float(lever[len("outclass"):] or 10))
        elif lever == "relock":
            _relock(P)
        elif lever.startswith("inc"):
            # E3: Catnip from kills times the Reawaken multiplier to this power (ReawakenConstantsData.reawaken_catnip_exponent).
            g.reawaken["reawaken_catnip_exponent"] = float(lever[3:] or 1.0)
        elif lever.startswith("gate") and not lever.startswith("gates"):
            g.reawaken["reawaken_min_gain_fraction"] = float(lever[4:])
        elif lever.startswith("coef"):
            g.reawaken["reawaken_stardust_coefficient"] = float(lever[4:])
        elif lever in ("kit", "kitold"):
            # E4 (kit: the measured alternative of 2026-10-01, today's data) or the numbers before it (kitold).
            new = lever == "kit"
            u = g.upgrades["kitten_power"]
            u.effect_per_level, u.cost_growth = (1.6, 3.6372) if new else (1.3, 2.04)
            u = g.upgrades["kitten_count"]
            u.effect_per_level, u.cost_growth = (2.0, 1.35) if new else (1.0, 1.2)
        elif lever.startswith("unl"):
            # E5: the Overkill unlock's splash (unl = today's 0.10; unl0.02 = before D17).
            g.branches["unlock_overkill"].level_effects = [float(lever[3:] or 0.10)]
        elif lever == "hpold":
            for t in g.tiers:
                if t.boss:
                    t.boss["total_hp"] = _HP_BEFORE_D17[t.boss["id"]]
        elif lever.startswith("bait"):
            # bait<f>: kills_per_bait times f (bait2 = twice the kills a bait); with 'pass' this is the FIRST farm only.
            f = float(lever[4:])
            for t in g.tiers:
                if t.boss:
                    t.boss["kills_per_bait"] = max(1, int(round(float(t.boss["kills_per_bait"]) * f)))
        elif lever.startswith("gates"):
            f = float(lever[5:])
            for t in g.tiers:
                t.catnip_to_unlock = float(t.catnip_to_unlock) * f
        elif lever.startswith("uni"):
            # uni<f>: owning the Unicorn multiplies paw and aura damage by f (D17: 1.5; uni1 = before D17)
            g.branches["unlock_unicorn"].level_effects = [float(lever[3:])]
        elif lever.startswith("hpall"):
            f = float(lever[5:])
            for t in g.tiers:
                if t.boss:
                    t.boss["total_hp"] = float(t.boss["total_hp"]) * f
        elif lever.startswith("hp"):
            # hp<tier>x<f>: that boss's HP times f
            tier_s, f = lever[2:].split("x")
            t = g.tiers[int(tier_s) - 1]
            t.boss["total_hp"] = float(t.boss["total_hp"]) * float(f)
        elif lever:
            raise SystemExit(f"unknown lever {lever}")


# ---------------------------------------------------------------------------------------------------------------------
# Setup (mirrors pawsim.main for --policy v4 --data csv)
# ---------------------------------------------------------------------------------------------------------------------

def load():
    code = P.load_code_constants_csv()
    P.CATCHES_BASE = float(code.get("card_level_cost_base", P.CATCHES_BASE_DEFAULT))
    P.CATCHES_GROWTH = float(code.get("card_level_cost_growth", P.CATCHES_GROWTH_DEFAULT))
    P.COVERAGE_MODEL = True
    P.FIX_F1 = P.FIX_F2 = P.FIX_F3 = True
    P.ADD_LEAK_FRACTION = P.ADD_LEAK_DEFAULT
    P.GOLDEN_CATCH_RATE = P.GOLDEN_CATCH_DEFAULT
    return P.load_game_data_csv()


# ---------------------------------------------------------------------------------------------------------------------
# Metrics
# ---------------------------------------------------------------------------------------------------------------------

NOTICE = 0.08


def pct(xs, q):
    if not xs:
        return 0.0
    xs = sorted(xs)
    return xs[min(int(q * len(xs)), len(xs) - 1)]


def metrics(res, g, label):
    total = res.total_seconds
    rea_t = [r["t"] for r in REA]
    bounds = [0] + rea_t + [total]
    runs = []
    for i in range(len(bounds) - 1):
        a, b = bounds[i], bounds[i + 1]
        ticks = [x for x in TICKS if a <= x[0] < b]
        first_at = {}
        for x in ticks:
            for k in range(1, x[2] + 1):
                first_at.setdefault(k, x[0] - a)
        deepest = max((x[2] for x in ticks), default=1)
        top_wave = max((x[3] for x in ticks), default=0)
        inc2 = next((x[4] for x in ticks if x[0] - a >= 120), 0.0)
        dps2 = next((x[5] for x in ticks if x[0] - a >= 120), 0.0)
        buys = [x for x in BUYS if a <= x[0] < b]
        nb = [x[0] for x in buys if x[5] >= NOTICE]
        gaps_n = [nb[k] - nb[k - 1] for k in range(1, len(nb))]
        if nb:
            gaps_n.append(b - nb[-1])
            gaps_n.insert(0, nb[0] - a)
        moments = sorted(set(x[0] for x in buys))
        gaps_a = [moments[k] - moments[k - 1] for k in range(1, len(moments))]
        if moments:
            gaps_a.append(b - moments[-1])
        runs.append(dict(idx=i + 1, start=a, minutes=(b - a) / 60.0, deepest=deepest, top_wave=top_wave,
                         first_at=first_at, income_2min=inc2, dps_2min=dps2, buys=len(buys),
                         noticeable=len(nb), worst_wait_notice=max(gaps_n, default=b - a),
                         worst_wait_any=max(gaps_a, default=b - a),
                         sd=REA[i]["sd"] if i < len(REA) else 0.0,
                         mult_after=REA[i]["mult_after"] if i < len(REA) else None))
    # Reawaken feel: run N reaching run N-1's deepest biome.
    for i in range(1, len(runs)):
        prev, cur = runs[i - 1], runs[i]
        d = prev["deepest"]
        prev_t = prev["first_at"].get(d)
        cur_t = cur["first_at"].get(d)
        cur["catchup_s"] = cur_t
        cur["prev_reach_s"] = prev_t
        cur["catchup_ratio"] = (cur_t / prev_t) if (cur_t is not None and prev_t) else None
        cur["deeper"] = cur["deepest"] > prev["deepest"]
        cur["income_vs_prev_2min"] = (cur["income_2min"] / prev["income_2min"]) if prev["income_2min"] else None
    # Whole-game waits.
    allb = [x[0] for x in BUYS if x[5] >= NOTICE]
    gaps = [allb[k] - allb[k - 1] for k in range(1, len(allb))]
    long_wait_time = sum(gp for gp in gaps if gp > 180)
    moments = sorted(set(x[0] for x in BUYS))
    gaps_any = [moments[k] - moments[k - 1] for k in range(1, len(moments))]
    kb = sum(1 for x in TICKS if x[8] < 1.0) / max(len(TICKS), 1)
    out = dict(
        kill_bound_share=kb,
        label=label, cleared=res.cleared, total_min=total / 60.0, reawakens=res.reawaken_count,
        end_mult=res.end_multiplier, dead_air_min=res.dead_air / 60.0, drought_min=res.drought / 60.0,
        drought_window=[res.drought_window[0] / 60.0, res.drought_window[1] / 60.0],
        shop_buys=len(BUYS), noticeable_buys=len(allb),
        notice_gap_med=pct(gaps, 0.5), notice_gap_p90=pct(gaps, 0.9), notice_gap_max=max(gaps, default=0),
        share_time_waiting_over_3min=long_wait_time / max(total, 1.0),
        any_gap_med=pct(gaps_any, 0.5), any_gap_p90=pct(gaps_any, 0.9), any_gap_max=max(gaps_any, default=0),
        tier_arrival={k: v / 60.0 for k, v in res.tier_arrival.items()},
        bosses=[(st / 60.0, dur, idx) for st, dur, idx in res.boss_log],
        runs=runs, rea=REA, tree=TREE, ticks=TICKS, buys=BUYS, treestep=TREESTEP, bait_ready=BAITREADY,
        buys_by_line={},
    )
    for x in BUYS:
        e = out["buys_by_line"].setdefault(x[3], [0, []])
        e[0] += 1
        e[1].append(x[5])
    out["buys_by_line"] = {k: (v[0], statistics.median(v[1])) for k, v in out["buys_by_line"].items()}
    return out


def summary(m):
    print(f"\n===== {m['label']} =====")
    print(f"cleared {m['cleared']}  game {m['total_min']:.0f} min  Reawakens {m['reawakens']}  end mult {m['end_mult']:.0f}x")
    print(f"kill-bound share of play {100 * m['kill_bound_share']:.0f}%")
    print(f"dead air {m['dead_air_min']:.1f} min   nothing NEW {m['drought_min']:.1f} min "
          f"(min {m['drought_window'][0]:.0f}-{m['drought_window'][1]:.0f})")
    print(f"shop buys {m['shop_buys']}  noticeable {m['noticeable_buys']}  "
          f"wait for a noticeable buy: median {m['notice_gap_med']:.0f}s p90 {m['notice_gap_p90']:.0f}s "
          f"max {m['notice_gap_max'] / 60:.1f} min; time in waits > 3 min: {100 * m['share_time_waiting_over_3min']:.0f}%")
    print("run  min  deepest wave  catch-up(prev deepest)   deeper  income@2min vs prev  sd granted  worst wait (noticeable / any)")
    for r in m["runs"]:
        cu = ""
        if r.get("catchup_ratio") is not None:
            cu = f"{r['catchup_s'] / 60:5.1f} vs {r['prev_reach_s'] / 60:5.1f} min ({r['catchup_ratio']:.2f})"
        elif r["idx"] > 1:
            cu = "never reached"
        iv = r.get("income_vs_prev_2min")
        print(f"{r['idx']:>3} {r['minutes']:5.1f}  {r['deepest']:>4}  {r['top_wave']:>4}  {cu:<26} "
              f"{str(r.get('deeper', '')):<6} {'' if iv is None else f'x{iv:6.2f}':<10} {r['sd']:>12,.0f}   "
              f"{r['worst_wait_notice'] / 60:5.1f} / {r['worst_wait_any'] / 60:5.1f} min")
    print("first Bait ready: " + "  ".join(f"biome {k}: min {v[0] / 60:.0f}, a fight of {v[1]:.0f} s"
                                         for k, v in sorted(m["bait_ready"].items())))
    print("tree first levels (unlock -> util / dps / income gain at purchase):")
    for e in m["tree"]:
        if e["from_lv"] == 0:
            print(f"   run {e['run']:>2}  {e['id']:<22} lv->{e['to_lv']:<2} cost {e['cost']:>9,.0f}  "
                  f"util {100 * e['util_gain']:+7.1f}%  dps {100 * e['dps_gain']:+7.1f}%  income {100 * e['inc_gain']:+7.1f}%")


def waits_report(path):
    m = json.load(open(path))


    ticks = m["ticks"]
    buys = m["buys"]
    NOTICE = 0.08
    nb = [b for b in buys if b[5] >= NOTICE]
    boss_tiers = {3, 6, 8, 11, 12}


    def tick_at(t):
        best = None
        for x in ticks:
            if x[0] <= t:
                best = x
            else:
                break
        return best


    print(f"== {m['label']}: waits over 3 min between noticeable shop buys ==")
    tot = 0
    by_tier = {}
    for k in range(1, len(nb)):
        a, b = nb[k - 1][0], nb[k][0]
        if b - a <= 180:
            continue
        x = tick_at(a)
        tot += b - a
        by_tier[x[2]] = by_tier.get(x[2], 0) + (b - a)
        # time in the wait spent in each biome
        inside = {}
        for y in ticks:
            if a <= y[0] < b:
                inside[y[2]] = inside.get(y[2], 0) + 10
        where = " ".join(f"b{k2}:{v / 60:.0f}m" for k2, v in sorted(inside.items()))
        any_buys = sum(1 for z in buys if a < z[0] < b)
        print(f"  min {a / 60:6.1f} +{(b - a) / 60:5.1f}  run {x[1] + 1:>2}  {where:<24} wave {x[3]:>2}  "
              f"small buys inside {any_buys:>3}  surplus {x[8]:8.1f}x  next: {nb[k][3]}")
    print(f"  total {tot / 60:.0f} min of {m['total_min']:.0f}")
    print("  by biome at the wait's start: " + ", ".join(f"b{k}: {v / 60:.0f} min" for k, v in sorted(by_tier.items())))

    # Time per run in each biome, and how much of it is in boss biomes farming
    print("== minutes per run by biome ==")
    runs = {}
    for x in ticks:
        r = runs.setdefault(x[1] + 1, {})
        r[x[2]] = r.get(x[2], 0) + 10
    for r, d in sorted(runs.items()):
        print(f"  run {r:>2}: " + "  ".join(f"b{k}:{v / 60:4.1f}" for k, v in sorted(d.items())))

    if m.get("treestep"):
        print("== what each Reawaken's Stardust bought (power step from the tree spend) ==")
        per = {}
        for t, run, u0, u1, d0, d1, i0, i1 in m["treestep"]:
            e = per.setdefault(run, [1.0, 1.0, 1.0])
            e[0] *= (u1 / u0) if u0 else 1
            e[1] *= (d1 / d0) if d0 else 1
            e[2] *= (i1 / i0) if i0 else 1
        for run, (u, d, i) in sorted(per.items()):
            rea = next((r for r in m["rea"] if r["n"] == run), None)
            ms = f"mult x{rea['mult_after'] / rea['mult_before']:.2f}" if rea else ""
            news = [e["id"] for e in m["tree"] if e["run"] == run and e["from_lv"] == 0]
            print(f"  after Reawaken {run:>2}: tree util x{u:6.2f} dps x{d:6.2f} income x{i:6.2f}  {ms}  new: {', '.join(news)}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--label", default="current", help="output name (default: the proposal's)")
    ap.add_argument("--no-waves", action="store_true")
    ap.add_argument("--proposal", default="")
    ap.add_argument("--waits", default="", help="print where the long waits are in a saved run (a build/econ/*.json)")
    a = ap.parse_args()
    if a.waits:
        waits_report(a.waits)
        return
    OUT.mkdir(parents=True, exist_ok=True)
    if a.label == "current":
        a.label = a.proposal or "today"
    WAVES[0] = not a.no_waves
    g = load()
    wave_tiers(g)
    if a.proposal:
        apply(a.proposal, g, P, RULES)
    res = P.simulate(g, "v4")
    m = metrics(res, g, a.label)
    (OUT / f"{a.label}.json").write_text(json.dumps(m, indent=1, default=str))
    summary(m)


if __name__ == "__main__":
    main()
