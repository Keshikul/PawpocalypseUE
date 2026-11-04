#pragma once

// The rules that CHANGE progress (docs/03 §2 PcProgress): the boss ledger and passage, the visual stage, Catnip and
// Stardust credited and spent, kills and the wave turning over, travel between biomes, bait, the shop purchase, the
// Reawaken, the play clocks, and a new game. A port of GameState's mutating functions (game_state.gd); the pure reads
// they lean on are PcEconomy's and PcStats'. The STATE itself is the struct Pc::PcProgress (PcStats.h).
//
// Godot's signals are not PawCore's: each function returns what happened, and the game module rebuilds stats, emits its
// delegates and plays sounds from that.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <string>

namespace Pc
{
	// --- the boss ledger ---------------------------------------------------------------------------------------------

	// is_boss_defeated(): POWER, the permanent record the evolution ladder and the tree's boss keys read.
	PAWCORE_API bool IsBossDefeated(const PcProgress& P, const std::string& TierId);
	// is_boss_cleared_this_cycle(): PASSAGE, re-locked by every Reawaken.
	PAWCORE_API bool IsBossClearedThisCycle(const PcProgress& P, const std::string& TierId);
	struct PcBossMark
	{
		bool bNew = false;      // mark_boss_defeated()'s return: a first-ever clear (victory feedback fires once)
		bool bDefeated = false; // boss_defeated was emitted: a first clear, or the first rematch of this cycle
	};
	// mark_boss_defeated(): a first clear records power and passage (and stops the final-boss clock, first time only); a
	// rematch already beaten in an earlier cycle opens this cycle's passage and nothing else. Grants nothing: the boss holds
	// the key to its job's tree node, it does not hand the job over.
	PAWCORE_API PcBossMark MarkBossDefeated(const PcGameData& D, PcProgress& P, const std::string& TierId);
	// TierController._on_boss_defeated(), its rule half: the reward credited as a kill payout, the ledger, and the ending
	// flag when the boss stands in the last boss biome.
	PAWCORE_API PcBossMark RecordBossDefeat(const PcGameData& D, PcProgress& P, const std::string& TierId, double RewardCatnip);
	// hud.gd _on_boss_defeated_for_victory(): THE ENDING comes up when boss_defeated is emitted (Mark.bDefeated: a first
	// clear, or the first rematch of a cycle) for the last boss realm. It shows FinalBossSeconds, the first clear's time.
	PAWCORE_API bool RaisesVictory(const PcGameData& D, const std::string& TierId, const PcBossMark& Mark);
	// is_unicorn_owned(): the unlock node bought (not merely sold).
	PAWCORE_API bool IsUnicornOwned(const PcProgress& P);
	// get_visual_stage(): the earned stage, or the first mounted one once the Unicorn is owned, whichever is higher. The
	// damage ladder (EvolutionStage) is untouched: the node is a purchase, the multipliers are earned.
	PAWCORE_API int64_t VisualStage(const PcGameData& D, const PcProgress& P);

	// --- currency ----------------------------------------------------------------------------------------------------

	// _credit_currency(): the bank and both lifetime counters rise together.
	PAWCORE_API void CreditCurrency(PcProgress& P, double Amount);
	// add_currency(): a KILL's payout, through every multiplier that prices a kill (KillCatnip), then credited.
	PAWCORE_API void AddCurrency(const PcGameData& D, PcProgress& P, double BaseReward);
	// credit_untouched_catnip(): Catnip that is not a kill (interest cards): floored, no minimum, no multipliers.
	PAWCORE_API void CreditUntouchedCatnip(PcProgress& P, double Amount);
	// grant_achievement_stardust(): spendable AND ever-granted (the Reawaken multiplier reads the latter).
	PAWCORE_API void GrantAchievementStardust(PcProgress& P, double Amount);
	// spend_stardust(): a plain subtraction (the tree checks affordability).
	PAWCORE_API void SpendStardust(PcProgress& P, double Amount);

	// --- kills and waves ---------------------------------------------------------------------------------------------

	struct PcKillStep
	{
		bool bWaveTurned = false; // wave_changed was emitted
		int64_t Wave = 0;         // the cached wave index after the kill
		double InterestPaid = 0.0; // Compound's payment on the turnover
		bool bStatsDirty = false; // Body Count's ten-thousand crossed or a wave turned: rebuild the stats
	};
	// increment_kill_count(): every kill counter, Infestation's bonus, and the wave turning over (Compound pays on the bank,
	// Reputation counts it, Fence's window closes). The ONE place that decides a wave turned over.
	PAWCORE_API PcKillStep IncrementKillCount(const PcGameData& D, PcProgress& P);
	// _pay_catnip_interest(): Compound, a share of the bank, credited without the kill multipliers (they priced a kill and
	// the bank already carries them). Returns what was paid.
	PAWCORE_API double PayCatnipInterest(const PcGameData& D, PcProgress& P);

	// --- travel ------------------------------------------------------------------------------------------------------

