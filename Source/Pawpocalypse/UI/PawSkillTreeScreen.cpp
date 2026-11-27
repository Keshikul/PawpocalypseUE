#include "UI/PawSkillTreeScreen.h"
#include "UI/PawUiTime.h"
#include "UI/PawUiMemory.h"

#include "UI/PawIcons.h"

#include "Components/Image.h"

#include "Input/PawInputMap.h"
#include "Save/PawSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "PcData.h"
#include "PcTree.h"
#include "PcTreeText.h"
#include "Realm/PawRealmSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "UI/PawBoba.h"
#include "UI/PawButton.h"
#include "UI/PawPrompts.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawTreeTables.h"
#include "UI/PawUiConstants.h"
#include "Render/PawFxCanvas.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "UI/PawTreeSky.h"
#include "UI/PawUnicornCeremony.h"
#include "UI/PawConfirmDialog.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

const FName UPawSkillTreeScreen::PauseHolder(TEXT("tree"));

namespace
{
	using namespace PawUi;
	using Pc::PcTreeNodeState;

	// The tooltip's digest (review CR9): everything its lines are made from, but the node.
	bool SameTip(const Pc::PcTreeTip& A, const Pc::PcTreeTip& B)
	{
		return A.bKnown == B.bKnown && A.bUnlock == B.bUnlock && A.Owned == B.Owned && A.Max == B.Max && A.bMaxed == B.bMaxed
			&& A.bShowNow == B.bShowNow && A.Next == B.Next && A.Cost == B.Cost && A.Reason == B.Reason && A.Rival == B.Rival
			&& A.Parent == B.Parent && A.Gate == B.Gate && A.Have == B.Have && A.bBuyPrompt == B.bBuyPrompt;
	}

	// skill_tree_ui.gd / skill_tree.tscn / upgrade_tooltip.gd / skill_tree_connector_layer.gd (presentation constants).
	// The Compass concept's (C1, 2026-11-26: its drawSkill, drawLinks, drawRoot, drawLabels), in canvas px at zoom 1.
	constexpr double NodePx = Pc::TreeLayout::NodeSize;  // a skill's square (84)
	constexpr double JobScale = 1.25;                   // a job's diamond stands on a box this much bigger
	constexpr double IconShare = 0.72, JobIconShare = 0.62; // the picture's share of its node's box
	constexpr double ContentTop = 130.0;                // ContentArea offset_top
	constexpr double HubRadius = Pc::TreeLayout::HubSize * 0.5; // the hub's yellow disc
	constexpr double HubIconPx = 86.0;                  // the paw on it
	constexpr double HubNamePx = 30.0;                  // its name under it, on a plate (a link runs under it)
	constexpr double HubNameGap = 12.0;
	constexpr double BranchNamePx = 30.0;               // a branch's name in the field
	constexpr double BranchNameOutline = 4.0;
	constexpr double NameFloorPx = 16.0;                // the names never smaller than this on screen (the text floor)
	constexpr double BranchNameHalfW = 90.0, BranchNameHalfH = 22.0; // the room a name takes in the fit
	constexpr double HaloPad = 16.0;                    // HALO_PAD
	constexpr double HaloAlphaLow = 0.55, HaloAlphaHigh = 1.0; // HALO_ALPHA
	constexpr double HaloPeriodSec = 1.6;               // HALO_PERIOD_SEC
	constexpr double HaloGlowAlpha = 0.30;              // glow.bg_color = Color(Palette.GOLD, 0.30)
	constexpr double ShinePeriodSec = 1.1;              // SHINE_PERIOD_SEC
	constexpr double CounterPx = 16.0;                  // counter font_size
	constexpr double ChipPx = 16.0;                     // the level and price chips' words (the text floor)
	constexpr double ChipStarPx = 18.0;                 // the price chip's star
	constexpr double CheckRadius = 12.0;                // an owned one-time skill's tick
	constexpr double PriceGap = 4.0;                    // between the node's box and its price chip
	constexpr double ChipFloorPx = 16.0;                // a chip's words never smaller than this on screen (the text floor):
	                                                    // the tree's fit zoom is about 0.55 at 1080p (the tour, 2026-11-26)
	constexpr double ShadowX = 6.0, ShadowY = 7.0;      // a sticker's hard shadow
	constexpr double CloseBrowseW = 60.0, CloseBrowseH = 36.0;   // CLOSE_BROWSE_SIZE
	constexpr double CloseBrowseMarginX = 20.0, CloseBrowseMarginY = 60.0; // CLOSE_BROWSE_MARGIN
	constexpr double CloseSpendW = 420.0, CloseSpendH = 92.0;    // CLOSE_SPEND_SIZE
	constexpr double CloseSpendGapBottom = 96.0;        // D18 (S4): above the legend (bottom-left now; Godot: 40, CLOSE_SPEND_GAP_BOTTOM)
	constexpr double CloseSpendPx = 34.0;               // the Continue caption's font_size
	constexpr double ZoomButtonPx = 44.0;               // ZoomIn custom_minimum_size
	constexpr double ZoomBoxX = 24.0, ZoomBoxY = 150.0; // ZoomControls position
	constexpr double TipWidth = 380.0;                  // UpgradeTooltip WIDTH_PX
	constexpr double TipMargin = 14.0;                  // MARGIN_PX
	constexpr double TipTitlePx = 20.0;                 // its title font_size
	constexpr double TipDockX = 24.0, TipDockY = 72.0;  // TIP_DOCK_INSET
	constexpr double TreePromptPx = 22.0;               // legend.icon_px; the tooltip's bbcode(..., 22)
	constexpr double FollowBox = 0.6;                   // FOLLOW_BOX
	constexpr double FollowSec = 0.18;                  // FOLLOW_SEC
	constexpr double PanPxPerSec = 1400.0;              // PAN_PX_PER_SEC
	constexpr double ZoomPerSec = 1.2;                  // ZOOM_PER_SEC
	constexpr double LinkUnderWidth = 15.0, LinkCoreWidth = 6.5; // a link's ink road and its core
	constexpr double LinkDash = 14.0, LinkGap = 9.0;    // the core of a link to a skill on sale now
	constexpr double CloseSpendIconPx = 40.0;           // D18 (S4): the Reawaken swirl on Continue
	constexpr double RainbowInset = 7.0, RainbowWidth = 5.0; // the Unicorn's rings
	constexpr int32 RainbowSegments = 10;
	constexpr double RainbowTurnsPerSec = 0.12;
	constexpr int32 PulseRings = 3;                     // PULSE_RINGS
	constexpr double PulseStep = 13.0, PulseWidth = 6.0, PulseHz = 0.8; // PULSE_*

	std::string Utf8(const FString& S) { return std::string(TCHAR_TO_UTF8(*S)); }

	// A trunk's look (C1); an unknown trunk wears the first's.
	const PawTree::FTrunkLook& TrunkLook(const FString& TrunkId)
	{
		for (const PawTree::FTrunkLook& L : PawTree::TrunkLooks)
		{
			if (TrunkId == L.Id)
			{
				return L;
			}
		}
		return PawTree::TrunkLooks[0];
	}

	bool InList(const FString& Id, const TCHAR* const* List, int32 Count)
	{
		for (int32 I = 0; I < Count; ++I)
		{
			if (Id == List[I])
			{
				return true;
			}
		}
		return false;
	}

	// FPawFxCanvas takes sRGB floats (straight alpha); the tokens are linear. The widget's tint (a node's shine, a faint
	// node's opacity) multiplies in.
	FLinearColor ToCanvas(const FLinearColor& Linear, const FWidgetStyle& Style)
	{
		const FLinearColor T = Linear * Style.GetColorAndOpacityTint();
		const FColor S = T.ToFColorSRGB();
		return FLinearColor(S.R / 255.0f, S.G / 255.0f, S.B / 255.0f, T.A);
	}

	// A rounded rectangle W x H about C, its corners R round (a chip, a skill's square), clockwise from the top right, open.
	TArray<FVector2D> RoundRectPath(const FVector2D& C, double W, double H, double R)
	{
		R = FMath::Min(R, FMath::Min(W, H) * 0.5);
		const double HX = W * 0.5 - R, HY = H * 0.5 - R;
		const FVector2D Corners[4] = { FVector2D(HX, -HY), FVector2D(HX, HY), FVector2D(-HX, HY), FVector2D(-HX, -HY) };
		TArray<FVector2D> P;
		for (int32 K = 0; K < 4; ++K)
		{
			for (int32 I = 0; I <= 5; ++I)
			{
				const double A = UE_DOUBLE_HALF_PI * (K - 1) + UE_DOUBLE_HALF_PI * I / 5.0;
				P.Add(C + Corners[K] + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
			}
		}
		return P;
	}

	FLinearColor WithAlpha(FLinearColor C, double A)
	{
		C.A = static_cast<float>(A);
		return C;
	}

	FVector2D V(const Pc::PcVec2& P) { return FVector2D(P.X, P.Y); }

	void Lines(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const TArray<FVector2D>& Points, const FLinearColor& C, double W)
	{
		FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Points, ESlateDrawEffect::None, C, true, static_cast<float>(W));
	}
}

// --- links ------------------------------------------------------------------------------------------------------------

TArray<FVector2D> UPawTreeLinks::RingPath(const FVector2D& C, const FVector2D& Half)
{
	// C1: a circle round the node (the Unicorn is a one-time unlock: a circle), from the top, clockwise, closed. (D18's
	// hexagon until 2026-11-26.)
	TArray<FVector2D> Pts;
	const int32 Cuts = RainbowSegments * 6;
	for (int32 I = 0; I < Cuts; ++I)
	{
		const double A = UE_DOUBLE_TWO_PI * I / Cuts - UE_DOUBLE_HALF_PI;
		Pts.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Half.Y);
	}
	const FVector2D First = Pts[0]; // a copy: adding an element of the array to itself may reallocate under it
	Pts.Add(First);
	return Pts;
}

