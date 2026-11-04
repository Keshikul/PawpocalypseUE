#if defined(PAWCORE_STANDALONE_TESTS)

// PcHud JobPicker / PickJob against the REAL JobPicker (job_picker.gd), tools/godot_fixtures/job_picker_fixture.gd: what the
// picker offers (its rows in order, the "(last run)" mark, the row the selection opens on, the second ask's share), then a
// press on an offered row and the run state after it; when the press asks again (Union Rules), the reopened list and a
// second press.

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcHud.h"
#include "PcJobs.h"

#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string PickerDataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	const Pc::PcGameData& PickerGame()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(PickerDataDir(), Data, Errors));
			bLoaded = true;
		}
		return Data;
	}

	std::vector<std::string> Cut(const std::string& S, char Sep)
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

	// The fixture's "offered" cell: id:lastrun;... in row order.
	std::string Offered(const Pc::PcJobPicker& J)
	{
		std::string Out;
		for (const Pc::PcJobPickRow& R : J.Rows)
		{
			Out += (Out.empty() ? "" : ";") + R.JobId + (R.bLastRun ? ":1" : ":0");
		}
		return Out;
	}

	// Godot records the default row's index, or -1 when there is none (the selection then opens on the first row).
	std::string Focus(const Pc::PcJobPicker& J)
	{
		for (const Pc::PcJobPickRow& R : J.Rows)
		{
			if (R.bLastRun)
			{
				return std::to_string(J.Focus);
			}
		}
		CHECK(J.Focus == 0);
		return "-1";
	}

	std::string Lead(const Pc::PcJobPicker& J)
	{
		// The first row's blurb up to its first full stop, read only on the second ask: "At <n>% strength", or the baseline's
		// own ("Wear one and only one. ...") when it is the only row.
		if (!J.bSecond)
		{
			return std::string();
		}
		return J.Rows.size() > 1 ? "At " + std::to_string(J.SharePercent) + "% strength" : std::string("Wear one and only one");
	}

	struct PickerResult
	{
		int Rows = 0, Second = 0, AsksAgain = 0, LastRunBaseline = 0, Unknown = 0, Focused = 0, OnlyBaseline = 0;
	};

	PickerResult RunPickerFixture(const std::string& Path)
	{
		PickerResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		Pc::PcGameData D = PickerGame();
		D.RebuildIndex();
		Pc::PcSkillBranch* Union = nullptr;
		for (Pc::PcSkillBranch& B : D.Branches)
		{
			Union = B.Id == "union_rules" ? &B : Union;
		}
		REQUIRE(Union);
		REQUIRE(Union->LevelEffects.size() == 1);
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = Cut(Line, '\t');
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
			CAPTURE(Line);
			Union->LevelEffects[0] = std::strtod(V.at("union_effect").c_str(), nullptr);
			Pc::PcProgress P;
			for (const std::string& Id : Cut(V.at("jobs_held"), ';'))
			{
				if (!Id.empty())
				{
					P.JobsHeld.insert(Id);
					R.Unknown += D.FindJob(Id) ? 0 : 1;
				}
			}
			if (V.at("union_level") != "0")
			{
				P.BranchLevels["union_rules"] = std::atoll(V.at("union_level").c_str());
			}
			P.RunJob = V.at("run_job");
			P.RunJobSecond = V.at("run_job_second");
			P.CurrentJob = V.at("current_job");
			P.bRunJobChosen = false;
			CHECK((Pc::ShouldAskForJob(P) ? "1" : "0") == V.at("should_ask"));

			const bool bSecond = V.at("second") == "1";
			const Pc::PcJobPicker First = Pc::JobPicker(D, P, bSecond);
			CHECK(Offered(First) == V.at("offered"));
			CHECK(Focus(First) == V.at("focus"));
			CHECK(Lead(First) == V.at("lead"));
			CHECK(First.Rows.back().JobId.empty());
			R.Second += bSecond ? 1 : 0;
			R.LastRunBaseline += First.Rows.back().bLastRun ? 1 : 0;
			R.Focused += V.at("focus") != "-1" ? 1 : 0;
			R.OnlyBaseline += First.Rows.size() == 1 ? 1 : 0;

			const bool bAgain = Pc::PickJob(D, P, bSecond, V.at("press"));
			CHECK((bAgain ? "1" : "0") == V.at("asks_again"));
			CHECK(P.RunJob == V.at("run_after"));
			CHECK(P.RunJobSecond == V.at("second_after"));
			CHECK(P.CurrentJob == V.at("current_after"));
			CHECK((P.bRunJobChosen ? "1" : "0") == V.at("chosen_after"));
			if (bAgain)
			{
				++R.AsksAgain;
				const Pc::PcJobPicker Again = Pc::JobPicker(D, P, true);
				CHECK(Offered(Again) == V.at("offered2"));
				CHECK(Focus(Again) == V.at("focus2"));
				CHECK(Lead(Again) == V.at("lead2"));
				CHECK_FALSE(Pc::PickJob(D, P, true, V.at("press2")));
			}
			else
			{
				CHECK(V.at("offered2") == "-");
			}
			CHECK(P.RunJob == V.at("run_final"));
			CHECK(P.RunJobSecond == V.at("second_final"));
			CHECK(P.CurrentJob == V.at("current_final"));
			CHECK((P.bRunJobChosen ? "1" : "0") == V.at("chosen_final"));
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcHud JobPicker / PickJob match Godot's JobPicker on 1,000 random states")
{
	const PickerResult R = RunPickerFixture(PickerDataDir() + "/../Source/PawCore/Tests/Data/job_picker_godot.tsv");
	CHECK(R.Rows == 1000);
	// Every path: the second ask opened directly and reached by a press, the baseline as last run's choice, a held job
	// with no data, a default row, and a picker offering nothing but the baseline.
	CHECK(R.Second > 200);
	CHECK(R.AsksAgain > 200);
	CHECK(R.LastRunBaseline > 50);
	CHECK(R.Unknown > 50);
	CHECK(R.Focused > 300);
	CHECK(R.OnlyBaseline > 5);
	MESSAGE("second " << R.Second << ", asks again " << R.AsksAgain << ", last-run baseline " << R.LastRunBaseline << ", unknown held "
		<< R.Unknown << ", focused " << R.Focused << ", only the baseline " << R.OnlyBaseline);
}

TEST_CASE("Long: PcHud JobPicker matches all 3,000 Godot picker rows" * doctest::test_suite("long") * doctest::skip())
{
	const PickerResult R = RunPickerFixture(PickerDataDir() + "/../build/job_picker_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Job picker rules from the Godot comments")
{
	const Pc::PcGameData& D = PickerGame();
	std::vector<std::string> Ids;
	for (const Pc::PcJob& J : D.Jobs)
	{
		Ids.push_back(J.Id);
	}
	REQUIRE(Ids.size() >= 2);
	Pc::PcProgress P;
	P.JobsHeld = { Ids[0], Ids[1] };
	P.CurrentJob = Ids[1];

	// "The baseline. Always last, always available"; the previous choice is the default.
	const Pc::PcJobPicker J = Pc::JobPicker(D, P, false);
	REQUIRE(J.Rows.size() == 3);
	CHECK(J.Rows.back().JobId.empty());
	CHECK(J.Rows[static_cast<size_t>(J.Focus)].JobId == Ids[1]);
	// No last-run row (the preference names a job no longer held): the selection opens on the first row.
	Pc::PcProgress Lost = P;
	Lost.CurrentJob = "no_such_job";
	CHECK(Pc::JobPicker(D, Lost, false).Focus == 0);

	// Without Union Rules one answer is the whole question.
	CHECK_FALSE(Pc::PickJob(D, P, false, Ids[0]));
	CHECK(P.bRunJobChosen);
	CHECK(P.RunJob == Ids[0]);
	CHECK(P.CurrentJob == Ids[0]);
	CHECK(P.RunJobSecond.empty());
}

#endif
