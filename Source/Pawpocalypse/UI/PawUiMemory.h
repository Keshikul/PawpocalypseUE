#pragma once

// What the UI remembers from one screen to the next (review CR14). Godot keeps it in static vars, for the life of the
// process; here it is the game instance's (in a packaged game the same thing), so none of it carries into another game
// instance: the next automation test, another PIE session. The rules stay Godot's: the dock's open tab and buy step and the
// tree's last node are kept for the session, across games (hud.gd's and skill_tree_ui.gd's statics); the Jobs count seen
// starts again with every game (main_menu.gd _on_slot_chosen sets HUD._jobs_seen = -1; here any new game or load, by
// UPawGameSubsystem's progress epoch).

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PcHud.h"

#include "PawUiMemory.generated.h"

UCLASS()
class PAWPOCALYPSE_API UPawUiMemory : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// The game instance's memory (null without one).
	static UPawUiMemory* Of(const UObject* Context);

	// Kept for the session (Godot's statics).
	Pc::PcDockTab ActiveTab = Pc::PcDockTab::Upgrades; // hud.gd _active_tab
	int64 BuyMultiplier = 1;                            // hud.gd _buy_multiplier: x1 / x10 / x100 / max
	FString TreeLastSelected;                           // skill_tree_ui.gd _last_selected

	// The Jobs badge's count already seen: -1 until this game's first HUD refresh, again after every new game or load.
	int64& JobsSeen();

private:
	int64 JobsSeenValue = -1;
	int64 SeenEpoch = -1;
};
