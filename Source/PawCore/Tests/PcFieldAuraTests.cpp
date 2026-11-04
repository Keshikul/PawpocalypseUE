#if defined(PAWCORE_STANDALONE_TESTS)

// PcField's aura, Snare and zones (phase 2, slice 3). Their pure parts are checked against the real Godot code
// (PcAuraTests.cpp); these pin what only a running field shows, each from the Godot code and its comments.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcData.h"
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

	// A quiet field (no spawner, no paw: attack speed is zeroed), the cat at (CatX, CatY), the aura owned.
	struct Aura
	{
		const Pc::PcGameData& D;
		Pc::PcProgress P;
		Pc::PcField F;
		Pc::PcFieldInput In;

		explicit Aura(const Pc::PcGameData& Data, bool bOwned = true, double CatX = 800.0, double CatY = 450.0) : D(Data)
		{
			P.CurrentBiomeId = "tier_2";
			if (bOwned)
			{
				P.BranchLevels["unlock_aura"] = 1;
			}
			Pc::BeginField(F, D, P, "tier_2", Arena, 9);
			F.SpawnTimeLeft = 1e18;
			In.CatX = CatX;
			In.CatY = CatY;
			F.CatX = CatX;
			F.CatY = CatY;
			Refresh();
		}

		// The paw is silenced (its strikes would muddy every damage number here).
		void Refresh()
		{
			Pc::RefreshFieldStats(F, D, P);
			F.Stats.AttackSpeedMultiplier = 0.0;
		}

		void Steps(int N)
		{
			for (int K = 0; K < N; ++K)
			{
				Pc::StepField(F, D, P, In, Step);
			}
		}

		int32_t Mouse(const char* Id, double X, double Y, double Hp = 1e9)
		{
			const int32_t S = Pc::SpawnEnemyAt(F, D, P, Id, X, Y, 1.0, 0.0);
			REQUIRE(S >= 0);
			F.SpeedMult[static_cast<size_t>(S)] = 0.0; // held in place
			F.Hp[static_cast<size_t>(S)] = Hp;
			F.LifetimeLeft[static_cast<size_t>(S)] = 1e9;
			return S;
		}

		double Hp(int32_t S) const { return F.Hp[static_cast<size_t>(S)]; }

		size_t Count(Pc::PcFieldEventKind Kind) const
		{
			return static_cast<size_t>(std::count_if(F.Events.begin(), F.Events.end(), [Kind](const Pc::PcFieldEvent& E) { return E.Kind == Kind; }));
		}
	};
}

TEST_CASE("Aura: contact damage inside the radius only, and nothing at all before the aura is bought")
{
	const Pc::PcGameData& D = Game();
	Aura A(D);
	const double Radius = Pc::AuraRadius(D, A.P);
	REQUIRE(Pc::AuraDps(D.Combat, A.F.Stats) > 0.0);
	REQUIRE(A.F.Stats.AttackSpeedMultiplier == 0.0);
	const int32_t In = A.Mouse("alley_dog", 800.0 + Radius * 0.9, 450.0);
	const int32_t Out = A.Mouse("alley_dog", 800.0 + Radius * 1.1, 450.0);
	A.F.InvulnTimer[static_cast<size_t>(In)] = -1e9; // (the dog has no shield cycle; kept harmless)
	A.F.InvulnTimer[static_cast<size_t>(Out)] = -1e9;
	const double Before = A.Hp(In);
	A.Steps(1);
	CHECK(Before - A.Hp(In) == doctest::Approx(Pc::AuraDps(D.Combat, A.F.Stats) * Step).epsilon(1e-5)); // off 1e9 HP: the difference keeps ~7 digits
	CHECK(A.Hp(Out) == 1e9);

	// Unowned: no damage, and not even the Snare's slow.
	Aura N(D, false);
	N.P.BranchLevels["unlock_snare"] = 1;
	N.Refresh();
	const int32_t M = N.Mouse("alley_mouse", 810.0, 450.0);
	N.Steps(1);
	CHECK(N.Hp(M) == 1e9);
	CHECK(N.F.SlowMult[static_cast<size_t>(M)] == 1.0);
}

