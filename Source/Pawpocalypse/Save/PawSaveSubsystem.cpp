#include "Save/PawSaveSubsystem.h"

#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "Misc/ScopeLock.h"
#include "PcField.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSettingsSubsystem.h"

#include <atomic>
#include <string>

// What the writer's tasks report back. Shared, so a task never holds the subsystem.
struct FPawSaveWriteStats
{
	std::atomic<int32> Done{ 0 };
	std::atomic<int32> Failed{ 0 };
	std::atomic<int64> Finished{ 0 }; // the last ticket the writer finished (one write at a time, so in order)
	FCriticalSection Lock;
	TSet<int64> FailedTickets;        // under Lock
};

namespace
{
	std::string ToStd(const TArray<uint8>& Bytes)
	{
		return std::string(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<size_t>(Bytes.Num()));
	}

	TArray<uint8> ToBytes(const std::string& S)
	{
		return TArray<uint8>(reinterpret_cast<const uint8*>(S.data()), static_cast<int32>(S.size()));
	}

	Pc::PcSaveReadError Parse(const TArray<uint8>& Bytes, Pc::PcSaveState& Out, FString* Why = nullptr)
	{
		std::string Detail;
		const Pc::PcSaveReadError E = Pc::ReadSaveFile(ToStd(Bytes), Out, &Detail);
		if (Why)
		{
			*Why = UTF8_TO_TCHAR(Detail.c_str());
		}
		return E;
	}

	// The entry exists and reads as a save (or is a newer build's, which is not ours to judge or replace).
	bool HoldsASave(IPawSaveStorage& S, const FString& Name, TArray<uint8>* OutBytes = nullptr)
	{
		TArray<uint8> Bytes;
		if (!S.Exists(Name) || !S.Read(Name, Bytes))
		{
			return false;
		}
		Pc::PcSaveState Ignored;
		const Pc::PcSaveReadError E = Parse(Bytes, Ignored);
		if (E != Pc::PcSaveReadError::None && E != Pc::PcSaveReadError::NewerFormat)
		{
			return false;
		}
		if (OutBytes)
		{
			*OutBytes = MoveTemp(Bytes);
		}
		return true;
	}

	double UnixNow()
	{
		return (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalSeconds();
	}

	// The write, on the writer thread: temp, verify, backup the current slot if it reads, the slot, drop the temp.
	bool WriteSlotAtomically(IPawSaveStorage& S, const FString& Name, const TArray<uint8>& Bytes, FString& Error)
	{
		const FString Temp = Name + TEXT("_tmp");
		const FString Backup = Name + TEXT("_bak");
		TArray<uint8> Current;
		const bool bHasCurrent = S.Exists(Name) && S.Read(Name, Current);
		Pc::PcSaveReadError CurrentRead = Pc::PcSaveReadError::Empty;
		if (bHasCurrent)
		{
			Pc::PcSaveState Ignored;
			CurrentRead = Parse(Current, Ignored);
			if (CurrentRead == Pc::PcSaveReadError::NewerFormat)
			{
				Error = TEXT("the slot holds a newer build's save: not overwritten");
				return false;
			}
			if (CurrentRead != Pc::PcSaveReadError::None)
			{
				// A slot that does not read is kept aside before anything replaces it (F47: a new game started in an
				// unreadable slot), never thrown away: a broken save is evidence, and might be repairable by hand.
				S.Write(Name + TEXT("_corrupt"), Current);
			}
		}
		if (!S.Write(Temp, Bytes))
		{
			Error = TEXT("the temp file could not be written");
			return false;
		}
		TArray<uint8> Back;
		if (!S.Read(Temp, Back) || Back != Bytes)
		{
			Error = TEXT("the temp file did not read back as written");
			return false;
		}
		if (bHasCurrent)
		{
			// A slot that does not read is never copied over a good backup (Godot's rename did exactly that).
			if (CurrentRead == Pc::PcSaveReadError::None && !S.Write(Backup, Current))
			{
				Error = TEXT("the backup could not be written");
				return false;
			}
		}
		if (!S.Write(Name, Bytes))
		{
			Error = TEXT("the slot could not be written (the temp file is kept for the next start)");
			return false;
		}
		S.Delete(Temp);
		return true;
	}
}

UPawSaveSubsystem::UPawSaveSubsystem() = default;
UPawSaveSubsystem::~UPawSaveSubsystem() = default;

void UPawSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UPawGameSubsystem>();
	Collection.InitializeDependency<UPawSettingsSubsystem>();
	Stats = MakeShared<FPawSaveWriteStats>();
	// An automated run never touches the real slots (docs/09 §1 rule 4): a test hands in its own storage.
	FString Why;
	Storage = PawStorageForThisRun(Why);
	bEnabled = Storage.IsValid();
	UE_LOG(LogPaw, Log, TEXT("Save: %s"), *Why);
	bInitialized = true;
	StartOnLastSlot();
	if (bEnabled)
	{
		RestoredFromBackup = 0;
		RecoverInterruptedSaves();
		// Godot's SaveManager loads the last slot at startup; the title's Continue then simply plays it.
		LoadGame(ActiveSlot);
	}
	SeedDigest();
	if (FSlateApplication::IsInitialized())
	{
		ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this,
			&UPawSaveSubsystem::OnActivationChanged);
	}
	BackgroundHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UPawSaveSubsystem::OnEnterBackground);
}

