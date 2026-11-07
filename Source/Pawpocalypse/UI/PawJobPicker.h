#pragma once

// The job picker (docs/05 §6; Godot job_picker.gd): "Which job are you taking into this run?", asked once, answered once.
// THE LOCK IS THE MECHANIC: a job swapped mid-run costs nothing to choose. A Modal-stack screen that holds the field (the
// realm's pause hold "job_picker"; Godot pauses the tree), opened by the HUD at the end of a Reawaken's spend step when
// JobPicker.should_ask(); the travel to the starting biome waits for the answer (OnChosen).
//
// What it offers is PawCore's decision (PcHud JobPicker, exact against the real job_picker.gd): the held jobs by id, the
// previous choice marked "(last run)" and selected, the baseline ("No job") always last. Under Union Rules a first answer
// that leaves another job held asks again, in place, for the second slot ("At n% strength." on every blurb; the primary is
// not offered twice; "No second job"). There is no close and no Back: the baseline is the way to decline.
//
// The guard (MenuFocus.guard_ok, as the card pick: UPawCardPickScreen::GuardAllows): a press counts once ModalGuardSec has
// passed since the list opened (the second ask restarts it), and for the keys and the pad only a press that BEGAN after it
// opened. The guard sits on the buttons (ClickRow), not on Pick(), the programmatic way in.
//
// Pad and keys (controller plan 7): one column of rows, wired by PawUi::WireRows (wrapping); opens on last run's job.
//
// THE LOOK IS GODOT'S DEFAULT THEME (F21, measured: Tests/Data/victory_look_godot.txt jp.*): its own CanvasLayer under the
// HUD, outside the themed Root, so a translucent dark panel, stock grey buttons, a white note; the title keeps its INK (dark
// on dark, as Godot draws it), the blurbs their text_muted.

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"

#include "PawJobPicker.generated.h"

class UBorder;
class UPawButton;
class UPawRealmSubsystem;
class UTextBlock;
class UVerticalBox;

UCLASS()
class PAWPOCALYPSE_API UPawJobPicker : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static const FName PauseHolder; // "job_picker"

	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// Runs once the run's answer is complete (after the second, under Union Rules), with the screen already closed: the
	// HUD's travel to the starting biome.
	TFunction<void()> OnChosen;

	// open(): re-reads what is offered and redraws; the guard's clock restarts. bRefocus: the default row takes focus (the
	// second ask, while up; on activation Common UI focuses the desired target).
	void ShowRows(bool bSecond, bool bRefocus);

	// The programmatic way in (_pick): no guard. Returns false for a row that does not exist.
	bool Pick(int32 Row);
	// The button's way in: through the guard.
	void ClickRow(int32 Row);
	bool IsGuardOpen() const;
	void AdvanceClock(double DeltaSeconds) { Clock += DeltaSeconds; }
	void NoteKeyDown(bool bRepeat);
	void NoteMouseDown();

	// What the tests read (and only read).
	bool IsAskingSecond() const { return bSecond; }
	int32 GetRowCount() const { return RowIds.Num(); }
	FString GetRowJobId(int32 Row) const { return RowIds.IsValidIndex(Row) ? RowIds[Row] : FString(); }
	UPawButton* GetRowButton(int32 Row) const;
	FText GetRowLabel(int32 Row) const;
	FText GetRowBlurb(int32 Row) const;
	// Rows drawn (pooled ones beyond the list are collapsed).
	int32 GetShownRowCount() const;
	FText GetTitleText() const;
	FText GetNoteText() const;
	UWidget* GetDim() const;
	UBorder* GetPanel() const { return Panel; }
	UTextBlock* GetNote() const { return Note; }
	const TArray<UWidget*>& GetWired() const { return Wired; }

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

private:
	void EnsureRow(int32 Index);
	void OnLayoutReady();
	bool CanTakeFocus() const;
	UPawRealmSubsystem* Realm() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Dim;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Panel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Title;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Note;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> List;
	UPROPERTY()
	TArray<TObjectPtr<UPawButton>> Buttons; // pooled with their blurbs: an open reuses them and hides the rest
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> Blurbs;

	TArray<FString> RowIds;
	TArray<UWidget*> Wired;
	int32 FocusRow = 0;
	bool bSecond = false;
	double Clock = 0.0;
	double OpenedAt = 0.0;
	double PressBeganAt = -1.0e9;
	bool bLastWasPointer = false;
};
