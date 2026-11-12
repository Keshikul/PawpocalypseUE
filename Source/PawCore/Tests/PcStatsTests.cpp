#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcJobs.h"
#include "PcStats.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	const Pc::PcGameData& Game()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(DataDir(), Data, Errors));
			bLoaded = true;
		}
		return Data;
	}

	std::vector<std::string> Split(const std::string& S, char Sep)
	{
		std::vector<std::string> Out;
		std::string Cur;
		std::istringstream In(S);
		while (std::getline(In, Cur, Sep))
		{
			Out.push_back(Cur);
		}
		return Out;
	}

	std::map<std::string, int64_t> Pairs(const std::string& S)
	{
		std::map<std::string, int64_t> Out;
		for (const std::string& P : Split(S, ';'))
		{
			const size_t Eq = P.find('=');
			if (Eq != std::string::npos)
			{
				Out[P.substr(0, Eq)] = std::atoll(P.c_str() + Eq + 1);
			}
		}
		return Out;
	}

	double Ulps(double A, double B)
	{
		if (A == B)
		{
			return 0.0;
		}
		return std::fabs(A - B) / std::fabs(std::nextafter(A, INFINITY) - A);
	}

	struct StatsFixtureResult
	{
		int Rows = 0;
		int ExactFields = 0;
		int UlpFields = 0;
		double WorstUlps = 0.0;
	};

	StatsFixtureResult RunStatsFixture(const std::string& Path)
	{
		StatsFixtureResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		const Pc::PcGameData& D = Game();
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			// Keep empty cells (a bare split on '\t' with getline would drop a trailing empty one).
			std::vector<std::string> Cells;
			size_t Start = 0;
			while (true)
			{
				const size_t Tab = Line.find('\t', Start);
				Cells.push_back(Line.substr(Start, Tab == std::string::npos ? std::string::npos : Tab - Start));
				if (Tab == std::string::npos)
				{
					break;
				}
				Start = Tab + 1;
			}
			if (Header.empty())
			{
				Header = Cells;
				continue;
			}
			REQUIRE(Cells.size() == Header.size());
			std::map<std::string, std::string> V;
			for (size_t I = 0; I < Cells.size(); ++I)
			{
				V[Header[I]] = Cells[I];
			}
			Pc::PcProgress P;
			P.UpgradeLevels = Pairs(V.at("upgrades"));
			P.BranchLevels = Pairs(V.at("branches"));
			P.CardLevels = Pairs(V.at("cards"));
			P.RunJob = V.at("run_job");
			P.RunJobSecond = V.at("run_job_second");
			P.ReawakenCount = std::atoll(V.at("reawaken_count").c_str());
			P.DeepestBiomeIndex = std::atoll(V.at("deepest").c_str());
			for (const std::string& B : Split(V.at("bosses"), ';'))
			{
				if (!B.empty())
				{
					P.BossesDefeated.insert(B);
				}
			}
			P.bFinalBossDefeated = V.at("final") == "1";
			P.StardustGrantedEver = std::strtod(V.at("stardust").c_str(), nullptr);
			P.WavesHere = std::atoll(V.at("waves").c_str());
			P.CardsTakenThisRun = std::atoll(V.at("cards_taken").c_str());
			P.KillsThisCycle = std::atoll(V.at("kills").c_str());

			const Pc::PcCombatStats S = Pc::RebuildStats(D, P);
			CAPTURE(R.Rows);
			CHECK(S.bAuraOwned == (V.at("aura_owned") == "1"));
			const std::pair<const char*, double> Fields[] = {
				{ "paw_damage_multiplier", S.PawDamageMultiplier }, { "attack_speed_multiplier", S.AttackSpeedMultiplier },
				{ "shred_per_stack", S.ShredPerStack }, { "execute_threshold", S.ExecuteThreshold },
				{ "aura_damage_multiplier", S.AuraDamageMultiplier }, { "aura_radius_branch_multiplier", S.AuraRadiusBranchMultiplier },
				{ "evolution_damage_multiplier", S.EvolutionDamageMultiplier }, { "weapon_multiplier", S.WeaponMultiplier },
				{ "additive_damage_sum", S.AdditiveDamageSum }, { "multiplicative_damage_product", S.MultiplicativeDamageProduct },
				{ "crit_chance", S.CritChance }, { "crit_multiplier", S.CritMultiplier }, { "overkill_fraction", S.OverkillFraction },
				{ "kitten_power_multiplier", S.KittenPowerMultiplier }, { "kitten_additive_damage_sum", S.KittenAdditiveDamageSum },
				{ "kitten_multiplicative_damage_product", S.KittenMultiplicativeDamageProduct },
				{ "poop_power_multiplier", S.PoopPowerMultiplier }, { "poop_additive_damage_sum", S.PoopAdditiveDamageSum },
				{ "poop_multiplicative_damage_product", S.PoopMultiplicativeDamageProduct },
				{ "kitten_speed_multiplier", S.KittenSpeedMultiplier }, { "shield_pierce_fraction", S.ShieldPierceFraction },
				{ "pounce_power_multiplier", S.PouncePowerMultiplier }, { "pounce_cooldown_reduction", S.PounceCooldownReduction },
				{ "reawaken_multiplier", S.ReawakenMultiplier },
			};
			for (const auto& [Name, Got] : Fields)
			{
				const double Want = std::strtod(V.at(Name).c_str(), nullptr);
				if (Got == Want)
				{
					++R.ExactFields;
					continue;
				}
				++R.UlpFields;
				const double U = Ulps(Got, Want);
				R.WorstUlps = std::max(R.WorstUlps, U);
				// pow() is the only operation that differs: Godot's MinGW build calls the legacy msvcrt pow, which is a few
				// ulps less accurate than MSVC's (measured up to ~11 ulps on effect ^ level at level 40). 1e-13 relative is
				// a hundred times tighter than anything a player could see, and far looser than any real formula drift.
				const double Relative = std::fabs(Got - Want) / std::max(std::fabs(Want), 1e-300);
				CHECK_MESSAGE(Relative <= 1e-13, std::string(Name) << ": PawCore " << Got << " vs Godot " << Want << " (" << U << " ulps)");
			}
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcStats matches Godot's GameState._rebuild_stats() on 1,000 random progress states")
{
	const StatsFixtureResult R = RunStatsFixture(DataDir() + "/../Source/PawCore/Tests/Data/stats_godot.tsv");
	CHECK(R.Rows == 1000);
	MESSAGE("fields equal bit for bit: " << R.ExactFields << ", within a few ulps (pow): " << R.UlpFields << ", worst " << R.WorstUlps << " ulps");
}

TEST_CASE("Long: PcStats matches all 3,000 Godot _rebuild_stats() rows" * doctest::test_suite("long") * doctest::skip())
{
	// build/stats_godot_full.tsv comes from `py tools/make_godot_fixtures.py` (the real Godot code; not in git).
	const StatsFixtureResult R = RunStatsFixture(DataDir() + "/../build/stats_godot_full.tsv");
	CHECK(R.Rows == 3000);
	MESSAGE("fields equal bit for bit: " << R.ExactFields << ", within a few ulps (pow): " << R.UlpFields << ", worst " << R.WorstUlps << " ulps");
}

TEST_CASE("Stats rules from the Godot comments")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P;
	// A fresh save: no aura, stage 1, no multipliers anywhere.
	Pc::PcCombatStats S = Pc::RebuildStats(D, P);
	CHECK_FALSE(S.bAuraOwned);
	CHECK(S.EvolutionDamageMultiplier == 1.0);
	CHECK(S.ReawakenMultiplier == 1.0);
	// The boss ladder stops at the first gap.
	P.BossesDefeated = { "tier_3", "tier_8" };
	CHECK(Pc::EvolutionStage(D, P) == 2);
	P.BossesDefeated = { "tier_3", "tier_6", "tier_8", "tier_11", "tier_12" };
	P.bFinalBossDefeated = true;
	CHECK(Pc::EvolutionStage(D, P) == 7);
	// The Reawaken multiplier keys off Stardust EVER granted, as a power law.
	CHECK(Pc::ReawakenMultiplier(D.Reawaken, 0.0) == 1.0);
	CHECK(Pc::ReawakenMultiplier(D.Reawaken, D.Reawaken.ReawakenMultScale * 3.0) == doctest::Approx(std::pow(4.0, D.Reawaken.ReawakenMultExponent)));
	// Union Rules: a second job at half share turns x2.5 into x1.75, not x1.25.
	Pc::PcProgress J;
	J.RunJobSecond = "blacksmith";
	CHECK(Pc::JobTerm(D, J, &Pc::PcJob::PawDamageMultiplier) == 1.0); // no Union Rules owned: nothing
	// Caps grow per floor and per Reawaken, and never apply to an uncapped line.
	const Pc::PcUpgrade* Overkill = D.FindUpgrade("overkill");
	REQUIRE(Overkill);
	Pc::PcProgress Deep;
	Deep.DeepestBiomeIndex = 7;
	Deep.ReawakenCount = 12;
	CHECK(Pc::EffectiveCap(*Overkill, Deep) == doctest::Approx(Overkill->Cap + Overkill->CapPerFloor * 7.0 + Overkill->CapPerReawaken * 12.0));
	const Pc::PcUpgrade* Paw = D.FindUpgrade("paw_damage");
	REQUIRE(Paw);
	CHECK(Pc::EffectiveCap(*Paw, Deep) == -1.0);
}

