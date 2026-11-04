#include "UI/PawSkillTreeScreen.h"

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
#include "Engine/World.h"
#include "PcData.h"
#include "PcTree.h"
#include "PcTreeText.h"
#include "Realm/PawRealmSubsystem.h"
#include "Rendering/DrawElements.h"
#include "UI/PawButton.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawTreeTables.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"

const FName UPawSkillTreeScreen::PauseHolder(TEXT("tree"));

namespace
{
	using namespace PawUi;
	using Pc::PcTreeNodeState;

	// skill_tree_ui.gd / skill_tree.tscn / upgrade_tooltip.gd / skill_tree_connector_layer.gd (presentation constants).
	constexpr double NodePx = Pc::TreeLayout::NodeSize;  // NODE_SIZE
	constexpr double ContentTop = 130.0;                // ContentArea offset_top
	constexpr double HubPlateW = 150.0, HubPlateH = 44.0; // HUB_PLATE_SIZE
	constexpr double HaloPad = 16.0;                    // HALO_PAD
	constexpr double HaloRadius = 26.0;                 // HALO_RADIUS
	constexpr double HaloAlphaLow = 0.55, HaloAlphaHigh = 1.0; // HALO_ALPHA
	constexpr double HaloPeriodSec = 1.6;               // HALO_PERIOD_SEC
	constexpr double HaloGlowAlpha = 0.30;              // glow.bg_color = Color(Palette.GOLD, 0.30)
	constexpr double ReadyLerp = 0.34;                  // READY_LERP
	constexpr double ShinePeriodSec = 1.1;              // SHINE_PERIOD_SEC
	constexpr double CaptionWidth = NodePx + 80.0;      // CAPTION_WIDTH
	constexpr double CaptionPx = 16.0;                  // CAPTION_FONT_PX
	constexpr double CaptionGap = 6.0;                  // name_label.position.y: + NODE_SIZE.y / 2 + 6
	constexpr double CounterPx = 16.0;                  // counter font_size
	constexpr double CloseBrowseW = 60.0, CloseBrowseH = 36.0;   // CLOSE_BROWSE_SIZE
	constexpr double CloseBrowseMarginX = 20.0, CloseBrowseMarginY = 60.0; // CLOSE_BROWSE_MARGIN
	constexpr double CloseSpendW = 420.0, CloseSpendH = 92.0;    // CLOSE_SPEND_SIZE
	constexpr double CloseSpendGapBottom = 40.0;        // CLOSE_SPEND_GAP_BOTTOM
	constexpr double CloseSpendPx = 34.0;               // the Continue caption's font_size
	constexpr double ZoomButtonPx = 44.0;               // ZoomIn custom_minimum_size
	constexpr double ZoomBoxX = 24.0, ZoomBoxY = 150.0; // ZoomControls position
	constexpr double WarnPx = 24.0;                     // the unspent plate's text
	constexpr double WarnTop = 0.32;                    // its y: view.y * 0.32
	constexpr double TipWidth = 380.0;                  // UpgradeTooltip WIDTH_PX
	constexpr double TipMargin = 14.0;                  // MARGIN_PX
	constexpr double TipTitlePx = 20.0;                 // its title font_size
	constexpr double TipDockX = 24.0, TipDockY = 72.0;  // TIP_DOCK_INSET
	constexpr double FollowBox = 0.6;                   // FOLLOW_BOX
	constexpr double FollowSec = 0.18;                  // FOLLOW_SEC
	constexpr double PanPxPerSec = 1400.0;              // PAN_PX_PER_SEC
	constexpr double ZoomPerSec = 1.2;                  // ZOOM_PER_SEC
	constexpr double StickDead = 0.2;                   // the pad's steer dead zone
	constexpr double LineWidth = 5.0, LineWidthOwned = 7.0; // LINE_WIDTH, LINE_WIDTH_OWNED
	constexpr double RainbowInset = 7.0, RainbowWidth = 5.0, RainbowCorner = 20.0; // RAINBOW_*
	constexpr int32 RainbowSegments = 10;
	constexpr double RainbowTurnsPerSec = 0.12;
	constexpr int32 PulseRings = 3;                     // PULSE_RINGS
	constexpr double PulseStep = 13.0, PulseWidth = 6.0, PulseHz = 0.8; // PULSE_*

	std::string Utf8(const FString& S) { return std::string(TCHAR_TO_UTF8(*S)); }

