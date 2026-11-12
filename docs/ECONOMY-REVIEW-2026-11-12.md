# Economy review, 2026-11-12: what the economy should be, and today's measured against it

> The designer, 2026-11-12: "I want you to look into the economy again. The important thing is that the gameplay should be
> fun and the player should never be left for long periods with nothing to buy. Every awakening should make the player feel
> stronger and unlocks should feel meaningful. Rethink the whole economy again and see what it should be like. Then compare
> to our current economy."
>
> **BUILT the same day (D17, HANDOFF-LOG (102))**: the designer chose all six changes and "shorter denser game", and made
> the Unicorn a damage step (x1.5). E6's boss HP came out of a fixed point on the first fight's length when its Bait is
> ready (x2.37 / x2.04 / x15.8 / x31 / x139: 89-171 s). Measured on the built game (`py tools/econ_review.py`): 112 min,
> 13 Reawakens, waits over 3 min 4 % (longest 4.0 min), nothing new at most 14 min. `--proposal before_d17` reproduces the
> "today" column below. The text below is the review as the designer read it.
>
> **A proposal. Nothing is built and nothing in `Data/` changed.** Every number below comes from `tools/econ_review.py`
> (pawsim on `Data/*.csv`, the player who Reawakens as soon as the gate opens, plus the game's waves, plus a log of every
> buy, tree level and Reawaken; proposals are applied in memory). Each change is the designer's call (D16: a change with a
> balance trade-off is proposed and made on a yes). Godot's economy rework of 2026-08-24
> (`C:\Games\pawpocalypse\docs\pawpocalypse-economy-redesign.md`) is the background: its exponent budget, Overkill bounty and
> waves are in the game and stay.

---

## 1. The three goals as numbers

