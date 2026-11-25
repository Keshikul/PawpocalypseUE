# Localisation: the plan (phase 7, started 2026-11-18)

The designer, 2026-11-18, asked which languages the store lists: "Check all most used steam languages as translate into all
of them. Also if we make changes later make sure that we don't have to retranslate everything and just redo things we
changed." This is phase 7 (doc 11), planned in doc 08 §4. This page records the decisions and the order.

## 1. Which languages

Steam Hardware & Software Survey, October 2026 (store.steampowered.com/hwsurvey; read 2026-11-18 through a fetch whose
language table was model-extracted from the page, so the figures are approximate):

| Share | Languages |
|---|---|
| In the game's set (21) | English 39.0, Simplified Chinese 22.0, Russian 10.1, Spanish (Spain) 4.8, Portuguese (Brazil) 4.5, German 2.9, Japanese 2.5, French 2.4, Polish 1.7, Korean 1.4, Turkish 1.4, Traditional Chinese 1.3, Spanish (Latin America) 0.9, Thai 0.8, Ukrainian 0.7, Italian 0.6, Czech 0.6, Hungarian 0.4, Portuguese (Portugal) 0.4, Swedish 0.3, Vietnamese 0.2 |
| Not in it | Dutch 0.27, Danish 0.22, Indonesian 0.16, Finnish 0.15, Romanian 0.13, Norwegian 0.12, Greek 0.06, Bulgarian 0.05, Arabic and Malay about 0 |

**The 21 languages Godot already shipped cover about 98.9 % of the survey.** Every language above 0.25 % is in the set
except Dutch (0.27 %, tied with Swedish). Adding Dutch and Danish would add about 0.5 %; Greek, Bulgarian and Arabic
would need new fonts (Arabic also right-to-left layout). **Decided (designer, 2026-11-18): "Add dutch and danish": 23
languages.** Dutch and Danish have no Godot translation: every line is new for them, with a glossary first. Order: "Restyle
then translate" (`docs/WORKLIST-NIGHT-2026-11-18.md`).

**Decided (designer, 2026-11-23): no native reads.** "Native speakers will not be reviewing the text so skip that." The translations as they stand (machine-checked: `loc_lint`, `loc_status`, the language pass, the package; every line read for meaning, §2e-§2f) are what ships. The language lists drop "(draft)" (D20, the designer: "drop the draft"; HANDOFF-LOG (138)): each language in its own name alone.

## 2. Only what changed is translated again

- **The store page** (`build/steam/localization`): `translated_from.json` keeps, per language and field, a hash of the
  English the translation was made from. `make_store_localization.py --todo` lists the fields to translate again;
  the pack is not written while one is stale; `--stamp` records a new translation. Self-test: an English edit makes only
  that field stale, in every language (built 2026-11-18).
- **The game** (Unreal's localisation, doc 08 §4): each culture's `.archive` keeps the English source a translation was
  made for. A changed English string comes out of the next gather as untranslated in every culture, and only it; the rest
  keep their translations. The tool to build (`tools/loc_status.py`): per culture, the keys missing or stale, with the
  English and, for a stale one, the old English beside the new (a translator changes the part that changed).

## 2a. Built 2026-11-19 (night)

- **The translations' files**: `Content/Pawpocalypse/Localization/Translations/<culture>/ST_UI.csv`, `ST_Data.csv`,
  `ST_Tree.csv`, columns `"Key","Text","Source"`: Source is the English the translation was made for. Unreal culture
  names: Godot's `es_MX` is `es-419` (Steam's Latin American Spanish), `pt` is `pt-PT`, `pt_BR` `pt-BR`, `zh_Hans`
  `zh-Hans`, `zh_Hant` `zh-Hant`; Dutch `nl`, Danish `da`.
