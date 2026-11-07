#pragma once

// The Unicorn ceremony (unicorn_ceremony.gd, phase 6 slice 6a): played once over the skill tree the moment the Unicorn node is
// bought. PawCore's PcUnicornCeremony (exact against the real script, fixture `unicorn`) painted in Slate: the dim, the
// rainbow's six arcs and the confetti and stars (FPawFxCanvas), the mounted cat (player_cat_stage_4 from DA_FieldAtlas,
// mirrored, rearing about the middle of its foot). The last part of the tree's root overlay, collapsed until it plays; while
// it plays it is Visible (Godot's MOUSE_FILTER_STOP: the tree cannot be bought from mid-fiesta), holds the keyboard focus
// (Godot's grab_focus: Common UI's pad A finds no node to click) and the tree hands it every key; any press after 0.6 s skips
// to the fade. On `finished` it collapses and calls OnFinished (the tree takes its focus back: _enter_focus). A piece with its
// own Widget Blueprint (PawWidgets.h).

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PcUnicorn.h"
#include "Styling/SlateBrush.h"

#include "PawUnicornCeremony.generated.h"

class UPawSpriteAtlas;

UCLASS()
class PAWPOCALYPSE_API UPawUnicornCeremony : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	// UnicornCeremony.play(): laid out for the view (the tree's size), the first whinny, the timeline.
	void Play(const FVector2D& ViewSize);
	bool IsPlaying() const { return bPlaying; }
	// _input: true when the input skips to the fade (it is eaten either way while the ceremony plays).
	bool Input(Pc::PcUnicornInput What);
	// A frame: what the tick does (tests step it by hand). Plays its sounds; on `finished`, collapses and calls OnFinished.
	void StepFor(double Dt);
	const Pc::PcUnicornCeremony& GetState() const { return State; }
	bool HasArt() const { return Atlas != nullptr && Cell != INDEX_NONE; }
	// What the last paint drew (tests): the shapes (arcs, confetti, stars and rims) and the cat.
	int32 GetPaintedShapes() const { return PaintedShapes; }
	bool WasCatPainted() const { return bCatPainted; }

	TFunction<void()> OnFinished;

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
	void Sounds();

	UPROPERTY(Transient)
	TObjectPtr<const UPawSpriteAtlas> Atlas;
	FSlateBrush PageBrush;
	int32 Cell = INDEX_NONE;
	Pc::PcUnicornCeremony State;
	bool bPlaying = false;
	uint64 Plays = 0;
	mutable int32 PaintedShapes = 0;
	mutable bool bCatPainted = false;
};
