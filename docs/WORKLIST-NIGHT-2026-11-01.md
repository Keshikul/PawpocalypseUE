# Night worklist, 2026-11-01 (unattended, ~8 h)

> **Closed (history)** 2026-11-02 06:15; result in HANDOFF-LOG (23). Written 2026-11-01 from HANDOFF-LOG (22), docs/11 phase 3 and a designer decision taken the same evening. The status
> column is updated as the night goes; the closing HANDOFF-LOG entry (23) summarises the result, and this list is then marked closed.

The backlog that does not wait on the designer: the designer's Territory decision (below), then phase 3 slice 3 of docs/11 (the
Catches, Achievements and Jobs pages), then the nightly `-Full`, which has not run since phase 3 began (last: 2026-10-31).

**Designer decision, 2026-11-01: "Territory wins."** After a Reawaken with Legacy's Territory part (`legacy` row, `extra_effects.biome`),
the new run opens in `get_starting_biome_index()`'s biome, not in the first biome. In Godot today the two travels race: Territory's
`tier_controller.gd` `_on_reawakened` schedules a scene change after `BOSS_DEATH_SEQUENCE_SEC` (a SceneTree timer, which runs while the
tree is open and would swap the scene out from under the open tree), and `hud.gd` `_return_to_first_biome` sends the player to the
first biome when the spend step ends (after the job picker). Read from the code; not yet run.

