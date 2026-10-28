# 02 - Game design handoff: the whole game, every rule

> Consolidated on 2026-10-28 from the Godot project's `pawpocalypse.md` (its standing rules, re-checked against
> code 2026-09-30), `docs/pawpocalypse-gdd.md` v9 (2026-10-01/02), `docs/pawpocalypse-technical-design-doc.md`, the
> autoloads and `/data`. The numbers live in `Data/*.csv` and [appendix A](appendix-a-content-catalogue.md); this
> doc says what they MEAN and which rules may never break. **Every "never" here has a measured incident behind it in
> the Godot history.** When porting a system, read its section here, then the Godot code (appendix B), then write
> the C++.

---

## 1. Identity

- **Genre**: semi-active idle/incremental cat-sim with a permanent skill tree, a per-run card layer and a finite
  ending. 2D, a fixed diorama per biome (no scrolling camera). Windows first (Steam; itch today), built to port to
  PlayStation and Xbox: **keyboard/mouse AND gamepad from day one**.
- **Pitch**: you are a small, malevolent cat. You catch mice. You get stronger, the enemies get weirder, and by the
  end you are an armoured cat riding a unicorn through the Litterbox Dimension.
- **Tone is load-bearing.** The cat is mean, not cuddly. The comedy is the gap between the sparkly presentation
  (Rainbow Poop, Stardust, unicorns) and the animal's contempt. Keep the sparkly vocabulary. **Never name anything
  cutely**; names read like a spiteful animal's idea of fun (Giblets, Rigor, Spite, Bloodlust, Rainbow Gut). The art
  is chubby and big-eyed (the "sweet" grammar): that is the FORM; expressions stay derpy and contemptuous.
  Store/public copy: no "spiteful cat" phrasing (designer), keep player text short.
- **Length is an outcome, not a target** (designer, 2026-10-01). pawsim: the Litter King at ~6h 11m after 9
  Reawakens; the 5x soak of a build: ~10h game time, 10 Reawakens. Never tune toward a number of hours; the fix for
  length is content.
- **Pillars**: escalating absurdity with serious systems; semi-active (kittens hunt alone, but aiming, Pounce timing
  and Bait beat idling); a line, not a treadmill (it ends); readable power (every buy should be NOTICEABLE, ~8%+ of
  income); no offline progress (removed 2026-09-10, a decision, not a gap).
- **Boba**, the designer's real cat ("CEO and General Manager of everything"), appears **in menus only**, never in
  play and never over the skill tree.

## 2. The core loop

1. **Hover** over mice. The **paw** strikes the mouse under the cursor at its attack speed (discrete hits, real crit
   rolls). Once bought, the **aura** damages everything inside it continuously.
2. Kills pay **Catnip** and **XP**. A mouse not killed wanders off and despawns: no penalty.
3. Spend Catnip on **fifteen shop lines** and on **Bait**.
4. Each XP level deals **three cards, keep one**.
5. Earn Catnip this run to open the next biome; beat bosses to open the next block of biomes.
6. **Bait a boss** in its biome; beating it raises the **evolution stage**.
7. **Reawaken**: trade the run for **Stardust**, spend it on the permanent tree, start again further along.
8. Kill **the Litter King** in biome 12: the victory screen, the end.

## 3. Combat

**No player HP, no fail state, no territory line, no knockback.** Never reintroduce them. The only loss in the game is
a boss timer running out.

**All damage is calculated in ONE place** (Godot `CombatResolver`, here `PawCore::Combat`). Callers decide WHO is hit;
the resolver decides HOW MUCH. It is stateless: it returns DPS numbers and callers apply `dps * dt`.

