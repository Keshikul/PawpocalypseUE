# The Sticker Box restyle (plan, 2026-11-17): D18

**Status (2026-11-17, HANDOFF-LOG (107)):** S0 done (this plan, the decision recorded). **Next: S1, the palette.**
It replaces the plum restyle's look (D14, `WORKLIST-RESTYLE-2026-11-07.md`): that worklist's pipeline, tools and gates stay;
its colours, chrome, fonts and drawings are what this one changes.

## 1. What was decided

- **Designer, 2026-11-17**, on a look concept with three directions: "You don't have to follow the current color or palette or
  ui. Just start fresh", then "Sticker box looks great". Then the order of work: "We need to redesign all enemies according to
  this. Also create all uis concept in this style before moving to unreal changed. All ui screens need to be redesigned. After
  all that is done steam store page assets and descriptions."
- **The approved references** (claude.ai artifacts, private to the designer; their HTML source, which draws every shape in
  code, is readable with the Artifact tool's `read`):
  - Look: **Chonk Looks**, version 1, "Sticker Box": https://claude.ai/artifact/CCUYB9dcTRyt1d5X1dtrMT
  - Every enemy and boss: **Bestiary**, version 4 ("Everything else good" after four fixes):
    https://claude.ai/artifact/B92ezCRMKpDeXShegRWVV8
  - Every UI screen and the kit: **UI Screens**, version 4 ("Good continue"): https://claude.ai/artifact/Homk9frz3jpvik8LYjpmA4
- **What changes:** the palette, the ink, the chrome, the fonts, every field drawing (creatures, the cat's stages, kittens,
  bosses, props, floors, effects, cursors, icons, cards) and the screens' composition where the UI concept differs (§4 S4).
- **What does not:** every rule, number and timing (PawCore, `Data/`, fixtures, pawsim); the words (D15; the concepts use the
  approved `ST_UI` text); the layout rules of doc 02 §13 (the field fills the window, the dock on the right, the status top
  left, Pounce opposite the dock, every menu a dock tab, focus a ring); the generators-as-code principle (every SVG comes from
  `tools/art`, never hand-edited); every gate.
- **Superseded:** D14's plum tokens and M PLUS Rounded 1c as the text face; R5's anatomy drawings (the bestiary replaces
  their shapes; the species cues they defined are kept). R5's open reviews (bosses, imp, rig motion, the cat's pale coats) are
  closed by the bestiary's approval. R6 (icons, badges, cards) becomes S6 here.

## 2. The look spec (the artifacts are the reference; `tools/art/palette.py` is the source of truth once S1 writes it)

- **Ink** `#2A2131` (warm plum-black): every outline and every letter on cream. Creatures 3.6 at a 100 px drawing; props 2.4,
  icons 2.6, no rim on either.
- **The rim**: a white die-cut edge `#FFFDF6`, 4.3 outside the ink, on every creature, the cat, its kittens and mounts, and
  the bosses. Never on props, ground or UI.
- **Two fills a creature** (fur and belly), shared eye white `#FFFFFF` and pink `#F58BA8`; flat, no gradients, no texture.
- **Faces**: enemies panic (huge mismatched eyes, an o mouth, a sweat drop); the cat schemes (crossed pinprick pupils, brows
  down, a fang grin, the tongue out; never sleepy or "unimpressed"); bosses are sure of themselves (pupils forward, brows
  down).
- **UI**: card-stock panels `#FFF8EB` with a 3.5 px ink edge and a hard ink shadow (5, 6); rows `#F4E7CF` with a 2.5 px edge;
  the primary button `#FFC83D` with ink text and a 4 px ink shadow (the brightest thing in its panel); can't-afford `#EADFCB`
  with `#7E7369` text and a faded icon; danger `#C93323` outline; focus a 4 px `#3D7BFF` ring, never a fill; badge `#F2545B`;
  bars on `#E8DABF`, XP and Stardust `#8B6BEF`, Catnip `#5DB33D`; the skill tree's sky `#2D3A63`.
