#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"

// Where the save's bytes live (docs/08 §2). The save subsystem only ever names entries ("slot_0", "slot_0_tmp", ...); the
// backend decides what an entry is. Every method may be called from the save subsystem's writer thread, one call at a time
// (its pipe never runs two writes at once), and from the game thread for reads (after the writes are flushed).
// One exception (review CR19, 2026-11-10): the settings (UPawSettingsSubsystem) read and write their own entry, "settings",
// on the game thread, outside that pipe. Safe while it is another entry than every slot, the platform backend keeps no state
// between calls, and the memory backend locks each call.
class PAWPOCALYPSE_API IPawSaveStorage
{
public:
	virtual ~IPawSaveStorage() = default;
	virtual bool Exists(const FString& Name) = 0;
	virtual bool Read(const FString& Name, TArray<uint8>& Out) = 0;
	virtual bool Write(const FString& Name, const TArray<uint8>& Data) = 0;
	virtual bool Delete(const FString& Name) = 0;
	// For logs and the tests' sandbox check.
	virtual FString Describe() const = 0;
};

// Which storage this run reads and writes, for everything kept between runs (the slots, the settings; docs/09 §1 rule 4):
// -NoSave: none; -PawSaveDir=<folder>: that folder; an automated run (automation, -unattended) without it: none, so the
// designer's own files stay byte-identical; otherwise the platform's save system. Null: nothing is read or written.
// OutWhy says which, for the log.
PAWPOCALYPSE_API TSharedPtr<IPawSaveStorage> PawStorageForThisRun(FString& OutWhy);

// The shipping backend: the platform's save system (ISaveGameSystem through IPlatformFeaturesModule), so a console port
// swaps the backend at no cost (KB P/01). On Windows it is files under Saved/SaveGames.
class PAWPOCALYPSE_API FPawPlatformSaveStorage : public IPawSaveStorage
{
public:
	virtual bool Exists(const FString& Name) override;
	virtual bool Read(const FString& Name, TArray<uint8>& Out) override;
	virtual bool Write(const FString& Name, const TArray<uint8>& Data) override;
	virtual bool Delete(const FString& Name) override;
	virtual FString Describe() const override { return TEXT("platform save system"); }
};

// A folder of <Name>.sav files: the sandbox for automated runs (-PawSaveDir=<folder>), so the designer's own saves stay
// byte-identical.
class PAWPOCALYPSE_API FPawFolderSaveStorage : public IPawSaveStorage
{
public:
	explicit FPawFolderSaveStorage(const FString& InFolder);
	virtual bool Exists(const FString& Name) override;
	virtual bool Read(const FString& Name, TArray<uint8>& Out) override;
	virtual bool Write(const FString& Name, const TArray<uint8>& Data) override;
	virtual bool Delete(const FString& Name) override;
	virtual FString Describe() const override { return FString::Printf(TEXT("folder %s"), *Folder); }

private:
	FString PathOf(const FString& Name) const;
	FString Folder;
};

// The tests' backend: entries in memory, and the faults a disk can have (a write that fails, a write cut short).
class PAWPOCALYPSE_API FPawMemorySaveStorage : public IPawSaveStorage
{
public:
	virtual bool Exists(const FString& Name) override;
	virtual bool Read(const FString& Name, TArray<uint8>& Out) override;
	virtual bool Write(const FString& Name, const TArray<uint8>& Data) override;
	virtual bool Delete(const FString& Name) override;
	virtual FString Describe() const override { return TEXT("memory"); }

	// Test hooks. FailWritesAfter = N: the next N writes succeed, every one after fails (-1 = never fail). TruncateNamed:
	// a write to an entry with this suffix keeps only its first half and reports success (a crash mid-write).
	void FailWritesAfter(int32 N);
	void TruncateWritesTo(const FString& Suffix);
	void Put(const FString& Name, const TArray<uint8>& Data);
	TArray<FString> Names();
	int32 WriteCount();

private:
	FCriticalSection Lock;
	TMap<FString, TArray<uint8>> Entries;
	int32 WritesLeft = -1;
	FString TruncateSuffix;
	int32 Writes = 0;
};
