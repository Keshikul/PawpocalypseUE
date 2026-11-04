#if defined(PAWCORE_STANDALONE_TESTS)

// PcTreeLayout against the REAL Godot layout code (tools/godot_fixtures/tree_layout_fixture.gd): the shipping tree and 400
// random trees, every node's seat, the bounds, the canvas, and nav_pick in four directions from every node. Then the rules
// Godot's own gate (ci/check_hud.gd) holds the shipping tree to: every data node seated and nothing else, even spacing, no
// link through a node, no two links crossing; and the node states.

#include "ThirdParty/doctest/doctest.h"
#include "PcData.h"
#include "PcTree.h"
#include "PcTreeLayout.h"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
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

	std::vector<double> Doubles(const std::string& S)
	{
		std::vector<double> Out;
		if (S.empty())
		{
			return Out;
		}
		for (const std::string& P : Split(S, ';'))
		{
			Out.push_back(std::strtod(P.c_str(), nullptr));
		}
		return Out;
	}

	// The data with the fixture row's tree: only its branches, each with its parent ("-" = a trunk).
	Pc::PcGameData TreeOf(const std::vector<std::string>& Ids, const std::vector<std::string>& Parents)
	{
		const Pc::PcGameData& Real = Game();
		Pc::PcGameData D = Real;
		D.Branches.clear();
		for (size_t I = 0; I < Ids.size(); ++I)
		{
			const Pc::PcSkillBranch* B = Real.FindBranch(Ids[I]);
			REQUIRE(B != nullptr);
			Pc::PcSkillBranch Copy = *B;
			Copy.Parent = Parents[I] == "-" ? std::string() : Parents[I];
			D.Branches.push_back(Copy);
		}
		D.RebuildIndex();
		return D;
	}

	struct LayoutResult
	{
		int Rows = 0;
		int Nodes = 0;
		int Unclear = 0;
		double WorstPx = 0.0;
	};

	// PcTreeLayout emulates Godot's 32-bit Vector2 arithmetic, so every seat is the same 32-bit value: exact (the double
	// port was up to 1,247 px off, a whole cell moved by a tie, and 4,496 pad picks differed: 2026-11-02).
	constexpr double SeatTolerance = 1e-9;

	LayoutResult RunLayoutFixture(const std::string& Path)
	{
		std::ifstream In(Path);
		REQUIRE(In.good());
		LayoutResult R;
		std::string Line;
		std::vector<std::string> Header;
		const Pc::PcVec2 Dirs[4] = { { 0.0, -1.0 }, { 0.0, 1.0 }, { -1.0, 0.0 }, { 1.0, 0.0 } };
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> C = Split(Line, '\t');
			if (Header.empty())
			{
				Header = C;
				REQUIRE(Header.size() == 14);
				continue;
			}
			REQUIRE(C.size() == 14);
			const std::string Row = C[0];
			const Pc::PcGameData D = TreeOf(Split(C[1], ';'), Split(C[2], ';'));
			const Pc::PcTreeLayout L = Pc::LayOutTree(D);
			const std::vector<std::string> Order = Split(C[3], ';');
			INFO("tree " << Row);
			REQUIRE(L.Order == Order);
			const double HubX = std::strtod(C[4].c_str(), nullptr), HubY = std::strtod(C[5].c_str(), nullptr);
			CHECK(std::fabs(L.Hub.X - HubX) < SeatTolerance);
			CHECK(std::fabs(L.Hub.Y - HubY) < SeatTolerance);
			const std::vector<double> Xs = Doubles(C[6]), Ys = Doubles(C[7]);
			REQUIRE(Xs.size() == Order.size());
			for (size_t I = 0; I < Order.size(); ++I)
			{
				const auto It = L.Nodes.find(Order[I]);
				REQUIRE(It != L.Nodes.end());
				const double Off = std::max(std::fabs(It->second.X - Xs[I]), std::fabs(It->second.Y - Ys[I]));
				R.WorstPx = std::max(R.WorstPx, Off);
				INFO("node " << Order[I] << " at " << It->second.X << ", " << It->second.Y << "; Godot " << Xs[I] << ", " << Ys[I]);
				CHECK(Off < SeatTolerance);
				++R.Nodes;
			}
			const std::vector<double> Bounds = Doubles(C[8]), Canvas = Doubles(C[9]);
			const Pc::PcRect2 B = Pc::TreeBounds(L);
			CHECK(std::fabs(B.X - Bounds[0]) < SeatTolerance);
			CHECK(std::fabs(B.Y - Bounds[1]) < SeatTolerance);
			CHECK(std::fabs(B.W - Bounds[2]) < SeatTolerance);
			CHECK(std::fabs(B.H - Bounds[3]) < SeatTolerance);
			const Pc::PcVec2 Cv = Pc::TreeCanvasSize(B);
			CHECK(std::fabs(Cv.X - Canvas[0]) < SeatTolerance);
			CHECK(std::fabs(Cv.Y - Canvas[1]) < SeatTolerance);
			for (int K = 0; K < 4; ++K)
			{
				const std::vector<std::string> Want = Split(C[10 + K], ';');
				REQUIRE(Want.size() == Order.size());
				for (size_t I = 0; I < Order.size(); ++I)
				{
					const std::string Got = Pc::TreeNavPick(L, Order[I], Dirs[K]);
					INFO("nav from " << Order[I] << " dir " << K);
					CHECK((Got.empty() ? std::string("-") : Got) == Want[I]);
				}
			}
			R.Unclear += static_cast<int>(L.UnclearLimbs.size());
			if (Row == "0")
			{
				CHECK(L.UnclearLimbs.empty()); // the shipping tree places every limb clear
			}
			++R.Rows;
		}
		return R;
	}

	double SegDist(Pc::PcVec2 P, Pc::PcVec2 A, Pc::PcVec2 B)
	{
		const double NX = B.X - A.X, NY = B.Y - A.Y, L2 = NX * NX + NY * NY;
		double T = L2 > 0.0 ? ((P.X - A.X) * NX + (P.Y - A.Y) * NY) / L2 : 0.0;
		T = std::max(0.0, std::min(1.0, T));
		const double X = A.X + NX * T - P.X, Y = A.Y + NY * T - P.Y;
		return std::sqrt(X * X + Y * Y);
	}

	bool Cross(Pc::PcVec2 A, Pc::PcVec2 B, Pc::PcVec2 C, Pc::PcVec2 D)
	{
		auto O = [](Pc::PcVec2 P, Pc::PcVec2 Q, Pc::PcVec2 R) { return (Q.X - P.X) * (R.Y - P.Y) - (Q.Y - P.Y) * (R.X - P.X); };
		const double D1 = O(C, D, A), D2 = O(C, D, B), D3 = O(A, B, C), D4 = O(A, B, D);
		return ((D1 > 1e-6 && D2 < -1e-6) || (D1 < -1e-6 && D2 > 1e-6)) && ((D3 > 1e-6 && D4 < -1e-6) || (D3 < -1e-6 && D4 > 1e-6));
	}
}