- **`tools/port_po.py`** (gate `translations carried over`, `--check`; a planted row is CAUGHT): Godot's 20 `.po` files
  carried over by English and context (a key's prefix names Godot's msgctxt: UPG_ upgrade, CARD_ card, SKILL_ / TREE_
  skillbranch ...; an ST_UI comment's "ctx <name>"). **517 of the 837 lines that need words carried in 16 languages, 514
  in cs, pl, ru, uk** (their three plurals have more than two forms: ICU plural patterns, later). Report:
  `build/loc/port_po.md`. Not carried: 332 changed or new English lines, 1 whose placeholders differ.
- **`tools/loc_status.py`** (gate `loc status self-test`): per culture, translated / missing / STALE (its Source is not
  today's English) / broken placeholders; a multiplier (x5) or a letterless line needs no words. `--list` prints a stale
  line's old English beside the new. `--todo` writes `build/loc/todo/<culture>.csv`: every line to translate with the
  English, the old English of a stale line, the translator comment and the store glossary's terms it contains.
- `--apply` and the title's language menu: built 2026-11-20 (§2b, §2c).
- **The run-time route (built 2026-11-20, HANDOFF-LOG (128)):** the translation files registered as polyglot text at
  start (`PawText::RegisterTranslations`, KB Unreal/20 §3: no `.locres`, no Dashboard round trip, the language changes
  live), a row only when its Source is today's English (a stale line shows the English until it is translated again).
  Doc 08 §4 planned `.locres` through PO files; the files serve either route.

## 2b. Built 2026-11-20

- **The languages' rules** (`PcLocales`, port of `locales.gd`, fixture `locales`: available, from_system, from_steam, the
  picker's labels with each language's own "(draft)" from Godot's .po files; the "(draft)" dropped 2026-11-23, D20):
  the setting keeps Godot's codes
  (`es_MX`, `pt`, `zh_Hans`), the game applies Unreal's culture for each. Dutch and Danish are in the table after
  Swedish and appear once they have translations. F52 (doc 01 §5) fixed: Portugal no longer gets Brazilian Portuguese,
  Spanish outside Spain gets the Latin American translation, Macau gets Traditional Chinese.
- **Shown at start and on change** (`UPawSettingsSubsystem::ApplyLanguage`): the setting resolved (system: the OS
  language), a `-culture=` on the command line wins (the tour, the shots); never in the editor.
- **Settings > Video > Language**: "System", then every COMPLETE language in its own name with "(draft)" in its own
  words (`SETTINGS_language_draft`, Godot's "(draft)" carried with its 20 translations; dropped 2026-11-23, D20: the
  name alone, the line gone from the tables); a pick saves and shows it at
  once. Complete = no line missing (`PawText::TranslationCount(...).Missing`, counted as `loc_status.py` counts: 837
  lines need words since §2c's fix). Godot lists only loaded `.po` files "so an unfinished language cannot be chosen by
  accident", and its files were complete; on 2026-11-20 no language was (319 lines each to translate), so the list was
  System and English (German complete since §2c).
  **`-PawDraftLanguages`** on the command line lists the unfinished ones too, for review: their missing lines show in
  English.
- The loader also leaves out a row whose `{placeholders}` differ from the English (as `loc_status.py` counts it broken).
- **The title's language list** (2026-11-20, HANDOFF-LOG (129); `main_menu.gd` `_open_language_menu`): the cluster's
  Language opens a card above it, one row per language offered, each in its own name; the language shown on the yellow
  plate with the tick; the keys start there; a row saves and shows its language and closes the list; Back, the button
  again or a press off the card close it; the title's words follow a new language. 21 rows scroll inside the card at
  1280 x 800 and 150 % UI (`Layout.Title`). The concept draws only the button: the list is built from the kit and waits
  for the designer's look.
- In the editor, game text follows a culture only with `Localization.ForceLoadGameLocalizationInEditor` on (the
  preview needs compiled game localisation, which this game does not have): `PawText::ShowCulture` sets it.

## 2c. Built 2026-11-20 (HANDOFF-LOG (130))

- **The files are the translations.** `port_po.py` used to write each culture's files from Godot's `.po` alone: a run
  after a hand translation would have dropped it. Now, per key in the English table's order: the file's row when it is in
  use (its Source is today's English, its placeholders the English's: Godot's or the port's own), else Godot's
  translation of today's English, else the file's row as it is (stale or broken: kept, so the old English shows beside
  the new), else none. A row whose key left the tables is dropped. `--check` fails when a run would change a file. Dutch and
  Danish get files with their first row. The report (`build/loc/port_po.md`) counts Godot's, the port's own and the stale
  rows kept.
- **`loc_status.py --apply build/loc/todo/<culture>.csv`**: the work file's Translation column into the culture's files,
  Source = the work file's English (what the translator saw: a line whose English changed since arrives stale and is
  listed again). All or nothing: a row for no line, a key twice or a `{placeholder}` lost or added refuses the whole file.
  Written through `port_po.py`'s merge, so a later `port_po` run keeps every row. `--todo` never overwrites a work file
  holding a translation not yet applied. Self-test (gate `loc status self-test`): the refusals, the Source seen, the
  merge order, the guard, and an apply into a copy of the German files (less the lines Godot never translated) that a
  later `port_po` run leaves byte for byte; 11 planted faults in the two tools each failed it.
- **Fixed:** `loc_status.py` counted the `n` of an escaped line break (`\n` in the CSV) as a letter, so the layout-only
  `TITLE_leaderboard_text` ("{rank}\n\n{offline}\n\n{last}") needed words for it and not for the game
  (`PawText::NeedsWords` reads the unescaped English): 838 against 837. It now reads a cell as the engine does.
- **Every language complete: 22 translations x 837 lines** (2026-11-20). Godot's 20: the 319 lines each had no Godot
  translation for (322 in cs, pl, ru, uk), with each carried translation's own terms (German Katzenminze, Satz,
  Wiedererwachen; es-419 "Presiona", "Configuración"; pt-PT "tu", Salto, ranhura; ...). Dutch and Danish: all 837, terms
  chosen here and listed at the top of their scratch dicts, recorded in the handoff (nl Kattenkruid, Sterrenstof, Lokaas,
  Sprong, Herontwaken, Rijken, Vangsten; da Kattemynte, Stjernestøv, Madding, Spring, Genopvågn, Riger, Fangster): the
  store page's Dutch and Danish follow them. Numbers keep the game's '.'. Every language is offered in Settings and on the
  title, each "(draft)" in its own words until D20 dropped it (2026-11-23: no native read will come).
- **Grammar the placeholders cannot follow** (KB Unreal/20 §3): Russian, Polish, Czech and Ukrainian put a count after a
  colon ("Перебросы: +{v}") so no noun agrees with a number (the three plural lines Godot gave more forms need no ICU
  pattern now); Hungarian and Turkish put no suffix on a `{placeholder}`; Swedish avoids `{v}:e` where the number varies.
- **Two of Godot's carried translations fixed** (D16, faults): Traditional Chinese called the spawn rate 重新整理速度 (a
  machine conversion of 刷新, "browser refresh"): now 生成速度 / 生成率; Czech "Duhový Hovínko" (a masculine adjective on a
  neuter noun): now "Duhové hovínko", in the carried lines too (a work file's fix rows).
- **Font coverage reads the translation files** (it read only Godot's `.po`): 26 culture lists, 6938 characters, PASS; a
  planted unsupported character in a German line is CAUGHT.
- The store scenes in a language: `py tools/store_shots.py --culture de` -> `build/steam/shots/de/` (German: all nine;
  Russian and Thai: three and two).
- Open: a language that was complete drops out of the lists when an English edit makes one of its lines stale (complete
  only, as Godot). Before release, a packaging gate for the shipped languages (step 3 below) keeps that from shipping.
  A native read of every language before it ships; the pseudo-locale tour (step 4).

## 2d. Built 2026-11-20 (HANDOFF-LOG (131)): the language pass (step 4's tour)

- **The pass is a test, not screenshots.** `Pawpocalypse.UI.Layout.Languages` lays out every screen with words (the
  title and its windows, the slots and the delete prompt, the six dock pages with a filled save, the card pick with every
  card, the skill tree with the four longest tips in each language, the pause menu and its slots, the three settings
  sections, a Reawaken's ceremony, spend step and job picker, the victory) in English and the 22 translations at the
  tightest size, 1280 x 800 at 150 % UI (1153 x 720 of layout), and holds every text to its box
  (`PawLayout::TextMisses`: wider than its box, a whole line cut by a box that clips, or off the screen). What it finds
  goes to `Saved/Layout/languages.txt`. `Pawpocalypse.UI.Layout.PseudoLocale` does the same in the pseudo-locale;
  `Pawpocalypse.UI.Layout.TextMisses` proves the detector sees a text too wide and a line cut and passes one that fits.
  The three run in 25 s, in the everyday `Pawpocalypse.UI` gate.
- **The pseudo-locale** (`PawText::Pseudo`, `SetPseudoLocale`, `-PseudoLoc` on the command line for the tours): every
  string table's English rewritten in place, each letter accented (Latin-1, which every face of `F_Paw` has),
  `{placeholders}` and multipliers kept, about 40 % more in words of up to six letters (one long padding word could never
  wrap), bracketed. Names never translated (the logo, Boba, Discord, the pad brands) stay. `screen_tour.py --culture X` (was `godot_tour.py`)
  (or `pseudo`) shoots the real game in a language.
- **Found and fixed** (the first run: 594 texts in 23 languages, 30 in the pseudo-locale; most in English too):
  - At 150 % UI the screen is 720 tall: the **Reawaken ceremony**'s title went off the top and its payout off the bottom,
    the **slot picker**'s fifth slot off the right, the **card pick**'s Reroll off the bottom in German. Each now stands
    in a box that shrinks it to the screen and never grows it (the ceremony's cat, title and payout on a 880 x 900 stage;
    the slots by their row, 1496 wide; the pick's column 16 px off the edges).
  - Headlines a language makes wider than their box shrink to it: the ceremony's ("WIEDERERWACHEN", "YENİDEN UYANIYOR"),
    the victory's ("OVERWINNING", "ZWYCIĘSTWO", "CHIẾN THẮNG"), the controls table's column heads ("HANDKONTROLL").
  - Wraps wider than their boxes: a button's words beside its icon now wrap by what the icon leaves (the Catches rows'
    held cards, the Realms rows, the Reawaken button: ten languages); the job rows wrapped at 360 beside a 339 space
    (English too): every state button is now as wide as the widest of Worn / Owned / Locked in the language, measured
    with its font, and the words wrap by it; the award rows by their Claim button; the job picker's blurb 4 px short of
    its card (a shrunk font measures ~1.5 % wider); the credits and leaderboard windows' text short of the scroll bar.
  - The card face's body padding was still Godot's 18 (the asset missed the Sticker change to 13: the text box 10 px
    under its wrap); the skill tree's hub pill now grows with its words ("Árbol de habilidades" ran past Godot's 150).
  - Translations shortened where a line was wider than its box: Boba's role in hu, it, ru, sv ("AD e direttore
    generale", "VD och general manager", "Гендиректор и управляющий", "Vezérigazgató és ügyvezető", and the tooltip with
    it); Hungarian's save status ("Van mentés — ...").
  - In the assets: `bake_widgets --graft` (new: `wrapat:` copies a text's wrap width; `wrap:Code=Asset>Wrapper` when the
    auto-names differ). WBP_ReawakenCeremony, WBP_VictoryScreen, WBP_CardPickScreen, WBP_ControlsPanel, WBP_MainMenu,
    WBP_CardFace.
- Open: the native reads (none: decided 2026-11-23, §1); the real game's screenshots at the tight size in a few languages for the designer's eye (the
  test proves the boxes, not the look of a shrunk headline).

## 2e. Built 2026-11-20 (HANDOFF-LOG (132)): the wording review

- **What was read**: all 22 translations, by machine where a rule can be strict (numbers, line breaks, lines left in
  English, the game's words for Catnip / Stardust / Bait in every sentence that names them, each name translated one way,
  repeated English translated the same way, how each language addresses the player, mouse words, punctuation, quotes,
  percent signs, scripts mixed, length), then by eye: every short UI label side by side in all 22 languages. Not a native
  read.
- **Held up**: every name is translated one way in each language; every line the English repeats is repeated; each
  language addresses the player one way (du, vous, ты/вы...); every sentence that names a currency uses the language's
  word for it.
- **Fixed (122 lines)**:
  - "every 100th kill" written as a word in cs, pl, ru, sv, uk ("сотое", "setne"...): now the number ("100-е", "100.",
    "100:e"); Hungarian "x3" as a word ("háromszorosáért"); Czech and Polish dropped a "1" (real numbers: the text rules).
  - Japanese and Korean left the credits line "created by keshi" in English.
  - Turkish "Reawaken early" said "wake early" (Erken uyan), unlike every other Turkish line: "Erken yeniden uyan".
  - The card's Take / Banish buttons: Banish was a noun beside a verb in es, es-419, pl, ru, uk, tr ("Destierro",
    "Изгнание"): now verbs.
  - Korean "Swapped with {action}" used 와, which is wrong after a consonant; the word is not known: 와(과), as its Bait line
    writes 이(가).
  - Percent signs: Turkish wrote "40%" in 53 lines and its own "%40" in 21 (one line both): now "%40" throughout. Czech,
    French and Swedish had two lines with "40 %": now "40%" as their other 100.
  - Danish and Dutch capitalised Catnip and Stardust like the English but not Bait: "Madding", "Lokaas" now too.
  - **F55** (doc 01 §5): both Chinese scripts named enemies after rats where the English names no animal (Swarm 鼠群,
    Exterminator 灭鼠专家, Infestation 鼠患): now 敌群 / 敵群, 歼灭专家 / 殲滅專家, 泛滥 / 氾濫 ("enemies, never mice").
