#include "PcTreeLayout.h"

#include "PcTree.h"

#include <algorithm>
#include <cmath>
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

		// Godot's CMP_EPSILON as a real_t, and the 32-bit Math::is_equal_approx.
		const double CmpEpsilon = R32(0.00001);
		bool Approx32(double A, double B)
		{
			if (A == B)
			{
				return true;
			}
			double Tolerance = R32(CmpEpsilon * std::fabs(A));
			if (Tolerance < CmpEpsilon)
			{
				Tolerance = CmpEpsilon;
			}
			return std::fabs(R32(A - B)) < Tolerance;
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
		double Angle(PcVec2 A) { return R32(std::atan2(A.Y, A.X)); }
		double AngleTo(PcVec2 A, PcVec2 To) { return R32(std::atan2(Cross(A, To), Dot(A, To))); }
		PcVec2 Rotated(PcVec2 A, double By)
		{
			const double B = R32(By);
			const double S = R32(std::sin(B)), C = R32(std::cos(B));
			return { R32(R32(A.X * C) - R32(A.Y * S)), R32(R32(A.X * S) + R32(A.Y * C)) };
		}
		PcVec2 FromAngle(double A) { const double B = R32(A); return { R32(std::cos(B)), R32(std::sin(B)) }; }
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
		bool Approx32(PcVec2 A, PcVec2 B) { return Approx32(A.X, B.X) && Approx32(A.Y, B.Y); }

		// Geometry2D::get_closest_point_to_segment.
		PcVec2 ClosestOnSegment(PcVec2 Point, PcVec2 A, PcVec2 B)
		{
			const PcVec2 P = Sub(Point, A);
			const PcVec2 N = Sub(B, A);
			const double L2 = LengthSq(N);
			if (L2 < R32(1e-20))
			{
				return A;
			}
			const double D = R32(Dot(N, P) / L2);
			if (D <= 0.0)
			{
				return A;
			}
			if (D >= 1.0)
			{
				return B;
			}
			return Add(A, Mul(N, D));
		}

		// Geometry2D::segment_intersects_segment: parallel and colinear segments never intersect.
		bool SegmentsIntersect(PcVec2 FromA, PcVec2 ToA, PcVec2 FromB, PcVec2 ToB)
		{
			const PcVec2 B = Sub(ToA, FromA);
			PcVec2 C = Sub(FromB, FromA);
			PcVec2 D = Sub(ToB, FromA);
			const double ABLen = Dot(B, B);
			if (ABLen <= 0.0)
			{
				return false;
			}
			const PcVec2 Bn = Div(B, ABLen);
			C = { R32(R32(C.X * Bn.X) + R32(C.Y * Bn.Y)), R32(R32(C.Y * Bn.X) - R32(C.X * Bn.Y)) };
			D = { R32(R32(D.X * Bn.X) + R32(D.Y * Bn.Y)), R32(R32(D.Y * Bn.X) - R32(D.X * Bn.Y)) };
			if ((C.Y < -CmpEpsilon && D.Y < -CmpEpsilon) || (C.Y > CmpEpsilon && D.Y > CmpEpsilon))
			{
				return false;
			}
			if (Approx32(C.Y, D.Y))
			{
				return false;
			}
			const double ABPos = R32(D.X + R32(R32(R32(C.X - D.X) * D.Y) / R32(D.Y - C.Y)));
			return !(ABPos < 0.0 || ABPos > 1.0);
		}

		const PcVec2 CanvasCentre = Div(V(CanvasW, CanvasH), 2.0); // CANVAS_SIZE / 2.0
		const double NodeSizeX = R32(NodeSize);                 // NODE_SIZE.x
		const double LinkClear = NodeSizeX * 1.15;              // LINK_CLEARANCE: NODE_SIZE.x * 1.15, a GDScript double

		struct Cell
		{
			int Q = 0;
			int R = 0;
			bool operator==(const Cell& O) const { return Q == O.Q && R == O.R; }
			bool operator<(const Cell& O) const { return Q != O.Q ? Q < O.Q : R < O.R; }
		};

		// _hex_to_px: computed in GDScript doubles, stored in a Vector2.
		PcVec2 HexToPx(Cell C)
		{
			return V(HexPitch * (static_cast<double>(C.Q) + static_cast<double>(C.R) * 0.5),
				HexPitch * std::sqrt(3.0) * 0.5 * static_cast<double>(C.R));
		}

		// The cells exactly Ring steps from Centre, in Godot's fixed order.
		std::vector<Cell> HexRing(Cell Centre, int Ring)
		{
			std::vector<Cell> Out;
			for (int DQ = -Ring; DQ <= Ring; ++DQ)
			{
				for (int DR = -Ring; DR <= Ring; ++DR)
				{
					if (std::max(std::max(std::abs(DQ), std::abs(DR)), std::abs(-DQ - DR)) != Ring)
					{
						continue;
					}
					Out.push_back({ Centre.Q + DQ, Centre.R + DR });
				}
			}
			return Out;
		}

		const Cell ConeOrigin{ ConeOriginQ, ConeOriginR };

		struct Taken
		{
			std::map<Cell, std::string> Cells; // "" = reserved (the hub's line behind the trunk)
			bool Has(Cell C) const { return Cells.count(C) != 0; }
		};

		int FreeNeighbours(Cell C, const Taken& T)
		{
			const PcVec2 Origin = HexToPx(ConeOrigin);
			int N = 0;
			for (const Cell& Near : HexRing(C, 1))
			{
				if (!T.Has(Near) && std::fabs(Angle(Sub(HexToPx(Near), Origin))) <= Rad(ConeHalfDeg))
				{
					++N;
				}
			}
			return N;
		}

		int Crossings(Cell From, Cell To, const Taken& T)
		{
			const PcVec2 A = HexToPx(From), B = HexToPx(To);
			int N = 0;
			for (const auto& KV : T.Cells)
			{
				if (KV.first == From || KV.first == To)
				{
					continue;
				}
				const PcVec2 C = HexToPx(KV.first);
				if (DistanceTo(ClosestOnSegment(C, A, B), C) < LinkClear)
				{
					++N;
				}
			}
			return N;
		}

		bool CutsALink(Cell From, Cell To, const std::vector<std::pair<Cell, Cell>>& Links)
		{
			const PcVec2 A = HexToPx(From), B = HexToPx(To);
			for (const auto& L : Links)
			{
				if (L.first == From || L.second == From || L.first == To || L.second == To)
				{
					continue;
				}
				if (SegmentsIntersect(A, B, HexToPx(L.first), HexToPx(L.second)))
				{
					return true;
				}
			}
			return false;
		}

		// _seat(): the free cell a child takes. The score is GDScript arithmetic (double) on Vector2 results (32-bit).
		Cell Seat(Cell Parent, const Taken& T, const std::vector<std::pair<Cell, Cell>>& Links, int Kids)
		{
			const PcVec2 ParentPx = HexToPx(Parent);
			const PcVec2 Origin = HexToPx(ConeOrigin);
			Cell Best = Parent, Fallback = Parent;
			double BestScore = Inf, FallbackScore = Inf;
			for (int Ring = 1; Ring <= HexReach; ++Ring)
			{
				for (const Cell& C : HexRing(Parent, Ring))
				{
					if (T.Has(C))
					{
						continue;
					}
					const PcVec2 Px = HexToPx(C);
					const double Stray = std::fabs(Angle(Sub(Px, Origin)));
					double Score = DistanceTo(Px, ParentPx);
					Score += ConePenalty * Stray;
					Score += BackwardPenalty * std::max(ParentPx.X - Px.X, 0.0);
					Score += CrowdingPenalty * static_cast<double>(std::max(Kids - FreeNeighbours(C, T), 0));
					const bool bCrosses = Crossings(Parent, C, T) > 0 || CutsALink(Parent, C, Links);
					if (Stray > Rad(ConeHalfDeg) || bCrosses)
					{
						const double Penalised = Score + (bCrosses ? 100000.0 : 0.0);
						if (Penalised < FallbackScore)
						{
							FallbackScore = Penalised;
							Fallback = C;
						}
						continue;
					}
					if (Score < BestScore)
					{
						BestScore = Score;
						Best = C;
					}
				}
				if (BestScore < Inf && Ring >= 2)
				{
					break;
				}
			}
			return BestScore < Inf ? Best : Fallback;
		}

		// Children of each parent, in branch order (_children_by_parent).
		std::map<std::string, std::vector<std::string>> ChildrenByParent(const PcGameData& D, const std::vector<std::string>& Order)
		{
			std::map<std::string, std::vector<std::string>> Out;
			for (const std::string& Id : Order)
			{
				const PcSkillBranch* B = D.FindBranch(Id);
				if (B && !B->Parent.empty())
				{
					Out[B->Parent].push_back(Id);
				}
			}
			return Out;
		}

		// _lay_out_limb(): one trunk's subtree on its own lattice; id -> cell in seating order.
		std::vector<std::pair<std::string, Cell>> LayOutLimb(const std::string& Trunk, const std::map<std::string, std::vector<std::string>>& Children)
		{
			std::vector<std::pair<std::string, Cell>> CellOf{ { Trunk, Cell{ 0, 0 } } };
			std::map<std::string, Cell> Lookup{ { Trunk, Cell{ 0, 0 } } };
			Taken T;
			T.Cells[Cell{ 0, 0 }] = Trunk;
			for (int Back = 1; Back <= HexReach; ++Back)
			{
				T.Cells[Cell{ -Back, 0 }] = "";
			}
			std::vector<std::pair<Cell, Cell>> Links{ { ConeOrigin, Cell{ 0, 0 } } };
			std::vector<std::string> Queue{ Trunk };
			for (size_t Head = 0; Head < Queue.size(); ++Head)
			{
				const std::string Id = Queue[Head];
				const auto Kids = Children.find(Id);
				if (Kids == Children.end())
				{
					continue;
				}
				for (const std::string& Kid : Kids->second)
				{
					const auto Grand = Children.find(Kid);
					const int GrandKids = Grand == Children.end() ? 0 : static_cast<int>(Grand->second.size());
					const Cell C = Seat(Lookup[Id], T, Links, GrandKids);
					T.Cells[C] = Kid;
					Lookup[Kid] = C;
					CellOf.push_back({ Kid, C });
					Links.push_back({ Lookup[Id], C });
					Queue.push_back(Kid);
				}
			}
			return CellOf;
		}

		struct Segment
		{
			PcVec2 A, B;
		};

		bool ShareAnEnd(const Segment& S, const Segment& O)
		{
			return Approx32(S.A, O.A) || Approx32(S.A, O.B) || Approx32(S.B, O.A) || Approx32(S.B, O.B);
		}

		using Placed = std::vector<std::pair<std::string, PcVec2>>;

		bool LimbIsClear(const Placed& Limb, const std::vector<Segment>& Links, const Placed& Done, const std::vector<Segment>& DoneLinks)
		{
			for (const auto& N : Limb)
			{
				for (const auto& O : Done)
				{
					const double Need = O.first == "hub" ? HexPitch : LimbGapPx;
					if (DistanceTo(N.second, O.second) < Need - 0.5)
					{
						return false;
					}
				}
			}
			for (const Segment& L : Links)
			{
				for (const auto& O : Done)
				{
					const PcVec2 C = O.second;
					if (Approx32(C, L.A) || Approx32(C, L.B))
					{
						continue;
					}
					if (DistanceTo(ClosestOnSegment(C, L.A, L.B), C) < LinkClear)
					{
						return false;
					}
				}
				for (const Segment& S : DoneLinks)
				{
					if (ShareAnEnd(L, S))
					{
						continue;
					}
					if (SegmentsIntersect(L.A, L.B, S.A, S.B))
					{
						return false;
					}
				}
			}
			return true;
		}

		const PcVec2* Find(const Placed& P, const std::string& Id)
		{
			for (const auto& KV : P)
			{
				if (KV.first == Id)
				{
					return &KV.second;
				}
			}
			return nullptr;
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
				return { CanvasCentre.X, CanvasCentre.Y, 0.0, 0.0 };
			}
			MinP = Min(MinP, Sub(Hub, HubHalf));
			MaxP = Max(MaxP, Add(Hub, HubHalf));
			const PcVec2 Size = Sub(MaxP, MinP);
			return { MinP.X, MinP.Y, Size.X, Size.Y };
		}
	}

	const std::vector<std::string>& TrunkWheel()
	{
		static const std::vector<std::string> Wheel{ "instinct", "claws", "unlock_aura", "fortune", "unlock_unicorn", "unlock_snare" };
		return Wheel;
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
			Names.push_back(B.Id);
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
			if (!Seen.count(B.Id))
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
		const auto Children = ChildrenByParent(D, L.Order);
		const std::vector<std::string> Trunks = TrunkOrder(D);

		// _compute_layout(): every limb about the hub at (0, 0).
		Placed Done{ { "hub", PcVec2{} } };
		std::vector<Segment> DoneLinks;
		for (size_t I = 0; I < Trunks.size(); ++I)
		{
			const std::string& Trunk = Trunks[I];
			const double Axis = -Pi / 2.0 + Tau * static_cast<double>(I) / static_cast<double>(Trunks.size()); // GDScript double
			const PcVec2 Dir = FromAngle(Axis);
			const auto Local = LayOutLimb(Trunk, Children);
			Placed Limb;
			std::vector<Segment> Links;
			const double Start = static_cast<double>(-ConeOriginQ);
			for (int Step = 0; Step <= LimbPushMax * 2; ++Step)
			{
				// dir * HEX_PITCH * (start + step * 0.5): two Vector2 x scalar products, left to right.
				const PcVec2 Offset = Mul(Mul(Dir, HexPitch), Start + static_cast<double>(Step) * 0.5);
				Limb.clear();
				for (const auto& KV : Local)
				{
					Limb.push_back({ KV.first, Add(Rotated(HexToPx(KV.second), Axis), Offset) });
				}
				Links.clear();
				Links.push_back({ PcVec2{}, *Find(Limb, Trunk) });
				for (const auto& KV : Local)
				{
					const PcSkillBranch* B = D.FindBranch(KV.first);
					if (B && !B->Parent.empty())
					{
						const PcVec2* From = Find(Limb, B->Parent);
						if (From)
						{
							Links.push_back({ *From, *Find(Limb, KV.first) });
						}
					}
				}
				if (LimbIsClear(Limb, Links, Done, DoneLinks))
				{
					break;
				}
				if (Step == LimbPushMax * 2)
				{
					L.UnclearLimbs.push_back(Trunk);
				}
			}
			Done.insert(Done.end(), Limb.begin(), Limb.end());
			DoneLinks.insert(DoneLinks.end(), Links.begin(), Links.end());
		}
		L.Hub = CanvasCentre;
		for (const auto& KV : Done)
		{
			if (KV.first != "hub")
			{
				L.Nodes[KV.first] = Add(CanvasCentre, KV.second);
			}
		}

		SeparateTreeNodes(L);

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

	PcRect2 TreeBounds(const PcTreeLayout& L)
	{
		return BoundsOf(L.Order, L.Nodes, L.Hub);
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

	std::string TreeNavPick(const PcTreeLayout& L, const std::string& FromId, const PcVec2& Direction)
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
				if (Id == FromId || It == L.Nodes.end())
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
