// The skill tree's engine half (docs/09 L3; docs/05 §6.2). PcTreeLayout / PcTreeText are pinned in PawCore against the real
// Godot code; these prove the screen built on them, on the REAL root layout (its Slate built, so the Menu stack activates).
//
// Pawpocalypse.UI.Tree.Opens       T opens a browse on the Menu stack (once), the field holds, every node seated where the
//                                  layout says, the names (BRANCH_LABELS: "Splash"; a branch's in the field), the reveal
//                                  (a fresh game shows part of the tree), the fit to what is shown and the centre, the X.
// Pawpocalypse.UI.Tree.Browse      read-only: a press buys nothing however rich; READY halos still show; the tooltip says so;
//                                  closing releases the field and never travels.
// Pawpocalypse.UI.Tree.Spend       a Reawaken (after its ceremony) opens the spend step (Continue, the cheapest node selected), a press buys one
//                                  level, the node and its link turn owned, leaving asks in the confirm dialog (P4.7: "You
//                                  still have Stardust to spend." / "Start the next run?", Leave / Stay), Stay stays, Leave
//                                  travels to the starting biome.
// Pawpocalypse.UI.Tree.Text        every branch's effect at every level, rendered through ST_Tree + FText exactly as Godot's
//                                  _effect_text (the Godot fixture); the tooltip's title and lines.
// Pawpocalypse.UI.Tree.Navigation  every shown node's four rules are PcTreeLayout's picks among the shown (itself when
//                                  nothing lies that way); a hidden node is never a target.
// Pawpocalypse.UI.Tree.Zoom        zoom about the view's centre, clamped; Q's next affordable node.
// Pawpocalypse.UI.Tree.Paint       the links (owned only with both ends, in the trunk's colour, as shown as their far end),
//                                  the Unicorn's rings; the stickers (C1: a shape per kind, owned in the trunk's card, the
//                                  rest dark, the gold ring to buy now), the chips (level, tick, price), the reveal.
// Pawpocalypse.UI.Tree.TipOnce     the tooltip is built when its node or numbers change, not every frame; the selection
//                                  follows a node's focus event (review CR9).
// Pawpocalypse.UI.Tree.KeepsKeys   the tree (and the pause menu, settings, the title: every screen that reads Back itself) holds
//                                  the keys when a click lands on nothing focusable; Esc to the tree itself asks, then leaves
//                                  (playtest 2026-11-26, notes 11, 27).
//
// Negative tests: tools/sabotage_realm.py (`tree_*`).

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Blueprint/WidgetNavigation.h"
#include "Misc/AutomationTest.h"
#include "UI/PawLayoutCheck.h"
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
#include "UI/PawMainMenu.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawBoba.h"
#include "UI/PawConfirmDialog.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawTreeTables.h"
#include <set>
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/PawTextTesting.h"

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
			Hud->SetModalStack(Root->ModalStack); // the tree's leave question (P4.7)
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

	// What PcTreeLayout reveals for this progress (C1).
	std::set<std::string> ShownOf(const Pc::PcGameData& D, const Pc::PcProgress& P)
	{
		std::set<std::string> Out;
		for (const Pc::PcSkillBranch& B : D.Branches)
		{
			if (Pc::TreeNodeReveal(D, P, B.Id) != Pc::PcTreeReveal::Hidden)
			{
				Out.insert(B.Id);
			}
		}
		return Out;
	}

	const PawTree::FTrunkLook* LookOf(const Pc::PcGameData& D, const std::string& Id)
	{
		const FString Trunk = UTF8_TO_TCHAR(Pc::TreeTrunkOf(D, Id).c_str());
		for (const PawTree::FTrunkLook& L : PawTree::TrunkLooks)
		{
			if (Trunk == L.Id)
			{
				return &L;
			}
		}
		return nullptr;
	}
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
	const UPawBobaSubsystem* Boba = W.GI->GetSubsystem<UPawBobaSubsystem>();
	TestTrue(TEXT("P4.3: Boba is held off while the tree is up (never over the skill tree)"),
		Boba && Boba->GetState().Suppressors.count("tree") == 1);
	TestTrue(TEXT("a menu's input config"), Tree->GetDesiredInputConfig().IsSet() && Tree->GetDesiredInputConfig()->GetInputMode() == ECommonInputMode::Menu);

	const Pc::PcTreeLayout L = Pc::LayOutTree(W.D());
	TestEqual(TEXT("every skill on the tree a node (a retired one is not, D-e)"), Tree->GetNodeCount(), static_cast<int32>(Pc::BranchOrder(W.D()).size()));
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
	TestEqual(TEXT("every node its branch's icon"), Pictured, static_cast<int32>(Pc::BranchOrder(W.D()).size()));
	// C1: no caption under a node; the name is the tooltip's title, the branch's name stands in the field.
	Tree->ShowTip(TEXT("overkill"), true);
	TestTrue(TEXT("names are BRANCH_LABELS, not the data's name"), S(Tree->GetTipTitle()).StartsWith(TEXT("Splash")));
	Tree->ShowTip(FString(), false);
	TestEqual(TEXT("a branch's name in the field"), S(Tree->GetBranchLabel(TEXT("claws"))), FString(TEXT("CLAWS")));
	// The reveal: a fresh game shows the trunks and one ring ahead, the rest hidden (collapsed, out of reach).
	const std::set<std::string> Shown = ShownOf(W.D(), W.P());
	TestEqual(TEXT("what is shown is PcTreeLayout's reveal"), Tree->GetShownCount(), static_cast<int32>(Shown.size()));
	TestTrue(TEXT("a fresh game shows part of the tree"), Tree->GetShownCount() < Tree->GetNodeCount());
	int32 Agrees = 0;
	for (const Pc::PcSkillBranch& Br : W.D().Branches)
	{
		const UPawTreeNode* N = Tree->GetNode(UTF8_TO_TCHAR(Br.Id.c_str()));
		const bool bShown = Shown.count(Br.Id) != 0;
		// A retired skill (D-e) has no node at all: off the board by construction.
		Agrees += (N ? bShown == Tree->IsShown(UTF8_TO_TCHAR(Br.Id.c_str())) && bShown == (N->GetVisibility() != ESlateVisibility::Collapsed)
			: !bShown && Pc::IsBranchRetired(W.D(), Br.Id)) ? 1 : 0;
	}
	TestEqual(TEXT("every hidden node off the board, every shown one on it"), Agrees, static_cast<int32>(W.D().Branches.size()));
	const Pc::PcRect2 B = Tree->GetFitBounds();
	TestTrue(TEXT("the fit frames what is shown, not the whole tree"), B.W < Pc::TreeBounds(L).W);
	TestEqual(TEXT("fitted on open (the content area under the header)"), Tree->GetZoom(), Pc::TreeFitZoom(B, 1920.0, 950.0), 1e-9);
	TestEqual(TEXT("and centred"), Tree->GetPan().X, 960.0 - B.CentreX() * Tree->GetZoom(), 1e-6);
	// R4h: the real view size arrives on the first tick (at 150 % UI: 1280 x 720); the fit and the centre follow it.
	Tree->SetViewSize(FVector2D(1280.0, 720.0));
	const double Content150 = 720.0 - (1080.0 - 950.0);
	TestEqual(TEXT("150 %: fitted to the view the player sees"), Tree->GetZoom(), Pc::TreeFitZoom(B, 1280.0, Content150), 1e-9);
	TestEqual(TEXT("150 %: centred across"), Tree->GetPan().X, 640.0 - B.CentreX() * Tree->GetZoom(), 1e-6);
	TestEqual(TEXT("150 %: centred down"), Tree->GetPan().Y, Content150 * 0.5 - B.CentreY() * Tree->GetZoom(), 1e-6);
	// Once the player moves the view, a size change leaves it alone.
	Tree->ZoomBy(Pc::TreeLayout::ZoomStep);
	const double Chosen = Tree->GetZoom();
	Tree->SetViewSize(FVector2D(1920.0, 1080.0));
	TestEqual(TEXT("a zoom the player chose survives a resize"), Tree->GetZoom(), Chosen, 1e-9);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeTipOnceTest, "Pawpocalypse.UI.Tree.TipOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Review CR9: the tooltip's lines are built when its node or its numbers change, not every frame (NativeTick only places
