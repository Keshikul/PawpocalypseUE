#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCombat.h"
#include "PcCsv.h"
#include "PcData.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	const Pc::PcCombatConstants& Constants()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(DataDir(), Data, Errors));
			bLoaded = true;
		}
		return Data.Combat;
	}

	// Equal bit for bit, or both NaN / both the same infinity.
	bool Same(double A, double B)
	{
		return A == B || (std::isnan(A) && std::isnan(B));
	}

	// Units in the last place between two finite doubles of the same sign.
	double Ulps(double A, double B)
	{
		if (A == B)
		{
			return 0.0;
		}
		return std::fabs(A - B) / std::fabs(std::nextafter(A, INFINITY) - A);
	}

	struct FixtureResult
	{
		int Rows = 0;
		int PowUlpRows = 0;
	};

	// Runs every row of a Godot CombatResolver fixture through PcCombat. Returns counts; failures are doctest CHECKs.
	FixtureResult RunCombatFixture(const std::string& Path)
	{
		FixtureResult Result;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		const Pc::PcCombatConstants Shipped = Constants();
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			std::vector<std::string> Cells;
			std::string Cell;
			std::istringstream Row(Line);
			while (std::getline(Row, Cell, '\t'))
			{
				Cells.push_back(Cell);
			}
			if (Header.empty())
			{
				Header = Cells;
				continue;
			}
			REQUIRE(Cells.size() == Header.size());
			std::map<std::string, std::string> V;
			for (size_t I = 0; I < Cells.size(); ++I)
			{
				V[Header[I]] = Cells[I];
			}
			auto D = [&](const char* Name) { return std::strtod(V.at(Name).c_str(), nullptr); };

			Pc::PcCombatConstants C = Shipped;
			if (V.at("flat_bounty") == "1")
			{
				C.OverkillBountyExponent = 0.0;
			}
			Pc::PcCombatStats S;
			S.bAuraOwned = V.at("aura_owned") == "1";
			S.PawDamageMultiplier = D("paw_damage_multiplier");
			S.AttackSpeedMultiplier = D("attack_speed_multiplier");
			S.ShredPerStack = D("shred_per_stack");
			S.ExecuteThreshold = D("execute_threshold");
			S.AuraDamageMultiplier = D("aura_damage_multiplier");
			S.AuraRadiusBranchMultiplier = D("aura_radius_branch_multiplier");
			S.EvolutionDamageMultiplier = D("evolution_damage_multiplier");
			S.WeaponMultiplier = D("weapon_multiplier");
			S.AdditiveDamageSum = D("additive_damage_sum");
			S.MultiplicativeDamageProduct = D("multiplicative_damage_product");
			S.CritChance = D("crit_chance");
			S.CritMultiplier = D("crit_multiplier");
			S.OverkillFraction = D("overkill_fraction");
			S.KittenPowerMultiplier = D("kitten_power_multiplier");
			S.KittenAdditiveDamageSum = D("kitten_additive_damage_sum");
			S.KittenMultiplicativeDamageProduct = D("kitten_multiplicative_damage_product");
			S.PoopPowerMultiplier = D("poop_power_multiplier");
			S.PoopAdditiveDamageSum = D("poop_additive_damage_sum");
			S.PoopMultiplicativeDamageProduct = D("poop_multiplicative_damage_product");
			S.KittenSpeedMultiplier = D("kitten_speed_multiplier");
			S.ShieldPierceFraction = D("shield_pierce_fraction");
			S.PouncePowerMultiplier = D("pounce_power_multiplier");
			S.PounceCooldownReduction = D("pounce_cooldown_reduction");
			S.ReawakenMultiplier = D("reawaken_multiplier");
			const int64_t Stacks = std::atoll(V.at("stacks").c_str());
			const bool bVulnerable = V.at("vulnerable") == "1";
			const double Rate = D("rate");
			const double Excess = D("excess");
			const double MaxHp = D("max_hp");
			const double Reward = D("reward");

			CAPTURE(Result.Rows);
			CHECK(Same(Pc::DamageScale(S), D("damage_scale")));
			CHECK(Same(Pc::PawStrikeDamage(C, S, Stacks), D("strike")));
			CHECK(Same(Pc::StrikesPerSec(C, S), D("strikes_per_sec")));
			CHECK(Same(Pc::PawDps(C, S), D("paw_dps")));
			CHECK(Same(Pc::AuraDps(C, S), D("aura_dps")));
			CHECK(Same(Pc::PlayerDps(C, S), D("player_dps")));
			CHECK(Same(Pc::KittenDps(C, S), D("kitten_dps")));
			CHECK(Same(Pc::PounceDamage(C, S, bVulnerable, Rate), D("pounce")));
			CHECK(Same(Pc::PounceAuraMultiple(C, S, Rate), D("pounce_multiple")));
			CHECK(Same(Pc::PounceCapScale(C, S, Rate), D("pounce_cap_scale")));
			CHECK(Same(Pc::PounceCooldown(C, S), D("pounce_cooldown")));
			CHECK(Same(Pc::OverkillDamage(S, Excess), D("overkill_damage")));
			CHECK(Same(Pc::PoopDps(C, S), D("poop_dps")));
			// The one pow() in the resolver: Godot's C runtime (MinGW) and MSVC's may round it differently in the last bit.
			const double Bounty = Pc::OverkillBounty(C, S, Excess, MaxHp, Reward);
			const double Want = D("overkill_bounty");
			if (!Same(Bounty, Want))
			{
				++Result.PowUlpRows;
				CHECK_MESSAGE(Ulps(Bounty, Want) <= 4.0, "overkill bounty " << Bounty << " vs Godot " << Want);
			}
			++Result.Rows;
		}
		return Result;
	}
}

