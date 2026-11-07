#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcBackdrop.h"
#include "PcCsv.h"
#include "PcRainbow.h"
#include "PcGodotLook.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The rainbow bar, the rainbow frame and the skill tree's sky against Godot (fixture `rainbow`: the real rainbow_bar.gd,
// rainbow_frame.gd and skill_tree_sky.gd compiled from their own source with only their draw calls renamed to recorders).
// The bar and the frame: Godot's float32 Vector2 / Color against PawCore's doubles, to a tolerance far below a pixel or a
// colour step (a HUE_SPAN of 0.74 instead of 0.75 moves the last band's hue by 0.0098). The sky: PcBackdrop reproduces
// Godot's float32 and its RNG, so its layout is compared EXACTLY and its frames to the last float bit.

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
			while (C.size() < Header.size())
			{
				C.push_back(""); // a trailing empty cell
			}
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

	double StepFor(int Seq) { return Seq % 5 == 4 ? 0.25 : Pc::GodotRangeStep; }
}

TEST_CASE("Rainbow: Godot's Range.set_value, Color.from_hsv and fposmod")
{
	CHECK(Pc::GodotRangeValue(0.6295, 0.0, 1.0, 0.01) == doctest::Approx(0.63).epsilon(1e-12));
	CHECK(Pc::GodotRangeValue(1.3, 0.0, 1.0, 0.01) == 1.0);
	CHECK(Pc::GodotRangeValue(-0.2, 0.0, 1.0, 0.01) == 0.0);
	CHECK(Pc::GodotRangeValue(2.1, 1.0, 9.0, 0.25) == doctest::Approx(2.0));
	CHECK(Pc::GodotFposmod(-0.25, 1.0) == doctest::Approx(0.75));
	CHECK(Pc::GodotFposmod(2.5, 1.0) == doctest::Approx(0.5));
	const Pc::PcColor Red = Pc::GodotFromHsv(0.0, 1.0, 1.0);
	CHECK(Red.R == 1.0);
	CHECK(Red.G == 0.0);
	CHECK(Red.B == 0.0);
	const Pc::PcColor Grey = Pc::GodotFromHsv(0.4, 0.0, 0.5, 0.3);
	CHECK(Grey.G == 0.5);
	CHECK(Grey.A == 0.3);
	// _fill_fraction(): clamped, and 0 for an empty range (a Range never hands it more, but the script does not rely on that).
	CHECK(Pc::RainbowBarFraction(1.3, 0.0, 1.0) == 1.0);
	CHECK(Pc::RainbowBarFraction(-0.5, 0.0, 1.0) == 0.0);
	CHECK(Pc::RainbowBarFraction(3.0, 2.0, 6.0) == doctest::Approx(0.25));
	CHECK(Pc::RainbowBarFraction(0.5, 1.0, 1.0) == 0.0);
}

