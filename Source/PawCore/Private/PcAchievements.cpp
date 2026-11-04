#include "PcAchievements.h"

#include "PcProgress.h"

namespace Pc
{
	const PcAchievement* FindAchievement(const PcGameData& D, const std::string& Id)
	{
		for (const PcAchievement& A : D.Achievements)
		{
			if (A.Id == Id)
			{
				return &A;
			}
		}
		return nullptr;
	}

	double AchievementProgress(const PcGameData& D, const PcProgress& P, const PcAchievement& A)
	{
		(void)D;
		switch (A.Metric)
		{
		case AchievementMetric::LIFETIME_KILLS:
			return static_cast<double>(P.LifetimeKills);
		case AchievementMetric::LIFETIME_CATNIP:
			return P.LifetimeCatnipEver;
		case AchievementMetric::REAWAKEN_COUNT:
			return static_cast<double>(P.ReawakenCount);
		case AchievementMetric::BOSSES_DEFEATED:
			return static_cast<double>(P.BossesDefeated.size());
		case AchievementMetric::EVOLUTION_STAGE:
			return static_cast<double>(EvolutionStage(D, P));
		case AchievementMetric::UPGRADE_LEVEL:
			return static_cast<double>(UpgradeLevel(P, A.UpgradeId));
		case AchievementMetric::SKILL_BRANCH_LEVEL:
			return static_cast<double>(BranchLevel(P, A.BranchId));
		}
		// An unknown metric never unlocks (Godot returns 0 rather than taking the game down).
		return 0.0;
	}

	bool IsAchievementEarned(const PcProgress& P, const std::string& Id)
	{
		return P.AchievementsEarned.count(Id) > 0;
	}

	int64_t EarnedAchievementCount(const PcGameData& D, const PcProgress& P)
	{
		int64_t N = 0;
		for (const PcAchievement& A : D.Achievements)
		{
			N += P.AchievementsEarned.count(A.Id) > 0 ? 1 : 0;
		}
		return N;
	}

	double TotalStardustOnOffer(const PcGameData& D)
	{
		double Total = 0.0;
		for (const PcAchievement& A : D.Achievements)
		{
			Total += A.StardustReward;
		}
		return Total;
	}

	std::vector<std::string> EvaluateAchievements(const PcGameData& D, PcProgress& P)
	{
		std::vector<std::string> Earned;
		for (const PcAchievement& A : D.Achievements)
		{
			if (IsAchievementEarned(P, A.Id))
			{
				continue;
			}
			if (AchievementProgress(D, P, A) >= A.Threshold)
			{
				P.AchievementsEarned.insert(A.Id);
				Earned.push_back(A.Id);
			}
		}
		return Earned;
	}

	double AchievementReward(const PcGameData& D, const std::string& Id)
	{
		const PcAchievement* A = FindAchievement(D, Id);
		return A ? A->StardustReward : 0.0;
	}

	bool IsAchievementClaimable(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		if (AchievementReward(D, Id) <= 0.0)
		{
			return false;
		}
		return IsAchievementEarned(P, Id) && !IsAchievementClaimed(P, Id);
	}

	bool IsAchievementClaimed(const PcProgress& P, const std::string& Id)
	{
		return P.AchievementsClaimed.count(Id) > 0;
	}

	int64_t UnclaimedAchievementCount(const PcGameData& D, const PcProgress& P)
	{
		int64_t N = 0;
		for (const PcAchievement& A : D.Achievements)
		{
			N += IsAchievementClaimable(D, P, A.Id) ? 1 : 0;
		}
		return N;
	}

	double UnclaimedStardust(const PcGameData& D, const PcProgress& P)
	{
		double Total = 0.0;
		for (const PcAchievement& A : D.Achievements)
		{
			if (IsAchievementClaimable(D, P, A.Id))
			{
				Total += A.StardustReward;
			}
		}
		return Total;
	}

	double ClaimAchievement(const PcGameData& D, PcProgress& P, const std::string& Id)
	{
		if (!IsAchievementClaimable(D, P, Id))
		{
			return 0.0;
		}
		const PcAchievement* A = FindAchievement(D, Id);
		if (!A)
		{
			return 0.0;
		}
		P.AchievementsClaimed.insert(Id);
		const double Reward = A->StardustReward;
		if (Reward > 0.0)
		{
			GrantAchievementStardust(P, Reward);
		}
		return Reward;
	}

	double ClaimAllAchievements(const PcGameData& D, PcProgress& P)
	{
		double Total = 0.0;
		for (const PcAchievement& A : D.Achievements)
		{
			Total += ClaimAchievement(D, P, A.Id);
		}
		return Total;
	}

	std::vector<std::string> ApplyAchievementSave(const PcGameData& D, PcProgress& P,
		const std::set<std::string>& Earned, const std::set<std::string>& Claimed)
	{
		P.AchievementsEarned = Earned;
		P.AchievementsClaimed = Claimed;
		return EvaluateAchievements(D, P);
	}
}