	FLinearColor TrunkColour(const FString& Id)
	{
		for (const PawTree::FBranchColour& C : PawTree::BranchColours)
		{
			if (Id == C.Id)
			{
				return C.Colour;
			}
		}
		return FLinearColor::White; // BRANCH_COLORS.get(id, Color.WHITE)
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

	// Godot's Color.lerp works on the stored (sRGB) values: lerp there, alpha linear.
	FLinearColor LerpSrgb(const FLinearColor& A, const FLinearColor& B, double T)
	{
		const FColor SA = A.ToFColorSRGB(), SB = B.ToFColorSRGB();
		auto Mix = [T](uint8 X, uint8 Y) { return static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<double>(X), static_cast<double>(Y), T))); };
		FLinearColor Out = PawTokens::Srgb(Mix(SA.R, SB.R), Mix(SA.G, SB.G), Mix(SA.B, SB.B));
		Out.A = static_cast<float>(FMath::Lerp(static_cast<double>(A.A), static_cast<double>(B.A), T));
		return Out;
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
	TArray<FVector2D> Pts;
	const double R = RainbowCorner;
	const FVector2D Corners[4] = { C + FVector2D(Half.X - R, -Half.Y + R), C + FVector2D(Half.X - R, Half.Y - R),
		C + FVector2D(-Half.X + R, Half.Y - R), C + FVector2D(-Half.X + R, -Half.Y + R) };
	const double Start[4] = { -UE_DOUBLE_PI / 2.0, 0.0, UE_DOUBLE_PI / 2.0, UE_DOUBLE_PI };
	for (int32 K = 0; K < 4; ++K)
	{
		for (int32 I = 0; I <= RainbowSegments; ++I)
		{
			const double A = Start[K] + (UE_DOUBLE_PI / 2.0) * static_cast<double>(I) / RainbowSegments;
			Pts.Add(Corners[K] + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
		}
	}
	const FVector2D First = Pts[0]; // a copy: adding an element of the array to itself may reallocate under it (Unreal asserts)
	Pts.Add(First);
	return Pts;
}

int32 UPawTreeLinks::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Culling, FSlateWindowElementList& Out,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, G, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	// Unowned first, so an owned branch is never crossed by a grey line drawn over it (two passes, as Godot).
	for (const bool bPassOwned : { false, true })
	{
		for (const FLink& L : Links)
		{
			if (L.bOwned != bPassOwned)
			{
				continue;
			}
			const TArray<FVector2D> Seg{ L.A, L.B };
			if (L.bShut)
			{
				Lines(Out, Layer, G, Seg, PawTree::LinkShut, LineWidth * 0.7); // "you cannot" darker than "not yet"
			}
			else
			{
				Lines(Out, Layer, G, Seg, bPassOwned ? L.Colour : PawTree::LinkDim, bPassOwned ? LineWidthOwned : LineWidth);
			}
		}
	}
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
		FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Pts, Colours, ESlateDrawEffect::None, FLinearColor::White, true, static_cast<float>(Width));
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
	for (const FVector2D& C : Rainbow)
	{
		DrawRainbow(C, Half, RainbowWidth, 1.0);
	}
	for (const FVector2D& C : JobRings)
	{
		Lines(Out, Layer, G, RingPath(C, Half), PawTokens::GOLD, RainbowWidth); // JOB_RING_COLOUR
	}
	return Layer;
}

// --- a node -------------------------------------------------------------------------------------------------------------

UPawTreeNodeStyle::UPawTreeNodeStyle()
{
	// The frame is the node's own border; the button draws only the selection ring (focus = selection, docs/05 §2 rule 10).
	NormalBase = NoBrush();
	NormalHovered = Rounded(FLinearColor::Transparent, PawTokens::UI_FOCUS, 3.0f, 6.0f);
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
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(static_cast<float>(NodePx));
		Box->SetHeightOverride(static_cast<float>(NodePx));
		WidgetTree->RootWidget = Box;
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		Box->SetContent(Root);
		Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* FS = Root->AddChildToOverlay(Frame);
		FS->SetHorizontalAlignment(HAlign_Fill);
		FS->SetVerticalAlignment(VAlign_Fill);
		// Ladders carry "3/10" in the bottom right; unlocks carry nothing.
		CounterText = MakeText(WidgetTree, CounterPx, FLinearColor::White);
		UOverlaySlot* CS = Root->AddChildToOverlay(CounterText);
		CS->SetHorizontalAlignment(HAlign_Right);
		CS->SetVerticalAlignment(VAlign_Bottom);
		CS->SetPadding(FMargin(4.0f, 0.0f, 6.0f, 4.0f));
	}
	const bool bOk = Super::Initialize();
	SetRenderTransformPivot(FVector2D(0.5, 0.5));
	return bOk;
}

