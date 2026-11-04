#include "Dev/Spikes/SpikeUi.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Core/PawLog.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "UnrealClient.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace SpikeS4
{
	const TArray<int32> PageRows = { 15, 3, 12, 6, 10, 6 }; // Upgrades, Reawaken, Realms, Catches, Achievements, Jobs
	const TCHAR* TabNames[] = { TEXT("Upgrades"), TEXT("Reawaken"), TEXT("Realms"), TEXT("Catches"), TEXT("Achievements"), TEXT("Jobs") };
	constexpr int32 HexRadius = 5;
	constexpr double NodeSize = 48.0;

	bool HasFocus(const UWidget* W)
	{
		return W && (W->HasAnyUserFocus() || W->HasFocusedDescendants());
	}

	FSlateBrush Rounded(const FLinearColor& Fill, const FLinearColor& Outline, float Width)
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.TintColor = FSlateColor(Fill);
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		B.OutlineSettings.CornerRadii = FVector4(8.0, 8.0, 8.0, 8.0);
		B.OutlineSettings.Color = FSlateColor(Outline);
		B.OutlineSettings.Width = Width;
		return B;
	}

	bool IsBack(const FKey& K) { return K == EKeys::Gamepad_FaceButton_Right || K == EKeys::Escape; }
}

// --- button ----------------------------------------------------------------------------------------------------------

USpikeButtonStyle::USpikeButtonStyle()
{
	const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("FFF9E3")));
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("1A1A1A")));
	const FLinearColor Gold = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("F2C65B")));
	NormalBase = SpikeS4::Rounded(Paper, Ink, 2.0f);
	NormalHovered = SpikeS4::Rounded(Gold, Ink, 4.0f); // focus = selection: the one highlight (docs/05 §6.1)
	NormalPressed = SpikeS4::Rounded(Gold, Ink, 2.0f);
	SelectedBase = NormalBase;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalPressed;
	Disabled = SpikeS4::Rounded(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("C7C1AB"))), Ink, 2.0f);
	ButtonPadding = FMargin(12.0f, 6.0f);
}

USpikeButton::USpikeButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetStyle(USpikeButtonStyle::StaticClass());
}

bool USpikeButton::Initialize()
{
	// Content must exist BEFORE Super::Initialize: UCommonButtonBase::Initialize wraps WidgetTree->RootWidget in its internal
	// button, and UUserWidget::Initialize keeps a widget tree that already exists (UE 5.8.3 source).
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		Label = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 16;
		Label->SetFont(Font);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("1A1A1A")))));
		WidgetTree->RootWidget = Label;
	}
	return Super::Initialize();
}

void USpikeButton::SetLabel(const FString& Text)
{
	if (Label)
	{
		Label->SetText(FText::FromString(Text));
	}
}

// --- HUD + dock ------------------------------------------------------------------------------------------------------

