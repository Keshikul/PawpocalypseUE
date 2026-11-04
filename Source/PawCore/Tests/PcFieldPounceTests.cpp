#if defined(PAWCORE_STANDALONE_TESTS)

// PcField's Pounce (phase 2, slice 4). Its pure parts are checked against the real Godot code (PcPounceTests.cpp); these
// pin the cast itself, the press edge, Reflex, the Cone of Shame and the Pounce nodes and cards, from the Godot code.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcJobs.h"

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

	// A quiet field (no spawner), the cat in the middle, Pounce owned unless told otherwise. The paw keeps its speed: Pounce
	// is priced off the player's DPS, paw included, so the tests put their mice beyond the paw's reach (62 px) and inside
	// the Pounce's radius (160 px).
	struct Pouncer
	{
		const Pc::PcGameData& D;
		Pc::PcProgress P;
		Pc::PcField F;
		Pc::PcFieldInput In;

		explicit Pouncer(const Pc::PcGameData& Data, bool bOwned = true) : D(Data)
		{
			P.CurrentBiomeId = "tier_2";
			if (bOwned)
			{
				P.BranchLevels["unlock_pounce"] = 1;
			}
			Pc::BeginField(F, D, P, "tier_2", Arena, 13);
			F.SpawnTimeLeft = 1e18;
			In.CatX = 800.0;
			In.CatY = 450.0;
			F.CatX = 800.0;
			F.CatY = 450.0;
			Refresh();
		}

		void Refresh()
		{
			Pc::RefreshFieldStats(F, D, P);
		}

		void Steps(int N)
		{
			for (int K = 0; K < N; ++K)
			{
				Pc::StepField(F, D, P, In, Step);
			}
		}

		// One press: down for a step, then up for a step.
		void Press()
		{
			In.bPounceDown = true;
			Steps(1);
			In.bPounceDown = false;
			Steps(1);
		}

		int32_t Mouse(const char* Id, double X, double Y, double Hp = 1e9)
		{
			const int32_t S = Pc::SpawnEnemyAt(F, D, P, Id, X, Y, 1.0, 0.0);
			REQUIRE(S >= 0);
			F.SpeedMult[static_cast<size_t>(S)] = 0.0;
			F.Hp[static_cast<size_t>(S)] = Hp;
			F.LifetimeLeft[static_cast<size_t>(S)] = 1e9;
			F.InvulnTimer[static_cast<size_t>(S)] = -1e9; // a signature's shield never turns over during a test
			return S;
		}

		double Hp(int32_t S) const { return F.Hp[static_cast<size_t>(S)]; }

		size_t Pounces() const
		{
			return static_cast<size_t>(std::count_if(F.Events.begin(), F.Events.end(), [](const Pc::PcFieldEvent& E) { return E.Kind == Pc::PcFieldEventKind::Pounce; }));
		}
	};

	// The burst at x1 on a target with no open window, against the stats the field holds.
	double Burst(const Pouncer& A, bool bVulnerable = false, double Scale = 1.0)
	{
		const double B = Pc::PounceDamage(A.D.Combat, A.F.Stats, bVulnerable, Scale);
		REQUIRE(B > 0.0); // a burst of zero would make every damage check below trivially true
		return B;
	}
}

TEST_CASE("Pounce: nothing before it is bought: no cast, no cooldown, no Reflex")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D, false);
	A.P.BranchLevels["unlock_reflex"] = 1;
	A.Refresh();
	const int32_t M = A.Mouse("alley_mouse", 920.0, 450.0);
	A.Press();
	A.Steps(60);
	CHECK(A.Pounces() == 0);
	CHECK(A.Hp(M) == 1e9);
	CHECK(A.F.PounceCooldownLeft == 0.0);
}

