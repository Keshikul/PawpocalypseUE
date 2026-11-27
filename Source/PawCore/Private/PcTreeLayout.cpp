#include "PcTreeLayout.h"

#include "PcTree.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <utility>

// GODOT'S VECTOR2 IS 32-BIT (real_t = float in the shipping build), and this layout is decided at its rounding: the hex
// lattice puts neighbours at EXACTLY 60 degrees, which is the navigation cone's edge, and seat scores tie between cells to
// the last bit. Ported in plain double, the pad's picks came out different on the shipping tree (32 of 348) and 147 random
// seats moved (measured 2026-11-02 against the fixture). So every Vector2 operation below is done as Godot does it: each
// arithmetic result rounded to the nearest 32-bit value (R32: correctly rounded, which a single IEEE float operation is),
// while what GDScript computes on plain floats (scores, sums of lengths, the axis angle) stays in double, as it does there.
// PawCore forbids the `float` type (L0), so the rounding is done on the bits of a double with frexp / ldexp.

namespace Pc
{
	namespace
	{
		using namespace TreeLayout;

		constexpr double Pi = 3.14159265358979323846;
		constexpr double Tau = 2.0 * Pi;
		constexpr double Inf = std::numeric_limits<double>::infinity();

		// The nearest 32-bit value (round half to even), for the normal range this layout lives in; zero, infinities and
		// values below the 32-bit normal range keep their exact handling.
		double R32(double X)
		{
			if (X == 0.0 || !std::isfinite(X))
			{
				return X;
			}
			int E = 0;
			const double M = std::frexp(X, &E); // |M| in [0.5, 1)
			if (E < -125)
			{
				return std::ldexp(std::nearbyint(std::ldexp(X, 149)), -149); // subnormal: a fixed 2^-149 step
			}
			return std::ldexp(std::nearbyint(std::ldexp(M, 24)), E - 24);
		}

		double Rad(double Deg) { return Deg * Pi / 180.0; } // deg_to_rad: GDScript double

		// --- Vector2, 32-bit ---
		PcVec2 V(double X, double Y) { return { R32(X), R32(Y) }; } // Vector2(x, y): stored as real_t
		PcVec2 Add(PcVec2 A, PcVec2 B) { return { R32(A.X + B.X), R32(A.Y + B.Y) }; }
		PcVec2 Sub(PcVec2 A, PcVec2 B) { return { R32(A.X - B.X), R32(A.Y - B.Y) }; }
		PcVec2 Mul(PcVec2 A, double S) { const double S32 = R32(S); return { R32(A.X * S32), R32(A.Y * S32) }; }
		PcVec2 Div(PcVec2 A, double S) { const double S32 = R32(S); return { R32(A.X / S32), R32(A.Y / S32) }; }
		double Dot(PcVec2 A, PcVec2 B) { return R32(R32(A.X * B.X) + R32(A.Y * B.Y)); }
		double Cross(PcVec2 A, PcVec2 B) { return R32(R32(A.X * B.Y) - R32(A.Y * B.X)); }
		double LengthSq(PcVec2 A) { return R32(R32(A.X * A.X) + R32(A.Y * A.Y)); }
		double Length(PcVec2 A) { return R32(std::sqrt(LengthSq(A))); }
		double DistanceTo(PcVec2 A, PcVec2 B) { return Length(Sub(A, B)); }
		double AngleTo(PcVec2 A, PcVec2 To) { return R32(std::atan2(Cross(A, To), Dot(A, To))); }
		PcVec2 Normalized(PcVec2 A)
		{
			double L = LengthSq(A);
			if (L == 0.0)
			{
				return A;
			}
			L = R32(std::sqrt(L));
			return { R32(A.X / L), R32(A.Y / L) };
		}
		PcVec2 Min(PcVec2 A, PcVec2 B) { return { std::min(A.X, B.X), std::min(A.Y, B.Y) }; }
		PcVec2 Max(PcVec2 A, PcVec2 B) { return { std::max(A.X, B.X), std::max(A.Y, B.Y) }; }

