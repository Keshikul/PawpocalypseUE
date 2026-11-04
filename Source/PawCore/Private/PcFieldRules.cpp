#include "PcFieldRules.h"

#include "PcCombat.h"
#include "PcEconomy.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcTree.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Pc
{
	namespace
	{
		const PcUpgrade& Upgrade(const PcGameData& D, const char* Id)
		{
			static const PcUpgrade Missing;
			const PcUpgrade* U = D.FindUpgrade(Id);
			return U ? *U : Missing;
		}

		bool SnareUnlocked(const PcProgress& P)
		{
			return BranchLevel(P, "unlock_snare") > 0;
		}

		// TierData._weight_at(): a missing entry counts as a common.
		double WeightAt(const PcGameData& D, const PcTier& Tier, size_t I, double SignatureWeight)
		{
			const double W = std::max(Tier.SpawnWeights[I], 0.0);
			const PcEnemy* E = D.FindEnemy(Tier.SpawnPool[I]);
			return E && E->bIsSignature ? W * SignatureWeight : W;
		}

		size_t UsablePoolSize(const PcTier& Tier)
		{
			return std::min(Tier.SpawnPool.size(), Tier.SpawnWeights.size());
		}
	}

	double SpawnRateMultiplier(const PcGameData& D, const PcProgress& P)
	{
		return CappedEffect(Upgrade(D, "spawn_rate"), UpgradeLevel(P, "spawn_rate"), P)
			* CardCostMultiplier(D, P, CardCost::SPAWN_RATE)
			* JobTerm(D, P, &PcJob::SpawnRateMultiplier)
			* std::max(BranchEffect(D, P, "swarm"), 1.0)
			* (1.0 + P.InfestationBonus);
	}

	double SpawnRatePerSec(const PcGameData& D, const PcProgress& P, const PcTier& Tier)
	{
		return std::max(Tier.SpawnRatePerMin * SpawnRateMultiplier(D, P) / 60.0, 0.0001);
	}

	PcSpawnCadence SpawnCadence(const PcGameData& D, double PerSec)
	{
		PcSpawnCadence Out;
		Out.Batch = std::max<int64_t>(1, static_cast<int64_t>(std::ceil(PerSec * D.Code.MinSpawnInterval)));
		Out.Interval = std::max(D.Code.MinSpawnInterval, static_cast<double>(Out.Batch) / PerSec);
		return Out;
	}

	double ThresholdDamageMultiplier(const PcGameData& D, const PcProgress& P, int64_t LiveEnemies)
	{
		const double Crowd = LiveEnemies < D.Code.CrowdThreshold ? 1.0 : std::max(BranchEffect(D, P, "swarm_logic"), 1.0);
		const double LastStand = LiveEnemies >= D.Code.LastStandThreshold ? 1.0 : std::max(BranchEffect(D, P, "last_stand"), 1.0);
		return Crowd * LastStand;
	}

	double SnaredDamageMultiplier(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "traps"), 1.0);
	}

	double PlayWithFoodMultiplier(const PcGameData& D, const PcProgress& P)
	{
		if (!SnareUnlocked(P) || BranchLevel(P, "play_with_food") <= 0)
		{
			return 1.0;
		}
		return BranchEffect(D, P, "play_with_food");
	}

	double FunnelBounces(const PcGameData& D, const PcProgress& P)
	{
		if (!SnareUnlocked(P) || BranchLevel(P, "funnel") <= 0)
		{
			return 0.0;
		}
		return BranchEffect(D, P, "funnel");
	}

	double EnemyDrift(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "misdirection"), 0.0);
	}

	double PrismKillMultiplier(const PcGameData& D, const PcProgress& P)
	{
		if (BranchLevel(P, "unlock_prism") <= 0 || BranchLevel(P, "spectrum") <= 0)
		{
			return 1.0;
		}
		return BranchEffect(D, P, "spectrum");
	}

	double TotalSpawnWeight(const PcGameData& D, const PcTier& Tier, double SignatureWeight)
	{
		double Total = 0.0;
		for (size_t I = 0; I < UsablePoolSize(Tier); ++I)
		{
			Total += WeightAt(D, Tier, I, SignatureWeight);
		}
		return Total;
	}

	const PcEnemy* PickEnemy(const PcGameData& D, const PcTier& Tier, double Roll, double SignatureWeight)
	{
		const size_t Size = UsablePoolSize(Tier);
		if (Size == 0)
		{
			return nullptr;
		}
		const double Total = TotalSpawnWeight(D, Tier, SignatureWeight);
		if (Total <= 0.0)
		{
			return D.FindEnemy(Tier.SpawnPool[0]);
		}
		const double Target = std::clamp(Roll, 0.0, 0.9999) * Total;
		double Running = 0.0;
		for (size_t I = 0; I < Size; ++I)
		{
			Running += WeightAt(D, Tier, I, SignatureWeight);
			if (Target < Running)
			{
				return D.FindEnemy(Tier.SpawnPool[I]);
			}
		}
		return D.FindEnemy(Tier.SpawnPool[Size - 1]);
	}

	PcRect PlayfieldRect(const PcGameData& D, double ViewW, double ViewH, const PcRect& Dock, double FallbackMargin,
		bool bDockOnLeft)
	{
		double Left = 0.0;
		double Right = ViewW;
		if (Dock.W > 0.0)
		{
			if (Dock.CentreX() < ViewW * 0.5)
			{
				Left = Dock.Right();
			}
			else
			{
				Right = Dock.X;
			}
		}
		else if (bDockOnLeft)
		{
			Left = FallbackMargin;
		}
		else
		{
			Right = ViewW - FallbackMargin;
		}
		// Never outside the viewport: a mid-resize dock can report an edge off screen.
		Left = std::clamp(Left, 0.0, ViewW);
		Right = std::clamp(Right, 0.0, ViewW);
		// Never so narrow the arena stops working (the band, the spawner and the boss arena need room).
		const double MinWidth = D.Code.MinPlayfieldWidth;
		if (Right - Left < MinWidth)
		{
			if (Left > 0.0)
			{
				Left = std::max(0.0, Right - MinWidth);
			}
			else
			{
				Right = std::min(ViewW, Left + MinWidth);
			}
		}
		return PcRect{ Left, 0.0, Right - Left, ViewH };
	}

	PcEnemySpawnStats EnemySpawnStats(const PcGameData& D, const PcProgress& P, const PcEnemy& Enemy)
	{
		PcEnemySpawnStats Out;
		Out.Hp = Enemy.Hp * CardCostMultiplier(D, P, CardCost::MOUSE_HP) * WaveHpMultiplier(D, P);
		Out.MaxHp = Out.Hp;
		Out.WaveRewardMultiplier = WaveRewardMultiplier(D, P);
		Out.SpeedMultiplier = CardCostMultiplier(D, P, CardCost::MOUSE_SPEED);
		Out.FunnelBounces = FunnelBounces(D, P);
		return Out;
	}

	PcBlowResult ApplyBlow(const PcGameData& D, const PcProgress& P, PcBlowTarget& Target, double Amount, double ShieldPierce,
		double ThresholdNow)
	{
		PcBlowResult Out;
		Out.HpAfter = Target.Hp;
		if (!Target.Data)
		{
			return Out;
		}
		if (Target.bShielded)
		{
			// Shield Pierce lets a share through; at 0 a shielded enemy takes nothing at all.
			if (ShieldPierce <= 0.0)
			{
				return Out;
			}
			Amount *= ShieldPierce;
		}
		// Traps: checked on the enemy because every source arrives here.
		if (Target.bSnared)
		{
			Amount *= SnaredDamageMultiplier(D, P);
		}
		Amount *= ThresholdNow;
		const double HpBefore = Target.Hp;
		Target.Hp -= Amount;
		Out.HpAfter = Target.Hp;
		if (Target.Hp > 0.0)
		{
			return Out;
		}
		Out.bKilled = true;
		Out.Overkill = -Target.Hp;
		double Reward = Target.Data->RewardCatnip * Target.WaveRewardMultiplier;
		// Play With Food: a mouse finished off while already nearly dead (against its BASE hp) pays more.
		if (HpBefore > 0.0 && HpBefore <= Target.Data->Hp * D.Code.PlayWithFoodHpFraction)
		{
			const double Food = PlayWithFoodMultiplier(D, P);
			Reward *= Food;
			Out.bToyedWith = Food > 1.0;
		}
		// Spectrum: a beam kill pays extra.
		if (Target.bKilledByBeam)
		{
			Reward *= PrismKillMultiplier(D, P);
		}
		Out.Reward = Reward;
		return Out;
	}
}

