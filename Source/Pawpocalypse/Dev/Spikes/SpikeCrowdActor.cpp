#include "Dev/Spikes/SpikeCrowdActor.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Core/PawLog.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "RHIStats.h"
#include "RenderTimer.h"
#include "Rendering/DrawElements.h"
#include "UnrealClient.h"
#include "Widgets/SLeafWidget.h"

namespace SpikeS2
{
	constexpr double FieldW = 1920.0;
	constexpr double FieldH = 1080.0;
	constexpr double Step = 1.0 / 60.0;
	constexpr int32 MaxStepsPerFrame = 8;
	constexpr int32 NumEnemies = 320;
	constexpr int32 NumKittens = 30;
	constexpr int32 NumZones = 64;
	constexpr int32 NumEffects = 40;
	constexpr int32 NumCells = 40;
	constexpr int32 AtlasCols = 8;
	// Time, not frames: packaged builds skip drawing a component until its PSOs are precached, which on a weak GPU took
	// longer than 120 frames (the ISM run measured an empty field on the integrated GPU).
	constexpr double WarmupSeconds = 6.0;
	constexpr int32 SampleFrames = 600;
	const TCHAR* Root = TEXT("/Game/Pawpocalypse/Dev/Spikes/S2");

	double ZoneZ() { return 5.0; }
	double ShadowZ() { return 20.0; }
	double CreatureZ(double Y) { return 100.0 + (Y / FieldH) * 100.0; }
	double EffectZ() { return 300.0; }

	template <typename T>
	T* Load(const FString& Name)
	{
		return LoadObject<T>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), Root, *Name, *Name));
	}

	double Percentile(TArray<double> V, double P)
	{
		if (V.Num() == 0)
		{
			return 0.0;
		}
		V.Sort();
		return V[FMath::Clamp(static_cast<int32>(P * (V.Num() - 1)), 0, V.Num() - 1)];
	}

	double Mean(const TArray<double>& V)
	{
		double Sum = 0.0;
		for (double X : V)
		{
			Sum += X;
		}
		return V.Num() ? Sum / V.Num() : 0.0;
	}
}

// The Slate renderer (docs/04 §5.3): the whole field painted by one leaf widget, sorted on the CPU.
class SSpikeCrowdCanvas : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SSpikeCrowdCanvas) {}
	SLATE_END_ARGS()

	// Textures: atlas, shadow, zone, ring.
	void Construct(const FArguments&, const ASpikeCrowdActor* InOwner, const TArray<TObjectPtr<UTexture2D>>& Tex, bool bInStrictOrder)
	{
		UTexture2D* Atlas = Tex[0];
		Owner = InOwner;
		bStrictOrder = bInStrictOrder;
		CellBrushes.SetNum(SpikeS2::NumCells);
		for (int32 I = 0; I < SpikeS2::NumCells; ++I)
		{
			FSlateBrush& B = CellBrushes[I];
			B.SetResourceObject(Atlas);
			B.ImageSize = FVector2D(256.0, 256.0);
			const float U = (I % SpikeS2::AtlasCols) / 8.0f;
			const float V = (I / SpikeS2::AtlasCols) / 8.0f;
			B.SetUVRegion(FBox2f(FVector2f(U, V), FVector2f(U + 0.125f, V + 0.125f)));
		}
		ShadowBrush.SetResourceObject(Tex[1]);
		ZoneBrush.SetResourceObject(Tex[2]);
		RingBrush.SetResourceObject(Tex[3]);
		SetCanTick(false);
		ForceVolatile(true);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(SpikeS2::FieldW, SpikeS2::FieldH); }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geo, const FSlateRect&, FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&, bool) const override
	{
		const double T0 = FPlatformTime::Seconds();
		const FVector2f Local = Geo.GetLocalSize();
		const float K = Local.Y / static_cast<float>(SpikeS2::FieldH);
		const float OX = (Local.X - static_cast<float>(SpikeS2::FieldW) * K) * 0.5f;

		Sorted.Reset();
		for (const FS2DrawItem& It : Owner->GetDrawItems())
		{
			Sorted.Add(&It);
		}
		// Bands first, then feet-Y inside the creature band: the CPU does what the depth buffer does in the world renderer.
		Sorted.Sort([](const FS2DrawItem& A, const FS2DrawItem& B)
		{
			return A.Kind != B.Kind ? A.Kind < B.Kind : (A.Kind == FS2DrawItem::Creature ? A.Pos.Y < B.Pos.Y : false);
		});

		int32 Layer = LayerId;
		for (const FS2DrawItem* It : Sorted)
		{
			const FSlateBrush* Brush = It->Kind == FS2DrawItem::Creature ? &CellBrushes[It->Cell]
				: It->Kind == FS2DrawItem::Shadow ? &ShadowBrush : It->Kind == FS2DrawItem::Zone ? &ZoneBrush : &RingBrush;
			const float S = static_cast<float>(It->Size) * K;
			const FVector2f TopLeft(OX + static_cast<float>(It->Pos.X) * K - S * 0.5f, static_cast<float>(It->Pos.Y) * K - S * 0.5f);
			// Band layers batch by texture (fast, but Slate may reorder overlaps inside a band); strict mode gives every
			// creature its own layer so the Y order is exact, at the cost of batching.
			const int32 ItemLayer = bStrictOrder && It->Kind == FS2DrawItem::Creature ? ++Layer : LayerId + 1 + It->Kind;
			FSlateDrawElement::MakeBox(Out, ItemLayer, Geo.ToPaintGeometry(FVector2f(S, S), FSlateLayoutTransform(TopLeft)),
				Brush, ESlateDrawEffect::None, It->Color);
		}
		PaintMs = (FPlatformTime::Seconds() - T0) * 1000.0;
		return FMath::Max(Layer, LayerId + 5);
	}

	mutable double PaintMs = 0.0;

