#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcPause.h"
#include "PcTitle.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// The pause menu against the REAL pause_menu.tscn (fixture `pause`): every press of every row replayed through PcPause, and
// what Godot showed after it rebuilt in Godot's English. The captions are refreshed where Godot refreshes them (opening the
// Save page, arming an overwrite): between refreshes Godot's are stale, and so are these.

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	std::vector<std::string> Cells(const std::string& S)
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

	std::string Two(int64_t V)
	{
		char Buf[32];
		std::snprintf(Buf, sizeof(Buf), "%02lld", static_cast<long long>(V));
		return Buf;
	}

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

	std::string Mice(int64_t N)
	{
		const Pc::PcKills K = Pc::KillsText(N);
		return K.Number + (K.Form == Pc::PcKillsForm::Millions ? "M mice" : K.Form == Pc::PcKillsForm::Thousands ? "K mice" : " mice");
	}

	const std::string Dash = " \xE2\x80\x94 ";
	const std::string Dot = " \xC2\xB7 ";

	struct FSim
	{
		Pc::PcPauseState S;
		std::set<int64_t> Present;
		std::set<int64_t> Readable;
		std::map<int64_t, std::string> Summary; // the slots this row saved: the game's own line
		int64_t Active = 0;
		std::vector<std::string> Captions = std::vector<std::string>(5);
		std::string Game;

		void Refresh()
		{
			for (int64_t I = 0; I < 5; ++I)
			{
				const Pc::PcPauseSlotLine L = Pc::PauseSlotLine(S, I, Readable.count(I) > 0, Active);
				const std::string N = std::to_string(I + 1);
				const std::string Here = L.bCurrent ? " (current)" : "";
				switch (L.Form)
				{
				case Pc::PcPauseSlotForm::Empty: Captions[static_cast<size_t>(I)] = "Slot " + N + Dash + "empty" + Here; break;
				case Pc::PcPauseSlotForm::Overwrite: Captions[static_cast<size_t>(I)] = "Slot " + N + Here + Dash + "OVERWRITE? press again"; break;
				default:
					Captions[static_cast<size_t>(I)] = "Slot " + N + Here + Dash + "Reawaken " + (Summary.count(I) ? Summary[I] : std::string("?"));
					break;
				}
			}
		}
	};

	struct FResult
	{
		int Rows = 0, Presses = 0, Armed = 0, Saved = 0, Overwrote = 0, QuitArmed = 0, StaleArm = 0;
	};

	FResult RunPauseFixture(const std::string& Path)
	{
		FResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		FSim Sim;
		std::string RowId;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> C = Cells(Line);
			if (Header.empty())
			{
				Header = C;
				continue;
			}
			REQUIRE(C.size() == Header.size());
			std::map<std::string, std::string> V;
			for (size_t I = 0; I < C.size(); ++I)
			{
				V[Header[I]] = C[I];
			}
			const std::string Op = V.at("op");
			CAPTURE(V.at("row"));
			CAPTURE(Op);
			if (V.at("row") != RowId)
			{
				// A new row: the slot files and the active slot as written, the menu opened.
				RowId = V.at("row");
				REQUIRE(Op == "open");
				++R.Rows;
				// One menu for every row, as in the fixture: its captions are the last refresh's, whatever row made them.
				const std::vector<std::string> Stale = Sim.Captions;
				Sim = FSim();
				Sim.Captions = Stale;
				for (const char Ch : V.at("present"))
				{
					Sim.Present.insert(Ch - '0');
					Sim.Readable.insert(Ch - '0'); // a junk file is found out at its first caption, below
				}
				Sim.Active = std::atoll(V.at("active").c_str());
				Sim.Game = V.at("game_reawakens") + Dot + Clock(std::strtod(V.at("game_play").c_str(), nullptr)) + Dot
					+ Mice(std::atoll(V.at("game_kills").c_str()));
				Pc::PauseOpen(Sim.S);
			}
			else
			{
				++R.Presses;
				const int64_t Arg = Op == "slot" ? std::atoll(V.at("arg").c_str()) : -1;
				if (Op == "open") { Pc::PauseOpen(Sim.S); }
				else if (Op == "close") { Pc::PauseClose(Sim.S); }
				else if (Op == "slots") { Pc::PauseShowSlots(Sim.S); Sim.Refresh(); }
				else if (Op == "main")
				{
					R.StaleArm += Sim.S.PendingOverwrite >= 0 ? 1 : 0;
					Pc::PauseShowMain(Sim.S);
				}
				else if (Op == "slot")
				{
					const bool bWasArmed = Sim.S.PendingOverwrite == Arg;
					const Pc::PcPauseAction A = Pc::PausePressSlot(Sim.S, Arg, Sim.Present.count(Arg) > 0);
					if (A == Pc::PcPauseAction::SaveToSlot)
					{
						++R.Saved;
						R.Overwrote += bWasArmed ? 1 : 0;
						Sim.Present.insert(Arg);
						Sim.Readable.insert(Arg);
						Sim.Summary[Arg] = Sim.Game;
						Sim.Active = Arg;
					}
					else
					{
						++R.Armed;
						Sim.Refresh();
					}
				}
				else if (Op == "quit1")
				{
					CHECK(Pc::PausePressQuit(Sim.S) == Pc::PcPauseAction::None);
					++R.QuitArmed;
				}
			}
			// What Godot shows.
			CHECK((V.at("open") == "1") == Sim.S.bOpen);
			CHECK((V.at("paused") == "1") == Sim.S.bOpen);
			CHECK((V.at("main_page") == "1") == !Sim.S.bSlotsPage);
			CHECK((V.at("slot_page") == "1") == Sim.S.bSlotsPage);
			CHECK(std::atoll(V.at("pending").c_str()) == Sim.S.PendingOverwrite);
			CHECK((V.at("exit_pending") == "1") == Sim.S.bExitPending);
			CHECK((V.at("discard") == "1") == Sim.S.bExitPending);
			CHECK(V.at("quit_caption") == (Sim.S.bExitPending ? "Save and quit to menu" : "Back to Main Menu"));
			const std::string N = std::to_string(Sim.S.StatusSlot + 1);
			CHECK(V.at("status") == (Sim.S.Status == Pc::PcPauseStatus::None ? std::string()
				: Sim.S.Status == Pc::PcPauseStatus::Occupied ? "Slot " + N + " already has a game. Press it again to overwrite."
				: "Saved to slot " + N + "."));
			CHECK(std::atoll(V.at("active").c_str()) == Sim.Active);
			std::string Present;
			for (const int64_t P : Sim.Present)
			{
				Present += std::to_string(P);
			}
			CHECK(V.at("present") == Present);
			for (int64_t I = 0; I < 5; ++I)
			{
				CAPTURE(I);
				std::string Want = Sim.Captions[static_cast<size_t>(I)];
				const std::string& Got = V.at("s" + std::to_string(I));
				// A file there that Godot could not read (the fixture's junk) captions as empty: learn it once, then hold to it.
				if ((Want.find(Dash + "Reawaken ") != std::string::npos || Want.find("OVERWRITE") != std::string::npos)
					&& Got.find(Dash + "empty") != std::string::npos && !Sim.Summary.count(I) && Sim.Readable.count(I))
				{
					Sim.Readable.erase(I);
					Sim.Refresh();
					Want = Sim.Captions[static_cast<size_t>(I)];
				}
				// A slot written before the row: its numbers are the file's, so the line is checked up to them.
				if (Want.size() >= 1 && Want.back() == '?')
				{
					Want.pop_back();
					CHECK(Got.compare(0, Want.size(), Want) == 0);
				}
				else
				{
					CHECK(Got == Want);
				}
			}
		}
		return R;
	}
}