void UPawTreeNode::FrameColours(PcTreeNodeState S, const FLinearColor& Trunk, FLinearColor& OutFill, FLinearColor& OutEdge)
{
	// The plate one step above the sky, and owned FROM the plate toward the trunk's colour (never dimmed toward black,
	// which could land darker than LOCKED and invert the ladder).
	const FLinearColor DarkBody = WithAlpha(PawTokens::SKY_PLATE, 0.94);
	const FLinearColor OwnedBody = WithAlpha(LerpSrgb(PawTokens::SKY_PLATE, Trunk, 0.42), 0.95);
	const FLinearColor DimBorder = WithAlpha(Trunk, 0.45);
	switch (S)
	{
	case PcTreeNodeState::Owned:
		OutFill = OwnedBody;
		OutEdge = Trunk;
		break;
	case PcTreeNodeState::Ready:
		OutFill = LerpSrgb(DarkBody, OwnedBody, ReadyLerp);
		OutEdge = LerpSrgb(DimBorder, Trunk, ReadyLerp);
		break;
	default:
		OutFill = DarkBody;
		OutEdge = DimBorder;
		break;
	}
}

void UPawTreeNode::SetLook(PcTreeNodeState InState, const FLinearColor& Trunk, bool bUnlock, const FText& Counter, bool bInShine)
{
	State = InState;
	FLinearColor Fill, Edge;
	FrameColours(InState, Trunk, Fill, Edge);
	// Unlocks keep their heavier border and rounder corners in every state.
	Frame->SetBrush(Rounded(Fill, Edge, bUnlock ? 5.0f : 3.0f, bUnlock ? 14.0f : 4.0f));
	CounterText->SetText(Counter);
	const double CounterAlpha = InState == PcTreeNodeState::Owned ? 0.95 : InState == PcTreeNodeState::Ready ? 0.72 : 0.45;
	SetTextColour(CounterText, WithAlpha(FLinearColor::White, CounterAlpha));
	if (bShine != bInShine)
	{
		bShine = bInShine;
		ShineClock = 0.0;
		SetColorAndOpacity(FLinearColor::White);
	}
}

void UPawTreeNode::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (bShine)
	{
		// An unbought unlock you can buy now shines (self_modulate to SHINE_COLOR and back).
		ShineClock = FMath::Fmod(ShineClock + DeltaSeconds, ShinePeriodSec);
		const double T = 0.5 - 0.5 * FMath::Cos(ShineClock / ShinePeriodSec * UE_DOUBLE_TWO_PI);
		SetColorAndOpacity(FLinearColor::White + (PawTree::ShineTint - FLinearColor::White) * static_cast<float>(T));
	}
}

FText UPawTreeNode::GetCounter() const { return CounterText ? CounterText->GetText() : FText::GetEmpty(); }
FLinearColor UPawTreeNode::GetFrameFill() const { return Frame ? Frame->Background.TintColor.GetSpecifiedColor() : FLinearColor::Transparent; }
FLinearColor UPawTreeNode::GetFrameEdge() const { return Frame ? Frame->Background.OutlineSettings.Color.GetSpecifiedColor() : FLinearColor::Transparent; }

// --- the screen -----------------------------------------------------------------------------------------------------------

namespace
{
	FString GLastSelected; // the node the tree was left on, across opens (Godot's static var _last_selected)
}

FString UPawSkillTreeScreen::LastSelected() { return GLastSelected; }

bool UPawSkillTreeScreen::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;
		// Its own opaque backdrop (docs/05 §2 rule 11): the tree's night sky (the stars of skill_tree_sky.gd wait for
		// phase 6). Visible: it takes the drag and the wheel, and nothing reaches the field.
		UBorder* Sky = WidgetTree->ConstructWidget<UBorder>();
		Sky->SetBrush(Rounded(PawTokens::SKY_BASE, PawTokens::SKY_BASE, 0.0f, 0.0f));
		Sky->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* SkySlot = Root->AddChildToOverlay(Sky);
		SkySlot->SetHorizontalAlignment(HAlign_Fill);
		SkySlot->SetVerticalAlignment(VAlign_Fill);
		// The content area: below the header, clipped; the canvas under a render transform inside it.
		UBorder* Content = WidgetTree->ConstructWidget<UBorder>();
		Content->SetBrush(NoBrush());
		Content->SetPadding(FMargin(0.0f, static_cast<float>(ContentTop), 0.0f, 0.0f));
		Content->SetClipping(EWidgetClipping::ClipToBounds);
		Content->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* ContentSlot = Root->AddChildToOverlay(Content);
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Canvas->SetRenderTransformPivot(FVector2D(0.0, 0.0));
		Content->SetContent(Canvas);
		BuildChrome(Root);
	}
	return Super::Initialize();
}

