#pragma once

// The widget baker (KB U/09 §2-2.1; Source/Pawpocalypse/UI/PawWidgets.h has the contract). Writes, for every class in its list
// that has NO asset yet, the Widget Blueprint WBP_<Name> (a widget's code layout, its bound parts named and marked as variables)
// or the Blueprint BP_<Name> (a button style), under /Game/Pawpocalypse/UI/Widgets. NEVER over an existing asset: the asset
// owns its layout from the moment it exists (designer 2026-11-04); to rebuild one from code, delete it first.
//
// Run headless: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<abs>/tools/bake_widgets.py (editor closed).
// The list (PawUi::BakedClasses, in the game module so its parity test walks it too) is ordered: styles, then pieces (a piece used inside another before it), then screens; the resolver's cache is reset
// after each asset so the next one holds the new Blueprints.

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "PawWidgetBaker.generated.h"

UCLASS()
class UPawWidgetBaker : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Bakes every listed class without an asset. Returns one line per class: "baked", "kept" (an asset exists) or "FAILED ...".
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString BakeWidgets();

	// GRAFTING: new parts of a class's code layout put into its EXISTING Widget Blueprint, which keeps everything else it has
	// (the designer's edits). Ops, ';'-separated:
	//   add:Name        the part goes beside its nearest sibling (in the code layout) that the asset has, else into its parent
	//                   when the asset has that, with the code's slot settings;
	//   swap:Old>New    the part takes the asset's Old's place and Old's slot (the designer's placement of it); Old goes.
	//   restyle:Name    the part's LOOK from the code layout into the asset's part of that name (a border's brush and padding,
	//                   an image's brush, a bar's style, a size box's overrides); its slot and the rest stay the asset's
	//                   (phase 6 slice 5a: the chrome reaching assets baked before it);
	//   props:Name      a piece's own settings (its native class's EditAnywhere properties: a button's label, its size)
	//                   from the code layout into the asset's piece of that name (phase 6 slice 5b: the tabs' words gone);
	//   slot:Name       its slot and tilt; text:Name a text's whole look; remove:Name the part and all under it (R4c);
	//   wrap:Part>New   the asset's Part, with everything under it as the asset has it, moves into a copy of the code layout's
	//                   panel New, which takes Part's place (R4h: the settings pages into a scroll box);
	//   names:          lists the asset's widgets, Name(Class): changes nothing.
	// All or nothing: one FAILED op and nothing is saved (R4d).
	// A panel with children goes in too when the asset has none of them: made empty, then each child added into it (5b).
	// A piece is a user widget: its properties go in, its own Blueprint draws it. Never touches anything
	// not named. Returns one line per op ("added", "swapped", "restyled", "kept" when the asset already has it, "FAILED ...").
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString GraftParts(const FString& ClassName, const FString& Ops);

	// THE PALETTE MIGRATION (the restyle, docs/WORKLIST-RESTYLE-2026-11-07.md): every baked asset saved its colours as values
	// when it was baked, so a palette change never reaches it by itself. Pairs "RRGGBB>RRGGBB;..." (old sRGB -> new, from
	// tools/bake_widgets.py --recolour: the palette the assets were last migrated to against today's). Every colour of every
	// baked asset's look (its tree widgets in the editable Widget Blueprint, its default object; PawUi::ForEachColour) that
	// is EXACTLY an old palette colour becomes the new one, alpha kept; a colour the designer set by hand to anything else is
	// untouched. Returns one line per asset ("N recoloured" or "unchanged"), "FAILED ..." on a save error.
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString Recolour(const FString& Pairs);

	// THE ROLE-AWARE MIGRATION (restyle R3): as Recolour, with one map for TEXT colours and one for SURFACES
	// (PawUi::ForEachColourRole), because the plum flip turns one old colour into two (ink text -> Bone, ink frames kept).
	// KeepInk: asset names (';'-joined) whose text sits on a light surface and keeps its colour (a card's bone body).
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString RecolourByRole(const FString& TextPairs, const FString& SurfacePairs, const FString& EdgePairs, const FString& KeepInk);

	// THE FONT MIGRATION (restyle R2): every baked asset saved its fonts as values too (the engine's Roboto, UTextBlock's
	// default at bake time). Every font of every baked asset's look (PawUi::ForEachFont) that is an ENGINE font (or none)
	// becomes F_Paw; its typeface name, size, outline and everything else are kept (F_Paw's "Regular" and "Bold" are both
	// the text weight: tools/fonts.py). A font the designer chose from the project is untouched. Returns one line per asset.
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString Refont();

	// THE TEXT FLOOR (restyle R4b): every font and piece text size of every baked asset under PawUi::TextFloorPx (16 px =
	// 12 pt) rises to it (PawUi::UnderTextFloor; a keycap's label exempt). Returns one line per asset.
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString TextFloor();

};
