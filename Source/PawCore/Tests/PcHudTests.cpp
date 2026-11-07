#if defined(PAWCORE_STANDALONE_TESTS)

// The HUD's decisions (PcHud, from hud.gd and tier_controller.gd's boss panel): which line each readout says, what is
// enabled, which tab wears a badge. Each case asserts both sides of its decision, so a test cannot pass on a constant.

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCheats.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcHud.h"
#include "PcJobs.h"
#include "PcAchievements.h"
#include "PcTree.h"
#include "PcProgress.h"
#include "PcStats.h"

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

	Pc::PcProgress Fresh(const Pc::PcGameData& D)
	{
		Pc::PcProgress P;
		Pc::NewGame(D, P);
		Pc::BeginBiome(D, P, D.Tiers.front().Id);
		return P;
	}

	Pc::PcField FieldIn(const Pc::PcGameData& D, const Pc::PcProgress& P, const std::string& TierId)
	{
		Pc::PcField F;
		Pc::BeginField(F, D, P, TierId, Pc::PcRect{ 0.0, 0.0, 1352.0, 1080.0 }, 11);
		return F;
	}

	// The first biome on the ladder that holds a boss.
	const Pc::PcTier& FirstBossTier(const Pc::PcGameData& D)
	{
		for (const Pc::PcTier& T : D.Tiers)
		{
			if (!T.BossData.empty())
			{
				return T;
			}
		}
		FAIL("no boss biome in the data");
		return D.Tiers.front();
	}
}

TEST_CASE("HUD: the shop's rows, multipliers and order")
{
	const Pc::PcGameData& D = Game();
	REQUIRE(Pc::ShopRowOrder().size() == 15);
	for (const std::string& Id : Pc::ShopRowOrder())
	{
		CHECK_MESSAGE(D.FindUpgrade(Id) != nullptr, Id);
	}
	CHECK(Pc::NextBuyMultiplier(1) == 5);
	CHECK(Pc::NextBuyMultiplier(5) == 10);
	CHECK(Pc::NextBuyMultiplier(10) == -1);
	CHECK(Pc::NextBuyMultiplier(-1) == 1);
	CHECK(Pc::NextBuyMultiplier(7) == 5); // unknown: counted as x1

	Pc::PcProgress P = Fresh(D);
	const double Cost = Pc::UpgradeCostById(D, P, "paw_damage");
	P.Catnip = Cost - 0.5;
	Pc::PcShopRow Row = Pc::ShopRow(D, P, "paw_damage", 1);
	CHECK(Row.Kind == Pc::PcShopRowKind::Price);
	CHECK(Row.Level == 0);
	CHECK(Row.Price == Cost);
	CHECK_FALSE(Row.bEnabled);
	P.Catnip = Cost;
	CHECK(Pc::ShopRow(D, P, "paw_damage", 1).bEnabled);
	// F30 (2026-11-04): inside the purchase's epsilon the row is lit, and the purchase agrees.
	P.Catnip = Cost - D.Code.CurrencyEpsilon * 0.5;
	CHECK(Pc::ShopRow(D, P, "paw_damage", 1).bEnabled);
	{
		Pc::PcProgress Q = P;
		CHECK(Pc::PurchaseUpgrade(D, Q, "paw_damage"));
	}

	// Hidden while its branch is unbought; shown once it is.
	P.BranchLevels["unlock_aura"] = 0;
	CHECK(Pc::ShopRow(D, P, "aura_radius", 1).Kind == Pc::PcShopRowKind::Hidden);
	P.BranchLevels["unlock_aura"] = 1;
	CHECK(Pc::ShopRow(D, P, "aura_radius", 1).Kind == Pc::PcShopRowKind::Price);

	// The kitten lines say "buy a Kitten first" until one is owned, whatever their visibility.
	P.UpgradeLevels["kitten_count"] = 0;
	CHECK(Pc::ShopRow(D, P, "kitten_power", 1).Kind == Pc::PcShopRowKind::KittenLocked);
	CHECK(Pc::ShopRow(D, P, "kitten_speed", 1).Kind == Pc::PcShopRowKind::KittenLocked);
	CHECK_FALSE(Pc::ShopRow(D, P, "kitten_power", 1).bEnabled);
	P.UpgradeLevels["kitten_count"] = 1;
	CHECK(Pc::ShopRow(D, P, "kitten_power", 1).Kind != Pc::PcShopRowKind::KittenLocked);
	CHECK(Pc::ShopRow(D, P, "kitten_speed", 1).Kind != Pc::PcShopRowKind::KittenLocked);

	// At the cap: MAX, disabled, whatever the bank.
	const Pc::PcUpgrade* Spawn = D.FindUpgrade("spawn_rate");
	REQUIRE(Spawn != nullptr);
	P.Catnip = 1e30;
	CHECK(Pc::ShopRow(D, P, "spawn_rate", 1).Kind == Pc::PcShopRowKind::Price);
	int64_t Guard = 0;
	while (!Pc::IsUpgradeAtCap(D, P, "spawn_rate") && Guard++ < 10000)
	{
		P.UpgradeLevels["spawn_rate"] += 1;
	}
	REQUIRE(Pc::IsUpgradeAtCap(D, P, "spawn_rate"));
	Row = Pc::ShopRow(D, P, "spawn_rate", 10);
	CHECK(Row.Kind == Pc::PcShopRowKind::Max);
	CHECK_FALSE(Row.bEnabled);
}