void UPawSkillTreeScreen::BuildChrome(UOverlay* Root)
{
	UWidgetTree* T = WidgetTree;
	// The balance on the sky, not on a panel: paper, not ink.
	StardustText = MakeText(T, LabelPx, PawTokens::SKY_STAR);
	UOverlaySlot* SS = Root->AddChildToOverlay(StardustText);
	SS->SetHorizontalAlignment(HAlign_Right);
	SS->SetVerticalAlignment(VAlign_Top);
	SS->SetPadding(FMargin(0.0f, 20.0f, 20.0f, 0.0f));

	UVerticalBox* ZoomBox = T->ConstructWidget<UVerticalBox>();
	UOverlaySlot* ZS = Root->AddChildToOverlay(ZoomBox);
	ZS->SetHorizontalAlignment(HAlign_Left);
	ZS->SetVerticalAlignment(VAlign_Top);
	ZS->SetPadding(FMargin(static_cast<float>(ZoomBoxX), static_cast<float>(ZoomBoxY), 0.0f, 0.0f));
	for (const bool bIn : { true, false })
	{
		UPawButton* B = T->ConstructWidget<UPawButton>(UPawButton::StaticClass());
		B->SetLabel(PawText::Ui(bIn ? TEXT("TREE_zoom_in") : TEXT("TREE_zoom_out")));
		B->SetMinDimensions(static_cast<int32>(ZoomButtonPx), static_cast<int32>(ZoomButtonPx));
		B->SetIsFocusable(false); // the mouse's; the pad zooms on the triggers
		B->OnClicked().AddWeakLambda(this, [this, bIn]() { ZoomBy(bIn ? Pc::TreeLayout::ZoomStep : -Pc::TreeLayout::ZoomStep); });
		ZoomBox->AddChildToVerticalBox(B)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	Legend = MakeText(T, LabelPx, PawTokens::SKY_STAR);
	Legend->SetText(PawText::Ui(TEXT("TREE_legend"), PawText::PromptArgs()));
	UOverlaySlot* LS = Root->AddChildToOverlay(Legend);
	LS->SetHorizontalAlignment(HAlign_Center);
	LS->SetVerticalAlignment(VAlign_Bottom);
	LS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	// The unspent warning, in the middle (playtest 2026-10-01 #32: the caption change at the bottom went unseen on a pad).
	WarningPlate = T->ConstructWidget<UBorder>();
	WarningPlate->SetBrush(Rounded(PawTokens::UI_PANEL, PawTokens::INK, 3.0f, 8.0f));
	WarningPlate->SetPadding(FMargin(18.0f));
	WarningPlate->SetVisibility(ESlateVisibility::Collapsed);
	WarningText = MakeText(T, WarnPx, PawTokens::INK);
	WarningText->SetJustification(ETextJustify::Center);
	WarningPlate->SetContent(WarningText);
	UOverlaySlot* WS = Root->AddChildToOverlay(WarningPlate);
	WS->SetHorizontalAlignment(HAlign_Center);
	WS->SetVerticalAlignment(VAlign_Top);

	CloseButton = T->ConstructWidget<UPawButton>(UPawButton::StaticClass());
	CloseButton->SetIsFocusable(false); // Back / B close it on the keys and pad; the selection stays on the tree
	CloseButton->OnClicked().AddWeakLambda(this, [this]() { PressClose(); });
	CloseSlot = Root->AddChildToOverlay(CloseButton);

	// The tooltip, placed absolutely over everything.
	UCanvasPanel* TipLayer = T->ConstructWidget<UCanvasPanel>();
	TipLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UOverlaySlot* TL = Root->AddChildToOverlay(TipLayer);
	TL->SetHorizontalAlignment(HAlign_Fill);
	TL->SetVerticalAlignment(VAlign_Fill);
	TipPanel = T->ConstructWidget<UBorder>();
	TipPanel->SetBrush(Rounded(PawTokens::UI_PANEL, PawTokens::INK, 3.0f, 8.0f));
	TipPanel->SetPadding(FMargin(16.0f, 12.0f));
	TipPanel->SetVisibility(ESlateVisibility::Collapsed);
	UVerticalBox* TipBox = T->ConstructWidget<UVerticalBox>();
	TipPanel->SetContent(TipBox);
	TipTitle = MakeText(T, TipTitlePx, PawTokens::INK);
	TipBox->AddChildToVerticalBox(TipTitle)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	for (int32 I = 0; I < 7; ++I)
	{
		UTextBlock* Line = MakeText(T, LabelPx, PawTokens::INK);
		Line->SetAutoWrapText(false);
		Line->SetWrapTextAt(static_cast<float>(TipWidth - 32.0));
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

	LinksLayer = T->ConstructWidget<UPawTreeLinks>(UPawTreeLinks::StaticClass());
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
	// Halos first (under the nodes).
	for (const FString& Id : Order)
	{
		UBorder* Halo = T->ConstructWidget<UBorder>();
		Halo->SetBrush(Rounded(WithAlpha(PawTokens::GOLD, HaloGlowAlpha), WithAlpha(PawTokens::GOLD, HaloGlowAlpha), 0.0f, static_cast<float>(HaloRadius)));
		Halo->SetVisibility(ESlateVisibility::Collapsed);
		Place(Halo, V(Layout.Nodes.at(Utf8(Id))), FVector2D(NodePx + 2.0 * HaloPad));
		Halos.Add(Id, Halo);
	}
	// The hub: a pill just big enough for its words, on the sky, belonging to no trunk.
	UBorder* Hub = T->ConstructWidget<UBorder>();
	Hub->SetBrush(Rounded(WithAlpha(PawTokens::SKY_PLATE, 0.96), WithAlpha(PawTokens::SKY_PLATE_EDGE, 0.8), 2.0f, 18.0f));
	Hub->SetHorizontalAlignment(HAlign_Center);
	Hub->SetVerticalAlignment(VAlign_Center);
	Hub->SetVisibility(ESlateVisibility::HitTestInvisible);
	UTextBlock* HubText = MakeText(T, LabelPx, PawTokens::SKY_STAR);
	HubText->SetText(PawText::Ui(TEXT("TREE_hub")));
	Hub->SetContent(HubText);
	Place(Hub, V(Layout.Hub), FVector2D(HubPlateW, HubPlateH));

	for (const FString& Id : Order)
	{
		const FVector2D C = V(Layout.Nodes.at(Utf8(Id)));
		UPawTreeNode* N = T->ConstructWidget<UPawTreeNode>(UPawTreeNode::StaticClass());
		N->BranchId = Id;
		N->OnClicked().AddWeakLambda(this, [this, Id]() { PressNode(Id); });
		Place(N, C, FVector2D(NodePx));
		Nodes.Add(Id, N);
		// The caption below the square, two lines at a fixed width, outlined in the sky so a trunk line under it reads.
		UTextBlock* Cap = MakeText(T, CaptionPx, PawTokens::SKY_STAR);
		FSlateFontInfo Font = Cap->GetFont();
		Font.OutlineSettings.OutlineSize = static_cast<int32>(OverFieldOutline);
		Font.OutlineSettings.OutlineColor = WithAlpha(PawTokens::SKY_BASE, 0.95);
		Cap->SetFont(Font);
		Cap->SetJustification(ETextJustify::Center);
		Cap->SetAutoWrapText(false);
		Cap->SetWrapTextAt(static_cast<float>(CaptionWidth));
		Cap->SetText(PawText::TreeLabel(Utf8(Id)));
		UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Cap);
		CS->SetPosition(FVector2D(C.X - CaptionWidth * 0.5, C.Y + NodePx * 0.5 + CaptionGap));
		CS->SetSize(FVector2D(CaptionWidth, CaptionPx * 3.4));
		Captions.Add(Id, Cap);
	}
	WriteNavigation();
}

void UPawSkillTreeScreen::WriteNavigation()
{
	// nav_pick() once per layout, written as explicit rules: no node that way = stay put (never off the tree).
	const Pc::PcVec2 Dirs[4] = { { 0.0, -1.0 }, { 0.0, 1.0 }, { -1.0, 0.0 }, { 1.0, 0.0 } };
	const EUINavigation Nav[4] = { EUINavigation::Up, EUINavigation::Down, EUINavigation::Left, EUINavigation::Right };
	for (int32 I = 0; I < Order.Num(); ++I)
	{
		UPawTreeNode* N = Nodes[Order[I]];
		for (int32 K = 0; K < 4; ++K)
		{
			const std::string Pick = Pc::TreeNavPick(Layout, Utf8(Order[I]), Dirs[K]);
			N->SetNavigationRuleExplicit(Nav[K], Pick.empty() ? N : Nodes[UTF8_TO_TCHAR(Pick.c_str())].Get());
		}
		N->SetNavigationRuleExplicit(EUINavigation::Next, Nodes[Order[(I + 1) % Order.Num()]]);
		N->SetNavigationRuleExplicit(EUINavigation::Previous, Nodes[Order[(I - 1 + Order.Num()) % Order.Num()]]);
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
	}
}

void UPawSkillTreeScreen::ApplyTransform()
{
	FWidgetTransform T;
	T.Translation = Pan;
	T.Scale = FVector2D(Zoom, Zoom);
	Canvas->SetRenderTransform(T);
}

void UPawSkillTreeScreen::FitAndCentre()
{
	const FVector2D View = ContentSize();
	const Pc::PcRect2 B = Pc::TreeBounds(Layout);
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
		const UPawGameSubsystem* G = Game();
		const bool bUnspent = G && G->IsDataReady() && Pc::HasAffordableBranch(G->Data(), G->Progress());
		CloseButton->SetLabel(PawText::Ui(!bConfirmLeaving ? TEXT("TREE_continue") : bUnspent ? TEXT("TREE_leave_unspent") : TEXT("TREE_start_next")));
	}
	else
	{
		CloseSlot->SetHorizontalAlignment(HAlign_Right);
		CloseSlot->SetVerticalAlignment(VAlign_Top);
		CloseSlot->SetPadding(FMargin(0.0f, static_cast<float>(CloseBrowseMarginY), static_cast<float>(CloseBrowseMarginX), 0.0f));
		CloseButton->SetMinDimensions(static_cast<int32>(CloseBrowseW), static_cast<int32>(CloseBrowseH));
		CloseButton->SetLabelSize(ButtonPx);
		CloseButton->SetLabel(PawText::Ui(TEXT("TREE_close")));
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
		FText Counter;
		if (!B->bIsUnlock)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("owned"), PawText::Count(Owned));
			Args.Add(TEXT("max"), PawText::Count(Pc::BranchMaxLevel(*B)));
			Counter = PawText::Ui(TEXT("TREE_counter"), Args);
		}
		// Shine: an unlock BUYABLE and unbought, in the spend step (a locked one would invite a click that does nothing).
		const bool bShine = B->bIsUnlock && Owned == 0 && Pc::TreeCanBuyNow(D, P, S) && bSpendMode;
		Nodes[Id]->SetLook(State, TrunkColour(Id), B->bIsUnlock, Counter, bShine);
		Halos[Id]->SetVisibility(State == PcTreeNodeState::Ready ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	// The links (an edge is owned only when both ends are bought), the rainbow verbs, the breathing Unicorn, the job rings.
	LinksLayer->Links.Reset();
	for (const Pc::PcTreeLink& L : Pc::TreeLinks(D, P))
	{
		UPawTreeLinks::FLink Out;
		Out.A = L.From == "hub" ? V(Layout.Hub) : V(Layout.Nodes.at(L.From));
		Out.B = V(Layout.Nodes.at(L.To));
		Out.Colour = TrunkColour(UTF8_TO_TCHAR(L.To.c_str()));
		Out.bOwned = L.bOwned;
		Out.bShut = L.bShut;
		LinksLayer->Links.Add(Out);
	}
	LinksLayer->Rainbow.Reset();
	LinksLayer->Pulse.Reset();
	LinksLayer->JobRings.Reset();
	for (const FString& Id : Order)
	{
		const FVector2D C = V(Layout.Nodes.at(Utf8(Id)));
		if (InList(Id, PawTree::PulseBranches, UE_ARRAY_COUNT(PawTree::PulseBranches)))
		{
			LinksLayer->Pulse.Add(C);
		}
		if (InList(Id, PawTree::RainbowBranches, UE_ARRAY_COUNT(PawTree::RainbowBranches)))
		{
			LinksLayer->Rainbow.Add(C);
		}
		const Pc::PcSkillBranch* B = D.FindBranch(Utf8(Id));
		if (B && !B->JobId.empty())
		{
			LinksLayer->JobRings.Add(C);
		}
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
		Lines.Add({ Effect(Tip.Owned), PawTokens::INK });
	}
	else
	{
		if (Tip.bShowNow)
		{
			FFormatNamedArguments A;
			A.Add(TEXT("effect"), Effect(Tip.Owned));
			Lines.Add({ PawText::Ui(TEXT("TREE_now"), A), PawTokens::INK });
		}
		FFormatNamedArguments A;
		A.Add(TEXT("effect"), Effect(Tip.Next));
		Lines.Add({ PawText::Ui(TEXT("TREE_next"), A), PawTree::TipNext });
		FFormatNamedArguments C;
		C.Add(TEXT("n"), PawText::Count(Tip.Cost));
		Lines.Add({ PawText::Ui(TEXT("TREE_cost"), C), PawTokens::INK });
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
		Lines.Add({ PawText::Ui(TEXT("TREE_buy"), PawText::PromptArgs()), PawTokens::INK });
	}
	for (int32 I = 0; I < TipLines.Num(); ++I)
	{
		const bool bShow = I < Lines.Num();
		TipLines[I]->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShow)
		{
			TipLines[I]->SetText(Lines[I].Key);
			SetTextColour(TipLines[I], Lines[I].Value);
		}
	}
	TipPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	TipPanel->ForceLayoutPrepass();
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

