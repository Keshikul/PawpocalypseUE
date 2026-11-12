#include "PcValidate.h"

#include "PcJobs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

namespace Pc
{
	namespace
	{
		// --- constants of the gates (the Godot reasoning is kept beside each) -------------------------------------------

		// evolution_stage_count and boss_realms come from Data/CodeConstants.csv (GameState constants, D.Code).
		// docs/06 §6: required_evolution_stage blocks 1-3:1, 4-6:2, 7-8:3, 9-11:4, 12:5.
		const std::vector<int64_t> StageGateLadder = { 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5 };

		// check_damage.gd: the ladder tops out where it always did (a shorter ladder must be re-spread, not truncated).
		constexpr double EvolutionTopWeaponMultiplier = 1.40;
		// A fully evolved cat is worth x128 from evolution alone (base ^ (stages - 1)), +/- 2.
		constexpr double EvolutionTopDamageMultiplier = 128.0;
		constexpr double EvolutionTopDamageTolerance = 2.0;

		// check_resources.gd (Numbers Doc §4): catnip_per_kill = 0.0433 x HP in the second biome, the per-HP curve then
		// declining by 3.40/3.90 per biome (HP x3.90, reward x3.40 per biome: each biome needs 1.15x more DPS per unit of
		// income). NOT a band: breaking "rewards track HP" once made the game mathematically uncompletable.
		constexpr double CatnipPerHp = 0.0433;
		constexpr double RewardHpDivergence = 3.40 / 3.90;
		constexpr double CatnipTolerance = 0.02;
		// Signatures pay double the tier's common Catnip and carry 2.5x its HP.
		constexpr double SignatureRewardFactor = 2.0 / 2.5;
		// Gristle buys one live-enemy SLOT for twelve commons' HP and pays 28 commons: the premium is the slot's price.
		constexpr double GristleRewardPremium = 28.0 / 12.0;
		constexpr double GristleMaxPremium = 3.0;
		// A flier is caught ~45% of the time; paying 1/0.45 makes expected Catnip per spawn equal to a common's. The same
		// number lives in pawsim (FLIER_CATCH_RATE): if one moves, the other must.
		constexpr double FlierCatchRate = 0.45;
		constexpr double FlierRewardPremium = 1.0 / FlierCatchRate;
		constexpr double FlierMaxPremium = 3.0;
		// Godot checked these two ceilings at runtime on constants; here a change past them does not compile.
		static_assert(GristleRewardPremium <= GristleMaxPremium, "one fat mouse must not become the economy");
		static_assert(FlierRewardPremium <= FlierMaxPremium, "a premium past 3x is a jackpot, not a miss rate");

		// Wave ladder: the first biome's wave 1 under 700 kills (the measured knee is 300), last/first at least 4x.
		constexpr double WaveLadderMaxFirst = 700.0;
		constexpr double WaveLadderMinSpread = 4.0;
		// Spawn rates climb strictly from the Alley on (index 2 = Alley -> Park); Backyard -> Alley is exempt (designer).
		constexpr size_t SpawnLadderFirstStep = 2;
		// A bait gate is a design bound, not a mirror of an engine ceiling: past 400,000 kills it is a wall.
		constexpr double MaxBaitGateKills = 400000.0;
		// Uncapped multiplicative shop lines: the SUM of return exponents log(effect)/log(growth) <= 0.80. Over 1.0
		// income reaches infinity in finite time and no HP figure can fix it (measured 1.986 on 2026-08-24).
		constexpr double ExponentBudget = 0.80;
		// Fraction lines (cap in 0..1) must stay under 0.9 at the deepest biome (7 floors of growth) after 12 Reawakens.
		constexpr double FractionCeiling = 0.9;
		constexpr double DeepestFloors = 7.0;
		constexpr double DeepestReawakens = 12.0;
		// The kittens' return exponent may run hotter than the paw's, never more than 0.02 colder.
		constexpr double KittenExponentSlack = 0.02;
		// Reward bounds of a biome: below 1.0 it pays worse than the Backyard; over 4.0 it dwarfs every earlier one.
		constexpr double RewardMultiplierMin = 1.0;
		constexpr double RewardMultiplierMax = 4.0;
		// Cards: every flavour holds 4 cards, exactly one with a cost, except the DECK flavour (none).
		constexpr int CardsPerFlavour = 4;
		constexpr int CostCardsPerFlavour = 1;
		// docs/06 §6: achievement Stardust rewards within 1..12.
		constexpr double AchievementStardustMin = 1.0;
		constexpr double AchievementStardustMax = 12.0;

		struct Checker
		{
			std::vector<std::string> Failures;

			void Fail(const char* Rule, const std::string& What)
			{
				Failures.push_back(std::string("[") + Rule + "] " + What);
			}
		};

