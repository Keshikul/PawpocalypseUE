# Phase 6 slice 6: the Unicorn ceremony, the Reawaken banner, Boba, the tutorial (plan, 2026-11-07)

**Status (2026-11-10): 6a DONE (HANDOFF-LOG (63)). 6b-6d wait for the restyle** (designer, 2026-11-07: restyle first;
`WORKLIST-RESTYLE-2026-11-07.md`); they are built in the port's own look after R7, their rules and timing still exact
against Godot. The surveys below are of the Godot repo on 2026-11-07: re-read the scripts before each slice.

Surveyed 2026-11-07 (HANDOFF-LOG (63)) from the Godot repo as it is today: `scripts/systems/unicorn_ceremony.gd` (242 lines),
`scripts/autoload/boba_cameo.gd` (367), `scripts/systems/tutorial.gd` (561) and Godot's own gate `ci/check_tutorial.gd` (503),
their callers (`skill_tree_ui.gd`, `main_menu.gd`, `pause_menu.gd`, `settings_dialog.gd`, `victory_screen.gd`, `hud.gd`,
`catches_pick_ui.gd`) and `hud.gd`'s Reawaken banner. What the port has already: the save's `tutorial_done` (`PcSave`,
`UPawGameSubsystem::Slot().TutorialDone`), the Tutorial tips switch (`PcSettings::bTutorialTips`, the Controls page), the
title's Boba badge frame and caption (no photo), the sounds `unicorn_neigh`, `boba_cameo`, `boss_defeated` in `DA_AudioBank`,
`UPawRainbowFrame` (5d), `PcJuice::TweenInterpolate` (sine, quad, back, elastic; NOT bounce), and the probe technique of 5d
(a script recompiled with its draw calls and global random draws renamed to recorders) and 5c (tweens stepped by
`custom_step`).

Order: **6a, 6b, 6c, 6d**. Each slice ends like 5a-5d: L1 against a fixture of the real Godot code, the pieces as Widget
Blueprints (`PawUi::BakedClasses`, baked once, grafted into existing screens), `Pawpocalypse.UI.*` tests, sabotages seen
CAUGHT, the tour beside Godot, everyday `check_all` exit 0, docs, HANDOFF-LOG.

## 6a: the Unicorn ceremony (unicorn_ceremony.gd)

> **DONE 2026-11-07 (HANDOFF-LOG (63))**: as below; the ceremony is the tree's own top part (not a stack screen). Godot's
> Tween sequencing became `PcTween`, for 6c to reuse.

**What Godot does.** Bought `unlock_unicorn` in the tree (`skill_tree_ui.gd _on_node_pressed`): the tooltip hides and a
CanvasLayer (layer 15, PROCESS_ALWAYS: the tree pauses the game) plays over the tree: 0.00 the tree dims (INK to alpha 0.55
in 0.25 s) and the first whinny; three hops (0.32 s each, a sine arc 0.22 x the sprite's side high) of the mounted cat
(`player_cat_stage_4`, mirrored, a square 0.55 x the view's height, resting at x centred and its foot at 0.62 x the height)
from off the right edge; the second whinny; the rear (rotation -16 deg in 0.18 s, TRANS_BACK, and scale 1.12); the fiesta
(`boss_defeated`; 70 confetti from the chest, 110 confetti raining across the screen, 26 gold stars with an ink rim: global
`randf` / `randf_range`); the settle back (0.35 s after a 0.25 s delay, TRANS_ELASTIC); the rainbow grows from 1.25 s over
0.55 s (six arcs, 64 points, `band + 1` wide, antialiased); 3.40 s fade (root alpha to 0 in 0.35 s), `finished`, freed. Any
press (mouse button, a key that is not an echo, a pad button) after 0.6 s skips to the fade and is eaten. Its root takes
every click (MOUSE_FILTER_STOP, focus): the tree cannot be bought from mid-fiesta; the tree takes no keys while it plays
(`ceremony_playing()`), and gets its focus back on `finished` (`_enter_focus`).

**Port.** PawCore `PcUnicornCeremony` (the clock, the tween timeline through `TweenInterpolate`, the bits' physics with drag,
sway and spin, the rainbow's growth, skip and fade; random draws from `PcRng`, or replayed). `UPawUnicornCeremony` (a
piece; Menu-stack or the tree's own top layer, the decision in the slice: it must take input and hold the tree's keys)
painting through `FPawFxCanvas` (+ `Arc` with antialiasing, star polygons and their ink polyline) and the stage-4 cat from
`DA_FieldAtlas` (as `UPawGarden` draws atlas sprites). Sounds through `UPawAudioSubsystem`.

**Fixture `unicorn`.** The real script recompiled: `draw_arc` / `draw_colored_polygon` / `draw_polyline`, `randf` /
`randf_range` and `AudioManager.play` renamed to recorders (the receiver `_sky.` dropped with the rename); its tweens
stepped by `custom_step` from a synchronous `_ready` (5c); random frame lengths, presses at random moments (before and
after 0.6 s; echo, mouse, pad), sizes 1280x720 to 3840x1080. Per frame: clock, dim alpha, the sprite's position / rotation /
scale, rainbow growth, root alpha, done, every bit, the draw list, the sounds. Then the tree's side in the port: buying the
Unicorn plays it, nothing else does; the tree takes no key while it plays; a click on the tree under it buys nothing.

**Done when** L1 `Unicorn` matches the fixture (float32 tolerance), `Pawpocalypse.UI.Unicorn.*` (plays on the purchase only,
eats input, skip after 0.6 s, hands focus back), sabotages CAUGHT, a tour shot mid-fiesta beside Godot.

## 6b: the Reawaken banner (hud.gd _build_reawaken_banner, _refresh_reawaken_banner)

A prerequisite of 6d's Reawaken cue ("On the tab the first-Reawaken banner is already saying it"). A paper plate (UI_PANEL,
3 px ink, radius 8, margins 22 / 34 / 12 / 12) with "Reawaken is ready  —  open the Reawaken tab" (22 px, ink, the Stardust
icon wrapped before it), centred over the PLAYFIELD (not the screen: the dock owns one side), wearing `RainbowFrame`
(margin 2, corner 10, width 5, active). Shown while `can_reawaken() and reawaken_count == 0 and the tab is not Reawaken`;
pops in when it shows; jitters while shown. Port: a PcHud decision (`ReawakenBanner`), the plate in `UPawHudScreen` (graft),
a second `UPawRainbowFrame` following it. Fixture: the real hud.tscn's `_refresh_reawaken_banner` on random states (the
victory fixture already runs the real HUD). Read the rest of `_refresh_reawaken_banner` (where over the field) first.