int32 UPawTreeLinks::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Culling, FSlateWindowElementList& Out,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, G, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	// C1 (the Compass concept's drawLinks): an ink road under every shown link first; then the cores, unowned first, so an
	// owned branch is never crossed by a dim line drawn over it (two passes, as Godot). The core: the trunk's colour where
	// owned, dashed cream to a skill on sale now, dim where not yet, darker where shut. A hidden skill's link is not drawn;
	// one ring ahead, faint.
	for (const FLink& L : Links)
	{
		if (L.Alpha > 0.0f)
		{
			Lines(Out, Layer, G, TArray<FVector2D>{ L.A, L.B }, WithAlpha(PawTree::LinkUnder, L.Alpha), LinkUnderWidth);
		}
	}
	for (const bool bPassOwned : { false, true })
	{
		for (const FLink& L : Links)
		{
			if (L.Alpha <= 0.0f || L.bOwned != bPassOwned)
			{
				continue;
			}
			const TArray<FVector2D> Seg{ L.A, L.B };
			if (L.bShut)
			{
				Lines(Out, Layer, G, Seg, WithAlpha(PawTree::LinkShut, PawTree::LinkShut.A * L.Alpha), LinkCoreWidth * 0.7);
			}
			else if (L.bOwned)
			{
				Lines(Out, Layer, G, Seg, WithAlpha(L.Colour, L.Alpha), LinkCoreWidth);
			}
			else if (L.bNext)
			{
				const FVector2D D = L.B - L.A;
				const double Len = D.Size();
				const FVector2D U = Len > 0.0 ? D / Len : FVector2D::ZeroVector;
				for (double T = 0.0; T < Len; T += LinkDash + LinkGap)
				{
					Lines(Out, Layer, G, TArray<FVector2D>{ L.A + U * T, L.A + U * FMath::Min(T + LinkDash, Len) },
						WithAlpha(PawTree::LinkNext, L.Alpha), LinkCoreWidth);
				}
			}
			else
			{
				Lines(Out, Layer, G, Seg, WithAlpha(PawTree::LinkOff, L.Alpha), LinkCoreWidth);
			}
		}
	}
	// The hub (the concept's drawRoot): a hard shadow, a die-cut rim, ink, the yellow disc; its paw and name stand on it.
	if (bHub)
	{
		FPawFxCanvas Disc;
		Disc.Circle(HubAt + FVector2D(ShadowX, ShadowY), HubRadius + 6.0, ToCanvas(PawTree::NodeShadow, Style));
		Disc.Circle(HubAt, HubRadius + 6.0, ToCanvas(PawTree::NodeRim, Style));
		Disc.Circle(HubAt, HubRadius + 2.0, ToCanvas(PawTokens::INK, Style));
		Disc.Circle(HubAt, HubRadius, ToCanvas(PawTokens::UI_TREE_READY_RING, Style));
		PawUi::PaintCanvas(Disc, Out, Layer + 1, G.GetAccumulatedRenderTransform());
	}
	const int32 RingLayer = Layer + 2;
	const FVector2D Half = FVector2D(NodePx * 0.5 + RainbowInset);
	auto DrawRainbow = [&](const FVector2D& C, const FVector2D& H, double Width, double Alpha)
	{
		const TArray<FVector2D> Pts = RingPath(C, H);
		TArray<FLinearColor> Colours;
		for (int32 I = 0; I < Pts.Num(); ++I)
		{
			const double Hue = FMath::Fmod(static_cast<double>(I) / (Pts.Num() - 1) + RainbowPhase, 1.0);
			FLinearColor Col = FLinearColor::MakeFromHSV8(static_cast<uint8>(Hue * 255.0), static_cast<uint8>(0.85 * 255.0), 255);
			Col.A = static_cast<float>(Alpha);
			Colours.Add(Col);
		}
		FSlateDrawElement::MakeLines(Out, RingLayer, G.ToPaintGeometry(), Pts, Colours, ESlateDrawEffect::None, FLinearColor::White, true, static_cast<float>(Width));
	};
	// The pulse (the Unicorn): rings breathing outward behind its own.
	const double Breath = 0.5 + 0.5 * FMath::Sin(PulseClock * UE_DOUBLE_TWO_PI * PulseHz);
	for (const FVector2D& C : Pulse)
	{
		for (int32 K = PulseRings; K >= 1; --K)
		{
			const double Grow = (K * PulseStep) * (0.55 + 0.45 * Breath);
			DrawRainbow(C, Half + FVector2D(Grow), PulseWidth, (1.0 - static_cast<double>(K) / (PulseRings + 1)) * (0.35 + 0.5 * Breath));
		}
	}
	for (const FVector2D& C : Pulse)
	{
		DrawRainbow(C, Half, RainbowWidth, 1.0); // its own ring, in every state it shows in
	}
	return RingLayer;
}

// --- a node -------------------------------------------------------------------------------------------------------------

UPawTreeNodeStyle::UPawTreeNodeStyle()
{
	// The frame is the node's own border; the button draws only the selection ring (focus = selection, docs/05 §2 rule 10).
	NormalBase = NoBrush();
	NormalHovered = Rounded(FLinearColor::Transparent, PawTokens::UI_FOCUS, FocusRingWidth, 6.0f);
	NormalPressed = NormalHovered;
	SelectedBase = NormalBase;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalHovered;
	Disabled = NormalBase;
	ButtonPadding = FMargin(0.0f);
	CustomPadding = FMargin(0.0f);
}

UPawTreeNode::UPawTreeNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetStyle(UPawTreeNodeStyle::StaticClass());
}

bool UPawTreeNode::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
		Box->SetWidthOverride(static_cast<float>(NodePx));
		Box->SetHeightOverride(static_cast<float>(NodePx));
		WidgetTree->RootWidget = Box;
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Box->SetContent(Root);
		// D18 (S4): the hexagon first, under everything (the concept's treeNode).
		Plate = WidgetTree->ConstructWidget<UPawHexPlate>(PawUi::Resolve<UPawHexPlate>(), TEXT("Plate"));
		UOverlaySlot* PS = Root->AddChildToOverlay(Plate);
		PS->SetHorizontalAlignment(HAlign_Fill);
		PS->SetVerticalAlignment(VAlign_Fill);
		Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Frame"));
		Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* FS = Root->AddChildToOverlay(Frame);
		FS->SetHorizontalAlignment(HAlign_Fill);
		FS->SetVerticalAlignment(VAlign_Fill);
		// The branch's icon, in the middle (skill_tree_ui.gd ICON_SIZE 56), under the counter.
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* IS = Root->AddChildToOverlay(Icon);
		IS->SetHorizontalAlignment(HAlign_Center);
		IS->SetVerticalAlignment(VAlign_Center);
		// Ladders carry "3/10" in the bottom right; unlocks carry nothing.
		CounterText = Heavy(MakeText(WidgetTree, CounterPx, FLinearColor::White, false, TEXT("CounterText")));
		UOverlaySlot* CS = Root->AddChildToOverlay(CounterText);
		CS->SetHorizontalAlignment(HAlign_Right);
		CS->SetVerticalAlignment(VAlign_Bottom);
		CS->SetPadding(FMargin(4.0f, 0.0f, 6.0f, 4.0f));
	}
	const bool bOk = Super::Initialize();
	SetRenderTransformPivot(FVector2D(0.5, 0.5));
	if (bOk && PawUi::ShouldWire(this))
	{
		OnNodeReady();
	}
	return bOk;
}

void UPawTreeNode::SetIconPath(const FString& Path, double Px)
{
	if (Icon)
	{
		const FSlateBrush B = PawUi::IconBrush(Path, Px);
		Icon->SetVisibility(B.GetResourceObject() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Icon->SetBrush(B);
	}
}

FString UPawTreeNode::GetIconName() const
{
	const UObject* Tex = Icon && Icon->GetVisibility() != ESlateVisibility::Collapsed ? Icon->GetBrush().GetResourceObject() : nullptr;
	return Tex ? Tex->GetName() : FString();
}

void UPawTreeNode::OnNodeReady()
{
	// The look in play: BP_TreeNodeStyle once baked.
	if (!GetStyle() || GetStyle()->GetClass() == UPawTreeNodeStyle::StaticClass())
	{
		SetStyle(PawUi::Resolve(UPawTreeNodeStyle::StaticClass()));
	}
	StardustBrush = PawUi::IconBrush(PawUi::CurrencyIconName(TEXT("stardust")), ChipStarPx); // the price chip's star
}

void UPawTreeNode::FrameColours(PcTreeNodeState S, const FLinearColor& Card, FLinearColor& OutFill, FLinearColor& OutEdge)
{
	// C1 (the Compass concept's drawSkill; designer 2026-11-26: "Darker cards for the skill menu"): owned, the trunk's card
	// on an ink edge; to buy now, dark in the gold ring; the rest dark on a darker edge. (D18 until then: owned green, the
	// rest slate.)
	switch (S)
	{
	case PcTreeNodeState::Owned:
		OutFill = Card;
		OutEdge = PawTokens::INK;
		break;
	case PcTreeNodeState::Ready:
		OutFill = PawTree::NodeDark;
		OutEdge = PawTokens::UI_TREE_READY_RING;
		break;
	default:
		OutFill = PawTree::NodeDark;
		OutEdge = PawTree::NodeEdgeOff;
		break;
	}
}

void UPawTreeNode::SetLook(const FPawNodeLook& In)
{
	const bool bShineChanged = Look.bShine != In.bShine;
	Look = In;
	FLinearColor Fill, Edge;
	FrameColours(In.State, In.Card, Fill, Edge);
	// The plate paints the sticker in these (its NativePaint); the old frame stays clear.
	if (Plate)
	{
		Plate->SetLook(In.Shape, Fill, Edge, In.bOwned ? PawTree::NodeRim : PawTree::NodeRimOff, In.State == PcTreeNodeState::Ready);
	}
	Frame->SetBrush(NoBrush());
	if (Icon)
	{
		// The picture full where owned or on sale, faded on what is not yet within reach (the concept's 0.55).
		Icon->SetRenderOpacity(In.bOwned || In.Reveal == Pc::PcTreeReveal::Full ? 1.0f : 0.55f);
	}
	// C1: the level is painted on a chip over the picture (NativePaint), not this text.
	CounterText->SetText(In.Counter);
	CounterText->SetVisibility(ESlateVisibility::Collapsed);
	// The reveal: one ring ahead faint, the rest hidden (and so out of the mouse's and the pad's reach).
	SetRenderOpacity(In.Reveal == Pc::PcTreeReveal::Ghost ? PawTree::GhostOpacity : 1.0f);
	SetVisibility(In.Reveal == Pc::PcTreeReveal::Hidden ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (bShineChanged)
	{
		ShineClock = 0.0;
		SetColorAndOpacity(FLinearColor::White);
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UPawTreeNode::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
	int32 LayerId, const FWidgetStyle& InStyle, bool bParentEnabled) const
{
	// After the children (a user widget's NativePaint runs after them): the chips over the picture. Only on a node shown in
	// full (one ring ahead shows its shape and picture alone).
	const int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, InStyle, bParentEnabled);
	const FVector2D Size(Geometry.GetLocalSize());
	if (Look.Reveal != Pc::PcTreeReveal::Full || Size.X <= 0.0 || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().GetRenderer())
	{
		return Layer;
	}
	const double S = FMath::Min(Size.X, Size.Y);
	const FVector2D C = Size * 0.5;
	const FLinearColor Tint = InStyle.GetColorAndOpacityTint();
	// Readable at any zoom: zoomed out, the chips stay ChipFloorPx on screen (they grow against their node).
	const UPawSkillTreeScreen* Owner = Screen.Get();
	const double Zoom = Owner ? FMath::Max(Owner->GetZoom(), 0.05) : 1.0;
	const double K = FMath::Max(1.0, ChipFloorPx / (ChipPx * Zoom));
	ChipScale = K;
	const double Px = ChipPx * K, StarPx = ChipStarPx * K, Tick = CheckRadius * K;
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo Font(PawUi::PawFont(), PawUi::FontPt(Px), FName(TEXT("Black")));
	// A chip (the concept's chip()): ink, its colour, its words with a picture before them, centred on At.
	auto Chip = [&](const FVector2D& At, const FString& Words, const FLinearColor& Fill, const FSlateBrush* Picture)
	{
		const FVector2D WordsSize = Measure->Measure(Words, Font);
		const double PicW = Picture ? StarPx + 3.0 * K : 0.0;
		const double H = Px * 1.4, W = WordsSize.X + PicW + Px * 0.9;
		FPawFxCanvas Pill;
		Pill.Polygon(RoundRectPath(At, W + 4.0, H + 4.0, (H + 4.0) * 0.45), ToCanvas(PawTokens::INK, InStyle));
		Pill.Polygon(RoundRectPath(At, W, H, H * 0.4), ToCanvas(Fill, InStyle));
		PawUi::PaintCanvas(Pill, Out, Layer + 1, Geometry.GetAccumulatedRenderTransform());
		const FVector2D Left = At - FVector2D(W * 0.5 - Px * 0.45, 0.0);
		if (Picture)
		{
			FSlateDrawElement::MakeBox(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2f(StarPx, StarPx),
				FSlateLayoutTransform(FVector2f(Left - FVector2D(0.0, StarPx * 0.5)))), Picture, ESlateDrawEffect::None, Tint);
		}
		FSlateDrawElement::MakeText(Out, Layer + 2, Geometry.ToPaintGeometry(FVector2f(WordsSize),
			FSlateLayoutTransform(FVector2f(Left + FVector2D(PicW, -WordsSize.Y * 0.5)))), Words, Font, ESlateDrawEffect::None, PawTokens::INK * Tint);
	};
	// An owned ladder's level, bottom right (green when full); an owned one-time skill's tick, top right; the price of what is
	// on sale, under it (yellow when it can be paid). The concept: "readable without hovering".
	if (!Look.Counter.IsEmpty())
	{
		Chip(C + FVector2D(S * 0.32, S * 0.5 - Px * 0.4), Look.Counter.ToString(), Look.bMaxed ? PawTokens::UI_TREE_OWNED : PawTokens::UI_CARD_PLATE, nullptr);
	}
	if (Look.bCheck)
	{
		const FVector2D At = C + FVector2D(S * 0.36, -S * 0.36);
		FPawFxCanvas TickMark;
		TickMark.Circle(At, Tick + 2.5 * K, ToCanvas(PawTokens::INK, InStyle));
		TickMark.Circle(At, Tick, ToCanvas(PawTokens::UI_TREE_OWNED, InStyle));
		const FVector2D Mark[3] = { At + FVector2D(-0.45, 0.0) * Tick, At + FVector2D(-0.1, 0.35) * Tick, At + FVector2D(0.5, -0.35) * Tick };
		TickMark.Polyline(TConstArrayView<FVector2D>(Mark, 3), ToCanvas(PawTokens::INK, InStyle), Tick * 0.28);
		PawUi::PaintCanvas(TickMark, Out, Layer + 1, Geometry.GetAccumulatedRenderTransform());
	}
	if (!Look.Price.IsEmpty())
	{
		Chip(C + FVector2D(0.0, S * 0.5 + PriceGap * K + Px * 0.7 + 2.0), Look.Price.ToString(), Look.bAffordable ? PawTokens::UI_TREE_READY_RING : PawTree::ChipShort,
			StardustBrush.GetResourceObject() ? &StardustBrush : nullptr);
	}
	++ChipPaints;
	return Layer + 2;
}

void UPawTreeNode::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	DeltaSeconds = PawUi::StepDelta(this, DeltaSeconds); // under a fixed step, that step once a frame (PawUiTime.h)
	if (Look.bShine)
	{
		// An unbought unlock you can buy now shines (self_modulate to SHINE_COLOR and back).
		ShineClock = FMath::Fmod(ShineClock + DeltaSeconds, ShinePeriodSec);
		const double T = 0.5 - 0.5 * FMath::Cos(ShineClock / ShinePeriodSec * UE_DOUBLE_TWO_PI);
		SetColorAndOpacity(FLinearColor::White + (PawTree::ShineTint - FLinearColor::White) * static_cast<float>(T));
	}
}

