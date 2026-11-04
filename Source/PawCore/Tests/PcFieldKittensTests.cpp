#if defined(PAWCORE_STANDALONE_TESTS)

// PcField's kittens (phase 2, slice 2). Their pure parts and one whole hunt are checked against the real Godot code
// (PcKittensTests.cpp); these pin what only a running field shows: the pool and when it is re-counted, the two-pass
// claim tally (Godot's own ci/check_kitten_spread.gd, ported), stickiness and rebalancing, contact damage, and the kitten
// cards and nodes, each from the Godot code and its comments.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcTree.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
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

	// A quiet field (no spawner), the cat parked far away (it strikes nothing), `Kittens` owned and awake on (WakeX, WakeY).
	struct Pack
	{
		const Pc::PcGameData& D;
		Pc::PcProgress P;
		Pc::PcField F;
		Pc::PcFieldInput In;

		Pack(const Pc::PcGameData& Data, int64_t Kittens, double WakeX = 800.0, double WakeY = 450.0, const char* Biome = "tier_1")
			: D(Data)
		{
			P.CurrentBiomeId = Biome;
			Pc::BeginField(F, D, P, Biome, Arena, 5);
			P.UpgradeLevels["kitten_count"] = Kittens; // owned after BeginField, so they wake where the test says
			F.SpawnTimeLeft = 1e18;
			In.CatX = -10000.0;
			In.CatY = -10000.0;
			Recount(WakeX, WakeY);
		}

		// The kittens are re-counted with the cat at (X, Y) (they wake on the cat).
		void Recount(double X, double Y)
		{
			F.CatX = X;
			F.CatY = Y;
			Pc::SyncFieldKittens(F, D, P);
		}

		void Steps(int N, double Dt = Step)
		{
			for (int K = 0; K < N; ++K)
			{
				Pc::StepField(F, D, P, In, Dt);
			}
		}

		int32_t Mouse(const char* Id, double X, double Y)
		{
			const int32_t S = Pc::SpawnEnemyAt(F, D, P, Id, X, Y, 1.0, 0.0);
			REQUIRE(S >= 0);
			return S;
		}

		// Holds a mouse where it is (the tests that measure kittens must not have their prey wander off).
		void Pin(int32_t S)
		{
			F.SpeedMult[static_cast<size_t>(S)] = 0.0;
		}

		std::vector<int32_t> AwakeIndices() const
		{
			std::vector<int32_t> Out;
			for (size_t K = 0; K < F.KActive.size(); ++K)
			{
				if (F.KActive[K])
				{
					Out.push_back(static_cast<int32_t>(K));
				}
			}
			return Out;
		}

		// The claim tally as the field's next step will see it.
		std::map<int32_t, int64_t> Claims() const
		{
			std::map<int32_t, int64_t> Out;
			for (size_t K = 0; K < F.KActive.size(); ++K)
			{
				const int32_t T = F.KTarget[K];
				if (F.KActive[K] && T >= 0 && F.Active[static_cast<size_t>(T)] && F.Serial[static_cast<size_t>(T)] == F.KTargetSerial[K])
				{
					++Out[T];
				}
			}
			return Out;
		}

		size_t Count(Pc::PcFieldEventKind Kind) const
		{
			return static_cast<size_t>(std::count_if(F.Events.begin(), F.Events.end(), [Kind](const Pc::PcFieldEvent& E) { return E.Kind == Kind; }));
		}
	};

	// The shipping data plus one test-only card: two kitten effects (KITTEN_DRAG, KITTEN_FROM_SIGNATURE) are in the code
	// but no shipping card grants them, so their rules are pinned with a card made for the test.
	Pc::PcGameData WithCard(const char* Id, Pc::CardEffect Effect, double Value)
	{
		Pc::PcGameData D = Game();
		Pc::PcCard C;
		C.Id = Id;
		C.Effect = Effect;
		C.LevelValues = { Value };
		D.Cards.push_back(C);
		return D;
	}
}