TEST_CASE("Pounce: a press fires once into the radius; a held button never re-fires; a press on cooldown is spent")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	const int32_t In = A.Mouse("alley_mouse", 800.0 + D.Combat.PounceRadiusPx * 0.9, 450.0);
	const int32_t Out = A.Mouse("alley_mouse", 800.0 + D.Combat.PounceRadiusPx * 1.1, 450.0);
	A.In.bPounceDown = true;
	A.Steps(1);
	CHECK(A.Pounces() == 1);
	CHECK(1e9 - A.Hp(In) == doctest::Approx(Burst(A)).epsilon(1e-5));
	CHECK(A.Hp(Out) == 1e9);
	CHECK(A.F.PounceCooldownLeft == doctest::Approx(Pc::PounceCooldown(D.Combat, A.F.Stats) - Step).epsilon(1e-9));
	CHECK(Pc::PounceCooldownFraction(A.F) < 0.05);
	// Held through the whole cooldown and past it: still one.
	A.Steps(static_cast<int>(std::ceil(Pc::PounceCooldown(D.Combat, A.F.Stats) / Step)) + 30);
	CHECK(A.Pounces() == 1);
	CHECK(Pc::PounceCooldownFraction(A.F) == 1.0);
	// Released and pressed again: a second.
	A.In.bPounceDown = false;
	A.Steps(1);
	A.Press();
	CHECK(A.Pounces() == 2);
	// A press during the cooldown is not queued for later.
	A.Press();
	A.Steps(static_cast<int>(std::ceil(Pc::PounceCooldown(D.Combat, A.F.Stats) / Step)) + 30);
	CHECK(A.Pounces() == 2);
}

TEST_CASE("Pounce: the vulnerable bonus lands only on a target whose window is open")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	const int32_t Open = A.Mouse("park_goose", 920.0, 450.0);    // a signature with a shield cycle, vulnerable now
	const int32_t Shut = A.Mouse("park_goose", 680.0, 450.0);    // the same, shielded
	A.F.Shielded[static_cast<size_t>(Shut)] = 1;
	const int32_t Common = A.Mouse("alley_mouse", 800.0, 570.0); // no window at all
	REQUIRE(Burst(A, true) > Burst(A));
	A.Press();
	CHECK(1e9 - A.Hp(Open) == doctest::Approx(Burst(A, true)).epsilon(1e-5));
	CHECK(A.Hp(Shut) == 1e9);
	CHECK(1e9 - A.Hp(Common) == doctest::Approx(Burst(A)).epsilon(1e-5));
}

TEST_CASE("Pounce: Reflex fires the instant it is ready, at its own multiplier")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.BranchLevels["unlock_reflex"] = 1;
	A.Refresh();
	const double Mult = Pc::AutoPounceMultiplier(D, A.P);
	REQUIRE(Mult > 1.0);
	const int32_t M = A.Mouse("alley_mouse", 920.0, 450.0);
	A.Steps(1);
	REQUIRE(A.Pounces() == 1);
	const auto E = std::find_if(A.F.Events.begin(), A.F.Events.end(), [](const Pc::PcFieldEvent& V) { return V.Kind == Pc::PcFieldEventKind::Pounce; });
	CHECK((E->Flags & Pc::FieldFlag::Auto) != 0);
	CHECK(1e9 - A.Hp(M) == doctest::Approx(Burst(A, false, Mult)).epsilon(1e-5));
	A.Steps(static_cast<int>(std::ceil(Pc::PounceCooldown(D.Combat, A.F.Stats) / Step)) + 2);
	CHECK(A.Pounces() == 2); // and again as soon as it is back
}

TEST_CASE("Pounce: the Daredevil's Cone of Shame roots the cat, and a rooted cat is not 'still'")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.RunJob = "daredevil";
	A.Refresh();
	const double Lock = Pc::JobPounceLockSec(D, A.P);
	REQUIRE(Lock > 0.0);
	A.Steps(30); // some stillness banked first
	const double Still = A.P.PlayerStillSeconds;
	REQUIRE(Still > 0.0);
	A.Press();
	A.In.CatX = 300.0; // the player tries to leave
	A.Steps(static_cast<int>(Lock / Step) - 4);
	CHECK(A.F.CatX == 800.0);
	// The press step still counted (the cat stood still, then pounced); from then on frozen, neither growing nor reset.
	CHECK(A.P.PlayerStillSeconds == doctest::Approx(Still + Step));
	A.Steps(8);
	CHECK(A.F.CatX == 300.0);
	CHECK(A.P.PlayerStillSeconds < Still); // the move after the root reset it
}

