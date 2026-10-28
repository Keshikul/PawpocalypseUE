# 09 - Testing and gates

> KB: `Unreal/18-testing-automation.md` (layers, automation tests, fixtures, tours, input tours, release gate), `Godot/03`
> (the gate rules learned on this very game). Godot: 19 gates, 12,952 assertions, `bash tools/check_all.sh` ~75 s; a soak that
> plays a whole exported game. **The gate culture is the most important non-code thing being ported.**

## 1. Rules (carried from Godot, all paid for)

1. **A gate that has never failed has never been tested.** Negative-test every assertion: break the real code, watch it go red,
   and first confirm the sabotage LANDED.
2. **Assert the wiring as well as the behaviour.** Calling the function under test proves nothing about whether the game calls
   it (nine wiring gates were green while three skill branches could not be bought).
3. **The log is the verdict**: runners grep for `Error:`/`Fatal`/`Ensure condition failed`/`Test Completed. Result={Fail}` and our own
   `PAW_FAIL` lines, not only the exit code. Every runner has a timeout.
4. **Sandbox the user's data** in every automated run: saves (`-NoSave` or a temp save dir), settings (`-UserDir=` a temp dir /
   reset Enhanced Input user settings), so the designer's own game stays byte-identical. A gate that writes saves refuses to run
   unsandboxed.
5. **One command runs everything** (`tools/check_all.ps1`, with `-Fast` and name filters). A gate not in it stops being run.
6. **Export-only gates**: some bugs exist only in packaged builds (the Godot build that loaded zero evolution stages; Unreal's
   uncooked assets and FName case). They run on a packaged Development build before every release and at the end of every phase.
7. **Compare per-gate counts with a real run**, not a remembered total.
8. **Never run two editor test sessions at once** (they corrupt each other) `[KB U/18 §2]`.

## 2. Layers (cheapest first) `[KB U/18 §1]`

| Layer | Tool | Time | Runs |
|---|---|---|---|
| L0 Static checks | Python: no `float` in PawCore; no colour literals outside tokens; no `PlaySound` outside audio; no player text outside string tables; every gameplay constant mapped; data enums parse; no `GetAllActorsOfClass` in gameplay | seconds | every change |
| L1 PawCore unit tests | standalone exe (doctest/Catch2), no engine | seconds | every change |
| L2 Simulator checks | `pawsim` invariants (`verify_pawsim.py`), `check_sims.py`, **mirror** (pawsim vs PawCore CLI) | seconds | data/rule changes |
| L3 Engine automation tests `Paw.*` | `UnrealEditor-Cmd ... -ExecCmds="Automation RunTests Paw.;Quit" -TestExit="Automation Test Queue Empty" -nullrhi -unattended` | minutes | every C++ change |
| L4 Tours | `-game -RenderOffscreen` screenshot tour, pad tour, mouse tour (real Slate events) | minutes | UI/input changes |
| L5 Packaged gates | the same tests/tours against a packaged Development build | ~5-10 min | end of phase, before release |
| L6 Soak | packaged build plays the whole game at 5x (pace) and 20x (stability), plus a headless PawCore soak at 1000x | 30-150 min | weekly, before release |

## 3. The 19 Godot gates, mapped

