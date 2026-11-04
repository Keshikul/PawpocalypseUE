# Night worklist, 2026-10-30 (unattended, ~8 h)

> **Closed (history)** (worked 2026-10-30 23:40 to 2026-10-31 morning; HANDOFF-LOG (18) has the result). Every item is
> done, the stretch item and one more slice (6, the boss) too. Written 2026-10-30 from the backlog (docs/11 phase 2, HANDOFF-LOG (17), the named TODOs in
> `PcField`). The status column is updated as the night goes; the closing HANDOFF-LOG entry summarises the result.

The designer asked for a list of work that takes the night. The backlog that does not wait on the designer is phase 2 of docs/11:
the realm's rules in `PawCore`, slice by slice (slice 1 done, HANDOFF (16)). Phase 3 onwards depends on phase 2, the spike code
deletion waits on a commit, and F2/F3/F6/F7/F11 and Q1-Q9 are the designer's. So the night is **phase 2 slices 2, 3 and 4**, with
slice 5 as the stretch item.

**Ground rules for the night** (CLAUDE.md standing rules):
- No commit, push, package or export (not asked). No design changes: a difference with Godot is a port bug; an ambiguity in Godot is
  ported as Godot has it and written up as a finding (doc 01 §5 and the Godot repo's `docs/pawpocalypse-port-findings.md`).
- The Godot project is read-only: fixtures run in the scratch copy; its `git status` is recorded before and checked after.
- Every slice follows the slice-1 pattern (docs/11 phase 2): pure parts into `PcFieldRules` against a Godot fixture; the stateful
  parts in `PcField` pinned by rule tests from the Godot code and comments; each new assertion negative-tested by a `field_*`
  sabotage that must be seen CAUGHT. Godot's comments' reasoning is ported with each function.
- `double` everywhere, no `float` in PawCore; all damage through `PcCombat`; constants from `Data/CodeConstants.csv` (a new gameplay
  constant goes through `Data/CodeConstantsMap.csv` and the exporter, never hardcoded).
- `sabotage_pawcore.py` edits sources in place: no code is written while it runs, and never two test runs at once.
- A slice that cannot be finished is left with every unfinished rule a named TODO, its finished parts green; the night moves on.
- No `-Full` tonight: it ran green this morning (HANDOFF (17)) and would take ~4 h of the 8; each slice's new sabotages and the
  everyday gates are run instead. The next `-Full` goes on the next free night.

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight**: no Unreal editor running; Godot repo `git status` recorded; `tools\check_all.ps1` (everyday) green as the baseline | 15 min | baseline log in `build/check_all/` | **done**: everyday gates PASS (`_night_baseline.txt`); the designer's editor was still open (PID 3000, not touched), so the two L3 engine gates SKIP all night |
| 1 | **Slice 2, kittens** (`kitten.gd` 503 lines + the kitten parts of `tier_controller.gd`): pool sized to ownership, seek nearest through the spatial grid, roam / orbit / rebalance, contact damage through `PcCombat`; the card/node rules: temporary kittens, Kitten Kill Explodes, illusions + Grand Finale (incl. the Misdirection pull TODO), Kitten Foreman | 2.5 h | pure parts match a Godot fixture (a real `Kitten` node where it is pure); rule tests green; every new `field_kitten_*` sabotage CAUGHT; everyday `check_all` green | **done**: Godot fixture `kittens_fixture.gd` (real Kitten + Enemy nodes, 3,000 rows: getters, pool, orbit, pick, overcrowding, one whole hunt) matches; 11 rule tests (incl. Godot's own `check_kitten_spread`); 34 sabotages CAUGHT (2 rewritten: one did not compile, one was an equivalent mutant). Found F13, F14, F15 |
| 2 | **Slice 3, the aura and zones**: aura contact + slow, Snare / Traps / Gridlock, aura pulse, trail, echo, edge burn, damage zones, Rainbow Poop | 2 h | same as 1 (`field_aura_*`, `field_zone_*`) | **done**: `aura_fixture.gd` (getters, aura cards, aura/poop/player DPS, a real Enemy's `apply_slow`) matches; 13 rule tests; 34 sabotages CAUGHT. Found F16 |
| 3 | **Slice 4, Pounce**: cooldown, the product cap, vulnerable windows, craters, chain, kitten mimic, Insurance, refunds, the pad press edge (as a rule on the input the field receives) | 2 h | same as 1 (`field_pounce_*`); the product cap asserted at its ceiling | **done**: `pounce_fixture.gd` matches; 11 rule tests; 26 sabotages CAUGHT (2 rewritten to compile). The product cap is `PcCombat::PounceDamage`'s, already proved in phase 1 |
| 4 | **Stretch: slice 5**, the paw's and the kill's extras (Anvil Paw echo, Riddle, Rhythm, Inscrutable, Hammerfall; Burglar's Cut, Overflow, Catnip Bomb, Long Lunch, Levy, Pied Piper, Sixth Sense, Sleight of Hand), then Prism | rest | each finished rule tested and sabotaged; the rest stay named TODOs | **done, whole**: `extras_fixture.gd` matches; 14 rule tests; 36 sabotages CAUGHT (3 rewritten: an equivalent mutant, a non-compiling one, one hidden by whole-Catnip rounding). Found F17 and a slice-1 gap (the despawn recovery), ported |
| 5 | **Close**: every `field_*` sabotage run together (+ clean build PASS); everyday `check_all` green; Godot repo unchanged; docs 00, 03, 09 §7, 11 (slice statuses), the code-constant appendices if constants were added; new technical lessons into `C:\Games\Knowledge`; HANDOFF-LOG (18) with the exact next step; this list marked closed | 45 min | all of it written, with the measured numbers | **done**: see HANDOFF-LOG (18); the nightly `-Full` closed the night |

**Past the list (time was left):** slice 6, the boss (`boss_fixture.gd`, a real Boss node's op log; 11 rule tests; 31
sabotages; three Godot literals into the code constants), the PawCore halves of phase 2's exit criteria (a soak of the first
three biomes, the 320-mouse step at 0.093 ms), the sabotage build made ~20x faster, and, once the designer's editor had closed,
the DataTables re-imported, the editor target rebuilt and every gate (the engine ones too) run.

**Order and time**: 0 -> 1 -> 2 -> 3 -> 5, about 7.5 h; item 4 takes whatever is left before item 5 must start (no later than
45 min before the end). If a slice runs long, the later slices shrink, never item 5.

**For the designer before leaving**: close the Unreal editor (UBT and the engine gates skip or fail with it open) and leave the PC
on.
