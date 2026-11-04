# Pawpocalypse (Unreal) - START HERE

> Written **2026-10-28** for the port of Pawpocalypse from Godot 4.7.1 (`C:\Games\pawpocalypse`, itch 0.2.5) to
> **Unreal Engine 5.8.3** (`C:\Games\pawpocalypseUE\Pawpocalypse`, a blank C++ project created the same day).
> Every claim about the Godot game was read from its code, `/data`, or its own docs on that date. Every claim about
> Unreal is tagged with where it came from: `[KB]` the knowledge base at `C:\Games\Knowledge` (verified on UE 5.8.3 in
> a real project unless it says otherwise), `[Docs]` Epic documentation, `[Forum]` forum/community reports, `[Plan]`
> a decision this handoff makes that must still be proved by a spike.

## What this is

A **recreation of the same game**, not a sequel and not a redesign. The design, the numbers, the art and the audio
carry over; the engine, the code and the build pipeline are new. The Godot project stays the reference build and the
shipping product until the Unreal build reaches parity (doc 11, phase 9).

## Reading order

| # | Doc | Read it when |
|---|---|---|
| 00 | this page | first |
| 01 | [01-port-brief-and-decisions.md](01-port-brief-and-decisions.md) | first: scope, the decisions already taken, the ones waiting on the designer |
| 02 | [02-game-design-handoff.md](02-game-design-handoff.md) | before any gameplay code: the whole game, every rule, every invariant |
| 03 | [03-technical-architecture.md](03-technical-architecture.md) | before any C++: modules, classes, the sim/presentation split, ticking |
| 04 | [04-2d-rendering-in-unreal.md](04-2d-rendering-in-unreal.md) | before drawing anything: the research on 2D in Unreal, the traps, the rendering plan and its spikes |
| 05 | [05-ui-input-controller.md](05-ui-input-controller.md) | before any widget or input action |
| 06 | [06-data-pipeline-and-simulator.md](06-data-pipeline-and-simulator.md) | before touching `Data/` or a balance number |
| 07 | [07-art-audio-pipeline.md](07-art-audio-pipeline.md) | before importing a texture or a sound |
| 08 | [08-saves-settings-localization.md](08-saves-settings-localization.md) | before saves, options or any player-facing text |
| 09 | [09-testing-and-gates.md](09-testing-and-gates.md) | before claiming anything works |
| 10 | [10-build-release-steam.md](10-build-release-steam.md) | before packaging |
| 11 | [11-roadmap.md](11-roadmap.md) | to know what to do next, with exit criteria and session prompts |
| 12 | [12-pitfalls.md](12-pitfalls.md) | when something breaks, and before every phase |
| 13 | [13-research-sources.md](13-research-sources.md) | to check where a claim came from |
| A | [appendix-a-content-catalogue.md](appendix-a-content-catalogue.md) | generated: every biome, boss, enemy, card, node, job, achievement |
| B | [appendix-b-godot-to-unreal-map.md](appendix-b-godot-to-unreal-map.md) | generated: all 79 Godot scripts (29,258 lines), where each goes and in which phase |
| C | [appendix-c-code-constants.md](appendix-c-code-constants.md) | generated: the 569 numeric/colour constants and numeric `@export` defaults that live in Godot CODE, not `/data`, with their reasons |

Project rules for agents: [../CLAUDE.md](../CLAUDE.md). Engine knowledge: `C:\Games\Knowledge\INDEX.md` (read it, then
only the documents it points to; the **`2D/` folder** (`2D/01-2d-optimization-best-practices.md`,
`2D/02-unreal-2d.md`) was written from this handoff's research).

## How we proceed (the short version)

1. **Phase 0, setup and four spikes (about 3-4 sessions).** Fix the template's project settings (Lumen, ray tracing,
   Substrate, Nanite off; an empty startup map), add git and LFS, set up the module layout. Then run four small,
   throwaway experiments that settle the decisions the research could not: how sprites are drawn (and whether
   colours stay exact), whether 1,500 moving sprites hold 60 fps on a Steam Deck-class budget, the SVG to texture
   pipeline, and the dock plus skill tree with Common UI on a pad. **Nothing in phase 1 starts before the spike
   results are written into doc 04 and doc 05.**
2. **Phase 1, the rules without pictures.** The CSVs in `Data/` become DataTables; a pure C++ library holds every
   formula; `pawsim.py` reads the same CSVs; a mirror test proves the two agree. The economy is the game's spine and
   is ported, and proved, first.
