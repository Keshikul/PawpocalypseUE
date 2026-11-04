#include "Core/PawPlayerController.h"

#include "EngineUtils.h"
#include "Realm/PawRealmView.h"
#include "UI/PawHudScreen.h"
#include "UI/PawRootLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

APawPlayerController::APawPlayerController()
{
	// The cat follows the pointer: the cursor is always visible.
	bShowMouseCursor = true;
}

void APawPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	RootLayout = CreateWidget<UPawRootLayout>(this, UPawRootLayout::StaticClass());
	RootLayout->AddToViewport();
	// The HUD where there is a field to play (the title map gets its menu in phase 4).
	bool bRealm = false;
	for (TActorIterator<APawRealmView> It(GetWorld()); It; ++It)
	{
		bRealm = true;
		break;
	}
	if (bRealm)
	{
		Hud = RootLayout->GameStack->AddWidget<UPawHudScreen>(UPawHudScreen::StaticClass());
		Hud->SetMenuStack(RootLayout->MenuStack);   // the card pick and the skill tree open there
		Hud->SetModalStack(RootLayout->ModalStack); // the job picker
	}
}
