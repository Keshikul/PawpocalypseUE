// The card pick's engine half (docs/09 L3; docs/11 phase 3). PcHud CardPick's decisions are pinned in PawCore; these prove
// the screen built around them: it opens on the real Menu stack when the progress holds a hand, holds the field still, shows
// each card as the player reads it, takes through the guard, re-deals on Reroll and Banish, closes and lets the field go,
// and wires its rows for the pad.
//
// Pawpocalypse.UI.CardPick.Opens       no hand: nothing; a hand: the screen is up on the Menu stack (once), the field holds,
//                                      the dim takes clicks, the title, the cards, the focus target.
// Pawpocalypse.UI.CardPick.Look        NEW / LEVEL n of max, the body at the level taking it gives, the cost frame and warning,
//                                      the flavour band and word, Blueprint's line, the Take label.
// Pawpocalypse.UI.CardPick.Guard       the rule: too soon, a key held from before the deal, a repeat, a fresh key, the mouse.
// Pawpocalypse.UI.CardPick.Take        a guarded click takes the card, closes, releases the field; a stale card closes too.
// Pawpocalypse.UI.CardPick.Reroll      the button's count, a reroll re-deals and restarts the guard, none left hides it.
// Pawpocalypse.UI.CardPick.Banish      Banish shows only with a banish held; it removes the card and re-deals; banished dry
//                                      closes.
// Pawpocalypse.UI.CardPick.Navigation  Take row wraps, down to that card's Banish, then Reroll, then back to the middle card.
// Pawpocalypse.UI.CardPick.PauseHolds  holders stack; a click swallowed while held stays swallowed until it is let go.
//
// Negative tests: tools/sabotage_realm.py (`pick_*`).