- **Kept, on purpose**: "10.000" for ten thousand in de, da, es, it, nl, pt, tr, vi (the language's own mark; the game's
  decimals keep '.', and only whole thousands are written this way); counters a language adds ("1 回", "1 体"); the
  abbreviations "Máx.", "Niv."; Italian "Gioco di prestigio" for Sleight of Hand (a conjuring trick, not the Reawaken).
- **The gate** `tools/loc_lint.py` (check_all `translations lint` + its self-test): numbers kept (a thousands mark of the
  language's own allowed), line breaks, no line of 3+ words left in English, no mouse or rat word where the English has
  none (F55), F53 and F54 stay fixed (until now asserted by nothing), one percent style per language. The self-test plants
  each fault in a copy of today's rows and requires it caught.

## 2f. Built 2026-11-20 (HANDOFF-LOG (133)): every line read for meaning

- **What was read**: every line longer than a short label (473 English lines, each beside its 22 translations), for
  meaning, grammar and plainness; the short labels were read in §2e. Still not a native read: idiom and tone need one.
- **Fixed (96 lines, 21 languages)**:
  - Wrong meaning: es / es-419 named the Rival Champion "El Gato Rival" (a different boss); Turkish Fence "Kaçakçı"
    (smuggler; a fence is "Yatakçı"); Polish Overflow "Przelew" (a bank transfer); Russian Swarm Caller "Зовущий рой" ("the
    swarm that calls"); Thai Reawaken said the beaten bosses "still lose"; Hungarian "nem tölt vissza" ("does not
    recharge") for "has no cooldown"; Czech "se přehání" (said of weather) for the beam's sweep; "fading" as a picture
    fades in de, hu, pt-BR (the bonus decays); Italian "derivano" (derive) for drift; Japanese スネア (a snare drum) for the
    trap: くくり罠; Chinese 第二份工作 (a real second job) for the second job: 职业 / 職業, the game's word.
  - **F56**: Traditional Chinese Critical Mass 臨界品質 ("critical quality"), now 臨界質量; and 幾率 (the Simplified 几率
    converted) for chance in 7 lines, now 機率 as its crit chance upgrade.
  - **F55, Czech and Polish**: Exterminator "Deratizátor" / "Deratyzator" (a rat exterminator), now "Likvidátor" /
    "Eksterminator".
  - Grammar: a case suffix on a number placeholder, which must follow the number's own vowels (Turkish "%{v}'ini", 3
    lines; Hungarian "x{v}-szereséért"): rephrased so the suffix sits on a fixed word; "(s)" plurals that are wrong for 1
    in cs, pl, ru, uk ("kartu/y", "карт(ы)"): the count after a colon, as those languages do elsewhere; the rooftops biome
    without its article or case in de, fr, es, es-419, it, pt-BR, pt-PT, nl ("bei Stadtdächer", "à Toits de la ville");
    German "pro ... genommener Karte" (pro takes the accusative); Danish "Ikke den" for a neuter card; Korean enemies
    without the counter 명, as the same skills' blurbs have it.
  - One address form: Ukrainian (ти) had four formal verbs ("покличте", "заробіть", "Натисніть"); Hungarian "mentsen" (3rd
    person) for "save".
  - Half-translated or unidiomatic: "Full ház" / "Full ev" (Full House) now "Teli ház" / "Ev dolusu"; Danish "Sidste stand"
    now "Sidste skanse"; Danish / Swedish "every 6th s" as "Hvert 6. s" / "Var 6:e s" now "sekund" written out (11 lines);
    Polish "na stać" (not Polish) in the tree's legend; pt-PT "Cocô" (Brazilian) now "Cocó" as its own skill name (6
    lines); pt-PT "sobrepor" (superimpose) for overwrite; pt-BR "Toque" (touch) for press; "por você / por ti" (by you)
    for "for you"; Turkish "boss" left in English; Chinese "Reawaken progress" with the numbers first and a dangling
    clause.