TArray<FText> UPawSkillTreeScreen::GetTipLines() const
{
	TArray<FText> Out;
	if (TipPanel && TipPanel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		for (const UTextBlock* L : TipLines)
		{
			if (L->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Out.Add(L->GetText());
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
	if (!bSpendMode || !G || !G->IsDataReady())
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
	if (Bought.bAutoCardOn)
	{
		G->Slot().bAutoPickCard = true;
	}
	if (UPawRealmSubsystem* R = Realm())
	{
		R->OnProgressChanged(true); // Vitality and the like change the kittens; every level the stats
	}
	// Spending resets the prompt: a warning left armed from an earlier press would fire on the wrong one.
	bConfirmLeaving = false;
	WarningPlate->SetVisibility(ESlateVisibility::Collapsed);
	RefreshAll();
	ShowTip(Id, bTipDocked); // the tooltip under the cursor shows the next level's price at once (designer 2026-08-23)
	return true;
}

void UPawSkillTreeScreen::PressClose()
{
	// ALWAYS asks after a Reawaken (designer 2026-10-01): leaving starts the next run.
	if (bSpendMode && !bConfirmLeaving)
	{
		bConfirmLeaving = true;
		const UPawGameSubsystem* G = Game();
		const bool bUnspent = G && G->IsDataReady() && Pc::HasAffordableBranch(G->Data(), G->Progress());
		WarningText->SetText(PawText::Ui(bUnspent ? TEXT("TREE_warn_unspent") : TEXT("TREE_warn_next")));
		WarningPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		StyleClose();
		return;
	}
	ShowTip(FString(), false);
	bConfirmLeaving = false;
	WarningPlate->SetVisibility(ESlateVisibility::Collapsed);
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
	const std::string Next = Pc::NextAffordableBranch(G->Data(), G->Progress(), Utf8(GLastSelected));
	if (Next.empty())
	{
		return FString();
	}
	const FString Id = UTF8_TO_TCHAR(Next.c_str());
	GLastSelected = Id;
	if (CanTakeFocus() && Nodes.Contains(Id))
	{
		Nodes[Id]->SetFocus();
	}
	OnNodeFocused(Id);
	return Id;
}

void UPawSkillTreeScreen::OnNodeFocused(const FString& Id)
{
	GLastSelected = Id;
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
	if ((Id.IsEmpty() || !Nodes.Contains(Id)) && Nodes.Contains(GLastSelected))
	{
		Id = GLastSelected;
	}
	if ((Id.IsEmpty() || !Nodes.Contains(Id)) && Pc::TrunkWheel().size() > 0)
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
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, true);
	}
	BuildCanvas();
	bConfirmLeaving = false;
	WarningPlate->SetVisibility(ESlateVisibility::Collapsed);
	ShowTip(FString(), false);
	// Recentred on every open: a player who dragged the canvas somewhere odd still finds the tree where they expect it.
	FitAndCentre();
	RefreshAll();
	if (const UWidget* Target = NativeGetDesiredFocusTarget())
	{
		if (const UPawTreeNode* N = Cast<UPawTreeNode>(Target))
		{
			GLastSelected = N->BranchId;
		}
	}
	Super::NativeOnActivated();
}

void UPawSkillTreeScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, false);
	}
}

