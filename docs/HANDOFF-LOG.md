# Handoff log

Newest first. Every session that changes the state of this project adds an entry: where the project is, what happened, what is open,
and the exact next step. Read `docs/00-START-HERE.md` first, then the newest entry here.

---

## 2026-11-04 (34) - A whole-codebase review; one port bug fixed: the kittens chased the oldest boss add

### Where the project is
- Unchanged from (33) but for the fix below: phase 4 slices 1-2 done, the title screen next. The port is committed and pushed (`10c118e`); the Godot repo is not.

### What happened
1. **A review of the docs and the code** (PawCore whole; the game module's Core, Data, Realm, Render, Save, Dev and the UI's
   logic; tests and tools sampled). Its findings are under Open; only item 2 was acted on (designer: "Yes" to the fix).
2. **Port bug fixed: `PcBossAdd::Serial` was never set** (`PcField.cpp` TickBossAdds), so every add had serial 0. The kittens
   pick the add nearest the cat and find it again by serial (`HuntKittenAdd`): with every serial 0 they chased `Adds[0]`, the
   OLDEST add, and the "already intercepted this step" guard never fired, so the pool chewed through several adds in one step.
   Godot passes the add node itself (`tier_controller.gd` 1955, `kitten.gd` 385). Now `A.Serial = F.AddSpawns`. The only test
   used one add; new test "Boss: with two adds out, the kittens chase the one nearest the cat, and once it falls the rest
   wait". Sabotages `adds_share_one_serial`, `kittens_chase_the_next_add`: CAUGHT (with `kittens_ignore_adds`), clean build passes.
3. **The engine gates never run on a stale DLL** (designer: "Yes, do those"). `tools/run_automation.py` builds the editor target
   first and refuses to test when the build fails (`--no-build` for a caller that just built: `sabotage_realm.py`, `check_all`).
   `check_all`'s everyday set gained the `UBT editor build` gate BEFORE the engine gates (moved out of `-Full`); a failed build
   FAILS every engine gate unrun. **A SKIP is no longer a pass**: exit 2 ("NOT PROVEN"). CLAUDE.md and doc 09 §7 say so.
   Each path seen: a fake `UnrealEditor-*` process (a renamed ping.exe in the scratchpad, stopped after) -> 6 SKIP, exit 2; a
   new file with `#error` -> `run_automation` exit 1 "nothing was tested", `check_all` UBT FAIL + 5 engine gates FAIL unrun,
   exit 1 (file and its intermediates deleted); clean -> every gate PASS, exit 0; `sabotage_realm.py --only
   realm_no_step_cap` (now `--no-build`) CAUGHT, clean build passes.
4. **Stale text fixed:** `PcCardDraw.h` RerollHand and doc 02 §7 (F11); `PawRealmSubsystem.h` ChooseCard and doc 03 §2 twice
   (F14); `PcCheats.h` CheatUnlockAllStages (F18); `PcJson.h` (the shortest exact text, not 17 digits); `PawRealmView` Burst
   (a total cap); `check_all.ps1` header (~5.5 h); doc 02 §3: kittens never scale with evolution (the code never has; Godot's
   GDD line 95 still says they do: for the designer).

### Verified
- Everyday `check_all`: every gate PASS. Then UBT (the PawCore DLL newer than the edit) and `Pawpocalypse.Realm` again on the
  rebuilt binaries: 9/9 PASS. (The everyday set's L3 had run on the previous DLL: see the gate item under Open.) L1: 161 cases.

### Open (the review's findings, none acted on)
- **For the designer (Godot first; each a new finding for doc 01 §5 and the Godot findings file):**
  - `deepest_biome_index` is not saved: after a restart (Continue starts in biome 1) every `cap_per_floor` falls back to the
    biome-1 caps until the player walks back down (against game_state.gd 2120's own comment), and every unlocked biome reads
    "new" (`PcHud` ZonePlate / IsRealmNew).
  - `pending_second_helping` is not saved: quitting before biome 4 loses Second Helping's deferred half of a Reawaken.
  - Also reset by a restart: Body Count (`kills_this_cycle`), Reputation (`waves_here`), Levy's count, Infestation.
  - A restart between Reawaken and the job pick: no spend step (the tree is read-only outside it), no job asked this run,
    Territory's start skipped.
  - The shop button disables at `catnip < cost` (hud.gd 1673, `PcHud` ShopRow) while the purchase forgives `currency_epsilon`.
  - Godot's GDD (`docs/pawpocalypse-gdd.md` line 95) says kittens scale with evolution; the code never has (pawsim's
    kitten_dps_each says why). The port's doc 02 is fixed; the GDD is the designer's.
- **Robustness:** `ShortAmount(NaN)` reads `Suffixes[-1]`, `FixedLikeGodot` / `OneDecimalLikeGodot` `atoi` past the string on
  NaN / inf (`PcFormat.cpp`), and a NaN would round-trip through the save; `PcJson` reads `1e400` as 0 (from_chars leaves the
  value on out_of_range) and its duplicate-key check is O(n^2); `ApplySave` does not clamp levels (a negative branch level locks
  the node); `BossRealms.back()` unguarded in `MarkBossDefeated` / `RecordBossDefeat`; a validator failure in Shipping leaves
  no realm at all (doc 03 §6 says it only logs).
- **Before the title screen (slice 3):** `NewGame()` changes the save digest, so the end-of-frame save writes the EMPTY game into
  the ACTIVE slot unless `SaveGame(newSlot)` runs the same frame (`paw.Cheat.Reset` does this to the real slot today);
  `LoadGame` does not restart the realm (ApplySave clears `CurrentBiomeId`): any Load must `StartRealm`; `paw.Cheat.Reawaken`
  leaves the realm biome-less (no ceremony, spend step or travel).
- **Gates (the build order and SKIP are done, item 3):** both sabotage scripts edit real sources (safe only while the tree is
  committed); no sanitizer build of PawCore; the colour lint skips `Realm/`; Dev/Spikes still compiles into Shipping.
- Still open from (33): commit the Godot repo (the designer's call); the F19 hint's translations; F6; the phase 3 packaged-build gate.

### Next step
*"Act on the rest of HANDOFF-LOG (34)'s findings: the non-finite guards in PcFormat / PcJson; write up the save findings for the
designer (Godot first). Then phase 4 slice 3, the title screen, minding the NewGame / LoadGame save traps listed there."*

---

## 2026-11-03 (33) - The nightly -Full after the findings pass: 20 of 21 green, two stale PawCore sabotages retargeted

### Where the project is
- Unchanged from (32): phase 4 slices 1-2 done, every open finding fixed (Godot first). Nothing committed in either repo.

### What happened
1. Ran `tools\check_all.ps1 -Full` (about 5.5 h; summary in `build/check_all/_full_1103.txt`). 20 of 21 gates PASS, including
   L3 Pawpocalypse.Save, Godot fixtures reproduce, L1 long, pawsim parity sweep and Realm sabotage (every realm sabotage caught).
2. **PawCore sabotage FAILED, both from harness drift, not from the code** (441 caught, 2 not):
   - `kitten_recount_every_step` filtered on "Kittens: a card that costs*", a test the F14 pass renamed, so the filter matched no test
     (MISSED, and the clean-build check failed too). Retargeted to "Kittens*": CAUGHT, clean build passes.
   - `ach_waiting_is_earned` still wrote the one-argument `EarnedAchievementCount(P)` that F9 replaced with `(D, P)` (NOBUILD).
     Fixed the mutant: CAUGHT, clean build passes.
   Only `tools/sabotage_pawcore.py` changed; the full PawCore sabotage gate was not rerun (the two fixed entries were, with `--only`).
3. Docs brought up to date (designer: "make sure all docs are up to date"):
   - the nightly's duration, ~5.5 h (CLAUDE.md, doc 00, and doc 09 §7 with the measured gate times);
   - the test and sabotage counts: L1 160 cases / 4.72 M assertions, long 31 / 5.5 M, PawCore 443 sabotages, realm 236;
   - 39 achievements (docs 01, 06);
   - F23's second job is now saved (docs 08, 11);
   - the victory screen now wears the game theme (doc 00);
   - a `\b` (backspace) that had eaten a backslash in doc 09's save row.
   Appendices A, B and C were regenerated to scratch and are identical to the ones in docs/.

### Open
- Commit both repos (designer); a translator's check of the F19 hint; F6 (whether the silent lock is wanted); the phase 3
  packaged-build gate.

### Next step
*"Phase 4 slice 3: the title screen on L_Title (main_menu.gd: Continue / New Game / Load over five slot cards with summaries and
delete-with-confirm; New Game in an empty slot = NewGame + SaveGame(slot); Report a bug, Discord, Settings, Quit saving only with
progress); then the pause menu and the settings popup."*

---

## 2026-11-03 (32) - Q3 no; F24 the run's cards saved; every open finding fixed (Godot first, the port followed)

### Where the project is
- **Designer decisions 2026-11-03**: Q3 "No itch saves for unreal" (no importer); F24 "the runs cards and xp should be saved";
  then, asked which fixes: **"All open findings"** (each finding's suggested fix, Godot first).
- Phase 4: slices 1-2 done (31); slice 3 (the title screen) not started this session: the findings took it.
- **Nothing committed** in either repo. Godot repo, uncommitted for the designer: `scripts/autoload/` achievement_manager,
  card_manager, game_state, save_manager; `scripts/data/pawpocalypse_save_data.gd`; `scripts/systems/` hud, job_picker, kitten,
  tier_controller, victory_screen; `data/achievements/` two files DELETED; `locale/` (pot and every .po: merged, LF kept);
  the findings file. Godot's `tools/check_all.sh`: ALL GREEN, 12,949 assertions (6 fewer: the two achievements).

### What happened
1. **F24, the run's cards and XP** (Godot first): PawpocalypseSaveData gains `run_cards_saved` and the run's cards (owned,
   banished, Catches level and XP, rerolls, extra choices, banishes, Fixation hands, the HAND on screen, Blueprint's promise,
   Prototype's mark, cards_taken_this_run): new @exports, no version bump, an older save loads as it always did.
   `CardManager.apply_run_save()` / `reoffer_after_load()` (TierController calls it: a saved hand comes back, the same cards).
   Port: `PcSave` (fields, capture, Godot's restore: unknown ids dropped, the mark only on a card in the hand), JSON string
   arrays; the HUD already opens the pick on any pending hand; `Pawpocalypse.UI.CardPick.SavedHand`. Fixture `save` extended:
   2,100 restored runs, 540 older saves keep memory's, 797 hands re-offered once.
2. **Q3**: no importer; doc 08 §2.1, doc 11.
3. **Every open finding with a fix** (Godot first; then port; doc 01 §5 and the Godot findings file have each row):
   F13 Giblets at `death_position`; F16 the five Overkill splashes from `death_position`; F17 Riddle spent by the NEXT swing;
   F14 a card taken re-counts the kittens (TierController on `card_taken`; port: `ChooseCard` and Auto-Card in the field);
   F18 the unlock-all cheat sets this cycle's passage; F19 the puddle cards open with Rainbow Poop, hint "Buy the Rainbow Poop
   branch in the skill tree." (20 translations built from each language's own strings, 8 hand-fixed: **worth a translator's
   look**); F23 Union Rules' second job saved; F7 Fence's window closes in `begin_biome`; F8 Kitten Speed refused with no
   kitten in `purchase_upgrade`; F9 `earned_count` counts the roster only; F10 one promise per deal; F11 a reroll / banish
   re-marks and re-promises; F2 the two unearnable achievements REMOVED (39 now); **F6: behaviour kept, comment fixed** (the
   comment's silent lock would have taken "No job" away and nothing implemented it: the one finding where the comment's version
   was not taken). Not fixed (no fix to apply): F3 (a design note), F4 / F5 / F12 (known divergences), F15 (cards that may still
   ship); F25 (Godot's .tres precision) is an engine trait.
4. **The port**: data re-exported (39 achievements), DataTables re-imported, ST_Data / appendices A-C regenerated; PcCards,
   PcAchievements (`EarnedAchievementCount(D, P)`), PcProgress, PcCheats, PcCardDraw (`MarkAndPromise`), PcField (the parking-
   spot switches deleted; Riddle; Auto-Card re-count), PcValidate (the known-issues list now empty), the realm's `ChooseCard`;
   every Godot fixture regenerated; tests that pinned Godot's old behaviour flipped to assert the fixes; new tests (Auto-Card
   kitten re-count, Kitten Speed lock). The cards fixture's `step_rerolls` column now switches auto-pick off (it was measuring
   the deal too; exposed when F10 shifted Godot's random sequence; KB G/03 §6). `Data/CodeConstantsMap.csv` rows for the new
   save fields (the data-export gate caught them).
5. Sabotages: PawCore 6 (F24) + 20 (the findings: 9 re-targeted from the old pinned behaviour, 11 new; two first written with
   unused names failed /WX and were rewritten), realm 3 (saved hand, Giblets, ChooseCard re-count): all caught.

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1103d.txt`). PawCore 160 test cases. UI 38, Realm 9,
  Save 8, Data 2. Godot `check_all.sh`: ALL GREEN. `make_godot_fixtures.py` regenerated every fixture.

### Open
- **For the designer**: commit both repos; the F19 hint's translations; F6 (say if the silent lock is wanted after all);
  the packaged-build gate for phase 3.
- Nightly `-Full` not run since (30); it should run before phase 4 continues (the fixtures and many sabotages changed).

### Next step
*"Run the nightly `check_all -Full`. Then phase 4 slice 3: the title screen on L_Title (main_menu.gd: Continue / New Game /
Load over five slot cards with summaries and delete-with-confirm; New Game in an empty slot = NewGame + SaveGame(slot); Report a
bug, Discord, Settings, Quit saving only with progress); then the pause menu and the settings popup."*

---

## 2026-11-03 (31) - F21 and F22 decided and fixed; phase 4 slices 1-2: the save and its container

### Where the project is
- **Designer decisions 2026-11-03**: F21 "yes" (the job picker and the victory screen wear the game theme); F22 "that should not
  happen" (the Realms page must not walk past a boss not beaten this run). Both fixed in Godot first, the port followed.
- **Phase 4 started** (doc 11): slice 1 (the save in PawCore) and slice 2 (its container in the game) done. Next: the title screen.
- **Nothing committed** in either repo. Godot repo, uncommitted for the designer: `scripts/autoload/game_state.gd`
  (`is_passage_open`), `scripts/systems/hud.gd` (F22, and F20 from (23)), `job_picker.gd` + `victory_screen.gd` (F21),
  `locale/messages.pot` (re-extracted: source line references only, LF kept), `card_manager.gd` + `tier_controller.gd` (from
  earlier), and the findings file. Godot's whole `tools/check_all.sh` passes (12,955 assertions) in a sandboxed copy.

### What happened
1. **F22** in Godot: `GameState.is_passage_open(stage)` (the stage AND this cycle's bosses), used by `is_biome_unlocked()` and by
   both branches of `_refresh_travel_panel()`: a row behind an unbeaten boss reads "<biome> — locked" (an existing string) and is
   disabled. Port: `Pc::IsPassageOpen`, `PcHud RealmRow` asks it; `PcHudTests` new case; the playthrough now pins that the page
   does NOT travel there. Sabotages: 2 PawCore + `playthrough_realm_press_unguarded`, caught.
2. **F21** in Godot: the picker's root gets the game theme (its `WindowPanel` applies); the victory screen gets a themed root and
   its column a 640-wide `WindowPanel`. Fixture `victory` updated (the new tree; texture styleboxes printed) and regenerated.
   Port: `PawUi::WindowBrush()` / `WindowPadding()`, both screens on paper with ink text and the theme's buttons; the engine look
   (`SetEngineLook`, `UPawEngineButtonStyle`, the `GODOT_*` tokens and their generator block) removed. 6 sabotages caught.
3. **Slice 1, `PcSave` + `PcJson`** (PawCore): Godot's PawpocalypseSaveData v7 field for field; `CaptureSave` (banks the live
   biome; every known upgrade / branch id, zeros included) and `ApplySave` (Godot's load order, the v6 tree migration, the
   Unicorn split, the reset path for old versions, take_job, the run-job seed); the file: `PAWSAVE <format> <crc32> <len>` +
   JSON body, every double exact. **Fixture `save`** (new): the REAL save_game() / load_game() on 3,000 random states and saves;
   all 1,000 / 3,000 rows equal. Found on the way: **Godot's .tres writer is lossy** (16 digits; float32-exact doubles written with
   float32's digits), so the fixture reads back what a load sees and the save side compares within 1e-7 (F25, KB G/07).
   21 sabotages caught (one equivalent mutant replaced).
4. **Slice 2, the container** (`Source/Pawpocalypse/Save`): `UPawSaveSubsystem` over `IPawSaveStorage` (platform
   `ISaveGameSystem` / folder / memory). ISaveGameSystem has no rename, so the write is: temp, verify, backup only if the current
   slot reads, slot, drop the temp; startup recovery; backup fallback on load; a newer build's slot never read nor overwritten;
   the end-of-frame save from a digest of exactly what Godot's seven save signals report (one write per frame); autosave every
   30 s of unpaused play; focus loss, suspend, quit; boot loads the active slot. Sandbox: `-NoSave`, `-PawSaveDir=`; automation
   and `-unattended` runs get no storage at all; the bench and tour now pass `-NoSave`. `Pawpocalypse.Save.*` 8 tests, gate
   `L3 Pawpocalypse.Save` in `check_all`. 23 realm sabotages caught (two real test gaps closed from the first misses: a good slot
   beside an older backup; the field's own auto-travel input).
5. **The per-slot automations** moved from HUD globals onto `UPawGameSubsystem::Slot()` (with the tutorial's steps); the realm
   feeds them to the field every frame; New Game clears them. **Port bug fixed**: buying Auto-Travel / Auto-Card in the tree did
   not switch the automation on (PcTree reported it, nobody read it); `UI.Tree.Spend` now checks it.
6. **Doc fixes**: doc 08 §1-2 rewritten to what Godot saves (it had promised run state Godot never saved) and how the container
   works; doc 02 §4 (a boss timeout does not send the player back or switch auto-travel off: Godot never did); docs 00, 01 (F21,
   F22 decided; F23-F25 new), 05, 09, 11. KB: U/10 §1 (atomic save over ISaveGameSystem; sandbox by construction), G/07 (.tres
   precision), U/18 §8 (counting CRLF), INDEX symptom rows.

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1103b.txt`), incl. the new `L3 Pawpocalypse.Save`.
- `make_godot_fixtures.py --only victory --only save --check`: byte for byte. Godot `check_all.sh` (sandboxed copy): ALL GREEN.
- Sabotages this session: PawCore 2 (F22) + 21 (save), realm 7 (F21 / F22) + 23 (save, toggles): all caught.

### Open
- **For the designer**: F23 (Union Rules' second job is not saved), F24 (the run's cards and XP are not saved: a restart loses
  them), Q3 (import itch saves?); the Godot changes to commit; the packaged-build gate for phase 3.
- The last slot used is not remembered yet (Godot keeps it in settings.cfg): the active slot starts at 0 until the settings slice.
- Nightly `-Full` not run since the save code.

### Next step
*"Phase 4 slice 3: the title screen on L_Title (docs/05, main_menu.gd): Continue / New Game / Load over five slot cards with
summaries and delete-with-confirm, New Game in an empty slot = NewGame + SaveGame(slot), the garden and Boba, Report a bug,
Discord, Settings, Quit (saves only with progress); the realm opens after it. Then the pause menu and the settings popup."*

---

## 2026-11-03 (30) - The nightly -Full: two sabotage-harness bugs fixed, every gate green

### Where the project is
- Unchanged from (29): phase 3's exit criteria met, the packaged-build gate waits for the designer's ask. No game code changed.
- **Nothing committed** in either repo; the Godot repo was only read (its status before and after: the F20 changes and the
  findings file from (23)/(29)).

### What happened
1. `check_all -Full` (night of 2026-11-02, ~3.7 h, `build/check_all/_full_1102.txt`): 19 gates PASS, 2 FAIL, both harness bugs:
   - **PawCore sabotage: 38 MISSED** (366 caught). Every one had a multi-line target in a CRLF file: `PcField.cpp`, `PcFormat.cpp`
     and `PcHud.cpp` are CRLF, every other source LF (counted with `tr -cd '\r' | wc -c`; Git Bash's `grep -c $'\r$'` first
     reported every file CRLF, wrongly); `sabotage_pawcore.py` checked targets with `read_text()` (CRLF read as LF, so "occurs
     once") but replaced in `read_bytes().decode()` (CRLF kept), so the replace changed nothing, the unchanged code was rebuilt
     and passed. Fixed: `in_file_endings()` puts the target in the file's own endings, the count runs on the same raw text, and
     a mutant whose replace changes nothing is refused as `NOLAND` before any build (negative-tested: it fires). Same fix in
     `sabotage_realm.py` (its sources are LF today, so it was latent there).
   - **Realm sabotage: STALE** `hud_boss_message_dropped`: (27) put the victory call between its two target lines. Re-targeted
     on the same assignment (`BossMessageTier = F.TierIndex;` before `if (E.Flags & Pc::FieldFlag::Victory)`).
2. KB U/18 §8 (count and replace in the same bytes; assert the mutant landed), INDEX row and a symptom-index row.

### Verified
- The 38 rerun with the fix: 38/38 CAUGHT, clean PASS (`build/check_all/_rerun_38_1103.txt`). With the 366 caught in the
  nightly: all 404 PawCore sabotages caught.
- `sabotage_realm.py` in full: 208/208 CAUGHT, clean PASS (`build/check_all/Realm sabotage.log`, 04:48).
- Every other `-Full` gate PASS in the nightly (L1 long suite, Godot fixtures reproduce, mirror 200k, fuzz 30k, pawsim parity
  sweep, UBT, the self-tests, Godot repo untouched).

### Open
- For the designer, unchanged: F21, F22; the packaged-build gate that closes phase 3.
- The whole `check_all -Full` has not run in one pass with the fixed scripts (its two failing gates were rerun alone).

### Next step
*"When the designer asks for a package, the packaged-build gate (`py tools/packaged_bench.py` and the tour on the package) to
close phase 3; otherwise start phase 4 per docs/11 (the save container, doc 08 §2)."*

---

## 2026-11-02 (29) - Phase 3 exit: the playthrough through the UI; finding F22

### Where the project is
- **Phase 3's exit criteria are met** (doc 11): `Paw.Playthrough` green through the UI's entry points, the tree's layout tests
  green since (25), the screenshot tour reviewed (28). The one gate left before phase 3 is closed is the **packaged-build gate**
  (CLAUDE.md: packaged gates run at the end of every phase), which needs a package: only on the designer's ask.
- **Nothing committed** in either repo. In the Godot repo this session wrote only `docs/pawpocalypse-port-findings.md` (F21,
  F22); the uncommitted F20 changes from (23) stand.

### What happened
1. **`Pawpocalypse.UI.Playthrough`** (`UI/PawPlaythroughTests.cpp`): one run on the real root layout, HUD and realm, every action
   through the UI: a clean slate; real kills (the cat on the nearest mouse, hands taken through the card pick) until the dock
   can buy; nothing bought with nothing banked; eight dock purchases (each one level, never below zero, power never backwards);
   the Realms page's travel; the five bosses in order by the Bait button (the biome behind a standing boss shut, the kill opens
   it, one stage each and two for the last); the ending with the last boss only (it holds the world, OK gives it back); the
   Reawaken's two presses (paid what the page promised, the multiplier up, the run cleared, the bosses kept, the time kept); the
   ceremony, then the spend step (74 nodes, each exactly its price, a refused press spends nothing); out to the starting biome
   (no screen left, the world runs, the tree kept); kills pay again; everything finite. Godot's ci/check_playthrough.gd is the
   model: invariants, never balance. Save / load joins in phase 4. The gates a real run takes hours to open (this cycle's
   Catnip, a boss's bait kills) are credited as the cheat overlay credits them; the kill blows on a boss land between steps.
2. **Finding F22** (from its first run): after a Reawaken the Realms page walks past every boss without baiting it. Godot's rule
   (`is_biome_unlocked` -> `_bosses_cleared_for_stage`) re-locks the passage, and its comment says why; but the Realms rows ask
   only `is_tier_unlocked` (the permanent stage) and the Catnip, and the button changes scene unasked. The port copies Godot
   (`PcHud RealmRow`); the playthrough pins both halves. Doc 01 §5 and the Godot repo's findings file.
3. Docs 11 (phase 3 status), 09 (the playthrough row), 00. KB G/03 §6 (a rule tested on the state layer, bypassed by the UI).

### Verified
- `Pawpocalypse.UI.Playthrough`: PASS (upgrades 8, cards taken 2, nodes 74, Stardust paid 185405, kills 104).
- Sabotages `playthrough_upgrades_survive`, `playthrough_passage_kept`, `playthrough_payout_short`, `playthrough_nodes_free`:
  4/4 CAUGHT, clean PASS.
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102f.txt`; UI now 37 tests). UBT: Succeeded.

### Open
- **For the designer**: F21 (the job picker's and victory screen's look), F22 (the Realms page and passage); the packaged-build
  gate that closes phase 3 (ask for a package when ready).
- Nightly `-Full` not run since (23).

### Next step
*"Run the nightly `check_all -Full` (~3 h, editor closed). Then, when the designer asks for a package, the packaged-build gate
(`py tools/packaged_bench.py` and the tour on the package) to close phase 3; otherwise start phase 4 per docs/11 (the save
container, doc 08 §2)."*

---

## 2026-11-02 (28) - Phase 3 exit: the screenshot tour against Godot; two port bugs fixed from it

### Where the project is
- Phase 3 (docs/11): every screen built, and now looked at side by side with Godot. Left for phase 3's exit: `Paw.Playthrough`
  through the UI entry points. The only differences left on screen are phase 6's (art, fonts, tab icons, tutorial, Boba).
- **Nothing committed** in either repo. In the Godot repo this session wrote only `docs/pawpocalypse-port-findings.md` (F21, the
  one file the port writes there); the uncommitted F20 changes from (23) stand.

### What happened
1. **`py tools/godot_tour.py`** (new): the same steps and state in both games (new game, tier_1, 1000 Catnip; six tabs; tree
   browse; a card hand; a Reawaken with three jobs held: ceremony, spend step, job picker; the ending at 4200 s). Godot:
   `tools/godot_fixtures/tour_probe.gd` in the sandboxed scratch copy, rendered (opengl3). Unreal: the `paw.*` steps, plus new
   `paw.Wait <frames>` and `paw.Take` (through the card pick screen, as its button does; the realm's ChooseCard alone left the
   screen up and every later shot under it), with `t.MaxFPS 60` (the editor build ran ~500 fps uncapped, so frame waits were
   0.06 s). Shots pair in `build/tour/compare/<shot>.png`.
2. **The dims are right**: sampled on the dock's paper under each (same colour in both games), card pick and job picker (72, 64,
   79) vs (72, 64, 80), victory (42, 39, 45) both. The "dims read lighter" open item since (24) is closed: only the missing field
   art made them look lighter.
3. **Port bug fixed: the job picker's look.** Godot's JobPicker is a CanvasLayer under the HUD, like the victory screen, so it
   draws in Godot's DEFAULT theme: a translucent dark panel (radius 3, no frame, no margins), stock grey buttons, a white 16 px
   note, the INK title dark on dark. Measured by the `victory` fixture (`jp.*` keys in `victory_look_godot.txt`); new token
   `GODOT_PANEL`; `UPawJobPicker` rows `SetEngineLook`. The title being near-invisible is clearly not what job_picker.gd meant
   (it asks for `WindowPanel`): **finding F21** for the designer (doc 01 §5 and the Godot repo's findings file), ported as drawn.
4. **Port bug fixed: the Realms auto-travel box** was always shown; Godot hides it until the tree's Auto-Travel node is owned
   (`auto_travel_check.visible = GameState.owns_auto_travel()`). `RefreshRealms` now does the same (`Pc::OwnsAutoTravel`).
5. Docs 05 §6.3 (the picker's look, the tour), 11, 00, CLAUDE.md (the tour command). KB U/18 §4 (uncapped editor `-game` breaks
   frame-paced tours; a tour step must use the button's path; measure overlays on a shared colour), INDEX row.

### Verified
- UI automation 36 PASS (`Hud.Realms`: the box hidden, then shown once owned and in the column; `JobPicker.Asks`: stock buttons).
- Sabotages `jobpick_game_look`, `hud_auto_travel_always_shown`: CAUGHT, clean PASS.
- `make_godot_fixtures.py --only victory --check`: byte for byte; `gen_paw_tokens.py --check`: current.
- The tour: 12 of 12 pairs, no `LogPaw: Warning` in the Unreal log. Reviewed: tabs, tree browse / spend, card pick, ceremony,
  job picker, victory.
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102e.txt`). UBT: Succeeded.

### Open
- **F21** (designer): should the job picker and the victory screen wear the game theme? Until then both are ported as Godot
  draws them.
- Godot's tour window came out 1904x1001 despite `--fullscreen` (the compare sheet scales both to 960x540, so it is slightly
  squashed); Godot's own `tools/screen_shots.sh` says fullscreen gives 1920x1080 there. Not looked into.
- The victory and picker selection ring also shows on mouse hover (Common UI hovered = selected); Godot rings focus only.
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 exit per docs/11: `Paw.Playthrough` (reset, earn, buy, travel, kill bosses, Reawaken, buy nodes, the ending) through the
UI entry points, as an engine automation test; then the nightly `-Full`; ask the designer about F21."*

---

## 2026-11-02 (27) - Phase 3 slice 7: the victory screen; numbers printed Godot's way

### Where the project is
- Phase 3 (docs/11): every screen in its steps is built (the Unicorn ceremony waits for the tree's art, phase 6). The ending
  comes up when the last boss falls. Left for phase 3's exit: the screenshot tour against Godot and `Paw.Playthrough`.
- **Designer decision this session (2026-11-02): "Match godot number".** Numbers are now printed as Godot prints them:
  ungrouped, a '.' point in every language ("+125000 Stardust", not "+125,000").
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand; this session wrote nothing there).

### What happened
1. **Fixture `victory`** (`tools/godot_fixtures/victory_fixture.gd`): the REAL hud.tscn (which builds VictoryScreen and connects
   it to `boss_defeated`), 3,000 random ledgers: `format_duration`, `mark_boss_defeated` (signal, new, the clock), whether the
   ending came up, its line, the pause flag before / up / after OK. It also prints the screen's resolved LOOK, which the runner
   saves to `Tests/Data/victory_look_godot.txt`. Finding: the screen is its own CanvasLayer outside the HUD's themed Root, so
   **Godot draws it in the engine's DEFAULT theme** (white 16 px Open Sans SemiBold labels, the stock grey button).
2. **PawCore**: `PcHud DurationParts` (format_duration), `PcProgress RaisesVictory` (boss_defeated emitted AND the last boss
   realm), the field's `FieldFlag::Victory` on `BossDefeated`. `PcVictoryTests.cpp`: all 1,000 rows equal (all 3,000 in the
   long test), coverage asserted; a field test (first clear, first rematch of a cycle: up; already cleared this cycle, another
   boss: not).
