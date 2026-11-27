#if defined(PAWCORE_STANDALONE_TESTS)

// P4.3 (playtest 2026-11-26, notes 2 and 3): Boba's clock and her cameos' paths.

#include "ThirdParty/doctest/doctest.h"
#include "PcBoba.h"
#include "PcJuice.h"

#include <cmath>
#include <map>
#include <vector>

namespace
{
	const std::vector<double> Photos = { 0.77, 0.83, 0.89, 0.64, 0.58, 0.48, 1.84, 0.72, 0.52, 1.41, 1.53, 0.56, 0.91, 0.85 };
	constexpr double W = 1920.0;
	constexpr double H = 1080.0;

	// Steps until a cameo starts (or the time runs out); the seconds it took.
	double UntilCameo(Pc::PcBoba& B, double Limit, bool bEffects = true)
	{
		double T = 0.0;
		while (T < Limit)
		{
			T += 0.1;
			if (Pc::BobaStep(B, 0.1, bEffects, Photos, H))
			{
				return T;
			}
		}
		return -1.0;
	}
}

TEST_CASE("Boba: the menu clock")
{
	Pc::PcBoba B;
	B.Seed = 7;
	CHECK(B.Timer < 0.0);
	CHECK_FALSE(Pc::BobaStep(B, 100.0, true, Photos, H)); // no menu: nothing
	Pc::BobaEnterMenu(B, "main_menu");
	CHECK(B.Timer >= Pc::BobaFirstMin);
	CHECK(B.Timer <= Pc::BobaFirstMax);
	const double First = UntilCameo(B, 20.0);
	CHECK(First >= Pc::BobaFirstMin - 1e-9);
	CHECK(First <= Pc::BobaFirstMax + 0.1 + 1e-9);
	CHECK(B.Cameo.bLive);
	CHECK(B.Timer >= Pc::BobaEveryMin - 1e-9);
	CHECK(B.Timer <= Pc::BobaEveryMax);

	// A second menu over it keeps the clock; the last one closing stops it, but the cameo in flight finishes.
	Pc::BobaEnterMenu(B, "settings");
	Pc::BobaLeaveMenu(B, "main_menu");
	CHECK(B.Timer > 0.0);
	Pc::BobaLeaveMenu(B, "settings");
	CHECK(B.Timer < 0.0);
	CHECK(B.Cameo.bLive);
	const double Left = B.Cameo.Duration - B.Cameo.T;
	CHECK_FALSE(Pc::BobaStep(B, Left + 0.01, true, Photos, H));
	CHECK_FALSE(B.Cameo.bLive);
	CHECK(UntilCameo(B, 200.0) < 0.0); // closed: never again until a menu opens
}

TEST_CASE("Boba: the switches (effects, no photo, the skill tree, one at a time)")
{
	Pc::PcBoba B;
	B.Seed = 11;
	Pc::BobaEnterMenu(B, "pause");
	CHECK(UntilCameo(B, 400.0, false) < 0.0); // effects off: the clock only reschedules
	CHECK(B.Timer > 0.0);
	CHECK_FALSE(Pc::BobaForce(B, Pc::PcBobaKind::Nyan, false, Photos, H)); // a forced one too (the first try's finding)
	CHECK_FALSE(Pc::BobaForce(B, Pc::PcBobaKind::Nyan, true, {}, H));     // no photo
	CHECK(Pc::BobaForce(B, Pc::PcBobaKind::Nyan, true, Photos, H));
	CHECK(B.Cameo.Kind == Pc::PcBobaKind::Nyan);
	CHECK_FALSE(Pc::BobaForce(B, Pc::PcBobaKind::Drop, true, Photos, H)); // one at a time
	CHECK_FALSE(Pc::BobaForceAny(B, true, Photos, H));

	// The skill tree cuts her and holds the clock.
	Pc::BobaSuppress(B, "tree", true);
	CHECK_FALSE(B.Cameo.bLive);
	CHECK(UntilCameo(B, 400.0) < 0.0);
	CHECK_FALSE(Pc::BobaForce(B, Pc::PcBobaKind::Flyby, true, Photos, H));
	Pc::BobaSuppress(B, "tree", false);
	CHECK(UntilCameo(B, 100.0) > 0.0);
}

