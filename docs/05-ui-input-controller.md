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
  template already put them in the right file.
- `CommonInputSettings`: `InputData` = `BP_PawUIInputData` (Click and Back actions), `bEnableEnhancedInputSupport=True`.
  Platform settings for Windows with TWO controller data assets (keyboard, gamepad); the default gamepad name must match or
  glyphs come out empty.
- `bEnableUserSettings=True` for Enhanced Input remapping; actions marked Player Mappable.
- `bUseSplitscreen=False`.

## 2. Screen structure

```
UPawRootLayout (owned by APawPlayerController, created on BeginPlay)
  Layer.Game    : UPawHudScreen (activatable, Game input config, CapturePermanently? see §4) - ALWAYS at the bottom
                    field overlay: Pounce clock, XP bar, boss banner/bar, status row (currencies, zone bar), tutorial overlay
                    UPawDock: tab strip (6 icon tabs with badges) + page switcher
  Layer.Menu    : UPawSkillTreeScreen, UPawCardPickScreen, UPawReawakenCeremony, UPawUnicornCeremony, UPawPauseMenu, UPawVictoryScreen
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

**The dock is not a Common UI stack of screens**: it is part of the HUD screen (the game keeps running while you shop). Its
pages are plain `UUserWidget`s in a switcher; the HUD screen's focus target is the dock's remembered row.

## 3. Input actions (Godot -> Enhanced Input, 1:1)

Bind keys by POSITION: Enhanced Input maps `FKey`s, which are physical scan positions for letters on Windows; verify AZERTY
puts WASD on ZQSD in a test (the Godot gate asserts it).

| Godot action | Keyboard/mouse | Pad | Context |
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

- **Over open field the OS pointer hides and the cat IS the cursor. Over the dock, a popup or a paused game the pointer is
  the paw** (`ui_cursor_paw_open/closed`, closed with claws while the button is held). `UPawInputDeviceSubsystem` asks the
  field owner `PointerOverField()` (inside the field rect and no widget under the mouse, via Slate's widget path under the
  cursor).
- **Paw cursor: hardware cursors**, not a widget (software cursors lag; Epic Won't Fix) `[Forum]`: two PNGs generated by
  `sweet_ui.py`, placed under `Content/Slate/Cursors/` and set per `EMouseCursor` type through
  `UGameViewportClient::SetHardwareCursor` at startup; the folder is staged. **Verify in a packaged build** (reports of
  cursors that only work in PIE). Fallback: a software cursor widget with interpolation accepted.
- **Mouse capture**: the HUD is a free-cursor mode: **`CaptureDuringMouseDown`, `DoNotLock`, cursor shown** (with
  `NoCapture` single clicks are lost and only double-clicks arrive; with `..._IncludingInitialMouseDown` pad A becomes a mouse
  click) `[KB U/08 §3.1-3.2]`. Pounce on LMB arrives through the field input path, not a widget.
- **One press path**: LMB-on-field, RT and any future key go through one `TryPounce(Source)` `[KB U/03 §7]`.
- **Pad virtual cursor**: the right stick moves a virtual cursor (the cat) that STAYS where it was left; R3 re-centres; while
  the stick steers, the OS pointer is hidden and warped onto the cat each frame (`FSlateApplication::SetCursorPos` /
  `APlayerController::SetMouseLocation`), and the device subsystem **ignores the mouse-move echo of each warp** for
  `WARP_ECHO_MSEC` (without that the stick loses the cursor on its first frame: a Godot bug). **Anything that follows "the
  cursor" asks `UPawInputDeviceSubsystem::PointerPosition()`**, never the mouse.
- Common UI's own analog cursor is NOT used for the field (the cat is the cursor); it may stay on for menus. It consumes D-pad
  keys before `NativeOnKeyDown` `[KB U/08 §3.3]`: dock navigation uses explicit navigation rules, not key handlers.

## 5. Device detection and glyphs

- Two separate questions, as in Godot: the **prompt family** (keyboard, Xbox, PlayStation, Steam Deck; an unknown pad draws
  as Xbox; **no Nintendo**, designer 2026-10-01) and the **cursor source** (mouse or stick). Both switch on a device's first
  INPUT, never on connect (a connect is a toast). Mouse MOTION changes the cursor source, never the family (the Deck trackpad
  rule). Base it on `UCommonInputSubsystem::OnInputMethodChanged` `[KB U/08 §5]`.
- **Windows pads: Unreal reads XInput only** (GameInput plugin off by default): every pad looks like Xbox, and a PlayStation
  pad plugged in directly does not work at all; through Steam Input it works as Xbox and the real type comes from the Steam
  API `[KB U/08 §5.1]`. Settings > Controls > Button prompts (Auto / Xbox / PlayStation / Steam Deck) is the override until
  Steam integration lands (doc 10 §5). Decide in phase 5 whether to enable the GameInput plugin for direct DualSense support.
- **Glyphs are generated** (`tools/sweet_input.py` via `sweet_ui.py --write`) per family, named by POSITION (`face_down` is
  A on Xbox, Cross on PlayStation): import them as one texture set per family into `CommonInputBaseControllerData` assets.
  **Keyboard keys are keycaps drawn at runtime around the key's live display name** (remaps and AZERTY read right); only
  arrows and mouse buttons are images (the font has no arrow glyphs).
- **No hardcoded prompt text**. Strings carry action tokens (`"a burst on {pounce}"`); a `UPawPromptText` (rich text
  decorator that resolves `{action}` to the current glyph) and `UPawPromptIcon` redraw on device, language or binding change
  and on **`ControlMappingsRebuiltDelegate`** (a legend built before contexts apply shows no glyphs) `[KB U/08 §4]`.

## 6. The screens

### 6.1 HUD and dock (`UPawHudScreen`, `UPawDock`)
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
- 87 nodes on a hex lattice, links drawn between them, pan and zoom, three node states, rings/halos/pulses.
- Build: one C++ widget; **links and decorations drawn in `NativePaint`** (`FSlateDrawElement::MakeLines` with
  anti-aliasing, or `MakeSpline`), **nodes as `UCommonButtonBase` children** positioned in ONE canvas panel inside a
  pan/zoom container (render transform). One canvas is acceptable; nested canvases are not `[KB U/09 §1]`.
- Lattice positions computed by the ported layout code (`skill_tree_ui.gd`'s layout section); a test asserts every data node
  is seated, nothing is seated without data, spacing is even, no link crosses a node or another link (all four were Godot
  `check_hud` assertions).
- **Pad navigation from the lattice**: each node's four neighbours written once per layout (`nav_pick` port) as explicit
  navigation; a test walks every node from where the tree opens. Q/X next affordable; right stick pans; RT/LT zoom.
- Tooltip docked (pad) or following (mouse). Unicorn purchase plays `UPawUnicornCeremony` over the tree. **Boba never over the
  tree** (`UPawBobaSubsystem::Suppress()`, which also cuts a cameo in flight).
- Asks before every return to the game after a Reawaken.

### 6.3 Others
Card pick (3 cards, keep one; auto-pick node), job picker, pause menu (Save, Settings, Quit to title; quit saves first),
settings popup (Audio / Video / Controls with remapping rows: a row is a LOGICAL action writing several Enhanced Input
actions, Esc/Start/B are system keys and can never be taken, Reset button), victory screen, main menu with five slots,
ceremonies, the tutorial overlay (`HitTestInvisible`, never focusable), cheat overlay (F1, dev only).

## 7. Look: tokens, brushes, fonts

- **`PawTokens.h`** generated from `tools/palette.py` (colours, the UI panel/button/text/focus colours, radii, frame width,
  font sizes). Screens never hold a hex value; a test greps for `FLinearColor(` / `FColor(` literals outside the generated header.
- **Panels and buttons are `FSlateRoundedBoxBrush`** with an outline: cream `#FFF9E3`, 3 px `#1A1A1A`, radius 6-8. All
  four button states share one shape (only fill changes); focus is a ring (outline colour `UI_FOCUS`), not a plate `[KB U/09 §4]`.
  Brushes stay crisp at 4K without nine-slice art; the Godot nine-patch PNGs from `sweet_ui.py` are not needed (keep them
  only where a texture is genuinely decorative).
- A widget kit (panel, row, price label with currency icon, badge, bar, tab button) keeps screens short.
- **Fonts**: the Godot game's Latin font + Noto Sans CJK Medium (`.ttc` must be split into per-language `.otf`/`.ttf` faces:
  Slate composite fonts take font faces, and variable fonts do not work in Slate) + Noto Sans Thai, in one composite font
  with sub-font ranges per script `[KB U/09 §5]`. Licences: OFL (already in the Godot repo).
- Text sizes: body ~24-26 px at 1080p, never under 18 px; Deck floor 12 px at 1280x800 `[KB U/09 §7]`.
- UI motion (`UiJuice`: press squash, hover lift, badge pop) as cheap widget transforms/tweens, not Sequencer animations.

## 8. Tests for this area (doc 09 has the full list)

Every screen's rows reachable by pad (an input tour with real Slate key events, not synthetic Enhanced Input injection
`[KB U/08 §6]`); UI actions do not consume field keys; HUD capture mode; every action bound on both devices or listed; every
glyph family has art for every pad button; remap rules (no system key taken, swap instead of unbinding, reset); dock side
flip; status row hit-test invisibility; no colour literals; tree seating/spacing/crossing/navigation; the padplay loop
(title, shop, a card, travel, the first boss, Reawaken, the tree, settings, save and quit with a pad alone).