TEST_CASE("Aura: Crowd Work counts the crowd before any damage lands")
{
	const Pc::PcGameData& D = Game();
	Aura A(D);
	A.P.BranchLevels["job_town_crier"] = 1;
	A.P.BranchLevels["crowd_work"] = 1;
	A.Refresh();
	const double Per = Pc::AuraCrowdScaling(D, A.P);
	REQUIRE(Per > 0.0);
	std::vector<int32_t> Mice;
	for (int I = 0; I < 4; ++I)
	{
		Mice.push_back(A.Mouse("alley_mouse", 800.0 + 5.0 * I, 450.0));
	}
	A.Mouse("alley_mouse", 1500.0, 850.0); // outside: not part of the crowd
	A.Steps(1);
	const double Each = Pc::AuraDps(D.Combat, A.F.Stats) * (1.0 + Per * 4.0) * Step;
	for (const int32_t S : Mice)
	{
		CHECK(1e9 - A.Hp(S) == doctest::Approx(Each).epsilon(1e-5)); // off 1e9 HP: the difference keeps ~7 digits // the first reached is hit for the whole crowd too
	}
}

TEST_CASE("Aura: Aura Slow holds while in range, clears at the first move without it, and Traps hits the slowed harder")
{
	const Pc::PcGameData& D = Game();
	Aura A(D);
	A.P.CardLevels["rigor"] = 3; // 0.45: the best (lowest) level
	A.Refresh();
	const int32_t M = A.Mouse("alley_mouse", 820.0, 450.0);
	A.F.SpeedMult[static_cast<size_t>(M)] = 1.0;
	A.Steps(1);
	CHECK(A.F.SlowMult[static_cast<size_t>(M)] == 0.45); // applied, and kept through this step's move
	A.Steps(5);
	CHECK(A.F.SlowMult[static_cast<size_t>(M)] == 0.45); // re-applied every step it stays in range
	{
		const double X0 = A.F.X[static_cast<size_t>(M)], Y0 = A.F.Y[static_cast<size_t>(M)];
		A.Steps(1);
		const double Pace = D.Code.EnemySpeedPxPerSec * 0.45 * D.FindEnemy("alley_mouse")->SpeedMultiplier * Step;
		CHECK(std::hypot(A.F.X[static_cast<size_t>(M)] - X0, A.F.Y[static_cast<size_t>(M)] - Y0) == doctest::Approx(Pace));
	}
	// The cat leaves: the next move finds the slow stale and the mouse speeds back up.
	A.In.CatX = 100.0;
	A.In.CatY = 100.0;
	A.Steps(1);
	CHECK(A.F.SlowMult[static_cast<size_t>(M)] == 1.0);

	// Traps: a snared mouse takes more from every blow (here the aura's own).
	Aura T(D);
	T.P.CardLevels["rigor"] = 1;
	T.P.BranchLevels["job_ratcatcher"] = 1;
	T.P.BranchLevels["traps"] = 10;
	T.Refresh();
	const double Traps = Pc::SnaredDamageMultiplier(D, T.P);
	REQUIRE(Traps > 1.0);
	const int32_t S = T.Mouse("alley_mouse", 820.0, 450.0);
	T.Steps(1);
	CHECK(1e9 - T.Hp(S) == doctest::Approx(Pc::AuraDps(D.Combat, T.F.Stats) * Traps * Step).epsilon(1e-5)); // off 1e9 HP: the difference keeps ~7 digits
}

TEST_CASE("Aura: the Snare slows every ground mouse arena-wide, never a flier; Gridlock reaches fliers over the snared")
{
	const Pc::PcGameData& D = Game();
	Aura A(D, true, 100.0, 100.0);
	A.P.BranchLevels["unlock_snare"] = 1;
	A.P.BranchLevels["gridlock"] = 1;
	A.Refresh();
	const double Snare = Pc::SnareMultiplier(D, A.P);
	const double Grid = Pc::GridlockMultiplier(D, A.P);
	REQUIRE(Snare < 1.0);
	REQUIRE(Grid < 1.0);
	const int32_t Ground = A.Mouse("alley_mouse", 1200.0, 600.0);  // far from the aura: the Snare still reaches it
	const int32_t Over = A.Mouse("alley_flying_mouse", 1250.0, 600.0); // within Gridlock's reach of a ground mouse
	const int32_t Clear = A.Mouse("alley_flying_mouse", 1500.0, 150.0); // nobody under it
	A.Steps(1);
	CHECK(A.F.SlowMult[static_cast<size_t>(Ground)] == Snare);
	CHECK(A.F.SlowMult[static_cast<size_t>(Over)] == Grid);
	CHECK(A.F.SlowMult[static_cast<size_t>(Clear)] == 1.0);

	// Deadweight REPLACES the Snare's slow rather than stacking on it.
	A.P.BranchLevels["deadweight"] = 10;
	A.Refresh();
	A.Steps(1);
	CHECK(A.F.SlowMult[static_cast<size_t>(Ground)] == doctest::Approx(1.0 - 0.8));
}