- **The gate**: `loc_lint`'s mouse rule knew no cs / pl exterminator word, and its English test matched "rat" anywhere,
  so every English line with "rate" (spawn rate, frame rate) or "crater", and every line with a `{mice}` count, was
  skipped (16 lines, in every language; all clean once checked). Now whole words, on the English without its placeholders; the self-test plants a fault on such a line and on
  "Deratizátor" (shown MISSED with the old rule put back, CAUGHT with the new). F56 stays fixed (FIXED).

## 2g. Built 2026-11-23 (HANDOFF-LOG (135)): a language changed while a screen is up

- **The question** ((128)): text a screen makes a string once keeps the old language until the screen rebuilds (Godot
  rebuilds every code-built string on `SettingsManager.language_changed`). String-table text, formatted text and joined
  text follow a culture change by themselves (the engine invalidates every widget on the change; KB U/20 §4).
- **The test, `Pawpocalypse.UI.Language.Switch`** (everyday, in `Pawpocalypse.UI`; ~2 min): the language pass's tour of
  every screen (`TourScreens`, now with hooks) three times with the same steps: built in English and switched to German
  at each screen and back; built in German and switched to English and back; built in English with German chosen before
  the first screen. Every look must hold the words of the same screen built fresh in its language (a multiset: the texts
  of the laid-out Slate tree, combo boxes and Slate-built keycaps included, and the tooltips;
  `PawLayout::ArrangeWords`). The switch is the game's: the culture, then `UPawSettingsSubsystem::SetLanguage`.
  `Saved/Layout/language_switch.txt` lists every difference.