		const PcVec2 CanvasCentre = Div(V(CanvasW, CanvasH), 2.0); // CANVAS_SIZE / 2.0
		// --- the Compass grid (C1, designer 2026-11-26: "The compass is better but can you space it out a little") ---
		// Integer squares around the root at (0, 0). Every rule below is decided on whole numbers or on doubles computed the
		// same way every time, so the layout is the same on every machine (the concept page ran the same steps in JavaScript).
		struct GridCell
		{
			int X = 0;
			int Y = 0;
			bool operator<(const GridCell& O) const { return X != O.X ? X < O.X : Y < O.Y; }
			bool operator==(const GridCell& O) const { return X == O.X && Y == O.Y; }
		};

		// Halves round up (toward +infinity), as the concept's JavaScript Math.round does: -3.5 is -3, not -4.
		int RoundHalfUp(double X) { return static_cast<int>(std::floor(X + 0.5)); }

		int Orient(GridCell P, GridCell Q, GridCell R)
		{
			const long long V2 = static_cast<long long>(Q.X - P.X) * (R.Y - P.Y) - static_cast<long long>(Q.Y - P.Y) * (R.X - P.X);
			return V2 > 0 ? 1 : (V2 < 0 ? -1 : 0);
		}

		// Two links cross when each one's ends lie strictly on both sides of the other; links sharing an end never cross.
		bool LinksCross(GridCell A, GridCell B, GridCell C, GridCell D)
		{
			if (A == C || A == D || B == C || B == D)
			{
				return false;
			}
			return Orient(A, B, C) * Orient(A, B, D) < 0 && Orient(C, D, A) * Orient(C, D, B) < 0;
		}

		// The distance from square C to the link A-B (in squares).
		double SquareToLink(GridCell C, GridCell A, GridCell B)
		{
			const double VX = B.X - A.X, VY = B.Y - A.Y, L2 = VX * VX + VY * VY;
			const double T = L2 > 0.0 ? std::clamp(((C.X - A.X) * VX + (C.Y - A.Y) * VY) / L2, 0.0, 1.0) : 0.0;
			return std::hypot(A.X + VX * T - C.X, A.Y + VY * T - C.Y);
		}

		// The angle and radius of a square seen from the root, x shrunk by the ellipse's stretch (the screen is wide).
		double GridAngle(double X, double Y) { return std::atan2(Y, X / GridStretch); }
		double GridRadius(double X, double Y) { return std::hypot(X / GridStretch, Y); }

		// Each trunk's slice of the compass: TrunkWheel's order clockwise from straight up, equal slices.
		struct Slice
		{
			double Lo = 0.0, Hi = 0.0, Mid = 0.0; // degrees
		};

		bool InSlice(const Slice& S, double X, double Y)
		{
			double A = GridAngle(X, Y) * 180.0 / Pi;
			while (A < S.Lo - 180.0)
			{
				A += 360.0;
			}
			while (A > S.Lo + 180.0)
			{
				A -= 360.0;
			}
			return A >= S.Lo && A <= S.Hi;
		}

		struct GridPlacement
		{
			std::map<std::string, GridCell> Cells;
			std::vector<std::string> Unseated; // nothing clear within reach: placed anyway, reported (a bug)
		};

