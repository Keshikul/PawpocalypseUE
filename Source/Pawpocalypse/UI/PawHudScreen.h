#pragma once

// The HUD (docs/05 §6.1; Godot hud.gd, and tier_controller.gd's boss panel): the one persistent bottom screen on the Game
// stack. Over the field, never taking a click: the status row (Catnip, Stardust, the wave line, the zone plate) in the
// field's far top corner, the Catches XP bar along the field's bottom, the Pounce clock above it in the corner away from the
// dock, the boss panel centred over the field. Beside the field, the dock: six tabs and their six pages, the selection IS
// focus (explicit navigation, the remembered row follows focus continuously: spike S4).
//
// What each readout says is PawCore's decision (PcHud); this class turns it into string-table text and draws it. Readouts
// refresh at 15 Hz (HUD_REFRESH_HZ); a press refreshes at once. The dock's rect is reported to the realm, which measures
// the field beside it.

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "PcHud.h"
#include "UI/PawJuice.h"

#include "PawHudScreen.generated.h"

class UBorder;
class UCommonActivatableWidgetStack;
class UPawCardPickScreen;
class UPawJobPicker;
class UPawReawakenCeremony;
class UPawVictoryScreen;
class UPawSkillTreeScreen;
class UPawPauseMenu;
class UHorizontalBox;
class UOverlaySlot;
class UPawButton;
class UPawIconText;
class UImage;
class UPawGameSubsystem;
class UPawPounceClock;
class UPawPromptIcon;
class UPawPromptText;
class UPawRainbowBar;
class UPawRainbowFrame;
class UPawRealmSubsystem;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

