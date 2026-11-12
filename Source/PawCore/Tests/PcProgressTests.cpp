#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcJobs.h"
#include "PcProgress.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
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

	std::set<std::string> Ids(const std::string& S)
	{
		std::set<std::string> Out;
		for (const std::string& Id : SplitKeep(S, ';'))
		{
			if (!Id.empty())
			{
				Out.insert(Id);
			}
		}
		return Out;
	}

	std::string Join(const std::set<std::string>& S)
	{
		std::string Out;
		for (const std::string& Id : S)
		{
			Out += (Out.empty() ? "" : ";") + Id;
		}
		return Out;
	}

	// The level maps as the fixture prints them: sorted, non-zero only.
	std::string PairsNz(const std::map<std::string, int64_t>& M)
	{
		std::string Out;
		for (const auto& [K, V] : M)
		{
			if (V != 0)
			{
				Out += (Out.empty() ? "" : ";") + K + "=" + std::to_string(V);
			}
		}
		return Out;
	}

	std::string PairsAll(const std::map<std::string, int64_t>& M)
	{
		std::string Out;
		for (const auto& [K, V] : M)
		{
			Out += (Out.empty() ? "" : ";") + K + "=" + std::to_string(V);
		}
		return Out;
	}

	double Hex(const std::string& S)
	{
		return std::strtod(S.c_str(), nullptr);
	}

	int64_t Int(const std::string& S)
	{
		return std::atoll(S.c_str());
	}

	const char* Flag(bool b)
	{
		return b ? "1" : "0";
	}

	using Row = std::map<std::string, std::string>;

	// pow() differs by C runtime (Godot's MinGW build calls msvcrt pow; KB Godot/03 §6): an upgrade's price can differ by a
	// few ulps, and so can the Catnip left after paying it. Those compare within 1e-14 of the largest Catnip in play this row
	// (the operands' scale, not the result's); every decision (bought, refused, how many) is still compared exactly.
	struct PowTolerance
	{
		double Scale = 0.0;
		int Inexact = 0;
		double Worst = 0.0; // relative to Scale
	};
	PowTolerance Tol;

	bool PowClose(double Got, double Want)
	{
		if (Got == Want)
		{
			return true;
		}
		const double Scale = std::max({ std::fabs(Want), std::fabs(Got), Tol.Scale });
		const double Rel = std::fabs(Got - Want) / Scale;
		++Tol.Inexact;
		Tol.Worst = std::max(Tol.Worst, Rel);
		return Rel <= 1e-14;
	}

	Pc::PcProgress ReadState(const Row& V, const std::string& Pre)
	{
		Pc::PcProgress P;
		P.UpgradeLevels = Pairs(V.at(Pre + "upgrades"));
		P.BranchLevels = Pairs(V.at(Pre + "branches"));
		P.CardLevels = Pairs(V.at(Pre + "cards"));
		P.BanishedCards = Ids(V.at(Pre + "banished"));
		P.CatchesLevel = Int(V.at(Pre + "catches_level"));
		P.CatchesPoints = Hex(V.at(Pre + "catches_points"));
		P.ExtraChoices = Int(V.at(Pre + "extra_choices"));
		P.JobsHeld = Ids(V.at(Pre + "jobs_held"));
		P.CurrentJob = V.at(Pre + "current_job");
		P.RunJob = V.at(Pre + "run_job");
		P.RunJobSecond = V.at(Pre + "run_job_second");
		P.bRunJobChosen = V.at(Pre + "run_job_chosen") == "1";
		P.Catnip = Hex(V.at(Pre + "catnip"));
		P.Stardust = Hex(V.at(Pre + "stardust"));
		P.ReawakenCount = Int(V.at(Pre + "reawaken_count"));
		P.DeepestBiomeIndex = Int(V.at(Pre + "deepest"));
		P.BossesDefeated = Ids(V.at(Pre + "bosses"));
		P.bFinalBossDefeated = V.at(Pre + "final") == "1";
		P.StardustGrantedEver = Hex(V.at(Pre + "granted"));
		P.CurrentBiomeId = V.at(Pre + "biome");
		P.TierKillCount = Int(V.at(Pre + "tier_kills"));
		P.TierKillsByBiome = Pairs(V.at(Pre + "kills_by_biome"));
		P.BaitSpentByBiome = Pairs(V.at(Pre + "bait_spent"));
		P.BossesClearedThisCycle = Ids(V.at(Pre + "cleared"));
		P.CurrentWaveIndex = Int(V.at(Pre + "wave_index"));
		P.KillsThisWave = Int(V.at(Pre + "kills_this_wave"));
		P.LifetimeCatnipThisCycle = Hex(V.at(Pre + "catnip_cycle"));
		P.LifetimeCatnipEver = Hex(V.at(Pre + "catnip_ever"));
		P.LifetimeKills = Int(V.at(Pre + "lifetime_kills"));
		P.KillsSinceTithe = Int(V.at(Pre + "kills_since_tithe"));
		P.KillsThisCycle = Int(V.at(Pre + "kills_this_cycle"));
		P.WavesHere = Int(V.at(Pre + "waves_here"));
		P.CardsTakenThisRun = Int(V.at(Pre + "cards_taken"));
		P.InfestationBonus = Hex(V.at(Pre + "infestation"));
		P.PendingSecondHelping = Hex(V.at(Pre + "pending_second"));
		P.PlaySecondsThisCycle = Hex(V.at(Pre + "play_cycle"));
		P.TotalPlaySeconds = Hex(V.at(Pre + "play_total"));
		P.FinalBossSeconds = Hex(V.at(Pre + "final_seconds"));
		P.RealtimeInterestElapsed = Hex(V.at(Pre + "interest_elapsed"));
		return P;
	}

	// Every field of the state the fixture records, compared one by one so a failure names the field.
	void CheckState(const Pc::PcProgress& P, const Row& V, const std::string& Pre)
	{
		CHECK(PairsNz(P.UpgradeLevels) == V.at(Pre + "upgrades"));
		CHECK(PairsNz(P.BranchLevels) == V.at(Pre + "branches"));
		CHECK(PairsAll(P.CardLevels) == V.at(Pre + "cards"));
		CHECK(Join(P.BanishedCards) == V.at(Pre + "banished"));
		CHECK(P.CatchesLevel == Int(V.at(Pre + "catches_level")));
		CHECK(P.CatchesPoints == Hex(V.at(Pre + "catches_points")));
		CHECK(P.ExtraChoices == Int(V.at(Pre + "extra_choices")));
		CHECK(Join(P.JobsHeld) == V.at(Pre + "jobs_held"));
		CHECK(P.CurrentJob == V.at(Pre + "current_job"));
		CHECK(P.RunJob == V.at(Pre + "run_job"));
		CHECK(P.RunJobSecond == V.at(Pre + "run_job_second"));
		CHECK(Flag(P.bRunJobChosen) == V.at(Pre + "run_job_chosen"));
		CHECK_MESSAGE(PowClose(P.Catnip, Hex(V.at(Pre + "catnip"))), P.Catnip << " vs Godot " << V.at(Pre + "catnip"));
		CHECK(P.Stardust == Hex(V.at(Pre + "stardust")));
		CHECK(P.ReawakenCount == Int(V.at(Pre + "reawaken_count")));
		CHECK(P.DeepestBiomeIndex == Int(V.at(Pre + "deepest")));
		CHECK(Join(P.BossesDefeated) == V.at(Pre + "bosses"));
		CHECK(Flag(P.bFinalBossDefeated) == V.at(Pre + "final"));
		CHECK(P.StardustGrantedEver == Hex(V.at(Pre + "granted")));
		CHECK(P.CurrentBiomeId == V.at(Pre + "biome"));
		CHECK(P.TierKillCount == Int(V.at(Pre + "tier_kills")));
		CHECK(PairsAll(P.TierKillsByBiome) == V.at(Pre + "kills_by_biome"));
		CHECK(PairsAll(P.BaitSpentByBiome) == V.at(Pre + "bait_spent"));
		CHECK(Join(P.BossesClearedThisCycle) == V.at(Pre + "cleared"));
		CHECK(P.CurrentWaveIndex == Int(V.at(Pre + "wave_index")));
		CHECK(P.KillsThisWave == Int(V.at(Pre + "kills_this_wave")));
		CHECK(P.LifetimeCatnipThisCycle == Hex(V.at(Pre + "catnip_cycle")));
		CHECK(P.LifetimeCatnipEver == Hex(V.at(Pre + "catnip_ever")));
		CHECK(P.LifetimeKills == Int(V.at(Pre + "lifetime_kills")));
		CHECK(P.KillsSinceTithe == Int(V.at(Pre + "kills_since_tithe")));
		CHECK(P.KillsThisCycle == Int(V.at(Pre + "kills_this_cycle")));
		CHECK(P.WavesHere == Int(V.at(Pre + "waves_here")));
		CHECK(P.CardsTakenThisRun == Int(V.at(Pre + "cards_taken")));
		CHECK(P.InfestationBonus == Hex(V.at(Pre + "infestation")));
		CHECK(P.PendingSecondHelping == Hex(V.at(Pre + "pending_second")));
		CHECK(P.PlaySecondsThisCycle == Hex(V.at(Pre + "play_cycle")));
		CHECK(P.TotalPlaySeconds == Hex(V.at(Pre + "play_total")));
		CHECK(P.FinalBossSeconds == Hex(V.at(Pre + "final_seconds")));
		CHECK(P.RealtimeInterestElapsed == Hex(V.at(Pre + "interest_elapsed")));
	}

	const Pc::PcBoss& BossOf(const Pc::PcGameData& D, const std::string& TierId)
	{
		const int64_t I = Pc::RealmIndex(D, TierId);
		REQUIRE(I >= 0);
		const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(I)].BossData);
		REQUIRE(Boss != nullptr);
		return *Boss;
	}

	struct ProgressResult
	{
		int Rows = 0;
		std::map<std::string, int> Ops;
		std::map<std::string, int> Paths; // each op's interesting outcomes
	};

	ProgressResult RunProgressFixture(const std::string& Path)
	{
		ProgressResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		const Pc::PcGameData& D = Game();
		std::vector<std::string> UpgradeIds;
		for (const Pc::PcUpgrade& U : D.Upgrades)
		{
			UpgradeIds.push_back(U.Id);
		}
		std::sort(UpgradeIds.begin(), UpgradeIds.end());
		std::vector<std::string> BossTiers;
		for (const Pc::PcTier& T : D.Tiers)
		{
			if (!T.BossData.empty())
			{
				BossTiers.push_back(T.Id);
			}
		}
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
			Row V;
			for (size_t I = 0; I < Cells.size(); ++I)
			{
				V[Header[I]] = Cells[I];
			}
			CAPTURE(R.Rows);
			Pc::PcProgress P = ReadState(V, "s0_");
			// The row's Catnip scale: the bank at the start, and every bank the ops leave (Godot's, from the digests).
			Tol.Scale = std::fabs(P.Catnip);
			for (const std::string& Dg : SplitKeep(V.at("digests"), '|'))
			{
				Tol.Scale = std::max(Tol.Scale, std::fabs(Hex(SplitKeep(Dg, ',')[0])));
			}
			// The starting state round-trips: the reader and the checker agree before any rule runs.
			CheckState(P, V, "s0_");

			const std::vector<std::string> Ops = SplitKeep(V.at("ops"), '|');
			const std::vector<std::string> Results = SplitKeep(V.at("results"), '|');
			const std::vector<std::string> Digests = SplitKeep(V.at("digests"), '|');
			REQUIRE(Ops.size() == Results.size());
			REQUIRE(Ops.size() == Digests.size());
			for (size_t K = 0; K < Ops.size(); ++K)
			{
				const std::string& Op = Ops[K];
				CAPTURE(K);
				CAPTURE(Op);
				const size_t Colon = Op.find(':');
				const std::string Name = Op.substr(0, Colon);
				const std::string Arg = Op.substr(Colon + 1);
				const std::vector<std::string> Args = SplitKeep(Arg, ',');
				const std::string& Want = Results[K];
				R.Ops[Name] += 1;
				if (Name == "kill")
				{
					int64_t Turns = 0;
					for (int64_t M = Int(Arg); M > 0; --M)
					{
						Turns += Pc::IncrementKillCount(D, P).bWaveTurned ? 1 : 0;
					}
					CHECK(std::to_string(Turns) + "," + std::to_string(P.CurrentWaveIndex) == Want);
					R.Paths["wave turned"] += Turns > 0 ? 1 : 0;
				}
				else if (Name == "add")
				{
					Pc::AddCurrency(D, P, Hex(Arg));
				}
				else if (Name == "untouched")
				{
					Pc::CreditUntouchedCatnip(P, Hex(Arg));
				}
				else if (Name == "grant")
				{
					Pc::GrantAchievementStardust(P, Hex(Arg));
				}
				else if (Name == "spend")
				{
					Pc::SpendStardust(P, Hex(Arg));
				}
				else if (Name == "travel")
				{
					const Pc::PcArrival A = Pc::BeginBiome(D, P, Arg);
					R.Paths["travel second helping"] += A.SecondHelpingPaid > 0.0 ? 1 : 0;
					R.Paths["travel deeper"] += A.bDeeper ? 1 : 0;
					R.Paths["travel no-op"] += A.bMoved ? 0 : 1;
				}
				else if (Name == "boss")
				{
					const Pc::PcBossMark M = Pc::MarkBossDefeated(D, P, Arg);
					CHECK(std::string(Flag(M.bNew)) + "," + (M.bDefeated ? "1" : "0") == Want);
					R.Paths[M.bNew ? "boss new" : (M.bDefeated ? "boss rematch passage" : "boss nothing")] += 1;
				}
				else if (Name == "bait")
				{
					const bool bOk = Pc::BuyBait(D, P, BossOf(D, Arg));
					CHECK(Flag(bOk) == Want);
					R.Paths[bOk ? "bait bought" : "bait refused"] += 1;
				}
				else if (Name == "buy")
				{
					const bool bOk = Pc::PurchaseUpgrade(D, P, Arg);
					CHECK(Flag(bOk) == Want);
					R.Paths[bOk ? "buy bought" : "buy refused"] += 1;
				}
				else if (Name == "bulk")
				{
					const int64_t Got = Pc::PurchaseUpgradeBulk(D, P, Args[0], Int(Args[1]));
					CHECK(std::to_string(Got) == Want);
					R.Paths["bulk several"] += Got > 1 ? 1 : 0;
				}
				else if (Name == "preview")
				{
					const Pc::PcBulkPreview Pv = Pc::PreviewBulkPurchase(D, P, Args[0], Int(Args[1]));
					const std::vector<std::string> W = SplitKeep(Want, ',');
					CHECK(std::to_string(Pv.Levels) == W[0]);
					CHECK_MESSAGE(PowClose(Pv.Cost, Hex(W[1])), Pv.Cost << " vs Godot " << W[1]);
					R.Paths["preview several"] += Pv.Levels > 1 ? 1 : 0;
				}
				else if (Name == "reawaken")
				{
					const Pc::PcReawakenResult Re = Pc::Reawaken(D, P, Arg == "1");
					const std::vector<std::string> W = SplitKeep(Want, ',');
					CHECK(Flag(Re.bDone) == W[0]);
					CHECK(Re.StardustGained == Hex(W[1]));
					R.Paths[Re.bDone ? "reawaken done" : "reawaken refused"] += 1;
				}
				else if (Name == "play")
				{
					Pc::TickPlayClock(P, Hex(Arg));
				}
				else if (Name == "interest")
				{
					R.Paths["interest paid"] += Pc::TickRealtimeInterest(D, P, Hex(Arg)) > 0.0 ? 1 : 0;
				}
				else if (Name == "infest")
				{
					Pc::TickInfestation(D, P, Hex(Arg));
				}
				else if (Name == "newgame")
				{
					Pc::NewGame(D, P);
				}
				else
				{
					FAIL("unknown op " << Name);
				}
				const std::vector<std::string> G = SplitKeep(Digests[K], ',');
				REQUIRE(G.size() == 7);
				CHECK_MESSAGE(PowClose(P.Catnip, Hex(G[0])), P.Catnip << " vs Godot " << G[0]);
				CHECK(P.Stardust == Hex(G[1]));
				CHECK(P.CurrentBiomeId == G[2]);
				CHECK(P.TierKillCount == Int(G[3]));
				CHECK(P.CurrentWaveIndex == Int(G[4]));
				CHECK(P.KillsThisWave == Int(G[5]));
				CHECK(P.ReawakenCount == Int(G[6]));
			}
			CheckState(P, V, "s1_");

			// The reads of the state the ops left.
			CHECK(std::to_string(Pc::VisualStage(D, P)) == V.at("visual_stage"));
			CHECK(std::to_string(Pc::EvolutionStage(D, P)) == V.at("evolution_stage"));
			const std::vector<std::string> AtCap = SplitKeep(V.at("at_cap"), ';');
			const std::vector<std::string> Visible = SplitKeep(V.at("visible"), ';');
			const std::vector<std::string> Costs = SplitKeep(V.at("costs"), ';');
			REQUIRE(AtCap.size() == UpgradeIds.size());
			for (size_t U = 0; U < UpgradeIds.size(); ++U)
			{
				CAPTURE(UpgradeIds[U]);
				CHECK(Flag(Pc::IsUpgradeAtCap(D, P, UpgradeIds[U])) == AtCap[U]);
				CHECK(Flag(Pc::IsUpgradeVisible(P, UpgradeIds[U])) == Visible[U]);
				CHECK_MESSAGE(PowClose(Pc::UpgradeCostById(D, P, UpgradeIds[U]), Hex(Costs[U])), "cost of " << UpgradeIds[U]);
				R.Paths["at cap"] += AtCap[U] == "1" ? 1 : 0;
			}
			CHECK(Flag(Pc::IsKittenPowerLocked(D, P)) == V.at("kitten_power_locked"));
			CHECK(Flag(Pc::IsKittenSpeedLocked(D, P)) == V.at("kitten_speed_locked"));
			const std::vector<std::string> BaitUnlocked = SplitKeep(V.at("bait_unlocked"), ';');
			const std::vector<std::string> BaitAffordable = SplitKeep(V.at("bait_affordable"), ';');
			REQUIRE(BaitUnlocked.size() == BossTiers.size());
			for (size_t B = 0; B < BossTiers.size(); ++B)
			{
				CAPTURE(BossTiers[B]);
				CHECK(Flag(Pc::IsBaitUnlocked(D, P, BossOf(D, BossTiers[B]))) == BaitUnlocked[B]);
				CHECK(Flag(Pc::CanAffordBait(P, BossOf(D, BossTiers[B]))) == BaitAffordable[B]);
			}
			R.Paths["visual above earned"] += Pc::VisualStage(D, P) > Pc::EvolutionStage(D, P) ? 1 : 0;
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcProgress matches Godot's GameState mutations on 1,000 random states x 12 operations")
{
	const ProgressResult R = RunProgressFixture(DataDir() + "/../Source/PawCore/Tests/Data/progress_godot.tsv");
	CHECK(R.Rows == 1000);
	// Every operation, and each one's outcomes both ways.
	for (const char* Op : { "kill", "add", "untouched", "grant", "spend", "travel", "boss", "bait", "buy", "bulk", "preview",
		"reawaken", "play", "interest", "infest" })
	{
		CHECK_MESSAGE(R.Ops.count(Op) == 1, Op);
		CHECK_MESSAGE(R.Ops.at(Op) > 100, Op);
	}
	CHECK(R.Ops.at("newgame") > 20);
	std::string Seen;
	for (const auto& [Path, N] : R.Paths)
	{
		Seen += Path + " " + std::to_string(N) + ", ";
	}
	for (const char* Path : { "wave turned", "travel second helping", "travel deeper", "travel no-op", "boss new",
		"boss rematch passage", "boss nothing", "bait bought", "bait refused", "buy bought", "buy refused", "bulk several",
		"preview several", "reawaken done", "reawaken refused", "interest paid", "at cap", "visual above earned" })
	{
		CHECK_MESSAGE(R.Paths.count(Path) == 1, Path);
		CHECK_MESSAGE(R.Paths.at(Path) > 10, Path << " (" << Seen << ")");
	}
	MESSAGE(Seen);
	MESSAGE("Catnip and prices within the pow tolerance: " << Tol.Inexact << " inexact, worst " << Tol.Worst << " of scale");
}

TEST_CASE("Long: PcProgress matches all 3,000 Godot mutation rows" * doctest::test_suite("long") * doctest::skip())
{
	const ProgressResult R = RunProgressFixture(DataDir() + "/../build/progress_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Progress rules from the Godot comments")
{
	const Pc::PcGameData& D = Game();
	const std::string Final = D.Code.BossRealms.back();

	// The final boss: the ending flag and the clock stop on the FIRST clear only.
	{
		Pc::PcProgress P;
		P.TotalPlaySeconds = 100.0;
		const Pc::PcBossMark First = Pc::RecordBossDefeat(D, P, Final, 50.0);
		CHECK(First.bNew);
		CHECK(P.bFinalBossDefeated);
		CHECK(P.FinalBossSeconds == 100.0);
		CHECK(P.Catnip > 0.0); // the reward is credited as a kill payout
		P.TotalPlaySeconds = 500.0;
		P.BossesClearedThisCycle.clear();
		const Pc::PcBossMark Rematch = Pc::RecordBossDefeat(D, P, Final, 50.0);
		CHECK_FALSE(Rematch.bNew);
		CHECK(Rematch.bDefeated); // this cycle's passage opens
		CHECK(P.FinalBossSeconds == 100.0);
		// Another boss never sets the ending.
		Pc::PcProgress Q;
		Pc::RecordBossDefeat(D, Q, D.Code.BossRealms.front(), 0.0);
		CHECK_FALSE(Q.bFinalBossDefeated);
	}

	// Data with no bosses (refused by the validator) must not read past the empty list: nothing is the final boss.
	{
		Pc::PcGameData NoBosses = D;
		NoBosses.Code.BossRealms.clear();
		Pc::PcProgress P;
		P.TotalPlaySeconds = 100.0;
		const Pc::PcBossMark M = Pc::RecordBossDefeat(NoBosses, P, Final, 0.0);
		CHECK(M.bNew);
		CHECK_FALSE(P.bFinalBossDefeated);
		CHECK(P.FinalBossSeconds == 0.0);
		CHECK_FALSE(Pc::RaisesVictory(NoBosses, Final, M));
	}

	// A refused summon keeps the bait AND the Catnip (all-or-nothing): one Catnip short.
	for (const Pc::PcTier& T : D.Tiers)
	{
		const Pc::PcBoss* Boss = D.FindBoss(T.BossData);
		if (!Boss || Boss->BaitRequired <= 0 || Boss->BaitCostCatnip <= 0.0)
		{
			continue;
		}
		CAPTURE(T.Id);
		Pc::PcProgress P;
		P.CurrentBiomeId = T.Id;
		P.BaitKillsByBiome[P.CurrentBiomeId] = Boss->BaitRequired * Boss->KillsPerBait;
		P.Catnip = Boss->BaitCostCatnip - 1.0;
		REQUIRE(Pc::IsBaitUnlocked(D, P, *Boss));
		CHECK_FALSE(Pc::BuyBait(D, P, *Boss));
		CHECK(Pc::BaitHeld(D, P, T.Id) == Boss->BaitRequired);
		CHECK(P.Catnip == Boss->BaitCostCatnip - 1.0);
		P.Catnip = Boss->BaitCostCatnip;
		CHECK(Pc::BuyBait(D, P, *Boss));
		CHECK(Pc::BaitHeld(D, P, T.Id) == 0);
		CHECK(P.Catnip == 0.0);
	}

	// The epsilon: a billionth short still buys, and the bank never goes negative.
	{
		const Pc::PcUpgrade& U = D.Upgrades.front();
		Pc::PcProgress P;
		P.Catnip = Pc::UpgradeCost(U, 0) - 1e-9;
		REQUIRE(Pc::IsUpgradeVisible(P, U.Id));
		CHECK(Pc::PurchaseUpgrade(D, P, U.Id));
		CHECK(P.Catnip == 0.0);
	}

	// Kitten Speed is refused with no kitten, in the rule and not only by the shop's hidden button (F8, 2026-11-03).
	{
		Pc::PcProgress P;
		P.Catnip = 1e9;
		REQUIRE(Pc::IsKittenSpeedLocked(D, P));
		CHECK_FALSE(Pc::PurchaseUpgrade(D, P, "kitten_speed"));
		CHECK(P.Catnip == 1e9);
		CHECK(Pc::PurchaseUpgradeBulk(D, P, "kitten_speed", 10) == 0);
		P.UpgradeLevels["kitten_count"] = 1;
		REQUIRE_FALSE(Pc::IsKittenSpeedLocked(D, P));
		CHECK(Pc::PurchaseUpgrade(D, P, "kitten_speed"));
	}

	// Bulk = single buys looped; the preview agrees with what the bulk then buys.
	for (const Pc::PcUpgrade& U : D.Upgrades)
	{
		// The kitten lines are refused with no kitten (the preview, like Godot's, does not re-check the lock).
		if (!Pc::IsUpgradeVisible(Pc::PcProgress(), U.Id) || U.Id == "kitten_power" || U.Id == "kitten_speed")
		{
			continue;
		}
		CAPTURE(U.Id);
		Pc::PcProgress P;
		P.Catnip = 1e6;
		const Pc::PcBulkPreview Pv = Pc::PreviewBulkPurchase(D, P, U.Id, -1);
		const double Before = P.Catnip;
		CHECK(Pc::PurchaseUpgradeBulk(D, P, U.Id, -1) == Pv.Levels);
		CHECK(Before - P.Catnip == doctest::Approx(Pv.Cost));
		CHECK(Pv.Levels <= D.Code.BulkMaxSteps);
	}

	// The Reawaken keeps the tree, the ledger, the jobs held and the preference; resets the run.
	{
		Pc::PcProgress P;
		P.LifetimeCatnipEver = 1e12;
		P.LifetimeCatnipThisCycle = 1e12;
		P.Catnip = 12345.0;
		P.BranchLevels["vitality"] = 1;
		P.BossesDefeated = { D.Code.BossRealms.front() };
		P.BossesClearedThisCycle = { D.Code.BossRealms.front() };
		P.JobsHeld = { "blacksmith" };
		REQUIRE(Pc::LockRunJob(P, "blacksmith"));
		P.CurrentBiomeId = "tier_2";
		P.TierKillsByBiome["tier_1"] = 99;
		P.CardLevels["any"] = 2;
		REQUIRE(Pc::CanReawaken(D, P));
		const double Pending = Pc::PendingStardust(D, P);
		const Pc::PcReawakenResult Re = Pc::Reawaken(D, P);
		REQUIRE(Re.bDone);
		CHECK(Re.StardustGained == Pending);
		CHECK(P.Stardust == Pending);
		CHECK(P.Catnip == 0.0); // no Legacy
		CHECK(P.BranchLevels.at("vitality") == 1);
		CHECK(P.BossesDefeated.size() == 1);
		CHECK(P.BossesClearedThisCycle.size() == 1); // D17 E1: a beaten boss stays beaten
		CHECK(P.JobsHeld.count("blacksmith") == 1);
		CHECK(P.CurrentJob == "blacksmith");
		CHECK(P.RunJob.empty());
		CHECK(P.CurrentBiomeId.empty());
		CHECK(P.TierKillsByBiome.empty());
		CHECK(P.CardLevels.empty());
		CHECK(P.ReawakenCount == 1);
		// Split-invariant: a second Reawaken straight away has nothing to grant.
		CHECK(Pc::PendingStardust(D, P) == 0.0);
	}

	// Second Helping: half now, half on reaching biome 4; never on reaching biome 3.
	{
		Pc::PcProgress P;
		P.LifetimeCatnipEver = 1e12;
		P.BranchLevels["second_helping"] = 1;
		REQUIRE(Pc::BranchEffect(D, P, "second_helping") > 0.0);
		const double Pending = Pc::PendingStardust(D, P);
		REQUIRE(Pc::Reawaken(D, P, true).bDone);
		CHECK(P.Stardust == Pending - Pending * 0.5);
		CHECK(P.PendingSecondHelping == Pending * 0.5);
		CHECK(Pc::BeginBiome(D, P, D.Tiers[2].Id).SecondHelpingPaid == 0.0);
		CHECK(Pc::BeginBiome(D, P, D.Tiers[3].Id).SecondHelpingPaid == std::floor(Pending * 0.5));
		CHECK(P.PendingSecondHelping == 0.0);
	}

	// Travel keeps each place's kills; Reputation is lost by leaving; going back is not "deeper".
	{
		Pc::PcProgress P;
		Pc::BeginBiome(D, P, D.Tiers[0].Id);
		P.TierKillCount = 77;
		P.WavesHere = 5;
		CHECK(Pc::BeginBiome(D, P, D.Tiers[1].Id).bDeeper);
		CHECK(P.WavesHere == 0);
		CHECK(P.TierKillCount == 0);
		CHECK_FALSE(Pc::BeginBiome(D, P, D.Tiers[0].Id).bDeeper);
		CHECK(P.TierKillCount == 77);
		CHECK_FALSE(Pc::BeginBiome(D, P, D.Tiers[0].Id).bMoved);
	}

	// A new game clears everything a save carries, the tree included.
	{
		Pc::PcProgress P;
		P.BranchLevels["vitality"] = 3;
		P.JobsHeld = { "blacksmith" };
		P.CurrentJob = "blacksmith";
		P.LifetimeKills = 5;
		P.FinalBossSeconds = 9.0;
		P.bFinalBossDefeated = true;
		P.UpgradeLevels[D.Upgrades.front().Id] = 4;
		Pc::NewGame(D, P);
		CHECK(Pc::BranchLevel(P, "vitality") == 0);
		CHECK(P.JobsHeld.empty());
		CHECK(P.CurrentJob.empty());
		CHECK(P.LifetimeKills == 0);
		CHECK(P.FinalBossSeconds == 0.0);
		CHECK_FALSE(P.bFinalBossDefeated);
		CHECK(Pc::UpgradeLevel(P, D.Upgrades.front().Id) == 0);
	}

	// The clocks: Infestation never goes below zero; Compound Interest pays once per period and its clock resets unowned.
	{
		Pc::PcProgress P;
		P.InfestationBonus = 0.1;
		Pc::TickInfestation(D, P, 10.0);
		CHECK(P.InfestationBonus == 0.0);
		P.RealtimeInterestElapsed = 30.0;
		CHECK(Pc::TickRealtimeInterest(D, P, 1.0) == 0.0);
		CHECK(P.RealtimeInterestElapsed == 0.0);
	}
}

TEST_CASE("D17 E1/E2: a Reawaken keeps a beaten boss beaten and the Bait earned toward the next; the waves restart")
{
	// Designer, 2026-11-12 (docs/ECONOMY-REVIEW-2026-11-12.md): Godot re-locked passage on every Reawaken and cleared the
	// Bait with the waves, so every run re-farmed 40 Bait for every boss already beaten: 52 % of the game.
	const Pc::PcGameData& D = Game();
	const std::string First = D.Code.BossRealms[0];
	const std::string Second = D.Code.BossRealms[1];
	const Pc::PcTier* SecondTier = nullptr;
	for (const Pc::PcTier& T : D.Tiers)
	{
		SecondTier = T.Id == Second ? &T : SecondTier;
	}
	REQUIRE(SecondTier);
	const Pc::PcBoss* SecondBoss = D.FindBoss(SecondTier->BossData);
	REQUIRE(SecondBoss);

	Pc::PcProgress P;
	P.LifetimeCatnipEver = 1e12;
	Pc::MarkBossDefeated(D, P, First);
	REQUIRE(Pc::EvolutionStage(D, P) == 2);
	// Three Bait's worth of kills in the second boss's biome, one Bait spent on a summon that ran out of time.
	Pc::BeginBiome(D, P, Second);
	for (int64_t K = 0; K < 3 * SecondBoss->KillsPerBait; ++K)
	{
		Pc::IncrementKillCount(D, P);
	}
	P.BaitSpentByBiome[Second] = 1;
	REQUIRE(Pc::BaitEarned(D, P, Second) == 3);
	REQUIRE(Pc::CurrentWave(D, *SecondTier, P.TierKillCount) > 0);

	REQUIRE(Pc::Reawaken(D, P, true).bDone);

	// E1: the first boss stays beaten: its passage is open in the new run, and a biome behind it opens on Catnip alone.
	CHECK(Pc::IsBossClearedThisCycle(P, First));
	CHECK(Pc::BossesClearedForStage(D, P, 2));
	const Pc::PcTier* Behind = nullptr;
	for (const Pc::PcTier& T : D.Tiers)
	{
		Behind = (!Behind && T.RequiredEvolutionStage == 2) ? &T : Behind;
	}
	REQUIRE(Behind);
	P.LifetimeCatnipThisCycle = Behind->CatnipToUnlock;
	CHECK(Pc::IsBiomeUnlocked(D, P, *Behind));
	// E2: the Bait earned and spent toward the unbeaten boss is kept...
	CHECK(Pc::BaitEarned(D, P, Second) == 3);
	CHECK(P.BaitSpentByBiome.at(Second) == 1);
	// ...and the waves are not: back in the biome, the run starts at wave 0.
	CHECK(P.TierKillsByBiome.empty());
	Pc::BeginBiome(D, P, Second);
	CHECK(P.TierKillCount == 0);
	CHECK(Pc::CurrentWave(D, *SecondTier, P.TierKillCount) == 0);
	// Kills keep paying into the same bank.
	for (int64_t K = 0; K < SecondBoss->KillsPerBait; ++K)
	{
		Pc::IncrementKillCount(D, P);
	}
	CHECK(Pc::BaitEarned(D, P, Second) == 4);
	// A new game forgets the bank.
	Pc::NewGame(D, P);
	CHECK(P.BaitKillsByBiome.empty());
	CHECK(Pc::BaitEarned(D, P, Second) == 0);
}

#endif
