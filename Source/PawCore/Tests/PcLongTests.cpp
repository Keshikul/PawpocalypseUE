#if defined(PAWCORE_STANDALONE_TESTS)

// LONG TEST B (docs/09 §2): property tests over randomised states, millions of cases. Skipped by the normal L1 run;
// run them with
//   powershell -ExecutionPolicy Bypass -File tools\build_pawcore_tests.ps1 -Long
// PAWCORE_LONG_SCALE (default 1) multiplies every case count: 0.01 is a smoke run, 1 is the ~30-minute nightly.
// Seeds are fixed and printed, so a failure reproduces.
//
// Each property is a rule the Godot code states in a comment or the design depends on; the point of running millions
// of states is the corner a fixture never reaches (a level past a cap, a branch combination nobody buys together, a
// magnitude near overflow).

#include "ThirdParty/doctest/doctest.h"
#include "PcCombat.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcFormat.h"
#include "PcRng.h"
#include "PcStats.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	const Pc::PcGameData& Game()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(DataDir(), Data, Errors));
			bLoaded = true;
		}
		return Data;
	}

	int64_t Cases(int64_t Nightly)
	{
		const char* Env = std::getenv("PAWCORE_LONG_SCALE");
		const double Scale = Env ? std::strtod(Env, nullptr) : 1.0;
		return std::max<int64_t>(1, static_cast<int64_t>(static_cast<double>(Nightly) * Scale));
	}

	struct Timer
	{
		std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();
		double Seconds() const { return std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count(); }
	};

	// A random progress state anywhere in (and past) the game: levels past every cap, any branch at any level, owned cards,
	// any job pair, deep Reawaken counts, Stardust to 1e12.
	Pc::PcProgress RandomProgress(const Pc::PcGameData& D, std::mt19937_64& R)
	{
		std::uniform_real_distribution<double> U(0.0, 1.0);
		auto Below = [&R](int64_t N) { return static_cast<int64_t>(R() % static_cast<uint64_t>(N)); };
		Pc::PcProgress P;
		for (const Pc::PcUpgrade& Up : D.Upgrades)
		{
			if (U(R) < 0.8)
			{
				P.UpgradeLevels[Up.Id] = Below(121);
			}
		}
		for (const Pc::PcSkillBranch& B : D.Branches)
		{
			if (U(R) < 0.5 && !B.LevelCosts.empty())
			{
				P.BranchLevels[B.Id] = 1 + Below(static_cast<int64_t>(B.LevelCosts.size()));
			}
		}
		for (const Pc::PcCard& C : D.Cards)
		{
			if (U(R) < 0.3 && !C.LevelValues.empty())
			{
				P.CardLevels[C.Id] = 1 + Below(static_cast<int64_t>(C.LevelValues.size()));
			}
		}
		if (U(R) < 0.7)
		{
			P.RunJob = D.Jobs[static_cast<size_t>(Below(static_cast<int64_t>(D.Jobs.size())))].Id;
		}
		if (U(R) < 0.5)
		{
			P.RunJobSecond = D.Jobs[static_cast<size_t>(Below(static_cast<int64_t>(D.Jobs.size())))].Id;
		}
		P.ReawakenCount = Below(41);
		P.DeepestBiomeIndex = Below(static_cast<int64_t>(D.Tiers.size()));
		const int64_t Beaten = Below(static_cast<int64_t>(D.Code.BossRealms.size()) + 1);
		for (int64_t B = 0; B < Beaten; ++B)
		{
			P.BossesDefeated.insert(D.Code.BossRealms[static_cast<size_t>(B)]);
		}
		P.bFinalBossDefeated = U(R) < 0.15;
		P.StardustGrantedEver = U(R) < 0.9 ? std::pow(10.0, U(R) * 12.0) : 0.0;
		P.WavesHere = Below(201);
		P.CardsTakenThisRun = Below(61);
		P.KillsThisCycle = Below(10000001);
		return P;
	}

	bool AllFinite(const Pc::PcCombatStats& S)
	{
		const double V[] = { S.PawDamageMultiplier, S.AttackSpeedMultiplier, S.ShredPerStack, S.ExecuteThreshold,
			S.AuraDamageMultiplier, S.AuraRadiusBranchMultiplier, S.EvolutionDamageMultiplier, S.WeaponMultiplier,
			S.AdditiveDamageSum, S.MultiplicativeDamageProduct, S.CritChance, S.CritMultiplier, S.OverkillFraction,
			S.KittenPowerMultiplier, S.KittenAdditiveDamageSum, S.KittenMultiplicativeDamageProduct, S.PoopPowerMultiplier,
			S.PoopAdditiveDamageSum, S.PoopMultiplicativeDamageProduct, S.KittenSpeedMultiplier, S.ShieldPierceFraction,
			S.PouncePowerMultiplier, S.PounceCooldownReduction, S.ReawakenMultiplier };
		for (double X : V)
		{
			if (!std::isfinite(X))
			{
				return false;
			}
		}
		return true;
	}

	double OwnDps(const Pc::PcGameData& D, const Pc::PcCombatStats& S)
	{
		return Pc::PlayerDps(D.Combat, S);
	}
}

