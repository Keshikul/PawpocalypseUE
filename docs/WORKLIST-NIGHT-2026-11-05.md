# Night worklist, 2026-11-05 (unattended, ~23:00 to morning)

> **Closed (history)** 2026-11-06 06:00; result in HANDOFF-LOG (56).

> Written 2026-11-05 23:10 from HANDOFF-LOG (55) and the designer's words: *"When the test finishes fix anything that's broken.
> I want you to create [and] execute an overnight plan."* The status column is updated as the night goes; the closing
> HANDOFF-LOG entry (56) summarises the result, and this list is then marked closed.

The nightly `-Full` was started at ~20:40 (log `build/check_all_full_20261105.log`, per gate `build/check_all/`). It is the night's
spine: nothing that touches a source, a tool, `Data/` or an asset may run beside it (its sabotages edit sources in place and
rebuild). Estimated end ~05:00-06:00 (7.6 h measured-based estimate, plus another project's soak sharing the CPU). So the night
has three parts: **read-only work while it runs** (the slice 5 design, written here), **the fixes when it ends** (the designer's
first ask), then **slice 5a** if the gates are green and time is left.

**Ground rules for the night** (CLAUDE.md standing rules):
- No commit, push, package or export (not asked). The Godot repo is read-only (its `git status` is checked by `check_all`).
- While `-Full` runs: only `docs/` and the knowledge base are written; no build, no tool run that writes outside `build/`.
- A gate failure is fixed at its cause: a stale sabotage is retargeted (and seen CAUGHT alone), a real bug is fixed with a test
  that fails without the fix. A failing gate is rerun ALONE after its fix; the night ends on an everyday `check_all` exit 0.
- No design changes. A Godot ambiguity is ported as Godot has it and written up as a finding (doc 01 §5 + the Godot findings file).
- Widget Blueprints own their layout: never re-bake; new parts by `--graft`; looks changed in the assets by script.
- A piece that cannot be finished stops with its finished parts green and every unfinished rule a named TODO.

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight**: this project has no Unreal process; the everyday `check_all` green before the nightly (HANDOFF-LOG (55)); the nightly started | - | log exists | **done** 20:40: everyday exit 0 at 20:30; another project's soak running (ignored by the guard) |
| 1 | **Slice 5 design, read-only** (while `-Full` runs): Godot's theme (`pawpocalypse_theme.tres`: every StyleBoxTexture, its margins and content margins), the UI SVGs (`assets/art/ui`, sizes), `ui_juice.gd`, `rainbow_bar.gd`, `rainbow_frame.gd`, `skill_tree_sky.gd`, the icon call sites (`Art.texture` by constructed name); the port's placeholders (grep "phase 6" in `Source/Pawpocalypse/UI`); the pipeline (`raster_svg.py` groups, `import_art.py`, the `BP_*` styles, `PawUiKit` brushes). Output: §Slice 5 below, split into slices 5a-5d each with its own done-when | 2 h | §Slice 5 written | **done** 23:05: 5a (the chrome, with Godot's margins table and the first step: who owns a button's look), 5b icons, 5c motion, 5d rainbows and sky |
| 2 | **Watch `-Full`** to its end; read every gate's log | - | the summary table read | **done** 02:44: 29 of 30 gates PASS (PawCore sabotage 563/563 in 3 h 15 min, Godot fixtures, L1 long, mirror 200k, fuzz 30k, pawsim sweep 1 h 53 min, L4 pad-only); **Realm sabotage FAIL in 0 s**: refused to start, one STALE target (`settings_effects_ignored`: its line in PawRealmView.cpp was rewritten by slice 3, the effects budget moved into FPawVfx), so none of its 388 sabotages ran |
| 3 | **Fix what it found** (designer's ask). For each failed gate: the cause, the fix, the gate rerun alone; a MISSED sabotage retargeted and seen CAUGHT; then the everyday `check_all` | 0.5-2 h | every failed gate PASS alone; everyday exit 0 | **done** 05:45: (a) `settings_effects_ignored` retargeted, CAUGHT alone; the cause fixed: new everyday gate `sabotage targets current`, seen failing on a rewritten target in either tool; (b) the realm gate rerun 02:47-05:42: 381/388 CAUGHT; of the 7 MISSED `view_no_effects` retargeted (CAUGHT alone) and 6 retired (bake-only lines since the Widget Blueprints; their properties are the assets', still asserted by the tests); (c) everyday `check_all` exit 0 |
| 4 | **Slice 5a: the chrome** (only with 3 done and >= 2 h left): the nine-patch art of §Slice 5a in place of the flat rounded brushes (buttons x 4 states, panel, window, dock, bar track / fill, separator), through the raster -> import -> brush pipeline, with a test asserting each brush's texture and margins, sabotages seen CAUGHT, a `-game` screenshot beside Godot's | 2-3 h | `Pawpocalypse.UI.*` green, sabotages CAUGHT, shots compared, everyday exit 0 | **not started**: item 3 ended at 05:45, under the 2 h this needs; the design (below) is ready for the next session |
| 5 | **Close**: docs 00, 09, 11 (counts, state); KB for any new lesson; HANDOFF-LOG (56) with the exact next step; this list marked closed | 30 min | written | **done** 06:00: docs 00, 09; KB U/18 §8 + INDEX; HANDOFF-LOG (56); this list closed |

**Order**: 1 while 2 runs; 3 the moment 2 ends; 4 only if 3 is done and the everyday gate is green; 5 always. If 3 takes the
rest of the night, 4 waits for the next session (3 is never cut).

**For the designer in the morning**: the hardware check from (55) (a `-game` run with two output devices: pick one, plug in a
pad with a headset, unplug the chosen device) is still yours.

## Slice 5 (the UI art and motion): the design

Read 2026-11-05 night from Godot's `scenes/ui/pawpocalypse_theme.tres` (250 lines), `assets/art/ui/` (79 SVGs),
`scripts/systems/ui_juice.gd`, `rainbow_bar.gd`, `rainbow_frame.gd`, `skill_tree_sky.gd`, and the port's
`Source/Pawpocalypse/UI` (the brushes are `PawUi::Rounded` flat rounded boxes: 17 call sites in PawButton, PawHudScreen,
PawPounceClock, PawPrompts, PawToast, plus PawMainMenu / PawCardPickScreen / PawJobPicker; the button looks live in the six
`BP_*Style` assets' Class Defaults).

### 5a: the chrome (nine-patch art) - first, it changes every screen

> **DONE 2026-11-06 (HANDOFF-LOG (58)), with corrections to the table below**: Godot draws far fewer of these than this table
> says. The dock overrides `DockPanel` with a StyleBoxFlat (`ui_dock.svg` is drawn nowhere); the pause, settings and title
> windows, the toast, the boss panel, the zone plate and bar and the XP bar override too. WindowPanel's consumers are the job
> picker and the victory only (the title's windows override it). `ui_window` content margins: 22 / 20. The separator draws
> nothing in Godot (F41). Step 1 measured: the `BP_*Style` Blueprints save no values, so the C++ defaults reach them. The ring
> is painted over the plate (a state brush cannot carry it; a locked row is never hovered).

Godot's chrome is `StyleBoxTexture` over its own SVGs, rasterised at **scale 1.0** (`svg/scale=1.0` in each `.import`), with
`texture_margin` the SAME in the source and on screen (1:1 corners, never scaled), and these numbers (texture margins / content
margins, px):

| Style | SVG (px) | texture margin L T R B | content margin L T R B | Godot consumers |
|---|---|---|---|---|
| `ui_button_normal` / `_hover` / `_pressed` / `_disabled` | 256 x 64 | 16 16 16 16 | 16 6 16 6 | Button, OptionButton (4 states); CheckBox keeps the defaults |
| `ui_panel` | 128 x 128 | 20 20 20 20 | 14 12 14 12 | Panel, PanelContainer, PopupMenu's panel |
| `ui_window` (type variation WindowPanel) | 256 x 256 | 48 48 48 48 | (read in 5a) | modals: confirm dialog, settings, job picker, victory |
| `ui_dock` (type variation DockPanel) | 128 x 128 | 20 20 20 20 | 18 14 16 14 | the dock |
| `ui_bar_track` / `ui_bar_fill` | 64 x 16 | 8 4 8 4 | - | ProgressBar background / fill (the zone bar: 10 px tall) |
| `ui_separator` | 64 x 8 | 8 2 8 2 | - | HSeparator, PopupMenu's separator |
| focus | StyleBoxFlat | ring 3 px `#FF7055`, radius 12, expand 3 | - | Button, CheckBox, OptionButton focus (keys and pads only) |

Godot's own rule (ci/check_ui_theme.gd): `margin top + bottom < the shortest consumer`, `left + right < the narrowest`
(shortest: ProgressBar 10, Button 34, Panel 42, WindowPanel 240). Font colours: Button `#1A1A1A`, pressed `#8F836F`, disabled
`#C7C1AB`, 16 px; Label 17 px ink.

**In Unreal**: an `FSlateBrush` with `DrawAs = Box`, `ImageSize` = the SVG's own size, `Margin` = texture margin / image size
per side (a FRACTION in UMG), over a raster of the SVG. Raster at **2x** (crisp at 4K, the same layout: ImageSize stays 1x)
or 1x (Godot's exact pixels at 1080p): decide by a side-by-side shot at 1920 x 1080 against Godot, 1x first (it is Godot's).
Pipeline: a `ui_chrome` group in `tools/raster_svg.py` (--check gate already covers groups), `tools/import_art.py` (UI textures:
no mips, `TC_EditorIcon` / UI group, sRGB, never streamed), the textures under `/Game/Pawpocalypse/Art/UI/Chrome`.

**Steps** (each with its test before the next):
1. **Find out who owns a button's look now**: a BP child's Class Defaults serialise only what differs from the parent's CDO, so a
   changed C++ default may or may not reach `BP_ButtonStyle`. Measure it (change one default in a scratch build, read the BP's
   value headless) before deciding: if it propagates, change `UPawButtonStyle` & co.; if not, set the BP values by a headless
   script (`unreal.EditorAssetLibrary` + `set_editor_property` on the generated class's CDO, then save), the WBP rule kept.
2. `PawUi::Chrome(EPawChrome)` -> the box brush for a style (one table: texture, image size, margins), beside `Rounded`;
   `PawUiConstants.h` "until the art arrives" constants retired as they are replaced.
3. Buttons (the six `BP_*Style` assets: normal / hover / pressed / disabled = the four textures; the focus ring stays a flat
   3 px ring); panels and the dock (`WBP_HudScreen` dock + panels, `WBP_PauseMenu`, `WBP_SettingsDialog`, `WBP_JobPicker`,
   `WBP_VictoryScreen` = window); the zone bar track / fill; separators. Where a WBP holds the brush in its tree, the asset is
   edited by script (never re-baked); where code sets it at runtime, the code changes.
4. Test `Pawpocalypse.UI.Chrome`: every consumer's brush is the right texture, Box, margins = Godot's / size, ImageSize; Godot's
   margin rule over the port's measured consumers. Realm sabotages: a wrong margin, a texture swapped, the focus ring lost.
5. Shots: `py tools/godot_tour.py` (the dock, the pause menu, the settings, the job picker) beside Godot's; differences fixed.

### 5b: the icons (by constructed name, Godot `Art.texture`)

> **DONE 2026-11-06 (HANDOFF-LOG (59))**: 178 icons at the SVGs' own 128 px (Godot imports them at scale 1.0, no mips),
> `DA_Icons` + `PawUi::Icon`; only 17 of the `ui_*` SVGs are drawn by Godot (`ui_card_frame`, `ui_close`, `ui_lock`,
> `ui_skill_node*`, `ui_slot` by nothing). The quick bar came with it. Godot's open tab is the pressed plate, not a colour.

`ui_tab_<id>` x 6 (tabs are icon buttons 52 x 46, hud.gd TAB_ICON_SIZE), `ui_icon_catnip` / `_stardust` / `_bait` (26 px on
buttons, 24 beside labels, the status bar), `ui_quick_*` x 4, `ui_menu_*` x 3 (the title's 56 px plates), `achievement_<id>` x 41
(36 px), `ui_lock`, `ui_close`, `ui_slot`, `ui_card_frame`, `ui_skill_node[_locked|_owned]`, `ui_tick` (phase 7), and the data's
`icon_path` for cards, skills (56 px) and jobs (40 px). One `UPawIconAtlas`-style data asset (name -> texture) generated by the
importer, so a missing name fails a test, not a screen. Placeholders to replace: grep "phase 6" in `Source/Pawpocalypse/UI`
(the tab names, the currency discs, the achievement marks drawn as discs, the Pounce clock's icon, the title cluster's words,
the card picture slot, the tree's node icons). Test: every name Godot constructs resolves; sabotage: one missing.

### 5c: the motion (`ui_juice.gd`)

> **DONE 2026-11-06 (HANDOFF-LOG (60))**: `PcJuice` (PawCore) is Godot's Tween easing and UiJuice's state machine, exact
> against fixture `juice` (the real Tween and the real ui_juice.gd on a real Button); `FPawJuice` (UI) applies it as a render
> transform at Godot's call sites. Built: hover on the shop rows, job buttons, tabs, travel rows, claim buttons, tree nodes,
> the title's cluster; jitter on the Reawaken tab, Bait, newly open travel rows, the zone plate, READY tree nodes; punch on a
> bought row or node; pop-in on the tree's leaving plate; and the Reawaken tab's glow (`_tick_reawaken_glow`). Not built
> (their owners are not ported yet): Boba's badge, cameo and CEO photo, the Reawaken banner, the language menu, the tutorial
> plate, the custom tooltip. Godot's Bait restarts its lean every refresh: finding F42, copied.

Call sites in Godot: `hover` 8, `jitter` 13 (incl. `.bind` 4), `pop_in` 6, `punch` 2. Rules to keep: motion never changes
layout (render transform only: UMG `SetRenderTransform` scale / angle, pivot centre); hover owns scale, jitter owns rotation;
every motion is restarted on each event (a killed tween), TRANS_BACK / TRANS_ELASTIC EASE_OUT reproduced as functions of time
in PawCore (exact against Godot's Tween curves: a fixture sampling them); `effects_enabled` off stops the idle jitter only.
Jitter on ONE thing per screen (the Reawaken tab when ready, the next biome's plate, a READY node).

### 5d: the rainbow pieces and the tree's sky

> **DONE 2026-11-07 (HANDOFF-LOG (62))**: built as designed below, with a probe that RECOMPILES each script with its
> draw calls renamed (a subclass cannot intercept them). Only the zone plate's frame is built: `rainbow_frame.gd` is NOT
> Bait's own frame (Bait leans; the plate reads "Bait ready" in its frame) and the Reawaken banner's frame waits for the banner.

`rainbow_bar.gd` (the Unicorn's rainbow fill), `rainbow_frame.gd` (the Bait-ready plate's frame), `skill_tree_sky.gd` (the tree
screen's backdrop). Each is `_draw()` code: ported like slice 3's effects (a Slate paint with Godot's draw calls), each against
a probe fixture of the real node as slice 2 did for the backdrop.

**Order**: 5a (every screen), 5b, 5c, 5d; then slice 6 (the unicorn ceremony, Boba, the tutorial overlay); then the phase 6
exit tours (doc 11).
