# Night worklist, 2026-11-10 (unattended, from the designer's go to about 07:30)

> Written 2026-11-10 22:00 from HANDOFF-LOG (92) and `WORKLIST-REVIEW-2026-11-10.md` (designer: "Create an overnight
> plan"). **Closed 2026-11-11 06:08** (HANDOFF-LOG (96)): every step done or written up below, the proof green, the editor
> may be opened. HANDOFF-LOG (93)-(95) carry each step's results; (96) sums up the night.

The night does the review's fixes that need no decision: batch A (correctness), then the code half of batch C and the
small items of batch D, each proven by a test that fails on today's code. It measures the frame on a quiet PC (the debug
overlay's share, the Slate invalidation A/B left open in (89)), drafts the words batch B needs, and gets the questions
ready. A player sees three changes only: the press prompt is back in the realm (CR1, what was meant), the stick no longer
moves the cat under a paused menu (CR6, as in Godot), and the title no longer rewrites the loaded slot every 30 s (CR21, a
Godot fault not ported: D16, designer 2026-11-10, "Do not port faults from godot").

## Before the go (the designer)

1. **Close the Unreal editor, and leave it closed until the closing entry says the night is done.** A sabotage run builds
   broken DLLs on purpose until its clean rebuild; an editor opened in between loads them.
2. **Leave the PC on, signed in and unlocked, with this session open.** Sleep is 30 minutes on mains (`powercfg`, read
   21:55): the night holds a keep-awake request from a background process (no power setting changed; it ends with the
   night). A locked screen can throttle the game windows that the bench and the pad tour open.
3. **Say whether the night may package**: two Development packages into `build/package` (git-ignored, about 5-15 min
   each, nothing leaves the PC), for before / after frame numbers. Without it, step 1 runs on the existing package from
   (85) and step 5 is skipped.

## Ground rules (CLAUDE.md)

- No commit, push or export; a package only with the go's yes. The Godot repo is read only tonight (its findings file got
  F47-F50 before the night).
- **Never `git checkout`, `restore`, `stash` or `reset`**: the tree holds everything since `b0ce7be`, uncommitted. Every
  file is copied to `build/night/backup/` before its first edit tonight; undoing an item = copying its files back.
- Nothing a player reads or sees is changed by choice: no words (CR2 and CR15 are drafted only), no layout (no bake, no
  graft, no asset edit), no look, no rule or number beyond the listed fixes: CR1 restores what was meant; CR6 matches Godot
  (`player.gd` sets no `process_mode`, so its `_process` stops with the paused tree); CR21 is a Godot fault fixed here
  (D16, F48). Batch F, the other Godot faults, waits for its fixture mechanism.
- Each fix: its test FAILS on today's code first (a test that cannot fail means the finding is wrong: written up, nothing
  changed); then the fix; the test passes; a new sabotage in `tools/sabotage_realm.py` is seen CAUGHT alone (`--only`).
  Existing tests are never weakened; a sabotage whose target a fix rewrote is retargeted and seen CAUGHT.
- One test run at a time. While `check_all`, a bench or a sabotage run is going, only `docs/` and `build/night/` are written.
- A failure whose cause is not clear within the hour is written up, that item's files are restored from the backup, and the
  night moves on. The bench's own FAIL on draw calls is doc 01 Q11 (the designer's), not a night failure.
- An Unreal process of this project appears (`tools/unreal_procs.py` exit 1): no further build, the state is written, the
  night stops. Another project's UnrealBuildTool running: wait for it before a build or a package (`ConflictingInstance`,
  KB U/24). A measurement taken beside another project's jobs is marked noisy.
