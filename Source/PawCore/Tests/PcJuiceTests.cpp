#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcJuice.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// The UI's motion against Godot (fixture `juice`): Tween.interpolate_value at every easing UiJuice uses (and LINEAR) x every
// ease, and the REAL ui_juice.gd on a real Button, 160 random sequences of its events and frames, the jitter's leans replayed
// from the recorded draws. Godot computes in float32 (real_t), PawCore in doubles: compared to a tolerance far below what an
// eye or a changed constant would make (a BACK of 1.7 instead of 1.70158 is off by 2e-4).

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

	std::vector<std::map<std::string, std::string>> Read(const std::string& Name)
	{
		std::string Text;
		REQUIRE(Pc::ReadTextFile(DataDir() + "/../Source/PawCore/Tests/Data/" + Name, Text));
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

	Pc::PcTrans Trans(const std::string& S)
	{
		if (S == "sine") return Pc::PcTrans::Sine;
		if (S == "quad") return Pc::PcTrans::Quad;
		if (S == "back") return Pc::PcTrans::Back;
		if (S == "elastic") return Pc::PcTrans::Elastic;
		REQUIRE(S == "linear");
		return Pc::PcTrans::Linear;
	}

	Pc::PcEase Ease(const std::string& S)
	{
		if (S == "in") return Pc::PcEase::In;
		if (S == "out") return Pc::PcEase::Out;
		if (S == "in_out") return Pc::PcEase::InOut;
		REQUIRE(S == "out_in");
		return Pc::PcEase::OutIn;
	}
}

TEST_CASE("Juice: Godot's Tween easing, every transition x ease UiJuice uses")
{
	const auto Rows = Read("juice_curves_godot.tsv");
	REQUIRE(Rows.size() == 8840);
	std::set<std::string> Seen;
	double Worst = 0.0;
	int64_t Off = 0;
	for (const auto& R : Rows)
	{
		Seen.insert(R.at("trans") + "/" + R.at("ease"));
		const double Got = Pc::TweenInterpolate(Hex(R.at("initial")), Hex(R.at("delta")), Hex(R.at("elapsed")), Hex(R.at("duration")),
			Trans(R.at("trans")), Ease(R.at("ease")));
		const double Err = std::fabs(Got - Hex(R.at("value")));
		Worst = Err > Worst ? Err : Worst;
		if (Err > 2e-6 && Off++ < 5)
		{
			MESSAGE(R.at("trans") << "/" << R.at("ease") << " at " << Hex(R.at("elapsed")) << ": " << Got << " vs Godot " << Hex(R.at("value")));
		}
	}
	MESSAGE("worst curve difference from Godot's float32: " << Worst);
	CHECK(Off == 0);
	CHECK(Seen.size() == 20); // 5 transitions x 4 eases
}