TEST_CASE("Aura: an aura kill's Overkill spills from where the victim died, onto the mouse beside it (F16)")
{
	const Pc::PcGameData& D = Game();
	Aura A(D);
	A.P.UpgradeLevels["overkill"] = 10;
	A.Refresh();
	REQUIRE(A.F.Stats.OverkillFraction > 0.0);
	const int32_t Victim = A.Mouse("alley_mouse", 800.0, 450.0, 1e-9);
	const int32_t Neighbour = A.Mouse("alley_mouse", 805.0, 450.0); // beside the corpse, in the aura too
	const int32_t Corner = A.Mouse("alley_mouse", 40.0, 40.0);      // far away (the old spill's target: nearest the parking spot)
	A.Steps(1);
	REQUIRE_FALSE(A.F.Active[static_cast<size_t>(Victim)]);
	const auto Splash = std::find_if(A.F.Events.begin(), A.F.Events.end(), [](const Pc::PcFieldEvent& E) { return E.Kind == Pc::PcFieldEventKind::Splash; });
	REQUIRE(Splash != A.F.Events.end());
	CHECK(Splash->Slot == Neighbour);
	CHECK(Splash->X == 800.0);
	CHECK(A.Hp(Corner) == 1e9); // untouched
	// The neighbour took its own aura damage AND the spill.
	CHECK(1e9 - A.Hp(Neighbour) > Pc::AuraDps(D.Combat, A.F.Stats) * Step * (1.0 + 1e-6));
}

TEST_CASE("Zones: the trail drops only when the cat has moved, half the aura wide, hurts the ground, and expires")
{
	const Pc::PcGameData& D = Game();
	Aura A(D, true, 200.0, 200.0);
	A.P.CardLevels["ember_trail"] = 4;
	A.Refresh();
	const double Fraction = Pc::CardEffectValue(D, A.P, Pc::CardEffect::AURA_TRAIL);
	REQUIRE(Fraction > 0.0);
	// Parked: the first drop lands at the cadence, then no more (a parked cursor must not stack a second aura).
	const int Cadence = static_cast<int>(std::ceil(D.Code.AuraTrailDropSec / Step));
	A.Steps(Cadence + 1);
	REQUIRE(A.F.Zones.size() == 1);
	CHECK(A.F.Zones[0].Radius == doctest::Approx(Pc::AuraRadius(D, A.P) * 0.5));
	CHECK(A.F.Zones[0].PowerFraction == Fraction);
	CHECK(A.F.Zones[0].Kind == Pc::PcZoneKind::Trail);
	A.Steps(Cadence * 3);
	CHECK(A.F.Zones.size() == 1);
	// Moving: a drop per cadence.
	for (int K = 0; K < 3; ++K)
	{
		A.In.CatX += D.Code.AuraTrailMinMovePx + 30.0;
		A.Steps(Cadence + 1);
	}
	CHECK(A.F.Zones.size() >= 3);
	// A ground mouse standing on a zone takes PlayerDps x fraction x step from it; a flier there takes nothing.
	Aura Z(D, true, 1500.0, 850.0);
	Z.P.CardLevels["ember_trail"] = 4;
	Z.Refresh();
	Z.F.Stats.AttackSpeedMultiplier = 1.0; // the paw counts in the player's DPS; nothing is in its reach
	REQUIRE(Pc::PlayerDps(D.Combat, Z.F.Stats) > Pc::AuraDps(D.Combat, Z.F.Stats));
	Pc::PcDamageZone Patch;
	Patch.X = 300.0;
	Patch.Y = 300.0;
	Patch.Radius = 60.0;
	Patch.PowerFraction = Fraction;
	Patch.Remaining = 2.0 * Step + 1e-9;
	Patch.Duration = 1.0;
	Z.F.Zones.push_back(Patch);
	Z.F.TrailElapsed = -1e9; // no new drops during the test
	const int32_t G = Z.Mouse("alley_mouse", 310.0, 300.0);
	const int32_t Fl = Z.Mouse("alley_flying_mouse", 300.0, 310.0);
	Z.Steps(1);
	CHECK(1e9 - Z.Hp(G) == doctest::Approx(Pc::PlayerDps(D.Combat, Z.F.Stats) * Fraction * Step).epsilon(1e-6));
	CHECK(Z.Hp(Fl) == 1e9);
	Z.Steps(2);
	CHECK(Z.F.Zones.empty());
}

