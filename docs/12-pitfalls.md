# 12 - Pitfalls: what already went wrong (Godot), what will go wrong (Unreal), and the guard for each

> Read before every phase. Sources: the Godot handoff's "Traps" sections and `pawpocalypse.md`, `Knowledge/Unreal/24` (master
> gotcha list), `Knowledge/2D/01-02`. Each row names the guard (a test, a rule, a setting). When a new one bites, add it here
> AND to the knowledge base (technical, no project names).

## 1. Design and economy traps (engine-independent, they will come back in C++)

| Trap (Godot incident) | Guard here |
|---|---|
| Rewards grew x1.35/tier against HP x3.33/tier: the game became uncompletable (551x divergence) | Validator: `reward = 0.0433 x hp` |
| Paw Damage at effect 1.12 / cost 1.08: DPS to 2.4e10 | Validator: `cost_growth > effect_per_level`; summed exponent < 0.80 |
| A term that does not track the main curve disappeared (rewards, kittens, poop, Pounce, bounty: four times) | Doc 02 §3 rule; every new term priced off `player_dps()` or the victim's reward |
| A shorter ladder silently lost its top (evolution base, weapon multiplier: 16x nerfs nearly shipped) | Test asserts stage 7 = x128 and weapon x1.40 |
| Pounce uncapped measured 90.6% of all damage | Product cap test |
| Overkill chaining would clear the board every frame | "never chains" test |
| Stardust multiplier keyed to the balance made the tree a trap | Keyed to `stardust_granted_ever`; test buys a node and checks the multiplier did not fall |
| A refused boss summon ate the bait | Test |
| Boss timeout auto-travelled the player back into the fight | Timeout turns auto-travel off; test |
| A DPS-coupled spawner made density a property of the cat | Spawn rate test: Paw Damage 200 changes the rate by exactly nothing |
| Kitten pool sized from the base cap: deep runs paid for invisible kittens | Pool-grows-to-owned test |
| Three skill branches unbuyable for a day while every gate passed | Playthrough test buys through the real UI path; tree seating test both directions |
| Achievement Stardust never granted for a whole version | Test asserts the BALANCE moves |
| `aura_power_3` parsed from the left = the Reawaken crash | Gone in Godot since 2026-08-21: nodes are keyed by branch id alone, nothing is parsed. Only v5 saves still carry old ids; the save port migrates them (`_migrate_branch_levels`), tested with multi-word ids (phase 4) |
| New Game left the previous slot's jobs, kills, clock and run state | New-game-clears-everything test over every save field |
| `begin_biome()` early-returned on a stale id, so a restore was a no-op | `BeginBiome` always resets when called by load/Reawaken; test |
| Bulk buy wrote a save per level (137 ms freeze) | Coalesced `RequestSave()`; test counts writes |
| A hidden 41-row list refreshed 15 Hz (39 ms/s) | Hidden pages idle; test |
| A gate passed for days while proving nothing | Negative-test every assertion; confirm the sabotage landed |
| A repro script committed a real Reawaken to the designer's save | Every automated run sandboxed; tests refuse to start unsandboxed |
| Length was tuned toward a number of hours | Length is reported, never targeted |

## 2. Engine-change traps (Godot -> Unreal)

