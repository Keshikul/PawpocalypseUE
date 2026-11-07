#pragma once

// The rainbow pieces (phase 6 slice 5d), Godot's two _draw() nodes painted in Slate from PawCore's PcRainbow (exact against
// the real scripts, fixture `rainbow`):
//   UPawRainbowBar    rainbow_bar.gd: the Reawaken page's bar. The theme's track (a ProgressBar whose fill draws nothing:
//                     Godot's StyleBoxEmpty override), then 48 bands of a scrolling rainbow over the filled width, then the
//                     sparks thrown off the leading edge (they rise past the bar: nothing clips them, as in Godot).
//   UPawRainbowFrame  rainbow_frame.gd: a turning rainbow outline round ANOTHER widget (hud.gd's zone plate while it shouts:
//                     Bait, or a biome to go to). Godot makes it the plate's sibling that follows the plate's rect from the
//                     outside (a container would lay it out as a row); so does the port: the frame sits in the HUD's root
//                     overlay and paints round the followed widget's last painted geometry, grown by Margin along its own
//                     axes, so a leaning (jittering) plate's frame leans with it.
// Both are pieces with their own Widget Blueprints (PawWidgets.h); both never take input.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PcRainbow.h"

#include "PawRainbow.generated.h"

class UProgressBar;

UCLASS()
class PAWPOCALYPSE_API UPawRainbowBar : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	// reawaken_bar.value = x: the Range keeps it snapped to 0.01 and in 0..1.
	void SetValue(double Value);
	double GetValue() const { return Value; }
	double GetFraction() const { return Pc::RainbowBarFraction(Value, 0.0, 1.0); }
	const Pc::PcRainbowBar& GetState() const { return State; }
	// The shapes the last paint drew (tests).
	int32 GetPaintedBands() const { return PaintedBands; }
	int32 GetPaintedSparks() const { return PaintedSparks; }
	// One step by hand (tests): what the tick does with the widget's own size.
	void StepFor(double Dt, const FVector2D& Size) { State.Step(Dt, Size.X, Size.Y, GetFraction()); }

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

	// The theme's track (hud.tscn ReawakenBar keeps the theme's background).
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Track;

private:
	void BuildLayout();

	Pc::PcRainbowBar State;
	double Value = 0.0;
	mutable int32 PaintedBands = 0;
	mutable int32 PaintedSparks = 0;
};

UCLASS()
class PAWPOCALYPSE_API UPawRainbowFrame : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	// follow: the widget to hug from the outside; active: the frame shows (and turns) only while set.
	void SetFollow(UWidget* Widget) { Follow = Widget; }
	UWidget* GetFollow() const { return Follow.Get(); }
	void SetActive(bool bOn) { bActive = bOn; }
	bool IsActive() const { return bActive; }
	// Godot's visible: active and the followed widget shown.
	bool IsShown() const;
	double GetPhase() const { return Phase; }
	// The points the last paint traced (tests): 37 for a frame drawn, 0 for none.
	int32 GetPaintedPoints() const { return PaintedPoints; }
	// One step by hand (tests): what the tick does.
	void StepFor(double Dt) { Phase = Pc::RainbowFramePhase(Phase, Dt, TurnsPerSec, IsShown()); }

	// Its settings (hud.gd sets them per frame; the zone plate's: margin 2, corner 8, width 4).
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	double Margin = Pc::RainbowFrame::Margin;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	double Corner = Pc::RainbowFrame::Corner;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	double LineWidth = Pc::RainbowFrame::Width;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	double TurnsPerSec = Pc::RainbowFrame::TurnsPerSec;

protected:
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;

private:
	TWeakObjectPtr<UWidget> Follow;
	bool bActive = false;
	double Phase = 0.0;
	mutable int32 PaintedPoints = 0;
};
