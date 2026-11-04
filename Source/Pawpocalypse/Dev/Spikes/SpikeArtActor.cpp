#include "Dev/Spikes/SpikeArtActor.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Core/PawLog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "TextureResource.h"

namespace SpikeS3
{
	const TCHAR* Root = TEXT("/Game/Pawpocalypse/Dev/Spikes/S3");
	constexpr double FieldW = 1920.0;
	constexpr double FieldH = 1080.0;
	constexpr double PageSize = 2048.0;
	constexpr int32 SettleFrames = 90;
}

ASpikeArtActor::ASpikeArtActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASpikeArtActor::BeginPlay()
{
	Super::BeginPlay();

	FString Variant = TEXT("r2x"), Comp = TEXT("bc7");
	int32 Res = 1080;
	FParse::Value(FCommandLine::Get(), TEXT("-S3Variant="), Variant);
	FParse::Value(FCommandLine::Get(), TEXT("-S3Comp="), Comp);
	FParse::Value(FCommandLine::Get(), TEXT("-S3Res="), Res);
	RunName = FString::Printf(TEXT("%s_%s_%d"), *Variant, *Comp, Res);

	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *(FPaths::ProjectDir() / TEXT("SourceArt/png/spike_s3/manifest.csv")));
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/M_S3_Atlas.M_S3_Atlas"), SpikeS3::Root));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	TMap<int32, UInstancedStaticMeshComponent*> Pages;
	int32 Count = 0;
	for (int32 L = 1; L < Lines.Num(); ++L)
	{
		TArray<FString> C;
		Lines[L].ParseIntoArray(C, TEXT(","));
		if (C.Num() < 9 || C[0] != Variant)
		{
			continue;
		}
		const int32 Page = FCString::Atoi(*C[2]);
		UInstancedStaticMeshComponent*& Ism = Pages.FindOrAdd(Page);
		if (!Ism)
		{
			const FString TexName = FString::Printf(TEXT("T_S3_%s_p%d_%s"), *Variant, Page, *Comp);
			UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), SpikeS3::Root, *TexName, *TexName));
			if (!Tex || !Mat)
			{
				UE_LOG(LogPaw, Error, TEXT("S3: missing %s or the material"), *TexName);
				return;
			}
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, this);
			MID->SetTextureParameterValue(TEXT("Tex"), Tex);
			Ism = NewObject<UInstancedStaticMeshComponent>(this);
			Ism->SetStaticMesh(Plane);
			Ism->SetMaterial(0, MID);
			Ism->NumCustomDataFloats = 4;
			Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Ism->SetupAttachment(RootComponent);
			Ism->RegisterComponent();
		}
		const double X = FCString::Atod(*C[3]), Y = FCString::Atod(*C[4]), S = FCString::Atod(*C[5]);
		const double SX = FCString::Atod(*C[6]), SY = FCString::Atod(*C[7]), SS = FCString::Atod(*C[8]);
		const FVector2D Center(SX + SS * 0.5, SY + SS * 0.5);
		const FVector Location(SpikeS3::FieldH * 0.5 - Center.Y, Center.X - SpikeS3::FieldW * 0.5, 10.0);
		const int32 I = Ism->AddInstance(FTransform(FRotator(0.0, 90.0, 0.0), Location, FVector(SS / 100.0, SS / 100.0, 1.0)));
		// UV rect of the sprite inside its page (without the extruded border), as in the planned atlas material.
		Ism->SetCustomData(I, { static_cast<float>(X / SpikeS3::PageSize), static_cast<float>(Y / SpikeS3::PageSize),
			static_cast<float>(S / SpikeS3::PageSize), static_cast<float>(S / SpikeS3::PageSize) }, true);
		++Count;
	}

	// The S1 view and post-process: neutral, exposure 0 EV.
	APostProcessVolume* PPV = GetWorld()->SpawnActor<APostProcessVolume>();
	PPV->bUnbound = true;
	FPostProcessSettings& P = PPV->Settings;
	P.bOverride_AutoExposureMethod = true;
	P.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	P.bOverride_AutoExposureBias = true;
	P.AutoExposureBias = 0.0f;
	P.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	P.AutoExposureApplyPhysicalCameraExposure = 0;
	P.bOverride_ToneCurveAmount = true;
	P.ToneCurveAmount = 0.0f;
	P.bOverride_BlueCorrection = true;
	P.BlueCorrection = 0.0f;
	P.bOverride_ExpandGamut = true;
	P.ExpandGamut = 0.0f;
	P.bOverride_VignetteIntensity = true;
	P.VignetteIntensity = 0.0f;

	// Capture at the run's resolution: the field is 1920x1080 units either way, so 2160 draws every sprite at 2x its pixels.
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->RenderTargetFormat = RTF_RGBA8;
	RT->bForceLinearGamma = false; // encode once (spike S1)
	RT->InitAutoFormat(static_cast<uint32>(SpikeS3::FieldW * Res / 1080), static_cast<uint32>(Res));
	RT->UpdateResourceImmediate(true);
	Capture = NewObject<USceneCaptureComponent2D>(this);
	Capture->SetupAttachment(RootComponent);
	Capture->SetRelativeLocationAndRotation(FVector(0.0, 0.0, 1000.0), FRotator(-90.0, 0.0, 0.0));
	Capture->ProjectionType = ECameraProjectionMode::Orthographic;
	Capture->OrthoWidth = SpikeS3::FieldW;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = true;
	Capture->TextureTarget = RT;
	Capture->RegisterComponent();

	// -S3Screen (windowed): also a real camera, so the back buffer can be read (the capture's AA differs from the screen's,
	// spike S1). OrthoWidth = field height under MaintainYFOV (docs/04 §6).
	if (FParse::Param(FCommandLine::Get(), TEXT("S3Screen")))
	{
		ACameraActor* Cam = GetWorld()->SpawnActor<ACameraActor>(FVector(0.0, 0.0, 1000.0), FRotator(-90.0, 0.0, 0.0));
		UCameraComponent* C = Cam->GetCameraComponent();
		C->SetProjectionMode(ECameraProjectionMode::Orthographic);
		C->bOverrideAspectRatioAxisConstraint = true;
		C->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV);
		C->SetOrthoWidth(SpikeS3::FieldH);
		C->SetConstraintAspectRatio(false);
		ScreenCamera = Cam;
	}

	UE_LOG(LogPaw, Display, TEXT("S3 run '%s': %d sprites on %d pages"), *RunName, Count, Pages.Num());
}

void ASpikeArtActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (ScreenCamera)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && PC->GetViewTarget() != ScreenCamera)
		{
			PC->SetViewTarget(ScreenCamera);
		}
	}
	if (bDone || !Capture || ++Frames < SpikeS3::SettleFrames)
	{
		return;
	}
	bDone = true;
	if (ScreenCamera)
	{
		// Back buffer: what the player sees, FXAA included. The capture is written below as before.
		ScreenHandle = UGameViewportClient::OnScreenshotCaptured().AddLambda([this](int32 W, int32 H, const TArray<FColor>& Colors)
		{
			UGameViewportClient::OnScreenshotCaptured().Remove(ScreenHandle);
			TArray<FColor> Px = Colors;
			for (FColor& P : Px)
			{
				P.A = 255;
			}
			FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Spikes/S3") / RunName + TEXT("_screen.png")), FImageView(Px.GetData(), W, H));
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->ConsoleCommand(TEXT("quit"));
			}
		});
		FScreenshotRequest::RequestScreenshot(false);
	}
	UTextureRenderTarget2D* RT = Capture->TextureTarget;
	TArray<FColor> Pixels;
	if (RT && RT->GameThread_GetRenderTargetResource()->ReadPixels(Pixels))
	{
		for (FColor& Px : Pixels)
		{
			Px.A = 255;
		}
		const FString Path = FPaths::ProjectSavedDir() / TEXT("Spikes/S3") / RunName + TEXT(".png");
		FImageUtils::SaveImageByExtension(*Path, FImageView(Pixels.GetData(), RT->SizeX, RT->SizeY));
		UE_LOG(LogPaw, Display, TEXT("S3 wrote %s"), *Path);
	}
	if (!ScreenCamera)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->ConsoleCommand(TEXT("quit"));
		}
	}
}