		std::string Num(double V)
		{
			char Buf[64];
			std::snprintf(Buf, sizeof(Buf), "%.4g", V);
			return Buf;
		}

		bool ApproxEqual(double A, double B)
		{
			// Godot is_equal_approx: relative 1e-5 with an absolute floor.
			const double Tolerance = std::max(1e-5 * std::fabs(A), 1e-5);
			return std::fabs(A - B) < Tolerance;
		}

		std::string BaseName(const std::string& Path)
		{
			const size_t Slash = Path.find_last_of('/');
			const std::string File = Slash == std::string::npos ? Path : Path.substr(Slash + 1);
			const size_t Dot = File.find_last_of('.');
			return Dot == std::string::npos ? File : File.substr(0, Dot);
		}

		template <typename RowT>
		void UniqueIds(Checker& C, const std::vector<RowT>& Rows, const char* Table)
		{
			std::set<std::string> Seen;
			for (const RowT& R : Rows)
			{
				if (R.Id.empty())
				{
					C.Fail("ids.empty", std::string(Table) + ": a row has no id");
				}
				else if (!Seen.insert(R.Id).second)
				{
					C.Fail("ids.duplicate", std::string(Table) + ": id " + R.Id + " appears twice");
				}
				// CLAUDE.md: lower-case string ids. Lookups are exact, so "Paw_Damage" would be a second id that silently
				// finds nothing (found by tools/fuzz_validator.py, 2026-10-28: the only hostile corruption class let through).
				if (std::any_of(R.Id.begin(), R.Id.end(), [](char Ch) { return Ch >= 'A' && Ch <= 'Z'; }))
				{
					C.Fail("ids.lower_case", std::string(Table) + ": id " + R.Id + " is not lower-case");
				}
			}
		}

		// --- per-class checks (check_resources.gd _check_typed) --------------------------------------------------------

		void CheckJobs(Checker& C, const PcGameData& D)
		{
			for (const PcJob& J : D.Jobs)
			{
				const std::string Where = "job " + J.Id;
				const bool bNeutral = IsJobNeutral(J);
				// EVERY COLLAR IS A TRADE: an all-1.0 job sits in the list looking equippable and does nothing.
				if (bNeutral)
				{
					C.Fail("job.neutral", Where + " changes nothing; an equippable no-op");
				}
				const bool bHasCost = JobHasACost(J);
				// `unemployed` is the deliberate exception: it pays for its +10% by being the last unlock, not by a stat.
				if (J.Id != "unemployed" && !bHasCost)
				{
					C.Fail("job.free_gift", Where + " gives without taking; a job is a trade, not an upgrade");
				}
				if (J.Id.empty() || J.DisplayName.empty())
				{
					C.Fail("job.no_name", Where + " has no id or no name");
				}
				if (J.Description.empty())
				{
					C.Fail("job.no_description", Where + " has no description; the trade has to be legible before it is worn");
				}
				// The icon (or the hat, when there is one) must be this job's own art.
				if (!J.HatPath.empty())
				{
					if (BaseName(J.HatPath) != "hat_" + J.Id)
					{
						C.Fail("job.wrong_art", Where + " wears hat " + BaseName(J.HatPath) + " of another job");
					}
				}
				else if (BaseName(J.IconPath) != "job_" + J.Id)
				{
					C.Fail("job.wrong_art", Where + " shows icon " + BaseName(J.IconPath) + " of another job");
				}
				// A boss drop AND a Reawaken gate is two ways to earn one thing; the second would be dead code.
				if (!J.DroppedByBiome.empty() && J.RequiredReawakens > 0)
				{
					C.Fail("job.drop_and_gate", Where + " has both a boss drop and a Reawaken gate");
				}
				if (!J.DroppedByBiome.empty() && std::none_of(D.Tiers.begin(), D.Tiers.end(), [&](const PcTier& T) { return T.Id == J.DroppedByBiome; }))
				{
					C.Fail("ref.job_biome", Where + " is dropped by unknown biome " + J.DroppedByBiome);
				}
			}
		}