void UPawSkillTreeScreen::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	SetViewSize(FVector2D(Geometry.GetLocalSize()));
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
	// The pad: right stick pans, the triggers (or = and -) zoom, both held.
	if (Stick.Size() > StickDead)
	{
		bFollowing = false;
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
	// The selection follows focus continuously (spike S4); the tooltip follows the mouse's node or docks for the pad.
	FString Hovered, Focused;
	for (const TPair<FString, TObjectPtr<UPawTreeNode>>& N : Nodes)
	{
		if (N.Value->IsHovered())
		{
			Hovered = N.Key;
		}
		if (N.Value->HasAnyUserFocus() || N.Value->HasFocusedDescendants())
		{
			Focused = N.Key;
		}
	}
	if (!Focused.IsEmpty() && Focused != GLastSelected)
	{
		OnNodeFocused(Focused);
	}
	if (!Hovered.IsEmpty())
	{
		ShowTip(Hovered, false);
	}
	else if (!TipBranch.IsEmpty() && !bTipDocked)
	{
		ShowTip(FString(), false);
	}
	else if (!TipBranch.IsEmpty())
	{
		ShowTip(TipBranch, true);
	}
	if (WarningPlate->GetVisibility() != ESlateVisibility::Collapsed)
	{
		if (UOverlaySlot* WS = Cast<UOverlaySlot>(WarningPlate->Slot))
		{
			WS->SetPadding(FMargin(0.0f, static_cast<float>(ViewSize.Y * WarnTop), 0.0f, 0.0f));
		}
	}
}