TEST_CASE("Zones: at most max_damage_zones, the oldest dropped first")
{
	const Pc::PcGameData& D = Game();
	Aura A(D, true, 100.0, 100.0);
	A.P.CardLevels["ember_trail"] = 1;
	A.Refresh();
	// The trail lives 2.5 s and drops every 0.2 s, so it never reaches the ceiling by itself: the test forces a drop
	// every step and keeps every zone alive, so only the ceiling can remove one. Each zone is marked by where it fell.
	for (int K = 0; K < D.Code.MaxDamageZones + 5; ++K)
	{
		A.F.TrailElapsed = 1e9;
		A.In.CatX = 100.0 + 25.0 * K;
		A.Steps(1);
		for (Pc::PcDamageZone& Z : A.F.Zones)
		{
			Z.Remaining = 1e9; // nothing expires: only the ceiling removes
		}
	}
	CHECK(static_cast<int64_t>(A.F.Zones.size()) == D.Code.MaxDamageZones);
	CHECK(A.F.Zones.front().X == doctest::Approx(100.0 + 25.0 * 5)); // the first five went, oldest first
}

TEST_CASE("Zones: Echo drops ONE full-size zone at the cat per beat, living exactly one beat")
{
	const Pc::PcGameData& D = Game();
	Aura A(D);
	A.P.BranchLevels["job_town_crier"] = 1;
	A.P.BranchLevels["echo"] = 5;
	A.Refresh();
	const double Power = Pc::AuraEchoPower(D, A.P);
	REQUIRE(Power > 0.0);
	const int Beat = static_cast<int>(std::ceil(D.Code.EchoDropSec / Step));
	for (int K = 0; K < 5 * Beat; ++K)
	{
		A.Steps(1);
		CHECK(A.F.Zones.size() <= 1);
	}
	REQUIRE(A.F.Zones.size() == 1);
	CHECK(A.F.Zones[0].Kind == Pc::PcZoneKind::Echo);
	CHECK(A.F.Zones[0].Radius == doctest::Approx(Pc::AuraRadius(D, A.P)));
	CHECK(A.F.Zones[0].PowerFraction == Power);
}

TEST_CASE("Edge Burn: the band along the rim burns at aura DPS x the card; the middle does not")
{
	const Pc::PcGameData& D = Game();
	Aura A(D, true, -5000.0, -5000.0); // the cat off the field: no aura contact
	A.P.CardLevels["hot_borders"] = 3;
	A.Refresh();
	A.F.Stats.AttackSpeedMultiplier = 1.0; // the paw counts in the player's DPS, which Edge Burn must NOT read
	REQUIRE(Pc::PlayerDps(D.Combat, A.F.Stats) > Pc::AuraDps(D.Combat, A.F.Stats));
	const double Fraction = Pc::CardEffectValue(D, A.P, Pc::CardEffect::EDGE_BURN);
	const int32_t Rim = A.Mouse("alley_mouse", Arena.X + D.Code.EdgeBurnBandPx * 0.5, 450.0);
	const int32_t Middle = A.Mouse("alley_mouse", 800.0, 450.0);
	A.Steps(1);
	CHECK(1e9 - A.Hp(Rim) == doctest::Approx(Pc::AuraDps(D.Combat, A.F.Stats) * Fraction * Step).epsilon(1e-6));
	CHECK(A.Hp(Middle) == 1e9);
}

TEST_CASE("Aura pulse: every interval the aura detonates for (card + Standing Wave) seconds of aura DPS")
{
	const Pc::PcGameData& D = Game();
	Aura A(D, true, 800.0, 450.0);
	A.P.CardLevels["thunderclap"] = 1;
	A.P.BranchLevels["standing_wave"] = 1;
	A.Refresh();
	const double Seconds = Pc::CardEffectValue(D, A.P, Pc::CardEffect::AURA_PULSE) + Pc::AuraBeatSeconds(D, A.P);
	REQUIRE(Seconds > Pc::CardEffectValue(D, A.P, Pc::CardEffect::AURA_PULSE));
	const int32_t M = A.Mouse("alley_dog", 820.0, 450.0);
	A.F.InvulnTimer[static_cast<size_t>(M)] = -1e9;
	const int Interval = static_cast<int>(std::ceil(D.Code.AuraPulseIntervalSec / Step));
	A.Steps(Interval - 2);
	CHECK(A.Count(Pc::PcFieldEventKind::AuraPulse) == 0);
	const double Before = A.Hp(M);
	A.Steps(3);
	CHECK(A.Count(Pc::PcFieldEventKind::AuraPulse) == 1);
	const double Contact = Pc::AuraDps(D.Combat, A.F.Stats) * Step * 3.0;
	CHECK(Before - A.Hp(M) == doctest::Approx(Pc::AuraDps(D.Combat, A.F.Stats) * Seconds + Contact).epsilon(1e-6));
}

