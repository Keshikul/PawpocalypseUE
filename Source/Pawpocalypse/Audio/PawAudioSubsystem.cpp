#include "Audio/PawAudioSubsystem.h"

#include "Audio/PawAudioBank.h"
#include "AudioDevice.h"
#include "AudioDeviceNotificationSubsystem.h"
#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "AudioThread.h"
#include "Components/AudioComponent.h"
#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "PcField.h"
#include "PcSettings.h"
#include "PcStats.h"
#include "Save/PawSettingsSubsystem.h"
#include "Sound/SoundWave.h"

namespace
{
	const TCHAR* BankPath = TEXT("/Game/Pawpocalypse/Audio/DA_AudioBank.DA_AudioBank");

	double DbToAmp(double Db)
	{
		return FMath::Pow(10.0, Db / 20.0);
	}

	const TCHAR* FollowTheSystem = TEXT("Default");

	// The mixer's platform (WASAPI on Windows) behind the main audio device, or null (no device: a headless run).
	Audio::IAudioMixerPlatformInterface* MixerPlatform()
	{
		FAudioDevice* Device = GEngine ? GEngine->GetMainAudioDeviceRaw() : nullptr;
		return Device ? static_cast<Audio::FMixerDevice*>(Device)->GetAudioMixerPlatform() : nullptr;
	}

	// The system's active output devices (the device cache is read-locked: safe on the game thread).
	TArray<Audio::FAudioPlatformDeviceInfo> ActiveDevices()
	{
		Audio::IAudioMixerPlatformInterface* Platform = MixerPlatform();
		Audio::IAudioPlatformDeviceInfoCache* Cache = Platform ? Platform->GetDeviceInfoCache() : nullptr;
		return Cache ? Cache->GetAllActiveOutputDevices() : TArray<Audio::FAudioPlatformDeviceInfo>();
	}

	TArray<FString> EngineDeviceNames()
	{
		TArray<FString> Names;
		for (const Audio::FAudioPlatformDeviceInfo& D : ActiveDevices())
		{
			Names.AddUnique(D.Name); // by name, as Godot stores it (a second device of the same name is the first)
		}
		return Names;
	}

	// AudioServer.output_device = Name: "Default" follows the system's default from now on; a device is pinned (the mixer
	// stops listening for default changes, which it would otherwise follow even after a swap). Never in the editor.
	bool EngineSwapTo(const FString& Name)
	{
		if (GIsEditor || !MixerPlatform())
		{
			return false;
		}
		FString Id;
		const bool bFollow = Name == FollowTheSystem;
		if (!bFollow)
		{
			for (const Audio::FAudioPlatformDeviceInfo& D : ActiveDevices())
			{
				if (D.Name == Name)
				{
					Id = D.DeviceId;
					break;
				}
			}
			if (Id.IsEmpty())
			{
				return false;
			}
		}
		// On the audio thread, as UAudioMixerBlueprintLibrary::SwapAudioOutputDevice does; forced, so the player's last pick
		// replaces a swap still in flight. An empty id is the system's default.
		FAudioThread::RunCommandOnAudioThread([Id, bFollow]()
		{
			if (Audio::IAudioMixerPlatformInterface* Platform = MixerPlatform())
			{
				Platform->SetIsListeningForDeviceEvents(bFollow);
				Platform->RequestDeviceSwap(Id, /*bInForce*/ true, TEXT("Pawpocalypse: Settings > Audio > Output"));
			}
		});
		return true;
	}
}

UPawAudioSubsystem* UPawAudioSubsystem::Of(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UPawAudioSubsystem>() : nullptr;
}

void UPawAudioSubsystem::PlayFrom(const UObject* WorldContext, const TCHAR* Event)
{
	if (UPawAudioSubsystem* A = Of(WorldContext))
	{
		A->Play(Event);
	}
}

void UPawAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UPawSettingsSubsystem>();
	Collection.InitializeDependency<UPawGameSubsystem>();
	Bank = LoadObject<UPawAudioBank>(nullptr, BankPath);
	UE_CLOG(!Bank, LogPaw, Error, TEXT("Audio: no %s (py tools/import_audio.py --run)"), BankPath);
	Now = []() { return FPlatformTime::Seconds(); };
	VoicePlayers.SetNum(Voices);
	VoiceDb.Init(0.0f, Voices);
	// apply_audio() at startup and on every change of the settings; and when the pinned device goes away.
	if (UPawSettingsSubsystem* S = GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>())
	{
		SettingsHandle = S->OnChanged.AddUObject(this, &UPawAudioSubsystem::ApplyOutputDevice);
	}
	if (UAudioDeviceNotificationSubsystem* Notify = GEngine ? UAudioDeviceNotificationSubsystem::Get() : nullptr)
	{
		DeviceRemovedHandle = Notify->DeviceRemovedNative.AddWeakLambda(this, [this](FString)
		{
			if (AppliedOutput != FollowTheSystem && !OutputDeviceList().Contains(AppliedOutput))
			{
				ApplyOutputDevice();
			}
		});
	}
	ApplyOutputDevice();
}

