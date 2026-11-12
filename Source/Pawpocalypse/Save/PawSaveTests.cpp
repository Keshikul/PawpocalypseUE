// Pawpocalypse.Save.*: the save container (UPawSaveSubsystem over PcSave), on a memory storage. The rules of WHAT is saved
// and how a load applies are proven against Godot in PawCore (PcSaveTests, fixture `save`); these prove the container:
// the sandbox, the round trip, the atomic write and every crash point, the backup, the moments that save, the autosave.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "PcAchievements.h"
#include "PcData.h"
#include "PcProgress.h"
#include "PcEconomy.h"
#include "PcCheats.h"
#include "PcFieldRules.h"
#include "PcSave.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"

#include <string>

namespace
{
	struct FSaveWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawSaveSubsystem* Save = nullptr;
		TSharedPtr<FPawMemorySaveStorage> Disk;

		bool Open(FAutomationTestBase& T, bool bSandbox = true)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			Save = GI->GetSubsystem<UPawSaveSubsystem>();
			if (!T.TestNotNull(TEXT("the save subsystem exists"), Save) || !T.TestNotNull(TEXT("the game subsystem exists"), Game)
				|| !T.TestTrue(TEXT("the game data is ready"), Game->IsDataReady()))
			{
				return false;
			}
			Game->NewGame();
			if (bSandbox)
			{
				Disk = MakeShared<FPawMemorySaveStorage>();
				Save->UseStorageForTest(Disk);
			}
			return true;
		}

		void Close()
		{
			if (Save)
			{
				Save->UseStorageForTest(nullptr); // nothing written on the way out (Deinitialize's autosave)
			}
			if (GI)
			{
				GI->Shutdown();
			}
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		Pc::PcProgress& P() { return Game->Progress(); }
		const Pc::PcGameData& D() { return Game->Data(); }

		// A game worth saving: something in every field the save carries.
		void Play()
		{
			Pc::PcProgress& Pr = P();
			Pr.Catnip = 123456.789;
			Pr.Stardust = 42.5;
			Pr.ReawakenCount = 3;
			Pr.LifetimeCatnipEver = 9.87654321e12;
			Pr.LifetimeCatnipThisCycle = 5.5e9;
			Pr.StardustGrantedEver = 77.25;
			Pr.UpgradeLevels[D().Upgrades.front().Id] = 12;
			Pr.BranchLevels[D().Branches.front().Id] = 1;
			Pr.BossesDefeated = { D().Code.BossRealms.front() };
			Pr.BossesClearedThisCycle = { D().Code.BossRealms.front() };
			Pr.LifetimeKills = 31337;
			Pr.TotalPlaySeconds = 4321.5;
			Pr.PlaySecondsThisCycle = 600.25;
			Pr.CurrentBiomeId = D().Tiers[1].Id;
			Pr.TierKillCount = 250;
			Pr.BaitSpentByBiome[D().Code.BossRealms.front()] = 40;
			// D17 E2: the Bait bank, kept through Reawakens (more than this run's kills, as in play: every kill counts in both).
			Pr.BaitKillsByBiome[D().Code.BossRealms.front()] = 9700;
			Pr.BaitKillsByBiome[D().Tiers[1].Id] = 1250;
			Pr.JobsHeld = { D().Jobs.front().Id };
			Pr.CurrentJob = D().Jobs.front().Id;
			Pr.RunJob = D().Jobs.front().Id;
			Pr.bRunJobChosen = true;
			Pr.AchievementsEarned = { D().Achievements.front().Id };
			Game->Slot().bAutoTravel = true;
			Game->Slot().TutorialDone = { "card", "tree" };
			// Settled, as play leaves it: a load's re-scan then has nothing new to earn.
			Pc::EvaluateAchievements(D(), Pr);
		}

		// Everything a save carries, as the file it would write (the clock zeroed: it is stamped per write).
		FString Snapshot()
		{
			Pc::PcProgress Copy = P();
			Pc::PcSaveState S = Pc::CaptureSave(D(), Copy, Game->Slot(), 0.0);
			return UTF8_TO_TCHAR(Pc::WriteSaveFile(S, "test").c_str());
		}

		TArray<uint8> Bytes(const FString& Name)
		{
			TArray<uint8> Out;
			Disk->Read(Name, Out);
			return Out;
		}
	};

	TArray<uint8> Garbage()
	{
		const char* Text = "PAWSAVE 1 00000000 5\nhello";
		return TArray<uint8>(reinterpret_cast<const uint8*>(Text), static_cast<int32>(strlen(Text)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveSandboxTest, "Pawpocalypse.Save.Sandbox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveSandboxTest::RunTest(const FString& Parameters)
{
	// An automation run with no -PawSaveDir never reads or writes the real slots (docs/09 §1 rule 4).
	FSaveWorld W;
	if (!W.Open(*this, false))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("an automation run"), GIsAutomationTesting);
	TestFalse(TEXT("saving is off"), W.Save->IsEnabled());
	TestNull(TEXT("no storage at all: nothing to touch"), W.Save->GetStorage());
	W.Play();
	W.Save->SaveGame(0);
	W.Save->Autosave();
	W.Save->Tick(100.0f);
	W.Save->Flush();
	TestEqual(TEXT("nothing written"), W.Save->GetWritesDone(), 0);
	TestTrue(TEXT("a load does nothing"), W.Save->LoadGame(0) == EPawLoadOutcome::Disabled);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveRoundTripTest, "Pawpocalypse.Save.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveRoundTripTest::RunTest(const FString& Parameters)
{
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("an empty disk has no save"), W.Save->HasAnySave());
	TestTrue(TEXT("an empty slot loads nothing"), W.Save->LoadGame(3) == EPawLoadOutcome::NoSave);
	W.Play();
	const FString Before = W.Snapshot();
	W.Save->SaveGame(3);
	W.Save->Flush();
	TestEqual(TEXT("one write"), W.Save->GetWritesDone(), 1);
	TestEqual(TEXT("saving to a slot makes it the active one"), W.Save->GetActiveSlot(), 3);
	TestTrue(TEXT("the slot exists"), W.Save->HasSlot(3));
	TestFalse(TEXT("no temp file left"), W.Disk->Exists(TEXT("slot_3_tmp")));
	TestFalse(TEXT("no backup of nothing"), W.Disk->Exists(TEXT("slot_3_bak")));

	// A new game in memory, then the load: every field comes back.
	W.Game->NewGame();
	TestTrue(TEXT("the new game differs"), W.Snapshot() != Before);
	TestFalse(TEXT("a new game switches the automation off"), W.Game->Slot().bAutoTravel);
	TestTrue(TEXT("loaded"), W.Save->LoadGame(3) == EPawLoadOutcome::Loaded);
	// The load forgets where the player stood (the realm restores the biome's counter on arrival); the save banked it.
	TestEqual(TEXT("the biome forgotten"), FString(W.P().CurrentBiomeId.c_str()), FString());
	TestEqual(TEXT("its kills banked"), W.P().TierKillsByBiome[W.D().Tiers[1].Id], static_cast<int64>(250));
	Pc::PcProgress& P = W.P();
	P.CurrentBiomeId = W.D().Tiers[1].Id;
	P.TierKillCount = 250;
	TestEqual(TEXT("every saved field round-trips"), W.Snapshot(), Before);
	TestTrue(TEXT("the slot's automation came back"), W.Game->Slot().bAutoTravel);
	TestTrue(TEXT("the tutorial came back"), W.Game->Slot().TutorialDone.count("tree") == 1);

	Pc::PcSlotSummary M;
	TestTrue(TEXT("a summary reads"), W.Save->ReadSlotSummary(3, M));
	TestEqual(TEXT("summary: Reawakens"), M.Reawakens, static_cast<int64>(3));
	TestEqual(TEXT("summary: kills"), M.Kills, static_cast<int64>(31337));
	TestTrue(TEXT("summary: stamped"), M.LastPlayedUnix > 1.7e9);
	TestFalse(TEXT("no summary for an empty slot"), W.Save->ReadSlotSummary(4, M));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveAtomicTest, "Pawpocalypse.Save.Atomic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveAtomicTest::RunTest(const FString& Parameters)
{
	// The errors this test provokes on purpose (a refused write, a broken slot) are logged as errors in play.
	AddExpectedError(TEXT("the temp file did not read back as written"), EAutomationExpectedErrorFlags::Contains, 1, false);
	AddExpectedError(TEXT("the backup could not be written"), EAutomationExpectedErrorFlags::Contains, 1, false);
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Play();
	W.Save->SaveGame(0);
	W.Save->Flush();
	const TArray<uint8> First = W.Bytes(TEXT("slot_0"));
	W.P().Catnip = 1.0;
	W.Save->SaveGame();
	W.Save->Flush();
	TestTrue(TEXT("the previous save is the backup"), W.Bytes(TEXT("slot_0_bak")) == First);
	TestTrue(TEXT("the slot is the new save"), W.Bytes(TEXT("slot_0")) != First);

	// The temp is written cut short (a crash mid-write): the verify refuses it, the slot and backup stand.
	const TArray<uint8> Second = W.Bytes(TEXT("slot_0"));
	W.Disk->TruncateWritesTo(TEXT("_tmp"));
	W.P().Catnip = 2.0;
	W.Save->SaveGame();
	W.Save->Flush();
	TestEqual(TEXT("a write that does not verify fails"), W.Save->GetWritesFailed(), 1);
	TestTrue(TEXT("the slot untouched"), W.Bytes(TEXT("slot_0")) == Second);
	TestTrue(TEXT("the backup untouched"), W.Bytes(TEXT("slot_0_bak")) == First);
	W.Disk->TruncateWritesTo(FString());

	// The disk fails after the temp (the backup write fails): nothing of the slot changes.
	W.Disk->FailWritesAfter(1);
	W.P().Catnip = 3.0;
	W.Save->SaveGame();
	W.Save->Flush();
	TestEqual(TEXT("the failed backup stops the write"), W.Save->GetWritesFailed(), 2);
	TestTrue(TEXT("the slot still untouched"), W.Bytes(TEXT("slot_0")) == Second);
	W.Disk->FailWritesAfter(-1);

	// A broken slot never becomes the backup (Godot's rename put the broken file over the good backup).
	W.Disk->Put(TEXT("slot_0"), Garbage());
	W.P().Catnip = 4.0;
	W.Save->SaveGame();
	W.Save->Flush();
	TestTrue(TEXT("the good backup kept"), W.Bytes(TEXT("slot_0_bak")) == First);
	TestTrue(TEXT("and the slot written"), W.Save->LoadGame(0) == EPawLoadOutcome::Loaded);
	TestEqual(TEXT("the latest Catnip"), W.P().Catnip, 4.0);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveCorruptTest, "Pawpocalypse.Save.CorruptFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveCorruptTest::RunTest(const FString& Parameters)
{
	// The errors this test provokes on purpose (a refused write, a broken slot) are logged as errors in play.
	AddExpectedError(TEXT("is corrupt"), EAutomationExpectedErrorFlags::Contains, 2, false);
	AddExpectedError(TEXT("and its backup are unreadable"), EAutomationExpectedErrorFlags::Contains, 1, false);
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Play();
	W.Save->SaveGame(1);
	W.Save->Flush();
	const FString First = W.Snapshot();
	W.P().Catnip = 99.0;
	W.Save->SaveGame();
	W.Save->Flush();
	// The slot breaks on disk; the backup (the first save) loads, the broken file is kept aside.
	const TArray<uint8> Broken = Garbage();
	W.Disk->Put(TEXT("slot_1"), Broken);
	W.Game->NewGame();
	TestTrue(TEXT("loaded from the backup"), W.Save->LoadGame(1) == EPawLoadOutcome::LoadedFromBackup);
	TestTrue(TEXT("the broken file kept as evidence"), W.Bytes(TEXT("slot_1_corrupt")) == Broken);
	Pc::PcProgress& P = W.P();
	P.CurrentBiomeId = W.D().Tiers[1].Id;
	P.TierKillCount = 250;
	TestEqual(TEXT("the backup's game"), W.Snapshot(), First);
	// Both broken: nothing is applied, the game in memory stands.
	W.Disk->Put(TEXT("slot_1_bak"), Broken);
	W.Game->NewGame();
	const FString Fresh = W.Snapshot();
	TestTrue(TEXT("both broken: refused"), W.Save->LoadGame(1) == EPawLoadOutcome::Corrupt);
	TestEqual(TEXT("nothing applied"), W.Snapshot(), Fresh);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveCrashTest, "Pawpocalypse.Save.CrashRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveCrashTest::RunTest(const FString& Parameters)
{
	// The errors this test provokes on purpose (a refused write, a broken slot) are logged as errors in play.
	AddExpectedError(TEXT("is corrupt"), EAutomationExpectedErrorFlags::Contains, 1, false);
	AddExpectedError(TEXT("and its backup are unreadable"), EAutomationExpectedErrorFlags::Contains, 1, false);
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Play();
	W.Save->SaveGame(2);
	W.Save->Flush();
	const TArray<uint8> Old = W.Bytes(TEXT("slot_2"));
	W.P().Catnip = 7.0;
	W.Save->SaveGame();
	W.Save->Flush();
	const TArray<uint8> New = W.Bytes(TEXT("slot_2"));

	// 1. Crash after the temp, before the slot (the slot missing): the verified temp finishes the write.
	W.Disk->Delete(TEXT("slot_2"));
	W.Disk->Put(TEXT("slot_2_tmp"), New);
	W.Save->RecoverInterruptedSaves();
	TestTrue(TEXT("missing slot: restored from the temp"), W.Bytes(TEXT("slot_2")) == New);
	TestFalse(TEXT("the temp consumed"), W.Disk->Exists(TEXT("slot_2_tmp")));

	// 2. Crash mid-way through the slot (cut short), the temp good: from the temp; the broken one kept.
	TArray<uint8> Half(New.GetData(), New.Num() / 2);
	W.Disk->Put(TEXT("slot_2"), Half);
	W.Disk->Put(TEXT("slot_2_tmp"), New);
	W.Save->RecoverInterruptedSaves();
	TestTrue(TEXT("cut-short slot: restored from the temp"), W.Bytes(TEXT("slot_2")) == New);
	TestTrue(TEXT("the cut-short file kept"), W.Bytes(TEXT("slot_2_corrupt")) == Half);

	// 3. Crash while writing the temp (the temp cut short), the slot gone too: the backup.
	W.Disk->Delete(TEXT("slot_2"));
	W.Disk->Put(TEXT("slot_2_tmp"), Half);
	W.Save->RecoverInterruptedSaves();
	TestTrue(TEXT("bad temp: restored from the backup"), W.Bytes(TEXT("slot_2")) == Old);

	// 4. Crash after the slot, before the temp went: the slot stands, the stray temp goes.
	W.Disk->Put(TEXT("slot_2"), New);
	W.Disk->Put(TEXT("slot_2_tmp"), New);
	W.Save->RecoverInterruptedSaves();
	TestTrue(TEXT("a good slot is never replaced"), W.Bytes(TEXT("slot_2")) == New);
	TestFalse(TEXT("the stray temp removed"), W.Disk->Exists(TEXT("slot_2_tmp")));

	// 4b. A good slot beside an OLDER backup and no temp (the normal state after any save): untouched, nothing set aside.
	W.Disk->Delete(TEXT("slot_2_corrupt"));
	W.Save->RecoverInterruptedSaves();
	TestTrue(TEXT("a good slot never rolls back to its backup"), W.Bytes(TEXT("slot_2")) == New);
	TestFalse(TEXT("and is not called corrupt"), W.Disk->Exists(TEXT("slot_2_corrupt")));

	// 5. Nothing to recover from: left alone (the load then reports it).
	W.Disk->Put(TEXT("slot_4"), Half);
	W.Save->RecoverInterruptedSaves();
	TestTrue(TEXT("nothing better: left as it is"), W.Bytes(TEXT("slot_4")) == Half);
	TestTrue(TEXT("and the load refuses it"), W.Save->LoadGame(4) == EPawLoadOutcome::Corrupt);

	// Deleting a slot takes its backup and temp too, or recovery would bring it back.
	W.Disk->Put(TEXT("slot_2_tmp"), New);
	W.Save->DeleteSlot(2);
	W.Save->RecoverInterruptedSaves();
	TestFalse(TEXT("a deleted slot stays deleted"), W.Save->HasSlot(2));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveMomentsTest, "Pawpocalypse.Save.Moments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveMomentsTest::RunTest(const FString& Parameters)
{
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSaveSubsystem* S = W.Save;
	auto Writes = [S]() { S->Flush(); return S->GetWritesDone(); };
	S->Tick(0.0f);
	TestEqual(TEXT("nothing happened: nothing written"), Writes(), 0);

	// A bulk buy: 54 levels in one frame are ONE write (Godot: 137 ms a press before coalescing).
	const std::string Line = W.D().Upgrades.front().Id;
	for (int32 I = 0; I < 54; ++I)
	{
		W.P().UpgradeLevels[Line] += 1;
	}
	S->Tick(0.0f);
	TestEqual(TEXT("a bulk buy is one write"), Writes(), 1);
	S->Tick(0.0f);
	TestEqual(TEXT("and nothing the next frame"), Writes(), 1);

	// Each moment Godot saves on, one at a time.
	struct FMoment { const TCHAR* Name; TFunction<void(Pc::PcProgress&)> Do; };
	const Pc::PcGameData& D = W.D();
	const TArray<FMoment> Moments = {
		{ TEXT("a shop level"), [&D](Pc::PcProgress& P) { P.UpgradeLevels[D.Upgrades.back().Id] += 1; } },
		{ TEXT("a tree level"), [&D](Pc::PcProgress& P) { P.BranchLevels[D.Branches.back().Id] += 1; } },
		{ TEXT("a boss beaten"), [&D](Pc::PcProgress& P) { P.BossesClearedThisCycle.insert(D.Code.BossRealms.back()); } },
		{ TEXT("a job"), [&D](Pc::PcProgress& P) { P.JobsHeld.insert(D.Jobs.back().Id); } },
		{ TEXT("a claim"), [&D](Pc::PcProgress& P) { P.AchievementsClaimed.insert(D.Achievements.back().Id); } },
		{ TEXT("a card taken"), [](Pc::PcProgress& P) { P.CardsTakenThisRun += 1; } },
		{ TEXT("a Reawaken"), [](Pc::PcProgress& P) { P.ReawakenCount += 1; } },
	};
	int32 Expect = 1;
	for (const FMoment& M : Moments)
	{
		M.Do(W.P());
		S->Tick(0.0f);
		TestEqual(FString::Printf(TEXT("%s saves"), M.Name), Writes(), ++Expect);
	}
	// Kills and Catnip are the autosave's: they never write on their own.
	W.P().Catnip += 1e6;
	W.P().LifetimeKills += 500;
	W.P().TierKillCount += 500;
	S->Tick(0.0f);
	TestEqual(TEXT("kills and Catnip wait for the autosave"), Writes(), Expect);
	// An explicit save (the pause menu's Save, a new game's slot) supersedes the frame's: one write, not two.
	W.P().UpgradeLevels[Line] += 1;
	S->SaveGame();
	S->Tick(0.0f);
	TestEqual(TEXT("an explicit save is the frame's save"), Writes(), ++Expect);
	// RequestSave: once at the end of the frame however often asked.
	S->RequestSave();
	S->RequestSave();
	S->Tick(0.0f);
	TestEqual(TEXT("requests coalesce"), Writes(), ++Expect);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveAutosaveTest, "Pawpocalypse.Save.Autosave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveAutosaveTest::RunTest(const FString& Parameters)
{
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSaveSubsystem* S = W.Save;
	auto Writes = [S]() { S->Flush(); return S->GetWritesDone(); };
	UPawRealmSubsystem* R = W.World ? W.World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("the realm subsystem"), R))
	{
		W.Close();
		return false;
	}
	// The title (no realm running) writes nothing unprompted, a game worth keeping loaded or not: nothing changes there.
	// Review CR21 / F48 (2026-11-10): Godot's timer ran on the title and rewrote the loaded slot every 30 s.
	W.P().LifetimeCatnipEver = 10.0;
	S->Tick(31.0f);
	TestEqual(TEXT("the title: 30 s write nothing"), Writes(), 0);
	S->NoteFocusLostForTest();
	TestEqual(TEXT("the title: a focus loss writes nothing"), Writes(), 0);
	W.P().LifetimeCatnipEver = 0.0;
	// In play the clock runs. A fresh game has nothing to keep: the 30 s pass and nothing is written (no empty slot for
	// Continue to offer).
	R->SetViewSize(1920.0, 1080.0);
	TestTrue(TEXT("the realm starts"), R->StartRealm(7));
	S->Tick(31.0f);
	TestEqual(TEXT("an idle fresh game writes nothing"), Writes(), 0);
	TestFalse(TEXT("no slot appears"), S->HasAnySave());
	// Catnip earned: the next 30 s write once.
	W.P().LifetimeCatnipEver = 10.0;
	S->Tick(29.0f);
	TestEqual(TEXT("not before 30 s"), Writes(), 0);
	S->Tick(1.5f);
	TestEqual(TEXT("at 30 s"), Writes(), 1);
	// Paused (a screen holds the field): the clock stops.
	R->SetPaused(true);
	const double Was = S->GetAutosaveClock();
	S->Tick(100.0f);
	TestEqual(TEXT("paused: the clock holds"), S->GetAutosaveClock(), Was);
	TestEqual(TEXT("paused: nothing written"), Writes(), 1);
	R->SetPaused(false);
	// Quit: Deinitialize saves (synchronously) a game worth keeping.
	S->Autosave();
	TestEqual(TEXT("the quit save"), Writes(), 2);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveVersionsTest, "Pawpocalypse.Save.Versions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSaveVersionsTest::RunTest(const FString& Parameters)
{
	// The errors this test provokes on purpose (a refused write, a broken slot) are logged as errors in play.
	AddExpectedError(TEXT("the slot holds a newer build"), EAutomationExpectedErrorFlags::Contains, 1, false);
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	// A newer build's file: never read, never overwritten.
	W.Play();
	Pc::PcProgress Copy = W.P();
	std::string Newer = Pc::WriteSaveFile(Pc::CaptureSave(W.D(), Copy, W.Game->Slot(), 1.0), "future");
	Newer.replace(0, 9, "PAWSAVE 7");
	const TArray<uint8> NewerBytes(reinterpret_cast<const uint8*>(Newer.data()), static_cast<int32>(Newer.size()));
	W.Disk->Put(TEXT("slot_0"), NewerBytes);
	W.Game->NewGame();
	TestTrue(TEXT("a newer build's save is not loaded"), W.Save->LoadGame(0) == EPawLoadOutcome::NewerBuild);
	TestEqual(TEXT("nothing applied"), W.P().ReawakenCount, static_cast<int64>(0));
	W.P().LifetimeCatnipEver = 5.0;
	W.Save->SaveGame(0);
	W.Save->Flush();
	TestTrue(TEXT("and never overwritten"), W.Bytes(TEXT("slot_0")) == NewerBytes);
	TestEqual(TEXT("the write refused"), W.Save->GetWritesFailed(), 1);

	// An old version (v5): Godot's reset path, and a save written at once in the current version.
	W.Play();
	Copy = W.P();
	Pc::PcSaveState Old = Pc::CaptureSave(W.D(), Copy, W.Game->Slot(), 1.0);
	Old.Version = 5;
	const std::string OldFile = Pc::WriteSaveFile(Old, "old");
	W.Disk->Put(TEXT("slot_1"), TArray<uint8>(reinterpret_cast<const uint8*>(OldFile.data()), static_cast<int32>(OldFile.size())));
	W.Game->NewGame();
	const int32 Before = W.Save->GetWritesDone();
	TestTrue(TEXT("an old version loads"), W.Save->LoadGame(1) == EPawLoadOutcome::Loaded);
	TestEqual(TEXT("its progression reset"), W.P().ReawakenCount, static_cast<int64>(0));
	TestEqual(TEXT("its Catnip kept"), W.P().Catnip, Old.Catnip);
	W.Save->Flush();
	TestEqual(TEXT("and written back at once"), W.Save->GetWritesDone(), Before + 1);
	Pc::PcSaveState Back;
	const TArray<uint8> Written = W.Bytes(TEXT("slot_1"));
	TestEqual(TEXT("in the current version"),
		Pc::ReadSaveFile(std::string(reinterpret_cast<const char*>(Written.GetData()), Written.Num()), Back) == Pc::PcSaveReadError::None
			? Back.Version : int64(-1), Pc::PcGodotSaveVersion);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSaveMidFightTest, "Pawpocalypse.Save.MidFight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// A save while a boss is up (doc 11 phase 4: busy-state saves). Godot keeps no fight in the save: what is written is the
// progress (the kills banked in the biome), never "beaten". Loaded, the boss is not on the ledger and nothing is up. Godot
// kept the Bait paid for the fight spent (finding F36); the port writes the save as if the summon had not happened, the
// Bait and the Catnip it cost given back (D16, the designer's call, 2026-11-11).
bool FPawSaveMidFightTest::RunTest(const FString& Parameters)
{
	FSaveWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawRealmSubsystem* Realm = W.World->GetSubsystem<UPawRealmSubsystem>();
	const Pc::PcGameData& D = W.D();
	const std::string BossRealm = D.Code.BossRealms.front();
	const int32 Index = static_cast<int32>(Pc::RealmIndex(D, BossRealm));
	const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(Index)].BossData);
	if (!TestNotNull(TEXT("the realm"), Realm) || !TestNotNull(TEXT("the first boss"), Boss))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	Realm->SetViewSize(1920.0, 1080.0);
	Pc::CheatUnlockAllTiers(D, P); // the ladder open (the cheat marks every boss beaten) ...
	P.BossesDefeated.erase(BossRealm); // ... but this one: its fight is the point
	TestTrue(TEXT("the realm starts"), Realm->StartRealm(7));
	TestTrue(TEXT("to the boss biome"), Realm->Travel(Index));
	const std::string BiomeId = D.Tiers[static_cast<size_t>(Index)].Id;
	// An earlier fight here was lost on time: its Bait stays spent (only the fight in progress is given back).
	P.BaitSpentByBiome[BiomeId] = Boss->BaitRequired;
	P.BaitKillsByBiome[BiomeId] += Boss->BaitRequired * Boss->KillsPerBait; // the kills that paid for it (Bait held never reads below 0)
	P.BaitKillsByBiome[BiomeId] += Pc::BaitShortfall(D, P, *Boss) * Boss->KillsPerBait; // the Bait bank (D17 E2)
	const double Cost = Pc::SummonCatnipCost(D, P, *Boss);
	P.Catnip += Cost;
	TestTrue(TEXT("the summon costs Catnip too (both halves of F36 are seen)"), Cost > 0.0 && Boss->BaitRequired > 0);
	if (!TestTrue(TEXT("the boss is summoned"), Realm->SummonBoss()))
	{
		W.Close();
		return false;
	}
	Realm->AdvanceFrame(1.0 / 60.0);
	TestTrue(TEXT("the fight is on"), Realm->GetSim().Field.bBossUp);
	const double CatnipAtSave = P.Catnip;
	W.Save->SaveGame(2);
	W.Save->Flush();
	TestEqual(TEXT("the game in memory paid for both fights and fights on"), P.BaitSpentByBiome[BiomeId], 2 * Boss->BaitRequired);
	const int64 Banked = P.TierKillCount;
	TestFalse(TEXT("not beaten"), Pc::IsBossDefeated(P, BossRealm));
	// The next session: a new game in memory, the slot loaded.
	W.Game->NewGame();
	TestTrue(TEXT("the mid-fight save loads"), W.Save->LoadGame(2) == EPawLoadOutcome::Loaded);
	TestFalse(TEXT("the boss is not on the ledger"), Pc::IsBossDefeated(W.P(), BossRealm));
	TestFalse(TEXT("nor cleared this cycle"), W.P().BossesClearedThisCycle.count(BossRealm) > 0);
	TestEqual(TEXT("the biome's kills are banked"), W.P().TierKillsByBiome[D.Tiers[static_cast<size_t>(Index)].Id], Banked);
	TestTrue(TEXT("the realm starts from it"), Realm->StartRealm(7));
	TestTrue(TEXT("back in the boss biome"), Realm->Travel(Index));
	TestFalse(TEXT("no boss up after a load"), Realm->GetSim().Field.bBossUp);
	// F36 (D16; designer 2026-11-11, "Refund Bait and Catnip"): the fight is not saved, so neither is what its summon cost.
	// Godot kept the Bait spent, so the whole Bait was owed again before the boss came out.
	TestEqual(TEXT("the Bait paid for the fight is back: none owed"), Pc::BaitShortfall(D, W.P(), *Boss), static_cast<int64_t>(0));
	TestEqual(TEXT("the earlier, lost fight's Bait stays spent"), W.P().BaitSpentByBiome[BiomeId], Boss->BaitRequired);
	TestEqual(TEXT("and the Catnip it cost is back"), W.P().Catnip, CatnipAtSave + Cost);
	TestTrue(TEXT("so the boss can be summoned again at once"), Realm->SummonBoss());
	W.Close();
	return true;
}

#endif
