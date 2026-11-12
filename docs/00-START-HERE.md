# Pawpocalypse (Unreal) - START HERE

> Written **2026-10-28** for the port of Pawpocalypse from Godot 4.7.1 (`C:\Games\pawpocalypse`, itch 0.2.5) to
> **Unreal Engine 5.8.3** (`C:\Games\pawpocalypseUE\Pawpocalypse`, a blank C++ project created the same day).
> Every claim about the Godot game was read from its code, `/data`, or its own docs on that date. Every claim about
> Unreal is tagged with where it came from: `[KB]` the knowledge base at `C:\Games\Knowledge` (verified on UE 5.8.3 in
> a real project unless it says otherwise), `[Docs]` Epic documentation, `[Forum]` forum/community reports, `[Plan]`
> a decision this handoff makes that must still be proved by a spike.

## What this is

A **recreation of the same game**, not a sequel and not a redesign. The design, the numbers and the audio carry over;
the engine, the code and the build pipeline are new. The Godot project stays the reference build and the shipping product
until the Unreal build reaches parity (doc 11, phase 9). **Three exceptions, all the designer's**: the look is the port's own
since 2026-11-07 ("Plum everywhere", D14), so is the player-facing English since 2026-11-09 (D15), and since 2026-11-10 the
port fixes Godot's faults instead of copying them (D16, "Do not port faults from godot"; doc 01 §5).

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
| B | [appendix-b-godot-to-unreal-map.md](appendix-b-godot-to-unreal-map.md) | generated: all 79 Godot scripts, where each goes and in which phase |
| C | [appendix-c-code-constants.md](appendix-c-code-constants.md) | generated: the 580 numeric/colour constants and numeric `@export` defaults that live in Godot CODE, not `/data`, with their reasons |
| | [WORKLIST-RESTYLE-2026-11-07.md](WORKLIST-RESTYLE-2026-11-07.md) | the restyle (D14): decisions, the look spec, R0-R7, the review loop |
| | [WORKLIST-SLICE6-2026-11-07.md](WORKLIST-SLICE6-2026-11-07.md) | phase 6 slice 6: the Unicorn ceremony (done), the Reawaken banner, Boba, the tutorial |
| | [WORKLIST-REVIEW-2026-11-10.md](WORKLIST-REVIEW-2026-11-10.md) | the code review of 2026-11-10: 23 findings (CR1-CR23), each with its evidence, fix and failing test; the batched fix plan; what was found sound |
| | [WORKLIST-NIGHT-2026-11-10.md](WORKLIST-NIGHT-2026-11-10.md) | the night of 2026-11-10 (closed 2026-11-11, HANDOFF-LOG (93)-(96)): the review's batch A and the frame half of C and D, D16's batch F (every Godot fault the port copied), frame numbers before and after, the words drafted, the proof; each item's test, sabotages and evidence |
| | [HANDOFF-LOG.md](HANDOFF-LOG.md) | every session's entry, newest first; entries (1)-(72) in [HANDOFF-LOG-ARCHIVE.md](HANDOFF-LOG-ARCHIVE.md) |

