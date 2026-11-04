#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCardDraw.h"
#include "PcCards.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcProgress.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
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

	std::vector<std::string> List(const std::string& S)
	{
		std::vector<std::string> Out;
		for (const std::string& Id : SplitKeep(S, ';'))
		{
			if (!Id.empty())
			{
				Out.push_back(Id);
			}
		}
		return Out;
	}

	std::string JoinList(const std::vector<std::string>& S)
	{
		std::string Out;
		for (size_t I = 0; I < S.size(); ++I)
		{
			Out += (I ? ";" : "") + S[I];
		}
		return Out;
	}

	std::string Join(const std::set<std::string>& S)
	{
		std::string Out;
		for (const std::string& Id : S)
		{
			Out += (Out.empty() ? "" : ";") + Id;
		}
		return Out;
	}

	std::string PairsAll(const std::map<std::string, int64_t>& M)
	{
		std::string Out;
		for (const auto& [K, V] : M)
		{
			Out += (Out.empty() ? "" : ";") + K + "=" + std::to_string(V);
		}
		return Out;
	}

	double Hex(const std::string& S)
	{
		return std::strtod(S.c_str(), nullptr);
	}

	int64_t Int(const std::string& S)
	{
		return std::atoll(S.c_str());
	}

	uint64_t U64(const std::string& S)
	{
		return std::strtoull(S.c_str(), nullptr, 10);
	}

	const char* Flag(bool b)
	{
		return b ? "1" : "0";
	}

	// Godot 4.7.1's global RNG (RandomPCG): PCG32 seeded by seed(s), randi() = one output, randf() = one output rounded to
	// a 32-bit float and scaled by 2^-32 (pinned by the fixture's calibration column). The rounding to 24 significant bits
	// is done in integers: PawCore has no `float`.
	class GodotRolls final : public Pc::PcCardRolls
	{
	public:
		explicit GodotRolls(uint64_t Seed)
		{
			Inc = (1442695040888963407ull << 1) | 1ull;
			State = 0;
			Next();
			State += Seed;
			Next();
		}

		uint32_t Next()
		{
			const uint64_t Old = State;
			State = Old * 6364136223846793005ull + Inc;
			const uint32_t Xorshifted = static_cast<uint32_t>(((Old >> 18u) ^ Old) >> 27u);
			const uint32_t Rot = static_cast<uint32_t>(Old >> 59u);
			return (Xorshifted >> Rot) | (Xorshifted << ((32u - Rot) & 31u));
		}

		static double FloatUnit(uint32_t U)
		{
			uint64_t Q = U;
			int Shift = 0;
			while ((Q >> Shift) >= (1ull << 24))
			{
				++Shift;
			}
			if (Shift > 0)
			{
				const uint64_t Rem = Q & ((1ull << Shift) - 1);
				const uint64_t Half = 1ull << (Shift - 1);
				Q >>= Shift;
				if (Rem > Half || (Rem == Half && (Q & 1ull)))
				{
					++Q;
				}
				Q <<= Shift;
			}
			return std::ldexp(static_cast<double>(Q), -32);
		}

		double Unit() override
		{
			++Used;
			return FloatUnit(Next());
		}

		int64_t Below(int64_t Count) override
		{
			++Used;
			return static_cast<int64_t>(Next() % static_cast<uint32_t>(Count));
		}

		int Used = 0;

	private:
		uint64_t State = 0;
		uint64_t Inc = 0;
	};

	using Row = std::map<std::string, std::string>;

	// Every field of the fixture's digest, in its order.
	void CheckDigest(const Pc::PcProgress& P, const std::string& Want)
	{
		const std::vector<std::string> G = SplitKeep(Want, ',');
		REQUIRE(G.size() == 13);
		CHECK(JoinList(P.Hand) == G[0]);
		CHECK(JoinList(P.PromisedCards) == G[1]);
		CHECK(P.PrototypeCard == G[2]);
		CHECK(P.Rerolls == Int(G[3]));
		CHECK(P.Banishes == Int(G[4]));
		CHECK(P.FixationHands == Int(G[5]));
		CHECK(P.ExtraChoices == Int(G[6]));
		CHECK(P.CatchesLevel == Int(G[7]));
		CHECK(P.CatchesPoints == Hex(G[8]));
		CHECK(P.Catnip == Hex(G[9]));
		CHECK(PairsAll(P.CardLevels) == G[10]);
		CHECK(Join(P.BanishedCards) == G[11]);
		CHECK(P.CardsTakenThisRun == Int(G[12]));
	}

	struct DrawResult
	{
		int Rows = 0;
		std::map<std::string, int> Ops;
		std::map<std::string, int> Paths;
	};

	void CheckCalibration(const std::string& Calib)
	{
		REQUIRE(!Calib.empty());
		for (const std::string& Part : SplitKeep(Calib, '|'))
		{
			const std::vector<std::string> F = SplitKeep(Part, '=');
			REQUIRE(F.size() == 4);
			const uint64_t Seed = U64(F[0]);
			CAPTURE(Seed);
			GodotRolls A(Seed);
			for (const std::string& V : SplitKeep(F[1], ';'))
			{
				CHECK(A.Next() == U64(V));
			}
			GodotRolls B(Seed);
			for (const std::string& V : SplitKeep(F[2], ';'))
			{
				CHECK(B.Unit() == Hex(V));
			}
			GodotRolls M(Seed);
			const std::vector<std::string> Mixed = SplitKeep(F[3], ';');
			REQUIRE(Mixed.size() == 4);
			CHECK(M.Unit() == Hex(Mixed[0]));
			CHECK(M.Next() == U64(Mixed[1]));
			CHECK(M.Unit() == Hex(Mixed[2]));
			CHECK(M.Below(7) == Int(Mixed[3]));
		}
	}

	DrawResult RunDrawFixture(const std::string& Path)
	{
		DrawResult R;
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
			Row V;
			for (size_t I = 0; I < Cells.size(); ++I)
			{
				V[Header[I]] = Cells[I];
			}
			CAPTURE(R.Rows);
			if (!V.at("calib").empty())
			{
				CheckCalibration(V.at("calib"));
				R.Paths["calibration"] += 1;
			}

			Pc::PcProgress P;
			P.UpgradeLevels = Pairs(V.at("s0_upgrades"));
			P.BranchLevels = Pairs(V.at("s0_branches"));
			for (const std::string& Id : List(V.at("s0_jobs_held")))
			{
				P.JobsHeld.insert(Id);
			}
			P.RunJob = V.at("s0_run_job");
			P.RunJobSecond = V.at("s0_run_job_second");
			P.ReawakenCount = Int(V.at("s0_reawaken_count"));
			P.DeepestBiomeIndex = Int(V.at("s0_deepest"));
			const bool bAutoPick = V.at("s0_auto_pick") == "1";
			const std::vector<std::string> S0 = SplitKeep(V.at("s0_digest"), ',');
			REQUIRE(S0.size() == 13);
			P.Hand = List(S0[0]);
			P.PromisedCards = List(S0[1]);
			P.PrototypeCard = S0[2];
			P.Rerolls = Int(S0[3]);
			P.Banishes = Int(S0[4]);
			P.FixationHands = Int(S0[5]);
			P.ExtraChoices = Int(S0[6]);
			P.CatchesLevel = Int(S0[7]);
			P.CatchesPoints = Hex(S0[8]);
			P.Catnip = Hex(S0[9]);
			P.CardLevels = Pairs(S0[10]);
			for (const std::string& Id : List(S0[11]))
			{
				P.BanishedCards.insert(Id);
			}
			P.CardsTakenThisRun = Int(S0[12]);
			CHECK(PairsAll(P.CardLevels) == V.at("s0_cards"));
			CheckDigest(P, V.at("s0_digest"));

			const std::vector<std::string> Ops = SplitKeep(V.at("ops"), '|');
			const std::vector<std::string> Results = SplitKeep(V.at("results"), '|');
			const std::vector<std::string> Digests = SplitKeep(V.at("digests"), '|');
			REQUIRE(Ops.size() == Results.size());
			REQUIRE(Ops.size() == Digests.size());
			for (size_t K = 0; K < Ops.size(); ++K)
			{
				const std::string& Op = Ops[K];
				CAPTURE(K);
				CAPTURE(Op);
				const size_t Colon = Op.find(':');
				const std::string Name = Op.substr(0, Colon);
				const std::vector<std::string> Args = SplitKeep(Op.substr(Colon + 1), ',');
				const std::string& Want = Results[K];
				R.Ops[Name] += 1;
				const int64_t FixationBefore = P.FixationHands;
				const bool bPendingBefore = !P.Hand.empty();
				const auto Dealt = [&R, &P, &D, FixationBefore](const Pc::PcDeal& Deal) {
					R.Paths["deal offered"] += Deal.bOffered ? 1 : 0;
					R.Paths["deal auto-picked"] += Deal.AutoPicked.empty() ? 0 : 1;
					R.Paths["deal empty"] += !Deal.bOffered && Deal.AutoPicked.empty() ? 1 : 0;
					R.Paths["deal promised next"] += Deal.bOffered && !P.PromisedCards.empty() ? 1 : 0;
					R.Paths["deal prototype marked"] += Deal.bOffered && !P.PrototypeCard.empty() ? 1 : 0;
					R.Paths["deal under fixation"] += FixationBefore > 0 ? 1 : 0;
					R.Paths["deal wide hand"] += static_cast<int64_t>(P.Hand.size()) > D.Code.BaseHandSize ? 1 : 0;
					(void)D;
				};
				if (Name == "gain")
				{
					GodotRolls Rolls(U64(Args[0]));
					const Pc::PcCatchesGain G = Pc::GainCatches(D, P, Hex(Args[1]), Rolls, bAutoPick);
					CHECK(Flag(G.bLevelled) == Want);
					R.Paths["gain levelled"] += G.bLevelled ? 1 : 0;
					R.Paths["gain while a hand is up"] += bPendingBefore ? 1 : 0;
					R.Paths["gain paid interest"] += G.InterestPaid > 0.0 ? 1 : 0;
					if (G.bLevelled)
					{
						Dealt(G.Deal);
					}
				}
				else if (Name == "draw")
				{
					GodotRolls Rolls(U64(Args[0]));
					const std::vector<std::string> Got = Pc::DrawHand(D, P, Int(Args[1]), Rolls);
					CHECK(JoinList(Got) == Want);
					R.Paths["draw short of size"] += static_cast<int64_t>(Got.size()) < Int(Args[1]) ? 1 : 0;
					R.Paths["draw under fixation"] += P.FixationHands > 0 ? 1 : 0;
				}
				else if (Name == "offer")
				{
					GodotRolls Rolls(U64(Args[0]));
					const bool bHadPromise = !P.PromisedCards.empty();
					Dealt(Pc::OfferHand(D, P, Rolls, bAutoPick));
					R.Paths["deal opened with a promise"] += bHadPromise ? 1 : 0;
				}
				else if (Name == "reroll")
				{
					GodotRolls Rolls(U64(Args[0]));
					const bool bOk = Pc::RerollHand(D, P, Rolls);
					CHECK(Flag(bOk) == Want);
					R.Paths[bOk ? "reroll done" : "reroll refused"] += 1;
				}
				else if (Name == "choose")
				{
					const int64_t Before = Pc::CardLevel(P, Args[0]);
					const bool bMarked = !P.PrototypeCard.empty() && P.PrototypeCard == Args[0];
					const Pc::PcTake T = Pc::ChooseCard(D, P, Args[0]);
					CHECK(Flag(T.bTaken) == Want);
					R.Paths[T.bTaken ? "choose taken" : "choose refused"] += 1;
					R.Paths["choose prototype bonus"] += T.bTaken && bMarked && T.NewLevel > Before + 1 ? 1 : 0;
					R.Paths["choose capped"] += T.bTaken && T.NewLevel == Before ? 1 : 0;
					if (T.bTaken)
					{
						for (const Pc::PcCard& C : D.Cards)
						{
							if (C.Id == Args[0])
							{
								R.Paths[std::string("choose ") + Pc::ToName(C.Effect)] += 1;
							}
						}
					}
				}
				else if (Name == "banish")
				{
					GodotRolls Rolls(U64(Args[0]));
					const bool bOk = Pc::BanishFromHand(D, P, Args[1], Rolls);
					CHECK(Flag(bOk) == Want);
					R.Paths[bOk ? "banish done" : "banish refused"] += 1;
					R.Paths["banish dry"] += bOk && P.Hand.empty() ? 1 : 0;
				}
				else
				{
					FAIL("unknown op " << Name);
				}
				CheckDigest(P, Digests[K]);
			}
			++R.Rows;
		}
		return R;
	}
}

