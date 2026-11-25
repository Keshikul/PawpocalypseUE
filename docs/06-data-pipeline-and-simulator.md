# 06 - Data pipeline and the simulator

> KB: `Unreal/05-data-driven-design.md` (CSV as source of truth, DataTables, ids, parity tests), `Unreal/17` (headless Python),
> `Godot/03` §4 (simulators that parse the real data, the mirror gate). **Since D22 (2026-11-23) `Data/*.csv` is this game's
> own and edited here**; the exporter from the Godot project and every tool that read it were deleted 2026-11-24 (HANDOFF-LOG
> (145)). Sections below that describe them are kept as the record of how the data came to be.

## 1. The flow

```
Data/*.csv   (SOURCE OF TRUTH, edited by hand; exported once from the first try's /data, 2026-10-28)
     |        |
     |        +--> tools/pawsim.py, tools/econ_review.py (the balance numbers)
     |        +--> PawCore CLI (pawcore_sim.exe) -- mirror test: same fixture, diff outputs
tools/import_data.py (headless editor, editor closed)
     |
Content/Pawpocalypse/Data/DT_*.uasset (committed, LFS)
     |
FPawDataAdapter -> PcData (+ the shared validator) at startup
```

- **CSV is edited, DataTables are generated.** Never edit a `DT_*` in the editor. A parity test fails if any DataTable differs
  from its CSV ("edited the CSV, forgot the import") `[KB U/05 §2]`.
- Loose CSVs are not packaged: the game reads the DataTables; only tools read the CSVs.

## 2. Editing `Data/` (and the exporter it came from)

**Today**: edit the CSV, run `py tools/import_data.py --run` (editor closed), and a balance change is measured with
`tools/econ_review.py` first and made on the designer's yes (CLAUDE.md). A new column also needs `tools/gen_paw_rows.py`
and its field in `PcDataRows.h` (edited by hand since D22).

**History.** The exporter (`tools/export_godot_data.py`, deleted 2026-11-24) ran on 2026-10-28: **266 resources in 11 classes** (41 achievements, 5 bosses, 28 cards, 1 combat constants, 59 enemies,
7 evolution stages, 10 jobs, 1 Reawaken constants, 87 skill nodes, 12 biomes, 15 shop lines), every enum written by NAME, 0
unknown enum values (since F2, 2026-11-03: 264 resources, 39 achievements).

Conventions it wrote, kept for hand edits:
- first column `id`, lower-case; `_godot_file` last (provenance; it also orders the deck and the achievement roster, see §4,
  so it stays until that order has another home);
- lists joined with `;`, dictionaries as compact JSON (`extra_effects`), colours as `r;g;b;a`;
- a reference to another resource is that resource's `id` (`spawn_pool`, `golden_enemy`, `boss_data`);
- asset paths are still Godot `res://assets/art/...svg`: the importer maps them to visual-definition ids (doc 07 §4).

From D16 to D22 (2026-11-12 to 2026-11-23) a number changed here was a text replacement in `tools/godot_fixes.py`, applied
by the exporter, so `Data/` still equalled a fresh export; D22 retired that (the CSVs are edited directly) and the file was
deleted 2026-11-24.

