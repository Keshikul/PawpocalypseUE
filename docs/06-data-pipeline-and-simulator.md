# 06 - Data pipeline and the simulator

> KB: `Unreal/05-data-driven-design.md` (CSV as source of truth, DataTables, ids, parity tests), `Unreal/17` (headless Python),
> `Godot/03` §4 (simulators that parse the real data, the mirror gate). Godot: `tools/pawsim.py` (2,762 lines), `pawsim2.py`,
> `tools/shop_step.py`, `ci/check_resources.gd`, `ci/check_mirror.gd`.

## 1. The flow

```
Godot /data/*.tres  --(tools/export_godot_data.py, while both projects exist)-->  Data/*.csv   (SOURCE OF TRUTH in this repo)
                                                                                   |        |
                         tools/import_data.py (headless editor, editor closed)  <--+        +--> tools/pawsim.py (re-pointed)
                                   |                                                         +--> PawCore CLI (pawcore_sim.exe)
                         Content/Pawpocalypse/Data/DT_*.uasset (committed, LFS)                      |
                                   |                                                         mirror test: same fixture, diff outputs
                         FPawDataAdapter -> PcData (+ the shared validator) at startup
```

- **CSV is edited, DataTables are generated.** Never edit a `DT_*` in the editor. A parity test fails if any DataTable differs
  from its CSV ("edited the CSV, forgot the import") `[KB U/05 §2]`.
- Loose CSVs are not packaged: the game reads the DataTables; only tools read the CSVs.

## 2. The exporter (`tools/export_godot_data.py`)

Already run on 2026-10-28: **266 resources in 11 classes** (41 achievements, 5 bosses, 28 cards, 1 combat constants, 59 enemies,
7 evolution stages, 10 jobs, 1 Reawaken constants, 87 skill nodes, 12 biomes, 15 shop lines), every enum written by NAME, 0
unknown enum values. Re-run it whenever the Godot `/data` changes while the Godot game is still the reference:

```
py tools/export_godot_data.py --godot C:/Games/pawpocalypse --out Data --catalogue docs/appendix-a-content-catalogue.md
git diff Data/        # every change is a design change made in Godot: review it like one
```

Conventions it writes (keep them for hand edits after the Godot project is retired):
- first column `id`, lower-case; `_godot_file` last (provenance; drop the column when Godot is retired);
- lists joined with `;`, dictionaries as compact JSON (`extra_effects`), colours as `r;g;b;a`;
- a reference to another resource is that resource's `id` (`spawn_pool`, `golden_enemy`, `boss_data`);
- asset paths are still Godot `res://assets/art/...svg`: the importer maps them to visual-definition ids (doc 07 §4).

**After Godot is retired** the CSVs are edited directly; the exporter is deleted.

## 3. Enums: by NAME, append-only meaning

Godot serialises enums by index, which is why the Godot rules say "append only, never reorder". This project stores enum
**names** in CSVs and saves (`effect_type = DAMAGE_ADDITIVE`), and C++ converts with a table, so reordering is no longer fatal.
Keep the habit anyway: **never rename an enum value** (the name is now the stored key), keep retired values in the C++ enum
marked `Retired`, and a test fails on any CSV/save value that does not parse.

## 4. DataTables and row structs

| CSV | DataTable | Row struct | Notes |
|---|---|---|---|
| `TierData.csv` | `DT_Tiers` | `FPawTierRow` | `spawn_pool`/`spawn_weights` parallel lists; `boss_data` id |
| `EnemyData.csv` | `DT_Enemies` | `FPawEnemyRow` | `movement` enum by name |
| `BossData.csv` | `DT_Bosses` | `FPawBossRow` | phase lists |
| `UpgradeData.csv` | `DT_Upgrades` | `FPawUpgradeRow` | `effect_type`, `target_stat` enums |
| `SkillBranchData.csv` | `DT_SkillNodes` | `FPawSkillNodeRow` | `level_costs`, `level_effects` lists; `extra_effects` JSON (parse once at load into named channels: `mult`, ...) |
| `CardData.csv` | `DT_Cards` | `FPawCardRow` | `{v}` placeholders in descriptions become FText named args (doc 08 §4) |
| `JobData.csv` | `DT_Jobs` | `FPawJobRow` | |
| `EvolutionData.csv` | `DT_Evolution` | `FPawEvolutionRow` | |
| `AchievementData.csv` | `DT_Achievements` | `FPawAchievementRow` | |
| `CombatConstantsData.csv`, `ReawakenConstantsData.csv` | `DT_Constants` (one row each) or a `UPawConstants` data asset | | single-row tables |
| `CodeConstants.csv` (new) | `DT_CodeConstants` | `FPawConstantRow {id, value, reason}` | the gameplay constants from appendix C (§5) |
| `AudioEvents.csv` (new) | `DT_AudioEvents` | doc 07 §6 | |

