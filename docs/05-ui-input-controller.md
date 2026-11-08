# 05 - UI, input and controller support

> The Godot game is fully playable from keyboard/mouse and a pad (controller plan phases 0-6 and 8, gates 17 `input`
> and 18 `padplay`). That contract ports unchanged. The Unreal plumbing is Common UI + Enhanced Input, set up exactly
> as `C:\Games\Knowledge\Unreal\08-common-ui-controller-input.md` says. **Read that doc in full before writing any
> widget**: its section 3 holds the gamepad bugs that cost days in a real 5.8.3 project.
> Godot sources: `docs/pawpocalypse-controller-plan.md` (1,383 lines, the designer's decisions in §14),
> `pawpocalypse.md` "Input Rules" and "UI Layout Rules".

## 1. Plumbing (phase 0 sets it, phase 5 completes it)

- `GameViewportClientClassName=/Script/CommonUI.CommonGameViewportClient` in `DefaultEngine.ini` (without it the pad does
  nothing) `[KB U/08 §1]`.
- `CommonUI.*` cvars in `DefaultEngine.ini [ConsoleVariables]`, never `DefaultGame.ini` (ignored there) `[KB U/03 §6]`. The
  5.8.3 template had put them in `DefaultGame.ini`; phase 0 moved them (2026-10-28).
- `CommonInputSettings`: `InputData` = `BP_PawUIInputData` (Click and Back actions), `bEnableEnhancedInputSupport=True`.
  Platform settings for Windows with TWO controller data assets (keyboard, gamepad); the default gamepad name must match or
  glyphs come out empty.
- `bEnableUserSettings=True` for Enhanced Input remapping; actions marked Player Mappable.
- `bUseSplitscreen=False`.
- **As built (phase 5 slices 1-3b)**: the viewport client, the cvars, `bEnableEnhancedInputSupport` and `bEnableUserSettings`
  are set. `BP_PawUIInputData` and the controller data assets are NOT used: Slate gets Godot's accept / back keys from
  `FPawNavigationConfig`, each screen handles `back` itself (§3, slice 2), and the glyphs are this game's own (`PawGlyphs`,
  §5), not Common UI's controller data. **Remapping is Godot's own** `[bindings]` in settings.cfg, applied to a live mapping
  context the game owns (slice 4, §3), not Enhanced Input's user settings (`bEnableUserSettings` stays on but nothing uses
  it: no action is Player Mappable).

## 2. Screen structure

```
UPawRootLayout (owned by APawPlayerController, created on BeginPlay)
  Layer.Game    : UPawHudScreen (activatable, Game input config, CapturePermanently? see §4) - ALWAYS at the bottom
                    field overlay: Pounce clock, XP bar, boss banner/bar, status row (currencies, zone bar), tutorial overlay
                    UPawDock: tab strip (6 icon tabs with badges) + page switcher
  Layer.Menu    : UPawSkillTreeScreen (and over it its own UPawUnicornCeremony), UPawCardPickScreen, UPawReawakenCeremony, UPawPauseMenu, UPawVictoryScreen
  Layer.Modal   : UPawSettingsDialog, confirm dialogs, UPawJobPicker
Title map: UPawMainMenu (slots, settings, Report a bug, Discord, Boba garden)
```

Rules from the KB, all of which bit a real project `[KB U/08 §2]`:
1. One persistent bottom HUD activatable with a **Game** input config; every menu stacks above it; closing the last menu
   restores game input. **Never call Set Input Mode by hand.**
2. Every screen implements `GetDesiredFocusTarget()`; exactly one Set Focus per activation.
3. Activatables nest, never siblings.
4. B/Esc closes the top screen and never leaves a half state; Start closes the pause menu too.
5. Screens are pooled: reset state in `SetTarget*`, not only on construction.
6. A sub-screen over Pause tracks "covered by child" so the world does not unpause.
7. Anything animating while paused uses `FTSTicker`.

**Every screen and piece is a Widget Blueprint the designer edits (designer, 2026-11-04; HANDOFF-LOG (38))**. Each stays a C++
class that can build its own layout; `tools/bake_widgets.py` (PawpocalypseEditor `UPawWidgetBaker`) writes that layout ONCE into
`/Game/Pawpocalypse/UI/Widgets/WBP_<Name>` (the button styles into `BP_<Name>` Blueprints), and from then on **the asset owns the
layout**: nothing bakes over it. The contract (`UI/PawWidgets.h`): `Initialize` builds in code only for a native class, the parts
code needs are `BindWidgetOptional` with the widget's name (repeated ones `Prefix0..n`, found by `CollectNamed`), and
`OnLayoutReady` wires both modes (clicks, formatted texts, state); rows that come from the data (the shop, the realms, the cards,
the achievements, the jobs, the job picker's jobs, the card faces, the tree's nodes) are built at runtime into named containers,
never baked; a piece's instance settings (a button's label, size, wrap, alignment, flat look) are properties, so the owner's
asset keeps them. Widgets are made through `PawUi::Resolve` (the Blueprint when one exists; `paw.UI.CodeLayout 1` forces the code
layout). `Pawpocalypse.UI.Widgets.AreBlueprints` / `.Parity` check every class in `PawUi::BakedClasses`; every UI test runs on
the shipped Blueprints. A designer's change can make Parity differ ON PURPOSE (the asset owns the layout): read what it names.

**The dock is not a Common UI stack of screens**: it is part of the HUD screen (the game keeps running while you shop). Its
pages are plain `UUserWidget`s in a switcher; the HUD screen's focus target is the dock's remembered row.

**Proven by spike S4 (2026-10-28, doc 04 §10)**: this structure, built in C++ only, passed 1,193 focus checks with real Slate
key events (pad and keyboard), focus never lost. Rules it added:
8. **The remembered row follows focus continuously** (on every focus change, not on tab switch): Common UI re-applies
   `GetDesiredFocusTarget()` when the input device changes, before your key handler runs.
9. **C++ widgets build their content before `Super::Initialize()`** (create the `WidgetTree`, set the root): Common buttons
   then wrap it in their internal button.
10. **Focus = selection is styled explicitly** (a focused brush/outline per button style); Common UI's hover-on-focus is not
    applied consistently.
11. Every full-screen menu (the tree) has its own opaque backdrop.

## 3. Input actions (Godot -> Enhanced Input, 1:1)

