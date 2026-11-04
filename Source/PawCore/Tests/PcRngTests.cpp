#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcRng.h"

#include <cmath>
#include <vector>

TEST_CASE("RNG is a pure function of its inputs")
{
	CHECK(Pc::RngHash(1, 2, 3, 4) == Pc::RngHash(1, 2, 3, 4));
	CHECK(Pc::RngHash(1, 2, 3, 4) != Pc::RngHash(1, 2, 3, 5));
	CHECK(Pc::RngHash(1, 2, 3, 4) != Pc::RngHash(1, 3, 2, 4)); // argument order matters
	CHECK(Pc::RngHash(0, 0, 0, 0) != 0);
	// Frozen values, computed independently by a Python port of the same mix: if these change, every saved replay and
	// bug-report seed changes meaning, and a Python tool using the hash would drift from the game.
	CHECK(Pc::RngHash(2026, 10, 28, Pc::Salt::Crit) == 0x98cb0a496ac54a24ull);
	CHECK(Pc::RngHash(0, 0, 0, 0) == 0x2130748aaac80268ull);
}

TEST_CASE("RngUnit stays in [0, 1) and is uniform over a million draws")
{
	constexpr int Buckets = 64;
	constexpr int Draws = 1000000;
	std::vector<int> Count(Buckets, 0);
	double Sum = 0.0;
	for (int I = 0; I < Draws; ++I)
	{
		const double U = Pc::RngUnit(7, static_cast<uint64_t>(I), 11, Pc::Salt::Golden);
		REQUIRE(U >= 0.0);
		REQUIRE(U < 1.0);
		++Count[static_cast<int>(U * Buckets)];
		Sum += U;
	}
	double Chi2 = 0.0;
	const double Expected = static_cast<double>(Draws) / Buckets;
	for (int C : Count)
	{
		Chi2 += (C - Expected) * (C - Expected) / Expected;
	}
	// 63 degrees of freedom: the 99.99th percentile is about 117; a broken mixer lands in the thousands.
	CHECK(Chi2 < 117.0);
	CHECK(std::fabs(Sum / Draws - 0.5) < 0.002);
}

TEST_CASE("RngBelow covers [0, Count) evenly, including a count that is not a power of two")
{
	constexpr uint64_t N = 7;
	std::vector<int> Count(N, 0);
	for (uint64_t I = 0; I < 700000; ++I)
	{
		const uint64_t V = Pc::RngBelow(N, 99, I, 0, Pc::Salt::SpawnPick);
		REQUIRE(V < N);
		++Count[V];
	}
	for (int C : Count)
	{
		CHECK(std::abs(C - 100000) < 1500);
	}
	CHECK(Pc::RngBelow(1, 1, 2, 3, 4) == 0);
}

TEST_CASE("Neighbouring ticks and entities are not correlated")
{
	// Lag-1 correlation of consecutive ticks, and of consecutive entities on one tick.
	double SumXY = 0.0, SumX = 0.0, SumY = 0.0, SumXX = 0.0, SumYY = 0.0;
	constexpr int N = 200000;
	for (int I = 0; I < N; ++I)
	{
		const double X = Pc::RngUnit(5, static_cast<uint64_t>(I), 1, 0);
		const double Y = Pc::RngUnit(5, static_cast<uint64_t>(I + 1), 1, 0);
		SumXY += X * Y; SumX += X; SumY += Y; SumXX += X * X; SumYY += Y * Y;
	}
	const double R = (N * SumXY - SumX * SumY) / std::sqrt((N * SumXX - SumX * SumX) * (N * SumYY - SumY * SumY));
	CHECK(std::fabs(R) < 0.01);
}

#endif
