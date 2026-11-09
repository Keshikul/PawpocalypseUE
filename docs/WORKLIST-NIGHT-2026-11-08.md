# Night worklist, 2026-11-08 (unattended, from the designer's "go" to ~07:00)

> **Closed (history)** 2026-11-08 night; result in HANDOFF-LOG (70).

> Written 2026-11-08 21:55 from HANDOFF-LOG (69) and `WORKLIST-RESTYLE-2026-11-07.md` (R4 progress). The designer:
> *"Create an overnight plan."* and *"make sure all text is simple and concise in the ui. Don't overexplain. Use short
> sentences."* The status column is updated as the night goes. HANDOFF-LOG (70) closes it.

**The night's job: close R4, and draft short text for the whole UI.**
- R4's last exit check fails: at 150 % UI size, four screens overflow. Fix them, prove it with a test, then do R4's
  review beside the canvas boards.
- The text: 128 of 841 strings run over 60 characters; 41 have a sentence over 14 words. Most are skill tree blurbs. The
  night drafts short versions. Nothing goes into the string tables until the designer says yes (question B).

**No `-Full` tonight.** It takes about 10 hours and nothing else can run beside it. It has not run since the restyle began.
It should run the night after R2-R4 are committed.

## Before the designer leaves (answers needed)

| # | Question | Recommendation |
|---|---|---|
| A | **Commit R2, R3 and R4 first?** Nothing is committed since `f6a16b3`. A commit gives the night a clean diff and a safe point to go back to. Without it, each item backs up its files to `build/night_20261108/backup/<item>/`. | Yes: say "commit R2-R4". |
| B | **The short text: draft only, or write it in?** A changed English string loses its 20 Godot translations: `port_po.py` matches on the English. Godot is still the spec for content. | **Draft only.** You get a review page in the morning: each string, now and proposed. You approve per line. Then it goes in (Godot first, or Unreal only: your call then). |
| C | **The tree at 150 %: fix the centring only, or also open it zoomed in** (the board's big nodes)? Zooming in changes behaviour. | Centring only. Zoom stays your call. |

## Ground rules (CLAUDE.md)

- No commit, push, package or export, except the commit in A if you say yes. Explicit paths only.
- The Godot repo stays read-only.
- No Unreal editor open. One headless run or test run at a time. Every run sandboxed.
- The assets own their layout. Changes go in by graft (`slot:`, `remove:` + `add:`), never a re-bake.
- Every new assertion is seen failing. Every new sabotage is seen CAUGHT alone.
- No new player-facing words written anywhere tonight (B).
- **Stop rule:** a failure not understood in 45 minutes is put back from its backup, marked **parked** with the cause so
  far, and the night moves on.
- The night ends on an everyday `check_all` exit 0, whatever got done.

## The list

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight.** No Unreal process. A answered. `git status` saved to `build/night_20261108/start.txt`. Baseline: `check_all` exit 0 from (69); rerun only if something changed. | 15 min | start.txt written | **done** 22:05: A yes, B draft, C centre; R2-R4 committed `7a6023c` (453 explicit paths, not pushed); no Unreal process |
| 1 | **A layout check that can fail.** Measure first: can a headless automation test lay a screen out at 1920x1080, 100 % and 150 %, and read each named part's rectangle? If yes: test `Pawpocalypse.UI.Layout.Fits`: on the title, the dock (each page), settings (each section) and the tree, no named part leaves the screen and no two siblings overlap. If no: a `paw.LayoutReport` console command writes the rectangles, and `godot_tour.py --uiscale` fails on a bad one. Either way it must fail on today's four. | 1.5 h | fails on the four, for the right reason | **done**: yes, headless works (`UI/PawLayoutCheck`: prepass, paint, which ticks, arrange); four tests `Pawpocalypse.UI.Layout.{Title,Dock,Settings,Tree}` at 1920x1080 and 1280x800, 100 % and 150 %. Failed on the title, the shop page and settings (the tree through its sabotage). **Found:** without the DPI curve the 1280x800 case reported overflows the game does not have; with it 100 % passes everywhere. A tilted part is judged by its untilted box |
| 2 | **The title at 150 %.** The logo sits above the menu, not under it. Boba's card and the version stay on screen (anchored from the right edge, inside). By graft `slot:`. | 45 min | item 1 passes for the title; 100 % unchanged by eye | **done**: not by `slot:` alone: the logo and the menu in one scale-down-only room (`LeftFit`; at 150 % both ~1.1x, where a menu-only room made the menu 0.95x), Boba's card in its own (`BobaFit`), the logo's empty box 1100 -> 1000. 1080p identical (measured rects) |
| 3 | **The shop page scrolls.** Its rows go in a scroll box (graft `remove:` + `add:`). A focused row scrolls into view on pad. | 1 h | item 1 passes; `padplay_tour.py` 41 checks pass | **done**: `ShopList` (as the other pages; `add:ShopList`); padplay 41 PASS |
| 4 | **The settings window fits.** A height cap; the section body scrolls; Close stays pinned. | 45 min | item 1 passes; settings tests pass | **done**: the pages in `PagesScroll`, by a new graft op `wrap:Pages>PagesScroll` (the asset's pages kept as they are); the window's height was already clamped, its content was not |
| 5 | **The tree centred at 150 %.** Measure first why (likely the fit uses raw pixels, not the scaled size). Fix in code. Test: the tree's centre within a few px of the screen centre at 100 % and 150 %. | 45 min | test seen failing, then passing | **done**: not raw pixels: it fitted on activation from the LAST size and never refit when the real one arrived on the first tick. `SetViewSize` refits until the player moves the view; `.Tree.Opens` + `.Layout.Tree` |
| 6 | **Sabotages** for items 1-5 (e.g. `shop_not_scrolling`, `tree_fit_raw_pixels`), each CAUGHT alone. Also measured and listed, not fixed: 1280x800 at 150 % (the Deck with big UI). | 45 min | each CAUGHT | **done**: `shop_not_scrolling`, `tree_fit_stale_size`, `tree_refit_after_zoom`, `title_menu_room_low` CAUGHT (481). Settings has no code sabotage: the scroll lives in the asset (its negative test: `.Layout.Settings` failing before the graft). 1280x800 at 150 %: in the tests, passes |
| 6b | **No grudges, no "unimpressed" cat** (designer, 22:40, mid-night: "remove any mention of grudges or cats being unimpressed. They were supposed to be removed in godot but slipped through"). The victory line "The cat is unimpressed." dropped (`ST_UI`, the port's own table; the Victory test updated after it was seen failing). Docs and rules: CLAUDE.md, the restyle's face rule and checklist. **Grudge Ledger** (name, caption, blurb) needs NEW words: drafted in item 7, applied after the designer picks (Godot first: `ST_Tree` is generated from it). Godot finding F43. Ids `grudge_ledger` / `GRUDGE_PER_CARD` stay (save keys). `text_audit.py` flags the banned words. | 30 min | no player text says either, except the node waiting for its name | **done**: Victory test failed on the old line, passes on the new. `text_audit.py --banned` lists exactly the node's two names. **Found:** the concept canvas says "grudge" twice (title tagline, tree chip); not in the game; left for the designer |
| 7 | **The text draft (read-only).** New `tools/text_audit.py`: every string in `ST_UI`, `ST_Tree`, `ST_Data`. It flags a string over its budget (a label over 3 words; a line over 60 characters; a sentence over 12 words; more than 2 sentences). `--self-test`: a planted long string must be flagged. For each flagged string, a short draft: same meaning, same numbers, same `{action}` tokens, still mean and funny. Output: `build/look/text_draft.md` and a review page (now vs proposed, with character counts). Nothing written to the tables. | 2 h | every flagged string has a draft; self-test seen failing | **done**: 122 of 841 over budget (24 cards and jobs, 84 tree, 14 screens); 122 drafts in `docs/TEXT-DRAFT-2026-11-08.csv`, every one within budget; the self-test flags each planted fault and passes clean lines (a first version flagged stat lines as labels: the label rule now applies to names, captions, tabs and buttons only); in `check_all` L0. Review page: https://claude.ai/artifact/5KnwdnDv3AZAS6Kz3bESzS (private) |
| 8 | **R4's exit review.** Each screen beside its canvas board. Measure first how to get the boards as images (read the canvas, render it locally in headless Chrome). If that fails, use the boards seen in R4 and say so. Pass or a named fault per screen, into the worklist's R4 progress. | 1 h | the list written | **done, changed**: the canvas renders only in its own runtime, so the review used the boards' sources (as in R4) and the night's shots; pass and faults in WORKLIST-RESTYLE "R4's exit review". R4 closed |
| 9 | **Gate.** Everyday `check_all` exit 0. Docs: worklist R4 closed (or what is left), 05 §7, 09, 12. KB: anything new about headless layout tests and scroll focus. | 1 h | exit 0 | **done**: exit 0, 21 of 21 (`build/check_all_night_r4h.log`); docs 05 §7, 09, 12 (three rows), 00, 11, CLAUDE.md; KB U/09 §2.3, §6 (five facts), both indexes |
| 10 | **Close.** HANDOFF-LOG (70) with the exact next step. Docs 00 and 11. This list marked closed. | 30 min | written | **done**: HANDOFF-LOG (70); this list closed |

**Order:** 0, 1, then 2-5 in any order (each needs 1), 6, 9, 10. Item 7 needs nothing: it fills waits (builds, test runs).
Item 8 runs once 2-5 are in. Items 9 and 10 are never cut. If item 1 cannot be built, items 2-5 still run, checked by the
150 % tour by eye, and the night says so.

**Risks I can see now.**
- A scroll box inside a Common UI page can break pad focus or the focus tours. `padplay_tour.py` decides. If it breaks and
  the fix is not clear in 45 minutes, item 3 is parked.
- Headless layout may not match the real viewport (DPI curve, safe zone). Item 1 checks it against the 150 % shots first.
- The text drafts are mine. They need your eye: meaning, jokes and register.

## For the designer in the morning

- The text review page: approve, edit or reject each line.
- The R4 review list beside the boards.
- Question C, if still open. The hardware check from HANDOFF-LOG (55).
- If A was yes: the `-Full` night can be next.
