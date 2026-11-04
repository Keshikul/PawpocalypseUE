#if defined(PAWCORE_STANDALONE_TESTS)

// PcField's rules (phase 2, slice 1). The stateful field is frame-driven in Godot and rolled its global RNG, so it cannot be
// replayed against Godot row by row; its pure parts are (PcFieldRulesTests.cpp). These tests pin each rule the Godot code
// and its comments state, on the real data, with the field's own deterministic randomness.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcFieldRules.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace
{
	const Pc::PcGameData& Game()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			const char* Env = std::getenv("PAWCORE_DATA_DIR");
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(Env ? Env : "Data", Data, Errors));
			bLoaded = true;
		}
		return Data;
	}

	const Pc::PcRect Arena{ 0.0, 0.0, 1600.0, 900.0 };
	constexpr double Step = 1.0 / 60.0;

	// A fresh run standing in a biome, the cat parked far outside the field (it strikes nothing).
	struct Run
	{
		Pc::PcProgress P;
		Pc::PcField F;
		Pc::PcFieldInput In;

		explicit Run(const std::string& Biome, uint64_t Seed = 7)
		{
			P.CurrentBiomeId = Biome;
			In.CatX = -10000.0;
			In.CatY = -10000.0;
			Pc::BeginField(F, Game(), P, Biome, Arena, Seed);
		}

		void Steps(int N)
		{
			for (int K = 0; K < N; ++K)
			{
				Pc::StepField(F, Game(), P, In, Step);
			}
		}

		// The field with no spawner: the tests place their own mice.
		void Quiet()
		{
			F.SpawnTimeLeft = 1e18;
		}

		size_t Count(Pc::PcFieldEventKind Kind) const
		{
			return static_cast<size_t>(std::count_if(F.Events.begin(), F.Events.end(), [Kind](const Pc::PcFieldEvent& E) { return E.Kind == Kind; }));
		}

		bool Alive(int32_t Slot) const
		{
			return Slot >= 0 && F.Active[static_cast<size_t>(Slot)] != 0;
		}
	};

	double Heading(const Pc::PcField& F, int32_t Slot)
	{
		return std::atan2(F.DirY[static_cast<size_t>(Slot)], F.DirX[static_cast<size_t>(Slot)]);
	}
}

TEST_CASE("Field: a step is a pure function of the seed, the progress and the input")
{
	auto Play = [](uint64_t Seed)
	{
		Run R("tier_1", Seed);
		for (int K = 0; K < 1800; ++K)
		{
			// The cat chases the oldest live mouse, so mice are struck, killed, split and paid.
			if (!R.F.Live.empty())
			{
				const size_t I = static_cast<size_t>(R.F.Live.front());
				R.In.CatX = R.F.X[I];
				R.In.CatY = R.F.Y[I];
			}
			Pc::StepField(R.F, Game(), R.P, R.In, Step);
		}
		return R;
	};
	const Run A = Play(11);
	const Run B = Play(11);
	const Run C = Play(12);
	CHECK(A.F.X == B.F.X);
	CHECK(A.F.Y == B.F.Y);
	CHECK(A.F.Hp == B.F.Hp);
	CHECK(A.F.Live == B.F.Live);
	CHECK(A.P.Catnip == B.P.Catnip);
	CHECK(A.P.LifetimeKills == B.P.LifetimeKills);
	CHECK(A.F.Events.size() == B.F.Events.size());
	CHECK(A.P.LifetimeKills > 10); // the chase really fought (16 kills in 30 s for a fresh cat in the Backyard)
	CHECK(A.F.X != C.F.X);        // another seed is another field
}

TEST_CASE("Field: the biome spawns at its rate times the player's levers, never past max_live_enemies")
{
	const Pc::PcGameData& D = Game();
	{
		Run R("tier_1");
		const double PerSec = Pc::SpawnRatePerSec(D, R.P, D.Tiers[0]);
		R.Steps(60 * 60);
		const double Expected = PerSec * 60.0;
		CHECK(static_cast<double>(R.Count(Pc::PcFieldEventKind::Spawned)) == doctest::Approx(Expected).epsilon(0.03));
		CHECK(R.Count(Pc::PcFieldEventKind::Killed) == 0); // the cat is away: nothing struck
		CHECK(R.Count(Pc::PcFieldEventKind::Despawned) > 0); // commons wander off unrewarded
	}
	{
		// A flood: Infestation far past anything reachable, in the busiest biome. The batch path and the ceiling.
		Run R("tier_12");
		R.P.InfestationBonus = 1000.0;
		const Pc::PcSpawnCadence Cadence = Pc::SpawnCadence(D, Pc::SpawnRatePerSec(D, R.P, D.Tiers[11]));
		CHECK(Cadence.Batch > 1);
		int64_t Most = 0;
		for (int K = 0; K < 240; ++K)
		{
			R.Steps(1);
			Most = std::max(Most, R.F.LiveCount());
			REQUIRE(R.F.LiveCount() <= D.Code.MaxLiveEnemies);
		}
		CHECK(Most == D.Code.MaxLiveEnemies);
	}
}

