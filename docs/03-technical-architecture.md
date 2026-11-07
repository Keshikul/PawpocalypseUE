# 03 - Technical architecture

> Unreal 5.8.3, C++. Knowledge-base references: `U/03` (framework, subsystems, sim/presentation, fixed timestep),
> `U/04` (C++/Blueprint split, references), `U/15` (tick discipline), `U/02` (coding standard, GC). Single player,
> no networking (D4), so the KB's replication rules are skipped; its sim/presentation split is kept in full.

## 1. The shape

```
Source/
  PawCore/              pure C++17, NO engine headers. Every rule and number of the game.
    Public/Pc*.h          model types, the rule functions, the event queue
    Private/
    Tests/                plain unit tests (doctest/Catch2 single header), built by a plain compiler in seconds
  Pawpocalypse/         the game module (Runtime)
    Core/                 GameInstance, GameMode, PlayerController, developer settings, log categories
    Data/                 DataTable row structs (FPaw*Row), the adapter that fills PawCore from DataTables
    Realm/                the realm world subsystem (drives PawCore's field sim), its view actor, the creatures' look
                          (PawFieldLook.h) and the effects (FPawVfx) (phase 6)
    Render/               the sprite renderer (shapes, atlas sprites, the floor, the backdrop mesh, effect bands), the
                          atlas asset, FPawFxCanvas (Godot's draw calls as triangles) (doc 04)
    UI/                   Common UI screens, the dock, the tree, widgets kit, tokens (doc 05)
    Input/                input device subsystem, glyphs, bindings (doc 05)
    Save/                 save container, slots, migrations (doc 08)
    Audio/                UPawAudioSubsystem (AudioManager) and the generated bank (doc 07 §6; built phase 6)
    Platform/             IPawPlatform: achievements, cloud, rich presence, store URL (Null + Steam impls, doc 10)
    Dev/                  cheat overlay, soak driver, screenshot tour, trailer director (compiled out of Shipping)
  PawpocalypseEditor/   editor-only: DataTable/CSV importer, texture/atlas importer, validators, widget baker
```

**Three modules plus PawCore.** The KB's measured advice: more modules than that add build boundaries without a
second consumer `[KB U/01 §2.3]`. `PawCore` builds two ways: as an Unreal module (only `Core` for the build glue, no
UObjects) and as a standalone console test exe through a tiny CMake or `cl` script (`tools/build_pawcore_tests.ps1`),
so rule tests run in seconds without the editor.

## 2. Why a pure core (and what goes in it)

The Godot game keeps its rules in autoloads (`GameState` 2,976 lines, `CombatResolver`, `CardManager`,
`SkillTreeManager`) and its field simulation in a node tree (`TierController` 3,008 lines, `Enemy`, `Kitten`,
`Boss`). Porting the rules into an engine-free library buys:

- **The mirror test becomes trivial**: `pawsim.py` and `PawCore` read the same CSVs; a CLI built from PawCore runs
  the same fixture and the outputs are diffed (doc 09). The Godot `check_mirror` was the only gate that compared two
  implementations, and it caught real drift.
- **The soak can run headless at 1000x**, not just as a packaged build at 5x/20x (the packaged soak still exists, for
  engine errors and frame times).
- **Determinism**: fixed step, seeded RNG, no engine time.
- **Saves are the model**: the save writes PawCore's state, nothing else.

**PawCore owns:**