namespace Pc
{
	int64_t KittenPoolBaseSize(const PcGameData& D)
	{
		const PcUpgrade* U = D.FindUpgrade("kitten_count");
		const int64_t Cap = U ? static_cast<int64_t>(U->Cap) : 0;
		return Cap + MaxVitalityBonus(D) + D.Code.MaxTempKittens;
	}

	void KittenOrbitSlot(const PcGameData& D, int64_t Index, int64_t Total, double& OutX, double& OutY)
	{
		const double Count = static_cast<double>(std::max<int64_t>(Total, 1));
		const double Angle = 6.283185307179586 * static_cast<double>(Index) / Count;
		OutX = std::cos(Angle) * D.Code.KittenOrbitRadiusPx;
		OutY = std::sin(Angle) * D.Code.KittenOrbitRadiusPx;
	}

	bool KittensSeekClusters(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "kitten_foreman") > 0.0;
	}

	double KittenIllusionPull(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "grand_finale"), 0.0);
	}

	int32_t PickKittenTarget(const PcGameData& D, double OX, double OY, const std::vector<int32_t>& Order,
		const std::vector<uint8_t>& Active, const std::vector<double>& X, const std::vector<double>& Y,
		const std::vector<int64_t>& Claims, bool bForeman)
	{
		// Squared distances: they order exactly as distances do, without a square root per mouse.
		const double Radius = D.Combat.BaseKittenDetectionRadius;
		const double RadiusSq = Radius * Radius;
		int32_t Best = -1;
		int64_t BestClaims = 0;
		double BestDistSq = std::numeric_limits<double>::infinity();
		for (const int32_t S : Order)
		{
			const size_t I = static_cast<size_t>(S);
			// Godot also walks the mice that died this frame (swept at its end); deactivate() parks them 10,000 px off
			// the field, outside any detection radius, so skipping them is the same answer.
			if (!Active[I])
			{
				continue;
			}
			const double DX = X[I] - OX;
			const double DY = Y[I] - OY;
			const double DSq = DX * DX + DY * DY;
			if (DSq > RadiusSq)
			{
				continue;
			}
			const int64_t C = Claims[I];
			// Once an unclaimed mouse is in hand only distance can improve on it (the fewest-claims key cannot be
			// beaten), so the common uncrowded board compares distance alone. A Foreman cannot take this path: it would
			// skip exactly the crowded mice the node exists to find.
			if (!bForeman && BestClaims == 0 && Best >= 0)
			{
				if (DSq < BestDistSq && C == 0)
				{
					Best = S;
					BestDistSq = DSq;
				}
				continue;
			}
			const bool bBetter = bForeman ? C > BestClaims : C < BestClaims;
			if (Best < 0 || bBetter || (C == BestClaims && DSq < BestDistSq))
			{
				Best = S;
				BestClaims = C;
				BestDistSq = DSq;
			}
		}
		return Best;
	}

	bool KittenOvercrowded(const PcGameData& D, double OX, double OY, int32_t Target, const std::vector<int32_t>& Order,
		const std::vector<uint8_t>& Active, const std::vector<double>& X, const std::vector<double>& Y,
		const std::vector<int64_t>& Claims)
	{
		const int64_t Mine = Claims[static_cast<size_t>(Target)];
		if (Mine <= 1)
		{
			return false;
		}
		const double Radius = D.Combat.BaseKittenDetectionRadius;
		const double RadiusSq = Radius * Radius;
		for (const int32_t S : Order)
		{
			const size_t I = static_cast<size_t>(S);
			if (!Active[I] || S == Target)
			{
				continue;
			}
			const double DX = X[I] - OX;
			const double DY = Y[I] - OY;
			if (DX * DX + DY * DY > RadiusSq)
			{
				continue;
			}
			if (Mine - Claims[I] >= D.Code.KittenRebalanceMargin)
			{
				return true;
			}
		}
		return false;
	}
}