bool USpikeHudScreen::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = RootOverlay;

		UVerticalBox* Dock = WidgetTree->ConstructWidget<UVerticalBox>();
		UOverlaySlot* DockSlot = RootOverlay->AddChildToOverlay(Dock);
		DockSlot->SetHorizontalAlignment(HAlign_Left);
		DockSlot->SetVerticalAlignment(VAlign_Top);
		DockSlot->SetPadding(FMargin(16.0f));

		UHorizontalBox* Strip = WidgetTree->ConstructWidget<UHorizontalBox>();
		Dock->AddChildToVerticalBox(Strip)->SetPadding(FMargin(0, 0, 0, 8));
		Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>();
		Dock->AddChildToVerticalBox(Pages);

		for (int32 P = 0; P < SpikeS4::PageRows.Num(); ++P)
		{
			USpikeButton* TabButton = WidgetTree->ConstructWidget<USpikeButton>(USpikeButton::StaticClass());
			TabButton->Id = FString::Printf(TEXT("tab:%d"), P);
			TabButton->SetLabel(SpikeS4::TabNames[P]);
			TabButton->OnClicked().AddLambda([this, P]() { SelectTab(P); });
			Strip->AddChildToHorizontalBox(TabButton)->SetPadding(FMargin(0, 0, 4, 0));
			Tabs.Add(TabButton);
			AllButtons.Add(TabButton);

			UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>();
			Pages->AddChild(Page);
			TArray<TObjectPtr<USpikeButton>>& PageRowsOut = Rows.AddDefaulted_GetRef();
			for (int32 R = 0; R < SpikeS4::PageRows[P]; ++R)
			{
				USpikeButton* Row = WidgetTree->ConstructWidget<USpikeButton>(USpikeButton::StaticClass());
				Row->Id = FString::Printf(TEXT("row:%d:%d"), P, R);
				Row->SetLabel(FString::Printf(TEXT("%s %02d   Lv 0   100 catnip"), SpikeS4::TabNames[P], R + 1));
				Page->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 0, 0, 4));
				PageRowsOut.Add(Row);
				AllButtons.Add(Row);
			}
		}

		// Explicit navigation, written once (docs/05 §6.1): rows up/down, the top row up to its tab, tabs left/right.
		for (int32 P = 0; P < Rows.Num(); ++P)
		{
			for (int32 R = 0; R < Rows[P].Num(); ++R)
			{
				USpikeButton* Row = Rows[P][R];
				Row->SetNavigationRuleExplicit(EUINavigation::Up, R > 0 ? Rows[P][R - 1].Get() : Tabs[P].Get());
				if (R + 1 < Rows[P].Num())
				{
					Row->SetNavigationRuleExplicit(EUINavigation::Down, Rows[P][R + 1]);
				}
				else
				{
					Row->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop);
				}
				Row->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
				Row->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop);
			}
			Tabs[P]->SetNavigationRuleBase(EUINavigation::Up, EUINavigationRule::Stop);
			if (P > 0) { Tabs[P]->SetNavigationRuleExplicit(EUINavigation::Left, Tabs[P - 1]); }
			else { Tabs[P]->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop); }
			if (P + 1 < Tabs.Num()) { Tabs[P]->SetNavigationRuleExplicit(EUINavigation::Right, Tabs[P + 1]); }
			else { Tabs[P]->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop); }
		}

		// Status row over the field: never takes input (docs/05 §6.1).
		Status = WidgetTree->ConstructWidget<UTextBlock>();
		Status->SetText(FText::FromString(TEXT("Catnip 1.2K    Stardust 3")));
		Status->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* StatusSlot = RootOverlay->AddChildToOverlay(Status);
		StatusSlot->SetHorizontalAlignment(HAlign_Right);
		StatusSlot->SetVerticalAlignment(VAlign_Top);
		StatusSlot->SetPadding(FMargin(24.0f));

		RememberedRow.Init(0, Rows.Num());
	}
	const bool bOk = Super::Initialize();
	SelectTab(0);
	return bOk;
}

void USpikeHudScreen::SelectTab(int32 Index)
{
	if (Rows.Num() == 0)
	{
		return;
	}
	Index = FMath::Clamp(Index, 0, Rows.Num() - 1);
	if (USpikeButton* Focused = FocusedButton(); Focused && Rows.IsValidIndex(Tab))
	{
		const int32 R = Rows[Tab].IndexOfByKey(Focused);
		if (R != INDEX_NONE)
		{
			RememberedRow[Tab] = R;
		}
	}
	Tab = Index;
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(Tab);
	}
	for (int32 P = 0; P < Tabs.Num(); ++P)
	{
		Tabs[P]->SetNavigationRuleExplicit(EUINavigation::Down, Rows[Tab][RememberedRow[Tab]]);
	}
	if (IsActivated())
	{
		Rows[Tab][RememberedRow[Tab]]->SetFocus();
	}
}

UWidget* USpikeHudScreen::NativeGetDesiredFocusTarget() const
{
	return Rows.IsValidIndex(Tab) ? Rows[Tab][RememberedRow[Tab]].Get() : nullptr;
}

TOptional<FUIInputConfig> USpikeHudScreen::GetDesiredInputConfig() const
{
	// The dock is navigated while the game runs (docs/05 §2). Spike question: does a Game input config still let Slate move
	// focus between rows, or does the HUD need All? -S4HudMode=game|all.
	FString Mode = TEXT("all");
	FParse::Value(FCommandLine::Get(), TEXT("-S4HudMode="), Mode);
	return FUIInputConfig(Mode == TEXT("game") ? ECommonInputMode::Game : ECommonInputMode::All, EMouseCaptureMode::CapturePermanently, false);
}

