#include "UI/PawTreeSky.h"

#include "Engine/GameInstance.h"
#include "Render/PawFxCanvas.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawUiKit.h"

namespace
{
	FLinearColor Godot(const Pc::PcSkyColour& C)
	{
		return FLinearColor(C.R, C.G, C.B, C.A); // FPawFxCanvas takes Godot colours as they are (sRGB, straight alpha)
	}
}

bool UPawTreeSky::Initialize()
{
	const bool bOk = Super::Initialize();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return bOk;
}

double UPawTreeSky::Budget() const
{
	const UGameInstance* GI = GetGameInstance();
	const UPawSettingsSubsystem* S = GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	return S ? S->EffectBudget() : 1.0;
}

void UPawTreeSky::LayOutFor(const FVector2D& Size)
{
	if (Size == LaidOutSize)
	{
		return;
	}
	LaidOutSize = Size;
	// _layout() rolls the clouds and stars again; the clock (_time) runs on.
	const double Time = Sky.Time;
	Sky = Pc::BuildSky(Size.X, Size.Y, Budget());
	Sky.Time = Time;
}

void UPawTreeSky::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	const FVector2D Size(Geometry.GetLocalSize());
	if (Size.X > 0.0 && Size.Y > 0.0)
	{
		LayOutFor(Size);
		StepFor(DeltaSeconds);
	}
}

int32 UPawTreeSky::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	PaintedStars = 0;
	PaintedClouds = 0;
	if (Sky.Width <= 0.0f)
	{
		return Layer;
	}
	// _draw(): the base over the whole sky; the stars under the clouds (a cloud passing over a star dims it); the clouds.
	FPawFxCanvas Canvas;
	const FVector2D Base[4] = { { 0.0, 0.0 }, { Sky.Width, 0.0 }, { Sky.Width, Sky.Height }, { 0.0, Sky.Height } };
	Canvas.Polygon(Base, Godot(Pc::PcSky::BaseColour()));
	for (const Pc::PcSkyStar& S : Sky.Stars)
	{
		Canvas.Circle(FVector2D(S.AtX, S.AtY), S.Radius, Godot(Sky.StarColour(S)));
		++PaintedStars;
	}
	std::vector<double> Xs, Ys;
	TArray<FVector2D> Outline;
	for (const Pc::PcSkyCloud& C : Sky.Clouds)
	{
		Sky.CloudOutline(C, Xs, Ys);
		Outline.Reset();
		for (size_t I = 0; I < Xs.size(); ++I)
		{
			Outline.Add(FVector2D(Xs[I], Ys[I]));
		}
		Canvas.Fan(FVector2D(C.AtX, C.AtY), Outline, Godot(Sky.CloudColour(C)));
		++PaintedClouds;
	}
	PawUi::PaintCanvas(Canvas, Out, Layer, Geometry.GetAccumulatedRenderTransform());
	return Layer;
}
