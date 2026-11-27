// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;
using System.IO;

public class PawpocalypseTarget : TargetRules
{
	public PawpocalypseTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("Pawpocalypse");
		// The game's own icon on the exe, its window and the taskbar (playtest 2026-11-26, note 2: the engine's Default.ico
		// showed). Not Build/Windows/Application.ico, the engine's place for it: Build/ is the gitignored build/ on Windows.
		// tools/make_app_icon.py writes it; the Steam target inherits it.
		if (ProjectFile != null)
		{
			WindowsPlatform.ApplicationIcon = Path.Combine(ProjectFile.Directory.FullName, "SourceArt", "icon", "Pawpocalypse.ico");
		}
	}
}
