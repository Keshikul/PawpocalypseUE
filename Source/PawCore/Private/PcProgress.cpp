#include "PcProgress.h"

#include "PcEconomy.h"
#include "PcJobs.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Pc
{
	namespace
	{
		// The last boss, the one that ends the run. The validator refuses data without bosses, but a rule must not read
		// past an empty list to find that out.
		bool IsFinalBossRealm(const PcGameData& D, const std::string& TierId)
		{
			return !D.Code.BossRealms.empty() && TierId == D.Code.BossRealms.back();
		}

		// GameState.BRANCH_GATED_UPGRADES: shop lines that only exist once their branch is bought. "Not purchased" means
		// NOT SHOWN (designer, 2026-08-18): a row the player cannot act on is noise in a list they are meant to scan.
		const char* GatingBranch(const std::string& UpgradeId)
		{
			if (UpgradeId == "pounce_power" || UpgradeId == "pounce_cooldown")
			{
				return "unlock_pounce";
			}
			if (UpgradeId == "overkill")
			{
				return "unlock_overkill";
			}
			// The aura is bought in the tree (v5), so the line that widens it has nothing to widen until then.
			if (UpgradeId == "aura_radius")
			{
				return "unlock_aura";
			}
			return nullptr;
		}

		// Godot's is_zero_approx(x).
		bool IsZeroApprox(double X)
		{
			return std::fabs(X) < 0.00001;
		}
	}

	// --- the boss ledger ---------------------------------------------------------------------------------------------

	bool IsBossDefeated(const PcProgress& P, const std::string& TierId)
	{
		return P.BossesDefeated.count(TierId) > 0;
	}

	bool IsBossClearedThisCycle(const PcProgress& P, const std::string& TierId)
	{
		return P.BossesClearedThisCycle.count(TierId) > 0;
	}

	PcBossMark MarkBossDefeated(const PcGameData& D, PcProgress& P, const std::string& TierId)
	{
		PcBossMark Out;
		if (IsBossDefeated(P, TierId))
		{
			// Beaten in an EARLIER cycle: the record and the stage do not move, but this cycle's passage does. That is the
			// whole point of re-baiting; returning early would make every rematch after the first open nothing.
			Out.bDefeated = P.BossesClearedThisCycle.insert(TierId).second;
			return Out;
		}
		P.BossesDefeated.insert(TierId);
		P.BossesClearedThisCycle.insert(TierId);
		// Stop the clock the moment the last boss dies; only the FIRST clear is recorded (a rematch is not a faster run).
		if (IsFinalBossRealm(D, TierId) && IsZeroApprox(P.FinalBossSeconds))
		{
			P.FinalBossSeconds = P.TotalPlaySeconds;
		}
		Out.bNew = true;
		Out.bDefeated = true;
		return Out;
	}

	PcBossMark RecordBossDefeat(const PcGameData& D, PcProgress& P, const std::string& TierId, double RewardCatnip)
	{
		AddCurrency(D, P, RewardCatnip);
		const PcBossMark Out = MarkBossDefeated(D, P, TierId);
		// The game's one "the run is complete" flag: the ending's step on the evolution ladder.
		if (IsFinalBossRealm(D, TierId))
		{
			P.bFinalBossDefeated = true;
		}
		return Out;
	}

	bool RaisesVictory(const PcGameData& D, const std::string& TierId, const PcBossMark& Mark)
	{
		return Mark.bDefeated && IsFinalBossRealm(D, TierId);
	}

	bool IsUnicornOwned(const PcProgress& P)
	{
		return BranchLevel(P, "unlock_unicorn") > 0;
	}

	int64_t VisualStage(const PcGameData& D, const PcProgress& P)
	{
		int64_t Stage = EvolutionStage(D, P);
		if (IsUnicornOwned(P))
		{
			Stage = std::max(Stage, D.Code.MountedStage);
		}
		return std::min(Stage, D.Code.EvolutionStageCount);
	}

	// --- currency ----------------------------------------------------------------------------------------------------

	void CreditCurrency(PcProgress& P, double Amount)
	{
		P.Catnip += Amount;
		P.LifetimeCatnipThisCycle += Amount;
		P.LifetimeCatnipEver += Amount;
	}

	void AddCurrency(const PcGameData& D, PcProgress& P, double BaseReward)
	{
		CreditCurrency(P, KillCatnip(D, P, BaseReward));
	}

	void CreditUntouchedCatnip(PcProgress& P, double Amount)
	{
		// Floored, not whole_catnip: interest is not a kill and must not pay a free 1 to an empty bank.
		const double Whole = std::floor(Amount);
		if (Whole > 0.0)
		{
			CreditCurrency(P, Whole);
		}
	}

	void GrantAchievementStardust(PcProgress& P, double Amount)
	{
		if (Amount <= 0.0)
		{
			return;
		}
		P.Stardust += Amount;
		P.StardustGrantedEver += Amount;
	}

	void SpendStardust(PcProgress& P, double Amount)
	{
		P.Stardust -= Amount;
	}

	// --- kills and waves ---------------------------------------------------------------------------------------------

	PcKillStep IncrementKillCount(const PcGameData& D, PcProgress& P)
	{
		PcKillStep Out;
		P.TierKillCount += 1;
		P.LifetimeKills += 1;
		P.KillsThisWave += 1;
		P.KillsSinceTithe += 1;
		const int64_t Before10k = P.KillsThisCycle / 10000;
		P.KillsThisCycle += 1;
		// Only when the number Body Count reads changes: a stat pass per kill is a stat pass sixty times a second.
		Out.bStatsDirty = P.KillsThisCycle / 10000 != Before10k;
		const double PerKill = std::max(BranchEffect(D, P, "infestation"), 0.0);
		if (PerKill > 0.0)
		{
			P.InfestationBonus = std::min(P.InfestationBonus + PerKill, D.Code.InfestationCeiling);
		}
		const PcTier* Here = nullptr;
		const int64_t Index = RealmIndex(D, P.CurrentBiomeId);
		if (Index >= 0)
		{
			Here = &D.Tiers[static_cast<size_t>(Index)];
		}
		const int64_t W = Here ? CurrentWave(D, *Here, P.TierKillCount) : 0;
		if (W != P.CurrentWaveIndex)
		{
			P.CurrentWaveIndex = W;
			Out.InterestPaid = PayCatnipInterest(D, P);
			// Reputation counts waves survived HERE, beside the turnover: one definition of a wave passing.
			P.WavesHere += 1;
			Out.bStatsDirty = true;
			// The wave IS Fence's window; a second definition of when it closes would drift from this one.
			P.KillsThisWave = 0;
			Out.bWaveTurned = true;
		}
		Out.Wave = P.CurrentWaveIndex;
		return Out;
	}

	double PayCatnipInterest(const PcGameData& D, PcProgress& P)
	{
		const double Rate = std::max(BranchEffect(D, P, "compound"), 0.0);
		if (Rate <= 0.0 || P.Catnip <= 0.0)
		{
			return 0.0;
		}
		// _credit_currency, NOT add_currency: the kill multipliers already priced this money once.
		const double Paid = WholeCatnip(P.Catnip * Rate);
		CreditCurrency(P, Paid);
		return Paid;
	}

	// --- travel ------------------------------------------------------------------------------------------------------

	int64_t RealmIndex(const PcGameData& D, const std::string& RealmId)
	{
		for (size_t I = 0; I < D.Tiers.size(); ++I)
		{
			if (D.Tiers[I].Id == RealmId)
			{
				return static_cast<int64_t>(I);
			}
		}
		return -1;
	}

	void BankTierKills(PcProgress& P)
	{
		if (!P.CurrentBiomeId.empty())
		{
			P.TierKillsByBiome[P.CurrentBiomeId] = P.TierKillCount;
		}
	}

	PcArrival BeginBiome(const PcGameData& D, PcProgress& P, const std::string& RealmId)
	{
		PcArrival Out;
		if (RealmId == P.CurrentBiomeId)
		{
			return Out;
		}
		Out.bMoved = true;
		// Waves are a property of the place, and the player keeps each place's progress.
		BankTierKills(P);
		// Reputation is earned HERE and lost by leaving: the one line that makes the node what it is.
		P.WavesHere = 0;
		// FENCE's window closes on leaving (and on a Reawaken, whose arrival comes through here with the biome cleared):
		// F7, designer 2026-11-03.
		P.KillsThisWave = 0;
		P.CurrentBiomeId = RealmId;
		const auto Banked = P.TierKillsByBiome.find(RealmId);
		P.TierKillCount = Banked == P.TierKillsByBiome.end() ? 0 : Banked->second;
		const int64_t Arrived = RealmIndex(D, RealmId);
		P.CurrentWaveIndex = Arrived >= 0 ? CurrentWave(D, D.Tiers[static_cast<size_t>(Arrived)], P.TierKillCount) : 0;
		// Second Helping pays its other half on reaching a place, so the half is a destination rather than a timer.
		if (P.PendingSecondHelping > 0.0 && Arrived >= D.Code.SecondHelpingBiomeIndex)
		{
			const double Second = std::floor(P.PendingSecondHelping);
			P.PendingSecondHelping = 0.0;
			if (Second > 0.0)
			{
				P.Stardust += Second;
				P.StardustGrantedEver += Second;
				Out.SecondHelpingPaid = Second;
			}
		}
		if (Arrived > P.DeepestBiomeIndex)
		{
			P.DeepestBiomeIndex = Arrived;
			Out.bDeeper = true;
		}
		return Out;
	}

	// --- bait --------------------------------------------------------------------------------------------------------

	bool IsBaitUnlocked(const PcGameData& D, const PcProgress& P, const PcBoss& Boss)
	{
		return BaitShortfall(D, P, Boss) == 0;
	}

	bool CanAffordBait(const PcProgress& P, const PcBoss& Boss)
	{
		return Boss.BaitCostCatnip <= 0.0 || P.Catnip >= Boss.BaitCostCatnip;
	}

	bool BuyBait(const PcGameData& D, PcProgress& P, const PcBoss& Boss)
	{
		if (!IsBaitUnlocked(D, P, Boss))
		{
			return false;
		}
		// ALL-OR-NOTHING, and the order is load-bearing: the first version spent the bait and THEN checked the Catnip, so a
		// summon refused for one Catnip still ate a whole biome's grind.
		const double Cost = SummonCatnipCost(D, P, Boss);
		const bool bAudited = AuditCatnipMultiple(D, P) > 0.0 && Boss.BaitRequired > 0;
		if (Cost > 0.0 && P.Catnip < Cost)
		{
			return false;
		}
		if (Boss.BaitRequired > 0 && !bAudited)
		{
			P.BaitSpentByBiome[P.CurrentBiomeId] += Boss.BaitRequired;
		}
		if (Cost > 0.0)
		{
			P.Catnip -= Cost;
		}
		return true;
	}

	// --- the shop ----------------------------------------------------------------------------------------------------

	bool IsUpgradeVisible(const PcProgress& P, const std::string& Id)
	{
		const char* Branch = GatingBranch(Id);
		return !Branch || BranchLevel(P, Branch) > 0;
	}

	bool IsUpgradeAtCap(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const PcUpgrade* U = D.FindUpgrade(Id);
		if (!U || U->Cap < 0.0)
		{
			return false;
		}
		return EffectTotal(*U, UpgradeLevel(P, Id)) >= EffectiveCap(*U, P);
	}

	bool IsKittenPowerLocked(const PcGameData& D, const PcProgress& P)
	{
		return KittenCount(D, P) <= 0;
	}

	bool IsKittenSpeedLocked(const PcGameData& D, const PcProgress& P)
	{
		return KittenCount(D, P) <= 0;
	}

	double UpgradeCostById(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const PcUpgrade* U = D.FindUpgrade(Id);
		return U ? UpgradeCost(*U, UpgradeLevel(P, Id)) : std::numeric_limits<double>::infinity();
	}

	bool PurchaseUpgrade(const PcGameData& D, PcProgress& P, const std::string& Id)
	{
		const PcUpgrade* U = D.FindUpgrade(Id);
		if (!U || IsUpgradeAtCap(D, P, Id))
		{
			return false;
		}
		// Hiding a button is a UI fact; this is the rule. Auto-buyers, the cheat overlay and the simulator come through here.
		if (!IsUpgradeVisible(P, Id))
		{
			return false;
		}
		// Kitten Speed is locked the same way, in the rule and not only by the shop's hidden button (F8, 2026-11-03).
		if ((Id == "kitten_power" && IsKittenPowerLocked(D, P)) || (Id == "kitten_speed" && IsKittenSpeedLocked(D, P)))
		{
			return false;
		}
		const double Cost = UpgradeCostById(D, P, Id);
		if (P.Catnip + D.Code.CurrencyEpsilon < Cost)
		{
			return false;
		}
		// Floored: the epsilon can let a purchase through a billionth short, and a bank that goes negative is worse than one
		// that forgives a rounding error.
		P.Catnip = std::max(P.Catnip - Cost, 0.0);
		P.UpgradeLevels[Id] = UpgradeLevel(P, Id) + 1;
		return true;
	}

	int64_t PurchaseUpgradeBulk(const PcGameData& D, PcProgress& P, const std::string& Id, int64_t Count)
	{
		// purchase_upgrade() looped rather than a closed form: a bulk path carrying its own copy of the rules is a bulk path
		// that eventually disagrees with the single buy.
		const int64_t Wanted = Count < 0 ? D.Code.BulkMaxSteps : std::min(Count, D.Code.BulkMaxSteps);
		int64_t Bought = 0;
		while (Bought < Wanted && PurchaseUpgrade(D, P, Id))
		{
			++Bought;
		}
		return Bought;
	}

	PcBulkPreview PreviewBulkPurchase(const PcGameData& D, const PcProgress& P, const std::string& Id, int64_t Count)
	{
		PcBulkPreview Out;
		const PcUpgrade* U = D.FindUpgrade(Id);
		if (!U)
		{
			return Out;
		}
		// Costs rise per level, so N x the current price is wrong: walk the curve.
		const int64_t Wanted = Count < 0 ? D.Code.BulkMaxSteps : std::min(Count, D.Code.BulkMaxSteps);
		int64_t Level = UpgradeLevel(P, Id);
		double Budget = P.Catnip;
		while (Out.Levels < Wanted)
		{
			if (U->Cap >= 0.0 && EffectTotal(*U, Level) >= EffectiveCap(*U, P))
			{
				break;
			}
			const double Step = UpgradeCost(*U, Level);
			if (Step > Budget + D.Code.CurrencyEpsilon)
			{
				break;
			}
			Budget = std::max(Budget - Step, 0.0);
			Out.Cost += Step;
			++Level;
			++Out.Levels;
		}
		return Out;
	}

	// --- Reawaken ----------------------------------------------------------------------------------------------------

	PcReawakenResult Reawaken(const PcGameData& D, PcProgress& P, bool bForce)
	{
		PcReawakenResult Out;
		if (!bForce && !CanReawaken(D, P))
		{
			return Out;
		}
		double Gained = PendingStardust(D, P);
		// SECOND HELPING: half lands now, half waits for a destination. It changes the SHAPE of a run, not its size.
		if (BranchEffect(D, P, "second_helping") > 0.0)
		{
			const double Deferred = Gained * 0.5;
			Gained -= Deferred;
			P.PendingSecondHelping += Deferred;
		}
		P.Stardust += Gained;
		P.StardustGrantedEver += Gained;
		// Legacy describes how the next run STARTS: applied at this moment, not as a stat.
		P.Catnip = std::floor(P.Catnip * CatnipCarryover(D, P));
		const int64_t FloorLevel = StartingUpgradeLevel(D, P);
		for (const PcUpgrade& U : D.Upgrades)
		{
			// Hidden lines get nothing: Muscle Memory restoring levels of an unbought Pounce would hand over an ability
			// through the back door. No level clamp: the cap is on the EFFECT.
			P.UpgradeLevels[U.Id] = (FloorLevel <= 0 || !IsUpgradeVisible(P, U.Id)) ? 0 : FloorLevel;
		}
		P.LifetimeCatnipThisCycle = 0.0;
		P.PlaySecondsThisCycle = 0.0;
		P.DeepestBiomeIndex = 0;
		// Forget where the player stood: every biome behind tier 1 relocks, and clearing this is what makes the next
		// begin_biome() register instead of returning early.
		P.CurrentBiomeId.clear();
		// Carrying banked wave progress forward would hand the second pass a boss gate already paid.
		P.TierKillsByBiome.clear();
		P.BaitSpentByBiome.clear();
		P.BossesClearedThisCycle.clear();
		P.ReawakenCount += 1;
		// A NEW RUN ASKS AGAIN; the preference stays as the picker's default.
		ClearRunJobs(P);
		// The compounding counters are the run, so they go with it.
		P.KillsThisCycle = 0;
		P.CardsTakenThisRun = 0;
		P.WavesHere = 0;
		ResetRunCards(D, P);
		Out.bDone = true;
		Out.StardustGained = Gained;
		return Out;
	}

	void ResetRunCards(const PcGameData& D, PcProgress& P)
	{
		P.CardLevels.clear();
		P.BanishedCards.clear();
		P.CatchesLevel = 0;
		P.CatchesPoints = 0.0;
		// NOT zero: Overtime is a Stardust purchase and Stardust survives a Reawaken, so its rerolls must too.
		P.Rerolls = std::max<int64_t>(0, static_cast<int64_t>(BranchEffect(D, P, "overtime")));
		P.PromisedCards.clear();
		P.PrototypeCard.clear();
		P.ExtraChoices = 0;
		P.Banishes = 0;
		P.FixationHands = 0;
		P.Hand.clear();
	}

	// --- the clocks --------------------------------------------------------------------------------------------------

	void TickPlayClock(PcProgress& P, double Delta)
	{
		P.PlaySecondsThisCycle += Delta;
		P.TotalPlaySeconds += Delta;
	}

	double TickRealtimeInterest(const PcGameData& D, PcProgress& P, double Delta)
	{
		const double Rate = std::max(BranchEffect(D, P, "compound_interest"), 0.0);
		if (Rate <= 0.0)
		{
			P.RealtimeInterestElapsed = 0.0;
			return 0.0;
		}
		P.RealtimeInterestElapsed += Delta;
		if (P.RealtimeInterestElapsed < D.Code.RealtimeInterestPeriodSec)
		{
			return 0.0;
		}
		P.RealtimeInterestElapsed = 0.0;
		if (P.Catnip <= 0.0)
		{
			return 0.0;
		}
		// Through _credit_currency, as Compound: the kill multipliers already priced this money.
		const double Paid = WholeCatnip(P.Catnip * Rate);
		CreditCurrency(P, Paid);
		return Paid;
	}

	void TickInfestation(const PcGameData& D, PcProgress& P, double Delta)
	{
		if (P.InfestationBonus <= 0.0)
		{
			return;
		}
		P.InfestationBonus = std::max(0.0, P.InfestationBonus - D.Code.InfestationDecayPerSec * Delta);
	}

	// --- a new game --------------------------------------------------------------------------------------------------

	void NewGame(const PcGameData& D, PcProgress& P)
	{
		// EVERYTHING A SAVE CARRIES (designer, 2026-10-01: a new game in an emptied slot offered two jobs never bought,
		// inherited from the game that was in memory). Tutorial cues and Long Lunch's stillness live outside PawCore.
		P.Catnip = 0.0;
		P.Stardust = 0.0;
		P.ReawakenCount = 0;
		P.TierKillCount = 0;
		P.TierKillsByBiome.clear();
		P.BaitSpentByBiome.clear();
		P.BossesClearedThisCycle.clear();
		P.CurrentBiomeId.clear();
		P.DeepestBiomeIndex = 0;
		P.LifetimeCatnipThisCycle = 0.0;
		P.PlaySecondsThisCycle = 0.0;
		P.LifetimeCatnipEver = 0.0;
		P.StardustGrantedEver = 0.0;
		P.BossesDefeated.clear();
		P.bFinalBossDefeated = false;
		P.JobsHeld.clear();
		P.CurrentJob.clear();
		ClearRunJobs(P);
		P.LifetimeKills = 0;
		P.TotalPlaySeconds = 0.0;
		P.FinalBossSeconds = 0.0;
		P.KillsThisCycle = 0;
		P.KillsThisWave = 0;
		P.KillsSinceTithe = 0;
		P.CurrentWaveIndex = 0;
		P.WavesHere = 0;
		P.CardsTakenThisRun = 0;
		P.InfestationBonus = 0.0;
		P.PendingSecondHelping = 0.0;
		P.bReawakenInProgress = false;
		P.RealtimeInterestElapsed = 0.0;
		P.PlayerStillSeconds = 0.0;
		P.LunchSeconds = 0.0;
		for (const PcUpgrade& U : D.Upgrades)
		{
			P.UpgradeLevels[U.Id] = 0;
		}
		// SkillTreeManager.reset_purchases(): the one time the tree resets.
		for (auto& [Id, Level] : P.BranchLevels)
		{
			Level = 0;
		}
		ResetRunCards(D, P);
	}
}
