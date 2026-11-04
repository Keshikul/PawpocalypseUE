#if defined(PAWCORE_STANDALONE_TESTS)

// The cheat overlay's rules (PcCheats, from GameState's debug_* functions): what each button gives, and what it leaves.

#include "ThirdParty/doctest/doctest.h"
#include "PcCheats.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcProgress.h"
#include "PcStats.h"

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

	Pc::PcProgress Fresh(const Pc::PcGameData& D)
	{
		Pc::PcProgress P;
		Pc::NewGame(D, P);
		Pc::BeginBiome(D, P, D.Tiers.front().Id);
		return P;
	}

	// The line's effect at a level, against its cap: additive levels add, multiplicative ones compound.
	double Effect(const Pc::PcUpgrade& U, int64_t Level)
	{
		return U.EffectType == Pc::UpgradeEffectType::ADDITIVE ? static_cast<double>(Level) * U.EffectPerLevel
			: std::pow(U.EffectPerLevel, static_cast<double>(Level));
	}
}

TEST_CASE("Cheats: the currency buttons credit Catnip normally and Stardust to the wallet alone")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	P.StardustGrantedEver = 7.0;
	Pc::CheatAddCurrency(P, Pc::CheatCatnipGrant, 0.0);
	CHECK(P.Catnip == 1000.0);
	CHECK(P.LifetimeCatnipThisCycle == 1000.0); // counts toward the cycle (the biome gates read it)
	CHECK(P.LifetimeCatnipEver == 1000.0);
	CHECK(P.Stardust == 0.0);
	Pc::CheatAddCurrency(P, 0.0, Pc::CheatStardustGrant);
	CHECK(P.Stardust == 500.0);
	CHECK(P.StardustGrantedEver == 7.0); // the Reawaken multiplier does not move
	CHECK(P.Catnip == 1000.0);
	Pc::CheatAddCurrency(P, 0.0, -50.0);
	CHECK(P.Stardust == 500.0); // nothing taken
}

TEST_CASE("Cheats: max upgrades puts every capped line at its cap and adds 25 to every uncapped one")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::CheatMaxAllUpgrades(D, P);
	int Capped = 0, Uncapped = 0;
	for (const Pc::PcUpgrade& U : D.Upgrades)
	{
		CAPTURE(U.Id);
		const int64_t L = Pc::UpgradeLevel(P, U.Id);
		if (U.Cap >= 0.0)
		{
			++Capped;
			const double Cap = Pc::EffectiveCap(U, P);
			CHECK(Effect(U, L) >= Cap * (1.0 - 1e-12)); // reaches the cap
			CHECK(Effect(U, L - 1) < Cap);              // and is the first level that does
		}
		else
		{
			++Uncapped;
			CHECK(L == Pc::CheatUncappedStep);
		}
	}
	CHECK(Capped > 0);
	CHECK(Uncapped > 0);

	// A second press: the uncapped lines move again, the capped ones stay.
	std::vector<int64_t> Before;
	for (const Pc::PcUpgrade& U : D.Upgrades)
	{
		Before.push_back(Pc::UpgradeLevel(P, U.Id));
	}
	Pc::CheatMaxAllUpgrades(D, P);
	for (size_t I = 0; I < D.Upgrades.size(); ++I)
	{
		const Pc::PcUpgrade& U = D.Upgrades[I];
		CAPTURE(U.Id);
		CHECK(Pc::UpgradeLevel(P, U.Id) == Before[I] + (U.Cap >= 0.0 ? 0 : Pc::CheatUncappedStep));
	}
}

TEST_CASE("Cheats: max upgrades never lowers a level")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	const Pc::PcUpgrade* Capped = nullptr;
	for (const Pc::PcUpgrade& U : D.Upgrades)
	{
		if (U.Cap >= 0.0)
		{
			Capped = &U;
			break;
		}
	}
	REQUIRE(Capped);
	P.UpgradeLevels[Capped->Id] = 100000;
	Pc::CheatMaxAllUpgrades(D, P);
	CHECK(Pc::UpgradeLevel(P, Capped->Id) == 100000);
}