| Source | Rule |
|---|---|
| **Paw** | Discrete strikes on the mouse under the cursor at `strikes_per_sec = base_attack_speed (2.0) x attack_speed_multiplier`. Strike = `base_paw_damage (4.0) x paw_damage_multiplier x shred x damage_scale`. Real crit roll per strike (`crit_multiplier` 2.0). The swipe draws an ink-edged arc and three claw slashes. **Shred**: consecutive strikes on the same mouse add `shred_per_stack` each, up to `shred_max_stacks` (10), the streak decaying after `shred_stack_seconds` (2.5). The resolver is told the stack count; the realm owns the streak. Projections count shred at HALF its ceiling. At most 12 strikes resolve per frame |
| **Aura** | Continuous damage inside a radius (`base_aura_radius` 77 px, `base_damage_per_sec` 8). **Exactly 0.0 until the Aura node is bought**, deliberately zero rather than small ("you do not have this yet" must not read as "this is weak") |
| **damage_scale** | Shared by paw and aura on purpose: `evolution_multiplier x weapon_multiplier x (1 + additive_sum) x multiplicative_product x reawaken_multiplier`. Claws applies to both; splitting it made Claws worthless from Reawaken 2 on |
| **Kittens** | Real pooled entities (not an abstract DPS). Each seeks the nearest mouse within `base_kitten_detection_radius` (500) and deals continuous contact damage (`base_kitten_damage_per_sec` 5, contact radius 30). Scale with evolution, **never with the weapon multiplier**. With nothing in range they **roam** (slower than hunt speed, so "spotted something" reads with no other cue): a frozen kitten reads as broken AI. The pool **grows to what the player owns** (it was once sized from the base cap and a deep run paid for invisible kittens). Kitten share of damage: ~43% at ten minutes, ~6.5% at the end; it must never reach zero |
| **Pounce** | Active burst (LMB / RT) worth `pounce_damage_seconds` (2.0) **seconds of player DPS** (never a flat number: the curve spans ten orders of magnitude), radius 160 px, cooldown 5 s, x`pounce_vulnerable_multiplier` (5) inside an enemy's vulnerable window. **Capped on the PRODUCT** of its dials at `pounce_max_aura_multiple` (2.0) x aura DPS (uncapped it measured 90.6% of all damage); the timing bonus sits OUTSIDE the cap. The cooldown is a clock disc in the field's bottom corner opposite the dock. Pounce slams the floor and leaves a crater zone (3 s). On a pad it fires on RT's press EDGE only |
| **Rainbow Poop** | Periodic AoE ground zone (`poop_cooldown_sec` 8, duration 3, radius 250, `poop_aura_fraction` 0.35 of player DPS), its own node under the Unicorn |
| **Overkill** | A killing blow's WASTED damage splashes to the **nearest** survivor and **never chains** (late-game per-frame damage dwarfs whole HP pools; a chain would clear the board every frame). `take_damage()` returns the excess. The **Overkill bounty** pays Catnip for damage with nowhere to go, priced in the victim's own reward, exponent 0.4, capped at `overkill_bounty_max_multiple` (4x) |
| **Prism** | Beams between targets; a beam kill pays a Spectrum bonus (the Prism chain under the Unicorn) |
| **Execute** | Capped additive shop line: kills below a HP fraction outright |

**Anything priced off the player's damage reads `player_dps()` (paw + aura together)**, never `aura_dps()` alone,
or it is worth nothing before the Aura node.

**Brackets stay separate.** "+X% damage" effects SUM into one additive bracket; "xN damage" effects MULTIPLY on top.
Collapsing them is the single easiest way to break the economy. Same split for kitten and poop damage.

**The rule learned four times:** *a term that does not track the main damage curve does not stay small, it
disappears.* Rewards track enemy HP; kitten damage tracks paw damage; poop and Pounce track player DPS; the
Overkill bounty is priced in the victim's reward. Check every new term against it.
**And its mirror:** *a ladder that gets shorter loses its top unless you re-spread it* (the evolution base is
re-spread so stage 7 stays x128).

### 3.1 Enemies

59 enemies (appendix A). Fields: `hp`, `reward_catnip`, `movement`, signature shields, flee/seek, speed, lifetime,
sparkle, size.

