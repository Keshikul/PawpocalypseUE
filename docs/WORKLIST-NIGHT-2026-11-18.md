# The night of 2026-11-18: the restyle's remaining screens, then translation

The designer, 2026-11-18: "Add dutch and danish. Restyle then translate. Create an overnight plan." Earlier the same day:
"Check all most used steam languages as translate into all of them. Also if we make changes later make sure that we don't
have to retranslate everything and just redo things we changed."

The order is the designer's: **finish the Sticker Box restyle (S4, then S6), then localisation (phase 7)**. Each slice
ends the way every slice does, or it is not done.

## The rules for the night (CLAUDE.md, unchanged)

- **No commit, push, package or export.** Stage nothing.
- **Sandbox every run** (`-PawSaveDir`, the tests' storages); the designer's saves and settings stay byte-identical.
- **One engine run at a time; never touch another project's Unreal** (Ice and others run on this PC): wait for it.
  Never kill or close any editor.
- **Planted faults only through `safe_mutate`**; `safe_mutate --list` empty at the end of every slice.
- **Python edits write LF** (`write_bytes` or `newline="\n"`), never a blanket line-ending pass (KB Unreal/20).
- **The machine ran low on memory today** and a background gate run was stopped. Long runs go in the foreground where
  they fit in 10 minutes; a run that is stopped is reported as stopped, never as passed.
- **New player-facing words are drafted** (`docs/TEXT-DRAFT-2026-11-19.csv`) for the morning, never final.
- **Layout follows the concept (E4)**; where it does not fit (1280 x 800 at 150 % UI, the Layout tests decide) the
  reason is written down and the nearest fitting layout is built.

## 0. First: prove today's batch

`tools/check_all.ps1` (everyday) to exit 0. The run of 2026-11-18 evening was stopped after 23 passing gates (HANDOFF-LOG
(118)). Nothing else starts until it passes.

## 1. The restyle (S4, then S6), screen by screen

Each screen: the concept's drawing beside today's game (`tools/godot_tour.py --skip-godot`, the store shots, or a
scripted shot like `build/night4/slots_shot.py`); grafts into its Widget Blueprint (the asset owns the layout); a test
read from what is drawn, seen to FAIL on the old asset and PASS after; a sabotage per new behaviour, seen CAUGHT; the
Layout tests at 4 sizes; a screenshot for the morning page.

| # | Screen (concept) | What changes |
|---|---|---|
| 1.1 | HUD: Upgrades (scrUpgrades) | the buy amount as its own row with the x1 / x5 / x10 / Max switch; the toast; the tutorial tip's plate; the status pills and the bottom-left level bar as drawn **DONE** (HANDOFF (119)); the tutorial tip waits for the tutorial (not ported) |
| 1.2 | HUD: Reawaken (scrReawaken) | the payout before the button: Stardust, the total, both multipliers before and after; **DONE** (the payout box, the check box above the yellow button; the rainbow progress bar kept: Godot's ready signal) |
| 1.3 | HUD: Realms (scrRealms) | rows that say what opens them; the rainbow frame on the zone plate and the Bait box while a summon is ready; **DONE** (icons on every row, the cat's biome yellow, Auto-travel under the map; the Bait box's own panel not built: the Bait button keeps its call look) |
| 1.4 | HUD: The Catches (scrCatches) | the run's four card slots, then the cards not yet available and why; Auto-pick as a toggle; **DONE** (every card kept and ordered: held, open, out of reach with the lock; no slots: the concept's four are a drawing, not a rule; no "Not available" heading: a new string for the designer if wanted) |
| 1.5 | HUD: Achievements (scrAwards) | rewards waiting for a claim, Claim all, progress bars for the rest; **DONE** (HANDOFF (120)) |
| 1.6 | HUD: Jobs (scrJobs) | the worn job on yellow; locked jobs say what opens them; the approved job icons; **DONE** (HANDOFF (120)) |
| 1.7 | Boss fight (scrBoss) | the boss panel at the top centre: name, phase, health with the phase marks, the timer; SHIELDED on a red tag; **DONE** (HANDOFF (121): it also covered the pills at 1920 x 1080, a Slate centring rule) |
| 1.8 | Skill tree (scrTree) | hex nodes on the night sky: owned green, buyable gold, locked dark; rainbow and gold rings; the side panel; **1.8a DONE** (colours and chrome, HANDOFF (122)); **1.8b hexagons DONE** (HANDOFF (126)); open: the star hub, the branch pills, the side card |
| 1.9 | Unicorn ceremony (scrUnicorn) | the rays, the cat climbing on, the line that says what changed; **DONE** (HANDOFF (123)) |
| 1.10 | Victory (scrVictory) | the confetti in the rainbow, the crown on the floor; **DONE** except the confetti (HANDOFF (124)) |
| 1.11 | Settings switches (S3) | the concept's switches for the checkboxes (a chrome piece: `sweet_ui.py`, raster, import); **DONE** (HANDOFF (124)) |
| 1.12 | S6: icons and cards | the card faces' art, the skill icons and the achievement icons in the kit's icon style (Missing Art for anything the concepts did not draw: drafted, shown in the morning, not shipped until approved); **NOT STARTED**: drafts for the designer first |

Anything a screen needs that is not in the concepts (a picture, a word) goes on the morning page as a draft.

## 2. Then localisation (phase 7; `docs/WORKLIST-LOC-2026-11-18.md`)

Started only once section 1 is done (the designer's order); tonight's remainder if any:
1. `tools/port_po.py`: Godot's 20 translations into Unreal's localisation for every unchanged line. **DONE**
   (2026-11-19: 517 of 837 lines in 16 languages, 514 in cs/pl/ru/uk; `tools/loc_status.py` and its `--todo` work files;
   both gated; WORKLIST-LOC §2a).
2. **Dutch and Danish join the 21** (the designer, 2026-11-18): 23 languages. They have no Godot translation: every line
   is new, with a glossary made first (Catnip, Stardust, Reawaken, the stages, the realms) so the game and the store
   agree.
3. The port's own lines (~280 changed by D15, plus every string new to the port) in all 22 other languages.
4. `tools/loc_status.py` and its gate: per culture, missing and stale lines (only what changed is translated again).
5. The language menu; the pseudo-locale tour; the store page in Dutch and Danish (`store_text.py`, stamped).

## 3. In the morning

- **DONE**: https://claude.ai/artifact/RCMPJYVDVQB1cWnzWHj54m. A review page (Artifact): each restyled screen, the concept beside the game, and what was kept and why; the words and
  pictures that wait for the designer.
- HANDOFF-LOG entries per slice; this page's table marked done or not, with the reason.
