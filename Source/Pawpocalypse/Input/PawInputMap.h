#pragma once

// Godot's input map as the port holds it (docs/05 §3): Data/InputMap.csv (tools/export_input_map.py, one row per Godot event)
// and the names of the assets made from it (PawpocalypseEditor UPawInputAssetBuilder): IA_<Action> per Godot action, one
// mapping context IMC_Pawpocalypse. The builder and the tests read the CSV through this, so both agree on every name.

#include "CoreMinimal.h"

#include "PcBindings.h"

class UInputAction;
class UInputMappingContext;
namespace Pc { struct PcSettings; }

struct PAWPOCALYPSE_API FPawInputMapRow
{
	FString Id;
	FString Action;       // the Godot action (lower-case, as Godot names it)
	float DeadZone = 0.5f;
	FString Device;       // keyboard | mouse | pad
	FString GodotEvent;   // key | mouse_button | pad_button | pad_axis
	int64 GodotCode = 0;
	int32 GodotAxisSign = 0;
	FName UeKey;          // the FKey
	int32 UeAxisSign = 0; // pad_axis: the half of Unreal's axis that counts
	bool bMerged = false; // the same Unreal key as another row of the action (keypad Enter is Enter): no mapping of its own
};

namespace PawInput
{
	PAWPOCALYPSE_API extern const TCHAR* const ActionsDir;   // /Game/Pawpocalypse/Input/Actions
	PAWPOCALYPSE_API extern const TCHAR* const ContextPath;  // /Game/Pawpocalypse/Input/IMC_Pawpocalypse

	// Data/InputMap.csv under the project.
	PAWPOCALYPSE_API FString InputMapCsv();
	// Reads the CSV; false (with Error) when it is missing or malformed.
	PAWPOCALYPSE_API bool ReadInputMap(const FString& Path, TArray<FPawInputMapRow>& Out, FString& Error);
	// "tab_prev" -> "IA_TabPrev", "tab_1" -> "IA_Tab1".
	PAWPOCALYPSE_API FString ActionAssetName(const FString& GodotAction);
	// Whether an action is read as an axis (any of its events is a stick or trigger axis): Axis1D, else Boolean.
	PAWPOCALYPSE_API bool IsAxisAction(const TArray<FPawInputMapRow>& Rows, const FString& GodotAction);

	// The loaded assets (null when they have not been generated).
	PAWPOCALYPSE_API UInputMappingContext* LoadContext();
	PAWPOCALYPSE_API UInputAction* LoadAction(const FString& GodotAction);

	// --- remapping (docs/05 §3; PawCore PcBindings, Godot InputBindings.apply) --------------------------------------------
	// Godot's map as the defaults remapping starts from (PawInputDefaults.h, generated from Data/InputMap.csv: keypad Enter
	// included), in Godot's InputMap order.
	PAWPOCALYPSE_API const Pc::PcInputState& DefaultState();
	// An action's dead zone as the map gives it (Godot's default 0.5 for one it does not know).
	PAWPOCALYPSE_API float DefaultDeadZone(const FString& GodotAction);
	// apply(): the live map from the player's bindings and switches (Swap sticks, Swap confirm and back, the stick dead zone)
	// made into the LIVE mapping context, every cached key answer forgotten, OnBindingsChanged told. The caller asks each
	// local player's Enhanced Input to rebuild (UPawInputDeviceSubsystem does).
	PAWPOCALYPSE_API void ApplyBindings(const Pc::PcSettings& Settings);
	// The live map apply() made (Godot events); the defaults until it runs.
	PAWPOCALYPSE_API const Pc::PcInputState& LiveState();
	// The context the game adds and every key question below reads: one transient context, rebuilt in place by apply().
	// Until apply() has run it holds IMC_Pawpocalypse's mappings.
	PAWPOCALYPSE_API UInputMappingContext* LiveContext();
	// The live map changed (the navigation config is rebuilt; the device subsystem redraws the prompts after it).
	PAWPOCALYPSE_API FSimpleMulticastDelegate& OnBindingsChanged();

