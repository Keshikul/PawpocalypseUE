#pragma once

// The game's sound (audio_manager.gd; docs/07 §6, phase 6 slice 4). The ONLY way a sound plays: Play(event) for the sound
// events of Data/AudioEvents.csv, PlayMusic(track) for the music of Data/AudioMusic.csv. Godot's rules, kept:
//   - each event has a minimum interval (a burst of kills is one drop), a pitch spread (1 +- spread) and a volume in dB;
//     its sounds play in turn (round robin);
//   - 16 voices taken in turn: the 17th sound cuts the oldest;
//   - music crossfades over 1.2 s (the incoming from -60 dB, the outgoing to -60 dB then stopped, each in dB, linearly);
//     the same track again does nothing; a track with no file keeps whatever plays (biomes 9-12 have none);
//   - the buses: Master x Music or SFX, each a linear amplitude (Godot's linear_to_db on a bus), Master muted by the mute
//     box or a zero; read live from the settings, so a slider moves what is playing;
//   - the rate limits and the crossfade run on real time (the audio autoload runs even while the tree is paused).
// What triggers which event follows Godot's signal hookups: an observer of the progress (a purchase, a Reawaken, a skill's
// level, an achievement earned or claimed, a hand offered, a card taken), the field's events (the realm passes them), the
// biome's music when the realm changes biome, and the UI's own clicks.
//
// THE OUTPUT DEVICE (settings_manager.gd _apply_output_device; Settings > Audio > Output; playtest 2026-10-01, #31): the
// system's devices by name, "Default" first (Godot's AudioServer.get_output_device_list()). The stored device while the system
// lists it, else "Default" (Pc::SettingsOutputWant), told to the engine only when that differs from what it was last told; at
// startup, on every settings change (apply_audio) and when a device goes away (the pinned one: back to the system's, never
// silence). A chosen device is PINNED (Godot's AudioServer stays on it: a pad bringing a headset, the system's default
// changing, move nothing); "Default" follows the system. Unreal's WASAPI mixer follows the system's default unless told not
// to (SetIsListeningForDeviceEvents), and a swap alone does not tell it: the port sets both. Never in the editor (the
// designer's own device): the list is read there, the swap is not made.

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"

#include <map>
#include <string>
#include <vector>

#include "PawAudioSubsystem.generated.h"

class UAudioComponent;
class UPawAudioBank;
class USoundWave;
namespace Pc
{
	struct PcFieldEvent;
}

UCLASS()
class PAWPOCALYPSE_API UPawAudioSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	static constexpr int32 Voices = 16;     // SFX_VOICES
	static constexpr double MusicFadeSec = 1.2;
	static constexpr double SilentDb = -60.0;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// The subsystem of the game instance that owns a world object (a widget, an actor), or null.
	static UPawAudioSubsystem* Of(const UObject* WorldContext);
	// AudioManager.play from anywhere (a UI click): Of(Context)->Play(Event) when there is one.
	static void PlayFrom(const UObject* WorldContext, const TCHAR* Event);

	// AudioManager.play / play_music / stop_music / has_music.
	void Play(const FString& Event);
	void PlayMusic(const FString& Track);
	void StopMusic();
	bool HasMusic(const FString& Track) const;
	const FString& CurrentTrack() const { return MusicTrack; }

	// The field's events of a frame (the realm calls this): the kill's drop, the golden, the boss's summon (and its music),
	// phase and death.
	void OnFieldEvents(const std::vector<Pc::PcFieldEvent>& Events, const FString& TierId);
	// The realm is in this biome now: its music (TierController._ready). Called every frame; acts on a change.
	void SetRealmTier(const FString& TierId);
	// TierController._on_reawakened -> _dismiss_boss: a boss up goes, its music with it.
	void OnBossDismissed(const FString& TierId);

	// What was asked of the sound, newest last (the tests; every Play that passed its rate limit, voiced or not: a headless
	// run has no audio device).
	struct FPlayed
	{
		FString Event;
		int32 Variant = 0;
		float Pitch = 1.0f;
		float VolumeDb = 0.0f;
	};
	const TArray<FPlayed>& Played() const { return Log; }
	void ClearLog() { Log.Reset(); }
	static constexpr int32 MaxLog = 256; // the log keeps the newest (review CR4)
	// The bus amplitudes now (Master x Music, Master x SFX).
	double MusicGain() const;
	double SfxGain() const;
	// The music fade, in dB, of the incoming and the outgoing players (tests).
	double IncomingDb() const { return FadeInDb; }
	double OutgoingDb() const { return FadeOutDb; }
	// The clock the rate limits and fades run on (tests replace it).
	TFunction<double()> Now;

	// The Output list's devices (AudioServer.get_output_device_list()): "Default", then each active device by name.
	TArray<FString> OutputDeviceList() const;
	// _apply_output_device() (see the top of this file).
	void ApplyOutputDevice();
	// What the engine was last told (AudioServer.output_device: "Default" at startup) and how many swaps were made (tests).
	const FString& GetAppliedOutputDevice() const { return AppliedOutput; }
	int32 GetOutputSwaps() const { return OutputSwaps; }
	// The system, replaced by the tests: its devices' names (without "Default"), and the swap to a name ("Default": follow
	// the system; true when it was made). Null: the engine's.
	TFunction<TArray<FString>()> SystemDevices;
	TFunction<bool(const FString& Name)> SwapOutput;
	// How a voice or a music player is made (the tests count them). Null: the engine's 2D sound that persists across level
	// changes, as the Godot autoload's players do. Each is made once and reused while valid (review CR3).
	TFunction<UAudioComponent*(USoundWave* Sound)> CreatePlayer;
	// Ticks the fade and the observer once (the tests, without the engine's tick).
	void TickOnce(float DeltaSeconds) { Tick(DeltaSeconds); }

	// FTickableGameObject
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UPawAudioSubsystem, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }

private:
	void ObserveProgress();
	UAudioComponent* Voice(int32 Index, USoundWave* Sound);
	UAudioComponent* MusicPlayer(int32 Index, USoundWave* Sound);
	UAudioComponent* MakePlayer(USoundWave* Sound);
	void ApplyVolumes();

	UPROPERTY(Transient)
	TObjectPtr<const UPawAudioBank> Bank;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> VoicePlayers;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicPlayers[2];

	TArray<float> VoiceDb;               // each voice's event volume, for live bus changes
	int32 NextVoice = 0;
	TMap<FString, int32> NextVariant;
	TMap<FString, double> LastPlayed;
	TArray<FPlayed> Log;

	FString MusicTrack;
	FString RealmTier;
	int32 ActiveMusic = 0;
	double FadeStart = -1.0;
	double FadeInDb = SilentDb, FadeOutDb = SilentDb;
	bool bOutgoingPlaying = false;

	FString AppliedOutput = TEXT("Default");
	int32 OutputSwaps = 0;
	FDelegateHandle SettingsHandle;
	FDelegateHandle DeviceRemovedHandle;

	// The observer's last look at the progress.
	int64 SeenEpoch = -1;
	int64 SeenUpgrades = 0, SeenBranches = 0, SeenCards = 0, SeenReawakens = 0;
	int64 SeenEarned = 0, SeenClaimed = 0;
	std::vector<std::string> SeenHand;
};
