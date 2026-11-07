#pragma once

// The player's preferences (Godot settings_manager.gd, and the rules of settings_dialog.gd; proven against both, fixture
// `settings`). Per machine, never in the save: wiping a game keeps the volume. The game module stores the file
// (UPawSettingsSubsystem), applies what the engine owns (the window, V-Sync, the frame cap) and draws the dialog
// (UPawSettingsDialog); everything that decides a value is here.
//
// The file is Godot's settings.cfg format (ConfigFile: [section] then key=value), read the way load_settings() reads it:
// a key missing keeps the value already held, and only the values Godot clamps are clamped (volumes are NOT: a file can hold
// 1.3, and the dialog then says 130%). A value of the wrong kind keeps the value already held (Godot would stop on it).
// [bindings] (the remapping, PcBindings) is read as Godot reads it: a list per action is kept, anything else is dropped. A
// list that is not of flat dictionaries (Godot keeps it, then fails on it when it applies) is dropped here too.

#include "PcBindings.h"
#include "PcCore.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Pc
{
	struct PcIntSize
	{
		int64_t X = 0;
		int64_t Y = 0;
		bool operator==(const PcIntSize& O) const { return X == O.X && Y == O.Y; }
	};

	struct PcSettings
	{
		// Audio. Three sliders, not one: music and effects are the two things players want apart.
		double MasterVolume = 1.0;
		double MusicVolume = 0.45;
		double SfxVolume = 1.0;
		bool bMuted = false;
		std::string OutputDevice = "Default"; // the device by the name the system lists, or "Default": follow the system
		// Video
		bool bFullscreen = false;
		bool bVsync = true;
		int64_t GraphicsQuality = 2;          // 0 Low, 1 Medium, 2 High: how much transient effect work is allowed
		int64_t MaxFps = 0;                   // 0 = unlimited (V-Sync already paces an idle screen)
		bool bEffectsEnabled = true;
		std::string Language = "system";
		int64_t ResolutionIndex = 2;          // into Resolutions: the WINDOW size
		// Game
		int64_t LastSlot = 0;                 // the slot in play last: a machine preference, not progression
		bool bConfirmReawaken = true;
		bool bTutorialTips = true;
		// Controls
		std::string PromptStyle = "auto";
		std::string DockSide = "left";
		bool bShowPrompts = true;
		bool bSwapConfirmBack = false;
		bool bSwapSticks = false;
		bool bInvertX = false;
		bool bInvertY = false;
		double CursorSpeed = 1.0;
		double StickDeadzone = 0.2;
		// [bindings]: the actions the player rebound, as saved (PcBindings: only what differs from the defaults).
		PcSavedBindings Bindings;
	};

	constexpr int64_t SettingsResolutionCount = 5;
	PAWCORE_API PcIntSize SettingsResolution(int64_t Index);          // clamped: 1280x720 .. 3840x2160
	constexpr int64_t SettingsFpsOptionCount = 6;
	PAWCORE_API int64_t SettingsFpsOption(int64_t Index);             // 0 (Unlimited), 30, 60, 120, 144, 240
	constexpr int64_t SettingsPromptStyleCount = 5;
	PAWCORE_API const char* SettingsPromptStyle(int64_t Index);       // auto, keyboard, xbox, playstation, steam_deck
	constexpr int64_t SettingsDockSideCount = 2;
	PAWCORE_API const char* SettingsDockSide(int64_t Index);          // left, right
	constexpr int64_t SettingsSlotCount = 5;                          // last_slot is clamped to 0..4
	constexpr int64_t SettingsQualityCount = 3;
	PAWCORE_API PcIntSize SettingsMinWindow();                        // 1024 x 600: never shrink a window below it
	constexpr double SettingsCursorSpeedMin = 0.5, SettingsCursorSpeedMax = 2.0;
	// Godot keeps both ranges in a Vector2, whose parts are 32-bit floats: the dead zone's floor is float(0.05), measured.
	constexpr double SettingsDeadzoneMin = static_cast<double>(0.05f), SettingsDeadzoneMax = static_cast<double>(0.5f);

	// load_settings(): the file read over S (a missing key keeps S's value). False when the text is not a settings file at
	// all (nothing read); S is then untouched.
	PAWCORE_API bool SettingsLoad(const std::string& Text, PcSettings& S);
	// save_settings(): the whole file, in Godot's layout; doubles written so they read back exactly.
	PAWCORE_API std::string SettingsWrite(const PcSettings& S);

	// The setters' own clamps (set_*): what a value becomes once stored.
	PAWCORE_API double SettingsClampVolume(double V);                 // 0..1
	PAWCORE_API int64_t SettingsClampQuality(int64_t V);              // 0..2
	PAWCORE_API int64_t SettingsClampFps(int64_t V);                  // >= 0
	PAWCORE_API int64_t SettingsClampSlot(int64_t V);                 // 0..4
	PAWCORE_API int64_t SettingsClampResolution(int64_t V);           // 0..4
	PAWCORE_API bool SettingsIsPromptStyle(const std::string& V);
	PAWCORE_API bool SettingsIsDockSide(const std::string& V);

	// effect_budget(): how much decorative effect the settings allow, 0..1 (Effects off: 0; Low 0.35, Medium 0.70, High 1).
	PAWCORE_API double EffectBudget(const PcSettings& S);

	// fit_to_screen(): the largest CLIENT area that fits once the decorations are added, never below the minimum window; a
	// screen with no size leaves the target alone.
	PAWCORE_API PcIntSize FitToScreen(PcIntSize Target, PcIntSize Usable, PcIntSize Decoration);
	// centred_position(): top-left for a centred window, never off the top or left of the screen.
	PAWCORE_API PcIntSize CentredPosition(PcIntSize ScreenPos, PcIntSize ScreenSize, PcIntSize Size);

	// --- the dialog (settings_dialog.gd) -------------------------------------------------------------------------------
	enum class PcSettingsSlider : uint8_t { Master, Music, Sfx };
	enum class PcSettingsSection : uint8_t { Audio, Video, Controls };
	constexpr int64_t SettingsSectionCount = 3;

	struct PcSettingsDialog
	{
		// What the sliders hold (snapped to their step, clamped to 0..1) and the percent each line shows.
		double Slider[3] = { 1.0, 0.45, 1.0 };
		int64_t Percent[3] = { 100, 45, 100 };
		PcSettingsSection Section = PcSettingsSection::Audio;
	};

	// The slider's step: Master 0.05, Music and Sound Effects 0.01 (settings_dialog.tscn).
	PAWCORE_API double SettingsSliderStep(PcSettingsSlider Which);
	// Godot's Range: a value snapped to the step from 0 (rounded half away from zero), then held to 0..1.
	PAWCORE_API double SettingsSliderSnap(double Value, double Step);
	// roundi(value * 100): the number on a volume line.
	PAWCORE_API int64_t SettingsPercent(double Value);
	// _sync_from_settings(): the sliders take the stored volumes (snapped, clamped); the lines say the STORED volume.
	PAWCORE_API void SettingsDialogSync(PcSettingsDialog& D, const PcSettings& S);
	// A slider moved to Value (by a drag, a key, a stick): false when it did not move (the snapped value is the one it
	// held; Godot emits nothing then, so nothing is stored). Moved: the line says the slider's value, the volume is stored.
	PAWCORE_API bool SettingsDialogMoveSlider(PcSettingsDialog& D, PcSettings& S, PcSettingsSlider Which, double Value);
	// _step_section(): the dialog's own pages, wrapping (LB / RB, Page Up / Down).
	PAWCORE_API void SettingsDialogStepSection(PcSettingsDialog& D, int64_t Direction);

	// The item each list shows selected (_sync_from_settings).
	PAWCORE_API int64_t SettingsFpsSelected(const PcSettings& S);      // a cap not on the list shows Unlimited, and stays
	PAWCORE_API int64_t SettingsPromptsSelected(const PcSettings& S);
	PAWCORE_API int64_t SettingsDockSelected(const PcSettings& S);
	// The Output list (_fill_output_option): the system's devices, then the stored one if it is not among them (unplugged
	// since it was saved: kept, so plugging it back in finds it). "Default" is shown as "Follow the system".
	struct PcSettingsOutputList
	{
		std::vector<std::string> Devices;
		int64_t Selected = -1;
	};
	PAWCORE_API PcSettingsOutputList SettingsOutputList(const std::vector<std::string>& SystemDevices, const PcSettings& S);
	// _apply_output_device(): the device the game should play to now. The stored one while the system lists it, else
	// "Default" (unplugged since it was saved: the system's device, never silence; the setting itself is kept).
	PAWCORE_API std::string SettingsOutputWant(const std::vector<std::string>& SystemDevices, const PcSettings& S);
}
