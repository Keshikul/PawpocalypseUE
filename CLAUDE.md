# PAWPOCALYPSE (Unreal) - Project Instructions

> Written 2026-10-28 at the start of the port. If anything here disagrees with the code or `Data/`, **the code is right and this
> file is a bug**: fix it in the same pass. Until parity (docs/11 phase 9) the **Godot game at `C:\Games\pawpocalypse` is the
> specification**: where this project and the Godot code disagree on a rule or number, Godot is right. **The LOOK is the
> exception (designer, 2026-11-07): "The unreal version does not have to match godot, they can be different." and "Plum
> everywhere."** So is the player-facing TEXT since 2026-11-09 (D15: the port's own plain English, below). The look's reference is the concept canvas and `docs/WORKLIST-RESTYLE-2026-11-07.md`; the palette, the art
> generators (`tools/art`) and every look test are this project's own. Rules, numbers, behaviour and timing still follow Godot.

## What this is
A recreation of Pawpocalypse (2D idle/incremental cat-sim, Godot 4.7.1, itch 0.2.5) in **Unreal Engine 5.8.3** (launcher build
5.8.3-58210709), C++. Same game: rules, numbers, content, sounds, controls, languages; its own look (the plum restyle, 2026-11-07). Windows/Steam first, built to port to
PlayStation and Xbox: keyboard/mouse AND gamepad from day one. Single player, no networking.

**Start every session with `docs/00-START-HERE.md`, then the newest entry of `docs/HANDOFF-LOG.md`; end every session that changed
anything by adding an entry there** (where the project is, what happened, what is open, the exact next step). The handoff is docs 00-13 plus appendices A (content, generated), B (Godot
script -> Unreal map, generated), C (constants living in Godot code, generated).

## Knowledge base
`C:\Games\Knowledge` holds verified engine knowledge. Read `INDEX.md`, then only what it points to. For this game always:
`2D/01-2d-optimization-best-practices.md`, `2D/02-unreal-2d.md`, `Unreal/24-gotchas-master-list.md`. When something fails, check the
symptom index first. New technical lessons go back into it with a confidence tag (2D-only facts in `2D/`, engine-general in `Unreal/`),
never project names or design decisions.

## Standing rules
- **Never commit, push, package or export unless the designer asks in that message.** Stage explicit paths, never `git add -A`.
  No force-push.
- **Measure, don't assume.** Balance claims come from `tools/pawsim.py` (reads `Data/*.csv`); performance claims from packaged builds.
- **A test that has never failed has never been tested.** Negative-test every assertion; confirm the sabotage landed.
- **Sandbox every automated run** (saves, settings, input remaps): the designer's own game must stay byte-identical.
- **No Lumen, no ray tracing** (designer, 2026-10-28), and none of Nanite, VSM, distance fields, Substrate, auto exposure, bloom,
  motion blur: the game is flat and unlit (docs/03 §7, docs/04).
- **No design changes during the port** (the look excepted since 2026-11-07, D14: it is changed here only, never in Godot).
  Open design questions are answered in Godot first, then re-exported
  (`py tools/export_godot_data.py ...`). A difference is either a port bug or a design change for after parity, never both.