- **`reward_catnip = 0.0433 x hp`** for every enemy (`CATNIP_PER_HP`). v1 let rewards grow x1.35/tier against HP
  x3.33/tier: a 551x divergence that made the game mathematically uncompletable. Tier 1 is the one hand-authored
  exception; the **flier premium** (`1 / FLIER_CATCH_RATE`, 0.45) is the one legitimate departure (it buys back a
  measured miss rate). Any HP change re-derives rewards in the same pass.
- **Enemy HP lives only in `EnemyData.hp`**; no copy on the biome.
- **Movement verbs (append-only enum)**: `GROUND` wanders, is slowed by the aura and takes ground-zone damage.
  `FLIER` crosses in a straight line, ignores slows and takes **nothing** from ground zones (passive coverage cannot
  farm it; it asks the player to move the cursor somewhere for a reason). Chargers, splitters, ranged are named,
  undesigned.
- **Signature enemies** (tier 3 up) carry periodic invulnerability windows (`invuln_vulnerable_sec` /
  `invuln_shield_sec`) and a white glow. `take_damage()` is a no-op while shielded, less what Shield Pierce buys.
  This, not "more HP", is what differentiates signature enemies and boss phases; it was asked and answered once.
- **Golden mice** spawn at the biome's `golden_chance` (raised by Fortune, clamped at `MAX_GOLDEN_CHANCE` 0.35), flee
  and time out: hard to catch, not to kill.
- **Gristle** enemies (one per biome pool), **sparkle**, `reward_exempt` and `seeks_player` flags: see
  `Data/EnemyData.csv`.
- At most **320 live enemies** (`MAX_LIVE_ENEMIES`); spawns enter from the field's edges within ~52 degrees of dead
  centre, 24 px inset.
- **Everything that walks stands on a light ground shadow** (fainter under fliers), all shadows on ONE layer under
  every creature (drawn between creatures they cost +416 draw calls in Godot).

## 4. Biomes, gates, waves, bosses

| # | Biome | Stage needed | Spawns/min | Pays x | Boss |
|---|---|---|---|---|---|
| 1 | Backyard | 1 | 120 | 1.00 | |
| 2 | Alley | 1 | 66 | 1.40 | |
| 3 | Neighborhood Park | 1 | 69 | 1.40 | The Park Tyrant |
| 4 | City Rooftops | 2 | 162 | 1.40 | |
| 5 | Sewers | 2 | 192 | 1.57 | |
| 6 | Rival Cat Turf | 2 | 228 | 1.76 | The Rival Champion |
| 7 | Portal Zone | 3 | 270 | 1.97 | |
| 8 | Mythic Realm | 3 | 324 | 2.21 | The Void God |
| 9 | The Neighbour's Kitchen | 4 | 337 | 2.51 | |
| 10 | The Vet's Waiting Room | 4 | 350 | 2.81 | |
| 11 | The Pet Food Factory | 4 | 363 | 3.11 | The Foreman |
| 12 | The Litterbox Dimension | 5 | 376 | 3.41 | The Litter King |

- **A biome is a weighted mix** of enemies from several tiers (`spawn_pool` + `spawn_weights`), not one common and
  one signature.
- **The spawn rate is the biome's number times the player's levers, nothing else**: `spawn_rate_per_min x
  spawn_rate_multiplier` (Spawn Rate line, Swarm, Infestation, the Ratcatcher job, card costs). **The cat's damage
  never touches it** (a DPS-coupled spawner was measured and withdrawn 2026-09-14). Every biome from the Alley on
  spawns strictly faster than the one before; the Backyard is the one exemption. No biome pays less per kill than the
  one before. `reward_multiplier` is the one term SUPPOSED to diverge from the HP curve (bounded, not derived).
- **Two independent gates open a biome**: `required_evolution_stage` in BLOCKS (1-3 stage 1, 4-6 stage 2, 7-8
  stage 3, 9-11 stage 4, 12 stage 5: a boss kill opens a whole block, an event) and `catnip_to_unlock` measured
  against **`lifetime_catnip_this_cycle`** (monotonic, so "bought once" needs no ledger; resets on Reawaken).
  Travelling backwards is never gated. Passage past a boss's biome asks whether that boss was beaten THIS run:
  **passage re-locks every cycle, power never does.** A flat Catnip gate paces only the first run or two (income grows
  5-10x a run); biomes 10-12 are priced at ~3 minutes of income at the first Void God kill.
