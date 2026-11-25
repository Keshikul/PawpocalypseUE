# Handoff log

Newest first. Every session that changes the state of this project adds an entry: where the project is, what happened, what is open,
and the exact next step. Read `docs/00-START-HERE.md` first, then the newest entry here.

Entries (1) to (127) (2026-10-28 to 2026-11-19) are in [HANDOFF-LOG-ARCHIVE.md](HANDOFF-LOG-ARCHIVE.md). A reference such as "HANDOFF-LOG (41)" in any doc means the numbered entry,
in whichever file holds it.

---

## 2026-11-25 (148) - The bosses drawn bigger than the cat (D25)

**Where the project is.** As (147), plus this entry's work, uncommitted. The designer: "Please make the bosses larger.
They should be bigger than we are". No package was made.

**What happened.**
- **Measured first** (each drawing's opaque area at the size the game draws it): at x0.9 of the 256 px art (Godot's
  boss.tscn, 230 px) the Park Tyrant, the first boss, covered 15,200 px2, the same as the stage-1 cat at 175 px; the
  Foreman 22,300, the Litter King 35,800.
- **`PawLook::BossScale` 0.9 -> 1.4** (`Realm/PawFieldLook.h`; 358 px drawn; the death collapse reads the same number).
  The Park Tyrant is now 341 x 208 px visible (2.4x the cat's area), the Litter King 341 x 341 (5.7x). The shield
  countdown ring (`PawVfx.cpp` `TelegraphRadius`) is 0.45 of the drawn size (104 -> 161 px), so it still hugs the body.
- **The atlas**: the bosses packed at 2x the new size (720 px cells); `tools/pack_atlas.py` and `tools/art/look_lint.py`
  now read BossScale from `PawFieldLook.h` (one number, never two); one 4096 page, 63 % full (was ~55 %);
  `import_art.py --run atlas` run. `review_anatomy.py` and a `sweet_anatomy.py` comment follow.
- **Look only, no rule changed**: the paw still hits the boss within 62 px of its centre (`base_paw_reach`), the kittens
  go in when the cat is inside the aura's radius of it; the bigger body does not widen either. The arena inset (220 px)
  still holds the largest boss's half size (179 px) inside the field.
- Docs: doc 01 §2 D25.

**Proof.** `check_all` (everyday) exit 0, all 37 gates PASS (`build/check_all_20261125_148.log`; the field atlas
reproduces, look lint, L3 `Pawpocalypse.Realm` with the atlas import check, the Shipping game build). Every boss shot in
its own biome with the cat beside it (a scratch run, sandboxed saves, 1920x1080): each clearly bigger than the cat, none
under the HUD.

### Open (the designer's)
- Is 1.4 right? One number in `PawFieldLook.h` (then `pack_atlas.py`, `import_art.py --run atlas`).
- Should hits count anywhere on the bigger body? Today the paw reaches 62 px from the boss's centre (the drawing now
  reaches ~170 px out), so a cat on the boss's edge does not hit it. Widening that is a rule change (PawCore, its tests).
