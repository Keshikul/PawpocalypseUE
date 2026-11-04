#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tasks/Pipe.h"
#include "Tasks/Task.h"
#include "Tickable.h"

#include "PcSave.h"

#include "PawSaveSubsystem.generated.h"

class IPawSaveStorage;
class UPawGameSubsystem;

// What the last LoadGame() did, for the title screen's word to the player and for the tests.
enum class EPawLoadOutcome : uint8
{
	NotTried,
	NoSave,            // the slot is empty
	Loaded,
	LoadedFromBackup,  // the slot would not read; its backup did (the broken file is kept as "<slot>_corrupt")
	Corrupt,           // neither the slot nor its backup reads: nothing was applied
	NewerBuild,        // written by a newer build: never read, never overwritten
	Disabled,          // saving is off (-NoSave, or an automated run with no sandbox)
};

// SaveManager's port (save_manager.gd; docs/08 §2): five slots, the active one, the moments that save, the atomic write
// and the recovery, over PawCore's PcSave (the state and the file) and an IPawSaveStorage (the bytes).
//
// WHEN IT SAVES (Godot's timings): every 30 s of unpaused play (autosave, skipped with nothing to keep); when the window
// loses focus or the app is suspended; on quit (Deinitialize, synchronously); and at the end of any frame in which a
// save-worthy thing happened, coalesced to ONE write however many (Godot connects upgrade_purchased, branch_leveled_up,
// boss_defeated, job_taken, achievement_claimed, card_taken and reawakened; here a digest of exactly what those signals
// report is compared once per frame, so a bulk buy of 54 levels is one write and every caller is covered).
//
// THE WRITE (ISaveGameSystem has no rename): serialise on the game thread, then on a worker, one write at a time: write
// "<slot>_tmp", read it back and verify it, copy the current slot to "<slot>_bak" (only if it reads: a broken slot never
// overwrites a good backup), write the slot, delete the temp. At startup a slot that is missing or will not read is
// restored from a verified temp, else from its backup.
//
// SANDBOX (docs/09 §1 rule 4): -NoSave turns saving off; -PawSaveDir=<folder> keeps the slots in that folder; an
// automation run never reads or writes the real slots unless a test hands in its own storage (UseStorageForTest).
UCLASS()
class PAWPOCALYPSE_API UPawSaveSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	static constexpr int32 SlotCount = 5;
	static constexpr double AutosaveSeconds = 30.0; // save_manager.gd AUTOSAVE_SECONDS

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject: the end-of-frame save, the autosave clock.
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual ETickableTickType GetTickableTickType() const override;

	// --- the slots -----------------------------------------------------------------------------------------------------
	static FString SlotName(int32 Slot);
	bool HasSlot(int32 Slot);
	bool HasAnySave();
	// slot_summary(): what the slot holds, without applying it. False for an empty or unreadable slot.
	bool ReadSlotSummary(int32 Slot, Pc::PcSlotSummary& Out);
	void DeleteSlot(int32 Slot);
	int32 GetActiveSlot() const { return ActiveSlot; }

	// --- saving and loading --------------------------------------------------------------------------------------------
	// Save at the end of this frame, however many things ask.
	void RequestSave();
	// Write now (the pause menu's Save, a Reawaken, a new game creating its slot). Slot >= 0 also makes it the active slot.
	void SaveGame(int32 Slot = -1);
	// The unprompted save (the timer, focus loss, quit): skipped while there is nothing to keep.
	void Autosave();
	// Loads a slot into the live game (-1: the active one). Sets the active slot when something was applied.
	EPawLoadOutcome LoadGame(int32 Slot = -1);
	EPawLoadOutcome GetLastLoad() const { return LastLoad; }
	// Blocks until every queued write has finished.
	void Flush();

	// _recover_interrupted_saves(): run at startup; public for the tests' crash cases.
	void RecoverInterruptedSaves();

	bool IsEnabled() const { return bEnabled; }
	int32 GetWritesDone() const;
	int32 GetWritesFailed() const;

	// The tests' sandbox: their own storage, saving on (even in an automation run). Resets the counters and the clock.
	void UseStorageForTest(TSharedPtr<IPawSaveStorage> InStorage);
	IPawSaveStorage* GetStorage() const { return Storage.Get(); }
	// The digest the end-of-frame save compares (public for the tests).
	uint64 SaveWorthyDigest() const;
	double GetAutosaveClock() const { return AutosaveClock; }

private:
	UPawGameSubsystem* Game() const;
	bool IsWorldPaused() const;
	void QueueWrite(int32 Slot, TArray<uint8>&& Bytes);
	void SeedDigest();
	void OnActivationChanged(bool bActive);
	void OnEnterBackground();

	TSharedPtr<IPawSaveStorage> Storage;
	TSharedPtr<struct FPawSaveWriteStats> Stats; // shared with the writer's tasks: they never hold `this`
	UE::Tasks::FPipe Pipe{ TEXT("PawSave") };
	UE::Tasks::FTask LastWrite;
	int32 ActiveSlot = 0;
	EPawLoadOutcome LastLoad = EPawLoadOutcome::NotTried;
	bool bEnabled = false;
	bool bInitialized = false;
	bool bSaveRequested = false;
	double AutosaveClock = 0.0;
	uint64 LastDigest = 0;
	FDelegateHandle ActivationHandle;
	FDelegateHandle BackgroundHandle;

public:
	UPawSaveSubsystem();
	virtual ~UPawSaveSubsystem() override;
};