FReply USpikeHudScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	const FKey K = KeyEvent.GetKey();
	if (K == EKeys::Gamepad_RightShoulder || K == EKeys::E)
	{
		SelectTab((Tab + 1) % Rows.Num());
		return FReply::Handled();
	}
	if (K == EKeys::Gamepad_LeftShoulder || K == EKeys::Q)
	{
		SelectTab((Tab + Rows.Num() - 1) % Rows.Num());
		return FReply::Handled();
	}
	if ((K == EKeys::Gamepad_FaceButton_Top || K == EKeys::T) && OnOpenTree)
	{
		OnOpenTree();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

void USpikeHudScreen::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	// The remembered row follows focus continuously. Spike S4 found that remembering it only when the tab changes is too late:
	// switching from keyboard to pad made Common UI re-apply NativeGetDesiredFocusTarget() (the stale row) before the RB handler
	// ran, so the page "remembered" row 0.
	if (USpikeButton* Focused = FocusedButton(); Focused && Rows.IsValidIndex(Tab))
	{
		const int32 R = Rows[Tab].IndexOfByKey(Focused);
		if (R != INDEX_NONE)
		{
			RememberedRow[Tab] = R;
		}
	}
}

void USpikeHudScreen::RefreshRows(int32 Tick)
{
	for (int32 R = 0; R < Rows[Tab].Num(); ++R)
	{
		Rows[Tab][R]->SetLabel(FString::Printf(TEXT("%s %02d   Lv %d   %d catnip"), SpikeS4::TabNames[Tab], R + 1, Tick, 100 + Tick * (R + 1)));
	}
	Status->SetText(FText::FromString(FString::Printf(TEXT("Catnip %d    Stardust 3"), 1200 + Tick)));
}

USpikeButton* USpikeHudScreen::FocusedButton() const
{
	for (USpikeButton* B : AllButtons)
	{
		if (SpikeS4::HasFocus(B))
		{
			return B;
		}
	}
	return nullptr;
}

// --- skill tree ------------------------------------------------------------------------------------------------------

int32 USpikeTreeLinks::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& Out,
	int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("1A1A1A")));
	for (const TPair<FVector2D, FVector2D>& L : Links)
	{
		TArray<FVector2f> Points = { FVector2f(L.Key), FVector2f(L.Value) };
		FSlateDrawElement::MakeLines(Out, LayerId, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Ink, true, 3.0f);
	}
	return Super::NativePaint(Args, Geometry, Culling, Out, LayerId + 1, Style, bParentEnabled);
}