#include "Blueprint/WidgetNavigation.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "PcCards.h"
#include "PcData.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawHudScreen.h"
#include "UI/PawRootLayout.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// A standalone game instance, a realm on a fixed seed, the real root layout (its Slate built, so its stacks activate
	// what they hold) and a HUD that opens the pick on its Menu stack.
	struct FPickWorld
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
			if (!T.TestNotNull(TEXT("the realm subsystem"), Realm) || !T.TestNotNull(TEXT("the game subsystem"), Game)
				|| !T.TestTrue(TEXT("the game data is ready"), Game->IsDataReady()))
			{
				return false;
			}
			Game->NewGame();
			Realm->SetViewSize(1920.0, 1080.0);
			if (!T.TestTrue(TEXT("the realm starts"), Realm->StartRealm(77)))
			{
				return false;
			}
			Root = CreateWidget<UPawRootLayout>(GI, UPawRootLayout::StaticClass());
			Hud = CreateWidget<UPawHudScreen>(GI, UPawHudScreen::StaticClass());
			if (!T.TestNotNull(TEXT("the root layout"), Root) || !T.TestNotNull(TEXT("the HUD"), Hud))
			{
				return false;
			}
			RootSlate = Root->TakeWidget(); // the stacks' switchers exist: a widget added is activated
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

		// A hand as CardManager leaves it after _offer_hand, without the roll: the tests choose the cards.
		UPawCardPickScreen* Deal(FAutomationTestBase& T, std::vector<std::string> Hand, int64 Level = 1)
		{
			P().Hand = std::move(Hand);
			P().CatchesLevel = Level;
			Hud->Pump(0.0);
			UPawCardPickScreen* Pick = Hud->GetCardPick();
			T.TestTrue(TEXT("the pick is up"), Pick && Pick->IsActivated());
			return Pick;
		}

		// Past the guard with a fresh key press, as a player on the keys or a pad.
		void PastGuard(UPawCardPickScreen* Pick)
		{
			Pick->AdvanceClock(PawUi::ModalGuardSec + 0.05);
			Pick->NoteKeyDown(false);
		}
	};

	FString S(const FText& T) { return T.ToString(); }
	bool Shown(const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden; }

	UWidget* Nav(const UWidget* W, EUINavigation Dir)
	{
		const UWidgetNavigation* N = W ? W->Navigation.Get() : nullptr;
		if (!N)
		{
			return nullptr;
		}
		const FWidgetNavigationData& Data = Dir == EUINavigation::Up ? N->Up : Dir == EUINavigation::Down ? N->Down
			: Dir == EUINavigation::Left ? N->Left : N->Right;
		return Data.Rule == EUINavigationRule::Explicit ? Data.Widget.Get() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickOpensTest, "Pawpocalypse.UI.CardPick.Opens", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickOpensTest::RunTest(const FString& Parameters)
{
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Hud->Pump(0.0);
	TestFalse(TEXT("no hand, no pick"), W.Hud->IsCardPickUp());
	TestEqual(TEXT("nothing on the Menu stack"), W.Root->MenuStack->GetNumWidgets(), 0);
	const uint64 StepsBefore = W.Realm->GetSim().Field.StepCount;
	W.Realm->AdvanceFrame(FPawRealmSim::Step * 2.0);
	TestTrue(TEXT("the field runs with no hand up"), W.Realm->GetSim().Field.StepCount > StepsBefore);

	UPawCardPickScreen* Pick = W.Deal(*this, { "bounty", "close_quarters", "spite" }, 3);
	if (!Pick)
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("on the Menu stack"), W.Root->MenuStack->GetWidgetList().Contains(Pick));
	TestTrue(TEXT("the HUD knows it is up"), W.Hud->IsCardPickUp());
	TestEqual(TEXT("three cards drawn"), Pick->GetShownCardCount(), 3);
	W.Hud->Pump(0.0);
	W.Hud->Pump(0.0);
	TestEqual(TEXT("once, however often the HUD looks"), W.Root->MenuStack->GetNumWidgets(), 1);
	TestTrue(TEXT("the field is held by the pick"), W.Realm->IsPauseHeldBy(UPawCardPickScreen::PauseHolder) && W.Realm->IsPaused());
	const uint64 Held = W.Realm->GetSim().Field.StepCount;
	W.Realm->AdvanceFrame(1.0);
	TestEqual(TEXT("a held field does not step"), W.Realm->GetSim().Field.StepCount, Held);
	TestTrue(TEXT("the dim takes every click (no Pounce through the modal)"), Pick->GetDim() && Pick->GetDim()->GetVisibility() == ESlateVisibility::Visible);
	TestTrue(TEXT("a menu's input config"), Pick->GetDesiredInputConfig().IsSet() && Pick->GetDesiredInputConfig()->GetInputMode() == ECommonInputMode::Menu);

	TestEqual(TEXT("the title is the level"), S(Pick->GetTitleText()), FString(TEXT("LEVEL 3")));
	TestEqual(TEXT("the subtitle"), S(Pick->GetSubtitleText()), FString(TEXT("Take one. You are not owed a good hand.")));
	TestEqual(TEXT("three cards"), Pick->GetCardCount(), 3);
	TestNull(TEXT("and no fourth"), Pick->GetCard(3));
	TestEqual(TEXT("in deal order"), Pick->GetCardId(1), FString(TEXT("close_quarters")));
	TestTrue(TEXT("opens on the first card's Take"), Pick->GetCard(0) && Pick->GetDesiredFocusTarget() == Pick->GetCard(0)->GetTake());
	TestFalse(TEXT("no Reroll without rerolls"), Shown(Pick->GetRerollButton()));
	TestFalse(TEXT("no Banish without a banish"), Shown(Pick->GetCard(0)->GetBanish()));

	// Dilated: a bigger hand reuses the pooled cards and adds one.
	W.P().Hand = { "bounty", "close_quarters", "spite", "rigor" };
	Pick->ShowHand(false);
	TestEqual(TEXT("four cards"), Pick->GetCardCount(), 4);
	TestTrue(TEXT("the fourth is shown"), Shown(Pick->GetCard(3)));
	W.P().Hand = { "bounty", "spite", "rigor" };
	Pick->ShowHand(false);
	TestEqual(TEXT("back to three"), Pick->GetCardCount(), 3);
	TestNull(TEXT("the pooled fourth is not a card"), Pick->GetCard(3));
	TestEqual(TEXT("and is not drawn"), Pick->GetShownCardCount(), 3);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickSavedHandTest, "Pawpocalypse.UI.CardPick.SavedHand", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickSavedHandTest::RunTest(const FString& Parameters)
{
	// F24 (designer 2026-11-03): the run's cards and a hand on screen are saved; a load brings the SAME hand back up (Godot:
	// CardManager.reoffer_after_load), so reloading is never a free reroll.
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSaveSubsystem* Save = W.GI->GetSubsystem<UPawSaveSubsystem>();
	if (!TestNotNull(TEXT("the save subsystem"), Save))
	{
		W.Close();
		return false;
	}
	Save->UseStorageForTest(MakeShared<FPawMemorySaveStorage>());
	W.P().CardLevels["bounty"] = 2;
	W.P().CatchesPoints = 77.5;
	UPawCardPickScreen* Pick = W.Deal(*this, { "close_quarters", "spite", "bounty" }, 4);
	if (!Pick)
	{
		Save->UseStorageForTest(nullptr);
		W.Close();
		return false;
	}
	Save->SaveGame(0);
	Save->Flush();
	// The player takes a card (the pick closes), then reloads.
	Pick->Take(1);
	W.Hud->Pump(0.0);
	TestFalse(TEXT("taken: the pick is down"), W.Hud->IsCardPickUp());
	TestTrue(TEXT("the load succeeds"), Save->LoadGame(0) == EPawLoadOutcome::Loaded);
	TestEqual(TEXT("the card taken is not kept"), W.P().CardLevels.count("spite"), static_cast<size_t>(0));
	TestEqual(TEXT("the run's cards came back"), W.P().CardLevels["bounty"], static_cast<int64_t>(2));
	TestEqual(TEXT("the XP came back"), W.P().CatchesPoints, 77.5);
	W.Hud->Pump(0.0);
	UPawCardPickScreen* Again = W.Hud->GetCardPick();
	if (TestTrue(TEXT("the saved hand is up again"), Again && Again->IsActivated()))
	{
		TestEqual(TEXT("its level"), S(Again->GetTitleText()), FString(TEXT("LEVEL 4")));
		TestEqual(TEXT("three cards"), Again->GetCardCount(), 3);
		TestEqual(TEXT("the same first card"), Again->GetCardId(0), FString(TEXT("close_quarters")));
		TestEqual(TEXT("the same second card"), Again->GetCardId(1), FString(TEXT("spite")));
		TestEqual(TEXT("the same third card"), Again->GetCardId(2), FString(TEXT("bounty")));
	}
	Save->UseStorageForTest(nullptr);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickLookTest, "Pawpocalypse.UI.CardPick.Look", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickLookTest::RunTest(const FString& Parameters)
{
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().CardLevels["bounty"] = 1;      // held: taking it gives level 2
	W.P().PromisedCards = { "rigor", "spite" }; // Blueprint's promise for the next hand
	UPawCardPickScreen* Pick = W.Deal(*this, { "bounty", "close_quarters", "spite" });
	if (!Pick || !Pick->GetCard(2))
	{
		W.Close();
		return false;
	}
	UPawCardFace* Bounty = Pick->GetCard(0);
	UPawCardFace* Close = Pick->GetCard(1);
	UPawCardFace* Spite = Pick->GetCard(2);
	TestEqual(TEXT("the name"), S(Bounty->GetNameText()), FString(TEXT("Bounty")));
	TestEqual(TEXT("a held card: the level taking it gives"), S(Bounty->GetRankText()), FString(TEXT("LEVEL 2 of 3")));
	TestEqual(TEXT("muted"), Bounty->GetRankColour(), PawTokens::UI_TEXT_MUTED);
	TestEqual(TEXT("the body at that level"), S(Bounty->GetBodyText()), FString(TEXT("Signature enemies pay 3x Catnip.")));
	TestEqual(TEXT("a new card says so"), S(Close->GetRankText()), FString(TEXT("NEW")));
	TestEqual(TEXT("in the alert colour"), Close->GetRankColour(), PawTokens::UI_CARD_BANISH);
	TestEqual(TEXT("{v} and {c} at level one"), S(Close->GetBodyText()),
		FString(TEXT("Your aura detonates for 5 seconds of damage, but shrinks to 0.7x its radius.")));
	TestTrue(TEXT("a cost card wears the red frame"), Close->HasCostFrame());
	TestTrue(TEXT("and says so"), Close->IsWarningShown());
	TestFalse(TEXT("a free card has neither"), Bounty->HasCostFrame() || Bounty->IsWarningShown());
	TestEqual(TEXT("the flavour word"), S(Bounty->GetFlavourText()), FString(TEXT("SPOILS")));
	TestEqual(TEXT("the flavour band"), Bounty->GetBandColour(), PawTokens::UI_CARD_DECK);
	TestEqual(TEXT("an aura card's word"), S(Close->GetFlavourText()), FString(TEXT("AURA")));
	TestEqual(TEXT("an aura card's band"), Close->GetBandColour(), PawTokens::UI_CARD_AURA);
	TestEqual(TEXT("a deck card's word"), S(Spite->GetFlavourText()), FString(TEXT("THE DECK")));
	TestEqual(TEXT("a deck card's band"), Spite->GetBandColour(), PawTokens::GREY_HEADER);
	TestEqual(TEXT("Take"), S(Bounty->GetTake()->GetLabel()), FString(TEXT("Take")));
	TestEqual(TEXT("Blueprint's line under the subtitle"), S(Pick->GetSubtitleText()),
		FString(TEXT("Take one. You are not owed a good hand.\nNext hand will hold: Rigor, Spite")));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickGuardTest, "Pawpocalypse.UI.CardPick.Guard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickGuardTest::RunTest(const FString& Parameters)
{
	using G = UPawCardPickScreen;
	const double Q = PawUi::ModalGuardSec;
	// The rule alone: (dealt at, now, last was the pointer, the key press began at).
	TestFalse(TEXT("too soon, even for a fresh key"), G::GuardAllows(10.0, 10.0 + Q - 0.01, false, 10.1));
	TestFalse(TEXT("too soon for the mouse"), G::GuardAllows(10.0, 10.0 + Q - 0.01, true, 0.0));
	TestTrue(TEXT("the mouse once the guard is over"), G::GuardAllows(10.0, 10.0 + Q, true, 0.0));
	TestFalse(TEXT("a key held from before the deal"), G::GuardAllows(10.0, 12.0, false, 9.9));
	TestFalse(TEXT("the press that dealt this hand"), G::GuardAllows(10.0, 12.0, false, 10.0));
	TestTrue(TEXT("a key pressed after the deal"), G::GuardAllows(10.0, 10.0 + Q, false, 10.2));

	// The screen notes what it sees.
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawCardPickScreen* Pick = W.Deal(*this, { "bounty", "spite", "rigor" });
	if (!Pick)
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("closed as the hand is dealt"), Pick->IsGuardOpen());
	Pick->AdvanceClock(Q + 0.1);
	TestFalse(TEXT("the Space held to buy: no press began since"), Pick->IsGuardOpen());
	Pick->NoteKeyDown(true);
	TestFalse(TEXT("its repeats are the same press"), Pick->IsGuardOpen());
	Pick->ClickTake(0);
	TestTrue(TEXT("a refused click takes nothing"), Pick->IsActivated() && W.P().Hand.size() == 3 && Pc::CardLevel(W.P(), "bounty") == 0);
	Pick->AdvanceClock(0.01);
	Pick->NoteKeyDown(false);
	TestTrue(TEXT("a fresh press after the deal"), Pick->IsGuardOpen());
	Pick->NoteMouseDown();
	TestTrue(TEXT("the mouse after the guard"), Pick->IsGuardOpen());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickTakeTest, "Pawpocalypse.UI.CardPick.Take", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickTakeTest::RunTest(const FString& Parameters)
{
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawCardPickScreen* Pick = W.Deal(*this, { "bounty", "spite", "rigor" });
	if (!Pick)
	{
		W.Close();
		return false;
	}
	W.PastGuard(Pick);
	Pick->ClickTake(1);
	TestEqual(TEXT("the card is taken"), Pc::CardLevel(W.P(), "spite"), static_cast<int64_t>(1));
	TestEqual(TEXT("and only it"), Pc::CardLevel(W.P(), "bounty"), static_cast<int64_t>(0));
	TestEqual(TEXT("a deck card applies at once (Spite's rerolls)"), W.P().Rerolls, static_cast<int64_t>(2));
	TestTrue(TEXT("the hand is gone"), W.P().Hand.empty());
	TestFalse(TEXT("the pick closes"), Pick->IsActivated());
	TestFalse(TEXT("its hold is released"), W.Realm->IsPauseHeldBy(UPawCardPickScreen::PauseHolder));
	TestFalse(TEXT("the field runs again"), W.Realm->IsPaused());
	const uint64 Before = W.Realm->GetSim().Field.StepCount;
	W.Realm->AdvanceFrame(FPawRealmSim::Step * 2.0);
	TestTrue(TEXT("and steps"), W.Realm->GetSim().Field.StepCount > Before);
	W.Hud->Pump(0.0);
	TestFalse(TEXT("no hand, the HUD opens nothing"), W.Hud->IsCardPickUp());

	// The next hand reuses the pooled screen with nothing kept from the last one.
	UPawCardPickScreen* Again = W.Deal(*this, { "rigor", "bounty", "notoriety" }, 2);
	if (Again)
	{
		TestEqual(TEXT("the new hand"), Again->GetCardId(0), FString(TEXT("rigor")));
		TestEqual(TEXT("the new level"), S(Again->GetTitleText()), FString(TEXT("LEVEL 2")));
		TestTrue(TEXT("Spite's rerolls show"), Shown(Again->GetRerollButton()));
		TestFalse(TEXT("and the guard is closed again"), Again->IsGuardOpen());
		// A stale hand: the card on screen is no longer in it. Nothing is granted and the screen still closes.
		W.P().Hand = { "gold_rush", "scavenger", "giblets" };
		AddExpectedError(TEXT("was not in the offered hand"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("a stale card is refused"), Again->Take(0));
		TestEqual(TEXT("nothing granted"), Pc::CardLevel(W.P(), "rigor"), static_cast<int64_t>(0));
		TestFalse(TEXT("and the screen does not trap the player"), Again->IsActivated());
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickRerollTest, "Pawpocalypse.UI.CardPick.Reroll", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickRerollTest::RunTest(const FString& Parameters)
{
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Rerolls = 2;
	UPawCardPickScreen* Pick = W.Deal(*this, { "bounty", "spite", "rigor" });
	if (!Pick)
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("Reroll with rerolls"), Shown(Pick->GetRerollButton()));
	TestEqual(TEXT("its count"), S(Pick->GetRerollButton()->GetLabel()), FString(TEXT("Reroll  (2 left)")));
	Pick->ClickReroll();
	TestEqual(TEXT("guarded like the cards"), W.P().Rerolls, static_cast<int64_t>(2));
	W.PastGuard(Pick);
	Pick->ClickReroll();
	TestEqual(TEXT("one reroll spent"), W.P().Rerolls, static_cast<int64_t>(1));
	TestEqual(TEXT("a fresh hand, still up"), static_cast<int32>(W.P().Hand.size()), Pick->GetCardCount());
	TestTrue(TEXT("still up"), Pick->IsActivated() && Pick->GetCardCount() > 0);
	TestEqual(TEXT("the count follows"), S(Pick->GetRerollButton()->GetLabel()), FString(TEXT("Reroll  (1 left)")));
	TestFalse(TEXT("a re-deal closes the guard: the A that pressed Reroll takes nothing"), Pick->IsGuardOpen());
	Pick->AdvanceClock(PawUi::ModalGuardSec + 0.05);
	TestFalse(TEXT("not even once the time is up"), Pick->IsGuardOpen());
	for (int32 I = 0; I < Pick->GetCardCount(); ++I)
	{
		TestEqual(TEXT("the screen shows the hand dealt"), Pick->GetCardId(I), FString(UTF8_TO_TCHAR(W.P().Hand[static_cast<size_t>(I)].c_str())));
	}
	W.PastGuard(Pick);
	Pick->ClickReroll();
	TestEqual(TEXT("the last one"), W.P().Rerolls, static_cast<int64_t>(0));
	TestFalse(TEXT("none left: no button"), Shown(Pick->GetRerollButton()));
	TestFalse(TEXT("nothing to spend"), Pick->Reroll());
	TestTrue(TEXT("and the hand stays up"), Pick->IsActivated());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickBanishTest, "Pawpocalypse.UI.CardPick.Banish", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickBanishTest::RunTest(const FString& Parameters)
{
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Banishes = 1;
	UPawCardPickScreen* Pick = W.Deal(*this, { "bounty", "spite", "rigor" });
	if (!Pick || !Pick->GetCard(2))
	{
		W.Close();
		return false;
	}
	for (int32 I = 0; I < 3; ++I)
	{
		TestTrue(TEXT("every card has Banish while one is held"), Shown(Pick->GetCard(I)->GetBanish()));
	}
	TestEqual(TEXT("Banish"), S(Pick->GetCard(0)->GetBanish()->GetLabel()), FString(TEXT("Banish")));
	Pick->ClickBanish(0);
	TestEqual(TEXT("guarded"), W.P().Banishes, static_cast<int64_t>(1));
	W.PastGuard(Pick);
	Pick->ClickBanish(0);
	TestTrue(TEXT("the card is out of the run"), W.P().BanishedCards.count("bounty") == 1);
	TestEqual(TEXT("the banish is spent"), W.P().Banishes, static_cast<int64_t>(0));
	TestEqual(TEXT("the level is not: no card taken"), Pc::CardLevel(W.P(), "spite") + Pc::CardLevel(W.P(), "rigor"), static_cast<int64_t>(0));
	TestTrue(TEXT("a new hand is up"), Pick->IsActivated() && !W.P().Hand.empty());
	TestFalse(TEXT("without the banished card"), std::find(W.P().Hand.begin(), W.P().Hand.end(), "bounty") != W.P().Hand.end());
	TestFalse(TEXT("no banish left: no Banish"), Shown(Pick->GetCard(0)->GetBanish()));
	TestFalse(TEXT("the re-deal closes the guard"), Pick->IsGuardOpen());

	// Banished dry: every other card already out, so the re-deal comes back empty and the screen closes.
	W.P().Banishes = 1;
	W.P().Hand = { "rigor" };
	for (const Pc::PcCard& C : W.D().Cards)
	{
		if (C.Id != "rigor")
		{
			W.P().BanishedCards.insert(C.Id);
		}
	}
	Pick->ShowHand(true);
	W.PastGuard(Pick);
	Pick->ClickBanish(0);
	TestTrue(TEXT("the deck is empty"), W.P().Hand.empty());
	TestFalse(TEXT("the screen closes rather than show no cards and no exit"), Pick->IsActivated());
	TestFalse(TEXT("and lets the field go"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickNavigationTest, "Pawpocalypse.UI.CardPick.Navigation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickNavigationTest::RunTest(const FString& Parameters)
{
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Rerolls = 1;
	W.P().Banishes = 1;
	UPawCardPickScreen* Pick = W.Deal(*this, { "bounty", "spite", "rigor" });
	if (!Pick || !Pick->GetCard(2))
	{
		W.Close();
		return false;
	}
	UWidget* T0 = Pick->GetCard(0)->GetTake();
	UWidget* T1 = Pick->GetCard(1)->GetTake();
	UWidget* T2 = Pick->GetCard(2)->GetTake();
	UWidget* B0 = Pick->GetCard(0)->GetBanish();
	UWidget* B1 = Pick->GetCard(1)->GetBanish();
	UWidget* B2 = Pick->GetCard(2)->GetBanish();
	UWidget* R = Pick->GetRerollButton();
	TestEqual(TEXT("seven controls wired"), Pick->GetWired().Num(), 7);
	TestTrue(TEXT("right along the hand"), Nav(T0, EUINavigation::Right) == T1 && Nav(T1, EUINavigation::Right) == T2);
	TestTrue(TEXT("and round"), Nav(T2, EUINavigation::Right) == T0 && Nav(T0, EUINavigation::Left) == T2);
	TestTrue(TEXT("down to that card's Banish"), Nav(T0, EUINavigation::Down) == B0 && Nav(T2, EUINavigation::Down) == B2);
	TestTrue(TEXT("Banish up to its Take"), Nav(B1, EUINavigation::Up) == T1);
	TestTrue(TEXT("down from any Banish to Reroll"), Nav(B0, EUINavigation::Down) == R && Nav(B2, EUINavigation::Down) == R);
	TestTrue(TEXT("Reroll up to the middle card's Banish"), Nav(R, EUINavigation::Up) == B1);
	TestTrue(TEXT("and down round to the middle card"), Nav(R, EUINavigation::Down) == T1);
	TestTrue(TEXT("up from the hand round to Reroll"), Nav(T0, EUINavigation::Up) == R);
	TestTrue(TEXT("Reroll alone in its row"), Nav(R, EUINavigation::Left) == R && Nav(R, EUINavigation::Right) == R);

	// Without Banish and Reroll the hand is the whole screen: up and down stay on the card.
	W.P().Rerolls = 0;
	W.P().Banishes = 0;
	Pick->ShowHand(false);
	TestEqual(TEXT("three controls"), Pick->GetWired().Num(), 3);
	TestTrue(TEXT("down stays on the card"), Nav(T1, EUINavigation::Down) == T1 && Nav(T1, EUINavigation::Up) == T1);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPickPauseHoldsTest, "Pawpocalypse.UI.CardPick.PauseHolds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPickPauseHoldsTest::RunTest(const FString& Parameters)
{
	FPickWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawRealmSubsystem* R = W.Realm;
	R->SetPauseHold(TEXT("card_pick"), true);
	R->SetPauseHold(TEXT("tree"), true);
	R->SetPauseHold(TEXT("card_pick"), false);
	TestTrue(TEXT("one closing never resumes the field under another"), R->IsPaused());
	R->SetPauseHold(TEXT("tree"), false);
	TestFalse(TEXT("the last one lets it go"), R->IsPaused());

	// The click that takes a card is not a Pounce: swallowed while held, and until the button is let go.
	R->SetPounceDown(false);
	R->SetPauseHold(TEXT("card_pick"), true);
	R->SetPounceDown(true);
	TestFalse(TEXT("a press under a held field is swallowed"), R->GetSim().Input.bPounceDown);
	R->SetPauseHold(TEXT("card_pick"), false);
	R->SetPounceDown(true);
	TestFalse(TEXT("still held after the screen closes: still swallowed"), R->GetSim().Input.bPounceDown);
	R->SetPounceDown(false);
	R->SetPounceDown(true);
	TestTrue(TEXT("let go and pressed again: the player's own Pounce"), R->GetSim().Input.bPounceDown);
	R->SetPounceDown(false);

	// A fresh realm under a held pause starts held.
	R->SetPauseHold(TEXT("card_pick"), true);
	R->StartRealm(78);
	TestTrue(TEXT("a restarted field stays held"), R->IsPaused());
	R->SetPauseHold(TEXT("card_pick"), false);
	W.Close();
	return true;
}

#endif