3. **Phases 2-8**: the realm, progression, saves and menus, pad parity, presentation, localisation, gates and
   packaging (doc 11 has each with its exit criteria).
4. **Phase 9, parity**: side-by-side with the Godot build, a soak of the whole game, the designer's playtest. Only
   then does Unreal replace Godot as the shipping build.

The full plan with session prompts is [11-roadmap.md](11-roadmap.md).

## Where the port is (2026-11-03)

- **Phase 0 done**: settings, modules, maps, spikes S1-S4 all recorded (doc 04 §10, doc 05); spike code kept until a commit
  preserves it in history, then deleted.
- **Phase 1 done**: the rules were ported into `PawCore` one Godot manager at a time, each proved against the REAL
  Godot code on thousands of random states (doc 09 §7). Ported: data + validator, `PcFormat`, `PcRng`, `PcCombat`, `PcStats`,
  `PcEconomy`, `PcCards` + `PcCardDraw`, `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`: every Godot manager's
  rules; `Data/CodeConstants.csv` (the gameplay constants of Godot code); the 12 DataTables (`tools/import_data.py`) and
  the game's data adapter, proven equal to the CSVs. Its exit gate, the nightly `-Full` ran green 2026-10-30 (HANDOFF-LOG (17)).
- **Phase 2: all 7 slices done** (HANDOFF-LOG (16), (18), (19)): `PcField` simulates the whole realm (mice, kittens, the
  aura, Snare and zones, Pounce, the paw's and the kill's extras, the Prism, the boss and its adds), checked against the real
  Godot code, rule tests, a soak and 272 PawCore sabotages (445 across PawCore today); and the game runs it: L_Realm opens on a live field (`APawRealmView`,
  `UPawRealmSubsystem` over `FPawRealmSim`, `UPawSpriteRenderer`; placeholder discs until phase 6), with the debug HUD and the
  cheats (`paw.Cheat.*`). **Phase 2 closed 2026-11-01** (HANDOFF-LOG (20)): every card effect observed in a live realm
  (`Pawpocalypse.Realm.CardEffects`, `.EveryCard`) and the packaged-build gate (`py tools/packaged_bench.py`: 320 mice in a
  packaged Development build, game thread 2.7 ms p99 against 4 ms, 14 draw calls, the cooked L_Realm drawing).
- **Phase 3 started 2026-11-01** (HANDOFF-LOG (21)): the HUD and the dock's Upgrades page are live (`Source/Pawpocalypse/UI`,
  `PcHud` in PawCore, the string tables `ST_UI` / `ST_Data`, `PawTokens.h`); the Realms and Reawaken pages since (22); the
  Catches, Achievements and Jobs pages since (23), every dock page built; the realm earns achievements; after a Reawaken the
  run opens in Territory's biome (designer 2026-11-01, "Territory wins", F20, fixed in Godot first). The card pick screen since
  (24): a Menu-stack screen that holds the field while a hand is up; the skill tree's layout and navigation in PawCore
  (`PcTreeLayout`, exact against Godot); the skill tree screen since (25): browse (T / pad Y) and the Reawaken's spend step, every
  word generated from skill_tree_ui.gd (`tools/gen_tree_tables.py`). The Reawaken ceremony and the job picker since (26), so a
  Reawaken runs Godot's whole order: ceremony, spend step, job picker, travel (doc 05 §6.3). The victory screen since (27), and
  numbers printed Godot's way (ungrouped, culture-invariant). The screenshot tour against Godot since (28) (`py tools/godot_tour.py`).
  The whole run through the UI since (29) (`Pawpocalypse.UI.Playthrough`): **phase 3's exit criteria are met** but the
  packaged-build gate, which waits for the designer to ask for a package. The nightly `-Full` since (30). 
- **Phase 4 started 2026-11-03** (HANDOFF-LOG (31)): F21 and F22 decided by the designer and fixed in Godot first, the port followed;
  the save in PawCore (`PcSave`, `PcJson`: exact against the real save_manager.gd, fixture `save`) and its container in the game
  (`Source/Pawpocalypse/Save`: `UPawSaveSubsystem`, five slots, the atomic write and recovery, autosave, the sandbox: `-NoSave`,
  `-PawSaveDir=`). Next: the title screen (doc 11 phase 4).
