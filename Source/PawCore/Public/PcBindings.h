#pragma once

// Remapping (Godot input_bindings.gd, InputBindings, controller plan 8; proven against it, fixture `input_bindings`). The
// game module turns what this decides into its mapping context (Godot codes -> FKeys) and draws the Controls page; every rule
// is here.
//
// A LOGICAL ROW IS SEVERAL ACTIONS: "Up" is menu_up AND ui_up, "Previous tab" is tab_prev, ui_left and section_prev. A rebind
// replaces the old event with the new one in EVERY member, so the mirrors cannot drift apart. A key another row had is SWAPPED
// (that row gets the old key back); the keys of the actions nobody may rebind (the number keys, U, the zoom keys, the steer
// stick) are refused, and Esc, Start and pad B are SYSTEM keys: always the menu, always Back.
//
// STORED AS WHOLE ACTIONS, only those that differ from the defaults (settings.cfg [bindings], PcSettings::Bindings): a list of
// serialized events per action, Godot's small dictionaries ({"type": "key", "physical_keycode": 65}). Swap sticks and Swap
// confirm and back are TRANSFORMS applied on top (BindingApply), never stored in the bindings.
//
// Events are Godot's: a key by its physical keycode, a mouse button, a pad button, one direction of a pad axis (Godot's axes:
// +Y is DOWN). The defaults are Data/InputMap.csv's, in Godot's InputMap order (BindingGodotOrder).

