#include "UI/PawGarden.h"

#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "PcBackdropTones.h"
#include "Render/PawSpriteAtlas.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Save/PawSettingsSubsystem.h"
#include "Styling/CoreStyle.h"
#include "UI/PawWidgets.h"

namespace
{
	const TCHAR* AtlasPath = TEXT("/Game/Pawpocalypse/Art/Field/DA_FieldAtlas.DA_FieldAtlas");
	const TCHAR* FloorPath = TEXT("/Game/Pawpocalypse/Art/Backdrops/T_backdrop_tier_1.T_backdrop_tier_1"); // the menu borrows the Backyard's

	const Pc::PcBackdropTones* Tones(const std::string& Id)
	{
		for (const Pc::PcBackdropTones& T : Pc::BackdropTones)
		{
			if (Id == T.Id)
			{
				return &T;
			}
		}
		return nullptr;
	}

	FSlateVertex Vertex(const FSlateRenderTransform& T, const FVector2D& P, const FVector2f& Uv, const FColor& C)
	{
		return FSlateVertex::Make<ESlateVertexRounding::Disabled>(T, FVector2f(P), Uv, C);
	}
}

bool UPawGarden::Initialize()
{
	const bool bOk = Super::Initialize();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		// Both live under /Game/Pawpocalypse (always cooked); loaded once, with the title.
		Atlas = LoadObject<UPawSpriteAtlas>(nullptr, AtlasPath);
		FloorTexture = LoadObject<UTexture2D>(nullptr, FloorPath);
		if (FloorTexture)
		{
			// _draw_backdrop: the floor tiled at half its texture's size from the top-left.
			FloorBrush.SetResourceObject(FloorTexture);
			FloorBrush.ImageSize = FVector2D(FloorTexture->GetSizeX(), FloorTexture->GetSizeY()) * Pc::Backdrop::TileScale;
			FloorBrush.Tiling = ESlateBrushTileType::Both;
			FloorBrush.DrawAs = ESlateBrushDrawType::Image;
		}
		PageBrushes.Reset();
		if (Atlas)
		{
			for (UTexture2D* Page : Atlas->Pages)
			{
				FSlateBrush B;
				B.SetResourceObject(Page);
				PageBrushes.Add(B);
			}
		}
	}
	return bOk;
}

double UPawGarden::Budget() const
{
	const UGameInstance* GI = GetGameInstance();
	const UPawSettingsSubsystem* S = GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	return S ? S->EffectBudget() : 1.0;
}

void UPawGarden::Relayout(const FVector2D& Size)
{
	auto Cells = [this](const Pc::PcBackdropLayout& L, TArray<int32>& Out)
	{
		Out.Reset();
		for (const Pc::PcBackdropProp& P : L.Props)
		{
			Out.Add(Atlas ? Atlas->Find(UTF8_TO_TCHAR(P.Stem.c_str())) : INDEX_NONE);
		}
	};
	if (Size != LaidOutSize)
	{
		LaidOutSize = Size;
		Scatter = Pc::BuildBackdrop("menu", Size.X, Size.Y, Budget());
		Cells(Scatter, ScatterCells);
		FrameBudget = -1.0;
	}
	if (const double Now = Budget(); Now != FrameBudget)
	{
		FrameBudget = Now;
		Frame = Pc::BuildBackdrop("menu", Size.X, Size.Y, Now);
		Cells(Frame, FrameCells);
	}
}

void UPawGarden::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	Clock += DeltaSeconds;
	const FVector2D Size(Geometry.GetLocalSize());
	if (Size.X > 0.0 && Size.Y > 0.0)
	{
		Relayout(Size);
	}
}