TEST_CASE("Boba: never the same photo twice running; the kinds by weight")
{
	Pc::PcBoba B;
	B.Seed = 3;
	Pc::BobaEnterMenu(B, "main_menu");
	std::map<Pc::PcBobaKind, int> Kinds;
	int Last = -1;
	int Repeats = 0;
	const int N = 3000;
	for (int I = 0; I < N; ++I)
	{
		B.Cameo.bLive = false;
		B.Timer = 0.05;
		REQUIRE(Pc::BobaStep(B, 0.1, true, Photos, H));
		Repeats += B.Cameo.Photo == Last ? 1 : 0;
		Last = B.Cameo.Photo;
		CHECK(B.Cameo.Photo >= 0);
		CHECK(B.Cameo.Photo < static_cast<int>(Photos.size()));
		++Kinds[B.Cameo.Kind];
	}
	CHECK(Repeats == 0);
	const double Total = 4.0 + 1.0 + 3.0 + 4.0 + 2.0;
	const Pc::PcBobaKind All[] = { Pc::PcBobaKind::Flyby, Pc::PcBobaKind::Spin, Pc::PcBobaKind::Nyan, Pc::PcBobaKind::Peek, Pc::PcBobaKind::Drop };
	for (int K = 0; K < 5; ++K)
	{
		const double Share = static_cast<double>(Kinds[All[K]]) / N;
		CHECK_MESSAGE(std::abs(Share - Pc::BobaWeights[K] / Total) < 0.03, K);
	}
}

TEST_CASE("Boba: the paths")
{
	Pc::PcBoba B;
	B.Seed = 5;
	for (int I = 0; I < 200; ++I)
	{
		const Pc::PcBobaKind Kind = static_cast<Pc::PcBobaKind>(I % 5);
		B.Cameo.bLive = false;
		REQUIRE(Pc::BobaForce(B, Kind, true, Photos, H));
		const Pc::PcBobaCameo& C = B.Cameo;
		CHECK(C.Size >= Pc::BobaSizeMin * H);
		CHECK(C.Size <= Pc::BobaSizeMax * H);
		const double HalfW = C.Size * C.Aspect * 0.5;
		const double HalfH = C.Size * 0.5;
		const Pc::PcBobaPose Begin = Pc::BobaPose(C, W, H, 0.0);
		const Pc::PcBobaPose End = Pc::BobaPose(C, W, H, C.Duration);
		CHECK(Begin.bVisible);
		CHECK_FALSE(Pc::BobaPose(C, W, H, C.Duration + 0.01).bVisible);
		switch (Kind)
		{
		case Pc::PcBobaKind::Flyby:
		case Pc::PcBobaKind::Spin:
		case Pc::PcBobaKind::Nyan:
			// From just off one side to just off the other, mirrored when going right.
			CHECK(Begin.X == doctest::Approx(C.bRight ? -HalfW : W + HalfW));
			CHECK(End.X == doctest::Approx(C.bRight ? W + HalfW : -HalfW));
			CHECK(Begin.bFlip == C.bRight);
			CHECK(C.Duration >= (Kind == Pc::PcBobaKind::Nyan ? 3.0 : 3.6));
			CHECK(C.Duration <= (Kind == Pc::PcBobaKind::Nyan ? 4.2 : 5.6));
			if (Kind == Pc::PcBobaKind::Spin)
			{
				CHECK(std::abs(End.Rotation) == doctest::Approx(360.0 * C.Turns));
			}
			break;
		case Pc::PcBobaKind::Peek:
		{
			// Hidden at both ends; at the hold, BobaPeekShow of her in from the edge.
			const Pc::PcBobaPose Hold = Pc::BobaPose(C, W, H, C.In + 0.001);
			switch (C.Edge)
			{
			case Pc::PcBobaEdge::Left:
				CHECK(Begin.X == doctest::Approx(-HalfW));
				CHECK(Hold.X + HalfW == doctest::Approx(Pc::BobaPeekShow * 2.0 * HalfW).epsilon(0.02));
				break;
			case Pc::PcBobaEdge::Right:
				CHECK(Begin.X == doctest::Approx(W + HalfW));
				CHECK(W - (Hold.X - HalfW) == doctest::Approx(Pc::BobaPeekShow * 2.0 * HalfW).epsilon(0.02));
				break;
			case Pc::PcBobaEdge::Top:
				CHECK(Begin.Y == doctest::Approx(-HalfH));
				CHECK(std::abs(Hold.Rotation - 180.0) <= 3.0 + 1e-9); // upside down (and its wobble)
				break;
			case Pc::PcBobaEdge::Bottom:
				CHECK(Begin.Y == doctest::Approx(H + HalfH));
				break;
			}
			CHECK(Pc::BobaPose(C, W, H, C.Duration - 1e-9).X == doctest::Approx(Begin.X).epsilon(0.01));
			break;
		}
		case Pc::PcBobaKind::Drop:
			CHECK(Begin.Y == doctest::Approx(-HalfH));
			CHECK(Pc::BobaPose(C, W, H, C.In).Y == doctest::Approx(H * Pc::BobaDropLand));
			CHECK(End.Y == doctest::Approx(H + HalfH));
			break;
		}
	}
}

