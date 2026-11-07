#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcHud.h"
#include "PcStats.h"
#include "PcTitle.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The title screen against the REAL main_menu.tscn (fixture `title`): the rules here are values; Godot's English is rebuilt
// from them with Godot's own templates (main_menu.gd, pause_menu.gd, victory_screen.gd), so a wrong number, form or
// bucket shows as a different line. The game module's words come from ST_UI and are checked against the same rows by
// Pawpocalypse.UI.Title.*.

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	std::vector<std::string> SplitTabs(const std::string& S)
	{
		std::vector<std::string> Out;
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find('\t', Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	// The fixture writes a line break as "\n" and a backslash as "\\".
	std::string Unescape(const std::string& S)
	{
		std::string Out;
		for (size_t I = 0; I < S.size(); ++I)
		{
			if (S[I] == '\\' && I + 1 < S.size())
			{
				const char C = S[++I];
				Out += C == 'n' ? '\n' : C == 't' ? '\t' : C;
			}
			else
			{
				Out += S[I];
			}
		}
		return Out;
	}

	double Hex(const std::string& S) { return std::strtod(S.c_str(), nullptr); }
	int64_t Int(const std::string& S) { return std::atoll(S.c_str()); }

	std::string Two(int64_t V)
	{
		char Buf[32];
		std::snprintf(Buf, sizeof(Buf), "%02lld", static_cast<long long>(V));
		return Buf;
	}

	// PauseMenu.format_duration, in English.
	std::string Clock(double Seconds)
	{
		const Pc::PcClock C = Pc::SlotClock(Seconds);
		switch (C.Form)
		{
		case Pc::PcClockForm::HoursMinutes: return std::to_string(C.H) + "h " + Two(C.M) + "m";
		case Pc::PcClockForm::MinutesSeconds: return std::to_string(C.M) + "m " + Two(C.S) + "s";
		default: return std::to_string(C.S) + "s";
		}
	}

	std::string Kills(int64_t N)
	{
		const Pc::PcKills K = Pc::KillsText(N);
		return K.Number + (K.Form == Pc::PcKillsForm::Millions ? "M mice" : K.Form == Pc::PcKillsForm::Thousands ? "K mice" : " mice");
	}

	std::string When(double Unix, double Now)
	{
		const Pc::PcWhen W = Pc::WhenSaved(Unix, Now);
		switch (W.Form)
		{
		case Pc::PcWhenForm::Unknown: return "unknown";
		case Pc::PcWhenForm::JustNow: return "just now";
		case Pc::PcWhenForm::Minutes: return std::to_string(W.N) + " min ago";
		case Pc::PcWhenForm::Hours: return std::to_string(W.N) + " hr ago";
		case Pc::PcWhenForm::Days: return std::to_string(W.N) + " days ago";
		default:
		{
			char Buf[32];
			std::snprintf(Buf, sizeof(Buf), "%04lld-%02lld-%02lld", static_cast<long long>(W.Year),
				static_cast<long long>(W.Month), static_cast<long long>(W.Day));
			return Buf;
		}
		}
	}

	// VictoryScreen.format_duration (the leaderboard's time), in English.
	std::string LongDuration(double Seconds)
	{
		const Pc::PcDuration T = Pc::DurationParts(Seconds);
		return T.bHours ? std::to_string(T.H) + "h " + Two(T.M) + "m " + Two(T.S) + "s" : std::to_string(T.M) + "m " + Two(T.S) + "s";
	}

	struct TitleResult
	{
		int Rows = 0;
		int Found = 0, NotFound = 0, ContinueOnProgress = 0;
		int Filled = 0, Unreadable = 0;
		std::map<int, int> WhenForms, KillForms, ClockForms, ClearForms;
	};

	TitleResult RunTitleFixture(const std::string& Path)
	{
		TitleResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		const std::string Para = "Runs will be ranked by time to the final boss \xE2\x80\x94 the clock stops the moment the last boss falls, "
			"and only a first clear counts. A rematch is not a faster run.\n\nThe board is not connected yet, so nothing here leaves "
			"your machine. Your time is already being recorded against that day.\n\n";
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = SplitTabs(Line);
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
			++R.Rows;
			CAPTURE(R.Rows);
			Pc::PcProgress P;
			P.LifetimeCatnipEver = Hex(V.at("catnip_ever"));
			P.ReawakenCount = Int(V.at("reawakens"));
			P.LifetimeKills = Int(V.at("kills"));
			P.FinalBossSeconds = Hex(V.at("final_seconds"));
			P.bFinalBossDefeated = V.at("final_defeated") == "1";
			const bool bOnDisk = !V.at("has_slots").empty();

			// The menu.
			const Pc::PcTitleMenu M = Pc::TitleMenu(bOnDisk, P);
			CHECK((V.at("continue_disabled") == "1") == !M.bContinue);
			CHECK((V.at("load_disabled") == "1") == !M.bLoad);
			const std::string Status = M.bSaveFound
				? "Save found \xE2\x80\x94 " + std::to_string(M.Reawakens) + (M.Reawakens == 1 ? " Reawaken, " : " Reawakens, ")
					+ std::to_string(M.Kills) + " mice caught."
				: "No save yet. Start a new game.";
			CHECK(Unescape(V.at("status")) == Status);
			CHECK(V.at("menu_focus") == (M.bContinue ? "ContinueButton" : "NewGameButton"));
			R.Found += M.bSaveFound ? 1 : 0;
			R.NotFound += M.bSaveFound ? 0 : 1;
			R.ContinueOnProgress += (M.bContinue && !bOnDisk) ? 1 : 0;

			// The slot screen opens on the last slot used.
			CHECK(Pc::SlotScreenFocus(Int(V.at("slot_focus_last")), 5) == Int(V.at("slot_focus")));

			// The leaderboard.
			const Pc::PcClearForm Clear = Pc::LeaderboardClear(P);
			++R.ClearForms[static_cast<int>(Clear)];
			const std::string Last = Clear == Pc::PcClearForm::Time ? "Your clear:  " + LongDuration(P.FinalBossSeconds)
				: Clear == Pc::PcClearForm::BeforeTheClock
				? "You cleared the final boss before the run clock existed, so there is no time to show for it."
				: "You have not cleared the final boss yet.";
			CHECK(Unescape(V.at("leaderboard")) == Para + Last);

			// The five cards: an empty or unreadable slot reads Empty and offers no Delete.
			for (int S = 0; S < 5; ++S)
			{
				CAPTURE(S);
				const std::string Pre = "s" + std::to_string(S) + "_";
				const bool bFilled = Int(V.at(Pre + "reawakens")) >= 0;
				CHECK((V.at(Pre + "delete") == "1") == bFilled);
				std::string Want = "Empty\n\nStart a new game here.";
				if (bFilled)
				{
					++R.Filled;
					const double Unix = Hex(V.at(Pre + "unix"));
					const double Now = Hex(V.at(Pre + "now"));
					++R.WhenForms[static_cast<int>(Pc::WhenSaved(Unix, Now).Form)];
					Want = "Reawaken " + V.at(Pre + "reawakens") + "\n" + Clock(Hex(V.at(Pre + "play"))) + " played\n"
						+ Kills(Int(V.at(Pre + "kills"))) + "\n\nLast saved\n" + When(Unix, Now);
				}
				else if (V.at("has_slots").find(static_cast<char>('0' + S)) != std::string::npos)
				{
					++R.Unreadable;
				}
				CHECK(Unescape(V.at(Pre + "info")) == Want);
			}

			// The formatters on their own.
			const double WUnix = Hex(V.at("when_unix"));
			CHECK(Unescape(V.at("when")) == When(WUnix, Hex(V.at("when_now"))));
			++R.WhenForms[static_cast<int>(Pc::WhenSaved(WUnix, Hex(V.at("when_now"))).Form)];
			CHECK(Unescape(V.at("dur")) == Clock(Hex(V.at("dur_in"))));
			++R.ClockForms[static_cast<int>(Pc::SlotClock(Hex(V.at("dur_in"))).Form)];
			CHECK(Unescape(V.at("kills_out")) == Kills(Int(V.at("kills_in"))));
			++R.KillForms[static_cast<int>(Pc::KillsText(Int(V.at("kills_in"))).Form)];
		}
		return R;
	}
}

