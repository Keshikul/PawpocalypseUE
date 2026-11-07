#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcHud.h"

#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The dock's held keys against the REAL dock_navigator.gd in the real HUD (fixture `dock_repeat`): 600 sequences, each from a
// chosen row of the Upgrades page with nothing held, then 40 frames of keys held and let go with chosen frame lengths; after
// every frame the presses made, the selected row and the navigator's own counters are compared. The rows wrap; the shop rows
// repeat a held press, the others never.

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	std::vector<std::string> Split(const std::string& S, char Sep)
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

	std::vector<std::map<std::string, std::string>> Read(const std::string& Path)
	{
		std::string Text;
		REQUIRE(Pc::ReadTextFile(Path, Text));
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		std::vector<std::map<std::string, std::string>> Rows;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> C = Split(Line, '\t');
			if (Header.empty())
			{
				Header = C;
				continue;
			}
			REQUIRE(C.size() == Header.size());
			std::map<std::string, std::string> Row;
			for (size_t I = 0; I < C.size(); ++I)
			{
				Row[Header[I]] = C[I];
			}
			Rows.push_back(Row);
		}
		return Rows;
	}

	double Hex(const std::string& S) { return std::strtod(S.c_str(), nullptr); }
}

TEST_CASE("DockRepeat: every held frame matches the real dock_navigator.gd")
{
	const auto RowsTable = Read(DataDir() + "/../Source/PawCore/Tests/Data/dock_repeat_rows_godot.tsv");
	std::vector<bool> Repeats;
	for (const auto& R : RowsTable)
	{
		Repeats.push_back(R.at("repeats") == "1");
	}
	REQUIRE(Repeats.size() > 2);
	const auto Frames = Read(DataDir() + "/../Source/PawCore/Tests/Data/dock_repeat_godot.tsv");
	Pc::PcDockRepeat R;
	int64_t Row = 0;
	bool bPressWas = false;
	int Firsts = 0, Repeated = 0, Steps = 0, RepeatSteps = 0, NoRepeatRow = 0, Disabled = 0, Restored = 0, Shown = 0;
	for (const auto& F : Frames)
	{
		INFO("seq " << F.at("seq") << " step " << F.at("step"));
		if (F.at("step") == "start")
		{
			R = Pc::PcDockRepeat();
			Row = std::stoll(F.at("row"));
			bPressWas = false;
			continue;
		}
		const bool bPress = F.at("press") == "1";
		const double Dt = Hex(F.at("dt"));
		const int64_t Dir = Pc::DockTickNav(R, F.at("up") == "1", F.at("down") == "1", Dt);
		const int64_t N = static_cast<int64_t>(Repeats.size());
		if (Dir != 0 && F.at("sel_nav") == "1")
		{
			const bool bFirst = R.NavHeld == 0.0;
			Row = ((Row + Dir) % N + N) % N; // step_row wraps
			++Steps;
			RepeatSteps += bFirst ? 0 : 1;
		}
		else if (Dir != 0)
		{
			Row = std::stoll(F.at("row")); // nothing selected: step_row selects the remembered row (the HUD's memory)
			++Restored;
		}
		const bool bSelection = F.at("sel_press") == "1";
		const Pc::PcDockPress P = Pc::DockTickPress(R, bPress && !bPressWas, bPress, bSelection, Repeats[static_cast<size_t>(Row)], Dt);
		if (P == Pc::PcDockPress::ShowSelection)
		{
			Row = std::stoll(F.at("row")); // restore_selection(): the remembered row
			++Shown;
		}
		bPressWas = bPress;
		// press() does nothing on a disabled row (the HUD's: a locked button takes no press).
		const int Pressed = P == Pc::PcDockPress::Press && F.at("disabled") == "0" ? 1 : 0;
		Disabled += P == Pc::PcDockPress::Press && F.at("disabled") == "1" ? 1 : 0;
		CHECK(std::to_string(Pressed) == F.at("presses"));
		CHECK(std::to_string(Row) == F.at("row"));
		CHECK(R.PressHeld == Hex(F.at("press_held")));
		CHECK(R.PressNext == Hex(F.at("press_next")));
		CHECK(std::to_string(R.NavDir) == F.at("nav_dir"));
		CHECK(R.NavHeld == Hex(F.at("nav_held")));
		CHECK(R.NavNext == Hex(F.at("nav_next")));
		if (Pressed)
		{
			(R.PressHeld == 0.0 ? Firsts : Repeated) += 1;
		}
		NoRepeatRow += bPress && !Repeats[static_cast<size_t>(Row)] && R.PressHeld == 0.0 && Pressed == 0 ? 1 : 0;
	}
	MESSAGE("frames " << Frames.size() << ", first presses " << Firsts << ", repeated presses " << Repeated << ", row steps " << Steps
		<< " (held repeats " << RepeatSteps << "), a press held on a row that does not repeat " << NoRepeatRow << ", on a disabled row " << Disabled << "; nothing selected: a step selected " << Restored << ", a press showed the selection " << Shown);
	CHECK(Firsts > 100);
	CHECK(Repeated > 20);
	CHECK(RepeatSteps > 20);
	CHECK(NoRepeatRow > 20);
	CHECK(Shown > 20);
	CHECK(Restored > 5);
}

#endif