TEST_CASE("F3: Patience reads the job's slowness only; Attack Speed never lowers the Sphinx's damage")
{
	// D16, designer 2026-11-12. Godot paid Patience for the cat's FINAL attack speed, so every Attack Speed level shrank it:
	// with the aura, from Patience 5 every level LOWERED damage (doc 01 §5 F3). The port reads the job's speed only.
	const Pc::PcGameData& D = Game();
	const Pc::PcSkillBranch* Patience = D.FindBranch("patience");
	const Pc::PcJob* Sphinx = D.FindJob("sphinx");
	REQUIRE(Patience);
	REQUIRE(Sphinx);
	REQUIRE(Patience->LevelEffects.size() == 10);
	CHECK(Patience->LevelEffects.back() == doctest::Approx(0.6)); // +6% a level (the designer's numbers, Data/ via godot_fixes.py)
	REQUIRE(Sphinx->AttackSpeedMultiplier < 1.0);

	Pc::PcProgress P;
	P.RunJob = "sphinx";
	P.BranchLevels["patience"] = 10;
	P.BranchLevels["unlock_aura"] = 1;
	P.DeepestBiomeIndex = 11;
	P.ReawakenCount = 9;
	const double Want = D.Combat.CritMultiplier * Sphinx->CritDamageMultiplier
		* (1.0 + Patience->LevelEffects.back() * (1.0 / Sphinx->AttackSpeedMultiplier - 1.0));
	double LastDps = 0.0;
	for (int64_t Level = 0; Level <= 6; ++Level)
	{
		P.UpgradeLevels["attack_speed"] = Level;
		const Pc::PcCombatStats S = Pc::RebuildStats(D, P);
		INFO("Attack Speed level " << Level << ", attack speed x" << S.AttackSpeedMultiplier);
		CHECK(S.CritMultiplier == doctest::Approx(Want).epsilon(1e-12)); // the shop's levels never move it
		const double Dps = Pc::PlayerDps(D.Combat, S);
		CHECK(Dps >= LastDps); // and so a level never lowers paw + aura damage
		LastDps = Dps;
	}
	// A job at full speed gets nothing from it, nor does a run with no job.
	P.UpgradeLevels.clear();
	P.RunJob = "blacksmith";
	CHECK(Pc::RebuildStats(D, P).CritMultiplier == doctest::Approx(D.Combat.CritMultiplier * Pc::JobTerm(D, P, &Pc::PcJob::CritDamageMultiplier)));
	P.RunJob.clear();
	CHECK(Pc::RebuildStats(D, P).CritMultiplier == doctest::Approx(D.Combat.CritMultiplier));
}