| Godot gate | What it proves | Unreal home |
|---|---|---|
| `check.sh` (every script compiles, no `DirAccess` on `res://`) | build health | L3 build of Editor + Game targets; **Shipping compiles** (dev-only calls guarded) `[KB U/19 §2]`; compile-all-Blueprints commandlet (an uncompilable BP is "Unknown cook failure") |
| `check_resources.sh` (data loads, economy invariants) | data | L1 shared validator on the CSVs + L3 `Paw.Data.*` (DataTable parity with CSV, every row loads, ids lower-case) |
| `check_textures.sh` (import rules, backdrop seams) | art | L3 `Paw.Art.*`: every atlas texture Never Stream / sRGB / mips per kind; atlas rects in range; L4 seam shots per aspect |
| `check_ui_theme.sh` (9-patch geometry) | UI theme | L3 `Paw.UI.Theme`: tokens header matches `palette.py`; button styles share one shape; focus ring set |
| `check_cards.sh` (XP track, deck shape) | cards | L1 `PcCards` + validator |
| `check_card_effects.sh` (every effect RUNS in a live realm) | cards wiring | L3 `Paw.Realm.CardEffects`: a test world, every effect applied, its consumer observed (the Godot gate caught a handler calling a nonexistent method) |
| `check_overkill.sh` (nearest survivor, never chains) | combat | L1 `PcField` tests |
| `check_kitten_spread.sh` (pool spreads, no dogpile) | kittens | L1 `PcField` tests |
| `check_damage.sh` (+ `--export`) (damage, Reawaken, Bait, Pounce cap, bounty, Legacy, cat size; ladder ceilings x128) | combat/economy | L1 + L3 `Paw.Damage.*`; **L5 on a packaged build** (the evolution stages load, the cat does damage) |
| `check_playthrough.sh` (**a real run**: reset, earn, buy, travel, kill bosses, Reawaken, save, load, invariants) | the loop | L3 `Paw.Playthrough` in a test world through the real entry points; plus L1 headless version |
| `check_stress.sh` (every pool saturated) | limits | L3 `Paw.Stress`: 320 enemies, max kittens, 64 zones, max effects; frame-time sample |
| `check_sims.sh` | simulators | L2 |
| `check_mirror.sh` | game == sim | L2 mirror (three-way, doc 06 §8) + palette tokens |
| `check_steam.sh` (+ `--export`) | build config | L0 config checks (presets, plugins per target, never-ship folders) + L5 pak listing audit (doc 10) |
| `check_loc.sh` (every text through `tr()`, `.pot` current) | text | L0 text checker + L3 every string table key resolves in every culture; placeholder sets match |
| `check_input.sh` (Input Map contract, device switching, glyphs, prompts, Pounce on trigger, stick cursor, dock, modals, tree, remapping) | input | L3 `Paw.Input.*` + L4 pad and mouse tours with real Slate events `[KB U/08 §6]` |
| `check_padplay.sh` (**the whole loop with a pad alone**) | pad parity | L4 `-PadPlay` tour: title, shop, a card, travel, first boss, Reawaken, tree, settings, save and quit, pad events only |
| `check_tutorial.sh` (cues walked in a live realm; suppressed in soak/trailer/shots; never takes a click or focus; veteran saves skip) | tutorial | L3 `Paw.Tutorial` + L4 |
| `check_hud.sh` (HUD paths, slots, audio, pause, tree gating, Boba stays in menus, tree seating/spacing) | UI wiring | L3 `Paw.UI.*`: every `BindWidget` resolves (baked WBP/code parity `[KB U/09 §2]`), every tree node seated and reachable, one expanding child per page, Boba never referenced by gameplay code |

## 4. New tests the engine change requires

| Test | Why |
|---|---|
| **Colour gate** (doc 04 §3): palette swatches through the real sprite material and post-process, read back within +/-2 | The tonemapper/exposure trap |
| **Precision gate**: PawCore has no `float`; save round trip of 1e15+ values and 17-digit doubles; formatting of 1e3..1e300 | D13 |
| **Cook gate**: in a packaged build, every DataTable row found by id, every visual definition loads, every audio event resolves, string tables present in every culture | Cooker and FName-case traps |
| **Determinism**: the same seed and inputs give the same state after N steps; save at step K + load + continue == never saved | Fixed-step contract; save/busy rules |
| **Frame-rate independence**: the same game minute at 30, 60, 144 fps gives the same Catnip within tolerance | The 5x/20x soak lesson |
| **Travel cost**: biome change < 16 ms, no level load | doc 03 §4 |
| **Unfocused throttle**: focus lost -> frame cap applied; CPU% in the soak log | Idle-game power use |
| **Hardware cursor in a packaged build** | doc 05 §4 |
| **Capture mode**: HUD uses `CaptureDuringMouseDown`; UI actions do not consume field keys | `[KB U/08 §3]` |
| **Glyph coverage**: every pad button has art in every family; keyboard keycaps render every bound key | `[KB U/08 §6]` |
| **Font coverage**: every character of every culture's strings is in the composite font | boxes in packaged text |

## 5. The soak (Godot `tools/soak.sh` + `ci/soak.gd`)

- A dev-only `UPawSoakDriver` (packaged Development build, `-Soak -Minutes=150 -Speed=5`) plays through the UI's own rules: aims
  the real cat (virtual cursor), buys through the shop API the buttons call, takes cards, summons bosses with Bait when the Realms
  page would allow it, travels only through `IsBiomeUnlocked()`, Reawakens, buys tree nodes. **It asks what the UI would allow; it
  never reaches into the model.** It logs boss kills, auto-bait, the game's own auto-travel, Catnip-gate waits, in game minutes;
  frame time p50/p95/p99 and the worst frames with what happened; leaks (UObject count, memory); every `Error`/`Warning` line;
  stalls. Tutorial suppressed.
- **Trust 5x for pace, 20x for stability** (Godot measured 10 h at 5x vs 15-16 h at 20x). With the fixed step the gap should
  close: measure it, and record whether 20x pace now matches.
- **Never edit the soak driver while a soak runs** (Godot: bash re-read the script mid-run). Launch detached, watch the log.
- Report format compatible with the Godot `soak_report.txt` so runs can be compared across engines.

## 6. Tours and what the agent "sees"

- `-UITour=<mode>` screenshot tours offscreen at 1920x1080, 1280x800, 2560x1080, 3840x1080 and in the pseudo-locale at the largest
  UI scale; every screen, every dock tab, the tree zoomed in/out, each biome, each evolution stage, each boss `[KB U/18 §4]`.
- Input in one step, the shot in the next (capture happens at frame end).
- Review every image; crop small text to 1:1 or 2x before judging. Compare with the Godot `build/playtest/` and
  `build/tutorial_shots/` images side by side for parity (phase 9).
