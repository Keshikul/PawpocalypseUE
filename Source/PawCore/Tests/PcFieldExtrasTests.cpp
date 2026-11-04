#if defined(PAWCORE_STANDALONE_TESTS)

// PcField's extras (phase 2, slice 5): the paw's nodes, the kill's nodes, the despawn recovery, the spawner's nodes, Sleight
// of Hand and the Prism. Their getters are checked against the real Godot code (PcExtrasTests.cpp); these pin the rules
// themselves, each from the Godot code and its comments.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcProgress.h"

#include <algorithm>
#include <cmath>
#include <iterator>
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

	// A quiet field (no spawner unless asked), the cat at (CatX, CatY).
	struct Extra
	{
		const Pc::PcGameData& D;
		Pc::PcProgress P;
		Pc::PcField F;
		Pc::PcFieldInput In;

		explicit Extra(const Pc::PcGameData& Data, double CatX = 800.0, double CatY = 450.0, bool bSpawner = false) : D(Data)
		{
			P.CurrentBiomeId = "tier_2";
			Pc::BeginField(F, D, P, "tier_2", Arena, 21);
			if (!bSpawner)
			{
				F.SpawnTimeLeft = 1e18;
			}
			In.CatX = CatX;
			In.CatY = CatY;
			F.CatX = CatX;
			F.CatY = CatY;
			Refresh();
		}

		void Refresh() { Pc::RefreshFieldStats(F, D, P); }

		void Steps(int N)
		{
			for (int K = 0; K < N; ++K)
			{
				Pc::StepField(F, D, P, In, Step);
			}
		}

		int32_t Mouse(const char* Id, double X, double Y, double Hp = 1e12)
		{
			const int32_t S = Pc::SpawnEnemyAt(F, D, P, Id, X, Y, 1.0, 0.0);
			REQUIRE(S >= 0);
			F.SpeedMult[static_cast<size_t>(S)] = 0.0;
			F.Hp[static_cast<size_t>(S)] = Hp;
			F.LifetimeLeft[static_cast<size_t>(S)] = 1e9;
			return S;
		}

		double Hp(int32_t S) const { return F.Hp[static_cast<size_t>(S)]; }

		std::vector<Pc::PcFieldEvent> Of(Pc::PcFieldEventKind Kind) const
		{
			std::vector<Pc::PcFieldEvent> Out;
			std::copy_if(F.Events.begin(), F.Events.end(), std::back_inserter(Out), [Kind](const Pc::PcFieldEvent& E) { return E.Kind == Kind; });
			return Out;
		}
	};

	// The Catnip that AddCurrency(Base) credits from a given progress (the multipliers it applies are the progress's).
	double Credited(const Pc::PcGameData& D, const Pc::PcProgress& From, double Base)
	{
		Pc::PcProgress Copy = From;
		const double Before = Copy.Catnip;
		Pc::AddCurrency(D, Copy, Base);
		return Copy.Catnip - Before;
	}

	// Swings until N strikes have landed on one mouse that cannot die; returns the strike events.
	std::vector<Pc::PcFieldEvent> Swings(Extra& A, size_t N)
	{
		for (int Guard = 0; Guard < 100000 && A.Of(Pc::PcFieldEventKind::Strike).size() < N; ++Guard)
		{
			A.Steps(1);
		}
		std::vector<Pc::PcFieldEvent> S = A.Of(Pc::PcFieldEventKind::Strike);
		S.resize(N);
		return S;
	}
}

TEST_CASE("Extras: Butcher's Rhythm crits on its share of swings, every one of them earned")
{
	const Pc::PcGameData& D = Game();
	Extra A(D);
	A.P.BranchLevels["butchers_rhythm"] = 1;
	A.Refresh();
	A.F.Stats.CritChance = 0.0; // only the counter can crit
	A.Mouse("alley_mouse", 830.0, 450.0);
	const std::vector<Pc::PcFieldEvent> S = Swings(A, 100);
	const size_t Crits = static_cast<size_t>(std::count_if(S.begin(), S.end(), [](const Pc::PcFieldEvent& E) { return (E.Flags & Pc::FieldFlag::Crit) != 0; }));
	const size_t Earned = static_cast<size_t>(std::count_if(S.begin(), S.end(), [](const Pc::PcFieldEvent& E) { return (E.Flags & Pc::FieldFlag::Earned) != 0; }));
	// 0.1 summed ten times is 0.9999999999999999 in a double (Godot's too): the first lands on the 11th swing.
	CHECK(Crits >= 9);
	CHECK(Crits <= 10);
	CHECK(Earned == Crits);
}