## 6c: Boba (boba_cameo.gd; the title, pause, settings, victory, tree call sites)

**The photos.** 14 PNGs in Godot's `assets/art/boba/` (photos, 304-640 px, 5.5 MB; trimmed by Godot's `tools/boba_prep.py`
from `content/boba/`; listed with `ResourceDir.list(dir, ".png", true)`, sorted). Port: copied by a tool (`--check` = byte
identical to Godot's) into `SourceArt/boba/` (LFS: they are not regenerated), imported by `tools/import_art.py --run boba` as
UI textures (`T_boba_*`, sRGB, no mips) + a list asset in sorted order (a new photo in Godot's folder is in the rotation
after the re-export, as Godot's "nothing registered by hand").

**The cameo clock** (`UPawBobaSubsystem`, a GameInstance subsystem over PawCore `PcBoba`): `EnterMenu(tag)` / `LeaveMenu(tag)`
(the first open menu schedules 6-14 s; then every 25-60 s; the last to close stops the clock, a cameo in flight finishes);
`Suppress(tag)` / `Unsuppress` (the tree: nothing new, a cameo in flight CUT); the timer skips (reschedules) while busy,
with the effects off, with no photo, or suppressed; `Force(kind)` (the title badge's click; the victory's "nyan" 1.5 s after
it lands). Kinds weighted flyby 4, spin 1, nyan 3, peek 4, drop 2; a photo never twice running; height 0.24-0.34 of the
screen. **To check, a candidate finding**: `force()` skips the effects-off check the clock honours, so the victory screen
flies Boba with the effects off.

**The cameos** (one piece on a top layer over every menu, Godot's layer 20; never takes input): flyby (across at a random
height, a 2.2-cycle bob, mirrored when going right, 3.6-5.6 s), spin (the same, 1-2 turns, TRANS_SINE in-out), nyan (3-4.2 s,
a 5 deg rock, the six-band trail: 60 points sampled every 7 px of travel, bands `size.y * 0.55 / 6` apart, hsv(i/6 * 0.78,
0.75, 1, 0.9), antialiased polylines), peek (from one of four edges to 0.55 / 0.6 of his size, TRANS_BACK out 0.7 s, jitter
1.4-2.2 s, TRANS_QUAD in 0.5 s; upside down from the top), drop (TRANS_BOUNCE out 1.3 s to 0.58 of the height, jitter
1.0-1.8 s, out the bottom TRANS_QUAD in 0.55 s). `boba_cameo` sound on each. Needs **TRANS_BOUNCE** in `TweenInterpolate`
(and the `juice` fixture's curves extended to it).

**The menus.** The title: the badge's photo (the first in sorted order), its hover pop, a click forces a cameo; `main_menu`
entered / left. Pause: `pause` entered / left; "CEO on duty", a random photo 180 x 220 beside the window's bottom right
(16 px right of it), popping in, tooltip "CEO on duty". Settings: `settings`. Victory: `victory`, and the nyan 1.5 s later if
still up. The tree: suppress on open (browse and spend), unsuppress on close.

**Fixture `boba`.** The real autoload recompiled with `randf` / `randf_range` / `randi_range`, `AudioManager.play` and
`UiJuice.jitter` renamed to recorders, the Trail's `draw_polyline` too; its Timer and tweens stepped by hand; random
sequences of enter / leave / suppress / unsuppress / force / frames. Per step: scheduled, busy, last kind, the sprite's
position / size / rotation / flip / visible, the trail. L1 `Boba`; `Pawpocalypse.UI.Boba.*` (menus only, never in play,
never over the tree, the effects switch, the title click, the pause photo, the victory nyan).

## 6d: the tutorial (tutorial.gd; Godot's gate ci/check_tutorial.gd)

**What Godot does.** A paw (the cursor art, 64 px, tapping every 0.9 s: closed for 0.16 s, a 6 px dip, 0.88 scale, pivot at
the toes) on ONE thing and at most one short line (22 px ink on a paper plate: 3 px ink, radius 8, margins 16 / 9) beside
it. Eleven steps, first eligible wins: card, tree, boss, reawaken, bait, travel, kitten, buy, claim, pounce, hover. A step
shows after its condition held 1.5 s (grace); 4 s quiet after one ends (breather; the modal steps card and tree skip it);
given up after 45 s (boss: 7 s, it is information); done is a FACT about the game (`is_done`: kills >= 3, an upgrade bought,
a kitten, a card taken, a claim, deeper than the first biome, a boss beaten or up, a Reawaken, a skill bought or two
Reawakens, Pounce seen cooling; a veteran (a Reawaken) skips most), retired whether or not the cue showed; eligibility
needs no pause and no ceremony for the field / dock steps; out of season hides it without marking it done. Never takes a
click or focus; layer above the HUD and the tree, under settings / pause / the ceremony (over the card draft for `card`).
Off with the switch (mid-cue too) and in the soak / trailer (`suppressed`). Targets: the dock row via its tab first
(`_dock_target`), the draft's first Take, the tree's cheapest affordable node in the spend step, the Pounce clock, the
nearest live mouse to the cat (hover). Lines: ten keys in Godot's "tutorial" context (the pounce line with its prompt
token; the hover line with the mouse-move glyph or the right stick); Reawaken shows no line on its tab (the banner, 6b).
Plate placement: beside the dock on the field side, level with the row; above the Pounce clock; beside the tree node; under
the boss banner; clamped on screen.

**Port.** PawCore `PcTutorial` (the state machine over a facts struct per think, 10 Hz: done / eligible per step, from
PawCore state and the HUD's views; the tap; the plate placement given rects). `UPawTutorialOverlay` (a piece on its own
layer above the HUD and the tree, under the menus; `HitTestInvisible`, never focusable) reading `UPawHudScreen` /
`UPawCardPickScreen` / `UPawSkillTreeScreen` views (Godot's `upgrade_button`, `tab_button`, `claim_control`,
`new_realm_row`, `bait_button`, `reawaken_button`, `pounce_gauge`, `first_take`, `tutorial_node`), writing only
`Slot().TutorialDone`. A `-NoTutorial` switch for the soak, the bench and the tours (Godot's `suppressed`). Strings into
`ST_UI` (`TUTORIAL_*`).

**Fixture `tutorial`.** The real tutorial.gd recompiled with its reads of the HUD, the controller, the draft, the tree and
the ceremony renamed to a stub the fixture drives (GameState and AchievementManager are the real autoloads, set per row):
random fact sequences through `think(dt)` with chosen deltas; per think: current, pending, the done set. And `_tap`,
`_tap_point`, `_plate_position` on random rects. Then **Godot's gate walked in the port** (`Pawpocalypse.UI.Tutorial.Walk`:
the first run of check_tutorial.gd step for step, incl. never takes input, lines <= 7 words and no "!", a veteran skips, the
switch mid-cue, the save round trip, a new game clears it): **phase 6's "tutorial gate green"**.

## After slice 6

Phase 6's exit (doc 11): the colour gate on a package, the tours (12 biomes x 7 stages x 5 bosses), every audio event once
in a logged tour (the Unicorn's and Boba's sounds now have their players), the tutorial gate.