		void CheckUpgrades(Checker& C, const PcGameData& D)
		{
			for (const PcUpgrade& U : D.Upgrades)
			{
				const std::string Where = "upgrade " + U.Id;
				// Cap growth on an UNCAPPED line is authored intent that does nothing (effective_cap returns -1 early).
				if (U.Cap < 0.0 && U.CapPerFloor != 0.0)
				{
					C.Fail("upgrade.cap_growth_on_uncapped", Where + " has cap_per_floor " + Num(U.CapPerFloor) + " on an uncapped line");
				}
				if (U.Cap < 0.0 && U.CapPerReawaken != 0.0)
				{
					C.Fail("upgrade.cap_growth_on_uncapped", Where + " has cap_per_reawaken " + Num(U.CapPerReawaken) + " on an uncapped line");
				}
				if (U.CapPerReawaken < 0.0 || U.CapPerFloor < 0.0)
				{
					C.Fail("upgrade.cap_growth_negative", Where + " lowers its ceiling as the player progresses");
				}
				// A fraction line (overkill, shield pierce, pounce cooldown) reaching 1.0 makes something free.
				const double AtDeepest = U.Cap + U.CapPerFloor * DeepestFloors + U.CapPerReawaken * DeepestReawakens;
				if (U.Cap >= 0.0 && U.Cap <= 1.0 && AtDeepest > FractionCeiling)
				{
					C.Fail("upgrade.fraction_ceiling", Where + " reaches " + Num(AtDeepest) + " at the deepest biome after 12 Reawakens");
				}
				// CLAUDE.md hard rule. Godot's check for it sat in a second `upgrade_` branch that the first one always
				// returned before (check_resources.gd lines 125 and 286): it never ran. Here it does.
				if (U.Cap < 0.0 && U.EffectType == UpgradeEffectType::MULTIPLICATIVE && U.EffectPerLevel >= U.CostGrowth)
				{
					C.Fail("upgrade.runaway", Where + " is uncapped multiplicative with effect_per_level " + Num(U.EffectPerLevel)
						+ " >= cost_growth " + Num(U.CostGrowth) + "; it gets cheaper per point of effect forever");
				}
			}
		}

		void CheckBossPhases(Checker& C, const PcBoss& B, const std::string& Where)
		{
			// Boss.gd indexes the phase arrays by phase number: a length mismatch is a crash mid-fight.
			const size_t Phases = B.PhaseVulnerableSec.size();
			if (Phases == 0)
			{
				C.Fail("boss.phases", Where + " has no phases");
				return;
			}
			if (B.PhaseShieldSec.size() != Phases)
			{
				C.Fail("boss.phases", Where + " has " + std::to_string(B.PhaseShieldSec.size()) + " shield windows for " + std::to_string(Phases) + " phases");
			}
			if (B.PhaseHpThresholds.size() + 1 != Phases)
			{
				C.Fail("boss.phases", Where + " needs " + std::to_string(Phases - 1) + " thresholds, has " + std::to_string(B.PhaseHpThresholds.size()));
			}
			for (double V : B.PhaseVulnerableSec)
			{
				if (V <= 0.0)
				{
					C.Fail("boss.phases", Where + " has a non-positive vulnerable window; permanently shielded");
					break;
				}
			}
		}