		GridPlacement PlaceOnGrid(const PcGameData& D, const std::vector<std::string>& Trunks)
		{
			GridPlacement Out;
			std::map<std::string, std::string> TrunkOf;
			std::map<std::string, int> DepthOf;
			std::map<std::string, std::vector<std::string>> Kids;
			for (const PcSkillBranch& B : D.Branches)
			{
				if (!B.Parent.empty() && !IsBranchRetired(D, B.Id))
				{
					Kids[B.Parent].push_back(B.Id);
				}
			}
			for (const PcSkillBranch& B : D.Branches)
			{
				if (IsBranchRetired(D, B.Id))
				{
					continue; // not laid out (D-e)
				}
				const PcSkillBranch* T = &B;
				int Depth = 0;
				while (T && !T->Parent.empty() && Depth < 64)
				{
					const PcSkillBranch* Up = D.FindBranch(T->Parent);
					if (!Up)
					{
						break;
					}
					T = Up;
					++Depth;
				}
				TrunkOf[B.Id] = T ? T->Id : B.Id;
				DepthOf[B.Id] = Depth;
			}
			std::map<std::string, int> SizeOf;
			std::function<int(const std::string&)> Size = [&](const std::string& Id)
			{
				const auto It = SizeOf.find(Id);
				if (It != SizeOf.end())
				{
					return It->second;
				}
				int N = 1;
				const auto K = Kids.find(Id);
				if (K != Kids.end())
				{
					for (const std::string& C : K->second)
					{
						N += Size(C);
					}
				}
				return SizeOf[Id] = N;
			};
			// The slices, and each trunk's own square on them.
			const double Span = 360.0 / static_cast<double>(std::max<size_t>(Trunks.size(), 1));
			std::map<std::string, Slice> Slices;
			std::map<std::string, GridCell> TrunkAt;
			for (size_t I = 0; I < Trunks.size(); ++I)
			{
				Slice S;
				S.Lo = -90.0 - Span * 0.5 + Span * static_cast<double>(I);
				S.Hi = S.Lo + Span;
				S.Mid = (S.Lo + S.Hi) * 0.5;
				Slices[Trunks[I]] = S;
				const double A = S.Mid * Pi / 180.0;
				TrunkAt[Trunks[I]] = { RoundHalfUp(std::cos(A) * GridTrunkRadius * GridStretch), RoundHalfUp(std::sin(A) * GridTrunkRadius) };
			}
			// Inner rings first; at one depth, the trunks in PlaceOrder; the bigger subtree first; then by id.
			std::vector<std::string> Order;
			for (const PcSkillBranch& B : D.Branches)
			{
				if (!IsBranchRetired(D, B.Id))
				{
					Order.push_back(B.Id);
				}
			}
			const std::vector<std::string>& Pref = GridPlaceOrder();
			auto Rank = [&](const std::string& Trunk)
			{
				const auto It = std::find(Pref.begin(), Pref.end(), Trunk);
				return It == Pref.end() ? static_cast<long long>(Pref.size()) + static_cast<long long>(std::find(Trunks.begin(), Trunks.end(), Trunk) - Trunks.begin())
					: static_cast<long long>(It - Pref.begin());
			};
			std::sort(Order.begin(), Order.end(), [&](const std::string& A, const std::string& B)
			{
				if (DepthOf[A] != DepthOf[B]) return DepthOf[A] < DepthOf[B];
				if (Rank(TrunkOf[A]) != Rank(TrunkOf[B])) return Rank(TrunkOf[A]) < Rank(TrunkOf[B]);
				if (Size(A) != Size(B)) return Size(A) > Size(B);
				return A < B;
			});

			std::map<GridCell, std::string> Used{ { GridCell{ 0, 0 }, std::string() } }; // "" = the root
			std::vector<std::pair<GridCell, GridCell>> Links;
			for (const std::string& Id : Order)
			{
				const PcSkillBranch* B = D.FindBranch(Id);
				const std::string& Trunk = TrunkOf[Id];
				const auto SIt = Slices.find(Trunk);
				const Slice S = SIt == Slices.end() ? Slice{} : SIt->second;
				const bool bTrunk = B->Parent.empty() || !Out.Cells.count(B->Parent);
				const GridCell From = bTrunk ? GridCell{ 0, 0 } : Out.Cells[B->Parent];
				GridCell Best{};
				bool bFound = false;
				if (bTrunk && TrunkAt.count(Id) && !Used.count(TrunkAt[Id]))
				{
					Best = TrunkAt[Id];
					bFound = true;
				}
				else
				{
					const double Want = bTrunk ? GridTrunkRadius : GridRadius(From.X, From.Y) + GridRingStep;
					const double MidRad = S.Mid * Pi / 180.0;
					double BestScore = -Inf;
					for (const int Reach : { 3, 5, 8 })
					{
						for (int DX = -Reach; DX <= Reach; ++DX)
						{
							for (int DY = -Reach; DY <= Reach; ++DY)
							{
								if (!DX && !DY)
								{
									continue;
								}
								const GridCell To{ From.X + DX, From.Y + DY };
								if (Used.count(To))
								{
									continue;
								}
								const double R = GridRadius(To.X, To.Y);
								if (R < GridMinRadius)
								{
									continue;
								}
								bool bClear = true;
								for (const auto& L : Links)
								{
									if (LinksCross(L.first, L.second, From, To))
									{
										bClear = false;
										break;
									}
								}
								for (auto U = Used.begin(); bClear && U != Used.end(); ++U)
								{
									if (!(U->first == From) && !(U->first == To) && SquareToLink(U->first, From, To) < GridLinkClearance)
									{
										bClear = false; // the new link would pass over a skill
									}
								}
								for (size_t K = 0; bClear && K < Links.size(); ++K)
								{
									const auto& L = Links[K];
									if (!(L.first == To) && !(L.second == To) && SquareToLink(To, L.first, L.second) < GridLinkClearance)
									{
										bClear = false; // the new skill would sit on a link already drawn
									}
								}
								if (!bClear)
								{
									continue;
								}
								const double Len = std::hypot(static_cast<double>(DX), static_cast<double>(DY));
								const double Clean = (DX == 0 || DY == 0 || std::abs(DX) == std::abs(DY)) ? 0.0 : 1.0;
								double DA = std::fabs(GridAngle(To.X, To.Y) - MidRad);
								if (DA > Pi)
								{
									DA = 2.0 * Pi - DA;
								}
								int Crowd = 0;
								for (int AX = -1; AX <= 1; ++AX)
								{
									for (int AY = -1; AY <= 1; ++AY)
									{
										const auto O = Used.find(GridCell{ To.X + AX, To.Y + AY });
										if (O != Used.end() && !O->second.empty() && TrunkOf[O->second] != Trunk)
										{
											++Crowd;
										}
									}
								}
								const double Outside = InSlice(S, To.X, To.Y) ? 0.0 : 1.0;
								const double Score = -std::fabs(R - Want) * GridRingWeight - (Len - 1.0) * 1.1 - Clean * 0.9
									- DA * GridCentreWeight - Outside * GridOutsideWeight - static_cast<double>(Crowd) * 1.5;
								if (Score > BestScore)
								{
									BestScore = Score;
									Best = To;
									bFound = true;
								}
							}
						}
						if (bFound)
						{
							break;
						}
					}
				}
				if (!bFound)
				{
					Out.Unseated.push_back(Id);
					const double MidRad = S.Mid * Pi / 180.0;
					Best = { From.X + RoundHalfUp(std::cos(MidRad) * 6.0 * GridStretch), From.Y + RoundHalfUp(std::sin(MidRad) * 6.0) };
				}
				Out.Cells[Id] = Best;
				Used[Best] = Id;
				Links.push_back({ From, Best });
			}
			return Out;
		}