TEST_CASE("The card draw matches Godot's CardManager deal, card for card, on 1,000 random states x 8 seeded operations")
{
	const DrawResult R = RunDrawFixture(DataDir() + "/../Source/PawCore/Tests/Data/draw_godot.tsv");
	CHECK(R.Rows == 1000);
	for (const char* Op : { "gain", "draw", "offer", "reroll", "choose", "banish" })
	{
		CHECK_MESSAGE(R.Ops.count(Op) == 1, Op);
		CHECK_MESSAGE(R.Ops.at(Op) > 100, Op);
	}
	std::string Seen;
	for (const auto& [Path, N] : R.Paths)
	{
		Seen += Path + " " + std::to_string(N) + ", ";
	}
	CHECK(R.Paths.count("calibration") == 1);
	for (const char* Path : { "gain levelled", "gain while a hand is up", "gain paid interest", "deal offered",
		"deal auto-picked", "deal promised next", "deal prototype marked", "deal under fixation", "deal wide hand",
		"deal opened with a promise", "draw short of size", "draw under fixation", "reroll done", "reroll refused",
		"choose taken", "choose refused", "choose prototype bonus", "choose REROLL", "choose EXTRA_CHOICE",
		"choose FIXATION_HANDS", "choose BANISH", "banish done", "banish refused", "deal empty", "banish dry" })
	{
		CHECK_MESSAGE(R.Paths.count(Path) == 1, Path << " (" << Seen << ")");
		CHECK_MESSAGE((R.Paths.count(Path) ? R.Paths.at(Path) : 0) > 10, Path << " (" << Seen << ")");
	}
	MESSAGE(Seen);
}

