# Night worklist, 2026-11-07 (unattended, from the designer's "go" to ~07:00)

> **Closed (history)** 2026-11-08 00:50; result in HANDOFF-LOG (66).

> Written 2026-11-07 evening from HANDOFF-LOG (65) and `WORKLIST-RESTYLE-2026-11-07.md` (R0, R1 done; next R2). The designer:
> *"Create an overnight plan."* The status column is updated as the night goes; the closing HANDOFF-LOG entry (66) summarises
> the result, and this list is then marked closed.

**The night's job is R2 (fonts), finished to its exit, plus the read-only groundwork for R3.** R3's palette flip (plum
panels, bone text) runs only if R2 is green early and the designer has said yes (question C): it cannot be left half done,
because dark text on half-flipped dark panels is unreadable.

**No `-Full` tonight.** It takes about 10 hours and nothing else may run beside it (its sabotages edit sources and rebuild).
Tonight's new sabotages are each run alone and seen CAUGHT. The next free night should be a `-Full` night: the first since the
restyle started.

## Before the designer leaves (answers needed)

| # | Question | Recommendation |
|---|---|---|
| A | **Commit R0 and R1 first?** The night stacks on about 410 uncommitted changes. A commit makes the night's work a clean diff and a safe point to go back to. Without one, every item backs up the files it touches to `build/night_20261107/backup/<item>/` first. | Yes: say "commit R0/R1" in that message. |
| B | **Font files into the repo** (LFS: `.gitattributes` already tracks ttf/ttc): M PLUS Rounded 1c Black and ExtraBold (3.6 MB each, already fetched from google/fonts for the font test); its `OFL.txt` (4 KB, one fetch from the same repo); Noto Sans CJK Medium `.ttc` (18 MB) and Noto Sans Thai Medium (38 KB), COPIED from the Godot repo's `assets/fonts` (the Godot repo itself stays untouched). | Yes. |
| C | **R3's palette flip tonight if there is time?** (Plum panels and bone text across every screen; rule: only started with 3 h or more left and R2 green.) | Yes, with the rule. Otherwise you wake up to the fonts plus the R3 inventory, and the flip is the next session. |

## Ground rules for the night (CLAUDE.md standing rules)

- No commit, push, package or export unless A is answered yes, and then only the R0/R1 commit. Stage explicit paths.
- The Godot repo is read-only (its `git status` is checked by `check_all`).
- No Unreal editor open; one headless run or test run at a time; every run sandboxed.
- Widget Blueprints own their layout: never re-bake. A look change in an asset is a scripted migration (like `--recolour`) or a
  `--graft restyle:`, and is guarded by a test that was seen to fail before the migration.
- Every new assertion is seen to fail; every new sabotage is seen CAUGHT alone.
- **Stop rule.** If a failure is not understood within 45 minutes, put the item's files back from its backup (or from the
  commit), mark it **parked** with the cause so far, and move to the next item that does not depend on it.
- The night ends on an everyday `check_all` exit 0, whatever got done.

