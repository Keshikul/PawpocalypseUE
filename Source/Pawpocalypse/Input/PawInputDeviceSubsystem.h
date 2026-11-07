#pragma once

// Which device the player is using (docs/05 §4-5; Godot InputDevice): PawCore's PcDevice, fed every Slate input event before
// any widget sees it (an input preprocessor that consumes nothing, as Godot's _input watches), and what it decides acted on:
//   the prompt FAMILY (OnFamilyChanged: the prompts redraw; Settings > Controls > Button prompts pins it);
//   the CURSOR SOURCE (the realm view steers the cat with the stick or follows the mouse);
//   the OS pointer hidden over the open field and while the stick steers (the cat IS the cursor there);
//   a toast on a pad connected (after the boot window) or the pad in hand disconnected;
//   the warp echo (the cat warps the pointer onto itself while the stick steers: that motion is not a hand on the mouse);
//   THE PAW CURSOR (input_device.gd _build_paw_cursor / _apply_paw): the OS pointer is a paw, open, closed with claws while
//   the left button is held; hardware cursors (a widget cursor lags), the arrow and the pointing hand both, from the loose
//   PNGs in Content/Pawpocalypse/UI/Cursors (tools/raster_svg.py --group cursor; staged). Never in the editor or without
//   rendering (Godot: never headless).
//
// Pads on Windows: Unreal reads XInput only (KB U/08 §5.1), and XInput names every pad "XInput Controller": a pad is the Xbox
// family (Godot's family_from_info on that name) until Steam Input says otherwise (doc 10 §5). The Button prompts override is
// the answer for a DualSense meanwhile.
//
// In the editor (PIE) the preprocessor is not installed (it would watch the editor's own input); the tests call the Observe*
// entries directly.

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "PcBindings.h"
#include "PcDevice.h"

#include "PawInputDeviceSubsystem.generated.h"

class IInputProcessor;
class UPawPromptIcon;
class UPawToast;
class UWidget;
struct FAnalogInputEvent;
struct FKeyEvent;
struct FPointerEvent;

