# Night worklist, 2026-11-12 (unattended, from the designer's go to about 08:30)

> Written 2026-11-12 22:00 from HANDOFF-LOG (103) (designer: "Make sure all docs are up to date. Commit and push. Create an
> overnight plan"). **Waiting for the go.** The closing entry will be HANDOFF-LOG (104).

The night proves D17, committed and pushed this evening, with the nightly `-Full`, which D17 has not had yet: 40 gates,
about 10 h, every PawCore and realm sabotage (662 and 563), the long suite, the pawsim parity sweep, the Godot fixtures and
the pad tour. While it runs, the desk work measures the one item D17 left open: **something new on every late Reawaken**
(a tree price pass, a measured proposal; nothing changed). A player sees no change tonight.

The `if` audit's fixes (HANDOFF-LOG (103)) are a choice at the go. They change no rule, number, word or look, but they edit
source files, and the source tree is the sabotage runs' workspace: they go either before the nightly (which then proves them
too, and the night ends about two hours later) or tomorrow.

## Before the go (the designer)

1. **Close the Unreal editor and leave it closed until the closing entry says it may be opened.** A sabotage run builds
   broken DLLs on purpose until its clean rebuild; an editor opened in between loads them.
2. **Leave the PC on, signed in and unlocked, with this session open.** The night holds a keep-awake request from a
   background process (`build/night/keep_awake.ps1`, as on 2026-11-10; no power setting is changed; it ends with the night).
3. **Say whether the audit's fixes 1-3 go in tonight.**
   - **No (recommended)**: the nightly starts at the go on the pushed commit and proves exactly what is on GitHub; the
     editor is free about 10 h after the go (a go at 22:30: about 08:30), in time to play D17. Fixes 1-3 are made tomorrow
     with their proof, and the next nightly covers them.
   - **Yes**: steps 1-3 first (about 2.5 h), then the nightly on the end state; the editor is free about 12.5 h after the
     go. The fixes stay uncommitted until you say.

## Ground rules (CLAUDE.md)

- No commit, push, package or export tonight. The Godot repo is read only (the fixtures are made from a scratch copy).
- The night starts from the pushed commit, so `git diff` is the night's whole change. **Never `git checkout`, `restore`,
  `stash` or `reset`**: an item is undone by writing its files back with `git show HEAD:<path>`.
- One test run at a time. While `check_all`, a sabotage run or a build is going, only `docs/` and `build/` are written (a
  sabotage run edits the source tree in place). The desk work reads `Data/` and `tools/` and writes `build/night2/` and
  `build/econ/` only; its levers live in a script under `build/night2/`, never in `tools/`, `Data/` or `Source/`.