- **UI must say what to DO**: a locked Realms row names the boss blocking it or the Catnip still to earn, never an
  abstract "Stage 2".
- **Waves**: each biome escalates against itself (`kills_per_wave`, `wave_kill_growth`, `wave_hp_growth`,
  `wave_reward_growth`; `MAX_WAVE` 64): stay and the mice get tougher and pay more. Wave progress is saved.
- **Bosses** live in `GameState.BOSS_REALMS = [tier_3, tier_6, tier_8, tier_11, tier_12]`, **the progression ladder**:
  every other progression number derives from that list, never from a tier number. There are no boss arenas.
- **Bait**: a boss comes out when you hold **40 Bait** (`bait_required`; dropped by kills in its own biome every
  `kills_per_bait` 240 / 811 / 1,500 / 2,200 / 2,800) **and pay its Catnip price** (`bait_cost_catnip`), charged per
  summon including rematches. The 40 never changes: the player gets faster against a fixed yardstick. Bait is on the
  Realms page **in every biome** (outside a boss biome it names where the next boss is); when a summon is in hand the
  button leans and the zone plate reads "Bait ready" in its rainbow frame.
- **Boss fights**: HP phases (`phase_hp_thresholds`) with vulnerable/shield windows, adds that heal the boss
  (`add_*`), a **time limit counted from first ENGAGEMENT**. Running out sends the player back one biome AND switches
  auto-travel off (or it would teleport them into the fight they just lost). A refused summon must not eat the bait
  (a real bug). **Boss HP sets WHEN a gate opens, not how long a fight lasts** (the sim engages at the earliest
  winnable moment): never tune boss HP by watching fight duration.
- **`bosses_defeated` (biome id -> bool) is the ledger.** Keys stay biome ids forever (re-keying un-clears saves).

## 5. Evolution

| Stage | Granted by | The cat | weapon x |
|---|---|---|---|
| 1 | start | Tiny Kitten | 1.000 |
| 2 | Park Tyrant | Armored Tomcat | 1.067 |
| 3 | Rival Champion | Cyber Cat | 1.133 |
| 4 | Void God | Rainbow Cavalry (on a unicorn) | 1.200 |
| 5 | The Foreman | Alicorn (wings) | 1.267 |
| 6 | Litter King (5th clear) | Void Unicorn | 1.333 |
| 7 | the same kill: `final_boss_defeated` | The Last Unicorn | 1.400 |

- `get_evolution_stage()` counts **CONSECUTIVE** clears down `BOSS_REALMS` (a cheat kill of a late boss cannot skip
  the ladder); stage 7 adds `final_boss_defeated`. **Nothing else grants a stage**: not the tree, not Reawaken.
- `evolution_damage_multiplier = evolution_damage_per_stage ^ (stage - 1)`, base **2.2449** (data), stage 7 = x128,
  computed live, never stored. Re-spread the base whenever the ladder's length changes; a test asserts the ceiling.
- **The mount follows the Unicorn node, not the ledger**: `get_visual_stage()` is at least 4 once `unlock_unicorn` is
  bought. From stage 4 the MOUNT evolves while the fat grey tabby on its back never changes (the joke).
- Weapons: a per-stage overlay (`player_weapon_stage_<n>`, art for 2-7), drawn leaning up-right and **mirrored in the
  paw** so it points away from the left-facing cat; held over the unicorn's back when mounted. `weapon_type` is
  authored and read by nothing.
- Hats (jobs) are big, two-thirds of the cat's width, with a cream sticker outline, drawn above the weapon.

## 6. The shop (Catnip, resets on Reawaken)

Fifteen lines, **not uniform** (`UPGRADE_PATHS`):
- **Multiplicative**: Paw Damage, Attack Speed, Aura Radius, Currency Find, Kitten Power, Kitten Speed, Pounce Power,
  Spawn Rate.
