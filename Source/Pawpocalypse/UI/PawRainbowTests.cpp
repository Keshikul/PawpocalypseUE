// Pawpocalypse.UI.Rainbow (phase 6 slice 5d): the rainbow pieces and the tree's sky where Godot puts them. Their shapes are
// PawCore's (L1 `Rainbow`, `Skill tree sky`: exact against the real scripts, fixture `rainbow`); these prove the game paints
// them where and when Godot does.
//
// Pawpocalypse.UI.Rainbow.Bar    the Reawaken page's bar is rainbow_bar.gd: the theme's track with no fill of its own, the
//                                value Godot's Range keeps, sparks from the leading edge, 48 bands painted (none when empty).
//                                Every other HUD bar fills Godot's way (caps kept: Scale) at the value its Range keeps.
// Pawpocalypse.UI.Rainbow.Frame  the zone plate's frame follows the plate (margin 2, corner 8, width 4), shows and turns only
//                                while the plate shouts and is shown, paints its 37-point outline round the plate.
// Pawpocalypse.UI.Rainbow.Sky    the tree's sky is skill_tree_sky.gd: laid out as PcSky for its size and the effects budget,
//                                stepped while up, every star and cloud painted over the Sky border (which still takes input).
//
// Negative tests: tools/sabotage_realm.py (`rainbow_*`).

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/HittestGrid.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "PcBackdrop.h"
#include "PcHud.h"
#include "PcRainbow.h"
#include "Realm/PawRealmSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawHudScreen.h"
#include "UI/PawRainbow.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawTreeSky.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "Widgets/SWindow.h"

