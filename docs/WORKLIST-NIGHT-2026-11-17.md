# Night worklist, 2026-11-17 (unattended, from the designer's go to about 08:30)

> Written 2026-11-17 22:15 from HANDOFF-LOG (108) (designer: "Make an overnight plan"). **Not started: waiting for the go.**
> The closing entry will be HANDOFF-LOG (109).

The night does two things at once. **The nightly `-Full` proves the tree as it stands at the go**: D17 has never had a whole
nightly (the last one was cut by the power failure), and since then the `if` audit's fixes 1-4, the waiting engine guards and
the colour inventory have gone in, all uncommitted. That is about 10 h, 42 gates, every PawCore and realm sabotage (667 and
564). **While it runs, the desk work prepares the Sticker Box restyle off the tree**, so the morning starts with answers, not
questions: S1's palette and recolour rules checked against the real inventory, the fonts measured in all 21 languages for
E1, and the first batch of new creatures drawn by the generators for review. **Nothing the player sees changes tonight**:
no asset, texture or `Source/` file is touched by the desk work.

## Before the go (the designer)

1. **Leave the Unreal editor closed until the closing entry says it may be opened.** A sabotage run builds broken DLLs on
   purpose until its clean rebuild; an editor opened in between loads them. (It is closed now.)
2. **Leave the PC on, signed in and unlocked, with this session open.** The night holds a keep-awake request from a background
   process (`build/night3/keep_awake.ps1`, the 2026-11-10 script; no power setting changes; it ends with the night).
3. **Commit (104)-(108) first, or not.** The tree holds five entries of uncommitted work (35 changed files).
   - **Not tonight (recommended):** the night proves exactly the tree as it is and saves a copy of it first
     (`build/night3/before/`); you commit in the morning, knowing it passed.
   - **Commit before the go:** say "commit" (and "push" if you want it on GitHub); the night then starts from that commit.
4. **May the night download two fonts?** Fredoka and Nunito, OFL, from Google's own font repository
   (`github.com/google/fonts`), into `build/night3/fonts/` only, to measure them for E1. **Recommended: yes.** With a no,
   item 2b is skipped and E1 waits.

## Ground rules (CLAUDE.md)

- No commit, push, package or export tonight (unless step 3 above said so, before the go). The Godot repo is read only.
- **The tree at the go is the night's starting point and is saved** (`build/night3/before/`: `git status`, `git diff`, and a
  copy of every changed and untracked file). Never `git checkout`, `restore`, `stash` or `reset`: anything undone is written
  back from that copy.
- **One test run at a time.** While the nightly runs, the desk work writes `build/night3/` and `docs/` only: never `tools/`,
  `Source/`, `Data/`, `Content/` or `SourceArt/` (a sabotage run edits the source tree in place and rebuilds). It works on
  copies of the tools under `build/night3/`, run with `-I` from there.
- No Unreal process of the desk work's: no headless editor, no import, no bake, no build. Edge headless (to render the
  concepts) and Python are fine.
- An Unreal process of this project appears (`tools/unreal_procs.py` exit 1): the nightly's own guard waits and refuses; the
  night writes the state and stops nothing else. Another project's UnrealBuildTool: the nightly waits for it.
- A desk item whose cause of trouble is not clear within the hour is written up and left; the night moves on. A nightly gate
  that fails keeps its log; its cause is looked for in the logs and the code (no second run during the nightly) and written
  up for the morning. Nothing is "fixed" by weakening a test.

## The night

| # | Work | Est. | Done when | Status |
|---|---|---|---|---|
| 0 | **Preflight**: the everyday `check_all` on (108) PASS (running since 22:00, `build/check_all_20261117_colours.log`); `tools/unreal_procs.py` exit 0; other Unreal processes listed; the keep-awake on; both repos' `git status` saved; the tree copied to `build/night3/before/` | 10 min | all true | |
| 1 | **The nightly**: `check_all.ps1 -Full` in the background (`build/check_all_full_20261117.log`); the session wakes when it ends | ~10 h | 42 of 42 PASS, exit 0 | |
| 2a | **S1 off the tree** (desk, during 1). Copies of `palette.py`, `gen_paw_tokens.py` in `build/night3/s1/`: the Sticker UI table (the worklist's values), the text and painted key sets, rule 7 for Sticker's pairs; the recolour RULES (role, property path suffix, old key, new key) and the role maps; **the pre-flight against `build/widget_colours.tsv`**: every one of the 7,549 saved colours that is a changing palette colour placed by exactly one rule or map. `UPawWidgetBaker::RecolourByRule` and the test changes written as a patch (`build/night3/s1/s1.patch`), not applied | 2-3 h | rule 7 green; the pre-flight places every changing colour (or lists each it cannot, with why); the patch reviewed against the code it changes | |
| 2b | **E1, the fonts measured** (desk, only with the go's yes on fonts). Fredoka (variable, instanced to 700) and Nunito (700, 800) into `build/night3/fonts/`; their coverage of every language's characters with `check_font_coverage.py`'s own character sets; the proposed composite (Nunito text, Fredoka titles where it has the letters, Nunito's heaviest where not, M PLUS Rounded for Japanese, Noto CJK and Thai as today) run through a scratch copy of the gate; a sheet per language (PIL: a title, a number, a button, a sentence from `ST_UI`'s translations where they exist) | 1.5-2 h | the coverage table (each language: which face draws titles and text, any box); 21 sheets in `build/night3/fonts/sheets/`; the gate's scratch run green and seen to fail with a face removed | |
| 2c | **S5 batch 1 drawn by the generators** (desk). The 12 commons of the Bestiary (mouse, vole, rat, pigeon, sewer rat, slime rat, ghost, imp, ant, hamster, factory rat, dust bunny) in the generator grammar, in a scratch copy of `tools/art` under `build/night3/art/`: the ink, the two fills, the faces, the white rim made as E2 recommends (the alpha dilated when rastered). A review sheet like `review_anatomy.py`'s: each new drawing beside the Bestiary's (the artifact rendered headless) at 64 px, 36 px and 2x, on its floor | 3-4 h | `build/night3/art/review_batch1.png`; each drawing's notes (where it differs from the Bestiary and why); `look_lint.py` run on the scratch output with its plum rules, the failures listed as what S5's new lint must allow | |
| 3 | **Close**: HANDOFF-LOG (109) (each gate's result and time; 2a-2c's results and files; whether **the editor may be opened**; the next step); doc 00 (the nightly); doc 09's last `-Full`; the worklist's S1 and E1 updated with what was measured; this list closed; the keep-awake stopped | 20 min | written | |

**Order and cut lines.** 0, then 1 at once; 2a, 2b, 2c in that order during 1 (2a first: it is the morning's next step);
3 when 1 ends. A desk item not done when 1 ends is written up as far as it got, and 3 still closes the night: the closing
entry is never held for desk work. If 0's everyday run fails, the night does not start 1: the failure is written up and the
desk work runs alone.

## What the morning gets

- The tree proven by a whole nightly (or every failure written up with its cause), ready to commit.
- S1 ready to apply: the palette, the rules and the pre-flight already checked against the real assets, the C++ as a patch.
  Applying it, migrating and the everyday `check_all` is then about 2 h with the editor closed.
- E1 answered with evidence: what each language looks like in Fredoka and Nunito, and what falls back.
- A first look at the new creatures as the game will draw them, beside the approved Bestiary, for "keep going" or "change X".