TEST_CASE("Pounce: Nine Lives Insurance refunds a root that killed nothing, not one that killed")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.RunJob = "daredevil";
	A.P.BranchLevels["nine_lives_insurance"] = 1;
	A.Refresh();
	const double Lock = Pc::JobPounceLockSec(D, A.P);
	A.Press();
	A.Steps(static_cast<int>(Lock / Step) + 2);
	CHECK(A.F.PounceCooldownLeft == 0.0);
	// The same with a kill in the window: the cooldown keeps running.
	Pouncer B(D);
	B.P.RunJob = "daredevil";
	B.P.BranchLevels["nine_lives_insurance"] = 1;
	B.Refresh();
	B.Mouse("alley_mouse", 920.0, 450.0, 1e-6);
	B.Press();
	B.Steps(static_cast<int>(Lock / Step) + 2);
	CHECK(B.F.PounceCooldownLeft > 0.0);
}

TEST_CASE("Pounce: Crash Landing turns the root into a crater; the crater card leaves a patch where it landed")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.RunJob = "daredevil";
	A.P.BranchLevels["crash_landing"] = 5;
	A.P.BranchLevels["unlock_aura"] = 1;
	A.Refresh();
	A.In.bPounceDown = true;
	A.Steps(1);
	REQUIRE(A.F.Zones.size() == 1);
	CHECK(A.F.Zones[0].Kind == Pc::PcZoneKind::Crater);
	CHECK(A.F.Zones[0].Radius == doctest::Approx(Pc::AuraRadius(D, A.P)));
	CHECK(A.F.Zones[0].PowerFraction == Pc::PounceCraterPower(D, A.P));
	CHECK(A.F.Zones[0].Duration == Pc::JobPounceLockSec(D, A.P));

	Pouncer B(D);
	B.P.CardLevels["scorched_landing"] = 2;
	B.Refresh();
	B.In.bPounceDown = true;
	B.Steps(1);
	REQUIRE(B.F.Zones.size() == 1); // no root without the Daredevil: only the card's patch
	CHECK(B.F.Zones[0].Radius == D.Combat.PounceRadiusPx);
	CHECK(B.F.Zones[0].PowerFraction == Pc::CardEffectValue(D, B.P, Pc::CardEffect::POUNCE_CRATER));
	CHECK(B.F.Zones[0].Duration == D.Code.PounceCraterLifetimeSec);
}

TEST_CASE("Pounce: Chain reaches the nearest mice beyond the rim, nearest first, without the window bonus")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.CardLevels["chain_pounce"] = 2; // two more
	A.Refresh();
	const double R = D.Combat.PounceRadiusPx;
	const int32_t Far = A.Mouse("alley_mouse", 800.0 + R + 300.0, 450.0);
	const int32_t Near = A.Mouse("park_goose", 800.0 + R + 50.0, 450.0); // open window, yet no bonus on a chain
	const int32_t Mid = A.Mouse("alley_mouse", 800.0, 450.0 + R + 120.0);
	const int32_t Inside = A.Mouse("alley_mouse", 900.0, 450.0); // the blast hits it once; the chain must not again
	A.In.bPounceDown = true;
	A.Steps(1);
	CHECK(1e9 - A.Hp(Inside) == doctest::Approx(Burst(A)).epsilon(1e-5));
	CHECK(1e9 - A.Hp(Near) == doctest::Approx(Burst(A)).epsilon(1e-5));
	CHECK(1e9 - A.Hp(Mid) == doctest::Approx(Burst(A)).epsilon(1e-5));
	CHECK(A.Hp(Far) == 1e9);
}

