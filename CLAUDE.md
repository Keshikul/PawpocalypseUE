# PAWPOCALYPSE (Unreal) - Project Instructions

> Written 2026-10-28 at the start of the port. If anything here disagrees with the code or `Data/`, **the code is right and this
> file is a bug**: fix it in the same pass. **THIS IS THE GAME** (designer, 2026-11-23, D22: "Forget about godot. Treat this as
> a new game. Godot is just a first try. This is the real version."). This project is the source of truth for every rule,
> number, word and look. The Godot game at `C:\Games\pawpocalypse` was the first try: history only. Nothing is matched to it,
> no gate reads it, no tool needs it (the Godot-reading tools were deleted 2026-11-24), and a rule is never changed to agree
> with it. What came from it (Data/, the recorded fixtures under `Source/PawCore/Tests/Data/*_godot.tsv`, the translations) is
> this game's own now. The look is **Sticker Box** (D18, 2026-11-17: the approved concepts, Bestiary and UI Screens, linked in
> `docs/WORKLIST-STICKER-2026-11-17.md`); the player-facing text is plain English (D15, below). Older docs speak of Godot as
> "the reference" and of "the port": since D22 read those as history.

## What this is
Pawpocalypse, a 2D idle/incremental cat-sim, in **Unreal Engine 5.8.3** (launcher build 5.8.3-58210709), C++. Its first try was a
Godot game (itch 0.2.5); this is the real version (D22). Its own look (Sticker Box, D18) and words (D15). **PC** (Q1, designer
2026-11-23: Windows/Steam, the Steam Deck among PCs): keyboard/mouse AND gamepad from day one. Single player, no networking.
Steam Next Fest (February 2027) ships the latest build of this game at the time (Q4). Version 0.MINOR.PATCH (D21).

**Start every session with `docs/00-START-HERE.md`, then the newest entry of `docs/HANDOFF-LOG.md`; end every session that changed
anything by adding an entry there** (where the project is, what happened, what is open, the exact next step). The handoff is docs 00-13 (the
numbers themselves are `Data/*.csv`; the appendices generated from Godot were deleted 2026-11-24).

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
- **Bugs are fixed as found; a change with a design or balance trade-off is proposed to the designer** (balance with
  `tools/pawsim.py` numbers, which read `Data/*.csv`) and made on a yes, asserted by a test, and recorded in doc 01 §2.
  Never because Godot does it differently (D22).
