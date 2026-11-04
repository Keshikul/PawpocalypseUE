#include "PcJobs.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		// Godot's is_equal_approx(a, b): exact, or within CMP_EPSILON (1e-5) relative, at least 1e-5 absolute.
		bool IsEqualApprox(double A, double B)
		{
			if (A == B)
			{
				return true;
			}
			const double Tolerance = std::max(0.00001 * std::fabs(A), 0.00001);
			return std::fabs(A - B) < Tolerance;
		}

		// Godot's is_zero_approx(x).
		bool IsZeroApprox(double X)
		{
			return std::fabs(X) < 0.00001;
		}
	}

	std::vector<const PcJob*> AllJobs(const PcGameData& D)
	{
		std::vector<const PcJob*> Out;
		for (const PcJob& J : D.Jobs)
		{
			Out.push_back(&J);
		}
		// Stable because the equip screen is a list the player learns the shape of.
		std::stable_sort(Out.begin(), Out.end(), [](const PcJob* A, const PcJob* B)
		{
			const bool bAReawaken = A->DroppedByBiome.empty();
			const bool bBReawaken = B->DroppedByBiome.empty();
			if (bAReawaken != bBReawaken)
			{
				return !bAReawaken;
			}
			if (bAReawaken)
			{
				return A->RequiredReawakens < B->RequiredReawakens;
			}
			return A->DroppedByBiome < B->DroppedByBiome;
		});
		return Out;
	}

	bool HoldsJob(const PcProgress& P, const std::string& Id)
	{
		return P.JobsHeld.count(Id) > 0;
	}

	bool JobIsAvailable(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		return D.FindJob(Id) && HoldsJob(P, Id);
	}

	bool OfferJob(const PcGameData& D, PcProgress& P, const std::string& Id)
	{
		return D.FindJob(Id) && P.JobsHeld.insert(Id).second;
	}

	bool LockRunJob(PcProgress& P, const std::string& Id)
	{
		if (!Id.empty() && !HoldsJob(P, Id))
		{
			return false;
		}
		P.RunJob = Id;
		P.CurrentJob = Id;
		return true;
	}

	bool LockRunJobSecond(const PcGameData& D, PcProgress& P, const std::string& Id)
	{
		if (Id.empty())
		{
			P.RunJobSecond.clear();
			return true;
		}
		if (!HasUnionRules(D, P) || Id == P.RunJob || !HoldsJob(P, Id))
		{
			return false;
		}
		P.RunJobSecond = Id;
		return true;
	}

	bool TakeJob(PcProgress& P, const std::string& Id)
	{
		if (!Id.empty() && !HoldsJob(P, Id))
		{
			return false;
		}
		if (P.CurrentJob == Id)
		{
			return false;
		}
		P.CurrentJob = Id;
		return true;
	}

	void ClearRunJobs(PcProgress& P)
	{
		// Clearing the lock is what makes the choice per-run rather than permanent.
		P.RunJob.clear();
		P.RunJobSecond.clear();
		P.bRunJobChosen = false;
	}

	bool HasUnionRules(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "union_rules") > 0.0;
	}

	double UnionRulesShare(const PcGameData& D, const PcProgress& P)
	{
		return std::clamp(BranchEffect(D, P, "union_rules"), 0.0, 1.0);
	}

	const PcJob* EquippedJob(const PcGameData& D, const PcProgress& P)
	{
		return P.RunJob.empty() ? nullptr : D.FindJob(P.RunJob);
	}

	const PcJob* SecondaryJob(const PcGameData& D, const PcProgress& P)
	{
		if (P.RunJobSecond.empty() || !HasUnionRules(D, P) || P.RunJobSecond == P.RunJob)
		{
			return nullptr;
		}
		return D.FindJob(P.RunJobSecond);
	}

	double JobTerm(const PcGameData& D, const PcProgress& P, double PcJob::* Field)
	{
		double Total = 1.0;
		if (const PcJob* Primary = EquippedJob(D, P))
		{
			Total *= Primary->*Field;
		}
		if (const PcJob* Second = SecondaryJob(D, P))
		{
			Total *= 1.0 + (Second->*Field - 1.0) * UnionRulesShare(D, P);
		}
		return Total;
	}

	double JobCritChanceBonus(const PcGameData& D, const PcProgress& P)
	{
		double Total = 0.0;
		if (const PcJob* Primary = EquippedJob(D, P))
		{
			Total += Primary->CritChanceBonus;
		}
		if (const PcJob* Second = SecondaryJob(D, P))
		{
			Total += Second->CritChanceBonus * UnionRulesShare(D, P);
		}
		return Total;
	}

	int64_t JobExtraCards(const PcGameData& D, const PcProgress& P)
	{
		int64_t Total = 0;
		if (const PcJob* Primary = EquippedJob(D, P))
		{
			Total += Primary->ExtraCardsInHand;
		}
		if (const PcJob* Second = SecondaryJob(D, P))
		{
			Total += static_cast<int64_t>(std::floor(static_cast<double>(Second->ExtraCardsInHand) * UnionRulesShare(D, P)));
		}
		return Total;
	}

	double JobPounceLockSec(const PcGameData& D, const PcProgress& P)
	{
		double Total = 0.0;
		if (const PcJob* Primary = EquippedJob(D, P))
		{
			Total += Primary->PounceLockSec;
		}
		if (const PcJob* Second = SecondaryJob(D, P))
		{
			Total += Second->PounceLockSec * UnionRulesShare(D, P);
		}
		return Total;
	}

	std::string JobHatPath(const PcGameData& D, const PcProgress& P)
	{
		const PcJob* J = EquippedJob(D, P);
		return J ? J->HatPath : std::string();
	}

	bool IsJobNeutral(const PcJob& J)
	{
		return IsEqualApprox(J.PawDamageMultiplier, 1.0)
			&& IsEqualApprox(J.AuraDamageMultiplier, 1.0)
			&& IsEqualApprox(J.AuraRadiusMultiplier, 1.0)
			&& IsEqualApprox(J.KittenDamageMultiplier, 1.0)
			&& IsEqualApprox(J.PounceDamageMultiplier, 1.0)
			&& IsEqualApprox(J.AllDamageMultiplier, 1.0)
			&& IsEqualApprox(J.CatnipMultiplier, 1.0)
			&& IsEqualApprox(J.CritChanceMultiplier, 1.0)
			&& IsEqualApprox(J.AttackSpeedMultiplier, 1.0)
			&& IsZeroApprox(J.CritChanceBonus)
			&& IsEqualApprox(J.CritDamageMultiplier, 1.0)
			&& IsEqualApprox(J.EnemyHealthMultiplier, 1.0)
			&& IsEqualApprox(J.SpawnRateMultiplier, 1.0)
			&& J.ExtraCardsInHand == 0
			&& IsEqualApprox(J.PounceLockSec, 0.0);
	}

	bool JobHasACost(const PcJob& J)
	{
		// "unemployed" is the deliberate exception (no cost): it pays for its +10% by being the last unlock.
		return J.PawDamageMultiplier < 1.0
			|| J.CritChanceMultiplier < 1.0
			|| J.AttackSpeedMultiplier < 1.0
			// NOT < 1.0: tougher enemies is the cost, so this one reads the other way.
			|| J.EnemyHealthMultiplier > 1.0
			|| J.SpawnRateMultiplier < 1.0
			|| J.AuraDamageMultiplier < 1.0
			|| J.AuraRadiusMultiplier < 1.0
			|| J.KittenDamageMultiplier < 1.0
			|| J.PounceDamageMultiplier < 1.0
			|| J.AllDamageMultiplier < 1.0
			|| J.CatnipMultiplier < 1.0
			|| J.PounceLockSec > 0.0;
	}

	bool ShouldAskForJob(const PcProgress& P)
	{
		if (P.bRunJobChosen)
		{
			return false;
		}
		return P.JobsHeld.size() >= 1;
	}

	int64_t OtherJobsThan(const PcProgress& P, const std::string& Id)
	{
		return static_cast<int64_t>(P.JobsHeld.size()) - (HoldsJob(P, Id) ? 1 : 0);
	}
}
