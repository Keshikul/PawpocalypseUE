#if defined(PAWCORE_STANDALONE_TESTS)

// PcField's boss (phase 2, slice 6). The boss's own rules are checked against a real Godot Boss node (PcBossTests.cpp);
// these pin the fight around it, each from TierController's code and comments: the bait, the relocation, engagement and
// the fight clock, the paw on the boss, the adds, the kittens' duty, the defeat and the travel after it, Auto-Bait,
// Auto-Travel, everything else that reaches the boss, the dismissal.

#include "ThirdParty/doctest/doctest.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcProgress.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
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

	// The Park (biome 3) with its Tyrant's bait earned three times over and the Catnip to pay for it; no spawner; the cat
	// parked far away unless a test moves it.
	struct Fight
	{
		const Pc::PcGameData& D;
		const Pc::PcBoss& B;
		Pc::PcProgress P;
		Pc::PcField F;
		Pc::PcFieldInput In;

		static const Pc::PcBoss& BossOf(const Pc::PcGameData& Data, const std::string& TierId)
		{
			for (const Pc::PcTier& T : Data.Tiers)
			{
				if (T.Id == TierId)
				{
					return *Data.FindBoss(T.BossData);
				}
			}
			FAIL("no tier " << TierId);
			return *Data.FindBoss("park_tyrant");
		}

		explicit Fight(const Pc::PcGameData& Data, bool bBait = true, const std::string& TierId = "tier_3")
			: D(Data), B(BossOf(Data, TierId))
		{
			P.CurrentBiomeId = TierId;
			if (bBait)
			{
				P.BaitKillsByBiome[P.CurrentBiomeId] = B.BaitRequired * B.KillsPerBait * 3;
				P.Catnip = 1e7;
			}
			// The next biome's Catnip gate open, so only the boss stands between the cat and it.
			P.LifetimeCatnipThisCycle = 1e30;
			P.LifetimeCatnipEver = 1e30;
			Pc::BeginField(F, D, P, TierId, Arena, 33);
			F.SpawnTimeLeft = 1e18;
			Cat(-5000.0, -5000.0);
		}

		void Cat(double X, double Y)
		{
			In.CatX = X;
			In.CatY = Y;
			F.CatX = X;
			F.CatY = Y;
		}

		void Steps(int N)
		{
			for (int K = 0; K < N; ++K)
			{
				Pc::StepField(F, D, P, In, Step);
			}
		}

		void Refresh() { Pc::RefreshFieldStats(F, D, P); }

		std::vector<Pc::PcFieldEvent> Of(Pc::PcFieldEventKind Kind) const
		{
			std::vector<Pc::PcFieldEvent> Out;
			std::copy_if(F.Events.begin(), F.Events.end(), std::back_inserter(Out), [Kind](const Pc::PcFieldEvent& E) { return E.Kind == Kind; });
			return Out;
		}

		// Holds the boss's shield down for a test that is not about the shield.
		void Unshield()
		{
			F.bBossShielded = false;
			F.BossInvulnTimer = -1e9;
		}
	};
}

TEST_CASE("Boss: summoning buys the bait all-or-nothing; refused, nothing is spent; never twice at once")
{
	const Pc::PcGameData& D = Game();
	Fight None(D, false);
	CHECK_FALSE(Pc::SummonFieldBoss(None.F, D, None.P));
	CHECK_FALSE(None.F.bBossUp);
	CHECK(None.P.BaitSpentByBiome["tier_3"] == 0);

	Fight A(D);
	const double Catnip = A.P.Catnip;
	REQUIRE(Pc::CanSummonFieldBoss(A.F, D, A.P));
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	CHECK(A.F.bBossUp);
	CHECK(A.F.BossHp == A.B.TotalHp);
	CHECK(A.F.BossPhase == 1);
	CHECK(A.F.BossX == Arena.CentreX());
	CHECK(A.F.BossY == Arena.CentreY());
	CHECK(A.P.BaitSpentByBiome["tier_3"] == A.B.BaitRequired);
	CHECK(A.P.Catnip == Catnip - A.B.BaitCostCatnip);
	CHECK(A.Of(Pc::PcFieldEventKind::BossSummoned).size() == 1);
	CHECK_FALSE(Pc::CanSummonFieldBoss(A.F, D, A.P));
	CHECK_FALSE(Pc::SummonFieldBoss(A.F, D, A.P));
	CHECK(A.P.BaitSpentByBiome["tier_3"] == A.B.BaitRequired);
}

