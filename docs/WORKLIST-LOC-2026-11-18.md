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
- Still to build: an `--apply` that writes a translated work file into the culture's files, stamping the Source the
  translator SAW (KB Unreal/20 §3); the title's language menu.
- **The run-time route (built 2026-11-20, HANDOFF-LOG (128)):** the translation files registered as polyglot text at
  start (`PawText::RegisterTranslations`, KB Unreal/20 §3: no `.locres`, no Dashboard round trip, the language changes
  live), a row only when its Source is today's English (a stale line shows the English until it is translated again).
  Doc 08 §4 planned `.locres` through PO files; the files serve either route.

## 2b. Built 2026-11-20

- **The languages' rules** (`PcLocales`, port of `locales.gd`, fixture `locales`: available, from_system, from_steam, the
  picker's labels with each language's own "(draft)" from Godot's .po files): the setting keeps Godot's codes
  (`es_MX`, `pt`, `zh_Hans`), the game applies Unreal's culture for each. Dutch and Danish are in the table after
  Swedish and appear once they have translations. F52 (doc 01 §5) fixed: Portugal no longer gets Brazilian Portuguese,
  Spanish outside Spain gets the Latin American translation, Macau gets Traditional Chinese.
- **Shown at start and on change** (`UPawSettingsSubsystem::ApplyLanguage`): the setting resolved (system: the OS
  language), a `-culture=` on the command line wins (the tour, the shots); never in the editor.
- **Settings > Video > Language**: "System", then every COMPLETE language in its own name with "(draft)" in its own
  words (`SETTINGS_language_draft`, Godot's "(draft)" carried with its 20 translations); a pick saves and shows it at
  once. Complete = no line missing (`PawText::TranslationCount(...).Missing`, counted as `loc_status.py` counts: 838
  lines need words). Godot lists only loaded `.po` files "so an unfinished language cannot be chosen by accident", and
  its files were complete; today no language is (320 lines each to translate), so the list is System and English.
  **`-PawDraftLanguages`** on the command line lists the unfinished ones too, for review: their missing lines show in
  English.
- The loader also leaves out a row whose `{placeholders}` differ from the English (as `loc_status.py` counts it broken).
- The title's Language button stays disabled until its menu is built (next step; the concept draws only the button).
- In the editor, game text follows a culture only with `Localization.ForceLoadGameLocalizationInEditor` on (the
  preview needs compiled game localisation, which this game does not have): `PawText::ShowCulture` sets it.

## 3. Order

1. **Carry Godot's translations over** (`tools/port_po.py`, doc 08 §4): gather the string tables headless, export PO per
   culture, fill each from Godot's `.po` by the English source string, import, compile `.locres`. Unchanged lines (about
   530 of 807) arrive translated in all 20 languages.
2. **The port's own lines**: the ~280 lines the plain-text pass changed (D15) and every string new to the port (the
   Sticker Box restyle's words among them): translated per language with the game's glossary
   (`build/steam/research/glossary.tsv`), the same terms as the existing translations; each needs a native read before a
   language ships.
3. **`tools/loc_status.py` and its gate**: every shipped culture 100 % (or a reviewed exception list), placeholders
   identical, stale lines listed.
4. **The language menu** (the title's Language button waits for it), the pseudo-locale tour at 1280 x 800 and 150 % UI,
   font coverage (already a gate).
5. **The store**: each language listed once its game text and store page are both done and read.
