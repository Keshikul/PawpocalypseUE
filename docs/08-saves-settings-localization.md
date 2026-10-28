# 08 - Saves, settings and localisation

> KB: `Unreal/10-save-systems.md`, `Unreal/20-localization-text.md`, `Unreal/03` §6 (config), `Godot/06` (the gettext
> pipeline this game uses today). Godot: `save_manager.gd` (516 lines), `pawpocalypse_save_data.gd`, `settings_manager.gd`,
> `tools/loc.py`, `docs/pawpocalypse-localization-bible.md` (629 lines: read it before writing any string).

## 1. The save: what it holds

The Godot save (`PawpocalypseSaveData`) is flat. Ported field for field into `FPawSaveState` (PawCore) -> JSON:

| Group | Fields |
|---|---|
| Header | `magic` "PAWSAVE", `version`, `crc32` of the body, `written_unix`, `game_build` |
| Currencies | `catnip`, `stardust` (double) |
| Progress | `reawaken_count`, `upgrade_levels` {id: int}, `skill_branch_levels` {id: int}, `bosses_defeated` {biome: bool}, `bosses_cleared_cycle`, `final_boss_defeated`, `final_boss_seconds` |
| Lifetime | `lifetime_catnip_ever`, `lifetime_catnip_this_cycle`, `stardust_granted_ever`, `lifetime_kills`, `tier_kill_counts` {biome: int}, `bait_spent` |
| Achievements | `achievements_earned`, `achievements_claimed` |
| Clocks | `last_played_unix`, `play_seconds_this_cycle`, `total_play_seconds` |
| Run state | current biome, waves per biome, bait held, XP/level, owned cards and levels (per-run), `run_job`, `run_job_chosen`, `current_job`, `jobs_held` |
| Per-slot toggles | `auto_travel`, `auto_pick_card` (in the SAVE, not settings: a new game starts with both off) |
| Migrations/flags | `unicorn_split`, `tutorial_done` {step: bool} |

**Rule from the Godot history**: `debug_reset_progress()` / New Game must clear EVERYTHING the save carries (it once left jobs,
kills, play clock and run state from the slot the title screen had loaded). A test fills every field, starts a new game, and
asserts every field is back to its default.

## 2. The container (decided, D8)

- **JSON text** (diffable, like the Godot `.tres`), written by PawCore's serialiser (every field; unknown keys ignored; missing keys
  defaulted), `int64` and `double` exact (doubles written with 17 significant digits, round-trip tested).
- **Atomic write** exactly as Godot does it and as the KB recommends: write `slot_N.tmp.json`, verify (re-read + CRC), rename the
  old slot to `slot_N.bak.json`, rename the temp into place. A slot that will not load (bad CRC, bad JSON) falls back to its
  backup and the player is told; the broken file is moved aside as `.corrupt`. A crash between renames is finished at startup.
  Never crash on load `[KB U/10 §1]`.
- **Location**: `FPaths::ProjectSavedDir()/SaveGames/` via `IPlatformFile`, or through `ISaveGameSystem` with a byte array so a
  console port swaps the backend (`[KB P/01]`: save data via the platform service). **Decision: go through `ISaveGameSystem`**
  (`UGameplayStatics::SaveDataToSlot`/`LoadDataFromSlot` take raw bytes) so consoles cost nothing later; implement temp/backup
  on top with slot names `slot_N`, `slot_N_tmp`, `slot_N_bak`.
- **Five slots**, slot metadata (biome, Reawakens, mice caught, play time, last played) readable without loading the whole game
  (a small `slot_N_meta` entry written with each save) `[KB U/10 §1]`.
- **When it saves** (Godot timings kept): autosave every 30 s; on focus loss / suspend; on close (handle the window close request,
  save, then quit; code paths that quit directly save first); on events via `RequestSave()` which coalesces to ONE write at the
  end of the frame (a bulk buy emits one event per level: a write per level froze an xMax press 137 ms; coalesced 4.8 ms). Reawaken,
  quit and the pause menu's Save write immediately. Skip autosave for a game with no progress (an idle title screen never writes an
  empty slot).
- **Writes off the game thread**: serialise on the game thread (small), write on a worker (`AsyncTask`), queue (never two writes at
  once), synchronous on exit `[KB U/10 §1]`.
- **Only completed transactions are saved** `[KB U/10 §3]`: a card hand on screen is saved as "hand pending" with its seeded
  contents so it cannot be re-rolled by reloading; a boss fight in progress is saved as "not engaged" (the bait is refunded or the
  fight restarts: designer decision, recommend refund); the Reawaken ceremony saves before it plays.
- **Versioning**: the Godot rule "a version bump is a progression WIPE" is replaced by real migrations (`v1 -> v2 -> ...`), each
  with a test that loads a stored file of the previous version. A data change that alters what a saved value means bumps the
  version in the same commit `[KB U/10 §2]`.
- **Settings never in the save** (per device, §3). **Steam Auto-Cloud** on the save folder later (no code) `[KB S/03]`.
- Dev flags: `-NoSave` (tests and tours never touch real slots), `-LoadSlot=N`, and every automated run uses a sandboxed save dir
  (the Godot project once committed a real Reawaken to the designer's save from a throwaway repro). Tests refuse to start
  unsandboxed.

### 2.1 Importing itch saves (if the designer says yes, Q3)
`UPawGodotSaveImporter`: reads `%APPDATA%/Godot/app_userdata/Pawpocalypse/save_slot_N.tres` (text resource), parses the flat
`key = value` lines (the exporter's parser handles the same syntax), maps fields 1:1 into `FPawSaveState`, writes a new slot, and
never modifies the Godot file. A test imports a fixture save from each Godot save version still in the wild.

## 3. Settings (per device)

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

| Godot | Unreal |
|---|---|
| `tr("Catch 1 mice in total.")` | `PAW_TEXT("ACH", "kills_1_requirement")` -> `FText::FromStringTable("ST_Achievements", "kills_1_requirement")` |
| `msgctxt` for short ambiguous words | a distinct key per meaning (string table keys are already unique) |
| `.format({"n": x})` | `FText::Format(Pattern, FFormatNamedArguments{{TEXT("n"), FText::AsNumber(x)}})`. **Argument names are `FString`, not FName** (FName case flips in packaged builds and the placeholder shows raw) `[KB U/05 §3]` |
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
- Numbers: `FText::AsNumber` is culture-aware; the abbreviated big-number format (`Amounts.short`: K, M, B, then scientific) is a
  game rule in PawCore, its suffixes are translatable strings.
- String tables are loose CSVs: register them at startup and **stage the folder** (`DirectoriesToAlwaysStageAsUFS`), or they are
  missing in packaged builds; an empty string-table CSV fails to import (needs the header and one row) `[KB U/20 §2]`.
- Fonts per culture: doc 07 §5.
- **Store and Steam text**: achievement names per language for the Steamworks panel are generated from the string tables (the Godot
  plan did it from `.po`).
