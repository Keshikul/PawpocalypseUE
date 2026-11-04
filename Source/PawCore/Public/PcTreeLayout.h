#pragma once

// The skill tree's shape (Godot skill_tree_ui.gd, its layout and navigation sections): which order the branches are
// drawn in, where every node sits, the canvas around them, which node each direction leads to, and what each node shows
// (LOCKED / READY / OWNED). Pure: the screen (UPawSkillTreeScreen) only draws what this decides. Proven against the real
// Godot layout code on the shipping tree and on 400 random trees (Tests/Data/tree_layout_godot.tsv).
//
// THE LAYOUT (Godot's own words, condensed): each trunk's subtree is seated on its own pointy-top hex lattice (pitch
// HexPitch), the trunk at the origin, the cells behind it reserved for the hub's line, every child in the free cell nearest
// its parent that is inside the limb's wedge (ConeHalf, seen from a virtual hub ConeOrigin cells behind the trunk), forward,
// with room for its own children, and whose line crosses no node and no link. The limbs are rotated onto axes 60 degrees
// apart (straight up first) and pushed out from the hub half a cell at a time until clear of every limb already placed.
// Then any two nodes closer than NodeGap are pushed apart along the outer one's ray, and the whole tree is slid so its top
// left sits at FitMargin.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <map>
#include <string>
#include <vector>

namespace Pc
{
	struct PcVec2
	{
		double X = 0.0;
		double Y = 0.0;
	};

	struct PcRect2
	{
		double X = 0.0, Y = 0.0, W = 0.0, H = 0.0;
		double CentreX() const { return X + W * 0.5; }
		double CentreY() const { return Y + H * 0.5; }
	};

	// skill_tree_ui.gd's constants (Data/CodeConstantsMap.csv destination "presentation").
	namespace TreeLayout
	{
		constexpr double NodeSize = 96.0;          // NODE_SIZE (square)
		constexpr double HubSize = 132.0;          // HUB_SIZE (square): the hub's footprint in the bounds
		constexpr double NodeGap = 196.0;          // NODE_GAP: centre to centre after separation
		constexpr double CanvasW = 6600.0;         // CANVAS_SIZE: the floor of the canvas
		constexpr double CanvasH = 3400.0;
		constexpr double ZoomMin = 0.18;           // ZOOM_MIN
		constexpr double ZoomMax = 1.60;           // ZOOM_MAX
		constexpr double ZoomStep = 0.12;          // ZOOM_STEP (the wheel, the buttons)
		constexpr double FitMargin = 90.0;         // FIT_MARGIN
		constexpr double FitMax = 1.0;             // FIT_MAX
		constexpr int SeparationPasses = 32;       // SEPARATION_PASSES
		constexpr double HexPitch = 240.0;         // HEX_PITCH
		constexpr int HexReach = 6;                // HEX_REACH
		constexpr double ConeHalfDeg = 34.0;       // CONE_HALF
		constexpr int ConeOriginQ = -3;            // CONE_ORIGIN (axial q, r)
		constexpr int ConeOriginR = 0;
		constexpr double ConePenalty = 120.0;      // CONE_PENALTY
		constexpr double BackwardPenalty = 0.6;    // BACKWARD_PENALTY
		constexpr double LinkClearance = NodeSize * 1.15; // LINK_CLEARANCE
		constexpr double CrowdingPenalty = 220.0;  // CROWDING_PENALTY
		constexpr double LimbGapPx = HexPitch * 1.732; // LIMB_GAP_PX
		constexpr int LimbPushMax = 24;            // LIMB_PUSH_MAX
		constexpr double CaptionDrop = 28.0;       // _layout_bounds: the caption under a node
		constexpr double NavConeDeg = 60.0;        // NAV_CONE
		constexpr double NavConeWideDeg = 89.0;    // NAV_CONE_WIDE
		constexpr double NavAnglePenalty = 1.0;    // NAV_ANGLE_PENALTY
	}