TEST_CASE("Boss: a save during the fight is the save from before the summon (F36, D16); a fight that ended keeps its cost")
{
	const Pc::PcGameData& D = Game();
	// The file a save would write, from a copy (CaptureSave banks the live kills into the progress it is handed).
	auto File = [&D](const Pc::PcProgress& P, const Pc::PcSummonPayment* FightUp)
	{
		Pc::PcProgress Copy = P;
		return Pc::WriteSaveFile(Pc::CaptureSave(D, Copy, Pc::PcSlotExtras(), 1.0, FightUp), "test");
	};

	Fight A(D);
	CHECK(Pc::FightPayment(A.F) == nullptr);
	const std::string Before = File(A.P, nullptr);
	const double Catnip = A.P.Catnip;
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	const Pc::PcSummonPayment* Paid = Pc::FightPayment(A.F);
	REQUIRE(Paid != nullptr);
	CHECK(Paid->BiomeId == "tier_3");
	CHECK(Paid->Bait == A.B.BaitRequired);
	CHECK(Paid->Catnip == A.B.BaitCostCatnip);
	CHECK(A.B.BaitRequired > 0); // both halves of the payment are seen
	CHECK(A.B.BaitCostCatnip > 0.0);
	// The game in memory paid and fights on; the file is the one from before the summon, byte for byte.
	CHECK(A.P.Catnip == Catnip - A.B.BaitCostCatnip);
	CHECK(A.P.BaitSpentByBiome["tier_3"] == A.B.BaitRequired);
	CHECK(File(A.P, Paid) == Before);
	CHECK(File(A.P, nullptr) != Before); // without the refund the file carries the spend (Godot's way)
	Pc::PcProgress Copy = A.P;
	const Pc::PcSaveState S = Pc::CaptureSave(D, Copy, Pc::PcSlotExtras(), 1.0, Paid);
	CHECK(S.Catnip == Catnip);
	CHECK(S.BaitSpent.count("tier_3") == 0);

	// Out of time, it leaves with the bait: the cost of losing stands, and a save says so.
	A.F.AddTimer = 1e9;
	A.Unshield();
	A.Cat(A.F.BossX, A.F.BossY);
	A.Steps(1);
	A.Cat(-5000.0, -5000.0);
	A.Steps(static_cast<int>(std::ceil(A.B.TimeLimitSec / Step)) + 2);
	REQUIRE_FALSE(A.F.bBossUp);
	CHECK(Pc::FightPayment(A.F) == nullptr);

	// A second summon's payment is its own, not the two added up; a save taken after it keeps the first fight's cost.
	const std::string AfterLoss = File(A.P, nullptr);
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	REQUIRE(Pc::FightPayment(A.F) != nullptr);
	CHECK(Pc::FightPayment(A.F)->Bait == A.B.BaitRequired);
	CHECK(File(A.P, Pc::FightPayment(A.F)) == AfterLoss);
	// Won: the ledger records it, nothing is given back.
	Pc::DamageFieldBoss(A.F, D, A.P, A.In, A.B.TotalHp * 2.0, 0.0);
	CHECK_FALSE(A.F.bBossUp);
	CHECK(Pc::FightPayment(A.F) == nullptr);
	// Dismissed (a Reawaken): nothing is given back either.
	Fight C(D);
	REQUIRE(Pc::SummonFieldBoss(C.F, D, C.P));
	Pc::DismissFieldBoss(C.F);
	CHECK(Pc::FightPayment(C.F) == nullptr);

	// AUDIT pays the summon in Catnip only (no Bait): the payment is what was taken, so all of it comes back.
	Fight Au(D);
	Au.P.BranchLevels["audit"] = 1;
	REQUIRE(Pc::AuditCatnipMultiple(D, Au.P) > 0.0);
	const std::string AuBefore = File(Au.P, nullptr);
	const double AuCost = Pc::SummonCatnipCost(D, Au.P, Au.B);
	REQUIRE(Pc::SummonFieldBoss(Au.F, D, Au.P));
	REQUIRE(Pc::FightPayment(Au.F) != nullptr);
	CHECK(Pc::FightPayment(Au.F)->Bait == 0);
	CHECK(Pc::FightPayment(Au.F)->Catnip == AuCost);
	CHECK(AuCost > Au.B.BaitCostCatnip);
	CHECK(File(Au.P, Pc::FightPayment(Au.F)) == AuBefore);
}

