#pragma once

// The level-up cards (docs/03 §2 PcCards): the Catches XP curve, which cards may be offered, the hand size, what owned cards
// add up to. A port of the pure halves of Godot's CardManager (card_manager.gd). Dealing, rerolls, banishes and taking a
// card are PcCardDraw.h.

#include "PcCore.h"
#include "PcData.h"
#include "PcEnums.h"
#include "PcStats.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	// CardData.max_level() / value_at() / cost_at(): levels are 1-based; outside 1..max the value and the cost are 0.
	PAWCORE_API int64_t CardMaxLevel(const PcCard& Card);
	PAWCORE_API double CardValueAt(const PcCard& Card, int64_t Level);
	PAWCORE_API double CardCostAt(const PcCard& Card, int64_t Level);
	// level_of(): the level owned this run, 0 for none.
	PAWCORE_API int64_t CardLevel(const PcProgress& P, const std::string& Id);

	// catches_for_level(): XP for ONE level: base x growth ^ level.
	PAWCORE_API double CatchesForLevel(const PcGameData& D, int64_t Level);
	// total_catches_for_level(): XP to reach `Level` from zero (the series pawsim's catches_levels_for_points inverts).
	PAWCORE_API double TotalCatchesForLevel(const PcGameData& D, int64_t Level);

	// EFFECT_REQUIRES / COST_REQUIRES: what a card needs the player to own before it may be offered ("" = nothing).
	PAWCORE_API const char* CardEffectRequirement(CardEffect Effect);
	PAWCORE_API const char* CardCostRequirement(CardCost Cost);
	// _requirement_met(): "aura", "pounce", "overkill", "poop", "kitten", "kitten_pounce"; anything else is met.
	PAWCORE_API bool CardRequirementMet(const PcGameData& D, const PcProgress& P, const std::string& Requirement);
	// eligible_cards(): not banished, not maxed, its effect and its cost have something to act on, and a kitten-cost card
	// leaves a kitten (two when the rival kitten mark is owned).
	PAWCORE_API bool IsCardEligible(const PcGameData& D, const PcProgress& P, const PcCard& Card);
	PAWCORE_API std::vector<const PcCard*> EligibleCards(const PcGameData& D, const PcProgress& P);
	// unlock_hint(): why a card is not on offer, walking the gates in eligible_cards()'s order, so the step named is the
	// step actually blocking. None = nothing stands in the way ("" in Godot).
	enum class PcCardHintKind : uint8_t
	{
		None,
		Banished,         // "Removed from this run."
		MaxLevel,         // "Already at its highest level."
		Requirement,      // REQUIREMENT_TEXT[Requirement]: its effect's, else its cost's, requirement is unmet
		NotEnoughKittens, // "You do not have enough kittens to pay its price yet."
	};
	struct PcCardHint
	{
		PcCardHintKind Kind = PcCardHintKind::None;
		std::string Requirement; // Requirement only: "aura", "pounce", "overkill", "poop", "kitten", "kitten_pounce"
	};
	PAWCORE_API PcCardHint CardUnlockHint(const PcGameData& D, const PcProgress& P, const PcCard& Card);
	// hand_size(): base_hand_size + extra choices + the job's extra cards + Curiosity's one.
	PAWCORE_API int64_t HandSize(const PcGameData& D, const PcProgress& P);
	// effect_value(): the owned cards of one effect, their values summed.
	PAWCORE_API double CardEffectValue(const PcGameData& D, const PcProgress& P, CardEffect Effect);
	// effect_best_low(): for effects where lower is better, the lowest owned value (0 when none is owned).
	PAWCORE_API double CardEffectBestLow(const PcGameData& D, const PcProgress& P, CardEffect Effect);

	// add_kill_catches(), as a pure step: bank `Amount` XP and report whether a level was reached and how many rerolls the
	// kill handed over. A pending hand banks XP without consuming a level.
	struct PcCatchesStep
	{
		int64_t Level = 0;
		double Points = 0.0;
		bool bLevelled = false;
		int64_t RerollsGranted = 0;
	};
	PAWCORE_API PcCatchesStep AddKillCatches(const PcGameData& D, const PcProgress& P, double Amount, bool bHandPending);
}
