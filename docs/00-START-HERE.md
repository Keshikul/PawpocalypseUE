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
| B | [appendix-b-godot-to-unreal-map.md](appendix-b-godot-to-unreal-map.md) | generated: all 79 Godot scripts (29,577 lines on 2026-11-05), where each goes and in which phase |
| C | [appendix-c-code-constants.md](appendix-c-code-constants.md) | generated: the 580 numeric/colour constants and numeric `@export` defaults that live in Godot CODE, not `/data`, with their reasons |

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

## Where the port is (2026-11-08)

- **Phase 0 done**: settings, modules, maps, spikes S1-S4 all recorded (doc 04 §10, doc 05); the spike code, scripts and assets
  were deleted 2026-11-04 (HANDOFF-LOG (35)) and live on in commit `10c118e`.
- **Phase 1 done**: the rules were ported into `PawCore` one Godot manager at a time, each proved against the REAL
  Godot code on thousands of random states (doc 09 §7). Ported: data + validator, `PcFormat`, `PcRng`, `PcCombat`, `PcStats`,
  `PcEconomy`, `PcCards` + `PcCardDraw`, `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`: every Godot manager's
  rules; `Data/CodeConstants.csv` (the gameplay constants of Godot code); the 12 DataTables (`tools/import_data.py`) and
  the game's data adapter, proven equal to the CSVs. Its exit gate, the nightly `-Full` ran green 2026-10-30 (HANDOFF-LOG (17)).