TEST_CASE("Boba: the nyan trail follows her, newest first")
{
	Pc::PcBoba B;
	B.Seed = 9;
	REQUIRE(Pc::BobaForce(B, Pc::PcBobaKind::Nyan, true, Photos, H));
	B.Cameo.T = B.Cameo.Duration * 0.5;
	const auto Trail = Pc::BobaTrail(B.Cameo, W, H);
	REQUIRE(Trail.size() == static_cast<size_t>(Pc::BobaTrailPoints));
	const Pc::PcBobaPose Now = Pc::BobaPose(B.Cameo, W, H);
	CHECK(Trail.front().first == doctest::Approx(Now.X));
	const double Dx = std::abs(Trail[1].first - Trail[0].first);
	CHECK(Dx == doctest::Approx(Pc::BobaTrailStep).epsilon(0.01));
	// Behind her: the trail lies on the side she came from.
	CHECK((B.Cameo.bRight ? Trail.back().first < Now.X : Trail.back().first > Now.X));
	B.Cameo.Kind = Pc::PcBobaKind::Flyby;
	CHECK(Pc::BobaTrail(B.Cameo, W, H).empty());
}

TEST_CASE("Juice: the bounce easing (Boba's drop)")
{
	CHECK(Pc::TweenInterpolate(0.0, 1.0, 0.0, 1.0, Pc::PcTrans::Bounce, Pc::PcEase::Out) == doctest::Approx(0.0));
	CHECK(Pc::TweenInterpolate(0.0, 1.0, 1.0, 1.0, Pc::PcTrans::Bounce, Pc::PcEase::Out) == doctest::Approx(1.0));
	// easing_equations.h: t = 0.5 -> 7.5625 (0.5 - 1.5/2.75)^2 + 0.75.
	const double Mid = 7.5625 * std::pow(0.5 - 1.5 / 2.75, 2.0) + 0.75;
	CHECK(Pc::TweenInterpolate(0.0, 1.0, 0.5, 1.0, Pc::PcTrans::Bounce, Pc::PcEase::Out) == doctest::Approx(Mid));
	CHECK(Pc::TweenInterpolate(0.0, 1.0, 0.25, 1.0, Pc::PcTrans::Bounce, Pc::PcEase::In)
		== doctest::Approx(1.0 - Pc::TweenInterpolate(0.0, 1.0, 0.75, 1.0, Pc::PcTrans::Bounce, Pc::PcEase::Out)));
}

#endif