TEST_CASE("Long: the card draw matches all 3,000 Godot deal rows" * doctest::test_suite("long") * doctest::skip())
{
	const DrawResult R = RunDrawFixture(DataDir() + "/../build/draw_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Card draw rules from the Godot comments")
{
	const Pc::PcGameData& D = Game();

	// The game's rolls: the same seed and deal deal the same hand; another deal number deals another.
	{
		Pc::PcProgress P;
		Pc::PcRngCardRolls A(7, 1), B(7, 1);
		const std::vector<std::string> First = Pc::DrawHand(D, P, 3, A);
		CHECK(First.size() == 3);
		CHECK(Pc::DrawHand(D, P, 3, B) == First);
		int Different = 0;
		for (uint64_t Deal = 2; Deal < 12; ++Deal)
		{
			Pc::PcRngCardRolls C(7, Deal);
			Different += Pc::DrawHand(D, P, 3, C) != First ? 1 : 0;
		}
		CHECK(Different >= 9);
	}

	// Weighted: over many deals the first card of a hand turns up in proportion to its weight (cost cards are rarer).
	{
		Pc::PcProgress P;
		const std::vector<const Pc::PcCard*> Pool = Pc::EligibleCards(D, P);
		REQUIRE(Pool.size() > 5);
		double Total = 0.0;
		for (const Pc::PcCard* C : Pool)
		{
			Total += C->Weight;
		}
		std::map<std::string, int> Seen;
		const int N = 200000;
		for (int I = 0; I < N; ++I)
		{
			Pc::PcRngCardRolls R(99, static_cast<uint64_t>(I));
			Seen[Pc::DrawHand(D, P, 1, R).at(0)] += 1;
		}
		for (const Pc::PcCard* C : Pool)
		{
			const double Want = C->Weight / Total;
			const double Got = static_cast<double>(Seen[C->Id]) / N;
			CHECK_MESSAGE(std::fabs(Got - Want) < 5.0 * std::sqrt(Want * (1.0 - Want) / N), C->Id << " " << Got << " vs " << Want);
		}
	}

	// The hand is the contract: a card not on offer is refused; a banish re-deals without consuming the level.
	{
		Pc::PcProgress P;
		P.Hand = { "spite", "dilated", "fixation" };
		P.Banishes = 1;
		P.CatchesLevel = 4;
		CHECK(!Pc::ChooseCard(D, P, "not_that_one").bTaken);
		Pc::PcRngCardRolls R(1, 1);
		CHECK(Pc::BanishFromHand(D, P, "spite", R));
		CHECK(P.BanishedCards.count("spite") == 1);
		CHECK(P.CatchesLevel == 4);
		CHECK(P.Banishes == 0);
		CHECK(std::find(P.Hand.begin(), P.Hand.end(), "spite") == P.Hand.end());
		Pc::PcRngCardRolls R2(1, 2);
		CHECK(!Pc::BanishFromHand(D, P, P.Hand.at(0), R2)); // none left
	}

	// The deck cards act where the card is taken; Prototype's bonus lands on the marked card, capped at its max.
	{
		Pc::PcProgress P;
		P.BranchLevels["prototype"] = 1;
		P.Hand = { "spite" };
		P.PrototypeCard = "spite";
		const Pc::PcTake T = Pc::ChooseCard(D, P, "spite");
		CHECK(T.bTaken);
		CHECK(T.NewLevel == 2); // one up, plus Prototype's one
		CHECK(P.Rerolls == 3);  // spite at level 2
		CHECK(P.Hand.empty());
		CHECK(P.PrototypeCard.empty());
		CHECK(P.CardsTakenThisRun == 1);
	}

	// A level-up while a hand is up banks the XP and deals nothing.
	{
		Pc::PcProgress P;
		P.Hand = { "spite" };
		Pc::PcRngCardRolls R(3, 3);
		const Pc::PcCatchesGain G = Pc::GainCatches(D, P, 1e9, R, false);
		CHECK(!G.bLevelled);
		CHECK(P.CatchesPoints == 1e9);
		CHECK(P.Hand == std::vector<std::string>{ "spite" });
	}

	// Fixation falls back to the whole pool when too few owned cards could fill the hand.
	{
		Pc::PcProgress P;
		P.FixationHands = 1;
		P.CardLevels["spite"] = 1;
		Pc::PcRngCardRolls R(5, 5);
		CHECK(Pc::DrawHand(D, P, 3, R).size() == 3);
	}

	// A new run keeps Overtime's rerolls and drops everything else the deal carried.
	{
		Pc::PcProgress P;
		P.BranchLevels["overtime"] = 2;
		P.Rerolls = 9;
		P.Banishes = 2;
		P.FixationHands = 1;
		P.Hand = { "spite" };
		P.PromisedCards = { "dilated" };
		P.PrototypeCard = "spite";
		Pc::ResetRunCards(D, P);
		CHECK(P.Rerolls == 2);
		CHECK(P.Banishes == 0);
		CHECK(P.FixationHands == 0);
		CHECK(P.Hand.empty());
		CHECK(P.PromisedCards.empty());
		CHECK(P.PrototypeCard.empty());
	}
}

#endif
