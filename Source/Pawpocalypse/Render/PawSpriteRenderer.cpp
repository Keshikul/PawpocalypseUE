#include "Render/PawSpriteRenderer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Render/PawSpriteAtlas.h"
#include "ProceduralMeshComponent.h"
#include "Render/PawFxCanvas.h"
#include "Engine/Texture2D.h"

namespace
{
	constexpr double PlaneSize = 100.0; // /Engine/BasicShapes/Plane is 100 x 100 units
	constexpr int32 ShapeFloats = 4;    // linear R, G, B, A
	constexpr int32 SpriteFloats = 9;   // linear R, G, B, A; the cell's U0, V0, U1, V1; whiten
	const FTransform Parked(FRotator::ZeroRotator, FVector(0.0, 0.0, -10000.0), FVector::ZeroVector);

	// Translucent components draw in this order, whatever their distance (docs/04 R6).
	constexpr int32 FillPriority = -1;
	constexpr int32 ZonesPriority = 0;      // also the Ground effects
	constexpr int32 ShadowPriority = 1;
	constexpr int32 UnderFxPriority = 2;
	constexpr int32 SoftPriority = 3;
	constexpr int32 OverFxPriority = 4;
	constexpr int32 FxPriority = 5;
	constexpr int32 FxBandPriority[] = { ZonesPriority, UnderFxPriority, OverFxPriority };

	int32 ShapeIndex(EPawLayer Layer)
	{
		switch (Layer)
		{
		case EPawLayer::Floor: return 0;
		case EPawLayer::Zones: return 1;
		case EPawLayer::Fx: return 2;
		default: return INDEX_NONE;
		}
	}
}

UInstancedStaticMeshComponent* UPawSpriteRenderer::MakeComponent(const TCHAR* Name, UMaterialInterface* Material, int32 Floats,
	int32 SortPriority)
{
	UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(GetOwner(), Name);
	C->SetStaticMesh(QuadMesh);
	C->SetMaterial(0, Material);
	C->NumCustomDataFloats = Floats;
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetMobility(EComponentMobility::Movable);
	C->SetTranslucentSortPriority(SortPriority);
	C->SetupAttachment(this);
	C->RegisterComponent();
	Components.Add(C);
	return C;
}

void UPawSpriteRenderer::Setup(UStaticMesh* Quad, UMaterialInterface* Flat, UMaterialInterface* Soft, UMaterialInterface* Disc,
	UMaterialInterface* Ring)
{
	if (bSetUp)
	{
		return;
	}
	QuadMesh = Quad;
	FlatFloorMaterial = Flat;
	Shapes[0].Comp = MakeComponent(TEXT("Floor"), Flat, ShapeFloats, 0);
	Shapes[1].Comp = MakeComponent(TEXT("Zones"), Soft, ShapeFloats, ZonesPriority);
	Shapes[2].Comp = MakeComponent(TEXT("Fx"), Ring, ShapeFloats, FxPriority);
	for (FBatch& B : Shapes)
	{
		B.Floats = ShapeFloats;
	}
	bSetUp = true;
}

void UPawSpriteRenderer::SetupSprites(const UPawSpriteAtlas* InAtlas, UMaterialInterface* MaskedMaterial,
	UMaterialInterface* SoftMaterial)
{
	if (!bSetUp || HasSprites() || !InAtlas || !MaskedMaterial || !SoftMaterial || InAtlas->Pages.Num() == 0)
	{
		return;
	}
	Atlas = InAtlas;
	for (int32 P = 0; P < InAtlas->Pages.Num(); ++P)
	{
		for (int32 K = 0; K < KindCount; ++K)
		{
			UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(K == Masked ? MaskedMaterial : SoftMaterial, this);
			M->SetTextureParameterValue(TEXT("Atlas"), InAtlas->Pages[P]);
			PageMaterials.Add(M);
			static const TCHAR* Names[] = { TEXT("Sprites"), TEXT("SpritesSoft"), TEXT("Shadows") };
			FBatch& B = Pages.AddDefaulted_GetRef();
			B.Floats = SpriteFloats;
			B.Comp = MakeComponent(*FString::Printf(TEXT("%s_%d"), Names[K], P), M, SpriteFloats,
				K == Shadow ? ShadowPriority : SoftPriority);
		}
	}
}