void UPawTreeNode::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	// Review CR9: the screen learns of the focus from here (Slate raises it when focus enters this node, its inner button
	// included), not by asking all 87 nodes every frame.
	if (UPawSkillTreeScreen* S = Screen.Get())
	{
		S->NodeFocused(BranchId);
	}
}

FLinearColor UPawTreeNode::GetFrameFill() const { return Plate ? Plate->GetFill() : FLinearColor::Transparent; }
FLinearColor UPawTreeNode::GetFrameEdge() const { return Plate ? Plate->GetEdge() : FLinearColor::Transparent; }
int32 UPawTreeNode::GetPlatePaints() const { return Plate ? Plate->GetPaints() : 0; }
bool UPawTreeNode::IsFrameClear() const { return !Frame || Frame->Background.DrawAs == ESlateBrushDrawType::NoDrawType; }

bool UPawHexPlate::Initialize()
{
	// Nothing of its own to lay out: it paints its box.
	const bool bOk = Super::Initialize();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return bOk;
}

void UPawHexPlate::SetLook(EPawNodeShape InShape, const FLinearColor& InFill, const FLinearColor& InEdge, const FLinearColor& InRim, bool bInReadyRing)
{
	if (Shape == InShape && Fill == InFill && Edge == InEdge && Rim == InRim && bReadyRing == bInReadyRing)
	{
		return;
	}
	Shape = InShape;
	Fill = InFill;
	Edge = InEdge;
	Rim = InRim;
	bReadyRing = bInReadyRing;
	Invalidate(EInvalidateWidgetReason::Paint);
}

TArray<FVector2D> UPawHexPlate::ShapePath(EPawNodeShape In, const FVector2D& C, double Size)
{
	switch (In)
	{
	case EPawNodeShape::Diamond:
	{
		const double R = Size * 0.66;
		return { C + FVector2D(0.0, -R), C + FVector2D(R, 0.0), C + FVector2D(0.0, R), C + FVector2D(-R, 0.0) };
	}
	case EPawNodeShape::Circle:
	{
		TArray<FVector2D> P;
		for (int32 I = 0; I < 48; ++I)
		{
			const double A = UE_DOUBLE_TWO_PI * I / 48.0 - UE_DOUBLE_HALF_PI;
			P.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Size * 0.5);
		}
		return P;
	}
	default:
		return RoundRectPath(C, Size * 0.94, Size * 0.94, Size * 0.17);
	}
}

int32 UPawHexPlate::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
	int32 LayerId, const FWidgetStyle& InStyle, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, InStyle, bParentEnabled) + 1;
	const FVector2D Size(Geometry.GetLocalSize());
	if (Size.X <= 0.0 || Size.Y <= 0.0 || Fill.A <= 0.0f)
	{
		return Layer;
	}
	// The concept's drawSkill, back to front: the hard shadow, the die-cut rim, the edge (a gold ring with an ink line
	// inside it when it can be bought now), the fill. One canvas: drawn in call order.
	const double S = FMath::Min(Size.X, Size.Y);
	const FVector2D C = Size * 0.5;
	FPawFxCanvas Paint;
	Paint.Polygon(ShapePath(Shape, C + FVector2D(ShadowX, ShadowY), S + 10.0), ToCanvas(PawTree::NodeShadow, InStyle));
	Paint.Polygon(ShapePath(Shape, C, S + 11.0), ToCanvas(Rim, InStyle));
	Paint.Polygon(ShapePath(Shape, C, S + (bReadyRing ? 7.0 : 4.0)), ToCanvas(Edge, InStyle));
	if (bReadyRing)
	{
		Paint.Polygon(ShapePath(Shape, C, S + 1.5), ToCanvas(PawTokens::INK, InStyle));
	}
	Paint.Polygon(ShapePath(Shape, C, S), ToCanvas(Fill, InStyle));
	PawUi::PaintCanvas(Paint, Out, Layer, Geometry.GetAccumulatedRenderTransform());
	++Paints;
	return Layer;
}

// --- the screen -----------------------------------------------------------------------------------------------------------

// The node the tree was left on, across opens and games (Godot's static var _last_selected): UPawUiMemory (review CR14:
// the game instance's, so never another test's or PIE session's).
FString& UPawSkillTreeScreen::LastSelectedRef() const
{
	UPawUiMemory* M = UPawUiMemory::Of(this);
	return M ? M->TreeLastSelected : LoneLastSelected;
}

FString UPawSkillTreeScreen::LastSelected() const { return LastSelectedRef(); }

bool UPawSkillTreeScreen::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		// Its own opaque backdrop (docs/05 §2 rule 11): the tree's night sky. Visible: it takes the drag and the wheel, and
		// nothing reaches the field.
		UBorder* Sky = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Sky"));
		Sky->SetBrush(Rounded(PawTokens::SKY_BASE, PawTokens::SKY_BASE, 0.0f, 0.0f));
		Sky->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* SkySlot = Root->AddChildToOverlay(Sky);
		SkySlot->SetHorizontalAlignment(HAlign_Fill);
		SkySlot->SetVerticalAlignment(VAlign_Fill);
		// skill_tree.tscn Root/Background: skill_tree_sky.gd's clouds and stars over it (phase 6 slice 5d). Takes no input.
		TreeSky = WidgetTree->ConstructWidget<UPawTreeSky>(PawUi::Resolve<UPawTreeSky>(), TEXT("TreeSky"));
		UOverlaySlot* StarsSlot = Root->AddChildToOverlay(TreeSky);
		StarsSlot->SetHorizontalAlignment(HAlign_Fill);
		StarsSlot->SetVerticalAlignment(VAlign_Fill);
		// The content area: below the header, clipped; the canvas under a render transform inside it.
		UBorder* Content = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Content"));
		Content->SetBrush(NoBrush());
		Content->SetPadding(FMargin(0.0f, static_cast<float>(ContentTop), 0.0f, 0.0f));
		Content->SetClipping(EWidgetClipping::ClipToBounds);
		Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* ContentSlot = Root->AddChildToOverlay(Content);
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Canvas->SetRenderTransformPivot(FVector2D(0.0, 0.0));
		Content->SetContent(Canvas);
		BuildChrome(Root);
		// The Unicorn's arrival over all of it (unicorn_ceremony.gd: its own CanvasLayer 15 over the tree's 1). Collapsed until
		// it plays.
		Unicorn = WidgetTree->ConstructWidget<UPawUnicornCeremony>(PawUi::Resolve<UPawUnicornCeremony>(), TEXT("Unicorn"));
		UOverlaySlot* UnicornSlot = Root->AddChildToOverlay(Unicorn);
		UnicornSlot->SetHorizontalAlignment(HAlign_Fill);
		UnicornSlot->SetVerticalAlignment(VAlign_Fill);
	}
	const bool bOk = Super::Initialize();
	// The screen itself takes the keys when a click lands on nothing focusable (the sky, Continue, the zoom buttons): Slate
	// gives focus to the first focusable widget under the click, and without this that was the game's viewport, so Back / Esc
	// never reached NativeOnPreviewKeyDown and the unspent warning could not be answered (playtest 2026-11-26, notes 11, 27;
	// KB U/24 "a read-only tab or panel leaves focus on SViewport").
	SetIsFocusable(true);
	if (bOk && PawUi::ShouldWire(this))
	{
		PawUi::UseAtlasIcons(this); // the asset's saved icon and plate brushes: the atlas's cells (one batch)
		OnLayoutReady();
	}
	return bOk;
}

