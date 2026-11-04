#pragma once

// The skill tree's rules (docs/03 §2 PcTree): what the tree will sell, at what price, what closes a door, and what a
// purchase does. A port of Godot's SkillTreeManager (skill_tree_manager.gd). The tree never resets (CLAUDE.md); the
// branch levels live in PcProgress::BranchLevels, the wallet in PcProgress::Stardust.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	// SkillBranchData.max_level(): the ladder's length.
	PAWCORE_API int64_t BranchMaxLevel(const PcSkillBranch& B);
	// is_max_level(): false for an unknown id.
	PAWCORE_API bool IsBranchMaxLevel(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// get_next_cost(): the next level's price, -1 at max level, +infinity for an unknown id (Godot's sentinels, kept).
	PAWCORE_API double NextBranchCost(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// can_afford_next(): a real price the wallet covers.
	PAWCORE_API bool CanAffordNextBranch(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// branch_for_job(): the branch that hands over `Job`, "" if none (the data names the job on the branch only).
	PAWCORE_API std::string BranchForJob(const PcGameData& D, const std::string& Job);
	// blocking_rival(): the owned rival that closed this branch, "" if none has.
	PAWCORE_API std::string BlockingRival(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// is_branch_available() / _is_branch_purchasable(): the required parent owned, no rival owned, the gating boss beaten,
	// and the Reawaken gate met (the three ability unlocks read their own Reawaken constants). Says nothing about price or
	// max level. True for an unknown id, as in Godot; the price (+infinity) is what stops it.
	PAWCORE_API bool IsBranchAvailable(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// branch_reawaken_gate(): the Reawaken the branch becomes buyable on, for "locked until Reawaken N".
	PAWCORE_API int64_t BranchReawakenGate(const PcGameData& D, const std::string& Id);

	// purchase_next_level(): on success the Stardust is spent, the level rises, a job node hands its job over (once), and an
	// automation node asks for its own switch to be turned on (the settings live outside PawCore).
	struct PcBranchPurchase
	{
		bool bBought = false;
		double Cost = 0.0;
		std::string JobOffered;     // "" unless this purchase handed over a job not held before
		bool bAutoTravelOn = false; // SettingsManager.set_auto_travel(true)
		bool bAutoCardOn = false;   // SettingsManager.set_auto_pick_card(true)
	};
	PAWCORE_API PcBranchPurchase PurchaseNextBranchLevel(const PcGameData& D, PcProgress& P, const std::string& Id);
	// grant_level(): a free level (Muscle Memory II); never lowers, clamps to the ladder; unknown ids are ignored.
	PAWCORE_API void GrantBranchLevel(const PcGameData& D, PcProgress& P, const std::string& Id, int64_t Level);

	// trunk_branches(): the branches naming no parent, sorted.
	PAWCORE_API std::vector<std::string> TrunkBranches(const PcGameData& D);
	// get_max_possible_vitality_bonus(): Vitality's top level, truncated (sizes the kitten pool).
	PAWCORE_API int64_t MaxVitalityBonus(const PcGameData& D);
}