TEST_CASE("HUD: a bulk row prices the walked sum, and the press buys exactly what it said")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	// The walked sum of ten levels, priced level by level on a copy.
	Pc::PcProgress Walk = P;
	Walk.Catnip = 1e30;
	double Sum = 0.0;
	for (int I = 0; I < 10; ++I)
	{
		Sum += Pc::UpgradeCostById(D, Walk, "paw_damage");
		Walk.UpgradeLevels["paw_damage"] += 1;
	}
	P.Catnip = Sum;
	Pc::PcShopRow Row = Pc::ShopRow(D, P, "paw_damage", 10);
	CHECK(Row.Kind == Pc::PcShopRowKind::Bulk);
	CHECK(Row.Levels == 10);
	CHECK(Row.Price == doctest::Approx(Sum).epsilon(1e-12));
	CHECK(Row.bEnabled);
	CHECK(Row.Price > 10.0 * Pc::UpgradeCostById(D, P, "paw_damage")); // never "10x the current price"
	const double Before = P.Catnip;
	CHECK(Pc::PressShopRow(D, P, "paw_damage", 10) == 10);
	CHECK(Pc::UpgradeLevel(P, "paw_damage") == 10);
	CHECK(Before - P.Catnip == doctest::Approx(Row.Price).epsilon(1e-12));

	// Short of one level at x10: the row names ONE level's price and is disabled; the press buys nothing.
	P.Catnip = 0.0;
	Row = Pc::ShopRow(D, P, "paw_damage", 10);
	CHECK(Row.Kind == Pc::PcShopRowKind::Price);
	CHECK(Row.Price == Pc::UpgradeCostById(D, P, "paw_damage"));
	CHECK_FALSE(Row.bEnabled);
	CHECK(Pc::PressShopRow(D, P, "paw_damage", 10) == 0);

	// Max (-1): as many as the bank pays for, here exactly three (on a line three levels from its cap or more).
	Pc::PcProgress Three = Fresh(D);
	Walk = Three;
	double ThreeCost = 0.0;
	for (int I = 0; I < 3; ++I)
	{
		ThreeCost += Pc::UpgradeCostById(D, Walk, "currency_find");
		Walk.UpgradeLevels["currency_find"] += 1;
	}
	REQUIRE_FALSE(Pc::IsUpgradeAtCap(D, Walk, "currency_find"));
	Three.Catnip = ThreeCost + 0.25 * Pc::UpgradeCostById(D, Walk, "currency_find");
	Row = Pc::ShopRow(D, Three, "currency_find", -1);
	CHECK(Row.Kind == Pc::PcShopRowKind::Bulk);
	CHECK(Row.Levels == 3);
	CHECK(Pc::PressShopRow(D, Three, "currency_find", -1) == 3);
	// A line that reaches its cap first stops there, whatever the bank (Attack Speed caps early in a fresh game).
	Pc::PcProgress Capped = Fresh(D);
	Capped.Catnip = 1e30;
	const int64_t Bought = Pc::PressShopRow(D, Capped, "attack_speed", -1);
	CHECK(Pc::IsUpgradeAtCap(D, Capped, "attack_speed"));
	CHECK(Pc::ShopRow(D, Capped, "attack_speed", -1).Kind == Pc::PcShopRowKind::Max);
	CHECK(Bought == Pc::UpgradeLevel(Capped, "attack_speed"));
}

TEST_CASE("HUD: the Catches bar fills within the level")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	P.CatchesLevel = 3;
	const double Floor = Pc::TotalCatchesForLevel(D, 3);
	const double Next = Pc::TotalCatchesForLevel(D, 4);
	P.CatchesPoints = Floor;
	CHECK(Pc::CatchesBar(D, P).Fraction == 0.0);
	CHECK(Pc::CatchesBar(D, P).Level == 3);
	P.CatchesPoints = (Floor + Next) * 0.5;
	CHECK(Pc::CatchesBar(D, P).Fraction == doctest::Approx(0.5).epsilon(1e-12));
	CHECK(P.CatchesPoints / Next > 0.7); // the running-total ratio would read nearly full
	P.CatchesPoints = Next * 3.0;
	CHECK(Pc::CatchesBar(D, P).Fraction == 1.0);
}

TEST_CASE("HUD: the zone plate says progress, ready, bait or nothing")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcField F = FieldIn(D, P, D.Tiers.front().Id);

	// A fresh game: the first locked biome's Catnip gate.
	const Pc::PcTier* Locked = nullptr;
	for (const Pc::PcTier& T : D.Tiers)
	{
		if (!Pc::IsBiomeUnlocked(D, P, T))
		{
			Locked = &T;
			break;
		}
	}
	REQUIRE(Locked != nullptr);
	Pc::PcZonePlate Plate = Pc::ZonePlate(F, D, P);
	CHECK(Plate.Kind == Pc::PcZonePlateKind::Progress);
	CHECK(Plate.Need == Locked->CatnipToUnlock);
	CHECK(Plate.Have == doctest::Approx(Locked->CatnipToUnlock - Pc::CatnipUntilUnlocked(P, *Locked)));
	CHECK_FALSE(Plate.bRainbow);

	// Something open past the deepest biome reached: ready, full, rainbow.
	bool bOpened = false;
	for (int I = 0; I < 60 && !bOpened; ++I)
	{
		P.LifetimeCatnipThisCycle += Locked->CatnipToUnlock * 0.1 + 1.0;
		bOpened = Pc::IsBiomeUnlocked(D, P, *Locked);
	}
	if (bOpened && Pc::RealmIndex(D, Locked->Id) > P.DeepestBiomeIndex)
	{
		Plate = Pc::ZonePlate(F, D, P);
		CHECK(Plate.Kind == Pc::PcZonePlateKind::Ready);
		CHECK(Plate.Fraction == 1.0);
		CHECK(Plate.bRainbow);
		// ...until the player has been there.
		P.DeepestBiomeIndex = Pc::RealmIndex(D, Locked->Id);
		CHECK(Pc::ZonePlate(F, D, P).Kind != Pc::PcZonePlateKind::Ready);
	}
	else
	{
		FAIL_CHECK("the first locked biome never opened on Catnip: the Ready branch is untested");
	}

	// Everything open and visited: nothing to say.
	Pc::PcProgress All = Fresh(D);
	Pc::CheatUnlockAllStages(D, All);
	for (const Pc::PcTier& T : D.Tiers)
	{
		All.BossesClearedThisCycle.insert(T.Id);
	}
	All.DeepestBiomeIndex = static_cast<int64_t>(D.Tiers.size()) - 1;
	bool bAllOpen = true;
	for (const Pc::PcTier& T : D.Tiers)
	{
		bAllOpen = bAllOpen && Pc::IsBiomeUnlocked(D, All, T);
	}
	REQUIRE(bAllOpen);
	CHECK(Pc::ZonePlate(F, D, All).Kind == Pc::PcZonePlateKind::Hidden);

	// Bait first: in a boss biome, its bait held and paid for, the boss not beaten this cycle.
	const Pc::PcTier& BossTier = FirstBossTier(D);
	const Pc::PcBoss* Boss = D.FindBoss(BossTier.BossData);
	REQUIRE(Boss != nullptr);
	Pc::PcProgress B = Fresh(D);
	Pc::CheatUnlockAllTiers(D, B);
	Pc::BeginBiome(D, B, BossTier.Id);
	Pc::PcField BF = FieldIn(D, B, BossTier.Id);
	B.Catnip = 1e30;
	B.TierKillCount += Pc::BaitShortfall(D, B, *Boss) * Boss->KillsPerBait;
	REQUIRE(Pc::CanSummonFieldBoss(BF, D, B));
	B.BossesClearedThisCycle.erase(BossTier.Id);
	if (Pc::IsBaitUnlocked(D, B, *Boss))
	{
		Plate = Pc::ZonePlate(BF, D, B);
		CHECK(Plate.Kind == Pc::PcZonePlateKind::Bait);
		CHECK(Plate.BossId == BossTier.BossData);
		CHECK(Plate.bRainbow);
		// Bait beats "next biome ready" when both are true: a boss in the way is the bigger news (designer, 2026-10-01).
		const int64_t Deepest = B.DeepestBiomeIndex;
		const double CycleCatnip = B.LifetimeCatnipThisCycle;
		B.DeepestBiomeIndex = 0;
		B.LifetimeCatnipThisCycle = 1e30; // the Catnip gates open too (the cheat opened the boss gates only)
		bool bNewToo = false;
		for (int64_t I = 1; I < static_cast<int64_t>(D.Tiers.size()); ++I)
		{
			bNewToo = bNewToo || Pc::IsRealmNew(D, B, I);
		}
		REQUIRE(bNewToo);
		CHECK(Pc::ZonePlate(BF, D, B).Kind == Pc::PcZonePlateKind::Bait);
		B.DeepestBiomeIndex = Deepest;
		B.LifetimeCatnipThisCycle = CycleCatnip;
		B.BossesClearedThisCycle.insert(BossTier.Id); // a rematch does not shout
		CHECK(Pc::ZonePlate(BF, D, B).Kind != Pc::PcZonePlateKind::Bait);
		B.BossesClearedThisCycle.erase(BossTier.Id);
		B.Catnip = 0.0; // the bait unaffordable: not calling
		CHECK(Pc::ZonePlate(BF, D, B).Kind != Pc::PcZonePlateKind::Bait);
	}
	else
	{
		FAIL_CHECK("the first boss's bait is still locked after the cheat: the Bait branch is untested");
	}
}