#include "PcCore.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Pc
{
	enum class PcInputKind : uint8_t { None, Key, Mouse, JoyButton, JoyAxis };

	struct PcInputEvent
	{
		PcInputKind Kind = PcInputKind::None;
		int64_t Code = 0; // the physical keycode, the mouse button, the pad button or the pad axis (Godot's numbers)
		int64_t Sign = 0; // a pad axis: the sign of its value (-1, 0, +1)

		bool IsNone() const { return Kind == PcInputKind::None; }
		bool operator==(const PcInputEvent& O) const { return Kind == O.Kind && Code == O.Code && Sign == O.Sign; }
		bool operator!=(const PcInputEvent& O) const { return !(*this == O); }
	};

	// The Godot codes the rules name.
	namespace GodotInput
	{
		constexpr int64_t KeyEscape = 4194305;
		constexpr int64_t JoyA = 0, JoyB = 1, JoyStart = 6;
		constexpr int64_t AxisLeftX = 0, AxisLeftY = 1, AxisRightX = 2, AxisRightY = 3, AxisTriggerLeft = 4, AxisTriggerRight = 5;
		constexpr int64_t MouseWheelUp = 4, MouseWheelDown = 5, MouseWheelLeft = 6, MouseWheelRight = 7;
	}

	// One action's events, in order (Godot's InputMap.action_get_events).
	struct PcActionEvents
	{
		std::string Action;
		std::vector<PcInputEvent> Events;
	};
	// The map: every action, in Godot's InputMap order.
	using PcInputState = std::vector<PcActionEvents>;

	PAWCORE_API const std::vector<PcInputEvent>* BindingFind(const PcInputState& State, const std::string& Action);
	// Godot's `state[action]`: the action's list, made (at the end) when the state has none.
	PAWCORE_API std::vector<PcInputEvent>& BindingAt(PcInputState& State, const std::string& Action);

	// --- the saved form (settings.cfg [bindings]) -------------------------------------------------------------------------
	// A value inside a saved event's dictionary, as settings.cfg holds it.
	enum class PcCfgKind : uint8_t { Int, Float, String, Bool };
	struct PcCfgValue
	{
		PcCfgKind Kind = PcCfgKind::Int;
		int64_t I = 0;
		double F = 0.0;
		std::string S;
		bool B = false;
		PAWCORE_API bool operator==(const PcCfgValue& O) const;
	};
	// One serialized event: a Dictionary, its keys in its own order (serialize()'s, or the file's: Godot writes them sorted).
	struct PcSavedEvent
	{
		std::vector<std::pair<std::string, PcCfgValue>> Fields;
		PAWCORE_API const PcCfgValue* Get(const std::string& Key) const;
		bool operator==(const PcSavedEvent& O) const { return Fields == O.Fields; }
	};
	struct PcSavedAction
	{
		std::string Action;
		std::vector<PcSavedEvent> Events;
		bool operator==(const PcSavedAction& O) const { return Action == O.Action && Events == O.Events; }
	};
	// SettingsManager.bindings: action -> serialized events, only the actions that differ from the defaults, in order.
	using PcSavedBindings = std::vector<PcSavedAction>;

	// --- the defaults -----------------------------------------------------------------------------------------------------
	// An event from Data/InputMap.csv's Godot columns (godot_event: key | mouse_button | pad_button | pad_axis); None if unknown.
	PAWCORE_API PcInputEvent BindingEventOf(const std::string& GodotEvent, int64_t Code, int64_t AxisSign);
	// Godot's InputMap order: its built-in ui_* actions first (in Godot's own order), then the project's, as given.
	PAWCORE_API PcInputState BindingGodotOrder(const PcInputState& State);

	// --- the logical rows (LOGICAL, in the Controls page's order) ----------------------------------------------------------
	enum class PcBindResult : uint8_t { Bound, Swapped, RefusedSystem, RefusedTaken, Unchanged }; // Godot's Result, in order
	enum class PcBindDevice : uint8_t { Keyboard, Pad };

	constexpr int BindingRowCount = 10;
	PAWCORE_API const char* BindingRowId(int Row);      // pounce, tab_prev, tab_next, up, down, press, buy_amount, skill_tree, recentre, pause
	PAWCORE_API const char* BindingRowLabel(int Row);   // LABELS: "Pounce", "Previous tab", ... (the English; ST_UI translates)
	PAWCORE_API int BindingRowIndex(const std::string& Logical); // -1 when it is not a row
	// The row's actions; the first is the one whose binding the Controls page shows.
	PAWCORE_API std::vector<std::string> BindingMembers(const std::string& Logical);
	// LOCKED: Pause on both devices (it IS the system key), Re-centre on the keyboard (pad-only by design).
	PAWCORE_API bool BindingLocked(const std::string& Logical, PcBindDevice Device);

	PAWCORE_API bool BindingSameEvent(const PcInputEvent& A, const PcInputEvent& B); // None equals None only
	PAWCORE_API bool BindingIsSystem(const PcInputEvent& E);                         // Esc, Start, pad B
	PAWCORE_API PcBindDevice BindingDeviceOf(const PcInputEvent& E);                 // a mouse button is the keyboard's
	// The event a row's cell shows for a device: the first of that device's events in the row's first member; None if none.
	PAWCORE_API PcInputEvent BindingEventFor(const std::string& Logical, PcBindDevice Device, const PcInputState& State);

	struct PcBindOutcome
	{
		PcBindResult Result = PcBindResult::Unchanged;
		std::string Other; // Swapped: the row that had the key (and now has the old one)
	};
	// rebind(): `Logical` on the device `Event` belongs to, in the UNtransformed state.
	PAWCORE_API PcBindOutcome BindingRebind(PcInputState& State, const std::string& Logical, const PcInputEvent& Event);
	// reset_row(): the row back to its defaults, and any row that then shares a key with it, until it settles.
	PAWCORE_API void BindingResetRow(PcInputState& State, const PcInputState& Defaults, const std::string& Logical);

	// --- saving ---------------------------------------------------------------------------------------------------------
	PAWCORE_API PcSavedEvent BindingSerialize(const PcInputEvent& E);
	// deserialize(): None for what is not an event (an unknown type, keycode 0). Numbers convert as Godot's int() does.
	PAWCORE_API PcInputEvent BindingDeserialize(const PcSavedEvent& E);
	// working_state(): the defaults, with every saved action that deserializes to at least one event in their place.
	PAWCORE_API PcInputState BindingWorkingState(const PcSavedBindings& Saved, const PcInputState& Defaults);
	// diff_from_defaults(): every action whose events differ from its default, serialized, in the state's order.
	PAWCORE_API PcSavedBindings BindingDiff(const PcInputState& State, const PcInputState& Defaults);

	// --- applying -------------------------------------------------------------------------------------------------------
	// apply(): the live map: the working state (an event an action already has is not added twice, as InputMap does), then
	// Swap sticks (left and right sticks' axes trade) and Swap confirm and back (pad A and B trade in ui_accept, press,
	// ui_cancel, back).
	PAWCORE_API PcInputState BindingApply(const PcSavedBindings& Saved, bool bSwapSticks, bool bSwapConfirm, const PcInputState& Defaults);
	// The steer actions' dead zone apply() sets: the stick dead zone held to 0.05 .. 0.9, stored as Godot's float.
	PAWCORE_API double BindingSteerDeadZone(double DeadZone);

	// --- capturing ------------------------------------------------------------------------------------------------------
	// A raw event as the Controls page sees it while a cell listens.
	struct PcCapturedEvent
	{
		PcInputKind Kind = PcInputKind::None;
		int64_t Code = 0;        // physical keycode / button / axis
		int64_t KeyCode = 0;     // a key: its keycode (Godot falls back to it when the physical one is 0)
		bool bPressed = true;
		bool bEcho = false;
		double AxisValue = 0.0;  // a pad axis
	};
	// binding_from(): a key by position, a mouse button but the wheel, a pad button, a TRIGGER past half way (as +1); None for
	// a release, an echo, a wheel, a stick, drift.
	PAWCORE_API PcInputEvent BindingFrom(const PcCapturedEvent& E);

	// --- what the Controls page says (controls_panel.gd) -------------------------------------------------------------------
	constexpr double BindingListenSec = 5.0; // LISTEN_SEC: a cell listening this long with nothing pressed cancels
}