TEST_SUITE("long")
{
	TEST_CASE("Long: every random state gives finite, non-negative stats and DPS" * doctest::skip())
	{
		const Pc::PcGameData& D = Game();
		std::mt19937_64 R(0x5EED0001);
		const int64_t N = Cases(45000000);
		Timer T;
		int64_t Bad = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			const Pc::PcProgress P = RandomProgress(D, R);
			const Pc::PcCombatStats S = Pc::RebuildStats(D, P);
			const double Dps[] = { Pc::PawDps(D.Combat, S), Pc::AuraDps(D.Combat, S), Pc::KittenDps(D.Combat, S), Pc::PoopDps(D.Combat, S),
				Pc::CurrencyFindMultiplier(D, P) };
			bool bOk = AllFinite(S) && S.CritChance >= 0.0 && Pc::KittenCount(D, P) >= 0;
			for (double X : Dps)
			{
				bOk = bOk && std::isfinite(X) && X >= 0.0;
			}
			const int64_t Stage = Pc::EvolutionStage(D, P);
			bOk = bOk && Stage >= 1 && Stage <= D.Code.EvolutionStageCount;
			if (!bOk && Bad++ < 10)
			{
				FAIL_CHECK("state " << I << ": paw " << Dps[0] << " aura " << Dps[1] << " kitten " << Dps[2] << " poop " << Dps[3]
					<< " find " << Dps[4] << " stage " << Stage);
			}
		}
		CHECK(Bad == 0);
		MESSAGE(N << " states in " << T.Seconds() << " s");
	}

	TEST_CASE("Long: one more level of a damage line never lowers that damage" * doctest::skip())
	{
		// Paw lines raise the cat's own DPS; kitten lines raise a kitten's. One level is the shop's unit of purchase: a buy
		// that made the cat WEAKER would be a trap. Patience ties crit to attack speed, Whisker Trade trades crit for
		// damage; this is where such an interaction would show.
		//
		// FOUND 2026-10-28 (F3): with Patience owned and the cat slower than base (the Sphinx), Attack Speed lowered paw DPS
		// in Godot: Patience read the FINAL attack speed (1 + per x (1/speed - 1)), so d(DPS)/d(speed) went negative once
		// per x cc x cm > 1 + cc(cm - 1); it was counted here as an exemption (48,665 of 9,000,000 states, 2026-11-11).
		// FIXED 2026-11-12 (D16, the designer's call): Patience reads the job's speed only, so the exemption is gone and an
		// Attack Speed level that lowers DPS fails like any other line.
		const Pc::PcGameData& D = Game();
		struct Line
		{
			const char* Id;
			bool bKitten;
		};
		const Line Lines[] = { { "paw_damage", false }, { "attack_speed", false }, { "shred", false }, { "crit_chance", false },
			{ "kitten_power", true } };
		std::mt19937_64 R(0x5EED0002);
		const int64_t N = Cases(9000000);
		Timer T;
		int64_t Bad = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			Pc::PcProgress P = RandomProgress(D, R);
			for (const Line& L : Lines)
			{
				const Pc::PcCombatStats Before = Pc::RebuildStats(D, P);
				const double A = L.bKitten ? Pc::KittenDps(D.Combat, Before) : OwnDps(D, Before);
				const int64_t Level = Pc::UpgradeLevel(P, L.Id);
				P.UpgradeLevels[L.Id] = Level + 1;
				const Pc::PcCombatStats After = Pc::RebuildStats(D, P);
				const double B = L.bKitten ? Pc::KittenDps(D.Combat, After) : OwnDps(D, After);
				P.UpgradeLevels[L.Id] = Level;
				if (!(B < A * (1.0 - 1e-12)))
				{
					continue;
				}
				if (Bad++ < 10)
				{
					FAIL_CHECK("state " << I << ": " << std::string(L.Id) << " " << Level << " -> " << (Level + 1) << " lowers DPS " << A << " -> " << B);
				}
			}
		}
		CHECK(Bad == 0);
		MESSAGE(N << " states x 5 lines in " << T.Seconds() << " s; " << Bad << " lowered DPS");
	}

	TEST_CASE("Long: one more level of a damage branch never lowers that damage" * doctest::skip())
	{
		const Pc::PcGameData& D = Game();
		std::vector<const Pc::PcSkillBranch*> Damage;
		for (const Pc::PcSkillBranch& B : D.Branches)
		{
			switch (B.EffectType)
			{
			case Pc::BranchEffectType::DAMAGE_ADDITIVE:
			case Pc::BranchEffectType::DAMAGE_MULTIPLICATIVE:
			case Pc::BranchEffectType::KITTEN_DAMAGE_ADDITIVE:
			case Pc::BranchEffectType::AURA_DAMAGE_MULTIPLICATIVE:
				Damage.push_back(&B);
				break;
			default:
				break;
			}
		}
		REQUIRE(Damage.size() >= 3); // claws, instinct, aura_damage at least: else this test tests nothing
		std::mt19937_64 R(0x5EED0003);
		const int64_t N = Cases(7500000);
		Timer T;
		int64_t Bad = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			Pc::PcProgress P = RandomProgress(D, R);
			for (const Pc::PcSkillBranch* B : Damage)
			{
				const int64_t Level = Pc::BranchLevel(P, B->Id);
				if (Level >= static_cast<int64_t>(B->LevelCosts.size()))
				{
					continue;
				}
				const bool bKitten = B->EffectType == Pc::BranchEffectType::KITTEN_DAMAGE_ADDITIVE;
				const Pc::PcCombatStats Before = Pc::RebuildStats(D, P);
				const double A = bKitten ? Pc::KittenDps(D.Combat, Before) : OwnDps(D, Before);
				P.BranchLevels[B->Id] = Level + 1;
				const Pc::PcCombatStats After = Pc::RebuildStats(D, P);
				const double Bv = bKitten ? Pc::KittenDps(D.Combat, After) : OwnDps(D, After);
				P.BranchLevels[B->Id] = Level;
				if (Level == 0)
				{
					P.BranchLevels.erase(B->Id);
				}
				if (Bv < A * (1.0 - 1e-12) && Bad++ < 10)
				{
					FAIL_CHECK("state " << I << ": branch " << B->Id << " " << Level << " -> " << (Level + 1) << " lowers DPS " << A << " -> " << Bv);
				}
			}
		}
		CHECK(Bad == 0);
		MESSAGE(N << " states x " << Damage.size() << " branches in " << T.Seconds() << " s");
	}

	TEST_CASE("Long: more bosses beaten or more Stardust ever never lowers the cat's damage" * doctest::skip())
	{
		const Pc::PcGameData& D = Game();
		std::mt19937_64 R(0x5EED0004);
		std::uniform_real_distribution<double> U(0.0, 1.0);
		const int64_t N = Cases(12000000);
		Timer T;
		int64_t Bad = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			Pc::PcProgress P = RandomProgress(D, R);
			const double A = OwnDps(D, Pc::RebuildStats(D, P));
			Pc::PcProgress Q = P;
			const size_t Next = Q.BossesDefeated.size();
			if (Next < D.Code.BossRealms.size())
			{
				Q.BossesDefeated.insert(D.Code.BossRealms[Next]);
			}
			else
			{
				Q.bFinalBossDefeated = true;
			}
			Q.StardustGrantedEver = P.StardustGrantedEver * (1.0 + U(R)) + U(R);
			const double B = OwnDps(D, Pc::RebuildStats(D, Q));
			if (B < A * (1.0 - 1e-12) && Bad++ < 10)
			{
				FAIL_CHECK("state " << I << ": progress lowers DPS " << A << " -> " << B);
			}
		}
		CHECK(Bad == 0);
		MESSAGE(N << " states in " << T.Seconds() << " s");
	}

	TEST_CASE("Long: the Reawaken multiplier rises with Stardust ever granted and stays finite to 1e300" * doctest::skip())
	{
		const Pc::PcGameData& D = Game();
		const int64_t N = Cases(500000000);
		Timer T;
		double Prev = Pc::ReawakenMultiplier(D.Reawaken, 0.0);
		CHECK(Prev == 1.0);
		int64_t Bad = 0;
		for (int64_t I = 1; I <= N; ++I)
		{
			const double Stardust = std::pow(10.0, 300.0 * static_cast<double>(I) / static_cast<double>(N));
			const double M = Pc::ReawakenMultiplier(D.Reawaken, Stardust);
			if ((!std::isfinite(M) || M < Prev) && Bad++ < 10)
			{
				FAIL_CHECK("stardust " << Stardust << ": multiplier " << M << " after " << Prev);
			}
			Prev = M;
		}
		CHECK(Bad == 0);
		MESSAGE(N << " values in " << T.Seconds() << " s, multiplier at 1e300 Stardust " << Prev);
	}

	TEST_CASE("Long: prices rise every level; uncapped multiplicative lines get dearer faster than they pay" * doctest::skip())
	{
		// CLAUDE.md hard rule: uncapped multiplicative lines have cost_growth > effect_per_level, so cost / effect rises
		// forever and no level becomes a bargain. Checked numerically, in logs, to level 100,000 (far past overflow).
		const Pc::PcGameData& D = Game();
		const int64_t Levels = Cases(2000000) / 20;
		Timer T;
		int64_t Bad = 0;
		for (const Pc::PcUpgrade& Up : D.Upgrades)
		{
			double PrevCost = -1.0;
			double PrevRatio = -INFINITY;
			const bool bUncappedMult = Up.Cap < 0.0 && Up.EffectType == Pc::UpgradeEffectType::MULTIPLICATIVE;
			for (int64_t L = 0; L <= Levels; ++L)
			{
				const double Cost = Pc::UpgradeCost(Up, L);
				if (std::isnan(Cost) || Cost < 0.0 || (std::isfinite(PrevCost) && PrevCost >= 0.0 && std::isfinite(Cost) && Cost <= PrevCost))
				{
					if (Bad++ < 10)
					{
						FAIL_CHECK(Up.Id << " level " << L << ": price " << Cost << " after " << PrevCost);
					}
				}
				PrevCost = Cost;
				if (bUncappedMult)
				{
					const double LogRatio = std::log(Up.BaseCost) + static_cast<double>(L) * (std::log(Up.CostGrowth) - std::log(Up.EffectPerLevel));
					if (!(LogRatio > PrevRatio) && Bad++ < 10)
					{
						FAIL_CHECK(Up.Id << " level " << L << ": cost/effect stops rising");
					}
					PrevRatio = LogRatio;
				}
			}
		}
		for (const Pc::PcSkillBranch& B : D.Branches)
		{
			for (size_t L = 0; L < B.LevelCosts.size(); ++L)
			{
				const double Cost = Pc::BranchCost(B, static_cast<int64_t>(L));
				if (!(std::isfinite(Cost) && Cost > 0.0) && Bad++ < 10)
				{
					FAIL_CHECK("branch " << B.Id << " level " << L << ": price " << Cost);
				}
			}
			CHECK(Pc::BranchCost(B, static_cast<int64_t>(B.LevelCosts.size())) == -1.0);
		}
		CHECK(Bad == 0);
		MESSAGE(D.Upgrades.size() << " lines x " << Levels << " levels in " << T.Seconds() << " s");
	}

	TEST_CASE("Long: number formatting never breaks on any finite double" * doctest::skip())
	{
		// Random BIT PATTERNS, so every exponent from 1e-308 to 1e308 and both signs get hit, not just pretty numbers.
		std::mt19937_64 R(0x5EED0005);
		Pc::PcAmountStyle Cjk;
		Cjk.Group = 10000.0;
		Cjk.DecimalSeparator = ",";
		Cjk.Suffixes = { "W", "Y", "Z", "J", "G", "X" };
		const int64_t N = Cases(60000000);
		Timer T;
		int64_t Bad = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			uint64_t Bits = R();
			double V;
			static_assert(sizeof(V) == sizeof(Bits), "double is 64 bits");
			std::memcpy(&V, &Bits, sizeof(V));
			if (!std::isfinite(V))
			{
				continue;
			}
			const std::string A = Pc::ShortAmount(V);
			const std::string B = Pc::ShortAmount(V, Cjk);
			const bool bOk = !A.empty() && !B.empty() && A.find("nan") == std::string::npos && A.find("inf") == std::string::npos
				&& A == Pc::ShortAmount(V) && (V >= 0.0 || A[0] == '-' || A == "0");
			if (!bOk && Bad++ < 10)
			{
				FAIL_CHECK("value " << V << " -> '" << A << "' / '" << B << "'");
			}
		}
		CHECK(Bad == 0);
		MESSAGE(N << " bit patterns in " << T.Seconds() << " s");
	}

	TEST_CASE("Long: the hash RNG stays uniform over twenty billion draws" * doctest::skip())
	{
		// The fixed-step simulation draws every crit, spawn and card from RngHash(seed, tick, entity, salt). A bias at
		// the 1e-4 level is invisible in a unit test and very visible as a crit rate a player measures over a long run.
		constexpr int Buckets = 1024;
		std::vector<int64_t> Count(Buckets, 0);
		const int64_t N = Cases(20000000000);
		Timer T;
		double Sum = 0.0;
		for (int64_t I = 0; I < N; ++I)
		{
			const uint64_t Tick = static_cast<uint64_t>(I) >> 8;
			const uint64_t Entity = static_cast<uint64_t>(I) & 255u;
			++Count[static_cast<size_t>(Pc::RngBelow(Buckets, 2026, Tick, Entity, Pc::Salt::Crit))];
			if ((I & 15) == 0)
			{
				Sum += Pc::RngUnit(7, Tick, Entity, Pc::Salt::Spawn);
			}
		}
		const double Expected = static_cast<double>(N) / Buckets;
		double Chi2 = 0.0;
		for (int64_t C : Count)
		{
			Chi2 += (static_cast<double>(C) - Expected) * (static_cast<double>(C) - Expected) / Expected;
		}
		// 1023 degrees of freedom: mean 1023, sd ~45. 1023 + 6 sd = 1293 fails a fair generator about once in a billion.
		CHECK(Chi2 < 1293.0);
		const double Mean = Sum / static_cast<double>((N + 15) / 16);
		CHECK(std::fabs(Mean - 0.5) < 6.0 * std::sqrt(1.0 / 12.0 / static_cast<double>((N + 15) / 16)));
		MESSAGE(N << " draws in " << T.Seconds() << " s, chi2 " << Chi2 << " (1023 dof), mean unit " << Mean);
	}

	TEST_CASE("Long: the Stardust grant is split-invariant however the Catnip is split" * doctest::skip())
	{
		// GameState.get_pending_stardust(): computed against LIFETIME Catnip less what was paid, so cashing out in many
		// small cycles earns exactly what one big cycle does (sqrt is concave: on a resettable bank, 200 small cycles paid
		// ~7,000 Stardust for one cycle's 500).
		const Pc::PcGameData& D = Game();
		std::mt19937_64 R(0x5EED0006);
		std::uniform_real_distribution<double> U(0.0, 1.0);
		const int64_t N = Cases(3000000);
		Timer T;
		int64_t Bad = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			const double Total = std::floor(std::pow(10.0, U(R) * 15.0));
			Pc::PcProgress Split;
			const int Cuts = 1 + static_cast<int>(R() % 20);
			for (int C = 1; C <= Cuts; ++C)
			{
				Split.LifetimeCatnipEver = C == Cuts ? Total : std::floor(Total * (static_cast<double>(C) / Cuts) * U(R));
				Split.LifetimeCatnipEver = std::max(Split.LifetimeCatnipEver, 0.0);
				Split.StardustGrantedEver += Pc::PendingStardust(D, Split);
			}
			Pc::PcProgress Once;
			Once.LifetimeCatnipEver = Total;
			if (Split.StardustGrantedEver != Pc::PendingStardust(D, Once) && Bad++ < 10)
			{
				FAIL_CHECK("lifetime " << Total << " in " << Cuts << " cuts: " << Split.StardustGrantedEver << " vs " << Pc::PendingStardust(D, Once));
			}
		}
		CHECK(Bad == 0);
		MESSAGE(N << " splits in " << T.Seconds() << " s");
	}

	TEST_CASE("Long: the Reawaken gate in Catnip opens within one Catnip of the gate in Stardust" * doctest::skip())
	{
		// get_reawaken_requirement_catnip() is the Stardust condition solved for lifetime Catnip; Godot's check_hud asserted
		// the two forms flip within one Catnip of each other (an identity true only on paper is two disagreeing owners).
		const Pc::PcGameData& D = Game();
		std::mt19937_64 R(0x5EED0007);
		std::uniform_real_distribution<double> U(0.0, 1.0);
		const int64_t N = Cases(3000000);
		Timer T;
		int64_t Bad = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			Pc::PcProgress P;
			P.StardustGrantedEver = U(R) < 0.1 ? 0.0 : std::floor(std::pow(10.0, U(R) * 7.0));
			const double Needed = Pc::ReawakenRequirementCatnip(D, P);
			P.LifetimeCatnipEver = Needed;
			const bool bOpenAt = Pc::IsReawakenSuggested(D, P);
			P.LifetimeCatnipEver = std::max(Needed - std::max(1.0, Needed * 1e-15), 0.0);
			const bool bShutBelow = !Pc::IsReawakenSuggested(D, P);
			if (!(bOpenAt && bShutBelow) && Bad++ < 10)
			{
				FAIL_CHECK("granted " << P.StardustGrantedEver << ": open at " << Needed << " " << bOpenAt << ", shut just below " << bShutBelow);
			}
		}
		CHECK(Bad == 0);
		MESSAGE(N << " grants in " << T.Seconds() << " s");
	}

	TEST_CASE("Long: the wave banner and the wave bar agree on every kill count" * doctest::skip())
	{
		// current_wave() inverts the geometric kill series; wave_progress() evaluates it forward. One series read two ways:
		// with the index the banner shows, the bar is never negative, never past full, and full only at the ceiling.
		const Pc::PcGameData& D = Game();
		std::mt19937_64 R(0x5EED0008);
		const int64_t N = Cases(5000000);
		Timer T;
		int64_t Bad = 0;
		int64_t Checked = 0;
		for (int64_t I = 0; I < N; ++I)
		{
			const Pc::PcTier& Tier = D.Tiers[static_cast<size_t>(R() % D.Tiers.size())];
			if (Tier.KillsPerWave <= 0.0)
			{
				continue;
			}
			Pc::PcProgress P;
			P.CurrentBiomeId = Tier.Id;
			P.TierKillCount = static_cast<int64_t>(R() % 3000000);
			P.CurrentWaveIndex = Pc::CurrentWave(D, Tier, P.TierKillCount);
			const Pc::PcWaveProgress W = Pc::WaveProgress(D, P);
			const bool bFull = W.Done >= W.Costs;
			// Float series: the banner may tick over a hair before the bar fills, so "full" is allowed within 1e-9 relative.
			const bool bOk = W.Costs > 0.0 && W.Done >= 0.0 && W.Done <= W.Costs
				&& (!bFull || P.CurrentWaveIndex == D.Code.MaxWave || W.Costs - W.Done <= W.Costs * 1e-9);
			++Checked;
			if (!bOk && Bad++ < 10)
			{
				FAIL_CHECK(Tier.Id << " kills " << P.TierKillCount << ": wave " << P.CurrentWaveIndex << " bar " << W.Done << " / " << W.Costs);
			}
		}
		CHECK(Bad == 0);
		CHECK(Checked > 0);
		MESSAGE(Checked << " kill counts in " << T.Seconds() << " s");
	}
}

#endif
