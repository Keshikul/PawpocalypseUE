#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "PawSpriteRenderer.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPawSpriteAtlas;
class FPawFxCanvas;
class UProceduralMeshComponent;
class UStaticMesh;
class UTexture2D;

// The draw layers, back to front (docs/04 §5.1, the renderer spike S2 chose). Floor, Zones and Fx are shapes computed by
// their materials (one instanced component each); Shadows, Bodies and Overlays are atlas sprites (phase 6).
enum class EPawLayer : uint8
{
	Floor,    // opaque: the backdrop's floor, a texture tiled from the view's top-left (or flat paper without one)
	Props,    // the backdrop's props (atlas sprites, masked), over its patches and puddles, under everything that moves
	Zones,    // translucent soft discs: the aura, the poop, trails, echoes, craters
	Shadows,  // the ground shadow under every creature: ONE band under all of them (Godot's GroundShadow z -1)
	Bodies,   // the creatures: mice, kittens, the cat, the boss, its adds, in Godot's draw order (FPawSprite::Order)
	Overlays, // drawn with the bodies, in their order, but not a creature: signature glows, the cat's weapon and hat
	Fx,       // translucent rings: strikes, kills, pounces, blasts (additive showed nothing on the light floor)
	Count
};

// The effects' bands (phase 6 slice 3), each one translucent mesh rebuilt every frame from an FPawFxCanvas, in Godot's order:
//   Ground  under the creatures' shadows (Godot's z -1 nodes before the enemies in the tree: Edge Burn's strips and embers)
//   Under   over the shadows, under everything that stands (the poop zone, the damage zones: z -1, after the enemies)
//   Over    among and over the sprites: each vertex at the depth of its Godot z and tree place, so a masked sprite in front
//           covers it (the Lone Hunter's halo, the aura ring, the slam, the sparkle, the boss's telegraph, then the swipes,
//           beams, bursts and shards on top)
enum class EPawFxBand : uint8
{
	Ground,
	Under,
	Over,
	Count
};

// One atlas sprite of a frame.
struct FPawSprite
{
	int32 Cell = INDEX_NONE;   // into the atlas (UPawSpriteAtlas::Find)
	double X = 0.0, Y = 0.0;   // its centre, field pixels
	double Width = 0.0, Height = 0.0;
	FLinearColor Tint = FLinearColor::White; // LINEAR (PawLook::Modulate turns Godot's modulate into it); A < 1: translucent
	bool bFlipX = false;       // Sprite2D.flip_h
	float Whiten = 0.0f;       // 0 = the art; 1 = a white silhouette of it (the signature glow)
	double Angle = 0.0;        // radians, clockwise on screen, about the sprite's centre (the title garden's sway)
	// Godot's draw order: z_index first, then the order in the scene tree (PawLook::Order). Higher = in front.
	double Order = 0.0;
};

// Draws the field as instanced quads: the caller adds every item of the frame between BeginFrame and EndFrame; EndFrame
// writes each component in one batch. Instances are pooled at a high-water mark and parked (scale 0, below the floor) when
// unused, never removed: removal reorders instance indices (docs/04 §5.1).
//
// Sprites: per atlas page, a MASKED component (opaque art: per-pixel depth, so Godot's order holds whatever the instance
// order), a SOFT one (art with alpha < 1 or whitened; translucent, drawn after the masked ones and depth-tested against them)
// and a SHADOW one (translucent, under everything that stands). Translucent components are ordered by sort priority
// (Zones < Shadows < Soft < Fx); inside one component translucent instances do not sort (docs/04 R6), which only matters
// where two translucent sprites overlap.
//
// Coordinates are field pixels (x right, y down, origin at the view's top-left); one field pixel is one world unit, the
// orthographic camera looks down -Z with screen up = +X (spike S1).
UCLASS()
class PAWPOCALYPSE_API UPawSpriteRenderer : public USceneComponent
{
	GENERATED_BODY()

public:
	// Creates the shape layers' components. Mesh: a 100 x 100 plane; one material each for Floor, Zones, Fx (Soft is the
	// zones'; Disc is no longer drawn with but kept for the debug look).
	void Setup(UStaticMesh* Quad, UMaterialInterface* Flat, UMaterialInterface* Soft, UMaterialInterface* Disc,
		UMaterialInterface* Ring);
	// The sprite layers: three components per atlas page, each with an instance of its material wearing the page.
	void SetupSprites(const UPawSpriteAtlas* InAtlas, UMaterialInterface* MaskedMaterial, UMaterialInterface* SoftMaterial);
	bool IsSetUp() const { return bSetUp; }
	bool HasSprites() const { return Atlas != nullptr && Pages.Num() > 0; }
	const UPawSpriteAtlas* GetAtlas() const { return Atlas; }

