// The skill tree's engine half (docs/09 L3; docs/05 §6.2). PcTreeLayout / PcTreeText are pinned in PawCore against the real
// Godot code; these prove the screen built on them, on the REAL root layout (its Slate built, so the Menu stack activates).
//
// Pawpocalypse.UI.Tree.Opens       T opens a browse on the Menu stack (once), the field holds, every node seated where the
//                                  layout says, the captions (BRANCH_LABELS: "Splash"), the fit and centre, the X.
// Pawpocalypse.UI.Tree.Browse      read-only: a press buys nothing however rich; READY halos still show; the tooltip says so;
//                                  closing releases the field and never travels.
// Pawpocalypse.UI.Tree.Spend       a Reawaken (after its ceremony) opens the spend step (Continue, the cheapest node selected), a press buys one
//                                  level, the node and its link turn owned, leaving asks once (unspent / next run), then
//                                  travels to the starting biome.
// Pawpocalypse.UI.Tree.Text        every branch's effect at every level, rendered through ST_Tree + FText exactly as Godot's
//                                  _effect_text (the Godot fixture); the tooltip's title and lines.
// Pawpocalypse.UI.Tree.Navigation  every node's four rules are PcTreeLayout's picks (itself when nothing lies that way).
// Pawpocalypse.UI.Tree.Zoom        zoom about the view's centre, clamped; Q's next affordable node.
// Pawpocalypse.UI.Tree.Paint       the links (owned only with both ends), the rainbow verbs, the pulse, the job rings; the
//                                  frames LOCKED < READY < OWNED in one series.
//
// Negative tests: tools/sabotage_realm.py (`tree_*`).

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Blueprint/WidgetNavigation.h"
#include "Misc/AutomationTest.h"
#include "UI/PawWidgets.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "PcTree.h"
#include "PcTreeLayout.h"
#include "PcTreeText.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawTreeTables.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FTreeWorld
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

		UPawSkillTreeScreen* OpenTree(FAutomationTestBase& T, bool bSpend)
		{
			T.TestTrue(TEXT("the tree opens"), Hud->OpenSkillTree(bSpend));
			UPawSkillTreeScreen* Tree = Hud->GetSkillTree();
			T.TestTrue(TEXT("and is up"), Tree && Tree->IsActivated());
			return Tree;
		}
	};

	FString S(const FText& T) { return T.ToString(); }

	UWidget* Rule(const UWidget* W, EUINavigation Dir)
	{
		UWidgetNavigation* N = W ? W->Navigation.Get() : nullptr;
		if (!N)
		{
			return nullptr;
		}
		// The fields, not GetNavigationData(): that one is WITH_EDITOR only, and the Development game target compiles these
		// tests too (it failed to build until 2026-11-04).
		const FWidgetNavigationData& Data = Dir == EUINavigation::Up ? N->Up : Dir == EUINavigation::Down ? N->Down
			: Dir == EUINavigation::Left ? N->Left : Dir == EUINavigation::Right ? N->Right
			: Dir == EUINavigation::Next ? N->Next : N->Previous;
		return Data.Rule == EUINavigationRule::Explicit ? Data.Widget.Get() : nullptr;
	}

	double Luma(const FLinearColor& C) { return 0.2126 * C.R + 0.7152 * C.G + 0.0722 * C.B; }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeOpensTest, "Pawpocalypse.UI.Tree.Opens", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeOpensTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, false);
	if (!Tree)
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("a browse"), Tree->IsSpendMode());
	TestFalse(TEXT("not a second time while it is up"), W.Hud->OpenSkillTree(false));
	TestEqual(TEXT("one screen on the Menu stack"), W.Root->MenuStack->GetNumWidgets(), 1);
	TestTrue(TEXT("the field is held by the tree"), W.Realm->IsPauseHeldBy(UPawSkillTreeScreen::PauseHolder) && W.Realm->IsPaused());
	TestTrue(TEXT("the HUD knows a menu is up (hold-to-buy waits)"), W.Hud->IsMenuUp());
	TestTrue(TEXT("a menu's input config"), Tree->GetDesiredInputConfig().IsSet() && Tree->GetDesiredInputConfig()->GetInputMode() == ECommonInputMode::Menu);

	const Pc::PcTreeLayout L = Pc::LayOutTree(W.D());
	TestEqual(TEXT("every branch a node"), Tree->GetNodeCount(), static_cast<int32>(W.D().Branches.size()));
	int32 Seated = 0;
	for (const auto& KV : L.Nodes)
	{
		const UPawTreeNode* N = Tree->GetNode(UTF8_TO_TCHAR(KV.first.c_str()));
		Seated += N && Tree->GetLayout().Nodes.at(KV.first).X == KV.second.X && Tree->GetLayout().Nodes.at(KV.first).Y == KV.second.Y ? 1 : 0;
	}
	TestEqual(TEXT("every node where PcTreeLayout seats it"), Seated, static_cast<int32>(L.Nodes.size()));
	// Phase 6 slice 5b: every node wears its branch's icon (icon_path; skill_tree_ui.gd ICON_SIZE 56).
	int32 Pictured = 0;
	for (const Pc::PcSkillBranch& B : W.D().Branches)
	{
		const UPawTreeNode* N = Tree->GetNode(UTF8_TO_TCHAR(B.Id.c_str()));
		Pictured += N && !B.IconPath.empty() && N->GetIconName() == TEXT("T_") + FPaths::GetBaseFilename(UTF8_TO_TCHAR(B.IconPath.c_str())) ? 1 : 0;
	}
	TestEqual(TEXT("every node its branch's icon"), Pictured, static_cast<int32>(W.D().Branches.size()));
	TestEqual(TEXT("captions are BRANCH_LABELS, not the data's name"), S(Tree->GetCaption(TEXT("overkill"))), FString(TEXT("Splash")));
	TestEqual(TEXT("a trunk's caption"), S(Tree->GetCaption(TEXT("claws"))), FString(TEXT("Claws")));
	const Pc::PcRect2 B = Pc::TreeBounds(L);
	TestEqual(TEXT("fitted on open (the content area under the header)"), Tree->GetZoom(), Pc::TreeFitZoom(B, 1920.0, 950.0), 1e-9);
	TestEqual(TEXT("and centred"), Tree->GetPan().X, 960.0 - B.CentreX() * Tree->GetZoom(), 1e-6);
	TestEqual(TEXT("the browse X"), S(Tree->GetCloseButton()->GetLabel()), FString(TEXT("X")));
	TestEqual(TEXT("the balance"), S(Tree->GetStardustText()), FString(TEXT("Stardust: 0")));
	TestTrue(TEXT("the focus target is a node"), Cast<UPawTreeNode>(Tree->GetDesiredFocusTarget()) != nullptr);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeBrowseTest, "Pawpocalypse.UI.Tree.Browse", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeBrowseTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Stardust = 1e12;
	// Away from the starting biome, so a browse that wrongly travelled there would show.
	TestTrue(TEXT("in the second biome"), W.Realm->Travel(1));
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, false);
	if (!Tree)
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("a browse never buys"), Tree->PressNode(TEXT("instinct")));
	TestEqual(TEXT("nothing bought"), Pc::BranchLevel(W.P(), "instinct"), static_cast<int64_t>(0));
	TestEqual(TEXT("nothing spent"), W.P().Stardust, 1e12);
	TestTrue(TEXT("an affordable node still reads READY"), Tree->GetNode(TEXT("instinct"))->GetState() == Pc::PcTreeNodeState::Ready);
	TestTrue(TEXT("with its halo"), Tree->IsHaloShown(TEXT("instinct")));
	TestFalse(TEXT("no shine outside the spend step"), Tree->GetNode(TEXT("unlock_aura"))->IsShining());
	Tree->ShowTip(TEXT("instinct"), true);
	const TArray<FText> Lines = Tree->GetTipLines();
	TestTrue(TEXT("the tooltip says browsing is read-only"), Lines.ContainsByPredicate([](const FText& T) { return T.ToString() == TEXT("Spend after a Reawaken."); }));
	const int64 Travels = W.Realm->GetSim().Travels;
	Tree->PressClose();
	TestFalse(TEXT("one press closes a browse"), Tree->IsActivated());
	TestFalse(TEXT("the field is let go"), W.Realm->IsPaused());
	TestEqual(TEXT("a browse never travels"), W.Realm->GetSim().Travels, Travels);
	TestEqual(TEXT("still in the second biome"), static_cast<int64>(W.Realm->GetSim().Field.TierIndex), static_cast<int64>(1));
	TestTrue(TEXT("it opens again"), W.Hud->OpenSkillTree(false));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeSpendTest, "Pawpocalypse.UI.Tree.Spend", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeSpendTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();
	// A Reawaken through the HUD's own two presses opens the spend step instead of travelling.
	if (!W.Hud->GetAskReawakenButton()->IsTicked())
	{
		W.Hud->ToggleAskBeforeReawaken();
	}
	P.LifetimeCatnipEver = 1e12;
	P.LifetimeCatnipThisCycle = 1e12;
	P.Catnip = 1e12;
	// The first trunk bought out, so the cheapest node for sale is not where a tree opens anyway (the first trunk).
	P.BranchLevels[Pc::TrunkWheel().front()] = Pc::BranchMaxLevel(*D.FindBranch(Pc::TrunkWheel().front()));
	W.Hud->RefreshNow();
	const int64 TravelsBefore = W.Realm->GetSim().Travels;
	W.Hud->PressReawaken();
	W.Hud->PressReawaken();
	TestEqual(TEXT("reawakened"), P.ReawakenCount, static_cast<int64_t>(1));
	// The ceremony first (Pawpocalypse.UI.Ceremony.*); its end opens the spend step.
	TestTrue(TEXT("the ceremony plays first"), W.Hud->GetCeremony()->IsPlaying() && !W.Hud->IsMenuUp());
	for (int32 I = 0; I < 400 && W.Hud->GetCeremony()->IsPlaying(); ++I)
	{
		W.Hud->Pump(1.0 / 60.0);
	}
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	if (!TestTrue(TEXT("the spend step is up"), Tree && Tree->IsActivated() && Tree->IsSpendMode()))
	{
		W.Close();
		return false;
	}
	TestEqual(TEXT("no travel before the spend step ends"), W.Realm->GetSim().Travels, TravelsBefore);
	TestEqual(TEXT("Continue"), S(Tree->GetCloseButton()->GetLabel()), FString(TEXT("Continue")));
	const std::string Cheapest = Pc::CheapestAffordableBranch(D, P);
	TestFalse(TEXT("the payout buys something"), Cheapest.empty());
	TestTrue(TEXT("and it is not the first trunk"), Cheapest != Pc::TrunkWheel().front());
	const UPawTreeNode* Target = Cast<UPawTreeNode>(Tree->GetDesiredFocusTarget());
	TestTrue(TEXT("the selection opens on the cheapest node for sale"), Target && Target->BranchId == UTF8_TO_TCHAR(Cheapest.c_str()));

	const FString Id = UTF8_TO_TCHAR(Cheapest.c_str());
	const double Before = P.Stardust;
	const double Price = Pc::NextBranchCost(D, P, Cheapest);
	TestTrue(TEXT("a press buys one level"), Tree->PressNode(Id));
	TestEqual(TEXT("at its price"), P.Stardust, Before - Price, 1e-6);
	TestEqual(TEXT("one level"), Pc::BranchLevel(P, Cheapest), static_cast<int64_t>(1));
	TestTrue(TEXT("the node turns owned"), Tree->GetNode(Id)->GetState() == Pc::PcTreeNodeState::Owned);
	bool bLinkOwned = false;
	for (const UPawTreeLinks::FLink& L : Tree->GetLinks()->Links)
	{
		const FVector2D At(Tree->GetLayout().Nodes.at(Cheapest).X, Tree->GetLayout().Nodes.at(Cheapest).Y);
		if (L.B.Equals(At) && L.bOwned)
		{
			bLinkOwned = true;
		}
	}
	const Pc::PcSkillBranch* Br = D.FindBranch(Cheapest);
	if (Br && Br->Parent.empty())
	{
		TestTrue(TEXT("a trunk's link from the hub turns owned"), bLinkOwned);
	}
	FFormatNamedArguments A;
	A.Add(TEXT("n"), PawText::Count(static_cast<int64>(P.Stardust)));
	TestEqual(TEXT("the balance follows"), S(Tree->GetStardustText()), S(PawText::Ui(TEXT("TREE_stardust"), A)));

	// Leaving asks once, then goes; the second press travels to the starting biome.
	const bool bUnspent = Pc::HasAffordableBranch(D, P);
	Tree->PressClose();
	TestTrue(TEXT("the first press arms"), Tree->IsCloseArmed() && Tree->IsActivated());
	TestTrue(TEXT("the plate in the middle says so"), Tree->IsWarningShown());
	TestEqual(TEXT("the warning"), S(Tree->GetWarningText()),
		FString(bUnspent ? TEXT("You still have Stardust to spend.\nPress again to leave anyway.") : TEXT("Back to the game?\nPress again to start the next run.")));
	TestEqual(TEXT("and the button"), S(Tree->GetCloseButton()->GetLabel()), FString(bUnspent ? TEXT("Leave Stardust unspent?") : TEXT("Start the next run?")));
	TestEqual(TEXT("still no travel"), W.Realm->GetSim().Travels, TravelsBefore);
	const int64 Where = Pc::ReawakenTravelIndex(D, P);
	TestTrue(TEXT("a starting biome to go to"), Where >= 0);
	Tree->PressClose();
	TestFalse(TEXT("the second press leaves"), Tree->IsActivated());
	TestEqual(TEXT("and travels to the starting biome, once"), W.Realm->GetSim().Travels, TravelsBefore + 1);
	TestEqual(TEXT("the biome Territory decides"), static_cast<int64>(W.Realm->GetSim().Field.TierIndex), Where);
	TestFalse(TEXT("the field runs"), W.Realm->IsPaused());

	// A purchase disarms a pending leave (the warning would fire on the wrong press).
	P.Stardust = 1e12;
	UPawSkillTreeScreen* Again = W.OpenTree(*this, true);
	if (Again)
	{
		Again->PressClose();
		TestTrue(TEXT("armed"), Again->IsCloseArmed());
		const std::string Next = Pc::CheapestAffordableBranch(D, P);
		Again->PressNode(UTF8_TO_TCHAR(Next.c_str()));
		TestFalse(TEXT("a purchase disarms"), Again->IsCloseArmed());
		TestFalse(TEXT("and hides the plate"), Again->IsWarningShown());

		// Buying Auto-Travel or Auto-Card switches that automation ON (doc 02 §9: otherwise the player buys it and nothing
		// happens). Their limb is owned up to them, so the press itself is the only thing that can switch them.
		UPawGameSubsystem* G = W.Game;
		G->Slot().bAutoTravel = false;
		G->Slot().bAutoPickCard = false;
		for (const char* Node : { "auto_travel", "auto_card" })
		{
			for (std::string Up = D.FindBranch(Node)->Parent; !Up.empty(); Up = D.FindBranch(Up)->Parent)
			{
				P.BranchLevels[Up] = Pc::BranchMaxLevel(*D.FindBranch(Up));
			}
		}
		P.Stardust = 1e12;
		TestTrue(TEXT("Auto-Travel bought"), Again->PressNode(TEXT("auto_travel")));
		TestTrue(TEXT("and switched on"), G->Slot().bAutoTravel);
		TestTrue(TEXT("the realm runs with it"), W.Realm->GetAutoTravel());
		TestFalse(TEXT("Auto-Card not yet"), G->Slot().bAutoPickCard);
		TestTrue(TEXT("Auto-Card bought"), Again->PressNode(TEXT("auto_card")));
		TestTrue(TEXT("and switched on"), G->Slot().bAutoPickCard);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeTextTest, "Pawpocalypse.UI.Tree.Text", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeTextTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	// Every branch's _effect_text at every level, Godot's English, against the FText the game builds (ST_Tree + Format + Join).
	TArray<FString> Rows;
	const FString Fixture = FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/tree_text_godot.tsv");
	if (!TestTrue(TEXT("the Godot fixture"), FFileHelper::LoadFileToStringArray(Rows, *Fixture)))
	{
		W.Close();
		return false;
	}
	int32 Compared = 0, Wrong = 0;
	for (const FString& Row : Rows)
	{
		TArray<FString> C;
		Row.ParseIntoArray(C, TEXT("\t"), false);
		if (C.Num() != 5 || C[0] != TEXT("fx"))
		{
			continue;
		}
		const FString Got = S(PawText::TreeEffect(Pc::TreeEffectPhrases(W.D(), TCHAR_TO_UTF8(*C[1]), FCString::Atoi64(*C[2]))));
		if (Got != C[4])
		{
			if (++Wrong <= 5)
			{
				AddError(FString::Printf(TEXT("%s level %s: \"%s\", Godot \"%s\""), *C[1], *C[2], *Got, *C[4]));
			}
		}
		++Compared;
	}
	TestTrue(TEXT("every branch level compared"), Compared > 600);
	TestEqual(TEXT("and each says what Godot says"), Wrong, 0);

	// The tooltip as read: a ladder's title carries owned/max; Next in its colour; why a node is shut.
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, true);
	if (Tree)
	{
		Tree->ShowTip(TEXT("claws"), true);
		const Pc::PcSkillBranch* Claws = W.D().FindBranch("claws");
		TestEqual(TEXT("a ladder's title"), S(Tree->GetTipTitle()), FString::Printf(TEXT("Claws  0/%lld"), static_cast<long long>(Pc::BranchMaxLevel(*Claws))));
		const TArray<FText> Lines = Tree->GetTipLines();
		TestTrue(TEXT("the blurb first"), Lines.Num() > 0 && S(Lines[0]) == S(PawText::TreeBlurb("claws")));
		const FString Next = FString(TEXT("Next: ")) + S(PawText::TreeEffect(Pc::TreeEffectPhrases(W.D(), "claws", 1)));
		TestTrue(TEXT("the next level"), Lines.ContainsByPredicate([&](const FText& T) { return T.ToString() == Next; }));
		TestTrue(TEXT("its cost"), Lines.ContainsByPredicate([&](const FText& T) { return T.ToString().StartsWith(TEXT("Cost: ")) && T.ToString().EndsWith(TEXT(" Stardust")); }));
		TestTrue(TEXT("too poor: what you have"), Lines.ContainsByPredicate([](const FText& T) { return T.ToString() == TEXT("You have 0."); }));
		// A blurb's action token is the device's prompt (input_prompts_godot.tsv blurb_pounce, family 0: the left mouse button).
		Tree->ShowTip(TEXT("unlock_pounce"), true);
		const TArray<FText> Pounce = Tree->GetTipLines();
		TestTrue(TEXT("a blurb's {pounce} is the mouse button's glyph"), Pounce.Num() > 0
			&& S(Pounce[0]).Contains(TEXT("a burst on <g:input_keyboard_mouse_left>, worth more")));
		// An unlock's title has no count.
		Tree->ShowTip(TEXT("unlock_aura"), true);
		TestEqual(TEXT("an unlock's title"), S(Tree->GetTipTitle()), FString(TEXT("Aura")));
		Tree->ShowTip(FString(), false);
		TestEqual(TEXT("hidden"), Tree->GetTipLines().Num(), 0);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeNavigationTest, "Pawpocalypse.UI.Tree.Navigation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeNavigationTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, false);
	if (!Tree)
	{
		W.Close();
		return false;
	}
	const Pc::PcTreeLayout& L = Tree->GetLayout();
	const Pc::PcVec2 Dirs[4] = { { 0.0, -1.0 }, { 0.0, 1.0 }, { -1.0, 0.0 }, { 1.0, 0.0 } };
	const EUINavigation Nav[4] = { EUINavigation::Up, EUINavigation::Down, EUINavigation::Left, EUINavigation::Right };
	int32 Right = 0, Total = 0, StayPut = 0;
	for (const std::string& Id : L.Order)
	{
		const UPawTreeNode* N = Tree->GetNode(UTF8_TO_TCHAR(Id.c_str()));
		for (int32 K = 0; K < 4; ++K)
		{
			const std::string Pick = Pc::TreeNavPick(L, Id, Dirs[K]);
			const UWidget* Want = Pick.empty() ? N : Tree->GetNode(UTF8_TO_TCHAR(Pick.c_str()));
			Right += Rule(N, Nav[K]) == Want ? 1 : 0;
			StayPut += Pick.empty() ? 1 : 0;
			++Total;
		}
		TestNotNull(TEXT("Next is written"), Rule(N, EUINavigation::Next));
	}
	TestEqual(TEXT("every node's four rules are the layout's picks"), Right, Total);
	TestTrue(TEXT("an edge node keeps the selection (stay put), never leaves the tree"), StayPut > 0);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeZoomTest, "Pawpocalypse.UI.Tree.Zoom", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeZoomTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, false);
	if (!Tree)
	{
		W.Close();
		return false;
	}
	// The point under the middle of the view stays there.
	const FVector2D Centre(960.0, 475.0);
	const FVector2D Under = (Centre - Tree->GetPan()) / Tree->GetZoom();
	Tree->ZoomBy(Pc::TreeLayout::ZoomStep);
	TestEqual(TEXT("zoomed in by a step"), Tree->GetZoom(), Pc::TreeFitZoom(Pc::TreeBounds(Tree->GetLayout()), 1920.0, 950.0) + Pc::TreeLayout::ZoomStep, 1e-9);
	TestTrue(TEXT("the centre stays put"), ((Centre - Tree->GetPan()) / Tree->GetZoom()).Equals(Under, 1e-6));
	for (int32 I = 0; I < 40; ++I)
	{
		Tree->ZoomBy(Pc::TreeLayout::ZoomStep);
	}
	TestEqual(TEXT("clamped at the top"), Tree->GetZoom(), Pc::TreeLayout::ZoomMax, 1e-9);
	for (int32 I = 0; I < 40; ++I)
	{
		Tree->ZoomBy(-Pc::TreeLayout::ZoomStep);
	}
	TestEqual(TEXT("and the bottom"), Tree->GetZoom(), Pc::TreeLayout::ZoomMin, 1e-9);

	// Q / X: the next node for sale after the selection, in branch order.
	TestTrue(TEXT("nothing for sale, nothing moves"), Tree->JumpToAffordable().IsEmpty());
	W.P().Stardust = 1e12;
	W.P().ReawakenCount = 99;
	const FString From = UPawSkillTreeScreen::LastSelected();
	const FString Got = Tree->JumpToAffordable();
	TestEqual(TEXT("the next affordable"), Got, FString(UTF8_TO_TCHAR(Pc::NextAffordableBranch(W.D(), W.P(), TCHAR_TO_UTF8(*From)).c_str())));
	TestEqual(TEXT("and it is the selection now"), UPawSkillTreeScreen::LastSelected(), Got);
	TestEqual(TEXT("its tooltip docks"), Tree->GetTipBranch(), Got);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreePaintTest, "Pawpocalypse.UI.Tree.Paint", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreePaintTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, false);
	if (!Tree)
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	const UPawTreeLinks* L = Tree->GetLinks();
	TestEqual(TEXT("a link per branch"), L->Links.Num(), static_cast<int32>(D.Branches.size()));
	int32 Owned = 0;
	for (const UPawTreeLinks::FLink& K : L->Links)
	{
		Owned += K.bOwned ? 1 : 0;
	}
	TestEqual(TEXT("nothing bought, nothing walked"), Owned, 0);
	int32 Verbs = 0, Jobs = 0;
	for (const TCHAR* Id : PawTree::RainbowBranches)
	{
		Verbs += D.FindBranch(TCHAR_TO_UTF8(Id)) ? 1 : 0;
	}
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		Jobs += B.JobId.empty() ? 0 : 1;
	}
	TestEqual(TEXT("a rainbow ring per verb"), L->Rainbow.Num(), Verbs);
	TestEqual(TEXT("the Unicorn breathes"), L->Pulse.Num(), 1);
	TestEqual(TEXT("a gold ring per job node"), L->JobRings.Num(), Jobs);

	// A child bought under an unbought parent (a free grant can do it) is not a walked road; with the parent, it is.
	std::string Child, Parent;
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		if (!B.Parent.empty())
		{
			Child = B.Id;
			Parent = B.Parent;
			break;
		}
	}
	W.P().BranchLevels[Child] = 1;
	Tree->RefreshAll();
	auto LinkTo = [&](const std::string& Id)
	{
		const FVector2D At(Tree->GetLayout().Nodes.at(Id).X, Tree->GetLayout().Nodes.at(Id).Y);
		for (const UPawTreeLinks::FLink& K : Tree->GetLinks()->Links)
		{
			if (K.B.Equals(At))
			{
				return K;
			}
		}
		return UPawTreeLinks::FLink();
	};
	TestFalse(TEXT("child without parent: not owned"), LinkTo(Child).bOwned);
	W.P().BranchLevels[Parent] = 1;
	Tree->RefreshAll();
	TestTrue(TEXT("both ends: owned"), LinkTo(Child).bOwned);
	FLinearColor Want = FLinearColor::White; // BRANCH_COLORS.get(id, Color.WHITE)
	for (const PawTree::FBranchColour& C : PawTree::BranchColours)
	{
		if (FString(UTF8_TO_TCHAR(Child.c_str())) == C.Id)
		{
			Want = C.Colour;
		}
	}
	TestTrue(TEXT("an owned link wears its branch's colour"), LinkTo(Child).Colour.Equals(Want));

	// The frames: one series, LOCKED < READY < OWNED in brightness, for every trunk's colour; the node wears it.
	for (const PawTree::FBranchColour& C : PawTree::BranchColours)
	{
		FLinearColor LF, LE, RF, RE, OF, OE;
		UPawTreeNode::FrameColours(Pc::PcTreeNodeState::Locked, C.Colour, LF, LE);
		UPawTreeNode::FrameColours(Pc::PcTreeNodeState::Ready, C.Colour, RF, RE);
		UPawTreeNode::FrameColours(Pc::PcTreeNodeState::Owned, C.Colour, OF, OE);
		if (!TestTrue(FString::Printf(TEXT("%s: locked <= ready <= owned"), C.Id), Luma(LF) <= Luma(RF) + 1e-6 && Luma(RF) <= Luma(OF) + 1e-6
			&& LE.A < RE.A && RE.A < OE.A + 1e-6))
		{
			break;
		}
	}
	FLinearColor Fill, Edge;
	UPawTreeNode::FrameColours(Pc::PcTreeNodeState::Owned, PawTree::BranchColours[0].Colour, Fill, Edge);
	const UPawTreeNode* Claws = Tree->GetNode(PawTree::BranchColours[0].Id);
	W.P().BranchLevels[TCHAR_TO_UTF8(PawTree::BranchColours[0].Id)] = 1;
	Tree->RefreshAll();
	TestTrue(TEXT("an owned node wears the owned frame"), Claws && Claws->GetFrameEdge().Equals(Edge) && Claws->GetFrameFill().Equals(Fill));
	TestEqual(TEXT("a ladder's counter"), S(Claws->GetCounter()), FString::Printf(TEXT("1/%lld"),
		static_cast<long long>(Pc::BranchMaxLevel(*D.FindBranch(TCHAR_TO_UTF8(PawTree::BranchColours[0].Id))))));
	TestTrue(TEXT("an unlock has no counter"), Tree->GetNode(TEXT("unlock_aura"))->GetCounter().IsEmpty());
	W.Close();
	return true;
}

#endif