- **Found and fixed: the Settings dialog's lists.** A combo box holds its options as strings: Language ("System"),
  Quality, Max FPS ("Unlimited"), Prompts, Dock side and Output ("Follow the system") kept the old language after a
  change made outside the dialog. Not a bug a player could see today (only the dialog's own list changes the language
  while it is up, and its pick filled every list again), but the dialog now hears the change as Godot's lists do
  (`OnSettingsChanged`: every list again when the language setting or the language shown changed; a slider's change
  leaves them alone), and its pick no longer fills them itself.
- **Everything else already followed**: the title (all its layers and the language list), every dock page, the card
  pick, the skill tree and its tooltips, the spend step, the pause menu, the three settings pages and the Controls panel,
  the Reawaken ceremony, the job picker, the victory screen, the HUD in a boss fight (the boss panel: its own short tour
  in the test, raised as `Layout.Boss` raises it).
- **The test's own blind spots, found by its first runs**: reading only UMG text widgets missed the combo boxes (Slate
  text); counting only parts in view gave 1,100 false differences on the tree (its map pans as it ticks); choosing the
  tooltips "longest in this language" picked other nodes per language. All three fixed in the test.
- **Sabotages** (`tools/sabotage_realm.py`, `switch_*`), all CAUGHT: the dialog deaf to the change, a prompt line deaf to
  it (`UPawPromptText` makes its template a string with glyph markup), a tree caption made a string when the tree is
  built, every `ST_UI` line made a string. Two first candidates MISSED and were replaced, for reasons that are not holes:
  the title's own handler (with it off the title's words still follow: its hint is a prompt line, its status line
  string-table text; what it alone remakes is the language list's plate and tick, held by `Title.Language`), and a dock
  page title's `SetText` (it runs only at the bake: the Widget Blueprint holds the line, a string-table reference).

