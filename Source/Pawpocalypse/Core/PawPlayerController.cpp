#include "Core/PawPlayerController.h"

#include "EngineUtils.h"
#include "Core/PawLog.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Input/PawInputMap.h"
#include "InputMappingContext.h"
#include "Realm/PawRealmView.h"
#include "UI/PawHudScreen.h"
#include "UI/PawMainMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawWidgets.h"
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
	// Godot's input map, every action live at once (docs/05 §3), as the player bound it: the live context (remapping rebuilds
	// it in place and asks Enhanced Input to rebuild, UPawInputDeviceSubsystem). The field reads its actions from here.
	if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (UInputMappingContext* Map = PawInput::LiveContext())
		{
			Input->AddMappingContext(Map, 0);
			UE_LOG(LogPaw, Log, TEXT("Input: %s added (%d mappings)"), *Map->GetName(), Map->GetMappings().Num());
		}
	}
	RootLayout = CreateWidget<UPawRootLayout>(this, PawUi::Resolve<UPawRootLayout>());
	RootLayout->AddToViewport();
	// The HUD where there is a field to play; the title screen everywhere else (L_Title, the game's first map).
	bool bRealm = false;
	for (TActorIterator<APawRealmView> It(GetWorld()); It; ++It)
	{
		bRealm = true;
		break;
	}
	if (bRealm)
	{
		Hud = RootLayout->GameStack->AddWidget<UPawHudScreen>(PawUi::Resolve<UPawHudScreen>());
		Hud->SetMenuStack(RootLayout->MenuStack);   // the card pick and the skill tree open there
		Hud->SetModalStack(RootLayout->ModalStack); // the job picker
	}
	else
	{
		MainMenu = RootLayout->GameStack->AddWidget<UPawMainMenu>(PawUi::Resolve<UPawMainMenu>(), [this](UPawMainMenu& Menu)
		{
			Menu.SetModalStack(RootLayout->ModalStack); // its Settings
		});
	}
}