private:
	const ASpikeCrowdActor* Owner = nullptr;
	bool bStrictOrder = false;
	TArray<FSlateBrush> CellBrushes;
	FSlateBrush ShadowBrush, ZoneBrush, RingBrush;
	mutable TArray<const FS2DrawItem*> Sorted;
};

ASpikeCrowdActor::ASpikeCrowdActor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ASpikeCrowdActor::BeginPlay()
{
	Super::BeginPlay();

	FParse::Value(FCommandLine::Get(), TEXT("-S2Run="), RunName);
	FString Mode = TEXT("ism");
	FParse::Value(FCommandLine::Get(), TEXT("-S2Renderer="), Mode);
	Renderer = Mode == TEXT("paper") ? ERenderer::Paper : Mode == TEXT("slate") ? ERenderer::Slate : ERenderer::Ism;

	SetupView();
	InitSim();
	BuildDrawItems(1.0);
	switch (Renderer)
	{
	case ERenderer::Ism: SetupIsm(); break;
	case ERenderer::Paper: SetupPaper(); break;
	case ERenderer::Slate: SetupSlate(); break;
	}
	UE_LOG(LogPaw, Display, TEXT("S2 run '%s': renderer=%s items=%d"), *RunName, *Mode, Items.Num());
}

void ASpikeCrowdActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Canvas && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Canvas.ToSharedRef());
	}
	Canvas.Reset();
	Super::EndPlay(Reason);
}

void ASpikeCrowdActor::SetupView()
{
	UWorld* World = GetWorld();
	ACameraActor* Camera = World->SpawnActor<ACameraActor>(FVector(0.0, 0.0, 1000.0), FRotator(-90.0, 0.0, 0.0));
	UCameraComponent* Cam = Camera->GetCameraComponent();
	Cam->SetProjectionMode(ECameraProjectionMode::Orthographic);
	// OrthoWidth is the visible height under MaintainYFOV (spike S1, docs/04 §6).
	Cam->bOverrideAspectRatioAxisConstraint = true;
	Cam->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV);
	Cam->SetOrthoWidth(SpikeS2::FieldH);
	Cam->SetConstraintAspectRatio(false);
	SpikeCamera = Camera;

	APostProcessVolume* PPV = World->SpawnActor<APostProcessVolume>();
	PPV->bUnbound = true;
	FPostProcessSettings& S = PPV->Settings;
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = 0.0f;
	S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	S.AutoExposureApplyPhysicalCameraExposure = 0;
	S.bOverride_ToneCurveAmount = true;
	S.ToneCurveAmount = 0.0f;
	S.bOverride_BlueCorrection = true;
	S.BlueCorrection = 0.0f;
	S.bOverride_ExpandGamut = true;
	S.ExpandGamut = 0.0f;
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.0f;
}

