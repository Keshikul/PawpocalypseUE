#pragma once

// The skill tree's sky (skill_tree_sky.gd, phase 6 slice 5d; designer 2026-09-14: "purple dark with cloud like things
// floating around and maybe some stars but not too bright. It should not overpower the skill icons. Just tiny movements and
// change of color."): PawCore's PcSky (exact against the real node, fixture `rainbow`) painted in Slate over the tree
// screen's own opaque Sky border (which still takes the drag and the wheel). Laid out when its size changes, at the effects
// budget of that moment, as Godot does on a viewport resize; stepped only while it is up (UMG ticks a shown widget; Godot
// checks _tree_open()). Painted, never takes input. A piece with its own Widget Blueprint (PawWidgets.h).

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PcBackdrop.h"

#include "PawTreeSky.generated.h"

UCLASS()
class PAWPOCALYPSE_API UPawTreeSky : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	const Pc::PcSky& GetSky() const { return Sky; }
	// Lays the sky out for a size now and steps it (the tick does both from the widget's own geometry; tests by hand).
	void LayOutFor(const FVector2D& Size);
	void StepFor(double Dt) { Sky.Step(Dt); }
	// What the last paint drew (tests).
	int32 GetPaintedStars() const { return PaintedStars; }
	int32 GetPaintedClouds() const { return PaintedClouds; }

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	double Budget() const;

	Pc::PcSky Sky;
	FVector2D LaidOutSize = FVector2D::ZeroVector;
	mutable int32 PaintedStars = 0;
	mutable int32 PaintedClouds = 0;
};
