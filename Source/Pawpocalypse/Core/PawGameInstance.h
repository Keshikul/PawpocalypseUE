#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

#include "PawGameInstance.generated.h"

// Thin by design (docs/03 §3): the title -> game fade and startup checks land here. Game state lives in
// UPawGameSubsystem over PawCore, never in this class.
UCLASS()
class PAWPOCALYPSE_API UPawGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
