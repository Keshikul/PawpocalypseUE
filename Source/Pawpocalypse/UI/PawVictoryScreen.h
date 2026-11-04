#pragma once

// The ending (docs/05 §6.3; Godot victory_screen.gd): "THE VOID IS QUIET", a line of flavour, the run's time, and OK. Shown
// by the HUD when the field's BossDefeated event carries FieldFlag::Victory (PcProgress RaisesVictory: boss_defeated for
// the last boss realm, a first clear or the first rematch of a cycle), with the FIRST clear's time (FinalBossSeconds).
//
// A Modal-stack screen above everything (Godot: CanvasLayer 128, over the card pick's 100) that holds the field (the
// realm's pause hold "victory"; Godot pauses the tree and restores what it found on dismiss, which pause holders do by
// construction). Its full-screen dim eats every click. OK is focused on show (MenuFocus.enter), with no guard and no Back:
// OK is the only way out.
//
// THE LOOK IS GODOT'S DEFAULT THEME: the screen is its own CanvasLayer under the HUD's, outside the themed Root, so its
// labels are white at the engine's 16 px and its button is the engine's grey plate (measured from the real screen:
// Tests/Data/victory_look_godot.txt; tokens GODOT_*). The engine's font (Open Sans SemiBold) waits for the font phase.
//
// Not yet: the Boba cameo's "victory" menu visit and its nyan fly-past 1.5 s in (BobaCameo, phase 6).

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"

#include "PawVictoryScreen.generated.h"

class UBorder;
class UPawButton;
class UPawRealmSubsystem;
class UTextBlock;

UCLASS()
class PAWPOCALYPSE_API UPawVictoryScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static const FName PauseHolder; // "victory"

	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// show_victory(seconds): the time line ("Cleared in {time}").
	void SetSeconds(double Seconds);
	// dismiss(): OK's press. Closes the screen, then OnDismissed.
	void Dismiss();
	TFunction<void()> OnDismissed;

	// What the tests read (and only read).
	FText GetHeadlineText() const;
	FText GetFlavourText() const;
	FText GetTimeText() const;
	UPawButton* GetOkButton() const { return Ok; }
	UWidget* GetDim() const;
	UBorder* GetPanel() const { return Panel; }
	UTextBlock* GetHeadline() const { return Headline; }
	const TArray<UWidget*>& GetWired() const { return Wired; }

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

private:
	UPawRealmSubsystem* Realm() const;

	UPROPERTY()
	TObjectPtr<UBorder> Dim;
	UPROPERTY()
	TObjectPtr<UBorder> Panel;
	UPROPERTY()
	TObjectPtr<UTextBlock> Headline;
	UPROPERTY()
	TObjectPtr<UTextBlock> Flavour;
	UPROPERTY()
	TObjectPtr<UTextBlock> Time;
	UPROPERTY()
	TObjectPtr<UPawButton> Ok;
	TArray<UWidget*> Wired;
};
