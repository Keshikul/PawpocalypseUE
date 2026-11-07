#pragma once

// The title screen (docs/05 §2: L_Title's UPawMainMenu; Godot main_menu.tscn / main_menu.gd, proven by fixture `title`).
//
// The menu (Continue, New Game, Load, Leaderboard, Settings, Credits, Exit) on a paper panel under the name, the status
// line, the version; the five save slots (New Game and Load open the same screen: an empty slot starts a game there, an
// occupied one loads it; Delete on a card asks first); the leaderboard placeholder and the credits; Back closes the top
// layer and, on the menu itself, asks to quit. One screen with layers, as Godot's, and exactly one has the keys: the top
// one, whose default takes the selection whenever a layer opens or closes (_focus_top).
//
// SAVE TRAPS (HANDOFF-LOG (34)): a new game is written to its slot in the same frame it is made (SaveGame(slot) makes that
// slot active and reseeds the end-of-frame digest, so the empty game never lands in the slot that was active); a load is
// followed by opening L_Realm, whose realm starts from the loaded game (nothing of the old realm survives a level).
//
// NOT YET, each with its phase: the garden behind the menu, Boba's photo and his cameo on a click, the cluster's icons
// (the art, phase 6: a paper floor, an empty frame and the button's word stand in, at Godot's sizes); the menu music
// (audio, phase 6); the Language list (phase 7: its button is there and disabled). Settings opens the settings dialog on
// the Modal stack; the slot screen opens on the last slot used (the settings' last_slot, which the active slot follows).
//
// A WIDGET BLUEPRINT (PawWidgets.h): WBP_MainMenu owns the layout; the parts below are bound by name (the repeated ones as
// MenuButton0..6, ClusterButton0..2, SlotCard0..4, SlotDelete0..4, SlotText0..4, SlotNumber0..4, SlotSize0..4,
// SlotColumn0..4). OnLayoutReady wires them and fills what is formatted (the version, the hint, the slot numbers).

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "UI/PawJuice.h"

#include "PawMainMenu.generated.h"

class UBorder;
class UCommonActivatableWidgetStack;
class UCanvasPanel;
class UOverlay;
class UPawButton;
class UPawPromptText;
class UPawGameSubsystem;
class UPawSaveSubsystem;
class USizeBox;
class UTextBlock;
class UVerticalBox;

// The cluster's square plates: paper, an ink frame, the band tone on hover, the slot tone pressed (main_menu.gd
// _build_cluster). Never selected: the cluster is mouse-only (focus_mode NONE).
UCLASS()
class PAWPOCALYPSE_API UPawPlateButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPawPlateButtonStyle();
};

UCLASS()
class PAWPOCALYPSE_API UPawMainMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	enum class ELayer : uint8 { Menu, Slots, Leaderboard, Credits, Confirm };
	static constexpr int32 SlotCount = 5;

	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// The seams (Godot's url_opener): what Continue / a slot does once the game is ready, what opens a link, what quits.
	// Defaults: open L_Realm; the platform's browser; request exit. The tests replace them.
	TFunction<void()> OnPlay;
	TFunction<void(const FString&)> OnOpenUrl;
	TFunction<void()> OnQuit;
	// The clock the slot cards measure "last saved" against (Unix seconds); the tests fix it.
	TFunction<double()> Now;

	static const TCHAR* BugReportUrl;
	static const TCHAR* DiscordUrl;

	// What a press does (the buttons call these; so do the tests).
	void PressContinue();
	void PressNewGame();
	void PressLoad();
	void PressLeaderboard();
	void PressSettings();       // the settings dialog, on the Modal stack (SetModalStack); nothing without one
	void SetModalStack(UCommonActivatableWidgetStack* Stack);
	void PressCredits();
	void PressExit();
	void PressSlot(int32 SlotIndex);
	void PressDelete(int32 SlotIndex);
	void PressConfirmYes();
	void PressConfirmNo();
	void PressClose();          // the credits' or the leaderboard's Close
	void PressSlotsBack();
	void PressCluster(int32 Index); // 0 bug, 1 Discord, 2 language
	// Back from the top layer down (_back): the confirm, the credits, the leaderboard, the slots; on the menu, "Quit?".
	void Back();
	// _refresh() and _refresh_slot_cards(): read the game and the slots again.
	void Refresh();

	ELayer GetTopLayer() const;
	bool IsLayerShown(ELayer Layer) const;

	// What the tests read (and only read).
	UPawButton* GetMenuButton(int32 Index) const; // 0 Continue .. 6 Exit
	UPawButton* GetSlotCard(int32 SlotIndex) const;
	UPawButton* GetSlotDelete(int32 SlotIndex) const;
	UPawButton* GetConfirmYes() const { return ConfirmYes; }
	UPawButton* GetConfirmNo() const { return ConfirmNo; }
	UPawButton* GetCloseButton(ELayer Layer) const;
	UPawButton* GetSlotsBack() const { return SlotsBack; }
	UPawButton* GetClusterButton(int32 Index) const;
	FPawJuice& GetJuice() { return Juice; } // the cluster's hover pops (NativeTick steps it)
	FText GetStatusText() const;
	FText GetSlotText(int32 SlotIndex) const;
	FText GetSlotNumberText(int32 SlotIndex) const;
	FText GetLeaderboardText() const;
	FText GetConfirmText() const;
	FText GetVersionText() const;
	// The hint as shown (PawGlyphs::Neutral: a prompt is <g:stem> or <k:label>).
	FString GetHintText() const;
	FText GetNameText() const;
	FText GetCreditsText() const;
	float GetSlotCardWidth(int32 SlotIndex) const;
	const TArray<UWidget*>& GetWired() const { return Wired; }
	UWidget* GetTopDefault() const { return NativeGetDesiredFocusTarget(); }
	// Steps the slot cards' swell by DeltaSeconds (NativeTick does it every frame; the tests drive it).
	void StepSwell(double DeltaSeconds);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
	UPawGameSubsystem* Game() const;
	UPawSaveSubsystem* Saves() const;
	UPawButton* MakeMenuButton(UVerticalBox* Box, int32 Index, const TCHAR* Key, double Height, double Px, bool bFirst);
	UBorder* MakeWindowLayer(UCanvasPanel* Root, FName Name, double Width, double Height, double ScrimAlpha, UVerticalBox*& OutBody);
	void OnLayoutReady();
	void Show(ELayer Layer, bool bShow);
	// _focus_top(): wire the top layer and put the selection on its default.
	void FocusTop();
	void RememberOpener();
	void OpenSlots();
	void AskConfirm(const FText& Prompt, TFunction<void()> Then);
	void RefreshSlotCards();
	void RefreshLeaderboard();
	bool CanTakeFocus() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VersionText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawPromptText> HintPrompts;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> MenuButtons;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> ClusterButtons;
	FPawJuice Juice;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotsLayer;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LeaderboardLayer;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CreditsLayer;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ConfirmLayer;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LeaderboardText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> ConfirmYes;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> ConfirmNo;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> LeaderboardClose;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> CreditsClose;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> SlotsBack;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> SlotCards;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> SlotDeletes;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotTexts;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotNumbers;
	UPROPERTY()
	TArray<TObjectPtr<USizeBox>> SlotSizes;
	UPROPERTY()
	TArray<TObjectPtr<USizeBox>> SlotColumns;
	TArray<float> SlotScale;
	TArray<UWidget*> Wired;
	TWeakObjectPtr<UWidget> MenuReturn;
	TWeakObjectPtr<UCommonActivatableWidgetStack> ModalStack;
	TFunction<void()> ConfirmThen;
	bool bConfirmSafeIsNo = true;
};
