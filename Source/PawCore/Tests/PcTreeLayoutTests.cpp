#if defined(PAWCORE_STANDALONE_TESTS)

// PcTreeLayout: the Compass (concept C1, designer 2026-11-26). The shipping tree: every skill seated on its own grid
// square, one pitch apart at least, none on the root's; no link crossing another or passing over a skill; each trunk in its
// slice; the whole tree inside a one-screen box; the same tree every time; still clean without the four skills the
// playtest's decisions remove. The reveal (one ring ahead), the pad's reach over the revealed skills, the node states.
// (Until 2026-11-26 this file proved the first try's hex layout against Godot's code, tree_layout_godot.tsv; retired.)

#include "ThirdParty/doctest/doctest.h"
#include "PcData.h"
#include "PcTree.h"
#include "PcTreeLayout.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
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

namespace
{
	// The checks every Compass tree must pass: every data skill seated once, on the grid, on its own square, none on the
	// root's; no link crossing another; no link passing over a third skill. Returns the problems, empty when clean.
	std::vector<std::string> CompassProblems(const Pc::PcGameData& D, const Pc::PcTreeLayout& L)
	{
		std::vector<std::string> Out;
		const double P = Pc::TreeLayout::GridPitch;
		size_t Active = 0;
		for (const Pc::PcSkillBranch& B : D.Branches)
		{
			Active += Pc::IsBranchRetired(D, B.Id) ? 0 : 1; // a retired skill is not on the tree (D-e)
		}
		if (L.Nodes.size() != Active)
		{
			Out.push_back("seated " + std::to_string(L.Nodes.size()) + " of " + std::to_string(Active));
		}
		if (!L.UnclearLimbs.empty())
		{
			Out.push_back("unseated: " + L.UnclearLimbs.front());
		}
		std::set<std::pair<long long, long long>> Squares{ { 0, 0 } };
		for (const auto& N : L.Nodes)
		{
			const double GX = (N.second.X - L.Hub.X) / P, GY = (N.second.Y - L.Hub.Y) / P;
			if (std::fabs(GX - std::round(GX)) > 1e-9 || std::fabs(GY - std::round(GY)) > 1e-9)
			{
				Out.push_back(N.first + " is off the grid");
			}
			if (!Squares.insert({ std::llround(GX), std::llround(GY) }).second)
			{
				Out.push_back(N.first + " shares a square");
			}
		}
		std::vector<std::pair<Pc::PcVec2, Pc::PcVec2>> Links;
		std::vector<std::pair<std::string, std::string>> Ends;
		for (const Pc::PcSkillBranch& B : D.Branches)
		{
			if (Pc::IsBranchRetired(D, B.Id))
			{
				continue;
			}
			const Pc::PcVec2 From = B.Parent.empty() ? L.Hub : L.Nodes.at(B.Parent);
			Links.push_back({ From, L.Nodes.at(B.Id) });
			Ends.push_back({ B.Parent.empty() ? std::string("hub") : B.Parent, B.Id });
		}
		for (size_t I = 0; I < Links.size(); ++I)
		{
			for (const auto& N : L.Nodes)
			{
				if (N.first != Ends[I].first && N.first != Ends[I].second
					&& SegDist(N.second, Links[I].first, Links[I].second) < Pc::TreeLayout::GridLinkClearance * P - 1e-6)
				{
					Out.push_back("the link to " + Ends[I].second + " passes over " + N.first);
				}
			}
			for (size_t J = I + 1; J < Links.size(); ++J)
			{
				if (Cross(Links[I].first, Links[I].second, Links[J].first, Links[J].second))
				{
					Out.push_back("the links to " + Ends[I].second + " and " + Ends[J].second + " cross");
				}
			}
		}
		return Out;
	}
}

