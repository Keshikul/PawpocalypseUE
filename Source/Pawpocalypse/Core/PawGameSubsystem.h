#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

// Complete types: UHT's generated constructors and destructor see these members.
#include "PcData.h"
#include "PcSave.h"
#include "PcStats.h"

#include "PawGameSubsystem.generated.h"

// GameState's port (docs/03 §3): owns the rules' data (built from the DataTables through FPawDataAdapter and judged by
// PawCore's validator) and the one PcProgress. Thin until phase 3, which adds the delegates the UI listens to; the realm
// (UPawRealmSubsystem) reads and changes the progress through here.
UCLASS()
class PAWPOCALYPSE_API UPawGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPawGameSubsystem();
	virtual ~UPawGameSubsystem() override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// False when the data failed to load or validate (development builds also log it as an error at startup).
	bool IsDataReady() const { return bDataReady; }
	const TArray<FString>& DataErrors() const { return Errors; }

	const Pc::PcGameData& Data() const { return *GameData; }
	Pc::PcProgress& Progress() { return *Model; }
	const Pc::PcProgress& Progress() const { return *Model; }

	// What the save slot carries beside the progress: the per-slot automations (SettingsManager.auto_travel /
	// auto_pick_card: in the SAVE, so a new game starts with both off) and the tutorial's finished steps. The realm reads
	// the automations from here every frame; the HUD's boxes and the tree's Auto-Travel / Auto-Card nodes write them.
	Pc::PcSlotExtras& Slot() { return SlotState; }
	const Pc::PcSlotExtras& Slot() const { return SlotState; }

	// GameState.debug_reset_progress() / the title's New Game: everything a save carries, cleared (the slot's automations
	// and tutorial too: main_menu.gd _on_slot_chosen switches both automations off for a new game).
	void NewGame();

private:
	TUniquePtr<Pc::PcGameData> GameData;
	TUniquePtr<Pc::PcProgress> Model;
	Pc::PcSlotExtras SlotState;
	TArray<FString> Errors;
	bool bDataReady = false;
};