TEST_CASE("Rainbow Poop: a zone under the cat each cooldown, poop DPS without Shield Pierce; Poop Trail and Duration")
{
	const Pc::PcGameData& D = Game();
	Aura A(D, false, 800.0, 450.0);
	A.P.BranchLevels["unlock_unicorn"] = 1;
	A.P.BranchLevels["unlock_poop"] = 1;
	A.P.CardLevels["lingering_rainbow"] = 1;
	A.Refresh();
	REQUIRE(Pc::PoopUnlocked(A.P));
	const int32_t M = A.Mouse("alley_mouse", 820.0, 450.0);
	const int Cooldown = static_cast<int>(std::ceil(D.Combat.PoopCooldownSec / Step));
	A.Steps(Cooldown - 2);
	CHECK(A.Count(Pc::PcFieldEventKind::PoopDropped) == 0);
	A.Steps(3); // the timer fires within a step of the cooldown (its summed clock)
	REQUIRE(A.Count(Pc::PcFieldEventKind::PoopDropped) == 1);
	const auto Drop = std::find_if(A.F.Events.begin(), A.F.Events.end(), [](const Pc::PcFieldEvent& E) { return E.Kind == Pc::PcFieldEventKind::PoopDropped; });
	CHECK(Drop->ToX == doctest::Approx(D.Combat.PoopDurationSec + 3.0)); // Lingering Rainbow adds its seconds
	CHECK(Drop->Value == D.Combat.PoopRadiusPx);
	const double Before = A.Hp(M);
	A.Steps(1);
	CHECK(Before - A.Hp(M) == doctest::Approx(Pc::PoopDps(D.Combat, A.F.Stats) * Step).epsilon(1e-6));

	// Rainbow Trail shortens the cycle, from the NEXT one (a Godot Timer restarts with its old wait before it signals).
	Aura T(D, false, 800.0, 450.0);
	T.P.BranchLevels["unlock_unicorn"] = 1;
	T.P.BranchLevels["unlock_poop"] = 1;
	T.P.CardLevels["rainbow_trail"] = 3; // 1.5 s
	T.Refresh();
	T.Steps(Cooldown + 1);
	CHECK(T.Count(Pc::PcFieldEventKind::PoopDropped) == 1); // the first after the full cooldown
	T.Steps(Cooldown + 1);
	CHECK(T.Count(Pc::PcFieldEventKind::PoopDropped) == 2); // the second still on the old wait
	T.F.Events.clear();
	T.Steps(static_cast<int>(std::ceil(1.5 / Step)) + 1);
	CHECK(T.Count(Pc::PcFieldEventKind::PoopDropped) == 1); // then every 1.5 s

	// Without the Unicorn: the timer runs, nothing drops.
	Aura N(D, false);
	N.Steps(Cooldown * 2);
	CHECK(N.Count(Pc::PcFieldEventKind::PoopDropped) == 0);
}

TEST_CASE("Stillness: the cat holding still fills Carry and Long Lunch's bank; moving empties Carry only")
{
	const Pc::PcGameData& D = Game();
	Aura A(D);
	A.P.BranchLevels["job_town_crier"] = 1;
	A.P.BranchLevels["carry"] = 10;
	A.Refresh();
	const double Flat = Pc::AuraRadius(D, A.P);
	A.Steps(static_cast<int>(std::ceil(D.Code.CarryFullSeconds / Step)) + 2);
	CHECK(A.P.PlayerStillSeconds >= D.Code.CarryFullSeconds);
	CHECK(Pc::AuraRadius(D, A.P) > Flat);
	CHECK(Pc::CarryRadiusBonus(D, A.P) == doctest::Approx(Pc::BranchEffect(D, A.P, "carry")));
	// A mouse just past the flat radius is reached once the cat has held still.
	const int32_t M = A.Mouse("alley_mouse", 800.0 + Flat * 1.02, 450.0);
	A.Steps(1);
	CHECK(A.Hp(M) < 1e9);
	const double Lunch = A.P.LunchSeconds;
	A.In.CatX += 50.0;
	A.Steps(1);
	CHECK(A.P.PlayerStillSeconds == 0.0);
	CHECK(A.P.LunchSeconds == Lunch); // only a kill spends the bank
	CHECK(Pc::AuraRadius(D, A.P) == doctest::Approx(Flat));
}

#endif
