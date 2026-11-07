#pragma once

// The garden behind the title menu (main_menu.gd _build_garden; designer, 2026-09-16: "green like grass with puddles and
// shapes and trees and bushes slightly dancing"): the game's own backdrop, biome "menu" (the Backyard's floor and palette,
// its grass and rocks, the Park's flowers, bushes round the edge, five trees in the outer quarters, puddles of the Rooftops'
// sky), every plant swaying. PcBackdrop lays it out exactly as Godot's BiomeBackdrop; this piece paints it in Slate, over
// the title's paper floor and under everything else, as Godot's Control child does. Painted, never takes input.
//
// When Godot lays out what (kept): the scatter and the trees on a resize only (_rebuild), with the effects budget of that
// moment; the patches, puddles and border rows every frame (the garden dances, so _draw runs each frame), with the budget
// of now.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PcBackdrop.h"
#include "Styling/SlateBrush.h"

#include "PawGarden.generated.h"

class UPawSpriteAtlas;
class UTexture2D;

UCLASS()
class PAWPOCALYPSE_API UPawGarden : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	// What the last paint laid out (tests).
	const Pc::PcBackdropLayout& GetScatter() const { return Scatter; }
	const Pc::PcBackdropLayout& GetFrame() const { return Frame; }
	double GetClock() const { return Clock; }
	bool HasArt() const { return Atlas != nullptr && FloorTexture != nullptr; }
	// Lays the garden out for a size now (the tick does it from the widget's own geometry).
	void LayOutFor(const FVector2D& Size) { Relayout(Size); }

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	void Relayout(const FVector2D& Size);
	double Budget() const;

	UPROPERTY(Transient)
	TObjectPtr<const UPawSpriteAtlas> Atlas;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FloorTexture;

	FSlateBrush FloorBrush;
	TArray<FSlateBrush> PageBrushes;
	Pc::PcBackdropLayout Scatter;   // the scatter and the trees (laid out on a resize)
	Pc::PcBackdropLayout Frame;     // the patches, puddles, border rows and corners (the budget of now)
	TArray<int32> ScatterCells, FrameCells;
	FVector2D LaidOutSize = FVector2D::ZeroVector;
	double FrameBudget = -1.0;
	double Clock = 0.0;
};
