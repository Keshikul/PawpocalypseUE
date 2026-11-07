#include "UI/PawUnicornCeremony.h"

#include "Audio/PawAudioSubsystem.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Render/PawFxCanvas.h"
#include "Render/PawSpriteAtlas.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "UI/PawTokens.h"
#include "UI/PawUiKit.h"

namespace
{
	const TCHAR* AtlasPath = TEXT("/Game/Pawpocalypse/Art/Field/DA_FieldAtlas.DA_FieldAtlas");
	const TCHAR* CatStem = TEXT("player_cat_stage_4"); // ART: the mounted stage the node just put the cat on
	constexpr double FeatherPx = 1.0;                   // the antialiased arcs and rims (no FXAA over Slate)

	FLinearColor Godot(const Pc::PcColor& C, double Alpha)
	{
		// FPawFxCanvas takes Godot colours as they are (sRGB floats, straight alpha); the layer's fade multiplies in.
		return FLinearColor(static_cast<float>(C.R), static_cast<float>(C.G), static_cast<float>(C.B), static_cast<float>(C.A * Alpha));
	}

	// Godot's sRGB Color from a token (PawTokens hold linear colours).
	Pc::PcColor FromToken(const FLinearColor& Linear)
	{
		const FColor S = Linear.ToFColorSRGB();
		return { S.R / 255.0, S.G / 255.0, S.B / 255.0, Linear.A };
	}
}

bool UPawUnicornCeremony::Initialize()
{
	const bool bOk = Super::Initialize();
	SetVisibility(ESlateVisibility::Collapsed);
	SetIsFocusable(true);
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Atlas = LoadObject<UPawSpriteAtlas>(nullptr, AtlasPath);
		Cell = Atlas ? Atlas->Find(CatStem) : INDEX_NONE;
		if (Atlas && Cell != INDEX_NONE && Atlas->Pages.IsValidIndex(Atlas->Cell(Cell).Page))
		{
			PageBrush.SetResourceObject(Atlas->Pages[Atlas->Cell(Cell).Page]);
		}
	}
	return bOk;
}

void UPawUnicornCeremony::Play(const FVector2D& ViewSize)
{
	State = Pc::PcUnicornCeremony();
	State.Seed = 0x554E4943ull + Plays++;
	State.Start(ViewSize.X, ViewSize.Y);
	bPlaying = true;
	// Takes every click (the tree under it must not be bought from mid-fiesta) and the focus.
	SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();
	Sounds();
}

bool UPawUnicornCeremony::Input(Pc::PcUnicornInput What)
{
	return bPlaying && State.Input(What);
}

void UPawUnicornCeremony::Sounds()
{
	for (const Pc::PcUnicornSound S : State.Sounds)
	{
		UPawAudioSubsystem::PlayFrom(this, S == Pc::PcUnicornSound::Neigh ? TEXT("unicorn_neigh") : TEXT("boss_defeated"));
	}
	State.Sounds.clear();
}

void UPawUnicornCeremony::StepFor(double Dt)
{
	if (!bPlaying)
	{
		return;
	}
	State.Step(Dt);
	Sounds();
	if (State.bFinished)
	{
		// finished.emit(); queue_free(): the tree is handed back.
		bPlaying = false;
		SetVisibility(ESlateVisibility::Collapsed);
		if (OnFinished)
		{
			OnFinished();
		}
	}
}

void UPawUnicornCeremony::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	StepFor(DeltaSeconds);
}

FReply UPawUnicornCeremony::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Input(Pc::PcUnicornInput::MouseDown);
	return FReply::Handled(); // every click, skipping or not
}

FReply UPawUnicornCeremony::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (!bPlaying)
	{
		return Super::NativeOnKeyDown(Geometry, Event);
	}
	Input(Event.IsRepeat() ? Pc::PcUnicornInput::KeyEcho
		: (Event.GetKey().IsGamepadKey() ? Pc::PcUnicornInput::PadDown : Pc::PcUnicornInput::KeyDown));
	return FReply::Handled();
}

