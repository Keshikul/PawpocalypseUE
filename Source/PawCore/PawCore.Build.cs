using UnrealBuildTool;

// PawCore: every rule and number of the game as plain C++17 (docs/03 §1-2). Its own code includes no engine headers;
// Core is linked only for the module glue in Private/PawCoreModule.cpp. The same sources build without the engine
// through tools/build_pawcore_tests.ps1, so rule tests run in seconds.
public class PawCore : ModuleRules
{
	public PawCore(ReadOnlyTargetRules Target) : base(Target)
	{
		// No shared PCH: it would force-include engine headers into PawCore and hide a missing std include until the
		// standalone build trips over it.
		PCHUsage = PCHUsageMode.NoPCHs;

		// No unity build either: each .cpp compiles alone, exactly as the standalone build compiles it. In a unity group
		// two files' file-local helpers of the same name (IsEqualApprox, Upgrade, ...) collide as soon as the grouping
		// reshuffles, which adding, removing or merely editing a file can do (UBT's adaptive unity pulls git-modified files
		// out of the groups). KB U/01 §2.5.
		bUseUnity = false;

		PrivateDependencyModuleNames.Add("Core");
	}
}