**Ground rules for the night** (CLAUDE.md standing rules):
- No commit, push, package or export (not asked). A difference with Godot is a port bug; an ambiguity is ported as Godot has it
  and written up as a finding (doc 01 §5 and the Godot repo's `docs/pawpocalypse-port-findings.md`).
- The Godot repo is read-only **except item 1**: the designer's decision is made there first, left uncommitted. Its `git status` is
  recorded before (`M scripts/autoload/card_manager.gd` and `?? docs/pawpocalypse-port-findings.md` were already there at 2026-11-01
  evening: **not ours to touch**) and checked after; only `hud.gd` / `tier_controller.gd` (and the findings doc) may change.
- Every page follows the slice 1/2 pattern: decisions in `PcHud` (L1 tests + sabotages, each seen CAUGHT), then the page in
  `UPawHudScreen`, then `Pawpocalypse.UI.*` tests and their game-module sabotages, each seen CAUGHT. Unaffordable or unavailable
  rows are LOCKED (`UPawButton::SetLocked`), never disabled; every page has a focus target and its rows in `PageColumn`.
- Player-facing text: FText from `ST_UI` (short, mean and funny, Godot's own strings first); data names from `ST_Data`.
- `double` for every economy value; constants from `Data/CodeConstants.csv` (a new one through `CodeConstantsMap.csv` + exporter).
- Sabotage scripts edit sources in place: no code is written while they run, never two test runs at once. Docs may be written then.
- A page that cannot be finished is left with every unfinished rule a named TODO, its finished parts green; the night moves on.
- Screenshots come from the editor build (`-game`, `paw.Tab`, `paw.Shot`), **not a new package** (packaging was not asked tonight).

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight**: no Unreal process of this project (`tools/unreal_procs.py`); Godot `git status` recorded; everyday `tools\check_all.ps1` green as the baseline | 15 min | baseline log in `build/check_all/` | **done** 23:20: every everyday gate PASS (`_night_1101_baseline.txt`); no editor running; Godot status saved (`_night_1101_godot_status_before.txt`) |
| 1 | **Territory wins, Godot first then the port.** (a) Godot: one destination for the post-Reawaken travel, `GameState.get_starting_biome_index()`, taken at the end of the spend step (after the job picker), where `_return_to_first_biome` runs now; the `tier_controller` timer travel removed or kept only for a Reawaken path that opens no tree (find every path into `reawaken()` first); comments say why (the decision, the open-modal teardown). Run Godot's own checks (`ci/check_damage.gd` has the Territory clamp check) and a fixture run showing both a Territory and a no-Territory Reawaken land where intended. (b) Record: doc 01 (the decision, dated, designer's words), the Godot findings doc. (c) Port: the reset in the realm uses `Pc::StartingBiomeIndex` at the same point (today: Territory's travel, else the first biome); the HANDOFF (22) open question closed. | 1.5 h | Godot change made and its checks green (uncommitted); `Pawpocalypse.UI.Hud.Reawaken` covers Territory and no-Territory; a sabotage of each branch CAUGHT | **done**: Godot measured first (new `tools/check_godot_reawaken_travel.py`, real HUD, 4 cases): with Territory the 1.1 s timer swapped the scene mid-ceremony and **the spend step never opened**, or Territory was ignored for tier 1 = **F20**. Godot fixed (`hud.gd` `_return_to_starting_biome`, timer removed in `tier_controller.gd`; uncommitted); Godot ci scripts/hud/damage/playthrough/mirror/tutorial PASS; the check 4/4, and 3/4 FAIL on Godot HEAD. Port: `Pc::ReawakenTravelIndex` (L1 + 3 sabotages CAUGHT), `DoReawaken`, the realm's deferred travel removed; Realm + UI tests updated, 4 realm sabotages CAUGHT. Doc 01 §5 + Godot findings F20 |
| 2 | **Catches page**: this run's cards (`PcCards` state; name, level/stack, description), the auto-pick box (only with Auto-Card owned, its setting), the empty state | 1.25 h | `PcHud` L1 + sabotages CAUGHT; page live; UI test + sabotages CAUGHT | **done**: `Pc::CardUnlockHint` (PcCards), `PcHud` `CatchesPage` + `CardNumber`; ST_Data gains CARD/ACH/JOB/SKILL names and descriptions (`gen_data_strings.py`); `UPawButton` detail line / flat / left; `Pawpocalypse.UI.Hud.Catches` |
| 3 | **Achievements page**: rows (locked / earned-unclaimed / claimed, progress), Claim, Claim all, the count on the tab badge, rewards through `PcAchievements` | 1.5 h | same as 2; a claim pays exactly once (pressed twice, Claim all after Claim) | **done**: `PcHud` `AchievementsPage`; the page (marks as discs: no star glyph); `Pawpocalypse.UI.Hud.Achievements` presses each twice |
| 4 | **Jobs page**: the current job, its effect line, what changing costs / when it can change (as Godot's job page has it); the job picker itself is phase 3 later, a named TODO | 1 h | same as 2 | **done**: `PcHud` `JobRows` (worn / owned / no node / closed / boss / Reawakens / ready); the page; `Pawpocalypse.UI.Hud.Jobs`. PawCore: 4 new L1 cases, 29 sabotages CAUGHT (1 first MISSED: a near-whole card number, fixed with a relative-tolerance case) |
| 5 | **Screenshot tour** of every live tab (editor `-game`), compared by eye with Godot's pages; differences fixed or written as TODOs | 30 min | shots under `Saved/PawShots/`, paths in the handoff | **done**: `catches.png`, `awards.png` match Godot's layout; found that the game never earned achievements (only tests did): wired into the realm (each step, progress changes, the Reawaken) + test + 2 sabotages CAUGHT. Jobs not shot (no cheat grants a job). TODO phase 6: the Achievements tab count spills past its text tab |
| 6 | **The nightly `-Full`** (~3 h+: the realm sabotage gate alone is ~100 min). Starts no later than 3.5 h before the end, whatever is unfinished; no code is written while it runs | 3 h+ | log in `build/check_all/`, every gate PASS or each failure explained | **done with one failure, fixed**: 20/21 PASS; Realm sabotage missed `hud_reawaken_realm_not_told` (test strengthened, CAUGHT) and had one unexplained clean failure of `.Realm.ReawakenTravel` (passes 6/6 alone; the runner now keeps the log). Rerun 97/97 CAUGHT, clean PASS; everyday `check_all` PASS |
| 7 | **Close** (written while 6 runs): docs 00, 05 §6.1, 09 (test and sabotage counts), 11 (phase 3 status), 01 §5 if a finding; new technical lessons into `C:\Games\Knowledge` (tagged, no project names); Godot `git status` checked against the preflight; HANDOFF-LOG (23) with the exact next step; this list marked closed | 45 min | all of it written, with measured numbers | **done**: docs 00/01/05/09/11, KB (one scene change per modal flow), Godot status = preflight + `hud.gd`, `tier_controller.gd`; HANDOFF-LOG (23). Stretch **done**: `PcHud` `CardPick` (L1 + 11 sabotages CAUGHT) |

**Order and time**: 0 -> 1 -> 2 -> 3 -> 4 -> 5, about 6 h of work, but item 6 must start 3.5 h before the end: whatever of 2-5 is
not done by then stays for the next session (1 is never cut). If everything is done early, the stretch item is the **card pick
screen's decisions** in `PcHud` (L1 + sabotages only, no page), stopped before 6 starts.

**For the designer before leaving**: close the Unreal editor (UBT and the engine gates skip or fail with it open) and leave the PC on.
