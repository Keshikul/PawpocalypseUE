#pragma once

// SPIKE S1 (docs/04 §8): throwaway. Delete with the rest of Dev/Spikes once the results are in docs/04 §10.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SpikeLookActor.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

// Draws the S1 test scene from field coordinates (1920x1080, y down) through an orthographic camera: a palette swatch
// row (per-instance colour), a static grid of the 40 rasterised sprites (edges), and 200 moving sprites (ghosting).
// After a warm-up it samples frame/GPU times, reads the back buffer, compares every swatch with its palette value,
// writes Saved/Spikes/S1/<run>.png + .txt, and quits.
//
// Command line: -S1Run=<name> -S1AA=<r.AntiAliasingMethod> -S1A2C (alpha-to-coverage material) -S1EV=<exposure bias>
//               -S1ToneCurve=<amount> (negative test: 1 should fail the colour check)
UCLASS()
class ASpikeLookActor : public AActor
{
	GENERATED_BODY()

public:
	ASpikeLookActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	struct FS1Mover
	{
		int32 Comp = 0;
		int32 Index = 0;
		FVector2D Pos;
		FVector2D Vel;
		double Size = 0.0;
	};

	struct FS1Swatch
	{
		FString Name;
		FColor Expected;
		FVector2D Center;
	};

	void SetupViewAndPostProcess();
	void BuildSwatches(UMaterialInterface* SwatchMaterial);
	void BuildSprites(UMaterialInterface* SpriteMaterial);
	void RequestReadBack();
	// FViewport::ReadPixels has no back buffer under -RenderOffscreen (D3D12 ensure, black pixels); the screenshot
	// delegate delivers the final frame in both modes.
	void OnScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors);
	void CaptureAndAnalyze();
	void Analyze(TArray<FColor> Pixels, FIntPoint Size, const FString& Label);
	FDelegateHandle ScreenshotHandle;

	UPROPERTY()
	TObjectPtr<class USceneCaptureComponent2D> Capture;

	FTransform FieldTransform(const FVector2D& Center, double Size, double Z) const;
	double CreatureZ(double FieldY) const;

	UPROPERTY()
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> SpriteComps;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> SwatchComp;

	UPROPERTY()
	TObjectPtr<AActor> SpikeCamera;

	TArray<FS1Mover> Movers;
	TArray<FS1Swatch> Swatches;

	FString RunName = TEXT("default");
	int32 FrameCount = 0;
	int32 Samples = 0;
	double FrameMsSum = 0.0;
	double GpuMsSum = 0.0;
	double FrameMsMax = 0.0;
	bool bReported = false;
};
