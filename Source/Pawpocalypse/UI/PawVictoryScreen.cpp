#include "UI/PawVictoryScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

const FName UPawVictoryScreen::PauseHolder(TEXT("victory"));

namespace
{
	using namespace PawUi;

	// A label of the box: the box's full width, centred text (horizontal_alignment = CENTER).
	UTextBlock* BoxLabel(UWidgetTree* T, UVerticalBox* Box, double Px, bool bFirst, FName Name)
	{
		UTextBlock* Text = MakeText(T, Px, PawTokens::UI_TEXT, false, Name); // the theme's Label: ink (F21)
		Text->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* S = Box->AddChildToVerticalBox(Text);
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.0f, bFirst ? 0.0f : static_cast<float>(VictorySeparation), 0.0f, 0.0f));
		return Text;
	}
}

bool UPawVictoryScreen::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		UOverlay* Root = T->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The dim: MOUSE_FILTER_STOP on purpose, nothing behind the ending may be clicked while it is up.
		Dim = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
		Dim->SetBrush(Rounded(PawTokens::VICTORY_DIM, PawTokens::VICTORY_DIM, 0.0f, 0.0f));
		Dim->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);

		// The panel: the theme's WindowPanel (F21, designer 2026-11-03), anchored at the centre, 640 wide, as tall as its
		// column (grow both ways).
		USizeBox* Size = T->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(static_cast<float>(VictoryPanelWidth));
		Size->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
		SizeSlot->SetHorizontalAlignment(HAlign_Center);
		SizeSlot->SetVerticalAlignment(VAlign_Center);
		Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrush(WindowBrush());
		Panel->SetPadding(WindowPadding());
		Panel->SetVisibility(ESlateVisibility::Visible); // the plate stops clicks too
		Size->SetContent(Panel);
		UVerticalBox* Box = T->ConstructWidget<UVerticalBox>();
		Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Panel->SetContent(Box);

		Headline = BoxLabel(T, Box, VictoryHeadlinePx, true, TEXT("Headline"));
		Headline->SetText(PawText::Ui(TEXT("VICTORY_headline")));
		Flavour = BoxLabel(T, Box, LabelPx, false, TEXT("Flavour"));
		// AUTOWRAP_WORD_SMART at the column's width, the panel's less its margins (a fixed wrap width: KB U/09 §3).
		Flavour->SetAutoWrapText(false);
		Flavour->SetWrapTextAt(static_cast<float>(VictoryPanelWidth - 2.0 * WindowPadX));
		Flavour->SetText(PawText::Ui(TEXT("VICTORY_flavour")));
		Time = BoxLabel(T, Box, VictoryTimePx, false, TEXT("Time"));

		Ok = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("Ok"));
		Ok->SetMinDimensions(static_cast<int32>(VictoryOkWidth), static_cast<int32>(VictoryOkHeight));
		Ok->SetLabel(PawText::Ui(TEXT("VICTORY_ok")));
		UVerticalBoxSlot* OkSlot = Box->AddChildToVerticalBox(Ok);
		OkSlot->SetHorizontalAlignment(HAlign_Center); // SIZE_SHRINK_CENTER
		OkSlot->SetPadding(FMargin(0.0f, static_cast<float>(VictorySeparation), 0.0f, 0.0f));
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		OnLayoutReady();
	}
	return bOk;
}

void UPawVictoryScreen::OnLayoutReady()
{
	if (Ok)
	{
		Ok->OnClicked().AddWeakLambda(this, [this]() { Dismiss(); });
	}
}

UPawRealmSubsystem* UPawVictoryScreen::Realm() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
}

void UPawVictoryScreen::SetSeconds(double Seconds)
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("time"), PawText::Duration(Seconds));
	Time->SetText(PawText::Ui(TEXT("VICTORY_time"), Args));
}

UWidget* UPawVictoryScreen::NativeGetDesiredFocusTarget() const
{
	return Ok;
}

TOptional<FUIInputConfig> UPawVictoryScreen::GetDesiredInputConfig() const
{
	// A modal over a held field: the game's input stays out; OK is a Slate widget (no viewport capture).
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UPawVictoryScreen::NativeOnActivated()
{
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, true);
	}
	// MenuFocus.wire([[_ok]]): a row of one, wrapping onto itself; Common UI focuses OK after this.
	Wired = WireRows({ { Ok.Get() } });
	Super::NativeOnActivated();
}

void UPawVictoryScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, false);
	}
}

void UPawVictoryScreen::Dismiss()
{
	if (!IsActivated())
	{
		return; // dismiss(): `if not visible: return`
	}
	DeactivateWidget();
	TFunction<void()> Then = MoveTemp(OnDismissed);
	OnDismissed = nullptr;
	if (Then)
	{
		Then();
	}
}

FText UPawVictoryScreen::GetHeadlineText() const { return Headline ? Headline->GetText() : FText::GetEmpty(); }
FText UPawVictoryScreen::GetFlavourText() const { return Flavour ? Flavour->GetText() : FText::GetEmpty(); }
FText UPawVictoryScreen::GetTimeText() const { return Time ? Time->GetText() : FText::GetEmpty(); }
UWidget* UPawVictoryScreen::GetDim() const { return Dim; }