3. **`UPawVictoryScreen`** on the **Modal** stack (pause holder "victory"; Godot's layer 128 is over the card pick's 100):
   dim (eats clicks), headline, flavour, "Cleared in {time}" (`PawText::Duration`), OK (focused, no guard, no Back). The HUD
   raises it from the field event with `FinalBossSeconds` (`ShowVictory`, never twice). Look: `PawTokens::GODOT_*` generated
   from the look file (`gen_paw_tokens.py` LOOK_COLOURS), `VICTORY_DIM` from the script's literal (CODE_COLOURS),
   `UPawButton::SetEngineLook` / `UPawEngineButtonStyle`. Strings `VICTORY_*`, `DURATION_hms/ms`.
4. **Numbers**: `PawText::Count` is culture-invariant ungrouped digits; new `TwoDigits` ("%02d") and `Fixed` ("%.Nf" through
   `Pc::FixedLikeGodot`, Windows Godot's half-up rounding); the HUD's boss HP %, boss timer, realm reward multiplier and the
   Reawaken multipliers moved off `FText::AsNumber`. L0 `ui-rules` now fails on `FText::AsNumber/AsPercent/AsCurrency` in UI
   code (self-test plant added). Docs 05 §6.3 and 08 updated.
5. Tour step `paw.Victory [seconds]`.
6. **KB**: G/07 (a CanvasLayer breaks theme inheritance: code-built screens get the engine's default theme), G/06 §3 (Godot's
   built-in font is Open Sans SemiBold), U/20 §1 (`FText::AsNumber` groups; how to print culture-invariant), INDEX rows.

### Verified
- PawCore L1: 150 test cases PASS (victory fixture 1,000/1,000; coverage: hours 365, negative clocks 41, shown 449, from a
  rematch 120, silent last-boss kills 121, other bosses 346).
- UI automation: 36 PASS (+3: `Victory.Ending`, `.OnlyFinal`, `.Numbers`).
- Sabotages: PawCore 8/8 CAUGHT (`duration_*`, `victory_*`); game module 15/15 CAUGHT (`victory_*`, `numbers_*`,
  `duration_minutes_unpadded`), clean PASS (`build/sabotage_victory.txt`).
- Screenshot (editor build, `-game`): `Saved/PawShots/victory.png`: the dim, THE VOID IS QUIET, the two flavour lines,
  "Cleared in 1h 10m 00s", the grey OK, Godot's layout.
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102d.txt`); `check_pawcore_static.py --self-test`: every
  plant CAUGHT, the new `ui-rules: engine number format` included. UBT: Succeeded.

### Open
- The dims (card pick, job picker, now the victory screen) read lighter than their alpha should: still unchecked against Godot
  side by side (likely blending; the tour will tell).
- The engine font (Open Sans SemiBold) waits for the font phase; the victory OK shows its ring on mouse hover too (Common UI's
  hovered = selected); no Boba cameo (phase 6).
- Earlier placeholders stand (window plate, job_taken save, hat ring; doc 05 §6.3).
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 exit per docs/11: the screenshot tour against Godot: the card pick, the tree, the job picker, the victory screen and
every tab, side by side with Godot shots (and settle why the dims read lighter); then `Paw.Playthrough` through the UI entry
points; then the nightly `-Full`."*

---

## 2026-11-02 (26) - Phase 3 slice 6: the Reawaken ceremony and the job picker

### Where the project is
- Phase 3 (docs/11): a Reawaken now runs Godot's whole order: the payout read, the reset, **the ceremony**, the tree's spend
  step, **the job picker** (when `should_ask`), then the travel to the starting biome. Left in phase 3: the victory screen.
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand; this session wrote nothing there).

### What happened
1. **`PcHud JobPicker` / `PickJob`** (PawCore): what job_picker.gd offers (the held jobs that have data, by id; "(last run)" and
   the default row; the baseline last; the second ask under Union Rules without the primary, with its share) and what a press
   does (lock, ask again, or lock the second slot and mark the run asked). New fixture **`job_picker`**
   (`tools/godot_fixtures/job_picker_fixture.gd`): the REAL JobPicker opened on 3,000 random states, its rows read back off the
   buttons, a press through `_pick`, and the reopened list when it asks again: **all equal** (1,000 L1 rows, all 3,000 in the
   long test). Only one test-side fix: with nothing but the baseline offered, the first blurb is the baseline's own.
2. **`UPawReawakenCeremony`** (reawaken_ceremony.gd): the HUD's top layer, hit-test invisible; flash + "REAWAKENING" in (0.35 s),
   the payout (0.25 s), hold 1.1 s, all out (0.45 s), one sine in-out tween (`AlphasAt`). The field runs under it (Godot does
   not pause for it), but its clock stops while anything holds the field (Godot's bound tween). The payout is read BEFORE the
   reset (Second Helping makes it differ from what lands). Its colours are scene literals: `tools/gen_paw_tokens.py` now also
   generates `CEREMONY_*` from reawaken_ceremony.tscn (SCENE_COLOURS; `SrgbF` = Godot's srgb_to_linear).
3. **`UPawJobPicker`** (job_picker.gd) on the **Modal** stack (pause holder "job_picker"; `SetModalStack` from the player
   controller): title, note, rows + blurbs (pooled), the card pick's guard (restarted by the second ask), one wired column,
   opens on last run's job. `UPawRealmSubsystem::PickJob` rebuilds the stats.
4. **The HUD**: `DoReawaken` -> ceremony; its end -> `OpenSpendStepWhenFree` (waits while anything is on the Menu stack, never
   skips); the tree's leave -> `ReturnToStartingBiome` (the picker when `ShouldAskForJob`, its answer -> `RefreshJobs` + travel).
5. `ST_UI` + 2 `CEREMONY_` and 8 `JOBPICK_` lines; L0 key prefixes + both. Doc 05 §6.3 has the design and the differences kept.
6. Screenshot tour steps `paw.Reawaken [jobs]` (the HUD's two presses on a run made worth it) and `paw.Leave`.
7. **Tests**: UI 33 (+6: `Ceremony.Tween/.Plays/.Waits`, `JobPicker.Asks/.Union/.NoJobs`; `Hud.Reawaken` and `Tree.Spend` now
   play the ceremony out); L1 147 (+3); PawCore sabotages +9 (`picker_*`, all CAUGHT after one test added: no last-run row ->
   first row); game-module sabotages +25 (`ceremony_*`, `jobpick_*`, all CAUGHT first time); `cards_choose_keeps_stale_stats`
   made unique again (PickJob repeats its line).
8. **KB**: G/03 §6 (a UI script's decisions pinned against the real UI node; a bound tween freezes under a pausing modal).

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102c.txt`); L0 again after the tour steps. UBT: Succeeded.
- `py tools/sabotage_realm.py` (the 25 new + the 2 touched): 27/27 CAUGHT, clean PASS (`build/sabotage_realm_1102c.txt`).
  `py tools/sabotage_pawcore.py --only picker_*`: 9/9 CAUGHT.