void UPawSpriteRenderer::BeginFrame(double LogicalW, double LogicalH)
{
	ViewW = LogicalW;
	ViewH = LogicalH;
	for (FBatch& B : Shapes)
	{
		B.Transforms.Reset();
		B.Custom.Reset();
	}
	for (FBatch& B : Pages)
	{
		B.Transforms.Reset();
		B.Custom.Reset();
	}
	for (TArray<FPawSprite>& S : Sprites)
	{
		S.Reset();
	}
}

FVector UPawSpriteRenderer::FieldToWorld(double X, double Y, double Z) const
{
	// Field (x right, y down) -> world (screen up = +X, screen right = +Y), the view's centre at the origin (spike S1).
	return FVector(ViewH * 0.5 - Y, X - ViewW * 0.5, Z);
}

double UPawSpriteRenderer::ShapeZ(EPawLayer Layer) const
{
	switch (Layer)
	{
	case EPawLayer::Floor: return 0.0;
	case EPawLayer::Props: return 3.0; // + the prop's order (PropZ); the patches and puddles lie at 2
	case EPawLayer::Zones: return 5.0;
	case EPawLayer::Shadows: return 20.0;
	case EPawLayer::Fx: return 300.0;
	default: return 0.0;
	}
}

double UPawSpriteRenderer::BodyZ(double Order)
{
	// Over the shadows and the low effect bands (20, 25), under the top effects (TopFxZ 300). Why the mice get room (the R5
	// rig, 2026-11-09): a rigged mouse is drawn as overlapping masked layers a fraction of a draw place apart, and layers
	// whose depths round to the same value fight for their overlap. (Under the engine's default r.EarlyZPassOnlyMaterialMasking
	// 1 they drew black, the colour pass painting transparent texels; DefaultEngine.ini turns it off.) So the places the mice
	// really use (a mouse's place is 1 + its pool slot, the first free one: under max_live_enemies + 1, Realm test) get 0.4
	// each, the layers 0.12 of a place apart = 0.048 world units:
	//   z 0, places 0..MicePlaces       30 .. 190     the mice
	//   z 0, the rest (the boss, 1999)  190 .. 191.6  in order, packed
	//   z 1-5 (kittens, the cat, its weapon and hat, the adds)  192 .. 287, 0.0095 per place
	const double O = FMath::Clamp(Order, 0.0, 12000.0);
	constexpr double TreeSpan = 2000.0;   // PawLook::TreeSpan
	if (O < MicePlaces)
	{
		return 30.0 + O * MiceStep;
	}
	if (O < TreeSpan)
	{
		return 30.0 + MicePlaces * MiceStep + (O - MicePlaces) * 0.001;
	}
	return 192.0 + (O - TreeSpan) * 0.0095;
}

UPawSpriteRenderer::FBatch& UPawSpriteRenderer::ShapeBatch(EPawLayer Layer)
{
	return Shapes[ShapeIndex(Layer)];
}

void UPawSpriteRenderer::Add(EPawLayer Layer, double X, double Y, double Width, double Height, const FLinearColor& Color)
{
	if (ShapeIndex(Layer) == INDEX_NONE)
	{
		return;
	}
	FBatch& B = ShapeBatch(Layer);
	// Yaw +90 keeps the plane's UVs upright on screen (spike S1); it turns the plane's local X to world +Y (screen right),
	// so the width scales local X and the height local Y.
	B.Transforms.Emplace(FRotator(0.0, 90.0, 0.0), FieldToWorld(X, Y, ShapeZ(Layer)), FVector(Width / PlaneSize, Height / PlaneSize, 1.0));
	B.Custom.Append({ Color.R, Color.G, Color.B, Color.A });
}

