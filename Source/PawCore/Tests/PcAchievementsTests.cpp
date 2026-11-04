#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcAchievements.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcProgress.h"

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

	// One bit per roster entry, in roster order (the fixture's digest).
	std::string Bits(const Pc::PcGameData& D, const std::set<std::string>& S)
	{
		std::string Out;
		for (const Pc::PcAchievement& A : D.Achievements)
		{
			Out += S.count(A.Id) ? '1' : '0';
		}
		return Out;
	}

	std::string JoinList(const std::vector<std::string>& S)
	{
		std::string Out;
		for (size_t I = 0; I < S.size(); ++I)
		{
			Out += (I ? ";" : "") + S[I];
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

	struct AchievementsResult
	{
		int Rows = 0;
		std::map<std::string, int> Ops;
		std::map<std::string, int> Paths;
	};

	AchievementsResult RunAchievementsFixture(const std::string& Path)
	{
		AchievementsResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
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

			// The roster order, written once.
			if (!V.at("roster").empty())
			{
				std::vector<std::string> Roster;
				for (const Pc::PcAchievement& A : Game().Achievements)
				{
					Roster.push_back(A.Id);
				}
				CHECK(JoinList(Roster) == V.at("roster"));
				R.Paths["roster order"] += 1;
			}

			// This row's roster: the shipping one, or a copy with Godot's in-memory overwrites.
			Pc::PcGameData Edited;
			const bool bEdited = !V.at("overrides").empty();
			if (bEdited)
			{
				Edited = Game();
				for (const std::string& O : SplitKeep(V.at("overrides"), ';'))
				{
					const std::vector<std::string> F = SplitKeep(O, ',');
					REQUIRE(F.size() == 3);
					bool bFound = false;
					for (Pc::PcAchievement& A : Edited.Achievements)
					{
						if (A.Id == F[0])
						{
							A.Threshold = Hex(F[1]);
							A.StardustReward = Hex(F[2]);
							bFound = true;
						}
					}
					REQUIRE_MESSAGE(bFound, F[0]);
				}
				Edited.RebuildIndex();
				R.Paths["edited roster"] += 1;
			}
			const Pc::PcGameData& D = bEdited ? Edited : Game();

			Pc::PcProgress P;
			P.LifetimeKills = Int(V.at("s0_kills"));
			P.LifetimeCatnipEver = Hex(V.at("s0_catnip_ever"));
			P.ReawakenCount = Int(V.at("s0_reawaken_count"));
			P.BossesDefeated = Ids(V.at("s0_bosses"));
			P.bFinalBossDefeated = V.at("s0_final") == "1";
			P.UpgradeLevels = Pairs(V.at("s0_upgrades"));
			P.BranchLevels = Pairs(V.at("s0_branches"));
			P.Stardust = Hex(V.at("s0_stardust"));
			P.StardustGrantedEver = Hex(V.at("s0_granted"));

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
				if (Name == "load")
				{
					REQUIRE(Args.size() == 2);
					const std::vector<std::string> Got = Pc::ApplyAchievementSave(D, P, Ids(Args[0]), Ids(Args[1]));
					CHECK(JoinList(Got) == Want);
					R.Paths["load earned"] += Got.empty() ? 0 : 1;
					R.Paths["load retired id"] += Ids(Args[0]).count("retired_achievement") ? 1 : 0;
				}
				else if (Name == "set")
				{
					const std::string& What = Args[0];
					if (What == "kills")
					{
						P.LifetimeKills = Int(Args[1]);
					}
					else if (What == "catnip")
					{
						P.LifetimeCatnipEver = Hex(Args[1]);
					}
					else if (What == "reawaken")
					{
						P.ReawakenCount = Int(Args[1]);
					}
					else if (What == "boss")
					{
						P.BossesDefeated.insert(Args[1]);
					}
					else if (What == "final")
					{
						P.bFinalBossDefeated = true;
					}
					else if (What == "up")
					{
						P.UpgradeLevels[Args[1]] = Int(Args[2]);
					}
					else if (What == "br")
					{
						P.BranchLevels[Args[1]] = Int(Args[2]);
					}
					else
					{
						REQUIRE(What == "none");
					}
					const std::vector<std::string> Got = Pc::EvaluateAchievements(D, P);
					CHECK(JoinList(Got) == Want);
					R.Paths["set earned"] += Got.empty() ? 0 : 1;
					R.Paths["set earned several"] += Got.size() > 1 ? 1 : 0;
				}
				else if (Name == "claim")
				{
					const double Got = Pc::ClaimAchievement(D, P, Arg);
					CHECK_MESSAGE(Got == Hex(Want), Got << " vs Godot " << Want);
					R.Paths[Got > 0.0 ? "claim paid" : "claim refused"] += 1;
				}
				else if (Name == "claimall")
				{
					const double Got = Pc::ClaimAllAchievements(D, P);
					CHECK_MESSAGE(Got == Hex(Want), Got << " vs Godot " << Want);
					R.Paths[Got > 0.0 ? "claim all paid" : "claim all nothing"] += 1;
					R.Paths["claim all fractional"] += Got != std::floor(Got) ? 1 : 0;
				}
				else if (Name == "newgame")
				{
					// The title screen: debug_reset_progress() (its signals run one re-scan against the OLD earned set),
					// then the tree's and the achievements' apply_save_data({}).
					Pc::NewGame(D, P);
					std::vector<std::string> Got = Pc::EvaluateAchievements(D, P);
					for (const std::string& Id : Pc::ApplyAchievementSave(D, P, {}, {}))
					{
						Got.push_back(Id);
					}
					CHECK(JoinList(Got) == Want);
				}
				else
				{
					FAIL("unknown op " << Name);
				}
				const std::vector<std::string> G = SplitKeep(Digests[K], ',');
				REQUIRE(G.size() == 6);
				CHECK(Bits(D, P.AchievementsEarned) == G[0]);
				CHECK(Bits(D, P.AchievementsClaimed) == G[1]);
				CHECK(std::to_string(P.AchievementsEarned.size()) == G[2]);
				CHECK(std::to_string(P.AchievementsClaimed.size()) == G[3]);
				CHECK(P.Stardust == Hex(G[4]));
				CHECK(P.StardustGrantedEver == Hex(G[5]));
			}

			// The reads of the state the ops left, for every achievement and an unknown id.
			const std::vector<std::string> Asked = SplitKeep(V.at("asked"), ';');
			const std::vector<std::string> Progress = SplitKeep(V.at("progress"), ';');
			const std::vector<std::string> Reward = SplitKeep(V.at("reward"), ';');
			const std::vector<std::string> Earned = SplitKeep(V.at("earned"), ';');
			const std::vector<std::string> Claimable = SplitKeep(V.at("claimable"), ';');
			const std::vector<std::string> Claimed = SplitKeep(V.at("claimed"), ';');
			REQUIRE(Asked.size() == D.Achievements.size() + 1);
			REQUIRE(Progress.size() == Asked.size());
			for (size_t I = 0; I < Asked.size(); ++I)
			{
				const std::string& Id = Asked[I];
				CAPTURE(Id);
				const Pc::PcAchievement* A = Pc::FindAchievement(D, Id);
				CHECK((A != nullptr) == !Progress[I].empty());
				if (A)
				{
					const double Value = Pc::AchievementProgress(D, P, *A);
					CHECK_MESSAGE(Value == Hex(Progress[I]), Value << " vs Godot " << Progress[I]);
					R.Paths["on the threshold"] += Value == A->Threshold ? 1 : 0;
					R.Paths["earned, pays nothing"] +=
						Pc::IsAchievementEarned(P, Id) && !Pc::IsAchievementClaimed(P, Id) && A->StardustReward <= 0.0 ? 1 : 0;
				}
				CHECK(Pc::AchievementReward(D, Id) == Hex(Reward[I]));
				CHECK(Flag(Pc::IsAchievementEarned(P, Id)) == Earned[I]);
				CHECK(Flag(Pc::IsAchievementClaimable(D, P, Id)) == Claimable[I]);
				CHECK(Flag(Pc::IsAchievementClaimed(P, Id)) == Claimed[I]);
				R.Paths["claimable"] += Claimable[I] == "1" ? 1 : 0;
				R.Paths["claimed, not earned"] += Claimed[I] == "1" && Earned[I] == "0" ? 1 : 0;
			}
			CHECK(std::to_string(Pc::EarnedAchievementCount(D, P)) == V.at("earned_count"));
			CHECK(std::to_string(Pc::UnclaimedAchievementCount(D, P)) == V.at("unclaimed_count"));
			CHECK(Pc::UnclaimedStardust(D, P) == Hex(V.at("unclaimed_stardust")));
			CHECK(Pc::TotalStardustOnOffer(D) == Hex(V.at("on_offer")));
			R.Paths["retired id counted"] += P.AchievementsEarned.count("retired_achievement") ? 1 : 0;
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcAchievements matches Godot's AchievementManager on 1,000 random states x 10 operations")
{
	const AchievementsResult R = RunAchievementsFixture(DataDir() + "/../Source/PawCore/Tests/Data/achievements_godot.tsv");
	CHECK(R.Rows == 1000);
	for (const char* Op : { "load", "set", "claim", "claimall" })
	{
		CHECK_MESSAGE(R.Ops.count(Op) == 1, Op);
		CHECK_MESSAGE(R.Ops.at(Op) > 100, Op);
	}
	CHECK(R.Ops.count("newgame") == 1);
	CHECK(R.Ops.at("newgame") > 20);
	std::string Seen;
	for (const auto& [Path, N] : R.Paths)
	{
		Seen += Path + " " + std::to_string(N) + ", ";
	}
	CHECK(R.Paths.count("roster order") == 1);
	for (const char* Path : { "edited roster", "load earned", "load retired id", "set earned", "set earned several",
		"claim paid", "claim refused", "claim all paid", "claim all nothing", "claim all fractional", "on the threshold",
		"earned, pays nothing", "claimable", "claimed, not earned", "retired id counted" })
	{
		CHECK_MESSAGE(R.Paths.count(Path) == 1, Path);
		CHECK_MESSAGE(R.Paths.at(Path) > 10, Path << " (" << Seen << ")");
	}
	MESSAGE(Seen);
}

TEST_CASE("Long: PcAchievements matches all 3,000 Godot achievement rows" * doctest::test_suite("long") * doctest::skip())
{
	const AchievementsResult R = RunAchievementsFixture(DataDir() + "/../build/achievements_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Achievement rules from the Godot comments")
{
	const Pc::PcGameData& D = Game();

	// The roster is in display order (the numbered file names), not the CSV's id order.
	REQUIRE(D.Achievements.size() == 39); // 41 until F2 (2026-11-03) removed the two that counted merged-away branches
	CHECK(D.Achievements.front().Id == "kills_1");
	CHECK(D.Achievements.back().Id == "br_unicorn_5");

	// Earning and collecting are two events; a claim pays once, into Stardust AND Stardust ever granted.
	{
		Pc::PcProgress P;
		CHECK(Pc::ClaimAchievement(D, P, "kills_1") == 0.0); // not earned
		P.LifetimeKills = 1;
		CHECK(Pc::EvaluateAchievements(D, P) == std::vector<std::string>{ "kills_1" });
		CHECK(P.Stardust == 0.0); // earned, not paid
		CHECK(Pc::UnclaimedAchievementCount(D, P) == 1);
		CHECK(Pc::ClaimAchievement(D, P, "kills_1") == 1.0);
		CHECK(P.Stardust == 1.0);
		CHECK(P.StardustGrantedEver == 1.0);
		CHECK(Pc::ClaimAchievement(D, P, "kills_1") == 0.0); // a double-click pays nothing
		CHECK(P.Stardust == 1.0);
		CHECK(Pc::UnclaimedAchievementCount(D, P) == 0);
		// Nothing un-earns: the counter falling leaves it earned, and a re-scan only adds.
		P.LifetimeKills = 0;
		CHECK(Pc::EvaluateAchievements(D, P).empty());
		CHECK(Pc::IsAchievementEarned(P, "kills_1"));
	}

	// An achievement that pays nothing is not claimable, earned or not; the reward switched off in data switches the claim
	// off with it.
	{
		Pc::PcGameData Free = D;
		for (Pc::PcAchievement& A : Free.Achievements)
		{
			A.StardustReward = 0.0;
		}
		Pc::PcProgress P;
		P.LifetimeKills = 100;
		CHECK(Pc::EvaluateAchievements(Free, P).size() == 2);
		CHECK(Pc::UnclaimedAchievementCount(Free, P) == 0);
		CHECK(Pc::UnclaimedStardust(Free, P) == 0.0);
		CHECK(Pc::ClaimAllAchievements(Free, P) == 0.0);
		CHECK(P.AchievementsClaimed.empty());
	}

	// Claim all takes everything waiting, in one go, and the badge's Stardust says what that was worth.
	{
		Pc::PcProgress P;
		P.LifetimeKills = 1000;
		P.ReawakenCount = 2;
		Pc::EvaluateAchievements(D, P);
		const double Waiting = Pc::UnclaimedStardust(D, P);
		CHECK(Waiting > 0.0);
		CHECK(Pc::ClaimAllAchievements(D, P) == Waiting);
		CHECK(P.Stardust == Waiting);
		CHECK(Pc::UnclaimedStardust(D, P) == 0.0);
	}

	// Bosses count the permanent ledger; the stage counts bosses in ladder order.
	{
		Pc::PcProgress P;
		P.BossesDefeated = { D.Code.BossRealms[1] };
		CHECK(Pc::AchievementProgress(D, P, *Pc::FindAchievement(D, "boss_1")) == 1.0);
		CHECK(Pc::AchievementProgress(D, P, *Pc::FindAchievement(D, "stage_2")) == 1.0); // the first boss is not beaten
		P.BossesClearedThisCycle = { D.Code.BossRealms[0], D.Code.BossRealms[2] };              // passage is not power
		CHECK(Pc::AchievementProgress(D, P, *Pc::FindAchievement(D, "boss_1")) == 1.0);
	}

	// A load replaces both sets and re-scans; a new game ends with both empty.
	{
		Pc::PcProgress P;
		P.LifetimeKills = 100;
		const std::vector<std::string> Got = Pc::ApplyAchievementSave(D, P, { "kills_1", "retired" }, { "kills_1" });
		CHECK(Got == std::vector<std::string>{ "kills_100" });
		CHECK(Pc::EarnedAchievementCount(D, P) == 2); // a retired id does not count (F9, 2026-11-03)
		CHECK(!Pc::IsAchievementClaimable(D, P, "kills_1"));
		Pc::NewGame(D, P);
		Pc::EvaluateAchievements(D, P);
		CHECK(Pc::ApplyAchievementSave(D, P, {}, {}).empty());
		CHECK(P.AchievementsEarned.empty());
		CHECK(P.AchievementsClaimed.empty());
	}

	// Unknown ids: no reward, never claimable, nothing paid.
	CHECK(Pc::FindAchievement(D, "no_such") == nullptr);
	CHECK(Pc::AchievementReward(D, "no_such") == 0.0);
	{
		Pc::PcProgress P;
		P.AchievementsEarned.insert("no_such");
		CHECK(!Pc::IsAchievementClaimable(D, P, "no_such"));
		CHECK(Pc::ClaimAchievement(D, P, "no_such") == 0.0);
	}
}

#endif
