#include "UI/PawUnicornCeremony.h"
#include "UI/PawUiTime.h"

#include "Audio/PawAudioSubsystem.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Render/PawFxCanvas.h"
#include "Render/PawSpriteAtlas.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "UI/PawIcons.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

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
		RaysBrush = PawUi::IconBrush(TEXT("ui_icon_unicorn_rays"), 1920.0);
		LinePlate = PawUi::Rounded(PawTokens::UI_PANEL, PawTokens::INK, 3.5f, 24.0f);
	}
	return bOk;
}

FText UPawUnicornCeremony::GetTitleText() const
{
	return PawText::Data(TEXT("SKILL"), "unlock_unicorn").ToUpper(); // the skill's name in capitals (FText::ToUpper: per culture)
}

FText UPawUnicornCeremony::GetLineText() const
{
	return PawText::Ui(TEXT("REAWAKEN_unicorn"));
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
	DeltaSeconds = PawUi::StepDelta(this, DeltaSeconds); // under a fixed step, that step once a frame (PawUiTime.h)
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
	bRaysPainted = false;
	bTitlePainted = false;
	if (!bPlaying)
	{
		return Layer;
	}
	const Pc::PcUnicornCeremony& C = State;
	const double Fade = C.RootA;
	const FSlateRenderTransform& T = Geometry.GetAccumulatedRenderTransform();
	FPawFxCanvas Canvas;
	const FVector2D Size(Geometry.GetLocalSize());
	// D18 (S4, the UI concept's scrUnicorn): its coral rays and sparks behind the rainbow, faded in and out as Godot's dim
	// (Palette.INK at DimA) was; the dim where the picture is missing.
	const double Show = FMath::Clamp(C.DimA / Pc::Unicorn::DimAlpha, 0.0, 1.0) * Fade;
	if (RaysBrush.GetResourceObject())
	{
		FSlateDrawElement::MakeBox(Out, Layer++, Geometry.ToPaintGeometry(), &RaysBrush, ESlateDrawEffect::None,
			FLinearColor::White.CopyWithNewOpacity(static_cast<float>(Show)));
		bRaysPainted = Show > 0.0;
	}
	else
	{
		const FVector2D Dim[4] = { { 0.0, 0.0 }, { Size.X, 0.0 }, { Size.X, Size.Y }, { 0.0, Size.Y } };
		Canvas.Polygon(Dim, Godot(FromToken(PawTokens::INK), C.DimA * Fade));
	}
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
	// D18 (S4, the UI concept's scrUnicorn): UNICORN as its sticker title (yellow on a thick ink outline) at the top, and the
	// line that says what changed on a cream plate under the rider; both fade with the rays. In the concept's 1600 x 900 frame.
	// The title at 80 (the concept's 140): the game's rainbow and rider stand higher than the concept's, over its 140.
	if (Show > 0.01)
	{
		const double Kx = Size.X / 1600.0, Ky = Size.Y / 900.0, K = FMath::Min(Kx, Ky);
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		FSlateFontInfo TitleFont(PawUi::PawFont(), static_cast<float>(PawUi::FontPt(84.0 * K)), FName(TEXT("Black")));
		TitleFont.OutlineSettings.OutlineSize = FMath::RoundToInt(PawUi::CeremonyTitleOutline * 2.0 * K);
		TitleFont.OutlineSettings.OutlineColor = PawTokens::INK.CopyWithNewOpacity(static_cast<float>(Show));
		const FString Title = GetTitleText().ToString();
		const FVector2D TitleSize = Measure->Measure(Title, TitleFont);
		FSlateDrawElement::MakeText(Out, ++Layer, Geometry.ToPaintGeometry(FVector2f(TitleSize),
			FSlateLayoutTransform(FVector2f(static_cast<float>(800.0 * Kx - TitleSize.X * 0.5), static_cast<float>(80.0 * Ky - TitleSize.Y * 0.5)))),
			Title, TitleFont, ESlateDrawEffect::None, PawTokens::UI_SELECTED.CopyWithNewOpacity(static_cast<float>(Show)));
		bTitlePainted = true;
		const FVector2D PlatePos(470.0 * Kx, 720.0 * Ky), PlateSize(660.0 * Kx, 86.0 * Ky);
		FSlateDrawElement::MakeBox(Out, ++Layer, Geometry.ToPaintGeometry(FVector2f(PlateSize), FSlateLayoutTransform(FVector2f(PlatePos))),
			&LinePlate, ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(static_cast<float>(Show)));
		FSlateFontInfo LineFont(PawUi::PawFont(), static_cast<float>(PawUi::FontPt(26.0 * K)), FName(TEXT("Black")));
		const FString Line = GetLineText().ToString();
		const FVector2D LineSize = Measure->Measure(Line, LineFont);
		FSlateDrawElement::MakeText(Out, ++Layer, Geometry.ToPaintGeometry(FVector2f(LineSize),
			FSlateLayoutTransform(FVector2f(PlatePos + (PlateSize - LineSize) * 0.5))),
			Line, LineFont, ESlateDrawEffect::None, PawTokens::UI_TEXT.CopyWithNewOpacity(static_cast<float>(Show)));
	}
	return Layer;
}
