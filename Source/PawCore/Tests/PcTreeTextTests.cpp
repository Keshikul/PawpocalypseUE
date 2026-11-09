#if defined(PAWCORE_STANDALONE_TESTS)

// PcTreeText against the REAL Godot wording (tools/godot_fixtures/tree_text_fixture.gd): every branch's _effect_text() at
// every level, rendered here through the generated ST_Tree.csv English exactly as the game renders it (the phrase, its one
// argument, ", " between parts), and the number formats on 2,000 values each. Then the tooltip's decisions.

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcFormat.h"
#include "PcTree.h"
#include "PcTreeText.h"

#include <cstdlib>
#include <fstream>
#include <map>
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

	const std::map<std::string, std::string>& TreeStrings()
	{
		static std::map<std::string, std::string> Map;
		if (Map.empty())
		{
			std::string Text, Error;
			REQUIRE(Pc::ReadTextFile(DataDir() + "/../Content/Pawpocalypse/Localization/Strings/ST_Tree.csv", Text));
			Pc::CsvTable T;
			REQUIRE(Pc::ParseCsv(Text, T, Error));
			const int K = T.Column("Key"), S = T.Column("SourceString");
			for (const auto& Row : T.Rows)
			{
				Map[Row[static_cast<size_t>(K)]] = Row[static_cast<size_t>(S)];
			}
		}
		return Map;
	}

	// The lines whose English the port rewrote (Data/TextOverrides.csv, designer 2026-11-09): key -> Godot's English, which
	// is what the Godot fixture shows. The generator proves the rewrite keeps the same {tokens}.
	const std::map<std::string, std::string>& GodotWords()
	{
		static std::map<std::string, std::string> Map;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::string Text, Error;
			REQUIRE(Pc::ReadTextFile(DataDir() + "/TextOverrides.csv", Text));
			Pc::CsvTable T;
			REQUIRE(Pc::ParseCsv(Text, T, Error));
			const int Table = T.Column("table"), K = T.Column("key"), G = T.Column("godot");
			for (const auto& Row : T.Rows)
			{
				if (Row[static_cast<size_t>(Table)] == "ST_Tree")
				{
					Map[Row[static_cast<size_t>(K)]] = Row[static_cast<size_t>(G)];
				}
			}
			bLoaded = true;
		}
		return Map;
	}

	// What Godot shows: each phrase (Godot's English) with its argument filled, ", " between them.
	std::string Render(const std::vector<Pc::PcTreePhrase>& Parts)
	{
		std::string Out;
		for (const Pc::PcTreePhrase& P : Parts)
		{
			const auto It = TreeStrings().find(P.Key);
			REQUIRE_MESSAGE(It != TreeStrings().end(), "ST_Tree has no " << P.Key);
			const auto Godot = GodotWords().find(P.Key);
			std::string Text = Godot != GodotWords().end() ? Godot->second : It->second;
			if (!P.Arg.empty())
			{
				const std::string Token = "{" + P.Arg + "}";
				const size_t At = Text.find(Token);
				REQUIRE_MESSAGE(At != std::string::npos, P.Key << " has no " << Token);
				Text.replace(At, Token.size(), P.Number);
			}
			Out += (Out.empty() ? "" : ", ") + Text;
		}
		return Out;
	}

	std::vector<std::string> Split(const std::string& S, char Sep)
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

	std::string Format(const std::string& Code, double V)
	{
		if (Code == "R100") return Pc::RoundiLikeGodot(V * 100.0);
		if (Code == "Int") return Pc::IntLikeGodot(V);
		if (Code == "F0") return Pc::FixedLikeGodot(V, 0);
		if (Code == "F1") return Pc::FixedLikeGodot(V, 1);
		if (Code == "F2") return Pc::FixedLikeGodot(V, 2);
		if (Code == "F3") return Pc::FixedLikeGodot(V, 3);
		if (Code == "F1x100") return Pc::FixedLikeGodot(V * 100.0, 1);
		if (Code == "F0PerMinute") return Pc::FixedLikeGodot(60.0 / std::max(V, 0.001), 0);
		if (Code == "RInverse") return Pc::RoundiLikeGodot(1.0 / std::max(V, 0.001));
		FAIL("unknown format " << Code);
		return std::string();
	}
}

TEST_CASE("PcTreeText matches Godot's _effect_text on every branch at every level, and its number formats")
{
	const Pc::PcGameData& D = Game();
	std::ifstream In(DataDir() + "/../Source/PawCore/Tests/Data/tree_text_godot.tsv");
	REQUIRE(In.good());
	std::string Line;
	bool bHeader = false;
	int Fx = 0, Worded = 0, Nums = 0, Channels = 0;
	while (std::getline(In, Line))
	{
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		if (!bHeader)
		{
			bHeader = true;
			continue;
		}
		const std::vector<std::string> C = Split(Line, '\t');
		REQUIRE(C.size() == 5);
		if (C[0] == "fx")
		{
			const std::vector<Pc::PcTreePhrase> Parts = Pc::TreeEffectPhrases(D, C[1], std::atoll(C[2].c_str()));
			INFO(C[1] << " level " << C[2]);
			CHECK(Render(Parts) == C[4]);
			++Fx;
			Worded += C[4].empty() ? 0 : 1;
			Channels += Parts.size() > 1 ? 1 : 0;
		}
		else
		{
			const double V = std::strtod(C[3].c_str(), nullptr);
			INFO(C[1] << " of " << C[3]);
			CHECK(Format(C[1], V) == C[4]);
			++Nums;
		}
	}
	CHECK(Fx > 600);        // measured 640: every branch, levels 0..max+1
	CHECK(Worded > 400);    // measured 456 (levels 0 and max+1 say nothing)
	CHECK(Channels > 20);   // the merged ladders' channels are compared, not only the primary
	CHECK(Nums == 18000);
	MESSAGE(Fx << " branch levels (" << Worded << " worded, " << Channels << " with channels), " << Nums << " numbers");
}

