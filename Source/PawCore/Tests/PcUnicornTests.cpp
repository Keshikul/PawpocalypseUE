#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcTween.h"
#include "PcUnicorn.h"
#include "PcGodotLook.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The Unicorn ceremony against Godot (fixture `unicorn`: the real unicorn_ceremony.gd, its draw calls, random draws and
// sounds renamed to recorders, its tweens stepped by custom_step after its _process, presses fed to its _input). Godot's
// Vector2s and tween equations are float32, PawCore's doubles: compared to a tolerance far below a pixel (a hop 0.21 of the
// side high instead of 0.22 is 6 px off; a rear of -15 degrees 0.017 rad).

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
		if (S.empty())
		{
			return Out;
		}
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
			if (!Line.empty() && Line.back() == '\r')
			{
				Line.pop_back();
			}
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			std::vector<std::string> C = Split(Line, '\t');
			if (Header.empty())
			{
				Header = C;
				continue;
			}
			while (C.size() < Header.size())
			{
				C.push_back("");
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

	std::vector<double> Numbers(const std::string& S, char Sep)
	{
		std::vector<double> Out;
		for (const std::string& P : Split(S, Sep))
		{
			Out.push_back(Hex(P));
		}
		return Out;
	}

	struct Worst
	{
		double Value = 0.0;
		void See(double A, double B) { Value = std::max(Value, std::fabs(A - B)); }
	};

	Pc::PcUnicornInput InputOf(const std::string& S)
	{
		if (S == "key") return Pc::PcUnicornInput::KeyDown;
		if (S == "echo") return Pc::PcUnicornInput::KeyEcho;
		if (S == "mouse") return Pc::PcUnicornInput::MouseDown;
		if (S == "pad") return Pc::PcUnicornInput::PadDown;
		if (S == "release") return Pc::PcUnicornInput::KeyUp;
		return Pc::PcUnicornInput::Other;
	}
}

TEST_CASE("Tween: Godot's sequencing (steps, parallel, delays, callbacks, the leftover)")
{
	double A = 0.0, B = 5.0;
	std::vector<int> Called;
	Pc::PcTweenHost H;
	H.Get = [&](int Id) { return Id == 0 ? A : B; };
	H.Set = [&](int Id, double V) { (Id == 0 ? A : B) = V; };
	H.Call = [&](int Id) { Called.push_back(Id); };
	Pc::PcTween T;
	T.Property(0, 1.0, 0.5);
	T.Callback(7);
	T.Property(1, 15.0, 1.0).Delay = 0.5; // starts from where B is when its DELAY ends, not when its step starts
	T.Parallel();
	T.Interval(0.1);
	T.Step(0.25, H);
	CHECK(A == doctest::Approx(0.5));
	T.Step(0.5, H);   // A lands at 0.5 s; the callback fires; 0.25 s of leftover runs into the next step (half its delay)
	CHECK(A == 1.0);
	CHECK(Called == std::vector<int>{ 7 });
	CHECK(B == 5.0);
	B = 7.0;          // moved by something else during the delay
	T.Step(0.5, H);   // the delay ends; a quarter of B's second, from 7
	CHECK(B == doctest::Approx(9.0));
	T.Step(0.5, H);
	CHECK(B == doctest::Approx(13.0));
	CHECK(T.IsValid());
	T.Step(0.6, H);
	CHECK(B == 15.0);
	CHECK_FALSE(T.IsValid());
}

