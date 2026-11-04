#if defined(PAWCORE_STANDALONE_TESTS)

// The Pounce's pure rules against the REAL Godot code (tools/godot_fixtures/pounce_fixture.gd): GameState's Pounce, Reflex
// and Daredevil getters, the Pounce cards' values, and CombatResolver's pounce cooldown and damage (vulnerable or not, at
// x1 and at the auto-pounce multiplier).

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcFieldRules.h"
#include "PcJobs.h"

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

	std::vector<std::string> SplitKeep(const std::string& S, char Sep)
	{
		std::vector<std::string> Out;
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find(Sep, Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	std::map<std::string, int64_t> Pairs(const std::string& S)
	{
		std::map<std::string, int64_t> Out;
		for (const std::string& P : SplitKeep(S, ';'))
		{
			const size_t Eq = P.find('=');
			if (Eq != std::string::npos)
			{
				Out[P.substr(0, Eq)] = std::atoll(P.c_str() + Eq + 1);
			}
		}
		return Out;
	}

	double Hex(const std::string& S)
	{
		return std::strtod(S.c_str(), nullptr);
	}

	bool NearPow(double Got, double Want)
	{
		return Got == Want || std::fabs(Got - Want) <= 1e-13 * std::fabs(Want);
	}

	struct PounceResult
	{
		int Rows = 0;
		std::map<std::string, int> Paths;
	};

	PounceResult RunPounceFixture(const std::string& Path)
	{
		PounceResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		const Pc::PcGameData& D = Game();
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = SplitKeep(Line, '\t');
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
			CAPTURE(R.Rows);

			Pc::PcProgress P;
			P.UpgradeLevels = Pairs(V.at("upgrades"));
			P.BranchLevels = Pairs(V.at("branches"));
			P.CardLevels = Pairs(V.at("cards"));
			P.RunJob = V.at("run_job");
			P.RunJobSecond = V.at("run_job_second");
			P.ReawakenCount = std::atoll(V.at("reawaken_count").c_str());
			P.DeepestBiomeIndex = std::atoll(V.at("deepest").c_str());
			P.CurrentBiomeId = V.at("current_biome");
			P.StardustGrantedEver = Hex(V.at("stardust"));
			const Pc::PcCombatStats Stats = Pc::RebuildStats(D, P);

			CHECK(Pc::IsPounceOwned(P) == (V.at("owned") == "1"));
			CHECK(Pc::AutoPounceUnlocked(P) == (V.at("auto") == "1"));
			const double AutoMult = Pc::AutoPounceMultiplier(D, P);
			CHECK(AutoMult == Hex(V.at("auto_mult")));
			CHECK(Pc::HasPounceInsurance(D, P) == (V.at("insurance") == "1"));
			CHECK(Pc::PounceCraterPower(D, P) == Hex(V.at("crater_power")));
			CHECK(Pc::PounceLockRefund(D, P) == Hex(V.at("lock_refund")));
			CHECK(Pc::JobPounceLockSec(D, P) == Hex(V.at("lock_sec")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::POUNCE_CHAIN) == Hex(V.at("chain")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::POUNCE_CRATER) == Hex(V.at("crater_card")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::POUNCE_KILL_REFUND) == Hex(V.at("kill_refund")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::KITTEN_MIMIC_POUNCE) == Hex(V.at("mimic")));
			CHECK(NearPow(Pc::PounceCooldown(D.Combat, Stats), Hex(V.at("cooldown"))));
			CHECK(NearPow(Pc::PounceDamage(D.Combat, Stats, false), Hex(V.at("damage"))));
			CHECK(NearPow(Pc::PounceDamage(D.Combat, Stats, true), Hex(V.at("damage_vulnerable"))));
			CHECK(NearPow(Pc::PounceDamage(D.Combat, Stats, false, AutoMult), Hex(V.at("damage_auto"))));
			CHECK(NearPow(Pc::PounceDamage(D.Combat, Stats, true, AutoMult), Hex(V.at("damage_auto_vulnerable"))));
			R.Paths["owned"] += V.at("owned") == "1" ? 1 : 0;
			R.Paths["reflex"] += AutoMult > 1.0 ? 1 : 0;
			R.Paths["insurance"] += V.at("insurance") == "1" ? 1 : 0;
			R.Paths["rooted"] += Hex(V.at("lock_sec")) > 0.0 ? 1 : 0;
			R.Paths["crash landing"] += Hex(V.at("crater_power")) > 0.0 ? 1 : 0;
			R.Paths["momentum"] += Hex(V.at("lock_refund")) > 0.0 ? 1 : 0;
			R.Paths["vulnerable bonus"] += Hex(V.at("damage_vulnerable")) > Hex(V.at("damage")) ? 1 : 0;
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("Pounce matches Godot: getters, the Pounce cards, cooldown and damage")
{
	const PounceResult R = RunPounceFixture(DataDir() + "/../Source/PawCore/Tests/Data/pounce_godot.tsv");
	CHECK(R.Rows == 1000);
	for (const char* Path : { "owned", "reflex", "insurance", "rooted", "crash landing", "momentum", "vulnerable bonus" })
	{
		CAPTURE(Path);
		CHECK(R.Paths.count(Path) == 1);
		CHECK(R.Paths.at(Path) >= 3);
	}
}

TEST_CASE("Long: Pounce matches all 3,000 Godot rows" * doctest::test_suite("long") * doctest::skip())
{
	const PounceResult R = RunPounceFixture(DataDir() + "/../build/pounce_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

#endif
