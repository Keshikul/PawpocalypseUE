#pragma once

// The card DRAW (docs/03 §2 PcCards): XP gained and the level-up it may trigger, the weighted deal without replacement,
// Blueprint's promised cards, Prototype's marked card, Fixation, auto-pick, reroll, banish and taking a card. A port of
// CardManager's stateful half (card_manager.gd); the pure half (eligibility, hand size, effect totals) is PcCards.h. The
// state lives in PcProgress (the struct): Hand, PromisedCards, PrototypeCard, Rerolls, Banishes, FixationHands.
//
// Every random number comes from a PcCardRolls, asked in the order Godot asks its global randf()/randi(). The game feeds it
// from PcRng (PcRngCardRolls: deterministic per seed and deal); the tests feed it Godot's own stream, so a hand is compared
// card for card with the real game's.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	class PAWCORE_API PcCardRolls
	{
	public:
		virtual ~PcCardRolls() = default;
		// randf(): uniform in [0, 1].
		virtual double Unit() = 0;
		// randi() % Count, Count > 0.
		virtual int64_t Below(int64_t Count) = 0;
	};

	// The game's rolls: PcRng over (Seed, Deal, the n-th roll of this deal, Salt::Cards). A new Deal number per level-up,
	// reroll or banish keeps every deal independent and replayable.
	class PAWCORE_API PcRngCardRolls final : public PcCardRolls
	{
	public:
		PcRngCardRolls(uint64_t InSeed, uint64_t InDeal) : Seed(InSeed), Deal(InDeal) {}
		double Unit() override;
		int64_t Below(int64_t Count) override;

	private:
		uint64_t Seed;
		uint64_t Deal;
		uint64_t Rolls = 0;
	};

	// get_card_foresight() (Blueprint), get_card_prototype_levels() (Prototype), get_rerolls_per_level() (Second Opinion),
	// get_starting_rerolls() (Overtime): the tree nodes the deal reads, whole and never negative.
	PAWCORE_API int64_t CardForesight(const PcGameData& D, const PcProgress& P);
	PAWCORE_API int64_t CardPrototypeLevels(const PcGameData& D, const PcProgress& P);
	// has_auto_card(): the node owned AND the setting on (SettingsManager.auto_pick_card, the game module's).
	PAWCORE_API bool HasAutoCard(const PcGameData& D, const PcProgress& P, bool bAutoPickSetting);

	// draw_hand(): up to Size cards from the eligible pool, weighted (a negative weight counts as 0), without replacement,
	// one Unit() per card. Under Fixation the pool is the owned eligible cards when they can fill the hand. Reads only.
	PAWCORE_API std::vector<std::string> DrawHand(const PcGameData& D, const PcProgress& P, int64_t Size, PcCardRolls& R);

	struct PcDeal
	{
		bool bOffered = false;      // hand_offered: a hand is up for the player
		std::string AutoPicked;     // auto-pick took this card instead (the screen never opens)
		int64_t AutoPickedLevel = 0;
	};
	// _offer_hand(): the promised cards first, the rest drawn around them (deduped), Fixation spent on the deal; then
	// auto-pick, or Blueprint's promise and Prototype's mark. An exhausted deck deals nothing.
	PAWCORE_API PcDeal OfferHand(const PcGameData& D, PcProgress& P, PcCardRolls& R, bool bAutoPickSetting);

	struct PcCatchesGain
	{
		bool bLevelled = false;
		double InterestPaid = 0.0; // Catnip Interest, paid before the hand
		PcDeal Deal;
	};
	// add_kill_catches(): XP banked; a level-up (never while a hand is up) hands over Second Opinion's rerolls, pays the
	// interest card and deals.
	PAWCORE_API PcCatchesGain GainCatches(const PcGameData& D, PcProgress& P, double Amount, PcCardRolls& R,
		bool bAutoPickSetting);

	// prototype_bonus(): extra levels the card arrives at, 0 unless it is the marked one.
	PAWCORE_API int64_t PrototypeBonus(const PcGameData& D, const PcProgress& P, const std::string& CardId);

	// reroll_hand(): a fresh hand for one reroll; Prototype marks a card of the NEW hand and Blueprint promises again
	// (_mark_and_promise(): F11, designer 2026-11-03; they used to keep the thrown-away hand's mark and promise).
	PAWCORE_API bool RerollHand(const PcGameData& D, PcProgress& P, PcCardRolls& R);

	struct PcTake
	{
		bool bTaken = false;
		int64_t NewLevel = 0;
	};
	// choose(): only a card in the hand. The level is one up plus Prototype's bonus, capped at the card's max; the hand
	// closes; Grudge Ledger counts it; the deck cards (reroll, extra choice, Fixation, banish) apply here.
	PAWCORE_API PcTake ChooseCard(const PcGameData& D, PcProgress& P, const std::string& CardId);

	// banish(): out of this run's deck.
	PAWCORE_API void BanishCard(PcProgress& P, const std::string& CardId);
	// banish_from_hand(): spends a banish on a card in the hand and re-deals; the level is NOT consumed. True also when
	// the re-deal comes back empty (the deck banished dry: the pause is released).
	PAWCORE_API bool BanishFromHand(const PcGameData& D, PcProgress& P, const std::string& CardId, PcCardRolls& R);
}