void UPawSpriteRenderer::AddSprite(EPawLayer Layer, const FPawSprite& S)
{
	if (Layer != EPawLayer::Props && Layer != EPawLayer::Shadows && Layer != EPawLayer::Bodies && Layer != EPawLayer::Overlays)
	{
		return;
	}
	Sprites[static_cast<int32>(Layer)].Add(S);
	if (!HasSprites() || !Atlas->Cells.IsValidIndex(S.Cell))
	{
		return;
	}
	const FPawAtlasCell& C = Atlas->Cell(S.Cell);
	if (!Atlas->Pages.IsValidIndex(C.Page))
	{
		return;
	}
	const bool bSoft = S.Tint.A < 1.0f || S.Whiten > 0.0f;
	const int32 Kind = Layer == EPawLayer::Shadows ? Shadow : bSoft ? Translucent : Masked;
	FBatch& B = Pages[C.Page * KindCount + Kind];
	// Props: 3 + 0.004 per place in the backdrop's order (under 2.0 for 500 props), so a later prop covers an earlier one.
	const double Z = Layer == EPawLayer::Shadows ? ShapeZ(EPawLayer::Shadows)
		: Layer == EPawLayer::Props ? ShapeZ(EPawLayer::Props) + FMath::Clamp(S.Order, 0.0, 490.0) * 0.004
		: BodyZ(S.Order);
	// Screen-clockwise is a positive yaw (screen up = +X, screen right = +Y, seen from +Z).
	B.Transforms.Emplace(FRotator(0.0, 90.0 + FMath::RadiansToDegrees(S.Angle), 0.0), FieldToWorld(S.X, S.Y, Z),
		FVector(S.Width / PlaneSize, S.Height / PlaneSize, 1.0));
	// A mirrored sprite reads its cell right to left.
	const float U0 = S.bFlipX ? C.U1 : C.U0;
	const float U1 = S.bFlipX ? C.U0 : C.U1;
	B.Custom.Append({ S.Tint.R, S.Tint.G, S.Tint.B, S.Tint.A, U0, C.V0, U1, C.V1, S.Whiten });
}

void UPawSpriteRenderer::Flush(FBatch& B)
{
	UInstancedStaticMeshComponent* C = B.Comp;
	if (!C)
	{
		return;
	}
	const int32 Used = B.Transforms.Num();
	// Grow the pool to the high-water mark; never shrink it.
	if (C->GetInstanceCount() < Used)
	{
		TArray<FTransform> More;
		More.Init(Parked, Used - C->GetInstanceCount());
		C->AddInstances(More, false, false, false);
	}
	const int32 Pool = C->GetInstanceCount();
	for (int32 I = Used; I < Pool; ++I)
	{
		B.Transforms.Add(Parked);
		B.Custom.AddZeroed(B.Floats);
	}
	for (int32 I = 0; I < Pool; ++I)
	{
		C->SetCustomData(I, TArrayView<const float>(B.Custom.GetData() + I * B.Floats, B.Floats), false);
	}
	if (Pool > 0)
	{
		// One batch per component; its render state is dirtied once, here (the custom data above did not).
		C->BatchUpdateInstancesTransforms(0, B.Transforms, false, true, false);
	}
	B.Drawn = Used;
}

void UPawSpriteRenderer::EndFrame()
{
	if (!bSetUp)
	{
		return;
	}
	for (FBatch& B : Shapes)
	{
		Flush(B);
	}
	for (FBatch& B : Pages)
	{
		Flush(B);
	}
	for (int32 L = 0; L < static_cast<int32>(EPawLayer::Count); ++L)
	{
		const int32 Shape = ShapeIndex(static_cast<EPawLayer>(L));
		Drawn[L] = Shape != INDEX_NONE ? Shapes[Shape].Drawn : Sprites[L].Num();
	}
}

int32 UPawSpriteRenderer::DrawnCount(EPawLayer Layer) const
{
	return Drawn[static_cast<int32>(Layer)];
}

int32 UPawSpriteRenderer::PoolSize(EPawLayer Layer) const
{
	const int32 Shape = ShapeIndex(Layer);
	if (Shape != INDEX_NONE)
	{
		return Shapes[Shape].Comp ? Shapes[Shape].Comp->GetInstanceCount() : 0;
	}
	// The sprite layers share their components: Shadows has its own, Bodies and Overlays the masked and soft ones.
	int32 N = 0;
	for (int32 I = 0; I < Pages.Num(); ++I)
	{
		const bool bShadow = I % KindCount == Shadow;
		if (bShadow == (Layer == EPawLayer::Shadows) && Pages[I].Comp)
		{
			N += Pages[I].Comp->GetInstanceCount();
		}
	}
	return N;
}

void UPawSpriteRenderer::SetFloorTexture(UTexture2D* Texture, UMaterialInterface* FloorMaterial)
{
	if (!bSetUp || Texture == FloorTexture)
	{
		return;
	}
	FloorTexture = Texture;
	if (!Texture || !FloorMaterial)
	{
		Shapes[0].Comp->SetMaterial(0, FlatFloorMaterial);
		return;
	}
	if (!FloorMaterialInstance || FloorMaterialInstance->Parent != FloorMaterial)
	{
		FloorMaterialInstance = UMaterialInstanceDynamic::Create(FloorMaterial, this);
	}
	FloorMaterialInstance->SetTextureParameterValue(TEXT("Floor"), Texture);
	Shapes[0].Comp->SetMaterial(0, FloorMaterialInstance);
}