TEST_CASE("Boss: its shield turns over on the phase's clock, and it relocates the instant its window opens")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.F.AddTimer = 1e9; // no adds in this test
	const int Vulnerable = static_cast<int>(std::ceil(A.B.PhaseVulnerableSec[0] / Step));
	A.Steps(Vulnerable + 1);
	CHECK(A.F.bBossShielded);
	REQUIRE(A.Of(Pc::PcFieldEventKind::BossWindow).size() == 1);
	CHECK((A.Of(Pc::PcFieldEventKind::BossWindow)[0].Flags & Pc::FieldFlag::Shielded) != 0);
	const double AnchorX = A.F.BossAnchorX, AnchorY = A.F.BossAnchorY;
	const int Shield = static_cast<int>(std::ceil(A.B.PhaseShieldSec[0] / Step));
	A.Steps(Shield + 1);
	CHECK_FALSE(A.F.bBossShielded);
	REQUIRE(A.Of(Pc::PcFieldEventKind::BossWindow).size() == 2);
	CHECK((A.F.BossAnchorX != AnchorX || A.F.BossAnchorY != AnchorY));
	const double Inset = std::min(D.Code.BossArenaInset, std::min(Arena.W, Arena.H) * 0.3);
	CHECK(A.F.BossAnchorX >= Arena.X + Inset);
	CHECK(A.F.BossAnchorX <= Arena.Right() - Inset);
	CHECK(A.F.BossAnchorY >= Arena.Y + Inset);
	CHECK(A.F.BossAnchorY <= Arena.Bottom() - Inset);
	// Far from its new anchor it races there (fast but continuous), then drifts.
	if (std::hypot(A.F.BossX - A.F.BossAnchorX, A.F.BossY - A.F.BossAnchorY) > D.Code.BossRelocateDistancePx + 10.0)
	{
		const double X0 = A.F.BossX, Y0 = A.F.BossY;
		A.Steps(1);
		CHECK(std::hypot(A.F.BossX - X0, A.F.BossY - Y0) == doctest::Approx(D.Code.BossRelocateSpeed * Step));
	}
	A.Steps(600);
	CHECK(std::hypot(A.F.BossX - A.F.BossAnchorX, A.F.BossY - A.F.BossAnchorY) <= std::hypot(D.Code.BossWobbleXPx, D.Code.BossWobbleYPx) + 1.0);
}

TEST_CASE("Boss: the aura bites only while the cat engages; the clock runs from the first engagement; out of time it leaves with the bait")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	A.P.BranchLevels["unlock_aura"] = 1;
	A.Refresh();
	A.F.Stats.AttackSpeedMultiplier = 0.0; // the aura alone
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.F.AddTimer = 1e9;
	A.Steps(120);
	CHECK(A.F.BossHp == A.B.TotalHp);
	CHECK(A.F.BossFightElapsed == 0.0); // a countdown from arrival would time the whole biome
	A.Unshield();
	A.Cat(A.F.BossX, A.F.BossY);
	const double Hp = A.F.BossHp;
	A.Steps(1);
	CHECK(Hp - A.F.BossHp == doctest::Approx(Pc::AuraDps(D.Combat, A.F.Stats) * Step));
	CHECK(A.F.bBossEngagedOnce);
	A.Cat(-5000.0, -5000.0);
	const int Limit = static_cast<int>(std::ceil(A.B.TimeLimitSec / Step));
	A.Steps(Limit - 10);
	CHECK(A.F.bBossUp); // the clock keeps running away from it
	A.Steps(20);
	CHECK_FALSE(A.F.bBossUp);
	CHECK(A.Of(Pc::PcFieldEventKind::BossTimeout).size() == 1);
	CHECK(A.Of(Pc::PcFieldEventKind::BossDefeated).empty());
	CHECK(A.P.BossesDefeated.count("tier_3") == 0);
	CHECK(A.P.BaitSpentByBiome["tier_3"] == A.B.BaitRequired); // gone: the whole cost of losing
	CHECK(A.F.Adds.empty());
}