TEST_CASE("PcTreeLayout matches Godot's SkillTreeUI layout on the shipping tree and 400 random trees")
{
	const LayoutResult R = RunLayoutFixture(DataDir() + "/../Source/PawCore/Tests/Data/tree_layout_godot.tsv");
	CHECK(R.Rows == 401);
	CHECK(R.Nodes > 10000);
	// The random trees reach the crowded fallback (a limb with no clear offset) as Godot did: it was exercised.
	CHECK(R.Unclear > 0);
	MESSAGE(R.Nodes << " seats, worst " << R.WorstPx << " px off; " << R.Unclear << " limbs placed by the fallback");
}

TEST_CASE("The shipping tree: every node seated, spaced, no link through a node or another link (Godot's check_hud)")
{
	const Pc::PcGameData& D = Game();
	const Pc::PcTreeLayout L = Pc::LayOutTree(D);
	REQUIRE(L.Nodes.size() == D.Branches.size());
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		CHECK(L.Nodes.count(B.Id) == 1);
	}
	CHECK(L.Order.size() == D.Branches.size());
	CHECK(L.UnclearLimbs.empty());
	// Spacing: no two nodes closer than NodeGap less a pixel (the separation's promise), none overlapping the hub.
	double Closest = 1e18;
	for (auto A = L.Nodes.begin(); A != L.Nodes.end(); ++A)
	{
		for (auto B = std::next(A); B != L.Nodes.end(); ++B)
		{
			Closest = std::min(Closest, std::hypot(A->second.X - B->second.X, A->second.Y - B->second.Y));
		}
		CHECK(std::hypot(A->second.X - L.Hub.X, A->second.Y - L.Hub.Y) >= Pc::TreeLayout::HexPitch - 0.5);
	}
	CHECK(Closest >= Pc::TreeLayout::NodeGap - 1.0);
	// Links: from each node's parent (or the hub); none passes over a third node, none crosses another.
	std::vector<std::pair<Pc::PcVec2, Pc::PcVec2>> Links;
	std::vector<std::pair<std::string, std::string>> Ends;
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		const Pc::PcVec2 From = B.Parent.empty() ? L.Hub : L.Nodes.at(B.Parent);
		Links.push_back({ From, L.Nodes.at(B.Id) });
		Ends.push_back({ B.Parent.empty() ? std::string("hub") : B.Parent, B.Id });
	}
	int ThroughNode = 0, Crossing = 0;
	for (size_t I = 0; I < Links.size(); ++I)
	{
		for (const auto& N : L.Nodes)
		{
			if (N.first == Ends[I].first || N.first == Ends[I].second)
			{
				continue;
			}
			if (SegDist(N.second, Links[I].first, Links[I].second) < Pc::TreeLayout::NodeSize * 0.5)
			{
				++ThroughNode;
			}
		}
		for (size_t J = I + 1; J < Links.size(); ++J)
		{
			if (Cross(Links[I].first, Links[I].second, Links[J].first, Links[J].second))
			{
				++Crossing;
			}
		}
	}
	CHECK(ThroughNode == 0);
	CHECK(Crossing == 0);
	// Inside its canvas, top left at the margin.
	const Pc::PcRect2 B = Pc::TreeBounds(L);
	const Pc::PcVec2 Cv = Pc::TreeCanvasSize(B);
	CHECK(B.X >= Pc::TreeLayout::FitMargin - 1e-9);
	CHECK(B.Y >= Pc::TreeLayout::FitMargin - 1e-9);
	CHECK(B.X + B.W <= Cv.X);
	CHECK(B.Y + B.H <= Cv.Y);
	// Every node reaches every other by the four directions (the pad never strands a node).
	std::set<std::string> Seen{ L.Order.front() };
	std::vector<std::string> Queue{ L.Order.front() };
	const Pc::PcVec2 Dirs[4] = { { 0.0, -1.0 }, { 0.0, 1.0 }, { -1.0, 0.0 }, { 1.0, 0.0 } };
	for (size_t H = 0; H < Queue.size(); ++H)
	{
		for (const Pc::PcVec2& Dir : Dirs)
		{
			const std::string Next = Pc::TreeNavPick(L, Queue[H], Dir);
			if (!Next.empty() && Seen.insert(Next).second)
			{
				Queue.push_back(Next);
			}
		}
	}
	CHECK(Seen.size() == L.Nodes.size());
}

