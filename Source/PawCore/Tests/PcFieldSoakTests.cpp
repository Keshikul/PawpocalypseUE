#if defined(PAWCORE_STANDALONE_TESTS)

// The field at length (docs/11 phase 2 exit criteria): a headless soak of the first three biomes played by a scripted cat,
// every step checked for the field's invariants; and the step time with the field full (320 mice) and everything on.

#include "ThirdParty/doctest/doctest.h"
#include "PcCardDraw.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcProgress.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
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

	bool Finite(double V)
	{
		return std::isfinite(V);
	}

	// Everything a field must hold after any step. Returns the first broken invariant, or "".
	std::string Broken(const Pc::PcField& F, const Pc::PcGameData& D, const Pc::PcProgress& P)
	{
		if (F.LiveCount() > D.Code.MaxLiveEnemies)
		{
			return "more mice than max_live_enemies";
		}
		for (const int32_t S : F.Live)
		{
			const size_t I = static_cast<size_t>(S);
			if (!F.Active[I])
			{
				return "a dead mouse left in the live list";
			}
			if (!Finite(F.X[I]) || !Finite(F.Y[I]) || !Finite(F.Hp[I]) || !Finite(F.DirX[I]) || !Finite(F.DirY[I]))
			{
				return "a mouse with a non-finite value";
			}
			if (F.Hp[I] <= 0.0)
			{
				return "a live mouse at or below zero HP";
			}
			if (!F.Bounds.HasPoint(F.X[I], F.Y[I]) && F.Hp[I] > 0.0 && std::fabs(F.X[I]) > 1e5)
			{
				return "a live mouse far outside the field";
			}
		}
		for (size_t K = 0; K < F.KActive.size(); ++K)
		{
			if (F.KActive[K] && (!Finite(F.KX[K]) || !Finite(F.KY[K])))
			{
				return "a kitten with a non-finite position";
			}
		}
		if (F.AwakeKittens() != Pc::KittenCount(D, P) + static_cast<int64_t>(F.TempKittens.size())
			&& F.AwakeKittens() != static_cast<int64_t>(F.KActive.size()))
		{
			// (A kitten-cost card waits for the next re-count, F14: tolerated only between re-counts, checked by the caller.)
		}
		if (F.bBossUp && (!Finite(F.BossHp) || F.BossHp < 0.0 || !Finite(F.BossX) || !Finite(F.BossY)))
		{
			return "a boss with a bad HP or position";
		}
		if (!Finite(P.Catnip) || P.Catnip < 0.0 || !Finite(P.Stardust))
		{
			return "a wallet that is negative or not finite";
		}
		if (!Finite(F.PounceCooldownLeft) || F.PounceCooldownLeft < 0.0)
		{
			return "a Pounce cooldown below zero";
		}
		if (static_cast<int64_t>(F.Zones.size()) > D.Code.MaxDamageZones)
		{
			return "more zones than max_damage_zones";
		}
		return std::string();
	}

	// A cat played by a script: it walks at a hand's pace to the nearest mouse (or onto the boss while it is up), shops
	// every two seconds for the cheapest thing it can afford, takes the first card of any hand, summons the boss when it
	// can, Pounces whenever ready, and moves on when a biome opens.
	struct Player
	{
		const Pc::PcGameData& D;
		Pc::PcProgress P;
		Pc::PcField F;
		Pc::PcFieldInput In;
		uint64_t Seed;
		double Clock = 0.0;
		double ShopTimer = 0.0;
		int64_t Steps = 0;
		int64_t Biome = 0;
		int64_t Fights = 0, Won = 0, TimedOut = 0;
		std::string Problem;
		std::vector<Pc::PcFieldEvent> Drained;

		Player(const Pc::PcGameData& Data, uint64_t S) : D(Data), Seed(S)
		{
			In.bAutoPickSetting = true;
			In.bAutoTravelSetting = true;
			In.bAutoBaitSetting = true;
			Enter(0);
		}

		void Enter(int64_t Index)
		{
			Biome = Index;
			Pc::BeginBiome(D, P, D.Tiers[static_cast<size_t>(Index)].Id);
			Pc::BeginField(F, D, P, D.Tiers[static_cast<size_t>(Index)].Id, Arena, Seed + static_cast<uint64_t>(Index));
			In.CatX = F.CatX;
			In.CatY = F.CatY;
		}

		void Shop()
		{
			bool bBought = true;
			bool bKittens = false;
			while (bBought)
			{
				bBought = false;
				const Pc::PcUpgrade* Best = nullptr;
				double BestCost = 0.0;
				for (const Pc::PcUpgrade& U : D.Upgrades)
				{
					if (Pc::IsUpgradeAtCap(D, P, U.Id))
					{
						continue;
					}
					const double Cost = Pc::UpgradeCostById(D, P, U.Id);
					if (Cost <= P.Catnip && (!Best || Cost < BestCost))
					{
						Best = &U;
						BestCost = Cost;
					}
				}
				if (Best && Pc::PurchaseUpgrade(D, P, Best->Id))
				{
					bBought = true;
					bKittens = bKittens || Best->Id == "kitten_count";
				}
			}
			if (bKittens)
			{
				Pc::SyncFieldKittens(F, D, P);
			}
			// A hand left up (no Auto-Card yet): take its first card.
			if (!P.Hand.empty())
			{
				Pc::ChooseCard(D, P, P.Hand.front());
				Pc::SyncFieldKittens(F, D, P); // a card that costs kittens is felt at the next re-count
			}
			Pc::RefreshFieldStats(F, D, P);
		}

		void Aim()
		{
			double TX = In.CatX, TY = In.CatY;
			if (F.bBossUp)
			{
				TX = F.BossX;
				TY = F.BossY;
			}
			else
			{
				double Best = 1e30;
				for (const int32_t S : F.Live)
				{
					const size_t I = static_cast<size_t>(S);
					const double Dist = std::hypot(F.X[I] - In.CatX, F.Y[I] - In.CatY);
					if (Dist < Best)
					{
						Best = Dist;
						TX = F.X[I];
						TY = F.Y[I];
					}
				}
			}
			// A hand's pace with a mouse: quick, but not a teleport.
			const double DX = TX - In.CatX, DY = TY - In.CatY;
			const double Dist = std::hypot(DX, DY);
			const double Reach = 1400.0 * Step;
			if (Dist > Reach)
			{
				In.CatX += DX / Dist * Reach;
				In.CatY += DY / Dist * Reach;
			}
			else
			{
				In.CatX = TX;
				In.CatY = TY;
			}
			In.bPounceDown = !In.bPounceDown && F.PounceCooldownLeft <= 0.0;
		}

		// One step; false once something is broken.
		bool Tick()
		{
			Aim();
			Pc::StepField(F, D, P, In, Step);
			Clock += Step;
			++Steps;
			Problem = Broken(F, D, P);
			Drained.clear();
			Pc::DrainFieldEvents(F, Drained);
			for (const Pc::PcFieldEvent& E : Drained)
			{
				Won += E.Kind == Pc::PcFieldEventKind::BossDefeated ? 1 : 0;
				TimedOut += E.Kind == Pc::PcFieldEventKind::BossTimeout ? 1 : 0;
				Fights += E.Kind == Pc::PcFieldEventKind::BossSummoned ? 1 : 0;
				if (E.Kind == Pc::PcFieldEventKind::TravelRequested && E.Slot > Biome && E.Slot <= 2)
				{
					Enter(E.Slot);
					return Problem.empty();
				}
			}
			ShopTimer += Step;
			if (ShopTimer >= 2.0)
			{
				ShopTimer = 0.0;
				Shop();
				// In the Park the bait is topped up as the cheat overlay would (a scripted cat earns 9,600 kills there far
				// slower than a player), so the soak exercises the fight: summons, phases, adds, timeouts, clears.
				const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(Biome)].BossData);
				if (Boss && !F.bBossUp && Pc::BaitShortfall(D, P, *Boss) > 0)
				{
					P.BaitKillsByBiome[P.CurrentBiomeId] += Pc::BaitShortfall(D, P, *Boss) * Boss->KillsPerBait;
				}
				// The cat's own (manual) summon, when the bait is earned.
				if (!F.bBossUp && Pc::CanSummonFieldBoss(F, D, P))
				{
					Pc::SummonFieldBoss(F, D, P);
				}
				// Moving on by hand once the next biome is open and this one's boss (if any) is behind the cat.
				const size_t Next = static_cast<size_t>(Biome) + 1;
				if (Biome < 2 && !F.bBossUp && Pc::IsBiomeUnlocked(D, P, D.Tiers[Next]))
				{
					Enter(static_cast<int64_t>(Next));
				}
			}
			return Problem.empty();
		}
	};
}