TEST_CASE("Juice: the real ui_juice.gd, 160 sequences of events and frames")
{
	const auto Rows = Read("juice_seq_godot.tsv");
	const auto LeanRows = Read("juice_leans_godot.tsv");
	REQUIRE(Rows.size() == 9600);
	std::map<int64_t, std::vector<double>> Leans;
	for (const auto& R : LeanRows)
	{
		Leans[std::stoll(R.at("seq"))].push_back(Hex(R.at("lean_deg")));
	}
	std::map<std::string, int64_t> Ops;
	int64_t Off = 0;
	double Worst = 0.0;
	int64_t Moving = 0;
	int64_t Seq = -1;
	Pc::PcJuice J;
	bool bDisabled = false, bVisible = true, bEffects = true;
	for (const auto& R : Rows)
	{
		const int64_t S = std::stoll(R.at("seq"));
		if (S != Seq)
		{
			Seq = S;
			J = Pc::PcJuice();
			J.RecordedLeansDeg = Leans.count(S) ? Leans[S] : std::vector<double>();
			bDisabled = false;
			bVisible = true;
			bEffects = true;
		}
		const std::string& Op = R.at("op");
		++Ops[Op];
		if (Op == "enter") Pc::JuiceHoverEnter(J, bDisabled);
		else if (Op == "exit") Pc::JuiceHoverExit(J);
		else if (Op == "down") Pc::JuicePressDown(J);
		else if (Op == "up") Pc::JuicePressUp(J, false); // headless: the Button is never hovered (no pointer)
		else if (Op == "punch") Pc::JuicePunch(J);
		else if (Op == "pop_in") Pc::JuicePopIn(J);
		else if (Op == "jitter_on") Pc::JuiceSetJitter(J, true, bEffects, bVisible);
		else if (Op == "jitter_off") Pc::JuiceSetJitter(J, false, bEffects, bVisible);
		else if (Op == "disable") bDisabled = true;
		else if (Op == "enable") bDisabled = false;
		else if (Op == "hide") bVisible = false;
		else if (Op == "show") bVisible = true;
		else if (Op == "effects_off") bEffects = false;
		else if (Op == "effects_on") bEffects = true;
		else
		{
			REQUIRE(Op == "step");
			Pc::JuiceStep(J, Hex(R.at("dt")), bVisible);
		}
		const double Want[3] = { Hex(R.at("scale_x")), Hex(R.at("scale_y")), Hex(R.at("rotation")) };
		const double Got[3] = { J.ScaleX, J.ScaleY, J.Rotation };
		bool bOff = (R.at("jitter") == "1") != J.bJitter;
		for (int I = 0; I < 3; ++I)
		{
			const double Err = std::fabs(Got[I] - Want[I]);
			Worst = Err > Worst ? Err : Worst;
			bOff = bOff || Err > 1e-5;
		}
		Moving += (Want[0] != 1.0 || Want[1] != 1.0 || Want[2] != 0.0) ? 1 : 0;
		if (bOff && Off++ < 8)
		{
			MESSAGE("seq " << S << " line " << R.at("line") << " " << Op << ": scale " << Got[0] << "," << Got[1] << " rot " << Got[2]
				<< " jitter " << J.bJitter << " vs Godot " << Want[0] << "," << Want[1] << " rot " << Want[2] << " jitter " << R.at("jitter"));
		}
	}
	MESSAGE("worst sequence difference from Godot's float32: " << Worst);
	CHECK(Off == 0);
	// Coverage: every event, and most lines with something moving (a fixture of still Buttons would prove nothing).
	CHECK(Ops.size() == 15);
	for (const auto& [Op, N] : Ops)
	{
		CHECK_MESSAGE(N >= 150, Op);
	}
	CHECK(Moving > 4000);
	CHECK(LeanRows.size() >= 250);
}

TEST_CASE("Juice: the release grows back to the hover size while still hovered")
{
	// The fixture cannot hover headless (is_hovered() is the pointer's); _on_release's other half by its own words.
	Pc::PcJuice J;
	Pc::JuiceHoverEnter(J, false);
	Pc::JuiceStep(J, 1.0, true);
	CHECK(J.ScaleX == Pc::JuiceHoverScale);
	Pc::JuicePressDown(J);
	Pc::JuiceStep(J, 1.0, true);
	CHECK(J.ScaleX == Pc::JuicePressScale);
	Pc::JuicePressUp(J, true);
	Pc::JuiceStep(J, 0.05, true);
	CHECK(J.ScaleX > Pc::JuicePressScale);
	Pc::JuiceStep(J, 1.0, true);
	CHECK(J.ScaleX == Pc::JuiceHoverScale);
	CHECK(J.ScaleY == Pc::JuiceHoverScale);
	Pc::JuicePressUp(J, false);
	Pc::JuiceStep(J, 1.0, true);
	CHECK(J.ScaleX == 1.0);
	CHECK(Pc::JuiceAtRest(J));
	// The game's leans (no recording): inside +-JITTER_DEG, and not all the same.
	Pc::PcJuice K;
	K.Seed = 7;
	Pc::JuiceSetJitter(K, true, true, true);
	std::set<double> Lands;
	for (int I = 0; I < 40; ++I)
	{
		Pc::JuiceStep(K, 0.2, true);
		CHECK(std::fabs(K.Rotation) <= Pc::JuiceJitterDeg * 3.14159265358979323846 / 180.0 + 1e-12);
		Lands.insert(K.Rotation);
	}
	CHECK(Lands.size() > 30);
	CHECK(!Pc::JuiceAtRest(K));
}

#endif