TEST_CASE("Title: the menu, the slot cards, the leaderboard and the formatters match the real main_menu.tscn")
{
	const TitleResult R = RunTitleFixture(DataDir() + "/../Source/PawCore/Tests/Data/title_godot.tsv");
	MESSAGE("rows " << R.Rows << ", save found " << R.Found << ", none " << R.NotFound << ", continue on memory alone "
		<< R.ContinueOnProgress << ", cards filled " << R.Filled << ", unreadable " << R.Unreadable);
	CHECK(R.Rows == 1000);
	// Coverage: every form appears (a fixture that never takes a branch has not tested it).
	CHECK(R.Found > 100);
	CHECK(R.NotFound > 5);
	CHECK(R.Unreadable > 20);
	for (int F = 0; F <= static_cast<int>(Pc::PcWhenForm::Date); ++F)
	{
		CAPTURE(F);
		CHECK(R.WhenForms.count(F) == 1);
	}
	for (int F = 0; F <= static_cast<int>(Pc::PcKillsForm::Plain); ++F)
	{
		CHECK(R.KillForms.count(F) == 1);
	}
	for (int F = 0; F <= static_cast<int>(Pc::PcClockForm::Seconds); ++F)
	{
		CHECK(R.ClockForms.count(F) == 1);
	}
	for (int F = 0; F <= static_cast<int>(Pc::PcClearForm::NotYet); ++F)
	{
		CHECK(R.ClearForms.count(F) == 1);
	}
}

TEST_CASE("Title: the long suite runs all 3,000 title rows" * doctest::test_suite("long") * doctest::skip())
{
	const TitleResult R = RunTitleFixture(DataDir() + "/../build/title_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Title: the date past a week is the stamp's UTC day, across eras and leap days")
{
	auto Date = [](double Unix)
	{
		const Pc::PcWhen W = Pc::WhenSaved(Unix, Unix + 30.0 * 86400.0);
		REQUIRE(W.Form == Pc::PcWhenForm::Date);
		return W.Year * 10000 + W.Month * 100 + W.Day;
	};
	CHECK(Date(1.0) == 19700101);
	CHECK(Date(86399.9) == 19700101);
	CHECK(Date(86400.0) == 19700102);
	CHECK(Date(951782400.0) == 20000229);  // 2000 is a leap year
	CHECK(Date(951868800.0) == 20000301);
	CHECK(Date(4107542400.0) == 21000301); // 2100 is not
	CHECK(Date(1798761599.0) == 20261231);
	CHECK(Date(1798761600.0) == 20270101);
	CHECK(Pc::WhenSaved(0.0, 100.0).Form == Pc::PcWhenForm::Unknown);
	CHECK(Pc::WhenSaved(-5.0, 100.0).Form == Pc::PcWhenForm::Unknown);
	CHECK(Pc::WhenSaved(200.0, 100.0).Form == Pc::PcWhenForm::JustNow); // a stamp from the future
}

#endif