TEST_CASE("HUD: the wave line counts bait where a boss lives, waves elsewhere")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	REQUIRE(D.Tiers.front().BossData.empty());
	Pc::PcWaveLine Line = Pc::WaveLine(D, P);
	CHECK(Line.bVisible == (Pc::WaveProgress(D, P).Costs > 0.0));
	CHECK_FALSE(Line.bBait);
	CHECK(Line.Wave == P.CurrentWaveIndex);

	const Pc::PcTier& BossTier = FirstBossTier(D);
	const Pc::PcBoss* Boss = D.FindBoss(BossTier.BossData);
	Pc::CheatUnlockAllTiers(D, P);
	Pc::BeginBiome(D, P, BossTier.Id);
	Line = Pc::WaveLine(D, P);
	REQUIRE(Line.bVisible);
	CHECK(Line.bBait);
	CHECK(Line.Needed == Boss->BaitRequired);
	CHECK(Line.Held == std::min(Pc::BaitHeld(D, P, BossTier.Id), Boss->BaitRequired));
	P.TierKillCount += (Boss->BaitRequired + 5) * Boss->KillsPerBait; // more than needed: clamped
	Line = Pc::WaveLine(D, P);
	CHECK(Line.Held == Boss->BaitRequired);
}

TEST_CASE("HUD: the Pounce clock and the boss panel")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcField F = FieldIn(D, P, D.Tiers.front().Id);
	P.BranchLevels["unlock_pounce"] = 0;
	CHECK_FALSE(Pc::PounceClock(F, P).bVisible);
	P.BranchLevels["unlock_pounce"] = 1;
	F.PounceCooldownFull = 5.0;
	F.PounceCooldownLeft = 5.0;
	Pc::PcPounceClock Clock = Pc::PounceClock(F, P);
	CHECK(Clock.bVisible);
	CHECK(Clock.Fraction == doctest::Approx(Pc::PounceCooldownFraction(F)));
	CHECK(Clock.Fraction < 0.01);
	F.PounceCooldownLeft = 0.0;
	CHECK(Pc::PounceClock(F, P).Fraction == 1.0);

	CHECK_FALSE(Pc::BossPanel(F, D).bVisible);
	const Pc::PcTier& BossTier = FirstBossTier(D);
	const Pc::PcBoss* Boss = D.FindBoss(BossTier.BossData);
	Pc::CheatUnlockAllTiers(D, P);
	Pc::BeginBiome(D, P, BossTier.Id);
	Pc::PcField BF = FieldIn(D, P, BossTier.Id);
	P.Catnip = 1e30;
	P.TierKillCount += Pc::BaitShortfall(D, P, *Boss) * Boss->KillsPerBait;
	REQUIRE(Pc::SummonFieldBoss(BF, D, P));
	Pc::PcBossPanel Panel = Pc::BossPanel(BF, D);
	CHECK(Panel.bVisible);
	CHECK(Panel.Fraction == 1.0);
	CHECK(Panel.Phase == 1);
	CHECK_FALSE(Panel.bTimerVisible); // not before the fight is engaged
	BF.BossHp = Boss->TotalHp * 0.25;
	BF.bBossShielded = true;
	Panel = Pc::BossPanel(BF, D);
	CHECK(Panel.Fraction == doctest::Approx(0.25));
	CHECK(Panel.bShielded);
	if (Boss->TimeLimitSec > 0.0)
	{
		BF.bBossEngagedOnce = true;
		BF.BossFightElapsed = Boss->TimeLimitSec - 30.0;
		Panel = Pc::BossPanel(BF, D);
		CHECK(Panel.bTimerVisible);
		CHECK(Panel.Remaining == doctest::Approx(30.0));
		CHECK_FALSE(Panel.bTimerDanger);
		BF.BossFightElapsed = Boss->TimeLimitSec - 10.0;
		CHECK(Pc::BossPanel(BF, D).bTimerDanger);
		BF.BossFightElapsed = Boss->TimeLimitSec + 50.0;
		CHECK(Pc::BossPanel(BF, D).Remaining == 0.0);
	}
	Pc::DismissFieldBoss(BF);
	CHECK_FALSE(Pc::BossPanel(BF, D).bVisible);
}

