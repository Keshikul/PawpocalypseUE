#pragma once

// The Controls page (Godot controls_panel.gd, Settings > Controls; proven against it in the real settings dialog, fixture
// `controls`): what a press, a key while a cell listens, the clock, a Reset, a switch or a slider does to the page and to the
// settings, and what the page then shows. The game module draws it (UPawControlsPanel) and turns Slate's events into
// PcCapturedEvent; every decision is here. The bindings' own rules are PcBindings.
//
// LISTENING: pressing a cell listens for the next key, mouse button, pad button or trigger of the cell's device. Back (Esc,
// or the pad's back button as the live map has it) cancels; a click while a CONTROLLER cell listens cancels; anything that
// cannot be a binding (a release, an echo, the wheel, a stick) or belongs to the other device is swallowed and it keeps
// listening; five seconds of nothing cancel. The status line says what happened.

#include "PcBindings.h"
#include "PcCore.h"
#include "PcDevice.h"
#include "PcSettings.h"

#include <cstdint>
#include <string>

namespace Pc
{
	// The status line under the table (its words are the string table's).
	enum class PcControlsLine : uint8_t
	{
		None,           // "" (also what an unchanged binding leaves)
		Listening,      // "Press a key or button for {action}. Esc or B cancels."
		NothingPressed, // "Nothing pressed, nothing changed."
		Cancelled,      // "Cancelled."
		Done,           // "Done."
		Swapped,        // "Swapped with {action}."
		RefusedSystem,  // "Esc, Start and B always open the menu and go back."
		Taken,          // "That one is taken."
		Reset,          // "Reset."
		ResetAll,       // "Every control is back to its default."
	};

	struct PcControlsState
	{
		bool bListening = false;
		std::string ListenRow;                         // the logical row a cell listens for
		PcBindDevice ListenDevice = PcBindDevice::Keyboard;
		double ListenLeft = 0.0;                       // seconds until it gives up
		PcControlsLine Line = PcControlsLine::None;
		std::string LineRow;                           // the row the line names (Listening, Swapped)
	};

	// The six switches, in the page's order.
	enum class PcControlsCheck : uint8_t { ShowPrompts, TutorialTips, SwapConfirmBack, SwapSticks, InvertX, InvertY };
	constexpr int ControlsCheckCount = 6;
	PAWCORE_API bool ControlsCheckValue(const PcSettings& S, PcControlsCheck Check);
	// set_control(): false (nothing stored, nothing saved) when the value is the one held.
	PAWCORE_API bool ControlsSetCheck(PcSettings& S, PcControlsCheck Check, bool bOn);

	// The two sliders: cursor speed 0.5 .. 2 step 0.05, stick dead zone 0.05 .. 0.5 step 0.01 (Godot keeps both ranges in a
	// Vector2: 32-bit floats, so the dead zone's floor is float(0.05)).
	enum class PcControlsSlider : uint8_t { CursorSpeed, StickDeadzone };
	PAWCORE_API double ControlsSliderMin(PcControlsSlider Which);
	PAWCORE_API double ControlsSliderMax(PcControlsSlider Which);
	PAWCORE_API double ControlsSliderStep(PcControlsSlider Which);
	// Godot's Range.set_value: snapped to the step FROM THE MINIMUM (k / (1 / step) + min, k = floor((v - min) / step + 0.5)),
	// then held to the range. Measured on 1,112 moves (fixture `controls`).
	PAWCORE_API double ControlsSliderSnap(PcControlsSlider Which, double Value);
	PAWCORE_API double ControlsSliderSetting(const PcSettings& S, PcControlsSlider Which);
	// set_control(): false when the value given is the one held; else stored, held to the range.
	PAWCORE_API bool ControlsSetSlider(PcSettings& S, PcControlsSlider Which, double Value);
	// roundi(value * 100): the number on a slider's line ("Cursor speed  {n}%"), from the STORED value.
	PAWCORE_API int64_t ControlsPercent(double Value);

	// --- listening -----------------------------------------------------------------------------------------------------
	// A cell pressed: it listens (a locked cell is disabled and cannot be pressed: the caller's).
	PAWCORE_API void ControlsListen(PcControlsState& C, const std::string& Row, PcBindDevice Device);
	// The clock: true when the listening timed out (now not listening, "Nothing pressed").
	PAWCORE_API bool ControlsTick(PcControlsState& C, double DeltaSeconds);

	enum class PcControlsTake : uint8_t
	{
		NotListening, // not the page's: the event goes on
		Passed,       // mouse motion: not the page's either
		Swallowed,    // the page took it and did nothing with it (it keeps listening)
		Cancelled,
		Bound,        // a binding was tried: see Result
	};
	struct PcControlsInput
	{
		PcControlsTake Take = PcControlsTake::NotListening;
		PcBindResult Result = PcBindResult::Unchanged;
		bool bSaved = false; // the settings' bindings changed (save them, apply them)
	};
	// _input(): one event while (maybe) listening. bMotion: mouse motion. bBack: the event presses `back` or `ui_cancel` in the
	// live map (a press, not an echo).
	PAWCORE_API PcControlsInput ControlsInput(PcControlsState& C, PcSettings& S, const PcInputState& Defaults, const PcCapturedEvent& E,
		bool bMotion, bool bBack);
	// bind(): `Event` onto `Row` (also the gate's direct path), the line set, listening ended. bSaved as above.
	PAWCORE_API PcBindResult ControlsBind(PcControlsState& C, PcSettings& S, const PcInputState& Defaults, const std::string& Row,
		const PcInputEvent& Event, bool& bSaved);
	// A row's Reset (Pause's is disabled): the row (and what it then shares a key with) back to the defaults; "Reset.".
	PAWCORE_API void ControlsResetRow(PcControlsState& C, PcSettings& S, const PcInputState& Defaults, const std::string& Row);
	// Reset all controls: no bindings, the two help switches on, the rest off, cursor speed 1, dead zone 0.2.
	PAWCORE_API void ControlsResetAll(PcControlsState& C, PcSettings& S);

	// --- what the page shows -------------------------------------------------------------------------------------------
	PAWCORE_API bool ControlsCellListening(const PcControlsState& C, const std::string& Row, PcBindDevice Device);
	PAWCORE_API bool ControlsResetEnabled(const std::string& Row);
	// pad_column_family(): the family the controller column draws: the Button prompts choice when it names a pad, else the pad
	// in use, else the pad last detected, else Xbox.
	PAWCORE_API PcFamily ControlsPadColumnFamily(const std::string& PromptStyle, PcFamily Family, PcFamily Detected);
}
