#include "PcStats.h"

#include "PcJobs.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		const PcUpgrade& Upgrade(const PcGameData& D, const char* Id)
		{
			static const PcUpgrade Missing;
			const PcUpgrade* U = D.FindUpgrade(Id);
			return U ? *U : Missing;
		}

		int64_t FloorsReached(const PcProgress& P)
		{
			return std::max<int64_t>(P.DeepestBiomeIndex, 0);
		}

		// Patience pays the Sphinx (who swings at half speed) more the slower it swings; a pure function of another stat,
		// so the resolver sees it for free.
		double PatienceCritBonus(const PcGameData& D, const PcProgress& P, double AttackSpeed)
		{
			const double Per = BranchEffect(D, P, "patience");
			if (Per <= 0.0 || AttackSpeed <= 0.0)
			{
				return 0.0;
			}
			return Per * std::max(1.0 / AttackSpeed - 1.0, 0.0);
		}

		// Severance grows with Reawakens already taken: nothing on a fresh save, a great deal on an old one.
		double SeveranceMultiplier(const PcGameData& D, const PcProgress& P)
		{
			const double Per = BranchEffect(D, P, "severance");
			if (Per <= 0.0)
			{
				return 1.0;
			}
			return 1.0 + Per * static_cast<double>(P.ReawakenCount);
		}

		// Reputation, Grudge Ledger and Body Count: ADDITIVE with each other and MULTIPLICATIVE with everything else. Three
		// compounding terms multiplying each other would square and cube a long run.
		double CompoundingMultiplier(const PcGameData& D, const PcProgress& P)
		{
			double Total = 0.0;
			const double PerWave = BranchEffect(D, P, "reputation");
			if (PerWave > 0.0)
			{
				Total += PerWave * static_cast<double>(P.WavesHere);
			}
			const double PerCard = BranchEffect(D, P, "grudge_ledger");
			if (PerCard > 0.0)
			{
				Total += PerCard * static_cast<double>(P.CardsTakenThisRun);
			}
			const double Per10k = BranchEffect(D, P, "body_count");
			if (Per10k > 0.0)
			{
				Total += Per10k * std::floor(static_cast<double>(P.KillsThisCycle) / 10000.0);
			}
			return 1.0 + Total;
		}
	}

	int64_t UpgradeLevel(const PcProgress& P, const std::string& Id)
	{
		const auto It = P.UpgradeLevels.find(Id);
		return It == P.UpgradeLevels.end() ? 0 : It->second;
	}

	int64_t BranchLevel(const PcProgress& P, const std::string& Id)
	{
		const auto It = P.BranchLevels.find(Id);
		return It == P.BranchLevels.end() ? 0 : It->second;
	}

	double EffectTotal(const PcUpgrade& U, int64_t Level)
	{
		if (U.EffectType == UpgradeEffectType::MULTIPLICATIVE)
		{
			return std::pow(U.EffectPerLevel, static_cast<double>(Level));
		}
		return U.EffectPerLevel * static_cast<double>(Level);
	}

	double EffectiveCap(const PcUpgrade& U, const PcProgress& P)
	{
		if (U.Cap < 0.0)
		{
			return -1.0;
		}
		return U.Cap + U.CapPerFloor * static_cast<double>(FloorsReached(P)) + U.CapPerReawaken * static_cast<double>(P.ReawakenCount);
	}

	double CappedEffect(const PcUpgrade& U, int64_t Level, const PcProgress& P)
	{
		const double Value = EffectTotal(U, Level);
		return U.Cap >= 0.0 ? std::min(Value, EffectiveCap(U, P)) : Value;
	}

	double BranchEffect(const PcGameData& D, const PcProgress& P, const std::string& Id, const std::string& Channel)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		if (!B)
		{
			return 0.0;
		}
		const int64_t Level = BranchLevel(P, Id);
		if (!Channel.empty())
		{
			const auto It = B->ExtraEffects.find(Channel);
			if (Level <= 0 || It == B->ExtraEffects.end() || It->second.empty())
			{
				return 0.0;
			}
			return It->second[static_cast<size_t>(std::min<int64_t>(Level, static_cast<int64_t>(It->second.size()))) - 1];
		}
		if (Level <= 0 || B->LevelEffects.empty())
		{
			return 0.0;
		}
		return B->LevelEffects[static_cast<size_t>(std::min<int64_t>(Level, static_cast<int64_t>(B->LevelCosts.size()))) - 1];
	}

	int64_t EvolutionStage(const PcGameData& D, const PcProgress& P)
	{
		int64_t Stage = 1;
		for (const std::string& Realm : D.Code.BossRealms)
		{
			if (!P.BossesDefeated.count(Realm))
			{
				break;
			}
			++Stage;
		}
		// THE LAST UNICORN (stage 7) is the Litter King's: finishing the game is the seventh step.
		if (P.bFinalBossDefeated)
		{
			++Stage;
		}
		return std::min(Stage, D.Code.EvolutionStageCount);
	}

	double ReawakenMultiplier(const PcReawakenConstants& R, double StardustGrantedEver)
	{
		const double Scale = R.ReawakenMultScale;
		if (Scale <= 0.0)
		{
			return 1.0;
		}
		return std::pow(1.0 + std::max(StardustGrantedEver, 0.0) / Scale, R.ReawakenMultExponent);
	}

	double CardCostMultiplier(const PcGameData& D, const PcProgress& P, CardCost Cost)
	{
		// The deck in Godot's order (card files sorted by name = by id), so even the product's rounding matches.
		double Product = 1.0;
		for (const PcCard& Card : D.Cards)
		{
			if (Card.Cost != Cost)
			{
				continue;
			}
			const auto It = P.CardLevels.find(Card.Id);
			const int64_t Level = It == P.CardLevels.end() ? 0 : It->second;
			if (Level > 0)
			{
				const bool bHasCost = Card.Cost != CardCost::NONE && Level >= 1 && Level <= static_cast<int64_t>(Card.CostValues.size());
				Product *= bHasCost ? Card.CostValues[static_cast<size_t>(Level - 1)] : 0.0;
			}
		}
		return Product;
	}

	double CardCostTotal(const PcGameData& D, const PcProgress& P, CardCost Cost)
	{
		double Total = 0.0;
		for (const PcCard& Card : D.Cards)
		{
			if (Card.Cost != Cost)
			{
				continue;
			}
			const auto It = P.CardLevels.find(Card.Id);
			const int64_t Level = It == P.CardLevels.end() ? 0 : It->second;
			if (Level > 0 && Card.Cost != CardCost::NONE && Level <= static_cast<int64_t>(Card.CostValues.size()))
			{
				Total += Card.CostValues[static_cast<size_t>(Level - 1)];
			}
		}
		return Total;
	}

	int64_t KittenCount(const PcGameData& D, const PcProgress& P)
	{
		const int64_t Vitality = static_cast<int64_t>(BranchEffect(D, P, "vitality"));
		const int64_t Total = static_cast<int64_t>(EffectTotal(Upgrade(D, "kitten_count"), UpgradeLevel(P, "kitten_count"))) + Vitality;
		// Kittens lost to a card are whole units, so they SUBTRACT. Floored at zero: a negative count would silently invert
		// the kitten loop rather than empty it.
		return std::max<int64_t>(0, Total - static_cast<int64_t>(CardCostTotal(D, P, CardCost::KITTEN_COUNT)));
	}

	double CurrencyFindMultiplier(const PcGameData& D, const PcProgress& P)
	{
		return CappedEffect(Upgrade(D, "currency_find"), UpgradeLevel(P, "currency_find"), P)
			* CardCostMultiplier(D, P, CardCost::CATNIP_YIELD);
	}

	double UpgradeCost(const PcUpgrade& U, int64_t Level)
	{
		return U.BaseCost * std::pow(U.CostGrowth, static_cast<double>(Level));
	}

	double BranchCost(const PcSkillBranch& B, int64_t Level)
	{
		if (Level < 0 || Level >= static_cast<int64_t>(B.LevelCosts.size()))
		{
			return -1.0;
		}
		return B.LevelCosts[static_cast<size_t>(Level)];
	}

	PcCombatStats RebuildStats(const PcGameData& D, const PcProgress& P)
	{
		PcCombatStats S;
		// _capped_effect, not _effect_total, on the uncapped damage lines too: a no-op today, but an authored cap on them
		// would otherwise have been silently INERT.
		S.PawDamageMultiplier = CappedEffect(Upgrade(D, "paw_damage"), UpgradeLevel(P, "paw_damage"), P);
		S.KittenPowerMultiplier = CappedEffect(Upgrade(D, "kitten_power"), UpgradeLevel(P, "kitten_power"), P);

		// --- the paw (v5) ---
		S.AttackSpeedMultiplier = CappedEffect(Upgrade(D, "attack_speed"), UpgradeLevel(P, "attack_speed"), P);
		S.AttackSpeedMultiplier *= JobTerm(D, P, &PcJob::AttackSpeedMultiplier);
		// The Sphinx multiplies crit AFTER the cap: the cap stops the shop line running away; a job trade is another lever.
		S.CritChance = CappedEffect(Upgrade(D, "crit_chance"), UpgradeLevel(P, "crit_chance"), P);
		S.CritChance = std::min((S.CritChance + JobCritChanceBonus(D, P)) * JobTerm(D, P, &PcJob::CritChanceMultiplier), 1.0);
		S.ShredPerStack = CappedEffect(Upgrade(D, "shred"), UpgradeLevel(P, "shred"), P);
		// Dent adds after Shred's cap on purpose (a Stardust branch is a different lever from the shop line).
		S.ShredPerStack += std::max(BranchEffect(D, P, "dent"), 0.0);
		S.ExecuteThreshold = CappedEffect(Upgrade(D, "execute"), UpgradeLevel(P, "execute"), P);

		// --- the aura, which must be bought (v5): guarded on LEVEL, because level 1 buys the ability (a x1.0 effect) ---
		S.bAuraOwned = BranchLevel(P, "unlock_aura") > 0;
		if (S.bAuraOwned && BranchLevel(P, "aura_damage") > 0)
		{
			S.AuraDamageMultiplier = BranchEffect(D, P, "aura_damage");
		}
		if (BranchLevel(P, "aura_reach") > 0)
		{
			S.AuraRadiusBranchMultiplier = BranchEffect(D, P, "aura_reach");
		}

		S.KittenSpeedMultiplier = EffectTotal(Upgrade(D, "kitten_speed"), UpgradeLevel(P, "kitten_speed"));
		S.PouncePowerMultiplier = EffectTotal(Upgrade(D, "pounce_power"), UpgradeLevel(P, "pounce_power"))
			* CardCostMultiplier(D, P, CardCost::POUNCE_DAMAGE);
		const PcUpgrade& Cooldown = Upgrade(D, "pounce_cooldown");
		S.PounceCooldownReduction = std::min(EffectTotal(Cooldown, UpgradeLevel(P, "pounce_cooldown")), EffectiveCap(Cooldown, P));
		const PcUpgrade& Pierce = Upgrade(D, "shield_pierce");
		S.ShieldPierceFraction = std::min(EffectTotal(Pierce, UpgradeLevel(P, "shield_pierce")), EffectiveCap(Pierce, P));
		const PcUpgrade& Overkill = Upgrade(D, "overkill");
		S.OverkillFraction = std::min(EffectTotal(Overkill, UpgradeLevel(P, "overkill")), EffectiveCap(Overkill, P));

		S.CritMultiplier = D.Combat.CritMultiplier * JobTerm(D, P, &PcJob::CritDamageMultiplier);
		// AFTER attack speed is final: Patience is a function of it.
		S.CritMultiplier *= 1.0 + PatienceCritBonus(D, P, S.AttackSpeedMultiplier);
		// Evolution damage is a pure function of stage; the base is data (5.04 ^ 3 = x128 after the ladder was re-spread).
		const int64_t Stage = EvolutionStage(D, P);
		S.EvolutionDamageMultiplier = std::pow(D.Combat.EvolutionDamagePerStage, static_cast<double>(Stage - 1));
		const PcEvolution* Evo = nullptr;
		for (const PcEvolution& E : D.Evolutions)
		{
			if (E.Stage == Stage)
			{
				Evo = &E;
			}
		}
		if (!Evo)
		{
			for (const PcEvolution& E : D.Evolutions)
			{
				if (E.Stage == 1)
				{
					Evo = &E;
				}
			}
		}
		S.WeaponMultiplier = Evo ? Evo->WeaponMultiplier : PcEvolution().WeaponMultiplier;
		S.ReawakenMultiplier = ReawakenMultiplier(D.Reawaken, P.StardustGrantedEver);

		// Pounce and Overkill branches, guarded on level > 0 (level 1 of Pounce buys the ability, a x1.0 effect).
		if (BranchLevel(P, "pounce") > 0)
		{
			S.PouncePowerMultiplier *= BranchEffect(D, P, "pounce");
		}
		// The unlock carries the base splash and the ladder adds to it.
		if (BranchLevel(P, "unlock_overkill") > 0)
		{
			const double OverkillCap = EffectiveCap(Overkill, P);
			double Splash = BranchEffect(D, P, "unlock_overkill");
			if (BranchLevel(P, "overkill") > 0)
			{
				Splash = BranchEffect(D, P, "overkill");
			}
			S.OverkillFraction = std::min(S.OverkillFraction + Splash, OverkillCap);
		}

		// Claws (v6) is the old Ferocity and Mystique in one node: the additive half is level_effects, the multiplicative
		// half the "mult" channel. Collapsing them remains the single easiest way to break the economy.
		S.AdditiveDamageSum += BranchEffect(D, P, "claws");
		if (BranchLevel(P, "claws") > 0)
		{
			S.MultiplicativeDamageProduct *= BranchEffect(D, P, "claws", "mult");
		}
		S.KittenAdditiveDamageSum += BranchEffect(D, P, "instinct");
		if (BranchLevel(P, "unicorn") > 0)
		{
			S.PoopMultiplicativeDamageProduct *= BranchEffect(D, P, "unicorn");
		}

		// FERAL INSTINCT: the kittens inherit a share of the paw's crit, as an expected value (no discrete swing to crit on).
		const double CritShare = std::clamp(BranchEffect(D, P, "whisker_trade"), 0.0, 1.0);
		if (CritShare > 0.0)
		{
			const double Inherited = S.CritChance * CritShare;
			S.KittenMultiplicativeDamageProduct *= 1.0 + Inherited * (S.CritMultiplier - 1.0);
		}

		// THE JOB, last, as a modifier on the finished cat. all_damage rides Claws' multiplicative bracket.
		if (EquippedJob(D, P))
		{
			S.PawDamageMultiplier *= JobTerm(D, P, &PcJob::PawDamageMultiplier);
			S.AuraDamageMultiplier *= JobTerm(D, P, &PcJob::AuraDamageMultiplier);
			S.AuraRadiusBranchMultiplier *= JobTerm(D, P, &PcJob::AuraRadiusMultiplier);
			S.KittenMultiplicativeDamageProduct *= JobTerm(D, P, &PcJob::KittenDamageMultiplier);
			S.PouncePowerMultiplier *= JobTerm(D, P, &PcJob::PounceDamageMultiplier);
			S.MultiplicativeDamageProduct *= JobTerm(D, P, &PcJob::AllDamageMultiplier);
		}
		// Outside the job block: Severance is bought with Stardust and stays bought.
		S.MultiplicativeDamageProduct *= SeveranceMultiplier(D, P);
		S.MultiplicativeDamageProduct *= CompoundingMultiplier(D, P);
		return S;
	}
}
