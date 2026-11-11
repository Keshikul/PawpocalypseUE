# Worklist: the code review of 2026-11-10, findings and the fix plan

> A read-only review of the whole port on 2026-11-10 (designer: "Read the gdd. Do a very thorough code review. Take note of
> faults and think how to fix them. Create a fix plan."). **No code, data or asset was changed.** Read first: the Godot GDD v9
> (`C:\Games\pawpocalypse\docs\pawpocalypse-gdd.md`), docs 00, 02, 03, 12, the newest HANDOFF-LOG entries, the knowledge base's
> gotcha list. Then the source: every module's build rules and the config; PawCore's combat, stats, cards, save, JSON, format
> and the field's structure; the game module's realm, render, core, save, settings, audio, input, the HUD, the title, the tree,
> dev, data and text; the editor's baker guard; `check_all.ps1`. Every engine claim below was checked in the UE 5.8.3 source
> (file named). The general lessons went into the knowledge base (HANDOFF-LOG (92)).
>
> Each fix follows the standing rules: a test that FAILS on today's code first, a sabotage seen CAUGHT, `check_all` exit 0.
> A fault Godot has too is fixed here, not copied (D16, designer 2026-11-10: "Do not port faults from godot"), and listed
> in docs/01 §5. New player words are drafted for the designer first (CLAUDE.md).

## 1. Verdict

The port is in good health. PawCore is careful code: the save reader never throws and bounds its depth, non-finite numbers
are handled on load (F33), double-to-integer casts are guarded against undefined behaviour, and the rules are proven against
the real Godot code. The save pipeline (one writer pipe, temp + verify + backup, recovery at startup, the sandbox) is right.
Pause holders are paired, every screen has an input config, Shipping compiles the dev code out, the baker never bakes over an
asset. The faults are in the engine glue, where headless tests cannot look: one missing on-screen prompt after a map change
(CR1), save outcomes the title never shows (CR2), the audio voices (CR3, CR4), and per-frame work in the effects meshes (CR5).
None corrupts a save. Two need the designer's words. The four Godot-side questions are settled by D16 (2026-11-10): three
are faults the port fixes, one was stale docs.

## 2. The findings at a glance

| ID | Sev | Area | Finding | Kind | Size |
|---|---|---|---|---|---|
| CR1 | **H** | Input | The press prompt on the focused button is gone for good after the first map change | port bug | S |
| CR2 | **M-H** | Title, saves | The title ignores the load's outcome: a newer build's slot plays a session that is never saved | port bug (+ a Godot fault, F47: fixed here too) | M, words first |
| CR3 | **M** | Audio | Every sound and every track makes a new audio component; the 16-voice rule never cuts | port bug | S |
| CR4 | **M** | Audio | The play log (for tests) grows for as long as the game runs | port bug | S |
| CR5 | **M** perf | Render | The three effect bands rebuild a collision body and a scene proxy every frame, empty or not | port bug | S, then M |
| CR6 | L-M | Realm, input | The stick moves the cat and warps the pointer while the field is paused | port bug (verify first) | S |
| CR7 | L | Dev | The debug overlay is on in Development packages, inside the frame gate's measurement | tooling | S |
| CR8 | L | Settings | Every slider step writes the settings file on the game thread | port bug | S |
| CR9 | L | Tree | The tooltip is rebuilt every frame; focus found by scanning 87 nodes every frame | perf | S |
| CR10 | L | Input | The focus prompt walks the widget tree every frame before its cheap check | perf | S |
| CR11 | L | UI | `DA_Icons` held by a function-static `TStrongObjectPtr` (released after the engine at exit) | hygiene | S |
| CR12 | L | UI | Glyph textures and baked widget classes cached weakly: GC'd, then reloaded synchronously | perf | S |
| CR13 | L | HUD | A HUD's destruct clears the dock's key consumer even if a newer HUD set it | latent bug | S |
| CR14 | L | UI | Global UI state (open tab, tree selection, jobs seen) outlives worlds, games and tests | hygiene | M |
| CR15 | L | HUD | Quick Save says "Saved" when saving is off or the write fails | port bug | S, words first |
| CR16 | L | Render | The atlas index re-builds on every lookup if two cells share an id | latent perf | S |
| CR17 | L | Realm | Per-frame allocations: backdrop key, atlas lookups by string, event vector, sprite copies, audio hand copy | perf | S |
| CR18 | L | Render | Every frame uploads the whole instance pool, which only grows | watch | - |
| CR19 | L | Save | The storage interface's threading comment is wrong (settings write on the game thread) | doc | S |
| CR20 | L | Settings | Fullscreen takes the primary display's size, not the window's display | port bug | S |
| CR21 | L | Save | Autosave keeps rewriting the loaded slot while the title is up | Godot fault (F48): fixed here | S |
| CR22 | L | Save | Saved card levels are not clamped like shop and tree levels (F32) | Godot fault (F49): fixed here | S |
| CR23 | L | Docs | Doc 02 says the Overkill bounty is "capped at 4x"; with exponent 0.4 nothing caps it | stale docs (F50): doc 02 fixed | done |

## 3. The findings

### CR1 (H) The press prompt on the focused button is gone after the first map change

- **Where**: `Source/Pawpocalypse/Input/PawInputDeviceSubsystem.cpp:553-559` (`PlaceFocusPrompt`).
- **What happens**: the prompt widget is created once, on the first ticker tick, which is on `L_Title`, and added to the
  viewport inside `if (!FocusPrompt)`. `UUserWidget::GetWorld()` caches the world it first resolves through the game instance
  (the title's), and `AddToViewport` uses a slot with `bAutoRemoveOnWorldRemoved = true`. When Continue opens `L_Realm`,
  `UGameViewportSubsystem::HandleRemoveWorld` removes every such widget of the dying world. Nothing adds the prompt again: in
  the realm (the dock, the card pick, the tree, the pause menu) keys and pad players never see the press glyph on the focused
  button, and after a quit to the title the title loses it too. Godot's prompt is an autoload's node and never goes away.
- **Evidence**: UE 5.8.3 `UserWidget.cpp` (`GetWorld`, `AddToViewport`), `GameViewportSubsystem.cpp` (`HandleRemoveWorld`,
  bound to `OnWorldCleanup`). The tests check only `FocusPromptAt` and `FocusPromptShown` (pure functions); `paw.PadPlay`
  checks focus, not the prompt; the toast survives because `ShowToast` re-adds it (`IsInViewport` check, line 517).
- **Verify first** (one minute): `-game` from `L_Title`, Continue, take the pad, look at the dock.
- **Fix**: in `PlaceFocusPrompt`, `if (!FocusPrompt->IsInViewport()) FocusPrompt->AddToViewport(UPawToast::ZOrder - 1);` each
  time it is placed (as `ShowToast` does), or add it through `UGameViewportSubsystem::AddWidget` with a slot whose
  `bAutoRemoveOnWorldRemoved = false`.
- **Test first**: an engine test that removes the prompt from the viewport (as the world cleanup does), ticks the subsystem and
  asserts `IsInViewport()` (fails today); and `paw.PadPlay` asserts the prompt is in the viewport and visible on the focused
  dock row after the travel from the title. Sabotage: drop the re-add line; CAUGHT.

### CR2 (M-H) The title ignores the load's outcome

- **Where**: `Source/Pawpocalypse/UI/PawMainMenu.cpp:934` (`PressSlot` calls `LoadGame` and drops the result), `:846`
  (`PressContinue`), `:644` (`Refresh`: Continue is on when any slot file exists). `EPawLoadOutcome`'s own comment says it is
  "for the title screen's word to the player"; only tests read it.
- **What happens**:
  - **A newer build's slot** (a downgrade: a Steam beta branch rolled back, a Cloud save from a newer build on another PC): at
    startup `LoadGame` refuses it (`NewerBuild`) and memory holds a fresh game. Continue is enabled, the realm opens, and every
    autosave, the quit save and Quick Save fail in `WriteSlotAtomically` ("the slot holds a newer build's save: not overwritten",
    logged only). **The whole session is never saved and the player is never told.** Port-only logic (Godot has no newer-format
    refusal): a port bug.
  - **Slot and backup both unreadable**: Continue plays a fresh game into that slot and the first autosave writes over it (the
    old bytes survive as `slot_N_corrupt`). Godot does the same (`save_manager.gd` `load_game`: `push_error`, then return):
    a Godot fault (F47), which the port fixes too (D16), words first.
  - **A slot chosen in the picker that fails to load** plays whatever was in memory (another slot's game) with the active slot
    unchanged, and says nothing.
- **Fix**: `PressSlot` / `PressContinue` branch on the outcome. `NewerBuild`: refuse to play that slot and say why; at startup,
  if the active slot is a newer build's, Continue is off (or opens the slot picker). `Corrupt`: say so and let the player choose
  (start fresh in that slot, or pick another; F47). `LoadedFromBackup`: one line that the backup was used. Make `Pc::TitleMenu` take the outcome so the rule is
  PawCore's and unit-tested.
- **Words first**: up to three new `ST_UI` lines (newer build, unreadable, restored from backup), drafted in
  `docs/TEXT-DRAFT-*.csv` and through `tools/text_audit.py` before any code.
- **Test first**: `Pawpocalypse.UI.Title`: a memory storage whose active slot is a newer format; assert Continue does not open
  the realm and the status says so (fails today). Same for a slot picked in the picker. Sabotage: ignore the outcome again.

### CR3 (M) Every sound makes a new audio component

- **Where**: `Source/Pawpocalypse/Audio/PawAudioSubsystem.cpp:232` (`Voice`) and `:248` (`MusicPlayer`):
  `if (!IsValid(V) || V->GetWorld() != World)`.
- **What happens**: `CreateSound2D(..., bPersistAcrossLevelTransition = true)` builds the component from the audio device alone
  (`FCreateComponentParams(AudioDevice)`): its outer is the transient package and it is never registered with a world, so
  `GetWorld()` is always null and the guard is true on EVERY call. Each `Play` makes a new `UAudioComponent` and drops the old
  one still playing: the 16-voice rule ("the 17th cuts the oldest") never cuts anything (`Stop()` lands on the new, silent
  component), a volume slider misses the dropped sounds, the GC gets a new object for every sound (~10-20 a second in a busy
  biome). A looping track replaced inside the 1.2 s crossfade (A -> B -> A, e.g. the boss music and back) plays on until the
  GC destroys its component (`UAudioComponent::BeginDestroy` stops looping sounds only), up to about a minute.
- **Evidence**: UE 5.8.3 `GameplayStatics.cpp:1667`, `AudioDevice.cpp` `CreateComponent`, `ActorComponent.cpp:1101`
  `GetWorld_Uncached`, `AudioComponent.cpp:182`. Headless tests have no audio device (`CreateSound2D` returns null), so no
  test can see it. The knowledge base's own audio recipe carried the faulty "re-created when the world changes" note
  (corrected there, U/14 §2.1).
- **Fix**: the persistent components survive a map change by themselves: create each voice and each music player once and
  reuse it while `IsValid()`. Route creation through a `TFunction` seam, as `SwapOutput` / `SystemDevices` already are.
- **Test first**: with a counting creator, 40 `Play` calls make at most 16 components and two `PlayMusic` calls make at most 2
  (fails today: 40 and 2+). Sabotage: restore the world check; CAUGHT.

### CR4 (M) The play log grows without end

- **Where**: `Source/Pawpocalypse/Audio/PawAudioSubsystem.cpp:283` (`Log.Add` on every play that passes its rate limit).
- **What happens**: the log exists for the tests (`Played()`, `ClearLog()`: only tests call them) but fills in every build,
  Shipping too. `mouse_killed` alone is capped near 9 a second: ~30,000 entries an hour, each with an `FString`. An idle game
  left on overnight (no offline progress: players leave it running) holds tens of MB of it.
- **Fix**: compile it only `#if WITH_DEV_AUTOMATION_TESTS` and fill it only while a test switches it on, or a fixed ring of 256.
- **Test first**: 10,000 plays leave the log at its cap (fails today). Sabotage: remove the cap.

### CR5 (M, performance) The effect bands rebuild collision and a scene proxy every frame

- **Where**: `Source/Pawpocalypse/Render/PawSpriteRenderer.cpp:373-394` (`SetFx`, called three times a frame).
- **What happens**: an empty band calls `ClearAllMeshSections()` every frame and a busy one `CreateMeshSection()`. Both end in
  `UpdateLocalBounds(); UpdateCollision(); MarkRenderStateDirty();` whatever the collision settings. With
  `bUseAsyncCooking = false` (line 373) `UpdateCollision` rebuilds the body setup synchronously (new GUID,
  `InvalidatePhysicsData`, `CreatePhysicsMeshes`, `RecreatePhysicsState`), and `MarkRenderStateDirty` recreates the scene proxy
  (new GPU buffers) on the render thread. So the field pays three body rebuilds and three proxy recreations every frame, even
  with no effect on screen; plus two `TArray` allocations per band. The 0.29 ms the effects added to the view's draw (doc 04,
  KB 2D/02 §4.6) includes the body rebuilds; the proxy cost is render-thread time nobody has measured.
- **Evidence**: UE 5.8.3 `ProceduralMeshComponent.cpp:633-635`, `:781-786`, `:1034-1076`.
- **Fix, two steps**: (1) remember which bands are empty and skip the clear while they stay empty (a few lines); (2) keep each
  band a fixed-capacity, un-indexed triangle list (three vertices per triangle, unused ones collapsed to a point) updated with
  `UpdateMeshSection` (same vertex count: a render command, no collision, no new proxy), and call `CreateMeshSection` only to
  grow it. If that is still too heavy, a small custom primitive whose proxy builds the triangles in `GetDynamicMeshElements`.
- **Test first**: a renderer counter of section rebuilds: 60 empty frames rebuild nothing (fails today: 180). Then
  `packaged_bench.py` before and after (game thread, and `stat unit`'s draw thread), reported in the HANDOFF-LOG.

### CR6 (L-M) The stick steers the cat while the field is paused

- **Where**: `Source/Pawpocalypse/Realm/PawRealmView.cpp:215-233` (`SampleInput`: recentre and the stick path).
- **What happens**: the view samples input every frame, paused or not. While a pause holder is up (card pick, tree, pause
  menu, settings), a deflected stick still calls `MoveCat` (the field's input and `PC->SetMouseLocation`, a warp). Godot's
  `player.gd _process` does not run under a paused tree. With Common UI's Menu input the controller may hold the stick's last
  value, so the cat's input can move under a menu the player is steering with the same stick.
- **Verify first**: hold the stick, let a card pick open, keep holding; watch the pointer and where the cat is on resume.
  Godot's side, checked 2026-11-10: `player.gd` sets no `process_mode` (no scene overrides it), so its `_process` (line
  489, the stick) stops with the paused tree. The night worklist's failing test replaces the hand check.
- **Fix**: skip recentre and the stick path while `R->IsPaused()` (the mouse path is harmless: the field ignores it until it
  steps).
- **Test first**: with a pause hold, a stick deflection leaves `Sim.Input.CatX/Y` unchanged and warps nothing.

### CR7 (L) The debug overlay is inside the frame gate

- **Where**: `Source/Pawpocalypse/Dev/PawDebugHud.cpp:17` (`paw.DebugHud` defaults to 1); `PawGameMode.cpp` installs it in
  every non-Shipping build; `tools/packaged_bench.py` and `paw.Bench` do not turn it off.
- **What happens**: the Development package the frame gate measures (and any Development playtest build) draws the overlay:
  ten canvas texts, five `Printf` and a `TActorIterator` a frame, inside the game-thread numbers and the draw-call count the
  gate fails on (68.7 against 50, doc 01 Q11).
- **Fix**: `packaged_bench.py` adds `paw.DebugHud 0` to `-ExecCmds` (and `paw.Bench` sets it); or default it to 0 outside the
  editor. Re-measure: the draw calls with and without it go in the HANDOFF-LOG next to (89)'s split.

### CR8 (L) Every slider step writes the settings file

- **Where**: `Source/Pawpocalypse/Save/PawSettingsSubsystem.cpp:81` (`Save`), `:99` (`Changed`).
- **What happens**: every setter writes the whole file synchronously on the game thread; a volume or cursor-speed slider drag
  writes it dozens of times a second through `ISaveGameSystem` (a file write each time): possible hitches while dragging.
- **Fix**: mark dirty; write once at the end of the frame (an `FTSTicker` one-shot), and at once only on `Deinitialize`.
  Godot writes on every set, but when the write happens is not a rule.
- **Test first**: ten `SetMasterVolume` calls in one frame make one write (`GetWrites()`; fails today with 10).

### CR9 (L) The tree's tooltip is rebuilt every frame

- **Where**: `Source/Pawpocalypse/UI/PawSkillTreeScreen.cpp:1231` (`NativeTick` -> `ShowTip`), `:964` (`ForceLayoutPrepass`).
- **What happens**: while a node is hovered or the tip is docked for the pad, every frame runs `Pc::TreeTip`, about eight
  `FText::Format`, a glyph parse per line and a layout prepass of the tip panel. The rich text itself is not rebuilt
  (`SRichTextBlock::SetText` returns on an identical text, 5.8.3 `SRichTextBlock.cpp:116`), so this is CPU only, while a
  paused menu is up. The same tick asks all 87 nodes for focus.
- **Fix**: rebuild the lines only when the node, the docking or the tip's numbers change (a small digest); move the tip every
  frame. Track focus from the nodes' focus events instead of scanning.

### CR10 (L) The focus prompt searches the widget tree every frame

- **Where**: `Source/Pawpocalypse/Input/PawInputDeviceSubsystem.cpp:568`.
- **What happens**: `FSlateApplication::FindPathToWidget` (a walk of the window's widgets with their geometry) runs every frame
  whenever a button has focus, before the cheap `FocusPromptShown(true, bLastWasPointer)` that hides it for mouse players.
- **Fix**: check `bLastWasPointer` first; with CR1, do the path search only when focus or the layout changed.

### CR11 (L) A static strong pointer to `DA_Icons`

- **Where**: `Source/Pawpocalypse/UI/PawIcons.cpp:34`.
- **What happens**: the function-static `TStrongObjectPtr` is released by the C++ static destructors at exit, after the engine
  has shut down (the release goes through `ReleaseUObject`); release order across files is unspecified. No crash seen yet; it
  also pins the asset across PIE sessions.
- **Fix**: own it in a subsystem `UPROPERTY` (the icons are a game-instance concern), or reset it on `FCoreDelegates::OnPreExit`.

### CR12 (L) Weakly cached assets reload synchronously

- **Where**: `Source/Pawpocalypse/UI/PawGlyphs.cpp:189` (glyph textures in a `TWeakObjectPtr` map); `PawWidgets.cpp`
  `PawUi::Resolve` (baked classes, weakly).
- **What happens**: a glyph texture no widget shows is collected by the GC; the next prompt redraw (every prompt redraws on a
  device switch) loads it again with `LoadObject`, synchronously, in the middle of play. The same for a Widget Blueprint class
  whose last instance went away. The project's rule is "nothing loads per event".
- **Fix**: keep strong references (a subsystem array) for the glyphs of the families in use, and for resolved classes.

### CR13 (L) A HUD's destruct can clear a newer HUD's dock consumer

- **Where**: `Source/Pawpocalypse/UI/PawHudScreen.cpp:1307`.
- **What happens**: `NativeDestruct` sets `ConsumeForDock = nullptr` without checking it is its own. If a new HUD activates
  before the old one is destructed (travel ordering), the new HUD's consumer is wiped and pad A presses dock rows twice again
  (the bug fixed in HANDOFF-LOG (49)).
- **Fix**: remember the owner in the device subsystem (`SetDockConsumer(Owner, Fn)` / `ClearDockConsumer(Owner)`).

### CR14 (L) Global UI state outlives worlds and games

- **Where**: `PawHudScreen.cpp:67` (`GActiveTab`), `PawSkillTreeScreen.cpp:364` (`GLastSelected`), the jobs-seen static
  (`UPawHudScreen::ForgetJobsSeen`).
- **What happens**: Godot's static vars live for the process too, so behaviour matches; but the values cross a quit to the
  title, a New Game, PIE sessions and automation tests (test order can leak a tab into the next test).
- **Fix**: move them to a game-instance subsystem with an explicit reset on New Game and load, and in the tests' fixture.

### CR15 (L) Quick Save says "Saved" when it did not save

- **Where**: `Source/Pawpocalypse/UI/PawHudScreen.cpp:2779`.
- **What happens**: the text is set at once, even with saving off (`-NoSave`, an automated run) and before the writer knows the
  write failed (CR2's newer-build slot fails every write).
- **Fix**: say "Saved" when the write is queued and saving is enabled; after the pipe reports a failure, the failure text.
  Words first.

### CR16 (L) The atlas index can rebuild on every lookup

- **Where**: `Source/Pawpocalypse/Render/PawSpriteAtlas.cpp:10`.
- **What happens**: the cache counts as valid while `ById.Num() == Cells.Num()`. Two cells with one id (or two ids equal in
  `FString`'s case-insensitive hash) make it rebuild the whole map and the rigs on every `Find` / `Rig`, i.e. dozens of
  times a frame, silently. An editor edit that keeps the count leaves it stale.
- **Fix**: a `bIndexed` flag reset in `PostLoad` / `PostEditChangeProperty`; `import_art.py` and `Look.Atlas` assert the ids
  are unique.

### CR17 (L) Per-frame allocations in the realm

- `PawRealmView.cpp:428`: `FString::Printf` key every frame to notice a biome or size change: compare the tier index and sizes.
- `DrawCreatures`: about seven atlas lookups by string a frame (shadow, kitten, add, the boss, the cat stage, the weapon by
  `Printf`, the hat), each building an `FString`: cache the cells per biome and per stage; the `TSet` of add serials.
- `PcField.cpp:2635`: `DrainFieldEvents` swaps the vector out, so the field's event queue loses its capacity every step:
  iterate and `clear()`.
- `PawSpriteRenderer.cpp:189`: every sprite is copied into `Sprites[]` for the tests, Shipping included: keep it for tests only.
- `PawAudioSubsystem.cpp:458`: the hand (`std::vector<std::string>`) copied every frame: compare first.
- `SampleInput`: a `TMap<FKey, float>` and four `KeysOf` arrays a frame.
- None is large; together they are a few hundred allocations a second. Fix in one pass with the frame gate re-run.

### CR18 (L, watch) The whole instance pool is uploaded every frame

- `PawSpriteRenderer.cpp:238`: `SetCustomData` for every pooled instance and one transform batch for the whole pool, parked
  ones included. The pool grows to the largest frame ever drawn and never shrinks. Fine at today's numbers (GPU 0.47 ms); worth
  a line in the bench report (pool size vs drawn) so a regression shows.

### CR19 (L) The storage interface's threading comment is wrong

- `Source/Pawpocalypse/Save/PawSaveStorage.h:7` says writes come from the save writer thread only; `UPawSettingsSubsystem`
  writes `settings` on the game thread without `Flush()`. Safe today (another file; the generic save system holds no state;
  the memory store locks). Say so in the comment, or route the settings write through the save pipe.

### CR20 (L) Fullscreen uses the primary display

- `Source/Pawpocalypse/Save/PawSettingsSubsystem.cpp:146`: `RequestResolutionChange(PrimaryDisplayWidth, PrimaryDisplayHeight,
  WindowedFullscreen)` even when the window is on another monitor. Use the monitor the window is on
  (`FDisplayMetrics::MonitorInfo` containing the window's centre). Godot goes fullscreen on the window's screen.

### CR21 (L, a Godot fault, D16) Autosave on the title

- `Source/Pawpocalypse/Save/PawSaveSubsystem.cpp:219`: the 30 s clock runs while the title is up (no realm is paused there), so
  a loaded game is written again every 30 s and its "played ... ago" stays "just now". Probably Godot's behaviour too (its
  timer is an autoload's); check before changing.
- **Checked 2026-11-10: Godot does the same.** `save_manager.gd` lines 77-87: the autosave `Timer` is a child of the
  autoload and stops only with a paused tree; the title does not pause it. The port matches, so under D16 it is a Godot
  fault (F48: the timer's own comment says a state that is not changing is not written) and the port fixes it: the
  unprompted saves (the timer, a focus loss) run only while a realm is up. Test first: with no realm, 31 s of ticks and a
  focus loss write nothing. Scheduled in the night worklist; **done** that night (HANDOFF-LOG (93)).

### CR22 (L, a Godot fault, D16) Saved card levels are not clamped

- `Source/PawCore/Private/PcSave.cpp:185`: shop and tree levels are clamped on load (F32); card levels are taken as saved.
  Harmless (every read bounds-checks: `CardValueAt`, `CardCostAt`) but inconsistent. D16: a Godot fault (F49, Godot's
  `card_manager.gd` `apply_run_save()`), fixed here: clamp to 0..max like the others, in batch F (PawCore and its save
  fixture).

### CR23 (L, resolved: stale docs) The Overkill bounty's cap

- `docs/02-game-design-handoff.md:65` says "exponent 0.4, capped at `overkill_bounty_max_multiple` (4x)"; the Godot GDD says
  "capped at 4x". The data ships `overkill_bounty_exponent` 0.4, and with an exponent above 0 the code (Godot's and the port's)
  never reads the cap: the bounty is `min(w, w^0.4)` of the victim's reward, uncapped (a 1e15x overkill pays 1e6x). Either the
  docs or the data are wrong. A question for the designer; then fix doc 02 (and the GDD in Godot) to match.
- **Resolved 2026-11-10: the docs were stale, not the data.** Godot's economy rework (2026-08-24, `de61fadb`) switched on
  the curve that `pawpocalypse-economy-redesign.md` §2.2 planned ("Replace the [flat cap] ... surplus ^ 0.40"); the GDD's
  "capped at 4x" and the comments' "ships at zero" (`combat_resolver.gd` 222, `combat_constants_data.gd` 170) predate it.
  Doc 02 corrected; Godot's docs are F50. Nothing to decide.

## 4. The fix plan

Order: what a player meets first, smallest diffs first. Each item: the failing test, the fix, the sabotage CAUGHT, then
`check_all` (everyday) exit 0; one HANDOFF-LOG entry per batch. Nothing is committed until the designer asks.

| Batch | Items | Needs | Proof |
|---|---|---|---|
| A, correctness | CR1, CR3, CR4, CR13, CR6, CR21 (F48) | nothing | new engine tests; `paw.PadPlay` asserts the prompt; one `-game` look from the title |
| B, words | CR2 (with F47, its unreadable slot), CR15 | the designer's approval of up to four `ST_UI` lines (drafted first) | title tests per load outcome; Quick Save test with saving off |
| C, frame | CR5 step 1, CR7, CR8, CR10, CR16, CR17; then CR5 step 2, CR9, CR12 | a quiet PC for the bench (HANDOFF-LOG (89)) | renderer and settings counters; `packaged_bench.py` before / after, with `paw.DebugHud 0` |
| D, hygiene | CR11, CR14, CR19, CR20 | nothing | existing tests stay green; one test per moved static's reset |
| E, the designer's calls (D16) | F36 (refund the Bait lost to a quit mid-boss?), F3 (Attack Speed can lower DPS with Patience) | the designer; pawsim numbers first for F3 | doc 01 §5 |
| F, Godot's faults the port copies today (D16) | F34, F35, F37, F38, F39, F40, F42, CR22 (F49), F51 (leaving without saving keeps the game; found the night of 2026-11-10) | how the fixture tests carry a change (doc 01 D16; recommended: a patch per fault applied to the fixtures' scratch copy of Godot) | each fixture regenerated by name; a test of the port's rule; a sabotage each |

Estimated size: A about half a day; B a day after the words are approved; C one to two days with measurements; D half a
day; F a day, the fixture mechanism first.

**Scheduled**: batch A, the code half of C and the small items of D for the night of 2026-11-10, on the designer's go
(`WORKLIST-NIGHT-2026-11-10.md`).

**Done the night of 2026-11-10** (HANDOFF-LOG (93), (94); each with a test seen failing first and a sabotage CAUGHT):
CR1, CR3, CR4, CR13, CR6, CR21 (batch A); CR5 step 1, CR7, CR10, CR16, CR18's report line, CR19; CR20. **Moved out**: CR8
(the settings' Godot-parity test counts one write per change, as Godot writes; deferring writes changes that test's
meaning, for a small cost). **Still open**: CR2 and CR15 (the words drafted: `docs/TEXT-DRAFT-2026-11-11.csv`), CR5 step 2
(the numbers do not ask for it: HANDOFF-LOG (95)), CR9, CR11, CR12, CR14, CR17. **Batch F done** the same night (part 2):
F34, F35, F37, F38, F39, F40, F42, F49 (CR22), F51, through `tools/godot_fixes.py` where a Godot fixture carries the
fault (HANDOFF-LOG (95)). Batch E (F36, F3) stays the designer's.
The night's proof (every Godot fixture, the whole realm gate, PawCore's sabotages on what changed, L4, `check_all`):
HANDOFF-LOG (96).

**Batch B done 2026-11-11** (HANDOFF-LOG (97)), the words approved by the designer that morning: CR2 with F47 (every load
outcome on the title; the unreadable slot's card, the `title` fixture made with `godot_fixes.py` F47) and CR15 (the quick
bar and the pause menu say "Saved" only for a write that happened; write tickets). **F36 decided and done** the same day
(batch E: refund the summon's Bait and Catnip on a mid-fight save). **Still open**: F3 (the designer's), CR5 step 2, CR9,
CR11, CR12, CR14, CR17.

**After batch C**, the frame gate's draw-call question (doc 01 Q11) should be re-asked with clean numbers: the scene's own
count (`showflag.Rendering 0` split) without the debug overlay. **Measured** the night of 2026-11-10 on a quiet PC
(HANDOFF-LOG (95)): 67.7 for the whole frame with the overlay off (68.7 with it), 41.6 without the scene, so the scene's
own ~26. Ready for the designer.

## 5. Reviewed and sound (so the next review does not redo it)

- Build: unity off in both modules, no PCH for PawCore, editor-only module separate; config: no Lumen / RT / Nanite /
  Substrate, FXAA, `r.EarlyZPassOnlyMaterialMasking=False`, Common UI cvars in `DefaultEngine.ini`.
- PawCore: the combat resolver (brackets separate, `PlayerDps` for Pounce and poop, Pounce capped on the product), stats,
  cards (every level read bounds-checked), the save reader and writer (depth limit, duplicate keys, out-of-range numbers,
  non-finite values zeroed on load, CRC and length checked), number formatting (no undefined casts), the field's step order
  and travel guard after a boss.
- The save subsystem: one pipe, temp + verify + backup, never copying a broken slot over a good backup, recovery at startup,
  the sandbox (`-NoSave`, `-PawSaveDir=`, automated runs off), `Flush` before every read, a synchronous save on exit.
- Subsystem dependencies initialised (`InitializeDependency`), tickables never tick their CDO, delegates removed in
  `Deinitialize` or bound weakly.
- The realm's fixed step (spiral guard, snapshot before each step, interpolation), pause holders paired on activate /
  deactivate, input configs on every screen (the HUD's capture during mouse down), the input preprocessor first.
- Shipping compiles out the cheats, the bench, the tours and their console commands; the debug HUD class is not used in
  Shipping.
- The data adapter refuses data the validator rejects (no game on bad data); string tables registered at module start.
- The widget baker never bakes over an existing asset; `check_all.ps1` exits 2 on a skipped gate.
