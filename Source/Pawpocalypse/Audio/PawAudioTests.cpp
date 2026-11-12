// Pawpocalypse.Audio.*: the sound (phase 6 slice 4). The bank against Data/Audio*.csv and the prune both ways; AudioManager's
// rules (the rate limits, round robin, the pitch spread, the buses, the music's crossfade and its carrying on); what plays
// when (the progress observer, the field's events, the biome's music), as Godot's hookups make them.

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Audio/PawAudioBank.h"
#include "Audio/PawAudioSubsystem.h"
#include "AudioDeviceNotificationSubsystem.h"
#include "Components/AudioComponent.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcField.h"
#include "PcStats.h"
#include "Save/PawSettingsSubsystem.h"
#include "Sound/SoundWave.h"

namespace
{
	struct FAudioWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawAudioSubsystem* Audio = nullptr;
		UPawGameSubsystem* Game = nullptr;
		double Clock = 100.0;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Audio = GI->GetSubsystem<UPawAudioSubsystem>();
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			if (!T.TestNotNull(TEXT("the audio subsystem"), Audio) || !T.TestTrue(TEXT("data"), Game && Game->IsDataReady()))
			{
				return false;
			}
			Audio->Now = [this]() { return Clock; };
			Audio->TickOnce(0.0f); // the observer's first look: a baseline, no sound
			Audio->ClearLog();
			return true;
		}
		void Close()
		{
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
		int32 Count(const TCHAR* Event) const
		{
			int32 N = 0;
			for (const UPawAudioSubsystem::FPlayed& P : Audio->Played())
			{
				N += P.Event == Event ? 1 : 0;
			}
			return N;
		}
	};

	TArray<TArray<FString>> ReadCsv(const FString& Name)
	{
		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectDir() / TEXT("Data") / Name));
		TArray<TArray<FString>> Rows;
		for (int32 I = 1; I < Lines.Num(); ++I)
		{
			TArray<FString> Cols;
			Lines[I].ParseIntoArray(Cols, TEXT(","), false);
			Rows.Add(Cols);
		}
		return Rows;
	}

	FString AssetNameFor(const FString& File)
	{
		// tools/import_audio.py asset_name(): SW_ + the stem with every run of other characters as one underscore.
		FString Stem = FPaths::GetBaseFilename(File), Out;
		bool bGap = false;
		for (const TCHAR C : Stem)
		{
			if (FChar::IsAlnum(C))
			{
				if (bGap && !Out.IsEmpty())
				{
					Out.AppendChar(TEXT('_'));
				}
				Out.AppendChar(C);
				bGap = false;
			}
			else
			{
				bGap = true;
			}
		}
		return TEXT("SW_") + Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawAudioBankTest, "Pawpocalypse.Audio.Bank", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawAudioBankTest::RunTest(const FString& Parameters)
{
	const UPawAudioBank* Bank = LoadObject<UPawAudioBank>(nullptr, TEXT("/Game/Pawpocalypse/Audio/DA_AudioBank.DA_AudioBank"));
	if (!TestNotNull(TEXT("DA_AudioBank (py tools/import_audio.py --run)"), Bank))
	{
		return false;
	}
	TSet<FString> Played;
	const TArray<TArray<FString>> Events = ReadCsv(TEXT("AudioEvents.csv"));
	TestEqual(TEXT("one bank event per CSV row"), Bank->Events.Num(), Events.Num());
	for (const TArray<FString>& R : Events)
	{
		const FPawAudioEvent* E = R.Num() >= 5 ? Bank->FindEvent(R[0]) : nullptr;
		if (!TestNotNull(*FString::Printf(TEXT("the bank has %s"), R.Num() ? *R[0] : TEXT("?")), E))
		{
			continue;
		}
		TestEqual(*(R[0] + TEXT(": min interval")), E->MinInterval, FCString::Atof(*R[1]));
		TestEqual(*(R[0] + TEXT(": pitch")), E->Pitch, FCString::Atof(*R[2]));
		TestEqual(*(R[0] + TEXT(": volume")), E->VolumeDb, FCString::Atof(*R[3]));
		TArray<FString> Files;
		R[4].ParseIntoArray(Files, TEXT(";"));
		if (TestEqual(*(R[0] + TEXT(": its sounds")), E->Sounds.Num(), Files.Num()))
		{
			for (int32 I = 0; I < Files.Num(); ++I)
			{
				const USoundWave* W = E->Sounds[I];
				TestTrue(*(R[0] + TEXT(": in Godot's order, each its file")), W && W->GetName() == AssetNameFor(Files[I]) && !W->bLooping);
				if (W)
				{
					Played.Add(W->GetPathName());
				}
			}
		}
	}
	const TArray<TArray<FString>> Tracks = ReadCsv(TEXT("AudioMusic.csv"));
	TestEqual(TEXT("one bank track per CSV row"), Bank->Music.Num(), Tracks.Num());
	for (const TArray<FString>& R : Tracks)
	{
		const USoundWave* W = R.Num() >= 2 ? Bank->FindTrack(R[0]) : nullptr;
		TestTrue(*FString::Printf(TEXT("track %s: its file, looping"), R.Num() ? *R[0] : TEXT("?")),
			W && W->GetName() == AssetNameFor(R[1]) && W->bLooping);
		if (W)
		{
			Played.Add(W->GetPathName());
		}
	}
	// The prune, the other way: no sound in the game that nothing plays.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(TEXT("/Game/Pawpocalypse/Audio"), Assets, true);
	int32 Waves = 0;
	for (const FAssetData& A : Assets)
	{
		if (A.AssetClassPath == USoundWave::StaticClass()->GetClassPathName())
		{
			++Waves;
			TestTrue(*FString::Printf(TEXT("%s is played by an event or a track"), *A.AssetName.ToString()),
				Played.Contains(A.GetObjectPathString()));
		}
	}
	TestEqual(TEXT("as many sounds as are played"), Waves, Played.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawAudioRulesTest, "Pawpocalypse.Audio.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawAudioRulesTest::RunTest(const FString& Parameters)
{
	FAudioWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawAudioSubsystem* A = W.Audio;
	// The rate limit (mouse_killed: 0.11 s) and the round robin over its four drops.
	A->Play(TEXT("mouse_killed"));
	W.Clock += 0.05;
	A->Play(TEXT("mouse_killed"));
	TestEqual(TEXT("a second drop inside 0.11 s is swallowed"), W.Count(TEXT("mouse_killed")), 1);
	for (int32 I = 0; I < 4; ++I)
	{
		W.Clock += 0.2;
		A->Play(TEXT("mouse_killed"));
	}
	TArray<int32> Variants;
	for (const UPawAudioSubsystem::FPlayed& P : A->Played())
	{
		Variants.Add(P.Variant);
		TestTrue(TEXT("pitch inside 1 +- 0.16"), P.Pitch >= 0.84f && P.Pitch <= 1.16f);
		TestEqual(TEXT("its volume"), P.VolumeDb, -18.0f);
	}
	TestEqual(TEXT("round robin, wrapping"), Variants, TArray<int32>({ 0, 1, 2, 3, 0 }));
	A->ClearLog();
	A->Play(TEXT("boss_summon"));
	TestEqual(TEXT("no spread: pitch 1"), A->Played().Last().Pitch, 1.0f);
	A->Play(TEXT("boss_summon"));
	TestEqual(TEXT("no interval: both play"), W.Count(TEXT("boss_summon")), 2);
	A->ClearLog();
	A->Play(TEXT("no_such_event"));
	TestEqual(TEXT("an unknown event plays nothing"), A->Played().Num(), 0);

	// The buses: Master x Music / SFX, the mute box silencing all.
	UPawSettingsSubsystem* S = W.GI->GetSubsystem<UPawSettingsSubsystem>();
	if (TestNotNull(TEXT("settings"), S))
	{
		S->SetMasterVolume(0.5);
		S->SetMusicVolume(0.4);
		S->SetSfxVolume(0.8);
		TestEqual(TEXT("music gain"), A->MusicGain(), 0.2, 1e-9);
		TestEqual(TEXT("sfx gain"), A->SfxGain(), 0.4, 1e-9);
		S->SetMuted(true);
		TestEqual(TEXT("muted: silence"), A->MusicGain() + A->SfxGain(), 0.0);
		S->SetMuted(false);
	}

	// The music: a crossfade in dB over 1.2 s; the same track again nothing; a track with no file keeps what plays.
	A->PlayMusic(TEXT("menu"));
	TestEqual(TEXT("the title's music"), A->CurrentTrack(), FString(TEXT("menu")));
	W.Clock += 0.6;
	A->TickOnce(0.0f);
	TestEqual(TEXT("the first track fades in: half way, -30 dB"), A->IncomingDb(), -30.0, 1e-6);
	W.Clock += 0.7;
	A->TickOnce(0.0f);
	TestEqual(TEXT("in full"), A->IncomingDb(), 0.0, 1e-9);
	A->PlayMusic(TEXT("tier_1"));
	W.Clock += 0.3;
	A->TickOnce(0.0f);
	TestEqual(TEXT("crossfade: the new at -45 dB"), A->IncomingDb(), -45.0, 1e-6);
	TestEqual(TEXT("crossfade: the old at -15 dB"), A->OutgoingDb(), -15.0, 1e-6);
	A->PlayMusic(TEXT("tier_9"));
	TestEqual(TEXT("biome 9 has no track: the Backyard's carries on"), A->CurrentTrack(), FString(TEXT("tier_1")));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawAudioVoicesTest, "Pawpocalypse.Audio.Voices", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawAudioVoicesTest::RunTest(const FString& Parameters)
{
	// Review CR3 (2026-11-10): the sixteen voices and the two music players are made once and reused. Each was made again for
	// every sound: a component that persists across level changes belongs to the audio device, not a world, so its GetWorld()
	// is always null and the old check against the world never held. The 17th sound then never cut the oldest, and a looping
	// track replaced inside its crossfade played on until the garbage collector took its component. The players here are
	// made as the engine's are: in the transient package, in no world.
	FAudioWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawAudioSubsystem* A = W.Audio;
	int32 Made = 0;
	A->CreatePlayer = [&Made](USoundWave* Sound)
	{
		++Made;
		UAudioComponent* C = NewObject<UAudioComponent>(GetTransientPackage());
		C->SetSound(Sound);
		return C;
	};
	for (int32 I = 0; I < 40; ++I)
	{
		W.Clock += 1.0;
		A->Play(TEXT("boss_summon"));
	}
	TestEqual(TEXT("40 sounds on the 16 voices make 16 players"), Made, static_cast<int32>(UPawAudioSubsystem::Voices));
	const int32 Voiced = Made;
	for (const TCHAR* Track : { TEXT("menu"), TEXT("tier_1"), TEXT("menu"), TEXT("tier_1") })
	{
		A->PlayMusic(Track);
		W.Clock += 0.5; // inside the 1.2 s crossfade: the track going out is the one coming back
		A->TickOnce(0.0f);
	}
	TestEqual(TEXT("four tracks on the two music players make 2"), Made - Voiced, 2);
	// Review CR4: the play log (for these tests) kept every sound for as long as the game ran; it keeps the newest 256.
	A->ClearLog();
	for (int32 I = 0; I < 10000; ++I)
	{
		A->Play(TEXT("boss_summon"));
	}
	TestEqual(TEXT("10,000 sounds leave the newest 256 in the log"), A->Played().Num(), 256);
	TestEqual(TEXT("the newest is last"), A->Played().Last().Event, FString(TEXT("boss_summon")));
	A->CreatePlayer = nullptr;
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawAudioWhenTest, "Pawpocalypse.Audio.When", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawAudioWhenTest::RunTest(const FString& Parameters)
{
	FAudioWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawAudioSubsystem* A = W.Audio;
	Pc::PcProgress& P = W.Game->Progress();
	auto Step = [&W, A]() { W.Clock += 1.0; A->TickOnce(0.0f); };

	// The progress observer: Godot's signal hookups and the direct calls beside them. Godot played five events twice, layered;
	// here each plays once (F40, a Godot fault the port does not copy: D16).
	P.UpgradeLevels["paw_damage"] += 1;
	Step();
	TestEqual(TEXT("a purchase: one (the second inside 50 ms)"), W.Count(TEXT("purchase")), 1);
	P.ReawakenCount += 1;
	Step();
	TestEqual(TEXT("a Reawaken: once (F40)"), W.Count(TEXT("reawaken")), 1);
	P.BranchLevels["unlock_aura"] += 1;
	Step();
	TestEqual(TEXT("a skill level: once (F40)"), W.Count(TEXT("skill_purchase")), 1);
	P.Hand = { "a", "b", "c" };
	Step();
	TestEqual(TEXT("a new hand: once (F40)"), W.Count(TEXT("card_offered")), 1);
	P.Hand = { "d", "e", "f" };
	Step();
	TestEqual(TEXT("a reroll: once more"), W.Count(TEXT("card_offered")), 2);
	Step();
	TestEqual(TEXT("the same hand a frame later: nothing more (the hand seen is kept, review CR17)"), W.Count(TEXT("card_offered")), 2);
	P.Hand.clear();
	P.CardLevels["some_card"] = 1;
	Step();
	TestEqual(TEXT("a card taken: once (F40)"), W.Count(TEXT("card_taken")), 1);
	P.AchievementsEarned.insert("first_blood");
	Step();
	TestEqual(TEXT("an achievement: once"), W.Count(TEXT("achievement_earned")), 1);
	P.AchievementsClaimed.insert("first_blood");
	Step();
	TestEqual(TEXT("a claim: once"), W.Count(TEXT("achievement_claim")), 1);
	// A new game (or a load) replaces the progress: heard as nothing.
	A->ClearLog();
	W.Game->NewGame();
	P.ReawakenCount = 7;
	P.UpgradeLevels["paw_damage"] = 30;
	W.Game->MarkProgressReplaced();
	Step();
	TestEqual(TEXT("a replaced progress plays nothing"), A->Played().Num(), 0);

	// The field: a golden kill (its chime and the drop), the boss (summon and its music, phase, defeat twice).
	auto Ev = [](Pc::PcFieldEventKind K, uint32 Flags = 0)
	{
		Pc::PcFieldEvent E;
		E.Kind = K;
		E.Flags = Flags;
		return E;
	};
	A->SetRealmTier(TEXT("tier_1"));
	TestEqual(TEXT("the biome's music"), A->CurrentTrack(), FString(TEXT("tier_1")));
	A->OnFieldEvents({ Ev(Pc::PcFieldEventKind::Killed, Pc::FieldFlag::Golden) }, TEXT("tier_1"));
	TestEqual(TEXT("golden chime"), W.Count(TEXT("golden_caught")), 1);
	TestEqual(TEXT("the drop"), W.Count(TEXT("mouse_killed")), 1);
	A->OnFieldEvents({ Ev(Pc::PcFieldEventKind::BossSummoned) }, TEXT("tier_1"));
	TestEqual(TEXT("the summon's bong"), W.Count(TEXT("boss_summon")), 1);
	TestEqual(TEXT("the boss's music"), A->CurrentTrack(), FString(TEXT("boss")));
	W.Clock += 1.0;
	A->OnFieldEvents({ Ev(Pc::PcFieldEventKind::BossPhase), Ev(Pc::PcFieldEventKind::BossDefeated),
		Ev(Pc::PcFieldEventKind::BossTimeout) }, TEXT("tier_1"));
	TestEqual(TEXT("a phase"), W.Count(TEXT("boss_phase")), 1);
	TestEqual(TEXT("defeated: once (F40)"), W.Count(TEXT("boss_defeated")), 1);
	TestEqual(TEXT("a timeout leaves the boss's music on (as Godot)"), A->CurrentTrack(), FString(TEXT("boss")));
	A->OnBossDismissed(TEXT("tier_1"));
	TestEqual(TEXT("a Reawaken's dismissal brings the biome's back"), A->CurrentTrack(), FString(TEXT("tier_1")));
	A->SetRealmTier(TEXT("tier_2"));
	TestEqual(TEXT("travel: the next biome's"), A->CurrentTrack(), FString(TEXT("tier_2")));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawAudioOutputTest, "Pawpocalypse.Audio.Output", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawAudioOutputTest::RunTest(const FString& Parameters)
{
	// settings_manager.gd _apply_output_device: the stored device while the system lists it, else "Default"; the engine told
	// only on a change; run on every settings change and when the pinned device goes away.
	FAudioWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawAudioSubsystem* A = W.Audio;
	UPawSettingsSubsystem* S = W.GI->GetSubsystem<UPawSettingsSubsystem>();
	const FString Speakers = TEXT("Speakers (Realtek(R) Audio)"), Headset = TEXT("Headset Earphone (Wireless Controller)");
	TArray<FString> System = { Speakers, TEXT("Default"), Speakers };
	TArray<FString> Swaps;
	A->SystemDevices = [&System]() { return System; };
	A->SwapOutput = [&Swaps](const FString& Name) { Swaps.Add(Name); return true; };
	TestEqual(TEXT("Default first, each device once"), A->OutputDeviceList(), (TArray<FString>{ TEXT("Default"), Speakers }));
	System = { Speakers };
	TestEqual(TEXT("at startup: following the system"), A->GetAppliedOutputDevice(), FString(TEXT("Default")));
	A->ApplyOutputDevice();
	TestEqual(TEXT("Default stored: nothing to tell"), Swaps.Num(), 0);
	S->SetOutputDevice(Speakers);
	TestEqual(TEXT("a device chosen: swapped to it"), Swaps, TArray<FString>{ Speakers });
	TestEqual(TEXT("and pinned"), A->GetAppliedOutputDevice(), Speakers);
	S->SetMasterVolume(0.5);
	TestEqual(TEXT("apply_audio again: nothing new"), Swaps.Num(), 1);
	// A pad brings a headset: the list grows, the chosen device keeps the sound.
	System.Add(Headset);
	S->SetMusicVolume(0.4);
	TestEqual(TEXT("a new device takes nothing from a chosen one"), A->GetAppliedOutputDevice(), Speakers);
	// The chosen device goes away (the system says so): the system's, never silence; the setting is kept.
	System = { Headset };
	UAudioDeviceNotificationSubsystem* Notify = UAudioDeviceNotificationSubsystem::Get();
	if (TestNotNull(TEXT("the engine's device notices"), Notify))
	{
		Notify->DeviceRemovedNative.Broadcast(TEXT("{paw-test-no-such-device}"));
		TestEqual(TEXT("gone: back to the system's"), A->GetAppliedOutputDevice(), FString(TEXT("Default")));
		TestEqual(TEXT("swapped once more"), Swaps, (TArray<FString>{ Speakers, TEXT("Default") }));
		Notify->DeviceRemovedNative.Broadcast(TEXT("{paw-test-no-such-device}"));
		TestEqual(TEXT("following the system already: a removal tells nothing"), Swaps.Num(), 2);
	}
	TestEqual(TEXT("the setting kept"), FString(UTF8_TO_TCHAR(S->Get().OutputDevice.c_str())), Speakers);
	// Back again: the next apply_audio finds it (Godot: any audio setting's change, or a restart).
	System.Add(Speakers);
	S->SetSfxVolume(0.9);
	TestEqual(TEXT("plugged back in: found on the next apply"), A->GetAppliedOutputDevice(), Speakers);
	S->SetOutputDevice(TEXT("Default"));
	TestEqual(TEXT("Follow the system"), A->GetAppliedOutputDevice(), FString(TEXT("Default")));
	TestEqual(TEXT("four swaps in all"), Swaps.Num(), 4);
	// A swap the engine refused leaves what it was told as it was (tried again on the next apply).
	A->SwapOutput = [](const FString&) { return false; };
	S->SetOutputDevice(Speakers);
	TestEqual(TEXT("refused: still following the system"), A->GetAppliedOutputDevice(), FString(TEXT("Default")));
	// The engine's own swap: never in the editor (the designer's device).
	A->SwapOutput = nullptr;
	A->ApplyOutputDevice();
	TestEqual(TEXT("in the editor the engine is never told"), A->GetAppliedOutputDevice(), FString(TEXT("Default")));
	A->SystemDevices = nullptr;
	const TArray<FString> Engine = A->OutputDeviceList();
	TestTrue(TEXT("the engine's list begins with Default too"), Engine.Num() > 0 && Engine[0] == TEXT("Default"));
	S->SetOutputDevice(TEXT("Default"));
	W.Close();
	return true;
}

#endif
