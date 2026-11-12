#pragma once

// The assets the UI looks up on events, held for the engine's life (review CR12): a prompt's glyph texture, the baked Widget
// Blueprint class a C++ class resolves to, the game's font. They were cached weakly, so the GC could take one no widget was
// showing, and the next lookup (every prompt redraws on a device switch) loaded it again, synchronously, in the middle of
// play: nothing should load per event. A glyph stem with no texture is remembered too, so a keycap prompt does not look for
// a package on every redraw. An engine subsystem rather than a static (KB U/02 §2: a static strong pointer is released after
// the engine); the accessors keep their signatures, with no world context.

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"

#include "PawUiAssets.generated.h"

class UFont;
class UTexture2D;

UCLASS()
class PAWPOCALYPSE_API UPawUiAssets : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	// Null without an engine; the callers then load on every lookup, as before any cache.
	static UPawUiAssets* Get();

	// PawGlyphs::Texture: a stem's texture, or null for a stem with none (asked once either way).
	UPROPERTY()
	TMap<FString, TObjectPtr<UTexture2D>> Glyphs;

	// PawUi::Resolve: a C++ class's baked Blueprint class, or the C++ class itself when none is baked. Emptied when the code
	// layout is forced or the resolver reset. A value the editor's force-delete nulled is looked up again.
	UPROPERTY()
	TMap<TObjectPtr<UClass>, TObjectPtr<UClass>> Classes;

	// PawUi::PawFont: F_Paw.
	UPROPERTY()
	TObjectPtr<UFont> Font;

	virtual void Deinitialize() override;
};
