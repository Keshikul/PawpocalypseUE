# 01 - Port brief, scope and decisions

## 1. The brief

Rebuild Pawpocalypse in Unreal Engine 5.8.3 **as the same game**: same rules, same numbers, same content, same
look, same sounds, same controls on keyboard/mouse and pad, same twenty-one languages, same save behaviour. The
Godot game (`C:\Games\pawpocalypse`) is the specification. Where this handoff and the Godot code disagree, the
**Godot code is right** until phase 9 signs off parity, and this handoff is the bug.

**Why it is worth doing carefully:** the Godot project's own history (its handoff is 5,742 lines) is a record of
traps that cost days each: rewards that stopped tracking HP, a build that loaded zero evolution stages, a
damage term that disappeared, gates that passed while proving nothing. Almost every one of those has an Unreal
equivalent. Doc 12 lists them side by side. **The economy and its invariants are the most valuable thing being
ported, more valuable than any code.**

### 1.1 Scope

| Ported as-is (content and design) | Rebuilt (engine side) | Not ported (Godot-only) |
|---|---|---|
| Every `/data` number: 12 biomes, 59 enemies, 5 bosses, 7 stages, 15 shop lines, 28 cards, 87 tree nodes, 10 jobs, 41 achievements, combat and Reawaken constants (`Data/*.csv`) | All gameplay code (29,258 lines of GDScript, appendix B) in C++ | `PackLoader` and `.pck` splitting (Unreal has pak/IoStore chunks, doc 10) |
| The art generators (`tools/make_sweet_art.py` and siblings) and their 377 SVGs | The renderer for sprites, shadows and VFX primitives (doc 04) | `ResourceDir` and the `.remap` trap (Unreal's equivalent trap is the cooker, doc 12) |
| The palette (`tools/palette.py`) and the Stacklands paper look | The UI: every screen as Common UI widgets (doc 05) | `check.sh`'s script reload trick, `--headless` quirks, `.uid` files |
| The audio files (Kenney CC0, Abstraction music, the synthesised whinny) and the `EVENTS` table | Saves, settings, input, localisation plumbing | Godot export presets, `embed_pck`, the `itch-release` preset files |
| `pawsim.py` (re-pointed at `Data/*.csv`) | The 19 gates, as Unreal automation tests plus Python checks (doc 09) | GL Compatibility renderer specifics |
| The 20 translations (`locale/*.po`) | The soak driver, trailer director and store-shot probe | Movie Maker mode (Unreal: Movie Render Queue) |
| Every design rule in the Godot `pawpocalypse.md` | The release scripts (UAT, SteamPipe, butler) | |

### 1.2 Out of scope until parity

No new content, no balance changes, no new features. Open Godot-side design questions (the kitten/noticeable-buy
trade-off, the three card effects with no card, jobs never judged as a system) are **answered in Godot first** and
then re-exported (doc 06 §2). Porting and redesigning at once makes every difference ambiguous: "is this a port bug
or a design change?" must always have one answer.

## 2. Decisions taken by this handoff

Each has a reason. Re-opening one needs a measurement, not a preference.

| # | Decision | Why | Status |
|---|---|---|---|
| D1 | **UE 5.8.3 launcher build**, pinned (`5.8.3-58210709`); no engine upgrade until after launch | It is the version the knowledge base was verified on; 5.8 is the last planned major UE5 release `[KB U/01]` | Taken |
| D2 | **All gameplay logic in C++.** Blueprints only for visual tuning (sprite/VFX/widget looks). A **pure C++ core library** (`PawCore`, no engine dependency) holds every formula: combat, prices, Reawaken, evolution, gates | Testable in seconds, mirrors `pawsim.py`, and UE6's planned Blueprint sunset makes C++ the durable choice `[KB U/01 §1, U/03 §4]` | Taken |
| D3 | **CSV in `Data/` is the source of truth**, imported headless into DataTables; enums by NAME; lower-case string ids | Text diffs, scriptable, the simulator reads the same file; FName case changes between editor and packaged builds `[KB U/05]` | Taken |
| D4 | **Single player, no networking code at all.** No replication, no Iris, no sessions | The game has none; the KB's co-op machinery (GameState replication, server RPCs) is cost without benefit here | Taken |
| D5 | **The field is drawn in world space under a top-down orthographic camera, by our own instanced sprite renderer**, not one Actor per mouse | Hundreds of moving entities; one Actor each is the classic UE slowdown (300 flipbook actors measured ~20 fps in a forum report) `[Forum]`; Paper2D gets little development `[Forum]` | **Candidate - spike S1/S2 decides** (doc 04 §5) |
| D6 | **UI is Common UI + UMG, screens in C++** (`UCommonActivatableWidget`), styled by rounded-box brushes and one token header generated from `palette.py` | Pad support from day one `[KB U/08]`; Stacklands paper panels are exactly `FSlateRoundedBoxBrush` with an outline `[KB U/09 §4]` | Taken; spike S4 proves the dock and the tree |
| D7 | **Input is Enhanced Input + Common Input**, every Godot action mapped 1:1 (doc 05 §3) | The Godot game's input contract is already console-grade | Taken |
| D8 | **Saves are our own versioned text format** (JSON) with magic, version, CRC, temp+rename, a backup and five slots; not a raw `USaveGame` | The Godot save is a flat text resource with atomic writes; a renamed `USaveGame` class loads null `[KB U/10]` | Taken |
| D9 | **FText everywhere; string tables**; the 20 existing translations are carried over by script from `locale/*.po` | 20 languages at 100% are an asset; Unreal imports PO through the Localization Dashboard `[Docs]` | Taken; the converter is phase 7 |
| D10 | **Art stays generated SVG**, rasterised by script (resvg) to PNG at the sizes the game needs, packed into atlases, imported headless | Unreal has no production SVG texture import (Epic's `SVGImporter` is an Experimental VirtualProduction plugin) `[Docs]` | Taken; spike S3 sizes it |
| D11 | **Fixed-timestep simulation** (60 Hz) driven by one world subsystem, with a game-speed scale for the soak | Continuous `dps * delta` damage and the pawsim mirror need determinism; Godot's per-frame delta made the soak's 20x lie about pace `[KB U/03 §4.1]` | Taken |
| D12 | **The Godot build keeps shipping** (itch, and the Steam demo if Next Fest comes first) until phase 9 | A port that is 80% done is a game that does not work | Taken; see Q4 |
| D13 | **Double precision for every economy value** (`double` in C++, never `float`) | Godot's `float` is 64-bit; Catnip passes 1e12 and boss HP and DPS climb ten orders of magnitude. A C++ `float` is 32-bit (7 digits) and silently changes the economy | Taken; gated (doc 09) |

## 3. Questions for the designer

Answer these before or during phase 0. Each has a recommendation; none blocks the spikes.

| # | Question | Recommendation |
|---|---|---|
| Q1 | **What does the move to Unreal have to deliver?** (Consoles? Steam Deck performance? A different look later? Career skills?) The answer sets priorities: a console goal pulls doc 10 §6 (source engine, platform abstractions) forward | Write the one-line goal into this doc |
| Q2 | **1:1 recreation first, changes after parity?** | Yes (§1.2). Changes during the port make every difference ambiguous |
| Q3 | **Should itch players' Godot saves import into the Unreal build?** The Godot save is a flat text `.tres` (`PawpocalypseSaveData`, 27 fields), so a one-way importer is about a session | Yes, one-way, from the title screen ("Import a save from the itch version") |
| Q4 | **Steam Next Fest (February 2027) from which build?** The Godot plan (`pawpocalypse-steam-launch-plan.md`) targets it. The port will not reach parity by then at any realistic pace | Ship the demo from Godot; launch from whichever build is at parity |
| Q5 | **Art resolution**: the Godot game draws at a 1920x1080 logical size. Rasterise for 4K (2x) so the art stays sharp on big screens? | Yes: 2x rasters, mipmapped (doc 07 §2) |
| Q6 | **The look under Unreal**: exact Godot colours, or allow Unreal's post-processing (bloom, glow) for juice? | Exact colours first (a gate asserts them, doc 04 §3); effects only as a later, deliberate addition |
| Q7 | **The music licence carries over unchanged** (Abstraction's loops: credit required, the author does not endorse AI-related use; the trailer avoids them). Still your call? | Keep as-is; record it in the third-party register (doc 07 §5) |
| Q8 | **Boba** stays menus-only, never over the tree, as in Godot? | Yes, unchanged |

## 4. Risks, honestly

| Risk | Likelihood | What we do |
|---|---|---|
| **Length of the port.** 29k lines of GDScript plus 19 gates, the art pipeline, 21 languages and pad support | Certain | Phased plan with exit criteria (doc 11); parity before features; the Godot build keeps shipping |
| **Colours shift** (the tonemapper and exposure change unlit sprites) | High if ignored | Spike S1 plus a pixel-readback colour gate (doc 04 §3) |
| **Sprite edges alias or ghost** (masked edges are hard; temporal AA smears moving sprites) | Medium | Spike S1 compares the options and measures them (doc 04 §4) |
| **Translucent sort order** between overlapping creatures | Medium | Depth from Y, not translucency sorting (doc 04 §4) |
| **Packaged-only bugs** (uncooked assets, FName case) | High (the Godot project shipped one) | One always-cooked root, lower-case string ids, an export gate on a packaged build every phase (doc 09, 10) |
| **Build size** grows from ~200 MB (Godot) to roughly 150-320 MB of engine before content `[Forum]` | Certain, acceptable | Trim plugins for Game targets, `-nodebuginfo`, Nanite off `[KB U/19 §3]` |
| **Steam patch size** if paks are laid out badly | Medium | Chunking by churn, verified in phase 8 (doc 10 §3) |
| **Agents cannot see the editor** | Certain | Text-first: C++, CSV, ini, Python builders; screenshot tours to see the game `[KB U/23]` |