	// The view's logical size (docs/04 §6: height 1080, width from the window's aspect).
	void BeginFrame(double LogicalW, double LogicalH);
	// A shape centred at (X, Y), Width x Height px (a disc when equal), in linear colour; Alpha is the opacity (ignored by
	// the opaque Floor). Layer: Floor, Zones or Fx.
	void Add(EPawLayer Layer, double X, double Y, double Width, double Height, const FLinearColor& Color);
	// An atlas sprite. Layer: Props, Shadows, Bodies or Overlays (a prop's Order is its place in the backdrop's draw order).
	void AddSprite(EPawLayer Layer, const FPawSprite& Sprite);

	// The floor: Texture tiled at TilePx per tile from the view's top-left (biome_backdrop.gd _draw_backdrop), drawn by AddFloor
	// each frame. A null texture keeps the flat floor (Add(Floor, ...)).
	void SetFloorTexture(UTexture2D* Texture, UMaterialInterface* FloorMaterial);
	void AddFloor(double LogicalW, double LogicalH, double TilePx);
	UTexture2D* GetFloorTexture() const { return FloorTexture; }
	// The backdrop's patches, puddles and rims: triangles in field px, sRGB vertex colours, drawn translucent in the order
	// given (one mesh section: the GPU keeps primitive order, so Godot's painter's order holds). Empty arrays clear it.
	void SetBackdropFill(const TArray<FVector2D>& Points, const TArray<FColor>& Colors, const TArray<int32>& Triangles,
		UMaterialInterface* FillMaterial);
	int32 BackdropFillTriangles() const { return FillTriangles; }
	// An effects band, this frame (an empty canvas clears it). The canvas's Z is the world Z of each vertex.
	void SetFx(EPawFxBand Band, const FPawFxCanvas& Canvas, UMaterialInterface* FxMaterial);
	int32 FxTriangles(EPawFxBand Band) const { return FxTris[static_cast<int32>(Band)]; }
	// How many times a band's mesh section was cleared or created (the tests): each one rebuilds the mesh's collision body and
	// its scene proxy (UProceduralMeshComponent: UpdateCollision + MarkRenderStateDirty).
	int32 FxSectionRebuilds() const { return FxRebuilds; }
	// The world Z of the Ground and Under bands (between the floor's props and the bodies).
	static constexpr double GroundFxZ = 15.0;
	static constexpr double UnderFxZ = 25.0;
	// The world Z over every sprite (Godot's z 45-52 effects). Not above 300: the camera's automatic ortho planes clip Z 400
	// (APawRealmView's constructor).
	static constexpr double TopFxZ = 300.0;
	// The mice's draw places (z 0) that get room in depth, MiceStep world units each (BodyZ): a mouse's place is 1 + its pool
	// slot, so this must stay over max_live_enemies + 1 (Pawpocalypse.Realm checks it against the data).
	static constexpr double MicePlaces = 400.0;
	static constexpr double MiceStep = 0.4;
	void EndFrame();

	// What the last EndFrame drew in a layer, and the pool behind it (tests, the debug HUD, the bench).
	int32 DrawnCount(EPawLayer Layer) const;
	int32 PoolSize(EPawLayer Layer) const;
	// Every sprite of the last frame, as added (the tests and the bench read what was drawn where). Not kept in Shipping
	// (review CR17: a copy of every sprite a frame, for the tests): empty there.
	const TArray<FPawSprite>& LastSprites(EPawLayer Layer) const;
	// The Z a sprite of this order is drawn at (tests).
	static double BodyZ(double Order);

	// Field pixels -> world, for the camera and the tests.
	FVector FieldToWorld(double X, double Y, double Z) const;

private:
	struct FBatch
	{
		TObjectPtr<UInstancedStaticMeshComponent> Comp;
		TArray<FTransform> Transforms;
		TArray<float> Custom;
		int32 Floats = 4;
		int32 Drawn = 0;
	};
	enum EKind : int32 { Masked, Translucent, Shadow, KindCount };

	UInstancedStaticMeshComponent* MakeComponent(const TCHAR* Name, UMaterialInterface* Material, int32 Floats, int32 SortPriority);
	static void Flush(FBatch& B);
	FBatch& ShapeBatch(EPawLayer Layer);
	double ShapeZ(EPawLayer Layer) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Components;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PageMaterials;
	UPROPERTY(Transient)
	TObjectPtr<const UPawSpriteAtlas> Atlas;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> QuadMesh;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FloorTexture;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FloorMaterialInstance;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> FlatFloorMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Fill;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> FxMeshes;
	int32 FxTris[static_cast<int32>(EPawFxBand::Count)] = {};
	int32 FxRebuilds = 0;
	bool FxFull[static_cast<int32>(EPawFxBand::Count)] = {}; // a band's mesh holds a section
	int32 FillTriangles = 0;
	double FillViewW = 0.0, FillViewH = 0.0;

	FBatch Shapes[3];                    // Floor, Zones, Fx
	TArray<FBatch> Pages;                // page * KindCount + kind
	TArray<FPawSprite> Sprites[static_cast<int32>(EPawLayer::Count)];
	int32 SpriteCount[static_cast<int32>(EPawLayer::Count)] = {};
	int32 Drawn[static_cast<int32>(EPawLayer::Count)] = {};
	double ViewW = 1920.0;
	double ViewH = 1080.0;
	bool bSetUp = false;
};
