#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "PawRealmView.generated.h"

class UCameraComponent;
class UMaterialInterface;
class UPawRealmSubsystem;
class UPawSpriteRenderer;
class UStaticMesh;

namespace Pc
{
	struct PcFieldEvent;
}

// The realm on screen (docs/03 §3, docs/04 §5-6): the orthographic camera, the field drawn by UPawSpriteRenderer, the
// effects the field's events ask for, and the input the field reads (the cat at the cursor, Pounce held). One per
// L_Realm, placed by tools/create_maps.py with its mesh and materials set on the instance, so the cook follows them.
//
// Each frame (its own tick): sample the input, advance the realm (UPawRealmSubsystem), turn the frame's events into
// effects, draw everything interpolated between the last two steps.
//
// The look is a placeholder until phase 6 (atlas art, visual definitions): every body is a disc in its data's tint at its
// data's sprite size, golden and signature mice in their own colours.
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

	// Off for the tests, which drive the realm themselves.
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	bool bStartRealmOnBeginPlay = true;
	// Off for the tests: the cursor and the pad are not sampled.
	UPROPERTY(EditAnywhere, Category = "Pawpocalypse")
	bool bSampleInput = true;

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
	int32 LiveEffects() const { return Effects.Num(); }
	int64 EffectsSpawned() const { return Spawned; }
	double LastDrawMs() const { return DrawMs; }

	// Logical size from a viewport size in pixels; field pixels from a viewport position (orthographic: a scale).
	static double LogicalWidthFor(double ViewportW, double ViewportH);
	static FVector2D FieldFromViewport(const FVector2D& Pixel, double ViewportH);

private:
	struct FFx
	{
		double X = 0.0, Y = 0.0, From = 0.0, To = 0.0, Age = 0.0, Life = 0.3;
		FLinearColor Color;
	};

	UPawRealmSubsystem* Realm() const;
	void UpdateViewSize();
	void SampleInput();
	void SpawnEffects(const Pc::PcFieldEvent& E);
	void Burst(double X, double Y, double From, double To, double Life, const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "Pawpocalypse")
	TObjectPtr<UPawSpriteRenderer> Renderer;

	TArray<FFx> Effects;
	double LogicalWidth = 1920.0;
	double PoopRadius = 0.0;
	double DrawMs = 0.0;
	int64 Spawned = 0;
	FIntPoint LastViewport = FIntPoint::ZeroValue;
};