void UPawSkillTreeScreen::OnLayoutReady()
{
	// The parts, in either mode.
	PawUi::CollectNamed(WidgetTree, TEXT("TipPrompt"), 7, TipLines);
	CloseSlot = CloseButton ? Cast<UOverlaySlot>(CloseButton->Slot) : nullptr;
	TipSlot = TipPanel ? Cast<UCanvasPanelSlot>(TipPanel->Slot) : nullptr;
	if (CloseButton)
	{
		CloseButton->OnClicked().AddWeakLambda(this, [this]() { PressClose(); });
	}
	if (Unicorn)
	{
		// finished: _enter_focus(), the tree takes the keys back.
		TWeakObjectPtr<UPawSkillTreeScreen> Self(this);
		Unicorn->OnFinished = [Self]()
		{
			UPawSkillTreeScreen* S = Self.Get();
			UWidget* Target = S ? S->NativeGetDesiredFocusTarget() : nullptr;
			if (Target && S->CanTakeFocus())
			{
				Target->SetFocus();
			}
		};
	}
	for (const bool bIn : { true, false })
	{
		if (UPawButton* B = Cast<UPawButton>(WidgetTree->FindWidget(bIn ? TEXT("ZoomIn") : TEXT("ZoomOut"))))
		{
			B->OnClicked().AddWeakLambda(this, [this, bIn]() { ZoomBy(bIn ? Pc::TreeLayout::ZoomStep : -Pc::TreeLayout::ZoomStep); });
		}
	}
	// Not baked: the legend's template (its prompts follow the device).
	if (LegendPrompts)
	{
		LegendPrompts->SetTemplate(PawText::Ui(TEXT("TREE_legend")));
	}
}

void UPawSkillTreeScreen::BuildChrome(UOverlay* Root)
{
	UWidgetTree* T = WidgetTree;
	// The balance: R4f on a plum pill (the board's Stardust pill; Godot's stood on the sky).
	UBorder* StardustPill = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StardustPill"));
	StardustPill->SetBrush(Chrome(EPawChrome::PlateSmall));
	StardustPill->SetPadding(PlatePadding(FMargin(static_cast<float>(R4PillPadX), static_cast<float>(R4PillPadY)), EPawChrome::PlateSmall));
	StardustPill->SetVisibility(ESlateVisibility::HitTestInvisible);
	// D18 (S4): ink on the cream plate (the plum era's cream text read 1:1 on it: nothing showed).
	StardustText = Heavy(MakeText(T, LabelPx, PawTokens::UI_TEXT, false, TEXT("StardustText")));
	StardustPill->SetContent(StardustText);
	UOverlaySlot* SS = Root->AddChildToOverlay(StardustPill);
	SS->SetHorizontalAlignment(HAlign_Right);
	SS->SetVerticalAlignment(VAlign_Top);
	SS->SetPadding(FMargin(0.0f, 20.0f, 20.0f, 0.0f));

	UVerticalBox* ZoomBox = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ZoomBox"));
	UOverlaySlot* ZS = Root->AddChildToOverlay(ZoomBox);
	ZS->SetHorizontalAlignment(HAlign_Left);
	ZS->SetVerticalAlignment(VAlign_Top);
	ZS->SetPadding(FMargin(static_cast<float>(ZoomBoxX), static_cast<float>(ZoomBoxY), 0.0f, 0.0f));
	for (const bool bIn : { true, false })
	{
		UPawButton* B = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), bIn ? TEXT("ZoomIn") : TEXT("ZoomOut"));
		B->SetLabel(PawText::Ui(bIn ? TEXT("TREE_zoom_in") : TEXT("TREE_zoom_out")));
		B->SetMinDimensions(static_cast<int32>(ZoomButtonPx), static_cast<int32>(ZoomButtonPx));
		B->SetIsFocusable(false); // the mouse's; the pad zooms on the triggers
		ZoomBox->AddChildToVerticalBox(B)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	// skill_tree_ui.gd _build_legend: a PromptLabel along the bottom, prompts 22 px.
	LegendPrompts = T->ConstructWidget<UPawPromptText>(PawUi::Resolve<UPawPromptText>(), TEXT("LegendPrompts"));
	LegendPrompts->IconPx = static_cast<int32>(TreePromptPx);
	LegendPrompts->TextPx = static_cast<float>(LabelPx);
	LegendPrompts->TextColour = PawTokens::UI_TEXT; // D18 (S4): ink on the cream plate (was the plum era's cream: unreadable)
	// R4f: on a plum pill (the board's legend bar).
	UBorder* LegendPlate = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LegendPlate"));
	LegendPlate->SetBrush(Chrome(EPawChrome::PlateSmall));
	LegendPlate->SetPadding(PlatePadding(FMargin(static_cast<float>(R4HintPadX), static_cast<float>(R4HintPadY)), EPawChrome::PlateSmall));
	LegendPlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	LegendPlate->SetContent(LegendPrompts);
	UOverlaySlot* LS = Root->AddChildToOverlay(LegendPlate);
	// D18 (S4, the UI concept's scrTree): the bottom-left corner (Godot: the bottom centre, where Continue now stands over it).
	LS->SetHorizontalAlignment(HAlign_Left);
	LS->SetVerticalAlignment(VAlign_Bottom);
	LS->SetPadding(FMargin(24.0f, 0.0f, 0.0f, 24.0f));

	CloseButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("CloseButton"));
	CloseButton->SetIsFocusable(false); // Back / B close it on the keys and pad; the selection stays on the tree
	CloseSlot = Root->AddChildToOverlay(CloseButton);

	// The tooltip, placed absolutely over everything.
	UCanvasPanel* TipLayer = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TipLayer"));
	TipLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* TL = Root->AddChildToOverlay(TipLayer);
	TL->SetHorizontalAlignment(HAlign_Fill);
	TL->SetVerticalAlignment(VAlign_Fill);
	TipPanel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TipPanel"));
	TipPanel->SetBrush(Chrome(EPawChrome::Window)); // R3: the plum plate
	TipPanel->SetPadding(PlatePadding(FMargin(16.0f, 12.0f), EPawChrome::Window));
	TipPanel->SetVisibility(ESlateVisibility::Collapsed);
	UVerticalBox* TipBox = T->ConstructWidget<UVerticalBox>();
	TipPanel->SetContent(TipBox);
	TipTitle = Heavy(MakeText(T, TipTitlePx, PawTokens::UI_TEXT, false, TEXT("TipTitle")));
	TipBox->AddChildToVerticalBox(TipTitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	for (int32 I = 0; I < 7; ++I)
	{
		// Lines with prompts inline (the buy line, a blurb's {pounce}: InputGlyphs.bbcode at 22 px, shown whatever the
		// Show button prompts setting).
		UPawPromptText* Line = T->ConstructWidget<UPawPromptText>(PawUi::Resolve<UPawPromptText>(), PawUi::Numbered(TEXT("TipPrompt"), I));
		Line->IconPx = static_cast<int32>(TreePromptPx);
		Line->TextPx = static_cast<float>(LabelPx);
		Line->TextColour = PawTokens::UI_TEXT;
		Line->bCentred = false;
		Line->bHideWithPrompts = false;
		Line->WrapAt = static_cast<float>(TipWidth - 32.0);
		Line->SetVisibility(ESlateVisibility::Collapsed);
		TipBox->AddChildToVerticalBox(Line)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		TipLines.Add(Line);
	}
	TipSlot = TipLayer->AddChildToCanvas(TipPanel);
	TipSlot->SetAutoSize(true);
}

