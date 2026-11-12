#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PcBackdrop.h"
#include "PcFieldRules.h"
#include "Realm/PawVfx.h"
#include "Render/PawFxCanvas.h"

#include "PawRealmView.generated.h"

class UCameraComponent;
class UInputAction;
class APlayerController;
class UPawInputDeviceSubsystem;
class UMaterialInterface;
class UPawRealmSubsystem;
class UPawSpriteAtlas;
class UPawSpriteRenderer;
class UStaticMesh;
class UTexture2D;

namespace Pc
{
	struct PcFieldEvent;
	struct PcGameData;
	struct PcBoss;
	struct PcEvolution;
	struct PcJob;
}

// The realm on screen (docs/03 §3, docs/04 §5-6): the orthographic camera, the field drawn by UPawSpriteRenderer, the
// effects the field's events ask for, and the input the field reads (the cat at the cursor, Pounce held). One per
// L_Realm, placed by tools/create_maps.py with its mesh and materials set on the instance, so the cook follows them.
//
// Each frame (its own tick): sample the input, advance the realm (UPawRealmSubsystem), turn the frame's events into
// effects, draw everything interpolated between the last two steps.
//
// The look (phase 6 slice 1): every creature is its Godot drawing from the field atlas (DA_FieldAtlas), at Godot's size, tint,
// facing, breathing pulse and draw order (Realm/PawFieldLook.h): the mice with their shield and snare tints and the signature
// glow, the kittens (gold, illusory), the cat in its visual stage with its weapon and hat and the Pounce hop, the boss with
// its twitch, shield and death, its adds, and the ground shadow under everything that stands. Without an atlas (a map not
// yet configured) the field draws nothing but its floor, zones and effects. The effects are still placeholder rings
// (phase 6's VFX slice).
UCLASS()
class PAWPOCALYPSE_API APawRealmView : public AActor
{
	GENERATED_BODY()

public:
	APawRealmView();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UStaticMesh> QuadMesh;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> FlatMaterial;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> SoftMaterial;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> DiscMaterial;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> RingMaterial;
	// Phase 6: the atlas sprites (tools/create_field_materials.py, tools/import_art.py; set by tools/create_maps.py).
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> SpriteMaterial;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> SpriteSoftMaterial;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UPawSpriteAtlas> FieldAtlas;
	// Phase 6 slice 2: the biome backdrop (biome_backdrop.gd): the floor material and every biome's floor (T_backdrop_tier_N,
	// referenced here so they cook and load with the map, never on travel), the patches' and puddles' material.
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> FloorMaterial;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UMaterialInterface> BackdropFillMaterial;
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	TArray<TObjectPtr<UTexture2D>> FloorTextures;

	// Off for the tests, which drive the realm themselves.
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	bool bStartRealmOnBeginPlay = true;
	// Off for the tests: the cursor and the pad are not sampled.
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	bool bSampleInput = true;

	// tier_controller.gd _unhandled_input("pounce"): the action held, unless the press is the POINTER's over the dock (the
	// dock's own buttons take a click; Godot never sees it as a pounce). A trigger pounces wherever the cat is.
	static bool PounceHeld(bool bAction, bool bPointerInput, bool bOverDock) { return bAction && !(bPointerInput && bOverDock); }
	// tier_controller.gd field_hides_pointer(): the pointer is over the open field (the OS pointer hides there; the cat IS the
	// cursor) when it is inside the playfield and no GUI is under it (here: the dock, the one panel over the field).
	// player.gd _process, the stick path: the cat moves by velocity and is clamped to the playfield (Vector2.clamp: the far
	// edges are IN, unlike has_point).
	static FVector2D StickStep(const FVector2D& Cat, const FVector2f& Velocity, double Dt, const Pc::PcRect& Bounds)
	{
		return FVector2D(FMath::Clamp(Cat.X + Velocity.X * Dt, Bounds.X, Bounds.X + Bounds.W),
			FMath::Clamp(Cat.Y + Velocity.Y * Dt, Bounds.Y, Bounds.Y + Bounds.H));
	}
	static bool PointerOverField(const FVector2D& Pointer, const Pc::PcRect& Bounds, bool bOverDock)
	{
		return !bOverDock && Bounds.HasPoint(Pointer.X, Pointer.Y);
	}
	// What steers the cat this frame (player.gd _process): R3's recentre, else the stick while it has the cursor, else the
	// pointer. Nothing while the Cone of Shame roots the cat, and nothing while the field is paused: Godot's player stops with
	// the paused tree (review CR6: the stick kept steering here and warped the pointer under a card pick or a menu).
	enum class ESteer : uint8 { Nothing, Recentre, Stick, Pointer };
	static ESteer SteerBy(bool bPaused, bool bRooted, bool bRecentre, bool bStickSteering, bool bPointer)
	{
		return bPaused || bRooted ? ESteer::Nothing : bRecentre ? ESteer::Recentre : bStickSteering ? ESteer::Stick
			: bPointer ? ESteer::Pointer : ESteer::Nothing;
	}

	// The logical view (docs/04 §6): 1080 high, as wide as the window's aspect makes it.
	static constexpr double LogicalHeight = 1080.0;
	double GetLogicalWidth() const { return LogicalWidth; }

	// The renderer's layers and the view's size (BeginPlay; the tests call it on a view spawned into a world that is not
	// playing).
	void InitView();
	// One frame without the engine's tick (the tests): advance by RealDelta and draw.
	void RunFrame(double RealDelta);
	// Draw the realm as it is now (no advance).
	void Draw();