void ASpikeCrowdActor::InitSim()
{
	Rng.Initialize(2026);
	for (int32 I = 0; I < SpikeS2::NumEnemies; ++I)
	{
		const FVector2D P(Rng.FRandRange(60.0, SpikeS2::FieldW - 60.0), Rng.FRandRange(60.0, SpikeS2::FieldH - 60.0));
		const double A = Rng.FRandRange(0.0, 2.0 * UE_DOUBLE_PI);
		EnemyPos.Add(P);
		EnemyPrev.Add(P);
		EnemyVel.Add(FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rng.FRandRange(60.0, 220.0));
		EnemyCell.Add(7 + Rng.RandRange(0, SpikeS2::NumCells - 8)); // creatures; cells 0-6 are bosses
		EnemySize.Add(Rng.FRandRange(56.0, 84.0));
	}
	for (int32 I = 0; I < SpikeS2::NumKittens; ++I)
	{
		const FVector2D P(Rng.FRandRange(200.0, 1700.0), Rng.FRandRange(200.0, 900.0));
		KittenPos.Add(P);
		KittenPrev.Add(P);
		KittenCell.Add(7 + Rng.RandRange(0, SpikeS2::NumCells - 8));
	}
	const FLinearColor ZoneColors[] = { FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("98BE98"))),
		FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("E5D4B7"))), FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("AFD1EC"))) };
	for (int32 I = 0; I < SpikeS2::NumZones; ++I)
	{
		ZonePos.Add(FVector2D(Rng.FRandRange(0.0, SpikeS2::FieldW), Rng.FRandRange(0.0, SpikeS2::FieldH)));
		ZoneSize.Add(Rng.FRandRange(120.0, 280.0));
		FLinearColor C = ZoneColors[I % 3];
		C.A = 0.45f;
		ZoneColor.Add(C);
	}
	for (int32 I = 0; I < SpikeS2::NumEffects; ++I)
	{
		EffectPos.Add(FVector2D(Rng.FRandRange(0.0, SpikeS2::FieldW), Rng.FRandRange(0.0, SpikeS2::FieldH)));
		EffectLife.Add(Rng.FRandRange(0.4, 0.9));
		EffectAge.Add(Rng.FRandRange(0.0, EffectLife.Last()));
	}
}

void ASpikeCrowdActor::StepSim(double Dt)
{
	EnemyPrev = EnemyPos;
	KittenPrev = KittenPos;
	for (int32 I = 0; I < EnemyPos.Num(); ++I)
	{
		FVector2D& P = EnemyPos[I];
		FVector2D& V = EnemyVel[I];
		P += V * Dt;
		if (P.X < 30.0 || P.X > SpikeS2::FieldW - 30.0) { V.X = -V.X; }
		if (P.Y < 30.0 || P.Y > SpikeS2::FieldH - 30.0) { V.Y = -V.Y; }
	}
	// Kittens seek the nearest enemy: brute force 30 x 320 per step (PcField will use a grid, docs/04 §9).
	for (int32 K = 0; K < KittenPos.Num(); ++K)
	{
		double Best = TNumericLimits<double>::Max();
		FVector2D Target = KittenPos[K];
		for (const FVector2D& E : EnemyPos)
		{
			const double D = FVector2D::DistSquared(E, KittenPos[K]);
			if (D < Best && D > 400.0)
			{
				Best = D;
				Target = E;
			}
		}
		KittenPos[K] += (Target - KittenPos[K]).GetSafeNormal() * 300.0 * Dt;
	}
	for (int32 I = 0; I < EffectAge.Num(); ++I)
	{
		EffectAge[I] += Dt;
		if (EffectAge[I] > EffectLife[I])
		{
			EffectAge[I] = 0.0;
			EffectPos[I] = FVector2D(Rng.FRandRange(0.0, SpikeS2::FieldW), Rng.FRandRange(0.0, SpikeS2::FieldH));
		}
	}
}

