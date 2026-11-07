#pragma once

// The Reawaken ceremony (Godot reawaken_ceremony.gd + scenes/ui/reawaken_ceremony.tscn): the screen goes white, the run's
// payout is stated plainly, then the player is handed to the skill tree's spend step. Reawakening is the one moment that
// throws everything away for a permanent multiplier; a silent button press would make it feel like a mis-click.
//
// Godot plays it on a CanvasLayer (10) that takes no input and does NOT pause the game: the field runs on under the flash.
// Here it is the HUD's top layer (over the dock and the field's readouts, under the Menu stack, as Godot's card pick at
// layer 100 is over it), hit-test invisible. Its clock is the HUD's: Step() is driven from UPawHudScreen::Pump, and only
// while the realm runs, because Godot's tween is bound to a pausable node and stops while anything pauses the game (a card
// pick dealt under the flash freezes it until the card is taken).
//
// The sequence (one sine in-out tween, durations from the Godot constants): the flash and the headline fade in over
// FLASH_IN_SEC, then the payout over 0.25 s, a HOLD_SEC hold, then all three fade out over FADE_OUT_SEC; `finished`.

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "PawReawakenCeremony.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class PAWPOCALYPSE_API UPawReawakenCeremony : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	// play(stardust, reawaken_count): the payout is what the Reawaken PAID, read before it ran (after it the grant is folded
	// into the totals and is no longer separable); the count is the one after it.
	void Play(double Stardust, int64 ReawakenCount);
	// One frame of the tween. True on the frame the sequence ends (Godot's `finished`); the ceremony is hidden again then.
	bool Step(double DeltaSeconds);
	bool IsPlaying() const { return bPlaying; }
	double GetElapsed() const { return Elapsed; }

	// The three alphas at T seconds into the sequence (the tween, as a pure function the tests pin).
	struct FAlphas
	{
		float Flash = 0.0f;
		float Headline = 0.0f;
		float Payout = 0.0f;
	};
	static FAlphas AlphasAt(double T);
	static double Duration();

	// What the tests read (and only read): what is drawn.
	FAlphas GetDrawnAlphas() const;
	FText GetHeadlineText() const;
	FText GetPayoutText() const;

private:
	void Apply(const FAlphas& A);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Flash;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Headline;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Payout;
	double Elapsed = 0.0;
	bool bPlaying = false;
};
