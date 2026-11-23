# Handoff log

Newest first. Every session that changes the state of this project adds an entry: where the project is, what happened, what is open,
and the exact next step. Read `docs/00-START-HERE.md` first, then the newest entry here.

Entries (1) to (72) (2026-10-28 to 2026-11-09, everything committed through `b0ce7be`) are in
[HANDOFF-LOG-ARCHIVE.md](HANDOFF-LOG-ARCHIVE.md). A reference such as "HANDOFF-LOG (41)" in any doc means the numbered entry,
in whichever file holds it.

---

## 2026-11-23 (136) - Phase 7: every language in the package (`packaged_languages.py`); the ICU preset fixed

**Where the project is.** On (135) (uncommitted; (134) committed and pushed, `0b5b17a`). The designer: "Do Packaged
language check": (135)'s second choice, with the word to package. This entry's work is uncommitted, like (135)'s.

**What happened** (`docs/WORKLIST-LOC-2026-11-18.md` §2h).
- **The check, `py tools/packaged_languages.py`** (L5): `paw.Languages` (`Source/Pawpocalypse/Dev/PawLanguages.cpp`, new,
  development builds only) in the packaged Development build, on the title, `-NoSave`, twice. Run 1: started with
  `-culture=ru`, the start language applied and shown. Run 2: the title's language list opened and every language picked
  in it as a player picks it: offered (every culture with a translation file), complete, current in the engine, every line
  of the three string tables showing its translation (851), every character in the face `F_Paw` picks for it, and Thai /
  Japanese / Chinese breaking a line inside their words. A screenshot per language; `build/loc/package/sheet.png`.
- **Found: the package could show 18 of the 23 languages.** The project had no ICU setting, so the engine's default
  `InternationalizationPreset=English` staged English ICU data alone. zh-Hans, zh-Hant, pt-BR, pt-PT and es-419 were
  refused by the engine: picked, the row was ticked and every word stayed in the language before (748-823 lines each).
  Thai never wrapped inside a sentence (no break dictionary: 0 breaks in its longest line). The editor has the engine's
  full ICU data, so no editor test could see either.
- **Fixed**: `InternationalizationPreset=All` in `Config/DefaultGame.ini` (the only preset with Russian, Polish and Thai; ~23
  MB more on disk, ICU 64: 27 MB against 4.2). `UPawSettingsSubsystem::ApplyLanguage` now logs a culture the engine refuses.
  `CulturesToStage` stays `en` (polyglot text, no `.locres`: not needed, measured).
- **L0 `packaging`** (new, in `check_pawcore_static.py`, everyday): the ini keeps the preset and the staged Localization folder,
  so neither can go back between packages; its self-test plants three faults.
- Knowledge base: U/20 §5 (the presets measured in a package, the packaged test's method) and a dated note under §3's
  `[Docs]` packaging line; a U/24 row; the symptom index and both indexes.

**Proof.** The English-preset package: FAIL (the five cultures refused, Thai 0 breaks;
`build/packaged_languages_20261123_before.log`). With `All`: PASS, 23 languages (Thai 35 breaks, Chinese 8, Japanese 13; 0
characters without a glyph; `_after.log`). A package made with two planted faults (`safe_mutate`, files restored): a
character no face has in a German line (U+0F00) CAUGHT, an emptied French line CAUGHT (French not offered), the other 21
still ok. Then a clean package again: PASS, 23 languages (`_final.log`; `build/package` is this one). L0 self-test: 24 plants CAUGHT (the three `packaging` ones among them), the real sources PASS. `check_all` (everyday) exit 0, every gate PASS (`build/check_all_20261123_pkglang136.log`; L0 `packaging` PASS).

### Open
- Seen and approved: the title and its language list in every language, from the package (`build/loc/package/sheet.png`).
  The designer, 2026-11-23: "The text is good."
- The native reads (review sheets); the designer's eye on the shrunk headlines (the Reawaken ceremony's, the victory's, the
  controls table's column heads: not in that sheet); the store page's Dutch and Danish.
- Not packaged: Shipping (the ICU data is staged the same way). The package in `build/package` (git-ignored) is today's.
- From (135): toasts, the Unicorn ceremony's painted words and screens not built yet are not in `Language.Switch`.

### Next step
The native reads' review sheets (one per language: every line, the English beside it, a fix column `--apply` takes back;
needs no decision). Done is `check_all` exit 0.

## 2026-11-23 (135) - Phase 7: a language changed while a screen is up (`Language.Switch`); the Settings lists follow

**Where the project is.** On (134) (committed and pushed, `0b5b17a`). The designer: "read handoff. what's next?", then
"do option 3": (134)'s third choice, the string-built text check. This entry's work is uncommitted.

**What happened** (`docs/WORKLIST-LOC-2026-11-18.md` §2g).
- **The test, `Pawpocalypse.UI.Language.Switch`** (everyday, in `Pawpocalypse.UI`, ~2 min). The language pass's tour of
  every screen (`TourScreens` in `PawLayoutTests.cpp`, now with hooks; `Layout.Languages` and `.PseudoLocale` unchanged),
  plus the HUD in a boss fight, three times with the same steps: built in English and switched to German at each screen
  and back; built in German and switched to English and back; built in English with German chosen before the first
  screen. Every look must hold the words of the same screen built fresh in its language, as a multiset: the text of every
  laid-out Slate text block and rich text, in view or not, and every tooltip (`PawLayout::ArrangeWords`, new). The switch
  is the game's: the culture, then `UPawSettingsSubsystem::SetLanguage` (OnChanged). Differences go to
  `Saved/Layout/language_switch.txt`.
- **Found: the Settings dialog's lists.** Combo boxes hold strings, so Language ("System"), Quality, Max FPS
  ("Unlimited"), Prompts, Dock side and Output ("Follow the system") kept the old language after a change made outside the
  dialog. A player could not see it (only the dialog's own Language list changes the language while it is up, and its
  pick refilled every list), but the dialog now hears the change as Godot's lists hear `language_changed`:
  `UPawSettingsDialog::OnSettingsChanged` fills every list again when the language setting or the language shown changed
  (a slider's change leaves them alone); `PickLanguage` no longer fills them itself.
- **Everything else already followed**: the title (every layer, the language list), every dock page, the card pick, the
  tree and its tooltips, the spend step, pause, the three settings pages and the Controls panel, the ceremony, the job
  picker, the victory screen, the boss panel. String-table, formatted and joined text follows by itself; prompt lines
  remake their markup on the settings' change.
- **The test's own blind spots, found on its first runs and closed**: UMG text widgets alone missed the combo boxes (now
  the Slate tree); parts in view alone gave 1,100 false differences on the tree (its map pans as it ticks); "the longest
  tooltips in this language" picked other nodes per language (fixed nodes when hooked).
- Knowledge base: U/20 §4 (what follows a culture change by itself, `UComboBoxString`, the test method), a U/24 row, the
  symptom index and both indexes.

**Proof.** `Language.Switch` PASS (it failed first on the six lists, as above). Sabotages `switch_*` all CAUGHT: the
dialog deaf to the change, a prompt line deaf to it, a tree caption made a string, every `ST_UI` line made a string. Two
first candidates MISSED and were replaced, neither a hole: the title's own handler (its words follow without it: the hint
is a prompt line, the status line string-table text; its plate and tick are `Title.Language`'s, which catches
`title_language_words_not_remade`), and a dock page title's `SetText` (it runs only at the bake: the asset holds the
line). `Pawpocalypse.UI.Settings` 10 PASS. `check_all` (everyday) exit 0, every gate PASS
(`build/check_all_20261123_switch135.log`; `Pawpocalypse.UI` 109 tests).

### Open
- (134)'s list stands, less this item: the native reads (review sheets), the packaged check of the shipped languages
  (needs the word to package), the designer's eye on the shrunk headlines and the language list, the store page's Dutch
  and Danish.
- Not covered by the test: toasts (made per event), the Unicorn ceremony's painted words (read from the string table on
  every paint), screens not built yet (the Reawaken banner, the tutorial).

### Next step
The designer's call: the native reads' review sheets (needs no decision), or the packaged language check (needs the word
to package). Done is `check_all` exit 0.

## 2026-11-21 (134) - Handoff: phase 7 where (133) left it; what is uncommitted; the choices waiting

**Where the project is.** Committed and pushed through (128) (`56a5a4e`). (129)-(133) are uncommitted: the title's
language list, `loc_status --apply` and all 22 translations (drafts), the language pass, the wording review with the
`loc_lint` gate, every line read for meaning. Nothing changed since (133). The designer: "Create a handoff", then
"Commit and push": (129)-(134) committed and pushed the same day (the list below is that commit). The Godot repo's
findings file stays uncommitted there.

**Proof, today.** `loc_lint` PASS (0 findings, 22 cultures); its self-test PASS (13 plants caught); every culture
837 / 837, 0 stale, 0 broken; `port_po --check` OK; font coverage PASS (26 languages, 6943 characters per weight). The
last `check_all` (everyday, exit 0, `build/check_all_20261120_deepread133.log`) ran 22:24-22:30 on 2026-11-20, after the
last translation edit (22:23). Only `tools/loc_lint.py` changed after it (F56 and its plant); the gates above cover it.
In the Godot repo the port's one file is `docs/pawpocalypse-port-findings.md` (untracked there; F52-F56 added); every
other change there is the designer's.

**What the commit of (129)-(134) holds**:
- New: `tools/loc_lint.py`; Dutch and Danish, `Content/Pawpocalypse/Localization/Translations/da/` and `nl/`.
- The other 20 cultures' translation files (60 CSVs).
- `Source/Pawpocalypse/UI/` (20 files), `Source/Pawpocalypse/Dev/PawShot.cpp`,
  `Source/PawpocalypseEditor/PawWidgetBaker.cpp`.
- Six grafted assets in `Content/Pawpocalypse/UI/Widgets/`: `WBP_CardFace`, `WBP_CardPickScreen`, `WBP_ControlsPanel`,
  `WBP_MainMenu`, `WBP_ReawakenCeremony`, `WBP_VictoryScreen`.
- Tools: `bake_widgets.py`, `check_all.ps1`, `check_font_coverage.py`, `godot_tour.py`, `loc_status.py`, `port_po.py`,
  `sabotage_realm.py`, `store_shots.py`.
- Docs: `CLAUDE.md`, docs 00, 01, 08, 09, 11, `HANDOFF-LOG.md`, `WORKLIST-LOC-2026-11-18.md`.

### Open, phase 7 (gathered from (128)-(133))
- **A native read of every language** before it ships (each still says "(draft)"). `loc_status --todo` now writes
  nothing (every line is translated), so a reviewer has no file yet. To build: one review sheet per language (every
  line, the English beside it, a column for the fix) in the work file's form, so `--apply` takes the fixes back as
  (130)'s fix-only files did.
- **A packaged check of the shipped languages**: each one offered and complete in the package (the worklist's order,
  step 3). Needs the designer's word to package.
- **Text a screen builds once as a string** ((128); not checked since): a language changed in Settings while the HUD or
  the tree is up. String-table and formatted text follow the switch (`Language.Translations`); text built as a string
  keeps the old language until its screen rebuilds. The title remakes its words ((129)); the HUD hears a settings change
  for the dock side only. Check with `-PawDraftLanguages`.
- **The designer's eye**: the shrunk headlines and the slot picker at 150 % UI
  (`py tools/godot_tour.py --skip-godot --res 1280x800 --uiscale 1.5 --culture de`); the title's language list, the
  kit's card and rows, which the concept does not draw (`Saved/PawShots/title_language_en.png`, `_de.png`).
- The store page in Dutch and Danish: the designer, "not yet" (terms in (130)).

### Open, from before phase 7
`docs/00-START-HERE.md`, "Waiting on the designer", and the drafts it names: the card pictures and the skill and badge
pictures ((127)), `docs/TEXT-DRAFT-2026-11-18.csv`. No complete `-Full` since 2026-11-11/12 (D17's nightly is owed).

### Next step
The designer's call, one of:
1. **The packaged language check**: package (the designer's word), then prove in the package that every shipped
   language is offered and complete.
2. **The native reads' review sheets** (above), so readers can start. Needs no decision.
3. **The string-built text check** (above). Needs no decision.

Done is `check_all` exit 0.

## 2026-11-20 (133) - Phase 7: every translated line read for meaning; 96 fixed; F56; the lint's skipped lines

**Where the project is.** On (132), uncommitted ((129)-(133)). The designer: "fix what the pass finds", then chose "Read
every line deeper" (every line of all 22 languages, for meaning, grammar and plainness).

**What happened** (`docs/WORKLIST-LOC-2026-11-18.md` §2f). Every line longer than a short label (473 English lines, each
beside its 22 translations) read; the short labels were read in (132). Not a native read. **96 lines fixed in 21
languages**:
- Wrong meanings: the Rival Champion called "the Rival Cat" (es, es-419), Fence "smuggler" (tr), Overflow "bank transfer"
  (pl), Thai Reawaken "beaten bosses still lose", Japanese スネア (a snare drum), Chinese 第二份工作 (a real second job).
- Grammar: Turkish and Hungarian case suffixes on a number placeholder; Slavic "(s)" plurals wrong for 1 or 5+; the
  rooftops biome without its article or case in 8 languages.
- Address and words: Ukrainian formal verbs in an informal language; "Full ház" / "Full ev"; pt-PT "Cocô"; da / sv
  "every 6th s".
- **F56** (doc 01 §5, Godot's findings doc): zh-Hant Critical Mass 臨界品質 ("critical quality"), now 臨界質量.
- **F55 extended**: the cs / pl Exterminator was a rat exterminator ("Deratizátor" / "Deratyzator").

**A hole in `loc_lint` closed.** Its mouse rule exempted any line whose English held "rat" anywhere, so "spawn rate",
"frame rate", "crater" and `{mice}` placeholders skipped 16 English lines in every language. Now it matches whole words on
the English with its placeholders removed; it also knows the cs / pl exterminator words, and F56 stays fixed.

**Proof.** The fixes went through a script that writes the files' own CSV form and requires every listed row found
(96 rows; every rule matched). `loc_lint` PASS (0 findings, 22 cultures). Its self-test CAUGHT all 13 planted faults, the
new ones on a "rate" line, a `{mice}` line, "Deratizátor" and 臨界品質. With the old English rule put back in memory, the
"rate" and `{mice}` plants are MISSED: the plants test the change. Every culture 837 / 837, 0 stale, 0 broken;
`port_po --check` OK; font coverage PASS (くくり罠, 機率, 職業, 명 in fonts). `check_all` (everyday) exit 0
(`build/check_all_20261120_deepread133.log`), including `Pawpocalypse.UI.Layout.Languages`: the new lines fit their
boxes. After it, `loc_lint` gained the F56 entry and its plant; that gate and its self-test were re-run: PASS. Knowledge
base: U/20 (Turkish percent-first suffixes, Slavic "(s)" hedges, English-keyed lint exemptions) and INDEX.

### Open
- **A native read of every language** before it ships: two machine-and-eye passes cannot judge idiom and tone.
- (131)'s open items stand: the designer's eye on the shrunk headlines; a packaging gate for the shipped languages; the
  store page's Dutch and Danish (the designer: not yet).

### Next step
The designer's call: a packaged-build language check (needs the word to package), or the native reads.

## 2026-11-20 (132) - Phase 7: the wording review of the 22 translations; `loc_lint` gate; F55

**Where the project is.** On (131), uncommitted ((129)-(132)). The designer: "Don't do steam page stuff yet. Check
languages", then chose "Review the wording" (not the packaged-build check, not a no-package run).

**What happened** (`docs/WORKLIST-LOC-2026-11-18.md` §2e). Every translation swept by machine (numbers, line breaks,
English left over, the currency words in every sentence, names translated one way, repeats, how the player is addressed,
mouse words, punctuation, percent signs, scripts, length) and every short UI label read side by side in all 22 languages.
Not a native read. **122 lines fixed**: numbers written as words ("every 100th kill" in cs, pl, ru, sv, uk; Hungarian x3;
a dropped 1 in cs, pl); the credits line left in English in ja, ko; Turkish "wake early" for "Reawaken early"; Banish a
noun beside the verb Take in es, es-419, pl, ru, uk, tr; Korean 와 after an unknown word (와(과)); one percent style per
language (Turkish %40 throughout; cs, fr, sv 40%); Danish and Dutch Bait capitalised as their Catnip and Stardust; **F55**:
both Chinese scripts called Swarm, Exterminator and Infestation by rat words (敌群, 歼灭专家, 泛滥 now; doc 01 §5, Godot's
findings doc). New gate `tools/loc_lint.py` (+ self-test) in check_all: numbers, line breaks, untranslated lines, mice,
F53/F54/F55 stay fixed, one percent style.

**Proof.** `loc_lint` PASS (22 cultures); its self-test CAUGHT all 9 planted faults and passes a narrow no-break space as a
thousands mark. `port_po --check` OK; every culture 837 / 837, 0 stale, 0 broken; font coverage PASS (6944 characters).
The changed lines still fit their boxes in every language (`Layout.Languages` in the `Pawpocalypse.UI` gate). `check_all`
(everyday) passed, exit 0, with the two new gates (`build/check_all_20261120_wording132.log`).

### Open
- **A native read of every language** before it ships: this review catches what rules and a side-by-side read can, not
  idiom or tone.
- (131)'s open items stand: the designer's eye on the shrunk headlines; a packaging gate for the shipped languages; the
  store page's Dutch and Danish (the designer: not yet).

### Next step
The designer's call: a packaged-build language check (needs the word to package), or the native reads.

## 2026-11-20 (131) - Phase 7: the language pass (no text past its box in any language at 1280 x 800, 150 % UI)

**Where the project is.** On (130), uncommitted ((129)-(131) all are). The designer: "do the level pass", read as the
language (pseudo-locale) pass, (130)'s next step.

**What happened** (`docs/WORKLIST-LOC-2026-11-18.md` §2d).
- **The pass is three tests** in the everyday `Pawpocalypse.UI` gate (25 s together). `Layout.Languages`: every screen
  with words (title and windows, slots, delete prompt, the six dock pages with a filled save, the card pick with every
  card, the skill tree with each language's four longest tips, pause and its slots, the settings sections, a Reawaken's
  ceremony, spend step and job picker, the victory) in English and the 22 translations at 1280 x 800 and 150 % UI, every
  text held to its box (`PawLayout::TextMisses`: wider than its box, a line cut by a box that clips, off the screen;
  `Saved/Layout/languages.txt`). `Layout.PseudoLocale`: the same in the pseudo-locale (`PawText::Pseudo`,
  `SetPseudoLocale`, `-PseudoLoc`: letters accented, placeholders kept, ~40 % longer in short words). `Layout.TextMisses`:
  the detector sees a text too wide and a line cut, and passes one that fits.
- **First run: 594 texts in 23 languages, 30 in the pseudo-locale**, most of them real in English too. Fixed:
  - At 150 % UI the screen is 720 tall: the Reawaken ceremony's title off the top and payout off the bottom, the slot
    picker's fifth slot off the right, the card pick's Reroll off the bottom (German). Each now shrinks to the screen and
    never grows (the ceremony on an 880 x 900 stage; the slots by their row, 1496 wide, not Godot's 1680 box; the pick's
    column 16 px off the edges).
  - Headlines shrink to their box when a language's word is wider: the ceremony's, the victory's, the controls table's
    column heads.
  - Wraps wider than their boxes: a button's words beside its icon wrap by what the icon leaves (Catches, Realms, the
    Reawaken button; ten languages); the job rows wrapped at 360 beside 339 (English too): every state button is as wide
    as the widest of Worn / Owned / Locked in the language (measured with its font) and the words wrap by it; award rows by
    their Claim; the job picker's blurb 4 px short (a shrunk font measures ~1.5 % wider); the credits / leaderboard text
    short of the scroll bar.
  - The card face's body padding was still Godot's 18 in the asset (the Sticker change to 13 missed it); the skill tree's
    hub pill grows with its words ("Árbol de habilidades").
  - Translations shortened: Boba's role (and tooltip) in hu, it, ru, sv; the Hungarian save status.
  - Assets grafted (`bake_widgets --graft`; new `wrapat:` and `wrap:Code=Asset>Wrapper`): WBP_ReawakenCeremony,
    WBP_VictoryScreen, WBP_CardPickScreen, WBP_ControlsPanel, WBP_MainMenu, WBP_CardFace.
  - The tour itself: it left the card pick open (which held back the spend step and the pause menu, hiding both screens
    from the check); it now takes a card the way a player does, and a screen that does not open is a finding.
- No rule or number changed (layout and draft translations only): nothing for doc 01 §5.

**Proof.** The three tests PASS (0 findings in 23 languages and the pseudo-locale). Sabotages (`tools/sabotage_realm.py`,
7 new): all CAUGHT (`text_miss_width_blind`, `text_miss_cut_blind` by `Layout.TextMisses`; `pseudo_pad_one_word` by
`Layout.PseudoLocale`; `button_wrap_ignores_icon`, `job_words_under_button`, `tree_hub_fixed_width`,
`job_picker_blurb_full_width` by `Layout.Languages`), the clean sources PASS. The pre-fix state was each fix's failing case (the first runs' findings, by part and language).
`port_po --check` OK; every culture 837 / 837, 0 stale, 0 broken. `check_all` (everyday) passed, exit 0
(`build/check_all_20261120_pass131.log`).

### Open
- **A native read of every language** before it ships (each is a draft).
- The designer's eye on the shrunk headlines and the slot picker at 150 % UI (the test proves the boxes, not the look):
  `py tools/godot_tour.py --skip-godot --res 1280x800 --uiscale 1.5 --culture de` shoots them.
- A packaging gate for the shipped languages; the store page's Dutch and Danish. (129)'s items stand.

### Next step
The packaging gate for the shipped languages (every shipped culture complete in the packaged build), then the store page
in Dutch and Danish; the native reads are the designer's (or translators') to arrange.

## 2026-11-20 (130) - Phase 7: `loc_status --apply`; every language translated (22 x 837 lines, drafts)

**Where the project is.** On (129), uncommitted. The designer: "Do next" ((129)'s next step: `--apply`, then the
translations).

**What happened** (`docs/WORKLIST-LOC-2026-11-18.md` §2c).
- **The files are the translations.** `port_po.py` rebuilt each culture's files from Godot's `.po` alone, so a run after
  a hand translation would have dropped it. Now a run keeps every row in use (Godot's or the port's own), fills only what
  Godot has for today's English, keeps a stale or broken row (the old English shows beside the new) and drops a row whose
  key left the tables. Dutch and Danish get files with their first row. Its report counts Godot's, own and stale rows.
- **`loc_status.py --apply <work file>`**: the Translation column of `build/loc/todo/<culture>.csv` into the culture's
  files, Source = the English the translator saw; all or nothing (no such line, a key twice, a placeholder lost or added);
  written through `port_po`'s merge. `--todo` never overwrites a work file with translations not yet applied.
- **Fixed:** `loc_status.py` counted an escaped line break's `n` as a letter (`TITLE_leaderboard_text` needed words for
  it, not for the game): 838 against the game's 837. **Font coverage read only Godot's `.po`**: a character new in the
  port's own lines, and all of Dutch and Danish, went unchecked; it now reads the translation files.
- **Every language translated: 22 x 837 lines** (Godot's 20: the 319 lines each lacked, 322 in cs/pl/ru/uk; Dutch and
  Danish: all 837), each with its carried translation's terms. Dutch and Danish terms chosen here (the store page has no
  Dutch or Danish yet and follows them): nl Kattenkruid, Sterrenstof, Lokaas, kittens, Sprong, Herontwaken, Rijken,
  Vangsten, Vaardighedenboom, Beroep; da Kattemynte, Stjernestøv, Madding, killinger, Spring, Genopvågn, Riger, Fangster,
  Færdighedstræ, Erhverv. Grammar the placeholders cannot follow (KB U/20 §3): cs/pl/ru/uk put a count after a colon (no
  ICU plurals needed), Hungarian and Turkish put no suffix on a placeholder, Swedish avoids `{v}:e`. Numbers keep the
  game's '.'. Every language is offered in Settings and on the title, each still "(draft)" in its own words.
- **Two of Godot's carried translations fixed** (D16, faults; Godot's files untouched): Traditional Chinese's spawn rate
  重新整理速度 (a machine conversion: "browser refresh") is 生成速度 / 生成率; Czech "Duhový Hovínko" (wrong gender) is
  "Duhové hovínko".
- `store_shots.py --culture X`: the store scenes in any language (`build/steam/shots/<culture>/`). **Found in the Russian
  shot:** the locked Kitten Speed row's one line ran past the row (a line holds about 47 characters). Fixed in the
  translations, through fix-only work files and `--apply`: Kitten Speed on two lines (name, then the requirement) in
  ru, uk, ko, fr, pt-PT, pt-BR, es, es-419, it; Kitten Power (already two lines) with a shorter requirement in the
  Romance languages. Other screens' long lines are for the pseudo-locale tour (worklist step 4).
