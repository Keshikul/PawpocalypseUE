using UnrealBuildTool;

// Editor-only: DataTable/CSV importer, texture/atlas importer, validators, widget baker (docs/03 §1). Never ships.
public class PawpocalypseEditor : ModuleRules
{
	public PawpocalypseEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Pawpocalypse", "PawCore" });

		// The widget baker (PawWidgetBaker): Widget Blueprints from the UI's code layouts, Blueprints of the button styles.
		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "UMGEditor", "Kismet", "KismetCompiler", "AssetRegistry",
			"CommonUI", "Slate", "SlateCore" });

		// The input assets (PawInputAssetBuilder): Enhanced Input actions and the mapping context from Data/InputMap.csv.
		PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "InputCore" });
	}
}