void UPawAudioSubsystem::Deinitialize()
{
	if (UPawSettingsSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr)
	{
		S->OnChanged.Remove(SettingsHandle);
	}
	if (UAudioDeviceNotificationSubsystem* Notify = GEngine ? UAudioDeviceNotificationSubsystem::Get() : nullptr)
	{
		Notify->DeviceRemovedNative.Remove(DeviceRemovedHandle);
	}
	for (UAudioComponent* V : VoicePlayers)
	{
		if (IsValid(V))
		{
			V->Stop();
		}
	}
	for (UAudioComponent* M : MusicPlayers)
	{
		if (IsValid(M))
		{
			M->Stop();
		}
	}
	Super::Deinitialize();
}

double UPawAudioSubsystem::MusicGain() const
{
	const UPawSettingsSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	if (!S)
	{
		return 1.0;
	}
	// _apply_bus: a muted or zero Master silences all; each bus a linear amplitude.
	const auto& P = S->Get();
	return P.bMuted ? 0.0 : P.MasterVolume * P.MusicVolume;
}

double UPawAudioSubsystem::SfxGain() const
{
	const UPawSettingsSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	if (!S)
	{
		return 1.0;
	}
	const auto& P = S->Get();
	return P.bMuted ? 0.0 : P.MasterVolume * P.SfxVolume;
}

TArray<FString> UPawAudioSubsystem::OutputDeviceList() const
{
	TArray<FString> List = { FollowTheSystem };
	for (const FString& Name : SystemDevices ? SystemDevices() : EngineDeviceNames())
	{
		if (Name != FollowTheSystem)
		{
			List.AddUnique(Name);
		}
	}
	return List;
}

void UPawAudioSubsystem::ApplyOutputDevice()
{
	const UPawSettingsSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	if (!S)
	{
		return;
	}
	std::vector<std::string> System;
	for (const FString& Name : OutputDeviceList())
	{
		System.push_back(TCHAR_TO_UTF8(*Name));
	}
	const FString Want = UTF8_TO_TCHAR(Pc::SettingsOutputWant(System, S->Get()).c_str());
	if (Want == AppliedOutput)
	{
		return;
	}
	if (SwapOutput ? SwapOutput(Want) : EngineSwapTo(Want))
	{
		UE_LOG(LogPaw, Log, TEXT("Audio: output device '%s' (was '%s')"), *Want, *AppliedOutput);
		AppliedOutput = Want;
		++OutputSwaps;
	}
}

UAudioComponent* UPawAudioSubsystem::MakePlayer(USoundWave* Sound)
{
	if (CreatePlayer)
	{
		return CreatePlayer(Sound);
	}
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	// Persistent across a level change, as the Godot autoload's players are.
	return World ? UGameplayStatics::CreateSound2D(World, Sound, 1.0f, 1.0f, 0.0f, nullptr, true, false) : nullptr;
}

// Each voice and music player is made ONCE and reused while it is valid (review CR3). A component made to persist across
// level changes belongs to the audio device, not a world: its GetWorld() is always null, and the old check against the world
// made a new component for every sound (the 17th never cut the oldest; a looping track replaced inside its crossfade played
// on until the garbage collector took its component).
UAudioComponent* UPawAudioSubsystem::Voice(int32 Index, USoundWave* Sound)
{
	TObjectPtr<UAudioComponent>& V = VoicePlayers[Index];
	if (!IsValid(V))
	{
		V = MakePlayer(Sound);
	}
	else
	{
		V->SetSound(Sound);
	}
	return V;
}

UAudioComponent* UPawAudioSubsystem::MusicPlayer(int32 Index, USoundWave* Sound)
{
	TObjectPtr<UAudioComponent>& M = MusicPlayers[Index];
	if (!IsValid(M))
	{
		M = MakePlayer(Sound);
	}
	else if (Sound)
	{
		M->SetSound(Sound);
	}
	return M;
}

