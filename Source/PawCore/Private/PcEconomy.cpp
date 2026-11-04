#include "PcEconomy.h"

#include "PcJobs.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		// realm_data(): the biome by id, or null.
		const PcTier* FindTier(const PcGameData& D, const std::string& Id)
		{
			if (Id.empty())
			{
				return nullptr;
			}
			for (const PcTier& T : D.Tiers)
			{
				if (T.Id == Id)
				{
					return &T;
				}
			}
			return nullptr;
		}

		// Godot's is_equal_approx(a, b): exact, or within CMP_EPSILON (1e-5) relative, at least 1e-5 absolute.
		bool IsEqualApprox(double A, double B)
		{
			if (A == B)
			{
				return true;
			}
			double Tolerance = 0.00001 * std::fabs(A);
			if (Tolerance < 0.00001)
			{
				Tolerance = 0.00001;
			}
			return std::fabs(A - B) < Tolerance;
		}

		int64_t ClampWave(const PcGameData& D, double Wave)
		{
			// int() truncates toward zero, then clampi(.., 0, MAX_WAVE).
			const int64_t W = static_cast<int64_t>(Wave);
			return std::clamp<int64_t>(W, 0, D.Code.MaxWave);
		}
	}

	// --- what a kill pays --------------------------------------------------------------------------------------------

	double WholeCatnip(double Amount)
	{
		// A Tier 1 common pays 0.33 and the HUD shows whole numbers: the first kills read 0, 0, 0, 1 while the kittens
		// made the number climb on their own. Rounding to the nearest whole number would make a kill pay nothing; the
		// floor of 1 guarantees every kill is felt (designer, 2026-08-17). roundf rounds halves away from zero.
		if (Amount <= 0.0)
		{
			return 0.0;
		}
		return std::max(1.0, std::round(Amount));
	}

	double UntouchedCatnip(double Amount)
	{
		// Interest is NOT a kill: a floor of 1 would pay a free +1 to a player with an empty bank.
		const double Whole = std::floor(Amount);
		return Whole > 0.0 ? Whole : 0.0;
	}

	double BiomeRewardMultiplier(const PcGameData& D, const PcProgress& P)
	{
		// Reward and HP both climb per tier, so before this every biome paid the same Catnip per second. Read from the
		// biome the player stands in: deep biomes must not hand out a permanent global multiplier.
		const PcTier* T = FindTier(D, P.CurrentBiomeId);
		if (!T || T->RewardMultiplier <= 0.0)
		{
			return 1.0;
		}
		return T->RewardMultiplier;
	}

	double FortuneYieldMultiplier(const PcGameData& D, const PcProgress& P)
	{
		// Guarded on level > 0 like every branch: an unowned branch must not apply a bonus nobody bought.
		if (BranchLevel(P, "fortune") <= 0)
		{
			return 1.0;
		}
		const double Effect = BranchEffect(D, P, "fortune");
		return Effect > 0.0 ? Effect : 1.0;
	}

	double GoldenChanceBonus(const PcGameData& D, const PcProgress& P)
	{
		if (BranchLevel(P, "fortune") <= 0)
		{
			return 0.0;
		}
		return std::max(BranchEffect(D, P, "fortune", "golden"), 0.0);
	}

	double GoldenChanceFor(const PcGameData& D, const PcProgress& P, const PcTier& Tier)
	{
		// ADDED to the biome's chance, not multiplied: a biome with no goldens stays that way, and the addition says so.
		if (Tier.GoldenEnemy.empty() || Tier.GoldenChance <= 0.0)
		{
			return 0.0;
		}
		return std::min(Tier.GoldenChance + GoldenChanceBonus(D, P), D.Code.MaxGoldenChance);
	}

	double JobCatnipMultiplier(const PcGameData& D, const PcProgress& P)
	{
		return JobTerm(D, P, &PcJob::CatnipMultiplier);
	}

	double FenceMultiplier(const PcGameData& D, const PcProgress& P)
	{
		const double Per = BranchEffect(D, P, "fence");
		if (Per <= 0.0)
		{
			return 1.0;
		}
		return 1.0 + Per * static_cast<double>(std::min(P.KillsThisWave, D.Code.FenceMaxKills));
	}

	double KillCatnip(const PcGameData& D, const PcProgress& P, double BaseReward)
	{
		// The order of the product is Godot's, so even the rounding matches.
		return WholeCatnip(BaseReward
			* CurrencyFindMultiplier(D, P)
			* BiomeRewardMultiplier(D, P)
			* FortuneYieldMultiplier(D, P)
			* JobCatnipMultiplier(D, P)
			* FenceMultiplier(D, P));
	}

	// --- waves -------------------------------------------------------------------------------------------------------

	int64_t CurrentWave(const PcGameData& D, const PcTier& Tier, int64_t TierKillCount)
	{
		// Wave N costs kills_per_wave x g^N kills, so reaching wave W takes kills_per_wave x (g^W - 1) / (g - 1). This
		// inverts the series rather than counting up: it runs once per kill, on the hottest path in the game.
		if (Tier.KillsPerWave <= 0.0)
		{
			return 0;
		}
		const double G = std::max(Tier.WaveKillGrowth, 1.0);
		const double K = static_cast<double>(TierKillCount) / Tier.KillsPerWave;
		if (IsEqualApprox(G, 1.0))
		{
			return ClampWave(D, K);
		}
		return ClampWave(D, std::log(1.0 + K * (G - 1.0)) / std::log(G));
	}

	double WaveHpMultiplier(const PcGameData& D, const PcProgress& P)
	{
		const double JobTermValue = JobTerm(D, P, &PcJob::EnemyHealthMultiplier);
		const PcTier* T = FindTier(D, P.CurrentBiomeId);
		if (!T || T->KillsPerWave <= 0.0)
		{
			return JobTermValue;
		}
		return std::pow(T->WaveHpGrowth, static_cast<double>(P.CurrentWaveIndex)) * JobTermValue;
	}

	double WaveRewardMultiplier(const PcGameData& D, const PcProgress& P)
	{
		const PcTier* T = FindTier(D, P.CurrentBiomeId);
		if (!T || T->KillsPerWave <= 0.0)
		{
			return 1.0;
		}
		return std::pow(T->WaveRewardGrowth, static_cast<double>(P.CurrentWaveIndex));
	}

	PcWaveProgress WaveProgress(const PcGameData& D, const PcProgress& P)
	{
		// The same series as CurrentWave, read forward, so a progress bar and the wave banner cannot disagree.
		const PcTier* T = FindTier(D, P.CurrentBiomeId);
		if (!T || T->KillsPerWave <= 0.0)
		{
			return {};
		}
		const double G = std::max(T->WaveKillGrowth, 1.0);
		const double W = static_cast<double>(P.CurrentWaveIndex);
		const double Costs = T->KillsPerWave * std::pow(G, W);
		const double StartedAt = IsEqualApprox(G, 1.0) ? T->KillsPerWave * W : T->KillsPerWave * (std::pow(G, W) - 1.0) / (G - 1.0);
		// Clamped: the wave index stops at max_wave while the kill counter does not; past it the honest answer is a full bar.
		PcWaveProgress Out;
		Out.Done = std::clamp(static_cast<double>(P.TierKillCount) - StartedAt, 0.0, Costs);
		Out.Costs = Costs;
		return Out;
	}

	// --- biomes ------------------------------------------------------------------------------------------------------

	bool IsTierUnlocked(const PcGameData& D, const PcProgress& P, int64_t RequiredStage)
	{
		return EvolutionStage(D, P) >= RequiredStage;
	}

	bool BossesClearedForStage(const PcGameData& D, const PcProgress& P, int64_t RequiredStage)
	{
		// Without this the bait economy is optional: bosses_defeated persists, so from the second cycle a player could
		// walk past every boss biome without baiting anything.
		const int64_t Needed = std::min<int64_t>(RequiredStage - 1, static_cast<int64_t>(D.Code.BossRealms.size()));
		for (int64_t I = 0; I < Needed; ++I)
		{
			if (!P.BossesClearedThisCycle.count(D.Code.BossRealms[static_cast<size_t>(I)]))
			{
				return false;
			}
		}
		return true;
	}

	bool IsPassageOpen(const PcGameData& D, const PcProgress& P, int64_t RequiredStage)
	{
		return IsTierUnlocked(D, P, RequiredStage) && BossesClearedForStage(D, P, RequiredStage);
	}

	bool IsBiomeUnlocked(const PcGameData& D, const PcProgress& P, const PcTier& Tier)
	{
		if (!IsPassageOpen(D, P, Tier.RequiredEvolutionStage))
		{
			return false;
		}
		return P.LifetimeCatnipThisCycle >= Tier.CatnipToUnlock;
	}

	double CatnipUntilUnlocked(const PcProgress& P, const PcTier& Tier)
	{
		return std::max(Tier.CatnipToUnlock - P.LifetimeCatnipThisCycle, 0.0);
	}

	// --- Reawaken ----------------------------------------------------------------------------------------------------

	double ReawakenRequirement(const PcGameData& D, const PcProgress& P)
	{
		// Priced in Stardust, not Catnip: the old Catnip threshold unlocked Reawaken #2 paying 5 Stardust against a balance
		// of 1,059. The first needs the absolute floor (a fraction of 0 is satisfied by the first kill).
		return std::max(D.Reawaken.ReawakenMinStardust, P.StardustGrantedEver * D.Reawaken.ReawakenMinGainFraction);
	}

	double PendingStardust(const PcGameData& D, const PcProgress& P)
	{
		// Against LIFETIME Catnip less what was already paid: sqrt is concave, so a grant on a resettable bank paid
		// ~7,000 Stardust across 200 small cycles for what one cycle paid 500.
		const double Total = std::floor(D.Reawaken.ReawakenStardustCoefficient * std::sqrt(P.LifetimeCatnipEver));
		return std::max(0.0, Total - P.StardustGrantedEver);
	}

	bool CanReawaken(const PcGameData& D, const PcProgress& P)
	{
		// The first Reawaken is the one irreversible action a new player could take for two Stardust; after it the choice
		// is the player's (designer, 2026-08-20).
		if (P.ReawakenCount >= 1)
		{
			return PendingStardust(D, P) >= 1.0;
		}
		return PendingStardust(D, P) >= ReawakenRequirement(D, P);
	}

	bool IsReawakenSuggested(const PcGameData& D, const PcProgress& P)
	{
		return PendingStardust(D, P) >= ReawakenRequirement(D, P);
	}

	double ReawakenRequirementCatnip(const PcGameData& D, const PcProgress& P)
	{
		// A relabelling, not a rebalance: floor(coef x sqrt(L)) - granted >= requirement, solved for L. ceil because the
		// grant FLOORS (without it the gate read open one Catnip band early; Godot's check_hud caught it at 25 granted).
		const double Coef = D.Reawaken.ReawakenStardustCoefficient;
		if (Coef <= 0.0)
		{
			return 0.0;
		}
		const double Needed = std::ceil(P.StardustGrantedEver + ReawakenRequirement(D, P));
		return std::pow(Needed / Coef, 2.0);
	}

	double CatnipUntilReawaken(const PcGameData& D, const PcProgress& P)
	{
		return std::max(ReawakenRequirementCatnip(D, P) - P.LifetimeCatnipEver, 0.0);
	}

	PcBarProgress ReawakenBarProgress(const PcGameData& D, const PcProgress& P)
	{
		// Rebased on where this cycle began, so the bar is empty after a Reawaken (designer, 2026-08-23).
		const double CycleStart = P.LifetimeCatnipEver - P.LifetimeCatnipThisCycle;
		PcBarProgress Out;
		Out.Earned = std::max(P.LifetimeCatnipThisCycle, 0.0);
		Out.Required = std::max(ReawakenRequirementCatnip(D, P) - CycleStart, 0.0);
		return Out;
	}

	double ProjectedReawakenMultiplier(const PcGameData& D, const PcProgress& P)
	{
		return ReawakenMultiplier(D.Reawaken, P.StardustGrantedEver + PendingStardust(D, P));
	}

	// --- bait --------------------------------------------------------------------------------------------------------

	int64_t BaitEarned(const PcGameData& D, const PcProgress& P, const std::string& BiomeId)
	{
		// A divisor on the banked kill count: no state of its own, so it cannot drift from it.
		const PcTier* T = FindTier(D, BiomeId);
		if (!T || T->BossData.empty())
		{
			return 0;
		}
		const PcBoss* Boss = D.FindBoss(T->BossData);
		if (!Boss)
		{
			return 0;
		}
		const int64_t Per = std::max<int64_t>(Boss->KillsPerBait, 1);
		int64_t Kills = 0;
		if (BiomeId == P.CurrentBiomeId)
		{
			Kills = P.TierKillCount;
		}
		else
		{
			const auto It = P.TierKillsByBiome.find(BiomeId);
			Kills = It == P.TierKillsByBiome.end() ? 0 : It->second;
		}
		return Kills / Per;
	}

	bool BaitCarriesOver(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "deep_pockets") > 0.0;
	}

	int64_t BaitHeld(const PcGameData& D, const PcProgress& P, const std::string& BiomeId)
	{
		auto Spent = [&P](const std::string& Id)
		{
			const auto It = P.BaitSpentByBiome.find(Id);
			return It == P.BaitSpentByBiome.end() ? int64_t(0) : It->second;
		};
		if (BaitCarriesOver(D, P))
		{
			// DEEP POCKETS: every biome's bait less every biome's spending (bait used to be stranded where it dropped).
			int64_t Earned = 0;
			int64_t TotalSpent = 0;
			for (const PcTier& T : D.Tiers)
			{
				Earned += BaitEarned(D, P, T.Id);
				TotalSpent += Spent(T.Id);
			}
			return std::max<int64_t>(Earned - TotalSpent, 0);
		}
		return std::max<int64_t>(BaitEarned(D, P, BiomeId) - Spent(BiomeId), 0);
	}

	double AuditCatnipMultiple(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "audit"), 0.0);
	}

	int64_t BaitShortfall(const PcGameData& D, const PcProgress& P, const PcBoss& Boss)
	{
		// Read against the CURRENT biome: working biome 6 does not earn biome 8's fight.
		if (Boss.BaitRequired <= 0)
		{
			return 0;
		}
		if (AuditCatnipMultiple(D, P) > 0.0)
		{
			return 0;
		}
		return std::max<int64_t>(0, Boss.BaitRequired - BaitHeld(D, P, P.CurrentBiomeId));
	}

	double SummonCatnipCost(const PcGameData& D, const PcProgress& P, const PcBoss& Boss)
	{
		// AUDIT charges a multiple of the price the boss already carries: bait_required counts KILLS, and there is no
		// authored exchange rate between a kill and a Catnip.
		double Cost = std::max(Boss.BaitCostCatnip, 0.0);
		const double Audit = AuditCatnipMultiple(D, P);
		if (Audit > 0.0 && Boss.BaitRequired > 0)
		{
			Cost += std::max(Boss.BaitCostCatnip, 0.0) * Audit;
		}
		return Cost;
	}

	// --- Legacy ------------------------------------------------------------------------------------------------------

	double CatnipCarryover(const PcGameData& D, const PcProgress& P)
	{
		return std::clamp(BranchEffect(D, P, "legacy"), 0.0, 1.0);
	}

	int64_t StartingUpgradeLevel(const PcGameData& D, const PcProgress& P)
	{
		return std::max<int64_t>(0, static_cast<int64_t>(BranchEffect(D, P, "legacy", "levels")));
	}

	int64_t StartingBiomeIndex(const PcGameData& D, const PcProgress& P)
	{
		// Clamped to what the ladder has opened: Territory is a shortcut through ground already taken, never a way in.
		const int64_t Wanted = std::max<int64_t>(1, static_cast<int64_t>(BranchEffect(D, P, "legacy", "biome")));
		int64_t Deepest = 1;
		for (size_t I = 0; I < D.Tiers.size(); ++I)
		{
			if (IsTierUnlocked(D, P, D.Tiers[I].RequiredEvolutionStage))
			{
				Deepest = static_cast<int64_t>(I) + 1;
			}
		}
		return std::min(Wanted, Deepest);
	}

	int64_t ReawakenTravelIndex(const PcGameData& D, const PcProgress& P)
	{
		if (D.Tiers.empty())
		{
			return -1;
		}
		const int64_t Index = std::clamp<int64_t>(StartingBiomeIndex(D, P) - 1, 0, static_cast<int64_t>(D.Tiers.size()) - 1);
		return P.CurrentBiomeId == D.Tiers[static_cast<size_t>(Index)].Id ? -1 : Index;
	}
}