namespace Pc
{
	bool IsAuraOwned(const PcProgress& P)
	{
		return BranchLevel(P, "unlock_aura") > 0;
	}

	double AuraReachMultiplier(const PcGameData& D, const PcProgress& P)
	{
		if (BranchLevel(P, "aura_reach") <= 0)
		{
			return 1.0;
		}
		const double Effect = BranchEffect(D, P, "aura_reach");
		return Effect > 0.0 ? Effect : 1.0;
	}

	double CarryRadiusBonus(const PcGameData& D, const PcProgress& P)
	{
		const double Top = BranchEffect(D, P, "carry");
		if (Top <= 0.0)
		{
			return 0.0;
		}
		return Top * std::clamp(P.PlayerStillSeconds / D.Code.CarryFullSeconds, 0.0, 1.0);
	}

	double AuraRadiusMultiplier(const PcGameData& D, const PcProgress& P)
	{
		const PcUpgrade* U = D.FindUpgrade("aura_radius");
		const double Line = U ? EffectTotal(*U, UpgradeLevel(P, "aura_radius")) : 0.0;
		return (Line + CarryRadiusBonus(D, P)) * AuraReachMultiplier(D, P) * CardCostMultiplier(D, P, CardCost::AURA_RADIUS);
	}

	double AuraRadius(const PcGameData& D, const PcProgress& P)
	{
		return D.Combat.BaseAuraRadius * AuraRadiusMultiplier(D, P);
	}

