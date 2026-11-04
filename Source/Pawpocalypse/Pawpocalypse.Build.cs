// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Pawpocalypse : ModuleRules
{
	public Pawpocalypse(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Folders per docs/03 §1 (Core, Data, Realm, Render, UI, Input, Save, Audio, Platform, Dev); includes are
		// written from the module root, e.g. "Core/PawGameMode.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { "PawCore" });

		// The UI (Common UI over UMG and Slate, docs/05) and the packaged frame gate (paw.Bench: thread and GPU times).
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG", "CommonUI", "CommonInput", "RHI", "RenderCore" });

		// SPIKES only (Dev/Spikes: back-buffer PNG, Paper2D). Remove with the spike code.
		PrivateDependencyModuleNames.AddRange(new string[] { "ImageCore", "Paper2D" });
	}
}
