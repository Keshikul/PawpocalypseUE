#pragma once

// The economy (docs/03 §2 PcEconomy): what a kill pays, how waves grow, when a biome opens, what a Reawaken grants, bait and
// Legacy. A port of the pure halves of Godot's GameState (game_state.gd); each function names its Godot original and keeps
// the reasoning. Mutations (buying bait, crediting Catnip, the Reawaken itself) are the game module's, built on these.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <string>

namespace Pc
{
	// --- what a kill pays --------------------------------------------------------------------------------------------

	// whole_catnip(): a payout rounded to a whole number and never below 1, so every kill is felt (0 for <= 0).
	PAWCORE_API double WholeCatnip(double Amount);
	// credit_untouched_catnip(): Catnip that is NOT a kill (interest) is floored, with no minimum of 1.
	PAWCORE_API double UntouchedCatnip(double Amount);
	// get_biome_reward_multiplier(): the biome the player STANDS in (going back to farm pays what that place pays).
	PAWCORE_API double BiomeRewardMultiplier(const PcGameData& D, const PcProgress& P);
	// get_fortune_yield_multiplier(): Fortune's yield, x1 unless the branch is owned.
	PAWCORE_API double FortuneYieldMultiplier(const PcGameData& D, const PcProgress& P);
	// get_golden_chance_bonus() / golden_chance_for(): added to the biome's own chance, capped at max_golden_chance; a biome
	// with no golden mouse stays at 0.
	PAWCORE_API double GoldenChanceBonus(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double GoldenChanceFor(const PcGameData& D, const PcProgress& P, const PcTier& Tier);
	// get_job_catnip_multiplier().
	PAWCORE_API double JobCatnipMultiplier(const PcGameData& D, const PcProgress& P);
	// get_fence_multiplier(): 1 + per x min(kills this wave, fence_max_kills).
	PAWCORE_API double FenceMultiplier(const PcGameData& D, const PcProgress& P);
	// add_currency(): a kill's base reward through every multiplier (the Reawaken's Catnip multiplier first, D17 E3), then
	// whole_catnip.
	PAWCORE_API double KillCatnip(const PcGameData& D, const PcProgress& P, double BaseReward);

	// --- waves -------------------------------------------------------------------------------------------------------

	// current_wave(): the geometric kill series inverted; 0 in a biome without waves; at most max_wave.
	PAWCORE_API int64_t CurrentWave(const PcGameData& D, const PcTier& Tier, int64_t TierKillCount);
	// wave_hp_multiplier() / wave_reward_multiplier(): read the CACHED wave index, so nothing on screen is retroactively
	// buffed. The HP one carries the job's enemy-health term even in a biome without waves.
	PAWCORE_API double WaveHpMultiplier(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double WaveRewardMultiplier(const PcGameData& D, const PcProgress& P);
	// wave_progress(): [kills into this wave, kills this wave costs]; {0, 0} = this biome has no waves.
	struct PcWaveProgress
	{
		double Done = 0.0;
		double Costs = 0.0;
	};
	PAWCORE_API PcWaveProgress WaveProgress(const PcGameData& D, const PcProgress& P);

	// --- biomes ------------------------------------------------------------------------------------------------------

	// is_tier_unlocked(): the PERMANENT ladder (evolution stage).
	PAWCORE_API bool IsTierUnlocked(const PcGameData& D, const PcProgress& P, int64_t RequiredStage);
	// _bosses_cleared_for_stage(): the first N-1 bosses beaten THIS cycle (passage re-locks; power never does).
	PAWCORE_API bool BossesClearedForStage(const PcGameData& D, const PcProgress& P, int64_t RequiredStage);
	// is_passage_open(): the boss half of is_biome_unlocked, without the Catnip (the stage AND this cycle's bosses). The
	// Realms rows ask it (F22, designer 2026-11-03).
	PAWCORE_API bool IsPassageOpen(const PcGameData& D, const PcProgress& P, int64_t RequiredStage);
	// is_biome_unlocked(): stage, this cycle's bosses, and this cycle's lifetime Catnip.
	PAWCORE_API bool IsBiomeUnlocked(const PcGameData& D, const PcProgress& P, const PcTier& Tier);
	// catnip_until_unlocked().
	PAWCORE_API double CatnipUntilUnlocked(const PcProgress& P, const PcTier& Tier);

	// --- Reawaken ----------------------------------------------------------------------------------------------------

	// get_reawaken_requirement(): Stardust the next Reawaken must be worth: max(floor, granted x fraction).
	PAWCORE_API double ReawakenRequirement(const PcGameData& D, const PcProgress& P);
	// get_pending_stardust(): floor(coef x sqrt(lifetime Catnip EVER)) - granted. Split-invariant: reset-spam earns nothing.
	PAWCORE_API double PendingStardust(const PcGameData& D, const PcProgress& P);
	// can_reawaken(): the first Reawaken is gated by the requirement; after that any whole Stardust (designer, 2026-08-20).
	PAWCORE_API bool CanReawaken(const PcGameData& D, const PcProgress& P);
	// is_reawaken_suggested().
	PAWCORE_API bool IsReawakenSuggested(const PcGameData& D, const PcProgress& P);
	// get_reawaken_requirement_catnip(): the same gate solved for lifetime Catnip, ((granted + requirement) / coef)^2,
	// with a ceiling because the grant FLOORS.
	PAWCORE_API double ReawakenRequirementCatnip(const PcGameData& D, const PcProgress& P);
	// catnip_until_reawaken().
	PAWCORE_API double CatnipUntilReawaken(const PcGameData& D, const PcProgress& P);
	// get_reawaken_bar_progress(): [this cycle's Catnip, this cycle's requirement] (rebased, so a fresh cycle reads empty).
	struct PcBarProgress
	{
		double Earned = 0.0;
		double Required = 0.0;
	};
	PAWCORE_API PcBarProgress ReawakenBarProgress(const PcGameData& D, const PcProgress& P);
	// get_projected_reawaken_multiplier(): the multiplier after Reawakening now (same curve as ReawakenMultiplier).
	PAWCORE_API double ProjectedReawakenMultiplier(const PcGameData& D, const PcProgress& P);
	// D17 E3 (2026-11-12): what the Reawaken multiplies Catnip from kills by: the damage multiplier to
	// reawaken_catnip_exponent. Now, after Reawakening now (the Reawaken tab), and for any Stardust total.
	PAWCORE_API double ReawakenCatnipMultiplierFor(const PcReawakenConstants& R, double StardustGrantedEver);
	PAWCORE_API double ReawakenCatnipMultiplier(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double ProjectedReawakenCatnipMultiplier(const PcGameData& D, const PcProgress& P);

	// --- bait --------------------------------------------------------------------------------------------------------

	// bait_earned(): kills in that biome / its boss's kills_per_bait; 0 for a biome with no boss. The current biome reads
	// the live counter, the others their banked kills.
	PAWCORE_API int64_t BaitEarned(const PcGameData& D, const PcProgress& P, const std::string& BiomeId);
	// bait_carries_over(): Deep Pockets.
	PAWCORE_API bool BaitCarriesOver(const PcGameData& D, const PcProgress& P);
	// bait_held(): earned less spent, never negative; with Deep Pockets every biome's bait counts everywhere.
	PAWCORE_API int64_t BaitHeld(const PcGameData& D, const PcProgress& P, const std::string& BiomeId);
	// get_audit_catnip_multiple(): Audit pays the bait off in Catnip (0 = bait still required).
	PAWCORE_API double AuditCatnipMultiple(const PcGameData& D, const PcProgress& P);
	// bait_shortfall(): bait still owed in the CURRENT biome before its boss comes out; 0 under Audit.
	PAWCORE_API int64_t BaitShortfall(const PcGameData& D, const PcProgress& P, const PcBoss& Boss);
	// The Catnip buy_bait() charges for a summon: the boss's price, plus price x Audit when Audit buys the bait.
	PAWCORE_API double SummonCatnipCost(const PcGameData& D, const PcProgress& P, const PcBoss& Boss);

	// --- Legacy ------------------------------------------------------------------------------------------------------

	// get_catnip_carryover(): the share of Catnip a Reawaken keeps, 0..1.
	PAWCORE_API double CatnipCarryover(const PcGameData& D, const PcProgress& P);
	// get_starting_upgrade_level(): the level every visible line restarts at.
	PAWCORE_API int64_t StartingUpgradeLevel(const PcGameData& D, const PcProgress& P);
	// get_starting_biome_index(): 1-based; Territory's wish clamped to the deepest biome the ladder has opened.
	PAWCORE_API int64_t StartingBiomeIndex(const PcGameData& D, const PcProgress& P);
	// hud.gd _travel_to_starting_biome(): where the player goes after a Reawaken's spend step, as an index into D.Tiers
	// (0-based), or -1 when already standing in it. Territory wins (designer 2026-11-01): the starting biome, deeper or
	// shallower than the one the Reawaken happened in. reawaken() clears the current biome, so after a real Reawaken
	// this always travels (from inside the biome it reloads it, as Godot's change_scene does).
	PAWCORE_API int64_t ReawakenTravelIndex(const PcGameData& D, const PcProgress& P);
}
