#pragma once

// The pause menu (docs/05 §6.3; Godot pause_menu.tscn / pause_menu.gd; its rules are PcPause, proven by fixture `pause`).
//
// Continue, Save (the five slots: an occupied one asks for a second press), Settings, Back to Main Menu (a second press saves
// and goes to the title; "Quit WITHOUT saving" shows beside it while it is armed). A Menu-stack screen that holds the field
// (pause holder "pause"). Opened by the HUD on Esc / Start when nothing else is up and the field runs; closed by Esc / B /
// Start, and on the Save page Esc and B go back to the main page first.
//
// A WIDGET BLUEPRINT from the start (WBP_PauseMenu; PawWidgets.h): the parts below are bound by name (SlotButton0..4).
//
// Settings opens the settings dialog on the Modal stack; this menu steps aside (hidden, still holding the field) and comes
// back when it closes, the selection on Settings.
//
// AS GODOT HAS IT: an armed overwrite survives Back to the main page (finding F34, open). NOT YET: the "CEO on duty" photo
// beside the window and Boba's pause visit (phase 6).

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"

#include "PcPause.h"

#include "PawPauseMenu.generated.h"

class UBorder;
class UCommonActivatableWidgetStack;
class UPawButton;
class UPawRealmSubsystem;
class UPawSaveSubsystem;
class UTextBlock;
class UVerticalBox;

UCLASS()
class PAWPOCALYPSE_API UPawPauseMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static const FName PauseHolder; // "pause"
	static constexpr int32 SlotCount = 5;

	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// What leaving for the title does (default: open L_Title); the tests replace it.
	TFunction<void()> OnLeave;

	// The presses (the buttons call these; so do the tests).
	void PressContinue();
	void PressSave();
	void PressSettings();
	void PressMainMenu();
	void PressDiscard();
	void PressSlot(int32 SlotIndex);
	void PressSlotsBack();
	// Esc / B (bBack) or Start (pause): on the Save page Back returns to the main page, anything else closes.
	void PressBackOrPause(bool bBack);

	const Pc::PcPauseState& GetState() const { return State; }
	// The Modal stack the settings dialog opens on (the HUD hands it over). Without one Settings does nothing.
	void SetModalStack(UCommonActivatableWidgetStack* Stack);
	// The dialog is up and this menu has stepped aside for it.
	bool IsSettingsUp() const { return bSettingsUp; }
	bool IsSlotsPage() const { return State.bSlotsPage; }

	// What the tests read (and only read).
	UPawButton* GetButton(const TCHAR* Name) const;
	UPawButton* GetSlotButton(int32 SlotIndex) const;
	FText GetStatusText() const;
	FText GetTitleText() const;
	// The page the player sees: the main page, or the Save page (bSlots).
	bool IsPageShown(bool bSlots) const;
	const TArray<UWidget*>& GetWired() const { return Wired; }

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
	void OnLayoutReady();
	void Close();
	void Leave(bool bSave);
	void ShowPage();
	void RefreshSlots();
	void RefreshMain();
	void FocusPage(UWidget* On);
	bool CanTakeFocus() const;
	UPawRealmSubsystem* Realm() const;
	UPawSaveSubsystem* Saves() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Scrim;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Window;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Title;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> MainPage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> SlotPage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> ContinueButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SaveButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SettingsButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> MainMenuButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> DiscardButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotTitle;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> SlotList;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SlotBackButton;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> SlotButtons;

	Pc::PcPauseState State;
	bool bLastSaveFailed = false; // the status line's "Saved" was not (review CR15): "Slot {n} was not saved."
	TArray<UWidget*> Wired;
	TWeakObjectPtr<UCommonActivatableWidgetStack> ModalStack;
	bool bSettingsUp = false;
	ESlateVisibility VisibleAs = ESlateVisibility::SelfHitTestInvisible;
	// The button the selection comes back to after Settings: Common UI re-applies the desired target once the dialog is gone
	// (it overrode a SetFocus); cleared when the menu opens again (it opens on Continue).
	TWeakObjectPtr<UWidget> ReturnFocus;
};