TEST_CASE("Field: a spawn enters at an edge, inset, heading inward within the spread")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_5");
	int Checked = 0;
	for (int K = 0; K < 1800; ++K)
	{
		const size_t Before = R.F.Events.size();
		R.Steps(1);
		for (size_t E = Before; E < R.F.Events.size(); ++E)
		{
			const Pc::PcFieldEvent& Ev = R.F.Events[E];
			if (Ev.Kind != Pc::PcFieldEventKind::Spawned)
			{
				continue;
			}
			const double Inset = D.Code.SpawnEdgeInset;
			const bool bOnEdge = std::fabs(Ev.Y - Inset) < 1e-9 || std::fabs(Ev.Y - (Arena.H - Inset)) < 1e-9
				|| std::fabs(Ev.X - Inset) < 1e-9 || std::fabs(Ev.X - (Arena.W - Inset)) < 1e-9;
			CHECK(bOnEdge);
			CHECK(Arena.HasPoint(Ev.X, Ev.Y));
			// Heading against the line to the centre: within the spread (it has not moved yet: it was born this step).
			const double ToCentre = std::atan2(Arena.CentreY() - Ev.Y, Arena.CentreX() - Ev.X);
			double Off = std::fabs(Heading(R.F, Ev.Slot) - ToCentre);
			Off = std::min(Off, 2.0 * 3.141592653589793 - Off);
			CHECK(Off <= D.Code.SpawnDirectionSpread + 1e-9);
			++Checked;
		}
	}
	CHECK(Checked > 80); // ~3.2 a second for 30 s
}

TEST_CASE("Field: movement verbs: a flier crosses straight and fast, a common wanders, gristle closes on the cat")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_2");
	R.Quiet();
	R.In.CatX = 1400.0;
	R.In.CatY = 450.0;
	const int32_t Flier = Pc::SpawnEnemyAt(R.F, D, R.P, "alley_flying_mouse", 200.0, 200.0, 1.0, 0.0);
	const int32_t Common = Pc::SpawnEnemyAt(R.F, D, R.P, "alley_mouse", 800.0, 450.0, 0.0, 1.0);
	const int32_t Gristle = Pc::SpawnEnemyAt(R.F, D, R.P, "alley_gristle", 200.0, 800.0, 0.0, -1.0);
	REQUIRE((Flier >= 0 && Common >= 0 && Gristle >= 0));
	const double GristleStart = std::hypot(R.F.X[Gristle] - R.In.CatX, R.F.Y[Gristle] - R.In.CatY);
	const double CommonHeading = Heading(R.F, Common);
	R.Steps(30);
	// The flier: heading untouched, speed = enemy speed x the flier multiplier x its data multiplier.
	CHECK(R.F.DirX[Flier] == 1.0);
	CHECK(R.F.DirY[Flier] == 0.0);
	const double FlierPace = D.Code.EnemySpeedPxPerSec * D.Code.EnemyFlierSpeedMultiplier * D.FindEnemy("alley_flying_mouse")->SpeedMultiplier;
	CHECK(R.F.X[Flier] - 200.0 == doctest::Approx(FlierPace * 30.0 * Step));
	// The common: its heading drifted, at most wander_turn_rate per second.
	const double Turned = std::fabs(Heading(R.F, Common) - CommonHeading);
	CHECK(Turned > 0.0);
	CHECK(Turned <= D.Code.EnemyWanderTurnRate * 30.0 * Step + 1e-9);
	// Gristle lumbers toward the cat.
	R.Steps(150);
	CHECK(std::hypot(R.F.X[Gristle] - R.In.CatX, R.F.Y[Gristle] - R.In.CatY) < GristleStart - 50.0);
}

