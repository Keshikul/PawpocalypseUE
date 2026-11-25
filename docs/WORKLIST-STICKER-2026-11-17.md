# The Sticker Box restyle (plan, 2026-11-17): D18

**Status (2026-11-24, HANDOFF-LOG (145)):** S0, S1, S2's text half (Nunito), S3 (the chrome), S4 (every screen as the UI
concept draws it, HANDOFF-LOG (111)-(124)) and S5 (every character, the floors) done. **Left**: S2's title face (Fredoka,
F_PawTitle, E1), S6 (the card art and the skill / achievement icons: drafts waiting since (127)), the props, the rig
(E3), S7 (the sign-off tour, `tools/screen_tour.py`). (Was, 2026-11-18, (110): S1, S2's text half and S5 done.)
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
- **Faces**: enemies panic (huge mismatched eyes, an o mouth; **no tear or sweat drop**: designer, 2026-11-17, "All the
  enemies have tear drops. Remove the tear drops"; removed from the Bestiary and UI Screens, version 5, the robot mouse's
  oil drip with them); the cat schemes (crossed pinprick pupils, brows
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
| E1 | **Fonts in every language.** Fredoka has Latin and Hebrew only; Nunito has Latin, Cyrillic and Vietnamese, not Greek, Japanese, Korean, Chinese or Thai. | Text in Nunito; titles and numbers in Fredoka where it has the letters, and in Nunito's heaviest weight where it does not (Cyrillic, Vietnamese); M PLUS Rounded 1c stays as the face for Japanese and any script Nunito lacks; Noto CJK and Thai stay. The coverage gate decides; S2 shows the 21 language sheets before anything ships. **Measured 2026-11-17 night** (`build/night3/fonts/README.md`, 21 sheets). Nunito 700 covers every player language's text; the editor-only `xa` keeps M PLUS for 13 rare letters. Fredoka 700 has 320 characters: it carries titles in en, de, es, es-MX, fr, it, pt, pt-BR, sv; Nunito 900 carries cs, hu, pl, ru, tr, uk, vi, for the whole title. **Fredoka's digits are proportional (no tabular figures), so a number that changes in place stays Nunito** (Nunito's digits are all one width). F_Paw and a new F_PawTitle both pass the gate's check; each sub-font was seen to fail when removed. Titles need a second rule the coverage gate cannot see, one face per language; it was written and seen to fail. |
| E2 | **How the rim is made.** | Bake it into each sprite when the SVG is rastered (dilate the alpha, fill white, under the drawing): exact, no runtime cost, no new draw calls. Costs about 5 px of atlas padding per sprite. **Tried 2026-11-17 night** on batch 1 (`build/night3/art/`). One choice is left: the kernel. A disc reads as die-cut; the concept's feMorphology is a square, thicker at the corners (silhouette IoU 0.91-0.95 between the two). The rim clears 10 of the 12 new grounds; **Kitchen `#ECDFC9` (13.4) and Vet `#BDE0D5` (18.4) are too pale for it** (CIE76, min 22). |
| E3 | **The rig** (moving wings and legs, R5). | Keep it: the new drawings come with the same layers (wings for the moth, bat, flying mouse, seagull, crow, parrot, imp; legs where the drawing has them), turned by the existing rig code. |
| E4 | Layout changes the UI concept makes (S4 list). | As drawn in the concept; each is grafted into the Widget Blueprint (the asset owns the layout). |
| E5 | The concept's open items: locked Realms rows naming what opens them (one new string), the job picker's lines, the tutorial line, Boba's photo. | Drafted as text drafts first (CLAUDE.md), with their 20 translations owed. |

## 4. Phases

Each ends as every slice does: everyday `check_all` exit 0, new and retargeted sabotages seen CAUGHT, docs, HANDOFF-LOG.
After any palette change: `py tools/gen_paw_tokens.py`, `py tools/art/build_art.py`, `raster_svg.py`, `pack_atlas.py`,
`import_art.py --run`, `py tools/bake_widgets.py --run --recolour`, **`py tools/gen_paw_tokens.py` again** (the recolour
rewrites the widgets' snapshot, and the retired list in `PawTokens.h` is made from it; found 2026-11-18, L0 failed without
it), then `check_all` (the restyle worklist's recipe).

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

  **Prepared off the tree, night of 2026-11-17** (`build/night3/s1/README.md`; nothing applied):
  - `s1.patch`: 11 files, applies clean on 1a10eeb.
  - The palette's rules: 0 problems.
  - **The pre-flight against the inventory places every saved colour: 412 move, 7,137 stay, 0 unplaced.** It was seen to
    fail first: the zone and boss bars' tracks, in Deep Plum, had no rule.
  - 42 rules. The kept card ink is now a rule (`asset:object` rules exist for one widget).
  - Changes from the list above:
    - the four outlined Butter titles (main menu name, Level up, job picker, victory) become **yellow sticker text** with
      their ink outline, not ink;
    - only Boba's unoutlined name on its card becomes ink;
    - the tree tooltip's five colours (`gen_tree_tables.py`) become UI roles (the plum mapping refuses the Sticker table).
  - The art rebuilt from a scratch copy: 534 of 554 files change; look_lint PASS.
  - The morning's order is in that README.
- **S2, fonts (1-2 sessions).** Fredoka and Nunito (OFL) into `SourceArt/fonts`; `tools/fonts.py` per E1;
  `import_fonts.py`; `check_font_coverage.py` green and seen to fail; `bake_widgets.py --refont`; the 21 language sheets.
  **The text half, done 2026-11-18 (night, on E1's measured recommendation; one step to undo):**
  - F_Paw's Regular and Bold are Nunito 700, its Black is Nunito 900, instanced with their own names
    (`updateFontNames`).
  - Japanese Han and kana stay on M PLUS ExtraBold: a sub-font holds one face, so Japanese titles lose one weight step
    until the spec allows a face per weight.
  - The pseudolocale xa keeps M PLUS for Latin Extended.
  - Gate PASS (22 languages, 6,301 characters); self-test seen to fail.
  - The texts follow without a refont: their font is the class default, F_Paw (delta serialisation), and the typeface
    names are unchanged.
  **Still to do:** F_PawTitle (Fredoka 700, the culture and gap sub-fonts, the one-face rule in the gate) and the titles
  that use it.
- **S3, chrome (2 sessions).** `tools/art/sweet_ui.py`: card-stock nine-patches with the ink edge and hard shadow, the button
  family (primary, secondary, off, danger), the blue focus ring, tabs and badge, toggles, the x1/x5/x10/Max switch,
  keycaps and pad glyph plates, bars, the rainbow frame; `import_art.py chrome`; the kit board as the check.
- **S4, screens (4-6 sessions).** Every screen against the UI concept, as grafts: the title (fenced garden, sticker logo,
  Boba's polaroid, the in-game key hint), the slots (the cat at the save's stage), pause, settings (Controls table with
  keycaps and pad glyphs side by side), victory (confetti, the crown on the floor), the dock (buy amount on its own row, the
  key legend in two rows), all six tabs, the boss panel top centre, the card pick (flavour band, NEW starburst), the job
  picker (the cat in each job's hat), the skill tree (hex nodes on the night sky), the Reawaken and Unicorn ceremonies (rays).
  At 1920x1080, 1280x800 and 150 % UI.
  **The dock's upgrade rows done 2026-11-18 (HANDOFF-LOG (111)):**
  - Each row is the upgrade's icon, its name, "Lv N" under it in the muted colour, and the price on its own plate at the
    right: the raised yellow when it can be paid, the off plate when not.
  - Built from `UPawButton` with one new part grafted into `WBP_Button` (`PriceTag`: `SetPriceTag`, and
    `SetIconSlotKept` so the names line up), plus the chrome's raised yellow `ui_button_primary`.
  - The concept's 8 upgrade icons are `ui_icon_upgrade_<id>`, recorded from its shop.
  - The words: `SHOP_row_level` and `SHOP_row_max_plate` (the approved words, split) and `SHOP_row_level_bulk` (a draft),
    all in `docs/TEXT-DRAFT-2026-11-18.csv`.
  - Sabotages `hud_row_says_max` (retargeted) and `hud_row_plate_always_yellow` (new), seen caught.
  - Not yet:
    - the concept's "Buy amount" row with its x1 / x5 / x10 / Max switch (today a "Buy x1" button);
    - the 7 icons it did not draw (drafts on Missing Art, https://claude.ai/artifact/J265sfqjxSbdsSNsXU3VSh, with the Bait
      icon, the 7 hats and the job icons);
    - a can't-afford row keeps the row look and dims only its price in the concept; the game's locked style dims the
      whole row.
  **Also done 2026-11-18 (HANDOFF-LOG (112)):**
  - the Reawaken ceremony: the concept's rays, halo and sparks, the armoured cat, the sticker title, the payout on a
    panel;
  - the pause menu: the sticker title, each button's icon, Continue on yellow;
  - the victory's OK on yellow.
  **The card pick done 2026-11-18 (HANDOFF-LOG (114)):**
  - the band holds the flavour alone, spaced out; the picture on a panel tinted with the flavour (55 % to white, the
    concept's mix); the name under it;
  - "LEVEL n of max" with a dot per level, filled to the level taking it gives, on every card (a new one too);
  - NEW on the concept's red starburst over the band's right end (`ui_icon_card_new`, `sticker_art.write_card_pick`; the
    word is the game's string over it, ink-outlined);
  - the lifted card (pointer or pad) has its Take on the yellow plate;
  - LEVEL n as a sticker title with "Take 1 card." in ink, on a card-stock panel; Reroll carries the Reawaken arrow;
  - grafts: `WBP_CardFace` `wrap:Plate>Stack; add:NewBadge; remove/add NameText, RankText, Art (into ArtPanel, LevelRow);
    restyle:Band; text:FlavourText`; `WBP_CardPickScreen` `remove:Title; remove:Subtitle; add:TitlePanel`. The baker's
    `wrap:` now allows a wrapper with more children (each after with `add:`).
  - Kept as today: the band colours (the concept coloured only 3 of the 7 flavours, two of them near-identical yellows,
    which loses "three kinds at a glance"); Banish under Take inside the card (the concept puts it beside Take, under the
    card).
  **The title done 2026-11-18 (HANDOFF-LOG (114)):**
  - the concept's cast on the garden: the Tiny Kitten and three mice (two mirrored, the concept's flips), UI pictures from
    the recorded cast (`ui_icon_title_cat` 512, `ui_icon_title_mouse` 256; `sticker_art.write_title`), never a click;
  - the logo centred at the top and upright (R4c had it tilted at the top left); the status line in heavy ink;
  - Continue was already on yellow while there is something to continue (R4c);
  - grafts: `add:TitleCat; add:TitleMouse0..2; remove:LogoSize; add:LogoSize; slot:MenuSize; text:StatusText`.
  - Kept, with the reason: the logo at R4c's height (the concept's lower logo meets the menu at 150 % UI:
    `Pawpocalypse.UI.Layout.Title` caught it); the two right-hand mice closer in (one left the screen at 1280 x 800,
    150 %); the links as icon plates at the bottom right and the key hint at the bottom left, and the status line inside
    the menu panel (the concept's bottom row, labelled links beside the hint, and a status pill do not fit at 1280 x 800
    and 150 % UI, 1153 units wide); no shadows or idle motion under the cast yet.
  **The save slots done 2026-11-18 (HANDOFF-LOG (115)):**
  - each card shows the save's cat at its stage in a photo well (the concept's slotCard): grass for a save (the new UI
    token `photo`, #BFE3A3, the concept's), card stock for an empty slot, the off plate for one that cannot be read;
  - the stage is the game's rule read from the file: `PcSlotSummary` now carries the bosses beaten and the ending, and
    `Pc::EvolutionStageFor` is the one rule (`EvolutionStage` calls it);
  - seven pictures, `ui_icon_slot_cat_1..7` (256 px; `sticker_art.write_slots`: the stage's cat with its weapon);
  - "Slot N" inside the card at its top (it was ink under the card, on the dark scrim); the card 380 tall;
  - CHOOSE A SLOT as a sticker title.
  - Kept: the summary's words and centring; the Delete button's look (the concept's danger kind is a later pass).
  **The hats and the job picker done 2026-11-18 (HANDOFF-LOG (116)).** The designer, on Missing Art v2: "The hats are
  good. The ones that are specific. Not the top hats": the seven drafted hats approved, and option B for the job icons.
  - The ten hats are recorded like the cast: `tools/art/concept_hats.js` (the approved page's seven, word for word, and
    the job -> hat table) runs beside the UI concept in `sticker_record.js`; `sticker_cast.JOB_HATS`.
  - In the field: `hat_<job>` drawn in one square every hat shares; `PawFieldLook.h`'s hat constants worked out from it
    (standing (-0.189, -0.202) 0.775; mounted through riderFig's transform (-0.010, -0.220) 0.366).
  - The job icons are each job's own hat (`job_<job>`, option B).
  - The job picker is the concept's: a card per job (up to five a row, the panel shrinks to fit), the Tiny Kitten in the
    job's hat on a photo (`ui_icon_job_cat_<job>`, `_none` the bare kitten), the name on its button, the blurb; the lifted
    card's button on yellow. Kept: every held job at once and the job's name as the button (the concept pages three jobs
    with LB / RB and a new "Wear" word).
  **Two upgrade icons and settings done 2026-11-18 (HANDOFF-LOG (117)).**
  - The designer: "Change shread and shield pirce. The lines are too straight and dint convey tearing things apart",
    then, on Missing Art v3, "Shred b, shield a": Shred is a sheet ripped in two (over the concept's three straight
    lines), Shield Pierce the shield split along a jagged crack. `tools/art/concept_icons.js`, recorded like the hats.
  - Settings: SETTINGS a sticker title; Close on yellow; the controls table's head on a raised band in capitals; every
    other row on a faint band. Kept: the checkboxes (the concept's switches are a chrome piece, S3), the per-row Reset
    and the binding plates, the centred title (the concept's sits left beside an X).
  Next: the other dock tabs, the boss panel, the skill tree, the Unicorn ceremony.
- **S5, the field (4-6 sessions).** The bestiary's drawings in the generator grammar (`sweet_shapes`: ellipses, tubes with ink
  edges, the faces), batch by batch with `review_anatomy.py` sheets beside the bestiary: the 12 commons, 12 signatures, the
  gristle by tier, the flying mouse, bat and golden mouse, the 5 bosses and the add, the cat's 7 stages, kittens, the job hats
  and weapons. The rim (E2) in `raster_svg.py` / `pack_atlas.py`. The 12 floors and their props (thinner line, no rim). The
  effects (`fx_look.py`: the aura's yellow ring, the white swipe arc with three slashes, the shield's dashed ring, the
  signature glow). The paw cursors (open; held with arc toes). `look_lint.py` rewritten to §2 (one ink at one width, the rim
  present, two fills, eyes that read at 56 px, creatures clear of their floors) with its self-test. The packaged frame gate.
  **Batch 1 drawn, night of 2026-11-17** (`build/night3/art/README.md`).
  - The Bestiary's own JS, run in Node, gave the references.
  - `sticker_draw.py` is the concept's primitives in the grammar; `sticker_commons.py` is the 12 commons, transcribed.
  - **Faithful:** silhouette IoU 0.993-0.998 without rims, colour within 8/255. No tear drops (removed at the designer's word the same night).
  - For the lint: each shape carries its own ink width; the eye rule's pattern cannot see them.
  - **Eyes measured at 56 px:** the four rats 36 px, pigeon 54, vole 63, all under the floor of 64. Either bigger eyes or a
    lower floor: the designer's call.
  - The ground rule should measure the rim, not the body.
  **The pipeline built, not landed (night of 2026-11-17 into 18;** `build/night3/s5/`, `s5_apply.py` on a scratch copy):
  - `sticker_draw.py`, `sticker_commons.py` and `palette.STICKER_CREATURES`.
  - `make_sweet_art` writes the Sticker drawings over the grammar's and drops their rig layers.
  - `pack_atlas` makes the rim at raster time.
  - `look_lint` reads per-shape ink widths and Sticker's eye white; its self-test gets Sticker plants.
  - **Blocked by a rule of the designer's (2026-11-09: "no shade used twice"):** the approved Bestiary's own cast breaks
    it in **57 pairs** (`build/night3/s5/unique_in_bestiary.txt`; CIE76 under `UNIQUE_MIN` 16). Three are identical
    colours:
    - the add and the moth, `#B7A3E8`;
    - the Void Unicorn and the Void God, `#3E2F72`;
    - the Foreman and the first gristle, `#D08478`.
    Others: the grey cat against the rat (6.5) and the pigeon (8.0), the unicorn white against the hamster (8.2) and the
    ghost (10.3).
    A partial swap adds more pairs: a new body against an old one.
  - **The designer's call:**
    - (a) move the Bestiary's clashing colours apart (a recolour of about 20 characters, drawn as a review sheet first);
    - or (b) change the rule for Sticker Box, where the die-cut rim, the outline and the silhouette tell characters
      apart. For example, keep it within one realm's spawn pool (what is on screen together) and drop it across realms.
  - The other two open lint questions decide which drawings pass:
    - the eye floor: the four rats 36 px, pigeon 54, vole 63, under 64;
    - the floors: the hamster on the Vet floor.
  **LANDED 2026-11-18 (day; HANDOFF-LOG (110)).** The designer, on the review page: "The enemies all seem good", then
  "start rewiring all new graphics to the game". Read as option (b) for the shade rule, and as approval of the cast as
  drawn: no colour in the Bestiary moved.
  - **Every character is the Bestiary's own drawing, recorded, not transcribed.** `node tools/art/sticker_record.js` runs
    the approved concept's code (`tools/art/concept_bestiary.html`, version 5; the kitten's colours from
    `concept_ui_screens.html`) with its primitives replaced by recorders, and writes `tools/art/sticker_cast.py` (one
    function per character, the concept's numbers and colour names) and `tools/art/sticker_colours.json` (read by
    `palette.STICKER_CREATURES`). 60 drawings: the 12 commons, 12 signatures, the flying mouse, the bat, the golden mouse,
    the 12 gristles (each tier its own colour), the 5 bosses and the add, the cat's 7 stages, its weapons (cut out of the
    concept's figures), the kitten, the cat's base drawing. Fidelity against the concept's own render: IoU 0.996-1.000,
    colour under 1/255. Gate `Sticker cast is the concept` (`--check`), seen to fail on a planted edit (safe_mutate).
  - `tools/art/sticker_art.py` writes them over the grammar's drawings (`make_sweet_art`), removes their rig layers (E3
    open: the drawings are still sprites), and fits each group of the cat's stages in one frame. **The hat and weapon
    constants (`PawFieldLook.h`) are worked out from the new cat** and the art build stops when they differ: the weapon
    where the concept holds it (behind the standing cat; the lance over the rider's shoulder), drawn facing the cat's way
    (`WeaponFlipX` now mirrors with the cat; test and sabotage follow). The ten job hats are still the grammar's, placed on
    the new head as on the old: **the concept drew three hats; the other seven wait for the designer.**
  - The cyber cat's visor is see-through, as approved (`sticker_draw.GLASS`, `fill-opacity`; look_lint allows it on a
    Sticker drawing only).
  - **The floors are the Bestiary's grounds** (`palette.STICKER_GROUNDS`: ground, patch, shadow; detail a fifth of the way
    to the shadow). `PcBackdropTones.h`, `PcGodotLook.h` and the 12 floor textures follow. Six prop colours moved to the
    nearest shade that clears its new floor (`sweet_biomes.PROP_COLOURS`); the props are still the grammar's drawings.
  - The rim is made when packed (`pack_atlas.die_cut`, a disc: E2 still open). The atlas is 3 pages (107 sprites) without
    the rig layers; `T_Atlas_Field_3` was removed.
  - **look_lint's Sticker rules:** per-shape ink widths up to the drawing's line; the concept's colours and their mix();
    eyes measured at the size the game draws each drawing (floor 27 px, the bat's as approved); "no shade used twice"
    within a realm between enemies (`UNIQUE_APPROVED`: six pairs as approved, among them the pigeon and seagull, and the
    add against the bat and the flying mouse); bodies close to their floors as approved (`APPROVED`, the KNOWN format and
    ratchet). Palette rules 3 and 4 now leave the creatures to the drawn check. The self-test's plants moved to drawings
    that still carry each rule; every plant caught.
  - **Also landed the same day:** the UI concept's kit icons over 18 of the game's (the six tabs, the quick bar, the menu's
    links, Catnip, Stardust, the tick, close and lock; recorded from `concept_ui_screens.html` at its 2.6 icon line,
    `sticker_art.write_icons`, written by `build_art` after the hand-made icons); the paw cursors (open and held, the
    kit's, line 2.8, the rim made by `raster_svg`); the aura as the concept draws it (`fx_look.py`: its yellow ring at
    85 % over a 16 % fill, no ink edge; crits in Sticker's yellow); the strike as the concept's swipe (a white arc on its
    ink edge, the three claw slashes in ink, Godot's length, no paw; D3's paw kept behind `STRIKE_PAW`; three new
    sabotages caught). Still the old: the card art and the skill and achievement icons (S6), the props' drawings, the
    chrome's diagonal shadow (S3: brush margins saved in the assets), the signature glow and the boss's shield ring, the
    screens' layouts but the dock's rows (S4).
  - **For the designer** (none blocks): the add is 5 from the Mythic floor (lilac on lilac) and the Litter King 7 from the
    Litterbox; the standing cat 11 from the Alley; the seven hats; E2's disc; E3's rig.
- **S6, icons, badges and cards (2 sessions).** The 178 icons and the card faces in the kit's icon style.
- **S7, sign-off (1 session).** The tour of every screen beside the concept; the designer's review.
- **Then:** phase 6's 6b-6d in the new look (the Reawaken banner, Boba, the tutorial), and after that the Steam store page's
  assets and descriptions (the designer's order).

## 5. Checking against the concept

The concept pages render headless (Edge `--headless --screenshot`), so a screen of the game and the same screen of the concept
can be laid side by side at the same size: `tools/screen_tour.py` for the game (was `godot_tour.py`'s Unreal half), the artifact's HTML for the concept.

## 6. Risks

- **Fonts**: Fredoka's missing scripts (E1); a title face swap per language must not change line heights enough to clip.
- **Recolour ambiguity**: plum to card stock flips text from light to dark; the migration is role-aware (`RecolourByRole`)
  and refuses an ambiguous map. Any colour it cannot place is listed, never guessed.
- **Atlas space and the rim**: more padding per sprite; `pack_atlas.py --check` and the page count are the measure.
- **The look lint** encodes the plum spec: until S5 rewrites it, a drawing that follows §2 can fail it. Its KNOWN list is not
  grown to hide that; the rule changes with its self-test.
