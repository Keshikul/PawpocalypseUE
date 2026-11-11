# 01 - Port brief, scope and decisions

## 1. The brief

Rebuild Pawpocalypse in Unreal Engine 5.8.3 **as the same game**: same rules, same numbers, same content, same sounds,
same controls on keyboard/mouse and pad, same twenty-one languages, same save behaviour. The Godot game
(`C:\Games\pawpocalypse`) is the specification for all of that: where this handoff and the Godot code disagree on a rule
or a number, the **Godot code is right** until phase 9 signs off parity, and this handoff is the bug, **unless the Godot
code is at fault** (D16).

**Three exceptions, all the designer's:** the **look** is the port's own since 2026-11-07 (D14: "Plum everywhere"), and so is
the **player-facing English** since 2026-11-09 (D15). Their references are the concept canvas,
`docs/WORKLIST-RESTYLE-2026-11-07.md` and `Data/TextOverrides.csv`, not Godot. And since 2026-11-10 this is **a standalone
game whose Godot faults are fixed, not ported** (D16: "This is a standalone version. If godot has rules or mechanics that
don't make sense we should change them. Do not port faults from godot."): §5 lists them.

**Why it is worth doing carefully:** the Godot project's own history (its handoff is 5,742 lines) is a record of
traps that cost days each: rewards that stopped tracking HP, a build that loaded zero evolution stages, a
damage term that disappeared, gates that passed while proving nothing. Almost every one of those has an Unreal
equivalent. Doc 12 lists them side by side. **The economy and its invariants are the most valuable thing being
ported, more valuable than any code.**

### 1.1 Scope

| Ported as-is (content and design) | Rebuilt (engine side) | Not ported (Godot-only) |
|---|---|---|
| Every `/data` number: 12 biomes, 59 enemies, 5 bosses, 7 stages, 15 shop lines, 28 cards, 87 tree nodes, 10 jobs, 39 achievements (41 until F2, 2026-11-03), combat and Reawaken constants (`Data/*.csv`) | All gameplay code (79 GDScript files, appendix B) in C++ | `PackLoader` and `.pck` splitting (Unreal has pak/IoStore chunks, doc 10) |
| The art generators (`tools/art`, copied from Godot in phase 6; this project's own since D14) | The renderer for sprites, shadows and VFX primitives (doc 04) | `ResourceDir` and the `.remap` trap (Unreal's equivalent trap is the cooker, doc 12) |
| The audio files (Kenney CC0, Abstraction music, the synthesised whinny) and the `EVENTS` table | The UI: every screen as Common UI widgets and editable Widget Blueprints (doc 05) | `check.sh`'s script reload trick, `--headless` quirks, `.uid` files |
| `pawsim.py` (re-pointed at `Data/*.csv`) | Saves, settings, input, localisation plumbing | Godot export presets, `embed_pck`, the `itch-release` preset files |
| The 20 translations (`locale/*.po`) | Godot's gates, as Unreal automation tests plus Python checks (doc 09) | GL Compatibility renderer specifics |
| Every design rule in the Godot `pawpocalypse.md` | The soak driver, trailer director and store-shot probe; the release scripts (UAT, SteamPipe, butler) | Movie Maker mode (Unreal: Movie Render Queue) |
| | **The look** (D14: palette `tools/art/palette.py`, plum chrome, M PLUS Rounded 1c) and **the English** (D15) | Godot's Stacklands cream look |

### 1.2 Out of scope until parity

No new content, no new features, and no balance change but a Godot fault's fix on the designer's yes (D16). Open design
questions (the kitten/noticeable-buy trade-off, the three card effects with no card, jobs never judged as a system) are
answered here with the designer, not in Godot (D16). Porting and redesigning at once makes every difference ambiguous, so
every departure from Godot is listed (§5) with the test that asserts it: "is this a port bug or a change?" must always have
one answer. The look and the text (§1) are the port's own.

## 2. Decisions taken by this handoff

Each has a reason. Re-opening one needs a measurement, not a preference.

| # | Decision | Why | Status |
|---|---|---|---|
| D1 | **UE 5.8.3 launcher build**, pinned (`5.8.3-58210709`); no engine upgrade until after launch | It is the version the knowledge base was verified on; 5.8 is the last planned major UE5 release `[KB U/01]` | Taken |
| D2 | **All gameplay logic in C++.** Blueprints only for visual tuning (sprite/VFX/widget looks). A **pure C++ core library** (`PawCore`, no engine dependency) holds every formula: combat, prices, Reawaken, evolution, gates | Testable in seconds, mirrors `pawsim.py`, and UE6's planned Blueprint sunset makes C++ the durable choice `[KB U/01 §1, U/03 §4]` | Taken |
| D3 | **CSV in `Data/` is the source of truth**, imported headless into DataTables; enums by NAME; lower-case string ids | Text diffs, scriptable, the simulator reads the same file; FName case changes between editor and packaged builds `[KB U/05]` | Taken |
| D4 | **Single player, no networking code at all.** No replication, no Iris, no sessions | The game has none; the KB's co-op machinery (GameState replication, server RPCs) is cost without benefit here | Taken |
| D5 | **The field is drawn in world space under a top-down orthographic camera, by our own instanced sprite renderer**, not one Actor per mouse | Hundreds of moving entities; one Actor each is the classic UE slowdown (300 flipbook actors measured ~20 fps in a forum report) `[Forum]`; Paper2D gets little development `[Forum]` | Taken: spikes S1/S2 (doc 04 §10); built as `UPawSpriteRenderer` (instanced quads, atlas pages) |
| D6 | **UI is Common UI + UMG, screens in C++** (`UCommonActivatableWidget`), styled by brushes and one token header generated from `palette.py` | Pad support from day one `[KB U/08]` | Taken; spike S4 proved the dock and the tree. Since 2026-11-04 (designer) every screen and piece is ALSO an editable Widget Blueprint over its C++ class (baked once, the asset owns the layout: CLAUDE.md, doc 05) |
| D7 | **Input is Enhanced Input + Common Input**, every Godot action mapped 1:1 (doc 05 §3) | The Godot game's input contract is already console-grade | Taken; built in phase 5 (Godot's map as data, a live context from the player's bindings, devices, glyphs, remapping, keys by position on any layout, the dock's held keys, the pad-only playthrough: doc 05 §3-5) |
| D8 | **Saves are our own versioned text format** (JSON) with magic, version, CRC, temp+rename, a backup and five slots; not a raw `USaveGame` | The Godot save is a flat text resource with atomic writes; a renamed `USaveGame` class loads null `[KB U/10]` | Taken; built in phase 4 (doc 08) |
| D9 | **FText everywhere; string tables**; the 20 existing translations are carried over by script from `locale/*.po` | 20 languages at 100% are an asset; Unreal imports PO through the Localization Dashboard `[Docs]` | Taken; the converter is phase 7 (D15's changed English lines need translating then) |
| D10 | **Art stays generated SVG**, rasterised by script (resvg) to PNG at the sizes the game needs, packed into atlases, imported headless | Unreal has no production SVG texture import (Epic's `SVGImporter` is an Experimental VirtualProduction plugin) `[Docs]` | Taken; spike S3 sized it; built in phase 6 (doc 07) |
| D11 | **Fixed-timestep simulation** (60 Hz) driven by one world subsystem, with a game-speed scale for the soak | Continuous `dps * delta` damage and the pawsim mirror need determinism; Godot's per-frame delta made the soak's 20x lie about pace `[KB U/03 §4.1]` | Taken |
| D12 | **The Godot build keeps shipping** (itch, and the Steam demo if Next Fest comes first) until phase 9 | A port that is 80% done is a game that does not work | Taken; see Q4 |
| D13 | **Double precision for every economy value** (`double` in C++, never `float`) | Godot's `float` is 64-bit; Catnip passes 1e12 and boss HP and DPS climb ten orders of magnitude. A C++ `float` is 32-bit (7 digits) and silently changes the economy | Taken; gated (doc 09) |
| D14 | **The look leaves Godot (2026-11-07, designer)**: "The unreal version does not have to match godot, they can be different." and "Plum everywhere." Font M PLUS Rounded 1c; the cat stays grey | The Stacklands cream read too sweet; the designer chose the original field with plum menus on the concept canvas (https://claude.ai/artifact/3wbqpr2HT4daaSmtHtNT2K). Rules, numbers, behaviour and timing still follow Godot exactly | Taken; plan `docs/WORKLIST-RESTYLE-2026-11-07.md` (R0-R4 done; R5, the field, drawn and waiting on the designer's last reviews; R6-R7 to come). Supersedes Q6 and the Stacklands look |
| D15 | **The player text is the port's own (2026-11-09, designer)**: plain and informative, real numbers, one word per idea ("increase", "permanently", "enemies" not "mice"): "all the text should just be to the point informative information"; on the drafts: "All your picks are good but change mice to enemies". Grudge Ledger shows as Stacked Deck (id `grudge_ledger` kept) | 279 lines changed. Godot keeps its words (D12: it keeps shipping, and its loc gate needs every English line translated), so the text lives here: `ST_UI` directly, `Data/TextOverrides.csv` for the tables generated from Godot; Godot-parity tests run on Godot's words (`PawTextTesting`) | Taken; HANDOFF-LOG (71), (72); doc 08 §4; F46 asks whether Godot takes the same text |
| D16 | **A standalone game: Godot's faults are fixed, not ported (2026-11-10, designer)**: "This is a standalone version. If godot has rules or mechanics that don't make sense we should change them. Do not port faults from godot. This is a standalone project" | A copied fault ships in the new game and must be found again later. Godot stays the reference for everything that is not at fault, and its fixtures stay the proof | Taken. A bug or an inconsistency is fixed as found; a change with a design or balance trade-off is proposed (balance with pawsim numbers) and made on the designer's yes. Made here, never in the Godot repo; listed in §5 with its test. **How the Godot-fixture tests carry a change** (built 2026-11-11, the night worklist's part 2): `tools/godot_fixes.py` holds the port's fix to Godot's code per finding (exact text replaced, found once, never a no-op), and `make_godot_fixtures.py` applies them to its scratch copy only, so every other row is still proven against the real Godot code; the everyday gate `Godot fixes current` fails the day Godot's code moves under a fix. First used for F34, F37, F49 |

## 3. Questions for the designer

| # | Question | Status |
|---|---|---|
| Q1 | **What does the move to Unreal have to deliver?** (Consoles? Steam Deck performance? A different look later? Career skills?) A console goal pulls doc 10 §6 (source engine, platform abstractions) forward | Open. Recommendation: write the one-line goal into this doc |
| Q2 | **1:1 recreation first, changes after parity?** | Yes (§1.2), with three exceptions, all the designer's: the look (D14), the text (D15) and Godot's faults (D16) |
| Q3 | **Should itch players' Godot saves import into the Unreal build?** | **Decided 2026-11-03: no** ("No itch saves for unreal"): no importer |
| Q4 | **Steam Next Fest (February 2027) from which build?** The Godot plan (`pawpocalypse-steam-launch-plan.md`) targets it; the port will not reach parity by then at any realistic pace | Open. Recommendation: ship the demo from Godot; launch from whichever build is at parity |
| Q5 | **Art resolution**: rasterise for 4K (2x) so the art stays sharp on big screens? | Done as recommended: 2x rasters, mipmapped (doc 07 §2) |
| Q6 | **The look under Unreal**: exact Godot colours, or allow post-processing (bloom, glow)? | Superseded by D14 (the port's own palette; still no bloom or post-processing, CLAUDE.md) |
| Q7 | **The music licence carries over unchanged** (Abstraction's loops: credit required, the author does not endorse AI-related use; the trailer avoids them). Still your call? | Open. Recommendation: keep as-is; record it in the third-party register (doc 07 §5) |
| Q8 | **Boba** stays menus-only, never over the tree, as in Godot? | Yes (CLAUDE.md); to be built in phase 6 slice 6c |
| Q9 | **Edge smoothing (spike S3)**: sharpest art with a one-pixel stair-step on the outlines of moving creatures (no AA), or smooth outlines with the whole sprite slightly softer (FXAA)? Godot gets both by alpha-blending; Unreal sprites are alpha-tested so they sort by depth | Open; FXAA is set until then. If neither is acceptable, measure supersampling on the Deck-class GPU |
| Q10 | **An unfocused frame cap (2026-11-05)**: drop to a low frame rate in the background, as an idle game left running should. Godot has none, so the port does not either: a design change | Open. Recommendation: 30 fps unfocused, a setting to turn it off, measured in a soak; since D16 it need not wait for parity |
| Q11 | **The frame gate's draw-call budget (2026-11-10)**: docs/04 §9 budgets "< 50 draw calls for the field"; `paw.Bench` counts the whole frame. The packaged build draws 68.7: the field ~26, the HUD and fixed passes ~43 (HANDOFF-LOG (89)) | Open. Recommendation: gate the field's own draw calls against 50 and give the HUD its own line (R6's UI icon atlas lowers it); or a whole-frame budget above 69 |

## 4. Risks, honestly

| Risk | Likelihood | What we do |
|---|---|---|
| **Length of the port.** 29k lines of GDScript plus Godot's gates, the art pipeline, 21 languages and pad support | Certain | Phased plan with exit criteria (doc 11); parity before features; the Godot build keeps shipping |
| **Colours shift** (the tonemapper and exposure change unlit sprites) | High if ignored | Spike S1 proved them exact; a pixel-readback colour gate (doc 04 §3) |
| **Sprite edges alias or ghost** (masked edges are hard; temporal AA smears moving sprites) | Medium | Spike S1 compared the options (FXAA set; Q9) |
| **Sort order** between overlapping creatures | Medium | Godot's draw order as depth (doc 04 §4); a masked material's same-depth layers need `r.EarlyZPassOnlyMaterialMasking=False` (HANDOFF-LOG (80)) |
| **Packaged-only bugs** (uncooked assets, FName case) | High (the Godot project shipped one) | One always-cooked root, lower-case string ids, a packaged-build gate every phase (doc 09, 10) |
| **Build size** grows from ~200 MB (Godot) to roughly 150-320 MB of engine before content `[Forum]` | Certain, acceptable | Trim plugins for Game targets, `-nodebuginfo`, Nanite off `[KB U/19 §3]` |
| **Steam patch size** if paks are laid out badly | Medium | Chunking by churn, verified in phase 8 (doc 10 §3) |
| **Agents cannot see the editor** | Certain | Text-first: C++, CSV, ini, Python builders; screenshot tours to see the game `[KB U/23]` |

## 5. Findings in the Godot game (found by the port's tests)

The port compares every rule against the real Godot code, so it finds Godot bugs. **Since D16 (2026-11-10) the port fixes
them itself instead of copying them**: a bug or an inconsistency as found, a change with a design or balance trade-off on
the designer's yes; never in the Godot repo. Before D16 each was ported as Godot had it and fixed in Godot first (§5.2); the
Status column says what D16 does with each open one. **The designer's copy, for the Godot build (D12), lives in the Godot
repo: `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`** (same numbers, with file and line and the full
write-up). Add every new finding to both.

### 5.1 Open

| # | Finding | Where | Status |
|---|---|---|---|
| F3 | With Patience owned and the cat slower than base (the Sphinx), buying Attack Speed can LOWER paw DPS: the shop sells a trap in that build | the combat formula (design, not a code bug) | Open design note; the long suite counts it instead of failing. D16: the designer's call (a balance change: pawsim numbers first) |
| F4 | The Godot pawsim caps the kitten count at the shop cap; the game does not | `tools/pawsim.py` vs `GameState` | Known divergence, unreachable in play; the mirror keeps its states inside the cap (doc 06 §8) |
| F5 | `Amounts.short` prints an overflowed integer past ~9.2e18, and for NaN and infinity (`-9223372036854775808Qi`) | `amounts.gd` | Unreachable in play (boss HP tops out near 2e12). PawCore prints the digits, `nanQi` / `infQi` (`PcFormat.cpp`) |
| F12 | Godot's hands cannot be replayed: the deal draws from the engine's global `randf()`, which `AudioManager.play()` also draws from | `card_manager.gd` `draw_hand()`, `audio_manager.gd` `play()` | Port difference by design: `PcRng` per (seed, deal); the rules and weights are proved card for card against Godot's own stream with sounds silenced |
| F15 | Two kitten card effects (`KITTEN_DRAG`, `KITTEN_FROM_SIGNATURE`) have no card | `card_data.gd` effects 11, 13 | Open, harmless; ported, tested with a test-only card |
| F25 | Godot's `.tres` save rounds a double that is exactly a float32 to float32's digits (up to ~6e-8 relative) | the engine's text resource writer | Low; the port keeps every double exactly (doc 08 §2) |
| F34 | The pause menu's armed overwrite survives leaving the Save page: back on it, ONE press overwrites (fixture `pause`: 835 of 26,000 presses) | `pause_menu.gd` `_show_main()`, `_pending_overwrite` | **Fixed in the port 2026-11-11** (D16): `PauseShowMain` disarms it, as Godot's comment promises; the `pause` fixture is made from Godot with the same fix (`tools/godot_fixes.py`); 227 such presses in the fixture, each disarmed |
| F35 | Ticking Fullscreen does not grey Resolution until the dialog is reopened | `settings_dialog.gd` (the box's signal, `resolution_option.disabled`) | **Fixed in the port 2026-11-11** (D16): ticking the box greys Resolution at once (`Pawpocalypse.UI.Settings.FullscreenGreys`; `.Applies` asserted Godot's way before). The `settings` fixture never presses Fullscreen (headless), so no Godot-side fix was needed |
| F36 | Quitting mid-boss loses the Bait paid for the fight (the fight is not saved; `bait_spent` is) | `game_state.gd` the Bait spend; `bait_spent` | **Decided 2026-11-11 (designer: "Refund it", then "Refund Bait and Catnip"), fixed in the port the same day** (D16): a save taken during a fight is written as if its summon had not happened, the Bait back in its biome's ledger and the Catnip back in the purse (under Audit only Catnip is paid, so only Catnip comes back); a fight that ends (won, timed out, a Reawaken) keeps its cost, as in Godot. The summon records what `BuyBait` took (`PcField::SummonPaid`, `Pc::FightPayment`), `CaptureSave` gives it back in the file only. Tests: L1 "Boss: a save during the fight is the save from before the summon" (the file byte for byte), `Pawpocalypse.Save.MidFight` (turned from Godot's rule); 7 PawCore sabotages, 1 realm. No Godot fixture saves mid-fight, so no `godot_fixes.py` entry. A player can now quit a fight they are losing and retry at no cost (the timeout's cost is only paid by staying) |
| F37 | Settings > Controls: the controller column can show an old pad (fixture `controls`: 45 of 6,500 moves) | `controls_panel.gd` `sync()` | **Fixed in the port 2026-11-11** (D16): the device subsystem tells the page when the pad detected changes (`OnDetectedChanged`) and it redraws; the `controls` fixture is made from Godot with the same fix (`tools/godot_fixes.py`: a `detected_family_changed` signal): 44 rows changed, a new pad in 364 moves |
| F38 | Steered right with the stick, the cat's hat and weapon stay where they were (only the texture flips) | `player.gd` `_apply_hat`, `_apply_weapon`; `sprite_liveliness.gd` `face()` | **Fixed in the port 2026-11-11** (D16): the hat and the weapon mirror with the cat as one figure, offsets and drawings (`PawLook::FacingOverlay`, `HatFlipX`, `WeaponFlipX`; `Pawpocalypse.Realm.Look.Rules`) |
| F39 | The Cone of Shame's and the job's rings draw at twice the cat's position (`global_position` set before `add_child`) | `vfx_burst.gd` `spawn()` | **Fixed in the port 2026-11-11** (D16): both rings drawn round the cat, following it (`Pawpocalypse.Realm.Vfx.RingOnCat`) |
| F40 | Five sounds play twice, layered (a signal hookup and a direct call): Reawaken, boss defeated, skill level, new hand, card taken | `audio_manager.gd` `_connect_events` and the calls beside it | **Fixed in the port 2026-11-11** (D16): each of the five plays once (`Pawpocalypse.Audio.When`) |
| F41 | The separators draw nothing (a 4 px slice of the texture's empty rows) | `pawpocalypse_theme.tres` `SBT_separator`; `ui_separator.svg` | Open in Godot; the port's own look (D14) draws its separators |
| F42 | The Bait button's lean never settles: it restarts 15 times a second and trembles | `hud.gd` `_refresh_bait_button()` | **Fixed in the port 2026-11-11** (D16): a refresh leaves a running lean alone; it stops only with no boss to call (`Pawpocalypse.UI.Juice.BaitLean`: 5 refreshes restarted it 5 times before) |
| F43 | Grudges and the "unimpressed" cat slipped through (designer, 2026-11-08: "They were supposed to be removed in godot but slipped through"): the victory line, the skill Grudge Ledger | `victory_screen.gd`; `grudge_ledger.tres`; `skill_tree_ui.gd` | The port dropped the line and shows Stacked Deck (D15); Godot still has both: whether it takes them (F46) |
| F44 | Two skill texts promise what the code does not do: Auto-Pounce's "earn more while the game is shut" (offline earnings removed 2026-09-10); Second Helping pays at City Rooftops, not "the Park" | `unlock_reflex.tres`, `second_helping.tres`; `game_state.gd` `begin_biome()` | The port's text says City Rooftops (approved 2026-11-09); Godot's text is the one that is wrong |
| F45 | The Engineer's blurb claims "every card taken arrives a level higher"; the job has only an extra card and x0.8 damage | `job_engineer.tres` | None to decide: the port's text drops the claim |
| F46 | The port's player text differs from Godot's (D15: 279 lines) | `Data/TextOverrides.csv` (Godot's line beside each) | Whether Godot takes the same text (then re-extract the .pot and translate) |
| F47 | A slot whose file and backup are both unreadable: the game plays a fresh game in it without a word, and the first autosave after anything is earned writes over it | `save_manager.gd` `load_game()` (`push_error`, then return); `main_menu.gd` `_refresh_slot_cards()` | **Fixed in the port 2026-11-11** (D16; review CR2, batch B, the words approved that day): the title says "Slot {n} could not be read. Choose a slot.", Continue opens the slots instead of playing a fresh game over it, the slot's card reads "Cannot be read" and starts a new game there, and the writer keeps the old file as `slot_N_corrupt` before replacing it. `Pc::ClassifySlot`, `Pc::TitleMenu`'s notice; the `title` fixture is made from Godot with the same card (`tools/godot_fixes.py` F47: L1 FAILED on 90 cards before it). Tests: L1 "Title: the load's outcome reaches the player", `Pawpocalypse.UI.Title.LoadOutcomes`, `.MatchesGodot` |
| F48 | The autosave rewrites the loaded slot every 30 s while the title is up, though nothing changes (the timer's own comment: a state that is not changing is not written) | `save_manager.gd` 77-87 (`autosave_timer`) | **Fixed in the port 2026-11-10** (D16; review CR21): the 30 s timer, a focus loss and the platform's background event save only while a realm is up; the quit save and the asked-for saves are unchanged (`Pawpocalypse.Save.Autosave`; sabotages `autosave_on_title`, `focus_save_on_title`) |
| F49 | Saved card levels are not clamped on load, unlike shop and tree levels (F32) | `card_manager.gd` `apply_run_save()`; the port's `PcSave.cpp` `ApplyRunCards` | **Fixed in the port 2026-11-11** (D16, review CR22): a level under 1 is no card, one past the card's levels its top level; the `save` fixture is made from Godot with the same fix (438 rows changed) |
| F50 | The GDD ("capped at 4x") and two code comments ("ships at zero") describe the Overkill bounty's flat ceiling; the data ships the curve, `min(w, w^0.4)` (`overkill_bounty_exponent` 0.4 since the economy rework, 2026-08-24, `de61fadb`, as the economy redesign §2.2 planned), which never reads the ceiling | `pawpocalypse-gdd.md` 98; `combat_resolver.gd` 218-237; `combat_constants_data.gd` 160-184 | Docs only: the rule is the curve. The port's doc 02 corrected 2026-11-10 (review CR23); Godot's docs are the Godot build's |
| F51 | Leaving to the title without saving keeps the unsaved game: Continue plays it (it does not reload the slot), and the title's autosave (F48) writes it within 30 s, against the discard's own comment ("Leaves without writing") | `pause_menu.gd` `_on_discard_pressed()`; `main_menu.gd` `_on_continue()` | **Fixed in the port 2026-11-11** (D16): the discard goes back to the slot as last written, or to a fresh game when nothing was ever written (`UPawSaveSubsystem::DiscardUnsaved`; `Pawpocalypse.UI.Pause.DiscardDrops`, `.Leaves`). In the Godot copy since 2026-11-11 |

### 5.2 Fixed (Godot first, the port followed)

F1 was decided 2026-10-29, F20 2026-11-01, F21-F22 and F24 one by one and the rest of F2-F23 together on 2026-11-03
(designer: "fix all open findings", each finding's suggested fix), F26-F33 on 2026-11-04 ("Save it", "Save all three",
"Resume on load", "Apply all four"). Each fix is uncommitted in the Godot repo for the designer; the fixtures were
regenerated and the port's tests flipped from pinning Godot's old behaviour to asserting the fix, with their sabotages seen
CAUGHT (HANDOFF-LOG (9), (23), (32), (36)).

| # | What was wrong | The fix |
|---|---|---|
| F1 | Second Opinion granted rerolls on every kill that did NOT level up | Once per Catches level (`PcCards::AddKillCatches`) |
| F2 | Achievements `br_ferocity_5`, `br_mystique_6` could never be earned (branches merged away) | Both removed: 39 achievements |
| F6 | The job picker's comment said one held job is no choice; the code asks with one | The behaviour kept (one job plus "No job" is a choice), the comment fixed |
| F7 | Fence's window did not close on travel or Reawaken | `begin_biome()` resets `kills_this_wave` |
| F8 | The Kitten Speed lock was a UI rule only | `purchase_upgrade()` refuses it while locked |
| F9 | The achievement count included ids the roster no longer has | `earned_count()` counts roster ids only |
| F10 | A deal promised the next hand twice | One promise (`_mark_and_promise()`) |
| F11 | A reroll or banish kept the old hand's Prototype mark and Blueprint promise | Redone on every deal, reroll and banish |
| F13 | Kitten Kill Explodes (Giblets) hurt nothing: the blast landed at the parked corpse | `hunt()` emits `death_position` |
| F14 | A card that costs kittens took them late | `card_taken` triggers a re-count |
| F16 | Overkill from the aura, Pounce and Hammerfall spilled from the parked corpse's corner | `splash_overkill()` gets `death_position` |
| F17 | Riddle was spent on the swing that armed it | Only an earlier swing's arming is spent |
| F18 | The cheat "Unlock all stages" left biomes behind a boss shut | It also sets this cycle's passage |
| F19 | The puddle cards waited for Rainbow Gut, not Rainbow Poop | `_requirement_met("poop")` asks Rainbow Poop; the hint says so (translations worth a translator's look) |
| F20 | A Reawaken with Territory lost the spend step, or Territory was ignored | "Territory wins": one destination at the end of the spend step (`Pc::ReawakenTravelIndex`) |
| F21 | The job picker and the victory screen were outside the game's theme (an invisible title) | The game theme (since D14 the port's own) |
| F22 | After a Reawaken the Realms page walked past every boss without baiting it | `is_passage_open()` (`Pc::IsPassageOpen`, `PcHud RealmRow`) |
| F23 | Union Rules' second job was not saved | `run_job_second` saved |
| F24 | The run's cards, Catches level and XP were not saved | Saved, and a restored hand re-offered (`Pawpocalypse.UI.CardPick.SavedHand`) |
| F26 | The deepest biome reached was not saved, so a restart weakened the cat | `deepest_biome_index` saved (clamped on load) |
| F27 | Second Helping's deferred Stardust was not saved | `pending_second_helping` saved |
| F28 | Body Count's and Levy's counters reset on a restart | `kills_this_cycle`, `kills_since_tithe` saved |
| F29 | Quitting mid-Reawaken skipped the rest of it | `reawaken_in_progress` saved; the load resumes at the spend step (`Pawpocalypse.UI.Ceremony.Resumes`) |
| F30 | The shop greyed out a purchase the rule allowed (a 1e-6 window) | The button asks `catnip + epsilon < cost` |
| F31 | The GDD said kittens scale with evolution; the code never has | The GDD line fixed |
| F32 | Saved levels were not range-checked on load | Clamped on load (`PcSave` ApplySave) |
| F33 | A NaN or infinity survived a save | Non-finite numbers load as 0 |
