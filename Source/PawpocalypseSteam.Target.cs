using UnrealBuildTool;
using System.Collections.Generic;

// The Steam build (doc 10 §5): the game target with Steam switched on (Pawpocalypse.Build.cs sets PAW_STEAM_BUILD=1 for this
// target's name; Platform/PawSteam holds the App ID). The plain Pawpocalypse target never starts the Steam API: the editor,
// every test and gate, and builds outside Steam, so nothing automated can unlock an achievement on the designer's account.
// Packaged by `py tools/package.py --config Shipping --steam`; its start file is renamed Pawpocalypse.exe (Steam's launch option).
public class PawpocalypseSteamTarget : PawpocalypseTarget
{
	public PawpocalypseSteamTarget(TargetInfo Target) : base(Target)
	{
		// Both game targets share the launcher engine's build environment, so both compile the game module into
		// Intermediate/Build/Win64/x64/UnrealGame/<Config>: building one after the other recompiles the game module (its
		// Definitions header changes with PAW_STEAM_BUILD), never mixes them. bOverrideBuildEnvironment does not give a target
		// its own intermediates (tried 2026-11-26).
	}
}