		// _layout_bounds(): Vector2 min / max, a Rect2 of 32-bit parts.
		PcRect2 BoundsOf(const std::vector<std::string>& Order, const std::map<std::string, PcVec2>& Nodes, PcVec2 Hub)
		{
			const PcVec2 Half = Div(V(NodeSize, NodeSize), 2.0);
			const PcVec2 HubHalf = Div(V(HubSize, HubSize), 2.0);
			PcVec2 MinP{ Inf, Inf }, MaxP{ -Inf, -Inf };
			for (const std::string& Id : Order)
			{
				const auto It = Nodes.find(Id);
				if (It == Nodes.end())
				{
					continue;
				}
				MinP = Min(MinP, Sub(It->second, Half));
				MaxP = Max(MaxP, Add(Add(It->second, Half), V(0.0, CaptionDrop)));
			}
			if (MinP.X == Inf)
			{
				MinP = MaxP = Hub; // nothing shown: the hub alone (the first try: an empty rect at the canvas centre)
			}
			MinP = Min(MinP, Sub(Hub, HubHalf));
			MaxP = Max(MaxP, Add(Hub, HubHalf));
			const PcVec2 Size = Sub(MaxP, MinP);
			return { MinP.X, MinP.Y, Size.X, Size.Y };
		}
	}

	const std::vector<std::string>& TrunkWheel()
	{
		// The Compass (C1, 2026-11-26), clockwise from straight up.
		static const std::vector<std::string> Wheel{ "claws", "instinct", "unlock_aura", "unlock_unicorn", "fortune", "unlock_snare" };
		return Wheel;
	}

	const std::vector<std::string>& GridPlaceOrder()
	{
		// The order the trunks seat their skills in, at one depth (the concept page's, whose result was checked clean).
		static const std::vector<std::string> Order{ "claws", "instinct", "unlock_aura", "fortune", "unlock_snare", "unlock_unicorn" };
		return Order;
	}

	std::vector<std::string> TrunkOrder(const PcGameData& D)
	{
		std::vector<std::string> Out;
		for (const std::string& Id : TrunkWheel())
		{
			if (D.FindBranch(Id))
			{
				Out.push_back(Id);
			}
		}
		for (const std::string& Id : TrunkBranches(D))
		{
			if (std::find(Out.begin(), Out.end(), Id) == Out.end())
			{
				Out.push_back(Id);
			}
		}
		return Out;
	}