FReply UPawSkillTreeScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// Keys until phase 5's Enhanced Input actions (docs/05 §3): back / open_skill_tree close (the spend step asks once);
	// buy_amount jumps; = / - and the triggers zoom.
	const FKey K = KeyEvent.GetKey();
	if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right || K == EKeys::T || K == EKeys::Gamepad_FaceButton_Top)
	{
		if (!KeyEvent.IsRepeat())
		{
			PressClose();
		}
		return FReply::Handled();
	}
	if (K == EKeys::Q || K == EKeys::Gamepad_FaceButton_Left)
	{
		JumpToAffordable();
		return FReply::Handled();
	}
	if (K == EKeys::Equals || K == EKeys::Add || K == EKeys::Gamepad_RightTrigger)
	{
		ZoomHeld = 1.0;
		return FReply::Handled();
	}
	if (K == EKeys::Hyphen || K == EKeys::Subtract || K == EKeys::Gamepad_LeftTrigger)
	{
		ZoomHeld = -1.0;
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

FReply UPawSkillTreeScreen::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	const FKey K = KeyEvent.GetKey();
	if (K == EKeys::Equals || K == EKeys::Add || K == EKeys::Gamepad_RightTrigger || K == EKeys::Hyphen || K == EKeys::Subtract
		|| K == EKeys::Gamepad_LeftTrigger)
	{
		ZoomHeld = 0.0;
		return FReply::Handled();
	}
	return Super::NativeOnKeyUp(Geometry, KeyEvent);
}

