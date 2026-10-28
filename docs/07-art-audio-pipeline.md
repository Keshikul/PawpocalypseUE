# 07 - Art and audio pipeline

> KB: `Unreal/16` (naming, folders, redirectors, textures), `Unreal/17` (headless import), `Unreal/14` (audio), `2D/01` §4
> (textures for 2D), `Unreal/22` (LFS). Godot: `docs/art-pipeline.md`, `docs/pawpocalypse-sweet-style.md`,
> `docs/pawpocalypse-ui-art-spec.md`, `docs/pawpocalypse-colour-reference.md`, `docs/pawpocalypse-art-audio-doc.md`.

## 1. What exists in Godot and carries over

| Thing | Godot location | Count / size | Port |
|---|---|---|---|
| Art generators (the "sweet" grammar: ellipses, rounded rects, triangles, polylines, uniform ink outline) | `tools/make_sweet_art.py`, `sweet_ui.py`, `sweet_biomes.py`, `sweet_props.py`, `sweet_shapes.py`, `sweet_evolutions.py`, `sweet_input.py`, `make_ground_shadow.py`, `flatten_backdrop.py`, `palette.py` | 377 SVGs (8.7 MB) | **Copied into `tools/art/`** of this repo, unchanged except output paths. The SVGs stay generated: **never hand-edit one** |
| Backdrops | `assets/art/backdrops/backdrop_tier_N.png` (flattened) + biome scenes | 12 | Re-flattened PNGs; floor tiles and props per §4 |
| Boba photos | `content/boba/*.png` -> `tools/boba_prep.py` | menus only | Copied; same prep |
| Audio | `assets/audio/music` (Abstraction loops, OGG), `assets/audio/sfx/kenney` (201 Kenney CC0 OGG), `unicorn_neigh` (synthesised by `make_neigh.py`) | 83 MB in the repo (pruned to what plays at release) | Copied to `SourceAudio/` (LFS), imported per §6 |
| Fonts | `assets/fonts`: Noto Sans CJK Medium `.ttc`, Noto Sans Thai, the Latin face, OFL texts | | §5, doc 05 §7 |
| Credits | `assets/audio/CREDITS.md` | | Copied; in-game credits screen |

`resvg_py` 0.5.0 and Pillow 12.3 are already installed on this machine (the Godot pipeline uses them).

## 2. Rasterising (decided; sizes set by spike S3)

Unreal has no production SVG texture import (doc 04 R11), so:

```
tools/art/*.py --all --write  ->  SourceArt/svg/<type>/<name>.svg      (text, committed, generated)
tools/raster_svg.py            ->  SourceArt/png/<type>/<name>.png      (gitignored, regenerated)
tools/pack_atlas.py            ->  SourceArt/atlas/<page>.png + Data/Atlas.csv (id, page, x, y, w, h, pivot)
tools/import_art.py (headless) ->  /Game/Pawpocalypse/Art/...           (T_*, committed via LFS)
```

- **Raster scale**: the Godot game fits each drawing by its texture's LONGEST EDGE to a target on-screen size
  (`sprite_target_px`, e.g. 64.4 for the Backyard mouse; `SPRITE_TARGET_PX` for the cat). Rasterise each sprite at
  **2x its largest on-screen size at 1080p** (4K-sharp) rounded up to a multiple of 4, not at an arbitrary square. A 64 px
  mouse rasterises at 132 px, not 512 (the Godot project measured 228 MB of VRAM for over-authored sprites vs ~70 MB right-sized).
- **The fit rules stay the generator's**: every drawing fitted to its square at write time with 3 units of margin (strokes
  included); creatures bottom-aligned (feet on the ground line); icons centred; hats and weapons only shrunk. **The cat stages
  share ONE fit transform** (hat and weapon offsets depend on it); `--batch player` prints it.
- **Trim** transparent margins in the packer and record the pivot (feet point) per sprite so trimming never moves a creature.
- **Premultiplied alpha** in the rasteriser output (dark fringes otherwise when scaled) or straight alpha with a material that
  handles it consistently; spike S3 picks one and the material matches.

## 3. Colour tokens