- **The store media of (147) show the old boss size**: screenshot 02, the boss GIF and clip, and the trailer's boss
  segment. Retake on the word (`store_shots.py` en + de whole runs: 02 follows 01's state; `store_clips.py --only boss
  boss_ui`, then `--cut`).
- As (147).

### Next step
The designer's answer on the size and the hit reach; then retake the boss media (above).

## 2026-11-25 (147) - The Steam screenshots retaken; GIFs, clips and a trailer recorded in the game (no text in the GIFs)

**Where the project is.** As (146), plus this entry's work, uncommitted. The designer: "Read the handoff. Retake the
screenshots and gifs and video please. Make sure there is no text in gifs". No package was made.

**What happened.**
- **Screenshots** (`tools/store_shots.py`): English 9, German 9, French and Portuguese 01, Russian 01/04/05, Thai 01/04
  (the same set as 2026-11-19). Two scenes restaged: **01** buys a few rows and fills the field at 5x first (the plain
  12 s wait left a sparse field and every row at Lv 0); **07** buys 22 nodes and zooms in (new `paw.Zoom`; fitted, the
  node names could not be read at 1920x1080).
- **Clips, GIFs, trailer: new `tools/store_clips.py`** and new dev steps in `Dev/PawShot.cpp` (header comment lists
  them): `paw.Clip <name> <frames> [ui] [append]` (a BMP every frame, without the widgets unless `ui`), `paw.Steer
  hunt|boss|off` (a spring-damped hand on the cat: the thickest crowd near it, or beside the boss), `paw.Pounce`,
  `paw.Quit`, `paw.Dock off` (the field the whole window wide), `paw.AutoPick`, `paw.Own <branch>`, `paw.Row <n>`,
  `paw.Ever <stardust>`, `paw.Camera <zoom>` (a punch-in that follows the cat, rendered, not cropped), `paw.Zoom`; every
  step is logged (`PAW_STEP`) and, while steering, the field once a game second (`PAW_FIELD`: live, kills, boss HP).
  Run with `-UseFixedTimeStep -FPS=60`. 11 takes: 6 **text-free** (hunt, aura, pounce, boss, realms montage, rider:
  no widget captured, the field draws no text, every GIF's contact sheet checked) and 5 with the HUD for the trailer
  (upgrades, cards, boss, Reawaken + tree, Unicorn). Out: 6 GIFs (780 px, 4.3-7.1 MB), 6 webm + mp4 clips (1170x658,
  1.5-3.0 MB), a 57 s trailer (1080p60, the first try's CC0 score from its measured bar, the drop on the aura, the logo
  last, no captions, no game sound).
- **Staging, measured** (`PAW_FIELD`): every upgrade maxed kills each enemy on arrival and the maxed kittens fill the frame
  (the first takes showed no enemy); MaxUpgrades alone beats the Park Tyrant 6-9.5 s after it arrives (with a Reawaken
  multiplier it died before the take began); a Reawaken multiplier alone in realm 8 gives 1-2 kills a second (the paw's
  rate is the limit); Rainbow Poop, and the aura with a big multiplier at 1.6x, emptied the view. `paw.Row n` presses
  `Pc::ShopRowOrder()[n]`, which counts the hidden Aura Radius (5): the first runs bought Currency Find for Spawn Rate.
  Takes differ run to run (the realm's seed, the auto-picked cards): review every take.
- **Game code, no player-facing change: `UI/PawUiTime.h/.cpp` `PawUi::StepDelta`**. Slate ticks widgets with its REAL
  delta even under a fixed step (and `Slate.UseFixedDeltaTime` does not reach a widget's tick: the paint passes
  `GetDeltaTime()`), and a screenshot with the widgets paints the window twice: the Unicorn ceremony's 3.75 s lasted
  ~0.6 s of a take. Now the 13 widget ticks that drive their own motion step by Slate's delta in play (no fixed step,
  unchanged) and by the fixed step once per engine frame in a recording. Test `Pawpocalypse.UI.StepDelta`; sabotages
  `stepdelta_real_time`, `stepdelta_every_paint` CAUGHT.
- **Engine facts found (KB U/18 §4, U/24, 2D/02, INDEX)**: an ortho camera component moved off its actor does not pan (the
  auto planes centre the view on the owner): the punch-in is a rig actor; the auto planes shrink with the ortho width (2x
  cut the cat's body): the rig's `AutoPlaneShift` -600; BMP frames, not PNG (720 frames in ~20 s instead of ~3.4 a
  second); **the editor build's `-game` exits 0xC0000005 after its log closes on every quit** (also with the committed
  `PawShot.cpp` and a plain `quit`: not this work; the crash reporter then holds the process ~1 min, so
  `store_clips.py` ends it once the log has closed); a source restored with an older mtime left UBT's DLL stale (the
  first batch ran without the new steps and waited forever: the step log is why it is now plain to see).
- **Drive**: `Promotional/Steam/2026-11-25` (screenshots + `_1920.jpg`, localized, gifs, clips, trailer, contact sheets,
  README); the 2026-11-19 screenshots moved to `_Archive/Steam_screenshots_2026-11-19`; the map updated. The 11 takes stay
  in `build/steam/clips/takes` (lossless, 6.8 GB, untracked): `py tools/store_clips.py --cut` re-cuts everything from them
  without the game.
- Docs: CLAUDE.md (the tool), STEAM-PAGE-DRAFT (the media section, the open item).

**Proof.** `check_all` (everyday) exit 0, all 37 gates PASS (`build/check_all_20261125_147.log`; L3 `Pawpocalypse.UI` with
the new test, the Shipping game build). `run_automation Pawpocalypse.UI.StepDelta` PASS; `sabotage_realm --only
stepdelta_real_time` and `stepdelta_every_paint` CAUGHT, the clean build passes; safe_mutate: no mutation left.
`store_shots.py`: 9 of 9 (en), 9 of 9 (de), 1 + 1 + 3 + 2 (fr, pt-PT, ru, th). `store_clips.py`: every take's frames
complete (11 takes); every GIF's and clip's contact sheet looked at (`build/steam/clips/contact`): no text.

### Open (the designer's)
- Look at the new media (Drive `Promotional/Steam/2026-11-25`); pick the clip for each About block (upgrades, cards and the
  tree exist only with the HUD, so only in the trailer).
- The trailer has no game sound (frame-by-frame sound is unsolved, KB U/18 §4) and no captions (new words would be
  drafted first); its music is the first try's CC0 track.
- As (145) and (146); commit (144)-(147) on the word.

### Next step
As (145). The store page's screenshots and clips are ready to upload once the page goes up.

## 2026-11-24 (146) - The Drive folder laid out like BrineTime's; the Steam work, builds and logs in it

**Where the project is.** As (145) (nothing in the repo changed but docs 00 and this log). The designer: "Create a folder
in drive and put all the things in there. Look at how the cabbage drive folder is structured and do the same."

**What happened.** `My Drive/Games/BrineTime` (the Cabbage project) is `Builds/` (dated build zips), `Logs/`, and
`Promotional/` with a README map, a fonts-and-colours sheet, dated Steam sets (`upload`, `template`, `screenshots/final`),
`localization`, `Fonts`, `Icon`, `Logo`, `Research` and `_Archive` (nothing deleted). `My Drive/Games/Pawpocalypse`
already held the first try's Promotional set (2026-11-03) and the phase-4 zip; it is now laid the same way (through Drive
for desktop, `G:\My Drive\Games\Pawpocalypse`):
- `Builds/`: the phase-4 zip (moved) and `Pawpocalypse-2026-11-24-soak.zip` (the Development package of (144), 208 MB:
  before D24 and the claim fix; no new package was made).
- `Logs/Pawpocalypse-logs-20261124.zip`: the soak, PSO and bench reports, the econ measurements (D24's levers), the
  packaged languages check, the package and audit logs, the gate logs of (141)-(145).
- `Promotional/`: `README.md` (the map), `FONTS-AND-COLORS.md` (written from `tools/art/palette.py` and `tools/fonts.py`),
  `Fonts/`, `Icon/`, `Logo/`; `Steam/description.md` (the page text, version 3), `Steam/2026-11-18/` (the store set:
  `upload/` the 13 images, `template/` their HTML, script and size checks, its 9 screenshots), `Steam/2026-11-19/`
  (the newer screenshots and the German, French, Portuguese, Russian, Thai retakes; its README says to retake: the HUD,
  boss panel, tree, ceremony and victory screen were restyled after them), `Steam/localization/` (21 languages);
  `Research/` (the approved concepts, the drafts waiting for the designer, the 11 comparable store pages); `_Archive/`
  (the 2026-10-03 Steam kit, the first try's trailer, itch page and PlayStation placeholders, with a README).
- Found on the way: **the game's exe has no icon of its own** (no `.ico` anywhere: Unreal's default); the store's app icon
  exists (`Promotional/Icon`). Drive's Steam folder still had version 2 of the page text; `description.md` is version 3.
- The upload (about 230 MB) runs in Drive for desktop's background.

### Open (the designer's)
As (145), less `build/steam` living only on this PC (it is on Drive now); plus the exe's icon (from `Promotional/Icon`,
on a go) and the screenshots' retake before the store page goes up.

### Next step
As (145).

## 2026-11-24 (145) - D24 (the pace slowed: 113 -> 175 min); the docs brought up to date; the clean-up; a code review (one bug fixed)

**Where the project is.** Pushed through (143) (`8e773dc`); (144) and this entry are uncommitted. The designer: "The pace
seems to be too fast. Slow it down. Make sure all docs are up to date. Go through all docs and remove unnecessary ones.
Also if there are any unused assets that we used to use and don't anymore. Clean up the project. Clean up any other files
that we created temporarily. After that do a very thorough code review. Look at the big picture of the project and if all
systems interact properly". No package was made (the package in `build/package` is (144)'s, before D24).

**What happened.**
- **The pace, first a correction**: (143) and (144) compared the soak with plain pawsim (161-164 min), which has no waves;
  `tools/econ_review.py` (pawsim with the game's waves) said 113 min, and the 5x soak won at 113.6: the game was exactly the
  length D17 chose. Told to the designer with the options. **Eleven levers measured** in memory (`build/econ/pace_*.json`,
  table in ECONOMY-REVIEW §10): only more kills a Bait lengthened the game inside D17's goals; fewer Stardust or a weaker
  Catnip multiplier brought the waits back (17-41 % of play). The designer chose **"2h 55m: 2x Bait + tougher bosses"
  (D24)**: `Data/BossData.csv` `kills_per_bait` x2 (480 / 1,622 / 3,000 / 4,400 / 5,600), `total_hp` x5.7 / x3.5 / x4.9 /
  x3.2 / x2.7, so the first fight at Bait-ready lasts 91-186 s. `econ_review` on the built data: **175 min**, 14
  Reawakens, waits over 3 min 12 % (longest 5.6 min), nothing new at most 17.2 min. DataTables re-imported
  (`DT_BossData`). Tests: PawCore "the data" asserts the ten numbers; **the boss fixture test failed every row** (it loaded
  the shipping HP), fixed by holding the HP it was recorded with (`PcBossTests.cpp`): a rule change still fails it.
  Recorded: doc 01 D24, doc 02, doc 06, ECONOMY-REVIEW §10, the Steam draft's length note. Old saves (the designer's test
  saves only): Bait spent before D24 counts against half the kills now, so with Deep Pockets the next boss also needs that
  shortfall; no migration written (pre-release).
- **Docs**: doc 00 rewritten for the game as it stands (phases, the look, the gates, what waits on the designer, what is
  where); doc 11 a status table; doc 09 §7 current (37 everyday gates, 49 in `-Full`, the nightly owed since 2026-11-12);
  doc 06 (the data is edited here; the exporter's history), doc 07 §0 and doc 05 §7 (Sticker Box as built, Nunito; plum
  as history), doc 01 (D12, D14, D18, Q8 statuses; the brief marked history), docs 02, 04, 08, 12; the worklists' status
  lines (STICKER: S0-S5 done; SLICE6: **6b, 6c and 6d are not built**: no Reawaken banner, no Boba cameo, no tutorial).
  HANDOFF-LOG entries (73)-(127) moved to the archive (the live log 3,183 -> about 820 lines; nothing lost, checked).
- **Removed** (all in git history): docs: 5 closed night worklists, the 2026-11-10 review worklist (closed in (100)), 4 text
  drafts (every line approved and in the tables), appendices A, B, C (generated from Godot; A would have gone stale with
  D24). Tools that read the Godot project (D22): `export_godot_data`, `export_code_constants`, `export_input_map`,
  `export_audio`, `list_godot_code_constants`, `make_port_map`, `make_godot_fixtures` and `tools/godot_fixtures/` (72
  files), `godot_fixes`, `check_godot_reawaken_travel`, `check_pawsim_parity`, `gen_pc_data`, `gen_pc_enums`,
  `gen_tree_tables`; pawsim's `--data tres` choice. `godot_tour.py`'s Unreal half kept as **`tools/screen_tour.py`** (this
  game only, its own translations for the font sheets, the Unreal guard added; run: 18 of 18 screens shot). The headers
  those generators wrote now say they are edited by hand; a dev error message and L0's docstring named deleted tools.
- **Unused assets**: a headless registry scan (461 assets, 105 with no referencer; each checked against the C++ that loads
  by path or constructed name): ONE dead, **`FF_MPlusRounded1c_Black`** (plum's title face, unused since D18): out of
  `tools/fonts.py`, `PawFontSpec.h`, `import_fonts.py`, `review_d3.py`; the asset and its 3.6 MB `.ttf` deleted.
- **Temporary files**: 11.3 GB out of `build/` and `Saved/` (old packages and zips, 12 old symbol folders, the Godot
  scratch copy, night and tour outputs, crash dumps, staged builds, rotated logs, `tools/__pycache__`). Kept: the current
  packages and their symbols, the test build, `build/steam` (the Steam page's work), the econ / soak / PSO / bench /
  language reports, the newest proofs, and **`build/*_godot_full.tsv`** (the long suite's full fixtures, 158 MB, no longer
  reproducible: noted in docs 00, 06, 09).
- **The code review** (the seams between systems; the findings below). Found and fixed: **a claimed achievement left the
  field's stats as they were**: the reward raises the Stardust ever granted, which the damage multiplier reads, and the
  field kept the old multiplier until the next purchase (`Pawpocalypse.UI.Hud.Achievements`, made first: FAILED, 1.00
  against the rules' 1.18; `UPawHudScreen::PressClaim` / `PressClaimAll` now rebuild the field's stats; sabotages
  `claim_leaves_field_stats`, `claim_all_leaves_field_stats` CAUGHT). Sound, checked: PawCore engine-free and L0-gated; one
  parser for every route to the data; the fixed step (no time while paused, capped catch-up with dropped time counted,
  per-step interpolation, a travel mid-frame); the pause holders (six screens, each holds on activation and releases on
  deactivation); every close path of the tree through `PressClose`; saves (atomic, coalesced, flushed on quit, recovered);
  no raw or non-weak delegate binding in the game module; every UObject member a `UPROPERTY` (one const pointer is a cache
  key); no hand-set input mode; the dev code out of Shipping.
- **The soak on D24** (editor build, 5x, `build/soak/editor_20261124_214341_d24_5x.txt`): **won at 175.1 game minutes**
  (35.1 real; 13 Reawakens, 279 tree levels, 0 refused, 0 Error / 0 Warning lines; worst frame the tree's first spend step,
  122.8 ms; no stall), matching econ_review's 175. **But its first Park Tyrant fight timed out**: it Reawakened at game minute
  47.9 holding its 40 Bait (D17 E2 keeps them), came back and summoned at 50.9 with a fresh run's cat (~10,000 DPS), took
  the boss to 40 % in the 300 s limit and lost the Bait: the rematch came at 90.4, 35 game minutes later (D17's HP fell to
  the same cat). econ_review prices a fight at the moment the Bait is ready, not after a Reawaken. Measured in memory: the
  Park Tyrant at x3.0 of D17's HP instead of x5.7 (`hp3x0.526` on today's data) leaves the game at 175 min with the same
  waits, its first fight 47 s at Bait-ready; the soak's fresh cat would have won in about 260-270 s of its 300. The
  designer's call.

**Proof.** `check_all` (everyday) exit 0, all 37 gates PASS (`build/check_all_20261124_145.log`; the run before it, on the
D24 data and the clean-up before the review's fix, PASS too: `_145a.log`; one between FAILED `sabotage targets current`:
two older HUD sabotages aimed at the claim lines the fix changed, retargeted and both CAUGHT again). PawCore L1 212 / 212
on D24; the mirror (60,657 assertions) and the 3k fuzz PASS; `import_data --run` PASS (`DT_BossData`);
`run_automation Pawpocalypse.UI.Hud` 17 PASS (the new claim assertions FAILED before the fix: 1.00 against 1.18);
sabotages `claim_leaves_field_stats`, `claim_all_leaves_field_stats`, `hud_awards_claim_pays_every_press`,
`hud_awards_claim_all_dead` CAUGHT; `packaged_bench --self-test`, `packaged_pso --self-test`, the font gates PASS;
`tools/screen_tour.py` 18 of 18 shots; the soak on D24 (above) PASS. safe_mutate: no mutation left.

### Open (the designer's)
- **D24's Park Tyrant can beat a player who Reawakens with the Bait in hand and summons at once** (the soak lost its first
  fight and 35 game minutes): keep x5.7, or x3.0 (measured: the same 175 min), or a longer first time limit.
- **Not built**: the first-Reawaken banner (6b), Boba's photo and cameo (6c), the tutorial (6d). **The Controls page shows a
  "Tutorial tips" check box that nothing reads**: hide it until 6d, or build 6d.
- **Steam integration** (doc 10 §5: achievements, Cloud, the overlay's pause, rich presence; after the App ID); **Report
  a bug opens the first try's itch page** (`UPawMainMenu::BugReportUrl`).
- **The skill tree's first open each session costs 114-127 ms** (every soak's worst frame but the stall): creating the tree
  screen ahead (at the realm's start) would move it off the player's press. A change to do on a go.
- **The card deck's and the achievement roster's order come from `_godot_file`** (the first try's file paths): since the
  CSVs are edited by hand, a new card's made-up path decides its place, and so the deal. An explicit order column would end
  that.
- **`build/steam` holds the Steam page's work only on this PC** (untracked): backed up to Drive in (146).
- The 1 s whole-process stall of the 5x soaks; the owed nightly `-Full`; the Deck pass; commit and push (144), (145).

### Next step
On the designer's word: commit and push (144)-(145); then the choices above (6b-6d, the Tutorial tips box, the tree
pre-creation). Done is `check_all` exit 0.

## 2026-11-24 (144) - Packaged: the PSO fix confirmed over a whole game; the packaged soak wins at 113.6 (5x) and 93.7 (20x)

**Where the project is.** (141)-(143) committed and pushed (`8e773dc`, the designer: "Yes and package and run the soak").
This entry is uncommitted. The Development package in `build/package` (19:29, this source) carries the fixed soak driver.

**What happened.**
- **Packaged** (`py tools/package.py`, Development; UAT 115 s; audit PASS, 392.5 MB; languages PASS; bench PASS on the best
  of three, 3.93 ms, two runs over 4 ms while two other projects' Unreal runs shared the PC). Its third gate, the PSO
  check's bench route, **FAILED with 0 misses**: the route is one `packaged_bench.py` run, and that run's game-thread p99
  (4.64 ms with the validation on) failed the bench's 4 ms budget, minutes after the bench gate had passed the same package.
- **Fixed, the gate**: `packaged_bench.py --no-budget` reports the game thread's p99 and judges every other check;
  `packaged_pso.py`'s bench route passes it (`route_cmd`). Self-tests: bench +6 cases, PSO +1; 4 planted faults CAUGHT
  (scratch `neg_144.py` through safe_mutate: the budget ignored in `judge`, `--no-budget` judging anyway, `clean_pass`
  ignoring it, the route without the flag); no mutation left. Re-run: **PASS**, 0 misses, 6 engine PSOs at start-up, none
  in play (game thread 5.01 ms, reported).
- **The PSO fix of (142) confirmed** (`packaged_pso.py`, the whole game at 20x): **0 of this game's materials missed**
  (M_BackdropFill was the one); 10 engine global PSOs: 6 at start-up, the same 4 at game minute 4 as before (noted).
- **The packaged soak with the fixed driver**: **5x won at 113.6 game minutes** (22.9 real; 12 Reawakens, 252 tree levels,
  8 jobs, 0 refused, 6.95 million Stardust unspent at the win; 0 Error / 0 Warning; memory 987 -> 989 MB) and **20x, with
  the PSO validation on, at 93.7** (6.5 real; 265 levels). Reports `build/soak/20261124_195838-5x.txt`,
  `20261124_200632-20x.txt`; the 5x run's log kept in `build/soak/20261124_195838-5x_logs/`.
- **A whole-process stall in the 5x run, the second**: frames of 1060.7 and 794.3 ms at game minute 65.1 (0.8 game minutes
  after Reawaken 8, the soak only playing), 166 ms at 65.5, 99 ms at 60.2; the log has nothing there but `LogAudioMixer:
  Audio Buffer Underrun (starvation)` (3 in the whole run, all beside these frames): the audio thread starved too, so the
  whole process stood still, not the game thread alone. Windows' System and Application logs: nothing at either stall.
  Another project's tests started 4 Unreal instances at 19:48:14-15 and ran to ~19:49:45; the stall was at 19:48:49.
  (141)'s 0.5 s stall (5x, game minute 144, an underrun beside it) is the same shape. Seen in both packaged 5x runs, in
  none of six 20x runs (3 packaged, 3 editor). Not proven to be the PC.
- **For the designer (not a balance claim)**: the soak's player wins at 113.6 game minutes at 5x and 89-96 at 20x;
  pawsim's v4 player takes 161.

**Proof.** `check_all` (everyday) exit 0, all 37 gates PASS (`build/check_all_20261124_144.log`). `packaged_bench.py
--self-test`, `packaged_pso.py --self-test` PASS; 4 planted faults CAUGHT; `packaged_pso.py --route bench` PASS on the package (`build/pso/report_20261124_bench.txt`), the
whole-game route PASS (`build/pso/report.txt`); `packaged_soak.py --speed 5` PASS.

### Open (the designer's)
- The stall: a 5x soak on an idle PC (no other project's runs); a stall watchdog in the Development build (KB U/15 §5:
  a thread that captures every thread's stack when a frame runs 0.5 s, and how late it woke itself) would name it.
- The pace gap: the soak at 113.6 (5x) against pawsim's 161 (a design question).
- Commit and push (144).
- Still: the tree's ~114-127 ms first spend step, the 4 engine PSOs at game minute 4, the Deck pass (no Deck).

### Next step
On the designer's word: build the stall watchdog, or run `py tools/packaged_soak.py --speed 5` with the PC idle.
Done is `check_all` exit 0.

## 2026-11-24 (143) - The soak's failed run explained: its tree froze at the Unicorn in every soak; fixed, it wins at 89-96 game minutes

**Where the project is.** (141), (142) and this entry are uncommitted (`847a05f` is the last push). Nothing was packaged:
the packaged soak still carries the old driver until the next package (the designer's word); the runs below are the editor
build with `-game` (`UnrealEditor.exe Pawpocalypse.uproject /Game/Pawpocalypse/Maps/L_Realm -game -NoSave ... -ExecCmds=
"r.VSync 0,t.MaxFPS 0,paw.Soak 240 20"`, the soak's report in the log's PAW_SOAK lines, copied to `build/soak/editor_*.txt`).

**What happened.** The designer: "Look into soak" (the 20x run of (142) that did not win).
- **What the failed run showed**: from tier 8 on its field sat at the 320-enemy cap for 30-40 game minutes at a time with
  almost no Catnip coming in; boss fights took 25 and 36 game minutes (the passing run: 9 and 4); the final boss, up at
  198.2, was never beaten. The soak sits its cat on the boss, and the paw strikes the boss only when no enemy is in its reach
  (`PcField.cpp` `StrikeOnce`, by design: "the boss is the thing that does not wander off"), so a cat too weak to clear the
  crowd hit the boss with its aura alone. Both 20x runs, the 5x run and today's ended with ~20 million Stardust unspent
  after only 17-23 tree levels in 12 visits to the tree (yesterday's trial versions: 44-87).
- **The soak report now says why** (`Dev/PawSoak.cpp`): each game minute the kills, the wave, the live enemies' mean HP in
  paw hits, the aura's DPS, the boss's HP left; the build (tree, upgrades, cards, job, paw / aura / kitten output) at every
  Reawaken, every boss and the end; a boss gone with the HP it had left; the Stardust unspent at the end.
- **The cause: the driver froze the tree at the Unicorn.** Buying Unlock Unicorn (2,500 Stardust, around the 6th of 12
  Reawakens) plays the Unicorn ceremony over the tree; `PressNode` refuses while it plays. The driver read that refusal as
  "nothing left to buy" and pressed Close twice. The ceremony steps in its `NativeTick`, which runs only while the tree is
  shown, so it never ended; the tree screen is pooled by the Menu stack, so every later spend step opened with the ceremony
  still "playing", the first press was refused again and the driver left again: **no tree purchase for the rest of the
  game** (instrumented run with the old driver: the same 15 nodes from Reawaken 6 to the end, 34,300 then 145,000 Stardust
  unbought; it still won, at 191.5). Wins and the loss were that crippled cat's luck with cards.
- **A player cannot reach it**: the ceremony is the tree's last child, Visible (it takes every click, the Close button
  included) and the tree's `NativeOnPreviewKeyDown` hands every key to it. Only the dev drivers call `PressClose()`
  directly (the soak; `paw.Leave` in `PawShot.cpp`, whose Unicorn scene waits 4.6 s, past the 3.75 s ceremony).
- **Fixed, both sides.** The soak waits while `IsCeremonyPlaying()`, leaves only when `JumpToAffordable` finds nothing, and
  FAILs if the tree refuses a purchase it offered (a game fault, reported with the node). The game: `PressClose()` refuses
  while the ceremony plays (the rule the keys already had), so no code path can wedge it; `Pawpocalypse.UI.Unicorn.Plays`
  asserts Close leaves the tree up mid-fiesta; sabotage `unicorn_tree_closes_under_it` CAUGHT. `Pawpocalypse.UI.Playthrough`
  did the same thing as the soak (it buys every node, the Unicorn included, then presses Close twice): the first `check_all`
  after the guard FAILED it (the tree stayed up); it now steps the ceremony to its end before leaving and asserts it ended.
- **With the tree working** (editor build, 20x, `-NoSave`): **won at 89.2 game minutes** (6.0 real; 12 Reawakens, 271 tree
  levels, 7 jobs, every boss beaten in about a game minute) and **96.1** (11 Reawakens, 247 levels, 7 jobs); 0 Error / 0
  Warning lines, 0 refusals, never held. Before: 169.7 (5x), 176.3, 191.5 and one loss. Worst frame still the tree's first
  spend step (125-127 ms in the editor build).
- **For the designer (not a balance claim)**: the soak's player now finishes in 89-96 game minutes; pawsim's v4 player
  takes 161. The soak buys every tree node it can at once, Reawakens when suggested and fights every boss on the spot; why
  the two differ is not measured. The 5x pace run with the fixed driver is not run (`docs/09` §5: "trust 5x for pace"; the
  soak paces its presses in real seconds, 10 game seconds between Upgrades passes at 20x).

**Proof.** `check_all` (everyday) exit 0, all 37 gates PASS (`build/check_all_20261124_143.log`; the run before it FAILED
`L3 Pawpocalypse.UI` on the Playthrough test, above). `run_automation Pawpocalypse.UI.Unicorn` PASS (2),
`Pawpocalypse.UI.Playthrough` PASS;
`sabotage_realm --only unicorn_tree_closes_under_it` CAUGHT, the clean build passes; safe_mutate: no mutation left. Soak
reports: `build/soak/editor_*_a.txt` (old driver, instrumented), `_b`, `_c` (fixed).

### Open (the designer's)
- **Package** (confirms the PSO fix of (142) and puts the fixed soak driver in the packaged soak), then `packaged_soak.py`
  at 5x (the pace) and 20x.
- The pace gap: the soak at 89-96 game minutes against pawsim's 161 (a design question; measure the 5x run first).
- Commit and push (141), (142), (143).
- Still from (142): the tree's ~112-127 ms first open, the 4 engine PSOs at first use, the Deck pass (no Deck).

### Next step
On the designer's word: `py tools/package.py`, then `py tools/packaged_pso.py` and `py tools/packaged_soak.py --speed 5`.
Done is `check_all` exit 0.

## 2026-11-24 (142) - D23 (global invalidation off); the bench judges the best of three; the PSO check: M_BackdropFill fixed

**Where the project is.** (141) and this entry are uncommitted (`847a05f` is the last push). Nothing was packaged: the PSO
fix is proven by an automation test and three sabotages, and waits for the next package (the designer's word) to be seen in
the packaged validation.

**What happened.**
- **D23**: Slate global invalidation stays off (the designer, after the re-measure in (141): "I feel like our game is cheap
  enough. Keep it off for now."). Doc 01.
- **The bench's game thread, made a stable gate** (the designer: "Do the test fix"). The planned median of three FAILED in
  practice: three runs 3.66, 6.16, 5.47 ms. Measured why: every part of the game thread slower together in the slow runs
  (the field's drawing 0.43 -> 0.94 ms with the same field), the CPU's effective clock nearly the same (121-132 % of base,
  typeperf, `build/bench_clock`), a 60 fps cap no steadier (p99 2.97-4.32); a Ryzen 5 7600 (no efficiency cores), nothing
  else running. Across 12 uncapped runs of one package p99 2.77-6.75, 5 over 4 ms: a median of three fails ~38 % of the
  time at that rate. **Now**: up to three runs, the game thread on the BEST (interference only slows a run; ~7 % at that
  rate, and one run when the first is clean), every other check in every run (`tools/packaged_bench.py`, `--self-test`
  12 cases + the game's reason wording; 6 planted faults CAUGHT; everyday gate `bench verdict self-test`). Run on the
  package: PASS on the first run (3.29 ms).
- **The PSO check** (`tools/packaged_pso.py`; KB U/12 §6): a route with `r.PSOPrecache.Validation=2`, the log read for
  `PSO PRECACHING MISS` blocks and the engine's `Encountered a new ... PSO` lines, each placed at start-up / play / exit;
  FAIL on a `Missed` of one of this game's materials. Never `-clearPSODriverCache` (read in the 5.8.3 source: on NVIDIA it
  deletes every file of the driver's shader cache folders, which every game on the PC shares). Self-test 9 checks, 7
  planted faults CAUGHT; everyday gate `pso check self-test`; its bench route is now one of `package.py`'s gates.
  **Run over a whole game (the soak at 20x) on today's package**: ONE of this game's materials missed, **M_BackdropFill**
  (ShadersOnly, FLocalVertexFactory, TranslucencyAfterDOF, "Material not precached yet") at the realm's first frame: the
  backdrop's patches and the effects bands are `UProceduralMeshComponent`s, which in 5.8 name no material for precaching
  (no `CollectPSOPrecacheData`), so its pipeline state was compiled at the first draw. The sprites, floor and the rest:
  precached. Engine global shaders: 7 at start-up, **4 at first use mid-play** (game minute 3.4, and the same 4 at the end
  of every bench run: something the engine draws the first time; under 10 ms with this PC's warm driver cache), noted.
  **Fixed**: `Render/PawFillMesh.h/.cpp`, `UPawFillMeshComponent` names each material with the local vertex factory; the
  renderer makes the backdrop's mesh and the three bands with it and names their material as they are made (the bands
  drew nothing until their first effect). `Pawpocalypse.Realm.View` asserts all four meshes name M_BackdropFill with
  FLocalVertexFactory; sabotages `pso_fill_mesh_names_nothing`, `pso_fill_mesh_sections_only`, `pso_fx_band_named_late`
  CAUGHT, the clean build passes.
- **The soak with the validation on did not win**: the final boss up at game minute 198.2 and still up at 240 (the passing
  20x run: up at 154.4, beaten at 176.3); 18 tree nodes bought against 23, 5 Baits against 6; 0 errors, 0 warnings, memory
  flat. The validation touches only rendering, so it reads as the soak player's luck, not the change: but one failed
  whole-game run in three needs a look (the bot's boss fight, or a late-game balance edge). Not investigated.
- **The worst frame of the soak, 112 ms, is the skill tree's first spend step** (game minute 8.0; also ~111 ms in (141)'s
  20x run): no PSO compiled near it, so the tree screen built the first time. Noted, not investigated.

**Proof.** `check_all` (everyday) exit 0, all 37 gates PASS (`build/check_all_20261124_142.log`). `packaged_bench.py --self-test`, `packaged_pso.py --self-test`,
`package.py --self-test` PASS; their planted faults CAUGHT (scratch scripts, through safe_mutate; no mutation left).
`run_automation Pawpocalypse.Realm.View` PASS; `sabotage_realm --only` the three PSO sabotages: all CAUGHT, clean PASS.

### Open (the designer's)
- **Package** to confirm the PSO fix (package.py's gates now include the PSO bench route; then the whole-game route by hand).
- Commit and push (141), (142).
- The 20x soak that did not win (investigate the final boss stretch); the tree's 112 ms first open; the 4 engine PSOs at
  first use (their cost on a cold driver cache is unmeasured: the clean-machine check of doc 10 §7).
- No Deck (the designer has none): the Deck-class pass stays open.

### Next step
On the designer's word: `py tools/package.py`, then `py tools/packaged_pso.py` (whole game): expect no `Missed` of this game's
materials. Done is `check_all` exit 0.

## 2026-11-24 (141) - Phase 8's exit tests: the whole-game soak PASSES in the package (5x, 20x); a one-number patch is 1.2 MiB; the package trimmed by 46.7 MB

**Where the project is.** (137)-(140) committed and pushed at the designer's word (`847a05f`, "Commit and push. Do the next
steps"). This entry's work is uncommitted.

**What happened.**
- **The soak, built and run.** `paw.Soak [game-minutes] [speed]` (`Source/Pawpocalypse/Dev/PawSoak.cpp`, dev builds) plays a
  whole game through the UI's own presses (doc 09 §5 "as built"); `tools/packaged_soak.py` runs it in the packaged
  Development build (`-NoSave`). **PASS at 5x** (won at game minute 169.7, 34 real minutes) **and 20x** (176.3, 11.4 real):
  12 Reawakens, 5 bosses, **0 Error and 0 Warning lines**, never held by an unknown screen, memory flat (1,002 -> 1,015 MB
  peak at 5x), the UObject count flat; frame p99 17.7 ms at 5x (60 fps cap), 18.4 ms at 20x (uncapped). pawsim's v4
  player takes 161 minutes. Reports: `build/soak/*.txt`.
  - Three faults of the driver found by running it (KB U/18 §6): it Reawakened whenever the button ALLOWED it (after the
    first Reawaken from 1 Stardust: an early cash-out, the player's choice), 14 times in an hour: **seen by the designer**
    ("Your test is reawakening at 1 startdust as soon and reawaken is a avaliable is that by design?"); now only when the
    game SUGGESTS it (`Pc::IsReawakenSuggested`), each payout logged. Its cat hunted the nearest regular enemy, so every
    boss fight was lost to the clock; now the boss while one is up. Switching tabs to press another page's button left
    the Upgrades rows stale; now the dock stays on Upgrades and the soak asks the core rules the pages use
    (`Pc::CanReawaken`, `Pc::BaitButton`, `Pc::RealmRow`). Also: the locked look is not `disabled`; 20x needs the frame
    uncapped (8 catch-up steps of 1/60 s a frame: 8x at 60 fps); Git Bash rewrote `/Game/...` (launch from PowerShell).
  - One **0.5 s stall** in the 5x run at game minute 144, with an audio buffer underrun logged at the same moment (the
    whole process stood still); not in the 20x run on the same build; no game work logged around it. To watch.
- **The archive trimmed** (`package.py` step 5): the D3D12 / Vulkan debug layers and UAT's manifests removed after staging;
  Development 439.1 -> 392.5 MB, Shipping 262.5 -> 257.0 MB; the audit now FAILS if one is left; the trimmed build passes
  `packaged_languages` and `packaged_bench`. Self-test: a planted debug layer CAUGHT; the trim on a scratch archive.
- **Patch size measured** (`tools/steam_delta.py`, 1 MiB chunks at fixed offsets as SteamPipe; self-test, everyday gate):
  one balance number changed (Attack Speed's base cost 9 -> 10, through safe_mutate, reimported, packaged; the CSV and
  `DT_UpgradeData` restored, parity checked) = **1.20 MiB**: one chunk of the 77 MiB `.ucas` and the `.utoc`; every other
  file byte-identical between the two runs. Default settings kept (doc 10 §3).
- **The bench's one 73-90 ms frame is its own screenshot**: it now reports its worst frame's time and its distance from the
  screenshot request (1 frame after, each run).
- **The game thread's p99 straddles its 4 ms budget on this PC**: 3.48, 3.69, 4.25, 5.16 ms on the same package today
  (2.6-4.3 yesterday): the bench flips between PASS and FAIL on it. Not attributed to a change.
- KB: U/18 §6 (the soak's lessons), U/19 §3 (the patch measured; strip the debug layers), the symptom index.

**Proof.** `check_all` (everyday) exit 0, all 35 gates PASS (`build/check_all_20261124_141.log`). Packaged soaks PASS (5x, 20x).
Development packaged clean afterwards: audit PASS, `packaged_languages` PASS (23), `packaged_bench` 46.7 draws, game thread p99
5.16 (FAIL), then three re-runs 4.25 (FAIL), 3.48, 3.69 (PASS). Shipping: audit PASS, 257.0 MB. `package.py --self-test` and
`steam_delta.py --self-test` PASS.

### Open (the designer's)
- **The game thread's budget**: 4 ms p99 on this PC is inside the run-to-run noise. Choices: judge the median of three runs;
  cap the bench at a player's frame; or cut the game thread (Slate global invalidation measured 2026-11-10: p99 4.39 ->
  3.34 ms; it needs every animated widget checked, a day's work).
  - **Re-measured 2026-11-24** (the designer: "Remeasure. I feel like maybe we shouldn't do that."): this package, 6 runs
    each, off/on interleaved, no other engine running, the cvar's state read back from each log (`build/bench_gi/summary.md`):
    game thread mean 2.06 -> 1.48 ms (median), **p99 3.58 -> 2.48** (off 3.03-6.75, 2 of 6 FAIL; on 2.36-2.77, 6 of 6
    PASS); draws 46.8 and GPU 0.38 ms both ways. Not switched on: the designer's call. Widgets that paint from their own
    tick and never invalidate (would freeze with it on): the pounce clock, the rainbow bar and frame, the tree's sky, the
    garden, the Reawaken ceremony; also painting their own: the button, the settings dialog, the tree (it invalidates).
  - **Decided 2026-11-24 (designer): keep it off** ("I feel like our game is cheap enough. Keep it off for now."; doc 01
    D23). Still open: the bench's game-thread p99 flips PASS/FAIL on this PC (judge the median of three runs, or a
    budget with room).
- Commit and push (141).
- The Deck-class pass (hardware); the PSO check; the 0.5 s stall if it comes back.

### Next step
The PSO check (first-use shader compiles in a fresh package: the soak's log and `r.PSOPrecache` stats), then the game-thread
question above. Done is `check_all` exit 0.

## 2026-11-23 (140) - D22: this is the game (Godot a first try; no gate reads it); Q1/Q4/Q7/Q9 decided; the background frame cap (Q10); the chrome plates in the UI atlas: 46.8 draw calls, the bench PASSES (Q11)

**Where the project is.** On (139) (uncommitted, as are (137)-(140); `a0161f1` is the last push). The designer answered the
open questions: "Q1: pc. Q4: the latest unreal build we have at the time Q7: yes Q9: keep for now Q10: try to fix Q11: if cant
fix increase limit. Forget about godot. Treat this as a new game. Godot is just a first try. This is the real version."

**What happened.**
- **D22 (doc 01 §2): this is the game.** CLAUDE.md's opening, doc 00, doc 11 (phase 9 is the designer's release sign-off),
  the memory: Godot is history, never a reference. Every gate that read the Godot repo is retired: the L0 `data-export` rule
  and the Godot generators' checks (`gen_pc_enums`, `gen_pc_data`, `gen_tree_tables`: PcEnums.h, PcDataRows.h,
  PawTreeTables.h, PcTreeWording.h and ST_Tree.csv are now edited directly), `audio export reproduces`, `Godot fixes
  current`, `Godot repo untouched`, and in `-Full` `Godot fixtures reproduce` and `pawsim parity sweep`. `pawsim` reads
  `Data/*.csv` by default. The font check reads the game's own tables, translations and language table (`PcLocales.cpp`),
  not Godot's .po and locales.gd. `port_po.py` writes the translation files with no Godot file (`--godot`: the old
  carry-over), so `loc_status --apply` works without the Godot project; the gate is `translation files in form`. The
  Godot-only tools stay in `tools/`, marked history in CLAUDE.md. The recorded fixtures (`*_godot.tsv`) are this game's
  expected behaviour now: a rule change edits code, test and fixture rows directly.
- **Q1 PC, Q4 Next Fest ships the latest Unreal build, Q7 the music licence kept, Q9 FXAA kept** (doc 01 §3).
- **Q10, the background frame cap, built.** `Pc::BackgroundFrameCap` (30, or the player's lower cap; PawCore test);
  `UPawSettingsSubsystem` on `OnApplicationActivationStateChanged` (and at start when the window is not active): remembers
  `t.MaxFPS`, sets the cap at the priority the cvar holds (a `-ExecCmds` value has console priority), gives it back with
  focus; `paw.BackgroundFps` (30; 0 = off). `Pawpocalypse.UI.Settings.BackgroundCap`. `paw.Bench` turns it off (a bench
  window that opened behind the terminal would measure 30 fps) or, with `background`, measures it: **33.5 ms frames in the
  package** behind another window (`t.MaxFPS=30`), the field full, the game thread's work per frame unchanged. No setting in
  the menu (it would need new words, drafted first).
- **Q11: fixed, the limit kept.** The 12 chrome plates joined the UI atlas (icons, glyphs and plates: 273 pictures, one 2048
  page; a skyline packer, as shelves needed 2 pages). `PawUi::Chrome` and `PawUi::AtlasBrush` draw cells; the saved plates of
  the pause menu, settings dialog, skill tree, job picker and victory are swapped too (`Pawpocalypse.UI.Widgets.
  IconsFromAtlas` found them: it now walks borders and Common UI buttons' built styles); `UPawButton::
  NativeOnCurrentTextStyleChanged` swaps the button state plates Common UI builds. **The frame: 46.8 draw calls (83.6 this
  morning), `packaged_bench` PASS twice**; the UI 37.8, the scene 9. Cells are used only in the game and in the tests that
  ask (`PawUi::SetAtlasForTest`): the editor and the baker draw the textures, so a transient cell is never saved into an
  asset (found while doing this: the icon cells of (139) could have been, by a bake or a designer's save).
- The frame now (packaged Development, 1280x800, field full): draws 46.8; frame 1.9-2.2 ms mean; render thread 1.05 ms;
  GPU 0.38 ms; the game thread p99 3.9-4.0 ms in these runs (PASS; it swings run to run on this PC).
- KB: U/09 §3 (plates, Common UI buttons' styles, cells kept out of the editor, skyline packing), U/15 §6 (the final count;
  the background cap measured, the cvar's set priority), 2D/01 (the measured cap), the symptom index.

**Proof.** `check_all` (everyday) exit 0, all 34 gates PASS, none reading the Godot repo (`build/check_all_20261123_d22_140.log`; again after the exe guard fix: `..._140b.log`, 34 PASS).
L0 self-test: every plant CAUGHT (the generated plants now PawDataRows.h, PawTokens.h, PawFontSpec.h, PawFxLook.h, ST_Data.csv).
Font coverage and its self-test PASS on the game's own files; `loc_status --self-test` PASS without Godot; `port_po --check` OK.
Sabotages CAUGHT: `background_cap_ignores_players_cap` (PawCore), `background_cap_never`, `background_cap_kept_in_front`,
`atlas_button_plates_own_texture`, `atlas_chrome_own_texture` (realm). Packaged: `packaged_bench` PASS (46.8; `build/drawcalls/plates_1`,
`plates_2`), the `background` run 33.5 ms frames, `packaged_languages` PASS (23). Shipping: packaged, audit PASS (262.5 MB, exe 119.6). Its first try FAILed package.py's own exe guard: `check_all`'s Shipping build gate had built the same exe 16 s before the run, UAT found it up to date and archived it, older than the run though current; the guard now accepts the project's own up-to-date binary (same size and time, no newer source) and its self-test covers five cases (KB U/19 §2).

### Open (the designer's)
- Commit and push: (137)-(140) are uncommitted.
- A menu switch for the background cap (new words: drafted first) if wanted; "mute when unfocused" (the KB's other idle-game rule).
- Phase 8's rest: the size work (46.7 MB of debug layers staged), the packaged soak, the Deck pass, patch size, the PSO check,
  the 73-90 ms hitch once per bench run, video memory (~+21 MB; the per-icon textures could stop loading).
- The restyle's leftovers (S6 icons and card art, props, the title font, E3's rig) and the drafts waiting (Missing Art, the
  text draft of 2026-11-18); the store page's Dutch and Danish (not yet).

### Next step
Phase 8: the size work and the packaged soak (`package.py`, then a whole game at 5x), then the Deck pass when hardware is at
hand. Done is `check_all` exit 0.

## 2026-11-23 (139) - The first packages (Development, Shipping); draw calls 83.6 -> 50.6 (renderer switches, one field atlas page, the UI icon atlas)

**Where the project is.** On (138) (uncommitted, as is this entry; `a0161f1` is the last push). The designer: "Package. Also look
into lowering our draw calls. What is the latest ms numbers? Also for characters etc are you using one master material with
instances for each enemy? Or is everything it's own material? If so please create a master with instances".

**What happened.**
- **Packaged, both configurations** (`tools/package.py`, its first runs). Development: 439 MB (exe 255.0, paks 121.0), audit
  PASS, `packaged_languages` PASS (23 languages), `packaged_bench` FAIL on draw calls only. Shipping: 262.4 MB (exe 119.6), audit
  PASS. Both packaged again at the end with everything below (`build/package`, `build/package_shipping`).
- **The materials question: already one master per kind, with instances.** No creature has a material of its own: every
  enemy, kitten, the cat, the bosses, hats and props are cells of one sprite atlas, drawn by instanced quads with `M_Sprite`
  (masked) / `M_SpriteSoft` (translucent) and one dynamic instance per atlas PAGE; which picture and tint is per-instance
  custom data. Nothing was made per enemy (it would have cost draw calls, not saved them).
- **Where the draw calls went** (ProfileGPU mid-bench, `Slate.ShowBatching`, the scene off; `build/drawcalls/`): 83.6 a frame
  = the scene 23 (the sprites only 8: a depth prepass forced by DBuffer 4, occlusion queries 8, post 3) + the UI 60.5 (every
  icon its own texture: one Slate batch each).
- **Fixed, measured step by step on packages:**
  - `r.DBuffer=False`, `r.AllowOcclusionQueries=False` (83.6 -> ~72), then `r.VirtualTextures=False` (the prepass was then
    "Forced by VirtualTexture") and `r.EarlyZPass=0` (a prepass for the floor alone): the scene 23 -> 9 (base 2, translucency
    4, post 3). `Pawpocalypse.Realm.Look.Atlas` asserts all four. `r.SeparateTranslucency 0` would save 1 more but blends
    translucency straight into scene colour (exact-colour risk): not done.
  - The field atlas on ONE 4096 page (`tools/pack_atlas.py` PAGE; 3 pages of 2048 before, each a draw per kind): base pass 4
    -> 2. ~+5 MB video memory. `import_art.py` now removes a page texture the atlas no longer has before the editor starts
    (a headless delete leaves the file). The Look test asserts one page.
  - **The UI atlas**: `tools/pack_ui_atlas.py` packs the 196 icons of 128 px and the 65 input glyphs onto one 2048 page
    (`T_Atlas_UI_0`); `import_art.py --run icons` imports it and writes every cell into `DA_Icons` (`AtlasPages`,
    `AtlasNames`, `AtlasCellPage`, `AtlasUV`). `UPawUiCell` (a UObject on `ISlateTextureAtlasInterface`) is a region of
    the page; `PawUi::IconBrush` and the prompts draw cells; `PawUi::UseAtlasIcons` swaps the brushes the Widget
    Blueprints saved (Button, CardFace, HudScreen, MainMenu, ReawakenCeremony, Toast). The UI 60.5 -> 41.5. ~+16 MB video
    memory (the per-icon textures stay referenced by `DA_Icons`: a later saving).
  - New tests: `Pawpocalypse.UI.IconAtlas` (every 128 px icon and glyph in the atlas, the page's texels there byte for byte
    its own texture's, the brush the cell, larger pictures their own textures, every icon on the HUD from the page) and
    `Pawpocalypse.UI.Widgets.IconsFromAtlas` (every baked class, as its asset builds it: no image draws an icon texture the
    atlas has). Everyday gate `UI atlas reproduces` (`pack_ui_atlas.py --check`).
- **The frame now** (packaged Development, 1280x800, the field full; before -> after): draw calls 83.6 -> 50.5-50.7;
  render thread 1.13 -> ~1.0 ms mean; GPU 0.42 -> 0.38 ms; the frame 1.4-2.8 ms mean, p99 2.4-4.3 (the game thread swings
  1.4 to 2.8 ms between identical runs on this PC: noise, as before). The bench still FAILs: 50.6 >= 50 (and one run's game
  thread p99 4.3 >= 4.0, another's 2.6).
- Found on the way: naming the new test `Pawpocalypse.UI.Icons.Atlas` HID the old `Pawpocalypse.UI.Icons` from its filter
  (it ran one test, the new one); renamed `Pawpocalypse.UI.IconAtlas`, both run (KB U/18 §2).
- KB: U/15 §6 (ProfileGPU per pass; a flat 2D scene's overhead and the four switches; the UI atlas's effect), U/09 §3 (UI
  icons from one atlas texture), 2D/02 §4.4 (one page over several), U/18 §2 (a child test name), U/24 rows, the symptom index.

**Proof.** `check_all` (everyday) exit 0, all 37 gates PASS, `UI atlas reproduces` among them (`build/check_all_20261123_drawcalls_139b.log`; the first run FAILed one gate, `sabotage targets current`: `widgets_blueprint_never_wired` targeted the title's wiring block this change edited; retargeted, seen CAUGHT again). Sabotages: `atlas_icon_brush_own_texture`, `atlas_cell_wrong_region`, `atlas_hud_saved_icons_kept`,
`atlas_prompt_glyph_own_texture` CAUGHT (the clean build passing); the texel check FAILed on one icon's cell planted a pixel
right (`ui.csv`, safe_mutate, imported, then the clean csv imported again); `UI atlas reproduces` FAILed on one changed byte.
The draw-call numbers: `build/drawcalls/<set>/<run>/` (logs with the ProfileGPU tables, bench reports, ShowBatching shots).

### Open (the designer's)
- **The chrome plates into the UI atlas** (the row plates, price chips, tab plates: one texture per plate style; some live in
  the Common UI button styles' brushes): the frame under 50. Or Q11's other way, the field gated alone (it is 9).
- Video memory: ~+21 MB in all (the UI page 16, the field page ~5). The per-icon textures could stop loading (soft references
  in `DA_Icons`) to take back most of the 16.
- Commit and push: (137), (138), (139) are uncommitted.
- The store page's Dutch and Danish (not yet); from (135), toasts and the Unicorn ceremony's painted words in `Language.Switch`.

### Next step
The chrome plates into the UI atlas (`pack_ui_atlas.py` with the chrome PNGs, Box brushes on cells, the button styles' brushes
swapped where the live style is built), then package and bench; done is the bench under 50 and `check_all` exit 0.

## 2026-11-23 (138) - D20: no "(draft)" in the language lists; D21: the version is 0.MINOR.PATCH, from 0.1.0

**Where the project is.** On (137) (uncommitted, as is this entry; `a0161f1` is the last push). The designer, answering
(137)'s open questions: "Use minor type version numbers. drop the draft. Whats next?" Nothing was packaged (not asked).

**What happened.**
- **D20 (doc 01 §2): no "(draft)" beside a language.** `Pc::LocalePickerLabel(Code)` is the language's name in itself and
  nothing else (`Pc::LocaleReviewed` and the marker argument gone); `PawText::LanguageLabel` with it. The line
  `SETTINGS_language_draft` left `ST_UI.csv`, and `port_po.py` dropped it from all 22 translations (23 files, one row each,
  nothing else changed; every language still complete, 836 / 836 lines). Godot keeps its rule, so the change is in
  `tools/godot_fixes.py` (D20, on the fixture's scratch copy only) and the `locales` fixture was made again from Godot with
  it: only the 20 label rows changed (`--only locales`, 30 s). The PawCore test proves the fixture carries the change (20
  languages whose Godot "(draft)" exists, labelled without it); Settings > Language and the title's list assert "Deutsch",
  Dutch and Danish their bare names.
- **D21 (doc 01 §2): the version.** `Config/DefaultGame.ini` `ProjectVersion=0.1.0` (0.MINOR.PATCH): the title shows
  "v0.1.0" (it showed the engine's default "v1.0.0.0"). `package.py --bump` raises PATCH for a build that leaves the PC;
  MINOR by hand. `Pawpocalypse.UI.Title.Look` now asserts the line is "v" + the version and the version's form.
- **A tool bug found by the new sabotage:** `sabotage_pawcore.py` read the test exe's output in the ANSI code page; a
  failure that names a language in its own script ("繁體中文": the byte 0x81) failed to decode in the reader thread, so
  `stdout` was `None` and the runner crashed on exactly the run that should say CAUGHT. Now UTF-8 with `errors="replace"`
  (the realm runner and `run_automation.py` already were). Reproduced alone in a scratch script; KB U/17 §3.
- Docs: doc 01 §2 (D20, D21), 00, 08, 10 §2, 11 (phases 7, 8), WORKLIST-LOC (§1, §2b, §2c's record).

**Proof.** `check_all` (everyday) exit 0, all 36 gates PASS (`build/check_all_20261123_d20_138.log`). L1 211 / 211.
Sabotages: `locales_draft_marker_back` (PawCore: "(draft)" put back for every language but English) CAUGHT;
`language_label_is_the_code` (realm: the label is the code) CAUGHT by `Pawpocalypse.UI.Language.Labels`; the version
planted back to 1.0.0.0 (`safe_mutate`, `DefaultGame.ini` restored byte for byte) FAILed `Pawpocalypse.UI.Title.Look`
("the version is 0.MINOR.PATCH"). `port_po --check` OK, `loc_lint` PASS, `loc_status` 22 x 836 / 836.

### Open (the designer's)
- **Package?** `py tools/package.py` (Development: the audit, `packaged_languages.py`, `packaged_bench.py`), then
  `--config Shipping`. The bench FAILs on draw calls (68.7 against 50) until Q11 is decided.
- **Q11** (doc 01 §3): the draw-call budget. Recommendation: the field's own draw calls against 50, the HUD its own line.
- The store page's Dutch and Danish (the designer: not yet).
- From (135): toasts, the Unicorn ceremony's painted words and screens not built yet are not in `Language.Switch`.
- (137) and (138) are uncommitted.

### Next step
On the designer's word: `py tools/package.py`, then `py tools/package.py --config Shipping`; then phase 8's size work,
the packaged soak, the Deck pass. Done is `check_all` exit 0.

## 2026-11-23 (137) - No native reads (decided); the headlines seen; the settings lists' ring fixed; phase 8 started (`package.py`, the plugin trim, Shipping compiles)

**Where the project is.** (135) and (136) committed and pushed at the designer's word (`a0161f1`). The designer, after
(136)'s sheet: "The text is good." Then: "Commit and push. Do whatever you can next. Native speakers will not be reviewing
the text so skip that." This entry's work is uncommitted. Nothing was packaged (not asked).

**What happened.**
- **Decided: no native reads** (WORKLIST-LOC §1, doc 11, 00). The translations as they stand ship. Every language but English
  still says "(draft)" in the language lists (Godot's rule, `Pc::LocaleReviewed`): **the designer's call** whether it stays.
- **The shrunk headlines, for the designer's eye**: `build/loc/headlines/sheet.png` (the Reawaken ceremony, the victory and
  Settings > Controls at 1280x800 and 150 % UI, in en, de, tr, nl, pl, vi, sv; the real game, `godot_tour.py` steps). Every
  headline fits its box (WIEDERERWACHEN, YENİDEN UYANIYOR, OVERWINNING, ZWYCIĘSTWO, CHIẾN THẮNG, HANDKONTROLL).
- **Found and fixed: the settings lists' ring sat off the lists in a windowed game** (the default). `UPawSettingsDialog::
  NativePaint` drew a list's hover / focus ring (UI_FOCUS, Sticker Box's blue) at `GetCachedGeometry()`, which is DESKTOP
  space; paint is window space. Right in fullscreen and in every headless test (window at 0,0); measured off by (+319,
  +117) px in a centred 1280x800 window (over the dialog's edge and off the screen), one row low at 1920x1080. Now
  `GetPaintSpaceGeometry()`: the rings hug the lists (shots in `build/tour/exp/`, before and after). **L0 `ui-rules`**: no
  paint function draws with cached / tick-space geometry; its self-test plants the old line: CAUGHT. (A port bug, not a
  Godot fault.) The stale "Butter" comment on `PawUi::FocusRing` corrected.
- **Phase 8 started: `tools/package.py`** (doc 10 §2; not run: packaging is the designer's word). Refusals (an editor of this
  project, a packaged copy, another UAT), the archive DELETED first, `--bump`, UAT, the exe newer than the run, the `.pdb`
  kept in `build/symbols/`, the audit, README and `Send logs.bat` in the archive, the L5 gates (Development).
  - The audit on the package made by hand (`--audit`, read only): FAIL on two stale `Saved` folders (an `Engine/Saved`
    file from 2026-11-04: **the archive was never cleaned**, its README was still phase 4's "English only, no audio yet");
    WARN 48.8 MB not needed (graphics debug layers; Boost). Self-test: 12 plants each CAUGHT by its own rule (everyday gate
    `package audit self-test`).
  - Its own bugs, found by testing it: the UAT guard matched its own PowerShell query (it would have refused every run;
    now by process name, proved with a stand-in `AutomationTool.exe`); `Send logs.bat`'s bare `tar` was Git's GNU tar
    ("Cannot connect to C:"), now `%SystemRoot%\System32\tar.exe` (tested in a scratch archive, `PAW_LOGS_OUT`).
- **The plugin trim was stale**: an engine update added FabLauncher (editor-only, enabled by default), which pulled USD,
  Boost and its DLLs into the game target. `py tools/trim_game_plugins.py` again (only FabLauncher added): USD and Boost
  left the link and the receipt; the Development exe 295.9 -> 254.9 MB. New `--check` (everyday gate `game plugins
  trimmed`), seen to FAIL on the stale `.uproject` first.
- **Shipping compiles** (first time ever: 178 s cold, exe 119.6 MB). Everyday gate `UBT game build (Shipping)`, seen to fail
  on a planted Shipping-only `#error` (safe_mutate; restored, then a clean build).
- KB: U/19 §2 (a never-cleaned archive, auditing with UnrealPak -List), §3 (USD's Boost via FabLauncher, the trim's
  result), §4 (`tar` in a batch file); U/09 §3 (cached vs paint geometry); two U/24 rows; the symptom index.

**Proof.** `check_all` (everyday) exit 0, all 36 gates PASS, the three new ones among them (`build/check_all_20261123_phase8_137.log`). L0 self-test PASS with the new paint plant CAUGHT. The fix seen in the game: the ring at (108, 221)-(1156, 280) round the Language list in a 1280x800 window at 150 %, where it was at (428, 341) running off the screen; after a Down press, round Resolution at 1920x1080 and at 1280x800. `package.py --self-test` 12 CAUGHT; its audit FAILed the hand-made package on its two stale `Saved` folders. Trim `--check` FAIL before the re-run, PASS after. Shipping: FAIL on the plant, PASS clean.

### Open (the designer's)
- **Package?** `py tools/package.py` (Development: the audit and both L5 gates; the bench still FAILs on draw calls until
  doc 01 Q11's budget is decided), then `--config Shipping`. Nothing was packaged this session.
- **The version.** `ProjectVersion` is unset, so the title shows "v1.0.0.0" (`PawText`, the engine's default). Godot's
  scheme is 0.MINOR.BUILD (0.0.x internal, 0.1.x reserved for the first public playtest; itch is 0.2.5): the Unreal
  build's starting version is the designer's. `package.py --bump` refuses until it is set.
- **"(draft)"** beside every translated language in the lists: keep it (Godot's rule) or drop it now that no native read
  will come.
- The headlines sheet (`build/loc/headlines/sheet.png`) for a look; the store page's Dutch and Danish (the designer: not
  yet); Q11.
- From (135): toasts, the Unicorn ceremony's painted words and screens not built yet are not in `Language.Switch`.

### Next step
On the designer's word: `py tools/package.py`, then `py tools/package.py --config Shipping`; then phase 8's size work (the
debug layers, the archive's size after the trim) and the packaged soak. Done is `check_all` exit 0.

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