bool USpikeTreeScreen::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = RootOverlay;
		// ONE canvas for the whole lattice, panned/zoomed by its render transform (docs/05 §6.2; nested canvases are not).
		Lattice = WidgetTree->ConstructWidget<UCanvasPanel>();
		UOverlaySlot* LatticeSlot = RootOverlay->AddChildToOverlay(Lattice);
		LatticeSlot->SetHorizontalAlignment(HAlign_Fill);
		LatticeSlot->SetVerticalAlignment(VAlign_Fill);
		Lattice->SetRenderTransformPivot(FVector2D(0.5, 0.5));

		// 87 nodes on a hex lattice: the 91 cells of radius 5 minus 4 corners (positions stand in for the ported layout).
		TArray<FIntPoint> Cells;
		for (int32 Q = -SpikeS4::HexRadius; Q <= SpikeS4::HexRadius; ++Q)
		{
			for (int32 R = -SpikeS4::HexRadius; R <= SpikeS4::HexRadius; ++R)
			{
				if (FMath::Abs(Q + R) <= SpikeS4::HexRadius)
				{
					Cells.Add(FIntPoint(Q, R));
				}
			}
		}
		Cells.RemoveAll([](const FIntPoint& C) { return (FMath::Abs(C.X) == 5 && FMath::Abs(C.Y) == 0) || (C.X == 0 && FMath::Abs(C.Y) == 5); });
		check(Cells.Num() == 87);
		for (const FIntPoint& C : Cells)
		{
			NodePos.Add(FVector2D(960.0 + 76.0 * (C.X + C.Y * 0.5), 540.0 + 66.0 * C.Y));
		}
		StartNode = Cells.IndexOfByKey(FIntPoint(0, 0));

		USpikeTreeLinks* Links = WidgetTree->ConstructWidget<USpikeTreeLinks>(USpikeTreeLinks::StaticClass());
		UCanvasPanelSlot* LinkSlot = Lattice->AddChildToCanvas(Links);
		LinkSlot->SetPosition(FVector2D::ZeroVector);
		LinkSlot->SetSize(FVector2D(1920.0, 1080.0));
		Links->SetVisibility(ESlateVisibility::HitTestInvisible);
		// Each node links to the neighbour one step closer to the centre (a tree, as the skill tree is).
		for (int32 I = 0; I < Cells.Num(); ++I)
		{
			const FIntPoint C = Cells[I];
			const int32 D = (FMath::Abs(C.X) + FMath::Abs(C.Y) + FMath::Abs(C.X + C.Y)) / 2;
			const FIntPoint Dirs[] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, -1}, {-1, 1} };
			for (const FIntPoint& Dir : Dirs)
			{
				const FIntPoint N = C + Dir;
				const int32 ND = (FMath::Abs(N.X) + FMath::Abs(N.Y) + FMath::Abs(N.X + N.Y)) / 2;
				const int32 J = Cells.IndexOfByKey(N);
				if (J != INDEX_NONE && ND < D)
				{
					Links->Links.Add({ NodePos[I], NodePos[J] });
					break;
				}
			}
		}

		for (int32 I = 0; I < Cells.Num(); ++I)
		{
			USpikeButton* Node = WidgetTree->ConstructWidget<USpikeButton>(USpikeButton::StaticClass());
			Node->Id = FString::Printf(TEXT("node:%d"), I);
			Node->SetLabel(FString::FromInt(I));
			UCanvasPanelSlot* NodeSlot = Lattice->AddChildToCanvas(Node);
			NodeSlot->SetPosition(NodePos[I] - FVector2D(SpikeS4::NodeSize * 0.5));
			NodeSlot->SetSize(FVector2D(SpikeS4::NodeSize));
			Nodes.Add(Node);
		}

		// Pad navigation from the lattice: the best neighbour per direction, written once (the Godot nav_pick idea).
		const FVector2D Dirs[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
		const EUINavigation NavDirs[4] = { EUINavigation::Up, EUINavigation::Down, EUINavigation::Left, EUINavigation::Right };
		Nav.SetNum(Nodes.Num());
		for (int32 I = 0; I < Nodes.Num(); ++I)
		{
			for (int32 D = 0; D < 4; ++D)
			{
				int32 Best = -1;
				double BestScore = TNumericLimits<double>::Max();
				for (int32 J = 0; J < Nodes.Num(); ++J)
				{
					const FVector2D Delta = NodePos[J] - NodePos[I];
					const double Along = FVector2D::DotProduct(Delta, Dirs[D]);
					const double Across = FMath::Abs(FVector2D::CrossProduct(Delta, Dirs[D]));
					if (J == I || Along <= 1.0 || Across > Along * 1.5)
					{
						continue;
					}
					const double Score = Along + 2.0 * Across;
					if (Score < BestScore)
					{
						BestScore = Score;
						Best = J;
					}
				}
				Nav[I][D] = Best;
				// -S4Sabotage (negative test): node 10 loses its Up rule while the graph the tour walks still has it.
				FString Sabotage;
				const bool bBreak = I == 10 && D == 0 && FParse::Value(FCommandLine::Get(), TEXT("-S4Sabotage="), Sabotage) && Sabotage == TEXT("nav");
				if (Best >= 0 && !bBreak) { Nodes[I]->SetNavigationRuleExplicit(NavDirs[D], Nodes[Best]); }
				else { Nodes[I]->SetNavigationRuleBase(NavDirs[D], EUINavigationRule::Stop); }
			}
		}
	}
	return Super::Initialize();
}

UWidget* USpikeTreeScreen::NativeGetDesiredFocusTarget() const
{
	// -S4Sabotage (negative test): no focus target, so focus falls out of the UI; the tour must report it.
	FString Sabotage;
	if (FParse::Value(FCommandLine::Get(), TEXT("-S4Sabotage="), Sabotage) && Sabotage == TEXT("focus"))
	{
		return nullptr;
	}
	return Nodes.IsValidIndex(StartNode) ? Nodes[StartNode].Get() : nullptr;
}

