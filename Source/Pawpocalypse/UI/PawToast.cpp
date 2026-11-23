#include "UI/PawToast.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "UI/PawIcons.h"
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
	constexpr double IconPx = 28.0;     // D18 (S4): the UI concept's controller icon before the words
	constexpr float IconGap = 10.0f;
}

bool UPawToast::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Plate"));
		Plate->SetBrush(PawUi::Chrome(PawUi::EPawChrome::PlateSmall)); // R3: the small plum plate
		Plate->SetPadding(PawUi::PlatePadding(FMargin(PadX, PadY), PawUi::EPawChrome::PlateSmall));
		UOverlaySlot* S = Root->AddChildToOverlay(Plate);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetVerticalAlignment(VAlign_Top);
		S->SetPadding(FMargin(0.0f, TopOffset, 0.0f, 0.0f));
		// D18 (S4, the UI concept's scrUpgrades): the controller icon, then the words, heavy (every toast is the pad's).
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Line"));
		Plate->SetContent(Line);
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		Icon->SetBrush(PawUi::IconBrush(TEXT("ui_icon_pad"), IconPx));
		UHorizontalBoxSlot* IS = Line->AddChildToHorizontalBox(Icon);
		IS->SetVerticalAlignment(VAlign_Center);
		IS->SetPadding(FMargin(0.0f, 0.0f, IconGap, 0.0f));
		Message = PawUi::Heavy(PawUi::MakeText(WidgetTree, TextPx, PawTokens::UI_TEXT, false, TEXT("Message")));
		Line->AddChildToHorizontalBox(Message)->SetVerticalAlignment(VAlign_Center);
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		PawUi::UseAtlasIcons(this); // the asset's saved icon brushes: the atlas's cells (one batch)
	}
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

bool UPawToast::ShowsPadIcon() const
{
	const UImage* Icon = WidgetTree ? Cast<UImage>(WidgetTree->FindWidget(TEXT("Icon"))) : nullptr;
	const UObject* Art = Icon ? Icon->GetBrush().GetResourceObject() : nullptr;
	return Art && Icon->GetVisibility() != ESlateVisibility::Collapsed && Art->GetName() == TEXT("T_ui_icon_pad")
		&& Message && Icon->GetParent() == Message->GetParent()
		&& Icon->GetParent()->GetChildIndex(Icon) < Icon->GetParent()->GetChildIndex(Message);
}

bool UPawToast::IsMessageHeavy() const
{
	return Message && Message->GetFont().TypefaceFontName == FName(TEXT("Black"));
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