TEST_CASE("Boss: with no mouse in reach the paw swings at the boss; the shred streak builds on it")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	A.P.UpgradeLevels["shred"] = 20;
	A.Refresh();
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.F.AddTimer = 1e9;
	A.Unshield();
	A.F.Stats.CritChance = 0.0;
	for (int K = 0; K < 240; ++K)
	{
		A.Cat(A.F.BossX + 30.0, A.F.BossY); // the cat follows it, inside the paw's reach but not on it
		Pc::StepField(A.F, D, A.P, A.In, Step);
	}
	const std::vector<Pc::PcFieldEvent> S = A.Of(Pc::PcFieldEventKind::Strike);
	REQUIRE(S.size() > 3);
	CHECK(std::all_of(S.begin(), S.end(), [](const Pc::PcFieldEvent& E) { return (E.Flags & Pc::FieldFlag::Boss) != 0; }));
	CHECK(S[0].Value == doctest::Approx(Pc::PawStrikeDamage(D.Combat, A.F.Stats, 0)));
	CHECK(S[1].Value > S[0].Value); // a streak on the one target that cannot wander off
	CHECK(A.F.bShredOnBoss);
	CHECK(A.F.BossHp < A.B.TotalHp);
}

TEST_CASE("Boss: adds walk in at the interval and heal it on arrival; the cat's whole damage intercepts them")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.F.BossHp = A.B.TotalHp * 0.5;
	A.Steps(1);
	REQUIRE(A.F.Adds.size() == 1); // the add timer starts at zero
	CHECK(A.F.Adds[0].Hp == doctest::Approx(A.B.TotalHp * A.B.AddHpFraction));
	// Launched from an edge, and already a step along (it moves the frame it is launched, as Godot's add does).
	const double Slack = D.Code.SpawnEdgeInset + A.B.AddSpeedPxPerSec * Step + 1e-6;
	const bool bOnEdge = A.F.Adds[0].X <= Arena.X + Slack || A.F.Adds[0].X >= Arena.Right() - Slack
		|| A.F.Adds[0].Y <= Arena.Y + Slack || A.F.Adds[0].Y >= Arena.Bottom() - Slack;
	CHECK(bOnEdge);
	A.Unshield();
	const double Hp = A.F.BossHp;
	for (int K = 0; K < 60 * 60 && A.Of(Pc::PcFieldEventKind::BossHealed).empty(); ++K)
	{
		A.Steps(1);
	}
	REQUIRE(A.Of(Pc::PcFieldEventKind::BossHealed).size() == 1);
	CHECK(A.F.BossHp == doctest::Approx(Hp + A.B.TotalHp * A.B.AddHealFraction));
	// Healed past full: capped.
	A.F.BossHp = A.B.TotalHp;
	Pc::HealFieldBoss(A.F, D, A.B.TotalHp);
	CHECK(A.F.BossHp == A.B.TotalHp);

	// Interception: the cat stands on the next add; its whole damage (the paw's DPS, though the paw never reaches it).
	Fight C(D);
	REQUIRE(Pc::SummonFieldBoss(C.F, D, C.P));
	C.Steps(1);
	REQUIRE(C.F.Adds.size() == 1);
	C.Cat(C.F.Adds[0].X, C.F.Adds[0].Y);
	const double AddHp = C.F.Adds[0].Hp;
	C.Steps(1);
	REQUIRE(C.F.Adds.size() == 1);
	CHECK(AddHp - C.F.Adds[0].Hp == doctest::Approx(Pc::PlayerDps(D.Combat, C.F.Stats) * Step));
	C.F.Adds[0].Hp = 1e-9;
	C.Cat(C.F.Adds[0].X, C.F.Adds[0].Y);
	C.Steps(1);
	CHECK(C.F.Adds.empty());
	CHECK(C.Of(Pc::PcFieldEventKind::AddIntercepted).size() == 1);
}