TEST_CASE("Extras: Inscrutable spends the crit chance as a cadence and never rolls; without it the crits are luck")
{
	const Pc::PcGameData& D = Game();
	Extra A(D);
	A.P.BranchLevels["inscrutable"] = 1;
	A.Refresh();
	A.F.Stats.CritChance = 0.25;
	A.Mouse("alley_mouse", 830.0, 450.0);
	const std::vector<Pc::PcFieldEvent> S = Swings(A, 200);
	for (size_t K = 0; K < S.size(); ++K)
	{
		CAPTURE(K);
		// Exactly every fourth swing, earned.
		const bool bCrit = (S[K].Flags & Pc::FieldFlag::Crit) != 0;
		CHECK(bCrit == ((K + 1) % 4 == 0));
		CHECK(((S[K].Flags & Pc::FieldFlag::Earned) != 0) == bCrit);
	}
	Extra B(D);
	B.Refresh();
	B.F.Stats.CritChance = 0.25;
	B.Mouse("alley_mouse", 830.0, 450.0);
	const std::vector<Pc::PcFieldEvent> L = Swings(B, 400);
	const size_t Lucky = static_cast<size_t>(std::count_if(L.begin(), L.end(), [](const Pc::PcFieldEvent& E) { return (E.Flags & Pc::FieldFlag::Crit) != 0; }));
	CHECK(Lucky > 60);
	CHECK(Lucky < 140);
	CHECK(std::none_of(L.begin(), L.end(), [](const Pc::PcFieldEvent& E) { return (E.Flags & Pc::FieldFlag::Earned) != 0; }));
}

TEST_CASE("Extras: Riddle arms on a crit and the NEXT swing finishes the mouse (F17); never through a shield")
{
	// A riddle that always arms (a test-only effect of 1.0), and a crit forced on the first swing (Butcher's Rhythm
	// charged to the brink).
	Pc::PcGameData D = Game();
	for (Pc::PcSkillBranch& B : D.Branches)
	{
		if (B.Id == "riddle")
		{
			B.LevelEffects = { 1.0 };
		}
	}
	Extra A(D);
	A.P.BranchLevels["riddle"] = 1;
	A.P.BranchLevels["butchers_rhythm"] = 1;
	A.Refresh();
	A.F.Stats.CritChance = 0.0;
	A.F.RhythmCharge = 0.95;
	const int32_t M = A.Mouse("alley_mouse", 830.0, 450.0, 1e12);
	// F17, designer 2026-11-03 (fixed in Godot first): the crit arms it, and only the NEXT swing spends it (the code used
	// to finish the mouse on the very swing that armed it, against its own comment).
	const std::vector<Pc::PcFieldEvent> S = Swings(A, 1);
	CHECK((S[0].Flags & Pc::FieldFlag::Crit) != 0);
	CHECK((S[0].Flags & Pc::FieldFlag::Execute) == 0);
	CHECK(A.F.Active[static_cast<size_t>(M)]); // a 1e12 mouse survives the swing that armed it
	CHECK(A.F.bRiddleArmed);
	A.F.RhythmCharge = 0.0; // no forced crit on the next swing: it only answers the riddle
	const std::vector<Pc::PcFieldEvent> S2 = Swings(A, 2); // the strikes so far: the arming one, then this one
	REQUIRE(S2.size() == 2);
	CHECK((S2[1].Flags & Pc::FieldFlag::Crit) == 0);
	CHECK((S2[1].Flags & Pc::FieldFlag::Execute) != 0);
	CHECK_FALSE(A.F.Active[static_cast<size_t>(M)]); // gone on the next
	CHECK_FALSE(A.F.bRiddleArmed);

	// Armed against a shield: not spent, nothing killed through it.
	Extra B(D);
	B.Refresh();
	B.F.Stats.CritChance = 0.0;
	B.F.bRiddleArmed = true;
	const int32_t G = B.Mouse("park_goose", 830.0, 450.0, 1e12);
	B.F.Shielded[static_cast<size_t>(G)] = 1;
	B.F.InvulnTimer[static_cast<size_t>(G)] = -1e9;
	Swings(B, 3);
	CHECK(B.Hp(G) == 1e12);
	CHECK(B.F.bRiddleArmed);
}

