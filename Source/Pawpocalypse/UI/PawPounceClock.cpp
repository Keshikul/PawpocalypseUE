#include "UI/PawPounceClock.h"

#include "UI/PawIcons.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawWidgets.h"
#include "UI/PawUiKit.h"

namespace
{
	// A filled circle as a rounded box whose radius is half its side.
	void Disc(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2D& Centre, double R, const FLinearColor& Fill,
		const FLinearColor& Frame = FLinearColor::Transparent, float FrameWidth = 0.0f)
	{
		const FSlateBrush Brush = PawUi::Rounded(Fill, Frame, FrameWidth, static_cast<float>(R));
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(2.0 * R, 2.0 * R), FSlateLayoutTransform(FVector2f(Centre - FVector2D(R, R)))),
			&Brush, ESlateDrawEffect::None, Fill);
	}
}

bool UPawPounceClock::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		// A fixed-size box gives the widget its desired size; the clock is all paint.
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size"));
		Box->SetWidthOverride(PawUi::PounceDiameter);
		Box->SetHeightOverride(PawUi::PounceDiameter);
		WidgetTree->RootWidget = Box;
	}
	const bool bOk = Super::Initialize();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	return bOk;
}

void UPawPounceClock::SetFraction(double Value)
{
	Fraction = FMath::Clamp(Value, 0.0, 1.0);
}

void UPawPounceClock::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (IsReady())
	{
		Clock = FMath::Fmod(Clock + DeltaSeconds, 1.0 / PawUi::PounceReadyHz);
	}
}

int32 UPawPounceClock::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	const int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	const FVector2D Size(Geometry.GetLocalSize());
	const FVector2D C = Size * 0.5;
	const double R = FMath::Min(Size.X, Size.Y) * 0.5 - PawUi::PounceReadyRing;

	if (IsReady())
	{
		// The breathing gold ring outside the ink one.
		const double Pulse = 0.5 + 0.5 * FMath::Sin(Clock * 2.0 * PI * PawUi::PounceReadyHz);
		FLinearColor Gold = PawTokens::GOLD;
		Gold.A = static_cast<float>(0.55 + 0.45 * Pulse);
		const double Width = PawUi::PounceReadyRing * (0.7 + 0.3 * Pulse);
		Disc(Out, Layer, Geometry, C, R + PawUi::PounceRing + Width, Gold);
	}
	Disc(Out, Layer + 1, Geometry, C, R + PawUi::PounceRing, PawTokens::INK);
	Disc(Out, Layer + 2, Geometry, C, R, PawTokens::UI_PANEL);
	// The paw on the disc (pounce_gauge.gd: skill_pounce.svg, r * 1.3 square, centred).
	static const FSlateBrush Paw = PawUi::IconBrush(TEXT("skill_pounce"), 64.0);
	if (Paw.GetResourceObject())
	{
		const double S = R * 1.3;
		FSlateDrawElement::MakeBox(Out, Layer + 3, Geometry.ToPaintGeometry(FVector2f(S, S), FSlateLayoutTransform(FVector2f(C - FVector2D(S, S) * 0.5))), &Paw);
	}

	if (Fraction < 1.0)
	{
		// What is LEFT of the cooldown, from the hand round to twelve o'clock: a fan of triangles in translucent ink.
		const double Start = -PI / 2.0 + 2.0 * PI * Fraction;
		const double End = -PI / 2.0 + 2.0 * PI;
		const int32 Steps = FMath::Max(static_cast<int32>(48.0 * (1.0 - Fraction)), 2);
		FLinearColor Ink = PawTokens::INK;
		Ink.A = static_cast<float>(PawUi::PounceWedgeAlpha);
		const FColor VertexColour = Ink.ToFColorSRGB();
		const FSlateRenderTransform& T = Geometry.GetAccumulatedRenderTransform();
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;
		Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, FVector2f(C), FVector2f(0.5f, 0.5f), VertexColour));
		for (int32 I = 0; I <= Steps; ++I)
		{
			const double A = FMath::Lerp(Start, End, static_cast<double>(I) / Steps);
			const FVector2D P = C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R;
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, FVector2f(P), FVector2f(0.5f, 0.5f), VertexColour));
			if (I > 0)
			{
				Indices.Add(0);
				Indices.Add(static_cast<SlateIndex>(I));
				Indices.Add(static_cast<SlateIndex>(I + 1));
			}
		}
		const FSlateResourceHandle White = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
		FSlateDrawElement::MakeCustomVerts(Out, Layer + 4, White, Verts, Indices, nullptr, 0, 0);
		// The hand, so the wedge's edge reads as a clock and not a bite.
		TArray<FVector2f> Hand = { FVector2f(C), FVector2f(C + FVector2D(FMath::Cos(Start), FMath::Sin(Start)) * R) };
		FSlateDrawElement::MakeLines(Out, Layer + 5, Geometry.ToPaintGeometry(), Hand, ESlateDrawEffect::None, PawTokens::INK, true, 2.0f);
	}
	return Layer + 5;
}
