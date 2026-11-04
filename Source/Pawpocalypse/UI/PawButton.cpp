#include "UI/PawButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"

namespace
{
	constexpr float Radius = 8.0f;
	constexpr float Frame = 2.0f;
	constexpr float FocusFrame = 3.0f; // the theme's focus ring (SBF_focus: 3 px, UI_FOCUS)
	constexpr float StarSize = 14.0f;
	constexpr float CheckSize = 18.0f;
}

UPawButtonStyle::UPawButtonStyle()
{
	using namespace PawUi;
	NormalBase = Rounded(PawTokens::UI_BUTTON, PawTokens::UI_BUTTON_FRAME, Frame, Radius);
	NormalHovered = Rounded(PawTokens::UI_BUTTON_HOVER, PawTokens::UI_FOCUS, FocusFrame, Radius);
	NormalPressed = Rounded(PawTokens::UI_BUTTON_PRESSED, PawTokens::UI_FOCUS, FocusFrame, Radius);
	SelectedBase = NormalBase;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalPressed;
	Disabled = Rounded(PawTokens::UI_BUTTON_DISABLED, PawTokens::UI_TEXT_DISABLED, Frame, Radius);
	// theme Button content margins: 16 x 6
	ButtonPadding = FMargin(16.0f, 6.0f);
	CustomPadding = FMargin(0.0f);
}

UPawLockedButtonStyle::UPawLockedButtonStyle()
{
	using namespace PawUi;
	NormalBase = Rounded(PawTokens::UI_BUTTON_DISABLED, PawTokens::UI_TEXT_DISABLED, Frame, Radius);
	NormalHovered = Rounded(PawTokens::UI_BUTTON_DISABLED, PawTokens::UI_FOCUS, FocusFrame, Radius);
	NormalPressed = NormalHovered;
	SelectedBase = NormalBase;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalHovered;
	Disabled = NormalBase;
	ButtonPadding = FMargin(16.0f, 6.0f);
	CustomPadding = FMargin(0.0f);
}

UPawFlatButtonStyle::UPawFlatButtonStyle()
{
	using namespace PawUi;
	NormalBase = NoBrush();
	NormalHovered = Rounded(PawTokens::UI_BUTTON_HOVER, PawTokens::UI_FOCUS, FocusFrame, Radius);
	NormalPressed = NormalHovered;
	SelectedBase = NormalBase;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalHovered;
	Disabled = NormalBase;
	ButtonPadding = FMargin(10.0f, 4.0f);
	CustomPadding = FMargin(0.0f);
}

UPawButton::UPawButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetStyle(UPawButtonStyle::StaticClass());
}

bool UPawButton::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		Plate = WidgetTree->ConstructWidget<UBorder>();
		Plate->SetBrush(PawUi::NoBrush());
		Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* PlateSlot = Root->AddChildToOverlay(Plate);
		PlateSlot->SetHorizontalAlignment(HAlign_Fill);
		PlateSlot->SetVerticalAlignment(VAlign_Fill);
		PlateSlot->SetPadding(FMargin(-14.0f, -4.0f)); // under the label, inside the frame

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>();
		Line->SetVisibility(ESlateVisibility::HitTestInvisible);
		LineSlot = Root->AddChildToOverlay(Line);
		LineSlot->SetHorizontalAlignment(HAlign_Center);
		LineSlot->SetVerticalAlignment(VAlign_Center);
		CheckBox = WidgetTree->ConstructWidget<UBorder>();
		CheckBox->SetPadding(FMargin(CheckSize * 0.5f));
		CheckBox->SetVisibility(ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* CheckSlot = Line->AddChildToHorizontalBox(CheckBox);
		CheckSlot->SetVerticalAlignment(VAlign_Center);
		CheckSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		// The label, and under it the detail line (collapsed until a row sets one).
		UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* TextsSlot = Line->AddChildToHorizontalBox(Texts);
		TextsSlot->SetVerticalAlignment(VAlign_Center);
		Label = PawUi::MakeText(WidgetTree, PawUi::ButtonPx, PawTokens::UI_TEXT);
		Label->SetJustification(ETextJustify::Center);
		Texts->AddChildToVerticalBox(Label);
		Detail = PawUi::MakeText(WidgetTree, PawUi::LabelPx, PawTokens::UI_TEXT); // a Label under the heading (hud.gd)
		Detail->SetVisibility(ESlateVisibility::Collapsed);
		Texts->AddChildToVerticalBox(Detail);
		CountText = PawUi::MakeText(WidgetTree, PawUi::ButtonPx, PawTokens::UI_TEXT);
		CountText->SetVisibility(ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* CountSlot = Line->AddChildToHorizontalBox(CountText);
		CountSlot->SetVerticalAlignment(VAlign_Center);
		CountSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));

		// The star sits ON the top-left border (hud.gd _set_tab_star: offset 1, -4). A drawn disc until the badge art
		// arrives with phase 6 (the font has no star glyph).
		Star = WidgetTree->ConstructWidget<UBorder>();
		Star->SetBrush(PawUi::Rounded(PawTokens::ALERT, PawTokens::INK, 2.0f, StarSize * 0.5f));
		Star->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* StarSlot = Root->AddChildToOverlay(Star);
		StarSlot->SetHorizontalAlignment(HAlign_Left);
		StarSlot->SetVerticalAlignment(VAlign_Top);
		StarSlot->SetPadding(FMargin(-15.0f, -10.0f, 0.0f, 0.0f));
		Star->SetPadding(FMargin(StarSize * 0.5f));
	}
	return Super::Initialize();
}