- **Player-facing text is plain and informative** (designer, 2026-11-09: "i do not want the text to be like a poem or any kind
  of language with character", "all the text should just be to the point informative information"): real numbers, not
  "increases crit"; one word per idea (the list: `VOCAB` in `tools/text_audit.py`; "increase", never "grows";
  "permanently", never "from now on"; "enemies", never "mice": designer, 2026-11-09, "we don't only kill mice").
  The cat is still malevolent, never cute, in the art. **Player-facing text is short.**
  Designer, 2026-11-08: "make sure all text is simple and concise in the ui. Don't overexplain. Use short sentences."
  New or changed UI words are drafted for the designer first (they also cost the string's 20 translations).
  The text: `ST_UI` and `ST_Tree` edited directly; `ST_Data` generated from `Data/*.csv` with `Data/TextOverrides.csv`
  over it (`tools/gen_data_strings.py`). A changed line is translated again (`tools/loc_status.py`: stale, then --todo).
  Designer, 2026-11-08: no grudges and never an "unimpressed" cat, anywhere (they slipped through in Godot).
  Internal ids (`grudge_ledger`, `GRUDGE_PER_CARD`) stay: save keys and enum names are never renamed.
  "Prestige" is never used (it is Reawaken). Boba (the designer's cat) appears in menus only, never over the skill tree.

## Hard rules (docs/02 §14 has the full checklist)
- Gameplay numbers live in `Data/*.csv` (or `Data/CodeConstants.csv`: the game's own, edited here), never hardcoded; DataTables are generated from CSV, never
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
py tools/gen_paw_rows.py                                                 # DataTable row structs from the Data/*.csv headers
py tools/gen_paw_tokens.py                                               # UI/PawTokens.h, the colours, from tools/art/palette.py (+ PcBackdropTones.h, Tests/PcGodotLook.h)
py tools/gen_fx_look.py [--check]                                        # Realm/PawFxLook.h: the aura's, strike paw's and crit's look (D3) from tools/art/fx_look.py
py tools/art/review_d3.py                                                # build/review/d3_aura_strike.png: the D3 review sheet (approved 2026-11-09)
py tools/art/look_lint.py [--self-test | --known]                       # the field's drawings follow the look spec (tokens, one ink, flat, no outline crossing itself, clear of their floors, eyes); KNOWN and APPROVED only shrink
node tools/art/sticker_record.js [--check]                               # Sticker Box (D18): the approved Bestiary's own code recorded -> tools/art/sticker_cast.py + sticker_colours.json (never edit those by hand)
py tools/art/sticker_art.py [OUT_DIR | --overlays]                       # the recorded cast as the game's files (make_sweet_art calls it); --overlays: the cat's hat and weapon constants PawFieldLook.h must hold
py tools/art/review_anatomy.py                                           # build/review/anatomy.png: the anatomy batch in sweet_anatomy.SPECIES / BATCH beside today's (R5)
py tools/art/review_rig.py                                               # build/review/rig.gif + rig.png: the enemies' wings and legs moving (SourceArt/svg/rigs, R5)
py tools/fonts.py [--check]                                              # UI/PawFontSpec.h: the font F_Paw's table (faces, typefaces, sub-fonts)
py tools/import_fonts.py --run                                           # SourceArt/fonts -> FF_* faces + the composite F_Paw, headless (editor closed)
py tools/check_font_coverage.py [--self-test]                            # every language's characters reach a face of F_Paw that has them
py tools/gen_input_defaults.py [--check]                                 # Input/PawInputDefaults.h: Godot's input map (remapping's defaults)
py tools/gen_data_strings.py                                             # ST_Data.csv: data names for the string tables
py tools/import_data.py [--check | --self-test]                          # Data/*.csv -> DT_* (headless, editor closed) + parity
powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1   # PawCore L1 tests, plain MSVC, seconds (-Long: long suite)
powershell -ExecutionPolicy Bypass -File tools\check_all.ps1             # every gate (-Full: + negative tests and long runs)
py tools/econ_review.py [--proposal recommended | pass+inc0.5 ...] [--waits FILE]   # the economy review's numbers: pawsim + waves + buy logs, proposals in memory -> build/econ
py tools/sabotage_pawcore.py [--only NAME]                               # one-line PawCore sabotages; each must be CAUGHT
py tools/safe_mutate.py [--list | --self-test]                           # puts back any planted fault a dead run left in the tree (check_all's first gate)
py tools/run_automation.py [--no-build] [Pawpocalypse.Realm]             # engine automation tests, headless (editor closed; builds first; --self-test: the guard's wait, tools/unreal_procs.py)
py tools/bake_widgets.py --run [--graft "Class=add:Part;swap:Old>New"]  # Widget Blueprints for UI classes that have none (never over one); --graft puts NEW parts into an existing one (restyle:Part a part's look, restyle:Code>Asset when an unnamed part's auto-name differs (names:x / codenames:x list both), props:Part a piece's settings, slot:Part its place and tilt, text:Part a text's whole look, wrapat:Part a text's wrap width, remove:Part, wrap:Part>Wrapper moves the asset's part (children kept) into a new panel (wrap:Code=Asset>Wrapper when the auto-names differ); add: takes a panel with children; all or nothing); --textfloor raises texts under 16 px; --recolour / --refont migrate the assets' saved colours / fonts; --colours lists every colour they save, by widget, property and role (build/widget_colours.tsv)
py tools/art/build_art.py [--check]                                     # the art generators -> SourceArt/svg (--check: = the committed SourceArt/svg)
py tools/raster_svg.py [--check]                                        # SourceArt/svg -> png (the glyphs: 56 px; the cursors; the chrome and icons 1:1)
py tools/pack_atlas.py [--check]                                        # the field's sprites and rig layers -> SourceArt/png/atlas (ONE 4096 page + field.csv: a page is a draw call per kind)
py tools/pack_ui_atlas.py [--check]                                     # the 128 px UI icons -> SourceArt/png/ui_atlas (one 2048 page + ui.csv; import_art icons brings it in: one Slate batch for the icons)
py tools/import_art.py --run [input|chrome|icons|atlas|floors]                       # SourceArt -> T_* textures, DA_FieldAtlas, DA_Icons (+ the UI atlas), the floors, headless (editor closed)
py tools/import_audio.py --run                                          # SourceAudio -> SW_* sounds + DA_AudioBank, headless (editor closed)
py tools/create_input_assets.py --run                                    # IA_* / IMC_Pawpocalypse from Data/InputMap.csv (generated, never edited)
py tools/sabotage_realm.py [--only NAME]                                 # game-module sabotages of the realm tests (UBT each, ~1 min)
py tools/packaged_bench.py [--seconds 20] [--runs 3] [--exec "paw.Rig 0"] # L5: paw.Bench in the packaged build (build/package): frame gate, up to 3 runs, the game thread on the best (--runs 1 --exec: an A/B)
py tools/package.py [--config Shipping] [--no-gates] [--bump]           # the designer's word only: clean archive, UAT, symbols, audit, README + Send logs.bat, the L5 gates (--audit [DIR]: an existing package; --self-test)
py tools/packaged_soak.py [--speed 5|20] [--minutes 240]                # L6: paw.Soak in the packaged build: a whole game through the UI's presses -> build/soak (PASS: won, no Error, no stall)
py tools/packaged_pso.py [--route soak|bench] [--log FILE]               # the PSO check: PSO precache validation along a route -> build/pso/report.txt (FAIL: one of this game's materials compiled at first draw); never clears the driver cache
py tools/packaged_languages.py [--start ru]                             # L5: paw.Languages in the packaged build: every language offered, complete, shown, drawn (glyphs), Thai/CJK wrapped -> build/loc/package (sheet.png)
py tools/store_shots.py [--res 1920x1080] [--only NAME ...] [--culture de]   # the Steam screenshots: scripted scenes in the real game (paw.Do runs a cheat in order), sandboxed saves -> build/steam/shots[/<culture>] (opens a window)
py tools/store_clips.py [--only TAKE ...] [--cut] [--list]              # the Steam GIFs (no text: the field alone), description clips (webm + mp4) and trailer: takes recorded frame by frame in the real game (paw.Clip, -UseFixedTimeStep), sandboxed saves -> build/steam/clips (opens a window)
py tools/text_audit.py [--self-test | --banned]                         # player text over budget (short lines, short sentences), off-list words (VOCAB), banned words; drafts: the newest docs/TEXT-DRAFT-*.csv
py tools/text_overrides.py [--self-test]                                # Data/TextOverrides.csv: the port's own English over Godot's (gen_data_strings applies it; --check: the tables agree)
py tools/port_po.py [--check]                                           # the translation files in their written form (no Godot since D22; --godot: the old carry-over) <- was: Godot's .po translations -> Localization/Translations/<culture>/<table>.csv ("Key","Text","Source"); THE FILES ARE THE TRANSLATIONS: a run keeps every row in use (the port's own too) and fills only what Godot has
py tools/loc_lint.py [--self-test]                                     # the translations' wording rules: numbers kept, line breaks, nothing left in English, no mice for enemies, one percent style
py tools/loc_status.py [--list | --todo | --apply WORK_FILE | --self-test]   # per culture: translated / missing / stale / broken; --todo: the work file build/loc/todo/<culture>.csv; --apply: its Translation column in, stamped with the English the translator saw (all or nothing)
#   the game loads the translations at start (PawText::RegisterTranslations) and offers only complete languages; -PawDraftLanguages: unfinished ones too
py tools/padplay_tour.py                                                 # L4: the pad-only playthrough + focus tours (-game window; sandboxed saves)
py tools/screen_tour.py [--set screens|biomes|fonts] [--res WxH] [--uiscale 1.5] [--culture de|pseudo]   # every screen / biome / language's font sheet shot in the editor build -> build/tour (opens a window; -NoSave)
UnrealEditor-Cmd.exe ... -run=pythonscript -script=C:/abs/tools/create_field_materials.py   # the field and sprite materials (then create_maps.py)
py tools/trim_game_plugins.py                                            # rewrites the .uproject plugin list (doc 03 §7)
```
- PawCore test sources are wrapped in `#if defined(PAWCORE_STANDALONE_TESTS)` (UBT compiles every `.cpp` in the module folder);
  `Private/PawCoreModule.cpp` is the only PawCore file allowed an engine include.
- Close the editor before building after header/`UPROPERTY`/new-file changes; Live Coding only for `.cpp` bodies; never Hot Reload.
- Headless editor Python: `UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=C:/abs/forward/slash.py -nullrhi -unattended -nop4
  -SCCProvider=None -stdout`, editor closed, `unreal.log_warning` for output.
- Never kill or close the designer's editor; check for running editor processes before headless runs; never two test runs at once.
- A negative test that plants a fault in a real project file does it through `safe_mutate.mutated()` (journaled, written by
  rename), never a bare write restored in `finally`: a power cut on 2026-11-13 left a sabotaged source all zeros.
- Write Python to files and run them (heredocs break on this project's apostrophes).
- `tools/check_all.ps1` is the one command that runs every gate (everyday set, ~6 min; `-Full` = nightly, ~10 h measured 2026-11-10). Run it before calling
  anything done: done is exit 0. Exit 2 means a gate was SKIPPED (an editor was open): not proven, not done. The engine gates
  run on the editor target it builds first; never on an older DLL. It builds the game target too (what a package compiles:
  editor-only API passes the editor build and breaks only there).
- The recorded fixtures (`Source/PawCore/Tests/Data/*_godot.tsv`, recorded from the first try) are this game's expected
  behaviour: a rule change edits the code, its test and the fixture rows it changes directly (never Godot). Never
  write to the Godot repo.

## Folders
```
Source/PawCore/            pure C++ rules + field sim, no engine headers; Tests/
Source/Pawpocalypse/       game module: Core, Data, Realm, Render, UI, Input, Save, Audio, Dev (Platform: planned, not built; doc 10 §5)
Source/PawpocalypseEditor/ importers, validators, builders (editor only)
Content/Pawpocalypse/      everything shipped (always cooked): Maps, Data, Art, Materials, Audio, Fonts, UI, Localization
Data/                      CSV source of truth (the game's own, edited here)
SourceArt/, SourceAudio/   generated SVGs / rasters (png gitignored) / audio sources (LFS)
tools/                     exporters, importers, pawsim, art generators (tools/art), check_all
docs/                      the handoff
```
Do not create folders outside this structure without asking.
