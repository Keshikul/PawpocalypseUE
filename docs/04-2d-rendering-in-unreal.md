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
   the real sprite material and the real post-process, reads the pixels back, and asserts every swatch within +/-2 per
   channel of the palette value. Negative-test it by enabling Tone Curve Amount 1 and watching it fail.
   **Readback, as proven by spike S1 (§10):** a `USceneCaptureComponent2D` on the game camera, `SCS_FinalColorLDR`,
   into a `UTextureRenderTarget2D` with `RenderTargetFormat = RTF_RGBA8` and `bForceLinearGamma = false`, read with
   `GameThread_GetRenderTargetResource()->ReadPixels`. That works headless (`-RenderOffscreen`). `FViewport::ReadPixels`,
   `FScreenshotRequest` and `HighResShot` all return black frames offscreen (no back buffer), so they only work in a
   windowed or packaged run.
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
| A. Forward + MSAA 4x + alpha-to-coverage | `r.ForwardShading=1`, `r.AntiAliasingMethod=3` (MSAA), material "Alpha to Coverage" on, steepened mask | Best candidate: crisp, no ghosting, cheap at this scene complexity. **S1 (2026-10-28): rejected. Alpha to Coverage breaks on desktop forward in 5.8.3 (black rectangles; documented as mobile), and plain MSAA leaves masked outlines jagged** |
| B. Deferred + TSR | defaults, masked sprites | Smooth edges, risk of smear on moving sprites and blur on ink lines |
| C. Deferred + FXAA | `r.AntiAliasingMethod=1` | Cheap, softens ink lines |
| D. Translucent sprites + Sort Along Axis | one component per sprite, sort priority from Y each frame | Perfect edges; draw calls scale with sprite count (R3); kept only as the fallback for a few special layers (soft glows, the rainbow frame) |

**Chosen by S1 (§10): C, FXAA.** Smooth outlines, no ghosting on movers, +0.02-0.03 ms.

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
**Built (2026-10-31, phase 2 slice 7)**: `UPawSpriteRenderer` (`Source/Pawpocalypse/Render`), five layers (floor, zones, shadows, bodies, effects), one `UInstancedStaticMeshComponent` each on `/Engine/BasicShapes/Plane`, 4 custom floats (linear RGB + alpha) per instance, pooled at a high-water mark and parked, one transform batch a layer a frame. Its materials are generated (`tools/create_field_materials.py`) and draw placeholder shapes from the UVs alone (a disc with an ink rim for bodies, soft discs, rings) until phase 6's atlas swaps the body material. Measured in the editor build: 320 mice + 20 kittens, advance + draw 0.06 ms (draw 0.045 ms). **The effects layer is translucent, not additive**: additive light on the paper floor (#FFF9E3) is invisible (seen in the first screenshot); §7's additive bursts must use a translucent or darkening blend on light biomes.
**Spike S2 (§10) chose this renderer**: 13 draw calls for 774 items, game thread 0.6 ms, worst-case GPU 5.15 ms at 1280x800.

### 5.2 Rejected: Paper2D components per entity
Paper2D's `UPaperSpriteComponent` per mouse is the documented way and would be the fastest to prototype, but it is one
component (one or more draw calls, plus transform updates) per sprite (R3) and its development has stalled (R1). Use it
only if spike S2 shows the instanced renderer is not worth its cost, which is unlikely at 320+ entities. **S2: rejected
(~2,200 draw calls, 2x the game-thread cost).**

### 5.3 The other credible option: draw the field in Slate
Slate draws boxes, lines, splines and custom vertices with exact UI colours, anti-aliased lines, DPI scaling that
behaves like Godot's `canvas_items` stretch, and batching by texture/layer. The Godot VFX primitives (`_draw` of arcs,
polylines, polygons) map almost 1:1 onto `FSlateDrawElement::MakeLines` / `MakeCustomVerts`. Costs: no Niagara, no
depth buffer (sort on the CPU by Y, cheap for ~700 items), everything on the game thread in `OnPaint`, trailer capture
through Movie Render Queue's UI pass. **Spike S2 builds a 30-minute prototype of this too** and compares game-thread
cost at 700 elements. Choose by measurement; the world-space renderer is the default. **S2: Slate drew 774 items for
0.09 ms of paint and half of ISM's GPU time on a weak GPU; kept as the measured fallback (§10).**

## 6. Camera and resolution