UCLASS()
class PAWPOCALYPSE_API UPawInputDeviceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const Pc::PcDeviceState& GetState() const { return State; }
	Pc::PcFamily GetFamily() const { return State.Family; }
	bool IsStickSteering() const { return State.Cursor == Pc::PcCursorSource::Stick; }
	bool IsPointerHidden() const { return State.bPointerHidden; }
	bool LastWasPointer() const { return State.bLastWasPointer; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFamilyChanged, Pc::PcFamily);
	FOnFamilyChanged OnFamilyChanged;
	// A setting changed and this subsystem has acted on it (the bindings applied, the prompt style read): what draws a prompt
	// or a binding listens HERE, not to the settings, so it never redraws from the map before the remap reached it (Godot's
	// InputDevice.bindings_changed).
	FSimpleMulticastDelegate OnControlsChanged;
	// How many times the bindings were applied (InputBindings.apply; the tests read it).
	int32 GetBindingsApplied() const { return BindingsApplied; }

	// What the preprocessor calls (and the tests). Each returns nothing and consumes nothing.
	void ObserveKey(const FKeyEvent& Event, bool bDown);
	void ObserveAnalog(const FAnalogInputEvent& Event);
	void ObserveMouseMove(const FPointerEvent& Event);
	void ObserveMouseButton(const FPointerEvent& Event, bool bDown);
	void ObserveConnection(int32 Device, bool bConnected);

	// The cat warps the pointer onto itself while the stick steers (player.gd _warp_pointer_here).
	void NoteWarp();
	// R3: the stick takes the cursor without an axis event.
	void ClaimStick();
	// The modal guard: may a press count for a modal that opened at OpenedMs (this subsystem's clock)?
	bool PressCountsFor(int64 OpenedMs) const;
	// The pointer is over the open field (the realm view answers every frame).
	void SetPointerOverField(bool bOver);
	// This subsystem's clock, in milliseconds (Godot's Time.get_ticks_msec); the tests replace it.
	TFunction<int64()> Clock;
	int64 NowMs() const;

	// THE PRESS PROMPT ON THE FOCUSED BUTTON (input_device.gd focus_prompt; the dock's dock_navigator.gd _row_prompt): shown
	// while keys or a pad drive the menus (not the pointer), on a button with focus. In the dock, which draws its own,
	// OUTSIDE the row on the field side (designer 2026-10-01: never on the words); anywhere else inside the button's right
	// edge. One prompt on the top layer, so every screen has it without wiring.
	void SetDockArea(UWidget* Dock, bool bDockOnLeft);
	// Where it goes for a button at TopLeft of Size (any one space), the prompt being PromptSize.
	static FVector2D FocusPromptAt(const FVector2D& TopLeft, const FVector2D& Size, const FVector2D& PromptSize, bool bInDock, bool bDockOnLeft);
	// Godot's test: a button (BaseButton) with focus, and the last thing used was not the pointer.
	static bool FocusPromptShown(bool bFocusedButton, bool bLastWasPointer) { return bFocusedButton && !bLastWasPointer; }
	UPawPromptIcon* GetFocusPrompt() const { return FocusPrompt; }

	// The tests' hook: the pad last detected (Godot's fixtures name families XInput never reports), the family refreshed as a
	// claim would (OnFamilyChanged when it moves).
	void SetDetectedForTest(Pc::PcFamily Detected);

	// Godot's Input.is_action_pressed: a key or pad button of the action held, or one of its axis halves past its dead zone
	// (the analog value, never Slate's digital stand-in). Fed by every event observed; forgotten when the game loses focus.
	bool IsActionHeld(const TCHAR* Action) const;
	// Whether a key of the action went down since the last ask (and forget it): a tap that came and went between two polls
	// still counts as a press (Input.is_action_just_pressed).
	bool TakeActionDown(const TCHAR* Action);
	// THE DOCK TAKES ITS KEYS FIRST (dock_navigator.gd _input consumes them): while this answers true for a key, the event is
	// consumed here, before any widget, Slate's navigation or Common UI's analog cursor (which would turn pad A into a click:
	// a second press). This subsystem's processor is the FIRST registered. The HUD sets it.
	TFunction<bool(const FKey&)> ConsumeForDock;
	bool ConsumesKey(const FKey& Key) const { return ConsumeForDock && ConsumeForDock(Key); }
	// The paw: closed while the left button is held (the tests read it; the engine's cursor follows only in a game).
	bool IsPawClosed() const { return bPawClosed; }
	int32 GetPawApplied() const { return PawApplied; }
	// input_device.gd: PAW_PX 40 and PAW_HOTSPOT (20, 4) -> the engine's hotspot, a fraction of the image.
	static constexpr double PawPx = 40.0;
	static FVector2D PawHotSpot() { return FVector2D(20.0 / PawPx, 4.0 / PawPx); }
	static FName PawCursorPath(bool bClosed);

	// The toast on screen, if any (the tests read it).
	FText GetToastText() const;
	bool IsToastShown() const;

	// _watch_for_pad_audio(): a pad connected after the boot window starts a watch: the output devices now, and again
	// Pc::DevicePadAudioWaitMs later (this subsystem's clock, real time: a paused game too); a device that appeared while the
	// game follows the system's says where to fix it (Pc::DevicePadAudioToast). Each connect watches on its own. The tick
	// runs the watches that are due; so do the tests.
	void RunPadAudioWatches();
	int32 GetPadAudioWatches() const { return PadAudioWatches.Num(); }

private:
	void Changed(Pc::PcFamily Before, Pc::PcCursorSource CursorBefore, bool bHiddenBefore, Pc::PcDeviceToast ToastBefore);
	void ApplyPointer();
	void ShowToast(Pc::PcDeviceToast Toast);
	void ReadSettings();
	void Observe(const Pc::PcDeviceEvent& E);
	bool Tick(float DeltaSeconds);
	void PlaceFocusPrompt();

	Pc::PcDeviceState State;
	// What the bindings were last applied from (apply() runs again only when one of these changes: controls_changed).
	bool bBindingsApplied = false;
	Pc::PcSavedBindings AppliedBindings;
	bool bAppliedSwapSticks = false;
	bool bAppliedSwapConfirm = false;
	double AppliedDeadzone = 0.0;
	int32 BindingsApplied = 0;
	TSet<FKey> HeldKeys;
	TArray<FKey> DownsSinceAsked;
	TMap<FKey, float> AxisNow;
	FDelegateHandle ActivationHandle;
	bool bPawClosed = false;
	bool bPawReady = false;
	int32 PawApplied = 0;
	void ApplyPaw();
	TSharedPtr<IInputProcessor> Processor;
	FTSTicker::FDelegateHandle TickHandle;
	FDelegateHandle ConnectionHandle;
	UPROPERTY()
	TObjectPtr<UPawToast> Toast;
	UPROPERTY()
	TObjectPtr<UPawPromptIcon> FocusPrompt;
	TWeakObjectPtr<UWidget> DockArea;
	bool bDockLeft = false;
	double ToastLeft = 0.0;
	struct FPadAudioWatch
	{
		std::vector<std::string> Before;
		int64 DueMs = 0;
	};
	TArray<FPadAudioWatch> PadAudioWatches;
	std::vector<std::string> OutputDevices() const;
};
