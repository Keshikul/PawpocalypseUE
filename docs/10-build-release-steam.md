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
  registered with `CookRule=AlwaysCook`; maps `L_Title`, `L_Realm` (and test maps for packaged tests in Development only).
  String tables and hardware cursors staged as UFS `[KB U/19 §1]`.
- `DefaultScalability.ini`: scalability cvars only (anything else stops the cook) `[KB U/12 §2]`.

## 2. Packaging script (`tools/package.ps1`)

UAT `BuildCookRun` wrapper `[KB U/19 §2]`:
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

## 5. Steam integration (after the App ID; the Godot plan's workstream F)

- `IPawPlatform` interface (Null implementation by default; Steam implementation in a `Steam` target via `CustomConfig = "Steam"`
  `[KB U/01 §2.4]`): achievements (mirroring the 41 in-game ones), rich presence (biome, stage), Cloud (Auto-Cloud on the save
  folder: no code), the real pad type behind Steam Input for glyphs, overlay-aware pause, the store/discussions URL for Report a
  bug (`MainMenu.BUG_REPORT_URL` points at itch until then).
- Online Subsystem Steam is enabled only for these services; **no SteamSockets, no sessions** (single player). `steam_appid.txt`
  only while developing.
- Gameplay code never calls Steam directly `[KB S/01 §3]`.
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
(`stat psoprecache`) and a first-run hitch check with `-clearPSODriverCache` on a clean machine without Unreal or Visual Studio
`[KB U/12 §6]`; soak at 5x for the whole game; save-version bump in the same commit as any save-meaning change; a rollback build
kept; beta branch on Steam first `[KB S/03]`.