- **Capped additive fractions**: Crit Chance, Shred, Execute, Overkill, Shield Pierce, Pounce Recovery.
- **Capped additive integer**: Kitten Count (cap grows two kittens a biome and half a kitten a Reawaken; first kitten
  150 Catnip, each next x1.2).
- **Hard rule: uncapped multiplicative lines must have `cost_growth > effect_per_level`** (marginal value per Catnip
  scales as `(effect/cost)^level`; Paw Damage at 1.12/1.08 drove DPS to 2.4e10). Their summed return exponent stays
  under 0.80, or the economy reaches infinity in finite time. Capped lines are exempt.
- **Caps rise with depth**: `cap_per_floor` per biome reached, `cap_per_reawaken`; never applied to cost growth.
- **Prices never scale with power**: a price is a function of level alone (the Reawaken requirement is the one
  exception).
- **Branch-gated lines are NOT SHOWN until bought** (`BRANCH_GATED_UPGRADES`): Pounce Power/Recovery need
  `unlock_pounce`, Overkill needs `unlock_overkill`, Aura Radius needs `unlock_aura`. Not shown-and-disabled: a row the
  player cannot act on is noise. Kitten Power and Speed are locked until one kitten is owned.
- **Bulk buying** (x1/x10/xMax, Q / X cycles): a bulk buy equals N singles exactly; preview equals charge; caps
  refuse rather than charge; `BULK_MAX_STEPS` 500; one save per frame however many levels.
- **Overkill, Kitten Speed, Shield Pierce and Pounce Recovery are not sustained-DPS terms**: raising their caps does not
  scale DPS.
- The shop **does not scroll**: all lines stacked; when it overflows, make rows fit.
- The step between buys is set by `tools/shop_step.py` (raises effect and growth to the same power: same curve,
  different buy size); a buy under ~8% of income is the complaint of 2026-09-14.

## 7. Cards (the choice layer, per run)

- Kills grant **XP**; each level deals a hand of **3** (`BASE_HAND_SIZE`, jobs can add) from **28** cards and the
  player keeps one. XP to next level `24 x 1.68^level` (`LEVEL_COST_BASE`/`GROWTH`; identifiers say `catches_*`,
  the player sees XP/LEVEL). A run offers ~13 picks.
- **The deck keeps 4 cards** ("keep 3" meant the pick screen).
- **Everything about cards resets on Reawaken**; permanent power is the tree's job.
- **Seven flavours** (Aura, Kitten, Pounce, Swarm, Zone, Fortune, Deck), four cards each, exactly one with a cost per
  flavour (a test asserts the shape: "the kind of rule that erodes one convenient addition at a time").
- **Six cards carry a COST** (`CardData.Cost`), 22 are pure upside. **`Cost.KITTEN_COUNT` is the only cost that
  subtracts whole units**: eligibility prices the delta and requires one kitten left over.
- **Cards only offer what the player has unlocked** (no Pounce card before Pounce).
- Card levels: taking a card you hold levels it (`level_values`).
- Auto-pick is a tree node (Auto-Card), off in a new game.
- Three effects are implemented with no card (`KITTEN_DRAG`, `KITTEN_FROM_SIGNATURE`, `OVERKILL_TARGETS`): port the
  effect code, keep the "no card" list in the test.

## 8. Currencies and Reawaken

- **Two currencies, one job each. Never a third.** **Catnip** (kills; shop and Bait; resets on Reawaken less Legacy
  carry-over). **Stardust** (Reawakening and achievements only; buys the tree; never resets).
- **Reawaken resets** Catnip, shop levels, cards, XP, this run's passage and the job pick; **keeps** the tree, the boss
  ledger and the evolution stage. Grants `floor(0.125 x sqrt(lifetime_catnip_ever)) - stardust_granted_ever`.
- **`reawaken_multiplier = (1 + stardust_granted_ever / 2.5) ^ 0.5`**: soft-capped, keyed to Stardust **EVER
  granted**, never the balance (spending must never lower it). Not the old `k ^ points`.