TEST_CASE("The Compass: every skill on its own square, no crossing links, no link over a skill, each trunk in its slice, one screen")
{
	const Pc::PcGameData& D = Game();
	const Pc::PcTreeLayout L = Pc::LayOutTree(D);
	const std::vector<std::string> Problems = CompassProblems(D, L);
	for (const std::string& Pr : Problems)
	{
		INFO(Pr);
		CHECK(false);
	}
	CHECK(Problems.empty());
	// The same tree every time (no hidden state, no order from a hash).
	const Pc::PcTreeLayout Again = Pc::LayOutTree(D);
	for (const auto& N : L.Nodes)
	{
		CHECK(Again.Nodes.at(N.first).X == N.second.X);
		CHECK(Again.Nodes.at(N.first).Y == N.second.Y);
	}
	// Each trunk stands in the middle of its slice: TrunkWheel clockwise from straight up, equal slices.
	const std::vector<std::string> Trunks = Pc::TrunkOrder(D);
	const double Span = 360.0 / static_cast<double>(Trunks.size());
	for (size_t I = 0; I < Trunks.size(); ++I)
	{
		const Pc::PcVec2 T = L.Nodes.at(Trunks[I]);
		const double A = std::atan2(T.Y - L.Hub.Y, (T.X - L.Hub.X) / Pc::TreeLayout::GridStretch) * 180.0 / 3.14159265358979323846;
		double Want = -90.0 + Span * static_cast<double>(I);
		double Off = std::fabs(A - Want);
		Off = std::min(Off, 360.0 - Off);
		INFO(Trunks[I] << " at " << A << " degrees, its slice's middle " << Want);
		CHECK(Off <= Span * 0.5);
	}
	CHECK(Trunks.front() == "claws"); // straight up first (the concept)
	// One screen: no wider than 16 squares, no taller than 15 (the concept measured 15 x 14).
	double X0 = 1e18, X1 = -1e18, Y0 = 1e18, Y1 = -1e18;
	for (const auto& N : L.Nodes)
	{
		X0 = std::min(X0, N.second.X); X1 = std::max(X1, N.second.X);
		Y0 = std::min(Y0, N.second.Y); Y1 = std::max(Y1, N.second.Y);
	}
	CHECK((X1 - X0) / Pc::TreeLayout::GridPitch + 1.0 <= 16.0);
	CHECK((Y1 - Y0) / Pc::TreeLayout::GridPitch + 1.0 <= 15.0);
	MESSAGE("the Compass: " << (X1 - X0) / Pc::TreeLayout::GridPitch + 1.0 << " x " << (Y1 - Y0) / Pc::TreeLayout::GridPitch + 1.0 << " squares");
}

TEST_CASE("The Compass stays clean without the four skills the playtest removes (Audit, Deep Pockets, Auto-Bait, Auto-Card)")
{
	Pc::PcGameData D = Game();
	const std::set<std::string> Gone{ "audit", "deep_pockets", "auto_bait", "auto_card" };
	D.Branches.erase(std::remove_if(D.Branches.begin(), D.Branches.end(), [&](const Pc::PcSkillBranch& B) { return Gone.count(B.Id) != 0; }),
		D.Branches.end());
	D.RebuildIndex();
	const std::vector<std::string> Problems = CompassProblems(D, Pc::LayOutTree(D));
	for (const std::string& Pr : Problems)
	{
		INFO(Pr);
		CHECK(false);
	}
	CHECK(Problems.empty());
}

TEST_CASE("The reveal: owned or on sale in full, one ring ahead faint, the rest hidden; the pad only among the revealed")
{
	const Pc::PcGameData& D = Game();
	Pc::PcProgress P;
	using R = Pc::PcTreeReveal;
	// A fresh game: the trunks for sale show in full, a gated trunk faintly; a trunk's first ring faint; deeper hidden.
	CHECK(Pc::TreeNodeReveal(D, P, "claws") == R::Full);
	CHECK(Pc::TreeNodeReveal(D, P, "unlock_aura") == R::Full);
	REQUIRE(Pc::BranchReawakenGate(D, "unlock_snare") > 0);
	CHECK(Pc::TreeNodeReveal(D, P, "unlock_snare") == R::Ghost);
	CHECK(Pc::TreeNodeReveal(D, P, "aura_damage") == R::Ghost);
	CHECK(Pc::TreeNodeReveal(D, P, "grand_finale") == R::Hidden); // under the Magician, under the Aura
	CHECK(Pc::TreeNodeReveal(D, P, "no_such_skill") == R::Hidden);
	// Buying the Aura brings its ring into full view and the next ring into sight.
	P.BranchLevels["unlock_aura"] = 1;
	P.ReawakenCount = 5;
	CHECK(Pc::TreeNodeReveal(D, P, "aura_damage") == R::Full);
	CHECK(Pc::TreeNodeReveal(D, P, "job_magician") == R::Full);
	CHECK(Pc::TreeNodeReveal(D, P, "grand_finale") == R::Ghost);
	// The pad's picks keep to the set it is given.
	const Pc::PcTreeLayout L = Pc::LayOutTree(D);
	std::set<std::string> Shown;
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		if (Pc::TreeNodeReveal(D, P, B.Id) != R::Hidden)
		{
			Shown.insert(B.Id);
		}
	}
	const Pc::PcVec2 Dirs[4] = { { 0.0, -1.0 }, { 0.0, 1.0 }, { -1.0, 0.0 }, { 1.0, 0.0 } };
	int Picks = 0;
	for (const std::string& From : Shown)
	{
		for (const Pc::PcVec2& Dir : Dirs)
		{
			const std::string To = Pc::TreeNavPick(L, From, Dir, &Shown);
			Picks += To.empty() ? 0 : 1;
			CHECK((To.empty() || Shown.count(To) == 1));
		}
	}
	CHECK(Picks > 0);
}

