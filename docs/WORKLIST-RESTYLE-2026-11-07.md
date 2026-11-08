# The restyle: the plum look (plan, 2026-11-07)

**Status (2026-11-08, HANDOFF-LOG (68)): R0, R1, R2, R3 DONE (the colour flip, then the shapes: Deep Plum edges, one solid
shadow, the pressed button sinking); next R4.** D1 answered: M PLUS Rounded 1c. D2 answered: the
cat stays grey. Designer: restyle now, before slice 6's 6b-6d. Written after a design review of the concept in the browser (§3).

## 1. What was decided, and what it changes

- **Designer, 2026-11-07:** "The unreal version does not have to match godot, they can be different." Then: **"Plum
  everywhere."** The look of the Unreal build (palette, ink, chrome, fonts, layout, art, effects) stops following Godot.
- **What still follows Godot exactly:** every rule and number (`PawCore`, `Data/*.csv`, the fixtures, pawsim, the
  sabotages), behaviour and timing (`PcJuice`, `PcTween`, ceremonies), content, sounds, controls, languages. A restyle
  never changes a rule; a rule change still goes through Godot (CLAUDE.md).
- **The reference is the concept canvas**: https://claude.ai/artifact/3wbqpr2HT4daaSmtHtNT2K (private to the designer).
  The boards that define the look: "Original, plum menus" (the field and HUD), "Main menu (plum)", "Skill tree (plum)",
  "Font test: 21 languages", "Level up: keep one card", "The cast" (shapes only: its cream ground is superseded), "Style
  rules" (the rules, not its cream). The cream boards and the three alternatives (Grubby, Night Shift, Creature Feature)
  are history, not spec.
- **Records to update in R0:** `CLAUDE.md` ("same game: ... look", the Godot-is-spec paragraph, the hard rule list),
  `docs/01` (a new decision row, and Q6), `docs/04` §3 (colour exactness now means "exact to `palette.py`"), `docs/05` §7,
  `docs/07` (the art pipeline is ours now), `docs/09` (which gates compare with Godot), `docs/11` (where this sits).

## 2. Open decisions (designer)

| # | Question | Recommendation |
|---|---|---|
| D1 | **Font.** A: M PLUS Rounded 1c (Black titles and numbers, ExtraBold text), one family. B: Dela Gothic One titles + Nunito 800/900 text. | **A.** Covers all Latin and Cyrillic languages, Vietnamese and Japanese; only ko/zh/th fall back to Noto; one look in every language. The logo is art (never translated), so it may use any face, Lilita One included. |
| D2 | **The cat's colour.** The concept drew a ginger cat; the game's cat is warm grey on purpose (`palette.py` rule 4: it must clear every ground, light and dark). | **Keep grey**, give it the concept's expression and proportions. Ginger fails rule 4 on the Backyard lawn and the Litterbox sand and changes a design decision the restyle does not need. |
| D3 | **The paw cursor and the aura** (design review finds 1 and 2, §3). | Decide on the review sheet in R5; recommendations there. |

## 3. Design review of the concept (2026-11-07, every board seen rendered in the browser)

Fixed on the canvas the same day: the skill tree's bottom nodes (Unicorn, Aura Reach, Aura Power) ran under the prompt bar;
the main menu's small cloud sat on the logo; the font board left a white strip at its foot.

Open, to settle in the phase that owns them:
1. **The paw cursor reads as a lamp.** Flipped to strike downward, its arm stub points up into nothing. Draw the strike paw
   coming in from the top edge of a short motion arc, or keep the existing open/closed cursor pair
   (`ui_cursor_paw_open/closed`) restyled; never a free-floating arm. (R5)
2. **The aura muddies the lawn.** Bubblegum pink at 13 % over green blends to olive grey. Pink is the reward colour
   (research board); the aura is a threat. Use a bold dashed ring with no fill, or a 10 % white lift inside it; the canvas
   board "Original, plum menus" has an Aura tweak to compare. (R1 tokens, R5 art)