| Trap | Why it bites in the port | Guard |
|---|---|---|
| **32-bit `float`** | Godot `float` is 64-bit; C++ `float` is 32-bit (7 digits). Catnip passes 1e12, damage spans ten orders of magnitude | `double` everywhere in PawCore; L0 grep for `float`; precision round-trip test |
| **Enum by index** | Godot stores indices; reordering repointed saves | Enums stored by NAME in CSV and saves; never rename a value |
| **Per-frame `delta` game logic** | Godot's variable delta made 20x soaks lie about pace | Fixed step; frame-rate independence test |
| **Scene rebuild per travel** | Godot's worst frames (135-150 ms) | One map; `BeginBiome` swaps data; travel < 16 ms test |
| **`load()` per kill** | 1.8 ms each in Godot; Unreal sync loads hitch worse | Everything resolved at biome start; soft refs async-loaded with the handle kept |
| **Export-only bugs** (`.remap` scan loaded zero evolution stages) | Unreal's version: assets not cooked, FName case differs, loose files not staged | Always-cooked root, lower-case ids, staged folders, packaged gates every phase |
| **Hidden focus presses twice** (Godot 4.6+) | Slate keeps keyboard focus on a clicked button too | Confirm moves focus off the button; nothing double-handles `press` |
| **`MOUSE_FILTER_STOP` over the field ate Pounce** | A visible full-screen UMG widget eats clicks the same way | Field overlays `HitTestInvisible`; test clicks through every overlay |
| **Godot's pad had no `ui_accept`** | Unreal: Common UI needs `CommonGameViewportClient` and input data or the pad does nothing | Phase 0 config; pad tour |
| **Godot comments carry the reasoning** | A port that drops comments loses why each number is what it is | Port the "why" comment with each function (doc 03 §8) |

## 3. Unreal traps (from the knowledge base, verified on 5.8.3 unless marked)