void UPawSkillTreeScreen::BuildCanvas()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady() || Nodes.Num() > 0)
	{
		return;
	}
	// _ensure_built(): laid out once, when first opened (Godot measured 82 ms for the build on every scene load).
	Layout = Pc::LayOutTree(G->Data());
	Order.Reset();
	for (const std::string& Id : Layout.Order)
	{
		Order.Add(UTF8_TO_TCHAR(Id.c_str()));
	}
	const Pc::PcVec2 Size = Pc::TreeCanvasSize(Pc::TreeBounds(Layout));
	UWidgetTree* T = WidgetTree;

	LinksLayer = T->ConstructWidget<UPawTreeLinks>(PawUi::Resolve<UPawTreeLinks>());
	LinksLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* LSlot = Canvas->AddChildToCanvas(LinksLayer);
	LSlot->SetPosition(FVector2D::ZeroVector);
	LSlot->SetSize(FVector2D(Size.X, Size.Y));

	auto Place = [this](UWidget* W, const FVector2D& Centre, const FVector2D& Extent)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetPosition(Centre - Extent * 0.5);
		S->SetSize(Extent);
		return S;
	};
	// A node's box: a job's diamond stands on a bigger one (C1).
	auto BoxOf = [G](const FString& Id)
	{
		const Pc::PcSkillBranch* B = G->Data().FindBranch(Utf8(Id));
		return NodePx * (B && !B->JobId.empty() ? JobScale : 1.0);
	};
	// Halos first (under the nodes): round, the node's box and the pad.
	for (const FString& Id : Order)
	{
		const double Box = BoxOf(Id);
		UBorder* Halo = T->ConstructWidget<UBorder>();
		Halo->SetBrush(Rounded(WithAlpha(PawTokens::GOLD, HaloGlowAlpha), WithAlpha(PawTokens::GOLD, HaloGlowAlpha), 0.0f, static_cast<float>(Box * 0.5 + HaloPad)));
		Halo->SetVisibility(ESlateVisibility::Collapsed);
		Place(Halo, V(Layout.Nodes.at(Utf8(Id))), FVector2D(Box + 2.0 * HaloPad));
		Halos.Add(Id, Halo);
	}
	// The hub (the concept's root): the links layer paints its yellow disc; a paw stands on it, its name under it, belonging
	// to no trunk.
	LinksLayer->HubAt = V(Layout.Hub);
	LinksLayer->bHub = true;
	UImage* HubPaw = T->ConstructWidget<UImage>();
	HubPaw->SetBrush(PawUi::IconBrush(TEXT("ui_icon_upgrade_paw_damage"), HubIconPx));
	HubPaw->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* PawSlot = Canvas->AddChildToCanvas(HubPaw);
	PawSlot->SetAutoSize(true);
	PawSlot->SetAlignment(FVector2D(0.5, 0.5));
	PawSlot->SetPosition(V(Layout.Hub));
	UTextBlock* HubText = Heavy(MakeText(T, HubNamePx, PawTokens::UI_TREE_READY_RING));
	HubText->SetText(PawText::Ui(TEXT("TREE_hub")));
	// On a dark plate: the Unicorn's link runs straight down under it (the tour, 2026-11-26).
	UBorder* HubPlate = T->ConstructWidget<UBorder>();
	HubPlate->SetBrush(Rounded(WithAlpha(PawTree::LinkUnder, 0.92), WithAlpha(PawTree::LinkUnder, 0.92), 0.0f, 12.0f));
	HubPlate->SetPadding(FMargin(12.0f, 2.0f));
	HubPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	HubPlate->SetContent(HubText);
	HubPlate->SetRenderTransformPivot(FVector2D(0.5, 0.0)); // it grows downward from under the disc (ApplyTransform)
	HubName = HubPlate;
	UCanvasPanelSlot* HubSlot = Canvas->AddChildToCanvas(HubPlate);
	HubSlot->SetAutoSize(true);
	HubSlot->SetAlignment(FVector2D(0.5, 0.0));
	HubSlot->SetPosition(V(Layout.Hub) + FVector2D(0.0, HubRadius + HubNameGap));

	for (const FString& Id : Order)
	{
		const FVector2D C = V(Layout.Nodes.at(Utf8(Id)));
		const double Box = BoxOf(Id);
		UPawTreeNode* N = T->ConstructWidget<UPawTreeNode>(PawUi::Resolve<UPawTreeNode>());
		N->BranchId = Id;
		N->Screen = this;
		if (const Pc::PcSkillBranch* Branch = G->Data().FindBranch(Utf8(Id)))
		{
			N->SetIconPath(UTF8_TO_TCHAR(Branch->IconPath.c_str()), Box * (Branch->JobId.empty() ? IconShare : JobIconShare));
		}
		N->OnClicked().AddWeakLambda(this, [this, Id]() { PressNode(Id); });
		Place(N, C, FVector2D(Box));
		Nodes.Add(Id, N);
		Juice.Hover(N, this); // UiJuice.hover(btn)
	}
	// Each branch's name in the field (the concept's drawLabels): its trunk's colour on an ink outline, placed by RefreshAll
	// past what is shown of it. No caption under a node since C1 (the name is in the tooltip).
	for (const std::string& Trunk : Pc::TrunkOrder(G->Data()))
	{
		const FString TrunkId = UTF8_TO_TCHAR(Trunk.c_str());
		UTextBlock* Name = Heavy(MakeText(T, BranchNamePx, WithAlpha(TrunkLook(TrunkId).Link, 0.9)));
		FSlateFontInfo Font = Name->GetFont();
		Font.OutlineSettings.OutlineSize = static_cast<int32>(BranchNameOutline);
		Font.OutlineSettings.OutlineColor = PawTree::LinkUnder;
		Name->SetFont(Font);
		Name->SetText(PawText::TreeLabel(Trunk).ToUpper());
		Name->SetVisibility(ESlateVisibility::Collapsed);
		Name->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		UCanvasPanelSlot* NS = Canvas->AddChildToCanvas(Name);
		NS->SetAutoSize(true);
		NS->SetAlignment(FVector2D(0.5, 0.5));
		BranchLabels.Add(TrunkId, Name);
	}
}

std::set<std::string> UPawSkillTreeScreen::ShownIds() const
{
	std::set<std::string> Out;
	for (const FString& Id : Shown)
	{
		Out.insert(Utf8(Id));
	}
	return Out;
}

void UPawSkillTreeScreen::WriteNavigation()
{
	// nav_pick() among the shown nodes (C1: a hidden node is out of the pad's reach), written as explicit rules: no node that
	// way = stay put (never off the tree). Written again whenever the reveal changes.
	const std::set<std::string> Among = ShownIds();
	TArray<FString> Seen;
	for (const FString& Id : Order)
	{
		if (Shown.Contains(Id))
		{
			Seen.Add(Id);
		}
	}
	const Pc::PcVec2 Dirs[4] = { { 0.0, -1.0 }, { 0.0, 1.0 }, { -1.0, 0.0 }, { 1.0, 0.0 } };
	const EUINavigation Nav[4] = { EUINavigation::Up, EUINavigation::Down, EUINavigation::Left, EUINavigation::Right };
	for (int32 I = 0; I < Seen.Num(); ++I)
	{
		UPawTreeNode* N = Nodes[Seen[I]];
		for (int32 K = 0; K < 4; ++K)
		{
			const std::string Pick = Pc::TreeNavPick(Layout, Utf8(Seen[I]), Dirs[K], &Among);
			N->SetNavigationRuleExplicit(Nav[K], Pick.empty() ? N : Nodes[UTF8_TO_TCHAR(Pick.c_str())].Get());
		}
		N->SetNavigationRuleExplicit(EUINavigation::Next, Nodes[Seen[(I + 1) % Seen.Num()]]);
		N->SetNavigationRuleExplicit(EUINavigation::Previous, Nodes[Seen[(I - 1 + Seen.Num()) % Seen.Num()]]);
	}
}

UPawRealmSubsystem* UPawSkillTreeScreen::Realm() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
}

UPawGameSubsystem* UPawSkillTreeScreen::Game() const
{
	const UPawRealmSubsystem* R = Realm();
	return R ? R->GetGame() : nullptr;
}

bool UPawSkillTreeScreen::IsCeremonyPlaying() const
{
	return Unicorn && Unicorn->IsPlaying();
}

bool UPawSkillTreeScreen::CanTakeFocus() const
{
	return IsActivated() && GetOwningLocalPlayer() != nullptr;
}

FVector2D UPawSkillTreeScreen::ContentSize() const
{
	const FVector2D C(ViewSize.X, ViewSize.Y - ContentTop);
	return C.X > 0.0 && C.Y > 0.0 ? C : FVector2D(1920.0, 950.0);
}

void UPawSkillTreeScreen::SetViewSize(const FVector2D& Size)
{
	if (Size.X > 0.0 && Size.Y > 0.0 && Size != ViewSize)
	{
		ViewSize = Size;
		StyleClose();
		if (!bViewTouched)
		{
			FitAndCentre(); // R4h: fitted to the size the player sees, not the last one
		}
	}
}

void UPawSkillTreeScreen::ApplyTransform()
{
	FWidgetTransform T;
	T.Translation = Pan;
	T.Scale = FVector2D(Zoom, Zoom);
	Canvas->SetRenderTransform(T);
	// The names keep the text floor on screen: zoomed out, they grow against the tree (the chips do the same in their paint).
	NameScale = FMath::Max(1.0, NameFloorPx / (BranchNamePx * FMath::Max(Zoom, 0.05)));
	const FVector2D Grow(NameScale, NameScale);
	for (const TPair<FString, TObjectPtr<UTextBlock>>& KV : BranchLabels)
	{
		KV.Value->SetRenderScale(Grow);
	}
	if (HubName)
	{
		HubName->SetRenderScale(Grow);
	}
}

void UPawSkillTreeScreen::FitAndCentre()
{
	FVector2D View = ContentSize();
	if (bSpendMode)
	{
		// The spend step's Continue stands at the bottom centre: the tree fits above it (the tour, 2026-11-26: it covered
		// the Unicorn, straight down from the hub).
		View.Y = FMath::Max(View.Y * 0.5, View.Y - (CloseSpendH + CloseSpendGapBottom));
	}
	const Pc::PcRect2 B = GetFitBounds(); // C1: what is shown, not the whole tree
	Zoom = Pc::TreeFitZoom(B, View.X, View.Y);
	// _recenter_view(): the middle of the laid-out tree in the middle of the view.
	Pan = View * 0.5 - FVector2D(B.CentreX(), B.CentreY()) * Zoom;
	ApplyTransform();
}

void UPawSkillTreeScreen::ZoomBy(double Delta)
{
	// The point under the middle of the view stays there (scaling about the canvas corner would swing the tree away).
	const double Before = Zoom;
	Zoom = FMath::Clamp(Zoom + Delta, Pc::TreeLayout::ZoomMin, Pc::TreeLayout::ZoomMax);
	if (FMath::IsNearlyEqual(Before, Zoom))
	{
		return;
	}
	const FVector2D Centre = ContentSize() * 0.5;
	const FVector2D Focus = (Centre - Pan) / Before;
	Pan = Centre - Focus * Zoom;
	bFollowing = false;
	bViewTouched = true;
	ApplyTransform();
}

FVector2D UPawSkillTreeScreen::NodeOnScreen(const FString& Id) const
{
	const auto It = Layout.Nodes.find(Utf8(Id));
	if (It == Layout.Nodes.end())
	{
		return FVector2D::ZeroVector;
	}
	return FVector2D(0.0, ContentTop) + Pan + V(It->second) * Zoom;
}

void UPawSkillTreeScreen::Follow(const FString& Id, bool bInstant)
{
	// Keep the selection inside the central FOLLOW_BOX of the view (content coordinates).
	const auto It = Layout.Nodes.find(Utf8(Id));
	if (It == Layout.Nodes.end())
	{
		return;
	}
	const FVector2D View = ContentSize();
	const FVector2D BoxMin = View * (1.0 - FollowBox) * 0.5, BoxMax = BoxMin + View * FollowBox;
	const FVector2D At = Pan + V(It->second) * Zoom;
	if (At.X >= BoxMin.X && At.X <= BoxMax.X && At.Y >= BoxMin.Y && At.Y <= BoxMax.Y)
	{
		return;
	}
	const FVector2D Target = Pan + ((BoxMin + BoxMax) * 0.5 - At);
	bViewTouched = true;
	if (bInstant)
	{
		Pan = Target;
		bFollowing = false;
		ApplyTransform();
		return;
	}
	bFollowing = true;
	FollowFrom = Pan;
	FollowTo = Target;
	FollowT = 0.0;
}