3. **Five boards are still cream**: Level up (card bodies), The cast, Twelve biomes, Style rules, Palette research. Plum
   everywhere means: card bodies Bone `#F6EEF8` on plum, never cream; the Style board is rewritten as the spec in R1.
4. **The concept's text is set in Lilita One**, which fails seven of the game's languages (font test). Every board is
   re-checked in the chosen font once D1 is answered.
5. **Small labels.** The dock's secondary lines (13 px) and the tree's level chips (12 px) are below what a TV at 3 m reads;
   the game already has a UI size option. R4 sets a floor (§5) and checks it at 1280x800 and 150 % UI size.
6. **Empty space in the dock** below the last shop line in the mock-up: the real dock has fifteen lines; R4 keeps the list
   scrolling with the kittens strip pinned, never a void.
7. The fence pattern draws a faint seam on the canvas (a browser artefact of the mock-up, not of the game): ignore.

## 4. The look spec (what R1 writes into `tools/art/palette.py` and the Style board)

- **Tokens.** Plum `#2A1A33` (panels; the ink of every outline and letter), Deep Plum `#120A17` (panel edges, drop shadows),
  Plum Raised `#3E2A4A` (rows, inactive tabs), Bone `#F6EEF8` (text on plum; card bodies), Lilac `#DCCFE4` (secondary text),
  Butter `#FFD23F` (focus, the selected thing, numbers), Catnip `#5FD068` (buying, Catnip), Stardust `#B98CFF` (Stardust,
  rewards only), Crimson `#D2263F` (threat, bosses, can't-afford), the field's lawn greens kept as they are. Final values
  come out of `palette.py`'s rules, not this list: where a rule fails, the rule wins and the canvas is updated.
- **Shapes.** One ink, one weight: every outline in Plum at one width at every size. Ellipse first, then fatter. Flat
  fills, no gradients, no glow; one shadow, solid, straight down (panels 8 px, buttons 4-5 px at 1080p). Rounded corners
  16-30 px. Pressed buttons sink by their shadow.
- **Faces.** The cat: half-lidded, mismatched, unimpressed, one fang; never sparkly. Prey: wide eyes, tiny pupils, a sweat
  drop when hunted. Mean, not cute; chubby, not baby.
- **Type.** D1's family; titles and numbers in the heavy weight with a Plum outline and a solid drop shadow; text never
  below the floor in §5.

## 5. Phases

Before R0: **finish phase 6 slice 6 (6b, 6c, 6d)** as planned in `WORKLIST-SLICE6`. They port behaviour against Godot
fixtures and their pieces will be restyled with everything else; restyling first would mean styling screens that do not
exist yet. Each phase below ends as every slice does: everyday `check_all` exit 0, sabotages seen CAUGHT, docs, HANDOFF-LOG.

### R0: unhook the look from Godot (1 session)

> **DONE 2026-11-07 (HANDOFF-LOG (65))**: `gen_paw_tokens.py` reads `tools/art/palette.py` (no colour moved: 70 tokens
> byte-identical); the Godot scene/script literals are `palette.py LITERALS`; `build_art.py --check` compares with our
> `SourceArt/svg` only; the Godot fixtures' colours pass through the generated `Tests/PcGodotLook.h` (Godot's palette frozen at
> the fork in `tools/art/palette_godot_snapshot.json` -> ours; `Pc::PcFromGodot`; refuses an ambiguous map), sabotage
> `godot_look_off` CAUGHT. The look fixtures `title_look_godot.txt` / `victory_look_godot.txt` are read by no test (comments
> only). Records: CLAUDE.md, docs 00/01 (D14)/04/05/07/09.
- Record the decision (§1 list).
- `tools/art/build_art.py --check` compares the generators' output with the project's own committed `SourceArt/svg`, not
  Godot's `assets/art`. Same for `raster_svg.py --check` and `pack_atlas.py --check` (they already compare with our files:
  verify).
