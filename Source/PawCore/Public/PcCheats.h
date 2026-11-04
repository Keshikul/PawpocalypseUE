#pragma once

// The cheat overlay's rules (cheat_overlay.gd + GameState's debug_* functions): development tools, never a shipped
// feature. Pure progress mutations like the rest of PawCore; the game module calls them from its dev-only console
// commands and does what Godot's signals did afterwards (re-count the kittens, rebuild the stats, the realm's Reawaken
// handling). Ported as Godot has them, quirks included: they are what the designer tests with.

#include "PcCore.h"
#include "PcData.h"
#include "PcStats.h"

#include <cstdint>

namespace Pc
{
	// CheatOverlay.DEBUG_CATNIP_GRANT / DEBUG_STARDUST_GRANT: what one press of each button grants.
	constexpr double CheatCatnipGrant = 1000.0;
	constexpr double CheatStardustGrant = 500.0;
	// GameState.DEBUG_UNCAPPED_STEP: levels one press adds to a line with no cap.
	constexpr int64_t CheatUncappedStep = 25;

	// debug_add_currency(): Catnip through the normal credit (it counts toward the cycle's lifetime Catnip); Stardust
	// straight into the wallet (NOT Stardust ever granted: the Reawaken multiplier does not move).
	PAWCORE_API void CheatAddCurrency(PcProgress& P, double Catnip, double Stardust);
	// debug_max_all_upgrades(): every capped line to the level that reaches its effective cap, every uncapped line up by
	// CheatUncappedStep; never down.
	PAWCORE_API void CheatMaxAllUpgrades(const PcGameData& D, PcProgress& P);
	// debug_set_reawaken_count(): the count alone (the Unicorn button sets the Unicorn's unlock count). The game then runs
	// its Reawaken handling, as Godot's `reawakened` signal did.
	PAWCORE_API void CheatSetReawakenCount(PcProgress& P, int64_t Count);
	// debug_advance_stage(): marks the boss holding up the next evolution stage defeated (one stage); nothing at the top.
	// True when a boss was marked.
	PAWCORE_API bool CheatAdvanceStage(const PcGameData& D, PcProgress& P);
	// debug_unlock_all_tiers(): every boss defeated (the stage gate only).
	PAWCORE_API void CheatUnlockAllTiers(const PcGameData& D, PcProgress& P);
	// debug_unlock_all_stages(): every boss defeated, the cycle's Catnip raised to the deepest biome's price, the deepest
	// biome the last, AND this cycle's passage for every boss realm: all three gates, so every biome opens (doc 01 §5 F18,
	// designer 2026-11-03; it used to leave the passage shut and every biome behind a boss locked on a fresh cycle).
	PAWCORE_API void CheatUnlockAllStages(const PcGameData& D, PcProgress& P);
}