TEST_CASE("Rainbow bar: the real rainbow_bar.gd, 30 bars x 20 frames, the sparks replayed from Godot's draws")
{
	const auto Rows = Read("rainbow_bar_godot.tsv");
	REQUIRE(Rows.size() == 600);
	Pc::PcRainbowBar Bar;
	int Seq = -1;
	Worst Pos, Col, Clock;
	int Bands = 0, Dots = 0, Spawned = 0, Full = 0, Empty = 0, Capped = 0;
	for (const auto& R : Rows)
	{
		const int S = std::atoi(R.at("seq").c_str());
		if (S != Seq)
		{
			Seq = S;
			Bar = Pc::PcRainbowBar();
			Bar.bReplay = true;
		}
		CAPTURE(Seq);
		CAPTURE(R.at("frame"));
		const double W = Hex(R.at("w")), H = Hex(R.at("h")), Min = Hex(R.at("min")), Max = Hex(R.at("max"));
		// The value the Range kept: snapped to its step, held in the range.
		const double Value = Pc::GodotRangeValue(Hex(R.at("asked")), Min, Max, StepFor(Seq));
		REQUIRE(Value == doctest::Approx(Hex(R.at("value"))).epsilon(1e-12));
		const double Fraction = Pc::RainbowBarFraction(Value, Min, Max);
		Full += Fraction >= 1.0;
		Empty += Fraction <= 0.0;
		const std::vector<double> Draws = Numbers(R.at("draws"), ';');
		Bar.Recorded.insert(Bar.Recorded.end(), Draws.begin(), Draws.end());
		const size_t Before = Bar.Draws;
		Capped += Fraction >= 1.0 && Bar.Sparks.size() > static_cast<size_t>(Pc::RainbowBar::SparkFullCap);
		Bar.Step(Hex(R.at("dt")), W, H, Fraction);
		REQUIRE(Bar.Draws - Before == Draws.size()); // the same number of sparks spawned, so the same draws taken
		Spawned += static_cast<int>(Draws.size() / 5);
		Clock.See(Bar.Time, Hex(R.at("time")));
		REQUIRE(Bar.Accumulator == doctest::Approx(Hex(R.at("acc"))).epsilon(1e-9));

		const std::vector<std::string> Sparks = Split(R.at("sparks"), ';');
		REQUIRE(Bar.Sparks.size() == Sparks.size());
		for (size_t I = 0; I < Sparks.size(); ++I)
		{
			const std::vector<double> G = Numbers(Sparks[I], ',');
			const Pc::PcRainbowSpark& P = Bar.Sparks[I];
			Pos.See(P.X, G[0]);
			Pos.See(P.Y, G[1]);
			Pos.See(P.Vx, G[2]);
			Pos.See(P.Vy, G[3]);
			Clock.See(P.Age, G[4]);
			Col.See(P.Hue, G[5]);
		}

		std::vector<Pc::PcRainbowBand> MyBands;
		std::vector<Pc::PcRainbowDot> MyDots;
		Bar.Draw(W, H, Fraction, MyBands, MyDots);
		const std::vector<std::string> GBands = Split(R.at("bands"), ';');
		REQUIRE(MyBands.size() == GBands.size());
		for (size_t I = 0; I < GBands.size(); ++I)
		{
			const std::vector<double> G = Numbers(GBands[I], ',');
			Pos.See(MyBands[I].X, G[0]);
			Pos.See(MyBands[I].W, G[1]);
			Col.See(MyBands[I].Color.R, G[2]);
			Col.See(MyBands[I].Color.G, G[3]);
			Col.See(MyBands[I].Color.B, G[4]);
			CHECK(MyBands[I].Color.A == 1.0);
		}
		Bands += static_cast<int>(GBands.size());
		const std::vector<std::string> GDots = Split(R.at("circles"), ';');
		REQUIRE(MyDots.size() == GDots.size());
		for (size_t I = 0; I < GDots.size(); ++I)
		{
			const std::vector<double> G = Numbers(GDots[I], ',');
			Pos.See(MyDots[I].X, G[0]);
			Pos.See(MyDots[I].Y, G[1]);
			Pos.See(MyDots[I].Radius, G[2]);
			Col.See(MyDots[I].Color.R, G[3]);
			Col.See(MyDots[I].Color.G, G[4]);
			Col.See(MyDots[I].Color.B, G[5]);
			Col.See(MyDots[I].Color.A, G[6]);
		}
		Dots += static_cast<int>(GDots.size());
	}
	MESSAGE("rainbow bar: worst position " << Pos.Value << ", colour " << Col.Value << ", clock " << Clock.Value << "; "
		<< Bands << " bands, " << Dots << " spark circles, " << Spawned << " sparks spawned, " << Capped << " full bars at the cap");
	// Coverage: the fixture saw bands, sparks, a full bar and an empty one.
	CHECK(Bands > 1000);
	CHECK(Dots > 500);
	CHECK(Full > 20);
	CHECK(Empty > 20);
	CHECK(Capped > 3); // a full bar holding more than 24 sparks throws no more
	CHECK(Pos.Value < 2e-3);
	CHECK(Col.Value < 1e-5);
	CHECK(Clock.Value < 1e-6);
}