| Area | Content (Godot origin) |
|---|---|
| `PcData` | plain structs mirroring every CSV row (`PcTier`, `PcEnemy`, `PcBoss`, `PcUpgrade`, `PcSkillBranch`, `PcCard`, `PcJob`, `PcEvolution`, `PcAchievement`, `PcCombatConstants`, `PcReawakenConstants`) and a validator (ranges, cross references, the economy invariants of `check_resources`) |
| `PcCombat` | the resolver: `PawStrikeDamage`, `StrikesPerSec`, `PawDps`, `AuraDps`, `PlayerDps`, `KittenDps`, `PounceBurst`, `PounceCapScale`, `PoopDps`, `OverkillBounty` (`combat_resolver.gd`) |
| `PcStats` | `CombatStats` rebuilt from shop levels, tree, cards, job, evolution, Reawaken (`GameState._rebuild_stats`) |
| `PcEconomy` | prices, bulk buy, caps per floor/Reawaken, branch gating, Bait, Reawaken grant and multiplier, Legacy, gates (`GameState`) |
| `PcProgress` | the rules that CHANGE progress (`GameState`'s mutations): boss ledger (power vs this cycle's passage, the final-boss clock and ending flag), visual stage, Catnip/Stardust credited and spent, kills and the wave turnover (Compound, Reputation, Fence, Infestation), travel (banked kills, Second Helping's deferred half, deepest biome), bait, the shop purchase (single, bulk, preview), the Reawaken (incl. the run's cards and `PcJobs::ClearRunJobs`), the play clocks, New Game. The pure reads stay in `PcEconomy`/`PcStats` (evolution stage, waves, biome gates) |
| `PcCards` | XP track, eligibility, hand size, costs, effect totals (`CardManager`'s pure half). **`PcCardDraw`**: the deal (weighted, without replacement, deck in Godot's file order), Blueprint's promise, Prototype's mark, Fixation, auto-pick, reroll, banish, taking a card, the level-up that deals (`CardManager`'s stateful half). Every random number comes from a `PcCardRolls`: `PcRngCardRolls` (PcRng per seed and deal) in the game, Godot's own PCG32 stream in the tests. Deal state (hand, promise, mark, rerolls, banishes, Fixation) lives in `PcProgress` (the struct) |
| `PcHud` (2026-11-01) | The HUD's DECISIONS, no strings (hud.gd, tier_controller.gd's boss panel): the shop's row order and each row's kind (hidden, kitten-locked, MAX, price, bulk) and enabled state, the buy multipliers, the press; the Catches bar within the level; the zone plate (bait first, then next biome ready, then the Catnip gate); the wave line; the Pounce clock; the boss panel; the tab badges and visibility. The game module turns each into string-table text |
| `PcTree` | node levels, what the tree sells (required parent, rival fork, boss key, Reawaken gate), prices, purchase, free grants (`SkillTreeManager`). Nodes are keyed by branch id alone; the old `<branch>_<level>` keys only survive in v5 saves, migrated by the save port (`save_manager.gd` `_migrate_branch_levels`, phase 4) |
| `PcJobs` | jobs held, the run lock vs the standing preference, Union Rules' second job and share, every job term, the hat, `is_neutral`/`has_a_cost`, the picker's two rules (`GameState` jobs, `JobData`, `JobPicker.should_ask`). The one home of the job math: `PcStats`, `PcEconomy`, `PcCards` and the validator call it |
| `PcAchievements` | the roster in Godot's display order (the sorted `.tres` names, kept by the loader from `_godot_file`), each metric's progress value, the full re-scan that earns (returns the ids earned, in roster order), claims one by one and all at once through `GrantAchievementStardust`, the badge (unclaimed count and Stardust), the load (`apply_save_data`). Earned/claimed sets live in `PcProgress` (the struct). PawCore has no signals: the game module calls `EvaluateAchievements` after every change that can move a counter (`AchievementManager`) |
| `PcField` | **the realm simulation**: enemy pool (struct-of-arrays), spawning, movement verbs, shields, golden flee, kittens (seek/roam), zones (poop, crater, trails), Pounce, overkill splash, Prism beams, boss phases and adds, wave counters, at a fixed 60 Hz step (`TierController`, `Enemy`, `Kitten`, `Boss`, `BossAdd`) |
| `PcEvents` | an event queue the field and economy push to (`Kill{id, x, y, golden, crit}`, `Strike`, `PounceSlam`, `BossPhase`, `LevelUp`, `HandOffered`, `Purchase`...), drained by the game module each frame for VFX, audio and UI dirty flags. **As built**: the field's `PcFieldEvent`s (a kind, a slot, two points, a value, 32 flag bits: since phase 6 also Riddle, Threshold and Beam for the effects); the economy has no queue: the audio observes the progress (doc 07 §6) |
| `PcRng` | stateless hash RNG (seed, tick, entity id, salt) `[KB U/03 §4.2]` |
| `PcFormat` | number formatting rules that are game rules (`Amounts.short`) as plain strings; the game module wraps them in FText |
| `PcBackdrop` | phase 6: biome_backdrop.gd's layout (floor, patches, puddles, props, the title's garden), exact against Godot, with Godot's own RNG (`PcGodotRng`) and `hash()`; the one PawCore file allowed `float` (Godot's real_t) |
| `PcJuice` | phase 6 slice 5c: ui_juice.gd's state machine (hover, press, purchase punch, pop-in, the lean; one tween per property, each starting from the value it finds) and Godot's Tween easing (`TweenInterpolate`: linear, sine, quad, back, elastic x in, out, in-out, out-in), exact against Godot (fixture `juice`). Stepped by its owner; the game's `FPawJuice` writes it as a widget's render transform |
| `PcRainbow` | phase 6 slice 5d: rainbow_bar.gd (`PcRainbowBar`: the clock, the sparks and their spawn accumulator, the 48 bands) and rainbow_frame.gd (`RainbowFrameOutline`, `RainbowFramePhase`, `RainbowFrameAround`), and Godot's `Color.from_hsv`, `fposmod` and `Range.set_value` (`GodotRangeValue`: a ProgressBar's 0.01 step); the bar's sparks draw from `PcRng` (salt `RainbowSpark`); exact against the real scripts within float32 (fixture `rainbow`). The tree's sky (`PcSky`, skill_tree_sky.gd) lives in `PcBackdrop` (Godot's RNG, hash and float32 again): to the last float bit |
| `PcTween`, `PcUnicorn` | phase 6 slice 6a: Godot's Tween sequencing (`PcTween`: steps, parallel tweeners, delays, callbacks, intervals, the leftover running on; easing by `TweenInterpolate`) and unicorn_ceremony.gd (`PcUnicornCeremony`: the timeline, the fiesta's bits, skip and fade, what it draws), exact against the real script within float32 (fixture `unicorn`) |

