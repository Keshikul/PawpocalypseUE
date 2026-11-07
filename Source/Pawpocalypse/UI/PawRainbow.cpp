#include "UI/PawRainbow.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Render/PawFxCanvas.h"
#include "UI/PawJuice.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

namespace
{
	FLinearColor Godot(const Pc::PcColor& C)
	{
		// FPawFxCanvas takes Godot colours as they are (sRGB floats, straight alpha).
		return FLinearColor(static_cast<float>(C.R), static_cast<float>(C.G), static_cast<float>(C.B), static_cast<float>(C.A));
	}

	// The antialiased line's soft edge, px: by eye against Godot's draw_polyline_colors(..., antialiased = true).
	constexpr double FrameFeatherPx = 1.0;
}

// --- the bar ---------------------------------------------------------------------------------------------------------

bool UPawRainbowBar::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		BuildLayout();
	}
	const bool bOk = Super::Initialize();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (bOk && PawUi::ShouldWire(this) && Track)
	{
		Track->SetPercent(0.0f); // the rainbow IS the fill
	}
	return bOk;
}

void UPawRainbowBar::BuildLayout()
{
	WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	// The theme's track; its fill draws nothing (rainbow_bar.gd _ready: add_theme_stylebox_override("fill", StyleBoxEmpty)).
	Track = PawUi::MakeThemeBar(WidgetTree, TEXT("Track"));
	FProgressBarStyle S = Track->GetWidgetStyle();
	S.FillImage = PawUi::NoBrush();
	Track->SetWidgetStyle(S);
	WidgetTree->RootWidget = Track;
}

void UPawRainbowBar::SetValue(double InValue)
{
	Value = Pc::GodotRangeValue(InValue, 0.0, 1.0, Pc::GodotRangeStep);
}

void UPawRainbowBar::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	StepFor(DeltaSeconds, FVector2D(Geometry.GetLocalSize()));
}

int32 UPawRainbowBar::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	const FVector2D Size(Geometry.GetLocalSize());
	std::vector<Pc::PcRainbowBand> Bands;
	std::vector<Pc::PcRainbowDot> Dots;
	State.Draw(Size.X, Size.Y, GetFraction(), Bands, Dots);
	FPawFxCanvas Canvas;
	for (const Pc::PcRainbowBand& B : Bands)
	{
		const FVector2D Corners[4] = { { B.X, 0.0 }, { B.X + B.W, 0.0 }, { B.X + B.W, Size.Y }, { B.X, Size.Y } };
		Canvas.Polygon(Corners, Godot(B.Color));
	}
	for (const Pc::PcRainbowDot& D : Dots)
	{
		Canvas.Circle(FVector2D(D.X, D.Y), D.Radius, Godot(D.Color));
	}
	PaintedBands = static_cast<int32>(Bands.size());
	PaintedSparks = static_cast<int32>(Dots.size());
	PawUi::PaintCanvas(Canvas, Out, Layer, Geometry.GetAccumulatedRenderTransform());
	return Layer;
}

// --- the frame -------------------------------------------------------------------------------------------------------

bool UPawRainbowFrame::Initialize()
{
	// Nothing of its own to lay out: it paints round another widget.
	const bool bOk = Super::Initialize();
	SetVisibility(ESlateVisibility::HitTestInvisible); // over the playfield: a frame that ate a click would break Pounce
	return bOk;
}

bool UPawRainbowFrame::IsShown() const
{
	return bActive && Follow.IsValid() && FPawJuice::IsShownInTree(Follow.Get());
}

void UPawRainbowFrame::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	StepFor(DeltaSeconds);
}

int32 UPawRainbowFrame::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	PaintedPoints = 0;
	const UWidget* Plate = Follow.Get();
	if (!IsShown() || !Plate)
	{
		return Layer;
	}
	// The plate as it was last painted: its size, and the transform that put it on screen (its lean included).
	const FGeometry& At = Plate->GetPaintSpaceGeometry();
	const FVector2D PlateSize(At.GetLocalSize());
	if (PlateSize.X <= 0.0 || PlateSize.Y <= 0.0)
	{
		return Layer;
	}
	const Pc::PcRect R = Pc::RainbowFrameAround(PlateSize.X, PlateSize.Y, Margin);
	std::vector<double> Xs, Ys;
	std::vector<Pc::PcColor> Colors;
	Pc::RainbowFrameOutline(R.W, R.H, Corner, Phase, Xs, Ys, Colors);
	TArray<FVector2D> Points;
	TArray<FLinearColor> PointColors;
	for (size_t I = 0; I < Xs.size(); ++I)
	{
		Points.Add(FVector2D(R.X + Xs[I], R.Y + Ys[I]));
		PointColors.Add(Godot(Colors[I]));
	}
	FPawFxCanvas Canvas;
	Canvas.Polyline(Points, PointColors, LineWidth, FrameFeatherPx);
	PaintedPoints = Points.Num();
	PawUi::PaintCanvas(Canvas, Out, Layer, At.GetAccumulatedRenderTransform());
	return Layer;
}
