#pragma once

// The Pounce cooldown as a clock (pounce_gauge.gd; designer, 2026-10-01): a paper disc in the field's bottom corner away
// from the dock. While Pounce recharges an ink wedge covers what is left of the cooldown and shrinks clockwise from twelve
// o'clock, with a hand at its edge; when ready the disc wears a breathing gold ring. Painted, never takes input. The paw
// icon on the disc arrives with phase 6's art.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "PawPounceClock.generated.h"

UCLASS()
class PAWPOCALYPSE_API UPawPounceClock : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	// 0 = just fired, 1 = ready.
	void SetFraction(double Value);
	double GetFraction() const { return Fraction; }
	bool IsReady() const { return Fraction >= 1.0; }

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	double Fraction = 1.0;
	double Clock = 0.0;
};