TEST_CASE("Field: a golden mouse is turned back at the edge and leaves only when its lifetime ends")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_1");
	R.Quiet();
	const Pc::PcEnemy& Golden = *D.FindEnemy("backyard_golden_mouse");
	REQUIRE(Golden.LifetimeSec > 0.0);
	// Born beside the right edge, running right, the cat right behind it.
	const int32_t G = Pc::SpawnEnemyAt(R.F, D, R.P, Golden.Id, 1560.0, 450.0, 1.0, 0.0);
	R.In.CatX = 1450.0;
	R.In.CatY = 450.0;
	const int Life = static_cast<int>(std::ceil(Golden.LifetimeSec / Step));
	int K = 0;
	for (; K < Life + 2 && R.Alive(G); ++K)
	{
		R.Steps(1);
		if (R.Alive(G))
		{
			CHECK(Arena.HasPoint(R.F.X[G], R.F.Y[G]));
		}
	}
	CHECK_FALSE(R.Alive(G));
	CHECK(K >= Life - 1); // it lived its whole lifetime
	CHECK(R.Count(Pc::PcFieldEventKind::Despawned) == 1);
	CHECK(R.P.Catnip == 0.0);
}

TEST_CASE("Field: a common walking off the field is gone, unrewarded; Funnel turns it back first")
{
	const Pc::PcGameData& D = Game();
	{
		Run R("tier_1");
		R.Quiet();
		const int32_t M = Pc::SpawnEnemyAt(R.F, D, R.P, "backyard_mouse", 1590.0, 450.0, 1.0, 0.0);
		R.Steps(20);
		CHECK_FALSE(R.Alive(M));
		CHECK(R.Count(Pc::PcFieldEventKind::Despawned) == 1);
		CHECK(R.P.Catnip == 0.0);
		CHECK(R.P.LifetimeKills == 0);
	}
	{
		Run R("tier_1");
		R.Quiet();
		R.P.BranchLevels["unlock_snare"] = 1;
		R.P.BranchLevels["funnel"] = static_cast<int64_t>(D.FindBranch("funnel")->LevelCosts.size());
		const double Bounces = Pc::FunnelBounces(D, R.P);
		REQUIRE(Bounces >= 1.0);
		const int32_t M = Pc::SpawnEnemyAt(R.F, D, R.P, "backyard_mouse", 1590.0, 450.0, 1.0, 0.0);
		const double Given = R.F.Bounces[M];
		CHECK(Given >= std::floor(Bounces));
		R.Steps(20);
		CHECK(R.Alive(M)); // turned back toward the middle
		CHECK(R.F.Bounces[M] == Given - 1.0);
		CHECK(R.F.X[M] < 1590.0);
	}
}

TEST_CASE("Field: a signature's shield cycles on its data, and the paw does nothing through it")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_3");
	R.Quiet();
	const Pc::PcEnemy& Goose = *D.FindEnemy("park_goose");
	REQUIRE(Goose.InvulnVulnerableSec > 0.0);
	const int32_t S = Pc::SpawnEnemyAt(R.F, D, R.P, Goose.Id, 800.0, 450.0, 1.0, 0.0);
	const int Vulnerable = static_cast<int>(std::ceil(Goose.InvulnVulnerableSec / Step));
	R.Steps(Vulnerable - 1);
	CHECK(R.F.Shielded[S] == 0);
	R.Steps(2);
	CHECK(R.F.Shielded[S] == 1);
	// Shielded, with the cat on top of it: no damage lands (no Shield Pierce owned).
	const double Hp = R.F.Hp[S];
	R.In.CatX = R.F.X[S];
	R.In.CatY = R.F.Y[S];
	R.Steps(30);
	CHECK(R.F.Hp[S] == Hp);
	CHECK(R.Count(Pc::PcFieldEventKind::Strike) > 0);
	R.In.CatX = -10000.0;
	R.Steps(static_cast<int>(std::ceil(Goose.InvulnShieldSec / Step)));
	CHECK(R.F.Shielded[S] == 0);
	CHECK(R.Count(Pc::PcFieldEventKind::Shield) >= 2);
}

