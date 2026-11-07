#pragma once

// CombatStats rebuilt from progress (docs/03 §2 PcStats): the port of GameState._rebuild_stats() and the helpers it reads
// (upgrade effects and caps, skill-node effects and channels, jobs with Union Rules, card costs, Patience, Dent, Feral
// Instinct, Severance, the compounding nodes, the evolution ladder and the Reawaken multiplier). Pure: the same progress
// snapshot always gives the same stats.

#include "PcCombat.h"
#include "PcCore.h"
#include "PcData.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace Pc
{
	// The parts of the game state the stats are a function of (GameState + SkillTreeManager + CardManager fields).
	struct PcProgress
	{
		std::map<std::string, int64_t> UpgradeLevels;
		std::map<std::string, int64_t> BranchLevels;
		std::map<std::string, int64_t> CardLevels; // owned cards' levels (CardManager.owned)
		std::string RunJob;                        // the job in effect this run (GameState.run_job)
		std::string RunJobSecond;                  // Union Rules' second job
		int64_t ReawakenCount = 0;
		int64_t DeepestBiomeIndex = 0;             // 0 in the Backyard; floors_reached() = max(this, 0)
		std::set<std::string> BossesDefeated;      // biome ids of the boss ledger
		bool bFinalBossDefeated = false;
		double StardustGrantedEver = 0.0;          // the Reawaken multiplier keys off Stardust EVER granted (CLAUDE.md)
		int64_t WavesHere = 0;                     // Reputation
		int64_t CardsTakenThisRun = 0;             // Grudge Ledger
		int64_t KillsThisCycle = 0;                // Body Count

		// --- the run's economy state (PcEconomy reads these; RebuildStats does not) ---
		std::string CurrentBiomeId;                       // the biome the player stands in ("" before the first)
		int64_t TierKillCount = 0;                        // kills in the current biome this visit
		std::map<std::string, int64_t> TierKillsByBiome;  // banked kills of the biomes left (bait is earned per biome)
		std::map<std::string, int64_t> BaitSpentByBiome;
		std::set<std::string> BossesClearedThisCycle;     // PASSAGE re-locks every cycle; POWER (BossesDefeated) never does
		int64_t CurrentWaveIndex = 0;                     // cached, so a mouse keeps the wave it spawned in
		int64_t KillsThisWave = 0;                        // Fence
		double LifetimeCatnipThisCycle = 0.0;             // biome gates read this
		double LifetimeCatnipEver = 0.0;                  // the Stardust grant reads this (split-invariant)

		// --- the run's cards (CardManager; PcCards reads these) ---
		std::set<std::string> BanishedCards;              // CardManager.banished: never offered again this run
		int64_t ExtraChoices = 0;                         // CardManager.extra_choices (the EXTRA_CHOICE card)
		int64_t CatchesLevel = 0;                         // CardManager.catches_level
		double CatchesPoints = 0.0;                       // CardManager.catches_points (XP banked)
		int64_t Rerolls = 0;                              // CardManager.rerolls (Overtime's survive a Reawaken)
		int64_t Banishes = 0;                             // CardManager.banishes
		int64_t FixationHands = 0;                        // CardManager.fixation_hands: hands dealt from owned cards
		std::vector<std::string> Hand;                    // CardManager._hand: the hand up, in deal order (empty = none)
		std::vector<std::string> PromisedCards;           // CardManager._promised: Blueprint's cards for the NEXT hand
		std::string PrototypeCard;                        // CardManager._prototype_card: offered above its level

		// --- the wallet and the jobs the tree handed over (PcTree reads and writes these) ---
		double Stardust = 0.0;                            // GameState.stardust (spendable; StardustGrantedEver never falls)
		std::set<std::string> JobsHeld;                   // GameState.jobs_held: bought once, kept for good
		std::string CurrentJob;                           // GameState.current_job: the standing preference (PcJobs)
		bool bRunJobChosen = false;                       // GameState.run_job_chosen: the picker has been answered this run

		// --- the rest of what PcProgress (the module) changes ---
		double Catnip = 0.0;                              // GameState.catnip (spendable)
		int64_t LifetimeKills = 0;                        // never resets but on a new game
		int64_t KillsSinceTithe = 0;                      // Levy's counter
		double InfestationBonus = 0.0;                    // spawn rate Infestation has added, decaying
		double PendingSecondHelping = 0.0;                // Second Helping's deferred half of the last Reawaken
		// GameState.reawaken_in_progress: from a Reawaken until the run lands in its starting biome (ceremony, spend step,
		// job picker, travel). The HUD sets and clears it; saved, so a load in between resumes at the spend step (F29).
		bool bReawakenInProgress = false;
		double PlaySecondsThisCycle = 0.0;
		double TotalPlaySeconds = 0.0;
		double FinalBossSeconds = 0.0;                    // the play clock at the FIRST final-boss clear (0 = not yet)
		double RealtimeInterestElapsed = 0.0;             // seconds toward the next Compound Interest payment
		// The cat's stillness (Player._process writes them; never saved: a saved value would open with a bonus not earned).
		double PlayerStillSeconds = 0.0;                  // Carry reads it; reset the moment the cat moves
		double LunchSeconds = 0.0;                        // Long Lunch's bank: only a kill spends it, moving does not

		// --- the achievements (PcAchievements; AchievementManager.earned / .claimed) ---
		std::set<std::string> AchievementsEarned;         // only ever gains entries, but for a load or a new game
		std::set<std::string> AchievementsClaimed;        // the reward TAKEN (earning and collecting are two events)
	};

	// --- the pieces, exposed because the shop, the tree UI and the mirror read them too ---------------------------------

	PAWCORE_API int64_t UpgradeLevel(const PcProgress& P, const std::string& Id);
	PAWCORE_API int64_t BranchLevel(const PcProgress& P, const std::string& Id);
	// UpgradeData: effect_per_level ^ level (multiplicative) or effect_per_level x level (additive).
	PAWCORE_API double EffectTotal(const PcUpgrade& U, int64_t Level);
	// The cap after the per-floor and per-Reawaken growth; -1 for an uncapped line. EVERY read of a cap that decides what a
	// player may reach goes through here (Godot had five scattered reads; a bonus applied to four of five is a cap that
	// silently disagrees with the shop button reporting it).
	PAWCORE_API double EffectiveCap(const PcUpgrade& U, const PcProgress& P);
	PAWCORE_API double CappedEffect(const PcUpgrade& U, int64_t Level, const PcProgress& P);
	// SkillTreeManager.get_effect(id, channel): the level's effect (or channel ladder entry), 0 at level 0 or unknown id.
	PAWCORE_API double BranchEffect(const PcGameData& D, const PcProgress& P, const std::string& Id, const std::string& Channel = std::string());
	// 1 + one per boss of the ladder beaten in order, + 1 for the final boss, at most 7.
	PAWCORE_API int64_t EvolutionStage(const PcGameData& D, const PcProgress& P);
	// (1 + stardust_granted_ever / scale) ^ exponent: a power law that keeps paying forever and never explodes.
	PAWCORE_API double ReawakenMultiplier(const PcReawakenConstants& R, double StardustGrantedEver);
	// Product of the owned cards' costs of one kind.
	PAWCORE_API double CardCostMultiplier(const PcGameData& D, const PcProgress& P, CardCost Cost);
	// Sum of the owned cards' costs of one kind, for costs counted in whole units taken (kittens).
	PAWCORE_API double CardCostTotal(const PcGameData& D, const PcProgress& P, CardCost Cost);

	// GameState.get_kitten_count(): the shop line plus Vitality, minus kittens a card took, never below zero.
	PAWCORE_API int64_t KittenCount(const PcGameData& D, const PcProgress& P);
	// GameState.get_currency_find_multiplier(): the capped shop line times the Catnip-yield card costs.
	PAWCORE_API double CurrencyFindMultiplier(const PcGameData& D, const PcProgress& P);
	// UpgradeData.cost_for_level(): base_cost x cost_growth ^ level, the price of the NEXT level when at `Level`.
	PAWCORE_API double UpgradeCost(const PcUpgrade& U, int64_t Level);
	// SkillBranchData.cost_for_next_level(): the authored price, -1 at max level (Godot's sentinel, kept).
	PAWCORE_API double BranchCost(const PcSkillBranch& B, int64_t Level);

	// The whole snapshot, field for field as GameState._rebuild_stats() builds it.
	PAWCORE_API PcCombatStats RebuildStats(const PcGameData& D, const PcProgress& P);
}
