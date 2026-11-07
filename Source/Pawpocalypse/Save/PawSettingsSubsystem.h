#pragma once

// The player's preferences (docs/08 §3; Godot SettingsManager): PawCore's PcSettings, kept in the entry "settings" of the
// same storage as the slots (Godot: user://settings.cfg beside them, in Godot's ConfigFile text), so the sandbox rules are
// the slots' (PawStorageForThisRun: -NoSave, -PawSaveDir=, off in automated runs; tests hand in their own).
//
// The ONE writer: every change goes through a setter here, which stores the value (with Godot's clamp), writes the whole
// file and applies what the engine owns. What it applies, and when:
//   the window (windowed at the fitted, centred resolution, or fullscreen), V-Sync, the frame cap: at startup (one frame
//   after it, once the window exists) and on change; never in the editor, never in a run with no settings storage (the
//   bench, the tour: the command line rules), and the window never when the command line names one (-ResX, -windowed ...);
//   the menu side: the HUD listens (OnChanged);
//   the effects budget: read where effects spawn (EffectBudget);
//   ask before Reawakening: read by the HUD;
//   the last slot used: read by the save subsystem at startup, written when a slot is saved into or loaded.
//   the controls (the bindings, the swaps, the dead zone, the prompts): UPawInputDeviceSubsystem listens and applies them;
//   the stick's inversion and the cursor speed: read where the stick steers (the cat, the skill tree's pan).
//   the volumes and mute: read live by UPawAudioSubsystem; the output device: UPawAudioSubsystem listens and applies it.
// NOT YET, each with its phase: the language (phase 7), the tutorial tips (stored; the tutorial is phase 6).

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PcSettings.h"

#include "PawSettingsSubsystem.generated.h"

class IPawSaveStorage;

// What the engine was last told (or would have been, where applying is off): the tests read it.
struct FPawVideoApplied
{
	bool bFullscreen = false;
	int32 Width = 0;
	int32 Height = 0;
	bool bVsync = true;
	int64 MaxFps = 0;
	int32 Count = 0;          // how many times the video settings were applied
	bool bToEngine = false;   // the last one reached the engine
};

UCLASS()
class PAWPOCALYPSE_API UPawSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static const TCHAR* const EntryName; // "settings"

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const Pc::PcSettings& Get() const { return Settings; }
	// EffectBudget(): 0 with effects off, else by quality (0.35 / 0.70 / 1).
	double EffectBudget() const { return Pc::EffectBudget(Settings); }
	bool IsDockOnLeft() const { return Settings.DockSide != "right"; }

	// The setters (settings_manager.gd set_*): store with Godot's clamp, write the file, apply.
	void SetMasterVolume(double V);
	void SetMusicVolume(double V);
	void SetSfxVolume(double V);
	void SetMuted(bool b);
	void SetOutputDevice(const FString& Name);
	void SetFullscreen(bool b);
	void SetVsync(bool b);
	void SetResolutionIndex(int64 Index);
	void SetGraphicsQuality(int64 Quality);
	void SetMaxFps(int64 Fps);
	void SetEffectsEnabled(bool b);
	void SetLastSlot(int64 Slot);
	void SetConfirmReawaken(bool b);
	void SetPromptStyle(const FString& Style);
	void SetDockSide(const FString& Side);
	// Settings > Controls (controls_panel.gd through set_control): nothing happens when the value is the one held (Godot
	// returns early), else it is stored (the two sliders clamped), saved and applied. The prompts and the bindings follow
	// through UPawInputDeviceSubsystem (OnControlsChanged).
	void SetShowPrompts(bool b);
	void SetTutorialTips(bool b);
	void SetSwapConfirmBack(bool b);
	void SetSwapSticks(bool b);
	void SetInvertX(bool b);
	void SetInvertY(bool b);
	void SetCursorSpeed(double V);   // 0.5 .. 2
	void SetStickDeadzone(double V); // 0.05 .. 0.5
	// set_bindings(): the actions that differ from the defaults (Pc::BindingDiff), stored and saved whatever they were.
	void SetBindings(const Pc::PcSavedBindings& Bindings);
	// "Reset all controls" (_on_reset_all): no bindings; the two help switches on; every swap and inversion off; the cursor
	// speed 1 and the stick dead zone 0.2.
	void ResetControls();

	// Anything changed (the HUD re-reads the menu side, the dialog its controls).
	DECLARE_MULTICAST_DELEGATE(FOnSettingsChanged);
	FOnSettingsChanged OnChanged;

	const FPawVideoApplied& GetVideoApplied() const { return Video; }
	// Whether the video settings may reach the engine: never in the editor (the designer's own editor), never in a run with
	// no settings storage (the bench, the tour: the command line rules), and only with a game viewport.
	static bool MayApplyToEngine(bool bEditor, bool bStored, bool bViewport);
	int32 GetWrites() const { return Writes; }
	bool IsStored() const { return Storage.IsValid(); }

	// The tests' sandbox: their own storage, read now (as at startup); null: none, the defaults.
	void UseStorageForTest(TSharedPtr<IPawSaveStorage> InStorage);
	// Reads the file again (a test that put one in its storage).
	void Reload();

private:
	void Save();
	void Changed(bool bVideo);
	void ApplyVideo();

	Pc::PcSettings Settings;
	TSharedPtr<IPawSaveStorage> Storage;
	FPawVideoApplied Video;
	int32 Writes = 0;
	bool bWindowFromCommandLine = false;
	FTSTicker::FDelegateHandle StartupApply;
};