	// realm_index(): the biome's place in the ladder (0-based), -1 if unknown.
	PAWCORE_API int64_t RealmIndex(const PcGameData& D, const std::string& RealmId);
	// _bank_tier_kills(): the live kill counter written back into the per-biome map.
	PAWCORE_API void BankTierKills(PcProgress& P);
	struct PcArrival
	{
		bool bMoved = false;           // false: already standing there (a no-op)
		double SecondHelpingPaid = 0.0; // Stardust Second Helping's deferred half paid on this arrival
		bool bDeeper = false;          // a new deepest biome: the caps moved
	};
	// begin_biome(): bank the place left, restore the place reached, Reputation lost by leaving, the deepest biome noted.
	PAWCORE_API PcArrival BeginBiome(const PcGameData& D, PcProgress& P, const std::string& RealmId);

	// --- bait --------------------------------------------------------------------------------------------------------

	// is_bait_unlocked(): no bait still owed in the current biome.
	PAWCORE_API bool IsBaitUnlocked(const PcGameData& D, const PcProgress& P, const PcBoss& Boss);
	// can_afford_bait(): the boss's price (Audit's extra is not counted here, as in Godot).
	PAWCORE_API bool CanAffordBait(const PcProgress& P, const PcBoss& Boss);
	// buy_bait(): ALL-OR-NOTHING, in this order: refused while bait is owed or the Catnip is short; then the bait is spent
	// (unless Audit bought it) and the Catnip charged.
	PAWCORE_API bool BuyBait(const PcGameData& D, PcProgress& P, const PcBoss& Boss);

	// --- the shop ----------------------------------------------------------------------------------------------------

	// is_upgrade_visible(): lines gated on a tree node exist only once it is bought.
	PAWCORE_API bool IsUpgradeVisible(const PcProgress& P, const std::string& Id);
	// is_upgrade_at_cap(): false for an uncapped line or an unknown id.
	PAWCORE_API bool IsUpgradeAtCap(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// is_kitten_power_locked() / is_kitten_speed_locked(): no kitten, nothing to multiply.
	PAWCORE_API bool IsKittenPowerLocked(const PcGameData& D, const PcProgress& P);
	PAWCORE_API bool IsKittenSpeedLocked(const PcGameData& D, const PcProgress& P);
	// get_upgrade_cost(): the next level's price, +infinity for an unknown id.
	PAWCORE_API double UpgradeCostById(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// purchase_upgrade(): every rule that makes a purchase legal lives here (cap, visibility, the Kitten Power lock,
	// affordability within currency_epsilon); the bank is floored at 0.
	PAWCORE_API bool PurchaseUpgrade(const PcGameData& D, PcProgress& P, const std::string& Id);
	// purchase_upgrade_bulk(): purchase_upgrade() looped, stopping at the first refusal; Count < 0 = xMax. At most
	// bulk_max_steps. Returns the levels bought.
	PAWCORE_API int64_t PurchaseUpgradeBulk(const PcGameData& D, PcProgress& P, const std::string& Id, int64_t Count);
	struct PcBulkPreview
	{
		int64_t Levels = 0;
		double Cost = 0.0;
	};
	// preview_bulk_purchase(): the same walk without buying. Checks cap, budget and the step bound only (the button is
	// already hidden for visibility and the Kitten Power lock).
	PAWCORE_API PcBulkPreview PreviewBulkPurchase(const PcGameData& D, const PcProgress& P, const std::string& Id, int64_t Count);

	// --- Reawaken ----------------------------------------------------------------------------------------------------

	struct PcReawakenResult
	{
		bool bDone = false;
		double StardustGained = 0.0; // what landed now (Second Helping defers half)
	};
	// reawaken(): the grant (half deferred under Second Helping), Legacy's carryover and starting level, every per-run
	// counter, the passage and the banked kills, the job lock; the tree, the boss ledger and the Stardust stay. Also the
	// run's cards (CardManager.reset_run(), which Godot runs from the `reawakened` signal).
	PAWCORE_API PcReawakenResult Reawaken(const PcGameData& D, PcProgress& P, bool bForce = false);
	// CardManager.reset_run(): the run's cards, banishes, XP, extra choices, the hand, Blueprint's promise and Prototype's
	// mark; rerolls back to Overtime's starting count.
	PAWCORE_API void ResetRunCards(const PcGameData& D, PcProgress& P);

	// --- the clocks --------------------------------------------------------------------------------------------------

	// GameState._process(): both play clocks.
	PAWCORE_API void TickPlayClock(PcProgress& P, double Delta);
	// tick_realtime_interest(): Compound Interest, once per realtime_interest_period_sec; the clock resets while unowned.
	// Returns what was paid.
	PAWCORE_API double TickRealtimeInterest(const PcGameData& D, PcProgress& P, double Delta);
	// tick_infestation(): Infestation's bonus bleeds away.
	PAWCORE_API void TickInfestation(const PcGameData& D, PcProgress& P, double Delta);

	// --- a new game --------------------------------------------------------------------------------------------------

	// debug_reset_progress() (what the title screen's New Game calls): EVERYTHING a save carries and every per-run counter,
	// the tree, the jobs and the run's cards. A new game in an emptied slot must not inherit the game that was in memory.
	// Not the achievements: as in Godot, the title screen then empties them itself (ApplyAchievementSave, PcAchievements.h),
	// after the re-scan the reset's own signals ran.
	PAWCORE_API void NewGame(const PcGameData& D, PcProgress& P);
}
