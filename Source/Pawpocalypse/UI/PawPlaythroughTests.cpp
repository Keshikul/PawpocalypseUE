// THE PLAYTHROUGH (docs/09 L3 `Paw.Playthrough`, docs/11 phase 3 exit; Godot ci/check_playthrough.gd): a whole run, played
// through the UI's entry points on the REAL root layout, HUD and realm: reset, earn by real kills, buy on the dock, travel on
// the Realms page, every boss baited and killed in order, the ending, the Reawaken with its ceremony, the spend step's nodes,
// the trip to the starting biome. Like Godot's, it asserts INVARIANTS (true at any tuning), never balance: pacing is pawsim's.
//
// Pawpocalypse.UI.Playthrough   one test, the run in order; each step names what must hold after it.
//
// Not here yet: save / load (phase 4), the pause menu (phase 4). The state a real run takes hours to reach (the Catnip gates,
// a boss's bait kills) is credited the way the cheat overlay credits it; every ACTION goes through the UI.
//
// Negative tests: tools/sabotage_realm.py (`playthrough_*`).

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UI/PawWidgets.h"
#include "PcCheats.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "PcTree.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawHudScreen.h"
#include "UI/PawJobPicker.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawVictoryScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include <cmath>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FPlayWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawRootLayout* Root = nullptr;
		UPawHudScreen* Hud = nullptr;
		TSharedPtr<SWidget> RootSlate;
		int32 CardsTaken = 0;

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
			if (!T.TestTrue(TEXT("the realm starts"), Realm->StartRealm(2026)))
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
			if (!Hud->GetAskReawakenButton()->IsTicked())
			{
				Hud->ToggleAskBeforeReawaken();
			}
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
		Pc::PcField& F() { return Realm->GetSim().Field; }
		double Dps() { return Pc::PlayerDps(D().Combat, Pc::RebuildStats(D(), P())); }
		int64 UpgradeLevels()
		{
			int64 N = 0;
			for (const auto& KV : P().UpgradeLevels)
			{
				N += KV.second;
			}
			return N;
		}

		// One frame as a player plays it: the cat on the nearest mouse, the field, the HUD; a hand that comes up is taken
		// through the card pick (its first card), as a player would.
		void Frame()
		{
			Pc::PcField& Fd = F();
			double Best = 1e300;
			for (size_t I = 0; I < Fd.Active.size(); ++I)
			{
				if (Fd.Active[I])
				{
					const double Dx = Fd.X[I] - Fd.CatX, Dy = Fd.Y[I] - Fd.CatY;
					if (Dx * Dx + Dy * Dy < Best)
					{
						Best = Dx * Dx + Dy * Dy;
						Realm->GetSim().Input.CatX = Fd.X[I];
						Realm->GetSim().Input.CatY = Fd.Y[I];
					}
				}
			}
			Realm->AdvanceFrame(FPawRealmSim::Step);
			Hud->Pump(FPawRealmSim::Step);
			if (Hud->IsCardPickUp() && Hud->GetCardPick()->Take(0))
			{
				++CardsTaken;
				Hud->Pump(FPawRealmSim::Step);
			}
		}

		// Hunts until the condition holds or the seconds run out.
		template <typename FCond>
		bool Hunt(double Seconds, FCond Cond)
		{
			for (int32 I = 0; I < static_cast<int32>(Seconds / FPawRealmSim::Step); ++I)
			{
				if (Cond())
				{
					return true;
				}
				Frame();
			}
			return Cond();
		}

		// The first dock row that can be bought now (unlocked, shown), or INDEX_NONE.
		int32 AffordableRow()
		{
			Hud->RefreshNow();
			for (int32 I = 0; Hud->GetRowButton(I); ++I)
			{
				const UPawButton* B = Hud->GetRowButton(I);
				if (B->GetVisibility() != ESlateVisibility::Collapsed && !B->IsLocked())
				{
					return I;
				}
			}
			return INDEX_NONE;
		}

		bool Finite()
		{
			const Pc::PcProgress& Pr = P();
			bool bOk = std::isfinite(Pr.Catnip) && std::isfinite(Pr.Stardust) && std::isfinite(Pr.LifetimeCatnipEver)
				&& std::isfinite(Pr.LifetimeCatnipThisCycle) && std::isfinite(Pr.StardustGrantedEver) && std::isfinite(Dps());
			const Pc::PcField& Fd = F();
			for (size_t I = 0; I < Fd.Active.size(); ++I)
			{
				bOk = bOk && (!Fd.Active[I] || (std::isfinite(Fd.X[I]) && std::isfinite(Fd.Y[I]) && std::isfinite(Fd.Hp[I])));
			}
			return bOk && std::isfinite(Fd.CatX) && std::isfinite(Fd.CatY);
		}

		// The gates a run takes hours to open, credited as the cheat overlay credits them: this cycle's Catnip past the
		// deepest biome's (the passage behind each boss is NOT credited: the kills open it).
		void CreditCatnipGates()
		{
			double Deepest = 0.0;
			for (const Pc::PcTier& Tier : D().Tiers)
			{
				Deepest = FMath::Max(Deepest, Tier.CatnipToUnlock);
			}
			P().LifetimeCatnipThisCycle = FMath::Max(P().LifetimeCatnipThisCycle, Deepest * 2.0);
			P().LifetimeCatnipEver = FMath::Max(P().LifetimeCatnipEver, P().LifetimeCatnipThisCycle);
			Hud->RefreshNow();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPlaythroughTest, "Pawpocalypse.UI.Playthrough", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPlaythroughTest::RunTest(const FString& Parameters)
{
	FPlayWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();

	// --- a clean slate ----------------------------------------------------------------------------------------------------
	TestEqual(TEXT("clean: no Catnip"), P.Catnip, 0.0);
	TestEqual(TEXT("clean: no Stardust"), P.Stardust, 0.0);
	TestEqual(TEXT("clean: never reawakened"), P.ReawakenCount, int64_t(0));
	TestEqual(TEXT("clean: no upgrade levels"), W.UpgradeLevels(), int64(0));
	TestTrue(TEXT("clean: no tree levels"), P.BranchLevels.empty());
	TestTrue(TEXT("clean: no bosses"), P.BossesDefeated.empty());
	TestEqual(TEXT("clean: the first biome"), W.F().TierIndex, int64_t(0));
	TestEqual(TEXT("clean: evolution stage 1"), Pc::EvolutionStage(D, P), int64_t(1));

	// --- earn: real kills -------------------------------------------------------------------------------------------------
	W.Hud->SelectTab(Pc::PcDockTab::Upgrades);
	const bool bEarned = W.Hunt(240.0, [&W]() { return W.AffordableRow() != INDEX_NONE; });
	TestTrue(TEXT("earn: the cat's kills pay for a first upgrade within four minutes"), bEarned);
	TestTrue(TEXT("earn: mice died"), P.LifetimeKills > 0);
	TestTrue(TEXT("earn: lifetime Catnip covers the bank"), P.LifetimeCatnipEver + 1e-6 >= P.Catnip && P.Catnip > 0.0);

	// --- buy on the dock: never what cannot be afforded; power never goes backwards -----------------------------------------
	{
		const double Banked = P.Catnip;
		const int64 Levels = W.UpgradeLevels();
		P.Catnip = 0.0;
		W.Hud->RefreshNow();
		for (int32 I = 0; W.Hud->GetRowButton(I); ++I)
		{
			W.Hud->PressRow(I);
		}
		TestEqual(TEXT("buy: nothing bought with nothing banked"), W.UpgradeLevels(), Levels);
		TestEqual(TEXT("buy: and nothing spent"), P.Catnip, 0.0);
		P.Catnip = Banked;
	}
	int32 Bought = 0;
	double DpsWas = W.Dps();
	for (int32 Round = 0; Round < 40 && Bought < 8; ++Round)
	{
		const int32 Row = W.AffordableRow();
		if (Row == INDEX_NONE)
		{
			W.Hunt(30.0, [&W]() { return W.AffordableRow() != INDEX_NONE; });
			continue;
		}
		const double Before = P.Catnip;
		const int64 Levels = W.UpgradeLevels();
		W.Hud->PressRow(Row);
		TestEqual(TEXT("buy: one press, one level"), W.UpgradeLevels(), Levels + 1);
		TestTrue(TEXT("buy: it cost something"), P.Catnip < Before);
		TestTrue(TEXT("buy: never below zero"), P.Catnip >= 0.0);
		const double Now = W.Dps();
		TestTrue(TEXT("buy: power never goes backwards"), Now + 1e-9 >= DpsWas);
		DpsWas = Now;
		++Bought;
	}
	TestTrue(TEXT("buy: a run buys several upgrades"), Bought >= 3);

	// --- travel, and every boss in order -----------------------------------------------------------------------------------
	W.Hud->SelectTab(Pc::PcDockTab::Realms);
	W.CreditCatnipGates();
	W.Hud->PressRealm(1);
	TestEqual(TEXT("travel: the Realms page travels"), W.F().TierIndex, int64_t(1));
	const std::string Last = D.Code.BossRealms.back();
	int64_t Stage = Pc::EvolutionStage(D, P);
	for (const std::string& BossRealm : D.Code.BossRealms)
	{
		const int32 Index = static_cast<int32>(Pc::RealmIndex(D, BossRealm));
		W.Hud->PressRealm(Index);
		if (!TestEqual(*FString::Printf(TEXT("travel to %hs"), BossRealm.c_str()), W.F().TierIndex, static_cast<int64_t>(Index)))
		{
			break;
		}
		const int32 Behind = Index + 1;
		if (Behind < static_cast<int32>(D.Tiers.size()))
		{
			W.Hud->PressRealm(Behind);
			TestEqual(TEXT("bosses: the biome behind a standing boss stays shut"), W.F().TierIndex, static_cast<int64_t>(Index));
		}
		const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(Index)].BossData);
		if (!TestNotNull(TEXT("the boss"), Boss))
		{
			break;
		}
		// Its bait: the kills a real fight takes hours to bank, and the Catnip it costs.
		P.TierKillCount += Pc::BaitShortfall(D, P, *Boss) * Boss->KillsPerBait;
		P.Catnip += Pc::SummonCatnipCost(D, P, *Boss);
		W.Hud->RefreshNow();
		W.Hud->PressBait();
		if (!TestTrue(*FString::Printf(TEXT("bosses: the Bait summons %hs's boss"), BossRealm.c_str()), W.F().bBossUp))
		{
			break;
		}
		W.Frame(); // the fight is on the field: the blows below land between its steps, as a swing does
		for (int32 I = 0; I < 8 && W.F().bBossUp; ++I)
		{
			W.F().bBossShielded = false;
			W.F().BossInvulnTimer = 0.0;
			Pc::DamageFieldBoss(W.F(), D, P, W.Realm->GetSim().Input, 1e300, 1.0);
			W.Frame();
		}
		TestFalse(TEXT("bosses: down"), W.F().bBossUp);
		TestTrue(TEXT("bosses: on the ledger"), Pc::IsBossDefeated(P, BossRealm));
		const int64_t NewStage = Pc::EvolutionStage(D, P);
		// Each clear is one evolution stage; the last is two (game_state.gd: stage 7 is the ENDING's, final_boss_defeated).
		TestEqual(TEXT("bosses: each clear is one evolution stage, the ending one more"), NewStage, Stage + (BossRealm == Last ? 2 : 1));
		Stage = NewStage;
		const bool bVictory = W.Hud->GetVictory() && W.Hud->GetVictory()->IsActivated();
		TestEqual(TEXT("the ending comes with the last boss and no other"), bVictory, BossRealm == Last);
		if (bVictory)
		{
			TestTrue(TEXT("the ending holds the world"), W.Realm->IsPaused());
			TestTrue(TEXT("the run's time is on the clock"), P.FinalBossSeconds > 0.0 && P.bFinalBossDefeated);
			W.Hud->GetVictory()->GetOkButton()->OnClicked().Broadcast();
			TestFalse(TEXT("OK gives the game back"), W.Realm->IsPaused());
		}
		if (Behind < static_cast<int32>(D.Tiers.size()))
		{
			W.Hud->PressRealm(Behind);
			TestEqual(TEXT("bosses: the kill opens the way on"), W.F().TierIndex, static_cast<int64_t>(Behind));
		}
		TestTrue(TEXT("bosses: everything finite"), W.Finite());
	}
	TestEqual(TEXT("every boss beaten"), static_cast<int32>(P.BossesDefeated.size()), static_cast<int32>(D.Code.BossRealms.size()));

	// --- the Reawaken: paid, the run cleared, power kept --------------------------------------------------------------------
	W.Hunt(5.0, []() { return false; }); // a few seconds on in the last biome
	W.Hud->SelectTab(Pc::PcDockTab::Reawaken);
	W.Hud->RefreshNow();
	const double Pending = Pc::PendingStardust(D, P);
	const double StardustWas = P.Stardust;
	const double MultiplierWas = Pc::ReawakenMultiplier(D.Reawaken, P.StardustGrantedEver);
	const double FinalSeconds = P.FinalBossSeconds;
	TestTrue(TEXT("reawaken: something to be paid"), Pending > 0.0);
	W.Hud->PressReawaken();
	TestTrue(TEXT("reawaken: the first press only arms it"), W.Hud->IsReawakenArmed() && P.ReawakenCount == 0);
	W.Hud->PressReawaken();
	TestEqual(TEXT("reawaken: the second press reawakens"), P.ReawakenCount, int64_t(1));
	TestEqual(TEXT("reawaken: paid what the page promised"), P.Stardust, StardustWas + Pending);
	TestTrue(TEXT("reawaken: the damage multiplier rose"), Pc::ReawakenMultiplier(D.Reawaken, P.StardustGrantedEver) > MultiplierWas);
	TestEqual(TEXT("reawaken: the upgrades reset"), W.UpgradeLevels(), Pc::StartingUpgradeLevel(D, P) * static_cast<int64>(P.UpgradeLevels.size()));
	TestEqual(TEXT("reawaken: this cycle's Catnip reset"), P.LifetimeCatnipThisCycle, 0.0);
	TestTrue(TEXT("reawaken: the passage re-locks"), P.BossesClearedThisCycle.empty());
	TestEqual(TEXT("reawaken: the power stays (every boss still beaten)"), static_cast<int32>(P.BossesDefeated.size()), static_cast<int32>(D.Code.BossRealms.size()));
	TestEqual(TEXT("reawaken: the run's time stands"), P.FinalBossSeconds, FinalSeconds);
	TestTrue(TEXT("reawaken: the ceremony plays"), W.Hud->GetCeremony()->IsPlaying());

	// --- the spend step: nodes bought for exactly their price ------------------------------------------------------------
	UPawSkillTreeScreen* Tree = nullptr;
	W.Hunt(10.0, [&W, &Tree]()
	{
		Tree = W.Hud->GetSkillTree();
		return Tree && Tree->IsActivated() && Tree->IsSpendMode();
	});
	if (!TestTrue(TEXT("spend: the tree opens after the ceremony"), Tree && Tree->IsActivated() && Tree->IsSpendMode()))
	{
		W.Close();
		return false;
	}
	int32 Nodes = 0;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		for (const Pc::PcSkillBranch& B : D.Branches)
		{
			const double Cost = Pc::NextBranchCost(D, P, B.Id);
			const int64_t Level = Pc::BranchLevel(P, B.Id);
			const double Before = P.Stardust;
			if (Tree->PressNode(UTF8_TO_TCHAR(B.Id.c_str())))
			{
				++Nodes;
				TestEqual(TEXT("spend: one level"), Pc::BranchLevel(P, B.Id), Level + 1);
				TestTrue(TEXT("spend: exactly its price"), FMath::IsNearlyEqual(P.Stardust, Before - Cost, 1e-6 * FMath::Max(1.0, Before)));
				TestTrue(TEXT("spend: never below zero"), P.Stardust >= 0.0);
			}
			else
			{
				TestEqual(TEXT("spend: a refused press spends nothing"), P.Stardust, Before);
			}
		}
	}
	TestTrue(TEXT("spend: the payout buys nodes"), Nodes > 0);
	const TMap<FString, int64> Owned = [&P]()
	{
		TMap<FString, int64> M;
		for (const auto& KV : P.BranchLevels)
		{
			M.Add(UTF8_TO_TCHAR(KV.first.c_str()), KV.second);
		}
		return M;
	}();

	// --- out: the job (when one is held), then the starting biome -------------------------------------------------------
	const int64 Where = Pc::ReawakenTravelIndex(D, P);
	Tree->PressClose();
	Tree->PressClose();
	if (UPawJobPicker* Picker = W.Hud->GetJobPicker(); Picker && Picker->IsActivated())
	{
		Picker->Pick(Picker->GetRowCount() - 1); // the baseline
		if (Picker->IsActivated())
		{
			Picker->Pick(Picker->GetRowCount() - 1);
		}
	}
	TestFalse(TEXT("out: no screen left up"), W.Hud->IsMenuUp());
	TestEqual(TEXT("out: the run starts in its starting biome"), W.F().TierIndex, static_cast<int64_t>(Where));
	TestFalse(TEXT("out: the world runs"), W.Realm->IsPaused());
	for (const auto& KV : Owned)
	{
		TestEqual(TEXT("out: the tree never resets"), Pc::BranchLevel(P, TCHAR_TO_UTF8(*KV.Key)), KV.Value);
	}
	// PASSAGE RE-LOCKS: the rule (is_biome_unlocked) shuts the biome behind the first boss again, even with this cycle's
	// Catnip gates credited; only its kill opens it.
	W.Hud->SelectTab(Pc::PcDockTab::Realms);
	W.CreditCatnipGates();
	const int32 FirstBoss = static_cast<int32>(Pc::RealmIndex(D, D.Code.BossRealms.front()));
	const Pc::PcTier& Behind1 = D.Tiers[static_cast<size_t>(FirstBoss + 1)];
	TestFalse(TEXT("out: the passage behind the first boss re-locked"), Pc::IsBiomeUnlocked(D, P, Behind1));
	// F22 (designer 2026-11-03, "that should not happen"): the Realms page asks the passage too, so a biome behind a boss
	// not beaten this run is locked there and its button goes nowhere (Godot fixed first: is_passage_open).
	const int64_t Here = W.F().TierIndex;
	const int32 Shut = Here != FirstBoss + 1 ? FirstBoss + 1 : FirstBoss + 2;
	TestTrue(TEXT("out: F22, the biome behind the first boss is locked on the page"),
		Pc::RealmRow(D, P, Shut).Kind == Pc::PcRealmRowKind::Locked);
	W.Hud->PressRealm(Shut);
	TestEqual(TEXT("out: F22, the Realms page does not walk past an unbeaten boss"), W.F().TierIndex, Here);
	// And the new run plays: kills pay again.
	const int64 KillsWas = P.LifetimeKills;
	W.Hunt(30.0, [&P, KillsWas]() { return P.LifetimeKills > KillsWas; });
	TestTrue(TEXT("again: the new run's kills pay"), P.LifetimeKills > KillsWas);
	TestTrue(TEXT("again: everything finite"), W.Finite());
	AddInfo(FString::Printf(TEXT("upgrades bought %d, cards taken %d, nodes bought %d, Stardust paid %.0f, kills %lld"), Bought,
		W.CardsTaken, Nodes, Pending, static_cast<long long>(P.LifetimeKills)));
	W.Close();
	return true;
}

#endif
