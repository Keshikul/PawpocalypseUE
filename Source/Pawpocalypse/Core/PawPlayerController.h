#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "PawPlayerController.generated.h"

class UPawHudScreen;
class UPawMainMenu;
class UPawRootLayout;

// Owns the Common UI root layout (docs/05 §2: the local player controller, never the game mode), and later the input
// contexts and the cursor (phases 4-5). In a map with a realm (L_Realm) it pushes the HUD onto the Game stack; anywhere
// else (L_Title) the title screen.
UCLASS()
class PAWPOCALYPSE_API APawPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APawPlayerController();
	virtual void BeginPlay() override;

	UPawRootLayout* GetRootLayout() const { return RootLayout; }
	UPawHudScreen* GetHud() const { return Hud; }
	UPawMainMenu* GetMainMenu() const { return MainMenu; }

private:
	UPROPERTY()
	TObjectPtr<UPawRootLayout> RootLayout;
	UPROPERTY()
	TObjectPtr<UPawHudScreen> Hud;
	UPROPERTY()
	TObjectPtr<UPawMainMenu> MainMenu;
};
