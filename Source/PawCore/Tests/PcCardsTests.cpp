#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCards.h"
#include "PcCsv.h"
#include "PcData.h"

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

	std::vector<std::string> SplitKeep(const std::string& S, char Sep)
	{
		std::vector<std::string> Out;
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find(Sep, Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	std::map<std::string, int64_t> Pairs(const std::string& S)
	{
		std::map<std::string, int64_t> Out;
		for (const std::string& P : SplitKeep(S, ';'))
		{
			const size_t Eq = P.find('=');
			if (Eq != std::string::npos)
			{
				Out[P.substr(0, Eq)] = std::atoll(P.c_str() + Eq + 1);
			}
		}
		return Out;
	}

	double Hex(const std::string& S)
	{
		return std::strtod(S.c_str(), nullptr);
	}

	// card_data.gd Effect, in declaration order (= CardData.Effect.values() in Godot).
	const Pc::CardEffect AllEffects[] = { Pc::CardEffect::AURA_TRAIL, Pc::CardEffect::AURA_PULSE, Pc::CardEffect::AURA_SLOW,
		Pc::CardEffect::KITTEN_KILL_EXPLODES, Pc::CardEffect::KITTEN_DRAG, Pc::CardEffect::GOLDEN_KITTEN,
		Pc::CardEffect::KITTEN_FROM_SIGNATURE, Pc::CardEffect::KITTEN_MIMIC_POUNCE, Pc::CardEffect::LONE_HUNTER,
		Pc::CardEffect::POUNCE_CHAIN, Pc::CardEffect::POUNCE_CRATER, Pc::CardEffect::POUNCE_KILL_REFUND,
		Pc::CardEffect::GOLDEN_FREQUENCY, Pc::CardEffect::SIGNATURE_FREQUENCY, Pc::CardEffect::CATNIP_BOMB_EVERY,
		Pc::CardEffect::EDGE_BURN, Pc::CardEffect::OVERKILL_TARGETS, Pc::CardEffect::POOP_DURATION, Pc::CardEffect::POOP_TRAIL,
		Pc::CardEffect::SIGNATURE_BOUNTY, Pc::CardEffect::CATNIP_INTEREST, Pc::CardEffect::SCAVENGE, Pc::CardEffect::EXTRA_CHOICE,
		Pc::CardEffect::REROLL, Pc::CardEffect::BANISH, Pc::CardEffect::FIXATION_HANDS };

	struct CardsResult
	{
		int Rows = 0;
		int Levelled = 0;
		int SecondOpinionGrants = 0;
	};

	CardsResult RunCardsFixture(const std::string& Path)
	{
		CardsResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		const Pc::PcGameData& D = Game();
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = SplitKeep(Line, '\t');
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
			Pc::PcProgress P;
			P.UpgradeLevels = Pairs(V.at("upgrades"));
			P.BranchLevels = Pairs(V.at("branches"));
			P.CardLevels = Pairs(V.at("cards"));
			for (const std::string& B : SplitKeep(V.at("banished"), ';'))
			{
				if (!B.empty())
				{
					P.BanishedCards.insert(B);
				}
			}
			P.RunJob = V.at("run_job");
			P.RunJobSecond = V.at("run_job_second");
			P.ReawakenCount = std::atoll(V.at("reawaken_count").c_str());
			P.DeepestBiomeIndex = std::atoll(V.at("deepest").c_str());
			P.ExtraChoices = std::atoll(V.at("extra_choices").c_str());
			P.CatchesLevel = std::atoll(V.at("catches_level").c_str());
			P.CatchesPoints = Hex(V.at("catches_points"));
			CAPTURE(R.Rows);

			std::string Eligible;
			for (const Pc::PcCard* Card : Pc::EligibleCards(D, P))
			{
				Eligible += (Eligible.empty() ? "" : ";") + Card->Id;
			}
			CHECK_MESSAGE(Eligible == V.at("eligible"), "eligible: PawCore " << Eligible << " vs Godot " << V.at("eligible"));
			CHECK(std::to_string(Pc::HandSize(D, P)) == V.at("hand_size"));
			const std::vector<std::string> Values = SplitKeep(V.at("effect_values"), ';');
			const std::vector<std::string> Lows = SplitKeep(V.at("effect_best_low"), ';');
			REQUIRE(Values.size() == std::size(AllEffects));
			REQUIRE(Lows.size() == std::size(AllEffects));
			for (size_t E = 0; E < std::size(AllEffects); ++E)
			{
				CHECK_MESSAGE(Pc::CardEffectValue(D, P, AllEffects[E]) == Hex(Values[E]), "effect_value " << Pc::ToName(AllEffects[E]));
				CHECK_MESSAGE(Pc::CardEffectBestLow(D, P, AllEffects[E]) == Hex(Lows[E]), "effect_best_low " << Pc::ToName(AllEffects[E]));
			}
			const int64_t Probe = std::atoll(V.at("probe_level").c_str());
			// pow() differs by C runtime: relative 1e-13.
			CHECK(Pc::CatchesForLevel(D, Probe) == doctest::Approx(Hex(V.at("catches_for_probe"))).epsilon(1e-13));
			CHECK(Pc::TotalCatchesForLevel(D, Probe) == doctest::Approx(Hex(V.at("total_for_probe"))).epsilon(1e-13));

			const Pc::PcCatchesStep Step = Pc::AddKillCatches(D, P, Hex(V.at("amount")), V.at("hand_pending") == "1");
			CHECK(std::to_string(Step.Level) == V.at("step_level"));
			CHECK(Step.Points == Hex(V.at("step_points")));
			CHECK((Step.bLevelled ? "1" : "0") == V.at("step_levelled"));
			CHECK_MESSAGE(std::to_string(Step.RerollsGranted) == V.at("step_rerolls"), "rerolls: PawCore " << Step.RerollsGranted << " vs Godot " << V.at("step_rerolls"));
			R.Levelled += Step.bLevelled ? 1 : 0;
			R.SecondOpinionGrants += (Step.bLevelled && Step.RerollsGranted > 0) ? 1 : 0;
			CHECK_MESSAGE((Step.bLevelled || Step.RerollsGranted == 0), "Second Opinion granted rerolls on a kill that did not level");
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("PcCards matches Godot's CardManager on 1,000 random states")
{
	const CardsResult R = RunCardsFixture(DataDir() + "/../Source/PawCore/Tests/Data/cards_godot.tsv");
	CHECK(R.Rows == 1000);
	CHECK(R.Levelled > 50);            // both branches of the kill step are exercised
	CHECK(R.SecondOpinionGrants > 20); // the grant itself is exercised, on levelling kills only
	MESSAGE("levelled " << R.Levelled << ", Second Opinion grants on a level " << R.SecondOpinionGrants);
}

TEST_CASE("Long: PcCards matches all 3,000 Godot CardManager rows" * doctest::test_suite("long") * doctest::skip())
{
	const CardsResult R = RunCardsFixture(DataDir() + "/../build/cards_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Card rules from the Godot comments")
{
	const Pc::PcGameData& D = Game();
	// The first card costs 24 kills (raised from 8: the run had no opening, it had a card).
	CHECK(Pc::TotalCatchesForLevel(D, 1) == doctest::Approx(24.0));
	CHECK(Pc::TotalCatchesForLevel(D, 0) == 0.0);
	// A fresh run: no aura, no pounce, no kittens, so every card needing one is held back.
	Pc::PcProgress Fresh;
	for (const Pc::PcCard* Card : Pc::EligibleCards(D, Fresh))
	{
		CHECK_MESSAGE(std::string(Pc::CardEffectRequirement(Card->Effect)).empty(), Card->Id << " offered with nothing to act on");
	}
	// A banished card is never offered.
	Pc::PcProgress Banish;
	const std::vector<const Pc::PcCard*> Before = Pc::EligibleCards(D, Banish);
	REQUIRE_FALSE(Before.empty());
	Banish.BanishedCards.insert(Before[0]->Id);
	const std::vector<const Pc::PcCard*> After = Pc::EligibleCards(D, Banish);
	CHECK(After.size() + 1 == Before.size());
	CHECK(Pc::HandSize(D, Fresh) == 3);
}

#endif