void UPawSaveSubsystem::Deinitialize()
{
	if (ActivationHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
	}
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundHandle);
	// The close button: save, then quit (save_manager.gd NOTIFICATION_WM_CLOSE_REQUEST), synchronously.
	Autosave();
	Flush();
	bInitialized = false;
	Super::Deinitialize();
}

UPawGameSubsystem* UPawSaveSubsystem::Game() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UPawGameSubsystem>() : nullptr;
}

const UPawRealmSubsystem* UPawSaveSubsystem::RunningRealm() const
{
	const UGameInstance* GI = GetGameInstance();
	const UWorld* W = GI ? GI->GetWorld() : nullptr;
	const UPawRealmSubsystem* R = W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	return R && R->IsRunning() ? R : nullptr;
}

bool UPawSaveSubsystem::IsWorldPaused() const
{
	const UGameInstance* GI = GetGameInstance();
	const UWorld* W = GI ? GI->GetWorld() : nullptr;
	const UPawRealmSubsystem* R = W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	return R && R->IsPaused();
}

bool UPawSaveSubsystem::IsRealmRunning() const
{
	return RunningRealm() != nullptr;
}

// --- ticking -------------------------------------------------------------------------------------------------------------

TStatId UPawSaveSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPawSaveSubsystem, STATGROUP_Tickables);
}

bool UPawSaveSubsystem::IsTickable() const
{
	return bInitialized && !IsTemplate();
}

ETickableTickType UPawSaveSubsystem::GetTickableTickType() const
{
	return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional;
}

void UPawSaveSubsystem::Tick(float DeltaTime)
{
	const UPawGameSubsystem* G = Game();
	if (!bEnabled || !G || !G->IsDataReady())
	{
		return;
	}
	// One write at the end of a frame in which something worth keeping happened, however many things did.
	const uint64 Digest = SaveWorthyDigest();
	if (Digest != LastDigest)
	{
		LastDigest = Digest;
		bSaveRequested = true;
	}
	if (bSaveRequested)
	{
		SaveGame();
	}
	// Runs while a game is played. It pauses with the game (Godot's timer pauses with the tree: an open skill tree or card pick
	// writes nothing new) and stops on the title, where nothing changes (review CR21, F48: Godot's ran on the title and rewrote
	// the loaded slot every 30 s, a game left without saving included).
	if (IsRealmRunning() && !IsWorldPaused())
	{
		AutosaveClock += DeltaTime;
		if (AutosaveClock >= AutosaveSeconds)
		{
			AutosaveClock = 0.0;
			Autosave();
		}
	}
}