- **Player-facing text is plain and informative** (designer, 2026-11-09: "i do not want the text to be like a poem or any kind
  of language with character", "all the text should just be to the point informative information"): real numbers, not
  "increases crit"; one word per idea (the list: `VOCAB` in `tools/text_audit.py`; "increase", never "grows";
  "permanently", never "from now on"; "enemies", never "mice": designer, 2026-11-09, "we don't only kill mice").
  The cat is still malevolent, never cute, in the art. **Player-facing text is short.**
  Designer, 2026-11-08: "make sure all text is simple and concise in the ui. Don't overexplain. Use short sentences."
  New or changed UI words are drafted for the designer first (they also cost the string's 20 translations).
  The approved text lives here, not in Godot: `ST_UI` directly; for the tables generated from Godot (`ST_Data`, `ST_Tree`)
  in `Data/TextOverrides.csv`, which the generators apply (Godot's translation gate needs every English string translated,
  so the port does not change Godot's words).
  Designer, 2026-11-08: no grudges and never an "unimpressed" cat, anywhere (they slipped through in Godot).
  Internal ids (`grudge_ledger`, `GRUDGE_PER_CARD`) stay: save keys and enum names are never renamed.
  "Prestige" is never used (it is Reawaken). Boba (the designer's cat) appears in menus only, never over the skill tree.

## Hard rules (docs/02 §14 has the full checklist)
- Gameplay numbers live in `Data/*.csv` (or `Data/CodeConstants.csv`), never hardcoded; DataTables are generated from CSV, never
  edited.
- All damage through `PcCombat`; additive and multiplicative brackets stay separate; anything priced off damage reads `PlayerDps()`.
- **`double` for every economy/damage value; no `float` in PawCore** (one exception, 2026-11-05: `PcBackdrop`, which
  reproduces Godot's float32 `Vector2` / RNG bit for bit; L0 `FLOAT_IS_THE_SPEC`). Lower-case string ids, never FName identity.
- Enums stored by NAME; never rename a value.
- Two currencies (Catnip, Stardust), never a third. The tree never resets. The Reawaken multiplier keys off Stardust EVER granted.
- Rewards = 0.0433 x HP; uncapped multiplicative lines `cost_growth > effect_per_level`; the spawn rate never reads the cat's damage.
- Fixed-step simulation (60 Hz) in PawCore; presentation interpolates; nothing loads per frame or per kill; travel never loads a level.
- **Every UI screen and piece is an editable Widget Blueprint** (designer, 2026-11-04): a C++ class (rules, wiring, tests) whose
  code layout `tools/bake_widgets.py` bakes ONCE into `WBP_<Name>` (button styles: `BP_<Name>`) under
  `/Game/Pawpocalypse/UI/Widgets`; from then on THE ASSET OWNS THE LAYOUT: never re-bake over it (a layout fix is made in the
  asset), create widgets through `PawUi::Resolve`, follow the contract in `Source/Pawpocalypse/UI/PawWidgets.h`, add the class
  to `PawUi::BakedClasses`. `Pawpocalypse.UI.Widgets.*` asserts every listed class is a Blueprint and matches its code layout.
- Entities are array rows, not Actors. UI is Common UI; never Set Input Mode by hand; every screen has a focus target; every action
  has a keyboard/mouse and a pad binding (or is on the documented one-device list).
- Every player-facing string is FText from a string table; format argument names are `FString`; control prompts are action tokens.
- Everything shipped lives under `/Game/Pawpocalypse` (always cooked). Packaged-build gates run at the end of every phase.

## Build and tools (Windows; run batch files from PowerShell)
```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" PawpocalypseEditor Win64 Development -Project="C:\Games\pawpocalypseUE\Pawpocalypse\Pawpocalypse.uproject" -WaitMutex
py tools/export_godot_data.py --godot C:/Games/pawpocalypse --out Data --catalogue docs/appendix-a-content-catalogue.md   # + CodeConstants.csv from the reviewed Data/CodeConstantsMap.csv
py tools/list_godot_code_constants.py --godot C:/Games/pawpocalypse --out docs/appendix-c-code-constants.md
py tools/gen_paw_rows.py                                                 # DataTable row structs from the Data/*.csv headers
py tools/gen_paw_tokens.py                                               # UI/PawTokens.h, the colours, from tools/art/palette.py (+ PcBackdropTones.h, Tests/PcGodotLook.h)
py tools/fonts.py [--check]                                              # UI/PawFontSpec.h: the font F_Paw's table (faces, typefaces, sub-fonts)
py tools/import_fonts.py --run                                           # SourceArt/fonts -> FF_* faces + the composite F_Paw, headless (editor closed)
py tools/check_font_coverage.py [--self-test]                            # every language's characters reach a face of F_Paw that has them
py tools/gen_input_defaults.py [--check]                                 # Input/PawInputDefaults.h: Godot's input map (remapping's defaults)
py tools/gen_data_strings.py                                             # ST_Data.csv: data names for the string tables
py tools/import_data.py [--check | --self-test]                          # Data/*.csv -> DT_* (headless, editor closed) + parity
py tools/make_port_map.py --godot C:/Games/pawpocalypse --out docs/appendix-b-godot-to-unreal-map.md
powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1   # PawCore L1 tests, plain MSVC, seconds (-Long: long suite)
powershell -ExecutionPolicy Bypass -File tools\check_all.ps1             # every gate (-Full: + negative tests and long runs)
py tools/make_godot_fixtures.py [--only tree] [--check]                  # Godot ground truth (scratch copy, sandboxed)
py tools/sabotage_pawcore.py [--only NAME]                               # one-line PawCore sabotages; each must be CAUGHT
py tools/run_automation.py [--no-build] [Pawpocalypse.Realm]             # engine automation tests, headless (editor closed; builds first)
py tools/bake_widgets.py --run [--graft "Class=add:Part;swap:Old>New"]  # Widget Blueprints for UI classes that have none (never over one); --graft puts NEW parts into an existing one (restyle:Part a part's look, restyle:Code>Asset when an unnamed part's auto-name differs (names:x / codenames:x list both), props:Part a piece's settings, slot:Part its place and tilt, text:Part a text's whole look, remove:Part, wrap:Part>Wrapper moves the asset's part (children kept) into a new panel; add: takes a panel with children; all or nothing); --textfloor raises texts under 16 px; --recolour / --refont migrate the assets' saved colours / fonts
py tools/art/build_art.py [--check]                                     # the art generators -> SourceArt/svg (--check: = the committed SourceArt/svg)
py tools/raster_svg.py [--check]                                        # SourceArt/svg -> png (the glyphs: 56 px; the cursors; the chrome and icons 1:1)
py tools/pack_atlas.py [--check]                                        # the field's sprites -> SourceArt/png/atlas (2 pages + field.csv)
py tools/import_art.py --run [input|chrome|icons|atlas|floors]                       # SourceArt -> T_* textures, DA_FieldAtlas, the floors, headless (editor closed)
py tools/export_audio.py [--check]                                      # Godot's sound table -> Data/Audio*.csv + the played files -> SourceAudio/
py tools/import_audio.py --run                                          # SourceAudio -> SW_* sounds + DA_AudioBank, headless (editor closed)
py tools/create_input_assets.py --run                                    # IA_* / IMC_Pawpocalypse from Data/InputMap.csv (generated, never edited)
py tools/sabotage_realm.py [--only NAME]                                 # game-module sabotages of the realm tests (UBT each, ~1 min)
py tools/packaged_bench.py [--seconds 20]                                # L5: paw.Bench in the packaged build (build/package): frame gate
py tools/godot_tour.py [--skip-godot] [--skip-ue] [--set biomes|fonts] [--res WxH] [--uiscale 1.5]   # the screenshot tour, both games -> build/tour/compare (opens windows)
py tools/text_audit.py [--self-test | --banned]                         # player text over budget (short lines, short sentences), off-list words (VOCAB), banned words; drafts: the newest docs/TEXT-DRAFT-*.csv
py tools/text_overrides.py [--self-test]                                # Data/TextOverrides.csv: the port's own English over Godot's (gen_data_strings / gen_tree_tables apply it)
py tools/padplay_tour.py                                                 # L4: the pad-only playthrough + focus tours (-game window; sandboxed saves)
UnrealEditor-Cmd.exe ... -run=pythonscript -script=C:/abs/tools/create_field_materials.py   # the field and sprite materials (then create_maps.py)
py tools/trim_game_plugins.py                                            # rewrites the .uproject plugin list (doc 03 §7)
```
- PawCore test sources are wrapped in `#if defined(PAWCORE_STANDALONE_TESTS)` (UBT compiles every `.cpp` in the module folder);
  `Private/PawCoreModule.cpp` is the only PawCore file allowed an engine include.
- Close the editor before building after header/`UPROPERTY`/new-file changes; Live Coding only for `.cpp` bodies; never Hot Reload.
- Headless editor Python: `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=C:/abs/forward/slash.py -nullrhi -unattended -nop4
  -SCCProvider=None -stdout`, editor closed, `unreal.log_warning` for output.
- Never kill or close the designer's editor; check for running editor processes before headless runs; never two test runs at once.
- Write Python to files and run them (heredocs break on this project's apostrophes).
- `tools/check_all.ps1` is the one command that runs every gate (everyday set; `-Full` = nightly, ~10 h measured 2026-11-05/06). Run it before calling
  anything done: done is exit 0. Exit 2 means a gate was SKIPPED (an editor was open): not proven, not done. The engine gates
  run on the editor target it builds first; never on an older DLL. It builds the game target too (what a package compiles:
  editor-only API passes the editor build and breaks only there). Porting a Godot manager follows the recipe in docs/09 §7: real Godot fixture, coverage asserted, sabotages seen
  caught.
- A designer decision that changes a Godot rule is made in the Godot repo first (left uncommitted for the designer), then the
  fixtures are regenerated (`py tools/make_godot_fixtures.py --only <name>`) and PawCore follows. Godot findings: docs/01 §5, and the same list for the designer in the Godot repo's
  `docs/pawpocalypse-port-findings.md` (the one Godot file the port writes without a decision; left uncommitted).

## Folders
```
Source/PawCore/            pure C++ rules + field sim, no engine headers; Tests/
Source/Pawpocalypse/       game module: Core, Data, Realm, Render, UI, Input, Save, Audio, Platform, Dev
Source/PawpocalypseEditor/ importers, validators, builders (editor only)
Content/Pawpocalypse/      everything shipped (always cooked): Maps, Data, Art, Materials, Audio, Fonts, UI, Localization
Data/                      CSV source of truth (exported from Godot until parity)
SourceArt/, SourceAudio/   generated SVGs / rasters (png gitignored) / audio sources (LFS)
tools/                     exporters, importers, pawsim, art generators (tools/art), check_all
docs/                      the handoff
```
Do not create folders outside this structure without asking.
