# 07 - Art and audio pipeline

> KB: `Unreal/16` (naming, folders, redirectors, textures), `Unreal/17` (headless import), `Unreal/14` (audio), `2D/01` §4
> (textures for 2D), `Unreal/22` (LFS). Godot: `docs/art-pipeline.md`, `docs/pawpocalypse-sweet-style.md`,
> `docs/pawpocalypse-ui-art-spec.md`, `docs/pawpocalypse-colour-reference.md`, `docs/pawpocalypse-art-audio-doc.md`.

## 1. What exists in Godot and carries over

| Thing | Godot location | Count / size | Port |
|---|---|---|---|
| Art generators (the "sweet" grammar: ellipses, rounded rects, triangles, polylines, uniform ink outline) | `tools/make_sweet_art.py`, `sweet_ui.py`, `sweet_biomes.py`, `sweet_props.py`, `sweet_shapes.py`, `sweet_evolutions.py`, `sweet_input.py`, `make_ground_shadow.py`, `flatten_backdrop.py`, `palette.py` | 377 SVGs (8.7 MB) | **Copied into `tools/art/`** of this repo in phase 6 (output paths changed); this project's own since D14 (2026-11-07), now 554 files with the restyle's own additions. The SVGs stay generated: **never hand-edit one** |
| Backdrops | `assets/art/backdrops/backdrop_tier_N.png` (flattened) + biome scenes | 12 | Re-flattened PNGs; floor tiles and props per §4 |
| Boba photos | `content/boba/*.png` -> `tools/boba_prep.py` | menus only | Copied; same prep |
| Audio | `assets/audio/music` (Abstraction loops, OGG), `assets/audio/sfx/kenney` (201 Kenney CC0 OGG), `unicorn_neigh` (synthesised by `make_neigh.py`) | 83 MB in the repo (pruned to what plays at release) | Copied to `SourceAudio/` (LFS), imported per §6 |
| Fonts | `assets/fonts`: Noto Sans CJK Medium `.ttc`, Noto Sans Thai, the Latin face, OFL texts | | Noto copied; the Latin face replaced by M PLUS Rounded 1c (D14); all in `SourceArt/fonts` with their OFL texts. §5, doc 05 §7 |
| Credits | `assets/audio/CREDITS.md` | | Copied; in-game credits screen |

`resvg_py` 0.5.0 and Pillow 12.3 are already installed on this machine (the Godot pipeline uses them).

## 2. Rasterising (decided; sizes set by spike S3)

Unreal has no production SVG texture import (doc 04 R11), so:

