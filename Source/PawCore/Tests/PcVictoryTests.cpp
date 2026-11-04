#if defined(PAWCORE_STANDALONE_TESTS)

// The victory screen's rules against the REAL HUD (hud.tscn's VictoryScreen and GameState), tools/godot_fixtures/
// victory_fixture.gd: VictoryScreen.format_duration() on every row's seconds; then GameState.mark_boss_defeated() on a
// random ledger, whether boss_defeated was emitted, whether the ending came up (PcProgress RaisesVictory), and the line it
// shows ("Cleared in " + the first clear's time).

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcHud.h"
#include "PcProgress.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string VictoryDataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	const Pc::PcGameData& VictoryGame()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(VictoryDataDir(), Data, Errors));
			bLoaded = true;
		}
		return Data;
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

	// The English strings ("{h}h {m}m {s}s" / "{m}m {s}s", "%02d"), as the UI fills them from the string table.
	std::string English(const Pc::PcDuration& T)
	{
		char Buf[96];
		if (T.bHours)
		{
			std::snprintf(Buf, sizeof(Buf), "%lldh %02lldm %02llds", static_cast<long long>(T.H), static_cast<long long>(T.M),
				static_cast<long long>(T.S));
		}
		else
		{
			std::snprintf(Buf, sizeof(Buf), "%lldm %02llds", static_cast<long long>(T.M), static_cast<long long>(T.S));
		}
		return Buf;
	}

	struct VictoryResult
	{
		int Rows = 0, Hours = 0, Negative = 0, Shown = 0, Rematch = 0, Silent = 0, OtherBoss = 0, Stamped = 0;
	};

	VictoryResult RunVictoryFixture(const std::string& Path)
	{
		VictoryResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		const Pc::PcGameData& D = VictoryGame();
		REQUIRE_FALSE(D.Code.BossRealms.empty());
		const std::string Last = D.Code.BossRealms.back();
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = Split(Line, '\t');
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
			const double Seconds = std::strtod(V.at("seconds").c_str(), nullptr);
			const Pc::PcDuration T = Pc::DurationParts(Seconds);
			CHECK(English(T) == V.at("duration"));
			R.Hours += T.bHours ? 1 : 0;
			R.Negative += Seconds < 0.0 ? 1 : 0;

			const std::string Tier = V.at("tier");
			Pc::PcProgress P;
			if (V.at("defeated_before") == "1")
			{
				P.BossesDefeated.insert(Tier);
			}
			if (V.at("cleared_before") == "1")
			{
				P.BossesClearedThisCycle.insert(Tier);
			}
			P.FinalBossSeconds = std::strtod(V.at("final_before").c_str(), nullptr);
			P.TotalPlaySeconds = std::strtod(V.at("play").c_str(), nullptr);
			const Pc::PcBossMark Mark = Pc::MarkBossDefeated(D, P, Tier);
			CHECK((Mark.bDefeated ? "1" : "0") == V.at("signalled"));
			CHECK((Mark.bNew ? "1" : "0") == V.at("new"));
			CHECK(P.FinalBossSeconds == std::strtod(V.at("final_after").c_str(), nullptr));
			const bool bShown = Pc::RaisesVictory(D, Tier, Mark);
			CHECK((bShown ? "1" : "0") == V.at("shown"));
			if (bShown)
			{
				CHECK("Cleared in " + English(Pc::DurationParts(P.FinalBossSeconds)) == V.at("time_text"));
				// The ending holds the world while it is up, and OK gives back exactly what it found.
				CHECK(V.at("paused_up") == "1");
				++R.Shown;
				R.Rematch += Mark.bNew ? 0 : 1;
				R.Stamped += Mark.bNew && V.at("final_before") != V.at("final_after") ? 1 : 0; // the clock stopped here
			}
			else
			{
				CHECK(V.at("time_text") == "-");
				CHECK(V.at("paused_up") == V.at("paused_before"));
				R.Silent += Tier == Last ? 1 : 0;
				R.OtherBoss += Tier != Last && Mark.bDefeated ? 1 : 0;
			}
			CHECK(V.at("paused_after") == V.at("paused_before"));
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcHud DurationParts / PcProgress RaisesVictory match Godot's VictoryScreen on 1,000 random states")
{
	const VictoryResult R = RunVictoryFixture(VictoryDataDir() + "/../Source/PawCore/Tests/Data/victory_godot.tsv");
	CHECK(R.Rows == 1000);
	// Every path: times with and without hours, a negative clock, the ending from a first clear and from a first rematch of
	// a cycle, a last-boss kill that raises nothing (already cleared this cycle), another boss's clear.
	CHECK(R.Hours > 200);
	CHECK(R.Negative > 20);
	CHECK(R.Shown > 100);
	CHECK(R.Rematch > 20);
	CHECK(R.Silent > 20);
	CHECK(R.OtherBoss > 100);
	CHECK(R.Stamped > 20);
	MESSAGE("hours " << R.Hours << ", negative " << R.Negative << ", shown " << R.Shown << ", from a rematch " << R.Rematch
		<< ", silent last-boss kills " << R.Silent << ", other bosses " << R.OtherBoss << ", first clears stamped " << R.Stamped);
}

TEST_CASE("Long: the victory rules match all 3,000 Godot rows" * doctest::test_suite("long") * doctest::skip())
{
	const VictoryResult R = RunVictoryFixture(VictoryDataDir() + "/../build/victory_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Victory rules from the Godot comments")
{
	const Pc::PcGameData& D = VictoryGame();
	// ci/check_hud.gd: "a run time formats as 2h 25m 30s", "a short time formats as 1m 35s".
	CHECK(English(Pc::DurationParts(8730.0)) == "2h 25m 30s");
	CHECK(English(Pc::DurationParts(95.0)) == "1m 35s");
	CHECK(English(Pc::DurationParts(4200.0)) == "1h 10m 00s");
	// Exactly an hour has hours; a second short has none; fractions drop; nothing is negative.
	CHECK(Pc::DurationParts(3600.0).bHours);
	CHECK_FALSE(Pc::DurationParts(3599.999).bHours);
	CHECK(English(Pc::DurationParts(59.999)) == "0m 59s");
	CHECK(English(Pc::DurationParts(-30.0)) == "0m 00s");
	CHECK(English(Pc::DurationParts(std::numeric_limits<double>::quiet_NaN())) == "0m 00s");
	// Hours never roll into days.
	CHECK(English(Pc::DurationParts(90061.0)) == "25h 01m 01s");

	// It fires on the FINAL boss and on no other; and only when boss_defeated is emitted.
	const std::string Last = D.Code.BossRealms.back();
	Pc::PcBossMark Emitted;
	Emitted.bDefeated = true;
	CHECK(Pc::RaisesVictory(D, Last, Emitted));
	CHECK_FALSE(Pc::RaisesVictory(D, D.Code.BossRealms.front(), Emitted));
	CHECK_FALSE(Pc::RaisesVictory(D, Last, Pc::PcBossMark()));
	// A rematch shows the time that counted: the first clear's, not the clock now.
	Pc::PcProgress P;
	P.TotalPlaySeconds = 4200.0;
	CHECK(Pc::MarkBossDefeated(D, P, Last).bNew);
	CHECK(P.FinalBossSeconds == 4200.0);
	P.BossesClearedThisCycle.clear();
	P.TotalPlaySeconds = 9000.0;
	const Pc::PcBossMark Again = Pc::MarkBossDefeated(D, P, Last);
	CHECK(Pc::RaisesVictory(D, Last, Again));
	CHECK(P.FinalBossSeconds == 4200.0);
}

#endif
