#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcTree.h"

#include <algorithm>
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

	double Hex(const std::string& S)
	{
		return std::strtod(S.c_str(), nullptr);
	}

	struct TreeResult
	{
		int Rows = 0;
		int Bought = 0;
		int JobsOffered = 0;
		int AutoSwitches = 0;
	};

	TreeResult RunTreeFixture(const std::string& Path)
	{
		TreeResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		const Pc::PcGameData& D = Game();
		std::vector<std::string> JobIds;
		for (const Pc::PcJob& J : D.Jobs)
		{
			JobIds.push_back(J.Id);
		}
		std::sort(JobIds.begin(), JobIds.end());
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
			P.BranchLevels = Pairs(V.at("branches"));
			P.ReawakenCount = std::atoll(V.at("reawaken_count").c_str());
			P.BossesDefeated = Ids(V.at("bosses"));
			P.Stardust = Hex(V.at("stardust"));
			P.JobsHeld = Ids(V.at("jobs_held"));
			CAPTURE(R.Rows);

			// Every branch, in Godot's BRANCH_PATHS order (the fixture names it, so the CSV order is not assumed here).
			const std::vector<std::string> Branches = SplitKeep(V.at("ids"), ';');
			REQUIRE(Branches.size() == D.Branches.size());
			const std::vector<std::string> Available = SplitKeep(V.at("available"), ';');
			const std::vector<std::string> Costs = SplitKeep(V.at("next_cost"), ';');
			const std::vector<std::string> Maxed = SplitKeep(V.at("maxed"), ';');
			const std::vector<std::string> Rivals = SplitKeep(V.at("rival"), ';');
			const std::vector<std::string> Gates = SplitKeep(V.at("gate"), ';');
			const std::vector<std::string> Affordable = SplitKeep(V.at("affordable"), ';');
			for (size_t B = 0; B < Branches.size(); ++B)
			{
				const std::string& Id = Branches[B];
				CAPTURE(Id);
				CHECK((Pc::IsBranchAvailable(D, P, Id) ? "1" : "0") == Available[B]);
				CHECK(Pc::NextBranchCost(D, P, Id) == Hex(Costs[B]));
				CHECK((Pc::IsBranchMaxLevel(D, P, Id) ? "1" : "0") == Maxed[B]);
				CHECK(Pc::BlockingRival(D, P, Id) == Rivals[B]);
				CHECK(std::to_string(Pc::BranchReawakenGate(D, Id)) == Gates[B]);
				CHECK((Pc::CanAffordNextBranch(D, P, Id) ? "1" : "0") == Affordable[B]);
			}
			const std::vector<std::string> JobBranches = SplitKeep(V.at("job_branches"), ';');
			REQUIRE(JobBranches.size() == JobIds.size());
			for (size_t J = 0; J < JobIds.size(); ++J)
			{
				CHECK_MESSAGE(Pc::BranchForJob(D, JobIds[J]) == JobBranches[J], "branch_for_job " << JobIds[J]);
			}
			std::string Trunks;
			for (const std::string& T : Pc::TrunkBranches(D))
			{
				Trunks += (Trunks.empty() ? "" : ";") + T;
			}
			CHECK(Trunks == V.at("trunks"));
			CHECK(std::to_string(Pc::MaxVitalityBonus(D)) == V.at("vitality_max"));

			const std::string Probe = V.at("probe");
			const std::set<std::string> JobsBefore = P.JobsHeld;
			const Pc::PcBranchPurchase Buy = Pc::PurchaseNextBranchLevel(D, P, Probe);
			CHECK((Buy.bBought ? "1" : "0") == V.at("bought"));
			CHECK(P.Stardust == Hex(V.at("stardust_after")));
			CHECK(std::to_string(Pc::BranchLevel(P, Probe)) == V.at("level_after"));
			CHECK(Join(P.JobsHeld) == V.at("jobs_after"));
			CHECK((Buy.bAutoTravelOn ? "1" : "0") == V.at("auto_travel"));
			CHECK((Buy.bAutoCardOn ? "1" : "0") == V.at("auto_card"));
			CHECK(Buy.JobOffered.empty() == (P.JobsHeld == JobsBefore));

			const std::string GrantId = V.at("grant_id");
			Pc::GrantBranchLevel(D, P, GrantId, std::atoll(V.at("grant_level").c_str()));
			CHECK(std::to_string(Pc::BranchLevel(P, GrantId)) == V.at("grant_after"));

			R.Bought += Buy.bBought ? 1 : 0;
			R.JobsOffered += Buy.JobOffered.empty() ? 0 : 1;
			R.AutoSwitches += (Buy.bAutoTravelOn || Buy.bAutoCardOn) ? 1 : 0;
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcTree matches Godot's SkillTreeManager on 1,000 random states")
{
	const TreeResult R = RunTreeFixture(DataDir() + "/../Source/PawCore/Tests/Data/tree_godot.tsv");
	CHECK(R.Rows == 1000);
	// Every purchase path is exercised: plain levels, job nodes, the automation switches.
	CHECK(R.Bought > 200);
	CHECK(R.JobsOffered > 30);
	CHECK(R.AutoSwitches > 10);
	MESSAGE("bought " << R.Bought << ", jobs handed over " << R.JobsOffered << ", automation switched on " << R.AutoSwitches);
}

TEST_CASE("Long: PcTree matches all 3,000 Godot SkillTreeManager rows" * doctest::test_suite("long") * doctest::skip())
{
	const TreeResult R = RunTreeFixture(DataDir() + "/../build/tree_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Tree rules from the Godot comments")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P;
	// An unknown id: the tree "sells" it (Godot's predicate falls through), but at +infinity nobody can pay.
	CHECK(Pc::IsBranchAvailable(D, P, "no_such_branch"));
	P.Stardust = 1e300;
	CHECK_FALSE(Pc::CanAffordNextBranch(D, P, "no_such_branch"));
	CHECK_FALSE(Pc::PurchaseNextBranchLevel(D, P, "no_such_branch").bBought);
	CHECK(P.Stardust == 1e300);

	// A maxed branch is never bought again, however rich the wallet.
	const Pc::PcSkillBranch& Any = D.Branches.front();
	P.BranchLevels[Any.Id] = Pc::BranchMaxLevel(Any);
	CHECK(Pc::NextBranchCost(D, P, Any.Id) == -1.0);
	CHECK_FALSE(Pc::PurchaseNextBranchLevel(D, P, Any.Id).bBought);

	// A free grant never lowers a level and never passes the ladder.
	Pc::GrantBranchLevel(D, P, Any.Id, 0);
	CHECK(Pc::BranchLevel(P, Any.Id) == Pc::BranchMaxLevel(Any));
	Pc::GrantBranchLevel(D, P, Any.Id, 999);
	CHECK(Pc::BranchLevel(P, Any.Id) == Pc::BranchMaxLevel(Any));

	// The fork: owning a rival closes the door, and the tooltip can name it.
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		if (B.ExclusiveWith.empty())
		{
			continue;
		}
		Pc::PcProgress Q;
		Q.ReawakenCount = 99;
		Q.BranchLevels[B.Parent] = 1;
		for (const Pc::PcSkillBranch& Boss : D.Branches)
		{
			Q.BossesDefeated.insert(Boss.GatedByBoss);
		}
		CHECK_MESSAGE(Pc::IsBranchAvailable(D, Q, B.Id), B.Id);
		Q.BranchLevels[B.ExclusiveWith.front()] = 1;
		CHECK_MESSAGE(!Pc::IsBranchAvailable(D, Q, B.Id), B.Id);
		CHECK(Pc::BlockingRival(D, Q, B.Id) == B.ExclusiveWith.front());
	}

	// The three ability unlocks read their own Reawaken constants, not the branch's required_reawakens. The shipping data
	// has every one of these at 0, so the fixture cannot tell them apart: a copy with distinct numbers does.
	{
		Pc::PcGameData Gated = D;
		Gated.Reawaken.PounceUnlockReawakenCount = 1;
		Gated.Reawaken.OverkillUnlockReawakenCount = 2;
		Gated.Reawaken.UnicornUnlockReawakenCount = 3;
		for (Pc::PcSkillBranch& B : Gated.Branches)
		{
			B.RequiredReawakens = 7;
		}
		Gated.RebuildIndex();
		const std::pair<const char*, int64_t> Unlocks[] = { { "unlock_pounce", 1 }, { "unlock_overkill", 2 }, { "unlock_unicorn", 3 } };
		for (const auto& [Id, Gate] : Unlocks)
		{
			CAPTURE(Id);
			REQUIRE(Gated.FindBranch(Id) != nullptr);
			CHECK(Pc::BranchReawakenGate(Gated, Id) == Gate);
			Pc::PcProgress Q;
			Q.BranchLevels[Gated.FindBranch(Id)->Parent] = 1;
			Q.BossesDefeated.insert(Gated.FindBranch(Id)->GatedByBoss);
			Q.ReawakenCount = Gate - 1;
			CHECK_FALSE(Pc::IsBranchAvailable(Gated, Q, Id));
			Q.ReawakenCount = Gate;
			CHECK(Pc::IsBranchAvailable(Gated, Q, Id));
		}
		CHECK(Pc::BranchReawakenGate(Gated, "vitality") == 7);
	}

	// A job node hands its job over once.
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		if (B.JobId.empty())
		{
			continue;
		}
		Pc::PcProgress Q;
		Q.ReawakenCount = 99;
		Q.Stardust = 1e300;
		Q.BranchLevels[B.Parent] = 1;
		Q.BossesDefeated.insert(B.GatedByBoss);
		const Pc::PcBranchPurchase First = Pc::PurchaseNextBranchLevel(D, Q, B.Id);
		REQUIRE_MESSAGE(First.bBought, B.Id);
		CHECK(First.JobOffered == B.JobId);
		CHECK(Q.JobsHeld.count(B.JobId) == 1);
		CHECK(Pc::BranchForJob(D, B.JobId) == B.Id);
	}
}

#endif
