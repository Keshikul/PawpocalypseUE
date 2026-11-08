// The ending (docs/09 L3; docs/05 §6.3): the victory screen on the REAL root layout (its Slate built, so the stacks
// activate), raised by a real boss kill through the field's events. When it comes up is PcProgress RaisesVictory and the
// time format PcHud DurationParts, both pinned in PawCore against the real hud.tscn's VictoryScreen; these prove the screen.
//
// Pawpocalypse.UI.Victory.Ending     the last boss's first clear: the screen on the Modal stack, the field held, the
//                                    headline, the flavour and "Cleared in 1h 10m 00s" (the clock stopped at the kill),
//                                    OK focused; OK closes it and gives back exactly the pause it found.
// Pawpocalypse.UI.Victory.OnlyFinal  another boss's clear raises nothing; a last-boss rematch already cleared this cycle
//                                    raises nothing; the first rematch of a new cycle raises it with the FIRST clear's
//                                    time; a second show while one is up is ignored.
// Pawpocalypse.UI.Victory.Numbers    Godot's number printing: ints ungrouped, "%02d", "%.Nf" with a '.' point, and the
//                                    run time's two forms.
//
// Negative tests: tools/sabotage_realm.py (`victory_*`).

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AutomationTest.h"
#include "UI/PawWidgets.h"
#include "PcCheats.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcProgress.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawRootLayout.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiKit.h"
#include "UI/PawVictoryScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	FString S(const FText& T) { return T.ToString(); }

	struct FVictoryWorld
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
			Hud->SetModalStack(Root->ModalStack);
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

		// Travel to a boss realm, bait and summon its boss, kill it, and let one frame and the HUD see it.
		bool KillBoss(FAutomationTestBase& T, const std::string& TierId)
		{
			const Pc::PcGameData& Data = D();
			const int32 Index = static_cast<int32>(Pc::RealmIndex(Data, TierId));
			const Pc::PcBoss* Boss = Index >= 0 ? Data.FindBoss(Data.Tiers[static_cast<size_t>(Index)].BossData) : nullptr;
			if (!T.TestNotNull(TEXT("the boss"), Boss))
			{
				return false;
			}
			// Every biome open (the cheat marks every boss beaten), but this boss's own ledger as the test left it.
			const bool bBeaten = P().BossesDefeated.count(TierId) > 0;
			Pc::CheatUnlockAllTiers(Data, P());
			if (!bBeaten)
			{
				P().BossesDefeated.erase(TierId);
			}
			if (Realm->GetSim().Field.TierIndex != Index && !T.TestTrue(TEXT("travel to the boss biome"), Realm->Travel(Index)))
			{
				return false;
			}
			P().TierKillCount += Pc::BaitShortfall(Data, P(), *Boss) * Boss->KillsPerBait;
			P().Catnip = 1e300;
			if (!T.TestTrue(TEXT("summoned"), Realm->SummonBoss()))
			{
				return false;
			}
			Realm->AdvanceFrame(FPawRealmSim::Step);
			Hud->Pump(1.0 / 60.0);
			Pc::PcField& F = Realm->GetSim().Field;
			for (int32 I = 0; I < 4 && F.bBossUp; ++I)
			{
				F.bBossShielded = false;
				F.BossInvulnTimer = 0.0;
				Pc::DamageFieldBoss(F, Data, P(), Realm->GetSim().Input, 1e300, 1.0);
			}
			Realm->AdvanceFrame(FPawRealmSim::Step);
			Hud->Pump(1.0 / 60.0);
			return T.TestFalse(TEXT("the boss is down"), F.bBossUp);
		}

		bool VictoryUp() const
		{
			const UPawVictoryScreen* V = Hud->GetVictory();
			return V && V->IsActivated();
		}

		int32 VictoriesOnStack() const
		{
			int32 N = 0;
			for (const UCommonActivatableWidget* W : Root->ModalStack->GetWidgetList())
			{
				N += Cast<UPawVictoryScreen>(W) ? 1 : 0;
			}
			return N;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawVictoryEndingTest, "Pawpocalypse.UI.Victory.Ending", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawVictoryEndingTest::RunTest(const FString& Parameters)
{
	FVictoryWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const std::string Last = W.D().Code.BossRealms.back();
	W.P().TotalPlaySeconds = 4200.0;
	TestFalse(TEXT("no ending before the last boss falls"), W.VictoryUp());
	if (!W.KillBoss(*this, Last))
	{
		W.Close();
		return false;
	}
	UPawVictoryScreen* V = W.Hud->GetVictory();
	TestTrue(TEXT("the last boss's first clear raises the ending"), W.VictoryUp());
	TestEqual(TEXT("one ending"), W.VictoriesOnStack(), 1);
	TestTrue(TEXT("on the Modal stack, on top"), V && W.Root->ModalStack->GetActiveWidget() == V);
	TestTrue(TEXT("the ending holds the world"), W.Realm->IsPaused());
	// The play clock ran on through the summon's frames; it stopped at the kill.
	TestTrue(TEXT("the clock stopped at the kill"), W.P().FinalBossSeconds >= 4200.0 && W.P().FinalBossSeconds < 4201.0
		&& W.P().FinalBossSeconds <= W.P().TotalPlaySeconds);
	if (V)
	{
		TestEqual(TEXT("headline"), S(V->GetHeadlineText()), FString(TEXT("THE VOID IS QUIET")));
		TestEqual(TEXT("flavour"), S(V->GetFlavourText()), FString(TEXT("You have killed the last thing that moved.\nThe cat is unimpressed.")));
		TestEqual(TEXT("the run time"), S(V->GetTimeText()), FString(TEXT("Cleared in 1h 10m 00s")));
		TestTrue(TEXT("OK takes the focus"), V->GetDesiredFocusTarget() == V->GetOkButton());
		TestEqual(TEXT("OK"), S(V->GetOkButton()->GetLabel()), FString(TEXT("OK")));
		// The game theme (F21, designer 2026-11-03): the theme's button, ink text, on the paper WindowPanel.
		TestTrue(TEXT("OK wears the game theme's button"), V->GetOkButton()->HasThemeLook());
		// R4g: the headline a Butter title (outlined in ink), as the plum boards draw titles.
		TestTrue(TEXT("a Butter headline"), V->GetHeadline()->GetColorAndOpacity().GetSpecifiedColor().Equals(PawTokens::UI_SELECTED)
			&& V->GetHeadline()->GetFont().OutlineSettings.OutlineSize == PawUi::ButterTitleOutline);
		// The theme's WindowPanel: ui_window.svg's nine-patch (phase 6 slice 5a; Pawpocalypse.UI.Chrome checks every brush).
		const FSlateBrush& Plate = V->GetPanel()->Background;
		TestTrue(TEXT("on the window plate"), Plate.DrawAs == ESlateBrushDrawType::Box && Plate.GetResourceObject() != nullptr
			&& Plate.GetResourceObject() == PawUi::Chrome(PawUi::EPawChrome::Window).GetResourceObject());
		TestEqual(TEXT("OK is wired to itself (a one-button screen)"), V->GetWired().Num(), 1);
		TestEqual(TEXT("the dim eats every click"), V->GetDim()->GetVisibility(), ESlateVisibility::Visible);
		// The field stays held while the screen is up.
		const uint64 Steps = W.Realm->GetSim().Field.StepCount;
		W.Realm->AdvanceFrame(FPawRealmSim::Step * 4.0);
		TestTrue(TEXT("nothing moves behind the ending"), W.Realm->GetSim().Field.StepCount == Steps);
		// OK.
		V->GetOkButton()->OnClicked().Broadcast();
		TestFalse(TEXT("OK dismisses the ending"), V->IsActivated());
		TestFalse(TEXT("OK gives the game back"), W.Realm->IsPaused());
		W.Realm->AdvanceFrame(FPawRealmSim::Step * 2.0);
		TestTrue(TEXT("the field runs again"), W.Realm->GetSim().Field.StepCount > Steps);
		W.Hud->Pump(1.0 / 60.0);
		TestFalse(TEXT("a frame later it is still gone"), W.VictoryUp());
	}

	// Again, with something else holding the world as well (Godot: the pause menu underneath): OK restores what it found.
	W.Hud->ShowVictory(95.0);
	V = W.Hud->GetVictory();
	TestTrue(TEXT("shown again"), W.VictoryUp());
	TestEqual(TEXT("a time under an hour"), V ? S(V->GetTimeText()) : FString(), FString(TEXT("Cleared in 1m 35s")));
	W.Realm->SetPauseHold(TEXT("test_menu"), true);
	if (V)
	{
		V->Dismiss();
	}
	TestTrue(TEXT("the pause it found stays"), W.Realm->IsPaused());
	W.Realm->SetPauseHold(TEXT("test_menu"), false);
	TestFalse(TEXT("and lifts with its own holder"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawVictoryOnlyFinalTest, "Pawpocalypse.UI.Victory.OnlyFinal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawVictoryOnlyFinalTest::RunTest(const FString& Parameters)
{
	FVictoryWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	const std::string Last = D.Code.BossRealms.back();
	if (!W.KillBoss(*this, D.Code.BossRealms.front()))
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("an ordinary boss raises no ending"), W.VictoryUp());

	// The last boss, beaten in an earlier cycle and already re-cleared in this one: boss_defeated is not emitted.
	W.P().BossesDefeated.insert(Last);
	W.P().BossesClearedThisCycle.insert(Last);
	W.P().FinalBossSeconds = 4200.0;
	W.P().TotalPlaySeconds = 99999.0;
	if (!W.KillBoss(*this, Last))
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("a rematch already cleared this cycle raises nothing"), W.VictoryUp());

	// A new cycle's first rematch: the ending again, with the time that counted.
	W.P().BossesClearedThisCycle.erase(Last);
	if (!W.KillBoss(*this, Last))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("the first rematch of a cycle raises the ending"), W.VictoryUp());
	TestEqual(TEXT("the first clear's time stands"), W.P().FinalBossSeconds, 4200.0);
	UPawVictoryScreen* V = W.Hud->GetVictory();
	TestEqual(TEXT("it shows the first clear's time"), V ? S(V->GetTimeText()) : FString(), FString(TEXT("Cleared in 1h 10m 00s")));
	// show_victory while up: `if visible: return`.
	W.Hud->ShowVictory(10.0);
	TestEqual(TEXT("never a second ending"), W.VictoriesOnStack(), 1);
	TestTrue(TEXT("the same screen"), W.Hud->GetVictory() == V);
	TestEqual(TEXT("its line unchanged"), V ? S(V->GetTimeText()) : FString(), FString(TEXT("Cleared in 1h 10m 00s")));
	if (V)
	{
		V->Dismiss();
		V->Dismiss(); // dismiss twice: `if not visible: return`
	}
	TestFalse(TEXT("dismissed"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawVictoryNumbersTest, "Pawpocalypse.UI.Victory.Numbers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawVictoryNumbersTest::RunTest(const FString& Parameters)
{
	// Godot prints an int with str(): never grouped ("+125000 Stardust"), in every language.
	TestEqual(TEXT("a count is ungrouped"), S(PawText::Count(125000)), FString(TEXT("125000")));
	TestEqual(TEXT("a negative count"), S(PawText::Count(-1234567)), FString(TEXT("-1234567")));
	TestEqual(TEXT("%02d"), S(PawText::TwoDigits(5)), FString(TEXT("05")));
	TestEqual(TEXT("%02d of three digits"), S(PawText::TwoDigits(123)), FString(TEXT("123")));
	TestEqual(TEXT("%.1f"), S(PawText::Fixed(87.26, 1)), FString(TEXT("87.3")));
	TestEqual(TEXT("%.2f, big and ungrouped"), S(PawText::Fixed(12345.5, 2)), FString(TEXT("12345.50")));
	// Windows Godot rounds a tie half up (msvcrt), where the CRT printf rounds it to even: "0.3", never "0.2".
	TestEqual(TEXT("%.1f rounds a tie as Godot does"), S(PawText::Fixed(0.25, 1)), FString(TEXT("0.3")));
	TestEqual(TEXT("a run time with hours"), S(PawText::Duration(8730.0)), FString(TEXT("2h 25m 30s")));
	TestEqual(TEXT("a run time without"), S(PawText::Duration(95.0)), FString(TEXT("1m 35s")));
	TestEqual(TEXT("hours past a thousand stay ungrouped"), S(PawText::Duration(3600.0 * 1500.0 + 61.0)), FString(TEXT("1500h 01m 01s")));
	// A culture with a comma decimal point and other grouping changes none of it.
	const FString Was = FInternationalization::Get().GetCurrentCulture()->GetName();
	FInternationalization::Get().SetCurrentCulture(TEXT("de"));
	TestEqual(TEXT("de: still ungrouped"), S(PawText::Count(125000)), FString(TEXT("125000")));
	TestEqual(TEXT("de: still a '.' point"), S(PawText::Fixed(1.5, 2)), FString(TEXT("1.50")));
	FInternationalization::Get().SetCurrentCulture(Was);
	return true;
}

#endif
