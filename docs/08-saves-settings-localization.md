# 08 - Saves, settings and localisation

> KB: `Unreal/10-save-systems.md`, `Unreal/20-localization-text.md`, `Unreal/03` §6 (config), `Godot/06` (the gettext
> pipeline this game uses today). Godot: `save_manager.gd` (516 lines), `pawpocalypse_save_data.gd`, `settings_manager.gd`,
> `tools/loc.py`, `docs/pawpocalypse-localization-bible.md` (629 lines: read it before writing any string).

## 1. The save: what it holds

**Built 2026-11-03 (phase 4 slice 1, HANDOFF-LOG (31)): `PcSave` (PawCore) is Godot's `PawpocalypseSaveData` (schema v7) field for
field, and `CaptureSave` / `ApplySave` are `save_game()` / `load_game()`, proven against the REAL save_manager.gd (fixture `save`,
3,000 random states saved and 3,000 random saves loaded over random live states; PcSaveTests).**

| Group | Fields (Godot's names, which are also the JSON keys) |
|---|---|
| Schema | `version` (7: Godot's own number, so a Godot save imports 1:1) |
| Currencies | `catnip`, `stardust` |
| Progress | `reawaken_count`, `upgrade_levels` {id: int} (EVERY known id, zeros included: a load only overwrites the ids it finds), `skill_branch_levels`, `bosses_defeated` {biome: true}, `bosses_cleared_cycle`, `final_boss_defeated`, `final_boss_seconds` |
| Lifetime | `lifetime_catnip_ever`, `lifetime_catnip_this_cycle`, `stardust_granted_ever`, `lifetime_kills`, `tier_kill_counts` {biome: int} (the live biome banked in first), `bait_spent` |
| Achievements | `achievements_earned`, `achievements_claimed` |
| Clocks | `last_played_unix`, `play_seconds_this_cycle`, `total_play_seconds` |
| Jobs | `jobs_held`, `current_job`, `run_job`, `run_job_chosen`, `run_job_second` (Union Rules' second job: F23, 2026-11-03; "" by default, no version bump) |
| Per-slot toggles | `auto_travel`, `auto_pick_card` (in the SAVE, not settings: a new game starts with both off; `UPawGameSubsystem::Slot()`) |
| Migrations / flags | `unicorn_split`, `tutorial_done` {step: true} |
| The run's cards (F24, 2026-11-03) | `run_cards_saved`, `cards_owned` {card: level}, `cards_banished`, `catches_level`, `catches_points`, `rerolls`, `extra_choices`, `banishes`, `fixation_hands`, `hand` [ids], `promised` [ids], `prototype_card`, `cards_taken_this_run`: replaced whole on load (unknown ids dropped; the Prototype mark only on a card in the hand); a save without `run_cards_saved` leaves memory's; a restored hand is up again at once (the HUD opens the pick on any pending hand; Godot: `reoffer_after_load`) |
| Where the run stands (F26-F29, 2026-11-04) | `deepest_biome_index` (the per-floor caps; clamped to the biomes), `pending_second_helping` (Second Helping's deferred Stardust), `kills_this_cycle` (Body Count), `kills_since_tithe` (Levy), `reawaken_in_progress` (a save made between a Reawaken and the run's arrival in its starting biome resumes at the spend step: the HUD opens it once nothing else is up, then the job picker and the travel) |

**NOT saved, exactly as in Godot**: the live biome (a load forgets it: the realm restores the counter on arrival), the wave
counters, Reputation (`waves_here`) and Infestation (run-scoped on purpose). A load leaves those as they were in
memory; the title's New Game resets everything first. (Until 2026-11-03 the run's cards were not saved either; the designer
decided they are, F24, fixed in Godot first. Until 2026-11-04 neither were the deepest biome, Second Helping's Stardust,
Body Count's and Levy's counts or a Reawaken in progress: F26-F29, the same way.)

**A damaged save loads safely** (F32-F33, 2026-11-04, Godot first): every number that is not finite reads as 0, a skill level
is clamped to 0..max and a shop level to >= 0, the run's counters to >= 0.

**The load, Godot's order** (it matters: the achievement re-scan runs before the jobs and per-biome maps are restored): the
currencies and ledger and the KNOWN upgrade ids; kills and clocks; the tree (v6 shape migration, Unicorn split; every known
branch takes the saved level or 0); the achievements (replaced, re-scanned); the toggles; the per-biome maps; the biome
forgotten; the jobs (`current_job` through `take_job`, so one not held is refused; `run_job` seeded from it on an old save).
A save older than v7 takes Godot's reset path (Catnip and upgrade levels kept, everything else of the progression reset, a save
written at once).

**Rule from the Godot history**: `debug_reset_progress()` / New Game must clear EVERYTHING the save carries (it once left jobs,
kills, play clock and run state from the slot the title screen had loaded). `UPawGameSubsystem::NewGame()` clears the progress
and the slot's toggles and tutorial.

## 2. The container (decided, D8; built 2026-11-03)

- **The file** (`PcSave` WriteSaveFile / ReadSaveFile): one header line `PAWSAVE <format> <crc32> <body length>`, then the body
  as JSON with Godot's key names. The format is the CONTAINER's version (1). Every double is written as the shortest text that
  reads back bit for bit (std::to_chars), integers exactly; unknown keys ignored, missing keys defaulted (Godot's @export); a
  wrong type, a bad checksum, a cut-short body or a newer format is refused whole, never half-read. **Godot's own .tres is
  lossy** (16 significant digits, and a double that is exactly a float32 is written with float32's digits: 66126.65625 ->
  "66126.66"); the port is not, so the fixture compares Godot's written doubles within 1e-7 (KB Godot gotchas).
- **Storage** (`IPawSaveStorage`): `FPawPlatformSaveStorage` over `ISaveGameSystem` (consoles cost nothing later; on Windows
  files under Saved/SaveGames), `FPawFolderSaveStorage` for `-PawSaveDir=<folder>`, `FPawMemorySaveStorage` for the tests (with
  write failures and cut-short writes on demand). Entries: `slot_N`, `slot_N_tmp`, `slot_N_bak`, `slot_N_corrupt` (N = 0..4).
- **The atomic write** (`ISaveGameSystem` has no rename, so not Godot's two renames): on a worker, one write at a time
  (`UE::Tasks::FPipe`): write the temp, read it back and compare, copy the current slot to the backup ONLY if it reads (Godot's
  rename put a broken slot over the good backup), write the slot, delete the temp. A slot holding a newer build's save is never
  overwritten. **Startup recovery**: a slot that is missing or will not read is kept aside as `_corrupt`, then finished from the
  verified temp, else the backup; a good slot is never replaced (a stray temp is deleted). **A load** of a slot that will not read
  falls back to its backup (`LoadedFromBackup`, the broken file kept); both broken: nothing applied (`Corrupt`). Never crash on
  load `[KB U/10 §1]`.
- **Five slots**; the summary (`ReadSlotSummary`, Godot's slot_summary) reads the ~3 KB save itself, as Godot does (no separate
  meta entry). **The last slot used** belongs to the settings (SettingsManager.last_slot, built 2026-11-04 (40)): the saves start on
  it.
- **When it saves** (Godot timings): every 30 s of unpaused play (`Autosave`, skipped with nothing to keep: an idle fresh install
  never writes an empty slot); on focus loss and suspend; on quit (`Deinitialize`, synchronously); and at the end of a frame in
  which a save-worthy thing happened, ONE write however many: Godot's signals (upgrade_purchased, branch_leveled_up,
  boss_defeated, job_taken, achievement_claimed, card_taken, reawakened) are replaced by a digest of exactly what they report,
  compared once a frame (a bulk buy of 54 levels is one write; auto-picked cards and cheats are covered by construction).
  `RequestSave()` coalesces the same way; `SaveGame(slot)` writes now and makes that slot active. Startup loads the active slot
  (Godot's SaveManager._ready), before the realm starts.
- **Only completed transactions are saved** `[KB U/10 §3]`: a hand on screen is saved WITH its contents (and Blueprint's promise,
  Prototype's mark) and comes back up on load, so reloading is not a reroll (F24); a boss fight is not saved (Godot saves none).
- **Versioning**: the body's `version` is Godot's schema (7); a schema below it takes Godot's reset path. A change to the file
  layout bumps the container format, with a migration and a test loading a stored file of the previous format `[KB U/10 §2]`.
- **Settings never in the save** (per device, §3). **Steam Auto-Cloud** on the save folder later (no code) `[KB S/03]`.
- **Sandbox** (docs/09 §1 rule 4): `-NoSave` turns saving off; `-PawSaveDir=<folder>` keeps the slots there; an automation run or
  an `-unattended` run with neither never reads or writes the real slots (`Pawpocalypse.Save.Sandbox`); the packaged bench and the
  screenshot tour pass `-NoSave`. Tests hand in a memory storage (`UseStorageForTest`).

### 2.1 Importing itch saves
**Decided 2026-11-03 (designer, Q3): no.** The Unreal build does not import Godot saves.

## 3. Settings (per device)

**As built, 2026-11-04 (HANDOFF-LOG (40))**: Godot is the specification, so the settings are Godot's SettingsManager, not the
list planned below. PawCore `PcSettings` holds them (every field of settings_manager.gd, its defaults and its load-time
clamps) and reads and writes **Godot's settings.cfg text** (ConfigFile: `[audio]`, `master_volume=1.0`, ...), proven against
files Godot wrote (fixture `settings`). Numbers are read the way Godot reads them (its `String::to_float` is not correctly
rounded for long mantissas: "0.9500000000000001" reads 0.95); the writer picks a spelling that reads back exactly (every
value the game holds does; an arbitrary double comes back within two units in the last place). `UPawSettingsSubsystem`
(game instance) keeps the file as the entry `settings` of the SAME storage as the slots (Godot: user://settings.cfg beside
them), so the sandbox is the slots' (`PawStorageForThisRun`: -NoSave, -PawSaveDir=, off in automated runs). It is the one
writer (each setter stores with Godot's clamp, writes the whole file, applies). Applied: the window (fitted to the work area
and centred, or borderless fullscreen; `FSystemResolution::RequestResolutionChange`), `r.VSync`, `t.MaxFPS`, one frame after
startup and on change, never in the editor, never in a run with no settings storage (the bench and the tour keep their
command line), the window never when the command line names one; the menu side (the HUD listens), ask before Reawakening
(the HUD's box), the effects budget (the realm's bursts), the last slot used (the saves start on it; saving into or loading a
slot moves it). Since phase 5, read too: the stick dead zone (slice 3), the button prompt style and Show button prompts (slice 3b),
Invert X / Y and the cursor speed (the cat and the tree's pan), and since slice 4 (2026-11-05) **the remapping: Godot's own
`[bindings]`**, not Enhanced Input's user settings (settled for Godot, the specification: a row moves several actions and a swap
touches two rows, which per-key "player mappable" slots cannot hold; and a Godot player's settings.cfg reads the same). The
section is read and written as Godot's ConfigFile does (`PcSettings::Bindings`, a list of small dictionaries per action that
differs from the defaults; Godot writes their keys sorted; a value that is not a list is dropped on load), proven on 1,500
files Godot wrote, hand edits included (fixture `input_bindings`). `UPawInputDeviceSubsystem` applies the bindings, Swap sticks,
Swap confirm and back and the dead zone when one of them changes (`PawInput::ApplyBindings`, the live mapping context; docs/05
§3). The Controls page's setters: `SetBindings`, `SetSwapConfirmBack`, `SetSwapSticks`, `SetInvertX/Y`, `SetCursorSpeed`,
`SetStickDeadzone`, `SetTutorialTips`, `ResetControls` (Godot's set_control: an equal value changes nothing). Nothing writes
Enhanced Input's `EnhancedInputUserSettings.sav`. Stored only until their phase: the language (7), the tutorial tips (6). The
volumes and mute act since phase 6 slice 4 (`UPawAudioSubsystem` reads them live); Output since 2026-11-05 (the system's
devices in the list, the stored one played to while it is there, else the system's; doc 07 §6). Not built from the plan below (not in Godot): UI volume, mute when unfocused, unfocused frame cap, UI
scale, window mode beyond windowed / fullscreen.

The plan, as written at the start of the port:

`UPawGameUserSettings : UGameUserSettings` (writes `GameUserSettings.ini`, per machine, never cloud):
- Audio: master, music, SFX, UI volumes; mute all; mute when unfocused.
- Video: window mode, resolution (window size; clamp to desktop; never offer sizes above it `[KB U/09 §6]`), vsync, frame cap
  (30/60/120/unlimited), unfocused frame cap, effects quality (full/reduced/off), UI scale (90-150%).
- Controls: menu side (left/right), button prompts (Auto/Xbox/PlayStation/Steam Deck), tutorial tips on/off, ask before
  Reawakening, remapping (Enhanced Input user settings save; tests reset it afterwards `[KB U/08 §4]`).
- Language (culture), defaulting to the OS culture if supported, else English.
- **Written only through this class** (Godot: `save_settings()` rewrote the file whole; a second writer lost changes).

## 4. Localisation (D9)

**Today (Godot)**: gettext. `locale/messages.pot` + 21 `.po` files (cs, de, es, es_MX, fr, hu, it, ja, ko, pl, pt, pt_BR, ru, sv,
th, tr, uk, vi, zh_Hans, zh_Hant, and `xa` the pseudo-locale), **807 msgids, 429 with `msgctxt`, 5 plurals**, `msgid` = the English
source string, named placeholders `{name}` filled by `.format()`, `tr_n` for counts, action tokens like `{pounce}` resolved to glyphs.
All 20 real languages at 100%.

**Unreal**: FText everywhere, **string tables** keyed by stable keys, English as source, translations compiled to `.locres`.

**Live since 2026-11-01** (phase 3's HUD): `Content/Pawpocalypse/Localization/Strings/ST_UI.csv` (hand-written, Godot's English
as the source, a comment per row) and `ST_Data.csv` (generated by `tools/gen_data_strings.py` from the `display_name` of
`Data/UpgradeData.csv`, `TierData.csv`, `BossData.csv`: `UPG_<id>_name`, `TIER_<id>_name`, `BOSS_<id>_name`), registered by
the game module's `StartupModule` (`PawText::RegisterTables`, `FStringTableRegistry::Internal_LocTableFromFile` relative to the
Content folder) and staged loose (`+DirectoriesToAlwaysStageAsUFS=(Path="Pawpocalypse/Localization")`; the packaged build
shows the text). Accessors `PawText::Ui(Key[, Args])`, `PawText::Data(Prefix, Id)`; numbers through `PawText::Amount`
(`Amounts.short`, culture-invariant until its suffixes join a table) and `PawText::Count` / `TwoDigits` / `Fixed` (Godot's `str()`, `"%02d"`, `"%.Nf"`: ungrouped, a '.' point in every
language; designer 2026-11-02, "match Godot"). L0 gate `ui-rules`: no hard-coded text in the UI code, no `FText::AsNumber`, every key it names exists, every `ST_UI` key is named. A quoted CSV value may
span lines (`SHOP_kitten_power_locked`, tested); a written `\n` is a line break too (the import unescapes it,
`ReplaceEscapedCharWithChar`, StringTableCore.cpp 5.8).

**The port's own English (2026-11-09).** The designer's text pass (plain, informative, real numbers, one word per idea;
CLAUDE.md) changed 279 English lines. `ST_UI` holds its own; the generated tables take theirs from `Data/TextOverrides.csv`
(`table, key, text, godot, note`; `tools/text_overrides.py`): `gen_data_strings.py` and `gen_tree_tables.py` write `text` and
fail when a key is not generated, Godot's English is no longer `godot` (review the line), or the `{tokens}` differ.
Godot's words are not changed: its `loc` gate needs every English string translated in every shipped language. The tree
text test (`PcTreeTextTests`) still compares against Godot's wording through the `godot` column. **For `port_po.py`**: a
changed line has no Godot translation to match (its `godot` column is the msgid that has one): those lines go to
translators.

| Godot | Unreal |
|---|---|
| `tr("Catch 1 mice in total.")` | `PAW_TEXT("ACH", "kills_1_requirement")` -> `FText::FromStringTable("ST_Achievements", "kills_1_requirement")` |
| `msgctxt` for short ambiguous words | a distinct key per meaning (string table keys are already unique) |
| `.format({"n": x})` | `FText::Format(Pattern, FFormatNamedArguments{{TEXT("n"), PawText::Count(x)}})` (Godot prints the int ungrouped). **Argument names are `FString`, not FName** (FName case flips in packaged builds and the placeholder shows raw) `[KB U/05 §3]` |
| `tr_n(singular, plural, n)` | ICU plural in ONE pattern: `"{n}|plural(one=Reawaken,other=Reawakens)"`, per language forms (Polish, Russian, Czech, Ukrainian need `few`/`many`) |
| `{pounce}` action tokens | kept as literal tokens in the English source; the prompt rich-text decorator replaces them (doc 05 §5). Translators keep them untouched (the bible's rule) |
| `xa` pseudo-locale | Unreal's built-in pseudo-localisation (`-culture=` + the Localization Dashboard preview) **plus** the existing `xa.po` carried as an extra culture for the tour, because it is what the Godot tests compared against |
| `# TRANSLATABLE: ctx` dictionaries, `# NO_TRANSLATE` | string-table rows; `FText::AsCultureInvariant` for the exceptions ("Pawpocalypse" and "Boba" are never translated) |
| `tools/loc.py extract`, the `loc` gate (every `.text =` through `tr()`, `.pot` current) | a checker script: no `FText::FromString` on player text, no `LOCTEXT` literals outside string tables, every key used exists, every table key is used `[KB U/20 §2]` |

**Carrying the 20 translations over (phase 7, `tools/port_po.py`)**:
1. Author English string tables as CSV (`Content/Pawpocalypse/Localization/Strings/ST_*.csv`, one per area: UI, ITM, CARD, SKILL,
   JOB, ACH, TUT, ERR) with stable keys. Data-driven text (cards, nodes, jobs, achievements, upgrades, biomes, enemies, bosses)
   gets keys by convention from ids (`CARD_bloodlust_name`, `CARD_bloodlust_desc`), so data rows carry no label columns at runtime.
2. Gather text in the Localization Dashboard (or the `GatherText` commandlet headless), export PO per culture.
3. `port_po.py` fills each exported Unreal `.po` (msgctxt = namespace,key; msgid = English) by matching the **English source
   string** (plus the Godot msgctxt where the Godot file has one) against the Godot `.po` for the same language, converting
   `tr_n` pairs into ICU plural patterns and logging every string it could not match (new or changed English) for translation.
4. Import the filled PO through the Dashboard (import from the project's own Localization folder: importing from a copied folder of
   only `.po` files is reported to fail `[Forum]`), compile `.locres`, commit.
5. Gate: every culture 100% (or a reviewed exception list), no placeholder lost or renamed (compare `{...}` sets per string),
   pseudo-locale tour at the smallest resolution and largest UI scale finds no overflow `[KB U/20 §2]`.

- **Unreal culture codes** differ from Godot's: `zh_Hans` -> `zh-Hans`, `zh_Hant` -> `zh-Hant`, `pt_BR` -> `pt-BR`, `es_MX` ->
  `es-MX`. Map them once in `port_po.py` and in the language menu.
- Numbers: Godot prints them culture-invariant (no grouping, '.' point), so the port does too (`PawText::Count` / `Fixed`;
  `FText::AsNumber` would group and localise them: gated out of the UI code). The abbreviated big-number format (`Amounts.short`: K, M, B, then scientific) is a
  game rule in PawCore, its suffixes are translatable strings.
- String tables are loose CSVs: register them at startup and **stage the folder** (`DirectoriesToAlwaysStageAsUFS`), or they are
  missing in packaged builds; an empty string-table CSV fails to import (needs the header and one row) `[KB U/20 §2]`.
- Fonts per culture: doc 07 §5.
- **Store and Steam text**: achievement names per language for the Steamworks panel are generated from the string tables (the Godot
  plan did it from `.po`).
