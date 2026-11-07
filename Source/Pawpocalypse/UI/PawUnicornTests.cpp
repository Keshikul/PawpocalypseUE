// Pawpocalypse.UI.Unicorn (phase 6 slice 6a): the Unicorn ceremony where Godot plays it. Its frames are PcUnicornCeremony's
// (L1 `Unicorn`: exact against the real unicorn_ceremony.gd, fixture `unicorn`); this proves the tree plays it when, and only
// when, the Unicorn is bought; that it takes the clicks and the keys while it plays (the tree buys nothing, Back does not
// close the tree), skips on a press after 0.6 s and never before, whinnies twice and fanfares once, paints the cat, the
// rainbow and the fiesta, and hands the tree back when it ends.
//
// Negative tests: tools/sabotage_realm.py (`unicorn_*`).

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Audio/PawAudioSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Input/HittestGrid.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "PcTreeLayout.h"
#include "PcUnicorn.h"
#include "Realm/PawRealmSubsystem.h"
#include "Rendering/DrawElements.h"
#include "UI/PawHudScreen.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawUnicornCeremony.h"
#include "UI/PawWidgets.h"
#include "Widgets/SWindow.h"

namespace
{
	struct FUnicornWorld
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
			// The sound's rate limits read this clock: it moves with the ceremony, as real time would.
			if (UPawAudioSubsystem* A = Audio())
			{
				A->Now = [this]() { return Clock; };
			}
			return true;
		}

		void Step(UPawUnicornCeremony* C, double Dt)
		{
			Clock += Dt;
			C->StepFor(Dt);
		}

		double Clock = 100.0;

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
		UPawAudioSubsystem* Audio() { return GI->GetSubsystem<UPawAudioSubsystem>(); }
	};

	int32 CountPlayed(UPawAudioSubsystem* A, const TCHAR* Event)
	{
		int32 N = 0;
		for (const UPawAudioSubsystem::FPlayed& P : A ? A->Played() : TArray<UPawAudioSubsystem::FPlayed>())
		{
			N += P.Event == Event;
		}
		return N;
	}

	// Paints off screen; returns the triangle count of every custom-vertex batch drawn.
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawUnicornPlaysTest, "Pawpocalypse.UI.Unicorn.Plays", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawUnicornPlaysTest::RunTest(const FString& Parameters)
{
	FUnicornWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Stardust = 1e12;
	TestTrue(TEXT("the tree opens to spend"), W.Hud->OpenSkillTree(true));
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	UPawUnicornCeremony* C = Tree ? Tree->GetUnicorn() : nullptr;
	if (!TestNotNull(TEXT("the tree's Unicorn ceremony"), C))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("it has the mounted cat's art"), C->HasArt());
	TestFalse(TEXT("nothing plays on open"), C->IsPlaying());
	TestTrue(TEXT("hidden until it plays"), C->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("the last part of the tree: over all of it"),
		C->GetParent() && C->GetParent()->GetChildIndex(C) == C->GetParent()->GetChildrenCount() - 1);

	// Another node bought: no ceremony.
	const FString Other = UTF8_TO_TCHAR(Pc::CheapestAffordableBranch(W.Game->Data(), W.P()).c_str());
	TestTrue(TEXT("a node that is not the Unicorn"), !Other.IsEmpty() && Other != TEXT("unlock_unicorn"));
	TestTrue(TEXT("it buys"), Tree->PressNode(Other));
	TestFalse(TEXT("and plays nothing"), C->IsPlaying());

	// The Unicorn: the ceremony.
	if (UPawAudioSubsystem* A = W.Audio())
	{
		A->ClearLog();
	}
	TestTrue(TEXT("the Unicorn buys"), Tree->PressNode(TEXT("unlock_unicorn")));
	TestTrue(TEXT("and the ceremony plays"), C->IsPlaying() && Tree->IsCeremonyPlaying());
	TestTrue(TEXT("it takes every click"), C->GetVisibility() == ESlateVisibility::Visible);
	TestEqual(TEXT("the first whinny"), CountPlayed(W.Audio(), TEXT("unicorn_neigh")), 1);

	// While it plays: nothing is bought, a press before 0.6 s does not skip.
	auto Levels = [&W]()
	{
		int64 N = 0;
		for (const auto& L : W.P().BranchLevels)
		{
			N += L.second;
		}
		return N;
	};
	const int64 Before = Levels();
	const std::string Next = Pc::CheapestAffordableBranch(W.Game->Data(), W.P());
	TestFalse(TEXT("the tree buys nothing mid-fiesta"), !Next.empty() && Tree->PressNode(UTF8_TO_TCHAR(Next.c_str())));
	TestEqual(TEXT("(no level moved)"), Levels(), Before);
	W.Step(C, 0.3);
	TestFalse(TEXT("a key before 0.6 s does not skip"), C->Input(Pc::PcUnicornInput::KeyDown));
	W.Step(C, 0.4);
	TestFalse(TEXT("an echo does not skip"), C->Input(Pc::PcUnicornInput::KeyEcho));
	TestFalse(TEXT("a key up does not skip"), C->Input(Pc::PcUnicornInput::KeyUp));

	// The rest of the show: the second whinny, the fanfare and the fiesta, the rainbow; painted.
	W.Step(C, 1.0);
	TestEqual(TEXT("the second whinny"), CountPlayed(W.Audio(), TEXT("unicorn_neigh")), 2);
	TestEqual(TEXT("the fanfare"), CountPlayed(W.Audio(), TEXT("boss_defeated")), 1);
	TestTrue(TEXT("the fiesta: 206 bits"), C->GetState().Bits.size() == 206);
	TestTrue(TEXT("the rainbow grows"), C->GetState().Grow > 0.0);
	const TArray<int32> Batches = PaintOffScreen(Tree, FVector2f(1920.0f, 1080.0f));
	TestTrue(TEXT("the cat painted"), C->WasCatPainted());
	// The dim 2; six arcs of 63 segments, stroked and feathered (6 each); 180 confetti quads (2); 26 stars fanned (10); their
	// rims, 10 segments (6 each).
	TestTrue(TEXT("the dim, the rainbow and the fiesta reached Slate as one batch"),
		Batches.Contains(2 + 6 * 63 * 6 + 180 * 2 + 26 * 10 + 26 * 10 * 6));
	TestEqual(TEXT("six arcs, 180 confetti, 26 stars and their rims"), C->GetPaintedShapes(), 6 + 180 + 26 * 2);

	// A press after 0.6 s skips to the fade; the tree comes back when it ends.
	TestTrue(TEXT("a mouse press skips"), C->Input(Pc::PcUnicornInput::MouseDown));
	TestTrue(TEXT("fading"), C->GetState().bDone && C->IsPlaying());
	W.Step(C, 0.2);
	W.Step(C, 0.2);
	TestFalse(TEXT("finished"), C->IsPlaying());
	TestTrue(TEXT("collapsed again"), C->GetVisibility() == ESlateVisibility::Collapsed);
	TestFalse(TEXT("the tree is the tree again"), Tree->IsCeremonyPlaying());
	TestTrue(TEXT("and buys again"), !Pc::CheapestAffordableBranch(W.Game->Data(), W.P()).empty()
		&& Tree->PressNode(UTF8_TO_TCHAR(Pc::CheapestAffordableBranch(W.Game->Data(), W.P()).c_str())));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawUnicornRunsOutTest, "Pawpocalypse.UI.Unicorn.RunsOut", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawUnicornRunsOutTest::RunTest(const FString& Parameters)
{
	FUnicornWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Stardust = 1e12;
	TestTrue(TEXT("the tree opens to spend"), W.Hud->OpenSkillTree(true));
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	UPawUnicornCeremony* C = Tree ? Tree->GetUnicorn() : nullptr;
	if (!TestNotNull(TEXT("the ceremony"), C) || !TestTrue(TEXT("the Unicorn buys"), Tree->PressNode(TEXT("unlock_unicorn"))))
	{
		W.Close();
		return false;
	}
	// Left alone: it fades at 3.4 s and is gone 0.35 s later.
	double T = 0.0;
	while (C->IsPlaying() && T < 10.0)
	{
		W.Step(C, 1.0 / 60.0);
		T += 1.0 / 60.0;
	}
	TestTrue(TEXT("it ends on its own at 3.4 + 0.35 s"), T > 3.7 && T < 3.8);
	TestEqual(TEXT("two whinnies"), CountPlayed(W.Audio(), TEXT("unicorn_neigh")), 2);
	W.Close();
	return true;
}

#endif
