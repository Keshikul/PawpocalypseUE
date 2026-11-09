#include "PcCardDraw.h"

#include "PcCards.h"
#include "PcProgress.h"
#include "PcRng.h"

#include <algorithm>

namespace Pc
{
	namespace
	{
		const PcCard* FindCard(const PcGameData& D, const std::string& Id)
		{
			for (const PcCard& Card : D.Cards)
			{
				if (Card.Id == Id)
				{
					return &Card;
				}
			}
			return nullptr;
		}

		bool Contains(const std::vector<std::string>& Ids, const std::string& Id)
		{
			return std::find(Ids.begin(), Ids.end(), Id) != Ids.end();
		}

		int64_t WholeEffect(const PcGameData& D, const PcProgress& P, const char* Branch)
		{
			return std::max<int64_t>(0, static_cast<int64_t>(BranchEffect(D, P, Branch)));
		}

		// Blueprint: the cards the NEXT hand opens with, never more than one short of a full hand.
		void PromiseNextHand(const PcGameData& D, PcProgress& P, PcCardRolls& R);

		// _mark_and_promise(): Prototype marks one card of the FINAL hand, then Blueprint promises the next. Every deal runs it:
		// the first deal, and since 2026-11-03 a reroll and a banish (F11: they kept the old hand's mark and promise); the
		// deal no longer promises twice (F10: the first promise was thrown away unseen).
		void MarkAndPromise(const PcGameData& D, PcProgress& P, PcCardRolls& R)
		{
			P.PrototypeCard.clear();
			if (CardPrototypeLevels(D, P) > 0 && !P.Hand.empty())
			{
				P.PrototypeCard = P.Hand[static_cast<size_t>(R.Below(static_cast<int64_t>(P.Hand.size())))];
			}
			PromiseNextHand(D, P, R);
		}

		void PromiseNextHand(const PcGameData& D, PcProgress& P, PcCardRolls& R)
		{
			P.PromisedCards.clear();
			const int64_t Want = std::min(CardForesight(D, P), std::max<int64_t>(HandSize(D, P) - 1, 0));
			if (Want <= 0)
			{
				return;
			}
			P.PromisedCards = DrawHand(D, P, Want, R);
		}
	}

	double PcRngCardRolls::Unit()
	{
		return RngUnit(Seed, Deal, Rolls++, Salt::Cards);
	}

	int64_t PcRngCardRolls::Below(int64_t Count)
	{
		return static_cast<int64_t>(RngBelow(static_cast<uint64_t>(Count), Seed, Deal, Rolls++, Salt::Cards));
	}

	int64_t CardForesight(const PcGameData& D, const PcProgress& P)
	{
		return WholeEffect(D, P, "blueprint");
	}

	int64_t CardPrototypeLevels(const PcGameData& D, const PcProgress& P)
	{
		return WholeEffect(D, P, "prototype");
	}

	bool HasAutoCard(const PcGameData& D, const PcProgress& P, bool bAutoPickSetting)
	{
		return BranchEffect(D, P, "auto_card") > 0.0 && bAutoPickSetting;
	}

	std::vector<std::string> DrawHand(const PcGameData& D, const PcProgress& P, int64_t Size, PcCardRolls& R)
	{
		std::vector<const PcCard*> Pool = EligibleCards(D, P);
		// Fixation: only owned cards, when there are enough of them to fill the hand (an early Fixation must not stall).
		if (P.FixationHands > 0)
		{
			std::vector<const PcCard*> Owned;
			for (const PcCard* Card : Pool)
			{
				if (CardLevel(P, Card->Id) > 0)
				{
					Owned.push_back(Card);
				}
			}
			if (static_cast<int64_t>(Owned.size()) >= Size)
			{
				Pool = Owned;
			}
		}
		std::vector<std::string> Out;
		while (static_cast<int64_t>(Out.size()) < Size && !Pool.empty())
		{
			double Total = 0.0;
			for (const PcCard* Card : Pool)
			{
				Total += std::max(Card->Weight, 0.0);
			}
			if (Total <= 0.0)
			{
				break;
			}
			double Roll = R.Unit() * Total;
			const PcCard* Picked = Pool.back();
			for (const PcCard* Card : Pool)
			{
				Roll -= std::max(Card->Weight, 0.0);
				if (Roll <= 0.0)
				{
					Picked = Card;
					break;
				}
			}
			Out.push_back(Picked->Id);
			Pool.erase(std::find(Pool.begin(), Pool.end(), Picked));
		}
		return Out;
	}

