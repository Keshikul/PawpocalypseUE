#pragma once

#include "CoreMinimal.h"

// THE ONLY CODE THAT CALLS THE STEAM API (doc 10 §5). Everything else asks UPawPlatformSubsystem.
//
// Steam is on only in the PawpocalypseSteam target (PAW_STEAM_BUILD=1, set by Pawpocalypse.Build.cs for that target) and
// never in an automated run (automation, -unattended): the editor, every test and gate, and the plain Pawpocalypse build
// never start the Steam API, so nothing automated can unlock an achievement or change the presence on the designer's
// account. The code is compiled in every Windows build (PAW_WITH_STEAMWORKS) so the editor build checks it.
//
// The Steam API is called directly (the engine's Steamworks SDK, loaded by the SteamShared plugin), not through Online
// Subsystem Steam: a single-player game needs achievements, rich presence, the overlay, Steam Input's pad type, the
// language and the Steam ID, and the launcher engine would ignore a Steam target's CustomConfig (KB U/01 §2.4).
namespace PawSteam
{
	// Steamworks > App ID (doc 10 §5). tools/steam_files.py and tools/package.py read this line.
	constexpr uint32 AppId = 5720010;

	// True in the PawpocalypseSteam target (whether or not the API started).
	PAWPOCALYPSE_API bool IsSteamBuild();

	// Game module startup, Shipping Steam build: started outside Steam (the exe opened by hand), Steam is asked to start the
	// game and this returns true (the caller exits). False when Steam started it, outside the Steam build, outside Shipping.
	// The engine's own relaunch needs an App ID compiled into the engine (UE_PROJECT_STEAMSHIPPINGID), which the installed
	// engine cannot have (KB S/03 "Shipping builds started outside Steam").
	PAWPOCALYPSE_API bool RestartThroughSteamIfNeeded();

	// Game module startup / shutdown: starts the Steam API (Steam build, not automated) and pumps its callbacks every frame.
	// A Development Steam build started from its exe runs as App ID AppId through the SteamAppId environment variable.
	PAWPOCALYPSE_API void Startup();
	PAWPOCALYPSE_API void Shutdown();

	// True while the Steam API runs (Steam build, Steam running, the account owns the game).
	PAWPOCALYPSE_API bool IsActive();

	// The 64-bit Steam ID in decimal (Auto-Cloud's {64BitSteamID}); empty without Steam.
	PAWPOCALYPSE_API FString SteamId64();
	// The language the player chose for this game in Steam (an API name: "german", "schinese", "latam"); empty without Steam.
	PAWPOCALYPSE_API FString GameLanguage();
	// Running on a Steam Deck.
	PAWPOCALYPSE_API bool IsOnSteamDeck();
	// The real family of the first pad Steam Input passes on as an XInput pad ("PlayStation", "SteamDeck", "Switch", "Xbox",
	// "Generic"); None when Steam translates no pad. Steam Input is started once (Init(false): the game keeps reading XInput).
	PAWPOCALYPSE_API FName TranslatedPadFamily();

	// Sets every achievement of ApiNames the account does not have yet, then stores them once. Returns how many were set.
	// bOutRetry: Steam had not loaded the account's achievements yet (or an id is unknown to Steamworks): call again later.
	PAWPOCALYPSE_API int32 WriteAchievements(const TArray<FString>& ApiNames, bool& bOutRetry);
	// Rich presence: steam_display = Token (a "#" token of the Steamworks rich presence file); empty clears it.
	PAWPOCALYPSE_API void SetPresence(const FString& Token);
	// Opens a web page in the Steam overlay. False when the overlay is off or there is no Steam.
	PAWPOCALYPSE_API bool OpenOverlayWebPage(const FString& Url);

	// The Steam overlay opened (true) or closed (false). Fired on the game thread.
	PAWPOCALYPSE_API TMulticastDelegate<void(bool)>& OnOverlay();
	// Steam loaded the account's achievements (UserStatsReceived_t): a write that had to wait can go now.
	PAWPOCALYPSE_API FSimpleMulticastDelegate& OnStatsReady();
}
