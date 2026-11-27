# Economy review, 2026-11-27: the playtest's economy pass (E1-E6), measured together

> **A proposal. Nothing is built and nothing in `Data/` changed.** Every number comes from `tools/econ_review.py` (pawsim
> on `Data/*.csv`, plus the game's waves and a log of every buy, tree level and Reawaken; the levers are applied in
> memory). The worklist (docs/WORKLIST-PLAYTEST-2026-11-26.md §8) asks for six economy changes at once; they interact, so
> they are measured as one set. Each is the designer's call (CLAUDE.md: a balance change is proposed and made on a yes).

## 1. The targets

The designer, 2026-11-26: "Slow down game even more. We should be able to afford a kitten at 15 min. We should be the
strong one untill we get a few kittens."

| Goal | Target | Today |
|---|---|---|
| First kitten affordable | 15 min (D-a) | 3.4 min |
| Biome 2 | ~12 min | 4.5 |
| First Reawaken | ~25 min | 9.8 |
| First boss | ~90 min | 50.5 |
| Game won | ~6 h | 175 min |
| Waits over 3 min between noticeable buys | ≤ 10 % of play (D17) | 12 % |
| Longest stretch with nothing new | ≤ 15 min (D17) | 17.2 min |
| The cat outdamages the kittens | until ~3-4 kittens | 86 % at 2 kittens, 61 % at 4 |

## 2. What pawsim learned tonight (E0, the income cards)

