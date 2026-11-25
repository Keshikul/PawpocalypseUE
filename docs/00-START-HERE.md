# Pawpocalypse (Unreal) - START HERE

> Rewritten **2026-11-24** (HANDOFF-LOG (145)) for the game as it stands. The project began 2026-10-28 as a port of the
> Godot game (`C:\Games\pawpocalypse`, itch 0.2.5) to **Unreal Engine 5.8.3**; older docs and HANDOFF-LOG entries speak of
> "the port", "parity" and "Godot, the reference": read those parts as history.

## What this is

**THIS IS THE GAME (D22, 2026-11-23, the designer: "Forget about godot. Treat this as a new game. Godot is just a first
try. This is the real version.").** This project is the source of truth for every rule, number, word and look; the Godot
game is history: nothing is matched to it, no gate or tool reads it (the Godot-reading tools were deleted 2026-11-24). What
came from it (`Data/`, the recorded fixtures `Source/PawCore/Tests/Data/*_godot.tsv`, the translations) is this game's own.

Pawpocalypse is a 2D idle/incremental cat-sim (doc 02): PC (Windows/Steam, the Steam Deck among PCs; Q1), keyboard/mouse
and pad, single player, 23 languages. The look is **Sticker Box** (D18), the text plain English (D15). Steam Next Fest
(February 2027) ships the newest build (Q4). Version 0.MINOR.PATCH, now 0.1.0 (D21).

## Reading order

| # | Doc | Read it when |
|---|---|---|
| 00 | this page | first |
| 01 | [01-port-brief-and-decisions.md](01-port-brief-and-decisions.md) | the decisions taken (D1-D24), the designer's answers (Q1-Q13), the first try's faults fixed here (F1-F56) |
| 02 | [02-game-design-handoff.md](02-game-design-handoff.md) | before any gameplay change: the whole game, every rule, every invariant |
| 03 | [03-technical-architecture.md](03-technical-architecture.md) | before any C++: modules, classes, the sim/presentation split, ticking |
| 04 | [04-2d-rendering-in-unreal.md](04-2d-rendering-in-unreal.md) | before drawing anything: 2D in Unreal, the renderer, the draw-call budget |
| 05 | [05-ui-input-controller.md](05-ui-input-controller.md) | before any widget or input action |
| 06 | [06-data-pipeline-and-simulator.md](06-data-pipeline-and-simulator.md) | before touching `Data/` or a balance number (pawsim, `econ_review`) |
| 07 | [07-art-audio-pipeline.md](07-art-audio-pipeline.md) | before importing a texture, a font or a sound |
| 08 | [08-saves-settings-localization.md](08-saves-settings-localization.md) | before saves, options or any player-facing text |
| 09 | [09-testing-and-gates.md](09-testing-and-gates.md) | before claiming anything works |
| 10 | [10-build-release-steam.md](10-build-release-steam.md) | before packaging or anything Steam |
| 11 | [11-roadmap.md](11-roadmap.md) | the phases, their exit criteria and where each stands |
| 12 | [12-pitfalls.md](12-pitfalls.md) | when something breaks, and before every phase |
| 13 | [13-research-sources.md](13-research-sources.md) | to check where a claim came from |
| | [HANDOFF-LOG.md](HANDOFF-LOG.md) | **every session's entry, newest first: where the project is and the exact next step**; (1)-(127) in [HANDOFF-LOG-ARCHIVE.md](HANDOFF-LOG-ARCHIVE.md) |
| | [ECONOMY-REVIEW-2026-11-12.md](ECONOMY-REVIEW-2026-11-12.md) | the economy's goals and the measurements behind D17, D19 and D24 (the pace, §10) |
| | [STEAM-PAGE-DRAFT-2026-11-18.md](STEAM-PAGE-DRAFT-2026-11-18.md) | the Steam page's words, signed off 2026-11-18 (every number from `Data/`) |
| | [WORKLIST-STICKER-2026-11-17.md](WORKLIST-STICKER-2026-11-17.md) | **the look (D18, Sticker Box)**: the approved concepts' links, the look spec, S0-S7, what is left |
| | [WORKLIST-SLICE6-2026-11-07.md](WORKLIST-SLICE6-2026-11-07.md) | **not built yet**: the Reawaken banner (6b), Boba's cameo (6c) and the tutorial (6d), surveyed from the first try |
| | [WORKLIST-LOC-2026-11-18.md](WORKLIST-LOC-2026-11-18.md) | localisation's decisions: the 23 languages, the variants, the translation files |
| | [WORKLIST-RESTYLE-2026-11-07.md](WORKLIST-RESTYLE-2026-11-07.md) | history: the plum look (D14, replaced by D18); its pipeline, tools and gates are still the ones in use |

Project rules for agents: [../CLAUDE.md](../CLAUDE.md) (also the list of every tool). Engine knowledge:
`C:\Games\Knowledge\INDEX.md` (read it, then only what it points to; for this game always `2D/01`, `2D/02` and
`Unreal/24-gotchas-master-list.md`).

## Where the project is (2026-11-24)

| Phase | State |
|---|---|
| 0 Setup and spikes | **Done** 2026-10-28 |
| 1 Data and rules (`PawCore`) | **Done** 2026-10-30 |
| 2 The realm (`PcField`, the renderer) | **Done** 2026-11-01 |
| 3 Progression UI | **Done** 2026-11-02 |
| 4 Title, saves, settings, pause | **Done** 2026-11-04 (every screen an editable Widget Blueprint since (38)) |
| 5 Pad and keyboard | **Done** 2026-11-05; the hand-checks on real pads and an AZERTY keyboard are the designer's |
| 6 Presentation | **Done but slice 6's last three**: the art, backdrops, effects, sound, chrome, icons, motion, the Unicorn ceremony (6a). **Not built: the HUD's first-Reawaken banner (6b), Boba's photo and cameo (6c), the tutorial (6d)** ([WORKLIST-SLICE6](WORKLIST-SLICE6-2026-11-07.md)) |
| The look (D18, Sticker Box) | S0-S5 done (palette, Nunito, chrome, every screen as the concept draws it, every creature); left: S6's card art and skill / achievement icons (drafts waiting since (127)), the props, the title font (E1), the rig (E3) |
| 7 Localisation | **Done** 2026-11-23: 22 translations complete, loaded at run time, every language checked in the package; no native reads (D20) |
| 8 Hardening and packaging | **Done but the Deck pass** (no Deck): `tools/package.py`, the bench (46.8 draw calls), the whole-game soak, the PSO check (0 misses), patch size (1.2 MiB for a one-number change) |
| 9 Release sign-off | Not started: the designer plays the game and signs it off (D22) |

**The pace (D24, 2026-11-24)**: the designer asked for a slower game; twice the kills a Bait and tougher bosses take the
game from 113 to 175 min of pawsim time for the player who buys at once (doc 01 D24, `ECONOMY-REVIEW` §10).

**Git**: `https://github.com/Keshikul/PawpocalypseUE`, branch `master`, LFS for binaries. Pushed through HANDOFF-LOG (143)
(`8e773dc`); the newest entry says what is uncommitted. Commits and pushes only at the designer's word.

**Gates**: `powershell -ExecutionPolicy Bypass -File tools\check_all.ps1`: the everyday set, 37 gates, ~6-10 min (done is
exit 0); `-Full`: 49 gates, ~10 h (the sabotages, the long suite, the pad tour). The last complete `-Full` was 2026-11-11/12
(HANDOFF-LOG (99)): one is owed. The packaged gates (`packaged_languages`, `packaged_bench`, `packaged_pso`,
`packaged_soak`) run on a package, which is the designer's word (doc 09 §7, doc 10).

### Waiting on the designer

- **To build, on a go**: the Reawaken banner (6b), Boba (6c) and the tutorial (6d); the stall watchdog (a 1 s whole-process stall seen twice in 5x
  soaks, HANDOFF-LOG (144)).
- **Drafts and decisions**: the card pictures and the skill / achievement icons (HANDOFF-LOG (127)); the Missing Art page;
  the title font (E1) and the rig (E3) (WORKLIST-STICKER); a menu switch for the background frame cap and "mute when
  unfocused" (new words, drafted first); the store page's Dutch and Danish.
- **From the code review of 2026-11-24** (HANDOFF-LOG (145)): the Controls page's "Tutorial tips" box does nothing until
  6d (hide it or build 6d); the tree's first open costs 114-127 ms (create it ahead, on a go); Report a bug opens the first
  try's itch page (Steam's discussions after the App ID); Steam integration (doc 10 §5); the deck's and the achievement
  roster's order come from the `_godot_file` paths (an explicit order column).