- PawCore is not touched tonight (CR17's `PcField` part waits), so PawCore's 628 sabotages need no rerun.

## The night

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight**: the guard exit 0; other Unreal processes listed; the keep-awake on; both repos' `git status` saved (`build/night/status_before_{port,godot}.txt`); the everyday `check_all` on the untouched tree (`build/check_all_night0.log`) | 15 min | exit 0 (otherwise that failure is the night's first item) | **done** 22:20. Go 22:13 ("Go.package ok"). No Unreal process of any project; keep-awake on 22:14 (`build/night/keep_awake.log`); status saved; `check_all` exit 0, 27 of 27 (`build/check_all_night0.log`) |
| 1 | **Frame numbers, before** (quiet PC). On the existing package (85), `packaged_bench.py` three times each with `--exec`: (a) nothing; (b) `paw.DebugHud 0`; (c) (b) + `Slate.EnableGlobalInvalidation 1`; (d) (b) + `showflag.Rendering 0` (the frame without the scene: the HUD's share, for Q11). With the go's yes: package tonight's starting tree, then (a) and (b) on it | 30-50 min | the medians (game thread mean and p99, draw calls, GPU) in HANDOFF-LOG (93) | **done** 22:28, on a fresh package of the starting tree (128 s; kept as `build/night/package_before`). `build/night/bench_before.md`: draws 68.65 (a) -> 67.65 with the overlay off (b): **the overlay is 1 draw call**, not the HUD's 43; the frame without the scene (d) 41.7. Game thread mean / p99 a 2.68 / 5.05, b 2.36 / 4.39, c 1.81 / 3.34, d 2.10 / 3.11 ms; GPU 0.46 ms. **Noisy**: another project's headless engine runs (Cabbage) during 11 of 12 runs; the draw calls are not affected by that |
| 2 | **Batch A**: CR1, CR3, CR4, CR13, CR6, CR21 (table below) | 3.5-4 h | each item: test failing, fix, test passing, sabotage CAUGHT; then `check_all` exit 0; HANDOFF-LOG (93) | **fixed** 22:56 (35 min): every new test seen FAILING on the old code (`build/night/a1_*.log`: 40 / 16 audio players, 4 / 2 music, 10,000 / 256 log, the old HUD wiping the new one's consumer, the paused field steered, the title writing; L4 "in the viewport 0") and passing after (`a2_*.log`; L4 42 / 42, `build/night/cr1_dock_prompt.png`); 8 new sabotages and 2 retargeted, all 10 CAUGHT, the clean build passing (`build/night/sab_a.log`). `check_all` exit 0 at 23:06 (`build/check_all_night_a.log`); HANDOFF-LOG (93) |
| 3 | **Batch C's code half, batch D's small items**: CR7 (with CR18's report line), CR5 step 1, CR8, CR10, CR16, CR19 | 2-2.5 h | the same; HANDOFF-LOG (94) | **done** 23:42 except CR8: CR5 step 1 (180 empty-band rebuilds in 60 frames -> 0), CR16 (3 index builds -> 1), CR10 (21 widget searches in 20 frames with the pointer -> 0), CR7 + CR18 (the bench turns the overlay off and reports the pool), CR19 (the comment); 3 sabotages CAUGHT (`build/night/sab_c.log`). **CR8 moved out**: the settings' Godot-parity test counts one write per change (Godot writes on every set too); deferring writes changes that test's meaning, for a small cost. `check_all` exit 0 (`build/check_all_night_c.log`); HANDOFF-LOG (94) |
| 4 | **Stretch**, in this order, only with step 3 green and before 04:30: CR11, CR17 (game module only), CR12, CR20 | - | the same | **CR20 done** (the window's monitor, not the primary; this PC has two monitors; sabotage CAUGHT, `build/night/sab_d.log`). CR11 (an exit-time release no headless test sees), CR12 (a holder for the glyph textures: a day's design) and CR17 (little for the risk) not done |
| 5 | **Frame numbers, after** (only with the go's yes): package; (a) and (b) again | 30 min | the before / after table in the closing entry | **done** 23:54, both packages on a QUIET PC (no other Unreal process in any of 24 runs; `build/night/bench_after.md`, `bench_before_quiet.md`): draws 68.7 -> **67.7** (the overlay, CR7); the scene's share ~26, the frame without the scene 41.6 (Q11). Game thread mean / p99, render thread and GPU unchanged within run-to-run noise (game 1.5-2.3 / 2.2-3.6 ms, under the 4 ms budget when quiet: the 'near 4 ms' of (85) was another project's load; render 1.1-1.3 ms; GPU 0.45 ms). CR5 step 1 shows nothing measurable in this busy scene (its bands are rarely empty), and the effects cost little: **CR5 step 2 is not asked for by the numbers**. CR18: pool 1,417-1,435 bodies for 1,280-1,287 drawn |
| 6 | **The proof**: `sabotage_realm.py --only` for every sabotage on a file touched tonight (about 190 today: HUD 108, save 18, input 14, view 17, audio 10, settings 4, icons 2, render and atlas; about 1.6 h) and tonight's new ones; `padplay_tour.py`; the final `check_all`. If step 3 ends before 03:00: the whole realm gate (490, about 4 h) instead of the subset | 2 h | every sabotage CAUGHT and the clean build passing; L4 every check; `check_all` exit 0 | moved to part 2 (P6), so one proof covers the whole night |
| 7 | **Desk work**, while 1, 6 and the builds run: the words for CR2 (with F47's unreadable slot) and CR15 in `docs/TEXT-DRAFT-2026-11-11.csv`, through `text_audit.py`'s budgets and VOCAB; batch F's fixture mechanism tried on paper (how `make_godot_fixtures.py` would apply a patch to its scratch copy); the review worklist's states; doc 09's counts; KB tags raised to `[Verified]` where tonight's real windows prove a `[Docs]` rule (U/09 §3 by CR1's L4 check) | 1 h | written | **done** in the waits: `docs/TEXT-DRAFT-2026-11-11.csv` (CR2 with F47, CR15: 7 lines within `text_audit.py`'s budgets and VOCAB); the fixture mechanism built (part 2, P0) instead of tried on paper; the review worklist's states; doc 09's counts (27 / 40 gates, 629 / 516 sabotages); KB U/09 §3 and U/24 raised to `[Verified]` by CR1's L4 check, U/15 §7-§8 and U/18 §5, §8, §9 written (no project names) |
| 8 | **Close**: the night's whole diff from the backups (`build/night/night.diff`); the closing HANDOFF-LOG entry (what changed with its proof, the numbers, the drafts, the questions, the hand checks, the next step), whose first line says whether **the editor may be opened**; doc 00; this list marked closed | 20 min | written | **done** 06:08: `build/night/night.diff` (48 files, 16.8 MB, nearly all of it the three regenerated fixtures) and `night_code.diff` without them (45 files, 2,115 lines); every other source file whose time changed tonight is a sabotage target written back byte for byte; HANDOFF-LOG (96), whose title says the editor may be opened; doc 00; doc 01's F48; doc 09's last run; this list closed; the keep-awake stopped |

**Order and cut lines**: 0, 1, 2, 3, (4), 5, 6, 8; 7 fills the waits. Step 6 starts by 05:30 whatever is unfinished (an
unfinished item is restored from the backup, never left half done). When time runs short, 4 goes first, then the last
items of 3 (they are independent); 2 is never cut for 3. Long runs go in the background; the session wakes when they end.

## The items (evidence and reasoning: WORKLIST-REVIEW §3)

| Item | The test that must fail first | The fix | New sabotage |
|---|---|---|---|
| CR1 | `paw.PadPlay` (L4: a real window, as headless runs have no viewport): after Continue, with a dock row focused by the pad, the prompt is in the viewport and visible; a screenshot `build/night/cr1_dock_prompt.png` | `PlaceFocusPrompt` adds the prompt again when `!IsInViewport()`, as `ShowToast` does | `prompt_not_readded` -> `PADPLAY` |
| CR3 | `Pawpocalypse.Audio.Voices`: through a counting creator (a `TFunction` seam, like `SwapOutput`), 40 plays make at most 16 components, two tracks at most 2 | create each voice and each music player once; reuse it while `IsValid()` | `audio_voice_world_check` |
| CR4 | in `Pawpocalypse.Audio.Rules`: 10,000 plays leave the log at its cap | the log keeps the last 256 | `audio_log_unbounded` |
| CR13 | `Pawpocalypse.UI.Hud.DockConsumer`: an old HUD's destruct leaves a newer HUD's consumer in place | the device subsystem keeps the owner: `SetDockConsumer(Owner, Fn)`, `ClearDockConsumer(Owner)` | `hud_destruct_clears_any` |
| CR6 | `Pawpocalypse.Input`: with a pause held, a deflected stick leaves the field's input unchanged and warps nothing | skip the recentre and the stick path while `IsPaused()` | `view_steers_when_paused` |
| CR21 | `Pawpocalypse.Save` (autosave): with no realm up, 31 s of ticks and a focus loss write nothing (fails today: a write) | the unprompted saves (the 30 s timer, a focus loss) run only while a realm is up; the quit save and the asked-for saves unchanged. If a Godot fixture asserts the old rule, CR21 moves to batch F | `autosave_on_title` |
| CR7, CR18 | the bench report says `debughud=0`, and gives the pool size against drawn | `paw.Bench` and `packaged_bench.py` turn the overlay off | none (tooling: the numbers are its proof) |
| CR5 step 1 | `Pawpocalypse.Realm.Vfx` (a renderer counter): 60 empty frames rebuild no section (fails today: 180) | remember an empty band and skip the clear while it stays empty. Check the `Fill` mesh (the same calls, `PawSpriteRenderer.cpp:339`) for the same pattern | `fx_clears_every_frame` |
| CR8 | a settings test: ten setter calls in one frame make one write (fails today: 10); the sandbox still holds | mark dirty; one write at the frame's end (an `FTSTicker` one-shot); at once in `Deinitialize` | `settings_write_every_set` |
| CR10 | `Pawpocalypse.Input`: with the pointer last used, placing the prompt does no path search (a counter) | the cheap check first | `prompt_search_first` |
| CR16 | `Pawpocalypse.Realm.Look.Atlas`: with two cells sharing an id, two lookups build the index once; the shipped atlas's ids are unique | a `bIndexed` flag, reset on load and on edit | `atlas_index_by_count` |
| CR19 | none (a comment) | say that the settings write comes from the game thread, and why that is safe | none |
| CR11 | the `.Icons` tests, their two sabotages retargeted | `DA_Icons` held by a game-instance subsystem's `UPROPERTY` | retargeted |
| CR17 | the existing realm tests (no behaviour changes) | the game module's allocations listed in CR17 | none |
| CR12 | glyph textures and resolved classes survive a garbage collection | strong references held by a subsystem | `glyph_cache_weak` |
| CR20 | a pure function picks the monitor holding the window's centre (two monitor rectangles) | fullscreen uses it | `fullscreen_primary` |

**Not tonight, and why**
- **CR2 (with F47), CR15**: new words. Drafted tonight (step 7), built once the designer approves them.
- **Batch F** (D16: the Godot faults the port copies today: F34, F35, F37-F40, F42, CR22): each changes a Godot-fixture
  test, so the mechanism comes first (doc 01 D16).
- **CR5 step 2**: waits for step 5's numbers (step 1 and CR7 may be enough), and it changes how the effects are built: a
  look risk to check with the designer's eyes.
- **CR9** (the tree's tooltip): the tree waits on a review (R4's zoom). **CR14**: cross-cutting (HUD, tree, the tests'
  fixtures), a day session. **CR18**: a watch item; its report line goes in with CR7.
- **R6**: its direction board needs the designer first. **The nightly `-Full`** (about 10 h) does not fit beside the fixes;
  step 6 runs the part tonight's changes can break (the realm sabotages of the touched files, L4). The next free night
  runs it whole.

## Settled by D16 before the night (designer, 2026-11-10: "Do not port faults from godot")

- **CR21** is a Godot fault (F48: Godot autosaves on the title too, against its timer's own comment), fixed tonight in
  batch A.
- **CR2's unreadable slot** (F47) goes with CR2's words (batch B); **CR22** (F49) is batch F.
- **CR23** was stale docs (F50): Godot's economy rework (2026-08-24) shipped the curve its redesign planned, and the GDD and
  two comments still describe the old ceiling. Doc 02 corrected; nothing to decide.
- Left for the designer: F36 (refund the Bait lost to a quit mid-boss?) and F3 (the Attack Speed trap), doc 01 §5.

## Part 2: the follow-on plan (designer, 23:27: "When all this done create an overnight plan and do it without asking me")

Written 23:37, run without asking. It starts when part 1's step 5 (the frame after) is done, and it takes part 1's step 6
(the proof) to its end, so one proof covers the whole night. Its subject is the designer's newest rule, D16 ("Do not port
faults from godot"): batch F, the Godot faults the port copies today. The ground rules above still hold (no commit, push or
export; no new package; the Godot repo read only; backups before every first edit; a failing test first; sabotages CAUGHT;
one run at a time). Decisions that are the designer's stay open (F36, F3, Q11, the words); the fixture mechanism is a
technical choice, made here as doc 01 D16 recommends.

| # | Work | Done when | Status |
|---|---|---|---|
| P0 | **The mechanism** (doc 01 D16): `tools/godot_fixes.py`, the port's fixes to Godot's code as exact text replacements (one per finding, each applied exactly once, a no-op refused, a self-test), applied by `make_godot_fixtures.py` to its scratch copy only. Every other fixture stays byte-identical | the tool and its self-test; `make_godot_fixtures.py --check` on an untouched fixture still identical | **done** 23:59: `tools/godot_fixes.py` (5 replacements: F34, F37 x3, F49), its self-test (a stale target, a twice-found target, a no-op, the repo itself: all refused; CRLF kept); `--check --only amounts` byte-identical; the Godot repo's `git status` unchanged. New everyday gate `Godot fixes current` (each fix still finds its target in the Godot repo) |
| P1 | **F34**, the pause menu's armed overwrite survives leaving the Save page (one press overwrites a slot): Godot's fix in `godot_fixes.py`, the `pause` fixture regenerated (the port's test now FAILS against it), then the port (PcPause) | L1 + the pause tests pass; a PawCore sabotage CAUGHT | **done** 00:01: the regenerated `pause` fixture made L1 FAIL ('pending' armed), the port's `PauseShowMain` now disarms; L1 and `Pawpocalypse.UI.Pause` pass; the old sabotage `pause_main_disarms_f34` (which added the fix) replaced by `pause_main_keeps_overwrite_f34`, CAUGHT. The fixture changed in nearly every line: its presses come from one random stream, so the first changed state shifts every later press (the replay reads the recorded presses) |
| P2 | **F51**, leaving without saving keeps the game (Continue plays it): the port reloads the slot on a discard | a pause test seen failing, then passing; sabotage CAUGHT | **done** 00:06: `Pawpocalypse.UI.Pause.DiscardDrops` (999 unsaved kills kept, want the saved 100) and `.Leaves` (never saved: a fresh game) FAILED, then pass (`UPawSaveSubsystem::DiscardUnsaved`); 2 sabotages CAUGHT |
| P3 | **F40**, five sounds play twice: one of each pair (`Pawpocalypse.Audio.When` asserted the doubles) | the test asserts one, seen failing first; sabotage CAUGHT | **done** 00:10: `Pawpocalypse.Audio.When` now expects one of each, FAILED (2), then passes; `audio_reawaken_once` replaced by five `*_twice_f40` sabotages, all CAUGHT |
| P4 | **F39**, two rings at twice the cat's position: at the cat | a test of the ring's place, seen failing first; sabotage CAUGHT | **done** 00:13: `Pawpocalypse.Realm.Vfx.RingOnCat` FAILED (0 points round the cat, 96 at twice its position), then passes; sabotage CAUGHT |
| P5 | **F35, F37, F42, F38, F49** in this order, each the same way, while time is left | the same | **all done** 00:44. F35: `UI.Settings.FullscreenGreys` FAILED then passes (`.Applies` had asserted Godot's way; the fixture never presses Fullscreen, so no Godot fix needed). F42: `UI.Juice.BaitLean` FAILED (5 restarts in 5 refreshes) then passes. F37: Godot fix x3, the `controls` fixture regenerated (44 rows), L1 FAILED (440 cells) and the engine replay FAILED (180), then both pass (`OnDetectedChanged`). F49: Godot fix, the `save` fixture regenerated (438 rows), L1 FAILED (438) then passes; a PawCore sabotage retargeted, one added. F38: `Realm.Look.Rules` FAILED then passes (`FacingOverlay`, `HatFlipX`, `WeaponFlipX`). Sabotages all CAUGHT: realm 9, PawCore 2. The view's use of the facing (like CR6's `R->IsPaused()`) is covered by reading only |
| P6 | **The proof** (part 1's step 6, moved here): the realm sabotages of every file touched tonight, the PawCore sabotages of every PawCore file touched, `make_godot_fixtures.py --check` (every fixture reproduces with the fixes), the L1 long suite if PawCore changed, L4, the final `check_all`. Starts by 04:30 whatever is unfinished (an unfinished item is restored from the backups) | all CAUGHT, all identical, exit 0 | quick half **passed** 00:50-01:42 (`build/night/proof_quick.log`): every Godot fixture reproduces byte for byte with the port's fixes (1,091 s); the 43 PawCore sabotages on PcPause and PcSave all CAUGHT (795 s); the L1 long suite, 36 cases, 9.6 M assertions (1,233 s). Long half **passed** 01:43-05:59 (`build/night/proof_long.log`): the whole realm gate, 516 of 516 CAUGHT and the clean build passing (4 h 8 min, `proof_realm_sab.log`); L4, 44 of 44 (`proof_padplay.log`); the final `check_all`, 27 of 27 (`build/check_all_night_final.log`). Found while closing: the list of PawCore files above named SOURCE files, so the Controls page's 12 sabotages (for F37 its test and fixture changed, its source did not) were not in it; run after the proof, all CAUGHT (`build/night/proof_controls_sab.log`) |

## For the designer in the morning

- The closing entry (HANDOFF-LOG (96)), then the checks no test can make: the pad prompt on the dock (CR1; the screenshot is in
  `build/night/`), the sound in a busy biome and the boss music in and out (CR3: nothing should sound different, no track
  left playing), the stick held while a card pick opens (CR6), the title's slot time after a minute there (CR21); and part
  2's: the window on the second monitor with Fullscreen ticked (CR20), the steered cat's hat and weapon (F38), the Bait
  button when it calls (F42), the Controls page after picking up a different pad (F37).
- The drafts (CR2 with F47, CR15); Q11 with the overlay's share now measured; F36 and F3 (D16); batch F's fixture
  mechanism, built as doc 01 D16 recommends (to look over, not to decide).
- Nothing is committed.