int32 UPawGarden::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, Geometry, Culling, Out, LayerId, Style, bParentEnabled) + 1;
	if (!HasArt() || LaidOutSize.X <= 0.0)
	{
		return Layer;
	}
	const FSlateRenderTransform& T = Geometry.GetAccumulatedRenderTransform();
	auto* Renderer = FSlateApplication::Get().GetRenderer();

	// The floor.
	FSlateDrawElement::MakeBox(Out, Layer, Geometry.ToPaintGeometry(), &FloorBrush, ESlateDrawEffect::None, FLinearColor::White);

	// The patches, then each puddle and its rim (the Rooftops' sky in the Backyard's palette), one batch in Godot's order.
	if (const Pc::PcBackdropTones* Own = Tones(Frame.TonesId))
	{
		const Pc::PcBackdropTones* Water = Frame.WaterTonesId.empty() ? nullptr : Tones(Frame.WaterTonesId);
		auto Byte = [](double A) { return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(A * 255.0), 0, 255)); };
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;
		const FVector2f Uv(0.5f, 0.5f);
		for (const Pc::PcBackdropBlob& B : Frame.Blobs)
		{
			const Pc::PcTone& Tone = B.bPuddle ? (Water ? Water->Field : Own->Puddle) : Own->Patch;
			const FColor Fill(Tone.R, Tone.G, Tone.B, Byte(B.bPuddle ? Pc::Backdrop::PuddleAlpha : Pc::Backdrop::PatchAlpha));
			const int32 N = static_cast<int32>(B.Xs.size());
			const SlateIndex Base = static_cast<SlateIndex>(Verts.Num());
			Verts.Add(Vertex(T, FVector2D(B.AtX, B.AtY), Uv, Fill));
			for (int32 I = 0; I < N; ++I)
			{
				Verts.Add(Vertex(T, FVector2D(B.Xs[I], B.Ys[I]), Uv, Fill));
			}
			for (int32 I = 0; I < N; ++I)
			{
				Indices.Append({ Base, static_cast<SlateIndex>(Base + 1 + I), static_cast<SlateIndex>(Base + 1 + (I + 1) % N) });
			}
			if (!B.bPuddle)
			{
				continue;
			}
			const FColor Rim(Own->Detail.R, Own->Detail.G, Own->Detail.B, Byte(Pc::Backdrop::RimAlpha));
			const SlateIndex RimBase = static_cast<SlateIndex>(Verts.Num());
			for (int32 I = 0; I < N; ++I)
			{
				const FVector2D P(B.Xs[I], B.Ys[I]);
				const FVector2D A = (P - FVector2D(B.Xs[(I + N - 1) % N], B.Ys[(I + N - 1) % N])).GetSafeNormal();
				const FVector2D C = (FVector2D(B.Xs[(I + 1) % N], B.Ys[(I + 1) % N]) - P).GetSafeNormal();
				const FVector2D Half = (FVector2D(-A.Y, A.X) + FVector2D(-C.Y, C.X)).GetSafeNormal() * (Pc::Backdrop::PuddleRimPx * 0.5);
				Verts.Add(Vertex(T, P - Half, Uv, Rim));
				Verts.Add(Vertex(T, P + Half, Uv, Rim));
			}
			for (int32 I = 0; I < N; ++I)
			{
				const SlateIndex A0 = static_cast<SlateIndex>(RimBase + 2 * I), A1 = A0 + 1;
				const SlateIndex B0 = static_cast<SlateIndex>(RimBase + 2 * ((I + 1) % N)), B1 = B0 + 1;
				Indices.Append({ A0, B0, A1, A1, B0, B1 });
			}
		}
		if (Indices.Num() > 0)
		{
			const FSlateResourceHandle White = Renderer->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
			FSlateDrawElement::MakeCustomVerts(Out, ++Layer, White, Verts, Indices, nullptr, 0, 0);
		}
	}

	// The props in Godot's order: the scatter and the trees (laid out on a resize), then the border rows and the corners. Each
	// one swaying about the middle of its bottom edge (_draw_swaying), mirrored in place when its width is negative.
	auto DrawProp = [&](const Pc::PcBackdropProp& P, int32 Cell)
	{
		if (!Atlas->Cells.IsValidIndex(Cell) || !PageBrushes.IsValidIndex(Atlas->Cell(Cell).Page))
		{
			return;
		}
		const FPawAtlasCell& C = Atlas->Cell(Cell);
		const double W = FMath::Abs(P.W);
		const FVector2D Root(P.X + W * 0.5, P.Y + P.H);
		const double Angle = Pc::BackdropSway(P, Clock);
		const double Cs = FMath::Cos(Angle), Sn = FMath::Sin(Angle);
		auto At = [&](double X, double Y) { return Root + FVector2D(X * Cs - Y * Sn, X * Sn + Y * Cs); };
		const float U0 = P.W < 0.0 ? C.U1 : C.U0;
		const float U1 = P.W < 0.0 ? C.U0 : C.U1;
		const FColor White = FColor::White;
		TArray<FSlateVertex> Verts = {
			Vertex(T, At(-W * 0.5, -P.H), FVector2f(U0, C.V0), White),
			Vertex(T, At(W * 0.5, -P.H), FVector2f(U1, C.V0), White),
			Vertex(T, At(W * 0.5, 0.0), FVector2f(U1, C.V1), White),
			Vertex(T, At(-W * 0.5, 0.0), FVector2f(U0, C.V1), White),
		};
		TArray<SlateIndex> Indices = { 0, 1, 2, 0, 2, 3 };
		FSlateDrawElement::MakeCustomVerts(Out, ++Layer, Renderer->GetResourceHandle(PageBrushes[C.Page]), Verts, Indices, nullptr, 0, 0);
	};
	for (int32 I = 0; I < static_cast<int32>(Scatter.Props.size()); ++I)
	{
		const Pc::PcBackdropProp& P = Scatter.Props[static_cast<size_t>(I)];
		if (P.Kind == Pc::PcBackdropPropKind::Scatter || P.Kind == Pc::PcBackdropPropKind::Landmark)
		{
			DrawProp(P, ScatterCells[I]);
		}
	}
	for (int32 I = 0; I < static_cast<int32>(Frame.Props.size()); ++I)
	{
		const Pc::PcBackdropProp& P = Frame.Props[static_cast<size_t>(I)];
		if (P.Kind == Pc::PcBackdropPropKind::Border || P.Kind == Pc::PcBackdropPropKind::Edge)
		{
			DrawProp(P, FrameCells[I]);
		}
	}
	return Layer;
}