TEST_CASE("HUD: tab badges never sit on the open tab; Achievements count; Jobs appear with a job")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcField F = FieldIn(D, P, D.Tiers.front().Id);
	using T = Pc::PcDockTab;

	P.Catnip = 0.0;
	CHECK_FALSE(Pc::AnyUpgradeAffordable(D, P));
	CHECK_FALSE(Pc::TabState(F, D, P, T::Upgrades, T::Realms, 0).bBadge);
	P.Catnip = 1e30;
	CHECK(Pc::AnyUpgradeAffordable(D, P));
	CHECK(Pc::TabState(F, D, P, T::Upgrades, T::Realms, 0).bBadge);
	CHECK_FALSE(Pc::TabState(F, D, P, T::Upgrades, T::Upgrades, 0).bBadge); // open: no star

	CHECK_FALSE(Pc::TabState(F, D, P, T::Catches, T::Upgrades, 0).bBadge);
	P.Hand = { D.Cards.front().Id };
	CHECK(Pc::TabState(F, D, P, T::Catches, T::Upgrades, 0).bBadge);

	CHECK_FALSE(Pc::TabState(F, D, P, T::Jobs, T::Upgrades, 0).bVisible);
	REQUIRE_FALSE(D.Jobs.empty());
	P.JobsHeld.insert(D.Jobs.front().Id);
	Pc::PcTabState Jobs = Pc::TabState(F, D, P, T::Jobs, T::Upgrades, 0);
	CHECK(Jobs.bVisible);
	CHECK(Jobs.bBadge);
	CHECK_FALSE(Pc::TabState(F, D, P, T::Jobs, T::Upgrades, 1).bBadge); // seen

	const bool bSuggested = Pc::IsReawakenSuggested(D, P);
	CHECK(Pc::TabState(F, D, P, T::Reawaken, T::Upgrades, 0).bBadge == bSuggested);
	P.StardustGrantedEver = 0.0;

	// Realms: a biome open past the deepest one reached.
	Pc::PcProgress R = Fresh(D);
	CHECK(Pc::TabState(F, D, R, T::Realms, T::Upgrades, 0).bBadge == false);
	Pc::CheatUnlockAllStages(D, R);
	for (const Pc::PcTier& Tier : D.Tiers)
	{
		R.BossesClearedThisCycle.insert(Tier.Id);
	}
	R.DeepestBiomeIndex = 0; // the cheat also sets the deepest biome reached (F18's note)
	CHECK(Pc::IsRealmNew(D, R, 1));
	CHECK_FALSE(Pc::IsRealmNew(D, R, 0)); // where the player already is
	CHECK(Pc::TabState(F, D, R, T::Realms, T::Upgrades, 0).bBadge);
	R.DeepestBiomeIndex = static_cast<int64_t>(D.Tiers.size()) - 1;
	CHECK_FALSE(Pc::TabState(F, D, R, T::Realms, T::Upgrades, 0).bBadge);
}

TEST_CASE("HUD: the Realms page's rows say here, travel, boss, the Catnip still owed, or locked")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	REQUIRE(D.Tiers.size() >= 3);

	Pc::PcRealmRow Row = Pc::RealmRow(D, P, 0);
	CHECK(Row.Kind == Pc::PcRealmRowKind::Here);
	CHECK_FALSE(Row.bEnabled);

	// The second biome: its stage reached in a fresh game, its Catnip not earned.
	REQUIRE(Pc::IsTierUnlocked(D, P, D.Tiers[1].RequiredEvolutionStage));
	REQUIRE(Pc::CatnipUntilUnlocked(P, D.Tiers[1]) > 0.0);
	Row = Pc::RealmRow(D, P, 1);
	CHECK(Row.Kind == Pc::PcRealmRowKind::NeedsCatnip);
	CHECK(Row.Owed == Pc::CatnipUntilUnlocked(P, D.Tiers[1]));
	CHECK_FALSE(Row.bEnabled);
	P.LifetimeCatnipThisCycle += D.Tiers[1].CatnipToUnlock + 1.0;
	Row = Pc::RealmRow(D, P, 1);
	CHECK(Row.Kind == Pc::PcRealmRowKind::Travel);
	CHECK(Row.bEnabled);
	CHECK(Row.bNew == Pc::IsRealmNew(D, P, 1));
	CHECK_FALSE(Row.bShowReward);
	P.BranchLevels["cartography"] = 1;
	if (Pc::BranchEffect(D, P, "cartography") > 0.0)
	{
		Row = Pc::RealmRow(D, P, 1);
		CHECK(Row.bShowReward);
		CHECK(Row.RewardMultiplier == D.Tiers[1].RewardMultiplier);
	}
	else
	{
		FAIL_CHECK("Cartography's level 1 has no effect in the data: the reward line is untested");
	}

	// A biome whose evolution stage is not reached: locked, whatever the Catnip.
	int64_t Gated = -1;
	for (int64_t I = 0; I < static_cast<int64_t>(D.Tiers.size()) && Gated < 0; ++I)
	{
		if (!Pc::IsTierUnlocked(D, P, D.Tiers[static_cast<size_t>(I)].RequiredEvolutionStage))
		{
			Gated = I;
		}
	}
	REQUIRE(Gated >= 0);
	if (Pc::CatnipUntilUnlocked(P, D.Tiers[static_cast<size_t>(Gated)]) > 0.0)
	{
		CHECK(Pc::RealmRow(D, P, Gated).Kind == Pc::PcRealmRowKind::Locked); // the stage first: no "earn ... more" yet
	}
	else
	{
		FAIL_CHECK("the stage-gated biome owes no Catnip: the stage-before-Catnip order is untested");
	}
	P.LifetimeCatnipThisCycle = 1e30;
	CHECK(Pc::RealmRow(D, P, Gated).Kind == Pc::PcRealmRowKind::Locked);

	// A boss biome, open: names its boss; once beaten, cleared.
	Pc::CheatUnlockAllTiers(D, P);
	const int64_t BossIndex = Pc::RealmIndex(D, D.Code.BossRealms.front());
	Row = Pc::RealmRow(D, P, BossIndex);
	REQUIRE(Row.Kind == Pc::PcRealmRowKind::TravelBoss);
	CHECK(Row.BossId == D.Tiers[static_cast<size_t>(BossIndex)].BossData);
	CHECK(Row.bCleared == Pc::IsBossDefeated(P, D.Code.BossRealms.front()));
	P.BossesDefeated.erase(D.Code.BossRealms.front());
	CHECK_FALSE(Pc::RealmRow(D, P, BossIndex).bCleared);
	P.BossesDefeated.insert(D.Code.BossRealms.front());
	CHECK(Pc::RealmRow(D, P, BossIndex).bCleared);
}

