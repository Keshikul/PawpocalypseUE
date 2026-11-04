#include "UI/PawReawakenCeremony.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"

namespace
{
	using namespace PawUi;

	// Tween.TRANS_SINE with the default EASE_IN_OUT: -(cos(pi x) - 1) / 2.
	double SineInOut(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		return 0.5 - 0.5 * FMath::Cos(PI * X);
	}

	// A label centred in a rect given as Godot offsets about the screen's centre (anchors 0.5).
	UTextBlock* CentredLabel(UWidgetTree* T, UCanvasPanel* Canvas, double Px, const FLinearColor& Colour, double Top, double Bottom)
	{
		UBorder* Box = T->ConstructWidget<UBorder>();
		Box->SetBrush(NoBrush());
		Box->SetPadding(FMargin(0.0f));
		Box->SetHorizontalAlignment(HAlign_Center);
		Box->SetVerticalAlignment(VAlign_Center); // vertical_alignment = 1
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		UTextBlock* Text = MakeText(T, Px, Colour);
		Text->SetJustification(ETextJustify::Center); // horizontal_alignment = 1
		Box->SetContent(Text);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Box);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAutoSize(false);
		S->SetOffsets(FMargin(static_cast<float>(-CeremonyTextHalfWidth), static_cast<float>(Top),
			static_cast<float>(2.0 * CeremonyTextHalfWidth), static_cast<float>(Bottom - Top)));
		return Text;
	}
}

bool UPawReawakenCeremony::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		// mouse_filter = 2 on every node: the ceremony never takes a click, the game under it runs and plays on.
		UCanvasPanel* Canvas = T->ConstructWidget<UCanvasPanel>();
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Canvas;

		Flash = T->ConstructWidget<UBorder>();
		Flash->SetBrush(Rounded(PawTokens::CEREMONY_FLASH, PawTokens::CEREMONY_FLASH, 0.0f, 0.0f));
		Flash->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* FlashSlot = Canvas->AddChildToCanvas(Flash);
		FlashSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FlashSlot->SetOffsets(FMargin(0.0f));

		Headline = CentredLabel(T, Canvas, CeremonyHeadlinePx, PawTokens::CEREMONY_HEADLINE, CeremonyHeadlineTop, CeremonyHeadlineBottom);
		Payout = CentredLabel(T, Canvas, CeremonyPayoutPx, PawTokens::CEREMONY_PAYOUT, CeremonyPayoutTop, CeremonyPayoutBottom);
	}
	const bool bOk = Super::Initialize();
	// visible = false until play().
	SetVisibility(ESlateVisibility::Collapsed);
	Apply(FAlphas());
	return bOk;
}

double UPawReawakenCeremony::Duration()
{
	return CeremonyFlashInSec + CeremonyPayoutInSec + CeremonyHoldSec + CeremonyFadeOutSec;
}

UPawReawakenCeremony::FAlphas UPawReawakenCeremony::AlphasAt(double T)
{
	FAlphas A;
	const double PayoutAt = CeremonyFlashInSec;
	const double HoldAt = PayoutAt + CeremonyPayoutInSec;
	const double FadeAt = HoldAt + CeremonyHoldSec;
	if (T < 0.0 || T >= Duration())
	{
		return A; // before, and after the sequence: everything at 0 (where each tween leaves it)
	}
	if (T < PayoutAt)
	{
		A.Flash = A.Headline = static_cast<float>(SineInOut(T / CeremonyFlashInSec));
		return A;
	}
	A.Flash = A.Headline = 1.0f;
	if (T < HoldAt)
	{
		A.Payout = static_cast<float>(SineInOut((T - PayoutAt) / CeremonyPayoutInSec));
		return A;
	}
	A.Payout = 1.0f;
	if (T >= FadeAt)
	{
		A.Flash = A.Headline = A.Payout = static_cast<float>(1.0 - SineInOut((T - FadeAt) / CeremonyFadeOutSec));
	}
	return A;
}

void UPawReawakenCeremony::Play(double Stardust, int64 ReawakenCount)
{
	Headline->SetText(PawText::Ui(TEXT("CEREMONY_headline")));
	FFormatNamedArguments Args;
	Args.Add(TEXT("n"), PawText::Count(ReawakenCount));
	// floori(stardust): whole Stardust, as Godot prints the int.
	Args.Add(TEXT("stardust"), PawText::Count(static_cast<int64>(FMath::FloorToDouble(Stardust))));
	Payout->SetText(PawText::Ui(TEXT("CEREMONY_payout"), Args));
	// A replay restarts the tween (Godot kills the running one).
	Elapsed = 0.0;
	bPlaying = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	Apply(AlphasAt(0.0));
}

bool UPawReawakenCeremony::Step(double DeltaSeconds)
{
	if (!bPlaying)
	{
		return false;
	}
	Elapsed += FMath::Max(0.0, DeltaSeconds);
	if (Elapsed >= Duration())
	{
		// _on_sequence_done(): hidden, then `finished`.
		bPlaying = false;
		Apply(FAlphas());
		SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}
	Apply(AlphasAt(Elapsed));
	return false;
}

void UPawReawakenCeremony::Apply(const FAlphas& A)
{
	// modulate:a on each node: the flash's own fill and each label as a whole.
	Flash->SetRenderOpacity(A.Flash);
	Headline->SetRenderOpacity(A.Headline);
	Payout->SetRenderOpacity(A.Payout);
}

UPawReawakenCeremony::FAlphas UPawReawakenCeremony::GetDrawnAlphas() const
{
	FAlphas A;
	A.Flash = Flash ? Flash->GetRenderOpacity() : 0.0f;
	A.Headline = Headline ? Headline->GetRenderOpacity() : 0.0f;
	A.Payout = Payout ? Payout->GetRenderOpacity() : 0.0f;
	return A;
}

FText UPawReawakenCeremony::GetHeadlineText() const { return Headline ? Headline->GetText() : FText::GetEmpty(); }
FText UPawReawakenCeremony::GetPayoutText() const { return Payout ? Payout->GetText() : FText::GetEmpty(); }
