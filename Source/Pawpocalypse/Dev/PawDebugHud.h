#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "PawDebugHud.generated.h"

// The realm's debug readout (docs/11 phase 2 slice 7), development builds only: frame and step times, steps per frame
// and those dropped, the field's counts, the wallet, the biome, the seed, the last travel. Toggle with `paw.DebugHud 0|1`.
// Plain canvas text, no Common UI: it is a developer's instrument, never a screen.
UCLASS()
class PAWPOCALYPSE_API APawDebugHud : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	double SmoothedFrameMs = 0.0;
};
