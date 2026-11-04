#include "PcCheats.h"

#include "PcProgress.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	void CheatAddCurrency(PcProgress& P, double Catnip, double Stardust)
	{
		CreditCurrency(P, Catnip);
		if (Stardust > 0.0)
		{
			P.Stardust += Stardust;
		}
	}

	void CheatMaxAllUpgrades(const PcGameData& D, PcProgress& P)
	{
		for (const PcUpgrade& U : D.Upgrades)
		{
			const int64_t Level = UpgradeLevel(P, U.Id);
			int64_t NewLevel = 0;
			if (U.Cap >= 0.0)
			{
				const double Reachable = EffectiveCap(U, P);
				if (U.EffectType == UpgradeEffectType::ADDITIVE)
				{
					NewLevel = static_cast<int64_t>(std::ceil(Reachable / U.EffectPerLevel));
				}
				else
				{
					NewLevel = static_cast<int64_t>(std::ceil(std::log(Reachable) / std::log(U.EffectPerLevel)));
				}
			}
			else
			{
				NewLevel = Level + CheatUncappedStep;
			}
			P.UpgradeLevels[U.Id] = std::max(Level, NewLevel);
		}
	}

	void CheatSetReawakenCount(PcProgress& P, int64_t Count)
	{
		P.ReawakenCount = Count;
	}

	bool CheatAdvanceStage(const PcGameData& D, PcProgress& P)
	{
		const int64_t Stage = EvolutionStage(D, P);
		if (Stage >= D.Code.EvolutionStageCount)
		{
			return false;
		}
		// Stage N is held up by the Nth boss (BOSS_REALMS is 0-indexed).
		const size_t Index = static_cast<size_t>(Stage - 1);
		if (Index >= D.Code.BossRealms.size())
		{
			return false;
		}
		MarkBossDefeated(D, P, D.Code.BossRealms[Index]);
		return true;
	}

	void CheatUnlockAllTiers(const PcGameData& D, PcProgress& P)
	{
		for (const std::string& Realm : D.Code.BossRealms)
		{
			P.BossesDefeated.insert(Realm);
		}
	}

	void CheatUnlockAllStages(const PcGameData& D, PcProgress& P)
	{
		CheatUnlockAllTiers(D, P);
		// The third gate too, this cycle's passage (F18, 2026-11-03): without it every biome behind a boss stayed shut.
		for (const std::string& Realm : D.Code.BossRealms)
		{
			P.BossesClearedThisCycle.insert(Realm);
		}
		double Deepest = 0.0;
		for (const PcTier& T : D.Tiers)
		{
			Deepest = std::max(Deepest, T.CatnipToUnlock);
		}
		P.LifetimeCatnipThisCycle = std::max(P.LifetimeCatnipThisCycle, Deepest);
		P.DeepestBiomeIndex = static_cast<int64_t>(D.Tiers.size()) - 1;
	}
}
