#pragma once

// The ONE damage path (CLAUDE.md hard rule; docs/02, docs/03 §2): the port of Godot's CombatResolver
// (scripts/autoload/combat_resolver.gd) and its CombatStats snapshot (scripts/systems/combat_stats.gd). Stateless: reads
// the combat constants and a stats snapshot, returns DPS numbers; callers apply dps * delta themselves each step.
//
// Damage is continuous DPS applied to whatever is in contact (Numbers Doc §1.1); crit is an expected-value term on the
// continuous rates, because a per-step dice roll would make damage framerate-dependent. The paw strike the realm rolls for
// real goes through PawStrikeDamage.
//
// Additive and multiplicative skill-tree terms are deliberately SEPARATE brackets and must stay that way: Claws carries
// both (the additive half in level_effects, the multiplicative half in its "mult" channel), so collapsing them into one
// additive sum would quietly halve it. The same split applies to kittens (Instinct) and poop (Unicorn).

#include "PcCore.h"
#include "PcDataRows.h"

namespace Pc
{
	// Aggregated input snapshot, assembled by PcStats from shop levels + skill nodes + evolution + Reawaken. Every term
	// arrives pre-resolved from data, so the resolver holds only the formula's structure, never a balance number.
	struct PcCombatStats
	{
		double PawDamageMultiplier = 1.0;        // UpgradeData(paw_damage).effect_per_level ^ level
		double AttackSpeedMultiplier = 1.0;      // UpgradeData(attack_speed)
		// Extra damage per shred stack, as a fraction of the strike. Ten stacks at 0.06 is +60% on a committed target.
		double ShredPerStack = 0.0;              // UpgradeData(shred)
		// A strike kills outright below this share of the victim's MAX hp. Never bypasses a shield.
		double ExecuteThreshold = 0.0;           // UpgradeData(execute)
		// The aura is a skill branch (v5): until bought the cat has only its paw and AuraDps() is zero.
		bool bAuraOwned = false;
		double AuraDamageMultiplier = 1.0;       // SkillBranchData(aura_damage)
		double AuraRadiusBranchMultiplier = 1.0; // SkillBranchData(aura_reach)
		double EvolutionDamageMultiplier = 1.0;  // evolution_damage_per_stage ^ (stage - 1)
		double WeaponMultiplier = 1.0;           // EvolutionData, x1.00 -> x1.40
		double AdditiveDamageSum = 0.0;          // Claws' additive bracket -> (1 + sum)
		double MultiplicativeDamageProduct = 1.0; // Claws' "mult" channel -> used directly
		double CritChance = 0.0;                 // UpgradeData(crit_chance)
		double CritMultiplier = 2.0;             // CombatConstantsData.crit_multiplier
		double OverkillFraction = 0.0;           // UpgradeData(overkill), clamped to cap; NOT a DPS term
		double KittenPowerMultiplier = 1.0;      // UpgradeData(kitten_power)
		double KittenAdditiveDamageSum = 0.0;    // Instinct's additive bracket -> (1 + sum)
		double KittenMultiplicativeDamageProduct = 1.0;
		double PoopPowerMultiplier = 1.0;        // neutral; nothing targets it directly today
		double PoopAdditiveDamageSum = 0.0;
		double PoopMultiplicativeDamageProduct = 1.0; // Unicorn's multiplicative bracket
		double KittenSpeedMultiplier = 1.0;      // movement, not damage
		// Fraction of damage that still lands on a SHIELDED target; capped well below 1.0.
		double ShieldPierceFraction = 0.0;       // UpgradeData(shield_pierce)
		double PouncePowerMultiplier = 1.0;      // UpgradeData(pounce_power)
		double PounceCooldownReduction = 0.0;    // UpgradeData(pounce_cooldown), clamped to the line's cap
		double ReawakenMultiplier = 1.0;         // soft-capped, see ReawakenConstantsData
	};

	// Everything both of the cat's damage sources are scaled by: the paw and the aura are two ways the same cat hits
	// things, and a bonus that applied to one but not the other would be a bug nobody could see.
	PAWCORE_API double DamageScale(const PcCombatStats& S);