TEST_CASE("Boss: while the cat engages, the kittens go for the nearest add first, then the boss")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	A.P.UpgradeLevels["kitten_count"] = 2;
	A.Cat(200.0, 200.0);
	// (two levels: four kittens since D17 E4; this test only needs some)
	Pc::SyncFieldKittens(A.F, D, A.P);
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.Steps(1);
	REQUIRE(A.F.Adds.size() == 1);
	A.F.AddTimer = 1e9;
	A.F.Adds[0].Speed = 0.0; // held where it is
	A.Cat(A.F.BossX, A.F.BossY);
	std::vector<double> Before;
	for (size_t K = 0; K < A.F.KActive.size(); ++K)
	{
		if (A.F.KActive[K])
		{
			Before.push_back(std::hypot(A.F.KX[K] - A.F.Adds[0].X, A.F.KY[K] - A.F.Adds[0].Y));
		}
	}
	A.Steps(1);
	size_t N = 0;
	for (size_t K = 0; K < A.F.KActive.size(); ++K)
	{
		if (A.F.KActive[K])
		{
			CHECK(A.F.KTarget[K] == -1); // the mouse dropped
			CHECK(std::hypot(A.F.KX[K] - A.F.Adds[0].X, A.F.KY[K] - A.F.Adds[0].Y) < Before[N++]);
		}
	}
	// No add: the boss.
	A.F.Adds.clear();
	std::vector<double> ToBoss;
	for (size_t K = 0; K < A.F.KActive.size(); ++K)
	{
		if (A.F.KActive[K])
		{
			ToBoss.push_back(std::hypot(A.F.KX[K] - A.F.BossX, A.F.KY[K] - A.F.BossY));
		}
	}
	A.Cat(A.F.BossX, A.F.BossY);
	A.Steps(1);
	N = 0;
	for (size_t K = 0; K < A.F.KActive.size(); ++K)
	{
		if (A.F.KActive[K])
		{
			CHECK(std::hypot(A.F.KX[K] - A.F.BossX, A.F.KY[K] - A.F.BossY) < ToBoss[N++]);
		}
	}
}

TEST_CASE("Boss: with two adds out, the kittens chase the one nearest the cat, and once it falls the rest wait")
{
	// tier_controller.gd _resolve_kittens(): ONE add, _nearest_active_add() to the cat, for every kitten; kitten.gd
	// hunt_add(): an add no longer active is a no-op, so the kittens after the killing blow do nothing this step.
	const Pc::PcGameData& D = Game();
	Fight A(D);
	// Two kittens, whatever Kitten Count's step (two a level since D17 E4).
	A.P.UpgradeLevels["kitten_count"] = static_cast<int64_t>(std::ceil(2.0 / D.FindUpgrade("kitten_count")->EffectPerLevel));
	Pc::SyncFieldKittens(A.F, D, A.P);
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.Steps(1);
	A.F.AddTimer = 0.0;
	A.Steps(1);
	REQUIRE(A.F.Adds.size() == 2);
	A.F.AddTimer = 1e9;
	CHECK(A.F.Adds[0].Serial != A.F.Adds[1].Serial);
	// The OLDER add far to the left, the newer one nearer, to the right; both held still; the cat on the boss (engaging),
	// out of its own aura's reach of either add.
	const double BX = A.F.BossX, BY = A.F.BossY;
	A.F.Adds[0].X = BX - 600.0;
	A.F.Adds[0].Y = BY;
	A.F.Adds[1].X = BX + 300.0;
	A.F.Adds[1].Y = BY;
	A.F.Adds[0].Speed = A.F.Adds[1].Speed = 0.0;
	const uint64_t Near = A.F.Adds[1].Serial;
	const uint64_t Far = A.F.Adds[0].Serial;
	std::vector<size_t> Awake;
	for (size_t K = 0; K < A.F.KActive.size(); ++K)
	{
		if (A.F.KActive[K])
		{
			Awake.push_back(K);
			A.F.KX[K] = BX;
			A.F.KY[K] = BY;
		}
	}
	REQUIRE(Awake.size() == 2);
	A.Cat(BX, BY);
	A.Steps(1);
	for (const size_t K : Awake)
	{
		// Toward the near add (to the right), not the old one (to the left).
		CHECK(A.F.KX[K] > BX);
	}

	// Both kittens on the near add, which dies to the first bite: the second kitten does not go after the other add.
	A.F.Events.clear();
	A.F.Adds[1].Hp = 1e-12;
	for (const size_t K : Awake)
	{
		A.F.KX[K] = A.F.Adds[1].X;
		A.F.KY[K] = A.F.Adds[1].Y;
	}
	const double FarHp = A.F.Adds[0].Hp;
	const double NearX = A.F.Adds[1].X, NearY = A.F.Adds[1].Y;
	A.Cat(A.F.BossX, A.F.BossY);
	A.Steps(1);
	REQUIRE(A.F.Adds.size() == 1);
	CHECK(A.F.Adds[0].Serial == Far);
	CHECK(A.F.Adds[0].Serial != Near);
	CHECK(A.F.Adds[0].Hp == FarHp);
	CHECK(A.Of(Pc::PcFieldEventKind::AddIntercepted).size() == 1);
	// The kitten after the killing blow stood still: no add left to chase this step.
	CHECK(A.F.KX[Awake[1]] == NearX);
	CHECK(A.F.KY[Awake[1]] == NearY);
}

