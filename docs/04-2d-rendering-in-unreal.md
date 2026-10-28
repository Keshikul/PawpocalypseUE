# 04 - 2D rendering in Unreal: research, traps, the plan and its spikes

> Unreal is a 3D engine with a 2D plugin bolted on. Everything in this doc exists because a 2D game in Unreal fights
> defaults that are right for 3D: physically based exposure, a filmic tonemapper, temporal anti-aliasing, distance
> sorting of translucency, texture streaming, one Actor per thing. The research is in §2 with its sources; the
> decisions in §3-§7; the spikes that must run before phase 1 in §8. Technical findings are also in the knowledge
> base: `C:\Games\Knowledge\2D\02-unreal-2d.md` (Unreal) and `C:\Games\Knowledge\2D\01-2d-optimization-best-practices.md`
> (engine-agnostic 2D optimisation). **No Lumen, no ray tracing** (designer, 2026-10-28): the game uses no lighting at all.

## 1. What the Godot game draws (the inventory to reproduce)

| Layer (back to front) | Godot | Count at peak |
|---|---|---|
| Biome backdrop | a scene per biome: floor (generated tiles, `sweet_biomes.py`), props (`sweet_props.py`), `BiomeBackdrop` (738 lines) | 1 per biome, static |
| Ground zones | Rainbow Poop, Pounce crater, aura trail, echo zones: drawn discs/rings | up to `MAX_DAMAGE_ZONES` 64 |
| Ground shadows | `GroundShadow`, ONE z layer under every creature (light, fainter under fliers) | ~350 |
| Aura ring | `AuraRing` around the cat | 1 |
| Creatures | enemies (59 kinds), kittens, boss, boss adds: single SVG drawings with a pulse, per-instance liveliness, tint, white flash, signature glow, golden sparkle | up to 320 enemies + kittens + boss |
| The cat | stage sprite + weapon overlay (mirrored) + job hat (z above weapon) | 1 |
| VFX primitives | `VfxSwipe` (ink-edged arc + three claw slashes), `VfxSlam`, `VfxBurst`, `VfxBeam` (Prism, overkill splash), `VfxShards`, `VfxEdgeFire`, boss telegraph, rainbow frame | dozens, short-lived; every one honours the graphics setting |
| UI | dock, status row, Pounce clock, XP bar, boss banner and bar, tutorial paw | always |

Logical resolution 1920x1080, stretch `canvas_items`, aspect `expand` (the logical viewport GROWS on wider screens; the
field fills it). GL Compatibility renderer, no post-processing. Measured on 0.2.4: frame p50/p95/p99 **2.38 / 4.17 /
4.55 ms** under a 20x soak. The port must not be slower on the same machine, and must hold 60 fps at 1280x800 on a
Steam Deck.

## 2. Research: 2D in Unreal, what people hit