- **Nothing is committed since `7410909`**: the designer reviews and commits.
- **One gate command**: `powershell -ExecutionPolicy Bypass -File tools\check_all.ps1` (everyday, minutes) or `-Full`
  (nightly, ~5.5 h; last run 2026-11-03: 20 of 21 green, the PawCore sabotage gate failing on two stale mutants, both retargeted and seen CAUGHT, HANDOFF-LOG (33)). Every gate has been seen to fail (doc 09 §7).
- Findings in the Godot game and open designer questions: doc 01 §3 and §5 (F13-F18 new 2026-10-31, F19 and F20 2026-11-01, F21 and F22 2026-11-02 (the job picker's look; the Realms page ignores passage); F13, F16
  and F19 were bugs a player can see; **every finding with a fix was fixed 2026-11-03** (designer: "all open findings", Godot first,
  uncommitted there; HANDOFF-LOG (32)); open: F3, F4, F5, F12, F15 (no fix to apply), F25 (Godot's .tres precision)); the designer's copy of the findings is
  `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md` (kept in step with §5).

## What already exists in this project

| Path | What |
|---|---|
| `Pawpocalypse.uproject`, `Config/` | UE 5.8.3-58210709 project (created 2026-10-28); phase 0 settings applied (doc 03 §7): no Lumen/RT/Nanite/Substrate, Common UI viewport, unused plugins off for the game (`tools/trim_game_plugins.py`) |
| `Source/PawCore/` | pure C++17 rules, no engine headers except `Private/PawCoreModule.cpp`. `Public/`: `PcCore`, `PcCsv`, `PcFieldRules`, `PcField` (phase 2: the realm simulation), `PcCheats` (the cheat overlay's rules, dev only), `PcData` (+ generated `PcDataRows.h`, `PcEnums.h`), `PcValidate` (57 rules), `PcFormat`, `PcRng`, `PcConstants` (the code constants, loaded from CSV), `PcCombat`, `PcStats` (also holds `PcProgress`, the state the rules read), `PcEconomy`, `PcCards`, `PcCardDraw`, `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`, `PcTreeLayout` (the skill tree's shape and pad navigation), `PcTreeText` (its words; generated `PcTreeWording.h`). `PcSave` (the save: Godot's PawpocalypseSaveData, save / load rules, the file), `PcJson` (its JSON). `Tests/`: doctest L1 + long suites, `PcSimCli.cpp` (the mirror CLI `pawcore_sim`), `Tests/Data/*_godot.tsv` (Godot ground truth) |
| `Source/Pawpocalypse/` | `Save/` (phase 4: `UPawSaveSubsystem` over `IPawSaveStorage`: platform / folder / memory; `Pawpocalypse.Save.*`); `UI/` (phase 3: `UPawRootLayout`, `UPawHudScreen`, `UPawCardPickScreen` (the Menu stack's first screen), `UPawSkillTreeScreen` (+ generated `PawTreeTables.h`), `UPawReawakenCeremony` (the HUD's top layer), `UPawJobPicker` (the Modal stack's first screen), `UPawVictoryScreen` (the ending, Modal stack, the game theme since F21), `UPawButton`, `UPawPounceClock`, `PawText` = the string tables, `PawUiKit`, generated `PawTokens.h`, `PawUiConstants.h`; the `Pawpocalypse.UI.*` tests); `Core/` (thin GameInstance, GameMode, PlayerController (creates the root layout and the HUD), `LogPaw`, `UPawGameSubsystem`: the data and the progress); `Data/` (`PawDataRows.h`, generated row structs; `FPawDataAdapter`, DataTables -> PawCore; the `Pawpocalypse.Data.*` automation tests); `Realm/` (`FPawRealmSim`: the fixed step and the glue; `UPawRealmSubsystem` (also `ChooseCard` / `RerollHand` / `BanishFromHand`, the card pick's entries, and the pause holders); `APawRealmView`: camera, input, effects, drawing; the `Pawpocalypse.Realm.*` tests, the card effects in `PawCardEffectsTests.cpp`); `Render/` (`UPawSpriteRenderer`, instanced quads); `Dev/` (`APawDebugHud`, the `paw.Cheat.*` console commands, `paw.Bench`: the packaged frame gate); `Dev/Spikes/` = **temporary** phase 0 spike code |
| `Source/PawpocalypseEditor/` | the editor module (empty until the importers, phase 1) |
| `Content/Pawpocalypse/` | `Maps/L_Title` (empty), `Maps/L_Realm` (one configured `APawRealmView`; both by `tools/create_maps.py`); `Materials/M_Field*` (the renderer's four generated materials, `tools/create_field_materials.py`); `Data/DT_*` (12 DataTables, generated by `tools/import_data.py`, never edited); `Localization/Strings/ST_UI.csv` (hand-written) and `ST_Data.csv` (generated), staged loose; `Dev/Spikes/` = temporary spike assets |
| `Data/*.csv` | **every gameplay resource of the Godot game** (266 resources, 11 classes), exported by `tools/export_godot_data.py`, enums written by NAME |
| `Data/CodeConstantsMap.csv`, `Data/CodeConstants.csv` | the reviewed destination of every Godot code constant (574 rows); the 66 gameplay ones it selects, exported with their values and reasons from the Godot code (`tools/export_code_constants.py`, run by the exporter) |
| `tools/export_godot_data.py`, `gen_pc_data.py`, `gen_pc_enums.py` | the exporter (doc 06 §2) and the generators of `PcDataRows.h` / `PcEnums.h` from the Godot data scripts (L0 checks they are current) |
| `tools/list_godot_code_constants.py`, `tools/make_port_map.py` | regenerate appendices C and B from the Godot repo (the map script fails if a Godot script is unmapped) |
| `tools/pawsim.py` | the balance simulator, reading `Data/*.csv` (byte-identical to the Godot pawsim on the `.tres`, doc 06 §7) |
| `tools/build_pawcore_tests.ps1` | builds `build/pawcore_tests/pawcore_tests.exe` and `pawcore_sim.exe` with plain MSVC (no engine) and runs the L1 tests in seconds; `-Long` runs the long suite |
| `tools/check_godot_reawaken_travel.py` | runs the REAL Godot HUD (scratch copy, sandboxed) through a Reawaken in four cases and checks the spend step opens and where the run lands (`--head`: the negative test against Godot's HEAD) |
| `tools/make_godot_fixtures.py`, `tools/godot_fixtures/` | runs the REAL Godot code (scratch copy, sandboxed) to write the ground-truth fixtures; one `*_fixture.gd` per ported manager, and per slice of the field (`field_rules`, `kittens`, `aura`, `pounce`, `extras`, `boss`) |
| `tools/gen_paw_rows.py`, `tools/import_data.py`, `tools/run_automation.py` | the DataTable row structs from the CSV headers; the headless import + parity (`--check`, `--self-test`); the engine automation tests headless (both refuse while an Unreal process of this project runs: `tools/unreal_procs.py`) |
| `tools/check_all.ps1` | **the one command for every gate** (doc 09 §7) |
| `tools/check_pawcore_static.py`, `check_mirror.py`, `fuzz_validator.py`, `check_pawsim_parity.py`, `sabotage_pawcore.py`, `sabotage_realm.py` | the gates and their negative tests: L0 static rules, PawCore vs pawsim, random CSV corruption, pawsim CSV vs `.tres`, one-line source sabotages of PawCore and of the game module's realm (doc 09 §7) |
| `tools/gen_paw_tokens.py`, `tools/gen_data_strings.py` | `UI/PawTokens.h` from Godot's `palette.gd`; `ST_Data.csv` (data names) from `Data/*.csv` (L0 checks both are current) |
| `tools/packaged_bench.py` | the L5 frame gate: runs `paw.Bench` in the packaged Development build under `build/package` (package by hand until `tools/package.ps1`, doc 10 §2) |
| `tools/unreal_procs.py` | the one guard every engine tool and gate asks before a headless run: blocks on this project's Unreal processes (or one naming no project), ignores other projects' (`--self-test`) |
| `tools/create_maps.py`, `tools/create_field_materials.py` | headless builders: the two maps and L_Realm's view (only what is missing); the field materials (`--force` rebuilds them) |
| `tools/spikes/` | **temporary** phase 0 spike scripts |
| `build/` | untracked outputs: test exes, full Godot fixtures (`*_godot_full.tsv`), gate logs (`build/check_all/`), the Godot scratch copy |
| `docs/` | this handoff; `WORKLIST-NIGHT-2026-10-28.md`, `-10-30.md` and `-11-01.md` are closed night work lists (history) |
| `CLAUDE.md` | the standing rules for any agent working here |

Git: `https://github.com/Keshikul/PawpocalypseUE`, branch `master`, LFS for binaries (`.gitattributes`). Its own repo, separate
from the Godot repo (`JanePonomareva/pawpocalypse`).
**Session history and the exact next step: [HANDOFF-LOG.md](HANDOFF-LOG.md)** (newest entry first; every session adds one).
