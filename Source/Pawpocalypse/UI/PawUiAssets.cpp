#include "UI/PawUiAssets.h"

#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"

UPawUiAssets* UPawUiAssets::Get()
{
	return GEngine ? GEngine->GetEngineSubsystem<UPawUiAssets>() : nullptr;
}

void UPawUiAssets::Deinitialize()
{
	// With the engine, not after it.
	Glyphs.Reset();
	Classes.Reset();
	Font = nullptr;
	Super::Deinitialize();
}