- The first Reawaken is gated (`reawaken_min_stardust` 10); after that whenever the player likes; the game SUGGESTS
  when (the tab's star waits for the suggestion, `reawaken_min_gain_fraction`). The Reawaken tab states the payout
  BEFORE the button: progress, pending Stardust, multiplier before -> after. Pressing arms a confirm ("Ask before
  Reawakening"), then plays the Reawaken ceremony and hands the player to the tree in spend mode. **Auto-Reawaken is
  ruled out**: it is the one decision the loop asks of the player.
- **Legacy** is the only thing that changes how a run STARTS: Catnip carry-over, a floor under every shop level, a
  starting biome (clamped to what the boss ledger earned). Applied once inside `reawaken()`.
- The word **"Prestige" is never used**, in code or UI.

## 9. The skill tree (Stardust, permanent, NEVER resets)

- **87 nodes on six limbs of a hex lattice**; trunks from the hub: `instinct`, `claws`, `unlock_aura`, `fortune`,
  `unlock_unicorn`, `unlock_snare`. Every other node hangs off a parent and **needs its parent OWNED**
  (`parent_is_required`).
- **Unlocks** (one level) hand over a verb or rule; **ladders** (`x/N`) move a number, each level REPLACING the last
  (not stacking). Merged nodes carry both brackets (`claws`: additive in `level_effects`, multiplicative in
  `extra_effects.mult`; `fortune`: yield + Golden odds; `legacy`: the run accelerators).
- Limbs: **Claws** (paw damage both brackets; Pounce, Auto-Pounce, Overkill, jobs), **Instinct** (kitten damage,
  Vitality, Reputation, the three compounding nodes), **Aura** (the aura, power, reach), **Fortune** (yield, Golden,
  Legacy, automation: Auto-Travel, Auto-Bait, Auto-Card), **Unicorn** (the mount 2,500 Stardust with a ceremony and a
  rainbow pulse; Rainbow Poop 600; Rainbow Gut; the Prism chain), **Snare** (slows: Deadweight, Funnel, Play With
  Food, Swarm).
- **Nothing but a new game resets the tree.** **A node the layout does not seat cannot be bought**: a test asserts
  every data node is drawn and nothing is drawn without data (the v6 tree shipped Legacy unbuyable while every gate
  passed). Layout: even spacing by construction, no link crossing a node or another link (both measured).
- Node keys are `"<branch_id>_<level>"`: **parse from the RIGHT** (`aura_power_3` parsed from the left was the
  Reawaken crash).
- Visual states: three (owned, buyable, locked); rainbow rings on verb unlocks, gold halo on buyable now, a single
  gold ring on job nodes, the Unicorn pulses rainbow. Played from keys and pad: every node reachable from where the tree
  opens, Q / X jumps to the next affordable node, right stick pans, RT/LT (= / -) zoom.
- `*_unlocked()` = the tree will sell it; `*_owned()` = bought. Shop rows key off owned; the tree keys off unlocked.
  The Reawaken-count gates exist and are all 0.

## 10. Jobs

Ten jobs, each a tree node with a gold ring that hands over a JOB: a trade worn for a run (multipliers on
paw/aura/kitten/pounce/all damage, Catnip, crit, attack speed, enemy health, spawn rate, extra cards, pounce lock),
picked once at the start of a run. Beating a boss opens the gate on its job's node (`dropped_by_biome` /
`gated_by_boss`); Stardust buys it. Each job has a ladder, a verb and a rule-changer under it. **Jobs have never been
judged as a system by the designer**: port them exactly, do not "fix" them.

## 11. Achievements

41, paying **1-12 Stardust each, claimed by hand**. A new achievement is a data row, nothing else (every metric reads
an existing counter). The list refreshes only while its tab is open. **If a system pays the player, a test asserts
the BALANCE moves** (achievement Stardust was never granted for a whole version while every gate passed). Steam
achievements later mirror these (doc 10 §5).

## 12. The ending

The Litter King dies in the Litterbox Dimension, stage 7 is granted, the **victory screen** shows the first-clear time
(`final_boss_seconds` records only the first; a rematch is not a faster run). A cleared boss can be re-baited.

## 13. Presentation and UX rules

- **Layout**: the realm fills the window; the **dock is on the LEFT** (flippable in Settings > Controls > Menu side)
  with six icon-only tabs that fit one row: Upgrades, Reawaken, Realms, Catches, Achievements, Jobs. **Every menu is a
  tab in the dock** (no floating panels). The **status readout** (currencies, zone bar) sits top-right, mirrored
  opposite the dock. Currency never goes inside a tab.
- **Badges mean something NEW** (Realms: a biome open by both gates and deeper than any reached this run; Jobs: unseen;
  Reawaken: the SUGGESTED Reawaken; Achievements: a star by its count). The selected tab survives a biome change.
- **The skill tree is its own full-screen page** (from the Reawaken ceremony, the quick-access bar or T). **Settings
  is a popup** (Audio / Video / Controls). **Pause menu** on Esc/Start.
- **Title screen**: five save slots ("played 3 days ago"), a garden scene, Boba, Report a bug (opens the itch
  community board; later Steam discussions), Discord. Pre-slot saves migrated once.
- **The cursor**: over open field the OS pointer hides and the cat IS the cursor; over the dock, popups or a paused
  game the pointer is the generated paw (open; closed with claws while the button is held).
- **Tutorial**: a paw and one line. Eleven cues in order `card, tree, boss, reawaken, bait, travel, kitten, buy,
  claim, pounce, hover`; one cue at a time; the game's paw tapping one control and one short verb-first line. Never
  pauses, never takes a click or focus. A step is DONE when the game state says the lesson was learned (old saves and
  fast players skip ahead). Progress in the save (`tutorial_done`). Switch: Settings > Controls > Tutorial tips. Off
  in the soak, trailer and store shots. The lines are a draft awaiting the designer.
- **Paper look**: Stacklands colours; panels cream `#FFF9E3`, 3 px `#1A1A1A` frame, ink text, 6-8 px radius. **An
  enabled button is the brightest thing in its panel**; disabled dims. All four button states share one silhouette.
  Focus draws a ring, not a plate. Dimming text on paper is a colour change, never a multiply (it makes ink blacker).
  Every colour from the palette, generated (doc 07 §3).
- **Floating damage numbers are ruled out** (designer, 2026-09-12).
- **Nothing important is keyboard-only**; every toggle has an on-screen affordance. **UI copy is short.**
- **Auto-travel and auto-pick-card live in the SAVE SLOT** (a new game starts with both off).
- **The Achievements list and other heavy lists refresh only while visible**; per-kill events raise dirty flags spent
  at 15 Hz; anything the player CAUSES updates immediately.
- **Animation**: single drawings with a pulse and per-instance liveliness; no run cycles yet.
- **Audio**: one event table; `mouse_killed` is the hottest event, capped near 9/s.
- **Report a bug** and **Discord** links are the only external links.

## 14. The rules that may never break (checklist for every PR)

1. Every tunable number in `Data/` (or in the documented code-constant list, doc 06 §5).
2. All damage through the one resolver; brackets separate; `player_dps()` for anything priced off damage.
3. Uncapped multiplicative lines: `cost_growth > effect_per_level`; summed exponent < 0.80.
4. Rewards = 0.0433 x HP (tier 1 and the flier premium excepted).
5. Two currencies; Stardust only from Reawaken and achievements.
6. The tree never resets; the multiplier reads Stardust ever granted.
7. Evolution only from consecutive boss clears; stage 7 only from the ending; the ceiling is x128.
8. The spawn rate never reads the cat's damage; the biome ladder climbs.
9. Overkill splashes once to the nearest and never chains; the bounty is capped.
10. Pounce is capped on the product; the timing bonus outside.
11. No player HP, no fail state.
12. Enums append-only and stored BY NAME (doc 06 §3).
13. A save format change is a version bump with a migration test (doc 08).
14. A rule that changes what the player can reach is taught to pawsim in the same commit.
15. Every player-facing string is FText; every control prompt is an action token, never "press E".