- `tools/gen_paw_tokens.py` reads `tools/art/palette.py` (its `--json`), not Godot's `palette.gd`; `PcBackdropTones.h`
  likewise. The L0 `ui-literals` gate stays.
- List every test that asserts Godot's look (grep `Godot` in `Source/Pawpocalypse/UI/*Tests.cpp`, the `*_look_godot.txt`
  fixtures, `Pawpocalypse.UI.Chrome`, `.Icons`, `.Rainbow`) and split each assertion: behaviour stays against Godot, look
  moves to "equals our tokens/art". Each moved assertion is seen to fail once on purpose.
- `godot_tour.py` keeps working as a before/after tool; add `--ref look` (§6) for the canvas comparison.
- Exit: `check_all` exit 0 with no gate reading Godot's art or palette; the Godot repo untouched (its gate stays).

### R1: the palette (1-2 sessions)

> **DONE 2026-11-07 (HANDOFF-LOG (65))**: `palette.py` PLUM family + rule 6 (ten WCAG pairs, catnip/crimson by lightness);
> INK = Plum `#2A1A33` (the unused black family follows it); tokens `PLUM_*`; all 390 SVGs, the 4 hand SVGs, rasters,
> cursors and the atlas regenerated and imported. **Found:** every baked Widget Blueprint saved its colours as VALUES (158
> stale): new `UPawWidgetBaker::Recolour` + `py tools/bake_widgets.py --run --recolour` (exact old palette colours only;
> `tools/art/palette_widgets_snapshot.json` = the palette the assets carry) and the test `Pawpocalypse.UI.Widgets.Palette`
> (no asset holds a `PawTokens::PALETTE_RETIRED` colour; sabotage `palette_walker_blind` CAUGHT). `PawVfx`'s ink reads
> `Pc::Ink`. The canvas's Style board is the plum spec. Left for R3: the panels are still cream (`UI_*` tokens unchanged on
> purpose: dark ink on cream reads; plum surfaces need the chrome and text together).

**After any palette change from here on:** `py tools/gen_paw_tokens.py`, `py tools/art/build_art.py`, `raster_svg.py`,
`pack_atlas.py`, `import_art.py --run`, then `py tools/bake_widgets.py --run --recolour`, then `check_all`.
- `palette.py`: the plum family (§4) beside Stacklands; INK becomes Plum; every rule re-run; new rules: text tokens reach
  4.5:1 on their panels (3:1 for 24 px and up), Butter focus visible on every panel, Crimson distinct from Catnip in value as
  well as hue (colour-blind safe).
- Regenerate `PawTokens.h`, `PcBackdropTones.h`; the effects (`FPawFxCanvas`, `FPawVfx`) read tokens only.
- Rewrite the canvas's "Style rules" board as the spec in plum (tokens, shapes, faces, type), so the reference matches
  `palette.py`.
- Exit: rules green, tokens regenerated, the Style board updated, a tour of every screen (still old chrome) looks no worse.

### R2: fonts (2 sessions; replaces phase 7's font work)