		void CheckTiers(Checker& C, const PcGameData& D)
		{
			for (size_t Index = 0; Index < D.Tiers.size(); ++Index)
			{
				const PcTier& T = D.Tiers[Index];
				const std::string Where = "biome " + T.Id;
				// The pool is two PARALLEL arrays: a mismatch would degrade to "spawn the first entry".
				if (T.SpawnPool.empty())
				{
					C.Fail("tier.spawn_pool", Where + " has an empty spawn pool");
				}
				else if (T.SpawnPool.size() != T.SpawnWeights.size())
				{
					C.Fail("tier.spawn_pool", Where + " has " + std::to_string(T.SpawnPool.size()) + " pool entries but "
						+ std::to_string(T.SpawnWeights.size()) + " weights");
				}
				else
				{
					double Total = 0.0;
					for (size_t I = 0; I < T.SpawnPool.size(); ++I)
					{
						if (!D.FindEnemy(T.SpawnPool[I]))
						{
							C.Fail("ref.spawn_pool", Where + " spawns unknown enemy " + T.SpawnPool[I]);
						}
						if (T.SpawnWeights[I] < 0.0)
						{
							C.Fail("tier.spawn_pool", Where + " has a negative weight at entry " + std::to_string(I));
						}
						Total += T.SpawnWeights[I];
					}
					if (Total <= 0.0)
					{
						C.Fail("tier.spawn_pool", Where + " weights sum to " + Num(Total) + "; nothing could be picked");
					}
				}
				// The one term deliberately allowed to diverge from the HP curve, so it is bounded instead of tracked.
				if (T.RewardMultiplier < RewardMultiplierMin || T.RewardMultiplier > RewardMultiplierMax)
				{
					C.Fail("tier.reward_multiplier", Where + " reward_multiplier " + Num(T.RewardMultiplier) + " outside 1.0..4.0");
				}
				if (T.CatnipToUnlock < 0.0)
				{
					C.Fail("tier.catnip_to_unlock", Where + " has a negative catnip_to_unlock");
				}
				// A golden is rolled before the pool and wins outright: a chance with no enemy eats that share of spawns.
				if (T.GoldenChance > 0.0 && T.GoldenEnemy.empty())
				{
					C.Fail("tier.golden", Where + " has golden_chance " + Num(T.GoldenChance) + " and no golden enemy");
				}
				if (!T.GoldenEnemy.empty() && !D.FindEnemy(T.GoldenEnemy))
				{
					C.Fail("ref.golden", Where + " names unknown golden enemy " + T.GoldenEnemy);
				}
				// Every biome escalates inside itself (a flat plateau is the drought).
				if (T.KillsPerWave <= 0)
				{
					C.Fail("tier.wave_ladder", Where + " has no wave ladder");
				}
				else
				{
					if (T.WaveKillGrowth < 1.0)
					{
						C.Fail("tier.wave_ladder", Where + " wave_kill_growth " + Num(T.WaveKillGrowth) + " < 1: waves accelerate");
					}
					if (T.WaveHpGrowth <= 1.0)
					{
						C.Fail("tier.wave_ladder", Where + " wave_hp_growth " + Num(T.WaveHpGrowth) + " does not raise HP");
					}
					if (T.WaveRewardGrowth >= T.WaveHpGrowth)
					{
						C.Fail("tier.wave_ladder", Where + " wave reward grows as fast as HP; a wave that pays for itself");
					}
				}
				if (T.SpawnRatePerMin <= 0.0)
				{
					C.Fail("tier.spawn_rate", Where + " spawn_rate_per_min " + Num(T.SpawnRatePerMin) + "; it would spawn nothing");
				}
				// Stage gates (check_hud.gd): inside 1..7, never backwards, the first biome open, the designed blocks.
				if (T.RequiredEvolutionStage < 1 || T.RequiredEvolutionStage > D.Code.EvolutionStageCount)
				{
					C.Fail("tier.stage_gate", Where + " wants stage " + std::to_string(T.RequiredEvolutionStage) + ", outside 1..7");
				}
				if (Index > 0 && T.RequiredEvolutionStage < D.Tiers[Index - 1].RequiredEvolutionStage)
				{
					C.Fail("tier.stage_gate", Where + " needs a lower stage than the biome before it; the ladder goes backwards");
				}
				if (Index < StageGateLadder.size() && T.RequiredEvolutionStage != StageGateLadder[Index])
				{
					C.Fail("tier.stage_gate", Where + " needs stage " + std::to_string(T.RequiredEvolutionStage) + ", the designed block says "
						+ std::to_string(StageGateLadder[Index]));
				}
				if (!T.BossData.empty())
				{
					const PcBoss* B = D.FindBoss(T.BossData);
					if (!B)
					{
						C.Fail("ref.boss", Where + " names unknown boss " + T.BossData);
						continue;
					}
					if (B->TotalHp <= 0.0)
					{
						C.Fail("boss.hp", Where + " boss has non-positive total_hp");
					}
					// A boss only appears when bought: an unpriced boss is free, the "starts with a boss" bug again.
					if (B->BaitCostCatnip <= 0.0)
					{
						C.Fail("boss.bait_price", Where + " boss costs nothing to summon");
					}
					// The bait gate is expressed in kills, the quantity the player actually spends.
					if (B->BaitRequired <= 0)
					{
						C.Fail("boss.bait_gate", Where + " boss needs no bait: fought on arrival");
					}
					if (B->KillsPerBait <= 0)
					{
						C.Fail("boss.bait_gate", Where + " kills_per_bait " + std::to_string(B->KillsPerBait) + ": bait drops infinitely fast");
					}
					if (static_cast<double>(B->BaitRequired) * static_cast<double>(B->KillsPerBait) > MaxBaitGateKills)
					{
						C.Fail("boss.bait_gate", Where + " boss needs " + Num(static_cast<double>(B->BaitRequired) * B->KillsPerBait)
							+ " kills of bait: a wall, not a gate");
					}
					// The Nth boss is beaten at stage N+1: its biome must not ask for more.
					const std::vector<std::string>& Realms = D.Code.BossRealms;
					const auto Ladder = std::find(Realms.begin(), Realms.end(), T.Id);
					if (Ladder != Realms.end() && T.RequiredEvolutionStage > static_cast<int64_t>(Ladder - Realms.begin()) + 1)
					{
						C.Fail("tier.stage_gate", Where + " holds a boss it gates behind a stage you cannot have yet");
					}
				}
			}
			if (!D.Tiers.empty() && D.Tiers.front().RequiredEvolutionStage > 1)
			{
				C.Fail("tier.stage_gate", "the opening biome needs a stage above 1; a new player starts locked out");
			}
			// The boss biomes are exactly BOSS_REALMS, in both directions.
			std::vector<std::string> WithBoss;
			for (const PcTier& T : D.Tiers)
			{
				if (!T.BossData.empty())
				{
					WithBoss.push_back(T.Id);
				}
			}
			if (WithBoss != D.Code.BossRealms)
			{
				C.Fail("tier.boss_realms", "the biomes holding a boss are not code constant boss_realms, in that order");
			}
		}