UCLASS()
class PAWPOCALYPSE_API UPawHudScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// Every readout from the progress and the field, now.
	void RefreshNow();
	void RefreshLegend();
	// One frame's work without Slate: the field's events, the clock, hold-to-buy, the 15 Hz refresh (NativeTick = the dock's
	// geometry, then this). The tests drive it.
	void Pump(double DeltaSeconds);
	// The dock's rect for a HUD of this size (UI units), reported to the realm in field units when it changed.
	void ReportDockForSize(const FVector2D& Size);
	void SelectTab(Pc::PcDockTab Tab);
	Pc::PcDockTab GetActiveTab() const;
	// Whether tab I is shown (a hidden tab cannot be selected by its number key).
	bool IsTabShown(int32 I) const;
	// The dock's prompts (hud.gd _build_dock_prompts): the tab keys, the legend as shown (PawGlyphs::Neutral), the Pounce key.
	UPawPromptIcon* GetTabPrompt(bool bNext) const;
	FString GetDockLegend() const;
	UPawPromptIcon* GetPouncePrompt() const { return PouncePrompt; }
	// The dock (its rows draw their own press prompt outside the row: the focus prompt asks).
	UWidget* GetDockWidget() const;
	// Settings > Controls > Menu side (phase 4 stores it); the one place that knows a side (docs/05 §6.1).
	void SetDockOnLeft(bool bLeft);
	bool IsDockOnLeft() const;
	// The side the dock is DRAWN on (its slot), which follows the setting through OnChanged: the tests read it.
	bool IsDockDrawnOnLeft() const;
	void PressRow(int32 Row);
	void CycleMultiplier();
	// press (dock_navigator.gd): the selected dock button, pressed as a click would; false with nothing pressable selected.
	bool PressSelected();
	int64 GetBuyMultiplier() const;
	// The Realms page: travel, the Bait, the auto-travel box.
	void PressRealm(int32 Index);
	void PressBait();
	void ToggleAutoTravel();
	// The Reawaken page: the two-press confirm (the second press reawakens), its Cancel, the "Ask before" box.
	void PressReawaken();
	void CancelReawaken();
	void ToggleAskBeforeReawaken();
	bool IsReawakenArmed() const { return bReawakenArmed; }
	// The Catches page: the auto-pick box. The Achievements page: one Claim, Claim all.
	void ToggleAutoPick();
	void PressClaim(int32 Index);
	void PressClaimAll();
	// The selectable widgets of the open page, top to bottom: what navigation walks (locked ones included).
	TArray<UWidget*> PageColumn() const;
	// The Menu stack the card pick opens on (the player controller's root layout). While the progress holds a hand, the
	// pick is up (CardManager.hand_offered -> CatchesPickUI): Pump opens it, so a hand dealt by a kill, or one that was up
	// when the realm started, is shown either way. Hold-to-buy stops while it is up (Godot's paused tree stops the dock).
	void TickReawakenGlow();
	void SetMenuStack(UCommonActivatableWidgetStack* Stack);
	UPawCardPickScreen* GetCardPick() const { return CardPick; }
	bool IsCardPickUp() const;
	// The skill tree on the Menu stack: browse (T / pad Y, read-only), or the Reawaken's spend step, after which the run
	// goes to its starting biome. Not over another screen (Godot opened it only while nothing paused the game). False when
	// it did not open (no Menu stack, another screen up).
	bool OpenSkillTree(bool bSpend);
	UPawSkillTreeScreen* GetSkillTree() const { return SkillTree; }
	// The pause menu on the Menu stack (Esc / pad Start; pause_menu.gd _process): only while nothing else is up or holds the
	// field, and not on the frame after something else let go of it (the Esc that closed it must not open this). False when
	// it did not open.
	bool OpenPauseMenu();
	UPawPauseMenu* GetPauseMenu() const { return PauseMenu; }
	// Anything on the Menu stack (the pick, the tree): the dock's hold-to-buy waits.
	bool IsMenuUp() const;
	// The Modal stack the job picker opens on (the player controller's root layout). Without one, a Reawaken travels
	// without asking (as without a picker in the scene).
	void SetModalStack(UCommonActivatableWidgetStack* Stack);
	// A Reawaken, in Godot's order (hud.gd _run_reawaken_ceremony): the payout read, the reset, the ceremony (the HUD's top
	// layer; it runs on the HUD's clock while the field runs), then the spend step on the Menu stack; leaving it asks for
	// the run's job (the Modal stack, when JobPicker.should_ask()), then the run goes to its starting biome.
	// HUD._jobs_seen = -1 (main_menu.gd _on_slot_chosen): another game's count must not badge this one's jobs.
	static void ForgetJobsSeen();
	UPawReawakenCeremony* GetCeremony() const { return Ceremony; }
	UPawJobPicker* GetJobPicker() const { return JobPicker; }
	// THE ENDING (hud.gd _on_boss_defeated_for_victory): on a BossDefeated event flagged Victory, the victory screen on the
	// Modal stack with the first clear's time. Never a second one while one is up (show_victory: `if visible: return`).
	void ShowVictory(double Seconds);
	UPawVictoryScreen* GetVictory() const { return Victory; }
	// The ceremony has ended and the spend step waits for the Menu stack to clear (never in practice: anything on it holds
	// the field, and a held field holds the ceremony).
	bool IsSpendStepPending() const { return bSpendPending; }

	// What the tests read (and only read).
	UPawButton* GetRowButton(int32 Row) const { return Rows.IsValidIndex(Row) ? Rows[Row].Get() : nullptr; }
	UPawButton* GetTabButton(Pc::PcDockTab Tab) const;
	UPawButton* GetMultiplierButton() const { return MultiplierButton; }
	UPawButton* GetRealmButton(int32 Index) const { return RealmRows.IsValidIndex(Index) ? RealmRows[Index].Get() : nullptr; }
	UPawButton* GetBaitButton() const { return BaitButton; }
	UPawButton* GetAutoTravelButton() const { return AutoTravelButton; }
	UPawButton* GetReawakenButton() const { return ReawakenButton; }
	UPawButton* GetCancelReawakenButton() const { return CancelButton; }
	UPawButton* GetAskReawakenButton() const { return AskButton; }
	UPawButton* GetCardRow(int32 Index) const { return CardRows.IsValidIndex(Index) ? CardRows[Index].Get() : nullptr; }
	UPawButton* GetAutoPickButton() const { return AutoPickButton; }
	UPawButton* GetClaimButton(int32 Index) const { return ClaimButtons.IsValidIndex(Index) ? ClaimButtons[Index].Get() : nullptr; }
	UPawButton* GetClaimAllButton() const { return ClaimAllButton; }
	UPawButton* GetJobButton(int32 Index) const { return JobButtons.IsValidIndex(Index) ? JobButtons[Index].Get() : nullptr; }
	FText GetCatchesSummaryText() const;
	FText GetAchievementSummaryText() const;
	// An achievement row's two lines, and its mark's colour (★ ◆ ☆ in Godot: drawn as a disc until phase 6's art).
	FText GetAchievementText(int32 Index, bool bDetail) const;
	FLinearColor GetAchievementMark(int32 Index) const;
	FText GetJobText(int32 Index, bool bDetail) const;
	FText GetStageText() const;
	FText GetReawakenProgressText() const;
	FText GetPayoutText() const;
	FText GetTotalText() const;
	FText GetMultiplierText() const;
	UPawPounceClock* GetPounceClock() const { return PounceClock; }
	UWidget* GetStatusRow() const;
	UWidget* GetDock() const;
	UWidget* GetBossPanel() const;
	FText GetCatnipText() const;
	FText GetStardustText() const;
	FText GetZoneText() const;
	FText GetWaveText() const;
	// The quick bar (phase 6 slice 5b): the presses, and what a test needs to see.
	void PressQuickSettings();
	void PressQuickSkills();
	void PressQuickSave();
	void PressQuickQuit();
	// The quit's last step (a test swaps it for the level change).
	TFunction<void()> OnQuitToTitle;
	UPawButton* GetQuickButton(int32 I) const
	{
		const TObjectPtr<UPawButton> All[] = { QuickSettings, QuickSkills, QuickSave, QuickQuit };
		return I >= 0 && I < 4 ? All[I].Get() : nullptr;
	}
	FText GetQuickStatusText() const;
	// Phase 6 slice 5b: the icons (Pawpocalypse.UI.Icons).
	const UPawIconText* GetWaveLine() const { return WaveLine; }
	const UPawIconText* GetReawakenProgressLine() const { return ReawakenProgressLine; }
	const UPawIconText* GetTotalLine() const { return TotalLine; }
	const UPawIconText* GetAchievementSummaryLine() const { return AchievementSummaryLine; }
	const UImage* GetCurrencyIcon(bool bStardust) const { return bStardust ? StardustIcon.Get() : CatnipIcon.Get(); }
	const UImage* GetPayoutIcon() const { return PayoutIcon; }
	const UImage* GetAchievementIcon(int32 I) const { return AchievementIcons.IsValidIndex(I) ? AchievementIcons[I].Get() : nullptr; }
	const UImage* GetJobIcon(int32 I) const { return JobIcons.IsValidIndex(I) ? JobIcons[I].Get() : nullptr; }
	FText GetBossText() const;
	FText GetBossTimerText() const;
	FText GetXpText(bool bNext) const;
	// The value each bar KEPT (Godot's Range: snapped to its step, in 0..1); the fill drawn follows Godot's width rule.
	float GetZoneFill() const { return static_cast<float>(ZoneKept); }
	float GetXpFill() const { return static_cast<float>(XpKept); }
	bool IsZoneVisible() const;
	UWidget* GetZonePlate() const;
	// The rainbow pieces (phase 6 slice 5d): the Reawaken page's bar, the zone plate's frame.
	UPawRainbowBar* GetReawakenRainbow() const { return ReawakenRainbow; }
	UPawRainbowFrame* GetZoneReady() const { return ZoneReady; }
	// The dock's motion (UiJuice, phase 6 slice 5c): Pump steps it; the tests read and drive it.
	FPawJuice& GetJuice() { return Juice; }
	bool IsWaveVisible() const;
	bool IsBossPanelVisible() const;
	bool IsBossTimerVisible() const;
	// The dock's rect in field units, as last reported to the realm.
	FVector4 GetReportedDock() const { return ReportedDock; }
	int64 GetRefreshCount() const { return Refreshes; }

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual void NativeOnActivated() override;
	virtual void NativeDestruct() override;

