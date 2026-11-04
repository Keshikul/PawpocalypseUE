#include "Dev/Spikes/SpikeLookActor.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/PawLog.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace SpikeS1
{
	constexpr double FieldW = 1920.0;
	constexpr double FieldH = 1080.0;
	constexpr double CameraZ = 1000.0;
	constexpr double SwatchZ = 10.0;
	constexpr double CreatureBaseZ = 100.0;
	constexpr double CreatureSpan = 100.0;
	constexpr int32 NumMovers = 200;
	constexpr int32 WarmupFrames = 120;
	constexpr int32 SampleFrames = 300;
	const TCHAR* Root = TEXT("/Game/Pawpocalypse/Dev/Spikes/S1");
	// The engine plane is 100x100 units in XY, facing +Z.
	constexpr double PlaneSize = 100.0;

	FString InputDir()
	{
		return FPaths::ProjectDir() / TEXT("SourceArt/png/spike_s1");
	}
}

ASpikeLookActor::ASpikeLookActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASpikeLookActor::BeginPlay()
{
	Super::BeginPlay();

	FParse::Value(FCommandLine::Get(), TEXT("-S1Run="), RunName);
	int32 AAMethod = -1;
	if (FParse::Value(FCommandLine::Get(), TEXT("-S1AA="), AAMethod))
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
		{
			CVar->Set(AAMethod, ECVF_SetByCode);
		}
	}
	const bool bA2C = FParse::Param(FCommandLine::Get(), TEXT("S1A2C"));

	const FString SpriteMatPath = FString::Printf(TEXT("%s/%s"), SpikeS1::Root, bA2C ? TEXT("M_S1_Sprite_A2C.M_S1_Sprite_A2C") : TEXT("M_S1_Sprite.M_S1_Sprite"));
	UMaterialInterface* SpriteMat = LoadObject<UMaterialInterface>(nullptr, *SpriteMatPath);
	UMaterialInterface* SwatchMat = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/M_S1_Swatch.M_S1_Swatch"), SpikeS1::Root));
	if (!SpriteMat || !SwatchMat)
	{
		UE_LOG(LogPaw, Error, TEXT("S1: spike materials missing under %s (run tools: spike_s1_setup.py)"), SpikeS1::Root);
		return;
	}

	SetupViewAndPostProcess();
	BuildSwatches(SwatchMat);
	BuildSprites(SpriteMat);

	const IConsoleVariable* AA = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
	const IConsoleVariable* Fwd = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ForwardShading"));
	UE_LOG(LogPaw, Display, TEXT("S1 run '%s': r.AntiAliasingMethod=%d r.ForwardShading=%d A2C=%d swatches=%d sprites=%d movers=%d"),
		*RunName, AA ? AA->GetInt() : -1, Fwd ? Fwd->GetInt() : -1, bA2C ? 1 : 0, Swatches.Num(), SpriteComps.Num(), Movers.Num());
}