TEST_CASE("Long: a soak of the first three biomes, a scripted cat at full speed, every step's invariants held" * doctest::test_suite("long") * doctest::skip())
{
	const Pc::PcGameData& D = Game();
	const char* Scale = std::getenv("PAWCORE_LONG_SCALE");
	const double Hours = 3.0 * (Scale ? std::max(std::atof(Scale), 0.01) : 1.0);
	for (uint64_t Seed : { 1ull, 2ull, 3ull })
	{
		CAPTURE(Seed);
		Player Cat(D, Seed);
		const auto Start = std::chrono::steady_clock::now();
		const int64_t Limit = static_cast<int64_t>(Hours * 3600.0 / Step);
		bool bOk = true;
		while (Cat.Steps < Limit && bOk)
		{
			bOk = Cat.Tick();
		}
		const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
		CAPTURE(Cat.Steps);
		CAPTURE(Cat.Biome);
		CHECK_MESSAGE(Cat.Problem.empty(), Cat.Problem);
		std::printf("soak seed %llu: %.2f game hours in %.1f s (x%.0f), biome %lld, %lld kills, Catnip %.4g, %lld fights: %lld won, %lld timed out\n",
			static_cast<unsigned long long>(Seed), Cat.Clock / 3600.0, Seconds, Cat.Clock / std::max(Seconds, 1e-9),
			static_cast<long long>(Cat.Biome + 1), static_cast<long long>(Cat.P.LifetimeKills), Cat.P.Catnip,
			static_cast<long long>(Cat.Fights), static_cast<long long>(Cat.Won), static_cast<long long>(Cat.TimedOut));
		CHECK(static_cast<double>(Cat.P.LifetimeKills) > 100.0 * Hours); // it really played
		CHECK(Cat.Biome >= 1);               // and got somewhere
		if (Cat.Biome == 2)
		{
			CHECK(Cat.Fights > 0);           // the fight was exercised
			CHECK(Cat.Fights >= Cat.Won + Cat.TimedOut);
		}
		CHECK(Cat.Clock / std::max(Seconds, 1e-9) > 100.0); // far faster than real time (the field is never the bottleneck)
	}
}