> **DONE 2026-11-07 night (WORKLIST-NIGHT-2026-11-07, HANDOFF-LOG (66))**: as built in docs/07 §5. F_Paw (M PLUS Rounded
> 1c + Noto CJK `.ttc` by sub-face + Noto Thai), coverage gate (22 languages, self-test fails both ways), every text in
> F_Paw (UMG's default font; the assets followed: their fonts equalled the default), titles and numbers in Black
> (`PawUi::Heavy`, grafted by `restyle:`). Not done here: the pseudo-locale / 150 % tour (the port has neither translations
> nor a UI-size option until phase 7: the language sheets `godot_tour.py --set fonts` stand in; R4 does 150 %).
- Fetch D1's faces (OFL) into the repo, instance variable fonts to static weights (fontTools), import as `UFontFace`,
  one composite `F_Paw` with Noto CJK / Thai sub-fonts per culture (doc 07 §5, KB U/09 §5).
- **Gate `tools/check_font_coverage.py`** (from the 2026-11-07 test): every character of every language's strings against
  the composite's faces; negative test: a face with a script removed must FAIL.
- Titles and numbers: Slate outline (`OutlineSettings`, sizes by eye, KB U/09 §3) and shadow tokens.
- Exit: coverage gate green and seen to fail; the pseudo-locale tour shows no overflow at 1280x800 / 150 %.

### R3: chrome (2 sessions)

> **Colour flip DONE 2026-11-08 night (WORKLIST-NIGHT-2026-11-07 item 8, HANDOFF-LOG (66))**: `palette.py` UI = the plum
> values below (rule 7: thirteen UI pairs, all pass; tightest the boss banner 4.52:1); a new `field_label` token (Bone, the
> fill of labels over the field). The migration is ROLE-AWARE: `PawUi::ForEachColourRole` / `IsTextColour` (a text
> widget's or text style's ColorAndOpacity, a piece's TextColour = text; shadows, outlines, tints, brushes = surfaces),
> `UPawWidgetBaker::RecolourByRole`, `gen_paw_tokens.ui_role_pairs` (refuses ambiguity within a role; a text colour no
> surface token owns moves as a surface too: the sliders' bars), `bake_widgets.py --recolour` (KEEP_INK: WBP_CardFace,
> ink text on the Bone card body). 221 colours migrated in 12 assets (204, then 17 once text colours used as surfaces moved too). The Godot maps (`PcGodotLook.h`, the tree's
> branches) leave the UI table out; the tree tooltip's text goes through the text role (its two Godot literals -> Butter
> and Lilac). Code: 13 text call sites ink -> `UI_TEXT`; over-field labels `UI_FIELD_LABEL`; the Pounce clock's dial
> Bone; `UPawIconText` now APPLIES its `bOverField` (outline + light fill: it never did, ink text hid it); the title's
> logo over-field with a 4 px outline; the title's hint line ink (on the lawn); checkboxes bone / lilac frame; the input
> glyphs keep light keys (`sweet_input.py`: Bone, Lilac wells). Tests: `Pawpocalypse.UI.Widgets.Palette` failed with 76
> retired creams before the migration; new `.ColourRoles` (sabotage `colour_role_text_lost` CAUGHT). **Not done (R3's
> shapes):** the plates' solid drop shadows and Deep Plum edges in `sweet_ui.py` (frames are still INK, invisible on
> plum); the highlighted Travel row is a dull khaki (R4: Butter selection with the focus pass); the title's hint line
> crosses the bushes (R4: a plate).
> **Shapes DONE 2026-11-08 (HANDOFF-LOG (68))**: `sweet_ui.py`'s `plate()`: a Deep Plum edge (3 px) and ONE solid Deep
> Plum shadow straight down inside the bottom margin (buttons 4 px, panel and window 8, a new small plate 4); the pressed
> button SINKS onto its shadow (the silhouette check: three states one box, pressed the same box moved down by the
> shadow). Margins read off the drawings (`PawUi::ChromeTable`: buttons 16 all round; panel 16/16/16/24; window 24/24/24/32;
> `ui_plate_small` 64x48, 12/12/12/16) with each piece's shadow. **Decided per piece: nine-patch for every plate that casts
> a shadow** (a rounded box cannot draw one, KB U/09 §4): the code's flat plates (the dock, the boss panel, the zone plate,
> the title's menu, windows and Boba badge, the pause and settings windows, the tree's warning and tip, the toast) now wear
> the Window or the small plate, restyled into the assets (`restyle:`; the HUD's dock panel by `restyle:Border_0>Border_2`,
> its auto-name differs between code and asset). `PawUi::PlatePadding` lifts content off the shadow; buttons' content
> margins 16/4/16/8 (Common UI cannot move a pressed label, KB U/09 §4). The title's cluster buttons wear the theme's
> buttons. Palette: `panel_frame` / `button_frame` Deep Plum, new `shadow` and `card_ink` (a card keeps its ink). The
> migration gained a third ROLE, the EDGE (a brush's outline: `PawUi::IsEdgeColour`; `ui_role_pairs` text / surface / edge;
> `--recolour` moved the ink frames left in the assets to Deep Plum, KEEP_INK's card frame kept). Found: the quick bar's gear
> and door, the catches tab's cards and the boss / kitten badges were drawn in the button colour (cream once, Plum Raised
> since the flip: they vanished on plum): `sweet_ui.LIGHT` (Bone). Not here: the focus ring (coral) and the cards (R6).
- `sweet_ui.py`: plum plates with the solid drop shadow; the nine-patch rules in its header still hold (four states, one
  silhouette; radius under the margin; flat middle). Or, where cheaper, rounded-box brushes with outline (KB U/09 §4: crisp
  at 4K, no texture); decide per piece and record it.
