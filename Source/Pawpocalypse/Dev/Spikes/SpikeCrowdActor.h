#pragma once

// SPIKE S2 (docs/04 §8): throwaway. Delete with the rest of Dev/Spikes once the results are in docs/04 §10.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SpikeCrowdActor.generated.h"

class UInstancedStaticMeshComponent;
class UPaperSpriteComponent;
class UPaperSprite;
class UTexture2D;
class SSpikeCrowdCanvas;

// One item the renderers draw this frame, in field pixels (1920x1080, y down).
struct FS2DrawItem
{
	enum EKind : uint8 { Zone, Shadow, Creature, Effect };
	EKind Kind = Creature;
	FVector2D Pos;
	double Size = 0.0;
	int32 Cell = 0;
	FLinearColor Color = FLinearColor::White;
};

// The S2 crowd: 320 enemies + their shadows + 30 kittens + 64 zones + 40 effects driven by a fake fixed-step sim at
// 60 Hz, drawn by one of three renderers so their costs can be compared on the same load.
//
// Command line: -S2Run=<name> -S2Renderer=ism|paper|slate  (+ engine flags: -graphicsadapter=1 for the integrated GPU)
UCLASS()
class ASpikeCrowdActor : public AActor
{
	GENERATED_BODY()

public:
	ASpikeCrowdActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	const TArray<FS2DrawItem>& GetDrawItems() const { return Items; }

private:
	void SetupView();
	void InitSim();
	void StepSim(double Dt);
	void BuildDrawItems(double Alpha);

	void SetupIsm();
	void UpdateIsm();
	void SetupPaper();
	void UpdatePaper();
	void SetupSlate();

	void Sample(float DeltaSeconds, double UpdateMs);
	void Report();

	FTransform FieldTransform(const FVector2D& Center, double Size, double Z, double PlaneSize) const;

	enum class ERenderer : uint8 { Ism, Paper, Slate };
	ERenderer Renderer = ERenderer::Ism;
	FString RunName = TEXT("default");

	// Sim state (struct of arrays, as PcField will be).
	TArray<FVector2D> EnemyPos, EnemyPrev, EnemyVel;
	TArray<int32> EnemyCell;
	TArray<double> EnemySize;
	TArray<FVector2D> KittenPos, KittenPrev;
	TArray<int32> KittenCell;
	TArray<FVector2D> ZonePos;
	TArray<double> ZoneSize;
	TArray<FLinearColor> ZoneColor;
	TArray<FVector2D> EffectPos;
	TArray<double> EffectAge, EffectLife;
	double Accumulator = 0.0;
	FRandomStream Rng;

	TArray<FS2DrawItem> Items;

	// ISM renderer
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> IsmCreatures;
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> IsmShadows;
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> IsmZones;
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> IsmEffects;
	TArray<FTransform> ScratchTransforms;

	// Paper2D renderer: one component per drawn item, in Items order.
	UPROPERTY()
	TArray<TObjectPtr<UPaperSpriteComponent>> PaperComps;

	// Slate renderer
	TSharedPtr<SSpikeCrowdCanvas> Canvas;
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> SlateTextures;

	UPROPERTY()
	TObjectPtr<AActor> SpikeCamera;

	// Measurement
	int32 FrameCount = 0;
	TArray<double> FrameMs, GameMs, RenderMs, GpuMs, DrawCalls, Prims, UpdateMs;
	bool bReported = false;
	int32 QuitCountdown = 0;
	double WarmupElapsed = 0.0;
};
