#if defined(PAWCORE_STANDALONE_TESTS)

// PcFieldRules against the REAL Godot code (tools/godot_fixtures/field_rules_fixture.gd): GameState's field getters, the
// TierController spawn cadence, TierData.pick_enemy, TierController.playfield_rect, and a real pooled Enemy node's
// activate() and take_damage() under random shield / snare / Shield Pierce / threshold / beam states.

#include "ThirdParty/doctest/doctest.h"
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

	const Pc::PcTier& TierById(const Pc::PcGameData& D, const std::string& Id)
	{
		for (const Pc::PcTier& T : D.Tiers)
		{
			if (T.Id == Id)
			{
				return T;
			}
		}
		FAIL("no tier " << Id);
		return D.Tiers.front();
	}

	// Godot's Vector2/Rect2 are 32-bit: playfield_rect runs in single precision. PawCore never uses float (hard rule), so
	// the rectangle is compared at float32 resolution of the view it was computed in.
	bool NearFloat32(double A, double B, double Scale)
	{
		return std::fabs(A - B) <= Scale * 1.2e-7 + 1e-9;
	}

	// pow() differs by C runtime (Godot's MinGW build calls msvcrt pow; the shop lines and waves are powers): values that
	// went through one are compared within a relative 1e-13 of their scale, exactly like the economy test.
	bool NearPow(double Got, double Want, double Scale = 0.0)
	{
		return Got == Want || std::fabs(Got - Want) <= 1e-13 * std::max(std::fabs(Want), Scale);
	}

	struct FieldRulesResult
	{
		int Rows = 0;
		std::map<std::string, int> Paths;
	};

	FieldRulesResult RunFieldRulesFixture(const std::string& Path)
	{
		FieldRulesResult R;
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
			P.CurrentWaveIndex = std::atoll(V.at("wave_index").c_str());
			P.InfestationBonus = Hex(V.at("infestation"));
			const int64_t Live = std::atoll(V.at("live").c_str());
			const Pc::PcTier& Probe = TierById(D, V.at("probe_realm"));

			// --- GameState's getters and the spawn cadence: bit for bit ---
			const double PerSec = Pc::SpawnRatePerSec(D, P, Probe);
			const Pc::PcSpawnCadence Cadence = Pc::SpawnCadence(D, PerSec);
			CHECK(NearPow(Pc::SpawnRateMultiplier(D, P), Hex(V.at("spawn_mult"))));
			CHECK(NearPow(PerSec, Hex(V.at("spawn_per_sec"))));
			CHECK(Cadence.Batch == std::atoll(V.at("spawn_batch").c_str()));
			CHECK(NearPow(Cadence.Interval, Hex(V.at("spawn_interval"))));
			CHECK(Pc::ThresholdDamageMultiplier(D, P, Live) == Hex(V.at("threshold")));
			CHECK(Pc::SnaredDamageMultiplier(D, P) == Hex(V.at("snared")));
			CHECK(Pc::PlayWithFoodMultiplier(D, P) == Hex(V.at("play_with_food")));
			CHECK(Pc::FunnelBounces(D, P) == Hex(V.at("funnel")));
			CHECK(Pc::EnemyDrift(D, P) == Hex(V.at("drift")));
			CHECK(Pc::PrismKillMultiplier(D, P) == Hex(V.at("prism_kill")));
			R.Paths["batch > 1"] += Cadence.Batch > 1 ? 1 : 0;
			R.Paths["threshold > 1"] += Hex(V.at("threshold")) > 1.0 ? 1 : 0;
			R.Paths["snared > 1"] += Hex(V.at("snared")) > 1.0 ? 1 : 0;
			R.Paths["play with food > 1"] += Hex(V.at("play_with_food")) > 1.0 ? 1 : 0;
			R.Paths["funnel > 0"] += Hex(V.at("funnel")) > 0.0 ? 1 : 0;
			R.Paths["drift > 0"] += Hex(V.at("drift")) > 0.0 ? 1 : 0;
			R.Paths["spectrum > 1"] += Hex(V.at("prism_kill")) > 1.0 ? 1 : 0;

			// --- the pool ---
			const double Roll = Hex(V.at("roll"));
			const double SigWeight = Hex(V.at("sig_weight"));
			const Pc::PcEnemy* Picked = Pc::PickEnemy(D, Probe, Roll, SigWeight);
			CHECK((Picked ? Picked->Id : std::string()) == V.at("pick"));
			CHECK(Pc::TotalSpawnWeight(D, Probe, SigWeight) == Hex(V.at("total_weight")));
			R.Paths["roll out of 0..1"] += Roll < 0.0 || Roll >= 1.0 ? 1 : 0;
			R.Paths["signature weight"] += SigWeight > 1.0 ? 1 : 0;

			// --- the playfield (float32 in Godot) ---
			Pc::PcRect Dock;
			if (!V.at("dock").empty())
			{
				const std::vector<std::string> Dk = SplitKeep(V.at("dock"), ',');
				Dock = Pc::PcRect{ Hex(Dk[0]), Hex(Dk[1]), Hex(Dk[2]), Hex(Dk[3]) };
			}
			const double ViewW = Hex(V.at("view_x"));
			const double ViewH = Hex(V.at("view_y"));
			const Pc::PcRect Field = Pc::PlayfieldRect(D, ViewW, ViewH, Dock, 600.0, V.at("dock_left") == "1");
			const std::vector<std::string> Fr = SplitKeep(V.at("playfield"), ',');
			const double Scale = std::max(ViewW, ViewH) + std::fabs(Dock.X) + Dock.W;
			CHECK(NearFloat32(Field.X, Hex(Fr[0]), Scale));
			CHECK(NearFloat32(Field.Y, Hex(Fr[1]), Scale));
			CHECK(NearFloat32(Field.W, Hex(Fr[2]), Scale));
			CHECK(NearFloat32(Field.H, Hex(Fr[3]), Scale));
			R.Paths["no dock"] += Dock.W <= 0.0 ? 1 : 0;
			R.Paths["narrow field widened"] += std::fabs(Hex(Fr[2]) - D.Code.MinPlayfieldWidth) < 1e-3 ? 1 : 0;

			// --- one enemy: spawn stats and blows ---
			const Pc::PcEnemy* Enemy = D.FindEnemy(V.at("enemy"));
			REQUIRE(Enemy);
			const Pc::PcEnemySpawnStats S = Pc::EnemySpawnStats(D, P, *Enemy);
			CHECK(NearPow(S.Hp, Hex(V.at("hp"))));
			CHECK(NearPow(S.MaxHp, Hex(V.at("max_hp"))));
			CHECK(NearPow(S.WaveRewardMultiplier, Hex(V.at("wave_reward"))));
			CHECK(std::floor(S.FunnelBounces) == Hex(V.at("bounces_whole")));
			Pc::PcBlowTarget T;
			T.Data = Enemy;
			// The blows start from GODOT'S spawn stats, so the intake itself is compared bit for bit.
			T.Hp = Hex(V.at("hp"));
			T.WaveRewardMultiplier = Hex(V.at("wave_reward"));
			const std::vector<std::string> Blows = SplitKeep(V.at("blows"), '|');
			const std::vector<std::string> Results = SplitKeep(V.at("results"), '|');
			REQUIRE(Blows.size() == Results.size());
			for (size_t B = 0; B < Blows.size(); ++B)
			{
				CAPTURE(B);
				const std::vector<std::string> Bl = SplitKeep(Blows[B], ',');
				const std::vector<std::string> Rs = SplitKeep(Results[B], ',');
				T.bShielded = Bl[1] == "1";
				// apply_slow() does not reach a flier: Godot's slow flag stays clear for one.
				T.bSnared = Bl[2] == "1" && Enemy->Movement != Pc::EnemyMovementKind::FLIER;
				T.bKilledByBeam = Bl[4] == "1";
				const Pc::PcBlowResult Out = Pc::ApplyBlow(D, P, T, Hex(Bl[0]), Hex(Bl[3]), Hex(Bl[5]));
				CHECK(Out.Overkill == Hex(Rs[0]));
				CHECK(Out.HpAfter == Hex(Rs[1]));
				const bool bKilled = Hex(Rs[2]) >= 0.0;
				CHECK(Out.bKilled == bKilled);
				if (bKilled)
				{
					CHECK(Out.Reward == Hex(Rs[2]));
					CHECK(Out.bToyedWith == (Rs[3] == "1"));
					R.Paths["killed"] += 1;
					R.Paths["killed with overkill"] += Out.Overkill > 0.0 ? 1 : 0;
					R.Paths["toyed with"] += Out.bToyedWith ? 1 : 0;
					R.Paths["beam kill paid"] += T.bKilledByBeam && Hex(V.at("prism_kill")) > 1.0 ? 1 : 0;
				}
				R.Paths["shield blocked"] += T.bShielded && Hex(Bl[3]) <= 0.0 ? 1 : 0;
				R.Paths["shield pierced"] += T.bShielded && Hex(Bl[3]) > 0.0 ? 1 : 0;
				R.Paths["snared x traps"] += T.bSnared && Hex(V.at("snared")) > 1.0 ? 1 : 0;
				R.Paths["flier ignores snare"] += Bl[2] == "1" && !T.bSnared ? 1 : 0;
			}
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcFieldRules matches Godot: field getters, spawn cadence, pool pick, playfield, enemy activate/take_damage")
{
	const FieldRulesResult R = RunFieldRulesFixture(DataDir() + "/../Source/PawCore/Tests/Data/field_rules_godot.tsv");
	CHECK(R.Rows == 1000);
	// Coverage: every path the rules can take is exercised by the fixture (a path at zero proves nothing).
	for (const char* Path : { "batch > 1", "threshold > 1", "snared > 1", "play with food > 1", "funnel > 0", "drift > 0",
			 "spectrum > 1", "roll out of 0..1", "signature weight", "no dock", "narrow field widened", "killed",
			 "killed with overkill", "toyed with", "beam kill paid", "shield blocked", "shield pierced", "snared x traps",
			 "flier ignores snare" })
	{
		CAPTURE(Path);
		CHECK(R.Paths.count(Path) == 1);
		CHECK(R.Paths.at(Path) >= 3);
	}
}

TEST_CASE("Long: PcFieldRules matches all 3,000 Godot field-rules rows" * doctest::test_suite("long") * doctest::skip())
{
	const FieldRulesResult R = RunFieldRulesFixture(DataDir() + "/../build/field_rules_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

#endif