| Symptom | Cause | Fix |
|---|---|---|
| Pad does nothing / input goes to the world | Viewport client not `CommonGameViewportClient` | ini (doc 05 §1) |
| Pad A never reaches gameplay | UI context consumes A; or HUD capture `..._IncludingInitialMouseDown` turns A into a click | UI actions don't consume; `CaptureDuringMouseDown` for this free-cursor game |
| Only double-clicks work | `NoCapture` | `CaptureDuringMouseDown` |
| Common UI cvars ignored | In `DefaultGame.ini` | `DefaultEngine.ini [ConsoleVariables]` (already right) |
| Glyphs empty | Controller data name mismatch; legend built before mappings applied | Match names; redraw on `ControlMappingsRebuiltDelegate` |
| PlayStation pad dead on Windows | XInput only | Steam Input, or the GameInput plugin (decide in phase 5) |
| D-pad never reaches `NativeOnKeyDown` | Common UI analog cursor consumes it | Explicit navigation rules |
| Opening Settings over Pause unpauses | Pause screen unpauses on deactivate | Track "covered by child" |
| UI animations freeze while paused | World timers stop | `FTSTicker` |
| Screen shows old state | Common UI pools screens | Reset in `SetTarget*` |
| Machine stays remapped after a test | Enhanced Input user settings saved | Reset in tests |
| White box drawn | Default `FSlateBrush` | No-draw brush |
| Variable font renders wrong | Slate does not support variable fonts | Static faces |
| Headless font import asserts `CurrentApplication.IsValid()` | `UFontFace::PostEditChangeProperty` flushes Slate's font cache with no Slate app | Make the face in C++: `NewObject<UFontFace>` + `InitializeFromBulkData` (`UPawFontBuilder`, KB U/09 §5.1) |
| One language's name drawn in two weights (简体中文) | Its culture's sub-font covers some characters (M PLUS has 体中文, not 简); the rest fall to another face | A culture-less Han sub-font sends every Han character to one face; `check_font_coverage.py` models Slate's choice |
| A font walker reports a rich text's engine font after the migration | The rich text's `DefaultTextStyle` is Transient, rebuilt from its style set | Walkers skip `CPF_Transient` properties |
| A graft `swap:` leaves a panel EMPTY in the asset (R4c: the title's hint plate without its prompts) | The copy is the part alone; its children stay in the code layout | `swap:` now refuses a part with children: `remove:Old;add:New` (add copies the subtree) |
| A graft `add:` fails "the asset already has HorizontalBox_0" | An unnamed inner widget's auto-name collides with one the asset holds | Name every widget a graft will add; a graft is all or nothing (one FAILED op saves nothing) |
| `paw.UiScale` / a UI-size test changes nothing on screen | `FSlateApplication::SetApplicationScale` does not reach the game viewport | UMG's `UUserInterfaceSettings::ApplicationScale` (on top of the DPI curve); `godot_tour.py --uiscale 1.5` |
| A code change to a text's colour never shows, and its test passes | The test reads the shipped asset, which saved the colour as a value | `.Widgets.Parity` compares each named text's colour between asset and code; graft `text:Part` |
| A layout tweak "worked" in the tour but the screen did not change | `build/tour/ue` held old shots the tour never retook (title, pause, settings until R4c) | `godot_tour.py` deletes every shot before retaking it and shoots the menus |
| A recolour turns frames light, or text dark | One old colour became two (the ink: text Bone, frames ink); a map by value is ambiguous | Migrate by ROLE: `PawUi::IsTextColour`, `PawUi::IsEdgeColour` (R3's shapes), `bake_widgets.py --recolour` (text, surface and edge maps) |
| `restyle:Border_2` fails ("the code layout has no Border_2") or restyles the wrong plate | An unnamed part's auto-name in code drifted from the one it was baked with (the HUD's dock panel: `Border_0` in code, `Border_2` in the asset) | List both: `--graft "Class=names:x;codenames:x"` (each widget with its parent), then `restyle:CodeName>AssetName` (refused when the class or the parent's class differs) |
| A plate's shadow cannot be drawn | A rounded-box brush draws inside its geometry only | The nine-patch chrome with the shadow inside the bottom margin, `PawUi::PlatePadding` (KB U/09 §4) |
| Asset null only in packaged build | Referenced only from C++ defaults/ini/soft refs | Always-cooked root, Asset Manager types |
| `{Name}` placeholder raw in packaged build | FName arg names change case | `FString` arg names |
| "Unknown cook failure" | A Blueprint with compile errors | Compile all BPs before packaging |
| New exe not in the package, UAT says success | A running packaged copy locks the exe | Script refuses while it runs |
| Crash only in packaged build after a timer | Timer lambda cleared itself then used captures | Clear as last statement |
| `E_OUTOFMEMORY` creating a PSO at the menu | Unused RT/Substrate shaders | RT and Substrate off (the template had them ON) |
| Blueprint corruption after a code change | Live Coding a header change / Hot Reload | Close editor, full build |
| Sudden "redefinition" errors after adding a file | Unity-build regrouping of same-named file-local helpers | Unique names |
| Narrowing error on `FloorToInt` | Returns `int64` in 5.8 | Explicit cast |
| Headless Python did nothing | `-script=` with backslashes | Absolute forward-slash path |
| Headless delete vanished an asset | Editor git provider ran `git rm` | `-SCCProvider=None` |
| `/Game/...` became `C:/Program Files/Git/Game/...` | Git Bash path conversion | `MSYS_NO_PATHCONV=1` or PowerShell |
| Editor run killed mid-way | `Select-Object -First N` in a PowerShell pipeline | Pipe to `Out-Null`, read the log |
| Bash tool fails before running | Apostrophe in a quoted heredoc | Write scripts to files (this project's content has apostrophes everywhere) |
| Two test runs corrupt each other | Concurrent editor sessions | One at a time |
| A sabotage run reports MISSED and the clean build FAILS for every test at once | Another Unreal process started mid-run (another project's soak, 2026-10-31): the tools refuse and nothing ran | Never kill what is not ours. Fixed 2026-10-31: the guard (`tools/unreal_procs.py`) ignores a process that names another project's `.uproject`; one of this project, or naming none, still blocks |
| A sabotage reads MISSED though the test would fail | The test crashed the editor (it faked a state the field cannot have: a boss in a biome with none) and the runner counts "no test ran" as not caught | Build test states through the real entry points (summon, travel), never by setting flags |
| A game-module sabotage or a PawCore one fails the nightly though nothing broke | Its target line was changed by later work: the sabotage no longer names code that exists (9 realm ones found 2026-11-04, 1 PawCore one 2026-11-05, 1 realm one 2026-11-06: the realm gate refused and ran nothing) | `py tools/sabotage_realm.py --list` / `sabotage_pawcore.py --list` print STALE targets without building; since 2026-11-06 the everyday `check_all` runs both (gate `sabotage targets current`); retarget and run the fixed one with `--only` |
| A UI sabotage is MISSED though its test asserts the property | The line is in `Initialize`'s `BuildsInCode` branch, which since the Widget Blueprints runs only when `bake_widgets.py` builds the asset: the property is the asset's, a code edit cannot reach it (6 retired 2026-11-06) | Sabotage the asset instead (edit a scratch copy headless, run the test, restore); a code sabotage only for code that runs in the game |
| The package fails to compile though the editor builds | Editor-only API (`WITH_EDITORONLY_DATA` fields such as a texture's `Source`, `WITH_EDITOR` calls) in game-module code, tests included | `check_all` builds the game target too (`UBT game build`); wrap editor-only checks in `#if WITH_EDITORONLY_DATA` |
| A designer's Widget Blueprint edits lost | Re-baking a class whose asset exists | Never re-bake; new parts go in with `bake_widgets.py --graft` (doc 05 §5, CLAUDE.md) |
| A sabotage of a Godot rule is MISSED although the fixture is exact | The branch is unreachable from the defaults (a rebind onto a row with nothing on that device: only a hand-edited settings.cfg makes one), so random ops from the defaults never visit it | Start some fixture rows from a hand-edited state and assert the branch's count (HANDOFF-LOG (47)) |
| A `<=` -> `<` sabotage is MISSED | The fixture's random values never land exactly on the edge (a 5 s timeout ticked in 0.5 / 1 / 2 / 4.9 / 5.1 s steps) | Put the edge itself in the fixture's value list and assert how often it is hit (HANDOFF-LOG (48)) |
| WASD sits on the wrong keys on AZERTY, the zoom keys one key off on a Turkish Q keyboard | Unreal's `FKey` is the layout's virtual key, not the key's position (Godot binds positions) | Map every key with a place through the active layout (`Input/PawKeyCodes`); tests pin a US layout (`-PawUsKeyboard`) (HANDOFF-LOG (49)) |
| Pad A presses a dock row twice | Common UI's analog cursor turns pad A into a simulated click before any widget; the dock's own poll pressed too | The device subsystem's input processor is registered FIRST and consumes the dock's keys while it is active |
| The selection does not come back to Settings after the dialog closes | Common UI re-applies the screen's desired focus target when the modal above closes, overriding a `SetFocus` | `NativeGetDesiredFocusTarget` returns the button to come back to |
| A sabotage reports NOBUILD (an unused parameter) | Deleting the only use of a parameter is C4100 under `/WX` | Weaken the use instead (`(!b && Dt < 0.0)`) |
| A sabotage reports NOBUILD | `if (true \|\| ...)` is a constant condition: C4127 under `/WX` | Sabotage a statement instead (`A = B;` -> `B.clear();`) |
| Creatures drawn in the wrong order (a mouse over a kitten) | The plan assumed a Y-sort; Godot draws by `z_index`, then scene-tree order (phase 6) | `PawLook::Order` (z, then tree) and a View test that kittens sit over every mouse, the cat over both |
| Tints and translucency a shade off | A Godot `modulate` multiplies sRGB values (2D, no HDR); Unreal multiplies linear | The linear tint is `m^2.2` (`PawLook::Modulate`); translucent blending stays linear (a known small difference, doc 04 §5.1) |
| A ported Godot layout drifts by an ulp | Godot's `Vector2` / `Rect2` are float32, its script numbers double | Round to float exactly where Godot makes a Vector2 (`PcBackdrop`, the one L0 `FLOAT_IS_THE_SPEC` exception); the fixture compares to the float |
| A replayed Godot random layout differs | A `RandomNumberGenerator` object's `randf()` takes two outputs in float32, unlike the global `randf()`; the GLOBAL `randf_range` is `randd()`: three outputs, in doubles (a shadow RNG drifted after one draw, 2026-11-06); seeds come from Godot's own `hash()` | `PcGodotRng`, `GodotHashString`, `GodotHashBiomeConfig`, proven on the `backdrop` fixture (KB Godot/03) |
| An effect lands at twice its place | Godot: `global_position` set before `add_child` is a plain position; under a moved parent it doubles (F39) | Copy Godot until decided; measured in Godot itself |
| One sound plays twice, layered | Godot plays it from a signal hookup AND from the code beside the emit; a zero interval lets both through (F40) | Copy Godot until decided (`Pawpocalypse.Audio.When` asserts the doubles) |
| Effects (or any sprite) vanish above a certain height | The ortho camera's automatic clip planes cut world Z 400 with the camera at Z 1000; explicit planes clipped more (unexplained) | Keep every band in Z 0..300; `Pawpocalypse.Realm.Vfx.Canvas` asserts it (doc 04 §6) |
| A test crashes the editor and the run says PASS | The runner counted completed tests only | `run_automation.py` fails on a started test without a result and on `=== Critical error: ===` |
| An engine gate refuses: "an Unreal editor is running" | The previous gate's editor was still exiting | `run_automation.py` waits up to 30 s for this project's processes |
| A crash reading a pool slot after a travel | An effect keyed by a slot outlived the field it belonged to | Check the slot is still active with the same serial before reading it (`FPawVfx`) |
| A load or a new game is heard as a burst of sounds | The audio observes rising counters; a replaced progress jumps them | `UPawGameSubsystem::ProgressEpoch` bumped on a load / new game: the observer rebases silently |
| The selection ring missing on a locked row, or under pad focus | Common UI never hovers a non-interactable button, Slate never turns focus into hover, and a textured hover plate cannot also carry a ring | The ring is painted over the plate (`UPawButton::NativePaint`, `RingRule`: hovered, or focused when the last input was not the mouse) (HANDOFF-LOG (58)) |
| A line break inside a string-table text shows twice | Python's `Path.write_text` writes CRLF on Windows; the table import keeps the `\r` inside quoted strings | L0: any `\r` in an `ST_*.csv` fails; tools write with `newline="\n"` (HANDOFF-LOG (59)) |
| A per-frame button effect never moves | `UCommonButtonBase` is `meta = (DisableNativeTick)`: a button subclass's `NativeTick` never runs | The owning screen ticks it (`FPawJuice::Tick` from the screen's tick) (HANDOFF-LOG (60)) |
| A hover pop on a button Godot disables | The port keeps some buttons interactable where Godot disables them (the tree's nodes, focusable for the ring); Common UI withholds hover only from non-interactable buttons | `FPawJuice::SetHoverOff` carries Godot's `disabled` (HANDOFF-LOG (60)) |
| A visibility check is false in every automation test | `UWidget::IsVisible()` asks the Slate widget, which does not exist before a first paint | Read `GetVisibility()` up the parent chain (`FPawJuice::IsShownInTree`) (HANDOFF-LOG (60)) |
| A bar fills more smoothly in the port than in Godot | Godot's `ProgressBar` snaps its value to its `Range.step` (0.01 by default; hud.tscn's XP bar 0.0001) | `PawUi::SetBarValue` / `UPawRainbowBar::SetValue` keep what Godot's Range keeps (`Pc::GodotRangeValue`; fixture `rainbow`) (HANDOFF-LOG (62)) |
| A bar's rounded or nine-patch fill ends square | UMG's default fill style (`Mask`) clips the fill brush at the fraction; Godot draws the fill StyleBox at that width | `EProgressBarFillStyle::Scale` (`PawUi::MakeBar`); bars in an existing Widget Blueprint get it by `--graft "<Class>=restyle:<Bar>"` (the baker's restyle carries the fill style since 5d) |
| A Godot fixture run sits for half an hour and fails "did not finish" | A GDScript parse error in the fixture (e.g. `:=` on an untyped value) leaves headless Godot idle, never quitting | Run Godot once by hand on the scratch copy and read stdout (KB Godot/03) |
| A probe subclass of a Godot `_draw()` node records nothing | The script's `draw_*` calls bind to the native methods; a subclass cannot intercept them | The fixture recompiles the script's own source with only its draw calls renamed to recorders (`tools/godot_fixtures/rainbow_fixture.gd`; KB Godot/03) |
| A fixture of a node with `_process` does not reproduce | `_ready` turns processing on; a frame awaited before freezing it steps it by the wall clock | `set_process(false)` straight after `add_child`, no frame between (KB Godot/03) |
| A sabotage run started by mistake ran the whole list for hours | `sabotage_pawcore.py` / `sabotage_realm.py` with no `--only` run EVERY sabotage; a filter that matches nothing hands them none | Count the names before starting (a shell guard on the expected number); stopping a run mid-sabotage can leave that source sabotaged: check every target is its original text (HANDOFF-LOG (63)) |

## 4. 2D-in-Unreal traps (research, to be verified by the spikes)

| Symptom | Cause | Fix |
|---|---|---|
| Colours wrong/washed out | Tonemapper, auto exposure | doc 04 §3 + colour gate |
| Creatures overlap in the wrong order / flicker | Translucency sorts per component; never inside an instanced component | Depth from Y with masked sprites (doc 04 §4) |
| Jagged sprite edges | Masked = 1-bit alpha | FXAA (spike S1, doc 04 §10). Not MSAA (masked outlines are inside the quad) and not the Alpha to Coverage flag (broken on desktop) |
| Pad returns to the wrong row after switching keyboard -> pad | Common UI re-applies `GetDesiredFocusTarget()` on a device change; the remembered row was stale | Track the remembered row on every focus change (doc 05 §2 rule 8) |
| A navigation test passes with a rule missing | The walk only took shortest paths | Press every rule (doc 04 §10, S4) |
| The whole game looks soft | Desktop default screen percentage renders 1080p at 810p (4K at 1080p) and upscales | `r.ScreenPercentage.Default.Desktop.Mode=0`, `Default=100` (doc 03 §7) |
| Cook fails: "imports class ... in a module that is not available on the platform" (Interchange, NNEDenoiser) | Plugin restricted to the Editor target, content still referenced by engine config | `+DirectoriesToNeverCook` for its content root (doc 03 §7) |
| Layers missing for the first seconds of a packaged run | Components are not drawn until their PSOs are precached | Warm-up before measuring; PSO check in phase 8 |
| Field drawn too small (0.57x) in a window, right in a capture | Default `MaintainYFOV`: ortho width = visible height | `OrthoWidth = 1080` + `MaintainYFOV` on the camera (doc 04 §6) |
| Colour readback all black, or every colour off by one gamma step | Offscreen runs have no back buffer; render target gamma | Scene capture + `RTF_RGBA8` + `bForceLinearGamma=false` (doc 04 §3.5) |
| Smeared fast fliers | Temporal AA | Forward + MSAA or FXAA |
| Effect rings invisible | Additive blend over the light paper floor: no headroom (2026-10-31, `[Verified]`) | Translucent effects over light biomes (doc 04 §5.1) |
| Shadows missing under bodies | The shadow centred so close under the disc that the body covers all but a faded sliver | Centre it at the feet (0.45 x size below), check a screenshot |
| Blurry sprites | Streaming / mips | Never Stream; right raster size |
| Fringes around small sprites | Atlas mip bleeding | Padding + extrusion |
| Low fps with many sprites | One Actor/component each | Instanced renderer, entities as data |
| Cursor lags | Software cursor interpolation | Hardware cursors (verify packaged) |
| Idle game burns power in the background | No throttle | Unfocused frame cap from focus events, measured |
| Floor seams or stretching on ultrawide | Field grows with aspect | Tiling floors, seam shots per aspect |
| Big download for a small game | Engine baseline 150-320 MB | Trim plugins for Game target, Shipping, `-nodebuginfo`, no prereqs in the Steam depot |
| Huge Steam patches | Pak padding/IoStore layout | Measure; 1 MiB alignment per Valve; chunk by churn |
