#if defined(PAWCORE_STANDALONE_TESTS)

// The aura's and zones' pure rules against the REAL Godot code (tools/godot_fixtures/aura_fixture.gd): GameState's aura,
// Snare, Gridlock, Crowd Work, Echo, Standing Wave, Unicorn and Long Lunch getters, the aura cards' values, aura, poop and
// player DPS, and a real Enemy node's apply_slow() over a sequence of slows with the step turning over between them.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcFieldRules.h"

#include <algorithm>
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

	// pow() differs by C runtime (Godot's MinGW build calls msvcrt pow): values that went through one are compared within
	// a relative 1e-13, exactly like the other field fixtures.
	bool NearPow(double Got, double Want)
	{
		return Got == Want || std::fabs(Got - Want) <= 1e-13 * std::fabs(Want);
	}

	struct AuraResult
	{
		int Rows = 0;
		std::map<std::string, int> Paths;
	};

	AuraResult RunAuraFixture(const std::string& Path)
	{
		AuraResult R;
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
			P.PlayerStillSeconds = Hex(V.at("still"));
			P.LunchSeconds = Hex(V.at("lunch"));
			const Pc::PcCombatStats Stats = Pc::RebuildStats(D, P);

			// --- GameState's getters ---
			CHECK(Pc::IsAuraOwned(P) == (V.at("aura_owned") == "1"));
			CHECK(Stats.bAuraOwned == (V.at("aura_owned") == "1"));
			CHECK(NearPow(Pc::AuraRadiusMultiplier(D, P), Hex(V.at("radius_mult"))));
			CHECK(Pc::CarryRadiusBonus(D, P) == Hex(V.at("carry")));
			CHECK(Pc::AuraReachMultiplier(D, P) == Hex(V.at("reach")));
			CHECK(Pc::SnareMultiplier(D, P) == Hex(V.at("snare")));
			CHECK(Pc::GridlockMultiplier(D, P) == Hex(V.at("gridlock")));
			CHECK(Pc::AuraCrowdScaling(D, P) == Hex(V.at("crowd")));
			CHECK(Pc::AuraEchoPower(D, P) == Hex(V.at("echo")));
			CHECK(Pc::AuraBeatSeconds(D, P) == Hex(V.at("beat")));
			CHECK(Pc::PoopUnlocked(P) == (V.at("poop_unlocked") == "1"));
			CHECK(Pc::LongLunchPayout(D, P) == Hex(V.at("lunch_payout")));
			R.Paths["aura owned"] += V.at("aura_owned") == "1" ? 1 : 0;
			R.Paths["carry partial"] += Hex(V.at("carry")) > 0.0 && P.PlayerStillSeconds < D.Code.CarryFullSeconds ? 1 : 0;
			R.Paths["carry full"] += Hex(V.at("carry")) > 0.0 && P.PlayerStillSeconds >= D.Code.CarryFullSeconds ? 1 : 0;
			R.Paths["reach"] += Hex(V.at("reach")) != 1.0 ? 1 : 0;
			R.Paths["snare"] += Hex(V.at("snare")) < 1.0 ? 1 : 0;
			R.Paths["deadweight"] += Hex(V.at("snare")) < 1.0 && P.BranchLevels.count("deadweight") ? 1 : 0;
			R.Paths["gridlock"] += Hex(V.at("gridlock")) < 1.0 ? 1 : 0;
			R.Paths["crowd work"] += Hex(V.at("crowd")) > 0.0 ? 1 : 0;
			R.Paths["echo"] += Hex(V.at("echo")) > 0.0 ? 1 : 0;
			R.Paths["standing wave"] += Hex(V.at("beat")) > 0.0 ? 1 : 0;
			R.Paths["poop unlocked"] += V.at("poop_unlocked") == "1" ? 1 : 0;
			R.Paths["lunch partial"] += Hex(V.at("lunch_payout")) > 0.0 && P.LunchSeconds < D.Code.LunchFullSeconds ? 1 : 0;

			// --- the aura cards ---
			CHECK(Pc::CardEffectBestLow(D, P, Pc::CardEffect::AURA_SLOW) == Hex(V.at("aura_slow")));
			CHECK(Pc::CardEffectBestLow(D, P, Pc::CardEffect::POOP_TRAIL) == Hex(V.at("poop_trail")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::AURA_TRAIL) == Hex(V.at("aura_trail")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::AURA_PULSE) == Hex(V.at("aura_pulse")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::EDGE_BURN) == Hex(V.at("edge_burn")));
			CHECK(Pc::CardEffectValue(D, P, Pc::CardEffect::POOP_DURATION) == Hex(V.at("poop_duration")));
			R.Paths["aura slow card"] += Hex(V.at("aura_slow")) > 0.0 ? 1 : 0;
			R.Paths["poop trail card"] += Hex(V.at("poop_trail")) > 0.0 ? 1 : 0;

			// --- the damage the zones are priced in ---
			CHECK(NearPow(Pc::AuraDps(D.Combat, Stats), Hex(V.at("aura_dps"))));
			CHECK(NearPow(Pc::PoopDps(D.Combat, Stats), Hex(V.at("poop_dps"))));
			CHECK(NearPow(Pc::PlayerDps(D.Combat, Stats), Hex(V.at("player_dps"))));

			// --- one enemy taking slows ---
			const Pc::PcEnemy* E = D.FindEnemy(V.at("enemy"));
			REQUIRE(E);
			const bool bFlier = E->Movement == Pc::EnemyMovementKind::FLIER;
			double Slow = 1.0;
			uint8_t Fresh = 0;
			for (const std::string& Call : SplitKeep(V.at("slows"), '|'))
			{
				const std::vector<std::string> C = SplitKeep(Call, ',');
				if (C[0] == "1")
				{
					// The step turned over: Enemy._process's reset.
					if (!Fresh)
					{
						Slow = 1.0;
					}
					Fresh = 0;
				}
				Pc::ApplySlow(Slow, Fresh, bFlier, Hex(C[1]), C[2] == "1");
			}
			const std::vector<std::string> After = SplitKeep(V.at("slow_after"), ',');
			CHECK(Slow == Hex(After[0]));
			CHECK((Fresh != 0) == (After[1] == "1"));
			R.Paths["flier slowed by gridlock"] += bFlier && Slow < 1.0 ? 1 : 0;
			R.Paths["flier refused"] += bFlier && Slow == 1.0 ? 1 : 0;
			R.Paths["ground slowed"] += !bFlier && Slow < 1.0 ? 1 : 0;
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("Aura matches Godot: getters, aura cards, aura/poop/player DPS, apply_slow")
{
	const AuraResult R = RunAuraFixture(DataDir() + "/../Source/PawCore/Tests/Data/aura_godot.tsv");
	CHECK(R.Rows == 1000);
	for (const char* Path : { "aura owned", "carry partial", "carry full", "reach", "snare", "deadweight", "gridlock",
			 "crowd work", "echo", "standing wave", "poop unlocked", "lunch partial", "aura slow card", "poop trail card",
			 "flier slowed by gridlock", "flier refused", "ground slowed" })
	{
		CAPTURE(Path);
		CHECK(R.Paths.count(Path) == 1);
		CHECK(R.Paths.at(Path) >= 3);
	}
}

TEST_CASE("Long: aura matches all 3,000 Godot rows" * doctest::test_suite("long") * doctest::skip())
{
	const AuraResult R = RunAuraFixture(DataDir() + "/../build/aura_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

#endif