	PcDeal OfferHand(const PcGameData& D, PcProgress& P, PcCardRolls& R, bool bAutoPickSetting)
	{
		PcDeal Out;
		// The promised cards first; the filler is drawn full size and deduped (a promised card can be drawn again).
		P.Hand.clear();
		for (const std::string& Id : P.PromisedCards)
		{
			if (!Id.empty() && !Contains(P.Hand, Id))
			{
				P.Hand.push_back(Id);
			}
		}
		P.PromisedCards.clear();
		for (const std::string& Id : DrawHand(D, P, HandSize(D, P), R))
		{
			if (static_cast<int64_t>(P.Hand.size()) >= HandSize(D, P))
			{
				break;
			}
			if (!Contains(P.Hand, Id))
			{
				P.Hand.push_back(Id);
			}
		}
		// Spent on the DEAL, not on a reroll.
		if (P.FixationHands > 0)
		{
			P.FixationHands -= 1;
		}
		if (P.Hand.empty())
		{
			// The deck ran dry (every card maxed or banished): not an error.
			return Out;
		}
		if (HasAutoCard(D, P, bAutoPickSetting))
		{
			const std::string Id = P.Hand[static_cast<size_t>(R.Below(static_cast<int64_t>(P.Hand.size())))];
			const PcTake Take = ChooseCard(D, P, Id);
			Out.AutoPicked = Id;
			Out.AutoPickedLevel = Take.NewLevel;
			return Out;
		}
		MarkAndPromise(D, P, R);
		Out.bOffered = true;
		return Out;
	}

	PcCatchesGain GainCatches(const PcGameData& D, PcProgress& P, double Amount, PcCardRolls& R, bool bAutoPickSetting)
	{
		PcCatchesGain Out;
		const PcCatchesStep Step = AddKillCatches(D, P, Amount, !P.Hand.empty());
		P.CatchesPoints = Step.Points;
		if (!Step.bLevelled)
		{
			return Out;
		}
		P.CatchesLevel = Step.Level;
		P.Rerolls += Step.RerollsGranted;
		// Catnip Interest: paid BEFORE the hand, outside Currency Find.
		const double Rate = CardEffectValue(D, P, CardEffect::CATNIP_INTEREST);
		if (Rate > 0.0)
		{
			const double Before = P.Catnip;
			CreditUntouchedCatnip(P, P.Catnip * Rate);
			Out.InterestPaid = P.Catnip - Before;
		}
		Out.bLevelled = true;
		Out.Deal = OfferHand(D, P, R, bAutoPickSetting);
		return Out;
	}

	int64_t PrototypeBonus(const PcGameData& D, const PcProgress& P, const std::string& CardId)
	{
		if (CardId.empty() || CardId != P.PrototypeCard)
		{
			return 0;
		}
		return CardPrototypeLevels(D, P);
	}

	bool RerollHand(const PcGameData& D, PcProgress& P, PcCardRolls& R)
	{
		if (P.Hand.empty() || P.Rerolls <= 0)
		{
			return false;
		}
		P.Rerolls -= 1;
		P.Hand = DrawHand(D, P, HandSize(D, P), R);
		MarkAndPromise(D, P, R); // F11
		return true;
	}

	PcTake ChooseCard(const PcGameData& D, PcProgress& P, const std::string& CardId)
	{
		PcTake Out;
		const PcCard* Card = FindCard(D, CardId);
		if (!Card || !Contains(P.Hand, CardId))
		{
			return Out;
		}
		// Prototype's bonus is applied where the level is decided, so every effect reads the boosted level.
		int64_t NewLevel = CardLevel(P, CardId) + 1 + PrototypeBonus(D, P, CardId);
		if (CardMaxLevel(*Card) > 0)
		{
			NewLevel = std::min(NewLevel, CardMaxLevel(*Card));
		}
		P.CardLevels[CardId] = NewLevel;
		P.PrototypeCard.clear();
		P.Hand.clear();
		// Stacked Deck (grudge_ledger) counts cards TAKEN (a rerolled hand is not a card).
		P.CardsTakenThisRun += 1;
		const int64_t Value = static_cast<int64_t>(CardValueAt(*Card, NewLevel));
		switch (Card->Effect)
		{
		case CardEffect::REROLL:
			P.Rerolls += Value;
			break;
		case CardEffect::EXTRA_CHOICE:
			P.ExtraChoices = Value;
			break;
		case CardEffect::FIXATION_HANDS:
			P.FixationHands += Value;
			break;
		case CardEffect::BANISH:
			P.Banishes += Value;
			break;
		default:
			break;
		}
		Out.bTaken = true;
		Out.NewLevel = NewLevel;
		return Out;
	}

	void BanishCard(PcProgress& P, const std::string& CardId)
	{
		P.BanishedCards.insert(CardId);
	}

	bool BanishFromHand(const PcGameData& D, PcProgress& P, const std::string& CardId, PcCardRolls& R)
	{
		if (!FindCard(D, CardId) || !Contains(P.Hand, CardId) || P.Banishes <= 0)
		{
			return false;
		}
		P.Banishes -= 1;
		BanishCard(P, CardId);
		P.Hand = DrawHand(D, P, HandSize(D, P), R);
		if (!P.Hand.empty())
		{
			MarkAndPromise(D, P, R); // F11 (a deck banished dry has no hand to mark)
		}
		return true;
	}
}
