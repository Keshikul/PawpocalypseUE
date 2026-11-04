#include "Core/PawGameMode.h"

#include "Core/PawPlayerController.h"
#include "Dev/PawDebugHud.h"

APawGameMode::APawGameMode()
{
	PlayerControllerClass = APawPlayerController::StaticClass();

	// The cat is an entity in PawCore's field, drawn by the realm view; there is no pawn to possess (docs/03 §3).
	// With no class the engine skips the spawn instead of dropping a flying ADefaultPawn into the map.
	DefaultPawnClass = nullptr;

#if !UE_BUILD_SHIPPING
	// The realm's debug readout (`paw.DebugHud`); Shipping keeps the engine's empty HUD.
	HUDClass = APawDebugHud::StaticClass();
#endif
}