```
tools/art/*.py --all --write  ->  SourceArt/svg/<type>/<name>.svg      (text, committed, generated)
tools/raster_svg.py            ->  SourceArt/png/<type>/<name>.png      (gitignored, regenerated)
tools/pack_atlas.py            ->  SourceArt/png/atlas/field_<n>.png + field.csv (id, page, texel rect, UVs, size)
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

**Built** (phase 5 slice 3b, 2026-11-04; phase 6 slice 1, 2026-11-05):
- **`tools/art/`**: the Godot generators (`make_sweet_art.py`, `sweet_ui.py`, `sweet_biomes.py`, `sweet_props.py`,
  `sweet_shapes.py`, `sweet_evolutions.py`, `sweet_input.py`, `make_ground_shadow.py`, `palette.py`), copied unchanged but
  for their output paths (each has a PORT note). `flatten_backdrop.py` was not copied: it reads a removed "famine" style folder
  and generates nothing the game ships. Four UI drawings were hand-made in Godot (`app_icon`, `ui_menu_bug` / `discord` /
  `language`): their sources are `tools/art/hand/ui`.
- **`py tools/art/build_art.py`** writes `SourceArt/svg` (committed, generated), mirroring Godot's `assets/art` folders: 390
  files in phase 6 (the SVGs, the 12 backdrop floors, the ground shadow), 554 today (R5 added the rig layers in `rigs/`).
  `--check` regenerates into `build/` and fails on any byte that differs from `SourceArt/svg`. Until D14 it also compared
  with the Godot game's `assets/art` (byte-identical, 2026-11-05); since then the look is this project's own and only the
  committed set counts. Seen to fail: one ink hex changed, 738 differences. Gate `art generators reproduce`.
- `tools/raster_svg.py` (groups `input`, 56 px; `cursor`, 40 px; since phase 6 slice 5a `chrome`, the theme's 9 nine-patch
  SVGs, and since 5b `icons`, 178 SVGs: both at the SVG's own size, as Godot imports them at `svg/scale=1.0`): reads
  `SourceArt/svg` (no longer copies from Godot); `--check` covers every group.
- **`py tools/pack_atlas.py`**: the field's sprites (in phase 6: 107; 39 enemy drawings, 7 bosses incl. `boss_generic` and `boss_add`, the
  7 cat stages, 6 weapons, 10 hats, the kitten, the ground shadow; since slice 2 the 36 backdrop props: corner pieces at
  190 px, the title's tree at 264.5, the rest at 124.8) rasterised at 2x their largest drawn size and shelf-packed
  into `SourceArt/png/atlas/field_<n>.png` (then 3 pages of 2048, 63 % full; today 249 sprites with the rig's layers, 4 pages,
  75 %; 4 px extrusion) + `field.csv` (id, page, texel
  rect, UVs, native size). Not trimmed: every drawing is a square fitted by its longest edge and drawn centred, as Godot's
  Sprite2D draws it, so the cell IS the sprite. `--check` reproduces it (gate `field atlas reproduces`).
- **The rig (R5, 2026-11-09).** A drawing in `tools/art/sweet_anatomy.py` marks its moving parts with `limb(name, pivot,
  parts, motion, amp, hz, phase, back)`; `make_sweet_art` writes the static drawing as the rest pose (back layers, body,
  front layers) and each layer as `SourceArt/svg/rigs/<stem>__<layer>.svg` (the whole 128 box, so the layers lie on each
  other; the rig in the root's `data-` attributes). `pack_atlas.py` packs each layer at its drawing's size and adds the
  rig to `field.csv` (`rig_of, rig_order, pivot_u, pivot_v, motion, amp, hz, phase`); `import_art.py --run atlas` copies
  it into `FPawAtlasCell`; `UPawSpriteAtlas::Rig(cell)` lists a drawing's layers; the Realm view draws them, each turned
  about its pivot (`PawLook::RigAngleDeg`, `RigPose`; swing = amp sin, lift = amp (1 - cos) / 2, the creature's own phase
  added; layers 0.12 of a draw place apart in depth: closer, an overlapped layer drew black, `UPawSpriteRenderer::BodyZ`).
  With every batch's layers the atlas is 249 sprites on 4 pages (75 %, 2026-11-10). `py tools/art/review_rig.py [--batch]`
  draws the motion (the designer's review of the motion is still open).
- `tools/import_art.py --run [input|chrome|icons|atlas|floors]` (headless): the glyphs, the chrome (`Art/UI/Chrome/T_ui_*`)
  and the icons (`Art/UI/Icons/T_*` + `DA_Icons`, `UPawIconSet`, rewritten whole) as UI textures (no mips); the atlas as `T_Atlas_Field_<n>` (BC7,
  sRGB, mips, Never Stream, World group, clamped) and `DA_FieldAtlas` (`UPawSpriteAtlas`, rewritten whole) in
  `/Game/Pawpocalypse/Art/Field`. `Pawpocalypse.Realm.Look.Atlas` checks the asset against `field.csv` (a repack without a
  re-import fails it) and that every texture the data names is in it.
- The floors (slice 2): `py tools/import_art.py --run floors` imports the generator's own 12 PNGs from
  `SourceArt/svg/backdrops` as `T_backdrop_tier_N` (BC7, mips, WRAP, Never Stream, World) in `/Game/Pawpocalypse/Art/Backdrops`.
  `build_art.py --check` also runs the generator's seam gate (`sweet_biomes.py --seams`: worst join 1.33, limit 2.0).
- The UI art (phase 6 slices 5a-5b): Godot's theme nine-patches (`PawUi::Chrome`: the brush's margins are Godot's
  `texture_margin` over the SVG's size) and every icon Godot asks for by constructed name or `icon_path` (`PawUi::Icon`, =
  Godot's `Art.texture`: tabs, currencies, quick bar, the title's plates, achievements, cards, skills, jobs). Tests
  `Pawpocalypse.UI.Chrome`, `.Icons` (doc 05 §7). The rainbow bar and frame and the tree's sky are painted, not art
  (slice 5d, doc 05 §7). Still to come: Boba's photos.

### 2.1 Changing a design (what the designer does)

Shapes are not edited in Unreal (no SVG editing or production SVG import, doc 04 R11), and they were not edited in Godot
either: Godot only imported the generated SVGs. The source of a design is its generator in `tools/art/` (the sweet grammar)
plus `palette.py` for its colours. A change is: edit the generator -> `--batch <family>` contact sheet -> `--write` -> one
command each regenerates, packs and reimports (`py tools/art/build_art.py`, `py tools/pack_atlas.py`,
`py tools/import_art.py --run atlas`, editor closed), and the textures update in the editor. **What is
editable inside Unreal** (planned): everything in a creature's visual definition (`UPawVisualDefinition`, doc 03 §6): which
texture, on-screen size, tint/flash colours, sounds; gameplay numbers stay in `Data/*.csv`. **Not built in phase 6 slices 1-4**:
the look reads the data's columns and Godot's code constants (`Realm/PawFieldLook.h`); changes are made in the Godot project
and re-exported (CLAUDE.md: no design changes during the port). **Since 2026-11-07 (D14) the generators
are this project's own**: a look change is made here only, never in Godot; `build_art.py --check` compares with the committed
`SourceArt/svg`. (Before that, a generator change was made in Godot first and copied.)

### 2.2 Decided by spike S3 (2026-10-28, doc 04 §10)

Raster at **2x** the largest on-screen size at 1080p (target x 1.03 pulse; the cat x 1.08 hop more), rounded up to 4, with
`resvg_py`; field sprites compressed **BC7** (no measurable loss on this flat art, a quarter of the memory); atlas pages
**2048 x 2048**, cells edge-extruded **4 px**. The 52 field sprites take 2 pages, 11.2 MB of VRAM. The first versions of
`tools/raster_svg.py` / `tools/pack_atlas.py` are `tools/spikes/s3_raster_pack.py` (shelf packer; trimming and pivots still
to add; deleted with the spikes 2026-11-04: `git show 10c118e:tools/spikes/s3_raster_pack.py`).

## 3. Colour tokens

- **As built (2026-11-07, R0)**: `tools/gen_paw_tokens.py` imports `tools/art/palette.py` and writes `PawTokens.h`,
  `PcBackdropTones.h` and the test-only `Source/PawCore/Tests/PcGodotLook.h` (L0 checks all three are current; it refuses a
  palette that fails `palette.py`'s rules). The colours Godot authored as scene/script literals live in `palette.py LITERALS`.
  `PcGodotLook.h` maps Godot's palette (frozen at the fork: `tools/art/palette_godot_snapshot.json`) to ours, so a test that
  compares a Godot fixture's colour still proves WHICH colour is drawn where (`Pc::PcFromGodot`); the generator refuses an
  ambiguous map (one Godot colour that became two of ours). Before R0 it read the Godot repo's `palette.gd`.
- **The field effects' look (R5, D3, 2026-11-09)**: `tools/art/fx_look.py` holds the approved values (the aura's white lift
  and Bone ring, the strike paw's size, arc trail and fade, the crit colour, from `palette.py`); `tools/gen_fx_look.py` writes
  `Source/Pawpocalypse/Realm/PawFxLook.h` with them and the strike paw's shapes, read from `ui_cursor_paw_closed`'s drawing
  in `make_sweet_art.py` (ellipses tessellated; drawn by `FPawFxCanvas` as polygons and ink outlines, in the swipe's band, so
  the paw sits on top of its arc and claws: a translucent atlas sprite would draw under the band). Redraw the cursor in R6,
  re-run the generator, and the strike paw follows. L0 checks the header is current; `Pawpocalypse.Realm.Vfx.Look` checks
  what is drawn.
- **The look lint (R5, 2026-11-09)**: `tools/art/look_lint.py` checks the field's drawings against the look spec (WORKLIST-
  RESTYLE §4, §6 step 2): every colour a token or a token's grammar shade (`sweet_shapes.tint()` is recorded while every
  generator runs in-process, so a derived colour counts only if its base is allowed), one ink at one width, flat, each body clear
  of every floor it ships on, eyes that read at 56 px; since R5 also `unique` (no two characters' bodies within `UNIQUE_MIN`),
  `rig` (layers sharing a depth never overlap over a 2.5 s loop) and `crossing` (no polygon outline crosses itself: the tube
  caps' bow-tie, HANDOFF-LOG (87)). Its KNOWN table held the faults the art had when it was written (94) and only shrinks:
  23 left (2026-11-10), all the cat's pale coats on the light and dark floors, a look decision for the designer
  (`palette.PROPS` and `palette.GEAR` named the props' and the cat's gear colours, which removed the literals). The faces (designer, 2026-11-09): the cat keeps its face; every
  enemy draws `sweet_shapes.PREY_FACE = "panic"` (worried brows, tiny pupils, no shine); bosses and the unicorn mount pass
  `face="today"`. **One colour per character** (designer, 2026-11-09: "no same shade is used twice"): `palette.py` ENEMIES / BOSSES / PLAYER
  give every enemy kind, boss and the player its own body (CIE76 >= `UNIQUE_MIN` 16 from every other, the cat's belly and
  stripes included) and accent (>= `ACCENT_MIN` 10), inside a window that keeps each one itself and clear of its floors;
  rule 8 checks them, `look_lint`'s `unique` rule checks what is drawn. The anatomy pass (R5 batch 2, approved and written
  2026-11-09: mouse and its flier and golden copy, rat and the slime rat, the 12 gristles, moth, vole, hamster): `tools/art/sweet_anatomy.py` draws enemies from parts with two
  new grammar primitives, `sweet_shapes.tube` (a tapered limb, tail, snout or neck) and `oval` (a tilted ellipse), both
  polygons that `fuse()` grows by exactly the line width; `tools/art/review_anatomy.py` puts each beside today's at game size
  and lints the new drawings before anything is written. Written since (each approved on the canvas): batch 3, the birds;
  batch 4, dog, hound, ant, cockroach; batch 5, toad, crocodile, bat, dust bunny, robot mouse and the sludge cat (take A,
  "rising out of its puddle"; the ghost kept today's drawing). Drawn and WAITING for the designer: the imp (its wing
  rebuilt) and the bosses (`sweet_anatomy` `tyrant`, `foreman`, `litter_king`, `void_god`: nothing ships until approved).
  **The props** (R5, approved 2026-11-10): the 36 drawn in `sweet_biomes.PROP_DRAW` with more shape (a silhouette that says
  what each is, one level of detail, one light), same stems, ideas and colours; nothing in a prop may read as a face.
- `tools/art/palette.py` is the single source of every colour. Add an emitter: `py tools/art/palette.py --cpp` writes
  `Source/Pawpocalypse/UI/PawTokens.h` (as Godot's `--gd` writes `palette.gd`). A test compares the header with `palette.py`
  (Godot's `check_mirror` did the same for `palette.gd`).
- Hex values never appear in `.cpp`/`.h` outside the generated header, nor in materials outside a palette parameter
  collection generated from the same file (`MPC_Palette` or a `UPawPalette` data asset).
- `py tools/art/palette.py` keeps checking every creature against every floor it ships on (contrast).

## 4. Textures in Unreal

| Kind | Settings | Why |
|---|---|---|
| Creature / cat / weapon / hat atlas pages | sRGB, **BC7** (spike S3), mips ON with padding, filter Bilinear/Trilinear, **Never Stream**, texture group `World` or a custom `Sprites` group, LOD bias 0 | Drawn smaller than rasterised at 1080p; streaming blurs 2D |
| Backdrop floors | Seamless tile textures, mips ON, Never Stream, wrap Repeat | The field grows with the aspect (up to 32:9): the floor must tile, not stretch. A test renders each biome at 16:9, 16:10, 21:9, 32:9 and checks for seams (Godot gated seams too) |
| Props on backdrops | in the atlas, placed by the ported `BiomeBackdrop` layout code as static instances in their own Z band | |
| UI icons, cards, tab icons, currency icons, glyphs | `UserInterface2D`, no mips, Never Stream, sized for 2x | Drawn 1:1 in UI |
| Hardware cursors | PNG under `Content/Slate/Cursors/`, staged | doc 05 §4 |

- **Atlas cells padded and edge-extruded** by at least 2^(mip levels used) px (mip bleeding).
- **Naming**: textures `T_<id>` where `<id>` is the Godot asset name (`T_enemy_backyard_mouse`); atlas pages
  `T_Atlas_<group>_<n>` (as built: `T_Atlas_Field_<n>`); materials as built (phase 6, all generated by
  `tools/create_field_materials.py`): `M_Sprite` (masked atlas sprites), `M_SpriteSoft` (translucent: the shielded, glows,
  shadows), `M_Floor`, `M_BackdropFill` (sRGB vertex colours: the backdrop's patches and the effects), plus the shape
  materials `M_FieldFlat`/`Soft`/`Disc`/`Ring` (the flat floor fallback, zones, the old rings); per-page instances are
  dynamic (`UMaterialInstanceDynamic`); data assets `DA_<Name>` (`DA_FieldAtlas`, `DA_AudioBank`, `DA_Icons`) `[KB U/16 §1]`.
- **Folders**: everything shipped under `/Game/Pawpocalypse/` (always cooked). As built: `Art/Field` (the atlas),
  `Art/Backdrops` (the floors), `Art/UI/Input` (the glyphs), `Art/UI/Chrome` (the nine-patches), `Art/UI/Icons` (+
  `DA_Icons`), `UI/Cursors` (loose PNGs, staged), `Materials`, `Data`,
  `Audio/Sfx`, `Audio/Music` (+ `DA_AudioBank`), `Maps`, `UI/Widgets` (the Widget Blueprints), `Input`, `Localization`.
- **Visual definitions** (planned, not built: see §2.1) (`UPawVisualDefinition`, one per creature/boss/cat stage/weapon/hat/job/card/skill icon): atlas id,
  pivot, on-screen target px, tint, glow colour; found by id through the Asset Manager. Rules rows never hold asset paths
  (the CSV's `texture_path`/`icon_path` are mapped to ids by the importer) `[KB U/04 §5]`.
- **Move/rename only in the Content Browser or via the `git mv` + redirect + resave route**; never in Explorer `[KB U/16 §2]`.
- Never rename or reorder shipped asset files between releases without checking the patch size (doc 10 §3).

## 5. Fonts

**As built (restyle R2, 2026-11-07; the restyle plan's D1: M PLUS Rounded 1c).** `tools/fonts.py` is the one table: the faces in
`SourceArt/fonts` (M PLUS Rounded 1c ExtraBold and Black from Google Fonts; Noto Sans CJK Medium `.ttc` and Noto Sans Thai
Medium copied from the Godot repo; their OFL texts beside them, for the credits), the typefaces (`Regular` and `Bold` = the
text weight, ExtraBold; `Black` = titles and numbers), the sub-fonts and the fallback. It generates `UI/PawFontSpec.h`
(L0 checks it). `py tools/import_fonts.py --run` makes the `FF_*` faces and the composite `F_Paw` under
`/Game/Pawpocalypse/Fonts` through `UPawFontBuilder` (PawpocalypseEditor; the import factory crashes headless, KB U/09 §5.1).
The `.ttc` is ONE face asset, its faces picked by index (JP 0, KR 1, SC 2, TC 3). Sub-fonts: `ja` keeps Han and kana on
M PLUS; `ko`, `zh-Hans`, `zh-Hant` get their own Noto face for Han (and Hangul for ko); every other language gets Hangul from
Noto KR, Han from Noto SC (Godot's rule: a language's own name in the picker) and Thai from Noto Thai; the fallback is SC.
`[SlateStyle] DefaultFontName` makes F_Paw every UMG text's default; the baked assets follow (their fonts equalled the
default; `bake_widgets.py --refont` rewrites any saved engine font). Titles and numbers: `PawUi::Heavy` (Black; a Deep
Plum shadow on outlined text), grafted into the assets with `restyle:Part`. Gates: `tools/check_font_coverage.py` (every
character of every language through Slate's own face choice; L0 everyday, its self-test in `-Full`), tests
`Pawpocalypse.UI.Fonts.Composite`, `.Widgets.Fonts`, the parity test's text looks. The look in the game:
`py tools/godot_tour.py --set fonts` (each culture's sheet, `paw.FontSheet`).

## 6. Audio

**As built (phase 6 slice 4, 2026-11-05).** `py tools/export_audio.py` reads audio_manager.gd (EVENTS, the sounds each event
plays as `_load_all_variants` decides, FALLBACK_MUSIC) into `Data/AudioEvents.csv` / `Data/AudioMusic.csv` (also written by
`export_godot_data.py`, so the data-export gate covers them) and copies ONLY the 53 files something plays into `SourceAudio/`
(28.9 MB of Godot's 83); `--check` (gate `audio export reproduces`). **UE 5.8 imports OGG Vorbis directly** (SoundFactory:
wav, aif, ogg, flac, opus, mp3): no conversion. `py tools/import_audio.py --run` -> `SW_*` in `/Game/Pawpocalypse/Audio/{Sfx,Music}`
(music looping) and `DA_AudioBank` (`UPawAudioBank`). `UPawAudioSubsystem` (`Source/Pawpocalypse/Audio`) is AudioManager:
16 voices in turn, each event's interval / pitch spread / dB and round robin, the buses as linear amplitudes read live from
the settings (Master x Music / SFX; mute), the 1.2 s crossfade in dB, a track with no file keeps the music on (biomes 9-12),
real-time clocks. What plays when follows Godot's hookups: a progress observer (a new game or a load bumps
`UPawGameSubsystem::ProgressEpoch` and is heard as nothing), the field's events from the realm, the biome's music on a biome
change, the UI's clicks (dock tabs, the buy multiplier, a refused purchase, the pause menu, the title's cluster), the title's
music. Each sound plays once (F40, fixed 2026-11-11, D16: Godot plays five of them twice, layered, from a signal hookup and
a direct call; `Pawpocalypse.Audio.When`). `unicorn_neigh` plays since slice 6a (the Unicorn ceremony's two whinnies, with
`boss_defeated`'s fanfare, through `UPawAudioSubsystem::PlayFrom`). Not in this slice: `boba_cameo` (its caller, slice 6c), the
language list's click (phase 7), the credits screen's rows (the CREDITS.md is still empty in Godot), `make_neigh.py` (the
neigh is Godot's WAV, copied). **The Output device** (2026-11-05; settings_manager.gd `_apply_output_device`): the
Output list is the system's active devices by friendly name, "Default" first (Godot's `get_output_device_list()`, read from
the WASAPI mixer's device cache: `IAudioPlatformDeviceInfoCache::GetAllActiveOutputDevices`, read-locked, game thread);
`UPawAudioSubsystem::ApplyOutputDevice` plays to the stored device while it is listed, else the system's
(`Pc::SettingsOutputWant`), on startup, every settings change and when the pinned device is removed
(`UAudioDeviceNotificationSubsystem::DeviceRemovedNative`). A chosen device is PINNED: the swap
(`RequestDeviceSwap`, forced, on the audio thread) goes with `SetIsListeningForDeviceEvents(false)`, because the mixer
otherwise keeps following the system's default after a swap (a pad's headset would take the sound back); "Default" turns it
on again. Never in the editor. The pad-audio toast: doc 05 §3. Tests `Pawpocalypse.Audio.Output`, `.UI.Settings.Output`. The plan below (a DataTable, Sound Classes, a concurrency asset) was not needed.

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
- **Licences** (third-party register, `docs/third-party.md`: not created yet, due before release, phase 8): Kenney CC0; Abstraction music (credit
  required; author does not endorse AI-related use; designer's call, kept; never in the trailer); fonts OFL (the texts in `SourceArt/fonts`); the trailer music
  (Loyalty Freak Music, CC0) is trailer-only.

## 7. Animation and VFX art

- No frame animation: single drawings with a per-instance pulse and liveliness (as built on the CPU, in each sprite's
  transform: doc 04 §5.1), and since R5 the rig: moving parts as their own layers turned about a pivot (§2). If run cycles
  arrive later, `tools/make_anim_atlas.py`/`export_anim_atlas.py` already produce atlases; the
  material reads a frame index from per-instance data.
- VFX: planned as SDF geometry and small Niagara bursts; **built (phase 6 slice 3) as Godot's own draw calls** tessellated on the
  CPU (`FPawFxCanvas`, `FPawVfx`; doc 04 §7 as built): no VFX textures at all.
- The Unicorn ceremony's whinny (`make_neigh.py`) and the ceremonies' timings port as data on the ceremony widgets. As built
  (phase 6 slice 6a): the whinny is Godot's WAV, copied; the Unicorn ceremony's timings are code constants in Godot, so they
  are PawCore constants (`Pc::Unicorn` in `PcUnicorn.h`), proved against the real script by the fixture `unicorn`.

## 8. Store media

- **Screenshots and the trailer** come from the running game, as in Godot (Movie Maker there; here a dev-only `-Trailer`
  director subsystem + **Movie Render Queue** for deterministic capture, or `HighResShot` for stills). The tutorial is
  suppressed in these runs; the victory screen never appears in the trailer; captions are the designer's words; the score is
  CC0 party disco, never the Abstraction library (designer's rules, 2026-09-30).
- `tools/steam_gifs.py` and `trailer_edit.py` (ffmpeg) port unchanged; they only need new input paths.