TEST_CASE("Tree fit zoom, node states, the affordable jumps and the links")
{
	const Pc::PcGameData& D = Game();
	const Pc::PcTreeLayout L = Pc::LayOutTree(D);
	const Pc::PcRect2 B = Pc::TreeBounds(L);
	const double Fit = Pc::TreeFitZoom(B, 1920.0, 950.0);
	CHECK(Fit >= Pc::TreeLayout::ZoomMin);
	CHECK(Fit <= Pc::TreeLayout::FitMax);
	CHECK(Fit == doctest::Approx(std::min(1920.0 / (B.W + 180.0), 950.0 / (B.H + 180.0))));
	CHECK(Pc::TreeFitZoom(B, 0.0, 0.0) == Fit); // no view yet: Godot's 1920 x 950
	CHECK(Pc::TreeFitZoom(Pc::PcRect2{ 0.0, 0.0, 10.0, 10.0 }, 1920.0, 950.0) == Pc::TreeLayout::FitMax);

	Pc::PcProgress P;
	const std::string Trunk = Pc::TrunkOrder(D).front();
	CHECK(Trunk == "instinct");
	CHECK(Pc::TreeNodeState(D, P, Trunk) == Pc::PcTreeNodeState::Locked);
	CHECK(Pc::CheapestAffordableBranch(D, P).empty());
	CHECK_FALSE(Pc::HasAffordableBranch(D, P));
	CHECK(Pc::NextAffordableBranch(D, P, Trunk).empty());
	P.Stardust = Pc::NextBranchCost(D, P, Trunk);
	CHECK(Pc::TreeNodeState(D, P, Trunk) == Pc::PcTreeNodeState::Ready);
	CHECK(Pc::HasAffordableBranch(D, P));
	const std::string Cheapest = Pc::CheapestAffordableBranch(D, P);
	CHECK_FALSE(Cheapest.empty());
	CHECK(Pc::NextBranchCost(D, P, Cheapest) <= P.Stardust);
	// Wrapping: from the cheapest, the jump comes back round to a purchasable node (itself when it is the only one).
	CHECK(Pc::TreeCanBuyNow(D, P, Pc::NextAffordableBranch(D, P, Cheapest)));

	// Rich: every trunk for sale. The cheapest is the first of the lowest price in branch order (Godot's strict <), and
	// the jump walks the branch order from where it stands.
	{
		Pc::PcProgress Rich;
		Rich.Stardust = 1e300;
		Rich.ReawakenCount = 99;
		double Low = 1e308;
		std::vector<std::string> AtLow;
		for (const std::string& Id : Pc::BranchOrder(D))
		{
			if (!Pc::TreeCanBuyNow(D, Rich, Id))
			{
				continue;
			}
			const double C = Pc::NextBranchCost(D, Rich, Id);
			if (C < Low)
			{
				Low = C;
				AtLow.clear();
			}
			if (C == Low)
			{
				AtLow.push_back(Id);
			}
		}
		REQUIRE(AtLow.size() > 1); // a tie exists, so the rule is exercised
		CHECK(Pc::CheapestAffordableBranch(D, Rich) == AtLow.front());
		const std::vector<std::string> Order = Pc::BranchOrder(D);
		std::vector<std::string> ForSale;
		for (const std::string& Id : Order)
		{
			if (Pc::TreeCanBuyNow(D, Rich, Id))
			{
				ForSale.push_back(Id);
			}
		}
		REQUIRE(ForSale.size() > 2);
		CHECK(Pc::NextAffordableBranch(D, Rich, ForSale[1]) == ForSale[2]);
		CHECK(Pc::NextAffordableBranch(D, Rich, "no_such_branch") == ForSale[0]);
	}
	P.BranchLevels[Trunk] = 1;
	P.Stardust = 0.0;
	CHECK(Pc::TreeNodeState(D, P, Trunk) == Pc::PcTreeNodeState::Owned); // bought stays bought-looking, broke or not

	// Links: a trunk's link is owned with the trunk; a child's needs both ends.
	const std::vector<Pc::PcTreeLink> Links = Pc::TreeLinks(D, P);
	REQUIRE(Links.size() == D.Branches.size());
	for (const Pc::PcTreeLink& Lk : Links)
	{
		if (Lk.To == Trunk)
		{
			CHECK(Lk.From == "hub");
			CHECK(Lk.bOwned);
		}
		const Pc::PcSkillBranch* Br = D.FindBranch(Lk.To);
		if (Br && Br->Parent == Trunk)
		{
			CHECK_FALSE(Lk.bOwned); // the child is not bought
		}
		CHECK_FALSE((Lk.bFork && Lk.bShut));
	}
	{
		Pc::PcProgress Q;
		std::string Child, Parent;
		for (const Pc::PcSkillBranch& Br : D.Branches)
		{
			if (!Br.Parent.empty())
			{
				Child = Br.Id;
				Parent = Br.Parent;
				break;
			}
		}
		REQUIRE_FALSE(Child.empty());
		Q.BranchLevels[Child] = 1; // a free grant can do this (Muscle Memory)
		for (const Pc::PcTreeLink& Lk : Pc::TreeLinks(D, Q))
		{
			if (Lk.To == Child)
			{
				CHECK_FALSE(Lk.bOwned);
			}
			if (Lk.From == "hub")
			{
				CHECK_FALSE(Lk.bOwned); // no trunk bought
			}
		}
	}
}

