#include "PcTree.h"

#include <algorithm>
#include <limits>

namespace Pc
{
	int64_t BranchMaxLevel(const PcSkillBranch& B)
	{
		return static_cast<int64_t>(B.LevelCosts.size());
	}

	bool IsBranchMaxLevel(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		return B && BranchLevel(P, Id) >= BranchMaxLevel(*B);
	}

	double NextBranchCost(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		return B ? BranchCost(*B, BranchLevel(P, Id)) : std::numeric_limits<double>::infinity();
	}

	bool CanAffordNextBranch(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const double Cost = NextBranchCost(D, P, Id);
		return Cost >= 0.0 && P.Stardust >= Cost;
	}

	std::string BranchForJob(const PcGameData& D, const std::string& Job)
	{
		if (Job.empty())
		{
			return std::string();
		}
		// Godot walks BRANCH_PATHS in order; the export keeps that order, and the fixture test pins it.
		for (const PcSkillBranch& B : D.Branches)
		{
			if (B.JobId == Job)
			{
				return B.Id;
			}
		}
		return std::string();
	}

	std::string BlockingRival(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		if (!B)
		{
			return std::string();
		}
		for (const std::string& Rival : B->ExclusiveWith)
		{
			if (BranchLevel(P, Rival) > 0)
			{
				return Rival;
			}
		}
		return std::string();
	}

	bool IsBranchRetired(const PcGameData& D, const std::string& Id)
	{
		const std::vector<std::string>& Gone = D.Code.RetiredBranches;
		const PcSkillBranch* B = D.FindBranch(Id);
		for (size_t Guard = 0; B && Guard <= D.Branches.size(); ++Guard)
		{
			if (std::find(Gone.begin(), Gone.end(), B->Id) != Gone.end())
			{
				return true;
			}
			B = B->Parent.empty() ? nullptr : D.FindBranch(B->Parent);
		}
		return false;
	}

	double RefundRetiredBranches(const PcGameData& D, PcProgress& P)
	{
		double Refund = 0.0;
		for (const PcSkillBranch& B : D.Branches)
		{
			const auto It = P.BranchLevels.find(B.Id);
			if (It == P.BranchLevels.end() || !IsBranchRetired(D, B.Id))
			{
				continue;
			}
			const int64_t Bought = std::min<int64_t>(It->second, static_cast<int64_t>(B.LevelCosts.size()));
			for (int64_t L = 0; L < Bought; ++L)
			{
				Refund += B.LevelCosts[static_cast<size_t>(L)];
			}
			P.BranchLevels.erase(It);
		}
		P.Stardust += Refund;
		return Refund;
	}

	bool IsBranchAvailable(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		if (!B)
		{
			return true;
		}
		// Taken out of the game (D-e): never for sale.
		if (IsBranchRetired(D, Id))
		{
			return false;
		}
		// The only enforced prerequisite: a ladder cannot be bought before the unlock it hangs off.
		if (B->bParentIsRequired && !B->Parent.empty() && BranchLevel(P, B->Parent) <= 0)
		{
			return false;
		}
		// The fork: owning a rival closes the door for BUYING only (old saves keep what they have).
		if (!BlockingRival(D, P, Id).empty())
		{
			return false;
		}
		// The boss holds the key; Stardust buys what is behind it.
		if (!B->GatedByBoss.empty() && !P.BossesDefeated.count(B->GatedByBoss))
		{
			return false;
		}
		return P.ReawakenCount >= BranchReawakenGate(D, Id);
	}

	int64_t BranchReawakenGate(const PcGameData& D, const std::string& Id)
	{
		// The three ability unlocks keep their own constants: other systems ask whether Pounce exists yet, and a second
		// answer to the same question is how two gates come to disagree.
		if (Id == "unlock_pounce")
		{
			return D.Reawaken.PounceUnlockReawakenCount;
		}
		if (Id == "unlock_overkill")
		{
			return D.Reawaken.OverkillUnlockReawakenCount;
		}
		if (Id == "unlock_unicorn")
		{
			return D.Reawaken.UnicornUnlockReawakenCount;
		}
		const PcSkillBranch* B = D.FindBranch(Id);
		return B ? B->RequiredReawakens : 0;
	}

	PcBranchPurchase PurchaseNextBranchLevel(const PcGameData& D, PcProgress& P, const std::string& Id)
	{
		PcBranchPurchase Out;
		if (!IsBranchAvailable(D, P, Id) || IsBranchMaxLevel(D, P, Id) || !CanAffordNextBranch(D, P, Id))
		{
			return Out;
		}
		Out.bBought = true;
		Out.Cost = NextBranchCost(D, P, Id);
		P.Stardust -= Out.Cost;
		P.BranchLevels[Id] = BranchLevel(P, Id) + 1;
		// A job node hands its job over the moment it is bought; held is held, so it cannot double-grant.
		const PcSkillBranch* B = D.FindBranch(Id);
		if (B && !B->JobId.empty() && D.FindJob(B->JobId) && P.JobsHeld.insert(B->JobId).second)
		{
			Out.JobOffered = B->JobId;
		}
		// An automation node switches itself on, or the player buys it and nothing happens.
		Out.bAutoTravelOn = Id == "auto_travel";
		Out.bAutoCardOn = Id == "auto_card";
		return Out;
	}

	void GrantBranchLevel(const PcGameData& D, PcProgress& P, const std::string& Id, int64_t Level)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		if (!B)
		{
			return;
		}
		P.BranchLevels[Id] = std::clamp<int64_t>(std::max(BranchLevel(P, Id), Level), 0, BranchMaxLevel(*B));
	}

	std::vector<std::string> TrunkBranches(const PcGameData& D)
	{
		std::vector<std::string> Out;
		for (const PcSkillBranch& B : D.Branches)
		{
			if (B.Parent.empty())
			{
				Out.push_back(B.Id);
			}
		}
		std::sort(Out.begin(), Out.end());
		return Out;
	}

	int64_t MaxVitalityBonus(const PcGameData& D)
	{
		const PcSkillBranch* B = D.FindBranch("vitality");
		if (!B || B->LevelEffects.empty())
		{
			return 0;
		}
		return static_cast<int64_t>(B->LevelEffects.back());
	}
}