void UPawSkillTreeScreen::StyleClose()
{
	if (!CloseButton || !CloseSlot)
	{
		return;
	}
	// Browse: a small X in the top right (a dismiss); the spend step: a big Continue at the bottom centre (a decision; the
	// designer, 2026-08-23: "make the confirm button larger and not so in the corner, it can be missed").
	if (bSpendMode)
	{
		CloseSlot->SetHorizontalAlignment(HAlign_Center);
		CloseSlot->SetVerticalAlignment(VAlign_Bottom);
		CloseSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, static_cast<float>(CloseSpendGapBottom)));
		CloseButton->SetMinDimensions(static_cast<int32>(CloseSpendW), static_cast<int32>(CloseSpendH));
		CloseButton->SetLabelSize(CloseSpendPx);
		CloseButton->SetLabel(PawText::Ui(TEXT("TREE_continue")));
		// D18 (S4, the UI concept's scrTree): yellow, with the Reawaken swirl (it starts the next run). It stays big and in
		// the middle (the designer, 2026-08-23); the concept draws it in the corner.
		CloseButton->SetCallLook(true);
		CloseButton->SetIcon(PawUi::TabIconName(TEXT("reawaken")), CloseSpendIconPx);
	}
	else
	{
		CloseSlot->SetHorizontalAlignment(HAlign_Right);
		CloseSlot->SetVerticalAlignment(VAlign_Top);
		CloseSlot->SetPadding(FMargin(0.0f, static_cast<float>(CloseBrowseMarginY), static_cast<float>(CloseBrowseMarginX), 0.0f));
		CloseButton->SetMinDimensions(static_cast<int32>(CloseBrowseW), static_cast<int32>(CloseBrowseH));
		CloseButton->SetLabelSize(ButtonPx);
		CloseButton->SetLabel(PawText::Ui(TEXT("TREE_close")));
		CloseButton->SetCallLook(false);
		CloseButton->SetIcon(FString(), CloseSpendIconPx);
	}
}

void UPawSkillTreeScreen::RefreshAll()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady() || Nodes.Num() == 0)
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcProgress& P = G->Progress();
	// The reveal (C1): owned or on sale in full, one ring ahead faint, the rest hidden. The pad's rules follow it.
	TMap<FString, Pc::PcTreeReveal> Reveal;
	TSet<FString> NowShown;
	for (const FString& Id : Order)
	{
		const Pc::PcTreeReveal R = Pc::TreeNodeReveal(D, P, Utf8(Id));
		Reveal.Add(Id, R);
		if (R != Pc::PcTreeReveal::Hidden)
		{
			NowShown.Add(Id);
		}
	}
	const bool bShownChanged = NowShown.Num() != Shown.Num() || !NowShown.Includes(Shown);
	Shown = MoveTemp(NowShown);
	for (const FString& Id : Order)
	{
		const std::string S = Utf8(Id);
		const Pc::PcSkillBranch* B = D.FindBranch(S);
		if (!B)
		{
			continue;
		}
		const PcTreeNodeState State = Pc::TreeNodeState(D, P, S);
		const int64 Owned = Pc::BranchLevel(P, S);
		const int64 Max = Pc::BranchMaxLevel(*B);
		FPawNodeLook Look;
		Look.State = State;
		Look.Reveal = Reveal[Id];
		// The shape says the kind (C1): a job a diamond, a one-time skill a circle, a skill with levels a square.
		Look.Shape = !B->JobId.empty() ? EPawNodeShape::Diamond : Max <= 1 ? EPawNodeShape::Circle : EPawNodeShape::Square;
		Look.Card = TrunkLook(UTF8_TO_TCHAR(Pc::TreeTrunkOf(D, S).c_str())).Card;
		Look.bOwned = Owned > 0;
		// An owned ladder's level on a chip ("3/10", green when full); an owned one-time skill a tick.
		if (Owned > 0 && Max > 1)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("owned"), PawText::Count(Owned));
			Args.Add(TEXT("max"), PawText::Count(Max));
			Look.Counter = PawText::Ui(TEXT("TREE_counter"), Args);
			Look.bMaxed = Owned >= Max;
		}
		Look.bCheck = Owned > 0 && Max <= 1;
		// What is on sale (shown in full, not yet bought) shows its price; yellow when it can be paid now.
		if (Look.Reveal == Pc::PcTreeReveal::Full && Owned == 0)
		{
			Look.Price = PawText::Amount(Pc::NextBranchCost(D, P, S));
			Look.bAffordable = State == PcTreeNodeState::Ready;
		}
		// Shine: an unlock BUYABLE and unbought, in the spend step (a locked one would invite a click that does nothing).
		const bool bShine = B->bIsUnlock && Owned == 0 && Pc::TreeCanBuyNow(D, P, S) && bSpendMode;
		Look.bShine = bShine;
		Nodes[Id]->SetLook(Look);
		Halos[Id]->SetVisibility(State == PcTreeNodeState::Ready ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// btn.disabled = maxed or not available or not affordable or not _spend_mode: no hover pop on a node that cannot be
		// bought now. A READY node leans while it waits, on top of its halo (the halo does not rotate: a glow, not a frame).
		Juice.SetHoverOff(Nodes[Id], !(bSpendMode && Pc::TreeCanBuyNow(D, P, S)));
		Juice.Jitter(Nodes[Id], State == PcTreeNodeState::Ready && bSpendMode, FPawJuice::EffectsOn(this));
	}
	// The links (an edge is owned only when both ends are bought; its colour its trunk's), each as shown as its far end;
	// the breathing Unicorn once it shows in full.
	LinksLayer->Links.Reset();
	for (const Pc::PcTreeLink& L : Pc::TreeLinks(D, P))
	{
		UPawTreeLinks::FLink Out;
		Out.A = L.From == "hub" ? V(Layout.Hub) : V(Layout.Nodes.at(L.From));
		Out.B = V(Layout.Nodes.at(L.To));
		Out.Colour = TrunkLook(UTF8_TO_TCHAR(Pc::TreeTrunkOf(D, L.To).c_str())).Link;
		Out.bOwned = L.bOwned;
		Out.bShut = L.bShut;
		const Pc::PcTreeReveal* R = Reveal.Find(UTF8_TO_TCHAR(L.To.c_str()));
		const Pc::PcTreeReveal To = R ? *R : Pc::PcTreeReveal::Hidden;
		Out.Alpha = To == Pc::PcTreeReveal::Hidden ? 0.0f : To == Pc::PcTreeReveal::Ghost ? PawTree::GhostOpacity : 1.0f;
		Out.bNext = To == Pc::PcTreeReveal::Full && Pc::BranchLevel(P, L.To) == 0;
		LinksLayer->Links.Add(Out);
	}
	LinksLayer->Pulse.Reset();
	for (const FString& Id : Order)
	{
		if (InList(Id, PawTree::PulseBranches, UE_ARRAY_COUNT(PawTree::PulseBranches)) && Reveal[Id] == Pc::PcTreeReveal::Full)
		{
			LinksLayer->Pulse.Add(V(Layout.Nodes.at(Utf8(Id))));
		}
	}
	// Each branch's name, past what is shown of it.
	const std::set<std::string> Among = ShownIds();
	TSet<FString> Named;
	for (const Pc::PcTreeLabel& L : Pc::TreeBranchLabels(D, Layout, &Among))
	{
		const FString Trunk = UTF8_TO_TCHAR(L.Trunk.c_str());
		if (const TObjectPtr<UTextBlock>* Name = BranchLabels.Find(Trunk))
		{
			if (UCanvasPanelSlot* NS = Cast<UCanvasPanelSlot>((*Name)->Slot))
			{
				NS->SetPosition(V(L.At));
			}
			(*Name)->SetVisibility(ESlateVisibility::HitTestInvisible);
			Named.Add(Trunk);
		}
	}
	for (const TPair<FString, TObjectPtr<UTextBlock>>& KV : BranchLabels)
	{
		if (!Named.Contains(KV.Key))
		{
			KV.Value->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (bShownChanged)
	{
		WriteNavigation();
	}
	FFormatNamedArguments Args;
	Args.Add(TEXT("n"), PawText::Count(static_cast<int64>(P.Stardust)));
	StardustText->SetText(PawText::Ui(TEXT("TREE_stardust"), Args));
	StyleClose();
	if (!TipBranch.IsEmpty())
	{
		ShowTip(TipBranch, bTipDocked);
	}
}

void UPawSkillTreeScreen::ShowTip(const FString& Id, bool bDocked)
{
	TipBranch = Id;
	bTipDocked = bDocked;
	const UPawGameSubsystem* G = Game();
	if (Id.IsEmpty() || !G || !G->IsDataReady())
	{
		TipPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const std::string S = Utf8(Id);
	const Pc::PcTreeTip Tip = Pc::TreeTip(D, G->Progress(), S, bSpendMode);
	if (!Tip.bKnown)
	{
		TipPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	// Review CR9: NativeTick shows the tip every frame (the mouse's node, or docked for the pad), but its lines (about eight
	// formats, a glyph parse per line and a layout pass) are built again only when the node or the tip's numbers change.
	// Placing it stays every frame's (a zoom or a drag moves the node).
	const bool bRebuild = !bTipBuilt || Id != TipBuiltFor || !SameTip(Tip, TipBuilt);
	const bool bWasHidden = TipPanel->GetVisibility() == ESlateVisibility::Collapsed;
	if (bRebuild)
	{
		FText Title = PawText::TreeLabel(S);
		if (!Tip.bUnlock)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("name"), Title);
			Args.Add(TEXT("owned"), PawText::Count(Tip.Owned));
			Args.Add(TEXT("max"), PawText::Count(Tip.Max));
			Title = PawText::Ui(TEXT("TREE_title_count"), Args);
		}
		TipTitle->SetText(Title);
		TArray<TPair<FText, FLinearColor>> Lines;
		Lines.Add({ PawText::TreeBlurb(S), PawTree::TipBlurb });
		auto Effect = [&](int64 Level) { return PawText::TreeEffect(Pc::TreeEffectPhrases(D, S, Level)); };
		if (Tip.bMaxed)
		{
			Lines.Add({ PawText::Ui(TEXT("TREE_fully")), PawTree::TipFully });
			Lines.Add({ Effect(Tip.Owned), PawTokens::UI_TEXT });
		}
		else
		{
			if (Tip.bShowNow)
			{
				FFormatNamedArguments A;
				A.Add(TEXT("effect"), Effect(Tip.Owned));
				Lines.Add({ PawText::Ui(TEXT("TREE_now"), A), PawTokens::UI_TEXT });
			}
			FFormatNamedArguments A;
			A.Add(TEXT("effect"), Effect(Tip.Next));
			Lines.Add({ PawText::Ui(TEXT("TREE_next"), A), PawTree::TipNext });
			FFormatNamedArguments C;
			C.Add(TEXT("n"), PawText::Count(Tip.Cost));
			Lines.Add({ PawText::Ui(TEXT("TREE_cost"), C), PawTokens::UI_TEXT });
			FFormatNamedArguments R;
			switch (Tip.Reason)
			{
			case Pc::PcTreeTipReason::Closed:
				R.Add(TEXT("branch"), PawText::TreeLabel(Tip.Rival));
				Lines.Add({ PawText::Ui(TEXT("TREE_closed"), R), PawTree::TipReason });
				break;
			case Pc::PcTreeTipReason::NeedsParent:
				R.Add(TEXT("branch"), PawText::TreeLabel(Tip.Parent));
				Lines.Add({ PawText::Ui(TEXT("TREE_locked_parent"), R), PawTree::TipReason });
				break;
			case Pc::PcTreeTipReason::ReawakenGate:
				R.Add(TEXT("n"), PawText::Count(Tip.Gate));
				Lines.Add({ PawText::Ui(TEXT("TREE_locked_reawaken"), R), PawTree::TipReason });
				break;
			case Pc::PcTreeTipReason::BrowseOnly:
				Lines.Add({ PawText::Ui(TEXT("TREE_browse")), PawTree::TipReason });
				break;
			case Pc::PcTreeTipReason::Short:
				R.Add(TEXT("n"), PawText::Count(Tip.Have));
				Lines.Add({ PawText::Ui(TEXT("TREE_have"), R), PawTree::TipShort });
				break;
			default:
				break;
			}
		}
		if (Tip.bBuyPrompt)
		{
			Lines.Add({ PawText::Ui(TEXT("TREE_buy")), PawTokens::UI_TEXT });
		}
		for (int32 I = 0; I < TipLines.Num(); ++I)
		{
			const bool bShow = I < Lines.Num();
			TipLines[I]->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (bShow)
			{
				TipLines[I]->SetTextColour(Lines[I].Value);
				TipLines[I]->SetTemplate(Lines[I].Key);
			}
		}
		bTipBuilt = true;
		TipBuiltFor = Id;
		TipBuilt = Tip;
		++TipBuilds;
	}
	TipPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (bRebuild || bWasHidden)
	{
		TipPanel->ForceLayoutPrepass(); // its size this frame, for this frame's placement
	}
	const FVector2D TipSize = TipPanel->GetDesiredSize();
	FVector2D At;
	if (bDocked)
	{
		// Keys and pads: docked bottom left, the middle left clear for the selection the camera keeps there.
		At = FVector2D(TipDockX, FMath::Max(TipDockX, ViewSize.Y - TipSize.Y - TipDockY));
	}
	else
	{
		// The mouse: beside its node, re-seated every frame (a zoom or a drag moves the node).
		const FVector2D C = NodeOnScreen(Id);
		const double Half = NodePx * 0.5 * Zoom;
		At = FVector2D(C.X - Half - TipSize.X - TipMargin, C.Y - TipSize.Y * 0.5);
		At.X = FMath::Clamp(At.X, TipMargin, FMath::Max(TipMargin, ViewSize.X - TipSize.X - TipMargin));
		At.Y = FMath::Clamp(At.Y, TipMargin, FMath::Max(TipMargin, ViewSize.Y - TipSize.Y - TipMargin));
	}
	TipSlot->SetPosition(At);
}

FString UPawSkillTreeScreen::GetLegend() const
{
	return LegendPrompts ? LegendPrompts->GetNeutral() : FString();
}

TArray<FText> UPawSkillTreeScreen::GetTipLines() const
{
	TArray<FText> Out;
	if (TipPanel && TipPanel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		for (const UPawPromptText* L : TipLines)
		{
			if (L && L->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Out.Add(PawText::Verbatim(L->GetNeutral()));
			}
		}
	}
	return Out;
}

FText UPawSkillTreeScreen::GetTipTitle() const { return TipTitle ? TipTitle->GetText() : FText::GetEmpty(); }

bool UPawSkillTreeScreen::PressNode(const FString& Id)
{
	// The rule at the point the Stardust moves: only the spend step buys (browsing is read-only).
	UPawGameSubsystem* G = Game();
	if (!bSpendMode || !G || !G->IsDataReady() || IsCeremonyPlaying())
	{
		return false;
	}
	const Pc::PcBranchPurchase Bought = Pc::PurchaseNextBranchLevel(G->Data(), G->Progress(), Utf8(Id));
	if (!Bought.bBought)
	{
		return false;
	}
	// Buying Auto-Travel or Auto-Card switches that automation ON (otherwise the player buys it and nothing happens;
	// skill_tree_manager.gd purchase_next_level -> SettingsManager.set_auto_travel / set_auto_pick_card(true)).
	if (Bought.bAutoTravelOn)
	{
		G->Slot().bAutoTravel = true;
	}
	if (UPawRealmSubsystem* R = Realm())
	{
		R->OnProgressChanged(true); // Vitality and the like change the kittens; every level the stats
	}
	RefreshAll();
	if (Id == TEXT("unlock_unicorn") && Unicorn)
	{
		// _play_unicorn_ceremony(): the tooltip hides, the ceremony plays over the tree.
		TipPanel->SetVisibility(ESlateVisibility::Collapsed);
		Unicorn->Play(ViewSize.X > 0.0 ? ViewSize : FVector2D(1920.0, 1080.0));
	}
	Juice.Punch(Nodes.FindRef(Id)); // _play_purchase_pop: the shared purchase punch
	ShowTip(Id, bTipDocked); // the tooltip under the cursor shows the next level's price at once (designer 2026-08-23)
	return true;
}

void UPawSkillTreeScreen::PressClose()
{
	// Nothing closes the tree out from under the Unicorn (the keys skip it instead; it takes every click). It ticks with the
	// tree: closed mid-ceremony it never finished, and the tree refused every purchase after it (the soak's driver did this,
	// 2026-11-24).
	if (IsCeremonyPlaying())
	{
		return;
	}
	// ALWAYS asks after a Reawaken (designer 2026-10-01): leaving starts the next run. Since P4.7 a real question in the
	// confirm dialog (playtest 2026-11-26, note 27), Stay selected: "You still have Stardust to spend." while something is
	// affordable, else "Start the next run?". [Leave] [Stay]. One at a time.
	if (bSpendMode && ModalStack.IsValid())
	{
		if (IsAskingToLeave())
		{
			return;
		}
		const UPawGameSubsystem* G = Game();
		const bool bUnspent = G && G->IsDataReady() && Pc::HasAffordableBranch(G->Data(), G->Progress());
		ShowTip(FString(), false);
		const TWeakObjectPtr<UPawSkillTreeScreen> Self(this);
		LeaveDialog = UPawConfirmDialog::Open(ModalStack.Get(), PawText::Ui(bUnspent ? TEXT("TREE_leave_prompt") : TEXT("TREE_start_next")),
			PawText::Ui(TEXT("TREE_leave_yes")), PawText::Ui(TEXT("TREE_leave_no")),
			[Self]()
			{
				if (UPawSkillTreeScreen* S = Self.Get())
				{
					S->LeaveNow();
				}
			},
			[Self]()
			{
				// Stay: the selection back on the tree where it was.
				UPawSkillTreeScreen* S = Self.Get();
				UWidget* Target = S ? S->NativeGetDesiredFocusTarget() : nullptr;
				if (Target && S->CanTakeFocus())
				{
					Target->SetFocus();
				}
			});
		return;
	}
	LeaveNow();
}

void UPawSkillTreeScreen::LeaveNow()
{
	ShowTip(FString(), false);
	LeaveDialog = nullptr;
	const bool bWasSpending = bSpendMode;
	bSpendMode = false;
	TFunction<void()> Then = bWasSpending ? MoveTemp(OnSpendingFinished) : TFunction<void()>();
	OnSpendingFinished = nullptr;
	if (IsActivated())
	{
		DeactivateWidget();
	}
	if (Then)
	{
		Then();
	}
}

FString UPawSkillTreeScreen::JumpToAffordable()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return FString();
	}
	const std::string Next = Pc::NextAffordableBranch(G->Data(), G->Progress(), Utf8(LastSelectedRef()));
	if (Next.empty())
	{
		return FString();
	}
	const FString Id = UTF8_TO_TCHAR(Next.c_str());
	LastSelectedRef() = Id;
	if (CanTakeFocus() && Nodes.Contains(Id))
	{
		Nodes[Id]->SetFocus();
	}
	OnNodeFocused(Id);
	return Id;
}