	// TRUNK_WHEEL: the trunks clockwise from the top (a layout decision, so authored); any data trunk it misses is appended.
	PAWCORE_API const std::vector<std::string>& TrunkWheel();
	// trunk_order(): the wheel's trunks the data has, then the data's other trunks (sorted).
	PAWCORE_API std::vector<std::string> TrunkOrder(const PcGameData& D);
	// branch_order(): trunk first, then breadth first through children (each parent's children sorted by id); anything
	// unreachable from a trunk appended in the data's order, so a typo'd parent looks wrong instead of vanishing.
	PAWCORE_API std::vector<std::string> BranchOrder(const PcGameData& D);

	struct PcTreeLayout
	{
		PcVec2 Hub;                                  // "hub"
		std::map<std::string, PcVec2> Nodes;         // every branch's centre, canvas-local
		std::vector<std::string> Order;              // BranchOrder: draw order, navigation's Next / Previous
		std::vector<std::string> UnclearLimbs;       // trunks whose limb found no clear offset (Godot push_error): a bug
	};
	// _compute_layout() + _separate_layout() + _normalise_layout(): what the screen opens with.
	PAWCORE_API PcTreeLayout LayOutTree(const PcGameData& D);
	// _separate_layout(): any two nodes closer than NodeGap, the outer one (from the canvas centre) pushed out along its own
	// ray until they are NodeGap apart; up to SeparationPasses passes over the branch order. LayOutTree runs it; on the hex
	// lattice no seat is ever that close (no tree in the fixture moves), so the tests drive it directly.
	PAWCORE_API void SeparateTreeNodes(PcTreeLayout& L);
	// _layout_bounds(): every node square and the caption under it, and the hub's square.
	PAWCORE_API PcRect2 TreeBounds(const PcTreeLayout& L);
	// canvas_size_for(): the canvas a tree of this extent needs; CanvasW x CanvasH is the floor.
	PAWCORE_API PcVec2 TreeCanvasSize(const PcRect2& Bounds);
	// _fit_zoom(): the zoom at which the whole tree fits a view (with FitMargin air), never past FitMax.
	PAWCORE_API double TreeFitZoom(const PcRect2& Bounds, double ViewW, double ViewH);

	// nav_pick(): from a node, the node a direction leads to: inside NavCone of the direction (else the wider cone), the
	// lowest distance x (1 + angle share); a tie goes to the first in branch order. Empty = nothing that way (the node keeps
	// the selection). The hub is never a target.
	PAWCORE_API std::string TreeNavPick(const PcTreeLayout& L, const std::string& From, const PcVec2& Direction);

	// --- what a node shows (skill_tree_ui.gd _node_state, _refresh_node) ------------------------------------------------

	enum class PcTreeNodeState
	{
		Locked, // cannot be bought now (and was never bought)
		Ready,  // affordable THIS instant: the halo
		Owned,  // bought (wins outright: the node, not the next rung)
	};
	PAWCORE_API PcTreeNodeState TreeNodeState(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// Purchasable now: not maxed, sold (parent, gates, rivals), affordable.
	PAWCORE_API bool TreeCanBuyNow(const PcGameData& D, const PcProgress& P, const std::string& Id);
	// _cheapest_affordable(): where the spend step opens; empty when nothing is for sale.
	PAWCORE_API std::string CheapestAffordableBranch(const PcGameData& D, const PcProgress& P);
	// jump_to_affordable(): the next purchasable node after After, in branch order, wrapping (After itself last); empty
	// when none.
	PAWCORE_API std::string NextAffordableBranch(const PcGameData& D, const PcProgress& P, const std::string& After);
	// _has_affordable_branch(): drives "Leave Stardust unspent?" on the way out of the spend step.
	PAWCORE_API bool HasAffordableBranch(const PcGameData& D, const PcProgress& P);

	// _rebuild_links(): one link per branch, from its parent (or the hub) to it.
	struct PcTreeLink
	{
		std::string From; // "hub" for a trunk
		std::string To;
		bool bOwned = false; // both ends bought (a trunk: itself): drawn bright
		bool bFork = false;  // has rivals, not owned, not closed: drawn dashed
		bool bShut = false;  // a rival closed it
	};
	PAWCORE_API std::vector<PcTreeLink> TreeLinks(const PcGameData& D, const PcProgress& P);
}