TEST_CASE("Field: the paw strikes at its rate, shred builds on one target to its ceiling and resets on another")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_3");
	R.Quiet();
	// Tough enough to survive the whole test.
	const int32_t T = Pc::SpawnEnemyAt(R.F, D, R.P, "park_gristle", 800.0, 450.0, 1.0, 0.0);
	const double Rate = Pc::StrikesPerSec(D.Combat, R.F.Stats);
	for (int K = 0; K < 600; ++K)
	{
		R.In.CatX = R.F.X[T];
		R.In.CatY = R.F.Y[T];
		R.Steps(1);
	}
	REQUIRE(R.Alive(T));
	// The streak itself stops at the ceiling (PawStrikeDamage clamps too, so the damage alone cannot show an uncapped streak).
	CHECK(R.F.ShredStacks == D.Combat.ShredMaxStacks);
	std::vector<double> Blows;
	for (const Pc::PcFieldEvent& E : R.F.Events)
	{
		if (E.Kind == Pc::PcFieldEventKind::Strike)
		{
			Blows.push_back(E.Value);
		}
	}
	CHECK(static_cast<double>(Blows.size()) == doctest::Approx(Rate * 10.0).epsilon(0.05));
	REQUIRE(Blows.size() > static_cast<size_t>(D.Combat.ShredMaxStacks + 2));
	for (size_t K = 0; K < Blows.size(); ++K)
	{
		CAPTURE(K);
		const int64_t Stacks = std::min<int64_t>(static_cast<int64_t>(K), D.Combat.ShredMaxStacks);
		CHECK(Blows[K] == Pc::PawStrikeDamage(D.Combat, R.F.Stats, Stacks));
	}
	double Landed = 0.0;
	for (const double B : Blows)
	{
		Landed += B;
	}
	CHECK(R.F.Hp[T] == doctest::Approx(R.F.MaxHp[T] - Landed));
	// A second target: the streak starts over.
	const int32_t U = Pc::SpawnEnemyAt(R.F, D, R.P, "park_gristle", 300.0, 300.0, 1.0, 0.0);
	R.F.Events.clear();
	R.In.CatX = R.F.X[U];
	R.In.CatY = R.F.Y[U];
	R.Steps(40);
	REQUIRE(R.Count(Pc::PcFieldEventKind::Strike) > 0);
	CHECK(std::find_if(R.F.Events.begin(), R.F.Events.end(), [](const Pc::PcFieldEvent& E) { return E.Kind == Pc::PcFieldEventKind::Strike; })->Value
		== Pc::PawStrikeDamage(D.Combat, R.F.Stats, 0));
}

TEST_CASE("Field: crits land at the crit chance and pay the crit multiplier")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_3");
	R.Quiet();
	R.P.UpgradeLevels["crit_chance"] = 10;
	Pc::RefreshFieldStats(R.F, D, R.P);
	const double Chance = R.F.Stats.CritChance;
	REQUIRE((Chance > 0.05 && Chance < 0.95));
	const int32_t T = Pc::SpawnEnemyAt(R.F, D, R.P, "park_gristle", 800.0, 450.0, 1.0, 0.0);
	int Crits = 0;
	int Strikes = 0;
	for (int K = 0; K < 60 * 600 && R.Alive(T); ++K)
	{
		R.In.CatX = R.F.X[T];
		R.In.CatY = R.F.Y[T];
		R.F.Hp[T] = R.F.MaxHp[T]; // kept alive and in place: this test counts rolls
		R.F.X[T] = 800.0;
		R.F.Y[T] = 450.0;
		R.Steps(1);
		for (const Pc::PcFieldEvent& E : R.F.Events)
		{
			if (E.Kind == Pc::PcFieldEventKind::Strike)
			{
				++Strikes;
				const bool bCrit = (E.Flags & Pc::FieldFlag::Crit) != 0;
				Crits += bCrit ? 1 : 0;
				const double Plain = Pc::PawStrikeDamage(D.Combat, R.F.Stats, std::min<int64_t>(R.F.ShredStacks, D.Combat.ShredMaxStacks));
				CHECK(E.Value == doctest::Approx(bCrit ? Plain * R.F.Stats.CritMultiplier : Plain));
			}
		}
		R.F.Events.clear();
	}
	REQUIRE(Strikes > 1000); // 2 a second for 600 s
	const double Sigma = std::sqrt(Strikes * Chance * (1.0 - Chance));
	CHECK(std::fabs(Crits - Strikes * Chance) < 5.0 * Sigma);
}