TEST_CASE("Kittens: the pool is sized from data, grows past it, wakes on the cat and sleeps from the front")
{
	const Pc::PcGameData& D = Game();
	Pack K(D, 0);
	const int64_t Base = Pc::KittenPoolBaseSize(D);
	CHECK(Base == 10 + Pc::MaxVitalityBonus(D) + D.Code.MaxTempKittens); // Kitten Count's cap 10 in the data
	CHECK(static_cast<int64_t>(K.F.KActive.size()) == Base);
	CHECK(K.F.AwakeKittens() == 0);

	// Three owned: the first three of the pool wake, on the cat.
	K.P.UpgradeLevels["kitten_count"] = 3;
	K.Recount(500.0, 400.0);
	CHECK(K.AwakeIndices() == std::vector<int32_t>{ 0, 1, 2 });
	CHECK(K.F.KX[2] == 500.0);
	CHECK(K.F.KY[2] == 400.0);
	// Each kitten's slot is its own evenly spaced angle over the WHOLE pool.
	double OX = 0.0, OY = 0.0;
	Pc::KittenOrbitSlot(D, 1, Base, OX, OY);
	CHECK(K.F.KOrbitX[1] == OX);
	CHECK(K.F.KOrbitY[1] == OY);
	CHECK(std::hypot(OX, OY) == doctest::Approx(D.Code.KittenOrbitRadiusPx));
	CHECK(D.Code.KittenOrbitRadiusPx < D.Combat.BaseKittenContactRadius); // orbiting still counts as contact

	// Ownership past the pool (the cap rises with depth and Reawakens): the pool grows and every slot is re-spread.
	K.P.DeepestBiomeIndex = 11;
	K.P.ReawakenCount = 20;
	K.P.UpgradeLevels["kitten_count"] = Base + 5;
	REQUIRE(Pc::KittenCount(D, K.P) == Base + 5);
	K.Recount(500.0, 400.0);
	CHECK(static_cast<int64_t>(K.F.KActive.size()) == Base + 5);
	CHECK(K.F.AwakeKittens() == Base + 5);
	Pc::KittenOrbitSlot(D, 1, Base + 5, OX, OY);
	CHECK(K.F.KOrbitX[1] == OX);

	// Down to one: kittens sleep from the FRONT of the pool (Godot's loop), so the last one stays awake.
	K.P.UpgradeLevels["kitten_count"] = 1;
	K.Recount(500.0, 400.0);
	CHECK(K.AwakeIndices() == std::vector<int32_t>{ static_cast<int32_t>(Base + 4) });
	CHECK(static_cast<int64_t>(K.F.KActive.size()) == Base + 5); // never shrinks
}

TEST_CASE("Kittens: the field re-counts its kittens when the game asks, never on its own (a card taken is one ask, F14)")
{
	// The field does not watch the progress: the game re-counts on a Kitten Count purchase, a tree level, a Reawaken,
	// travel and (F14, 2026-11-03) a card taken (UPawRealmSubsystem::ChooseCard; Auto-Card inside the field itself).
	const Pc::PcGameData& D = Game();
	Pack K(D, 5);
	CHECK(K.F.AwakeKittens() == 5);
	K.P.CardLevels["lone_hunter"] = 1; // costs 2 kittens, set behind the game's back
	REQUIRE(Pc::KittenCount(D, K.P) == 3);
	K.Steps(120);
	CHECK(K.F.AwakeKittens() == 5);
	K.Recount(800.0, 450.0);
	CHECK(K.F.AwakeKittens() == 3);
}

TEST_CASE("Kittens: a kitten-cost card Auto-Card takes re-counts the kittens at once (F14)")
{
	// Godot's choose() emits card_taken on the auto-pick path too, and TierController re-counts on it (F14, 2026-11-03).
	const Pc::PcGameData& D = Game();
	Pack K(D, 5);
	CHECK(K.F.AwakeKittens() == 5);
	// The deck down to Lone Hunter (costs 2 kittens); Auto-Card owned and switched on; one kill from a level.
	for (const Pc::PcCard& C : D.Cards)
	{
		if (C.Id != "lone_hunter")
		{
			K.P.BanishedCards.insert(C.Id);
		}
	}
	K.P.BranchLevels["auto_card"] = 1;
	K.In.bAutoPickSetting = true;
	K.P.CatchesPoints = Pc::TotalCatchesForLevel(D, 1) - 0.5;
	const int32_t Victim = K.Mouse("backyard_mouse", 800.0, 450.0);
	K.Pin(Victim);
	K.F.Hp[static_cast<size_t>(Victim)] = 1e-6;
	K.Steps(2);
	REQUIRE_FALSE(K.F.Active[static_cast<size_t>(Victim)]);
	REQUIRE(K.P.CardLevels.count("lone_hunter") == 1); // taken by Auto-Card, no screen
	CHECK(Pc::KittenCount(D, K.P) == 3);
	CHECK(K.F.AwakeKittens() == 3); // at once, not at the next purchase or travel
}

