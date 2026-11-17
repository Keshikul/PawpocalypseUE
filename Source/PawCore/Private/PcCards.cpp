#include "PcCards.h"

#include "PcFieldRules.h"
#include "PcJobs.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		int64_t OwnedLevel(const PcProgress& P, const std::string& Id)
		{
			const auto It = P.CardLevels.find(Id);
			return It == P.CardLevels.end() ? 0 : It->second;
		}

		double CostAt(const PcCard& Card, int64_t Level)
		{
			if (Card.Cost == CardCost::NONE || Level < 1 || Level > static_cast<int64_t>(Card.CostValues.size()))
			{
				return 0.0;
			}
			return Card.CostValues[static_cast<size_t>(Level - 1)];
		}

		double ValueAt(const PcCard& Card, int64_t Level)
		{
			if (Level < 1 || Level > static_cast<int64_t>(Card.LevelValues.size()))
			{
				return 0.0;
			}
			return Card.LevelValues[static_cast<size_t>(Level - 1)];
		}

		// _rival_kitten_mark_owned(): Gilded Kitten and Lone Hunter paint DIFFERENT kittens.
		bool RivalKittenMarkOwned(const PcGameData& D, const PcProgress& P, const PcCard& Card)
		{
			CardEffect Rival = CardEffect::GOLDEN_KITTEN;
			if (Card.Effect == CardEffect::GOLDEN_KITTEN)
			{
				Rival = CardEffect::LONE_HUNTER;
			}
			else if (Card.Effect != CardEffect::LONE_HUNTER)
			{
				return false;
			}
			for (const PcCard& Other : D.Cards)
			{
				if (Other.Effect == Rival && OwnedLevel(P, Other.Id) > 0)
				{
					return true;
				}
			}
			return false;
		}

		bool CostLeavesSomething(const PcGameData& D, const PcProgress& P, const PcCard& Card)
		{
			if (Card.Cost != CardCost::KITTEN_COUNT)
			{
				return true;
			}
			const int64_t Current = OwnedLevel(P, Card.Id);
			const double Delta = CostAt(Card, Current + 1) - CostAt(Card, Current);
			if (Delta <= 0.0)
			{
				return true;
			}
			// One kitten has to survive the purchase; TWO when a rival mark is in play, or the second card would double up
			// on one animal or silently do nothing (designer, 2026-08-23).
			const double FloorKittens = RivalKittenMarkOwned(D, P, Card) ? 2.0 : 1.0;
			return static_cast<double>(KittenCount(D, P)) - Delta >= FloorKittens;
		}
	}

	double CatchesForLevel(const PcGameData& D, int64_t Level)
	{
		return D.Code.CardLevelCostBase * std::pow(D.Code.CardLevelCostGrowth, static_cast<double>(Level));
	}

	double TotalCatchesForLevel(const PcGameData& D, int64_t Level)
	{
		if (Level <= 0)
		{
			return 0.0;
		}
		const PcCodeConstants& C = D.Code;
		return C.CardLevelCostBase * (std::pow(C.CardLevelCostGrowth, static_cast<double>(Level)) - 1.0) / (C.CardLevelCostGrowth - 1.0);
	}

	PcCardRequirement CardEffectRequirement(CardEffect Effect)
	{
		switch (Effect)
		{
		case CardEffect::LONE_HUNTER: return PcCardRequirement::Kitten;
		case CardEffect::AURA_TRAIL: return PcCardRequirement::Aura;
		case CardEffect::AURA_PULSE: return PcCardRequirement::Aura;
		case CardEffect::AURA_SLOW: return PcCardRequirement::Aura;
		case CardEffect::EDGE_BURN: return PcCardRequirement::Aura;
		case CardEffect::POUNCE_CHAIN: return PcCardRequirement::Pounce;
		case CardEffect::POUNCE_CRATER: return PcCardRequirement::Pounce;
		case CardEffect::POUNCE_KILL_REFUND: return PcCardRequirement::Pounce;
		case CardEffect::POOP_DURATION: return PcCardRequirement::Poop;
		case CardEffect::POOP_TRAIL: return PcCardRequirement::Poop;
		case CardEffect::OVERKILL_TARGETS: return PcCardRequirement::Overkill;
		case CardEffect::KITTEN_KILL_EXPLODES: return PcCardRequirement::Kitten;
		case CardEffect::KITTEN_DRAG: return PcCardRequirement::Kitten;
		case CardEffect::GOLDEN_KITTEN: return PcCardRequirement::Kitten;
		case CardEffect::KITTEN_FROM_SIGNATURE: return PcCardRequirement::Kitten;
		case CardEffect::KITTEN_MIMIC_POUNCE: return PcCardRequirement::KittenPounce;
		default: return PcCardRequirement::None;
		}
	}

	PcCardRequirement CardCostRequirement(CardCost Cost)
	{
		switch (Cost)
		{
		case CardCost::AURA_RADIUS: return PcCardRequirement::Aura;
		case CardCost::POUNCE_DAMAGE: return PcCardRequirement::Pounce;
		case CardCost::KITTEN_COUNT: return PcCardRequirement::Kitten;
		default: return PcCardRequirement::None;
		}
	}

	bool CardRequirementMet(const PcGameData& D, const PcProgress& P, PcCardRequirement Requirement)
	{
		// No default: every value is named, and one added later and forgotten falls to the end, where the test
		// "Cards: each requirement is met by its own unlock and no other" fails on it (MSVC's C4062 is off by default).
		switch (Requirement)
		{
		case PcCardRequirement::None: return true;
		case PcCardRequirement::Aura: return BranchLevel(P, "unlock_aura") > 0;
		case PcCardRequirement::Pounce: return BranchLevel(P, "unlock_pounce") > 0;
		case PcCardRequirement::Overkill: return BranchLevel(P, "unlock_overkill") > 0;
		// Rainbow Poop makes the puddles, so the puddle cards open with it (F19, designer 2026-11-03; this read the node
		// `unicorn`, which is Rainbow Gut).
		case PcCardRequirement::Poop: return PoopUnlocked(P);
		case PcCardRequirement::Kitten: return KittenCount(D, P) > 0;
		case PcCardRequirement::KittenPounce: return KittenCount(D, P) > 0 && BranchLevel(P, "unlock_pounce") > 0;
		case PcCardRequirement::Count: break;
		}
		return true;
	}

	bool IsCardEligible(const PcGameData& D, const PcProgress& P, const PcCard& Card)
	{
		if (P.BanishedCards.count(Card.Id))
		{
			return false;
		}
		if (OwnedLevel(P, Card.Id) >= static_cast<int64_t>(Card.LevelValues.size()))
		{
			return false;
		}
		if (!CardRequirementMet(D, P, CardEffectRequirement(Card.Effect)))
		{
			return false;
		}
		if (!CardRequirementMet(D, P, CardCostRequirement(Card.Cost)))
		{
			return false;
		}
		return CostLeavesSomething(D, P, Card);
	}

	PcCardHint CardUnlockHint(const PcGameData& D, const PcProgress& P, const PcCard& Card)
	{
		PcCardHint Hint;
		if (P.BanishedCards.count(Card.Id))
		{
			Hint.Kind = PcCardHintKind::Banished;
			return Hint;
		}
		if (OwnedLevel(P, Card.Id) >= static_cast<int64_t>(Card.LevelValues.size()))
		{
			Hint.Kind = PcCardHintKind::MaxLevel;
			return Hint;
		}
		for (const PcCardRequirement Requirement : { CardEffectRequirement(Card.Effect), CardCostRequirement(Card.Cost) })
		{
			if (!CardRequirementMet(D, P, Requirement))
			{
				Hint.Kind = PcCardHintKind::Requirement;
				Hint.Requirement = Requirement;
				return Hint;
			}
		}
		if (!CostLeavesSomething(D, P, Card))
		{
			Hint.Kind = PcCardHintKind::NotEnoughKittens;
		}
		return Hint;
	}

	std::vector<const PcCard*> EligibleCards(const PcGameData& D, const PcProgress& P)
	{
		std::vector<const PcCard*> Out;
		for (const PcCard& Card : D.Cards)
		{
			if (IsCardEligible(D, P, Card))
			{
				Out.push_back(&Card);
			}
		}
		return Out;
	}

	int64_t CardMaxLevel(const PcCard& Card)
	{
		return static_cast<int64_t>(Card.LevelValues.size());
	}

	double CardValueAt(const PcCard& Card, int64_t Level)
	{
		return ValueAt(Card, Level);
	}

	double CardCostAt(const PcCard& Card, int64_t Level)
	{
		return CostAt(Card, Level);
	}

	int64_t CardLevel(const PcProgress& P, const std::string& Id)
	{
		return OwnedLevel(P, Id);
	}

	int64_t HandSize(const PcGameData& D, const PcProgress& P)
	{
		// The job widens every hand (the card economy is the one build axis the shop cannot touch); Curiosity's card is
		// permanent, the job's is a job you can quit.
		const int64_t RuleExtra = BranchEffect(D, P, "curiosity") > 0.0 ? 1 : 0;
		return D.Code.BaseHandSize + P.ExtraChoices + JobExtraCards(D, P) + RuleExtra;
	}

	double CardEffectValue(const PcGameData& D, const PcProgress& P, CardEffect Effect)
	{
		double Total = 0.0;
		for (const PcCard& Card : D.Cards)
		{
			if (Card.Effect != Effect)
			{
				continue;
			}
			const int64_t Level = OwnedLevel(P, Card.Id);
			if (Level > 0)
			{
				Total += ValueAt(Card, Level);
			}
		}
		return Total;
	}

	double CardEffectBestLow(const PcGameData& D, const PcProgress& P, CardEffect Effect)
	{
		double Best = 0.0;
		for (const PcCard& Card : D.Cards)
		{
			if (Card.Effect != Effect)
			{
				continue;
			}
			const int64_t Level = OwnedLevel(P, Card.Id);
			if (Level <= 0)
			{
				continue;
			}
			const double V = ValueAt(Card, Level);
			if (Best <= 0.0 || V < Best)
			{
				Best = V;
			}
		}
		return Best;
	}

	PcCatchesStep AddKillCatches(const PcGameData& D, const PcProgress& P, double Amount, bool bHandPending)
	{
		PcCatchesStep Out;
		Out.Level = P.CatchesLevel;
		Out.Points = P.CatchesPoints;
		if (Amount <= 0.0)
		{
			return Out;
		}
		Out.Points += Amount;
		if (bHandPending)
		{
			// A hand is up: XP still banks, the level waits for the choice.
			return Out;
		}
		if (Out.Points < TotalCatchesForLevel(D, Out.Level + 1))
		{
			return Out;
		}
		Out.Level += 1;
		Out.bLevelled = true;
		// Second Opinion: rerolls once per level, i.e. once per hand (designer, 2026-10-29; Godot fixed the same day,
		// they had been granted on every kill that did NOT level).
		Out.RerollsGranted = std::max<int64_t>(0, static_cast<int64_t>(BranchEffect(D, P, "second_opinion")));
		return Out;
	}
}