TEST_CASE("The shipping tree: every node reachable by the pad, inside its canvas")
{
	const Pc::PcGameData& D = Game();
	const Pc::PcTreeLayout L = Pc::LayOutTree(D);
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
	CHECK(Trunk == "claws"); // the Compass: straight up first (C1, 2026-11-26; the first try's wheel began with Instinct)
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
	REQUIRE(Links.size() == Pc::BranchOrder(D).size()); // one per skill on the tree (a retired one has none: D-e)
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
	// Crafted, because no grid seat is ever this close (LayOutTree never runs it).
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


TEST_CASE("Tree nav: the nearest in the cone, the angle's cost, the wide cone when the cone is empty, only Among")
{
	// Hand-made, so the right answer is known. A at the origin; B straight right 150 away; C 143 away but 25 degrees off
	// right: its angle costs it (143 x (1 + 25 / 60) = 202 > 150).
	Pc::PcTreeLayout L;
	L.Order = { "a", "b", "c" };
	L.Nodes["a"] = { 1000.0, 1000.0 };
	L.Nodes["b"] = { 1150.0, 1000.0 };
	L.Nodes["c"] = { 1130.0, 1060.0 };
	const Pc::PcVec2 Right{ 1.0, 0.0 }, Up{ 0.0, -1.0 }, Left{ -1.0, 0.0 };
	CHECK(Pc::TreeNavPick(L, "a", Right) == "b");       // straight beats nearer but off to the side
	CHECK(Pc::TreeNavPick(L, "a", Left).empty());       // nothing that way: stay put
	CHECK(Pc::TreeNavPick(L, "nobody", Right).empty());
	const std::set<std::string> NoB{ "a", "c" };
	CHECK(Pc::TreeNavPick(L, "a", Right, &NoB) == "c"); // B hidden: the next best
	// D 73 degrees off up: outside the 60 degree cone, inside the 89 degree wide one.
	Pc::PcTreeLayout W;
	W.Order = { "a", "d" };
	W.Nodes["a"] = { 1000.0, 1000.0 };
	W.Nodes["d"] = { 1100.0, 970.0 };
	CHECK(Pc::TreeNavPick(W, "a", Up) == "d");          // nothing in the cone: the wide cone finds D
	const std::set<std::string> Alone{ "a" };
	CHECK(Pc::TreeNavPick(W, "a", Up, &Alone).empty()); // D hidden: nothing up
}

TEST_CASE("The branch names: one per trunk shown, past its outermost shown skill, clear of every skill; the shown bounds")
{
	const Pc::PcGameData& D = Game();
	const Pc::PcTreeLayout L = Pc::LayOutTree(D);
	const double P = Pc::TreeLayout::GridPitch;
	// The whole tree: a name per trunk, further out than all its skills, nowhere near a skill or the hub.
	const std::vector<Pc::PcTreeLabel> All = Pc::TreeBranchLabels(D, L);
	REQUIRE(All.size() == Pc::TrunkOrder(D).size());
	for (const Pc::PcTreeLabel& N : All)
	{
		INFO(N.Trunk);
		const double LR = std::hypot(N.At.X - L.Hub.X, N.At.Y - L.Hub.Y);
		double Nearest = 1e18;
		for (const auto& KV : L.Nodes)
		{
			const double R = std::hypot(KV.second.X - L.Hub.X, KV.second.Y - L.Hub.Y);
			if (Pc::TreeTrunkOf(D, KV.first) == N.Trunk)
			{
				CHECK(R < LR);
			}
			Nearest = std::min(Nearest, std::hypot(KV.second.X - N.At.X, KV.second.Y - N.At.Y));
		}
		CHECK(Nearest >= 0.6 * P);
	}
	CHECK(Pc::TreeTrunkOf(D, "grand_finale") == "unlock_aura");
	CHECK(Pc::TreeTrunkOf(D, "claws") == "claws");
	CHECK(Pc::TreeTrunkOf(D, "no_such_skill").empty());
	// A fresh game: what is shown is a small part of the tree; the bounds and the names follow it.
	Pc::PcProgress Fresh;
	std::set<std::string> Shown;
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		if (Pc::TreeNodeReveal(D, Fresh, B.Id) != Pc::PcTreeReveal::Hidden)
		{
			Shown.insert(B.Id);
		}
	}
	REQUIRE(Shown.size() < D.Branches.size());
	const Pc::PcRect2 Whole = Pc::TreeBounds(L), Part = Pc::TreeBounds(L, &Shown);
	CHECK(Part.W < Whole.W);
	CHECK(Part.H <= Whole.H);
	CHECK(Part.X <= L.Hub.X - Pc::TreeLayout::HubSize * 0.5);
	const std::set<std::string> None;
	const Pc::PcRect2 HubOnly = Pc::TreeBounds(L, &None);
	CHECK(HubOnly.W == doctest::Approx(Pc::TreeLayout::HubSize));
	const std::vector<Pc::PcTreeLabel> Early = Pc::TreeBranchLabels(D, L, &Shown);
	CHECK(Early.size() == Pc::TrunkOrder(D).size()); // every trunk shows (in full or faintly) from the start
	for (size_t I = 0; I < Early.size(); ++I)
	{
		CHECK(std::hypot(Early[I].At.X - L.Hub.X, Early[I].At.Y - L.Hub.Y) <= std::hypot(All[I].At.X - L.Hub.X, All[I].At.Y - L.Hub.Y) + 1e-6);
	}
	CHECK(Pc::TreeBranchLabels(D, L, &None).empty());
}

#endif