TEST_CASE("Kittens: they spread across the mice in range (Godot's ci/check_kitten_spread.gd)")
{
	// Nine kittens from one spot (the real spawn behaviour, and the case that made every kitten agree on one "nearest"),
	// 1-5 mice 100 px apart inside the detection radius; selection only (a step too short to move or hurt anything).
	const Pc::PcGameData& D = Game();
	for (int Mice = 1; Mice <= 5; ++Mice)
	{
		CAPTURE(Mice);
		Pack K(D, 9, 700.0, 620.0);
		std::vector<int32_t> Slots;
		for (int I = 0; I < Mice; ++I)
		{
			Slots.push_back(K.Mouse("backyard_mouse", 700.0 + 100.0 * I, 500.0));
		}
		K.Steps(40, 1e-9);
		const std::map<int32_t, int64_t> Claims = K.Claims();
		int64_t Total = 0;
		int64_t Worst = 0;
		for (const int32_t S : Slots)
		{
			const int64_t C = Claims.count(S) ? Claims.at(S) : 0;
			Total += C;
			Worst = std::max(Worst, C);
		}
		const int64_t Ideal = (9 + Mice - 1) / Mice;
		CHECK(Total == 9);           // no kitten idles with a mouse in range
		CHECK(Worst <= Ideal + 1);   // balanced, nothing like the old 1/1/7 dogpile
	}
}

TEST_CASE("Kittens: a target is sticky, and a lopsided pool rebalances once and then stops")
{
	const Pc::PcGameData& D = Game();
	{
		// Sticky: a closer, unclaimed mouse does not pull a kitten off the one it committed to.
		Pack K(D, 1, 800.0, 450.0);
		const int32_t Far = K.Mouse("backyard_gristle", 1100.0, 450.0);
		K.Pin(Far);
		K.F.Hp[static_cast<size_t>(Far)] = 1e12;
		K.Steps(1);
		REQUIRE(K.F.KTarget[0] == Far);
		const int32_t Near = K.Mouse("backyard_mouse", K.F.KX[0] - 5.0, K.F.KY[0]);
		K.Pin(Near);
		K.Steps(10);
		CHECK(K.F.KTarget[0] == Far);
	}
	{
		// Ten kittens all committed to the first of three mice: the over-subscribed ones move, the pool settles at
		// 4/3/3 (or as close as order allows) and then no kitten changes target again: no flicker.
		Pack K(D, 10, 800.0, 450.0);
		std::vector<int32_t> Slots;
		for (int I = 0; I < 3; ++I)
		{
			Slots.push_back(K.Mouse("backyard_gristle", 760.0 + 40.0 * I, 450.0));
			K.Pin(Slots.back());
			K.F.Hp[static_cast<size_t>(Slots.back())] = 1e12;
		}
		for (size_t I = 0; I < K.F.KActive.size(); ++I)
		{
			K.F.KTarget[I] = K.F.KActive[I] ? Slots[0] : -1;
			K.F.KTargetSerial[I] = K.F.KActive[I] ? K.F.Serial[static_cast<size_t>(Slots[0])] : 0;
		}
		K.Steps(30);
		const std::map<int32_t, int64_t> Claims = K.Claims();
		int64_t Worst = 0, Least = 100;
		for (const int32_t S : Slots)
		{
			const int64_t C = Claims.count(S) ? Claims.at(S) : 0;
			Worst = std::max(Worst, C);
			Least = std::min(Least, C);
		}
		CHECK(Worst - Least < D.Code.KittenRebalanceMargin);
		CHECK(Worst - Least >= 0);
		const std::vector<int32_t> Settled = K.F.KTarget;
		K.Steps(120);
		CHECK(K.F.KTarget == Settled);
	}
}