void UPawAudioSubsystem::Play(const FString& Event)
{
	const FPawAudioEvent* E = Bank ? Bank->FindEvent(Event) : nullptr;
	if (!E)
	{
		UE_LOG(LogPaw, Warning, TEXT("Audio: no rules for event '%s'"), *Event);
		return;
	}
	if (E->Sounds.Num() == 0)
	{
		return;
	}
	// The rate limit, on real time.
	const double T = Now();
	double& Last = LastPlayed.FindOrAdd(Event, -1.0e9);
	if (T - Last < E->MinInterval)
	{
		return;
	}
	Last = T;
	int32& Next = NextVariant.FindOrAdd(Event, 0);
	const int32 Index = Next % E->Sounds.Num();
	Next = Index + 1;
	const float Pitch = FMath::IsNearlyZero(E->Pitch) ? 1.0f : FMath::FRandRange(1.0f - E->Pitch, 1.0f + E->Pitch);
	// The newest MaxLog only (review CR4: the log grew for as long as the game ran, Shipping too).
	if (Log.Num() >= MaxLog)
	{
		Log.RemoveAt(0, Log.Num() - MaxLog + 1, EAllowShrinking::No);
	}
	Log.Add({ Event, Index, Pitch, E->VolumeDb });
	// _take_voice: the voices in turn; the 17th sound cuts the oldest.
	const int32 V = NextVoice % Voices;
	++NextVoice;
	VoiceDb[V] = E->VolumeDb;
	if (UAudioComponent* C = Voice(V, E->Sounds[Index]))
	{
		C->Stop();
		C->SetPitchMultiplier(Pitch);
		C->SetVolumeMultiplier(static_cast<float>(DbToAmp(E->VolumeDb) * SfxGain()));
		C->Play();
	}
}

bool UPawAudioSubsystem::HasMusic(const FString& Track) const
{
	return Bank && Bank->FindTrack(Track) != nullptr;
}

void UPawAudioSubsystem::PlayMusic(const FString& Track)
{
	if (Track == MusicTrack)
	{
		return;
	}
	USoundWave* Sound = Bank ? Bank->FindTrack(Track) : nullptr;
	if (!Sound)
	{
		return; // no file for this track: whatever plays keeps playing
	}
	// outgoing.playing: a track was on (and not stopped); kept by the track, not the player, so it holds without an audio
	// device too (a headless run).
	bOutgoingPlaying = !MusicTrack.IsEmpty();
	MusicTrack = Track;
	const int32 Incoming = 1 - ActiveMusic;
	ActiveMusic = Incoming;
	FadeStart = Now();
	FadeInDb = SilentDb;
	FadeOutDb = bOutgoingPlaying ? 0.0 : SilentDb;
	if (UAudioComponent* In = MusicPlayer(Incoming, Sound))
	{
		In->Stop();
		In->SetVolumeMultiplier(static_cast<float>(DbToAmp(SilentDb) * MusicGain()));
		In->Play();
	}
}

void UPawAudioSubsystem::StopMusic()
{
	MusicTrack.Reset();
	FadeStart = -1.0;
	for (UAudioComponent* M : MusicPlayers)
	{
		if (IsValid(M))
		{
			M->Stop();
		}
	}
}

void UPawAudioSubsystem::ApplyVolumes()
{
	const double Sfx = SfxGain();
	for (int32 I = 0; I < Voices; ++I)
	{
		if (UAudioComponent* V = VoicePlayers[I]; IsValid(V) && V->IsPlaying())
		{
			V->SetVolumeMultiplier(static_cast<float>(DbToAmp(VoiceDb[I]) * Sfx));
		}
	}
	const double Music = MusicGain();
	if (UAudioComponent* In = MusicPlayers[ActiveMusic]; IsValid(In))
	{
		In->SetVolumeMultiplier(static_cast<float>(DbToAmp(FadeInDb) * Music));
	}
	if (UAudioComponent* Out = MusicPlayers[1 - ActiveMusic]; IsValid(Out) && Out->IsPlaying())
	{
		Out->SetVolumeMultiplier(static_cast<float>(DbToAmp(FadeOutDb) * Music));
	}
}

void UPawAudioSubsystem::Tick(float DeltaSeconds)
{
	// The crossfade: each player's dB moves linearly over MUSIC_FADE_SEC (a tween of volume_db), the outgoing stopped at its end.
	if (FadeStart >= 0.0)
	{
		const double T = FMath::Clamp((Now() - FadeStart) / MusicFadeSec, 0.0, 1.0);
		FadeInDb = FMath::Lerp(SilentDb, 0.0, T);
		if (bOutgoingPlaying)
		{
			FadeOutDb = FMath::Lerp(0.0, SilentDb, T);
		}
		if (T >= 1.0)
		{
			FadeStart = -1.0;
			if (bOutgoingPlaying)
			{
				if (UAudioComponent* Out = MusicPlayers[1 - ActiveMusic]; IsValid(Out))
				{
					Out->Stop();
				}
				bOutgoingPlaying = false;
			}
		}
	}
	ApplyVolumes();
	ObserveProgress();
}