TEST_CASE("Extras: Anvil Paw swings again on its accumulated share, drawn as an echo")
{
	const Pc::PcGameData& D = Game();
	Extra A(D);
	A.P.BranchLevels["job_blacksmith"] = 1;
	A.P.BranchLevels["anvil_paw"] = 10; // 0.5: every second swing doubles
	A.Refresh();
	A.Mouse("alley_mouse", 830.0, 450.0);
	const std::vector<Pc::PcFieldEvent> S = Swings(A, 150);
	const size_t Echoes = static_cast<size_t>(std::count_if(S.begin(), S.end(), [](const Pc::PcFieldEvent& E) { return (E.Flags & Pc::FieldFlag::Echo) != 0; }));
	CHECK(Echoes == 50); // 100 swings and their 50 echoes
	CHECK((S[2].Flags & Pc::FieldFlag::Echo) != 0); // swing, swing + echo: the echo follows the second
}

TEST_CASE("Extras: Hammerfall carries the swipe down the lane behind the target, the first N in live order")
{
	const Pc::PcGameData& D = Game();
	Extra A(D);
	A.P.BranchLevels["hammerfall"] = 1; // 3
	A.Refresh();
	A.F.Stats.CritChance = 0.0;
	const int32_t Target = A.Mouse("alley_mouse", 850.0, 450.0);
	// Out of it, and FIRST in live order, so the count cannot be what spares them.
	const int32_t Long = A.Mouse("alley_mouse", 1150.0, 450.0); // 300 px behind: past the range
	const int32_t Wide = A.Mouse("alley_mouse", 950.0, 560.0);  // 110 px across: off the lane
	const int32_t Back = A.Mouse("alley_mouse", 700.0, 450.0);  // behind the CAT: already swept past
	// In the lane (spawned in this order; the first three in LIVE order are hit, not the nearest three).
	const int32_t L250 = A.Mouse("alley_mouse", 1100.0, 450.0);
	const int32_t L100 = A.Mouse("alley_mouse", 950.0, 450.0);
	const int32_t L150 = A.Mouse("alley_mouse", 1000.0, 490.0); // 40 px across the lane: still in it
	const int32_t L200 = A.Mouse("alley_mouse", 1050.0, 450.0);
	const std::vector<Pc::PcFieldEvent> S = Swings(A, 1);
	const double Blow = S[0].Value;
	CHECK(S[0].Slot == Target);
	CHECK(1e12 - A.Hp(L250) == doctest::Approx(Blow));
	CHECK(1e12 - A.Hp(L100) == doctest::Approx(Blow));
	CHECK(1e12 - A.Hp(L150) == doctest::Approx(Blow));
	CHECK(A.Hp(L200) == 1e12); // the fourth in the lane: past the count
	CHECK(A.Hp(Long) == 1e12);
	CHECK(A.Hp(Wide) == 1e12);
	CHECK(A.Hp(Back) == 1e12);
	REQUIRE(A.Of(Pc::PcFieldEventKind::Cleave).size() == 1);
	CHECK(A.Of(Pc::PcFieldEventKind::Cleave)[0].ToX == doctest::Approx(1100.0));
}