## 2h. Built 2026-11-23 (HANDOFF-LOG (136)): every language in the package (step 3's packaging gate)

- **The check, `py tools/packaged_languages.py`** (L5, on a package made when the designer asks): `paw.Languages`
  (`Source/Pawpocalypse/Dev/PawLanguages.cpp`, development builds only) in the packaged Development build, on the title,
  `-NoSave`, twice. Run 1 starts the game with `-culture=ru`: the language applied at start is Russian and the engine shows
  it. Run 2 opens the title's language list as a player does and picks every language in it (`UPawMainMenu::PressLanguage`):
  the list must offer every culture with a translation file (an incomplete one drops out: complete only, as Godot), and each
  pick must be complete (no line missing or left out), current in the engine, every line of `ST_UI`, `ST_Data` and `ST_Tree`
  showing its translation (851 lines), every character in the face `F_Paw` picks for it (no fallback, no empty box), and
  Thai, Japanese and Chinese must break a line inside their words. A screenshot per language (the title with its list open)
  and `build/loc/package/sheet.png`; reports and logs in `build/loc/package/`.
- **Found: a package could show only 18 of the 23 languages.** The engine's default `InternationalizationPreset=English`
  stages English ICU data alone: zh-Hans, zh-Hant, pt-BR, pt-PT and es-419 were refused by the engine (picked, the ticked row
  moved and every word stayed in the language before: 748 to 823 lines each), and Thai never wrapped inside a sentence (no
  break dictionary: 0 breaks in its longest line, 35 with it). The editor has the engine's full ICU data, so no editor test
  could see it. **Fixed**: `InternationalizationPreset=All` (`Config/DefaultGame.ini`; ~23 MB more on disk, the only
  preset with Russian, Polish and Thai), and the game logs a culture the engine refuses (`ApplyLanguage`).
