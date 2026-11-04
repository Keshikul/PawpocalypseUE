#pragma once

// The achievements (docs/03 §2 PcAchievements): a port of AchievementManager (achievement_manager.gd). An achievement is a
// threshold on a counter PcProgress already keeps; earning one and collecting its Stardust are two separate events
// (designer, 2026-08-19), and an achievement that pays nothing cannot be claimed (2026-08-20). The earned and claimed sets
// are part of PcProgress (PcStats.h); the roster is PcGameData::Achievements, in Godot's display order.
//
// Godot re-scans the roster from six signals (kills, currency, purchases, Reawaken, a boss, a branch level). PawCore has
// no signals: the game module calls EvaluateAchievements after every change that can move a counter, and shows the ids it
// returns (the HUD's badge and sound). A re-scan only ever adds, so calling it too often is harmless.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace Pc
{
	// _by_id(): the first roster entry with that id, nullptr for none.
	PAWCORE_API const PcAchievement* FindAchievement(const PcGameData& D, const std::string& Id);
	// progress_value(): the current value of the counter the achievement watches. Bosses count the ledger's entries (the
	// permanent record, not this cycle's passage); an upgrade or branch counts its level.
	PAWCORE_API double AchievementProgress(const PcGameData& D, const PcProgress& P, const PcAchievement& A);

	// is_earned(): the earned set as loaded, ids the roster no longer has included. earned_count(): only ids still in the
	// roster (F9, 2026-11-03: a save that earned an achievement removed since showed "40 / 39").
	PAWCORE_API bool IsAchievementEarned(const PcProgress& P, const std::string& Id);
	PAWCORE_API int64_t EarnedAchievementCount(const PcGameData& D, const PcProgress& P);
	// total_stardust_on_offer(): what the whole roster pays (pawsim reads it: the pool is part of the tuned economy).
	PAWCORE_API double TotalStardustOnOffer(const PcGameData& D);

	// evaluate(): every achievement not yet earned whose counter reached its threshold is earned. Returns them in roster
	// order (Godot's achievement_earned, one per id).
	PAWCORE_API std::vector<std::string> EvaluateAchievements(const PcGameData& D, PcProgress& P);

	// reward_for(): the Stardust it pays, 0 for an unknown id.
	PAWCORE_API double AchievementReward(const PcGameData& D, const std::string& Id);
	// is_claimable(): earned, not yet claimed, and paying more than nothing (with every reward at 0 the whole claim UI goes
	// quiet by itself).
	PAWCORE_API bool IsAchievementClaimable(const PcGameData& D, const PcProgress& P, const std::string& Id);
	PAWCORE_API bool IsAchievementClaimed(const PcProgress& P, const std::string& Id);
	// unclaimed_count() / unclaimed_stardust(): the Achievements tab's badge and what it is worth.
	PAWCORE_API int64_t UnclaimedAchievementCount(const PcGameData& D, const PcProgress& P);
	PAWCORE_API double UnclaimedStardust(const PcGameData& D, const PcProgress& P);

	// claim(): takes one reward through GrantAchievementStardust (spendable AND ever-granted, so it raises the Reawaken
	// multiplier). Marked claimed before the grant: a second claim pays nothing. Returns the Stardust paid, 0 if none.
	PAWCORE_API double ClaimAchievement(const PcGameData& D, PcProgress& P, const std::string& Id);
	// claim_all(): claim() over the roster in order. Returns the total paid.
	PAWCORE_API double ClaimAllAchievements(const PcGameData& D, PcProgress& P);

	// apply_save_data(): the saved sets replace the ones in memory, then a re-scan (a load restores counters without the
	// events that moved them). The title screen's New Game calls it with two empty sets after NewGame (PcProgress.h), and
	// so does a save too old to keep its progress. Returns what the re-scan earned.
	PAWCORE_API std::vector<std::string> ApplyAchievementSave(const PcGameData& D, PcProgress& P,
		const std::set<std::string>& Earned, const std::set<std::string>& Claimed);
}