	// --- the live bindings: what a key IS (Godot's event.is_action / Input.is_action_just_pressed) -------------------------
	// Read from the live context (LiveContext). A screen asks these with the Slate key event it got, so the focus path stays
	// Slate's and every screen follows the map, remapped or not.
	//
	// The Godot actions bound to Key. A stick or trigger pushed past Slate's threshold arrives as a DIGITAL key
	// (Gamepad_LeftStick_Up, Gamepad_RightTrigger): it counts as the half of the axis it stands for.
	PAWPOCALYPSE_API const TArray<FString>& ActionsOfKey(const FKey& Key);
	PAWPOCALYPSE_API bool KeyIs(const FKey& Key, const TCHAR* GodotAction);
	// Every key bound to an action, in the map's order; with bStandIns, also the digital stand-ins of its axis halves.
	PAWPOCALYPSE_API TArray<FKey> KeysOf(const TCHAR* GodotAction, bool bStandIns = true);
	// The steer actions' keys (KeysOf steer_left, steer_right, steer_up, steer_down without stand-ins, each key once, in that
	// order), kept with the bindings and rebuilt with them: the realm samples them every frame (review CR17: four arrays a
	// frame before).
	PAWPOCALYPSE_API const TArray<FKey>& SteerKeys();
	// InputMap.has_action: the context maps this action (a prompt template's {token} is replaced only then).
	PAWPOCALYPSE_API bool HasAction(const FString& GodotAction);
	// The digital key Slate sends for one half of an axis (Gamepad_LeftY, +1 -> Gamepad_LeftStick_Up); false if none.
	PAWPOCALYPSE_API bool DigitalOfHalf(const FKey& Axis, int32 Sign, FKey& OutDigital);
	// Forget the cached bindings (they are read again on the next ask).
	PAWPOCALYPSE_API void ResetBindings();

	// --- Godot's Input.get_vector (the steer stick: the cat, slice 3; the skill tree's pan) -------------------------------
	// The raw strength an action takes from an axis key's value: |value| when the key is one of the action's halves and points
	// that way, else 0 (Godot's get_action_raw_strength: no dead zone).
	PAWPOCALYPSE_API float RawStrength(const TCHAR* GodotAction, const FKey& AxisKey, float Value);
	// Godot's get_action_strength from one axis key's value: the strength past the action's dead zone when the key is one of
	// its halves (UPawHalfAxis::Strength), else 0. A key's own strength while held is 1 (the caller's).
	PAWPOCALYPSE_API float Strength(const TCHAR* GodotAction, const FKey& AxisKey, float Value);
	// Whether the key is the digital stand-in of an axis half (Gamepad_RightTrigger, Gamepad_LeftStick_Up): a screen reading
	// strength takes the axis's analog value instead.
	PAWPOCALYPSE_API bool IsStandIn(const FKey& Key);
	// An action's dead zone (its half-axis modifier's; Godot's default 0.5 for an action with no axis).
	PAWPOCALYPSE_API float DeadZoneOf(const TCHAR* GodotAction);
	// Input.get_vector(neg_x, pos_x, neg_y, pos_y) from the four raw strengths, in float as Godot: a circular dead zone
	// (Godot: the four actions' average) rescaled to 1, never longer than 1. +Y is DOWN, as Godot's. Measured: fixture
	// `input_axis` (input_vector_godot.tsv, Pawpocalypse.Input.Vector).
	PAWPOCALYPSE_API FVector2f GodotVector(float NegX, float PosX, float NegY, float PosY, float DeadZone);
	// The steer stick from the axis values a screen has seen (Unreal's axes, key -> value): get_vector over steer_*.
	PAWPOCALYPSE_API FVector2f SteerVector(const TMap<FKey, float>& AxisValues);
	// Whether a key is one of the steer actions' axes (a screen keeps its value for SteerVector).
	PAWPOCALYPSE_API bool IsSteerAxis(const FKey& Key);
	// Settings > Controls > stick dead zone on the steer actions (InputBindings.apply: clamped to 0.05 .. 0.9). Negative: the
	// map's own (0.2).
	PAWPOCALYPSE_API void SetSteerDeadZone(float DeadZone);
	// Whether an axis value presses an action (Godot's is_action_pressed for that event; UPawHalfAxis::Pressed).
	PAWPOCALYPSE_API bool IsPressed(const TCHAR* GodotAction, const FKey& AxisKey, float Value);

	// player.gd stick_velocity: the cat's velocity (pixels a second) for a steer deflection (get_vector's output) and the
	// field's width: a full push crosses the field in StickCrossingSec, speed grows with the SQUARE of the deflection. In float,
	// as Godot's Vector2; measured (fixture `input_device`, input_velocity_godot.tsv).
	constexpr float StickCrossingSec = 1.2f; // STICK_CROSSING_SEC
	constexpr double StickCurve = 2.0;       // STICK_CURVE
	PAWPOCALYPSE_API FVector2f StickVelocity(const FVector2f& Input, float FieldWidth);
}