uint64 UPawSaveSubsystem::SaveWorthyDigest() const
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return 0;
	}
	const Pc::PcProgress& P = G->Progress();
	// FNV-1a over exactly what Godot's save signals report: levels bought (shop, tree), bosses beaten, jobs, claims, cards
	// taken, Reawakens.
	uint64 H = 1469598103934665603ull;
	auto Mix = [&H](const void* Data, size_t N)
	{
		const uint8* B = static_cast<const uint8*>(Data);
		for (size_t I = 0; I < N; ++I)
		{
			H = (H ^ B[I]) * 1099511628211ull;
		}
	};
	auto MixStr = [&Mix](const std::string& S) { Mix(S.data(), S.size() + 1); };
	auto MixInt = [&Mix](int64 V) { Mix(&V, sizeof(V)); };
	for (const auto& [K, V] : P.UpgradeLevels)
	{
		MixStr(K);
		MixInt(V);
	}
	MixInt(-1);
	for (const auto& [K, V] : P.BranchLevels)
	{
		MixStr(K);
		MixInt(V);
	}
	MixInt(-2);
	for (const std::string& K : P.BossesDefeated) { MixStr(K); }
	MixInt(-3);
	for (const std::string& K : P.BossesClearedThisCycle) { MixStr(K); }
	MixInt(-4);
	for (const std::string& K : P.JobsHeld) { MixStr(K); }
	MixStr(P.CurrentJob);
	MixStr(P.RunJob);
	MixInt(P.bRunJobChosen ? 1 : 0);
	for (const std::string& K : P.AchievementsClaimed) { MixStr(K); }
	MixInt(-5);
	MixInt(P.CardsTakenThisRun);
	MixInt(P.ReawakenCount);
	return H;
}

void UPawSaveSubsystem::SeedDigest()
{
	LastDigest = SaveWorthyDigest();
}

void UPawSaveSubsystem::OnActivationChanged(bool bActive)
{
	// Alt-tab, minimise, the Steam overlay: the moments before a player walks away. Not on the title, where nothing changes
	// (review CR21).
	if (!bActive && IsRealmRunning())
	{
		Autosave();
	}
}

void UPawSaveSubsystem::OnEnterBackground()
{
	if (IsRealmRunning())
	{
		Autosave(); // the console / Deck suspend: the only warning some platforms give
	}
}

// --- the slots -----------------------------------------------------------------------------------------------------------

FString UPawSaveSubsystem::SlotName(int32 Slot)
{
	return FString::Printf(TEXT("slot_%d"), FMath::Clamp(Slot, 0, SlotCount - 1));
}

bool UPawSaveSubsystem::HasSlot(int32 Slot)
{
	if (!bEnabled)
	{
		return false;
	}
	Flush();
	return Storage->Exists(SlotName(Slot));
}

bool UPawSaveSubsystem::HasAnySave()
{
	for (int32 I = 0; I < SlotCount; ++I)
	{
		if (HasSlot(I))
		{
			return true;
		}
	}
	return false;
}

bool UPawSaveSubsystem::ReadSlotSummary(int32 Slot, Pc::PcSlotSummary& Out)
{
	const Pc::PcSlotState State = SlotState(Slot, &Out);
	return State == Pc::PcSlotState::Readable || State == Pc::PcSlotState::ReadableFromBackup;
}

Pc::PcSlotState UPawSaveSubsystem::SlotState(int32 Slot, Pc::PcSlotSummary* OutSummary)
{
	if (!HasSlot(Slot))
	{
		return Pc::PcSlotState::Empty;
	}
	const FString Name = SlotName(Slot);
	auto ReadAs = [this](const FString& File, Pc::PcSaveState& Out)
	{
		TArray<uint8> Bytes;
		return Storage->Read(File, Bytes) ? Parse(Bytes, Out) : Pc::PcSaveReadError::Empty;
	};
	Pc::PcSaveState S;
	const Pc::PcSaveReadError SlotRead = ReadAs(Name, S);
	const FString Backup = Name + TEXT("_bak");
	const bool bBackup = SlotRead != Pc::PcSaveReadError::None && SlotRead != Pc::PcSaveReadError::NewerFormat
		&& Storage->Exists(Backup);
	const Pc::PcSaveReadError BackupRead = bBackup ? ReadAs(Backup, S) : Pc::PcSaveReadError::Empty;
	const Pc::PcSlotState State = Pc::ClassifySlot(true, SlotRead, bBackup, BackupRead);
	if (OutSummary && (State == Pc::PcSlotState::Readable || State == Pc::PcSlotState::ReadableFromBackup))
	{
		*OutSummary = Pc::SlotSummary(S);
	}
	return State;
}

