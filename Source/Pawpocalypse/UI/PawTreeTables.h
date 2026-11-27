#pragma once

// The skill tree's own colours, edited by hand (they began as the first try's skill_tree_ui.gd tables; since 2026-11-26 the
// Compass concept's, C1: docs/WORKLIST-PLAYTEST-2026-11-26.md). sRGB through PawTokens::Srgb like the palette.

#include "CoreMinimal.h"
#include "UI/PawTokens.h"

namespace PawTree
{
	// The Compass (concept C1, designer 2026-11-26): one look per trunk, shared by every skill growing off it. Card: an owned
	// skill's fill; Link: an owned link's colour and the branch's name in the field. A branch reads its trunk's
	// (PcTreeLayout's TreeTrunkOf); an unknown trunk draws the first.
	struct FTrunkLook
	{
		const TCHAR* Id;
		FLinearColor Card;
		FLinearColor Link;
	};
	inline const FTrunkLook TrunkLooks[] = {
		{ TEXT("claws"), PawTokens::Srgb(0x7A, 0x3A, 0x45), PawTokens::Srgb(0xF2, 0x82, 0x6E) },
		{ TEXT("instinct"), PawTokens::Srgb(0x7A, 0x55, 0x28), PawTokens::Srgb(0xF2, 0xB0, 0x5E) },
		{ TEXT("unlock_aura"), PawTokens::Srgb(0x6E, 0x5F, 0x1E), PawTokens::Srgb(0xFF, 0xD4, 0x47) },
		{ TEXT("unlock_unicorn"), PawTokens::Srgb(0x55, 0x3A, 0x7A), PawTokens::Srgb(0xC2, 0x9B, 0xFF) },
		{ TEXT("fortune"), PawTokens::Srgb(0x3D, 0x6A, 0x2A), PawTokens::Srgb(0x8B, 0xD4, 0x5E) },
		{ TEXT("unlock_snare"), PawTokens::Srgb(0x28, 0x5E, 0x58), PawTokens::Srgb(0x5E, 0xD6, 0xC3) },
	};

	// A skill's sticker (the concept's drawSkill): a hard shadow, a die-cut rim (bright when owned, faint when not), an
	// edge, the fill. Not owned: the dark fill (designer 2026-11-26: "Darker cards for the skill menu").
	inline const FLinearColor NodeDark = PawTokens::Srgb(0x1E, 0x18, 0x26);
	inline const FLinearColor NodeShadow = PawTokens::Srgb(0x04, 0x02, 0x06, 217);
	inline const FLinearColor NodeRim = PawTokens::Srgb(0xFF, 0xFD, 0xF6, 235);
	inline const FLinearColor NodeRimOff = PawTokens::Srgb(0xFF, 0xFD, 0xF6, 56);
	inline const FLinearColor NodeEdgeOff = PawTokens::Srgb(0x0E, 0x0A, 0x13);
	// The links: an ink road under every one; its core the trunk's colour where owned, dashed cream to a skill on sale now,
	// dim where not yet.
	inline const FLinearColor LinkUnder = PawTokens::Srgb(0x0E, 0x0A, 0x13);
	inline const FLinearColor LinkNext = PawTokens::Srgb(0xD9, 0xCB, 0xB5);
	inline const FLinearColor LinkOff = PawTokens::Srgb(0x4E, 0x45, 0x5C);
	// A price you cannot pay yet: the chip pale, not yellow.
	inline const FLinearColor ChipShort = PawTokens::Srgb(0xEA, 0xDF, 0xCB);
	// One ring ahead (the research's reveal): seen, faint.
	inline constexpr float GhostOpacity = 0.3f;

	// Rainbow rings that breathe (PULSE_BRANCHES): the Unicorn.
	inline const TCHAR* const PulseBranches[] = { TEXT("unlock_unicorn") };

	// The tooltip's colours (_on_node_hovered's bbcode, two of them not in the palette): the blurb, "Fully bought.", the
	// Next line, why it cannot be bought, and "You have n.".
	inline const FLinearColor TipBlurb = PawTokens::Srgb(0x2A, 0x21, 0x31);
	inline const FLinearColor TipFully = PawTokens::Srgb(0x4E, 0x9A, 0x2E);
	inline const FLinearColor TipNext = PawTokens::Srgb(0x2A, 0x21, 0x31);
	inline const FLinearColor TipReason = PawTokens::Srgb(0x6E, 0x62, 0x76);
	inline const FLinearColor TipShort = PawTokens::Srgb(0xC9, 0x33, 0x23);

	// SHUT (the road not taken: a rival branch was bought); SHINE_COLOR, a self_modulate MULTIPLIER (above 1: the node
	// brightens), not a colour.
	inline const FLinearColor LinkShut = PawTokens::Srgb(43, 41, 54, 166);
	inline const FLinearColor ShineTint = FLinearColor(1.3500f, 1.2500f, 0.8000f, 1.0000f);
}