TEST_CASE("Boss: a first clear pays and records at once, and travels on after the death sequence only with Auto-Travel on")
{
	const Pc::PcGameData& D = Game();
	auto Clear = [&D](bool bSetting, bool bRematch)
	{
		Fight A(D);
		if (bRematch)
		{
			A.P.BossesDefeated.insert("tier_3");
		}
		A.In.bAutoTravelSetting = bSetting;
		REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
		const double Catnip = A.P.Catnip;
		Pc::DamageFieldBoss(A.F, D, A.P, A.In, A.B.TotalHp * 2.0, 0.0);
		CHECK_FALSE(A.F.bBossUp);
		const std::vector<Pc::PcFieldEvent> Def = A.Of(Pc::PcFieldEventKind::BossDefeated);
		REQUIRE(Def.size() == 1);
		CHECK(((Def[0].Flags & Pc::FieldFlag::New) != 0) == !bRematch);
		CHECK(A.P.BossesDefeated.count("tier_3") == 1);
		CHECK(A.P.Catnip > Catnip);
		const bool bNextOpen = Pc::IsBiomeUnlocked(D, A.P, D.Tiers[3]);
		const int Sequence = static_cast<int>(std::ceil(D.Code.BossDeathSequenceSec / Step));
		A.Steps(Sequence - 2);
		CHECK(A.Of(Pc::PcFieldEventKind::TravelRequested).empty()); // the sequence plays first
		A.Steps(4);
		const std::vector<Pc::PcFieldEvent> T = A.Of(Pc::PcFieldEventKind::TravelRequested);
		const bool bExpected = bSetting && !bRematch && bNextOpen;
		CHECK(T.size() == (bExpected ? 1u : 0u));
		if (!T.empty())
		{
			CHECK(T[0].Slot == 3);
		}
		return bNextOpen;
	};
	CHECK(Clear(true, false)); // the Tyrant's first clear opens the next biome
	Clear(false, false);
	Clear(true, true);
}

TEST_CASE("Boss: the last boss's kill raises the ending when boss_defeated is emitted, and no other kill does")
{
	const Pc::PcGameData& D = Game();
	REQUIRE(D.Code.BossRealms.size() >= 2);
	// Victory: a first clear, or the first rematch of a cycle (hud.gd _on_boss_defeated_for_victory, on boss_defeated).
	auto Kill = [&D](const std::string& TierId, bool bBeaten, bool bClearedThisCycle)
	{
		Fight A(D, true, TierId);
		if (bBeaten)
		{
			A.P.BossesDefeated.insert(TierId);
		}
		if (bClearedThisCycle)
		{
			A.P.BossesClearedThisCycle.insert(TierId);
		}
		A.P.Catnip = 1e300; // the last boss's bait costs far more than the Tyrant's
		REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
		Pc::DamageFieldBoss(A.F, D, A.P, A.In, A.B.TotalHp * 2.0, 0.0);
		const std::vector<Pc::PcFieldEvent> Def = A.Of(Pc::PcFieldEventKind::BossDefeated);
		REQUIRE(Def.size() == 1);
		return (Def[0].Flags & Pc::FieldFlag::Victory) != 0;
	};
	const std::string Last = D.Code.BossRealms.back();
	CHECK(Kill(Last, false, false));
	CHECK(Kill(Last, true, false));
	CHECK_FALSE(Kill(Last, true, true));
	CHECK_FALSE(Kill(D.Code.BossRealms.front(), false, false));
}