void ASpikeCrowdActor::BuildDrawItems(double Alpha)
{
	Items.Reset();
	for (int32 I = 0; I < ZonePos.Num(); ++I)
	{
		Items.Add({ FS2DrawItem::Zone, ZonePos[I], ZoneSize[I], 0, ZoneColor[I] });
	}
	const FLinearColor ShadowColor(0.0f, 0.0f, 0.0f, 0.35f);
	for (int32 I = 0; I < EnemyPos.Num(); ++I)
	{
		const FVector2D P = FMath::Lerp(EnemyPrev[I], EnemyPos[I], Alpha);
		Items.Add({ FS2DrawItem::Shadow, P + FVector2D(0.0, EnemySize[I] * 0.35), EnemySize[I] * 0.9, 0, ShadowColor });
	}
	for (int32 I = 0; I < EnemyPos.Num(); ++I)
	{
		Items.Add({ FS2DrawItem::Creature, FMath::Lerp(EnemyPrev[I], EnemyPos[I], Alpha), EnemySize[I], EnemyCell[I], FLinearColor::White });
	}
	for (int32 I = 0; I < KittenPos.Num(); ++I)
	{
		Items.Add({ FS2DrawItem::Creature, FMath::Lerp(KittenPrev[I], KittenPos[I], Alpha), 48.0, KittenCell[I], FLinearColor::White });
	}
	for (int32 I = 0; I < EffectPos.Num(); ++I)
	{
		const double T = EffectAge[I] / EffectLife[I];
		Items.Add({ FS2DrawItem::Effect, EffectPos[I], 20.0 + 140.0 * T, 0, FLinearColor(1.0f, 0.85f, 0.4f, static_cast<float>(1.0 - T)) });
	}
}

FTransform ASpikeCrowdActor::FieldTransform(const FVector2D& Center, double Size, double Z, double PlaneSize) const
{
	// Field (x right, y down) -> world (screen up = +X, screen right = +Y); yaw +90 for the plane's UVs (spike S1).
	const FVector Location(SpikeS2::FieldH * 0.5 - Center.Y, Center.X - SpikeS2::FieldW * 0.5, Z);
	const double S = Size / PlaneSize;
	return FTransform(FRotator(0.0, 90.0, 0.0), Location, FVector(S, S, 1.0));
}

// --- ISM renderer (docs/04 §5.1) -------------------------------------------------------------------------------------

