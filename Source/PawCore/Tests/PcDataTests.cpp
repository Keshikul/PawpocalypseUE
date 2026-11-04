#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"

#include <cstdlib>
#include <string>

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	const Pc::PcGameData& Loaded()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::vector<std::string> Errors;
			const bool bOk = Pc::LoadGameData(DataDir(), Data, Errors);
			for (const std::string& E : Errors)
			{
				MESSAGE(E);
			}
			REQUIRE(bOk);
			bLoaded = true;
		}
		return Data;
	}
}

TEST_CASE("CSV parser: quotes, doubled quotes, commas and newlines inside quotes, CRLF, BOM")
{
	Pc::CsvTable T;
	std::string Error;
	const std::string Text = "\xEF\xBB\xBFid,text,n\r\na,\"x, \"\"y\"\"\nz\",1\r\nb,,2\n";
	REQUIRE(Pc::ParseCsv(Text, T, Error));
	CHECK(T.Header == std::vector<std::string>{ "id", "text", "n" });
	REQUIRE(T.Rows.size() == 2);
	CHECK(T.Rows[0][1] == "x, \"y\"\nz");
	CHECK(T.Rows[1][1].empty());
	CHECK(T.Column("n") == 2);
	CHECK(T.Column("missing") == -1);
}

TEST_CASE("CSV parser rejects malformed input")
{
	Pc::CsvTable T;
	std::string Error;
	CHECK_FALSE(Pc::ParseCsv("id,a\nx,\"open\n", T, Error));
	CHECK_FALSE(Pc::ParseCsv("id,a\nx,1,2\n", T, Error));
	CHECK(Error.find("cells") != std::string::npos);
}

TEST_CASE("Number parsing is strict and exact")
{
	double D = 0.0;
	CHECK(Pc::ParseDouble("2.07098e+12", D));
	CHECK(D == 2.07098e+12);
	CHECK(Pc::ParseDouble("0.0433", D));
	CHECK(D == 0.0433);
	CHECK_FALSE(Pc::ParseDouble("1.5x", D));
	CHECK_FALSE(Pc::ParseDouble("", D));
	CHECK_FALSE(Pc::ParseDouble("inf", D));
	CHECK_FALSE(Pc::ParseDouble("nan", D));
	int64_t I = 0;
	CHECK(Pc::ParseInt("300", I));
	CHECK(I == 300);
	CHECK_FALSE(Pc::ParseInt("3.0", I));
}

TEST_CASE("Row readers: empty cell keeps the default, bad cells are reported with class, row, id and column")
{
	Pc::CsvTable T;
	std::string Error;
	REQUIRE(Pc::ParseCsv("id,hp,movement,tint,is_signature\nmouse,,FLIER,0.5;0.5;0.5,maybe\n", T, Error));
	Pc::PcEnemy E;
	std::vector<std::string> Errors;
	Pc::ReadRow(T, 0, E, Errors);
	CHECK(E.Id == "mouse");
	CHECK(E.Hp == 10.0); // the Godot default
	CHECK(E.Movement == Pc::EnemyMovementKind::FLIER);
	CHECK(E.Tint[0] == 0.5);
	CHECK(E.Tint[3] == 1.0);
	REQUIRE(Errors.size() == 1);
	CHECK(Errors[0] == "EnemyData row 1 (mouse) is_signature: not true/false 'maybe'");

	REQUIRE(Pc::ParseCsv("id,movement\nmouse,FLYER\n", T, Error));
	Errors.clear();
	Pc::ReadRow(T, 0, E, Errors);
	REQUIRE(Errors.size() == 1);
	CHECK(Errors[0].find("unknown enum name 'FLYER'") != std::string::npos);
}