TEST_CASE("PcCombat matches Godot's CombatResolver bit for bit on 2,000 random stats snapshots")
{
	const FixtureResult R = RunCombatFixture(DataDir() + "/../Source/PawCore/Tests/Data/combat_godot.tsv");
	CHECK(R.Rows == 2000);
	MESSAGE("overkill bounty rows within a few ulps (pow differs by C runtime): " << R.PowUlpRows);
}

TEST_CASE("Long: PcCombat matches all 5,000 Godot CombatResolver rows" * doctest::test_suite("long") * doctest::skip())
{
	// build/combat_godot_full.tsv comes from `py tools/make_godot_fixtures.py` (the real Godot code; not in git).
	const FixtureResult R = RunCombatFixture(DataDir() + "/../build/combat_godot_full.tsv");
	CHECK(R.Rows == 5000);
	MESSAGE("overkill bounty rows within a few ulps (pow differs by C runtime): " << R.PowUlpRows);
}

TEST_CASE("Combat rules that hold whatever the numbers (Godot's comments, as checks)")
{
	const Pc::PcCombatConstants& C = Constants();
	Pc::PcCombatStats S;
	// The aura is worth exactly zero until bought, and the paw is not.
	CHECK(Pc::AuraDps(C, S) == 0.0);
	CHECK(Pc::PawDps(C, S) > 0.0);
	S.bAuraOwned = true;
	CHECK(Pc::AuraDps(C, S) > 0.0);
	// Pounce and poop are priced against the whole player, so the paw alone still gives them value.
	S.bAuraOwned = false;
	CHECK(Pc::PoopDps(C, S) > 0.0);
	CHECK(Pc::PounceDamage(C, S, false) > 0.0);
	// The vulnerable-window bonus is outside the cap: it still multiplies when the cap binds.
	S.PouncePowerMultiplier = 1000.0;
	REQUIRE(Pc::PounceCapScale(C, S) < 1.0);
	CHECK(Pc::PounceDamage(C, S, true) == doctest::Approx(Pc::PounceDamage(C, S, false) * C.PounceVulnerableMultiplier));
	// The sustained multiple never exceeds the cap once scaled.
	CHECK(Pc::PounceAuraMultiple(C, S) * Pc::PounceCapScale(C, S) == doctest::Approx(C.PounceMaxAuraMultiple));
	// A cooldown at zero: infinite multiple, burst scaled to nothing rather than NaN.
	S.PounceCooldownReduction = 1.0;
	CHECK(std::isinf(Pc::PounceAuraMultiple(C, S)));
	CHECK(Pc::PounceCapScale(C, S) == 0.0);
	// Poop does not apply the Reawaken multiplier twice.
	Pc::PcCombatStats R;
	R.ReawakenMultiplier = 3.0;
	CHECK(Pc::PoopDps(C, R) == doctest::Approx(Pc::PoopDps(C, Pc::PcCombatStats()) * 3.0));
	// Additive and multiplicative brackets stay separate: (1 + sum) x product, not (1 + sum + product - 1).
	Pc::PcCombatStats B;
	B.AdditiveDamageSum = 1.0;
	B.MultiplicativeDamageProduct = 2.0;
	CHECK(Pc::DamageScale(B) == 4.0);
	// Overkill bounty, curved form: continuous at a wasted multiple of exactly 1, linear below, sub-linear above.
	Pc::PcCombatConstants Curve = C;
	Curve.OverkillBountyExponent = 0.4;
	Pc::PcCombatStats O;
	O.OverkillFraction = 0.5;
	CHECK(Pc::OverkillBounty(Curve, O, 100.0, 100.0, 10.0) == doctest::Approx(5.0));
	CHECK(Pc::OverkillBounty(Curve, O, 50.0, 100.0, 10.0) == doctest::Approx(2.5));
	CHECK(Pc::OverkillBounty(Curve, O, 10000.0, 100.0, 10.0) == doctest::Approx(10.0 * 0.5 * std::pow(100.0, 0.4)));
	// Flat form: a ceiling.
	Pc::PcCombatConstants Flat = C;
	Flat.OverkillBountyExponent = 0.0;
	CHECK(Pc::OverkillBounty(Flat, O, 1e9, 100.0, 10.0) == doctest::Approx(10.0 * 0.5 * Flat.OverkillBountyMaxMultiple));
	// No bounty without overkill bought, without excess, or for a worthless victim.
	O.OverkillFraction = 0.0;
	CHECK(Pc::OverkillBounty(Curve, O, 1e9, 100.0, 10.0) == 0.0);
}

#endif
