# Pawpocalypse (Unreal) - START HERE

> Written **2026-10-28** for the port of Pawpocalypse from Godot 4.7.1 (`C:\Games\pawpocalypse`, itch 0.2.5) to
> **Unreal Engine 5.8.3** (`C:\Games\pawpocalypseUE\Pawpocalypse`, a blank C++ project created the same day).
> Every claim about the Godot game was read from its code, `/data`, or its own docs on that date. Every claim about
> Unreal is tagged with where it came from: `[KB]` the knowledge base at `C:\Games\Knowledge` (verified on UE 5.8.3 in
> a real project unless it says otherwise), `[Docs]` Epic documentation, `[Forum]` forum/community reports, `[Plan]`
> a decision this handoff makes that must still be proved by a spike.

## What this is

A **recreation of the same game**, not a sequel and not a redesign. The design, the numbers, the art and the audio
carry over; the engine, the code and the build pipeline are new. The Godot project stays the reference build and the
shipping product until the Unreal build reaches parity (doc 11, phase 9).

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
| B | [appendix-b-godot-to-unreal-map.md](appendix-b-godot-to-unreal-map.md) | generated: all 79 Godot scripts (29,258 lines), where each goes and in which phase |
| C | [appendix-c-code-constants.md](appendix-c-code-constants.md) | generated: the 456 numeric/colour constants that live in Godot CODE, not `/data`, with their reasons |

Project rules for agents: [../CLAUDE.md](../CLAUDE.md). Engine knowledge: `C:\Games\Knowledge\INDEX.md` (read it, then
only the documents it points to; the **`2D/` folder** (`2D/01-2d-optimization-best-practices.md`,
`2D/02-unreal-2d.md`) was written from this handoff's research).

## How we proceed (the short version)

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

## What already exists in this project

| Path | What |
|---|---|
| `Pawpocalypse.uproject`, `Source/`, `Config/` | the blank C++ project (UE 5.8.3-58210709, created 2026-10-28) |
| `Data/*.csv` | **every gameplay resource of the Godot game** (266 resources, 11 classes), exported by `tools/export_godot_data.py`, enums written by NAME |
| `tools/export_godot_data.py` | the exporter; re-run it while both projects exist (doc 06 §2) |
| `tools/list_godot_code_constants.py`, `tools/make_port_map.py` | regenerate appendices C and B from the Godot repo (the map script fails if a Godot script is unmapped) |
| `docs/` | this handoff |
| `CLAUDE.md` | the standing rules for any agent working here |

Git: `https://github.com/Keshikul/PawpocalypseUE`, branch `master`, LFS for binaries (`.gitattributes`). Its own repo, separate
from the Godot repo (`JanePonomareva/pawpocalypse`).
**Session history and the exact next step: [HANDOFF-LOG.md](HANDOFF-LOG.md)** (newest entry first; every session adds one).
