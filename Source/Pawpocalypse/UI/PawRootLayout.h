#pragma once

// The Common UI root (docs/05 §2): three activatable stacks over each other, Game at the bottom (the HUD, always), Menu
// (the tree, the card pick, ceremonies, pause), Modal (settings, confirms, the job picker). Owned by the local player
// controller, never the game mode (KB U/03 §1, U/08 §2). Never Set Input Mode by hand: each screen declares its input
// config and Common UI applies the topmost. A Widget Blueprint (WBP_RootLayout; PawWidgets.h).

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "PawRootLayout.generated.h"

class UCommonActivatableWidgetStack;

UCLASS()
class PAWPOCALYPSE_API UPawRootLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetStack> GameStack;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetStack> MenuStack;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetStack> ModalStack;
};