TEST_CASE("Unicorn: the real unicorn_ceremony.gd, 16 runs frame by frame, the fiesta replayed from Godot's draws")
{
	const auto Rows = Read("unicorn_godot.tsv");
	REQUIRE(Rows.size() > 400);
	Pc::PcUnicornCeremony C;
	int Run = -1;
	Worst Pos, Tw, Col, Bit;
	int Skipped = 0, Ignored = 0, Fiestas = 0, ArcRows = 0, Finished = 0, Shapes = 0;
	std::vector<Pc::PcUnicornArc> Arcs;
	std::vector<Pc::PcUnicornShape> Mine;
	for (const auto& R : Rows)
	{
		const int Rn = std::atoi(R.at("run").c_str());
		const double W = std::atof(R.at("w").c_str()), H = std::atof(R.at("h").c_str());
		if (Rn != Run)
		{
			Run = Rn;
			C = Pc::PcUnicornCeremony();
			C.bReplay = true;
			C.Start(W, H);
		}
		CAPTURE(Run);
		CAPTURE(R.at("frame"));
		const std::vector<double> Draws = Numbers(R.at("draws"), ';');
		C.Recorded.insert(C.Recorded.end(), Draws.begin(), Draws.end());
		if (R.at("press") != "-")
		{
			const bool bWasDone = C.bDone;
			const bool bSkip = C.Input(InputOf(R.at("press")));
			Skipped += bSkip;
			Ignored += !bSkip && !bWasDone;
		}
		const uint64_t Before = C.Draws;
		C.Step(Hex(R.at("dt")));
		REQUIRE(C.Draws - Before == Draws.size());
		Fiestas += Draws.size() > 100;
		Tw.See(C.Clock, Hex(R.at("clock")));
		Tw.See(C.DimA, Hex(R.at("dim_a")));
		Pos.See(C.X, Hex(R.at("x")));
		Pos.See(C.Y, Hex(R.at("y")));
		Tw.See(C.Rot, Hex(R.at("rot")));
		Tw.See(C.Scale, Hex(R.at("scale")));
		Tw.See(C.RootA, Hex(R.at("root_a")));
		Tw.See(C.Grow, Hex(R.at("grow")));
		REQUIRE((C.bDone ? "1" : "0") == R.at("done"));
		REQUIRE((C.bFinished ? "1" : "0") == R.at("finished"));
		Finished += C.bFinished;
		// The sounds of this frame (the first row also carries the first whinny, made when it started).
		std::string Sounds;
		for (const Pc::PcUnicornSound S : C.Sounds)
		{
			Sounds += std::string(Sounds.empty() ? "" : ";") + (S == Pc::PcUnicornSound::Neigh ? "unicorn_neigh" : "boss_defeated");
		}
		C.Sounds.clear();
		REQUIRE(Sounds == R.at("sounds"));

		// Every 23rd bit, and what it draws.
		const std::vector<std::string> GBits = Split(R.at("bits"), ';');
		REQUIRE(GBits.size() == (C.Bits.size() + 22) / 23);
		const std::vector<std::string> GShapes = Split(R.at("shapes"), ';');
		size_t ShapeAt = 0;
		for (size_t K = 0; K < GBits.size(); ++K)
		{
			const Pc::PcUnicornBit& B = C.Bits[K * 23];
			const std::vector<double> G = Numbers(GBits[K], ',');
			Bit.See(B.X, G[0]);
			Bit.See(B.Y, G[1]);
			Bit.See(B.Vx, G[2]);
			Bit.See(B.Vy, G[3]);
			Tw.See(B.Rot, G[4]);
			Tw.See(B.Spin, G[5]);
			Tw.See(B.Sx, G[6]);
			Tw.See(B.Sy, G[7]);
			REQUIRE((B.bStar ? 1.0 : 0.0) == G[11]);
			REQUIRE((B.bRain ? 1.0 : 0.0) == G[12]);
			Pc::UnicornBitShapes(B, Mine);
			if (!B.bStar)
			{
				const Pc::PcColor Cc = Pc::UnicornRainbow(B.Colour);
				Col.See(Cc.R, G[8]);
				Col.See(Cc.G, G[9]);
				Col.See(Cc.B, G[10]);
			}
			for (const Pc::PcUnicornShape& S : Mine)
			{
				REQUIRE(ShapeAt < GShapes.size());
				const std::vector<std::string> P = Split(GShapes[ShapeAt++], ':');
				REQUIRE(P[0] == (S.bRim ? "polyline" : "polygon"));
				// Godot's palette colour, as the project's palette has it now (the restyle, PcGodotLook.h): the fixture
				// still proves which colour each shape is drawn in.
				double ER = Hex(P[1]), EG = Hex(P[2]), EB = Hex(P[3]);
				Pc::PcFromGodot(ER, EG, EB);
				Col.See(S.Color.R, ER);
				Col.See(S.Color.G, EG);
				Col.See(S.Color.B, EB);
				Col.See(S.Color.A, Hex(P[4]));
				if (S.bRim)
				{
					CHECK(Hex(P[5]) == Pc::Unicorn::StarRimPx);
				}
				REQUIRE(P.size() == 6 + 2 * S.Xs.size());
				for (size_t I = 0; I < S.Xs.size(); ++I)
				{
					Bit.See(S.Xs[I], Hex(P[6 + 2 * I]));
					Bit.See(S.Ys[I], Hex(P[7 + 2 * I]));
				}
				++Shapes;
			}
		}
		REQUIRE(ShapeAt == GShapes.size());

		Pc::UnicornArcs(C, Arcs);
		const std::vector<std::string> GArcs = Split(R.at("arcs"), ';');
		REQUIRE(Arcs.size() == GArcs.size());
		for (size_t I = 0; I < Arcs.size(); ++I)
		{
			const std::vector<double> G = Numbers(GArcs[I], ',');
			Pos.See(Arcs[I].Cx, G[0]);
			Pos.See(Arcs[I].Cy, G[1]);
			Pos.See(Arcs[I].Radius, G[2]);
			Tw.See(Arcs[I].From, G[3]);
			Tw.See(Arcs[I].To, G[4]);
			CHECK(G[5] == Pc::Unicorn::ArcPoints);
			Col.See(Arcs[I].Color.R, G[6]);
			Col.See(Arcs[I].Color.G, G[7]);
			Col.See(Arcs[I].Color.B, G[8]);
			Pos.See(Arcs[I].Width, G[9]);
			CHECK(G[10] == 1.0); // antialiased
		}
		ArcRows += !Arcs.empty();
	}
	MESSAGE("unicorn: worst position " << Pos.Value << ", tween / clock " << Tw.Value << ", bit " << Bit.Value << ", colour "
		<< Col.Value << "; " << Skipped << " skips, " << Ignored << " presses ignored, " << Fiestas << " fiestas, " << ArcRows
		<< " frames with the rainbow, " << Shapes << " shapes, " << Finished << " finished frames");
	CHECK(Skipped >= 5);
	CHECK(Ignored >= 3);   // too early, an echo, a release
	CHECK(Fiestas == 16);
	CHECK(Finished > 0);
	CHECK(Pos.Value < 2e-3);
	CHECK(Tw.Value < 2e-5);
	CHECK(Bit.Value < 5e-3);
	CHECK(Col.Value < 1e-6);
}

#endif