TEST_CASE("Kittens: contact damage is kitten DPS x the kitten's multiplier x the step; a shield blocks it")
{
	const Pc::PcGameData& D = Game();
	Pack K(D, 1, 800.0, 450.0);
	K.P.UpgradeLevels["kitten_power"] = 10;
	Pc::RefreshFieldStats(K.F, D, K.P);
	const int32_t M = K.Mouse("backyard_gristle", 800.0, 450.0);
	K.Pin(M);
	K.F.Hp[static_cast<size_t>(M)] = 1000.0;
	K.Steps(30); // settle into the slot: in contact from here on
	const double Hp = K.F.Hp[static_cast<size_t>(M)];
	K.Steps(1);
	CHECK(Hp - K.F.Hp[static_cast<size_t>(M)] == doctest::Approx(Pc::KittenDps(D.Combat, K.F.Stats) * Step).epsilon(1e-9));
	CHECK(Pc::KittenDps(D.Combat, K.F.Stats) > D.Combat.BaseKittenDamagePerSec); // the power line really reached it

	// Out of contact range: no damage, the kitten closes in.
	Pack Far(D, 1, 100.0, 100.0);
	const int32_t N = Far.Mouse("backyard_gristle", 500.0, 100.0);
	Far.Pin(N);
	const double Before = Far.F.Hp[static_cast<size_t>(N)];
	Far.Steps(1);
	CHECK(Far.F.Hp[static_cast<size_t>(N)] == Before);
	CHECK(Far.F.KX[0] == doctest::Approx(100.0 + D.Code.KittenSpeedPxPerSec * Far.F.Stats.KittenSpeedMultiplier * Step).epsilon(1e-3));

	// Shielded and no Shield Pierce: nothing lands.
	Pack S(D, 1, 800.0, 450.0);
	const int32_t G = S.Mouse("backyard_gristle", 800.0, 450.0);
	S.Pin(G);
	S.F.Shielded[static_cast<size_t>(G)] = 1;
	const double Full = S.F.Hp[static_cast<size_t>(G)];
	S.Steps(30);
	CHECK(S.F.Hp[static_cast<size_t>(G)] == Full);
}

TEST_CASE("Kittens: with nothing in range they roam at the roam pace and stay on the field")
{
	const Pc::PcGameData& D = Game();
	Pack K(D, 4, 800.0, 450.0);
	const double X0 = K.F.KX[0], Y0 = K.F.KY[0];
	K.Steps(1);
	const double Pace = D.Code.KittenSpeedPxPerSec * D.Code.KittenRoamSpeedRatio * K.F.Stats.KittenSpeedMultiplier * Step;
	CHECK(std::hypot(K.F.KX[0] - X0, K.F.KY[0] - Y0) == doctest::Approx(Pace));
	// They do not move as one block: each starts on its own orbit angle.
	CHECK(std::atan2(K.F.KRoamDirY[0], K.F.KRoamDirX[0]) != doctest::Approx(std::atan2(K.F.KRoamDirY[1], K.F.KRoamDirX[1])));
	K.Steps(60 * 120);
	for (const int32_t I : K.AwakeIndices())
	{
		CHECK(Arena.HasPoint(K.F.KX[static_cast<size_t>(I)], K.F.KY[static_cast<size_t>(I)]));
	}
}

TEST_CASE("Kittens: Gilded Kitten marks the first awake kitten, Lone Hunter the last, never the same one")
{
	const Pc::PcGameData& D = Game();
	Pack K(D, 5);
	K.P.CardLevels["gilded_kitten"] = 1;
	K.P.CardLevels["lone_hunter"] = 1;
	K.Recount(800.0, 450.0); // Lone Hunter's cost: 5 - 2 = 3 awake
	K.Steps(1);
	const std::vector<int32_t> Awake = K.AwakeIndices();
	REQUIRE(Awake.size() == 3);
	CHECK(K.F.KDamageMult[static_cast<size_t>(Awake[0])] == Pc::CardEffectValue(D, K.P, Pc::CardEffect::GOLDEN_KITTEN));
	CHECK(K.F.KGolden[static_cast<size_t>(Awake[0])] == 1);
	CHECK(K.F.KDamageMult[static_cast<size_t>(Awake[1])] == 1.0);
	CHECK(K.F.KDamageMult[static_cast<size_t>(Awake[2])] == Pc::CardEffectValue(D, K.P, Pc::CardEffect::LONE_HUNTER));
	CHECK(K.F.KLone[static_cast<size_t>(Awake[2])] == 1);
	CHECK(K.F.KGolden[static_cast<size_t>(Awake[2])] == 0);

	// One kitten awake: Gilded wins it, the halo goes nowhere.
	K.P.UpgradeLevels["kitten_count"] = 3;
	K.Recount(800.0, 450.0);
	K.Steps(1);
	const std::vector<int32_t> One = K.AwakeIndices();
	REQUIRE(One.size() == 1);
	CHECK(K.F.KGolden[static_cast<size_t>(One[0])] == 1);
	CHECK(K.F.KLone[static_cast<size_t>(One[0])] == 0);
	CHECK(K.F.KDamageMult[static_cast<size_t>(One[0])] == Pc::CardEffectValue(D, K.P, Pc::CardEffect::GOLDEN_KITTEN));
}

