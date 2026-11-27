# Worklist: the designer's Steam playtest of 2026-11-26 (build 0.1.2)

> **Status: PLAN ONLY. Nothing is built yet.** Written 2026-11-26 (HANDOFF-LOG (156)) from the designer's 44 notes
> (the designer: "I did a steam test. Notes in text. Before doing anything read through and make a very detailed plan on
> how to fix stuff"). Phase 1 can start on the designer's go; the rest waits for the answers in §2.
>
> Notes are numbered by their line in the designer's file (1-44). Every claim below was read in the code or data, or
> measured with pawsim / `econ_review` in memory (nothing in `Data/` written). Anything only reasoned from the code, not
> seen running, is marked *(unverified)*.

**Progress (HANDOFF-LOG (157), (158)):**
- **Phase 1: built 2026-11-26**, P1.1-P1.14.
  - Its tests and 21 sabotages were seen failing (one, `vfx_pulse_fades_at_once`, needed a stronger fault).
  - `check_all` passes every gate; the screen tour was checked.
  - Not done: the Steamworks icon upload, P1.15's Kitten Speed clamp (moved to E3).
- **§5 words: approved 2026-11-26** ("Text approved. Dont translate anything though. We still might change stuff. We
  will retranslate closer to release"; then "Offer them, gaps in English": D28). Each line goes in with its feature,
  English only. In so far: the Vitality rename (four lines).
- **P4.2, the Compass tree: built 2026-11-26** (D27; HANDOFF-LOG (158)). The tooltip's redesign waits for P4.1's shared
  tooltip (its words, `TIP_*`, are approved). The sky stays: the designer asked for it on 2026-09-14.
- **C1:** https://claude.ai/artifact/5HCPtMKZ3GruMxwa5JR74E.
  - v1: styles.
  - v2: the designer said "Darker cards for the skill menu. But i also want you to work on the layout not just style",
    so v2 has layouts A (Boards) / B (Rings) / today's, and dark or light cards.
- **E0 started:** `tools/econ_review.py` levers `roster`, `nobait`, `fight<s>`, `up.*`, `tier.*`, `code.*`, plus the
  milestones line. First sweeps are in `build/econ_e*.txt`.

**Rules that apply to every item** (CLAUDE.md):
- New or changed player words are drafted first (§5); each changed line then needs its 22 translations.
- Every change is asserted by a test, and the test is seen failing once (a sabotage entry).
- Widget layout changes go into the `WBP_*` asset by `--graft`, never a re-bake.
- A design or balance change gets a doc 01 §2 entry.
- `check_all` must exit 0 before anything is called done.
- Package and Steam upload only on the designer's word.

---

## 1. What the investigation found

1. **"This looks like an earlier version of the Godot project": not quite.**
   - The port started from Godot's last commit (`350b2f5`, 2026-10-03, itch 0.2.5). Godot has no commits after it.
   - All 34 notes of the 2026-10-01 playtest are in this game. So are the late Godot features (pad support, atomic saves,
     the 30 s autosave, Report a bug, the Discord button, the biome 9-10 gates).
   - **What is missing:** three features never built here: **the tutorial** (slice 6d), **Boba's photo and cameo**
     (slice 6c) and **the upgrade hover tooltip**.
   - **Remembered from Godot, but Godot had them the same way as here:**
     - the card level caps;
     - the Catches page listing every card (Godot since 2026-09-11, at the designer's request then: "a tooltip like for
       upgrades that shows what you need to do to unlock");
     - auto-pick behind the Auto-Card tree node, switched on the Catches page only;
     - no tooltips on achievements;
     - the same Bait strings ("30 more bait").
   - All of these are changed anyway, as the designer's new requests. None is "matching Godot" (D22).
2. **Why the game feels like a speedrun.** The cat kills almost everything that spawns: kills are limited by spawns, not
   by damage, in **98 % of play** (`econ_review`). So income = spawn rate × Catnip per spawn, and damage upgrades only
   make enemies melt. Fewer enemies and a mostly-common mix (notes 9, 18) slow income directly.
3. **pawsim does not model card effects** (it counts card picks only, `tools/pawsim.py` v4). Hot Borders alone turns most
   escaping enemies into kills (§7 E5), so **the real game is faster than pawsim says.** The balance pass needs pawsim to
   learn the cards that raise income first (§7 E0).
4. **Bait was the only lever that lengthened the game** (D24: 113 → 175 min). Note 42 removes Bait, so a new gate is
   needed: the boss's HP against the cat's damage.
5. **"2 kittens appeared" is D17 E4** (2026-11-12): Kitten Count gives +2 kittens a buy (`Data/UpgradeData.csv:7`,
   `effect_per_level` 2.0). The shop row never says so. This repeats the 2026-10-01 note #22 ("kittens too cheap").
6. **The "free golden kitten" was a Vitality kitten turned gold** (inferred from the code). Vitality is the tree skill
   that gives free kittens (`SkillBranchData.csv:87`, `KITTEN_COUNT` +1 to +10). Gilded Kitten is offered once you have
   any kitten (`PcCards.cpp:141`) and paints the first kitten gold. It never creates one (`PcField.cpp:1131-1165`). The
   rename (note 38) is the fix; the rule stays.
7. **New defaults don't reach the designer's PC.** Every save writes the whole settings file, so the designer's PC
   already holds `music_volume=0.45` and a windowed 1920×1080. New defaults reach a fresh install only. So music gets a
   trim that reaches everyone (P1.12). To see the first-launch resolution, delete the `settings` entry in
   `%LOCALAPPDATA%\Pawpocalypse\Saved\SaveGames`.

---

## 2. Decisions for the designer (each with a recommendation)

> **Answered 2026-11-26 (designer):** "Slow down game even more. We should be able to afford a kitten at 15 min. We
> should be the strong one untill we get a few kittens. Continue with the plan".
> - **D-a:** the first kitten affordable at **15 min** (not ~6). The other milestones slow further in proportion; the
>   economy pass proposes them with numbers.
> - **New rule:** **the cat outdamages the kittens until the player owns a few.** The pass measures the cat's share of
>   damage against the kitten count, and the kitten lines are set so the kittens' total passes the cat only after ~3-4.
> - **D-b to D-n:** the recommendations stand ("Continue with the plan").
> - **Still shown before building:** the words (§5), the concepts (§6) and the economy proposal's numbers (§8).

| # | Notes | Question | Recommendation |
|---|---|---|---|
| D-a | 16, 33, 34, 41 | **Pace targets.** Today (pawsim, a player who buys at once and Reawakens when allowed): first kitten 3.4 min, biome 2 4.6, first Reawaken 9.8, biome 3 13.4, first boss 50.5, game won 175 min (14 Reawakens). The real game with cards is faster. | First kitten ~6 min, biome 2 ~12, first Reawaken ~25, first boss ~90, game ~6 h. D17's "always something to buy" goals stay: waits over 3 min ≤ 10 % of play, nothing new ≤ 15 min. |
| D-b | 9, 18 | **The enemy roster in each biome** (§7 E1 has the numbers). | **The ladder.** Each biome has 1 common (most enemies), the next biome's common as its rare (next biome it becomes the common), and its signature as the rarer. Fliers and gristles join only as your spawn rate rises. The "last biome's common" is dropped; the golden stays as now. The rares' share rises with the player's own spawn-rate boosts, never with the cat's damage. Backyard 120 → 60 enemies/min. |
| D-c | 33, 42 | **Bosses without Bait.** | Summon free from the boss's biome. The time limit becomes the strength check (~90-120 s). A boss you can't beat in time leaves; summon it again any time, at full HP. A beaten boss is beaten for good (the button says so). Bait removed entirely, including the tree nodes **Audit, Deep Pockets, Auto-Bait** (their Stardust refunded in old saves). **Add a strength meter** on the button (your damage against what the fight needs), so the player knows when to try. |
| D-d | 19 | **Cards past their level table** (today 2-4 levels a card). | Each take past the table adds the table's last step again (e.g. Hot Borders +0.08 a take). Downsides keep stacking. Values that fall (aura size, cooldowns) stop at a floor. Chances stop at 100 %; Dilated stops at 5 cards a hand. Warning: uncapped cards make each run stronger, against notes 34/41; the economy pass measures it. |
| D-e | 36 | **Auto-pick.** Today it needs the Auto-Card tree node (one Reawaken, 260 Stardust) and is switched on the Catches page. | **Free for everyone**, off in a new game. Turn it on from the card screen; turn it off in Settings (a Gameplay box) or on the card screen. The Auto-Card node is removed and its Stardust refunded. Alternative: keep the node and show the box only once it is owned. |
| D-f | 8 | **Waves.** The bar goes (asked). But waves are a real rule: every `kills_per_wave` kills (300 in the Backyard) enemies get ×1.35 HP and pay ×1.28; Fence, Reputation and Compound Interest count waves. | Keep waves as a hidden rule for now and reword those three skills so they don't say "wave". Revisit in the economy pass. Alternative: remove waves from the rules (an economy change). |
| D-g | 12, 13, 16 | **Kittens.** | +1 kitten a buy (each buy shows one kitten) and a higher price (numbers from the economy pass); base speed 260 → 180 px/s (−30 %); the shop row says "+1 kitten". |
| D-h | 17, 20 | **The signatures' thick white edge** (moth, dogs, goose). Cause (measured from the atlas): the white glow drawn behind every signature (asked for in Godot, 2026-10-01 note #4) reads as a huge rim; signatures are also drawn at 95 px against 64 px for commons; and the moth's concept is drawn small, so its lines scale up more. | A thinner, fainter glow (1.24× → 1.10×, half the alpha) and signatures drawn at ~80 px. Chosen from a review sheet (C3) showing today and 2-3 options side by side. |
| D-i | 37 | **"Twice as silent."** | −10 dB on all music (sounds half as loud), as a data trim that reaches every install. Alternative −6 dB (half the signal). |
| D-j | 24 | **Dock width.** Today 568 px. | 480 px (−15 %), every page re-wrapped and checked in all 23 languages. Alternative 520 px. |
| D-k | 30, 40 | **Hot Borders and Slow Burn.** Today both are the same rule (`EDGE_BURN`: enemies within 90 px of the edge burn, wherever the cat is); they add up. | Hot Borders burns only enemies walking out (what its text promises), with lower numbers after measuring. Slow Burn becomes its own card: **enemies your aura touched keep burning for 3 s after they leave it** ({v}× aura damage); it still costs spawns. |
| D-l | 38 | **Vitality's new name** (it gives free kittens; the icon is a heart). | "Extra Kittens", with a kitten icon. Avoid "Litter" (the Litterbox, the Litter King). The achievement "Boundless Vitality" is renamed with it (Steam re-upload). |
| D-m | 2 | **Which icon for the exe and taskbar.** | The 2026-11-25 key-art icon (D26, in Drive), so the exe, the taskbar and Steam match. Alternative: the first try's cat-face badge (`SourceArt/svg/ui/app_icon.svg`). |
| D-n | 42 | **Boss size.** Today 1.4× (D25: 358 px; the Park Tyrant has 2.4× the cat's area). | 1.8× (460 px), shown beside 1.4 and 2.2 in C5. Hits count anywhere on the body (D25's open question). |

---

## 3. Phase 1: bugs and removals (no decisions, no new words; can start on a go)

Sizes: S = under half a day, M = about a day, L = several days.

### P1.1 Esc leaves the skill tree; the unspent-Stardust warning (notes 11, 27) [M]
- **Cause** *(traced in Slate's source, not reproduced)*:
  - The tree screen never takes focus, and its zoom and X/Continue buttons are not focusable
    (`PawSkillTreeScreen.cpp:585, 619`).
  - A click on the sky or on Continue hands focus to the game viewport. The tree reads Esc/B in
    `NativeOnPreviewKeyDown` (`:1401-1420`), which only fires with focus inside the tree. The HUD ignores keys while a
    menu is up (`PawHudScreen.cpp:1761`).
  - The warning plate ("You have unspent Stardust. Press again to leave.", `TREE_warn_unspent`) only goes on a second
    Continue or a buy (`PressClose`, `:1186-1206`). After a click, Esc never arrives, so the plate stays. The KB has the
    same pattern (`Unreal/24` line 70).
- **Fix:**
  - `SetIsFocusable(true)` in `UPawSkillTreeScreen::Initialize`, as `PawUnicornCeremony.cpp:44` does.
  - A mouse click returns focus to the last node.
  - Esc and B then behave like Continue: browsing closes at once; in the spend step the first press warns, the second
    leaves.
  - The plate becomes a real Leave/Stay confirm in P4.7.
  - Check the pause menu and the settings dialog for the same focus loss (their scrims take clicks).
- **Tests:** a Slate-level test (click the sky, send Esc, the tree is closed; the same in the spend step, two presses),
  and a sabotage that drops the focusable line.

### P1.2 Slot 1 always large (note 6) [S]
- **Cause:**
  - `StepSwell` grows a card when it is hovered OR has focus (`PawMainMenu.cpp:1347-1375`).
  - Opening the slots focuses the active slot (`:894-898`, `:952-957`), and the mouse never moves focus.
  - The selection ring already uses the right rule: focus counts only when the last input was not the pointer
    (`UPawButton::RingRule`, `PawButton.cpp:239-244`).
- **Fix:** `StepSwell` uses `RingRule`. The same bug is on the card pick screen (`UPawCardFace::IsLifted`,
  `PawCardPickScreen.cpp:355-358`): fixed the same way.
- **Tests:** sizes with pointer input vs pad input, on both screens, plus a sabotage.

### P1.3 The Upgrades tab's red dot (note 26) [S]
- **Cause:** `Pc::AnyUpgradeAffordable` (`PcHud.cpp:244-262`) checks every upgrade, including rows the shop hides or
  locks: Kitten Speed (45 Catnip) with no kitten; Aura Radius, the Pounce lines and Overkill before their unlocks. Any
  Catnip at or above 45 keeps the dot lit.
- **Fix:** light it only when a row the shop shows can be bought (`ShopRow(...).bEnabled` over `ShopRowOrder()`).
- **Tests:** `PcHudTests` (`:399-404`) plus a hidden cheap row that must not light it, and a sabotage.

### P1.4 Settings: the hover frame cut off on the left (note 4) [S]
- **Cause:** the check rows (Fullscreen, V-Sync, Effects, Mute, the Controls rows) sit at x=0 inside the pages'
  ScrollBox, which clips; the focus ring draws 4 px outside the row (`PawUiKit.h:87-88`, `PawButton.cpp:221-228`).
- **Fix:** 6 px padding left and right on the ScrollBox's slot (`--graft "PawSettingsDialog=slot:Pages"`); the notes'
  wrap widths shrink to match. Check every other scrolling page the same way.
- **Tests:** a layout assertion that the ring rect sits inside the clip rect, plus a sabotage.

### P1.5 Three texts removed (notes 4, 10, 22) [S]
- **Settings:** "The interface keeps its size at every resolution…" (`SETTINGS_video_note`, `VideoNote`,
  `PawSettingsDialog.cpp:314`).
- **The Reawaken tab:** "Resets Catnip…" (`REAWAKEN_blurb`, an unnamed text at `PawHudScreen.cpp:2243`; its auto-name
  from `--graft "PawHudScreen=names:"`).
- **The card:** "This card has a downside." (`PICK_costs`, `WarnText`, `PawCardPickScreen.cpp:196-199, 306`). The red
  frame on cards that cost something stays (only the text was asked).
- **Each:**
  - `--graft remove:` from the asset and the code layout;
  - retire the string, its `Data/TextOverrides.csv` row and its 22 translations;
  - update the tests: `PawSettingsTests.cpp:465`; `PawCardPickTests.cpp:318-320`; sabotage `pick_warning_always`.

### P1.6 The wave bar removed (note 8) [S]
- **What goes:** `WavePlate > WaveBox > WaveLine, WaveBar` (`PawHudScreen.cpp:433-450, 468`, refresh `:2064-2082`) and
  the rule `Pc::WaveLine` (`PcHud.cpp:189-211`) if nothing else uses it.
- **Rules:** unchanged until D-f.
- **Bait:** in a boss biome the bar showed Bait held/needed. That stays visible on the Realms tab until Bait goes (E2).
- **Tests:** `PawHudTests.cpp:257, 302-306, 458-461, 472`, `PawIconTests.cpp:200-201`, `PawRainbowTests.cpp:212`,
  `PcHudTests.cpp:319-334`, sabotage `WaveShown`.

### P1.7 The pause menu: centred words, no icons (note 25) [S]
- **Cause:** `Dress()` adds an icon to each of the four buttons (`PawPauseMenu.cpp:176-179`). With an icon,
  `ApplyLineLayout` centres the words in the space after it (`PawButton.cpp:277-289`).
- **Fix:** remove the four calls.
- **Tests:** `PawPauseTests.cpp:444-447` asserts no icon.

### P1.8 The cat stays in the play area, and stays put on alt-tab (note 35) [S]
- **Cause:**
  - The pointer path is unclamped on purpose (`PawRealmView.cpp:233`: "wherever the pointer is, under the dock too (only
    the stick is clamped)").
  - When the mouse leaves the window, the cat keeps its last spot, usually under the dock, where no enemy goes: it looks
    gone and hits nothing.
  - The simulation keeps running when the window loses focus (30 fps cap, fixed step), so damage already continues
    *(unverified in play)*.
- **Fix:**
  - Clamp the pointer to the field's bounds in the view, like `StickStep` (`PawRealmView.h:96-100`).
  - Re-clamp when the bounds change (`PawRealmSim.cpp:37-44`).
- **Result:** the cat stops at the dock's edge while the mouse works the menu. On alt-tab it stays where the pointer left
  the field and keeps hitting.
- **Tests:** near `PawInputTests.cpp:1136`, plus a sabotage. Docs 02 and 05 updated.

### P1.9 Report a bug opens Steam's discussions (note 5) [S]
- **Change:** `PawMainMenu.cpp:51` (the itch board) → `https://steamcommunity.com/app/5720010/discussions/`.
- **Where it opens:** in the Steam build, the Steam overlay (`PawSteam::OpenOverlayWebPage`, already there); in the
  plain build, the browser.
- **Check:** the page loads while the store page is "coming soon" *(unverified; if not, it works once the page is
  public)*.
- **Docs:** 02 §13, 10 §5.1.

### P1.10 First launch at the main monitor's resolution (note 1) [S]
- **Cause:**
  - The default is a 1920×1080 window (`PcSettings.h:39, 45`).
  - The engine opens borderless at desktop size, then `ApplyVideo` shrinks it one frame later
    (`PawSettingsSubsystem.cpp:211-280`).
  - On a 1440p or 4K monitor that is a small window.
- **Fix:** on a first run only (no settings entry, `Reload`, `:93-111`), start borderless fullscreen at the primary
  monitor's size. No fixture changes (the 1,000-row settings fixture keeps its windowed default).
- **Tests:** a fresh-settings test.

### P1.11 The app icon (note 2) [S]
- **Cause:** no `Build/Windows/Application.ico` and no `WindowsPlatform.ApplicationIcon`, so the exe gets the engine's
  `Default.ico`.
- **Gotcha:** `Build/` is the same folder as the gitignored `build/` on Windows, so an icon there would never be
  committed.
- **Fix:**
  - A multi-size `.ico` (16-256 px) from the icon chosen in D-m, committed under `SourceArt/`.
  - Set `WindowsPlatform.ApplicationIcon` in `Source/Pawpocalypse.Target.cs` (the Steam target inherits it).
  - A `tools/package.py` audit rule: the exe's icon is not the engine default.
- **Separately, in Steamworks:** the client icon (.ico) and the library/community icons. Done in the browser with the
  designer's OK.
- **KB:** the `Build/` vs `build/` trap.

### P1.12 Music half as loud (note 37) [S]
- **Change:** a `music_trim_db` row in `Data/CodeConstants.csv` (−10 dB, or D-i's answer), applied in `MusicGain`
  (`PawAudioSubsystem.cpp:167-177`).
- **Why a trim:** it reaches the designer's existing settings, and the slider keeps its meaning.
- **Tests:** `PawAudioTests.cpp:223`, plus a sabotage.

### P1.13 Ember Trail red, the aura without its ring, Thunderclap visible (notes 39, 31, 32) [M]
- **Ember Trail:**
  - Its zones are drawn in `ZoneTrail` `#8CD1FF` (light blue), hard-coded in `PawVfx.cpp:27`: a first-try leftover the
    Sticker Box pass never reached.
  - Make it ember red (the palette's `#FF7055` / `#D86061`).
  - Move all zone colours into `tools/art/fx_look.py` → `PawFxLook.h`, with a look test and a sabotage.
- **The aura:**
  - Today a 6 px gold ring at 85 % over a 16 % fill (`fx_look.py:17-23`, `PawVfx.cpp:924-949`).
  - Remove the ring; the fill fades to clear over its outer quarter (a new feathered disc in `FPawFxCanvas`).
  - The Unicorn's rainbow ring gets the same feather.
  - Shown on the D3 review sheet (`tools/art/review_d3.py`) before and after.
  - **Tests:** `PawVfxTests.cpp:262-288`, sabotages `vfx_aura_ring_gold`, `vfx_aura_no_lift`.
- **Thunderclap:**
  - Today one 4 px pale ring over 0.4 s, fading as it grows, so it is gone before it clears the cat's sprite
    (`PawVfx.cpp:410-411, 684-697`).
  - New: a bold shock ring that keeps its strength out to the aura's edge, a short flash, and every enemy it hits
    flashes.
  - Test plus a sabotage.

### P1.14 The title's Exit asks first (note 43, part) [S]
- **Change:** the Exit button goes through the existing `AskConfirm(TITLE_quit_prompt, PressExit)`
  (`PawMainMenu.cpp:633, 1047-1060, 1113`). Esc on the title already asks this. No new words.
- **Tests:** `Pawpocalypse.UI.Title.BackAndQuit`, plus the button-asks case.

### P1.15 Found on the way (not in the notes; from reading the code, each confirmed by a failing test first)
- **Kitten Speed overshoots its cap.**
  - `KittenSpeedMultiplier` uses the uncapped total (`PcStats.cpp:269`); every other capped line clamps its effect
    (`CappedEffect`).
  - The cap only refuses a buy once the total is already at or over it, so a third level reaches ×3.996 against 3.0.
  - Legacy's "levels" channel starts every line at up to level 26 with no clamp ("the cap is on the EFFECT"), and that
    makes it unbounded: ×1.5869^26.
  - **Moved to E3:** the clamp changes the recorded `stats_godot.tsv` column `kitten_speed_multiplier`, and kittens are
    re-tuned there anyway.
- ~~"Fleet Paws" can't be earned~~ **(wrong, checked 2026-11-26):** Legacy's starting levels (up to 26, no clamp) reach
  every "upgrade level N" achievement. Without Legacy, the shop alone stops at levels 4-11 on six of those lines (Aura
  Radius, Kitten Speed, Overkill, Pounce Power, Shield Pierce, Spawn Rate), so those achievements need late Legacy.
  Noted for the economy pass, not a bug.
- **Auto-Bait can re-summon a beaten boss** (`CanSummonFieldBoss`, `PcField.cpp:2420-2424`, doesn't ask "beaten").
  Moot once Bait goes (E2), but "a beaten boss can't be summoned" is asserted there.
- **The Hot Borders band's drawing hard-codes its own 90 px** (`PawVfx.cpp:34`). It should read the data value.
- **Doc 05 §6.2 names `UPawBobaSubsystem::Suppress()`,** which doesn't exist yet (P4.3 builds it).

---

## 4. Answers that need no work

- **Note 44, autosave: yes.** The game saves:
  - every 30 s of play while a biome is up;
  - on every purchase, tree level, boss, job, award, card or Reawaken (at the end of that frame);
  - on alt-tab or focus loss;
  - on quit.
  
  A crash loses at most ~30 s of kills and Catnip. Writes are crash-safe (temp file, read back, backup, then replace;
  an unreadable slot is rebuilt from the temp or the backup). Covered by `Pawpocalypse.Save.Atomic / CrashRecovery /
  Autosave`.
- **Note 29, the free golden kitten:** see §1.6. The rename (D-l) is the fix.
- **Note 12, two kittens:** see §1.5 and D-g.
- **"Only 1 boss visible at once" (note 42):** already true. The field holds one boss, and the view draws the live boss
  or its corpse, never both. If two showed, tell me where (perhaps the beaten boss's corpse as a new one came in).

---

## 5. Phase 2: the words, drafted in one file for approval

One draft file, `docs/TEXT-DRAFT-2026-11-2x.csv` (`tools/text_audit.py` checks it), plain and short (D15). After
approval: the string tables, then the 22 translations (`tools/loc_status.py --todo` / `--apply`).

| Lines | For | Count |
|---|---|---|
| Vitality's new name (+ tree label, achievement name, blurb check) | D-l | 4 |
| Auto-pick: the card screen's box, "You can turn it off in Settings.", the Settings box | D-e, P4.5 | 3 |
| Quit: pause "Quit game", "Save and quit?", Save and quit / Cancel | P4.7 | 4 |
| Unspent Stardust: "You have unspent Stardust.", Leave / Stay | P1.1, P4.7 | 3 |
| Bosses: "Summon {boss}", "Beaten", the strength meter's words, "{boss} left. Try again when you are stronger." | E2 | ~5 (and ~14 Bait lines retired) |
| Hot Borders' and Slow Burn's new descriptions | E5 | 2 |
| Upgrades: one short line each (15) + "Now", "Next" | P4.1 | 17 |
| Achievements: how to earn each (39). The Steam draft `docs/STEAM-ACHIEVEMENTS-DRAFT-2026-11-26.csv` is reused, so one approval serves both. | P4.1 | 39 |
| Catches: "This run's cards", the total line's form | P4.6 | 2 |
| Card face: "Level {n}" (no "of max") | E4 | 1 |
| Kitten Count's row: "+1 kitten" | D-g | 1 |
| Fence, Reputation, Compound Interest without "wave" | D-f | 3 |
| Tree tooltip labels, as the new design needs | C1 | a few |
| The tutorial's lines, rewritten to the VOCAB rules (Godot's said "mice" and "forever") | P4.4 | ~11 |

---

## 6. Phase 3: concepts to approve before building (look changes)

- **C1 The skill tree and its tooltip (notes 15, 21).** A canvas artifact, made with the `game-ui-art` skill, using the
  real data (`SkillBranchData.csv`, `ST_Tree`), hover to see tips; 2-3 directions to choose from.
  - **Why it looks like another game today** (`PawSkillTreeScreen.cpp`, `PawTreeSky.cpp`, `PawTreeTables.h`):
    - a night-blue starfield (`SKY_BASE #2D3A63`);
    - thin straight links on a lattice;
    - hexagons, slate locked nodes with no ink edge;
    - spinning rainbow rings and breathing halos;
    - pale floating captions.
  - **The rest of the UI is Sticker Box:** cream card stock, a 3.5 px ink `#2A2131` edge, a hard ink shadow, a white
    die-cut rim, Fredoka titles, Nunito text.
  - **The direction:**
    - **a darker background** (deep ink-plum, a quiet texture, no glow);
    - nodes as die-cut sticker plates with the thick ink edge, white rim and hard shadow;
    - thick ink links with a cream core when owned;
    - captions on small paper tabs;
    - owned / buyable / locked shown by fill and a badge, not by glow;
    - the Unicorn still special.
  - **The tooltip:**
    - icon, name and level pips;
    - one line of at most ~8 words on what it does;
    - **Now → Next as two big numbers;**
    - a cost chip with the Stardust icon;
    - one coloured chip only when something blocks the buy ("Needs Cat Burglar").
    
    Today it is up to 8 lines of the same 17 px text: the blurb, a range sentence ("+15% to +130% … over 10 levels"),
    Now, Next, Cost, the reason, the prompt (`Pc::TreeTip`, `PcTreeText.cpp:113-163`).
- **C1's layout, v3 (2026-11-26).** The designer, on v2's boards: "It should be a tree not categories. Research similar
  games skill trees".
  - **The research** (Bills Must Be Paid, Nodebuster, Cookie Clicker's source, Idle Slayer, Unnamed Space Idle, Spirit
    Hunters, Soulstone Survivors, Rogue Legacy 2, Path of Exile's tree data) found the best-liked trees share:
    - one root, with branches told apart by colour and direction, never panels;
    - skills on a grid, with short straight or 45° links;
    - the whole tree on one screen where it fits;
    - reveal one ring ahead (full fog was Spirit Hunters' biggest complaint);
    - level and price readable without hovering.
  - **v3:** layouts A (Sunrise, the root at the bottom, recommended) and B (Compass, the root in the middle).
    - They are worked out by a greedy grid placement:
      - each branch prefers its own slice;
      - a skill takes the free square near its parent that grows one ring out;
      - no shared squares, no crossing links, no link over a skill.
    - Measured on the real tree: Sunrise 17 × 10 squares, Compass 15 × 14.
    - Branch colours; a square for a skill with levels, a circle for a one-time unlock, a diamond for a job.
    - Price chips on what can be bought now, level counters on owned skills.
  - **P4.2 ports that placement to PawCore** (`PcTreeLayout`, replacing the recorded Godot layout and its fixture).
  - **The designer picked it, 2026-11-26:** "The compass is better but can you space it out a little? All icons are
    touching. There should be some space between them".
    - v4: Compass, the grid pitch 132 → 172 with an 84-unit skill (a gap of about 75 units between neighbours).
    - The view frames what is revealed.
    - Dark cards (v2's answer).
- **C2 One tooltip card for Upgrades, Achievements and Catches** (notes 14, 23): the same card as C1's tooltip, placed
  beside the dock on the field side, level with the row.
- **C3 The signatures' edge** (notes 17, 20; D-h). **Picked and built 2026-11-27 (D31; HANDOFF-LOG (160)):** "Make the
  outline thinner (b) and change color to gold": B's glow (1.10x, half the alpha) in gold, their size unchanged. The sheet
  (`build/review/c3_signatures.png`, `tools/art/review_signatures.py`) compared:
  - today;
  - a thinner, fainter glow;
  - no glow at 80 px;
  - a different signature mark.
  
  The moth, both dogs and the goose included.
- **C4 The Engineer's hat from the side** (note 28): redraw `hardhat` in `tools/art/concept_hats.js` (today it is
  mirror-symmetric, a front view), kept inside today's hat square so `PawFieldLook.h`'s constants stay. A sheet: the cat
  facing both ways, the job icon, the job picker's kitten.
- **C5 The Summon button and the boss's size** (notes 33, 42; D-c, D-n): a HUD mock where the button clearly differs
  from the menu rows (the boss's portrait, its own colour, the strength meter); the Park Tyrant at 1.4 / 1.8 / 2.2 on
  the field beside the cat.
- **C6 Vitality's icon:** a heart today (`make_sweet_art.py:1315`) → a kitten.

---

## 7. Phase 4: features

### P4.1 One hover tooltip for Upgrades, Achievements and Catches (notes 14, 23) [L]
- **Upgrades built 2026-11-27** (HANDOFF-LOG (159)): `UPawTooltip` (WBP_Tooltip, grafted into WBP_HudScreen) in C1's look;
  `Pc::UpgradeTip` / `Pc::PlaceTip`; the row that shows its ring (hover or pad) gets it, beside the dock, level with the
  row; the approved words (`TIP_now`, `TIP_next`, `TIP_short`; `UPG_*_desc` as UpgradeData.csv's `description`, English
  only). **Left:** the achievements (their "how to earn" lines drafted in `docs/TEXT-DRAFT-2026-11-27.csv`, one per kind of
  goal), the Catches rows (with P4.6), the tabs and quick bar, the tree's tooltip moving onto the card (`TIP_gives`).
- **Today:** shop rows show nothing on hover. The tabs and the quick bar use the engine's plain tooltip. Achievement
  rows can't be hovered (`SelfHitTestInvisible` plates). An achievement's line is only its reward
  (`ACH_boss_1_desc` = "Reward: 4 Stardust"); nothing says how to earn it.
- **Build:**
  - A `UPawTooltip` piece (new class in `PawUi::BakedClasses`, baked once, grafted into `WBP_HudScreen`) in C2's look.
  - **Placement:** on the field side of the dock, centred on the row, kept on screen. Shown on hover and on pad
    selection; hidden on a tab change.
  - **PawCore `Pc::UpgradeTip`:** now, next, the change, price, affordable, maxed; L1 tests.
  - **Upgrades show:** name and level, the one line (§5), Now → Next, the cost ("You have N." in red when short).
  - **Achievements:** plates made hoverable and focusable; show name, how to earn, a progress bar, the reward, the
    state.
  - **Catches rows** use it too. The tabs and quick bar move to it as well (one look).
- **Tests:** placement, content per state, a sabotage each.

### P4.2 The skill tree redesign (notes 15, 21) [L, after C1]
- **Built 2026-11-26** (HANDOFF-LOG (158)): everything below but the sky (kept: the designer asked for it on
  2026-09-14, "purple dark with cloud like things floating around"), the grafts (none needed: the board is built at run time)
  and the tooltip (with P4.1). The first-open cost was not measured again.
- **Touches:**
  - the look code in `PawSkillTreeScreen.cpp` and `PawTreeTables.h`;
  - ~~the sky tokens in `tools/art/palette.py`~~ (kept);
  - grafts into `WBP_SkillTreeScreen`, `WBP_TreeNode`, `WBP_HexPlate`, `WBP_TreeLinks`, `WBP_TreeSky`,
    `BP_TreeNodeStyle`;
  - the tooltip's rule `Pc::TreeTip` reshaped (fewer lines).
- **Tests:** `Pawpocalypse.UI.Tree.Paint / Text / Browse / TipOnce`, `Layout.Tree`, the `tree_text_godot.tsv` fixture
  rows, the `tree_*` sabotages, the sky check (`PcRainbowTests.cpp:387`).
- **While here:** the tree's first open costs 114-127 ms; create it ahead (open since HANDOFF-LOG (145)).

### P4.3 Boba (notes 2, 3) [M]
- **Built 2026-11-27** (HANDOFF-LOG (159)): her 14 photos (`SourceArt/boba`, `import_art.py --run boba` → DA_Boba); the
  title's card straight with the first photo, a hover pop and a click; PawCore `PcBoba` (the clock, the kinds, the paths,
  the bounce easing) and `UPawBobaSubsystem` / `UPawBobaLayer` (WBP_BobaLayer, over every menu); the title, pause,
  Settings and victory (the nyan 1.5 s in) open her clock, the skill tree cuts and holds her. A forced cameo obeys the
  effects switch too (the first try's force() did not). **Not done:** the pause menu's "CEO on duty" photo (new words).
- **Today:** the title has an empty 220 px photo well in a card tilted 4° (`R4BobaAngle`, `PawUiConstants.h:327-333`;
  `PawMainMenu.cpp:324-366`), no click and no cameo. No Boba picture is anywhere in this project.
- **Build:**
  - **Photos:** copy the first try's 14 cut-outs (`C:\Games\pawpocalypse\assets\art\boba\boba_*.png`, 640 px, read
    only) into `SourceArt/boba/` (LFS), imported as `T_boba_*` by a new `import_art.py` set.
  - **The card:** straight (angle 0 in the constant and the asset), the photo in the well; a click starts a cameo, with
    a hover pop.
  - **The cameo** (`UPawBobaSubsystem`, a PawCore `PcBoba` for the timing and paths), as WORKLIST-SLICE6 §6c surveyed:
    - **how she moves:** she flies across, spins, crosses with a rainbow trail, peeks from an edge, or drops in with a
      bounce;
    - **where and when:** on the title, the pause menu, settings and the victory screen, every 25-60 s;
    - **never over the skill tree** (CLAUDE.md, Q8);
    - plays `boba_cameo`; off when effects are off.
- **Tests:** the card's look, the click, the timing, never on the tree, plus sabotages.

### P4.4 The tutorial (note 7) [L, after E2 because its boss step depends on the new rules]
- **Today:** only the Controls page's "Tutorial tips" box exists (`PawControlsPanel.cpp:115`, `bTutorialTips`), and
  nothing reads it. The save already keeps `tutorial_done`.
- **The first try's design** (`tutorial.gd`, WORKLIST-SLICE6 §6d):
  - the paw cursor tapping the one thing to do next, with at most one short line on a paper plate;
  - never takes input;
  - each step shows after its condition held 1.5 s, gives up after 45 s, and is done when the player has done it
    (shown or not);
  - a veteran skips most.
  - **Its steps:** hover enemies, buy, the first kitten, a new biome, the boss, Reawaken, claim an award, Pounce, the
    first card, the first tree buy.
- **Build:**
  - PawCore `PcTutorial` (facts at 10 Hz, the step order, the done flags).
  - `UPawTutorialOverlay` (baked WBP, hit-test invisible, over the HUD and the tree).
  - The steps adapted to this game's rules (the boss step to Summon).
  - The Controls box made to work.
  - `-NoTutorial` for the soak, bench, tours and store tools.
- **Tests:** each step's condition and placement, a sabotage each.

### P4.5 Auto-pick from the card screen (note 36) [M, per D-e]
- **Built 2026-11-26** (D29; HANDOFF-LOG (158)): auto-pick free; Auto-Card retired, not deleted (`retired_branches`,
  refunded on load; the Godot replays keep the first try's rules); the card screen's row (Reroll, the box, its note); the
  Settings Game page. Left for P4.6: the Catches page's toggle (hidden for good: nobody can own Auto-Card).
- **Build:**
  - A check box under Reroll (`--graft "CardPickScreen=add:AutoPickCheck"`). Turning it on takes a card from this hand
    at random at once, and every later hand.
  - The line "You can turn it off in Settings."
  - A Gameplay box in Settings, bound to the loaded slot's `bAutoPickCard` (auto-pick lives in the save slot; the box
    shows only in a game).
  - If D-e is "free": `Pc::HasAutoCard` stops asking for the node; the node is removed (its Stardust refunded); the
    tree's text "You can turn it off in Catches." goes.
- **Tests:** the box, the take at once, a new game starts off, plus sabotages.

### P4.6 The Catches page: this run's cards and their totals (note 19, the page) [M]
- **Today:** every card in deck order, held first (`PawHudScreen.cpp:2620-2710`).
- **New:**
  - only the cards taken this run;
  - each with its level and what it gives in total ("Hot Borders ×3: edges burn for 0.48× aura damage");
  - effects shared by two cards shown as one total.
- **Lands with E4** (uncapped levels), which changes the totals' form.

### P4.7 Quit prompts and "Save and quit" (note 43) [M]
- **Built 2026-11-26** (HANDOFF-LOG (158)): `UPawConfirmDialog` (WBP_ConfirmDialog), the pause menu's Quit game (grafted
  `add:QuitGameButton`), the tree's Leave / Stay (`remove:WarningPlate`; the words TREE_warn_unspent, TREE_warn_next and
  TREE_leave_unspent retired with their translation rows). The title's Exit and the Reawaken confirm keep their own
  prompts (both already ask). The new key for the dialog's yes is `PAUSE_quit_save` (`PAUSE_save_quit` was taken).
- **Today:**
  - The title's Exit button quits at once (Esc asks).
  - The pause menu can only go back to the title (an armed second press, "Save and quit to menu" / "Quit WITHOUT
    saving") and can't quit the game.
  - Closing the window saves silently.
- **Build:**
  - One confirm dialog on the Modal stack (`UPawConfirmDialog`, a new baked WBP), used by:
    - the title's Exit (P1.14 first uses the old prompt);
    - a new pause button "Quit game" → "Save and quit?" [Save and quit] [Cancel];
    - the tree's unspent Stardust [Leave] [Stay];
    - Reawaken's existing confirm.
  - **Closing the window:** keep the silent save, no prompt (recommended: nothing is lost).
- **Tests:** each path asks once and saves, plus sabotages.

### P4.8 A narrower dock (note 24) [M, per D-j]
- **Built 2026-11-27** (D30; HANDOFF-LOG (159)): 480 px, the wraps follow `DockWidth`, the asset grafted (DockBox and 13
  wrapped texts), the key legend wrapped; all UI and Input tests and the 23-language pass at 1280 x 800, 150 % UI. Not
  done: reading the width from the asset (the code's constant and the asset agree, and Parity now checks the wraps);
  `packaged_languages.py` waits for a package (the designer's word).
- **What changes:**
  - `PawUi::DockWidth` 568 → 480, with the wraps that come from it (`PageWrap` 520, `ButtonWrap` 480 and the
    job/award wraps, `PawHudScreen.cpp:88-106`);
  - `--graft props:DockBox` so the asset matches;
  - better: read the width from the asset once (it's in two places today).
- **Follows by itself:** the field gets wider (`Pc::PlayfieldRect`), and the cat's clamp (P1.8) follows.
- **Checks:** `Layout.Dock / Languages / PseudoLocale / TextMisses`, and `tools/packaged_languages.py` for the long
  German and Russian rows.

---

## 8. Phase 5: the economy pass (measured, proposed, then built on the designer's yes)

The notes ask for six economy changes at once: fewer enemies and a common-heavy mix, no Bait, uncapped cards, dearer
and slower kittens, a weaker Hot Borders, and a slower game. They interact, so they are measured together and proposed
as one set, with numbers, in a new `docs/ECONOMY-REVIEW-2026-11-xx.md` (as D17 and D24 were). The tools come first.

### E0 Tools first [L]
- **pawsim learns:**
  - the roster rule (weights that depend on the spawn multiplier);
  - kittens +1 a buy;
  - bosses without Bait (a fight is won when the cat's damage over the time limit beats the HP, with a margin);
  - **the card effects that change income:** edge burn (as the share of escaping enemies it kills), Bounty, Notoriety,
    Thunderclap, Ember Trail, Gilded Kitten. At least for a typical pick order; uncapped levels included.
- **`econ_review` gets levers** for spawn rate, pools, shop growth, card XP, the kitten step, boss HP and time limits.
  Today it has none for these.
- **Calibration:** against the real game (the 5× soak's logs on a package, at the designer's word), so pawsim's minutes
  mean the game's minutes.

### What is measured today (`econ_review`, today's `Data/`, buys at once, Reawakens when allowed)

| Milestone | Today | Backyard 60/min only* | A strict 90/10 ladder* |
|---|---|---|---|
| First kitten | 3.4 min | 5.0 | 3.6 |
| Biome 2 | 4.6 min | 6.3 | 5.7 |
| First Reawaken | 9.8 min | 11.5 | 12.1 |
| First boss | 50.5 min (90 s fight) | 56.1 | 60.1 (174 s fight) |
| Game won | 175 min, 14 Reawakens | 182 | 235 |
| Waits over 3 min / longest "nothing new" | 12 % / 17.2 min | 12 % / 17.2 | **29 % / 55 min** |

\* In memory, `Data/` not written. The strict ladder alone slows the game but brings back long waits (signatures,
fliers and gristles paid much of the Catnip), so the gates, prices and bosses are re-tuned with it, not after.

DPS today: 8 at the start, 104 at 5 min, 9,500 at 20 min, 63,000 at 30, 4×10⁷ at 60 min.

### E1 Spawning: fewer enemies, one common, rares that grow with density (notes 9, 18) [L]

**How it works today:**
- Each biome spawns `spawn_rate_per_min` (Backyard 120) × your spawn multiplier.
- The multiplier is the Spawn Rate shop line ×1.77 a level (up to ×8 + 0.6 a biome + 0.3 a Reawaken), times Swarm,
  Infestation, Ratcatcher and Slow Burn's cost.
- At most 320 enemies are alive. An enemy lives ~14 s on the field, so the Backyard's start is ~29 alive (~50 after the
  first Spawn Rate level at 0:32, ~90 after the second at 1:31).
- **The type is one weighted roll over 4-6 kinds that never changes** (`PcFieldRules.cpp:119-142`):
  - Backyard: mouse 92, moth 5, vole 3, gristle 2.
  - Biomes 2-7: last biome's common 15, own common 52, own signature 18, next biome's common 15, flier 10, gristle 3-6
    (own common under half).
  - Biomes 9-12: 4 kinds.

**The proposal (D-b), from the research (§9):**
- Few kinds at once: Vampire Survivors never spawns more than 3 types in a minute.
- A type previewed as rare, then made common: Vampire Survivors; Bills Must Be Paid's piggy ladder.
- Each tier tougher and paying a little better.

| Role | Which | Base weight | Grows with spawn rate (s^a) | Joins at |
|---|---|---|---|---|
| Common | the biome's own common (the Backyard mouse) | 88 | a = 0 | always |
| Rare | the next biome's common (the Alley vole): next biome it IS the common | 8 | a = 0.6 | always |
| Rarer | the biome's signature (the moth): keeps its card roles (2 catches, Bounty, Notoriety) | 4 | a = 0.8 | always |
| Special | the flier | 3 | a = 0.5 | spawn multiplier ≥ 2 |
| Special | the gristle | 1.5 | a = 0.7 | spawn multiplier ≥ 4 |
| Jackpot | the golden | 1 % roll, as today | - | always |

The share each kind gets as your spawn multiplier s rises (from the table above):

| s | common / rare / signature / flier / gristle |
|---|---|
| 1 (a new game) | 88 / 8 / 4 / - / - |
| 2 | 79 / 11 / 6 / 4 / - |
| 4 | 68 / 14 / 9 / 5 / 3 |
| 8 | 58 / 18 / 14 / 6 / 4 |

- **The chain of commons:** mouse → vole → park rat → pigeon → sewer rat → slime rat → ghost → imp → ant → hamster →
  factory rat → dust bunny (the last biome's rare is its own signature). Biomes 9-12 join the chain; today they carry
  nothing over.
- **The starting count halved:** Backyard 120 → 60/min (~15 alive at the start), and a weaker or dearer first Spawn Rate
  level so it doesn't double straight back. Later density is checked too: at a full spawn multiplier the Backyard would
  still have ~140 alive.
- **The invariant holds:** the mix reads only the player's own spawn-rate boosts, never the cat's damage.
- **Economy effect:** rares pay more per kill, so Catnip per spawn rises with s, and the Spawn Rate line becomes worth
  more than today. It is re-priced in the same pass.
- **Touches:**
  - `Data/TierData.csv`: new columns (a per-kind exponent and entry threshold) through `gen_paw_rows.py`, the row
    structs and `import_data`;
  - `WeightAt` / `PickEnemy` / `SpawnOne` / `ForeseeNextSpawn` (`PcFieldRules.cpp:30-35, 119-142`,
    `PcField.cpp:2061-2119`);
  - a validator ladder rule;
  - fixture `field_rules_godot.tsv` (81 rows probe the Backyard);
  - `PcFieldTests`, `PcValidateTests`, `PawCardEffectsTests.cpp:640, 714`;
  - sabotages `field_pick_ignores_signature_weight`, `cards_signature_frequency`;
  - pawsim's tier model;
  - doc 02 §4.
- **Optional:** a new game's first minute is mice only (the tutorial's "hover over enemies" step).

### E2 Bosses: Summon, strength is the gate, no Bait (notes 33, 42) [L]
- **Today:**
  - Bait = kills in the biome ÷ `kills_per_bait` (480 to 5,600; 40 Bait a boss: 19,200 kills for the Park Tyrant), or
    Catnip under the Audit node.
  - Bait is banked across Reawakens (E2 of D17).
  - Auto-Bait polls every second.
  - A save mid-fight refunds the summon (F36).
  - Time limits are 300-3,700 s, 2-20× the intended fight (91-186 s), so today patience beats strength.
  - "Park Tyrant — 30 more bait" means about 30 × 480 = 14,400 more kills, which nothing says (note 33's "30 what?").
- **New (D-c):**
  - The Bait button becomes **Summon {boss}** (C5's look), free, in the boss's biome.
  - A time limit of about 90-120 s.
  - A boss not beaten in time **leaves** (summon again any time, at full HP).
  - A beaten boss is gone for good.
  - The strength meter: the cat's DPS against HP ÷ limit.
  - **Boss HP re-priced against the cat's damage curve** so each boss falls at D-a's target time. This replaces Bait
    as the frontier's gate (D24's lever).
  - Bigger boss (D-n), and hits count over the whole body.
- **Removed:**
  - Bait, its bank, its Catnip price, F36's refund (nothing is paid);
  - `TickAutoBait`;
  - the tree nodes Audit, Deep Pockets and Auto-Bait (Stardust refunded; their children re-parented);
  - ~14 Bait strings.
- **Old saves:** the keys `bait_spent`, `bait_kills` and the summon payment are still read and ignored.
- **Touches:**
  - PawCore: `PcEconomy`, `PcProgress`, `PcField` (`CanSummonFieldBoss` asks "not beaten"), `PcHud`, `PcSave`,
    `PcValidate` (it rejects a zero Bait today);
  - tests: PcFieldBoss, PcHud, PcProgress, PcSave, PcEconomy (`economy_godot.tsv`'s `bait_*` columns), PcDataTests' ten
    D24 numbers (`:161-172`), PcFieldSoak;
  - the game module's tests and the soak, pad-play, shot and cheat drivers;
  - ~25 PawCore and 4 realm sabotage entries;
  - pawsim's Bait farm (`pawsim.py:469-472, 730-742, 1505-1640, 2339-2410`);
  - the tree fixtures (`tree*_godot.tsv`);
  - doc 02;
  - doc 01: a new D-entry that replaces D17 E2, D24's Bait half and F36.

### E3 Kittens (notes 12, 13, 16) [M, per D-g]
- **Kitten Count:** +2 → +1 a buy (`UpgradeData.csv:7`), with a dearer base and growth from the pass.
- **Kitten speed:** `kitten_speed_px_per_sec` 260 → 180 (`CodeConstants.csv:47`); the roaming speed follows (0.45×).
- **The Kitten Speed cap bug** (P1.15).
- **Touches:**
  - the D17 test `PcStatsTests.cpp:298-299`;
  - `kittens_godot.tsv` (599 rows at +1; the positions recorded at 260 px/s);
  - `progress_godot.tsv` (146 rows);
  - the "Full House" achievement;
  - doc 02:193.

### E4 Cards without a level cap (note 19) [L, per D-d]
- **Today:** a card's levels are its `level_values` list (2-4). Past it, `ValueAt` / `CostAt` return 0
  (`PcCards.cpp:19-35`), so simply removing the cap would switch cards off.
- **Breaks uncapped:**
  - Catnip Bomb (stops at 0), Close Quarters and Feral Focus (aura to 0), Compound Interest and Hoarder (run away),
    Rigor (enemies freeze, then the slow switches off), Slow Burn (spawns to 0), Stampede (speed unbounded), Rainbow
    Trail, Dilated (unbounded hand).
  - Multiplying cards (Bounty, Gilded Kitten ×32 doubling on) need an additive step.
- **Build:**
  - per-card columns in `Data/CardData.csv` for what each take past the table adds, and its floor/limit (D-d);
  - `ValueAt` / `CostAt` extrapolate;
  - `IsCardEligible` no longer drops a maxed card;
  - `ChooseCard`'s clamp and the save's load clamp (F49) become sanity limits;
  - the validator's `card.monotone` rule;
  - the card face shows "Level n", no dots;
  - Prototype's bonus levels.
- **Touches:**
  - fixtures `cards_godot.tsv` (944 of 1,000 rows hold a maxed card) and `draw_godot.tsv` (963), edited by a script;
  - `PcHudTests:769, 774, 914`, `PcLongTests:90`, `PawCardEffectsTests:244-252, 877, 977`, `PawCardPickTests`, the
    F49 save test;
  - sabotages `draw_choose_uncapped`, `catches_maxed_unsaid`, `Row.MaxLevel`, `load_*`.

### E5 Hot Borders and Slow Burn (notes 30, 40) [M, per D-k]
- **Today:** every enemy within 90 px of the field's edge takes aura DPS × the card's value each second, wherever the
  cat is. Every enemy is born inside that band (24 px in) and every escaping enemy walks out through it.
- **Measured** (Monte Carlo of the field rules):
  - The band is 28 % of the field; the base aura covers 1.3 %.
  - At level 3 an enemy gets ~0.75 s of free aura damage. Slow Burn level 2 adds 1.41 s.
  - Anything with less HP than that dies at the edge untouched. Hot Borders alone does ~5× the base aura's total
    damage.
  - It turns escapes into kills, so income per spawn can roughly double *(inferred)*.
- **Change:**
  - Hot Borders burns only enemies moving outward (−46 % exposure), numbers re-measured, its band drawn from the data.
  - Slow Burn gets its own `CardEffect` (appended, never renamed): an afterburn on enemies the aura touched, embers
    drawn on them, its cost unchanged.
- **Touches:**
  - `PcField.cpp:1526-1551`;
  - `PcEnums.h`, `PcCards`, `PawVfx`;
  - `CardData.csv:16, 26`, `TextOverrides.csv:100`;
  - `aura_godot.tsv` (~460 rows), `cards_godot.tsv` (~309);
  - `PawCardEffectsTests:366-380`;
  - both sabotage lists.

### E6 The pace (notes 16, 33, 34, 41) [L]
- **Levers, tuned together to D-a's targets:**
  - biome gates (`catnip_to_unlock`; biome 2 is 900 Catnip today);
  - shop prices and steps;
  - card XP (`card_level_cost_base` 24, growth 1.68);
  - Reawaken constants;
  - boss HP and limits (E2).
- **The aim behind "getting stronger should take more work":** today damage outgrows enemy HP so fast that every enemy
  dies in its first second. Damage lines should grow a little slower than enemy HP, so that for part of each biome some
  enemies escape and a damage buy shows.
- **Kept:** D17's goals (waits, nothing new).
- **Measured:** pawsim and `econ_review`, then the 5× soak on a package (the designer's word), then a playtest.
- **Recorded:** doc 01 D-entries (D27 on), and doc 02 rewritten where the rules changed.

---

## 9. The research behind E1 (summary)

Sources: https://store.steampowered.com/app/4421010/Bills_Must_Be_Paid/ ,
https://gameshorizon.com/guides/bills-must-be-paid-all-piggy-types-explained/ (a guide: may be dated),
https://vampire.survivors.wiki/w/Mad_Forest , https://brotato.wiki.spellsandguns.com/Waves ,
https://store.steampowered.com/app/3107330/Nodebuster/ , https://riskofrain2.wiki.gg/wiki/Directors .

- **Bills Must Be Paid** (Rike Games, Steam 2026-07-29; app 4421010):
  - 15 piggy types that always unlock in the same order;
  - each about 2× the HP of the last and paying ~16 % more per HP;
  - each with one trait (hard to hit, charges, explodes, restores stamina);
  - one rare "Rainbow" piggy that pays ~40× per HP;
  - breaking the first piggy gives a 70 % chance its "big brother" (tier 6) appears.
  
  Reviews' one complaint: piggies that look different but play the same. Exact spawn weights are not published.
- **Vampire Survivors:**
  - at most 3 enemy types in any minute;
  - types rotate each minute;
  - a type often appears first as a single big carrier, then a minute later as a common wave enemy.
- **Brotato:** one new type every 1-2 waves; one "Looter" that flees and pays ~8×; hordes pay 35 % less per enemy so
  more bodies don't mean more income.
- **Nodebuster:** rare enemies pay a different currency (no dilution); rarity is bought (+1 % a level).
- **Risk of Rain 2:** more spawn budget buys pricier, rarer monsters, not more bodies.
- **Lessons for E1:**
  - few kinds at once;
  - preview a type as rare, then make it common;
  - keep Catnip per HP rising gently per tier (~15-20 %);
  - make each new common change one thing the cursor must deal with;
  - a rare that pays far more per HP takes over income as its share rises, so the rares' reward per HP stays near the
    common's.

---

## 10. Order of work

1. **Phase 1** on the designer's go (one or two sessions; no answers needed).
2. **Meanwhile:**
   - the designer answers §2;
   - I draft §5's words;
   - I make §6's concepts (C1 first: it shapes P4.1 and P4.2).
3. **E0** (pawsim and `econ_review` learn the new rules and the cards).
4. **The economy proposal** (E1-E6 together, with numbers) → the designer → build.
5. **Phase 4** as each concept and its words are approved: P4.1 tooltip and P4.2 tree, P4.3 Boba, P4.5 auto-pick,
   P4.6 Catches, P4.7 quit, P4.8 dock. Then **P4.4 the tutorial last** (it points at the finished screens and rules).
6. **Close:**
   - `check_all` exit 0, plus a `-Full` run (owed since 2026-11-12);
   - `screen_tour` shots of every changed screen;
   - on the designer's word: a package, the packaged gates, and Steam upload 0.1.3;
   - Steamworks: the renamed achievement, the icons;
   - docs 01, 02, 05, 07, 08, 10 and the KB (the focus/ScrollBox lessons, `Build/` vs `build/`).

---

## 11. Every note and where it is handled

| # | Note (short) | Where |
|---|---|---|
| 1 | first launch at the monitor's resolution | P1.10 |
| 2 | Boba's picture missing; the app icon | P4.3; P1.11, D-m |
| 3 | Boba's card straight, click, fly/jump/pop | P4.3 |
| 4 | settings hover cut off; remove "The interface…" | P1.4; P1.5 |
| 5 | Report a bug → Steam discussions | P1.9 |
| 6 | slot 1 always large | P1.2 |
| 7 | the tutorial tips | P4.4 |
| 8 | remove the wave bar | P1.6, D-f |
| 9 | too many enemies at the start; mostly mice; half as many | E1, D-b |
| 10 | remove "Resets Catnip…" | P1.5 |
| 11 | Esc leaves the tree | P1.1 |
| 12 | first kitten gave 2 | §1.5, E3, D-g |
| 13 | kittens slower | E3, D-g |
| 14 | upgrade hover tooltip | P4.1, C2 |
| 15 | tree tooltips redesigned, less text | C1, P4.2 |
| 16 | kittens dearer; biome 2 later | E3, E6, D-a |
| 17 | moth and dog edges too thick | C3, D-h |
| 18 | enemy progression research; the ladder | §9, E1, D-b |
| 19 | no card level cap; Catches shows this run's cards and totals | E4, P4.6, D-d |
| 20 | goose edge too thick | C3, D-h |
| 21 | tree redesign, canvas artifact, darker | C1, P4.2 |
| 22 | remove "This card has a downside." | P1.5 |
| 23 | achievement tooltips | P4.1, C2 |
| 24 | narrower side menu | P4.8, D-j |
| 25 | pause menu centred, no icons | P1.7 |
| 26 | Upgrades red dot always on | P1.3 |
| 27 | unspent Stardust warning never leaves | P1.1, P4.7 |
| 28 | Engineer's hat from the side | C4 |
| 29 | golden kitten before any kitten | §1.6, D-l |
| 30 | Hot Borders too strong | E5, D-k |
| 31 | aura: no ring, just a feather | P1.13 |
| 32 | Thunderclap has no visual | P1.13 |
| 33 | Bait button unclear; Bait cheaper but later | E2, C5, D-c |
| 34 | the game is a speedrun | E6, D-a |
| 35 | the cat goes under the menu; stays on alt-tab | P1.8 |
| 36 | auto-pick on the card screen | P4.5, D-e |
| 37 | music half as loud | P1.12, D-i |
| 38 | rename Vitality | D-l, §5, C6 |
| 39 | Ember Trail red | P1.13 |
| 40 | Slow Burn not tied to the borders | E5, D-k |
| 41 | getting stronger should take more work | E6, D-a |
| 42 | Summon bosses, strength is the gate, bigger | E2, C5, D-c, D-n |
| 43 | quit prompts, Save and quit | P1.14, P4.7 |
| 44 | autosave? | §4 (yes) |