- **Phase 2: all 7 slices done** (HANDOFF-LOG (16), (18), (19)): `PcField` simulates the whole realm (mice, kittens, the
  aura, Snare and zones, Pounce, the paw's and the kill's extras, the Prism, the boss and its adds), checked against the real
  Godot code, rule tests, a soak and 272 PawCore sabotages (627 across PawCore today); and the game runs it: L_Realm opens on a live field (`APawRealmView`,
  `UPawRealmSubsystem` over `FPawRealmSim`, `UPawSpriteRenderer`; placeholder discs then, Godot's art since phase 6), with the debug HUD and the
  cheats (`paw.Cheat.*`). **Phase 2 closed 2026-11-01** (HANDOFF-LOG (20)): every card effect observed in a live realm
  (`Pawpocalypse.Realm.CardEffects`, `.EveryCard`) and the packaged-build gate (`py tools/packaged_bench.py`: 320 mice in a
  packaged Development build, game thread 2.7 ms p99 against 4 ms, 14 draw calls, the cooked L_Realm drawing).
- **Phase 3 started 2026-11-01** (HANDOFF-LOG (21)): the HUD and the dock's Upgrades page are live (`Source/Pawpocalypse/UI`,
  `PcHud` in PawCore, the string tables `ST_UI` / `ST_Data`, `PawTokens.h`); the Realms and Reawaken pages since (22); the
  Catches, Achievements and Jobs pages since (23), every dock page built; the realm earns achievements; after a Reawaken the
  run opens in Territory's biome (designer 2026-11-01, "Territory wins", F20, fixed in Godot first). The card pick screen since
  (24): a Menu-stack screen that holds the field while a hand is up; the skill tree's layout and navigation in PawCore
  (`PcTreeLayout`, exact against Godot); the skill tree screen since (25): browse (T / pad Y) and the Reawaken's spend step, every
  word generated from skill_tree_ui.gd (`tools/gen_tree_tables.py`; the English since 2026-11-09 the port's own where the
  designer's text pass changed it, `Data/TextOverrides.csv`). The Reawaken ceremony and the job picker since (26), so a
  Reawaken runs Godot's whole order: ceremony, spend step, job picker, travel (doc 05 §6.3). The victory screen since (27), and
  numbers printed Godot's way (ungrouped, culture-invariant). The screenshot tour against Godot since (28) (`py tools/godot_tour.py`).
  The whole run through the UI since (29) (`Pawpocalypse.UI.Playthrough`): **phase 3's exit criteria are met** but the
  packaged-build gate, which waits for the designer to ask for a package. The nightly `-Full` since (30). 
- **Phase 4 started 2026-11-03** (HANDOFF-LOG (31)): F21 and F22 decided by the designer and fixed in Godot first, the port followed;
  the save in PawCore (`PcSave`, `PcJson`: exact against the real save_manager.gd, fixture `save`) and its container in the game
  (`Source/Pawpocalypse/Save`: `UPawSaveSubsystem`, five slots, the atomic write and recovery, autosave, the sandbox: `-NoSave`,
  `-PawSaveDir=`). Slice 3, the title screen, since (37): `UPawMainMenu` on L_Title (`PcTitle`, fixture `title`); a new game
  writes its slot the same frame, a load opens L_Realm. **Since (38) every UI screen and piece is an editable Widget
  Blueprint** (`Content/Pawpocalypse/UI/Widgets`, baked once by `tools/bake_widgets.py`, the asset owns the layout; contract in
  `UI/PawWidgets.h`). The pause menu since (39): `UPawPauseMenu` / `WBP_PauseMenu` (`PcPause`, fixture `pause`), opened
  by the HUD on Esc / Start. The settings popup since (40): `UPawSettingsDialog` / `WBP_SettingsDialog` over
  `UPawSettingsSubsystem` (`PcSettings`, Godot's settings.cfg, fixture `settings`). **Phase 4 CLOSED 2026-11-04 (41)**: the
  exit review, the packaged gate PASS, the build on the designer's Google Drive (Games/Pawpocalypse).
- **Phase 5 started 2026-11-04 (42)**: slice 1 done, Godot's input map as `Data/InputMap.csv` and the Enhanced Input assets
  generated from it (`/Game/Pawpocalypse/Input`, `UPawHalfAxis`). Slice 2 since (43): the field and every screen read the map
  (`PawInput::KeyIs`), the menus navigate on Godot's `ui_*` keys. Slice 3 since (44): `PcDevice` / `UPawInputDeviceSubsystem`
  (prompt family, cursor source, toasts), the stick-steered cat. Slice 3b since (45): the glyphs and button prompts
  (`UI/PawGlyphs`, `WBP_PromptIcon`, `WBP_PromptText`; the art through `tools/raster_svg.py` + `tools/import_art.py`).
  Slice 4a/4b since (47): the remapping rules (`PcBindings`, exact against input_bindings.gd) and the map following them (the
  live mapping context, `PawInput::ApplyBindings`; Godot's `[bindings]` in settings.cfg); slice 4c since (48): the Controls page
  (`UPawControlsPanel` over `PcControls`, exact against the real panel). **Slice 4 done.** Slice 5 since (49): keys by position
  on any layout, the dock's held keys (`PcDockRepeat`), the paw cursors, the pad-only playthrough with the focus tours
  (`py tools/padplay_tour.py`). **Phase 5's work is done**; its hand-checks on real pads and a real AZERTY keyboard wait for the
  designer.
- **Phase 6 started 2026-11-05 (50)**: slice 1, the field's art. Godot's art generators live in `tools/art` and reproduce
  its `assets/art` byte for byte (`py tools/art/build_art.py --check`; 390 files in `SourceArt/svg`); the field's 71 sprites
  are an atlas (`py tools/pack_atlas.py`, `py tools/import_art.py --run atlas`: `DA_FieldAtlas`, two BC7 pages); the realm
  draws every creature as Godot does (sizes, breathing, facing, tints, the signature glow, the cat's stage, weapon, hat and
  hop, the boss's twitch, shield and death, the adds, the shadows) in Godot's draw order (z, then tree: NOT a Y-sort,
  doc 04 §4). Slice 2 since (51): the backdrops: `PcBackdrop` lays out every biome's floor, patches, puddles and props exactly
  as Godot's BiomeBackdrop (fixture `backdrop`, its RNG and hash reproduced); the realm draws them (a tiled floor, one
  procedural mesh, atlas props) and the title paints its garden (`UPawGarden`, grafted into `WBP_MainMenu`); all 12 biomes
  and the title seen beside Godot's (`py tools/godot_tour.py --set biomes`). Slice 3 since (52): the effects, every Godot
  vfx / ring / zone / telegraph / sparkle ported line by line onto a Godot-style draw API (`FPawFxCanvas`, `FPawVfx`) in three
  translucent bands in Godot's z order. Slice 4 since (53): the sound (`UPawAudioSubsystem`, `DA_AudioBank` from
  `py tools/export_audio.py` + `py tools/import_audio.py --run`; doc 07 §6 as built); the Output device and the pad-audio toast since (55). The nightly `-Full` ran 2026-11-05/06 and every failure was fixed (56). Slice 5a since (58): the chrome, Godot's nine-patch art where its theme draws it (`PawUi::Chrome`: the buttons, the settings lists, the job picker's and the victory's window, the wave bar, the separators), the selection ring painted over the plate, `Pawpocalypse.UI.Chrome`. Slice 5b since (59): the icons, everything Godot asks for by name or icon_path (`DA_Icons`, `PawUi::Icon`; the tabs as icons with the open tab's pressed plate, the currencies beside every amount, the achievement, job, card and skill pictures, the title's plates, the Pounce clock's paw) and the dock's quick bar (settings, tree, save, quit), `Pawpocalypse.UI.Icons`. The nightly `-Full` after 5b: every gate green (60). Slice 5c since (60): the motion, Godot's UiJuice (`PcJuice`, exact against Godot's own Tween easing and the real ui_juice.gd, fixture `juice`; `FPawJuice` at Godot's call sites: the hover pops, the press squash, the purchase punch, the leans of the one thing to do next, the leaving plate's pop-in; the Reawaken tab's glow; finding F42), `Pawpocalypse.UI.Juice.*`. Slice 5d since (62): the rainbows and the tree's sky, Godot's three `_draw()` nodes (`PcRainbow`, and `PcSky` in `PcBackdrop`: exact against the real scripts, each recompiled with its draw calls renamed to recorders, fixture `rainbow`; `UPawRainbowBar` the Reawaken page's bar, `UPawRainbowFrame` the zone plate's frame, `UPawTreeSky`, painted in Slate by `PawUi::PaintCanvas`), and every bar filled Godot's way (both caps, the Range's step: `PawUi::SetBarValue`), `Pawpocalypse.UI.Rainbow.*`. Slice 6 started (63): its plan in `docs/WORKLIST-SLICE6-2026-11-07.md` (6a the Unicorn ceremony, 6b the Reawaken banner, 6c Boba, 6d the tutorial); **6a done**: `PcTween` (Godot's Tween sequencing) and `PcUnicornCeremony`, exact against the real unicorn_ceremony.gd (fixture `unicorn`), `UPawUnicornCeremony` over the tree, `Pawpocalypse.UI.Unicorn.*`. 6b-6d (the Reawaken banner, Boba, the tutorial) wait for the restyle (next bullet).
- **The restyle (D14, 2026-11-07)**: the Unreal look leaves Godot (designer: "Plum everywhere"; font M PLUS Rounded 1c;
  the cat stays grey). Plan `docs/WORKLIST-RESTYLE-2026-11-07.md`; **R0-R3 done** (HANDOFF-LOG (65), (66), (68)): the
  palette is `tools/art/palette.py`; the ink is Plum `#2A1A33`; every text is M PLUS Rounded 1c (`F_Paw`, Noto for ko / zh /
  th), titles and numbers in Black; the panels are plum with bone text; every plate has a Deep Plum edge and one solid
  shadow straight down, a pressed button sinks onto it. **R4 (screens and layout) DONE** (HANDOFF-LOG (69), (70)): Butter
  focus and selection, the 16 px text floor, the title, the HUD, the dock's rows, the tree's round nodes and pills, Butter
  titles; every screen fits at 150 % UI (`Pawpocalypse.UI.Layout.*`). Its leftover faults wait on R6 art and the designer's
  words. **Next R5 (the field).** The plain text is IN (HANDOFF-LOG (72)): 279 lines approved by the designer, "enemies" not
  "mice"; `ST_UI` directly, `ST_Data` / `ST_Tree` through `Data/TextOverrides.csv` (Godot's words unchanged). Slice 6's
  6b-6d after the restyle.
- **Committed and pushed through `b0ce7be`** (2026-11-09, designer: "Commit and push"; `origin/master` on GitHub): restyle
  R0-R4, the plain-text pass (D15), HANDOFF-LOG (70)-(72).
- **One gate command**: `powershell -ExecutionPolicy Bypass -File tools\check_all.ps1` (everyday, minutes) or `-Full`
  (nightly, ~10 h measured: 627 PawCore and 484 realm sabotages since 2026-11-09). **Last everyday run 2026-11-09 (HANDOFF-LOG (72), the text applied): 24 of 24 green. Last `-Full` run 2026-11-06 (HANDOFF-LOG (60))**: all 31
  gates green, every sabotage caught (563 PawCore, 412 realm), 9 h 38 min. The run before, 2026-11-05/06 (HANDOFF-LOG (56)): 29 of 30
  gates green; the realm gate refused on one stale target, which was retargeted; its rerun caught 381 of 388, the 7 missed
  were 1 retargeted (`view_no_effects`, then CAUGHT alone) and 6 retired (bake-only lines since the Widget Blueprints): every
  sabotage in the list now seen CAUGHT, the everyday set exit 0. Since then the everyday set checks every sabotage target
  (`sabotage targets current`). Every gate has been seen to fail (doc 09 §7).
- Findings in the Godot game and open designer questions: doc 01 §3 and §5 (F13-F18 new 2026-10-31, F19 and F20 2026-11-01, F21 and F22 2026-11-02 (the job picker's look; the Realms page ignores passage); F13, F16
  and F19 were bugs a player can see; **every finding with a fix was fixed 2026-11-03** (designer: "all open findings", Godot first,
  uncommitted there; HANDOFF-LOG (32)); open: F3, F4, F5, F12, F15 (no fix to apply), F25 (Godot's .tres precision), F37 (2026-11-05: the Controls page's controller column can show an old pad; low priority), F38 (2026-11-05: a cat steered right by the stick keeps its hat and weapon on the left), F39 (2026-11-05: the Cone of Shame's and the job's rings draw at twice the cat's position), F40 (2026-11-05: five sounds play twice, layered), F41 (2026-11-06: Godot's separators draw nothing); **the save findings F26-F33, found, decided and fixed 2026-11-04** (Godot first, uncommitted
  there; HANDOFF-LOG (36)): the save now keeps the deepest biome, Second Helping's Stardust, Body Count's and Levy's counts
  and a Reawaken in progress (resumed at the spend step); a damaged save loads safely); the designer's copy of the findings is
  `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md` (kept in step with §5).

## What already exists in this project

| Path | What |
|---|---|
| `Pawpocalypse.uproject`, `Config/` | UE 5.8.3-58210709 project (created 2026-10-28); phase 0 settings applied (doc 03 §7): no Lumen/RT/Nanite/Substrate, Common UI viewport, unused plugins off for the game (`tools/trim_game_plugins.py`; ProceduralMeshComponent ON since phase 6: the backdrop and the effects) |
| `Source/PawCore/` | pure C++17 rules, no engine headers except `Private/PawCoreModule.cpp`. `Public/`: `PcCore`, `PcCsv`, `PcFieldRules`, `PcField` (phase 2: the realm simulation), `PcCheats` (the cheat overlay's rules, dev only), `PcData` (+ generated `PcDataRows.h`, `PcEnums.h`), `PcValidate` (57 rules), `PcFormat`, `PcRng`, `PcConstants` (the code constants, loaded from CSV), `PcCombat`, `PcStats` (also holds `PcProgress`, the state the rules read), `PcEconomy`, `PcCards`, `PcCardDraw`, `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`, `PcTreeLayout` (the skill tree's shape and pad navigation), `PcTreeText` (its words; generated `PcTreeWording.h`). `PcSave` (the save: Godot's PawpocalypseSaveData, save / load rules, the file), `PcJson` (its JSON), `PcTitle` (the title
screen's rules), `PcHud` (the HUD's decisions), `PcPause` (the pause menu's), `PcSettings` (Godot's settings.cfg and the
dialog), `PcDevice` (phase 5: the prompt family, the cursor source, the toasts), `PcBindings` (phase 5 slice 4: the remapping rules, input_bindings.gd), `PcControls` (the Controls page's rules, controls_panel.gd), `PcBackdrop` (phase 6: biome_backdrop.gd's layout, Godot's PCG32 RNG and hash() reproduced; generated `PcBackdropTones.h` from palette.gd; the one file allowed float, L0 `FLOAT_IS_THE_SPEC`), `PcJuice` (phase 6 slice 5c: ui_juice.gd's hover, press, punch, pop-in and lean, and Godot's Tween easing, `TweenInterpolate`), `PcRainbow` (slice 5d: rainbow_bar.gd and rainbow_frame.gd; Godot's `from_hsv`, `fposmod`, `Range.set_value`); `PcBackdrop` also holds `PcSky` (skill_tree_sky.gd); `PcTween` (slice 6a: Godot's Tween sequencing) and `PcUnicorn` (unicorn_ceremony.gd). `Tests/`: doctest L1 + long suites, `PcSimCli.cpp` (the mirror CLI `pawcore_sim`), `Tests/Data/*_godot.tsv` (Godot ground truth) |
| `Source/Pawpocalypse/` | `Input/` (phase 5: `PawInput::KeyIs` over the live mapping context (`PawInput::LiveContext` / `ApplyBindings`: the player's bindings, slice 4), `PawKeyCodes` (Godot events <-> FKeys), generated `PawInputDefaults.h`, `UPawHalfAxis`, `FPawNavigationConfig` (Godot's `ui_*` keys for Slate), `UPawInputDeviceSubsystem` (family, cursor source, toasts, the focus prompt); `Pawpocalypse.Input.*`); `Save/` (phase 4: `UPawSaveSubsystem` over `IPawSaveStorage`: platform / folder / memory, chosen by `PawStorageForThisRun`; `UPawSettingsSubsystem`, the player's preferences in the same storage; `Pawpocalypse.Save.*`); `UI/` (phase 3: `UPawRootLayout`, `UPawMainMenu` (the title screen, phase 4), `UPawPauseMenu` (phase 4, the Menu stack), `UPawSettingsDialog` (phase 4, the Modal stack; its Controls page ends in `UPawControlsPanel`, slice 4c), `UPawHudScreen`, `UPawCardPickScreen` (the Menu stack's first screen), `UPawSkillTreeScreen` (+ generated `PawTreeTables.h`), `UPawReawakenCeremony` (the HUD's top layer), `UPawJobPicker` (the Modal stack's first screen), `UPawVictoryScreen` (the ending, Modal stack, the game theme since F21), `UPawButton`, `UPawPounceClock`, `UPawToast`, `UPawGarden` (phase 6: the title's garden, painted in Slate), the button prompts (`PawGlyphs`, `UPawPromptIcon`, `UPawPromptText`; phase 5), every screen and piece an editable Widget Blueprint (`PawWidgets.h`, `PawUi::BakedClasses`), `PawText` = the string tables, `PawUiKit` (since phase 6 the chrome, `PawUi::Chrome`), `PawIcons` (slice 5b: `DA_Icons`, `PawUi::Icon`, `UPawIconText`), `FPawJuice` (slice 5c: the motion, one per screen), `UPawRainbowBar` / `UPawRainbowFrame` (`PawRainbow`) and `UPawTreeSky` (slice 5d: Godot's `_draw()` nodes painted in Slate, `PawUi::PaintCanvas`), `UPawUnicornCeremony` (slice 6a, the tree's top part), generated `PawTokens.h`, `PawUiConstants.h`; the `Pawpocalypse.UI.*` tests); `Core/` (thin GameInstance, GameMode, PlayerController (creates the root layout and the HUD), `LogPaw`, `UPawGameSubsystem`: the data and the progress); `Data/` (`PawDataRows.h`, generated row structs; `FPawDataAdapter`, DataTables -> PawCore; the `Pawpocalypse.Data.*` automation tests); `Realm/` (`FPawRealmSim`: the fixed step and the glue; `UPawRealmSubsystem` (also `ChooseCard` / `RerollHand` / `BanishFromHand`, the card pick's entries, and the pause holders); `APawRealmView`: camera, input, the backdrop, the creatures, the effects; `PawFieldLook.h` (phase 6: the creatures' look, from Godot's code); `FPawVfx` (phase 6: Godot's effects ported line by line); the `Pawpocalypse.Realm.*` tests (`.Look.*`, `.Vfx.*` among them), the card effects in `PawCardEffectsTests.cpp`); `Render/` (`UPawSpriteRenderer`: shape layers, atlas sprites (masked, soft, shadow per page), the tiled floor, the backdrop's procedural mesh, three effect bands; `UPawSpriteAtlas` (`DA_FieldAtlas`); `FPawFxCanvas`, Godot's draw calls as triangles); `Audio/` (phase 6: `UPawAudioSubsystem`, AudioManager; `UPawAudioBank` (`DA_AudioBank`); `Pawpocalypse.Audio.*`); `Dev/` (`APawDebugHud`, the `paw.Cheat.*` console commands, `paw.Bench`: the packaged frame gate, `paw.Shot` and its tour steps (`paw.Press` (a key through Slate), `paw.Prompts`, `paw.Settings`, and since phase 6 `paw.Cat`, `paw.Boss`, `paw.Travel`, `paw.Fx`)) |
| `Source/PawpocalypseEditor/` | the editor module: `UPawWidgetBaker` (bakes a class's code layout into its Widget Blueprint once; grafts new parts into an existing one), `UPawInputAssetBuilder` (the generated `IA_*` / `IMC_Pawpocalypse`) |
| `Content/Pawpocalypse/` | `Maps/L_Title` (empty), `Maps/L_Realm` (one configured `APawRealmView`; both by `tools/create_maps.py`); `Materials/` (generated by `tools/create_field_materials.py`: `M_Field*` (shapes), `M_Sprite` / `M_SpriteSoft` (atlas sprites), `M_Floor` (the tiled floor), `M_BackdropFill` (sRGB vertex colours: the backdrop's patches and the effects)); `Art/Field` (`T_Atlas_Field_*`, `DA_FieldAtlas`), `Art/Backdrops` (`T_backdrop_tier_*`), `Audio/` (`SW_*`, `DA_AudioBank`; `tools/import_audio.py`), `UI/Cursors` (the paw cursors, loose PNGs, staged); `Data/DT_*` (12 DataTables, generated by `tools/import_data.py`, never edited); `Localization/Strings/ST_UI.csv` (hand-written) and `ST_Data.csv` (generated), staged loose; `UI/Widgets` (`WBP_*` screens and pieces, `BP_*` button styles: the designer's to edit); `Input/` (`IA_*`, `IMC_Pawpocalypse`: generated, never edited); `Art/UI/Input` (`T_input_*`, Godot's glyphs, `tools/import_art.py`); `Art/UI/Chrome` (`T_ui_*`, the theme's nine-patches, phase 6 slice 5a); `Art/UI/Icons` (178 `T_*` icons and `DA_Icons`, slice 5b) |
| `Data/*.csv` | **every gameplay resource of the Godot game** (266 resources, 11 classes), exported by `tools/export_godot_data.py`, enums written by NAME |
| `Data/AudioEvents.csv`, `Data/AudioMusic.csv` | phase 6: audio_manager.gd's sound table and music (by `tools/export_audio.py`, also run by the exporter; not DataTables: `tools/import_audio.py` builds `DA_AudioBank`) |
| `Data/CodeConstantsMap.csv`, `Data/CodeConstants.csv` | the reviewed destination of every Godot code constant (585 rows); the 66 gameplay ones it selects, exported with their values and reasons from the Godot code (`tools/export_code_constants.py`, run by the exporter) |
| `Data/TextOverrides.csv` | the port's own English (2026-11-09, the designer's text pass: 279 lines, Godot's line beside each): `ST_UI` holds its lines directly, `gen_data_strings.py` / `gen_tree_tables.py` apply the `ST_Data` / `ST_Tree` ones (`tools/text_overrides.py`); never exported, Godot's words unchanged |
| `tools/text_audit.py`, `tools/text_overrides.py` | the text rule (budgets, one word per idea, banned words; drafts in `docs/TEXT-DRAFT-*.csv` while a pass is under review) and the port's own English over Godot's (doc 08 §4) |
| `tools/export_godot_data.py`, `gen_pc_data.py`, `gen_pc_enums.py` | the exporter (doc 06 §2) and the generators of `PcDataRows.h` / `PcEnums.h` from the Godot data scripts (L0 checks they are current) |
| `tools/list_godot_code_constants.py`, `tools/make_port_map.py` | regenerate appendices C and B from the Godot repo (the map script fails if a Godot script is unmapped) |
| `tools/pawsim.py` | the balance simulator, reading `Data/*.csv` (byte-identical to the Godot pawsim on the `.tres`, doc 06 §7) |
| `tools/build_pawcore_tests.ps1` | builds `build/pawcore_tests/pawcore_tests.exe` and `pawcore_sim.exe` with plain MSVC (no engine) and runs the L1 tests in seconds; `-Long` runs the long suite |
| `tools/check_godot_reawaken_travel.py` | runs the REAL Godot HUD (scratch copy, sandboxed) through a Reawaken in four cases and checks the spend step opens and where the run lands (`--head`: the negative test against Godot's HEAD) |
| `tools/make_godot_fixtures.py`, `tools/godot_fixtures/` | runs the REAL Godot code (scratch copy, sandboxed) to write the ground-truth fixtures; one `*_fixture.gd` per ported manager, and per slice of the field (`field_rules`, `kittens`, `aura`, `pounce`, `extras`, `boss`), and since phase 6 `backdrop` (a probe subclass of the real BiomeBackdrop records what it draws), `juice`, `rainbow` (the rainbow bar, frame and tree's sky: each script recompiled from its own source with its draw calls renamed to recorders) and `unicorn` (the same for unicorn_ceremony.gd, its tweens stepped by `custom_step`) |
| `tools/gen_paw_rows.py`, `tools/import_data.py`, `tools/run_automation.py` | the DataTable row structs from the CSV headers; the headless import + parity (`--check`, `--self-test`); the engine automation tests headless (both refuse while an Unreal process of this project runs: `tools/unreal_procs.py`) |
| `tools/check_all.ps1` | **the one command for every gate** (doc 09 §7) |
| `tools/bake_widgets.py`, `tools/create_input_assets.py`, `tools/export_input_map.py` | headless builders: the Widget Blueprints (`--graft` adds new parts to an existing one); the Enhanced Input assets from `Data/InputMap.csv`, itself exported from Godot's input map (by `export_godot_data.py`) |
| `tools/art/` | phase 6: the art generators, copied from Godot (a PORT note in each) and this project's own since D14 (2026-11-07), and `tools/art/hand/` (four hand-drawn UI SVGs); `py tools/art/build_art.py [--check]` writes `SourceArt/svg` and proves it reproducible (and the floors' seams); `palette.py` is the palette |
| `tools/raster_svg.py`, `tools/pack_atlas.py`, `tools/import_art.py` | `SourceArt/svg` -> `SourceArt/png` (the glyphs, the cursors, since 5a the chrome and since 5b the icons at the SVGs' own size; `--check`); the field atlas (107 sprites, 3 pages + `field.csv`; `--check`); headless import: `input`, `chrome` and `icons` (UI textures; `icons` also writes `DA_Icons`), `atlas` (`DA_FieldAtlas`), `floors` |
| `tools/export_audio.py`, `tools/import_audio.py` | phase 6: Godot's sound table -> `Data/Audio*.csv` + the played files -> `SourceAudio/` (`--check`); headless import -> `SW_*`, `DA_AudioBank` |
| `tools/godot_tour.py` | the screenshot tour of both games side by side (`build/tour/compare`); `--set biomes` (phase 6): the 12 biomes and the title |
| `tools/padplay_tour.py` | L4: the pad-only playthrough and the focus tours (`paw.PadPlay` in a real `-game` window, fresh sandboxed saves) |
| `tools/check_pawcore_static.py`, `check_mirror.py`, `fuzz_validator.py`, `check_pawsim_parity.py`, `sabotage_pawcore.py`, `sabotage_realm.py` | the gates and their negative tests: L0 static rules, PawCore vs pawsim, random CSV corruption, pawsim CSV vs `.tres`, one-line source sabotages of PawCore and of the game module's realm (doc 09 §7) |
| `tools/gen_paw_tokens.py`, `tools/gen_data_strings.py`, `tools/gen_input_defaults.py` | `UI/PawTokens.h` (and since phase 6 PawCore's `PcBackdropTones.h`) from `tools/art/palette.py` (since R0, 2026-11-07; and the test-only `Tests/PcGodotLook.h`); `ST_Data.csv` (data names) from `Data/*.csv`; `Input/PawInputDefaults.h` (Godot's input map, remapping's defaults) from `Data/InputMap.csv` (L0 checks all three are current) |
| `tools/packaged_bench.py` | the L5 frame gate: runs `paw.Bench` in the packaged Development build under `build/package` (package by hand until `tools/package.ps1`, doc 10 §2) |
| `tools/unreal_procs.py` | the one guard every engine tool and gate asks before a headless run: blocks on this project's Unreal processes (or one naming no project), ignores other projects' (`--self-test`) |
| `tools/create_maps.py`, `tools/create_field_materials.py` | headless builders: the two maps and L_Realm's view (only what is missing: materials, the atlas, the floors); the field, sprite, floor and fill materials (`--force` rebuilds them) |
| `SourceArt/`, `SourceAudio/` | `SourceArt/svg`: the generated art set (committed); `SourceArt/png`: rasters and atlas pages (gitignored, regenerated); `SourceAudio/`: the 53 sounds something plays (LFS) |
| `build/` | untracked outputs: test exes, full Godot fixtures (`*_godot_full.tsv`), gate logs (`build/check_all/`), the Godot scratch copy |
| `docs/` | this handoff; `WORKLIST-NIGHT-2026-10-28.md`, `-10-30.md` and `-11-01.md` are closed night work lists (history) |
| `CLAUDE.md` | the standing rules for any agent working here |

Git: `https://github.com/Keshikul/PawpocalypseUE`, branch `master`, LFS for binaries (`.gitattributes`). Its own repo, separate
from the Godot repo (`JanePonomareva/pawpocalypse`).
**Session history and the exact next step: [HANDOFF-LOG.md](HANDOFF-LOG.md)** (newest entry first; every session adds one).
