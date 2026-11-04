#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "PawGameMode.generated.h"

// Sets the framework classes and nothing else: no state, no UI (docs/03 §3, KB U/03 §1).
UCLASS()
class PAWPOCALYPSE_API APawGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APawGameMode();
};
