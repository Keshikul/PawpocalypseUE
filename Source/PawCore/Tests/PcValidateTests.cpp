#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcData.h"
#include "PcValidate.h"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>

namespace
{
	const Pc::PcGameData& Clean()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			const char* Env = std::getenv("PAWCORE_DATA_DIR");
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(Env ? Env : "Data", Data, Errors));
			bLoaded = true;
		}
		return Data;
	}

	template <typename RowT>
	RowT& ById(std::vector<RowT>& Rows, const std::string& Id)
	{
		for (RowT& R : Rows)
		{
			if (R.Id == Id)
			{
				return R;
			}
		}
		FAIL("no row " << Id);
		return Rows.front();
	}

	template <typename RowT, typename PredT>
	RowT& First(std::vector<RowT>& Rows, PredT Pred)
	{
		for (RowT& R : Rows)
		{
			if (Pred(R))
			{
				return R;
			}
		}
		FAIL("no row matches the test's precondition");
		return Rows.front();
	}

	// One deliberate break per rule. Each must make the validator report that rule's tag.
	const std::map<std::string, std::function<void(Pc::PcGameData&)>>& Sabotage()
	{
		using namespace Pc;
		static const std::map<std::string, std::function<void(PcGameData&)>> S = {
			{ "ids.empty", [](PcGameData& D) { D.Achievements[0].Id.clear(); } },
			{ "code.stage_ladder", [](PcGameData& D) { D.Code.BossRealms.pop_back(); } },
			{ "code.range", [](PcGameData& D) { D.Code.MaxGoldenChance = 1.0; } },
			{ "ids.duplicate", [](PcGameData& D) { D.Cards[1].Id = D.Cards[0].Id; } },
			{ "ids.lower_case", [](PcGameData& D) { D.Evolutions[2].Id = "Stage_3"; } },
			{ "job.neutral", [](PcGameData& D) { PcJob& J = ById(D.Jobs, "blacksmith"); J.PawDamageMultiplier = 1.0; J.AuraRadiusMultiplier = 1.0; } },
			{ "job.free_gift", [](PcGameData& D) { ById(D.Jobs, "blacksmith").AuraRadiusMultiplier = 1.0; } },
			{ "job.no_name", [](PcGameData& D) { ById(D.Jobs, "blacksmith").DisplayName.clear(); } },
			{ "job.no_description", [](PcGameData& D) { ById(D.Jobs, "blacksmith").Description.clear(); } },
			{ "job.wrong_art", [](PcGameData& D) { ById(D.Jobs, "blacksmith").HatPath = "res://assets/art/jobs/hat_magician.svg"; } },
			{ "job.drop_and_gate", [](PcGameData& D) { First(D.Jobs, [](const PcJob& J) { return !J.DroppedByBiome.empty(); }).RequiredReawakens = 1; } },
			{ "ref.job_biome", [](PcGameData& D) { First(D.Jobs, [](const PcJob& J) { return !J.DroppedByBiome.empty(); }).DroppedByBiome = "tier_99"; } },
			{ "upgrade.cap_growth_on_uncapped", [](PcGameData& D) { First(D.Upgrades, [](const PcUpgrade& U) { return U.Cap < 0.0; }).CapPerFloor = 0.1; } },
			{ "upgrade.cap_growth_negative", [](PcGameData& D) { First(D.Upgrades, [](const PcUpgrade& U) { return U.Cap >= 0.0; }).CapPerFloor = -0.1; } },
			{ "upgrade.fraction_ceiling", [](PcGameData& D) { First(D.Upgrades, [](const PcUpgrade& U) { return U.Cap >= 0.0 && U.Cap <= 1.0; }).Cap = 0.89; } },
			{ "upgrade.runaway", [](PcGameData& D)
				{
					PcUpgrade& U = First(D.Upgrades, [](const PcUpgrade& X) { return X.Cap < 0.0 && X.EffectType == UpgradeEffectType::MULTIPLICATIVE; });
					U.EffectPerLevel = U.CostGrowth;
				} },
			{ "tier.spawn_pool", [](PcGameData& D) { D.Tiers[0].SpawnWeights.pop_back(); } },
			{ "ref.spawn_pool", [](PcGameData& D) { D.Tiers[0].SpawnPool[0] = "ghost_mouse"; } },
			{ "tier.reward_multiplier", [](PcGameData& D) { D.Tiers[1].RewardMultiplier = 4.5; } },
			{ "tier.catnip_to_unlock", [](PcGameData& D) { D.Tiers[1].CatnipToUnlock = -1.0; } },
			{ "tier.golden", [](PcGameData& D) { D.Tiers[0].GoldenChance = 0.01; D.Tiers[0].GoldenEnemy.clear(); } },
			{ "ref.golden", [](PcGameData& D) { D.Tiers[0].GoldenEnemy = "no_such_golden"; } },
			{ "tier.wave_ladder", [](PcGameData& D) { D.Tiers[2].WaveHpGrowth = 1.0; } },
			{ "tier.spawn_rate", [](PcGameData& D) { D.Tiers[5].SpawnRatePerMin = 0.0; } },
			{ "tier.stage_gate", [](PcGameData& D) { D.Tiers[0].RequiredEvolutionStage = 2; } },
			{ "ref.boss", [](PcGameData& D) { ById(D.Tiers, "tier_3").BossData = "nobody"; } },
			{ "boss.hp", [](PcGameData& D) { D.Bosses[0].TotalHp = 0.0; } },
			{ "boss.bait_price", [](PcGameData& D) { D.Bosses[0].BaitCostCatnip = 0.0; } },
			{ "boss.bait_gate", [](PcGameData& D) { D.Bosses[0].KillsPerBait = 0; } },
			{ "tier.boss_realms", [](PcGameData& D) { D.Tiers[0].BossData = D.Bosses[0].Id; } },
			{ "boss.phases", [](PcGameData& D) { D.Bosses[0].PhaseShieldSec.pop_back(); } },
			{ "enemy.hp", [](PcGameData& D) { First(D.Enemies, [](const PcEnemy& E) { return E.Tier > 1 && !E.bRewardExempt; }).Hp = 0.0; } },
			{ "enemy.reward_scaling", [](PcGameData& D) { ById(D.Enemies, "alley_dog").RewardCatnip *= 1.05; } },
			{ "evolution.ladder", [](PcGameData& D) { D.Evolutions.erase(D.Evolutions.begin() + 3); } },
			{ "evolution.weapon", [](PcGameData& D) { D.Evolutions[3].WeaponMultiplier = D.Evolutions[2].WeaponMultiplier; } },
			{ "evolution.art", [](PcGameData& D) { D.Evolutions[2].TexturePath.clear(); } },
			{ "evolution.ceiling", [](PcGameData& D) { D.Combat.EvolutionDamagePerStage = 2.0; } },
			{ "achievement.threshold", [](PcGameData& D) { D.Achievements[0].Threshold = 0.0; } },
			{ "achievement.target", [](PcGameData& D) { D.Achievements[0].Metric = AchievementMetric::UPGRADE_LEVEL; D.Achievements[0].UpgradeId = "no_such_upgrade"; } },
			{ "achievement.stardust", [](PcGameData& D) { D.Achievements[0].StardustReward = 13; } },
			{ "branch.levels", [](PcGameData& D) { D.Branches[0].LevelEffects.pop_back(); } },
			{ "ref.branch_parent", [](PcGameData& D) { First(D.Branches, [](const PcSkillBranch& B) { return !B.Parent.empty(); }).Parent = "no_such_branch"; } },
			{ "branch.parent_required", [](PcGameData& D) { First(D.Branches, [](const PcSkillBranch& B) { return !B.Parent.empty(); }).bParentIsRequired = false; } },
			{ "ref.branch_exclusive", [](PcGameData& D) { D.Branches[0].ExclusiveWith = { "no_such_branch" }; } },
			{ "ref.branch_job", [](PcGameData& D) { First(D.Branches, [](const PcSkillBranch& B) { return B.EffectType == BranchEffectType::GRANTS_JOB; }).JobId = "no_such_job"; } },
			{ "ref.branch_boss", [](PcGameData& D) { First(D.Branches, [](const PcSkillBranch& B) { return !B.GatedByBoss.empty(); }).GatedByBoss = "tier_4"; } },
			{ "branch.cycle", [](PcGameData& D)
				{
					PcSkillBranch& Child = ById(D.Branches, "anvil_paw");
					PcSkillBranch& Parent = ById(D.Branches, Child.Parent);
					Parent.Parent = Child.Id;
					Parent.bParentIsRequired = true;
				} },
			{ "card.levels", [](PcGameData& D) { D.Cards[0].LevelValues.clear(); } },
			{ "card.description", [](PcGameData& D) { D.Cards[0].Description = "No number here."; } },
			{ "card.cost", [](PcGameData& D) { First(D.Cards, [](const PcCard& K) { return K.Cost != CardCost::NONE; }).CostValues.pop_back(); } },
			{ "card.weight", [](PcGameData& D) { D.Cards[0].Weight = 0.0; } },
			{ "card.monotone", [](PcGameData& D) { D.Cards[0].LevelValues[1] = D.Cards[0].LevelValues[0]; } },
			{ "card.deck_shape", [](PcGameData& D) { D.Cards.pop_back(); } },
			{ "upgrade.damage_lines", [](PcGameData& D) { ById(D.Upgrades, "kitten_power").EffectPerLevel = 1.0001; } },
			{ "tier.wave_ramp", [](PcGameData& D) { D.Tiers[0].KillsPerWave = 800; } },
			{ "tier.spawn_ladder", [](PcGameData& D) { D.Tiers[3].SpawnRatePerMin = D.Tiers[2].SpawnRatePerMin; } },
			{ "upgrade.exponent_budget", [](PcGameData& D)
				{
					PcUpgrade& U = First(D.Upgrades, [](const PcUpgrade& X) { return X.EffectType == UpgradeEffectType::MULTIPLICATIVE && X.CostGrowth > 1.0; });
					U.Cap = -1.0;
					U.CapPerFloor = 0.0;
					U.CapPerReawaken = 0.0;
					U.EffectPerLevel = std::pow(U.CostGrowth, 0.95);
				} },
		};
		return S;
	}

	size_t Count(const std::vector<std::string>& Failures, const std::string& Rule)
	{
		size_t N = 0;
		for (const std::string& F : Failures)
		{
			N += F.rfind("[" + Rule + "]", 0) == 0 ? 1 : 0;
		}
		return N;
	}

	// Real problems in the SHIPPING Godot data, found by this validator. They are fixed in the Godot project and
	// re-exported (no design or data changes during the port, CLAUDE.md); until then they are listed here, exactly, so
	// any NEW failure still fails the gate, and the test below fails when a listed one disappears (remove it then).
	// The list lives in PawCore (Pc::KnownGodotDataIssues): the game's adapter tolerates exactly the same failures.
	const std::vector<std::string>& KnownGodotDataIssues = Pc::KnownGodotDataIssues();
}

