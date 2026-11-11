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
1. **Steps 1, 3, 4 done 2026-10-28** (HANDOFF-LOG). Read `C:\Games\Knowledge\INDEX.md`, `2D/`, `Unreal/01, 03, 08, 09, 17, 18, 19, 24`, and docs 00-04 here.
2. ~~`git init`, `.gitignore`, `.gitattributes` (LFS)~~ **done 2026-10-28** (`https://github.com/Keshikul/PawpocalypseUE`).
3. Project settings per doc 03 §7 (Lumen, ray tracing, Substrate, Nanite, VSM, distance fields, auto exposure, motion blur, bloom off;
   startup maps; `CommonGameViewportClient`; DPI rule). Remove the OpenWorld default map. Plugins: enable CommonUI, EnhancedInput,
   PythonScriptPlugin, EditorScriptingUtilities; disable unused ones for Game targets.
4. Module layout per doc 03 §1 (`PawCore`, `Pawpocalypse`, `PawpocalypseEditor`) and the standalone PawCore test build script
   (`tools/build_pawcore_tests.ps1`). Maps from `tools/create_maps.py`.
5. Spikes S1-S4 (doc 04 §8), each in `Source/Pawpocalypse/Dev/Spikes` + a scratch map; record results in doc 04 §10 and
   `Knowledge/2D/02` (as `[Verified]` with date); delete spike code. **Status 2026-10-28: S1 DONE: exact colour proven (21
   swatches within 1-2 offscreen and on screen, exposure 0 EV, negative-tested); AA = FXAA. S2 DONE: renderer = instanced quads (13 draw calls, 0.6 ms game thread; Slate the fallback). S3 DONE: 2x
   rasters, BC7, 2048 pages; render resolution fixed at 100 %; no AA vs FXAA is designer question Q9. S4 DONE: the doc 05
   UI structure passed 1,193 pad/keyboard focus checks. All four spikes recorded; the spike code was deleted 2026-11-04
   (it lives on in commit `10c118e`).**

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

**Status 2026-10-30 (HANDOFF-LOG (17))**: PHASE 1 DONE: pawsim on the CSVs (byte-identical, 90-setting sweep); `PcData`
(+ generated `PcDataRows.h`, `PcEnums.h`); validator (55 rules, each negative-tested; found 2 Godot data bugs); `PcFormat`;
`PcRng`; `PcCombat`; `PcStats`; `PcEconomy`; `PcCards`; `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`, the card draw `PcCardDraw` (2026-10-29) (all against the real Godot code on random states); `pawcore_sim` +
the PawCore/pawsim mirror; long property suite; validator fuzz; `tools/check_all.ps1`; Second Opinion fixed in Godot and
PawCore by the designer's decision (doc 01 §5 F1); `Data/CodeConstants.csv` (56 gameplay constants from the reviewed map of
all 458 Godot code constants; PawCore and pawsim read it) (HANDOFF (15)); `tools/import_data.py` + the 12 DataTables + parity;
`FPawDataAdapter` and `Pawpocalypse.Data.Parity`, the game's third of the mirror (HANDOFF (15)). The nightly
`check_all.ps1 -Full` ran green on all of this 2026-10-30 (HANDOFF (17)).

**Exit criteria**: L1 tests (port the assertion intent of Godot `check_resources`, `check_cards`, `check_damage`'s pure parts,
`check_overkill`, the economy halves of `check_playthrough`) green and each negative-tested; mirror green at every evolution stage;
pawsim CSV report identical to the Godot-parsing report; no `float` in PawCore.

**Prompt**: *"Phase 1 of docs/11-roadmap.md: port GameState's economy into PawCore per docs/03 §2 and docs/06. Port the Godot
comments' reasoning with each function."*

## Phase 2 - the realm

**DONE 2026-11-01** (HANDOFF-LOG (20)): every exit criterion met, the packaged-build gate included.

**Steps**: `PcField` (pool, spawner with the biome mix, golden, gristle, fliers, signature shields, waves, kittens seek/roam with
the spatial grid, paw strikes with shred and crit, aura, zones, Pounce with the product cap and craters, overkill splash + bounty,
Rainbow Poop, Prism, Execute, boss phases/adds/time limit/bait, per-step budgets); `UPawRealmSubsystem` fixed step; `APawRealmView`
with the ortho camera, logical-size handling, the chosen sprite renderer (placeholder art from spike S3 is fine), shadows, the cat
following the pointer, the events drain; travel without level loads; a minimal debug HUD; the cheat overlay (F1). (Built 2026-10-31:
slice 7 below; the F1 panel moved to phase 3, its actions are console commands meanwhile.)