TEST_CASE("Ladder map (SkillBranchData.extra_effects JSON)")
{
	Pc::CsvTable T;
	std::string Error;
	REQUIRE(Pc::ParseCsv("id,extra_effects\nclaws,\"{\"\"mult\"\":[1.06, 1.12],\"\"x\"\":[]}\"\n", T, Error));
	Pc::PcSkillBranch B;
	std::vector<std::string> Errors;
	Pc::ReadRow(T, 0, B, Errors);
	CHECK(Errors.empty());
	REQUIRE(B.ExtraEffects.count("mult") == 1);
	CHECK(B.ExtraEffects.at("mult") == std::vector<double>{ 1.06, 1.12 });
	CHECK(B.ExtraEffects.at("x").empty());
}

TEST_CASE("The repo's Data/*.csv loads completely")
{
	const Pc::PcGameData& D = Loaded();
	CHECK(D.Tiers.size() == 12);
	CHECK(D.Enemies.size() == 59);
	CHECK(D.Bosses.size() == 5);
	CHECK(D.Upgrades.size() == 15);
	CHECK(D.Branches.size() == 87);
	CHECK(D.Evolutions.size() == 7);
	CHECK(D.Cards.size() == 28);
	CHECK(D.Jobs.size() == 10);
	CHECK(D.Achievements.size() == 39); // F2, 2026-11-03: two removed
	// Biome order is numeric, not the CSV's text order (tier_1, tier_10, tier_11, ...).
	CHECK(D.Tiers.front().Id == "tier_1");
	CHECK(D.Tiers[1].Id == "tier_2");
	CHECK(D.Tiers.back().Id == "tier_12");
	CHECK(D.Evolutions.front().Stage == 1);
	CHECK(D.Evolutions.back().Stage == 7);
}

TEST_CASE("Spot values match the CSV text")
{
	const Pc::PcGameData& D = Loaded();
	const Pc::PcTier& T1 = D.Tiers.front();
	CHECK(T1.DisplayName == "Backyard");
	CHECK(T1.SpawnRatePerMin == 120.0);
	CHECK(T1.KillsPerWave == 300);
	CHECK(T1.SpawnPool == std::vector<std::string>{ "backyard_mouse", "backyard_moth", "alley_mouse", "backyard_gristle" });
	CHECK(T1.SpawnWeights == std::vector<double>{ 92.0, 5.0, 3.0, 2.0 });
	CHECK(T1.GoldenEnemy == "backyard_golden_mouse");
	const Pc::PcEnemy* Dog = D.FindEnemy("alley_dog");
	REQUIRE(Dog);
	CHECK(Dog->Hp == 143.364);
	CHECK(Dog->bIsSignature);
	CHECK(Dog->Movement == Pc::EnemyMovementKind::GROUND);
	const Pc::PcUpgrade* Speed = D.FindUpgrade("attack_speed");
	REQUIRE(Speed);
	CHECK(Speed->EffectType == Pc::UpgradeEffectType::MULTIPLICATIVE);
	CHECK(Speed->CostGrowth == 2.4364);
	const Pc::PcSkillBranch* Anvil = D.FindBranch("anvil_paw");
	REQUIRE(Anvil);
	CHECK(Anvil->EffectType == Pc::BranchEffectType::DOUBLE_STRIKE_SHARE);
	CHECK(Anvil->LevelCosts.size() == 10);
	CHECK(Anvil->Parent == "job_blacksmith");
	const Pc::PcBoss* Foreman = D.FindBoss("factory_foreman");
	REQUIRE(Foreman);
	CHECK(Foreman->TotalHp == 2.07098e+12);
	CHECK(Foreman->PhaseHpThresholds == std::vector<double>{ 0.75, 0.50, 0.25 });
	CHECK(D.Combat.BasePawDamage == 4.0);
	CHECK(D.Combat.ShredMaxStacks == 10);
	CHECK(D.Reawaken.ReawakenStardustCoefficient == 0.125);
	// No column in the CSV: every row keeps the script default.
	CHECK(D.Reawaken.ReawakenMultPerPoint == 1.01);
	CHECK(D.FindEnemy("no_such_enemy") == nullptr);
}