int32 UPawUnicornCeremony::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	PaintedShapes = 0;
	bCatPainted = false;
	if (!bPlaying)
	{
		return Layer;
	}
	const Pc::PcUnicornCeremony& C = State;
	const double Fade = C.RootA;
	const FSlateRenderTransform& T = Geometry.GetAccumulatedRenderTransform();
	FPawFxCanvas Canvas;
	// The dim (Palette.INK at DimA), the whole view.
	const FVector2D Size(Geometry.GetLocalSize());
	const FVector2D Dim[4] = { { 0.0, 0.0 }, { Size.X, 0.0 }, { Size.X, Size.Y }, { 0.0, Size.Y } };
	Canvas.Polygon(Dim, Godot(FromToken(PawTokens::INK), C.DimA * Fade));
	// _draw_sky: the rainbow, then every bit.
	std::vector<Pc::PcUnicornArc> Arcs;
	Pc::UnicornArcs(C, Arcs);
	TArray<FVector2D> Points;
	TArray<FLinearColor> Colors;
	for (const Pc::PcUnicornArc& A : Arcs)
	{
		Points.Reset();
		Colors.Reset();
		for (int32 I = 0; I < Pc::Unicorn::ArcPoints; ++I)
		{
			const double Angle = A.From + (A.To - A.From) * I / (Pc::Unicorn::ArcPoints - 1);
			Points.Add(FVector2D(A.Cx + FMath::Cos(Angle) * A.Radius, A.Cy + FMath::Sin(Angle) * A.Radius));
			Colors.Add(Godot(A.Color, Fade));
		}
		Canvas.Polyline(Points, Colors, A.Width, FeatherPx);
		++PaintedShapes;
	}
	std::vector<Pc::PcUnicornShape> Shapes;
	for (const Pc::PcUnicornBit& B : C.Bits)
	{
		Pc::UnicornBitShapes(B, Shapes);
		for (const Pc::PcUnicornShape& S : Shapes)
		{
			Points.Reset();
			Colors.Reset();
			for (size_t I = 0; I < S.Xs.size(); ++I)
			{
				Points.Add(FVector2D(S.Xs[I], S.Ys[I]));
				Colors.Add(Godot(S.Color, Fade));
			}
			if (S.bRim)
			{
				Canvas.Polyline(Points, Colors, Pc::Unicorn::StarRimPx, FeatherPx);
			}
			else if (B.bStar)
			{
				Canvas.Fan(FVector2D(B.X, B.Y), Points, Colors[0]); // a star is star-shaped about its centre, not its tip
			}
			else
			{
				Canvas.Polygon(Points, Colors[0]);
			}
			++PaintedShapes;
		}
	}
	PawUi::PaintCanvas(Canvas, Out, Layer, T);

	// The cat over the sky: its square at (X, Y), mirrored, turned and scaled about the middle of its foot.
	if (HasArt())
	{
		const FPawAtlasCell& Art = Atlas->Cell(Cell);
		const double Side = C.Side;
		const FVector2D Pivot(Side * 0.5, Side);
		const FVector2D Origin(C.X, C.Y);
		const double Cs = FMath::Cos(C.Rot) * C.Scale, Sn = FMath::Sin(C.Rot) * C.Scale;
		auto At = [&](double Lx, double Ly)
		{
			const FVector2D Q(Lx - Pivot.X, Ly - Pivot.Y);
			return Origin + Pivot + FVector2D(Q.X * Cs - Q.Y * Sn, Q.X * Sn + Q.Y * Cs);
		};
		FColor Tint = FColor::White; // the art as it is, faded with the layer
		Tint.A = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Fade * 255.0), 0, 255));
		// flip_h: the left edge shows the art's right.
		TArray<FSlateVertex> Verts = {
			FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, FVector2f(At(0.0, 0.0)), FVector2f(Art.U1, Art.V0), Tint),
			FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, FVector2f(At(Side, 0.0)), FVector2f(Art.U0, Art.V0), Tint),
			FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, FVector2f(At(Side, Side)), FVector2f(Art.U0, Art.V1), Tint),
			FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, FVector2f(At(0.0, Side)), FVector2f(Art.U1, Art.V1), Tint),
		};
		TArray<SlateIndex> Indices = { 0, 1, 2, 0, 2, 3 };
		FSlateDrawElement::MakeCustomVerts(Out, ++Layer, FSlateApplication::Get().GetRenderer()->GetResourceHandle(PageBrush),
			Verts, Indices, nullptr, 0, 0);
		bCatPainted = true;
	}
	return Layer;
}