TEST_CASE("Extras: Cat Burglar's Cut sells the wasted damage, capped at cut_max_ratio of the kill's reward")
{
	const Pc::PcGameData& D = Game();
	Extra A(D, -5000.0, -5000.0);
	A.P.BranchLevels["cat_burglars_cut"] = 1;
	A.Refresh();
	Extra B(D, -5000.0, -5000.0);
	for (Extra* X : { &A, &B })
	{
		const int32_t M = X->Mouse("alley_mouse", 800.0, 450.0, 1e-9);
		X->F.MaxHp[static_cast<size_t>(M)] = 0.25; // the kill overshoots by 1 HP: four times its health, capped at 2
	}
	const Pc::PcProgress Before = A.P;
	Pc::KillAllEnemies(A.F, D, A.P, A.In);
	Pc::KillAllEnemies(B.F, D, B.P, B.In);
	const double Reward = D.FindEnemy("alley_mouse")->RewardCatnip * A.F.WaveReward[0];
	CHECK(A.P.Catnip - B.P.Catnip == doctest::Approx(Credited(D, Before, Reward * 0.5 * D.Code.CutMaxRatio)));
}

TEST_CASE("Extras: Overflow drops a Catnip bomb where an enormously overkilled mouse died")
{
	const Pc::PcGameData& D = Game();
	Extra A(D);
	A.P.BranchLevels["unlock_aura"] = 1;
	A.P.BranchLevels["overflow"] = 1; // x10 its health
	A.Refresh();
	A.F.Stats.CritChance = 0.0;
	const int32_t Victim = A.Mouse("alley_mouse", 830.0, 450.0, 1e-9);
	A.F.MaxHp[static_cast<size_t>(Victim)] = 1e-9;
	const int32_t Near = A.Mouse("alley_mouse", 980.0, 450.0); // 150 px from the corpse, outside aura and paw
	const int32_t Far = A.Mouse("alley_mouse", 1300.0, 450.0); // outside the bomb
	Swings(A, 1);
	REQUIRE_FALSE(A.F.Active[static_cast<size_t>(Victim)]);
	const std::vector<Pc::PcFieldEvent> Bombs = A.Of(Pc::PcFieldEventKind::CatnipBomb);
	REQUIRE(Bombs.size() == 1);
	CHECK(Bombs[0].X == 830.0);
	CHECK(1e12 - A.Hp(Near) == doctest::Approx(Pc::AuraDps(D.Combat, A.F.Stats) * D.Code.CatnipBombDamageSeconds));
	CHECK(A.Hp(Far) == 1e12);
}

TEST_CASE("Extras: the Catnip Bomb card goes off on every Nth kill")
{
	const Pc::PcGameData& D = Game();
	Extra A(D, -5000.0, -5000.0);
	A.P.CardLevels["catnip_bomb"] = 4; // every 15th
	A.Refresh();
	const int64_t Every = static_cast<int64_t>(Pc::CardEffectBestLow(D, A.P, Pc::CardEffect::CATNIP_BOMB_EVERY));
	REQUIRE(Every == 15);
	auto Kill = [&A](int64_t N)
	{
		for (int64_t K = 0; K < N; ++K)
		{
			A.Mouse("alley_mouse", 100.0 + 10.0 * static_cast<double>(K), 100.0, 1e-9);
		}
		Pc::KillAllEnemies(A.F, A.D, A.P, A.In);
	};
	Kill(Every - 1);
	CHECK(A.Of(Pc::PcFieldEventKind::CatnipBomb).empty());
	Kill(1);
	CHECK(A.Of(Pc::PcFieldEventKind::CatnipBomb).size() == 1);
	Kill(Every);
	CHECK(A.Of(Pc::PcFieldEventKind::CatnipBomb).size() == 2);
}

