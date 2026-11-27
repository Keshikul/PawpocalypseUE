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
// (audio, phase 6). Settings opens the settings dialog on the Modal stack; the slot screen opens on the last slot used (the
// settings' last_slot, which the active slot follows).
//
// THE LANGUAGE LIST (phase 7; main_menu.gd _open_language_menu, the designer 2026-09-21: "a menu should appear with all
// available languages and the player can click on the one they want"): the cluster's Language opens a paper list above it,
// one row per language offered (PawText::Languages: complete ones; -PawDraftLanguages: unfinished too), each in its own
// name, never translated; the language shown sits on the yellow plate with the tick. A row saves the language, shows it at
// once and closes the list; Back or a click anywhere else closes it. A layer like the others: its rows have the keys
// while it is up, the selection starting on the language shown. The concept draws only the button: the list follows the
// kit (a card, rows, the yellow "you are here").
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
class UImage;
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
	enum class ELayer : uint8 { Menu, Slots, Leaderboard, Credits, Confirm, Language };
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
	// Quits (saving first when there is something to keep). The Exit button and Back ask first: AskQuit.
	void PressExit();
	void AskQuit();
	void PressSlot(int32 SlotIndex);
	void PressDelete(int32 SlotIndex);
	void PressConfirmYes();
	void PressConfirmNo();
	void PressClose();          // the credits' or the leaderboard's Close
	void PressSlotsBack();
	void PressCluster(int32 Index); // 0 bug, 1 Discord, 2 language (opens or closes the list)
	// A language row (_on_language_chosen): the setting saved and shown, the list closed, this screen's words again.
	void PressLanguage(const FString& Code);
	// The list closed, as Back or a click outside it does (_close_language_menu).
	void CloseLanguageMenu();
	// A press at ScreenPosition that no row took while the list is up (the clear scrim's): off the list's card it closes
	// the list; on it (its edge, its scroll bar) nothing. False when the list is not up.
	bool PressOffLanguageMenu(const FVector2D& ScreenPosition);
	// Back from the top layer down (_back): the confirm, the credits, the leaderboard, the slots; on the menu, "Quit?".
	void Back();
	// _refresh() and _refresh_slot_cards(): read the game and the slots again.
	void Refresh();

	ELayer GetTopLayer() const;
	bool IsLayerShown(ELayer Layer) const;
	// P4.3: Boba's card: a click brings her over (the tests press it); its photo, the card itself.
	bool PressBoba();
	void ShowBobaPhoto();
	UImage* GetBobaPhoto() const;
	UWidget* GetBobaCard() const;

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
	// D18 (S4): the cat in a slot card's photo well (its texture's name; empty when none is shown) and the well's fill.
	FString GetSlotCatName(int32 SlotIndex) const;
	FLinearColor GetSlotPhotoColour(int32 SlotIndex) const;
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
	// The language list's rows, in its order, and the language each sets (Godot's codes).
	const TArray<TObjectPtr<UPawButton>>& GetLanguageRows() const { return LanguageRows; }
	const TArray<FString>& GetLanguageCodes() const { return LanguageCodes; }
	// Steps the slot cards' swell by DeltaSeconds (NativeTick does it every frame; the tests drive it).
	void StepSwell(double DeltaSeconds);
	// Whether a slot card swells: hovered, or selected by the keys or a pad (the ring's rule): the focus the slots open with
	// does not swell slot 1 under a mouse (playtest 2026-11-26, note 6).
	static bool SlotSwells(bool bHovered, bool bSelected, bool bLastWasPointer);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	// The clear scrim behind the language list: a press anywhere off the list closes it (Godot's LanguageScrim).
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

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
	void OpenLanguageMenu();
	// _fill_language_menu(): one row per language offered, rebuilt on every open.
	void FillLanguageMenu();
	// _on_language_changed(): this screen's code-built words again, in the language shown.
	void OnSettingsChanged();

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
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LanguageLayer;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LanguageMenu;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> LanguageList;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> LanguageRows;
	TArray<FString> LanguageCodes;
	FString ShownCulture; // the culture this screen's words were last made in (OnSettingsChanged)
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> SlotCards;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> SlotDeletes;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> SlotTexts;
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> SlotPhotos;
	UPROPERTY()
	TArray<TObjectPtr<class UImage>> SlotCats;
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
	bool bContinueOpensSlots = false; // F47: the active slot cannot be read (Refresh)
};
