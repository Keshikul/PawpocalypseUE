#include "Core/PawGameInstance.h"

#include "Core/PawLog.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/PawInputMap.h"
#include "Input/PawNavigationConfig.h"
#include "PcCore.h"

void UPawGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogPaw, Log, TEXT("Pawpocalypse started; rules library: %hs"), Pc::LibraryName());
	// Godot's GUI navigation (its ui_* actions) for the menus: never in the editor, whose own UI it would change.
	if (!GIsEditor && FSlateApplication::IsInitialized())
	{
		auto Install = []()
		{
			const TSharedRef<FPawNavigationConfig> Nav = MakeShared<FPawNavigationConfig>();
			FSlateApplication::Get().SetNavigationConfig(Nav);
			UE_LOG(LogPaw, Log, TEXT("Input: the menus navigate on Godot's ui_* keys (%d keys, %d accept / back)"), Nav->KeyEventRules.Num(),
				Nav->KeyActionRules.Num());
		};
		Install();
		// A remap moves the ui_* keys with their rows ("Up" is menu_up AND ui_up): the menus follow.
		PawInput::OnBindingsChanged().AddWeakLambda(this, [Install]()
		{
			if (FSlateApplication::IsInitialized())
			{
				Install();
			}
		});
	}
}