pawsim had none of the cards. It now has the ones that change income: Hot Borders and Slow Burn (edge burn, and Slow
Burn's spawn cost), Bounty, Notoriety, Gold Rush, Thunderclap and Close Quarters (aura pulses), Ember Trail, Gilded Kitten.
- **How:** each card's level is its expected level after the run's picks so far (pawsim's own Catches count), the picks
  spread over the 28-card deck by weight. `focus<k>` makes the income cards k times as likely to be taken: a hand of three
  holds one of them ~70 % of the time against their 33 % share of the deck, so a player who always takes one is `focus5`.
- **Off by default** (`cards` / `cardsunc` turn it on). Off, econ_review's output is identical line for line to pawsim
  before the change; `tools/check_pawsim_cards.py` (in check_all) checks the effects against hand arithmetic and catches
  5 / 5 planted faults.
- **Two assumptions, named and exposed as levers:** edge burn closes `1 - exp(-4 v)` of what still escapes (`edge<k>`),
  and Ember Trail's trail burns half the time (`trail<f>`). pawsim has no space, so both wait for the calibration below.

What it found, on today's `Data/`:

| | First kitten | Biome 2 | First Reawaken | First boss | Game | Waits > 3 min |
|---|---|---|---|---|---|---|
| Today, no cards | 3.4 | 4.5 | 9.8 | 50.5 | 175 | 12 % |
| Cards spread over the deck | 3.2 | 4.2 | 8.7 | 49.5 | 179 | 18 % |
| A player who seeks income cards (`focus5`) | 3.1 | 4.0 | 7.9 | 49.4 | 188 | 25 % |
| The same, uncapped levels (E4) | 3.1 | 4.0 | 7.9 | 49.4 | 188 | 25 % |
| Edge burn assumed half / double | 3.2 / 3.1 | 4.2 / 3.8 | 8.4 / 7.7 | 50.1 / 48.8 | 189 / 187 | 25 % / 24 % |

- **The cards bring the early milestones 10-20 % sooner.** The waits rise with them under today's Bait: Slow Burn's spawn
  cost slows the kills that make Bait.
- **Uncapped levels change nothing measurable here:** a run is 10-19 picks, spread over nine income cards; none passes
  its table. E4's risk is the player who takes one card again and again, which this model does not try.
- **Edge burn's strength barely matters here.** pawsim's coverage model has the cat reaching ~97 % of what spawns once
  its damage outpaces the spawns, so there is little left to burn. The playtest saw many enemies walk off, so the model
  is probably too generous there. **Calibration against the real game is the next step** (the 5x soak on a package, at
  the designer's word), before E5's numbers are trusted.

## 3. The sets measured

The base, P: **E1** the roster ladder (`roster`), the Backyard at 60/min (today 120); **E3** +1 kitten a buy (today +2),
Kitten Count at 900 (today 150); Spawn Rate x1.4 a level (today x1.77) at 60 (today 14); **E2** no Bait (`nobait`), a boss
tried when it would fall within 120 s (`fight120`); biome 2's gate 4,000 (today 900). From q0 on, measured with the
card-seeking player and **E5**'s Hot Borders at 0.54 of today's values (outward-only: the measured -46 % exposure).

| Set | Changes on top | First kitten | Biome 2 | First Reawaken | First boss | Game | Waits > 3 min | Nothing new |
|---|---|---|---|---|---|---|---|---|
| P (no cards) | | 18.8 | 20.1 | 24.5 | 130 | 268 | 22 % | 27.0 min |
| q0 | cards, E5 | 19.3 | 20.6 | 23.6 | 129 | 255 | 20 % | 28.6 |
| q1 | kittens 700, gate 2,000 | 15.6 | 14.8 | 21.5 | 115 | 239 | 14 % | 25.9 |
| q1 uncapped | | 15.6 | 14.8 | 21.5 | 115 | 239 | 14 % | 25.9 |
| r1 | q1 + Park Tyrant HP x0.35 | 15.6 | 14.8 | 21.5 | 98 | 228 | 20 % | 25.9 |
| r2 | r1 + Spawn Rate at 40, gate 1,500 | 12.7 | 11.1 | 18.4 | 84 | 212 | 22 % | 22.5 |
| r3 | r2 + every boss x1.6 | 12.7 | 11.1 | 18.4 | 96 | 246 | 26 % | 22.5 |
| r4 | r2 + bosses 6, 8, 11, 12 x2 | 12.7 | 11.1 | 18.4 | 84 | 257 | 28 % | 38.4 |
| r5 | r2 + Kitten Count 850 | 13.0 | 11.1 | 18.5 | 85 | 213 | 23 % | 22.5 |
| r6 | r2 + Kitten Count 1,200 | 17.2 | 11.1 | 17.9 | 84 | 211 | 20 % | 22.7 |
| **r7** | **r2 + Kitten Count 1,000** | **17.0** | **11.1** | **17.8** | **83** | **211** | **19 %** | **22.6** |

- **The first kitten jumps** between Kitten Count 850 (13 min) and 1,000 (17 min): a price just past the first run's
  savings is bought right after the first Reawaken.
- **The early targets are reachable.** Kitten Count's price sets the first kitten; biome 2's gate and Spawn Rate's first
  price set biome 2; the Park Tyrant's HP sets the first boss (with no Bait, strength is the only gate, so his HP comes
  down: x0.35 puts him at ~85-100 min).
- **The game is too short and the waits too long, and the two pull against each other.** Making later bosses tougher
  lengthens the game (r3, r4) but adds waits (26-28 %) and a 38-minute stretch with nothing new.
- **Where the waits are** (`--waits build/econ/r2.json`): 34 of r2's 46 waiting minutes are in biomes 8-12, with 60-1,300x
  the Catnip the next buy costs. The lines are not capped (Paw Damage and Currency Find have no cap); the wait is for a
  buy big enough to notice. Late in the game a Paw Damage level is a small share of a damage total the kittens, aura and
  tree carry, so no single buy registers. **This is structural:** prices and boss HP cannot fix it, and it is the same
  thing as the new rule "the cat is the strong one until a few kittens" seen from the other end. The fix is in what the
  shop offers late (bigger steps on the lines that matter then, or lines that multiply the kittens and aura), a design
  question below.
- **The cat's share of damage** cannot be read yet: in these sets the first run ends with one or two kittens (96 % at
  one, 81 % at two). It needs a longer first run or a per-kitten-count readout across runs (a tool change, next).

## 4. The recommendation: r7

| Change | Today | r7 | Item |
|---|---|---|---|
| The roster ladder (one common, the next biome's common as the rare, the signature rarer, fliers and gristles from a spawn multiplier of 2 and 4) | 4-6 fixed kinds | the ladder | E1 |
| Backyard spawn rate | 120 / min | 60 / min | E1 |
| Spawn Rate: a level's step, the first level's price | x1.77, 14 | x1.4, 40 | E1 / E6 |
| Kitten Count: kittens a buy, the first price | +2, 150 | +1, 1,000 | E3 |
| Biome 2's gate | 900 Catnip | 1,500 | E6 |
| Bait | 40 a boss, from kills | none: Summon, a 120 s limit | E2 |
| Park Tyrant's HP | 3.97 x 10^7 | x0.35 (1.39 x 10^7) | E2 |
| Hot Borders | 0.10 / 0.16 / 0.24 | outward-only (modelled as x0.54) | E5 |

What it gives against the targets:

| Goal | Target | r7 | Met? |
|---|---|---|---|
| First kitten | 15 min | 17.0 | close |
| Biome 2 | ~12 | 11.1 | yes |
| First Reawaken | ~25 | 17.8 | no: early |
| First boss | ~90 | 83 | yes |
| Game won | ~6 h | 3.5 h | no |
| Waits over 3 min | ≤ 10 % | 19 % | no |
| Nothing new | ≤ 15 min | 22.6 min (min 57-80) | no |

**Read it as: the first hour is right, the rest needs a design answer, not more tuning.** Every lever tried tonight that
lengthens the game (tougher bosses, dearer gates) also adds waits, because the late waits are for a buy big enough to
notice, and late in the game no shop line is (§3). Making the game 6 hours long with today's shop means more waiting.

## 5. Decisions for the designer

1. **Take r7 as the first hour** (E1, E2, E3's numbers, biome 2's gate, the Park Tyrant). Recommended.
2. **The length.** Options, each to measure:
   - (a) accept ~3.5 h for now and lengthen it once the late shop is fixed (recommended);
   - (b) tougher later bosses now (r3 / r4: 4-4.3 h, waits 26-28 %, a 38-minute stretch with nothing new).
3. **The late shop** (the waits' cause). Options to sketch, then measure:
   - (a) late lines that multiply what carries the damage then (kittens, aura), unlocked by biome;
   - (b) bigger steps on the cat's own lines past a biome (a "tier" of each line);
   - (c) the tree's spend step counted as "something to buy" (it is, for the player; the metric ignores it).
4. **Calibrate before building** (the worklist's E0 last step): the 5x soak on a package, so pawsim's minutes are the
   game's. Packaging is your word only. The soak also settles how many enemies really escape (§2), which decides E5's
   numbers.
5. **E4 (uncapped cards):** pawsim cannot see a difference; build it as planned, and the soak measures the player who
   stacks one card.

## 6. What these numbers cannot see

- **Space.** pawsim has no field: coverage (who escapes), edge burn and Ember Trail are modelled through named
  assumptions (§2).
- **E3's kitten speed** (260 → 180 px/s) is not modelled: pawsim's kittens have no speed, only the Kitten Speed line's
  share of coverage.
- **A real player** buys late, misses the Summon window, and takes cards by taste. These numbers are the player who buys
  at once and Reawakens when allowed.
- **The cat's share of damage** past two kittens (§3): the first run ends with one or two kittens in every set.

## 7. Reproduce

```
B="roster+tier.1.spawn_rate_per_min=*0.5+up.kitten_count.effect_per_level=1+up.spawn_rate.effect_per_level=1.4"
P="$B+up.kitten_count.base_cost=900+up.spawn_rate.base_cost=60+tier.2.catnip_to_unlock=4000+nobait+fight120"
py tools/econ_review.py --proposal "$(cat build/econ/r7.proposal.txt)" --label r7    # the recommendation
py tools/econ_review.py --waits build/econ/r7.json                                     # where its waits are
py tools/econ_review.py --proposal cards+focus5 --label e0_focus5                      # today, the card-seeking player
py tools/check_pawsim_cards.py [--self-test]                                           # the card model's check
```
The r-sets' lever strings are in `build/econ/r5.proposal.txt` .. `r7.proposal.txt`; the q- and r-runs' logs in
`build/econ/<label>.json`. One run takes ~2.5 min (~4 with four at once).
