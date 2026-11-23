#include "UI/PawReawakenCeremony.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/PawIcons.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

namespace
{
	using namespace PawUi;

	// Tween.TRANS_SINE with the default EASE_IN_OUT: -(cos(pi x) - 1) / 2.
	double SineInOut(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		return 0.5 - 0.5 * FMath::Cos(PI * X);
	}

	// A label centred in a rect given as Godot offsets about the stage's centre (anchors 0.5). bFit: the words shrink to the
	// rect when they are wider (the headline: "WIEDERERWACHEN" is wider than "REAWAKENING"; the language pass, 2026-11-20).
	UTextBlock* CentredLabel(UWidgetTree* T, UCanvasPanel* Canvas, double Px, const FLinearColor& Colour, double Top, double Bottom,
		FName Name, double HalfWidth = CeremonyTextHalfWidth, bool bFit = false)
	{
		UBorder* Box = T->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(Name.ToString() + TEXT("Box")));
		Box->SetBrush(NoBrush());
		Box->SetPadding(FMargin(0.0f));
		Box->SetHorizontalAlignment(HAlign_Center);
		Box->SetVerticalAlignment(VAlign_Center); // vertical_alignment = 1
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		UTextBlock* Text = MakeText(T, Px, Colour, false, Name);
		Text->SetJustification(ETextJustify::Center); // horizontal_alignment = 1
		if (bFit)
		{
			UScaleBox* Fit = T->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), FName(Name.ToString() + TEXT("Fit")));
			Fit->SetStretch(EStretch::ScaleToFitX); // by width alone: a display face's line runs a little over its box
			Fit->SetStretchDirection(EStretchDirection::DownOnly);
			Fit->SetVisibility(ESlateVisibility::HitTestInvisible);
			Fit->SetContent(Text);
			Box->SetContent(Fit);
		}
		else
		{
			Box->SetContent(Text);
		}
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Box);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAutoSize(false);
		S->SetOffsets(FMargin(static_cast<float>(-HalfWidth), static_cast<float>(Top),
			static_cast<float>(2.0 * HalfWidth), static_cast<float>(Bottom - Top)));
		return Text;
	}

	// The concept's ceremony pictures (tools/art/sticker_art.py write_ceremony): its rays, and its cat on its rim.
	const TCHAR* const RaysIcon = TEXT("ui_icon_ceremony_rays");
	const TCHAR* const CatIcon = TEXT("ui_icon_ceremony_cat");
}