TOptional<FUIInputConfig> USpikeTreeScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

FReply USpikeTreeScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	const FKey K = KeyEvent.GetKey();
	if (SpikeS4::IsBack(K))
	{
		// The real game routes Back through Common UI's input data (phase 5); the spike has none yet.
		DeactivateWidget();
		return FReply::Handled();
	}
	if (K == EKeys::Gamepad_RightTrigger || K == EKeys::Equals)
	{
		ZoomLevel = FMath::Min(ZoomLevel * 1.25f, 2.5f);
		return FReply::Handled();
	}
	if (K == EKeys::Gamepad_LeftTrigger || K == EKeys::Hyphen)
	{
		ZoomLevel = FMath::Max(ZoomLevel / 1.25f, 0.5f);
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

void USpikeTreeScreen::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	// Auto-pan: keep the focused node inside the screen with a margin, so pad navigation never walks off-screen.
	const FVector2D Size = Geometry.GetLocalSize();
	const FVector2D Pivot = Size * 0.5;
	const int32 F = FocusedNode();
	if (F >= 0)
	{
		const FVector2D OnScreen = Pivot + (NodePos[F] - Pivot) * ZoomLevel + Pan;
		const double Margin = 120.0;
		FVector2D Fix = FVector2D::ZeroVector;
		if (OnScreen.X < Margin) { Fix.X = Margin - OnScreen.X; }
		if (OnScreen.X > Size.X - Margin) { Fix.X = Size.X - Margin - OnScreen.X; }
		if (OnScreen.Y < Margin) { Fix.Y = Margin - OnScreen.Y; }
		if (OnScreen.Y > Size.Y - Margin) { Fix.Y = Size.Y - Margin - OnScreen.Y; }
		Pan += Fix;
	}
	if (ZoomLevel <= 1.0f)
	{
		Pan = FVector2D::ZeroVector; // the whole lattice fits at 1x
	}
	Lattice->SetRenderTransform(FWidgetTransform(Pan, FVector2D(ZoomLevel), FVector2D::ZeroVector, 0.0f));
}

int32 USpikeTreeScreen::FocusedNode() const
{
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		if (SpikeS4::HasFocus(Nodes[I]))
		{
			return I;
		}
	}
	return -1;
}

bool USpikeTreeScreen::IsNodeOnScreen(int32 Node) const
{
	if (!Nodes.IsValidIndex(Node))
	{
		return false;
	}
	const FGeometry& Screen = GetCachedGeometry();
	const FGeometry& N = Nodes[Node]->GetCachedGeometry();
	const FVector2D S0 = Screen.GetAbsolutePosition(), S1 = S0 + Screen.GetAbsoluteSize();
	const FVector2D N0 = N.GetAbsolutePosition(), N1 = N0 + N.GetAbsoluteSize();
	return N0.X >= S0.X && N0.Y >= S0.Y && N1.X <= S1.X && N1.Y <= S1.Y;
}

// --- root layout -----------------------------------------------------------------------------------------------------

bool USpikeRootLayout::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = RootOverlay;
		GameStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>();
		MenuStack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>();
		for (UWidget* W : { static_cast<UWidget*>(GameStack), static_cast<UWidget*>(MenuStack) })
		{
			UOverlaySlot* S = RootOverlay->AddChildToOverlay(W);
			S->SetHorizontalAlignment(HAlign_Fill);
			S->SetVerticalAlignment(VAlign_Fill);
		}
	}
	return Super::Initialize();
}

// --- tour ------------------------------------------------------------------------------------------------------------

ASpikeUiActor::ASpikeUiActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void ASpikeUiActor::BeginPlay()
{
	Super::BeginPlay();
	FParse::Value(FCommandLine::Get(), TEXT("-S4Run="), RunName);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	Root = CreateWidget<USpikeRootLayout>(PC, USpikeRootLayout::StaticClass());
	Root->AddToViewport();
	Hud = Root->GameStack->AddWidget<USpikeHudScreen>(USpikeHudScreen::StaticClass());
	Hud->OnOpenTree = [this]()
	{
		if (Tree && Tree->IsActivated())
		{
			return;
		}
		const double T0 = FPlatformTime::Seconds();
		Tree = Root->MenuStack->AddWidget<USpikeTreeScreen>(USpikeTreeScreen::StaticClass());
		Log.Add(FString::Printf(TEXT("INFO opening the tree (87 nodes built and activated) took %.1f ms on the game thread"), (FPlatformTime::Seconds() - T0) * 1000.0));
		bTourBuiltForTree = false;
	};
	BuildTour();
	FString Mode = TEXT("all");
	FParse::Value(FCommandLine::Get(), TEXT("-S4HudMode="), Mode);
	Log.Add(FString::Printf(TEXT("run=%s hud_mode=%s"), *RunName, *Mode));
}