TEST_CASE("Rainbow frame: the real rainbow_frame.gd, 40 frames x 10 steps; following a plate")
{
	const auto Rows = Read("rainbow_frame_godot.tsv");
	REQUIRE(Rows.size() == 400);
	int Seq = -1;
	double Phase = 0.0;
	Worst Pos, Col, Ph;
	int Drawn = 0, Idle = 0;
	std::vector<double> Xs, Ys;
	std::vector<Pc::PcColor> Colors;
	for (const auto& R : Rows)
	{
		const int S = std::atoi(R.at("seq").c_str());
		if (S != Seq)
		{
			Seq = S;
			Phase = 0.0;
		}
		CAPTURE(Seq);
		CAPTURE(R.at("frame"));
		const bool bActive = R.at("active") == "1";
		Phase = Pc::RainbowFramePhase(Phase, Hex(R.at("dt")), Hex(R.at("turns")), bActive);
		Ph.See(Phase, Hex(R.at("phase")));
		if (R.at("drawn") == "0")
		{
			CHECK_FALSE(bActive); // an active frame always draws; an idle one never
			++Idle;
			continue;
		}
		REQUIRE(bActive);
		++Drawn;
		CHECK(Hex(R.at("line_width")) == Hex(R.at("width")));
		CHECK(R.at("antialiased") == "1");
		Pc::RainbowFrameOutline(Hex(R.at("w")), Hex(R.at("h")), Hex(R.at("corner")), Phase, Xs, Ys, Colors);
		const std::vector<std::string> Points = Split(R.at("points"), ';');
		REQUIRE(Xs.size() == Points.size());
		REQUIRE(Points.size() == 37);
		for (size_t I = 0; I < Points.size(); ++I)
		{
			const std::vector<double> G = Numbers(Points[I], ':');
			Pos.See(Xs[I], G[0]);
			Pos.See(Ys[I], G[1]);
			Col.See(Colors[I].R, G[2]);
			Col.See(Colors[I].G, G[3]);
			Col.See(Colors[I].B, G[4]);
		}
	}
	MESSAGE("rainbow frame: worst point " << Pos.Value << ", colour " << Col.Value << ", phase " << Ph.Value << "; " << Drawn
		<< " drawn, " << Idle << " idle");
	CHECK(Drawn > 200);
	CHECK(Idle > 20);
	CHECK(Pos.Value < 1e-3);
	CHECK(Col.Value < 1e-5);
	CHECK(Ph.Value < 1e-6);

	// The follower: the plate's rect grown by the margin along the plate's own axes, leaning with it; shown only while the
	// plate is shown and the frame active; its hue turning only then.
	const auto Follows = Read("rainbow_follow_godot.tsv");
	REQUIRE(Follows.size() == 200);
	Worst At, Turn;
	int Leaning = 0, Hidden = 0;
	for (const auto& R : Follows)
	{
		CAPTURE(R.at("seq"));
		const double Rot = Hex(R.at("plate_rot"));
		const double M = Hex(R.at("margin"));
		const Pc::PcRect F = Pc::RainbowFrameAround(Hex(R.at("plate_w")), Hex(R.at("plate_h")), M);
		// The frame's corner in global space: the plate's origin plus its local corner turned by the plate's lean.
		const double Gx = Hex(R.at("plate_ox")) + F.X * std::cos(Rot) - F.Y * std::sin(Rot);
		const double Gy = Hex(R.at("plate_oy")) + F.X * std::sin(Rot) + F.Y * std::cos(Rot);
		At.See(Gx, Hex(R.at("x")));
		At.See(Gy, Hex(R.at("y")));
		At.See(F.W, Hex(R.at("w")));
		At.See(F.H, Hex(R.at("h")));
		At.See(Rot, Hex(R.at("rot")));
		const bool bShown = R.at("plate_shown") == "1" && R.at("active") == "1";
		CHECK((R.at("visible") == "1") == bShown);
		Turn.See(Pc::RainbowFramePhase(0.0, Hex(R.at("dt")), Pc::RainbowFrame::TurnsPerSec, bShown), Hex(R.at("phase")));
		Leaning += Rot != 0.0;
		Hidden += !bShown;
	}
	MESSAGE("rainbow follow: worst " << At.Value << ", phase " << Turn.Value << "; " << Leaning << " leaning, " << Hidden << " hidden");
	CHECK(Leaning > 50);
	CHECK(Hidden > 20);
	CHECK(At.Value < 1e-3);
	CHECK(Turn.Value < 1e-6);
}