**Status 2026-10-29**: built and proved against the real Godot code (doc 09 §7): `PcData` (+ generated `PcDataRows.h`,
`PcEnums.h`, loader `PcCsv`), the validator `PcValidate` (57 rules), `PcFormat`, `PcRng`, `PcCombat`, `PcStats`, `PcEconomy`
(prices, rewards, waves, biome gates, Reawaken, Bait, Legacy), `PcCards` + `PcCardDraw`, `PcTree`, `PcJobs`, `PcProgress`,
`PcAchievements`. The gameplay constants that live in Godot code are `PcGameData::Code` (`PcConstants.h`), loaded from
`Data/CodeConstants.csv` (doc 06 §5); a function that reads one takes the data. `PcEvents` became the field's own queue
(`PcFieldEvent`, drained by the game). **`PcField` is complete as a simulation (2026-10-31, docs/11 phase 2 slices 1-6)**:
the mice, the kittens, the aura, Snare and zones, Pounce, the paw's and the kill's extras, the Prism, the boss and its adds;
its pure parts (`PcFieldRules`) checked against the real Godot code, the rest pinned by rule tests and sabotages (doc 09 §7).
**The engine side is built (2026-10-31, slice 7)**: `FPawRealmSim` (the clock and the glue, no engine), `UPawRealmSubsystem`,
`APawRealmView` + `UPawSpriteRenderer`, `UPawGameSubsystem` (the data and the progress, thin until phase 3), the debug HUD and
the cheats (`PcCheats` + console commands); `Pawpocalypse.Realm.*` (doc 09 §7). Note the name: the STATE every rule reads is the struct
`Pc::PcProgress` in `PcStats.h` (levels, wallet, run state); the planned `PcProgress` module holds the rules that change it
(boss ledger, waves, passage). **Since**, each exact against its real Godot script (doc 09 §7): `PcHud` (2026-11-01, the
HUD's decisions), `PcTreeLayout` and `PcTreeText` (2026-11-02, the skill tree's shape and words), `PcSave` and `PcJson`
(2026-11-03, the save), `PcTitle`, `PcPause`, `PcSettings` (2026-11-04, phase 4's screens and settings.cfg), `PcDevice`
(2026-11-04, phase 5: input_device.gd), `PcBindings` (2026-11-05, phase 5 slice 4: input_bindings.gd, the remapping, and
settings.cfg's `[bindings]` in `PcSettings`), `PcControls` (2026-11-05, the Controls page: controls_panel.gd).

**What the game must do for `PcField`** (the field cannot see these events itself). Slice 7 wired the input (the cursor;
Pounce = the left mouse button or the right trigger past 0.5, read raw until phase 5), the drain, `TravelRequested`, travel,
the Reawaken's handling (`FPawRealmSim::OnReawakened`) and the bounds on a resize; `UPawRealmSubsystem::OnProgressChanged` is
the call for the purchases and tree levels phase 3 brings; `UPawRealmSubsystem::SetViewSize` / `SetDock` (2026-11-01) measure the field beside the dock's REAL rect (the HUD reports it; a press over it is never a Pounce: `IsOverDock`); `UPawRealmSubsystem::ChooseCard` (2026-11-01) is the card pick's: it
takes the card from the hand (`PcCardDraw` `ChooseCard`), rebuilds the field's stats, as Godot's `choose()` does, and
re-counts the kittens (F14, designer 2026-11-03: a kitten-cost card takes them at once; Auto-Card's pick does the same inside the
field). `StartRealm(Seed)` starts a fresh sim; a fixed seed replays a session (the tests' control and subject runs):
- Feed `PcFieldInput` every step: the cat's position (the cursor, or the pad-steered cat), `bPounceDown` (the Pounce action
  HELD: the field finds the press edge itself, so a pad trigger cannot re-fire on jitter), and the settings
  `bAutoPickSetting`, `bAutoTravelSetting`, `bAutoBaitSetting`.
- Call `SyncFieldKittens` where Godot re-counted the kittens: a Kitten Count purchase, a tree level, a Reawaken, a card taken
  (doc 01 §5 F14, since 2026-11-03). Call `RefreshFieldStats` after anything that changes the stats outside a kill. Call
  `DismissFieldBoss` (then `SyncFieldKittens`) on a Reawaken. `SummonFieldBoss` is the Summon button.
- Drain `PcFieldEvent`s every step: sounds and effects key off them, and `TravelRequested` (Slot = the biome index) is the
  field asking the game to travel (Auto-Travel, or after a first boss clear once the death sequence has played).
- Travel and Reawaken call `BeginField` again; nothing in a field is worth keeping across biomes. On a Reawaken whose
  starting biome (Territory, `PcEconomy::StartingBiomeIndex`) is deeper than the next one, Godot travels there after
  `boss_death_sequence_sec` (`TierController._on_reawakened()`): the game's to do, like every travel.
- On a resize or a dock-side change, `SetFieldBounds`: the mice, the kittens' roaming and the boss's relocations all read it.

Types: `double` for every economy and damage value (D13), `int64` for counts, `int32` for indices, `std::string` ids
(lower case), a small `PcVec2` (double x, y in field pixels). No `float` in PawCore at all: a test greps for it.

## 3. The game module: Godot autoloads -> Unreal classes

| Godot | Unreal | Kind | Notes |
|---|---|---|---|
| `PackLoader` | - | - | Unreal mounts paks itself; content chunking in doc 10 |
| `GameState` | `UPawGameSubsystem` | `UGameInstanceSubsystem` | As built: holds the data (`Pc::PcGameData`), the progress (`Pc::PcProgress`) and the slot's extras; no delegates (planned: owns the `PcGame` model; exposes delegates (`OnCurrencyChanged`, `OnUpgradePurchased`, `OnReawakened`, `OnBossDefeated`, `OnBaitChanged`, `OnWaveChanged`, `OnJobTaken`...) mirroring the Godot signals |
| `CombatResolver` | `PcCombat` (the planned `UPawCombatLibrary` BP wrapper is not built: no Blueprint reads damage) | pure functions | The one damage path |
| `SkillTreeManager` | part of `UPawGameSubsystem` over `PcTree` | | `branch_leveled_up` delegate |
| `SettingsManager` | `UPawSettingsSubsystem` over `PcSettings` (as built 2026-11-04; planned: `UPawGameUserSettings`) | `UGameInstanceSubsystem` | Godot's settings.cfg, field for field, in the saves' storage and sandbox (doc 08 §3): audio, video, language, dock side, prompt style, the stick switches, tutorial tips; the bindings are slice 4 of phase 5 (doc 05) |
| `InputDevice` | `UPawInputDeviceSubsystem` over `PcDevice` (built 2026-11-04) | `UGameInstanceSubsystem` (single player) | Every Slate event in: prompt family, cursor source, the connect toast, the warp echo, the hidden pointer, the focus prompt (doc 05 §4-5) |
| `AchievementManager` | `PcAchievements`, earned by the realm (`UPawRealmSubsystem`) and the HUD's page (built); `UPawAchievementSubsystem` waits for `IPawPlatform` | GameInstance subsystem | Forwards to `IPawPlatform` for Steam later |
| `CardManager` | part of `UPawGameSubsystem` over `PcCards` | | |
| `SaveManager` | `UPawSaveSubsystem` | GameInstance subsystem | Slots, autosave timer, request-save coalescing (doc 08) |
| `AudioManager` | `UPawAudioSubsystem` | GameInstance subsystem + `FTickableGameObject` | **Built (phase 6 slice 4)**: the event table and music from `DA_AudioBank`, rate limits, round robin, 16 voices, the buses from the settings, the crossfade; triggered by a progress observer, the realm's field events and the UI (doc 07 §6) |
| `BobaCameo` | `UPawBobaSubsystem` | GameInstance subsystem | `EnterMenu`/`LeaveMenu`/`Suppress` |
| `TierController` | `UPawRealmSubsystem` + `APawRealmView` | `UWorldSubsystem` + one actor | Drives `PcField` through `FPawRealmSim`; the view actor owns the camera and the sprite renderer and advances the realm from its own tick, right before it draws (a `UTickableWorldSubsystem` ticks after every actor, so the view would draw the previous frame's steps: a frame of latency between the cursor and the cat, for nothing) |
| `Player`, `Enemy`, `Kitten`, `Boss` | entities in `PcField`; drawn by `UPawSpriteRenderer` | not Actors | doc 04 §5 |
| `BiomeBackdrop`, the `vfx_*` / ring / zone / telegraph / sparkle nodes | `PcBackdrop` + `APawRealmView::DrawBackdrop` (the title's: `UPawGarden`); `FPawVfx` | not nodes: drawn each frame | doc 04 §5.1, §7 (phase 6) |
| `HUD` | `UPawRootLayout` + `UPawHudScreen` + dock pages | Common UI | Owned by `APawPlayerController`, never the GameMode `[KB U/03 §1]`. **Built 2026-11-01** (`Source/Pawpocalypse/UI`): `UPawRootLayout`, `UPawHudScreen` (status row, XP bar, Pounce clock, boss panel, the dock with the Upgrades page), `UPawButton`, `UPawPounceClock`, `PawText` (the string tables), `PawUiKit` (brushes, text, bars; since phase 6 slice 5a Godot's nine-patch chrome, `PawUi::Chrome`), `PawIcons` (slice 5b: `DA_Icons`, `PawUi::Icon`), `PawTokens.h` (generated colours), `PawUiConstants.h` (Godot's presentation constants); doc 05 §6.1 |
| `SkillTreeUI` | `UPawSkillTreeScreen` | Common UI activatable | doc 05 §6 |
| `MainMenu`, `PauseMenu`, `SettingsDialog`, `VictoryScreen`, ceremonies | `UPawMainMenu`, `UPawPauseMenu`, `UPawSettingsDialog`, `UPawVictoryScreen`, `UPawReawakenCeremony`, `UPawJobPicker`, `UPawCardPickScreen` (built) | activatable screens, each with its `WBP_*` | |
| `Tutorial` | `UPawTutorialOverlay` | widget, read-only | Reads views, writes only `tutorial_done` |
| `CheatOverlay` (F1) | `PcCheats` (the rules) + `paw.Cheat.*` console commands (`Dev/PawCheats.cpp`, built); the F1 panel `UPawCheatOverlay` in phase 3 | dev only | `#if !UE_BUILD_SHIPPING` |
| (Godot's debug readouts) | `APawDebugHud` (`paw.DebugHud`) | dev only | the GameMode's HUD class outside Shipping |
| `UiJuice` (static helper) | `FPawJuice` over `PcJuice` (built, phase 6 slice 5c) | a struct each screen owns and ticks (Common UI buttons do not tick) | Hover, press and punch from the buttons' own events, the lean and pop-in where Godot calls them; render transform about the centre, so layout never moves (doc 05 §7) |
| `RainbowBar`, `RainbowFrame`, `SkillTreeSky` | `UPawRainbowBar` (the Reawaken page's bar), `UPawRainbowFrame` (the zone plate's frame, following the plate from the HUD's root overlay), `UPawTreeSky` (over the tree's Sky border) over `PcRainbow` / `PcSky` (built, phase 6 slice 5d) | pieces with their own Widget Blueprints, painted in Slate (`PawUi::PaintCanvas`) | doc 05 §7 |
| `UnicornCeremony` | `UPawUnicornCeremony` over `PcUnicornCeremony` / `PcTween` (built, phase 6 slice 6a) | the skill tree's last part (`WBP_UnicornCeremony`), not a stack screen | Plays when the Unicorn is bought; eats every click, holds the keys (the tree buys nothing under it), skips after 0.6 s, hands the tree its focus back (doc 05 §6) |
| `Palette` (generated) | `PawTokens.h` (generated) | header | As built: `py tools/gen_paw_tokens.py` from Godot's generated `palette.gd` (planned: `tools/palette.py --cpp`, doc 07 §3) |

**Framework classes** stay thin: `APawGameMode` (sets classes; no state, no UI), `APawPlayerController` (input
contexts, the root layout, cursor), `UPawGameInstance` (level transition widget, startup checks). No `GameState`
replication; Unreal's `AGameStateBase` default is fine.

## 4. One map, no level loads during play

The Godot game frees and rebuilds the realm scene on every travel and Reawaken, and its worst frames (135-150 ms)
were those scene changes. In Unreal:

- **`L_Title`** (title screen) and **`L_Realm`** (the game). Both tiny. `L_Realm` is never reloaded during play.
- **Travel = `UPawRealmSubsystem::BeginBiome(id)`**: reset `PcField` to the new biome's data, swap the backdrop
  material/texture (preloaded async; biome textures are soft references loaded through `FStreamableManager` and the
  handle kept), keep everything else. Target: travel under 16 ms. A test measures it.
- Reawaken = the same path plus the model reset. **A realm can be rebuilt at any moment and nothing is lost**, because
  nothing worth keeping is in it (the Godot rule, kept).
- Title -> game is one `OpenLevel` behind a fade owned by the GameInstance (Cropout's pattern `[KB U/25]`).

## 5. Time: the fixed step

```
FPawRealmSim::Advance(RealDelta):                    // called by APawRealmView's tick, then it draws
    if (Paused) return;
    Accumulator += RealDelta * GameSpeed;            // GameSpeed: 1 in play; 5/20 in the soak; never World Time Dilation
    Steps = 0
    while (Accumulator >= Step && Steps < MaxStepsPerFrame /*8*/):
        Field.Step(Step /*1/60*/); Model.Step(Step)  // PawCore
        Accumulator -= Step; ++Steps
    if (Steps == MaxStepsPerFrame && Accumulator >= Step) Accumulator = 0   // spiral guard: only time really owed
                                                     // is dropped (counted in StepsDropped, on the debug HUD)
    Alpha = Accumulator / Step                       // presentation interpolates positions by Alpha
    DrainEvents()                                    // VFX, audio, UI dirty flags
```

- **The cursor/cat position is sampled once per frame** and handed to the field as input for the next steps; the paw
  strike resolves inside the step (at most 12 strikes per frame: keep the cap as a per-step budget).
- **Presentation interpolates** between the previous and current step positions (so 60 Hz sim looks smooth at 144 Hz).
- **The HUD refreshes at 15 Hz** from dirty flags; things the player causes update immediately (the Godot rule).
- **Paused** (pause menu, skill tree, card pick): the accumulator stops. UI animation while paused runs on `FTSTicker`,
  because world timers stop while paused `[KB U/08 §2.13, U/09 §3]`.
- **Play clock** (`play_seconds_this_cycle`, `total_play_seconds`, `final_boss_seconds`) counts sim time while playing,
  never menus.

## 6. Data at runtime

- DataTables (`DT_TierData`, `DT_EnemyData`, ... one per CSV, `DT_CodeConstants` included) imported headless from
  `Data/*.csv` (doc 06 §4, §9). Their rows hold each cell as TEXT; `FPawDataAdapter::BuildGameData` rebuilds the CSV text,
  parses it with PawCore's own `Pc::BuildGameData` and runs the **same validator** pawsim's checks use (built and proven equal
  to the CSV route, 2026-10-29). `UPawGameSubsystem::Initialize` calls it at startup (built 2026-10-31): development builds
  log every problem as an error, Shipping as a warning `[KB U/05 §2]`. In every build a problem leaves the game WITHOUT data
  (`bDataReady` false: no new game, no realm), never running on rules the validator refused; the gates catch it long before
  a package (2026-11-04: the code always did this; this line used to read as if Shipping played on).
- **Ids are lower-case `FString`/`std::string`, never `FName`-as-identity** (FName case differs between editor and
  packaged builds; a `FindRow` miss and a raw `{Name}` placeholder appeared only in packaged builds) `[KB U/05 §3]`.
  Row names may be FNames because DataTables require it, but lookups go through `IdFromName()` (lower-cases) and a
  test loads every table in a cooked build.
- **Visual definitions** (texture, atlas rect, tint, sprite size, sounds) are separate from rules: rules rows name an
  id; a `UPawVisualDefinition` primary data asset per creature/boss/cat stage maps id -> art `[KB U/04 §5]`. Swapping
  art never touches rules. **As built (phase 6)**: no per-creature asset yet; the art is found by the stem the data names
  (`texture_path` -> an atlas cell of `DA_FieldAtlas`), sizes and tints from the data's columns and Godot's code constants
  (`Realm/PawFieldLook.h`); every atlas page, floor and sound is referenced by the realm view or a bank, loaded with the map.
- **Never load an asset per frame or per kill** (the Godot `load()` lesson: 1.8 ms each, 90% of a 4 fps report).
  Everything the realm draws is resolved when a biome begins.

## 7. Engine settings for this project (phase 0 sets them)

The template enabled 3D features this game never uses. `Config/DefaultEngine.ini`:

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Pawpocalypse/Maps/L_Title.L_Title
EditorStartupMap=/Game/Pawpocalypse/Maps/L_Realm.L_Realm
GameInstanceClass=/Script/Pawpocalypse.PawGameInstance
GlobalDefaultGameMode=/Script/Pawpocalypse.PawGameMode

[/Script/Engine.RendererSettings]
r.DynamicGlobalIlluminationMethod=0      ; no Lumen (template had 1)
r.ReflectionMethod=0                      ; none
r.RayTracing=False                        ; template had True; unused RT/Substrate shaders caused E_OUTOFMEMORY PSO crashes [KB U/12 §1.3]
r.RayTracing.RayTracingProxies.ProjectEnabled=False
r.Substrate=False                         ; template had True
r.Nanite.ProjectEnabled=False             ; ~22-30 MB of shaders saved [KB U/12 §1.3]
r.GenerateMeshDistanceFields=False
r.Shadow.Virtual.Enable=0                 ; no shadows from lights at all; ground shadows are sprites
r.AllowStaticLighting=False
r.DefaultFeature.AutoExposure=False       ; exposure must never move (doc 04 §3)
r.DefaultFeature.LocalExposure.HighlightContrastScale=1.0   ; 1.0 = no local tone change (engine default); the template had 0.8
r.DefaultFeature.LocalExposure.ShadowContrastScale=1.0
r.DefaultFeature.MotionBlur=False
r.DefaultFeature.Bloom=False
r.DefaultFeature.AmbientOcclusion=False
r.DefaultFeature.LensFlare=False
r.AntiAliasingMethod=1                    ; FXAA, chosen by spike S1 (doc 04 §10); no AA vs FXAA open as Q9 (doc 01)
r.SkinCache.CompileShaders=False

[/Script/Engine.Engine]
GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient

[/Script/Engine.UserInterfaceSettings]
bAllowHighDPIInGameMode=True
UIScaleRule=ShortestSide                  ; 1080 -> 1.0, linear: the UI matches Godot's canvas_items stretch (doc 04 §6)
```

Remove from the `.uproject` any plugin the game does not use for Game targets (`TargetAllowList`), keep `CommonUI`,
`EnhancedInput`, `PythonScriptPlugin` + `EditorScriptingUtilities` (editor), `Paper2D` only if spike S1 picks it. The
`[ConsoleVariables]` CommonUI lines belong in `DefaultEngine.ini` `[KB U/03 §6]`; the 5.8.3 template had written them to
`DefaultGame.ini` (ignored there), phase 0 moved them.

**As set in phase 0 (2026-10-28)**, `Config/DefaultEngine.ini` / `DefaultGame.ini` carry the block above (the AA line since S1),
plus `bUseSplitscreen=False`, `+DirectoriesToAlwaysCook=(Path="/Game/Pawpocalypse")`, `+DirectoriesToNeverCook` for
`/Interchange` and `/NNEDenoiser` (added in spike S2: with those plugins editor-only, the cook failed on their content, which
engine config still references), and `[SystemSettings] r.ScreenPercentage.Default.Desktop.Mode=0`,
`r.ScreenPercentage.Default=100` (added in spike S3: the desktop default rendered 1080p at 810p and upscaled), `CommonInputSettings bEnableEnhancedInputSupport=True` and
Enhanced Input `bEnableUserSettings=True` (in `DefaultInput.ini`: the class is `config=Input`) (doc 05 §1). **Since 2026-11-05
`ProceduralMeshComponent` is enabled for the game too** (the backdrop's patches and puddles, doc 04 §5.1; `GAME_PLUGINS` in
`tools/trim_game_plugins.py`). The `.uproject` restricts **154 (now 153) default-enabled engine plugins to the
Editor target** (69 runtime plugins the game never uses: mobile, MetaHuman/ControlRig/IK, cloth, PCG, Datasmith/glTF/Interchange,
media players, MsQuic/Oodle network...; plus every editor-only and content-only plugin, because those pull runtime plugins such as
ControlRig and RigVM back into the game through their dependencies). Kept for the game: CommonUI, EnhancedInput, ProceduralMeshComponent (since phase 6), Niagara (unused: the effects are draw calls),
MetaSound and the audio plugins, Paper2D (until S1/S2), OnlineSubsystem/Null, XInputDevice, the device-profile selector,
RenderDoc/PIX (dev captures). Result: the game exe links 34 plugin folders instead of 64; Shipping exe 112 MB unpackaged.
Check what really ships with the link response file
`Intermediate/Build/Win64/x64/Pawpocalypse/<Config>/Pawpocalypse.exe.rsp`, not the `.target` receipt (that lists every enabled
plugin, code or not). To use a restricted plugin in the game, delete its `TargetAllowList`.

## 8. Coding rules

- Epic coding standard `[KB U/02]`; braces on new lines, tabs; `b` bools; `double` for economy; `int64` from
  `FMath::FloorToInt` (returns int64 in 5.8; cast explicitly `[KB U/01 §1.2]`).
- **Comments carry the reasoning, not the restatement**, as in the Godot codebase: when porting a function, port its
  "why" comment too (the Godot comments explain the measured incident behind each number). This is how the next
  agent avoids re-breaking it.
- Every `UObject*` member is a `UPROPERTY` (`TObjectPtr`); weak pointers for observers; delegates unbound in
  `EndPlay`/`Deinitialize`.
- Timer lambdas that clear their own timer do it as the last statement `[KB U/02 §4.1]`.
- No `#if PLATFORM_*` in gameplay; platform code behind `IPawPlatform`.
- Unity builds are OFF in PawCore and Pawpocalypse (`bUseUnity = false`, 2026-11-04): regrouping broke files nobody
  touched (HANDOFF-LOG (35)) `[KB U/01 §2.5]`. Unique names for file-local helpers still help a reader; unique header names.
- Live Coding only for `.cpp` body changes; close the editor and build for headers, `UPROPERTY` and new files; never Hot
  Reload `[KB U/01 §2.2]`.
- Source files UTF-8 without BOM; scripts preserve line endings `[KB U/02 §5]`.