void ASpikeUiActor::BuildTour()
{
	auto Key = [this](const FKey& K, const FString& Expect, const FString& Note = FString())
	{
		Steps.Add({ FStep::Key, K, FString(), 3, Note });
		if (!Expect.IsEmpty())
		{
			Steps.Add({ FStep::Expect, FKey(), Expect, 0, Note });
		}
	};
	Steps.Add({ FStep::Expect, FKey(), TEXT("row:0:0"), 0, TEXT("HUD opens with focus on the remembered row") });
	for (int32 R = 1; R < 15; ++R) { Key(EKeys::Gamepad_DPad_Down, FString::Printf(TEXT("row:0:%d"), R), TEXT("D-pad down through the 15 shop rows")); }
	Key(EKeys::Gamepad_DPad_Down, TEXT("row:0:14"), TEXT("down at the last row stays (no wrap, focus kept)"));
	Steps.Add({ FStep::Shot, FKey(), TEXT("dock"), 2 });
	for (int32 R = 13; R >= 0; --R) { Key(EKeys::Gamepad_DPad_Up, FString::Printf(TEXT("row:0:%d"), R), TEXT("D-pad up")); }
	Key(EKeys::Gamepad_DPad_Up, TEXT("tab:0"), TEXT("up from the top row reaches its tab"));
	Key(EKeys::Gamepad_DPad_Right, TEXT("tab:1"), TEXT("tabs left/right"));
	Key(EKeys::Gamepad_DPad_Left, TEXT("tab:0"), TEXT("tabs left/right"));
	Key(EKeys::Gamepad_DPad_Down, TEXT("row:0:0"), TEXT("down from the tab returns to the page"));
	for (int32 P = 1; P < 6; ++P) { Key(EKeys::Gamepad_RightShoulder, FString::Printf(TEXT("row:%d:0"), P), TEXT("RB switches tab, focus on the page")); }
	Key(EKeys::Gamepad_RightShoulder, TEXT("row:0:0"), TEXT("RB wraps to the first tab"));
	for (int32 R = 1; R <= 4; ++R) { Key(EKeys::Down, FString::Printf(TEXT("row:0:%d"), R), TEXT("keyboard arrows (parity)")); }
	Key(EKeys::Gamepad_DPad_Up, TEXT("row:0:3"), TEXT("switching keyboard -> pad mid-list keeps the place"));
	Key(EKeys::Gamepad_RightShoulder, TEXT("row:1:0"), TEXT("RB to Reawaken"));
	Key(EKeys::Gamepad_LeftShoulder, TEXT("row:0:3"), TEXT("LB back: the page's remembered row"));
	Steps.Add({ FStep::Refresh, FKey(), FString(), 0, TEXT("15 Hz refresh of every row label for 2 s") });
	Steps.Add({ FStep::Expect, FKey(), TEXT("row:0:3"), 0, TEXT("focus survives the refresh") });
	Key(EKeys::Gamepad_FaceButton_Top, FString(), TEXT("Y opens the skill tree"));
	// The tree steps are inserted here when the tree appears.
	Key(EKeys::Gamepad_FaceButton_Right, TEXT("row:0:3"), TEXT("B closes the tree; focus back on the dock's remembered row"));
	Steps.Add({ FStep::Wait, FKey(), FString(), 10 });
}

FString ASpikeUiActor::FocusId() const
{
	if (Tree && Tree->IsActivated())
	{
		const int32 N = Tree->FocusedNode();
		if (N >= 0)
		{
			return FString::Printf(TEXT("node:%d"), N);
		}
	}
	if (Hud)
	{
		if (USpikeButton* B = Hud->FocusedButton())
		{
			return B->Id;
		}
	}
	TSharedPtr<SWidget> Raw = FSlateApplication::Get().GetUserFocusedWidget(0);
	return Raw ? FString::Printf(TEXT("other(%s)"), *Raw->GetTypeAsString()) : TEXT("none");
}