TEST_CASE("HUD: after a Reawaken the Realms rows re-lock behind every boss not beaten this run (F22)")
{
	// Designer 2026-11-03 ("that should not happen"): the rows ask the passage, as auto-travel and the zone bar do. The
	// permanent stage is kept (every boss beaten ever), this run's clears are gone, the Catnip is no object.
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::CheatUnlockAllTiers(D, P);
	REQUIRE(D.Code.BossRealms.size() >= 2);
	P.BossesClearedThisCycle.clear();
	P.LifetimeCatnipThisCycle = 1e30;
	const int64_t FirstBoss = Pc::RealmIndex(D, D.Code.BossRealms.front());
	int64_t Checked = 0;
	for (int64_t I = 0; I < static_cast<int64_t>(D.Tiers.size()); ++I)
	{
		const Pc::PcTier& Tier = D.Tiers[static_cast<size_t>(I)];
		REQUIRE(Pc::IsTierUnlocked(D, P, Tier.RequiredEvolutionStage)); // the permanent ladder: every biome
		const Pc::PcRealmRow Row = Pc::RealmRow(D, P, I);
		if (Tier.RequiredEvolutionStage <= 1)
		{
			CHECK(Row.bEnabled == (Tier.Id != P.CurrentBiomeId)); // before the first boss: open as ever
			continue;
		}
		CHECK(Row.Kind == Pc::PcRealmRowKind::Locked);
		CHECK_FALSE(Row.bEnabled);
		++Checked;
	}
	CHECK(Checked > 0);
	// Still owing Catnip behind a boss: "locked", not "earn ... more" (the boss first, as the stage was).
	P.LifetimeCatnipThisCycle = 0.0;
	const int64_t Last = static_cast<int64_t>(D.Tiers.size()) - 1;
	REQUIRE(Pc::CatnipUntilUnlocked(P, D.Tiers[static_cast<size_t>(Last)]) > 0.0);
	CHECK(Pc::RealmRow(D, P, Last).Kind == Pc::PcRealmRowKind::Locked);
	// Standing in a re-locked biome (Territory's start): still "you are here".
	P.CurrentBiomeId = D.Tiers[static_cast<size_t>(Last)].Id;
	CHECK(Pc::RealmRow(D, P, Last).Kind == Pc::PcRealmRowKind::Here);
	// Beat the first boss this run: the block behind it opens again, the next stays shut.
	P.CurrentBiomeId = D.Tiers.front().Id;
	P.LifetimeCatnipThisCycle = 1e30;
	P.BossesClearedThisCycle.insert(D.Code.BossRealms.front());
	for (int64_t I = FirstBoss + 1; I < static_cast<int64_t>(D.Tiers.size()); ++I)
	{
		const Pc::PcTier& Tier = D.Tiers[static_cast<size_t>(I)];
		CHECK(Pc::RealmRow(D, P, I).bEnabled == (Tier.RequiredEvolutionStage <= 2));
		CHECK(Pc::RealmRow(D, P, I).bEnabled == Pc::IsBiomeUnlocked(D, P, Tier)); // the page agrees with the rule
	}
}

TEST_CASE("HUD: the stage line names the boss that grants the next stage")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcStageLine Line = Pc::StageLine(D, P);
	CHECK(Line.Stage == Pc::EvolutionStage(D, P));
	CHECK_FALSE(Line.bFullyEvolved);
	CHECK(Line.NextBossTier == D.Code.BossRealms[static_cast<size_t>(Line.Stage - 1)]);
	REQUIRE(Pc::CheatAdvanceStage(D, P));
	Line = Pc::StageLine(D, P);
	CHECK(Line.NextBossTier == D.Code.BossRealms[static_cast<size_t>(Line.Stage - 1)]);
	// Every boss beaten: stage 6, and no boss on the ladder grants the seventh (the ending does): "the next boss".
	Pc::CheatUnlockAllTiers(D, P);
	Line = Pc::StageLine(D, P);
	CHECK(Line.Stage == static_cast<int64_t>(D.Code.BossRealms.size()) + 1);
	CHECK_FALSE(Line.bFullyEvolved);
	CHECK(Line.NextBossTier.empty());
	P.bFinalBossDefeated = true;
	Line = Pc::StageLine(D, P);
	CHECK(Line.Stage == D.Code.EvolutionStageCount);
	CHECK(Line.bFullyEvolved);
}

TEST_CASE("HUD: the Bait button points elsewhere, names the bait or Catnip owed, offers the summon, or says the boss is here")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcField F = FieldIn(D, P, D.Tiers.front().Id);
	Pc::PcBaitButton B = Pc::BaitButton(F, D, P);
	CHECK(B.Kind == Pc::PcBaitKind::Elsewhere);
	CHECK(B.NextBossTier == D.Code.BossRealms.front());
	CHECK_FALSE(B.bEnabled);
	P.BossesClearedThisCycle.insert(D.Code.BossRealms.front());
	CHECK(Pc::BaitButton(F, D, P).NextBossTier == D.Code.BossRealms[1]); // the first NOT beaten this run
	for (const std::string& Id : D.Code.BossRealms)
	{
		P.BossesClearedThisCycle.insert(Id);
	}
	CHECK(Pc::BaitButton(F, D, P).Kind == Pc::PcBaitKind::Hidden);

	const Pc::PcTier& BossTier = FirstBossTier(D);
	const Pc::PcBoss* Boss = D.FindBoss(BossTier.BossData);
	Pc::PcProgress Q = Fresh(D);
	Pc::CheatUnlockAllTiers(D, Q);
	Q.BossesClearedThisCycle.clear();
	Pc::BeginBiome(D, Q, BossTier.Id);
	Pc::PcField BF = FieldIn(D, Q, BossTier.Id);
	Q.Catnip = 0.0;
	REQUIRE(Pc::BaitShortfall(D, Q, *Boss) > 0);
	B = Pc::BaitButton(BF, D, Q);
	CHECK(B.Kind == Pc::PcBaitKind::BaitOwed);
	CHECK(B.BaitOwed == Pc::BaitShortfall(D, Q, *Boss));
	CHECK(B.BossId == BossTier.BossData);
	CHECK_FALSE(B.bEnabled);
	Q.TierKillCount += Pc::BaitShortfall(D, Q, *Boss) * Boss->KillsPerBait;
	if (Boss->BaitCostCatnip > 0.0)
	{
		B = Pc::BaitButton(BF, D, Q);
		CHECK(B.Kind == Pc::PcBaitKind::CatnipShort);
		CHECK(B.CatnipShort == doctest::Approx(Boss->BaitCostCatnip - Q.Catnip));
	}
	Q.Catnip = Boss->BaitCostCatnip + 1.0;
	REQUIRE(Pc::IsBaitUnlocked(D, Q, *Boss));
	B = Pc::BaitButton(BF, D, Q);
	CHECK(B.Kind == Pc::PcBaitKind::Ready);
	CHECK(B.bEnabled);
	CHECK(B.bEnabled == Pc::IsBaitUnlocked(D, Q, *Boss));
	CHECK(B.bCalling == Pc::BaitIsCalling(BF, D, Q));
	Q.Catnip = 1e30;
	REQUIRE(Pc::SummonFieldBoss(BF, D, Q));
	B = Pc::BaitButton(BF, D, Q);
	CHECK(B.Kind == Pc::PcBaitKind::BossHere);
	CHECK_FALSE(B.bEnabled);
	// The boss up and the next bait already earned: still "is here", never an offer.
	Q.TierKillCount += Pc::BaitShortfall(D, Q, *Boss) * Boss->KillsPerBait;
	REQUIRE(Pc::BaitShortfall(D, Q, *Boss) == 0);
	B = Pc::BaitButton(BF, D, Q);
	CHECK(B.Kind == Pc::PcBaitKind::BossHere);
	CHECK_FALSE(B.bEnabled);
}

