#include "UI/PawRootLayout.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

bool UPawRootLayout::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		GameStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(), TEXT("GameStack"));
		MenuStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(), TEXT("MenuStack"));
		ModalStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>(UCommonActivatableWidgetStack::StaticClass(), TEXT("ModalStack"));
		for (UWidget* W : { static_cast<UWidget*>(GameStack), static_cast<UWidget*>(MenuStack), static_cast<UWidget*>(ModalStack) })
		{
			// No transition (KB U/08 §2 rule 6): a 0.4 s fade sets focus too early, and a screen is up the frame it is added.
			static_cast<UCommonActivatableWidgetContainerBase*>(W)->SetTransitionDuration(0.0f);
			UOverlaySlot* S = Root->AddChildToOverlay(W);
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}
	}
	return Super::Initialize();
}
