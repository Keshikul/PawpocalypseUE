# 10 - Build, release, Steam, itch, consoles

> KB: `Unreal/19-packaging-cooking-shipping.md`, `Unreal/22-source-control-git-lfs.md`, `Steam/01-03`, `Playstation/01`,
> `Godot/04` (the SteamPipe chunk lessons from this game). Godot: `docs/pawpocalypse-steam-export.md`,
> `docs/pawpocalypse-shipping-to-itch.md`, `docs/pawpocalypse-steam-launch-plan.md` (Next Fest Feb 2027, store page, demo,
> AI disclosure, business: still valid, engine-independent).

## 1. Repository and cooking

- **Git + LFS** (phase 0): `.gitignore` for `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`, `*.sln`, `*.slnx`,
  `.vs/`, `SourceArt/png/`, `build/`; LFS for `*.uasset`, `*.umap`, `*.png`, `*.wav`, `*.ogg`, `*.ttf`, `*.otf`, `*.ttc`
  `[KB U/22]`. Text stays text: C++, CSV, ini, Python, JSON, SVG, docs. Stage explicit paths, never `git add -A`. Run headless
  tools with `-SCCProvider=None` (the editor's git provider runs `git rm` on delete) `[KB U/17 §1]`.
- **Branches**: `master` only, plus a release branch when releases start (the Godot project's `itch-release` pattern: master
  merged into it, never the reverse). Never commit, push or export unless the designer asks in that message (Godot standing rule).
- **Cooking**: only assets reachable from `MapsToCook` and always-cook lists are packaged. **Everything shipped lives under
  `/Game/Pawpocalypse`** with `+DirectoriesToAlwaysCook=(Path="/Game/Pawpocalypse")`; Primary Asset types (visual definitions)
  registered with `CookRule=AlwaysCook`; maps `L_Title`, `L_Realm` (and test maps for packaged tests in Development only)
  **listed in `MapsToCook`**: `DirectoriesToAlwaysCook` did NOT cook the maps under it (UE 5.8.3, 2026-11-01: only the
  `GameDefaultMap` L_Title was cooked; the packaged L_Realm failed "does not exist on disk or in the loader"). Check a package
  with `UnrealPak <Pawpocalypse-Windows.utoc> -List`.
  String tables and hardware cursors staged as UFS `[KB U/19 §1]` (as built: `Pawpocalypse/Localization` and `Pawpocalypse/UI/Cursors`,
  the paw cursors' loose PNGs; the cursor in a packaged build is still to check).
- **Every language's ICU data: `InternationalizationPreset=All`** (2026-11-23, HANDOFF-LOG (136)). The engine's default,
  `English`, stages English ICU data alone: measured in a package, zh-Hans, zh-Hant, pt-BR, pt-PT and es-419 could not be
  chosen (picked, the words stayed in the language before) and Thai never wrapped inside a sentence (no break dictionary);
  the editor, with the engine's full data, showed none of it. `All` is the only preset with Russian, Polish and Thai: ~23 MB
  more on disk (ICU 64: 27 MB against 4.2). `CulturesToStage` stays the engine's `en`: the translations are polyglot text,
  not `.locres`, and every language showed its words without it. Gates: L0 `packaging` (the ini), L5
  `py tools/packaged_languages.py` (the package) `[KB U/20 §5]`.
- **Phase 6 content and the restyle, packaged once** (2026-11-10, HANDOFF-LOG (85), `build/package`): the field atlas (4 BC7
  pages with R5's rig layers), the 12 floors, 53 sounds (28.9 MB of OGG; the music looping SoundWaves), the fonts, the
  generated materials and the procedural meshes (the ProceduralMeshComponent plugin is enabled for the game target). All under
  `/Game/Pawpocalypse` and referenced by the realm view, `DA_FieldAtlas`, `DA_AudioBank` or loaded by path from there
  (`UPawGarden`, `UPawAudioSubsystem`). The bench drew every sprite (they cook); it FAILED on draw calls (doc 09 §7.3). Still
  to measure in a package: the size, the paw cursor (staged loose).
- `DefaultScalability.ini`: scalability cvars only (anything else stops the cook) `[KB U/12 §2]`.

## 2. Packaging script (`tools/package.py`)

**Written 2026-11-23 (HANDOFF-LOG (137)); first run the same day at the designer's word (HANDOFF-LOG (139)): Development 439 MB (exe 255.0, paks 121.0), audit PASS, `packaged_languages` PASS (23), `packaged_bench` FAIL on draw calls only (83.6, then 50.6 after the draw-call work); Shipping 262 MB (exe 119.6), audit PASS.** A Python tool, as every other
L5 tool (the plan said `package.ps1`). `py tools/package.py [--config Development|Shipping] [--no-gates] [--bump]`:
refuses while an editor of this project, a packaged copy or another UAT runs; DELETES the archive first (Development
`build/package`, Shipping `build/package_shipping`); `--bump` raises ProjectVersion's last number (the scheme: 0.MINOR.PATCH
from 0.1.0, the designer 2026-11-23, "Use minor type version numbers"; MINOR by hand; a package for the gates is not bumped); UAT `BuildCookRun` (the command below); the
archived exe must be this source's build; the `.pdb` and exe kept in `build/symbols/<stamp>-<config>/`; the archive TRIMMED
of the D3D12 / Vulkan debug layers and UAT's manifests (2026-11-24: 46.7 MB of a Development package, 5.4 of a Shipping
one; the audit fails if one is left); the audit; README
and `Send logs.bat` written into the archive; in Development the L5 gates (`packaged_languages.py`, `packaged_bench.py`,
`packaged_pso.py --route bench`: the PSO validation over one full field, 2026-11-24; its frame time reported, not
judged, since (144): the frame gate is the bench's own).
`--audit [ARCHIVE]` audits a package already made (read only; `build/package_audit_<config>.txt`: sizes, every archive
file, package and loose file). `--self-test` (everyday `check_all`, gate `package audit self-test`) plants each fault.
- **The audit** FAILs on: a `Saved` folder or a `.pdb` in the archive; a `/Game` package outside `/Game/Pawpocalypse`
  (the shader library at the content root allowed); Developers / Collections / Screenshots; a map other than L_Title and
  L_Realm (Shipping; Development only warns); either map missing; a string table, a culture's translation file or a paw
  cursor not staged; ICU data without Thai's dictionary or zh-Hant (the English preset, (136)); no exe. It WARNs, with the
  size, on what the engine stages that this game does not need.
- **First audit of the package made by hand (2026-11-23, `build/package`)**: 495.8 MB (exe 295.9 Development, paks 121.0).
  FAIL: two `Saved` folders, `Engine/Saved/Config/Windows/Manifest.ini` dated 2026-11-04 and the gates' own
  `Pawpocalypse/Saved`: **the archive was never cleaned between packages**, so files a newer package no longer makes stay
  (the archive's README was phase 4's, "English only, no audio yet"). The tool deletes the archive first. WARN: 31 files,
  48.8 MB, not needed: the D3D12 and Vulkan debug layers, and Boost + TBB DLLs. **The Boost and TBB DLLs come from USD**:
  the game exe links USDCore (`USDClasses`, `UnrealUSDWrapper`, `USDUtilities`) and USDImporter's runtime modules
  (`USDSchemas`, `GeometryCacheUSD`, `USDStage`) (`Intermediate/.../Pawpocalypse.exe.rsp`); the game uses no USD. **Why**:
  an engine update added FabLauncher (editor-only, enabled by default), which depends on USD; the trim had not run since.
  **Fixed the same day**: `py tools/trim_game_plugins.py` (FabLauncher restricted to the Editor target; nothing else
  changed): USD and Boost left the link and the receipt (`Binaries/Win64/Pawpocalypse.target`), the Development exe
  295.9 -> 254.9 MB. TBB stays (the engine links it). Gated: `trim_game_plugins.py --check` (everyday). The next package
  shows the archive's size.
- **Shipping compiles** (first time, 2026-11-23: `Build.bat Pawpocalypse Win64 Shipping`, 178 s cold, exe 119.6 MB);
  everyday `check_all` gate `UBT game build (Shipping)`, seen to fail on a planted Shipping-only `#error`. Not packaged
  yet.
- **`Send logs.bat`** (tested in a scratch archive, `PAW_LOGS_OUT`): this folder's and `%LOCALAPPDATA%`'s
  `Pawpocalypse\Saved` (logs, crashes, settings, saves) and `dxdiag /t`, zipped to the desktop. It calls
  `%SystemRoot%\System32\tar.exe` by path: a bare `tar` was Git's GNU tar, which read `C:` as a remote host and wrote nothing.

The UAT command (also by hand, ~1-2 min warm on the dev PC):
`RunUAT.bat BuildCookRun -project=<abs uproject> -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak
-archive -archivedirectory=<abs>/build/package -nodebuginfo -utf8output -unattended`. The
game target is what a package compiles: everyday `check_all` builds it too (gate `UBT game build`, since (41)), because
editor-only API in game-module code compiles in the editor and fails only here. Zip the archive WITHOUT
`Pawpocalypse/Saved` (the bench's and the tour's shots and logs land there when they run on the package).

The plan for the wrapper `[KB U/19 §2]` (as built above, except: no `-prereqs` option yet, no zip, no Shipping gate):
- `-Config Development|Shipping`, clean archive dir, no prereqs in the Steam depot (Steam installs the VC++ redist via its own
  install script; itch builds include `-prereqs`), `-nodebuginfo` with the `.pdb` copied to `Symbols/<time>-<config>/` locally.
- **Refuses to run while a packaged copy is running** (a locked exe makes UAT keep the OLD exe and report success) or another UAT
  runs.
- **Bumps the version itself** (one source: `Config/DefaultGame.ini [/Script/EngineSettings.GeneralProjectSettings] ProjectVersion`),
  two builds never share a version (Godot rule).
- Afterwards: lists what shipped (pak/IoStore listing via `UnrealPak -List` / `-ListContainer`), fails on never-ship content
  (`Developers/`, `Dev/` spike content, screenshots, test maps in Shipping), runs the L5 packaged gates (doc 09), writes a
  `Send logs` batch file and README into the archive `[KB U/19 §4]`.
- Shipping compiles regularly (every phase), because dev-only calls break it.

## 3. Patch size on Steam (verify in phase 8)

**Measured 2026-11-24 (HANDOFF-LOG (141), `tools/steam_delta.py`: 1 MiB chunks at fixed offsets, as SteamPipe cuts them)**:
two Development packages from the same source, the second with ONE balance number changed (Attack Speed's base cost
9 -> 10, `Data/UpgradeData.csv`, reimported): **1.20 MiB to send** (uncompressed; SteamPipe compresses chunks): one chunk
of `Pawpocalypse-Windows.ucas` (77.4 MiB) and its `.utoc` (0.2 MiB). Every other file byte-identical across the two
runs, the exe and the shader library included: the cook is deterministic and the IoStore container does not shift on a
small change, with the default settings (no padding alignment set, IoStore on, no chunking). Nothing to change for
now; re-measure after an engine upgrade or a big content change. The plan below stays the fallback.

- SteamPipe diffs files in ~1 MiB chunks at fixed offsets; inserted bytes shift everything after them `[KB G/04 §1]`.
- **Valve's SteamPipe docs: some Unreal versions pad assets inside pak files in a way that bloats updates; Valve suggests a
  padding alignment of 1 MiB (1048576)** via the pak parameters in the Windows platform automation script, after which pak
  compression can stay on `[Docs: partner.steamgames.com/doc/sdk/uploading]`. A forum suggests 2048 instead; they disagree, and
  **IoStore** containers were reported to change between builds even when content did not (UE-135588) `[Forum]`.
- **Plan**: split content into chunks by churn (code-adjacent data, UI, atlases per biome group, audio, fonts) with Primary Asset
  Labels and chunk ids, so a balance patch touches one small container (the Godot game's code-only patches were ~0.75 MiB of
  ~200 MiB). Then **measure** with Valve's tooling or the Godot project's `tools/steam_delta.py` approach: build N, change one
  number, build N+1, compute the SteamPipe-style delta. Pick the alignment and IoStore on/off from the measurement, record the
  result in `Knowledge/2D/02` or `Steam/03`.
- Never rename or reorder shipped assets casually between releases; batch engine upgrades with content (every player re-downloads
  the engine binaries).

## 4. itch

The Godot release flow ports: staging folder that is exactly the release, butler push to `pugtatogames/pawpocalypse:<channel>`
(a new channel for the Unreal build until it replaces the Godot one: the channel name is permanent), `butler status` before pushing
`[KB G/04 §4]`. **Code signing** (SmartScreen on browser downloads): Azure Artifact Signing ~$9.99/month or OV certificate; sign
the exe only. Unreal exes do not carry an appended blob, so the Godot antivirus-heuristic issue (embedded PCK) does not apply.

## 5. Steam integration (built 2026-11-26, HANDOFF-LOG (155))

**The App ID is 5720010** (checked 2026-11-26 with `steamcmd +login <account> +app_info_print 5720010`: name "Pawpocalypse",
depot 5720011). Depot 5720011 "Pawpocalypse Content". Packages: 1968922 Developer Comp (depot 5720011, Release State
Override: the team's accounts own and play it before release), 1968923 Beta Testing (keys), 1968924 the store package.

### 5.1 In the game (as built)

- **Two game targets.** `Pawpocalypse` (the editor, every test and gate, builds outside Steam): never starts the Steam API.
  `PawpocalypseSteam` (`Source/PawpocalypseSteam.Target.cs`): the same game with `PAW_STEAM_BUILD=1`
  (`Pawpocalypse.Build.cs` sets it from the target's name). An automated run (automation, `-unattended`) or `-PawNoSteam`
  never starts Steam even in the Steam build, so nothing automated can unlock an achievement on the designer's account.
  Both targets compile the game module into `Intermediate/Build/Win64/x64/UnrealGame/<Config>` (the launcher's shared build
  environment): building one after the other recompiles the game module, never mixes them `[KB U/01 §2.4]`.
- **The Steam API directly, not Online Subsystem Steam** (a single-player game needs none of its sessions, and the launcher
  engine ignores a Steam target's `CustomConfig`, so the OSS route needs an extra config module, `[KB U/01 §2.4]`). The engine's
  Steamworks SDK (v164); the SteamShared plugin (enabled for the game, `tools/trim_game_plugins.py`) loads `steam_api64.dll`.
  `Platform/PawSteam` is **the only code that calls Steam**; `Platform/PawPlatformSubsystem` is what the game asks
  (docs/03's `IPawPlatform`). In every other build each answer is "no platform".
- **Start**: the game module's `StartupModule` (`Pawpocalypse.cpp`): a Shipping Steam build opened outside Steam calls
  `SteamAPI_RestartAppIfNecessary(5720010)` and exits (Steam starts it); then `SteamAPI_InitEx`, callbacks pumped every frame
  (`FTSTicker`). A Development Steam build run from its exe gets the App ID from the `SteamAppId` environment variable it sets
  (no `steam_appid.txt`; the package audit fails on one).
- **Achievements**: every id in the progress's `AchievementsEarned` is set on Steam (`SetAchievement`, one `StoreStats`):
  a new one within a second (a 1 s poll of the earned count and the progress epoch, as the audio's), and on a load the ones a
  save earned before Steam knew (a save made outside Steam catches up). Steam loads the account's achievements at
  `SteamAPI_Init` (SDK 1.61+: no `RequestCurrentStats`); a write before that, or of an id Steamworks lacks, is retried
  (`UserStatsReceived_t` triggers one). API names = the ids of `Data/AchievementData.csv`.
- **Rich presence**: in a running realm `steam_display = "#Biome_<tier id>"`, the biome's name (the Steamworks file: the
  string tables' `TIER_<id>_name` in 23 languages); on the title none (Steam shows "Playing Pawpocalypse"). No new words.
- **The overlay**: `GameOverlayActivated_t` -> `UPawPlatformSubsystem::OnOverlay` -> the HUD's `OpenPauseMenu()` (refused
  while a menu or modal is up or the game is paused).
- **Saves and Steam Cloud**: under Steam the slots live in `SaveGames/<64-bit Steam ID>/` (Auto-Cloud's `{64BitSteamID}`; two
  accounts on one PC never share or upload each other's slots); the settings stay in the root, per PC `[KB U/10 §5.1]`.
  The slots made before the game knew the account move into the first account's folder once (`PawAdoptSlots`: copied,
  then deleted from the root only after every copy of that slot succeeded; never over a slot the folder has).
- **The language**: "system" means the language chosen for the game in Steam (`Pc::LocaleResolve`'s Steam argument).
- **Pads**: Steam Input's type of the first translated pad (`ISteamInput::Init(false)`, `GetInputTypeForHandle`) names
  the prompt family: PlayStation, Steam Deck, Xbox; Switch and DirectInput pads draw as Generic (Xbox's prompts; no Nintendo
  prompts). Read at most once a second; a known pad follows Steam's answer. The Button prompts setting still wins.
- Tests: `Pawpocalypse.Platform.*` (no Steam in the editor or tests, the presence token, the pad family, the slot move),
  each seen to fail (`tools/sabotage_realm.py`, `platform_*`). `py tools/steam_files.py --check` (everyday): the Steamworks
  files can be made and agree with the game.
- **Report a bug** opens `https://steamcommunity.com/app/5720010/discussions/` (playtest 2026-11-26, note 5), in the Steam
  overlay while Steam runs (`UPawPlatformSubsystem::OpenWebPage`), else the browser.
- **The exe's icon**: `SourceArt/icon/Pawpocalypse.ico` (`tools/make_app_icon.py`, from the key art's square icon, D26),
  set in `Source/Pawpocalypse.Target.cs` (`WindowsPlatform.ApplicationIcon`; not `Build/Windows/Application.ico`, which is
  the gitignored `build/` on Windows). Steam's own client and library icons are uploaded in Steamworks.
- **Not done**: the pad type and the overlay pause are untested on real hardware (no pad and no Steam run in a gate).

### 5.2 Steamworks (entered 2026-11-26 in the designer's browser, published)

| Page | Entered |
|---|---|
| Installation > General | install folder `Pawpocalypse`; launch option 0 `Pawpocalypse.exe`, Launch (Default), Windows, 64-bit only |
| Installation > Redistributables | Visual C++ 2022 (includes 2019, 2017, 2015) |
| Application > General | Supported OS: Windows, 64 Bit Only |
| Stats & Achievements > Achievements | 23 languages (Configure Languages); 39 achievements by Bulk Import (`build/steam/steamworks`, `py tools/steam_files.py`): names in 23 languages, descriptions English only (a DRAFT, `docs/STEAM-ACHIEVEMENTS-DRAFT-2026-11-26.csv`, for the designer; translated after the yes), our own icons achieved and not ("Automatically generate unachieved icons" unticked). **2026-11-27 (HANDOFF-LOG (160), published):** "Boundless Vitality" became "Big Family" (D-l: the skill is Extra Kittens; its old translations cleared, so the other languages show the English until the retranslation), and the two Rainbow Gut goals name their skill (they said "Unicorn"); `steam_files.py --check` now holds a skill goal to the tree's label |
| Community > Rich Presence | `rich_presence.vdf`: `#Biome_tier_1` ... `#Biome_tier_12`, 23 languages |
| Application > Steam Cloud | 10,000,000 bytes (Steam's minimum; a slot is ~3.5 KB), 20 files; Auto-Cloud: root `WinAppDataLocal`, subdirectory `Pawpocalypse/Saved/SaveGames/{64BitSteamID}`, pattern `slot_*.sav` (the slots, their `_bak` and `_tmp`; never `settings`), not recursive, [Cross-Platform]. **"Enable cloud support for developers only" is ticked (Steam's default): untick before release** |
| Application > Steam Input | opted in: PlayStation, Nintendo Switch, Generic (DirectInput), Any Future Devices (not Xbox: the game reads XInput); default template Generic Gamepad; Deck touchscreen: Left Mouse Click Emulation (default) |

Store page > Supported Features (achievements, Cloud, controller support) is the designer's, with the store page.

### 5.3 Uploading a build

`py tools/package.py --config Shipping --steam --bump` (the Steam target, `build/package_steam`, the start file renamed
`Pawpocalypse.exe`; the audit also fails on no `steam_api64.dll`, no start file, a `steam_appid.txt`), then
`py tools/steam_upload.py --user <the designer's Steamworks account> [--branch NAME]` (SteamCMD's cached login; App ID from
`PawSteam.h`, depot App ID + 1; excludes `*.pdb`, `*.bat`, `README.txt`, `Manifest_*.txt`). Without `--branch` the build
lands live nowhere: Steamworks > SteamPipe > Builds > pick the branch > Preview Change > Set Build Live Now; the default
branch asks for an SMS code (the account has no mobile authenticator): the designer's step. SteamCMD can set a beta branch
at upload, never the default one.

| BuildID | Version / commit | Upload | Live on default |
|---|---|---|---|
| 26942308 | 0.1.1, 1f7ce73 + the version bump; the plain target (no Steam API) | 243.8 MB, 48 files, 259 chunks (players: 159.8 MB) | 2026-11-26 (the designer) |
| 26943569 | 0.1.2, 1f7ce73 + (155); the Steam target (achievements, presence, Cloud, overlay pause, pad type) | 5 files added, 4 changed, 1 removed; 125 chunks (players: 47.3 MB) | 2026-11-26 (the designer) |
| 26966137 | 0.1.3, the commit of HANDOFF-LOG (160); the playtest's work (157)-(160): Phase 1, the Compass tree, the 480 dock, the shop tooltip, Boba, the gold signature glow | 7 files changed (133.5 MB), 141 chunks | waiting for the designer's SMS code (2026-11-27) |

### 5.4 Rules

- Gameplay code never calls Steam directly `[KB S/01 §3]`; no SteamSockets, no sessions (single player).
- **Steam Deck**: default controller config reaches everything; glyphs match; text >= 12 px at 1280x800; 60 fps target at 800p
  (30 minimum); graphics settings per device; the on-screen keyboard is not needed (no text entry) `[KB S/02]`. Run the Windows
  build under Proton on the Deck before claiming "Verified".
- Leaderboards: untrusted by default (widely cheated) `[KB G/04 §5]`: only with trusted writes, if ever.

## 6. Consoles (later; build in now)

From `[KB P/01]`: register as a PlayStation Partner / Xbox ID@Xbox early (approval takes calendar time); consoles need a
**source-built engine** with platform extensions from Epic console access. What this port does from day one so a console port is a
swap, not a rewrite: gamepad reaches everything; no mouse-only action; TV-safe margins (nothing critical within 5% of an edge);
saves through `ISaveGameSystem`; achievements/trophies through `IPawPlatform`; suspend/resume and controller-disconnect pause
(prompt); no `#if PLATFORM_*` in gameplay; required languages checked per platform; 60 fps.

## 7. Release checklist (Unreal version of the Godot one)

Pinned commit; Shipping + For Distribution; compile all Blueprints; all gates L0-L5 green; cooked asset audit (Asset Audit with the
cooked filter); pak listing audit (no never-ship content); colour gate on the packaged build; PSO precaching validated
(`py tools/packaged_pso.py`, the whole-game route: no `Missed` of this game's materials; 2026-11-24, HANDOFF-LOG (142)) and a
first-run hitch check with `-clearPSODriverCache` on a clean machine without Unreal or Visual Studio (NOT on a working PC: on
NVIDIA it deletes the driver's shader cache every game on the machine shares, `WindowsD3D12Device.cpp`)
`[KB U/12 §6]`; soak at 5x for the whole game; save-version bump in the same commit as any save-meaning change; a rollback build
kept; beta branch on Steam first `[KB S/03]`; the Steam build (`--steam`), Steam Cloud's "developers only" unticked, the
achievement descriptions approved and translated (doc 10 §5.2).
