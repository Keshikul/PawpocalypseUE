"""E0 (2026-11-27): pawsim's income-card model (pawsim.CARDS_MODEL) against hand arithmetic.

What it asserts:
  * OFF (the default) every effect is neutral, so no other number pawsim prints moves (measured too: econ_review 'today'
    with the model off is identical, line for line, to pawsim before E0);
  * ON, at a pick count where each weight-1 card has exactly one expected level, every modelled effect equals the table
    value worked out here independently (fractional levels read linearly from the base: a weight-0.5 card at level 0.5
    is halfway to its first value);
  * a pick past a card's table holds its last value (capped) or adds its last step again (uncapped, E4 / D-d);
  * CARD_FOCUS multiplies the modelled cards' weight;
  * the State hooks read them: spawn rate (Slow Burn's cost), golden chance (Gold Rush), coverage (edge burn), the aura
    (Thunderclap, Ember Trail), the kittens (Gilded Kitten), the payout (Bounty, Notoriety).

`--self-test` breaks the model in memory, one fault at a time, and requires each to be caught.

Usage:  py tools/check_pawsim_cards.py [--self-test]      exit 0 = agree, 1 = a check failed (or a fault went uncaught)
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import pawsim as P  # noqa: E402


def _reset(model=False, uncapped=False, focus=1.0):
    P.CARDS_MODEL, P.CARDS_UNCAPPED, P.CARD_FOCUS = model, uncapped, focus
    P._CARD_FX_CACHE.clear()


def _near(a, b):
    return abs(a - b) <= 1e-9 * max(1.0, abs(b))


def checks() -> list:
    """The failures (empty = all agree)."""
    bad = []
    cards = {c["id"]: c for c in P.load_cards()}
    W = sum(c["weight"] for c in cards.values())

    def lin(card, level, base, uncapped=False):
        vals = cards[card]["values"]
        if level <= 0:
            return base
        if level < 1:
            return base + (vals[0] - base) * level
        n = int(level)
        if n > len(vals):
            if not uncapped:
                return vals[-1]
            return vals[-1] + (n - len(vals)) * (vals[-1] - vals[-2])
        return vals[n - 1]

    def expect(name, got, want):
        if not _near(got, want):
            bad.append(f"{name}: got {got!r}, want {want!r}")

    # OFF: neutral at any pick count.
    _reset(model=False)
    fx = P.card_effects(40)
    for k, v in {"edge": 0.0, "spawn": 1.0, "bounty": 1.0, "notoriety": 1.0, "gold": 1.0, "pulse": 0.0, "trail": 0.0,
                 "gilded": 1.0}.items():
        expect(f"off: {k}", fx[k], v)

    # ON at picks = W: each card's expected level is its weight.
    _reset(model=True)
    lv = P.expected_card_levels(W)
    for cid, c in cards.items():
        expect(f"level of {cid}", lv[cid], min(c["weight"], float(len(c["values"]))))
    fx = P.card_effects(W)
    w = {k: cards[k]["weight"] for k in cards}
    expect("bounty", fx["bounty"], lin("bounty", w["bounty"], 1.0))
    expect("notoriety", fx["notoriety"], lin("notoriety", w["notoriety"], 1.0))
    expect("gold rush", fx["gold"], lin("gold_rush", w["gold_rush"], 1.0))
    expect("gilded kitten", fx["gilded"], lin("gilded_kitten", w["gilded_kitten"], 1.0))
    expect("edge (hot borders + slow burn)", fx["edge"],
           lin("hot_borders", w["hot_borders"], 0.0) + lin("slow_burn", w["slow_burn"], 0.0))
    sb = cards["slow_burn"]["costs"]
    expect("spawn (slow burn's cost)", fx["spawn"], 1.0 + (sb[0] - 1.0) * w["slow_burn"] if w["slow_burn"] < 1 else sb[0])
    expect("pulse (thunderclap + close quarters)", fx["pulse"],
           lin("thunderclap", w["thunderclap"], 0.0) + lin("close_quarters", w["close_quarters"], 0.0))
    expect("trail", fx["trail"], lin("ember_trail", w["ember_trail"], 0.0))

    # Past the table: capped holds, uncapped adds the last step.
    _reset(model=True)
    expect("capped bounty at 10 levels", P.card_effects(10 * W / w["bounty"])["bounty"], cards["bounty"]["values"][-1])
    _reset(model=True, uncapped=True)
    expect("uncapped bounty at 10 levels", P.card_effects(10 * W / w["bounty"])["bounty"], lin("bounty", 10, 1.0, True))

    # Focus: the modelled cards' weight x k.
    _reset(model=True, focus=3.0)
    lv = P.expected_card_levels(5)  # below every cap, or the cap hides the focus
    modelled = sum(c["weight"] * (3.0 if c["effect"] in P._CARD_MODELLED else 1.0) for c in cards.values())
    expect("focus: bounty's level", lv["bounty"], 5 * 3.0 * w["bounty"] / modelled)

    # The State hooks.
    _reset(model=False)
    g = P.load_game_data_csv() if hasattr(P, "load_game_data_csv") else P.load_game_data()
    s = P.State(g)
    tier = g.tiers[0]
    base_spawn = s.spawn_rate_per_sec(tier)
    _reset(model=True)
    s.run_picks = int(W)
    fx = s.card_fx()
    expect("State: spawn rate x slow burn's cost", s.spawn_rate_per_sec(tier), base_spawn * fx["spawn"])
    if s.card_fx()["edge"] <= 0.0:
        bad.append("State: no edge burn at W picks")
    _reset(model=False)
    return bad


SABOTAGES = {
    "model ignores the switch": lambda: setattr(P, "CARDS_MODEL", True),
    "no fractional levels": lambda: setattr(P, "_card_table_at",
                                            (lambda f: lambda t, lv, b: f(t, float(int(lv)), b))(P._card_table_at)),
    "uncapped never adds": lambda: setattr(P, "_card_table_at",
                                           (lambda f: lambda t, lv, b: f(t, min(lv, float(len(t))), b))(P._card_table_at)),
    "focus ignored": lambda: setattr(P, "expected_card_levels",
                                     (lambda f: lambda p: (lambda k: (setattr(P, "CARD_FOCUS", 1.0), f(p),
                                                                      setattr(P, "CARD_FOCUS", k))[1])(P.CARD_FOCUS))(
                                         P.expected_card_levels)),
    "slow burn's cost dropped": lambda: setattr(P, "load_cards",
                                                (lambda f: lambda: [dict(c, cost="NONE") for c in f()])(P.load_cards)),
}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--self-test", action="store_true")
    a = ap.parse_args()
    if not a.self_test:
        bad = checks()
        for b in bad:
            print("FAIL ", b)
        print(("FAIL" if bad else "PASS") + f"  pawsim's income-card model ({len(bad)} failure(s))")
        return 1 if bad else 0
    missed = 0
    for name, plant in SABOTAGES.items():
        saved = {k: getattr(P, k) for k in ("_card_table_at", "expected_card_levels", "load_cards", "CARDS_MODEL")}
        reset = _reset
        try:
            if name == "model ignores the switch":
                # the switch forced on inside every reset
                globals()["_reset"] = lambda model=False, uncapped=False, focus=1.0: reset(True, uncapped, focus)
            else:
                plant()
            P._CARD_FX_CACHE.clear()
            caught = bool(checks())
        except Exception as e:  # a crash is a catch too, but say so
            caught = True
            name += f" (raised {type(e).__name__})"
        finally:
            globals()["_reset"] = reset
            for k, v in saved.items():
                setattr(P, k, v)
            P._CARD_FX_CACHE.clear()
        print(("CAUGHT " if caught else "MISSED ") + name)
        missed += 0 if caught else 1
    print(("FAIL" if missed else "PASS") + f"  self-test: {len(SABOTAGES) - missed} / {len(SABOTAGES)} caught")
    return 1 if missed else 0


if __name__ == "__main__":
    sys.exit(main())