void UPawAudioSubsystem::ObserveProgress()
{
	const UPawGameSubsystem* G = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawGameSubsystem>() : nullptr;
	if (!G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcProgress& P = G->Progress();
	auto Sum = [](const std::map<std::string, int64_t>& M)
	{
		int64 N = 0;
		for (const auto& Kv : M)
		{
			N += Kv.second;
		}
		return N;
	};
	const int64 Upgrades = Sum(P.UpgradeLevels), Branches = Sum(P.BranchLevels), Cards = Sum(P.CardLevels);
	const int64 Earned = static_cast<int64>(P.AchievementsEarned.size()), Claimed = static_cast<int64>(P.AchievementsClaimed.size());
	if (SeenEpoch == G->ProgressEpoch())
	{
		// Godot's signal hookups (_connect_events) plus the direct calls next to them: where both play an event whose
		// min_interval is 0, Godot plays it TWICE, layered. Here each plays once (F40, a Godot fault the port does not copy:
		// D16). Elsewhere the second is rate-limited anyway.
		if (Upgrades > SeenUpgrades)
		{
			Play(TEXT("purchase"));  // upgrade_purchased + purchase_upgrade(): the second inside 50 ms
		}
		if (P.ReawakenCount > SeenReawakens)
		{
			Play(TEXT("reawaken"));  // once (F40: reawaken() played it, then the reawakened signal again)
		}
		if (Branches > SeenBranches)
		{
			Play(TEXT("skill_purchase")); // once (F40: branch_leveled_up and the tree's own play on a purchase)
		}
		if (Earned > SeenEarned)
		{
			Play(TEXT("achievement_earned"));
		}
		if (Claimed > SeenClaimed)
		{
			Play(TEXT("achievement_claim"));
		}
		if (Cards > SeenCards)
		{
			Play(TEXT("card_taken"));    // once (F40: choose() played it, then card_taken again)
		}
		if (!P.Hand.empty() && P.Hand != SeenHand)
		{
			Play(TEXT("card_offered"));  // hand_offered on every deal, reroll and banish; once (F40: _offer_hand() played it first)
		}
	}
	SeenEpoch = G->ProgressEpoch();
	SeenUpgrades = Upgrades;
	SeenBranches = Branches;
	SeenCards = Cards;
	SeenReawakens = P.ReawakenCount;
	SeenEarned = Earned;
	SeenClaimed = Claimed;
	if (SeenHand != P.Hand)
	{
		SeenHand = P.Hand; // review CR17: copied every frame before
	}
}

void UPawAudioSubsystem::SetRealmTier(const FString& TierId)
{
	if (TierId == RealmTier)
	{
		return;
	}
	RealmTier = TierId;
	if (!TierId.IsEmpty())
	{
		PlayMusic(TierId); // TierController._ready: the biome's own track (none for 9-12: the music carries on)
	}
}

void UPawAudioSubsystem::OnBossDismissed(const FString& TierId)
{
	PlayMusic(TierId);
}

void UPawAudioSubsystem::OnFieldEvents(const std::vector<Pc::PcFieldEvent>& Events, const FString& TierId)
{
	using K = Pc::PcFieldEventKind;
	for (const Pc::PcFieldEvent& E : Events)
	{
		switch (E.Kind)
		{
		case K::Killed:
			if (E.Flags & Pc::FieldFlag::Golden)
			{
				Play(TEXT("golden_caught"));
			}
			Play(TEXT("mouse_killed")); // the kill counter's signal and the controller's own: the second inside 110 ms
			break;
		case K::BossSummoned:
			Play(TEXT("boss_summon"));
			PlayMusic(TEXT("boss"));
			break;
		case K::BossPhase:
			Play(TEXT("boss_phase"));
			break;
		case K::BossDefeated:
			Play(TEXT("boss_defeated")); // once (F40: GameState.boss_defeated's hookup and _on_boss_defeated both played it)
			break;
		// BossTimeout: _check_boss_time_limit() withdraws the boss but leaves its music playing (only _dismiss_boss, on a
		// Reawaken, plays the biome's again): kept.
		default:
			break;
		}
	}
}