Project rules for agents: [../CLAUDE.md](../CLAUDE.md). Engine knowledge: `C:\Games\Knowledge\INDEX.md` (read it, then
only the documents it points to; the **`2D/` folder** (`2D/01-2d-optimization-best-practices.md`,
`2D/02-unreal-2d.md`) was written from this handoff's research).

## How we proceed (the plan as written on 2026-10-28; where it stands is the next section)

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

## Where the port is (2026-11-12)

**Now: the restyle's R5 (the field) is drawn and waits on the designer's last reviews; next R6 (icons, badges, cards).**
The night of 2026-11-10 made the code review's fixes that need no decision and fixed every Godot fault the port had copied
(D16): HANDOFF-LOG (93)-(96). On 2026-11-11 the designer approved the words and decided F36: the review's batch B (every
load outcome on the title, F47; "Saved" only when saved) and F36's refund are built: HANDOFF-LOG (97).
On 2026-11-12 the designer decided F3 (Patience reads the job's slowness only, +6% a level; built, the first D16 change to a
NUMBER), and the review's last two items were done: CR17 built, CR5 step 2 built, measured worse and taken back. **The code
review's worklist is closed**: HANDOFF-LOG (100). The designer then asked for the economy to be rethought: the review
and its measured proposal are in [ECONOMY-REVIEW-2026-11-12](ECONOMY-REVIEW-2026-11-12.md); the designer chose all six changes and a "shorter denser game", and made the Unicorn a damage step (x1.5): **D17, built the same day** (doc 01 D17, HANDOFF-LOG (102)). An audit of the
code's `if` use found no broad overuse and five places worth fixing (HANDOFF-LOG (103), the designer's call). **Tonight's
plan: [WORKLIST-NIGHT-2026-11-12](WORKLIST-NIGHT-2026-11-12.md)** (the nightly on D17; a tree price pass measured).
The exact next step is always the newest [HANDOFF-LOG](HANDOFF-LOG.md) entry. Doc 11 has each phase's exit criteria and
status; doc 09 §7 the gates as they are today.

| Phase | State |
|---|---|
| 0 Setup and spikes | **Done** 2026-10-28: settings, modules, maps, spikes S1-S4 recorded (doc 04 §10, doc 05); the spike code deleted 2026-11-04, kept in commit `10c118e` |
| 1 Data and rules | **Done** 2026-10-30 (HANDOFF-LOG (17)): every Godot manager's rules in `PawCore`, each proved against the REAL Godot code on thousands of random states (doc 09 §7); `Data/CodeConstants.csv`; the 12 DataTables and the game's data adapter, proven equal to the CSVs |
| 2 The realm | **Done** 2026-11-01 (20): `PcField` simulates the whole realm; the game runs it (`APawRealmView`, `UPawRealmSubsystem`, `UPawSpriteRenderer`); every card effect observed in a live realm; the packaged gate |
| 3 Progression UI | **Done** 2026-11-02 (21)-(29): the HUD and every dock page, the card pick, the skill tree with the Reawaken spend step, the ceremony, the job picker, the victory screen; the whole run through the UI (`Pawpocalypse.UI.Playthrough`) |
| 4 Title, saves, settings, pause | **Done** 2026-11-04 (31)-(41): the save (`PcSave`, five slots, atomic writes, recovery, the sandbox `-NoSave` / `-PawSaveDir=`), the title, the pause menu, the settings popup. **Since (38) every UI screen and piece is an editable Widget Blueprint** (baked once by `tools/bake_widgets.py`; the asset owns the layout) |
| 5 Pad and keyboard | **Work done** 2026-11-05 (42)-(49): Godot's input map as data, every screen on it, devices, glyphs and prompts, remapping and the Controls page, keys by position on any layout, the dock's held keys, the paw cursors, the pad-only playthrough. Waiting for the designer: hand-checks on real pads and a real AZERTY keyboard |
| 6 Presentation | Slices 1-5d and 6a **done** 2026-11-05/07 (50)-(63): the field's art and atlas, the backdrops, the effects, the sound, the chrome, the icons, the motion, the rainbows and the tree's sky, the Unicorn ceremony. **The restyle (D14) runs before 6b-6d** (the Reawaken banner, Boba, the tutorial: `WORKLIST-SLICE6-2026-11-07.md`) |
| The restyle (D14) | `WORKLIST-RESTYLE-2026-11-07.md`. **R0-R4 done** (65)-(70): the port's own palette (`tools/art/palette.py`), Plum ink, M PLUS Rounded 1c (`F_Paw`), plum chrome with Deep Plum edges and one solid shadow, Butter focus, the 16 px text floor, every screen fits at 150 % UI; the plain text (D15) (71)-(72). **R5 drawn** (73)-(90): D3 (aura, strike paw, crits), the look lint (everyday), the faces, the anatomy pass with one colour per character, the rig (moving wings and legs), the props. Waiting: the bosses, the imp, the rig's motion, the cat's pale coats, the draw-call budget (doc 01 Q11). R6, R7 to come |
| 7-9 | Localisation, hardening and packaging, parity sign-off: not started (the fonts were done early in R2) |