**How phase 2 is proved** (decided 2026-10-29, HANDOFF (16)): Godot's realm is frame-driven and rolls the engine's global RNG
(shared with sound pitch), so unlike the managers it cannot be replayed row by row. Its PURE parts are ported into
`PcFieldRules` and checked against the real Godot code (`field_rules_fixture.gd`: getters, cadence, pool pick, playfield, a real
`Enemy` node's `activate`/`take_damage`); the stateful `PcField` is pinned by rule tests from the Godot code and comments, each
negative-tested by a source sabotage. A rule that is pure in Godot goes through the fixture; one that is not gets a rule test.

**Slices** (each: port, fixture or rule tests, sabotages, `check_all`, docs, handoff):
1. **DONE (16)**: pool, spawner (golden, weighted mix, edge, inward heading, batches, the 320 ceiling), movement (wander,
   flier, gristle, golden flee + turn-back, Misdirection), lifetime, leaving the field, Funnel, shield cycles, damage intake
   (Shield Pierce, Traps, thresholds, Play With Food, Spectrum), the paw (rate, reach, shred, crit, Execute), Overkill splash
   (never chains), the kill payout (signature bounty, Overkill bounty, Catnip, kill count, Catches), events; Infestation and
   Compound Interest ticked by the field (TierController did it).
2. **DONE (18)**: kittens (pool sized to ownership and grown, the two-pass claim tally, sticky least-claimed targets with
   rebalancing, Kitten Foreman, roam, the orbit approach, contact damage) + Gilded Kitten, Lone Hunter, Kitten Drag, Kitten
   From Signature, Kitten Kill Explodes (F13), Grand Finale. No spatial grid: measured unnecessary (doc 04 §6).
3. **DONE (18)**: the aura (contact, Crowd Work, Aura Slow, Overkill from aura kills: F16), the Snare/Deadweight, Gridlock, the
   slow's life and Traps, the damage zone (trail, echo, its ceiling), Edge Burn, the pulse + Standing Wave, Rainbow Poop (Poop
   Trail, Poop Duration), the cat's stillness clocks (Carry, Long Lunch's bank).
4. **DONE (18)**: Pounce (ownership, cooldown, the press edge, Reflex, the vulnerable bonus, Chain, Kitten Mimic, the crater card,
   the Daredevil's Cone of Shame, Nine Lives Insurance, Crash Landing, Bloodlust, Momentum). (The product cap is `PcCombat`'s.)
5. **DONE (18)**: Anvil Paw, Butcher's Rhythm, Inscrutable, Riddle (F17), Hammerfall; Cat Burglar's Cut, Overflow, the Catnip
   Bomb, Long Lunch, Levy; Scavenge + Pickpocket on a despawn (missed by slice 1); Pied Piper, Sixth Sense, Sleight of Hand; the
   Prism (beam, Refraction, Full Spectrum, Spectrum kills, Critical Mass).
6. **DONE (18)**: the boss (bait summon all-or-nothing, phases by HP, the shield cycle and the relocation on each opened window,
   drift; the fight clock from the first engagement, the timeout: it leaves and the bait is spent), its adds (launch, re-aim,
   heal on arrival, intercepted by the cat's whole damage and the poop), everything else reaching it (paw when no mouse is in
   reach, aura while engaging, zones, pulse, poop, Pounce, the kittens' add-then-boss duty), the defeat (credited at once) and
   the travel after the death sequence, Auto-Bait, Auto-Travel. **Correction**: the exit criterion below said "timeout sends
   back a biome and turns auto-travel off"; Godot 4.7.1 does neither (`_check_boss_time_limit()` withdraws the boss and nothing
   else), and Godot is the spec until parity. The criterion now reads as Godot behaves.
