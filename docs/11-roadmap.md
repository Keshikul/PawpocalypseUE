# 11 - Roadmap: phases, exit criteria, session prompts

> A phase is done when its **exit criteria** are met and written down with evidence (test counts, measurements, screenshots),
> not when its code exists. Estimates are in agent working sessions (one focused session of a few hours), from the Godot
> project's pace; treat them as ranges. The Godot game keeps shipping throughout (D12).

## Overview

| Phase | Goal | Sessions | Depends on |
|---|---|---|---|
| 0 | Setup and four spikes | 3-4 | designer answers Q1, Q2 (others can wait) |
| 1 | Data and the rules (PawCore), proved against pawsim | 4-6 | 0 |
| 2 | The realm: the field, combat, enemies, kittens, Pounce, bosses | 5-7 | 1 |
| 3 | Progression UI: dock, shop, travel, cards, Reawaken, tree, jobs, achievements, ending | 5-7 | 2 |
| 4 | Title, slots, saves, settings, pause | 2-3 | 3 |
| 5 | Pad and keyboard parity: input contract, glyphs, remapping, virtual cursor, padplay | 3-4 | 3, 4 |
| 6 | Presentation: all art, backdrops, VFX, audio, ceremonies, Boba, tutorial, UI motion | 4-6 | 2, 3 |
| 7 | Localisation: string tables, 20 languages carried over, fonts | 2-3 | 3-6 |
| 8 | Gates, soak, performance (Deck), packaging, patch-size measurement | 3-4 | all |
| 9 | Parity sign-off and the designer's playtest | 1-2 + playtest | all |
| | **Total** | **~32-46 sessions** | |

## Phase 0 - setup and spikes

**Steps**
1. Read `C:\Games\Knowledge\INDEX.md`, `2D/`, `Unreal/01, 03, 08, 09, 17, 18, 19, 24`, and docs 00-04 here.
2. ~~`git init`, `.gitignore`, `.gitattributes` (LFS)~~ **done 2026-10-28** (local; the GitHub remote is pending, see `docs/HANDOFF-LOG.md`).
3. Project settings per doc 03 §7 (Lumen, ray tracing, Substrate, Nanite, VSM, distance fields, auto exposure, motion blur, bloom off;
   startup maps; `CommonGameViewportClient`; DPI rule). Remove the OpenWorld default map. Plugins: enable CommonUI, EnhancedInput,
   PythonScriptPlugin, EditorScriptingUtilities; disable unused ones for Game targets.
4. Module layout per doc 03 §1 (`PawCore`, `Pawpocalypse`, `PawpocalypseEditor`) and the standalone PawCore test build script.
5. Spikes S1-S4 (doc 04 §8), each in `Source/Pawpocalypse/Dev/Spikes` + a scratch map; record results in doc 04 §10 and
   `Knowledge/2D/02` (as `[Verified]` with date); delete spike code.

**Exit criteria**: editor and Game targets build from the command line; Shipping builds; settings diff reviewed; spike decisions
written: AA method, renderer, raster scale and compression, dock/tree widget structure; the colour readback within +/-2 proven on
at least the ink, cream and three palette colours; 320-enemy scene measured at 1080p and 1280x800.

**Session prompt**: *"Read docs/00-START-HERE.md and do phase 0 steps 1-4 of docs/11-roadmap.md. Do not start the spikes."* then
*"Run spike S1 from docs/04 §8; record the numbers in §10 and Knowledge/2D/02."* (one prompt per spike).

## Phase 1 - data and rules

**Steps**: `tools/import_data.py` (headless DataTables from `Data/*.csv`) + parity test; PawCore: `PcData` + validator (all of doc 06
§6, each assertion negative-tested), `PcCombat`, `PcStats`, `PcEconomy`, `PcProgress`, `PcCards`, `PcTree`, `PcJobs`,
`PcAchievements`, `PcFormat`, `PcRng`; `Data/CodeConstants.csv` + the constants map for the gameplay rows of appendix C; pawsim
re-pointed at the CSVs (byte-identical report); `pawcore_sim` CLI; the three-way mirror.

**Exit criteria**: L1 tests (port the assertion intent of Godot `check_resources`, `check_cards`, `check_damage`'s pure parts,
`check_overkill`, the economy halves of `check_playthrough`) green and each negative-tested; mirror green at every evolution stage;
pawsim CSV report identical to the Godot-parsing report; no `float` in PawCore.

**Prompt**: *"Phase 1 of docs/11-roadmap.md: port GameState's economy into PawCore per docs/03 §2 and docs/06. Port the Godot
comments' reasoning with each function."*

## Phase 2 - the realm

**Steps**: `PcField` (pool, spawner with the biome mix, golden, gristle, fliers, signature shields, waves, kittens seek/roam with
the spatial grid, paw strikes with shred and crit, aura, zones, Pounce with the product cap and craters, overkill splash + bounty,
Rainbow Poop, Prism, Execute, boss phases/adds/time limit/bait, per-step budgets); `UPawRealmSubsystem` fixed step; `APawRealmView`
with the ortho camera, logical-size handling, the chosen sprite renderer (placeholder art from spike S3 is fine), shadows, the cat
following the pointer, the events drain; travel without level loads; a minimal debug HUD; the cheat overlay (F1).