- **Type**: Fredoka 700 for titles and numbers; titles as stickers (yellow fill, ink outline, white rim); Nunito 700-800
  for everything read. Text never sits on the field without a panel.
- **Grounds** (low contrast, creatures carry the contrast): Backyard `#9ACF6C`, Alley `#A9B0BA`, Neighborhood Park
  `#7CC46A`, City Rooftops `#C9B5A7`, Sewers `#6F8F88`, Rival Cat Turf `#D8C878`, Portal Zone `#93A9DC`, Mythic Realm
  `#C7A6EE`, the Kitchen `#ECDFC9` (floor tiles), the Vet's `#BDE0D5`, the Factory `#B3B7BE`, the Litterbox `#E1D2A6`.
  Each with a patch tone and a shadow tone (the bestiary's `BIOMES`).
- **Creature colours**: the bestiary's `L.c` table, one entry per character.

## 3. Open decisions (designer)

| # | Question | Recommendation |
|---|---|---|
| E1 | **Fonts in every language.** Fredoka has Latin and Hebrew only; Nunito has Latin, Cyrillic and Vietnamese, not Greek, Japanese, Korean, Chinese or Thai. | Text in Nunito; titles and numbers in Fredoka where it has the letters, and in Nunito's heaviest weight where it does not (Cyrillic, Vietnamese); M PLUS Rounded 1c stays as the face for Japanese and any script Nunito lacks; Noto CJK and Thai stay. The coverage gate decides; S2 shows the 21 language sheets before anything ships. |
| E2 | **How the rim is made.** | Bake it into each sprite when the SVG is rastered (dilate the alpha, fill white, under the drawing): exact, no runtime cost, no new draw calls. Costs about 5 px of atlas padding per sprite. |
| E3 | **The rig** (moving wings and legs, R5). | Keep it: the new drawings come with the same layers (wings for the moth, bat, flying mouse, seagull, crow, parrot, imp; legs where the drawing has them), turned by the existing rig code. |
| E4 | Layout changes the UI concept makes (S4 list). | As drawn in the concept; each is grafted into the Widget Blueprint (the asset owns the layout). |
| E5 | The concept's open items: locked Realms rows naming what opens them (one new string), the job picker's lines, the tutorial line, Boba's photo. | Drafted as text drafts first (CLAUDE.md), with their 20 translations owed. |

## 4. Phases

Each ends as every slice does: everyday `check_all` exit 0, new and retargeted sabotages seen CAUGHT, docs, HANDOFF-LOG.
After any palette change: `py tools/gen_paw_tokens.py`, `py tools/art/build_art.py`, `raster_svg.py`, `pack_atlas.py`,
`import_art.py --run`, `py tools/bake_widgets.py --run --recolour`, then `check_all` (the restyle worklist's recipe).

- **S0, the decision (done 2026-11-17).** This plan; D18 in doc 01; CLAUDE.md and doc 00 say Sticker Box.
- **S1, the palette (1-2 sessions).** `tools/art/palette.py`: the UI table becomes §2's card stock; the ink becomes `#2A2131`;
  the grounds and creature colours become the bestiary's; every rule re-run (contrast pairs, the cat clear of every ground,
  no shade used twice, creatures clear of their grounds). Where a rule fails, the colour moves and the bestiary is updated;
  a rule never moves to fit a colour. Then the role-aware recolour of the Widget Blueprints (plum surfaces to card stock,
  bone text to ink), `PawTokens.h`, `PcBackdropTones.h`. Exit: rules green, assets migrated (`Pawpocalypse.UI.Widgets.Palette`
  green), a tour of every screen.
  **Scope:** the ink, the UI table and the tree's sky only. The grounds and the creature colours move in S5 with the drawings
  (rule 3's distances are between a creature and its ground; changing one without the other fails them for nothing).
  **Found 2026-11-17 (before any change): the value recolour cannot express Sticker Box.** Plum folded roles together: the
  panel IS the ink (`#2A1A33`), and the frames, the shadow, the pressed button and the key well are one Deep Plum
  (`#120A17`); bone (`#F6EEF8`) is the text, the boss banner's text and the field labels. Sticker Box splits them again
  (panel cream, ink ink; frames and shadow ink, pressed and well cream; boss text ink on a card-stock plate, field labels
  white). `gen_paw_tokens.ui_role_pairs` refuses a map where one old colour becomes two new ones within a role, by design:
  the assets saved values, not roles. The approach, in order:
  1. **An inventory** of every colour the baked assets save: asset, widget, property, role (text, surface, edge), value. A
     new `UPawWidgetBaker` report (`bake_widgets.py --run --colours`), written to `build/`.
  2. Keys no asset saves leave the map, as `focus` already does (`UI_PAINTED_KEYS`).
  3. Keys that are text colours by meaning (`selected_text`, `card_ink`) join `UI_TEXT_KEYS`.
  4. The boss banner becomes a card-stock plate with ink text and a danger tag for SHIELDED, as the concept draws it.
  5. Whatever is still ambiguous is re-applied from the code layout, part by part (`--graft "Class=restyle:Part"` and
     `text:Part`), which S3 and S4 do for every part anyway. Never a guess: a colour the map cannot place is listed.

  **Step 1 done 2026-11-17 (HANDOFF-LOG (108)):** `UPawWidgetBaker::ColourReport`, `py tools/bake_widgets.py --run --colours`
  -> `build/widget_colours.tsv`: **7,549 saved colours in 32 assets.** Where the plum UI colours are, by role and property:

  | Old (key) | Role | Where it is saved | Sticker Box |
  |---|---|---|---|
  | `#120A17` Deep Plum (frames, pressed, well, shadow) | edge (51) | every brush's and button state's outline | ink |
  | | surface (48) | `ItemStyle.*Brush` (the dropdown's rows, 28); `ShadowColorAndOpacity` (6); `*.Pressed.TintColor` (9) | raised cream; ink; pressed cream |
  | `#2A1A33` Plum (panel = ink) | surface (26) | `Font.OutlineSettings.OutlineColor` (6), `SliderHandleColor` (5); `Background` / `*.Normal` tints (15) | ink; ink; card stock |
  | | text (4), edge (1) | a label on the selected plate, a card's ink | ink |
  | `#F6EEF8` Bone (text) | text (82) | `ColorAndOpacity`, `TextColour` | ink |
  | | surface (35) | the combo's `*Foreground`, `ItemStyle.TextColor` / `SelectedTextColor`, `ForegroundColor`: text drawn by a style | ink |
  | `#3E2A4A` raised, `#564361` hover, `#34223E` disabled | surface, edge | rows, hovered and pressed tints, disabled tints | raised, hover, off creams |
  | `#DCCFE4` Lilac, `#B8ABC1` | text, surface, edge | muted text, the slider bar, a combo's pressed/disabled text, disabled outlines | muted text; off text |
  | `#FFD23F` Butter (focus = selected) | edge (41) | outlines that marked hover or the active row | the focus blue |
  | | text (5), surface (1) | text in Butter (the logo, names); one bar fill | ink (titles become stickers in S2-S4); yellow |
  | `#D2263F`, `#F2C65B`, `#FF7055`, `#70954F` | one or two each | the boss bar's fill and edge, the zone bar's fill and edge, the badge and the Banish label, a good line | danger, the green bar, badge and danger, good |

  **So the migration is by RULE, not by value alone:** a short list of (role, property path suffix, old key, new key) rules
  goes first (the shadow, the pressed tint, the dropdown's item brushes, a font's outline, the slider handle, the
  foregrounds, Butter edges -> focus, Butter text -> ink, the two bar fills), then the role maps for everything else. A new
  `UPawWidgetBaker::RecolourByRule` takes the rules with the path (the report's walk, made writable). **Pre-flight against the
  inventory before anything is saved:** every saved colour that is a changing palette colour must be placed by exactly one
  rule or map, or the run refuses and lists it. After the run, the inventory again: no changed key's old value left.
  **Also in S1:** the Sticker UI table (values in §2; text pairs re-checked: muted on a row 4.6:1, off text on off 4.6:1,
  focus on a row 3.1:1); `selected_text`, `card_ink`, `card_take`, `card_banish` join the text keys; `field_label` and
  `boss_banner_text` join the keys no asset saves (painted in code); the pairs "a Catnip amount" and "a Stardust amount"
  leave rule 7 (amounts are ink beside their icon; nothing draws an amount in its currency colour, checked in code);
  the C++ that names plum tokens (`PawShot.cpp`, `PawUiKit.cpp`'s title shadow) moves to role tokens; the tests that pin
  the plum spec move to Sticker's (`PawChromeTests`: the ring is blue and the selected plate yellow, no longer one
  colour; the title and victory headlines' colour checks follow S2's sticker titles).
- **S2, fonts (1-2 sessions).** Fredoka and Nunito (OFL) into `SourceArt/fonts`; `tools/fonts.py` per E1;
  `import_fonts.py`; `check_font_coverage.py` green and seen to fail; `bake_widgets.py --refont`; the 21 language sheets.
- **S3, chrome (2 sessions).** `tools/art/sweet_ui.py`: card-stock nine-patches with the ink edge and hard shadow, the button
  family (primary, secondary, off, danger), the blue focus ring, tabs and badge, toggles, the x1/x5/x10/Max switch,
  keycaps and pad glyph plates, bars, the rainbow frame; `import_art.py chrome`; the kit board as the check.
- **S4, screens (4-6 sessions).** Every screen against the UI concept, as grafts: the title (fenced garden, sticker logo,
  Boba's polaroid, the in-game key hint), the slots (the cat at the save's stage), pause, settings (Controls table with
  keycaps and pad glyphs side by side), victory (confetti, the crown on the floor), the dock (buy amount on its own row, the
  key legend in two rows), all six tabs, the boss panel top centre, the card pick (flavour band, NEW starburst), the job
  picker (the cat in each job's hat), the skill tree (hex nodes on the night sky), the Reawaken and Unicorn ceremonies (rays).
  At 1920x1080, 1280x800 and 150 % UI.
- **S5, the field (4-6 sessions).** The bestiary's drawings in the generator grammar (`sweet_shapes`: ellipses, tubes with ink
  edges, the faces), batch by batch with `review_anatomy.py` sheets beside the bestiary: the 12 commons, 12 signatures, the
  gristle by tier, the flying mouse, bat and golden mouse, the 5 bosses and the add, the cat's 7 stages, kittens, the job hats
  and weapons. The rim (E2) in `raster_svg.py` / `pack_atlas.py`. The 12 floors and their props (thinner line, no rim). The
  effects (`fx_look.py`: the aura's yellow ring, the white swipe arc with three slashes, the shield's dashed ring, the
  signature glow). The paw cursors (open; held with arc toes). `look_lint.py` rewritten to §2 (one ink at one width, the rim
  present, two fills, eyes that read at 56 px, creatures clear of their floors) with its self-test. The packaged frame gate.
- **S6, icons, badges and cards (2 sessions).** The 178 icons and the card faces in the kit's icon style.
- **S7, sign-off (1 session).** The tour of every screen beside the concept; the designer's review.
- **Then:** phase 6's 6b-6d in the new look (the Reawaken banner, Boba, the tutorial), and after that the Steam store page's
  assets and descriptions (the designer's order).

## 5. Checking against the concept

The concept pages render headless (Edge `--headless --screenshot`), so a screen of the game and the same screen of the concept
can be laid side by side at the same size: `godot_tour.py`'s Unreal half for the game, the artifact's HTML for the concept.

## 6. Risks

- **Fonts**: Fredoka's missing scripts (E1); a title face swap per language must not change line heights enough to clip.
- **Recolour ambiguity**: plum to card stock flips text from light to dark; the migration is role-aware (`RecolourByRole`)
  and refuses an ambiguous map. Any colour it cannot place is listed, never guessed.
- **Atlas space and the rim**: more padding per sprite; `pack_atlas.py --check` and the page count are the measure.
- **The look lint** encodes the plum spec: until S5 rewrites it, a drawing that follows §2 can fail it. Its KNOWN list is not
  grown to hide that; the rule changes with its self-test.
