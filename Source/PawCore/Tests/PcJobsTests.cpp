#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcJobs.h"

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

	Pc::PcSkillBranch& Branch(Pc::PcGameData& D, const std::string& Id)
	{
		for (Pc::PcSkillBranch& B : D.Branches)
		{
			if (B.Id == Id)
			{
				return B;
			}
		}
		FAIL("no branch " << Id);
		return D.Branches.front();
	}

	Pc::PcJob& Job(Pc::PcGameData& D, const std::string& Id)
	{
		for (Pc::PcJob& J : D.Jobs)
		{
			if (J.Id == Id)
			{
				return J;
			}
		}
		FAIL("no job " << Id);
		return D.Jobs.front();
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

	const char* Flag(bool b)
	{
		return b ? "1" : "0";
	}

	// The fixture's FIELDS, in its order.
	double Pc::PcJob::* const TermFields[] = {
		&Pc::PcJob::PawDamageMultiplier, &Pc::PcJob::AuraDamageMultiplier, &Pc::PcJob::AuraRadiusMultiplier,
		&Pc::PcJob::KittenDamageMultiplier, &Pc::PcJob::PounceDamageMultiplier, &Pc::PcJob::AllDamageMultiplier,
		&Pc::PcJob::CatnipMultiplier, &Pc::PcJob::CritChanceMultiplier, &Pc::PcJob::AttackSpeedMultiplier,
		&Pc::PcJob::CritDamageMultiplier, &Pc::PcJob::EnemyHealthMultiplier, &Pc::PcJob::SpawnRateMultiplier,
	};

	struct JobsResult
	{
		int Rows = 0;
		int Secondary = 0;        // a second job in effect
		int SecondaryCard = 0;    // ... that added a card through the floored share
		int FlooredAway = 0;      // ... whose share of a card was fractional and floored (round would differ at 0.5)
		int Clamped = 0;          // Union Rules owned with a share outside [0, 1]
		int StoredNoUnion = 0;    // a second job stored, Union Rules not owned: must pay nothing
		int Offered = 0, Locked = 0, LockRefused = 0, SecondLocked = 0, SecondRefused = 0, Taken = 0, TakeRefused = 0;
		int Ask = 0, NoAsk = 0;
	};

	JobsResult RunJobsFixture(const std::string& Path)
	{
		JobsResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		// A copy: the fixture overwrites Union Rules' one level IN MEMORY on 40 % of the rows, and the row says with what.
		Pc::PcGameData D = Game();
		D.RebuildIndex();
		Pc::PcSkillBranch& Union = Branch(D, "union_rules");
		REQUIRE(Union.LevelEffects.size() == 1);
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
			Union.LevelEffects[0] = Hex(V.at("union_effect"));
			Pc::PcProgress P;
			P.JobsHeld = Ids(V.at("jobs_held"));
			if (V.at("union_level") != "0")
			{
				P.BranchLevels["union_rules"] = std::atoll(V.at("union_level").c_str());
			}
			P.RunJob = V.at("run_job");
			P.RunJobSecond = V.at("run_job_second");
			P.CurrentJob = V.at("current_job");
			P.bRunJobChosen = V.at("run_job_chosen") == "1";

			std::string Order;
			for (const Pc::PcJob* J : Pc::AllJobs(D))
			{
				Order += (Order.empty() ? "" : ";") + J->Id;
			}
			CHECK(Order == V.at("all_jobs"));
			const std::vector<std::string> Asked = SplitKeep(V.at("asked"), ';');
			const std::vector<std::string> Holds = SplitKeep(V.at("holds"), ';');
			const std::vector<std::string> Available = SplitKeep(V.at("available"), ';');
			REQUIRE(Asked.size() == D.Jobs.size() + 1);
			for (size_t J = 0; J < Asked.size(); ++J)
			{
				CAPTURE(Asked[J]);
				CHECK(Flag(Pc::HoldsJob(P, Asked[J])) == Holds[J]);
				CHECK(Flag(Pc::JobIsAvailable(D, P, Asked[J])) == Available[J]);
			}
			CHECK(Flag(Pc::HasUnionRules(D, P)) == V.at("has_union"));
			CHECK(Pc::UnionRulesShare(D, P) == Hex(V.at("share")));
			const Pc::PcJob* Equipped = Pc::EquippedJob(D, P);
			const Pc::PcJob* Second = Pc::SecondaryJob(D, P);
			CHECK((Equipped ? Equipped->Id : std::string()) == V.at("equipped"));
			CHECK((Second ? Second->Id : std::string()) == V.at("secondary"));
			const std::vector<std::string> Terms = SplitKeep(V.at("terms"), ';');
			REQUIRE(Terms.size() == std::size(TermFields));
			for (size_t F = 0; F < Terms.size(); ++F)
			{
				CAPTURE(F);
				CHECK(Pc::JobTerm(D, P, TermFields[F]) == Hex(Terms[F]));
			}
			CHECK(Pc::JobCritChanceBonus(D, P) == Hex(V.at("crit_bonus")));
			CHECK(std::to_string(Pc::JobExtraCards(D, P)) == V.at("extra_cards"));
			CHECK(Pc::JobPounceLockSec(D, P) == Hex(V.at("pounce_lock")));
			CHECK(Pc::JobHatPath(D, P) == V.at("hat"));
			const std::vector<std::string> JobIds = SplitKeep(V.at("job_ids"), ';');
			const std::vector<std::string> Neutral = SplitKeep(V.at("neutral"), ';');
			const std::vector<std::string> Cost = SplitKeep(V.at("has_cost"), ';');
			REQUIRE(JobIds.size() == D.Jobs.size());
			for (size_t J = 0; J < JobIds.size(); ++J)
			{
				CAPTURE(JobIds[J]);
				REQUIRE(D.FindJob(JobIds[J]) != nullptr);
				CHECK(Flag(Pc::IsJobNeutral(*D.FindJob(JobIds[J]))) == Neutral[J]);
				CHECK(Flag(Pc::JobHasACost(*D.FindJob(JobIds[J]))) == Cost[J]);
			}
			const bool bAsk = Pc::ShouldAskForJob(P);
			CHECK(Flag(bAsk) == V.at("should_ask"));

			R.Secondary += Second ? 1 : 0;
			R.SecondaryCard += (Second && Pc::JobExtraCards(D, P) > (Equipped ? Equipped->ExtraCardsInHand : 0)) ? 1 : 0;
			if (Second && Second->ExtraCardsInHand > 0)
			{
				const double Share = static_cast<double>(Second->ExtraCardsInHand) * Pc::UnionRulesShare(D, P);
				R.FlooredAway += Share != static_cast<double>(static_cast<int64_t>(Share)) ? 1 : 0;
			}
			const double Effect = Union.LevelEffects[0];
			R.Clamped += (Pc::HasUnionRules(D, P) && (Effect > 1.0 || Effect < 0.0)) ? 1 : 0;
			R.StoredNoUnion += (!P.RunJobSecond.empty() && P.RunJobSecond != P.RunJob && D.FindJob(P.RunJobSecond)
				&& !Pc::HasUnionRules(D, P)) ? 1 : 0;
			R.Ask += bAsk ? 1 : 0;
			R.NoAsk += bAsk ? 0 : 1;

			// The four state changes, in the fixture's order.
			const bool bOffered = Pc::OfferJob(D, P, V.at("offer"));
			CHECK(Flag(bOffered) == V.at("offered"));
			CHECK(Join(P.JobsHeld) == V.at("held_after"));
			const bool bLocked = Pc::LockRunJob(P, V.at("lock"));
			CHECK(Flag(bLocked) == V.at("locked"));
			CHECK(P.RunJob == V.at("run_after"));
			CHECK(P.CurrentJob == V.at("current_after"));
			const bool bSecond = Pc::LockRunJobSecond(D, P, V.at("lock_second"));
			CHECK(Flag(bSecond) == V.at("locked_second"));
			CHECK(P.RunJobSecond == V.at("second_after"));
			const bool bTaken = Pc::TakeJob(P, V.at("take"));
			CHECK(Flag(bTaken) == V.at("taken"));
			CHECK(P.CurrentJob == V.at("current_after_take"));

			R.Offered += bOffered ? 1 : 0;
			(bLocked ? R.Locked : R.LockRefused) += 1;
			// "" always succeeds and proves nothing about the refusals; count the real ids only.
			if (!V.at("lock_second").empty())
			{
				(bSecond ? R.SecondLocked : R.SecondRefused) += 1;
			}
			(bTaken ? R.Taken : R.TakeRefused) += 1;
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcJobs matches Godot's GameState job functions on 1,000 random states")
{
	const JobsResult R = RunJobsFixture(DataDir() + "/../Source/PawCore/Tests/Data/jobs_godot.tsv");
	CHECK(R.Rows == 1000);
	// Every path is exercised: the second job paying (and its floored card), the clamp, a stored second job without the
	// node, and each state change both accepted and refused.
	CHECK(R.Secondary > 100);
	CHECK(R.SecondaryCard > 10);
	CHECK(R.FlooredAway > 20);
	CHECK(R.Clamped > 20);
	CHECK(R.StoredNoUnion > 50);
	CHECK(R.Offered > 100);
	CHECK(R.Locked > 100);
	CHECK(R.LockRefused > 50);
	CHECK(R.SecondLocked > 50);
	CHECK(R.SecondRefused > 50);
	CHECK(R.Taken > 100);
	CHECK(R.TakeRefused > 50);
	CHECK(R.Ask > 100);
	CHECK(R.NoAsk > 100);
	MESSAGE("second job " << R.Secondary << " (card " << R.SecondaryCard << ", floored away " << R.FlooredAway << "), clamped " << R.Clamped << ", stored without node "
		<< R.StoredNoUnion << ", offered " << R.Offered << ", locked " << R.Locked << "/" << R.LockRefused << ", second "
		<< R.SecondLocked << "/" << R.SecondRefused << ", taken " << R.Taken << "/" << R.TakeRefused << ", ask " << R.Ask
		<< "/" << R.NoAsk);
}

TEST_CASE("Long: PcJobs matches all 3,000 Godot job rows" * doctest::test_suite("long") * doctest::skip())
{
	const JobsResult R = RunJobsFixture(DataDir() + "/../build/jobs_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Job rules from the Godot comments")
{
	const Pc::PcGameData& Shipped = Game();
	Pc::PcGameData D = Shipped;
	D.RebuildIndex();

	// Every shipping job is a trade, and only "unemployed" gives without taking.
	for (const Pc::PcJob& J : Shipped.Jobs)
	{
		CAPTURE(J.Id);
		CHECK_FALSE(Pc::IsJobNeutral(J));
		CHECK(Pc::JobHasACost(J) == (J.Id != "unemployed"));
	}
	// The predicates on a job edited field by field: each field alone makes a job non-neutral; enemy health is the one
	// whose cost is ABOVE 1.0; crit bonus and extra cards are upsides only.
	{
		Pc::PcJob Blank;
		CHECK(Pc::IsJobNeutral(Blank));
		CHECK_FALSE(Pc::JobHasACost(Blank));
		Blank.PawDamageMultiplier = 1.000001; // within is_equal_approx: still neutral
		CHECK(Pc::IsJobNeutral(Blank));
		for (double Pc::PcJob::* Field : TermFields)
		{
			Pc::PcJob Up;
			Up.*Field = 1.5;
			Pc::PcJob Down;
			Down.*Field = 0.5;
			CHECK_FALSE(Pc::IsJobNeutral(Up));
			CHECK_FALSE(Pc::IsJobNeutral(Down));
			const bool bHealth = Field == &Pc::PcJob::EnemyHealthMultiplier;
			const bool bCritDamage = Field == &Pc::PcJob::CritDamageMultiplier; // Godot's has_a_cost() does not read it
			CHECK(Pc::JobHasACost(Up) == bHealth);
			CHECK(Pc::JobHasACost(Down) == (!bHealth && !bCritDamage));
		}
		Pc::PcJob Bonus;
		Bonus.CritChanceBonus = 0.1;
		Bonus.ExtraCardsInHand = 1;
		CHECK_FALSE(Pc::IsJobNeutral(Bonus));
		CHECK_FALSE(Pc::JobHasACost(Bonus));
		Pc::PcJob Lock;
		Lock.PounceLockSec = 3.0;
		CHECK(Pc::JobHasACost(Lock));
	}

	// The roster: boss drops first by biome id, then the Reawaken jobs by count, ties in data order.
	{
		const std::vector<const Pc::PcJob*> All = Pc::AllJobs(Shipped);
		REQUIRE(All.size() == Shipped.Jobs.size());
		bool bSeenReawaken = false;
		for (size_t I = 0; I < All.size(); ++I)
		{
			const bool bReawaken = All[I]->DroppedByBiome.empty();
			CHECK_FALSE((bSeenReawaken && !bReawaken)); // no boss drop after a Reawaken job
			bSeenReawaken |= bReawaken;
		}
	}

	// Union Rules: a share of 0.5 halves the DISTANCE from 1.0, not the multiplier. x2.5 at half strength is x1.75.
	REQUIRE(D.FindJob("blacksmith") != nullptr);
	REQUIRE(D.FindJob("engineer") != nullptr);
	REQUIRE(D.FindJob("daredevil") != nullptr);
	Pc::PcSkillBranch* Union = nullptr;
	for (Pc::PcSkillBranch& B : D.Branches)
	{
		Union = B.Id == "union_rules" ? &B : Union;
	}
	REQUIRE(Union != nullptr);
	Pc::PcProgress P;
	P.JobsHeld = { "blacksmith", "engineer", "daredevil", "sphinx" };
	REQUIRE(Pc::LockRunJob(P, "engineer"));
	CHECK_FALSE(Pc::LockRunJobSecond(D, P, "blacksmith")); // no Union Rules: refused
	P.RunJobSecond = "blacksmith";                        // a save made with the node, loaded after a respec
	CHECK(Pc::SecondaryJob(D, P) == nullptr);
	CHECK(Pc::JobTerm(D, P, &Pc::PcJob::PawDamageMultiplier) == 1.0);
	P.BranchLevels["union_rules"] = 1;
	Union->LevelEffects = { 0.5 };
	CHECK(Pc::JobTerm(D, P, &Pc::PcJob::PawDamageMultiplier) == doctest::Approx(1.75));
	// Additive and counted terms: the bonus scaled, the cards floored, the lock seconds added.
	Pc::PcGameData Edited = D;
	Job(Edited, "blacksmith").CritChanceBonus = 0.2;
	Job(Edited, "blacksmith").ExtraCardsInHand = 3;
	Job(Edited, "blacksmith").PounceLockSec = 2.0;
	Edited.RebuildIndex();
	CHECK(Pc::JobCritChanceBonus(Edited, P) == doctest::Approx(0.1));
	CHECK(Pc::JobExtraCards(Edited, P) == 1 + 1); // engineer's 1 + floor(3 x 0.5)
	CHECK(Pc::JobPounceLockSec(Edited, P) == doctest::Approx(1.0));
	// The second job wears no hat.
	CHECK(Pc::JobHatPath(D, P) == D.FindJob("engineer")->HatPath);
	// The share clamps to [0, 1]: more than a whole job is not paid, a negative share does not invert the trade.
	Union->LevelEffects = { 3.0 };
	CHECK(Pc::JobTerm(D, P, &Pc::PcJob::PawDamageMultiplier) == doctest::Approx(2.5));
	Union->LevelEffects = { -1.0 };
	CHECK_FALSE(Pc::HasUnionRules(D, P));
	CHECK(Pc::JobTerm(D, P, &Pc::PcJob::PawDamageMultiplier) == 1.0);
	Union->LevelEffects = { 0.5 };
	// The primary cannot also be the second; "" always clears.
	CHECK_FALSE(Pc::LockRunJobSecond(D, P, "engineer"));
	P.RunJobSecond = "engineer";
	CHECK(Pc::SecondaryJob(D, P) == nullptr);
	CHECK(Pc::LockRunJobSecond(D, P, ""));
	CHECK(P.RunJobSecond.empty());

	// The run lock and the preference are separate: take_job changes the preference only; lock_run_job sets both.
	CHECK(Pc::TakeJob(P, "daredevil"));
	CHECK(P.RunJob == "engineer");
	CHECK(Pc::EquippedJob(D, P)->Id == "engineer");
	CHECK_FALSE(Pc::TakeJob(P, "daredevil")); // unchanged
	CHECK_FALSE(Pc::TakeJob(P, "magician"));  // not held
	CHECK(Pc::TakeJob(P, ""));                // a bare neck is a legal preference
	CHECK_FALSE(Pc::LockRunJob(P, "magician"));
	CHECK(P.RunJob == "engineer");

	// A Reawaken asks again but remembers the preference.
	REQUIRE(Pc::LockRunJob(P, "daredevil"));
	REQUIRE(Pc::LockRunJobSecond(D, P, "blacksmith"));
	P.bRunJobChosen = true;
	CHECK_FALSE(Pc::ShouldAskForJob(P));
	Pc::ClearRunJobs(P);
	CHECK(P.RunJob.empty());
	CHECK(P.RunJobSecond.empty());
	CHECK(P.CurrentJob == "daredevil");
	CHECK(Pc::ShouldAskForJob(P));
	CHECK(Pc::JobTerm(D, P, &Pc::PcJob::PounceDamageMultiplier) == 1.0);
	Pc::PcProgress Bare;
	CHECK_FALSE(Pc::ShouldAskForJob(Bare)); // nothing held is not a choice

	// The picker asks for a second job only when another is held.
	CHECK(Pc::OtherJobsThan(P, "daredevil") == 3);
	CHECK(Pc::OtherJobsThan(P, "magician") == 4);

	// offer_job: once, and only for a real job.
	CHECK(Pc::OfferJob(D, P, "magician"));
	CHECK_FALSE(Pc::OfferJob(D, P, "magician"));
	CHECK_FALSE(Pc::OfferJob(D, P, "no_such_job"));
	CHECK_FALSE(Pc::JobIsAvailable(D, P, "no_such_job"));
}

#endif