TEST_CASE("D17 EU: owning the Unicorn multiplies paw and aura damage by x1.5, never the kittens; E4 and E5's numbers")
{
	// Designer, 2026-11-12: "Unicorn should be a damage step" (x1.5). It cost 2,500 Stardust and a ceremony and gave +13 %
	// with Rainbow Poop. The multiplicative bracket, shared by the paw and the aura.
	const Pc::PcGameData& D = Game();
	const Pc::PcSkillBranch* Unicorn = D.FindBranch("unlock_unicorn");
	REQUIRE(Unicorn);
	REQUIRE(Unicorn->LevelEffects.size() == 1);
	CHECK(Unicorn->LevelEffects[0] == doctest::Approx(1.5));
	CHECK(Unicorn->EffectType == Pc::BranchEffectType::DAMAGE_MULTIPLICATIVE); // the tooltip reads "x1.5 damage"
	Pc::PcProgress P;
	P.BranchLevels["unlock_aura"] = 1;
	P.BranchLevels["claws"] = 3;
	P.UpgradeLevels["kitten_count"] = 2;
	const Pc::PcCombatStats Without = Pc::RebuildStats(D, P);
	P.BranchLevels["unlock_unicorn"] = 1;
	const Pc::PcCombatStats With = Pc::RebuildStats(D, P);
	CHECK(With.MultiplicativeDamageProduct == doctest::Approx(Without.MultiplicativeDamageProduct * 1.5));
	CHECK(Pc::PawDps(D.Combat, With) == doctest::Approx(Pc::PawDps(D.Combat, Without) * 1.5));
	CHECK(Pc::AuraDps(D.Combat, With) == doctest::Approx(Pc::AuraDps(D.Combat, Without) * 1.5));
	CHECK(Pc::KittenDps(D.Combat, With) == doctest::Approx(Pc::KittenDps(D.Combat, Without)));
	// E4: bigger kitten buys (the alternative measured 2026-10-01); E5: the Overkill unlock splashes 10 %.
	const Pc::PcUpgrade* Power = D.FindUpgrade("kitten_power");
	const Pc::PcUpgrade* Count = D.FindUpgrade("kitten_count");
	REQUIRE(Power);
	REQUIRE(Count);
	CHECK(Power->EffectPerLevel == doctest::Approx(1.6));
	CHECK(Power->CostGrowth == doctest::Approx(3.6372));
	CHECK(Count->EffectPerLevel == doctest::Approx(2.0));
	CHECK(Count->CostGrowth == doctest::Approx(1.35));
	const Pc::PcSkillBranch* Overkill = D.FindBranch("unlock_overkill");
	REQUIRE(Overkill);
	CHECK(Overkill->LevelEffects.at(0) == doctest::Approx(0.1));
}

#endif