namespace
{
	struct FRainbowWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawRootLayout* Root = nullptr;
		UPawHudScreen* Hud = nullptr;
		TSharedPtr<SWidget> RootSlate;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			if (!T.TestNotNull(TEXT("the realm"), Realm) || !T.TestNotNull(TEXT("the game"), Game) || !T.TestTrue(TEXT("data"), Game->IsDataReady()))
			{
				return false;
			}
			Game->NewGame();
			Realm->SetViewSize(1920.0, 1080.0);
			if (!T.TestTrue(TEXT("the realm starts"), Realm->StartRealm(77)))
			{
				return false;
			}
			Root = CreateWidget<UPawRootLayout>(GI, PawUi::Resolve<UPawRootLayout>());
			Hud = CreateWidget<UPawHudScreen>(GI, PawUi::Resolve<UPawHudScreen>());
			if (!T.TestNotNull(TEXT("root"), Root) || !T.TestNotNull(TEXT("HUD"), Hud))
			{
				return false;
			}
			RootSlate = Root->TakeWidget();
			Hud->SetMenuStack(Root->MenuStack);
			Hud->RefreshNow();
			return true;
		}

		void Close()
		{
			RootSlate.Reset();
			if (GI)
			{
				GI->Shutdown();
			}
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		Pc::PcProgress& P() { return Game->Progress(); }
		const Pc::PcGameData& D() { return Game->Data(); }
		UPawSettingsSubsystem* Settings() { return GI->GetSubsystem<UPawSettingsSubsystem>(); }
	};

	// Paints a widget off screen at Size (KB U/09 §3: the Slate root held for the whole paint); returns the triangle count of
	// every custom-vertex batch drawn (each piece paints its Godot draw calls as one batch).
	TArray<int32> PaintOffScreen(UWidget* Widget, const FVector2f Size)
	{
		const TSharedRef<SWidget> S = Widget->TakeWidget();
		S->SlatePrepass(1.0f);
		const FGeometry Root = FGeometry::MakeRoot(FVector2D(Size), FSlateLayoutTransform());
		const TSharedRef<SWindow> Window = SNew(SWindow);
		FSlateWindowElementList List(Window);
		FHittestGrid Grid;
		FPaintArgs Args(&Window.Get(), Grid, FVector2D::ZeroVector, FApp::GetCurrentTime(), 0.016f);
		S->Paint(Args, Root, FSlateRect(0.0f, 0.0f, Size.X, Size.Y), List, 0, FWidgetStyle(), true);
		TArray<int32> Batches;
		for (const FSlateCustomVertsElement& E : List.GetUncachedDrawElements().Get<(uint8)EElementType::ET_CustomVerts>())
		{
			Batches.Add(E.Indices.Num() / 3);
		}
		return Batches;
	}

	// Triangles per Godot call as FPawFxCanvas makes them: a rect 2, a circle 64, a cloud's fan 26, a frame's 36 segments x
	// (the stroke 2 + its feather 2 x 2).
	constexpr int32 RectTris = 2, CircleTris = 64, CloudTris = 26, FrameTris = 36 * 6;

	template <typename T>
	T* Part(UUserWidget* W, const TCHAR* Name)
	{
		return W && W->WidgetTree ? Cast<T>(W->WidgetTree->FindWidget(FName(Name))) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRainbowBarTest, "Pawpocalypse.UI.Rainbow.Bar", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRainbowBarTest::RunTest(const FString& Parameters)
{
	FRainbowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawRainbowBar* Bar = W.Hud->GetReawakenRainbow();
	if (!TestNotNull(TEXT("the Reawaken page's bar is rainbow_bar.gd"), Bar))
	{
		W.Close();
		return false;
	}
	// The theme's track, and no fill of its own (rainbow_bar.gd: the fill overridden with a StyleBoxEmpty).
	if (const UProgressBar* Track = Part<UProgressBar>(Bar, TEXT("Track")); TestNotNull(TEXT("its track"), Track))
	{
		TestTrue(TEXT("the track's own fill draws nothing"), Track->GetWidgetStyle().FillImage.DrawAs == ESlateBrushDrawType::NoDrawType);
		TestEqual(TEXT("the track never fills"), Track->GetPercent(), 0.0f);
	}

	// The page sets the value; the Range keeps it in hundredths.
	W.P().LifetimeCatnipEver = 0.0;
	W.Hud->SelectTab(Pc::PcDockTab::Reawaken);
	W.Hud->RefreshNow();
	const Pc::PcReawakenPage Empty = Pc::ReawakenPage(W.D(), W.P(), false);
	TestEqual(TEXT("an empty bar"), Bar->GetValue(), Pc::GodotRangeValue(Empty.Fraction, 0.0, 1.0, Pc::GodotRangeStep));
	const FVector2D Size(300.0, 30.0);
	Bar->StepFor(1.0, Size);
	TestEqual(TEXT("an empty bar throws no sparks"), static_cast<int32>(Bar->GetState().Sparks.size()), 0);
	PaintOffScreen(Bar, FVector2f(Size));
	TestEqual(TEXT("an empty bar paints no band"), Bar->GetPaintedBands(), 0);

	// Part way: the page hands the bar the fraction, the Range keeps hundredths.
	{
		const Pc::PcReawakenPage Before = Pc::ReawakenPage(W.D(), W.P(), false);
		W.P().LifetimeCatnipThisCycle = Before.Required * 0.41237;
		W.P().LifetimeCatnipEver = W.P().LifetimeCatnipThisCycle;
		W.Hud->RefreshNow();
		const Pc::PcReawakenPage Mid = Pc::ReawakenPage(W.D(), W.P(), false);
		TestTrue(TEXT("a bar part way"), Mid.Fraction > 0.0 && Mid.Fraction < 1.0);
		TestEqual(TEXT("the page's fraction, kept in hundredths"), Bar->GetValue(), Pc::GodotRangeValue(Mid.Fraction, 0.0, 1.0, Pc::GodotRangeStep));
	}

	Bar->SetValue(0.6295);
	TestEqual(TEXT("0.6295 is kept as 0.63"), Bar->GetValue(), Pc::GodotRangeValue(0.6295, 0.0, 1.0, 0.01));
	TestTrue(TEXT("(snapped)"), FMath::IsNearlyEqual(Bar->GetValue(), 0.63, 1e-12));
	Bar->StepFor(0.5, Size); // 14 a second: 7
	const int32 Sparks = static_cast<int32>(Bar->GetState().Sparks.size());
	TestEqual(TEXT("sparks from the leading edge, 14 a second"), Sparks, 7);
	bool bAtEdge = Sparks > 0;
	for (const Pc::PcRainbowSpark& S : Bar->GetState().Sparks)
	{
		bAtEdge &= FMath::Abs(S.X - Size.X * Bar->GetFraction()) <= Pc::RainbowBar::SparkJitterPx && S.Y >= 0.0 && S.Y <= Size.Y && S.Vy < 0.0;
	}
	TestTrue(TEXT("each born at the edge, rising"), bAtEdge);
	const TArray<int32> Batches = PaintOffScreen(Bar, FVector2f(Size));
	TestEqual(TEXT("48 bands painted"), Bar->GetPaintedBands(), Pc::RainbowBar::BandCount);
	TestEqual(TEXT("every spark painted"), Bar->GetPaintedSparks(), Sparks);
	TestTrue(TEXT("one batch of 48 bands and the sparks reached Slate"),
		Batches.Contains(Pc::RainbowBar::BandCount * RectTris + Sparks * CircleTris));
	Bar->StepFor(Pc::RainbowBar::SparkLifetime, Size);
	bool bYoung = true;
	for (const Pc::PcRainbowSpark& S : Bar->GetState().Sparks)
	{
		bYoung &= S.Age < Pc::RainbowBar::SparkLifetime;
	}
	TestTrue(TEXT("a spark lives under 0.7 s"), bYoung);

	// Every other HUD bar: Godot's fill (both caps kept: Scale), in the asset and in the code layout.
	PawUi::SetCodeLayoutForced(true);
	UPawHudScreen* CodeHud = CreateWidget<UPawHudScreen>(W.GI, UPawHudScreen::StaticClass());
	PawUi::SetCodeLayoutForced(false);
	for (UPawHudScreen* H : { W.Hud, CodeHud })
	{
		const TCHAR* Mode = H == W.Hud ? TEXT("asset") : TEXT("code");
		for (const TCHAR* Name : { TEXT("WaveBar"), TEXT("ZoneBar"), TEXT("XpBar"), TEXT("BossBar") })
		{
			const UProgressBar* B = Part<UProgressBar>(H, Name);
			if (TestNotNull(FString::Printf(TEXT("%s (%s)"), Name, Mode), B))
			{
				TestTrue(FString::Printf(TEXT("%s keeps its fill's caps: Scale (%s)"), Name, Mode), B->GetBarFillStyle() == EProgressBarFillStyle::Scale);
			}
		}
	}
	TestTrue(TEXT("the XP bar keeps 1/10000ths"), FMath::IsNearlyEqual(PawUi::SetBarValue(nullptr, 0.123456, PawUi::XpBarStep, 0.0), 0.1235, 1e-12));
	TestTrue(TEXT("the others hundredths"), FMath::IsNearlyEqual(PawUi::SetBarValue(nullptr, 0.123456, Pc::GodotRangeStep, 0.0), 0.12, 1e-12));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRainbowFrameTest, "Pawpocalypse.UI.Rainbow.Frame", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRainbowFrameTest::RunTest(const FString& Parameters)
{
	FRainbowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawRainbowFrame* Frame = W.Hud->GetZoneReady();
	if (!TestNotNull(TEXT("the zone plate's frame"), Frame))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("it follows the zone plate"), Frame->GetFollow() == W.Hud->GetZonePlate());
	TestEqual(TEXT("margin 2"), Frame->Margin, Pc::RainbowFrame::ZoneMargin);
	TestEqual(TEXT("corner 8"), Frame->Corner, Pc::RainbowFrame::ZoneCorner);
	TestEqual(TEXT("width 4"), Frame->LineWidth, Pc::RainbowFrame::ZoneWidth);
	TestTrue(TEXT("it takes no input"), Frame->GetVisibility() == ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("a sibling under the HUD's root, not in the plate's column"), Frame->GetParent() == W.Hud->WidgetTree->RootWidget);
	// The code layout sets the same (the asset holds its own copy of these settings, so check both).
	PawUi::SetCodeLayoutForced(true);
	UPawHudScreen* CodeHud = CreateWidget<UPawHudScreen>(W.GI, UPawHudScreen::StaticClass());
	PawUi::SetCodeLayoutForced(false);
	const UPawRainbowFrame* CodeFrame = CodeHud ? CodeHud->GetZoneReady() : nullptr;
	if (TestNotNull(TEXT("the code layout's frame"), CodeFrame))
	{
		TestTrue(TEXT("code layout: margin 2, corner 8, width 4"), CodeFrame->Margin == Pc::RainbowFrame::ZoneMargin
			&& CodeFrame->Corner == Pc::RainbowFrame::ZoneCorner && CodeFrame->LineWidth == Pc::RainbowFrame::ZoneWidth);
	}

	// Nothing to shout about: no frame, no turning.
	W.Hud->RefreshNow();
	const Pc::PcZonePlate Quiet = Pc::ZonePlate(W.Realm->GetSim().Field, W.D(), W.P());
	TestFalse(TEXT("the plate does not shout yet"), Quiet.bRainbow);
	TestFalse(TEXT("so the frame is off"), Frame->IsActive());
	Frame->StepFor(1.0);
	TestEqual(TEXT("and does not turn"), Frame->GetPhase(), 0.0);
	const TArray<int32> QuietBatches = PaintOffScreen(W.Hud, FVector2f(1920.0f, 1080.0f));
	TestEqual(TEXT("and paints nothing"), Frame->GetPaintedPoints(), 0);
	TestFalse(TEXT("(no outline in Slate)"), QuietBatches.Contains(FrameTris));

	// A biome to go to: the plate shouts, the frame shows, turns 0.12 a second and paints round the plate.
	W.P().LifetimeCatnipThisCycle += W.D().Tiers[1].CatnipToUnlock + 1.0;
	W.Hud->RefreshNow();
	TestTrue(TEXT("the plate shouts"), Pc::ZonePlate(W.Realm->GetSim().Field, W.D(), W.P()).bRainbow);
	TestTrue(TEXT("the frame is on"), Frame->IsActive());
	TestTrue(TEXT("and shown with the plate"), Frame->IsShown());
	Frame->StepFor(1.0);
	TestTrue(TEXT("it turned 0.12"), FMath::IsNearlyEqual(Frame->GetPhase(), Pc::RainbowFrame::TurnsPerSec, 1e-12));
	const TArray<int32> LoudBatches = PaintOffScreen(W.Hud, FVector2f(1920.0f, 1080.0f));
	TestEqual(TEXT("the outline painted: 4 corners x 9 points, closed"), Frame->GetPaintedPoints(), 37);
	TestTrue(TEXT("its 36 segments, stroked and feathered, reached Slate"), LoudBatches.Contains(FrameTris));

	// The plate hidden: the frame hides and stops turning (rainbow_frame.gd: visible = follow.is_visible_in_tree() and active).
	UWidget* Plate = W.Hud->GetZonePlate();
	Plate->SetVisibility(ESlateVisibility::Collapsed);
	TestFalse(TEXT("hidden with its plate"), Frame->IsShown());
	const double Was = Frame->GetPhase();
	Frame->StepFor(1.0);
	TestEqual(TEXT("not turning while hidden"), Frame->GetPhase(), Was);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRainbowSkyTest, "Pawpocalypse.UI.Rainbow.Sky", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRainbowSkyTest::RunTest(const FString& Parameters)
{
	FRainbowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("the tree opens"), W.Hud->OpenSkillTree(false));
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	UPawTreeSky* Sky = Tree ? Tree->GetSky() : nullptr;
	if (!TestNotNull(TEXT("the tree's sky"), Sky))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("the sky takes no input"), Sky->GetVisibility() == ESlateVisibility::HitTestInvisible);
	if (const UBorder* Under = Part<UBorder>(Tree, TEXT("Sky")); TestNotNull(TEXT("the Sky border under it"), Under))
	{
		TestTrue(TEXT("which still takes the drag and the wheel"), Under->GetVisibility() == ESlateVisibility::Visible);
	}

	for (const bool bEffects : { true, false })
	{
		W.Settings()->SetEffectsEnabled(bEffects);
		const FVector2D Size = bEffects ? FVector2D(1920.0, 1080.0) : FVector2D(1280.0, 720.0); // a new size lays it out again
		const double Clock = Sky->GetSky().Time;
		Sky->LayOutFor(Size);
		TestEqual(TEXT("the clock runs on through a relayout (_time is not reset)"), Sky->GetSky().Time, Clock);
		const Pc::PcSky Want = Pc::BuildSky(Size.X, Size.Y, W.Settings()->EffectBudget());
		const Pc::PcSky& Got = Sky->GetSky();
		const FString At = bEffects ? TEXT("effects on") : TEXT("effects off");
		TestEqual(At + TEXT(": clouds"), static_cast<int32>(Got.Clouds.size()), static_cast<int32>(Want.Clouds.size()));
		TestEqual(At + TEXT(": stars"), static_cast<int32>(Got.Stars.size()), static_cast<int32>(Want.Stars.size()));
		TestTrue(At + TEXT(": at least the low budget's (3 clouds, 21 stars)"), Got.Clouds.size() >= 3 && Got.Stars.size() >= 21);
		TestTrue(At + TEXT(": the same sky (the first cloud where PcSky puts it)"),
			!Got.Clouds.empty() && Got.Clouds[0].AtX == Want.Clouds[0].AtX && Got.Clouds[0].AtY == Want.Clouds[0].AtY);
		const double Before = Got.Time;
		const float X0 = Got.Clouds.empty() ? 0.0f : Got.Clouds[0].AtX;
		Sky->StepFor(10.0);
		TestTrue(At + TEXT(": the clock runs"), FMath::IsNearlyEqual(Sky->GetSky().Time, Before + 10.0, 1e-9));
		TestTrue(At + TEXT(": and has run before"), bEffects || Before > 0.0);
		TestTrue(At + TEXT(": the clouds drift"), !Got.Clouds.empty() && Got.Clouds[0].AtX != X0);
		const TArray<int32> Batches = PaintOffScreen(Tree, FVector2f(Size));
		TestEqual(At + TEXT(": every star painted"), Sky->GetPaintedStars(), static_cast<int32>(Want.Stars.size()));
		TestEqual(At + TEXT(": every cloud painted"), Sky->GetPaintedClouds(), static_cast<int32>(Want.Clouds.size()));
		TestTrue(At + TEXT(": the base, the stars and the clouds reached Slate as one batch"),
			Batches.Contains(RectTris + CircleTris * static_cast<int32>(Want.Stars.size()) + CloudTris * static_cast<int32>(Want.Clouds.size())));
	}
	W.Settings()->SetEffectsEnabled(true);
	W.Close();
	return true;
}

#endif