- Screenshots (editor build, `-game`): `Saved/PawShots/ceremony_payout.png` (REAWAKENING, "Reawaken 1 / +125,000 Stardust"),
  `w1.png` (the fade out, the field showing through), `spend_after_ceremony.png`, `job_picker.png` (three jobs, "Daredevil
  (last run)", "No job" last, over the dimmed HUD).

### Open
- **Number grouping**: Godot formats `{n}` ints ungrouped ("+125000 Stardust", "Stardust: 125000"); the port's `PawText::Count`
  is `FText::AsNumber` (doc 08's mapping), so it shows "125,000". Visible on the ceremony, the tree's balance and costs, the Jobs
  page. A port bug or the intended localisation? A designer call (doc 08 §4), not changed here.
- The dims (card pick, job picker) read lighter than Color(SKY base, 0.84) should: still unchecked against Godot side by side.
- Placeholders: the window plate is a rounded box (ui_window.svg's nine-slice, phase 6); job_taken's save (phase 4) and the
  hat's ring (phase 6). The ceremony sits under the Menu stack: a tree browsed from under the flash draws over it (doc 05 §6.3).
- Not covered by tests: painting (screenshots are the check), the guard through real Slate key events (phase 5).
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 per docs/11: the victory screen (victory_screen.gd, on the HUD: show_victory / dismiss). There is NO achievement
toast in Godot (hud.gd _on_achievement_earned only refreshes the list and the tab badge; the earn sound is phase 6), so the
earlier handoffs' "achievement toast" step is dropped. Then ask the designer about number grouping (Open); side-by-side screenshots of the card pick, the tree and the job picker
against Godot; then the nightly `-Full`."*

---

## 2026-11-02 (25) - Phase 3 slice 5: the skill tree screen, browse and the Reawaken spend step

### Where the project is
- Phase 3 (docs/11): every dock page, the card pick and the **skill tree** (browse, and the spend step after a Reawaken) are built.
  Left in phase 3: the Reawaken ceremony and the job picker around the spend step, the achievement toast, the victory screen.
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand; this session wrote nothing there).

### What happened
1. **The tree's words and colours are generated** (`tools/gen_tree_tables.py --godot ...`, L0 checks them current, its self-test
   plants each): from skill_tree_ui.gd and skill_tree_connector_layer.gd -> `Content/.../ST_Tree.csv` (87 captions =
   BRANCH_LABELS, 87 blurbs, 79 effect phrases from `_describe`'s cases incl. per-branch and tr_n ones, 4 channel phrases),
   `PawCore/Public/PcTreeWording.h` (which phrase and which GDScript number expression each effect uses), `UI/PawTreeTables.h`
   (BRANCH_COLORS, RAINBOW / PULSE lists, the tooltip's 5 bbcode colours, DIM / SHUT, SHINE_COLOR). It fails on anything in
   `_describe` it cannot read.
2. **`PcTreeText`** (PawCore): the effect line's phrases at a level, and the tooltip's decisions (Now / Next / Cost; Closed by a
   rival, needs the parent, the Reawaken gate, browse-only, too poor; the buy prompt). **`PcFormat`** gains Godot's "%.Nf"
   (17 digits then half up, any N), `roundi`, `int`. New fixture `tree_text` (`tools/godot_fixtures/tree_text_fixture.gd`):
   every branch at every level, and 2,000 values x the 9 number expressions: **all equal** after one fix (Godot prints "-0").
3. **`UPawSkillTreeScreen`** (doc 05 §6.2 has the design): the canvas, painted links and rings, node frames by state, halos, shine,
   captions, hub, pan / zoom / fit, Q / X next affordable, the tooltip; browse (T / pad Y from the HUD, read-only) and the spend
   step (`UPawHudScreen::DoReawaken` opens it; leaving asks once; then `TravelToStartingBiome`; with no Menu stack it travels at
   once, as Godot with no tree). `ST_UI` + 24 `TREE_` lines and 6 `PROMPT_` placeholders (key names until phase 5's glyphs).
4. **The screenshot tour found a crash the tests could not**: `UPawTreeLinks::RingPath` added an array's own element to it
   (`Pts.Add(Pts[0])`), an assert the moment the tree painted; automation tests never paint. Fixed; KB U/24.
5. `paw.Tree [spend]` and `paw.Hand` (queued, with `paw.Shot`): the tree and a dealt hand for screenshot tours.
6. **Tests**: UI 27 (+7 `Pawpocalypse.UI.Tree.*`); L1 145 (+3); game-module sabotages 162 (+32 `tree_*`, all CAUGHT after a second
   round: the cheapest node was also the first trunk, and a browse's travel was guarded twice and invisible from the starting
   biome; one older sabotage repointed); PawCore sabotages 387 (+14, all CAUGHT; one older one made unique again).
7. **KB**: U/24 (a widget passes headless tests and crashes on first paint: `TArray::Add` of its own element).

### Verified
- Everyday `check_all`: every gate PASS (`build/check_all/_everyday_1102b.txt`). UBT editor build: Succeeded.
- Screenshots (editor build, `-game`): `Saved/PawShots/tree_browse.png`, `tree_spend.png` (READY halos, Continue), `card_pick.png`.
  The tree matches the Godot shape (six limbs, rings, the Unicorn's pulse, job rings).

### Open
- **Look at the screenshots against Godot**: the card pick's dim reads lighter than Color(SKY base, 0.84) should (Slate may blend
  translucency differently); at the fit zoom (~0.37) the captions are small, as in Godot.
- Placeholders (doc 05 §6.2): prompt key names, flat sky, no icons, halo without shadow, no hover lighten, no READY jitter, no
  Unicorn ceremony; the ceremony and job picker around the spend step.
- Not covered by tests: painting (screenshots are the check), hover, the stick / triggers, the follow glide.
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 per docs/11: the Reawaken ceremony (reawaken_ceremony.gd, before the spend step) and the job picker (after it, when
JobPicker.should_ask()), then the achievement toast; side-by-side screenshots of the card pick and the tree against Godot; then the
nightly `-Full`."*

---

## 2026-11-02 (24) - Phase 3 slice 4: the card pick screen; slice 5 begun: the skill tree's layout in PawCore

### Where the project is
- Phase 3 (docs/11): every dock page and now the **card pick** are built; the **skill tree**'s PawCore half (`PcTreeLayout`) is done
  and exact against Godot; its screen is next.
- **Nothing committed** in either repo (the Godot repo's uncommitted changes from (23) stand: `hud.gd`, `tier_controller.gd` (F20),
  `card_manager.gd` (F1), `docs/pawpocalypse-port-findings.md`; this session wrote nothing there).

### What happened
1. **The card pick** (`UI/PawCardPickScreen.h/.cpp`, `catches_pick_ui.gd`): the Menu stack's first screen. Cards (`UPawCardFace`: a
   never-focusable Common button = the whole-card click) with the flavour band and word, NEW / "LEVEL n of max", the description at
   the level taking it gives, the red frame and warning for a cost, Take, Banish while a banish is held; Reroll with its count;
   Dilated's bigger hands; no close, no Back. The guard (0.4 s after every deal; a key press must begin strictly after the deal:
   Godot's `>=` let the press that dealt the hand through on the same millisecond, its comment says the intent is this). Pad rows
   Take / Banish / Reroll by `PawUi::WireRows` (the port of `MenuFocus.wire`). Doc 05 §6.1 slice 4 has the whole design.
2. **The HUD opens it** (`OpenCardPickIfDealt`, every frame while the progress holds a hand; the stack's list is the one guard against
   a second); the player controller hands the HUD its Menu stack. **Hold-to-buy stops** under it.
3. **The realm's pause holders** (`UPawRealmSubsystem::SetPauseHold(FName, bool)`; `SetPaused` = "manual"): one screen closing
   never resumes the field under another; a restarted field stays held. **The click that takes a card is never a Pounce**:
   `SetPounceDown` swallows a press while held and until the button is let go (Godot pounces from `_unhandled_input`; the port polls).
   `RerollHand` / `BanishFromHand` (each deal its own roll stream off the field's seed).
4. The root layout's stacks have **no transition** now (KB U/08 rule 6): a screen is up the frame it is added.
5. `ST_UI` + 17 `PICK_` lines; L0's key prefixes + `PICK`.
6. **`PcTreeLayout`** (`Source/PawCore/Public/PcTreeLayout.h`): branch order, the hex seating (`_seat`), the limb push, separation,
   normalising, bounds, canvas, fit zoom, `nav_pick`, node states (LOCKED / READY / OWNED), links (owned / fork / shut), cheapest
   and next affordable, has-affordable. New fixture `tree_layout` (`tools/godot_fixtures/tree_layout_fixture.gd`: the real
   `skill_tree_ui.gd` instanced without its scene, the shipping tree + 400 random trees): **17,412 seats, 0 px off; every pad pick
   equal; the same 10 crowded-limb fallbacks**. The first port in double got 4,496 picks and 147 seats wrong: Godot's `Vector2` is
   32-bit and the lattice puts neighbours exactly on the 60 degree cone edge, so the layout now emulates 32-bit `Vector2` arithmetic
   per operation (rounding through `frexp`/`ldexp`; no `float` in PawCore). Godot's separation pass never moves a hex seat (240 px
   pitch > 196 px gap): exposed as `SeparateTreeNodes` and tested on a crafted layout.
7. **Tests**: UI 20 (+8 `Pawpocalypse.UI.CardPick.*`); L1 142 cases (+4); game-module sabotages 131 (+34, all CAUGHT); PawCore
   sabotages 373 (+24, all CAUGHT after a second round: 3 did not compile, 4 were missed, each fixed by a test that can see it).
8. **KB**: U/08 §3.4 (an activatable stack in an automation test needs `TakeWidget()` first; the 0.4 s default transition;
   `SetFocus` without a local player is an ERROR; nested Common buttons), G/07 (Godot's 32-bit `Vector2` in a port), INDEX symptoms.

### Verified
- Everyday `check_all` (2026-11-02, after both slices): every gate PASS (`build/check_all/_everyday_1102.txt`). UBT editor build with the final sources: Succeeded.
- `py tools/sabotage_realm.py` (the 34 `pick_*` / `realm_*`): 34/34 CAUGHT, clean PASS. `py tools/sabotage_pawcore.py` (the 24
  `layout_*` / `nav_*` / tree jumps): 24/24 CAUGHT, clean PASS.
- No screenshot of the card pick yet (no package, no `-game` run this session).

### Open
- The card pick is not seen on screen yet: a `-game` screenshot (a hand via `paw.Cheat.*`) belongs to the next session's tour.
- Not covered by tests: the card's hover / focus lift, the guard through real Slate key events (phase 5), hold-to-buy stopping under
  the pick. Card art waits for phase 6.
- The skill tree's names, blurbs, effect words and per-branch colours live in Godot CODE (`BRANCH_LABELS`, `BRANCH_BLURBS`,
  `EFFECT_DESCRIPTIONS`, `BRANCH_COLORS` in `skill_tree_ui.gd`): to be GENERATED (string tables, a colour header), not hand-copied.
- Doc 05 §6.2 described the tree as one lattice: it is six limbs (`TRUNK_WHEEL`) on hex lattices, 87 nodes; corrected there.
- Nightly `-Full` not run since (23).

### Next step
*"Phase 3 slice 5 per docs/11 and doc 05 §6.2: the skill tree SCREEN on `PcTreeLayout`. First generate `BRANCH_LABELS` /
`BRANCH_BLURBS` / `EFFECT_DESCRIPTIONS` into the string tables and `BRANCH_COLORS` into a header (from `skill_tree_ui.gd`, L0 checks
them current); then `UPawSkillTreeScreen` on the Menu stack with pause holder "tree": opaque sky, canvas with pan / zoom (fit on
open, wheel and buttons, RT / LT), links and rainbow / job rings painted under the nodes, node frames by state with the READY halo,
counters, captions, Stardust line; browse (T / pad Y, read-only, X) and the Reawaken spend step (Continue, the two-press leave,
buy through `PurchaseNextBranchLevel`, then travel: ceremony and job picker later); Q / X next affordable; the tooltip; its
`Pawpocalypse.UI.Tree.*` tests and sabotages; screenshot tour incl. the card pick."*

---

## 2026-11-02 (23) - The night: Territory wins (F20, Godot first); phase 3 slice 3, every dock page built

### Where the project is
- Phase 3 (docs/11): all six dock pages are live (Upgrades, Reawaken, Realms, Catches, Achievements, Jobs); the realm earns
  achievements; the card pick's decisions are in PawCore, its screen is not built. Night list:
  `docs/WORKLIST-NIGHT-2026-11-01.md` (closed).
- **Nothing committed** in either repo. **The Godot repo has two new uncommitted changes for the designer**:
  `scripts/systems/hud.gd` and `scripts/systems/tier_controller.gd` (F20, below), beside the earlier `card_manager.gd` (F1) and
  `docs/pawpocalypse-port-findings.md`.

### What happened
1. **Territory wins (designer, 2026-11-01).** Measured first on the REAL Godot HUD (new `tools/check_godot_reawaken_travel.py`:
   a sandboxed copy, a watcher node on `root`, four Reawakens): with Territory deeper than the biome you Reawaken in, its
   1.1 s timer changed scene during the 1.9 s ceremony and **the spend step (skill tree, job picker) never opened**; at or
   above it, Territory was ignored and the run went to tier 1. That is **F20** (doc 01 §5, Godot findings, Fixed). Godot
   fixed: `hud.gd` `_return_to_starting_biome()` / `_travel_to_starting_biome()` take the run to `get_starting_biome_index()`
   at the end of the spend step; the timer travel removed from `tier_controller.gd`. Godot's ci scripts / hud / damage /
   playthrough / mirror / tutorial pass; the check passes 4/4 and fails 3/4 on Godot's HEAD (`--head`). Port:
   `Pc::ReawakenTravelIndex` (L1, 3 sabotages), `UPawHudScreen::DoReawaken`; the realm's deferred travel deleted
   (`.Realm.ReawakenTravel` now asserts the realm never moves; `.UI.Hud.Reawaken` checks deeper, shallower, same).
2. **The Catches page**: every card in deck order as a flat, selectable row: held = "<name>  (Lv n/max)" over its description
   with `{v}`/`{c}` filled (`PcHud` `CardNumber` = `CardData._fmt`); not held = what stands in the way (`Pc::CardUnlockHint`,
   `unlock_hint()`'s gate order) or "Not taken yet."; the summary; the Auto-pick box (with Auto-Card; writes the realm).
   `UPawButton` gained a detail line, left alignment and a flat style.
3. **The Achievements page**: summary, Claim all, one row per achievement (mark, name, description or progress, Claim with its
   amount). **Found by the screenshot tour: nothing in the game earned achievements** (Godot re-scans on every kill, currency
   change, purchase...): `FPawRealmSim` now re-scans after every fixed step, `UPawRealmSubsystem` after a progress change, a
   summon and a Reawaken; earned ids queue in `EarnedAchievements` (the toast is not built).
4. **The Jobs page**: worn / owned / no node / closed by a rival / beat a boss / Reawaken n times / ready at its price; the
   state button is a statement, always locked (the pad still reads it). The tab stays hidden until a job is held (as Godot).
5. **Stretch**: the card pick's decisions, `PcHud` `CardPick` (level, each card's next level, value, price, NEW, the cost
   frame, foresight, rerolls, banish).
6. **Strings**: `ST_Data` gains CARD / ACH / JOB names and descriptions and SKILL names (`gen_data_strings.py`; 277 rows);
   `ST_UI` + 34 lines (CATCHES_, AWARDS_, JOBS_; L0's key prefixes widened). `PawText::DataField`, `PawText::CardNumber`.
7. **Tests**: L1 138 cases (+6); PawCore sabotages 349 (+42; first MISSED twice: a near-whole card number, and a flavour
   that was the default: both tests tightened, then CAUGHT); UI 12 tests (+3); game-module sabotages 97 (+19, all CAUGHT).
8. **KB**: Godot/01 §1 + G/07 + INDEX (one scene change per modal flow: a timer scene change frees the modals; how to test it
   headless).

### Verified
- Everyday `check_all`: every gate PASS (00:24). Nightly `-Full` (00:33-05:22): 20 of 21 gates PASS (PawCore sabotage 349/349 CAUGHT, L1 long suite, pawsim parity sweep, Godot fixtures, Godot repo untouched); **Realm sabotage FAILED** on two counts, both handled the same morning: (a) `hud_reawaken_realm_not_told` MISSED: since the HUD always travels after a Reawaken, the travel rebuilds the field and hid what `OnReawakened` does; the one effect only the signal has is earning the Reawaken's achievements before any step, so `.Hud.Reawaken` now asserts Nine Lives (`reawaken_1`) is earned by the press itself; CAUGHT. (b) the clean rerun of `Pawpocalypse.Realm.ReawakenTravel` failed once and passed 6 times alone (fixed seed, own process): cause unknown, because `run_automation` overwrites its log per filter; `sabotage_realm.py` now keeps a failing clean run's output (`build/sabotage_realm_clean_fail_*.log`) and prints its errors. Full realm sabotage rerun 06:07: **97/97 CAUGHT, clean PASS**; everyday `check_all` after it: every gate PASS. Watch for (b) recurring: if it does, the kept log names it.
- Screenshots from the editor build (`-game`; no package): `Saved/PawShots/catches.png`, `awards.png` (3 earned with their
  Claims after `paw.Cheat.AdvanceStage`), both as intended.

### Open
- Godot: commit `hud.gd` + `tier_controller.gd` (F20) when reviewed; F13, F14, F16-F19 still wait for decisions.
- Not built: the card pick screen, the skill tree, the ceremony, the job picker (Godot's Reawaken runs them before the
  travel), the achievement toast, tooltips, the Reawaken banner and glow, the travel list's remembered scroll; hold-to-buy
  untested. The Achievements tab's count spills past its text tab (goes with phase 6's icons). The skill tree screen will
  need `BRANCH_LABELS` (one differs from the data: `overkill` = "Splash").
- The Jobs page was not screenshotted (no cheat grants a job; its tab is hidden without one).

### Next step
*"Phase 3 slice 4 per docs/11: the card pick screen (a Modal-stack Common UI screen over the field; `PcHud` `CardPick` is
done): three cards with Take, Banish when held, Reroll, the guard against an accidental confirm, pause while up; its
Pawpocalypse.UI test and sabotages; then the skill tree."*

---

## 2026-11-01 (22) - Phase 3 slice 2: the Realms and Reawaken pages; rows locked, not disabled

### Where the project is
- Phase 3 (docs/11): the dock now has three live pages (Upgrades, Realms, Reawaken); Catches, Achievements and Jobs show their
  titles only. A full Reawaken can be played from the HUD (two presses, then back to the first biome or Territory's).
- **Nothing committed** in either repo.

### What happened
1. **`PcHud`** + the Realms rows (here / travel / boss, cleared / Catnip owed / locked; Cartography; new), the stage line, the
   Bait button (elsewhere / boss here / bait owed / Catnip short / ready; enabled; calling) and the Reawaken page (progress,
   payout, total, multipliers, the button's four kinds, the Unicorn line). 4 L1 tests; 15 sabotages, all caught after the
   tests were tightened (4 first missed: a stage-locked biome that still owed Catnip, the boss up with the next bait earned,
   a locked bait hiding the enabled flag, a total checked only with nothing to add).
2. **The pages** (`UPawHudScreen`): Realms (stage line, hint, Bait, auto-travel box, twelve rows in a scroll box, a press
   travels) and Reawaken (blurb, progress bar, payout with its Stardust disc, total, multiplier, Unicorn line, the button, the
   two-press confirm moving the selection to Cancel, Cancel, "Ask before Reawakening"; a tab change disarms). The reset:
   `Pc::Reawaken`, the realm's `OnReawakened`, then the first biome unless Territory's travel is pending. ST_UI + 32 lines.
3. **Rows LOCKED, not disabled** (`UPawButton::SetLocked`): slice 1 disabled unaffordable rows, and Slate never focuses a
   disabled widget (engine source: `SlateApplication.cpp`), so a pad could not move onto them; Godot's navigator selects them
   ("the player reads an unaffordable row"). Common UI's interaction off + a locked style that keeps the focus ring. Every
   page's navigation column includes its locked rows (`PageColumn`), and focus is remembered per page.
4. **Tests**: `Pawpocalypse.UI.Hud.Realms`, `.Hud.Reawaken` (9 UI tests); 15 new game-module sabotages + 2 retargeted, all
   caught (2 first missed: a total that "1.0M" rounding hid; the realm's `OnReawakened`, invisible because the return trip
   always re-enters the first biome: a Territory case now proves it). L0 key prefixes widened. `paw.Tab <n>` (dev) for tours.
5. **Found, matching Godot**: the reset clears the current biome, so the return to the first biome runs from inside it too
   (Godot's `_travel_to_first_biome` reloads the scene then).
6. **Docs**: 00, 05 §6.1, 09 (L1 132 cases, L3 UI 9 tests, sabotages 307 PawCore / 78 realm), 11. **KB**: U/08 §3.3 (a row
   that must be selectable but not pressed: interaction off + a locked style, never disabled; `bLocked` is taken).

### Verified
- `check_all` everyday: every gate PASS. PcHud sabotages 35/35; HUD game-module sabotages 33/33.
- Packaged screenshots `build/package/Windows/Pawpocalypse/Saved/PawShots/realms.png`, `reawaken.png`: both pages as intended.

### Open
- **Godot question (not checked)**: after a Reawaken with Territory, Godot both schedules the deeper travel (a SceneTree timer,
  which runs while paused) and, when the tree closes, travels to the first biome. Which one wins with the tree open needs a
  Godot run; until the tree exists the port does Territory's travel and skips the first-biome return.
- Not built: the ceremony, the skill tree, the job picker (the Reawaken sequence will route through them), the Reawaken
  banner and glow, the travel list's remembered scroll, tooltips; hold-to-buy still untested.
- The -Full nightly was not rerun (the realm sabotage gate is now ~100 min).

### Next step
*"Phase 3 slice 3 per docs/11: the Catches page (this run's cards, the auto-pick box), the Achievements page (rows, Claim,
Claim all, the count on the tab) and the Jobs page. Decisions in PcHud first (L1 + sabotages), then the pages, then
Pawpocalypse.UI tests and their sabotages, then a screenshot tour (`paw.Tab`, `paw.Shot`)."*

---

## 2026-11-01 (21) - Phase 3 slice 1: the HUD and the dock's Upgrades page

### Where the project is
- **Phase 3 started** (docs/11). L_Realm now opens with the HUD: the dock on the left (six tabs, badges, the Upgrades page
  with live prices, bulk buy and hold-to-buy), the status row (Catnip, Stardust, the wave line, the zone plate), the Catches XP
  bar, the Pounce clock and the boss panel; the field is measured beside the dock and a click on the dock never Pounces.
- **Nothing committed** in either repo (Godot repo unchanged this session beyond entry (20)'s F19).

### What happened
1. **`PcHud`** (PawCore): the decisions of hud.gd and tier_controller.gd's boss panel, no strings: shop rows (hidden,
   kitten-locked, MAX, price, bulk; enabled), the buy multipliers and the press, the Catches bar within the level, the zone plate
   (bait over ready over the Catnip gate), the wave line, the Pounce clock, the boss panel, tab badges and visibility. 7 L1
   tests; 20 sabotages, all caught (three first failed to BUILD: a sabotage left a parameter unused; and one was missed: no test
   had bait and a new biome at once; a case was added).
2. **Text and look**: `ST_UI.csv` (Godot's English for the HUD) and `ST_Data.csv` (generated, `tools/gen_data_strings.py`),
   registered by the game module's `StartupModule`, staged loose (`DefaultGame.ini`); `PawText` accessors. `UI/PawTokens.h`
   generated from Godot's `palette.gd` (`tools/gen_paw_tokens.py`); `PawUiConstants.h` (Godot's presentation constants, each with
   its source). L0 `ui-rules` (colour values, hard-coded text, keys named and used) with four planted violations caught; the
   `generated` gate checks the two new generated files.
3. **UI** (`Source/Pawpocalypse/UI`): `UPawRootLayout` (Game / Menu / Modal), `UPawHudScreen`, `UPawButton` (focus = a ring),
   `UPawPounceClock` (painted), `PawUiKit`; `APawPlayerController` creates the root and pushes the HUD where there is a realm
   view. The realm: `UPawRealmSubsystem::SetViewSize` / `SetDock` / `IsOverDock`; the view reports its size and ignores a
   mouse Pounce over the dock. `Build.cs`: the UI modules are permanent dependencies now (were listed as spike-only).
4. **Tests**: `Pawpocalypse.UI.*` (7: strings, readouts, press, dock, hit-test, navigation, boss) and 18 game-module sabotages
   (`hud_*`, `realm_dock_click_pounces`), all caught; `check_all` gains `L3 Pawpocalypse.UI`. `paw.Shot <name> [quit]` (dev):
   screenshots for tours.
5. **Seen in the packaged build** (screenshots): the first pass drew labels over the field as ink blocks (Slate's outline 5 is
   much heavier than Godot's); 2 reads right. Tab names did not fit; 13 px until the icons.
6. **Docs**: 00, 03 (`PcHud`, the HUD row, the dock-measured field), 05 §6.1 (built, placeholders), 08 §4 (the tables are
   live), 09 (L0, L1, L3 UI, sabotage counts), 11 (phase 3 status), CLAUDE.md (two generators). **KB**: U/08 §3.3 (Common UI
   header needing GameplayTags), U/09 §3 (no `UBorder::GetBrush`; outline sizes), U/20 §2 (CSV tables at startup, multi-line
   values), U/19 §2 (Git Bash mangles a `/Game/...` argument), INDEX rows.

### Verified
- `check_all` everyday: every gate PASS (L0 incl. ui-rules, L1 128 cases, mirror, fuzz, L3 Data, Realm, UI, Godot untouched).
  Sabotages: PcHud 20/20, HUD game module 18/18. L0 self-test: every planted violation caught.
- Packaged gate with the HUD: PASS, but the HUD costs: draw calls 13.6 -> 35.3 (budget 50), game thread p99 2.73 -> 3.12 ms
  (budget 4), render thread p99 8.8 -> 14.2 ms (uncapped, ~220 fps).

### For the designer
- Look at `build/package/Windows/Pawpocalypse/Saved/PawShots/hud_rich.png` (the HUD as it stands; placeholders: tab names
  instead of icons, discs instead of the currency icons and the badge star).
- Decide F19 (and F13-F18). Commits as before, plus `Source/Pawpocalypse/UI/`, `Source/PawCore/*/PcHud*`,
  `Content/Pawpocalypse/Localization/`, the controller, the realm subsystem and view changes, `Pawpocalypse.cpp`,
  `Pawpocalypse.Build.cs`, `Config/DefaultGame.ini`, `Dev/PawShot.cpp`, the new tools.

### Open
- Not built yet: the other five pages (titles only), the quick bar, tooltips, the fps readout, the tutorial overlay; the dock's
  keys are read raw until phase 5. Hold-to-buy is implemented but not tested (needs a real held press: the phase 5 input tour).
- The HUD's draw cost: try an invalidation box around the dock (KB U/09 §1) if the budget gets tight.
- The -Full nightly was not rerun; the realm sabotage gate is now ~80 min (63 sabotages).

### Next step
*"Phase 3 slice 2 per docs/11: the Realms page (twelve rows, gates, the current biome, Bait with its states, auto-travel box)
and the Reawaken page (progress, payout, multiplier, the two-press confirm with focus moved to Cancel). Decisions in PcHud first
(L1 + sabotages), then the pages in UPawHudScreen, then Pawpocalypse.UI tests and their sabotages."*

---

## 2026-11-01 (20) - Phase 2 closed: every card effect in a live realm; the packaged-build gate; F19

### Where the project is
- **Phase 2 DONE** (docs/11): every exit criterion met. **Next is phase 3**, the progression UI.
- **Nothing committed** in either repo. The Godot repo: only `docs/pawpocalypse-port-findings.md` changed by the port (F19
  added); `scripts/autoload/card_manager.gd` is the designer's own uncommitted Second Opinion fix of 2026-10-29 (untouched).

### What happened
1. **`Pawpocalypse.Realm.CardEffects`** (`Source/Pawpocalypse/Realm/PawCardEffectsTests.cpp`; the port of Godot's
   `ci/check_card_effects.gd`): a standalone game instance with its real subsystems; each card taken to its maximum level
   through the game's own entry, the realm run through `AdvanceFrame`. One case per effect, run twice on one seed (without the
   card, then with it); the consumer's mark must show only with the card: trail zones, pulses, slowed mice, rim damage on a
   frozen fat mouse, kitten blasts, the gilded kitten, Lone Hunter (and its kitten cost at the next sync, F14), chain damage
   beyond the rim, craters, Bloodlust's refunds, Peer Pressure's extra damage on the Pounce frame, golden and signature spawn
   counts, bombs, puddle length (exactly +10 s) and count, Bounty (5x the control's payout, Currency Find raised so rounding
   cannot fake it), Compound Interest (exactly the floored share of the bank), Scavenge, Dilated (+2 cards in the dealt hand),
   Spite/Not That One/Fixation held and spent on the hand the realm deals. A coverage check over every `CardEffect` (the 3
   with no card asserted cardless), Grudge Ledger's stats rebuilt on a take, a fixed seed replays exactly; F13 pinned by the
   blast's POSITION (off the field, as Godot has it). **`Pawpocalypse.Realm.EveryCard`**: the whole deck at its maximum at once
   for 900 frames (caps and finite state every frame, kills, pulses, bombs, Pounces, puddles; every cost read where it applies).
2. **Game entries**: `UPawRealmSubsystem::ChooseCard` (the pick UI's call: `PcCardDraw` `ChooseCard`, then the field's stats
   rebuilt, as Godot's `choose()`; kittens not re-synced, F14); `StartRealm(Seed)` starts a fresh sim (a second start had
   inherited the travel count that seeds each biome, so a fixed seed did not replay).
3. **Sabotages**: `tools/sabotage_realm.py` + 29 (the game's `ChooseCard` taking nothing or leaving stale stats, the sim kept
   across starts, each effect's consumer in PawCore broken one line at a time, F13 fixed, the mouse-speed cost): all CAUGHT,
   the clean build passing. The first run missed the sim-kept-across-starts one: the replay check was added for it.
4. **The packaged-build gate**: `paw.Bench` (`Dev/PawBench.cpp`, development builds) and `py tools/packaged_bench.py`.
   Packaged Development (UAT by hand, `build/package`, 387 MB; doc 10 §2). **The first packaged run failed**: L_Realm was not
   cooked. `DirectoriesToAlwaysCook` cooked every asset under `/Game/Pawpocalypse` except its maps (only the default map);
   fixed with `+MapsToCook` for L_Title and L_Realm in `Config/DefaultGame.ini`. Then PASS at 1280x800 (RTX 5070): 320 mice +
   20 kittens, game thread 1.40 ms mean / 2.73 ms p99 (budget 4), render thread 2.89 / 8.79 ms, GPU 0.40 / 0.43 ms, 13.6 draw
   calls (budget 50), the realm's steps + draw 0.08 ms; drawn == live every frame; screenshot checked (floor, mice, kittens,
   shadows, rings, HUD).
5. **Docs**: 00, 01 (F19), 03 (`ChooseCard`, the seeded start), 09 (the card rows, L5, sabotage counts, -Full timing), 10 (the
   maps, the interim UAT command), 11 (phase 2 closed with the numbers), CLAUDE.md (`packaged_bench.py`). **KB**: U/19 §1 (maps
   are not cooked by `DirectoriesToAlwaysCook`, `[Verified]` 5.8.3), INDEX symptom row.

### Verified
- `check_all` (everyday): every gate PASS, `L3 Pawpocalypse.Realm` now 9 tests. Card sabotages 29/29 CAUGHT. Packaged gate PASS
  (and seen to FAIL on the uncooked map). The -Full set was not rerun (last 2026-10-31); its `Realm sabotage` gate now takes
  ~55 min (45 sabotages).

### Found
- **F19** the puddle cards (Rainbow Trail, Lingering Rainbow) are offered only with Rainbow Gut (node id `unicorn`), not with
  Rainbow Poop (`unlock_poop`, what makes puddles); their hint says "Buy the Unicorn branch" (`unlock_unicorn`). Ported as Godot
  has it.
- Observed, ported as Godot has it (not a finding): Rainbow Trail's faster pace starts only from the second puddle (the
  PoopTimer restarts on its old 8 s wait before reading the new interval).

### For the designer
- Decide F19 (with F13-F18 and the older ones).
- Commits as before, plus `Source/Pawpocalypse/Realm/PawCardEffectsTests.cpp`, `Source/Pawpocalypse/Dev/PawBench.cpp`,
  the `Realm/PawRealmSubsystem.*` changes, `Config/DefaultGame.ini` (MapsToCook), `tools/packaged_bench.py`,
  `tools/sabotage_realm.py`.
- The spike code (`Dev/Spikes`, `Content/.../Dev/Spikes`) is still there and is cooked into Development packages; delete after
  a commit preserves it (doc 04 §8).

### Open
- `tools/package.ps1` (doc 10 §2: version bump, listing, never-ship check) is phase 8's; packaging is by hand until then.
- The packaged render thread (8.8 ms p99 at an uncapped ~340 fps) is the heaviest number; worth a look in phase 6 with the
  real atlas, at 60 fps capped.

### Next step
*"Start phase 3 per docs/11: the Common UI root layout and the HUD (dock, status row, Pounce clock, XP bar), the shop rows
calling `UPawRealmSubsystem::OnProgressChanged`, and the card pick calling `UPawRealmSubsystem::ChooseCard`. Read docs/05
first; spike S4 (`Dev/Spikes/SpikeUi`) is the proven structure."*

---

## 2026-10-31 (19) - Phase 2 slice 7: the engine side; the game opens on a live field; F18

### Where the project is
- **Phase 2: all 7 slices DONE** (docs/11). L_Realm opens on a running field: the cat follows the cursor, Pounce is the left
  mouse button (or the right trigger), mice, kittens, the boss and its adds, zones, shadows and effect rings are drawn,
  interpolated, at hundreds of fps; a debug HUD reads out the frame, the steps, the field and the wallet. The look is a
  placeholder (discs in each enemy's data tint and size) until phase 6's atlas.
- **Owed before phase 3**: the packaged-build gates (CLAUDE.md: "at the end of every phase"): a packaged frame time and the
  cooked L_Realm drawing. Packaging needs the designer's go-ahead, so it was not done.
- **Nothing committed** in either repo. The Godot repo: only `docs/pawpocalypse-port-findings.md` changed (F18 added).

### What happened
1. **`FPawRealmSim`** (`Source/Pawpocalypse/Realm`, no engine types in its logic): the fixed 60 Hz step over a real frame, the
   8-step guard (drops only time really owed, counted), pause, game speed, the play clock (sim time only), the snapshot the
   presentation interpolates from (a re-used slot never slides), the frame's events, travel (`BeginBiome` + `BeginField`,
   timed), the field's `TravelRequested`, and TierController's `_on_reawakened` (kittens, stats, boss dismissed, Territory's
   deeper start after the death sequence's beat).
2. **`UPawRealmSubsystem`** (a `UWorldSubsystem`, Game/PIE worlds only) over it, and **`UPawGameSubsystem`** (the data from the
   DataTables through `FPawDataAdapter`, loud in development; the `PcProgress`). Deviation from doc 03, recorded there: the
   realm is advanced by the view's own tick, right before it draws (a tickable subsystem ticks after the actors: a frame of
   cursor latency for nothing).
3. **`APawRealmView`** + **`UPawSpriteRenderer`** (`Render/`): spike S2's instanced quads, five layers, pooled and parked, one
   batch a layer; orthographic camera with S1's exposure settings; input sampled raw (phase 5 moves it to Enhanced Input);
   effects from events with a budget; bounds from the window (no dock until phase 3). Four generated materials
   (`tools/create_field_materials.py`); `tools/create_maps.py` places one configured view in L_Realm (only when missing).
   First screenshots found two presentation bugs, fixed: shadows hidden under their bodies; additive effects invisible on
   the light floor (now translucent: KB 2D/02).
4. **The debug HUD** (`APawDebugHud`, development builds, `paw.DebugHud`) and **the cheats**: the rules in PawCore (`PcCheats`,
   6 L1 tests, 9 sabotages), the overlay's buttons as console commands (`paw.Cheat.Catnip|Stardust|MaxUpgrades|Unicorn|
   Reawaken|Reset|AdvanceStage|UnlockTiers|UnlockStages|KillAll|Speed`, `paw.Travel`, `paw.Summon`, `paw.Spawn`). The F1
   panel with its password is phase 3's UI.
5. **Tests**: `Pawpocalypse.Realm.*`, 7 automation tests; `tools/sabotage_realm.py`, 16 game-module sabotages; `check_all` gains
   `L3 Pawpocalypse.Realm` (everyday) and `Realm sabotage` (-Full).
6. **The engine guard** (`tools/unreal_procs.py`, used by `run_automation.py`, `import_data.py`, `sabotage_realm.py` and
   `check_all`): another project's soak (`C:\Games\Cabbage`, run by another session every few minutes) kept making the realm
   sabotage run report MISSED for tests that never ran. The guard now ignores a process whose command line names ANOTHER
   project's `.uproject`; one of this project, or naming no project (it may be the designer's editor), still blocks.
   Self-tested (-Full gate `Unreal guard self-test`). That other process was never touched.
7. **Docs audit**: 00 (status, the file table: Realm/Render/Dev, Materials, the new tools), 03 (status, the subsystem row and
   why, the data load at startup, the cheat and HUD rows, the fixed-step pseudo-code), 04 §5.1 (the renderer as built), 07
   (placeholder material names), 09 (the L3 naming, the Realm row, counts, -Full timings, the guard; the card-effects engine
   test marked NOT built), 11 (slice 7 done, exit status, what is owed), 12 (four new traps), CLAUDE.md (the new tools),
   appendix B regenerated (the cheat row; one stale row from an earlier session refreshed); 05 (the interim raw Pounce read),
   06 (the mirror's adapter half, built since 2026-10-29).
8. **Knowledge base**: U/18 §2.1 (a standalone game instance in a test; frame-rate independence as a test), 2D/02 §2 (additive
   effects vanish over a light floor), indexes.

### Verified
- `Pawpocalypse.Realm.*` 7/7: frame-rate independence bit for bit at 30/50/60/144/240 fps and jittered; travel 0.006 ms worst
  (field) and 0.03 ms (with the redraw) against 16 ms; 320 mice + 20 kittens advance + draw 0.06 ms against 4 ms.
- Sabotages: realm all 16 CAUGHT on the final code, the clean build passing (`tools/sabotage_realm.py`, ~17 min); PawCore's 9 new (the cheats) all CAUGHT (272 in all). One realm miss on the first run was
  the test's fault (it faked a boss in a biome without one and crashed): rewritten to summon a real boss, then caught.
- L1: 121 cases, 4,392,529 assertions. Screenshots of the running game (1280x720 window): floor, shadows, bodies, kittens,
  rings, HUD all drawn.
- `check_all` (everyday): every gate PASS (L0, L1, mirror, fuzz 3k, L3 DataTable parity, L3 `Pawpocalypse.Data`, L3 `Pawpocalypse.Realm`, the Godot repo untouched). The -Full set was not rerun after slice 7 (last: 2026-10-31 01:45-04:11, all green); its new gates (`Realm sabotage`, `Unreal guard self-test`) were each run alone and pass

### Found
- **F18** the cheat "Unlock all stages" opens only the biomes behind no boss: it never sets this cycle's passage, the gate
  `is_biome_unlocked()` gained after it. Ported as Godot has it (dev tool only).

### For the designer
- Say go for the phase 2 packaged-build gates (a packaged Development build, its frame time at 320 mice, the cooked L_Realm).
- Decide F18 (with F13-F17 and the older ones).
- Commits as before, plus `Source/Pawpocalypse/{Core,Dev,Realm,Render}`, `Source/PawCore/*/PcCheats*`,
  `Content/Pawpocalypse/Materials/`, `Content/Pawpocalypse/Maps/L_Realm.umap`, the new tools.

### Open
- **Owed for phase 2's exit**: the engine-world card-effects test (`Paw.Realm.CardEffects`, doc 09 §3: every card effect
  applied in a live realm, its consumer observed); PawCore's rule tests pin the effects, the engine-world proof is not built.
- Placeholders by design: the look (phase 6), raw input reads (phase 5), no dock in the bounds (phase 3), the cheat panel
  (phase 3), auto-travel/auto-pick settings at Godot's defaults until phase 4's settings.
- The spike code (`Dev/Spikes`, `Content/.../Dev/Spikes`) is still there (doc 04 §8 says delete after); left for the designer.

### Next step
*"Close phase 2: build `Pawpocalypse.Realm.CardEffects` (doc 09 §3: every card effect applied in a live realm through the real
entry points, its consumer observed, each negative-tested in `tools/sabotage_realm.py`); then, once the designer says go, the
packaged-build gates. Then phase 3 per docs/11: the Common UI root layout and the HUD (dock, status row, Pounce clock), the
shop rows calling `UPawRealmSubsystem::OnProgressChanged`."*

## 2026-10-31 (18) - The night: phase 2 slices 2-6, `PcField` complete as a simulation; F13-F17

### Where the project is
- **Phase 2: slices 1-6 of 7 DONE** (docs/11). `PcField` simulates the whole realm: mice, kittens, the aura, Snare and zones,
  Pounce, the paw's and the kill's extras, the Prism, the boss and its adds. What is left of phase 2 is slice 7, the engine
  side (`UPawRealmSubsystem`, `APawRealmView`, renderer, debug HUD, cheat overlay); it needs the editor CLOSED to build.
- **Every gate green** (see Verified). **Nothing committed** in either repo. The Godot repo: only `docs/pawpocalypse-port-findings.md`
  changed (F13-F17 added; the file the port may write), `card_manager.gd` still carries the designer's uncommitted F1 fix.
- The designer's Unreal editor was open until about 01:30 (never touched); once it had closed, `DT_CodeConstants` was
  re-imported (`py tools/import_data.py`: three new rows), the editor target rebuilt with UBT, and the engine gates ran.

### What happened (the night worklist: `docs/WORKLIST-NIGHT-2026-10-30.md`)
1. **Slice 2, kittens.** New Godot fixture `kittens_fixture.gd` (a real `Kitten` and up to 24 real `Enemy` nodes: getters, pool
   size, orbit slot, `_pick_target`, `_is_overcrowded`, one whole `hunt()`), 3,000 rows. `PcFieldRules`: `KittenPoolBaseSize`,
   `KittenOrbitSlot`, `KittensSeekClusters`, `KittenIllusionPull`, `PickKittenTarget`, `KittenOvercrowded`. `PcField`: the pool,
   `SyncFieldKittens`, `HuntKitten`, the tally, roam, approach, contact damage, Gilded Kitten, Lone Hunter, Kitten Drag, Kitten
   From Signature, Kitten Kill Explodes, Grand Finale. Godot's own `check_kitten_spread` ported as a rule test. No spatial grid
   (measured: doc 04 §6).
2. **Slice 3, the aura.** Fixture `aura_fixture.gd` (getters, the aura cards, aura/poop/player DPS, a real Enemy's `apply_slow`).
   The aura, Snare/Deadweight, Gridlock, the slow's life (replaces slice 1's `Snared` flag with `SlowMult`/`SlowFresh`),
   Traps, the zone primitive, trail, echo, Edge Burn, the pulse, Rainbow Poop, the stillness clocks (`PcProgress`
   `PlayerStillSeconds`, `LunchSeconds`).
3. **Slice 4, Pounce.** Fixture `pounce_fixture.gd`. The cast, the press edge, Reflex, the bonus, Chain, Mimic, craters, the
   Cone of Shame (the cat rooted, stillness frozen), Insurance, Crash Landing, Bloodlust, Momentum.
4. **Slice 5, the extras.** Fixture `extras_fixture.gd`. Anvil Paw, Rhythm, Inscrutable, Riddle, Hammerfall, Burglar's Cut,
   Overflow, the Catnip Bomb, Long Lunch, Levy, Pied Piper, Sixth Sense, Sleight of Hand, the Prism; and the despawn recovery
   (Scavenge + Pickpocket), which slice 1 had missed.
5. **Slice 6, the boss.** Fixture `boss_fixture.gd` (a real `Boss` node, op logs). Summon, phases, shield cycle, relocation,
   drift, the fight clock and timeout, adds, the defeat and the travel after it, Auto-Bait, Auto-Travel, everything that hits
   the boss. Three inline Godot literals joined the code constants (`boss_relocate_distance_px`, `boss_wobble_x_px`,
   `boss_wobble_y_px`: map 574 rows, 66 gameplay constants). docs/11's exit criterion "timeout sends back a biome and turns
   auto-travel off" was wrong against Godot (it only withdraws the boss): corrected.
6. **The soak and the stress step** (`PcFieldSoakTests.cpp`, long suite): a scripted cat through the first three biomes, 3 seeds
   x 3 game hours, every step's invariants held; the full field (320 mice, 21 kittens, aura, zones, Prism) steps in 0.093 ms.
7. **Tooling**: `build_pawcore_tests.ps1` compiles with `/MP` and has `-TestsOnly`; `sabotage_pawcore.py` uses it: a sabotage
   went from ~4 min to ~12 s, so all of them run in under an hour.
8. **Knowledge base**: U/18 §8 (mutation tests: /WX non-compiling mutants, equivalent mutants, unique targets, copying a tree
   mid-run, /MP), G/03 §6 (fixture runs inherit autoload state through the sandbox's saves), G/07 (a pooled node's position
   read after `deactivate()`), 2D/01 §5 (a grid is not needed at a few hundred entities, measured); indexes updated.

### Verified
- `tools\check_all.ps1` (everyday): every gate PASS, the L3 engine gates included. L1: 115 test cases, 4,392,381 assertions.
- The long field tests (the five new 3,000-row fixtures + field rules, the soak, the step): 494,602 assertions, PASS.
- Sabotages: 263, each slice's batch seen CAUGHT tonight (161 new: kittens 34, aura 34, Pounce 26, extras 36, boss 31), every
  miss explained and fixed (non-compiling mutants, equivalent mutants, a test hidden by whole-Catnip rounding, a doctest filter
  with a comma, positions exactly at distance 0). The runner now refuses a comma in a filter (negative-tested).
- The nightly `-Full`: every gate PASS (17 gates, 2.6 h, 01:45-04:11): the 263 sabotages all CAUGHT in 46 min, the Godot fixtures reproduce, the long suite (20 min), the 200k mirror, the 30k fuzz, the pawsim parity sweep, the UBT build, the Godot repo untouched. Afterwards no sabotage is left in the source (`build/check_source_clean.py`).

### Found (doc 01 §5; the designer's copy in the Godot repo)
- **F13** Kitten Kill Explodes (Giblets) never hurts anything: the blast is placed at the dead mouse's parking spot.
- **F14** a card that costs kittens takes them only at the next re-count.
- **F15** `KITTEN_DRAG` and `KITTEN_FROM_SIGNATURE` have no card.
- **F16** Overkill from the aura, Pounce and Hammerfall spills from the parking spot (to the top-left-most mouse).
- **F17** Riddle executes on the swing that armed it, against its comment.
All ported as Godot has them, each behind a one-line switch or a pinned test, so a decision is a one-line change + a fixture
regeneration.

### For the designer
- Decide F13, F14, F16, F17 (players can see F13 and F16), F15; still open from before: F2, F3, F6, F7, F11, Q1-Q9.
- Commits (paths as before, plus the new files under `Source/PawCore/`, `tools/godot_fixtures/`, `Data/`, docs, and the
  re-imported `Content/Pawpocalypse/Data/DT_CodeConstants.uasset`).

### Open
- Slice 7 (engine side) needs the editor closed to build.
- Known approximations, by design: a mouse spawned in a step first moves on the next; PcRng instead of Godot's global RNG; the
  boss and adds move after the mice (child order); `_boss_retreating` is never set in Godot and is not ported.

### Next step
*"Phase 2 slice 7 per docs/11 and docs/03 ('What the game must do for PcField'): UPawRealmSubsystem (fixed 60 Hz step,
interpolation, the events drain, the contract), APawRealmView with the spike-S2 renderer, the debug HUD and cheat overlay,
travel < 16 ms, the frame-rate independence test (the editor closed to build)."*

## 2026-10-30 (17) - The nightly `-Full` run: every gate green; phase 1 exits

### Where the project is
- **Phase 1 done**: its last open exit item, the nightly `tools\check_all.ps1 -Full`, ran green (exit 0, ~3.6 h). Phase 2 slice 1
  of 7 done; slice 2 (kittens) next.
- **Nothing committed** in either repo; no code changed this session. Godot repo untouched (the run's before/after check PASS).

### What happened
1. `check_all.ps1 -Full`, every gate PASS: L0 static, L1 PawCore tests, mirror, validator fuzz 3k, L3 DataTable parity,
   L3 Pawpocalypse.Data, L0 self-test, mirror self-test, **PawCore sabotage (all 102 caught, clean build passes; 4,453 s)**,
   Godot fixtures reproduce (123 s), **L1 long suite (21 cases, 4,611,069 assertions; 1,674 s)**, mirror 200k states,
   validator fuzz 30k (287 s), **pawsim parity sweep (90/90 identical; 6,893 s)**, UBT editor build, Godot repo untouched.
2. `DataTable parity self-test` was SKIPPED inside the run (an Unreal editor was opened partway through). Rerun alone afterwards
   with no editor running: `py tools\import_data.py --self-test` -> `CAUGHT a changed EnemyData cell`, exit 0
   (`build\check_all\DataTable_parity_self-test_rerun.log`). So every gate has now passed on the current tree.
3. Docs brought up to date: 00 (status, Phase 1 done), 09 §7 (the nightly's measured times; an editor makes engine gates
   SKIP), 11 (phase 1 DONE), CLAUDE.md and `check_all.ps1`'s header (~3.5 h, was ~3 h).

### For the designer
- Nothing new to decide. Still open from before: F2, F3, F6, F7, F11, Q1-Q9; commits (paths as (16)).
- Keep the editor closed during a `-Full` run, or its engine self-test is skipped.

### Open
- Slice 1 known approximations, by design (see (16)).

### Next step
*"Phase 2 slice 2 per docs/11: kittens (pool sized to ownership, seek/roam/orbit/rebalance, contact damage, the spatial grid),
following the slice-1 pattern (pure parts against a Godot fixture, rule tests, sabotages)."*

---

## 2026-10-29 (16) - Phase 2 started: the field's pure rules against Godot, `PcField` slice 1

### Where the project is
- **Phase 2 under way**: slice 1 of 7 DONE and verified (docs/11 phase 2 lists the slices and how each is proved). Phase 1's exit
  still waits on a nightly `-Full` (not run: the designer needed the PC).
- **Gates, measured today, all PASS**: `tools\check_all.ps1` everyday set (L0; L1 51 test cases, 4,255,585 assertions, was 38;
  mirror; validator fuzz; L3 DataTable parity 12 tables incl. `DT_CodeConstants` re-imported at 63 rows; `Pawpocalypse.Data`
  4,213 cells equal). Long field-rules fixture 3,000 rows, 88,256 assertions PASS. UBT editor build succeeded.
- **All 20 new field sabotages CAUGHT** (+ clean PASS). First run: 15 caught; the other 5 were the test's or the script's fault,
  each fixed and rerun CAUGHT:
  - 3 NOBUILD (a sabotage left a parameter unused; warnings are errors): texts rewritten to keep them used.
  - `field_shred_uncapped` MISSED: `PawStrikeDamage` clamps the stacks itself, so damage cannot show an uncapped streak; the
    paw test now asserts `ShredStacks` stops at the ceiling.
  - `field_execute_through_shield` MISSED: without Shield Pierce the shield blocks the blow either way; the Execute test now
    asserts a shielded mouse is never even SWUNG for its whole HP.
- **Nothing committed** in either repo. The Godot repo is unchanged (every fixture ran in the scratch copy).

### What happened
1. **How phase 2 is proved** (docs/11): Godot's realm is frame-driven and rolls the global RNG, so it cannot be replayed row by
   row. Pure parts -> `PcFieldRules` against a Godot fixture; the stateful field -> rule tests + sabotages. Seven slices listed.
2. **Code constants: `@export` defaults joined.** Enemy and kitten speeds (`speed_px_per_sec`, `wander_turn_rate`,
   `flee_centre_bias`, `flier_speed_multiplier`, kitten speed, orbit radius, roam ratio) are `@export var` defaults, not
   `const`, so appendix C and the map had never seen them. `list_godot_code_constants.py` now scans numeric exports (appendix C:
   569); the map has 571 rows (new destination `data` for data classes' field defaults, 89; `save` 14); 63 gameplay constants
   (7 new fields in `PcCodeConstants`). `export_code_constants.py` refuses an exported default that a `.tscn` overrides (none
   does today). A trailing `## doc` comment no longer leaves a `#` in the reason.
3. **`PcFieldRules`** (`Public/PcFieldRules.h`): `SpawnRateMultiplier`, `SpawnRatePerSec`, `SpawnCadence` (batch/interval),
   `ThresholdDamageMultiplier`, `SnaredDamageMultiplier`, `PlayWithFoodMultiplier`, `FunnelBounces`, `EnemyDrift`,
   `PrismKillMultiplier`, `TotalSpawnWeight`, `PickEnemy`, `PlayfieldRect`, `EnemySpawnStats`, `ApplyBlow`. New Godot fixture
   `tools/godot_fixtures/field_rules_fixture.gd/.tscn` (`--only field_rules`): random states + a REAL pooled `Enemy` node's
   `activate()` and up to 4 `take_damage()` blows. Test `Tests/PcFieldRulesTests.cpp` (pow-derived values within 1e-13, blows bit
   for bit starting from Godot's own spawn stats, the playfield at float32 resolution because Godot's `Rect2` is float32;
   19 coverage paths asserted).
4. **`PcField` slice 1** (`Public/PcField.h`, `Private/PcField.cpp`): struct-of-arrays pool, Godot's frame order (strikes, flee
   targets, threshold published for the NEXT step with this step's dead still counted, Compound Interest and Infestation
   ticked, sweep; then the spawn timer; then every mouse moves; sweep), PcRng per (seed, step, serial, salt) with new salts
   (`SpawnEdge/Along/Turn`, `Funnel`, `Wander`), `SpawnEnemyAt`, `KillAllEnemies`, `PcFieldEvent` queue. Every later-slice rule
   is a named TODO in the header and the .cpp. `Tests/PcFieldTests.cpp`: 12 rule tests.
5. Sabotages +20 (`field_*`, 102 total). Docs 03, 06 §5, 09, 11 (phase 2 plan), 00; `make_godot_fixtures.py` docstring.
   Knowledge base `Godot/03` §6 + `INDEX.md`: Vector2/Rect2 are float32; fixture rows inherit autoload state unless set; a real
   scene node as the oracle.

### For the designer
- Nothing new to decide. Still open from before: F2, F3, F6, F7, F11, Q1-Q9; commits (paths as (15), plus the new files under
  `Source/PawCore/`, `tools/godot_fixtures/`, `Data/`).

### Open
- `tools\check_all.ps1 -Full` (~3 h) when the PC is free (overdue since (8)).
- Slice 1 known approximations, by design: a mouse spawned in a step first moves on the next; the field rolls PcRng, not
  Godot's global RNG.

### Next step
*"Phase 2 slice 2 per docs/11: kittens (pool sized to ownership, seek/roam/orbit/rebalance, contact damage, the spatial grid),
following the slice-1 pattern (pure parts against a Godot fixture, rule tests, sabotages). If the designer agrees first, run
`tools\check_all.ps1 -Full`."*

---

## 2026-10-29 (15) - Code constants from CSV; the DataTables imported and proven equal to the CSVs

### Where the project is
- **Phase 1's work list is done**: `Data/CodeConstants.csv`, `tools/import_data.py` + the 12 DataTables + parity, and the game's
  third of the mirror (`FPawDataAdapter`, `Pawpocalypse.Data.Parity`). The phase exit still needs a nightly `-Full` run
  (not run since (8); the designer asked for no long run today: the PC was needed).
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS, incl. the two new engine gates (L1 38 test cases,
  4,222,519 assertions; mirror 58,651 bit-identical; L3 DataTable parity 12 tables; `Pawpocalypse.Data` 2 tests, 4,185 cells
  equal). L0 `--self-test` 8 plants CAUGHT. `import_data.py --self-test` CAUGHT. 12 sabotages (3 new, 9 retargeted) CAUGHT +
  clean PASS. UBT editor build succeeded.
- **Nothing committed** in either repo. The Godot repo is unchanged by this session.

### What happened
1. **Code constants** (doc 06 §5). `Data/CodeConstantsMap.csv` is the reviewed destination of all 456 appendix C constants plus
   `BOSS_REALMS` and Second Helping's literal biome index: 56 `code_constants:<id>` (GameState, CardManager, and the field's
   rules for phase 2), 307 presentation, 66 palette, 21 input, 3 save, 2 audio, 3 dev; borderline calls carry a note.
   `tools/export_code_constants.py` (run by `export_godot_data.py`) writes `Data/CodeConstants.csv` with the values and reasons
   read from the Godot code, and fails on any unmapped, vanished, doubled or unparseable constant, so L0's data-export gate
   now covers it (self-tests: an edited value, a dropped map row, both CAUGHT). `list_godot_code_constants.py` exposes `scan()`
   (appendix C regenerated byte-identical).
2. **PawCore reads it**: `PcConstants.h` is now `PcCodeConstants` (`PcGameData::Code`), loader `ReadCodeConstants` (every field
   exactly one row, no unknown ids, integers integral; 6 malformed-file cases tested; 3 loader sabotages CAUGHT). No more
   constexpr copies. API change: functions that read a constant take the data: `EvolutionStage(D, P)`, `VisualStage(D, P)`,
   `CatchesForLevel(D, L)`, `TotalCatchesForLevel(D, L)`, `CurrentWave(D, Tier, K)`, `IsTierUnlocked(D, P, S)`,
   `BossesClearedForStage(D, P, S)`, `IsBiomeUnlocked(D, P, T)`, `MarkBossDefeated(D, P, Id)`, `TickInfestation(D, P, Dt)`.
   Validator +2 rules (57): `code.stage_ladder`, `code.range`, each negative-tested. pawsim `--data csv` reads
   `max_golden_chance` and the Catches curve from the CSV (`--catches-*` still sweep); the mirror is unchanged (bit-identical).
3. **`sabotage_pawcore.py`** refuses to run (exit 2, nothing built) when any target is not in its file exactly once (the open
   item from (13)); on its first run it found 3 targets this session's signature changes had broken. 82 sabotages.
4. **DataTables** (doc 06 §4, §9). Row structs are GENERATED (`tools/gen_paw_rows.py` -> `Source/Pawpocalypse/Data/PawDataRows.h`,
   L0-checked) with every column an `FString` = the cell's text (the importer stores FString cells verbatim, read in the 5.8.3
   source). `FPawDataAdapter` rebuilds the CSV text and calls the new `Pc::BuildGameData` (the one parser; `LoadGameData` now
   reads files and calls it too; `Pc::GameDataTableNames()`, `Pc::KnownGodotDataIssues()` moved out of the tests).
   Names: `DT_<CsvName>` / `FPaw<Name>Row` (docs said `DT_Tiers`/`FPawSkillNodeRow`; mechanical names now, docs updated).
   `tools/import_data.py` (headless; refuses while any Unreal editor runs; any `LogDataTable` warning fails) imported all 12 into
   `Content/Pawpocalypse/Data/` and checks parity on the saved assets. `tools/run_automation.py` runs the engine tests headless.
   `check_all` gained `L3 DataTable parity` and `L3 Pawpocalypse.Data` (SKIP, not FAIL, while an editor is open; the summary
   says what was skipped); `-Full` gained the parity self-test. Both engine gates were seen to FAIL before the first import.
5. A test assumption that proved wrong: `RemoveRow` + `AddRow` does not move a row to the end (the row map reuses the slot).
   Recorded in the knowledge base (`Unreal/05` §2.1, with the text-row pattern and the Python import recipe; `INDEX.md`).
6. Docs 00, 03, 06, 09, 11, `CLAUDE.md` (commands).

### For the designer
- Commit when happy (explicit paths, now including `Data/` and `Content/Pawpocalypse/Data/`): `CLAUDE.md Config/
  Pawpocalypse.uproject Source/ Content/ Data/ docs/ tools/`. The Godot repo still has (9)'s fix and (14)'s findings file.
- Another project's headless editor runs (Cabbage) were seen during this session; the engine gates wait or SKIP while any
  Unreal editor runs, by design.

### Open
- `tools\check_all.ps1 -Full` (~3 h) when the PC is free: overdue since (8); it now includes the new self-tests.
- A cooked-build test that loads every table and finds every row by id (phase 8 packaged gates).
- Godot decisions F2, F3, F6, F7, F11 and Q1-Q9 unchanged.

### Next step
*"Run `tools\check_all.ps1 -Full` (the designer must agree: ~3 h). If green, phase 1's exit is met: start phase 2 per docs/11
(`PcField` first, reading the field rows of `PcCodeConstants`)."*

---

## 2026-10-29 (14) - The Godot findings, written up for the designer in the Godot repo

### Where the project is
- Unchanged from (13): phase 1 has every Godot manager's rules in PawCore; left: `Data/CodeConstants.csv`,
  `tools/import_data.py` + DataTable parity, the game-side mirror. No code changed this session, so no gates were re-run.
- **Nothing committed** in either repo. The Godot repo now has two uncommitted changes: the session (9) Second Opinion fix
  (`scripts/autoload/card_manager.gd`) and the new `docs/pawpocalypse-port-findings.md`.

### What happened
1. **Designer's request: the Godot bugs the port found, in a document in the Godot folder.** New
   `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`: F1-F12 with the same numbers as doc 01 §5, grouped as
   *waiting on a decision* (F11, F6, F7, F2, F3), *low priority* (F8, F9, F10, F5, F4), *not a bug here* (F12) and *fixed* (F1,
   uncommitted), each with file, function and line (as of today) and the question to answer.
2. Docs brought in step: doc 01 §5 (points at the Godot copy; new findings go in both), doc 00 (where findings live), doc 06
   §8 (the Godot fix loop), doc 09 §7 recipe step 6 (record findings in both), `CLAUDE.md` (the findings file is the one Godot
   file the port writes without a decision). The other docs were checked against (13) and are current.

### For the designer
- Read `C:\Games\pawpocalypse\docs\pawpocalypse-port-findings.md`; the first table is the five decisions. F11 is the one players
  can see. Commit it with the `card_manager.gd` fix when you are happy with both.

### Open
- As (13): phase 1 remaining; `sabotage_pawcore.py` exact-once target check; `tools\check_all.ps1 -Full` overdue.

### Next step
As (13): *"Continue phase 1 per docs/11: `Data/CodeConstants.csv` (the gameplay rows of appendix C) and move `PcConstants.h`
onto it, then `tools/import_data.py` + DataTable parity; `tools\check_all.ps1`. Before that, if the designer agrees, run
`tools\check_all.ps1 -Full`."*

---

## 2026-10-29 (13) - The card draw ported, card for card against Godot's own random stream

### Where the project is
- **Phase 1: every Godot manager's rules are now in PawCore.** Left in phase 1: `Data/CodeConstants.csv`,
  `tools/import_data.py` + DataTable parity, the game-side third of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L1 37 test cases, 4,222,474 assertions). Long
  draw fixture (3,000 rows, 413,462 assertions) PASS. 16 new sabotages CAUGHT; one more retargeted (below). UBT editor build
  succeeded. Last full nightly: (8), before `PcTree`, `PcJobs`, `PcProgress`, `PcAchievements`, the draw.
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcCardDraw`** (`Public/PcCardDraw.h`, `Private/PcCardDraw.cpp`), CardManager's stateful half: `DrawHand` (weighted,
   without replacement, Fixation's owned pool with its fallback), `OfferHand` (Blueprint's promised cards first, deduped
   filler, Fixation spent on the deal, auto-pick, Prototype's mark, the promise drawn twice as Godot does, F10),
   `GainCatches` (the level-up: Second Opinion's rerolls, Catnip Interest before the hand, the deal), `PrototypeBonus`,
   `RerollHand`, `ChooseCard` (the deck cards act here), `BanishCard`, `BanishFromHand`; `CardForesight`,
   `CardPrototypeLevels`, `HasAutoCard`.
   - **Randomness is injected**: `PcCardRolls` (`Unit()` = randf, `Below(n)` = randi % n). The game's is
     `PcRngCardRolls` (PcRng per seed and deal number); the test's is a copy of Godot's PCG32 global RNG.
   - `PcProgress` (the struct) gained `Rerolls`, `Banishes`, `FixationHands`, `Hand`, `PromisedCards`, `PrototypeCard`.
     `ResetRunCards` now takes the data: rerolls go back to Overtime's count, and the hand/promise/mark/banishes/Fixation
     are cleared. `PcCards.h` exposes `CardMaxLevel`, `CardValueAt`, `CardCostAt`, `CardLevel`.
   - The loader now sorts the DECK by `_godot_file` too (`SortByGodotFile`, shared with the achievement roster): the draw
     walks the pool in deck order. Today file order = id order, so no sabotage can show it; the import must keep it.
2. **Godot fixture** `tools/godot_fixtures/draw_fixture.gd/.tscn` (`--only draw`): random run states, 8 ops each. Every op
   that can draw calls `seed(s)` with `s` written into the op. Godot's global RNG is PCG32 and `randf()` is ONE output
   rounded to a float times 2^-32 (pinned by the `calib` column, row 0). Two things had to be kept off the stream:
   **AudioManager.play() rolls the same global RNG for pitch**, behind a wall-clock rate limit (the first replay differed
   on row 1 because of it: F12), so the fixture empties its variants; and the pick screen's UI crashed headless after
   thousands of deals (children freed on a frame that never comes), so it is freed at start. About one row in seven has a
   deck nearly run dry. `Tests/Data/draw_godot.tsv` (1,000 rows, 3.2 MB), `build/draw_godot_full.tsv`.
3. **`Tests/PcCardDrawTests.cpp`**: replays card for card (hand, promise, mark, rerolls, banishes, Fixation, extra
   choices, XP, Catnip, owned levels, banished set, Grudge Ledger after every op); coverage asserted (1,000 rows: 1,400+
   deals offered, ~250 auto-picked, ~700 Prototype marks, ~660 promises, ~160 empty deals, 23 banishes that emptied the
   deck, every deck card taken, Prototype's bonus applied ~40 times...). Rules test: the game's rolls are deterministic
   per (seed, deal); first picks follow the weights within 5 sigma over 200,000 deals; the hand is the contract; banish
   does not consume the level; deck cards and Prototype on taking; XP while a hand is up; Fixation's fallback; Overtime
   survives the reset.
4. **Sabotages** (now 79): `draw_with_replacement`, `_fixation_ignored`, `_fixation_no_fallback`, `_promise_not_dealt`,
   `_no_dedupe`, `_fixation_not_spent`, `_single_promise`, `_auto_pick_without_setting`, `_prototype_any_card`,
   `_choose_uncapped`, `_extra_choice_adds`, `_reroll_free`, `_banish_free`, `_no_second_opinion`,
   `_reset_drops_overtime`: all CAUGHT (`_reset_drops_overtime` first failed to compile, unused parameter; rewritten).
   Retargeted after the code moved: `achievements_roster_csv_order` (now `SortByGodotFile`), and
   `progress_reawaken_keeps_cards` (`ResetRunCards(D, P)`); both CAUGHT. A check that every sabotage's target text occurs
   exactly once found the second one; worth adding to `sabotage_pawcore.py --list`.
5. **Findings** (doc 01 §5): **F10** a deal promises the next hand twice (the first only spends rolls); **F11** reroll and
   banish keep the old hand's Prototype mark and Blueprint promise; **F12** Godot's hands cannot be replayed (global RNG
   shared with sound pitch); the port differs by design (`PcRng`).
6. Docs 00, 01 (F10-F12), 02 §7 (the deal), 03, 09 §7, 11. Knowledge base `Godot/03` §6 + `INDEX.md`: replaying the global
   RNG (PCG32 seeding, randf), audio rolling the same stream, UI nodes piling up headless.

### For the designer
- **F11** is the one with a player-visible effect: should a reroll re-mark Prototype's card and redraw Blueprint's promise?
  F6, F7, F8, F9 still open. Each is a Godot change first.
- Still from (9): commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: `Data/CodeConstants.csv` (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable parity
  (L3; must keep deck and roster order); the game-side mirror.
- `sabotage_pawcore.py`: refuse to run when a target is not found exactly once (it reported "not caught" instead).
- Run `tools\check_all.ps1 -Full` (~3 h): overdue, last run (8).

### Next step
*"Continue phase 1 per docs/11: `Data/CodeConstants.csv` (the gameplay rows of appendix C) and move `PcConstants.h` onto it,
then `tools/import_data.py` + DataTable parity; `tools\check_all.ps1`. Before that, if the designer agrees, run
`tools\check_all.ps1 -Full`."*

---

## 2026-10-29 (12) - `PcAchievements` ported: earn, claim, load

### Where the project is
- **Phase 1 under way.** PawCore now also holds **`PcAchievements`**. Left in phase 1: the card draw (and the
  rerolls/banishes state it owns), `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the game-side third
  of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L0; L1 35 test cases, 4,084,444 assertions; mirror;
  validator fuzz 3k; Godot repo untouched). Long achievements fixture (3,000 rows, 1,031,268 assertions) PASS. The 14 new
  sabotages all CAUGHT, clean build PASS (one first failed to COMPILE, an unused parameter under warnings-as-errors, and was
  rewritten; then CAUGHT). UBT editor build succeeded. Last full nightly: (8), before `PcTree`, `PcJobs`, `PcProgress`,
  `PcAchievements`.
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcAchievements`** (`Public/PcAchievements.h`, `Private/PcAchievements.cpp`) from `achievement_manager.gd`:
   `FindAchievement`, `AchievementProgress` (the seven metrics), `IsAchievementEarned`, `EarnedAchievementCount`,
   `TotalStardustOnOffer`, `EvaluateAchievements` (the full re-scan; returns the ids earned in roster order),
   `AchievementReward`, `IsAchievementClaimable` (earned, unclaimed, reward > 0), `IsAchievementClaimed`,
   `UnclaimedAchievementCount`, `UnclaimedStardust`, `ClaimAchievement` (marked before paying, pays through
   `GrantAchievementStardust`), `ClaimAllAchievements`, `ApplyAchievementSave` (replace both sets, re-scan).
   - `PcProgress` (the struct) gained `AchievementsEarned`, `AchievementsClaimed`.
   - **No signals**: the game module calls `EvaluateAchievements` after every change that can move a counter (Godot's six
     signals). `NewGame` stays a port of `debug_reset_progress()`; as in Godot the title screen then empties the
     achievements with `ApplyAchievementSave({}, {})`.
   - **Roster order**: Godot lists the folder and sorts the file names (`achievement_01_kills_1` ...), the CSV is sorted by
     id. `LoadGameData` now keeps `_godot_file` for this table and sorts the roster by it (`PcData.cpp`).
2. **Godot fixture** `tools/godot_fixtures/achievements_fixture.gd/.tscn` (`--only achievements`): an op log of 10 ops per
   row (load a save incl. a retired id and claims without earns, a counter moves + re-scan, claim, claim all, New Game);
   each op's result (earned ids in emit order through `achievement_earned`, or Stardust paid) and a digest (earned/claimed
   as bits in roster order, set sizes, both Stardust totals); then every achievement's progress, reward and flags. 40 % of
   rows overwrite thresholds and rewards in memory (0, negative, fractional). `Tests/Data/achievements_godot.tsv` (1,000
   rows, 5.1 MB; the first form with full id lists was 8.5 MB), `build/achievements_godot_full.tsv`.
3. **`Tests/PcAchievementsTests.cpp`**: replays the ops; coverage asserted (1,000 rows: 1,084 claims paid / 2,166 refused,
   958 claim-alls paid / 504 nothing / 95 fractional, 872 re-scans earned, 218 earning several at once, 1,730 values exactly
   on the threshold, 80 earned achievements paying nothing, 427 claimed-not-earned, 368 edited rosters). Long on 3,000;
   rules from the comments (two events, a double claim pays nothing, nothing un-earns, zero reward = not claimable,
   claim all, ledger not passage, load and New Game, roster order).
4. **Sabotages** (now 64): `achievements_strict_threshold`, `_claim_not_marked`, `_zero_reward_claimable`,
   `_claimed_still_claimable`, `_bosses_count_passage`, `_stage_is_visual`, `_catnip_this_cycle`, `_load_merges`,
   `_load_no_rescan`, `_claim_all_stops`, `_grant_spendable_only`, `_badge_counts_earned`, `_unclaimed_stardust_earned`,
   `_roster_csv_order`: all CAUGHT.
5. **Finding** (doc 01 §5): **F9** (low) the earned count includes ids no longer in the roster (a save holding a removed
   achievement shows "42 / 41"). Ported as Godot.
6. Docs 00, 01 (F9), 02 §11 (the rules), 03 (module table, status), 09 §7, 11; appendix B regenerated (no change). Knowledge
   base `Godot/03` §6: replaying signal handlers that fire mid-sequence, Resource folder load order, bit-string digests.

### For the designer
- **F9** joins F6, F7, F8 (doc 01 §5): each is a Godot change first if you want it. F2 (two unreachable achievements) is
  still open and touches this port only through the data.
- Still from (9): commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: the card DRAW (weighted through `PcRng`, rerolls, banishes, the hand); `Data/CodeConstants.csv`
  (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable parity (L3); the game-side mirror. The DataTable import
  must keep the achievement roster order (`_godot_file`).
- Run `tools\check_all.ps1 -Full` (~3 h): overdue, last run (8).

### Next step
*"Continue phase 1 per docs/11: port the card DRAW (`card_manager.gd`: the weighted hand through `PcRng`, rerolls, banishes,
extra choices) following the recipe in docs/09 §7, then `tools\check_all.ps1`."*

---

## 2026-10-29 (11) - `PcProgress` ported: every rule that changes progress

### Where the project is
- **Phase 1 under way.** PawCore now also holds **`PcProgress`** (the module). Left in phase 1: `PcAchievements`, the card
  draw (and the rerolls/banishes state it owns), `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the
  game-side third of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L0; L1 33 test cases, 3,740,590 assertions; mirror
  58,651 bit-identical; validator fuzz 3k; Godot repo untouched). Long progress fixture (3,000 rows, 803,621 assertions) PASS.
  The 20 new sabotages all CAUGHT, clean build PASS (two first failed to COMPILE, unused variable under warnings-as-errors,
  and were rewritten to keep it used; then CAUGHT). UBT editor build succeeded (compiles `PcProgress.cpp`,
  `PcProgressTests.cpp`). Last full nightly: (8), before `PcTree`, `PcJobs`, `PcProgress`.
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcProgress`** (`Public/PcProgress.h`, `Private/PcProgress.cpp`) from `game_state.gd`'s mutating functions:
   `MarkBossDefeated` (power vs this cycle's passage, the final-boss clock on the first clear only), `RecordBossDefeat`
   (TierController's rule half: reward, ledger, ending flag), `IsUnicornOwned`, `VisualStage`; `CreditCurrency`,
   `AddCurrency`, `CreditUntouchedCatnip`, `GrantAchievementStardust`, `SpendStardust`; `IncrementKillCount` (+ Compound on
   the wave turnover, Reputation, Fence, Infestation), `PayCatnipInterest`; `RealmIndex`, `BankTierKills`, `BeginBiome`
   (Second Helping's deferred half, deepest biome); `IsBaitUnlocked`, `CanAffordBait`, `BuyBait` (all-or-nothing);
   `IsUpgradeVisible`, `IsUpgradeAtCap`, the kitten locks, `UpgradeCostById`, `PurchaseUpgrade`, `PurchaseUpgradeBulk`,
   `PreviewBulkPurchase`; `Reawaken` (+ `ResetRunCards`, `PcJobs::ClearRunJobs`); `TickPlayClock`, `TickRealtimeInterest`,
   `TickInfestation`; `NewGame`. Functions return what happened; signals, stat rebuilds and sounds are the game module's.
   - `PcProgress` (the struct) gained `Catnip`, `LifetimeKills`, `KillsSinceTithe`, `InfestationBonus`,
     `PendingSecondHelping`, both play clocks, `FinalBossSeconds`, `RealtimeInterestElapsed`. `PcConstants.h` gained
     `CurrencyEpsilon`, `BulkMaxSteps`, `MountedStage`, `LevyEvery`, the Infestation pair, `RealtimeInterestPeriodSec`,
     `SecondHelpingBiomeIndex`.
   - Not here, on purpose: the kill funnel's own rules in `tier_controller.gd` (Levy's lump, Long Lunch, the overkill
     bounty) belong to `PcField` (phase 2); rerolls/banishes/the hand belong to the card draw.
2. **Godot fixture** `tools/godot_fixtures/progress_fixture.gd/.tscn` (`--only progress`): an OPERATION LOG. Each row sets a
   random state, runs 12 random ops, records each op's result (boss_defeated / wave_changed counted through signals, the
   Reawaken grant read from `reawakened`), a digest after each op, and the whole state at the end. `make_godot_fixtures.py`
   gained `convert_tokens` (doubles inside `|`/`,`/`:` cells; used for this table only). `Tests/Data/progress_godot.tsv`
   (1,000 rows, 3.2 MB), `build/progress_godot_full.tsv`.
3. **`Tests/PcProgressTests.cpp`**: replays the ops; coverage asserted for every op and each outcome both ways (1,000 rows:
   365 Reawakens done / 165 refused, 457 new boss clears / 164 rematch passages / 207 nothing, bait 235 / 707, buys 401 /
   769, waves turned 453, Second Helping paid on travel 233, 68 new games...). Long on 3,000; rules from the comments
   (ending flag and clock, a summon one Catnip short keeps bait and Catnip, the epsilon, bulk = preview, what a Reawaken keeps,
   Second Helping at biome 4 not 3, travel, New Game, the clocks).
   - **`pow()` ulps** (KB Godot/03 §6) make some upgrade prices differ by a few ulps, and so the Catnip after paying them:
     compared within 1e-14 of the row's largest bank (measured worst 1.2e-15; 6,103 inexact values), every decision exact.
4. **Sabotages** (now 50): `progress_rematch_no_passage`, `_final_clock_every_clear`, `_ending_on_any_boss`,
   `_kill_skips_multipliers`, `_untouched_whole`, `_no_interest_on_wave`, `_fence_not_reset`, `_infestation_uncapped`,
   `_reputation_kept`, `_second_helping_early`, `_bait_never_short`, `_audit_spends_bait`, `_no_epsilon`,
   `_kitten_power_unlocked`, `_preview_ignores_cap`, `_hidden_line_restored`, `_reawaken_keeps_passage`,
   `_reawaken_keeps_cards`, `_newgame_keeps_jobs`, `_visual_ignores_unicorn`: all CAUGHT.
5. **Findings** (doc 01 §5): **F7** Fence's kill count is not reset by travel or Reawaken (and never in a biome without
   waves); **F8** the Kitten Speed lock is UI-only (`purchase_upgrade()` enforces only Kitten Power's). Both ported as Godot.
6. Docs 00, 01 (F7, F8), 03 (module table, status), 09 §7, 11; appendix B regenerated. Knowledge base `Godot/03` §6: op-log
   fixtures for stateful code; `pow` ulps reaching balances through spending.

### For the designer
- **F6** (one held job: ask or lock in silently?), **F7** (should travel / Reawaken reset Fence?), **F8** (enforce the Kitten
  Speed lock in the rule?): each is a Godot change first if you want it.
- Still from (9): commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: `PcAchievements`; the card DRAW (weighted through `PcRng`, rerolls, banishes, the hand); 
  `Data/CodeConstants.csv` (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable parity (L3); the game-side mirror.
- Run `tools\check_all.ps1 -Full` (~3 h): overdue, last run (8).

### Next step
*"Continue phase 1 per docs/11: port `PcAchievements` (`achievement_manager.gd`: metrics, evaluate, claims paying Stardust
through `PcProgress::GrantAchievementStardust`) following the recipe in docs/09 §7, then `tools\check_all.ps1`."*

---

## 2026-10-29 (10) - `PcJobs` ported; the job math has one home

### Where the project is
- **Phase 1 under way.** PawCore now also holds **`PcJobs`**. Left in phase 1: `PcProgress` (the module), `PcAchievements`, the
  card draw, `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the game-side third of the mirror.
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS (L0; L1 31 test cases, 3,472,407 assertions; mirror
  58,651, bit-identical; validator fuzz 3k; Godot repo untouched). Long jobs fixture (3,000 rows, 261,004 assertions) PASS. The 11
  new sabotages all CAUGHT, clean build PASS (the 19 older ones were not re-run; PcStats/PcEconomy/PcCards changed only by
  moving the job helpers, and their fixtures still pass with identical counts). UBT editor build succeeded (compiles
  `PcJobs.cpp`, `PcJobsTests.cpp`). Last full nightly: (8).
- **Nothing committed** in either repo. The Godot repo still has only the session (9) Second Opinion fix uncommitted.

### What happened
1. **`PcJobs`** (`Public/PcJobs.h`, `Private/PcJobs.cpp`) from `game_state.gd` "Jobs", `job_data.gd` and `job_picker.gd`:
   `AllJobs` (roster order), `HoldsJob`, `JobIsAvailable`, `OfferJob`, `LockRunJob`, `LockRunJobSecond`, `TakeJob`,
   `ClearRunJobs` (the Reawaken's part, for the `PcProgress` port to call), `HasUnionRules`, `UnionRulesShare`, `EquippedJob`,
   `SecondaryJob`, `JobTerm`, `JobCritChanceBonus`, `JobExtraCards`, `JobPounceLockSec`, `JobHatPath`, `IsJobNeutral`,
   `JobHasACost`, `ShouldAskForJob`, `OtherJobsThan`. Signals stay with the caller (functions return bool).
   - The job helpers that lived privately in `PcStats.cpp` (and `JobTerm` / `JobExtraCards` in `PcStats.h`) MOVED here;
     `PcStats`, `PcEconomy`, `PcCards` and the validator (`job.neutral`, `job.free_gift`) now call `PcJobs`. One copy of the
     job math.
   - `PcProgress` gained `CurrentJob` (the standing preference) and `bRunJobChosen`.
2. **Godot fixture** `tools/godot_fixtures/jobs_fixture.gd/.tscn`, registered in `make_godot_fixtures.py` (`--only jobs`): 3,000
   random states. The shipping Union Rules share is a single 0.5, so 40 % of rows overwrite it IN MEMORY in the scratch copy
   (0, 0.25, 0.7, 1, 1.3, -0.2, 2, random) and record the value; the test applies it to a data copy. A card job is aimed at
   the second slot 20 % of the time (uniform states reached the floored card 4 times per 1,000). `Tests/Data/jobs_godot.tsv`,
   `build/jobs_godot_full.tsv`. Equal to Godot on the first run.
3. **`Tests/PcJobsTests.cpp`**: L1 on 1,000 rows with coverage asserted (second job 329, card via floor 16, floored away 114,
   clamped 54, stored second without the node 288, every state change accepted and refused, picker asks / does not), long on
   3,000, and rules from the Godot comments on edited data copies.
4. **Sabotages** (`tools/sabotage_pawcore.py`, now 30): `jobs_share_halves_number`, `jobs_cards_rounded`, `jobs_share_unclamped`,
   `jobs_second_without_node`, `jobs_effect_reads_preference`, `jobs_second_may_be_primary`, `jobs_take_unchanged`,
   `jobs_ask_needs_two`, `jobs_health_cost_inverted`, `jobs_roster_reversed`, `jobs_reawaken_keeps_lock`: all CAUGHT.
5. **Finding F6** (doc 01 §5): the job picker's comment says one held job should lock in silently, but `should_ask()` asks at
   `>= 1`. Ported as the code has it.
6. Docs: 00, 01 (F6), 02 §10 (the job rules), 03 (module table, status), 09 §7 (counts), 11 (status); appendix B regenerated.
   Knowledge base `Godot/03` §6: feeding the oracle other data in memory; `sort_custom` is stable at 16 elements or fewer.

### For the designer
- **F6**: should the job picker open when you hold exactly one job? Godot asks today; its own comment says it should not.
- Everything from (9) still stands: commit the Godot Second Opinion fix and the Unreal work (explicit paths:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/`), then delete the spike code; F2, F3, Q1-Q9 open.

### Open
- Phase 1 remaining: `PcProgress` (boss ledger, waves, passage, and the Reawaken that calls `PcJobs::ClearRunJobs`),
  `PcAchievements`, the card DRAW, `Data/CodeConstants.csv`, `tools/import_data.py` + DataTable parity, the game-side mirror.
- Run `tools\check_all.ps1 -Full` (~3 h) once; last full run was (8), before `PcTree` and `PcJobs`.

### Next step
*"Continue phase 1 per docs/11: port the `PcProgress` module (`GameState` boss ledger `mark_boss_defeated`, evolution and visual
stage, waves, biome unlocks, passage per cycle, final boss, and the Reawaken's resets incl. `PcJobs::ClearRunJobs`) following the
recipe in docs/09 §7, then `tools\check_all.ps1`."*

---

## 2026-10-29 (9) - Second Opinion fixed (once per Catches level); `PcTree` ported; every doc brought up to date

### Where the project is
- **Phase 0 done** (spikes S1-S4 recorded; spike code kept until a commit preserves it, then deleted).
- **Phase 1 under way.** PawCore holds `PcData` (+ generated `PcDataRows.h`, `PcEnums.h`), the validator (55 rules),
  `PcFormat`, `PcRng`, `PcCombat`, `PcStats`, `PcEconomy`, `PcCards` (all but the hand draw) and, new today, **`PcTree`**.
  Every module equals the REAL Godot code on random states (Godot fixtures, doc 09 §7) and PawCore equals pawsim bit for bit
  (the mirror).
- **Gates, measured today**: `tools\check_all.ps1` everyday set all PASS: L0 static; L1 29 test cases, 3,385,261 assertions;
  mirror 58,651 assertions, all bit-identical; validator fuzz 3k; Godot repo unchanged by the run. Long fixtures for cards
  (3,000 rows) and tree (3,000 rows, 1,635,003 assertions) PASS. Sabotages: `py tools/sabotage_pawcore.py`, all 19 run today and
  all CAUGHT, clean build PASS. UBT editor build
  succeeded and compiles `PcTree.cpp` and `PcTreeTests.cpp`. The full nightly (`check_all.ps1 -Full`) was last run in (8).
- **Nothing committed** in either repo. One file changed in the Godot repo (below).

### What happened
1. **Designer decision: Second Opinion hands over its rerolls once per card, i.e. once per Catches level / hand** (doc 01 §5
   F1, doc 02 §9 cards).
   - Godot first (CLAUDE.md): `C:\Games\pawpocalypse\scripts\autoload\card_manager.gd` `add_kill_catches()`: the
     `rerolls += GameState.get_rerolls_per_level()` line moved from the not-levelled branch to just after
     `catches_level += 1`. **UNCOMMITTED in the Godot repo** (`git -C C:\Games\pawpocalypse status` shows exactly this file).
     Godot's own `ci/cards_test.tscn` passes on it (263 assertions; run in the sandboxed scratch copy).
   - PawCore: `PcCards::AddKillCatches` grants `RerollsGranted` on the levelling path only.
   - Fixtures regenerated from the fixed Godot (`make_godot_fixtures.py --only cards`): only `step_rerolls` changed; 97
     levelling rows grant, 0 non-levelling rows do. `PcCardsTests.cpp` now asserts no grant without a level and > 20 grants.
   - Sabotages: `second_opinion_every_kill` (the old bug put back) and `second_opinion_never`: both CAUGHT.
2. **`PcTree` ported** (`Source/PawCore/Public/PcTree.h`, `Private/PcTree.cpp`) from `skill_tree_manager.gd`: what the tree
   sells (required parent, rival fork, boss key, Reawaken gate with the three ability-unlock constants), prices with
   Godot's sentinels (-1 maxed, +inf unknown id), affordability, `BranchForJob`, `BlockingRival`, the purchase (Stardust
   spent, level +1, a job node hands its job over once, Auto-Travel / Auto-Card ask for their switch ON) and the free grant
   (never lowers, clamps). `PcProgress` (in `PcStats.h`) gained `Stardust` and `JobsHeld`.
   - Godot fixture `tools/godot_fixtures/tree_fixture.gd/.tscn`, wired into `make_godot_fixtures.py` (`--only tree`):
     3,000 states, every branch per row; a third of the probes aimed at job and automation nodes, and every gate in front of
     the probe opened half the time (uniform states had reached the automation nodes 0 times). Full set: 1,040 purchases,
     242 jobs handed over, 81 automation switches. `Tests/Data/tree_godot.tsv` (1,000 rows), `build/tree_godot_full.tsv`.
   - `Source/PawCore/Tests/PcTreeTests.cpp`: L1 (545,099 assertions, coverage asserted), long (3,000 rows), and rules from the
     Godot comments. Equal to Godot on the first run.
   - The shipping data sets the three ability-unlock constants AND those branches' `required_reawakens` all to 0, so the
     fixture cannot see which one the gate reads (sabotage `tree_pounce_gate_swapped` was MISSED by it); a rules test on a
     data copy with distinct numbers now catches it. Sabotages `tree_rival_ignored`, `tree_boss_gate_ignored`,
     `tree_pounce_gate_swapped`, `tree_grant_lowers`: all CAUGHT.
3. **A stale rule removed from the docs**: "tree keys are parsed from the RIGHT" (docs 02, 03, 12, appendix B via
   `tools/make_port_map.py`). Godot keys nodes by branch id alone since 2026-08-21 (`ci/check_hud.gd`); old keys survive only
   in v5 saves, migrated by `save_manager.gd` `_migrate_branch_levels` (the save port, phase 4).
4. **Docs brought up to date**: doc 00 (new "Where the port is" and a full inventory of what exists); doc 01 new §5 "Findings
   in the Godot game" (F1-F5 with status); doc 02 (tree fork, boss key, job hand-over, automation switch, free grants; Second
   Opinion rule); doc 03 (PawCore status, `PcSkillBranch` name, the `PcProgress` struct vs module note); doc 06 (pawsim on
   CSV done, mirror numbers 58,651 / 11 sabotages, the Godot oracle list, the CLI's real flags); doc 09 §7 (counts, tree,
   and **the recipe for porting a manager**); doc 11 status; doc 12; CLAUDE.md (check_all exists, the gate commands, the
   Godot-first rule for decisions); the night worklist marked closed. Appendices A and C regenerated: unchanged; B: one line.
   Knowledge base: `Godot/03` §6 (aimed probes with asserted coverage; degenerate data hides a rule; operand-scaled
   tolerance), `Godot/00-INDEX.md`.

### For the designer
- **Commit the Godot fix** in the Godot repo when you are happy with it: `scripts/autoload/card_manager.gd` only.
- **Commit the Unreal work** (nothing since `7410909`). Stage explicit paths, never `git add -A`:
  `CLAUDE.md Config/ Pawpocalypse.uproject Source/ Content/ docs/ tools/` (`build/` and `__pycache__/` are ignored). After the
  commit, delete the spike code (`Source/Pawpocalypse/Dev/Spikes`, `Content/Pawpocalypse/Dev/Spikes`, `tools/spikes`) per doc 04 §10.
- Still open: F2 achievements `br_ferocity_5` / `br_mystique_6` unearnable; F3 Patience x Attack Speed trap (design note);
  questions Q1-Q9 (doc 01 §3; Q9 = no AA vs FXAA).

### Open
- Phase 1 remaining: `PcJobs`, `PcProgress` (the module: boss ledger, waves, passage), `PcAchievements`, the card DRAW
  (weighted, through `PcRng`); `Data/CodeConstants.csv` (+ `PcConstants.h` moves there); `tools/import_data.py` + DataTable
  parity (L3); the game-side third of the mirror.
- `make_godot_fixtures.py --check` for cards reproduces only against the Godot tree WITH the fix (committed or not).
- Run `tools\check_all.ps1 -Full` once more (~3 h) after the next module; last full run was (8).

### Next step
*"Continue phase 1 per docs/11: port the jobs (`GameState` job functions: held, worn per run, Union Rules' second job, job
terms) into `PcJobs` following the recipe in docs/09 §7 (Godot fixture via `tools/make_godot_fixtures.py`, coverage asserted,
sabotages seen caught), then `tools\check_all.ps1`."*

---

## 2026-10-28 (8) - night session: phase 1 core rules, every gate built and negative-tested

### Where the project is
- **Phase 1 is well under way** (docs/11 status line). PawCore now holds `PcData` (+ generated `PcDataRows.h`, `PcEnums.h`),
  the validator (55 rules), `PcFormat`, `PcRng`, `PcCombat`, `PcStats`, `PcEconomy`, `PcCards`. Each rule module is proved against the
  REAL Godot code on random inputs: `PcCombat` bit for bit on 5,000 `CombatResolver` rows, `PcStats` on 3,000
  `_rebuild_stats()` states, `PcEconomy` on 3,000 economy states, `PcCards` on 3,000 `CardManager` states, `PcFormat` on
  4,403 `Amounts.short` values (only `pow`
  differs, by up to 14 ulps of msvcrt vs MSVC). PawCore also matches pawsim bit for bit (the mirror, 58,651 assertions).
- **`tools/check_all.ps1` exists** (doc 09 §7): the everyday set in minutes, `-Full` the nightly (~3 h). Every gate has its
  own negative test (self-test or sabotage) and each was seen to fail.
- Nothing committed. Godot repo untouched (checked before and after every run). The UBT editor build compiles all of
  PawCore (8.7 s).

### What happened this session (worklist: `docs/WORKLIST-NIGHT-2026-10-28.md`)
1. pawsim copied in and taught to read `Data/*.csv`; byte-identical to its `.tres` loader on the full game and over a
   90-setting sweep (`tools/check_pawsim_parity.py`; `--self-test` added: one changed CSV cell must show).
2. PawCore data layer + validator (each rule with a sabotage). Found: achievements `br_ferocity_5` / `br_mystique_6` count
   branches that were merged into `claws` (unearnable, 2 Stardust each); Godot's `check_resources` runaway branch never ran.
3. `PcFormat`, `PcRng`, `PcCombat`, `PcStats`, `PcEconomy`, `PcCards` + the Godot fixtures, now regenerated by ONE tool:
   `py tools/make_godot_fixtures.py [--check]` (scratch copy of the Godot project, sandboxed APPDATA; `--check` proved all
   fixtures reproduce byte for byte).
4. `pawcore_sim.exe` (`Tests/PcSimCli.cpp`) + `tools/check_mirror.py`: Godot's mirror fixture at every stage, 2,000 random
   states, every price, the Stardust grant, Legacy, Fortune, golden chances. `--self-test`: 11 data sabotages, all caught.
5. Long suite (`build_pawcore_tests.ps1 -Long`, ~30 min): full Godot fixtures + property tests over ~150 M random states
   (see doc 09 §7). `tools/sabotage_pawcore.py`: 8 one-line source sabotages, each caught by its long test.
6. Validator fuzz (`tools/fuzz_validator.py`): 30,000 typed single-cell corruptions. It found two validator gaps, both now
   rules: **`ids.lower_case`** (upper-case ids passed) and **`ref.branch_boss`** (a `gated_by_boss` naming a non-boss biome
   locks the branch forever). Remaining uncaught hostile classes are explained in the script (dead `ability_ids`; a shorter
   ladder on a no-cost card).
7. `tools/check_pawcore_static.py` (L0: no float, engine-free PawCore, guarded tests, generated headers current, `Data/` =
   a fresh Godot export) with `--self-test`; `tools/check_all.ps1` runs it all.
8. Docs: 06 §8, 09 §7, 11 (phase 1 status), this log. KB: Godot/03 §6 (Godot as a bit-exact oracle: `var_to_bytes` 32-bit
   floats, msvcrt `%.1f` rounding, MinGW `pow` ulps), Godot/07 rows, Unreal/23 (Python `\b` in Windows paths), INDEX.

### Final results of the night's runs
- `tools/check_all.ps1` (everyday set): all PASS; L1 27 test cases / 2,839,149 assertions; mirror 58,651; fuzz 3k clean.
- Long suite (36 min): 10/10 PASS (45 M states; 20 billion RNG draws, chi2 1,010 on 1,023 dof; Patience trade-off in
  48,665 of 9 M states). The economy/cards long cases (added later) then ran separately at full scale: 5/5 PASS after one
  fix: the full economy fixture showed `wave_done` 1.9e-12 off in 3 of 3,000 rows (a small difference of two large pow
  series), so that one field now has a tolerance scaled to its operands; a one-kill sabotage is still caught.
- `tools/sabotage_pawcore.py`: 14/14 caught. Mirror `--self-test` 11/11. Parity `--self-test` caught. L0 `--self-test` 5/5.
- pawsim parity sweep: 90/90 identical (110 min). Validator fuzz 30k: every hostile corruption caught or explained.
- Godot repo `git status` empty after every run.

### For the designer (found tonight, nothing changed in Godot)
- **Godot bug, Second Opinion**: `card_manager.gd` `add_kill_catches()` adds `get_rerolls_per_level()` rerolls in the branch
  where the kill did NOT level up (commit ca6b9784), so an owner gains rerolls on nearly every kill, not per level as the
  comment and node text say. `check_damage` tests the getter, not the grant. **Fixed 2026-10-29, entry (9).**
- **Godot data bug**: achievements `br_ferocity_5`, `br_mystique_6` can never be earned (merged branch ids).
- **Design note (not a bug)**: with Patience owned and the cat slower than base (the Sphinx), buying Attack Speed can LOWER
  paw DPS (d DPS / d speed < 0 once per x crit chance x crit mult > 1 + crit chance x (crit mult - 1)). The shop would sell a
  trap in that build. Counted by the long suite, not failed.
- **pawsim vs game**: pawsim caps the kitten count, the game does not (only reachable past the shop's cap; harmless).
- `Amounts.short` prints an overflowed integer past ~9.2e18 after the suffixes run out (PawCore prints the digits).
- Still open: Q1-Q9 (Q9 = no AA vs FXAA); commit of phase 0 + tonight's work.

### Open
- Phase 1 remaining: `PcProgress`, `PcTree`, `PcJobs`, `PcAchievements`; `PcCards`' hand DRAW (weighted, through `PcRng`;
  the eligibility, hand size, values and XP step are done, Second Opinion ported as Godot has it, flagged); `Data/CodeConstants.csv` (+ the constants in `PcConstants.h` move there);
  `tools/import_data.py` + DataTable parity (L3); the game-side third of the mirror.
- Spike code still to delete after a commit (entry (7)).

### Next step
Designer: read "For the designer" above, then commit (explicit paths; the night's files are listed in the worklist).
Then: *"Continue phase 1 per docs/11: port SkillTreeManager into PcTree (buy rules, prerequisites, exclusivity, respec) with
a Godot fixture via tools/make_godot_fixtures.py, then run `tools\check_all.ps1`."*

---

## 2026-10-28 (7) - spike S4 done: all four spikes recorded

### Where the project is
- **Phase 0 step 5 is done in substance**: S1-S4 recorded in doc 04 §10 (and doc 05/07). What remains of phase 0: delete the
  spike code and assets (after a commit keeps them in history), the designer's review of the phase 0 changes, and the open
  questions Q1-Q9 (Q9 = no AA vs FXAA). Then phase 1. Nothing committed yet.
- S4: the doc 05 UI structure (root layout with Game/Menu stacks; HUD activatable with the dock; tree activatable with 87 nodes
  in one canvas) built in C++ only passed **1,193 focus checks** with real Slate key events, focus never lost; every one of the
  tree's 87 x 4 navigation rules pressed and verified.

### What happened this session
1. `Source/Pawpocalypse/Dev/Spikes/SpikeUi.*` (button style, row button, HUD/dock, tree + link painter, root layout, tour
   actor), `tools/spikes/spike_s4_setup.py` (map `L_SpikeUi`). Deps added to the SPIKES line of `Pawpocalypse.Build.cs`: UMG,
   CommonUI, CommonInput, InputCore.
2. First tour: 455 pass, 3 fail, all one cause: the dock's remembered row went stale when the input device switched (Common UI
   re-applies the focus target). Fixed by tracking it every frame; a device-switch check added.
3. Negative tests: `-S4Sabotage=focus` failed loudly (focus to `SViewport`); `-S4Sabotage=nav` first PASSED (the walk only took
   shortest paths), so the walk now presses every rule, and then failed as it must.
4. Screenshots (windowed): the focus highlight is inconsistent (style it explicitly) and the tree needs an opaque backdrop.
5. Docs: 04 §10 (S4), 05 (§2 rules 8-11, §6.2), 11, 12, this log; KB Unreal/08 §3.4, Unreal/09 §3, Unreal/24, INDEX.

### Open
- Designer: review the phase 0 + spike changes and decide what to commit; Q1-Q9.
- Delete spike code/assets once committed (list in entry (6) plus `SpikeUi.*`, `spike_s4_setup.py`, `Content/.../Dev/Spikes/S4`,
  `Saved/Spikes`). Keep `tools/spikes/s3_raster_pack.py` ideas for phase 6 (or move it to `tools/` first).
- Real-pad checks (Xbox, PlayStation via Steam Input) are phase 5.

### Next step
Designer review and commit, then: *"Delete the phase 0 spike code and assets per docs/HANDOFF-LOG.md (7), confirm the phase 0
exit criteria in docs/11, then start phase 1."*

---

## 2026-10-28 (6) - spike S3 done: 2x rasters, BC7, 2048 pages; render resolution fixed; AA reopened as Q9

### Where the project is
- **S1, S2, S3 done** (doc 04 §10); S4 (dock and tree on a pad) not started. Nothing committed.
- S3 decisions (doc 07 §2.2): rasters at 2x the largest on-screen size (`resvg_py`), BC7, 2048 atlas pages extruded 4 px; the 52
  field sprites take 2 pages, 11.2 MB of VRAM.
- **Config fix**: `DefaultEngine.ini [SystemSettings] r.ScreenPercentage.Default.Desktop.Mode=0`, `r.ScreenPercentage.Default=100`.
  The engine's desktop default rendered a 1080p window at 810p (4K at 1080p) and upscaled, the biggest blur in the chain.
- **S1's AA choice reopened**: its windowed runs had been made on that upscaled image. Re-run at 100 %: the choice is no AA vs
  FXAA, a look question, now **Q9 in doc 01 §3** (image sent to the designer: `build/q9_edge_smoothing.png`). FXAA stays set.
- Spike material (delete after S4): `Source/Pawpocalypse/Dev/Spikes/` (SpikeLook, SpikeCrowd, SpikeArt), the SPIKES deps in
  `Pawpocalypse.Build.cs`, `tools/spikes/`, `Content/Pawpocalypse/Dev/Spikes/S1..S3`, `SourceArt/png/spike_s*` (gitignored).

### What happened this session
1. Offline: `tools/spikes/s3_raster_pack.py` (raster variants r1x / r2x / Godot-native, shelf packer, ideal and Godot-equivalent
   references); Unreal: `SpikeArtActor` + `spike_s3_setup.py` (BC7 and uncompressed pages, atlas material, `L_SpikeArt`);
   `spike_s3_runs.ps1`, `spike_s3_compare.py` (interior error, whole-cell error, sharpness vs ideal).
2. A different Unreal project on this machine (`C:\Games\Cabbage`, packaging) kept running `UnrealEditor-Cmd`; the editor guard
   aborted twice. Its processes never touched this project (git status unchanged). The S3 scripts now guard only against an
   editor open on Pawpocalypse (window title), since the command lines of those processes are not readable.
3. The first capture of the batch drew nothing (shader compile); rerun. Captures alone did not show the screen's blur; windowed
   back-buffer runs found it, the screen-percentage cause was isolated with forced-100 % and no-AA runs, and the config fix was
   verified by a run without overrides.
4. Docs: 04 (§10 S3 + S1 amendment), 07 §2.2 and §4, 03 §7, 01 (Q9), 11, 12, this log; KB 2D/02 §4.3, Unreal/24, INDEX.

### Open
- **Q9 (designer)**: no AA vs FXAA. If neither, measure supersampling on the Deck-class GPU.
- S4. Then delete all spike code and assets, and phase 0 ends (exit criteria in docs/11).
- Not measured in S3: cooked disk size, trimming/pivots, premultiplied alpha (phase 6).
- Designer: review/commit the phase 0 and spike config changes; Q1-Q9.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S4 from docs/04 §8 (dock and skill tree with Common UI on a
pad; decisions into doc 05)."*

---

## 2026-10-28 (5) - spike S2 done: renderer = instanced quads

### Where the project is
- **S1 and S2 done** (doc 04 §10). S2: the instanced-quad renderer (doc 04 §5.1) stays: 13 draw calls for 774 items, game
  thread 0.6 ms, worst-case GPU 5.15 ms at 1280x800 on an integrated GPU weaker than the Deck's. Paper2D rejected (~2,200 draw
  calls). Slate is the measured fallback (half the GPU time on the weak GPU). S3 and S4 not started. Nothing committed.
- A packaged Development build exists in `build/package_s2` (gitignored; spike only, not a release). `Config/DefaultGame.ini`
  gained `+DirectoriesToNeverCook` for `/Interchange` and `/NNEDenoiser`: **without it the project does not cook** (the phase 0
  plugin trim made their code editor-only while engine config still references their content).
- Spike material (delete after S3/S4): `Source/Pawpocalypse/Dev/Spikes/SpikeLookActor.*`, `SpikeCrowdActor.*`, the SPIKES deps
  in `Pawpocalypse.Build.cs`, `tools/spikes/` (S1 and S2 setup/run/compare), `Content/Pawpocalypse/Dev/Spikes/S1`, `/S2`,
  `SourceArt/png/spike_s1`, `spike_s2` (gitignored).

### What happened this session
1. Built the S2 crowd (fake 60 Hz sim, 774 items) with three renderers: ISM with an atlas (per-instance UV offset), Paper2D per
   item, Slate leaf widget (band sort, and a strict per-creature-layer variant).
2. Editor sanity runs: the first ISM run drew nothing (shaders still compiling); the additive material drew squares (fixed:
   RGB x A).
3. First package failed to cook (Interchange/NNEDenoiser, see above). Second: Paper2D drew nothing (its materials loaded by path
   were not cooked) and ISM was still empty on the iGPU after 120 frames (PSO precaching). Fixed: sprites reference their
   materials, warm-up 6 s, a screenshot per run.
4. Third package: 12 runs, all checked by screenshot. Paper2D's soft layers still mis-rendered (the setup's `load_asset` could not
   find `/Paper2D/TranslucentUnlitSpriteMaterial`); recorded as a caveat, the verdict does not depend on it.
5. A process slip, for the record: one edit script failed (a heredoc mangled its match) while the rebuild pipeline had already
   been launched in the same step; the pipeline was stopped before it built anything wrong and no processes were left.
6. Docs: 04 (§5, §10 S2), 03 §7, 11, 12, this log; `tools/trim_game_plugins.py` docstring; KB 2D/02 §4.2, Unreal/19 §3.1,
   Unreal/24, INDEX.

### Open
- S3 (SVG -> PNG -> atlas -> import; `resvg_py` is installed; compression BC7 vs UserInterface2D; raster scale) and S4 (dock and
  skill tree on a pad).
- Why ISM costs 2x Slate's GPU on the weak iGPU (deferred passes? fill rate of large translucent zones?): look in phase 2/8
  with Insights before the Deck pass.
- Designer: review/commit the phase 0 + spike config changes; Q1-Q8.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S3 from docs/04 §8; record the numbers in §10, doc 07 and
Knowledge/2D/02."*

---

## 2026-10-28 (4) - spike S1 done: AA = FXAA

### Where the project is
- **Spike S1 is finished** (doc 04 §10): exact colour proven (within 1 offscreen, within 2 on the real screen, 1080p and
  1280x800), exposure 0 EV, colour-gate method decided, **AA = FXAA** (`r.AntiAliasingMethod=1` now in `DefaultEngine.ini`,
  confirmed live). S2-S4 not started. Still nothing committed.
- Spike material stays until S2-S4 are recorded (S2 can reuse the S1 scene): `Source/Pawpocalypse/Dev/Spikes/`, `tools/spikes/`,
  `Content/Pawpocalypse/Dev/Spikes/S1/`, the SPIKE deps in `Pawpocalypse.Build.cs`.

### What happened this session
1. With the designer's go-ahead, nine windowed runs (back-buffer readback; `tools/spikes/spike_s1_runs.ps1 -Windowed`).
2. The first windowed run showed the field at 0.57x: the local player's default `MaintainYFOV` makes `OrthoWidth` the visible
   **height**. Fixed in the spike (`OrthoWidth = 1080` + `MaintainYFOV` override); **doc 04 §6 corrected** (it said
   `OrthoWidth = logical field width`).
3. Screen comparison: FXAA smooth with no ghosting (0.33 ms GPU); TAA smears moving faces; TSR blurs and ghosts (1.16 ms); forward
   MSAA leaves masked outlines jagged. Decision FXAA.
4. Docs: 04 (§4 choice, §6 camera, §10 results), 03 §7, 11, 12, this log; KB 2D/02 §3 and §4.1, Unreal/24, INDEX.

### Open
- S2 (renderer for 320 enemies; packaged-build timings at 1080p and 1280x800), S3 (raster pipeline; `resvg_py` is installed),
  S4 (dock and tree on a pad). Translucent layers (glows) were not built in S1: check them in phase 6.
- Designer: review/commit the phase 0 changes; Q1-Q8.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S2 from docs/04 §8; record the numbers in §10 and
Knowledge/2D/02."*

---

## 2026-10-28 (3) - spike S1: colour proven exact; AA method still open

### Where the project is
- Phase 0 step 5 (spikes) is under way. **S1 colour: done and decided** (doc 04 §10). **S1 AA method: open**, it needs runs with a
  real window. S2-S4 not started. Still nothing committed; the step 1-4 settings diff still waits for the designer's review.
- A plan page for the whole port was published for the designer (claude.ai artifact "Pawpocalypse Unreal Port", built from
  docs/11, 04 §8 and 01 §3).
- Temporary spike material, to delete once S1-S4 are recorded: `Source/Pawpocalypse/Dev/Spikes/SpikeLookActor.*` (and the
  `RHI`, `ImageCore` deps in `Pawpocalypse.Build.cs`, marked SPIKE), `tools/spikes/` (setup, runs, compare),
  `Content/Pawpocalypse/Dev/Spikes/S1/` (40 textures, 3 materials, `L_SpikeLook`), `SourceArt/png/spike_s1/` (gitignored).

### What happened this session
1. Sprites for S1: the official resvg binary download was refused by the permission check, so the 40 sprites (33 creatures + 7
   bosses at 2x) were rasterised with **Godot's own SVG renderer** from an empty scratch project with `APPDATA` sandboxed. The
   Godot repo stayed untouched (no new writes, clean `git status`). Afterwards it turned out `resvg_py` was installed all along
   (doc 07 §1 says so; an earlier check was misread), so S3 can use it.
2. The spike scene: ortho camera, 21 palette swatches, a static sprite grid, 200 movers, doc 04 §3 post-process; readback, timing,
   PNG and report to `Saved/Spikes/S1/`.
3. **Colour: all 21 swatches within 1 of the palette** at exposure bias 0 EV in every AA mode; Tone Curve 1 fails it (worst 35).
4. Traps found and written into the KB (2D/02 §4.1, Unreal/03 §1, Unreal/24): screen readback is black under `-RenderOffscreen`;
   render-target gamma (`RTF_RGBA8` + `bForceLinearGamma=false` is the only right combination); a scene capture does not show the
   screen's AA; the material Alpha to Coverage flag breaks on desktop; plane UVs need yaw +90; with no pawn the controller resets
   the view target after BeginPlay.
5. AA timings (GPU ms incl. capture): none 0.52, FXAA 0.57, TAA 0.64, forward MSAA 0.69, TSR 1.40. Option A (MSAA + A2C) is out.
6. Docs updated: 04 (§3.5 readback method, §4 option A note, §10 results), 07 (§2.1 how a design is changed), 11 (status), 12
   (pitfalls), 13 (sources note), 00 (temporary spike folders).

### Open
- **S1 AA decision** needs windowed runs (a game window appears for ~20 s per run on the designer's desktop): candidates forward
  + MSAA 4x (plain masked), TAA, FXAA, TSR, at 1080p and 1280x800. Ask the designer before running them.
- Then S2 (renderer), S3 (raster pipeline, can use `resvg_py`), S4 (dock and tree on a pad).
- Designer: review and commit the phase 0 settings diff; questions Q1-Q8.

### Next step
After the designer agrees to windowed runs: *"Finish spike S1: run the AA candidates windowed per doc 04 §10, decide, set
`r.AntiAliasingMethod` in DefaultEngine.ini, record in §10 and Knowledge/2D/02."* Then S2.

---

## 2026-10-28 (2) - phase 0 steps 1-4 done: settings, modules, maps, PawCore test build

### Where the project is
- Phase 0 steps 1-4 of docs/11 are done; **step 5 (spikes S1-S4) has not started**. Nothing is committed: every change below is in
  the working tree, waiting for the designer to review the settings diff (a phase 0 exit criterion) and ask for a commit.
- Builds from the command line: **Editor, Game Development, Game Shipping** (all `Result: Succeeded`, no warnings from our code).
  The game boots headless (`-game -nullrhi`): `LogPaw: Pawpocalypse started; rules library: PawCore`, `L_Title` loads,
  `Game class is 'PawGameMode'`.

### What happened this session
1. Read the KB pieces docs/11 step 1 names (INDEX, 2D/02, Unreal 01, 12 §1, 24, the relevant parts of 08, 17, 19) and docs 00-05, 09-11.
2. **Settings** (docs/03 §7, now with an "As set in phase 0" paragraph): Lumen, RT, Substrate, Nanite, VSM, distance fields,
   static lighting, auto exposure, motion blur, bloom, AO, lens flare, skin cache all off; local exposure contrast 1.0 (the template
   had 0.8, which shifts flat colours); maps `L_Title` (game default) / `L_Realm` (editor startup); `PawGameInstance`,
   `PawGameMode`; `bUseSplitscreen=False`; `CommonGameViewportClient`; high DPI + linear ShortestSide curve; always-cook
   `/Game/Pawpocalypse`; `bEnableEnhancedInputSupport`; Enhanced Input user settings (in `DefaultInput.ini`: the class is
   `config=Input`). **Doc bug fixed**: docs 03/05 said the template's Common UI cvars were already in `DefaultEngine.ini`; they were
   in `DefaultGame.ini` (ignored), now moved. Every cvar and the map/class settings were read back live in a headless editor; the
   rest were checked against the engine headers (property names, ini file per class).
3. **Plugins**: CommonUI, EnhancedInput, PythonScriptPlugin on; EditorScriptingUtilities editor-only; **154 engine plugins
   restricted to the Editor target** by `tools/trim_game_plugins.py` (re-runnable, scans the engine). First attempt (69 runtime
   plugins) still linked ControlRig/RigVM/GeometryCache because editor-only plugins depend on them; restricting those too took the
   game from 64 linked plugin folders to 34. Shipping exe 112 MB (unpackaged, before any content). Paper2D stays until S1/S2.
4. **Modules** (docs/03 §1): `PawCore` (pure C++, `NoPCHs`, placeholder `Pc::LibraryName()` until phase 1; `PcCore.h` defines
   `DLLEXPORT`/`DLLIMPORT` itself because UBT's `PAWCORE_API` needs them and PawCore includes no engine header), `Pawpocalypse`
   (`Core/`: thin `UPawGameInstance`, `APawGameMode` with no pawn, `APawPlayerController` with the cursor shown, `LogPaw`),
   `PawpocalypseEditor` (empty). The other `Source/Pawpocalypse/` folders get created when their first file arrives.
5. **`tools/build_pawcore_tests.ps1`**: PawCore + `Source/PawCore/Tests` into one exe with plain MSVC (`/std:c++17 /W4 /WX`),
   doctest 2.4.11 (MIT, `Tests/ThirdParty/doctest/`). 1 test passes; **negative-tested**: a wrong return value fails (exit 1) and a
   compile error fails (exit 1); restored code passes.
6. **`tools/create_maps.py`**: created the empty `L_Title` and `L_Realm` under `Content/Pawpocalypse/Maps` (LFS); only creates
   missing maps.
7. Knowledge base: new `[Verified]` entries in Unreal/01 §2.1 and §2.3.1 (vcvars stderr in PowerShell; engine-free module:
   DLLEXPORT, Tests guard), Unreal/19 §3.1 (plugin trimming that actually works, check the `.exe.rsp`), Unreal/17 §2 (reading
   settings from Python; which ini each settings class reads), Unreal/08 (template writes Common UI cvars to the wrong file),
   Unreal/03 §1 (no-pawn game mode), 2D/02 §3 (cvars confirmed live; template local exposure 0.8), Unreal/24 rows, INDEX rows.

### Open
- **The designer reviews the settings diff** (`git diff Config Pawpocalypse.uproject`), then asks for a commit if happy. Untracked
  files to stage explicitly: `Content/Pawpocalypse/Maps/*.umap`, `Source/PawCore/`, `Source/Pawpocalypse/Core/`,
  `Source/PawpocalypseEditor/`, `tools/build_pawcore_tests.ps1`, `tools/create_maps.py`, `tools/trim_game_plugins.py`.
- Not verified live: `CommonGameViewportClient` actually in use (needs a run with a renderer; check it in S1).
- Left for later steps on purpose: `r.AntiAliasingMethod` (S1), Common UI `InputData` + controller data assets (S4 / phase 5),
  hardware cursors (phase 5), `PawTokens.h`, `tools/check_all.ps1` (phase 1).
- The designer questions Q1-Q8 (docs/01 §3) are still unanswered; none blocks the spikes.

### Next step
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then run spike S1 from docs/04 §8; record the numbers in §10 and
Knowledge/2D/02."* Before it: the designer's review of this session's diff.

---

## 2026-10-28 - the port is planned; nothing is built yet

### Where the project is
- `C:\Games\pawpocalypseUE\Pawpocalypse`: a **blank UE 5.8.3 C++ project** (`5.8.3-58210709`) created by the designer, plus the
  complete port handoff. Git repo on `master` (LFS configured), pushed to **`https://github.com/Keshikul/PawpocalypseUE`**
  (its own repo, separate from the Godot one: designer, 2026-10-28).
- **No engine settings changed yet.** The template still has Lumen, ray tracing, Substrate, VSM and distance fields ON and the
  OpenWorld startup map: phase 0 step 3 fixes that (docs/03 §7). **Do not build anything on the template defaults.**
- The Godot game (`C:\Games\pawpocalypse`, GitHub `JanePonomareva/pawpocalypse`, itch 0.2.5) is the specification and keeps
  shipping until parity (docs/01 D12).

### What happened this session
1. Read the whole knowledge base (`C:\Games\Knowledge`: Unreal 00-26, Steam, PlayStation, Godot 03-05) and the Godot project's
   design and technical docs (`pawpocalypse.md`, GDD v9, technical design, outstanding, handoff headings, data schemas).
2. Web research on 2D in Unreal and on 2D optimisation (sources: docs/13). Key findings: Paper2D gets little development; one
   Actor per sprite does not scale; the tonemapper and auto exposure shift flat colours; translucency sorts per component, never
   inside an instanced component; masked edges need MSAA + alpha-to-coverage or temporal AA; texture streaming blurs sprites;
   no production SVG import; software cursors lag; idle games need their own unfocused throttle; Valve suggests 1 MiB pak padding
   alignment for SteamPipe deltas.
3. Wrote the handoff: docs 00-13, `CLAUDE.md`, and three generated appendices.
4. Exported every Godot `/data` resource to `Data/*.csv` (266 resources, 11 classes, enums by name) with
   `tools/export_godot_data.py`; listed the 456 constants living in Godot code (`tools/list_godot_code_constants.py`, appendix C);
   mapped all 79 Godot scripts (29,258 lines) to their Unreal destinations (`tools/make_port_map.py`, appendix B).
5. Designer, mid-session: **"No lumen, no raytracing"** (recorded in CLAUDE.md, docs/03 §7, docs/04); asked for 2D optimisation
   research and for the knowledge base to be split by dimension.
6. Knowledge base: new `2D/` folder (`01-2d-optimization-best-practices.md`, `02-unreal-2d.md`), `3D/` (an index into the
   3D-specific Unreal docs, which were NOT moved because the global CLAUDE.md links them by path) and `VR/` (empty placeholder);
   README and INDEX updated. The knowledge folder is not a git repository.

### Open: questions for the designer (docs/01 §3)
Q1 what the move must deliver (consoles? Deck? look?); Q2 freeze design until parity (recommended yes); Q3 import itch saves
(recommended yes); Q4 Next Fest Feb 2027 demo from Godot (recommended); Q5 2x rasters for 4K (recommended); Q6 exact Godot colours
first (recommended); Q7 music licence unchanged; Q8 Boba unchanged. **None blocks phase 0.**

### Open: decisions the spikes must make (docs/04 §8, record in docs/04 §10 and Knowledge/2D/02)
S1 anti-aliasing method and exposure value (colour readback within +/-2); S2 renderer (instanced quads vs Paper2D vs Slate canvas);
S3 raster scale, compression, atlas size, OGG import; S4 dock and skill tree widget structure on a pad.

### Next step
Open a session in `C:\Games\pawpocalypseUE\Pawpocalypse` and run:
*"Read docs/00-START-HERE.md and docs/HANDOFF-LOG.md, then do phase 0 steps 1-4 of docs/11-roadmap.md. Do not start the spikes."*
Phase 0 step 2 (git) is already done. Push only when the designer asks in that message.