void UPawSaveSubsystem::DeleteSlot(int32 Slot)
{
	if (!bEnabled)
	{
		return;
	}
	Flush();
	const FString Name = SlotName(Slot);
	// And its backup and any half-written save, or the recovery would bring the deleted game back on the next start.
	Storage->Delete(Name);
	Storage->Delete(Name + TEXT("_bak"));
	Storage->Delete(Name + TEXT("_tmp"));
}

void UPawSaveSubsystem::RecoverInterruptedSaves()
{
	for (int32 I = 0; I < SlotCount; ++I)
	{
		const FString Name = SlotName(I);
		const FString Temp = Name + TEXT("_tmp");
		if (HoldsASave(*Storage, Name))
		{
			// A crash after the slot was written and before the temp went: the slot is the newer, the temp a copy.
			Storage->Delete(Temp);
			continue;
		}
		if (Storage->Exists(Name))
		{
			// Kept, never thrown away: a broken save is evidence (and might be repairable by hand).
			TArray<uint8> Broken;
			if (Storage->Read(Name, Broken))
			{
				Storage->Write(Name + TEXT("_corrupt"), Broken);
			}
		}
		// A missing or broken slot is finished from the newest good copy: the verified temp, else the backup.
		for (const FString& Candidate : { Temp, Name + TEXT("_bak") })
		{
			TArray<uint8> Bytes;
			if (HoldsASave(*Storage, Candidate, &Bytes) && Storage->Write(Name, Bytes))
			{
				if (Candidate == Temp)
				{
					Storage->Delete(Temp);
				}
				else
				{
					// The game is the one before the last save: the title says so once it loads (CR2).
					RestoredFromBackup |= 1u << I;
				}
				UE_LOG(LogPaw, Warning, TEXT("Save: slot %d restored from %s"), I + 1, *Candidate);
				break;
			}
		}
	}
}

// --- saving --------------------------------------------------------------------------------------------------------------

void UPawSaveSubsystem::RequestSave()
{
	bSaveRequested = true;
}

int64 UPawSaveSubsystem::SaveGame(int32 Slot)
{
	bSaveRequested = false; // an explicit save supersedes any queued one
	UPawGameSubsystem* G = Game();
	if (!bEnabled || !G || !G->IsDataReady())
	{
		return 0;
	}
	if (Slot >= 0)
	{
		// Saving somewhere is the clearest statement about where the next autosave belongs (and the last slot used).
		ActiveSlot = FMath::Clamp(Slot, 0, SlotCount - 1);
		RememberSlot();
		ClearTitleNotice(); // the game is in a slot of the player's choosing now
	}
	// F36 (D16): a boss fight is not saved, so neither is what its summon cost.
	const UPawRealmSubsystem* R = RunningRealm();
	const Pc::PcSummonPayment* FightUp = R ? Pc::FightPayment(R->GetSim().Field) : nullptr;
	const Pc::PcSaveState S = Pc::CaptureSave(G->Data(), G->Progress(), G->Slot(), UnixNow(), FightUp);
	const std::string File = Pc::WriteSaveFile(S, TCHAR_TO_UTF8(FApp::GetBuildVersion()));
	SeedDigest();
	return QueueWrite(ActiveSlot, ToBytes(File));
}

int64 UPawSaveSubsystem::QueueWrite(int32 Slot, TArray<uint8>&& Bytes)
{
	TSharedPtr<IPawSaveStorage> S = Storage;
	TSharedPtr<FPawSaveWriteStats> St = Stats;
	const FString Name = SlotName(Slot);
	const int64 Ticket = ++NextTicket;
	LastWrite = Pipe.Launch(TEXT("PawSaveWrite"), [S, St, Name, Ticket, Data = MoveTemp(Bytes)]()
	{
		FString Error;
		if (WriteSlotAtomically(*S, Name, Data, Error))
		{
			St->Done.fetch_add(1);
		}
		else
		{
			St->Failed.fetch_add(1);
			{
				FScopeLock Guard(&St->Lock);
				St->FailedTickets.Add(Ticket);
			}
			UE_LOG(LogPaw, Error, TEXT("Save: %s failed: %s"), *Name, *Error);
		}
		St->Finished.store(Ticket);
	});
	return Ticket;
}

