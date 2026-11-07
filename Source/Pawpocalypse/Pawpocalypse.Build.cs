// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Pawpocalypse : ModuleRules
{
	public Pawpocalypse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// No unity build: every .cpp compiles on its own. Unity groups reshuffle whenever a file is added or removed, and
		// file-local helpers with the same name in two files (S, Shown, Rule, Utf8 in the UI tests; colour constants a local
		// elsewhere shadows: C4459) then stop compiling though nothing in them changed (deleting the 8 spike files did it,
		// 2026-11-04). Also catches a missing include that a unity neighbour hid. KB U/01 §2.5.
		bUseUnity = false;

		// Folders per docs/03 §1 (Core, Data, Realm, Render, UI, Input, Save, Audio, Platform, Dev); includes are
		// written from the module root, e.g. "Core/PawGameMode.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "PawCore" });

		// The title's version line (UGeneralProjectSettings::ProjectVersion).
		PrivateDependencyModuleNames.Add("EngineSettings");

		// The UI (Common UI over UMG and Slate, docs/05) and the packaged frame gate (paw.Bench: thread and GPU times).
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG", "CommonUI", "CommonInput", "RHI", "RenderCore" });

		// The settings' window: the display's work area (FDisplayMetrics).
		PrivateDependencyModuleNames.Add("ApplicationCore");

		// The backdrop's patches and puddles (phase 6 slice 2): one vertex-coloured mesh, Godot's draw order kept.
		PrivateDependencyModuleNames.Add("ProceduralMeshComponent");

		// Settings > Audio > Output: the system's output devices and the swap (the audio mixer's platform, WASAPI on Windows).
		PrivateDependencyModuleNames.AddRange(new string[] { "AudioMixer", "AudioMixerCore" });
	}
}