// it); the selection follows a node's focus event, not a scan of every node every frame.
bool FPawTreeTipOnceTest::RunTest(const FString& Parameters)
{
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Stardust = 1e12;
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, false);
	if (!Tree)
	{
		W.Close();
		return false;
	}
	auto Texts = [](const TArray<FText>& Lines)
	{
		TArray<FString> Out;
		for (const FText& L : Lines)
		{
			Out.Add(L.ToString());
		}
		return FString::Join(Out, TEXT(" | "));
	};
	Tree->ShowTip(TEXT("instinct"), true);
	const int32 Built = Tree->GetTipBuilds();
	const FString Lines = Texts(Tree->GetTipLines());
	TestFalse(TEXT("the tip has lines"), Lines.IsEmpty());
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Tree->UpdateTip(); // NativeTick's tooltip step
	}
	TestEqual(TEXT("a second of frames builds nothing"), Tree->GetTipBuilds(), Built);
	TestEqual(TEXT("still docked on the node"), Tree->GetTipBranch(), FString(TEXT("instinct")));
	TestEqual(TEXT("with the same lines"), Texts(Tree->GetTipLines()), Lines);

	// Its numbers change (a level): built again, once.
	W.P().BranchLevels["instinct"] += 1;
	Tree->UpdateTip();
	Tree->UpdateTip();
	TestEqual(TEXT("a new level builds it again, once"), Tree->GetTipBuilds(), Built + 1);
	TestNotEqual(TEXT("with the new numbers"), Texts(Tree->GetTipLines()), Lines);

	// Another node takes the focus: the selection and the docked tip follow it.
	const FString Other = TEXT("unlock_aura");
	Tree->NodeFocused(Other);
	TestEqual(TEXT("the selection follows the node's focus"), Tree->LastSelected(), Other);
	TestEqual(TEXT("and the tip docks on it"), Tree->GetTipBranch(), Other);
	TestEqual(TEXT("built once"), Tree->GetTipBuilds(), Built + 2);
	Tree->UpdateTip();
	TestEqual(TEXT("and not again"), Tree->GetTipBuilds(), Built + 2);

	int32 Told = 0;
	for (const std::string& Id : Tree->GetLayout().Order)
	{
		const UPawTreeNode* N = Tree->GetNode(UTF8_TO_TCHAR(Id.c_str()));
		Told += N && N->Screen.Get() == Tree ? 1 : 0;
	}
	TestEqual(TEXT("every node tells this screen of its focus"), Told, Tree->GetNodeCount());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeSpendTest, "Pawpocalypse.UI.Tree.Spend", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeSpendTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
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

	// Leaving asks (P4.7, playtest 2026-11-26 note 27: a real question, not "press again"), Stay selected; Stay stays; Leave
	// travels to the starting biome.
	const bool bUnspent = Pc::HasAffordableBranch(D, P);
	Tree->PressClose();
	UPawConfirmDialog* Ask = Tree->GetLeaveDialog();
	TestTrue(TEXT("Continue asks, the tree still up"), Ask != nullptr && Tree->IsActivated());
	TestEqual(TEXT("the question"), Ask ? S(Ask->GetPrompt()) : FString(),
		FString(bUnspent ? TEXT("You still have Stardust to spend.") : TEXT("Start the next run?")));
	TestTrue(TEXT("Leave / Stay"), Ask && S(Ask->GetYesLabel()) == TEXT("Leave") && S(Ask->GetNoLabel()) == TEXT("Stay"));
	TestTrue(TEXT("Stay selected (the safe answer)"), Ask && Ask->GetDesiredFocusTarget() == Ask->GetNoButton());
	TestEqual(TEXT("Continue keeps its word"), S(Tree->GetCloseButton()->GetLabel()), FString(TEXT("Continue")));
	Tree->PressClose();
	TestTrue(TEXT("one question at a time"), Tree->GetLeaveDialog() == Ask && Tree->IsActivated());
	TestTrue(TEXT("Stay answers"), Tree->AnswerLeave(false));
	TestTrue(TEXT("Stay: the tree stays, the question goes"), Tree->IsActivated() && !Tree->IsAskingToLeave());
	TestEqual(TEXT("still no travel"), W.Realm->GetSim().Travels, TravelsBefore);
	const int64 Where = Pc::ReawakenTravelIndex(D, P);
	TestTrue(TEXT("a starting biome to go to"), Where >= 0);
	Tree->PressClose();
	TestTrue(TEXT("asked again"), Tree->IsAskingToLeave());
	TestTrue(TEXT("Leave answers"), Tree->AnswerLeave(true));
	TestFalse(TEXT("Leave leaves"), Tree->IsActivated());
	TestEqual(TEXT("and travels to the starting biome, once"), W.Realm->GetSim().Travels, TravelsBefore + 1);
	TestEqual(TEXT("the biome Territory decides"), static_cast<int64>(W.Realm->GetSim().Field.TierIndex), Where);
	TestFalse(TEXT("the field runs"), W.Realm->IsPaused());

	// A purchase disarms a pending leave (the warning would fire on the wrong press).
	P.Stardust = 1e12;
	UPawSkillTreeScreen* Again = W.OpenTree(*this, true);
	if (Again)
	{
		// Buying Auto-Travel switches that automation ON (doc 02 §9: otherwise the player buys it and nothing happens). Its
		// limb is owned up to it, so the press itself is the only thing that can switch it. Auto-Card is retired (D-e:
		// auto-pick is free), so never for sale.
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
		TestFalse(TEXT("Auto-Card is never for sale (retired, D-e)"), Again->PressNode(TEXT("auto_card")));
		TestFalse(TEXT("and auto-pick stays as it was"), G->Slot().bAutoPickCard);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeTextTest, "Pawpocalypse.UI.Tree.Text", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeTextTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
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
	// Twice: a fresh game's reveal, then a richer one (the rules are written again when the reveal grows).
	for (const bool bLater : { false, true })
	{
		if (bLater)
		{
			W.P().BranchLevels["claws"] = 1;
			W.P().BranchLevels["unlock_aura"] = 1;
			W.P().ReawakenCount = 6;
			Tree->RefreshAll();
		}
		const std::set<std::string> Shown = ShownOf(W.D(), W.P());
		int32 Right = 0, Total = 0, StayPut = 0, ToHidden = 0;
		for (const std::string& Id : L.Order)
		{
			const UPawTreeNode* N = Tree->GetNode(UTF8_TO_TCHAR(Id.c_str()));
			if (!Shown.count(Id))
			{
				continue;
			}
			for (int32 K = 0; K < 4; ++K)
			{
				const std::string Pick = Pc::TreeNavPick(L, Id, Dirs[K], &Shown);
				const UWidget* Want = Pick.empty() ? N : Tree->GetNode(UTF8_TO_TCHAR(Pick.c_str()));
				Right += Rule(N, Nav[K]) == Want ? 1 : 0;
				StayPut += Pick.empty() ? 1 : 0;
				const UPawTreeNode* Got = Cast<UPawTreeNode>(Rule(N, Nav[K]));
				ToHidden += Got && !Shown.count(TCHAR_TO_UTF8(*Got->BranchId)) ? 1 : 0;
				++Total;
			}
			TestNotNull(TEXT("Next is written"), Rule(N, EUINavigation::Next));
		}
		TestTrue(TEXT("some nodes shown"), Total > 0);
		TestEqual(TEXT("every shown node's four rules are the layout's picks among the shown"), Right, Total);
		TestEqual(TEXT("no rule leads to a hidden node"), ToHidden, 0);
		TestTrue(TEXT("an edge node keeps the selection (stay put), never leaves the tree"), StayPut > 0);
	}
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
	TestEqual(TEXT("zoomed in by a step"), Tree->GetZoom(), Pc::TreeFitZoom(Tree->GetFitBounds(), 1920.0, 950.0) + Pc::TreeLayout::ZoomStep, 1e-9);
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
	const FString From = Tree->LastSelected();
	const FString Got = Tree->JumpToAffordable();
	TestEqual(TEXT("the next affordable"), Got, FString(UTF8_TO_TCHAR(Pc::NextAffordableBranch(W.D(), W.P(), TCHAR_TO_UTF8(*From)).c_str())));
	TestEqual(TEXT("and it is the selection now"), Tree->LastSelected(), Got);
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
	TestEqual(TEXT("a link per skill on the tree"), L->Links.Num(), static_cast<int32>(Pc::BranchOrder(D).size()));
	int32 Owned = 0;
	for (const UPawTreeLinks::FLink& K : L->Links)
	{
		Owned += K.bOwned ? 1 : 0;
	}
	TestEqual(TEXT("nothing bought, nothing walked"), Owned, 0);
	TestTrue(TEXT("the hub's disc"), L->bHub);
	// The reveal on the board (C1): a skill two rings out is hidden, its link not drawn; one ring ahead faint, no price.
	const UPawTreeNode* Deep = Tree->GetNode(TEXT("grand_finale"));
	const UPawTreeNode* Ahead = Tree->GetNode(TEXT("aura_damage"));
	const UPawTreeNode* Trunk = Tree->GetNode(TEXT("instinct"));
	TestTrue(TEXT("two rings out: off the board"), Deep && Deep->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("one ring ahead: faint, no price"), Ahead && Ahead->GetVisibility() == ESlateVisibility::Visible
		&& FMath::IsNearlyEqual(Ahead->GetRenderOpacity(), PawTree::GhostOpacity) && Ahead->GetPrice().IsEmpty());
	TestTrue(TEXT("a trunk on sale: in full, its price on a chip"), Trunk && FMath::IsNearlyEqual(Trunk->GetRenderOpacity(), 1.0f)
		&& !Trunk->GetPrice().IsEmpty());
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
	TestEqual(TEXT("a hidden skill's link is not drawn"), LinkTo("grand_finale").Alpha, 0.0f);
	TestEqual(TEXT("one ring ahead: its link faint"), LinkTo("aura_damage").Alpha, PawTree::GhostOpacity);
	TestTrue(TEXT("a link to a skill on sale: shown, dashed"), LinkTo("instinct").Alpha == 1.0f && LinkTo("instinct").bNext);
	// The shapes say the kind: a skill with levels a square, a one-time skill a circle, a job a diamond.
	const Pc::PcSkillBranch* Job = nullptr;
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		Job = !Job && !B.JobId.empty() ? &B : Job;
	}
	TestTrue(TEXT("a ladder is a square"), Tree->GetNode(TEXT("claws"))->GetLook().Shape == EPawNodeShape::Square);
	TestTrue(TEXT("an unlock is a circle"), Tree->GetNode(TEXT("unlock_aura"))->GetLook().Shape == EPawNodeShape::Circle);
	TestTrue(TEXT("a job is a diamond"), Job && Tree->GetNode(UTF8_TO_TCHAR(Job->Id.c_str()))->GetLook().Shape == EPawNodeShape::Diamond);

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
	TestFalse(TEXT("child without parent: not owned"), LinkTo(Child).bOwned);
	W.P().BranchLevels[Parent] = 1;
	Tree->RefreshAll();
	TestTrue(TEXT("both ends: owned"), LinkTo(Child).bOwned);
	const PawTree::FTrunkLook* ChildLook = LookOf(D, Child);
	TestTrue(TEXT("an owned link wears its trunk's colour"), ChildLook && LinkTo(Child).Colour.Equals(ChildLook->Link));

	// The stickers (C1, the Compass concept; designer 2026-11-26: "Darker cards for the skill menu"): owned in the trunk's
	// card on ink, a node to buy now dark in the gold ring, the rest dark; the owned card lighter than the dark.
	for (const PawTree::FTrunkLook& C : PawTree::TrunkLooks)
	{
		FLinearColor LF, LE, RF, RE, OF, OE;
		UPawTreeNode::FrameColours(Pc::PcTreeNodeState::Locked, C.Card, LF, LE);
		UPawTreeNode::FrameColours(Pc::PcTreeNodeState::Ready, C.Card, RF, RE);
		UPawTreeNode::FrameColours(Pc::PcTreeNodeState::Owned, C.Card, OF, OE);
		if (!TestTrue(FString::Printf(TEXT("%s: owned its card on ink, ready dark in the gold ring, locked dark"), C.Id),
			OF.Equals(C.Card) && OE.Equals(PawTokens::INK) && RE.Equals(PawTokens::UI_TREE_READY_RING) && RF.Equals(PawTree::NodeDark)
			&& LF.Equals(PawTree::NodeDark) && !LE.Equals(PawTokens::UI_TREE_READY_RING) && Luma(LF) < Luma(OF)))
		{
			break;
		}
	}
	const PawTree::FTrunkLook* ClawsLook = LookOf(D, "claws");
	const UPawTreeNode* Claws = Tree->GetNode(TEXT("claws"));
	const UPawTreeNode* Aura = Tree->GetNode(TEXT("unlock_aura"));
	W.P().BranchLevels["claws"] = 1;
	W.P().BranchLevels["unlock_aura"] = 1;
	Tree->RefreshAll();
	TestTrue(TEXT("an owned node wears its trunk's card on ink"), ClawsLook && Claws && Claws->GetFrameEdge().Equals(PawTokens::INK)
		&& Claws->GetFrameFill().Equals(ClawsLook->Card));
	TestEqual(TEXT("an owned ladder's level on its chip"), S(Claws->GetCounter()), FString::Printf(TEXT("1/%lld"),
		static_cast<long long>(Pc::BranchMaxLevel(*D.FindBranch("claws")))));
	TestTrue(TEXT("an owned skill shows no price"), Claws->GetPrice().IsEmpty());
	TestTrue(TEXT("an owned unlock: no counter, a tick"), Aura && Aura->GetCounter().IsEmpty() && Aura->GetLook().bCheck);
	TestTrue(TEXT("the old frame is clear"), Claws && Aura && Claws->IsFrameClear() && Aura->IsFrameClear());
	const int32 Before = Claws ? Claws->GetPlatePaints() : 0;
	const int32 ChipsBefore = Claws ? Claws->GetChipPaints() : 0;
	PawLayout::Arrange(Tree, FVector2D(1920.0, 1080.0), 1.0f, 1);
	TestTrue(TEXT("a node paints its sticker"), Claws && Claws->GetPlatePaints() > Before);
	TestTrue(TEXT("and its chips"), Claws && Claws->GetChipPaints() > ChipsBefore);
	// Zoomed out, the chips keep the text floor on screen (they grow against their node); at 1:1 they are their size.
	TestTrue(TEXT("fitted below 1:1"), Tree->GetZoom() < 1.0);
	TestEqual(TEXT("zoomed out: the chips grow to the floor"), Claws ? Claws->GetChipScale() : 0.0, 1.0 / Tree->GetZoom(), 1e-6);
	while (Tree->GetZoom() < 1.0)
	{
		Tree->ZoomBy(Pc::TreeLayout::ZoomStep);
	}
	PawLayout::Arrange(Tree, FVector2D(1920.0, 1080.0), 1.0f, 1);
	TestEqual(TEXT("at 1:1 or closer: their own size"), Claws ? Claws->GetChipScale() : 0.0, 1.0, 1e-9);
	TestEqual(TEXT("and the names theirs"), Tree->GetNameScale(), 1.0, 1e-9);
	while (Tree->GetZoom() > 0.4)
	{
		Tree->ZoomBy(-Pc::TreeLayout::ZoomStep);
	}
	TestEqual(TEXT("zoomed out: the names grow to the floor (16 px of a 30 px name)"), Tree->GetNameScale(), 16.0 / (30.0 * Tree->GetZoom()), 1e-9);
	const TArray<FVector2D> Ring = UPawTreeLinks::RingPath(FVector2D(0.0, 0.0), FVector2D(40.0));
	bool bRound = Ring.Num() > 12;
	for (const FVector2D& Pt : Ring)
	{
		bRound = bRound && FMath::IsNearlyEqual(Pt.Size(), 40.0, 1e-6);
	}
	TestTrue(TEXT("a ring is a circle from the top, closed"), bRound && FMath::IsNearlyEqual(Ring[0].Y, -40.0, 1e-6)
		&& FMath::IsNearlyEqual(Ring[0].X, 0.0, 1e-6) && Ring[0].Equals(Ring.Last()));
	// The Unicorn's rings (a trunk on sale from the start: shown in full).
	TestEqual(TEXT("the Unicorn breathes"), Tree->GetLinks()->Pulse.Num(), 1);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTreeKeepsKeysTest, "Pawpocalypse.UI.Tree.KeepsKeys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTreeKeepsKeysTest::RunTest(const FString& Parameters)
{
	// Playtest 2026-11-26 (notes 11, 27): a click on the sky, Continue or a zoom button gave focus to the game's viewport
	// (Slate focuses the first widget under a click that supports keyboard focus), so Esc never reached the tree and its
	// "unspent Stardust" warning could not be answered. The screen itself must take that focus, and read Esc there.
	FTreeWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSkillTreeScreen* Tree = W.OpenTree(*this, true);
	if (!Tree)
	{
		W.Close();
		return false;
	}
	const TSharedRef<SWidget> Slate = Tree->TakeWidget();
	TestTrue(TEXT("a click on nothing focusable leaves the keys on the tree, not the viewport"), Slate->SupportsKeyboardFocus());
	const FKeyEvent Esc(EKeys::Escape, FModifierKeysState(), 0u, false, 0u, 0u);
	Slate->OnPreviewKeyDown(FGeometry(), Esc);
	UPawConfirmDialog* Ask = Tree->GetLeaveDialog();
	TestTrue(TEXT("Esc held by the tree itself: it asks"), Ask != nullptr);
	// The question takes Back too: Esc there is Stay (the safe answer), and the tree stays.
	if (Ask)
	{
		const TSharedRef<SWidget> AskSlate = Ask->TakeWidget();
		TestTrue(TEXT("the question holds the keys after a click on its dim"), AskSlate->SupportsKeyboardFocus());
		AskSlate->OnPreviewKeyDown(FGeometry(), Esc);
	}
	TestTrue(TEXT("Esc on the question: Stay"), Tree->IsActivated() && !Tree->IsAskingToLeave());
	Slate->OnPreviewKeyDown(FGeometry(), Esc);
	TestTrue(TEXT("Esc asks again"), Tree->AnswerLeave(true));
	TestFalse(TEXT("Leave: the warning is answered, the tree gone"), Tree->IsActivated());

	// Every other screen that reads Back in its own NativeOnPreviewKeyDown: the same rule (their scrims and plates take clicks).
	const TPair<const TCHAR*, UUserWidget*> Others[] = {
		{ TEXT("the pause menu"), CreateWidget<UPawPauseMenu>(W.GI, PawUi::Resolve<UPawPauseMenu>()) },
		{ TEXT("settings"), CreateWidget<UPawSettingsDialog>(W.GI, PawUi::Resolve<UPawSettingsDialog>()) },
		{ TEXT("the title"), CreateWidget<UPawMainMenu>(W.GI, PawUi::Resolve<UPawMainMenu>()) },
	};
	for (const TPair<const TCHAR*, UUserWidget*>& O : Others)
	{
		TestTrue(FString::Printf(TEXT("%s holds the keys after a click on its background"), O.Key),
			O.Value && O.Value->TakeWidget()->SupportsKeyboardFocus());
	}
	W.Close();
	return true;
}

#endif