EPawWriteResult UPawSaveSubsystem::GetWriteResult(int64 Ticket) const
{
	if (Ticket <= 0 || !Stats)
	{
		return EPawWriteResult::NotQueued;
	}
	if (Stats->Finished.load() < Ticket)
	{
		return EPawWriteResult::Pending;
	}
	FScopeLock Guard(&Stats->Lock);
	return Stats->FailedTickets.Contains(Ticket) ? EPawWriteResult::Failed : EPawWriteResult::Written;
}

void UPawSaveSubsystem::Autosave()
{
	const UPawGameSubsystem* G = Game();
	// Nothing to keep yet: an idle fresh install never writes an empty slot (and never offers Continue on it).
	if (!bEnabled || !G || !G->IsDataReady() || !Pc::HasProgressToSave(G->Progress()))
	{
		return;
	}
	SaveGame();
}

void UPawSaveSubsystem::Flush()
{
	if (LastWrite.IsValid())
	{
		LastWrite.Wait();
	}
}

int32 UPawSaveSubsystem::GetWritesDone() const
{
	return Stats ? Stats->Done.load() : 0;
}

int32 UPawSaveSubsystem::GetWritesFailed() const
{
	return Stats ? Stats->Failed.load() : 0;
}

// --- loading -------------------------------------------------------------------------------------------------------------

EPawLoadOutcome UPawSaveSubsystem::LoadGame(int32 Slot)
{
	UPawGameSubsystem* G = Game();
	if (!bEnabled || !G || !G->IsDataReady())
	{
		LastLoad = EPawLoadOutcome::Disabled;
		return LastLoad;
	}
	Flush();
	const int32 Which = FMath::Clamp(Slot < 0 ? ActiveSlot : Slot, 0, SlotCount - 1);
	const FString Name = SlotName(Which);
	TArray<uint8> Bytes;
	if (!Storage->Exists(Name) || !Storage->Read(Name, Bytes))
	{
		LastLoad = EPawLoadOutcome::NoSave;
		SetLoadNotice(Which, LastLoad);
		return LastLoad;
	}
	Pc::PcSaveState S;
	FString Why;
	const Pc::PcSaveReadError E = Parse(Bytes, S, &Why);
	EPawLoadOutcome Outcome = EPawLoadOutcome::Loaded;
	if (E == Pc::PcSaveReadError::NewerFormat)
	{
		UE_LOG(LogPaw, Warning, TEXT("Save: slot %d was written by a newer build; left alone"), Which + 1);
		LastLoad = EPawLoadOutcome::NewerBuild;
		SetLoadNotice(Which, LastLoad);
		return LastLoad;
	}
	if (E != Pc::PcSaveReadError::None)
	{
		// The previous save, from before the write that broke this one: losing one purchase beats losing the game.
		UE_LOG(LogPaw, Error, TEXT("Save: slot %d is corrupt (%hs: %s), trying its backup"), Which + 1,
			Pc::SaveReadErrorName(E), *Why);
		TArray<uint8> Backup;
		if (!Storage->Read(Name + TEXT("_bak"), Backup) || Parse(Backup, S) != Pc::PcSaveReadError::None)
		{
			UE_LOG(LogPaw, Error, TEXT("Save: slot %d and its backup are unreadable; nothing loaded"), Which + 1);
			LastLoad = EPawLoadOutcome::Corrupt;
			SetLoadNotice(Which, LastLoad);
			return LastLoad;
		}
		Storage->Write(Name + TEXT("_corrupt"), Bytes);
		Outcome = EPawLoadOutcome::LoadedFromBackup;
	}
	else if (RestoredFromBackup & (1u << Which))
	{
		Outcome = EPawLoadOutcome::LoadedFromBackup; // the startup recovery already put the backup in the slot
	}
	RestoredFromBackup &= ~(1u << Which);
	ActiveSlot = Which;
	RememberSlot();
	SetLoadNotice(Which, Outcome);
	const Pc::PcLoadResult R = Pc::ApplySave(G->Data(), G->Progress(), S, G->Slot());
	G->Slot() = R.Extras;
	G->MarkProgressReplaced();
	if (R.bTreeMigrated)
	{
		UE_LOG(LogPaw, Warning, TEXT("Save: migrated the skill tree to the v6 unlock / ladder shape"));
	}
	SeedDigest();
	if (R.Path == Pc::PcLoadPath::OldVersion)
	{
		UE_LOG(LogPaw, Warning, TEXT("Save: v%lld predates the current schema (v%lld): progression reset, Catnip and upgrade levels kept"),
			static_cast<long long>(S.Version), static_cast<long long>(Pc::PcGodotSaveVersion));
		SaveGame();
	}
	LastLoad = Outcome;
	return LastLoad;
}