TEST_CASE("Tree separation: the outer of two crowded nodes moves out along its ray to NodeGap (skill_tree_ui.gd)")
{
	// Crafted, because no hex-lattice seat is ever this close (the fixture never moves one).
	Pc::PcTreeLayout L;
	L.Order = { "inner", "outer", "far" };
	const double CX = Pc::TreeLayout::CanvasW / 2.0, CY = Pc::TreeLayout::CanvasH / 2.0;
	L.Nodes["inner"] = { CX + 400.0, CY };
	L.Nodes["outer"] = { CX + 500.0, CY };        // 100 px from inner, further from the centre
	L.Nodes["far"] = { CX, CY - 1000.0 };         // nowhere near: never moves
	Pc::SeparateTreeNodes(L);
	CHECK(L.Nodes["inner"].X == doctest::Approx(CX + 400.0));
	CHECK(L.Nodes["inner"].Y == doctest::Approx(CY));
	CHECK(L.Nodes["outer"].X == doctest::Approx(CX + 400.0 + Pc::TreeLayout::NodeGap).epsilon(1e-6));
	CHECK(L.Nodes["outer"].Y == doctest::Approx(CY));
	CHECK(L.Nodes["far"].Y == doctest::Approx(CY - 1000.0));

	// Order does not decide who moves; the radius does.
	Pc::PcTreeLayout M;
	M.Order = { "outer", "inner" };
	M.Nodes["inner"] = { CX, CY + 300.0 };
	M.Nodes["outer"] = { CX, CY + 350.0 };
	Pc::SeparateTreeNodes(M);
	CHECK(M.Nodes["inner"].Y == doctest::Approx(CY + 300.0));
	CHECK(M.Nodes["outer"].Y == doctest::Approx(CY + 300.0 + Pc::TreeLayout::NodeGap).epsilon(1e-6));
}

#endif