- Re-import under the same asset names; find the Widget Blueprint parts that hold copied brushes or colours and restyle
  them with `bake_widgets.py --graft "Class=restyle:Part"` (never re-bake).
- Exit: `Pawpocalypse.UI.Chrome` asserts the new brushes; every screen in the tour wears plum chrome.

**Inventory (night 2026-11-07, item 7 of `WORKLIST-NIGHT-2026-11-07.md`; read from the sources):**
- **The flip is data, three ways.** The `UI` tokens in `palette.py` reach (a) the code through `PawTokens.h`, (b) the
  nine-patch chrome through `sweet_ui.py` (buttons x 4 states, panel, window, dock, slot, card frame, bar track and fill:
  re-rastered, re-imported under the same names), (c) the Widget Blueprints' saved colour values through `--recolour`.
  Mechanism per piece: unchanged. Nine-patch stays nine-patch (crisp at 1x, the pipeline exists), and the 46 `PawUi::Rounded`
  brushes (14 files) stay rounded boxes; their fill and frame colours are values the walker recolours.
- **The blocker: one old colour, two new ones.** Today `#2A1A33` is both the text and the frames. Plum everywhere
  splits it: text Bone, frames Deep Plum. `colour_pairs` refuses an ambiguous map (by design), so the recolour must
  become ROLE-AWARE: a colour inside a text's `ColorAndOpacity` (UTextBlock, FTextBlockStyle) maps through the text map,
  every other colour through the surface map.
- **Code.** `PawTokens::INK` is a text colour at 12 call sites (titles, slot titles, tooltips, the job picker): they become
  `UI_TEXT`. INK stays INK where it is an outline or a frame (47 uses in 11 UI files, each looked at). `UI_PANEL` (18 uses)
  is also the fill of every label over the field (`MakeText` bOverField): that needs its own token (Bone), or the
  counters turn plum on the lawn.