private:
	void BuildOverlay(class UOverlay* Root);
	void BuildDock(class UOverlay* Root);
	void BuildUpgradesPage(UVerticalBox* Page);
	void BuildRealmsPage(UVerticalBox* Page);
	void BuildReawakenPage(UVerticalBox* Page);
	void BuildCatchesPage(UVerticalBox* Page);
	void BuildAchievementsPage(UVerticalBox* Page);
	void BuildJobsPage(UVerticalBox* Page);
	class UScrollBox* PageScroll(UVerticalBox* Page, FName Name = NAME_None);
	// The Widget Blueprint contract (PawWidgets.h): wiring in either mode, and the rows made from the data at runtime into
	// their named containers (Page0 for the shop, RealmList, CardList, AchievementList, JobList), never baked.
	void OnLayoutReady();
	void BuildShopRows();
	void BuildRealmRows();
	void BuildCardRows();
	void BuildAchievementRows();
	void BuildJobRows();
	void RefreshRealms();
	void RefreshReawaken();
	void RefreshCatches();
	void RefreshAchievements();
	void RefreshJobs();
	void DoReawaken();
	void OpenSpendStepWhenFree();
	void ReturnToStartingBiome();
	void TravelToStartingBiome();
	UTextBlock* PageText(UVerticalBox* Page, const FLinearColor& Colour, float WrapAt, FName Name = NAME_None);
	void ApplyDockSide();
	void WriteNavigation();
	void RefreshShop();
	void RefreshStatus();
	void RefreshTabs();
	void RefreshField();
	void ReadFieldEvents();
	void TickRepeat(float DeltaSeconds);
	// Whether the dock takes this key before anything else (UPawInputDeviceSubsystem::ConsumeForDock).
	bool DockTakesKey(const FKey& Key) const;
	void RememberFocus();
	UPawRealmSubsystem* Realm() const;
	UPawGameSubsystem* Game() const;
	void OpenCardPickIfDealt();
	// hud.gd _resume_reawaken() (F29): a save made mid-Reawaken picks up at the spend step once nothing else is up.
	void ResumeReawakenIfPending();

	// --- the dock ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> DockBox;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> Tabs;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> Rows;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> MultiplierButton;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> PageTitles;
	// Realms
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StageText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> BaitButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> AutoTravelButton;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> RealmRows;
	// Reawaken
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawIconText> ReawakenProgressLine;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawRainbowBar> ReawakenRainbow; // rainbow_bar.gd (hud.tscn ReawakenBar)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PayoutText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawIconText> TotalLine;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MultiplierText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnicornText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> ReawakenButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> CancelButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> AskButton;
	// Catches
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CatchesSummary;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> AutoPickButton;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> CardRows;
	// Achievements
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawIconText> AchievementSummaryLine;
	// The quick bar
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> QuickBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> QuickSettings;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> QuickSkills;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> QuickSave;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> QuickQuit;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuickStatus;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> ClaimAllButton;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> AchievementNames;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> AchievementDetails;
	UPROPERTY()
	TArray<TObjectPtr<UImage>> AchievementIcons;
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> AchievementMarks;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> ClaimButtons;
	TArray<FLinearColor> AchievementMarkColours;
	// Jobs
	UPROPERTY()
	TArray<TObjectPtr<UImage>> JobIcons;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> JobNames;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> JobDetails;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> JobButtons;

	// --- over the field ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> StatusRow;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> CurrencyBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CatnipIcon;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> StardustIcon;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> PayoutIcon;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CatnipText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StardustText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> WaveBox;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawIconText> WaveLine;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> WaveBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ZonePlate;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawRainbowFrame> ZoneReady; // the plate's rainbow outline (hud.gd zone_rainbow, "ZoneReady")
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ZoneText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ZoneBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> XpRow;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> XpNow;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> XpNext;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> XpBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawPounceClock> PounceClock;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawPromptIcon> PouncePrompt;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawPromptIcon> TabPrevPrompt;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawPromptIcon> TabNextPrompt;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawPromptText> DockLegend;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BossPanel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> BossBar;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossTimer;

	// Slots placed by side.
	UPROPERTY()
	TObjectPtr<UOverlaySlot> DockSlot;
	UPROPERTY()
	TObjectPtr<UOverlaySlot> StatusSlot;
	UPROPERTY()
	TObjectPtr<UOverlaySlot> XpSlot;
	UPROPERTY()
	TObjectPtr<UOverlaySlot> ClockSlot;
	UPROPERTY()
	TObjectPtr<UOverlaySlot> PouncePromptSlot;
	UPROPERTY()
	TObjectPtr<UOverlaySlot> BossSlot;

	TWeakObjectPtr<UCommonActivatableWidgetStack> MenuStack;
	TWeakObjectPtr<UCommonActivatableWidgetStack> ModalStack;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawReawakenCeremony> Ceremony;
	UPROPERTY()
	TObjectPtr<UPawJobPicker> JobPicker;
	UPROPERTY()
	TObjectPtr<UPawVictoryScreen> Victory;
	bool bSpendPending = false;
	UPROPERTY()
	TObjectPtr<UPawCardPickScreen> CardPick;
	UPROPERTY()
	TObjectPtr<UPawSkillTreeScreen> SkillTree;
	UPROPERTY()
	TObjectPtr<UPawPauseMenu> PauseMenu;
	uint64 HeldByOtherFrame = 0;             // the last frame something other than the pause menu held the field (0: never)

	TArray<TWeakObjectPtr<UWidget>> Remembered; // per tab: where the player was (follows focus continuously, spike S4)
	bool bReawakenArmed = false;               // between the first press of Reawaken and its confirmation
	double RefreshClock = 0.0;
	int64 Refreshes = 0;
	FVector4 ReportedDock = FVector4(0, 0, 0, 0);
	FVector2D LastSize = FVector2D::ZeroVector;
	bool bLastLeft = true;

	// The boss panel's after-word: "DEFEATED", "OUT OF TIME". It stays until travel or the next summon (tier_controller.gd:
	// the label outlives the boss until the scene changes).
	FText BossMessage;
	int64 BossMessageTier = -1;
	uint64 LastEventStep = MAX_uint64; // the field step whose events were read
	double ZoneKept = 0.0, XpKept = 0.0; // what the zone and XP bars' Ranges kept
	FPawJuice Juice;                   // the dock's motion: hover pops, the purchase punch, the leans (ui_juice.gd)

	// The dock's held keys (dock_navigator.gd _tick_nav / _tick_press: PcDockRepeat): what is held is the device subsystem's.
	Pc::PcDockRepeat DockRepeat;
	bool bDockPressWas = false;
	// Whether the dock takes its own keys now (dock_navigator.gd is_active: the HUD up, nothing holding the field).
	bool DockActive() const;
	// The selected row (the focused widget of the open page), or null; its index in PageColumn().
	UWidget* DockSelection(int32* OutIndex = nullptr) const;
	void StepDockRow(int32 Direction);
	void RestoreDockSelection();
};