- **Git**: committed and pushed through HANDOFF-LOG (103) (2026-11-12, at the designer's request: F3, CR17, the economy
  review and D17, the `if` audit, tonight's plan).
  The Godot repo holds uncommitted decisions made there first for the designer (the findings'
  fixes; since (81) the dock default "right" in `settings_manager.gd`).
- **Gates**: `powershell -ExecutionPolicy Bypass -File tools\check_all.ps1` (everyday, 27 gates, ~6 min) or `-Full`
  (nightly, 40 gates, ~10 h: 662 PawCore and 563 realm sabotages since D17, 2026-11-12). Last everyday run 2026-11-12: 27
  of 27 green on D17, every new sabotage caught (HANDOFF-LOG (102)). Last `-Full` 2026-11-11/12 (on (99)):
  40 of 40 green, exit 0, 9 h 54 min, every sabotage caught (641 PawCore, 554 realm) (HANDOFF-LOG (99)). Every gate has
  been seen to fail (doc 09 §7).
- **The packaged frame gate** (2026-11-10, (85), (89); measured again on a quiet PC the same night, (95)): FAILS on draw
  calls, 67.7 for the whole frame against 50 (68.7 until the debug overlay left the bench, CR7); the field draws ~26, the
  frame without the scene 41.6. The game thread 1.5-2.3 ms mean and 2.2-3.6 ms p99 (under its 4 ms), the GPU 0.45 ms.
  The budget is the designer's call (doc 01 Q11). Packaging only when the designer asks. Measured again 2026-11-12 on (99)
  and on (100) (packaged at the designer's request, a quiet PC, interleaved): 67.7 draws, game 1.8-2.1 ms mean, render
  1.1 ms, the same before and after CR17 (HANDOFF-LOG (100)); the before package is kept in `build/package_before_60a6956`.
- **Godot findings and open questions**: doc 01 §3 (Q1, Q4, Q7, Q9-Q11 open; Q12 decided: D17) and §5 (open: F4, F5, F12, F15, F25, F41,
  F43-F46, F50; fixed in the port the night of 2026-11-10: F34, F35, F37-F40, F42, F48, F49, F51; on 2026-11-11, batch B:
  F36 (the designer's call: refund the summon) and F47; on 2026-11-12 F3 (the designer's call: Patience reads the job's
  slowness only, +6% a level)). Since D16 (2026-11-10) the port fixes Godot's faults itself
  instead of copying them; the designer's copy, for the Godot build, is
  `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`, kept in step (F51 added 2026-11-11; uncommitted there).

### Waiting on the designer (gathered 2026-11-10 from the HANDOFF-LOG's open lists)

- **R5 reviews** (the canvas): the bosses, the imp, the rig's motion; the cat's pale coats (look lint KNOWN 23).
- **Decisions**: the frame gate's draw-call budget (doc 01 Q11, with the night's clean numbers); the tree opening zoomed in
  (restyle R4); Q1, Q4, Q7, Q9, Q10 (doc 01 §3) (F36 decided 2026-11-11, F3 2026-11-12). Batch F's fixture mechanism was
  built the night of 2026-11-10 as doc 01 D16 recommends (`tools/godot_fixes.py`; since F3 it carries Godot's data too).
- **Words** (drafted first, CLAUDE.md): `docs/TEXT-DRAFT-2026-11-12.csv` (Patience's two tree lines, F3) was approved 2026-11-12 and is in `ST_Tree` (through `Data/TextOverrides.csv`); `docs/TEXT-DRAFT-2026-11-11.csv` was approved 2026-11-11 and is in `ST_UI` (batch
  B). Still to draft: the pause menu's slot line for a newer build's or an unreadable slot (it reads Empty there; saving to
  a newer one says "Slot {n} was not saved."); Continue's slot line on the title; the title's `{n} kills` lines (the port's own
  wording, HANDOFF-LOG (72)); the dock's price pills and kittens strip (restyle R4/R6).
- **The canvas's own text**: the Style board's "unimpressed" (rule 3), and two "grudge" lines (the title's tagline, the tree's
  chip). None of them is in the game.
- **Hand-checks**: a real Xbox pad and a PlayStation pad through Steam Input, a real AZERTY keyboard (phase 5); the Output
  device swap with two real devices and a pad's headset (HANDOFF-LOG (55)); the paw cursor in a packaged build; the night's
  eight (HANDOFF-LOG (96): the pad prompt on the dock after Continue, a busy biome's sound and the boss music, the stick
  under a card pick, a minute on the title, the window on the second monitor, the steered cat's hat and weapon, the Bait
  button, the Controls page after a pad swap).
- **Commits**: in the Godot repo, the decisions made there first (the findings' fixes, the dock default, the findings file):
  that repo also holds uncommitted work that is not the port's, so it is the designer's to commit.
- Known gap, not a decision: the skill text's numbers are literal (only the level counts are checked), so a `Data/` change to
  a skill's ladder does not update its blurb (HANDOFF-LOG (72)); the 280 changed lines are untranslated until phase 7.

## What already exists in this project

| Path | What |
|---|---|
| `Pawpocalypse.uproject`, `Config/` | UE 5.8.3-58210709 project (created 2026-10-28); phase 0 settings applied (doc 03 §7): no Lumen/RT/Nanite/Substrate, Common UI viewport, unused plugins off for the game (`tools/trim_game_plugins.py`; ProceduralMeshComponent ON since phase 6: the backdrop and the effects) |
| `Source/PawCore/` | pure C++17 rules, no engine headers except `Private/PawCoreModule.cpp`. `Public/`: `PcCore`, `PcCsv`, `PcFieldRules`, `PcField` (phase 2: the realm simulation), `PcCheats` (the cheat overlay's rules, dev only), `PcData` (+ generated `PcDataRows.h`, `PcEnums.h`), `PcValidate` (58 rules), `PcFormat`, `PcRng`, `PcConstants` (the code constants, loaded from CSV), `PcCombat`, `PcStats` (also holds `PcProgress`, the state the rules read), `PcEconomy`, `PcCards`, `PcCardDraw`, `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`, `PcTreeLayout` (the skill tree's shape and pad navigation), `PcTreeText` (its words; generated `PcTreeWording.h`). `PcSave` (the save: Godot's PawpocalypseSaveData, save / load rules, the file), `PcJson` (its JSON), `PcTitle` (the title
screen's rules), `PcHud` (the HUD's decisions), `PcPause` (the pause menu's), `PcSettings` (Godot's settings.cfg and the
dialog), `PcDevice` (phase 5: the prompt family, the cursor source, the toasts), `PcBindings` (phase 5 slice 4: the remapping rules, input_bindings.gd), `PcControls` (the Controls page's rules, controls_panel.gd), `PcBackdrop` (phase 6: biome_backdrop.gd's layout, Godot's PCG32 RNG and hash() reproduced; generated `PcBackdropTones.h` from `tools/art/palette.py`; the one file allowed float, L0 `FLOAT_IS_THE_SPEC`), `PcJuice` (phase 6 slice 5c: ui_juice.gd's hover, press, punch, pop-in and lean, and Godot's Tween easing, `TweenInterpolate`), `PcRainbow` (slice 5d: rainbow_bar.gd and rainbow_frame.gd; Godot's `from_hsv`, `fposmod`, `Range.set_value`); `PcBackdrop` also holds `PcSky` (skill_tree_sky.gd); `PcTween` (slice 6a: Godot's Tween sequencing) and `PcUnicorn` (unicorn_ceremony.gd). `Tests/`: doctest L1 + long suites, `PcSimCli.cpp` (the mirror CLI `pawcore_sim`), `Tests/Data/*_godot.tsv` (Godot ground truth) |
| `Source/Pawpocalypse/` | `Input/` (phase 5: `PawInput::KeyIs` over the live mapping context (`PawInput::LiveContext` / `ApplyBindings`: the player's bindings, slice 4), `PawKeyCodes` (Godot events <-> FKeys), generated `PawInputDefaults.h`, `UPawHalfAxis`, `FPawNavigationConfig` (Godot's `ui_*` keys for Slate), `UPawInputDeviceSubsystem` (family, cursor source, toasts, the focus prompt); `Pawpocalypse.Input.*`); `Save/` (phase 4: `UPawSaveSubsystem` over `IPawSaveStorage`: platform / folder / memory, chosen by `PawStorageForThisRun`; `UPawSettingsSubsystem`, the player's preferences in the same storage; `Pawpocalypse.Save.*`); `UI/` (phase 3: `UPawRootLayout`, `UPawMainMenu` (the title screen, phase 4), `UPawPauseMenu` (phase 4, the Menu stack), `UPawSettingsDialog` (phase 4, the Modal stack; its Controls page ends in `UPawControlsPanel`, slice 4c), `UPawHudScreen`, `UPawCardPickScreen` (the Menu stack's first screen), `UPawSkillTreeScreen` (+ generated `PawTreeTables.h`), `UPawReawakenCeremony` (the HUD's top layer), `UPawJobPicker` (the Modal stack's first screen), `UPawVictoryScreen` (the ending, Modal stack, the game theme since F21), `UPawButton`, `UPawPounceClock`, `UPawToast`, `UPawGarden` (phase 6: the title's garden, painted in Slate), the button prompts (`PawGlyphs`, `UPawPromptIcon`, `UPawPromptText`; phase 5), every screen and piece an editable Widget Blueprint (`PawWidgets.h`, `PawUi::BakedClasses`), `PawText` = the string tables, `PawUiKit` (since phase 6 the chrome, `PawUi::Chrome`), `PawIcons` (slice 5b: `DA_Icons`, `PawUi::Icon`, `UPawIconText`; since review CR11 `DA_Icons` is held by the engine subsystem `UPawIconCache`), `UPawUiAssets` (review CR12: an engine subsystem holding what the UI looks up on events: the prompts' glyph textures, a missing one remembered, the resolved Widget Blueprint classes, `F_Paw`), `UPawUiMemory` (review CR14: what the screens remember across rebuilds, Godot's statics: the dock's tab and step and the tree's last node for the session, the Jobs count seen per game; the game instance's), `FPawJuice` (slice 5c: the motion, one per screen), `UPawRainbowBar` / `UPawRainbowFrame` (`PawRainbow`) and `UPawTreeSky` (slice 5d: Godot's `_draw()` nodes painted in Slate, `PawUi::PaintCanvas`), `UPawUnicornCeremony` (slice 6a, the tree's top part), generated `PawTokens.h`, `PawUiConstants.h`; the `Pawpocalypse.UI.*` tests); `Core/` (thin GameInstance, GameMode, PlayerController (creates the root layout and the HUD), `LogPaw`, `UPawGameSubsystem`: the data and the progress); `Data/` (`PawDataRows.h`, generated row structs; `FPawDataAdapter`, DataTables -> PawCore; the `Pawpocalypse.Data.*` automation tests); `Realm/` (`FPawRealmSim`: the fixed step and the glue; `UPawRealmSubsystem` (also `ChooseCard` / `RerollHand` / `BanishFromHand`, the card pick's entries, and the pause holders); `APawRealmView`: camera, input, the backdrop, the creatures, the effects; `PawFieldLook.h` (phase 6: the creatures' look, from Godot's code); `FPawVfx` (phase 6: Godot's effects ported line by line); the `Pawpocalypse.Realm.*` tests (`.Look.*`, `.Vfx.*` among them), the card effects in `PawCardEffectsTests.cpp`); `Render/` (`UPawSpriteRenderer`: shape layers, atlas sprites (masked, soft, shadow per page), the tiled floor, the backdrop's procedural mesh, three effect bands; `UPawSpriteAtlas` (`DA_FieldAtlas`); `FPawFxCanvas`, Godot's draw calls as triangles); `Audio/` (phase 6: `UPawAudioSubsystem`, AudioManager; `UPawAudioBank` (`DA_AudioBank`); `Pawpocalypse.Audio.*`); `Dev/` (`APawDebugHud`, the `paw.Cheat.*` console commands, `paw.Bench`: the packaged frame gate, `paw.Shot` and its tour steps (`paw.Press` (a key through Slate), `paw.Prompts`, `paw.Settings`, and since phase 6 `paw.Cat`, `paw.Boss`, `paw.Travel`, `paw.Fx`)) |
| `Source/PawpocalypseEditor/` | the editor module: `UPawWidgetBaker` (bakes a class's code layout into its Widget Blueprint once; grafts new parts into an existing one; migrates the assets' saved colours and fonts), `UPawInputAssetBuilder` (the generated `IA_*` / `IMC_Pawpocalypse`), `UPawFontBuilder` (the `FF_*` faces and `F_Paw`, headless) |
| `Content/Pawpocalypse/` | `Maps/L_Title` (empty), `Maps/L_Realm` (one configured `APawRealmView`; both by `tools/create_maps.py`); `Materials/` (generated by `tools/create_field_materials.py`: `M_Field*` (shapes), `M_Sprite` / `M_SpriteSoft` (atlas sprites), `M_Floor` (the tiled floor), `M_BackdropFill` (sRGB vertex colours: the backdrop's patches and the effects)); `Art/Field` (`T_Atlas_Field_*`, `DA_FieldAtlas`), `Art/Backdrops` (`T_backdrop_tier_*`), `Audio/` (`SW_*`, `DA_AudioBank`; `tools/import_audio.py`), `UI/Cursors` (the paw cursors, loose PNGs, staged); `Data/DT_*` (12 DataTables, generated by `tools/import_data.py`, never edited); `Localization/Strings/ST_UI.csv` (hand-written) and `ST_Data.csv` (generated), staged loose; `UI/Widgets` (`WBP_*` screens and pieces, `BP_*` button styles: the designer's to edit); `Input/` (`IA_*`, `IMC_Pawpocalypse`: generated, never edited); `Art/UI/Input` (`T_input_*`, Godot's glyphs, `tools/import_art.py`); `Art/UI/Chrome` (`T_ui_*`, the theme's nine-patches, phase 6 slice 5a); `Art/UI/Icons` (178 `T_*` icons and `DA_Icons`, slice 5b); `Fonts/` (`FF_*` faces and the composite `F_Paw`, restyle R2) |
| `Data/*.csv` | **every gameplay resource of the Godot game** (266 resources, 11 classes), exported by `tools/export_godot_data.py`, enums written by NAME |
| `Data/AudioEvents.csv`, `Data/AudioMusic.csv` | phase 6: audio_manager.gd's sound table and music (by `tools/export_audio.py`, also run by the exporter; not DataTables: `tools/import_audio.py` builds `DA_AudioBank`) |
| `Data/CodeConstantsMap.csv`, `Data/CodeConstants.csv` | the reviewed destination of every Godot code constant (585 rows); the 66 gameplay ones it selects, exported with their values and reasons from the Godot code (`tools/export_code_constants.py`, run by the exporter) |
| `Data/TextOverrides.csv` | the port's own English (2026-11-09, the designer's text pass: 279 lines, Godot's line beside each; 280 since F3, 2026-11-12): `ST_UI` holds its lines directly, `gen_data_strings.py` / `gen_tree_tables.py` apply the `ST_Data` / `ST_Tree` ones (`tools/text_overrides.py`); never exported, Godot's words unchanged |
| `tools/text_audit.py`, `tools/text_overrides.py` | the text rule (budgets, one word per idea, banned words; drafts in `docs/TEXT-DRAFT-*.csv` while a pass is under review) and the port's own English over Godot's (doc 08 §4) |
| `tools/export_godot_data.py`, `gen_pc_data.py`, `gen_pc_enums.py` | the exporter (doc 06 §2) and the generators of `PcDataRows.h` / `PcEnums.h` from the Godot data scripts (L0 checks they are current) |
| `tools/list_godot_code_constants.py`, `tools/make_port_map.py` | regenerate appendices C and B from the Godot repo (the map script fails if a Godot script is unmapped) |
| `tools/pawsim.py` | the balance simulator, reading `Data/*.csv` (byte-identical to the Godot pawsim on the `.tres`, doc 06 §7) |
| `tools/econ_review.py` | pawsim plus the game's waves, a log of every buy, tree level and Reawaken, and proposals applied in memory: the economy review's numbers (doc 06 §7). `today` is D17's economy; `--proposal before_d17` the one before it |
| `tools/build_pawcore_tests.ps1` | builds `build/pawcore_tests/pawcore_tests.exe` and `pawcore_sim.exe` with plain MSVC (no engine) and runs the L1 tests in seconds; `-Long` runs the long suite |
| `tools/check_godot_reawaken_travel.py` | runs the REAL Godot HUD (scratch copy, sandboxed) through a Reawaken in four cases and checks the spend step opens and where the run lands (`--head`: the negative test against Godot's HEAD) |
| `tools/make_godot_fixtures.py`, `tools/godot_fixtures/` | runs the REAL Godot code (scratch copy, sandboxed) to write the ground-truth fixtures; one `*_fixture.gd` per ported manager, and per slice of the field (`field_rules`, `kittens`, `aura`, `pounce`, `extras`, `boss`), and since phase 6 `backdrop` (a probe subclass of the real BiomeBackdrop records what it draws), `juice`, `rainbow` (the rainbow bar, frame and tree's sky: each script recompiled from its own source with its draw calls renamed to recorders) and `unicorn` (the same for unicorn_ceremony.gd, its tweens stepped by `custom_step`) |
| `tools/gen_paw_rows.py`, `tools/import_data.py`, `tools/run_automation.py` | the DataTable row structs from the CSV headers; the headless import + parity (`--check`, `--self-test`); the engine automation tests headless (both refuse while an Unreal process of this project runs: `tools/unreal_procs.py`) |
| `tools/check_all.ps1` | **the one command for every gate** (doc 09 §7) |
| `tools/bake_widgets.py`, `tools/create_input_assets.py`, `tools/export_input_map.py` | headless builders: the Widget Blueprints (`--graft` adds new parts to an existing one); the Enhanced Input assets from `Data/InputMap.csv`, itself exported from Godot's input map (by `export_godot_data.py`) |
| `tools/art/` | phase 6: the art generators, copied from Godot (a PORT note in each) and this project's own since D14 (2026-11-07): `make_sweet_art.py` (every creature, the cat, icons, cards, cursors; writes the rig layers), `sweet_shapes.py` (the grammar: ellipses, `tube`, `oval`, `fuse`, the faces), `sweet_anatomy.py` (R5: enemies and bosses with bodies, the rig's `limb()`), `sweet_biomes.py` (the 12 floors and the 36 props), `sweet_props.py`, `sweet_evolutions.py` (the cat's stages, mounts, weapons), `sweet_ui.py` (the chrome), `sweet_input.py` (the glyphs), `make_ground_shadow.py`, `palette.py` (the palette and its rules), `tools/art/hand/` (four hand-drawn UI SVGs); `py tools/art/build_art.py [--check]` writes `SourceArt/svg` (554 files) and proves it reproducible (and the floors' seams) |
| `tools/raster_svg.py`, `tools/pack_atlas.py`, `tools/import_art.py` | `SourceArt/svg` -> `SourceArt/png` (the glyphs, the cursors, since 5a the chrome and since 5b the icons at the SVGs' own size; `--check`); the field atlas (249 sprites with the rig's layers, 4 pages + `field.csv`; `--check`); headless import: `input`, `chrome` and `icons` (UI textures; `icons` also writes `DA_Icons`), `atlas` (`DA_FieldAtlas`), `floors` |
| `tools/export_audio.py`, `tools/import_audio.py` | phase 6: Godot's sound table -> `Data/Audio*.csv` + the played files -> `SourceAudio/` (`--check`); headless import -> `SW_*`, `DA_AudioBank` |
| `tools/godot_tour.py` | the screenshot tour of both games side by side (`build/tour/compare`); `--set biomes` (phase 6): the 12 biomes and the title |
| `tools/padplay_tour.py` | L4: the pad-only playthrough and the focus tours (`paw.PadPlay` in a real `-game` window, fresh sandboxed saves) |
| `tools/check_pawcore_static.py`, `check_mirror.py`, `fuzz_validator.py`, `check_pawsim_parity.py`, `sabotage_pawcore.py`, `sabotage_realm.py` | the gates and their negative tests: L0 static rules, PawCore vs pawsim, random CSV corruption, pawsim CSV vs `.tres`, one-line source sabotages of PawCore and of the game module's realm (doc 09 §7) |
| `tools/art/look_lint.py`, `tools/art/review_anatomy.py`, `tools/art/review_rig.py` | restyle R5 (2026-11-09): the look spec on the field's drawings, an everyday gate (palette tokens or their grammar shades only, one ink at one width, flat, each body clear of every floor it ships on, eyes that read at 56 px, one colour per character, rig layers that never collide, no self-crossing outline; KNOWN 23, a list that only shrinks; `--self-test` plants one fault per rule); the review sheets: a batch beside today's at game size, and the rig's motion (`build/review/rig.gif`) |
| `tools/fonts.py`, `tools/import_fonts.py`, `tools/check_font_coverage.py` | restyle R2: the font table (`UI/PawFontSpec.h`), the headless import of `SourceArt/fonts` into `F_Paw`, and the coverage gate (every language's characters reach a face that has them) |
| `tools/gen_fx_look.py`, `tools/art/fx_look.py`, `tools/art/review_d3.py` | restyle R5, D3 (2026-11-09): the field effects' look (the aura lifted with a Bone ring, the clutching paw on the strike, crits in Butter) in one place, `fx_look.py`; the generator writes `Realm/PawFxLook.h` (the paw's shapes from `ui_cursor_paw_closed`'s drawing; L0 checks it is current); the review sheet the designer approved draws from the same values |
| `tools/gen_paw_tokens.py`, `tools/gen_data_strings.py`, `tools/gen_input_defaults.py` | `UI/PawTokens.h` (and since phase 6 PawCore's `PcBackdropTones.h`) from `tools/art/palette.py` (since R0, 2026-11-07; and the test-only `Tests/PcGodotLook.h`); `ST_Data.csv` (data names) from `Data/*.csv`; `Input/PawInputDefaults.h` (Godot's input map, remapping's defaults) from `Data/InputMap.csv` (L0 checks all three are current) |
| `tools/packaged_bench.py` | the L5 frame gate: runs `paw.Bench` in the packaged Development build under `build/package` (package by hand until `tools/package.ps1`, doc 10 §2) |
| `tools/unreal_procs.py` | the one guard every engine tool and gate asks before a headless run: blocks on this project's Unreal processes (or one naming no project), ignores other projects' (`--self-test`) |
| `tools/create_maps.py`, `tools/create_field_materials.py` | headless builders: the two maps and L_Realm's view (only what is missing: materials, the atlas, the floors); the field, sprite, floor and fill materials (`--force` rebuilds them) |
| `SourceArt/`, `SourceAudio/` | `SourceArt/svg`: the generated art set (committed); `SourceArt/png`: rasters and atlas pages (gitignored, regenerated); `SourceAudio/`: the 53 sounds something plays (LFS) |
| `build/` | untracked outputs: test exes, full Godot fixtures (`*_godot_full.tsv`), gate logs (`build/check_all/`), the Godot scratch copy |
| `docs/` | this handoff (the reading order above); the closed night work lists were deleted 2026-11-10 (their results are in the HANDOFF-LOG; the files are in git history) |
| `CLAUDE.md` | the standing rules for any agent working here |

Git: `https://github.com/Keshikul/PawpocalypseUE`, branch `master`, LFS for binaries (`.gitattributes`). Its own repo, separate
from the Godot repo (`JanePonomareva/pawpocalypse`).
**Session history and the exact next step: [HANDOFF-LOG.md](HANDOFF-LOG.md)** (newest entry first; every session adds one).