void ASpikeCrowdActor::SetupIsm()
{
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	auto Make = [this, Plane](const TCHAR* Name, UMaterialInterface* Mat, int32 Floats)
	{
		UInstancedStaticMeshComponent* C = NewObject<UInstancedStaticMeshComponent>(this, Name);
		C->SetStaticMesh(Plane);
		C->SetMaterial(0, Mat);
		C->NumCustomDataFloats = Floats;
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(RootComponent);
		C->RegisterComponent();
		return C;
	};
	UMaterialInterface* Soft = SpikeS2::Load<UMaterialInterface>(TEXT("M_S2_Soft"));
	UMaterialInstanceDynamic* ShadowMat = UMaterialInstanceDynamic::Create(Soft, this);
	ShadowMat->SetTextureParameterValue(TEXT("Tex"), SpikeS2::Load<UTexture2D>(TEXT("T_S2_shadow")));
	UMaterialInstanceDynamic* ZoneMat = UMaterialInstanceDynamic::Create(Soft, this);
	ZoneMat->SetTextureParameterValue(TEXT("Tex"), SpikeS2::Load<UTexture2D>(TEXT("T_S2_zone")));

	IsmZones = Make(TEXT("IsmZones"), ZoneMat, 4);
	IsmShadows = Make(TEXT("IsmShadows"), ShadowMat, 4);
	IsmCreatures = Make(TEXT("IsmCreatures"), SpikeS2::Load<UMaterialInterface>(TEXT("M_S2_Atlas")), 2);
	IsmEffects = Make(TEXT("IsmEffects"), SpikeS2::Load<UMaterialInterface>(TEXT("M_S2_Add")), 4);

	for (const FS2DrawItem& It : Items)
	{
		switch (It.Kind)
		{
		case FS2DrawItem::Zone:
		{
			const int32 I = IsmZones->AddInstance(FieldTransform(It.Pos, It.Size, SpikeS2::ZoneZ(), 100.0));
			IsmZones->SetCustomData(I, { It.Color.R, It.Color.G, It.Color.B, It.Color.A });
			break;
		}
		case FS2DrawItem::Shadow:
		{
			const int32 I = IsmShadows->AddInstance(FieldTransform(It.Pos, It.Size, SpikeS2::ShadowZ(), 100.0));
			IsmShadows->SetCustomData(I, { It.Color.R, It.Color.G, It.Color.B, It.Color.A });
			break;
		}
		case FS2DrawItem::Creature:
		{
			const int32 I = IsmCreatures->AddInstance(FieldTransform(It.Pos, It.Size, SpikeS2::CreatureZ(It.Pos.Y), 100.0));
			// Per-instance atlas cell, set once (docs/04 §5.1); the material adds it to TexCoord * 1/8.
			IsmCreatures->SetCustomData(I, { (It.Cell % SpikeS2::AtlasCols) / 8.0f, (It.Cell / SpikeS2::AtlasCols) / 8.0f });
			break;
		}
		case FS2DrawItem::Effect:
		{
			const int32 I = IsmEffects->AddInstance(FieldTransform(It.Pos, It.Size, SpikeS2::EffectZ(), 100.0));
			IsmEffects->SetCustomData(I, { It.Color.R * It.Color.A, It.Color.G * It.Color.A, It.Color.B * It.Color.A, 1.0f });
			break;
		}
		}
	}
}

void ASpikeCrowdActor::UpdateIsm()
{
	// One batch per moving component per frame, render state dirtied once by the batch call (docs/04 §5.1).
	auto Batch = [this](UInstancedStaticMeshComponent* Comp, FS2DrawItem::EKind Kind)
	{
		ScratchTransforms.Reset();
		for (const FS2DrawItem& It : Items)
		{
			if (It.Kind != Kind)
			{
				continue;
			}
			const double Z = Kind == FS2DrawItem::Creature ? SpikeS2::CreatureZ(It.Pos.Y) : Kind == FS2DrawItem::Shadow ? SpikeS2::ShadowZ() : SpikeS2::EffectZ();
			ScratchTransforms.Add(FieldTransform(It.Pos, It.Size, Z, 100.0));
		}
		Comp->BatchUpdateInstancesTransforms(0, ScratchTransforms, false, true, false);
	};
	Batch(IsmShadows, FS2DrawItem::Shadow);
	Batch(IsmCreatures, FS2DrawItem::Creature);
	Batch(IsmEffects, FS2DrawItem::Effect);
	int32 E = 0;
	for (const FS2DrawItem& It : Items)
	{
		if (It.Kind == FS2DrawItem::Effect)
		{
			IsmEffects->SetCustomData(E++, { It.Color.R * It.Color.A, It.Color.G * It.Color.A, It.Color.B * It.Color.A, 1.0f }, false);
		}
	}
}

// --- Paper2D renderer (docs/04 §5.2, measured to confirm the rejection) ----------------------------------------------