TEST_CASE("Code constants: the shipped values, and every malformed CodeConstants.csv refused")
{
	const Pc::PcGameData& D = Loaded();
	CHECK(D.Code.BossRealms == std::vector<std::string>{ "tier_3", "tier_6", "tier_8", "tier_11", "tier_12" });
	CHECK(D.Code.EvolutionStageCount == 7);
	CHECK(D.Code.CardLevelCostBase == 24.0);
	CHECK(D.Code.CardLevelCostGrowth == 1.68);
	CHECK(D.Code.CurrencyEpsilon == 1e-6);
	CHECK(D.Code.MinSpawnInterval == 1.0 / 60.0); // written from Godot's `1.0 / 60.0` in round-trip form
	CHECK(D.Code.SecondHelpingBiomeIndex == 3);
	CHECK(D.Code.PlayerStillEpsilonPx == 1.5);

	// The shipped file, re-read, with one change at a time: each must be refused with a message naming the id.
	std::string Text;
	REQUIRE(Pc::ReadTextFile(DataDir() + "/CodeConstants.csv", Text));
	const auto Read = [](const std::string& Csv, std::string& Message)
	{
		Pc::CsvTable T;
		std::string Error;
		REQUIRE(Pc::ParseCsv(Csv, T, Error));
		Pc::PcCodeConstants Out;
		std::vector<std::string> Errors;
		const bool bOk = Pc::ReadCodeConstants(T, Out, Errors);
		Message.clear();
		for (const std::string& E : Errors)
		{
			Message += E + "\n";
		}
		return bOk;
	};
	std::string Message;
	REQUIRE_MESSAGE(Read(Text, Message), Message);
	const auto Replace = [&Text](const std::string& From, const std::string& To)
	{
		const size_t At = Text.find(From);
		REQUIRE(At != std::string::npos);
		REQUIRE(Text.find(From, At + 1) == std::string::npos);
		return Text.substr(0, At) + To + Text.substr(At + From.size());
	};
	struct Case { const char* Name; std::string Csv; const char* Mentions; };
	const Case Cases[] = {
		{ "a row missing", Replace("\nlevy_every,", "\nnot_levy_every,"), "no row for 'levy_every'" },
		{ "an unknown id", Replace("\nlevy_every,", "\nnot_levy_every,"), "unknown id 'not_levy_every'" },
		{ "a row twice", Text + "max_wave,64,,x\r\n", "'max_wave' appears twice" },
		{ "an integer with a fraction", Replace("\nmax_wave,64,", "\nmax_wave,64.0,"), "max_wave" },
		{ "a number that is text", Replace("\nmax_golden_chance,0.35,", "\nmax_golden_chance,lots,"), "max_golden_chance" },
		{ "an empty list item", Replace("tier_3;tier_6;", "tier_3;;"), "boss_realms" },
	};
	for (const Case& C : Cases)
	{
		CAPTURE(C.Name);
		CHECK_FALSE(Read(C.Csv, Message));
		CHECK_MESSAGE(Message.find(C.Mentions) != std::string::npos, Message);
	}
}

TEST_CASE("Every id is lower case (docs/03 §6: never FName identity, never case-sensitive surprises)")
{
	const Pc::PcGameData& D = Loaded();
	auto Lower = [](const std::string& S) { for (char C : S) { if (C >= 'A' && C <= 'Z') { return false; } } return !S.empty(); };
	for (const auto& R : D.Tiers) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
	for (const auto& R : D.Enemies) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
	for (const auto& R : D.Bosses) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
	for (const auto& R : D.Upgrades) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
	for (const auto& R : D.Branches) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
	for (const auto& R : D.Cards) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
	for (const auto& R : D.Jobs) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
	for (const auto& R : D.Achievements) { CHECK_MESSAGE(Lower(R.Id), R.Id); }
}

#endif
