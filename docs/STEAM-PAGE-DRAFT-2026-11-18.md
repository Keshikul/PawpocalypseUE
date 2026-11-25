# The Steam page: the draft (2026-11-18)

> Written the night of 2026-11-17 (designer: "Our focus is steam page and screenshots"). **A draft for sign-off**: nothing
> here is on Steam or in the game. The words are the port's own plain English (D15: "to the point informative
> information", short sentences, real numbers, the VOCAB words: enemies, Reawaken, permanently). Every number below is
> read from `Data/` on 2026-11-17, and each is cited in the table at the end. `build/steam/text_check.txt` runs the game's
> own text rules (`tools/text_audit.py`'s sentence, vocabulary and banned-word checks) on every line.
>
> It replaces the Godot repo's `docs/store/pawpocalypse-steam-page.md` (2026-10-03). That draft was written in a voice
> ("Then it gets out of hand.", "It poops rainbows. They hurt.") before the designer asked for plain text, and three of its
> numbers are out of date: 16 kinds of enemy (now 39 drawings), 41 achievements (now 39), 6 to 10 hours (about 2 h for a
> fast player after D17; about 3 h since D24, 2026-11-24).

## Version 3 (2026-11-18): signed off

The designer, 2026-11-18: the text and the top tags **approved**; length **left off** the page; the AI disclosure **the
designer writes**; languages: "Check all most used steam languages as translate into all of them. Also if we make changes
later make sure that we don't have to retranslate everything and just redo things we changed" (see Store localization).
Two fixes since version 2:
- **Stardust increases damage AND Catnip** in this game. Version 2 changed it to damage alone from Godot's code
  (`combat_resolver.gd`), but D17's E3 (approved 2026-11-12) made the Reawaken multiply Catnip from kills too
  (`PcEconomy.cpp` `ReawakenCatnipMultiplier`, x1.37 a Reawaken). The store describes this game, not Godot's.
- **The length line is gone** (the designer's answer).

## Version 2 (2026-11-18): what changed, and why

Version 2 follows a read of 11 comparable store pages (`build/steam/research/NOTES.md`) and a check of every claim
against `Data/` and Godot's rules.
- **Two claims were wrong in version 1.** "The cat evolves 7 times": the cat has 7 stages and starts at the first, so
  it evolves 6 times (`GameState.get_evolution_stage`: 5 bosses, then the ending). "Each stage has its own weapon":
  stages 3 and 6 share a weapon type (`EvolutionData.csv`, RAINBOW_MINIGUN); each stage changes the weapon instead.
- **"21 languages" is gone from the features.** It contradicted the Languages section (English only until phase 7).
- **The short description names what the cat hunts.** The plainest strong pages name their things ("destroying
  Asteroids, Planets, and Stars"; "Watch the frog cut grass"). "Enemies" alone said nothing. "Mice" stays out (VOCAB).
- **The genre stays in the first sentence.** 6 of the 11 pages open with it and 8 name it in their first sentence.
- **The About headings start with a verb**, as theirs do ("Hit the rock", "Cut grass", "Decorate").
- **Jobs are described as the game has them:** unlocked in the tree, one picked for each run (Godot's
  `lock_run_job`). Version 1 said "after a Reawaken".

## Short description (Steam allows 300 characters; this is 242, under the 280 the translations need)

> Pawpocalypse is an incremental game about a cat. It hunts geese, crocodiles, ghosts and 36 more kinds of enemy.
> Buy upgrades, pick cards and beat 5 bosses. Reawaken to make every run stronger. The cat evolves 6 times and ends
> up on a unicorn.

## About this game

Each block is a heading, one to three short sentences, and a clip under it: the shape of the comparable pages (Gnorp
Apologue, Rusty's Retirement, Ropuka's Idle Island, A Game About Feeding A Black Hole).

`[clip: hunt]`
**Hunt with your paw.**
Move over an enemy and your paw hits it. Every kill pays Catnip.

`[clip: upgrades]`
**Buy 15 upgrades.**
Paw damage, attack speed, crits, the aura and Pounce. Kittens hunt for you.

`[clip: cards]`
**Pick a card at every level.**
28 cards change how a run plays. Some come with a downside.

`[clip: boss]`
**Call 5 bosses.**
Buy Bait to call a boss. Each boss you beat evolves your cat and opens new realms.

`[clip: evolve]`
**Evolve 6 times.**
Tiny Kitten, Armored Tomcat, Cyber Cat, then four unicorns. Each stage changes your weapon.

`[clip: reawaken]`
**Reawaken.**
Reset your Catnip and upgrades for Stardust. Stardust increases damage and Catnip permanently.

`[clip: tree]`
**Grow the skill tree.**
87 skills in a tree that never resets. Unlock 9 jobs and pick one for each run. Each job changes how your cat fights.

**Features**
- 12 realms, from the Backyard to the Litterbox Dimension
- 39 kinds of enemy and 5 bosses
- 15 upgrades, 28 cards and 9 jobs
- 87 skills in a tree that never resets
- 7 stages for your cat, from Tiny Kitten to The Last Unicorn
- 39 achievements
- Keyboard and mouse or a controller, with every control remappable

## Description media (the clips): measured 2026-11-18

From the 11 comparable pages (`build/steam/research/media.tsv`; each file's own header read, not guessed):
- **Steam serves no GIFs any more.** Every animated piece is a webm and an mp4 of the same clip (an uploaded GIF is
  converted). Make clips as video; a GIF is only a source.
- **Widths:** 1170 px (Idle Slayer, A Game About Feeding A Black Hole; Steam scales them to its 780 px column), 780 px
  (Ropuka's Idle Island: the column itself), 616 px (Gnorp Apologue, Nodebuster, Rusty's Retirement: the pre-2025 column),
  400 px (Stacklands).
- **Shapes:** 16:9 gameplay (1170 x 658, 614 x 336) and wide strips (616 x 160 to 616 x 258, 780 x 250, 1170 x 244).
- **Sizes (webm):** 50 KB to 1 MB for clips up to 780 px; 0.7 to 2.7 MB for 1170 px gameplay; one 8 MB outlier.
- **Ours, recommended:** 1170 x 658 gameplay loops, 6 to 12 s (Steam's maximum is 12 s), under 3 MB each, webm and mp4;
  heading strips 1170 x 244 if the headings become pictures.
- **Recorded 2026-11-25 (HANDOFF-LOG (147))** in the real game, frame by frame, by `py tools/store_clips.py` (no screen
  recorder: the game writes each frame, ffmpeg from Python's `imageio-ffmpeg` encodes): six clips as webm + mp4 (1170 x 658,
  9-10.5 s, 1.5-3.0 MB: hunt, aura, pounce, boss, realms, rider), the same six as GIFs (780 px, 6-7.5 s, under 8 MB) and a
  57 s trailer (1920 x 1080, 60 fps, the first try's CC0 score). **The GIFs and clips have no text** (designer, 2026-11-25:
  "Make sure there is no text in gifs"): they are recorded without a single widget, the field drawn alone. Only the trailer
  shows the HUD (the dock, the cards, the tree, the ceremonies). Drive: `Promotional/Steam/2026-11-25`.
- The 9 screenshots are 1920 x 1080 (Steam's minimum is 1280 x 720; all comparable pages use 1920 x 1080); retaken
  2026-11-25 after the restyles of (120)-(126).

## Tags (Steam counts the first 20; the first 5 weigh most)

Incremental, Idler, Clicker, Cats, Casual, 2D, Cute, Colorful, Funny, Singleplayer, Cartoony, Stylized, Indie,
Dark Humor, Top-Down, Resource Management, Replay Value, Arcade, Strategy, Score Attack.

The first two are what every comparable page leads with (the 2026-10-03 read of Nodebuster, Digseum, A Game About Feeding
A Black Hole, Piñata Go Boom, Max Manos, Chop Chains). "Cats" is the one that sets us apart. **Changed from the Godot
draft:** "Cute" moves up and "Colorful" stays. Sticker Box is cute and colourful, and those are the two words players
search for that look.

## Languages

The store must list only what ships ("the store and the build must agree", launch plan K). The port has all 20
translations of Godot's lines, but **the 280 lines changed for the port are untranslated until phase 7**.
- **Recommended:** list English only until phase 7.
- Then list each language once it is reviewed (launch plan decision 6).

## Store localization (the page in 20 languages): drafted 2026-11-18

`build/steam/localization/` holds the short description and the About section in the game's 20 languages, in the shape
Steamworks' Localization tab uploads (`storepage_localization.json`, one entry per language API name), and one readable
file per language for review.
- **Each uses the game's own words** for its terms (Catnip, Stardust, Reawaken, the stages, the realms), taken from the
  game's translations (`build/steam/research/glossary.tsv`), so the page and the game agree.
- **They are drafts:** AI-translated from version 2 above, every short description under 280 characters. Each needs a
  native speaker's read before upload, and they are uploaded only with the language itself (see Languages). A change to
  the English means a new pass.

## Content and the AI disclosure (the Steamworks content survey)

- **Content:** cartoon violence only (a cat hits enemies; they vanish). All screenshots can be marked suitable for all ages.
- **AI disclosure, pre-generated content** (draft, replaces the Godot draft's):
  > The game's art is drawn by code. AI tools helped write that code.
  > AI tools helped make the look, its concept art and the translations.
  > The design and every final decision are the developer's.
- Open: the music (launch plan decision 4).

## System requirements (to measure, not to guess)

Not written tonight: they come from the packaged build's frame gate (`tools/packaged_bench.py`) on the low-end machine,
and packaging is the designer's call. Placeholders for the form: Windows 10 64-bit, DirectX 12, 4 GB RAM, 1 GB disk.
Confirm each with a measured build before the page goes public.

## Where each number comes from

| Claim | Source (2026-11-17; the rows marked v2 checked 2026-11-18) |
|---|---|
| 12 realms | `Data/TierData.csv`, 12 rows |
| 5 bosses | `Data/BossData.csv`: Park Tyrant, Rival Champion, Void God, Foreman, Litter King |
| 39 kinds of enemy | `Data/EnemyData.csv`: 59 spawned ids drawn as 39 different enemies (the realm variants of one enemy share a drawing) |
| 15 upgrades | `Data/UpgradeData.csv`, 15 rows |
| 28 cards | `Data/CardData.csv`, 28 rows |
| 9 jobs | `Data/JobData.csv`: 10 rows, one of them Unemployed |
| 87 skills | `Data/SkillBranchData.csv`, 87 rows |
| 7 stages, 6 evolutions (v2) | `Data/EvolutionData.csv`: 7 rows, Tiny Kitten (the start) to The Last Unicorn. Godot's `get_evolution_stage`: one stage per boss of 5, one more for the ending. Stages 3 and 6 share RAINBOW_MINIGUN, so "each stage changes your weapon", not "its own weapon" |
| geese, crocodiles, ghosts (v2) | `Data/EnemyData.csv`: `enemy_park_goose`, `enemy_sewer_crocodile`, `enemy_portal_ghost`; 36 more = the other 36 of the 39 drawings |
| Stardust increases damage and Catnip (v3) | damage: `PcCombat.cpp` (`ReawakenMultiplier`, Godot's `combat_resolver.gd`); Catnip: D17 E3, `PcEconomy.cpp` `ReawakenCatnipMultiplier` (the damage multiplier to the 0.5, x1.37 a Reawaken) |
| 9 jobs unlocked in the tree, one per run (v2) | `Data/SkillBranchData.csv`: 9 GRANTS_JOB nodes plus Unemployed; Godot `lock_run_job`, "offered once per run" |
| 39 achievements | `Data/AchievementData.csv`, 39 rows |
| remappable controls, a controller | doc 05; `Data/InputMap.csv` (every action has a keyboard/mouse and a pad binding) |

## Open, for the designer

Answered 2026-11-18: the text approved; length off; tags approved; the AI disclosure is the designer's to write;
languages: the most-used Steam languages, all translated, changes retranslated alone. Still open:
1. The store translations: a native speaker's read each before upload.
2. The clips: recorded 2026-11-25 (above). Which clip goes under which About block is the designer's pick (the About
   blocks name hunt, upgrades, cards, boss, evolve, reawaken and tree; the text-free clips cover hunt, boss, the aura,
   Pounce, the realms and the rider; upgrades, cards and the tree exist only with the HUD, in the trailer).
