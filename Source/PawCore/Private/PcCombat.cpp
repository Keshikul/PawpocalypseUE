#include "PcCombat.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Pc
{
	namespace
	{
		double CritFactor(const PcCombatStats& S)
		{
			return 1.0 + S.CritChance * (S.CritMultiplier - 1.0);
		}
	}

	double DamageScale(const PcCombatStats& S)
	{
		return S.EvolutionDamageMultiplier * S.WeaponMultiplier * (1.0 + S.AdditiveDamageSum) * S.MultiplicativeDamageProduct
			* S.ReawakenMultiplier;
	}

	double PawStrikeDamage(const PcCombatConstants& C, const PcCombatStats& S, int64_t ShredStacks)
	{
		const double Shred = 1.0 + S.ShredPerStack * static_cast<double>(std::max<int64_t>(ShredStacks, 0));
		return C.BasePawDamage * S.PawDamageMultiplier * Shred * DamageScale(S);
	}

	double StrikesPerSec(const PcCombatConstants& C, const PcCombatStats& S)
	{
		return std::max(C.BaseAttackSpeed * S.AttackSpeedMultiplier, 0.0);
	}

	double PawDps(const PcCombatConstants& C, const PcCombatStats& S)
	{
		const double AverageStacks = static_cast<double>(C.ShredMaxStacks) * 0.5;
		const double Shred = 1.0 + S.ShredPerStack * AverageStacks;
		return C.BasePawDamage * S.PawDamageMultiplier * Shred * DamageScale(S) * StrikesPerSec(C, S) * CritFactor(S);
	}

	double AuraDps(const PcCombatConstants& C, const PcCombatStats& S)
	{
		if (!S.bAuraOwned)
		{
			return 0.0;
		}
		return C.BaseDamagePerSec * S.PawDamageMultiplier * S.AuraDamageMultiplier * DamageScale(S) * CritFactor(S);
	}

	double PlayerDps(const PcCombatConstants& C, const PcCombatStats& S)
	{
		return PawDps(C, S) + AuraDps(C, S);
	}

	double KittenDps(const PcCombatConstants& C, const PcCombatStats& S)
	{
		return C.BaseKittenDamagePerSec * S.KittenPowerMultiplier * (1.0 + S.KittenAdditiveDamageSum) * S.KittenMultiplicativeDamageProduct
			* S.ReawakenMultiplier;
	}

	double PounceDamage(const PcCombatConstants& C, const PcCombatStats& S, bool bTargetVulnerable, double RateMultiplier)
	{
		const double Dials = S.PouncePowerMultiplier * RateMultiplier;
		double Burst = PlayerDps(C, S) * C.PounceDamageSeconds * Dials;
		Burst *= PounceCapScale(C, S, RateMultiplier);
		if (bTargetVulnerable)
		{
			Burst *= C.PounceVulnerableMultiplier;
		}
		return Burst;
	}

	double PounceAuraMultiple(const PcCombatConstants& C, const PcCombatStats& S, double RateMultiplier)
	{
		const double Cooldown = PounceCooldown(C, S);
		if (Cooldown <= 0.0)
		{
			return std::numeric_limits<double>::infinity();
		}
		const double Dials = S.PouncePowerMultiplier * RateMultiplier;
		return C.PounceDamageSeconds * Dials / Cooldown;
	}

	double PounceCapScale(const PcCombatConstants& C, const PcCombatStats& S, double RateMultiplier)
	{
		const double Cap = C.PounceMaxAuraMultiple;
		if (Cap <= 0.0)
		{
			return 1.0;
		}
		const double Multiple = PounceAuraMultiple(C, S, RateMultiplier);
		if (Multiple <= Cap)
		{
			return 1.0;
		}
		return Cap / Multiple;
	}

	double PounceCooldown(const PcCombatConstants& C, const PcCombatStats& S)
	{
		return C.PounceCooldownSec * (1.0 - S.PounceCooldownReduction);
	}

	double OverkillDamage(const PcCombatStats& S, double ExcessDamage)
	{
		if (ExcessDamage <= 0.0)
		{
			return 0.0;
		}
		return ExcessDamage * S.OverkillFraction;
	}

	double OverkillBounty(const PcCombatConstants& C, const PcCombatStats& S, double ExcessDamage, double VictimMaxHp, double VictimReward)
	{
		if (ExcessDamage <= 0.0 || VictimMaxHp <= 0.0 || VictimReward <= 0.0)
		{
			return 0.0;
		}
		if (S.OverkillFraction <= 0.0)
		{
			return 0.0;
		}
		const double Wasted = ExcessDamage / VictimMaxHp;
		const double Exponent = C.OverkillBountyExponent;
		const double WastedMultiple = Exponent > 0.0 ? std::min(Wasted, std::pow(Wasted, Exponent))
			: std::min(Wasted, C.OverkillBountyMaxMultiple);
		return VictimReward * S.OverkillFraction * WastedMultiple;
	}

	double PoopDps(const PcCombatConstants& C, const PcCombatStats& S)
	{
		return PlayerDps(C, S) * C.PoopAuraFraction * S.PoopPowerMultiplier * (1.0 + S.PoopAdditiveDamageSum) * S.PoopMultiplicativeDamageProduct;
	}
}