void ASpikeUiActor::Check(bool bOk, const FString& What)
{
	bOk ? ++Passes : ++Fails;
	Log.Add(FString::Printf(TEXT("%s %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), *What));
}

void ASpikeUiActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bReported)
	{
		if (--QuitCountdown == 0)
		{
			GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));
		}
		return;
	}
	const FString Focus = FocusId();
	if (StepIndex > 0 && (Focus.StartsWith(TEXT("other")) || Focus == TEXT("none")))
	{
		++FocusLostFrames;
	}

	// The tree just appeared: walk every node reachable from where it opens (shortest key path to each in BFS order).
	if (Tree && !bTourBuiltForTree && Tree->Nodes.Num() > 0)
	{
		bTourBuiltForTree = true;
		TArray<FStep> T;
		T.Add({ FStep::Wait, FKey(), FString(), 5 });
		T.Add({ FStep::Expect, FKey(), FString::Printf(TEXT("node:%d"), Tree->StartNode), 0, TEXT("tree opens with focus on its start node") });
		T.Add({ FStep::Key, EKeys::Gamepad_RightTrigger, FString(), 3 });
		T.Add({ FStep::Key, EKeys::Gamepad_RightTrigger, FString(), 3 });
		T.Add({ FStep::Key, EKeys::Gamepad_RightTrigger, FString(), 3 });
		T.Add({ FStep::CheckZoom, FKey(), FString(), 0, TEXT("RT zooms in") });
		const FKey DirKeys[4] = { EKeys::Gamepad_DPad_Up, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_DPad_Right };
		const int32 N = Tree->Nodes.Num();
		TArray<int32> Order;
		TSet<int32> Seen;
		Order.Add(Tree->StartNode);
		Seen.Add(Tree->StartNode);
		for (int32 I = 0; I < Order.Num(); ++I)
		{
			for (int32 D = 0; D < 4; ++D)
			{
				const int32 J = Tree->Nav[Order[I]][D];
				if (J >= 0 && !Seen.Contains(J)) { Seen.Add(J); Order.Add(J); }
			}
		}
		Log.Add(FString::Printf(TEXT("INFO tree: %d nodes, %d reachable by the navigation graph from node %d"), N, Order.Num(), Tree->StartNode));
		// Shortest key path over the directed navigation graph, appended as key + expected focus steps.
		auto GoTo = [&](int32 From, int32 To)
		{
			TArray<int32> Prev, PrevDir;
			Prev.Init(-1, N);
			PrevDir.Init(-1, N);
			TArray<int32> Q = { From };
			Prev[From] = From;
			for (int32 H = 0; H < Q.Num() && Prev[To] < 0; ++H)
			{
				for (int32 D = 0; D < 4; ++D)
				{
					const int32 J = Tree->Nav[Q[H]][D];
					if (J >= 0 && Prev[J] < 0) { Prev[J] = Q[H]; PrevDir[J] = D; Q.Add(J); }
				}
			}
			TArray<int32> Path;
			for (int32 X = To; X != From && Prev[X] >= 0; X = Prev[X]) { Path.Insert(X, 0); }
			for (int32 X : Path)
			{
				T.Add({ FStep::Key, DirKeys[PrevDir[X]], FString(), 2 });
				T.Add({ FStep::Expect, FKey(), FString::Printf(TEXT("node:%d"), X), 0, TEXT("tree walk") });
			}
		};
		// Every rule, not just reachability (the shortest-path walk alone missed a deliberately removed rule): at each node press
		// all four directions, expect the planned neighbour (or to stay where the rule is Stop), and come back.
		int32 Cur = Tree->StartNode;
		for (int32 Node : Order)
		{
			GoTo(Cur, Node);
			Cur = Node;
			for (int32 D = 0; D < 4; ++D)
			{
				const int32 J = Tree->Nav[Node][D];
				T.Add({ FStep::Key, DirKeys[D], FString(), 2 });
				T.Add({ FStep::Expect, FKey(), FString::Printf(TEXT("node:%d"), J >= 0 ? J : Node), 0, TEXT("tree walk") });
				if (J >= 0)
				{
					GoTo(J, Node);
				}
			}
		}
		T.Add({ FStep::Shot, FKey(), TEXT("tree"), 2 });
		Steps.Insert(T, StepIndex);
	}

	if (RefreshFramesLeft > 0)
	{
		if (RefreshFramesLeft % 4 == 0) // 15 Hz at 60 fps (docs/05 §6.1)
		{
			Hud->RefreshRows(RefreshFramesLeft);
		}
		RefreshFrameMs.Add(DeltaSeconds * 1000.0);
		--RefreshFramesLeft;
		return;
	}
	NavFrameMs.Add(DeltaSeconds * 1000.0);

	if (WaitFrames > 0)
	{
		--WaitFrames;
		return;
	}
	if (!Steps.IsValidIndex(StepIndex))
	{
		Report();
		return;
	}
	const FStep S = Steps[StepIndex++];
	switch (S.Kind)
	{
	case FStep::Key:
	{
		FSlateApplication& App = FSlateApplication::Get();
		App.ProcessKeyDownEvent(FKeyEvent(S.KeyToPress, FModifierKeysState(), 0, false, 0, 0));
		App.ProcessKeyUpEvent(FKeyEvent(S.KeyToPress, FModifierKeysState(), 0, false, 0, 0));
		WaitFrames = S.Frames;
		break;
	}
	case FStep::Expect:
	{
		const FString Now = FocusId();
		const bool bTreeWalk = S.Note == TEXT("tree walk");
		if (!bTreeWalk || Now != S.Expected)
		{
			Check(Now == S.Expected, FString::Printf(TEXT("%s: expected %s, focus on %s"), *S.Note, *S.Expected, *Now));
		}
		else
		{
			++Passes; // the walk logs failures only (one line per node would bury the rest)
		}
		if (bTreeWalk && Tree)
		{
			const int32 Node = FCString::Atoi(*S.Expected.Mid(5));
			if (!Tree->IsNodeOnScreen(Node))
			{
				Check(false, FString::Printf(TEXT("auto-pan: node %d focused but off screen at zoom %.2f"), Node, Tree->Zoom()));
			}
		}
		break;
	}
	case FStep::CheckZoom:
		Check(Tree && Tree->Zoom() > 1.5f, FString::Printf(TEXT("%s: zoom %.2f"), *S.Note, Tree ? Tree->Zoom() : 0.0f));
		break;
	case FStep::Refresh:
		RefreshFramesLeft = 120;
		break;
	case FStep::Shot:
		if (FParse::Param(FCommandLine::Get(), TEXT("S4Shot")))
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Spikes/S4") / RunName + TEXT("_") + S.Expected + TEXT(".png"), true, false);
		}
		WaitFrames = S.Frames;
		break;
	case FStep::Wait:
		WaitFrames = S.Frames;
		break;
	}
}