TEST_CASE("Extras: Long Lunch pays its bank on a kill and empties it; Levy pays every levy_every kills")
{
	const Pc::PcGameData& D = Game();
	{
		Extra A(D, -5000.0, -5000.0);
		A.P.BranchLevels["long_lunch"] = 10;
		A.P.LunchSeconds = D.Code.LunchFullSeconds; // a full bank: x5
		A.Refresh();
		Extra B(D, -5000.0, -5000.0);
		A.Mouse("alley_mouse", 800.0, 450.0, 1e-9);
		B.Mouse("alley_mouse", 800.0, 450.0, 1e-9);
		const Pc::PcProgress Before = A.P;
		Pc::KillAllEnemies(A.F, D, A.P, A.In);
		Pc::KillAllEnemies(B.F, D, B.P, B.In);
		const double Reward = D.FindEnemy("alley_mouse")->RewardCatnip * A.F.WaveReward[0];
		CHECK(A.P.Catnip - B.P.Catnip == doctest::Approx(Credited(D, Before, Reward * 5.0)).epsilon(1e-9));
		CHECK(A.P.LunchSeconds == 0.0);
	}
	{
		Extra A(D, -5000.0, -5000.0);
		A.P.BranchLevels["levy"] = 1; // x20
		A.P.KillsSinceTithe = D.Code.LevyEvery - 1;
		A.Refresh();
		Extra B(D, -5000.0, -5000.0);
		B.P.KillsSinceTithe = D.Code.LevyEvery - 1;
		A.Mouse("alley_mouse", 800.0, 450.0, 1e-9);
		B.Mouse("alley_mouse", 800.0, 450.0, 1e-9);
		const Pc::PcProgress Before = A.P;
		Pc::KillAllEnemies(A.F, D, A.P, A.In);
		Pc::KillAllEnemies(B.F, D, B.P, B.In);
		const double Reward = D.FindEnemy("alley_mouse")->RewardCatnip * A.F.WaveReward[0];
		CHECK(A.P.Catnip - B.P.Catnip == doctest::Approx(Credited(D, Before, Reward * 20.0)).epsilon(1e-9));
		CHECK(A.P.KillsSinceTithe == 0);
		CHECK(B.P.KillsSinceTithe == D.Code.LevyEvery); // unowned: the count runs on, nothing paid
	}
}

TEST_CASE("Extras: a mouse that walks off recovers Scavenge + Pickpocket of its base reward; a golden recovers nothing")
{
	const Pc::PcGameData& D = Game();
	Extra A(D, -5000.0, -5000.0);
	A.P.CardLevels["scavenger"] = 1;                     // 0.3
	A.P.BranchLevels["pickpocket"] = 2;                  // + 0.1
	A.Refresh();
	// A rich mouse: Catnip is credited in whole units, so a small reward would round 0.3 and 0.4 of it alike.
	const int32_t M = A.Mouse("litterbox_gristle", Arena.Right() - 1.0, 450.0);
	A.F.SpeedMult[static_cast<size_t>(M)] = 10.0;        // walking straight out
	const Pc::PcProgress Before = A.P;
	A.Steps(3);
	REQUIRE(A.Of(Pc::PcFieldEventKind::Despawned).size() == 1);
	CHECK(A.P.Catnip - Before.Catnip == doctest::Approx(Credited(D, Before, D.FindEnemy("litterbox_gristle")->RewardCatnip * 0.4)));
	CHECK(A.P.LifetimeKills == Before.LifetimeKills);   // not a kill
	// A golden is reward-exempt: it leaves by its lifetime and pays nothing back.
	Extra G(D, -5000.0, -5000.0);
	G.P.CardLevels["scavenger"] = 3;
	G.Refresh();
	const int32_t Gold = G.Mouse("alley_golden_mouse", 800.0, 450.0);
	G.F.LifetimeLeft[static_cast<size_t>(Gold)] = Step * 0.5;
	const double Catnip = G.P.Catnip;
	G.Steps(2);
	REQUIRE(G.Of(Pc::PcFieldEventKind::Despawned).size() == 1);
	CHECK(G.P.Catnip == Catnip);
}