void ASpikeLookActor::SetupViewAndPostProcess()
{
	UWorld* World = GetWorld();

	// Looking straight down -Z: screen up = world +X, screen right = world +Y (FieldTransform maps field px onto that).
	ACameraActor* Camera = World->SpawnActor<ACameraActor>(FVector(0.0, 0.0, SpikeS1::CameraZ), FRotator(-90.0, 0.0, 0.0));
	UCameraComponent* Cam = Camera->GetCameraComponent();
	Cam->SetProjectionMode(ECameraProjectionMode::Orthographic);
	// The local player's default AspectRatioAxisConstraint is MaintainYFOV (BaseEngine.ini): OrthoWidth then sets the
	// visible HEIGHT and the width follows the window's aspect. Measured: OrthoWidth 1920 showed the field at 0.57x.
	// Height fixed at 1080 with the width expanding is exactly Godot's expand aspect (docs/04 §6), so say so explicitly.
	Cam->bOverrideAspectRatioAxisConstraint = true;
	Cam->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV);
	Cam->SetOrthoWidth(SpikeS1::FieldH);
	Cam->SetConstraintAspectRatio(false);
	const bool bManualPlanes = FParse::Param(FCommandLine::Get(), TEXT("S1ManualPlanes"));
	Cam->SetAutoCalculateOrthoPlanes(!bManualPlanes);
	if (bManualPlanes)
	{
		Cam->SetOrthoNearClipPlane(0.0f);
		Cam->SetOrthoFarClipPlane(2.0f * SpikeS1::CameraZ);
	}
	SpikeCamera = Camera;

	// Offscreen runs have no back buffer, so they read a scene capture of the same view (valid for colour, but it does
	// not show the screen's AA). Windowed runs read the back buffer and skip the capture, which would also add GPU time.
	const bool bOffscreen = FParse::Param(FCommandLine::Get(), TEXT("RenderOffscreen"));
	if (bOffscreen)
	{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	// Encode once, like the back buffer: the tonemapper writes sRGB when the target's display gamma is 2.2 (= sRGB), and
	// a non-sRGB RGBA8 target stores those bytes as they are. Measured both wrong ways: InitAutoFormat leaves
	// bForceLinearGamma=true (display gamma 1.0, linear bytes, every swatch = expected^2.2); an sRGB target with gamma 2.2
	// encodes twice (every swatch = expected^(1/2.2)).
	RT->RenderTargetFormat = RTF_RGBA8;
	RT->bForceLinearGamma = false;
	RT->InitAutoFormat(static_cast<uint32>(SpikeS1::FieldW), static_cast<uint32>(SpikeS1::FieldH));
	RT->UpdateResourceImmediate(true);
	Capture = NewObject<USceneCaptureComponent2D>(Camera);
	Capture->SetupAttachment(Cam);
	Capture->ProjectionType = ECameraProjectionMode::Orthographic;
	Capture->OrthoWidth = SpikeS1::FieldW;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = true;
	Capture->bAlwaysPersistRenderingState = true;
	Capture->TextureTarget = RT;
	Capture->RegisterComponent();
	}

	// docs/04 §3: exposure fixed, tonemapper neutralised (not removed), every other post effect off.
	float EV = 0.0f;
	FParse::Value(FCommandLine::Get(), TEXT("-S1EV="), EV);
	float ToneCurve = 0.0f;
	FParse::Value(FCommandLine::Get(), TEXT("-S1ToneCurve="), ToneCurve);

	APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>();
	PPV->bUnbound = true;
	FPostProcessSettings& S = PPV->Settings;
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = EV;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = 0;
	S.bOverride_ToneCurveAmount = true;
	S.ToneCurveAmount = ToneCurve;
	S.bOverride_BlueCorrection = true;
	S.BlueCorrection = 0.0f;
	S.bOverride_ExpandGamut = true;
	S.ExpandGamut = 0.0f;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.0f;
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.0f;
	S.bOverride_FilmGrainIntensity = true;
	S.FilmGrainIntensity = 0.0f;
	S.bOverride_SceneFringeIntensity = true;
	S.SceneFringeIntensity = 0.0f;
	S.bOverride_MotionBlurAmount = true;
	S.MotionBlurAmount = 0.0f;
	S.bOverride_AmbientOcclusionIntensity = true;
	S.AmbientOcclusionIntensity = 0.0f;
	S.bOverride_LensFlareIntensity = true;
	S.LensFlareIntensity = 0.0f;
	S.bOverride_LocalExposureHighlightContrastScale = true;
	S.LocalExposureHighlightContrastScale = 1.0f;
	S.bOverride_LocalExposureShadowContrastScale = true;
	S.LocalExposureShadowContrastScale = 1.0f;
}