- **Proposed values** (as built differs: frames stayed INK and focus coral, left for R3's shapes and R4; the built values are in `palette.py` and the DONE note above) (rule 6 decides; each pair added to `PLUM_PAIRS`): panel = Plum; panel_band and button = Plum Raised;
  panel_frame, button_frame, slot and button_pressed = Deep Plum; text = Bone; text_muted = Lilac; focus = Butter; good =
  Catnip; danger = Crimson; card_plate = Bone (card bodies Bone, their text stays ink); the five tab colours kept (pastel
  tabs on plum); catnip and stardust kept.

### R4: screens and layout (4-6 sessions)
- In the Widget Blueprints (the asset owns the layout): the HUD (pill counters, the plum plaque, the Pounce clock, the next
  biome bar), the dock (bigger rows, icon tiles, the buy toggle, the kittens strip pinned), the main menu (the plum panel,
  Continue with its slot line, Boba's pinned card), the skill tree (bigger round nodes with name and level chips, jobs as
  rounded squares, the detail panel; `PcTreeLayout` positions untouched, drawn at a larger scale), card pick, pause,
  settings, controls, job picker, victory, ceremonies, toasts, prompts.
- **Text floor:** nothing under 16 px at 1080p and 100 % UI size; secondary text in Lilac, never below 4.5:1.
- Pad focus: the Butter ring on every focusable thing; `padplay_tour.py` focus tours pass.
- Exit: each screen beside its canvas board (§6) passes review; the focus tours pass; no overflow at 150 %.

**R4's steps (2026-11-08, read off the canvas boards' sources and the tour):**
- **R4a focus and selection.** The ring Butter (4 px, 4 px out); the held toggle (the open tab) a Butter plate sunk like
  the pressed one; "the selected thing" (a settings section, a biome just reachable, the Bait calling) a Butter plate with
  ink text, never a tint (any translucent yellow over plum is khaki). Tokens `focus`, `selected`, `selected_text`.
- **R4b the text floor.** 16 px at 1080p = 12 pt (`PawUi::FontPt`): a test walks every asset and code layout; the
  constants under 16 rise; the assets' saved sizes migrate. Exempt, by name: the keycap label inside a glyph (part of the
  icon) and the tree's captions (they scale with the player's zoom).
