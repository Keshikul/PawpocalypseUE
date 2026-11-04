#pragma once

// The card pick (docs/05 §6.3; Godot catches_pick_ui.gd): three cards, keep one (more with Dilated). A Menu-stack screen
// over the dimmed field; the field holds still while it is up (the realm's pause hold "card_pick"). What it shows is
// PawCore's decision (PcHud CardPick); this class turns it into string-table text and cards.
//
// THERE IS NO CLOSE AND NO BACK, and that is the point (Godot's header): the only exits are taking a card, or a banish that
// empties the deck. Reroll deals a new hand in place.
//
// Each card: the flavour on a coloured header band, the name, NEW or "LEVEL n of max", the description at the level taking
// it gives, a red frame and a warning when it costs something; Take, and Banish under it while a banish is held. A click
// anywhere on a card takes it (designer 2026-08-29); the card itself is mouse-only, Take is what the pad lands on.
//
// THE GUARD (controller plan 6.2; MenuFocus.guard_ok): a press counts only once ModalGuardSec has passed since the hand was
// dealt (a reroll's deal included), and for the keys and the pad only a press that BEGAN after the deal, so the Space held
// to buy, or the A that pressed Reroll, never takes a card. The guard sits on the buttons (Click*), not on Take() and
// friends: those are the programmatic way in (the tests, later the soak and the playthrough).
//
// Pad and keys (controller plan 7): rows Take (one per card) / Banish / Reroll, wired by PawUi::WireRows (wrapping);
// opens on the first card's Take.

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"

#include "PawCardPickScreen.generated.h"

class UBorder;
class UHorizontalBox;
class UPawButton;
class UPawRealmSubsystem;
class USizeBox;
class UTextBlock;
class UVerticalBox;

// A card face draws nothing of its own as a button: the plate inside it is the card.
UCLASS()
class PAWPOCALYPSE_API UPawCardFaceStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPawCardFaceStyle();
};

// One card: a Common button (the whole-card click, never focusable) around the plate, with Take and Banish inside it.
UCLASS()
class PAWPOCALYPSE_API UPawCardFace : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPawCardFace(const FObjectInitializer& ObjectInitializer);
	virtual bool Initialize() override;

	struct FLook
	{
		FText Flavour;
		FLinearColor Band = FLinearColor::White;
		FText Name;
		FText Rank;
		FLinearColor RankColour = FLinearColor::Black;
		FText Body;
		bool bCosts = false;
		bool bBanish = false;
	};
	void SetLook(const FLook& Look);

	// Hovered by the mouse (anywhere on the card, Take and Banish included) or Take / Banish selected: the card lifts.
	bool IsLifted() const;
	// The lift's tween, one frame (NativeTick; the tests drive it).
	void StepLift(double DeltaSeconds);
	float GetLiftScale() const { return LiftScale; }
	// Read from what is drawn (the plate's frame, the band's fill), not from a flag beside it.
	bool HasCostFrame() const;
	FLinearColor GetBandColour() const;

	UPawButton* GetTake() const { return Take; }
	UPawButton* GetBanish() const { return Banish; }
	FText GetFlavourText() const;
	FText GetNameText() const;
	FText GetRankText() const;
	FLinearColor GetRankColour() const;
	FText GetBodyText() const;
	bool IsWarningShown() const;

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UBorder> Plate;
	UPROPERTY()
	TObjectPtr<UBorder> Band;
	UPROPERTY()
	TObjectPtr<UTextBlock> FlavourText;
	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY()
	TObjectPtr<UTextBlock> RankText;
	UPROPERTY()
	TObjectPtr<UTextBlock> BodyText;
	UPROPERTY()
	TObjectPtr<UTextBlock> WarnText;
	UPROPERTY()
	TObjectPtr<UPawButton> Take;
	UPROPERTY()
	TObjectPtr<UPawButton> Banish;
	float LiftScale = 1.0f;
};

UCLASS()
class PAWPOCALYPSE_API UPawCardPickScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static const FName PauseHolder; // "card_pick"

	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// Re-reads the hand (PcHud CardPick) and redraws; the guard's clock restarts (a new deal). No hand: the screen closes.
	// bRefocus: the first Take takes focus (a re-deal while up; on activation Common UI focuses the desired target).
	void ShowHand(bool bRefocus);

	// The programmatic way in: no guard. Take closes the screen (also when the card was refused: a stale hand must not trap
	// the player, as Godot); Banish re-deals or, with the deck banished dry, closes; Reroll re-deals.
	bool Take(int32 Index);
	bool Banish(int32 Index);
	bool Reroll();
	// The buttons' way in: through the guard.
	void ClickTake(int32 Index);
	void ClickBanish(int32 Index);
	void ClickReroll();

	// MenuFocus.guard_ok for this screen, now.
	bool IsGuardOpen() const;
	// The rule itself: ModalGuardSec since the deal, and a pointer press or a key / pad press that began after the deal.
	static bool GuardAllows(double DealtAt, double Now, bool bLastWasPointer, double PressBeganAt);
	// The screen's clock (NativeTick advances it; the tests drive it) and what the preview handlers note.
	void AdvanceClock(double DeltaSeconds) { Clock += DeltaSeconds; }
	void NoteKeyDown(bool bRepeat);
	void NoteMouseDown();

	// What the tests read (and only read).
	int32 GetCardCount() const { return HandIds.Num(); }
	FString GetCardId(int32 Index) const { return HandIds.IsValidIndex(Index) ? HandIds[Index] : FString(); }
	UPawCardFace* GetCard(int32 Index) const { return Index >= 0 && Index < HandIds.Num() && Faces.IsValidIndex(Index) ? Faces[Index].Get() : nullptr; }
	// Cards drawn (pooled ones beyond the hand are collapsed).
	int32 GetShownCardCount() const;
	UPawButton* GetRerollButton() const { return RerollButton; }
	UWidget* GetDim() const;
	FText GetTitleText() const;
	FText GetSubtitleText() const;
	// The selectable widgets as wired, in reading order.
	const TArray<UWidget*>& GetWired() const { return Wired; }

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent) override;

private:
	UPawCardFace* FaceAt(int32 Index);
	void Close();
	bool CanTakeFocus() const;
	void ShowReroll(int64 Rerolls);
	UPawRealmSubsystem* Realm() const;

	UPROPERTY()
	TObjectPtr<UBorder> Dim;
	UPROPERTY()
	TObjectPtr<UTextBlock> Title;
	UPROPERTY()
	TObjectPtr<UTextBlock> Subtitle;
	UPROPERTY()
	TObjectPtr<UHorizontalBox> Row;
	UPROPERTY()
	TObjectPtr<UPawButton> RerollButton;
	UPROPERTY()
	TArray<TObjectPtr<UPawCardFace>> Faces; // pooled: a deal reuses them and hides the rest (screens are pooled too, rule 5)

	TArray<FString> HandIds;
	TArray<UWidget*> Wired;
	double Clock = 0.0;
	double DealtAt = 0.0;
	double PressBeganAt = -1.0e9;
	bool bLastWasPointer = false;
};