| # | Finding | Source quality |
|---|---|---|
| R1 | **Paper2D** is Epic's sprite system (sprites, flipbooks, tile maps) and still ships enabled by default in UE5, but the community reports almost no architectural development since ~2017 and open bugs remain (an atlas-group hang/crash reproduced in 5.3, 5.6 and UE5-main, filed Dec 2025). Epic has made no deprecation statement | Epic docs + bug tracker; blog opinion |
| R2 | **PaperZD** (free, Fab/GitHub) is the community's animation layer over Paper2D (anim graphs, notifies, directional sprites). We have no frame animation (single drawings with a pulse), so it is **not needed** | Fab listing |
| R3 | **One Actor per sprite does not scale**: a forum report of 300 moving flipbook actors at ~20 fps; "each component = one or more draw calls" (Paper2D thread); separate components are not batched | Forum (UE4-era, directionally right) |
| R4 | **Instancing** is the way to draw many: instanced static meshes with **per-instance custom data** (floats read by the material: atlas frame, tint, flash) let one draw call draw a whole type. Custom-data float count is set once (changing it reallocates and zeroes). A bullet-hell toolkit claims 50k instanced bullets at 60 fps (vendor claim) | Epic API docs; Fab vendor |
| R5 | **`UPaperGroupedSpriteComponent`** renders and culls many sprite instances "as a single unit", but nothing documents its draw-call behaviour or its per-instance colour/frame options well enough to rely on; Paper2D's sprite atlas is the batching mechanism behind it | Epic API docs |
| R6 | **Translucency sorts per OBJECT (per component), not per instance or per pixel.** Inside one instanced component, translucent instances do not sort against each other. Between components, sort is by distance to camera unless the project's **Translucent Sort Policy = Sort Along Axis** (Epic: "best for 2D"); a 4.22 bug where the axis had no effect on Paper2D was fixed in 4.23. Translucent sprites draw BEHIND masked ones regardless of Y order (forum) | Epic API + forum + bug tracker |
| R7 | **Masked (alpha-tested) sprites depth-sort correctly per pixel** and work inside instancing, but their edges are hard: an anti-aliased SVG edge becomes a jagged 1-bit edge. Fixes: temporal AA (blurs and ghosts moving sprites), or **forward shading + MSAA + alpha-to-coverage** (supported since 4.15; masks need a steeper opacity gradient or too much is cut away) | Forum (Epic staff, 4.15) |
| R8 | **Colours wash out**: the filmic tonemapper (default since 4.15) and auto-exposure change unlit sprite colours; `ShowFlag.Tonemapper 0` "fixes" it but disables a whole section of post-processing (Epic staff warning). Tone Curve Amount 0, Blue Correction 0 and Expand Gamut 0 are the community route to closer colours; nobody documents an exact match | Forum, ACES Central |
| R9 | **Blurry sprites**: Paper2D and texture streaming do not play well; the fix is **Never Stream** on sprite textures; mip and filter choice matter; AA and motion blur blur sprites further | Forum |
| R10 | **Orthographic camera** is fully supported since 5.3 (Lumen, VSM, TSR work with it), but Epic says perspective comes first and newer features may not be optimised for ortho. Near planes can sit behind the camera; depth is scaled for 16-bit precision (`r.Ortho.*` cvars). With no lighting and unlit materials most of this does not bite | Epic docs |
| R11 | **SVG**: no production importer. Epic's `SVGImporter` plugin is Experimental under VirtualProduction (nanosvg rasteriser, SVG as data/actor); third-party UMG SVG plugins exist (one reported icons turning blue after a 5.4 -> 5.5 upgrade) | Epic API docs; forum |
| R12 | **Software cursors lag** the hardware cursor (interpolated; Epic closed it Won't Fix). **Custom hardware cursors** (`Project Settings > User Interface > Hardware Cursors`, PNG/CUR/ANI under a `Content/Slate/...` path, referenced WITHOUT extension or `_WxH` suffix) are reported working in PIE and missing in packaged builds unless staged | Forum, bug tracker |
| R13 | **Package size**: an empty UE5 Windows Shipping build is roughly 150-320 MB depending on version and settings (prerequisites installer ~48 MB, cooked compression, cooked maps, plugins) | Community guides |
| R14 | Lighting, shadows, Lumen, Nanite, ray tracing, Substrate: none of it is useful to a flat unlit 2D game, and leaving it on cost a real project a PSO out-of-memory crash and ~22-30 MB of shaders | KB U/12 §1.3 (verified) |

**What this means**: do not build the game out of Paper2D actors. Draw the field with a small renderer of our own on
top of instanced quads and unlit materials, under an orthographic camera, with every post-process feature that
changes pixels switched off, and prove the colours and edges with measurements.

## 3. Colour fidelity (decided; proved by spike S1 and a gate)

The Stacklands palette is the game's identity; an ink `#1A1A1A` that renders `#262626` is a bug.

1. **Every field material is Unlit.** No lights in the level at all.
2. **Exposure fixed**: project `r.DefaultFeature.AutoExposure=False`; in the one unbound Post Process Volume:
   Metering Mode **Manual**, Exposure Compensation chosen so an unlit 1.0 stays 1.0 (spike S1 measures it; with
   "Apply Physical Camera Exposure" off a manual exposure of 0 EV is the starting point).
3. **Tonemapper neutralised, not removed**: Tone Curve Amount 0, Blue Correction 0, Expand Gamut 0, film slope/toe/
   shoulder left default (Tone Curve 0 bypasses them), Saturation/Contrast/Gamma/Gain neutral, no vignette, grain,
   chromatic aberration, bloom, lens flare, motion blur, local exposure. Do NOT use `ShowFlag.Tonemapper 0` (kills the
   sRGB output path and other post-processing; Epic warning).
4. **Textures**: sRGB on for colour art; compression chosen by spike S3 (BC7 vs uncompressed `UserInterface2D`).
5. **The colour gate** (doc 09): a test map renders one flat swatch per palette entry (from `tools/palette.py`) through
   the real sprite material and the real post-process, reads the pixels back (`FViewport::ReadPixels` or a
   `SceneCapture2D` to a render target with the final colour), and asserts every swatch within +/-2 per channel of the
   palette value. Negative-test it by enabling Tone Curve Amount 1 and watching it fail.
6. UI (UMG/Slate) does not go through the tonemapper the same way; the colour gate also renders a UI swatch row.

**Fallback if exactness cannot be reached in the scene**: render the field into a render target with an unlit,
untonemapped composite (a Scene Capture with `SCS_FinalColorLDR` and post-processing disabled, or the "draw the field
in Slate" option §5.3). The spike decides; the gate keeps it honest.

## 4. Sorting and edges (decided in principle; the AA method is spike S1's)

**Sort by depth, not by translucency.** The Godot game sorts creatures by Y (lower on screen = in front). In Unreal:

- The camera looks straight down the -Z axis at the XY plane; field pixel (x, y) maps to world (x, -y) with
  `1 field px = 1 unreal unit`.
- Each sprite's Z = `BaseLayerZ + (y_feet / FieldHeight) * LayerSpan`: lower on screen = higher Z = closer to the
  camera. Depth testing then sorts every pixel correctly, **including between instances of one instanced component**
  (which translucency cannot, R6).
- Fixed layers keep their order by Z band: backdrop < ground zones < shadows (one band, R1 of the Godot rules) <
  aura ring < creatures (Y-sorted band) < cat < weapon < hat < over-creature VFX.
- That means creature materials are **Masked** (or opaque with alpha test) so they write depth.

**Edges then need anti-aliasing**, and the choice has costs (R7). Spike S1 renders the same moving crowd four ways and
measures edge quality (screenshots at 1080p and 800p, zoomed 2x), ghosting on fast fliers, and frame time:

| Option | How | Expected |
|---|---|---|
| A. Forward + MSAA 4x + alpha-to-coverage | `r.ForwardShading=1`, `r.AntiAliasingMethod=3` (MSAA), material "Alpha to Coverage" on, steepened mask | Best candidate: crisp, no ghosting, cheap at this scene complexity |
| B. Deferred + TSR | defaults, masked sprites | Smooth edges, risk of smear on moving sprites and blur on ink lines |
| C. Deferred + FXAA | `r.AntiAliasingMethod=1` | Cheap, softens ink lines |
| D. Translucent sprites + Sort Along Axis | one component per sprite, sort priority from Y each frame | Perfect edges; draw calls scale with sprite count (R3); kept only as the fallback for a few special layers (soft glows, the rainbow frame) |

Soft, glow-like things (signature glow, golden sparkle, aura ring fill, poop zones) are **Translucent or Additive** in
their OWN Z band where order inside the band does not matter.

## 5. Drawing many things: the renderer

### 5.1 Plan: `UPawSpriteRenderer` over instanced quads

- **Entities are not Actors.** They are rows in `PcField` arrays (doc 03 §2). The view reads them once per frame.
- One `UInstancedStaticMeshComponent` per **atlas page x material kind** (masked creature, translucent glow, shadow),
  using a 1x1 quad mesh. Per-instance custom data floats (set once, `NumCustomDataFloats = 8`):
  `[0] atlas rect index`, `[1] tint packed or palette index`, `[2] flash (0..1)`, `[3] alpha`, `[4] flip X (+1/-1)`,
  `[5] pulse phase`, `[6] glow`, `[7] spare`. The material looks up the UV rect from a small **atlas-rect texture**
  (a float texture, one texel per sprite) or a material parameter array, so one material draws every creature in a
  page.
- **Pool the instances**: the component holds a high-water mark of instances; dead ones are parked (scale 0 or alpha
  0 and off-screen), never removed (removal reorders instance indices). Update transforms in one batch per frame
  (`BatchUpdateInstancesTransforms` / `UpdateInstances` with `bMarkRenderStateDirty` only once).
- **Interpolate** positions between sim steps (doc 03 §5).
- **The pulse and liveliness** (Godot: per-instance scale/rotation wobble) move into the material (vertex shader:
  `sin(Time * rate + phase)` scale about the feet), costing nothing on the CPU.
- **Shadows**: one instanced component, one soft-ellipse texture or SDF material, its own Z band, translucent (order
  within the band is irrelevant).
- **The cat, weapon and hat**: three instances (or three plain components): only one cat.
- **Boss**: its own component (big, one).

**Budget**: ~6-10 draw calls for the whole field plus the backdrop and VFX, against a KB budget of 2,500 `[KB U/15]`.

### 5.2 Rejected: Paper2D components per entity
Paper2D's `UPaperSpriteComponent` per mouse is the documented way and would be the fastest to prototype, but it is one
component (one or more draw calls, plus transform updates) per sprite (R3) and its development has stalled (R1). Use it
only if spike S2 shows the instanced renderer is not worth its cost, which is unlikely at 320+ entities.

### 5.3 The other credible option: draw the field in Slate
Slate draws boxes, lines, splines and custom vertices with exact UI colours, anti-aliased lines, DPI scaling that
behaves like Godot's `canvas_items` stretch, and batching by texture/layer. The Godot VFX primitives (`_draw` of arcs,
polylines, polygons) map almost 1:1 onto `FSlateDrawElement::MakeLines` / `MakeCustomVerts`. Costs: no Niagara, no
depth buffer (sort on the CPU by Y, cheap for ~700 items), everything on the game thread in `OnPaint`, trailer capture
through Movie Render Queue's UI pass. **Spike S2 builds a 30-minute prototype of this too** and compares game-thread
cost at 700 elements. Choose by measurement; the world-space renderer is the default.

## 6. Camera and resolution

- One `ACameraActor`, **Orthographic**, looking down -Z, `OrthoWidth = logical field width`, auto ortho planes off, near
  and far set explicitly around the Z bands (R10's depth-precision notes; `r.Ortho.*` stay default unless spike S1 sees
  z-fighting).
- **Logical size**: `LogicalHeight = 1080`, `LogicalWidth = 1080 * (window width / window height)`: exactly Godot's
  `expand` aspect. Recompute on viewport resize (`FViewport::ViewportResizedEvent`); everything cached against the old
  size (kitten roam bounds, boss position, spawn edges, `playable_bounds()`) re-derives.
- **The UI matches**: DPI scale rule ShortestSide with a linear curve 1080 -> 1.0 (and 2160 -> 2.0), so UMG and the field
  share one "logical pixel". Full-rect widgets use anchors, never offsets.
- `playfield_rect()` excludes the dock (left by default, flippable): read the dock's real geometry, never assume a side
  or that the field starts at x = 0 (Godot rule).
- **Screen-space to field**: `field_x = mouse_x / DPIScale`, `field_y = mouse_y / DPIScale` (orthographic: no
  deprojection needed; still route through one function so the pad's virtual cursor and the mouse share it).
- **Resolution option** changes the WINDOW size; internal render size is the window's (no fixed internal resolution):
  the art is 2x so it stays sharp at 4K. Steam Deck: 1280x800 native, everything at the 16:10 logical width.

## 7. Effects

| Godot primitive | Unreal plan |
|---|---|
| Discs and rings (aura ring, poop zone, crater, slam ring, telegraph) | **SDF material on an instanced quad** (`M_FxShape`, per-instance: shape, radius, thickness, colour, progress). Crisp at any resolution; one draw call per band |
| Arcs and slashes (`VfxSwipe`: ink-edged arc + three claw slashes; claw marks on the victim) | Same SDF material (arc = ring masked by angle; slash = capsule), ink edge as a second SDF threshold |
| Beams (Prism, overkill splash) | Capsule SDF quads stretched between two points |
| Bursts, shards, sparkles, edge-fire embers | **Niagara** CPU sprite emitters (few particles each), pooled, fixed bounds, no warm-up, an Effect Type per category for budgets `[KB U/13]`; or instanced quads if spike S2 shows Niagara overhead per burst |
| White flash, signature glow, golden sparkle, tint | Per-instance custom data in the creature material |
| Rainbow frame / rainbow bar | UI materials (UMG) |
| Graphics setting ("effects: full / reduced / off") | One `EffectsBudget` in `UPawGameUserSettings`; every effect spawner asks it; "reduced" scales counts with a floor of one; "off" returns nothing (the Godot gate's rule) |

Every effect is triggered by draining `PcEvents` (doc 03 §5), never by the simulation calling into rendering.

## 8. The phase-0 spikes (throwaway code in `Source/Pawpocalypse/Dev/Spikes`, deleted after)

| Spike | Build | Measure | Decides |
|---|---|---|---|
| **S1 Look** | `L_SpikeLook`: ortho camera, unlit masked + translucent quad materials, the post-process of §3, 40 real creature PNGs from the Godot SVGs at 2x, a palette swatch row, 200 sprites moving at flier speed; AA options A-D | Colour readback vs palette (+/-2), edge screenshots at 1080p/800p, ghosting on movers, GPU ms | AA method; exposure value; masked vs translucent per layer; the colour gate's method |
| **S2 Crowd** | 320 enemies + 320 shadows + 30 kittens + 64 zones + 40 effects, driven by a fake sim at 60 Hz, three renderers: instanced quads (§5.1), Paper2D components (§5.2), Slate canvas (§5.3) | Game/render/GPU ms (packaged Development, `stat unit`, Insights) at 1080p on the dev PC and at 1280x800 with the GPU limited; draw calls | The renderer |
| **S3 Pipeline** | `tools/raster_svg.py` (resvg) -> PNG at 1x/2x -> atlas packer -> headless import with Never Stream, mips, sRGB, BC7 vs UI2D | Visual diff vs Godot screenshots, atlas page count, VRAM, disk size | Raster scale, compression, atlas size (doc 07) |
| **S4 Dock and tree** | Common UI root layout with the dock (6 tabs, 15 shop rows), the status row, and a 87-node canvas tree with pan/zoom, all driven by keyboard and pad | Focus never lost; every row/node reachable; no frame spikes when lists refresh; `stat slate` | Widget structure (doc 05) |

Write each spike's numbers and decision into this doc (§10) and into `C:\Games\Knowledge\2D\02-unreal-2d.md` (tagged
`[Verified]`), then delete the spike code.

## 9. Optimisation rules for this game

From `Knowledge/2D/01` applied to Pawpocalypse. Each one is a test or a logged soak metric (doc 09), not a memory.

| Rule | Why / how |
|---|---|
| **No Lumen, no ray tracing, no Nanite, no VSM, no distance fields, no Substrate** (doc 03 §7) | Nothing in the game is lit; leaving them on costs shaders, PSOs and VRAM (a real project crashed creating an unused RT PSO) |
| **Entities are array rows, never Actors**; pools with high-water marks; dead instances parked, never removed | 320 enemies + shadows + kittens + zones; per-Actor cost is the classic 2D-in-UE slowdown |
| **A uniform spatial grid** in `PcField`, cell ~ 64-100 field px, rebuilt once per step | Kittens seek the nearest mouse (detection radius 500), overkill splashes to the nearest survivor, the aura and zones test overlap: all-pairs at 320 x 30 every step is waste. Measure the crowded case (everything under the cat) |
| **Per-step budgets** (12 strikes, 3 splash beams per frame as in Godot; spawns per step capped) | Late-game damage kills a field in one frame; effects must not follow it |
| **Shadows, ground zones and glows in their own Z bands**; creatures in one Y-sorted band | Interleaving breaks batches (Godot: shadows on one layer took 632 draw calls to 2) |
| **Octagon quads** for creature instances if spike S2 shows fill-rate cost at 1280x800 | Chubby round sprites leave the corners of a quad empty; an octagon cuts most of it for 8 vertices |
| **Atlas cells padded and edge-extruded** by 2^(mip levels used) px; mips only where sprites are drawn smaller than rasterised | Mip bleeding between cells shows as coloured fringes on small mice |
| **Never Stream** on every field texture; biome textures loaded when the biome begins, never per frame | Streaming blurs 2D sprites; loads during play hitch |
| **Unfocused/minimised throttle**: on focus loss, cap to 30 fps (option: 15) and keep the fixed-step sim exact; restore on focus; "mute when unfocused" option | An idle game is left running for hours; engine background throttles are unreliable, so the game does it from `FSlateApplication` focus events and the soak measures CPU% |
| **Frame cap options** (30/60/120/unlimited) + vsync; no hidden cap under vsync | The Godot cap hid headroom |
| **HUD at 15 Hz from dirty flags; hidden tabs idle** | Godot's achievements tab cost 39 ms/s while hidden |
| **Effects quality setting honoured by every spawner** (floor of one in "reduced") | Godot gate rule |
| **Travel without a level load** (doc 03 §4) | Scene changes were Godot's worst frames (135-150 ms) |
| **Budgets**: 60 fps at 1280x800 on a Deck-class GPU; < 50 draw calls for the field at peak; game thread < 4 ms at peak on the dev PC | Godot measured p99 4.55 ms; the port must not be slower |

## 10. Spike results

*Empty until phase 0 runs. Each entry: date, machine, engine build, the numbers, the decision.*