TEST_CASE("Cheats: advance stage beats the next boss on the ladder, one stage a press, nothing at the top")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	REQUIRE(D.Code.EvolutionStageCount > 1);
	int64_t Stage = Pc::EvolutionStage(D, P);
	CHECK(Stage == 1);
	size_t Boss = 0;
	while (Stage < D.Code.EvolutionStageCount && Boss < D.Code.BossRealms.size())
	{
		CAPTURE(Stage);
		CHECK_FALSE(Pc::IsBossDefeated(P, D.Code.BossRealms[Boss]));
		REQUIRE(Pc::CheatAdvanceStage(D, P));
		CHECK(Pc::IsBossDefeated(P, D.Code.BossRealms[Boss])); // in ladder order
		CHECK(Pc::EvolutionStage(D, P) == Stage + 1);
		Stage = Pc::EvolutionStage(D, P);
		++Boss;
	}
	CHECK(Boss == D.Code.BossRealms.size());
	const auto Defeated = P.BossesDefeated;
	if (Stage >= D.Code.EvolutionStageCount)
	{
		CHECK_FALSE(Pc::CheatAdvanceStage(D, P));
	}
	CHECK(P.BossesDefeated == Defeated);
}

TEST_CASE("Cheats: unlock all tiers clears the bosses only; unlock all stages opens every biome")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::CheatUnlockAllTiers(D, P);
	for (const std::string& Realm : D.Code.BossRealms)
	{
		CAPTURE(Realm);
		CHECK(Pc::IsBossDefeated(P, Realm));
		CHECK_FALSE(Pc::IsBossClearedThisCycle(P, Realm)); // the permanent record only, as Godot sets it
	}
	CHECK(P.LifetimeCatnipThisCycle == 0.0);
	CHECK(P.DeepestBiomeIndex == 0);
	bool bSomeShut = false;
	for (const Pc::PcTier& T : D.Tiers)
	{
		bSomeShut = bSomeShut || !Pc::IsBiomeUnlocked(D, P, T); // the Catnip gate still holds
	}
	CHECK(bSomeShut);

	// It opens EVERY biome: all three gates, the stage, the Catnip and this cycle's passage (F18, 2026-11-03: the passage gate
	// came after the cheat, and until then it left every biome behind a boss shut).
	Pc::PcProgress Q = Fresh(D);
	Pc::CheatUnlockAllStages(D, Q);
	for (const std::string& Realm : D.Code.BossRealms)
	{
		CHECK(Pc::IsBossClearedThisCycle(Q, Realm));
	}
	for (const Pc::PcTier& T : D.Tiers)
	{
		CAPTURE(T.Id);
		CHECK(Pc::IsBiomeUnlocked(D, Q, T));
	}
	CHECK(Q.DeepestBiomeIndex == static_cast<int64_t>(D.Tiers.size()) - 1);
	CHECK(Q.Catnip == 0.0); // the gate is satisfied, the wallet untouched
	const double Raised = Q.LifetimeCatnipThisCycle;
	Q.LifetimeCatnipThisCycle = Raised * 10.0;
	Pc::CheatUnlockAllStages(D, Q);
	CHECK(Q.LifetimeCatnipThisCycle == Raised * 10.0); // never lowered
}

TEST_CASE("Cheats: the Reawaken count is set alone")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	P.Catnip = 123.0;
	Pc::CheatSetReawakenCount(P, D.Reawaken.UnicornUnlockReawakenCount);
	CHECK(P.ReawakenCount == D.Reawaken.UnicornUnlockReawakenCount);
	CHECK(P.Catnip == 123.0);
	CHECK(P.StardustGrantedEver == 0.0);
}

#endif