TEST_CASE("Boss: Auto-Bait summons as soon as it can (owned AND switched on); Auto-Travel asks to move on when no boss is up")
{
	const Pc::PcGameData& D = Game();
	{
		Fight A(D);
		A.P.BranchLevels["auto_bait"] = 1;
		A.In.bAutoBaitSetting = true;
		A.Steps(static_cast<int>(std::ceil(D.Code.AutoTravelPollSec / Step)) + 2);
		CHECK(A.F.bBossUp);
		Fight Off(D);
		Off.P.BranchLevels["auto_bait"] = 1; // owned, switched off
		Off.Steps(120);
		CHECK_FALSE(Off.F.bBossUp);
		Fight Unowned(D);
		Unowned.In.bAutoBaitSetting = true;
		Unowned.Steps(120);
		CHECK_FALSE(Unowned.F.bBossUp);
	}
	{
		Fight A(D);
		A.P.BossesDefeated.insert("tier_3"); // the next biome opened: power ever, and passage this cycle
		A.P.BossesClearedThisCycle.insert("tier_3");
		A.P.BranchLevels["auto_travel"] = 1;
		A.In.bAutoTravelSetting = true;
		REQUIRE(Pc::IsBiomeUnlocked(D, A.P, D.Tiers[3]));
		REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
		A.F.AddTimer = 1e9;
		A.Steps(180);
		CHECK(A.Of(Pc::PcFieldEventKind::TravelRequested).empty()); // never while the boss is up
		Pc::DismissFieldBoss(A.F);
		A.Steps(static_cast<int>(std::ceil(D.Code.AutoTravelPollSec / Step)) + 2);
		REQUIRE(A.Of(Pc::PcFieldEventKind::TravelRequested).size() >= 1);
		CHECK(A.Of(Pc::PcFieldEventKind::TravelRequested)[0].Slot == 3);
	}
}

TEST_CASE("Boss: Pounce and a zone reach it; a Pounce counts as engaging")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	A.P.BranchLevels["unlock_pounce"] = 1;
	A.Refresh();
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.F.AddTimer = 1e9;
	A.Unshield();
	// Inside the Pounce's radius, outside the aura's: not engaging, until the Pounce lands.
	A.Cat(A.F.BossX + 120.0, A.F.BossY);
	A.F.BossDriftTimer = 1e9;
	A.F.BossDriftX = A.F.BossX;
	A.F.BossDriftY = A.F.BossY;
	const double Hp = A.F.BossHp;
	const double Burst = Pc::PounceDamage(D.Combat, A.F.Stats, true);
	A.In.bPounceDown = true;
	A.Steps(1);
	CHECK(Hp - A.F.BossHp == doctest::Approx(Burst)); // the boss's window is open: the bonus
	CHECK(A.F.bBossEngagedOnce);

	// A zone on it.
	Fight Z(D);
	REQUIRE(Pc::SummonFieldBoss(Z.F, D, Z.P));
	Z.F.AddTimer = 1e9;
	Z.Unshield();
	Pc::PcDamageZone Patch;
	Patch.X = Z.F.BossX + 100.0; // off its centre: a zone reaching the boss by its radius, not by standing on it
	Patch.Y = Z.F.BossY;
	Patch.Radius = 300.0;
	Patch.PowerFraction = 0.5;
	Patch.Remaining = 10.0;
	Patch.Duration = 10.0;
	Z.F.Zones.push_back(Patch);
	const double ZHp = Z.F.BossHp;
	Z.Steps(1);
	CHECK(ZHp - Z.F.BossHp == doctest::Approx(Pc::PlayerDps(D.Combat, Z.F.Stats) * 0.5 * Step));
}

TEST_CASE("Boss: exactly on a phase threshold it is already in the next phase")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	const double T = A.B.PhaseHpThresholds[0];
	A.F.BossHp = T * A.B.TotalHp;
	REQUIRE(A.F.BossHp / A.B.TotalHp == T); // exactly on it
	Pc::DamageFieldBoss(A.F, D, A.P, A.In, 0.0, 0.0);
	CHECK(A.F.BossPhase == 2);
}

TEST_CASE("Boss: dismissed (a Reawaken), it leaves with its adds and nothing is credited")
{
	const Pc::PcGameData& D = Game();
	Fight A(D);
	REQUIRE(Pc::SummonFieldBoss(A.F, D, A.P));
	A.Steps(1);
	REQUIRE_FALSE(A.F.Adds.empty());
	const double Catnip = A.P.Catnip;
	Pc::DismissFieldBoss(A.F);
	CHECK_FALSE(A.F.bBossUp);
	CHECK(A.F.Adds.empty());
	CHECK(A.P.Catnip == Catnip);
	CHECK(A.P.BossesDefeated.empty());
	// And it can be summoned again with the next bait.
	CHECK(Pc::SummonFieldBoss(A.F, D, A.P));
}

#endif