		void CheckEnemies(Checker& C, const PcGameData& D)
		{
			for (const PcEnemy& E : D.Enemies)
			{
				// Tier 1 is hand-authored (Numbers Doc §5); the golden mouse is priced on rarity.
				if (E.Tier <= 1 || E.bRewardExempt)
				{
					continue;
				}
				const std::string Where = "enemy " + E.Id;
				if (E.Hp <= 0.0)
				{
					C.Fail("enemy.hp", Where + " has non-positive hp");
					continue;
				}
				const double PerHp = CatnipPerHp * std::pow(RewardHpDivergence, static_cast<double>(E.Tier - 1));
				double Expected = PerHp * E.Hp;
				if (E.bIsSignature)
				{
					Expected = PerHp * E.Hp * SignatureRewardFactor;
				}
				if (E.bIsGristle)
				{
					Expected = PerHp * E.Hp * GristleRewardPremium;
				}
				else if (E.Movement == EnemyMovementKind::FLIER)
				{
					Expected = PerHp * E.Hp * FlierRewardPremium;
				}
				if (std::fabs(E.RewardCatnip - Expected) > Expected * CatnipTolerance)
				{
					C.Fail("enemy.reward_scaling", Where + " reward_catnip " + Num(E.RewardCatnip) + " drifts from the reward-per-HP curve (expected "
						+ Num(Expected) + "); rewards must track enemy HP");
				}
			}
		}

		void CheckEvolution(Checker& C, const PcGameData& D)
		{
			// Contiguous 1..7: a gap makes the cat un-evolve mid-game.
			for (int64_t Stage = 1; Stage <= D.Code.EvolutionStageCount; ++Stage)
			{
				if (std::none_of(D.Evolutions.begin(), D.Evolutions.end(), [&](const PcEvolution& E) { return E.Stage == Stage; }))
				{
					C.Fail("evolution.ladder", "stage " + std::to_string(Stage) + " is missing");
				}
			}
			double Previous = 0.0;
			for (const PcEvolution& E : D.Evolutions)
			{
				if (E.WeaponMultiplier <= Previous)
				{
					C.Fail("evolution.weapon", "stage " + std::to_string(E.Stage) + " weapon multiplier " + Num(E.WeaponMultiplier)
						+ " does not beat the previous " + Num(Previous));
				}
				Previous = E.WeaponMultiplier;
				if (E.TexturePath.empty())
				{
					C.Fail("evolution.art", "stage " + std::to_string(E.Stage) + " has no texture_path; the cat would not change");
				}
			}
			if (!D.Evolutions.empty() && !ApproxEqual(D.Evolutions.back().WeaponMultiplier, EvolutionTopWeaponMultiplier))
			{
				C.Fail("evolution.weapon", "the top stage pays " + Num(D.Evolutions.back().WeaponMultiplier) + ", expected 1.40");
			}
			const double Reached = std::pow(D.Combat.EvolutionDamagePerStage, static_cast<double>(D.Code.EvolutionStageCount - 1));
			if (std::fabs(Reached - EvolutionTopDamageMultiplier) >= EvolutionTopDamageTolerance)
			{
				C.Fail("evolution.ceiling", "evolution damage reaches x" + Num(Reached) + ", expected x128");
			}
		}

		void CheckAchievements(Checker& C, const PcGameData& D)
		{
			for (const PcAchievement& A : D.Achievements)
			{
				const std::string Where = "achievement " + A.Id;
				if (A.Threshold <= 0.0)
				{
					C.Fail("achievement.threshold", Where + " has a non-positive threshold; it unlocks instantly");
				}
				if (A.Metric == AchievementMetric::UPGRADE_LEVEL && !D.FindUpgrade(A.UpgradeId))
				{
					C.Fail("achievement.target", Where + " counts levels of unknown upgrade '" + A.UpgradeId + "'");
				}
				if (A.Metric == AchievementMetric::SKILL_BRANCH_LEVEL && !D.FindBranch(A.BranchId))
				{
					C.Fail("achievement.target", Where + " counts levels of unknown branch '" + A.BranchId + "'");
				}
				if (A.StardustReward < AchievementStardustMin || A.StardustReward > AchievementStardustMax)
				{
					C.Fail("achievement.stardust", Where + " rewards " + Num(A.StardustReward) + " Stardust, outside 1..12");
				}
			}
		}

