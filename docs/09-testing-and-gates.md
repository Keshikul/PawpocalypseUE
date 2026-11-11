# 09 - Testing and gates

> KB: `Unreal/18-testing-automation.md` (layers, automation tests, fixtures, tours, input tours, release gate), `Godot/03`
> (the gate rules learned on this very game). Godot: 19 gates, 12,952 assertions, `bash tools/check_all.sh` ~75 s; a soak that
> plays a whole exported game. **The gate culture is the most important non-code thing being ported.**

## 1. Rules (carried from Godot, all paid for)

1. **A gate that has never failed has never been tested.** Negative-test every assertion: break the real code, watch it go red,
   and first confirm the sabotage LANDED.
2. **Assert the wiring as well as the behaviour.** Calling the function under test proves nothing about whether the game calls
   it (nine wiring gates were green while three skill branches could not be bought).
3. **The log is the verdict**: runners grep for `Error:`/`Fatal`/`Ensure condition failed`/`Test Completed. Result={Fail}` and our own
   `PAW_FAIL` lines, not only the exit code. Every runner has a timeout.
4. **Sandbox the user's data** in every automated run: saves (`-NoSave` or a temp save dir), settings (`-UserDir=` a temp dir /
   reset Enhanced Input user settings), so the designer's own game stays byte-identical. A gate that writes saves refuses to run
   unsandboxed.
5. **One command runs everything** (`tools/check_all.ps1`: the everyday set by default, `-Full` for the nightly; §7). A gate not
   in it stops being run.
6. **Export-only gates**: some bugs exist only in packaged builds (the Godot build that loaded zero evolution stages; Unreal's
   uncooked assets and FName case). They run on a packaged Development build before every release and at the end of every phase.
7. **Compare per-gate counts with a real run**, not a remembered total.
8. **Never run two editor test sessions at once** (they corrupt each other) `[KB U/18 §2]`.

## 2. Layers (cheapest first) `[KB U/18 §1]`