	UPawSpriteRenderer* GetRenderer() const { return Renderer; }
	int32 LiveEffects() const { return Vfx.Live(); }
	const FPawVfx& GetVfx() const { return Vfx; }
	FPawVfx& GetVfxForTest() { return Vfx; }
	bool MakeVfxContext(FPawVfx::FContext& C, double Dt) const;
	// The look's state, for the tests: the cat's hop left, the boss's death animation running, a mouse slot's facing.
	double CatHopLeft() const { return HopLeft; }
	bool IsBossDying() const { return BossDeath.Cell != INDEX_NONE; }
	bool IsMouseFlipped(int32 Slot) const { return Mice.IsValidIndex(Slot) && Mice[Slot].bFlip; }
	// The backdrop drawn last (its layout from PcBackdrop) and how many times it was laid out.
	const Pc::PcBackdropLayout& GetBackdrop() const { return Backdrop; }
	int32 BackdropBuilds() const { return BackdropBuildCount; }
	int64 EffectsSpawned() const { return Vfx.Spawned(); }
	double LastDrawMs() const { return DrawMs; }

	// Logical size from a viewport size in pixels; field pixels from a viewport position (orthographic: a scale).
	static double LogicalWidthFor(double ViewportW, double ViewportH);
	static FVector2D FieldFromViewport(const FVector2D& Pixel, double ViewportH);

private:

	UPawRealmSubsystem* Realm() const;
	void UpdateViewSize();
	void SampleInput();
	void SpawnEffects(const Pc::PcFieldEvent& E);

	UPROPERTY(VisibleAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UPawSpriteRenderer> Renderer;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> PounceAction; // IA_Pounce (the input map's), found at the first sample
	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> RecentreAction;
	bool bRecentreWasDown = false;
	// The cat to (X, Y) by the stick or a re-centre, the OS pointer warped onto it.
	void MoveCat(APlayerController* PC, UPawInputDeviceSubsystem* Device, double X, double Y);

	// Per mouse slot / kitten: what the look remembers between frames (the facing persists through the dead zone).
	struct FMover
	{
		uint64 Serial = 0;
		double LastX = 0.0;
		bool bSeen = false;
		bool bFlip = false;
	};
	struct FBossDeath
	{
		int32 Cell = INDEX_NONE;
		double X = 0.0, Y = 0.0, Size = 0.0, Age = 0.0;
		FVector2D Shake[6];
	};
	void TickLook(double RealDelta, bool bPaused);
	void DrawCreatures();
	// The floor, the patches and puddles, the props: laid out again when the biome or the view's size changes
	// (BiomeBackdrop._rebuild on size_changed), with the effects budget of that moment.
	void DrawBackdrop();
	UTexture2D* FloorTextureFor(const std::string& Stem) const;
	int32 CellFor(const FString& Path) const;
	void Face(FMover& M, uint64 Serial, double X, double Dt);

	Pc::PcBackdropLayout Backdrop;
	// What the backdrop was laid out for: the biome (its tier index; INDEX_NONE for none) and the view's size in thousandths
	// of a pixel (review CR17: a string made and compared every frame before).
	bool bBackdropLaid = false;
	int32 BackdropTier = INDEX_NONE;
	int64 BackdropW = 0, BackdropH = 0;
	TArray<int32> BackdropCells;    // per prop of the layout, its atlas cell
	int32 BackdropBuildCount = 0;

	FPawVfx Vfx;
	FPawFxCanvas FxGround, FxUnder, FxOver;
	TArray<FMover> Mice;
	TArray<FMover> Kittens;
	TArray<int32> EnemyCells;       // per PcGameData::Enemies index, built with the atlas
	// The other drawings the field names, looked up once and kept while the atlas, the data and what they name stay (review
	// CR17: about seven lookups by name a frame, one of them through Printf).
	struct FCells
	{
		const UPawSpriteAtlas* Atlas = nullptr;
		const Pc::PcGameData* Data = nullptr;
		int32 Shadow = INDEX_NONE, Kitten = INDEX_NONE, Add = INDEX_NONE;
		const Pc::PcBoss* Boss = nullptr;
		int32 BossCell = INDEX_NONE;
		int64 Stage = -1;
		const Pc::PcEvolution* Evo = nullptr;
		int32 Cat = INDEX_NONE, Weapon = INDEX_NONE;
		const Pc::PcJob* Job = nullptr;
		int32 Hat = INDEX_NONE;
	};
	FCells Cells;
	TMap<uint64, double> AddBorn;   // an add's serial -> the look clock when it first appeared (its pulse's age)
	TSet<uint64> AddsNow;           // this frame's adds (kept: its storage is reused)
	TMap<FKey, float> SteerAxes;    // the steer stick's axis values this frame (kept: its storage is reused, review CR17)
	FBossDeath BossDeath;
	FRandomStream LookRandom{ 0x5eed };
	double LookClock = 0.0;         // seconds of unpaused play: the pulse, the glow (a paused Godot tree stops its _process)
	double LookDt = 0.0;            // the last frame's, for the facing
	double HopLeft = 0.0;
	bool bHopNow = false;           // the hop drew this frame (player.gd _apply_hop ran with time left)
	FVector2D DeathStart = FVector2D::ZeroVector;
	bool bCatFlip = false;
	double StickVelX = 0.0;         // the steer stick's velocity.x (player.gd faces by it, never by the mouse)
	double Anger = 0.0, TwitchTimer = 0.0;
	FVector2D Twitch = FVector2D::ZeroVector;
	double LogicalWidth = 1920.0;
	double DrawMs = 0.0;
	FIntPoint LastViewport = FIntPoint::ZeroValue;
};