- Player-facing columns (`display_name`, `description`, `requirement_text`) are NOT read as text by the game: they are the
  English source for string tables keyed by id (`ITM_card_bloodlust_name`), doc 08 §4. The CSV keeps them for review.
- **Row names = ids**; lookups lower-case through one `IdFromName()`; a test loads every table in a cooked build and finds every
  row by id (the FName case trap shows only in packaged builds) `[KB U/05 §3]`.
- Every type is registered with the Asset Manager or lives under the always-cooked root `/Game/Pawpocalypse` (doc 10 §1).

## 5. Constants that live in code (appendix C)

The Godot game keeps **456 numeric/colour constants in scripts**, not in `/data`
([appendix C](appendix-c-code-constants.md), regenerated by `tools/list_godot_code_constants.py`). Some are gameplay rules
(`LEVEL_COST_BASE` 24, `LEVEL_COST_GROWTH` 1.68, `BASE_HAND_SIZE` 3, `MAX_WAVE` 64, `MAX_LIVE_ENEMIES` 320,
`MAX_GOLDEN_CHANCE` 0.35, `CROWD_THRESHOLD` 40, `LEVY_EVERY` 100, `INFESTATION_*`, `FENCE_MAX_KILLS` ...); most are presentation
(sizes, colours, effect timings).

- **Gameplay** constants go into `Data/CodeConstants.csv` (id, value, reason) when the system that uses them is ported. pawsim
  mirrors some of them by hand today (e.g. `CATCHES_BASE`/`GROWTH`): after the move it reads the CSV instead, which removes a
  drift risk the Godot code comments warn about.
- **Presentation** constants go into the C++ class that owns them, with the Godot "why" comment.
- A test cross-checks: every appendix C gameplay constant has a CSV row or an explicit "presentation" marking in a reviewed list
  (`Data/CodeConstantsMap.csv`: `godot_name, destination, note`). Nothing is silently dropped.

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
- **pawsim cannot price Legacy** (its utility is a rate) and bundles the Unicorn with its best child; `--autotune` fits noise.
  These limits carry over.
- **A rule that changes what the player can reach is taught to pawsim in the same commit** (Godot rule, kept).
- Shop re-stepping stays `py tools/shop_step.py --step N --write`, now writing CSV rows instead of 15 `.tres` files.

## 8. The mirror test (Godot `check_mirror`, now stronger)

PawCore builds a CLI (`pawcore_sim --fixture <name> --report json`) that runs the same fixtures pawsim runs (DPS at each
evolution stage with a fixed shop/tree/card state, prices, Reawaken grants, gate openings), and `tools/check_mirror.py` diffs
the two reports with a relative tolerance of 1e-9. In Godot the mirror compared the game with the sim on one fixture; here
three implementations meet: pawsim, PawCore, and the game (an automation test feeds PawCore's report through the game's
adapter path to prove the DataTable route gives the same numbers as the CSV route).

## 9. Headless import

- `tools/import_data.py` runs `UnrealEditor-Cmd.exe Pawpocalypse.uproject -run=pythonscript -script=C:/.../import_data.py
  -nullrhi -unattended -nop4 -SCCProvider=None -stdout` (absolute, forward slashes; `unreal.log_warning` for output; editor
  CLOSED) `[KB U/17]`. It creates missing DataTables with the right row struct, fills them from CSV
  (`unreal.DataTableFunctionLibrary.fill_from_csv_file`), saves them explicitly, and prints `PAW_IMPORT ok <table> <rows>`.
- Run from PowerShell (Git Bash rewrites `/Game/...` paths unless `MSYS_NO_PATHCONV=1`) `[KB U/17 §3]`.
- The parity test then compares DataTable rows with CSV rows field by field.
