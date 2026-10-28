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
| `aura_power_3` parsed from the left = the Reawaken crash | Keys parsed from the right; test with multi-word ids |
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

## 4. 2D-in-Unreal traps (research, to be verified by the spikes)

| Symptom | Cause | Fix |
|---|---|---|
| Colours wrong/washed out | Tonemapper, auto exposure | doc 04 §3 + colour gate |
| Creatures overlap in the wrong order / flicker | Translucency sorts per component; never inside an instanced component | Depth from Y with masked sprites (doc 04 §4) |
| Jagged sprite edges | Masked = 1-bit alpha | MSAA + alpha-to-coverage (spike S1) |
| Smeared fast fliers | Temporal AA | Forward + MSAA or FXAA |
| Blurry sprites | Streaming / mips | Never Stream; right raster size |
| Fringes around small sprites | Atlas mip bleeding | Padding + extrusion |
| Low fps with many sprites | One Actor/component each | Instanced renderer, entities as data |
| Cursor lags | Software cursor interpolation | Hardware cursors (verify packaged) |
| Idle game burns power in the background | No throttle | Unfocused frame cap from focus events, measured |
| Floor seams or stretching on ultrawide | Field grows with aspect | Tiling floors, seam shots per aspect |
| Big download for a small game | Engine baseline 150-320 MB | Trim plugins for Game target, Shipping, `-nodebuginfo`, no prereqs in the Steam depot |
| Huge Steam patches | Pak padding/IoStore layout | Measure; 1 MiB alignment per Valve; chunk by churn |
