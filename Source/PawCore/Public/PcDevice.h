#pragma once

// Which device the player is using, as TWO questions (Godot input_device.gd; proven against it, fixture `input_device`): the
// PROMPT FAMILY (which glyphs to draw) and the CURSOR SOURCE (what steers the cat, the mouse or the steer stick). Everything
// switches on the first INPUT from a device, never on connect (a connect is a toast). Mouse MOTION steers but never flips
// the prompts (a trackpad, a bumped desk); a click is a claim on both. The game module feeds it every event before the GUI
// sees it (UPawInputDeviceSubsystem) and acts on what it decides (the pointer, the toast, the prompts).

#include "PcCore.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Pc
{
	// InputGlyphs.Family, in Godot's order.
	enum class PcFamily : uint8_t { Keyboard, Xbox, PlayStation, SteamDeck, Generic };
	enum class PcCursorSource : uint8_t { Mouse, Stick };
	// PadAudio: _watch_for_pad_audio's "No sound?" toast (the game module decides it with DevicePadAudioToast).
	enum class PcDeviceToast : uint8_t { None, Connected, Disconnected, PadAudio };

	// input_device.gd's constants (input feel, not gameplay: they stay with the rules that use them).
	constexpr double DeviceAxisClaim = 0.5;      // AXIS_CLAIM: a stick or trigger past this is the player choosing the pad
	constexpr double DeviceMouseClaimPx = 6.0;   // MOUSE_CLAIM_PX: motion adding up to this takes the cat back
	constexpr int64_t DeviceMouseQuietMs = 250;  // MOUSE_QUIET_SEC: travel resets after this long without motion
	constexpr int64_t DeviceBootQuietMs = 2000;  // BOOT_QUIET_SEC: pads plugged in at launch connect without a toast
	constexpr int64_t DeviceWarpEchoMs = 120;    // WARP_ECHO_MSEC: motion this soon after a warp is the warp's own echo
	constexpr int64_t DeviceModalGuardMs = 400;  // MODAL_GUARD_MSEC: a modal ignores presses this soon after it opens
	constexpr int64_t DevicePadAudioWaitMs = 2000; // PAD_AUDIO_WAIT_SEC: Windows adds a pad's audio endpoint a beat after it

	struct PcDeviceState
	{
		PcFamily Family = PcFamily::Keyboard;     // what prompts draw: the override, else what was detected
		PcFamily Detected = PcFamily::Keyboard;   // what the last input said
		PcCursorSource Cursor = PcCursorSource::Mouse;
		int64_t ActiveDevice = -1;                // the pad that last sent input
		std::map<int64_t, PcFamily> DeviceFamilies;
		double MouseTravel = 0.0;
		int64_t LastMotionMs = 0;
		int64_t WarpUntilMs = 0;
		int64_t PressBeganMs = -1;                // when `press` last went DOWN
		bool bPressAxisDown = false;
		bool bLastWasPointer = true;              // the last deliberate input was the pointer (a click, real travel)
		bool bPointerOverField = false;           // the pointer is over the open field (the realm answers)
		bool bPointerHidden = false;              // the OS pointer is hidden: the stick steers, or it is over the field
		int64_t BootMs = 0;
		std::string PromptStyle = "auto";         // Settings > Controls > Button prompts
		bool bSwapSticks = false;                 // Settings > Controls > Swap sticks: steer on the left stick
		PcDeviceToast Toast = PcDeviceToast::None; // the last toast shown
	};

	// One input event, as observe() reads it. bPressAction: the event is bound to `press` (event.is_action("press"));
	// bPressPressed: and pressed past its dead zone (is_action_pressed). Axis: Godot's joypad axis (0 LX, 1 LY, 2 RX, 3 RY, 4 LT,
	// 5 RT); AxisValue in Godot's sense (+Y down).
	enum class PcDeviceEventKind : uint8_t { Key, MouseButton, MouseMotion, PadButton, PadMotion };
	struct PcDeviceEvent
	{
		PcDeviceEventKind Kind = PcDeviceEventKind::Key;
		bool bPressed = false;
		bool bEcho = false;
		double MotionLength = 0.0;
		int64_t Device = 0;
		int64_t Axis = 0;
		double AxisValue = 0.0;
		bool bPressAction = false;
		bool bPressPressed = false;
	};

	// observe(): the switching table.
	PAWCORE_API void DeviceObserve(PcDeviceState& S, const PcDeviceEvent& E, int64_t NowMs);
	// family_from_info(): the detection table (vendor / product ids, then names; an unknown pad is Generic, drawn as Xbox).
	PAWCORE_API PcFamily DeviceFamilyFromInfo(int64_t VendorId, int64_t ProductId, const std::string& Name, const std::string& RawName,
		bool bOnDeck);
	// register_device(): a pad's family remembered (from what the OS says about it).
	PAWCORE_API void DeviceRegister(PcDeviceState& S, int64_t Device, PcFamily Family);
	// The family of a pad never registered: Godot asks the OS, and with nothing to go on it is Generic.
	PAWCORE_API PcFamily DeviceFamilyOf(PcDeviceState& S, int64_t Device);
	// _refresh_family(): the override if one is set, else what was detected. True when the family changed.
	PAWCORE_API bool DeviceRefreshFamily(PcDeviceState& S);
	// claim_stick(): R3's re-centre takes the cursor without an axis event.
	PAWCORE_API void DeviceClaimStick(PcDeviceState& S);
	// warp_pointer(): the motion this soon after is the warp's echo.
	PAWCORE_API void DeviceWarp(PcDeviceState& S, int64_t NowMs);
	// press_counts_for(): may a press count for a modal that opened at OpenedMs?
	PAWCORE_API bool DevicePressCountsFor(const PcDeviceState& S, int64_t OpenedMs, int64_t NowMs);
	// _on_joy_connection_changed(): a connect registers the pad (as the OS describes it now) and toasts after the boot window;
	// a disconnect of the pad in hand goes back to the mouse and keys and says so.
	PAWCORE_API void DeviceConnection(PcDeviceState& S, int64_t Device, bool bConnected, PcFamily Family, int64_t NowMs);
	// _watch_for_pad_audio(): a pad connected after the boot window (the toast said so) may bring an AUDIO device that takes the
	// system's default output (a DualShock 4's headphone jack; the silence of playtest 2026-10-01, #31). The output device list
	// is read at the connect and DevicePadAudioWaitMs later (by name, "Default" among them); the toast says where to fix it
	// when a name appeared that was not there before and the game follows the system (OutputDevice "Default"). Each connect
	// watches on its own.
	PAWCORE_API bool DevicePadAudioToast(const std::vector<std::string>& Before, const std::vector<std::string>& After,
		const std::string& OutputDevice);
	// The steer stick's axes: the right stick (2, 3), the left with Swap sticks.
	PAWCORE_API bool DeviceIsSteerAxis(const PcDeviceState& S, int64_t Axis);
	// The pointer over the field changed (the realm answers each frame); it hides there, and while the stick steers.
	PAWCORE_API void DeviceSetOverField(PcDeviceState& S, bool bOver);
	// is_pad()
	inline bool DeviceIsPad(const PcDeviceState& S) { return S.Family != PcFamily::Keyboard; }
}