**Exit criteria**: `Paw.Realm.*` tests: card effects run, stress (320 enemies) frame time inside budget, kitten spread, overkill
never chains, boss timeout sends back a biome and turns auto-travel off, a refused summon keeps the bait; travel < 16 ms;
frame-rate independence test green; a headless 1000x PawCore soak of the first three biomes clean.

## Phase 3 - progression UI

**Steps**: Common UI root layout; HUD screen with the dock (6 tabs, badges, side flip), status row, Pounce clock, XP bar, boss
banner; shop rows with bulk buy and hold-repeat; Realms with gates and Bait; Reawaken tab + confirm + ceremony; card pick; skill tree
screen (lattice layout port, pan/zoom, states, tooltips, Unicorn ceremony); job picker; achievements tab with claims; victory screen.

**Exit criteria**: `Paw.Playthrough` (reset, earn, buy, travel, kill bosses, Reawaken, buy nodes, the ending) green through the UI
entry points; tree seating/spacing/crossing tests green; screenshot tour of every tab and the tree reviewed against the Godot shots.

## Phase 4 - title, saves, settings, pause

**Steps**: save container (doc 08 §2) with temp/backup/CRC/queue/async, five slots + metadata, autosave rules, request-save
coalescing, new game clears everything; title screen (slots, garden, Report a bug, Discord); pause menu; settings popup (audio,
video, controls placeholders); the Godot save importer if Q3 = yes.

**Exit criteria**: round trip of every field; corrupt-slot fallback; crash-between-renames recovery; busy-state saves (card hand,
boss engaged, ceremony); previous-version load test; designer's save dir untouched by every test (sandbox check).

## Phase 5 - pad and keyboard parity

**Steps**: every Godot action in Enhanced Input (doc 05 §3) with both devices; device subsystem (family, cursor source, connect toast,
warp echo); glyph families and prompt text/icons; dock navigation; every screen's focus wiring; tree navigation; remapping UI and
rules; hardware cursors (paw open/closed); unfocused throttle.

**Exit criteria**: `Paw.Input.*` and the pad/mouse tours green; the padplay tour plays the whole loop with a pad alone; hand-check on a
real Xbox pad and a PlayStation pad through Steam Input; the AZERTY test.

## Phase 6 - presentation

**Steps**: copy the art generators into `tools/art/`, raster + atlas + headless import for all 377 SVGs; backdrops for 12 biomes
(tiling floors, props); visual definitions; cat stages with weapons and hats; signature glow, golden sparkle, flashes, pulses; VFX
primitives (SDF shapes + Niagara bursts) with the effects setting; audio event table, buses, music crossfades, rate limits; ceremonies;
Boba cameo in menus; tutorial overlay (eleven cues); UI motion.

**Exit criteria**: colour gate green on the packaged build; screenshot tour of all 12 biomes x 7 stages x 5 bosses reviewed against
Godot; every audio event plays once in a tour (logged); pruned audio both directions; tutorial gate green.

## Phase 7 - localisation

**Steps**: string tables for every player-facing string (doc 08 §4); the text checker; `tools/port_po.py`; Dashboard import;
composite fonts with CJK/Thai; pseudo-locale tour; language menu.

**Exit criteria**: 20 cultures complete (or a reviewed exception list of strings new to the port); placeholder sets identical;
font coverage test green; no overflow in the pseudo-locale tour at 1280x800 and 150% UI scale.

## Phase 8 - hardening and packaging

**Steps**: all gates in `tools/check_all.ps1` with `-Fast`; packaged gates; the packaged soak at 5x for the whole game and 20x for 30
minutes; Steam Deck / 1280x800 performance pass; package script with listing audit; chunking + patch-size measurement (doc 10 §3);
PSO precache check; Shipping build; crash/log collection.

**Exit criteria**: soak "no problems found" with 0 engine errors over a whole game; p99 frame time recorded and not worse than the
Godot build on the same PC; 60 fps at 1280x800 on the Deck-class target; a one-number balance patch measured at a small delta.

## Phase 9 - parity sign-off

**Steps**: side-by-side review (same save imported into both builds if Q3 = yes; same biome/stage screenshots; pawsim length report
unchanged); the designer plays the Unreal build; differences become tickets: "port bug" or "design change for later", never both.

**Exit criteria**: the designer signs off; the Unreal build becomes the shipping build; the Godot project is archived (its `/data`
exporter and `_godot_file` columns retired, doc 06 §2).

## Standing between phases

- After each phase: update docs 00-12 where reality changed (a doc that disagrees with the code is a bug), add new technical lessons to
  `C:\Games\Knowledge` with confidence tags (dimension folder for 2D-only facts, engine folder otherwise), re-run the full gate set.