TEST_CASE("HUD: the Reawaken page states the payout and what the button will do")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcReawakenPage Page = Pc::ReawakenPage(D, P, false);
	CHECK(Page.bFirst);
	CHECK_FALSE(Page.bReady);
	CHECK(Page.Button == Pc::PcReawakenButtonKind::NotYet);
	CHECK(Page.Pending == Pc::PendingStardust(D, P));
	CHECK(Page.TotalAfter == P.Stardust + Page.Pending);
	CHECK(Page.Fraction >= 0.0);
	CHECK(Page.Fraction < 1.0);
	CHECK(Pc::ReawakenPage(D, P, true).Button == Pc::PcReawakenButtonKind::Confirm);

	// Past the gate: Reawaken, the multiplier after above the one now.
	P.LifetimeCatnipEver = 1e12;
	P.LifetimeCatnipThisCycle = 1e12;
	Page = Pc::ReawakenPage(D, P, false);
	REQUIRE(Page.bReady);
	CHECK(Page.Button == Pc::PcReawakenButtonKind::Reawaken);
	CHECK(Page.Fraction == 1.0);
	CHECK(Page.MultiplierAfter > Page.MultiplierNow);
	CHECK(Page.Count == 0);
	P.Stardust = 5.0;
	Page = Pc::ReawakenPage(D, P, false);
	REQUIRE(Page.Pending > 0.0);
	CHECK(Page.TotalAfter == 5.0 + Page.Pending);

	// After the first: early (one Stardust in, below the suggestion) says what it cashes out.
	Pc::PcProgress E = Fresh(D);
	E.ReawakenCount = 1;
	bool bFound = false;
	for (double L = 1.0; L < 1e15 && !bFound; L *= 1.5)
	{
		E.LifetimeCatnipEver = L;
		bFound = Pc::CanReawaken(D, E) && !Pc::IsReawakenSuggested(D, E);
	}
	REQUIRE(bFound);
	Page = Pc::ReawakenPage(D, E, false);
	CHECK_FALSE(Page.bFirst);
	CHECK(Page.Button == Pc::PcReawakenButtonKind::Early);
	CHECK(Page.Pending >= 1.0);

	CHECK_FALSE(Pc::ReawakenPage(D, P, false).bUnicorn);
	P.BranchLevels["unlock_unicorn"] = 1;
	CHECK(Pc::ReawakenPage(D, P, false).bUnicorn);
}

namespace
{
	const Pc::PcCatchesRow& RowFor(const Pc::PcCatchesPage& Page, const std::string& Id)
	{
		for (const Pc::PcCatchesRow& R : Page.Rows)
		{
			if (R.CardId == Id)
			{
				return R;
			}
		}
		FAIL("no Catches row for " << Id);
		static Pc::PcCatchesRow None;
		return None;
	}
}

TEST_CASE("HUD: card numbers drop trailing zeros (CardData._fmt)")
{
	CHECK(Pc::CardNumber(2.0) == "2");
	CHECK(Pc::CardNumber(12.0) == "12");
	CHECK(Pc::CardNumber(0.35) == "0.35");
	CHECK(Pc::CardNumber(0.1) == "0.1");
	CHECK(Pc::CardNumber(0.05) == "0.05");
	CHECK(Pc::CardNumber(1.5) == "1.5");
	CHECK(Pc::CardNumber(0.7) == "0.7");
	CHECK(Pc::CardNumber(2.000001) == "2");  // is_equal_approx: whole
	CHECK(Pc::CardNumber(12345.006) == "12345"); // its tolerance is relative (1e-5 x value): whole, not "12345.01"
	CHECK(Pc::CardNumber(0.0) == "0");
}

