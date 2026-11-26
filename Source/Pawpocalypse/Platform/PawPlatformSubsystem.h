#pragma once

// The platform (doc 10 §5, docs/03's IPawPlatform): what the game asks of the store it runs on. Steam in the PawpocalypseSteam
// build (Platform/PawSteam, the only code that calls the Steam API); nothing anywhere else (the editor, the tests, the plain
// build), where every answer is "no platform" and every write is dropped. Gameplay code never calls Steam.
//
//   ACHIEVEMENTS: every achievement the progress holds is set on Steam (a new one within a second; on a load, those earned
//     before Steam knew, so a save made outside Steam catches up). The API names are the game's ids (Data/AchievementData.csv),
//     the same on Steamworks (tools/steam_files.py writes the upload files and checks them).
//   RICH PRESENCE: in the realm, steam_display = "#Biome_<tier id>" (the biome's name, translated in the Steamworks rich
//     presence file from the string tables); on the title, none (Steam shows the game's name).
//   THE OVERLAY: OnOverlay(true) when the Steam overlay opens; the HUD opens the pause menu.
//   SAVES: the slots live in a folder per Steam account (SaveUserFolder: the 64-bit Steam ID, Auto-Cloud's {64BitSteamID});
//     the settings stay per PC (KB U/10 §5.1).
//   THE LANGUAGE: "system" means the language chosen for the game in Steam (PlatformLanguage).
//   PADS: the real family of a pad Steam Input passes on as an Xbox pad (TranslatedPadFamily), for the prompts.

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PcDevice.h"

#include "PawPlatformSubsystem.generated.h"

UCLASS()
class PAWPOCALYPSE_API UPawPlatformSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// True while a platform runs (Steam started).
	static bool IsActive();
	// The save slots' folder for this platform user (the 64-bit Steam ID); empty without a platform (the SaveGames root).
	static FString SaveUserFolder();
	// The language chosen for the game on the platform (a Steam API name); empty without one.
	static FString PlatformLanguage();
	// The family of a pad the platform translates into an Xbox pad; unset when it translates none (or there is no platform).
	// Asked at most once a second (cached between).
	static TOptional<Pc::PcFamily> TranslatedPadFamily();
	// Steam Input's family name -> the prompt family (no Nintendo glyphs: a Switch pad draws as Generic, i.e. Xbox's).
	static TOptional<Pc::PcFamily> FamilyFromPlatform(FName Family);

	// The presence token for where the player is: "#Biome_<id>" in a running realm with a biome, else "" (cleared).
	static FString PresenceToken(bool bInRealm, const FString& BiomeId);

	// The Steam overlay opened (true) or closed (false).
	TMulticastDelegate<void(bool)> OnOverlay;

	// What was last sent (tests, logs).
	const FString& GetPresence() const { return Presence; }
	int32 GetAchievementsSet() const { return AchievementsSet; }

	// One pass of the work the ticker does each second (the tests call it).
	void Poll();

private:
	void SyncAchievements(bool bForce);
	void SyncPresence();

	FTSTicker::FDelegateHandle Ticker;
	FDelegateHandle OverlayHandle;
	FDelegateHandle StatsHandle;
	FString Presence;
	bool bPresenceSent = false;
	int64 SeenEpoch = -1;
	int64 SeenEarned = -1;
	bool bAchievementsRetry = false;
	int32 AchievementsSet = 0;
};