TEST_CASE("Godot's fixed-decimal rounding: 17 digits, then half up")
{
	CHECK(Pc::FixedLikeGodot(1.5, 2) == "1.50");
	CHECK(Pc::FixedLikeGodot(1.25, 1) == "1.3");   // a binary-exact half rounds up
	CHECK(Pc::FixedLikeGodot(1.45, 1) == "1.5");   // 1.4500000000000000 at 17 digits
	CHECK(Pc::FixedLikeGodot(9.95, 1) == "9.9");   // 9.9499999999999993 at 17 digits
	CHECK(Pc::FixedLikeGodot(0.0, 3) == "0.000");
	CHECK(Pc::FixedLikeGodot(0.0004, 3) == "0.000");
	CHECK(Pc::FixedLikeGodot(0.0005, 3) == "0.001");
	CHECK(Pc::FixedLikeGodot(-1.5, 0) == "-2");
	CHECK(Pc::FixedLikeGodot(59.5, 0) == "60");
	CHECK(Pc::RoundiLikeGodot(2.5) == "3");
	CHECK(Pc::RoundiLikeGodot(-2.5) == "-3");
	CHECK(Pc::IntLikeGodot(-2.7) == "-2");
}

TEST_CASE("The tree tooltip: what it says, and why a node cannot be bought")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P;
	CHECK_FALSE(Pc::TreeTip(D, P, "no_such_branch", true).bKnown);

	// A trunk, never bought, no Stardust: Next and its cost; in browse it says spend after a Reawaken, in the spend step
	// how much you have.
	Pc::PcTreeTip T = Pc::TreeTip(D, P, "instinct", false);
	CHECK(T.bKnown);
	CHECK_FALSE(T.bMaxed);
	CHECK_FALSE(T.bShowNow);
	CHECK(T.Next == 1);
	CHECK(T.Cost == static_cast<int64_t>(Pc::NextBranchCost(D, P, "instinct")));
	CHECK(T.Reason == Pc::PcTreeTipReason::BrowseOnly);
	CHECK_FALSE(T.bBuyPrompt);
	P.Stardust = 0.5;
	T = Pc::TreeTip(D, P, "instinct", true);
	CHECK(T.Reason == Pc::PcTreeTipReason::Short);
	CHECK(T.Have == 0); // int(0.5)
	P.Stardust = 1e12;
	T = Pc::TreeTip(D, P, "instinct", true);
	CHECK(T.Reason == Pc::PcTreeTipReason::None);
	CHECK(T.bBuyPrompt);
	T = Pc::TreeTip(D, P, "instinct", false);
	CHECK_FALSE(T.bBuyPrompt); // browsing never buys
	CHECK(T.Reason == Pc::PcTreeTipReason::BrowseOnly);

	// Owned and not maxed: Now and Next. Maxed: Fully bought, no cost, no prompt.
	const Pc::PcSkillBranch* Instinct = D.FindBranch("instinct");
	REQUIRE(Instinct);
	P.BranchLevels["instinct"] = 1;
	T = Pc::TreeTip(D, P, "instinct", true);
	if (Pc::BranchMaxLevel(*Instinct) > 1)
	{
		CHECK(T.bShowNow);
		CHECK(T.Next == 2);
	}
	P.BranchLevels["instinct"] = Pc::BranchMaxLevel(*Instinct);
	T = Pc::TreeTip(D, P, "instinct", true);
	CHECK(T.bMaxed);
	CHECK_FALSE(T.bShowNow);
	CHECK_FALSE(T.bBuyPrompt);
	CHECK(T.Reason == Pc::PcTreeTipReason::None);

	// A required parent unbought names the parent; a decided fork names the rival; else the Reawaken gate.
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		Pc::PcProgress Q;
		Q.Stardust = 1e12;
		Q.ReawakenCount = 99;
		for (const Pc::PcSkillBranch& Any : D.Branches)
		{
			Q.BossesDefeated.insert(Any.GatedByBoss);
		}
		if (B.bParentIsRequired && !B.Parent.empty() && B.ExclusiveWith.empty())
		{
			const Pc::PcTreeTip U = Pc::TreeTip(D, Q, B.Id, true);
			CHECK(U.Reason == Pc::PcTreeTipReason::NeedsParent);
			CHECK(U.Parent == B.Parent);
		}
		if (!B.ExclusiveWith.empty())
		{
			Q.BranchLevels[B.Parent] = 1;
			Q.BranchLevels[B.ExclusiveWith.front()] = 1;
			const Pc::PcTreeTip U = Pc::TreeTip(D, Q, B.Id, true);
			CHECK(U.Reason == Pc::PcTreeTipReason::Closed);
			CHECK(U.Rival == B.ExclusiveWith.front());
		}
	}
	bool bGated = false;
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		if (B.Parent.empty() && Pc::BranchReawakenGate(D, B.Id) > 0)
		{
			Pc::PcProgress Q;
			const Pc::PcTreeTip U = Pc::TreeTip(D, Q, B.Id, true);
			CHECK(U.Reason == Pc::PcTreeTipReason::ReawakenGate);
			CHECK(U.Gate == Pc::BranchReawakenGate(D, B.Id));
			bGated = true;
		}
	}
	CHECK(bGated); // a gated trunk exists, so the gate wording is exercised
}

#endif