TEST_CASE("Extras: the Pied Piper summons every mouse on a ring around the cat; Sixth Sense's foreseen spawn arrives as foreseen")
{
	const Pc::PcGameData& D = Game();
	Extra A(D, 800.0, 450.0, true);
	A.P.BranchLevels["pied_piper"] = 1; // 280 px
	A.Refresh();
	A.Steps(60 * 20);
	const std::vector<Pc::PcFieldEvent> Spawns = A.Of(Pc::PcFieldEventKind::Spawned);
	REQUIRE(Spawns.size() > 5);
	for (const Pc::PcFieldEvent& E : Spawns)
	{
		CHECK(std::hypot(E.X - 800.0, E.Y - 450.0) == doctest::Approx(280.0));
	}

	Extra S(D, -5000.0, -5000.0, true);
	S.P.BranchLevels["sixth_sense"] = 1;
	S.Refresh();
	int Checked = 0;
	for (int K = 0; K < 60 * 30 && Checked < 5; ++K)
	{
		const bool bHad = S.F.bForeseen;
		const int32_t Kind = S.F.ForeseenKind;
		const double FX = S.F.ForeseenX, FY = S.F.ForeseenY;
		const size_t Before = S.Of(Pc::PcFieldEventKind::Spawned).size();
		S.Steps(1);
		const std::vector<Pc::PcFieldEvent> After = S.Of(Pc::PcFieldEventKind::Spawned);
		if (bHad && After.size() > Before)
		{
			const Pc::PcFieldEvent& E = After[Before];
			CHECK(S.F.Kind[static_cast<size_t>(E.Slot)] == Kind);
			CHECK(E.X == FX);
			CHECK(E.Y == FY);
			++Checked;
		}
	}
	CHECK(Checked == 5);
	CHECK(S.F.bForeseen); // always one ahead
}

TEST_CASE("Extras: Sleight of Hand kills the first eligible mouse each interval, through the kill path; goldens are skipped")
{
	const Pc::PcGameData& D = Game();
	Extra A(D, -5000.0, -5000.0);
	A.P.BranchLevels["sleight_of_hand"] = 1; // 3 a minute
	A.Refresh();
	const double Interval = Pc::VanishInterval(D, A.P);
	REQUIRE(Interval == doctest::Approx(20.0));
	const int32_t Gold = A.Mouse("alley_golden_mouse", 400.0, 300.0);
	const int32_t First = A.Mouse("alley_mouse", 500.0, 300.0);
	const int32_t Second = A.Mouse("alley_mouse", 600.0, 300.0);
	const int64_t Kills = A.P.LifetimeKills;
	A.Steps(static_cast<int>(Interval / Step) - 2);
	CHECK(A.P.LifetimeKills == Kills);
	A.Steps(4);
	CHECK(A.F.Active[static_cast<size_t>(Gold)]);
	CHECK_FALSE(A.F.Active[static_cast<size_t>(First)]);
	CHECK(A.F.Active[static_cast<size_t>(Second)]);
	CHECK(A.P.LifetimeKills == Kills + 1); // counted and paid like any kill
}

TEST_CASE("Extras: the Prism's beam splits its DPS over the nearest and Refraction's chain, or the sweep")
{
	const Pc::PcGameData& D = Game();
	auto Prism = [&D](Extra& A)
	{
		A.P.BranchLevels["unlock_aura"] = 1;
		A.P.BranchLevels["unlock_prism"] = 1;
		A.Refresh();
		A.F.Stats.AttackSpeedMultiplier = 0.0; // the paw out of it
	};
	{
		Extra A(D);
		Prism(A);
		const double Beam = Pc::PrismBeamDps(D, A.P, A.F.Stats);
		REQUIRE(Beam > 0.0);
		const int32_t Near = A.Mouse("alley_mouse", 900.0, 450.0); // beyond the aura's 77 px
		const int32_t Next = A.Mouse("alley_mouse", 1000.0, 450.0);
		A.Steps(1);
		CHECK(1e12 - A.Hp(Near) == doctest::Approx(Beam * Step).epsilon(1e-3));
		CHECK(A.Hp(Next) == 1e12);
		REQUIRE(A.Of(Pc::PcFieldEventKind::PrismBeam).size() == 1);
	}
	{
		Extra A(D);
		A.P.BranchLevels["refraction"] = 1; // one more link, walking outward from the last
		Prism(A);
		const double Beam = Pc::PrismBeamDps(D, A.P, A.F.Stats);
		const int32_t Near = A.Mouse("alley_mouse", 900.0, 450.0);
		const int32_t Next = A.Mouse("alley_mouse", 1000.0, 450.0);   // nearest to the first link
		const int32_t Other = A.Mouse("alley_mouse", 700.0, 470.0);   // nearer the cat than Next, farther from Near
		A.Steps(1);
		CHECK(1e12 - A.Hp(Near) == doctest::Approx(Beam * Step / 2.0).epsilon(1e-3));
		CHECK(1e12 - A.Hp(Next) == doctest::Approx(Beam * Step / 2.0).epsilon(1e-3));
		CHECK(A.Hp(Other) == 1e12);
	}
	{
		Extra A(D);
		A.P.BranchLevels["spectrum"] = 1;
		A.P.BranchLevels["unlock_full_spectrum"] = 1;
		Prism(A);
		const double Beam = Pc::PrismBeamDps(D, A.P, A.F.Stats);
		const double Aura = Pc::AuraDps(D.Combat, A.F.Stats);
		const int32_t In1 = A.Mouse("alley_mouse", 840.0, 450.0); // both inside the aura: the sweep crosses them
		const int32_t In2 = A.Mouse("alley_mouse", 800.0, 400.0);
		const int32_t Out = A.Mouse("alley_mouse", 1000.0, 450.0);
		A.Steps(1);
		CHECK(1e12 - A.Hp(In1) == doctest::Approx((Aura + Beam / 2.0) * Step).epsilon(1e-3));
		CHECK(1e12 - A.Hp(In2) == doctest::Approx((Aura + Beam / 2.0) * Step).epsilon(1e-3));
		CHECK(A.Hp(Out) == 1e12);
	}
}