TEST_CASE("The shipped data passes the validator, apart from the listed Godot data issues")
{
	const std::vector<std::string> Failures = Pc::ValidateGameData(Clean());
	for (const std::string& F : Failures)
	{
		const bool bKnown = std::find(KnownGodotDataIssues.begin(), KnownGodotDataIssues.end(), F) != KnownGodotDataIssues.end();
		CHECK_MESSAGE(bKnown, "new validator failure: " << F);
	}
	for (const std::string& K : KnownGodotDataIssues)
	{
		CHECK_MESSAGE(std::find(Failures.begin(), Failures.end(), K) != Failures.end(), "fixed in the data? remove it from the list: " << K);
	}
}

TEST_CASE("Every validator rule has a negative test")
{
	for (const std::string& Rule : Pc::ValidatorRules())
	{
		CHECK_MESSAGE(Sabotage().count(Rule) == 1, "no sabotage for rule " << Rule);
	}
	for (const auto& Entry : Sabotage())
	{
		const auto& Rules = Pc::ValidatorRules();
		CHECK_MESSAGE(std::find(Rules.begin(), Rules.end(), Entry.first) != Rules.end(), "sabotage for unknown rule " << Entry.first);
	}
}

TEST_CASE("Each rule fires on its sabotage (and the clean data does not)")
{
	const std::vector<std::string> Baseline = Pc::ValidateGameData(Clean());
	for (const auto& [Rule, Break] : Sabotage())
	{
		CAPTURE(Rule);
		Pc::PcGameData Broken = Clean();
		Break(Broken);
		Broken.RebuildIndex();
		const std::vector<std::string> Failures = Pc::ValidateGameData(Broken);
		// The break must ADD failures of its own rule (a rule may already fire on a listed Godot data issue).
		CHECK_MESSAGE(Count(Failures, Rule) > Count(Baseline, Rule), "rule " << Rule << " did not fire on its sabotage");
	}
}

#endif