- One `ACameraActor`, **Orthographic**, looking down -Z, **`OrthoWidth = 1080` (the logical HEIGHT) with the camera's
  `AspectRatioAxisConstraint` overridden to `MaintainYFOV`**: under that constraint (the engine's default for local players)
  the ortho "width" is the visible height and the width follows the window, which is exactly the expand aspect below
  (spike S1 measured it; corrected 2026-10-28, this line used to say `OrthoWidth = logical field width`). Near and far set
  around the Z bands (R10's depth-precision notes; S1 used auto planes and saw no z-fighting; `r.Ortho.*` stay default).
- With no pawn, the player controller resets its view target to itself after `BeginPlay`: `APawRealmView` sets the camera
  as view target once the controller is ready (S1 found it).
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
| ~~A uniform spatial grid in `PcField`~~ **Not needed (measured 2026-10-31)** | The planned grid was for kittens seeking, splash and overlap tests at 320 x 30. Measured with plain loops over the struct-of-arrays rows: a full field (320 mice, 21 kittens, the aura, zones, the Prism, Snare, Gridlock) steps in **0.093 ms** (`Long: the step time ...`, MSVC /O2). The loops also keep Godot's tie order exact for free. Revisit only if a profile says so |
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

*Each entry: date, machine, engine build, the numbers, the decision.*

### S4 Dock and tree - 2026-10-28 - DONE: the doc 05 structure works on pad and keyboard

Built in C++ only (`Source/Pawpocalypse/Dev/Spikes/SpikeUi.*`, map `L_SpikeUi`): a root layout with Game and Menu
`UCommonActivatableWidgetStack`s; the HUD activatable with the dock (6 tab buttons, a `UWidgetSwitcher` of pages, 15 shop rows
of `UCommonButtonBase`, explicit navigation) and a `HitTestInvisible` status row; the skill tree activatable with 87 Common-button
nodes on a hex lattice in ONE canvas, links painted by a `HitTestInvisible` widget beneath them, explicit 4-way navigation
computed from the geometry, RT/LT zoom and auto-pan by render transform. A dev actor walks it with **real Slate key events**
(`FSlateApplication::ProcessKeyDownEvent`, D-pad, shoulders, face buttons, arrows) and checks focus after every step.

Result (offscreen and windowed, 1920x1080): **1,193 checks pass, focus lost on 0 frames**: all 15 rows down and up, no wrap at
the end, top row to its tab, tabs left/right, RB/LB switch pages with focus on the page, keyboard arrows, a keyboard -> pad
switch mid-list keeps the place, focus survives a 15 Hz refresh, Y opens the tree on its start node, RT zooms, **every one of
the 87 x 4 navigation rules pressed and verified** (each focused node also checked on screen after auto-pan), B closes the tree
and focus returns to the dock's remembered row. Costs: opening the tree (87 buttons built and activated) 6-9 ms on the game
thread; relabelling 15 rows + the status row at 15 Hz: frame p99 1.8-2.3 ms, no spike. The HUD passes with both
`ECommonInputMode::Game` and `All` (with injected Slate events; confirm on real pads in phase 5).

Negative tests: (1) before a fix, the remembered row failed for real (below); (2) `-S4Sabotage=focus` (no focus target on the
tree): focus fell to `SViewport` on 1,679 frames, 412 checks failed; (3) `-S4Sabotage=nav` (one rule removed): the first tour
walked shortest paths to every node and **passed anyway**, so the walk was changed to press every rule; then it failed.

**Decisions** (doc 05 updated): the structure of doc 05 §2 and §6 stands. Rules found by the spike:
- **The dock's remembered row must follow focus continuously.** Remembering it only on a tab switch failed: a keyboard -> pad
  switch made Common UI re-apply `GetDesiredFocusTarget()` (the stale row) before the shoulder-button handler ran.
- **Build C++ widget content before `Super::Initialize()`**: create the `WidgetTree`, set its root, then call the parent;
  `UCommonButtonBase::Initialize` wraps the root in its internal button and `UUserWidget::Initialize` keeps an existing tree.
- **Style the focus explicitly** (the "one highlight" of doc 05 §6.1): tree nodes showed the hover brush on focus, dock rows
  only a thin default outline.
- **The tree screen needs its own opaque backdrop** (the dock showed through under the zoomed lattice).
- **Tours press every navigation rule**, not shortest paths: reachability alone missed a removed rule.
- Back (B/Esc) was handled by hand: Common UI's input data (Click/Back actions) is phase 5 work (doc 05 §1).

### S3 Pipeline - 2026-10-28 - DONE: 2x rasters, BC7, 2048 pages; and the game now renders at 100 %

Scope: the 52 field sprites (every enemy texture at its `sprite_target_px`, the 7 bosses at native size, the 7 cat stages at
175 px, the kitten at 96). `tools/spikes/s3_raster_pack.py` rasterises with `resvg_py` 0.5.0 (installed), sizes = the largest
on-screen size (target x 1.03 breathing pulse, the cat x 1.08 hop more) rounded up to 4; shelf-packs 2048 pages with cells
edge-extruded by 4 px; renders the **ideal** reference (each SVG rasterised directly at its drawn size) and a Godot-equivalent
(native-size raster minified through a mip chain, as Godot's svg/scale 1.0 + linear-mipmap filter does). Unreal side:
`SpikeArtActor` draws every sprite at its drawn size from the imported pages, captured offscreen (1080p and 4K) and on the
real screen (1080p, windowed); `tools/spikes/spike_s3_compare.py` measures per sprite: interior colour error (edges excluded),
error over the whole cell, and sharpness (gradient energy vs the ideal).

| Variant | texels (1 page = 4.19 M) | pages @2048 | VRAM BC7 / uncompressed (with mips) |
|---|---|---|---|
| r1x (1x largest on-screen size) | 0.94 M | 1 | 5.6 / 22.4 MB |
| **r2x (2x)** | 3.74 M | 2 (1 @4096) | **11.2** / 44.7 MB |
| godot (native SVG size) | 1.29 M | 1 | 5.6 / 22.4 MB |

| Image (interior error / sharpness; mean over 52 sprites) | 1080p screen, no AA | 1080p screen, FXAA | 4K capture |
|---|---|---|---|
| Godot-equivalent (offline) | 3.47 / 0.97 | - | - |
| r1x BC7 | 5.77 / 0.96 | 7.96 / 0.89 | 8.91 / 1.00 |
| **r2x BC7** | 6.86 / 0.95 | 8.76 / 0.88 | **4.26 / 1.05** |
| godot-size BC7 | 6.32 / 0.95 | 8.29 / 0.89 | 5.66 / 1.04 |
| r2x uncompressed (capture, 1080 / 4K) | 8.75 / 0.91 | | 4.21 / 1.05 |

**Decisions** (doc 07 §2, §4 updated):
- **Raster scale 2x** of the largest on-screen size (doc 07's plan stands): at 4K it halves the error (4.3 vs 8.9); at 1080p it
  costs about one point (6.9 vs 5.8) for being minified.
- **Compression BC7**: indistinguishable from uncompressed on this flat art (interior error +0.05 to +0.10) at a quarter of the
  memory. UI icons stay `UserInterface2D` (drawn 1:1, doc 07 §4).
- **Atlas pages 2048**, cells extruded 4 px: the field sprites take 2 pages at 2x (11.2 MB VRAM); one 4096 page would hold them
  but wastes half of 21 MB. One more ISM per page is one more draw call (S2: 13 total).
- Not measured here: cooked disk size (measure with the full set in phase 6/8), trimming and pivots (phase 6 packer),
  premultiplied alpha (the masked material thresholds alpha; revisit with the translucent glow layers).

**Found on the way, and fixed in config: the game rendered below native resolution.** The desktop default
`r.ScreenPercentage.Default.Desktop.Mode` = BasedOnDisplayResolution (`[Rendering.AutoScreenPercentage]`) renders a 1080p window at
810p and a 4K window at 1080p, then upscales: 1080p screen interior error 14.8 and sharpness 0.78, against 8.8 / 0.88 at 100 %.
Now `[SystemSettings] r.ScreenPercentage.Default.Desktop.Mode=0`, `r.ScreenPercentage.Default=100` in `DefaultEngine.ini`,
verified by a windowed run without overrides (8.76 / 0.88, identical to the forced run). Scene captures were unaffected (they
render at the target's size), which is why S1's capture numbers looked sharper than the screen.

**S1's AA comparison re-run at 100 %** (it had been made on the upscaled image): TAA still ghosts moving faces and TSR still
smears (1.52 ms GPU); MSAA is identical to no AA (75.6 % blended edge pixels both). The real choice is **no AA** (sharpest art,
0.95; a one-pixel stair-step on the outlines of moving sprites) vs **FXAA** (smooth moving outlines; the whole sprite softer,
0.88, interior error +2). Godot alpha-blends its sprites and gets both. This is a call about the look: **open question Q9 for
the designer (doc 01 §3)**; FXAA stays set until answered. Supersampling (screen percentage above 100) would give both at a GPU
cost not yet measured on the Deck-class GPU.

### S2 Crowd - 2026-10-28 - DONE: renderer = instanced quads (ISM); Slate is the measured fallback

Scene (`L_SpikeCrowd`, `Source/Pawpocalypse/Dev/Spikes/SpikeCrowdActor`): 320 enemies + 320 shadows + 30 kittens (seek the
nearest enemy, brute force) + 64 translucent ground zones + 40 additive effect rings = 774 items, fake sim at a fixed 60 Hz
with interpolation, same items drawn by three renderers: **ISM** (4 instanced components: atlas creatures masked with a
per-instance UV offset, translucent shadows and zones tinted by per-instance colour, additive effects; transforms in one batch
per component per frame), **Paper2D** (one `UPaperSpriteComponent` per item), **Slate** (one leaf widget `OnPaint`, CPU sort by
band then feet-Y; "strict" = one layer per creature for exact overlap order). Measured in a **packaged Development build**
(`tools/spikes/spike_s2_runs.ps1`), windowed, after a 6 s warm-up, 600 frames. dGPU = RTX 5070; iGPU = the dev PC's integrated
AMD Radeon (2 RDNA2 CUs, roughly a third of a Steam Deck GPU: a worst case for the Deck-class budget).

| Renderer | 1920x1080 dGPU frame / game / GPU ms | draws | 1280x800 dGPU frame / GPU | 1280x800 iGPU frame / game / GPU | spike update ms |
|---|---|---|---|---|---|
| **ISM** | 1.28 / 0.63 / 0.41 | **13** | 1.26 / 0.39 | 5.59 / 0.71 / 5.15 | 0.06 |
| Paper2D | 2.55 / 1.44 / 0.52 | **~2,200** | 2.50 / 0.50 | 5.09 / 1.74 / 4.52 | 0.45 |
| Slate | 1.08 / 0.66 / 0.30 | 8 | 1.08 / 0.26 | 3.00 / 0.90 / **2.61** | 0.09 (incl. paint) |
| Slate strict order | 1.22 / 0.67 / 0.30 | 357 | 1.21 / 0.26 | 3.01 / 0.90 / 2.62 | 0.10 |

Every run was checked by screenshot. **Caveat on Paper2D**: its three translucent sprites kept a lit default material (the
setup's `load_asset` could not find `/Paper2D/TranslucentUnlitSpriteMaterial`), so zones and rings drew as black squares; its
GPU numbers understate it. Its draw calls and game-thread cost decide it regardless.

**Decision: the instanced-quad renderer (§5.1) stays.** All budgets of §9 hold with large margins: 13 draw calls (< 50), game
thread 0.6-0.7 ms (< 4 ms), worst-case GPU 5.15 ms at 1280x800 on a GPU weaker than the Deck's (60 fps = 16.7 ms). It keeps
per-pixel depth sorting (§4), materials (SDF effects, flashes, pulse in the vertex shader) and Niagara. **Paper2D is rejected**:
~2,200 draw calls (44x the field budget) and 2x the game-thread cost. **Slate is the measured fallback**: on the weak GPU it
cost half of ISM's GPU time (2.61 vs 5.15 ms) at equal CPU; if phase 8's Deck profiling shows GPU pressure, look first at what
ISM pays for on that GPU (the deferred scene passes; fill rate of the 64 large translucent zones: §9's octagon quads), then at
Slate. Slate's exact Y order (one layer per creature) costs 357 batches but no measurable time at this size.

Also learned:
- **Packaged builds do not draw a component until its PSOs are precached** (`r.PSOPrecache.*`): with a 120-frame warm-up the
  ISM field was still empty on the iGPU. Measure after a time-based warm-up and check a screenshot. For the game: the realm's
  first frames may show missing layers until precache completes (phase 8 PSO check, doc 10).
- **Editor `-game` timings include shader compilation**: the first editor ISM run drew nothing ("Preparing Shaders").
- **The phase 0 plugin trim broke cooking**: Interchange and NNEDenoiser content is referenced from engine config, and the cook
  fails on it once their code is editor-only. Fixed with `+DirectoriesToNeverCook` (doc 03 §7).
- **Engine-plugin assets loaded only by path from C++ are not cooked** (the Paper2D sprite materials): reference them from an
  asset, or the packaged build gets `Failed to find object`.

### S1 Look - 2026-10-28 - DONE: exact colour proven; AA = FXAA

Machine: RTX 5070 (driver 610.88), D3D12 SM6, UE 5.8.3-58210709 launcher, editor binary `-game -RenderOffscreen` at
1920x1080. Scene (`L_SpikeLook`, `Source/Pawpocalypse/Dev/Spikes/SpikeLookActor`): ortho camera straight down -Z, 21 palette
swatches (per-instance colour, unlit opaque), 40 Godot sprites (33 creatures + 7 bosses rasterised at 2x by Godot's own SVG
renderer; textures uncompressed `UserInterface2D`, sRGB, Never Stream, mips), 200 movers (a fifth at 900 px/s), Y-to-Z depth.
Run with `tools/spikes/spike_s1_runs.ps1`, compared with `tools/spikes/spike_s1_compare.py`.

**Colour: PASS, decided.** With §3's settings (Unlit; unbound post-process volume: Manual exposure, **bias 0 EV**, physical
camera exposure off, Tone Curve Amount 0, Blue Correction 0, Expand Gamut 0, bloom/vignette/grain/fringe/motion blur/AO/lens
flare 0, local exposure contrast 1.0) every one of the 21 swatches is within **1** of the palette (ink #1A1A1A read #1A1A1A or
#191919; paper #FFF9E3 read #FEF9E3), in every AA mode. Negative test: Tone Curve Amount 1 -> worst diff 35 (ink #050505, paper
#DCDBD4), FAIL as it must. So: **exposure value 0 EV; the colour gate method is the scene-capture readback in §3.5.**

| Run | AA | GPU ms (avg, includes the capture) | Notes |
|---|---|---|---|
| N_None | none (deferred) | 0.52 | baseline |
| C_FXAA | FXAA | 0.57 | |
| B2_TAA | TAA | 0.64 | |
| B_TSR | TSR (engine default) | **1.40** | 2x the others at this scene size |
| A0_MSAA | forward + MSAA 4x | 0.69 | first run compiled forward shaders (one 69 ms hitch) |
| A_MSAA_A2C | forward + MSAA 4x + material Alpha to Coverage | 0.68 | **broken: black rectangles cut through sprites.** The
  material flag is documented "for masked material on mobile"; on desktop forward it does not work. Option A as written is out |

The scene capture does **not** show the screen's AA (on the static sprites FXAA, TAA and TSR gave pixel-identical edges and
MSAA was identical to none), so the AA comparison was repeated **windowed**, reading the back buffer (`FScreenshotRequest`),
no capture in the scene:

| Run (windowed) | GPU ms 1920x1080 | GPU ms 1280x800 | Colour (worst diff) | Edges at rest | Moving sprites (900 px/s) |
|---|---|---|---|---|---|
| none | 0.30 | - | 2 | smooth (mips) | stair-stepped outlines |
| **FXAA** | **0.33** | **0.31** | 2 | smooth | **smooth, identical to the sprite at rest: no ghosting** |
| TAA | 0.39 | 0.33 | 2 | smooth | faces dissolve into noisy smear |
| TSR | 1.16 | 0.86 | 1 | smooth | blurred and ghosted badly; ~3x FXAA's GPU time |
| forward + MSAA 4x | 0.42 | 0.39 | 2 | smooth | stair-stepped like none: a masked outline is inside the quad, so MSAA never sees it |

**Decision: FXAA** (`r.AntiAliasingMethod=1`, deferred, no forward shading), set in `Config/DefaultEngine.ini` and confirmed
live. **Amended in S3 (above): these runs were made at the engine's automatic screen percentage (810p upscaled); re-run at 100 %
the choice narrows to no AA vs FXAA, a look question for the designer (Q9).** Creature layers stay **Masked** (depth sorts every pixel, §4; overlaps looked right in every run). Translucent layers
(glows, aura fill) were not built in S1; they keep §4's plan (own Z band, order inside the band irrelevant) and get checked
when the presentation lands (phase 6). Colour on the real screen: within 2 in all nine windowed runs, both resolutions.
Exposure **0 EV**. Colour gate method: scene capture (§3.5) for headless runs; the back buffer in windowed/packaged runs.

Also learned (setup, not decisions):
- The engine `Plane` mesh's UVs put texture-up along world +Y: with the camera's screen-up = +X, sprites need yaw +90.
- With no pawn, the player controller resets its view target to itself after level `BeginPlay`; set the game camera as view
  target once the controller is ready (the spike re-asserts it per tick; the real `APawRealmView` must handle this).
- Masked sprite edges at rest already look smooth without AA because of the mips (2x rasters drawn at 0.6-0.75x).
- **Ortho width is the visible HEIGHT on the player's screen**: the local player's default `AspectRatioAxisConstraint` is
  `MaintainYFOV` (BaseEngine.ini), so `OrthoWidth = 1920` showed the field at 0.57x in a window (a scene capture does not
  go through the local player and looked right, which hid it). §6 is corrected accordingly.