TEST_CASE("Pounce: Kitten Mimic Pounce hits each kitten's own target for a share")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.CardLevels["peer_pressure"] = 1;
	A.P.UpgradeLevels["kitten_count"] = 2;
	A.Refresh();
	A.F.CatX = 100.0; // the kittens wake far from their prey (no contact this step)
	A.F.CatY = 100.0;
	Pc::SyncFieldKittens(A.F, D, A.P);
	A.F.CatX = 800.0;
	A.F.CatY = 450.0;
	const int32_t T1 = A.Mouse("alley_mouse", 1400.0, 800.0);
	const int32_t T2 = A.Mouse("alley_mouse", 1400.0, 120.0);
	const int32_t Free = A.Mouse("alley_mouse", 300.0, 800.0);
	size_t K = 0;
	for (size_t I = 0; I < A.F.KActive.size(); ++I)
	{
		if (A.F.KActive[I])
		{
			const int32_t T = K++ == 0 ? T1 : T2;
			A.F.KTarget[I] = T;
			A.F.KTargetSerial[I] = A.F.Serial[static_cast<size_t>(T)];
		}
	}
	REQUIRE(K == 2);
	const double Share = Pc::CardEffectValue(D, A.P, Pc::CardEffect::KITTEN_MIMIC_POUNCE);
	A.In.bPounceDown = true;
	A.Steps(1);
	CHECK(1e9 - A.Hp(T1) == doctest::Approx(Burst(A) * Share).epsilon(1e-5));
	CHECK(1e9 - A.Hp(T2) == doctest::Approx(Burst(A) * Share).epsilon(1e-5));
	CHECK(A.Hp(Free) == 1e9); // nobody's target
}

TEST_CASE("Pounce: Bloodlust's kills shave the cooldown (floored at zero); Momentum's shave the root")
{
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.CardLevels["bloodlust"] = 4;
	A.P.RunJob = "daredevil";
	A.P.BranchLevels["momentum"] = 10;
	A.Refresh();
	const double Refund = Pc::CardEffectValue(D, A.P, Pc::CardEffect::POUNCE_KILL_REFUND);
	const double LockRefund = Pc::PounceLockRefund(D, A.P);
	for (int I = 0; I < 3; ++I)
	{
		A.Mouse("alley_mouse", 920.0 + 5.0 * I, 450.0, 1e-6);
	}
	A.In.bPounceDown = true;
	A.Steps(1);
	CHECK(A.F.PounceCooldownLeft == doctest::Approx(Pc::PounceCooldown(D.Combat, A.F.Stats) - 3.0 * Refund - Step).epsilon(1e-9));
	CHECK(A.F.PounceLockLeft == doctest::Approx(Pc::JobPounceLockSec(D, A.P) - 3.0 * LockRefund).epsilon(1e-9));
	// Floored at zero: enough kills cannot bank a negative cooldown.
	Pouncer B(D);
	B.P.CardLevels["bloodlust"] = 4;
	B.Refresh();
	const int Kills = static_cast<int>(std::ceil(Pc::PounceCooldown(D.Combat, B.F.Stats) / Refund)) + 3;
	for (int I = 0; I < Kills; ++I)
	{
		B.Mouse("alley_mouse", 920.0 + 0.5 * I, 450.0, 1e-6);
	}
	B.In.bPounceDown = true;
	B.Steps(1);
	CHECK(B.F.PounceCooldownLeft == 0.0);
}

TEST_CASE("Pounce: a pounce kill's Overkill spills from where the victim died, onto the mouse beside it (F16)")
{
	// F16, designer 2026-11-03 (fixed in Godot first): the spill used to start from the parking spot (-10000, -10000) and
	// land on the mouse nearest the top-left corner.
	const Pc::PcGameData& D = Game();
	Pouncer A(D);
	A.P.UpgradeLevels["overkill"] = 10;
	A.Refresh();
	A.Mouse("alley_mouse", 920.0, 450.0, 1e-6);
	const int32_t Beside = A.Mouse("alley_mouse", 1000.0, 450.0); // beside it (80 px), outside the blast
	const int32_t Corner = A.Mouse("alley_mouse", 30.0, 30.0);
	A.In.bPounceDown = true;
	A.Steps(1);
	const auto Splash = std::find_if(A.F.Events.begin(), A.F.Events.end(), [](const Pc::PcFieldEvent& E) { return E.Kind == Pc::PcFieldEventKind::Splash; });
	REQUIRE(Splash != A.F.Events.end());
	CHECK(Splash->Slot == Beside);
	CHECK(Splash->Slot != Corner);
	CHECK(Splash->X == 920.0);
	CHECK(Splash->Y == 450.0);
}

#endif
