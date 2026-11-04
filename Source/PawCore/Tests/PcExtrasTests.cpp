#if defined(PAWCORE_STANDALONE_TESTS)

// The paw's and the kill's extras and the Prism, against the REAL Godot code (tools/godot_fixtures/extras_fixture.gd):
// every GameState getter of slice 5 and the Catnip Bomb and Scavenge card values.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcFieldRules.h"

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

	struct ExtrasResult
	{
		int Rows = 0;
		std::map<std::string, int> Paths;
	};

	ExtrasResult RunExtrasFixture(const std::string& Path)
	{
		ExtrasResult R;
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

			CHECK(Pc::GuaranteedCritShare(D, P) == Hex(V.at("rhythm")));
			CHECK(Pc::DeterministicCrit(D, P) == (V.at("inscrutable") == "1"));
			CHECK(Pc::DoubleStrikeShare(D, P) == Hex(V.at("anvil")));
			CHECK(Pc::PawCleaveCount(D, P) == std::atoll(V.at("cleave").c_str()));
			CHECK(Pc::CritExecuteChance(D, P) == Hex(V.at("riddle")));
			CHECK(Pc::OverkillCatnipShare(D, P) == Hex(V.at("cut")));
			CHECK(Pc::OverflowRatio(D, P) == Hex(V.at("overflow")));
			CHECK(Pc::LevyMultiple(D, P) == Hex(V.at("levy")));
			CHECK(Pc::PiedPiperRadius(D, P) == Hex(V.at("piper")));
			CHECK(Pc::SpawnForesight(D, P) == (V.at("foresight") == "1"));
			CHECK(Pc::VanishInterval(D, P) == Hex(V.at("vanish")));
			CHECK(Pc::PickpocketShare(D, P) == Hex(V.at("pickpocket")));
			CHECK(Pc::PrismOwned(P) == (V.at("prism_owned") == "1"));
			CHECK(NearPow(Pc::PrismBeamDps(D, P, Stats), Hex(V.at("beam_dps"))));
			CHECK(Pc::PrismSweeps(P) == (V.at("sweeps") == "1"));
			CHECK(Pc::PrismChainCount(D, P) == std::atoll(V.at("chain").c_str()));
			CHECK(Pc::PrismShardPayload(D, P) == Hex(V.at("payload")));
			CHECK(Pc::CardEffectBestLow(D, P, Pc::CardEffect::CATNIP_BOMB_EVERY) == Hex(V.at("bomb_every")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::SCAVENGE) == Hex(V.at("scavenge")));
			R.Paths["inscrutable"] += V.at("inscrutable") == "1" ? 1 : 0;
			R.Paths["cleave"] += std::atoll(V.at("cleave").c_str()) > 0 ? 1 : 0;
			R.Paths["vanish"] += Hex(V.at("vanish")) > 0.0 ? 1 : 0;
			R.Paths["beam"] += Hex(V.at("beam_dps")) > 0.0 ? 1 : 0;
			R.Paths["prism owned, no aura (beam 0)"] += V.at("prism_owned") == "1" && Hex(V.at("beam_dps")) == 0.0 ? 1 : 0;
			R.Paths["sweeps"] += V.at("sweeps") == "1" ? 1 : 0;
			R.Paths["refraction"] += std::atoll(V.at("chain").c_str()) > 0 ? 1 : 0;
			R.Paths["critical mass"] += Hex(V.at("payload")) > 0.0 ? 1 : 0;
			R.Paths["refraction without the prism"] += P.BranchLevels.count("refraction") && V.at("prism_owned") == "0" ? 1 : 0;
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("Extras match Godot: the paw's, the kill's and the spawner's nodes, the Prism, Catnip Bomb and Scavenge")
{
	const ExtrasResult R = RunExtrasFixture(DataDir() + "/../Source/PawCore/Tests/Data/extras_godot.tsv");
	CHECK(R.Rows == 1000);
	for (const char* Path : { "inscrutable", "cleave", "vanish", "beam", "prism owned, no aura (beam 0)", "sweeps", "refraction",
			 "critical mass", "refraction without the prism" })
	{
		CAPTURE(Path);
		CHECK(R.Paths.count(Path) == 1);
		CHECK(R.Paths.at(Path) >= 3);
	}
}

TEST_CASE("Long: extras match all 3,000 Godot rows" * doctest::test_suite("long") * doctest::skip())
{
	const ExtrasResult R = RunExtrasFixture(DataDir() + "/../build/extras_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

#endif