TEST_CASE("Extras: a beam kill pays Spectrum and makes a shard; Critical Mass detonates on every live mouse each N shards")
{
	const Pc::PcGameData& D = Game();
	auto BeamKillReward = [&D](bool bSpectrum)
	{
		Extra A(D);
		A.P.BranchLevels["unlock_aura"] = 1;
		A.P.BranchLevels["unlock_prism"] = 1;
		if (bSpectrum)
		{
			A.P.BranchLevels["spectrum"] = 10;
		}
		A.Refresh();
		A.F.Stats.AttackSpeedMultiplier = 0.0;
		const int32_t M = A.Mouse("alley_mouse", 900.0, 450.0, 1e-9);
		const Pc::PcProgress Before = A.P;
		const double Base = D.FindEnemy("alley_mouse")->RewardCatnip * A.F.WaveReward[static_cast<size_t>(M)];
		A.Steps(1);
		const std::vector<Pc::PcFieldEvent> K = A.Of(Pc::PcFieldEventKind::Killed);
		REQUIRE(K.size() == 1);
		CHECK(A.F.PrismShards == 1.0);
		// What the kill credited is the reward x Spectrum's multiplier when owned, through AddCurrency (which rounds).
		CHECK(K[0].Value == doctest::Approx(Credited(D, Before, Base * Pc::PrismKillMultiplier(D, Before))));
		return Pc::PrismKillMultiplier(D, Before);
	};
	CHECK(BeamKillReward(false) == 1.0);
	CHECK(BeamKillReward(true) > 1.0);

	Extra A(D);
	A.P.BranchLevels["unlock_aura"] = 1;
	A.P.BranchLevels["unlock_prism"] = 1;
	A.P.BranchLevels["critical_mass"] = 1;
	A.Refresh();
	A.F.Stats.AttackSpeedMultiplier = 0.0;
	const double Shards = D.Code.PrismShardsPerDetonation;
	for (int K = 0; K < static_cast<int>(Shards); ++K)
	{
		A.Mouse("alley_mouse", 900.0 + K, 450.0, 1e-9); // the beam kills one a step
	}
	const int32_t Witness = A.Mouse("alley_mouse", 1500.0, 850.0);
	A.Steps(static_cast<int>(Shards) - 1);
	CHECK(A.Of(Pc::PcFieldEventKind::PrismBurst).empty());
	const double Before = A.Hp(Witness);
	A.Steps(1);
	REQUIRE(A.Of(Pc::PcFieldEventKind::PrismBurst).size() == 1);
	CHECK(A.F.PrismShards == 0.0);
	CHECK(Before - A.Hp(Witness) == doctest::Approx(Pc::AuraDps(D.Combat, A.F.Stats) * Pc::PrismShardPayload(D, A.P)).epsilon(1e-3));
}

#endif
