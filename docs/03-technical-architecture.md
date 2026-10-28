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
    Realm/                the realm world subsystem (drives PawCore's field sim), its view actor
    Render/               the sprite renderer, shadows, VFX primitives (doc 04)
    UI/                   Common UI screens, the dock, the tree, widgets kit, tokens (doc 05)
    Input/                input device subsystem, glyphs, bindings (doc 05)
    Save/                 save container, slots, migrations (doc 08)
    Audio/                audio event subsystem (doc 07)
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
| `PcData` | plain structs mirroring every CSV row (`PcTier`, `PcEnemy`, `PcBoss`, `PcUpgrade`, `PcSkillNode`, `PcCard`, `PcJob`, `PcEvolution`, `PcAchievement`, `PcCombatConstants`, `PcReawakenConstants`) and a validator (ranges, cross references, the economy invariants of `check_resources`) |
| `PcCombat` | the resolver: `PawStrikeDamage`, `StrikesPerSec`, `PawDps`, `AuraDps`, `PlayerDps`, `KittenDps`, `PounceBurst`, `PounceCapScale`, `PoopDps`, `OverkillBounty` (`combat_resolver.gd`) |
| `PcStats` | `CombatStats` rebuilt from shop levels, tree, cards, job, evolution, Reawaken (`GameState._rebuild_stats`) |
| `PcEconomy` | prices, bulk buy, caps per floor/Reawaken, branch gating, Bait, Reawaken grant and multiplier, Legacy, gates (`GameState`) |
| `PcProgress` | boss ledger, evolution stage, visual stage, waves, biome unlocks, passage per cycle, final boss |
| `PcCards` | XP track, hand dealing (seeded), eligibility, costs, effects state (`CardManager`) |
| `PcTree` | node levels, parent rules, unlocked vs owned, key parsing from the right (`SkillTreeManager`) |
| `PcJobs`, `PcAchievements` | job pick/run state; metrics and claims |
| `PcField` | **the realm simulation**: enemy pool (struct-of-arrays), spawning, movement verbs, shields, golden flee, kittens (seek/roam), zones (poop, crater, trails), Pounce, overkill splash, Prism beams, boss phases and adds, wave counters, at a fixed 60 Hz step (`TierController`, `Enemy`, `Kitten`, `Boss`, `BossAdd`) |
| `PcEvents` | an event queue the field and economy push to (`Kill{id, x, y, golden, crit}`, `Strike`, `PounceSlam`, `BossPhase`, `LevelUp`, `HandOffered`, `Purchase`...), drained by the game module each frame for VFX, audio and UI dirty flags |
| `PcRng` | stateless hash RNG (seed, tick, entity id, salt) `[KB U/03 §4.2]` |
| `PcFormat` | number formatting rules that are game rules (`Amounts.short`) as plain strings; the game module wraps them in FText |

Types: `double` for every economy and damage value (D13), `int64` for counts, `int32` for indices, `std::string` ids
(lower case), a small `PcVec2` (double x, y in field pixels). No `float` in PawCore at all: a test greps for it.

## 3. The game module: Godot autoloads -> Unreal classes

| Godot | Unreal | Kind | Notes |
|---|---|---|---|
| `PackLoader` | - | - | Unreal mounts paks itself; content chunking in doc 10 |
| `GameState` | `UPawGameSubsystem` | `UGameInstanceSubsystem` | Owns the `PcGame` model; exposes delegates (`OnCurrencyChanged`, `OnUpgradePurchased`, `OnReawakened`, `OnBossDefeated`, `OnBaitChanged`, `OnWaveChanged`, `OnJobTaken`...) mirroring the Godot signals |
| `CombatResolver` | `PcCombat` (+ `UPawCombatLibrary` read-only BP wrapper for widgets) | pure functions | The one damage path |
| `SkillTreeManager` | part of `UPawGameSubsystem` over `PcTree` | | `branch_leveled_up` delegate |
| `SettingsManager` | `UPawGameUserSettings` (subclass of `UGameUserSettings`) | per-device ini | Audio, video, language, dock side, prompt style, tutorial tips; bindings via Enhanced Input user settings (doc 05) |
| `InputDevice` | `UPawInputDeviceSubsystem` | `ULocalPlayerSubsystem` | Wraps `UCommonInputSubsystem`: prompt family, cursor source, field owner, pointer warp (doc 05) |
| `AchievementManager` | `UPawAchievementSubsystem` | GameInstance subsystem | Forwards to `IPawPlatform` for Steam later |
| `CardManager` | part of `UPawGameSubsystem` over `PcCards` | | |
| `SaveManager` | `UPawSaveSubsystem` | GameInstance subsystem | Slots, autosave timer, request-save coalescing (doc 08) |
| `AudioManager` | `UPawAudioSubsystem` | GameInstance subsystem | Event table, rate limits, buses (doc 07) |
| `BobaCameo` | `UPawBobaSubsystem` | GameInstance subsystem | `EnterMenu`/`LeaveMenu`/`Suppress` |
| `TierController` | `UPawRealmSubsystem` + `APawRealmView` | `UTickableWorldSubsystem` + one actor | Drives `PcField`; the view actor owns the camera, backdrop, sprite renderer components |
| `Player`, `Enemy`, `Kitten`, `Boss` | entities in `PcField`; drawn by `UPawSpriteRenderer` | not Actors | doc 04 §5 |
| `HUD` | `UPawRootLayout` + `UPawHudScreen` + dock pages | Common UI | Owned by `APawPlayerController`, never the GameMode `[KB U/03 §1]` |
| `SkillTreeUI` | `UPawSkillTreeScreen` | Common UI activatable | doc 05 §6 |
| `MainMenu`, `PauseMenu`, `SettingsDialog`, `VictoryScreen`, ceremonies | activatable screens | | |
| `Tutorial` | `UPawTutorialOverlay` | widget, read-only | Reads views, writes only `tutorial_done` |
| `CheatOverlay` (F1) | `UPawCheatOverlay` | dev only | `#if !UE_BUILD_SHIPPING` |
| `Palette` (generated) | `PawTokens.h` (generated) | header | `py tools/palette.py --cpp` (doc 07 §3) |

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
UPawRealmSubsystem::Tick(RealDelta):
    if (Paused) return;
    Accumulator += RealDelta * GameSpeed;            // GameSpeed: 1 in play; 5/20 in the soak; never World Time Dilation
    Steps = 0
    while (Accumulator >= Step && Steps < MaxStepsPerFrame /*8*/):
        Field.Step(Step /*1/60*/); Model.Step(Step)  // PawCore
        Accumulator -= Step; ++Steps
    if (Steps == MaxStepsPerFrame) Accumulator = 0   // spiral-of-death guard; log it in dev
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

- DataTables (`DT_Tiers`, `DT_Enemies`, ... one per CSV) imported headless from `Data/*.csv` (doc 06). At startup the
  adapter `FPawDataAdapter` copies every row into `PcData` and runs the **same validator** pawsim's checks use;
  development builds fail loudly, Shipping logs and clamps `[KB U/05 §2]`.
- **Ids are lower-case `FString`/`std::string`, never `FName`-as-identity** (FName case differs between editor and
  packaged builds; a `FindRow` miss and a raw `{Name}` placeholder appeared only in packaged builds) `[KB U/05 §3]`.
  Row names may be FNames because DataTables require it, but lookups go through `IdFromName()` (lower-cases) and a
  test loads every table in a cooked build.
- **Visual definitions** (texture, atlas rect, tint, sprite size, sounds) are separate from rules: rules rows name an
  id; a `UPawVisualDefinition` primary data asset per creature/boss/cat stage maps id -> art `[KB U/04 §5]`. Swapping
  art never touches rules.
- **Never load an asset per frame or per kill** (the Godot `load()` lesson: 1.8 ms each, 90% of a 4 fps report).
  Everything the realm draws is resolved when a biome begins.

## 7. Engine settings for this project (phase 0 sets them)

The template enabled 3D features this game never uses. `Config/DefaultEngine.ini`:

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Pawpocalypse/Maps/L_Title.L_Title
EditorStartupMap=/Game/Pawpocalypse/Maps/L_Realm.L_Realm
GameInstanceClass=/Script/Pawpocalypse.PawGameInstance

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
r.DefaultFeature.MotionBlur=False
r.DefaultFeature.Bloom=False
r.DefaultFeature.AmbientOcclusion=False
r.DefaultFeature.LensFlare=False
r.AntiAliasingMethod=<spike S1 decides>   ; doc 04 §4
r.SkinCache.CompileShaders=False

[/Script/Engine.Engine]
GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient

[/Script/Engine.UserInterfaceSettings]
bAllowHighDPIInGameMode=True
UIScaleRule=ShortestSide                  ; 1080 -> 1.0, linear: the UI matches Godot's canvas_items stretch (doc 04 §6)
```

Remove from the `.uproject` any plugin the game does not use for Game targets (`TargetAllowList`), keep `CommonUI`,
`EnhancedInput`, `PythonScriptPlugin` + `EditorScriptingUtilities` (editor), `Paper2D` only if spike S1 picks it. The
template's `[ConsoleVariables]` CommonUI lines are already in `DefaultEngine.ini` (correct file `[KB U/03 §6]`).

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
- Unique names for file-local helpers (unity-build collisions `[KB U/01 §2.5]`), unique header names.
- Live Coding only for `.cpp` body changes; close the editor and build for headers, `UPROPERTY` and new files; never Hot
  Reload `[KB U/01 §2.2]`.
- Source files UTF-8 without BOM; scripts preserve line endings `[KB U/02 §5]`.