- Fixes (only with the go's yes): no behaviour change. Proof: every existing test passes unchanged; each sabotage whose
  target text a fix moved is retargeted and seen CAUGHT alone (`--only`); a guard a fix keeps or moves gets a sabotage, and
  if no test catches it, a test is written first and seen failing. A test is never weakened.
- An Unreal process of this project appears (`tools/unreal_procs.py` exit 1): no further build, the state is written, the
  night stops. Another project's UnrealBuildTool running: wait for it before a build (KB U/24, `ConflictingInstance`).
- A failure whose cause is not clear within the hour is written up and its item undone; the night moves on. A nightly gate
  that fails keeps its log; its cause is looked for from the logs and the code (no second test run during the nightly) and
  written up for the morning. Nothing is "fixed" by weakening a test.

## The night

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight**: `tools/unreal_procs.py` exit 0; other Unreal processes listed; the keep-awake on; both repos' `git status` saved (`build/night2/status_before_{port,godot}.txt`); the port tree is the pushed commit | 5 min | all true | |
| 1 | *(yes only)* **Fix 1, Overkill checked once** (table below) | 30 min | L1 green; its sabotages CAUGHT | |
| 2 | *(yes only)* **Fix 2, the HUD's refreshes checked once** | 1 h | `Pawpocalypse.UI` green; its sabotages CAUGHT | |
| 3 | *(yes only)* **Fix 3, card requirements as an enum**; then the everyday `check_all` on the end state | 1 h + 6 min | the same; `check_all` exit 0 | |
| 4 | **The nightly**: `check_all.ps1 -Full`, in the background (`build/check_all_full_20261112.log`); the session wakes when it ends | ~10 h | 40 of 40 PASS, exit 0 | |
| 5 | **Desk work during 4: the tree price pass** (below) | 2-3 h | the proposal and its numbers in the economy review; doc 01 Q13 | |
| 6 | **Close**: HANDOFF-LOG (104) (each gate's result and time, the fixes if made, the proposal, whether **the editor may be opened**, the next step); doc 00; doc 09's last `-Full`; this list closed; the keep-awake stopped | 20 min | written | |

**Order and cut lines.** With the yes: 0, 1, 2, 3, 4 (5 in 4's wait), 6. With the no: 0, 4 (5 in its wait), 6. A fix not
proven by 01:00 is undone and the nightly starts anyway: the nightly is never cut for a fix.

## The fixes (only with the go's yes; evidence in HANDOFF-LOG (103))

| Fix | The change | Proof |
|---|---|---|
| 1 | `PcField.cpp`: the 7 `if (... Overkill > 0.0)` around `SplashOverkill` go (`OverkillDamage` and `SplashOverkill` test the same thing again). `SplashOverkill`'s own guard becomes `if (!(Damage > 0.0)) return;`, so a NaN still stops there as it did at the callers | L1 204 of 204 unchanged; the sabotages on `SplashOverkill`, `OverkillDamage` and the one anchored on a removed guard (`Chain.Overkill`) retargeted where needed and CAUGHT; a new one on the kept guard |
| 2 | `PawHudScreen.cpp`: `RefreshNow` looks the realm and the game up once and returns while the data is not ready; the nine refreshes (`RefreshStatus`, `RefreshTabs`, `RefreshField`, `RefreshShop`, `RefreshRealms`, `RefreshReawaken`, `RefreshCatches`, `RefreshAchievements`, `RefreshJobs`) take what they read as arguments; `RefreshJobs`' other caller passes its own | `Pawpocalypse.UI` all green; every realm sabotage on the nine functions (`--only`; counted at the night) CAUGHT, retargeted where its text moved |
| 3 | `PcCards`: `enum class CardRequirement` (`NONE`, `AURA`, `POUNCE`, `OVERKILL`, `POOP`, `KITTEN`, `KITTEN_POUNCE`); `CardEffectRequirement` and `CardCostRequirement` return it; `CardRequirementMet` and the HUD's `CardRequirementKey` switch on it; a `CardRequirementName` keeps the strings wherever a Godot fixture compares them | L1 (the cards fixture unchanged); a new test: every value but `NONE` has its own hint key, not "other" (MSVC's missing-case warning C4062 is off by default, so the test covers it); a misspelled requirement no longer compiles; the sabotages on the old chains retargeted and CAUGHT |

Not tonight: fixes 4 (`RebuildStats`' level-gated multiplies) and 5 (the shared refocus and lookup helpers): smaller gains,
and fix 5 moves many realm sabotages' targets. Tomorrow, on the designer's word.

## The tree price pass (desk work, step 5)

- **The question.** D17 met the review's goals except one: something new on every late Reawaken. In D17's measured run
  (`build/econ/today.json`, 13 Reawakens) runs 8, 11 and 12 buy no new tree node, only ladder levels; the last new nodes
  (Critical Mass, Full Spectrum) come in run 10. The review said to price the tree "once the chosen economy's Stardust curve
  is known": it is known now.
- **The method.** From the run's tree log, each run's Stardust and what it buys; then candidate prices and Reawaken gates
  for the nodes still unbought late (the review names Full Spectrum, Second Helping, Curiosity and the jobs' rule-changers),
  applied in memory through `econ_review.py`'s levers from a script in `build/night2/`. Kept fixed: D17's rules and
  Stardust curve, the validator's rules on the tree's data, the game's length within about 10 % of 112 min.
- **The output.** A "Tree price pass (proposal)" section in `docs/ECONOMY-REVIEW-2026-11-12.md`: per run, what is new today
  and with the proposal; the game's length, the waits and the longest stretch with nothing new, against D17; and a new
  question Q13 in doc 01 for the designer. Nothing in `Data/` changes.
- **Its limit.** pawsim never buys jobs, and their rule-changers are part of what a late Reawaken can open; the proposal
  says which nodes it could not measure.
