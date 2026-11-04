#if defined(PAWCORE_STANDALONE_TESTS)

// The boss's own rules against the REAL Godot code (tools/godot_fixtures/boss_fixture.gd): a real Boss node's
// take_damage() (the shield, Shield Pierce, the phases, the kill), heal() (the cap, phases walked back) and its shield cycle
// (_process), operation for operation.

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcField.h"

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

	double Hex(const std::string& S)
	{
		return std::strtod(S.c_str(), nullptr);
	}

	struct BossResult
	{
		int Rows = 0;
		std::map<std::string, int> Paths;
	};

	BossResult RunBossFixture(const std::string& Path)
	{
		BossResult R;
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
			REQUIRE(Cells.size() == 3);
			CAPTURE(R.Rows);
			const std::string BossId = Cells[0];
			const Pc::PcBoss* B = D.FindBoss(BossId);
			REQUIRE(B);

			// A field in this boss's biome with the boss up exactly as Boss.activate() leaves it.
			Pc::PcProgress P;
			Pc::PcField F;
			for (size_t T = 0; T < D.Tiers.size(); ++T)
			{
				if (D.Tiers[T].BossData == BossId)
				{
					F.TierIndex = static_cast<int64_t>(T);
					P.CurrentBiomeId = D.Tiers[T].Id;
				}
			}
			REQUIRE(F.TierIndex >= 0);
			F.Bounds = Pc::PcRect{ 0.0, 0.0, 1920.0, 1080.0 };
			F.Stats = Pc::RebuildStats(D, P);
			F.bBossUp = true;
			F.BossHp = B->TotalHp;
			F.BossPhase = 1;
			F.BossX = F.BossAnchorX = F.BossDriftX = 960.0;
			F.BossY = F.BossAnchorY = F.BossDriftY = 540.0;
			Pc::PcFieldInput Input;

			const std::vector<std::string> Ops = SplitKeep(Cells[1], '|');
			const std::vector<std::string> States = SplitKeep(Cells[2], '|');
			REQUIRE(Ops.size() == States.size());
			for (size_t K = 0; K < Ops.size(); ++K)
			{
				CAPTURE(K);
				const std::vector<std::string> Op = SplitKeep(Ops[K], ',');
				const bool bWasShielded = F.bBossShielded;
				const int64_t PhaseBefore = F.BossPhase;
				if (Op[0] == "d")
				{
					Pc::DamageFieldBoss(F, D, P, Input, Hex(Op[1]), Hex(Op[2]));
					R.Paths["blocked by the shield"] += bWasShielded && Hex(Op[2]) <= 0.0 && F.bBossUp ? 1 : 0;
					R.Paths["pierced"] += bWasShielded && Hex(Op[2]) > 0.0 ? 1 : 0;
				}
				else if (Op[0] == "h")
				{
					Pc::HealFieldBoss(F, D, Hex(Op[1]));
					R.Paths["healed to the cap"] += F.bBossUp && F.BossHp == B->TotalHp && Hex(Op[1]) > 0.0 ? 1 : 0;
				}
				else
				{
					if (F.bBossUp)
					{
						Pc::TickFieldBoss(F, D, Hex(Op[1]));
					}
					R.Paths["shield turned"] += F.bBossShielded != bWasShielded ? 1 : 0;
				}
				const std::vector<std::string> S = SplitKeep(States[K], ',');
				CHECK(F.BossHp == Hex(S[0]));
				CHECK(F.BossPhase == std::atoll(S[1].c_str()));
				CHECK(F.bBossShielded == (S[2] == "1"));
				CHECK(F.BossInvulnTimer == Hex(S[3]));
				CHECK(F.bBossUp == (S[4] == "1"));
				R.Paths["phase rose"] += F.BossPhase > PhaseBefore ? 1 : 0;
				R.Paths["phase walked back"] += F.BossPhase < PhaseBefore ? 1 : 0;
				R.Paths["killed"] += S[4] == "0" && Op[0] == "d" && F.BossHp == 0.0 && PhaseBefore > 0 ? 1 : 0;
			}
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("The boss matches Godot's Boss node: damage, the shield, Shield Pierce, phases, heals, the cycle, the kill")
{
	const BossResult R = RunBossFixture(DataDir() + "/../Source/PawCore/Tests/Data/boss_godot.tsv");
	CHECK(R.Rows == 1000);
	for (const char* Path : { "blocked by the shield", "pierced", "healed to the cap", "shield turned", "phase rose",
			 "phase walked back", "killed" })
	{
		CAPTURE(Path);
		CHECK(R.Paths.count(Path) == 1);
		CHECK(R.Paths.at(Path) >= 3);
	}
}

TEST_CASE("Long: the boss matches all 3,000 Godot rows" * doctest::test_suite("long") * doctest::skip())
{
	const BossResult R = RunBossFixture(DataDir() + "/../build/boss_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

#endif