void UPawButton::SetLabel(const FText& Text)
{
	if (Label && !Label->GetText().EqualTo(Text))
	{
		Label->SetText(Text);
	}
}

FText UPawButton::GetLabel() const
{
	return Label ? Label->GetText() : FText::GetEmpty();
}

void UPawButton::SetLabelSize(double GodotPx)
{
	if (Label)
	{
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = PawUi::FontPt(GodotPx);
		Label->SetFont(Font);
	}
}

void UPawButton::SetLabelColour(bool bOverride, const FLinearColor& Colour, bool bAlsoWhenLocked)
{
	if (bColourOverride == bOverride && bColourWhenLocked == bAlsoWhenLocked && (!bOverride || OverrideColour == Colour))
	{
		return;
	}
	bColourOverride = bOverride;
	bColourWhenLocked = bAlsoWhenLocked;
	OverrideColour = Colour;
	ApplyLabelColour();
}

void UPawButton::SetWrapAt(float Width)
{
	for (UTextBlock* T : { Label.Get(), Detail.Get() })
	{
		if (T)
		{
			T->SetAutoWrapText(false);
			T->SetWrapTextAt(Width);
		}
	}
}

void UPawButton::SetDetail(const FText& Text, const FLinearColor& Colour)
{
	if (!Detail)
	{
		return;
	}
	const bool bShow = !Text.IsEmpty();
	Detail->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Detail->GetText().EqualTo(Text))
	{
		Detail->SetText(Text);
	}
	PawUi::SetTextColour(Detail, Colour);
}

FText UPawButton::GetDetail() const
{
	return Detail && Detail->GetVisibility() != ESlateVisibility::Collapsed ? Detail->GetText() : FText::GetEmpty();
}

void UPawButton::SetAlignLeft(bool bLeft)
{
	if (LineSlot)
	{
		LineSlot->SetHorizontalAlignment(bLeft ? HAlign_Left : HAlign_Center);
	}
	for (UTextBlock* T : { Label.Get(), Detail.Get() })
	{
		if (T)
		{
			T->SetJustification(bLeft ? ETextJustify::Left : ETextJustify::Center);
		}
	}
}

void UPawButton::SetFlat(bool bInFlat)
{
	bFlat = bInFlat;
	if (!bLockedState)
	{
		SetStyle(bFlat ? UPawFlatButtonStyle::StaticClass() : UPawButtonStyle::StaticClass());
	}
}

void UPawButton::SetLocked(bool bInLocked)
{
	if (bLockedState == bInLocked)
	{
		return;
	}
	bLockedState = bInLocked;
	SetIsInteractionEnabled(!bInLocked);
	SetStyle(bInLocked ? UPawLockedButtonStyle::StaticClass()
		: bFlat ? UPawFlatButtonStyle::StaticClass() : UPawButtonStyle::StaticClass());
	ApplyLabelColour();
}

void UPawButton::ApplyLabelColour()
{
	// A locked row reads disabled; a coloured one keeps its colour when it can be pressed.
	const bool bOverrideShows = bColourOverride && (!bLockedState || bColourWhenLocked);
	PawUi::SetTextColour(Label, bOverrideShows ? OverrideColour : bLockedState ? PawTokens::UI_TEXT_DISABLED : PawTokens::UI_TEXT);
}

void UPawButton::SetCheck(bool bShowBox, bool bTicked)
{
	bTickedState = bTicked;
	if (CheckBox)
	{
		CheckBox->SetVisibility(bShowBox ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		CheckBox->SetBrush(PawUi::Rounded(bTicked ? PawTokens::INK : PawTokens::UI_SLOT, PawTokens::INK, 2.0f, 4.0f));
	}
}

void UPawButton::SetPlate(bool bShow, const FLinearColor& Fill)
{
	if (Plate)
	{
		Plate->SetBrush(bShow ? PawUi::Rounded(Fill, Fill, 0.0f, Radius - 2.0f) : PawUi::NoBrush());
	}
}

void UPawButton::SetBadge(bool bShow, int64 Count)
{
	bBadge = bShow;
	if (Star)
	{
		Star->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (CountText)
	{
		CountText->SetVisibility(Count > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Count > 0)
		{
			CountText->SetText(PawText::Count(Count));
		}
	}
}