	double SnareMultiplier(const PcGameData& D, const PcProgress& P)
	{
		if (!SnareUnlocked(P))
		{
			return 1.0;
		}
		double Fraction = BranchEffect(D, P, "unlock_snare");
		if (BranchLevel(P, "deadweight") > 0)
		{
			Fraction = BranchEffect(D, P, "deadweight");
		}
		return std::clamp(1.0 - Fraction, 0.0, 1.0);
	}

	double GridlockMultiplier(const PcGameData& D, const PcProgress& P)
	{
		const double V = BranchEffect(D, P, "gridlock");
		return V > 0.0 && V < 1.0 ? V : 1.0;
	}

	double AuraCrowdScaling(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "crowd_work"), 0.0);
	}

	double AuraEchoPower(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "echo"), 0.0);
	}

	double AuraBeatSeconds(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "standing_wave"), 0.0);
	}

	bool PoopUnlocked(const PcProgress& P)
	{
		return IsUnicornOwned(P) && BranchLevel(P, "unlock_poop") > 0;
	}

	double LongLunchPayout(const PcGameData& D, const PcProgress& P)
	{
		const double Per = BranchEffect(D, P, "long_lunch");
		if (Per <= 0.0)
		{
			return 0.0;
		}
		return Per * std::clamp(P.LunchSeconds / D.Code.LunchFullSeconds, 0.0, 1.0);
	}

	void ApplySlow(double& SlowMultiplier, uint8_t& bSlowFresh, bool bFlier, double Multiplier, bool bReachesFliers)
	{
		if (Multiplier <= 0.0 || Multiplier >= 1.0)
		{
			return;
		}
		// Nothing on the ground reaches a flier, slows included (v4 Phase 8). Gridlock is the one exception: the flier is
		// caught by the tangle of everything already snared underneath it.
		if (bFlier && !bReachesFliers)
		{
			return;
		}
		SlowMultiplier = bSlowFresh ? std::min(SlowMultiplier, Multiplier) : Multiplier;
		bSlowFresh = 1;
	}
}