- Tests for 23 languages: `Language.Translations` (22 cultures), `.Labels` (Godot's 21 labels, then "Nederlands
  (concept)", "Dansk (udkast)", Dutch and Danish after Swedish), `Settings.Language`, `Title.Language`, `Layout.Title`
  (the list's 23 rows at the four sizes).

**Proof.** `loc_status --self-test` PASS (the refusals, the Source seen, the merge order, the guard, an apply into a copy
of the German files less Godot's gaps that a later `port_po` run leaves byte for byte); 11 planted faults in the two tools
each failed it (`safe_mutate`). `port_po --check` OK after every apply; every culture 837 / 837, 0 stale, 0 broken.
Font coverage PASS (26 culture lists, 6938 characters); a planted unsupported character in a German line CAUGHT. Engine:
`Language`, `Settings`, `Title`, `Layout.Title`: 25 PASS. Real game (`store_shots --culture`): German, all nine scenes,
nothing overflows; Russian (three scenes) and Thai (two): Thai renders
whole; Russian showed the locked-row overflow below, fixed and reshot in Russian, European Portuguese and French. `check_all` (everyday) passed, exit 0 (`build/check_all_20261120_loc3.log`).

### Open
- **A native read of every language** before it ships (each is a draft; the list shows "(draft)").
- A complete language drops out of the lists when an English edit makes one of its lines stale (complete only, as
  Godot): a packaging gate for the shipped languages is to build before release.
- The store page's Dutch and Danish (with the terms above); the pseudo-locale tour (worklist step 4).
- (129)'s open items stand: the list's look; whether players see unfinished languages (moot while every one is complete).

### Next step
The pseudo-locale tour at 1280 x 800 and 150 % UI (worklist step 4), then a packaging gate for the shipped languages;
the native reads are the designer's (or translators') to arrange.

## 2026-11-20 (129) - Phase 7: the title's language list

**Where the project is.** The designer: "Commit and push. Do next step". (128) and everything before it committed and
pushed (`56a5a4e`). This entry's work is uncommitted.

**What happened.** The cluster's Language (`main_menu.gd` `_open_language_menu`) opens a card above it: one row per
language offered (`PawText::Languages`: complete ones; `-PawDraftLanguages`: unfinished too), each in its own name with
its own "(draft)"; the language shown on the yellow plate with the tick (the concept's "you are here"). A layer like the
others (`ELayer::Language`, between the leaderboard and the slots, as Godot's `_focus_top`): its rows have the keys,
starting on the language shown; a row saves and shows its language and closes the list; Back, the button again or a
press off the card close it (`PressOffLanguageMenu`, the clear scrim's); the title's code-built words (the hint, the
status line) are made again when the language shown changes (it hears `UPawSettingsSubsystem::OnChanged`). Built in the
code layout and grafted into `WBP_MainMenu` (`PawMainMenu=add:LanguageLayer`: the layer, the card, a width box, a scroll
box, the list); the rows are made on every open. Sizes in `PawUiConstants.h` (`Language*`). The tour's `paw.Title`
takes `language`. The concept draws only the button: the list is the kit's card and rows, for the designer's look.

**Proof.** `Pawpocalypse.UI.Title.Language` (complete languages only, then the review's 21: names, the yellow plate and
tick on the language shown only, the keys' rows and start, Back, the toggle, a press off the card, a row saving `de`,
the hint and status made again in German, reopened on German); `Layout.Title` with the list open at its longest at the
four sizes (on screen, above the cluster: at 1280 x 800 and 150 % the 21 rows scroll inside the card);
`Title.Look` (Language live); the widget parity tests: 28 PASS. 8 sabotages `title_language_*` CAUGHT. In a real game
window (`-game`, `-NoSave`, `-PawDraftLanguages`): the list in English, and started with `-culture=de` the title in
German with Deutsch ticked (`Saved/PawShots/title_language_en.png`, `_de.png`): the packaged path, which the editor tests
cannot take. `check_all` (everyday) passed, exit 0 (`build/check_all_20261120_title2.log`; the first run failed only on
`prompt_title_hint_words`, whose target now occurs twice: retargeted, seen CAUGHT).

### Open
- The list's look is the kit's, not drawn by the concept: the designer's word (screenshots above).
- (128)'s other open items stand: `loc_status --apply`, the translations, ICU plurals, other screens' cached words.

### Next step
`loc_status --apply` (a translated work file into the culture's files, stamped with the English the translator saw),
then the translations themselves, language by language, each with a native read before it ships.

## 2026-11-20 (128) - Phase 7: the translations at run time, the languages' rules (F52), Settings > Language

**Where the project is.** On (127), uncommitted. The designer: "last session stopped. continue it". Session (127) had
ended cleanly, waiting for the designer's word on the card drafts and the skill and badge drafts (no answers or comments
on either page yet). Continued with what needs no decision: phase 7's next step from (125), the run-time loader and the
language list. No art, words or numbers changed.

**What happened.**
- **The languages' rules: `PcLocales`** (PawCore; port of `locales.gd`): each language's name in itself, Unreal's culture,
  Steam's name; `available`, `from_system`, `from_steam`, the picker's label, `apply_language`'s choice (`LocaleResolve`).
  The setting keeps Godot's codes (`es_MX`, `pt`, `zh_Hans`): settings.cfg stays Godot's. Dutch and Danish in the table
  after Swedish. Proven against the real `locales.gd` (new fixture `locales`: the script recompiled with the OS language
  and the loaded `.po` set as probes; 104 loaded sets x 85 OS locales and 28 Steam names, and every language's label with
  its own "(draft)" from Godot's `.po` files; reproduces byte for byte). **F52 fixed (D16, doc 01 §5)**: a Portugal system
  got Brazilian Portuguese, Spanish outside Spain and Mexico got Spain's Spanish, Macau got Simplified Chinese
  (`godot_fixes.py` applies the same fix to the fixture's Godot; the designer's findings file has F52, uncommitted).
- **The loader** (`PawText::RegisterTranslations`, at module start): the carried files registered as polyglot text; a
  row is used only when its Source is today's English and its `{placeholders}` are the English's. Counts per culture:
  in use, left out, missing (838 lines need words, counted as `loc_status.py` counts). `PawText::ShowCulture`.
- **Shown at start and on change** (`UPawSettingsSubsystem::ApplyLanguage`): the setting resolved (system: the OS
  language), `-culture=` wins, never in the editor. The store shots and the pad tour pass `-culture=en`.
- **Settings > Video > Language** opened: "System", then every complete language in its own name with its own
  "(draft)" (`SETTINGS_language_draft`: Godot's "(draft)", carried with its 20 translations). **Only complete languages
  are offered** (Godot: only loaded `.po` files, "so an unfinished language cannot be chosen by accident"; its files were
  complete). No language is complete today (320 lines each), so the list is System and English; **`-PawDraftLanguages`**
  lists the unfinished ones (their missing lines in English). A click's pick waits one tick (the list is refilled
  outside its own selection callback). The Video page now opens on Language (Godot: its first control).
- **Found:** in the editor, game translations never showed: the localisation preview needs a compiled `Game.locmeta`
  (none here) and, when on, writes polyglot strings in the editor ini's preview language. `Localization.ForceLoadGameLocalizationInEditor`
  does it (KB U/20 §3, dated note; symptom index). `tools/run_automation.py` crashed printing a failure's Japanese on a
  cp1252 console: fixed (a stand-in character).

**Proof.** PawCore 211 cases PASS (`Locales: *` four cases, every fixture row); 10 PawCore sabotages CAUGHT
(`locales_*`). Engine: `Pawpocalypse.UI.Language.Translations` (20 cultures; German and Japanese live, a text made
before the switch follows it, a formatted line, a two-line line, a stale line in English until its English matches),
`.Labels` (every label Godot's), `.Loader` (crafted files: each kind of row, the counts, a complete Danish offered, an
unfinished German only on request), `Pawpocalypse.UI.Settings.Language`, and the settings suite: 13 PASS. 14 game-module
sabotages `language_*` CAUGHT (two first refused to compile or lost their target: fixed and seen caught). `check_all`
(everyday) passed, exit 0 (`build/check_all_20261120_lang2.log`; the first run failed only on a sabotage target this
session had moved). The `locales` fixture reproduces byte for byte (`make_godot_fixtures.py --only locales --check`).

### Open
- **The title's Language button and its menu** (Godot's `_open_language_menu`: a paper list above the button, the
  current language ticked, hover and focus, Escape and a click outside close it). The concept draws only the button: the
  menu follows the kit and goes on the morning page.
- `loc_status --apply` (a work file into the culture's files, stamped with the English the translator saw).
- The translations themselves: 320 lines in each of 18 languages, 323 in cs/pl/ru/uk, 838 in Dutch and Danish.
- Code-built text that a screen turns into a string once (lists, cached labels) shows the old language until the screen
  rebuilds: Godot rebuilds on `language_changed`. Settings rebuilds its lists; the title, HUD and tree are to check with
  `-PawDraftLanguages` once their language can change while they are up.
- For the designer: whether players should see unfinished languages (today: no, as Godot).

### Next step
The title's language menu (a grafted layer in `WBP_MainMenu`, rows built from `PawText::Languages()`), its tests and
sabotages; then `loc_status --apply`; then translation.

## 2026-11-19 (127) - The skill pictures and the achievement badges drafted (1.12)

**Where the project is.** On (126), uncommitted. The designer: "Do drafts". Taken as: draft the skill and achievement
pictures now, the same way as the cards. The card drafts are not yet approved, so nothing was recorded into the game.

**What happened.** Two new draft files, in the kit's grammar like `concept_cards.js` (primitives the recorder knows, colours
only from L.c, U or mix() of them), run after the concept's kit, `concept_cards.js` and `concept_icons.js`:
- `tools/art/concept_skills.js`: `SKILL.<id>` for the 77 skill nodes (the ten job nodes keep their approved hats),
  `SKILL_WHY`, the helpers `SK`. One mark per idea (its header lists them): the aura = the gold ring of the approved Aura
  Radius icon, Catnip = the leaf, damage = three curved red scratches (`SK.claws`; not the concept's straight white lines:
  the designer on Shred, "The lines are too straight"), crit = the Crit Chance burst, increase = green arrow, one more =
  green plus, automatic = gear. Skills named as an approved upgrade show it (Pounce Power, Aura Reach, Overkill); Dent is
  the approved Shred icon with the arrow.
- `tools/art/concept_badges.js`: `badge(family, tier)` for the 18 families in `Data/AchievementData.csv`: today's rule
  kept (`sweet_ui.py`: one rosette, rim = tier by rank in thirds), the kit's scallop rosette, a paper centre (br_*: the
  tree's hexagon), the family's symbol at 0.68 (smaller drowned in the kit's fixed line). up_* = that upgrade's dock icon.
- Review page: https://claude.ai/artifact/KYQRRKXK3xcMR1mfEpxXCq (generator `build/drafts/make_skill_badge_drafts.py`;
  names from `ST_Data`, so Stacked Deck, not Godot's name; `build/drafts/smoke.js` renders every piece headless: 338, none
  empty; sheets `build/drafts/sheet.js SKILL|BADGE`). Four questions on it: the skills, the badges, the red scratches for
  damage everywhere, and redrawing the cards' coins (Catnip) and blue ring (aura) to the leaf and the gold ring.

No game file changed; no gate touched (`sticker_record.js` reads only `concept_icons.js` and `concept_hats.js`).

### Open
- For the recording step: the card drafts use two things the recorder refuses: `KC.crater`'s literal `#5E3F2A` (no name)
  and `hot_borders`'s `R(..., 'none')` (not a colour). Name or redraw them before recording the cards.
- Not drafted: the art's `achievement_br_ferocity_5` and `achievement_br_mystique_6` (no such achievement in `Data/`), and
  `skill_prism`, `skill_full_spectrum` (no node uses them).

### Next step
The designer's word on the card drafts (https://claude.ai/artifact/EcGLpgn5XtJTHjRSXaoxbv) and on these. On approval:
`sticker_record.js` reads `concept_cards.js`, `concept_skills.js` and `concept_badges.js` as it reads
`concept_icons.js`; `sticker_art` writes `cards/card_<id>.svg`, `skills/skill_<id>.svg` and `ui/achievement_<id>.svg`
(the badge's tier from the family's rank, as `sweet_ui.ACHIEVEMENTS` does); raster, import, `check_all`.

## 2026-11-19 (126) - The designer's answers; the skill tree's hexagons (1.8b)

**Where the project is.** On (125), uncommitted. The designer, on the review page's questions: "1yes. 2 do hexagon. 3 yes":
the two draft words approved (`SHOP_amount_label` "Buy amount", `HUD_level` "Level {level}": TEXT-DRAFT and ST_UI say
so); the skill tree's nodes as the concept's hexagons; drafts for the card, skill and achievement pictures (next).

**What happened (1.8b).** Every node is a hexagon, point up, filling its box top to bottom (the concept's treeNode),
painted by a new piece `UPawHexPlate` (its own Widget Blueprint, `WBP_HexPlate`; in `PawUi::BakedClasses` before the
node) that the node holds as its FIRST part, under the icon and the counter: a user widget's own NativePaint runs after
its children, so the node painting the hexagon itself covered its icon (seen, then moved). The plate takes the state's
fill and edge (the canvas wants sRGB floats: the tokens are converted) and the node's tint (its shine). The node's old
Frame stays, clear. The rainbow, pulse and job rings trace hexagons (`UPawTreeLinks::HexPath`); the glow behind a node to
buy now is round (Godot: a rounded square).

**Proof.** `UI.Tree.Paint`: the old frame clear, the plate painted on an arrange, an unlock's edge heavier, the ring a
closed hexagon point up. Sabotage `tree_node_no_hexagon` CAUGHT. Graft: `PawTreeNode=add:Plate` (WBP_HexPlate baked).
A crash on the way: `Pts.Add(Pts[1])` (KB Unreal/24 has it; copied first now). `Pawpocalypse.UI` + `Input` 119 PASS.
Shot `build/night5/tree_hex.png`.

**Then (the designer's 3).** The 28 card pictures drafted in the UI concept's kit grammar (`tools/art/concept_cards.js`:
`CARD.<id>`, helpers `KC`, a one-line "what it shows" per card in `CARD_WHY`), each beside today's picture, on
https://claude.ai/artifact/EcGLpgn5XtJTHjRSXaoxbv (generator `build/steam/page/make_picture_drafts.py`; a sheet for a
look, `build/night5/cards_sheet.js`). Drafts only: nothing in the game. Batched on purpose: the style approved on the
cards first, then the 79 skill pictures and the 20 achievement families the same way. `check_all` passed, exit 0
(`build/check_all_20261119_hex.log`; R4f's round-node sabotage became `tree_hex_on_its_side`, CAUGHT).

### Next step
The designer's word on the card drafts. On approval: record them (`sticker_record.js` reads `concept_cards.js` as it reads
`concept_icons.js`; `sticker_art` writes `cards/card_<id>.svg`), raster, import; then draft the skills and achievements.

## 2026-11-19 (125) - Night: localisation started: Godot's 20 translations carried over, the status and work files

**Where the project is.** On (124), uncommitted. The restyle's screens are done but 1.8b (the tree's hexagons) and 1.12
(card, skill and achievement art: drafts for the designer first); section 2 (localisation) started, as ordered.

**What happened.** `tools/port_po.py` carries Godot's translations to the port's three string tables by English and
context into `Content/Pawpocalypse/Localization/Translations/<culture>/<Table>.csv` ("Key","Text","Source": the English
each was made for, so a later English edit makes only that line stale). 517 of 837 lines in 16 languages, 514 in cs, pl,
ru, uk; 320 lines per language to translate (the port's changed and new English), all 837 in Dutch and Danish.
`tools/loc_status.py`: translated / missing / stale / broken per culture (self-test plants each case); `--todo` writes
`build/loc/todo/<culture>.csv` with the English, the comment and the glossary's terms. Both tools are gates in
`check_all` (`translations carried over`: a planted row CAUGHT). Culture names settled (es-419, pt-PT, ...). Details,
and the proposed run-time route (polyglot text, KB Unreal/20 §3): `docs/WORKLIST-LOC-2026-11-18.md` §2a.

### Open
- Translating: about 8,100 lines (320 x 20 + 837 x 2), each language with a native read before it ships.
- `--apply` (a work file into the culture's files, stamped with the English the translator saw), the run-time loader,
  the language menu, ICU plurals for cs / pl / ru / uk.

**End of the night.** The everyday `check_all` passed, exit 0 (`build/check_all_20261119_night2.log`; it first caught
a `FromString` and two colour literals in the Unicorn ceremony's paint: fixed). The Steam screenshots were shot again
with tonight's game (`build/steam/shots`, 9 of 9; not uploaded to Drive). The morning review page (each screen beside
the concept, the faults found, what waits for the designer, the translation table):
https://claude.ai/artifact/RCMPJYVDVQB1cWnzWHj54m (generator `build/night5/make_review.py`).

### Next step
The designer's answers on the review page (the two draft words; the tree's hexagons and its store shot; drafts for the
card, skill and achievement pictures; the confetti and the Bait panel). Then phase 7 on: `loc_status --apply`, the
run-time loader and the language menu, and the translations themselves (`build/loc/todo/<culture>.csv`).

## 2026-11-19 (124) - Night: the victory screen and the check box as the concept draws them (restyle 1.10, 1.11)

**Where the project is.** On (123), uncommitted.

**What happened.**
- **Victory (scrVictory):** VICTORY at the concept's 84 px with the ceremony's heavier ink outline; the realm stays bright
  under the panel (the layer still takes every click but paints nothing; Godot: a 0.88 near-black dim). Not built: the
  confetti (the concept's; a field effect for later).
- **The check box (the concept's toggle()):** 26 px, cream when off, yellow with the ink tick (`ui_tick`, now inside the
  box: `CheckTick`) when on, on an ink edge (R3: an ink fill when on, a muted edge). One piece: every check box in the
  game (Settings, Ask before Reawakening, Auto-travel, Auto-pick).
- 1.12 (the card faces' art, the skill and achievement icons in the kit's style) is NOT started: the concepts never drew
  those pictures, so they are drafts for the designer first (the plan says so); tonight went to the screens.

**Proof.** Grafts: WBP_VictoryScreen (text Headline, restyle Dim), WBP_Button (add CheckTick, restyle CheckBox).
`UI.Victory.Ending` reads the clear layer in the asset AND the code layout (a code-only sabotage was MISSED by Parity, so
the test now checks both), the headline's size and outline; `UI.Hud.ReawakenLook` reads the check box both ways.
Sabotages CAUGHT: victory_dark_dim, check_ticked_ink, check_tick_never_shown. `Pawpocalypse.UI` + `Input` 119 PASS.
Shots `build/night5/victory_a.png`, `check_a.png`.

### Next step
Section 2 of the night's plan: localisation (phase 7), starting with Godot's translations carried over.

## 2026-11-19 (123) - Night: the Unicorn ceremony as the concept draws it (restyle 1.9)

**Where the project is.** On (122), uncommitted.

**What happened.** The ceremony (PawCore's Godot-exact timeline, painted by `UPawUnicornCeremony`) gains the concept's
scrUnicorn: its coral rays and four white sparks (`ui_icon_unicorn_rays`, `sticker_art.write_ceremony`, the concept's
literals `UNICORN_RAYS` / `UNICORN_SPARKS`; 1920 x 1080 like the Reawaken ceremony's rays) behind the rainbow instead of
Godot's ink dim, faded in and out as the dim was (the dim stays where the picture is missing); "UNICORN" as its sticker
title (the skill's name in capitals, yellow on a thick ink outline) and the line that says what changed
(`REAWAKEN_unicorn`, the Reawaken tab's words: no new string) on a cream plate under the rider. Painted, so no layout
asset changed. The title sits at 80 in the concept's frame (its 140 overlapped the game's rider, who stands higher).

**Proof.** `UI.Unicorn.Plays` asserts the rays, the title and the line (the batch count lost the dim's 2 triangles);
`UI.Icons` exempts the picture from the 128 px rule like the other ceremony pictures. Sabotages CAUGHT:
unicorn_dim_not_rays, unicorn_no_title. Shot `build/night5/unicorn_a.png`.

### Next step
1.10: the victory screen as the concept's scrVictory.

## 2026-11-19 (122) - Night: the skill tree in the concept's colours (1.8a); unreadable text on plates found and fixed

**Where the project is.** On (121), uncommitted.

**Faults found (the store shot 07 shows them).** The skill tree's Stardust pill and its legend showed NOTHING: their text
was the plum era's cream (`SKY_STAR`) on the now-cream plate, 1.00:1. The pause menu's status line was green on cream
(3.3:1) and green even for "Slot n was not saved." and the overwrite warning. Fixed to ink. Continue stood on the legend
in the spend step at every size.

**New guard: `Pawpocalypse.UI.Widgets.ReadableOnPlates`.** Every text, prompt line and icon line every Widget Blueprint
puts on a painted plate must read against it (WCAG 2 AA: 4.5:1, 3:1 at 24 px and up; a sticker title is judged by its
ink outline). It FAILED on exactly the three faults above, then passed after the grafts. Sabotage
`tree_text_cream_on_cream` CAUGHT (Parity).

**What happened (1.8a, the concept's scrTree, colours and chrome).**
- The sky: the concept's night blue (`SKY` in `palette.py`; the plum sky kept as `SKY_PLUM`), its pale yellow dots, the
  clouds a breath lighter than the blue.
- Nodes: owned green on ink, a node to buy now raised in the heavy gold ring, the rest slate on a slate edge with the
  icon faded (Godot: the trunk's colour, a brightness ladder). The lock is NOT drawn on slate nodes: "Locked" here also
  means "reachable but too dear", and a lock would mislead.
- Links: ink with a cream core where owned, slate where not.
- Continue: yellow with the Reawaken swirl; it stays big and in the middle (the designer, 2026-08-23: "not so in the
  corner"; the concept has it in the corner), raised above the legend; the legend moved to the bottom-left (the
  concept's place).
- Every dimming scrim (pause, settings, card pick, job picker, the title's layers) is ink now, the concept's scrim; they
  borrowed the plum sky's base, which is now blue.
- Tests updated to the new rule: `Tree.Paint` (owned green on ink, ready in the gold ring, locked darkest), the PawCore
  sky test (the fixture proves position, size and alpha; the colours are the palette's: star, clouds between its two;
  the base compared in float). New: `Layout.Tree`'s spend step (Continue covers no legend: FAILED before the move).
  Sabotages CAUGHT: tree_owned_not_green, sky_star_in_cloud_colour (PawCore).
- `Pawpocalypse.UI` + `Input` 119 PASS; PawCore 207 PASS. Shot `build/night5/tree_a.png`.

### Open
- 1.8b: the concept's hexagon nodes (today: round skills, rounded-square unlocks), its yellow star hub and the branch
  name pills; the side panel as its white card with the Buy button.

### Next step
1.9 (the Unicorn ceremony). Checkpoint: the everyday `check_all` passed, exit 0 (`build/check_all_20261119_night.log`), after eight older sabotages were aimed at tonight's code (each seen CAUGHT again; `wave_plate_flat` became `field_plate_flat`) and the sky test's casts written without the word float (PawCore's no-float rule).

## 2026-11-19 (121) - Night: the boss panel (restyle 1.7), and why it covered the pills

**Where the project is.** On (120), uncommitted.

**A bug found (in the store shot 02 too).** At 1920 x 1080 the boss panel sat at x 12, over the Catnip and Stardust
pills. Cause (engine): Slate centres a child at `(allotted - child) / 2 + padding.Left - padding.Right`
(`LayoutUtils.h` AlignChild), not in the middle of the padded room, so the port's "centred on the field" (HAlign_Center,
the dock's width as padding) put it at (1920 - 760) / 2 - 568 = 12. KB `Unreal/24` has the row now.

**What happened (1.7, the concept's scrBoss).**
- `PlaceBossPanel` (every tick, writes only on change): the panel BESIDE the status panel, between it and the dock, where
  480 units fit; under the status panel where not (1280 x 800 at 150 %, 1920 x 1080 at 150 %); as wide as 760 or the
  room; placed left-aligned at a computed x, the width set on `BossBox`.
- The line heavy on the left; SHIELDED on its own red tag on the right (`BossShield`; Godot: a suffix on the line, so
  `BOSS_shield_suffix` is gone and `BOSS_status` lost its always-empty `{shield}`: no visible word changed); the bar the
  concept's (a track, an ink edge, the coral fill; Stardust while shielded).

**Proof.** New `UI.Layout.Boss`: summons the Park Tyrant and measures at the four sizes, dock on either side: the panel
covers no pill, the status panel or the dock, and at 1920 x 1080 sits beside the status panel. Godot's placement put
back (sabotage `boss_centred_on_the_field`) is CAUGHT by it; `boss_shield_tag_hidden` CAUGHT by `Hud.Boss` (new shield
checks). Grafts: wrap BossText > BossHead, add BossShield, restyle BossBar. `Pawpocalypse.UI` + `Input`: 118 PASS.
Shot: `build/night5/boss_a.png`.

### Open
- **The Steam screenshots need shooting again** (`py tools/store_shots.py`): 02 shows the old overlap, and every shot
  shows the HUD before tonight's restyle. Then the Drive store folder (it still has v2 of the text).
- The controls table's cells use the same centre-with-left-padding pattern (`PawControlsPanel.cpp` ~284): a 5 px shift,
  not chased.

### Next step
1.8: the skill tree as the concept's scrTree.

## 2026-11-19 (120) - Night: the dock's other five tabs as the UI concept draws them (restyle 1.2 to 1.6)

**Where the project is.** On (119), uncommitted; the night's plan `docs/WORKLIST-NIGHT-2026-11-18.md` (1.1 to 1.6 done).

**What happened.**
- **Reawaken (scrReawaken):** the payout in its own box (`PayoutPanel`: the heading muted and spaced, the Stardust star
  and "+n" 48 px in the concept's purple `UI_STARDUST_BRIGHT`, the total beside it, both multiplier lines heavy); "Ask
  before Reawakening" a plain check box (flat) ABOVE the button; the button yellow when it can be pressed, with the
  Reawaken swirl (Godot: Stardust's); the tab opens on the button. The rainbow progress bar is kept (Godot's "ready").
- **Realms (scrRealms):** the stage line heavy; every row's words on the left after an icon (a flag to travel, the paw
  where the cat is, the lock where it cannot go, Catnip where Catnip is missing); the cat's biome on the yellow plate and
  no longer locked (a press there does nothing: PcHud's row is not enabled); a newly open biome only leans (R4a made it
  yellow; yellow now means "here", as drawn); Auto-travel a plain check box under the map. Not built: the Bait box's own
  rainbow-framed panel (the Bait button keeps its call look and lean).
- **The Catches (scrCatches):** every card stays listed (designer, 2026-09-11), ordered: the run's cards, then the ones
  that can be taken, then the ones that cannot (banished, a skill or kittens missing), those locked with the lock; every
  row on its plate with the card's picture; the summary heavy; Auto-pick a plain check box under the list. The concept's
  "four slots" are a drawing, not a rule (nothing in Godot or the port limits a run to four); no "Not available this run"
  heading (it would be a new string: the designer's call).
- **Achievements (scrAwards):** each row on its plate (a secret one on the off plate); the name heavy and ink, the line
  under it muted; a bar before the numbers while it is still going; the trophy faded until earned; Claim and Claim all
  yellow. Godot's ★◆☆ disc (a stand-in until art) is hidden: the trophy and the plate say it.
- **Jobs (scrJobs):** each job on its plate, the worn one yellow, a locked one on the off plate with its hat faded; the
  name heavy. The state pill (Worn / Owned / Locked) stays on every row: it is what a pad selects to read the row.
- Parts the grafts find by name are named now (`ReawakenBarBox`, `RealmsBlurb`, `JobsBlurb`, the payout's parts): an
  unnamed text's auto-name shifted when the payout heading got its name, and Parity compares unnamed texts by auto-name.

**Proof.** Grafts into WBP_HudScreen (the payout lines out, `PayoutPanel` in fresh; Ask, Auto-travel and Auto-pick moved;
the stage line and summary restyled). New test `UI.Hud.ReawakenLook` (FAILED on the committed asset); `Hud.Realms`,
`Hud.Catches`, `Hud.Achievements`, `Hud.Jobs`, `UI.Icons` assert the new rules (older assertions of the old look updated:
the here row locked and green, card rows by index, the box first in the Catches column, the Reawaken button's star).
Sabotages CAUGHT: reawaken_never_yellow, reawaken_opens_on_the_check_box, realm_here_not_yellow, realm_rows_bare,
catches_unordered, catches_out_of_reach_open, awards_no_bar, awards_claim_plain, job_worn_plain. `Pawpocalypse.UI` +
`Pawpocalypse.Input`: 117 PASS. Shots in `build/night5` (`reawaken_a`, `realms_a`, `catches_a`, `awards_a`; the concept's
`c_*.png`). Not shot: Jobs (it needs a job bought through the spend step).

### Open
- The card pictures on the Catches rows are still the grammar's (S6: 1.12). The check boxes still draw Godot's square
  (1.11).

### Next step
1.7: the boss panel as the concept's scrBoss.

## 2026-11-18 (119) - Night: the HUD's Upgrades tab as the UI concept draws it (restyle 1.1)

**Where the project is.** On (118), uncommitted. The designer: "Add dutch and danish. Restyle then translate. Create an
overnight plan." The plan: `docs/WORKLIST-NIGHT-2026-11-18.md` (section 1, the restyle's remaining screens, then section 2,
localisation; Dutch and Danish join: 23 languages, `docs/WORKLIST-LOC-2026-11-18.md`). First the everyday `check_all`
passed (exit 0), so (118) is proven.

**What happened (1.1, the concept's scrUpgrades).**
- **The dock:** each page's title heavy, 30 px, on the left (`MakePageTitle`); the buy amount on its own row, "Buy amount"
  (muted) and a switch of four (x1 / x5 / x10 / Max) on a raised plate, the amount in use yellow (`SetActiveLook`), a press
  picks it, Q still steps through them, focus follows the pick, left / right along the switch, up to the tab, down to the
  first row (the switch is one stop of the column: the focused button, else the chosen). The old "Buy x1" button and its
  string `SHOP_buy_multiplier` are gone. The legend now above the quick bar; the quick bar's four buttons on plates
  sharing the width.
- **The status:** the pills' numbers 30 px (R4's 24); ONE panel under them, as wide as the pills: the biome's name (heavy),
  the next biome's line over its green bar, the wave line (muted) over its blue bar (Godot: two plates). The rainbow and
  the lean go round the whole panel now. The bars are the concept's (`UI_BAR_TRACK / ZONE / WAVE / XP`, new tokens from
  the recorded concept colours: `palette.py` UI).
- **The level:** along the bottom on a plate, "Level n" (heavy) before a purple bar; the next level's number stays at the
  bar's end (the concept leaves it out; it says where the bar leads).
- **Pounce:** "Pounce" and its key on a plate beside the clock (shown with the clock), centred on it.
- **The toast:** the concept's controller icon (`ui_icon_pad`, new: recorded from the concept's IC.pad, `sticker_art`
  NEW_ICONS) before the words, the words heavy.
- **Locked shop rows** show the lock (`ui_lock`, now rasterised and imported) instead of the greyed upgrade icon.
- New words, DRAFTS for the designer: `SHOP_amount_label` "Buy amount", `HUD_level` "Level {level}"
  (`docs/TEXT-DRAFT-2026-11-19.csv`; REAWAKEN_blurb carried).
- Not built: the concept's tutorial tip plate (the tutorial is not ported yet: phase 6's, only its setting is stored).

**Proof.**
- Grafts (the assets own the layout): `PawHudScreen` (remove MultiplierButton, add AmountRow; the six titles' text and
  slots; DockLegend moved; the quick buttons' props and slots; ZonePlate / WavePlate / XpRow / PouncePrompt removed and
  FieldPlate / XpPlate / PounceBox added fresh, every part NAMED: an unnamed part's auto-name drifts within one graft run,
  so restyles of `SizeBox_3` and `SizeBox_5` failed and saved nothing), `PawToast` (wrap Message > Line, add Icon).
- New tests: `UI.Hud.DockLook`, `UI.Hud.StatusLook`, `UI.Hud.PounceLook`; `Hud.Readouts` and `Hud.Navigation` read the
  switch; `Input.Device` the toast's icon. Each look test FAILED on the asset as committed (`build/night5/neg_asset.py`:
  the committed WBP swapped in through safe_mutate, then back). Older tests that pinned the old look updated (Chrome's
  table, the quick bar bare, the rainbow and the lean on the zone plate).
- Sabotages (`tools/sabotage_realm.py`), all CAUGHT: amount_press_steps, amount_never_yellow, amount_switch_stuck,
  page_title_light, biome_name_blank, rainbow_round_the_zone_line, level_number_alone, pounce_plate_always,
  locked_row_own_icon.
- `Pawpocalypse.UI` + `Pawpocalypse.Input`: 116 tests PASS. Static gate PASS. Shots: `build/night5/dock_a.png`,
  `status_a.png`, `upgrades_b.png` (concept: `build/night5/c_upgrades.png`).

### Open
- The two draft words. The Pounce plate is proven by test; a scripted shot of it needs Pounce bought through the spend
  tree, whose exit starts the next run (not chased).

### Next step
1.2 in `docs/WORKLIST-NIGHT-2026-11-18.md`: the Reawaken tab as the concept's scrReawaken.

## 2026-11-18 (118) - The designer's answers: the rest of Missing Art, the shop words, D19, the store signed off, languages

**Where the project is.** On (117), uncommitted. The designer: "The other missing art is ok. Do next steps. What are 3 shop
words? Ask me questions about the rest", then eight answers (below).

**The answers (2026-11-18).**
- The shop words `Lv {level}`, `MAX`, `Lv {level}  +{levels}`: approved (TEXT-DRAFT and ST_UI notes say so).
- E1, Fredoka titles: "We will work on title and logo in detail later. Keep as is for now."
- Store page: the text approved; length "Leave length off"; the top five tags approved; the AI disclosure "I'll write my
  own"; languages: "Check all most used steam languages as translate into all of them. Also if we make changes later
  make sure that we don't have to retranslate everything and just redo things we changed."
- Q13: "Raise the five prices": D19.

**What happened.**
- **The six other upgrade icons and Bait are in** (Kitten Power, Kitten Speed, Aura Radius, Overkill, Pounce Power,
  Pounce Recovery; `ui_icon_bait` over the old drawing): `tools/art/concept_icons.js`, copied from the page, recorded.
  `Hud.Readouts` asserts every one of the 15 rows draws an icon (6 FAILED before the import).
- **D19 built:** five late nodes repriced (Curiosity 20,000; Gridlock 20,000; Overflow 70,000; Full Spectrum 250,000;
  Standing Wave 900,000) as `tools/godot_fixes.py` D19 entries (exact text on Godot's `.tres`, the repo untouched),
  `Data/` re-exported (only those 5 rows changed), DataTables imported, the tree fixtures regenerated from Godot with the
  fix. Measured (`tools/econ_review.py`): the game 112.5 -> 113.3 min, 13 Reawakens, the longest stretch with nothing new
  13.9 -> 14.7 min. PawCore test "D19: ..." (seen to fail on Overflow put back to 520 through safe_mutate). doc 01: D19,
  Q13 decided.
- **A store-page error of mine, fixed:** version 2 had changed "Stardust increases damage and Catnip" to "damage" from
  Godot's code, but D17 E3 makes the Reawaken multiply Catnip from kills in this game. English and all 20 translations
  say "damage and Catnip" again; the length line is gone from all 21; the draft is version 3 (signed off).
- **Change tracking for the store translations:** `translated_from.json` (per language and field, a hash of the English
  it was made from); `--todo` lists what to translate again, `--stamp` records it; the pack refuses to build while a field
  is stale; self-test: an English edit makes only that field stale in all 20.
- **Languages** (`docs/WORKLIST-LOC-2026-11-18.md`): Steam's October 2026 survey (figures model-extracted from Valve's
  page: approximate): the game's 21 languages cover about 98.9 %; every language above 0.25 % is in the set except
  Dutch (0.27 %); Dutch and Danish would add about 0.5 %. The game itself is still English only: phase 7's order and the
  game's change tracking (Unreal's archives keep each translation's source) are in that worklist.

**Proof.** PawCore 207 cases pass; D19's negative test caught. UI: Hud + Icons 14 pass after the icons (6 failed before).
Store pack: 21 languages, every check; self-test 8 of 8. Everyday `check_all`: the first run FAILED two gates, both mine: editing `ST_UI.csv` with Python's `write_text` made it CRLF (L0 ui-rules; the ceremony's two-line payout doubled its break, `UI.Ceremony.Plays`); fixed to LF. A blanket CRLF-to-LF pass then broke two generator checks (data export, tree tables): the generators re-run, the static gate PASSES (KB Unreal/20 notes both). The second run was **stopped by Claude Code for low system memory** after 23 gates, all PASS (L0, L1 PawCore, the art and text gates, the editor build, DataTable parity, L3 Data, Realm, UI, Save); **not run: L3 Input and Audio, the game build, sabotage targets, Godot fixes current, Godot repo untouched.** Rerun in the foreground (the designer: "Restyle then translate. Create an overnight plan"): **every gate passed, exit 0** (`build/check_all_20261118_d19.log`). The night's plan: `docs/WORKLIST-NIGHT-2026-11-18.md`; Dutch and Danish added to the languages (23).

**Open (the designer).** Dutch and Danish (add them?); the AI disclosure text; a native read of each store translation;
the clips (needs a screen recorder); E1 later with the title and logo.

**Next step.** Phase 7 step 1 (`tools/port_po.py`: Godot's translations into Unreal's localisation, unchanged lines) and
the dock tabs (S4), in that order unless the designer says otherwise.

---

## 2026-11-18 (117) - Shred and Shield Pierce redrawn and in; settings as the concept draws them

**Where the project is.** On (116), uncommitted. The designer: "Change shread and shield pirce. The lines are too
straight and dint convey tearing things apart"; on Missing Art v3, "Shred b, shield a. Continue".

**What happened.**
- **Missing Art v3** (same link): three ways each to show tearing, drawn in the concept's grammar
  (`build/steam/page/tear_options.js`), previewed locally first (`tear_board.png`: the first slashes were too thin to
  fill, thickened). v4 marks the choice.
- **Shred B and Shield Pierce A are in the game:** `tools/art/concept_icons.js` (identical to the chosen options,
  checked) runs beside the concept in `sticker_record.js`; its icons replace the shop's (`ui_icon_upgrade_shred`) or
  fill a gap (`ui_icon_upgrade_shield_pierce`).
- **`UPawButton::IsIconShown` was wrong:** it counted a kept, empty icon place (Hidden) as shown, so a new assert passed
  for an icon that did not exist. It now means a picture is drawn; the 94 UI tests still pass with it, and the Shield
  Pierce assert failed until the icon was imported.
- **Settings** (the concept's scrSettings): SETTINGS a sticker title (`text:Title` grafted); Close on yellow; the
  controls table's head on a raised band (`wrap:TableHead>TableHeadPlate`), its words muted capitals; every other row
  on a faint band (rows are built at run time). Kept: the checkboxes, per-row Reset, the binding plates.

**Proof.** Icons: `Pawpocalypse.UI.Hud.Readouts` FAILED before the import (Shield Pierce), PASSED after; `check_all`
exit 0 (`build/check_all_20261118_icons.log`). Settings: `Settings.Look` FAILED on the old assets (title, head band;
Parity one border short), Settings + Controls + Widgets + Layout PASS (22) after the grafts. Sabotages `settings_close_plain`, `controls_heads_mixed_case`,
`controls_rows_unbanded`: all CAUGHT. Everyday `check_all`: **every gate passed, exit 0** (`build/check_all_20261118_settings.log`).

**Open (the designer).** Missing Art's six other upgrade icons and Bait; the three shop words; E1 (Fredoka titles); the
store page sign-off.

**Next step.** The other dock tabs (Reawaken, Realms, Catches, Achievements, Jobs), the boss panel, the skill tree, the
Unicorn ceremony.

---

## 2026-11-18 (116) - The approved hats in the field, the job icons, the job picker as the concept draws it

**Where the project is.** On (115), uncommitted. The designer, on Missing Art v2: "The hats are good. The ones that are
specific. Not the top hats" (the seven drafted hats approved; option B for the job icons, each job's own hat).

**What happened.**
- **The hats are recorded like the cast.** `tools/art/concept_hats.js` holds the approved page's seven hats (identical
  to `build/steam/page/make_missing_art.py`'s, checked) and the job -> hat table; `sticker_record.js` runs it beside the
  UI concept (whose `hatFig` draws the other three) and writes `sticker_cast.JOB_HATS`. The hats' literal colours got
  names (`hatCap` ... `hatYellow`). `--check` passes.
- **In the field:** each `hat_<job>` is drawn in one square all ten share (128 px, rimmed like the weapons);
  `sticker_art` works out the placement from it, the mounted one through riderFig's transform, and `PawFieldLook.h`
  holds it: standing (-0.189, -0.202) x 0.775, mounted (-0.010, -0.220) x 0.366 (`build/night4/hats_preview.png`: all
  ten on the Tiny Kitten and on the rider). `Pawpocalypse.Realm.Look.Rules` pins the new numbers.
- **The job icons** (`job_<job>`) are each job's hat alone, at the icon line (option B).
- **The job picker is the concept's** (scrJobPick): a card per job, up to five a row, in a box that shrinks the panel to
  fit; the Tiny Kitten in the job's hat on a photo (`ui_icon_job_cat_<job>`, 256 px; "No job": `_none`, the bare
  kitten); the job's name on its button; its blurb; the lifted card's button on yellow (last run's job opens focused).
  Grafted: `wrap:SizeBox_0>Fit; restyle:SizeBox_0`. Kept: every held job at once and the job's name as the button (the
  concept pages three jobs with LB / RB and a new "Wear" word, which would be new strings).

**Proof.** Look lint PASS; the recorder's check OK. Engine: JobPicker + Widgets + Icons + Chrome + Hud.Jobs + Realm.Look:
16 pass after the graft (Parity FAILED before it, naming the missing ScaleBox). Sabotages `jobpick_cat_bare`, `jobpick_never_yellow`, `jobpick_photo_plain`, `hat_on_the_old_head`: all CAUGHT. Everyday `check_all`: **every gate passed, exit 0** (`build/check_all_20261118_hats.log`).

**Open (the designer).** Missing Art version 3 (https://claude.ai/artifact/J265sfqjxSbdsSNsXU3VSh), after the designer's "Change
shread and shield pirce. The lines are too straight and dint convey tearing things apart": Shred A / B / C and Shield
Pierce A / B / C (drawn in the concept's grammar, `build/steam/page/tear_options.js`; a chosen one goes into
`tools/art` and is recorded like the hats); the other six upgrade icons and Bait. The three shop words; E1 (Fredoka
titles); the store page sign-off.

**Next step.** Settings (Controls with keycaps and pad glyphs side by side), the other dock tabs, the boss panel, the
skill tree, the Unicorn ceremony.

---

## 2026-11-18 (115) - S4: the save slots show each save's cat

**Where the project is.** On (114), uncommitted. The designer: "Go". **The editor may be opened** once the sabotages below
finish (another project's Unreal runs on this machine at times: wait for it, never close it).

**What happened.**
- **Each slot card shows the save's cat at its stage** (the UI concept's slotCard) in a photo well at the card's top:
  - grass for a save (new UI token `photo` #BFE3A3, the concept's; the palette's rules pass);
  - card stock for an empty slot, the off plate for one that cannot be read; no cat in either.
  The stage is the game's own rule read from the file: `Pc::PcSlotSummary` now carries `BossesDefeated` and
  `bFinalBossDefeated`, and `Pc::EvolutionStageFor` is the one rule (`EvolutionStage` calls it). Seven pictures,
  `ui_icon_slot_cat_1..7` (256 px, `sticker_art.write_slots`: the stage's cat with its weapon).
- "Slot N" moved inside the card, at its top (the concept's; under the card it was ink on the dark scrim, hard to read);
  the card is 380 tall. CHOOSE A SLOT is a sticker title.
- The title's figure lambda was named `Cast`, hiding Unreal's `Cast<>` in the same function; renamed `AddFigure` (its
  sabotage follows).
- `build/night4/slots_shot.py`: a slots screenshot with a real save, in a sandboxed folder (`-PawSaveDir` without
  `-NoSave`: that flag switches saving off, so a run with it can never show a save).

**Proof.** PawCore L1: 206 cases pass; the summary test asserts the stage (one boss: 3; out of order: 1; the end: 7);
sabotages `slot_summary_forgets_the_ending`, `slot_stage_ignores_the_ladder_order` CAUGHT. Engine: `Pawpocalypse.UI.Title.SlotCats`
(new) FAILED on the asset before the graft (no cat, no well) and PASSED after; Title + Icons + Widgets + Layout: 22 pass.
Sabotages `slot_cat_always_kitten`, `slot_well_always_grass`, `slot_cat_on_an_empty_slot`, `slot_number_under_the_card`, `title_cat_missing` (retargeted): all CAUGHT. Everyday `check_all`: **every gate passed, exit 0** (`build/check_all_20261118_slots.log`).

**Open (the designer).** As in (114).

**Next step.** The job picker once the hats are approved; until then settings (Controls with keycaps and pad glyphs side by
side), the other dock tabs, the boss panel, the skill tree, the Unicorn ceremony.

---

## 2026-11-18 (114) - S4: the card pick and the title as the UI concept draws them

**Where the project is.** On (113), uncommitted. The designer: "Oh continue with development. Whats next? Are all new art
assets and uis in?" (answered: not all; the list is in the worklist's S4 to S6). **The editor may be opened** (nothing
running once the sabotages below finish).

**What happened.**
- **The card pick is the concept's** (its scrPick):
  - the band holds the flavour alone, spaced out;
  - the picture sits on a panel tinted with the flavour (55 % to white, the concept's mix), the name under it;
  - every card says "LEVEL n of max" with a dot per level, filled to the level taking it gives (a new card too);
  - NEW is the concept's red starburst over the band's end (`ui_icon_card_new`, 128 px, `sticker_art.write_card_pick`;
    the word is the game's string over it);
  - the lifted card (pointer or pad) has its Take on the yellow plate;
  - LEVEL n is a sticker title with "Take 1 card." in ink, on a card-stock panel; Reroll carries the Reawaken arrow.
  Grafted into `WBP_CardFace` and `WBP_CardPickScreen`. `tools/bake_widgets.py`'s `wrap:` now allows a wrapper that holds
  more than the wrapped part (each other child after with `add:`). Kept: the data-driven band colours (the concept
  coloured 3 of 7 flavours, two of them near-identical yellows) and Banish under Take inside the card.
- **The title is the concept's** (its scrTitle):
  - the Tiny Kitten and three mice on the garden (`ui_icon_title_cat` 512, `ui_icon_title_mouse` 256,
    `sticker_art.write_title`);
  - the logo centred and upright;
  - the status line in heavy ink.
  `Pawpocalypse.UI.Layout.Title` caught two clashes on the first try (the concept's lower logo over the menu at 150 % UI;
  a mouse off the screen at 1280 x 800, 150 %): the logo keeps R4c's height and the right-hand mice sit closer in. The
  concept's bottom row (labelled links beside the hint) and its status pill do not fit at 1153 units wide: the links stay
  icon plates at the bottom right, the hint bottom left, the status in the menu panel.
- The icon test lets the title's two pictures be their own size, as the ceremony's.

**Proof.** Before each graft the new asserts FAILED on the asset (card pick: the starburst, dots, tint, title panel and 7
missing parts by Parity; title: logo, cast, status, 4 images by Parity), and PASSED after (CardPick + Widgets 16; Title +
Icons + Layout + Widgets 21). Sabotages: `pick_rank_always_new` (retargeted), `pick_new_badge_never`,
`pick_dots_never_filled`, `pick_art_panel_untinted`, `pick_take_never_yellow` all CAUGHT; the title's five
(`title_logo_left` retargeted, `title_logo_tilted`, `title_cat_missing`, `title_mice_not_mirrored`, `title_status_muted`):
all CAUGHT. Everyday `check_all`: **every gate passed, exit 0** (`build/check_all_20261118_pick_title.log`). Its first run failed L0 ui-rules (the art panel's tint built with `FromSRGBColor` in the screen's code): the concept's `mix()` is now `PawTokens::MixSrgb`, emitted by `tools/gen_paw_tokens.py`.

**Open (the designer).** As in (113) and (112): Missing Art (5 hats, 7 upgrade icons and Bait, job icons A or B), the three
shop words, E1 (Fredoka titles), the store page sign-off.

**Next step.** The job picker once the hats are approved; until then the save slots (the cat at the save's stage), then
settings (Controls with keycaps and pad glyphs side by side), the other dock tabs, the boss panel, the skill tree, the
Unicorn ceremony.

---

## 2026-11-18 (113) - The store set: text version 2 checked against 11 comparable pages and the data, 20 store translations, Drive

**Where the project is.** On (112), uncommitted. The designer: "Are all store assets including store localization up to
date and in drive? Look at similar games and read their store pages. Take note of the language they use and sizes of their
gifs. Make sure that all out descriptions are good and rewady". No game code changed; no engine run.

**What happened.**
- **Drive was out of date.** `Pawpocalypse/Promotional/Steam` held only the 2026-11-03 set (old look, old text, old
  clips and GIFs). None of the Sticker Box store art, the 2026-11-18 draft or the new screenshots were there, and no store
  translation existed. Now in a new folder beside the old one (old files kept):
  https://drive.google.com/drive/folders/15rMpsxaBC7ibx56pM7VpWSKpgNicScKO
  - the 13 store images and the 9 screenshots (byte sizes match the local files);
  - the draft, the 21-language text, the Steamworks localization JSON, the research notes, a README.
- **11 comparable pages read** (`build/steam/research`: `fetch_pages.py` over Steam's public `appdetails`; `NOTES.md`).
  None serves a GIF: Steam converts them to webm + mp4. Clip widths 1170 / 780 / 616 / 400 px; 1170 px gameplay
  0.7 to 2.7 MB. 6 of 11 short descriptions open with the genre; the plainest pages name concrete things.
- **The page text is version 2** (`docs/STEAM-PAGE-DRAFT-2026-11-18.md`, its first section lists every change). Two
  version 1 claims were wrong, found by checking each against `Data/` and Godot:
  - "evolves 7 times": the cat has 7 stages and starts at the first, so it evolves 6 times (`get_evolution_stage`);
  - "each stage has its own weapon": stages 3 and 6 share RAINBOW_MINIGUN.
  Also: Stardust raises damage only (`combat_resolver.gd`); jobs are unlocked in the tree and one is picked per run;
  "21 languages" was removed from the features (it contradicted English only). The short description names geese,
  crocodiles and ghosts ("mice" stays out, VOCAB). `build/steam/text_check.py` (now cut at the next section): 0 flags.
- **Store localization drafted** (`build/steam/localization`): `store_text.py` holds the page in English and the
  game's 20 languages, each with the game's own terms (`build/steam/research/glossary.py` reads Godot's `.po` files).
  `make_store_localization.py` writes `storepage_localization.json` and `readable/` only when every check passes:
  - short description at most 280 characters;
  - the same numbers as the English;
  - the game's Reawaken and stage names, and Catnip and Stardust word stems;
  - the English is the draft's, word for word.
  `--self-test` plants 7 faults and catches all 7. On the first run the checks caught de/fr/hu short descriptions over
  280 and Japanese counters; all fixed.
- **KB** Steam/05: the measured clip sizes, how to read a webm's size from its header, and how much a short
  description grows in translation (+19-23% for de/hu/fr).

**Proof.** `py -I build/steam/localization/make_store_localization.py`: 21 languages, every check passed;
`--self-test`: 7 of 7 caught. `py build/steam/text_check.py`: 0 flagged.

**Open (the designer).** The draft's "Open" list:
- sign off version 2;
- the length line;
- English only until phase 7;
- the AI disclosure;
- the top tags;
- a native read of each translation;
- clips in the new look (needs ffmpeg, the designer's call).
The screenshots need a retake once the title, card pick and job picker are restyled. Plus everything open in (112).

**Next step.** As in (112): the card pick as the concept draws it (grafts on `WBP_CardFace`), then the title.

---

## 2026-11-18 (112) - S4: the Reawaken ceremony and the pause menu as the concept draws them; Missing Art v2

**Where the project is.** On (111), uncommitted. The designer: "Go"; then, on Missing Art: "Engineer needs to have yellow
hard hat. Change daredevil to other color". **The editor may be opened** (nothing running).

**What happened.**
- **The Reawaken ceremony is the UI concept's** (its scrCeremony):
  - the flash's picture is the concept's rays (32, #7B5FD8 / #8B6BEF), its halo and eight sparks;
  - the armoured cat (stage 2 with its sword) sits in the middle;
  - REAWAKENING is a sticker title (yellow on an ink outline, 96 px), and the payout sits on a card-stock panel below;
  - the pictures are `ui_icon_ceremony_rays` (1920 x 1080) and `ui_icon_ceremony_cat` (512, rim), from
    `sticker_art.write_ceremony`;
  - `raster_svg` honours a root's `data-raster="WxH"` (only these two; a card's `width="256"` stays a 128 raster) and
    rims a 1:1 Sticker drawing;
  - the icon test lets these two pictures be their own size;
  - grafted into `WBP_ReawakenCeremony`: `restyle:Flash; add:Cat; slot:HeadlineBox; text:Headline; slot:PayoutBox;
    restyle:PayoutBox; text:Payout`.
  `UPawReawakenCeremony::HasRays` and `IsCatShown` are asserted by `Pawpocalypse.UI.Ceremony.Plays`: seen to FAIL on the
  asset before the graft, PASS after. Timing, words and behaviour are Godot's, unchanged. Store shot 06 is no longer a white
  screen.
- **The pause menu is the concept's**:
  - PAUSED is a sticker title (`text:Title` grafted; Parity failed before it, naming the old look);
  - each main button carries its icon (Continue the Reawaken arrow, Save, Settings, Back to Main Menu), set when the layout
    is ready;
  - Continue sits on the yellow plate (`SetCallLook`). `Pawpocalypse.UI.Pause.Look` asserts it; the new sabotage
    `pause_continue_plain` was seen caught.
- **The victory's OK sits on the yellow plate** (the concept's). `Pawpocalypse.UI.Victory.Ending` asserts it; the new
  sabotage `victory_ok_plain` was seen caught. `check_all` passed again with it
  (`build/check_all_20261118_victory.log`).
- **Missing Art version 2** (https://claude.ai/artifact/J265sfqjxSbdsSNsXU3VSh): the Engineer's hard hat is yellow, the
  Daredevil's stunt helmet crimson (the designer's notes); option A's job colours follow. The other drafts await an answer.

**Proof.** Everyday `check_all`: **every gate passed, exit 0**, on the ceremony (`build/check_all_20261118_cer.log`) and again with the pause menu (`build/check_all_20261118_pause.log`). The UI tests (93) pass; the ceremony's look asserts and the pause title's parity were seen to fail before their grafts; `pause_continue_plain` was caught.

**Open (the designer).** The rest of Missing Art (5 hats, 7 upgrade icons and Bait, job icons A or B); the three shop words;
as in (110).

**Next step.** The card pick as the concept draws it
(the flavour band's colours, the tinted art panel, the red NEW starburst, the level dots, the focused card's Take on yellow:
grafts on `WBP_CardFace`). The title (Continue on yellow). The job picker after the hats are approved.

---

## 2026-11-18 (111) - The strike as the concept draws it; the dock's upgrade rows (S4); Missing Art for approval

**Where the project is.** On (110), uncommitted. The designer: "Go" (after (110)'s report). **The editor may be opened**
(nothing running).

**What happened.**
- **The strike is the UI concept's swipe** (`fx_look.py` -> `PawFxLook.h`):
  - a white arc on its ink edge, the three claw slashes in ink with no colour inside, at Godot's length;
  - no paw: D3's clutching paw is kept behind `STRIKE_PAW` (off).
  - The Vfx look test asserts it. Sabotages:
    - three new ones (the paw back, the arc in Godot's bone, colour inside the slashes);
    - the crit sabotage retargeted;
    - with the two aura ones, all 6 seen caught.
- **The dock's upgrade rows as the concept draws them** (S4's first screen):
  - each row is the upgrade's icon, its name, "Lv N" under it, and the price on its own plate at the right: the raised
    yellow when it can be paid, the off plate when not;
  - built from `UPawButton` with one new part, `PriceTag`, grafted into `WBP_Button` (`bake_widgets --graft
    "PawButton=add:PriceTag"`), and `SetIconSlotKept`, so names line up when a row has no icon;
  - a new chrome plate, the raised yellow `ui_button_primary` (sweet_ui, raster, import, `EPawChrome::ButtonPrimary`,
    the chrome test);
  - the concept's 8 upgrade icons, recorded from its shop list as `ui_icon_upgrade_<id>` (186 icons now).
  - The words: `SHOP_row_level` "Lv {level}", `SHOP_row_max_plate` "MAX" (the approved words, split) and
    `SHOP_row_level_bulk` "Lv {level}  +{levels}" (a draft), all in `docs/TEXT-DRAFT-2026-11-18.csv`. The old one-line
    `SHOP_row_bulk` and `SHOP_row_max` were removed (L0: no source named them).
  - Tests and sabotages: the HUD readouts test asserts the parts; sabotages `hud_row_says_max` (retargeted) and
    `hud_row_plate_always_yellow` (new), with two older row sabotages, all seen caught.
- **Missing Art** (https://claude.ai/artifact/J265sfqjxSbdsSNsXU3VSh; source `build/steam/page/missing-art.html`, made
  by `make_missing_art.py` from the UI concept's own code): the drawings the approved concepts lack, drafted for approval:
  - the 7 upgrade icons the concept did not draw, and the Bait icon;
  - the 7 job hats it did not draw, on the cat;
  - the job icons two ways (the concept's coloured top hats, or each job's own hat).

**Proof.** Everyday `check_all`: **every gate passed, exit 0** (`build/check_all_20261118_dock3.log`). Before it: one run failed L0 (the two keys no source named, removed: `build/check_all_20261118_dock.log`), and one proved everything but the engine gates, which it SKIPPED (`..._dock2.log`): another Claude session was running Unreal for the Cabbage project, and the guard cannot tell whose a bare `UnrealEditor-Cmd -AllowCommandletRendering` is. The third run waited 7 minutes for it to end. The UI tests (93) and the Vfx tests pass, and the 10 sabotages above were seen caught.

**Open (the designer).** Missing Art's three questions (the icons, the hats, the job icons); the three shop words in the text
draft; as in (110): the add on the Mythic floor, E1, E2, E3, the Steam page sign-off, ffmpeg.

**Next step.** On Missing Art's answer: add the approved drawings to the UI concept file (`tools/art/concept_ui_screens.html`)
and record them (`sticker_record.js`). Without it: the concept's "Buy amount" row (an x1 / x5 / x10 / Max switch), the
rest of S4 (title, pause, the boss panel, the card pick), S3's diagonal shadow, then `store_shots`.

---

## 2026-11-18 (110) - S5 landed: every character recorded from the Bestiary, its grounds, the kit's icons and cursors, the aura; Menu Screens

**Where the project is.** On (109), uncommitted (S1, S2's text half, the store tools and art). The designer, after the
review page: "The app icons and shortcut icons are great. The enemies all seem good. Create an artifact with all menu
screen and then start rewiring all new graphics to the game." Then "Continue". Nothing committed (not asked).
**The editor may be opened** (nothing running).

**What happened.**
- **Menu Screens** (https://claude.ai/artifact/2GuLR3DBNV8soFRfLDrn8K): all 25 screens in Sticker Box, the approved UI
  concept's 17 plus the 8 layers it had not drawn (the delete and quit confirms, the leaderboard, the credits, pause's
  main page and its save page, the Audio and Video settings), each in the game's own `ST_UI` words. Built from the
  concept's code (`build/steam/page/menu-screens.html`).
- **"The enemies all seem good"** read as approval of the Bestiary as drawn: S5's question answered as option (b). No
  colour moved. The rules follow the approved cast instead, as tables that only shrink (`docs/WORKLIST-STICKER`, S5).
- **Every character is RECORDED from the approved concept, not transcribed.** `node tools/art/sticker_record.js` runs
  `tools/art/concept_bestiary.html` (version 5, now in the repo) with its primitives replaced by recorders and writes
  `tools/art/sticker_cast.py` and `sticker_colours.json` (read by `palette.STICKER_CREATURES`). Fidelity: IoU
  0.996-1.000 against the concept's own render, colour under 1/255. New gate `Sticker cast is the concept`, seen to fail
  on a planted edit (through safe_mutate).
- **In the game** (`tools/art/sticker_art.py`, called by `make_sweet_art`; the rim made at packing, a disc):
  - the 12 commons, 12 signatures, the flying mouse, bat and golden mouse, the 12 gristles, the 5 bosses and the add;
  - the cat's 7 stages, fitted in two shared frames; the weapons, cut out of the concept's figures; the kitten.
    **The hat and weapon constants in `PawFieldLook.h` are worked out from the new cat**, and the art build stops when
    they differ. The weapon is drawn facing the cat's way, so `WeaponFlipX` now mirrors with the cat (test and
    sabotage updated). The ten job hats are still the grammar's, placed on the new head.
  - **the floors are the Bestiary's grounds** (`palette.STICKER_GROUNDS`), with `PcBackdropTones.h`, `PcGodotLook.h`
    and the 12 floor textures. Six prop colours moved to clear their new floors.
  - the rig layers are gone with the old drawings (E3 open). The atlas is 3 pages; `T_Atlas_Field_3` removed. Two rig
    assertions became "no Sticker drawing carries rig layers" (`PawLookTests`, `PawRealmTests`).
  - **the UI concept's kit icons over 18 of the game's** (tabs, quick bar, menu links, Catnip, Stardust, tick, close,
    lock) and **the paw cursors** (open, held with arc toes), recorded from `concept_ui_screens.html`.
  - **the aura as the concept draws it** (`fx_look.py`): a yellow ring at 85 % over a 16 % fill, no ink edge; crits in
    Sticker's yellow. The Vfx look test follows.
- **look_lint's Sticker rules**, with every self-test plant caught:
  - per-shape ink widths;
  - the concept's colours and their mix();
  - eyes measured at the size the game draws each drawing (floor 27 px, the bat's);
  - "no shade used twice" between enemies of one realm (`UNIQUE_APPROVED`: six approved pairs);
  - bodies near their floors as approved (`APPROVED`);
  - the cyber visor's see-through fill allowed.
  Palette rules 3 and 4 leave the creatures to the drawn check. A lint bug was fixed: an eye whose pupil is drawn a few
  shapes later (the cat's, the bosses') was not seen.
- **Store screenshots retaken** with the new field (`py tools/store_shots.py`). The boss scene's cat moved off the
  Tyrant. The review page is updated (version 3): https://claude.ai/artifact/CAk7aojRbcMP17i32JpdGm
- `make_sweet_art.check_tokens` no longer checks the biome tokens it does not draw. `pack_atlas` reads a rig only from a
  layer with `data-rig-of`.
- Knowledge library: 2D/01 §13 (porting a JS concept into a generator by recording it; die-cut rims; overlays on shared
  fits); INDEX and the 2D index.

**Proof.** Everyday `check_all`: **every gate passed, exit 0** (`build/check_all_20261118_s5b.log`), the new gate included. The first run (`build/check_all_20261118_s5.log`) failed one gate, L3 Realm: the two rig assertions above, since restated. look_lint and its self-test pass, every plant caught. The new recording gate failed on a planted edit, and `safe_mutate` reports nothing left in place.

**Open (the designer).**
1. Worth a look in the shots: the add is lilac on the lilac Mythic floor (5 apart), the Litter King close to the
   Litterbox (7), the standing cat on the Alley (11). Each approved as drawn; any can be moved on a word.
2. Seven of the ten job hats are not in the concept: draw them there first?
3. E1 (Fredoka titles), E2 (a disc rim, as now, or the concept's square), E3 (wings and legs that move).
4. The Steam page sign-off and ffmpeg, as in (109).

**Next step.** Without a decision: S4's screens as grafts, starting with the dock (each upgrade row with its icon and a
yellow price button, as the concept draws it), then S3's diagonal shadow (the brushes' margins, in the assets), S6's
card art and skill icons, the strike (the concept's arc with three slashes), the props. Then `store_shots` again.

---

## 2026-11-18 (109) - The night: S1 and the text half of S2 in the game; the Steam page drafted; store screenshots scripted

**Where the project is.** On (108), committed and pushed as `1a10eeb` before the night. Since then the designer said:
- "All the enemies have tear drops. Remove the tear drops";
- "Create an execute an overnight plan. We don't need a long nightly run. Our focus is steam page and screenshots so all
  of those should be priority".

Uncommitted: S1 (the Sticker UI colours), the text half of S2 (Nunito), the store shot tool, the docs. **The editor may be
opened** (nothing is running; the last build is the editor target with everything below).

**What happened.**
- **The nightly `-Full` was stopped at 23:31** at the designer's word. 30 gates had passed, none failed. It was inside the
  PawCore sabotage gate; its planted fault (`PcField.cpp`, `hammerfall_one_extra`) was restored by `safe_mutate.py`.
- **Desk work before the new plan** (`docs/WORKLIST-NIGHT-2026-11-17.md`):
  - S1 prepared and pre-flighted (`build/night3/s1/`);
  - E1 measured: Nunito carries every player language; Fredoka has basic Latin only and proportional digits
    (`build/night3/fonts/`);
  - S5 batch 1 drawn: the 12 commons transcribed from the Bestiary, IoU 0.993-0.998 (`build/night3/art/`).
- **No tear drops.** Removed from:
  - the Bestiary and UI Screens concepts, both now version 5 (18 drops, plus the robot mouse's oil drip in the same spot);
  - batch 1's generators, along with the `drop()` helper itself.
  The look spec and the memory say so.
- **S1 in the game** (`docs/WORKLIST-NIGHT-2026-11-17-STEAM.md` step 1):
  - The patch applied; tokens, tree tables, effects, art, atlas and imports regenerated.
  - The Widget Blueprints recoloured by rule; the inventory again: no retired colour left in 7,549.
  - Found and fixed:
    - **(a)** The HUD's XP numbers (XpNow, XpNext) were saved field labels. The recolour made them ink; a `text:` graft
      restored them (Widgets.Parity).
    - **(b)** The recolour rewrites the widgets' palette snapshot, so the retired list must be regenerated afterwards
      (L0). The recipe now says so.
    - **(c)** That rewrite also dropped plum from the retired list. `tools/art/palette_plum_snapshot.json` keeps it, so
      `Pawpocalypse.UI.Widgets.Palette` still catches a slip back to plum.
    - **(d)** The run moved 287 colours where the pre-flight said 412. The other 125 are colours equal to a C++ class
      default: never saved in the asset, they had already followed the new tokens. The combo box rows came out pressed,
      as their constructor says.
- **S2's text half** (E1's measured recommendation; one step to undo):
  - F_Paw is Nunito 700 / 900, instanced with real names.
  - Japanese stays on M PLUS ExtraBold; the pseudolocale xa keeps M PLUS for Latin Extended.
  - Coverage PASS in 22 languages; self-test seen to fail.
  - F_PawTitle (Fredoka) waits for E1's answer.
- **The Steam page** (`docs/STEAM-PAGE-DRAFT-2026-11-18.md`; review page https://claude.ai/artifact/CAk7aojRbcMP17i32JpdGm):
  - **The words:** short description (198 characters), About, features, tags, languages, the AI disclosure. Every number
    is from `Data/`, and the old draft's 16 enemies, 41 achievements and 6-10 hours are corrected. All of it passes the
    game's own text rules.
  - **13 images** at Steam's exact sizes, made by the concepts' own drawing code (`build/steam/art/make_store_art.js`,
    `render.py`).
  - The logo reads at 120x45; the hero sits inside the never-covered box; the library capsule is clear of the covered
    strips.
- **The store screenshots, scripted** (`tools/store_shots.py`; `paw.Do` added to the shot queue so a cheat runs in order):
  nine scenes at 1920x1080, saves sandboxed. **All nine are right**, after five runs that each found a cause:
  - every level up deals a hand that holds the game, so the scenes clear them;
  - a renamed scene left its old file, so the tool now clears stale shots;
  - the unlock cheat marks every boss beaten, so the boss scene comes first;
  - **`paw.Boss` had topped up the wrong Bait counter since D17.** It banked `TierKillCount`, but the Bait is
    `BaitKillsByBiome`, so it never summoned; fixed in `PawShot.cpp` (development builds only).
  The shots show the new UI over the old field: progress shots, not the store set. They are on the review page.
- **S5's pipeline built, not landed** (`build/night3/s5/`): Sticker drawings in `make_sweet_art`, the rim in `pack_atlas`,
  `look_lint`'s Sticker rules.
  **Blocked by the designer's "no shade used twice" (2026-11-09):** the approved Bestiary breaks it in 57 pairs, three of
  them identical colours (`build/night3/s5/unique_in_bestiary.txt`).
- Knowledge library:
  - U/09: migrating a folded palette by path rules, and the class-default trap; font instancing names; tabular digits;
    title fonts with few letters; font sheets;
  - S/05: Pillow's RGBA draw.

**Proof.**
- Everyday `check_all`: **every gate passed, exit 0**, on S1 (`build/check_all_20261118_s1b.log`) and again on S1 + S2
  (`build/check_all_20261118_s2.log`).
- The first S1 run failed two gates (L0, L3 UI), both fixed above (`build/check_all_20261118_s1.log`).
- After the `paw.Boss` fix, the final tree once more: **every gate passed, exit 0**
  (`build/check_all_20261118_final.log`).

**Open (the designer).**
1. **The field (S5):** move the Bestiary's clashing colours apart, or keep "no shade used twice" only within a realm?
   Then the rats' and pigeon's eyes, and the hamster on the Vet floor.
2. E1: Fredoka titles (F_PawTitle)?
3. E2: a disc or a square rim?
4. The Steam page: sign off the words; the length line; English only until phase 7; the AI disclosure; the top five tags;
   the capsule scene.
5. A screen recorder (ffmpeg) for the About section's clips?
6. As before: E3-E5, Q13, playing D17, Q11, Q10.

**Next step.** The designer's answer on S5's colour rule. Then:
1. `py -I build/night3/s5/s5_apply.py .`, plus the colour or rule change;
2. `build_art`, `pack_atlas`, `import_art --run`;
3. `check_all`;
4. `py tools/store_shots.py`, the store set in the new look.

---

## 2026-11-17 (108) - S1 step 1: the colour inventory of every Widget Blueprint

**Where the project is.** On (107). The designer: "Continue". Nothing committed since (103); (104)-(108) uncommitted. **The
editor may be opened** (nothing is running).

**What happened.**
- **`UPawWidgetBaker::ColourReport`** (editor module only) and **`py tools/bake_widgets.py --run --colours`**: every colour
  every baked asset saves, over the migrations' own objects (the editable tree's widgets and the default object) and with
  `PawUi::ForEachColourRole`'s rules (transient skipped, the outermost text wins, a brush's outline an edge), one row each with
  its property path, to `build/widget_colours.tsv`. Read-only: it saves nothing.
- **The inventory: 7,549 saved colours in 32 assets.** Grouped by the palette the assets carry
  (`tools/art/palette_widgets_snapshot.json`), it shows where plum folded roles that Sticker Box splits: Deep Plum is
  outlines (51), the dropdown's item brushes, text shadows and pressed tints (48); Plum is panels and also every font outline
  and the slider handles; Bone is text and also the styles' foreground colours; Butter is the hover and active outlines (41)
  and some text. The table and the map it leads to are in the worklist's S1.
- **Decided for S1 (worklist):** the recolour goes by RULE, a short list of (role, property path suffix, old key, new key)
  first and the role maps after, with a pre-flight against the inventory (every changing colour placed exactly once, or no
  save) and the inventory run again after. Also listed there: the text keys that join `UI_TEXT_KEYS`, the keys painted in
  code, the two "amount" pairs that leave rule 7, the C++ naming plum tokens (`PawShot.cpp`, `PawUiKit.cpp`), the tests
  pinning the plum spec (`PawChromeTests` ring = selected plate = Butter).
- Docs: the worklist (S1's findings and map), CLAUDE.md (`--colours`).

**Proof.** The editor target built (UBT, 141 s). The report ran headless and wrote 7,549 rows. **The everyday `check_all`: 28 of 28 PASS, exit 0**
(`build/check_all_20261117_colours.log`). Committed and pushed with (104)-(107) at the designer's request ("Commit and push. Go").

**Open.** As (107): E1-E5, D17's nightly, Q13, playing D17, Q11, Q10. Uncommitted: (104)-(108).

**Next step.** S1 step 2, editor closed: the Sticker UI table in `tools/art/palette.py` (the worklist's values and pairs);
`gen_paw_tokens.py`'s rules and pre-flight; `UPawWidgetBaker::RecolourByRule`; the plum-pinning tests and the two C++ files;
then `gen_paw_tokens`, `build_art`, `raster_svg`, `pack_atlas`, `import_art --run`, `bake_widgets --run --recolour`, the
inventory again, `check_all`.

---

## 2026-11-17 (107) - D18: the Sticker Box look replaces plum; concepts approved; the restyle planned (S0)

**Where the project is.** On (106), nothing committed since (103); (104)-(106) still uncommitted and untouched here. D17's
nightly, Q13 and playing D17 are still owed. **The editor may be opened** (nothing is running).

**What happened.**
- **The designer chose a new look.** On three fresh directions drawn as a concept page: "You don't have to follow the current
  color or palette or ui. Just start fresh", then "Sticker box looks great", and the order of work: every enemy redrawn, every
  UI screen as a concept, then Unreal, then the Steam page's assets and descriptions.
- **The concepts were drawn and approved before any Unreal change** (claude.ai artifacts; their HTML draws every shape in
  code and is readable with the Artifact tool):
  - Look: https://claude.ai/artifact/CCUYB9dcTRyt1d5X1dtrMT (version 1, "Sticker Box").
  - **Bestiary** (approved at version 4, "Everything else good"): https://claude.ai/artifact/B92ezCRMKpDeXShegRWVV8. All 12
    biomes at game proportions (sizes from `sprite_target_px`), the gristle at all 12 tiers, the 5 bosses and the add. The
    species follow the ART FILES, not the enemy ids (`sewer_dog` is a crocodile, `sewer_squirrel` a rat, `turf_squirrel` a
    slime rat, `mythic_squirrel` an imp, `rooftop_goose` a seagull, `portal_goose` a hound). The designer's fixes: the gristle's
    anatomy (now high shoulders, a small sunk head, knuckles on the ground, low hips), a chunkier crocodile with its tail
    curled up, the parrot without a cone, the Rival Champion without the scar line (it read as a weapon) and with a gold medal
    on a red ribbon (a collar read wrong twice).
  - **UI Screens** (approved at version 4, "Good continue"): https://claude.ai/artifact/Homk9frz3jpvik8LYjpmA4. The kit and 17
    screens in the approved `ST_UI` words and the real bindings. The held paw cursor: the open paw's pad with single ink arcs
    for curled toes (two redraws). Open items listed on its page: locked Realms rows should name what opens them (a new string),
    the job picker's lines and the tutorial line are drafts, Boba's photo.
  - Seen on the way, for the designer: signature enemies are 95 px in biomes 1-8 and 72-77 px from the Kitchen on
    (`sprite_target_px`); golden mice stop sparkling from the Kitchen on. Both are the data as it is.
- **Recorded:** D18 in doc 01; CLAUDE.md and doc 00 say Sticker Box; the plan is `docs/WORKLIST-STICKER-2026-11-17.md` (the
  look spec, S0-S7, open decisions E1 fonts, E2 the rim, E3 the rig, E4 layout, E5 the draft lines). No code, data, art or
  asset changed.
- **Found before S1 started:** the Widget Blueprints' value recolour cannot express the new palette (plum folded the panel and
  the ink into one colour, and the frames, shadow, pressed state and key well into another; Sticker Box splits them). The
  approach is in the worklist's S1: an inventory of the colours the assets save first, then the map, then part-by-part
  restyles from code for what stays ambiguous.

**Open.** E1-E5 (the worklist's recommendations stand unless the designer says otherwise). D17's nightly, Q13, playing D17,
Q11, Q10, fix 5 (as (106)). Uncommitted: (104)-(107).

**Next step.** S1 step 1, with the editor closed: a `UPawWidgetBaker` colour report (`bake_widgets.py --run --colours`) listing
every colour every baked asset saves, by widget, property and role; then the Sticker Box UI table in `tools/art/palette.py`
from that inventory (worklist S1).

---

## 2026-11-13 (106) - Every engine guard waits, not only the test runner

**Where the project is.** On (105), nothing committed since (103). The designer asked "Anything left to do?"; one thing was:
(105) fixed the single-look race in `run_automation.py` only. Left for the designer as in (105): Q13, playing D17, D17's
nightly, the commit. **The editor may be opened** (nothing is running). Uncommitted: (104), (105) and this.

**What happened.**
- **The same race was in every other guard.** Each asked `unreal_procs.blocking()` once and refused (or SKIPPED) on what that
  one look saw: check_all's SKIP check before each engine gate, `sabotage_realm.py` at its start (at night it starts the
  moment L4's `-game` window returns: a blip there refuses the whole realm gate, the night's longest), `import_data.py`,
  `padplay_tour.py`, and the import/bake tools (`bake_widgets`, `create_input_assets`, `import_art`, `import_audio`,
  `import_fonts`).
- **The wait moved into the guard.** `wait_until_clear` and `guard_self_test` now live in `tools/unreal_procs.py`, with
  `wait_blocking()` (the real process list, the real clock) and `print_blocking()`; `py tools/unreal_procs.py --wait` is the
  same from the command line. Every caller above asks `wait_blocking()` and prints what blocked; check_all's
  `Test-UnrealBlocking` runs `--wait` and its SKIP line now lists the blocking processes. `run_automation.py` imports them
  (its `--self-test` still runs the wait's test). The cost when an editor IS open: about 30 s per engine gate before its SKIP.
- **Proof.** `unreal_procs.py --self-test` and `run_automation.py --self-test` PASS; the old loop and two other wrong waits,
  planted in scratch copies of the moved code, each FAIL it (3 of 3). End to end, a real process named
  `UnrealEditorGuardProbe.exe` (a renamed `ping.exe` in the scratchpad, naming no project, so it blocks): check_all's own
  `Test-UnrealBlocking` (taken from the file as it stands), `sabotage_realm.py` and `import_data.py --check` each waited about
  30 s and refused naming its pid (exit 2); a probe that left after 6 s was waited for and the guard went ahead (exit 0, 8 s).
  Both sabotage `--list` checks exit 0. **The everyday `check_all`: 28 of 28 PASS, exit 0**
  (`build/check_all_20261113_guard.log`). Not run: `padplay_tour.py` and the import/bake tools themselves (a one-line change
  each, compiled; the nightly runs L4).
- Docs: 09 (the guard), 00 (unreal_procs' row), CLAUDE.md (`run_automation.py --self-test`). KB: U/18 §2 (put the wait in the
  shared guard and make every caller use it; a renamed `ping.exe` as a test process), INDEX (the symptom row).

**Open.** As (105): D17's nightly (it now also runs the waited guard before every engine gate and the realm sabotage), Q13,
playing D17, Q11, Q10, R6, fix 5 (recommended: leave). Uncommitted: (104), (105), (106).

**Next step.** The designer plays D17 and answers Q13. Then, with the editor closed, the nightly `-Full` (~10 h). Commit when
the designer says.

---

## 2026-11-13 (105) - The `if` audit's fixes 1-4 made and proven (no rule, number, word or look changed); the engine guard's race fixed

**Where the project is.** On (104), nothing committed since (103). After the dev list the designer said "Don't do the long
run yet. Do the rest of things you can do". Done: the audit's fixes 1-3 (as planned in `WORKLIST-NIGHT-2026-11-12.md`, "The
fixes") and fix 4; not fix 5 (below). Left for the designer: Q13 (the tree's prices), playing D17, D17's nightly (which now
proves the fixes too), the commit. **The editor may be opened** (from 11:04; nothing is running). Uncommitted: (104)'s work
and this.

**What happened.**
- **Fix 1, Overkill tested once** (`PcField.cpp`). The six `if (R.Overkill > 0.0)` around `SplashOverkill` are gone (the
  audit counted 7; the code has 6). `SplashOverkill`'s own guard is now `if (!(Damage > 0.0)) return;`, so a NaN still stops
  there as it did at the callers. `OverkillDamage` keeps its `<= 0`: it is Godot's `overkill_damage`, with its own fixture.
  **A sabotage of the kept guard was MISSED** (`splash_without_overkill`, `if (Damage < 0.0)`). No test said that a blow
  that does not kill splashes nothing: the callers' guards had made it true. New L1 test "Field: Overkill splashes only a
  kill's excess" covers a blow that leaves its mouse alive and a kill with no Overkill bought; with it the sabotage is
  CAUGHT. `pounce_splash_from_parking_spot` retargeted (one tab less).
- **Fix 2, the HUD checks once** (`PawHudScreen`). `RefreshNow` looks the realm and the game up once and refreshes only when
  the game's data is ready. The nine refreshes take `const UPawRealmSubsystem&` / `const UPawGameSubsystem&`: their nine
  lookups and checks are gone, and so is `RefreshRealms`' second `Game()` lookup for the auto-travel box. `WriteNavigation`
  still runs on every refresh. The job picker's answer refreshes Jobs with the game that `ReturnToStartingBiome` checked,
  captured in the lambda (the game subsystem outlives the HUD). Realm sabotages retargeted: `jobpick_answer_never_travels`,
  `bait_lean_restarts_f42`.
- **Fix 3, a card's requirement is an enum** (`PcCards`, the HUD). `Pc::PcCardRequirement` holds None, Aura, Pounce,
  Overkill, Poop, Kitten, KittenPounce and Count. `CardEffectRequirement` / `CardCostRequirement` return it;
  `CardRequirementMet` and `UPawHudScreen::CardRequirementKey` (now public and static, for its test) switch on it with no
  `default`; `PcCardHint::Requirement` carries it. The plan's `CardRequirementName` was not needed: no fixture compares the
  strings. A misspelt requirement no longer compiles. A forgotten case falls to the old fallback ("met" / "other"), and
  since MSVC's C4062 is off, two tests now fail on it: L1 "Cards: each requirement is met by its own unlock and no other"
  (8 ownership cases, every value) and `Pawpocalypse.UI.Hud.Catches` (every value but None has its own key, present in
  ST_UI, no other value's). New sabotages, CAUGHT: `card_requirement_forgotten`, `card_requirement_crossed` (PawCore),
  `hud_catches_hint_forgotten` (realm). `catches_hint_cost_first` retargeted. `catches_requirement_unnamed` wrote the
  string `"aura"` into the field and no longer compiled, which the stale-target check cannot see (its target was still
  there): rewritten as `PcCardRequirement::Aura`, CAUGHT.
- **Fix 4, the multiplier nodes through one helper** (`PcStats`). `BranchFactor(D, P, Id, Channel)` gives the branch's
  effect once a level is owned and x1 before. `RebuildStats`' five guarded lines use it (Aura Reach, Pounce, Claws' "mult",
  the Unicorn's D17 step, Rainbow Gut); the results are bit-identical (x1.0 is exact). New sabotages
  `branch_factor_zero_unowned` and `branch_factor_unguarded` CAUGHT; `unicorn_no_damage_step` and
  `unicorn_step_reaches_kittens` retargeted and CAUGHT.
- **Fix 5 not made** (one shared "move the focus back" helper, one shared `Realm()` / `Game()` lookup). The copies differ in
  their conditions and targets, the gain is about ten one-line bodies, and it would move many realm sabotages' targets.
  Recommendation: leave it.
- **The engine guard's race, found and fixed.** The first `check_all` on the end state FAILED one gate: `L3 Pawpocalypse.UI`
  refused in 1 s at 10:53:50, the second the Realm gate ended ("an Unreal editor is running"), naming nothing. The cause:
  `run_automation.py`'s wait looped `while editor_running(): sleep`, then asked once more and refused at once on that second
  look, so a process that appeared between the two refused the gate without the 30 s wait. The cut nightly's Audio refusal
  (22:31:40, between two L3 gates) has the same signature. What blocked is unknown: by 10:55 the only Unreal process was
  another project's network test (Cabbage, which does not block by design). Now `wait_until_clear`: anything seen is waited
  for, the way is clear after two clean checks in a row, and the refusal prints each blocking process (pid, name, command
  line). `guard_self_test` (5 cases on stubbed process lists and clock) runs inside `py tools/unreal_procs.py --self-test` (the
  nightly's Unreal guard self-test, so no new gate); the old loop and two other wrong waits planted in scratch copies each
  FAIL it.
- **Proof.** L1 206 of 206, 6.70 M assertions (+2 tests). PawCore sabotages: 667 (+5); CAUGHT: every new or retargeted one,
  and every one on the changed code (the splash's 4, the cards' 11, PcStats' 13). Realm sabotages: 564 (+1); CAUGHT: all 36
  inside fix 2's functions and the 4 new or retargeted (`build/sabotage_realm_fix2_20261113.log`, 19 min).
  `Pawpocalypse.UI.Hud.*` and `.JobPicker.*` 16 of 16. **The everyday `check_all`: 28 of 28 PASS, exit 0**, on fixes 1-3
  (`build/check_all_20261113_audit_fixes.log`) and again on the end state with the guard fixed
  (`build/check_all_20261113_fix4.log`). Not run:
  the whole sabotage sets, L4, the long sweeps; the nightly runs them.
- Docs: 00, 09 (L1, the sabotage counts, the run, the guard), CLAUDE.md (`run_automation.py --self-test`), the night's
  worklist (the fixes as made). KB: U/18 §2 (a process guard with one check per pass that names what blocked), U/23 §5 (mutation-test the
  guard left after folding repeated ones; a string-to-enum change MSVC does not check for missing cases; a `false &&` mutant
  does not compile), U/18 §8 (after a type change, run every mutant in the changed files: one can stop compiling with its
  target intact), INDEX (two keyword rows, three symptom rows), U/00.

**Open.** D17's nightly, which now proves fixes 1-4 too (667 PawCore and 564 realm sabotages). Q13. Playing D17. Q11, Q10.
R6, after R5's last reviews. Fix 5 (recommended: leave). Uncommitted: (104) and (105).

**Next step.** The designer plays D17 and answers Q13. Then, with the editor closed, the nightly `-Full` (~10 h). Commit when
the designer says.

## 2026-11-13 (104) - The night cut by a power failure; one source zeroed and restored; planted faults now journaled

**Where the project is.** On (103): commit `4f015ee`, pushed. The night of `WORKLIST-NIGHT-2026-11-12.md` (go 22:17, "Go,
no fixes tonight") ended in a power failure at about 01:04. **D17 still has no nightly.** The desk work had finished before it
(the tree price pass: ECONOMY-REVIEW §9, doc 01 Q13). This morning the designer asked "The pc lost power last night. Was
anything lost?", then "make the sabotage power-safe". **The editor may be opened.** Uncommitted: the night's three docs,
`tools/safe_mutate.py` and its three callers, `check_all.ps1`, docs 00 and 09, CLAUDE.md, the worklist closed.

**What happened.**
- **The power failure** (System log: Kernel-Power 41; the last file writes at 01:04:28). The nightly, started 22:26:39 on
  `4f015ee`, was 2 h 38 min in, in `PawCore sabotage`: 352 of 662 CAUGHT, none missed. Before it, 26 of the 27 everyday gates
  and the 4 self-tests PASS. `L3 Pawpocalypse.Audio` failed in 1 s without running ("an Unreal editor is running; close it
  first" at 22:31:40, between two L3 gates that ran; the cause was not kept). It passes today (below).
- **What was lost: one file.** The sabotage in place (one of the Achievements page's, written 01:04:18) held
  `PcHud.cpp`'s original only in memory, restored in `finally`. After the reboot the file was 21,009 bytes of 0x00, the
  mutant's size: NTFS had kept the new size, not the unflushed data. The tree had been clean at 22:18
  (`build/night2/status_before_port.txt`), so the designer restored the file from the commit; it equals `HEAD`. Nothing
  else: `git fsck` clean here and in the Godot repo, the Godot repo's status unchanged since 22:18, no other zeroed file
  among the ~5,500 changed since noon (this project, the Godot repo, the KB, the other projects under `C:\Games`), and the
  night's docs intact.
- **Planted faults journaled: `tools/safe_mutate.py`.** `mutated(path, bytes, owner)` writes the original to
  `build/mutation_journal` and flushes it, then an entry (path, owner, pid and its creation time, both versions' SHA-256);
  the mutant and the restore each arrive as a flushed file renamed over the target, so the file is always one whole
  version. A run that dies leaves its entry; `recover()` puts the original back over the mutant, zeros, an empty or a
  missing file, and refuses (exit 2) a file changed since, a damaged entry or a run still alive. Callers:
  `sabotage_pawcore.py`, `sabotage_realm.py` (each recovers first) and `check_pawcore_static.py --self-test` (its 8 plants in
  generated files). The other self-tests already plant in scratch copies. `check_all`: `no mutant left in place`
  (everyday, first gate; 28 gates) and `safe-mutate self-test` (nightly; 42 gates).
- **Proof.** The self-test 14 of 14; 10 faults planted in a copy of the tool each fail it (among them an in-place write,
  which the self-test sees by the file id staying the same). Real runs through it: 2 PawCore sabotages and 1 realm sabotage
  CAUGHT with the tree unchanged after; a PawCore sabotage run killed mid-build (`taskkill /T /F`) left the mutant and its
  entry, and `py tools/safe_mutate.py` restored it (`git diff` empty); L0's self-test PASS; **the everyday `check_all` 28 of
  28 PASS, exit 0** (`build/check_all_20261113_safe_mutate.log`).
- KB: U/18 §8 (an in-place mutation run and a power cut: what it leaves, the journal that prevents it), INDEX (a keyword
  and a symptom row), U/00. Docs: 09 §7 (the cut nightly, the journal, the gate rows and counts), 00, CLAUDE.md (the tool and
  the rule: a planted fault goes through `safe_mutate.mutated()`), the worklist closed.

**Open.** D17's nightly. Everything open in (103): the audit's fixes 1-3 (4-5 later), Q13 (the tree's prices), playing D17;
Q11; R6. The night's docs and this change are uncommitted.

**Next step.** The designer plays D17 (the editor may be opened) and answers Q13 and the audit's fixes 1-3. Then, with the
editor closed, the nightly `-Full` (~10 h), after fixes 1-3 if they go in so that it proves them too. Commit when the
designer says.

## 2026-11-12 (103) - The `if` audit (a report; no code changed); docs brought up to date; committed and pushed; the night planned

**Where the project is.** On (102). The designer kept the approved Reawaken blurb ("Keep blurb"), asked "i've heard that
sometime claude tends to overuse ifs in code. Can you audit for excessive if use?", then "Make sure all docs are up to date.
Commit and push. Create an overnight plan". **(100)-(103) are committed and pushed** (one commit, explicit paths).
**Tonight's plan: `docs/WORKLIST-NIGHT-2026-11-12.md`, waiting for the go** (the nightly on D17, which it has not had; a
tree price pass measured as desk work; the audit's fixes 1-3 only if the designer says yes at the go).

**What happened.**
- **The audit** (report only). Without tests and generated files, the three modules hold 2,696 `if`s in 47,697 lines (5.7
  per 100), over 2,297 functions: a median of 0 per function, 3 at the 90th percentile, 11 at the 99th; 3 functions nest 4
  deep. Tests: 1,261 in 32,126 lines. `tools/`: 4.1 per 100 lines, 2 nested pairs that could merge. clang-tidy (Visual
  Studio's LLVM 19.1.5) on PawCore with the branch checks found no needless `return true` / `return false` and no `else`
  after `return`; it lists 10 conditions it would rewrite by De Morgan, 4 runs of identical cases in
  `CardEffectRequirement`'s switch, and 10 functions over cognitive complexity 40 (`CheckTiers` 83, `BuildBackdrop` 76, the
  generated `FromName` 76, `PcJson`'s `Value` 54, `BindingApply` 51; the rest 41-44). The scripts were one-off (scratch); the
  method is in KB U/23 §5.
- **Worth fixing** (the designer's call; none changes a rule, number, word or look):
  1. Overkill tested three times on one path: the 7 `if (... Overkill > 0.0)` around `SplashOverkill` (`PcField.cpp`),
     then `OverkillDamage`'s `<= 0`, then `SplashOverkill`'s `Damage <= 0`. Keep one, written `!(Damage > 0.0)` so a NaN
     still stops.
  2. The HUD's nine refreshes each look the game up and test that its data is ready, while only `RefreshNow` calls them
     (and `RefreshJobs` once more, from a place that has checked).
  3. A card's requirement is a string made from an enum, compared in two `if` chains (`CardRequirementMet`,
     `PcCards.cpp:128`; `CardRequirementKey`, `PawHudScreen.cpp:106`): a typo compiles and means "always met" / "other".
     An enum and a switch: a typo no longer compiles; a test covers every value (MSVC's C4062 is off by default).
  4. `RebuildStats`' five `if (BranchLevel(P, X) > 0) S.Y *= BranchEffect(...)` (D17's Unicorn line among them): one helper
     that gives x1 at level 0.
  5. The "move the focus back" snippet (7 copies in 4 screens) and the per-screen `Realm()` / `Game()` lookups (about 10
     classes): one shared helper each.
  Fine as they are: the validator's one `if` per rule; the generated `FromName` chains; the `FKey` chains (no switch on an
  `FKey`); the nested pointer checks over engine objects (35 flagged as "collapsible", 2 were plain conditions); the rules
  keyed on one mechanic's id (Godot's shape); the dev runner `PawShot` (52 `if`s; a switch would read better).
- **Docs up to date**: doc 01 Q12 now gives the built numbers (112 min, not the proposal's 155) and the open item; doc 05
  the Reawaken page's Catnip multiplier line; doc 06 the economy before and after D17 with and without waves; doc 00;
  this log (the blurb kept). Appendices B and C made again into scratch: identical to the committed ones. KB: U/01 §2.3.1
  (clang-tidy on an engine-free module without a compile database), U/23 §5 (auditing agent-written code for redundant
  branches; the NaN trap), both indexes.

**Open.** The audit's fixes (1-3 a choice at tonight's go; 4-5 later). Everything open in (102) and (101). The text audit
still reports the kept blurb over budget (a report, not a gate).

**Next step.** The designer's go for `docs/WORKLIST-NIGHT-2026-11-12.md`: close the editor, and say yes or no to fixes 1-3.
The night runs it; its closing entry is (104).

## 2026-11-12 (102) - D17 built: the economy (a beaten boss stays beaten, Bait kept, the Reawaken pays Catnip, the Unicorn x1.5)

**Where the project is.** On (101), nothing committed since (99) (the designer has not said to commit). The designer read the
economy review and answered: "Unicorn should be a damage step. Make the changes for shorter denser game"; then the Unicorn
x1.5 (recommended) and "Use these drafts" for the words. **D17 is built** (doc 01 D17, Q12 decided). `check_all` 27 of 27
green on the end state, exit 0 (`build/check_all_d17.log`). The Godot repo is untouched (the gate says so).

**What happened.**
- **The rules (PawCore, and Godot's copy for the fixtures only, `tools/godot_fixes.py` E1-E6, EU: 22 new entries):**
  E1 a Reawaken no longer clears `BossesClearedThisCycle` (a beaten boss stays beaten); E2 the Bait bank
  `BaitKillsByBiome` (every kill, kept by a Reawaken, cleared by a new game; `BaitEarned` reads it; `BaitSpentByBiome` kept
  too); E3 `KillCatnip` multiplies by `ReawakenCatnipMultiplier` = the damage multiplier ^ `reawaken_catnip_exponent` (0.5,
  a new ReawakenConstantsData field; x1.37 a Reawaken), plus `ProjectedReawakenCatnipMultiplier` for the Reawaken tab; EU
  `RebuildStats` multiplies the multiplicative damage bracket by `unlock_unicorn`'s effect (1.5, effect type now
  DAMAGE_MULTIPLICATIVE: its tooltip line is the existing "x{v} damage"). Validator rule 58, `reawaken.catnip_exponent` in
  [0, 1) (at 1 the Catnip feeds the Stardust feeds the multiplier: measured runs of 1-9 min).
- **The data** (`Data/` re-exported through the fixes; `import_data` + `--check` PASS): Kitten Power x1.6 / growth 3.6372,
  Kitten Count +2 / 1.35 (E4; the exponent budget 0.794 of 0.80); the Overkill unlock 0.1 (E5); boss HP x2.37 / x2.04 /
  x15.8 / x31 / x139 (E6), fitted by fixed point so the FIRST fight at the moment its Bait is ready lasts 89 / 148 / 171 /
  171 / 169 s (`tools/econ_review.py` now records it). The exporter and `gen_pc_data.py` read Godot's scripts through the
  fixes too (E3's field is an @export); `PcDataRows.h` and `PawDataRows.h` regenerated.
- **The save**: container format 2 with `bait_kills`; on load a boss on the ledger has its passage open, and a file with no
  Bait bank takes its kills this run as its Bait (Godot's fixture side does the same). Test: a stored format-1 file.
- **The Reawaken tab**: a new line "Catnip multiplier x{now} -> x{after}" (`CatnipMultiplierText`, grafted into
  `WBP_HudScreen` with `bake_widgets --graft "PawHudScreen=add:CatnipMultiplierText"`), and the approved blurb. Text:
  `ST_UI` (blurb changed, one key new), `Data/TextOverrides.csv` (the blurb's row; the Overkill unlock "10%"; the Unicorn's own
  effect-line override removed), `ST_Tree` regenerated; drafts in `docs/TEXT-DRAFT-2026-11-12.csv`. **text_audit reports the
  approved blurb over budget (161 chars, 4 sentences, > 120 / 3)**: a report, not a gate; raised with the designer.
- **pawsim taught D17** (rule 14): E1, E2, E3 in `catnip_per_sec`, the Unicorn in `_damage_scale`; the mirror compares the
  Catnip multiplier (new `pawcore_sim` column) and its self-test sabotages the exponent (12 of 12 caught). `econ_review.py`:
  `today` is D17 (112 min, 13 Reawakens, waits over 3 min 4 %, the longest 4.0 min, nothing new at most 14 min) and
  `--proposal before_d17` reproduces the review's old column exactly (406 min). Plain pawsim (no waves): 163 min, 13
  Reawakens; pawsim on Godot's `.tres` through the fixes is identical to pawsim on `Data/`.
- **Tests**: L1 204 of 204, 6.70 M assertions (+4: D17 E1/E2, E3, EU with E4/E5's numbers, save format 2). Every Godot
  fixture made again (13 of 51 differ from the last commit, F3's included; `title` and `input_device` restored as committed,
  wall-clock). Tests written for the old rules updated: the Reawaken check in PcProgressTests, the playthrough (passage stays
  open; F22 is now tested on a passage shut by hand), every test that faked Bait through the wave counter (8 PawCore, 6
  engine) writes the Bait bank, the kitten field tests count one kitten a level on a copy of the data (the shop's step is
  asserted in the D17 test), two kitten-count tests ask for two kittens whatever the step, the Save round trip carries a Bait
  bank, the Foreman's HP spot value, the mirror's Godot rows kept inside the kitten cap. **Sabotages**: PawCore 662 (+16 new,
  1 retargeted from "the Reawaken keeps passage" to "re-locks"), each CAUGHT; realm 563 (+2 for the Catnip line, 1
  retargeted), each CAUGHT.
- Docs: 00, 01 (D17; Q12 decided), 02 (§1 length, §4 passage and Bait and boss HP, §6 kittens, §8 the Reawaken, §9 the
  Unicorn, §14 rules 16-17), 03, 06 §7, 08 §2 (format 2), 09, the review's status. Knowledge base: U/17 §3 (a quoted heredoc
  halved backslashes and wrote a real CR) and its INDEX line.

**Open.** Everything open in (100) and (101) except Q12. The approved Reawaken blurb is over the text budget (4 sentences): the
designer keeps it ("Keep blurb", 2026-11-12). Not measured by pawsim: coverage and the spatial unlocks (judge in
`PcField`), jobs. The tree's last unlocks are still bought by Reawaken 8-10 (the review's "not met" item: a price pass once
D17's Stardust curve is played). Nightly `-Full` on (102) when the PC is free (~10 h).

**Next step.** The designer plays the new economy (a real run is the check pawsim cannot be). Then, on their word: commit
(100)-(102) with explicit paths; the nightly; R6 or Q11.

## 2026-11-12 (101) - The economy rethought and measured against today's (a proposal; nothing built)

**Where the project is.** On (100), still uncommitted (the designer has not said to commit). The designer asked: "look into
the economy again ... the player should never be left for long periods with nothing to buy. Every awakening should make the
player feel stronger and unlocks should feel meaningful. Rethink the whole economy ... Then compare to our current economy."
The answer is **`docs/ECONOMY-REVIEW-2026-11-12.md`** (doc 01 Q12, open). Nothing in `Data/`, PawCore or the game changed.

**What happened.**
- **Measured with a new tool, `tools/econ_review.py`**: pawsim (`--policy v4 --data csv`) plus the game's WAVES (pawsim has
  none: found today, doc 06 §7; the tool derives each biome's HP and reward from its kill bank as PawCore does), a log of
  every shop buy, tree level and Reawaken, and proposals applied in memory (`--proposal`, '+'-joined levers). Runs are
  deterministic (~2.5 min); the repo tool reproduced the scratch numbers exactly (`build/econ/today|pass|recommended.json`).
- **Today** (406 min of pawsim time, 11 Reawakens): **52 % of the game is the re-climb** to where the last run ended,
  because passage re-locks on a Reawaken and each rematch needs 40 Bait paid in kills (9,600-112,000 a boss), which the
  spawner caps and no Reawaken speeds up (17 of 22 boss fights are rematches; the last run is 59 min, 42 of them re-fighting
  four beaten bosses). The Reawaken multiplies damage only, so income 2 min into runs 3-6 is x1.00-x1.09 the run before.
  **44 % of play is in waits of over 3 min for a noticeable buy** (longest 25.8 min), 41 % of buys are noticeable (Kitten
  Count/Power: 595 buys at 0.2 % / 1.8 %), nothing new for up to 70 min. The Overkill unlock pays +3 % income.
- **The proposal, E1-E6**: a beaten boss stays beaten (E1); Bait survives a Reawaken (E2); the Reawaken multiplies Catnip by
  the square root of the damage multiplier, x1.37 a Reawaken (E3); bigger kitten buys (E4, the 2026-10-01 alternative); the
  Overkill unlock 10 % (E5); boss HP re-priced (E6, x30 as the stand-in: first fights 49-349 s). Measured together: re-climb
  6 %, waits over 3 min 5 % (longest 4.3 min), 60 % of buys noticeable, nothing new at most 19 min, income x2.1+ a run. E1
  alone: 243 min, re-climb 16 %, waits 27 %. Rejected by measurement: E3 at full strength (runs of 1-9 min, Catnip feeds
  Stardust), a smaller Reawaken step (gate +150 %), the out-classed-boss variant of E1 (waits 31 %), raising Catnip gates.
- **The trade-off for the designer: length.** With the repetition gone, today's content is ~155 min of pawsim time (~4 h of
  real play at the soak's ratio). Lengthening the frontier (twice / three times the kills a Bait) costs waits: 9 % / 21 %.
  Not met by E1-E6: late Reawakens buy no new node (in both economies; jobs are not modelled). Not measurable here: coverage
  and the spatial unlocks (Snare, Funnel, Aura Reach, the Prism chain): judge them in `PcField`.
- Docs: the review; doc 01 Q12; doc 00 (status, the tool's row, Q9-Q12); doc 06 §7 (pawsim has no waves; the tool); CLAUDE.md
  (the tool's line). No knowledge-base entry: nothing engine-technical was learned.

**Open.** Everything open in (100). Q12 (the six changes and the length).

**Next step.** The designer's answers to the review's §7. On a yes, per change: the rule in PawCore with a test that fails
on today's rule, the Godot fixture made with the same fix (`tools/godot_fixes.py`), pawsim taught the rule (and waves) in the
same pass, a save-version bump for E2's Bait bank (doc 08), doc 02 §4 rewritten, E6's boss HP by fixed point, then
`check_all`. Commit (100) and (101) only when the designer asks.

## 2026-11-12 (100) - F3 decided and built (Patience reads the job's slowness, +6% a level); CR17 built; CR5 step 2 measured worse and taken back; the review closed

**Where the project is.** On (99) (`60a6956`, committed and pushed). The designer chose F3's recommendation ("Job slowness,
+6%/lvl") and said "Yes, package and bench" for CR17 and CR5 step 2. All three are done; the code review's worklist is
closed. `check_all` 27 of 27 green on the end state (`build/check_all_final_20261112.log`). **Nothing is committed** (the
designer's call). The Godot repo: only its findings file changed (F3's row; uncommitted, as always).

**What happened.**
- **F3** (doc 01 §5; D16, the first change to a NUMBER). Godot paid Patience for the cat's FINAL attack speed
  (`get_patience_crit_bonus(stats.attack_speed_multiplier)`), so every Attack Speed level shrank it: with the aura every level
  lost damage from Patience 5, and the best paw-only Sphinx got nothing from Patience. Now `PcStats.cpp` reads the job's
  speed (`JobTerm(AttackSpeedMultiplier)`: the Sphinx's x0.5, so Patience pays exactly its ladder), and the ladder is
  0.06 .. 0.6 (was 0.2 .. 2.0). Best Sphinx against today (stat formula on `Data/`, as (99)): paw only x1.60; paw + aura
  x0.99 at biome 1, x1.22 at biome 12; every Attack Speed level now raises damage.
  - **How a data change is carried**: two `tools/godot_fixes.py` entries (F3: `game_state.gd`'s call, `patience.tres`'s
    `level_effects`). New `fixed_text` / `fixed_read`: the exporter (`export_godot_data.py`, every `.tres` read) and
    pawsim's `.tres` loader apply the same list in memory, so `Data/SkillBranchData.csv` carries 0.06 .. 0.6 from a fresh
    export (L0's data-export gate PASS; with `fixed_read` switched off the export differs, seen) and the pawsim parity
    sweep compares like with like (quick sweep 4 of 4 SAME). Self-test +2 checks (each seen failing on a planted
    `fixed_text`). Doc 01 D16, doc 06 §2, doc 09.
  - Re-exported `Data/` (only Patience's row and the catalogue's line changed); `DT_SkillBranchData` re-imported, parity PASS.
  - **Words** (drafted first, `docs/TEXT-DRAFT-2026-11-12.csv`; the designer chose "slow job" over naming the Sphinx):
    `TREE_patience_blurb` "Increases crit damage with a job that lowers attack speed. +6% to +60% over 10 levels.",
    `FX_CRIT_DAMAGE_FROM_SLOWNESS` "+{v}% crit damage with a slow job" (`Data/TextOverrides.csv`, now 280 lines; `ST_Tree`
    regenerated).
  - Proof: the old fixtures FAILED 6 L1 cases (stats, aura, extras, kittens, pounce, tree text: 170 assertions); every
    Godot fixture made again with the fix (6 of 51 files changed, exactly those; `title` and `input_device` record the
    wall clock and were restored, see the trap below); L1 200 of 200, 6.70 M assertions. New L1 "F3: Patience reads the
    job's slowness only" (crit multiplier the same at Attack Speed 0..6, damage never lower, nothing for a full-speed job)
    and the long suite's exemption REMOVED (any Attack Speed level that lowers damage fails): both FAILED on Godot's rule
    with the new numbers (the long one: "attack_speed 0 -> 1 lowers DPS"), PASS on the fix (90,000 states at 0.01).
    5 PawCore sabotages (3 for F3: the rule test, the stats fixture, the long suite; 2 for CR17): 5 CAUGHT.
  - Godot's findings file: F3's row says what the port does.
- **CR17** (per-frame allocations; review worklist): the realm view keeps the drawings' cells (shadow, kitten, add, boss,
  cat, weapon, hat) while the atlas, the data, the stage, the boss and the job stay; the steer keys live with the bindings
  (`PawInput::SteerKeys()`, rebuilt with them) and the axis map, the adds' set are kept members; the backdrop's key is the
  tier and the size in thousandths, not a `Printf` string; the field's event queue keeps its capacity
  (`DrainFieldEvents(F, Out)` appends to the realm's own list); the audio copies the hand only when it changed; the
  renderer counts sprites per layer and records them (`LastSprites`, for the tests and the bench) only outside Shipping.
  Tests: L1 "Field: draining the events empties the queue and keeps its capacity"; `Pawpocalypse.Realm.View` "30 steady
  frames look up no drawing by name" (+ the job's hat drawn, another job's at once; `UPawSpriteAtlas::FindCount`);
  `Pawpocalypse.Input.Remap` (the cached steer keys are the steer actions' and follow a stick swap); `Pawpocalypse.Audio.When`
  "the same hand a frame later: nothing more" (added when its sabotage was first MISSED: the test never had a steady
  frame). 7 new realm sabotages, 3 retargeted (`backdrop_laid_out_every_frame`, `realm_ignores_travel_request`,
  `render_counts_the_pool`): all CAUGHT. Realm sabotages 561, PawCore 646.
- **CR5 step 2, built as the review wrote it and taken back.** Each effects band as a fixed-capacity, un-indexed triangle
  list sent by `UpdateMeshSection` (with its test and 5 sabotages, all caught). Benched: the busy "over" band draws
  8,000-16,000 triangles, so the list held 16,384 and resent ~49,000 vertices a frame: **render thread 1.1 -> 2.9 ms mean,
  p99 1.7 -> 5.6 ms** (3 runs each, quiet PC, before and after interleaved; game thread unchanged). The renderer, its test
  and the sabotages went back to step 1 (an empty band cleared once); the code is kept in the scratchpad only. A busy band
  is still made again every frame (~10,000 sections in a 20 s bench, 1.1 ms render thread in all). Next, only if numbers
  ask: a custom primitive with dynamic indexed buffers. KB U/15 §7, 2D/02 §4.6, U/24 corrected with the measurement.
- **The bench** (packaged Development at the designer's request; `build/night/bench_cr17_*.md`; the before package kept in
  `build/package_before_60a6956`): before (99) vs after (100), interleaved, two rounds, medians of 3, overlay off: game
  2.02 / 1.87 vs 1.82 / 2.08 ms mean (p99 3.07-3.49 both), render 1.10-1.13 vs 1.15, the view's draw 0.56-0.63 vs
  0.52-0.63 ms, draws 67.7 both. **CR17 changes nothing measurable**; the frame gate still FAILS on draw calls only (Q11).
  The bench now reports the bands' triangles and the sections made (`fx(tris) ... rebuilds=`).
- **A trap, fixed for good**: restoring the two wall-clock fixtures with `git checkout` wrote them CRLF (`core.autocrlf`),
  and their L1 tests failed on the LAST column only ("invalid map key": the header kept `\r`). Rewritten LF, and
  `.gitattributes` now has `*.tsv text eol=lf` (all 51 fixtures are LF in the index and on disk). KB U/17 §3.
- Knowledge base: U/18 §9 (a fix to the reference's DATA: every direct reader applies the list), U/15 §7 and 2D/02 §4.6
  and U/24 (the fixed-capacity `UpdateMeshSection` measured worse for a busy band), U/17 §3 (the CRLF fixture trap); INDEX.
- Docs: doc 00, 01 (F3, D16), 06 §2, 09 (200 L1 cases, 646 / 561 sabotages, the gates' rows), the review worklist (closed).

**Open.** Nothing from the code review. The designer's: Q11 (the draw-call budget), R6, the R5 reviews, the hand checks of
(96), Q1, Q4, Q7, Q9, Q10; the words still to draft (doc 00). Commit when the designer asks (explicit paths: the list is
`git status`; `build/` and the scratchpad are not part of it).

**Next step.** Ask the designer to commit (100). Then R6 (icons, badges, cards) with the designer, or Q11 with the numbers
above. The nightly `-Full` on (100) when the PC is free for ~10 h (its PawCore and realm sabotage sets have grown by 5 and 7).

## 2026-11-11 (99) - Review CR12 (what the UI looks up on events, held) and CR9 (the tree's tooltip built on change, focus by event); the nightly green; F3's numbers

**Where the project is.** (98) is committed and pushed (`c982078`, at the designer's request). The designer asked what fits
before the nightly and said go to CR12 then CR9 (no decision needed for either). Both are built and proven. The nightly
`-Full` ran on them (2026-11-11 21:00:43 to 2026-11-12 06:54:36, `build/check_all_full_20261111.log`): **40 of 40 green,
exit 0**. (99) is committed and pushed at the designer's request (2026-11-12, the commit after `c982078`). Nothing is
uncommitted in this repo. The review's worklist has CR5 step 2 and CR17 left (a quiet PC); the rest open is the designer's.

**What happened.**
- **CR12**: the prompts' glyph textures (`PawGlyphs::Texture`), the Widget Blueprint classes `PawUi::Resolve` finds and
  `F_Paw` (`PawUi::PawFont`) were cached weakly: the GC could take one nothing showed, and the next lookup (every prompt
  redraws on a device switch) loaded it again, synchronously, mid-play. They are the engine subsystem `UPawUiAssets`'s now
  (`UPROPERTY` maps, released in `Deinitialize`; the accessors keep their signatures). A glyph stem with NO texture (a
  keycap prompt) is remembered too: the weak map could not tell "missing" from "collected", so every redraw of such a
  prompt looked for the package again. A value the editor's force-delete nulls is looked up again. The code layout and
  `ResetResolver` empty the classes as before. New test `Pawpocalypse.UI.Widgets.Held` (the holder holds the glyph, the
  class and the font; a second lookup, and a missing stem's second lookup, load nothing: `PawGlyphs::TextureLoads`); no
  forced GC, which proves nothing in the editor (`GARBAGE_COLLECTION_KEEPFLAGS` keeps `RF_Standalone` there). 6 sabotages,
  1 retargeted (`font_umg_default_lost`).
- **CR9**: the tree's tooltip was rebuilt every frame while shown (`Pc::TreeTip`, about eight `FText::Format`, a glyph
  parse per line, a layout prepass), and the tick asked all 87 nodes for focus. Now `ShowTip` builds the lines only when the
  node or the tip's numbers change (a digest of `Pc::PcTreeTip`), and places it every frame (a zoom or a drag moves the
  node); the selection follows a node's focus by `UPawTreeNode::NativeOnAddedToFocusPath` -> `UPawSkillTreeScreen::NodeFocused`
  (same rule: only when it is not already the selection). The hover read stays a per-node flag. New test
  `Pawpocalypse.UI.Tree.TipOnce` (60 frames build nothing; a new level builds once, with the new numbers; another node's
  focus moves the selection and docks the tip; every node tells its screen); L4 gains "the tree's selection and tooltip
  follow the focus" after the tree's focus tour (45 checks), the real-window proof of the event. 4 sabotages, one of them
  on L4 (`tree_focus_event_ignored`).
- Proof: CR12 first: the new test, then `Pawpocalypse.UI` 92 of 92 and `Pawpocalypse.Input` 19 of 19; every realm sabotage
  on `PawGlyphs.cpp` and `PawWidgets.cpp` and every one `.UI.Widgets.*` owns (24, `build/cr12_realm_sab.log`): 24 CAUGHT.
  Then CR9: `Pawpocalypse.UI.Tree` 8 of 8; L4 45 of 45 (a real window, 16:54); every realm sabotage on the tree screen and
  every one the tree tests own (53 with the new four, `build/cr9_realm_sab.log`): 53 CAUGHT, `tree_focus_event_ignored`
  by L4; the clean build passes; `check_all` 27 of 27 green (17:31).
- Knowledge base: U/02 §2 (the weak-cache fix verified: a subsystem's `UPROPERTY` maps, misses cached; the editor GC's keep
  flags read in the source), U/08 §5.3 (the focus-path event in place of a per-frame focus scan; headless tests have no
  focus); INDEX, U/00.
- Docs: doc 00 (the UI row, 554 realm sabotages), doc 05 and doc 11 (L4: 45 checks), doc 09 (93 UI tests, 45 L4 checks,
  554 realm sabotages), the review worklist (CR9, CR12 done).
- **F3's numbers, for the designer** (no change made). pawsim has no jobs and no Patience, so they come from the game's own
  stat formula on `Data/*.csv` (`build/f3_numbers.py`, output `build/f3_numbers.txt`, both gitignored: on the designer's
  PC only, the numbers below are the record; the slope agrees with the long
  suite's derivation, `PcLongTests.cpp`). Only Attack Speed's factors move: paw DPS ~ speed x crit factor, the aura ~ crit
  factor, and Patience raises crit damage only below base speed. The Sphinx, crit 100%, 9 Reawakens, the run's first
  biome (Attack Speed caps at 3 levels): with the paw alone, Patience 6+ makes the first level a loss (-6% to -23%), the
  third pays it back (+18% to +61%), and at the cap Patience pays nothing (speed is past base): the best paw-only Sphinx
  with Patience 10 equals one without it. With the aura (half the DPS), every Attack Speed level is a loss from Patience 5
  (-7% to -47%; at biome 12, from Patience 8: -12% to -47%). Halving Patience does not fix the aura's case (-7% to -27% at
  Patience 10). Patience read from the job's slowness only (the Sphinx's x0.5, never the shop's levels) removes the trap
  in every build, but as written it is a buff to the best Sphinx: x1.85 with the aura, x3.00 paw only; about +60% at
  level 10 (+6% a level) keeps the aura build's best where it is today.
- **The nightly** (`-Full`, started by `build/full_tonight.sh` at 21:00, editor closed): 40 of 40 PASS, exit 0, 9 h 54 min.
  PawCore sabotage 641 of 641 CAUGHT (3 h 17 min); Realm sabotage 554 of 554 CAUGHT (4 h 25 min); the pawsim parity sweep
  1 h 22 min; the long suite 20 min (Attack Speed lowered DPS under Patience in 48,665 of 9,000,000 states: F3, counted by
  design); the Godot fixtures 17 min; L4 45 of 45 (2 min). Docs 00 and 09 record it.
- Committed and pushed at the designer's request ("Commit and push"), explicit paths.

**Open.** The review's CR5 step 2 and CR17 (both want the bench on a quiet PC). The designer's: F3 (the numbers above;
recommended: Patience read from the job's slowness only, +6% a level, its tree text drafted first), Q11 (the draw-call
budget), R6, the hand checks of (96).

**Next step.** Ask the designer for the F3 call (the options and numbers above). On a yes to the recommendation: the rule in
`PcStats.cpp` (`PatienceCritBonus` from the job's speed, not `S.AttackSpeedMultiplier`); Patience's new levels in `Data/`
(the first D16 change to a NUMBER: `tools/godot_fixes.py` carries code today, so how the CSV, pawsim's parity with the
`.tres` and the Godot fixtures carry a data change is worked out first; never in the Godot repo); the long suite's
exemption removed (then every Attack Speed decrease fails); the tree text drafted for the designer; doc 01 §5. On a quiet PC instead: CR17, then
CR5 step 2, each with `packaged_bench.py` before and after.

## 2026-11-11 (98) - Review CR11 (the icon set's owner) and CR14 (the UI's memory, the game instance's); the nightly queued

**Where the project is.** (97) is committed and pushed (`7277c49`, at the designer's request). The designer then asked for
the suggestion: CR11 then CR14, and the nightly `-Full` tonight. Both are built and proven, then committed and pushed at the
designer's request. The nightly waits in this session's background for 21:00 (`build/full_tonight.sh`, log
`build/check_all_full_20261111.log`; it dies with the session, and its engine gates skip if an editor is open).

**What happened.**
- **CR11**: `DA_Icons` was kept by a function-static `TStrongObjectPtr`, released by the C++ static destructors after the
  engine. Now the engine subsystem `UPawIconCache` holds it in a `UPROPERTY` and lets it go in `Deinitialize`;
  `PawUi::Icons()` keeps its signature (no world context needed: `GEngine->GetEngineSubsystem`). `Pawpocalypse.UI.Icons`
  asserts the subsystem holds the set; sabotage `icons_set_not_kept`. Not testable headless: the exit itself.
- **CR14**: the HUD's open tab, buy step and Jobs count seen, and the tree's last node, were file-level globals that
  crossed game instances (tests, PIE). They are `UPawUiMemory`'s, a game-instance subsystem, with Godot's rules kept: the
  tab, the step and the tree's node last the session across games (Godot's statics); the Jobs count seen starts again with
  every game (main_menu.gd's slot choice set it to -1; here the progress epoch: any new game or load), so
  `UPawHudScreen::ForgetJobsSeen` is gone. A first version reset the tab and the node per game too, as the review
  proposed; that departs from Godot without a fault to fix (D16), so it was taken back before anything was proven on it.
  New test `Pawpocalypse.UI.Hud.Memory` (a rebuilt HUD opens on the tab and step; another game keeps them; a loaded game
  does not badge the jobs it held; another game instance shares nothing; a new tree reads the node); 5 sabotages, 5
  retargeted to the renamed accessors.
- Proof: `Pawpocalypse.UI` 30 of 30 on the touched screens; every realm sabotage on the HUD, the tree, the icons, the
  memory and the title, and every one their tests own (200, `build/cr1114_realm_sab.log`): 200 CAUGHT, the clean build
  passes; `check_all` 27 of 27 green (15:23).
- Knowledge base: U/02 §2 the static `TStrongObjectPtr` rule with the verified fix (an engine subsystem behind a
  context-free accessor) and a test note (`GARBAGE_OBJECT_FLAGS` does not exist in 5.8.3); INDEX.
- Docs: doc 00 (the UI row), doc 09 (91 UI tests, 544 realm sabotages), the review worklist (CR11, CR14 done).

**Open.** The review's CR5 step 2, CR9, CR12, CR17; F3; Q11; R6; the hand checks
of (96).

**Next step.** Read the nightly's result (`build/check_all_full_20261111.log`; the session's `build/full_tonight.log` has
its exit). Then, needing no one: CR12 (glyph textures and resolved widget classes held strongly: `glyph_cache_weak`), or
R6 with the designer.

## 2026-11-11 (97) - Batch B (the load's outcome on the title, "Saved" only when saved) and F36 (a mid-fight save refunds the summon)

**Where the project is.** The designer approved the 7 drafted lines (`docs/TEXT-DRAFT-2026-11-11.csv`, "Approve all 7")
and decided F36 ("Refund it", then "Refund Bait and Catnip"). Batch B of the code review is built (CR2 with F47, CR15),
and F36 with it. F51 is in the Godot repo's findings file. (73)-(97) committed and pushed at the end of this session,
at the designer's request (the Godot repo left as it was: its uncommitted work is not all the port's).

**What happened.**
- **The words**: the 7 lines are in `ST_UI` (`TITLE_status_newer`, `_unreadable`, `_backup`; `TITLE_slot_newer`,
  `_unreadable`; `QUICK_save_failed`; `PAUSE_status_not_saved`); `text_audit` 0 of 848 strings break a rule.
- **CR2 with F47** (every load outcome reaches the player; D16 for F47): a load of the active slot leaves the title a
  notice (`UPawSaveSubsystem::GetTitleNotice`, `Pc::PcTitleNotice`; `Pc::TitleMenu` takes it): a newer build's slot
  (Continue off), an unreadable one (Continue opens the slots, never a fresh game over it), a game from the backup (said
  until the player plays; also after the startup recovery finished the slot from its backup, which used to read as a plain
  load). The cards follow the file and its backup (`SlotState`, `Pc::ClassifySlot`): "Newer version" (a press does
  nothing), "Cannot be read" (a press starts a game there), a broken slot whose backup reads shows and loads the backup's
  game. The writer keeps a slot that does not parse aside as `_corrupt` before any write replaces it. A load from the
  slots that fails plays nothing (the slots come back). `tools/godot_fixes.py` F47 changes Godot's card for the `title`
  fixture: L1 FAILED on 90 cards (`build/batchb_l1_first.log`), the fixture made again (exactly the 90 unreadable cards
  changed; the rest differ only in the wall-clock dates the fixture never reproduces), then green.
- **CR15**: every `SaveGame` returns a ticket, `GetWriteResult` says Pending / Written / Failed / NotQueued (a serial pipe:
  one atomic "finished" counter and a set of failed tickets). The quick bar says "Not saved." when nothing was queued
  (saving off) and turns "Saved." into "Not saved." when the writer refuses (the HUD's `Pump` asks); the pause menu waits
  for its slot save (the game is held) and says "Slot {n} was not saved."
- **F36** (D16, the designer's call): a summon records what `BuyBait` took (`PcField::SummonPaid`; under Audit only
  Catnip), `Pc::FightPayment` returns it while the fight is up, `CaptureSave` gives it back in the file: a save during a
  fight is the save from before the summon, byte for byte. A fight that ends (won, timed out, a Reawaken) keeps its cost.
  `Pawpocalypse.Save.MidFight` turned from Godot's rule. Trade-off, told to the designer: quitting a fight that is being
  lost now costs nothing.
- Tests: L1 "Title: the load's outcome reaches the player" and "Boss: a save during the fight is the save from before the
  summon" (198 cases, 6.70 M assertions, green); engine `Pawpocalypse.UI.Title.LoadOutcomes` (new), `.UI.Pause.Overwrite`,
  `.UI.Icons` and `.Save.MidFight` extended; the 26 Title / Pause / Icons / Save tests green. Expected save errors declared
  (`AddExpectedError`): the save code logs a refused or corrupt file at Error, which fails a test otherwise.
- Sabotages: PawCore 641 (+12: the fight's payment and refund, the slot states, the title's notice; 2 retargeted), realm
  538 (+22: the notices, the cards, the kept file, the tickets, the quick bar, the pause line, the refund; 4 retargeted).
- The proof (the files and tests this batch touched): PawCore 81 sabotages, all CAUGHT (3 of the new ones first did
  not compile: an unused variable under /WX; rewritten to compile, then CAUGHT); realm 198, all CAUGHT (the 26 new or
  retargeted among them). Four first read MISSED: `save_midfight_bait_not_saved` had become an equivalent mutant (F36
  refunds the very Bait it cleared), so `Save.MidFight` now carries an earlier lost fight's spend that must survive the
  load (CAUGHT); the other three (`hud_realm_locked_travels`, `reawaken_never_resumes`, `focus_save_on_title`) have tests
  that fail for certain under them and were CAUGHT when run again: their runs never started. `sabotage_realm.py` now says
  NOTRUN (with the run's last output) and retries once instead of reporting such a run as MISSED. `check_all` exit 0
  (`build/check_all_batchb.log`), again after the MidFight change (`build/check_all_batchb2.log`).
- Godot repo (its findings file only, uncommitted): F51 added; F36 and F47 marked as the port's; the rows for F34, F35,
  F37-F42, F48, F49 that still said "the port copies Godot until decided" now say the port fixed them.
- Knowledge base: U/10 §1 the load-outcome rule raised with a verified shape (the notice, the cards from file and
  backup, the broken file kept before any write, write tickets, `AddExpectedError`); U/17 §3 Git Bash `sed -i` turns a
  whole CRLF file to LF (`sed -b` keeps it; it happened to the findings file this session and was put back); both indexes.
- Docs: doc 00, 01 §5 (F36, F47, F51), 05, 08 §2, 09, 11, the review worklist.

**Open.**
- The pause menu's Save page still labels a newer build's or an unreadable slot as Empty (no approved words; saving to a
  newer one says it was not saved). Words to draft if wanted.
- Not covered by a test (read only): a load from the slots that fails after its card was drawn (a race the tests cannot
  stage) plays nothing and shows the slots again.
- The designer's: the eight hand checks of (96), F3, Q11, R6, commits.

**Next step.** The designer asked (2026-11-11): commit and push (73)-(97), then review CR11 (the icon set's static strong pointer:
into an engine subsystem's property) and CR14 (the HUD's tab, the jobs seen and the tree's last node: into a game-instance
subsystem, reset on New Game and on load), then the nightly `-Full` on the next free night.

## 2026-11-11 (96) - The night closed: every gate green; the editor may be opened

**Where the project is.** The night of 2026-11-10 is done (`WORKLIST-NIGHT-2026-11-10.md`, closed): the code review's batch
A and the frame half of C and D (HANDOFF-LOG (93), (94)), and D16's batch F, every Godot fault the port copied (95), each fix
proven by a test seen failing first and sabotages CAUGHT. The proof at the end held (P6): every Godot fixture reproduces
byte for byte with the port's fixes; PawCore's 55 sabotages on what changed tonight (PcPause, PcSave, the Controls page's
test and fixture) and the whole realm gate (516 of 516, 4 h 8 min, the clean build passing) all CAUGHT; the long L1 suite
(36 cases, 9.6 M assertions); the pad-only playthrough (44 of 44); the everyday `check_all`, 27 of 27 (05:59,
`build/check_all_night_final.log`). Everything is uncommitted on top of b0ce7be; nothing was committed, pushed or exported;
the two night packages are in `build/package` (after) and `build/night/package_before`. The keep-awake request is stopped.

**What happened** (details in (93)-(95) and the worklist):
- Fixed (review): CR1 the press prompt after a map change; CR3 / CR4 the audio voices and play log; CR13 the dock's key
  consumer; CR6 the stick under a pause; CR21 / F48 the title's autosave; CR5 step 1 the empty effects bands; CR16 the atlas
  index; CR10 the prompt's widget search; CR7 the debug overlay out of the bench (CR18's pool line); CR19 the storage
  comment; CR20 fullscreen and the window's fit on the window's own monitor (this PC has two).
- Fixed (D16, batch F): F34, F35, F37, F38, F39, F40, F42, F49 (CR22), F51. The fixture mechanism `tools/godot_fixes.py`
  (applied to the Godot fixtures' scratch copy only) and its everyday gate `Godot fixes current`.
- Measured on a quiet PC, both packages: draws 68.7 -> 67.7 (the overlay); the frame without the scene 41.6 (Q11); the game
  thread p99 2.2-3.6 ms (the "near 4 ms" of (85) was another project's load); nothing else moved.
- Found while closing: P6 listed the PawCore SOURCE files changed tonight, so it left out the Controls page's 12 sabotages
  (for F37 only its test and its fixture changed). Run after the proof: all CAUGHT (`build/night/proof_controls_sab.log`).
- Drafted for the designer: `docs/TEXT-DRAFT-2026-11-11.csv` (CR2 with F47, CR15: 7 lines, within the text rules).
- Knowledge base (no project names): U/09 §3 and U/24 raised to `[Verified]` (a game-instance widget leaves the viewport with
  its map); U/15 §7 a measured note and a new §8 (measuring on a PC other jobs share); U/18 §5 (a pad tour handing over to
  the pointer), §8 (a fixed reference fault turns its mutant around; and from the close, which mutants re-prove a change:
  its tests and fixtures too), new §9 (tests against a reference implementation, and the faults a port does not copy);
  both indexes.
- Docs at the close: doc 00 (the three exceptions, the status lines, the waiting list), doc 01 (F48 marked fixed), doc 09
  (the last run), the night worklist closed. For the handoff, the lines that still described Godot's faults as copied, or
  the code before the review: doc 05 (F34, F35, F37, F42, F51; CR13), doc 07 (F40), doc 08 (CR21's autosave, F49 on load,
  CR20's monitor), doc 12 (F39, F40).
- Not done, with reasons: CR8 (the settings' Godot-parity test counts a write per change), CR11, CR12, CR14, CR17, CR9, CR5
  step 2 (the numbers do not ask for it); R6.
- The night's whole code diff, from the backups taken before each file's first edit: `build/night/night.diff` (48 files,
  16.8 MB, nearly all of it the three regenerated fixtures) and `build/night/night_code.diff` without them (45 files, 2,115
  lines). Every other source file whose time changed tonight is a sabotage target written back byte for byte (39 equal to
  b0ce7be; `PawRealmSubsystem.h` keeps the uncommitted line of (81)).
- Undo and commit: the night's backups stay in `build/night/backup/` until the night is committed (`py
  build/night/backup.py --restore <files>` undoes an item; its files are in `night_code.diff`). Everything in `git status`
  belongs to (73)-(96): 460 modified, 6 deleted (the closed night worklists, (91)), 15 untracked (new tools, docs and art;
  one is the folder `SourceArt/svg/rigs/`), nothing stray. A commit stages explicit paths, never `git add -A`.

**Open (the designer's).**
- Hand checks no test can make: the pad prompt on the dock after Continue (CR1; `build/night/cr1_dock_prompt.png`); a busy
  biome's sound and the boss music in and out (CR3); the stick held while a card pick opens (CR6); a minute on the title, its
  slot time (CR21); the window dragged to the second monitor, Fullscreen ticked (CR20); the cat steered right, its hat and
  weapon (F38); the Bait button when it calls (F42); Settings > Controls with a different pad picked up (F37).
- The words (`docs/TEXT-DRAFT-2026-11-11.csv`); F36 and F3 (D16 calls); Q11 with the clean numbers; R6; commits.

**Next step.** The designer first: the eight hand checks above and the words (`docs/TEXT-DRAFT-2026-11-11.csv`). Then, in
order: (1) with the words approved, batch B: CR2 with F47, and CR15 (`WORKLIST-REVIEW-2026-11-10.md` §3 has each one's
evidence, fix and failing test; the approved lines go into `ST_UI`); without them, R6 (its direction board needs the
designer first). (2) Needing no one: F51 into the Godot repo's findings file (F47-F50 are there; doc 01 §5's F51 row is the
text; the night kept that repo read only). (3) The next free night: the nightly `-Full` (this night ran the whole realm
gate, PawCore's sabotages only where it changed). Commits only when the designer asks.

## 2026-11-11 (95) - Night part 2: D16's batch F, every Godot fault the port copied, fixed; the frame before and after

**Where the project is.** The night worklist, part 2 (designer, 23:27: "When all this done create an overnight plan and do it
without asking me"): every item of batch F is fixed in the port, each with a test seen FAILING first and its sabotages
CAUGHT; next P6, the night's proof. Everything uncommitted on top of b0ce7be.

**What happened.**
- **Part 1 step 5, the frame after**, both packages on a QUIET PC (no other Unreal process in any of the 24 runs;
  `build/night/bench_after.md`, `bench_before_quiet.md`): draws 68.7 -> 67.7 (the debug overlay, now off in the bench: CR7);
  the frame without the scene 41.6 (Q11's HUD share). Game thread, render thread and GPU unchanged within run-to-run noise
  (game 1.5-2.3 ms mean, 2.2-3.6 ms p99, under the 4 ms budget when quiet: the "near 4 ms" of (85) was another project's
  load; GPU 0.45 ms). CR5 step 1 shows nothing measurable in this busy scene, and the effects cost little: CR5 step 2 is not
  asked for by the numbers. CR18: pool 1,417-1,435 bodies for 1,280-1,287 drawn.
- **The mechanism** (doc 01 D16, as recommended there): `tools/godot_fixes.py`, the port's fixes to Godot's code as exact
  text replacements (finding, file, old, new), applied by `make_godot_fixtures.py` to its SCRATCH COPY only: a fixture then
  shows Godot's behaviour with the faults fixed, every other row Godot's own. Each fix must find its target once and change
  something; a reused copy is recognised; the repo itself is refused. Self-test; new everyday gate `Godot fixes current`.
  The Godot repo's `git status` unchanged throughout.
- **F34** (one press overwrote a slot after leaving the Save page): Godot's fix in the tool, the `pause` fixture made with it
  (L1 FAILED on 'pending'), the port's `PauseShowMain` disarms. The old sabotage that ADDED the fix is replaced by one that
  takes it away. **F51** (quit without saving kept the game): `UPawSaveSubsystem::DiscardUnsaved` (the slot as last written,
  or a fresh game). **F40** (five sounds twice, layered): once each. **F39** (two rings at twice the cat's position): round
  the cat, following it. **F35** (Fullscreen did not grey Resolution until reopened): at once (the fixture never presses
  Fullscreen: no Godot fix needed). **F42** (the Bait button trembled): a refresh leaves a running lean alone. **F37** (the
  Controls page kept an old pad's buttons): Godot's fix (a `detected_family_changed` signal), the `controls` fixture made
  with it (44 rows; L1 FAILED on 440 cells, the engine replay on 180), the port's `OnDetectedChanged`. **F49** (card levels
  not range-checked on load): Godot's fix, the `save` fixture made with it (438 rows; L1 FAILED on 438), `PcSave` clamps.
  **F38** (hat and weapon did not mirror with the cat): they mirror as one figure (`PawLook::FacingOverlay`).
- Each test and sabotage is named in the night worklist (P1-P5). Tests asserting Godot's faults were turned to the fixed
  rule: `Audio.When` (twice -> once), `UI.Settings.Applies` (Resolution live -> greyed), `PcControlsTests` (a stale column
  -> none, its case counted), the pause test's coverage count kept. Not covered by a test (read only): the view's use of the
  facing for F38 and of the pause for CR6.
- Realm sabotages 516 (490 at the night's start); PawCore 629 (one F49 sabotage added, one F34 replaced, one retargeted).
- Doc 01 §5: F34, F35, F37, F38, F39, F40, F42, F49, F51 marked fixed in the port. F51 is not yet in the Godot repo's
  findings file (read only tonight).
- `check_all` exit 0 after batch F (`build/check_all_night_p5.log`, the new gate `Godot fixes current` among them).

**Next step.** P6, the proof: every Godot fixture made again and compared, PawCore's sabotages on PcPause and PcSave, the
long suite, then the whole realm gate (516), L4, the final `check_all`; then the closing entry.

## 2026-11-10 (94) - Night step 3: the frame's own work (CR5 step 1, CR10, CR16, CR7, CR18, CR19) and CR20

**Where the project is.** The night worklist, part 1 step 3 done and its stretch item CR20; next part 1 step 5 (the
frame after), then part 2 (the designer, 23:27: "When all this done create an overnight plan and do it without asking
me": batch F, the Godot faults the port copies, under D16). Everything uncommitted on top of b0ce7be.

**What happened.** The same proof for each: a counter or test on today's behaviour seen FAILING (`build/night/c1_*.log`,
`d1_settings.log`), the fix, passing (`c2_*.log`, `d2_settings.log`), a sabotage CAUGHT (`sab_c.log`, `sab_d.log`).
- CR5 step 1, the effects bands: `UPawSpriteRenderer::FxSectionRebuilds()`; `Pawpocalypse.Realm.View` asserts 60 frames of
  empty bands rebuild nothing. Before: 180 (three bands a frame, each a collision body and a scene proxy rebuilt). Fix: a
  band is cleared once, when it empties (`FxFull`). The backdrop's `Fill` mesh was checked: it is set once per biome or
  size, not per frame. Step 2 (fixed-capacity `UpdateMeshSection` for busy bands) waits for the numbers.
- CR16, the atlas index: `GetIndexBuilds()`; `Pawpocalypse.Realm.Look.AtlasIndex` (two cells sharing an id; and the
  shipped atlas has every id once: it does). Before: 3 builds for 3 lookups. Fix: built once (`bIndexed`, with the cell
  count as a second guard), again on `PostLoad` and `PostEditChangeProperty`.
- CR10, the prompt's widget search: `GetPromptPathSearches()`; `paw.PadPlay` moves the pointer through Slate at the end and
  asserts no search in 20 frames. Before: 21. Fix: the pointer test first. L4: 44 checks.
- CR7, the debug overlay: `paw.Bench` turns `paw.DebugHud` off while it measures (measured before: one draw call of the
  frame's 68.7). CR18: the bench report has a line `debughud=0 pool(bodies)=N drawn(bodies)=M`. No test: the after
  numbers are their proof.
- CR19: `PawSaveStorage.h` says the settings read and write their own entry on the game thread, and why that is safe.
- CR20 (stretch), fullscreen and the window's fit on the window's monitor: `UPawSettingsSubsystem::WindowMonitor`;
  `Pawpocalypse.UI.Settings.WindowMonitor` (two monitors, the window on the second). Before: the primary display's size
  and work area wherever the window was (Godot uses the window's current screen). Fix: the monitor holding the window's
  centre, else the primary. This PC has two monitors (1920x1080 primary, 1360x768 to its right): today a
  fullscreen game dragged to the second asked for 1920x1080 there. A hand check for the designer (drag the window to the
  second monitor, tick Fullscreen: it fills that monitor).
- Sabotages: `fx_clears_every_frame`, `atlas_index_by_count`, `prompt_search_first`, `window_monitor_primary`: all
  CAUGHT, the clean build passing. The realm sabotage list: 502.
- Not done, with reasons (night worklist): CR8 (the settings' Godot-parity test counts a write per change), CR11 (an
  exit-time release no headless test can see), CR12 (needs a holder for the glyph textures: a day's design), CR17 (a few
  allocations a frame: little for the risk).
- `check_all` exit 0 after step 3 and CR20 (`build/check_all_night_c.log`).

**Next step.** Part 1 step 5 (package; the frame after), then part 2 P0 (the fix mechanism) and P1 (F34).

## 2026-11-10 (93) - Night step 1-2: the frame before; review batch A fixed (CR1, CR3, CR4, CR13, CR6, CR21)

**Where the project is.** The night worklist (`WORKLIST-NIGHT-2026-11-10.md`) running since the designer's "Go.package ok"
(22:13). Batch A of the code review is fixed, each fix proven: a test seen FAILING on the code before it, the fix, the test
passing, a sabotage seen CAUGHT. Everything uncommitted on top of b0ce7be; nothing packaged but the two night packages.

**What happened.**
- Preflight: `check_all` exit 0 on the untouched tree (`build/check_all_night0.log`, 27/27). Keep-awake request held by a
  background process (this PC sleeps after 30 min on mains; no setting changed).
- **Frame numbers, before** (a fresh Development package of the starting tree, 128 s; `build/night/bench_before.md`, medians
  of 3): draws 68.65 as shipped, **67.65 with the debug overlay off (the overlay is 1 draw call)**, 41.7 with the scene off
  (the HUD and the frame's fixed passes). Game thread mean / p99: 2.68 / 5.05 as shipped, 2.36 / 4.39 overlay off, 1.81 /
  3.34 with Slate global invalidation, GPU 0.46 ms. NOISY: another project's headless engine runs (Cabbage, from 22:28)
  during 11 of 12 runs. The package is kept (`build/night/package_before`) to measure again in a quiet window.
- **Batch A** (the failing runs: `build/night/a1_*.log`; the passing: `a2_*.log`):
  - CR1, the press prompt after a map change: `paw.PadPlay` (L4, a real window) now checks the prompt at the focused dock row
    after the travel from the title. Before the fix: FAIL, "in the viewport 0, visibility 3" (set visible, not on screen).
    Fix: `PlaceFocusPrompt` adds it again when it is not in the viewport (and a game world exists), as the toast is. After:
    42 / 42 checks; screenshot `build/night/cr1_dock_prompt.png` (the A glyph beside the focused row, field side).
  - CR3 / CR4, the audio voices and the play log: a player-making seam (`CreatePlayer`, as `SwapOutput`), and
    `Pawpocalypse.Audio.Voices`. Before: 40 sounds made 40 players (want 16), four tracks 4 (want 2), 10,000 plays left
    10,000 log entries. Fix: each voice and music player made once and reused while valid (the world check is gone: a
    persistent component's GetWorld() is always null); the log keeps the newest 256 (`MaxLog`).
  - CR13, the dock's key consumer: `Pawpocalypse.UI.Hud.DockConsumer` (two HUDs; the old one's Slate goes). Before: the
    old HUD's destruct wiped the newer HUD's consumer. Fix: `SetDockConsumer(Owner, ...)` / `ClearDockConsumer(Owner)`
    clears its own only; the member is private.
  - CR6, the stick under a pause: the steering decision is `APawRealmView::SteerBy` (a behaviour-preserving extraction
    first), `Pawpocalypse.Input.SteerPaused`. Before: a paused field was still steered (stick, R3, pointer). Fix: nothing
    steers a paused field (Godot's player stops with the paused tree). The call site's wiring (`R->IsPaused()`) is covered
    by reading only: no headless test drives `SampleInput`.
  - CR21 / F48, the autosave on the title: `Pawpocalypse.Save.Autosave` starts on the title (no realm). Before: 30 s on the
    title wrote the loaded game. Fix: the 30 s clock, the focus-loss save and the background save run only while a realm
    runs (`IsRealmRunning`). The test's in-play half now starts a realm.
  - Sabotages (`build/night/sab_a.log`): 8 new (`prompt_not_readded`, `audio_voice_world_check`, `audio_music_world_check`,
    `audio_log_unbounded`, `hud_destruct_clears_any`, `view_steers_when_paused`, `autosave_on_title`,
    `focus_save_on_title`) and 2 retargeted (`autosave_ignores_pause`, `dock_consumes_nothing`): all 10 CAUGHT, the clean build passing.
    The realm sabotage list: 498.
- Found while doing it: **F51** (Godot's "quit without saving" keeps the game in memory: Continue plays it and the title's
  autosave wrote it); doc 01 §5 and batch F. **CR8 moved out of the night**: the settings' Godot-parity test counts one
  write per change (Godot writes on every set too); deferring writes changes that test's meaning, for a small cost.
- The words for CR2 (with F47) and CR15: `docs/TEXT-DRAFT-2026-11-11.csv`, 7 lines, all within `text_audit.py`'s rules.
- `check_all` exit 0 after batch A (`build/check_all_night_a.log`).

**Next step.** Night step 3 (CR7 with CR18, CR5 step 1, CR16, CR19, CR10), as in the night worklist.

## 2026-11-10 (92) - A code review of the whole port; the fix plan

**Where the project is.** As (91): R5 drawn, waiting on the designer's reviews; next R6. Everything uncommitted on top of
b0ce7be. This session changed docs only (two new worklists, this entry, 00's reading order) and the knowledge base; no code,
data or asset, so no gate run was needed.

**What happened.** Designer: "Read the gdd. Do a very thorough code review. Take note of faults and think how to fix them.
Create a fix plan. Also read the knowledge base ... and add findings there." Read the Godot GDD v9 and doc 02, then the
source module by module; every engine claim checked in the UE 5.8.3 source. Result: **`WORKLIST-REVIEW-2026-11-10.md`**,
23 findings (CR1-CR23) with where, evidence, fix, the test that must fail first, and a batched plan. The main ones:
- **CR1 (H)**: the press prompt on the focused button is created once on the title and added to the viewport once; the
  viewport subsystem removes it when the title's world is cleaned up, so keys and pad players never see it in the realm.
- **CR2 (M-H)**: the title ignores `LoadGame`'s outcome; a slot written by a newer build (a downgrade, a Cloud save) plays a
  session whose every save is refused, silently. The unreadable-slot half is Godot's behaviour too (a Godot finding).
- **CR3 / CR4 (M)**: audio: `GetWorld()` of a persistent `CreateSound2D` component is always null, so every sound makes a new
  component (the 16-voice rule never cuts; a looping track can outlive its crossfade); the tests' play log grows for ever.
- **CR5 (M, frame)**: the three effect bands call `ClearAllMeshSections` / `CreateMeshSection` every frame, and each call
  rebuilds a collision body (async cooking is off) and the scene proxy, even with nothing to draw.
- Smaller: the stick moves the cat under a paused menu (CR6), the debug overlay inside the frame gate (CR7), a settings write
  per slider step (CR8), per-frame UI and realm work (CR9, CR10, CR17), weak caches that reload synchronously (CR12), a static
  `TStrongObjectPtr` (CR11), the dock consumer cleared by an old HUD (CR13), and four questions (CR2's half, CR21-CR23, among
  them doc 02's "Overkill bounty capped at 4x", which the data's exponent 0.4 makes untrue).
- Reviewed and sound, listed in the worklist §5 so it is not redone: PawCore's rules, the save reader and pipeline, the
  fixed step, pause holders, input configs, Shipping guards, the data gate, the baker's guard, `check_all`'s exit codes.
- **Knowledge base** (technical only, no project names): U/14 §2.1 corrected (its sound-table recipe had the faulty
  "re-created when the world changes" check) and a bounded play log; U/15 §6 (dev overlays inside a frame gate) and a new
  §7 (per-frame procedural meshes); U/09 §1 (an unchanged rich text is not re-parsed) and §3 (game-instance viewport widgets
  removed on a map change); U/10 §1 (every load outcome must reach the player); U/02 §2 (weak caches reload synchronously; no
  static `TStrongObjectPtr`); U/18 §2 (what headless runs cannot see); 2D/02 §4.6 (a dated note); U/24 four rows; both
  indexes.

**Open.** The designer's: the night's go (and packaging, yes or no); the words for CR2 (with F47) and CR15 (drafted
tonight); F36 and F3 (D16 calls); batch F's fixture mechanism. As (91) otherwise.

- Then, designer: "Create an overnight plan": **`WORKLIST-NIGHT-2026-11-10.md`**, waiting for the go. Batch A and the
  code half of C, each with its failing test first and a new sabotage; frame numbers before and after on a quiet PC (the
  debug overlay's share, (89)'s Slate A/B); the realm sabotages of every touched file; the words for CR2 and CR15 drafted;
  the questions made ready. Found while planning: CR21 is Godot's behaviour too (its autosave timer runs on the title:
  not a port bug); CR6's Godot side confirmed (`player.gd` stops with the paused tree); this PC sleeps after 30 minutes
  on mains (the night holds a keep-awake request instead of changing the setting).
- Then, designer: "This is a standalone version. If godot has rules or mechanics that don't make sense we should change
  them. Do not port faults from godot. This is a standalone project." Recorded as **D16** (CLAUDE.md; doc 01 §1, §1.2, D16,
  Q2, Q10, §5): Godot stays the reference; its faults are fixed here (a bug as found, a design or balance trade-off on the
  designer's yes), never in the Godot repo, each with its test. Applied: the open findings the port copies (F34, F35,
  F37-F40, F42) and CR22 are the review's new **batch F** (their Godot-fixture tests need a mechanism first: recommended, a
  patch per fault applied to the fixtures' scratch copy of Godot); CR21 joins tonight's batch A; F36 and F3 are the
  designer's calls; **CR23 resolved**: Godot's economy rework (2026-08-24, `de61fadb`) shipped the curve its redesign
  planned, and the docs saying "capped at 4x" were stale (doc 02 corrected). The review's Godot-side findings entered as
  F47-F50 in doc 01 §5 and the Godot repo's findings file (uncommitted there).

**Next step.** The night worklist on the designer's go (its "Before the go": the editor closed, the PC left unlocked, a
yes or no to packaging). Without a go: batch A by day, or R6 first and the batches between its reviews.

## 2026-11-10 (91) - The docs brought up to date and cleaned up

**Where the project is.** As (90): R5 drawn, waiting on the designer's last reviews; next R6. Everything uncommitted on
top of b0ce7be. This session changed docs only (no code, data or asset), so no gate run was needed for it.

**What happened.** Designer: "Make sure all docs are up to date ... Do a detailed cleanup on the docs. Delete or merge if
you have to." Every doc was read against the code, the gate logs and the tools.
- **Deleted**: the six closed night worklists (`WORKLIST-NIGHT-2026-10-28` ... `-11-08`): every item was done and its
  result is in this log; their two "still yours" items (the hardware check of (55), the tree's zoom) are carried in 00's
  new waiting list. The files stay in git history.
- **Moved**: entries (1)-(72) (everything committed through `b0ce7be`) to `HANDOFF-LOG-ARCHIVE.md`, unchanged; this file
  keeps (73) on. "HANDOFF-LOG (n)" anywhere means the numbered entry in either file.
- **00**: the 90-line status wall became a phase table plus one **"Waiting on the designer"** list (gathered from the open
  lists of old entries: R5 reviews, Q11 and the tree's zoom, words, the canvas's own "unimpressed" and "grudge" lines,
  hand-checks, commits); tool rows current (atlas 249 / 4 pages, 554 SVGs, the generators named, fonts tools,
  `review_rig.py`); the reading order lists the two live worklists and the archive.
- **01**: the brief says what is the port's own (the look, D14; the English, D15); scope rows; D5 taken; the questions with a
  status column and a new **Q11, the draw-call budget**; §5 split into open (with F43-F46, which were only in the Godot copy)
  and a compact fixed table (the same boilerplate sentence had been repeated 25 times).
- **02**: the dock on the right by default; the R5 faces and anatomy; the rig. **03**: the module tree as built (Platform
  not built, the editor module's real contents), Boba and the tutorial marked planned, the rig's atlas cells, the
  `r.EarlyZPassOnlyMaterialMasking` ini line. **04**: the rig's depth (`BodyZ`) and the black-layer cause, 4 pages, the
  draw-call budget as measured. **05**: eleven "not yet" lines for things since built; the 16 px text floor. **06**: the
  shared Unreal guard. **07**: 554 files, the atlas, the look lint's rules and KNOWN 23, every batch's state, the props,
  the third-party register still to create. **08**: the dock default. **09**: §7 rewritten around today's gates and counts
  (196 cases / 6.70 M assertions, 131 engine tests, 628 / 490 sabotages, the last everyday and nightly runs): 56 -> 27 KB.
  **10**: the 2026-11-10 package; 39 achievements. **11**: phase 3's "Next:" chain condensed; a restyle section; phase 9
  without the Q3 importer. **12**: four R5 traps (black layers, the tube caps, shared-depth layers, the whole-frame draw
  count); the boss-timeout row corrected. **13**: one note fixed. **CLAUDE.md**: atlas pages, the nightly's timing.
- **Worklists**: RESTYLE's status, the superseded "before R0" and "dock on the left" notes, R5 as a status list, §6 as built
  (`--ref look` and `make_sweet_art --only` were never built). SLICE6's status.
- **Appendices A, B, C**: regenerated into the scratchpad from the Godot repo: identical to the committed ones (the Godot
  repo's `git status` unchanged).

**Open.** As (90).

**Next step.** As (90): the designer's reviews; then R6 (a direction board first).

## 2026-11-10 (90) - The props approved and written

**Where the project is.** R5. Everything is uncommitted on top of b0ce7be. `check_all` (everyday) passes, exit 0, every
gate (`build/check_all_props.log`), with the new props in. My side of R5 is drawn: the creatures (batches 1-5, the
bosses' merge), the props (this entry), the floors (regenerated in R1). Waiting on the designer: the bosses, the imp,
the rig motion, the cat's pale coats (23 KNOWN), the Style board's "unimpressed", and the draw-call budget ((89)).

**What happened.**
- **Designer: "Approved."** (the props boards on the canvas, version 33). The draft (scratch `props2.py`) moved into
  `tools/art/sweet_biomes.py` unchanged: its helpers (`poly`, `ink`, `arc`, `light`, `dots`, `ring_pts`, `leaf`,
  `flower`, `kibble_bits`, `r_`, LINE 2.2), the 36 functions and `PROP_DRAW` (stem -> function); `PROPS()` is now
  `{stem: draw() for stem, draw in PROP_DRAW.items()}`; the old inline drawings and `_r`, `_ring_parts`, `_star_parts`
  (used by nothing else) are gone. Proven identical to the approved draft (every stem's parts equal). A header comment
  keeps the lesson: nothing in a prop may read as a face or a creature.
- `build_art` (554 files), look lint PASS (104 drawings, the props among them via `B.PROPS()`), `raster_svg`,
  `pack_atlas` (249 sprites, 4 pages, 75 %), `import_art --run atlas` (editor closed). The biome tour
  (`godot_tour.py --skip-godot --set biomes`): the new props on all 12 floors (`build/tour/ue/biome_N.png`).
- Canvas (version 36): the board retitled "Review: R5, the 36 props redrawn (approved, written)".

**Open (the designer's).** As (89): the draw-call budget; the bosses, the imp, the rig motion, the pale coats, the Style
board's "unimpressed". R5's exit also wants the packaged frame gate, which needs a new package (the designer's ask) and
the budget decision.

**Next step.** R6 (icons, badges, cards): start with a review sheet of the direction (a few skill icons, a card, a badge,
today beside new) on the canvas before drawing all of them; plan the UI icon atlas there too (the HUD's draw calls,
(89)). The game-thread A/B (Slate invalidation) still wants a quiet PC.

## 2026-11-10 (89) - The sludge cat written (take A); the props redrawn for review; where the draw calls went

**Where the project is.** R5. Everything is uncommitted on top of b0ce7be. `check_all` (everyday) passes, exit 0
(`build/check_all_sludge.log`), with the new sludge cat in. Waiting for the designer on the canvas (version 33): the
bosses, the imp, the rig motion, and now **the props** ("Review: R5, the 36 props redrawn (today | new)" x10892 y10700,
and "... at game size, on their floors" x12298 y10700). The frame gate's draw calls wait on a decision (below).

**What happened.**
- **Designer: "The puddle sludge cat is good."** Take A ("rising out of its puddle") is `sweet_anatomy.sludge_cat`
  (with `puddle()`), replacing the old drawing; take B dropped. `build_art`, lint PASS (0 crossings), `raster_svg`,
  `pack_atlas` (249 sprites), `import_art --run atlas`. Seen in the game: `build/tour/ue/sludge_a.png` (Litterbox,
  `paw.Spawn litterbox_sludge_cat`; the white rim round it is the signature glow, Godot's rule, not the drawing). The
  canvas board is retitled "(A approved, written)".
- **The props, redrawn as a draft** (scratch `props2.py`, `DRAW` = the 36 stems -> functions; nothing in the repo
  changed). The same note as the enemies ("indistinct blobs... a bit more anatomy and shape"): each keeps its stem, its idea
  and its colours (`sweet_biomes.PROP_COLOURS`, `palette.PROPS`), and gets a silhouette that says what it is plus one level
  of detail (facets, seams, grain, bolts, rope, kibble shapes) and a light. Fixed before review: four marks that read as a
  face or a target (the bowl's paw print, the portal ring's swirl, eyes in the carrier) and the fur's strands (a dead
  bird). The look lint over the draft (scratch `props_lint.py`: the field's SVGs copied to scratch with the draft props
  written in) PASSES; a planted off-palette colour in one prop is CAUGHT, so it does see them. The props already had the
  plum ink since R1 (the ink is a token), so "props in Plum ink" needed no separate pass.
- **The frame gate's draw calls, located** (the existing `build/package` from (85), no repackaging): 68.7 draws normally,
  **42.6 with `showflag.Rendering 0`** (the scene off). So the field (sprites, backdrop, effects) is ~26 draw calls
  against its budget of 50 (docs/04 §9: "< 50 draw calls **for the field** at peak"), and the rest, ~43, is the HUD
  (Slate) and the frame's fixed passes. `paw.Bench` counts the whole frame (`GNumDrawCallsRHI`), so the R2-R4 HUD
  (plum chrome, icons, glyph tiles, each its own texture) pushed the total from 34.6 (2026-11-04) over the field's
  budget. `Slate.EnableGlobalInvalidation 1` changes no draw calls (68.7). The game-thread numbers (2.3-3.0 ms mean,
  p99 3.6-4.7 ms; with invalidation 1.8-2.3 / 3.0-3.8) are NOT conclusive: another project on this PC (Cabbage) ran
  headless editor jobs during every run.

**Open (the designer's).**
- **The draw-call budget.** Recommended: gate the field's own draw calls as the budget is written (the scene's), and give
  the HUD its own line; the HUD's count comes down in R6 when the UI icons are redrawn, by putting them in one atlas
  (Slate batches by texture). The alternative is a total-frame budget above 69. The GPU is 0.47 ms; nothing here is a
  real cost yet.
- The props (canvas), the bosses, the imp, the rig motion, the cat's pale coats, the Style board's "unimpressed".

**Next step.** On the props' approval: move `props2.py` into `sweet_biomes.PROPS()` (helpers `poly`, `ink`, `arc`,
`light`, `dots`, `ring_pts`, `leaf`, `flower`, `kibble_bits` with it), `build_art`, lint, `raster_svg`, `pack_atlas`,
`import_art --run atlas`, the biome tour, `check_all`. The game-thread A/B (invalidation on/off) wants a quiet PC: rerun
`py tools/packaged_bench.py` with and without `--exec "Slate.EnableGlobalInvalidation 1"` when no other editor runs.

## 2026-11-10 (88) - The sludge cat redone: two takes on the canvas

**Where the project is.** As (87), plus: designer, "Redo sludge cat". Two takes are on the canvas (version 32, boards
"Review: the sludge cat, redone (A or B)" and "... moving", x10892 / x12908, y9100), WAITING for the designer. Their code
is a scratch draft (`sludge2.py` in the session scratchpad: `sludge_cat_a`, `sludge_cat_b`, `puddle`); nothing in the repo
or the game changed for it.

**What happened.** Today's sludge cat read as a green cat with drips, not as sludge. Cue kept, A CAT, MELTING; same colours.
**A, rising out of its puddle**: half cat, half puddle, the head and chest standing up out of a wide slick, the front paws
dragging it along (they step), the tail curling out of the far end, one ear standing and one sliding off with a drip.
**B, sliding down itself**: sitting, the face slipping down one side, a sagging cheek, one ear gone flat and dripping, the
body slumped into a puddle with the paws sunk in it, the tail swaying (0.8 Hz). Both: lint clean, no crossing outline, no
shared-depth overlap, body 16.2 from the nearest character (the toad).

**Next step.** On the designer's pick: move that take into `sweet_anatomy.py` as `sludge_cat` (replacing today's), then
`build_art`, `raster_svg`, `pack_atlas`, `import_art --run atlas`, the lint, a Litterbox screenshot, `check_all`.

## 2026-11-10 (87) - Tube ends fixed (the "fan"), the nightly's two failures fixed, the bosses merged

**Where the project is.** R5. Everything is uncommitted on top of b0ce7be. `check_all` (everyday) passes, exit 0
(`build/check_all_caps.log`), after every change below except the boss merge, which changes no shipped file
(`build_art --check` identical, lint PASS). The bosses (redone) and the imp (rebuilt wing) wait for the designer on the
canvas (version 31, both boards re-rendered with the fixed ends).

**What happened.**
- **The nightly (`check_all -Full`, `build/check_all_full_r5.log`) failed 2 gates; both are fixed.**
  - *Godot fixtures reproduce*: `input_bindings_cfg_godot.tsv` still had `dock_side="left"` (the default moved to the
    right in (82)); the only difference, checked. Regenerated (`make_godot_fixtures.py --only input_bindings`).
  - *Realm sabotage*: `hud_press_waits_for_refresh` MISSED. `Pawpocalypse.UI.Hud.Press` asserted that a press the cat
    cannot pay buys nothing, never that the HUD answers on the frame. The test now changes Catnip behind the HUD's back
    before that press and asserts the new amount shows; the sabotage is CAUGHT, the clean build passes. The gate's other
    line (the clean run of `Controls.MatchesGodot`, exit 2) was an Unreal editor open at that moment, not a fault.
- **Designer: "at the end of some round long spline shapes it looks like the point is flipped so the outline doesn't
  continue but looks like a fan at the end. Try just creating an arm to see".** One arm alone showed it: every tube's
  round end (`sweet_shapes._tube_pts`, used by every arm, leg, neck, tail, tentacle) was walked from the wrong edge, so each
  end crossed itself into a bow-tie. Fixed (the cap now starts at heading + 90 deg and turns back through the tip). 36
  enemies changed; the art is rebuilt, rasterised, packed and imported (`import_art --run`).
  - New look_lint rule **crossing**: no `<polygon>` outline crosses itself. Self-test plant caught; the real sabotage (the
    old caps put back) gave 263 crossing outlines; with the fix, 0. The scratch bosses had 8-12 each and the imp 6:
    probably part of what the designer saw on the imp's wing.
  - Knowledge base: 2D/01 §12, the INDEX symptom line, 2D/00-INDEX.
- **The bosses merged** into `tools/art/sweet_anatomy.py` (`scallops`, `brow`, `feather_wing`, `tyrant`, `foreman`,
  `litter_king`, `void_god`; (84)'s drafts replaced); each draws exactly what the scratch draft drew. `SPECIES` / `BATCH`
  are the bosses, redone (review only: nothing ships until approved).

**Open (the designer's).** The bosses and the imp (canvas boards); the cat's pale coats (look_lint KNOWN, 23 ground
faults); commits (the port, and the Godot dock default left uncommitted in the Godot repo); the frame gate: 68.6-69.2 draw
calls against a budget of 50 ((85): budget or cost).

**Next step.** On the designer's approval of a boss or the imp: wire it into `make_sweet_art` (the file it replaces), then
`build_art`, `raster_svg`, `pack_atlas`, `import_art --run atlas`, the lint, a field screenshot, `check_all`. Otherwise:
profile the draw-call regression ((85): `stat RHI`, field vs HUD).

## 2026-11-10 (86) - The bosses redone (poses), on the canvas

**Where the project is.** R5. Designer, morning: "Redo bosses". The redone bosses are on the canvas (version 30, the boss
boards' images replaced). **Their code is still a scratch draft** (`bosses2.py` in the session scratchpad), to be merged into
`tools/art/sweet_anatomy.py` (replacing (84)'s `tyrant`, `foreman`, `litter_king`, `void_god`; `scallops`, `brow`,
`feather_wing` come with them) once `check_all -Full` (running, `build/check_all_full_r5.log`) has finished: the gates read
that file. Nothing is committed.

**What happened.** Read as: the drafts were enemies with extra lines, not bosses. Each now has a pose and a presence, the
two cues kept:
- the Park Tyrant HISSING: the neck thrust out low, a big head, the bill wide open on a tongue and a serrated edge, both
  wings raised behind like a cloak (`feather_wing`, lifting), a wide stance on clawed webbed feet, a brow;
- the Foreman upright, LEANING ON A GIANT WRENCH stood on its head (the fist on it; first tries crossed the face): a hard
  hat with a band and a lamp, a pauldron, a chest plate and a knee plate bolted on, a banded tail, fangs, a stitched scar;
- the Litter King ENTHRONED on a throne of litter clumps (grain specks): a robe, an ermine collar, a big jewelled crown,
  a sceptre he waves, a medal, little legs that swing;
- the Void God floating: one huge eye among five unpaired ones and two shut, a crown of horns of every length, a starfield
  inside it, a maw with teeth above and below, five tentacles with suckers, each on its own beat.
- Lint clean (the Tyrant's eyes enlarged, the big eye given a plain pupil to clear the 64 px floor); no shared-depth
  overlap; bodies unique (16.0-20.3). The trade-off: wide poses fill the square, so each body is smaller in it than
  today's blob.

## 2026-11-10 (85) - Overnight step 4: the package and the frame gate (FAILS on draw calls, not because of the rig)

**Where the project is.** R5, overnight plan step 4. A Development package is in `build/package`. **The packaged frame
gate FAILS: 68.6-69.2 draw calls (budget 50) and a game thread p99 near the 4 ms budget**, with the rig on or off: a
regression from before tonight, not yet located. `check_all -Full` started after this entry. Nothing is committed.

**What happened.**
- First run (00:14 package): FAIL on "drawn != live" (the bench counted bodies; a rigged mouse draws its layers), game
  thread p99 6.76 ms, 68.4 draw calls.
- `paw.Bench` now counts a rigged mouse's body layer as its body (as the Realm test does). `paw.Rig` (new console
  variable, PawRealmView.cpp: 1 rigged layers, 0 one still sprite; a presentation A/B and a fallback).
  `packaged_bench.py --exec "..."` runs console commands before the bench.
- Repackaging hit `ConflictingInstance` twice: another project on this PC (Cabbage) was building on the same engine
  (KB U/24). Waited for its UnrealBuildTool to finish, then packaged (exit 0).
- **A/B, 1280x800, RTX 5070, 320 mice, 20 kittens** (the other project's processes running alongside: noise):
  rig on 2.36 / 3.14 ms mean, 3.84 / 5.19 ms p99, 69.2 / 68.7 draws; rig off 2.96 / 2.53 ms mean, 4.36 / 4.08 ms p99,
  68.7 / 68.6 draws. **The rig is within the noise; the draw calls are the same without it.** The last packaged gate
  (2026-11-04, (41), phase 4): 1.30 / 2.58 ms, 34.6 draws. Everything since (phase 6: the atlas pages, effects meshes,
  backdrops; the R2-R4 restyle's HUD) was never measured packaged.
- Not fixed tonight (the plan: a failing gate whose cause is not clear within the hour is written up, not guessed at).

**Next step (step 4, open).** Find where the draw calls went: `stat RHI` / `stat SceneRendering` in the package, the field
without the HUD and the HUD without the field, the atlas pages x the three sprite components (12 instanced components
now), the effects bands. Then decide with the designer: the budget (50) or the cost.

## 2026-11-10 (84) - Overnight steps 2-3: the bosses (detailed) and the imp on the canvas; look faults 87 -> 23

**Where the project is.** R5 (the field), overnight plan. Step 1 is green (`check_all` exit 0,
`build/check_all_r5b5.log`). Steps 2-3 done; step 4 (the package and the frame gate, then `check_all -Full`) waiting
for another project's engine runs on this PC (Cabbage, a net smoke test) to finish. Nothing is committed.

**What happened.**
- Designer, mid-night: "Continue with the overnight plan. Also make bosses more detailed than the rest because they will be
  larger in size".
- **The bosses** (`sweet_anatomy`, review only; `SPECIES` = the five; `make_sweet_art` still draws today's): the Park
  Tyrant (goose NECK + twice the MASS: a swollen body and chest, a thick neck, ruffled neck feathers, a scalloped breast,
  a wing in two tiers, three tail feathers, a nostril, clawed webbed feet, a scowl), the Foreman (gristle MASS + PLATES:
  the new gristle with a helmet, a shoulder slab and a chest plate in the palette's metals, edge light and rivets, a
  helmet lamp, knuckles, a forearm, two fangs, a stitched scar on the haunch, fur between the plates; its three literal
  steels gone), the Litter King (ROUND + THE ONLY CROWN: sitting, jewels and balls on the crown, a cape with folds and an
  ermine trim, a sceptre with a starred orb he waves, toes, rosy cheeks, a belly), the Void God (NO FIXED SILHOUETTE:
  spikes, five unpaired eyes and two shut, cracks, a jagged maw with teeth, four tentacles with suckers on their own
  beats), the add (unchanged but its feet step). Kept: the Rival Champion (a cat) and the generic boss (plain on
  purpose). Removed in review: the Tyrant's wing tips (a jagged clump) and bill serrations (read as a moustache), the
  Foreman's dents (made smiley faces with the rivets) and a brow scar through an eye. Lint clean; no shared-depth overlap;
  bodies 16.0-20.3 from every other character.
- `review_anatomy` / `review_rig` take bosses. Canvas (version 28): "Review: the imp, its wing rebuilt" (+ moving) at
  x10892 y7600, "Review: R5 bosses (detailed)" (+ moving) at x14500 / x16400 y1020.
- **Step 3, the look faults 87 -> 23, no pixel changed** (`build_art --check` identical): `palette.PROPS` names the five
  props' colours (fur, bag, flowers, rock, toaster) and `palette.GEAR` the cat's (the rainbow, the steels, the hoof, the
  unicorn's coat and muzzle, the ice, the alicorn's blues, the nightmare's purples, the halo's wings, the haft); the
  generators read them by name. Both are in `as_dict` (the lint's tokens); not characters. KNOWN rewritten from `--known`:
  23 left, all `ground` (the cat's pale stages 2-7 on the Alley, Turf, Kitchen, Vets; stage 6 on the dark floors): a
  look decision (the designer keeps the cat's looks). The self-test's two KNOWN plants now use a fake KNOWN colour
  (stale entry; a known colour in another file): CAUGHT. `palette.check()` clean; `PawTokens.h` unchanged.

## 2026-11-09 (83) - Overnight step 1: batch 5 written (the ghost kept, the imp's wing rebuilt for review)

**Where the project is.** R5 (the field), the designer's overnight plan (below) running. Batch 5 is in the game but for
the ghost (today's kept) and the imp (rebuilt, on the canvas for review). Nothing is committed.

**The overnight plan** (designer: "The old ghost is better. The imp seems to have problems with a wing. Create an overnight
plan" ... "Yes"; read as: start now, and yes to a package for the frame gate; the imp waits for review, as recommended):
1. batch 5 written without the ghost; the imp's wing fixed and put up for review; shots; `check_all`.
2. the bosses drafted for review only (the Tyrant, the Foreman, the Litter King, the Void God, the adds; the Rival Champion
   is a cat, "Keep today's cats looks", and the generic boss is plain on purpose: both stay).
3. the known look faults that are mechanical (literal colours to tokens); anything visible goes to a board.
4. the package and the packaged frame gate (`packaged_bench.py`) with the rig's layers; then `check_all -Full` into the
   morning. No commits, no pushes, nothing visual into the game without the designer.

**What happened (step 1).**
- `sweet_anatomy`: the ghost removed (today's `_ghost` stays). The imp rebuilt: the wing three rounded panels fanned
  from the shoulder with finger bones (the concave polygon's grown outline broke and tore as it turned); both legs
  behind the body (the near leg's hip showed as a claw on the belly); the side spike that read as a nose is now an ear at
  the back of the head. `SPECIES` = the imp, for review.
- `make_sweet_art`: the toad, crocodile, bat, dust bunny, sludge cat, robot mouse draw `sweet_anatomy`; `_bat`,
  `_sludge_cat`, `_robot_mouse` removed. Pipeline: 249 sprites, 4 pages (75 %). Lint PASS (shared depths never touch;
  every body unique), self-test every plant caught. Realm 14/14. Shots (`r5b5_*.png`): Sewers, Turf, Mythic, Factory,
  Litterbox: 0 black pixels.
- `review_anatomy` / `review_rig` take bosses (`folder_of`, a boss's game size 256 x 0.9, an add's 45, cells sized to
  the batch's biggest, bosses fewer to a row).
- `check_all` started on this state (`build/check_all_r5b5.log`).

## 2026-11-09 (82) - check_all green on (79)-(81); batch 5 on the canvas

**Where the project is.** R5 (the field). `check_all` exit 0 on everything through (81): batch 4, the black-layer fix
(`r.EarlyZPassOnlyMaterialMasking=False`), the dock on the right (`build/check_all_r5dock.log`). Batch 5 is on the canvas,
**waiting for the designer**; the game's art is unchanged by it. Nothing is committed (the port: (73)-(82) on top of
`b0ce7be`; the Godot repo: `settings_manager.gd`, the dock default, uncommitted for the designer).

**What happened.**
- `sweet_anatomy`: toad, crocodile, bat (`bat_wing`), ghost, imp, dust bunny, sludge cat, robot mouse. Cues kept: toad
  SQUAT AND WARTY (a folded hind leg), crocodile LONG, LOW, TEETH (scutes, splayed legs), bat WINGS SPREAD (finger bones, a
  scalloped edge), ghost A SHEET (a wavy hem, arms up), imp A LITTLE DEVIL (horns, wings, a spade tail; the old teardrop
  silhouette is gone), dust bunny FLUFF WITH EARS, sludge cat A CAT MELTING (big drooping ears, drips, a chin drip; the
  first draft read as a newt), robot mouse TIN AND BOXY (rivets, wheels, a wind-up key). Rigged: the toad's arms, the
  crocodile's four legs (far pair -2, near pair -1), the imp's legs and wing, the sludge cat's legs, the bat's wings
  (mirrored flap), the ghost's arms, the dust bunny's ears (lift) and feet, the robot mouse's key (swing 25 degrees).
- Merge slips caught: a leftover `A.` prefix; front legs given depth 1 without `back=False` (`sweet_anatomy.depths`
  raised it).
- Checks on the batch (scratch `batch_check.py`: its rigs written to `build/review/batch_rig`, `look_lint.rig_faults`, and
  every body against every shipped body): no shared-depth overlap; the bat's membrane (a paler tint) was its largest
  colour, 14.1 from the mouse: now the bat's own colour (16.6 from the parrot). All lint clean; eyes at 56 px 78..216.
- Canvas (version 27): "Review: R5 batch 5, toad to robot mouse" (x10892 y4480) and "Review: R5 batch 5, moving" (x12908
  y4480); batch 4's board retitled "(approved, written)".

**Next step.** On approval: route `make_sweet_art` (the toad, crocodile, dust bunny, sludge cat in `_oddities`; the bat,
ghost, imp in `_nightlife`; `_robot_mouse`) to `sweet_anatomy`; pipeline; shots on the Turf, Sewers, Portal, Mythic,
Factory, Litterbox; `check_all`. Then the bosses.

## 2026-11-09 (81) - The dock on the right by default (the HUD mirrored)

**Where the project is.** R5 (the field) continues; the gameplay HUD is mirrored: the dock on the right by default.
`check_all` has not run on (80)-(81) yet. Nothing is committed: (73)-(81) are uncommitted on top of `b0ce7be`.

**What happened.**
- Designer: "Continue. Also o want to flip the menues. Right now the menu is in the left during gameplay and i want it to
  be on the right. So mirror the ui".
- The HUD already mirrors on the dock side (Godot's `_apply_dock_side`, Settings > Controls > Menu side). The decision is
  the DEFAULT: a settings rule, so Godot first (CLAUDE.md): `C:/Games/pawpocalypse/scripts/autoload/settings_manager.gd`
  `dock_side` "right" and the fallback for an unknown value "right" (**left uncommitted in the Godot repo for the
  designer**; its CI sets the side explicitly). Fixtures regenerated (`make_godot_fixtures.py --only settings`).
- The fixture script kept its own copy of the defaults (`tools/godot_fixtures/settings_fixture.gd` DEFAULTS, still
  "left"): fixed, and it now checks its copy against a fresh `settings_manager.gd` and stops otherwise (sabotage: the
  copy set back to "left" -> "Godot did not finish"; restored).
- PawCore follows: `PcSettings` DockSide "right", the load's fallback "right". The game: `UPawHudScreen` treats no
  settings as the right, `UPawSettingsSubsystem::IsDockOnLeft` is `== "left"`, the realm's `bDockLeft` starts false. Test:
  `PawSettingsTests` (the dock starts on the right, Left moves it, and back).
- `APawDebugHud` (development only) drew at the screen's top left, under the dock until now; it now sits at the field's
  bottom corner away from the dock, over the XP bar.
- Gates run: PawCore L1 196/196; `Pawpocalypse.UI` 84/84; a fresh game's shot (`Saved/PawShots/r5dock_backyard.png`):
  the dock right, the status panels top left, the XP bar across the field. The designer's Saved/SaveGames holds no
  settings file, so their game opens with the dock on the right.

**Next step.** `check_all` (exit 0); then batch 5: toad, crocodile, bat, ghost, imp, dust bunny, sludge cat, robot mouse.

## 2026-11-09 (80) - Batch 4 written; the black layers' real cause (r.EarlyZPassOnlyMaterialMasking)

**Where the project is.** R5 (the field). Batch 4 (dog, hound, ant, cockroach) is approved and written. The black-layer
bug's real cause is found and fixed in config and verified: `Pawpocalypse.Realm` 14/14, the cvar test's sabotage
CAUGHT, six floors 0 black pixels. **`check_all` has not run on this state** (the designer's editor was open). Nothing is committed: (73)-(80) are uncommitted on top of `b0ce7be`.

**What happened.**
- Designer: "Go" (batch 4 approved).
- `make_sweet_art`: `enemy_dog`, `enemy_alley_dog` (`A.dog()` + `_can()`: the can lands in the dog's mouth, kept),
  `enemy_portal_hound`, `enemy_kitchen_cockroach`, `enemy_kitchen_ant` draw `sweet_anatomy`; `_ant` is gone. No icon or
  card draws them. Pipeline: `build_art`, `raster_svg`, `pack_atlas` (228 sprites, 4 pages, 72 %), `import_art --run
  atlas`.
- **Shared depths.** Look.Atlas's new budget test (at most three layers each side) failed: the dogs have four legs behind,
  the cockroach six. `limb(..., depth=)` lets layers that never touch share a depth (`sweet_anatomy.depths`; the dogs'
  far legs -2, near legs -1; the ant's far legs -1, near +1; the cockroach's six -1); `write_rig` and `review_rig` use
  it. **New look_lint rule `rig`:** layers sharing a depth never overlap at any of 50 moments of a 2.5 s loop (over 3 px
  at 128 px is a fault); self-test plant (the dog's far and near front legs at one depth) CAUGHT.
- **The black silhouettes came back** on the shared-depth layers (the ant's and the hound's far legs), with the layers
  0.0096 apart. Widened the mice's depth band first: `BodyZ` now gives the mice's real places (1 + pool slot, under
  `max_live_enemies` 320 + 1; `MicePlaces` 400, `MiceStep` 0.4) 30..190, z 0's rest (the boss) 190..191.6, z 1-5
  192..287; layers 0.048 apart. Tests: the depth rises with the order everywhere, the boss over every mouse, layers >= 0.04
  apart, and (Realm) `max_live_enemies + 1 < MicePlaces`. Still black on shared depths. Bisected with `-dpcvars` on the
  Kitchen: occlusion culling, occlusion queries, `r.EarlyZPass`, `r.EarlyZSortMasked`, instance order: no change;
  **`r.EarlyZPassOnlyMaterialMasking=0`: 0 black pixels.** The engine default (1, read-only) clips a masked material only in
  the depth pre-pass; the colour pass draws the whole quad with depth EQUAL, so a layer at the same depth paints its
  transparent texels (RGB 0) over another's pixels. This was also the batch-3 black (layers rounded to one depth).
  **Fix:** `Config/DefaultEngine.ini` `r.EarlyZPassOnlyMaterialMasking=False` (shaders recompile; the editor needs a restart
  to take it). Test: Look.Atlas asserts the cvar is 0. KB: 2D/02 §4.6, U/24, the symptom index, corrected to this cause.
- Shots before the config fix (wider band only): Backyard, Alley, Rooftops, Vets 0 black pixels; Portal 857, Kitchen
  2345 (the shared-depth legs). With `-dpcvars=r.EarlyZPassOnlyMaterialMasking=0`: Kitchen 0.

- With the ini in place: `Pawpocalypse.Realm` 14/14; sabotage (the ini set back to True) fails Look.Atlas ("is off"),
  restored; shots on the Backyard, Alley, Rooftops, Portal, Kitchen, Vets (`r5b4_*.png`): 0 black pixels each.

**Open.** `check_all` on this state (blocked: the designer's editor was open). The packaged frame gate with the layers.
The editor recompiles some shaders the first time it opens with the new setting.

**Next step.** When the editor is closed: `check_all` (exit 0 = done). Then batch 5: toad, crocodile, bat, ghost, imp, dust bunny, sludge cat, robot mouse.

## 2026-11-09 (79) - The rig proven in the game (a depth fix); batch 4 on the canvas

**Where the project is.** R5 (the field). The rig is in the game and proven: `check_all` exit 0 (every gate,
`build/check_all_r5rig.log`). Batch 4 (dog, hound, ant, cockroach) is on the canvas, **waiting for the designer**; the
game's art is unchanged by it. Nothing is committed: (73)-(79) are uncommitted on top of `b0ce7be`.

**What happened.**
- Designer: "Go".
- Engine tests: `Pawpocalypse.Realm` 14/14 after one fix to my own test (single-precision `PI` against a 1e-9 tolerance;
  now `UE_DOUBLE_PI`). Sabotages, each CAUGHT: `RigPose` turning the wrong way (Look.Rules: three asserts), the view
  dropping the limbs (Realm field test: 0 of 948 limbs). Reverted.
- **In the game, a layer covered by another layer of the same creature drew as a pure black silhouette** (the pigeon's
  body under its wing, the gull's wings under its body, the gristle's limbs, the parrot's body), on some instances only.
  Isolated with shots: not the turn (angles forced to 0), not early Z (`-dpcvars=r.EarlyZPass=0`), not HZB / occlusion
  queries; moving the limbs off the body cured it, and wider depth between a creature's layers reduced it (0.0015 world
  units: most; 0.003: now and then). Cause: masked quads too close in Z under this ortho camera (mechanism unknown).
  **Fix:** `UPawSpriteRenderer::BodyZ` gives Godot's z 0 (the mice and the boss) 0.08 per draw place from Z 30 (30..190),
  z 1-5 0.01 per place (190..290, under TopFxZ 300); the view spaces a creature's layers 0.12 of a place apart (0.0096
  units) and its glow's layers between -0.58 and -0.46. Tests: Look.Atlas (at most three back and three front layers),
  Vfx (the layers' depth gap >= 0.009; the kittens over the boss). Shots with time x8 (`paw.Cheat.Speed 8`): Backyard,
  Park, Rooftops, Vets: 0 black pixels (`Saved/PawShots/r5r_*.png`). Knowledge base: 2D/02 §4.6, U/24, the symptom index.
  - Shot recipe note: the last step is `paw.Shot <name> quit`; a separate `,quit` quits at once.
- Draw cost at 320 mice (Realm field test): 0.30 -> 0.34 ms.
- **Batch 4 drawn** (`sweet_anatomy.dog / hound / ant / cockroach`, helpers `paw_leg`, `ant_leg`, `roach_leg`): the dog
  CHUNKY with FLOPPY EARS and its TONGUE out; the hound LEAN AND LONG (tall legs, a tucked waist, ribs); the ant THREE
  SEGMENTS (elbowed antennae, mandibles); the cockroach FLAT, now from above (side on it was a thin line at 72 px), six
  spiny legs splayed. All rigged: four legs on the dogs, six on the insects (alternate tripods). Lint clean; eyes at 56
  px 178 / 113 / 100 / 90; every body clears UNIQUE_MIN (the dog 16.04 from the Litter King).
- `review_rig.py --batch` animates the batch under review from its drawings (`build/review/rig_batch.gif`);
  `review_anatomy.py` no longer prints "THE THE" for a floor named "The ...". Canvas (version 26): "Review: R5 batch 4,
  dog, hound, ant, cockroach" (x10892 y2420) and "Review: R5 batch 4, moving" (x12908 y2420).

**Open.** The designer's call on batch 4 and on the motion. The alley dog's can is kept but not on the review sheet (it
is added by `make_sweet_art`; check where it lands on the new dog when writing). The packaged frame gate is unmeasured
with the layers.

**Next step.** On approval: route `make_sweet_art` (`_oddities`: dog, alley dog with `_can`, hound, cockroach;
`_nightlife`: `_ant`) to `sweet_anatomy`, place the can, run the pipeline (`build_art`, `raster_svg`, `pack_atlas`,
`import_art --run atlas`), shots on the Alley, Portal, Kitchen, `check_all`. Then toad, crocodile, bat, ghost, imp, dust
bunny, sludge cat, robot mouse; bosses last.

## 2026-11-09 (78) - The birds written; the rig: wings and legs as moving layers

**Where the project is.** R5 (the field). The birds (batch 3) are approved and in the game's art. The rig (wings and legs
as separate layers, turned by the game) is built end to end and on the canvas, **waiting for the designer** on the motion.
**Not yet proven in the engine**: the editor target builds and the atlas is imported (4 pages, 204 cells), but the
engine tests wait for the designer's editor to be closed (it was open again). Nothing is committed: (73)-(78) are uncommitted on top of `b0ce7be`.

**What happened.**
- Designer: "These look great. Also would it be possible to have wings as a separate layer so they can be animated? Just
  a basic wing flap rotation. Also could you do the same for the legs on other enemies?"
- The birds are written: `make_sweet_art._birds` draws `sweet_anatomy.goose / pigeon / seagull / parrot / crow` (the old
  `_parrot`, `_crow` are gone).
- **The rig, in the art** (`tools/art/sweet_anatomy.py`): `limb(name, pivot, parts, motion, amp, hz, phase, back)` and
  `leg(...)` mark a moving part; `rig_layers()` / `rest_pose()` split a drawing into back layers, the body, front layers.
  Motions: `swing` (amp sin: legs, a pair half a turn apart; spread wings) and `lift` (amp (1 - cos) / 2: a folded wing
  lifts and settles, 0 at rest). Rates: STEP_HZ 2, FLAP_HZ 3.2, LIFT_HZ 0.8, the gull 2.4 (all loop in 2.5 s).
  - Rigged: mouse (3 legs; the golden mouse too), rat (3 legs), flying mouse (2 wings; its legs hang still), gristle (two
    arms, a hind leg; all 12), moth (both wing pairs, mirrored), vole (two digging arms, a hind foot), goose, pigeon, crow
    (2 legs, a lifting folded wing), seagull (2 spread wings, mirrored), parrot (2 legs in front, a lifting wing).
    Not rigged: the slime rat (no legs), the hamster (sits). The unrigged rest of the cast follows batch by batch.
  - A leg that was fused into the body is now its own outlined shape behind it, so the body's outline crosses the top of
    the leg (needed for it to turn). The gristle lost its "arm against the chest" contour (it floated once the arm moved).
  - `sweet_evolutions.xform` carries a layer's pivot through the fit.
- **The pipeline.** `make_sweet_art.write_rig` writes `SourceArt/svg/rigs/<stem>__<layer>.svg` (112 files) with the rig in
  `data-` attributes, and the static drawing as the rest pose. `pack_atlas.py` packs each layer at its drawing's size and
  writes the rig columns into `field.csv` (204 sprites, 4 pages, 68 %). `import_art.py` copies them into `FPawAtlasCell`
  (new: `RigOf, RigOrder, PivotU, PivotV, Motion, Amplitude, Hz, Phase`).
- **The game.** `UPawSpriteAtlas::Rig(cell)` (built with the id index) lists a drawing's layers. `APawRealmView::DrawCreatures`
  draws a rigged mouse as its layers, each placed by `PawLook::RigPose` (the pivot stays put, mirrored creatures turn the
  other way) at `RigAngleDeg` with the creature's own phase; the layers sit 0.05 of order apart (0.0005 world units); the
  signature glow is drawn per layer too. Tests: `Look.Rules` (the formulas, the pivot staying put, mirroring), `Look.Atlas`
  (the rig columns imported as packed; every rig has one body, its drawing's size, pivots inside, a known motion; the
  seagull is rigged), `Realm` field test (one body per live thing plus every rigged mouse's limbs).
- **One colour per character held:** the shipped birds tripped `look_lint unique` (the pigeon's shaded wing and the
  parrot's green wing were each its largest colour, too near the mouse / the foreman and the tyrant / the crow). The
  pigeon's wing is now its body grey (outline and two bars draw it); the parrot's accent is macaw blue `#2D78C2` (wing,
  crest, tail). Lint PASS, self-test every plant caught, `palette.check()` clean.
- `tools/art/review_rig.py` (new) draws what ships, turned as the game turns it: `build/review/rig.gif` (2.5 s loop) and
  `rig.png` (rest and both extremes). On the canvas: "Review: R5, the rig (wings and legs moving)" (x10892 y1020, canvas
  version 25); the birds' board is retitled "(approved, written)".
- Gates run: `build_art --check` identical, `raster_svg --check`, `pack_atlas --check` identical, look lint PASS and its
  self-test, the editor target builds, `import_art --run atlas` (4 pages, 204 cells). **Not run** (editor open): the engine
  tests, `check_all`, in-game shots.

**Open.** The designer's call on the motion (speed, swing, which parts). Atlas memory went from 3 to 4 pages (the
layers are full squares, not trimmed; trimming is possible if it matters). The packaged frame gate is unmeasured with
the extra sprites (one quad per layer: about 4x the enemy quads).

**Next step.** When the editor is closed:
1. `py tools/import_art.py --run icons` if an icon draws a bird (the atlas is imported).
2. `py tools/run_automation.py --no-build Pawpocalypse.Realm.Look` and `Pawpocalypse.Realm`; a sabotage: `RigPose` with the
   sign of the turn flipped must fail `Look.Rules`.
3. In-game shots (PowerShell, `-game`): the Backyard, the Park, the Rooftops; then `check_all`.
4. Then batch 4 (dog and hound, ant and cockroach), rigged as drawn.

## 2026-11-09 (77) - Anatomy batch 3, the birds, on the canvas

**Where the project is.** R5 (the field). Batch 3 of the anatomy pass, the birds, is on the canvas, **waiting for the
designer**. The game's art is unchanged by this batch. Nothing is committed: (73)-(77) are uncommitted on top of `b0ce7be`.

**What happened.**
- Designer: "Go" (batch 3, (76)'s next step).
- `tools/art/sweet_anatomy.py` draws the five birds. Each keeps the cue the shape bible gave it and gains bird anatomy:
  - the goose: NECK; a tube neck on a teardrop body, an orange bill and webbed feet, a folded wing with feather lines;
  - the pigeon: NO NECK; a puffed chest, two wing bars, a fan tail, pink feet, a beak with its cere;
  - the seagull: WINGS OUT, near front on; arched wings with dark tips and feather lines, a yellow bill with the red
    spot;
  - the parrot: HOOKED BEAK AND CREST; upright, the beak in two parts, long tail feathers, two toes forward;
  - the crow: CORNERS; a hard folded wing, a wedge tail, a ruffled throat, a thick beak.
  - New helpers: `bird_feet`, `feathers`, `gull_wing`.
  - Two first tries at the gull failed (wings read as raised arms, then as fox ears; then a wing crossed the head). The
    near-frontal spread wing is the third.
- **`tools/art/review_anatomy.py` is now per batch.**
  - The batch is `sweet_anatomy.SPECIES` plus `BATCH` (title, crowd floor, notes).
  - Floors come from TierData (`look_lint.grounds`), with their display names.
  - The sheet: `build/review/anatomy.png`, 1936x2413. It is on the canvas as "Review: R5 batch 3, the birds" (x8876
    y3100, canvas version 23).
- All five lint clean. Eyes at 56 px: goose 92 (today 98), pigeon 114 (162), seagull 75 (76), parrot 104 (120), crow 92
  (83). The goose's and parrot's eyes were enlarged after the first measure (68 and 81).
- `build_art --check` identical; the lint and its self-test pass.

**Open.** The designer's call on the birds. My notes: in the crowd each reads as its own bird; the slim ones (the
parrot most of all) look a little smaller in their box, the cost of legs and tails, as in batch 2.

**Next step.** On approval:
1. Point `make_sweet_art._birds` (`_goose`, the pigeon, the seagull, `_parrot`, `_crow`) at `sweet_anatomy`.
2. Run `build_art`, `raster_svg`, `pack_atlas` and `import_art --run atlas` (plus `icons` if an icon draws a bird).
3. Take an in-game shot on the Rooftops and the Park; run `check_all`.
4. Then batch 4: the dog and the hound, the ant and the cockroach.

## 2026-11-09 (76) - Anatomy written (seven designs); one colour per character, no shade used twice

**Where the project is.** R5 (the field) is under way. The anatomy pass's first seven designs are approved and in the
game, with a new palette that gives every character its own colour. Nothing is committed: (73)-(76) are uncommitted on
top of `b0ce7be`.

**What happened.**
- Designer: "Direction's good. Also please make sure that every enemy has it's own color and player has own color too.
  So no same shade is used twice".
- **The new palette** (`tools/art/palette.py` ENEMIES, BOSSES, PLAYER):
  - Before, eleven Stacklands families served about 35 characters. One red was the rat, seagull, parrot, hound, imp and
    the boss's adds; one orange was the vole, squirrel, hamster, ant and the Rival Champion. The cockroach was the cat's
    stripe colour, and the pigeon and dust bunny were the kitten's cream.
  - Now every enemy kind that ships (26, `flying_mouse` new), every boss (7), the cat (body, stripes and its pale belly,
    the largest colour on the armoured stages) and the kitten each have a body colour of their own.
  - Bodies are at least `UNIQUE_MIN` 16 apart (CIE76; about 2.3 is just noticeable), within a hue/saturation/value
    window that keeps each one itself, and still clear of every floor they ship on.
  - Accents are each body's own shade or tint, at least `ACCENT_MIN` 10 from every other colour. Two exceptions: the
    goose's orange beak, and the dog's brown patches (no shade of its biscuit body was free).
  - Found by a numpy search over the windows (prototype in the scratchpad, not a tool); the values are written into
    `palette.py`.
  - Where the search changed a look: the parrot is green; the ghost ice; the goose lavender white; the generic boss
    crimson (threat); the Park Tyrant forest green (it now clears its own Park); the kitten tan (it now clears all twelve
    floors); the robot mouse a tin-toy teal (no steel grey was free).
  - A kind's biome copies share its colour: they are the same enemy. My reading of "every enemy"; the designer chose
    identical copies on 2026-09-14.
- **Rule 8** in `palette.check()`: no two bodies, nor an accent and any other colour, closer than the minimums.
  - `look_lint` gained `unique`: the drawn bodies of different characters that ship must be 16 apart (`species_of`: a
    kind's copies are one character).
  - It found the robot mouse 12 from the armoured cat's belly; the robot mouse moved.
  - `--self-test`: 14 plants, all CAUGHT (new: two enemies drawn in one shade; a palette body used twice).
  - KNOWN shrank from 94 to 87: the Park Tyrant, the kitten's four floors, the flying mouse's literal wings.
- **The anatomy, written.**
  - `make_sweet_art` draws through `sweet_anatomy`: `_mouse`, `_rat`, `_moth`, `_gristle` (the brute at tier t, seeded by
    biome), the vole, the hamster, `_flying_mouse`, `_golden_mouse` (new mouse plus sparkles) and `_slime_rat` (the rat,
    legless, with drips).
  - `sweet_anatomy.rat(legs=)`, `slime_rat()` and `flying_mouse()` are new.
  - The unshipped drawings (`enemy_mouse_signature`, `enemy_squirrel`, ghost and void mouse) keep the old grammar.
  - Then `build_art`, `raster_svg`, `pack_atlas`, `import_art --run atlas` and `--run icons`.
- **Seen in the game** (`-game` 1920x1080, `-NoSave`; `Saved/PawShots/r5_backyard.png`, `r5_litterbox.png`, `r5_park.png`):
  the new mice on the Backyard; dust bunnies, bats and the sludge cat on the Litterbox. The first launch from Git Bash
  failed (`/Game/...` rewritten to `C:/Program Files/Git/Game/...`), a known gotcha I did not check first. The KB row
  now carries this symptom.
- **L0 `generated` failed on the first gate run.** `gen_paw_tokens.py` maps Godot's frozen palette to ours (for the
  Godot-fixture tests, the widget migration and the tree's tables) and refuses an old colour that became several of ours.
  The cream face became the dust bunny, pigeon and kitten. The fix: `colour_pairs` now always leaves out the character
  tables (enemies, bosses, player). They are drawn only in sprite art; no Godot script reads them (checked), so no
  fixture, widget or tree table holds them.
  - `PawTokens.h` regenerated: `PALETTE_RETIRED` gains three old character colours (`#D86061`, `#EEE2AC`, `#F6BBAD`),
    which `Pawpocalypse.UI.Widgets.Palette` checks no widget asset holds.
- Records: docs 00, 07 and 09, the worklist; KB `Unreal/24` row on path conversion.

**Open.**
- The rest of the cast in the anatomy grammar, by family: birds (goose, pigeon, seagull, parrot, crow); dog and hound;
  ant and cockroach; toad, crocodile, bat, ghost, imp, dust bunny, sludge cat, robot mouse; bosses last.
- KNOWN: the mounts' literal colours and pale coats; five props' literals; the armour's steels.
- The Style board's "unimpressed"; the (72) list.
- Gates: the everyday `check_all` exit 0, 26 of 26 green (`build/check_all_r5b4.log`), after the L0 fix above.

**Next step.** Batch 3, the birds:
1. Add them to `sweet_anatomy.SPECIES` and draw them.
2. `py tools/art/review_anatomy.py`, then the board on the canvas.
3. On approval, write as in this entry.

## 2026-11-09 (75) - The faces decided and written; the anatomy pass (enemies with bodies) on the canvas

**Where the project is.** R5 (the field) is under way. The faces are decided and written into the game. The anatomy
pass (batch 2) is on the canvas, **waiting for the designer**. Nothing is committed: (73)-(75) are uncommitted on top of
`b0ce7be`.

**What happened.**
- Designer: "Keep today's cats looks. Panic for enemies but without the drop. Also the real thing i want you to fix is
  that most enemies look like indistinct blobs. I like the general style of our stuff but it needs a bit more anatomy and
  shape."
- **The faces, written.**
  - `sweet_shapes.py`: the cat options and the sweat drop removed. `PREY_FACE = "panic"` (worried brows, tiny pupils, no
    shine) is the default for every enemy.
  - Bosses (`make_sweet_art` `_tyrant` to `_boss_add`) and the unicorn mount (`sweet_evolutions`) pass `face="today"`.
  - 60 SVGs changed: the 46 creatures, plus 2 cards and 8 skill icons that draw mice (the panic face fits them: they are
    enemies). Then `build_art`, `raster_svg`, `pack_atlas`, `import_art --run atlas` and `--run icons`.
  - The lint passes; the smallest eyes went from 65.8 to 76 px at 56 px.
  - `tools/art/review_faces.py` deleted: the decision is made.
  - **Not done:** an in-game shot of the new faces.
- **The anatomy pass.**
  - Diagnosis: the whole cast seen at once. Almost every enemy is one horizontal bean, a head circle, googly eyes and foot
    nubs. Mice, rats, voles, hamsters and the 12 gristles are the same lump in different colours; the moth is three discs.
  - Two new primitives in `sweet_shapes.py`, both polygons that `fuse()` grows by exactly the line width (every existing
    drawing is unchanged):
    - `tube(spine, w0, w1)`: a tapered limb, tail, snout or neck, with `bez()` spines;
    - `oval(cx, cy, rx, ry, angle)`: a tilted ellipse.
  - **`tools/art/sweet_anatomy.py`** (new) draws seven designs:
    - the mouse: big pink ears, a pointed snout, long hind feet, a thin bare tail;
    - the rat: hunched, a wedge snout with incisors, a ringed bare tail on the ground;
    - the gristle: a knuckle-walking brute with a spiky ridge and lumps that climb with the tier, scars and an underbite
      fang, seeded by biome as before;
    - the moth: front on, tilted wings with eyespots and veins, a fuzzy thorax, a banded abdomen, feathered antennae, six
      legs;
    - the vole: still earless (its cue), with a blunt snout and spade paws;
    - the hamster: sitting up, stuffed cheeks, paws on a seed.
  - **`tools/art/review_anatomy.py`** (new) puts each beside today's on its own floor at its sprite_target_px, then 2x,
    with a crowd on the Backyard (today | new). It runs look_lint's rules on every new drawing: all lint clean.
  - On the canvas as "Review: R5 batch 2, anatomy" (x6860 y1020, canvas version 21); batch 1's title now records the
    decision.
  - My review:
    - In the crowd at game size, the new mouse, moth, gristle and hamster read as distinct animals; today's do not.
    - Cost: tails, ears and wings take room in the same sprite box, so bodies come out smaller. The eyes were enlarged
      to compensate and still sit 5-30 % below today's: rat 105 vs 115, moth 134 vs 191 px.
    - The rat (64 px) reads smaller than the mouse.
- Records: CLAUDE.md, docs 00 and 07, the worklist (§4 Faces superseded; the R5 note).

**Open.**
- The designer on batch 2: the direction, and each design.
- Then the variants: the golden, flying and signature mice follow the new mouse; the slime rat follows the rat; the
  other ten gristles are the same function at their tier.
- Then the rest of the cast by family (birds; dog and hound; squirrel; ant and cockroach; toad, crocodile, bat, ghost,
  imp, dust bunny, sludge cat, robot mouse), with bosses last.
- From (74): the KNOWN ground faults (the Park Tyrant, the kitten, the pale coats); the Style board's "unimpressed".
- Gates: the everyday `check_all` exit 0, 26 of 26 green after the faces' write and import (`build/check_all_r5b2.log`).

**Next step.** On approval of batch 2:
1. Point `make_sweet_art`'s batches at `sweet_anatomy` (mouse and its variants, rat and the slime rat, `_gristle`, moth,
   vole, hamster).
2. Run `build_art`, `raster_svg`, `pack_atlas`, `import_art --run atlas`; take an in-game shot on a light and a dark
   floor; run `check_all`.
3. Then batch 3: the next family.

## 2026-11-09 (74) - R5: the look lint (an everyday gate) and review batch 1, the faces, on the canvas

**Where the project is.** R5 (the field) is under way. D3 is built ((73)). The look lint now gates every field drawing.
Batch 1 of the creature review loop (the faces) is on the canvas, **waiting for the designer's pick**. Nothing is
committed: (73) and (74) are uncommitted on top of `b0ce7be`.

**What happened.**
- Designer: "go" (start R5's creature loop, (73)'s next step).
- **`tools/art/look_lint.py`** (WORKLIST-RESTYLE §6 step 2) checks the 104 field SVGs (creatures, bosses, player, props).
  - Rules: palette tokens or a token's grammar shade only, one ink at one width, flat, each body clear of every floor it
    ships on, eyes that read at 56 px, and `palette.py`'s own rules.
  - Colour provenance: the generators run in-process with `sweet_shapes.tint()` recorded. A derived colour counts only if
    its base and target are allowed, so a tinted literal still fails.
  - Ground: the body is the largest exact-colour fill on a 128 px raster. The floors each drawing ships on come from
    `TierData` (spawn pools, golden enemies, bosses). The generic boss, its adds, the cat and the kitten ship on all twelve.
  - Eyes: measured from the geometry (an EYE ellipse in INK with its INK pupil), because the ghost's white body is the
    colour of an eye. The floor is 64 px at 56 px, the seagull's 65.8 rounded down.
  - **It found 94 faults in today's art** (also in Godot's): the mounts', armour's, flying mouse's wings' and five props'
    literal colours; the Park Tyrant's body 14 from its own Park floor (min 22); the cream kitten and the pale coats lost
    on the light floors (the kitten 2 from the Kitchen); the void unicorn lost on the dark floors. These are its KNOWN
    ratchet: a new fault fails, and a fixed one must be crossed off.
  - `--self-test`: 12 plants, all CAUGHT (an off-palette fill, a tinted literal, a non-INK outline, a thinner width, a
    shape's own width, opacity, a gradient, shrunk eyes, a body the colour of its floor, a stale KNOWN entry, a known
    colour in a new file, a palette token moved onto its floor).
  - Both run in `check_all`'s everyday set ("look lint", "look lint self-test").
- **The faces.** `sweet_shapes.py` gained `CAT_FACE` (`today` | `sly` | `glare`) and `PREY_FACE` (`today` | `panic`). The
  default `today` draws the art byte-identical (`build_art --check` identical).
  - Cat: lidded eyes as one outlined polygon (the lid is the outline), mismatched pupils, a coral nose, a w-mouth or
    smirk, one fang. It applies to the player cat, stage 1 and the kitten (spec `face_kind: "cat"`).
  - Prey: worried brows, tiny pupils, no shine, a sweat drop (the `ocean` token).
- **`tools/art/review_faces.py`** draws `build/review/faces.png` (1740x3290) at the game's scale: the stages at 175, the
  kitten at 96, enemies at sprite_target_px, real floors and shadows, then 2x. It is on the canvas as the board
  "Review: R5 batch 1, the faces" (x5040 y1020, canvas version 20).
  - My review: B (sly) at first read as "unimpressed" (flat lids), so its lids were steepened. C (glare) reads as
    malevolent. Recommended: cat C, prey B.
  - Honest limits: at 56-64 px, prey B's brows and drop are faint (they read at 2x). The stage 4-7 rider's face is
    illegible at game size in every option. The drop is missing on the hand-built drawings (the gristle, the flying
    mouse's wing covers it); the write pass adds it.
- Records: CLAUDE.md (two tool lines), docs 00, 07 and 09 (the gate row), the worklist (R5 note).

**Open.**
- The designer's pick on batch 1.
- The canvas's Style board rule 3 still says "unimpressed" (banned 2026-11-08): the designer's call to edit.
- Still open from (72): the title's `{n} kills` wording, the canvas's two "grudge" lines, the tree's zoom, Continue's slot
  line, the hardware check, untranslated changed lines (phase 7).
- Gates: the everyday `check_all` exit 0, 26 of 26 green with the two new look-lint gates (`build/check_all_r5b1.log`).

**Next step.** On the designer's pick:
1. Teach `look_lint`'s eye rule the lidded polygon eye (visible white area). Flip the defaults in `sweet_shapes.py`, add the
   drop to the hand-built prey, and leave bosses on `today` (explicit `face=` in their `S.eyes` calls) until their batch.
2. `build_art.py`, `raster_svg.py`, `pack_atlas.py`, `import_art.py --run atlas`; in-game shots on a light and a dark
   floor; `check_all`.
3. Batch 2: the KNOWN ground faults (the Park Tyrant, the kitten, the pale coats) and the mounts' literal colours.

## 2026-11-09 (73) - R5 started: D3 decided and built (the aura lifted, the clutching paw on the strike, crits in Butter)

### Where the project is
- R0-R4 done; **R5 (the field) started**; its first decision, D3, answered and built. Committed through `b0ce7be`; this
  entry's work is not committed.
- Everyday `check_all` **exit 0, every gate passed** (`build/check_all_d3_built.log`). `Pawpocalypse.Realm.Vfx.*` 3 of 3 (`build/run_automation_d3.log`); the six new realm
  sabotages each CAUGHT (`build/sabotage_d3.log`).

### What happened
- **The review sheet** (`tools/art/review_d3.py` -> `build/review/d3_aura_strike.png`, on the canvas
  https://claude.ai/artifact/3wbqpr2HT4daaSmtHtNT2K as the board "Review: D3, the aura and the paw strike"): at 1080p from the
  game's own floors, enemies, shadows, aura radius (150: base 77, x3 at the cap) and `PawVfx.cpp`'s swipe geometry. Aura
  A (today: Godot's gold ring + 14 % fill; the game never had the concept's pink) to E on two light and two dark floors;
  strike S1 (today: arc + claws, no paw) to S3 at 25/50/75 % of the sweep, zoomed 2x; crit pink / Butter / Crimson.
  Found: the strike lands on the cat's own face (reach 62 px, inside the cat), in the game today too.
- **The designer: "Recommended."** Aura **D** (a 10 % white lift, a Bone ring on a Plum edge), strike **S3** (the clutching
  paw, `ui_cursor_paw_closed`'s drawing, rides the tip of an arc 0.6 of Godot's length, solid for the first 37 % of the
  sweep then fading), crit **Butter** (luck's and earned; an earned crit stays twice as wide). The Unicorn's rainbow aura
  keeps Godot's look. Timing, the arc's geometry and the claws stay Godot's.
- **Built:** `tools/art/fx_look.py` (the approved values, one place) -> **`tools/gen_fx_look.py`** -> generated
  **`Source/Pawpocalypse/Realm/PawFxLook.h`** (the colours as Godot Colors; the paw's shapes from the drawing function,
  ellipses tessellated, each stroked shape a fill then an ink outline; L0 `generated` checks it, its plant caught) ->
  `PawVfx.cpp` (`DrawStrikePaw`: the paw drawn as polygons in the swipe's band, after its arc and claws; a translucent atlas
  sprite would have drawn UNDER the band). `review_d3.py` reads `fx_look.py` too; the sheet regenerates byte-identical to
  the approved one (sha256 62e5e342...). When R6 redraws the cursor, re-run the generator and the strike paw follows.
- **Test** `Pawpocalypse.Realm.Vfx.Look`: the aura's ring Bone on a Plum edge, the white lift at 10 %, no gold; the paw at
  the arc's tip (computed from vfx_swipe.gd's numbers in the test), its claws ahead along the motion, at its size; crits
  Butter, no pink; a plain strike no Butter. The palette's bytes are written out in the test, not read from the header.
  **Sabotages** (realm, 490 now): `vfx_aura_ring_gold`, `vfx_aura_no_lift`, `vfx_strike_no_paw`, `vfx_paw_backwards`,
  `vfx_paw_at_the_cat`, `vfx_crit_pink`: **each CAUGHT**, the clean build passes.
- Crashed once on the way: closing the paw's outline with `Points.Add(Points[0])` (an element of the array being added to
  it; KB `Unreal/24` already had it, I did not look first; added: the check fires even with room to spare).
- **Seen in the game** (`-game`, 1920x1080): **`paw.Fx look`** (new, `Dev/PawShot.cpp`): holds the field, owns the aura,
  steps three swipes to 25 / 50 (crit) / 75 % (a swipe outlives no 30-frame queue beat). Shots
  `Saved/PawShots/d3_backyard.png` and `d3_sewers.png`: the ring and lift read on the lawn and in the Sewers, the paw at each
  arc's tip, the crit's arc Butter.
- **Finding for the creature sheets:** the game draws the cat from `player_cat_stage_N`, whose drawing leaves room inside the
  175 px box (~115 px wide at stage 1); the D3 sheet used `player_cat_base` (fills its box), so its cat was ~1.5x too big.
  The aura and paw were at their true sizes; D3 stands. Worklist R5 notes it.
- Docs: CLAUDE.md (two tool lines), 00 (status, tool row, 490), 07 §3 (the effects' look), 09 (490), worklist (D3 row, R5).

### Open
- Everything open in (72) still is. The strike landing on the cat's face is the game's rule (reach inside the cat), not a
  look question; raise with the designer only if the shots bother them.

### Next step
Start R5's creature loop (worklist §6): `tools/art/look_lint.py` (an everyday gate; one off-palette fill must FAIL), then the
first review batch, drawn with the stage art and sizes the game actually draws.
