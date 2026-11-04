# Night worklist, 2026-10-28 (unattended)

> **Closed (history).** Every item is done; the work continues from the newest HANDOFF-LOG entry and docs/11. Where this list
> says "Second Opinion ported as Godot has it", that was superseded on 2026-10-29 (doc 01 §5 F1, HANDOFF-LOG (9)).

The designer left for the night and asked for a long list of work plus long-running tests. This is that list, in the order it
is worked. Each item has a "done when"; the status column is updated as the night goes, and the newest HANDOFF-LOG entry
summarises the result. Phase 1 of docs/11 is the work: the game's rules in PawCore, proved against pawsim.

**Ground rules for the night** (CLAUDE.md standing rules): no commit, push, package or export (not asked); the spike code stays
until a commit keeps it in history; no design changes (a difference with Godot is a port bug); the Godot project is read-only
(every run that reads it leaves it byte-identical, checked with `git status` there); no game windows needed (everything below
is command-line or offscreen); every assertion negative-tested once.

| # | Work | Done when | Status |
|---|---|---|---|
| 1 | **pawsim into this repo** (`tools/pawsim.py`, doc 06 §7): copy unchanged, data dir pointed at the Godot repo read-only, baseline full-game reports for every policy | baseline reports stored under `build/pawsim/` | **done**: `tools/pawsim.py` copied; reads the Godot project read-only (`PAWSIM_GODOT_ROOT`); default run 4.5 min |
| 2 | **pawsim reads `Data/*.csv`** (one loader, enums by name, no rule change) | the CSV-loaded report is **byte-identical** to the `.tres`-loaded report for every policy | **done**: `--data csv`; full-game report byte-identical (MD5 13fc707a... both) |
| 3 | **Long test A: pawsim parity sweep**: both loaders over a grid of flags (policies x targeting x add-leak x boss-hp scale x fixes) | every combination byte-identical; script `tools/check_pawsim_parity.py` | running (108 settings x 2 loaders) |
| 4 | **PawCore CSV reader + `PcData`** (plain structs for all 11 CSVs, enums by name, lower-case ids) | loads every CSV; row counts and spot values asserted in L1 tests | **done**: `PcCsv`, generated `PcDataRows.h` (`tools/gen_pc_data.py`), `PcEnums.h` (`tools/gen_pc_enums.py`), `PcData`; 335 assertions |
| 5 | **Validator** (doc 06 §6, Godot `check_resources` intent) | every assertion implemented AND negative-tested (a mutated copy of the data makes it fail, test proves the failure) | **done**: 53 rules, each negative-tested; found 2 real Godot data bugs (unreachable achievements) and a Godot gate that never ran |
| 6 | **`PcRng`, `PcFormat`** (stateless hash RNG; `Amounts.short` number formatting) | L1 tests port the Godot formatting cases; 1e3..1e300; no `float` | **done**: `PcFormat` matches 4,402 Godot outputs bit for bit (Godot rounds like legacy msvcrt); `PcRng` frozen against a Python port |
| 7 | **`PcCombat`** (port of `combat_resolver.gd`, comments with their reasoning) | L1 tests; mirror against pawsim's damage formula | **done**: `PcCombat` matches 2,000 rows of Godot's real `CombatResolver` bit for bit (pow within 4 ulps); **plus** `PcStats` (port of `GameState._rebuild_stats`) matches 1,000 random Godot progress states (21,001 fields exact, the rest within 14 ulps of msvcrt `pow`); both negative-tested |
| 8 | **`pawcore_sim` CLI + `tools/check_mirror.py`** (doc 06 §8) | DPS per evolution stage and prices from PawCore == pawsim within 1e-9 relative | **done**: `pawcore_sim.exe` (`Tests/PcSimCli.cpp`, built by `build_pawcore_tests.ps1`) + `tools/check_mirror.py`: Godot's mirror fixture at every stage + 2,000 random states + every price, 20,554 assertions, **every value bit-identical**; `--self-test` catches 7 of 7 data sabotages. Found: pawsim caps the kitten count, the game does not (unreachable in play: the shop stops at the cap) |
| 9 | **Long test B: property tests** (randomised states, millions of cases): damage monotone in every upgrade, no NaN/inf, prices strictly rising, `cost_growth > effect_per_level` lines never overtake | a `--long` doctest suite running 30+ min clean | **done**: `build_pawcore_tests.ps1 -Long` (`PcLongTests.cpp` + the full Godot fixtures), ~150 M random states; `tools/sabotage_pawcore.py` proves 8 of its properties fail when the code breaks. Found: Patience x Attack Speed can lower DPS (by design; counted, reported) |
| 10 | **Long test C: validator fuzz**: thousands of random single-field corruptions of the CSVs; each must be rejected or proven harmless | report of caught/uncaught; every uncaught one explained | **done**: `tools/fuzz_validator.py` (types from `PcDataRows.h`; hostile/wild/benign); 30,000 trials; found 2 validator gaps, now rules `ids.lower_case`, `ref.branch_boss`; the rest explained in the script, and an unexplained class fails it |
| 11 | **`tools/check_all.ps1`** (CLAUDE.md: the one command for every gate): L0 static checks (no `float` in PawCore, no engine include outside the module glue), L1 tests, pawsim parity, mirror | runs green; each gate negative-tested | **done**: `tools/check_all.ps1` (+ `-Full`), `tools/check_pawcore_static.py --self-test`, mirror `--self-test` (11 sabotages), parity `--self-test` |
| 12 | Docs and knowledge base: doc 06, 09, 11, HANDOFF-LOG, new technical lessons into `C:\Games\Knowledge` | handoff written with the exact next step | **done**: HANDOFF-LOG (8), docs 06 §8, 09 §7, 11; KB Godot/03 §6, Godot/07, Unreal/23, INDEX |

Later phase-1 items that need more than one night (`PcStats`, `PcEconomy`, `PcProgress`, `PcCards`, `PcTree`, `PcJobs`,
`PcAchievements`, `Data/CodeConstants.csv`, DataTable import + parity test) are started only if the list above is finished.

**Past the list (started because the list finished):** `PcEconomy` (kill rewards, waves, biome gates, Reawaken, bait, Legacy)
with a 3,000-row Godot fixture (`tools/godot_fixtures/economy_fixture.gd`) and economy properties in the long suite;
`tools/make_godot_fixtures.py` (all Godot ground truth regenerated by one command, byte-identical); the mirror extended to the
economy. Found while scoping `PcCards`: the Second Opinion reroll bug in Godot (HANDOFF-LOG (8)).