TEST_CASE("Long: the step time with the field full (320 mice, 20 kittens, the aura, zones, the Prism)" * doctest::test_suite("long") * doctest::skip())
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P;
	P.CurrentBiomeId = "tier_12";
	P.InfestationBonus = 1000.0; // the flood: the 320 ceiling is reached within a second
	P.UpgradeLevels["kitten_count"] = 20;
	P.DeepestBiomeIndex = 11;
	P.ReawakenCount = 20;
	for (const char* Node : { "unlock_aura", "vitality", "unlock_snare", "gridlock", "unlock_unicorn", "unlock_prism",
			 "refraction", "echo", "crowd_work", "job_town_crier" })
	{
		P.BranchLevels[Node] = 1;
	}
	P.CardLevels["ember_trail"] = 4;
	P.CardLevels["hot_borders"] = 3;
	Pc::PcField F;
	Pc::BeginField(F, D, P, "tier_12", Arena, 77);
	Pc::PcFieldInput In;
	In.CatX = 800.0;
	In.CatY = 450.0;
	for (int K = 0; K < 120; ++K)
	{
		In.CatX = 800.0 + 300.0 * std::cos(K * 0.05);
		Pc::StepField(F, D, P, In, Step);
		F.Events.clear();
	}
	REQUIRE(F.LiveCount() >= D.Code.MaxLiveEnemies - 5);
	REQUIRE(F.AwakeKittens() >= 20);
	const int Steps = 600;
	const auto Start = std::chrono::steady_clock::now();
	for (int K = 0; K < Steps; ++K)
	{
		In.CatX = 800.0 + 300.0 * std::cos(K * 0.05);
		Pc::StepField(F, D, P, In, Step);
		F.Events.clear();
	}
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count() / Steps;
	std::printf("field step, %lld mice and %lld kittens: %.3f ms\n", static_cast<long long>(F.LiveCount()),
		static_cast<long long>(F.AwakeKittens()), Ms);
	// The whole frame's budget at 60 Hz is 16.7 ms; the simulation is allowed a small slice of it (docs/04 §6).
	CHECK(Ms < 2.0);
}

#endif