void ASpikeCrowdActor::SetupPaper()
{
	// Fixed names (a loose list file would not be in a packaged build): PS_S2_c00..c39 = atlas cells 0..39.
	TArray<UPaperSprite*> Cells;
	for (int32 I = 0; I < SpikeS2::NumCells; ++I)
	{
		if (UPaperSprite* S = SpikeS2::Load<UPaperSprite>(FString::Printf(TEXT("PS_S2_c%02d"), I)))
		{
			Cells.Add(S);
		}
	}
	UPaperSprite* Shadow = SpikeS2::Load<UPaperSprite>(TEXT("PS_S2_shadow"));
	UPaperSprite* Zone = SpikeS2::Load<UPaperSprite>(TEXT("PS_S2_zone"));
	UPaperSprite* Ring = SpikeS2::Load<UPaperSprite>(TEXT("PS_S2_ring"));
	// Each sprite carries its material (masked for creatures, translucent for the soft layers) as a hard reference, so
	// the cooker includes it; loading the Paper2D materials by path left them out of the packaged build.
	if (Cells.Num() != SpikeS2::NumCells || !Shadow || !Zone || !Ring)
	{
		UE_LOG(LogPaw, Error, TEXT("S2: Paper2D assets missing (cells %d)"), Cells.Num());
		return;
	}
	for (const FS2DrawItem& It : Items)
	{
		UPaperSpriteComponent* C = NewObject<UPaperSpriteComponent>(this);
		UPaperSprite* Sprite = It.Kind == FS2DrawItem::Creature ? Cells[It.Cell] : It.Kind == FS2DrawItem::Shadow ? Shadow : It.Kind == FS2DrawItem::Zone ? Zone : Ring;
		C->SetSprite(Sprite);
		C->SetSpriteColor(It.Color);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetupAttachment(RootComponent);
		C->RegisterComponent();
		PaperComps.Add(C);
	}
	UpdatePaper();
}

void ASpikeCrowdActor::UpdatePaper()
{
	// Paper2D sprites lie in the X-Z plane; turn local X to world +Y (screen right) and local Z to world +X (screen up).
	static const FRotator PaperToField = FRotationMatrix::MakeFromXZ(FVector(0.0, 1.0, 0.0), FVector(1.0, 0.0, 0.0)).Rotator();
	for (int32 I = 0; I < Items.Num() && I < PaperComps.Num(); ++I)
	{
		const FS2DrawItem& It = Items[I];
		if (It.Kind == FS2DrawItem::Zone && FrameCount > 0)
		{
			continue; // zones never move
		}
		const double Native = It.Kind == FS2DrawItem::Creature ? 256.0 : 128.0;
		const double Z = It.Kind == FS2DrawItem::Creature ? SpikeS2::CreatureZ(It.Pos.Y) : It.Kind == FS2DrawItem::Shadow ? SpikeS2::ShadowZ()
			: It.Kind == FS2DrawItem::Zone ? SpikeS2::ZoneZ() : SpikeS2::EffectZ();
		const FVector Location(SpikeS2::FieldH * 0.5 - It.Pos.Y, It.Pos.X - SpikeS2::FieldW * 0.5, Z);
		const double S = It.Size / Native;
		PaperComps[I]->SetRelativeTransform(FTransform(PaperToField, Location, FVector(S)));
		if (It.Kind == FS2DrawItem::Effect)
		{
			PaperComps[I]->SetSpriteColor(It.Color);
		}
	}
}

// --- Slate renderer (docs/04 §5.3) -----------------------------------------------------------------------------------

void ASpikeCrowdActor::SetupSlate()
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return;
	}
	// Slate brushes do not keep their textures alive: the actor holds them (UPROPERTY) for the run.
	SlateTextures = { SpikeS2::Load<UTexture2D>(TEXT("T_S2_atlas")), SpikeS2::Load<UTexture2D>(TEXT("T_S2_shadow")),
		SpikeS2::Load<UTexture2D>(TEXT("T_S2_zone")), SpikeS2::Load<UTexture2D>(TEXT("T_S2_ring")) };
	Canvas = SNew(SSpikeCrowdCanvas, this, SlateTextures, FParse::Param(FCommandLine::Get(), TEXT("S2SlateStrict")));
	GEngine->GameViewport->AddViewportWidgetContent(Canvas.ToSharedRef(), 0);
}

