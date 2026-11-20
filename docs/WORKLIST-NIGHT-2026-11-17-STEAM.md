# Night worklist, 2026-11-17 into 11-18: the Steam page and the screenshots

> Written 23:35 (designer: "Create an execute an overnight plan. We don't need a long nightly run. Our focus is steam page
> and screenshots so all of those should be priority"). It replaces the rest of `WORKLIST-NIGHT-2026-11-17.md`: that night's
> desk items 2a-2c are done, and its nightly was stopped at 23:31 (30 gates PASS, none failed; the PawCore sabotage's
> planted fault restored by `safe_mutate.py`; the tree is 1a10eeb plus docs). The closing entry is HANDOFF-LOG (109).

## What the morning gets

1. **The store page's words.** The short description, About, the feature list (every number from `Data/`), tags,
   languages and the AI disclosure. All in the port's plain English (D15), as a draft for sign-off.
2. **Every store and library image** in Sticker Box, at Steam's exact sizes, made from the approved drawings, and checked
   where Steam covers them and at the smallest size each is shown. A review page shows them all.
3. **Screenshots of the real game.** A scripted shot list (one command, the same scenes every time) at 1920x1080 with the
   HUD on, taken tonight. They show the new look **as far as the night gets it into Unreal** (step 3 below).
   **Store-ready screenshots need the whole look in the game** (S1-S5). Each one says which parts are still the old look.

## Rules for the night (CLAUDE.md)

- No commit, push, package or export. Stage nothing.
- Only the everyday `check_all` (about 6 min), after each engine step. No `-Full`.
- One test run at a time. Check for Unreal processes before every headless run. Never the designer's editor.
- Text: plain and informative, real numbers, the VOCAB words (enemies, Reawaken, permanently...). It is a draft: nothing
  in the game's string tables changes. `tools/text_audit.py` checks it.
- Open decisions take the measured recommendation, which can be undone in a step: E1 (fonts as measured), E2 (the disc
  rim), the approved drawings as they are (the eye floor question stays open). Each is listed in the closing entry.
- Everything is verified as it lands: the gates for engine steps, and the review sheets and pixel checks for art.

## The night, in order of what the store needs

| # | Work | Done when | Status |
|---|---|---|---|
| 0 | Stop the nightly; restore the planted fault; tree clean | `safe_mutate` clean, `git status` docs only | **done** 23:33 |
| 1 | **S1 applied** (the prepared `s1.patch`, `build/night3/s1/README.md`'s order): the UI in card stock and ink, the ink everywhere. Engine runs in the background while 2 and 3 are written | the inventory has no retired colour; everyday `check_all` exit 0 | **done** 23:52, the gates' result in the closing entry. Applied, then regenerated: tokens, tree tables, effects, art (look_lint PASS), atlas, import. Widget Blueprints recoloured by rule; the inventory again: **no retired colour left** (7,549). Found: two HUD labels over the field (XpNow, XpNext) were saved field labels, not text: grafted `text:` from the code layout (Widgets.Parity). The plum palette is kept as retired (`palette_plum_snapshot.json`), or the Palette test would stop catching it after a migration |
| 2 | **Store text**: the facts measured from `Data/` (realms, bosses, enemy kinds, cards, jobs, tree nodes, achievements, languages, length from `pawsim`); the comparable pages' short descriptions and tags re-read; `docs/STEAM-PAGE-DRAFT-2026-11-18.md` and `docs/TEXT-DRAFT-2026-11-18-steam.csv`; `text_audit` clean | every number cited to its source; audit clean | **done** 23:10. `docs/STEAM-PAGE-DRAFT-2026-11-18.md`: 27 lines through the game's text rules, 0 flagged. Old numbers fixed: 39 enemies (was 16), 39 achievements (was 41), about 2 h (was 6-10 h) |
| 3 | **Store and library art**: the logo (sticker title), header 920x430, small 462x174 (seen at 120x45), main 1232x706, vertical 748x896, library capsule 600x900, library header 920x430, hero 3840x1240 (no words; the never-covered box), library logo 1280x720 (transparent), page background 1438x810, app icon 184x184, event cover 800x450 and header 1920x622. Drawn from the Bestiary's own figures (Node), rendered exactly, in `build/steam/` and a review page | every size exact; covered areas and the small sizes checked | **done** 23:45. 13 images, exact sizes; the logo reads at 120x45; the hero inside the never-covered box; the library capsule clear of the covered strips. Review page (private): https://claude.ai/artifact/CAk7aojRbcMP17i32JpdGm |
| 4 | **The store shot list, scripted**: `tools/store_shots.py`, `paw.*` steps for 6-8 scenes (an early hunt, a crowd with the aura, a boss, the card pick, the skill tree, a late realm, the unicorn), 1920x1080, HUD on, into `build/steam/shots/` | the shots taken on the build as it stands | **done** 00:27: nine shots, 1920x1080. Five runs to get there, each finding a real cause:
  - level-up hands hold the game: the clears;
  - a renamed scene left old files: the tool now clears them;
  - the unlock cheat marks bosses beaten: the boss scene goes first;
  - **`paw.Boss` topped up the wrong Bait counter since D17** (fixed in `PawShot.cpp`);
  - `paw.Do` added: cheats run in queue order. |
| 5 | **The field in Sticker Box, for the shots' scenes** (S5, first part): the remaining Bestiary figures the scenes show, ported like batch 1 and checked against the Node references; the grounds of those realms; the rim at raster time (E2); their rig layers dropped until E3 is done; art rebuilt and imported | the shots' realms drawn in the new look; look_lint's new rules where needed; everyday `check_all` exit 0 | **built, not landed**. The pipeline works in a scratch copy (`build/night3/s5/`). **Blocked by the designer's "no shade used twice":** the approved Bestiary breaks it in 57 pairs (three identical colours). The designer's call: `docs/WORKLIST-STICKER-2026-11-17.md` S5 |
| 6 | **S2, the fonts** (E1 as measured): Nunito and Fredoka in `SourceArt/fonts`, `fonts.py`, `import_fonts`, the coverage gate with the one-face rule | coverage PASS, seen to fail; `check_all` exit 0 | **done** 00:10, the text half: F_Paw is Nunito 700 / 900; Japanese and xa stay on M PLUS. Coverage PASS in 22 languages, self-test seen to fail; `check_all` exit 0. F_PawTitle (Fredoka) waits for E1's answer |
| 7 | **Shots again** (4's script), then **close**: HANDOFF-LOG (109), docs 00, this list, the keep-awake stopped | written | **done**: HANDOFF-LOG (109) |

**Order and cut lines.** 1 first, because its engine runs take wall time and run while 2 and 3 are written. Then 4, then
5 and 6. 6 is cut first, then the late realms of 5. The closing entry is never held. If a gate fails and its cause is not
clear within the hour, the step is put back and written up.