TEST_CASE("HUD: the Catches page lists every card, held ones with their level, the rest with what stands in the way")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcCatchesPage Page = Pc::CatchesPage(D, P, false);
	REQUIRE(Page.Rows.size() == D.Cards.size());
	for (size_t I = 0; I < D.Cards.size(); ++I)
	{
		CHECK(Page.Rows[I].CardId == D.Cards[I].Id); // the deck's order
	}
	CHECK(Page.Taken == 0);
	CHECK_FALSE(Page.bPickingAuto);
	CHECK_FALSE(Page.bAutoPickShown);
	// Nothing owned: the Pounce cards name the Pounce branch, the aura cards the Aura branch.
	CHECK(RowFor(Page, "bloodlust").Hint.Kind == Pc::PcCardHintKind::Requirement);
	CHECK(RowFor(Page, "bloodlust").Hint.Requirement == "pounce");
	CHECK(RowFor(Page, "close_quarters").Hint.Requirement == "aura");
	// A card with nothing in the way says nothing ("Not taken yet.").
	CHECK(RowFor(Page, "bounty").Hint.Kind == Pc::PcCardHintKind::None);
	CHECK_FALSE(RowFor(Page, "bounty").bHeld);
	// Feral Focus: a Pounce effect paid in aura radius. The effect's gate is named first, then the cost's.
	CHECK(RowFor(Page, "feral_focus").Hint.Requirement == "pounce");
	P.BranchLevels["unlock_pounce"] = 1;
	Page = Pc::CatchesPage(D, P, false);
	CHECK(RowFor(Page, "feral_focus").Hint.Requirement == "aura");
	CHECK(RowFor(Page, "bloodlust").Hint.Kind == Pc::PcCardHintKind::None);
	P.BranchLevels["unlock_aura"] = 1;
	CHECK(RowFor(Pc::CatchesPage(D, P, false), "feral_focus").Hint.Kind == Pc::PcCardHintKind::None);

	// Lone Hunter costs kittens: none is a requirement, too few to pay is the price.
	CHECK(RowFor(Page, "lone_hunter").Hint.Requirement == "kitten");
	P.UpgradeLevels["kitten_count"] = 0;
	while (Pc::KittenCount(D, P) < 2 && P.UpgradeLevels["kitten_count"] < 50)
	{
		++P.UpgradeLevels["kitten_count"];
	}
	REQUIRE(Pc::KittenCount(D, P) == 2); // pays 2, and one has to survive
	CHECK(RowFor(Pc::CatchesPage(D, P, false), "lone_hunter").Hint.Kind == Pc::PcCardHintKind::NotEnoughKittens);
	while (Pc::KittenCount(D, P) < 3)
	{
		++P.UpgradeLevels["kitten_count"];
	}
	CHECK(RowFor(Pc::CatchesPage(D, P, false), "lone_hunter").Hint.Kind == Pc::PcCardHintKind::None);

	// Held: level, max, and the value and cost at that level for describe().
	P.CardLevels["bounty"] = 2;
	P.CardLevels["close_quarters"] = 1;
	P.CardLevels["dilated"] = 2;          // its last level
	P.BanishedCards.insert("spite");
	Page = Pc::CatchesPage(D, P, true);
	CHECK(Page.Taken == 3);
	CHECK(Page.bPickingAuto);
	const Pc::PcCatchesRow& Bounty = RowFor(Page, "bounty");
	CHECK(Bounty.bHeld);
	CHECK(Bounty.Level == 2);
	CHECK(Bounty.MaxLevel == 3);
	CHECK(Bounty.Value == 3.0);
	const Pc::PcCatchesRow& Close = RowFor(Page, "close_quarters");
	CHECK(Close.Value == 5.0);
	CHECK(Close.Cost == doctest::Approx(0.7));
	CHECK(RowFor(Page, "dilated").Hint.Kind == Pc::PcCardHintKind::MaxLevel);
	CHECK(RowFor(Page, "spite").Hint.Kind == Pc::PcCardHintKind::Banished);
	CHECK_FALSE(RowFor(Page, "spite").bHeld);
	// Banished outranks everything else (it is the first gate).
	P.BanishedCards.insert("lone_hunter");
	P.UpgradeLevels["kitten_count"] = 0;
	CHECK(RowFor(Pc::CatchesPage(D, P, false), "lone_hunter").Hint.Kind == Pc::PcCardHintKind::Banished);

	// The Auto-pick box appears with Auto-Card owned, ticked or not.
	P.BranchLevels["auto_card"] = 1;
	CHECK(Pc::CatchesPage(D, P, false).bAutoPickShown);
	CHECK_FALSE(Pc::CatchesPage(D, P, false).bPickingAuto);
}

TEST_CASE("HUD: the Achievements page: claimed, earned, secret or progress; the summary counts what waits")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	Pc::PcAchievementsPage Page = Pc::AchievementsPage(D, P);
	REQUIRE(Page.Rows.size() == D.Achievements.size());
	CHECK(Page.Total == static_cast<int64_t>(D.Achievements.size()));
	CHECK(Page.Earned == 0);
	CHECK(Page.Waiting == 0);
	CHECK(Page.Rows.front().Id == D.Achievements.front().Id);
	const Pc::PcAchievement& First = D.Achievements.front();
	CHECK(Page.Rows.front().Kind == Pc::PcAchievementRowKind::Progress);
	CHECK(Page.Rows.front().Threshold == First.Threshold);
	CHECK(Page.Rows.front().Value == Pc::AchievementProgress(D, P, First));
	CHECK_FALSE(Page.Rows.front().bClaimable);

	// Earned: claimable until claimed, then claimed; the summary and Claim all follow.
	const Pc::PcAchievement& A = D.Achievements[0];
	const Pc::PcAchievement& B = D.Achievements[1];
	P.AchievementsEarned.insert(A.Id);
	P.AchievementsEarned.insert(B.Id);
	Page = Pc::AchievementsPage(D, P);
	CHECK(Page.Rows[0].Kind == Pc::PcAchievementRowKind::Earned);
	CHECK(Page.Rows[0].bClaimable == (A.StardustReward > 0.0));
	CHECK(Page.Rows[0].Reward == A.StardustReward);
	CHECK(Page.Earned == 2);
	CHECK(Page.Waiting == 2);
	CHECK(Page.WaitingStardust == A.StardustReward + B.StardustReward);
	const double Before = P.Stardust;
	CHECK(Pc::ClaimAchievement(D, P, A.Id) == A.StardustReward);
	CHECK(Pc::ClaimAchievement(D, P, A.Id) == 0.0); // pays exactly once
	CHECK(P.Stardust == Before + A.StardustReward);
	Page = Pc::AchievementsPage(D, P);
	CHECK(Page.Rows[0].Kind == Pc::PcAchievementRowKind::Claimed);
	CHECK_FALSE(Page.Rows[0].bClaimable);
	CHECK(Page.Rows[1].Kind == Pc::PcAchievementRowKind::Earned);
	CHECK(Page.Waiting == 1);
	CHECK(Page.WaitingStardust == B.StardustReward);
	CHECK(Pc::ClaimAllAchievements(D, P) == B.StardustReward);
	CHECK(Pc::ClaimAllAchievements(D, P) == 0.0);
	CHECK(Pc::AchievementsPage(D, P).Waiting == 0);
	CHECK(P.Stardust == Before + A.StardustReward + B.StardustReward);

	// A secret one says nothing until it is earned (no shipped achievement is secret: a copy of the data).
	Pc::PcGameData Secret = D;
	Secret.Achievements[2].bSecret = true;
	CHECK(Pc::AchievementsPage(Secret, P).Rows[2].Kind == Pc::PcAchievementRowKind::Secret);
	P.AchievementsEarned.insert(Secret.Achievements[2].Id);
	CHECK(Pc::AchievementsPage(Secret, P).Rows[2].Kind == Pc::PcAchievementRowKind::Earned);
}