bool UPawReawakenCeremony::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		// mouse_filter = 2 on every node: the ceremony never takes a click, the game under it runs and plays on.
		UCanvasPanel* Canvas = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Canvas;

		Flash = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Flash"));
		// D18 (S4): the concept's rays, halo and sparks fill the screen (Godot: a flat flash, CEREMONY_FLASH, kept as the
		// fallback when the picture is missing).
		const FSlateBrush Rays = PawUi::IconBrush(RaysIcon, 1920.0);
		Flash->SetBrush(Rays.GetResourceObject() ? Rays : Rounded(PawTokens::CEREMONY_FLASH, PawTokens::CEREMONY_FLASH, 0.0f, 0.0f));
		Flash->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* FlashSlot = Canvas->AddChildToCanvas(Flash);
		FlashSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FlashSlot->SetOffsets(FMargin(0.0f));

		// The cat in the middle, the title above it as a sticker, the payout on a card-stock panel below (the concept's), on a
		// stage of their size that shrinks to the screen when it is shorter (the language pass, 2026-11-20: at 150 % UI the
		// screen is 720 tall and they reach 850: the title went off the top, the payout off the bottom, in every language).
		UScaleBox* Stage = T->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("Stage"));
		Stage->SetStretch(EStretch::ScaleToFit);
		Stage->SetStretchDirection(EStretchDirection::DownOnly);
		Stage->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* StageSlot = Canvas->AddChildToCanvas(Stage);
		StageSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		StageSlot->SetOffsets(FMargin(0.0f));
		USizeBox* StageSize = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StageSize"));
		StageSize->SetWidthOverride(static_cast<float>(CeremonyStageWidth));
		StageSize->SetHeightOverride(static_cast<float>(CeremonyStageHeight));
		StageSize->SetVisibility(ESlateVisibility::HitTestInvisible);
		Stage->SetContent(StageSize);
		UCanvasPanel* StageCanvas = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StageCanvas"));
		StageCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
		StageSize->SetContent(StageCanvas);
		Cat = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Cat"));
		Cat->SetBrush(PawUi::IconBrush(CatIcon, CeremonyCatPx));
		Cat->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* CatSlot = StageCanvas->AddChildToCanvas(Cat);
		CatSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CatSlot->SetAutoSize(false);
		CatSlot->SetOffsets(FMargin(static_cast<float>(-0.5 * CeremonyCatPx), static_cast<float>(CeremonyCatTop),
			static_cast<float>(CeremonyCatPx), static_cast<float>(CeremonyCatPx)));
		Headline = CentredLabel(T, StageCanvas, CeremonyHeadlinePx, PawTokens::UI_SELECTED, CeremonyHeadlineTop, CeremonyHeadlineBottom,
			TEXT("Headline"), CeremonyTextHalfWidth, true);
		ButterTitle(Headline, CeremonyTitleOutline);
		Payout = CentredLabel(T, StageCanvas, CeremonyPayoutPx, PawTokens::UI_TEXT, CeremonyPayoutTop, CeremonyPayoutBottom, TEXT("Payout"),
			CeremonyPayoutHalfWidth);
		if (UBorder* PayoutBox = Cast<UBorder>(Payout->GetParent()))
		{
			PayoutBox->SetBrush(Chrome(EPawChrome::Panel));
			PayoutBox->SetPadding(PlatePadding(FMargin(24.0f, 10.0f, 24.0f, 10.0f), EPawChrome::Panel));
		}
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		PawUi::UseAtlasIcons(this); // the asset's saved icon brushes: the atlas's cells (one batch)
	}
	// visible = false until play(). Not while baking: the asset keeps the layout drawn, so the designer sees it.
	if (bOk && PawUi::ShouldWire(this))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		Apply(FAlphas());
	}
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
	// D18: the payout fades with its panel (the text's box), the cat with the flash.
	UWidget* PayoutShown = Payout->GetParent() ? static_cast<UWidget*>(Payout->GetParent()) : static_cast<UWidget*>(Payout);
	PayoutShown->SetRenderOpacity(A.Payout);
	if (Cat)
	{
		Cat->SetRenderOpacity(A.Flash);
	}
}

UPawReawakenCeremony::FAlphas UPawReawakenCeremony::GetDrawnAlphas() const
{
	FAlphas A;
	A.Flash = Flash ? Flash->GetRenderOpacity() : 0.0f;
	A.Headline = Headline ? Headline->GetRenderOpacity() : 0.0f;
	const UWidget* PayoutShown = Payout && Payout->GetParent() ? static_cast<const UWidget*>(Payout->GetParent()) : Payout.Get();
	A.Payout = PayoutShown ? PayoutShown->GetRenderOpacity() : 0.0f;
	return A;
}

bool UPawReawakenCeremony::HasRays() const
{
	const UObject* Tex = Flash ? Flash->Background.GetResourceObject() : nullptr;
	return Tex && Tex->GetName() == FString(TEXT("T_")) + RaysIcon;
}

bool UPawReawakenCeremony::IsCatShown() const
{
	return Cat && Cat->GetVisibility() != ESlateVisibility::Collapsed && Cat->GetBrush().GetResourceObject() != nullptr;
}

FText UPawReawakenCeremony::GetHeadlineText() const { return Headline ? Headline->GetText() : FText::GetEmpty(); }
FText UPawReawakenCeremony::GetPayoutText() const { return Payout ? Payout->GetText() : FText::GetEmpty(); }