// --- frame -----------------------------------------------------------------------------------------------------------

void ASpikeCrowdActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bReported)
	{
		// A few frames' grace so a requested screenshot is written before the engine exits.
		if (--QuitCountdown == 0)
		{
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
			{
				PC->ConsoleCommand(TEXT("quit"));
			}
		}
		return;
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController(); PC && SpikeCamera && PC->GetViewTarget() != SpikeCamera)
	{
		PC->SetViewTarget(SpikeCamera);
	}

	const double T0 = FPlatformTime::Seconds();
	// Fixed 60 Hz step with an 8-step cap; presentation interpolates (docs/03 §5).
	Accumulator += DeltaSeconds;
	int32 Steps = 0;
	while (Accumulator >= SpikeS2::Step && Steps < SpikeS2::MaxStepsPerFrame)
	{
		StepSim(SpikeS2::Step);
		Accumulator -= SpikeS2::Step;
		++Steps;
	}
	if (Steps == SpikeS2::MaxStepsPerFrame)
	{
		Accumulator = 0.0;
	}
	BuildDrawItems(Accumulator / SpikeS2::Step);
	if (Renderer == ERenderer::Ism)
	{
		UpdateIsm();
	}
	else if (Renderer == ERenderer::Paper)
	{
		UpdatePaper();
	}
	const double Update = (FPlatformTime::Seconds() - T0) * 1000.0 + (Canvas ? Canvas->PaintMs : 0.0);

	++FrameCount;
	WarmupElapsed += DeltaSeconds;
	if (WarmupElapsed > SpikeS2::WarmupSeconds)
	{
		Sample(DeltaSeconds, Update);
	}
	if (FrameMs.Num() >= SpikeS2::SampleFrames)
	{
		Report();
	}
}

void ASpikeCrowdActor::Sample(float DeltaSeconds, double Update)
{
	FrameMs.Add(DeltaSeconds * 1000.0);
	GameMs.Add(FPlatformTime::ToMilliseconds(GGameThreadTime));
	RenderMs.Add(FPlatformTime::ToMilliseconds(GRenderThreadTime));
	GpuMs.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
	DrawCalls.Add(GNumDrawCallsRHI[0]);
	Prims.Add(GNumPrimitivesDrawnRHI[0]);
	UpdateMs.Add(Update);
}

void ASpikeCrowdActor::Report()
{
	bReported = true;
	const FIntPoint Size = GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport ? GEngine->GameViewport->Viewport->GetSizeXY() : FIntPoint::ZeroValue;
	auto Line = [](const TCHAR* Name, const TArray<double>& V)
	{
		return FString::Printf(TEXT("%-10s avg %8.3f  p99 %8.3f  max %8.3f"), Name, SpikeS2::Mean(V), SpikeS2::Percentile(V, 0.99), SpikeS2::Percentile(V, 1.0));
	};
	TArray<FString> Report;
	Report.Add(FString::Printf(TEXT("run=%s viewport=%dx%d items=%d frames=%d adapter=%s"), *RunName, Size.X, Size.Y, Items.Num(), FrameMs.Num(), *GRHIAdapterName));
	Report.Add(Line(TEXT("frame_ms"), FrameMs));
	Report.Add(Line(TEXT("game_ms"), GameMs));
	Report.Add(Line(TEXT("render_ms"), RenderMs));
	Report.Add(Line(TEXT("gpu_ms"), GpuMs));
	Report.Add(Line(TEXT("update_ms"), UpdateMs));
	Report.Add(Line(TEXT("draws"), DrawCalls));
	Report.Add(Line(TEXT("prims"), Prims));
	FFileHelper::SaveStringArrayToFile(Report, *(FPaths::ProjectSavedDir() / TEXT("Spikes/S2") / RunName + TEXT(".txt")));
	for (const FString& L : Report)
	{
		UE_LOG(LogPaw, Display, TEXT("S2 %s"), *L);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("S2Shot")))
	{
		FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Spikes/S2") / RunName + TEXT(".png"), true, false);
	}
	QuitCountdown = 10;
}