		void CheckBranches(Checker& C, const PcGameData& D)
		{
			for (const PcSkillBranch& B : D.Branches)
			{
				const std::string Where = "branch " + B.Id;
				if (B.LevelCosts.empty() || B.LevelCosts.size() != B.LevelEffects.size())
				{
					C.Fail("branch.levels", Where + " has " + std::to_string(B.LevelCosts.size()) + " costs for "
						+ std::to_string(B.LevelEffects.size()) + " effects");
				}
				if (!B.Parent.empty())
				{
					if (!D.FindBranch(B.Parent))
					{
						C.Fail("ref.branch_parent", Where + " has unknown parent " + B.Parent);
					}
					if (!B.bParentIsRequired)
					{
						C.Fail("branch.parent_required", Where + " has a parent that is not required");
					}
				}
				for (const std::string& X : B.ExclusiveWith)
				{
					if (!D.FindBranch(X))
					{
						C.Fail("ref.branch_exclusive", Where + " is exclusive with unknown branch " + X);
					}
				}
				if (B.EffectType == BranchEffectType::GRANTS_JOB && !D.FindJob(B.JobId))
				{
					C.Fail("ref.branch_job", Where + " grants unknown job '" + B.JobId + "'");
				}
				// skill_tree_manager.gd: a gated branch is buyable once bosses_defeated holds gated_by_boss. A name that is
				// not a boss realm can never be defeated, so the branch would be locked forever, silently (found by
				// tools/fuzz_validator.py, 2026-10-28).
				if (!B.GatedByBoss.empty()
					&& std::find(D.Code.BossRealms.begin(), D.Code.BossRealms.end(), B.GatedByBoss) == D.Code.BossRealms.end())
				{
					C.Fail("ref.branch_boss", Where + " is gated by '" + B.GatedByBoss + "', which is not a boss realm");
				}
			}
			// No cycles: walking up parents always ends.
			for (const PcSkillBranch& B : D.Branches)
			{
				std::set<std::string> Seen;
				const PcSkillBranch* Cur = &B;
				while (Cur && !Cur->Parent.empty())
				{
					if (!Seen.insert(Cur->Id).second)
					{
						C.Fail("branch.cycle", "branch " + B.Id + " sits on a parent cycle");
						break;
					}
					Cur = D.FindBranch(Cur->Parent);
				}
			}
		}

		void CheckCards(Checker& C, const PcGameData& D)
		{
			std::map<CardFlavour, int> PerFlavour, CostsPerFlavour;
			for (const PcCard& K : D.Cards)
			{
				const std::string Where = "card " + K.Id;
				// An owned card is re-offered at a higher level, so every level must be genuinely better.
				if (K.LevelValues.empty())
				{
					C.Fail("card.levels", Where + " has no levels; it can never be offered");
					continue;
				}
				if (K.Description.find("{v}") == std::string::npos)
				{
					C.Fail("card.description", Where + " description never shows the card's value");
				}
				if (K.Cost != CardCost::NONE && K.Description.find("{c}") == std::string::npos)
				{
					C.Fail("card.description", Where + " has a cost the description never mentions");
				}
				if (K.Cost == CardCost::NONE && !K.CostValues.empty())
				{
					C.Fail("card.cost", Where + " has cost values with no cost");
				}
				else if (K.Cost != CardCost::NONE && K.CostValues.size() != K.LevelValues.size())
				{
					C.Fail("card.cost", Where + " cost values are not parallel to its levels");
				}
				if (K.Weight <= 0.0)
				{
					C.Fail("card.weight", Where + " has a non-positive draw weight");
				}
				// Lower is better for these: kills between bombs, seconds between puddles, speed left to the mice.
				const bool bDescending = K.Effect == CardEffect::CATNIP_BOMB_EVERY || K.Effect == CardEffect::POOP_TRAIL
					|| K.Effect == CardEffect::AURA_SLOW;
				for (size_t I = 1; I < K.LevelValues.size(); ++I)
				{
					const bool bBetter = bDescending ? K.LevelValues[I] < K.LevelValues[I - 1] : K.LevelValues[I] > K.LevelValues[I - 1];
					if (!bBetter)
					{
						C.Fail("card.monotone", Where + " level " + std::to_string(I + 1) + " is not better than level " + std::to_string(I));
						break;
					}
				}
				++PerFlavour[K.Flavour];
				if (K.Cost != CardCost::NONE)
				{
					++CostsPerFlavour[K.Flavour];
				}
			}
			for (const auto& [Flavour, Count] : PerFlavour)
			{
				if (Count != CardsPerFlavour)
				{
					C.Fail("card.deck_shape", std::string("flavour ") + ToName(Flavour) + " holds " + std::to_string(Count) + " cards, expected 4");
				}
				const int Want = Flavour == CardFlavour::DECK ? 0 : CostCardsPerFlavour;
				if (CostsPerFlavour[Flavour] != Want)
				{
					C.Fail("card.deck_shape", std::string("flavour ") + ToName(Flavour) + " has " + std::to_string(CostsPerFlavour[Flavour])
						+ " cost cards, expected " + std::to_string(Want));
				}
			}
		}

		// --- cross-cutting checks ----------------------------------------------------------------------------------------

		double ReturnExponent(const PcUpgrade& U)
		{
			return std::log(U.EffectPerLevel) / std::log(U.CostGrowth);
		}