FReply UPawSkillTreeScreen::NativeOnAnalogValueChanged(const FGeometry& Geometry, const FAnalogInputEvent& Event)
{
	const FKey K = Event.GetKey();
	if (K == EKeys::Gamepad_RightX)
	{
		Stick.X = Event.GetAnalogValue();
		return FReply::Handled();
	}
	if (K == EKeys::Gamepad_RightY)
	{
		Stick.Y = -Event.GetAnalogValue(); // stick up is +Y; the screen's up is -Y
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

FText UPawSkillTreeScreen::GetCaption(const FString& Id) const
{
	const TObjectPtr<UTextBlock>* C = Captions.Find(Id);
	return C ? (*C)->GetText() : FText::GetEmpty();
}

bool UPawSkillTreeScreen::IsHaloShown(const FString& Id) const
{
	const TObjectPtr<UBorder>* H = Halos.Find(Id);
	return H && (*H)->GetVisibility() != ESlateVisibility::Collapsed;
}

FText UPawSkillTreeScreen::GetStardustText() const { return StardustText ? StardustText->GetText() : FText::GetEmpty(); }
FText UPawSkillTreeScreen::GetWarningText() const { return WarningText ? WarningText->GetText() : FText::GetEmpty(); }
bool UPawSkillTreeScreen::IsWarningShown() const { return WarningPlate && WarningPlate->GetVisibility() != ESlateVisibility::Collapsed; }
