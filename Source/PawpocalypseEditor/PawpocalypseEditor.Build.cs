using UnrealBuildTool;

// Editor-only: DataTable/CSV importer, texture/atlas importer, validators, widget baker (docs/03 §1). Never ships.
public class PawpocalypseEditor : ModuleRules
{
	public PawpocalypseEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Pawpocalypse", "PawCore" });
	}
}