TEST_CASE("Field: Overkill splashes the nearest survivor and never chains; the kill pays, counts and gives Catches")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_1");
	R.Quiet();
	R.P.UpgradeLevels["paw_damage"] = 60;   // one strike overkills a backyard mouse many times over
	R.P.UpgradeLevels["overkill"] = 20;
	Pc::RefreshFieldStats(R.F, D, R.P);
	REQUIRE(R.F.Stats.OverkillFraction > 0.0);
	// A at the cat; B nearest to A; C nearest to B but farther from A. A chain would go A -> B -> C.
	const int32_t A = Pc::SpawnEnemyAt(R.F, D, R.P, "backyard_mouse", 800.0, 450.0, 1.0, 0.0);
	const int32_t B = Pc::SpawnEnemyAt(R.F, D, R.P, "backyard_mouse", 840.0, 450.0, 1.0, 0.0);
	const int32_t C = Pc::SpawnEnemyAt(R.F, D, R.P, "backyard_mouse", 900.0, 450.0, 1.0, 0.0);
	const double HpC = R.F.Hp[C];
	R.In.CatX = 790.0;
	R.In.CatY = 450.0;
	const Pc::PcProgress Before = R.P;
	// Exactly one strike: the step the strike timer first fills.
	int K = 0;
	while (R.Count(Pc::PcFieldEventKind::Strike) == 0 && K++ < 120)
	{
		R.Steps(1);
	}
	REQUIRE(R.Count(Pc::PcFieldEventKind::Strike) == 1);
	CHECK_FALSE(R.Alive(A));
	CHECK_FALSE(R.Alive(B));     // the splash killed B...
	CHECK(R.Alive(C));           // ...and B's own excess went nowhere
	CHECK(R.F.Hp[C] == HpC);
	CHECK(R.Count(Pc::PcFieldEventKind::Splash) == 1);
	CHECK(R.Count(Pc::PcFieldEventKind::Killed) == 2);
	// Paid and counted: the Catnip credited is what the Killed events say, two kills, two Catches.
	double Paid = 0.0;
	for (const Pc::PcFieldEvent& E : R.F.Events)
	{
		if (E.Kind == Pc::PcFieldEventKind::Killed)
		{
			Paid += E.Value;
			CHECK(E.Value >= 1.0); // whole_catnip: every kill is felt
		}
	}
	CHECK(R.P.Catnip - Before.Catnip == Paid);
	CHECK(R.P.LifetimeKills == Before.LifetimeKills + 2);
	CHECK(R.P.TierKillCount == Before.TierKillCount + 2);
	CHECK(R.P.CatchesPoints + 24.0 * R.P.CatchesLevel >= Before.CatchesPoints + 2.0 - 1e-9);
}

TEST_CASE("Field: Execute finishes a mouse below its line, never through a shield")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_3");
	R.Quiet();
	R.P.UpgradeLevels["execute"] = 40;
	Pc::RefreshFieldStats(R.F, D, R.P);
	const double Line = R.F.Stats.ExecuteThreshold;
	REQUIRE(Line > 0.0);
	const int32_t T = Pc::SpawnEnemyAt(R.F, D, R.P, "park_gristle", 800.0, 450.0, 1.0, 0.0);
	R.F.Hp[T] = R.F.MaxHp[T] * Line * 0.99;
	R.In.CatX = 790.0;
	R.In.CatY = 450.0;
	R.Steps(40);
	CHECK_FALSE(R.Alive(T));
	// The same mouse shielded: the line does not reach it.
	const int32_t U = Pc::SpawnEnemyAt(R.F, D, R.P, "park_goose", 800.0, 450.0, 1.0, 0.0);
	R.F.Hp[U] = R.F.MaxHp[U] * Line * 0.99;
	R.F.Shielded[U] = 1;
	R.F.InvulnTimer[U] = -1000.0; // held shielded for the test
	const double Hp = R.F.Hp[U];
	R.In.CatX = R.F.X[U];
	R.In.CatY = R.F.Y[U];
	R.F.Events.clear();
	R.Steps(40);
	CHECK(R.Alive(U));
	CHECK(R.F.Hp[U] == Hp);
	// Not even swung for its whole HP: with Shield Pierce owned, an execute through the shield would land a share of it.
	REQUIRE(R.Count(Pc::PcFieldEventKind::Strike) > 0);
	for (const Pc::PcFieldEvent& E : R.F.Events)
	{
		if (E.Kind == Pc::PcFieldEventKind::Strike)
		{
			CHECK(E.Value <= Pc::PawStrikeDamage(D.Combat, R.F.Stats, D.Combat.ShredMaxStacks) * R.F.Stats.CritMultiplier);
		}
	}
}

TEST_CASE("Field: KillAllEnemies pays and counts every live mouse through the normal path")
{
	const Pc::PcGameData& D = Game();
	Run R("tier_1");
	R.Steps(600);
	const int64_t Live = R.F.LiveCount();
	REQUIRE(Live > 5);
	const int64_t Kills = R.P.LifetimeKills;
	Pc::KillAllEnemies(R.F, D, R.P, R.In);
	CHECK(R.F.LiveCount() == 0);
	CHECK(R.P.LifetimeKills == Kills + Live);
	CHECK(R.P.Catnip > 0.0);
}

#endif