TEST_CASE("HUD: the Jobs page: worn, owned, or the step that stands between the player and the job")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	auto Row = [&](const Pc::PcGameData& G, const std::string& Id) {
		for (const Pc::PcJobRow& R : Pc::JobRows(G, P))
		{
			if (R.JobId == Id)
			{
				return R;
			}
		}
		FAIL("no Jobs row for " << Id);
		return Pc::PcJobRow{};
	};
	const std::vector<Pc::PcJobRow> Rows = Pc::JobRows(D, P);
	const std::vector<const Pc::PcJob*> All = Pc::AllJobs(D);
	REQUIRE(Rows.size() == All.size());
	for (size_t I = 0; I < All.size(); ++I)
	{
		CHECK(Rows[I].JobId == All[I]->Id); // all_jobs()'s order
	}
	// Gated by a boss (Blacksmith: the tier_6 boss), by Reawakens (Magician: one), else ready at the node's price.
	CHECK(Row(D, "blacksmith").Kind == Pc::PcJobRowKind::BossGate);
	CHECK(Row(D, "blacksmith").BossTier == "tier_6");
	CHECK(Row(D, "magician").Kind == Pc::PcJobRowKind::ReawakenGate);
	CHECK(Row(D, "magician").Reawakens == 1);
	CHECK(Row(D, "unemployed").Reawakens == 3);
	P.BossesDefeated.insert("tier_6");
	CHECK(Row(D, "blacksmith").Kind == Pc::PcJobRowKind::Ready);
	CHECK(Row(D, "blacksmith").Cost == std::trunc(Pc::NextBranchCost(D, P, "job_blacksmith")));
	CHECK(Row(D, "blacksmith").Cost > 0.0);
	P.ReawakenCount = 1;
	CHECK(Row(D, "magician").Kind == Pc::PcJobRowKind::Ready);
	// The rival first: Daredevil bought closes Blacksmith, whatever else.
	P.BranchLevels["job_daredevil"] = 1;
	CHECK(Row(D, "blacksmith").Kind == Pc::PcJobRowKind::Closed);
	CHECK(Row(D, "blacksmith").RivalBranch == "job_daredevil");
	P.BossesDefeated.erase("tier_6");
	CHECK(Row(D, "blacksmith").Kind == Pc::PcJobRowKind::Closed);
	// Held: owned; worn when it is the run's job (not merely the standing choice).
	P.JobsHeld.insert("daredevil");
	P.CurrentJob = "daredevil";
	CHECK(Row(D, "daredevil").Kind == Pc::PcJobRowKind::Owned);
	P.RunJob = "daredevil";
	CHECK(Row(D, "daredevil").Kind == Pc::PcJobRowKind::Worn);
	// No node hands it over (none in the shipped data: a copy).
	Pc::PcGameData NoNode = D;
	for (Pc::PcSkillBranch& B : NoNode.Branches)
	{
		if (B.JobId == "sphinx")
		{
			B.JobId.clear();
		}
	}
	CHECK(Row(NoNode, "sphinx").Kind == Pc::PcJobRowKind::NoNode);
	CHECK(Row(D, "sphinx").Kind != Pc::PcJobRowKind::NoNode);
}

TEST_CASE("HUD: the card pick says the level, each card's next level and price, the reroll and the banish")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P = Fresh(D);
	CHECK_FALSE(Pc::CardPick(D, P).bOpen);  // no hand, no screen
	P.CatchesLevel = 4;
	P.Hand = { "bounty", "close_quarters", "lone_hunter" };
	P.CardLevels["bounty"] = 1;
	Pc::PcCardPick Pick = Pc::CardPick(D, P);
	CHECK(Pick.bOpen);
	CHECK(Pick.Level == 4);
	REQUIRE(Pick.Cards.size() == 3);
	CHECK(Pick.Cards[0].CardId == "bounty");          // deal order
	CHECK(Pick.Cards[2].CardId == "lone_hunter");
	// Bounty held at 1: taking it gives level 2 of 3, its value at 2; a doubling-down, not a new start.
	CHECK(Pick.Cards[0].Level == 2);
	CHECK(Pick.Cards[0].MaxLevel == 3);
	CHECK_FALSE(Pick.Cards[0].bNew);
	CHECK(Pick.Cards[0].Value == 3.0);
	CHECK_FALSE(Pick.Cards[0].bCosts);
	// Close Quarters not held: NEW, its first level's value and price; it costs aura radius.
	CHECK(Pick.Cards[1].bNew);
	CHECK(Pick.Cards[1].Level == 1);
	CHECK(Pick.Cards[1].Value == 5.0);
	CHECK(Pick.Cards[1].Cost == doctest::Approx(0.7));
	CHECK(Pick.Cards[1].bCosts);
	// The flavour band: Bounty is not an aura card (the default), so a flavour left unset would show.
	CHECK(Pick.Cards[0].Flavour == Pc::CardFlavour::FORTUNE);
	CHECK(Pick.Cards[1].Flavour == Pc::CardFlavour::AURA);
	// No rerolls, no banish, nothing promised: none of their lines.
	CHECK(Pick.Rerolls == 0);
	CHECK_FALSE(Pick.bBanish);
	CHECK(Pick.Foresight.empty());
	P.Rerolls = 2;
	P.Banishes = 1;
	P.PromisedCards = { "spite" };
	Pick = Pc::CardPick(D, P);
	CHECK(Pick.Rerolls == 2);
	CHECK(Pick.bBanish);
	REQUIRE(Pick.Foresight.size() == 1);
	CHECK(Pick.Foresight[0] == "spite");
	P.Rerolls = -1; // never a negative count on the button
	CHECK(Pc::CardPick(D, P).Rerolls == 0);
	// An id the data does not hold is skipped, never drawn blank.
	P.Hand.push_back("no_such_card");
	CHECK(Pc::CardPick(D, P).Cards.size() == 3);
}

#endif