		void CheckCrossCutting(Checker& C, const PcGameData& D)
		{
			// The shape of a damage line is its exponent: the kittens may compound hotter than the paw, never colder.
			const PcUpgrade* Paw = D.FindUpgrade("paw_damage");
			const PcUpgrade* Kitten = D.FindUpgrade("kitten_power");
			if (!Paw || !Kitten)
			{
				C.Fail("upgrade.damage_lines", "paw_damage or kitten_power is missing");
			}
			else if (ReturnExponent(*Kitten) < ReturnExponent(*Paw) - KittenExponentSlack)
			{
				C.Fail("upgrade.damage_lines", "kitten_power's return exponent " + Num(ReturnExponent(*Kitten)) + " is under paw_damage's "
					+ Num(ReturnExponent(*Paw)) + "; the kittens fall behind and become dead money");
			}

			// The wave ladder ramps across biomes and opens cheap.
			std::vector<double> Waves;
			for (const PcTier& T : D.Tiers)
			{
				if (T.KillsPerWave > 0)
				{
					Waves.push_back(static_cast<double>(T.KillsPerWave));
				}
			}
			if (Waves.size() < 2)
			{
				C.Fail("tier.wave_ramp", "fewer than two biomes carry a wave ladder");
			}
			else
			{
				for (size_t I = 1; I < Waves.size(); ++I)
				{
					if (Waves[I] < Waves[I - 1])
					{
						C.Fail("tier.wave_ramp", "kills_per_wave falls between biome " + std::to_string(I) + " and " + std::to_string(I + 1));
					}
				}
				if (Waves.front() > WaveLadderMaxFirst)
				{
					C.Fail("tier.wave_ramp", "the first biome's wave 1 needs " + Num(Waves.front()) + " kills, over the 700 ceiling");
				}
				if (Waves.back() / Waves.front() < WaveLadderMinSpread)
				{
					C.Fail("tier.wave_ramp", "the wave ladder spans only x" + Num(Waves.back() / Waves.front()) + ", under x4");
				}
			}

			// Every biome spawns faster than the one before it, from the Alley on, strictly.
			for (size_t I = SpawnLadderFirstStep; I < D.Tiers.size(); ++I)
			{
				if (D.Tiers[I].SpawnRatePerMin <= D.Tiers[I - 1].SpawnRatePerMin)
				{
					C.Fail("tier.spawn_ladder", "spawn rate goes " + Num(D.Tiers[I - 1].SpawnRatePerMin) + " -> " + Num(D.Tiers[I].SpawnRatePerMin)
						+ " between biome " + std::to_string(I) + " and " + std::to_string(I + 1));
				}
			}

			// The exponent budget over uncapped multiplicative lines (capped lines are exempt on purpose).
			double Total = 0.0;
			for (const PcUpgrade& U : D.Upgrades)
			{
				if (U.EffectType != UpgradeEffectType::MULTIPLICATIVE || U.Cap >= 0.0 || U.EffectPerLevel <= 1.0 || U.CostGrowth <= 1.0)
				{
					continue;
				}
				Total += ReturnExponent(U);
			}
			if (Total > ExponentBudget)
			{
				C.Fail("upgrade.exponent_budget", "the uncapped multiplicative lines sum to a return exponent of " + Num(Total) + " against 0.80");
			}

			// D17 E3: Catnip x the Reawaken multiplier ^ reawaken_catnip_exponent. At 1 or more, Catnip feeds the Stardust
			// grant, which feeds the multiplier: runs collapsed to 1-9 minutes (measured, docs/ECONOMY-REVIEW-2026-11-12.md).
			const double CatnipExponent = D.Reawaken.ReawakenCatnipExponent;
			if (!(CatnipExponent >= 0.0 && CatnipExponent < 1.0))
			{
				C.Fail("reawaken.catnip_exponent", "reawaken_catnip_exponent " + Num(CatnipExponent) + " is not in [0, 1)");
			}
		}
	}

	namespace
	{
		// --- the constants that live in Godot code (CodeConstants.csv) ----------------------------------------------------

