#include "UI/PawToast.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

namespace
{
	constexpr double TextPx = 18.0;     // the label's font_size
	constexpr float FrameWidth = 3.0f;  // set_border_width_all(3)
	constexpr float Radius = 8.0f;      // set_corner_radius_all(8)
	constexpr float PadX = 18.0f;       // content margins left / right
	constexpr float PadY = 5.0f;        // top / bottom
	constexpr float TopOffset = 6.0f;   // offset_top: the strip above everything
}

bool UPawToast::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Plate"));
		Plate->SetBrush(PawUi::Rounded(PawTokens::UI_PANEL, PawTokens::UI_PANEL_FRAME, FrameWidth, Radius));
		Plate->SetPadding(FMargin(PadX, PadY));
		UOverlaySlot* S = Root->AddChildToOverlay(Plate);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Top);
		S->SetPadding(FMargin(0.0f, TopOffset, 0.0f, 0.0f));
		Message = PawUi::MakeText(WidgetTree, TextPx, PawTokens::UI_TEXT, false, TEXT("Message"));
		Plate->SetContent(Message);
	}
	const bool bOk = Super::Initialize();
	if (bOk)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible); // it can appear over live play space
	}
	return bOk;
}

void UPawToast::SetMessage(const FText& Text)
{
	if (Message)
	{
		Message->SetText(Text);
	}
}

void UPawToast::ShowConnection(bool bConnected)
{
	SetMessage(PawText::Ui(bConnected ? TEXT("INPUT_toast_connected") : TEXT("INPUT_toast_disconnected")));
}

void UPawToast::ShowPadAudio()
{
	SetMessage(PawText::Ui(TEXT("INPUT_toast_pad_audio")));
}

FText UPawToast::GetMessage() const
{
	return Message ? Message->GetText() : FText::GetEmpty();
}

void UPawToast::SetFade(float Alpha)
{
	SetRenderOpacity(Alpha);
	SetVisibility(Alpha > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