FTransform ASpikeLookActor::FieldTransform(const FVector2D& Center, double Size, double Z) const
{
	// Field (x right, y down, origin top-left) -> world (screen up = +X, screen right = +Y).
	const FVector Location(SpikeS1::FieldH * 0.5 - Center.Y, Center.X - SpikeS1::FieldW * 0.5, Z);
	const double Scale = Size / SpikeS1::PlaneSize;
	// The engine plane's UVs put texture-up along world +Y (screen right): unrotated, every sprite showed turned 90 deg
	// counter-clockwise. Yaw +90 turns screen-up toward screen-right (clockwise on screen) and undoes it.
	return FTransform(FRotator(0.0, 90.0, 0.0), Location, FVector(Scale, Scale, 1.0));
}

double ASpikeLookActor::CreatureZ(double FieldY) const
{
	// docs/04 §4: lower on screen = closer to the camera, so depth sorts every pixel, even inside one instanced component.
	return SpikeS1::CreatureBaseZ + (FieldY / SpikeS1::FieldH) * SpikeS1::CreatureSpan;
}

void ASpikeLookActor::BuildSwatches(UMaterialInterface* SwatchMaterial)
{
	TArray<FString> Lines;
	FFileHelper::LoadFileToStringArray(Lines, *(SpikeS1::InputDir() / TEXT("palette.csv")));

	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	SwatchComp = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Swatches"));
	SwatchComp->SetStaticMesh(Plane);
	SwatchComp->SetMaterial(0, SwatchMaterial);
	SwatchComp->NumCustomDataFloats = 3;
	SwatchComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SwatchComp->SetupAttachment(RootComponent);
	SwatchComp->RegisterComponent();

	constexpr double Size = 56.0;
	constexpr double Gap = 8.0;
	constexpr int32 PerRow = 28;
	for (const FString& Line : Lines)
	{
		FString Name, Hex;
		if (!Line.Split(TEXT(","), &Name, &Hex))
		{
			continue;
		}
		const int32 I = Swatches.Num();
		const FVector2D Center(140.0 + (I % PerRow) * (Size + Gap) + Size * 0.5, 40.0 + (I / PerRow) * (Size + Gap) + Size * 0.5);
		const FColor Expected = FColor::FromHex(Hex);
		const FLinearColor Linear = FLinearColor::FromSRGBColor(Expected);
		const int32 Index = SwatchComp->AddInstance(FieldTransform(Center, Size, SpikeS1::SwatchZ));
		SwatchComp->SetCustomDataValue(Index, 0, Linear.R);
		SwatchComp->SetCustomDataValue(Index, 1, Linear.G);
		SwatchComp->SetCustomDataValue(Index, 2, Linear.B);
		Swatches.Add({ Name, Expected, Center });
	}
	SwatchComp->MarkRenderStateDirty();
}

void ASpikeLookActor::BuildSprites(UMaterialInterface* SpriteMaterial)
{
	TArray<FString> Names;
	FFileHelper::LoadFileToStringArray(Names, *(SpikeS1::InputDir() / TEXT("textures.txt")));
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	FRandomStream Rng(1337);

	TArray<double> Sizes;
	for (const FString& Name : Names)
	{
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("%s/Tex/%s.%s"), SpikeS1::Root, *Name, *Name));
		if (!Tex)
		{
			UE_LOG(LogPaw, Warning, TEXT("S1: texture %s missing"), *Name);
			continue;
		}
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(SpriteMaterial, this);
		MID->SetTextureParameterValue(TEXT("Tex"), Tex);

		UInstancedStaticMeshComponent* Comp = NewObject<UInstancedStaticMeshComponent>(this);
		Comp->SetStaticMesh(Plane);
		Comp->SetMaterial(0, MID);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetupAttachment(RootComponent);
		Comp->RegisterComponent();
		SpriteComps.Add(Comp);
		// Bosses are rasterised at 512 (256 logical), creatures at 256 (128 logical): draw at the logical size.
		Sizes.Add(Tex->GetSizeX() >= 512 ? 256.0 : 128.0);
	}

	// Static grid along the bottom: edge quality at rest, one of each sprite.
	const int32 N = SpriteComps.Num();
	for (int32 I = 0; I < N; ++I)
	{
		const double Size = Sizes[I] * 0.75;
		const FVector2D Center(80.0 + (I % 20) * 92.0, 830.0 + (I / 20) * 130.0);
		SpriteComps[I]->AddInstance(FieldTransform(Center, Size, CreatureZ(Center.Y)));
	}

	// Movers in the middle band at flier-like speeds, a fifth of them fast so ghosting has something to show.
	for (int32 M = 0; M < SpikeS1::NumMovers && N > 0; ++M)
	{
		FS1Mover Mover;
		Mover.Comp = M % N;
		Mover.Size = Sizes[Mover.Comp] * 0.6;
		Mover.Pos = FVector2D(Rng.FRandRange(60.0, SpikeS1::FieldW - 60.0), Rng.FRandRange(220.0, 700.0));
		const double Speed = (M % 5 == 0) ? 900.0 : Rng.FRandRange(120.0, 320.0);
		const double Angle = Rng.FRandRange(0.0, 2.0 * UE_DOUBLE_PI);
		Mover.Vel = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Speed;
		Mover.Index = SpriteComps[Mover.Comp]->AddInstance(FieldTransform(Mover.Pos, Mover.Size, CreatureZ(Mover.Pos.Y)));
		Movers.Add(Mover);
	}
}

void ASpikeLookActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bReported || SpriteComps.Num() == 0)
	{
		return;
	}

	// With no pawn the player controller may still be choosing its view target after our BeginPlay: insist every tick.
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && SpikeCamera && PC->GetViewTarget() != SpikeCamera)
	{
		UE_LOG(LogPaw, Display, TEXT("S1 frame %d: view target was %s, set to the spike camera"), FrameCount, *GetNameSafe(PC->GetViewTarget()));
		PC->SetViewTarget(SpikeCamera);
	}

	for (FS1Mover& M : Movers)
	{
		M.Pos += M.Vel * DeltaSeconds;
		if (M.Pos.X < 40.0 || M.Pos.X > SpikeS1::FieldW - 40.0) { M.Vel.X = -M.Vel.X; }
		if (M.Pos.Y < 200.0 || M.Pos.Y > 720.0) { M.Vel.Y = -M.Vel.Y; }
		M.Pos.X = FMath::Clamp(M.Pos.X, 40.0, SpikeS1::FieldW - 40.0);
		M.Pos.Y = FMath::Clamp(M.Pos.Y, 200.0, 720.0);
		SpriteComps[M.Comp]->UpdateInstanceTransform(M.Index, FieldTransform(M.Pos, M.Size, CreatureZ(M.Pos.Y)), false, false, false);
	}
	for (UInstancedStaticMeshComponent* Comp : SpriteComps)
	{
		Comp->MarkRenderStateDirty();
	}

	++FrameCount;
	if (FrameCount > SpikeS1::WarmupFrames)
	{
		const double FrameMs = DeltaSeconds * 1000.0;
		FrameMsSum += FrameMs;
		FrameMsMax = FMath::Max(FrameMsMax, FrameMs);
		GpuMsSum += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
		++Samples;
	}
	if (Samples >= SpikeS1::SampleFrames)
	{
		RequestReadBack();
	}
}

void ASpikeLookActor::CaptureAndAnalyze()
{
	UTextureRenderTarget2D* RT = Capture ? Capture->TextureTarget.Get() : nullptr;
	FTextureRenderTargetResource* Res = RT ? RT->GameThread_GetRenderTargetResource() : nullptr;
	TArray<FColor> Pixels;
	if (!Res || !Res->ReadPixels(Pixels))
	{
		UE_LOG(LogPaw, Error, TEXT("S1: scene capture readback failed"));
		return;
	}
	Analyze(MoveTemp(Pixels), FIntPoint(RT->SizeX, RT->SizeY), TEXT("capture"));
}