| Goal | Measured as | Target |
|---|---|---|
| Never long with nothing to buy | share of play spent in waits of over 3 minutes between **noticeable** shop buys (a buy that lifts income x damage by 8 % or more, pawsim's `NOTICEABLE_GAIN`); the longest such wait | under 10 %; under 5 min |
| A buy means something | share of shop buys that are noticeable | over 50 % |
| Every Reawaken makes the player stronger | minutes of each run spent getting back to where the last run ended (the **re-climb**); income 2 minutes into a run against the run before | under 15 % of the game; x2 or more a run |
| Unlocks mean something | each one-level tree unlock is a new verb or moves damage or income by 25 % or more; the longest stretch with nothing NEW (a biome, boss, stage, card level or tree node never had before) | no unlock under 5 %; under 20 min |

## 2. Today's economy, measured

| | today |
|---|---|
| game (pawsim time; the 5x soak of 2026-10-02 took ~1.5x pawsim's) | 406 min, 11 Reawakens |
| **re-climb** | **210 min, 52 % of the game**; 15-42 min of every run from run 5 on |
| income 2 min into a run against the run before (runs 3-6) | x1.00 to x1.09 |
| **waits of over 3 min for a noticeable buy** | **44 % of play**; the longest 25.8 min (median 14 s, p90 2.1 min) |
| nothing affordable at all, longest | 15.6 min |
| noticeable buys | 499 of 1,222 (41 %) |
| nothing new, longest | 70 min (biome 12) |
| boss fights | 22, of which 17 are rematches of bosses already beaten |

Why, in order of size:

1. **Passage re-locks on every Reawaken, and the rematch is paid in kills.** A boss beaten in an earlier run blocks the road
   again until it is fought again, and a fight needs 40 Bait, which only kills in its own biome drop (`kills_per_bait`
   240 / 811 / 1,500 / 2,200 / 2,800: 9,600 to 112,000 kills a boss). Kills are capped by the spawner (the cat's damage is
   many times what the spawner supplies for most of the game), so nothing a Reawaken gives makes them come faster. The last
   run is 59 minutes, 42 of them re-fighting four beaten bosses. This is most of the re-climb, most of the long waits (the
   cat stands in old biomes where every buy is already made) and the 70-minute stretch with nothing new.
2. **A Reawaken multiplies damage, and the run is paced by income.** The multiplier (x1.87 a Reawaken) goes into damage
   that is already surplus; Catnip a kill does not move. Runs 3-6 start exactly as fast as the run before.
3. **A Reawaken throws away Bait earned toward the next, unbeaten boss** (the kill bank is cleared with the waves).
4. **Late in a run the shop runs dry.** 11 of the 15 lines are capped and reach their cap within 1-4 levels in a whole game;
   then only Paw Damage (cost x3.49 a level) and Currency Find (x3.26) are left, against income that grows slowly inside a
   run. Kitten Count and Kitten Power are 595 of the 1,222 buys at a median gain of 0.2 % and 1.8 %.
5. **Several unlocks are small, and the late Reawakens buy nothing new.** Gain at the moment of purchase: Aura +79 % damage,
   Vitality +59 %, Pounce +30 %, Claws +22 %, Prism +12 %, the Unicorn with Rainbow Poop (2,500 + 600 Stardust, the
   ceremony unlock) +13 %, Fortune +5 % income, the Overkill unlock +3 % income (its tooltip says "2 %"). Reawakens 9 and
   11 buy no new node; Reawaken 10 buys two that measure about 0.

## 3. What the economy should be

**Principles**
- **P1. A Reawaken resets the run, never progress.** It clears Catnip, the shop, cards, waves and the Catnip gates; it never
  takes back a boss beaten or Bait earned.
- **P2. A Reawaken pays what paces the run.** That is Catnip as well as damage, so every run visibly starts faster.
- **P3. Time goes into the frontier**: the next biome, the next boss, its Bait. Never into re-doing.
- **P4. Every buy is a step**, and every unlock is a verb or a step you can see.
- **P5. Length comes from content, not repetition** (the designer, 2026-10-01: "the fix for length is content").

**The changes**

| # | Change | Fixes | Kind |
|---|---|---|---|
| E1 | **A beaten boss stays beaten.** Passage no longer re-locks on a Reawaken; a rematch stays possible | cause 1 | rule; reverses Godot's 2026-08-27 re-lock |
| E2 | **Bait earned toward a boss not yet beaten survives a Reawaken** (waves still reset) | cause 3 | rule; a save field |
| E3 | **The Reawaken multiplies Catnip too**, by the square root of the damage multiplier, `(1 + Stardust ever / 2.5) ^ 0.25`: x1.37 a Reawaken (damage keeps x1.87) | cause 2 | rule; one line on the Reawaken tab (words to draft) |
| E4 | **Bigger kitten buys**: Kitten Power x1.6 a level (cost growth 3.6372), Kitten Count +2 a buy (cost growth 1.35); the alternative measured 2026-10-01 | cause 4 | data |
| E5 | **The Overkill unlock pays 10 %**, not 2 % | cause 5 | data |
| E6 | **Boss HP re-priced** for the new damage curve. Without it every boss dies in 2-74 s; x30 (the stand-in measured here) gives first fights of 49-349 s (today 26-226 s). Final values by fixed point once E1-E3 are chosen | a consequence of E1-E3 | data |

E1 alone does most of the work. E3 makes the early Reawakens (runs 2-6) feel faster. E2 and E6 tidy the frontier.

## 4. Today against the redesign

| | today | E1 only | **recommended (E1-E6)** |
|---|---|---|---|
| game (pawsim) | 406 min | 243 min | **155 min** |
| Reawakens | 11 | 11 | 13 |
| re-climb | 52 % | 16 % | **6 %** |
| income 2 min into a run against the run before | x1.00-x1.09 (runs 3-6) | x1.00-x1.09 (runs 3-6) | **x2.1 or more, every run** |
| waits of over 3 min for a noticeable buy | 44 % | 27 % | **5 %** |
| longest wait for a noticeable buy | 25.8 min | 9.3 min | **4.3 min** |
| nothing affordable at all, longest | 15.6 min | 4.7 min | 4.3 min |
| noticeable buys | 41 % | 41 % | **60 %** |
| nothing new, longest | 70 min | 35 min | **19 min** |
| boss fights (rematches) | 22 (17) | 5 (0) | 5 (0) |
| first fights | 26-226 s | 27-232 s | 49-349 s |

What each Reawaken gives, today against recommended: damage x1.87 both; Catnip x1.00 against x1.37; the run after it
re-climbs in 2-42 min (growing every run) against 0.1-2.6 min (under a minute from run 6 on). The Overkill unlock:
+3 % income against +9 %.

**Not met by E1-E6: something new on every late Reawaken.** In both economies the tree's last unlocks are bought by
Reawaken 8 or 10; after that a Reawaken buys ladder levels only (and, in the game, the job a boss kill opened, which pawsim
never buys). Spreading the remaining unlocks (Full Spectrum, Second Helping, Curiosity, the jobs' rule-changers) over the late
Reawakens is a price pass on the tree, best made once the chosen economy's Stardust curve is known.

**Alternatives measured**
- **Out-classed bosses wave you through** (instead of E1: a beaten boss lets you pass only when you would kill it in 10 s or
  less): 292 min, re-climb 31 %, waits 31 %, longest wait 9.6 min. It keeps the ritual and keeps most of the problem.
- **E3 at full strength** (Catnip x the whole damage multiplier): Catnip feeds Stardust, which feeds the multiplier. Runs
  collapse to 1-9 minutes and the game to 159 min with 17 Reawakens. Too strong; the square root is the measured middle.
- **A smaller Reawaken step** (the gate at +150 % Stardust instead of +250 %): more, shorter runs, each only x1.58. With
  E1 alone it helps (waits 27 % -> 15 %); with E3 in, the waits stay at 11 % either way, the game gets longer (183 min
  against 156) and every Reawaken is a smaller step, the opposite of what the designer asked for. The gate stays.
- **Raising the Catnip gates of every biome** x3 or x10 adds only 20-30 min: in the new economy they pace the first visit,
  and the Catnip multiplier eats them on the re-climbs.

## 5. Length: the trade-off to decide

Today's 406 minutes are about half re-climb, and 44 % of play is spent waiting. With the repetition gone, today's content at
the recommended pace is **about 2.6 hours of pawsim time (about 4 hours of real play, at the soak's ratio)**. The levers that
add time back without repetition all lengthen the frontier, and the measured cost is waiting:

| recommended plus | game | waits over 3 min | longest wait | nothing new, longest |
|---|---|---|---|---|
| nothing | 155 min | 5 % | 4.3 min | 19 min |
| twice the kills a Bait | 184 min | 9 % | 5.4 min | 26 min |
| three times the kills a Bait | 237 min | 21 % | 7.8 min | 33 min |

So: **accept a shorter, denser game, or add content** (biomes, bosses, jobs). Stretching the frontier past ~3 hours of
pawsim time brings back the waits that this review exists to remove.

## 6. What these numbers cannot see

- **The player is pawsim's**: an optimal greedy buyer who Reawakens the moment the gate opens. That makes some runs 3-6
  minutes long right after a big jump; a person would likely play on. Real play is slower (the 5x soak: ~10 h against
  pawsim's 6h42m).
- **No positions.** Coverage (Aura Radius, Kitten Speed) and the spatial unlocks (Snare, Deadweight, Funnel, Aura Reach,
  Refraction, Critical Mass, Full Spectrum) all measure about 0 here, which says nothing about them. Godot's field
  simulator (2026-08-24) found Aura Radius the largest income lever in the game. **Judge them in `PcField`** (the port's own
  field simulation: kills and escapes with and without each) before changing any of them.
- **Not valued**: jobs, cards, automation, Legacy, achievements. pawsim never buys a job, so the late game's newest things
  (a job after each boss) are missing from the "nothing new" numbers in both columns.
- **pawsim itself has no waves.** This tool adds them. With waves, today's game is 406 min against pawsim's 403 (similar
  length, different details). Rule 14 (doc 02) says pawsim should learn every rule that changes what the player can reach;
  waves were added to Godot after pawsim was copied.

## 7. Decisions for the designer

1. **E1, a beaten boss stays beaten.** The big one. (Or the out-classed variant, measured above: much weaker.)
2. **E2, Bait survives a Reawaken.**
3. **E3, the Reawaken multiplies Catnip** (x1.37 a Reawaken), and its line on the Reawaken tab (words drafted on a yes).
4. **E4 and E5**, the kitten buys and the Overkill unlock.
5. **Length**: the shorter game, or content to fill it (§5).
6. **The Unicorn**: today it costs 2,500 Stardust, comes with a ceremony and gives +13 % with Rainbow Poop. Should mounting
   the Unicorn itself be a damage step? (Not measured; a number for the designer to choose.)

On a yes, building them is D16 work: each rule changed in PawCore with a test that fails on today's rule, the Godot fixture
made with the same fix (`tools/godot_fixes.py`), pawsim taught the rule in the same pass (rule 14), a save-version bump for
E2's Bait bank (doc 08), doc 02 §4 rewritten ("passage re-locks every cycle" goes), and E6's boss HP re-priced by fixed point.

## 8. Reproduce

```
py tools/econ_review.py                              # today              -> build/econ/today.json
py tools/econ_review.py --proposal recommended       # E1-E6              -> build/econ/recommended.json
py tools/econ_review.py --proposal pass              # E1 only
py tools/econ_review.py --proposal outclass10        # the out-classed variant
py tools/econ_review.py --proposal recommended+bait2 --label longer
py tools/econ_review.py --waits build/econ/today.json   # where the long waits are, run by run
```
Levers: `pass` (E1), `keepbait` (E2), `inc<k>` (E3: Catnip x multiplier^k), `kit` (E4), `unl` (E5), `hpall<f>` (E6), and
`outclass<s>`, `gate<f>`, `gates<f>`, `bait<f>`, `hp<tier>x<f>`, `coef<f>`. About 2.5 min a run; runs are deterministic.
