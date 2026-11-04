#pragma once

// The gameplay constants that live in Godot CODE rather than /data (docs/06 §5, appendix C), loaded from
// Data/CodeConstants.csv (written by tools/export_code_constants.py from the reviewed Data/CodeConstantsMap.csv; the CSV
// keeps each one's Godot reason). Every field has a CSV row and every row a field: the loader refuses a missing, doubled
// or unknown id, so a number cannot silently fall back to a copy in C++. No defaults here for the same reason.

#include "PcCore.h"
#include "PcCsv.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Pc
{
	struct PcCodeConstants
	{
		// --- the rules PawCore ports today (GameState, CardManager) ---------------------------------------------------
		// BOSS_REALMS: the five biomes holding a boss, in ladder order. Beating them in order raises the stage.
		std::vector<std::string> BossRealms;
		// EVOLUTION_STAGE_COUNT: stage 1 is the baseline, each boss grants one, the ending grants the seventh.
		int64_t EvolutionStageCount = 0;
		// MAX_WAVE: the wave counter stops here while the kill counter does not.
		int64_t MaxWave = 0;
		// MAX_GOLDEN_CHANCE: a chance at or above 1.0 would mean the pool never rolls and every enemy is golden.
		double MaxGoldenChance = 0.0;
		// CURRENCY_EPSILON: costs are fractional and Catnip is summed, so "can the player afford this" forgives a
		// millionth (12.9 - 6.0 == 6.899999999999999 must still buy the 6.9 level; xMax does this on every press).
		double CurrencyEpsilon = 0.0;
		// BULK_MAX_STEPS: the bound on one bulk purchase, xMax included (a free uncapped line would spin forever).
		int64_t BulkMaxSteps = 0;
		// MOUNTED_STAGE: the first stage the cat rides the Unicorn; owning the node shows at least this stage.
		int64_t MountedStage = 0;
		// LEVY_EVERY: Levy pays a lump every this many kills.
		int64_t LevyEvery = 0;
		// INFESTATION_DECAY_PER_SEC / INFESTATION_CEILING: spawn rate Infestation adds per kill bleeds away at this rate
		// and never passes the ceiling.
		double InfestationDecayPerSec = 0.0;
		double InfestationCeiling = 0.0;
		// REALTIME_INTEREST_PERIOD_SEC: Compound Interest pays once per this many seconds of play.
		double RealtimeInterestPeriodSec = 0.0;
		// begin_biome()'s literal: Second Helping's deferred half pays on arriving at this biome index (0-based: biome 4)
		// or deeper; a place to travel to, so the half is a destination rather than a timer.
		int64_t SecondHelpingBiomeIndex = 0;
		// FENCE_MAX_KILLS: Fence pays per kill this wave up to this many, so its ceiling is authored, not clipped.
		int64_t FenceMaxKills = 0;
		// CROWD_THRESHOLD / LAST_STAND_THRESHOLD: Swarm Logic pays at this many live mice or more, Last Stand below this
		// many (the field counts them; phase 2).
		int64_t CrowdThreshold = 0;
		int64_t LastStandThreshold = 0;
		// LUNCH_FULL_SECONDS / CARRY_FULL_SECONDS: the stillness that fills Long Lunch and Carry; more pays nothing more
		// (an uncapped bank pays for walking away from the game).
		double LunchFullSeconds = 0.0;
		double CarryFullSeconds = 0.0;
		// CardManager.LEVEL_COST_BASE / LEVEL_COST_GROWTH: the Catches XP curve (pawsim's CATCHES_BASE/GROWTH, now read
		// from the same row); the first level costs 24 kills (raised from 8 on 2026-08-18: "XP gain should be slower in
		// the beginning").
		double CardLevelCostBase = 0.0;
		double CardLevelCostGrowth = 0.0;
		// CardManager.BASE_HAND_SIZE.
		int64_t BaseHandSize = 0;

		// --- the field's rules (TierController, Boss, Kitten, Enemy, Player): read by PcField in phase 2 ---------------
		int64_t MaxLiveEnemies = 0;
		double MinSpawnInterval = 0.0;
		double MinPlayfieldWidth = 0.0;
		double SpawnEdgeInset = 0.0;
		double SpawnDirectionSpread = 0.0;
		int64_t MaxStrikesPerFrame = 0;
		double HammerfallLanePx = 0.0;
		double HammerfallRangePx = 0.0;
		int64_t MaxDamageZones = 0;
		double AuraTrailDropSec = 0.0;
		double AuraTrailLifetimeSec = 0.0;
		double AuraTrailMinMovePx = 0.0;
		double FinaleReachPx = 0.0;
		double GridlockReachPx = 0.0;
		double EchoDropSec = 0.0;
		double EdgeBurnBandPx = 0.0;
		double AuraPulseIntervalSec = 0.0;
		double PrismShardsPerDetonation = 0.0;
		double PounceCraterLifetimeSec = 0.0;
		double BossDeathSequenceSec = 0.0;
		double AutoTravelPollSec = 0.0;
		double CutMaxRatio = 0.0;
		double CatnipBombDamageSeconds = 0.0;
		double CatnipBombRadius = 0.0;
		int64_t MaxTempKittens = 0;
		double KittenExplosionRadius = 0.0;
		double BossRelocateSpeed = 0.0;
		double BossDriftSpeed = 0.0;
		double BossDriftInterval = 0.0;
		double BossArenaInset = 0.0;
		double BossAddArrivePx = 0.0;
		double BossRelocateDistancePx = 0.0; // farther than this from its anchor the boss relocates; nearer, it drifts
		double BossWobbleXPx = 0.0;          // the idle drift's reach around the anchor
		double BossWobbleYPx = 0.0;
		double PlayWithFoodHpFraction = 0.0;
		double KittenRoamHeadingSec = 0.0;
		double KittenRoamTurnSpread = 0.0;
		int64_t KittenRebalanceMargin = 0;
		double PlayerStillEpsilonPx = 0.0;
		// @export defaults in Godot (no scene overrides them; the exporter refuses if one ever does).
		double EnemySpeedPxPerSec = 0.0;
		double EnemyWanderTurnRate = 0.0;     // radians/sec of random heading drift
		double EnemyFleeCentreBias = 0.0;     // a fleeing mouse arcs back toward the middle instead of leaving by the edge
		double EnemyFlierSpeedMultiplier = 0.0;
		double KittenSpeedPxPerSec = 0.0;
		double KittenOrbitRadiusPx = 0.0;     // below base_kitten_contact_radius, so orbiting still counts as contact
		double KittenRoamSpeedRatio = 0.0;    // roaming is slower than hunting: "spotted something" reads with no other cue
	};

	// Fills Out from CodeConstants.csv's rows (`id, value, ...`). Every field's id must appear exactly once and every id
	// must name a field; integer fields take integers only. Problems are appended to Errors; false when there were any.
	PAWCORE_API bool ReadCodeConstants(const CsvTable& Table, PcCodeConstants& Out, std::vector<std::string>& Errors);

	// Every numeric field as (CSV id, value), integers widened; the validator's range rule walks it.
	PAWCORE_API std::vector<std::pair<std::string, double>> CodeConstantNumbers(const PcCodeConstants& C);
}
