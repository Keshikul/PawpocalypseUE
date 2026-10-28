# 13 - Research sources (2026-10-28)

Weight: Epic/Valve documentation > Epic bug tracker > forum threads (single reports; plausible, unreproduced) > blogs and
vendor listings. Much 2D-in-Unreal community material is from UE4: treat it as a hypothesis for the spikes, not a fact.
The knowledge base's own sources are in `C:\Games\Knowledge\Unreal\26-sources.md`.

## Epic documentation
- 2D in Unreal Engine (Paper 2D overview): https://dev.epicgames.com/documentation/en-us/unreal-engine/2d-in-unreal-engine
- Paper 2D overview: https://dev.epicgames.com/documentation/en-us/unreal-engine/paper-2d-overview-in-unreal-engine
- Orthographic camera (settings, cvars, limitations): https://dev.epicgames.com/documentation/unreal-engine/orthographic-camera-in-unreal-engine?lang=en-US
- `UPaperGroupedSpriteComponent`: https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Paper2D/UPaperGroupedSpriteComponent
- `UPaperSpriteAtlas`: https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Paper2D/UPaperSpriteAtlas?lang=en-US
- `ESpritePolygonMode` (Tight Bounding Box, Diced): https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/Paper2D/ESpritePolygonMode__Type
- `UInstancedStaticMeshComponent` / `SetNumCustomDataFloats` / `SetCustomData`: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UInstancedStaticMeshComponent
- `ETranslucentSortPolicy` (Sort Along Axis "best for 2D"): https://docs.unrealengine.com/4.26/en-US/API/Runtime/Engine/Engine/ETranslucentSortPolicy__Type/index.html
- SVGImporter plugin (Experimental): https://dev.epicgames.com/documentation/unreal-engine/API/PluginIndex/SVGImporter
- Color grading and the filmic tonemapper: https://dev.epicgames.com/documentation/unreal-engine/color-grading-and-the-filmic-tonemapper-in-unreal-engine
- Localization Dashboard: https://dev.epicgames.com/documentation/unreal-engine/localization-tools-in-unreal-engine?lang=en-US
- Patching: https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-create-a-patch-in-unreal-engine

## Epic bug tracker
- Paper2D component search (Sprite Atlas Group hang/crash, 5.3-5.6, Dec 2025): https://issues.unrealengine.com/issue/search?component=ue_gameplay_paper2d
- UE-72476 Translucent Sort Axis ignored by Paper2D (fixed 4.23): https://issues.unrealengine.com/issue/UE-72476
- UE-6675 software cursor lags hardware cursor (Won't Fix): https://issues.unrealengine.com/issue/UE-6675
- UE-135588 IoStore container changes between builds: https://issues.unrealengine.com/issue/UE-135588

## Valve
- SteamPipe uploading (Unreal pak padding alignment 1 MiB): https://partner.steamgames.com/doc/sdk/uploading

## Forums and community (medium/low weight)
- Paper2D washed-out sprites / tonemapper: https://answers.unrealengine.com/questions/586769/view.html , https://forums.unrealengine.com/t/disable-tonemapper/242463
- Washed-out colours in Unreal (realtimevfx): https://realtimevfx.com/t/desaturated-washed-out-colours-in-unreal/17798
- ACES/UE5 matching (Blue Correction, Expand Gamut): https://community.acescentral.com/t/ue5-lut-for-substance-painter/5143
- Translucent vs masked sprite sorting, sort axis: https://forums.unrealengine.com/t/ue4-2d-assets-clipping/375755 , https://answers.unrealengine.com/questions/218376/view.html
- Forward + MSAA + alpha-to-coverage (4.15, Epic staff): https://forums.unrealengine.com/t/issue-with-masked-materials-forward-rendering-msaa/385862 , https://forums.unrealengine.com/t/4-15-p2-msaa-and-alpha2coverage-problem/383502
- Paper2D and texture streaming (Never Stream): https://forums.unrealengine.com/t/project-paper2d/3179/113
- Many moving flipbook actors slow: https://answers.unrealengine.com/questions/468895/view.html
- Batching discussion (Paper2D thread): https://forums.unrealengine.com/t/project-paper2d/3179/457
- Per-instance UVs with ISM: https://forums.unrealengine.com/t/customise-uvs-per-instance-with-instanced-static-meshes/372701
- Hardware cursor in packaged builds: https://forums.unrealengine.com/t/custom-hardware-cursor-problem/437378 , https://answers.unrealengine.com/questions/503041/view.html
- Package size: https://forums.unrealengine.com/t/1950640 , https://www.strayspark.studio/blog/ue5-packaging-shipping-build-size-optimization
- PO import pitfalls: https://answers.unrealengine.com/questions/755446/view.html , https://github.com/Soskat/LocalizationPyTools
- Steam update size with paks: https://forums.unrealengine.com/t/steam-update-size/480513 , https://forums.unrealengine.com/t/pak-alignment/421739
- Background frame caps: https://answers.unrealengine.com/questions/456318/view.html , https://larstofus.com/2024/05/19/unreals-secret-editor-framerate-caps/
- PaperZD: https://www.fab.com/listings/6664e3b5-e376-47aa-a0dd-f7bbbd5b93c0 , https://github.com/heavybullets/PaperZD/releases

## General 2D optimisation (engine-agnostic)
- Unity, "A lightning round of great tips for 2D games": https://unity.com/blog/games/a-lightning-round-of-great-tips-for-2d-games
- "Are your Unity sprites too tight?" (tight vs full-rect meshes): https://gamedev.net/tutorials/programming/graphics/oops-are-your-unity-sprites-too-tight-r5501
- Draw calls and atlases: https://bugnet.io/blog/optimizing-draw-calls-for-better-game-performance , https://www.codeandweb.com/texturepacker/tutorials/using-spritesheets-with-unity
- Overdraw vs batching (LOVE forum): https://love2d.org/forums/viewtopic.php?t=87853
- Spatial partitioning for many entities: https://gamedev.net/forums/topic/385907-best-2d-space-partition-structure-for-point-proximity/ , https://stales.itch.io/modulus7/devlog/971426/collision-detection
- Unfocused frame cap as an engine feature (Flax): https://gitlab.flaxengine.com/Flax/FlaxEngine/commit/394860656ae809899ddecd7334f9fa3b277a1c03

## Not found / open
- No shipped-game postmortem of a 2D game built on Unreal 5 turned up; the plan relies on spikes instead.
- No authoritative statement on Paper2D's maintenance status for 5.6-5.8.
- No current (5.x) source on exact colour matching through the tonemapper, on MSAA alpha-to-coverage quality in 5.8, or on IoStore
  with SteamPipe deltas.
