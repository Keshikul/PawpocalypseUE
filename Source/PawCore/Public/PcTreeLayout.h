#pragma once

// The skill tree's shape: which order the branches are drawn in, where every node sits, the canvas around them, which
// node each direction leads to, what each node shows (LOCKED / READY / OWNED) and how much of it is revealed. Pure: the
// screen (UPawSkillTreeScreen) only draws what this decides.
//
// THE LAYOUT: the Compass (concept C1, designer 2026-11-26: "Compass is good. You can build it", after "It should be a
// tree not categories" and "can you space it out a little? All icons are touching"). One root, the hub, at the middle; every
// skill on a square of a grid (GridPitch apart); each trunk owns a slice of the compass (TrunkWheel clockwise from straight
// up, equal slices, measured on an ellipse GridStretch wider than tall) and stands GridTrunkRadius out in its middle. Inner
// rings first, every other skill takes the free square near its parent that best grows one ring out (GridRingStep), stays
// in its slice, keeps its link short, straight or at 45 degrees, and away from other branches; a square is never shared, no
// two links cross, no link passes over a skill. Then the whole tree is slid so its top left sits at FitMargin. The research
// behind it: docs/WORKLIST-PLAYTEST-2026-11-26.md, C1. (Until 2026-11-26 the layout was the first try's hex lattice, proven
// against Godot's code by Tests/Data/tree_layout_godot.tsv; retired with it.)

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <map>
#include <set>
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

	// The tree's look and layout constants (presentation, not rules).
	namespace TreeLayout
	{
		constexpr double NodeSize = 84.0;          // a skill's square (a job's diamond is drawn 1.25x)
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
		// The Compass grid (C1).
		constexpr double GridPitch = 172.0;        // between two squares' centres: an 84 skill and ~75 of air (the spacing ask)
		constexpr double GridStretch = 1.6;        // the compass is an ellipse this much wider than tall (the screen is wide)
		constexpr double GridTrunkRadius = 3.5;    // a trunk's distance from the root, in squares (on the ellipse)
		constexpr double GridMinRadius = 1.2;      // no skill closer to the root than this
		constexpr double GridRingStep = 1.15;      // a child wants to sit this much further out than its parent
		constexpr double GridRingWeight = 0.6;     // how much missing that ring costs a square
		constexpr double GridCentreWeight = 0.3;   // how much leaving the slice's middle costs (per radian)
		constexpr double GridOutsideWeight = 1.0;  // how much a square outside its trunk's slice costs
		constexpr double GridLinkClearance = 0.42; // a link may pass no nearer a skill's square than this (in squares)
		constexpr double GridLabelReach = 0.95;    // a branch's name: this many squares past its outermost shown skill
		constexpr double CaptionDrop = 0.0;        // no caption under a node since C1 (the name is in the tooltip)
		constexpr double NavConeDeg = 60.0;        // NAV_CONE
		constexpr double NavConeWideDeg = 89.0;    // NAV_CONE_WIDE
		constexpr double NavAnglePenalty = 1.0;    // NAV_ANGLE_PENALTY
	}

	// The trunks clockwise from the top: the compass's slices (a layout decision, so authored); any data trunk it misses is
	// appended.
	PAWCORE_API const std::vector<std::string>& TrunkWheel();
	// The order the trunks seat their skills in at one depth (the concept's; a different order gives a different tree).
	PAWCORE_API const std::vector<std::string>& GridPlaceOrder();
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
		std::vector<std::string> UnclearLimbs;       // skills that found no clear square within reach (placed anyway): a bug
	};
	// _compute_layout() + _separate_layout() + _normalise_layout(): what the screen opens with.
	PAWCORE_API PcTreeLayout LayOutTree(const PcGameData& D);
	// _separate_layout(): any two nodes closer than NodeGap, the outer one (from the canvas centre) pushed out along its own
	// ray until they are NodeGap apart; up to SeparationPasses passes over the branch order. Kept for a hand-made layout;
	// the grid never crowds, so LayOutTree does not run it.
	PAWCORE_API void SeparateTreeNodes(PcTreeLayout& L);
	// _layout_bounds(): every node square and the caption under it, and the hub's square. With Among, only those nodes (the
	// revealed ones: the view fits what the player can see, C1).
	PAWCORE_API PcRect2 TreeBounds(const PcTreeLayout& L, const std::set<std::string>* Among = nullptr);

	// The trunk a branch grows from (itself for a trunk); empty for an unknown id.
	PAWCORE_API std::string TreeTrunkOf(const PcGameData& D, const std::string& Id);

	// Each trunk's name in the field (C1): on the line from the hub through the middle of its shown skills, GridLabelReach
	// squares past the outermost of them. Trunk order; a trunk with no shown skill has none. With no Among, every skill.
	struct PcTreeLabel
	{
		std::string Trunk;
		PcVec2 At;
	};
	PAWCORE_API std::vector<PcTreeLabel> TreeBranchLabels(const PcGameData& D, const PcTreeLayout& L, const std::set<std::string>* Among = nullptr);
	// canvas_size_for(): the canvas a tree of this extent needs; CanvasW x CanvasH is the floor.
	PAWCORE_API PcVec2 TreeCanvasSize(const PcRect2& Bounds);
	// _fit_zoom(): the zoom at which the whole tree fits a view (with FitMargin air), never past FitMax.
	PAWCORE_API double TreeFitZoom(const PcRect2& Bounds, double ViewW, double ViewH);

	// nav_pick(): from a node, the node a direction leads to: inside NavCone of the direction (else the wider cone), the
	// lowest distance x (1 + angle share); a tie goes to the first in branch order. Empty = nothing that way (the node keeps
	// the selection). The hub is never a target; with Among, only those nodes are (the revealed ones).
	PAWCORE_API std::string TreeNavPick(const PcTreeLayout& L, const std::string& From, const PcVec2& Direction,
		const std::set<std::string>* Among = nullptr);

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

	// How much of a node shows (C1, the research's "one ring ahead"): Full when owned or on sale now (its parent owned, its
	// gates met: the price shows); Ghost when its parent (the hub for a trunk) is Full, faint, no price; else Hidden. The
	// tree grows as it is bought.
	enum class PcTreeReveal
	{
		Hidden,
		Ghost,
		Full,
	};
	PAWCORE_API PcTreeReveal TreeNodeReveal(const PcGameData& D, const PcProgress& P, const std::string& Id);

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