- **PASS on the package made with it: all 23 languages** (Thai 35 breaks; Chinese 8; Japanese 13; 0 characters without a
  glyph in any language). Seen to fail: the English preset (above), and a package made with two planted faults
  (`safe_mutate`): a character no face has in a German line (U+0F00: German FAIL, 1 character with no glyph) and an emptied
  French line (French not offered: FAIL). L0 `packaging` keeps the preset and the staged folder in the ini between packages
  (three plants caught).
- Seen: the title and its language list in every language, from the package (`build/loc/package/sheet.png`). The designer,
  2026-11-23: "The text is good." The shrunk headlines (§2d) are not in that sheet.
- Open: the Shipping configuration was not packaged (the ICU data is staged the same way).

## 3. Order

1. **Carry Godot's translations over** (`tools/port_po.py`, doc 08 §4): gather the string tables headless, export PO per
   culture, fill each from Godot's `.po` by the English source string, import, compile `.locres`. Unchanged lines (about
   530 of 807) arrive translated in all 20 languages.
2. **The port's own lines**: the ~280 lines the plain-text pass changed (D15) and every string new to the port (the
   Sticker Box restyle's words among them): translated per language with the game's glossary
   (`build/steam/research/glossary.tsv`), the same terms as the existing translations; each needs a native read before a
   language ships.
3. **`tools/loc_status.py` and its gate**: every shipped culture 100 % (or a reviewed exception list), placeholders
   identical, stale lines listed. DONE: `loc_status` (§2c); the packaging gate for the shipped languages (§2h, 2026-11-23).
4. **The language menu** (the title's Language button waits for it), the pseudo-locale tour at 1280 x 800 and 150 % UI,
   font coverage (already a gate). DONE 2026-11-20: the menu (§2b), the tour as a test in every language and the
   pseudo-locale (§2d).
5. **The store**: each language listed once its game text and store page are both done and read.