## The list

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight.** No Unreal process. A answered (commit, or the backup folder). `git status` saved to `build/night_20261107/start.txt`, so the night's own changes can be listed. Baseline: everyday `check_all` r1d exit 0 (HANDOFF-LOG (65)); rerun it only if anything changed since. | 15 min | start.txt written | **done** 22:50: designer "Do all of it" (A, B, C yes); everything outstanding since HANDOFF-LOG (34) committed as `f6a16b3` (1271 explicit paths, not pushed); no Unreal process |
| 1 | **R2a: the font sources and `F_Paw`.** `SourceArt/fonts/` holds the faces and their OFL texts (B). Measure first: does a `UFontFace` import the `.ttc` and does `FFontData::SubFaceIndex` pick its KR / SC / TC face? If yes, ship the one `.ttc`, as Godot does; if not, split it with fontTools. One table, `tools/fonts.py`, is the single source for the face files, typefaces and sub-fonts, read by both the importer and the gate (item 2). `tools/import_fonts.py --run` (headless) creates the `UFontFace` assets (CJK lazy-loaded) and the composite font `F_Paw` under `/Game/Pawpocalypse/Fonts`, built by an editor helper in `PawpocalypseEditor` (Python cannot reliably fill `FCompositeFont`). Default typefaces: `Regular` = M PLUS ExtraBold, `Bold` = M PLUS Black (the names the baked text already uses). Sub-fonts: `ko`, `zh-Hans`, `zh-Hant` by culture, Thai by range U+0E00-0E7F. Japanese is M PLUS's own. Test `Pawpocalypse.UI.Fonts.Composite`: runtime cache, each typeface and sub-font on its face; seen failing; a realm sabotage CAUGHT. | 1.5 h | test green, sabotage CAUGHT | **done** 23:15: the `.ttc` ships whole (`FFontData` takes a sub-face index; JP 0, KR 1, SC 2, TC 3, HK 4). **Found:** the font import factory (and any face property edit) asserts headless: `UFontFace::PostEditChangeProperty` flushes Slate's font cache unchecked; faces are made by `UPawFontBuilder::BuildFaces` (`InitializeFromBulkData`). Composite test green; sabotages `font_thai_face_wrong`, `font_umg_default_lost` CAUGHT |
| 2 | **R2b: the coverage gate.** `tools/check_font_coverage.py`: every character (NFC, minus whitespace and controls) of every shipped string, per culture (the port's own string tables and translations), against the faces `F_Paw` gives that culture (`getBestCmap`, the 2026-11-07 method in KB U/09 §5). `--self-test`: with the Thai range dropped, and with one Cyrillic letter removed from the cmap, it must FAIL. Added to `check_all` (L0, everyday). | 1 h | gate PASS, self-test seen failing both ways | **done** 23:00: PASS, 22 languages (incl. the pseudolocale), 6301 characters per weight; ja all M PLUS, ko Noto KR, zh SC / TC, th Noto Thai, nothing needs the fallback; self-test FAILS both ways; everyday gate + `-Full` self-test; `fonts.py --check` in L0 (planted change CAUGHT) |
| 3 | **R2c: the switch to `F_Paw`.** Measure first how many `FSlateFontInfo` values in the Widget Blueprints still point at the engine's Roboto. They were saved as values at bake time, like the colours were. **Code:** `PawUi::Font(role, px)` gives `F_Paw` at the right typeface; `PawUiKit`'s text helper, `PawPrompts`' default font (`GetDefaultFontStyle`) and every `GetFont` call site (17 UI files) go through it. L0 `ui-rules` gains "no `GetDefaultFontStyle`, no engine font path in UI code". **Assets:** the colour walker becomes a general struct visitor (`PawUi::ForEachColour` keeps its signature; `PawUi::ForEachFont` is added); `UPawWidgetBaker::Refont`; `py tools/bake_widgets.py --run --refont` (only Roboto is replaced; Regular to Regular, Bold to Bold, every other typeface to Regular; sizes and outlines kept). Test `Pawpocalypse.UI.Widgets.Fonts`: no asset font outside `F_Paw`, and the walker saw more than N fonts. It is seen failing before the migration; sabotage `font_walker_blind` CAUGHT. | 2 h | `Pawpocalypse.UI.*` green, the game shows M PLUS on every screen | **done** 23:25 (screens: item 5): **measured** the assets did NOT save Roboto: a baked text's font equals UTextBlock's default, so delta serialisation stores none and `[SlateStyle] DefaultFontName` alone moved 97 of 99; `--refont` wrote the 2 left (the prompt's rich-text override). The walker skips Transient properties (a rich text's `DefaultTextStyle`). Test failed 99, then 2, then passed; sabotage `font_walker_blind` CAUGHT; L0 `ui-rules` +2 rules, both CAUGHT |
| 4 | **R2d: titles and numbers.** Tokens for the outline (Plum, px by size band) and the solid drop shadow (Deep Plum, straight down), generated from `palette.py` like the colours. They go on the "title" and "number" text roles in `PawUi::Font`; title text in the assets gets them by `--graft "Class=restyle:Part"`, one listed part at a time. The spec says to size them "by eye", so the night applies the recommended size and also shoots a size sheet (three outline and shadow sizes on the main menu, the HUD counters and the level-up title) for the designer to pick from in the morning. Changing it is one token. | 1 h | applied, sheet shot, tests green | **done** 23:35: `PawUi::Heavy` (Black; a Deep Plum shadow 3 px when the text has an outline) at 21 parts (HUD counters, XP, payout; every title; card names; tree counters; slot numbers); `restyle:` now copies a text's typeface, outline and shadow; the parity test compares each named text's look (failed on the 21 before the graft); sabotage `heavy_typeface_lost` CAUGHT. Sheet `build/look/night_20261107/heavy_sizes.png`. **Seen:** on a plum panel the Deep Plum shadow and plum outline almost vanish |
| 5 | **The language tour.** `godot_tour.py` gains `--lang`, `--res` and `--ui-scale` on the Unreal side (`-culture=`; sandboxed settings at 150 %). It shoots every screen at 1280x800 and 150 % in en, ru, vi, ja, ko, zh-Hans and th, plus the longest-string language (measured from the tables), into `build/look/night_20261107/` with one contact sheet per language. Every clip, overflow or missing glyph is fixed in the asset that owns it or listed by screen. | 1 h | sheets written; faults fixed or listed | **done, changed** 23:45: the port has no translations (phase 7) and no UI-size option yet, so a language tour of the screens is impossible. Instead `paw.FontSheet <culture>` + `godot_tour.py --set fonts` draw each language's lines (Godot's .po) in F_Paw with the culture current: `build/look/fonts_in_game.png`, 22 of 22, no boxes. **Found and fixed:** 简体中文 in the picker mixed weights (M PLUS has 体中文, not 简): new culture-less Han sub-font -> Noto SC (Godot's rule) and a `ja` sub-font keeping M PLUS. `--res`: every screen at 1280x800 (`build/look/night_20261107/screens_1280x800.png`): no clipping seen |
| 6 | **Gate.** Everyday `check_all` exit 0 on the R2 state. Docs: `WORKLIST-RESTYLE` R2 marked done; docs 07 §5 and 09 (new gates) updated; KB U/09 §5 gets the night's measured facts (the `.ttc` sub-face, baked font values, `F_Paw` built headless). | 1 h | exit 0 | **done** 23:52: everyday `check_all` exit 0, every gate PASS incl. the new `font coverage` (`build/check_all_night_r2.log`); docs 07 §5, 09, CLAUDE.md tools, WORKLIST-RESTYLE R2 done; KB U/09 §5.1 (new), U/24 two gotchas, both indexes |
| 7 | **R3 groundwork (read-only).** List every Widget Blueprint part that holds a copied brush (box or rounded box) or a `UI_*` colour, per asset, from a headless dump. For each piece, choose a nine-patch plate (`sweet_ui.py`) or a rounded-box brush with outline (KB U/09 §4) and record why. List the `UI_*` text and surface pairs rule 6 must cover once the panels are plum. Written into `WORKLIST-RESTYLE` §R3. | 45 min | the R3 table written | **done** 23:50: `WORKLIST-RESTYLE` §R3 "Inventory". The blocker: `#2A1A33` is text AND frame today, and plum splits it (text Bone, frames Deep Plum), so `--recolour` must become role-aware (text colours vs surfaces) |
| 8 | **R3 flip** (only if C is yes, items 1-6 are green and at least 3 h remain; otherwise not started). In `palette.py`, the `UI_*` surfaces become plum and the text bone or lilac; rule 6 is extended to every pair from item 7; then `sweet_ui.py` plates, the pipeline (`gen_paw_tokens`, `build_art`, `raster_svg`, `pack_atlas`, `import_art --run`) and `bake_widgets --run --recolour`. `Pawpocalypse.UI.Chrome` is updated and seen failing on the old brushes. Then the tour and the everyday `check_all`. **If it cannot be green by 05:30, everything goes back to item 6's state** from the item's backup. | 3 h | exit 0 with plum chrome | **done** 00:35 (started 23:55, backup `build/night_20261107/backup/r2_state`): the colour flip, by ROLE (WORKLIST-RESTYLE §R3 note); everyday `check_all` exit 0 (`build/check_all_night_r3.log`, 18 of 18). R3's shapes (plate shadows, Deep Plum edges) left for the next session |
| 9 | **Close.** HANDOFF-LOG (66) with the exact next step; docs 00 and 11; this list marked closed. The morning sheet (below) is filled in. Optionally, the language contact sheets are uploaded to the concept canvas as a board "Review: fonts in game", placed beside "Font test: 21 languages". | 30 min | written | **done** 00:50: HANDOFF-LOG (66), docs 00, this list closed; morning sheets in `build/look/night_20261107` (canvas upload skipped: the sheets are local) |

**Order:** 0, 1, 2, 3, 4, 5, 6 (R2 complete and gated), 7, then 8 only by its rule, and always 9. If R2 runs long, 7 and 8
wait for the next session; 6 and 9 are never cut. Items 1 to 3 depend on each other. Item 4 needs 3. Item 7 needs nothing and
fills any wait.

**Risks I can see now.**
- M PLUS has a taller line than Roboto, so text in fixed-height rows may clip. Item 5 finds it, and the fix goes into the
  asset (`SizeBox`, padding), never a re-bake.
- `Pawpocalypse.UI.Widgets.*` compares each asset with its code layout. Both sides change font together in item 3, or the
  comparison fails for the wrong reason.
- The CJK face is 18 MB. Shipping it whole is correct first; subsetting it to the shipped strings is a later size decision,
  measured on a package.

## For the designer in the morning

- The contact sheets in `build/look/night_20261107/`, one per language, and the outline and shadow size sheet. Pick a size,
  or keep the night's choice.
- The faults item 5 listed but could not fix, if any.
- If item 8 did not run: say go for R3.
- Still yours from earlier nights: the hardware check from HANDOFF-LOG (55).
