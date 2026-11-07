#pragma once

// The settings popup (docs/05 §6.3, docs/08 §3; Godot settings_dialog.tscn / settings_dialog.gd; its rules are PcSettings,
// proven by fixture `settings`). Three sections, Audio / Video / Controls, a page each, and Close. Every control writes
// through UPawSettingsSubsystem (the one writer); nothing here touches the engine.
//
// On the Modal stack, pause holder "settings". Opened by the pause menu (which steps aside until it closes) and the title's
// Settings. Esc / pad B close it; Page Up / Down and LB / RB turn the section (not A / D or the arrows: those move a slider).
// The selection: the section bar on top, the page's controls one per row, Close; it opens on the page's first control.
//
// A WIDGET BLUEPRINT from the start (WBP_SettingsDialog; PawWidgets.h): the parts below are bound by name.
//
// AS GODOT HAS IT: Resolution is disabled while fullscreen as the dialog OPENS (ticking Fullscreen does not grey it until the
// next open); a stored volume out of 0..1 (a hand-edited file) shows as such on its line while its slider sits at the end.
// The Controls page ends in the Controls panel (UPawControlsPanel, controls_panel.gd: the binding table, the stick switches
// and sliders). While one of its cells listens the dialog hands it every key, click and analog value first (Godot: the panel
// takes the event before the dialog's own Back and page keys), so Esc cancels the listening, not the dialog.
// The Output list: "Follow the system", the system's devices (UPawAudioSubsystem::OutputDeviceList), and the stored one if it
// is gone (unplugged since it was saved); picking one plays there at once (the audio subsystem applies it).
// NOT YET, with its phase: Language is "System", disabled (phase 7).

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Components/ComboBoxString.h"

#include "PcSettings.h"

#include "PawSettingsDialog.generated.h"

class UBorder;
class UComboBoxString;
class UCommonActivatableWidgetStack;
class UPawButton;
class UPawControlsPanel;
class UPawSettingsSubsystem;
class USlider;
class UTextBlock;
class UVerticalBox;

// A list (Godot's OptionButton) in the game theme: a theme button's plate, the theme's popup panel, the 16 px label. Its
// look is set where a combo box allows it, in the constructor (the font and colour are fixed before its Slate exists).
UCLASS()
class PAWPOCALYPSE_API UPawOption : public UComboBoxString
{
	GENERATED_BODY()

public:
	UPawOption(const FObjectInitializer& ObjectInitializer);
};

UCLASS()
class PAWPOCALYPSE_API UPawSettingsDialog : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static const FName PauseHolder; // "settings"

	// Pushes the dialog on a Modal stack; OnClosed runs once when it closes, however it was closed. Null without a stack.
	static UPawSettingsDialog* Open(UCommonActivatableWidgetStack* Modal, TFunction<void()> OnClosed);

	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	TFunction<void()> OnClosed;

	// What a player does (the controls call these; so do the tests).
	void PressSection(Pc::PcSettingsSection Section);
	void StepSection(int64 Direction);
	void PressClose();
	void MoveSlider(Pc::PcSettingsSlider Which, double Value);
	void ToggleMute();
	void ToggleFullscreen();
	void ToggleVsync();
	void ToggleEffects();
	void PickResolution(int32 Index);
	void PickQuality(int32 Index);
	void PickFps(int32 Index);
	void PickPrompts(int32 Index);
	void PickDock(int32 Index);
	void PickOutput(int32 Index);

	// What the tests read (and only read).
	const Pc::PcSettingsDialog& GetDialogState() const { return Dialog; }
	FText GetVolumeLine(Pc::PcSettingsSlider Which) const;
	double GetSliderValue(Pc::PcSettingsSlider Which) const;
	bool IsPageShown(Pc::PcSettingsSection Section) const;
	UWidget* GetPart(const TCHAR* Name) const;
	UPawButton* GetCheck(const TCHAR* Name) const;
	UComboBoxString* GetOption(const TCHAR* Name) const;
	// Whether a list wears the selection ring (the dialog paints it over the list's plate: hovered, as a button's).
	bool OptionShowsRing(const UComboBoxString* List) const;
	UPawControlsPanel* GetControlsPanel() const { return ControlsPanel; }
	FText GetTitleText() const;
	const TArray<UWidget*>& GetWired() const { return Wired; }

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& Geometry, const FAnalogInputEvent& Event) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

private:
	void OnLayoutReady();
	void Sync();
	void FillOptions();
	void ShowSection();
	void FocusPage();
	void RefreshVolumeLines();
	void ShowPicked(UComboBoxString* List, int64 Index);
	bool CanTakeFocus() const;
	UPawSettingsSubsystem* Settings() const;
	UVerticalBox* PageOf(Pc::PcSettingsSection Section) const;
	// A cell of the Controls panel listens, on the page shown (is_visible_in_tree): the panel takes every input first.
	bool PanelListening() const;

	UFUNCTION()
	void OnMasterSlider(float Value);
	UFUNCTION()
	void OnMusicSlider(float Value);
	UFUNCTION()
	void OnSfxSlider(float Value);
	UFUNCTION()
	void OnOutputPicked(FString Item, ESelectInfo::Type How);
	UFUNCTION()
	void OnResolutionPicked(FString Item, ESelectInfo::Type How);
	UFUNCTION()
	void OnQualityPicked(FString Item, ESelectInfo::Type How);
	UFUNCTION()
	void OnFpsPicked(FString Item, ESelectInfo::Type How);
	UFUNCTION()
	void OnPromptsPicked(FString Item, ESelectInfo::Type How);
	UFUNCTION()
	void OnDockPicked(FString Item, ESelectInfo::Type How);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Scrim;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Window;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Title;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SectionAudio;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SectionVideo;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SectionControls;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> AudioPage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VideoPage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ControlsPage;
	// Audio
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VolumeLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> VolumeSlider;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MusicLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> MusicSlider;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SfxLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USlider> SfxSlider;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> MuteCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OutputLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> OutputOption;
	// Video
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> LanguageOption;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResolutionLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ResolutionOption;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> FullscreenCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> VsyncCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QualityLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> QualityOption;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaxFpsLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> MaxFpsOption;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> EffectsCheck;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VideoNote;
	// Controls
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptsLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> PromptsOption;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptsNote;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DockSideLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> DockSideOption;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DockSideNote;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawControlsPanel> ControlsPanel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> CloseButton;

	Pc::PcSettingsDialog Dialog;
	TArray<FString> OutputDevices; // the Output list's devices by name, in its order
	bool bSyncing = false;         // the controls are being set from the settings: no change is the player's
	TArray<UWidget*> Wired;
};