TEST_CASE("Kittens: Kitten Drag hauls a living victim toward the cat")
{
	const Pc::PcGameData D = WithCard("test_kitten_drag", Pc::CardEffect::KITTEN_DRAG, 120.0);
	Pack K(D, 1, 900.0, 450.0);
	K.P.CardLevels["test_kitten_drag"] = 1;
	K.In.CatX = 300.0; // the cat strikes nothing this far from the mouse
	K.In.CatY = 450.0;
	const int32_t M = K.Mouse("backyard_gristle", 900.0, 450.0);
	K.Pin(M);
	K.F.Hp[static_cast<size_t>(M)] = 1e12;
	K.Steps(5);
	const double X = K.F.X[static_cast<size_t>(M)];
	K.Steps(1);
	CHECK(X - K.F.X[static_cast<size_t>(M)] == doctest::Approx(120.0 * Step)); // moved toward the cat by drag x step
	CHECK(K.F.Y[static_cast<size_t>(M)] == doctest::Approx(450.0));
}

TEST_CASE("Kittens: Kitten From Signature wakes a temporary kitten per signature kill, capped, and it expires")
{
	const Pc::PcGameData D = WithCard("test_temp_kitten", Pc::CardEffect::KITTEN_FROM_SIGNATURE, 2.0);
	Pack K(D, 1);
	K.P.CardLevels["test_temp_kitten"] = 1;
	// A common pays no kitten; a signature pays one.
	K.Mouse("backyard_mouse", 1200.0, 300.0);
	Pc::KillAllEnemies(K.F, D, K.P, K.In);
	CHECK(K.F.AwakeKittens() == 1);
	K.Mouse("backyard_moth", 1200.0, 200.0);
	Pc::KillAllEnemies(K.F, D, K.P, K.In);
	CHECK(K.F.AwakeKittens() == 2);
	const int64_t Kills = K.P.LifetimeKills;
	for (int I = 0; I < D.Code.MaxTempKittens + 2; ++I)
	{
		K.Mouse("backyard_moth", 1200.0, 200.0); // the biome's signature
	}
	K.Mouse("backyard_mouse", 1200.0, 300.0);    // a common: no kitten
	Pc::KillAllEnemies(K.F, D, K.P, K.In);
	REQUIRE(K.P.LifetimeKills == Kills + D.Code.MaxTempKittens + 3);
	CHECK(K.F.AwakeKittens() == 1 + D.Code.MaxTempKittens); // capped at the pool's reserved headroom
	CHECK(static_cast<int64_t>(K.F.TempKittens.size()) == D.Code.MaxTempKittens);
	K.Steps(119); // 2 s at 60 Hz, less one step
	CHECK(K.F.AwakeKittens() == 1 + D.Code.MaxTempKittens);
	K.Steps(2);
	CHECK(K.F.AwakeKittens() == 1);
	CHECK(K.F.TempKittens.empty());
}