	std::vector<std::string> BranchOrder(const PcGameData& D)
	{
		// _children_of_all(): every parent's children, by sorted id.
		std::vector<std::string> Names;
		for (const PcSkillBranch& B : D.Branches)
		{
			if (!IsBranchRetired(D, B.Id))
			{
				Names.push_back(B.Id); // a retired skill is not on the tree (D-e)
			}
		}
		std::sort(Names.begin(), Names.end());
		std::map<std::string, std::vector<std::string>> Kids;
		for (const std::string& Id : Names)
		{
			const PcSkillBranch* B = D.FindBranch(Id);
			if (B && !B->Parent.empty())
			{
				Kids[B->Parent].push_back(Id);
			}
		}
		std::vector<std::string> Out;
		std::set<std::string> Seen;
		std::vector<std::string> Queue = TrunkOrder(D);
		for (size_t Head = 0; Head < Queue.size(); ++Head)
		{
			const std::string Id = Queue[Head];
			if (!Seen.insert(Id).second)
			{
				continue;
			}
			Out.push_back(Id);
			const auto It = Kids.find(Id);
			if (It != Kids.end())
			{
				Queue.insert(Queue.end(), It->second.begin(), It->second.end());
			}
		}
		for (const PcSkillBranch& B : D.Branches)
		{
			if (!Seen.count(B.Id) && !IsBranchRetired(D, B.Id))
			{
				Seen.insert(B.Id);
				Out.push_back(B.Id);
			}
		}
		return Out;
	}

	void SeparateTreeNodes(PcTreeLayout& L)
	{
		// _separate_layout(): the outer of two crowded nodes moves out along its own ray from the canvas centre.
		for (int Pass = 0; Pass < SeparationPasses; ++Pass)
		{
			bool bMoved = false;
			for (size_t I = 0; I < L.Order.size(); ++I)
			{
				const std::string& AId = L.Order[I];
				if (!L.Nodes.count(AId))
				{
					continue;
				}
				for (size_t J = I + 1; J < L.Order.size(); ++J)
				{
					const std::string& BId = L.Order[J];
					if (!L.Nodes.count(BId))
					{
						continue;
					}
					const PcVec2 A = L.Nodes[AId];
					const PcVec2 B = L.Nodes[BId];
					const double Gap = DistanceTo(A, B);
					if (Gap >= NodeGap)
					{
						continue;
					}
					const double AR = Length(Sub(A, CanvasCentre));
					const double BR = Length(Sub(B, CanvasCentre));
					const std::string& Mover = AR > BR ? AId : BId;
					PcVec2 Ray = Sub(L.Nodes[Mover], CanvasCentre);
					if (Length(Ray) < 0.001)
					{
						Ray = { 1.0, 0.0 };
					}
					// ray.normalized() * (ray.length() + (NODE_GAP - gap)): the scalar is GDScript double.
					L.Nodes[Mover] = Add(CanvasCentre, Mul(Normalized(Ray), Length(Ray) + (NodeGap - Gap)));
					bMoved = true;
				}
			}
			if (!bMoved)
			{
				break;
			}
		}
	}

	PcTreeLayout LayOutTree(const PcGameData& D)
	{
		PcTreeLayout L;
		L.Order = BranchOrder(D);
		const GridPlacement G = PlaceOnGrid(D, TrunkOrder(D));
		L.UnclearLimbs = G.Unseated;
		L.Hub = CanvasCentre;
		for (const auto& KV : G.Cells)
		{
			L.Nodes[KV.first] = PcVec2{ CanvasCentre.X + GridPitch * KV.second.X, CanvasCentre.Y + GridPitch * KV.second.Y };
		}

		// _normalise_layout(): the top left at FitMargin (never pulled back if it is already further in).
		const PcRect2 Extent = BoundsOf(L.Order, L.Nodes, L.Hub);
		PcVec2 Shift = Sub(V(FitMargin, FitMargin), PcVec2{ Extent.X, Extent.Y });
		if (!(Shift.X <= 0.0 && Shift.Y <= 0.0))
		{
			Shift = V(std::max(Shift.X, 0.0), std::max(Shift.Y, 0.0));
			L.Hub = Add(L.Hub, Shift);
			for (auto& KV : L.Nodes)
			{
				KV.second = Add(KV.second, Shift);
			}
		}
		return L;
	}