TEST_CASE("Skill tree sky: the real skill_tree_sky.gd at 4 sizes x 4 budgets, laid out and drawn for 10 frames each")
{
	const auto Layouts = Read("sky_layout_godot.tsv");
	const auto Frames = Read("sky_frames_godot.tsv");
	REQUIRE(Layouts.size() == 16);
	REQUIRE(Frames.size() == 160);
	CHECK(std::to_string(Pc::GodotHashString("skill_tree_sky")) == Layouts[0].at("seed"));
	Worst Pos, Col;
	int Wraps = 0, Polys = 0, StarsDrawn = 0;
	size_t F = 0;
	for (const auto& L : Layouts)
	{
		const double W = std::atof(L.at("w").c_str()), H = std::atof(L.at("h").c_str()), Budget = Hex(L.at("budget"));
		CAPTURE(W);
		CAPTURE(H);
		CAPTURE(Budget);
		Pc::PcSky Sky = Pc::BuildSky(W, H, Budget);
		const std::vector<std::string> Clouds = Split(L.at("clouds"), ';');
		const std::vector<std::string> Stars = Split(L.at("stars"), ';');
		REQUIRE(Sky.Clouds.size() == Clouds.size());
		REQUIRE(Sky.Stars.size() == Stars.size());
		for (size_t I = 0; I < Clouds.size(); ++I)
		{
			const std::vector<double> G = Numbers(Clouds[I], ',');
			const Pc::PcSkyCloud& C = Sky.Clouds[I];
			CHECK(C.AtX == G[0]);
			CHECK(C.AtY == G[1]);
			CHECK(C.Radius == G[2]);
			CHECK(C.DriftX == G[3]);
			CHECK(C.DriftY == G[4]);
			CHECK(C.HuePeriod == G[5]);
			CHECK(C.Phase == G[6]);
			CHECK(C.SlowAmp == G[7]);
			CHECK(C.FastAmp == G[8]);
		}
		for (size_t I = 0; I < Stars.size(); ++I)
		{
			const std::vector<double> G = Numbers(Stars[I], ',');
			const Pc::PcSkyStar& S = Sky.Stars[I];
			CHECK(S.AtX == G[0]);
			CHECK(S.AtY == G[1]);
			CHECK(S.Radius == G[2]);
			CHECK(S.Alpha == G[3]);
			CHECK(S.Period == G[4]);
			CHECK(S.Phase == G[5]);
		}
		std::vector<double> Xs, Ys;
		for (int K = 0; K < 10; ++K, ++F)
		{
			const auto& R = Frames[F];
			REQUIRE(R.at("w") == L.at("w"));
			REQUIRE(R.at("budget") == L.at("budget"));
			CAPTURE(K);
			std::vector<double> Was;
			for (const Pc::PcSkyCloud& C : Sky.Clouds)
			{
				Was.push_back(C.AtX);
			}
			Sky.Step(Hex(R.at("dt")));
			CHECK(Sky.Time == Hex(R.at("time")));
			const std::vector<std::string> Ats = Split(R.at("at"), ';');
			REQUIRE(Ats.size() == Sky.Clouds.size());
			for (size_t I = 0; I < Ats.size(); ++I)
			{
				const std::vector<double> G = Numbers(Ats[I], ':');
				Pos.See(Sky.Clouds[I].AtX, G[0]);
				Pos.See(Sky.Clouds[I].AtY, G[1]);
				Wraps += std::fabs(Sky.Clouds[I].AtX - Was[I]) > W * 0.5;
			}
			std::vector<double> Base = Numbers(R.at("base"), ',');
			Pc::PcFromGodot(Base[4], Base[5], Base[6]); // Godot's palette colour as ours now (the restyle, PcGodotLook.h)
			const Pc::PcSkyColour B = Pc::PcSky::BaseColour();
			CHECK(Base[0] == 0.0);
			CHECK(Base[1] == 0.0);
			CHECK(Base[2] == static_cast<double>(Sky.Width));
			CHECK(Base[3] == static_cast<double>(Sky.Height));
			CHECK(B.R == Base[4]);
			CHECK(B.G == Base[5]);
			CHECK(B.B == Base[6]);
			CHECK(B.A == Base[7]);
			const std::vector<std::string> GStars = Split(R.at("stars"), ';');
			REQUIRE(GStars.size() == Sky.Stars.size());
			for (size_t I = 0; I < GStars.size(); ++I)
			{
				std::vector<double> G = Numbers(GStars[I], ',');
				Pc::PcFromGodot(G[3], G[4], G[5]);
				const Pc::PcSkyStar& S = Sky.Stars[I];
				const Pc::PcSkyColour C = Sky.StarColour(S);
				Pos.See(S.AtX, G[0]);
				Pos.See(S.AtY, G[1]);
				Pos.See(S.Radius, G[2]);
				Col.See(C.R, G[3]);
				Col.See(C.G, G[4]);
				Col.See(C.B, G[5]);
				Col.See(C.A, G[6]);
			}
			StarsDrawn += static_cast<int>(GStars.size());
			const std::vector<std::string> GClouds = Split(R.at("clouds"), ';');
			REQUIRE(GClouds.size() == Sky.Clouds.size()); // PolyGuard passes every cloud: star-shaped, never thin
			for (size_t I = 0; I < GClouds.size(); ++I)
			{
				std::vector<double> G = Numbers(GClouds[I], ':');
				Pc::PcFromGodot(G[0], G[1], G[2]);
				const Pc::PcSkyColour C = Sky.CloudColour(Sky.Clouds[I]);
				Col.See(C.R, G[0]);
				Col.See(C.G, G[1]);
				Col.See(C.B, G[2]);
				Col.See(C.A, G[3]);
				Sky.CloudOutline(Sky.Clouds[I], Xs, Ys);
				REQUIRE(G.size() == 4 + 2 * Xs.size());
				for (size_t P = 0; P < Xs.size(); ++P)
				{
					Pos.See(Xs[P], G[4 + 2 * P]);
					Pos.See(Ys[P], G[5 + 2 * P]);
				}
				++Polys;
			}
		}
	}
	MESSAGE("sky: worst position " << Pos.Value << ", colour " << Col.Value << "; " << Polys << " clouds, " << StarsDrawn
		<< " stars drawn, " << Wraps << " wraps");
	CHECK(Wraps > 5);
	CHECK(Pos.Value == 0.0);
	CHECK(Col.Value == 0.0);
}

#endif