void UPawSpriteRenderer::AddFloor(double LogicalW, double LogicalH, double TilePx)
{
	// M_Floor reads its UV repeat from custom data 0 and 1: the quad's UVs run 0..1 from the view's top-left, so the first
	// tile's corner sits at (0, 0), as Godot's draw_texture_rect(tile = true) puts it.
	const double Tile = FMath::Max(TilePx, 1.0);
	Add(EPawLayer::Floor, LogicalW * 0.5, LogicalH * 0.5, LogicalW, LogicalH,
		FLinearColor(static_cast<float>(LogicalW / Tile), static_cast<float>(LogicalH / Tile), 0.0f, 1.0f));
}

void UPawSpriteRenderer::SetBackdropFill(const TArray<FVector2D>& Points, const TArray<FColor>& Colors, const TArray<int32>& Triangles,
	UMaterialInterface* FillMaterial)
{
	if (!bSetUp)
	{
		return;
	}
	if (!Fill)
	{
		Fill = NewObject<UProceduralMeshComponent>(GetOwner(), TEXT("BackdropFill"));
		Fill->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Fill->SetCastShadow(false);
		Fill->SetTranslucentSortPriority(FillPriority);
		Fill->SetupAttachment(this);
		Fill->RegisterComponent();
	}
	Fill->ClearAllMeshSections();
	FillTriangles = Triangles.Num() / 3;
	if (Triangles.Num() == 0 || !FillMaterial)
	{
		return;
	}
	TArray<FVector> Verts;
	Verts.Reserve(Points.Num());
	for (const FVector2D& P : Points)
	{
		Verts.Add(FieldToWorld(P.X, P.Y, 2.0));
	}
	TArray<FVector> Normals;
	Normals.Init(FVector::UpVector, Points.Num());
	Fill->CreateMeshSection(0, Verts, Triangles, Normals, TArray<FVector2D>(), Colors, TArray<FProcMeshTangent>(), false);
	Fill->SetMaterial(0, FillMaterial);
}

void UPawSpriteRenderer::SetFx(EPawFxBand Band, const FPawFxCanvas& Canvas, UMaterialInterface* FxMaterial)
{
	const int32 B = static_cast<int32>(Band);
	if (!bSetUp || B < 0 || B >= static_cast<int32>(EPawFxBand::Count))
	{
		return;
	}
	if (FxMeshes.Num() == 0)
	{
		static const TCHAR* Names[] = { TEXT("FxGround"), TEXT("FxUnder"), TEXT("FxOver") };
		for (int32 I = 0; I < static_cast<int32>(EPawFxBand::Count); ++I)
		{
			UProceduralMeshComponent* M = NewObject<UProceduralMeshComponent>(GetOwner(), Names[I]);
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			M->SetCastShadow(false);
			M->SetTranslucentSortPriority(FxBandPriority[I]);
			M->bUseAsyncCooking = false;
			M->SetupAttachment(this);
			M->RegisterComponent();
			FxMeshes.Add(M);
		}
	}
	UProceduralMeshComponent* M = FxMeshes[B];
	FxTris[B] = Canvas.Indices.Num() / 3;
	if (Canvas.IsEmpty() || !FxMaterial)
	{
		// An empty band stays as it is. A clear rebuilds the mesh's collision body and its scene proxy, so it is made once,
		// when the band empties, not every frame (review CR5: three bands did it every frame, empty or not).
		if (FxFull[B])
		{
			++FxRebuilds;
			M->ClearAllMeshSections();
			FxFull[B] = false;
		}
		return;
	}
	TArray<FVector> Verts;
	Verts.Reserve(Canvas.Verts.Num());
	for (const FVector& V : Canvas.Verts)
	{
		Verts.Add(FieldToWorld(V.X, V.Y, V.Z));
	}
	TArray<FVector> Normals;
	Normals.Init(FVector::UpVector, Verts.Num());
	++FxRebuilds;
	M->CreateMeshSection(0, Verts, Canvas.Indices, Normals, TArray<FVector2D>(), Canvas.Colors, TArray<FProcMeshTangent>(), false);
	M->SetMaterial(0, FxMaterial);
	FxFull[B] = true;
}

const TArray<FPawSprite>& UPawSpriteRenderer::LastSprites(EPawLayer Layer) const
{
	return Sprites[static_cast<int32>(Layer)];
}
