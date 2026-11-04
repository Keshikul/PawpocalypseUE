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

#include "PawHudScreen.generated.h"

class UBorder;
class UCommonActivatableWidgetStack;
class UPawCardPickScreen;
class UPawJobPicker;
class UPawReawakenCeremony;
class UPawVictoryScreen;
class UPawSkillTreeScreen;
class UHorizontalBox;
class UOverlaySlot;
class UPawButton;
class UPawGameSubsystem;
class UPawPounceClock;
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
	// One frame's work without Slate: the field's events, the clock, hold-to-buy, the 15 Hz refresh (NativeTick = the dock's
	// geometry, then this). The tests drive it.
	void Pump(double DeltaSeconds);
	// The dock's rect for a HUD of this size (UI units), reported to the realm in field units when it changed.
	void ReportDockForSize(const FVector2D& Size);
	void SelectTab(Pc::PcDockTab Tab);
	Pc::PcDockTab GetActiveTab() const;
	// Settings > Controls > Menu side (phase 4 stores it); the one place that knows a side (docs/05 §6.1).
	void SetDockOnLeft(bool bLeft);
	bool IsDockOnLeft() const;
	void PressRow(int32 Row);
	void CycleMultiplier();
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
	void SetMenuStack(UCommonActivatableWidgetStack* Stack);
	UPawCardPickScreen* GetCardPick() const { return CardPick; }
	bool IsCardPickUp() const;
	// The skill tree on the Menu stack: browse (T / pad Y, read-only), or the Reawaken's spend step, after which the run
	// goes to its starting biome. Not over another screen (Godot opened it only while nothing paused the game). False when
	// it did not open (no Menu stack, another screen up).
	bool OpenSkillTree(bool bSpend);
	UPawSkillTreeScreen* GetSkillTree() const { return SkillTree; }
	// Anything on the Menu stack (the pick, the tree): the dock's hold-to-buy waits.
	bool IsMenuUp() const;
	// The Modal stack the job picker opens on (the player controller's root layout). Without one, a Reawaken travels
	// without asking (as without a picker in the scene).
	void SetModalStack(UCommonActivatableWidgetStack* Stack);
	// A Reawaken, in Godot's order (hud.gd _run_reawaken_ceremony): the payout read, the reset, the ceremony (the HUD's top
	// layer; it runs on the HUD's clock while the field runs), then the spend step on the Menu stack; leaving it asks for
	// the run's job (the Modal stack, when JobPicker.should_ask()), then the run goes to its starting biome.
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
	FText GetBossText() const;
	FText GetBossTimerText() const;
	FText GetXpText(bool bNext) const;
	float GetZoneFill() const;
	float GetXpFill() const;
	bool IsZoneVisible() const;
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

