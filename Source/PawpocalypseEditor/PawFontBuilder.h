#pragma once

// The game's composite font F_Paw (restyle R2, 2026-11-07), built from the generated table UI/PawFontSpec.h
// (tools/fonts.py). Run by tools/import_fonts.py headless after it has imported the font faces (FF_*): Python cannot fill
// an FCompositeFont (FFontData's face asset and sub-face index are not exposed to it), so the composite is made here.

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "PawFontBuilder.generated.h"

UCLASS()
class UPawFontBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Creates F_Paw if it has no asset, then REWRITES its composite whole from the table (the font is generated, never
	// edited: a change goes into tools/fonts.py) and saves it. Returns "built ..." or "FAILED ...".
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString BuildFont();

	// Makes every face asset FF_* from its file in SourceArt/fonts, whole (a .ttc keeps all its faces), with its loading
	// policy; a face whose file is unchanged is kept. Done here, not by the font import factory: the import (and any property
	// edit) runs UFontFace::PostEditChangeProperty, which flushes Slate's font cache WITHOUT checking Slate exists and so
	// asserts in a headless commandlet (UE 5.8.3). Returns one line per face ("made", "kept", "FAILED ...").
	UFUNCTION(BlueprintCallable, Category = "Pawpocalypse")
	static FString BuildFaces();
};
