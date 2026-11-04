#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcProgress.h"

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
		// Keeps empty cells (getline would drop a trailing one).
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

	const Pc::PcTier* Tier(const Pc::PcGameData& D, const std::string& Id)
	{
		for (const Pc::PcTier& T : D.Tiers)
		{
			if (T.Id == Id)
			{
				return &T;
			}
		}
		return nullptr;
	}

	struct EconomyResult
	{
		int Rows = 0;
		int64_t Exact = 0;
		int64_t PowFields = 0;
		int SummonsCompared = 0;
	};

	EconomyResult RunEconomyFixture(const std::string& Path)
	{
		EconomyResult R;
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

			Pc::PcProgress P;
			P.UpgradeLevels = Pairs(V.at("upgrades"));
			P.BranchLevels = Pairs(V.at("branches"));
			P.CardLevels = Pairs(V.at("cards"));
			P.RunJob = V.at("run_job");
			P.RunJobSecond = V.at("run_job_second");
			P.ReawakenCount = std::atoll(V.at("reawaken_count").c_str());
			P.DeepestBiomeIndex = std::atoll(V.at("deepest").c_str());
			for (const std::string& B : SplitKeep(V.at("bosses"), ';'))
			{
				if (!B.empty())
				{
					P.BossesDefeated.insert(B);
				}
			}
			P.bFinalBossDefeated = V.at("final") == "1";
			P.StardustGrantedEver = Hex(V.at("stardust"));
			P.CurrentBiomeId = V.at("current_biome");
			P.TierKillCount = std::atoll(V.at("tier_kill_count").c_str());
			P.TierKillsByBiome = Pairs(V.at("kills_by_biome"));
			P.BaitSpentByBiome = Pairs(V.at("bait_spent"));
			for (const std::string& B : SplitKeep(V.at("cleared"), ';'))
			{
				if (!B.empty())
				{
					P.BossesClearedThisCycle.insert(B);
				}
			}
			P.CurrentWaveIndex = std::atoll(V.at("wave_index").c_str());
			P.KillsThisWave = std::atoll(V.at("kills_this_wave").c_str());
			P.LifetimeCatnipThisCycle = Hex(V.at("catnip_cycle"));
			P.LifetimeCatnipEver = Hex(V.at("catnip_ever"));
			const Pc::PcTier* Probe = Tier(D, V.at("probe_realm"));
			REQUIRE(Probe);
			const Pc::PcTier* Current = Tier(D, P.CurrentBiomeId);
			CAPTURE(R.Rows);

			auto Exact = [&R, &V](const char* Name, double Got)
			{
				const double Want = Hex(V.at(Name));
				CHECK_MESSAGE(Got == Want, std::string(Name) << ": PawCore " << Got << " vs Godot " << Want);
				++R.Exact;
			};
			// pow() differs by C runtime (Godot's MinGW build calls msvcrt pow): a relative 1e-13 on the pow-derived fields.
			auto Pow = [&R, &V](const char* Name, double Got)
			{
				const double Want = Hex(V.at(Name));
				const double Rel = Got == Want ? 0.0 : std::fabs(Got - Want) / std::max(std::fabs(Want), 1e-300);
				CHECK_MESSAGE(Rel <= 1e-13, std::string(Name) << ": PawCore " << Got << " vs Godot " << Want);
				++R.PowFields;
			};
			auto Int = [&V](const char* Name, int64_t Got)
			{
				CHECK_MESSAGE(std::to_string(Got) == V.at(Name), std::string(Name) << ": PawCore " << Got << " vs Godot " << V.at(Name));
			};

			Exact("whole", Pc::WholeCatnip(Hex(V.at("amount"))));
			Exact("kill_catnip", Pc::KillCatnip(D, P, Hex(V.at("base_reward"))));
			Exact("biome_mult", Pc::BiomeRewardMultiplier(D, P));
			Exact("fortune", Pc::FortuneYieldMultiplier(D, P));
			Exact("golden_bonus", Pc::GoldenChanceBonus(D, P));
			Exact("golden_for_probe", Pc::GoldenChanceFor(D, P, *Probe));
			Exact("job_catnip", Pc::JobCatnipMultiplier(D, P));
			Exact("fence", Pc::FenceMultiplier(D, P));
			Int("current_wave", Current ? Pc::CurrentWave(D, *Current, P.TierKillCount) : 0);
			Pow("wave_hp", Pc::WaveHpMultiplier(D, P));
			Pow("wave_reward", Pc::WaveRewardMultiplier(D, P));
			const Pc::PcWaveProgress Wave = Pc::WaveProgress(D, P);
			// Done = kills - where the wave started (a pow series): a few ulps on operands of ~kill-count size become a
			// large RELATIVE error on a small difference (measured 1.9e-12 at 91 kills into a wave). Scaled to the operands.
			{
				const double Want = Hex(V.at("wave_done"));
				const double Scale = std::max({ std::fabs(Want), static_cast<double>(P.TierKillCount), 1.0 });
				CHECK_MESSAGE(std::fabs(Wave.Done - Want) <= 1e-13 * Scale, "wave_done: PawCore " << Wave.Done << " vs Godot " << Want);
				++R.PowFields;
			}
			Pow("wave_costs", Wave.Costs);
			std::string Unlocked;
			for (const Pc::PcTier& T : D.Tiers)
			{
				Unlocked += Pc::IsBiomeUnlocked(D, P, T) ? '1' : '0';
			}
			CHECK_MESSAGE(Unlocked == V.at("unlocked"), "unlocked: PawCore " << Unlocked << " vs Godot " << V.at("unlocked"));
			Exact("until_probe", Pc::CatnipUntilUnlocked(P, *Probe));
			Exact("requirement", Pc::ReawakenRequirement(D, P));
			Exact("pending", Pc::PendingStardust(D, P));
			CHECK((Pc::CanReawaken(D, P) ? "1" : "0") == V.at("can_reawaken"));
			CHECK((Pc::IsReawakenSuggested(D, P) ? "1" : "0") == V.at("suggested"));
			Pow("requirement_catnip", Pc::ReawakenRequirementCatnip(D, P));
			Pow("until_reawaken", Pc::CatnipUntilReawaken(D, P));
			const Pc::PcBarProgress Bar = Pc::ReawakenBarProgress(D, P);
			Exact("bar_earned", Bar.Earned);
			Pow("bar_required", Bar.Required);
			Pow("projected", Pc::ProjectedReawakenMultiplier(D, P));
			std::string Earned;
			for (const Pc::PcTier& T : D.Tiers)
			{
				Earned += (Earned.empty() ? "" : ";") + std::to_string(Pc::BaitEarned(D, P, T.Id));
			}
			CHECK_MESSAGE(Earned == V.at("bait_earned"), "bait_earned: PawCore " << Earned << " vs Godot " << V.at("bait_earned"));
			Int("bait_held", Pc::BaitHeld(D, P, P.CurrentBiomeId));
			Exact("audit", Pc::AuditCatnipMultiple(D, P));
			const Pc::PcBoss* Boss = Current && !Current->BossData.empty() ? D.FindBoss(Current->BossData) : nullptr;
			if (!Boss)
			{
				CHECK(V.at("shortfall").empty());
			}
			else
			{
				const int64_t Shortfall = Pc::BaitShortfall(D, P, *Boss);
				Int("shortfall", Shortfall);
				// buy_bait() bought iff the bait was in hand and a 1e9 bank covered the price.
				const double Cost = Pc::SummonCatnipCost(D, P, *Boss);
				const bool bBought = Shortfall == 0 && !(Cost > 0.0 && 1.0e9 < Cost);
				CHECK(bBought == !V.at("summon_cost").empty());
				if (bBought)
				{
					Exact("summon_cost", 1.0e9 - (1.0e9 - Cost));
					++R.SummonsCompared;
				}
			}
			Exact("carryover", Pc::CatnipCarryover(D, P));
			Int("start_level", Pc::StartingUpgradeLevel(D, P));
			Int("start_biome", Pc::StartingBiomeIndex(D, P));
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcEconomy matches Godot's GameState economy on 1,000 random states")
{
	const EconomyResult R = RunEconomyFixture(DataDir() + "/../Source/PawCore/Tests/Data/economy_godot.tsv");
	CHECK(R.Rows == 1000);
	CHECK(R.SummonsCompared > 50); // the summon path must actually be exercised
	MESSAGE("exact fields " << R.Exact << ", pow-derived fields within 1e-13: " << R.PowFields << ", summons " << R.SummonsCompared);
}

TEST_CASE("Long: PcEconomy matches all 3,000 Godot economy rows" * doctest::test_suite("long") * doctest::skip())
{
	// build/economy_godot_full.tsv comes from `py tools/make_godot_fixtures.py` (the real Godot code; not in git).
	const EconomyResult R = RunEconomyFixture(DataDir() + "/../build/economy_godot_full.tsv");
	CHECK(R.Rows == 3000);
	MESSAGE("exact fields " << R.Exact << ", pow-derived fields within 1e-13: " << R.PowFields << ", summons " << R.SummonsCompared);
}

TEST_CASE("After a Reawaken: the starting biome, Territory's included (designer 2026-11-01: Territory wins)")
{
	// tools/check_godot_reawaken_travel.py proves the same four cases on the real Godot HUD.
	const Pc::PcGameData& D = Game();
	auto Index = [&](const std::string& Id) { return static_cast<int64_t>(Pc::RealmIndex(D, Id)); };
	Pc::PcProgress P;
	P.BossesDefeated = { "tier_3", "tier_6" }; // stage 3: biome 4 is on the ladder (the probe's state)
	P.BranchLevels["legacy"] = 5;               // Territory: biome 4
	REQUIRE(Pc::StartingBiomeIndex(D, P) == 4);
	// reawaken() clears the biome: the travel always runs after a real Reawaken, from any depth.
	P.CurrentBiomeId = "";
	CHECK(Pc::ReawakenTravelIndex(D, P) == Index("tier_4"));
	// Deeper than the start (tier_5) and shallower (tier_2): Territory wins both ways.
	P.CurrentBiomeId = "tier_5";
	CHECK(Pc::ReawakenTravelIndex(D, P) == Index("tier_4"));
	P.CurrentBiomeId = "tier_2";
	CHECK(Pc::ReawakenTravelIndex(D, P) == Index("tier_4"));
	// Already there: no travel.
	P.CurrentBiomeId = "tier_4";
	CHECK(Pc::ReawakenTravelIndex(D, P) == -1);
	// No Territory: the first biome.
	P.BranchLevels["legacy"] = 0;
	P.CurrentBiomeId = "tier_5";
	CHECK(Pc::ReawakenTravelIndex(D, P) == 0);
	P.CurrentBiomeId = "tier_1";
	CHECK(Pc::ReawakenTravelIndex(D, P) == -1);
	// Territory past the ladder is clamped to it: never a way in.
	P.BranchLevels["legacy"] = 10; // wants biome 6
	P.BossesDefeated.clear(); // stage 1: the ladder stops short of biome 6
	P.CurrentBiomeId = "";
	CHECK(Pc::ReawakenTravelIndex(D, P) == Pc::StartingBiomeIndex(D, P) - 1);
	CHECK(Pc::StartingBiomeIndex(D, P) < 6);
}

TEST_CASE("Economy rules from the Godot comments")
{
	const Pc::PcGameData& D = Game();
	// Every kill is felt; a payout of nothing is nothing; halves round away from zero.
	CHECK(Pc::WholeCatnip(0.33) == 1.0);
	CHECK(Pc::WholeCatnip(0.0) == 0.0);
	CHECK(Pc::WholeCatnip(-3.0) == 0.0);
	CHECK(Pc::WholeCatnip(2.5) == 3.0);
	// Interest has no floor of 1.
	CHECK(Pc::UntouchedCatnip(0.9) == 0.0);
	CHECK(Pc::UntouchedCatnip(7.9) == 7.0);
	// The Stardust grant is split-invariant: one cycle or two pay the same in total.
	Pc::PcProgress A;
	A.LifetimeCatnipEver = 4.0e6;
	const double OneCycle = Pc::PendingStardust(D, A);
	Pc::PcProgress B;
	B.LifetimeCatnipEver = 1.0e6;
	const double First = Pc::PendingStardust(D, B);
	B.StardustGrantedEver = First;
	B.LifetimeCatnipEver = 4.0e6;
	CHECK(First + Pc::PendingStardust(D, B) == OneCycle);
	// The Catnip form of the gate opens within one Catnip of the Stardust form.
	Pc::PcProgress G;
	G.StardustGrantedEver = 25.0;
	const double Needed = Pc::ReawakenRequirementCatnip(D, G);
	G.LifetimeCatnipEver = Needed;
	CHECK(Pc::IsReawakenSuggested(D, G));
	G.LifetimeCatnipEver = Needed - 1.0;
	CHECK_FALSE(Pc::IsReawakenSuggested(D, G));
	// A biome with no golden mouse stays at zero whatever Fortune adds.
	Pc::PcTier NoGolden;
	NoGolden.GoldenChance = 0.5;
	CHECK(Pc::GoldenChanceFor(D, A, NoGolden) == 0.0);
	// Passage re-locks every cycle: beating the boss in a past cycle opens nothing now.
	Pc::PcProgress Cycle;
	Cycle.BossesDefeated = { "tier_3" };
	CHECK_FALSE(Pc::BossesClearedForStage(D, Cycle, 2));
	Cycle.BossesClearedThisCycle = { "tier_3" };
	CHECK(Pc::BossesClearedForStage(D, Cycle, 2));
}

#endif