void UPawSkillTreeScreen::OnNodeFocused(const FString& Id)
{
	LastSelectedRef() = Id;
	Follow(Id, false);
	ShowTip(Id, true);
}

UWidget* UPawSkillTreeScreen::NativeGetDesiredFocusTarget() const
{
	// _enter_focus(): in the spend step the cheapest node for sale (that is why the tree is open); else where it was left;
	// else the first trunk.
	FString Id;
	const UPawGameSubsystem* G = Game();
	if (bSpendMode && G && G->IsDataReady())
	{
		Id = UTF8_TO_TCHAR(Pc::CheapestAffordableBranch(G->Data(), G->Progress()).c_str());
	}
	if ((Id.IsEmpty() || !Shown.Contains(Id)) && Shown.Contains(LastSelectedRef()))
	{
		Id = LastSelectedRef();
	}
	if ((Id.IsEmpty() || !Shown.Contains(Id)) && Pc::TrunkWheel().size() > 0)
	{
		Id = UTF8_TO_TCHAR(Pc::TrunkWheel().front().c_str());
	}
	const TObjectPtr<UPawTreeNode>* N = Nodes.Find(Id);
	return N ? N->Get() : static_cast<UWidget*>(CloseButton);
}

TOptional<FUIInputConfig> UPawSkillTreeScreen::GetDesiredInputConfig() const
{
	// A menu over a held field; the drag needs the mouse down inside the screen, which Slate gives without a viewport capture.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UPawSkillTreeScreen::NativeOnActivated()
{
	if (UPawBobaSubsystem* Boba = UPawBobaSubsystem::Of(this))
	{
		Boba->Suppress(TEXT("tree"), true); // P4.3, CLAUDE.md: Boba never over the skill tree (a cameo in flight is cut)
	}
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, true);
	}
	BuildCanvas();
	ShowTip(FString(), false);
	RefreshAll(); // the reveal first: the fit frames what is shown
	// Recentred on every open: a player who dragged the canvas somewhere odd still finds the tree where they expect it.
	bViewTouched = false;
	FitAndCentre();
	if (const UWidget* Target = NativeGetDesiredFocusTarget())
	{
		if (const UPawTreeNode* N = Cast<UPawTreeNode>(Target))
		{
			LastSelectedRef() = N->BranchId;
		}
	}
	Super::NativeOnActivated();
}

void UPawSkillTreeScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	if (UPawBobaSubsystem* Boba = UPawBobaSubsystem::Of(this))
	{
		Boba->Suppress(TEXT("tree"), false);
	}
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, false);
	}
}