- **R4c the title**, **R4d the HUD**, **R4e the dock**, **R4f the skill tree**, **R4g the rest** (card pick, pause,
  settings, controls, job picker, victory, ceremonies, toasts, prompts): each against its board. Layout moves go into the
  assets by graft (`slot:Part` copies a part's slot from the code layout). Kept on purpose, not the canvas: the dock stays on
  the LEFT (Godot's layout, the field's framing and every focus tour); no new player-facing words (the concept's tagline,
  "Biome 1 of 12", the kittens strip's lines) without the designer.
- **R4h the checks**: `padplay_tour.py` focus tours; the tour at 1280x800 and at 150 % (Slate's application scale); the
  sheets beside the boards.

**R4 progress (2026-11-08):**
- **R4a DONE**: tokens `focus` / `selected` Butter, `selected_text` ink (rule 7 pairs); chrome `ui_button_selected` (the
  pressed shape in Butter; the silhouette check covers it); the open tab on it, its label and count ink; `UPawButton::
  SetCallLook` (the Butter face under ink words, never a tint) for a biome just reachable, the Bait calling, the open settings
  section; the ring 4 px Butter, 4 px out (`FocusRingWidth`); the call plate fills the plate's FACE only
  (`ButtonFacePadding`, grafted `slot:Plate` into WBP_Button). The focus colour is painted, never saved
  (`gen_paw_tokens.UI_PAINTED_KEYS`). Sabotages `selected_plate_pressed_again`, `call_label_not_ink`, `focus_ring_thin`.
- **R4b DONE**: `PawUi::UnderTextFloor`, `Pawpocalypse.UI.Widgets.TextFloor` (failed on 11 texts in 5 assets, then passed),
  `bake_widgets.py --textfloor`; nine constants raised to 16 (Godot's values in their comments). Sabotages
  `text_floor_blind`, `small_text_again`.
- **R4c DONE (the title)**: the logo Butter 112 px, outlined, shadowed, tilted, top left; the menu on the left, words left at
  the board's sizes, Continue the Butter call while it can continue; the hint on a small plum plate bottom left; Boba's card
  on the right, tilted, a photo well, the name a Butter title; the cluster and the version (ink on the lawn) bottom right.
  New graft ops: `slot:` (a part's slot and tilt), `text:` (a text's whole look), `remove:`; `restyle:` on a border also
  copies its content alignment; `props:` a button's minimum size; `swap:` refuses a part with children (it lost the
  hint's prompts once: put back by `remove:HintPlate;add:HintPlate`). Test `Pawpocalypse.UI.Title.Plum`; sabotages
  `title_logo_centred`, `continue_not_called`. **Found:** `godot_tour.py` never shot the title, pause or settings (the copies
  in build/tour/ue were last night's; R3's review of those three looked at stale shots): it shoots them now and deletes a
  shot before retaking it.
- **R4d DONE (the HUD over the field)**: the currencies as plum pills (heavy Bone numbers, no outline); the wave line on its
  plum plate (`WavePlate`; the plate hides with it: `UPawHudScreen::WaveShown`); the XP bar a raised track, Deep Plum edge,
  Butter fill; the zone bar's edge Deep Plum. Kept on purpose: the counters top right, away from the dock (the board's
  top left is where our dock is). **Found:** a graft `add:` failed on an auto-name the asset already held
  (`HorizontalBox_0`) after a `remove:` had run, and the asset was saved without the currencies: a graft is now ALL OR
  NOTHING (an op FAILED saves nothing); grafted parts' inner widgets are named. Sabotages `wave_plate_flat`,
  `wave_plate_left_behind`.
- **R4e DONE as far as it goes without art and words (the dock)**: shop rows 56 px tall, the words on the left at 18 px;
  `text_disabled` now 4.5:1 on a locked row (`#B8ABC1`, was 3.8:1: a row the player cannot pay is still READ; rule 7
  raised; 13 asset colours recoloured). Open: the board's icon tiles (R6 art), price pills and second lines (new words: the
  designer), the kittens strip (words). Sabotage `shop_rows_centred`.
- **R4f DONE as far as the fit-all tree allows**: skills ROUND, unlocks rounded squares (`UPawTreeNode::GetFrameRadius`);
  the Stardust readout and the legend on plum pills (grafted `remove:` + `add:`); a prompt line on its own plate takes the
  plate with it when prompts are off (`UPawPromptText`; the title's hint too). Not done: the board's bigger nodes need the
  tree to open zoomed in (a behaviour change: the designer's call), and its detail panel (the tooltip stays). Sabotages
  `tree_nodes_square`, `prompt_plate_left_behind`.
- **R4g DONE (titles)**: `PawUi::ButterTitle` (Butter, Black, ink outline, the solid shadow): the card pick's title (tilted
  -2°, outline 4), the job picker's, the victory's headline. **Found:** the parity test now compares a text's COLOUR too,
  and caught the card pick's subtitle coded in Stacklands' old `DISABLED` grey while the asset held a plum one (code now
  `UI_TEXT_MUTED`, grafted). Sabotage `butter_title_bone` (first MISSED against the Victory test, which reads the asset:
  now aimed at `.Widgets.Parity`, CAUGHT). Pause, settings, ceremonies, toasts, prompts: seen, already plum since R3 / R4a.
- **R4h (checks) run**: `padplay_tour.py` 41 checks PASS (Butter focus everywhere); the tour at 1280x800
  (`--res 1280x800`, shots in `build/tour/ue_1280x800_1`): every screen fits. **At 150 %** (`godot_tour.py --uiscale 1.5`,
  `paw.UiScale`: UMG's `UUserInterfaceSettings::ApplicationScale`; Slate's own application scale does not reach the game
  viewport, measured) **four overflows, OPEN** (they matter once phase 7's UI-size option exists): the title (the logo
  under the menu, Boba's card and the version off screen), the dock's shop page (rows under the quick bar: that page does
  not scroll), the settings window (taller than the screen: Controls runs into Close), the tree (drawn off centre).
  Sheets: `build/look/r4h_150.png`, `build/look/r4h_1280.png`.

### R5: the field (4-6 sessions)
- Regenerate every creature (46 enemies, 7 bosses, the cat's 7 stages and base, 6 weapons, the kitten), the 36 props and
  the 12 backdrops in Plum ink; the faces per §4 (an expression pass in `sweet_shapes.py`: brows, half-lids, the fang).
- Settle the paw strike and the aura (§3 finds 1, 2) on a review sheet first.
- **Every creature goes through the review loop (§6).**
- Exit: atlas rebuilt (`pack_atlas.py`), the packaged frame gate (`packaged_bench.py`) unchanged, all 12 biomes in the tour.

### R6: icons, badges, cards (2-3 sessions)
- 79 skill icons, 31 card pictures, 20 job pictures, 41 achievement badges, tab and quick icons, currency icons, cursors,
  the app icon; the 65 input glyphs only recoloured (their shapes are the platforms' conventions).
- Exit: `Pawpocalypse.UI.Icons` green; contact sheets reviewed against the canvas.

### R7: sign-off (1 session)
- The full tour beside the canvas, every screen; the nightly `-Full`; the packaged-build gate; the designer's playthrough.

Rough total: 18-26 sessions, not measured.

## 6. Testing against the artifact

Every new or redrawn creature, and every screen, is checked against the canvas before it is written into the game.

**The creature review loop** (R5, and every new enemy after the restyle):
1. **Generate a review sheet, write nothing**: `make_sweet_art.py --only <id>` draws the creature at its in-game size
   (`EnemyData.sprite_target_px`: 56-108 px) and at 2x, on the lawn, on its own biome's floor, on a plum panel, and beside
   the cat at the same scale.
2. **Look-lint (automated; new `tools/art/look_lint.py`, an everyday gate once R5 starts):** every fill and stroke is a
   `palette.py` token; every outline is INK at the standard width; no gradient, filter or opacity on a body; the palette
   rules (distance from the cat, separation from every ground it ships on); at 56 px the eyes still cover at least the
   pixel count a check sets on the current cast. Negative test: one off-palette fill must FAIL.
3. **Against the canvas:** the sheet is uploaded to the canvas as a board "Review: <batch>" placed beside "The cast" and
   "Original, plum menus", so it is judged next to the reference at the same scale.
4. **Design review (Claude, with the design guidance):** the checklist below, written into the board's notes as pass or a
   named fault.
5. **The designer approves on the canvas** (a comment). Only then `--write`, import, atlas.

**Screens** (R3, R4): `godot_tour.py --ref look` shoots each screen in the game and composes it beside the PNG of its
canvas board (exported once per canvas revision into `build/look/ref/`, gitignored), into `build/look/compare/`. Reviewed
with the same checklist; differences are either fixed or recorded as a deliberate change to the canvas.

**The checklist:**
- Silhouette: chubby (ellipse-first), readable as itself at 56 px, distinct from the cat and from its biome-mates.
- Face: the cast's rules (§4): the cat unimpressed, prey panicked; mean, not cute.
- Ink: one outline colour and weight; nothing drawn without it except the deliberate exceptions the generators document.
- Colour: palette tokens only; reward pink only on rewards; threat in Crimson; separates from every ground it ships on.
- Flat: no gradients, glow or soft shadows.
- Layout (screens): consistent edges and padding across repeated rows; nothing clipped or overlapping; focus visible; text
  at or above the floor and at contrast; empty space intended, never a void.
- Text: the game's register: short, mean and funny; never "prestige"; Boba only in menus.

## 7. Risks

- **The parity gates lose their look half.** Mitigated in R0 by moving each assertion, not deleting it, and seeing each fail.
- **Widget Blueprints hold copies.** Some looks were copied into the assets at bake time; R3 lists them before changing
  anything, and restyles by graft.
- **Divergence from Godot's art makes later Godot fixes harder to port.** Rule and number changes still flow from Godot;
  art changes never do again. Record this in docs/07.
- **The concept is a mock-up.** Where it and `palette.py`'s rules disagree, the rule wins and the canvas is corrected.
