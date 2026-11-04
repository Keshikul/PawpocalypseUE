#if defined(PAWCORE_STANDALONE_TESTS)

// The kittens' rules against the REAL Godot code (tools/godot_fixtures/kittens_fixture.gd): GameState's kitten getters,
// the pool size, a real Kitten node's orbit slot, _pick_target and _is_overcrowded over real Enemy nodes, and one whole
// hunt() (the target it ends on, the claims after, where it stands, what the victim took and where Kitten Drag put it).

#include "ThirdParty/doctest/doctest.h"
#include "PcCombat.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcField.h"
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
	// a relative 1e-13, exactly like the economy and field-rules tests.
	bool NearPow(double Got, double Want, double Scale = 0.0)
	{
		return Got == Want || std::fabs(Got - Want) <= 1e-13 * std::max(std::fabs(Want), Scale);
	}

	// Godot's Vector2 is float32: a position is compared at float32 resolution of the field it lives in.
	bool NearFloat32(double A, double B, double Scale)
	{
		return std::fabs(A - B) <= Scale * 2.5e-7 + 1e-9;
	}

	// One kitten row in a field, as the fixture's lone Kitten node stood.
	void AddKitten(Pc::PcField& F)
	{
		F.KActive.assign(1, 1);
		for (auto* V : { &F.KX, &F.KY, &F.KOrbitX, &F.KOrbitY, &F.KRoamDirX, &F.KRoamDirY, &F.KRoamTimer })
		{
			V->assign(1, 0.0);
		}
		F.KDamageMult.assign(1, 1.0);
		F.KGolden.assign(1, 0);
		F.KLone.assign(1, 0);
		F.KIllusory.assign(1, 0);
		F.KTarget.assign(1, -1);
		F.KTargetSerial.assign(1, 0);
	}

	struct KittensResult
	{
		int Rows = 0;
		std::map<std::string, int> Paths;
	};

	KittensResult RunKittensFixture(const std::string& Path)
	{
		KittensResult R;
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

			// --- the getters ---
			CHECK(Pc::KittenCount(D, P) == std::atoll(V.at("kitten_count").c_str()));
			CHECK(Pc::KittenPoolBaseSize(D) == std::atoll(V.at("pool_base").c_str()));
			const bool bForeman = V.at("foreman") == "1";
			CHECK(Pc::KittensSeekClusters(D, P) == bForeman);
			CHECK(Pc::KittenIllusionPull(D, P) == Hex(V.at("illusion_pull")));
			CHECK_MESSAGE(NearPow(Pc::KittenDps(D.Combat, Stats), Hex(V.at("kitten_dps"))), Pc::KittenDps(D.Combat, Stats) << " vs " << Hex(V.at("kitten_dps")) << " pow " << Stats.KittenPowerMultiplier << " add " << Stats.KittenAdditiveDamageSum << " mul " << Stats.KittenMultiplicativeDamageProduct << " rw " << Stats.ReawakenMultiplier);
			CHECK(NearPow(Stats.KittenSpeedMultiplier, Hex(V.at("kitten_speed"))));
			R.Paths["foreman"] += bForeman ? 1 : 0;
			R.Paths["illusion pull"] += Hex(V.at("illusion_pull")) > 0.0 ? 1 : 0;
			R.Paths["kittens owned"] += std::atoll(V.at("kitten_count").c_str()) > 0 ? 1 : 0;

			// --- the orbit slot (float32 in Godot) ---
			double OrbitX = 0.0, OrbitY = 0.0;
			Pc::KittenOrbitSlot(D, std::atoll(V.at("orbit_index").c_str()), std::atoll(V.at("orbit_total").c_str()), OrbitX, OrbitY);
			const std::vector<std::string> Orbit = SplitKeep(V.at("orbit"), ',');
			CHECK(NearFloat32(OrbitX, Hex(Orbit[0]), D.Code.KittenOrbitRadiusPx));
			CHECK(NearFloat32(OrbitY, Hex(Orbit[1]), D.Code.KittenOrbitRadiusPx));

			// --- a field holding the fixture's mice, in its order (slot k = the k-th candidate) ---
			// The hunt's kill pays: a temporary kitten granted then would re-sync the field's pool and put this test's
			// kitten to sleep, which Godot's lone fixture kitten (outside any pool) never is. So the hunt runs without the
			// Kitten From Signature card; nothing else the hunt reads depends on it.
			Pc::PcProgress Hunt = P;
			for (const Pc::PcCard& C : D.Cards)
			{
				if (C.Effect == Pc::CardEffect::KITTEN_FROM_SIGNATURE)
				{
					Hunt.CardLevels.erase(C.Id);
				}
			}
			Pc::PcField F;
			F.TierIndex = 0;
			F.Bounds = Pc::PcRect{ 0.0, 0.0, 1920.0, 1080.0 };
			F.Stats = Stats;
			const std::vector<std::string> Anchor = SplitKeep(V.at("anchor"), ',');
			F.CatX = Hex(Anchor[0]);
			F.CatY = Hex(Anchor[1]);
			const std::vector<std::string> Origin = SplitKeep(V.at("origin"), ',');
			const double OX = Hex(Origin[0]);
			const double OY = Hex(Origin[1]);
			std::vector<std::string> Mice;
			if (!V.at("mice").empty())
			{
				Mice = SplitKeep(V.at("mice"), '|');
			}
			for (size_t K = 0; K < Mice.size(); ++K)
			{
				const std::vector<std::string> M = SplitKeep(Mice[K], ',');
				const int32_t Slot = Pc::SpawnEnemyAt(F, D, Hunt, M[6], Hex(M[0]), Hex(M[1]), 1.0, 0.0);
				REQUIRE(Slot == static_cast<int32_t>(K));
				F.Hp[K] = Hex(M[4]);
				F.Shielded[K] = M[5] == "1" ? 1 : 0;
				F.Claims[K] = std::atoll(M[2].c_str());
			}
			for (size_t K = 0; K < Mice.size(); ++K)
			{
				// A mouse killed earlier in the frame: still in the live list until the sweep, but free. (Freed only once
				// every mouse is placed, or the next spawn would take its slot.)
				const bool bParked = SplitKeep(Mice[K], ',')[3] == "1";
				F.Active[K] = bParked ? 0 : 1;
				R.Paths["parked mouse"] += bParked ? 1 : 0;
			}
			F.Events.clear();

			// --- _pick_target and _is_overcrowded ---
			const int32_t Picked = Pc::PickKittenTarget(D, OX, OY, F.Live, F.Active, F.X, F.Y, F.Claims, bForeman);
			CHECK(Picked == std::atoi(V.at("pick").c_str()));
			R.Paths["picked"] += Picked >= 0 ? 1 : 0;
			R.Paths["nothing in range"] += Picked < 0 && !Mice.empty() ? 1 : 0;
			R.Paths["picked a claimed mouse"] += Picked >= 0 && F.Claims[static_cast<size_t>(Picked)] > 0 ? 1 : 0;
			R.Paths["foreman picked"] += Picked >= 0 && bForeman ? 1 : 0;
			const int32_t Crowd = std::atoi(V.at("crowd_target").c_str());
			if (Crowd >= 0)
			{
				const bool bCrowded = Pc::KittenOvercrowded(D, OX, OY, Crowd, F.Live, F.Active, F.X, F.Y, F.Claims);
				CHECK(bCrowded == (V.at("overcrowded") == "1"));
				R.Paths["overcrowded"] += bCrowded ? 1 : 0;
				R.Paths["not overcrowded, claimed"] += !bCrowded && F.Claims[static_cast<size_t>(Crowd)] > 1 ? 1 : 0;
			}

			// --- one hunt() ---
			AddKitten(F);
			F.KX[0] = OX;
			F.KY[0] = OY;
			F.KOrbitX[0] = Hex(Orbit[0]);
			F.KOrbitY[0] = Hex(Orbit[1]);
			const std::vector<std::string> Roam = SplitKeep(V.at("roam_dir"), ',');
			F.KRoamDirX[0] = Hex(Roam[0]);
			F.KRoamDirY[0] = Hex(Roam[1]);
			F.KRoamTimer[0] = 1000.0;
			F.KDamageMult[0] = Hex(V.at("damage_mult"));
			F.KDragSpeed = Hex(V.at("drag"));
			const int32_t Before = std::atoi(V.at("hunt_target").c_str());
			F.KTarget[0] = Before;
			F.KTargetSerial[0] = Before >= 0 ? F.Serial[static_cast<size_t>(Before)] : 0;
			Pc::PcFieldInput Input;
			Pc::HuntKitten(F, D, Hunt, Input, 0, Hex(V.at("delta")));

			const int32_t After = std::atoi(V.at("after_target").c_str());
			CHECK(F.KTarget[0] == After);
			const std::vector<std::string> ClaimsAfter = SplitKeep(V.at("after_claims"), ';');
			for (size_t K = 0; K < Mice.size(); ++K)
			{
				CHECK(F.Claims[K] == std::atoll(ClaimsAfter[K].c_str()));
			}
			const std::vector<std::string> Pos = SplitKeep(V.at("after_pos"), ',');
			// float32 at the scale of where it ended: a roaming kitten at a deep Kitten Speed level crosses millions of px a step.
			CHECK(NearFloat32(F.KX[0], Hex(Pos[0]), std::max(2500.0, std::fabs(Hex(Pos[0])))));
			CHECK(NearFloat32(F.KY[0], Hex(Pos[1]), std::max(2500.0, std::fabs(Hex(Pos[1])))));
			R.Paths["roamed"] += After < 0 ? 1 : 0;
			R.Paths["kept its target"] += After >= 0 && After == Before ? 1 : 0;
			R.Paths["re-targeted"] += After >= 0 && Before >= 0 && After != Before ? 1 : 0;
			R.Paths["roam turned at the edge"] += After < 0 && (Hex(Roam[0]) != F.KRoamDirX[0]) ? 1 : 0;

			if (!V.at("after_victim").empty())
			{
				const size_t Victim = static_cast<size_t>(After >= 0 ? After : Before);
				const std::vector<std::string> Vc = SplitKeep(V.at("after_victim"), ',');
				// In contact or not is a float32 distance against the radius in Godot: a kitten that ends within a
				// thousandth of a pixel of the edge is not compared (counted, so the skip cannot grow unseen).
				const double Gap = std::hypot(Hex(Pos[0]) - F.X[Victim], Hex(Pos[1]) - F.Y[Victim]) - D.Combat.BaseKittenContactRadius;
				if (Vc[1] == "1" && std::fabs(Gap) < 1e-3)
				{
					R.Paths["skipped: contact edge"] += 1;
				}
				else
				{
					const bool bAlive = Vc[1] == "1";
					CHECK((F.Active[Victim] != 0) == bAlive);
					if (bAlive)
					{
						// What is left after a blow is compared at the scale of the HP it came off (a near-kill leaves a small
						// difference of two large numbers, and the blow itself carries pow's last-ulp differences).
						CHECK(NearPow(F.Hp[Victim], Hex(Vc[0]), F.MaxHp[Victim]));
						CHECK(NearFloat32(F.X[Victim], Hex(Vc[2]), 2500.0));
						CHECK(NearFloat32(F.Y[Victim], Hex(Vc[3]), 2500.0));
						R.Paths["victim hurt"] += Hex(Vc[0]) < F.MaxHp[Victim] ? 1 : 0;
						R.Paths["victim dragged"] += F.KDragSpeed > 0.0 && Hex(Vc[0]) < F.MaxHp[Victim] ? 1 : 0;
						R.Paths["shielded victim"] += F.Shielded[Victim] ? 1 : 0;
					}
					else
					{
						R.Paths["victim killed"] += 1;
					}
				}
			}
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("Kittens match Godot: getters, pool size, orbit slot, pick, overcrowding, one hunt")
{
	const KittensResult R = RunKittensFixture(DataDir() + "/../Source/PawCore/Tests/Data/kittens_godot.tsv");
	CHECK(R.Rows == 1000);
	for (const char* Path : { "foreman", "illusion pull", "kittens owned", "parked mouse", "picked", "nothing in range",
			 "picked a claimed mouse", "foreman picked", "overcrowded", "not overcrowded, claimed", "roamed", "kept its target",
			 "re-targeted", "roam turned at the edge", "victim hurt", "victim dragged", "shielded victim", "victim killed" })
	{
		CAPTURE(Path);
		CHECK(R.Paths.count(Path) == 1);
		CHECK(R.Paths.at(Path) >= 3);
	}
	// The edge skip stays rare: a handful in a thousand would mean the positions themselves had drifted.
	CHECK((R.Paths.count("skipped: contact edge") == 0 || R.Paths.at("skipped: contact edge") <= 3));
}

TEST_CASE("Long: kittens match all 3,000 Godot rows" * doctest::test_suite("long") * doctest::skip())
{
	const KittensResult R = RunKittensFixture(DataDir() + "/../build/kittens_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

#endif