		void CheckCodeConstants(Checker& C, const PcGameData& D)
		{
			const PcCodeConstants& K = D.Code;
			// GameState: stage 1 is the baseline, each boss grants one, the ending the last; spelled out in Godot rather
			// than derived, so the two numbers must be kept in step by hand.
			if (K.EvolutionStageCount != static_cast<int64_t>(K.BossRealms.size()) + 2)
			{
				C.Fail("code.stage_ladder", "evolution_stage_count " + std::to_string(K.EvolutionStageCount) + " is not the "
					+ std::to_string(K.BossRealms.size()) + " bosses + the baseline + the ending");
			}
			if (K.MountedStage < 2 || K.MountedStage > K.EvolutionStageCount)
			{
				C.Fail("code.stage_ladder", "mounted_stage " + std::to_string(K.MountedStage) + " is not a stage the cat can earn");
			}
			// Every number is finite and positive: each is a count, a period, a distance, a cap or a growth.
			for (const auto& [Id, Value] : CodeConstantNumbers(K))
			{
				if (!std::isfinite(Value) || Value <= 0.0)
				{
					C.Fail("code.range", Id + " = " + Num(Value) + "; every code constant is a positive amount");
				}
			}
			if (K.MaxGoldenChance >= 1.0)
			{
				C.Fail("code.range", "max_golden_chance " + Num(K.MaxGoldenChance) + ": at 1.0 every mouse is golden");
			}
			if (K.CardLevelCostGrowth <= 1.0)
			{
				C.Fail("code.range", "card_level_cost_growth " + Num(K.CardLevelCostGrowth) + ": the XP curve must grow");
			}
			// A forgiveness, not a discount: a thousandth of a Catnip is already visible on the shop's two decimals.
			if (K.CurrencyEpsilon >= 1e-3)
			{
				C.Fail("code.range", "currency_epsilon " + Num(K.CurrencyEpsilon) + " is a discount, not a rounding allowance");
			}
			if (K.SecondHelpingBiomeIndex >= static_cast<int64_t>(D.Tiers.size()))
			{
				C.Fail("code.range", "second_helping_biome_index " + std::to_string(K.SecondHelpingBiomeIndex) + " is past the last biome");
			}
			if (K.LastStandThreshold >= K.CrowdThreshold)
			{
				C.Fail("code.range", "Last Stand (under " + std::to_string(K.LastStandThreshold) + ") and Swarm Logic (from "
					+ std::to_string(K.CrowdThreshold) + ") overlap: owning both would be a permanent buff");
			}
		}
	}

	const std::vector<std::string>& ValidatorRules()
	{
		static const std::vector<std::string> Rules = {
			"ids.empty", "ids.duplicate", "ids.lower_case",
			"job.neutral", "job.free_gift", "job.no_name", "job.no_description", "job.wrong_art", "job.drop_and_gate", "ref.job_biome",
			"upgrade.cap_growth_on_uncapped", "upgrade.cap_growth_negative", "upgrade.fraction_ceiling", "upgrade.runaway",
			"tier.spawn_pool", "ref.spawn_pool", "tier.reward_multiplier", "tier.catnip_to_unlock", "tier.golden", "ref.golden",
			"tier.wave_ladder", "tier.spawn_rate", "tier.stage_gate", "ref.boss", "boss.hp", "boss.bait_price", "boss.bait_gate",
			"tier.boss_realms", "boss.phases",
			"enemy.hp", "enemy.reward_scaling",
			"evolution.ladder", "evolution.weapon", "evolution.art", "evolution.ceiling",
			"achievement.threshold", "achievement.target", "achievement.stardust",
			"branch.levels", "ref.branch_parent", "branch.parent_required", "ref.branch_exclusive", "ref.branch_job", "ref.branch_boss", "branch.cycle",
			"card.levels", "card.description", "card.cost", "card.weight", "card.monotone", "card.deck_shape",
			"upgrade.damage_lines", "tier.wave_ramp", "tier.spawn_ladder", "upgrade.exponent_budget", "reawaken.catnip_exponent",
			"code.stage_ladder", "code.range",
		};
		return Rules;
	}

	const std::vector<std::string>& KnownGodotDataIssues()
	{
		// Empty since 2026-11-03: the two achievements counting merged-away branches (br_ferocity_5, br_mystique_6, F2) were
		// removed in Godot by the designer's decision. A new entry goes here only with a finding in doc 01 §5.
		static const std::vector<std::string> Issues = {};
		return Issues;
	}

	std::vector<std::string> ValidateGameData(const PcGameData& D)
	{
		Checker C;
		UniqueIds(C, D.Tiers, "TierData");
		UniqueIds(C, D.Enemies, "EnemyData");
		UniqueIds(C, D.Bosses, "BossData");
		UniqueIds(C, D.Upgrades, "UpgradeData");
		UniqueIds(C, D.Branches, "SkillBranchData");
		UniqueIds(C, D.Evolutions, "EvolutionData");
		UniqueIds(C, D.Cards, "CardData");
		UniqueIds(C, D.Jobs, "JobData");
		UniqueIds(C, D.Achievements, "AchievementData");
		CheckJobs(C, D);
		CheckUpgrades(C, D);
		CheckTiers(C, D);
		for (const PcBoss& B : D.Bosses)
		{
			CheckBossPhases(C, B, "boss " + B.Id);
		}
		CheckEnemies(C, D);
		CheckEvolution(C, D);
		CheckAchievements(C, D);
		CheckBranches(C, D);
		CheckCards(C, D);
		CheckCrossCutting(C, D);
		CheckCodeConstants(C, D);
		return C.Failures;
	}
}