TEST_CASE("Pause: every press matches the real pause_menu.tscn")
{
	const FResult R = RunPauseFixture(DataDir() + "/../Source/PawCore/Tests/Data/pause_godot.tsv");
	MESSAGE("rows " << R.Rows << ", presses " << R.Presses << ", armed " << R.Armed << ", saved " << R.Saved << " (overwrites "
		<< R.Overwrote << "), quit armed " << R.QuitArmed << ", main page with an overwrite armed (F34) " << R.StaleArm);
	CHECK(R.Rows == 600);
	CHECK(R.Armed > 200);
	CHECK(R.Saved > 200);
	CHECK(R.Overwrote > 50);
	CHECK(R.QuitArmed > 100);
	CHECK(R.StaleArm > 50);
}

TEST_CASE("Pause: the long suite runs all 2,000 pause rows" * doctest::test_suite("long") * doctest::skip())
{
	const FResult R = RunPauseFixture(DataDir() + "/../build/pause_godot_full.tsv");
	CHECK(R.Rows == 2000);
}

TEST_CASE("Pause: the quit button's second press saves and leaves; the discard leaves; closing disarms")
{
	Pc::PcPauseState S;
	Pc::PauseOpen(S);
	CHECK(Pc::PausePressQuit(S) == Pc::PcPauseAction::None);
	CHECK(S.bExitPending);
	CHECK(Pc::PausePressQuit(S) == Pc::PcPauseAction::SaveAndLeave);
	CHECK(Pc::PausePressDiscard(S) == Pc::PcPauseAction::Leave);
	Pc::PauseClose(S);
	CHECK_FALSE(S.bExitPending);
	Pc::PauseOpen(S);
	CHECK(Pc::PausePressQuit(S) == Pc::PcPauseAction::None); // armed again from scratch, never a single-press quit
	CHECK(Pc::PauseSlotFocus(7, 5) == 4);
	CHECK(Pc::PauseSlotFocus(-1, 5) == 0);
}

#endif