void ASpikeUiActor::Report()
{
	bReported = true;
	QuitCountdown = 10;
	auto Stats = [](TArray<double> V)
	{
		if (V.Num() == 0) { return FString(TEXT("n/a")); }
		double Sum = 0.0;
		for (double X : V) { Sum += X; }
		V.Sort();
		return FString::Printf(TEXT("avg %.3f p99 %.3f max %.3f ms (%d frames)"), Sum / V.Num(), V[static_cast<int32>(0.99 * (V.Num() - 1))], V.Last(), V.Num());
	};
	Log.Insert(FString::Printf(TEXT("RESULT %s: %d pass, %d fail, focus lost on %d frames"), Fails == 0 && FocusLostFrames == 0 ? TEXT("PASS") : TEXT("FAIL"),
		Passes, Fails, FocusLostFrames), 0);
	Log.Add(TEXT("frames during refresh: ") + Stats(RefreshFrameMs));
	Log.Add(TEXT("frames otherwise:      ") + Stats(NavFrameMs));
	FFileHelper::SaveStringArrayToFile(Log, *(FPaths::ProjectSavedDir() / TEXT("Spikes/S4") / RunName + TEXT(".txt")));
	for (const FString& L : Log)
	{
		UE_LOG(LogPaw, Display, TEXT("S4 %s"), *L);
	}
}
