#pragma once

// The input assets (docs/05 §3), GENERATED from Data/InputMap.csv (Godot's input map; tools/export_input_map.py) and never
// edited by hand, like the DataTables: one Enhanced Input action IA_<Action> per Godot action under
// /Game/Pawpocalypse/Input/Actions, and one mapping context IMC_Pawpocalypse holding every event (Godot keeps every action live
// at once; one context does the same). An action with a stick or trigger event is Axis1D (its events read through
// UPawHalfAxis); any other is Boolean. Re-running updates the assets in place; an action no longer in the map is reported, never
// deleted.
//
// Run headless: py tools/create_input_assets.py --run (editor closed).

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "PawInputAssetBuilder.generated.h"

UCLASS()
class UPawInputAssetBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Builds the assets from the CSV at CsvPath. Returns one line per asset; a line starting "FAILED" on any error.
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString BuildInputAssets(const FString& CsvPath);
};
