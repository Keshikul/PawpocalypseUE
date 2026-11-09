# Handoff log

Newest first. Every session that changes the state of this project adds an entry: where the project is, what happened, what is open,
and the exact next step. Read `docs/00-START-HERE.md` first, then the newest entry here.

---

## 2026-11-09 (72) - The plain text is in: 279 lines, "enemies", the port's own English

### Where the project is
- As in (70)/(71): R0-R4 done, **next R5 (the field)**. Committed through `7a6023c`; (70), (71) and this entry are not
  committed.
- The designer's text pass is **applied**. Everyday `check_all` **exit 0, 24 of 24 gates** on the final state
  (`build/check_all_text_apply.log`); every engine test 130/130 (`build/run_automation_text_apply2.log`).

### What happened
- The designer on review 2: **"All your picks are good but change mice to enemies since we don't only kill mice."**
  Grudge Ledger becomes **Stacked Deck** (the first pick); Second Helping says **City Rooftops** (what the code pays; F44:
  Godot's "the Park" is the wrong one).
- **mice -> enemies**: every draft (58), plus 9 table lines that said mice and had no draft (the effect lines for Snare,
  drift, vanish, crowd; the title's `{n} mice` slot lines became `{n} kills`, `{n}K kills`, `{n}M kills`). `VOCAB` now says
  "enemy / enemies" (the input device, "keyboard and mouse", stays). 279 lines in all.
- **Where the text lives (a decision, mine; tell the designer):** NOT in Godot. Godot's `ci/check_loc.py` needs every English
  string translated in every shipped language, so ~200 changed strings there would fail its loc gate. So:
  `ST_UI` written directly; `ST_Data` / `ST_Tree` from **`Data/TextOverrides.csv`** (`table,key,text,godot,note`, all 279
  lines, the `ST_UI` ones as a record), applied by `gen_data_strings.py` / `gen_tree_tables.py` through
  `tools/text_overrides.py`, which fails when a key is not generated, Godot's line changed since (`godot` column), or the
  `{tokens}` differ; `--check`: the `ST_UI` lines equal `ST_UI.csv`. `Data/` tools skip the file (L0 `data-export`,
  `gen_paw_rows.py`, `import_data.py`). Godot finding **F46** says so for the designer.
- **Tests:** 22 engine tests compare screens with Godot's recorded words (`.MatchesGodot`, literal Godot English). They now
  run under **`PawTextTesting::FGodotWords`** (`UI/PawTextTesting.h`): the `godot` column swapped into the loaded tables
  (`FStringTable::SetSourceString`) and the port's back after. New **`Pawpocalypse.UI.Text.PortWords`**: every line loads as
  the port's English, the scope swaps all 279 and gives them back, and texts made inside it say Godot's words.
  `PcTreeTextTests` (PawCore) renders through the `godot` column too. Sabotages: PawCore test with a wrong `godot` word
  FAILED (seen, restored byte-identical); `sabotage_realm.py` `text_scope_never_restores`, `text_scope_swaps_nothing`,
  `text_scope_no_unescape` (484), **each CAUGHT**, source restored (`build/sabotage_text_scope.log`).
- **Gates added to `check_all`**: `no banned words` (`text_audit.py --banned`; seen failing on a planted "grudge"),
  `text overrides self-test` (11 plants caught), `text overrides agree`. The audit: **0 of 841 strings break a rule**.
- The draft file is retired (applied; its notes are in `Data/TextOverrides.csv`). Skill blurbs' "over N levels" checked
  against the level counts in `Data/SkillBranchData.csv`: 34 of 34 right.
- Found on the way: my first `ST_UI` rewrite went line by line and cut the 7 values that span lines (restored, redone with a
  whole-CSV parse that round-trips byte-identical; pitfall row). Docs: CLAUDE.md (the rule, the tool line), 00, 08 (where the
  port's English lives; for `port_po.py` the changed lines have no Godot translation), 09 (gate row), 12 (two rows); KB
  `Unreal/20` §2 (a table entry swapped for a test), both indexes; memory `ui-text-short`.
- **Docs sweep** (designer: "Make sure all docs are up to date ... any old references or namings"): every old English line
  searched in the live docs and code comments; every path the docs name checked. Fixed: Grudge Ledger -> Stacked Deck
  (`grudge_ledger`) in docs 02, 09 and nine code comments; the lines PawCore's decisions show (`PcCards.h`, `PcControls.h`,
  `PcHud.h`), the victory headline (doc 05, `PawVictoryScreen.h`), the pad-audio toast and the title's kills (doc 05);
  the restyle worklist's text rule ("mean and funny" -> plain); the palette's source in docs 03 / 04 (`tools/art/palette.py`,
  not Godot's `palette.gd`); the shop's scroll (doc 02); new **D15** (doc 01) and the CLAUDE.md header (the text is the
  port's own, like the look); doc 06 and 00 list `Data/TextOverrides.csv`; doc 11 (phase 7: the 279 lines go to translators);
  sabotage count 484 (00, 09); Godot finding F43 updated. Left on purpose: older HANDOFF entries and dated night worklists
  (history), Godot findings and Godot-side files quoting Godot's own words, paths of Godot files and of tools planned for
  later phases. Everyday `check_all` after the sweep: exit 0, 24 of 24 (`build/check_all_docs_sweep.log`).

### Open
- **The designer:** whether Godot should take the same text (F46: then re-extract the .pot and translate); the title's
  `{n} kills` lines were my wording (the mechanical "{n} enemies" read badly on a save card); the canvas's two "grudge"
  lines; the tree opening zoomed; Continue's slot line; the hardware check; committing (70)-(72).
- The text's numbers are literal (only the level counts are checked): a `Data/` change to a skill's ladder will not update
  its blurb. A check that recomputes each blurb's range from `Data/` would close it.
- The 279 changed lines are untranslated (phase 7's `port_po.py` sends them to translators).

### Next step
Start R5 of `docs/WORKLIST-RESTYLE-2026-11-07.md` (settle the paw strike and the aura on a review sheet first, D3).

---

## 2026-11-09 (71) - The text redone: plain, informative, real numbers, one word per idea

### Where the project is
- As in (70): R0-R4 done, **next R5 (the field)**. Committed through `7a6023c`; (70)'s night and this entry are not committed.
- **Waiting on the designer:** text review 2 (270 drafts, `docs/TEXT-DRAFT-2026-11-09.csv`, review page
  https://claude.ai/artifact/5KnwdnDv3AZAS6Kz3bESzS, same link as before) and Grudge Ledger's name. Nothing is written into
  the string tables yet.

### What happened
- The designer, on the first drafts: no "poem or any kind of language with character"; "from now on" -> "permanently";
  "Use the same words for same concepts. So increase is increase everywhere not grows in some."; "be more specific with
  number"; "all the text should just be to the point informative information". The designer had started editing on the
  first page; those edits live in their browser and carry over to the new page (it imports the old page's edits).
- **The drafts, redone (270):** every skill text gives its numbers from `Data/` over all levels ("x1.18 to x3 over 10
  levels"); jobs and cards are their numbers ("Catnip x2.5. Kitten damage x0.4."); screen lines lose the voice
  ("Take 1 card.", "VICTORY", "You defeated the final boss."); effect lines and achievement rewards ("Reward: 4 Stardust")
  use the same words. Each fact was checked against the code, not the old blurb: zones (Echo, Crash Landing, Ember Trail)
  are priced on the whole damage, not the aura; Cat Burglar's Cut pays up to +100%; Sleight of Hand is every 20s to 6s.
  Skill, card, biome and achievement **names were kept** (names, not text).
- **`tools/text_audit.py`**: a `VOCAB` list (one word per idea: increase, permanently, mice, escape, run, attack, aura radius,
  upgrade, skill, x2.5 with the x first, ...), flagged in every non-name string; descriptions that wrap get a 120-character
  budget (3 short sentences) instead of the 60-character line; a draft that loses or invents a `{token}` is flagged; reads
  the newest `docs/TEXT-DRAFT-*.csv` and lists drafts for strings within budget too. Self-test: every new rule seen
  failing, clean lines pass. 124 of 841 strings break a rule today; all 270 drafts pass. The 2026-11-08 draft file is
  retired (superseded).
- **Godot findings F44, F45** (`docs/pawpocalypse-port-findings.md`, uncommitted): Auto-Pounce's text promises offline
  earnings (removed 2026-09-10); Second Helping says "the Park" but pays at biome 4, City Rooftops; the Engineer's text claims
  a card-level bonus that is Prototype's.
- CLAUDE.md's register rule rewritten (plain and informative); memory `ui-text-short` updated.

### Open
- The designer's picks on review 2, the Grudge Ledger name, F44 (Park or City Rooftops).
- Applying: `ST_UI` here; `ST_Tree` and `ST_Data` are generated from Godot, so their lines go into Godot first and are
  re-exported. The numbers in the drafts are literal; when applied they should be filled from the data so they cannot go
  stale.

### Next step
If the designer has answered review 2: apply it (Godot first for `ST_Tree`/`ST_Data`), then add `text_audit.py --banned` to
`check_all`. Otherwise start R5 of `docs/WORKLIST-RESTYLE-2026-11-07.md` (settle the paw strike and the aura on a review
sheet first, D3).

---

## 2026-11-08 (70) - Night: R4 closed (every screen fits at 150 %), short-text drafts, no grudges

### Where the project is
- The restyle: **R0-R4 DONE**. R4 closed on its exit criteria: screens reviewed against the boards' sources, focus tours
  pass (41), **no overflow at 150 %** (`Pawpocalypse.UI.Layout.*`). Its leftover faults wait on R6 art and the designer's
  words (worklist "R4's exit review"). **Next: R5 (the field).**
- **Committed `7a6023c`** (R2-R4 up to R4h's first pass; 453 explicit paths; local, not pushed). The night's work after it
  is not committed.
- Everyday `check_all` **exit 0, 21 of 21 gates** on the night's final state (`build/check_all_night_r4h.log`; the new
  `text audit self-test` among them).
- **Waiting on the designer:** the short-text review (122 drafts) and Grudge Ledger's new name (page below).

### What happened (night list `docs/WORKLIST-NIGHT-2026-11-08.md`, designer: "A yes, B draft, C center, Go")
1. **Commit** R2-R4 as `7a6023c`.
2. **A layout test that fails on overflow**: `UI/PawLayoutCheck` lays a screen out headless (prepass, paint, which ticks,
   arrange; the DPI curve x the UI size) and lists every painted part's screen rect, clipping included. Four tests
   `Pawpocalypse.UI.Layout.{Title,Dock,Settings,Tree}` at 1920x1080 and 1280x800, 100 % and 150 %; seen failing on the
   real faults first. **Found:** without the DPI curve the 1280x800 case shows false overflows.
3. **Fixes**: the title's logo and menu in one scale-down-only room (`LeftFit`), Boba's card in its own (`BobaFit`), the
   logo's empty box 1000 wide (1080p unchanged, measured); the shop page scrolls (`ShopList`); settings' pages scroll above
   Close (`PagesScroll`, new graft op `wrap:Part>Wrapper`); the tree refits when its real size arrives (`bViewTouched`).
   Grafts: WBP_MainMenu, WBP_HudScreen, WBP_SettingsDialog. **Found:** naming two texts in code renumbered the code's
   unnamed texts and broke `.Widgets.Parity`: those two stay unnamed (pitfall row).
4. **Designer mid-night: "remove any mention of grudges or cats being unimpressed"**. The victory line "The cat is
   unimpressed." dropped (test seen failing first); CLAUDE.md, the restyle's face rule and checklist cleaned; Godot finding
   F43 (Godot still has both). Grudge Ledger needs a new name: drafted ("Stacked Deck"), applied after the designer picks,
   Godot first (`ST_Tree` is generated from it). Ids `grudge_ledger` / `GRUDGE_PER_CARD` stay (save keys). **The canvas
   itself says "grudge" twice** (title tagline, tree chip): left for the designer.
5. **Short text** (designer: "simple and concise ... Use short sentences"): `tools/text_audit.py` (budgets: a line 60
   chars, a sentence 12 words, 2 sentences, a name 4 words; banned words; `--self-test` in `check_all` L0). 122 of 841
   strings over budget; 122 drafts in `docs/TEXT-DRAFT-2026-11-08.csv`, all within budget. Review page (private):
   https://claude.ai/artifact/5KnwdnDv3AZAS6Kz3bESzS. Nothing written to the tables.
6. Sabotages `shop_not_scrolling`, `tree_fit_stale_size`, `tree_refit_after_zoom`, `title_menu_room_low`, each CAUGHT alone
   (481). `padplay_tour.py` 41 PASS. Sheets `build/look/night_150.png`, `build/look/night_100.png`. Docs: restyle worklist,
   00, 05 §7, 09, 11, 12 (three rows), CLAUDE.md; KB `Unreal/09` §2.3 and §6, both indexes.

### Open
- **The designer:** the text review (then apply: Godot first for `ST_Tree` / `ST_Data`, the port's own `ST_UI` here; a
  changed English line loses its 20 translations); Grudge Ledger's name (then `text_audit.py --banned` joins `check_all`);
  the canvas's two "grudge" lines; the tree opening zoomed in; words for Continue's slot line and the dock's second lines;
  the hardware check (55); commit the night.
- R4's leftovers for R6: Boba's photo, the dock's icon tiles, card art.
- The `-Full` nightly has not run since the restyle began: the next free night.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then start R5 (the field) of docs/WORKLIST-RESTYLE-2026-11-07.md:
settle the paw strike and the aura on a review sheet first (D3)."* If the designer has answered the text review first:
apply it (Godot first for generated tables) before R5.

---

## 2026-11-08 (69) - Restyle R4 nearly done: Butter focus, the text floor, the title, the HUD, the dock, the tree, the titles; 150 % open

### Where the project is
- The restyle: R0-R3 done; **R4: R4a-R4g done** as far as they go without new art and words (the worklist's "R4
  progress"); **R4h's checks ran**: the pad tours (41 checks) and the 1280x800 tour pass; **at 150 % four screens
  overflow (open, the next step)**.
- Everyday `check_all` **exit 0 on the final state** (`build/check_all_r4_session1.log`, every gate; the run before it
  failed twice on my own leftovers: `PawTokens.h` stale after the R4e recolour rewrote the widgets' snapshot, and the
  sabotage `prompt_line_ignores_setting` aimed at a line R4f rewrote: regenerated, retargeted, CAUGHT).
- **Nothing committed since `f6a16b3`**: R2, R3 and R4 wait for the designer's review.

### What happened (designer: "Do r4")
1. **R4's steps** written into the worklist from the canvas boards' sources (`MainMenu`, `PlumMenus`, `SkillTree`,
   `LevelUp`, read with the Artifact tool) and the tour. Kept on purpose, not the canvas: the dock stays left; no new
   player-facing words without the designer.
2. **R4a**: the ring and "the selected thing" Butter (tokens `focus`, `selected`, `selected_text`; chrome
   `ui_button_selected`; `UPawButton::SetCallLook`; the call plate on the plate's face only, `ButtonFacePadding`).
3. **R4b**: the 16 px floor (`PawUi::UnderTextFloor`, `--textfloor`, `.Widgets.TextFloor`), nine constants raised.
4. **R4c**: the title after "Main menu (plum)"; graft ops `slot:`, `text:`, `remove:`; `.Title.Plum`.
5. **R4d**: HUD pills, the wave plate, the bars. **Found and fixed**: a failed graft once saved the HUD asset without its
   currencies (put back the same session): grafts are all or nothing now.
6. **R4e**: the shop rows (taller, words left, 18 px); locked text legible (4.5:1).
6b. **R4f**: round skill nodes, rounded-square unlocks; the tree's Stardust and legend on pills; a prompt line takes its
   plate with it when prompts are off. **R4g**: `PawUi::ButterTitle` on the card pick (tilted), job picker and victory;
   `.Widgets.Parity` compares text colours now and caught a stale code colour (the card pick's subtitle).
6c. **R4h**: `godot_tour.py --res` / `--uiscale` (shots in their own folder) and `paw.UiScale` (UMG's ApplicationScale:
   Slate's does not reach the viewport, KB U/09 §6); the findings in the worklist.
7. **Found**: the tour never retook the title, pause and settings shots; R3's review of those three screens looked at
   last night's pictures. Fixed in `godot_tour.py`; the screens since seen fresh (they were on the plum plates).
8. Sabotages: 13 new, each CAUGHT alone (477 realm sabotages); `butter_title_bone` first MISSED (it was aimed at a test
   that reads the asset), retargeted. Docs: worklist, 00, 05 §7, 09, 12 (five rows), CLAUDE.md. KB `Unreal/09` §2.3 (moving
   a part in a saved asset; all-or-nothing grafts) and §6 (testing 150 %), both indexes.

### Open
- **150 % overflows** (`build/look/r4h_150.png`): the title (logo under the menu; Boba and the version off screen), the
  dock's shop page (does not scroll; rows under the quick bar), the settings window (taller than the screen), the tree
  (drawn off centre at that scale). Then R4's exit review beside the boards.
- For the designer: the tree opening zoomed in (the board's big nodes); words for the dock's second lines, price pills and
  kittens strip; R6 art for the dock's icon tiles.
- The designer: review and commit R2-R4; the title / numbers shadow size (66); the hardware check (55).

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then fix R4h's four 150 % overflows (worklist R4 progress) and
close R4 of docs/WORKLIST-RESTYLE-2026-11-07.md."*
If the designer says go for the night: `docs/WORKLIST-NIGHT-2026-11-08.md` (R4's close, plus a draft of short UI text: the
designer, 2026-11-08, "make sure all text is simple and concise in the ui", now in CLAUDE.md's register rule).

---

## 2026-11-08 (68) - Restyle R3 done: Deep Plum edges, one solid shadow, the pressed button sinks

### Where the project is
- The restyle (`docs/WORKLIST-RESTYLE-2026-11-07.md`): **R0-R3 DONE**. Next R4 (screens and layout). Slice 6's 6b-6d after.
- Everyday `check_all` exit 0 on this state (`build/check_all_r3_shapes.log`, 18 of 18). `Pawpocalypse.UI` 77 tests PASS.
- **Nothing committed since `f6a16b3`**: R2, R3's colour flip and R3's shapes all wait for the designer's review.

### What happened (designer: "Do r3")
1. **The plates** (`tools/art/sweet_ui.py`, `plate()`): a 3 px Deep Plum edge, ONE solid Deep Plum shadow straight down
   inside the bottom margin (buttons 4 px, panel and window 8, a new `ui_plate_small` 4). The pressed button sinks onto its
   shadow; the silhouette check now says three states share one box and pressed is that box moved down by the shadow
   (measured at half alpha: the resampling's halo reaches the canvas edge). Margins read off the drawings, each corner and
   the shadow inside the cut, the shortest-control rule kept: `PawUi::ChromeTable` (+ a `Shadow` column, `ChromeGap`).
2. **A rounded box cannot cast a shadow** (KB U/09 §4), so every plate the code drew flat now wears the chrome: the dock,
   the boss panel, the zone plate (small), the title's menu, windows and Boba badge, the pause and settings windows, the
   tree's warning and tip, the toast (small). `PawUi::PlatePadding` lifts content off the shadow; buttons' content margins
   16 / 4 / 16 / 8 (Common UI writes one padding into both normal and pressed: a pressed label cannot sink, KB U/09 §4). The
   title's cluster wears the theme's buttons. Grafted with `restyle:`; **found:** the HUD's dock panel is `Border_0` in the
   code layout but `Border_2` in the asset (auto-names drift): new graft ops `names:` / `codenames:` (each widget with its
   parent) and `restyle:Code>Asset` (refused when the class or the parent's class differs). The title's `Border_0-6` were
   checked to line up parent for parent before trusting them.
3. **Palette**: `panel_frame` / `button_frame` Deep Plum; new `shadow` (Deep Plum) and `card_ink` (the card's ink, rule 7's
   card pair). **The migration's third role, EDGE** (a brush's outline, `PawUi::IsEdgeColour`; a font's outline is not):
   `EPawColourRole`, `ui_role_pairs` text / surface / edge, `RecolourByRole(..., EdgePairs, ...)`; `--recolour` moved the ink
   frames left in the assets (#2A1A33 -> #120A17; WBP_CardFace's kept) and updated the widgets' snapshot.
4. **Found and fixed**: the quick bar's gear and door, the catches tab's cards and the boss / kitten badges were drawn in the
   button colour (cream once, Plum Raised since last night's flip): nearly invisible on plum. `sweet_ui.LIGHT` = Bone.
5. Tests: `Pawpocalypse.UI.Chrome` seen to FAIL first (the small plate not imported; 9 plates in the assets and 2 in code
   not the chrome; the job picker's and victory's saved margins old), then PASS; it now checks 7 more plates (code and
   asset) and that every plate's padding lifts content off its shadow. `.ColourRoles` covers the edge role. Sabotages
   `plate_shadow_ignored`, `colour_role_edge_lost`, `plate_flat_again` new, `chrome_margin_not_godots` retargeted,
   `colour_role_text_lost` and `palette_walker_blind` re-run: all 6 CAUGHT alone (464 realm sabotages).
6. Seen: the tour (`godot_tour.py --skip-godot`): title, HUD, pause, settings, tree, job picker, victory, card pick all on
   the plum plates; `build/look/r3s_screens.png`, `build/look/r3s_zoom.png` (1:1 x2: edges, shadows, the sunk tab).
7. Docs: worklist R3 DONE note, 00, 01, 05 §7, 09, 11, 12 (two rows), CLAUDE.md (graft ops). KB `Unreal/09` §2.3 (auto-name
   drift; colour roles) and §4 (no shadow on a rounded box; Common UI's pressed padding), both indexes, two symptom rows.

### Open
- R4: the focus ring and selection Butter (still coral), the title hint's plate, the dull Travel row; the layout pass.
- The cards (Bone bodies, ink frames, no shadow yet) are R6's; the tree's node frames are painted at runtime (R4/R6).
- The zone plate's rainbow frame hugs the plate's widget, so it now includes the 4 px shadow (looks fine; noted).
- The designer: review and commit R2 + R3; the title / numbers shadow size (66); the hardware check (55).

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then start R4 of docs/WORKLIST-RESTYLE-2026-11-07.md."*

---

## 2026-11-08 (67) - Docs brought up to date with R2 and R3

### Where the project is
- Unchanged from (66): R0-R2 done, R3's colour flip done; R2 and R3 NOT committed (the designer reviews them). No code,
  asset or test changed in this session: docs only.

### What happened (designer: "Make sure all docs are up to date")
- Every doc read against the code for claims the night made stale; fixed:
  - `00`: date, phase 6's "next" (6b-6d wait for the restyle), the last everyday run (66).
  - `01` D14's status.
  - `02` the paper look marked as Godot's.
  - `03` the font builder.
  - `05` the cream panels, keycaps and card plate (now the plum tokens and Bone, values from `PawTokens.h`), the font plan
    (the `.ttc` is one face, no split).
  - `07` the fonts row, "the restyle plan's D1", the OFL texts' place.
  - `11` the restyle before 6b-6d, phase 7's fonts done early.
  - `12` four rows: headless font import, mixed-weight Han, Transient rich-text style, the colour split by role.
  - The restyle worklist: the proposed R3 values marked as differing from what was built.
- **Measured:** F_Paw (M PLUS Rounded 1c) HAS ★ ☆ ◆ and the arrows (fontTools cmap). doc 05's "the font has no star /
  arrow glyph" was Godot's font. The drawn discs and arrow images stay; using the real marks is open (an R4 candidate).
- Appendices B and C regenerated to scratch: identical to the committed ones. A is Godot's data, unchanged.

### Open
- As (66): R3's shapes, R4's items, Boba's photo; the designer's review and commit of R2 + R3; the shadow size.
- New: the row marks and the badge star could use F_Paw's ★ ◆ ☆ instead of drawn discs (decide in R4).

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then finish R3 (the plates' shadows and edges) of
docs/WORKLIST-RESTYLE-2026-11-07.md."*

---

## 2026-11-08 (66) - The night: R0-R1 committed; R2 fonts done; R3's colour flip done (plum panels, bone text)

### Where the project is
- The restyle (`docs/WORKLIST-RESTYLE-2026-11-07.md`): **R0, R1, R2 done; R3's colour flip done**. Left of R3: the plates'
  solid drop shadows and Deep Plum edges (`sweet_ui.py` shapes). Then R4. Phase 6 slice 6 (6b-6d) after the restyle.
- Everyday `check_all` exit 0 on the final state (`build/check_all_night_r3.log`, 18 of 18 PASS; the R2 state's too,
  `build/check_all_night_r2.log`).
- **Committed `f6a16b3`** (local, not pushed): everything since `98e3a4f` (the port through slice 6a, R0, R1), on the
  designer's "Do all of it" to the night plan's question A. **R2 and R3 are NOT committed** (the night's changes:
  `git status`; the pre-R3 state is backed up in `build/night_20261107/backup/r2_state`).
- The game today: every text in M PLUS Rounded 1c (`F_Paw`), titles and numbers in Black; plum panels and rows, bone text,
  lilac secondary text; cards keep Bone bodies with ink text; labels over the field Bone with an ink outline.

### What happened (night plan `docs/WORKLIST-NIGHT-2026-11-07.md`, closed)
1. **R2a, the font.** `tools/fonts.py` (the table) -> `UI/PawFontSpec.h`; `tools/import_fonts.py --run` -> `FF_*` faces +
   composite `F_Paw` (`/Game/Pawpocalypse/Fonts`) via `UPawFontBuilder` (PawpocalypseEditor). **Found:** the font import
   factory asserts headless (`UFontFace::PostEditChangeProperty` flushes Slate's font cache unchecked): faces made with
   `InitializeFromBulkData`. The Noto CJK `.ttc` is one face asset, sub-faces by index. Sources + OFL texts in
   `SourceArt/fonts`. Test `Pawpocalypse.UI.Fonts.Composite`; sabotages `font_thai_face_wrong`, `font_umg_default_lost`.
2. **R2b, the gate** `tools/check_font_coverage.py`: models Slate's face choice; 22 languages, 6300 characters per weight,
   PASS; self-test fails both ways. Everyday gate; self-test in `-Full`; `fonts.py --check` in L0.
3. **R2c, the switch.** `[SlateStyle] DefaultFontName=/Game/Pawpocalypse/Fonts/F_Paw` (DefaultEngine.ini). **Measured:** the
   baked assets did not save Roboto (their fonts equalled the default): 97 of 99 moved with no asset change;
   `bake_widgets.py --refont` wrote the 2 left (a rich text's style override). Walker `PawUi::ForEachFont`, skips Transient
   properties. Test `.Widgets.Fonts` (failed 99, then 2); sabotage `font_walker_blind`. L0 `ui-rules`: no engine font.
4. **R2d, titles and numbers**: `PawUi::Heavy` (Black; a Deep Plum shadow 3 px on outlined text) at 21 parts, grafted
   (`restyle:` now copies a text's typeface, outline, shadow); the parity test compares every named text's look; sabotage
   `heavy_typeface_lost`. Size sheet `build/look/night_20261107/heavy_sizes.png`.
5. **Language sheets** (the port has no translations or UI-size option until phase 7): `paw.FontSheet <culture>`,
   `godot_tour.py --set fonts` -> `build/look/fonts_in_game.png`, 22 of 22. **Found:** 简体中文 mixed weights (M PLUS has
   体中文, not 简): culture-less Han sub-font -> Noto SC, `ja` sub-font keeps M PLUS. `godot_tour.py --res`: 1280x800 clean.
6. **R3 colour flip**: `palette.py` UI plum values + `field_label`; rule 7 (13 UI pairs). The migration by ROLE (one old
   ink, two new colours): `PawUi::ForEachColourRole` / `IsTextColour`, `UPawWidgetBaker::RecolourByRole`,
   `gen_paw_tokens.ui_role_pairs`, `bake_widgets.py --recolour` (KEEP_INK: WBP_CardFace). 221 colours in 12 assets.
   Godot maps leave the UI table out. Code: ink text -> `UI_TEXT` (13 sites), `UI_FIELD_LABEL`, Pounce dial Bone,
   `UPawIconText::bOverField` now applied (it never was), the logo outlined, the title hint ink, checkboxes, glyph keys
   Bone (`sweet_input.py`). Tests: Palette failed with 76 retired creams before the migration; new `.ColourRoles`
   (sabotage `colour_role_text_lost`). All 9 new sabotages seen CAUGHT alone.
7. Knowledge: `Unreal/09` §5.1 (new), `Unreal/24` two gotchas, both indexes. Docs: 00, 07 §5, 09, CLAUDE.md tools.

### For the designer (morning)
- `build/look/night_20261107/plum_after.png` (every screen now), `build/look/fonts_in_game.png` (22 languages),
  `build/look/night_20261107/heavy_sizes.png`: pick the shadow size (tonight 3 px; seen: on plum panels the shadow and
  outline barely show).
- Review and commit R2 + R3 when ready (not committed).
- Still yours from earlier: the hardware check from (55).

### Open
- R3's shapes: plate drop shadows, Deep Plum panel edges (frames are INK, invisible on plum).
- R4 items seen tonight: the highlighted Travel row is a dull khaki (Butter selection with the focus pass; focus ring still
  coral `ALERT`); the title's hint line crosses the bushes (a plate); the HUD wave line over bushes.
- Boba's photo on the title badge is still the empty placeholder (slice 6c).

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then finish R3 (the plates' shadows and edges) of
docs/WORKLIST-RESTYLE-2026-11-07.md."* After any palette change: `gen_paw_tokens.py`, `build_art.py`, `raster_svg.py`,
`pack_atlas.py`, `import_art.py --run`, `bake_widgets.py --run --recolour` (by role), `check_all`. After a font change:
`fonts.py`, `import_fonts.py --run`, `check_font_coverage.py`.

---

## 2026-11-07 (65) - Restyle R0 and R1: the look unhooked from Godot; the ink is plum

### Where the project is
- The restyle (`docs/WORKLIST-RESTYLE-2026-11-07.md`) is under way: **R0 and R1 done, next R2 (fonts)**. Phase 6 slice 6
  (6b-6d) waits behind it (designer: "Restyle now"). Everyday `check_all` exit 0 (`build/check_all_r1d.log`, every gate PASS). Nothing committed after `98e3a4f`.
- The game today: the field, the creatures, the chrome nine-patches and every UI text and frame are in Plum ink `#2A1A33`;
  the panels are still cream (R3 turns the surfaces plum together with the text).

### What happened this session
1. Designer: **font M PLUS Rounded 1c; the cat stays grey; restyle now** (D14 in docs/01).
2. **R0**: `tools/gen_paw_tokens.py` imports `tools/art/palette.py` (70 tokens byte-identical before any change); the Godot
   scene/script literals moved into `palette.py LITERALS`; `build_art.py --check` no longer compares with Godot; Godot
   fixture colours pass through the generated test-only `Source/PawCore/Tests/PcGodotLook.h` (Godot's palette frozen in
   `tools/art/palette_godot_snapshot.json`; `Pc::PcFromGodot` in PcUnicornTests and PcRainbowTests); sabotage
   `godot_look_off` CAUGHT. CLAUDE.md and docs 00/01/04/05/07/09 record that the look is ours.
3. **R1**: `palette.py` PLUM family (`PLUM_*` tokens) + rule 6 (ten WCAG contrast pairs; catnip/crimson differ in
   lightness); INK = Plum. All art regenerated and imported (390 SVGs, the 4 hand SVGs recoloured, rasters, cursors,
   3 atlas pages, 9 chrome, 12 floors, 178 icons, 65 glyphs). `PawVfx`'s ink reads `Pc::Ink`; `gen_tree_tables.py` passes
   Godot's tooltip/branch/link colours through the same map (the tooltip blurb was Godot's black).
4. **Found: baked Widget Blueprints keep colours as values** (158 stale after the ink change; the job picker's and victory's
   ink tests caught it). New: `PawUi::ForEachColour` / `LookObjects`, `UPawWidgetBaker::Recolour`, `py tools/bake_widgets.py
   --run --recolour` (exact old palette colours only, hand edits kept; `tools/art/palette_widgets_snapshot.json`), test
   `Pawpocalypse.UI.Widgets.Palette` (no `PawTokens::PALETTE_RETIRED` colour in any asset; seen to FAIL with 158; sabotage
   `palette_walker_blind` CAUGHT). L0 `ui-rules` caught `FromSRGBColor` in UI code: `FLinearColor(FColor)` instead.
5. Tour (Unreal side, `build/tour/ue`, 21:14-21:19): sampled pixels: title text, title buttons, dock frames, the cat and
   the biome art all `(42, 26, 51)`.
6. Canvas: the Style board rewritten as the plum spec (M PLUS Rounded 1c). Knowledge: `Unreal/09` §2.3 (baked colours are
   values; the migration), `Godot/03` §6 (keeping oracle fixtures through a restyle), INDEX rows.

### Open
- R2 fonts next (worklist §5). D3 (the paw strike and the aura) is decided on R5's review sheet.
- `docs/11` does not list the restyle yet (the worklist is the plan); `docs/00` status line to add when R3 changes what the
  game looks like.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then do R2 (fonts) of docs/WORKLIST-RESTYLE-2026-11-07.md."*
After any palette change: `gen_paw_tokens.py`, `build_art.py`, `raster_svg.py`, `pack_atlas.py`, `import_art.py --run`,
`bake_widgets.py --run --recolour`, `check_all`.

---

## 2026-11-07 (64) - The restyle planned: the Unreal look leaves Godot (plum)

### Where the project is
- As (63): phase 6 slice 6, **6a done, next 6b**. No code, asset or gate changed this session; only this entry and the new
  plan `docs/WORKLIST-RESTYLE-2026-11-07.md`. Nothing committed after `98e3a4f`.

### What happened this session
1. A look concept on a design canvas (https://claude.ai/artifact/3wbqpr2HT4daaSmtHtNT2K, private to the designer): the
   game "goofy and chubby", then not-cute alternatives, palette research on similar games, the original field with **plum
   menus** (chosen), a plum main menu (Boba's photo as the cameo) and skill tree, and a font test.
2. **Designer decisions:** "The unreal version does not have to match godot, they can be different." and **"Plum
   everywhere."** Rules and numbers still follow Godot exactly; the look does not. NOT yet written into CLAUDE.md / docs:
   that is R0 of the plan.
3. Font test (fontTools, every character of all 21 languages against each font's cmap): **M PLUS Rounded 1c** and **Dela
   Gothic One** cover every Latin and Cyrillic language, Vietnamese and Japanese; Nunito all but Japanese; **Lilita One fails
   cs, pl, hu, tr, vi, ru, uk**. Knowledge: `Unreal/09` §5 + INDEX row.
4. Design review of every concept board rendered in the browser: three layout collisions fixed on the canvas; seven open
   points recorded in the plan §3 (the paw cursor, the muddy aura, the cream boards, the font, small labels, the dock's void).

### Open
- **D1 font** (A: M PLUS Rounded 1c; B: Dela Gothic One + Nunito), **D2 the cat's colour** (recommend keep grey), D3 the paw
  and the aura: plan §2.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then do 6b of docs/WORKLIST-SLICE6-2026-11-07.md."* The restyle
(R0 onward) starts after slice 6, once D1 and D2 are answered.

---

## 2026-11-07 (63) - Phase 6 slice 6 planned; 6a: the Unicorn ceremony

### Where the project is
- Phase 6 slices 1-5 done; slice 6 started: its plan in `docs/WORKLIST-SLICE6-2026-11-07.md`, **6a done**. Everyday
  `check_all` exit 0 (`build/check_all_6a.log`; L1 196 cases, `Pawpocalypse.UI` 73 tests). Nothing committed after `98e3a4f`.
  **Next: 6b, the Reawaken banner.**

### What happened (designer: "Start slice 6")
1. **The survey** of unicorn_ceremony.gd, boba_cameo.gd, tutorial.gd, Godot's own gate ci/check_tutorial.gd and their callers,
   written as the slice 6 plan: 6a the Unicorn ceremony, 6b the Reawaken banner (the tutorial's Reawaken cue leans on it),
   6c Boba (the 14 photos, the cameo clock, five cameos, his places in the title / pause / settings / victory, never over
   the tree; needs TRANS_BOUNCE), 6d the tutorial (eleven cues, the paw, Godot's gate walked in the port = phase 6's
   "tutorial gate green"). A candidate finding noted for 6c: `BobaCameo.force()` skips the effects-off check.
2. **The fixture `unicorn`** (`tools/godot_fixtures/unicorn_fixture.gd`): the real script recompiled with its draw calls,
   global random draws and sounds renamed to recorders (5d's probe) and its class name replaced by its base; its tweens
   stepped by `custom_step` after its `_process` (5c); 16 runs at 4 sizes, presses (key, echo, mouse, pad, release) at random
   frames; per frame the clock, dim, cat, fade, rainbow, sounds, every 23rd bit and what it draws. `--check` byte for byte.
3. **PawCore**: `PcTween`, Godot's Tween sequencing (steps of parallel tweeners, delays, callbacks, intervals, a delayed
   property reading its start when its delay ends, the leftover time running into the next step), for 6c too;
   `PcUnicornCeremony` over it. Within float32 of Godot: the cat 4.6e-4 px, a bit 1.7e-3 px, a tween value 6e-7. INK and GOLD
   generated into PawCore's palette. Salt `UnicornFiesta`.
4. **The game**: `UPawUnicornCeremony` (`WBP_UnicornCeremony`), the tree's last part (`add:Unicorn`, backup of the asset in
   `build/wbp_backup_pre6a/`): plays when the Unicorn is bought and only then; Visible while it plays (eats clicks), holds
   the keyboard focus, the tree hands it every key and buys nothing; a press after 0.6 s skips; the tree takes its focus back
   when it ends. The cat from `DA_FieldAtlas`, the rest through `FPawFxCanvas`. `paw.Buy <branch>` for the tours; the tour's
   new `unicorn` shot on both sides.
5. Docs 00, 03, 04, 05, 06, 07, 09, 11, 12, the worklist, appendix B (appendices B and C regenerate unchanged); KB G/03
   (the delayed tweener, the leftover).

### Verified
- L1 `Unicorn`, `Tween` PASS; 19 PawCore sabotages `tween_*` / `unicorn_*` all CAUGHT (`build/sabotage_unicorn_6a.log`;
  `tween_delayed_reads_at_start` first MISSED: the test moved the value before the delayed step began; now during the delay,
  CAUGHT). 627 in all.
- `Pawpocalypse.UI.Unicorn.Plays`, `.RunsOut` PASS (the sound's rate limits on a driven clock: the first run dropped the second
  whinny, played 0.3 s of no time after the first); 8 realm sabotages `unicorn_*` all CAUGHT
  (`build/sabotage_unicorn_realm_6a.log`); `tree_browse_buys` retargeted to the extended purchase guard, CAUGHT. 455 in all.
- The tour (`build/tour/compare/unicorn.png`): the dim, the rainbow, the mounted cat, gold stars with ink rims and confetti, in
  both games (the Unreal shot lands later in the timeline: the tours' timing, not the ceremony's).
- Mishap, repaired: a guard-less shell line started `sabotage_pawcore.py` with no `--only` (every sabotage); stopped after a
  minute, it had left `PcProgress.cpp`'s rematch line sabotaged (`Out.bDefeated = false;`). Restored to the committed rule,
  every sabotage target checked original; the L1 suite and `check_all` pass after it. Doc 12 has the row.

### Open
- 6b-6d (the worklist). The tree's `NativeTick` stepping the ceremony is seen only in the tour (the tests step it by hand).
- From (62): the Reawaken banner and its frame (6b), the bars' minimum fill width side by side, the boss bar's step.

### Next step
*"Phase 6 slice 6b (docs/WORKLIST-SLICE6-2026-11-07.md §6b): the Reawaken banner. Read the rest of hud.gd
_refresh_reawaken_banner (its place over the field), add its decision to PcHud against the real HUD (a fixture over random
states, as the victory fixture runs the real hud.tscn), then the plate in UPawHudScreen (graft) with a second UPawRainbowFrame
following it (margin 2, corner 10, width 5), its pop-in and jitter."*

---

## 2026-11-07 (62) - Phase 6 slice 5d: the rainbows and the tree's sky

### Where the project is
- Phase 6 slices 1-4 and 5a-5d done. Everyday `check_all` exit 0 (`build/check_all_5d.log`; L1 194 cases, `Pawpocalypse.UI` 71 tests, the game build too). Nothing committed after `98e3a4f`. **Next: slice 6**,
  the unicorn ceremony, Boba in the menus, the tutorial overlay.

### What happened (designer: "Yes": start 5d)
1. **The fixture `rainbow`** (`tools/godot_fixtures/rainbow_fixture.gd`). A probe subclass cannot catch a `_draw()` script's
   `draw_*` calls (they bind to the native methods), so the fixture reads each script's own source, renames ONLY its draw
   calls (and the bar's global `randf_range` / `randf`) to recorders, recompiles it and drives it by hand: every other line is
   Godot's. Refuses to run when a rename finds nothing. Five tables: 30 bars x 20 frames (an occasional 1.9 s hitch so a full
   bar crosses its 24-spark cap), 40 frames x 10 steps, 200 frames following a leaning / hidden plate, the sky at 4 sizes x 4
   effects budgets x 10 frames (long steps so clouds wrap). `--check` reproduces it byte for byte. Found on the way: a fixture
   frozen one frame late moved by the wall clock (froze it straight after `add_child`); a GDScript parse error leaves headless
   Godot idle for the runner's whole 30 min (KB G/03).
2. **PawCore**: `PcRainbow` (rainbow_bar.gd: the clock, sparks, accumulator, bands; rainbow_frame.gd: outline, phase, the rect
   grown round a plate; Godot's `Color.from_hsv`, `fposmod`, `Range.set_value`), within float32 (worst 1.5e-4 px); the sky
   `PcSky` in `PcBackdrop` (Godot's RNG seeded by `hash("skill_tree_sky")`, float32 drift and wrap, twinkle, the two cloud
   violets, the two-octave outline): **to the last float bit**. The sky's four colours generated into `PcBackdropTones.h` from
   palette.gd. Salt `RainbowSpark`. L1: 4 cases.
3. **Godot's ProgressBar snaps its value to a 0.01 step** (the fixture showed 0.6296 -> 0.63): the port's bars did not.
   `PawUi::SetBarValue` keeps what Godot's Range keeps (the XP bar's step 0.0001) and sizes the fill as Godot does; every bar
   now draws its fill at the fill's width with both caps (`EProgressBarFillStyle::Scale`; UMG's `Mask` cut the cap off square:
   the "bar fill's end" open since (58)).
4. **The game**: `UPawRainbowBar` (`WBP_RainbowBar`; the Reawaken page's bar, swapped in for `ReawakenBar`), `UPawRainbowFrame`
   (`WBP_RainbowFrame`; `ZoneReady` in the HUD's root overlay, following the zone plate's painted geometry so it leans with
   the plate's jitter; the old hue-walking border stand-in removed), `UPawTreeSky` (`WBP_TreeSky`; over the tree's Sky border,
   which still takes the drag). Godot's draw calls through `FPawFxCanvas` (+ `Fan`, + per-point-colour `Polyline` with a 1 px
   feather) and `PawUi::PaintCanvas`. Grafts: `PawHudScreen=swap:ReawakenBar>ReawakenRainbow;add:ZoneReady;restyle:WaveBar;
   restyle:ZoneBar;restyle:XpBar;restyle:BossBar`, `PawSkillTreeScreen=add:TreeSky` (the baker's restyle now carries a bar's
   fill style). Backups of the two assets before the graft: `build/wbp_backup_pre5d/`.
5. Docs 00, 03, 04, 05, 07, 09, 11, 12, the worklist §5d, appendix B; KB G/03 (four entries), U/09 (two rows, two bullets),
   INDEX.

### Verified
- L1 194 cases PASS; 26 PawCore sabotages `rainbow_*` / `sky_*` all CAUGHT (`build/sabotage_rainbow_5d.log`; one first MISSED,
  `rainbow_fraction_unclamped`: the Range makes that clamp unreachable, so a unit check now pins it, then CAUGHT). 608 in all.
- `Pawpocalypse.UI.Rainbow.Bar`, `.Frame`, `.Sky`, `.Widgets.*`, `.Chrome`, `.Hud.*` PASS; 16 realm sabotages `rainbow_*` / `sky_*`
  all CAUGHT (`build/sabotage_rainbow_realm_5d.log`; `rainbow_frame_zone_margin` first MISSED: the parity test does not compare
  that setting of a nested piece, so the Frame test now checks the code layout's frame too, then CAUGHT). 447 in all.
  `hud_xp_not_refreshed` retargeted to the XP bar's new line (the everyday `sabotage targets current` found it), CAUGHT.
- The first everyday run failed two gates, both fixed: L0 no-float (the sky test held `float` vectors: `PcSky::CloudOutline`
  now hands out doubles, still exact) and the stale target above.
- The tour (`build/tour/compare`): the Reawaken page's rainbow bar, the zone plate's turning frame ("Next biome - ready") and
  the tree's violet sky with its stars and clouds, in both games.

### Open
- The Reawaken banner (hud.gd `_build_reawaken_banner`) is not ported, so neither is its rainbow frame (margin 2, corner 10,
  width 5): `UPawRainbowFrame` takes those settings when the banner comes.
- The fill's minimum width: `SetBarValue` uses Godot's `round(r * (w - m)) + m` (m = 16 for the theme fill), from 5a's reading
  of Godot's ProgressBar; not yet measured side by side at a small fraction.
- The boss bar's Range step is not checked (its Godot scene not read): it keeps the plain fraction, with the caps now kept.
- The frame's 1 px feather is by eye; the tour shot is the check.
- From (61): the title's slot cards swell linearly where Godot's tween is cubic ease-out.

### Next step
*"Phase 6 slice 6 (doc 11): the unicorn ceremony, Boba in the menus, the tutorial overlay. Start by surveying the Godot
scripts (the unicorn's ceremony, boba_cameo.gd, the tutorial) and writing their slice plan into a worklist, as §Slice 5 was."*

---

## 2026-11-07 (61) - The docs brought up to date after slices 5a-5c

### Where the project is
- As (60): phase 6 slices 1-4 and 5a-5c done, everyday `check_all` exit 0 (`build/check_all_5c.log`). Nothing committed after
  `98e3a4f`. Only docs and the port map's table changed in this session (no code, no assets). **Next: slice 5d.**

### What happened (designer: "Make sure all docs are up to date")
Every doc checked against the code and (58)-(60). Slices 5a-5c had updated docs 00, 01, 05, 09, 11 and the worklist; these had
fallen behind:
- **Appendix B** (`tools/make_port_map.py`, regenerated): `ui_juice.gd` named a `UPawUiJuice` that was never made (now `PcJuice`
  + `FPawJuice`, built); `currency_icons.gd` now `PawUi::Icon` + `UPawIconText`, built in 5b.
- **Doc 03**: a `PcJuice` row in the PawCore table, a `UiJuice` row in the node map, the chrome and icons in the HUD row.
- **Doc 00**: `PcJuice`, `PawIcons` and `FPawJuice` in the inventory; phase 2's line said 563 PawCore sabotages (582).
- **Doc 07**: nothing from 5a-5b was there: the raster groups `chrome` and `icons`, `import_art.py`'s five modes, the UI art as
  built (the "still to come" line listed the icons), `DA_Icons`, the `Art/UI/Chrome` and `Art/UI/Icons` folders; the pipeline
  sketch still showed the plan's `Data/Atlas.csv` (built: `SourceArt/png/atlas/field_<n>.png` + `field.csv`).
- **Doc 05**: placeholders that phase 6 replaced (the job icons, the text tabs, the title cluster's words) and two title items
  built long ago but still listed "not yet" (Settings, the last slot kept in the settings).
- **Doc 09**: the `art rasters reproduce` row covers the chrome and the icons.
- **Doc 12**: five rows from 5a-5c (the ring painted over the plate, CRLF in string tables, no NativeTick on Common UI buttons,
  hover on a button Godot disables, `IsVisible()` in tests) and the global `randf_range` in the Godot RNG row.
- **Doc 04** §8: the spike tools (`tools/spikes`) went with the spike code.
- Docs 01, 02, 06, 08, 10, 11, 13 and CLAUDE.md needed nothing.

### Verified
- Appendices A and C regenerated into a scratch folder: identical to the docs (and every `Data/*.csv` identical).
- Appendix B regenerated from the Godot repo as it is today (79 scripts, every one mapped).

### Open
- As (60). Also seen: the title's slot cards still swell linearly where Godot's tween is cubic ease-out (doc 05); Godot's easing
  is now in PawCore (`TweenInterpolate`), so it could be matched.

### Next step
*As (60): "Phase 6 slice 5d (docs/WORKLIST-NIGHT-2026-11-05.md §5d): `rainbow_bar.gd`, `rainbow_frame.gd` (the zone plate's and
Bait's frame), `skill_tree_sky.gd`, each a Slate paint of Godot's draw calls against a probe fixture of the real node (as slice
2's backdrop); it also fixes the bar fill's caps."*

---

## 2026-11-06 (60) - The nightly after 5b, all green; phase 6 slice 5c: the motion

### Where the project is
- Phase 6 slices 1-4, 5a, 5b and **5c** done. Everyday `check_all` exit 0 (`build/check_all_5c.log`; `Pawpocalypse.UI` 68
  tests, L1 190 cases, the game build too). Nothing committed after `98e3a4f`. **Next: slice 5d**, the rainbows and the tree's
  sky.

### What happened (designer: "Yes pmease": the nightly, fix what it finds, then 5c)
1. **The nightly `-Full`** (`build/check_all_full_20261106.log`, 12:46-22:24, 9 h 38 min): all 31 gates PASS, 563 PawCore and
   412 realm sabotages all CAUGHT, both clean builds pass. Nothing to fix.
2. **The fixture `juice`** (`tools/godot_fixtures/juice_fixture.gd`): Godot's own `Tween.interpolate_value` for linear, sine,
   quad, back, elastic x in, out, in_out, out_in (8,840 rows), and the REAL ui_juice.gd on a real Button: 160 random sequences
   of 60 events and frames (hover, press, punch, pop-in, jitter on / off, disabled, hidden, the effects setting), the tweens
   stepped by `Tween.custom_step` from a synchronous `_ready` (9,600 rows), and every jitter lean (301). The leans: Godot's
   global `randf_range` is `randd()` (THREE PCG outputs), not the RandomNumberGenerator's float `randf_range` (two): a shadow
   RNG drifted after the first lean (164 mismatches) until rebuilt from `randi()`; the fixture now refuses to finish on any
   lean that does not match where the rotation landed. `--check` reproduces it byte for byte.
3. **PawCore `PcJuice`** (`PcJuice.h/.cpp`): `TweenInterpolate` (easing_equations.h in doubles; worst 4.3e-7 off Godot's
   float32), and UiJuice's state machine: one tween per property (a new one kills the old), each leg starting from the value
   at its first step, the punch's leftover passed to its second leg, the jitter's next step a frame later, asleep when hidden,
   off with the effects. L1 `Juice` 3 cases (worst sequence 1.5e-7). Salt `Jitter` in PcRng.h.
4. **The game: `FPawJuice`** (`UI/PawJuice.h/.cpp`), one per screen, ticked by its owner, written as the render transform about
   the centre. Hover on the shop rows, job buttons, tabs, travel rows, claim buttons (hud.gd), the tree's nodes, the title's
   cluster; lean on the Reawaken tab (+ its hue glow, `_tick_reawaken_glow`, `PawUi::FromHsv`), Bait (F42), a newly open
   biome's row, the zone plate, READY nodes in the spend step; punch on a bought row or node; pop-in on the tree's leaving
   plate. Common UI's OnHovered is the pointer's and only on an enabled, interactable button (= `_on_enter`'s check); the
   tree's nodes stay interactable in the port, so `SetHoverOff` carries Godot's `disabled` there. Visibility reads the
   property, not `UWidget::IsVisible()` (false before a Slate widget exists: the KB had it, I re-found it).
5. **Finding F42** (docs/01 and the Godot repo's findings file): `_refresh_bait_button` stops the Bait's lean and starts it
   again on every refresh, so it trembles and never leans. Copied.
6. Docs 00, 01 (F42), 05 (motion), 09, 11, the worklist's §5c; KB G/03 (global randf_range = randd; Tweens stepped by hand),
   U/08 (OnHovered is the pointer's; UCommonButtonBase is DisableNativeTick), U/24, INDEX.

### Verified
- L1 `Juice`: 3 cases PASS; 19 PawCore sabotages `juice_*` all CAUGHT (`build/sabotage_juice_5c.log`, `_juice2_5c.log`: three
  first written did not compile under warnings-as-errors, reworded to keep the parameter used, then CAUGHT). 582 in all.
- `Pawpocalypse.UI.Juice.Hud`, `.Tree`, `.Title` PASS; 19 realm sabotages `juice_*` all CAUGHT
  (`build/sabotage_juice_realm_5c.log`). 431 in all.
- The tour (`build/tour/compare`): the open biome's row and the zone plate lean in both games.

### Open
- Not built (their owners are not ported): Boba's badge hover, cameo jitter and CEO pop-in, the Reawaken banner (pop-in and
  lean; HANDOFF-LOG (23) "the Reawaken banner and glow": the glow is done), the language menu, the tutorial plate, the tooltip.
- No test for Bait's lean (needs a biome with its bait earned); the tree's and title's `NativeTick` stepping is seen only in the
  tour (the tests step the juice directly).
- Common UI re-fires hover when a hovered button is locked or unlocked; Godot's mouse_entered does not: a hovered row that
  becomes affordable pops in the port, not in Godot. Small; not changed.
- From (59): the Achievements tab count, the bright yellow newly open plate, the tooltip, the bar fill's end (5d), the pad check
  of the ring.

### Next step
*"Phase 6 slice 5d (docs/WORKLIST-NIGHT-2026-11-05.md §5d): `rainbow_bar.gd`, `rainbow_frame.gd` (the zone plate's and Bait's
frame), `skill_tree_sky.gd`, each a Slate paint of Godot's draw calls against a probe fixture of the real node (as slice 2's
backdrop); it also fixes the bar fill's caps."*

---

## 2026-11-06 (59) - Phase 6 slice 5b: the icons and the quick bar

### Where the project is
- Phase 6 slices 1-4, 5a and **5b** done. Everyday `check_all` exit 0 (`build/check_all_5b.log`; `Pawpocalypse.UI` 65 tests,
  the game build too). Nothing committed after `98e3a4f`. **Next: the nightly `-Full`**, then slice 5c, the motion.

### What happened (designer: "Do all next slices in order untill you think you need a long test")
1. **Pipeline**: `tools/raster_svg.py` group `icons` (178 SVGs at their own 128 px, as Godot imports them: the 6 tabs, 3
   currencies, 4 quick buttons, 3 title plates, the tick, 41 achievements, the cards, skills and jobs; Godot draws only 17 of the
   `ui_*` SVGs, `ui_card_frame` / `ui_close` / `ui_lock` / `ui_skill_node*` / `ui_slot` nowhere); `tools/import_art.py icons`
   -> `/Game/Pawpocalypse/Art/UI/Icons/T_*` + `DA_Icons` (UPawIconSet). `PawUi::Icon(stem or res:// path)` is Godot's
   `Art.texture` (null when there is no art).
2. **Pieces**: `UPawButton` got an optional `Icon` (Godot's `Button.icon`: at the content's left edge, the text centred in what
   is left; an icon-only button centres it), `SetActiveLook` (a toggle held down: the theme's pressed plate, Common UI's
   selection with `SetIsInteractableWhenSelected(true)`) and the bare look (`UPawBareButtonStyle`, `BP_BareButtonStyle`: a
   TextureButton). New piece `UPawIconText` / `WBP_IconText` (`wrap_label`: a currency icon before a label).
3. **The HUD as hud.gd**: currencies (34 px), the wave line's Bait icon, **icon-only tabs** with the names as tooltips and the
   open tab on the pressed plate (the port's coloured tab plates were not Godot's: palette UI_TAB_* is not used by Godot's
   HUD), the Reawaken page (progress Catnip, payout and total Stardust, the button), achievement rows (36 px icon) with Claim /
   Claim all in Stardust, job rows (40 px), Bait (Catnip's icon when Catnip is short), realm rows asking for Catnip. Icons that
   rules decide are set in `OnLayoutReady` (both modes; build code reaches only the bake).
4. **The quick bar** (hud.tscn QuickSeparator / QuickBar / QuickStatus), absent from the port until now: Settings (the dialog on
   the Modal stack), the tree (browse), Save ("Saved."), Save and back to the title; mouse only, as Godot's navigator never
   visits it. New `ST_UI` keys `QUICK_*`.
5. **Elsewhere**: the title's three plates (icons, no words), the card pictures (104 px), the tree's node icons (56 px), the
   Pounce clock's paw (`skill_pounce`, r x 1.3).
6. **The baker**: `props:Part` (a piece's own settings into the asset: the tabs' and plates' words cleared), `add:` takes a panel
   with children (made empty in the asset, then each child by name). Grafted: `WBP_Button` (Icon), `WBP_HudScreen` (7 swaps:
   the discs, the wave line, the three Reawaken / achievement lines; the quick bar; tabs' props), `WBP_MainMenu` (plates' props),
   `WBP_CardFace` (Art), `WBP_TreeNode` (Icon); new `WBP_IconText`, `BP_BareButtonStyle`. Backups: `build/wbp_backup_pre5b/`.
7. **A line-ending trap, found and gated**: Python's `Path.write_text` writes CRLF on this machine; `ST_UI.csv` written so
   turned every line break inside a quoted string into two (4 UI tests failed: "line one\n\nline two"). Converted back to LF;
   **new L0 rule**: any `\r` in an `ST_*.csv` fails (seen failing on the CRLF table, passing after). Note: many sources are now
   CRLF (Python patches) and some LF (sed); Git normalises them on commit; only runtime-parsed files matter.
8. Docs 00, 05 (§7 icons, the placeholders list), 09, 11, the worklist's §5b, CLAUDE.md; KB U/09 (§2.3 panels with children and
   piece settings; §3 two rows), U/20 §2 (LF string tables), U/17 §3 (line endings from tools), INDEX.

### Verified
- `Pawpocalypse.UI.Icons` PASS: every constructed name and data icon_path resolves (Godot's 128 px), the HUD's icons, tabs and
  pressed plate, the quick bar; `.Title.Look`, `.CardPick.Look`, `.Tree.Opens` check the plates, card pictures and node icons.
- **17 new realm sabotages, all CAUGHT** (`icons_*` 13, `quick_*` 4; `build/sabotage_icons_5b.log`, `_quick_5b.log`,
  `_icons2_5b.log`); `chrome_tab_separator_flat` retargeted (`TabRule`: the quick separator made the old target ambiguous),
  CAUGHT again. 412 in all.
- The tour beside Godot: the dock (tabs, currencies, Reawaken page, achievements, realms), card pick, tree, title: Godot's icons
  where Godot has them.

### Open
- **The nightly `-Full`**: 5a and 5b changed most screens; the 382 older realm sabotages have not been run since (only
  "targets current" is checked daily).
- No test or sabotage for: the Pounce clock's paw (painted), the quick bar's Settings and tree presses (no stacks in the
  test world), the settings lists' ring (from (58)).
- The Achievements tab shows its count beside the trophy; in Godot's shot the count is not visible (" 1" on an expand_icon
  button): look whether Godot draws it at all before calling it a port difference.
- "Travel: <biome>" (a newly open biome): Godot's plate is bright yellow, the port's a 45 % gold wash (pre-existing; BIOME_READY_TINT).
- The custom tooltip (upgrade_tooltip.gd) is not built: the engine tooltip shows the tab and quick-bar names.
- From (58): the wave bar's fill end (5d), the pad check of the ring, everything open in (53)-(56).

### Next step
*"Run the nightly: `powershell -ExecutionPolicy Bypass -File tools\check_all.ps1 -Full` (~10 h, editor closed, nothing else
building). Fix whatever it finds (a MISSED sabotage: retarget or retire with the reason, as (56)). Then phase 6 slice 5c, the
motion (docs/WORKLIST-NIGHT-2026-11-05.md §5c): Godot's Tween easing (TRANS_BACK / ELASTIC / QUAD / SINE, EASE_OUT /
IN_OUT) in PawCore exact against a fixture sampling the real Tween, then UiJuice's hover / jitter / pop_in / punch at
Godot's call sites, render transform only."*

---

## 2026-11-06 (58) - Phase 6 slice 5a: the chrome

### Where the project is
- Phase 6 slices 1-4 and **5a** done. Everyday `check_all` exit 0 (every gate PASS, `Pawpocalypse.UI` 64 tests). Nothing
  committed after `98e3a4f`. Next: slice 5b, the icons.

### What happened (designer: "what's next on the dev list?" then "Yes please" to 5a, starting with step 1)
1. **Step 1 measured**: the six `BP_*Style` Blueprints save no property values (their name tables hold no `NormalBase`,
   `ButtonPadding`...), so a changed C++ constructor default reaches them. Confirmed after the rebuild: the Blueprints resolve to
   the new brushes (`Pawpocalypse.UI.Chrome` reads them). Code owns the button looks until the designer edits a BP.
2. **The design table was wrong about the consumers** (corrected in the worklist): Godot overrides the theme with flat
   styles on the dock (`hud.tscn` `DockPanel`: `ui_dock.svg` is drawn nowhere), the title / pause / settings windows, the toast,
   the boss panel, the zone plate and bar, the XP bar: those keep their Rounded brushes. What Godot's theme really draws, now
   drawn the same way: Button (4 states; a locked row = the disabled plate), the settings lists (OptionButton plates, the
   PopupMenu panel and hover), the WindowPanel (job picker, victory), the theme ProgressBar (the wave bar; the Reawaken bar's
   track), the HSeparators.
3. **Pipeline**: `tools/raster_svg.py` group `chrome` (9 SVGs at their own size, 1:1, as Godot's `svg/scale=1.0`; covered by
   the existing `--check` gate), `tools/import_art.py chrome` -> `/Game/Pawpocalypse/Art/UI/Chrome/T_ui_*` (UI textures).
   `PawUi::Chrome(EPawChrome)` (PawUiKit): Box brush, `ImageSize` = the SVG's size, `Margin` = Godot's `texture_margin` / size.
4. **The selection ring** (SBF_focus: 3 px UI_FOCUS, radius 12, 3 px outside) is no longer part of the hovered brush: it is
   painted OVER the plate by `UPawButton::NativePaint` (and `UPawSettingsDialog::NativePaint` for its lists). Rule
   (`UPawButton::RingRule`): hovered, or focused when the last input was not the mouse (Godot hides mouse focus). Why: a
   textured hover plate cannot also carry a ring, and Common UI never hovers a locked (interaction-off) button, nor does Slate
   turn focus into hover. **A behaviour change worth a look on a pad**: keyboard / pad focus now rings even where Common UI
   gave no hover (locked rows included).
5. **Assets**: a new baker op `restyle:Part` (copies a part's brush / padding / bar style / size from the code layout into the
   asset; nothing else) and `names:` (lists an asset's widgets). Applied: `WBP_HudScreen` (`swap:Border_3>TabSeparator`,
   `restyle:WaveBar`, `restyle:ReawakenBar`), `WBP_SettingsDialog` (`SectionSeparator`, `SectionSeparatorBox`: 4 px),
   `WBP_JobPicker` and `WBP_VictoryScreen` (`Panel`). Backups of the four pre-chrome assets:
   `build/wbp_backup_pre5a/` (untracked; the Widgets folder is not in git yet either).
6. **F41 (new)**: Godot's separators draw nothing (the 2 + 2 px texture margins fill the HSeparator's 4 px; measured in
   Godot's tour shot). The port copies it. doc 01 §5 and the Godot repo's `docs/pawpocalypse-port-findings.md` (uncommitted).
7. Docs 00, 01, 05 §7 (the look as built), 09, 11, the worklist's §5a, CLAUDE.md; KB U/09 (§2.3 restyle, §3 three rows,
   §4 nine-patches and BP default propagation), G/07 (HSeparator), INDEX.

### Verified
- `Pawpocalypse.UI.Chrome` PASS: the 9 brushes against Godot's numbers (held in the test, apart from PawUi's table), the
  textures' size and no mips, Godot's margin rule, BP_ButtonStyle / BP_LockedButtonStyle / UPawOption states, the 6 asset parts
  AND their code layout, the ring rule, the ring painted off screen on a real `SCommonButton` hover (0 at rest, 1 hovered).
- **13 new realm sabotages, all CAUGHT** (`chrome_*` 9, `ring_*` 4; `build/sabotage_chrome_5a.log`); clean build PASS; 395 in all.
- **The asset half seen to fail**: the four pre-chrome Widget Blueprints swapped in, the test failed on exactly the 7 asset
  checks (nothing from code), the new assets put back byte-identical.
- The tour beside Godot (`py tools/godot_tour.py`, both sides re-shot: the old Godot shots of the job picker and victory
  predated F21): the dock's rows, the job picker and the victory window now wear Godot's plates.
- Everyday `check_all` exit 0 (`build/check_all_5a.log`).

### Open
- The settings lists' ring (`OptionShowsRing`) has no test or sabotage: no headless settings dialog with a hovered list yet.
- UMG clips a bar's fill at the fraction; Godot resizes the fill nine-patch (both caps kept): the wave bar's fill end is
  square. For slice 5d (the bars' own paint, with rainbow_bar.gd).
- The dock's tabs: Godot's are icons with a darker plate when selected; the port's are words with an orange plate (5b).
- The Quick bar (and its separator) is not in the port yet (5b brings `ui_quick_*`).
- The pad check of the ring change (4 above) and everything open in (53)-(56).

### Next step
*"Phase 6 slice 5b, the icons (docs/WORKLIST-NIGHT-2026-11-05.md §5b): an icon data asset generated by the importer (name ->
texture), every name Godot constructs resolving (test + a sabotage with one missing), then the placeholders replaced (grep
"phase 6" in Source/Pawpocalypse/UI: tabs, currency discs, achievement marks, the Pounce clock, the title cluster, the card
picture, the tree's node icons), the tour beside Godot."*

---

## 2026-11-06 (57) - The docs brought up to date

### Where the project is
- As (56): the nightly baseline proven, nothing committed after `98e3a4f`. Next: phase 6 slice 5a, the chrome.

### What happened (designer: "Make sure all docs are up to date")
Every doc read against the code, the gate logs of 2026-11-06 05:45 and (55)-(56). Fixed:
- CLAUDE.md and `tools/check_all.ps1`'s header: the nightly is ~10 h (measured 2026-11-05/06), not ~7.1 h / ~5.5 h.
- Doc 00 and doc 11: "next: the nightly" replaced by the night's result and the next step (slice 5a; its design in
  `docs/WORKLIST-NIGHT-2026-11-05.md`).
- Doc 09 §7: PawCore 187 test cases / 6.61 M assertions (was 182), `Pawpocalypse.Input` 18 tests (`.PadAudio`),
  `Pawpocalypse.UI` 63; two everyday gates that had no row (`audio export reproduces`, `sabotage targets current`).
- Doc 12: the stale-target pitfall now names the everyday gate; a new row for UI sabotages MISSED in bake-only code.
- KB U/18 §8 (+ INDEX row and symptom): a mutant in code that only runs when an asset is created is caught by nothing.

### Verified
- Appendices A, B and C regenerated into a scratch folder: identical to the committed docs (and `Data/*.csv` identical).
- `sabotage_pawcore.py --list` and `sabotage_realm.py --list`: every target current after the edits.

### Open
- As (56).

### Next step
*As (56): "Phase 6 slice 5a, the chrome (docs/WORKLIST-NIGHT-2026-11-05.md §Slice 5a): first measure who owns a button's look
(does a changed C++ default reach `BP_ButtonStyle`?), then the nine-patch brushes from Godot's margins table, the test, the
sabotages, the shots beside Godot."*

---

## 2026-11-06 (56) - The nightly, fixed (night worklist 2026-11-05)

### Where the project is
- Phase 6 slices 1-4 done, plus the Output device and the pad-audio toast (55). **The nightly baseline is proven**: every gate
  ran and passed or was fixed and rerun alone; the everyday set exit 0 at 05:45. Next: phase 6 slice 5, starting with 5a.
  Nothing committed after `98e3a4f`.

### What happened (designer: "When the test finishes fix anything that's broken. I want you to create [and] execute an overnight plan")
The plan: `docs/WORKLIST-NIGHT-2026-11-05.md` (closed). Its read-only part, written while the nightly ran: **slice 5's design**
(5a the chrome with Godot's nine-patch margins table and the first step, 5b the icons, 5c the motion, 5d the rainbows and the
tree's sky).
1. **The nightly `-Full`** (20:40 -> 02:44, another project's soak sharing the CPU): 29 of 30 PASS: PawCore sabotage 563/563
   (3 h 15 min), the Godot fixtures (23 min), L1 long (24 min), mirror 200k, fuzz 30k, the pawsim sweep (1 h 53 min), L4 pad-only.
   **Realm sabotage FAILED in 0 s**: it refused to start on one STALE target, `settings_effects_ignored` (its line in
   PawRealmView.cpp was rewritten by slice 3), so none of its 388 ran.
2. **Fixed**: retargeted to the line that hands the effects budget to FPawVfx; CAUGHT alone. **The cause**: runs with `--only`
   check only the targets they name, so a stale one hides until the nightly. New everyday gate **`sabotage targets current`**
   (`--list` of both tools: every target, nothing built, seconds); seen to fail on a rewritten target in either tool and pass
   restored (byte-identical).
3. **The realm gate rerun** (02:47 -> 05:42): 381 of 388 CAUGHT, clean sources PASS. The 7 MISSED, each looked at:
   `view_no_effects` (since slice 3 the effects spawn in `FPawVfx::OnFrameEvents`, not `SpawnEffects`): retargeted, CAUGHT alone.
   Six were **retired**: their lines are in `Initialize`'s `BuildsInCode` branch, which since the Widget Blueprints (38) runs only
   when the baker builds the asset, so the property (a hit-test visibility, a dim, a panel brush, a button style) is the asset's;
   the tests still assert it on the real asset (they pass), but no code sabotage can reach it. 382 realm sabotages now, every
   one seen CAUGHT.
4. Docs 00, 09; KB U/18 §8 (check every mutation target daily) + INDEX (+ symptom).

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (05:45), the new gate among them.
- The night's logs: `build/check_all_full_20261105.log`, `build/check_all/*.log`, `build/realm_sabotage_rerun_20261106.log`.

### Open
- **Asset-held properties have no negative test**: the six retired sabotages' properties now live in Widget Blueprints, where a
  code sabotage cannot reach. Their tests read them from the real assets; an asset-level sabotage (edit the uasset headless,
  run the test, restore) would close the gap: worth doing with slice 5a, which edits those assets anyway.
- From (55): the Output device's real swap is not heard (the designer's two-device `-game` check). Everything else open in
  (53)-(55).

### Next step
*"Phase 6 slice 5a, the chrome (docs/WORKLIST-NIGHT-2026-11-05.md §Slice 5a): first measure who owns a button's look (does a
changed C++ default reach `BP_ButtonStyle`?), then the nine-patch brushes from Godot's margins table, the test, the sabotages,
the shots beside Godot."*

---

## 2026-11-05 (55) - The Output device and the pad-audio toast, before the nightly

### Where the project is
- Phase 6 slices 1-4 done, and the two gaps (54) found in phase 6's audio now closed. Next: the nightly `-Full`, then slice 5.
  Nothing committed after `98e3a4f`.

### What happened (designer: "What's next can you do next before the long test?")
The two gaps fit before the nightly so that its baseline covers them; slice 5 still waits for it (it changes every screen).
1. **PawCore**: `Pc::SettingsOutputWant` (settings_manager.gd `_apply_output_device`: the stored device while the system lists
   it, else "Default") and `Pc::DevicePadAudioToast` + `DevicePadAudioWaitMs` 2000 (input_device.gd `_watch_for_pad_audio`);
   `PcDeviceToast::PadAudio`. Hand-written L1 tests from the Godot code (no fixture: a headless Godot lists only "Default", so
   the real `AudioServer` cannot be driven); 3 sabotages CAUGHT (563 PawCore in all).
2. **The output device** (`UPawAudioSubsystem`: `OutputDeviceList`, `ApplyOutputDevice`): the system's active devices by
   friendly name from the WASAPI mixer's device cache (the same names Godot stores, so a Godot settings.cfg carries over),
   "Default" first; applied at startup, on every settings change and when the pinned device is removed. **Unreal follows the
   system's default even after a swap** (only `OpenAudioStream` sets the listening flag), so a chosen device is pinned with
   `SetIsListeningForDeviceEvents(false)` beside the forced `RequestDeviceSwap`; "Default" turns it back on. Never in the editor.
   Seams `SystemDevices` / `SwapOutput` for the tests. Modules `AudioMixer`, `AudioMixerCore`.
3. **The Output list** (`UPawSettingsDialog`) shows the system's devices; the settings tests fix the system to "Default" only
   (Godot's fixture is headless), so `.MatchesGodot` never depends on the machine.
4. **The pad-audio toast** (`UPawInputDeviceSubsystem`): a connect after the boot window starts a watch; 2 s later (real time,
   the core ticker: a paused game too) a new device while following the system shows Godot's text (`INPUT_toast_pad_audio`
   in ST_UI, `UPawToast::ShowPadAudio`); each connect watches on its own; the setting is read after the wait.
5. Docs 00, 05 (§3), 07 (§6), 08, 09, 11. KB: U/14 §2.2 (listing and pinning an output device; the trap), INDEX (+ symptom).

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (game target built too).
- New engine tests: `Pawpocalypse.Audio.Output`, `.UI.Settings.Output`, `.Input.PadAudio`. Sabotages all CAUGHT: 3 PawCore
  (`settings_output_want_unlisted`, `device_pad_audio_*` 2), 8 realm (`audio_output_*` 4, `pad_audio_*` 4); 388 realm in all.

### Open
- **Not heard**: the real swap was never made (the editor never makes it, headless has no device). The check is a `-game` run
  with two output devices: pick one, plug in a pad with a headset, unplug the chosen device. Then the KB's U/14 §2.2 goes from
  `[Docs]` to `[Verified]`.
- Not tested: the editor guard in the engine swap (a sabotage would need a real device to notice).
- Everything open in (53) and (54) other than these two.

### Next step
*"Run the nightly on its own: `powershell -ExecutionPolicy Bypass -File tools\check_all.ps1 -Full` (editor closed, ~7.6 h;
exit 0 = done). Fix what it finds. Then phase 6 slice 5 (survey in (53)), then slice 6, then the exit tours."*

---

## 2026-11-05 (54) - Docs audit after phase 6 slices 1-4

### Where the project is
- Unchanged from (53): phase 6 slices 1-4 done; next the nightly `-Full`, then slice 5. Nothing committed after `98e3a4f`.
  Only docs changed in this session (no code, no assets).

### What happened (designer: "Make sure all docs are up to date")
1. Every doc checked against the code as built. Updated: doc 00 (the state, the "what already exists" table: Render, Realm's
   look and effects, Audio, the art and audio tools, the materials, `SourceArt` / `SourceAudio`, PawCore's `PcBackdrop`, 560
   PawCore sabotages, ~7.6 h `-Full`), 03 (module tree, `PcEvents` as built, `PcBackdrop`, the audio row, the backdrop and
   effects rows, visual definitions as built: no per-creature asset, the art found by the data's stem; plugins:
   ProceduralMeshComponent on, Niagara unused), 04 (the pulse on the CPU, the sort priorities, the ortho Z-clip note, the
   effects as draw calls, §7's table marked as the plan), 05 (glyph source, menu music and click done), 06 (the audio CSVs are
   not tables), 07 (§2.1, folders, VFX as built), 08 (the volumes act), 09 (§7, the art and atlas gates, the runner's
   unfinished-test check), 10, 11 (phase 6: slice 3 built as draw calls, exit-criteria progress, next the `-Full`), 12 (new
   pitfalls), appendix B (regenerated). Docs 01, 02, 13 and CLAUDE.md needed nothing.
2. **Two gaps found** (now open in docs 05, 07, 08 and 11): the settings' **Output device** (Godot sets
   `AudioServer.output_device`; the port always plays to the default device) and the **pad-audio toast**
   (input_device.gd `_watch_for_pad_audio`). Neither was in a slice's list; both belong to phase 6 (audio) and should be done
   before phase 6's exit.

### Open
- The Output device choice and the pad-audio toast (above). Everything listed open in (53).

### Next step
Run the nightly gate on its own (nothing else beside it: it edits sources for its sabotages):
`powershell -ExecutionPolicy Bypass -File tools\check_all.ps1 -Full` (~7.6 h; exit 0 = done). Then phase 6 slice 5 (survey
in (53)), with the Output device and the pad-audio toast fitted in before the phase's exit tours.

---

## 2026-11-05 (53) - Phase 6 slice 4: the sound

### Where the project is
- Phase 6: slices 1-4 done; next slice 5 (UI art and motion). Nothing committed after `98e3a4f`.

### What happened (designer: "Do all next slices in order until you think you need a long test")
1. **The sound table from Godot**: `py tools/export_audio.py` (audio_manager.gd EVENTS / the sounds each plays / FALLBACK_MUSIC)
   -> `Data/AudioEvents.csv`, `Data/AudioMusic.csv` (also via `export_godot_data.py`: the data-export gate covers them; not
   tables: `NOT_TABLES` in import_data / gen_paw_rows) and the 53 files something plays -> `SourceAudio/` (28.9 MB; LFS);
   `--check` gate `audio export reproduces` (seen to fail on one changed number).
2. **UE 5.8 imports OGG directly**: `py tools/import_audio.py --run` -> `SW_*` (`/Game/Pawpocalypse/Audio/Sfx`, `/Music`, the music
   looping) and `DA_AudioBank` (`UPawAudioBank`).
3. **`UPawAudioSubsystem`** (`Source/Pawpocalypse/Audio`): AudioManager's rules (16 voices in turn, interval / pitch spread /
   dB / round robin per event, the buses from the settings live, the 1.2 s crossfade in dB, a track with no file carries on,
   real time). When: a progress observer (purchase, Reawaken, skill level, achievement earned / claimed, a hand, a card
   taken; `UPawGameSubsystem::ProgressEpoch` so a load or a new game is silent), the realm's field events (the drop, the
   golden chime, the boss's summon and music, phase, defeat), the biome's music on a biome change and back after a Reawaken
   dismisses the boss (a timeout leaves the boss's music on, as Godot), the UI's clicks (dock tabs, the buy multiplier, a
   refused purchase, pause, the title's cluster), the title's music.
4. **Finding F40**: five sounds play twice, layered (a signal hookup plus a direct call with no interval). The port copies it.
5. `run_automation.py` waits up to 30 s for this project's exiting editor (a gate had refused on its predecessor's dying process).
6. Docs 00, 01 (F40), 07 (§6 as built), 09, 11, CLAUDE.md. KB: U/14 §2.1 (OGG import, a sound table without Sound
   Classes, audio tests with no device, a progress observer), Godot/07 (a sound played twice), INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (new: `audio export reproduces`, `L3 Pawpocalypse.Audio` 3 tests). A `-game`
  run with the real device: no audio warning or error. Not heard: the designer's ears are the check.
- Sabotages all CAUGHT: 8 `audio_*` (380 realm sabotages in all).

### Open
- Not wired yet: `boba_cameo`, `unicorn_neigh` (their callers come in slice 6), the language list's click (phase 7).
- From before: F38-F40, Q9, phase 5's hand-checks, F34-F37, Q10, the nightly `-Full` not run since (33), the packaged bench.

### Stopped here for the long test (designer: "until you think you need a long test")
The nightly `-Full` has not run since (33); since then ~70 sabotages were added or retargeted (PawCore 549 -> 560, realm
351 -> 380), the event flags widened, the automation runner changed twice. Slice 5 rewrites the look of every screen's assets,
so it should start from a proven baseline. `-Full` edits sources for its sabotages: nothing else can run beside it.

### Slice 5, surveyed (for the next session)
- **Chrome is nine-patch art in Godot** (`scenes/ui/pawpocalypse_theme.tres`: StyleBoxTexture, `texture_margin` 16 for the
  buttons, 20 for the panel...): `ui_button_normal/hover/pressed/disabled`, `ui_panel`, `ui_window`, `ui_dock`, `ui_bar_track/fill`,
  `ui_separator` (+ `ui_slot`, `ui_card_frame`, `ui_skill_node[_locked|_owned]`, `ui_lock`, `ui_close`). The port draws rounded
  flat brushes (`PawUiConstants.h` "until the art arrives"). In UMG: Box brushes over 2x rasters with ImageSize = the SVG's own
  size and the margins as fractions; the button styles are `BP_*` assets that own their look (edit them by script, never re-bake).
- **Icons by constructed name** (Godot `Art.texture`): `ui_tab_<id>` (tabs are icons, 52 x 46 buttons, hud.gd TAB_ICON_SIZE),
  `ui_icon_catnip/stardust/bait` (currency_icons.gd: 26 px on buttons, 24 beside labels; the status bar's TextureRects; the
  Bait button), `ui_quick_*` (TextureButtons), `achievement_<id>` (36 px, hud.gd 2219), the data's `icon_path` for cards
  (catches_pick_ui.gd ART_HEIGHT), skills (skill_tree_ui.gd ICON_SIZE 56) and jobs (hud.gd 956: 40 px), `ui_menu_*` on the
  title's cluster (56 px plates), `ui_tick` (the language list, phase 7). Placeholders to replace are marked "phase 6" in
  `Source/Pawpocalypse/UI` (grep it: PawHudScreen, PawCardPickScreen, PawSkillTreeScreen, PawMainMenu, PawPounceClock, PawButton).
- **UI motion**: `ui_juice.gd` (194 lines), `rainbow_bar.gd`, `rainbow_frame.gd` (the Bait-ready plate), `skill_tree_sky.gd`.
- Pipeline: `tools/raster_svg.py` groups for the UI art at 2x the drawn size, `tools/import_art.py` (UI textures), parts
  grafted with `py tools/bake_widgets.py --run --graft "Class=add:Part"`.

### Next step
*"Run the nightly: `powershell -ExecutionPolicy Bypass -File tools\check_all.ps1 -Full` (editor closed, ~7.5 h). Fix what it
finds. Then phase 6 slice 5 (above), then slice 6 (the unicorn ceremony, Boba, the tutorial overlay), then the exit tours."*

---

## 2026-11-05 (52) - Phase 6 slice 3: the effects

### Where the project is
- Phase 6: slices 1-3 done (the field's art, the backdrops, the effects); next slice 4, audio. Nothing committed after `98e3a4f`.

### What happened (designer: "Do slice 3")
1. **Godot's effects ported as its own draw calls.** Every Godot effect is `_draw()` code, so instead of doc 04's SDF quads
   and Niagara the port carries a Godot-style draw API, `FPawFxCanvas` (Circle, Arc, Polyline, Line, Polygon, from_hsv:
   vertex-coloured triangles at a depth), and `FPawVfx` (`Realm/PawVfx`): vfx_burst / slam / swipe / beam / shards /
   edge_fire, aura_ring (incl. the Unicorn's rainbow), poop_zone_visual, damage_zone_field, boss_telegraph, golden_sparkle
   and the Lone Hunter's halo, line by line, with tier_controller.gd's and player.gd's choice per field event (the Pied
   Piper's arrivals, Sixth Sense's pulse, the drag tether, the rush pulse, Riddle, Execute, the swipe and Anvil Echo's reach,
   Hammerfall, Overkill's 3 beams a frame, the Prism's paced chain and burst, the Pounce's burst + slam + dust, the kill pops
   and the golden spray, the 70 ms bonus pops, the giblets, the bomb, the boss's window / phase / heal / adds and its three
   delayed death rings, the Cone of Shame's ring, the job ring). The effects setting as Godot's spawners do; a 2,048 live cap
   as a flood guard. The placeholder rings and discs are gone.
2. **PawCore**: three event flags the effects need (Riddle armed, Execute's threshold, the Prism's kill); `Flags` is 32 bits.
   Checked in the existing Riddle, Execute and beam-kill tests; 3 sabotages CAUGHT.
3. **The renderer**: three translucent procedural-mesh bands rebuilt each frame (`EPawFxBand`: Ground under the shadows,
   Under over them, Over per-vertex in Godot's z order among the sprites), priorities renumbered. **The camera's automatic
   ortho planes clip Z 400** (found when the top effects vanished; explicit planes made it worse, cause unknown): everything
   lies in Z 0..300, a test asserts it.
4. **A gate bug, fixed**: `tools/run_automation.py` counted completed tests only, so a test that crashed the editor (an
   access violation in `Pawpocalypse.Realm.View`, a stale sparkle slot after travel, also fixed) passed as "10 tests". It now
   fails on a started test without a result and on a critical error; seen to fail on that very crash.
5. **Finding F39** (doc 01 §5, the Godot findings file, uncommitted there): `VfxBurst.spawn` sets `global_position` before
   `add_child`, so the two rings spawned under the cat draw at twice its position (measured in the real Godot: (1200, 500)
   -> (2400, 1000)). The port copies it.
6. Dev tour step `paw.Fx` (one of each effect round the cat). Seen in a `-game` window: the burst, both swipes (plain with
   its ink edge, the earned crit pink), the beam, the gold shards, the slam with its cracks and dust, the boss's telegraph
   over the kittens.
7. Docs 00, 01 (F39), 04 (§7 as built), 09 (the Vfx tests, the runner, 560 / 372 sabotages), 11. KB: 2D/02 §4.6 (Godot effects
   as draw calls; the ortho clipping), U/18 §2 and U/24 (count started tests), Godot/07 (global_position before add_child),
   2D/00, INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0. Engine: Realm 13 tests (new `.Vfx.Mapping`, `.Vfx.Canvas`). Draw of 320
  mice + 20 kittens with effects 0.36 ms (0.07 before). `py tools/padplay_tour.py`: PASS, 41.
- Sabotages all CAUGHT: 3 PawCore `flag_*` (560 in all), 7 realm `vfx_*` (372 in all).

### Open
- Not reproduced: Godot's antialiased line feather (FXAA smooths); the slam's crack shapes (seeded per position, not by
  Godot's hash(Vector2)); translucent blending in linear light. The effects are not compared side by side with Godot (they
  are transient: a tour would need both games to spawn the same effect on cue).
- From before: F38, F39, Q9, phase 5's hand-checks, F34-F37, Q10, the nightly `-Full` (~7.5 h now) not run since (33), the
  packaged bench not run since slice 1.

### Next step
*"Phase 6 slice 4: audio (doc 07 §6): AudioManager.EVENTS -> Data/AudioEvents.csv -> DT_AudioEvents, the OGG import (or
the WAV conversion), UPawAudioSubsystem::Play with Godot's rate limits and pitch jitter, the buses and the settings' sliders,
the music's crossfade, the prune test both ways."*

---

## 2026-11-05 (51) - Phase 6 slice 2: the backdrops (and the title's garden)

### Where the project is
- Phase 6: slices 1 (the field's art) and 2 (the backdrops) **done**; next slice 3, the VFX. Nothing committed after `98e3a4f`.

### What happened (designer: "Continue")
1. **The layout, exact** (PawCore `PcBackdrop`): biome_backdrop.gd's scatter, the title's trees, the patches and puddles,
   the border rows and the corners, in Godot's draw order. Godot lays them out with its own `RandomNumberGenerator` seeded by
   its own `hash()`, so the port carries both (`PcGodotRng`: PCG32, `randf` = two outputs in float32; `GodotHashString`,
   `GodotHashBiomeConfig`: djb2, the Dictionary's murmur3 + fmix32), and rounds to float32 exactly where Godot makes a Vector2.
   New fixture `backdrop` (`tools/godot_fixtures/backdrop_fixture.gd`): a probe subclass of the REAL node records every prop
   and outline its own `_draw()` makes, in a SubViewport of 7 sizes (4:3 to 32:9) at 4 effects budgets, 13 ids: 364 layouts,
   10,442 rectangles and 1,701 outlines, all equal to the float. 8 PawCore sabotages, all CAUGHT (one, the blob in double
   instead of float, first NOBUILD, retargeted). L0 `no-float` gets ONE named exception, `FLOAT_IS_THE_SPEC` = PcBackdrop
   (CLAUDE.md says so; nothing economic lives there).
2. **The realm draws it** (`APawRealmView::DrawBackdrop`): the floor (`M_Floor`, the 12 generator PNGs imported as
   `T_backdrop_tier_N`, tiled at 256 px from the top-left; all 12 referenced by the view: loaded with the map, never on
   travel), the patches, puddles and rims (one `UProceduralMeshComponent` section with sRGB vertex colours, `M_BackdropFill`:
   Godot's painter's order by primitive order; **ProceduralMeshComponent is now enabled for the game target**,
   `tools/trim_game_plugins.py`), the props (atlas sprites in a new `Props` band). Laid out on a travel or a resize with the
   budget of that moment, as Godot (a settings change shows from the next one).
3. **The atlas** gains the 36 props (107 sprites, 3 pages); `tools/import_art.py --run floors`; `tools/create_maps.py` sets the
   new materials and the floor list; `PcBackdropTones.h` generated from palette.gd's BACKDROPS by `tools/gen_paw_tokens.py`
   (L0 checks it). `build_art.py --check` also runs the generator's floor seam gate (seen to fail at a lowered limit).
4. **The title's garden**: `UPawGarden` (new piece, `WBP_Garden`), grafted into `WBP_MainMenu` as `Garden` right over the
   paper floor (`bake_widgets.py --graft "PawMainMenu=add:Garden"`), paints the "menu" biome in Slate: the tiled floor, the
   puddles (the Rooftops' sky), the props swaying about their roots; the scatter laid out on a resize, the patches and border
   rows each frame with the budget of now (Godot's dancing `_draw`).
5. **Seen beside Godot**: `py tools/godot_tour.py --set biomes` (new: `biome_probe.gd`, `paw.Travel <n>`): all 12 biomes and
   the title, same props, patches and puddles in the same places (build/tour/compare). Differences seen, owned elsewhere: the
   dock's tab icons and the cluster's icons (UI art, slice 5), the title's font (phase 7), Boba's photo (slice 6).
6. Docs 00, 03 (§7 the plugin), 04 (§5.1), 05 (the garden), 07, 09, 11, CLAUDE.md. KB: Godot/03 (a RandomNumberGenerator
   object's randf is not the global one's; hash() of String / Dictionary; float32 Vector2; recording a node's draws through a
   probe subclass), 2D/02 §4.5 (tiled floor, ordered translucent polygons, a backdrop painted in Slate), 2D/00, INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (after the L0 exception). L1: the backdrop fixture exact. Engine: Realm 11,
  UI 61 (new `Pawpocalypse.UI.Title.Garden`). `py tools/padplay_tour.py`: PASS, 41 checks.
- Sabotages all CAUGHT: 8 PawCore `backdrop_*` (557 in all), 5 realm `backdrop_*` and 2 `garden_*` (365 in all).

### Open
- The rims are strips with averaged normals and hard edges (FXAA smooths them); Godot's polylines are antialiased. Patches
  and puddles blend in linear light here, in sRGB in Godot (a touch lighter at their edges).
- From before: F38, Q9, phase 5's hand-checks, F34-F37, Q10, the nightly `-Full` not run since (33), the packaged bench not
  run since slice 1 (package only when asked).

### Next step
*"Phase 6 slice 3: the VFX (doc 11 phase 6): M_FxShape (aura ring, poop zone, damage zones, boss telegraph, swipe, slam, beam,
the Lone Hunter halo), the bursts / shards / edge fire / golden sparkle, each honouring the effects setting, against the
Godot vfx_*.gd code."*

---

## 2026-11-05 (50) - Phase 6 started: slice 1, the field's art

### Where the project is
- **Phase 6 (presentation) started**, in six slices (doc 11 phase 6): (1) the field's art, **done here**; (2) backdrops;
  (3) VFX; (4) audio; (5) UI art and UI motion; (6) ceremonies, Boba, the tutorial; then the exit tours. Phase 5's hand-checks
  still wait for the designer. Nothing committed after `98e3a4f`.

### What happened (designer: "Read the handoff. Do phase 6")
1. **The art generators moved in** (`tools/art/`): Godot's sweet-grammar generators copied unchanged but for their output
   paths (a PORT note in each); four hand-drawn UI SVGs as sources in `tools/art/hand/ui`; `flatten_backdrop.py` left out
   (it reads a removed style folder). `py tools/art/build_art.py` writes `SourceArt/svg` (390 files, Godot's `assets/art`
   layout); `--check` proves it **byte-identical to Godot's `assets/art`** and `palette.py --gd` identical to Godot's
   `palette.gd`. Seen to fail (one ink hex: 738 differences). The glyphs moved from `SourceArt/svg/input` to
   `SourceArt/svg/ui/input`; `raster_svg.py` reads `SourceArt/svg` now (no Godot copy), its PNGs unchanged.
2. **The field atlas**: `py tools/pack_atlas.py` rasterises the 71 field sprites at 2x their largest drawn size (sizes from
   Godot's code and data, in its docstring) into 2 pages of 2048 + `field.csv` (UVs, native size); `--check` reproduces it.
   `py tools/import_art.py --run atlas`: `T_Atlas_Field_0/1` (BC7, mips, Never Stream, World) and `DA_FieldAtlas`
   (`UPawSpriteAtlas`, new). `tools/create_field_materials.py` adds `M_Sprite` (masked) and `M_SpriteSoft` (translucent):
   UV rect from custom data, tint, whiten. `tools/create_maps.py` sets them and the atlas on L_Realm's view.
3. **The renderer** (`UPawSpriteRenderer::AddSprite`): per atlas page a masked, a soft and a shadow component; the logical
   layers (Floor, Zones, Shadows, Bodies, new Overlays, Fx) kept, so the tests and the bench still count what is drawn.
   Translucent bands ordered by sort priority. Mirroring by swapping the cell's U0 / U1.
4. **The creatures as Godot draws them** (`APawRealmView::DrawCreatures`, constants and maths in `Realm/PawFieldLook.h`,
   every one from Godot's code): mice at `sprite_target_px` with the breathing pulse (vertical only), facing by travel with
   the 8 px/s dead zone, data tint x shield x snare (multiplied, as Godot), the signature glow (a white silhouette 1.24x,
   breathing, behind); kittens at 96 px, gold / illusory tints; the cat in its VISUAL stage (the unicorn mount follows the
   node), its weapon (mirrored) and job hat as children of the sprite, the Pounce hop (lift, crouch, stretch), facing by the
   stick only; the boss at its SVG size x 0.9 with its twitch (anger after a heal), shield tint and the death sequence (six
   shudders, the back-eased collapse); the adds' pulse; the ground shadow under every creature that stands (not the boss,
   not the adds: Godot has none there). **Draw order is Godot's, not a Y-sort**: doc 04 §4 assumed Godot sorted creatures by
   Y; it draws by z_index then tree order (mice < kittens < cat < weapon / adds < hat). Doc 04 corrected.
5. Dev tour steps `paw.Cat <x> <y>` (place the cat; the stick claims the cursor) and `paw.Boss` (the first boss realm if
   needed, bait topped up, a real summon). Screenshots of the field, the stage 3 cat with its weapon, the stage 6 unicorn with
   its flag, and the Park Tyrant with kittens over it: all as expected (not yet compared side by side with Godot's).
6. **Finding F38** (doc 01 §5 and the Godot findings file, uncommitted there): a cat steered right by the stick flips its
   picture but its hat and weapon stay on the left (Godot's `flip_h` does not move children). Read in the code; the port copies it.
7. Docs 00, 01 (F38), 04 (§4 the draw order, §5.1 built, §9), 07 (§2 built, §2.1, §3), 09 (two gates, the tests, 358
   sabotages), 11 (phase 6 slices), CLAUDE.md (the tool commands). KB: 2D/02 §4.4 (atlas sprites on instanced quads: UV rect
   and flip in custom data, sort priority bands, a source game's draw order, Godot modulate in linear, DataAssetFactory from
   Python, test the import's freshness), 2D/00, INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0, with two new gates (`art generators reproduce`, `field atlas reproduces`).
  Engine: `Pawpocalypse.Realm` 11 tests (new `.Look.Rules`, `.Look.Atlas`). Draw of 320 mice + 20 kittens 0.068 ms.
- `py tools/padplay_tour.py`: RESULT PASS, 41 checks.
- Sabotages, all CAUGHT: 7 new (`look_kittens_under_mice`, `look_enemy_without_art`, `look_cat_wears_the_kitten`,
  `look_shadow_wrong_art`, `look_kitten_half_size`, `look_atlas_no_stems`, `look_hop_lower`) and 2 stale ones retargeted
  (`view_no_shadows`, `render_counts_the_pool`); 358 realm sabotages in all. The atlas freshness check seen to fail on one
  changed UV in `field.csv`.

### Open
- **Not in slice 1, owned by later slices**: the Lone Hunter halo (drawn as the zones' soft discs, under the shadows, until
  the VFX slice's shape material), the golden sparkle (VFX), the effects (still placeholder rings), the floor (still flat
  paper). Per-creature `UPawVisualDefinition` assets (doc 07 §4) not built: the look reads the CSV columns and Godot's code
  constants; build them when a slice needs editable visuals.
- Known small difference: Godot blends translucent sprites in sRGB space, Unreal in linear (doc 04 §5.1).
- The side-by-side against Godot's field (the exit tour) waits for the backdrops. The packaged bench now also fails if the
  sprites did not cook; it has not run since this slice (package only when asked).
- F38 (new), Q9 (FXAA vs no AA: the masked outlines show their stair-step on movers), and from before: phase 5's hand-checks,
  F34-F37, Q10, the nightly `-Full` not run since (33).

### Next step
*"Phase 6 slice 2: the backdrops. Port biome_backdrop.gd (the 12 tiling floors from SourceArt/svg/backdrops, the props laid
out as Godot lays them, in the atlas or their own), tiled to any aspect up to 32:9 with a seam test; then the screenshot of
each biome against Godot's."*

---

## 2026-11-05 (49) - Phase 5 slice 5: keys by position, the dock's held keys, the paw, the pad-only playthrough

### Where the project is
- **Phase 5's work is done** (slices 1-5). Its exit criteria are met but the hand-checks the designer has to make: a real Xbox
  pad and a PlayStation pad through Steam Input, the paw cursor in a packaged build, a real AZERTY keyboard. Next: phase 6
  (presentation). Nothing committed after `98e3a4f`.

### What happened (designer: "Do slice 5")
1. **Keys by POSITION, on any keyboard layout (a port bug, found).** Doc 05 assumed Windows `FKey`s are positions; the engine
   source says otherwise (`FWindowsApplication`: the layout's virtual key), and THIS machine proved it: the designer's Turkish
   Q layout puts US `-`'s place on `EKeys::Asterix` and `=`'s on `EKeys::Hyphen`, so the port's zoom keys sat one key off
   Godot's. `Input/PawKeyCodes`: every key with a place (letters, digits, punctuation) maps through the ACTIVE layout (US scan
   code -> `MapVirtualKeyEx` -> the engine's own `GetKeyFromCodes`; back by the character for letters and digits, because the
   engine's own VK map is built from the system layout: on Turkish Q it gives `EKeys::I` the key code `VK_OEM_7`). The game
   logs the keys at startup ("on this keyboard, up is W Up, ..., zoom in Hyphen, zoom out Asterix" here). Automated runs pin US
   English (`-PawUsKeyboard`, run_automation.py). `Pawpocalypse.Input.Azerty` injects French AZERTY: ZQSD, the prompt "Z", a
   capture of Z is W's place.
2. **The dock's held keys** (dock_navigator.gd `_tick_nav` / `_tick_press`): PawCore `PcDockRepeat` (in `PcHud`), exact against
   the REAL navigator in the real HUD (new fixture `dock_repeat`: 600 sequences x 40 frames, a quarter starting with nothing
   selected). The HUD polls what is held (`UPawInputDeviceSubsystem::IsActionHeld` / `TakeActionDown`, Godot's
   `Input.is_action_pressed` / `just_pressed`): rows step at once, then after 0.35 s at 10 per second; a press presses at once,
   then on a shop row only repeats (0.35 s, 4 rising to 12 per second): hold-to-buy on Space, Enter, pad A and LT. The port's
   old hold-to-buy (any button held, the mouse too: not in Godot) is gone. The dock TAKES its keys first: the device
   subsystem's input processor is now registered FIRST and consumes press / up / down while the dock is active
   (`ConsumeForDock`), because Common UI's analog cursor turned pad A into a simulated click before any widget (one A bought
   twice). Its tab keys act on their own press; echoes swallowed.
3. **The paw cursor**: Godot's two SVGs rasterised at 40 px into `Content/Pawpocalypse/UI/Cursors` (`raster_svg.py --group
   cursor`; staged loose in DefaultGame.ini), hardware cursors for the arrow and the pointing hand, closed while the left button
   is held, both loaded at startup. Seen loading in a `-game` run.
4. **Pads in the background**: ignored (project.godot `ignore_joypad_on_unfocused_application`; Slate's default, set by the
   game); held keys forgotten when focus goes.
5. **The pad-only playthrough and the focus tours** (Godot ci/check_padplay.gd): `paw.PadPlay` (Dev/PawPadPlay.cpp) and
   `py tools/padplay_tour.py` (a real `-game` window, a fresh sandboxed save folder `build/padplay/saves`): pad buttons and a
   trigger through `FSlateApplication`, title -> new game -> shop (press, LT held, X) -> a card -> travel -> Bait -> Reawaken
   -> the tree -> pause -> Settings -> save and quit -> Continue; the focus tour at six screens. 41 checks, all PASS. Found by
   it: the double A (item 2), and the pause menu's return to Settings lost (Common UI re-applies the desired focus target when
   a modal closes: `UPawPauseMenu::ReturnFocus`). In the nightly `-Full`; `sabotage_realm.py` can run it as a test (`PADPLAY`).
6. **Not ported: the unfocused frame cap** the plan listed: Godot has none, so it is a design change (doc 01 Q10, after
   parity).
7. Docs 00, 01 (Q10, D7), 05 (§3 slices 5a / 5b, §4 the paw), 09, 10 (the cursor folder staged), 11, 12 (four pitfalls),
   CLAUDE.md. KB: U/08 §5.2 (FKey is the layout's; the engine's VK map is the system's; Common UI's simulated click and a
   first-registered processor; focus re-applied after a modal; pads in the background; hardware cursors from PNG; a pad-only
   playthrough through Slate), U/24 (three rows), INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0. L1: 182 cases, 6.42 M assertions. Engine: all 98 tests (`Input` 17).
- `py tools/padplay_tour.py`: RESULT PASS, 41 checks. Fixture `dock_repeat` reproduces byte for byte.
- Sabotages, all CAUGHT: 5 PawCore `dock_*` (549 in all; two first NOBUILD / MISSED and fixed: an unused parameter, and the
  no-selection path the fixture did not reach until it started some sequences with the focus on a tab); 7 game-module (351 in
  all): `keys_letters_by_name`, `paw_never_closes`, `held_stand_in_counts`, `held_axis_ignored`, and against the pad-only tour
  `dock_consumes_nothing`, `dock_held_press_ignored`, `pause_forgets_settings`.

### Open
- The designer's hand-checks (phase 5 exit): a real Xbox pad and a PlayStation pad through Steam Input; the paw cursor in a
  packaged build (package only when asked); a real AZERTY keyboard (only injected here).
- Q10 (the unfocused frame cap). A keyboard layout switched while the game runs is read at the next apply (a remap or a restart).
- From before: F34-F37; the `BOSS__name` warning; the version line; the nightly `-Full` (~7.1 h now) not run since (33).

### Next step
*"Phase 6, presentation (docs/11): start with its first slice. Before that, consider the nightly `-Full`: it has not run since
(33) and now includes the pad-only playthrough."*

---

## 2026-11-05 (48) - Phase 5 slice 4c: the Controls page; slice 4 done

### Where the project is
- Phase 5, slices 1-4 done. Next: slice 5 (the focus tours, the pad-only playthrough tour, the paw cursors, the unfocused
  throttle, the AZERTY check). Nothing committed after `98e3a4f`.

### What happened (designer: "Do next step", after (47))
1. **A Godot fixture `controls`** (tools/godot_fixtures/controls_fixture.gd): the REAL ControlsPanel in the real settings dialog,
   1,500 rows from a recorded start x 12 moves (a cell pressed, events while it listens: keys, echoes, releases, clicks, the
   wheel, pad buttons, sticks, triggers; the clock incl. its exact 5 s edge; Reset; Reset all; the switches; the sliders; the
   prompt style; a direct bind), the page after each. It first crashed Godot (signal 11): every redraw queue_free()s the old
   prompts and no frame ran to free them; a frame now passes per row (KB G/03).
2. **PawCore `PcControls`**: the status line, listening (cancel, swallow, bind, the timeout), Reset / Reset all, the switches,
   the sliders (Godot's Range snaps FROM ITS MINIMUM, then clamps; the dead zone's bounds are float32: measured on 1,112 moves,
   KB G/03), the controller column's family. L1: 2 cases (every move exact) + the long suite on all 1,500 rows.
3. **`UPawControlsPanel`** (UI/PawControlsPanel; `WBP_ControlsPanel` baked; grafted into `WBP_SettingsDialog`'s Controls page,
   backup before: `build/widgets_backup_47c/`): the switches, the binding table built at run time (a cell is a button with the
   binding drawn over it, `UPawPromptIcon` always shown), the status line, Reset all, the stick switches, the two sliders, in a
   480 px scroll box (Godot's fills what the page leaves: the asset owns the height; ~70 px spare in a 1080p shot). The dialog
   hands it every key, release, click, wheel turn and analog value while a cell listens; its selection rows include the table.
   ST_UI: 33 `CONTROLS_*` words (Godot's).
4. **Two things found on the way**: (a) a prompt's binding is the FIRST of the action's Godot events for the device; when that
   one has no Unreal key (a pad's Guide button) Godot draws nothing: `PawGlyphs::BindingFor` now reads the live Godot map
   (`PawInput::LiveState`) instead of the context's keys, which skipped it and drew the next one. (b) **F37** (doc 01 §5 and
   the Godot findings file): with Button prompts on Keyboard, a pad picked up does not redraw the page, so the controller
   column can show the old pad; low priority; the port copies Godot (the panel redraws only on Godot's signals).
5. Dev: `paw.Settings [audio|video|controls]` (Dev/PawShot.cpp). Seen in a `-game` run at 1920x1080: the page, the table with
   keycaps and Xbox glyphs, the D-pad walking to a controller cell, pad A listening ("Press..." on the focused cell).
6. Docs 00, 01 (F37), 03, 05 (slice 4c), 09 §7, 11, 12 (a `<=` edge never hit by random values); appendix B; CLAUDE.md (the
   nightly ~6.9 h). KB: G/03 (Range from its minimum; node pile-up in a UI fixture), U/08 §4 (a game's own bindings on a
   runtime context; capturing a key in a Common UI dialog), INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0. Its first run FAILED L0: the `CONTROLS_` string keys were not a known prefix
  and the row names were built by concatenation (now listed literally).
- L1: 181 cases, 6.23 M assertions; the long suite's Controls case on all 1,500 rows (2.1 M assertions). Fixtures `controls`
  and `input_bindings` reproduce byte for byte.
- `Pawpocalypse.UI.*` 61 (+ `.Controls.MatchesGodot`: 200 fixture rows on the shipped widgets with Slate's events through the
  dialog, zero differences; `.Controls.Dialog`; `.Settings.Look` now counts the table's 39 controls); `Pawpocalypse.Input.*` 16.
- Sabotages: 12 PawCore `controls_*` (544 in all) and 9 game-module `controls_*` (344 in all), all CAUGHT; `controls_timeout_strict`
  was first MISSED (the clock never landed on 5 s exactly: 5.0 added to the fixture's ticks, 18 edge hits asserted), and one aimed
  at a test hook was dropped. `glyph_any_device_binding` went STALE with the BindingFor change: retargeted, CAUGHT.

### Open
- Not yet seen on hardware or in a `-game` run: a remapped pad button driving the field (slice 5's tours); the scroll box's
  height is the asset's to adjust (480 px leaves ~70 px).
- F37 (low priority); from before: hold-to-repeat on the left trigger; the `BOSS__name` warning; F34, F35, F36; AZERTY; the
  version line; the nightly `-Full` (~6.9 h now) not run since (33).

### Next step
*"Phase 5 slice 5: the focus tours (every screen walked with real Slate key and pad events, focus never lost), the pad-only
playthrough tour (the whole loop with a pad alone), the paw cursors (open / closed), the unfocused frame cap, the AZERTY check
(whether Windows FKeys are positions). Consider running the nightly -Full first: it has not run since (33)."*

---

## 2026-11-05 (47) - Phase 5 slice 4a/4b: remapping, the rules and the map

### Where the project is
- Phase 5, slices 1, 2, 3, 3b done; slice 4 split in three: **4a (the rules) and 4b (the map) done**, 4c (the Controls page)
  next. Nothing committed after `98e3a4f`.

### What happened (designer: "Go, start slice 4")
1. **Settled: remapping is Godot's own `[bindings]` in settings.cfg**, not Enhanced Input's user settings (docs/05 §1, docs/08
   §3): a logical row writes several actions and a swap touches two rows, which per-key "player mappable" slots cannot hold;
   Godot is the specification, and a Godot player's settings.cfg reads the same.
2. **4a, the rules: PawCore `PcBindings`** (input_bindings.gd): the ten rows and their member actions, rebind (swap, refusals:
   system keys, locked rows, taken keys, unchanged), reset_row (and the rows it then shares a key with), the saved diff,
   working_state, apply() with Swap sticks / Swap confirm and back (and InputMap's own de-duplication, measured), the steer dead
   zone, binding_from(). `PcSettings` reads and writes `[bindings]` as Godot's ConfigFile does (lists of flat dictionaries,
   keys written sorted, a non-list dropped on load).
3. **A new Godot fixture `input_bindings`** (tools/godot_fixtures/input_bindings_fixture.gd): 3,000 rows x 8 random ops (a
   quarter starting from a hand-edited state with a row bare on one device), apply() with random switches, 3,000 raw events
   through binding_from(), 1,500 settings.cfg files written and read by Godot (hand edits: floats, strings, missing fields,
   unknown types, non-lists), the defaults in InputMap order. L1: 5 new test cases (`Bindings: *`) + the long suite on all
   3,000 rows; exact.
4. **4b, the map** (Source/Pawpocalypse/Input): `PawKeyCodes` (Godot events <-> FKeys: Godot's one Shift/Ctrl/Alt/Meta is
   both of Unreal's sides, keypad Enter is Enter, the Y axes flip; a Slate key captured as Godot's raw event);
   `PawInputDefaults.h` generated from Data/InputMap.csv by `tools/gen_input_defaults.py` (the CSV is not cooked; L0 checks it);
   `PawInput::LiveContext()` / `ApplyBindings()` / `LiveState()` / `OnBindingsChanged()`: ONE transient mapping context the
   player controller adds and every key question reads, rebuilt in place. `UPawInputDeviceSubsystem` applies when the
   bindings, a swap or the dead zone change, asks each local player's Enhanced Input to rebuild, then fires
   `OnControlsChanged` (the prompts redraw on it, not on the settings); the game instance installs a new navigation config
   on `OnBindingsChanged`. `UPawSettingsSubsystem`: SetBindings, SetSwapConfirmBack, SetSwapSticks, SetInvertX/Y,
   SetCursorSpeed, SetStickDeadzone, SetTutorialTips, ResetControls (set_control's early return on an equal value); Reload
   broadcasts.
5. Engine tests `Pawpocalypse.Input.RemapKeys`, `.RemapDefaults`, `.Remap` (Input: 16 tests).
6. Docs 00, 03, 05 §1/§3/§5, 08 §3, 09 §7, 11, 12 (two pitfalls), CLAUDE.md (the generator), appendix B (input_bindings.gd's
   destination; tools/make_port_map.py). KB: G/03 (ConfigFile's sorted Dictionary keys, InputMap's de-duplication and action
   order), G/00-INDEX, INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0. Its first run FAILED L0: `PcBindings` used `float` (the steer dead zone's
  float32 rounding, now done by arithmetic, `frexp` / `nearbyint` / `ldexp`) and `<deque>` (not on the allowed list; a vector
  queue now); the Bindings tests stayed exact after both.
- L1: 179 cases, 5.52 M assertions (+ 5 `Bindings: *`); the long suite's new case on all 3,000 binding rows (168 k assertions).
- `Pawpocalypse.Input.*` 16 tests (+ `.RemapKeys`, `.RemapDefaults`, `.Remap`); `Pawpocalypse.UI.*`, `.Save`, `.Realm`, `.Data`
  unchanged and green on the live context.
- Sabotages: 23 PawCore (`bind_*`, `settings_bindings_*`; 532 in all), all CAUGHT (two first MISSED / NOBUILD and fixed: the
  fixture now starts rows from a hand-edited state, pitfall in doc 12; `bind_deadzone_double` added with the L0 fix); 12
  game-module (`remap_*`; 335 in all), all CAUGHT, each with UBT, and the clean build passing.

### Open
- 4c, the Controls page. Not yet seen in a `-game` run: a remap reaching Enhanced Input's actions on the field (the tests read
  the live context and Slate's key path; the field's IA_Pounce after a pad rebind is to be seen in 4c's tour).
- From before: hold-to-repeat on the left trigger; the `BOSS__name` warning; F34, F35, F36; AZERTY; the version line; the
  nightly `-Full` (~6.7 h now) not run since (33).

### Next step
*"Phase 5 slice 4c: the Controls page (UPawControlsPanel, controls_panel.gd): Show button prompts and Tutorial tips, the
binding table (a row per logical action, a keyboard and a controller cell drawn as prompts, Reset), listening (Esc / B cancel,
5 s timeout, a click cancels a controller cell), the status line, Reset all controls, Swap confirm and back, Swap sticks,
Invert X / Y, the cursor speed and dead zone sliders; pad_column_family; grafted into WBP_SettingsDialog's Controls page;
proven against the real panel (extend fixture `input_bindings` or a new `controls`)."*

---

## 2026-11-05 (46) - The docs brought up to date after phase 5 slice 3b

### Where the project is
- Unchanged from (45): phase 5 slices 1, 2, 3 and 3b done. Next: slice 4 (remapping and the Controls page). Nothing committed
  after `98e3a4f`.

### What happened (designer: "make sure all docs are up to date")
1. Every name the living docs (00-13, CLAUDE.md) put in backticks checked against the project (files, classes, assets,
   tests): what remains unmatched is labelled as the plan or as a later phase's.
2. **Doc 00**: the status date, the PawCore sabotage count (509), the nightly (~6.5 h now, not run since (33)), and the table of
   what exists: `PcHud`, `PcPause`, `PcSettings`, `PcDevice`; `Input/`; the toast, the prompts and the Widget Blueprint
   contract; the editor module (it was "empty"); `UI/Widgets`, `Input/`, `Art/UI/Input`; the new tools.
3. **Doc 03**: PawCore modules since 2026-10-29; the autoload table as built (`UPawSettingsSubsystem` over `PcSettings`, not
   `UGameUserSettings`; `UPawInputDeviceSubsystem` a GameInstance subsystem over `PcDevice`; the screens by class; the token
   generator; what is planned and not built).
4. **Doc 05**: §1 as built (no `BP_PawUIInputData`, no controller data assets); the action table's context column is the
   plan (one `IMC_Pawpocalypse` as built); three "until phase 5" lines that phase 5 has done. **Doc 01** D6 and D7's status.
   **Doc 07** and **04**: the token generator and the field's event queue as built. **Doc 08**: the last slot, which settings
   are read, `[bindings]` left to slice 4.
5. **Doc 09 §7**: phases 1-5; L1 174 cases / 5.50 M (long 31 / 5.50 M); 509 PawCore and 323 realm sabotages; new rows for
   phase 4's UI tests (`.Title`, `.Pause`, `.Settings`, `.Widgets`), `Pawpocalypse.Input` (13), "art rasters reproduce" and the
   two UBT builds; `.Save` is 9 tests (`.MidFight`). **Doc 12**: three pitfalls (stale sabotages, editor-only API in the game
   target, a re-bake over a designer's asset). **Doc 06** and doc 00: the Godot code constants are 580 now (585 map rows,
   `save` 25: the save fixes added 11). **Doc 11**: "Next: slice 4". CLAUDE.md: the nightly ~6.5 h.
6. Appendices regenerated to scratch: A and C identical; **B** differed only in line counts (the designer's uncommitted Godot
   fixes: 29,577 lines now) and was replaced.
7. **Found on the way: one PawCore sabotage STALE** (`hud_row_enabled_short`: the shop's enabled line gained the currency
   epsilon). Retargeted, seen CAUGHT with `--only`, the clean build passing. `--list` of both sabotage tools: 0 STALE.
8. `Config/DefaultGame.ini`'s comment that promised `BP_PawUIInputData` for phase 5 now says it is not used.

### Verified
- `py tools/check_pawcore_static.py`: every gate PASS (the docs' generated parts and the data export current). The Godot
  repo's status unchanged by the appendix runs. No engine gate was needed: no source changed but `tools/sabotage_pawcore.py`.

### Open
- Unchanged from (45). The nightly `-Full` (now ~6.5 h) has not run since (33).

### Next step
*"Phase 5 slice 4: remapping (input_bindings.gd's rules in PawCore against the real script, saved bindings applied to the
mapping context, prompts redrawn on a remap) and the Controls page (bindings per device, Invert X / Y, Swap sticks, the stick
dead zone, Show button prompts)."*

---

## 2026-11-04 (45) - Phase 5 slice 3b: the glyphs and button prompts

### Where the project is
- Phase 5, slices 1, 2, 3 and 3b done. Every prompt Godot draws outside the tutorial and the Controls page is drawn here,
  for the device in hand. Next: slice 4 (remapping and the Controls page). Nothing committed after `98e3a4f`.

### What happened (designer: "Go", after (44))
1. **The art**: `tools/raster_svg.py` copies Godot's 65 generated glyph SVGs to `SourceArt/svg/input/` and rasterises them at
   56 px (2x the largest prompt) into `SourceArt/png/input/` (`--check` reproduces them byte for byte: a new everyday gate,
   "art rasters reproduce", seen to fail on one changed pixel). `tools/import_art.py --run` imports them headless as
   `T_input_*` under `/Game/Pawpocalypse/Art/UI/Input` (UserInterface2D, sRGB, no mips, Never Stream, UI group).
2. **The rules**: `UI/PawGlyphs` (input_glyphs.gd) against a new Godot fixture `input_glyphs`: every action x family (160),
   every bound key's label, the {token} replacement of the game's templates (50). Exact.
3. **The widgets**: `UPawPromptIcon` (`WBP_PromptIcon`: a glyph or a drawn keycap) and `UPawPromptText` (`WBP_PromptText`:
   rich text with a C++ decorator, `UPawPromptDecorator`), redrawn on a device change and on any settings change.
   `PawText::PromptArgs` and the `PROMPT_*` placeholder words are gone; ST_UI gained `HUD_legend_*` and `KEY_*`.
4. **Placed where Godot has them**: the HUD's tab keys, dock legend and Pounce key; the title's hint; the tree's legend and every
   tooltip line; the press prompt on the focused button (one prompt owned by `UPawInputDeviceSubsystem`: inside the button's
   right edge, or in the dock outside the row on the field side).
5. **Grafting** (`UPawWidgetBaker::GraftParts`, `py tools/bake_widgets.py --run --graft "Class=add:Part;swap:Old>New"`): the
   new parts went INTO the existing `WBP_HudScreen`, `WBP_MainMenu`, `WBP_SkillTreeScreen` (4 added; the hint, the tree's legend
   and its 7 tooltip lines swapped in their old slots); nothing else in them touched. Backup before:
   `build/widgets_backup_44b/`.
6. Dev commands `paw.Press <key> [device]` (a key through Slate) and `paw.Prompts <style>` (Dev/PawShot.cpp). Seen in `-game`
   runs: keycaps on the keys; Xbox glyphs after one pad press; PlayStation glyphs from the setting; the HUD, pause menu, tree,
   title.
7. `UPawSettingsSubsystem::SetShowPrompts` (Settings > Controls > Show button prompts; its control is slice 4).
8. Docs 05 §5 (as built), 07 §2, 11, 00, CLAUDE.md. KB U/09 §2.3 (grafting) and §2.4 (a rich text decorator), U/17 §3
   (script arguments), U/18 §2.1 (hold `TakeWidget`; texture sizes under -nullrhi), U/24 (two rows), INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (now with "art rasters reproduce"). Its first run FAILED the game-target build:
  the new texture test read editor-only data (`Source`, `MipGenSettings`); now under `WITH_EDITORONLY_DATA`.
- `Pawpocalypse.Input.*` 13 tests (new: `.Glyphs`, `.Prompts`); `Pawpocalypse.UI.*` 59 (the parity test on the grafted
  assets; the title's hint and the tree's tooltip as prompts).
- Sabotages, 12 new, all CAUGHT: `glyph_deck_draws_xbox`, `glyph_any_device_binding`, `glyph_escape_named_tab`,
  `glyph_printable_by_name`, `glyph_placeholders_eaten`, `glyph_markup_unescaped`, `prompt_icon_deaf`,
  `prompt_line_ignores_setting`, `prompt_text_fixed_family`, `prompt_focus_dock_side_swapped`, `prompt_hud_one_legend`,
  `prompt_title_hint_words`. `tree_blurb_tokens_raw` removed (the blurb is a template now; `prompt_text_fixed_family` covers it).
- **Found on the way: 9 game-module sabotages were STALE** (their target text gone since phase 4's settings and the bake: the
  nightly `-Full` would have failed on them). 7 retargeted and seen CAUGHT (`hud_dock_not_reported`, `hud_dock_side_ignored`,
  `hud_root_takes_clicks`, `hud_reawaken_one_press`, `pick_never_opens`, `save_automation_unsandboxed`,
  `title_unreadable_has_delete`); 2 RETIRED (`victory_white_text`, `jobpick_white_note`: the colour is the asset's since the
  bake, no source line reaches it; their tests still read the colour shown). All 323 now name code that exists.

### Open
- The tutorial's prompts (phase 6), the Controls page's binding cells and a redraw on a remap (slice 4), on a language change
  (phase 7). "Achievements" touches the RB/D cap in the tab strip until the tab icons (phase 6).
- From before: hold-to-repeat on the left trigger; Invert / Swap / dead-zone setters (slice 4); the `BOSS__name` warning; F34,
  F35, F36; AZERTY; the version line; the nightly `-Full`.

### Next step
*"Phase 5 slice 4: remapping (input_bindings.gd's rules in PawCore against the real script, saved bindings applied to the
mapping context, prompts redrawn on a remap) and the Controls page (bindings per device, Invert X / Y, Swap sticks, the stick
dead zone, Show button prompts)."*

---

## 2026-11-04 (44) - Phase 5 slice 3: devices, the stick-steered cat, the toast; glyphs split to slice 3b

### Where the project is
- Phase 5, slices 1-3 done, **except the glyphs**, split into slice 3b (they need Godot's per-family SVG art through the art
  pipeline, a job of its own). Nothing committed after `98e3a4f`.

### What happened (designer: "Do slice 3")
1. **The device rules** (`input_device.gd` `observe()`) in PawCore: `PcDevice` (prompt family, cursor source, the connect and
   disconnect toasts, the warp echo, the boot quiet time, the modal guard, the prompt-style setting, pad names -> family).
   Ground truth: a new Godot fixture `input_device` (three tables: events -> state, names -> family, stick -> velocity). The
   fixture is real time, so it records the clock it saw (`after` column) and `--check` re-runs it without comparing bytes.
2. **`UPawInputDeviceSubsystem`**: an `IInputProcessor` feeds every key, axis and mouse event to `PcDevice`; XInput pads are
   the Xbox family. The OS pointer hides when the stick or pad takes over (`CurrentMouseCursor`), and the toast shows.
3. **The toast**: `UPawToast` (`WBP_Toast`, in `BakedClasses`), at the top edge above every screen, 2.4 s then a 0.4 s fade.
   Strings `INPUT_toast_connected` / `_disconnected` in ST_UI; the toast picks them (`ShowConnection`), so the L0 lint sees them.
4. **The stick-steered cat** (`player.gd` `_process`): `PawInput::StickVelocity` (Godot's curve, float math as Godot),
   `APawRealmView::StickStep` (clamped to the playfield, far edges in), the pointer warped to the cat (`SetMouseLocation` +
   the warp echo), `recentre` on R3's rising edge, `PointerOverField` (`Rect2.has_point`, never over the dock).
5. **The stick dead-zone setting**: `PawInput::SetSteerDeadZone`, clamped 0.05 .. 0.9, on the steer actions only.
6. Tests: `Pawpocalypse.Input.Velocity`, `.Device`, `.BootQuiet`, `.PointerOverField`; 11 Input tests in all. L1: 174.
7. Docs 05 §4 (slice 3 as built), 06, 08 §3, 10, 11, 00. KB G/03 §6, G/07, U/08 §3.4 / §4 / §5, U/09, U/18 §7, U/24, INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0.
- PawCore device sabotages: 12/12 CAUGHT. Game-module slice 3 sabotages: 9/9 CAUGHT (`input_pointer_field_dock` was MISSED
  first: the test had no panel inside the playfield; one added. `input_device_toast_swapped` re-run CAUGHT after the strings
  moved into the toast).
- A `-game` run logs all three input lines (the IMC, the navigation config, the device subsystem).

### Open
- **Slice 3b, the glyphs**: per-family prompt icons and labels following `OnFamilyChanged`; keycaps drawn at run time.
- Hold-to-repeat for `press` on the left trigger. Invert X / Y, swap sticks and the dead-zone setters in the Settings
  subsystem (slice 4).
- `ST_Data 'BOSS__name'` warning (pre-existing); F34, F35, F36; AZERTY; the version line; the nightly `-Full`.

### Next step
*"Phase 5 slice 3b: the glyphs (Godot's per-family SVG art through the art pipeline, keycaps drawn at run time, prompt
icons and labels following OnFamilyChanged)."*

---

## 2026-11-04 (43) - Phase 5 slice 2: the field and every screen read Godot's input map

### Where the project is
- Phase 5, slices 1 and 2 done. No screen or field code reads a raw key any more (only the tree's mouse drag reads the mouse
  button, which in Godot is not an action either). Next: slice 3. Nothing committed after `98e3a4f`.

### What happened (designer: "Do slice 2")
1. **What a key IS** (`Input/PawInputMap`), the port's `event.is_action(...)`:
   - `PawInput::KeyIs`, `ActionsOfKey` and `KeysOf` are built from the live mapping context. A stick or trigger pushed past
     Slate's threshold arrives as a digital key (`Gamepad_RightTrigger`) and counts as its axis half.
   - Godot's `get_vector` and `get_action_strength` (`GodotVector`, `SteerVector`, `Strength`) are measured with a new table
     in the `input_axis` fixture: 3,008 `get_vector` deflections.
2. **The screens**, each through its own Slate key handler, ask the map:
   - the HUD: tabs (the D-pad's left / right now step tabs, as in Godot), `tab_1..6`, `buy_amount`, `open_upgrades` (U jumps
     to Upgrades: new), `open_skill_tree`, `pause`, and `press` on the left trigger (the selected dock button);
   - the pause menu (`back`, `pause`), the settings (`back`, sections), the title (`back`);
   - the skill tree: close keys, `buy_amount`, the zoom by trigger STRENGTH as Godot (a half pull zooms at half speed; the
     raw-key version zoomed at full speed), the pan from the steer stick through `get_vector` (dead zone removed, rescaled;
     it was a raw stick past 0.2).
3. **The menus navigate on Godot's `ui_*` keys**: `FPawNavigationConfig`, installed by the game instance (never the editor).
   W / A / S / D now move the selection in menus and keypad Enter accepts, as in Godot. Common UI's input data is not needed.
4. **The field**: the controller adds `IMC_Pawpocalypse` to its local player, and the realm view pounces on `IA_Pounce`; a
   pointer press over the dock is the dock's (`PounceHeld`). Seen in a `-game` run: both log lines ("IMC_Pawpocalypse added
   (71 mappings)", "the menus navigate on Godot's ui_* keys (12 keys, 5 accept / back)").
5. **Tests**: `Pawpocalypse.Input.Keys`, `.Navigation`, `.Vector`, `.Screens` (every key bound to each screen's actions sent
   through the screen's Slate widget, and unbound keys doing nothing), `.Pounce`; 7 Input tests in all. HUD test views:
   `IsTabShown`, `PressSelected`; tree: `GetSteer`, `GetZoomHeld`.
6. Docs 05 §3 (slice 2 as built), 11, 00. KB U/08 §3.4 (Slate navigation keys are not Enhanced Input's; asking which action
   a key is; key tests without a viewport), INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (the game target builds).
- Sabotages for slice 2, all 13 CAUGHT: `input_keys_no_standins`, `input_nav_standins_navigate`,
  `input_vector_square_deadzone`, `input_hud_tab_next_lost`, `input_hud_upgrades_elsewhere`, `input_hud_pause_on_back`,
  `input_pause_back_is_pause`, `input_settings_prev_goes_next`, `input_tree_zoom_sticks` (retargeted when the zoom moved to
  strength), `input_tree_zoom_trigger_full`, `input_tree_pan_raw`, `input_title_back_lost`, `input_pounce_dock_ignored`.

### Open
- **Not covered yet**:
  - hold-to-repeat for a `press` held on the left trigger (Space / Enter / A repeat through the buttons);
  - Invert X / Y and the stick dead-zone setting (slices 3-4);
  - the stick-steered cat and `recentre` (slice 3).
- **A pre-existing warning**, seen in every `-game` log since 2026-11-02: `ST_Data 'BOSS__name'`, a boss name asked for with an
  empty id. Not from this work; to trace.
- From before: F34, F35, F36; AZERTY; the version line; the nightly `-Full`.

### Next step
*"Phase 5 slice 3: the device subsystem (prompt family and cursor source on first input, the connect toast, the warp echo),
the stick-steered cat with recentre (Godot player.gd: stick_velocity, the curve, the dock bounds), the stick dead-zone setting
applied to the steer actions, and the glyphs."*

---

## 2026-11-04 (42) - Phase 5 started: slice 1, Godot's input map as data and Enhanced Input assets

### Where the project is
- Phase 5 (pad and keyboard parity) has started, in five slices (doc 11 phase 5); slice 1 is done. Nothing reads the new
  actions yet: the game still reads raw keys until slice 2. Nothing committed after `98e3a4f`.

### What happened (designer: "start on phase 5")
1. **`Data/InputMap.csv`** (`tools/export_input_map.py`, run by `export_godot_data.py`, so L0's data-export gate keeps it
   current; seen to FAIL on a hand edit): Godot's `project.godot [input]`, 32 actions and 72 events, one row per event with
   Godot's code beside the Unreal key. The export:
   - binds keys by position and refuses a keycode-only or modified key;
   - flips the stick Y signs (Godot +Y is down, Unreal's is up);
   - maps keypad Enter to Enter, marked `merged`.
   Not a DataTable: both generators skip it.
2. **The assets**, GENERATED by `py tools/create_input_assets.py --run` (PawpocalypseEditor `UPawInputAssetBuilder`), never
   edited by hand:
   - one `IA_<Action>` per Godot action under `/Game/Pawpocalypse/Input/Actions`: Axis1D when the action has a stick or
     trigger event, else Boolean;
   - ONE context, `IMC_Pawpocalypse`, with 71 mappings (Godot keeps every action live at once).
   `Input/PawInputMap.*` reads the CSV and names the assets for the builder and the tests alike.
3. **`UPawHalfAxis`**, an Enhanced Input modifier: one direction of an axis with Godot's dead zone. Measured against Godot's own
   `Input` with a new fixture, `input_axis` (5,726 stick and trigger values): pressed from the dead zone ON (`>=`), strength
   rescaled from the dead zone to 1. A pressed value under Enhanced Input's actuation tolerance (1e-4) reads 1e-4, so it stays
   pressed.
4. **Tests**:
   - `Pawpocalypse.Input.Map`: the assets equal the CSV; the Y signs flipped; every action has both devices except the
     one-device list (pad only: `steer_*`, `recentre`; keyboard only: `tab_1..6`, `open_upgrades`).
   - `Pawpocalypse.Input.HalfAxis`: Godot's 5,726 values through the context's own modifiers, exactly.
   - `check_all` gained `L3 Pawpocalypse.Input`.
5. **Tooling bugs found and fixed**:
   - The Godot fixtures shared one sandboxed `user://`: the settings fixture's last random `settings.cfg` set the next
     fixture's stick dead zone to 0.11. Every Godot run now starts from a fresh sandbox.
   - The title fixture (since (37)) records the wall clock, so `-Full`'s "fixtures reproduce" gate could never pass on it.
     The check now masks its clock columns and compares calendar dates as dates; everything else stays byte for byte. The
     save, title, pause, settings and input_axis fixtures reproduce.
6. Docs 05 §3 (as built), 06 §2, 11 (phase 5's slices), 00, CLAUDE.md (the tool). KB: G/03 §6 (Godot's axis actions; a
   fresh `user://` per fixture; wall-clock fixtures), U/08 §4 (generating Enhanced Input assets; one half of an axis; the
   actuation tolerance), INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (with `L3 Pawpocalypse.Input` and `UBT game build`).
- Sabotages, all CAUGHT: `input_halfaxis_no_rescale`, `_strict_deadzone`, `_no_floor`, `_both_ways`, `input_map_godot_sign`,
  `input_map_merged_ignored`.

### Open
- **The steer dead zone is a setting in Godot** (`stick_deadzone`, default 0.2, applied by `InputBindings.apply`): the
  assets carry 0.2. Applying the setting at run time is slice 3.
- **The AZERTY check**: doc 05 assumes Windows `FKey`s are positions; not measured yet (slice 5).
- From before: F34, F35, F36 for the designer; the version line; the nightly `-Full`; a pad look at the pause menu and the
  settings popup.

### Next step
*"Phase 5 slice 2: add IMC_Pawpocalypse to the player and route the game and every screen through the actions (pounce on
IA_Pounce, steer on IA_Steer*, the dock's tabs / rows / buy amount / tab_1..6, the tree's open / zoom / sections, Back / Pause on
every screen), Common UI's Click / Back from IA_UiAccept / IA_UiCancel; keep the existing UI tests green and add tests that drive
the actions."*

---

## 2026-11-04 (41) - Phase 4 closed: the exit review, the packaged gate, the build on Drive

### Where the project is
- **Phase 4 is closed.** A packaged Development build is on the designer's Google Drive:
  `Games/Pawpocalypse/Pawpocalypse-UE-phase4-Development-2026-11-04.zip` (190 MB; the archive is `build/package`).
- Next: phase 5 (pad and keyboard parity, doc 11). Nothing committed after `98e3a4f`.

### What happened (designer: "Go ahead. When package is done put it in drive-pawpocalypse")
1. **The exit review** is written under doc 11 phase 4's exit criteria: each one and the tests that prove it. One was not
   covered (a save taken with a boss up), so there is a new test, `Pawpocalypse.Save.MidFight`, sabotaged with
   `save_midfight_bait_not_saved` (CAUGHT). It found **F36**: the Bait paid for an interrupted fight stays spent, so the whole
   Bait is owed again after a load (the port copies Godot; in doc 01 §5 and the Godot findings file).
2. **Packaging found two things the everyday gates could not:**
   - **The game target did not compile.** `PawWidgetTests.cpp` (since (38)) used editor-only data (`ClassGeneratedBy`,
     `HasMetaData`); it is now `#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR`. `check_all` gained a `UBT game build` gate,
     seen to FAIL with the guard removed.
   - **The bench had been measuring a held field.** Since the card pick (phase 3), level-ups dealt hands that held the field:
     83 % of the measured frames were a paused world, and the bench still said PASS. `paw.Bench` now owns Auto-Card with the
     switch on, and FAILS when the field is held on more than 1 % of the frames. It was seen to FAIL in a package with
     auto-pick off (11,277 of 13,623 frames held).
3. **The packaged gate, PASS**, with the field running every frame (`held=0`). At 1280x800 on an RTX 5070, with 320 mice and
   20 kittens:
   - game thread 1.30 ms mean / 2.58 ms p99 (budget 4);
   - render thread 1.35 / 2.00 ms; GPU 0.36 / 0.38 ms;
   - 34.6 draw calls (budget 50; 13.6 at phase 2, before the HUD's screens).
   The packaged title screen was looked at in a sandboxed run (`-PawSaveDir` to a scratch folder; nothing written there).
4. **The zip** leaves out `Pawpocalypse/Saved` (the bench's shots and logs) and adds a README (a test build; no audio,
   placeholder art, the console in). It was copied into `G:\My Drive\Games\Pawpocalypse`, the only Pawpocalypse folder on the
   designer's Drive, and Drive shows it at full size.
5. Docs 11 (the exit review; phase 4 CLOSED), 10 (the game target, the zip), 00, CLAUDE.md (the game-target gate). KB: U/18 §7
   (a frame gate must prove the simulation ran), U/24 (the editor-only data row).

### Verified
- Everyday `check_all`: every gate PASS, exit 0, with the new `UBT game build`; L3 Save 9 tests.
- The packaged gate PASS (after it was seen to FAIL on a held field).

### Open
- For the designer: **F34, F35, F36**.
- No `-game` look yet at the pause menu or the settings popup with a pad.
- Carried over: the version line still reads v1.0.0.0 (no ProjectVersion yet); the nightly `-Full`; hardening the other
  screens; editable colours for the painted widgets; the HUD's quick bar.

### Next step
*"Phase 5 (doc 11): every Godot action in Enhanced Input with both devices, the device subsystem, glyphs, every screen's focus
wiring, remapping; first a -game look at the pause menu and the settings popup with a pad."*

---

## 2026-11-04 (40) - Phase 4: the settings popup; every phase 4 step built

### Where the project is
- Every step of phase 4 is built (the save, the title, the pause menu, the settings). Next: the phase's exit review and its
  packaged gate. Nothing committed after `98e3a4f`.

### What happened (designer: "Do pop up")
1. **Godot fixture `settings`** (`tools/godot_fixtures/settings_fixture.gd`, registered): the real SettingsManager reads a
   settings.cfg that Godot's own ConfigFile wrote (keys left out, values out of range, unknown names). Then six changes are
   made on the live settings dialog. After each step it records every setting and everything the dialog shows. 3,000 rows,
   plus 3,000 `fit_to_screen` / `centred_position` cases.
2. **PawCore `PcSettings`**: every field and default of settings_manager.gd. It reads and writes Godot's settings.cfg text
   with Godot's load-time clamps, and holds the dialog's rules: the sliders, the volume lines, each list's item, the
   sections, the Output list, the effects budget and fitting the window. Measured on the way and copied:
   - Godot's text-to-number reader is not correctly rounded: "0.9500000000000001" reads 0.95.
   - The sliders snap as `floor(v / step + 0.5) / (1 / step)`, which gives +0 below zero, where C's `round` gives -0.
   - The dead zone's floor is `float(0.05)`, because Vector2 holds 32-bit floats.
   - The port's writer picks a spelling that this reader gives back exactly.
3. **`UPawSettingsSubsystem`** (game instance) keeps the file as the entry `settings` of the slots' own storage. That storage
   is now chosen by `PawStorageForThisRun`, shared with the saves, so the sandbox is the same. It is the one writer, and it
   applies:
   - the window (fitted to the screen and centred, or fullscreen), V-Sync and the frame cap: never in the editor, never in a
     run without settings storage (the bench, the tour), and the window never when the command line names one;
   - the menu side (the HUD's dock follows), ask-before-Reawaken (the HUD's box; it was a global until now), and the effects
     budget (the realm's bursts honour it, as Godot's VfxBurst does);
   - the last slot used: the saves start on it, and saving into or loading a slot moves it.
4. **`UPawSettingsDialog`**, baked to **`WBP_SettingsDialog`**, on the Modal stack (pause holder "settings"):
   - three sections, pages and Close, as settings_dialog.tscn;
   - the lists are `UPawOption`, a themed combo box (its font can only be set in a subclass's constructor; KB U/09 §3);
   - opened by the pause menu's Settings (the menu steps aside and comes back on Settings) and by the title's (the selection
     comes home to it).
   Words: `SETTINGS_*` (37 keys); L0 knows the prefix.
5. **Tests**:
   - `PcSettingsTests` (L1): the replay and the window cases, a round trip, junk files, and `[bindings]` skipped.
   - `Pawpocalypse.UI.Settings.*`: MatchesGodot (300 rows on the widget), Look, FromPause, FromTitle, Persists, Applies.
   - `Realm.View` now also checks that effects off spawns no burst.
   - The pause and title Look tests follow Settings being live.
6. **Finding F35** (for the designer): ticking Fullscreen does not grey Resolution until the dialog is reopened. Written into
   doc 01 §5 and the Godot findings file; the port copies Godot.
7. Docs 05 §6.3, 08 §3 (as built; the planned extras that Godot does not have are not built), 11, 00. KB: G/03 §6 (the
   reader, the snapping, Vector2 floats), G/07, U/09 §3, U/24, INDEX.

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (L3 UI 59 tests).
- Sabotages, all CAUGHT:
  - **PawCore (15)**: `settings_*`. One first slipped through: the -0 slider. L1 compared with `==`, which holds -0 equal to
    +0; it now checks the sign bit, and the sabotage putting `round` back is caught.
  - **Game module (16)**: `settings_*` and `pause_settings_never_opens` (replacing `pause_settings_live`).
- A miss found on the way: a pick made by code did not move its list's selection. Fixed (`ShowPicked`) and sabotaged.

### Open
- **F34, F35** for the designer.
- **Phase 4's exit**: review each exit criterion against the tests, then the packaged gate (needs the designer's go, since it
  packages).
- **Not seen running yet**: no `-game` look at the settings popup or the pause menu (the tests run without a window, so the
  combo boxes' pad behaviour is unseen).
- **Not built yet**: the HUD's quick bar (it waits on its icons).
- Carried over: the version line; the nightly `-Full`; hardening the other screens; editable colours for painted widgets.

### Next step
*"Phase 4 exit: check each exit criterion of doc 11 phase 4 against the tests; a -game look at the pause menu and the
settings popup (pad and mouse); then, with the designer's go, the packaged gate."*

---

## 2026-11-04 (39) - Phase 4: the pause menu, Widget-Blueprint-first

### Where the project is
- Phase 4 slices 1-3 done, every UI class a Widget Blueprint (38), and now the pause menu. Next: the settings popup. Nothing
  committed after `98e3a4f`.

### What happened (designer: "Continue")
1. **`PcPause`** (PawCore): pause_menu.gd's rules as data (open, close, the two pages, the two-press overwrite, the two-press
   quit and the discard, each slot's caption form, the selection on the Save page). `PcPauseTests` replays every press of the
   real pause_menu.tscn (fixture `pause`: 600 rows in L1, 2,000 in the long suite), Godot's stale captions included.
2. **`UPawPauseMenu`**, built on the PawWidgets contract and baked once to **`WBP_PauseMenu`** (now in `PawUi::BakedClasses`):
   - the layout is pause_menu.tscn's (constants `Pause*` in PawUiConstants.h), on the Menu stack, pause holder "pause";
   - Save writes through `UPawSaveSubsystem::SaveGame(slot)`; Back to Main Menu's second press saves, flushes and opens L_Title
     (`OnLeave`); the discard leaves without writing;
   - Esc / pad B are back, pad Start closes;
   - Settings is disabled until its slice.
   Words: `PAUSE_*` in ST_UI (17 keys). Godot builds " (current)" in code, so the port has one key per form with it and one
   without. L0 knows the `PAUSE` prefix.
3. **The HUD opens it** (`UPawHudScreen::OpenPauseMenu`, Esc / pad Start) only when:
   - nothing is on the Menu or Modal stack;
   - nothing else holds the field;
   - and it is not the frame after something else let go (`HeldByOtherFrame`, Godot's `_held_by_other_frame`).
4. **Tests**: `Pawpocalypse.UI.Pause.*`, 6 tests:
   - MatchesGodot: 300 fixture rows on the widget; slot lines the menu wrote are checked whole, older files up to their numbers.
   - Look, Opens, Leaves, BackKeys and Overwrite.
5. Docs 05 §6.3, 11 (phase 4), 00.

### Verified
- Sabotages, CAUGHT:
  - **PawCore (9)**: `pause_*` in sabotage_pawcore.py. `pause_overwrite_unasked` first did not compile (`false &&` breaks
    warnings-as-errors in PawCore) and was rewritten as an inverted comparison.
  - **Game module (12)**: `pause_*` in sabotage_realm.py. `pause_opens_over_a_menu` was first MISSED: every real Menu-stack
    screen also holds the field, so the hold check hid a missing stack check. Opens now stacks a plain screen that does not
    hold the field, on the Menu stack and on the Modal stack; both stack sabotages are now CAUGHT.
- Everyday `check_all`: every gate PASS, exit 0. L3 UI: 53 tests (was 47), with Widgets.AreBlueprints and Parity covering
  WBP_PauseMenu.

### Open
- **F34** (designer): an overwrite armed on the Save page survives Back to the main page. The port copies Godot until it is
  decided.
- No `-game` look at the pause menu yet, and no pause step in `godot_tour.py`.
- Carried over from (37) and (38):
  - the version line;
  - the nightly `-Full`;
  - hardening the other screens against parts deleted from their assets;
  - editable colours for the painted widgets.

### Next step
*"Phase 4: the settings popup (Audio / Video / Controls placeholders; the last slot used), Widget-Blueprint-first; then the
pause menu's Settings button opens it."*

---

## 2026-11-04 (38) - Every UI screen and piece is an editable Widget Blueprint

### Where the project is
- Phase 4 slices 1-3 done. The pause menu was started (its Godot fixture `pause` is generated and registered:
  `tools/godot_fixtures/pause_fixture.gd`, `Tests/Data/pause_godot.tsv`; no PcPause or screen yet) when the designer asked for
  the UI as Widget Blueprints; it waits, to be built Blueprint-first. Nothing committed after `98e3a4f`.

### What happened (designer: "Make sure all ui assets are widget blueprints so i can edit them"; then, asked: the WIDGET
BLUEPRINT owns the layout once baked, and the system first, the pause menu after)
1. **The system** (KB U/09 §2-2.2's pattern): `UI/PawWidgets.h` (the contract, `PawUi::Resolve`, `paw.UI.CodeLayout`, the
   baking flag with its root class, `CollectNamed`, `PawUi::BakedClasses` = the bake order), PawpocalypseEditor `UPawWidgetBaker`
   (+ `tools/bake_widgets.py --run`, headless): for each listed class WITHOUT an asset, the code layout copied into
   `/Game/Pawpocalypse/UI/Widgets/WBP_<Name>` (bound parts named and made variables, a GUID per widget) or a `BP_<Name>` child of a
   button style. **Never over an existing asset.**
2. **19 assets**: BP_ButtonStyle, BP_LockedButtonStyle, BP_FlatButtonStyle, BP_PlateButtonStyle, BP_CardFaceStyle,
   BP_TreeNodeStyle; WBP_Button, WBP_CardFace, WBP_TreeNode, WBP_TreeLinks (paint only: an empty tree), WBP_PounceClock,
   WBP_ReawakenCeremony; WBP_RootLayout, WBP_MainMenu, WBP_HudScreen, WBP_CardPickScreen, WBP_SkillTreeScreen, WBP_JobPicker,
   WBP_VictoryScreen. Every class converted: parts `BindWidgetOptional` and named; `OnLayoutReady` wires both modes; the rows from
   data (shop, realms, cards, achievements, jobs, job picker rows, card faces, tree nodes) built at runtime into named containers
   (`Page0`, `RealmList`, `CardList`, `AchievementList`, `JobList`, `List`, `Row`, `Canvas`); `UPawButton`'s instance settings
   are properties (`LabelText`, `LabelPx`, `WrapAt`, `bAlignLeft`, `bFlatLook`), its styles resolved to their Blueprints. The
   player controller and the HUD create every screen through the resolver.
3. **Bugs met writing the baker** (KB U/09 §2.2, U/24): the Common button's own wrapper saved into WBP_Button, and nested
   pieces' transient insides copied along, ended in a stack overflow on the next bake; unwrapping and detaching fixed both. The
   parity test must skip Common UI's own optional binding (`InputActionWidget`). The title screen now survives a part the
   designer deletes (null-safe accessors).
4. **Tests**: `Pawpocalypse.UI.Widgets.AreBlueprints` (every listed class resolves to its Blueprint; none native) and `.Parity`
   (each asset against its code layout: every bound part in both, the same widget classes, the same words and button settings).
   Every UI test now runs on the Blueprints (`PawUi::Resolve` in the test worlds).
5. CLAUDE.md (a hard rule and the tool), doc 05 §2, doc 00; KB U/09 §2.2, U/24 two rows, INDEX, U/00.

### Verified
- Everyday `check_all`: every gate PASS, exit 0; L3 UI 47 tests (was 45), Realm 9, on the Blueprints.
- A real `-game` run from L_Title on the Blueprints: the menu, a new game, L_Realm with the HUD, the skill tree (shots
  `Saved/PawShots/wbp_*.png`).
- Sabotages, CAUGHT: `widgets_resolver_ignores_assets`, `widgets_part_unbound`, `widgets_blueprint_never_wired` (first MISSED:
  the unwired screen crashed the editor, which the harness reads as "not run"; the Look test now asserts every part wired,
  and the screen no longer crashes), `widgets_button_label_lost`, `widgets_code_drifts_from_asset` (Parity).

### Open
- **A Parity failure after a designer's edit can be on purpose** (the asset owns the layout): read what it names before
  "fixing" anything. The other screens still index some parts directly: a part deleted from their assets fails Parity (every
  gate) rather than shipping, but would crash at runtime; hardening them like the title is open.
- The painted widgets' colours (Pounce clock, tree links) are C++ tokens, not editable properties yet (KB: "state looks as
  EditAnywhere properties").
- From (37): the version line; the nightly `-Full`; no title step in `godot_tour.py`.
- **F34 (new, for the designer)**: Godot's pause menu keeps an armed overwrite after Back to the main page (835 of the
  `pause` fixture's presses), against its own comment; written into doc 01 §5 and the Godot findings file. The pause menu
  copies Godot until it is decided.

### Next step
*"Phase 4: the pause menu, Widget-Blueprint-first (PcPause from the generated `pause` fixture; UPawPauseMenu baked to
WBP_PauseMenu), then the settings popup."*

---

## 2026-11-04 (37) - Phase 4 slice 3: the title screen

### Where the project is
- Phase 4 slices 1-3 done: the save, its container, the title screen. Next: the pause menu, then the settings popup.
  Nothing committed after `98e3a4f` in either repo.

### What happened (designer: "Do phase 4 slice 3")
1. **Fixture `title`** (`tools/godot_fixtures/title_fixture.gd`, registered in `make_godot_fixtures.py`): the REAL
   main_menu.tscn over 3,000 random games in memory and random sets of real slot files (some missing, 265 unreadable): what
   Continue / Load / the status line / the five cards / the leaderboard / the default selection show, and
   `format_when` / `format_duration` / `_kills_text` on their own inputs (the week's edge sampled on purpose: a sabotage moving it
   to 10 days was MISSED until it was). Its resolved look -> `Tests/Data/title_look_godot.txt`.
2. **`PcTitle`** (PawCore): the menu (Continue = a save on disk OR progress in memory; Load = a save; the status counts the game
   IN MEMORY), the play clock, the kills text, "last saved" (buckets, then the UTC date: civil-from-days), the leaderboard's last
   line, the slot screen's default. `PcTitleTests`: Godot's English rebuilt from the values with Godot's own templates, equal on
   1,000 rows (3,000 in the long test), every form covered; a date test across leap days and eras.
3. **`UPawMainMenu`** (`UI/PawMainMenu.*`), pushed by the player controller wherever there is no realm (L_Title): Godot's
   layout (a canvas with the scene's anchors and offsets; `PawUiConstants` Title*), the menu, the slots (an empty card starts a
   game THERE, an occupied one loads; Delete asks, Cancel under the key, one-shot), the leaderboard placeholder, the credits, the
   confirm; `FocusTop` = `_focus_top` (only the top layer wired, disabled rows dropped); Back (Esc / pad B) closes the top layer,
   on the menu asks to quit; Exit saves a game worth keeping. The slot cards swell by size (the column too). **(34)'s save traps
   closed**: a new game = `NewGame` + an empty achievement load + `SaveGame(slot)` in the same frame (the old slot kept byte for
   byte, through the end-of-frame save and an autosave); a load is followed by `OpenLevel(L_Realm)`. Seams: `OnPlay`, `OnOpenUrl`,
   `OnQuit`, `Now`. `UPawPlateButtonStyle` (the cluster's plates). `UPawHudScreen::ForgetJobsSeen` (HUD._jobs_seen = -1).
4. **Words**: 49 `TITLE_*` keys and 4 new `PROMPT_*` (A, D, T, Esc: Godot's first keys) in ST_UI; `PawText` `SlotClock`, `Mice`,
   `When`, `Version`; L0's key pattern knows `TITLE`. A `\n` inside a string-table value comes out as a line break (measured:
   the cards equal Godot's; KB U/20 §2). `EngineSettings` added to the game module (the version line).
5. **`paw.Title <press> [n]`** (Dev/PawShot.cpp) and `APawPlayerController::GetMainMenu`: a real `-game` run from L_Title
   (`-PawSaveDir` to a scratch folder): the menu, the leaderboard, the slot screen, slot 1 -> `slot_0.sav` written ->
   `LoadMap L_Realm` -> "Realm started in tier_1" with the HUD up. Shots in `Saved/PawShots/title_*.png`.
6. Doc 05 §6.3 (the title screen), doc 11 phase 4, doc 00.

### Not yet (each waits for its phase; doc 05 §6.3)
- The garden (a paper floor), Boba's photo and the cameo on a click (the frame and caption keep his place), the cluster's icons
  (the words), the menu music: phase 6. The Language list: phase 7 (the button is there, disabled). Settings: the settings slice
  (disabled). The last slot used = the save's active slot until settings persist it. The title -> realm fade (doc 03 §4).
- The version line reads `v1.0.0.0`: `ProjectVersion` is unset in DefaultGame.ini (Godot's says v0.0.2 / itch 0.2.5): the
  designer's call (doc 10).

### Verified
- Everyday `check_all`: every gate PASS, exit 0 (L1 167 cases; L3 UI 45 tests, was 38). The Development game target builds.
- `Pawpocalypse.UI.Title.*` (6): MatchesGodot (300 fixture rows: status, Continue / Load, every card and its Delete, the
  leaderboard), Look, NewGame, Load, Delete, BackAndQuit.
- Sabotages, each CAUGHT, the clean build passing: game module 13 (`title_status_plural_swapped`, `title_card_when_ignores_clock`,
  `title_unreadable_has_delete`, `title_mice_k_plain`, `title_new_game_into_active_slot`, `title_new_game_keeps_achievements`,
  `title_load_skipped`, `title_confirm_default_yes`, `title_confirm_not_one_shot`, `title_exit_never_saves`,
  `title_menu_wires_disabled`, `title_back_quits_unasked`, `title_discord_opens_the_board`); PawCore 8 (`title_*` in
  `sabotage_pawcore.py`; `title_when_week_is_days` MISSED until the fixture sampled the week's edge).
- Not run: the nightly `-Full` (its fixture check now includes `title`); the hover swell is not driven by a test (no pointer
  in the automation world); no side-by-side shot of Godot's title yet (`godot_tour.py` has no title step).

### Next step
*"Phase 4: the pause menu (pause_menu.gd: Resume, Save, Settings, Quit to title, quit saves first; its format_duration and
_kills_text are PcTitle's already), then the settings popup and the last slot used."*

---

## 2026-11-04 (36) - The save findings F26-F33: written up, decided, fixed in Godot, the port followed

### Where the project is
- Phase 4 slices 1-2 done, the title screen (slice 3) next. Nothing committed after `98e3a4f` in either repo.

### What happened
1. **Written up** (designer: "Go"): every save finding of (34) and (35) re-read in the Godot code (file and line) into doc 01
   §5 and the designer's copy (`C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`): F26 the deepest biome not
   saved (Continue opens biome 1: every capped line back to its biome-1 cap, kittens 24 -> 10 after the Mythic Realm, every
   open biome NEW again); F27 Second Helping's deferred Stardust; F28 Body Count's and Levy's counts (Infestation and
   Reputation are documented as never saved on purpose: not findings); F29 a quit mid-Reawaken skips the spend step, the
   job picker and Territory's start; F30 the shop's `catnip < cost` vs the purchase's epsilon; F31 the GDD's kitten line;
   F32 saved levels unchecked; F33 NaN/inf survive a `.tres` save. F5 also notes NaN/inf.
2. **Decided** (designer, 2026-11-04, every recommended answer): save the deepest biome; save all three counters; resume a
   Reawaken on load; apply the four small fixes.
3. **Godot first** (uncommitted there; on top of its 2026-11-03 changes): `pawpocalypse_save_data.gd` five new @exports
   (no version bump); `save_manager.gd` saves / restores them (clamped), `_finite_numbers()` (every non-finite float of a
   loaded save reads 0); `skill_tree_manager.gd` / `game_state.gd` `apply_save_data()` clamp levels; `game_state.gd`
   `reawaken_in_progress` (reset by a new game); `hud.gd` sets it BEFORE `reawaken()` (SaveManager saves on `reawakened`,
   inside that call), clears it in `_travel_to_starting_biome()`, `_resume_reawaken()` deferred from `_ready()`, the shop
   button's epsilon; the GDD line. Tests added: `check_playthrough.gd` `_test_run_state_survives_a_restart`,
   `_test_damaged_save_loads_safely`; `check_hud.gd` `_test_shop_button_matches_the_purchase`,
   `_test_reawaken_resumes_after_a_restart`. `messages.pot` re-extracted (line references only). The suite's run had made
   two `.uid` files next to the designer's untracked probes: deleted, the repo's status is as before.
4. **The port followed**: `PcSaveState` + `CaptureSave` / the file / `ApplySave` (non-finite -> 0 first, then the clamps,
   then the five fields), `PcProgress::bReawakenInProgress` (cleared by `NewGame`), `PcHud` ShopRow's epsilon;
   `UPawHudScreen`: the flag set before `Pc::Reawaken`, cleared in `TravelToStartingBiome`, `ResumeReawakenIfPending()` in
   `Pump` (waits while the ceremony, the Menu stack or the Modal stack is up: unlike Godot it also covers a later load from
   the title). Fixture `save` extended (out-of-range and non-finite fields, the flag) and regenerated from the fixed Godot;
   the fixture test now also checks the file read back against Godot's save columns (a field the writer dropped would
   otherwise round-trip unseen) and asserts coverage of every new case.

### Verified
- Godot `tools/check_all.sh`: every gate green but `loc` before the `.pot` refresh; `loc` green after (playthrough 1,200
  assertions, hud 6,652). Godot sabotages (a scratchpad script, restored byte for byte): 12 of 12 make their gate fail;
  11 by the named assertion, `resume_always` by the hud gate timing out (the forced resume travels the test's HUD).
- Fixture `save` (3,000 rows; first 1,000 in the repo): 724 non-finite numbers loaded, 476 deepest biomes out of range,
  738 mid-Reawaken saves, negative levels and counters; Godot reads every non-finite number as 0 (so F33 was real: the
  `.tres` keeps NaN). PawCore matches on all 3,000 rows (the long test) and 1,000 in L1; L1 165 cases.
- PawCore sabotages, CAUGHT: `save_deepest_lost`, `file_drops_the_flag`, `reader_skips_levy`, `load_deepest_unclamped`,
  `load_pending_negative`, `load_body_count_lost`, `load_flag_lost`, `load_nan_kept`, `load_branch_unclamped`,
  `load_shop_negative`, `newgame_keeps_the_flag`, `shop_row_strict`. Game module (`sabotage_realm.py`), CAUGHT by
  `Pawpocalypse.UI.Ceremony.Resumes`: `reawaken_flag_never_set`, `reawaken_flag_never_cleared`, `reawaken_never_resumes`,
  `reawaken_resume_under_the_tree`.
- `make_godot_fixtures.py --check`: every fixture reproduces byte for byte from the changed Godot code (the other managers
  are untouched by it). Everyday `check_all`: every gate PASS, exit 0, after `Data/CodeConstantsMap.csv` gained the four new
  numeric save defaults (destination `save`; L0 had failed on them) and appendix C was regenerated (580 constants);
  `Data/CodeConstants.csv` unchanged.

### Open
- Commit both repos (the designer's call). The Godot repo now also holds F26-F33.
- From (35): no gate builds the game target; the tree nav test does not check where Next leads.
- From (34): the title-screen traps (`NewGame` / `LoadGame` / `paw.Cheat.Reawaken`).

### Next step
*"Phase 4 slice 3, the title screen (doc 11 phase 4), minding HANDOFF-LOG (34)'s NewGame / LoadGame save traps; a load from
the title must StartRealm, and a mid-Reawaken save resumes by itself (36)."*

---

## 2026-11-04 (35) - (34)'s robustness findings fixed; the spike code deleted

### Where the project is
- Phase 4 slices 1-2 done; the title screen (slice 3) next. Committed and pushed through `98e3a4f`; everything below is
  uncommitted, for the designer to review.

### What happened (designer: "Yes do that please", to (34)'s step 1 + the start-here fix + deleting the spikes)
1. **Godot measured first.** A headless probe in the sandboxed scratch copy (Godot 4.7.1): `"%.Nf" % NAN/INF/-INF` = `nan`
   / `inf` / `-inf` (N = 0..3); `int()` and `roundi()` of NAN, INF, -INF and 1e300 = `-9223372036854775808`;
   `Amounts.short(NAN)` and `(INF)` = `-9223372036854775808Qi`, `(-INF)` = `--9223372036854775808Qi` (NaN skips the loop and
   reads `SUFFIX_ORDER[-1]`, the last).
2. **`PcFormat`, non-finite safe.** One digit splitter (`SignificantDigits`, zeroed buffer, refuses text without an `e`)
   for `FixedLikeGodot` and `OneDecimalLikeGodot`: no more `atoi` past the string. `FixedLikeGodot` prints Godot's
   `nan`/`inf`/`-inf`; `IntLikeGodot` / `RoundiLikeGodot` give Godot's `INT64_MIN` for NaN, the infinities and past int64,
   spelled out instead of a C++ cast that is undefined; `ShortAmount` prints `nanQi` / `infQi` / `-infQi` (Godot's suffix;
   the value instead of int()'s overflow, the deviation it already had past int64; header says so). `FixedLikeGodot`'s
   past-1e15 buffer 64 -> 400 (1.8e308 with 6 decimals is 316 characters; it truncated).
3. **`PcJson`.** A number past a double's range now reads as every other reader reads it (`1e400` = inf, `1e-400` = 0, sign
   kept: `OverflowsDouble`, from the first significant digit's decimal exponent, saturated); it read 0 because `from_chars`
   leaves the value untouched on `result_out_of_range`. The duplicate-key check is a hash set per object (was a scan per key:
   100,000 keys = 5e9 compares). First-wins kept.
4. **`PcProgress`.** `IsFinalBossRealm` guards the three `BossRealms.back()` reads (`MarkBossDefeated`, `RecordBossDefeat`,
   `RaisesVictory`).
5. **Doc 03 §6 follows the code:** a validator failure leaves the game without data in every build (`bDataReady` false, no
   realm), Shipping included; only the log level differs. Running on refused rules would be worse; the gates catch it first.
6. **The spike code deleted** (all 124 files tracked and on origin first): `Source/Pawpocalypse/Dev/Spikes` (8),
   `tools/spikes` (10), `Content/Pawpocalypse/Dev/Spikes` (106, 4.9 MB); the spike-only `ImageCore` / `Paper2D` module
   dependencies out of `Pawpocalypse.Build.cs`. All of it is in `10c118e`; doc 07 says where the S3 raster/pack script is
   (`git show 10c118e:tools/spikes/s3_raster_pack.py`), docs 00, 04 and 11 say it is gone.
7. **Doc 00**: "nothing is committed since `7410909`" was stale (now: pushed through `98e3a4f`); the spike rows gone.
8. **Unity builds off in both modules** (`bUseUnity = false` in `Pawpocalypse.Build.cs` and `PawCore.Build.cs`). Deleting the
   spike files made the editor build fail in files nobody touched: the unity groups reshuffled, and so did UBT's adaptive
   unity, which pulls git-modified files out of the groups. Two failure shapes: same-named file-local helpers (`S` /
   `Shown` / `Rule` / `Utf8` in the UI tests, `IsEqualApprox` / `Upgrade` in PawCore: C2084) and a local shadowing another
   file's file-scope colour (`Paper` in PawRealmView vs PawHudScreen: C4459). Renaming would only fix today's grouping. Without
   unity each file compiles alone (PawCore exactly as the standalone build compiles it), and an edit recompiles one file.
9. **The Development GAME target did not build** (an older bug, found by building it): `PawTreeTests.cpp`'s nav helper
   called `UWidgetNavigation::GetNavigationData`, which only exists `WITH_EDITOR`; automation tests compile into the
   Development game build (the one `packaged_bench.py` packages). It now reads the fields, as PawHudTests does. Negative test: Left
   read as Right -> `Pawpocalypse.UI.Tree.Navigation` FAILED (261 of 348 rules), restored -> 7/7 PASS. (A Next -> Previous
   mutant was NOT caught: the test only asserts Next is non-null.) Game target Development and Shipping both compile.
10. **Knowledge base**: G/03 §6 (non-finite values in Godot, measured), G/07 row; U/10 §5.3 (a hand-written JSON reader:
   `from_chars` range, duplicate keys); U/01 §2.5 (unity regrouping on delete / adaptive edit, C4459, `bUseUnity = false`);
   U/24 rows (from_chars, unity, editor-only `GetNavigationData`); both folder indexes and the symptom index.

### Not acted on, and why (for the designer: Godot first)
- `ApplySave` does not clamp levels: Godot does not either (`skill_tree_manager.gd` 341 takes `int(saved)`; `game_state.gd`
  2511 the upgrade levels as they are), so clamping is a load-rule change. Same for a NaN or infinity in the save: Godot's
  `.tres` writes and reads them back too. Both go with the save findings of (34).

### Verified
- Everyday `check_all` after the last change: every gate PASS, exit 0 (L1 164 cases, was 161; L3 Data 2, Realm 9, Save 8,
  UI 38). UBT game target: Development and Shipping compile (Development did not before item 9).
- New tests: "Formatting never reads past its text on NaN or an infinity" (Godot's measured strings), "a number past a
  double's range", "a repeated key keeps its first value, and 100,000 keys read in linear time", and a no-bosses case in
  "Progress rules from the Godot comments".
- Sabotages, each seen CAUGHT with the clean build passing: 8 new (`format_nonfinite_reads_on`, `format_nonfinite_first_suffix`,
  `format_int_overflow_max`, `json_out_of_range_zero`, `json_overflow_by_digits`, `json_exponent_sign_lost`,
  `json_duplicate_kept`, `final_boss_on_no_bosses`) and 6 retargeted at the rewritten lines (`format_more_digits`,
  `progress_final_clock_every_clear`, `progress_ending_on_any_boss`, `victory_any_boss`, `victory_first_clear_only`,
  `victory_every_last_boss_kill`); `format_half_up_strict` and `format_no_negative_zero` re-run. 453 PawCore sabotages, every
  target present exactly once (checked). The digit buffer is zeroed so the guard's mutant fails the same way every run.
  The nightly `-Full` has not run on this state.

### Open
- (34)'s save findings, to write up for the designer (doc 01 §5 + the Godot findings file), now with the two above.
- (34)'s title-screen traps (`NewGame` / `LoadGame` / `paw.Cheat.Reawaken`); (34)'s gate items (sabotage scripts edit real
  sources; no sanitizer build; the colour lint skips `Realm/`). Dev/Spikes in Shipping: gone.
- **No gate builds the game target**: `check_all` builds only the editor target, which is how item 9 went unnoticed for a
  week. Proposed: a `UBT game build` gate (Development; ~30 s incremental). Not added without asking.
- The tree nav test does not check where Next leads (only that it is set).
- Still open from (33): commit the Godot repo; the F19 hint's translations; F6; the phase 3 packaged-build gate.

### Next step
*"Write up HANDOFF-LOG (34)'s save findings (plus (35)'s level clamp and non-finite values) for the designer: doc 01 §5 and
`C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`. Then phase 4 slice 3, the title screen, minding the NewGame /
LoadGame save traps in (34)."*

---

## 2026-11-04 (34) - A whole-codebase review; one port bug fixed: the kittens chased the oldest boss add

### Where the project is
- Unchanged from (33) but for the fix below: phase 4 slices 1-2 done, the title screen next. The port is committed and pushed (`10c118e`); the Godot repo is not.

### What happened
1. **A review of the docs and the code** (PawCore whole; the game module's Core, Data, Realm, Render, Save, Dev and the UI's
   logic; tests and tools sampled). Its findings are under Open; only item 2 was acted on (designer: "Yes" to the fix).
2. **Port bug fixed: `PcBossAdd::Serial` was never set** (`PcField.cpp` TickBossAdds), so every add had serial 0. The kittens
   pick the add nearest the cat and find it again by serial (`HuntKittenAdd`): with every serial 0 they chased `Adds[0]`, the
   OLDEST add, and the "already intercepted this step" guard never fired, so the pool chewed through several adds in one step.
   Godot passes the add node itself (`tier_controller.gd` 1955, `kitten.gd` 385). Now `A.Serial = F.AddSpawns`. The only test
   used one add; new test "Boss: with two adds out, the kittens chase the one nearest the cat, and once it falls the rest
   wait". Sabotages `adds_share_one_serial`, `kittens_chase_the_next_add`: CAUGHT (with `kittens_ignore_adds`), clean build passes.
3. **The engine gates never run on a stale DLL** (designer: "Yes, do those"). `tools/run_automation.py` builds the editor target
   first and refuses to test when the build fails (`--no-build` for a caller that just built: `sabotage_realm.py`, `check_all`).
   `check_all`'s everyday set gained the `UBT editor build` gate BEFORE the engine gates (moved out of `-Full`); a failed build
   FAILS every engine gate unrun. **A SKIP is no longer a pass**: exit 2 ("NOT PROVEN"). CLAUDE.md and doc 09 §7 say so.
   Each path seen: a fake `UnrealEditor-*` process (a renamed ping.exe in the scratchpad, stopped after) -> 6 SKIP, exit 2; a
   new file with `#error` -> `run_automation` exit 1 "nothing was tested", `check_all` UBT FAIL + 5 engine gates FAIL unrun,
   exit 1 (file and its intermediates deleted); clean -> every gate PASS, exit 0; `sabotage_realm.py --only
   realm_no_step_cap` (now `--no-build`) CAUGHT, clean build passes.
4. **Stale text fixed:** `PcCardDraw.h` RerollHand and doc 02 §7 (F11); `PawRealmSubsystem.h` ChooseCard and doc 03 §2 twice
   (F14); `PcCheats.h` CheatUnlockAllStages (F18); `PcJson.h` (the shortest exact text, not 17 digits); `PawRealmView` Burst
   (a total cap); `check_all.ps1` header (~5.5 h); doc 02 §3: kittens never scale with evolution (the code never has; Godot's
   GDD line 95 still says they do: for the designer).

### Verified
- Everyday `check_all`: every gate PASS. Then UBT (the PawCore DLL newer than the edit) and `Pawpocalypse.Realm` again on the
  rebuilt binaries: 9/9 PASS. (The everyday set's L3 had run on the previous DLL: see the gate item under Open.) L1: 161 cases.

### Open (the review's findings, none acted on)
- **For the designer (Godot first; each a new finding for doc 01 §5 and the Godot findings file):**
  - `deepest_biome_index` is not saved: after a restart (Continue starts in biome 1) every `cap_per_floor` falls back to the
    biome-1 caps until the player walks back down (against game_state.gd 2120's own comment), and every unlocked biome reads
    "new" (`PcHud` ZonePlate / IsRealmNew).
  - `pending_second_helping` is not saved: quitting before biome 4 loses Second Helping's deferred half of a Reawaken.
  - Also reset by a restart: Body Count (`kills_this_cycle`), Reputation (`waves_here`), Levy's count, Infestation.
  - A restart between Reawaken and the job pick: no spend step (the tree is read-only outside it), no job asked this run,
    Territory's start skipped.
  - The shop button disables at `catnip < cost` (hud.gd 1673, `PcHud` ShopRow) while the purchase forgives `currency_epsilon`.
  - Godot's GDD (`docs/pawpocalypse-gdd.md` line 95) says kittens scale with evolution; the code never has (pawsim's
    kitten_dps_each says why). The port's doc 02 is fixed; the GDD is the designer's.
- **Robustness:** `ShortAmount(NaN)` reads `Suffixes[-1]`, `FixedLikeGodot` / `OneDecimalLikeGodot` `atoi` past the string on
  NaN / inf (`PcFormat.cpp`), and a NaN would round-trip through the save; `PcJson` reads `1e400` as 0 (from_chars leaves the
  value on out_of_range) and its duplicate-key check is O(n^2); `ApplySave` does not clamp levels (a negative branch level locks
  the node); `BossRealms.back()` unguarded in `MarkBossDefeated` / `RecordBossDefeat`; a validator failure in Shipping leaves
  no realm at all (doc 03 §6 says it only logs).
- **Before the title screen (slice 3):** `NewGame()` changes the save digest, so the end-of-frame save writes the EMPTY game into
  the ACTIVE slot unless `SaveGame(newSlot)` runs the same frame (`paw.Cheat.Reset` does this to the real slot today);
  `LoadGame` does not restart the realm (ApplySave clears `CurrentBiomeId`): any Load must `StartRealm`; `paw.Cheat.Reawaken`
  leaves the realm biome-less (no ceremony, spend step or travel).
- **Gates (the build order and SKIP are done, item 3):** both sabotage scripts edit real sources (safe only while the tree is
  committed); no sanitizer build of PawCore; the colour lint skips `Realm/`; Dev/Spikes still compiles into Shipping.
- Still open from (33): commit the Godot repo (the designer's call); the F19 hint's translations; F6; the phase 3 packaged-build gate.

### Next step
*"Act on the rest of HANDOFF-LOG (34)'s findings: the non-finite guards in PcFormat / PcJson; write up the save findings for the
designer (Godot first). Then phase 4 slice 3, the title screen, minding the NewGame / LoadGame save traps listed there."*

---

## 2026-11-03 (33) - The nightly -Full after the findings pass: 20 of 21 green, two stale PawCore sabotages retargeted

### Where the project is
- Unchanged from (32): phase 4 slices 1-2 done, every open finding fixed (Godot first). Nothing committed in either repo.

### What happened
1. Ran `tools\check_all.ps1 -Full` (about 5.5 h; summary in `build/check_all/_full_1103.txt`). 20 of 21 gates PASS, including
   L3 Pawpocalypse.Save, Godot fixtures reproduce, L1 long, pawsim parity sweep and Realm sabotage (every realm sabotage caught).
2. **PawCore sabotage FAILED, both from harness drift, not from the code** (441 caught, 2 not):
   - `kitten_recount_every_step` filtered on "Kittens: a card that costs*", a test the F14 pass renamed, so the filter matched no test
     (MISSED, and the clean-build check failed too). Retargeted to "Kittens*": CAUGHT, clean build passes.
   - `ach_waiting_is_earned` still wrote the one-argument `EarnedAchievementCount(P)` that F9 replaced with `(D, P)` (NOBUILD).
     Fixed the mutant: CAUGHT, clean build passes.
   Only `tools/sabotage_pawcore.py` changed; the full PawCore sabotage gate was not rerun (the two fixed entries were, with `--only`).
3. Docs brought up to date (designer: "make sure all docs are up to date"):
   - the nightly's duration, ~5.5 h (CLAUDE.md, doc 00, and doc 09 §7 with the measured gate times);
   - the test and sabotage counts: L1 160 cases / 4.72 M assertions, long 31 / 5.5 M, PawCore 443 sabotages, realm 236;
   - 39 achievements (docs 01, 06);
   - F23's second job is now saved (docs 08, 11);
   - the victory screen now wears the game theme (doc 00);
   - a `\b` (backspace) that had eaten a backslash in doc 09's save row.
   Appendices A, B and C were regenerated to scratch and are identical to the ones in docs/.

### Open
- Commit both repos (designer); a translator's check of the F19 hint; F6 (whether the silent lock is wanted); the phase 3
  packaged-build gate.

### Next step
*"Phase 4 slice 3: the title screen on L_Title (main_menu.gd: Continue / New Game / Load over five slot cards with summaries and
delete-with-confirm; New Game in an empty slot = NewGame + SaveGame(slot); Report a bug, Discord, Settings, Quit saving only with
progress); then the pause menu and the settings popup."*

---

## 2026-11-03 (32) - Q3 no; F24 the run's cards saved; every open finding fixed (Godot first, the port followed)

### Where the project is
- **Designer decisions 2026-11-03**: Q3 "No itch saves for unreal" (no importer); F24 "the runs cards and xp should be saved";
  then, asked which fixes: **"All open findings"** (each finding's suggested fix, Godot first).
- Phase 4: slices 1-2 done (31); slice 3 (the title screen) not started this session: the findings took it.
- **Nothing committed** in either repo. Godot repo, uncommitted for the designer: `scripts/autoload/` achievement_manager,
  card_manager, game_state, save_manager; `scripts/data/pawpocalypse_save_data.gd`; `scripts/systems/` hud, job_picker, kitten,
  tier_controller, victory_screen; `data/achievements/` two files DELETED; `locale/` (pot and every .po: merged, LF kept);
  the findings file. Godot's `tools/check_all.sh`: ALL GREEN, 12,949 assertions (6 fewer: the two achievements).

### What happened
1. **F24, the run's cards and XP** (Godot first): PawpocalypseSaveData gains `run_cards_saved` and the run's cards (owned,
   banished, Catches level and XP, rerolls, extra choices, banishes, Fixation hands, the HAND on screen, Blueprint's promise,
   Prototype's mark, cards_taken_this_run): new @exports, no version bump, an older save loads as it always did.
   `CardManager.apply_run_save()` / `reoffer_after_load()` (TierController calls it: a saved hand comes back, the same cards).
   Port: `PcSave` (fields, capture, Godot's restore: unknown ids dropped, the mark only on a card in the hand), JSON string
   arrays; the HUD already opens the pick on any pending hand; `Pawpocalypse.UI.CardPick.SavedHand`. Fixture `save` extended:
   2,100 restored runs, 540 older saves keep memory's, 797 hands re-offered once.
2. **Q3**: no importer; doc 08 §2.1, doc 11.
3. **Every open finding with a fix** (Godot first; then port; doc 01 §5 and the Godot findings file have each row):
   F13 Giblets at `death_position`; F16 the five Overkill splashes from `death_position`; F17 Riddle spent by the NEXT swing;
   F14 a card taken re-counts the kittens (TierController on `card_taken`; port: `ChooseCard` and Auto-Card in the field);
   F18 the unlock-all cheat sets this cycle's passage; F19 the puddle cards open with Rainbow Poop, hint "Buy the Rainbow Poop
   branch in the skill tree." (20 translations built from each language's own strings, 8 hand-fixed: **worth a translator's
   look**); F23 Union Rules' second job saved; F7 Fence's window closes in `begin_biome`; F8 Kitten Speed refused with no
   kitten in `purchase_upgrade`; F9 `earned_count` counts the roster only; F10 one promise per deal; F11 a reroll / banish
   re-marks and re-promises; F2 the two unearnable achievements REMOVED (39 now); **F6: behaviour kept, comment fixed** (the
   comment's silent lock would have taken "No job" away and nothing implemented it: the one finding where the comment's version
   was not taken). Not fixed (no fix to apply): F3 (a design note), F4 / F5 / F12 (known divergences), F15 (cards that may still
   ship); F25 (Godot's .tres precision) is an engine trait.
4. **The port**: data re-exported (39 achievements), DataTables re-imported, ST_Data / appendices A-C regenerated; PcCards,
   PcAchievements (`EarnedAchievementCount(D, P)`), PcProgress, PcCheats, PcCardDraw (`MarkAndPromise`), PcField (the parking-
   spot switches deleted; Riddle; Auto-Card re-count), PcValidate (the known-issues list now empty), the realm's `ChooseCard`;
   every Godot fixture regenerated; tests that pinned Godot's old behaviour flipped to assert the fixes; new tests (Auto-Card
   kitten re-count, Kitten Speed lock). The cards fixture's `step_rerolls` column now switches auto-pick off (it was measuring
   the deal too; exposed when F10 shifted Godot's random sequence; KB G/03 §6). `Data/CodeConstantsMap.csv` rows for the new
   save fields (the data-export gate caught them).
5. Sabotages: PawCore 6 (F24) + 20 (the findings: 9 re-targeted from the old pinned behaviour, 11 new; two first written with
   unused names failed /WX and were rewritten), realm 3 (saved hand, Giblets, ChooseCard re-count): all caught.

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1103d.txt`). PawCore 160 test cases. UI 38, Realm 9,
  Save 8, Data 2. Godot `check_all.sh`: ALL GREEN. `make_godot_fixtures.py` regenerated every fixture.

### Open
- **For the designer**: commit both repos; the F19 hint's translations; F6 (say if the silent lock is wanted after all);
  the packaged-build gate for phase 3.
- Nightly `-Full` not run since (30); it should run before phase 4 continues (the fixtures and many sabotages changed).

### Next step
*"Run the nightly `check_all -Full`. Then phase 4 slice 3: the title screen on L_Title (main_menu.gd: Continue / New Game /
Load over five slot cards with summaries and delete-with-confirm; New Game in an empty slot = NewGame + SaveGame(slot); Report a
bug, Discord, Settings, Quit saving only with progress); then the pause menu and the settings popup."*

---

## 2026-11-03 (31) - F21 and F22 decided and fixed; phase 4 slices 1-2: the save and its container

### Where the project is
- **Designer decisions 2026-11-03**: F21 "yes" (the job picker and the victory screen wear the game theme); F22 "that should not
  happen" (the Realms page must not walk past a boss not beaten this run). Both fixed in Godot first, the port followed.
- **Phase 4 started** (doc 11): slice 1 (the save in PawCore) and slice 2 (its container in the game) done. Next: the title screen.
- **Nothing committed** in either repo. Godot repo, uncommitted for the designer: `scripts/autoload/game_state.gd`
  (`is_passage_open`), `scripts/systems/hud.gd` (F22, and F20 from (23)), `job_picker.gd` + `victory_screen.gd` (F21),
  `locale/messages.pot` (re-extracted: source line references only, LF kept), `card_manager.gd` + `tier_controller.gd` (from
  earlier), and the findings file. Godot's whole `tools/check_all.sh` passes (12,955 assertions) in a sandboxed copy.

### What happened
1. **F22** in Godot: `GameState.is_passage_open(stage)` (the stage AND this cycle's bosses), used by `is_biome_unlocked()` and by
   both branches of `_refresh_travel_panel()`: a row behind an unbeaten boss reads "<biome> — locked" (an existing string) and is
   disabled. Port: `Pc::IsPassageOpen`, `PcHud RealmRow` asks it; `PcHudTests` new case; the playthrough now pins that the page
   does NOT travel there. Sabotages: 2 PawCore + `playthrough_realm_press_unguarded`, caught.
2. **F21** in Godot: the picker's root gets the game theme (its `WindowPanel` applies); the victory screen gets a themed root and
   its column a 640-wide `WindowPanel`. Fixture `victory` updated (the new tree; texture styleboxes printed) and regenerated.
   Port: `PawUi::WindowBrush()` / `WindowPadding()`, both screens on paper with ink text and the theme's buttons; the engine look
   (`SetEngineLook`, `UPawEngineButtonStyle`, the `GODOT_*` tokens and their generator block) removed. 6 sabotages caught.
3. **Slice 1, `PcSave` + `PcJson`** (PawCore): Godot's PawpocalypseSaveData v7 field for field; `CaptureSave` (banks the live
   biome; every known upgrade / branch id, zeros included) and `ApplySave` (Godot's load order, the v6 tree migration, the
   Unicorn split, the reset path for old versions, take_job, the run-job seed); the file: `PAWSAVE <format> <crc32> <len>` +
   JSON body, every double exact. **Fixture `save`** (new): the REAL save_game() / load_game() on 3,000 random states and saves;
   all 1,000 / 3,000 rows equal. Found on the way: **Godot's .tres writer is lossy** (16 digits; float32-exact doubles written with
   float32's digits), so the fixture reads back what a load sees and the save side compares within 1e-7 (F25, KB G/07).
   21 sabotages caught (one equivalent mutant replaced).
4. **Slice 2, the container** (`Source/Pawpocalypse/Save`): `UPawSaveSubsystem` over `IPawSaveStorage` (platform
   `ISaveGameSystem` / folder / memory). ISaveGameSystem has no rename, so the write is: temp, verify, backup only if the current
   slot reads, slot, drop the temp; startup recovery; backup fallback on load; a newer build's slot never read nor overwritten;
   the end-of-frame save from a digest of exactly what Godot's seven save signals report (one write per frame); autosave every
   30 s of unpaused play; focus loss, suspend, quit; boot loads the active slot. Sandbox: `-NoSave`, `-PawSaveDir=`; automation
   and `-unattended` runs get no storage at all; the bench and tour now pass `-NoSave`. `Pawpocalypse.Save.*` 8 tests, gate
   `L3 Pawpocalypse.Save` in `check_all`. 23 realm sabotages caught (two real test gaps closed from the first misses: a good slot
   beside an older backup; the field's own auto-travel input).
5. **The per-slot automations** moved from HUD globals onto `UPawGameSubsystem::Slot()` (with the tutorial's steps); the realm
   feeds them to the field every frame; New Game clears them. **Port bug fixed**: buying Auto-Travel / Auto-Card in the tree did
   not switch the automation on (PcTree reported it, nobody read it); `UI.Tree.Spend` now checks it.
6. **Doc fixes**: doc 08 §1-2 rewritten to what Godot saves (it had promised run state Godot never saved) and how the container
   works; doc 02 §4 (a boss timeout does not send the player back or switch auto-travel off: Godot never did); docs 00, 01 (F21,
   F22 decided; F23-F25 new), 05, 09, 11. KB: U/10 §1 (atomic save over ISaveGameSystem; sandbox by construction), G/07 (.tres
   precision), U/18 §8 (counting CRLF), INDEX symptom rows.

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1103b.txt`), incl. the new `L3 Pawpocalypse.Save`.
- `make_godot_fixtures.py --only victory --only save --check`: byte for byte. Godot `check_all.sh` (sandboxed copy): ALL GREEN.
- Sabotages this session: PawCore 2 (F22) + 21 (save), realm 7 (F21 / F22) + 23 (save, toggles): all caught.

### Open
- **For the designer**: F23 (Union Rules' second job is not saved), F24 (the run's cards and XP are not saved: a restart loses
  them), Q3 (import itch saves?); the Godot changes to commit; the packaged-build gate for phase 3.
- The last slot used is not remembered yet (Godot keeps it in settings.cfg): the active slot starts at 0 until the settings slice.
- Nightly `-Full` not run since the save code.

### Next step
*"Phase 4 slice 3: the title screen on L_Title (docs/05, main_menu.gd): Continue / New Game / Load over five slot cards with
summaries and delete-with-confirm, New Game in an empty slot = NewGame + SaveGame(slot), the garden and Boba, Report a bug,
Discord, Settings, Quit (saves only with progress); the realm opens after it. Then the pause menu and the settings popup."*

---

## 2026-11-03 (30) - The nightly -Full: two sabotage-harness bugs fixed, every gate green

### Where the project is
- Unchanged from (29): phase 3's exit criteria met, the packaged-build gate waits for the designer's ask. No game code changed.
- **Nothing committed** in either repo; the Godot repo was only read (its status before and after: the F20 changes and the
  findings file from (23)/(29)).

### What happened
1. `check_all -Full` (night of 2026-11-02, ~3.7 h, `build/check_all/_full_1102.txt`): 19 gates PASS, 2 FAIL, both harness bugs:
   - **PawCore sabotage: 38 MISSED** (366 caught). Every one had a multi-line target in a CRLF file: `PcField.cpp`, `PcFormat.cpp`
     and `PcHud.cpp` are CRLF, every other source LF (counted with `tr -cd '\r' | wc -c`; Git Bash's `grep -c $'\r$'` first
     reported every file CRLF, wrongly); `sabotage_pawcore.py` checked targets with `read_text()` (CRLF read as LF, so "occurs
     once") but replaced in `read_bytes().decode()` (CRLF kept), so the replace changed nothing, the unchanged code was rebuilt
     and passed. Fixed: `in_file_endings()` puts the target in the file's own endings, the count runs on the same raw text, and
     a mutant whose replace changes nothing is refused as `NOLAND` before any build (negative-tested: it fires). Same fix in
     `sabotage_realm.py` (its sources are LF today, so it was latent there).
   - **Realm sabotage: STALE** `hud_boss_message_dropped`: (27) put the victory call between its two target lines. Re-targeted
     on the same assignment (`BossMessageTier = F.TierIndex;` before `if (E.Flags & Pc::FieldFlag::Victory)`).
2. KB U/18 §8 (count and replace in the same bytes; assert the mutant landed), INDEX row and a symptom-index row.

### Verified
- The 38 rerun with the fix: 38/38 CAUGHT, clean PASS (`build/check_all/_rerun_38_1103.txt`). With the 366 caught in the
  nightly: all 404 PawCore sabotages caught.
- `sabotage_realm.py` in full: 208/208 CAUGHT, clean PASS (`build/check_all/Realm sabotage.log`, 04:48).
- Every other `-Full` gate PASS in the nightly (L1 long suite, Godot fixtures reproduce, mirror 200k, fuzz 30k, pawsim parity
  sweep, UBT, the self-tests, Godot repo untouched).

### Open
- For the designer, unchanged: F21, F22; the packaged-build gate that closes phase 3.
- The whole `check_all -Full` has not run in one pass with the fixed scripts (its two failing gates were rerun alone).

### Next step
*"When the designer asks for a package, the packaged-build gate (`py tools/packaged_bench.py` and the tour on the package) to
close phase 3; otherwise start phase 4 per docs/11 (the save container, doc 08 §2)."*

---

## 2026-11-02 (29) - Phase 3 exit: the playthrough through the UI; finding F22

### Where the project is
- **Phase 3's exit criteria are met** (doc 11): `Paw.Playthrough` green through the UI's entry points, the tree's layout tests
  green since (25), the screenshot tour reviewed (28). The one gate left before phase 3 is closed is the **packaged-build gate**
  (CLAUDE.md: packaged gates run at the end of every phase), which needs a package: only on the designer's ask.
- **Nothing committed** in either repo. In the Godot repo this session wrote only `docs/pawpocalypse-port-findings.md` (F21,
  F22); the uncommitted F20 changes from (23) stand.

### What happened
1. **`Pawpocalypse.UI.Playthrough`** (`UI/PawPlaythroughTests.cpp`): one run on the real root layout, HUD and realm, every action
   through the UI: a clean slate; real kills (the cat on the nearest mouse, hands taken through the card pick) until the dock
   can buy; nothing bought with nothing banked; eight dock purchases (each one level, never below zero, power never backwards);
   the Realms page's travel; the five bosses in order by the Bait button (the biome behind a standing boss shut, the kill opens
   it, one stage each and two for the last); the ending with the last boss only (it holds the world, OK gives it back); the
   Reawaken's two presses (paid what the page promised, the multiplier up, the run cleared, the bosses kept, the time kept); the
   ceremony, then the spend step (74 nodes, each exactly its price, a refused press spends nothing); out to the starting biome
   (no screen left, the world runs, the tree kept); kills pay again; everything finite. Godot's ci/check_playthrough.gd is the
   model: invariants, never balance. Save / load joins in phase 4. The gates a real run takes hours to open (this cycle's
   Catnip, a boss's bait kills) are credited as the cheat overlay credits them; the kill blows on a boss land between steps.
2. **Finding F22** (from its first run): after a Reawaken the Realms page walks past every boss without baiting it. Godot's rule
   (`is_biome_unlocked` -> `_bosses_cleared_for_stage`) re-locks the passage, and its comment says why; but the Realms rows ask
   only `is_tier_unlocked` (the permanent stage) and the Catnip, and the button changes scene unasked. The port copies Godot
   (`PcHud RealmRow`); the playthrough pins both halves. Doc 01 §5 and the Godot repo's findings file.
3. Docs 11 (phase 3 status), 09 (the playthrough row), 00. KB G/03 §6 (a rule tested on the state layer, bypassed by the UI).

### Verified
- `Pawpocalypse.UI.Playthrough`: PASS (upgrades 8, cards taken 2, nodes 74, Stardust paid 185405, kills 104).
- Sabotages `playthrough_upgrades_survive`, `playthrough_passage_kept`, `playthrough_payout_short`, `playthrough_nodes_free`:
  4/4 CAUGHT, clean PASS.
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102f.txt`; UI now 37 tests). UBT: Succeeded.

### Open
- **For the designer**: F21 (the job picker's and victory screen's look), F22 (the Realms page and passage); the packaged-build
  gate that closes phase 3 (ask for a package when ready).
- Nightly `-Full` not run since (23).

### Next step
*"Run the nightly `check_all -Full` (~3 h, editor closed). Then, when the designer asks for a package, the packaged-build gate
(`py tools/packaged_bench.py` and the tour on the package) to close phase 3; otherwise start phase 4 per docs/11 (the save
container, doc 08 §2)."*

---

## 2026-11-02 (28) - Phase 3 exit: the screenshot tour against Godot; two port bugs fixed from it

### Where the project is
- Phase 3 (docs/11): every screen built, and now looked at side by side with Godot. Left for phase 3's exit: `Paw.Playthrough`
  through the UI entry points. The only differences left on screen are phase 6's (art, fonts, tab icons, tutorial, Boba).
- **Nothing committed** in either repo. In the Godot repo this session wrote only `docs/pawpocalypse-port-findings.md` (F21, the
  one file the port writes there); the uncommitted F20 changes from (23) stand.

### What happened
1. **`py tools/godot_tour.py`** (new): the same steps and state in both games (new game, tier_1, 1000 Catnip; six tabs; tree
   browse; a card hand; a Reawaken with three jobs held: ceremony, spend step, job picker; the ending at 4200 s). Godot:
   `tools/godot_fixtures/tour_probe.gd` in the sandboxed scratch copy, rendered (opengl3). Unreal: the `paw.*` steps, plus new
   `paw.Wait <frames>` and `paw.Take` (through the card pick screen, as its button does; the realm's ChooseCard alone left the
   screen up and every later shot under it), with `t.MaxFPS 60` (the editor build ran ~500 fps uncapped, so frame waits were
   0.06 s). Shots pair in `build/tour/compare/<shot>.png`.
2. **The dims are right**: sampled on the dock's paper under each (same colour in both games), card pick and job picker (72, 64,
   79) vs (72, 64, 80), victory (42, 39, 45) both. The "dims read lighter" open item since (24) is closed: only the missing field
   art made them look lighter.
3. **Port bug fixed: the job picker's look.** Godot's JobPicker is a CanvasLayer under the HUD, like the victory screen, so it
   draws in Godot's DEFAULT theme: a translucent dark panel (radius 3, no frame, no margins), stock grey buttons, a white 16 px
   note, the INK title dark on dark. Measured by the `victory` fixture (`jp.*` keys in `victory_look_godot.txt`); new token
   `GODOT_PANEL`; `UPawJobPicker` rows `SetEngineLook`. The title being near-invisible is clearly not what job_picker.gd meant
   (it asks for `WindowPanel`): **finding F21** for the designer (doc 01 §5 and the Godot repo's findings file), ported as drawn.
4. **Port bug fixed: the Realms auto-travel box** was always shown; Godot hides it until the tree's Auto-Travel node is owned
   (`auto_travel_check.visible = GameState.owns_auto_travel()`). `RefreshRealms` now does the same (`Pc::OwnsAutoTravel`).
5. Docs 05 §6.3 (the picker's look, the tour), 11, 00, CLAUDE.md (the tour command). KB U/18 §4 (uncapped editor `-game` breaks
   frame-paced tours; a tour step must use the button's path; measure overlays on a shared colour), INDEX row.

### Verified
- UI automation 36 PASS (`Hud.Realms`: the box hidden, then shown once owned and in the column; `JobPicker.Asks`: stock buttons).
- Sabotages `jobpick_game_look`, `hud_auto_travel_always_shown`: CAUGHT, clean PASS.
- `make_godot_fixtures.py --only victory --check`: byte for byte; `gen_paw_tokens.py --check`: current.
- The tour: 12 of 12 pairs, no `LogPaw: Warning` in the Unreal log. Reviewed: tabs, tree browse / spend, card pick, ceremony,
  job picker, victory.
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102e.txt`). UBT: Succeeded.

### Open
- **F21** (designer): should the job picker and the victory screen wear the game theme? Until then both are ported as Godot
  draws them.
- Godot's tour window came out 1904x1001 despite `--fullscreen` (the compare sheet scales both to 960x540, so it is slightly
  squashed); Godot's own `tools/screen_shots.sh` says fullscreen gives 1920x1080 there. Not looked into.
- The victory and picker selection ring also shows on mouse hover (Common UI hovered = selected); Godot rings focus only.
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 exit per docs/11: `Paw.Playthrough` (reset, earn, buy, travel, kill bosses, Reawaken, buy nodes, the ending) through the
UI entry points, as an engine automation test; then the nightly `-Full`; ask the designer about F21."*

---

## 2026-11-02 (27) - Phase 3 slice 7: the victory screen; numbers printed Godot's way

### Where the project is
- Phase 3 (docs/11): every screen in its steps is built (the Unicorn ceremony waits for the tree's art, phase 6). The ending
  comes up when the last boss falls. Left for phase 3's exit: the screenshot tour against Godot and `Paw.Playthrough`.
- **Designer decision this session (2026-11-02): "Match godot number".** Numbers are now printed as Godot prints them:
  ungrouped, a '.' point in every language ("+125000 Stardust", not "+125,000").
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand; this session wrote nothing there).

### What happened
1. **Fixture `victory`** (`tools/godot_fixtures/victory_fixture.gd`): the REAL hud.tscn (which builds VictoryScreen and connects
   it to `boss_defeated`), 3,000 random ledgers: `format_duration`, `mark_boss_defeated` (signal, new, the clock), whether the
   ending came up, its line, the pause flag before / up / after OK. It also prints the screen's resolved LOOK, which the runner
   saves to `Tests/Data/victory_look_godot.txt`. Finding: the screen is its own CanvasLayer outside the HUD's themed Root, so
   **Godot draws it in the engine's DEFAULT theme** (white 16 px Open Sans SemiBold labels, the stock grey button).
2. **PawCore**: `PcHud DurationParts` (format_duration), `PcProgress RaisesVictory` (boss_defeated emitted AND the last boss
   realm), the field's `FieldFlag::Victory` on `BossDefeated`. `PcVictoryTests.cpp`: all 1,000 rows equal (all 3,000 in the
   long test), coverage asserted; a field test (first clear, first rematch of a cycle: up; already cleared this cycle, another
   boss: not).
3. **`UPawVictoryScreen`** on the **Modal** stack (pause holder "victory"; Godot's layer 128 is over the card pick's 100):
   dim (eats clicks), headline, flavour, "Cleared in {time}" (`PawText::Duration`), OK (focused, no guard, no Back). The HUD
   raises it from the field event with `FinalBossSeconds` (`ShowVictory`, never twice). Look: `PawTokens::GODOT_*` generated
   from the look file (`gen_paw_tokens.py` LOOK_COLOURS), `VICTORY_DIM` from the script's literal (CODE_COLOURS),
   `UPawButton::SetEngineLook` / `UPawEngineButtonStyle`. Strings `VICTORY_*`, `DURATION_hms/ms`.
4. **Numbers**: `PawText::Count` is culture-invariant ungrouped digits; new `TwoDigits` ("%02d") and `Fixed` ("%.Nf" through
   `Pc::FixedLikeGodot`, Windows Godot's half-up rounding); the HUD's boss HP %, boss timer, realm reward multiplier and the
   Reawaken multipliers moved off `FText::AsNumber`. L0 `ui-rules` now fails on `FText::AsNumber/AsPercent/AsCurrency` in UI
   code (self-test plant added). Docs 05 §6.3 and 08 updated.
5. Tour step `paw.Victory [seconds]`.
6. **KB**: G/07 (a CanvasLayer breaks theme inheritance: code-built screens get the engine's default theme), G/06 §3 (Godot's
   built-in font is Open Sans SemiBold), U/20 §1 (`FText::AsNumber` groups; how to print culture-invariant), INDEX rows.

### Verified
- PawCore L1: 150 test cases PASS (victory fixture 1,000/1,000; coverage: hours 365, negative clocks 41, shown 449, from a
  rematch 120, silent last-boss kills 121, other bosses 346).
- UI automation: 36 PASS (+3: `Victory.Ending`, `.OnlyFinal`, `.Numbers`).
- Sabotages: PawCore 8/8 CAUGHT (`duration_*`, `victory_*`); game module 15/15 CAUGHT (`victory_*`, `numbers_*`,
  `duration_minutes_unpadded`), clean PASS (`build/sabotage_victory.txt`).
- Screenshot (editor build, `-game`): `Saved/PawShots/victory.png`: the dim, THE VOID IS QUIET, the two flavour lines,
  "Cleared in 1h 10m 00s", the grey OK, Godot's layout.
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102d.txt`); `check_pawcore_static.py --self-test`: every
  plant CAUGHT, the new `ui-rules: engine number format` included. UBT: Succeeded.

### Open
- The dims (card pick, job picker, now the victory screen) read lighter than their alpha should: still unchecked against Godot
  side by side (likely blending; the tour will tell).
- The engine font (Open Sans SemiBold) waits for the font phase; the victory OK shows its ring on mouse hover too (Common UI's
  hovered = selected); no Boba cameo (phase 6).
- Earlier placeholders stand (window plate, job_taken save, hat ring; doc 05 §6.3).
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 exit per docs/11: the screenshot tour against Godot: the card pick, the tree, the job picker, the victory screen and
every tab, side by side with Godot shots (and settle why the dims read lighter); then `Paw.Playthrough` through the UI entry
points; then the nightly `-Full`."*

---

## 2026-11-02 (26) - Phase 3 slice 6: the Reawaken ceremony and the job picker

### Where the project is
- Phase 3 (docs/11): a Reawaken now runs Godot's whole order: the payout read, the reset, **the ceremony**, the tree's spend
  step, **the job picker** (when `should_ask`), then the travel to the starting biome. Left in phase 3: the victory screen.
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand; this session wrote nothing there).

### What happened
1. **`PcHud JobPicker` / `PickJob`** (PawCore): what job_picker.gd offers (the held jobs that have data, by id; "(last run)" and
   the default row; the baseline last; the second ask under Union Rules without the primary, with its share) and what a press
   does (lock, ask again, or lock the second slot and mark the run asked). New fixture **`job_picker`**
   (`tools/godot_fixtures/job_picker_fixture.gd`): the REAL JobPicker opened on 3,000 random states, its rows read back off the
   buttons, a press through `_pick`, and the reopened list when it asks again: **all equal** (1,000 L1 rows, all 3,000 in the
   long test). Only one test-side fix: with nothing but the baseline offered, the first blurb is the baseline's own.
2. **`UPawReawakenCeremony`** (reawaken_ceremony.gd): the HUD's top layer, hit-test invisible; flash + "REAWAKENING" in (0.35 s),
   the payout (0.25 s), hold 1.1 s, all out (0.45 s), one sine in-out tween (`AlphasAt`). The field runs under it (Godot does
   not pause for it), but its clock stops while anything holds the field (Godot's bound tween). The payout is read BEFORE the
   reset (Second Helping makes it differ from what lands). Its colours are scene literals: `tools/gen_paw_tokens.py` now also
   generates `CEREMONY_*` from reawaken_ceremony.tscn (SCENE_COLOURS; `SrgbF` = Godot's srgb_to_linear).
3. **`UPawJobPicker`** (job_picker.gd) on the **Modal** stack (pause holder "job_picker"; `SetModalStack` from the player
   controller): title, note, rows + blurbs (pooled), the card pick's guard (restarted by the second ask), one wired column,
   opens on last run's job. `UPawRealmSubsystem::PickJob` rebuilds the stats.
4. **The HUD**: `DoReawaken` -> ceremony; its end -> `OpenSpendStepWhenFree` (waits while anything is on the Menu stack, never
   skips); the tree's leave -> `ReturnToStartingBiome` (the picker when `ShouldAskForJob`, its answer -> `RefreshJobs` + travel).
5. `ST_UI` + 2 `CEREMONY_` and 8 `JOBPICK_` lines; L0 key prefixes + both. Doc 05 §6.3 has the design and the differences kept.
6. Screenshot tour steps `paw.Reawaken [jobs]` (the HUD's two presses on a run made worth it) and `paw.Leave`.
7. **Tests**: UI 33 (+6: `Ceremony.Tween/.Plays/.Waits`, `JobPicker.Asks/.Union/.NoJobs`; `Hud.Reawaken` and `Tree.Spend` now
   play the ceremony out); L1 147 (+3); PawCore sabotages +9 (`picker_*`, all CAUGHT after one test added: no last-run row ->
   first row); game-module sabotages +25 (`ceremony_*`, `jobpick_*`, all CAUGHT first time); `cards_choose_keeps_stale_stats`
   made unique again (PickJob repeats its line).
8. **KB**: G/03 §6 (a UI script's decisions pinned against the real UI node; a bound tween freezes under a pausing modal).

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102c.txt`); L0 again after the tour steps. UBT: Succeeded.
- `py tools/sabotage_realm.py` (the 25 new + the 2 touched): 27/27 CAUGHT, clean PASS (`build/sabotage_realm_1102c.txt`).
  `py tools/sabotage_pawcore.py --only picker_*`: 9/9 CAUGHT.
- Screenshots (editor build, `-game`): `Saved/PawShots/ceremony_payout.png` (REAWAKENING, "Reawaken 1 / +125,000 Stardust"),
  `w1.png` (the fade out, the field showing through), `spend_after_ceremony.png`, `job_picker.png` (three jobs, "Daredevil
  (last run)", "No job" last, over the dimmed HUD).

### Open
- **Number grouping**: Godot formats `{n}` ints ungrouped ("+125000 Stardust", "Stardust: 125000"); the port's `PawText::Count`
  is `FText::AsNumber` (doc 08's mapping), so it shows "125,000". Visible on the ceremony, the tree's balance and costs, the Jobs
  page. A port bug or the intended localisation? A designer call (doc 08 §4), not changed here.
- The dims (card pick, job picker) read lighter than Color(SKY base, 0.84) should: still unchecked against Godot side by side.
- Placeholders: the window plate is a rounded box (ui_window.svg's nine-slice, phase 6); job_taken's save (phase 4) and the
  hat's ring (phase 6). The ceremony sits under the Menu stack: a tree browsed from under the flash draws over it (doc 05 §6.3).
- Not covered by tests: painting (screenshots are the check), the guard through real Slate key events (phase 5).
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 per docs/11: the victory screen (victory_screen.gd, on the HUD: show_victory / dismiss). There is NO achievement
toast in Godot (hud.gd _on_achievement_earned only refreshes the list and the tab badge; the earn sound is phase 6), so the
earlier handoffs' "achievement toast" step is dropped. Then ask the designer about number grouping (Open); side-by-side screenshots of the card pick, the tree and the job picker
against Godot; then the nightly `-Full`."*

---

## 2026-11-02 (25) - Phase 3 slice 5: the skill tree screen, browse and the Reawaken spend step

### Where the project is
- Phase 3 (docs/11): every dock page, the card pick and the **skill tree** (browse, and the spend step after a Reawaken) are built.
  Left in phase 3: the Reawaken ceremony and the job picker around the spend step, the achievement toast, the victory screen.
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand; this session wrote nothing there).

### What happened
1. **The tree's words and colours are generated** (`tools/gen_tree_tables.py --godot ...`, L0 checks them current, its self-test
   plants each): from skill_tree_ui.gd and skill_tree_connector_layer.gd -> `Content/.../ST_Tree.csv` (87 captions =
   BRANCH_LABELS, 87 blurbs, 79 effect phrases from `_describe`'s cases incl. per-branch and tr_n ones, 4 channel phrases),
   `PawCore/Public/PcTreeWording.h` (which phrase and which GDScript number expression each effect uses), `UI/PawTreeTables.h`
   (BRANCH_COLORS, RAINBOW / PULSE lists, the tooltip's 5 bbcode colours, DIM / SHUT, SHINE_COLOR). It fails on anything in
   `_describe` it cannot read.
2. **`PcTreeText`** (PawCore): the effect line's phrases at a level, and the tooltip's decisions (Now / Next / Cost; Closed by a
   rival, needs the parent, the Reawaken gate, browse-only, too poor; the buy prompt). **`PcFormat`** gains Godot's "%.Nf"
   (17 digits then half up, any N), `roundi`, `int`. New fixture `tree_text` (`tools/godot_fixtures/tree_text_fixture.gd`):
   every branch at every level, and 2,000 values x the 9 number expressions: **all equal** after one fix (Godot prints "-0").
3. **`UPawSkillTreeScreen`** (doc 05 §6.2 has the design): the canvas, painted links and rings, node frames by state, halos, shine,
   captions, hub, pan / zoom / fit, Q / X next affordable, the tooltip; browse (T / pad Y from the HUD, read-only) and the spend
   step (`UPawHudScreen::DoReawaken` opens it; leaving asks once; then `TravelToStartingBiome`; with no Menu stack it travels at
   once, as Godot with no tree). `ST_UI` + 24 `TREE_` lines and 6 `PROMPT_` placeholders (key names until phase 5's glyphs).
4. **The screenshot tour found a crash the tests could not**: `UPawTreeLinks::RingPath` added an array's own element to it
   (`Pts.Add(Pts[0])`), an assert the moment the tree painted; automation tests never paint. Fixed; KB U/24.
5. `paw.Tree [spend]` and `paw.Hand` (queued, with `paw.Shot`): the tree and a dealt hand for screenshot tours.
6. **Tests**: UI 27 (+7 `Pawpocalypse.UI.Tree.*`); L1 145 (+3); game-module sabotages 162 (+32 `tree_*`, all CAUGHT after a second
   round: the cheapest node was also the first trunk, and a browse's travel was guarded twice and invisible from the starting
   biome; one older sabotage repointed); PawCore sabotages 387 (+14, all CAUGHT; one older one made unique again).
7. **KB**: U/24 (a widget passes headless tests and crashes on first paint: `TArray::Add` of its own element).

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102b.txt`). UBT editor build: Succeeded.
- Screenshots (editor build, `-game`): `Saved/PawShots/tree_browse.png`, `tree_spend.png` (READY halos, Continue), `card_pick.png`.
  The tree matches the Godot shape (six limbs, rings, the Unicorn's pulse, job rings).

### Open
- **Look at the screenshots against Godot**: the card pick's dim reads lighter than Color(SKY base, 0.84) should (Slate may blend
  translucency differently); at the fit zoom (~0.37) the captions are small, as in Godot.
- Placeholders (doc 05 §6.2): prompt key names, flat sky, no icons, halo without shadow, no hover lighten, no READY jitter, no
  Unicorn ceremony; the ceremony and job picker around the spend step.
- Not covered by tests: painting (screenshots are the check), hover, the stick / triggers, the follow glide.
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 per docs/11: the Reawaken ceremony (reawaken_ceremony.gd, before the spend step) and the job picker (after it, when
JobPicker.should_ask()), then the achievement toast; side-by-side screenshots of the card pick and the tree against Godot; then the
nightly `-Full`."*

---

## 2026-11-02 (24) - Phase 3 slice 4: the card pick screen; slice 5 begun: the skill tree's layout in PawCore

### Where the project is
- Phase 3 (docs/11): every dock page and now the **card pick** are built; the **skill tree**'s PawCore half (`PcTreeLayout`) is done
  and exact against Godot; its screen is next.
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand: `hud.gd`, `tier_controller.gd` (F20),
  `card_manager.gd` (F1), `docs/pawpocalypse-port-findings.md`; this session wrote nothing there).

### What happened
1. **The card pick** (`UI/PawCardPickScreen.h/.cpp`, `catches_pick_ui.gd`): the Menu stack's first screen. Cards (`UPawCardFace`: a
   never-focusable Common button = the whole-card click) with the flavour band and word, NEW / "LEVEL n of max", the description at
   the level taking it gives, the red frame and warning for a cost, Take, Banish while a banish is held; Reroll with its count;
   Dilated's bigger hands; no close, no Back. The guard (0.4 s after every deal; a key press must begin strictly after the deal:
   Godot's `>=` let the press that dealt the hand through on the same millisecond, its comment says the intent is this). Pad rows
   Take / Banish / Reroll by `PawUi::WireRows` (the port of `MenuFocus.wire`). Doc 05 §6.1 slice 4 has the whole design.
2. **The HUD opens it** (`OpenCardPickIfDealt`, every frame while the progress holds a hand; the stack's list is the one guard against
   a second); the player controller hands the HUD its Menu stack. **Hold-to-buy stops** under it.
3. **The realm's pause holders** (`UPawRealmSubsystem::SetPauseHold(FName, bool)`; `SetPaused` = "manual"): one screen closing
   never resumes the field under another; a restarted field stays held. **The click that takes a card is never a Pounce**:
   `SetPounceDown` swallows a press while held and until the button is let go (Godot pounces from `_unhandled_input`; the port polls).
   `RerollHand` / `BanishFromHand` (each deal its own roll stream off the field's seed).
4. The root layout's stacks have **no transition** now (KB U/08 rule 6): a screen is up the frame it is added.
5. `ST_UI` + 17 `PICK_` lines; L0's key prefixes + `PICK`.
6. **`PcTreeLayout`** (`Source/PawCore/Public/PcTreeLayout.h`): branch order, the hex seating (`_seat`), the limb push, separation,
   normalising, bounds, canvas, fit zoom, `nav_pick`, node states (LOCKED / READY / OWNED), links (owned / fork / shut), cheapest
   and next affordable, has-affordable. New fixture `tree_layout` (`tools/godot_fixtures/tree_layout_fixture.gd`: the real
   `skill_tree_ui.gd` instanced without its scene, the shipping tree + 400 random trees): **17,412 seats, 0 px off; every pad pick
   equal; the same 10 crowded-limb fallbacks**. The first port in double got 4,496 picks and 147 seats wrong: Godot's `Vector2` is
   32-bit and the lattice puts neighbours exactly on the 60 degree cone edge, so the layout now emulates 32-bit `Vector2` arithmetic
   per operation (rounding through `frexp`/`ldexp`; no `float` in PawCore). Godot's separation pass never moves a hex seat (240 px
   pitch > 196 px gap): exposed as `SeparateTreeNodes` and tested on a crafted layout.
7. **Tests**: UI 20 (+8 `Pawpocalypse.UI.CardPick.*`); L1 142 cases (+4); game-module sabotages 131 (+34, all CAUGHT); PawCore
   sabotages 373 (+24, all CAUGHT after a second round: 3 did not compile, 4 were missed, each fixed by a test that can see it).
8. **KB**: U/08 §3.4 (an activatable stack in an automation test needs `TakeWidget()` first; the 0.4 s default transition;
   `SetFocus` without a local player is an ERROR; nested Common buttons), G/07 (Godot's 32-bit `Vector2` in a port), INDEX symptoms.

### Verified
- Everyday `check_all` (2026-11-02, after both slices): every gate PASS (`build/check_all/_everyday_1102.txt`). UBT editor build with the final sources: Succeeded.
- `py tools/sabotage_realm.py` (the 34 `pick_*` / `realm_*`): 34/34 CAUGHT, clean PASS. `py tools/sabotage_pawcore.py` (the 24
  `layout_*` / `nav_*` / tree jumps): 24/24 CAUGHT, clean PASS.
- No screenshot of the card pick yet (no package, no `-game` run this session).

### Open
- The card pick is not seen on screen yet: a `-game` screenshot (a hand via `paw.Cheat.*`) belongs to the next session's tour.
- Not covered by tests: the card's hover / focus lift, the guard through real Slate key events (phase 5), hold-to-buy stopping under
  the pick. Card art waits for phase 6.
- The skill tree's names, blurbs, effect words and per-branch colours live in Godot CODE (`BRANCH_LABELS`, `BRANCH_BLURBS`,
  `EFFECT_DESCRIPTIONS`, `BRANCH_COLORS` in `skill_tree_ui.gd`): to be GENERATED (string tables, a colour header), not hand-copied.
- Doc 05 §6.2 described the tree as one lattice: it is six limbs (`TRUNK_WHEEL`) on hex lattices, 87 nodes; corrected there.
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 slice 5 per docs/11 and doc 05 §6.2: the skill tree SCREEN on `PcTreeLayout`. First generate `BRANCH_LABELS` /
`BRANCH_BLURBS` / `EFFECT_DESCRIPTIONS` into the string tables and `BRANCH_COLORS` into a header (from `skill_tree_ui.gd`, L0 checks
them current); then `UPawSkillTreeScreen` on the Menu stack with pause holder "tree": opaque sky, canvas with pan / zoom (fit on
open, wheel and buttons, RT / LT), links and rainbow / job rings painted under the nodes, node frames by state with the READY halo,
counters, captions, Stardust line; browse (T / pad Y, read-only, X) and the Reawaken spend step (Continue, the two-press leave,
buy through `PurchaseNextBranchLevel`, then travel: ceremony and job picker later); Q / X next affordable; the tooltip; its
`Pawpocalypse.UI.Tree.*` tests and sabotages; screenshot tour incl. the card pick."*

---

## 2026-11-02 (23) - The night: Territory wins (F20, Godot first); phase 3 slice 3, every dock page built

### Where the project is
- Phase 3 (docs/11): all six dock pages are live (Upgrades, Reawaken, Realms, Catches, Achievements, Jobs); the realm earns
  achievements; the card pick's decisions are in PawCore, its screen is not built. Night list:
  `docs/WORKLIST-NIGHT-2026-11-01.md` (closed).
- **Nothing committed** in either repo. **The Godot repo has two new uncommitted changes for the designer**:
  `scripts/systems/hud.gd` and `scripts/systems/tier_controller.gd` (F20, below), beside the earlier `card_manager.gd` (F1) and
  `docs/pawpocalypse-port-findings.md`.

### What happened
1. **Territory wins (designer, 2026-11-01).** Measured first on the REAL Godot HUD (new `tools/check_godot_reawaken_travel.py`:
   a sandboxed copy, a watcher node on `root`, four Reawakens): with Territory deeper than the biome you Reawaken in, its
   1.1 s timer changed scene during the 1.9 s ceremony and **the spend step (skill tree, job picker) never opened**; at or
   above it, Territory was ignored and the run went to tier 1. That is **F20** (doc 01 §5, Godot findings, Fixed). Godot
   fixed: `hud.gd` `_return_to_starting_biome()` / `_travel_to_starting_biome()` take the run to `get_starting_biome_index()`
   at the end of the spend step; the timer travel removed from `tier_controller.gd`. Godot's ci scripts / hud / damage /
   playthrough / mirror / tutorial pass; the check passes 4/4 and fails 3/4 on Godot's HEAD (`--head`). Port:
   `Pc::ReawakenTravelIndex` (L1, 3 sabotages), `UPawHudScreen::DoReawaken`; the realm's deferred travel deleted
   (`.Realm.ReawakenTravel` now asserts the realm never moves; `.UI.Hud.Reawaken` checks deeper, shallower, same).
2. **The Catches page**: every card in deck order as a flat, selectable row: held = "<name>  (Lv n/max)" over its description
   with `{v}`/`{c}` filled (`PcHud` `CardNumber` = `CardData._fmt`); not held = what stands in the way (`Pc::CardUnlockHint`,
   `unlock_hint()`'s gate order) or "Not taken yet."; the summary; the Auto-pick box (with Auto-Card; writes the realm).
   `UPawButton` gained a detail line, left alignment and a flat style.
3. **The Achievements page**: summary, Claim all, one row per achievement (mark, name, description or progress, Claim with its
   amount). **Found by the screenshot tour: nothing in the game earned achievements** (Godot re-scans on every kill, currency
   change, purchase...): `FPawRealmSim` now re-scans after every fixed step, `UPawRealmSubsystem` after a progress change, a
   summon and a Reawaken; earned ids queue in `EarnedAchievements` (the toast is not built).
4. **The Jobs page**: worn / owned / no node / closed by a rival / beat a boss / Reawaken n times / ready at its price; the
   state button is a statement, always locked (the pad still reads it). The tab stays hidden until a job is held (as Godot).
5. **Stretch**: the card pick's decisions, `PcHud` `CardPick` (level, each card's next level, value, price, NEW, the cost
   frame, foresight, rerolls, banish).
6. **Strings**: `ST_Data` gains CARD / ACH / JOB names and descriptions and SKILL names (`gen_data_strings.py`; 277 rows);
   `ST_UI` + 34 lines (CATCHES_, AWARDS_, JOBS_; L0's key prefixes widened). `PawText::DataField`, `PawText::CardNumber`.
7. **Tests**: L1 138 cases (+6); PawCore sabotages 349 (+42; first MISSED twice: a near-whole card number, and a flavour
   that was the default: both tests tightened, then CAUGHT); UI 12 tests (+3); game-module sabotages 97 (+19, all CAUGHT).
8. **KB**: Godot/01 §1 + G/07 + INDEX (one scene change per modal flow: a timer scene change frees the modals; how to test it
   headless).

### Verified
- Everyday `check_all`: every gate PASS (00:24). Nightly `-Full` (00:33-05:22): 20 of 21 gates PASS (PawCore sabotage 349/349 CAUGHT, L1 long suite, pawsim parity sweep, Godot fixtures, Godot repo untouched); **Realm sabotage FAILED** on two counts, both handled the same morning: (a) `hud_reawaken_realm_not_told` MISSED: since the HUD always travels after a Reawaken, the travel rebuilds the field and hid what `OnReawakened` does; the one effect only the signal has is earning the Reawaken's achievements before any step, so `.Hud.Reawaken` now asserts Nine Lives (`reawaken_1`) is earned by the press itself; CAUGHT. (b) the clean rerun of `Pawpocalypse.Realm.ReawakenTravel` failed once and passed 6 times alone (fixed seed, own process): cause unknown, because `run_automation` overwrites its log per filter; `sabotage_realm.py` now keeps a failing clean run's output (`build/sabotage_realm_clean_fail_*.log`) and prints its errors. Full realm sabotage rerun 06:07: **97/97 CAUGHT, clean PASS**; everyday `check_all` after it: every gate PASS. Watch for (b) recurring: if it does, the kept log names it.
- Screenshots from the editor build (`-game`; no package): `Saved/PawShots/catches.png`, `awards.png` (3 earned with their
  Claims after `paw.Cheat.AdvanceStage`), both as intended.

### Open
- Godot: commit `hud.gd` + `tier_controller.gd` (F20) when reviewed; F13, F14, F16-F19 still wait for decisions.
- Not built: the card pick screen, the skill tree, the ceremony, the job picker (Godot's Reawaken runs them before the
  travel), the achievement toast, tooltips, the Reawaken banner and glow, the travel list's remembered scroll; hold-to-buy
  untested. The Achievements tab's count spills past its text tab (goes with phase 6's icons). The skill tree screen will
  need `BRANCH_LABELS` (one differs from the data: `overkill` = "Splash").
- The Jobs page was not screenshotted (no cheat grants a job; its tab is hidden without one).

### Next step
*"Phase 3 slice 4 per docs/11: the card pick screen (a Modal-stack Common UI screen over the field; `PcHud` `CardPick` is
done): three cards with Take, Banish when held, Reroll, the guard against an accidental confirm, pause while up; its
Pawpocalypse.UI test and sabotages; then the skill tree."*

---

## 2026-11-01 (22) - Phase 3 slice 2: the Realms and Reawaken pages; rows locked, not disabled

### Where the project is
- Phase 3 (docs/11): the dock now has three live pages (Upgrades, Realms, Reawaken); Catches, Achievements and Jobs show their
  titles only. A full Reawaken can be played from the HUD (two presses, then back to the first biome or Territory's).
- **Nothing committed** in either repo.

### What happened
1. **`PcHud`** + the Realms rows (here / travel / boss, cleared / Catnip owed / locked; Cartography; new), the stage line, the
   Bait button (elsewhere / boss here / bait owed / Catnip short / ready; enabled; calling) and the Reawaken page (progress,
   payout, total, multipliers, the button's four kinds, the Unicorn line). 4 L1 tests; 15 sabotages, all caught after the
   tests were tightened (4 first missed: a stage-locked biome that still owed Catnip, the boss up with the next bait earned,
   a locked bait hiding the enabled flag, a total checked only with nothing to add).
2. **The pages** (`UPawHudScreen`): Realms (stage line, hint, Bait, auto-travel box, twelve rows in a scroll box, a press
   travels) and Reawaken (blurb, progress bar, payout with its Stardust disc, total, multiplier, Unicorn line, the button, the
   two-press confirm moving the selection to Cancel, Cancel, "Ask before Reawakening"; a tab change disarms). The reset:
   `Pc::Reawaken`, the realm's `OnReawakened`, then the first biome unless Territory's travel is pending. ST_UI + 32 lines.
3. **Rows LOCKED, not disabled** (`UPawButton::SetLocked`): slice 1 disabled unaffordable rows, and Slate never focuses a
   disabled widget (engine source: `SlateApplication.cpp`), so a pad could not move onto them; Godot's navigator selects them
   ("the player reads an unaffordable row"). Common UI's interaction off + a locked style that keeps the focus ring. Every
   page's navigation column includes its locked rows (`PageColumn`), and focus is remembered per page.
4. **Tests**: `Pawpocalypse.UI.Hud.Realms`, `.Hud.Reawaken` (9 UI tests); 15 new game-module sabotages + 2 retargeted, all
   caught (2 first missed: a total that "1.0M" rounding hid; the realm's `OnReawakened`, invisible because the return trip
   always re-enters the first biome: a Territory case now proves it). L0 key prefixes widened. `paw.Tab <n>` (dev) for tours.
5. **Found, matching Godot**: the reset clears the current biome, so the return to the first biome runs from inside it too
   (Godot's `_travel_to_first_biome` reloads the scene then).
6. **Docs**: 00, 05 §6.1, 09 (L1 132 cases, L3 UI 9 tests, sabotages 307 PawCore / 78 realm), 11. **KB**: U/08 §3.3 (a row
   that must be selectable but not pressed: interaction off + a locked style, never disabled; `bLocked` is taken).

### Verified
- `check_all` everyday: every gate PASS. PcHud sabotages 35/35; HUD game-module sabotages 33/33.
- Packaged screenshots `build/package/Windows/Pawpocalypse/Saved/PawShots/realms.png`, `reawaken.png`: both pages as intended.

### Open
- **Godot question (not checked)**: after a Reawaken with Territory, Godot both schedules the deeper travel (a SceneTree timer,
  which runs while paused) and, when the tree closes, travels to the first biome. Which one wins with the tree open needs a
  Godot run; until the tree exists the port does Territory's travel and skips the first-biome return.
- Not built: the ceremony, the skill tree, the job picker (the Reawaken sequence will route through them), the Reawaken
  banner and glow, the travel list's remembered scroll, tooltips; hold-to-buy still untested.
- The -Full nightly was not rerun (the realm sabotage gate is now ~100 min).

### Next step
*"Phase 3 slice 3 per docs/11: the Catches page (this run's cards, the auto-pick box), the Achievements page (rows, Claim,
Claim all, the count on the tab) and the Jobs page. Decisions in PcHud first (L1 + sabotages), then the pages, then
Pawpocalypse.UI tests and their sabotages, then a screenshot tour (`paw.Tab`, `paw.Shot`)."*

---

## 2026-11-01 (21) - Phase 3 slice 1: the HUD and the dock's Upgrades page

### Where the project is
- **Phase 3 started** (docs/11). L_Realm now opens with the HUD: the dock on the left (six tabs, badges, the Upgrades page
  with live prices, bulk buy and hold-to-buy), the status row (Catnip, Stardust, the wave line, the zone plate), the Catches XP
  bar, the Pounce clock and the boss panel; the field is measured beside the dock and a click on the dock never Pounces.
- **Nothing committed** in either repo (Godot repo unchanged this session beyond entry (20)'s F19).

### What happened
1. **`PcHud`** (PawCore): the decisions of hud.gd and tier_controller.gd's boss panel, no strings: shop rows (hidden,
   kitten-locked, MAX, price, bulk; enabled), the buy multipliers and the press, the Catches bar within the level, the zone plate
   (bait over ready over the Catnip gate), the wave line, the Pounce clock, the boss panel, tab badges and visibility. 7 L1
   tests; 20 sabotages, all caught (three first failed to BUILD: a sabotage left a parameter unused; and one was missed: no test
   had bait and a new biome at once; a case was added).
2. **Text and look**: `ST_UI.csv` (Godot's English for the HUD) and `ST_Data.csv` (generated, `tools/gen_data_strings.py`),
   registered by the game module's `StartupModule`, staged loose (`DefaultGame.ini`); `PawText` accessors. `UI/PawTokens.h`
   generated from Godot's `palette.gd` (`tools/gen_paw_tokens.py`); `PawUiConstants.h` (Godot's presentation constants, each with
   its source). L0 `ui-rules` (colour values, hard-coded text, keys named and used) with four planted violations caught; the
   `generated` gate checks the two new generated files.
3. **UI** (`Source/Pawpocalypse/UI`): `UPawRootLayout` (Game / Menu / Modal), `UPawHudScreen`, `UPawButton` (focus = a ring),
   `UPawPounceClock` (painted), `PawUiKit`; `APawPlayerController` creates the root and pushes the HUD where there is a realm
   view. The realm: `UPawRealmSubsystem::SetViewSize` / `SetDock` / `IsOverDock`; the view reports its size and ignores a
   mouse Pounce over the dock. `Build.cs`: the UI modules are permanent dependencies now (were listed as spike-only).
4. **Tests**: `Pawpocalypse.UI.*` (7: strings, readouts, press, dock, hit-test, navigation, boss) and 18 game-module sabotages
   (`hud_*`, `realm_dock_click_pounces`), all caught; `check_all` gains `L3 Pawpocalypse.UI`. `paw.Shot <name> [quit]` (dev):
   screenshots for tours.
5. **Seen in the packaged build** (screenshots): the first pass drew labels over the field as ink blocks (Slate's outline 5 is
   much heavier than Godot's); 2 reads right. Tab names did not fit; 13 px until the icons.
6. **Docs**: 00, 03 (`PcHud`, the HUD row, the dock-measured field), 05 §6.1 (built, placeholders), 08 §4 (the tables are
   live), 09 (L0, L1, L3 UI, sabotage counts), 11 (phase 3 status), CLAUDE.md (two generators). **KB**: U/08 §3.3 (Common UI
   header needing GameplayTags), U/09 §3 (no `UBorder::GetBrush`; outline sizes), U/20 §2 (CSV tables at startup, multi-line
   values), U/19 §2 (Git Bash mangles a `/Game/...` argument), INDEX rows.

### Verified
- `check_all` everyday: every gate PASS (L0 incl. ui-rules, L1 128 cases, mirror, fuzz, L3 Data, Realm, UI, Godot untouched).
  Sabotages: PcHud 20/20, HUD game module 18/18. L0 self-test: every planted violation caught.
- Packaged gate with the HUD: PASS, but the HUD costs: draw calls 13.6 -> 35.3 (budget 50), game thread p99 2.73 -> 3.12 ms
  (budget 4), render thread p99 8.8 -> 14.2 ms (uncapped, ~220 fps).

### For the designer
- Look at `build/package/Windows/Pawpocalypse/Saved/PawShots/hud_rich.png` (the HUD as it stands; placeholders: tab names
  instead of icons, discs instead of the currency icons and the badge star).
- Decide F19 (and F13-F18). Commits as before, plus `Source/Pawpocalypse/UI/`, `Source/PawCore/*/PcHud*`,
  `Content/Pawpocalypse/Localization/`, the controller, the realm subsystem and view changes, `Pawpocalypse.cpp`,
  `Pawpocalypse.Build.cs`, `Config/DefaultGame.ini`, `Dev/PawShot.cpp`, the new tools.

### Open
- Not built yet: the other five pages (titles only), the quick bar, tooltips, the fps readout, the tutorial overlay; the dock's
  keys are read raw until phase 5. Hold-to-buy is implemented but not tested (needs a real held press: the phase 5 input tour).
- The HUD's draw cost: try an invalidation box around the dock (KB U/09 §1) if the budget gets tight.
- The -Full nightly was not rerun; the realm sabotage gate is now ~80 min (63 sabotages).

### Next step
*"Phase 3 slice 2 per docs/11: the Realms page (twelve rows, gates, the current biome, Bait with its states, auto-travel box)
and the Reawaken page (progress, payout, multiplier, the two-press confirm with focus moved to Cancel). Decisions in PcHud first
(L1 + sabotages), then the pages in UPawHudScreen, then Pawpocalypse.UI tests and their sabotages."*

---

## 2026-11-01 (20) - Phase 2 closed: every card effect in a live realm; the packaged-build gate; F19

### Where the project is
- **Phase 2 DONE** (docs/11): every exit criterion met. **Next is phase 3**, the progression UI.
- **Nothing committed** in either repo. The Godot repo: only `docs/pawpocalypse-port-findings.md` changed by the port (F19
  added); `scripts/autoload/card_manager.gd` is the designer's own uncommitted Second Opinion fix of 2026-10-29 (untouched).

### What happened
1. **`Pawpocalypse.Realm.CardEffects`** (`Source/Pawpocalypse/Realm/PawCardEffectsTests.cpp`; the port of Godot's
   `ci/check_card_effects.gd`): a standalone game instance with its real subsystems; each card taken to its maximum level
   through the game's own entry, the realm run through `AdvanceFrame`. One case per effect, run twice on one seed (without the
   card, then with it); the consumer's mark must show only with the card: trail zones, pulses, slowed mice, rim damage on a
   frozen fat mouse, kitten blasts, the gilded kitten, Lone Hunter (and its kitten cost at the next sync, F14), chain damage
   beyond the rim, craters, Bloodlust's refunds, Peer Pressure's extra damage on the Pounce frame, golden and signature spawn
   counts, bombs, puddle length (exactly +10 s) and count, Bounty (5x the control's payout, Currency Find raised so rounding
   cannot fake it), Compound Interest (exactly the floored share of the bank), Scavenge, Dilated (+2 cards in the dealt hand),
   Spite/Not That One/Fixation held and spent on the hand the realm deals. A coverage check over every `CardEffect` (the 3
   with no card asserted cardless), Grudge Ledger's stats rebuilt on a take, a fixed seed replays exactly; F13 pinned by the
   blast's POSITION (off the field, as Godot has it). **`Pawpocalypse.Realm.EveryCard`**: the whole deck at its maximum at once
   for 900 frames (caps and finite state every frame, kills, pulses, bombs, Pounces, puddles; every cost read where it applies).
2. **Game entries**: `UPawRealmSubsystem::ChooseCard` (the pick UI's call: `PcCardDraw` `ChooseCard`, then the field's stats
   rebuilt, as Godot's `choose()`; kittens not re-synced, F14); `StartRealm(Seed)` starts a fresh sim (a second start had
   inherited the travel count that seeds each biome, so a fixed seed did not replay).
3. **Sabotages**: `tools/sabotage_realm.py` + 29 (the game's `ChooseCard` taking nothing or leaving stale stats, the sim kept
   across starts, each effect's consumer in PawCore broken one line at a time, F13 fixed, the mouse-speed cost): all CAUGHT,
   the clean build passing. The first run missed the sim-kept-across-starts one: the replay check was added for it.
4. **The packaged-build gate**: `paw.Bench` (`Dev/PawBench.cpp`, development builds) and `py tools/packaged_bench.py`.
   Packaged Development (UAT by hand, `build/package`, 387 MB; doc 10 §2). **The first packaged run failed**: L_Realm was not
   cooked. `DirectoriesToAlwaysCook` cooked every asset under `/Game/Pawpocalypse` except its maps (only the default map);
   fixed with `+MapsToCook` for L_Title and L_Realm in `Config/DefaultGame.ini`. Then PASS at 1280x800 (RTX 5070): 320 mice +
   20 kittens, game thread 1.40 ms mean / 2.73 ms p99 (budget 4), render thread 2.89 / 8.79 ms, GPU 0.40 / 0.43 ms, 13.6 draw
   calls (budget 50), the realm's steps + draw 0.08 ms; drawn == live every frame; screenshot checked (floor, mice, kittens,
   shadows, rings, HUD).
5. **Docs**: 00, 01 (F19), 03 (`ChooseCard`, the seeded start), 09 (the card rows, L5, sabotage counts, -Full timing), 10 (the
   maps, the interim UAT command), 11 (phase 2 closed with the numbers), CLAUDE.md (`packaged_bench.py`). **KB**: U/19 §1 (maps
   are not cooked by `DirectoriesToAlwaysCook`, `[Verified]` 5.8.3), INDEX symptom row.

### Verified
- `check_all` (everyday): every gate PASS, `L3 Pawpocalypse.Realm` now 9 tests. Card sabotages 29/29 CAUGHT. Packaged gate PASS
  (and seen to FAIL on the uncooked map). The -Full set was not rerun (last 2026-10-31); its `Realm sabotage` gate now takes
  ~55 min (45 sabotages).

### Found
- **F19** the puddle cards (Rainbow Trail, Lingering Rainbow) are offered only with Rainbow Gut (node id `unicorn`), not with
  Rainbow Poop (`unlock_poop`, what makes puddles); their hint says "Buy the Unicorn branch" (`unlock_unicorn`). Ported as Godot
  has it.
- Observed, ported as Godot has it (not a finding): Rainbow Trail's faster pace starts only from the second puddle (the
  PoopTimer restarts on its old 8 s wait before reading the new interval).

### For the designer
- Decide F19 (with F13-F18 and the older ones).
- Commits as before, plus `Source/Pawpocalypse/Realm/PawCardEffectsTests.cpp`, `Source/Pawpocalypse/Dev/PawBench.cpp`,
  the `Realm/PawRealmSubsystem.*` changes, `Config/DefaultGame.ini` (MapsToCook), `tools/packaged_bench.py`,
  `tools/sabotage_realm.py`.
- The spike code (`Dev/Spikes`, `Content/.../Dev/Spikes`) is still there and is cooked into Development packages; delete after
  a commit preserves it (doc 04 §8).

### Open
- `tools/package.ps1` (doc 10 §2: version bump, listing, never-ship check) is phase 8's; packaging is by hand until then.
- The packaged render thread (8.8 ms p99 at an uncapped ~340 fps) is the heaviest number; worth a look in phase 6 with the
  real atlas, at 60 fps capped.

### Next step
*"Start phase 3 per docs/11: the Common UI root layout and the HUD (dock, status row, Pounce clock, XP bar), the shop rows
calling `UPawRealmSubsystem::OnProgressChanged`, and the card pick calling `UPawRealmSubsystem::ChooseCard`. Read docs/05
first; spike S4 (`Dev/Spikes/SpikeUi`) is the proven structure."*

---

## 2026-10-31 (19) - Phase 2 slice 7: the engine side; the game opens on a live field; F18

### Where the project is
- **Phase 2: all 7 slices DONE** (docs/11). L_Realm opens on a running field: the cat follows the cursor, Pounce is the left
  mouse button (or the right trigger), mice, kittens, the boss and its adds, zones, shadows and effect rings are drawn,
  interpolated, at hundreds of fps; a debug HUD reads out the frame, the steps, the field and the wallet. The look is a
  placeholder (discs in each enemy's data tint and size) until phase 6's atlas.
- **Owed before phase 3**: the packaged-build gates (CLAUDE.md: "at the end of every phase"): a packaged frame time and the
  cooked L_Realm drawing. Packaging needs the designer's go-ahead, so it was not done.
- **Nothing committed** in either repo. The Godot repo: only `docs/pawpocalypse-port-findings.md` changed (F18 added).

### What happened
1. **`FPawRealmSim`** (`Source/Pawpocalypse/Realm`, no engine types in its logic): the fixed 60 Hz step over a real frame, the
   8-step guard (drops only time really owed, counted), pause, game speed, the play clock (sim time only), the snapshot the
   presentation interpolates from (a re-used slot never slides), the frame's events, travel (`BeginBiome` + `BeginField`,
   timed), the field's `TravelRequested`, and TierController's `_on_reawakened` (kittens, stats, boss dismissed, Territory's
   deeper start after the death sequence's beat).
2. **`UPawRealmSubsystem`** (a `UWorldSubsystem`, Game/PIE worlds only) over it, and **`UPawGameSubsystem`** (the data from the
   DataTables through `FPawDataAdapter`, loud in development; the `PcProgress`). Deviation from doc 03, recorded there: the
   realm is advanced by the view's own tick, right before it draws (a tickable subsystem ticks after the actors: a frame of
   cursor latency for nothing).
3. **`APawRealmView`** + **`UPawSpriteRenderer`** (`Render/`): spike S2's instanced quads, five layers, pooled and parked, one
   batch a layer; orthographic camera with S1's exposure settings; input sampled raw (phase 5 moves it to Enhanced Input);
   effects from events with a budget; bounds from the window (no dock until phase 3). Four generated materials
   (`tools/create_field_materials.py`); `tools/create_maps.py` places one configured view in L_Realm (only when missing).
   First screenshots found two presentation bugs, fixed: shadows hidden under their bodies; additive effects invisible on
   the light floor (now translucent: KB 2D/02).
4. **The debug HUD** (`APawDebugHud`, development builds, `paw.DebugHud`) and **the cheats**: the rules in PawCore (`PcCheats`,
   6 L1 tests, 9 sabotages), the overlay's buttons as console commands (`paw.Cheat.Catnip|Stardust|MaxUpgrades|Unicorn|
   Reawaken|Reset|AdvanceStage|UnlockTiers|UnlockStages|KillAll|Speed`, `paw.Travel`, `paw.Summon`, `paw.Spawn`). The F1
   panel with its password is phase 3's UI.
5. **Tests**: `Pawpocalypse.Realm.*`, 7 automation tests; `tools/sabotage_realm.py`, 16 game-module sabotages; `check_all` gains
   `L3 Pawpocalypse.Realm` (everyday) and `Realm sabotage` (-Full).
6. **The engine guard** (`tools/unreal_procs.py`, used by `run_automation.py`, `import_data.py`, `sabotage_realm.py` and
   `check_all`): another project's soak (`C:\Games\Cabbage`, run by another session every few minutes) kept making the realm
   sabotage run report MISSED for tests that never ran. The guard now ignores a process whose command line names ANOTHER
   project's `.uproject`; one of this project, or naming no project (it may be the designer's editor), still blocks.
   Self-tested (-Full gate `Unreal guard self-test`). That other process was never touched.
7. **Docs audit**: 00 (status, the file table: Realm/Render/Dev, Materials, the new tools), 03 (status, the subsystem row and
   why, the data load at startup, the cheat and HUD rows, the fixed-step pseudo-code), 04 §5.1 (the renderer as built), 07
   (placeholder material names), 09 (the L3 naming, the Realm row, counts, -Full timings, the guard; the card-effects engine
   test marked NOT built), 11 (slice 7 done, exit status, what is owed), 12 (four new traps), CLAUDE.md (the new tools),
   appendix B regenerated (the cheat row; one stale row from an earlier session refreshed); 05 (the interim raw Pounce read),
   06 (the mirror's adapter half, built since 2026-10-29).
8. **Knowledge base**: U/18 §2.1 (a standalone game instance in a test; frame-rate independence as a test), 2D/02 §2 (additive
   effects vanish over a light floor), indexes.

### Verified
- `Pawpocalypse.Realm.*` 7/7: frame-rate independence bit for bit at 30/50/60/144/240 fps and jittered; travel 0.006 ms worst
  (field) and 0.03 ms (with the redraw) against 16 ms; 320 mice + 20 kittens advance + draw 0.06 ms against 4 ms.
- Sabotages: realm all 16 CAUGHT on the final code, the clean build passing (`tools/sabotage_realm.py`, ~17 min); PawCore's 9 new (the cheats) all CAUGHT (272 in all). One realm miss on the first run was
  the test's fault (it faked a boss in a biome without one and crashed): rewritten to summon a real boss, then caught.
- L1: 121 cases, 4,392,529 assertions. Screenshots of the running game (1280x720 window): floor, shadows, bodies, kittens,
  rings, HUD all drawn.
- `check_all` (everyday): every gate PASS (L0, L1, mirror, fuzz 3k, L3 DataTable parity, L3 `Pawpocalypse.Data`, L3 `Pawpocalypse.Realm`, the Godot repo untouched). The -Full set was not rerun after slice 7 (last: 2026-10-31 01:45-04:11, all green); its new gates (`Realm sabotage`, `Unreal guard self-test`) were each run alone and pass

### Found
- **F18** the cheat "Unlock all stages" opens only the biomes behind no boss: it never sets this cycle's passage, the gate
  `is_biome_unlocked()` gained after it. Ported as Godot has it (dev tool only).

### For the designer
- Say go for the phase 2 packaged-build gates (a packaged Development build, its frame time at 320 mice, the cooked L_Realm).
- Decide F18 (with F13-F17 and the older ones).
- Commits as before, plus `Source/Pawpocalypse/{Core,Dev,Realm,Render}`, `Source/PawCore/*/PcCheats*`,
  `Content/Pawpocalypse/Materials/`, `Content/Pawpocalypse/Maps/L_Realm.umap`, the new tools.

### Open
- **Owed for phase 2's exit**: the engine-world card-effects test (`Paw.Realm.CardEffects`, doc 09 §3: every card effect
  applied in a live realm, its consumer observed); PawCore's rule tests pin the effects, the engine-world proof is not built.
- Placeholders by design: the look (phase 6), raw input reads (phase 5), no dock in the bounds (phase 3), the cheat panel
  (phase 3), auto-travel/auto-pick settings at Godot's defaults until phase 4's settings.
- The spike code (`Dev/Spikes`, `Content/.../Dev/Spikes`) is still there (doc 04 §8 says delete after); left for the designer.

### Next step
*"Close phase 2: build `Pawpocalypse.Realm.CardEffects` (doc 09 §3: every card effect applied in a live realm through the real
entry points, its consumer observed, each negative-tested in `tools/sabotage_realm.py`); then, once the designer says go, the
packaged-build gates. Then phase 3 per docs/11: the Common UI root layout and the HUD (dock, status row, Pounce clock), the
shop rows calling `UPawRealmSubsystem::OnProgressChanged`."*

## 2026-10-31 (18) - The night: phase 2 slices 2-6, `PcField` complete as a simulation; F13-F17

### Where the project is
- **Phase 2: slices 1-6 of 7 DONE** (docs/11). `PcField` simulates the whole realm: mice, kittens, the aura, Snare and zones,
  Pounce, the paw's and the kill's extras, the Prism, the boss and its adds. What is left of phase 2 is slice 7, the engine
  side (`UPawRealmSubsystem`, `APawRealmView`, renderer, debug HUD, cheat overlay); it needs the editor CLOSED to build.
- **Every gate green** (see Verified). **Nothing committed** in either repo. The Godot repo: only `docs/pawpocalypse-port-findings.md`
  changed (F13-F17 added; the file the port may write), `card_manager.gd` still carries the designer's uncommitted F1 fix.
- The designer's Unreal editor was open until about 01:30 (never touched); once it had closed, `DT_CodeConstants` was
  re-imported (`py tools/import_data.py`: three new rows), the editor target rebuilt with UBT, and the engine gates ran.

### What happened (the night worklist: `docs/WORKLIST-NIGHT-2026-10-30.md`)
1. **Slice 2, kittens.** New Godot fixture `kittens_fixture.gd` (a real `Kitten` and up to 24 real `Enemy` nodes: getters, pool
   size, orbit slot, `_pick_target`, `_is_overcrowded`, one whole `hunt()`), 3,000 rows. `PcFieldRules`: `KittenPoolBaseSize`,
   `KittenOrbitSlot`, `KittensSeekClusters`, `KittenIllusionPull`, `PickKittenTarget`, `KittenOvercrowded`. `PcField`: the pool,
   `SyncFieldKittens`, `HuntKitten`, the tally, roam, approach, contact damage, Gilded Kitten, Lone Hunter, Kitten Drag, Kitten
   From Signature, Kitten Kill Explodes, Grand Finale. Godot's own `check_kitten_spread` ported as a rule test. No spatial grid
   (measured: doc 04 §6).
2. **Slice 3, the aura.** Fixture `aura_fixture.gd` (getters, the aura cards, aura/poop/player DPS, a real Enemy's `apply_slow`).
   The aura, Snare/Deadweight, Gridlock, the slow's life (replaces slice 1's `Snared` flag with `SlowMult`/`SlowFresh`),
   Traps, the zone primitive, trail, echo, Edge Burn, the pulse, Rainbow Poop, the stillness clocks (`PcProgress`
   `PlayerStillSeconds`, `LunchSeconds`).
3. **Slice 4, Pounce.** Fixture `pounce_fixture.gd`. The cast, the press edge, Reflex, the bonus, Chain, Mimic, craters, the
   Cone of Shame (the cat rooted, stillness frozen), Insurance, Crash Landing, Bloodlust, Momentum.
4. **Slice 5, the extras.** Fixture `extras_fixture.gd`. Anvil Paw, Rhythm, Inscrutable, Riddle, Hammerfall, Burglar's Cut,
   Overflow, the Catnip Bomb, Long Lunch, Levy, Pied Piper, Sixth Sense, Sleight of Hand, the Prism; and the despawn recovery
   (Scavenge + Pickpocket), which slice 1 had missed.
5. **Slice 6, the boss.** Fixture `boss_fixture.gd` (a real `Boss` node, op logs). Summon, phases, shield cycle, relocation,
   drift, the fight clock and timeout, adds, the defeat and the travel after it, Auto-Bait, Auto-Travel, everything that hits
   the boss. Three inline Godot literals joined the code constants (`boss_relocate_distance_px`, `boss_wobble_x_px`,
   `boss_wobble_y_px`: map 574 rows, 66 gameplay constants). docs/11's exit criterion "timeout sends back a biome and turns
   auto-travel off" was wrong against Godot (it only withdraws the boss): corrected.
6. **The soak and the stress step** (`PcFieldSoakTests.cpp`, long suite): a scripted cat through the first three biomes, 3 seeds
   x 3 game hours, every step's invariants held; the full field (320 mice, 21 kittens, aura, zones, Prism) steps in 0.093 ms.
7. **Tooling**: `build_pawcore_tests.ps1` compiles with `/MP` and has `-TestsOnly`; `sabotage_pawcore.py` uses it: a sabotage
   went from ~4 min to ~12 s, so all of them run in under an hour.
8. **Knowledge base**: U/18 §8 (mutation tests: /WX non-compiling mutants, equivalent mutants, unique targets, copying a tree
   mid-run, /MP), G/03 §6 (fixture runs inherit autoload state through the sandbox's saves), G/07 (a pooled node's position
   read after `deactivate()`), 2D/01 §5 (a grid is not needed at a few hundred entities, measured); indexes updated.

### Verified
- `tools\check_all.ps1` (everyday): every gate PASS, the L3 engine gates included. L1: 115 test cases, 4,392,381 assertions.
- The long field tests (the five new 3,000-row fixtures + field rules, the soak, the step): 494,602 assertions, PASS.
- Sabotages: 263, each slice's batch seen CAUGHT tonight (161 new: kittens 34, aura 34, Pounce 26, extras 36, boss 31), every
  miss explained and fixed (non-compiling mutants, equivalent mutants, a test hidden by whole-Catnip rounding, a doctest filter
  with a comma, positions exactly at distance 0). The runner now refuses a comma in a filter (negative-tested).
- The nightly `-Full`: every gate PASS (17 gates, 2.6 h, 01:45-04:11): the 263 sabotages all CAUGHT in 46 min, the Godot fixtures reproduce, the long suite (20 min), the 200k mirror, the 30k fuzz, the pawsim parity sweep, the UBT build, the Godot repo untouched. Afterwards no sabotage is left in the source (`build/check_source_clean.py`).

### Found (doc 01 §5; the designer's copy in the Godot repo)
- **F13** Kitten Kill Explodes (Giblets) never hurts anything: the blast is placed at the dead mouse's parking spot.
- **F14** a card that costs kittens takes them only at the next re-count.
- **F15** `KITTEN_DRAG` and `KITTEN_FROM_SIGNATURE` have no card.
- **F16** Overkill from the aura, Pounce and Hammerfall spills from the parking spot (to the top-left-most mouse).
- **F17** Riddle executes on the swing that armed it, against its comment.
All ported as Godot has them, each behind a one-line switch or a pinned test, so a decision is a one-line change + a fixture
regeneration.

### For the designer
- Decide F13, F14, F16, F17 (players can see F13 and F16), F15; still open from before: F2, F3, F6, F7, F11, Q1-Q9.
- Commits (paths as before, plus the new files under `Source/PawCore/`, `tools/godot_fixtures/`, `Data/`, docs, and the
  re-imported `Content/Pawpocalypse/Data/DT_CodeConstants.uasset`).

### Open
- Slice 7 (engine side) needs the editor closed to build.
- Known approximations, by design: a mouse spawned in a step first moves on the next; PcRng instead of Godot's global RNG; the
  boss and adds move after the mice (child order); `_boss_retreating` is never set in Godot and is not ported.

### Next step
*"Phase 2 slice 7 per docs/11 and docs/03 ('What the game must do for PcField'): UPawRealmSubsystem (fixed 60 Hz step,
interpolation, the events drain, the contract), APawRealmView with the spike-S2 renderer, the debug HUD and cheat overlay,
travel < 16 ms, the frame-rate independence test (the editor closed to build)."*

## 2026-10-30 (17) - The nightly `-Full` run: every gate green; phase 1 exits

### Where the project is
- **Phase 1 done**: its last open exit item, the nightly `tools\check_all.ps1 -Full`, ran green (exit 0, ~3.6 h). Phase 2 slice 1
  of 7 done; slice 2 (kittens) next.
- **Nothing committed** in either repo; no code changed this session. Godot repo untouched (the run's before/after check PASS).

### What happened
1. `check_all.ps1 -Full`, every gate PASS: L0 static, L1 PawCore tests, mirror, validator fuzz 3k, L3 DataTable parity,
   L3 Pawpocalypse.Data, L0 self-test, mirror self-test, **PawCore sabotage (all 102 caught, clean build passes; 4,453 s)**,
   Godot fixtures reproduce (123 s), **L1 long suite (21 cases, 4,611,069 assertions; 1,674 s)**, mirror 200k states,
   validator fuzz 30k (287 s), **pawsim parity sweep (90/90 identical; 6,893 s)**, UBT editor build, Godot repo untouched.
2. `DataTable parity self-test` was SKIPPED inside the run (an Unreal editor was opened partway through). Rerun alone afterwards
   with no editor running: `py tools\import_data.py --self-test` -> `CAUGHT a changed EnemyData cell`, exit 0
   (`build\check_all\DataTable_parity_self-test_rerun.log`). So every gate has now passed on the current tree.
3. Docs brought up to date: 00 (status, Phase 1 done), 09 §7 (the nightly's measured times; an editor makes engine gates
   SKIP), 11 (phase 1 DONE), CLAUDE.md and `check_all.ps1`'s header (~3.5 h, was ~3 h).

### For the designer
- Nothing new to decide. Still open from before: F2, F3, F6, F7, F11, Q1-Q9; commits (paths as (16)).
- Keep the editor closed during a `-Full` run, or its engine self-test is skipped.

### Open
- Slice 1 known approximations, by design (see (16)).

### Next step
*"Phase 2 slice 2 per docs/11: kittens (pool sized to ownership, seek/roam/orbit/rebalance, contact damage, the spatial grid),
following the slice-1 pattern (pure parts against a Godot fixture, rule tests, sabotages)."*

---

## 2026-10-29 (16) - Phase 2 started: the field's pure rules against Godot, `PcField` slice 1

### Where the project is
- **Phase 2 under way**: slice 1 of 7 DONE and verified (docs/11 phase 2 lists the slices and how each is proved). Phase 1's exit
  still waits on a nightly `-Full` (not run: the designer needed the PC).
- **Gates, measured today, all PASS**: `tools\check_all.ps1` everyday set (L0; L1 51 test cases, 4,255,585 assertions, was 38;
  mirror; validator fuzz; L3 DataTable parity 12 tables incl. `DT_CodeConstants` re-imported at 63 rows; `Pawpocalypse.Data`
  4,213 cells equal). Long field-rules fixture 3,000 rows, 88,256 assertions PASS. UBT editor build succeeded.
- **All 20 new field sabotages CAUGHT** (+ clean PASS). First run: 15 caught; the other 5 were the test's or the script's fault,
  each fixed and rerun CAUGHT:
  - 3 NOBUILD (a sabotage left a parameter unused; warnings are errors): texts rewritten to keep them used.
  - `field_shred_uncapped` MISSED: `PawStrikeDamage` clamps the stacks itself, so damage cannot show an uncapped streak; the
    paw test now asserts `ShredStacks` stops at the ceiling.
  - `field_execute_through_shield` MISSED: without Shield Pierce the shield blocks the blow either way; the Execute test now
    asserts a shielded mouse is never even SWUNG for its whole HP.
- **Nothing committed** in either repo. The Godot repo is unchanged (every fixture ran in the scratch copy).

### What happened
1. **How phase 2 is proved** (docs/11): Godot's realm is frame-driven and rolls the global RNG, so it cannot be replayed row by
   row. Pure parts -> `PcFieldRules` against a Godot fixture; the stateful field -> rule tests + sabotages. Seven slices listed.
2. **Code constants: `@export` defaults joined.** Enemy and kitten speeds (`speed_px_per_sec`, `wander_turn_rate`,
   `flee_centre_bias`, `flier_speed_multiplier`, kitten speed, orbit radius, roam ratio) are `@export var` defaults, not
   `const`, so appendix C and the map had never seen them. `list_godot_code_constants.py` now scans numeric exports (appendix C:
   569); the map has 571 rows (new destination `data` for data classes' field defaults, 89; `save` 14); 63 gameplay constants
   (7 new fields in `PcCodeConstants`). `export_code_constants.py` refuses an exported default that a `.tscn` overrides (none
   does today). A trailing `## doc` comment no longer leaves a `#` in the reason.
3. **`PcFieldRules`** (`Public/PcFieldRules.h`): `SpawnRateMultiplier`, `SpawnRatePerSec`, `SpawnCadence` (batch/interval),
   `ThresholdDamageMultiplier`, `SnaredDamageMultiplier`, `PlayWithFoodMultiplier`, `FunnelBounces`, `EnemyDrift`,
   `PrismKillMultiplier`, `TotalSpawnWeight`, `PickEnemy`, `PlayfieldRect`, `EnemySpawnStats`, `ApplyBlow`. New Godot fixture
   `tools/godot_fixtures/field_rules_fixture.gd/.tscn` (`--only field_rules`): random states + a REAL pooled `Enemy` node's
   `activate()` and up to 4 `take_damage()` blows. Test `Tests/PcFieldRulesTests.cpp` (pow-derived values within 1e-13, blows bit
   for bit starting from Godot's own spawn stats, the playfield at float32 resolution because Godot's `Rect2` is float32;
   19 coverage paths asserted).
4. **`PcField` slice 1** (`Public/PcField.h`, `Private/PcField.cpp`): struct-of-arrays pool, Godot's frame order (strikes, flee
   targets, threshold published for the NEXT step with this step's dead still counted, Compound Interest and Infestation
   ticked, sweep; then the spawn timer; then every mouse moves; sweep), PcRng per (seed, step, serial, salt) with new salts
   (`SpawnEdge/Along/Turn`, `Funnel`, `Wander`), `SpawnEnemyAt`, `KillAllEnemies`, `PcFieldEvent` queue. Every later-slice rule
   is a named TODO in the header and the .cpp. `Tests/PcFieldTests.cpp`: 12 rule tests.
5. Sabotages +20 (`field_*`, 102 total). Docs 03, 06 §5, 09, 11 (phase 2 plan), 00; `make_godot_fixtures.py` docstring.
   Knowledge base `Godot/03` §6 + `INDEX.md`: Vector2/Rect2 are float32; fixture rows inherit autoload state unless set; a real
   scene node as the oracle.

### For the designer
- Nothing new to decide. Still open from before: F2, F3, F6, F7, F11, Q1-Q9; commits (paths as (15), plus the new files under
  `Source/PawCore/`, `tools/godot_fixtures/`, `Data/`).

### Open
- `tools\check_all.ps1 -Full` (~3 h) when the PC is free (overdue since (8)).
- Slice 1 known approximations, by design: a mouse spawned in a step first moves on the next; the field rolls PcRng, not
  Godot's global RNG.

### Next step
*"Phase 2 slice 2 per docs/11: kittens (pool sized to ownership, seek/roam/orbit/rebalance, contact damage, the spatial grid),
following the slice-1 pattern (pure parts against a Godot fixture, rule tests, sabotages). If the designer agrees first, run
`tools\check_all.ps1 -Full`."*

---

## 2026-10-29 (15) - Code constants from CSV; the DataTables imported and proven equal to the CSVs

### Where the project is
- **Phase 1's work list is done**: `Data/CodeConstants.csv`, `tools/import_data.py` + the 12 DataTables + parity, and the game's
  third of the mirror (`FPawDataAdapter`, `Pawpocalypse.Data.Parity`). The phase exit still needs a nightly `-Full` run
  (not run since (8); the designer asked for no long run today: the PC was needed).
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS, incl. the two new engine gates (L1 38 test cases,
  4,222,519 assertions; mirror 58,651 bit-identical; L3 DataTable parity 12 tables; `Pawpocalypse.Data` 2 tests, 4,185 cells
  equal). L0 `--self-test` 8 plants CAUGHT. `import_data.py --self-test` CAUGHT. 12 sabotages (3 new, 9 retargeted) CAUGHT +
  clean PASS. UBT editor build succeeded.
- **Nothing committed** in either repo. The Godot repo is unchanged by this session.

### What happened
1. **Code constants** (doc 06 §5). `Data/CodeConstantsMap.csv` is the reviewed destination of all 456 appendix C constants plus
   `BOSS_REALMS` and Second Helping's literal biome index: 56 `code_constants:<id>` (GameState, CardManager, and the field's
   rules for phase 2), 307 presentation, 66 palette, 21 input, 3 save, 2 audio, 3 dev; borderline calls carry a note.
   `tools/export_code_constants.py` (run by `export_godot_data.py`) writes `Data/CodeConstants.csv` with the values and reasons
   read from the Godot code, and fails on any unmapped, vanished, doubled or unparseable constant, so L0's data-export gate
   now covers it (self-tests: an edited value, a dropped map row, both CAUGHT). `list_godot_code_constants.py` exposes `scan()`
   (appendix C regenerated byte-identical).
2. **PawCore reads it**: `PcConstants.h` is now `PcCodeConstants` (`PcGameData::Code`), loader `ReadCodeConstants` (every field
   exactly one row, no unknown ids, integers integral; 6 malformed-file cases tested; 3 loader sabotages CAUGHT). No more
   constexpr copies. API change: functions that read a constant take the data: `EvolutionStage(D, P)`, `VisualStage(D, P)`,
   `CatchesForLevel(D, L)`, `TotalCatchesForLevel(D, L)`, `CurrentWave(D, Tier, K)`, `IsTierUnlocked(D, P, S)`,
   `BossesClearedForStage(D, P, S)`, `IsBiomeUnlocked(D, P, T)`, `MarkBossDefeated(D, P, Id)`, `TickInfestation(D, P, Dt)`.
   Validator +2 rules (57): `code.stage_ladder`, `code.range`, each negative-tested. pawsim `--data csv` reads
   `max_golden_chance` and the Catches curve from the CSV (`--catches-*` still sweep); the mirror is unchanged (bit-identical).
3. **`sabotage_pawcore.py`** refuses to run (exit 2, nothing built) when any target is not in its file exactly once (the open
   item from (13)); on its first run it found 3 targets this session's signature changes had broken. 82 sabotages.
4. **DataTables** (doc 06 §4, §9). Row structs are GENERATED (`tools/gen_paw_rows.py` -> `Source/Pawpocalypse/Data/PawDataRows.h`,
   L0-checked) with every column an `FString` = the cell's text (the importer stores FString cells verbatim, read in the 5.8.3
   source). `FPawDataAdapter` rebuilds the CSV text and calls the new `Pc::BuildGameData` (the one parser; `LoadGameData` now
   reads files and calls it too; `Pc::GameDataTableNames()`, `Pc::KnownGodotDataIssues()` moved out of the tests).
   Names: `DT_<CsvName>` / `FPaw<Name>Row` (docs said `DT_Tiers`/`FPawSkillNodeRow`; mechanical names now, docs updated).
   `tools/import_data.py` (headless; refuses while any Unreal editor runs; any `LogDataTable` warning fails) imported all 12 into
   `Content/Pawpocalypse/Data/` and checks parity on the saved assets. `tools/run_automation.py` runs the engine tests headless.
   `check_all` gained `L3 DataTable parity` and `L3 Pawpocalypse.Data` (SKIP, not FAIL, while an editor is open; the summary
   says what was skipped); `-Full` gained the parity self-test. Both engine gates were seen to FAIL before the first import.
5. A test assumption that proved wrong: `RemoveRow` + `AddRow` does not move a row to the end (the row map reuses the slot).
   Recorded in the knowledge base (`Unreal/05` §2.1, with the text-row pattern and the Python import recipe; `INDEX.md`).
6. Docs 00, 03, 06, 09, 11, `CLAUDE.md` (commands).

### For the designer
- Commit when happy (explicit paths, now including `Data/` and `Content/Pawpocalypse/Data/`): `CLAUDE.md Config/
  Pawpocalypse.uproject Source/ Content/ Data/ docs/ tools/`. The Godot repo still has (9)'s fix and (14)'s findings file.
- Another project's headless editor runs (Cabbage) were seen during this session; the engine gates wait or SKIP while any
  Unreal editor runs, by design.

### Open
- `tools\check_all.ps1 -Full` (~3 h) when the PC is free: overdue since (8); it now includes the new self-tests.
- A cooked-build test that loads every table and finds every row by id (phase 8 packaged gates).
- Godot decisions F2, F3, F6, F7, F11 and Q1-Q9 unchanged.

### Next step
*"Run `tools\check_all.ps1 -Full` (the designer must agree: ~3 h). If green, phase 1's exit is met: start phase 2 per docs/11
(`PcField` first, reading the field rows of `PcCodeConstants`)."*

---

## 2026-10-29 (14) - The Godot findings, written up for the designer in the Godot repo

### Where the project is
- Unchanged from (13): phase 1 has every Godot manager's rules in PawCore; left: `Data/CodeConstants.csv`,
  `tools/import_data.py` + DataTable parity, the game-side mirror. No code changed this session, so no gates were re-run.
- **Nothing committed** in either repo. The Godot repo now has two uncommitted changes: the session (9) Second Opinion fix
  (`scripts/autoload/card_manager.gd`) and the new `docs/pawpocalypse-port-findings.md`.

### What happened
1. **Designer's request: the Godot bugs the port found, in a document in the Godot folder.** New
   `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`: F1-F12 with the same numbers as doc 01 §5, grouped as
   *waiting on a decision* (F11, F6, F7, F2, F3), *low priority* (F8, F9, F10, F5, F4), *not a bug here* (F12) and *fixed* (F1,
   uncommitted), each with file, function and line (as of today) and the question to answer.
2. Docs brought in step: doc 01 §5 (points at the Godot copy; new findings go in both), doc 00 (where findings live), doc 06
   §8 (the Godot fix loop), doc 09 §7 recipe step 6 (record findings in both), `CLAUDE.md` (the findings file is the one Godot
   file the port writes without a decision). The other docs were checked against (13) and are current.

### For the designer
- Read `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`; the first table is the five decisions. F11 is the one players
  can see. Commit it with the `card_manager.gd` fix when you are happy with both.

### Open
- As (13): phase 1 remaining; `sabotage_pawcore.py` exact-once target check; `tools\check_all.ps1 -Full` overdue.

### Next step
As (13): *"Continue phase 1 per docs/11: `Data/CodeConstants.csv` (the gameplay rows of appendix C) and move `PcConstants.h`
onto it, then `tools/import_data.py` + DataTable parity; `tools\check_all.ps1`. Before that, if the designer agrees, run
`tools\check_all.ps1 -Full`."*

---

## 2026-10-29 (13) - The card draw ported, card for card against Godot's own random stream

### Where the project is
- **Phase 1: every Godot manager's rules are now in PawCore.** Left in phase 1: `Data/CodeConstants.csv`,
  `tools/import_data.py` + DataTable parity, the game-side third of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L1 37 test cases, 4,222,474 assertions). Long
  draw fixture (3,000 rows, 413,462 assertions) PASS. 16 new sabotages CAUGHT; one more retargeted (below). UBT editor build
  succeeded. Last full nightly: (8), before `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`, the draw.
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcCardDraw`** (`Public/PcCardDraw.h`, `Private/PcCardDraw.cpp`), CardManager's stateful half: `DrawHand` (weighted,
   without replacement, Fixation's owned pool with its fallback), `OfferHand` (Blueprint's promised cards first, deduped
   filler, Fixation spent on the deal, auto-pick, Prototype's mark, the promise drawn twice as Godot does, F10),
   `GainCatches` (the level-up: Second Opinion's rerolls, Catnip Interest before the hand, the deal), `PrototypeBonus`,
   `RerollHand`, `ChooseCard` (the deck cards act here), `BanishCard`, `BanishFromHand`; `CardForesight`,
   `CardPrototypeLevels`, `HasAutoCard`.
   - **Randomness is injected**: `PcCardRolls` (`Unit()` = randf, `Below(n)` = randi % n). The game's is
     `PcRngCardRolls` (PcRng per seed and deal number); the test's is a copy of Godot's PCG32 global RNG.
   - `PcProgress` (the struct) gained `Rerolls`, `Banishes`, `FixationHands`, `Hand`, `PromisedCards`, `PrototypeCard`.
     `ResetRunCards` now takes the data: rerolls go back to Overtime's count, and the hand/promise/mark/banishes/Fixation
     are cleared. `PcCards.h` exposes `CardMaxLevel`, `CardValueAt`, `CardCostAt`, `CardLevel`.
   - The loader now sorts the DECK by `_godot_file` too (`SortByGodotFile`, shared with the achievement roster): the draw
     walks the pool in deck order. Today file order = id order, so no sabotage can show it; the import must keep it.
2. **Godot fixture** `tools/godot_fixtures/draw_fixture.gd/.tscn` (`--only draw`): random run states, 8 ops each. Every op
   that can draw calls `seed(s)` with `s` written into the op. Godot's global RNG is PCG32 and `randf()` is ONE output
   rounded to a float times 2^-32 (pinned by the `calib` column, row 0). Two things had to be kept off the stream:
   **AudioManager.play() rolls the same global RNG for pitch**, behind a wall-clock rate limit (the first replay differed
   on row 1 because of it: F12), so the fixture empties its variants; and the pick screen's UI crashed headless after
   thousands of deals (children freed on a frame that never comes), so it is freed at start. About one row in seven has a
   deck nearly run dry. `Tests/Data/draw_godot.tsv` (1,000 rows, 3.2 MB), `build/draw_godot_full.tsv`.
3. **`Tests/PcCardDrawTests.cpp`**: replays card for card (hand, promise, mark, rerolls, banishes, Fixation, extra
   choices, XP, Catnip, owned levels, banished set, Grudge Ledger after every op); coverage asserted (1,000 rows: 1,400+
   deals offered, ~250 auto-picked, ~700 Prototype marks, ~660 promises, ~160 empty deals, 23 banishes that emptied the
   deck, every deck card taken, Prototype's bonus applied ~40 times...). Rules test: the game's rolls are deterministic
   per (seed, deal); first picks follow the weights within 5 sigma over 200,000 deals; the hand is the contract; banish
   does not consume the level; deck cards and Prototype on taking; XP while a hand is up; Fixation's fallback; Overtime
   survives the reset.
4. **Sabotages** (now 79): `draw_with_replacement`, `_fixation_ignored`, `_fixation_no_fallback`, `_promise_not_dealt`,
   `_no_dedupe`, `_fixation_not_spent`, `_single_promise`, `_auto_pick_without_setting`, `_prototype_any_card`,
   `_choose_uncapped`, `_extra_choice_adds`, `_reroll_free`, `_banish_free`, `_no_second_opinion`,
   `_reset_drops_overtime`: all CAUGHT (`_reset_drops_overtime` first failed to compile, unused parameter; rewritten).
   Retargeted after the code moved: `achievements_roster_csv_order` (now `SortByGodotFile`), and
   `progress_reawaken_keeps_cards` (`ResetRunCards(D, P)`); both CAUGHT. A check that every sabotage's target text occurs
   exactly once found the second one; worth adding to `sabotage_pawcore.py --list`.
5. **Findings** (doc 01 §5): **F10** a deal promises the next hand twice (the first only spends rolls); **F11** reroll and
   banish keep the old hand's Prototype mark and Blueprint promise; **F12** Godot's hands cannot be replayed (global RNG
   shared with sound pitch); the port differs by design (`PcRng`).
6. Docs 00, 01 (F10-F12), 02 §7 (the deal), 03, 09 §7, 11. Knowledge base `Godot/03` §6 + `INDEX.md`: replaying the global
   RNG (PCG32 seeding, randf), audio rolling the same stream, UI nodes piling up headless.

### For the designer
- **F11** is the one with a player-visible effect: should a reroll re-mark Prototype's card and redraw Blueprint's promise?
  F6, F7, F8, F9 still open. Each is a Godot change first.
- Still from (9): commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: `Data/CodeConstants.csv` (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable parity
  (L3; must keep deck and roster order); the game-side mirror.
- `sabotage_pawcore.py`: refuse to run when a target is not found exactly once (it reported "not caught" instead).
- Run `tools\check_all.ps1 -Full` (~3 h): overdue, last run (8).

### Next step
*"Continue phase 1 per docs/11: `Data/CodeConstants.csv` (the gameplay rows of appendix C) and move `PcConstants.h` onto it,
then `tools/import_data.py` + DataTable parity; `tools\check_all.ps1`. Before that, if the designer agrees, run
`tools\check_all.ps1 -Full`."*

---

## 2026-10-29 (12) - `PcAchievements` ported: earn, claim, load

### Where the project is
- **Phase 1 under way.** PawCore now also holds **`PcAchievements`**. Left in phase 1: the card draw (and the
  rerolls/banishes state it owns), `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the game-side third
  of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L0; L1 35 test cases, 4,084,444 assertions; mirror;
  validator fuzz 3k; Godot repo untouched). Long achievements fixture (3,000 rows, 1,031,268 assertions) PASS. The 14 new
  sabotages all CAUGHT, clean build PASS (one first failed to COMPILE, an unused parameter under warnings-as-errors, and was
  rewritten; then CAUGHT). UBT editor build succeeded. Last full nightly: (8), before `PcTree`, `PcJobs`, `PcProgress`,
  `PcAchievements`.
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcAchievements`** (`Public/PcAchievements.h`, `Private/PcAchievements.cpp`) from `achievement_manager.gd`:
   `FindAchievement`, `AchievementProgress` (the seven metrics), `IsAchievementEarned`, `EarnedAchievementCount`,
   `TotalStardustOnOffer`, `EvaluateAchievements` (the full re-scan; returns the ids earned in roster order),
   `AchievementReward`, `IsAchievementClaimable` (earned, unclaimed, reward > 0), `IsAchievementClaimed`,
   `UnclaimedAchievementCount`, `UnclaimedStardust`, `ClaimAchievement` (marked before paying, pays through
   `GrantAchievementStardust`), `ClaimAllAchievements`, `ApplyAchievementSave` (replace both sets, re-scan).
   - `PcProgress` (the struct) gained `AchievementsEarned`, `AchievementsClaimed`.
   - **No signals**: the game module calls `EvaluateAchievements` after every change that can move a counter (Godot's six
     signals). `NewGame` stays a port of `debug_reset_progress()`; as in Godot the title screen then empties the
     achievements with `ApplyAchievementSave({}, {})`.
   - **Roster order**: Godot lists the folder and sorts the file names (`achievement_01_kills_1` ...), the CSV is sorted by
     id. `LoadGameData` now keeps `_godot_file` for this table and sorts the roster by it (`PcData.cpp`).
2. **Godot fixture** `tools/godot_fixtures/achievements_fixture.gd/.tscn` (`--only achievements`): an op log of 10 ops per
   row (load a save incl. a retired id and claims without earns, a counter moves + re-scan, claim, claim all, New Game);
   each op's result (earned ids in emit order through `achievement_earned`, or Stardust paid) and a digest (earned/claimed
   as bits in roster order, set sizes, both Stardust totals); then every achievement's progress, reward and flags. 40 % of
   rows overwrite thresholds and rewards in memory (0, negative, fractional). `Tests/Data/achievements_godot.tsv` (1,000
   rows, 5.1 MB; the first form with full id lists was 8.5 MB), `build/achievements_godot_full.tsv`.
3. **`Tests/PcAchievementsTests.cpp`**: replays the ops; coverage asserted (1,000 rows: 1,084 claims paid / 2,166 refused,
   958 claim-alls paid / 504 nothing / 95 fractional, 872 re-scans earned, 218 earning several at once, 1,730 values exactly
   on the threshold, 80 earned achievements paying nothing, 427 claimed-not-earned, 368 edited rosters). Long on 3,000;
   rules from the comments (two events, a double claim pays nothing, nothing un-earns, zero reward = not claimable,
   claim all, ledger not passage, load and New Game, roster order).
4. **Sabotages** (now 64): `achievements_strict_threshold`, `_claim_not_marked`, `_zero_reward_claimable`,
   `_claimed_still_claimable`, `_bosses_count_passage`, `_stage_is_visual`, `_catnip_this_cycle`, `_load_merges`,
   `_load_no_rescan`, `_claim_all_stops`, `_grant_spendable_only`, `_badge_counts_earned`, `_unclaimed_stardust_earned`,
   `_roster_csv_order`: all CAUGHT.
5. **Finding** (doc 01 §5): **F9** (low) the earned count includes ids no longer in the roster (a save holding a removed
   achievement shows "42 / 41"). Ported as Godot.
6. Docs 00, 01 (F9), 02 §11 (the rules), 03 (module table, status), 09 §7, 11; appendix B regenerated (no change). Knowledge
   base `Godot/03` §6: replaying signal handlers that fire mid-sequence, Resource folder load order, bit-string digests.

### For the designer
- **F9** joins F6, F7, F8 (doc 01 §5): each is a Godot change first if you want it. F2 (two unreachable achievements) is
  still open and touches this port only through the data.
- Still from (9): commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: the card DRAW (weighted through `PcRng`, rerolls, banishes, the hand); `Data/CodeConstants.csv`
  (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable parity (L3); the game-side mirror. The DataTable import
  must keep the achievement roster order (`_godot_file`).
- Run `tools\check_all.ps1 -Full` (~3 h): overdue, last run (8).

### Next step
*"Continue phase 1 per docs/11: port the card DRAW (`card_manager.gd`: the weighted hand through `PcRng`, rerolls, banishes,
extra choices) following the recipe in docs/09 §7, then `tools\check_all.ps1`."*

---

## 2026-10-29 (11) - `PcProgress` ported: every rule that changes progress

### Where the project is
- **Phase 1 under way.** PawCore now also holds **`PcProgress`** (the module). Left in phase 1: `PcAchievements`, the card
  draw (and the rerolls/banishes state it owns), `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the
  game-side third of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L0; L1 33 test cases, 3,740,590 assertions; mirror
  58,651 bit-identical; validator fuzz 3k; Godot repo untouched). Long progress fixture (3,000 rows, 803,621 assertions) PASS.
  The 20 new sabotages all CAUGHT, clean build PASS (two first failed to COMPILE, unused variable under warnings-as-errors,
  and were rewritten to keep it used; then CAUGHT). UBT editor build succeeded (compiles `PcProgress.cpp`,
  `PcProgressTests.cpp`). Last full nightly: (8), before `PcTree`, `PcJobs`, `PcProgress`.
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcProgress`** (`Public/PcProgress.h`, `Private/PcProgress.cpp`) from `game_state.gd`'s mutating functions:
   `MarkBossDefeated` (power vs this cycle's passage, the final-boss clock on the first clear only), `RecordBossDefeat`
   (TierController's rule half: reward, ledger, ending flag), `IsUnicornOwned`, `VisualStage`; `CreditCurrency`,
   `AddCurrency`, `CreditUntouchedCatnip`, `GrantAchievementStardust`, `SpendStardust`; `IncrementKillCount` (+ Compound on
   the wave turnover, Reputation, Fence, Infestation), `PayCatnipInterest`; `RealmIndex`, `BankTierKills`, `BeginBiome`
   (Second Helping's deferred half, deepest biome); `IsBaitUnlocked`, `CanAffordBait`, `BuyBait` (all-or-nothing);
   `IsUpgradeVisible`, `IsUpgradeAtCap`, the kitten locks, `UpgradeCostById`, `PurchaseUpgrade`, `PurchaseUpgradeBulk`,
   `PreviewBulkPurchase`; `Reawaken` (+ `ResetRunCards`, `PcJobs::ClearRunJobs`); `TickPlayClock`, `TickRealtimeInterest`,
   `TickInfestation`; `NewGame`. Functions return what happened; signals, stat rebuilds and sounds are the game module's.
   - `PcProgress` (the struct) gained `Catnip`, `LifetimeKills`, `KillsSinceTithe`, `InfestationBonus`,
     `PendingSecondHelping`, both play clocks, `FinalBossSeconds`, `RealtimeInterestElapsed`. `PcConstants.h` gained
     `CurrencyEpsilon`, `BulkMaxSteps`, `MountedStage`, `LevyEvery`, the Infestation pair, `RealtimeInterestPeriodSec`,
     `SecondHelpingBiomeIndex`.
   - Not here, on purpose: the kill funnel's own rules in `tier_controller.gd` (Levy's lump, Long Lunch, the overkill
     bounty) belong to `PcField` (phase 2); rerolls/banishes/the hand belong to the card draw.
2. **Godot fixture** `tools/godot_fixtures/progress_fixture.gd/.tscn` (`--only progress`): an OPERATION LOG. Each row sets a
   random state, runs 12 random ops, records each op's result (boss_defeated / wave_changed counted through signals, the
   Reawaken grant read from `reawakened`), a digest after each op, and the whole state at the end. `make_godot_fixtures.py`
   gained `convert_tokens` (doubles inside `|`/`,`/`:` cells; used for this table only). `Tests/Data/progress_godot.tsv`
   (1,000 rows, 3.2 MB), `build/progress_godot_full.tsv`.
3. **`Tests/PcProgressTests.cpp`**: replays the ops; coverage asserted for every op and each outcome both ways (1,000 rows:
   365 Reawakens done / 165 refused, 457 new boss clears / 164 rematch passages / 207 nothing, bait 235 / 707, buys 401 /
   769, waves turned 453, Second Helping paid on travel 233, 68 new games...). Long on 3,000; rules from the comments
   (ending flag and clock, a summon one Catnip short keeps bait and Catnip, the epsilon, bulk = preview, what a Reawaken keeps,
   Second Helping at biome 4 not 3, travel, New Game, the clocks).
   - **`pow()` ulps** (KB Godot/03 §6) make some upgrade prices differ by a few ulps, and so the Catnip after paying them:
     compared within 1e-14 of the row's largest bank (measured worst 1.2e-15; 6,103 inexact values), every decision exact.
4. **Sabotages** (now 50): `progress_rematch_no_passage`, `_final_clock_every_clear`, `_ending_on_any_boss`,
   `_kill_skips_multipliers`, `_untouched_whole`, `_no_interest_on_wave`, `_fence_not_reset`, `_infestation_uncapped`,
   `_reputation_kept`, `_second_helping_early`, `_bait_never_short`, `_audit_spends_bait`, `_no_epsilon`,
   `_kitten_power_unlocked`, `_preview_ignores_cap`, `_hidden_line_restored`, `_reawaken_keeps_passage`,
   `_reawaken_keeps_cards`, `_newgame_keeps_jobs`, `_visual_ignores_unicorn`: all CAUGHT.
5. **Findings** (doc 01 §5): **F7** Fence's kill count is not reset by travel or Reawaken (and never in a biome without
   waves); **F8** the Kitten Speed lock is UI-only (`purchase_upgrade()` enforces only Kitten Power's). Both ported as Godot.
6. Docs 00, 01 (F7, F8), 03 (module table, status), 09 §7, 11; appendix B regenerated. Knowledge base `Godot/03` §6: op-log
   fixtures for stateful code; `pow` ulps reaching balances through spending.

### For the designer
- **F6** (one held job: ask or lock in silently?), **F7** (should travel / Reawaken reset Fence?), **F8** (enforce the Kitten
  Speed lock in the rule?): each is a Godot change first if you want it.
- Still from (9): commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: `PcAchievements`; the card DRAW (weighted through `PcRng`, rerolls, banishes, the hand); 
  `Data/CodeConstants.csv` (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable parity (L3); the game-side mirror.
- Run `tools\check_all.ps1 -Full` (~3 h): overdue, last run (8).

### Next step
*"Continue phase 1 per docs/11: port `PcAchievements` (`achievement_manager.gd`: metrics, evaluate, claims paying Stardust
through `PcProgress::GrantAchievementStardust`) following the recipe in docs/09 §7, then `tools\check_all.ps1`."*

---

## 2026-10-29 (10) - `PcJobs` ported; the job math has one home

### Where the project is
- **Phase 1 under way.** PawCore now also holds **`PcJobs`**. Left in phase 1: `PcProgress` (the module), `PcAchievements`, the
  card draw, `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the game-side third of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L0; L1 31 test cases, 3,472,407 assertions; mirror
  58,651, bit-identical; validator fuzz 3k; Godot repo untouched). Long jobs fixture (3,000 rows, 261,004 assertions) PASS. The 11
  new sabotages all CAUGHT, clean build PASS (the 19 older ones were not re-run; PcStats/PcEconomy/PcCards changed only by
  moving the job helpers, and their fixtures still pass with identical counts). UBT editor build succeeded (compiles
  `PcJobs.cpp`, `PcJobsTests.cpp`). Last full nightly: (8).
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcJobs`** (`Public/PcJobs.h`, `Private/PcJobs.cpp`) from `game_state.gd` "Jobs", `job_data.gd` and `job_picker.gd`:
   `AllJobs` (roster order), `HoldsJob`, `JobIsAvailable`, `OfferJob`, `LockRunJob`, `LockRunJobSecond`, `TakeJob`,
   `ClearRunJobs` (the Reawaken's part, for the `PcProgress` port to call), `HasUnionRules`, `UnionRulesShare`, `EquippedJob`,
   `SecondaryJob`, `JobTerm`, `JobCritChanceBonus`, `JobExtraCards`, `JobPounceLockSec`, `JobHatPath`, `IsJobNeutral`,
   `JobHasACost`, `ShouldAskForJob`, `OtherJobsThan`. Signals stay with the caller (functions return bool).
   - The job helpers that lived privately in `PcStats.cpp` (and `JobTerm` / `JobExtraCards` in `PcStats.h`) MOVED here;
     `PcStats`, `PcEconomy`, `PcCards` and the validator (`job.neutral`, `job.free_gift`) now call `PcJobs`. One copy of the
     job math.
   - `PcProgress` gained `CurrentJob` (the standing preference) and `bRunJobChosen`.
2. **Godot fixture** `tools/godot_fixtures/jobs_fixture.gd/.tscn`, registered in `make_godot_fixtures.py` (`--only jobs`): 3,000
   random states. The shipping Union Rules share is a single 0.5, so 40 % of rows overwrite it IN MEMORY in the scratch copy
   (0, 0.25, 0.7, 1, 1.3, -0.2, 2, random) and record the value; the test applies it to a data copy. A card job is aimed at
   the second slot 20 % of the time (uniform states reached the floored card 4 times per 1,000). `Tests/Data/jobs_godot.tsv`,
   `build/jobs_godot_full.tsv`. Equal to Godot on the first run.
3. **`Tests/PcJobsTests.cpp`**: L1 on 1,000 rows with coverage asserted (second job 329, card via floor 16, floored away 114,
   clamped 54, stored second without the node 288, every state change accepted and refused, picker asks / does not), long on
   3,000, and rules from the Godot comments on edited data copies.
4. **Sabotages** (`tools/sabotage_pawcore.py`, now 30): `jobs_share_halves_number`, `jobs_cards_rounded`, `jobs_share_unclamped`,
   `jobs_second_without_node`, `jobs_effect_reads_preference`, `jobs_second_may_be_primary`, `jobs_take_unchanged`,
   `jobs_ask_needs_two`, `jobs_health_cost_inverted`, `jobs_roster_reversed`, `jobs_reawaken_keeps_lock`: all CAUGHT.
5. **Finding F6** (doc 01 §5): the job picker's comment says one held job should lock in silently, but `should_ask()` asks at
   `>= 1`. Ported as the code has it.
6. Docs: 00, 01 (F6), 02 §10 (the job rules), 03 (module table, status), 09 §7 (counts), 11 (status); appendix B regenerated.
   Knowledge base `Godot/03` §6: feeding the oracle other data in memory; `sort_custom` is stable at 16 elements or fewer.

### For the designer
- **F6**: should the job picker open when you hold exactly one job? Godot asks today; its own comment says it should not.
- Everything from (9) still stands: commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: `PcProgress` (boss ledger, waves, passage, and the Reawaken that calls `PcJobs::ClearRunJobs`),
  `PcAchievements`, the card DRAW, `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the game-side mirror.
- Run `tools\check_all.ps1 -Full` (~3 h) once; last full run was (8), before `PcTree` and `PcJobs`.

### Next step
*"Continue phase 1 per docs/11: port the `PcProgress` module (`GameState` boss ledger `mark_boss_defeated`, evolution and visual
stage, waves, biome unlocks, passage per cycle, final boss, and the Reawaken's resets incl. `PcJobs::ClearRunJobs`) following the
recipe in docs/09 §7, then `tools\check_all.ps1`."*

---

## 2026-10-29 (9) - Second Opinion fixed (once per Catches level); `PcTree` ported; every doc brought up to date

### Where the project is
- **Phase 0 done** (spikes S1-S4 recorded; spike code kept until a commit preserves it, then deleted).
- **Phase 1 under way.** PawCore holds `PcData` (+ generated `PcDataRows.h`, `PcEnums.h`), the validator (55 rules),
  `PcFormat`, `PcRng`, `PcCombat`, `PcStats`, `PcEconomy`, `PcCards` (all but the hand draw) and, new today, **`PcTree`**.
  Every module equals the REAL Godot code on random states (Godot fixtures, doc 09 §7) and PawCore equals pawsim bit for bit
  (the mirror).
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS: L0 static; L1 29 test cases, 3,385,261 assertions;
  mirror 58,651 assertions, all bit-identical; validator fuzz 3k; Godot repo unchanged by the run. Long fixtures for cards
  (3,000 rows) and tree (3,000 rows, 1,635,003 assertions) PASS. Sabotages: `py tools/sabotage_pawcore.py`, all 19 run today and
  all CAUGHT, clean build PASS. UBT editor build
  succeeded and compiles `PcTree.cpp` and `PcTreeTests.cpp`. The full nightly (`check_all.ps1 -Full`) was last run in (8).
- **Nothing committed** in either repo. One file changed in the Godot repo (below).

### What happened
1. **Designer decision: Second Opinion hands over its rerolls once per card, i.e. once per Catches level / hand** (doc 01 §5
   F1, doc 02 §9 cards).
   - Godot first (CLAUDE.md): `C:\Games\pawpocalypse\scripts\autoload\card_manager.gd` `add_kill_catches()`: the
     `rerolls += GameState.get_rerolls_per_level()` line moved from the not-levelled branch to just after
     `catches_level += 1`. **UNCOMMITTED in the Godot repo** (`git -C C:\Games\pawpocalypse status` shows exactly this file).
     Godot's own `ci/cards_test.tscn` passes on it (263 assertions; run in the sandboxed scratch copy).
   - PawCore: `PcCards::AddKillCatches` grants `RerollsGranted` on the levelling path only.
   - Fixtures regenerated from the fixed Godot (`make_godot_fixtures.py --only cards`): only `step_rerolls` changed; 97
     levelling rows grant, 0 non-levelling rows do. `PcCardsTests.cpp` now asserts no grant without a level and > 20 grants.
   - Sabotages: `second_opinion_every_kill` (the old bug put back) and `second_opinion_never`: both CAUGHT.
2. **`PcTree` ported** (`Source/PawCore/Public/PcTree.h`, `Private/PcTree.cpp`) from `skill_tree_manager.gd`: what the tree
   sells (required parent, rival fork, boss key, Reawaken gate with the three ability-unlock constants), prices with
   Godot's sentinels (-1 maxed, +inf unknown id), affordability, `BranchForJob`, `BlockingRival`, the purchase (Stardust
   spent, level +1, a job node hands its job over once, Auto-Travel / Auto-Card ask for their switch ON) and the free grant
   (never lowers, clamps). `PcProgress` (in `PcStats.h`) gained `Stardust` and `JobsHeld`.
   - Godot fixture `tools/godot_fixtures/tree_fixture.gd/.tscn`, wired into `make_godot_fixtures.py` (`--only tree`):
     3,000 states, every branch per row; a third of the probes aimed at job and automation nodes, and every gate in front of
     the probe opened half the time (uniform states had reached the automation nodes 0 times). Full set: 1,040 purchases,
     242 jobs handed over, 81 automation switches. `Tests/Data/tree_godot.tsv` (1,000 rows), `build/tree_godot_full.tsv`.
   - `Source/PawCore/Tests/PcTreeTests.cpp`: L1 (545,099 assertions, coverage asserted), long (3,000 rows), and rules from the
     Godot comments. Equal to Godot on the first run.
   - The shipping data sets the three ability-unlock constants AND those branches' `required_reawakens` all to 0, so the
     fixture cannot see which one the gate reads (sabotage `tree_pounce_gate_swapped` was MISSED by it); a rules test on a
     data copy with distinct numbers now catches it. Sabotages `tree_rival_ignored`, `tree_boss_gate_ignored`,
     `tree_pounce_gate_swapped`, `tree_grant_lowers`: all CAUGHT.
3. **A stale rule removed from the docs**: "tree keys are parsed from the RIGHT" (docs 02, 03, 12, appendix B via
   `tools/make_port_map.py`). Godot keys nodes by branch id alone since 2026-08-21 (`ci/check_hud.gd`); old keys survive only
   in v5 saves, migrated by `save_manager.gd` `_migrate_branch_levels` (the save port, phase 4).
4. **Docs brought up to date**: doc 00 (new "Where the port is" and a full inventory of what exists); doc 01 new §5 "Findings
   in the Godot game" (F1-F5 with status); doc 02 (tree fork, boss key, job hand-over, automation switch, free grants; Second
   Opinion rule); doc 03 (PawCore status, `PcSkillBranch` name, the `PcProgress` struct vs module note); doc 06 (pawsim on
   CSV done, mirror numbers 58,651 / 11 sabotages, the Godot oracle list, the CLI's real flags); doc 09 §7 (counts, tree,
   and **the recipe for porting a manager**); doc 11 status; doc 12; CLAUDE.md (check_all exists, the gate commands, the
   Godot-first rule for decisions); the night worklist marked closed. Appendices A and C regenerated: unchanged; B: one line.
   Knowledge base: `Godot/03` §6 (aimed probes with asserted coverage; degenerate data hides a rule; operand-scaled
   tolerance), `Godot/00-INDEX.md`.

### For the designer
- **Commit the Godot fix** in the Godot repo when you are happy with it: `scripts/autoload/card_manager.gd` only.
- **Commit the Unreal work** (nothing since `7410909`). Stage explicit paths, never `git add -A`:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/` (`build/` and `__pycache__/` are ignored). After the
  commit, delete the spike code (`Source/Pawpocalypse/Dev/Spikes`, `Content/Pawpocalypse/Dev/Spikes`, `tools/spikes`) per doc 04 §10.
- Still open: F2 achievements `br_ferocity_5` / `br_mystique_6` unearnable; F3 Patience x Attack Speed trap (design note);
  questions Q1-Q9 (doc 01 §3; Q9 = no AA vs FXAA).

### Open
- Phase 1 remaining: `PcJobs`, `PcProgress` (the module: boss ledger, waves, passage), `PcAchievements`, the card DRAW
  (weighted, through `PcRng`); `Data/CodeConstants.csv` (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable
  parity (L3); the game-side third of the mirror.
- `make_godot_fixtures.py --check` for cards reproduces only against the Godot tree WITH the fix (committed or not).
- Run `tools\check_all.ps1 -Full` once more (~3 h) after the next module; last full run was (8).

### Next step
*"Continue phase 1 per docs/11: port the jobs (`GameState` job functions: held, worn per run, Union Rules' second job, job
terms) into `PcJobs` following the recipe in docs/09 §7 (Godot fixture via `tools/make_godot_fixtures.py`, coverage asserted,
sabotages seen caught), then `tools\check_all.ps1`."*

---

## 2026-10-28 (8) - night session: phase 1 core rules, every gate built and negative-tested

### Where the project is
- **Phase 1 is well under way** (docs/11 status line). PawCore now holds `PcData` (+ generated `PcDataRows.h`, `PcEnums.h`),
  the validator (55 rules), `PcFormat`, `PcRng`, `PcCombat`, `PcStats`, `PcEconomy`, `PcCards`. Each rule module is proved against the
  REAL Godot code on random inputs: `PcCombat` bit for bit on 5,000 `CombatResolver` rows, `PcStats` on 3,000
  `_rebuild_stats()` states, `PcEconomy` on 3,000 economy states, `PcCards` on 3,000 `CardManager` states, `PcFormat` on
  4,403 `Amounts.short` values (only `pow`
  differs, by up to 14 ulps of msvcrt vs MSVC). PawCore also matches pawsim bit for bit (the mirror, 58,651 assertions).
- **`tools/check_all.ps1` exists** (doc 09 §7): the everyday set in minutes, `-Full` the nightly (~3 h). Every gate has its
  own negative test (self-test or sabotage) and each was seen to fail.
- Nothing committed. Godot repo untouched (checked before and after every run). The UBT editor build compiles all of
  PawCore (8.7 s).

### What happened this session (worklist: `docs/WORKLIST-NIGHT-2026-10-28.md`)
1. pawsim copied in and taught to read `Data/*.csv`; byte-identical to its `.tres` loader on the full game and over a
   90-setting sweep (`tools/check_pawsim_parity.py`; `--self-test` added: one changed CSV cell must show).
2. PawCore data layer + validator (each rule with a sabotage). Found: achievements `br_ferocity_5` / `br_mystique_6` count
   branches that were merged into `claws` (unearnable, 2 Stardust each); Godot's `check_resources` runaway branch never ran.
3. `PcFormat`, `PcRng`, `PcCombat`, `PcStats`, `PcEconomy`, `PcCards` + the Godot fixtures, now regenerated by ONE tool:
   `py tools/make_godot_fixtures.py [--check]` (scratch copy of the Godot project, sandboxed APPDATA; `--check` proved all
   fixtures reproduce byte for byte).
4. `pawcore_sim.exe` (`Tests/PcSimCli.cpp`) + `tools/check_mirror.py`: Godot's mirror fixture at every stage, 2,000 random
   states, every price, the Stardust grant, Legacy, Fortune, golden chances. `--self-test`: 11 data sabotages, all caught.
5. Long suite (`build_pawcore_tests.ps1 -Long`, ~30 min): full Godot fixtures + property tests over ~150 M random states
   (see doc 09 §7). `tools/sabotage_pawcore.py`: 8 one-line source sabotages, each caught by its long test.
6. Validator fuzz (`tools/fuzz_validator.py`): 30,000 typed single-cell corruptions. It found two validator gaps, both now
   rules: **`ids.lower_case`** (upper-case ids passed) and **`ref.branch_boss`** (a `gated_by_boss` naming a non-boss biome
   locks the branch forever). Remaining uncaught hostile classes are explained in the script (dead `ability_ids`; a shorter
   ladder on a no-cost card).
7. `tools/check_pawcore_static.py` (L0: no float, engine-free PawCore, guarded tests, generated headers current, `Data/` =
   a fresh Godot export) with `--self-test`; `tools/check_all.ps1` runs it all.
8. Docs: 06 §8, 09 §7, 11 (phase 1 status), this log. KB: Godot/03 §6 (Godot as a bit-exact oracle: `var_to_bytes` 32-bit
   floats, msvcrt `%.1f` rounding, MinGW `pow` ulps), Godot/07 rows, Unreal/23 (Python `\b` in Windows paths), INDEX.

### Final results of the night's runs
- `tools/check_all.ps1` (everyday set): all PASS; L1 27 test cases / 2,839,149 assertions; mirror 58,651; fuzz 3k clean.
- Long suite (36 min): 10/10 PASS (45 M states; 20 billion RNG draws, chi2 1,010 on 1,023 dof; Patience trade-off in
  48,665 of 9 M states). The economy/cards long cases (added later) then ran separately at full scale: 5/5 PASS after one
  fix: the full economy fixture showed `wave_done` 1.9e-12 off in 3 of 3,000 rows (a small difference of two large pow
  series), so that one field now has a tolerance scaled to its operands; a one-kill sabotage is still caught.
- `tools/sabotage_pawcore.py`: 14/14 caught. Mirror `--self-test` 11/11. Parity `--self-test` caught. L0 `--self-test` 5/5.
- pawsim parity sweep: 90/90 identical (110 min). Validator fuzz 30k: every hostile corruption caught or explained.
- Godot repo `git status` empty after every run.

### For the designer (found tonight, nothing changed in Godot)
- **Godot bug, Second Opinion**: `card_manager.gd` `add_kill_catches()` adds `get_rerolls_per_level()` rerolls in the branch
  where the kill did NOT level up (commit ca6b9784), so an owner gains rerolls on nearly every kill, not per level as the
  comment and node text say. `check_damage` tests the getter, not the grant. **Fixed 2026-10-29, entry (9).**
- **Godot data bug**: achievements `br_ferocity_5`, `br_mystique_6` can never be earned (merged branch ids).
- **Design note (not a bug)**: with Patience owned and the cat slower than base (the Sphinx), buying Attack Speed can LOWER
  paw DPS (d DPS / d speed < 0 once per x crit chance x crit mult > 1 + crit chance x (crit mult - 1)). The shop would sell a
  trap in that build. Counted by the long suite, not failed.
- **pawsim vs game**: pawsim caps the kitten count, the game does not (only reachable past the shop's cap; harmless).
- `Amounts.short` prints an overflowed integer past ~9.2e18 after the suffixes run out (PawCore prints the digits).
- Still open: Q1-Q9 (Q9 = no AA vs FXAA); commit of phase 0 + tonight's work.

### Open
- Phase 1 remaining: `PcProgress`, `PcTree`, `PcJobs`, `PcAchievements`; `PcCards`' hand DRAW (weighted, through `PcRng`;
  the eligibility, hand size, values and XP step are done, Second Opinion ported as Godot has it, flagged); `Data/CodeConstants.csv` (+ the constants in `PcConstants.h` move there);
  `tools/import_data.py` + DataTable parity (L3); the game-side third of the mirror.
- Spike code still to delete after a commit (entry (7)).

### Next step
Designer: read "For the designer" above, then commit (explicit paths; the night's files are listed in the worklist).
Then: *"Continue phase 1 per docs/11: port SkillTreeManager into PcTree (buy rules, prerequisites, exclusivity, respec) with
a Godot fixture via tools/make_godot_fixtures.py, then run `tools\check_all.ps1`."*

---

## 2026-10-28 (7) - spike S4 done: all four spikes recorded

### Where the project is
- **Phase 0 step 5 is done in substance**: S1-S4 recorded in doc 04 §10 (and doc 05/07). What remains of phase 0: delete the
  spike code and assets (after a commit keeps them in history), the designer's review of the phase 0 changes, and the open
  questions Q1-Q9 (Q9 = no AA vs FXAA). Then phase 1. Nothing committed yet.
- S4: the doc 05 UI structure (root layout with Game/Menu stacks; HUD activatable with the dock; tree activatable with 87 nodes
  in one canvas) built in C++ only passed **1,193 focus checks** with real Slate key events, focus never lost; every one of the
  tree's 87 x 4 navigation rules pressed and verified.

### What happened this session
1. `Source/Pawpocalypse/Dev/Spikes/SpikeUi.*` (button style, row button, HUD/dock, tree + link painter, root layout, tour
   actor), `tools/spikes/spike_s4_setup.py` (map `L_SpikeUi`). Deps added to the SPIKES line of `Pawpocalypse.Build.cs`: UMG,
   CommonUI, CommonInput, InputCore.
2. First tour: 455 pass, 3 fail, all one cause: the dock's remembered row went stale when the input device switched (Common UI
   re-applies the focus target). Fixed by tracking it every frame; a device-switch check added.
3. Negative tests: `-S4Sabotage=focus` failed loudly (focus to `SViewport`); `-S4Sabotage=nav` first PASSED (the walk only took
   shortest paths), so the walk now presses every rule, and then failed as it must.
4. Screenshots (windowed): the focus highlight is inconsistent (style it explicitly) and the tree needs an opaque backdrop.
5. Docs: 04 §10 (S4), 05 (§2 rules 8-11, §6.2), 11, 12, this log; KB Unreal/08 §3.4, Unreal/09 §3, Unreal/24, INDEX.

### Open
- Designer: review the phase 0 + spike changes and decide what to commit; Q1-Q9.
- Delete spike code/assets once committed (list in entry (6) plus `SpikeUi.*`, `spike_s4_setup.py`, `Content/.../Dev/Spikes/S4`,
  `Saved/Spikes`). Keep `tools/spikes/s3_raster_pack.py` ideas for phase 6 (or move it to `tools/` first).
- Real-pad checks (Xbox, PlayStation via Steam Input) are phase 5.

### Next step
Designer review and commit, then: *"Delete the phase 0 spike code and assets per docs/HANDOFF-LOG.md (7), confirm the phase 0
exit criteria in docs/11, then start phase 1."*

---

## 2026-10-28 (6) - spike S3 done: 2x rasters, BC7, 2048 pages; render resolution fixed; AA reopened as Q9

### Where the project is
- **S1, S2, S3 done** (doc 04 §10); S4 (dock and tree on a pad) not started. Nothing committed.
- S3 decisions (doc 07 §2.2): rasters at 2x the largest on-screen size (`resvg_py`), BC7, 2048 atlas pages extruded 4 px; the 52
  field sprites take 2 pages, 11.2 MB of VRAM.
- **Config fix**: `DefaultEngine.ini [SystemSettings] r.ScreenPercentage.Default.Desktop.Mode=0`, `r.ScreenPercentage.Default=100`.
  The engine's desktop default rendered a 1080p window at 810p (4K at 1080p) and upscaled, the biggest blur in the chain.
- **S1's AA choice reopened**: its windowed runs had been made on that upscaled image. Re-run at 100 %: the choice is no AA vs
  FXAA, a look question, now **Q9 in doc 01 §3** (image sent to the designer: `build/q9_edge_smoothing.png`). FXAA stays set.
- Spike material (delete after S4): `Source/Pawpocalypse/Dev/Spikes/` (SpikeLook, SpikeCrowd, SpikeArt), the SPIKES deps in
  `Pawpocalypse.Build.cs`, `tools/spikes/`, `Content/Pawpocalypse/Dev/Spikes/S1..S3`, `SourceArt/png/spike_s*` (gitignored).

### What happened this session
1. Offline: `tools/spikes/s3_raster_pack.py` (raster variants r1x / r2x / Godot-native, shelf packer, ideal and Godot-equivalent
   references); Unreal: `SpikeArtActor` + `spike_s3_setup.py` (BC7 and uncompressed pages, atlas material, `L_SpikeArt`);
   `spike_s3_runs.ps1`, `spike_s3_compare.py` (interior error, whole-cell error, sharpness vs ideal).
2. A different Unreal project on this machine (`C:\Games\Cabbage`, packaging) kept running `UnrealEditor-Cmd`; the editor guard
   aborted twice. Its processes never touched this project (git status unchanged). The S3 scripts now guard only against an
   editor open on Pawpocalypse (window title), since the command lines of those processes are not readable.
3. The first capture of the batch drew nothing (shader compile); rerun. Captures alone did not show the screen's blur; windowed
   back-buffer runs found it, the screen-percentage cause was isolated with forced-100 % and no-AA runs, and the config fix was
   verified by a run without overrides.
4. Docs: 04 (§10 S3 + S1 amendment), 07 §2.2 and §4, 03 §7, 01 (Q9), 11, 12, this log; KB 2D/02 §4.3, Unreal/24, INDEX.

### Open
- **Q9 (designer)**: no AA vs FXAA. If neither, measure supersampling on the Deck-class GPU.
- S4. Then delete all spike code and assets, and phase 0 ends (exit criteria in docs/11).
- Not measured in S3: cooked disk size, trimming/pivots, premultiplied alpha (phase 6).
- Designer: review/commit the phase 0 and spike config changes; Q1-Q9.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S4 from docs/04 §8 (dock and skill tree with Common UI on a
pad; decisions into doc 05)."*

---

## 2026-10-28 (5) - spike S2 done: renderer = instanced quads

### Where the project is
- **S1 and S2 done** (doc 04 §10). S2: the instanced-quad renderer (doc 04 §5.1) stays: 13 draw calls for 774 items, game
  thread 0.6 ms, worst-case GPU 5.15 ms at 1280x800 on an integrated GPU weaker than the Deck's. Paper2D rejected (~2,200 draw
  calls). Slate is the measured fallback (half the GPU time on the weak GPU). S3 and S4 not started. Nothing committed.
- A packaged Development build exists in `build/package_s2` (gitignored; spike only, not a release). `Config/DefaultGame.ini`
  gained `+DirectoriesToNeverCook` for `/Interchange` and `/NNEDenoiser`: **without it the project does not cook** (the phase 0
  plugin trim made their code editor-only while engine config still references their content).
- Spike material (delete after S3/S4): `Source/Pawpocalypse/Dev/Spikes/SpikeLookActor.*`, `SpikeCrowdActor.*`, the SPIKES deps
  in `Pawpocalypse.Build.cs`, `tools/spikes/` (S1 and S2 setup/run/compare), `Content/Pawpocalypse/Dev/Spikes/S1`, `/S2`,
  `SourceArt/png/spike_s1`, `spike_s2` (gitignored).

### What happened this session
1. Built the S2 crowd (fake 60 Hz sim, 774 items) with three renderers: ISM with an atlas (per-instance UV offset), Paper2D per
   item, Slate leaf widget (band sort, and a strict per-creature-layer variant).
2. Editor sanity runs: the first ISM run drew nothing (shaders still compiling); the additive material drew squares (fixed:
   RGB x A).
3. First package failed to cook (Interchange/NNEDenoiser, see above). Second: Paper2D drew nothing (its materials loaded by path
   were not cooked) and ISM was still empty on the iGPU after 120 frames (PSO precaching). Fixed: sprites reference their
   materials, warm-up 6 s, a screenshot per run.
4. Third package: 12 runs, all checked by screenshot. Paper2D's soft layers still mis-rendered (the setup's `load_asset` could not
   find `/Paper2D/TranslucentUnlitSpriteMaterial`); recorded as a caveat, the verdict does not depend on it.
5. A process slip, for the record: one edit script failed (a heredoc mangled its match) while the rebuild pipeline had already
   been launched in the same step; the pipeline was stopped before it built anything wrong and no processes were left.
6. Docs: 04 (§5, §10 S2), 03 §7, 11, 12, this log; `tools/trim_game_plugins.py` docstring; KB 2D/02 §4.2, Unreal/19 §3.1,
   Unreal/24, INDEX.

### Open
- S3 (SVG -> PNG -> atlas -> import; `resvg_py` is installed; compression BC7 vs UserInterface2D; raster scale) and S4 (dock and
  skill tree on a pad).
- Why ISM costs 2x Slate's GPU on the weak iGPU (deferred passes? fill rate of large translucent zones?): look in phase 2/8
  with Insights before the Deck pass.
- Designer: review/commit the phase 0 + spike config changes; Q1-Q8.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S3 from docs/04 §8; record the numbers in §10, doc 07 and
Knowledge/2D/02."*

---

## 2026-10-28 (4) - spike S1 done: AA = FXAA

### Where the project is
- **Spike S1 is finished** (doc 04 §10): exact colour proven (within 1 offscreen, within 2 on the real screen, 1080p and
  1280x800), exposure 0 EV, colour-gate method decided, **AA = FXAA** (`r.AntiAliasingMethod=1` now in `DefaultEngine.ini`,
  confirmed live). S2-S4 not started. Still nothing committed.
- Spike material stays until S2-S4 are recorded (S2 can reuse the S1 scene): `Source/Pawpocalypse/Dev/Spikes/`, `tools/spikes/`,
  `Content/Pawpocalypse/Dev/Spikes/S1/`, the SPIKE deps in `Pawpocalypse.Build.cs`.

### What happened this session
1. With the designer's go-ahead, nine windowed runs (back-buffer readback; `tools/spikes/spike_s1_runs.ps1 -Windowed`).
2. The first windowed run showed the field at 0.57x: the local player's default `MaintainYFOV` makes `OrthoWidth` the visible
   **height**. Fixed in the spike (`OrthoWidth = 1080` + `MaintainYFOV` override); **doc 04 §6 corrected** (it said
   `OrthoWidth = logical field width`).
3. Screen comparison: FXAA smooth with no ghosting (0.33 ms GPU); TAA smears moving faces; TSR blurs and ghosts (1.16 ms); forward
   MSAA leaves masked outlines jagged. Decision FXAA.
4. Docs: 04 (§4 choice, §6 camera, §10 results), 03 §7, 11, 12, this log; KB 2D/02 §3 and §4.1, Unreal/24, INDEX.

### Open
- S2 (renderer for 320 enemies; packaged-build timings at 1080p and 1280x800), S3 (raster pipeline; `resvg_py` is installed),
  S4 (dock and tree on a pad). Translucent layers (glows) were not built in S1: check them in phase 6.
- Designer: review/commit the phase 0 changes; Q1-Q8.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S2 from docs/04 §8; record the numbers in §10 and
Knowledge/2D/02."*

---

## 2026-10-28 (3) - spike S1: colour proven exact; AA method still open

### Where the project is
- Phase 0 step 5 (spikes) is under way. **S1 colour: done and decided** (doc 04 §10). **S1 AA method: open**, it needs runs with a
  real window. S2-S4 not started. Still nothing committed; the step 1-4 settings diff still waits for the designer's review.
- A plan page for the whole port was published for the designer (claude.ai artifact "Pawpocalypse Unreal Port", built from
  docs/11, 04 §8 and 01 §3).
- Temporary spike material, to delete once S1-S4 are recorded: `Source/Pawpocalypse/Dev/Spikes/SpikeLookActor.*` (and the
  `RHI`, `ImageCore` deps in `Pawpocalypse.Build.cs`, marked SPIKE), `tools/spikes/` (setup, runs, compare),
  `Content/Pawpocalypse/Dev/Spikes/S1/` (40 textures, 3 materials, `L_SpikeLook`), `SourceArt/png/spike_s1/` (gitignored).

### What happened this session
1. Sprites for S1: the official resvg binary download was refused by the permission check, so the 40 sprites (33 creatures + 7
   bosses at 2x) were rasterised with **Godot's own SVG renderer** from an empty scratch project with `APPDATA` sandboxed. The
   Godot repo stayed untouched (no new writes, clean `git status`). Afterwards it turned out `resvg_py` was installed all along
   (doc 07 §1 says so; an earlier check was misread), so S3 can use it.
2. The spike scene: ortho camera, 21 palette swatches, a static sprite grid, 200 movers, doc 04 §3 post-process; readback, timing,
   PNG and report to `Saved/Spikes/S1/`.
3. **Colour: all 21 swatches within 1 of the palette** at exposure bias 0 EV in every AA mode; Tone Curve 1 fails it (worst 35).
4. Traps found and written into the KB (2D/02 §4.1, Unreal/03 §1, Unreal/24): screen readback is black under `-RenderOffscreen`;
   render-target gamma (`RTF_RGBA8` + `bForceLinearGamma=false` is the only right combination); a scene capture does not show the
   screen's AA; the material Alpha to Coverage flag breaks on desktop; plane UVs need yaw +90; with no pawn the controller resets
   the view target after BeginPlay.
5. AA timings (GPU ms incl. capture): none 0.52, FXAA 0.57, TAA 0.64, forward MSAA 0.69, TSR 1.40. Option A (MSAA + A2C) is out.
6. Docs updated: 04 (§3.5 readback method, §4 option A note, §10 results), 07 (§2.1 how a design is changed), 11 (status), 12
   (pitfalls), 13 (sources note), 00 (temporary spike folders).

### Open
- **S1 AA decision** needs windowed runs (a game window appears for ~20 s per run on the designer's desktop): candidates forward
  + MSAA 4x (plain masked), TAA, FXAA, TSR, at 1080p and 1280x800. Ask the designer before running them.
- Then S2 (renderer), S3 (raster pipeline, can use `resvg_py`), S4 (dock and tree on a pad).
- Designer: review and commit the phase 0 settings diff; questions Q1-Q8.

### Next step
After the designer agrees to windowed runs: *"Finish spike S1: run the AA candidates windowed per doc 04 §10, decide, set
`r.AntiAliasingMethod` in DefaultEngine.ini, record in §10 and Knowledge/2D/02."* Then S2.

---

## 2026-10-28 (2) - phase 0 steps 1-4 done: settings, modules, maps, PawCore test build

### Where the project is
- Phase 0 steps 1-4 of docs/11 are done; **step 5 (spikes S1-S4) has not started**. Nothing is committed: every change below is in
  the working tree, waiting for the designer to review the settings diff (a phase 0 exit criterion) and ask for a commit.
- Builds from the command line: **Editor, Game Development, Game Shipping** (all `Result: Succeeded`, no warnings from our code).
  The game boots headless (`-game -nullrhi`): `LogPaw: Pawpocalypse started; rules library: PawCore`, `L_Title` loads,
  `Game class is 'PawGameMode'`.

### What happened this session
1. Read the KB pieces docs/11 step 1 names (INDEX, 2D/02, Unreal 01, 12 §1, 24, the relevant parts of 08, 17, 19) and docs 00-05, 09-11.
2. **Settings** (docs/03 §7, now with an "As set in phase 0" paragraph): Lumen, RT, Substrate, Nanite, VSM, distance fields,
   static lighting, auto exposure, motion blur, bloom, AO, lens flare, skin cache all off; local exposure contrast 1.0 (the template
   had 0.8, which shifts flat colours); maps `L_Title` (game default) / `L_Realm` (editor startup); `PawGameInstance`,
   `PawGameMode`; `bUseSplitscreen=False`; `CommonGameViewportClient`; high DPI + linear ShortestSide curve; always-cook
   `/Game/Pawpocalypse`; `bEnableEnhancedInputSupport`; Enhanced Input user settings (in `DefaultInput.ini`: the class is
   `config=Input`). **Doc bug fixed**: docs 03/05 said the template's Common UI cvars were already in `DefaultEngine.ini`; they were
   in `DefaultGame.ini` (ignored), now moved. Every cvar and the map/class settings were read back live in a headless editor; the
   rest were checked against the engine headers (property names, ini file per class).
3. **Plugins**: CommonUI, EnhancedInput, PythonScriptPlugin on; EditorScriptingUtilities editor-only; **154 engine plugins
   restricted to the Editor target** by `tools/trim_game_plugins.py` (re-runnable, scans the engine). First attempt (69 runtime
   plugins) still linked ControlRig/RigVM/GeometryCache because editor-only plugins depend on them; restricting those too took the
   game from 64 linked plugin folders to 34. Shipping exe 112 MB (unpackaged, before any content). Paper2D stays until S1/S2.
4. **Modules** (docs/03 §1): `PawCore` (pure C++, `NoPCHs`, placeholder `Pc::LibraryName()` until phase 1; `PcCore.h` defines
   `DLLEXPORT`/`DLLIMPORT` itself because UBT's `PAWCORE_API` needs them and PawCore includes no engine header), `Pawpocalypse`
   (`Core/`: thin `UPawGameInstance`, `APawGameMode` with no pawn, `APawPlayerController` with the cursor shown, `LogPaw`),
   `PawpocalypseEditor` (empty). The other `Source/Pawpocalypse/` folders get created when their first file arrives.
5. **`tools/build_pawcore_tests.ps1`**: PawCore + `Source/PawCore/Tests` into one exe with plain MSVC (`/std:c++17 /W4 /WX`),
   doctest 2.4.11 (MIT, `Tests/ThirdParty/doctest/`). 1 test passes; **negative-tested**: a wrong return value fails (exit 1) and a
   compile error fails (exit 1); restored code passes.
6. **`tools/create_maps.py`**: created the empty `L_Title` and `L_Realm` under `Content/Pawpocalypse/Maps` (LFS); only creates
   missing maps.
7. Knowledge base: new `[Verified]` entries in Unreal/01 §2.1 and §2.3.1 (vcvars stderr in PowerShell; engine-free module:
   DLLEXPORT, Tests guard), Unreal/19 §3.1 (plugin trimming that actually works, check the `.exe.rsp`), Unreal/17 §2 (reading
   settings from Python; which ini each settings class reads), Unreal/08 (template writes Common UI cvars to the wrong file),
   Unreal/03 §1 (no-pawn game mode), 2D/02 §3 (cvars confirmed live; template local exposure 0.8), Unreal/24 rows, INDEX rows.

### Open
- **The designer reviews the settings diff** (`git diff Config Pawpocalypse.uproject`), then asks for a commit if happy. Untracked
  files to stage explicitly: `Content/Pawpocalypse/Maps/*.umap`, `Source/PawCore/`, `Source/Pawpocalypse/Core/`,
  `Source/PawpocalypseEditor/`, `tools/build_pawcore_tests.ps1`, `tools/create_maps.py`, `tools/trim_game_plugins.py`.
- Not verified live: `CommonGameViewportClient` actually in use (needs a run with a renderer; check it in S1).
- Left for later steps on purpose: `r.AntiAliasingMethod` (S1), Common UI `InputData` + controller data assets (S4 / phase 5),
  hardware cursors (phase 5), `PawTokens.h`, `tools/check_all.ps1` (phase 1).
- The designer questions Q1-Q8 (docs/01 §3) are still unanswered; none blocks the spikes.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S1 from docs/04 §8; record the numbers in §10 and
Knowledge/2D/02."* Before it: the designer's review of this session's diff.

---

## 2026-10-28 - the port is planned; nothing is built yet

### Where the project is
- `C:\Games\pawpocalypseUE\Pawpocalypse`: a **blank UE 5.8.3 C++ project** (`5.8.3-58210709`) created by the designer, plus the
  complete port handoff. Git repo on `master` (LFS configured), pushed to **`https://github.com/Keshikul/PawpocalypseUE`**
  (its own repo, separate from the Godot one: designer, 2026-10-28).
- **No engine settings changed yet.** The template still has Lumen, ray tracing, Substrate, VSM and distance fields ON and the
  OpenWorld startup map: phase 0 step 3 fixes that (docs/03 §7). **Do not build anything on the template defaults.**
- The Godot game (`C:\Games\pawpocalypse`, GitHub `JanePonomareva/pawpocalypse`, itch 0.2.5) is the specification and keeps
  shipping until parity (docs/01 D12).

### What happened this session
1. Read the whole knowledge base (`C:\Games\Knowledge`: Unreal 00-26, Steam, PlayStation, Godot 03-05) and the Godot project's
   design and technical docs (`pawpocalypse.md`, GDD v9, technical design, outstanding, handoff headings, data schemas).
2. Web research on 2D in Unreal and on 2D optimisation (sources: docs/13). Key findings: Paper2D gets little development; one
   Actor per sprite does not scale; the tonemapper and auto exposure shift flat colours; translucency sorts per component, never
   inside an instanced component; masked edges need MSAA + alpha-to-coverage or temporal AA; texture streaming blurs sprites;
   no production SVG import; software cursors lag; idle games need their own unfocused throttle; Valve suggests 1 MiB pak padding
   alignment for SteamPipe deltas.
3. Wrote the handoff: docs 00-13, `CLAUDE.md`, and three generated appendices.
4. Exported every Godot `/data` resource to `Data/*.csv` (266 resources, 11 classes, enums by name) with
   `tools/export_godot_data.py`; listed the 456 constants living in Godot code (`tools/list_godot_code_constants.py`, appendix C);
   mapped all 79 Godot scripts (29,258 lines) to their Unreal destinations (`tools/make_port_map.py`, appendix B).
5. Designer, mid-session: **"No lumen, no raytracing"** (recorded in CLAUDE.md, docs/03 §7, docs/04); asked for 2D optimisation
   research and for the knowledge base to be split by dimension.
6. Knowledge base: new `2D/` folder (`01-2d-optimization-best-practices.md`, `02-unreal-2d.md`), `3D/` (an index into the
   3D-specific Unreal docs, which were NOT moved because the global CLAUDE.md links them by path) and `VR/` (empty placeholder);
   README and INDEX updated. The knowledge folder is not a git repository.

### Open: questions for the designer (docs/01 §3)
Q1 what the move must deliver (consoles? Deck? look?); Q2 freeze design until parity (recommended yes); Q3 import itch saves
(recommended yes); Q4 Next Fest Feb 2027 demo from Godot (recommended); Q5 2x rasters for 4K (recommended); Q6 exact Godot colours
first (recommended); Q7 music licence unchanged; Q8 Boba unchanged. **None blocks phase 0.**

### Open: decisions the spikes must make (docs/04 §8, record in docs/04 §10 and Knowledge/2D/02)
S1 anti-aliasing method and exposure value (colour readback within +/-2); S2 renderer (instanced quads vs Paper2D vs Slate canvas);
S3 raster scale, compression, atlas size, OGG import; S4 dock and skill tree widget structure on a pad.

### Next step
Open a session in `C:\Games\pawpocalypseUE\Pawpocalypse` and run:
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then do phase 0 steps 1-4 of docs/11-roadmap.md. Do not start the spikes."*
Phase 0 step 2 (git) is already done. Push only when the designer asks in that message.