TEST_CASE("Kittens: Kitten Kill Explodes goes off where the victim died and hurts what is near (F13)")
{
	const Pc::PcGameData& D = Game();
	Pack K(D, 1, 800.0, 450.0);
	K.P.CardLevels["giblets"] = 4;
	const int32_t Victim = K.Mouse("backyard_mouse", 800.0, 450.0);
	K.Pin(Victim);
	const int32_t Neighbour = K.Mouse("backyard_gristle", 830.0, 450.0);
	K.Pin(Neighbour);
	K.F.Hp[static_cast<size_t>(Victim)] = 1e-6; // dies at the first touch
	const double Full = K.F.Hp[static_cast<size_t>(Neighbour)];
	// The kitten goes for the victim (nearest, both unclaimed): steer it there by claiming the neighbour first.
	K.F.KTarget[0] = Victim;
	K.F.KTargetSerial[0] = K.F.Serial[static_cast<size_t>(Victim)];
	K.Steps(1);
	REQUIRE_FALSE(K.F.Active[static_cast<size_t>(Victim)]);
	REQUIRE(K.Count(Pc::PcFieldEventKind::KittenBlast) == 1);
	const auto Blast = std::find_if(K.F.Events.begin(), K.F.Events.end(), [](const Pc::PcFieldEvent& E) { return E.Kind == Pc::PcFieldEventKind::KittenBlast; });
	// F13, designer 2026-11-03 (fixed in Godot first): the blast went off at the parking spot (-10000, -10000) and reached
	// nothing; it goes off where the victim stood.
	CHECK(Blast->X == 800.0);
	CHECK(Blast->Y == 450.0);
	CHECK(Blast->Value == D.Code.KittenExplosionRadius);
	CHECK(K.F.Hp[static_cast<size_t>(Neighbour)] < Full); // 30 px away, inside the 150 px radius: hit
	// Without the card: no blast at all.
	Pack N(D, 1, 800.0, 450.0);
	const int32_t V2 = N.Mouse("backyard_mouse", 800.0, 450.0);
	N.F.Hp[static_cast<size_t>(V2)] = 1e-6;
	N.Steps(1);
	CHECK(N.Count(Pc::PcFieldEventKind::KittenBlast) == 0);
}

TEST_CASE("Kittens: Grand Finale's illusions pull nearby mice toward the cat, for one step at a time")
{
	const Pc::PcGameData& D = Game();
	auto Heading = [](const Pc::PcField& F, int32_t S) { return std::atan2(F.DirY[static_cast<size_t>(S)], F.DirX[static_cast<size_t>(S)]); };
	Pack K(D, 3, 800.0, 450.0);
	K.P.BranchLevels["job_magician"] = 1;
	K.P.BranchLevels["grand_finale"] = 1;
	K.P.RunJob = "magician"; // the Magician's kittens deal nothing: they are illusions
	Pc::RefreshFieldStats(K.F, D, K.P);
	REQUIRE(Pc::KittenIllusionPull(D, K.P) > 0.0);
	REQUIRE(K.F.Stats.KittenMultiplicativeDamageProduct == 0.0);
	K.In.CatX = 800.0;
	K.In.CatY = 100.0; // the cat, straight "up" from the mouse
	// The mouse heads right, next to the kittens; the cat is above it, out of paw reach.
	const int32_t M = K.Mouse("backyard_mouse", 800.0, 470.0);
	K.F.LifetimeLeft[static_cast<size_t>(M)] = 1e9;
	K.Steps(1);
	CHECK(K.F.KIllusory[0] == 1);
	CHECK(K.F.IllusionPull[static_cast<size_t>(M)] == Pc::KittenIllusionPull(D, K.P)); // applied this step...
	CHECK(K.F.IllusionFresh[static_cast<size_t>(M)] == 0);                               // ...and spent by the move
	K.Steps(59);
	// Steered toward the cat (up = -pi/2) far beyond a wander's random drift.
	CHECK(Heading(K.F, M) < -0.8);
	// No kitten near: the pull lasts one more step's check, then it is gone.
	K.P.UpgradeLevels["kitten_count"] = 0;
	K.Recount(800.0, 450.0);
	K.Steps(1);
	CHECK(K.F.IllusionPull[static_cast<size_t>(M)] == 0.0);

	// Without the job the kittens bite, and are not drawn as illusions.
	Pack B(D, 3, 800.0, 450.0);
	B.P.BranchLevels["job_magician"] = 1;
	B.P.BranchLevels["grand_finale"] = 1;
	Pc::RefreshFieldStats(B.F, D, B.P);
	B.Steps(1);
	CHECK(B.F.KIllusory[0] == 0);

	// Without the node, the same mouse only wanders.
	Pack W(D, 3, 800.0, 450.0);
	W.In.CatX = 800.0;
	W.In.CatY = 100.0;
	W.P.RunJob = "magician";
	Pc::RefreshFieldStats(W.F, D, W.P);
	const int32_t U = W.Mouse("backyard_mouse", 800.0, 470.0);
	W.F.LifetimeLeft[static_cast<size_t>(U)] = 1e9;
	W.Steps(60);
	CHECK(std::fabs(Heading(W.F, U)) < 0.5);
	CHECK(W.F.KIllusory[0] == 0);
}

#endif
