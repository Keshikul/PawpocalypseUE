#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "PawSpriteRenderer.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

// The draw layers, back to front, each one instanced component (docs/04 §5.1, the renderer spike S2 chose). Bodies are
// sorted by their feet's Y through their depth; the others are flat bands where order inside the band does not matter.
enum class EPawLayer : uint8
{
	Floor,   // opaque: the backdrop
	Zones,   // translucent soft discs: the aura, the poop, trails, echoes, craters
	Shadows, // translucent soft ellipses under the bodies
	Bodies,  // masked discs: mice, kittens, the cat, the boss, its adds (placeholder shapes until phase 6's atlas)
	Fx,      // translucent rings: strikes, kills, pounces, blasts (additive showed nothing on the light floor)
	Count
};

// Draws the field as instanced quads: the caller adds every item of the frame between BeginFrame and EndFrame; EndFrame
// writes each layer in one batch. Instances are pooled at a high-water mark and parked (scale 0, below the floor) when
// unused, never removed: removal reorders instance indices (docs/04 §5.1).
//
// Coordinates are field pixels (x right, y down, origin at the view's top-left); one field pixel is one world unit, the
// orthographic camera looks down -Z with screen up = +X (spike S1).
UCLASS()
class PAWPOCALYPSE_API UPawSpriteRenderer : public USceneComponent
{
	GENERATED_BODY()

public:
	// Creates the layers' components. Mesh: a 100 x 100 plane; one material per layer (Floor, Zones, Shadows, Bodies, Fx).
	void Setup(UStaticMesh* Quad, UMaterialInterface* Flat, UMaterialInterface* Soft, UMaterialInterface* Disc,
		UMaterialInterface* Ring);
	bool IsSetUp() const { return bSetUp; }

	// The view's logical size (docs/04 §6: height 1080, width from the window's aspect).
	void BeginFrame(double LogicalW, double LogicalH);
	// An item centred at (X, Y), Width x Height px (a disc when equal), in linear colour; Alpha is the opacity (ignored by
	// the opaque Floor and the masked Bodies).
	void Add(EPawLayer Layer, double X, double Y, double Width, double Height, const FLinearColor& Color);
	void EndFrame();

	// What the last EndFrame drew in a layer, and the pool behind it (tests, the debug HUD).
	int32 DrawnCount(EPawLayer Layer) const;
	int32 PoolSize(EPawLayer Layer) const;

	// Field pixels -> world, for the camera and the tests.
	FVector FieldToWorld(double X, double Y, double Z) const;

private:
	struct FLayer
	{
		TObjectPtr<UInstancedStaticMeshComponent> Comp;
		TArray<FTransform> Transforms;
		TArray<float> Custom; // 4 floats per item: linear R, G, B, A
		int32 Drawn = 0;
	};

	double LayerZ(EPawLayer Layer, double Y) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Components;

	FLayer Layers[static_cast<int32>(EPawLayer::Count)];
	double ViewW = 1920.0;
	double ViewH = 1080.0;
	bool bSetUp = false;
};