- **Hand-checks**: a real Xbox and PlayStation pad through Steam Input, an AZERTY keyboard, the Output device swap, the
  paw cursor in a package; a Deck-class machine (the Deck pass).
- **Release**: phase 9, the designer's playtest and sign-off.

## What is where

| Path | What |
|---|---|
| `Pawpocalypse.uproject`, `Config/` | UE 5.8.3-58210709; no Lumen / ray tracing / Nanite / Substrate / VSM, flat and unlit; the game's plugins trimmed (`tools/trim_game_plugins.py`); `ProjectVersion` (D21) |
| `Source/PawCore/` | the rules and the field simulation in plain C++17 (no engine headers but `Private/PawCoreModule.cpp`); `Tests/`: doctest L1 and the long suite, the mirror CLI, the recorded fixtures (`Tests/Data/*_godot.tsv`) |
| `Source/Pawpocalypse/` | the game module: `Core`, `Data` (DataTables -> PawCore), `Realm` (the fixed step, the view, the effects), `Render` (the instanced sprite renderer), `UI` (Common UI screens, each an editable Widget Blueprint), `Input`, `Save`, `Audio`, `Dev` (cheats, `paw.Bench`, `paw.Soak`, `paw.Shot`, ...) (doc 03) |
| `Source/PawpocalypseEditor/` | editor-only builders: the Widget Blueprint baker, the input assets, the fonts |
| `Content/Pawpocalypse/` | everything shipped (always cooked): maps, generated materials, the field and UI atlases, icons, sounds, fonts, `DT_*` (generated from CSV, never edited), `WBP_*` (the designer's to edit), the string tables and translations |
| `Data/*.csv` | the source of every number (doc 06); edited here, imported by `tools/import_data.py` |
| `tools/` | every tool and gate: the list with one line each is in `CLAUDE.md` |
| `SourceArt/`, `SourceAudio/` | the generated SVGs (committed), their rasters (gitignored), the fonts; the sounds (LFS) |
| `build/` | untracked outputs (logs, reports, packages). **Keep `build/*_godot_full.tsv`**: the long suite's full fixtures (158 MB), recorded from the first try and no longer reproducible |
| `docs/` | this handoff |
| Google Drive `My Drive/Games/Pawpocalypse` (`G:\My Drive\...` on this PC) | laid out like BrineTime's (2026-11-24): `Builds/` (`Pawpocalypse-<date>-<topic>.zip`), `Logs/`, `Promotional/` (its `README.md` maps every current file: the Steam set, screenshots, store text in 21 languages, fonts and colours, icon, logo, the concepts and drafts; old sets in `_Archive`) |
