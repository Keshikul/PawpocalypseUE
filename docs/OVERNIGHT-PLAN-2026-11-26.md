# Overnight plan, 2026-11-26 → 27

What runs unattended tonight, in order, and what waits for the designer in the morning. The state it starts from is
HANDOFF-LOG (158); the items are the playtest worklist's (docs/WORKLIST-PLAYTEST-2026-11-26.md).

## Rules for the night

- **Nothing leaves the machine:** no commit, push, package or Steam upload (CLAUDE.md). Everything since `38871c6` is
  still uncommitted; a checkpoint commit before the night is the designer's call.
- **No translation work** (D28). New player words are drafted in `docs/TEXT-DRAFT-2026-11-27.csv`, not shipped.
- **No balance change to `Data/`** before the designer approves the numbers (CLAUDE.md): the economy pass is measured
  and proposed only.
- **One heavy run at a time.** Before each build or engine test run:
  - no Unreal editor open (the project's tools refuse; another project's editor counts);
  - at least 12 GB of memory free.
  If either fails, switch to the engine-free work (the second list) and check again later.
- **The Ice project is never touched.** Its own test run (~16 GB) stopped two of today's `check_all` runs. If the system
  stops a run for low memory, it is not restarted on the session's own initiative (the session's rule): the night moves
  to engine-free work and the run waits for the morning.
- **Done means proven:** each item gets its tests, each assertion a sabotage seen CAUGHT, and `check_all` exit 0 at the
  end. A fault not fixed in two attempts is written down and left; the night moves on.

## The order

### Engine work (when the machine is free)

1. **Pre-flight (5 min).** No editor, memory free, `safe_mutate --list` clean.
2. **`check_all` (~45 min).** Proves P4.2, P4.7 and P4.5, which so far stand on their own test runs. Fix what fails.
3. **P4.8, the narrower dock (~1.5 h; D-j: 568 → 480).**
   - The dock width read from one place; the page and button wraps that follow it.
   - `--graft props:DockBox` so the asset matches.
   - Checks: `Layout.Dock`, `Layout.Languages` (23 languages at 1280 x 800, 150 % UI), `PseudoLocale`, `TextMisses`.
   - The screen tour.
4. **P4.1, one tooltip for Upgrades, Achievements, Catches and the tree (~4 h; C1's approved tooltip look, the approved
   `UPG_*_desc` and `TIP_*` words).**
   - A `UPawTooltip` piece (baked once): icon, name and level, one line, Now → Next as two big numbers, a cost chip, one
     chip when something blocks the buy.
   - PawCore `Pc::UpgradeTip` (now, next, price, affordable, maxed) with L1 tests.
   - Upgrades and Catches rows; the tree's tooltip moves to it (`Pc::TreeTip` shortened).
   - Achievements: rows made hoverable and focusable; reward, progress bar, state.
   - "How to earn" lines are new words: drafted for the morning, not shipped.
   - Placement beside the dock on the field side, kept on screen; hover and pad selection.
   - Tests and sabotages; the screen tour.
5. **P4.3, Boba (~3 h).**
   - The first try's 14 cut-outs copied (read only) into `SourceArt/boba/` (LFS) and imported.
   - The title's card straight, the photo in the well, a hover pop.
   - A click starts a cameo: `PcBoba` for timing and paths (flies across, spins, rainbow trail, peeks, drops in) and
     `UPawBobaSubsystem` to draw it.
   - Where: the title, pause, settings and victory screens, every 25-60 s. Never over the skill tree. Off with effects
     off.
   - Tests and sabotages; the screen tour.
6. **`check_all` again** at the end of the engine work.

### Engine-free work (any time the engine is blocked, or after)

7. **E0 and the economy proposal (~3 h).**
   - pawsim learns the income cards (edge burn, Bounty, Notoriety, Thunderclap, Ember Trail, Gilded Kitten) for a typical
     pick order, uncapped levels included.
   - The sweeps for E1-E6 together, aiming at the designer's targets:
     - "We should be able to afford a kitten at 15 min";
     - "We should be the strong one untill we get a few kittens".
   - `docs/ECONOMY-REVIEW-2026-11-27.md`: one recommended set with its milestones, waits and the cat's share of
     damage, and the alternatives. `Data/` is untouched.
8. **Concept sheets C3-C6 (~1.5 h)**, for picks in the morning:
   - C3: the signatures' edge (today, a fainter glow, none at 80 px, another mark; moth, dogs, goose);
   - C4: the Engineer's hat from the side;
   - C5: the Summon button, and the boss at 1.4 / 1.8 / 2.2;
   - C6: a kitten icon for Extra Kittens.
   Review PNGs in `build/review/`, gathered on one page.

### Morning report

The handoff entry for the night:
- what was built and its proof;
- what failed and why;
- the decisions waiting.

## Waiting for the designer in the morning

- The new words from P4.1 (`docs/TEXT-DRAFT-2026-11-27.csv`).
- The economy proposal (E1-E6).
- C3-C6 picks.
- A commit, and the Steam uploads:
  - the store page pack (with E2's Summon and the 83-skill tree);
  - the achievements file ("Big Family").

## Not tonight, and why

- **P4.6, the Catches page:** lands with E4 (uncapped cards changes its totals).
- **P4.4, the tutorial:** after E2 (its boss step depends on the new boss rules).
- **E1-E6 themselves:** they need the designer's yes on the numbers.
- **D-c's three skill removals (Audit, Deep Pockets, Auto-Bait):** with E2; one line in `retired_branches` then.