| Layer | Tool | Time | Runs |
|---|---|---|---|
| L0 Static checks | Python: no `float` in PawCore; no colour literals outside tokens; no `PlaySound` outside audio; no player text outside string tables; every gameplay constant mapped; data enums parse; no `GetAllActorsOfClass` in gameplay | seconds | every change |
| L1 PawCore unit tests | standalone exe (doctest/Catch2), no engine | seconds | every change |
| L2 Simulator checks | `pawsim` invariants (`verify_pawsim.py`), `check_sims.py`, **mirror** (pawsim vs PawCore CLI) | seconds | data/rule changes |
| L3 Engine automation tests `Pawpocalypse.*` (this doc's older `Paw.*` names are the same tests) | `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests Pawpocalypse.;Quit" -TestExit="Automation Test Queue Empty" -nullrhi -unattended` | minutes | every C++ change |
| L4 Tours | `-game -RenderOffscreen` screenshot tour, pad tour, mouse tour (real Slate events) | minutes | UI/input changes |
| L5 Packaged gates | the same tests/tours against a packaged Development build. **Built so far (2026-11-01)**: `py tools/packaged_bench.py [--exec "cmd"]` runs `paw.Bench` (Dev/PawBench.cpp) in the packaged build on L_Realm at 1280x800, vsync and cap off: the field filled to the live cap, 20 s measured (game thread p99 < 4 ms, draw calls < 50, counted for the WHOLE frame; render thread and GPU reported), what is drawn checked against what is live every frame (a rigged mouse's body layer counts as its body), the field materials and the quad loaded, a screenshot; verdict `PAW_BENCH PASS/FAIL`, exit code. Seen to fail: L_Realm uncooked (no verdict). PASS 2026-11-04 (34.6 draw calls); FAIL 2026-11-10 (68.7: §7.3) | ~5-10 min (package ~1-3 min warm, bench ~30 s) | end of phase, before release |
| L6 Soak | packaged build plays the whole game at 5x (pace) and 20x (stability), plus a headless PawCore soak at 1000x | 30-150 min | weekly, before release |

## 3. The 19 Godot gates, mapped

| Godot gate | What it proves | Unreal home |
|---|---|---|
| `check.sh` (every script compiles, no `DirAccess` on `res://`) | build health | L3 build of Editor + Game targets; **Shipping compiles** (dev-only calls guarded) `[KB U/19 §2]`; compile-all-Blueprints commandlet (an uncompilable BP is "Unknown cook failure") |
| `check_resources.sh` (data loads, economy invariants) | data | L1 shared validator on the CSVs + L3 `Paw.Data.*` (DataTable parity with CSV, every row loads, ids lower-case) |
| `check_textures.sh` (import rules, backdrop seams) | art | L3 `Paw.Art.*`: every atlas texture Never Stream / sRGB / mips per kind; atlas rects in range; L4 seam shots per aspect. **As built (phase 6)**: `Pawpocalypse.Realm.Look.Atlas` (cells in range, the import = the packer's manifest, every drawing the data names), the seam gate in `build_art.py --check`, the backdrop's exact layout at 4:3-32:9 (fixture `backdrop`); the import settings are not asserted |
| `check_ui_theme.sh` (9-patch geometry) | UI theme | L3 `Paw.UI.Theme`: tokens header matches `palette.py`; button styles share one shape; focus ring set |
| `check_cards.sh` (XP track, deck shape) | cards | L1 `PcCards` + validator (built: `PcCards` = the real `CardManager` on 3,000 states) |
| `check_card_effects.sh` (every effect RUNS in a live realm) | cards wiring | L3 `Pawpocalypse.Realm.CardEffects` (built 2026-11-01): a standalone game instance, the card taken through the game's own entry (`UPawRealmSubsystem::ChooseCard`), the realm run through `AdvanceFrame`; one case per effect, run twice on the same seed (without the card, then with it at its maximum level), the consumer's mark required in the second run and absent from the first; the deck cards spent on the hand the realm deals; a coverage check over every `CardEffect` (3 have no card: doc 01 §3, F15; a card for one fails the check until it gets a case); taking a card rebuilds the field's stats. Plus `Pawpocalypse.Realm.EveryCard` (Godot's "every card at once": invariants, kills, every cost where it is read) |
| `check_overkill.sh` (nearest survivor, never chains) | combat | L1 `PcField` tests |
| `check_kitten_spread.sh` (pool spreads, no dogpile) | kittens | L1 `PcField` tests |
| `check_damage.sh` (+ `--export`) (damage, Reawaken, Bait, Pounce cap, bounty, Legacy, cat size; ladder ceilings x128) | combat/economy | L1 + L3 `Paw.Damage.*`; **L5 on a packaged build** (the evolution stages load, the cat does damage) |
| `check_playthrough.sh` (**a real run**: reset, earn, buy, travel, kill bosses, Reawaken, save, load, invariants) | the loop | L3 `Paw.Playthrough` in a test world through the real entry points; plus L1 headless version. **Built 2026-11-02** as `Pawpocalypse.UI.Playthrough` (no save / load until phase 4) |
| `check_stress.sh` (every pool saturated) | limits | L3 `Paw.Stress`: 320 enemies, max kittens, 64 zones, max effects; frame-time sample |
| `check_sims.sh` | simulators | L2 |
| `check_mirror.sh` | game == sim | L2 mirror (three-way, doc 06 §8) + palette tokens |
| `check_steam.sh` (+ `--export`) | build config | L0 config checks (presets, plugins per target, never-ship folders) + L5 pak listing audit (doc 10) |
| `check_loc.sh` (every text through `tr()`, `.pot` current) | text | L0 text checker + L3 every string table key resolves in every culture; placeholder sets match |
| `check_input.sh` (Input Map contract, device switching, glyphs, prompts, Pounce on trigger, stick cursor, dock, modals, tree, remapping) | input | L3 `Paw.Input.*` + L4 pad and mouse tours with real Slate events `[KB U/08 §6]` |
| `check_padplay.sh` (**the whole loop with a pad alone**) | pad parity | BUILT 2026-11-05 (phase 5 slice 5b): `py tools/padplay_tour.py` (`paw.PadPlay`, a real `-game` window, fresh sandboxed saves, pad events through `FSlateApplication`; 41 checks with the focus tour of six screens; nightly `-Full` gate "L4 pad-only playthrough"; 3 sabotages seen to fail it). The plan: L4 `-PadPlay` tour: title, shop, a card, travel, first boss, Reawaken, tree, settings, save and quit, pad events only |
| `check_tutorial.sh` (cues walked in a live realm; suppressed in soak/trailer/shots; never takes a click or focus; veteran saves skip) | tutorial | L3 `Paw.Tutorial` + L4 |
| `check_hud.sh` (HUD paths, slots, audio, pause, tree gating, Boba stays in menus, tree seating/spacing) | UI wiring | L3 `Paw.UI.*`: every `BindWidget` resolves (baked WBP/code parity `[KB U/09 §2]`), every tree node seated and reachable, one expanding child per page, Boba never referenced by gameplay code |

## 4. New tests the engine change requires

| Test | Why |
|---|---|
| **Colour gate** (doc 04 §3): palette swatches through the real sprite material and post-process, read back within +/-2 | The tonemapper/exposure trap |
| **Precision gate**: PawCore has no `float`; save round trip of 1e15+ values and 17-digit doubles; formatting of 1e3..1e300 | D13 |
| **Cook gate**: in a packaged build, every DataTable row found by id, every visual definition loads, every audio event resolves, string tables present in every culture | Cooker and FName-case traps |
| **Determinism**: the same seed and inputs give the same state after N steps; save at step K + load + continue == never saved | Fixed-step contract; save/busy rules |
| **Frame-rate independence**: the same game minute at 30, 60, 144 fps gives the same Catnip within tolerance | The 5x/20x soak lesson |
| **Travel cost**: biome change < 16 ms, no level load | doc 03 §4 |
| **Unfocused throttle**: focus lost -> frame cap applied; CPU% in the soak log | Idle-game power use. NOT IN GODOT: a design question (doc 01 Q10), after parity |
| **Hardware cursor in a packaged build** | doc 05 §4 |
| **Capture mode**: HUD uses `CaptureDuringMouseDown`; UI actions do not consume field keys | `[KB U/08 §3]` |
| **Glyph coverage**: every pad button has art in every family; keyboard keycaps render every bound key | `[KB U/08 §6]` |
| **Font coverage**: every character of every culture's strings is in the composite font | boxes in packaged text |

## 5. The soak (Godot `tools/soak.sh` + `ci/soak.gd`)

- A dev-only `UPawSoakDriver` (packaged Development build, `-Soak -Minutes=150 -Speed=5`) plays through the UI's own rules: aims
  the real cat (virtual cursor), buys through the shop API the buttons call, takes cards, summons bosses with Bait when the Realms
  page would allow it, travels only through `IsBiomeUnlocked()`, Reawakens, buys tree nodes. **It asks what the UI would allow; it
  never reaches into the model.** It logs boss kills, auto-bait, the game's own auto-travel, Catnip-gate waits, in game minutes;
  frame time p50/p95/p99 and the worst frames with what happened; leaks (UObject count, memory); every `Error`/`Warning` line;
  stalls. Tutorial suppressed.
- **Trust 5x for pace, 20x for stability** (Godot measured 10 h at 5x vs 15-16 h at 20x). With the fixed step the gap should
  close: measure it, and record whether 20x pace now matches.
- **Never edit the soak driver while a soak runs** (Godot: bash re-read the script mid-run). Launch detached, watch the log.
- Report format compatible with the Godot `soak_report.txt` so runs can be compared across engines.

## 6. Tours and what the agent "sees"

- `-UITour=<mode>` screenshot tours offscreen at 1920x1080, 1280x800, 2560x1080, 3840x1080 and in the pseudo-locale at the largest
  UI scale; every screen, every dock tab, the tree zoomed in/out, each biome, each evolution stage, each boss `[KB U/18 §4]`.
- Input in one step, the shot in the next (capture happens at frame end).
- Review every image; crop small text to 1:1 or 2x before judging. Compare with the Godot `build/playtest/` and
  `build/tutorial_shots/` images side by side for parity (phase 9).

## 7. What exists today (updated 2026-11-10)

`powershell -ExecutionPolicy Bypass -File tools\check_all.ps1` runs the **everyday set** (27 gates, ~6 min) and `-Full` the
**nightly** (40 gates, ~10 h). The history of each count is in the HANDOFF-LOG entries named below.

- **Last everyday run 2026-11-11** (`build/check_all_night_final.log`, HANDOFF-LOG (96)): 27 of 27 PASS, exit 0. The same
  night, outside `-Full`: the whole realm gate (516 of 516 CAUGHT, 4 h 8 min), every Godot fixture reproduced, PawCore's
  sabotages on what the night changed (55), the long suite, L4 (44 of 44).
- **Last nightly 2026-11-10** (`build/check_all_full_r5.log`, HANDOFF-LOG (87)): 37 of 39 PASS; the two failures were fixed
  the same day and each rerun alone (a Godot fixture stale after the dock default moved; a realm sabotage MISSED, its test
  strengthened and the sabotage seen CAUGHT). Times: PawCore sabotage 3 h 42 min (628), Realm sabotage 3 h 56 min (490), the
  pawsim parity sweep 1 h 34 min, the long suite 20 min, the Godot fixtures 17 min, L4 2 min.
- **Exit codes**: 0 = every gate ran and passed; 1 = a failure; 2 = nothing failed but something was SKIPPED (an Unreal
  process of this project was open: not proven, not done).
- **The guard** (`tools/unreal_procs.py`, self-tested in `-Full`): the engine gates and tools refuse while an Unreal process of
  THIS project runs, or one naming no project (it may be the designer's editor opened through the project browser); another
  project's process is ignored.
- **The engine gates run on this source's DLLs**: `UBT editor build` runs first and a failed build FAILS every engine gate
  unrun; `UBT game build` runs last (editor-only API passes the editor build and breaks only the game target: found by the
  phase 4 packaged gate, HANDOFF-LOG (41)). `tools/run_automation.py` builds on its own too (`--no-build` for a caller that has
  just built), fails a run in which a test started and never completed (a crash), and waits up to 30 s for an exiting editor.
- `sabotage targets current` (everyday, seconds): every sabotage's target found exactly once in its source, so a target that a
  later change rewrote fails the day it happens, not the nightly. Never run it beside a live sabotage run.
- Logs in `build\check_all\<gate>.log`; the table and the exit code are the verdict; the Godot repo's `git status` is
  compared before and after (`Godot repo untouched`).

### 7.1 Everyday gates

| Gate | Command | What it proves | Its negative test |
|---|---|---|---|
| L0 static | `py tools/check_pawcore_static.py` | no `float` in PawCore (one named exception, `PcBackdrop`: `FLOAT_IS_THE_SPEC`); PawCore includes only std, `Pc*.h`, doctest (module glue excepted); every `Tests/*.cpp` guarded; every generated file current (`PcEnums.h`, `PcDataRows.h`, `PawDataRows.h`, `PawTokens.h`, `PcBackdropTones.h`, `PcGodotLook.h`, `PawFxLook.h`, `PawFontSpec.h`, `PawInputDefaults.h`, `PawTreeTables.h`, `ST_Data.csv`, `ST_Tree.csv`); `Data/*.csv` = a fresh export of the Godot project (`CodeConstants.csv` included: the export fails on a code constant with no row in `Data/CodeConstantsMap.csv`, or a mapped one gone); **ui-rules**: no colour value in UI code but the generated tokens, no hard-coded player text, no `FText::AsNumber` / `AsPercent`, no engine font, every `ST_UI` key named exists and every one is named | `--self-test` (nightly): one plant per rule, all caught |
| L1 PawCore tests | `tools\build_pawcore_tests.ps1` | **198 test cases, 6.70 M assertions** (2026-11-11): every PawCore module against the REAL Godot code through its fixture (`PcFormat` 4,403 outputs, `PcCombat` 2,000 rows bit for bit, `PcStats`, `PcEconomy`, `PcCards`, `PcCardDraw` card for card against Godot's own PCG32 stream, `PcTree`, `PcJobs`, `PcProgress` and `PcAchievements` as replayed operation logs, `PcFieldRules`, the field's kittens, aura, Pounce, extras and boss, `PcHud`, `PcTreeLayout` / `PcTreeText`, `PcSave`, `PcTitle`, `PcPause`, `PcSettings`, `PcDevice`, `PcBindings`, `PcControls`, `PcDockRepeat`, `PcBackdrop`, `PcJuice`, `PcRainbow` / `PcSky`, `PcTween` / `PcUnicorn`: 1,000 rows each unless named), the 57 validator rules, and rule tests for what only a running field shows (determinism per seed, the 320 ceiling, shields, Overkill never chains, kittens, zones, Pounce, the boss). Presentation scripts are recompiled from their own source with their draw calls renamed to recorders; float32 tolerances are stated per fixture | `py tools/sabotage_pawcore.py` (nightly): **641** one-line source sabotages (2026-11-11), each caught by its test; it refuses to run when a target is not in its file exactly once, and refuses a sabotage that changes nothing (NOLAND) |
| mirror (PawCore vs pawsim) | `py tools/check_mirror.py [--random N]` | `pawcore_sim.exe` == pawsim on Godot's mirror fixture at every stage, 2,000 random states (damage, kittens, income, the Stardust grant and gate, Legacy, Fortune, golden chances) and every price, 1e-9 relative (measured: bit-identical) | `--self-test` (nightly): 11 data sabotages, all caught |
| validator fuzz 3k | `py tools/fuzz_validator.py --trials N` | random single-cell corruptions of `Data/*.csv`: every hostile one rejected or explained in the script | its `EXPLAINED` list emptied -> it fails |
| art generators reproduce | `py tools/art/build_art.py --check` | `tools/art` regenerates `SourceArt/svg` byte for byte (554 files); the floors tile without a seam (`sweet_biomes.py --seams`). Until D14 (2026-11-07) it also had to equal Godot's `assets/art` | seen to FAIL: one ink hex, 738 differences; the seam gate at a lowered limit |
| art rasters reproduce | `py tools/raster_svg.py --check` | the glyphs (56 px), the cursors (40 px), the chrome (9) and the icons (178) re-rasterise byte for byte | seen to FAIL on one changed pixel |
| field atlas reproduces | `py tools/pack_atlas.py --check` | the field atlas (249 sprites with the rig's layers, 4 pages, + `field.csv`) packs again identically; `Pawpocalypse.Realm.Look.Atlas` then requires the imported `DA_FieldAtlas` to equal `field.csv` | the asset check seen to FAIL on one changed UV |
| look lint, look lint self-test | `py tools/art/look_lint.py [--self-test]` | restyle R5 (WORKLIST-RESTYLE §6 step 2): the field's 104 drawings (creatures, bosses, player, props) use palette tokens or a token's grammar shade (`sweet_shapes.tint()` recorded while the generators run); every outline INK at `STROKE`; flat (no gradient, filter or opacity); each body clears every floor it ships on; eyes at 56 px >= 64 px; `unique` (no two characters' bodies within `palette.UNIQUE_MIN` 16); `rig` (layers sharing a depth never overlap over a 2.5 s loop); `crossing` (no polygon outline crosses itself); `palette.py`'s own rules. KNOWN: 23 faults left (the cat's pale coats; it held 94 when written), a ratchet that only shrinks | `--self-test` (everyday): every rule planted once, all CAUGHT |
| audio export reproduces | `py tools/export_audio.py --check` | Godot's sound table exports again to the same `Data/Audio*.csv` and the same 53 played files | seen to FAIL on one changed number |
| font coverage | `py tools/check_font_coverage.py` | every character of every language (the English tables, Godot's .po, the picker's names) reaches a face of `F_Paw` that has it, through Slate's face choice | `--self-test` (nightly): the Thai sub-font removed, a Cyrillic letter cut: both FAIL |
| text audit self-test, no banned words | `py tools/text_audit.py --self-test`, `--banned` | the text rule (D15): budgets, one word per idea (`VOCAB`), the banned words ("grudge", "unimpressed", "prestige", ...) in no player text | the self-test's plants, all caught |
| text overrides self-test, text overrides agree | `py tools/text_overrides.py --self-test`, `--check` | the port's own English (`Data/TextOverrides.csv`, 279 lines) is what `ST_UI.csv` says; the `ST_Data` / `ST_Tree` lines still match Godot's line and keep its `{tokens}` | 11 plants, all caught; realm sabotages `text_scope_*` |
| sabotage targets current | `py tools/sabotage_pawcore.py --list` and `py tools/sabotage_realm.py --list` | above | seen to FAIL on one rewritten target in each module |
| Godot fixes current | `py tools/godot_fixes.py` and `--self-test` | the port's fixes to Godot's faults (D16; F34, F37, F49 so far), applied by `make_godot_fixtures.py` to its scratch copy only, each still find their target in the Godot repo (read only) | the self-test: a stale target, a twice-found target, a no-op and the repo itself are refused |
| UBT editor build | `Build.bat PawpocalypseEditor ...` | above | a failed build FAILS every engine gate unrun |
| L3 DataTable parity | `py tools/import_data.py --check` | every `DT_*` as saved = its `Data/*.csv`: row names = ids in order, every cell's text (12 tables) | `--self-test` (nightly): one HP changed must FAIL |
| L3 `Pawpocalypse.Data` | `py tools/run_automation.py Pawpocalypse.Data` | 2 tests: the game's route (DataTables -> CSV text -> `Pc::BuildGameData`) = the CSV route, 4,203 cells; the validator's verdict identical; a drift is reported | `.ParityCatchesDrift` |
| L3 `Pawpocalypse.Realm` | `py tools/run_automation.py Pawpocalypse.Realm` | 16 tests: the fixed step (frame-rate independence at 30-240 fps bit for bit, the spiral guard, interpolation), travel (every biome, 0.24 ms worst against 16 ms, nothing kept), Territory, the view at 320 mice + 20 kittens (one body per live thing, a rigged mouse's limbs, shadows, Godot's draw order; advance + draw 0.50 ms mean), every card effect observed at its consumer in a live realm (`.CardEffects`; the 3 cardless effects asserted cardless) and the whole deck at once (`.EveryCard`), the look (`.Look.Rules`, `.Look.Atlas`: the atlas = `field.csv`, at most three layers each side, `r.EarlyZPassOnlyMaterialMasking` off), the effects (`.Vfx.Mapping`, `.Canvas`, `.Look`: D3's aura, paw and crit) | `py tools/sabotage_realm.py` (nightly), §7.2 |
| L3 `Pawpocalypse.UI` | `py tools/run_automation.py Pawpocalypse.UI` | 90 tests on the shipped Widget Blueprints: the string tables and the port's own words (`.Strings`, `.Text.PortWords`; 22 Godot-parity tests run under Godot's words, `PawTextTesting::FGodotWords`); the HUD (`.Hud.*`: readouts, a press answering on the frame, the dock's rect on either side, nothing over the field takes a click, navigation, the boss panel, Realms, Reawaken, Catches, Achievements, Jobs); the card pick, the tree, the Reawaken ceremony, the job picker, the victory screen, the title, pause and settings (each `.MatchesGodot` on its fixture); the Controls page; `.Widgets.AreBlueprints` / `.Parity` (every class in `PawUi::BakedClasses` is its `WBP_*`, holding its code layout's parts); the palette and font migrations and the colour roles; `.Chrome`, `.Icons`, `.Juice.*`, `.Rainbow.*`, `.Unicorn.*`, `.Fonts.Composite`; `.Widgets.TextFloor` (no text under 16 px); `.Layout.*` (every screen fits at 1920x1080 and 1280x800, 100 % and 150 % UI); `.Playthrough` (the whole run through the UI). **Not covered**: hold-to-buy, the hover / focus lift, painting itself (screenshots are the check) | realm sabotages `hud_*`, `pick_*`, `tree_*`, `title_*`, `pause_*`, `settings_*`, `widgets_*`, `chrome_*`, `icons_*`, `juice_*`, ... |
| L3 `Pawpocalypse.Save` | `py tools/run_automation.py Pawpocalypse.Save` | 9 tests on a memory storage: the sandbox, the round trip, the atomic write, the corrupt fallback, crash recovery at each step of the write, the save moments (a 54-level bulk buy is one write), autosave, versions, `.MidFight` (F36) | `save_*`, `load_*`, `recover_*`, `moments_*`, `autosave_*` |
| L3 `Pawpocalypse.Input` | `py tools/run_automation.py Pawpocalypse.Input` (with `-PawUsKeyboard`) | 19 tests: the generated assets = `Data/InputMap.csv` and every action on both devices but the one-device list; Godot's dead zone (`.HalfAxis`), keys, navigation, `get_vector`, every screen's keys through its own Slate widget, Pounce, the pointer over the field, the stick-steered cat, devices and toasts, the pad-audio toast, glyphs (fixture `input_glyphs`), prompts, remapping (`.RemapKeys`, `.RemapDefaults`, `.Remap`), keys by position (`.Azerty`) | `input_*`, `glyph_*`, `prompt_*`, `remap_*`, `pad_audio_*` |
| L3 `Pawpocalypse.Audio` | `py tools/run_automation.py Pawpocalypse.Audio` | 5 tests: `.Bank` (DA_AudioBank = the CSVs, pruned both ways), `.Rules` (rate limits, round robin, pitch, buses, the crossfade), `.When` (what plays when; F40's doubles kept), `.Output` (the device list and the pinned device). **Not covered**: the engine's real device swap (a `-game` run with two devices) | `audio_*`, `audio_output_*` |
| UBT game build | `Build.bat Pawpocalypse ...` | above | |

### 7.2 Nightly only (`-Full`)

| Gate | What it proves |
|---|---|
| L0 self-test, mirror self-test, font coverage self-test, Unreal guard self-test, DataTable parity self-test | each everyday gate's own negative test |
| PawCore sabotage | 641 one-line sabotages of PawCore (2026-11-11; +12 for batch B and F36), each CAUGHT by L1 (or the long suite) |
| Godot fixtures reproduce | `py tools/make_godot_fixtures.py --check`: the REAL Godot code, run in a sandboxed scratch copy, writes today's fixtures byte for byte (the oracle; the C++ tests are its negative test) |
| L1 long suite | `build_pawcore_tests.ps1 -Long`: 36 cases, 9.6 M assertions: the full Godot fixtures (3,000-5,000 rows each); the field soak (a scripted cat through the first three biomes, 3 seeds x 3 game hours, every step's invariants); the full-field step (320 mice, 21 kittens, aura, zones, Prism: 0.093 ms against 2 ms); ~150 M random progress states (finite stats, damage never lowered by a level (Patience x Attack Speed excepted, counted: F3), prices rising, uncapped lines dearer faster than they pay, formatting over 60 M random bit patterns, the RNG uniform over 2e10 draws) |
| mirror 200k states, validator fuzz 30k | the everyday gates at scale |
| pawsim parity sweep | pawsim on `Data/*.csv` == pawsim on the Godot `.tres`, byte for byte, over 90 settings (`--self-test`: one enemy's HP +1 % must show) |
| L4 pad-only playthrough | `py tools/padplay_tour.py`: `paw.PadPlay` in a real `-game` window on fresh sandboxed saves, pad events through `FSlateApplication`: the title, a new game, the shop, a card, travel, the first boss's Bait, a Reawaken, the tree, the pause menu, Settings, save and quit, Continue; the focus tour of six screens; 41 checks |
| Realm sabotage | `py tools/sabotage_realm.py`: 538 one-line sabotages of the game module (490 until the night of 2026-11-10: +26 for the review's fixes and D16's batch F; +22 on 2026-11-11 for batch B and F36), each built with UBT and CAUGHT by its engine test. Six were retired on 2026-11-06 (lines that, since the Widget Blueprints, run only at the bake: the property is the asset's, and `.Widgets.Parity` checks it there) |

### 7.3 Not gated yet

- **L5 packaged gate**: `py tools/packaged_bench.py` (§2) runs on a package made by hand when the designer asks (doc 10 §2).
  Last run 2026-11-10 (HANDOFF-LOG (85), (89)): FAIL on draw calls, 68.7 for the whole frame against 50 (the field ~26); the
  budget is the designer's call (doc 01 Q11).
- From §4: the colour gate in the real build (spike S1 proved the method), the cook gate, frame-rate independence in a
  packaged build, the hardware cursor in a packaged build; the soak (§5); the tours at several resolutions and in the
  pseudo-locale (§6, phase 7).