	PcRect2 TreeBounds(const PcTreeLayout& L, const std::set<std::string>* Among)
	{
		if (!Among)
		{
			return BoundsOf(L.Order, L.Nodes, L.Hub);
		}
		std::vector<std::string> Shown;
		for (const std::string& Id : L.Order)
		{
			if (Among->count(Id))
			{
				Shown.push_back(Id);
			}
		}
		return BoundsOf(Shown, L.Nodes, L.Hub);
	}

	std::string TreeTrunkOf(const PcGameData& D, const std::string& Id)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		for (size_t Guard = 0; B && Guard <= D.Branches.size(); ++Guard)
		{
			const PcSkillBranch* Up = B->Parent.empty() ? nullptr : D.FindBranch(B->Parent);
			if (!Up)
			{
				return B->Id;
			}
			B = Up;
		}
		return std::string();
	}

	std::vector<PcTreeLabel> TreeBranchLabels(const PcGameData& D, const PcTreeLayout& L, const std::set<std::string>* Among)
	{
		std::vector<PcTreeLabel> Out;
		for (const std::string& Trunk : TrunkOrder(D))
		{
			std::vector<PcVec2> Shown;
			for (const auto& KV : L.Nodes)
			{
				if ((!Among || Among->count(KV.first)) && TreeTrunkOf(D, KV.first) == Trunk)
				{
					Shown.push_back(PcVec2{ KV.second.X - L.Hub.X, KV.second.Y - L.Hub.Y });
				}
			}
			if (Shown.empty())
			{
				continue;
			}
			double CX = 0.0, CY = 0.0;
			for (const PcVec2& P : Shown)
			{
				CX += P.X;
				CY += P.Y;
			}
			const double Len = std::hypot(CX, CY);
			const double UX = Len > 0.0 ? CX / Len : 0.0, UY = Len > 0.0 ? CY / Len : -1.0;
			double Reach = -Inf;
			for (const PcVec2& P : Shown)
			{
				Reach = std::max(Reach, P.X * UX + P.Y * UY);
			}
			Reach += GridLabelReach * GridPitch;
			Out.push_back(PcTreeLabel{ Trunk, PcVec2{ L.Hub.X + UX * Reach, L.Hub.Y + UY * Reach } });
		}
		return Out;
	}

	PcVec2 TreeCanvasSize(const PcRect2& B)
	{
		// canvas_size_for(): maxf over Rect2 parts (extent.end = position + size, 32-bit), stored in a Vector2.
		return V(std::max(CanvasW, R32(B.X + B.W) + FitMargin), std::max(CanvasH, R32(B.Y + B.H) + FitMargin));
	}

	double TreeFitZoom(const PcRect2& B, double ViewW, double ViewH)
	{
		if (ViewW <= 0.0 || ViewH <= 0.0)
		{
			ViewW = 1920.0;
			ViewH = 950.0;
		}
		const double NeedW = B.W + FitMargin * 2.0, NeedH = B.H + FitMargin * 2.0;
		if (NeedW <= 0.0 || NeedH <= 0.0)
		{
			return FitMax;
		}
		return std::clamp(std::min(ViewW / NeedW, ViewH / NeedH), ZoomMin, FitMax);
	}

	std::string TreeNavPick(const PcTreeLayout& L, const std::string& FromId, const PcVec2& Direction, const std::set<std::string>* Among)
	{
		const auto FromIt = L.Nodes.find(FromId);
		if (FromIt == L.Nodes.end())
		{
			return std::string();
		}
		const PcVec2 From = FromIt->second;
		const PcVec2 Dir = V(Direction.X, Direction.Y);
		for (const double Cone : { Rad(NavConeDeg), Rad(NavConeWideDeg) })
		{
			std::string Best;
			double BestScore = Inf;
			for (const std::string& Id : L.Order)
			{
				const auto It = L.Nodes.find(Id);
				if (Id == FromId || It == L.Nodes.end() || (Among && !Among->count(Id)))
				{
					continue;
				}
				const PcVec2 D = Sub(It->second, From);
				if (Length(D) < 1.0)
				{
					continue;
				}
				const double A = std::fabs(AngleTo(D, Dir));
				if (A > Cone)
				{
					continue;
				}
				const double Score = Length(D) * (1.0 + NavAnglePenalty * A / Rad(NavConeDeg));
				if (Score < BestScore)
				{
					BestScore = Score;
					Best = Id;
				}
			}
			if (!Best.empty())
			{
				return Best;
			}
		}
		return std::string();
	}

	bool TreeCanBuyNow(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		return IsBranchAvailable(D, P, Id) && !IsBranchMaxLevel(D, P, Id) && CanAffordNextBranch(D, P, Id);
	}

	PcTreeNodeState TreeNodeState(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		if (BranchLevel(P, Id) > 0)
		{
			return PcTreeNodeState::Owned;
		}
		if (IsBranchMaxLevel(D, P, Id))
		{
			return PcTreeNodeState::Locked;
		}
		if (IsBranchAvailable(D, P, Id) && CanAffordNextBranch(D, P, Id))
		{
			return PcTreeNodeState::Ready;
		}
		return PcTreeNodeState::Locked;
	}

	std::string CheapestAffordableBranch(const PcGameData& D, const PcProgress& P)
	{
		std::string Best;
		double BestCost = Inf;
		for (const std::string& Id : BranchOrder(D))
		{
			if (!TreeCanBuyNow(D, P, Id))
			{
				continue;
			}
			const double Cost = NextBranchCost(D, P, Id);
			if (Cost < BestCost)
			{
				BestCost = Cost;
				Best = Id;
			}
		}
		return Best;
	}

	std::string NextAffordableBranch(const PcGameData& D, const PcProgress& P, const std::string& After)
	{
		const std::vector<std::string> Order = BranchOrder(D);
		if (Order.empty())
		{
			return std::string();
		}
		const auto It = std::find(Order.begin(), Order.end(), After);
		const long long Start = It == Order.end() ? -1 : static_cast<long long>(It - Order.begin());
		const long long N = static_cast<long long>(Order.size());
		for (long long K = 1; K <= N; ++K)
		{
			const std::string& Id = Order[static_cast<size_t>((Start + K) % N)];
			if (TreeCanBuyNow(D, P, Id))
			{
				return Id;
			}
		}
		return std::string();
	}

	bool HasAffordableBranch(const PcGameData& D, const PcProgress& P)
	{
		for (const PcSkillBranch& B : D.Branches)
		{
			if (TreeCanBuyNow(D, P, B.Id))
			{
				return true;
			}
		}
		return false;
	}

	PcTreeReveal TreeNodeReveal(const PcGameData& D, const PcProgress& P, const std::string& Id)
	{
		const PcSkillBranch* B = D.FindBranch(Id);
		if (!B || IsBranchRetired(D, Id))
		{
			return PcTreeReveal::Hidden;
		}
		if (BranchLevel(P, Id) > 0 || IsBranchAvailable(D, P, Id))
		{
			return PcTreeReveal::Full;
		}
		// One ring ahead: faint under a parent that shows in full (a trunk's parent is the hub, always there).
		if (B->Parent.empty() || !D.FindBranch(B->Parent))
		{
			return PcTreeReveal::Ghost;
		}
		const PcSkillBranch* Up = D.FindBranch(B->Parent);
		const bool bParentFull = BranchLevel(P, Up->Id) > 0 || IsBranchAvailable(D, P, Up->Id);
		return bParentFull ? PcTreeReveal::Ghost : PcTreeReveal::Hidden;
	}

	std::vector<PcTreeLink> TreeLinks(const PcGameData& D, const PcProgress& P)
	{
		std::vector<PcTreeLink> Out;
		for (const std::string& Id : BranchOrder(D))
		{
			const PcSkillBranch* B = D.FindBranch(Id);
			if (!B)
			{
				continue;
			}
			PcTreeLink L;
			L.From = B->Parent.empty() ? std::string("hub") : B->Parent;
			L.To = Id;
			L.bOwned = BranchLevel(P, Id) > 0 && (L.From == "hub" || BranchLevel(P, L.From) > 0);
			L.bShut = !BlockingRival(D, P, Id).empty();
			L.bFork = !B->ExclusiveWith.empty() && !L.bOwned && !L.bShut;
			Out.push_back(L);
		}
		return Out;
	}
}