**As built, phase 5 slice 1 (2026-11-04, HANDOFF-LOG (42))**: Godot's input map is DATA. `tools/export_input_map.py` (run by
`export_godot_data.py`, so the L0 data-export gate keeps it current) writes `Data/InputMap.csv`: one row per Godot event (72
events, 32 actions: the table below plus `section_prev/next`, `zoom_in/out` and the `ui_*` overrides), with Godot's code and
the Unreal key. Keys are by POSITION (`physical_keycode`; the export refuses a keycode-only or modified key). Godot's stick Y
points DOWN, Unreal's UP: the export flips the Y signs. Keypad Enter has no Unreal key of its own: it is Enter (`merged`).
`py tools/create_input_assets.py --run` (PawpocalypseEditor `UPawInputAssetBuilder`) GENERATES the assets, never edited by
hand: `IA_<Action>` per Godot action under `/Game/Pawpocalypse/Input/Actions`, and ONE context `IMC_Pawpocalypse` holding every
event (Godot keeps every action live at once; the three contexts sketched below are not needed for that). An action with a
stick or trigger event is **Axis1D**, read through **`UPawHalfAxis`** (one direction of the axis with Godot's dead zone:
pressed from the dead zone ON, strength rescaled from it to 1, exactly Godot's own Input on 5,726 measured values, fixture
`input_axis`; a pressed value under Enhanced Input's actuation tolerance reads 1e-4 so it stays pressed); any other is
Boolean (a Boolean action would turn the axis into true / false before the modifier runs). The steer actions' dead zone is the
player's Settings > stick dead zone at run time in Godot (`InputBindings.apply`): 0.2 by default, applied with the settings in
slice 3. `Pawpocalypse.Input.Map` checks the assets equal the CSV and that every action has a keyboard / mouse AND a pad
binding except the ONE-DEVICE list (pad only: `steer_*`, `recentre`; keyboard only: `tab_1..6`, `open_upgrades`).
`Pawpocalypse.Input.HalfAxis` replays Godot's values through the context's own modifiers. The AZERTY check: slice 5 (below:
an `FKey` is NOT a position; keys with a place go through the keyboard's layout).

**Slice 2 (2026-11-04, HANDOFF-LOG (43)): the game and the screens read the map.**
- **The field**: `APawPlayerController` adds `IMC_Pawpocalypse` to its local player; `APawRealmView` reads `IA_Pounce` (the
  mouse button and the right trigger past its dead zone). A press over the dock is the dock's when the last input was the
  pointer (Common UI's input type), never a trigger's (`PounceHeld`). The stick-steered cat and `recentre` are slice 3.
- **The screens** ask what a Slate key event IS (`PawInput::KeyIs`, from the live mapping context; a stick or trigger pushed
  past Slate's threshold arrives as a digital key such as `Gamepad_RightTrigger` and counts as its axis half), so the focus
  path stays Slate's and every screen follows a remap: the HUD (`tab_prev / tab_next` (A / D, Left / Right, LB / RB, the
  D-pad's left / right: Godot steps tabs on the D-pad too), `tab_1..6`, `buy_amount`, `open_upgrades` (U: the Upgrades tab,
  new), `open_skill_tree`, `pause`, and `press` on a key the menus do not accept: the left trigger presses the selected dock
  button), the pause menu (`back`, `pause`), the settings (`back`, `section_prev / section_next`), the title (`back`), the
  skill tree (`back` / `open_skill_tree` close, `buy_amount` jumps, the zoom by Godot's `get_action_strength("zoom_in") -
  get_action_strength("zoom_out")`: a held = / - is 1, a trigger its strength past the dead zone; the pan from the steer stick
  through Godot's `Input.get_vector`: circular dead zone, rescaled, capped at 1; measured on 3,008 deflections).
- **The menus' navigation** is Godot's `ui_*` map: `FPawNavigationConfig` (installed by the game instance in a game, never
  the editor) replaces Slate's key rules with the map's (W / A / S / D beside the arrows and the D-pad, keypad Enter, the
  left stick at the `ui_*` dead zone; Tab still moves, as Godot's built-in `ui_focus_next`).
- **Common UI's input data (Click / Back) is not used**: the navigation config gives Slate Godot's accept and back keys, and
  each screen handles `back` itself (as each Godot screen polls it). Doc 05 §1's `BP_PawUIInputData` is not needed for that.
- `Pawpocalypse.Input.Keys / Navigation / Vector / Screens / Pounce` (every key bound to a screen's actions sent through the
  screen's own Slate widget). **Not yet**: hold-to-repeat for a `press` held on the left trigger (Space / Enter / A repeat
  through the buttons); the Invert X / Y switches join with the Controls page (slice 4).

**Slice 4a/4b (2026-11-05, HANDOFF-LOG (47)): remapping, the rules and the map.**
- **The rules are PawCore's `PcBindings`** (input_bindings.gd): the ten logical rows (a row is several actions: "Up" is
  menu_up AND ui_up), rebind with its swap ("Swapped with Down"), refusals (Esc, Start and pad B are system keys; Pause is
  locked, Re-centre on the keyboard; the keys of tab_1..6, open_upgrades, the zoom and steer actions are taken), reset_row
  (and every row it then shares a key with), the saved diff, apply() with Swap sticks and Swap confirm and back, binding_from()
  (what a captured event becomes). Exact against the real script (fixture `input_bindings`: 3,000 rows x 8 ops, some starting
  from a hand-edited state; 3,000 raw events; 1,500 settings.cfg files) and 22 sabotages.
- **Events are Godot's** (a physical keycode, a button, one half of an axis); `Input/PawKeyCodes` turns them into `FKey`s and
  back (Godot's one Shift / Ctrl / Alt / Meta code is both of Unreal's sides; keypad Enter is Enter; the Y axes flip). The
  defaults are Godot's map, keypad Enter included: `Input/PawInputDefaults.h`, generated from Data/InputMap.csv by
  `tools/gen_input_defaults.py` (the CSV is not cooked; L0 checks the header is current).
- **The live context**: `PawInput::LiveContext()` is ONE transient mapping context the game adds (the player controller) and
  every key question reads (`KeyIs`, `KeysOf`, the prompts, `FPawNavigationConfig`). `PawInput::ApplyBindings(Settings)`
  rebuilds it in place from `Pc::BindingApply` (the half-axis modifiers carry each action's dead zone; the steer actions the
  player's), forgets the cached answers and tells `OnBindingsChanged` (the game instance installs a new navigation config).
  `UPawInputDeviceSubsystem` calls it when the bindings, a swap or the dead zone changed, asks every local player's Enhanced
  Input to rebuild, then fires `OnControlsChanged`: the prompts redraw on THAT (not on the settings, which could reach them
  before the map did). With no bindings the live context is IMC_Pawpocalypse mapping for mapping (`.RemapDefaults`).
- `Pawpocalypse.Input.RemapKeys`, `.RemapDefaults`, `.Remap` (a remap reaches what a key is, the menus, a prompt on screen,
  the field's context; a swap; the switches; a Godot-written file; Reset all controls); 12 game-module sabotages (`remap_*`).
**Slice 4c (2026-11-05, HANDOFF-LOG (48)): the Controls page.** `UPawControlsPanel` (`WBP_ControlsPanel`, grafted into
`WBP_SettingsDialog`'s Controls page under the two lists), controls_panel.gd: Show button prompts and Tutorial tips; the
binding table (a row per logical action: its name, a keyboard-and-mouse cell and a controller cell, each a button with the
binding drawn over it as a prompt (`UPawPromptIcon`, always shown), and Reset; built at run time into `BindingTable`); the status
line; Reset all controls; Swap confirm and back, Swap sticks, Invert X / Y; the cursor speed and stick dead zone sliders. The
rules are PawCore's `PcControls` (fixture `controls`: the REAL panel in the real dialog, 1,500 rows x 12 moves, exact; Godot's
Range snaps FROM ITS MINIMUM). While a cell listens the dialog hands the panel every key, key release, click, wheel turn and
analog value first (`NativeOnPreviewKeyDown`, `NativeOnKeyUp`, `NativeOnPreviewMouseButtonDown`, `NativeOnMouseWheel`,
`NativeOnAnalogValueChanged`): Esc / B cancel the capture, not the dialog; a click or the wheel cancels a controller cell; a
trigger's digital stand-in is swallowed and its analog value binds it past half way, as Godot. A prompt's binding is the
FIRST of the action's Godot events for the device (the live map): one Unreal has no key for (a pad's Guide button) draws
nothing, as Godot (`PawGlyphs::BindingFor`). The page redraws only when Godot's would (a control setting, the family, a cell
listening): F37, the controller column can show a pad detected before. `Pawpocalypse.UI.Controls.MatchesGodot` (200 fixture rows
on the shipped widgets, Slate's events through the dialog) and `.Dialog`; 12 PawCore and 9 game-module sabotages.

**Slice 5a (2026-11-05, HANDOFF-LOG (49)): keys by position, the dock's held keys, the paw, the pad in the background.**
- **Keys by POSITION, on any layout.** The plan assumed Windows `FKey`s are positions: they are NOT (`FWindowsApplication`
  builds them from the layout's virtual key: on AZERTY W's place is `EKeys::Z`; on the designer's Turkish Q keyboard US `-`'s
  place is `EKeys::Asterix` and `=`'s is `EKeys::Hyphen`, so the zoom keys sat one key off Godot's until this slice). Every key
  with a place (letters, digits, punctuation) now goes through the ACTIVE LAYOUT (`Input/PawKeyCodes`: US scan code ->
  `MapVirtualKeyEx` -> the `FKey` the engine makes of it; back by the character for letters and digits), the others (arrows,
  F-keys, Enter, Space, the modifiers) are the same everywhere. The prompts draw the labels the player sees. The game logs
  them at startup ("on this keyboard, up is W Up ..."). Automated runs pin US English (`-PawUsKeyboard`, run_automation.py:
  the fixtures' labels are Godot's headless US ones); `Pawpocalypse.Input.Azerty` injects French AZERTY (ZQSD, the prompt
  "Z", a capture of Z is W's place). Not yet: a layout switched while the game runs is read at the next apply.
- **The dock's held keys** (dock_navigator.gd `_tick_nav` / `_tick_press`, PawCore `PcDockRepeat`, exact against the real
  navigator in the real HUD: fixture `dock_repeat`): the dock POLLS what is held (`UPawInputDeviceSubsystem::IsActionHeld`,
  Godot's `Input.is_action_pressed`; a tap between two frames is latched), on its own clock, for every device: up / down step
  the row at once, then after 0.35 s at 10 per second, wrapping; a press presses at once, then on a SHOP row only repeats after
  0.35 s at 4 rising to 12 per second (hold-to-buy: Space, Enter, pad A, LT); nothing selected, the first press or step
  selects the remembered row. The dock TAKES those keys first (`ConsumeForDock`: the device subsystem's input processor is
  registered FIRST and consumes them while the dock is active), so Slate's navigation and Common UI's analog cursor (which
  turns pad A into a click) never act on them twice. The port's old hold-to-buy (any button held down, the mouse too) is gone:
  Godot has none for the mouse. The dock's tab keys act on their own press; an echo is swallowed.
- **The paw cursor** (§4): hardware cursors from `Content/Pawpocalypse/UI/Cursors/ui_cursor_paw_{open,closed}.png` (40 px,
  `tools/raster_svg.py --group cursor`, staged loose), the arrow and the pointing hand, closed while the left button is held,
  both loaded at startup. Seen in a `-game` run; a packaged build still to be checked.
- **A pad does nothing while the game is in the background** (project.godot `ignore_joypad_on_unfocused_application`): Slate's
  default, set by the game; keys held when focus goes are forgotten.
- **Not ported: an unfocused frame cap** (doc 04 §R, doc 09's gate list): Godot has none, so it is a design change, for the
  designer after parity (doc 01 §3).

**Slice 5b (2026-11-05, HANDOFF-LOG (49)): the pad-only playthrough and the focus tours.** `paw.PadPlay`
(Dev/PawPadPlay.cpp, Godot ci/check_padplay.gd) run by `py tools/padplay_tour.py` in a real `-game` window on a fresh
sandboxed save folder: pad buttons and a trigger sent through `FSlateApplication` (the path XInput's events take), from the
title through a new game, the shop (a press, LT held 1.4 s, X), a card (the guard), travel, the first boss's Bait, a Reawaken
(armed on Cancel, up, A), the tree (a node, B asks, B leaves), the pause menu, Settings (RB to Controls, B back onto Settings),
save and quit, and the title on Continue; at the title, the dock, the card pick, the tree, the pause menu and the settings
dialog THE FOCUS TOUR presses every direction and checks the focus is never lost. 41 checks. Found by it: pad A pressed a
dock row twice (Common UI's simulated click and the dock; above), and the pause menu lost its return to Settings (Common UI
re-applies the desired focus target when a modal closes: `UPawPauseMenu::ReturnFocus`). In the nightly `-Full`
("L4 pad-only playthrough").

| Godot action | Keyboard/mouse | Pad | Context (the plan; as built ONE context, `IMC_Pawpocalypse`) |
|---|---|---|---|
| `pounce` | LMB | RT (press EDGE only: a trigger is an axis) | `IMC_Field` |
| `steer_left/right/up/down` (Axis2D `IA_Steer`) | (mouse is the steering) | right stick, squared response, a full push crosses the field in `STICK_CROSSING_SEC` | `IMC_Field` |
| `recentre` | - | R3 | `IMC_Field` |
| `tab_prev` / `tab_next` | A / D, Left / Right | LB / RB, D-pad left/right | `IMC_Dock` |
| `menu_up` / `menu_down` | W / S, Up / Down | D-pad up/down, left stick | `IMC_Dock` |
| `press` | Space, Enter | A, LT | `IMC_Dock` |
| `buy_amount` (cycle x1/x10/xMax) | Q | X | `IMC_Dock` |
| `tab_1`..`tab_6` | 1-6 | - (keyboard shortcuts) | `IMC_Dock` |
| `open_skill_tree` | T | Y | `IMC_Global` |
| `open_upgrades` | U | - | `IMC_Global` |
| `back` | Esc | B | Common UI Back |
| `pause` | Esc | Start | `IMC_Global` |
| `ui_accept` / `ui_cancel` | Enter, Space / Esc | A / B | Common UI Click / Back |
| Tree: next affordable | Q | X | tree screen action |
| Tree: pan | (mouse drag) | right stick | tree screen |
| Tree: zoom | = / -, wheel | RT / LT | tree screen |

- **Every action has a keyboard/mouse binding AND a pad binding**, except a documented `ONE_DEVICE` list (steer/recentre
  pad-only; `tab_1..6`, `open_upgrades` keyboard-only). A test enforces it; adding to the list is a design decision.
- **UI actions never consume keys gameplay needs**: the KB's bug where a higher-priority UI context ate pad A `[KB U/08 §3.1]`.
- **A hidden focus still takes Space/Enter** (Godot 4.6+ and Slate alike: a clicked button keeps keyboard focus). Nothing may
  also handle `press` for a key the GUI already pressed, or one key buys twice. **"Click Reawaken, then Space" must not reset
  the run**: arming a two-press confirm moves the selection OFF the button (Reawaken -> Cancel).
- **A held key that opened a screen must not press a button on that screen** (release-after-open ignored) `[KB U/08 §3.3]`.
- Hold-to-repeat only on rows marked repeatable (shop rows), never on a decision.

## 4. The cursor, the cat and capture modes

**As built, phase 5 slice 3 (2026-11-04, HANDOFF-LOG (44))**:
- **The device rules** are PawCore `PcDevice`, exact against the real `input_device.gd` (fixture `input_device`: its
  `observe()` fed 7,000 synthesized events with real time between some, every step replayed at the clock it ran at; and
  `family_from_info` on 3,000 ids and names). Two questions, the prompt FAMILY and the CURSOR SOURCE; everything switches on
  a device's first input, never on connect; mouse motion steers but never flips the prompts (6 px within 250 ms takes the cat
  back); a stick or trigger past 0.5 claims the pad, the steer stick (right, left with Swap sticks) the cursor; the warp echo
  (120 ms); the press guard (400 ms, a press that began after the modal opened); a toast on a connect after the 2 s boot
  window and on the pad in hand going away (back to the mouse and keys, no pause).
- **`UPawInputDeviceSubsystem`** feeds it every Slate event before any widget (an `IInputProcessor` that consumes nothing,
  installed in a game, not the editor): keys, pad buttons (a stick's or trigger's digital stand-in key is NOT an event: Godot
  never sees it), analog axes (Unreal's Y flipped to Godot's), mouse motion, buttons and the wheel (Godot sends the wheel as
  button presses). Every pad is the Xbox family (XInput: "XInput Controller") until Steam Input; the Button prompts setting
  pins it. It hides the OS pointer over the open field and while the stick steers (`PC->CurrentMouseCursor`), shows the toast
  (`UPawToast`, `WBP_Toast`, `INPUT_toast_*`), applies Settings > stick dead zone to the steer actions (clamped 0.05 .. 0.9).
- **The stick-steered cat** (`APawRealmView`, player.gd `_process`): the steer stick through `get_vector` (and Invert X / Y),
  `stick_velocity` (normalized x |v|^2 x field width / 1.2 s, in Godot's float arithmetic, measured on 2,000 deflections)
  x Cursor speed, clamped to the playfield (`StickStep`, far edges in); the OS pointer warped onto the cat
  (`PC->SetMouseLocation`, the warp noted); R3's `recentre` claims the stick and puts the cat in the middle; the Cone of
  Shame freezes both paths. The pointer is over the field (hidden) inside the playfield and off the dock while the field runs
  (Godot asks "no GUI control hovered": the dock is the one panel over the field here).
- **The pad-audio toast** (2026-11-05; input_device.gd `_watch_for_pad_audio`, playtest 2026-10-01 #31): a pad connected after
  the boot window starts a watch: the output devices now and 2 s later (`Pc::DevicePadAudioWaitMs`, real time); a device that
  appeared while Settings > Audio > Output follows the system says "The controller added a sound device. No sound? Pick your
  speakers in Settings, Audio, Output." (`Pc::DevicePadAudioToast`; `INPUT_toast_pad_audio`). Each connect watches on its own;
  the setting is read after the wait, as Godot's `await`. `Pawpocalypse.Input.PadAudio`. The glyphs: slice 3b, §5. The paw
  cursors: slice 5a (§3).

- **Over open field the OS pointer hides and the cat IS the cursor. Over the dock, a popup or a paused game the pointer is
  the paw** (`ui_cursor_paw_open/closed`, closed with claws while the button is held). `UPawInputDeviceSubsystem` asks the
  field owner `PointerOverField()` (inside the field rect and no widget under the mouse, via Slate's widget path under the
  cursor).
- **Paw cursor: hardware cursors**, not a widget (software cursors lag; Epic Won't Fix) `[Forum]`: AS BUILT (slice 5a) two
  40 px PNGs rasterised from Godot's SVGs into `Content/Pawpocalypse/UI/Cursors/` (staged loose), set for the arrow and the
  pointing hand through `UGameViewportClient::SetHardwareCursor` once the game has its window, swapped while the left button
  is held. **Verify in a packaged build** (reports of cursors that only work in PIE). Fallback: a software cursor widget.
- **Mouse capture**: the HUD is a free-cursor mode: **`CaptureDuringMouseDown`, `DoNotLock`, cursor shown** (with
  `NoCapture` single clicks are lost and only double-clicks arrive; with `..._IncludingInitialMouseDown` pad A becomes a mouse
  click) `[KB U/08 §3.1-3.2]`. Pounce on LMB arrives through the field input path, not a widget.
- **One press path**: LMB-on-field, RT and any future key go through one `TryPounce(Source)` `[KB U/03 §7]`. **Interim
  (2026-10-31, phase 2 slice 7)**: `APawRealmView::SampleInput` reads LMB and RT (> 0.5, Godot's dead zone) raw into the
  field's `bPounceDown` (the field finds the press edge itself); replaced in phase 5 slice 2 by `IA_Pounce` (§3).
- **Pad virtual cursor**: the right stick moves a virtual cursor (the cat) that STAYS where it was left; R3 re-centres; while
  the stick steers, the OS pointer is hidden and warped onto the cat each frame (`FSlateApplication::SetCursorPos` /
  `APlayerController::SetMouseLocation`), and the device subsystem **ignores the mouse-move echo of each warp** for
  `WARP_ECHO_MSEC` (without that the stick loses the cursor on its first frame: a Godot bug). **Anything that follows "the
  cursor" asks `UPawInputDeviceSubsystem::PointerPosition()`**, never the mouse.
- Common UI's own analog cursor is NOT used for the field (the cat is the cursor); it may stay on for menus. It consumes D-pad
  keys before `NativeOnKeyDown` `[KB U/08 §3.3]`: dock navigation uses explicit navigation rules, not key handlers.

## 5. Device detection and glyphs

**As built, phase 5 slice 3b (2026-11-04, HANDOFF-LOG (45))**:
- **The art**: Godot's 65 generated glyph SVGs (`assets/art/ui/input/<family>/`, from its `tools/sweet_input.py`; since phase 6
  generated here by `tools/art`, `SourceArt/svg/ui/input/`) are rasterised at **56 px** (2x the largest prompt, 28: doc 07 §2.2) by `tools/raster_svg.py`
  (`--check` reproduces them), then imported headless by `tools/import_art.py --run` as `T_input_<family>_<position>` under
  `/Game/Pawpocalypse/Art/UI/Input`: UserInterface2D, sRGB, no mips, Never Stream, the UI group. A glyph change: the
  generator in `tools/art` (ours since D14, 2026-11-07), then `build_art.py`, `raster_svg.py`, `import_art.py --run input`.
- **The rules** are `UI/PawGlyphs` (Godot `input_glyphs.gd`), exact against the real InputGlyphs (fixture `input_glyphs`:
  every action x family, 160 rows; every key the port binds, its label; the {token} replacement of the game's templates in
  every family, 50 rows): the action's FIRST binding the family's device can press, read from the live context (a remap
  moves every prompt); pad glyphs by POSITION (`face_down`), GENERIC drawing as Xbox; mouse buttons and the four arrows from
  the keyboard folder; any other key a keycap around its label (a printable key its character, upper-cased; Space / Enter /
  Esc / Tab / Shift / Ctrl / Alt / Backspace words from ST_UI `KEY_*`; the rest Godot's engine name, untranslated as there);
  in a template only names that ARE actions are replaced (`{n}` passes), and an action with nothing on the device leaves
  nothing.
- **The widgets** (Widget Blueprints, `PawUi::BakedClasses`): `UPawPromptIcon` (`WBP_PromptIcon`: a glyph, or a keycap:
  a Plum Raised key with a Bone label (restyle R3; Godot's was paper), ink frame 2 with a 4 bottom edge, corners 5) and `UPawPromptText` (`WBP_PromptText`: a rich text block whose
  decorator `UPawPromptDecorator` draws `<prompt g="stem" px="22"/>` as a glyph and `<prompt k="Space"/>` as Godot's inline
  keycap, bold Bone on the slot colour, Deep Plum since R3). Both redraw on `OnFamilyChanged` and on any settings change (Button prompts, Show
  button prompts); a prompt line hides whole with Show button prompts off, a tooltip's line never does (Godot's tooltip
  calls `InputGlyphs.bbcode` directly).
- **Where they are** (Godot's places): the HUD's tab keys flanking the tab strip (26 px), the dock legend at its foot
  (`HUD_legend_upgrades` / `HUD_legend_other`, 14 px text, 22 px prompts), the Pounce key beside the clock on the field side
  (28 px, shown with the clock); the title's hint (24 px); the tree's legend and every tooltip line (22 px: the buy line and a
  blurb's `{pounce}`); **the press prompt on the focused button**, one prompt on the top layer owned by
  `UPawInputDeviceSubsystem` (Godot's `focus_prompt` and the dock's `_row_prompt` in one): shown while keys or a pad drive
  the menus, inside the button's right edge (inset 10, 24 px), or in the dock OUTSIDE the row on the field side (gap 10,
  26 px; the HUD reports its dock and side, `SetDockArea`).
- New parts went into the existing screen assets by **grafting** (`py tools/bake_widgets.py --run --graft
  "Class=add:Part;swap:Old>New"`): added beside their nearest sibling, or swapped into the old part's slot (the title's
  `HintText` -> `HintPrompts`, the tree's `Legend` -> `LegendPrompts`, `TipLine0..6` -> `TipPrompt0..6`); nothing else in
  an asset is touched.
- Seen in a `-game` run (`paw.Press`, `paw.Prompts`: Dev/PawShot.cpp): keycaps on the keys, Xbox glyphs after one pad
  press, PlayStation glyphs from the setting; the HUD, the pause menu, the tree, the title.
- Not yet: the tutorial's prompts (the tutorial is phase 6), the Controls page's binding cells (slice 4c), a redraw on a
  language change (phase 7). A remap redraws every prompt since slice 4b (`UPawInputDeviceSubsystem::OnControlsChanged`,
  fired after the live context is rebuilt; our key questions read the context we rebuilt, so Enhanced Input's own
  `ControlMappingsRebuilt` is not needed for them). The tab names now share the strip with the
  tab keys; "Achievements" touches the RB cap until the tab icons arrive (phase 6: Godot's tabs are icons).

- Two separate questions, as in Godot: the **prompt family** (keyboard, Xbox, PlayStation, Steam Deck; an unknown pad draws
  as Xbox; **no Nintendo**, designer 2026-10-01) and the **cursor source** (mouse or stick). Both switch on a device's first
  INPUT, never on connect (a connect is a toast). Mouse MOTION changes the cursor source, never the family (the Deck trackpad
  rule). Base it on `UCommonInputSubsystem::OnInputMethodChanged` `[KB U/08 §5]`.
- **Windows pads: Unreal reads XInput only** (GameInput plugin off by default): every pad looks like Xbox, and a PlayStation
  pad plugged in directly does not work at all; through Steam Input it works as Xbox and the real type comes from the Steam
  API `[KB U/08 §5.1]`. Settings > Controls > Button prompts (Auto / Xbox / PlayStation / Steam Deck) is the override until
  Steam integration lands (doc 10 §5). Decide in phase 5 whether to enable the GameInput plugin for direct DualSense support.
- **Glyphs are generated** (in the Godot repo: `tools/sweet_input.py` via `sweet_ui.py --write`; copied and rasterised here by
  `tools/raster_svg.py`, as built above) per family, named by POSITION (`face_down` is
  A on Xbox, Cross on PlayStation): import them as one texture set per family into `CommonInputBaseControllerData` assets.
  **Keyboard keys are keycaps drawn at runtime around the key's live display name** (remaps and AZERTY read right); only
  arrows and mouse buttons are images (Godot's font had no arrow glyphs; F_Paw has them, measured 2026-11-08).
- **No hardcoded prompt text**. Strings carry action tokens (`"a burst on {pounce}"`); a `UPawPromptText` (rich text
  decorator that resolves `{action}` to the current glyph) and `UPawPromptIcon` redraw on device, language or binding change
  and on **`ControlMappingsRebuiltDelegate`** (a legend built before contexts apply shows no glyphs) `[KB U/08 §4]`.

## 6. The screens

### 6.1 HUD and dock (`UPawHudScreen`, `UPawDock`)

**Built 2026-11-01 (phase 3, first slice; HANDOFF-LOG (21))**: `UPawRootLayout` (Game / Menu / Modal stacks, created by
`APawPlayerController::BeginPlay`, the HUD pushed on Game in a map with a realm view); `UPawHudScreen` with the status row
(Catnip, Stardust, the wave line, the zone plate with its rainbow frame), the Catches XP bar, the Pounce clock
(`UPawPounceClock`, painted), the boss panel (status, HP bar recoloured while shielded, the fight clock, the DEFEATED / OUT OF
TIME after-word until travel or the next summon), and the dock (one `UPawDock` was not needed: the dock is part of the HUD
screen): six tabs with badges and Jobs hidden until a job is held, the Upgrades page (15 rows, Buy x1/x5/x10/Max, hold-to-buy,
the Kitten lines locked until a kitten), side flip (`SetDockOnLeft`), the dock's rect reported to the realm
(`UPawRealmSubsystem::SetDock`), which measures the field beside it and refuses a Pounce press over it. What each readout says
is PawCore's `PcHud` (the decisions of hud.gd, L1-tested); the widget turns it into `ST_UI` / `ST_Data` text. Placeholders:
the badge star is a drawn disc (Godot's font had no star glyph; F_Paw has ★ ☆ ◆ since restyle R2, so the real marks are possible: open); the tab icons, currency icons and the Pounce clock's paw
since phase 6 slice 5b (§7); every page is built since slice 3; the dock's keys (tabs, buy
amount, 1-6) are read in `NativeOnPreviewKeyDown` through the input map since phase 5 slice 2 (`PawInput::KeyIs`, §3); the quick bar (settings, tree, save, quit)
since phase 6 slice 5b (§7); the fps readout, tooltips and the tutorial overlay are not built.

**Slice 2 (2026-11-01, HANDOFF-LOG (22))**: the **Realms page** (the stage line naming the boss in the way, the hint, the Bait
button in every biome with its five states, the auto-travel box, twelve rows in a scroll box: here / travel / boss (cleared) /
earn more / locked, Cartography's reward, the newly-open row warmed) and the **Reawaken page** (progress toward the first or
the suggested point, the payout with its Stardust disc, the total after, the multiplier now and after, the Unicorn line, the
button's four states, the two-press confirm that moves the selection to Cancel, Cancel, the "Ask before Reawakening" box;
a tab change disarms). The reset runs `PcProgress` `Reawaken`, then the realm's `OnReawakened`, then travels to the starting
biome (`Pc::ReawakenTravelIndex`: the first, or Territory's; designer 2026-11-01 "Territory wins", F20); the realm itself
never travels on a Reawaken. Since (25) and (26) the HUD runs Godot's whole order before that travel: the ceremony, the
tree's spend step, the job picker (§6.3).
**Rows are LOCKED, not disabled** (`UPawButton::SetLocked`: Common UI's interaction off, Slate-enabled, a locked style that
keeps the focus ring): Slate never focuses a disabled widget, and Godot's navigator selects unaffordable rows because the
player reads them. Every page's column (`PageColumn`) includes its locked rows.

**Slice 3 (2026-11-02, HANDOFF-LOG (23))**: the **Catches page** (the summary: none / one / n cards, "Picking automatically.";
the Auto-pick box, shown once Auto-Card is owned, writing the realm's setting; EVERY card in deck order as a flat, selectable
row (`UPawButton::SetFlat`, `SetAlignLeft`, `SetDetail`: a 21 px heading and a second line): held = "<name>  (Lv n/max)" in
the good colour over the card's description with `{v}` / `{c}` filled (`PcHud` `CardNumber` = `CardData._fmt`); not held =
the name, muted, over what stands in the way (`Pc::CardUnlockHint`, the gates in `eligible_cards()`'s order) or "Not taken
yet."), the **Achievements page** (the summary with what waits; Claim all with the sum; one row per achievement: a mark, the
name, the description or progress, and a Claim button with its amount, built once and hidden; claimed / earned / secret /
progress; each Claim and Claim all pays once; the selection moves when the pressed button goes) and the **Jobs page** (the
blurb; one row per job in `all_jobs()` order: held = description, the text colour when worn (the run's job), muted when only
owned; else the step in the way: no node / closed by a rival / beat a boss / Reawaken n times / ready at the node's price; a
Worn / Owned / Locked button that is a statement, always locked). The decisions are `PcHud` `CatchesPage`,
`AchievementsPage`, `JobRows` (L1 + sabotages). Placeholders: Godot's row marks ★ ◆ ☆ are drawn discs (Godot's font had
no star glyph; F_Paw has them since R2: open); the job icons since phase 6 slice 5b; the hover tooltips (every card row, the quick bar) are not built; the tabs
are icons since slice 5b (the Achievements tab's count beside the trophy: whether Godot shows it is open, HANDOFF-LOG (59)). **Achievements are earned by the realm** (found by the
screenshot tour: nothing evaluated them): `FPawRealmSim` re-scans after every fixed step (Godot re-scans on every kill and
currency change) and `UPawRealmSubsystem` after a progress change, a summon and a Reawaken; the earned ids queue in
`EarnedAchievements` for the toast (not built). The card pick's decisions are in `PcHud` `CardPick` (L1 + 11 sabotages).

**Slice 4 (2026-11-02, HANDOFF-LOG (24))**: the **card pick** (`UPawCardPickScreen`, `catches_pick_ui.gd`), on the Menu stack.
- **Who opens it**: the HUD, every frame (`UPawHudScreen::OpenCardPickIfDealt`), while the progress holds a hand and the pick is
  not already on the Menu stack (the player controller hands the HUD its root layout's Menu stack). A hand dealt by a kill and a
  hand that was up when the realm started open the same way; the stack's widget list is the one guard against a second screen.
- **The field holds** while it is up: `UPawRealmSubsystem::SetPauseHold("card_pick", ...)` from `NativeOnActivated` /
  `NativeOnDeactivated`. The realm counts HOLDERS (`SetPauseHold(FName, bool)`; `SetPaused` is the "manual" holder): one screen
  closing never resumes the field under another (Godot's `_other_modal_open()`), and a restarted field starts held. The tree,
  the ceremony and the pause menu take their own holders.
- **No Pounce through it**: the dim is `Visible` (eats every click), and `UPawRealmSubsystem::SetPounceDown` swallows a press
  while anything holds the field AND after, until the button is let go: Godot fires Pounce from `_unhandled_input`, so the click
  that takes a card is never a Pounce; the port polls the button, and the field's edge detector would otherwise see the still-held
  button as a fresh press on the first step after the pick closes.
- **Hold-to-buy stops** while the pick is up (Godot's paused tree stops the dock); the held row starts over after it.
- **The cards**: `UPawCardFace` (a Common button that is never focusable: the whole-card click) around a Bone plate (cream until restyle R3; its text stays ink) with a 3 px
  ink frame (the alert red when the card costs something), the flavour word on a band in the flavour's colour, the name, NEW (alert
  colour) or "LEVEL n of max" (muted), the description at the level taking it gives (`{v}` / `{c}`, `PawText::CardNumber`), "This
  one costs you something.", Take, and Banish under it while a banish is held (smaller, alert-coloured). A hovered card, or one
  whose Take / Banish is selected, lifts to 1.06x over 0.09 s (render scale: 9 px a side fits the 20 px gap, so no z-order is
  needed). Cards are pooled across deals (Dilated's bigger hands add one). The card art waits for phase 6 (the layout keeps its
  place, as Godot's cards without art).
- **No close, no Back** (Godot's header: the only exits are a card taken, or a banish that empties the deck). Reroll re-deals in
  place; a stale card (not in the live hand) logs an error and closes rather than trap the player.
- **The guard** (`GuardAllows`): a press counts once `ModalGuardSec` (0.4 s, `MODAL_GUARD_MSEC`) has passed since the DEAL (a
  reroll's and a banish's included) and, for the keys and the pad, only a press that began strictly after the deal (the Space
  held to buy, its repeats, the A that pressed Reroll). Godot compares milliseconds with `>=`, where the press that dealt the hand
  can land on the deal's own millisecond; strictly-after is what its comment says it means. The guard sits on the buttons
  (`Click*`); `Take` / `Banish` / `Reroll` are the programmatic way in (tests, later the soak and the playthrough).
- **Pad and keys**: rows Take / Banish / Reroll wired by `PawUi::WireRows` (the port of `MenuFocus.wire`: wrapping at every edge,
  up / down to the nearest in x, which for centred rows of equal widths is the centred share); opens on the first Take; a re-deal
  focuses it again (only with a local player: the automation world has none).
- **Input config**: `Menu`, `NoCapture` (its buttons are Slate widgets; the viewport capture rule of KB U/08 §3.2 is for clicks
  on the field). The root layout's stacks now have no transition (KB U/08 §2 rule 6), so a screen is up the frame it is added.
- Tests `Pawpocalypse.UI.CardPick.*` on the REAL root layout (its Slate built with `TakeWidget()`, so the stack activates what it
  holds). **Not covered**: the hover / focus lift (needs Slate hover), the held-key path through real Slate key events (the guard's
  notes are driven directly: phase 5's tours), hold-to-buy stopping under the pick (hold-to-buy itself is untested).
- **Dock on the LEFT** by default, the status row and zone bar top-RIGHT, mirrored; **Settings > Controls > Menu side** flips
  both. `UPawHudScreen::ApplyDockSide()` is the one place that knows a side; `playfield_rect()` reads where the dock IS.
- **Tabs**: Upgrades, Reawaken, Realms, Catches, Achievements, Jobs: icon-only, one row, badges that mean something NEW,
  selected tab remembered across travel (static/subsystem state). New page = a page widget + one `TabSpec` row.
- **Shop rows** (15, never scroll): name, level, effect, price with currency icon, bulk amount; the enabled buy button is the
  brightest thing; hold-to-repeat; disabled rows read `font_disabled_color`-equivalent (a Slate disabled state ignores the
  normal colour: set the disabled style explicitly).
- **Status row**: transparent, `HitTestInvisible` except the two currency icons with tooltips (the Godot opaque panel once
  ate clicks aimed at mice). **Every widget over the field is `HitTestInvisible` or `SelfHitTestInvisible`** unless it must
  take input (a full-screen visible widget eats clicks: Godot's `MOUSE_FILTER_STOP` lesson).
- **Reawaken tab**: progress to threshold, pending Stardust, multiplier before -> after, then the button; "Ask before
  Reawakening" box; confirm moves focus to Cancel.
- **Realms tab**: twelve rows; locked rows name the boss or the Catnip still needed; current biome visibly distinct from
  locked (set the DISABLED style explicitly); Bait button on every biome; "Bait ready" rainbow plate.
- **Dock navigation** (Godot `DockNavigator`): ONE highlight; the selection IS focus; a real mouse hover selects too; a mouse
  selection ends when the mouse leaves the row; the first key with nothing selected SHOWS the remembered row rather than
  acting. In Unreal: explicit navigation rules on each row (`SetNavigationRuleExplicit`), focus = selection, hover sets focus.
- Refresh: event-driven dirty flags spent at 15 Hz; hidden pages do nothing; no UMG property bindings `[KB U/09 §1]`.

### 6.2 Skill tree (`UPawSkillTreeScreen`)
**Status 2026-11-02 (HANDOFF-LOG (24))**: the PawCore half is DONE: `PcTreeLayout` (branch order, the hex seating, limb push, separation, normalising, bounds, canvas, fit zoom, `nav_pick`, node states, links, cheapest / next affordable), every seat and pad pick EXACTLY Godot's on the shipping tree and 400 random trees. It reproduces Godot's 32-bit `Vector2` arithmetic: in plain double 4,496 pad picks and 147 seats differed (the lattice puts neighbours exactly on the 60 degree cone edge). NOTE: the tree today has **87 nodes** on a radial hex layout of six limbs (`TRUNK_WHEEL`), not one lattice; Stardust is spent ONLY in the Reawaken spend step (`open_for_spending`), browsing is read-only. The screen is next.
**The screen, 2026-11-02 (HANDOFF-LOG (25))**: `UPawSkillTreeScreen` on the Menu stack (pause holder "tree"), as designed above: one canvas panel under a render transform (pan, zoom about the view's centre, fit on open), `UPawTreeLinks` painting links (owned only with both ends) and the rainbow / pulse / job rings under the nodes, `UPawTreeNode` (a Common button; the frame of its state in the trunk's colour, READY lerped a third of the way, unlocks heavier and rounder; the counter on ladders; the shine), gold halos breathing behind READY nodes, captions (BRANCH_LABELS), the hub plate, the Stardust line, +/- buttons, the legend, the tooltip (beside the mouse's node, docked bottom left for keys / pad). Browse: T / pad Y (the HUD), read-only, X. Spend: the HUD's Reawaken opens it (Continue; the cheapest node selected; a press buys; the two-press leave with the centre plate), then travels to the starting biome. Every word and colour of skill_tree_ui.gd is GENERATED (`tools/gen_tree_tables.py` -> ST_Tree.csv, PcTreeWording.h, PawTreeTables.h; L0 checks them current). **Placeholders**: key names for the glyphs (`PROMPT_*`, phase 5), the sky (skill_tree_sky.gd: since phase 6 slice 5d), no node icons (phase 6), the halo without its soft shadow, no hover lighten, no READY jitter (UiJuice), no Unicorn ceremony. (The ceremony before the spend step and the job picker after it: §6.3, 2026-11-02.)
- 87 nodes on a hex lattice, links drawn between them, pan and zoom, three node states, rings/halos/pulses.
- Build: one C++ widget; **links and decorations drawn in `NativePaint`** (`FSlateDrawElement::MakeLines` with
  anti-aliasing, or `MakeSpline`), **nodes as `UCommonButtonBase` children** positioned in ONE canvas panel inside a
  pan/zoom container (render transform). One canvas is acceptable; nested canvases are not `[KB U/09 §1]`.
- Lattice positions computed by the ported layout code (`skill_tree_ui.gd`'s layout section); a test asserts every data node
  is seated, nothing is seated without data, spacing is even, no link crosses a node or another link (all four were Godot
  `check_hud` assertions).
- **Pad navigation from the lattice**: each node's four neighbours written once per layout (`nav_pick` port) as explicit
  navigation; a test walks every node from where the tree opens. Q/X next affordable; right stick pans; RT/LT zoom.
- Tooltip docked (pad) or following (mouse). Unicorn purchase plays `UPawUnicornCeremony` over the tree (AS BUILT, phase 6 slice 6a: the tree's last part, collapsed until it plays; while it plays it eats every click, holds the keys (the tree hands it each one and buys nothing: Back cannot close the tree), a press after 0.6 s skips to the fade, and the tree takes its focus back when it ends). **Boba never over the
  tree** (`UPawBobaSubsystem::Suppress()`, which also cuts a cameo in flight).
- Asks before every return to the game after a Reawaken.
- **S4 measured**: 87 Common-button nodes in one canvas, links in a `HitTestInvisible` paint widget beneath them, geometry
  navigation written once: all 87 x 4 rules verified by pressing them; opening the screen costs 6-9 ms; auto-pan by render
  transform kept every focused node on screen at 2x zoom.

### 6.3 Others

**The Reawaken ceremony and the job picker, 2026-11-02 (HANDOFF-LOG (26))**. `UPawHudScreen::DoReawaken` runs hud.gd's
order: the payout read (`PendingStardust`, BEFORE the reset: Second Helping defers half of what lands), `Pc::Reawaken`, the
realm's `OnReawakened`, then **`UPawReawakenCeremony`** (reawaken_ceremony.gd): the HUD's top layer, hit-test invisible, the
flash and "REAWAKENING" fading in (0.35 s), the payout "Reawaken n / +s Stardust" (0.25 s), a 1.1 s hold, all three out (0.45 s),
one sine in-out tween (`AlphasAt`, a pure function the tests pin). Godot does **not** pause for it (a CanvasLayer at 10, over
the HUD and the tree at 1, under the card pick at 100): the field runs under the flash; its clock is the HUD's `Pump`, and only
while the realm runs, because Godot's tween is bound to a pausable node (a card dealt under the flash freezes it). Its three
colours are the scene's literals, generated into `PawTokens.h` (`CEREMONY_*`, `tools/gen_paw_tokens.py` SCENE_COLOURS). Its
end opens the spend step (the tree); a screen still on the Menu stack makes it wait (`IsSpendStepPending`), never skip.
Leaving the spend step calls `ReturnToStartingBiome`: when `ShouldAskForJob` the **`UPawJobPicker`** (job_picker.gd) opens on
the **Modal** stack (pause holder "job_picker"; the player controller hands the HUD that stack), and the travel waits for its
answer. What it offers is `PcHud JobPicker` (exact against the real job_picker.gd, fixture `job_picker`): the held jobs that have
data, by id; the previous choice "(last run)" and selected; "No job" last. Under Union Rules a first answer that leaves another
job held asks again in place: the primary not offered, "At n% strength." on every blurb, "No second job". The guard is the card
pick's (`GuardAllows`; the second ask restarts it). One column wired by `WireRows`; no close, no Back (the baseline declines).
**Differences kept on purpose**: the ceremony sits under the Menu stack, so a tree browsed from under the flash draws over it
(in Godot the frozen flash covers the tree); job_taken's save and the hat's ring arrive with phases 4 and 6. **The picker wears
the game theme** (finding F21, decided 2026-11-03, fixed in Godot first: `_root.theme`): the theme's `WindowPanel` (paper, ink
frame, content margins 22 / 20: `PawUi::WindowBrush()` / `WindowPadding()`, drawn as a rounded brush until the art, phase 6),
the theme's buttons, ink text (the note 17 px). Measured into `victory_look_godot.txt` (`jp.*`). Until 2026-11-03 Godot drew
it in its DEFAULT theme (a CanvasLayer outside the themed Root), the title dark on dark.

**The victory screen, 2026-11-02 (HANDOFF-LOG (27))**. **`UPawVictoryScreen`** (victory_screen.gd) on the **Modal** stack
(Godot: CanvasLayer 128, over the card pick's 100), pause holder "victory" (Godot pauses the tree and restores what it found;
holders do that by construction). The HUD raises it from the field's `BossDefeated` event flagged `FieldFlag::Victory`, i.e.
`PcProgress RaisesVictory`: boss_defeated emitted (a first clear, or the first rematch of a cycle) for the LAST boss realm,
never another boss, never a rematch already cleared this cycle; it shows `FinalBossSeconds`, the first clear's time, through
`PawText::Duration` (`PcHud DurationParts`: "{h}h {m}m {s}s" / "{m}m {s}s", two-digit fields, negatives read 0). Both exact
against the real hud.tscn's VictoryScreen (fixture `victory`). A second show while it is up is ignored. Dim (eats clicks),
"THE VOID IS QUIET", the flavour line, "Cleared in ...", OK (focused on show; no guard, no Back: OK is the way out).
**It wears the game theme** (finding F21, decided 2026-11-03, fixed in Godot first): a themed root, and the column on a 640-wide
`WindowPanel` (the theme's ink labels would be dark on the dark dim), the theme's OK button, ink text (the flavour 17 px).
Measured from the real screen into `Tests/Data/victory_look_godot.txt` (the fixture writes it). Until 2026-11-03 Godot drew it in
its DEFAULT theme (white labels, the engine's grey button). **Differences kept on purpose**: the selection ring also shows on
a mouse hover (Common UI's hovered state is the selection; Godot rings only the focused button); no Boba "victory" visit or
nyan fly-past (BobaCameo, phase 6).

**The title screen, 2026-11-04 (HANDOFF-LOG (37); phase 4 slice 3)**. **`UPawMainMenu`** (main_menu.tscn / main_menu.gd) on
the Game stack of L_Title, the game's first map: the player controller pushes it wherever there is no realm. Its rules are
`PcTitle` (exact against the real main_menu.tscn, fixture `title`: Continue needs a save on disk or progress in memory, Load a
save; the status line counts the game IN MEMORY; the cards' play clock (`format_duration`: "{h}h {m}m" / "{m}m {s}s" / "{s}s"),
kills ("{n}K mice", "%.1f"), "last saved" (just now / min / hr / days, past a week the stamp's UTC date); the leaderboard's last
line; the slot screen opens on the last slot used). One widget with Godot's layers, each a full-screen scrim: the slots
(New Game and Load open the same picker; an empty card starts a game THERE, an occupied one loads it, no confirm), the
leaderboard placeholder, the credits, the shared confirm (Cancel under the key; one-shot). `FocusTop` is `_focus_top`: only
the top layer is wired (`WireRows`; disabled menu rows dropped: a door that cannot open is a dead stop for a pad) and its
default takes the selection; Back (the map's `back`: Esc / pad B) closes the top layer and on the menu asks "Quit Pawpocalypse?".
Exit saves a game worth saving, then quits. **The save traps of HANDOFF-LOG (34) are closed here**: a new game is `NewGame`,
an empty achievement load (`AchievementManager.apply_save_data({})`), then `SaveGame(slot)` in the same frame (that slot becomes
active and the end-of-frame digest is reseeded: the old slot keeps its game, `Pawpocalypse.UI.Title.NewGame` checks it byte for
byte); a load is followed by `OpenLevel(L_Realm)`, whose realm starts from the loaded game. Seams for the tests: `OnPlay` (open
L_Realm), `OnOpenUrl` (the browser), `OnQuit`, `Now`. Its words are `TITLE_*` in ST_UI (line breaks as `\n`, which the string
table import unescapes: the card texts equal Godot's). **The garden since 2026-11-05** (phase 6 slice 2): `UPawGarden`
(`WBP_Garden`, grafted into `WBP_MainMenu` as `Garden`, right over the paper floor), the "menu" backdrop painted in Slate and
swaying (doc 04 §5.1). **Not yet, each with its phase**: Boba's photo and the cameo on a click (the frame and caption keep his place), the Language list (phase 7: the button is there,
disabled). Since built: Settings (the dialog over the title), the last slot used (kept in the settings, `UPawSaveSubsystem::StartOnLastSlot`),
the cluster's icons (phase 6 slice 5b: no words on the plates) and their hover pop (slice 5c). The slot cards swell by size (1.10,
0.12 s, the column with them, so neighbours move) on hover or selection; linear where Godot's tween is cubic ease-out.

**The pause menu, 2026-11-04 (HANDOFF-LOG (39); phase 4)**. **`UPawPauseMenu`** (pause_menu.tscn / pause_menu.gd), a Widget
Blueprint from its first build (`WBP_PauseMenu`), on the Menu stack, pause holder "pause". Its rules are `PcPause` (exact
against the real pause_menu.tscn, fixture `pause`, every press of 600 rows in L1, 2,000 in the long suite): Continue; Save
opens the five slots (an occupied one asks first: "OVERWRITE? press again" on the button, the status line on the main page;
the second press writes and returns to the main page; an empty one writes at once); Back to Main Menu arms on its first press
("Save and quit to menu", the selection stays on it) and its second saves the game and opens L_Title; "Quit WITHOUT saving"
shows beside it while armed and leaves without writing; closing disarms both (reopened, one press never quits). **AS GODOT HAS
IT**: an overwrite armed on the Save page survives Back to the main page (finding F34, open). The slot captions are read again
each time the Save page opens and on arming, never between (Godot's are stale too). The HUD opens it on Esc / pad Start
(`OpenPauseMenu`) only while nothing is on the Menu or Modal stack and nothing else holds the field, and not on the frame after
something else let go (`_held_by_other_frame`: the Esc that closed the tree must not open this); the ceremony does not block
it (Godot runs the ceremony unpaused; the hold freezes it). In the menu, Esc / pad B are back (the Save page returns to the
main page first) and pad Start closes from either page. Words: `PAUSE_*` in ST_UI; Godot splices " (current)" in code, the
port has one key per slot form with it and one without. Seam for the tests: `OnLeave` (open L_Title).
`Pawpocalypse.UI.Pause.*` (6 tests; the words equal Godot's on 300 fixture rows). Settings (since HANDOFF-LOG (40)) opens the
settings popup; the menu steps aside (hidden, still holding the field) and comes back on Settings when it closes. **Not
yet**: the "CEO on duty" photo beside the window and Boba's pause cameo (phase 6 slice 6). The open's click since phase 6 slice 4.

**The settings popup, 2026-11-04 (HANDOFF-LOG (40); phase 4)**. **`UPawSettingsDialog`** (settings_dialog.tscn /
settings_dialog.gd), a Widget Blueprint from its first build (`WBP_SettingsDialog`), on the **Modal** stack, pause holder
"settings"; opened by the pause menu's Settings and the title's. Its rules are `PcSettings` (exact against the real
settings_manager.gd and settings_dialog.tscn, fixture `settings`: the settings.cfg Godot's own ConfigFile wrote, then six
changes on the live dialog per row, 1,000 rows in L1, 3,000 in the long suite). Three sections (Audio, Video, Controls) in a
bar of toggles (the selected one wears the theme's pressed plate), a page each, Close. **Audio**: Master (step 0.05), Music and
Sound Effects (0.01) sliders with their percent lines, "Mute all audio", Output. **Video**: Language (disabled, "System":
phase 7), Resolution (the five window sizes; greyed while fullscreen as the dialog OPENS, as Godot: finding F35), Fullscreen,
V-Sync, Graphics quality, Frame rate limit, Visual effects, the note. **Controls**: Button prompts, Menu side, their notes.
The selection: the section bar, the page's live controls one per row (a disabled list drops out), Close; it opens on the
page's first control. Esc / pad B close; Page Up / Down and LB / RB turn the section (not A / D or the arrows: those move a
slider; a slider moves on left / right at once, no "press to lock"). The lists are `UPawOption` (a combo box in the theme;
up / down leave it, A opens it). Every control writes through `UPawSettingsSubsystem` (docs/08 §3); the controls are set
from the settings on every open without counting as changes. **Godot's numbers, measured**: a slider snaps as
`floor(v / step + 0.5) / (1 / step)` (the nearest double to the decimal; `k * step` misses by a bit on 700 of 4,912 moves,
and `round` gives -0 below zero where Godot gives +0); a volume line shows `roundi(value * 100)`; a file's volume out of 0..1
is kept and shown (130%) while its slider rests at the end. Words: `SETTINGS_*` in ST_UI. `Pawpocalypse.UI.Settings.*` (6
tests; the dialog equals Godot's on 300 fixture rows). **Not yet**: the Output list's system devices (Godot sets AudioServer.output_device; the sound plays since phase 6 slice 4,
always to the default device: open); the language list (phase 7); the quick bar
that opens it from the HUD (it waits on its icons); the separator's art (a plain rule).

**The screenshot tour (2026-11-02, HANDOFF-LOG (28))**: `py tools/godot_tour.py` runs the same steps and state in both games
(Godot: `tools/godot_fixtures/tour_probe.gd` in the sandboxed scratch copy, rendered; Unreal: the `paw.*` tour steps of
`Dev/PawShot.cpp`, capped at 60 fps) and pairs the shots in `build/tour/compare/`. The dims measure equal to Godot's (the
dock's paper under each: within 1/255). Differences left are the art, fonts, tab icons and tutorial of phase 6.

**Numbers are printed the Godot way (2026-11-02, designer: "match Godot")**. Godot formats an int with `str()` and decimals
with `"%.Nf"`: never grouped, a '.' point in every language. `PawText::Count` (ungrouped digits), `TwoDigits` ("%02d") and
`Fixed` ("%.Nf") are culture-invariant; the L0 gate `ui-rules` fails on `FText::AsNumber` / `AsPercent` / `AsCurrency` in UI
code. (Amounts.short stays `PawText::Amount`.)

Card pick (3 cards, keep one; auto-pick node: built, §6.1 slice 4), job picker, pause menu (Save, Settings, Quit to title; quit saves first),
settings popup (Audio / Video / Controls with remapping rows: a row is a LOGICAL action writing several Enhanced Input
actions, Esc/Start/B are system keys and can never be taken, Reset button), victory screen, main menu with five slots,
ceremonies, the tutorial overlay (`HitTestInvisible`, never focusable), cheat overlay (F1, dev only).

## 7. Look: tokens, brushes, fonts

- **`PawTokens.h`** generated (as built: `tools/gen_paw_tokens.py` from `tools/art/palette.py` since R0, 2026-11-07; before that Godot's `palette.gd`) (colours, the UI panel/button/text/focus colours, radii, frame width,
  font sizes). Screens never hold a hex value; a test greps for `FLinearColor(` / `FColor(` literals outside the generated header.
- **Panels and buttons are `FSlateRoundedBoxBrush`**s with an outline where Godot draws a `StyleBoxFlat` (Godot's: cream `#FFF9E3`, 3 px `#1A1A1A`, radius 6-8;
  **since restyle R3, 2026-11-08, the plum tokens**: panels Plum `#2A1A33`, buttons Plum Raised `#3E2A4A`, pressed and
  slots Deep Plum `#120A17`, text Bone `#F6EEF8`, muted text Lilac `#DCCFE4`, labels over the field Bone with an ink
  outline (`UI_FIELD_LABEL`), card plates Bone with ink text; **since R4a the focus ring is Butter** (4 px, 4 px out) and
  "the selected thing" is a Butter plate with ink words (the open tab: chrome `ui_button_selected`; a call:
  `UPawButton::SetCallLook`); since R4b no text under 16 px (`PawUi::UnderTextFloor`): `docs/WORKLIST-RESTYLE-2026-11-07.md`). **R3's shapes (2026-11-08)**: every plate casts one solid Deep Plum shadow
  straight down and has a Deep Plum edge, so the code's flat plates (the dock, the boss panel, the zone plate, the title's
  menu, windows and badge, the pause and settings windows, the tree's warning and tip, the toast) wear the chrome's Window
  or small plate (`EPawChrome::PlateSmall`), and `PawUi::PlatePadding` keeps content off the shadow. The pressed button
  sinks onto its shadow; buttons' content margins 16 / 4 / 16 / 8 (the label centred on the plate). **Where Godot's theme draws its nine-patch art, the port draws the same art (phase 6 slice 5a,
  2026-11-06)**: `PawUi::Chrome(EPawChrome)`, a Box brush over the SVG rasterised at its own size (`tools/raster_svg.py`
  group `chrome`, `tools/import_art.py chrome` -> `/Game/Pawpocalypse/Art/UI/Chrome`), `ImageSize` = the SVG's size,
  `Margin` = Godot's `texture_margin` / size, so the corners draw 1:1 as Godot's. Worn by: the theme's Button (BP_ButtonStyle's
  four states; a locked row = Godot's disabled plate in every state), the settings lists (OptionButton plates, PopupMenu panel
  and hover), the WindowPanel (job picker, victory), the theme ProgressBar (the wave bar; the Reawaken bar's track), the
  HSeparators (dock tabs, settings: invisible, as in Godot, finding F41). (Until R3's shapes, 2026-11-08: since then the
  plates below wear it too, see the bullet above.) NOT worn where Godot overrides the theme with a flat
  style: the dock (`hud.tscn` overrides `DockPanel`, so `ui_dock.svg` is drawn nowhere), the title's, pause and settings
  windows, the toast, the boss panel, the zone plate and bar, the XP bar. **Focus is a ring** (`PawUi::FocusRing`: SBF_focus,
  3 px `UI_FOCUS`, radius 12, 3 px outside), painted OVER the plate by `UPawButton::NativePaint` (and the settings dialog
  for its lists) when the control is hovered or focused by keys / a pad (focus the mouse gave is not ringed, as Godot):
  a state brush cannot carry it, and a locked row is never hovered (Common UI). The bars fill Godot's way since slice 5d:
  the fill brush drawn at the fill's width with both caps (`EProgressBarFillStyle::Scale`), at the value Godot's Range keeps
  (`PawUi::SetBarValue`: snapped to the bar's step, 0.01, the XP bar's 0.0001; the fill `round(r * (w - m)) + m` wide, m = 16
  for the theme's nine-patch, 0 for a flat fill). Test `Pawpocalypse.UI.Chrome`.
- **Icons (phase 6 slice 5b, 2026-11-06)**: Godot's `Art.texture(stem)` is `PawUi::Icon(stem or res:// path)` over
  `DA_Icons` (UPawIconSet, generated by `tools/import_art.py icons`; null when there is no art, as Godot). A button's icon is
  `UPawButton::SetIcon` (Godot's `Button.icon`: at the content's left edge, the text centred in what is left; an icon-only
  tab centres it); a label with a currency before it is `UPawIconText` (`wrap_label`: 24 px, 8 apart). Icons decided by
  rules are set in `OnLayoutReady` (both modes), never only in the build code. The HUD: the currencies, the wave line (Bait),
  the six tabs as icons (names as tooltips; the open tab holds the theme's pressed plate: `SetActiveLook`), the Reawaken
  page, the achievement and job rows, Bait (Catnip's icon when Catnip is short), realm rows asking for Catnip, Claim / Claim
  all / Reawaken (Stardust). **The quick bar** (hud.tscn QuickSeparator / QuickBar / QuickStatus): four bare icon buttons
  (`UPawBareButtonStyle`, a TextureButton's look), mouse only (never focusable: the pad has Start and Y), Settings, the
  tree (browse), Save ("Saved." under it), Save and back to the title. Elsewhere: the title's three plates, the card pictures,
  the tree's node icons, the Pounce clock's paw. Not built: the custom tooltip (the engine tooltip shows the names).
- **Motion (phase 6 slice 5c, 2026-11-06)**: Godot's UiJuice. `Pc::PcJuice` (PawCore) holds one control's scale and rotation
  and its two tweens (one per property, a new one kills the old; each starts from the value it finds; the punch's two legs
  pass the leftover on), stepped by its owner; `Pc::TweenInterpolate` is Godot's Tween easing. `FPawJuice` (one per screen)
  wires and ticks it and writes it as the widget's render transform about the centre: layout never moves. Hover is the
  pointer's (Common UI fires OnHovered only for the mouse and only on an enabled, interactable button: `_on_enter`'s
  disabled check); a button the port keeps interactable where Godot disables it (the tree's nodes) is switched off with
  `SetHoverOff`. The effects setting stops the idle lean only. Where: hover on the shop rows, job buttons, tabs, travel rows,
  claim buttons, tree nodes, the title's cluster; lean on the Reawaken tab (and its hue glow) while a Reawaken is suggested
  and its page is shut, on Bait while it calls (restarted every refresh as Godot does: F42), on a newly open biome's row,
  on the zone plate while it shouts, on READY nodes in the spend step; punch on a bought shop row or tree node; pop-in on
  the tree's leaving plate. Tests `Pawpocalypse.UI.Juice.*` and the L1 `Juice` cases.
- **The rainbows and the tree's sky (phase 6 slice 5d, 2026-11-07)**: Godot's three `_draw()` nodes, their shapes from
  PawCore (`PcRainbow`, `PcSky`: exact against the real scripts, fixture `rainbow`), painted in Slate as Godot's draw calls
  (`FPawFxCanvas` -> `PawUi::PaintCanvas`, one batch per piece). **The Reawaken page's bar** (`UPawRainbowBar`,
  `WBP_RainbowBar`, part `ReawakenRainbow` in place of the old `ReawakenBar`): the theme's track with no fill of its own, 48
  bands of a scrolling rainbow over the filled width, sparks from the leading edge (14 a second, 0.7 s, none from an empty
  bar, none while a full one holds over 24; drawn from `PcRng`), the value kept in hundredths. **The zone plate's frame**
  (`UPawRainbowFrame`, `WBP_RainbowFrame`, part `ZoneReady`): a sibling in the HUD's root overlay that paints round the
  plate's last painted geometry grown 2 px along the plate's axes (so it leans with the plate's jitter), corner 8, width 4,
  a 1 px feather for Godot's antialiased line; on while the plate shouts (Bait, or a biome to go to) and shown only with the
  plate; the hue turns 0.12 a second only then. The plate's own border stays the 2 px ink (the port's old stand-in, a border
  walking the hue, is gone). The Reawaken banner's frame waits for the banner. **The tree's sky** (`UPawTreeSky`,
  `WBP_TreeSky`, part `TreeSky`): over the opaque `Sky` border (which still takes the drag and the wheel), laid out for its
  size at the effects budget of that moment (9 / 6 / 3 clouds, 60 / 42 / 21 stars), stepped while the tree is up, its clock
  running on through a relayout. Tests `Pawpocalypse.UI.Rainbow.*` and the L1 `Rainbow` / `Skill tree sky` cases.
- **The Unicorn ceremony (phase 6 slice 6a, 2026-11-07)**: unicorn_ceremony.gd. `Pc::PcUnicornCeremony` (the dim, three hops
  of the mounted cat from off the right edge, the rear and the elastic settle, the rainbow growing from 1.25 s, 70 confetti
  bursting from its chest, 110 raining across the screen, 26 gold stars; the fade at 3.4 s; a press after 0.6 s skips; two
  whinnies and the fanfare) over `Pc::PcTween`, Godot's Tween sequencing (steps, parallel tweeners, delays, callbacks, the
  leftover time running into the next step); exact against the real script (fixture `unicorn`). `UPawUnicornCeremony`
  (`WBP_UnicornCeremony`, part `Unicorn` of `WBP_SkillTreeScreen`) paints it (the arcs and rims with a 1 px feather; the
  cat `player_cat_stage_4` from `DA_FieldAtlas`, mirrored, turned about the middle of its foot). Tests
  `Pawpocalypse.UI.Unicorn.*`, L1 `Unicorn` and `Tween`.
- A widget kit (panel, row, price label with currency icon, badge, bar, tab button) keeps screens short.
- **Fonts** (as built, restyle R2, 2026-11-07; doc 07 §5): one composite `F_Paw`: M PLUS Rounded 1c (ExtraBold text,
  Black titles and numbers through `PawUi::Heavy`) with Noto Sans CJK Medium (the `.ttc` is ONE face asset, its faces
  picked by sub-face index: no split) and Noto Sans Thai Medium as sub-fonts per culture and script `[KB U/09 §5]`.
  It is UMG's default font (`[SlateStyle] DefaultFontName`), so no widget names a font (L0 forbids engine fonts in UI
  code). Static faces only (Slate has no variable fonts). Licences: OFL, the texts in `SourceArt/fonts`.
- Text sizes: body ~24-26 px at 1080p, never under 18 px; Deck floor 12 px at 1280x800 `[KB U/09 §7]`.
- UI motion (`UiJuice`: press squash, hover lift, badge pop) as cheap widget transforms/tweens, not Sequencer animations
  (as built: the Motion bullet above).

## 8. Tests for this area (doc 09 has the full list)

Every screen's rows reachable by pad (an input tour with real Slate key events, not synthetic Enhanced Input injection
`[KB U/08 §6]`); UI actions do not consume field keys; HUD capture mode; every action bound on both devices or listed; every
glyph family has art for every pad button; remap rules (no system key taken, swap instead of unbinding, reset); dock side
flip; status row hit-test invisibility; no colour literals; tree seating/spacing/crossing/navigation; the padplay loop
(title, shop, a card, travel, the first boss, Reawaken, the tree, settings, save and quit with a pad alone); the Unicorn
ceremony holding every click and key over the tree while it plays and handing the focus back (`Pawpocalypse.UI.Unicorn.*`,
phase 6 slice 6a).