namespace Pc
{
	bool IsPounceOwned(const PcProgress& P)
	{
		return BranchLevel(P, "unlock_pounce") > 0;
	}

	bool AutoPounceUnlocked(const PcProgress& P)
	{
		return BranchLevel(P, "unlock_reflex") > 0;
	}

	double AutoPounceMultiplier(const PcGameData& D, const PcProgress& P)
	{
		const double Effect = BranchEffect(D, P, "unlock_reflex");
		return Effect > 0.0 ? Effect : 1.0;
	}

	bool HasPounceInsurance(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "nine_lives_insurance") > 0.0;
	}

	double PounceCraterPower(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "crash_landing"), 0.0);
	}

	double PounceLockRefund(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "momentum"), 0.0);
	}
}

namespace Pc
{
	double GuaranteedCritShare(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "butchers_rhythm"), 0.0);
	}

	bool DeterministicCrit(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "inscrutable") > 0.0;
	}

	double DoubleStrikeShare(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "anvil_paw"), 0.0);
	}

	int64_t PawCleaveCount(const PcGameData& D, const PcProgress& P)
	{
		return std::max<int64_t>(0, static_cast<int64_t>(BranchEffect(D, P, "hammerfall")));
	}

	double CritExecuteChance(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "riddle"), 0.0);
	}

	double OverkillCatnipShare(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "cat_burglars_cut"), 0.0);
	}

	double OverflowRatio(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "overflow"), 0.0);
	}

	double LevyMultiple(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "levy"), 0.0);
	}

	double PickpocketShare(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "pickpocket"), 0.0);
	}

	double PiedPiperRadius(const PcGameData& D, const PcProgress& P)
	{
		return std::max(BranchEffect(D, P, "pied_piper"), 0.0);
	}

	bool SpawnForesight(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "sixth_sense") > 0.0;
	}

	double VanishInterval(const PcGameData& D, const PcProgress& P)
	{
		const double PerMinute = BranchEffect(D, P, "sleight_of_hand");
		if (PerMinute <= 0.0)
		{
			return 0.0;
		}
		return 60.0 / PerMinute;
	}

	bool PrismOwned(const PcProgress& P)
	{
		return BranchLevel(P, "unlock_prism") > 0;
	}

	double PrismBeamDps(const PcGameData& D, const PcProgress& P, const PcCombatStats& Stats)
	{
		if (!PrismOwned(P))
		{
			return 0.0;
		}
		return AuraDps(D.Combat, Stats) * BranchEffect(D, P, "unlock_prism");
	}

	bool PrismSweeps(const PcProgress& P)
	{
		return PrismOwned(P) && BranchLevel(P, "unlock_full_spectrum") > 0;
	}

	int64_t PrismChainCount(const PcGameData& D, const PcProgress& P)
	{
		if (!PrismOwned(P) || BranchLevel(P, "refraction") <= 0)
		{
			return 0;
		}
		return static_cast<int64_t>(BranchEffect(D, P, "refraction"));
	}

	double PrismShardPayload(const PcGameData& D, const PcProgress& P)
	{
		if (!PrismOwned(P) || BranchLevel(P, "critical_mass") <= 0)
		{
			return 0.0;
		}
		return BranchEffect(D, P, "critical_mass");
	}
}

namespace Pc
{
	bool OwnsAutoTravel(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "auto_travel") > 0.0;
	}

	bool OwnsAutoBait(const PcGameData& D, const PcProgress& P)
	{
		return BranchEffect(D, P, "auto_bait") > 0.0;
	}
}
