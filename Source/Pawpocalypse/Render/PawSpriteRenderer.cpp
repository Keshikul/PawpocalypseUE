#include "Render/PawSpriteRenderer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr double PlaneSize = 100.0; // /Engine/BasicShapes/Plane is 100 x 100 units
	constexpr int32 CustomFloats = 4;
	const FTransform Parked(FRotator::ZeroRotator, FVector(0.0, 0.0, -10000.0), FVector::ZeroVector);
}

void UPawSpriteRenderer::Setup(UStaticMesh* Quad, UMaterialInterface* Flat, UMaterialInterface* Soft, UMaterialInterface* Disc,
	UMaterialInterface* Ring)
{
	if (bSetUp)
	{
		return;
	}
	UMaterialInterface* Materials[] = { Flat, Soft, Soft, Disc, Ring };
	const TCHAR* Names[] = { TEXT("Floor"), TEXT("Zones"), TEXT("Shadows"), TEXT("Bodies"), TEXT("Fx") };
	for (int32 L = 0; L < static_cast<int32>(EPawLayer::Count); ++L)
	{
		UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(GetOwner(), Names[L]);
		C->SetStaticMesh(Quad);
		C->SetMaterial(0, Materials[L]);
		C->NumCustomDataFloats = CustomFloats;
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(this);
		C->RegisterComponent();
		Layers[L].Comp = C;
		Components.Add(C);
	}
	bSetUp = true;
}

void UPawSpriteRenderer::BeginFrame(double LogicalW, double LogicalH)
{
	ViewW = LogicalW;
	ViewH = LogicalH;
	for (FLayer& L : Layers)
	{
		L.Transforms.Reset();
		L.Custom.Reset();
	}
}

FVector UPawSpriteRenderer::FieldToWorld(double X, double Y, double Z) const
{
	// Field (x right, y down) -> world (screen up = +X, screen right = +Y), the view's centre at the origin (spike S1).
	return FVector(ViewH * 0.5 - Y, X - ViewW * 0.5, Z);
}

double UPawSpriteRenderer::LayerZ(EPawLayer Layer, double Y) const
{
	switch (Layer)
	{
	case EPawLayer::Floor: return 0.0;
	case EPawLayer::Zones: return 5.0;
	case EPawLayer::Shadows: return 20.0;
	case EPawLayer::Bodies: return 100.0 + FMath::Clamp(Y / FMath::Max(ViewH, 1.0), -0.4, 1.4) * 100.0; // lower = in front
	case EPawLayer::Fx: return 300.0;
	default: return 0.0;
	}
}

void UPawSpriteRenderer::Add(EPawLayer Layer, double X, double Y, double Width, double Height, const FLinearColor& Color)
{
	FLayer& L = Layers[static_cast<int32>(Layer)];
	// Yaw +90 keeps the plane's UVs upright on screen (spike S1); it turns the plane's local X to world +Y (screen right),
	// so the width scales local X and the height local Y.
	L.Transforms.Emplace(FRotator(0.0, 90.0, 0.0), FieldToWorld(X, Y, LayerZ(Layer, Y)),
		FVector(Width / PlaneSize, Height / PlaneSize, 1.0));
	L.Custom.Append({ Color.R, Color.G, Color.B, Color.A });
}

void UPawSpriteRenderer::EndFrame()
{
	if (!bSetUp)
	{
		return;
	}
	for (FLayer& L : Layers)
	{
		UInstancedStaticMeshComponent* C = L.Comp;
		const int32 Used = L.Transforms.Num();
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
			L.Transforms.Add(Parked);
			L.Custom.Append({ 0.0f, 0.0f, 0.0f, 0.0f });
		}
		for (int32 I = 0; I < Pool; ++I)
		{
			C->SetCustomData(I, TArrayView<const float>(L.Custom.GetData() + I * CustomFloats, CustomFloats), false);
		}
		if (Pool > 0)
		{
			// One batch per layer; its render state is dirtied once, here (the custom data above did not).
			C->BatchUpdateInstancesTransforms(0, L.Transforms, false, true, false);
		}
		L.Drawn = Used;
	}
}

int32 UPawSpriteRenderer::DrawnCount(EPawLayer Layer) const
{
	return Layers[static_cast<int32>(Layer)].Drawn;
}

int32 UPawSpriteRenderer::PoolSize(EPawLayer Layer) const
{
	const FLayer& L = Layers[static_cast<int32>(Layer)];
	return L.Comp ? L.Comp->GetInstanceCount() : 0;
}
