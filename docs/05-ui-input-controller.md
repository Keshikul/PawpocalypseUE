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
- **One press path**: LMB-on-field, RT and any future key go through one `TryPounce(Source)` `[KB U/03 §7]`. **Interim
  (2026-10-31, phase 2 slice 7)**: `APawRealmView::SampleInput` reads LMB and RT (> 0.5, Godot's dead zone) raw into the
  field's `bPounceDown` (the field finds the press edge itself); phase 5 replaces it with the Enhanced Input action.
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

**Built 2026-11-01 (phase 3, first slice; HANDOFF-LOG (21))**: `UPawRootLayout` (Game / Menu / Modal stacks, created by
`APawPlayerController::BeginPlay`, the HUD pushed on Game in a map with a realm view); `UPawHudScreen` with the status row
(Catnip, Stardust, the wave line, the zone plate with its rainbow frame), the Catches XP bar, the Pounce clock
(`UPawPounceClock`, painted), the boss panel (status, HP bar recoloured while shielded, the fight clock, the DEFEATED / OUT OF
TIME after-word until travel or the next summon), and the dock (one `UPawDock` was not needed: the dock is part of the HUD
screen): six tabs with badges and Jobs hidden until a job is held, the Upgrades page (15 rows, Buy x1/x5/x10/Max, hold-to-buy,
the Kitten lines locked until a kitten), side flip (`SetDockOnLeft`), the dock's rect reported to the realm
(`UPawRealmSubsystem::SetDock`), which measures the field beside it and refuses a Pounce press over it. What each readout says
is PawCore's `PcHud` (the decisions of hud.gd, L1-tested); the widget turns it into `ST_UI` / `ST_Data` text. Placeholders:
the tab icons, currency icons, badge star and the paw on the Pounce clock are drawn shapes until phase 6's art (the tab NAMES
show meanwhile, at 13 px); every page is built since slice 3; the dock's keys (tabs, buy
amount, 1-6) are read in `NativeOnPreviewKeyDown` until phase 5's Enhanced Input; the quick bar (settings, tree, save, quit)
waits for the screens it opens; the fps readout, tooltips and the tutorial overlay are not built.

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
`AchievementsPage`, `JobRows` (L1 + sabotages). Placeholders: Godot's row marks ★ ◆ ☆ are drawn discs (the font has no
star glyph) and the job icons wait for phase 6; the hover tooltips (every card row, the quick bar) are not built; the tab's
count sits past a text tab's edge until the tab icons arrive. **Achievements are earned by the realm** (found by the
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
- **The cards**: `UPawCardFace` (a Common button that is never focusable: the whole-card click) around a cream plate with a 3 px
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
**The screen, 2026-11-02 (HANDOFF-LOG (25))**: `UPawSkillTreeScreen` on the Menu stack (pause holder "tree"), as designed above: one canvas panel under a render transform (pan, zoom about the view's centre, fit on open), `UPawTreeLinks` painting links (owned only with both ends) and the rainbow / pulse / job rings under the nodes, `UPawTreeNode` (a Common button; the frame of its state in the trunk's colour, READY lerped a third of the way, unlocks heavier and rounder; the counter on ladders; the shine), gold halos breathing behind READY nodes, captions (BRANCH_LABELS), the hub plate, the Stardust line, +/- buttons, the legend, the tooltip (beside the mouse's node, docked bottom left for keys / pad). Browse: T / pad Y (the HUD), read-only, X. Spend: the HUD's Reawaken opens it (Continue; the cheapest node selected; a press buys; the two-press leave with the centre plate), then travels to the starting biome. Every word and colour of skill_tree_ui.gd is GENERATED (`tools/gen_tree_tables.py` -> ST_Tree.csv, PcTreeWording.h, PawTreeTables.h; L0 checks them current). **Placeholders**: key names for the glyphs (`PROMPT_*`, phase 5), a flat sky (skill_tree_sky.gd's stars, phase 6), no node icons (phase 6), the halo without its soft shadow, no hover lighten, no READY jitter (UiJuice), no Unicorn ceremony. (The ceremony before the spend step and the job picker after it: §6.3, 2026-11-02.)
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