void UPawSkillTreeScreen::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	DeltaSeconds = PawUi::StepDelta(this, DeltaSeconds); // under a fixed step, that step once a frame (PawUiTime.h)
	SetViewSize(FVector2D(Geometry.GetLocalSize()));
	Juice.Tick(DeltaSeconds);
	// The halos breathe; the rainbow turns; the pulse breathes.
	HaloClock += DeltaSeconds;
	const double T = 0.5 + 0.5 * FMath::Sin(HaloClock * UE_DOUBLE_TWO_PI / HaloPeriodSec);
	const float Alpha = static_cast<float>(FMath::Lerp(HaloAlphaLow, HaloAlphaHigh, T));
	for (const TPair<FString, TObjectPtr<UBorder>>& H : Halos)
	{
		if (H.Value->GetVisibility() != ESlateVisibility::Collapsed)
		{
			H.Value->SetRenderOpacity(Alpha);
		}
	}
	if (LinksLayer)
	{
		LinksLayer->RainbowPhase = FMath::Fmod(LinksLayer->RainbowPhase + DeltaSeconds * RainbowTurnsPerSec, 1.0);
		LinksLayer->PulseClock = FMath::Fmod(LinksLayer->PulseClock + DeltaSeconds, 1.0 / PulseHz);
	}
	// The pad (_tick_pad_view): the steer stick pans (Player.steer_input: dead zone already removed), zoom_in / zoom_out zoom,
	// both held.
	if (!Stick.IsZero())
	{
		bFollowing = false;
		bViewTouched = true;
		Pan -= Stick * PanPxPerSec * DeltaSeconds;
		ApplyTransform();
	}
	if (ZoomHeld != 0.0)
	{
		ZoomBy(ZoomHeld * ZoomPerSec * DeltaSeconds);
	}
	if (bFollowing)
	{
		FollowT = FMath::Min(1.0, FollowT + DeltaSeconds / FollowSec);
		const double E = 1.0 - FMath::Pow(1.0 - FollowT, 3.0); // TRANS_CUBIC, EASE_OUT
		Pan = FMath::Lerp(FollowFrom, FollowTo, E);
		bFollowing = FollowT < 1.0;
		ApplyTransform();
	}
	// The selection follows focus continuously (spike S4), by the nodes' focus events (NodeFocused); the tooltip follows the
	// mouse's node or docks for the pad.
	UpdateTip();
}

void UPawSkillTreeScreen::UpdateTip()
{
	const FString* Hovered = nullptr;
	for (const TPair<FString, TObjectPtr<UPawTreeNode>>& N : Nodes)
	{
		if (N.Value->IsHovered())
		{
			Hovered = &N.Key;
		}
	}
	if (Hovered)
	{
		ShowTip(*Hovered, false);
	}
	else if (!TipBranch.IsEmpty() && !bTipDocked)
	{
		ShowTip(FString(), false);
	}
	else if (!TipBranch.IsEmpty())
	{
		ShowTip(TipBranch, true);
	}
}

void UPawSkillTreeScreen::NodeFocused(const FString& Id)
{
	if (!Id.IsEmpty() && Id != LastSelectedRef() && Nodes.Contains(Id))
	{
		OnNodeFocused(Id);
	}
}

FReply UPawSkillTreeScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// skill_tree_ui.gd, through the input map: back / open_skill_tree close (the spend step asks once); buy_amount jumps to
	// the next affordable node; zoom_in / zoom_out zoom while held.
	const FKey K = KeyEvent.GetKey();
	if (IsCeremonyPlaying())
	{
		// The tree takes no keys while the Unicorn arrives (Back cannot close it out from under it); a press skips.
		Unicorn->Input(KeyEvent.IsRepeat() ? Pc::PcUnicornInput::KeyEcho
			: (K.IsGamepadKey() ? Pc::PcUnicornInput::PadDown : Pc::PcUnicornInput::KeyDown));
		return FReply::Handled();
	}
	if (PawInput::KeyIs(K, TEXT("back")) || PawInput::KeyIs(K, TEXT("open_skill_tree")))
	{
		if (!KeyEvent.IsRepeat())
		{
			PressClose();
		}
		return FReply::Handled();
	}
	if (PawInput::KeyIs(K, TEXT("buy_amount")))
	{
		JumpToAffordable();
		return FReply::Handled();
	}
	// zoom_in / zoom_out: a key holds strength 1; a trigger's digital stand-in is handled but changes nothing (its analog
	// value carries its strength, NativeOnAnalogValueChanged).
	if (PawInput::KeyIs(K, TEXT("zoom_in")) || PawInput::KeyIs(K, TEXT("zoom_out")))
	{
		if (!PawInput::IsStandIn(K))
		{
			bZoomInKey |= PawInput::KeyIs(K, TEXT("zoom_in"));
			bZoomOutKey |= PawInput::KeyIs(K, TEXT("zoom_out"));
			RefreshZoom();
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

FReply UPawSkillTreeScreen::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	const FKey K = KeyEvent.GetKey();
	if (PawInput::KeyIs(K, TEXT("zoom_in")) || PawInput::KeyIs(K, TEXT("zoom_out")))
	{
		if (!PawInput::IsStandIn(K))
		{
			bZoomInKey &= !PawInput::KeyIs(K, TEXT("zoom_in"));
			bZoomOutKey &= !PawInput::KeyIs(K, TEXT("zoom_out"));
			RefreshZoom();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyUp(Geometry, KeyEvent);
}

void UPawSkillTreeScreen::RefreshZoom()
{
	// _tick_pad_view: get_action_strength("zoom_in") - get_action_strength("zoom_out"); an action's strength is its strongest
	// event's (a held key 1, a trigger its strength past the dead zone).
	float In = bZoomInKey ? 1.0f : 0.0f;
	float Out = bZoomOutKey ? 1.0f : 0.0f;
	for (const TPair<FKey, float>& A : ZoomAxes)
	{
		In = FMath::Max(In, PawInput::Strength(TEXT("zoom_in"), A.Key, A.Value));
		Out = FMath::Max(Out, PawInput::Strength(TEXT("zoom_out"), A.Key, A.Value));
	}
	ZoomHeld = static_cast<double>(In - Out);
}

FReply UPawSkillTreeScreen::NativeOnAnalogValueChanged(const FGeometry& Geometry, const FAnalogInputEvent& Event)
{
	if (IsCeremonyPlaying())
	{
		return FReply::Handled(); // no steering or zooming under the ceremony
	}
	// Player.steer_input(): Input.get_vector over the steer actions (their keys and dead zone, from the input map), with
	// Settings > Controls > Invert X / Y. Godot's vector points +Y down, as the screen does.
	const FKey K = Event.GetKey();
	if (PawInput::KeyIs(K, TEXT("zoom_in")) || PawInput::KeyIs(K, TEXT("zoom_out")))
	{
		ZoomAxes.Add(K, Event.GetAnalogValue());
		RefreshZoom();
		return FReply::Handled();
	}
	if (PawInput::IsSteerAxis(K))
	{
		SteerAxes.Add(K, Event.GetAnalogValue());
		const FVector2f V = PawInput::SteerVector(SteerAxes);
		const UPawSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
		const bool bInvertX = Settings && Settings->Get().bInvertX;
		const bool bInvertY = Settings && Settings->Get().bInvertY;
		Stick = FVector2D(bInvertX ? -V.X : V.X, bInvertY ? -V.Y : V.Y);
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(Geometry, Event);
}

FReply UPawSkillTreeScreen::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	// A press nothing else took (the sky, the canvas between nodes): the drag.
	if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = true;
		bFollowing = false;
		DragLast = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	return Super::NativeOnMouseButtonDown(Geometry, Event);
}

FReply UPawSkillTreeScreen::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (bDragging && Event.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(Geometry, Event);
}

FReply UPawSkillTreeScreen::NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (bDragging)
	{
		const FVector2D Here = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		Pan += Here - DragLast;
		DragLast = Here;
		bViewTouched = true;
		ApplyTransform();
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(Geometry, Event);
}

FReply UPawSkillTreeScreen::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
	ZoomBy(Event.GetWheelDelta() > 0.0f ? Pc::TreeLayout::ZoomStep : -Pc::TreeLayout::ZoomStep);
	return FReply::Handled();
}

// --- test views -----------------------------------------------------------------------------------------------------------

UPawTreeNode* UPawSkillTreeScreen::GetNode(const FString& Id) const
{
	const TObjectPtr<UPawTreeNode>* N = Nodes.Find(Id);
	return N ? N->Get() : nullptr;
}

FText UPawSkillTreeScreen::GetBranchLabel(const FString& Trunk) const
{
	const TObjectPtr<UTextBlock>* N = BranchLabels.Find(Trunk);
	return N && (*N)->GetVisibility() != ESlateVisibility::Collapsed ? (*N)->GetText() : FText::GetEmpty();
}

FVector2D UPawSkillTreeScreen::GetBranchLabelAt(const FString& Trunk) const
{
	const TObjectPtr<UTextBlock>* N = BranchLabels.Find(Trunk);
	const UCanvasPanelSlot* S = N ? Cast<UCanvasPanelSlot>((*N)->Slot) : nullptr;
	return S ? S->GetPosition() : FVector2D::ZeroVector;
}

Pc::PcRect2 UPawSkillTreeScreen::GetFitBounds() const
{
	// The shown nodes and the hub (PcTreeLayout), and the room each shown branch name takes.
	const std::set<std::string> Among = ShownIds();
	const Pc::PcRect2 B = Pc::TreeBounds(Layout, &Among);
	double X0 = B.X, Y0 = B.Y, X1 = B.X + B.W, Y1 = B.Y + B.H;
	for (const TPair<FString, TObjectPtr<UTextBlock>>& KV : BranchLabels)
	{
		if (KV.Value->GetVisibility() == ESlateVisibility::Collapsed)
		{
			continue;
		}
		const FVector2D At = GetBranchLabelAt(KV.Key);
		X0 = FMath::Min(X0, At.X - BranchNameHalfW);
		X1 = FMath::Max(X1, At.X + BranchNameHalfW);
		Y0 = FMath::Min(Y0, At.Y - BranchNameHalfH);
		Y1 = FMath::Max(Y1, At.Y + BranchNameHalfH);
	}
	return Pc::PcRect2{ X0, Y0, X1 - X0, Y1 - Y0 };
}

bool UPawSkillTreeScreen::IsHaloShown(const FString& Id) const
{
	const TObjectPtr<UBorder>* H = Halos.Find(Id);
	return H && (*H)->GetVisibility() != ESlateVisibility::Collapsed;
}

FText UPawSkillTreeScreen::GetStardustText() const { return StardustText ? StardustText->GetText() : FText::GetEmpty(); }
void UPawSkillTreeScreen::SetModalStack(UCommonActivatableWidgetStack* Stack) { ModalStack = Stack; }

UPawConfirmDialog* UPawSkillTreeScreen::GetLeaveDialog() const
{
	UPawConfirmDialog* D = LeaveDialog.Get();
	return D && D->IsActivated() && !D->IsAnswered() ? D : nullptr;
}

bool UPawSkillTreeScreen::IsAskingToLeave() const { return GetLeaveDialog() != nullptr; }

bool UPawSkillTreeScreen::AnswerLeave(bool bLeave)
{
	UPawConfirmDialog* D = GetLeaveDialog();
	if (!D)
	{
		return false;
	}
	bLeave ? D->PressYes() : D->PressNo();
	return true;
}