void UPawSaveSubsystem::SetLoadNotice(int32 Slot, EPawLoadOutcome Outcome)
{
	// Only for the slot the player is left on: a slot that failed from the slots screen keeps its card's word there.
	if (Slot != ActiveSlot)
	{
		return;
	}
	switch (Outcome)
	{
	case EPawLoadOutcome::NewerBuild: TitleNotice = Pc::PcTitleNotice::NewerBuild; break;
	case EPawLoadOutcome::Corrupt: TitleNotice = Pc::PcTitleNotice::Unreadable; break;
	case EPawLoadOutcome::LoadedFromBackup: TitleNotice = Pc::PcTitleNotice::FromBackup; break;
	default: TitleNotice = Pc::PcTitleNotice::None; break;
	}
	TitleNoticeSlot = Slot;
}

void UPawSaveSubsystem::DiscardUnsaved()
{
	// F51 (D16): Godot's discard ("Leaves without writing") kept the game in memory: Continue played the discarded progress
	// and the title's autosave wrote it within 30 s. Here the unsaved play is dropped. A slot this build cannot read (corrupt,
	// a newer build's) leaves the game as it is; so does a run that saves nothing.
	UPawGameSubsystem* G = Game();
	if (!bEnabled || !G || !G->IsDataReady())
	{
		return;
	}
	if (LoadGame(ActiveSlot) == EPawLoadOutcome::NoSave)
	{
		G->NewGame();
		SeedDigest(); // nothing new to save: a fresh game is not written
	}
}

void UPawSaveSubsystem::StartOnLastSlot()
{
	// save_manager.gd _ready(): active_slot = clampi(SettingsManager.last_slot, 0, SLOT_COUNT - 1).
	ActiveSlot = 0;
	if (const UPawSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>())
	{
		ActiveSlot = static_cast<int32>(FMath::Clamp<int64>(Settings->Get().LastSlot, 0, SlotCount - 1));
	}
}

void UPawSaveSubsystem::RememberSlot()
{
	// SettingsManager.set_last_slot(): a machine preference, not progression; it belongs with the volume.
	if (UPawSettingsSubsystem* Settings = GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>())
	{
		Settings->SetLastSlot(ActiveSlot);
	}
}

// --- the tests' sandbox --------------------------------------------------------------------------------------------------

void UPawSaveSubsystem::UseStorageForTest(TSharedPtr<IPawSaveStorage> InStorage)
{
	Flush();
	Storage = InStorage;
	bEnabled = Storage.IsValid();
	Stats = MakeShared<FPawSaveWriteStats>();
	StartOnLastSlot(); // as at startup (a test hands the settings their storage first)
	AutosaveClock = 0.0;
	bSaveRequested = false;
	LastLoad = EPawLoadOutcome::NotTried;
	TitleNotice = Pc::PcTitleNotice::None;
	TitleNoticeSlot = 0;
	RestoredFromBackup = 0;
	NextTicket = 0;
	SeedDigest();
}