- `tools/art/palette.py` is the single source of every colour. Add an emitter: `py tools/art/palette.py --cpp` writes
  `Source/Pawpocalypse/UI/PawTokens.h` (as Godot's `--gd` writes `palette.gd`). A test compares the header with `palette.py`
  (Godot's `check_mirror` did the same for `palette.gd`).
- Hex values never appear in `.cpp`/`.h` outside the generated header, nor in materials outside a palette parameter
  collection generated from the same file (`MPC_Palette` or a `UPawPalette` data asset).
- `py tools/art/palette.py` keeps checking every creature against every floor it ships on (contrast).

## 4. Textures in Unreal

| Kind | Settings | Why |
|---|---|---|
| Creature / cat / weapon / hat atlas pages | sRGB, compression per spike S3 (BC7 or `UserInterface2D` RGBA), mips ON with padding, filter Bilinear/Trilinear, **Never Stream**, texture group `World` or a custom `Sprites` group, LOD bias 0 | Drawn smaller than rasterised at 1080p; streaming blurs 2D |
| Backdrop floors | Seamless tile textures, mips ON, Never Stream, wrap Repeat | The field grows with the aspect (up to 32:9): the floor must tile, not stretch. A test renders each biome at 16:9, 16:10, 21:9, 32:9 and checks for seams (Godot gated seams too) |
| Props on backdrops | in the atlas, placed by the ported `BiomeBackdrop` layout code as static instances in their own Z band | |
| UI icons, cards, tab icons, currency icons, glyphs | `UserInterface2D`, no mips, Never Stream, sized for 2x | Drawn 1:1 in UI |
| Hardware cursors | PNG under `Content/Slate/Cursors/`, staged | doc 05 §4 |

- **Atlas cells padded and edge-extruded** by at least 2^(mip levels used) px (mip bleeding).
- **Naming**: textures `T_<id>` where `<id>` is the Godot asset name (`T_enemy_backyard_mouse`); atlas pages
  `T_Atlas_<group>_<n>`; materials `M_Sprite_Masked`, `M_Sprite_Glow`, `M_Shadow`, `M_FxShape`, `M_Floor`; instances `MI_*`;
  data assets `DA_Visual_<id>` `[KB U/16 §1]`.
- **Folders**: everything shipped under `/Game/Pawpocalypse/` (always cooked): `Art/Atlas`, `Art/Backdrops`, `Art/UI`,
  `Art/Cursors` (the `Slate` folder sits outside `/Game` content by engine convention), `Materials`, `Data`, `Audio`, `Fonts`,
  `Maps`, `UI` (widget BPs if any).
- **Visual definitions** (`UPawVisualDefinition`, one per creature/boss/cat stage/weapon/hat/job/card/skill icon): atlas id,
  pivot, on-screen target px, tint, glow colour; found by id through the Asset Manager. Rules rows never hold asset paths
  (the CSV's `texture_path`/`icon_path` are mapped to ids by the importer) `[KB U/04 §5]`.
- **Move/rename only in the Content Browser or via the `git mv` + redirect + resave route**; never in Explorer `[KB U/16 §2]`.
- Never rename or reorder shipped asset files between releases without checking the patch size (doc 10 §3).

## 5. Fonts

- Split `NotoSansCJK-Medium.ttc` into its faces (JP, KR, SC, TC) with fontTools; import each as a `UFontFace`; Thai and the
  Latin face likewise. One composite font `F_Paw` with sub-fonts per script range (CJK per culture, Thai) `[KB U/09 §5]`.
- Static faces only (Slate ignores variable fonts). OFL notices shipped in the credits.
- The game's own Latin face must cover Latin Extended and Cyrillic (cs, hu, pl, tr, ru, uk, vi): check glyph coverage with a
  script against every `.po` file's characters; any missing glyph falls to a fallback face, never a box.

## 6. Audio

- **One event table**: Godot `AudioManager.EVENTS` (each row: event, files, `min_interval`, pitch jitter, `volume_db`) plus
  `FALLBACK_MUSIC`/`FALLBACK_SFX` become `Data/AudioEvents.csv` -> `DT_AudioEvents` (event key, sound asset id, bus,
  min interval, pitch range, volume) `[KB U/14 §2]`. It is the only table with asset references.
- `UPawAudioSubsystem::Play(EventKey)` is the only way to play a sound; a check script forbids `UGameplayStatics::PlaySound*`
  elsewhere `[KB U/14 §2]`. Rate limits live in the subsystem (`mouse_killed` ~9/s cap) plus a Sound Concurrency asset.
- **Buses**: Master, Music, SFX, UI via Sound Classes + Sound Mix (or Audio Modulation control buses) driven by the settings
  sliders; "mute all" and "mute when unfocused" options.
- **Music**: per-biome loops with a 1.2 s crossfade (`MUSIC_FADE_SEC`); the Abstraction stems (BASE / MELODY / JANGLES layers)
  can become Audio Modulation-driven stems later; port first as the Godot game plays them.
- **Import**: source files are OGG. Unreal 5.x imports WAV, and (version-dependent) other formats `[Unverified for OGG on
  5.8.3]`: spike S3 tries an OGG import; if it fails, `tools/convert_audio.py` converts to WAV 48 kHz 16/24-bit with ffmpeg once,
  and the WAVs are the LFS source. Set compression quality per class; stream music (> 5 s).
- **Prune**: only files referenced by `DT_AudioEvents` are imported (Godot pruned 130 of 180 unplayed files, 80 MiB). A test
  asserts both directions (every referenced file imported; nothing imported that nothing plays).
- WASAPI is the 5.8 Windows backend: test a headset unplug/replug `[KB U/14 §1]`.
- **Licences** (third-party register, `docs/third-party.md` to create in phase 6): Kenney CC0; Abstraction music (credit
  required; author does not endorse AI-related use; designer's call, kept; never in the trailer); fonts OFL; the trailer music
  (Loyalty Freak Music, CC0) is trailer-only.

## 7. Animation and VFX art

- No frame animation: single drawings with a per-instance pulse and liveliness, done in the sprite material's vertex shader
  (doc 04 §5.1). If run cycles arrive later, `tools/make_anim_atlas.py`/`export_anim_atlas.py` already produce atlases; the
  material reads a frame index from per-instance data.
- VFX are mostly geometry (SDF material) and small Niagara bursts (doc 04 §7); no VFX textures beyond a few soft dots/shards.
- The Unicorn ceremony's whinny (`make_neigh.py`) and the ceremonies' timings port as data on the ceremony widgets.

## 8. Store media

- **Screenshots and the trailer** come from the running game, as in Godot (Movie Maker there; here a dev-only `-Trailer`
  director subsystem + **Movie Render Queue** for deterministic capture, or `HighResShot` for stills). The tutorial is
  suppressed in these runs; the victory screen never appears in the trailer; captions are the designer's words; the score is
  CC0 party disco, never the Abstraction library (designer's rules, 2026-09-30).
- `tools/steam_gifs.py` and `trailer_edit.py` (ffmpeg) port unchanged; they only need new input paths.
