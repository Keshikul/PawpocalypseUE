#include "UI/PawConfirmDialog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/PawInputMap.h"
#include "UI/PawButton.h"
#include "UI/PawIcons.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	// The title's confirm panel's measures (main_menu.tscn ConfirmPanel), a little wider for "Save and quit".
	constexpr double ConfirmWidth = PawUi::TitleConfirmWidth;
	constexpr double ConfirmPromptPx = 22.0;
	constexpr double ConfirmButtonWidth = PawUi::TitleConfirmButtonWidth * 1.4;
	constexpr double ConfirmButtonHeight = PawUi::TitleConfirmButtonHeight * 1.2;
}

bool UPawConfirmDialog::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		using namespace PawUi;
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		UOverlay* Root = T->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The dim: what is under it is out of play until the question is answered; it takes every click (and answers none).
		Dim = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
		Dim->SetBrush(Rounded(PawTokens::INK.CopyWithNewOpacity(static_cast<float>(PickDimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		Dim->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);

		USizeBox* Size = T->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(static_cast<float>(ConfirmWidth));
		Size->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
		SizeSlot->SetHorizontalAlignment(HAlign_Center);
		SizeSlot->SetVerticalAlignment(VAlign_Center);
		Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrush(WindowBrush());
		Panel->SetPadding(WindowPadding());
		Panel->SetVisibility(ESlateVisibility::Visible);
		Size->SetContent(Panel);

		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>();
		Body->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Panel->SetContent(Body);
		PromptText = MakeText(T, ConfirmPromptPx, PawTokens::UI_TEXT, false, TEXT("PromptText"));
		PromptText->SetJustification(ETextJustify::Center);
		PromptText->SetAutoWrapText(false);
		PromptText->SetWrapTextAt(static_cast<float>(ConfirmWidth - 2.0 * TitlePanelPadX));
		UVerticalBoxSlot* PromptSlot = Body->AddChildToVerticalBox(PromptText);
		PromptSlot->SetHorizontalAlignment(HAlign_Center);

		UHorizontalBox* Answers = T->ConstructWidget<UHorizontalBox>();
		Answers->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UVerticalBoxSlot* AnswersSlot = Body->AddChildToVerticalBox(Answers);
		AnswersSlot->SetHorizontalAlignment(HAlign_Center);
		AnswersSlot->SetPadding(FMargin(0.0f, static_cast<float>(TitleConfirmSeparation), 0.0f, 0.0f));
		auto Answer = [&](const TCHAR* Name, bool bFirst)
		{
			UPawButton* B = T->ConstructWidget<UPawButton>(Resolve<UPawButton>(), Name);
			B->SetMinDimensions(static_cast<int32>(ConfirmButtonWidth), static_cast<int32>(ConfirmButtonHeight));
			Answers->AddChildToHorizontalBox(B)->SetPadding(FMargin(bFirst ? 0.0f : static_cast<float>(TitleConfirmRowSeparation), 0.0f,
				0.0f, 0.0f));
			return B;
		};
		YesButton = Answer(TEXT("YesButton"), true);
		NoButton = Answer(TEXT("NoButton"), false);
	}
	const bool bOk = Super::Initialize();
	// The keys stay here when a click lands on the dim (KB U/24: a panel with nothing focusable under the click hands the keys
	// to the viewport).
	SetIsFocusable(true);
	if (bOk && PawUi::ShouldWire(this))
	{
		PawUi::UseAtlasIcons(this); // the asset's saved plate brushes: the atlas's cells (one batch)
		OnLayoutReady();
	}
	return bOk;
}

void UPawConfirmDialog::OnLayoutReady()
{
	if (YesButton)
	{
		YesButton->OnClicked().AddWeakLambda(this, [this]() { PressYes(); });
	}
	if (NoButton)
	{
		NoButton->OnClicked().AddWeakLambda(this, [this]() { PressNo(); });
	}
}

UPawConfirmDialog* UPawConfirmDialog::Open(UCommonActivatableWidgetStack* Modal, const FText& Prompt, const FText& Yes, const FText& No,
	TFunction<void()> InOnYes, TFunction<void()> InOnNo)
{
	if (!Modal)
	{
		return nullptr;
	}
	return Modal->AddWidget<UPawConfirmDialog>(PawUi::Resolve<UPawConfirmDialog>(), [&](UPawConfirmDialog& D)
	{
		D.Ask(Prompt, Yes, No, MoveTemp(InOnYes), MoveTemp(InOnNo));
	});
}

void UPawConfirmDialog::Ask(const FText& Prompt, const FText& Yes, const FText& No, TFunction<void()> InOnYes, TFunction<void()> InOnNo)
{
	bAnswered = false;
	OnYes = MoveTemp(InOnYes);
	OnNo = MoveTemp(InOnNo);
	if (PromptText)
	{
		PromptText->SetText(Prompt);
	}
	if (YesButton)
	{
		YesButton->SetLabel(Yes);
	}
	if (NoButton)
	{
		NoButton->SetLabel(No);
	}
}

void UPawConfirmDialog::PressYes() { Answer(true); }
void UPawConfirmDialog::PressNo() { Answer(false); }

void UPawConfirmDialog::Answer(bool bYes)
{
	bAnswered = true;
	// The first answer counts: both are let go here, so a second press (or the other button) finds nothing to run. Closed
	// first, then the answer runs: a yes that opens another screen on this stack (the job picker after the tree) finds it
	// empty.
	TFunction<void()> Then = MoveTemp(bYes ? OnYes : OnNo);
	OnYes = nullptr;
	OnNo = nullptr;
	if (IsActivated())
	{
		DeactivateWidget();
	}
	if (Then)
	{
		Then();
	}
}

UWidget* UPawConfirmDialog::NativeGetDesiredFocusTarget() const
{
	return NoButton ? static_cast<UWidget*>(NoButton) : static_cast<UWidget*>(YesButton); // the safe answer under the key
}

TOptional<FUIInputConfig> UPawConfirmDialog::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

FReply UPawConfirmDialog::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (!KeyEvent.IsRepeat() && PawInput::KeyIs(KeyEvent.GetKey(), TEXT("back")))
	{
		PressNo();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

FText UPawConfirmDialog::GetPrompt() const { return PromptText ? PromptText->GetText() : FText::GetEmpty(); }
FText UPawConfirmDialog::GetYesLabel() const { return YesButton ? YesButton->GetLabel() : FText::GetEmpty(); }
FText UPawConfirmDialog::GetNoLabel() const { return NoButton ? NoButton->GetLabel() : FText::GetEmpty(); }