7. **DONE (19)**: the engine side. `FPawRealmSim` (the fixed step over a real frame, the 8-step guard, the snapshot the
   presentation interpolates from, the frame's events, travel, the Reawaken's deferred travel), `UPawRealmSubsystem`,
   `UPawGameSubsystem` (the data from the DataTables, the progress), `APawRealmView` (orthographic camera, the input, effects
   from events, interpolated drawing) over `UPawSpriteRenderer` (spike S2's instanced quads: floor, zones, shadows, bodies,
   effects; pooled, parked, one batch a layer) with four generated materials (`tools/create_field_materials.py`; placeholder
   discs until phase 6's atlas), placed in L_Realm by `tools/create_maps.py`; the debug HUD (`paw.DebugHud`); the cheats as
   console commands (`paw.Cheat.*`, rules in `PcCheats`: found F18; the F1 panel is phase 3's UI).

**Exit criteria** (`Paw.Realm.*` = the `Pawpocalypse.Realm.*` tests): `Paw.Realm.*` tests: card effects run, stress (320 enemies) frame time inside budget, kitten spread, overkill
never chains, boss timeout withdraws the boss with the bait spent and nothing credited, a refused summon keeps the bait;
travel < 16 ms; frame-rate independence test green; a headless 1000x PawCore soak of the first three biomes clean.
**Status 2026-10-31**: the PawCore halves are met: kitten spread (Godot's own `check_kitten_spread`, ported), Overkill never
chains, the boss timeout, a refused summon, the 320-mouse step (0.093 ms) and the soak (3 seeds x 3 game hours through the
first three biomes, boss fights included, every step's invariants held, at 1,100-7,800x real time). **The engine halves are
met (2026-10-31)**: `Pawpocalypse.Realm.*` (7 tests, 16 sabotages caught): frame-rate independence (30/50/60/144/240 fps and
jittered, bit for bit), travel 0.006 ms (the field) / 0.03 ms (with the redraw) against 16 ms, the 320-mouse frame (advance +
draw) 0.064 ms in the editor build against the 4 ms game-thread budget. **Closed 2026-11-01**: `Pawpocalypse.Realm.CardEffects` (23 card effects observed at their consumers in a live game
instance, each against a no-card control on one seed; the 3 cardless ones asserted; coverage over every effect) and
`.EveryCard` (the whole deck at once), 29 sabotages caught; the packaged gate (`py tools/packaged_bench.py`, `paw.Bench`): a
packaged Development build at 1280x800 on the dev PC (RTX 5070), 320 mice + 20 kittens: game thread 1.40 ms mean / 2.73 ms p99
(budget 4 ms), render thread 2.89 / 8.79 ms, GPU 0.40 / 0.43 ms, 13.6 draw calls (budget 50), the realm's steps + draw
0.08 ms; the cooked L_Realm draws every body, shadow and the floor (screenshot checked). Its first run failed for real: L_Realm
was not cooked (fixed with `MapsToCook`, doc 10 §1).

## Phase 3 - progression UI

**DONE 2026-11-02, but the packaged gate (met in phase 4's, 2026-11-04)**, in seven slices (HANDOFF-LOG (21)-(29)): the
Common UI root layout and the HUD (status row, zone plate, wave line, XP bar, Pounce clock, boss panel; the dock with six
tabs, badges, side flip and the Upgrades page with bulk buy and hold-to-buy), `PcHud` (the decisions) in PawCore, the
string tables and the colour tokens (21); the Realms and Reawaken pages (22); the Catches, Achievements and Jobs pages, and
Territory wins after a Reawaken (F20) (23); the card pick screen (24); the skill tree screen, browse and the Reawaken spend
step, `PcTreeLayout` exact against Godot (24, 25); the Reawaken ceremony and the job picker in hud.gd's order (26); the
victory screen and numbers printed Godot's way (27). Exit: the screenshot tour beside Godot (28: F21 found) and
`Pawpocalypse.UI.Playthrough`, a whole run through the UI's entry points (29: F22 found). Godot has no achievement toast.

**Steps**: Common UI root layout; HUD screen with the dock (6 tabs, badges, side flip), status row, Pounce clock, XP bar, boss
banner; shop rows with bulk buy and hold-repeat; Realms with gates and Bait; Reawaken tab + confirm + ceremony; card pick; skill tree
screen (lattice layout port, pan/zoom, states, tooltips, Unicorn ceremony); job picker; achievements tab with claims; victory screen.

**Exit criteria**: `Paw.Playthrough` (reset, earn, buy, travel, kill bosses, Reawaken, buy nodes, the ending) green through the UI
entry points; tree seating/spacing/crossing tests green; screenshot tour of every tab and the tree reviewed against the Godot shots.

## Phase 4 - title, saves, settings, pause

**Status 2026-11-03 (HANDOFF-LOG (31))**: started. Before it, the designer's F21 (the job picker and victory screen wear the game
theme) and F22 (the Realms rows ask this run's passage) were fixed in Godot and the port followed. Slice 1 DONE: the save in
PawCore (`PcSave`: Godot's PawpocalypseSaveData field for field, save_game() / load_game() exact against the real save_manager.gd,
fixture `save`; the file: header + JSON, every double exact, checksum; `PcJson`). Slice 2 DONE: the container
(`Source/Pawpocalypse/Save`: `UPawSaveSubsystem` over `IPawSaveStorage` (ISaveGameSystem / a folder / memory): five slots, the
atomic write and startup recovery, backup fallback, newer builds never touched, the end-of-frame save, autosave every 30 s of
unpaused play, focus loss, suspend and quit; the sandbox: `-NoSave`, `-PawSaveDir=`, automation never touches real slots); the
per-slot automations moved onto `UPawGameSubsystem::Slot()` (and the tree's Auto-Travel / Auto-Card now switch them on, a port
bug fixed); `Pawpocalypse.Save.*` (8 tests). Slice 3 DONE 2026-11-04 (HANDOFF-LOG (37)): the title screen on L_Title
(`UPawMainMenu`; its rules `PcTitle`, exact against the real main_menu.tscn, fixture `title`; `Pawpocalypse.UI.Title.*`, 6 tests,
the words equal Godot's on 300 rows; a -game run from L_Title through a new game into L_Realm). Waiting on later phases: the
garden, Boba's photo and cameo, the cluster's icons, the music (6), the Language list (7). Every UI class became an editable
Widget Blueprint 2026-11-04 (HANDOFF-LOG (38)). The pause menu DONE 2026-11-04 (HANDOFF-LOG (39)): `UPawPauseMenu` /
`WBP_PauseMenu` (its rules `PcPause`, exact against the real pause_menu.tscn, fixture `pause`; `Pawpocalypse.UI.Pause.*`, 6
tests, the words equal Godot's on 300 rows); the HUD opens it on Esc / Start. The settings popup DONE 2026-11-04 (HANDOFF-LOG
(40)): `UPawSettingsDialog` / `WBP_SettingsDialog` over `UPawSettingsSubsystem` (`PcSettings`: Godot's settings.cfg, exact
against the real settings_manager.gd and settings_dialog.tscn, fixture `settings`; `Pawpocalypse.UI.Settings.*`, 6 tests),
opened by the pause menu and the title; the last slot used, the menu side and ask-before-Reawaken are settings now. Every
step of this phase is built. Next: the phase's exit review (each criterion below against the tests that prove it) and its
packaged-build gate (needs the designer's go: packaging). Q3 decided: no Godot save importer. F24 decided and done (the run's cards
and XP are saved, a hand on screen comes back up); F23 fixed with every other open finding (the second job is saved). The nightly `-Full` after it: HANDOFF-LOG (33).

**Steps**: save container (doc 08 §2) with temp/backup/CRC/queue/async, five slots + metadata, autosave rules, request-save
coalescing, new game clears everything; title screen (slots, garden, Report a bug, Discord); pause menu; settings popup (audio,
video, controls placeholders). (No Godot save importer: Q3 = no, 2026-11-03.)

**Exit criteria**: round trip of every field; corrupt-slot fallback; crash-between-renames recovery; busy-state saves (card hand,
boss engaged, ceremony); previous-version load test; designer's save dir untouched by every test (sandbox check).

**Exit review, 2026-11-04 (HANDOFF-LOG (41))**: each criterion and what proves it (every one also sabotaged):
- Round trip of every field: `PcSaveTests` (Godot's save_game() / load_game() on 3,000 states, field for field; the file
  bit for bit) and `Pawpocalypse.Save.RoundTrip` (every saved field through the container).
- Corrupt-slot fallback: `Pawpocalypse.Save.CorruptFallback` (the backup loads; both bad: nothing loaded, nothing lost) and
  `PcSaveTests` (a damaged file is refused, never half-read).
- Crash-between-renames recovery: `Pawpocalypse.Save.Atomic` and `.CrashRecovery` (every crash point of the temp / backup
  / rename sequence).
- Busy-state saves: the card hand `Pawpocalypse.UI.CardPick.SavedHand` (a hand on screen comes back up, F24); the ceremony
  `Pawpocalypse.UI.Ceremony.Resumes` (a Reawaken in progress resumes at the spend step, F29); a boss engaged
  `Pawpocalypse.Save.MidFight` (new: the save loads, the boss not beaten, nothing up; the Bait paid stays spent: F36; since 2026-11-11 the summon's Bait and Catnip come back, the designer's call).
- Previous-version load: `Pawpocalypse.Save.Versions` (v5 resets as Godot does and is written back; a newer build's file is
  never read or overwritten).
- The designer's files untouched: `Pawpocalypse.Save.Sandbox`; the settings share the slots' sandbox
  (`PawStorageForThisRun`; every `Pawpocalypse.UI.Settings.*` test asserts an automated run has no storage of its own);
  check_all's "Godot repo untouched".
- The packaged gate, PASS 2026-11-04 (packaged Development, 382 MB): 320 mice + 20 kittens at 1280x800 (RTX 5070), the
  field running every measured frame; game thread 1.30 ms mean / 2.58 ms p99 (budget 4), render thread 1.35 / 2.00 ms, GPU
  0.36 / 0.38 ms, 34.6 draw calls (budget 50; 13.6 at phase 2, before the HUD's screens); the packaged title screen seen.
  Found by it: the game target did not compile (editor-only API in a test since (38); `check_all` now builds the game target
  too), and the bench had been measuring a field held by the card pick (now Auto-Card on, and a FAIL when the field is held).

**Phase 4 CLOSED 2026-11-04 (HANDOFF-LOG (41)).** The build: `G:\My Drive\Games\Pawpocalypse\Pawpocalypse-UE-phase4-Development-2026-11-04.zip`.

## Phase 5 - pad and keyboard parity

**Status 2026-11-04 (HANDOFF-LOG (42))**: started, in slices: (1) the input map as data and Enhanced Input assets; (2) the game
and every screen reading the actions (pounce, steer, the dock, the tree, the menus' Back / Pause / sections), Common UI's
Click / Back from the same actions; (3) the device subsystem (family, cursor source, toasts, the warp echo, the stick dead zone
setting) and the glyphs; (4) remapping (InputBindings' rules in PawCore, against the real input_bindings.gd) and the Controls
page; (5) the focus tours, the pad-only playthrough tour, the paw cursors, the unfocused throttle, the AZERTY check. Slice 1
DONE: `Data/InputMap.csv` (Godot's 72 events), `IA_*` / `IMC_Pawpocalypse` generated from it, `UPawHalfAxis` (Godot's axis
dead zone, measured: fixture `input_axis`), `Pawpocalypse.Input.*` (2 tests). Slice 2 DONE (HANDOFF-LOG (43)): the field's
pounce on `IA_Pounce`, every screen's keys through the map (`PawInput::KeyIs`), the menus' navigation from Godot's `ui_*`
(`FPawNavigationConfig`: WASD, keypad Enter), the tree's pan through Godot's `get_vector` (measured), `Pawpocalypse.Input.*`
(7 tests). Slice 3 DONE but the glyphs (HANDOFF-LOG (44)): PawCore `PcDevice` (exact against input_device.gd, fixture
`input_device`), `UPawInputDeviceSubsystem` (every Slate event, the hidden pointer, the toast `WBP_Toast`, the stick dead
zone), the stick-steered cat with `recentre` (`stick_velocity` measured), `Pawpocalypse.Input.*` (11 tests). Slice 3b DONE
(HANDOFF-LOG (45)): the glyphs (Godot's 65 SVGs rasterised and imported as UI textures, `tools/raster_svg.py` /
`tools/import_art.py`), `UI/PawGlyphs` exact against InputGlyphs (fixture `input_glyphs`), `UPawPromptIcon` /
`UPawPromptText` (Widget Blueprints) in Godot's places (the dock's tab keys, legend and Pounce key, the title's hint, the
tree's legend and tooltip, the press prompt on the focused button), grafted into the existing screen assets
(`bake_widgets.py --graft`); `Pawpocalypse.Input.*` (13 tests). Slice 4 in three parts (HANDOFF-LOG (47)): **4a DONE**,
the rules (`PcBindings`, exact against input_bindings.gd, fixture `input_bindings`; settings.cfg's `[bindings]` as Godot
writes it); **4b DONE**, the map (`PawInput::ApplyBindings`: the live mapping context every key question and the field read,
the Godot <-> Unreal key table, the generated defaults `PawInputDefaults.h`, the settings' Controls setters, the prompts and
the menus following a remap; `Pawpocalypse.Input.*` 16 tests). **4c DONE** (HANDOFF-LOG (48)): the Controls page
(`UPawControlsPanel` / `WBP_ControlsPanel` over PawCore `PcControls`, exact against controls_panel.gd in the real dialog, fixture
`controls`). **Slice 4 DONE.** Slice 5 (HANDOFF-LOG (49)) in two parts: **5a DONE**, keys by POSITION on any keyboard layout
(`FKey`s are the layout's, not positions: AZERTY and the designer's Turkish Q were wrong before), the dock's held keys
(`PcDockRepeat`, exact against dock_navigator.gd; hold-to-buy on Space / A / LT; the dock takes its keys before Common UI),
the paw cursors, pads ignored in the background; **5b DONE**, the pad-only playthrough with the focus tours
(`py tools/padplay_tour.py`, 41 checks, nightly). Not ported: the unfocused frame cap (not in Godot: doc 01 Q10).
**Phase 5's exit criteria**: met but the hand-checks on a real Xbox pad and a PlayStation pad through Steam Input, the paw
cursor in a packaged build, and the AZERTY test on a real AZERTY keyboard (injected here); those wait for the designer.

**Steps**: every Godot action in Enhanced Input (doc 05 §3) with both devices; device subsystem (family, cursor source, connect toast,
warp echo); glyph families and prompt text/icons; dock navigation; every screen's focus wiring; tree navigation; remapping UI and
rules; hardware cursors (paw open/closed); unfocused throttle.

**Exit criteria**: `Paw.Input.*` and the pad/mouse tours green; the padplay tour plays the whole loop with a pad alone; hand-check on a
real Xbox pad and a PlayStation pad through Steam Input; the AZERTY test.

## Phase 6 - presentation

**Status 2026-11-05 (HANDOFF-LOG (50))**: started, in slices: (1) the field's art: the generators in `tools/art`, the atlas,
the creatures drawn as Godot draws them; (2) the backdrops (the 12 tiling floors and their props, `biome_backdrop.gd`, seams
at 16:9 to 32:9); (3) the VFX (aura ring, poop zone, telegraph, swipe, slam, beam, the Lone Hunter halo; bursts, shards,
edge fire, the golden sparkle; the effects setting; planned as `M_FxShape` + Niagara, built as Godot's draw calls); (4) audio (the event table, buses, music crossfades, rate
limits, pruning both ways); (5) the UI art (card, skill, job, achievement and currency icons) and UI motion (`ui_juice.gd`,
rainbow bar and frame, the tree's sky); (6) the ceremonies (unicorn), Boba in the menus, the tutorial overlay; then the exit
tours (12 biomes x 7 stages x 5 bosses against Godot, the colour gate packaged). **Slice 1 DONE** (HANDOFF-LOG (50)):
`tools/art` byte-identical to Godot's `assets/art` (`py tools/art/build_art.py --check`), the field atlas
(`py tools/pack_atlas.py`, `DA_FieldAtlas`), `M_Sprite` / `M_SpriteSoft`, `UPawSpriteRenderer::AddSprite`, and the view's
creatures in Godot's look and draw order (`Realm/PawFieldLook.h`; `Pawpocalypse.Realm.Look.*`). Not done in slice 1 and owned
by later ones: the Lone Hunter's halo (a placeholder soft disc until the VFX slice), the golden sparkle (VFX), the per-creature
visual definition assets (doc 07 §4: the look reads the data's CSV columns and Godot's code constants; an editable
`UPawVisualDefinition` waits until a slice needs it). **Slice 2 DONE** (HANDOFF-LOG (51)): PawCore `PcBackdrop` exact against
the real biome_backdrop.gd (fixture `backdrop`), the floors (`T_backdrop_tier_N`, `M_Floor`), the patches and puddles (one
procedural mesh, `M_BackdropFill`), the props in the atlas, the title's garden (`UPawGarden`); the biome tour beside Godot
(`py tools/godot_tour.py --set biomes`). The seam test across aspects is the generator's own (`build_art.py --check`) plus
the exact layout at 4:3 to 32:9 in the fixture. **Slice 3 DONE** (HANDOFF-LOG (52)): the effects (`FPawFxCanvas`, `FPawVfx`; doc 04
§7 as built), the Lone Hunter's halo and the golden sparkle owed from slice 1 included; `Pawpocalypse.Realm.Vfx.*`; F39.
**Slice 4 DONE** (HANDOFF-LOG (53)): the sound (doc 07 §6 as built; `Pawpocalypse.Audio.*`; F40); open from it:
`boba_cameo` / `unicorn_neigh` (slice 6). The Output device choice and the pad-audio toast DONE 2026-11-05 (HANDOFF-LOG (55)). The nightly `-Full` ran 2026-11-05/06,
every failure fixed (HANDOFF-LOG (56)). **Slice 5a DONE 2026-11-06** (HANDOFF-LOG (58)): the theme's nine-patch chrome
where Godot's theme draws it (`PawUi::Chrome`; buttons, settings lists, the WindowPanel, the theme bars, the separators: F41),
the selection ring painted over the plate (hover, or keys / pad focus), `Pawpocalypse.UI.Chrome`, 13 sabotages CAUGHT, the
tour beside Godot. **Slice 5b DONE 2026-11-06** (HANDOFF-LOG (59)): the icons by constructed name and icon_path (`DA_Icons`,
`PawUi::Icon`), the dock's quick bar, `Pawpocalypse.UI.Icons`, 17 sabotages CAUGHT, the tour beside Godot. The nightly `-Full`
after it: every gate green. **Slice 5c DONE 2026-11-06** (HANDOFF-LOG (60)): the motion, UiJuice (`PcJuice` exact against
Godot's Tween and the real ui_juice.gd, fixture `juice`; `FPawJuice` at Godot's call sites; the Reawaken tab's glow; F42),
`Pawpocalypse.UI.Juice.*`, 19 PawCore + 19 realm sabotages CAUGHT. **Slice 5d DONE 2026-11-07** (HANDOFF-LOG (62)): the
rainbow bar, the zone plate's rainbow frame and the tree's sky (`PcRainbow`, `PcSky` exact against the real scripts, fixture
`rainbow`; `UPawRainbowBar`, `UPawRainbowFrame`, `UPawTreeSky` painted in Slate) and every bar filled Godot's way (caps kept,
the Range's step), `Pawpocalypse.UI.Rainbow.*`, 26 PawCore + 17 realm sabotages CAUGHT. **Slice 6 started 2026-11-07** (HANDOFF-LOG (63)): the plan in
`docs/WORKLIST-SLICE6-2026-11-07.md` (6a the Unicorn ceremony, 6b the Reawaken banner, 6c Boba, 6d the tutorial). **6a DONE**:
`PcTween` + `PcUnicornCeremony` exact against the real unicorn_ceremony.gd (fixture `unicorn`), `UPawUnicornCeremony` over
the tree, `Pawpocalypse.UI.Unicorn.*`, 19 PawCore + 8 realm sabotages CAUGHT, the tour's `unicorn` shot beside Godot.
**The restyle (D14) runs before 6b-6d** (designer, 2026-11-07; the section below). Then 6b, the Reawaken banner; 6c, Boba;
6d, the tutorial; the exit tours.

**Exit criteria progress (2026-11-05)**: the 12 biomes + the title seen beside Godot (`--set biomes`; one stage, no bosses side
by side yet); pruned audio both ways (`Pawpocalypse.Audio.Bank`); not yet: the colour gate on a package, every audio event
in a logged tour, the 7 stages x 5 bosses tour, the tutorial gate.

**Steps**: copy the art generators into `tools/art/`, raster + atlas + headless import for all 377 SVGs; backdrops for 12 biomes
(tiling floors, props); visual definitions; cat stages with weapons and hats; signature glow, golden sparkle, flashes, pulses; VFX
primitives (planned SDF shapes + Niagara; built as Godot's draw calls) with the effects setting; audio event table, buses, music crossfades, rate limits; ceremonies;
Boba cameo in menus; tutorial overlay (eleven cues); UI motion.

**Exit criteria**: colour gate green on the packaged build; screenshot tour of all 12 biomes x 7 stages x 5 bosses reviewed against
Godot (since D14 for layout, state and behaviour: the look is the port's own and is reviewed on the concept canvas); every
audio event plays once in a tour (logged); pruned audio both directions; tutorial gate green.

## The restyle (D14, inside phase 6)

The designer, 2026-11-07: "The unreal version does not have to match godot" and "Plum everywhere." The plan, its decisions
and each step's exit are in `docs/WORKLIST-RESTYLE-2026-11-07.md`; the reference is the concept canvas
(https://claude.ai/artifact/3wbqpr2HT4daaSmtHtNT2K). Status 2026-11-10:
- **R0-R4 DONE** (HANDOFF-LOG (65)-(70)): the palette is `tools/art/palette.py`, Plum ink, M PLUS Rounded 1c, plum chrome
  with Deep Plum edges and one solid shadow, Butter focus and selection, the 16 px text floor, every screen fits at 150 % UI.
  The plain-text pass (D15) on top (HANDOFF-LOG (71), (72)). Committed and pushed as `b0ce7be`.
- **R5, the field: drawn; waiting on the designer's last reviews** (HANDOFF-LOG (73)-(90), uncommitted): D3 (the aura, the
  strike paw, crits) built; the look lint everyday; the faces; the anatomy pass with one colour per character (batches 2-5
  written); the rig (wings and legs as moving layers) in the game; the props redrawn and written. Waiting: the bosses and the
  imp (drawn, on the canvas), the rig's motion, the cat's pale coats (look lint KNOWN 23), the packaged frame gate (doc 01 Q11).
- **R6** (icons, badges, cards) and **R7** (sign-off): to come.

## Phase 7 - localisation

**Steps**: string tables for every player-facing string (doc 08 §4); the text checker; `tools/port_po.py`; Dashboard import;
composite fonts with CJK/Thai (DONE early in restyle R2: `F_Paw`, doc 07 §5); pseudo-locale tour; language menu. The 279 lines of the plain-text pass (D15, 2026-11-09) have no Godot translation to carry over: `port_po.py` lists them for translators (their Godot English is `Data/TextOverrides.csv`'s `godot` column).

**Exit criteria**: 20 cultures complete (or a reviewed exception list of strings new to the port); placeholder sets identical;
font coverage test green (`tools/check_font_coverage.py`, everyday since R2); no overflow in the pseudo-locale tour at 1280x800 and 150% UI scale.

## Phase 8 - hardening and packaging

**Steps**: every gate in `tools/check_all.ps1` (everyday and `-Full`); packaged gates (a `tools/package.ps1`, doc 10 §2); the packaged soak at 5x for the whole game and 20x for 30
minutes; Steam Deck / 1280x800 performance pass; package script with listing audit; chunking + patch-size measurement (doc 10 §3);
PSO precache check; Shipping build; crash/log collection.

**Exit criteria**: soak "no problems found" with 0 engine errors over a whole game; p99 frame time recorded and not worse than the
Godot build on the same PC; 60 fps at 1280x800 on the Deck-class target; a one-number balance patch measured at a small delta.

## Phase 9 - parity sign-off

**Steps**: side-by-side review (the same game state set up in both builds by their cheats: no Godot save importer, Q3 = no; same
biome/stage screenshots; pawsim length report unchanged); the designer plays the Unreal build; differences become tickets: "port bug" or "design change for later", never both.

**Exit criteria**: the designer signs off; the Unreal build becomes the shipping build; the Godot project is archived (its `/data`
exporter and `_godot_file` columns retired, doc 06 §2).

## Standing between phases

- After each phase: update docs 00-12 where reality changed (a doc that disagrees with the code is a bug), add new technical lessons to
  `C:\Games\Knowledge` with confidence tags (dimension folder for 2D-only facts, engine folder otherwise), re-run the full gate set.
