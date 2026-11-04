#pragma once

// SPIKE S3 (docs/04 §8): throwaway. Delete with the rest of Dev/Spikes once the results are in docs/04 §10.

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "SpikeArtActor.generated.h"

class USceneCaptureComponent2D;

// Draws every field sprite from an S3 atlas variant at its real on-screen size (layout from
// SourceArt/png/spike_s3/manifest.csv, the same layout as the reference images), captures the frame colour-exactly (the
// scene-capture route proven in S1) and writes Saved/Spikes/S3/<variant>_<comp>_<res>.png, then quits.
//
// Command line: -S3Variant=r1x|r2x|godot -S3Comp=bc7|ui -S3Res=1080|2160
UCLASS()
class ASpikeArtActor : public AActor
{
	GENERATED_BODY()

public:
	ASpikeArtActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> Capture;

	UPROPERTY()
	TObjectPtr<AActor> ScreenCamera;
	FDelegateHandle ScreenHandle;

	FString RunName;
	int32 Frames = 0;
	bool bDone = false;
};
