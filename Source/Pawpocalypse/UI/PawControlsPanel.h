#pragma once

// Settings > Controls, below the prompt and menu-side rows (Godot controls_panel.gd; its rules are PcControls, proven against
// the real panel in the real dialog, fixture `controls`): Show button prompts and Tutorial tips; the binding table, ONE ROW
// PER LOGICAL ACTION (PcBindings) with a keyboard-and-mouse cell and a controller cell, each drawn as its own prompt, and a
// Reset; the status line; Reset all controls; Swap confirm and back, Swap sticks, Invert X / Y; the cursor speed and stick
// dead zone sliders. Every change goes through UPawSettingsSubsystem (the one writer); the bindings reach the game through
// UPawInputDeviceSubsystem (the live mapping context).
//
// LISTENING: pressing a cell listens; the dialog hands the panel every key, button, click and analog value first while it
// listens (Capture*), so the key being bound does not also move the selection, turn the page or close the dialog. A stick's
// or trigger's digital stand-in is swallowed: the analog value decides (Godot binds a trigger past half way, never a stick).
//
// THE PAGE REDRAWS ONLY WHEN TOLD (Godot's sync): a control setting changed (UPawInputDeviceSubsystem::OnControlsChanged,
// after the bindings were applied), the prompt family changed (OnFamilyChanged), a cell started or stopped listening. The
// controller column keeps the family of the last redraw (F37: a pad detected while the prompts are pinned to the keyboard
// does not redraw it until the next).
//
// A WIDGET BLUEPRINT (WBP_ControlsPanel; PawWidgets.h). The binding rows are the game's data (the logical rows): built at
// run time into BindingTable, never baked. Grafted into WBP_SettingsDialog's Controls page.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"

#include "PcControls.h"

#include "PawControlsPanel.generated.h"

class UPawButton;
class UPawInputDeviceSubsystem;
class UPawPromptIcon;
class UPawSettingsSubsystem;
class UScrollBox;
class USlider;
class UTextBlock;
class UVerticalBox;
struct FAnalogInputEvent;
struct FKeyEvent;

UCLASS()
class PAWPOCALYPSE_API UPawControlsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// What a player does (the controls call these; so do the tests).
	void PressCell(int32 Row, Pc::PcBindDevice Device);
	void PressReset(int32 Row);
	void PressResetAll();
	void ToggleCheck(Pc::PcControlsCheck Check);
	void MoveSlider(Pc::PcControlsSlider Which, double Value);
	// bind(): the gate's direct path (also what a capture ends in).
	Pc::PcBindResult Bind(int32 Row, const Pc::PcInputEvent& Event);

	// While a cell listens, the dialog gives it every input first; true = the panel took it.
	bool IsListening() const { return State.bListening; }
	bool CaptureKey(const FKeyEvent& Event, bool bDown);
	bool CaptureMouse(const FKey& Button, bool bDown);
	bool CaptureAnalog(const FAnalogInputEvent& Event);
	// Any of the above once it is Godot's event (bBack: it presses back or ui_cancel); the tests give it what Unreal cannot
	// deliver (a pad's Guide button).
	bool CaptureEvent(const Pc::PcCapturedEvent& Event, bool bBack);
	// The clock (a core ticker while listening; the tests call it).
	void Advance(double DeltaSeconds);

	// sync(): every cell, line, switch and slider from the live settings, the device and the listening state.
	void Sync();
	// The page's selection rows for the dialog (focus_rows()): a table row is [keyboard, controller, reset], left to right.
	TArray<TArray<UWidget*>> FocusRows() const;

	// What the tests read (and only read).
	const Pc::PcControlsState& GetState() const { return State; }
	FText GetStatusText() const;
	UPawButton* GetCell(int32 Row, Pc::PcBindDevice Device) const;
	UPawPromptIcon* GetCellPrompt(int32 Row, Pc::PcBindDevice Device) const;
	UPawButton* GetReset(int32 Row) const;
	UTextBlock* GetRowLabel(int32 Row) const;
	UPawButton* GetCheck(Pc::PcControlsCheck Check) const;
	USlider* GetSlider(Pc::PcControlsSlider Which) const;
	FText GetSliderLine(Pc::PcControlsSlider Which) const;
	UPawButton* GetResetAll() const { return ResetAllButton; }
	Pc::PcFamily GetDrawnPadColumn() const { return DrawnPadColumn; }
	// The tests' hook: nothing listening, an empty status line (a fixture row's start), redrawn.
	void ClearForTest();

private:
	void OnLayoutReady();
	void BuildRows();
	void StartTicker();
	void StopTicker();
	void EndListen(int32 RowWas, Pc::PcBindDevice DeviceWas);
	void SaveBindings(const Pc::PcSettings& S);
	void RefreshStatus();
	UPawSettingsSubsystem* Settings() const;
	UPawInputDeviceSubsystem* Device() const;
	Pc::PcSettings Current() const;

	UFUNCTION()
	void OnCursorSlider(float Value);
	UFUNCTION()
	void OnDeadzoneSlider(float Value);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> Scroll;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> ShowPromptsCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> TutorialTipsCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyboardHead;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PadHead;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> BindingTable;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Status;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> ResetAllButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SwapConfirmCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SwapSticksCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> InvertXCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> InvertYCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CursorSpeedLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> CursorSpeedSlider;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeadzoneLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> DeadzoneSlider;

	// The rows, built at run time (Row * 2 + device for the cells).
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> RowLabels;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> Cells;
	UPROPERTY()
	TArray<TObjectPtr<UPawPromptIcon>> CellPrompts;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> Resets;

	Pc::PcControlsState State;
	Pc::PcFamily DrawnPadColumn = Pc::PcFamily::Xbox;
	bool bSyncing = false;
	FTSTicker::FDelegateHandle Ticker;
	FDelegateHandle FamilyHandle;
	FDelegateHandle DetectedHandle;
	FDelegateHandle ControlsHandle;
};
