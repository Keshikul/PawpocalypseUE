# Night worklist, 2026-11-08 (unattended, from the designer's "go" to ~07:00)

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
| 0 | **Preflight.** No Unreal process. A answered. `git status` saved to `build/night_20261108/start.txt`. Baseline: `check_all` exit 0 from (69); rerun only if something changed. | 15 min | start.txt written | |
| 1 | **A layout check that can fail.** Measure first: can a headless automation test lay a screen out at 1920x1080, 100 % and 150 %, and read each named part's rectangle? If yes: test `Pawpocalypse.UI.Layout.Fits`: on the title, the dock (each page), settings (each section) and the tree, no named part leaves the screen and no two siblings overlap. If no: a `paw.LayoutReport` console command writes the rectangles, and `godot_tour.py --uiscale` fails on a bad one. Either way it must fail on today's four. | 1.5 h | fails on the four, for the right reason | |
| 2 | **The title at 150 %.** The logo sits above the menu, not under it. Boba's card and the version stay on screen (anchored from the right edge, inside). By graft `slot:`. | 45 min | item 1 passes for the title; 100 % unchanged by eye | |
| 3 | **The shop page scrolls.** Its rows go in a scroll box (graft `remove:` + `add:`). A focused row scrolls into view on pad. | 1 h | item 1 passes; `padplay_tour.py` 41 checks pass | |
| 4 | **The settings window fits.** A height cap; the section body scrolls; Close stays pinned. | 45 min | item 1 passes; settings tests pass | |
| 5 | **The tree centred at 150 %.** Measure first why (likely the fit uses raw pixels, not the scaled size). Fix in code. Test: the tree's centre within a few px of the screen centre at 100 % and 150 %. | 45 min | test seen failing, then passing | |
| 6 | **Sabotages** for items 1-5 (e.g. `shop_not_scrolling`, `tree_fit_raw_pixels`), each CAUGHT alone. Also measured and listed, not fixed: 1280x800 at 150 % (the Deck with big UI). | 45 min | each CAUGHT | |
| 7 | **The text draft (read-only).** New `tools/text_audit.py`: every string in `ST_UI`, `ST_Tree`, `ST_Data`. It flags a string over its budget (a label over 3 words; a line over 60 characters; a sentence over 12 words; more than 2 sentences). `--self-test`: a planted long string must be flagged. For each flagged string, a short draft: same meaning, same numbers, same `{action}` tokens, still mean and funny. Output: `build/look/text_draft.md` and a review page (now vs proposed, with character counts). Nothing written to the tables. | 2 h | every flagged string has a draft; self-test seen failing | |
| 8 | **R4's exit review.** Each screen beside its canvas board. Measure first how to get the boards as images (read the canvas, render it locally in headless Chrome). If that fails, use the boards seen in R4 and say so. Pass or a named fault per screen, into the worklist's R4 progress. | 1 h | the list written | |
| 9 | **Gate.** Everyday `check_all` exit 0. Docs: worklist R4 closed (or what is left), 05 §7, 09, 12. KB: anything new about headless layout tests and scroll focus. | 1 h | exit 0 | |
| 10 | **Close.** HANDOFF-LOG (70) with the exact next step. Docs 00 and 11. This list marked closed. | 30 min | written | |

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