**`Data/InputMap.csv`** (2026-11-04, exported from the first try's `project.godot [input]`; edited by hand since D22): one
row per event, the first try's codes beside the Unreal keys (doc 05 §3). It is not a DataTable: `tools/create_input_assets.py`
makes the Enhanced Input assets from it, `tools/gen_input_defaults.py` the remapping's defaults.

## 3. Enums: by NAME, append-only meaning

Godot serialises enums by index, which is why the Godot rules say "append only, never reorder". This project stores enum
**names** in CSVs and saves (`effect_type = DAMAGE_ADDITIVE`), and C++ converts with a table, so reordering is no longer fatal.
Keep the habit anyway: **never rename an enum value** (the name is now the stored key), keep retired values in the C++ enum
marked `Retired`, and a test fails on any CSV/save value that does not parse.

## 4. DataTables and row structs

**As built (2026-10-29):** one DataTable per CSV, `/Game/Pawpocalypse/Data/DT_<CsvName>` (`DT_TierData`, `DT_EnemyData`, ...,
`DT_CodeConstants`), row struct `FPaw<Name>Row` (`FPawTierRow`, `FPawSkillBranchRow`, `FPawCombatConstantsRow`,
`FPawCodeConstantsRow`, ...), all GENERATED into `Source/Pawpocalypse/Data/PawDataRows.h` by `tools/gen_paw_rows.py` from the
CSV headers (L0 checks it is current), with the list `PawDataTables()` (CSV name, asset path, row struct).

- **Every column is an `FString` holding the cell's text.** The importer stores an FString cell verbatim
  (`FStrProperty::ImportText` without `PPF_Delimited`, read in the 5.8.3 source), so a table holds exactly its CSV's text, and
  the game parses it with PawCore's one loader: `FPawDataAdapter` (`Data/PawDataAdapter.h`) rebuilds each table as a PawCore
  `CsvTable` and calls `Pc::BuildGameData`, then the shared validator. One parser for the tests, pawsim's mirror, `pawcore_sim`
  and the game: the DataTable route cannot read a number differently from the CSV route. The typed view is `PcGameData`; nobody
  reads the tables directly (and nobody edits them: `VisibleAnywhere`).
- Property names ARE the CSV column names (snake_case on purpose: the importer matches header to property). `_godot_file` is the
  property `godot_file` (the importer renames the header, the adapter maps it back): it carries the deck's and the achievement
  roster's order (`SortByGodotFile`).
- The adapter lower-cases row names and property names before using them: both are FNames, and FName case is not stable
  across builds `[KB U/05 §3]`.
- Player-facing columns (`display_name`, `description`, `requirement_text`) are NOT read as text by the game: they are the
  English source for string tables keyed by id (`ITM_card_bloodlust_name`), doc 08 §4. The CSV keeps them for review.
- `AudioEvents.csv` / `AudioMusic.csv` (phase 6 slice 4; exported from the first try's sound table, edited by hand since
  D22), NOT DataTables (`NOT_TABLES`): `tools/import_audio.py` builds `DA_AudioBank` from them (doc 07 §6).
- A COOKED build reads every table: the packaged soak (`tools/packaged_soak.py`) plays a whole game on them, and
  `packaged_languages` shows every data name; there is no separate find-every-row-by-id gate.
- Every type is registered with the Asset Manager or lives under the always-cooked root `/Game/Pawpocalypse` (doc 10 §1).

## 5. Constants that lived in the first try's code

The Godot game kept **580 numeric/colour constants and numeric `@export` defaults in scripts** (569 on 2026-10-29; the
save fixes F26-F33 added 11), not in `/data` (listed then in appendix C, deleted with its generator 2026-11-24). Some are gameplay rules
(`LEVEL_COST_BASE` 24, `LEVEL_COST_GROWTH` 1.68, `BASE_HAND_SIZE` 3, `MAX_WAVE` 64, `MAX_LIVE_ENEMIES` 320,
`MAX_GOLDEN_CHANCE` 0.35, `CROWD_THRESHOLD` 40, `LEVY_EVERY` 100, `INFESTATION_*`, `FENCE_MAX_KILLS` ...); most are presentation
(sizes, colours, effect timings).

**As built (2026-10-29):**
- `Data/TextOverrides.csv` (2026-11-09; `table, key, text, godot, note`) is source too, never exported: the port's own
  English for 279 player-facing lines (doc 08 §4, `tools/text_overrides.py`). The export comparison, `gen_paw_rows.py` and
  `import_data.py` skip it.
- `Data/CodeConstantsMap.csv` (`godot_name, destination, literal, note`) is the REVIEWED list: one row per appendix C constant
  (585 rows on 2026-11-05: the 580, plus `BOSS_REALMS`, a list, and four literals: Second Helping's biome index in `begin_biome()`, and
  since 2026-10-31 the boss's relocation distance and drift wobble (x and y) in `Boss._tick_movement()`). The
  `@export` defaults joined on 2026-10-29 (enemy and kitten speeds live there); the exporter refuses one a scene overrides.
  Destinations (585): `code_constants:<id>` (66: GameState, CardManager, and the field's rules in TierController, Boss,
  Kitten, Enemy, Player), `presentation` (313), `data` (89: data classes' field defaults, generated into `PcDataRows.h`),
  `palette` (66), `input` (21), `save` (25), `dev` (3), `audio` (2). Borderline calls carry a note (e.g. `ANVIL_ECHO_REACH`
  is the drawn swipe's reach, `MAX_SPLASH_BEAMS_PER_FRAME` caps beams drawn, not the spill).
- `Data/CodeConstants.csv` (`id, value, reason, godot_name`) was generated from the map and the Godot code until D22
  (`tools/export_code_constants.py`, deleted 2026-11-24); since then it is edited by hand like every CSV. Values in shortest
  round-trip form (`1.0 / 60.0` -> `0.016666666666666666`). The map stays as the record of where each constant went.
- PawCore reads it into `PcGameData::Code` (`PcCodeConstants`, `PcConstants.h`): every field needs exactly one row and every row
  a field, integers must be integers (tested, and the loader's checks are sabotaged). Functions that read a constant take the
  data (`EvolutionStage(D, P)`, `CatchesForLevel(D, Level)`, `CurrentWave(D, Tier, Kills)`...). The validator checks them
  (`code.stage_ladder`: stage count = bosses + 2, the mount within it; `code.range`: positive, golden < 1, XP growth > 1,
  epsilon a rounding allowance, Last Stand under Swarm Logic, Second Helping's biome exists).
- pawsim reads `max_golden_chance` and the Catches curve (`card_level_cost_*`, unless `--catches-*` sweeps them) from the
  same file (`--data csv`, the only source since 2026-11-24).
- **Presentation** constants go into the C++ class that owns them, with the Godot "why" comment (phases 2 and 6).

## 6. Economy invariants (Godot `check_resources`, ported into the shared validator)

The validator runs in PawCore (so the game, the CLI and the tests share it) and in pawsim's loader. It asserts, among others:

- every uncapped multiplicative shop line: `cost_growth > effect_per_level`; summed return exponent < 0.80;
- `reward_catnip = 0.0433 x hp` within tolerance for every enemy except tier 1 and the flier premium (x 1/0.45);
- spawn rate strictly climbing from the Alley on; no biome paying less per kill than the one before; `reward_multiplier` bounded;
- `required_evolution_stage` blocks (1-3:1, 4-6:2, 7-8:3, 9-11:4, 12:5); `BOSS_REALMS` = the five boss biomes;
- evolution ceiling `base ^ 6 = 128` (+/- tolerance) and weapon multipliers 1.00 -> 1.40;
- card deck shape: 28 cards, 7 flavours x 4, exactly one cost per flavour; the three effect ids with no card listed explicitly;
- every skill node with a parent has `parent_is_required = true`; parents exist; no cycles; job nodes name a job;
- achievements' Stardust rewards within 1-12; every metric known;
- no NaN/inf anywhere; lists the right lengths (levels vs costs vs effects).

**Every assertion is negative-tested once** (break the data on purpose, watch the validator fail, confirm the sabotage landed)
before it is trusted (the Godot rule: "a gate that has never failed has never been tested").

## 7. pawsim

- **Keep pawsim.py.** It is 2,762 lines of measured economy knowledge and the designer's length/pacing reports come from it. Phase
  1 re-points its loader from `.tres` parsing to `Data/*.csv` (one function, `load_*`), with no rule changes, and proves the
  numbers did not move: run the Godot-parsing pawsim and the CSV-parsing pawsim side by side; the full-game report must be
  byte-identical.
- **Done 2026-10-28**: `tools/pawsim.py` reads `Data/*.csv`; a parity sweep (`check_pawsim_parity.py`, deleted 2026-11-24
  with the `.tres` loader's source) proved the CSV pawsim and the `.tres` pawsim byte-identical over 90 settings.
- **pawsim cannot price Legacy** (its utility is a rate) and bundles the Unicorn with its best child; `--autotune` fits noise.
  These limits carry over.
- **A rule that changes what the player can reach is taught to pawsim in the same commit** (Godot rule, kept).
- Shop re-stepping stays `py tools/shop_step.py --step N --write`, now writing CSV rows instead of 15 `.tres` files.
- **D17 (2026-11-12) taught pawsim** E1 (a Reawaken keeps passage), E2 (the Bait bank, `bait_kills_by_biome`), E3
  (`reawaken_catnip_multiplier()`, in `catnip_per_sec`) and the Unicorn's x1.5 (`_damage_scale`); the mirror compares the
  Catnip multiplier too, and its self-test sabotages `reawaken_catnip_exponent`. `econ_review.py --proposal before_d17`
  puts the old rules and numbers back in memory and reproduces the review's "today" exactly (406 min).
- **pawsim has no waves** (found 2026-11-12): the game escalates a biome against its own kill bank (HP x1.35, reward x1.28 a
  wave, `TierData.kills_per_wave`...), which Godot added after pawsim was copied. Rule 14 says pawsim should learn it.
  `tools/econ_review.py` (the economy review, `docs/ECONOMY-REVIEW-2026-11-12.md`) adds waves on top of pawsim exactly as
  PawCore derives them, logs every shop buy, tree level and Reawaken, and applies proposals IN MEMORY (`--proposal`, `Data/`
  never written). With waves the game before D17 was 406 min against pawsim's 403; D17's is 112 min against pawsim's 163
  (without waves); D24's (2026-11-24) 175 min. **Compare the game with econ_review, never with plain pawsim**: the 5x
  soak matched econ_review's 113 min to a tenth (2026-11-24), while pawsim without waves said 164. It is a measuring
  tool, not a gate.

## 8. The mirror test (Godot `check_mirror`, now stronger)

PawCore builds a CLI (as built: `pawcore_sim --data Data --fixtures FILE [--out FILE]`, or `--validate`) that runs the same fixtures pawsim runs (DPS at each
evolution stage with a fixed shop/tree/card state, prices, Reawaken grants, gate openings), and `tools/check_mirror.py` diffs
the two reports with a relative tolerance of 1e-9. In Godot the mirror compared the game with the sim on one fixture; here
three implementations meet: pawsim, PawCore, and the game (an automation test feeds PawCore's report through the game's
adapter path to prove the DataTable route gives the same numbers as the CSV route).

**Built 2026-10-28** (the PawCore half; the game's adapter half since 2026-10-29: `Pawpocalypse.Data.Parity`, doc 09 §7, and
`UPawGameSubsystem` loads the game's data through it at startup since 2026-10-31): `pawcore_sim.exe` (source
`Source/PawCore/Tests/PcSimCli.cpp`, built next to the tests by `tools/build_pawcore_tests.ps1`, `-SimOnly` for just it) reads a
fixture file of upgrade and branch LEVELS and prints exact hex floats; `tools/check_mirror.py` builds the same states in pawsim
(`load_game_data_csv`) and compares. Fixtures: Godot's own mirror fixture (`ci/check_mirror.gd`) at every boss count, 2,000
seeded random states over the lines pawsim models (damage, kittens, income, the Stardust grant and the Reawaken gate, Legacy,
Fortune, golden chances), every upgrade price at levels 0..80, every branch price. Measured: every value bit-identical (58,651
assertions, 2026-10-29). `--self-test` runs it against 11 sabotaged data copies and requires each to be caught.
`--validate` turns the same exe into the validator CLI (`tools/fuzz_validator.py`).

Known divergence, recorded rather than hidden: the game's `get_kitten_count()` reads the UNCAPPED `_effect_total`, pawsim's
`kitten_count()` clamps to the line's cap. The shop never sells past the cap, so they agree on every reachable state; the random
states keep the kitten line inside its cap (`tools/check_mirror.py`, with the measurement).

Below pawsim are the **recorded fixtures** (`Source/PawCore/Tests/Data/*_godot.tsv`, and the long suite's full ones in
`build/*_godot_full.tsv`, 158 MB, untracked: keep them): rows recorded from the first try's real code (`Amounts.short`,
`CombatResolver`, `GameState`, `CardManager`, `SkillTreeManager`, the presentation scripts) by `make_godot_fixtures.py`,
deleted 2026-11-24. Since D22 they are this game's expected behaviour: a rule change edits the code, its test and the rows
it changes, by hand. A test that proves a RULE holds the numbers its fixture was recorded with, so a balance change does
not fail it (`PcBossTests.cpp` holds the boss HP of 2026-11-12 since D24). pawsim does not model the cards or the tree's
buy rules, so for those the fixtures are the only oracle.

## 9. Headless import

- `py tools/import_data.py` launches `UnrealEditor-Cmd.exe Pawpocalypse.uproject -run=pythonscript -script=<itself>
  -nullrhi -unattended -nop4 -SCCProvider=None -stdout` (absolute, forward slashes; `unreal.log_warning` for output)
  `[KB U/17]`. It creates missing DataTables with the right row struct, fills them from the CSV text
  (`fill_data_table_from_csv_string`, `_godot_file` renamed `godot_file`), saves them explicitly, prints
  `PAW_IMPORT ok <table> <rows>`, then checks parity on the saved assets (`PAW_PARITY ok <table>`). Any `LogDataTable` warning
  fails the run. (It refuses while an Unreal process of THIS project runs, or one naming no project: `tools/unreal_procs.py`,
  the guard every engine tool shares; another project's editor is ignored.) `--check` = parity only (nothing written); `--self-test` = a scratch copy with one changed cell must fail.
- Run from PowerShell (Git Bash rewrites `/Game/...` paths unless `MSYS_NO_PATHCONV=1`) `[KB U/17 §3]`.
- The parity test then compares DataTable rows with CSV rows field by field.
