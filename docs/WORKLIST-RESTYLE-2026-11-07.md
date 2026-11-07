# The restyle: the plum look (plan, 2026-11-07)

**Status (2026-11-07, HANDOFF-LOG (65)): R0 and R1 DONE; next R2 (fonts).** D1 answered: M PLUS Rounded 1c. D2 answered: the
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
- Fetch D1's faces (OFL) into the repo, instance variable fonts to static weights (fontTools), import as `UFontFace`,
  one composite `F_Paw` with Noto CJK / Thai sub-fonts per culture (doc 07 §5, KB U/09 §5).
- **Gate `tools/check_font_coverage.py`** (from the 2026-11-07 test): every character of every language's strings against
  the composite's faces; negative test: a face with a script removed must FAIL.
- Titles and numbers: Slate outline (`OutlineSettings`, sizes by eye, KB U/09 §3) and shadow tokens.
- Exit: coverage gate green and seen to fail; the pseudo-locale tour shows no overflow at 1280x800 / 150 %.

### R3: chrome (2 sessions)
- `sweet_ui.py`: plum plates with the solid drop shadow; the nine-patch rules in its header still hold (four states, one
  silhouette; radius under the margin; flat middle). Or, where cheaper, rounded-box brushes with outline (KB U/09 §4: crisp
  at 4K, no texture); decide per piece and record it.
- Re-import under the same asset names; find the Widget Blueprint parts that hold copied brushes or colours and restyle
  them with `bake_widgets.py --graft "Class=restyle:Part"` (never re-bake).
- Exit: `Pawpocalypse.UI.Chrome` asserts the new brushes; every screen in the tour wears plum chrome.

### R4: screens and layout (4-6 sessions)
- In the Widget Blueprints (the asset owns the layout): the HUD (pill counters, the plum plaque, the Pounce clock, the next
  biome bar), the dock (bigger rows, icon tiles, the buy toggle, the kittens strip pinned), the main menu (the plum panel,
  Continue with its slot line, Boba's pinned card), the skill tree (bigger round nodes with name and level chips, jobs as
  rounded squares, the detail panel; `PcTreeLayout` positions untouched, drawn at a larger scale), card pick, pause,
  settings, controls, job picker, victory, ceremonies, toasts, prompts.
- **Text floor:** nothing under 16 px at 1080p and 100 % UI size; secondary text in Lilac, never below 4.5:1.
- Pad focus: the Butter ring on every focusable thing; `padplay_tour.py` focus tours pass.
- Exit: each screen beside its canvas board (§6) passes review; the focus tours pass; no overflow at 150 %.

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
