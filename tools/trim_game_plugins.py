"""Rewrites the Plugins list of Pawpocalypse.uproject: unused engine plugins are restricted to the Editor target, so the
editor keeps them and the packaged game does not carry them (docs/03 §7, KB Unreal/19 §3.1).

Two groups are restricted:
  1. RUNTIME_UNUSED: runtime plugins enabled by default that this single-player, Windows-first, flat 2D game never uses.
  2. Every default-enabled plugin with no Runtime-type module (editor-only and content-only). They have no game code, but
     while enabled for every target they pull runtime plugins (ControlRig, RigVM, GeometryCache...) back into the game
     through their dependencies. Scanned from the engine install, so re-run after an engine update.
KEEP_FOR_GAME lists editor-type plugins a game plugin depends on, and the GPU capture tools for dev builds.

Restricting a plugin can break the COOK if engine config still references its content (Interchange, NNEDenoiser did):
add its content root to +DirectoriesToNeverCook in Config/DefaultGame.ini (KB Unreal/19 §3.1).

Check the result in the link response file Intermediate/Build/Win64/x64/Pawpocalypse/<Config>/Pawpocalypse.exe.rsp
(phase 0: 34 plugin folders linked instead of 64). To use a restricted plugin in the game, remove it from the lists here.

  py tools/trim_game_plugins.py [--engine "C:/Program Files/Epic Games/UE_5.8/Engine"]
"""
import argparse
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UPROJECT = os.path.join(ROOT, "Pawpocalypse.uproject")

# Always enabled for every target.
GAME_PLUGINS = ["CommonUI", "EnhancedInput", "PythonScriptPlugin"]
# Enabled, editor only (in addition to the restricted engine defaults below).
EDITOR_PLUGINS = ["EditorScriptingUtilities", "ModelingToolsEditorMode"]

RUNTIME_UNUSED = """
ACLPlugin ActorLayerUtilities ActorSequence AndroidDeviceProfileSelector AndroidFileServer AndroidMedia
AndroidMoviePlayer AndroidPermission AnimationSharing AppleImageUtils AppleMoviePlayer ArchVisCharacter AvfMedia
BackChannel CableComponent ChaosCloth ChaosNiagara ChaosUserDataPT ChaosVD CharacterAI ChunkDownloader
ConsoleVariables ControlRig ControlRigPhysics ControlRigSpline CustomMeshComponent Dataflow DatasmithContent
EngineCameras ExampleDeviceProfileSelector FacialAnimation GameplayCameras GeometryCache GLTFExporter
GoogleCloudMessaging GooglePAD IKRig ImgMedia Interchange InterchangeAssets InterchangeAxfAssets
IOSDeviceProfileSelector LauncherChunkInstaller LocationServicesBPLibrary LowLevelNetTrace MediaCompositing MediaPlate
MetaHumanSDK MobilePatchingUtils MsQuic MutableAssetUserData NFORDenoise NiagaraSimCaching NNEDenoiser
OnlineSubsystemGooglePlay OnlineSubsystemIOS OodleNetwork PCG ProceduralMeshComponent ResonanceAudio RigVM
TcpMessaging TweeningUtils UdpMessaging UObjectPlugin VariantManagerContent WebMMoviePlayer WindowsMoviePlayer WmfMedia
""".split()

# DataValidation <- EnhancedInput, EngineAssetDefinitions <- CommonUI, ContentBrowserAssetDataSource <- Metasound;
# RenderDoc/PIX for GPU captures in Development builds.
KEEP_FOR_GAME = {"DataValidation", "EngineAssetDefinitions", "ContentBrowserAssetDataSource", "RenderDocPlugin", "PixWinPlugin"}

RUNTIME_TYPES = {"Runtime", "RuntimeNoCommandlet", "RuntimeAndProgram", "ClientOnly", "ServerOnly", "CookedOnly"}


def editor_type_plugins(engine):
    found = []
    for dirpath, _, files in os.walk(os.path.join(engine, "Plugins")):
        for name in files:
            if not name.endswith(".uplugin"):
                continue
            text = open(os.path.join(dirpath, name), encoding="utf-8-sig").read()
            try:
                desc = json.loads(re.sub(r",\s*([}\]])", r"\1", text))  # some .uplugin files have trailing commas
            except ValueError:
                continue
            if not desc.get("EnabledByDefault"):
                continue
            types = {m.get("Type", "") for m in desc.get("Modules", [])}
            if not types & RUNTIME_TYPES:
                found.append(name[: -len(".uplugin")])
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--engine", default="C:/Program Files/Epic Games/UE_5.8/Engine")
    args = parser.parse_args()

    restricted = set(RUNTIME_UNUSED) | (set(editor_type_plugins(args.engine)) - KEEP_FOR_GAME)
    restricted -= set(GAME_PLUGINS) | set(EDITOR_PLUGINS)

    project = json.load(open(UPROJECT, encoding="utf-8-sig"))
    project["Plugins"] = (
        [{"Name": n, "Enabled": True} for n in GAME_PLUGINS]
        + [{"Name": n, "Enabled": True, "TargetAllowList": ["Editor"]} for n in EDITOR_PLUGINS]
        + [{"Name": n, "Enabled": True, "TargetAllowList": ["Editor"]} for n in sorted(restricted, key=str.lower)]
    )
    with open(UPROJECT, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(json.dumps(project, indent="\t"))
    print(f"{UPROJECT}: {len(restricted)} engine plugins restricted to the Editor target")


if __name__ == "__main__":
    main()
