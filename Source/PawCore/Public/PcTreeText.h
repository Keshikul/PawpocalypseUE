#pragma once

// What the skill tree's words say (skill_tree_ui.gd _effect_text, _describe, _describe_channel, _on_node_hovered): which
// string-table phrase each part of a branch's effect uses and the number in it, formatted exactly as GDScript does; and what
// the tooltip shows for a node. The game turns the keys into FText (ST_Tree for the phrases and names, ST_UI for the
// tooltip's own lines). The phrase table is generated from the Godot code (PcTreeWording.h, tools/gen_tree_tables.py) and
// proven against Godot's own _effect_text on every branch at every level (Tests/Data/tree_text_godot.tsv).

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <string>
#include <vector>

namespace Pc
{
	// One part of an effect: an ST_Tree key and its one argument ("" when the phrase has none).
	struct PcTreePhrase
	{
		std::string Key;    // FX_<EFFECT>[__<branch>][_one|_other] or CH_<channel>
		std::string Arg;    // "v", "n" or ""
		std::string Number; // the argument, as GDScript prints it ("35", "1.50", "3"): culture-invariant, as Godot's
	};
	// _effect_text(id, level): the primary effect's phrase (none for an effect _describe() does not word), then each merged
	// channel's, joined by the game with ", ". Empty for an unknown branch or a level outside 1..max. Channels follow
	// _describe_channel()'s order (mult, golden, levels, biome), which is the data's order for every shipping branch.
	PAWCORE_API std::vector<PcTreePhrase> TreeEffectPhrases(const PcGameData& D, const std::string& Id, int64_t Level);

	// _on_node_hovered(): what the tooltip shows for a node.
	enum class PcTreeTipReason
	{
		None,
		Closed,          // "Closed — you took <Rival>."
		NeedsParent,     // "Locked — buy <Parent> first."
		ReawakenGate,    // "Locked until Reawaken <Gate>."
		BrowseOnly,      // "Spend after a Reawaken." (sold, but this is not the spend step)
		Short,           // "You have <Have>." (the spend step, too poor)
	};
	struct PcTreeTip
	{
		bool bKnown = false;   // the branch exists
		bool bUnlock = false;  // no "owned/max" after the title
		int64_t Owned = 0;
		int64_t Max = 0;
		bool bMaxed = false;   // "Fully bought." over the effect at Owned
		bool bShowNow = false; // "Now: <effect at Owned>" (owned, not maxed)
		int64_t Next = 0;      // "Next: <effect at Next>" (not maxed)
		int64_t Cost = 0;      // "Cost: <n> Stardust": whole, int(cost)
		PcTreeTipReason Reason = PcTreeTipReason::None;
		std::string Rival;     // Closed
		std::string Parent;    // NeedsParent
		int64_t Gate = 0;      // ReawakenGate
		int64_t Have = 0;      // Short: int(stardust)
		bool bBuyPrompt = false; // "<press> Buy": the spend step, and the node can be bought now
	};
	PAWCORE_API PcTreeTip TreeTip(const PcGameData& D, const PcProgress& P, const std::string& Id, bool bSpendMode);
}