	// One paw strike against one mouse, before the crit roll. ShredStacks = consecutive strikes already landed on this
	// target (the realm owns the streak; the resolver is stateless).
	PAWCORE_API double PawStrikeDamage(const PcCombatConstants& C, const PcCombatStats& S, int64_t ShredStacks = 0);
	PAWCORE_API double StrikesPerSec(const PcCombatConstants& C, const PcCombatStats& S);

	// The paw's expected DPS: crit folded in, shred counted at HALF its ceiling (a sweeping player never stacks, a committed
	// one always does; full would make every projection optimistic in a way nothing downstream could correct).
	PAWCORE_API double PawDps(const PcCombatConstants& C, const PcCombatStats& S);

	// The aura's contact DPS: exactly ZERO until bought ("you do not have this yet" must not read as "this is weak").
	PAWCORE_API double AuraDps(const PcCombatConstants& C, const PcCombatStats& S);

	// Everything the player personally puts out. Pounce, ground zones and edge burn are priced against THIS, never the
	// aura alone (a fraction of a term worth zero until the Aura is bought is worth nothing).
	PAWCORE_API double PlayerDps(const PcCombatConstants& C, const PcCombatStats& S);

	// Each kitten's own contact DPS. Kittens miss the weapon multiplier and the Claws brackets by design.
	PAWCORE_API double KittenDps(const PcCombatConstants& C, const PcCombatStats& S);

	// One Pounce against one target. RateMultiplier = Auto-Pounce's burst multiplier (1 for a manual cast); a parameter
	// because the cap must see the whole product. The vulnerable-window bonus sits OUTSIDE the cap: it is payment for
	// timing a shield, it applies only to a target that has one, and it cannot be bought.
	PAWCORE_API double PounceDamage(const PcCombatConstants& C, const PcCombatStats& S, bool bTargetVulnerable, double RateMultiplier = 1.0);

	// Sustained Pounce DPS as a multiple of aura DPS: burst size over cooldown. Uncapped, three dials reached ~18x aura and
	// 90.6% of all damage in the game. Infinity when the cooldown is gone.
	PAWCORE_API double PounceAuraMultiple(const PcCombatConstants& C, const PcCombatStats& S, double RateMultiplier = 1.0);

	// How far a burst is scaled DOWN to keep the sustained multiple inside pounce_max_aura_multiple: 1 below the cap.
	// Capping the PRODUCT, not each dial, keeps every dial working until their combined effect is what is too large.
	PAWCORE_API double PounceCapScale(const PcCombatConstants& C, const PcCombatStats& S, double RateMultiplier = 1.0);

	// Seconds between Pounces after upgrades (Auto-Pounce reads the same value: two callers computing it would drift).
	PAWCORE_API double PounceCooldown(const PcCombatConstants& C, const PcCombatStats& S);

	// The share of a killing blow's WASTED damage that carries to the next mouse (the realm decides WHO it lands on).
	PAWCORE_API double OverkillDamage(const PcCombatStats& S, double ExcessDamage);

	// Catnip paid for damage that had nowhere to go, priced in the VICTIM'S OWN REWARD (a flat bounty would vanish by
	// biome 3). Bounded by data: exponent 0 = a flat ceiling at overkill_bounty_max_multiple; exponent > 0 =
	// min(wasted, wasted ^ exponent), linear below a wasted multiple of 1 and sub-linear above, meeting at exactly 1.
	PAWCORE_API double OverkillBounty(const PcCombatConstants& C, const PcCombatStats& S, double ExcessDamage, double VictimMaxHp,
		double VictimReward);

	// Rainbow Poop's zone DPS, priced against the whole player (player_dps already carries the Reawaken multiplier, so it is
	// not applied twice). An absolute poop number was worth 0.0% of total DPS for the whole run in the simulator.
	PAWCORE_API double PoopDps(const PcCombatConstants& C, const PcCombatStats& S);
}