private:
	void BuildOverlay(class UOverlay* Root);
	void BuildDock(class UOverlay* Root);
	void BuildUpgradesPage(UVerticalBox* Page);
	void BuildRealmsPage(UVerticalBox* Page);
	void BuildReawakenPage(UVerticalBox* Page);
	void BuildCatchesPage(UVerticalBox* Page);
	void BuildAchievementsPage(UVerticalBox* Page);
	void BuildJobsPage(UVerticalBox* Page);
	class UScrollBox* PageScroll(UVerticalBox* Page);
	void RefreshRealms();
	void RefreshReawaken();
	void RefreshCatches();
	void RefreshAchievements();
	void RefreshJobs();
	void DoReawaken();
	void OpenSpendStepWhenFree();
	void ReturnToStartingBiome();
	void TravelToStartingBiome();
	UTextBlock* PageText(UVerticalBox* Page, const FLinearColor& Colour, float WrapAt);
	void ApplyDockSide();
	void WriteNavigation();
	void RefreshShop();
	void RefreshStatus();
	void RefreshTabs();
	void RefreshField();
	void ReadFieldEvents();
	void TickRepeat(float DeltaSeconds);
	void RememberFocus();
	UPawRealmSubsystem* Realm() const;
	UPawGameSubsystem* Game() const;
	void OpenCardPickIfDealt();

	// --- the dock ---
	UPROPERTY()
	TObjectPtr<USizeBox> DockBox;
	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> Tabs;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> Rows;
	UPROPERTY()
	TObjectPtr<UPawButton> MultiplierButton;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> PageTitles;
	// Realms
	UPROPERTY()
	TObjectPtr<UTextBlock> StageText;
	UPROPERTY()
	TObjectPtr<UPawButton> BaitButton;
	UPROPERTY()
	TObjectPtr<UPawButton> AutoTravelButton;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> RealmRows;
	// Reawaken
	UPROPERTY()
	TObjectPtr<UTextBlock> ReawakenProgress;
	UPROPERTY()
	TObjectPtr<UProgressBar> ReawakenBar;
	UPROPERTY()
	TObjectPtr<UTextBlock> PayoutText;
	UPROPERTY()
	TObjectPtr<UTextBlock> TotalText;
	UPROPERTY()
	TObjectPtr<UTextBlock> MultiplierText;
	UPROPERTY()
	TObjectPtr<UTextBlock> UnicornText;
	UPROPERTY()
	TObjectPtr<UPawButton> ReawakenButton;
	UPROPERTY()
	TObjectPtr<UPawButton> CancelButton;
	UPROPERTY()
	TObjectPtr<UPawButton> AskButton;
	// Catches
	UPROPERTY()
	TObjectPtr<UTextBlock> CatchesSummary;
	UPROPERTY()
	TObjectPtr<UPawButton> AutoPickButton;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> CardRows;
	// Achievements
	UPROPERTY()
	TObjectPtr<UTextBlock> AchievementSummary;
	UPROPERTY()
	TObjectPtr<UPawButton> ClaimAllButton;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> AchievementNames;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> AchievementDetails;
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> AchievementMarks;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> ClaimButtons;
	TArray<FLinearColor> AchievementMarkColours;
	// Jobs
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> JobNames;
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> JobDetails;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> JobButtons;

	// --- over the field ---
	UPROPERTY()
	TObjectPtr<UVerticalBox> StatusRow;
	UPROPERTY()
	TObjectPtr<UHorizontalBox> CurrencyBar;
	UPROPERTY()
	TObjectPtr<UTextBlock> CatnipText;
	UPROPERTY()
	TObjectPtr<UTextBlock> StardustText;
	UPROPERTY()
	TObjectPtr<UVerticalBox> WaveBox;
	UPROPERTY()
	TObjectPtr<UTextBlock> WaveText;
	UPROPERTY()
	TObjectPtr<UProgressBar> WaveBar;
	UPROPERTY()
	TObjectPtr<UBorder> ZonePlate;
	UPROPERTY()
	TObjectPtr<UTextBlock> ZoneText;
	UPROPERTY()
	TObjectPtr<UProgressBar> ZoneBar;
	UPROPERTY()
	TObjectPtr<UHorizontalBox> XpRow;
	UPROPERTY()
	TObjectPtr<UTextBlock> XpNow;
	UPROPERTY()
	TObjectPtr<UTextBlock> XpNext;
	UPROPERTY()
	TObjectPtr<UProgressBar> XpBar;
	UPROPERTY()
	TObjectPtr<UPawPounceClock> PounceClock;
	UPROPERTY()
	TObjectPtr<UBorder> BossPanel;
	UPROPERTY()
	TObjectPtr<UTextBlock> BossText;
	UPROPERTY()
	TObjectPtr<UProgressBar> BossBar;
	UPROPERTY()
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
	TObjectPtr<UOverlaySlot> BossSlot;

	TWeakObjectPtr<UCommonActivatableWidgetStack> MenuStack;
	TWeakObjectPtr<UCommonActivatableWidgetStack> ModalStack;
	UPROPERTY()
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
	bool bZoneRainbow = false;         // the zone plate shouts (bait, or a biome to go to): its frame walks the hue

	// Hold-to-buy on a shop row (dock_navigator.gd PRESS_REPEAT_*).
	int32 HeldRow = INDEX_NONE;
	double HeldFor = 0.0;
	double NextRepeatAt = 0.0;
};
