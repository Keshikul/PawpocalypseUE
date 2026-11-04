#include "PcTreeText.h"

#include "PcFormat.h"
#include "PcTree.h"
#include "PcTreeWording.h"

#include <algorithm>
#include <cstring>

namespace Pc
{
	namespace
	{
		// effect_at_level / extra_effect_at_level (skill_branch_data.gd): the ladder's rung, the last one past its end.
		double LadderAt(const std::vector<double>& Values, int64_t Level, int64_t Max)
		{
			if (Level <= 0 || Values.empty())
			{
				return 0.0;
			}
			const int64_t Rung = std::min<int64_t>(std::min<int64_t>(Level, Max), static_cast<int64_t>(Values.size()));
			return Values[static_cast<size_t>(Rung - 1)];
		}

		double ExtraAt(const std::vector<double>& Values, int64_t Level)
		{
			if (Level <= 0 || Values.empty())
			{
				return 0.0;
			}
			return Values[static_cast<size_t>(std::min<int64_t>(Level, static_cast<int64_t>(Values.size())) - 1)];
		}

		// The GDScript expression each phrase's number is (gen_tree_tables.py NUMBERS).
		std::string Number(PcTreeNumber N, double Value)
		{
			switch (N)
			{
			case PcTreeNumber::R100: return RoundiLikeGodot(Value * 100.0);
			case PcTreeNumber::Int: return IntLikeGodot(Value);
			case PcTreeNumber::F0: return FixedLikeGodot(Value, 0);
			case PcTreeNumber::F1: return FixedLikeGodot(Value, 1);
			case PcTreeNumber::F2: return FixedLikeGodot(Value, 2);
			case PcTreeNumber::F3: return FixedLikeGodot(Value, 3);
			case PcTreeNumber::F1x100: return FixedLikeGodot(Value * 100.0, 1);
			case PcTreeNumber::F0PerMinute: return FixedLikeGodot(60.0 / std::max(Value, 0.001), 0);
			case PcTreeNumber::RInverse: return RoundiLikeGodot(1.0 / std::max(Value, 0.001));
			default: return std::string();
			}
		}

		// _describe(): the first case that fits, a branch's own case before the effect's general one (source order).
		bool Describe(const PcSkillBranch& B, double Value, PcTreePhrase& Out)
		{
			for (const PcTreeWording& W : TreeWordings)
			{
				if (W.Effect != B.EffectType || (W.Branch[0] != '\0' && B.Id != W.Branch))
				{
					continue;
				}
				if (W.Plural[0] != '\0')
				{
					// tr_n(): the English rule, one when the count is exactly 1.
					const bool bOne = static_cast<int64_t>(Value) == 1;
					if ((std::strcmp(W.Plural, "one") == 0) != bOne)
					{
						continue;
					}
				}
				Out.Key = W.Key;
				Out.Arg = W.Arg;
				Out.Number = Number(W.Number, Value);
				return true;
			}
			return false;
		}
	}

	std::vector<PcTreePhrase> TreeEffectPhrases(const PcGameData& D, const std::string& Id, int64_t Level)
	{
		std::vector<PcTreePhrase> Out;
		const PcSkillBranch* B = D.FindBranch(Id);
		if (!B)
		{
			return Out;
		}
		const int64_t Max = BranchMaxLevel(*B);
		if (Level <= 0 || Level > Max)
		{
			return Out;
		}
		PcTreePhrase Primary;
		if (Describe(*B, LadderAt(B->LevelEffects, Level, Max), Primary))
		{
			Out.push_back(Primary);
		}
		for (const PcTreeChannelWording& C : TreeChannelWordings)
		{
			const auto It = B->ExtraEffects.find(C.Channel);
			if (It == B->ExtraEffects.end())
			{
				continue;
			}
			PcTreePhrase P;
			P.Key = C.Key;
			P.Arg = C.Arg;
			P.Number = Number(C.Number, ExtraAt(It->second, Level));
			Out.push_back(P);
		}
		return Out;
	}

	PcTreeTip TreeTip(const PcGameData& D, const PcProgress& P, const std::string& Id, bool bSpendMode)
	{
		PcTreeTip T;
		const PcSkillBranch* B = D.FindBranch(Id);
		if (!B)
		{
			return T;
		}
		T.bKnown = true;
		T.bUnlock = B->bIsUnlock;
		T.Owned = BranchLevel(P, Id);
		T.Max = BranchMaxLevel(*B);
		T.bMaxed = T.Owned >= T.Max;
		const bool bAvailable = IsBranchAvailable(D, P, Id);
		if (!T.bMaxed)
		{
			T.bShowNow = T.Owned > 0;
			T.Next = T.Owned + 1;
			const double Cost = NextBranchCost(D, P, Id);
			T.Cost = static_cast<int64_t>(Cost);
			if (!bAvailable)
			{
				// Say WHY it cannot be bought: the rival that closed it, the parent still unbought, else the Reawaken gate.
				T.Rival = BlockingRival(D, P, Id);
				if (!T.Rival.empty())
				{
					T.Reason = PcTreeTipReason::Closed;
				}
				else if (B->bParentIsRequired && !B->Parent.empty() && BranchLevel(P, B->Parent) <= 0)
				{
					T.Reason = PcTreeTipReason::NeedsParent;
					T.Parent = B->Parent;
				}
				else
				{
					T.Reason = PcTreeTipReason::ReawakenGate;
					T.Gate = BranchReawakenGate(D, Id);
				}
			}
			else if (!bSpendMode)
			{
				T.Reason = PcTreeTipReason::BrowseOnly;
			}
			else if (P.Stardust < Cost)
			{
				T.Reason = PcTreeTipReason::Short;
				T.Have = static_cast<int64_t>(P.Stardust);
			}
		}
		T.bBuyPrompt = bSpendMode && !T.bMaxed && bAvailable && CanAffordNextBranch(D, P, Id);
		return T;
	}
}