void ASpikeLookActor::RequestReadBack()
{
	bReported = true;
	if (Capture)
	{
		CaptureAndAnalyze();
	}
	ScreenshotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &ASpikeLookActor::OnScreenshot);
	if (Capture)
	{
		// Offscreen: a plain screenshot reads the missing back buffer; HighResShot at least completes (black) and quits.
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->ConsoleCommand(TEXT("HighResShot 1"));
		}
	}
	else
	{
		// Windowed: the back buffer exactly as the player sees it, temporal AA history included.
		FScreenshotRequest::RequestScreenshot(false);
	}
}

void ASpikeLookActor::OnScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors)
{
	UGameViewportClient::OnScreenshotCaptured().Remove(ScreenshotHandle);
	Analyze(Colors, FIntPoint(Width, Height), TEXT("screen"));

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->ConsoleCommand(TEXT("quit"));
	}
}

void ASpikeLookActor::Analyze(TArray<FColor> Pixels, FIntPoint Size, const FString& Label)
{
	for (FColor& P : Pixels)
	{
		P.A = 255;
	}

	// Each swatch: mean of the central 9x9 pixels (field px -> viewport px by the viewport/field ratio).
	// Field height = viewport height at any aspect; the width is centred (16:10 shows 1728 of the 1920 field px).
	const double Scale = Size.Y / SpikeS1::FieldH;
	TArray<FString> Report;
	Report.Add(FString::Printf(TEXT("run=%s viewport=%dx%d frames=%d frame_ms_avg=%.3f frame_ms_max=%.3f gpu_ms_avg=%.3f"),
		*RunName, Size.X, Size.Y, Samples, FrameMsSum / Samples, FrameMsMax, GpuMsSum / Samples));
	int32 WorstDiff = 0;
	int32 Failures = 0;
	for (const FS1Swatch& Sw : Swatches)
	{
		const int32 CX = FMath::RoundToInt32(Size.X * 0.5 + (Sw.Center.X - SpikeS1::FieldW * 0.5) * Scale);
		const int32 CY = FMath::RoundToInt32(Sw.Center.Y * Scale);
		int64 R = 0, G = 0, B = 0, Count = 0;
		for (int32 Y = CY - 4; Y <= CY + 4; ++Y)
		{
			for (int32 X = CX - 4; X <= CX + 4; ++X)
			{
				const FColor& P = Pixels[Y * Size.X + X];
				R += P.R; G += P.G; B += P.B; ++Count;
			}
		}
		const FColor Got(static_cast<uint8>(R / Count), static_cast<uint8>(G / Count), static_cast<uint8>(B / Count));
		const int32 Diff = FMath::Max3(FMath::Abs(Got.R - Sw.Expected.R), FMath::Abs(Got.G - Sw.Expected.G), FMath::Abs(Got.B - Sw.Expected.B));
		WorstDiff = FMath::Max(WorstDiff, Diff);
		Failures += Diff > 2 ? 1 : 0;
		Report.Add(FString::Printf(TEXT("swatch %-22s expected #%02X%02X%02X got #%02X%02X%02X diff %d%s"), *Sw.Name,
			Sw.Expected.R, Sw.Expected.G, Sw.Expected.B, Got.R, Got.G, Got.B, Diff, Diff > 2 ? TEXT("  FAIL") : TEXT("")));
	}
	Report.Insert(FString::Printf(TEXT("colour: %d swatches, worst diff %d, %d over +/-2 -> %s"), Swatches.Num(), WorstDiff, Failures,
		Failures == 0 ? TEXT("PASS") : TEXT("FAIL")), 1);

	const FString OutDir = FPaths::ProjectSavedDir() / TEXT("Spikes/S1");
	const FString Base = OutDir / RunName + TEXT("_") + Label;
	FFileHelper::SaveStringArrayToFile(Report, *(Base + TEXT(".txt")));
	FImageUtils::SaveImageByExtension(*(Base + TEXT(".png")), FImageView(Pixels.GetData(), Size.X, Size.Y));
	for (const FString& Line : Report)
	{
		UE_LOG(LogPaw, Display, TEXT("S1 [%s] %s"), *Label, *Line);
	}
}
