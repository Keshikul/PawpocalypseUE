#include "Realm/PawRealmView.h"

#include "Camera/CameraComponent.h"
#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "Realm/PawRealmSubsystem.h"
#include "Render/PawSpriteRenderer.h"

namespace
{
	// Placeholder colours until phase 6's art (sRGB hex from the Godot palette where one exists).
	FLinearColor Hex(const TCHAR* Code, float Alpha = 1.0f)
	{
		FLinearColor C = FLinearColor::FromSRGBColor(FColor::FromHex(Code));
		C.A = Alpha;
		return C;
	}

	const FLinearColor Paper = Hex(TEXT("FFF9E3"));
	const FLinearColor Ink = Hex(TEXT("191919"));
	const FLinearColor Gold = Hex(TEXT("F2C14E"));
	const FLinearColor Signature = Hex(TEXT("C94F9B"));
	const FLinearColor ShieldBlue = Hex(TEXT("6FA8DC"));
	const FLinearColor KittenOrange = Hex(TEXT("E8964A"));
	const FLinearColor BossRed = Hex(TEXT("8E2C2C"));
	const FLinearColor AddViolet = Hex(TEXT("5B3F8C"));
	const FLinearColor MouseGrey = Hex(TEXT("8C8378"));

	constexpr double CatSizePx = 64.0;    // placeholder (phase 6: the cat's visual definition)
	constexpr double KittenSizePx = 34.0;
	constexpr double BossSizePx = 150.0;
	constexpr double AddSizePx = 30.0;

	FLinearColor ZoneColor(Pc::PcZoneKind Kind)
	{
		switch (Kind)
		{
		case Pc::PcZoneKind::Trail: return Hex(TEXT("F08A4B"));
		case Pc::PcZoneKind::Echo: return Hex(TEXT("7FB3E0"));
		case Pc::PcZoneKind::Crater: return Hex(TEXT("A07850"));
		default: return Ink;
		}
	}
}

APawRealmView::APawRealmView()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Renderer = CreateDefaultSubobject<UPawSpriteRenderer>(TEXT("Renderer"));
	Renderer->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);
	Camera->SetRelativeLocationAndRotation(FVector(0.0, 0.0, 1000.0), FRotator(-90.0, 0.0, 0.0));
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	// OrthoWidth is the visible HEIGHT under MaintainYFOV: the width follows the window (docs/04 §6, spike S1).
	Camera->bOverrideAspectRatioAxisConstraint = true;
	Camera->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainYFOV);
	Camera->SetOrthoWidth(LogicalHeight);
	Camera->SetConstraintAspectRatio(false);
	// Exact colours (docs/04 §3, spike S1): exposure pinned at 0 EV, no tone curve, no gamut or blue correction, no
	// vignette.
	FPostProcessSettings& S = Camera->PostProcessSettings;
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
	Camera->PostProcessBlendWeight = 1.0f;
}

UPawRealmSubsystem* APawRealmView::Realm() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UPawRealmSubsystem>() : nullptr;
}

void APawRealmView::BeginPlay()
{
	Super::BeginPlay();
	InitView();
	if (bStartRealmOnBeginPlay)
	{
		if (UPawRealmSubsystem* R = Realm())
		{
			R->StartRealm();
		}
	}
}

void APawRealmView::InitView()
{
	SetActorLocation(FVector::ZeroVector);
	if (!QuadMesh || !FlatMaterial || !SoftMaterial || !DiscMaterial || !RingMaterial)
	{
		UE_LOG(LogPaw, Error, TEXT("Realm view: a mesh or material is not set (py tools/create_maps.py places a configured view)"));
	}
	Renderer->Setup(QuadMesh, FlatMaterial, SoftMaterial, DiscMaterial, RingMaterial);
	UpdateViewSize();
}

double APawRealmView::LogicalWidthFor(double ViewportW, double ViewportH)
{
	return ViewportH > 0.0 ? LogicalHeight * ViewportW / ViewportH : LogicalHeight * 16.0 / 9.0;
}

FVector2D APawRealmView::FieldFromViewport(const FVector2D& Pixel, double ViewportH)
{
	// Orthographic and 1080 logical pixels high: a field pixel is a viewport pixel scaled (docs/04 §6). Both the mouse and
	// phase 5's virtual cursor go through here.
	const double Scale = ViewportH > 0.0 ? LogicalHeight / ViewportH : 1.0;
	return Pixel * Scale;
}

void APawRealmView::UpdateViewSize()
{
	FIntPoint Size(1920, 1080);
	if (const UGameViewportClient* VC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr; VC && VC->Viewport)
	{
		Size = VC->Viewport->GetSizeXY();
	}
	if (Size.X <= 0 || Size.Y <= 0 || Size == LastViewport)
	{
		return;
	}
	LastViewport = Size;
	LogicalWidth = LogicalWidthFor(Size.X, Size.Y);
	if (UPawRealmSubsystem* R = Realm())
	{
		// The subsystem measures the field from this and the dock's rect, which the HUD reports (docs/05 §6.1).
		R->SetViewSize(LogicalWidth, LogicalHeight);
	}
}

void APawRealmView::SampleInput()
{
	UPawRealmSubsystem* R = Realm();
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!R || !PC)
	{
		return;
	}
	if (PC->GetViewTarget() != this)
	{
		// With no pawn the controller resets its view target after BeginPlay (spike S1): claim it back.
		PC->SetViewTarget(this);
	}
	float MX = 0.0f, MY = 0.0f;
	bool bOverDock = false;
	if (PC->GetMousePosition(MX, MY) && LastViewport.Y > 0)
	{
		const FVector2D Field = FieldFromViewport(FVector2D(MX, MY), LastViewport.Y);
		R->SetCat(Field.X, Field.Y); // wherever the pointer is, under the dock too (player.gd: only the stick is clamped)
		bOverDock = R->IsOverDock(Field.X, Field.Y);
	}
	// Godot's `pounce` action: the left mouse button, or the right trigger past 0.5. Read raw until phase 5 moves every
	// action to Enhanced Input with remapping. A click on the dock is the dock's (Godot: the button takes the click).
	const bool bMouse = PC->IsInputKeyDown(EKeys::LeftMouseButton) && !bOverDock;
	const bool bDown = bMouse || PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis) > 0.5f;
	R->SetPounceDown(bDown);
}

void APawRealmView::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateViewSize();
	if (bSampleInput)
	{
		SampleInput();
	}
	RunFrame(DeltaSeconds);
}

void APawRealmView::RunFrame(double RealDelta)
{
	UPawRealmSubsystem* R = Realm();
	if (!R || !R->IsRunning())
	{
		return;
	}
	R->AdvanceFrame(RealDelta);
	for (const Pc::PcFieldEvent& E : R->GetSim().FrameEvents)
	{
		SpawnEffects(E);
	}
	for (int32 I = Effects.Num() - 1; I >= 0; --I)
	{
		Effects[I].Age += RealDelta;
		if (Effects[I].Age >= Effects[I].Life)
		{
			Effects.RemoveAtSwap(I);
		}
	}
	Draw();
}

void APawRealmView::Burst(double X, double Y, double From, double To, double Life, const FLinearColor& Color)
{
	// A cap on the live effects (docs/04 §9): late-game kills come in floods, the effects must not follow them. A total, not a
	// per-frame budget: a burst past the cap is dropped until older effects expire.
	constexpr int32 MaxEffects = 256;
	if (Effects.Num() < MaxEffects)
	{
		Effects.Add({ X, Y, From, To, 0.0, Life, Color });
		++Spawned;
	}
}

void APawRealmView::SpawnEffects(const Pc::PcFieldEvent& E)
{
	using K = Pc::PcFieldEventKind;
	switch (E.Kind)
	{
	case K::Strike:
		Burst(E.ToX, E.ToY, 8.0, (E.Flags & Pc::FieldFlag::Crit) ? 46.0 : 30.0, 0.18,
			(E.Flags & Pc::FieldFlag::Crit) ? Gold : Ink);
		break;
	case K::Killed:
		Burst(E.X, E.Y, 10.0, 48.0, 0.3, (E.Flags & Pc::FieldFlag::Golden) ? Gold
			: (E.Flags & Pc::FieldFlag::Execute) ? Hex(TEXT("D9483B")) : Hex(TEXT("E0803A")));
		break;
	case K::Splash:
	case K::Cleave:
		Burst(E.ToX, E.ToY, 6.0, 26.0, 0.2, Hex(TEXT("F08A4B")));
		break;
	case K::Pounce:
		Burst(E.X, E.Y, E.Value * 0.3, E.Value, 0.35, (E.Flags & Pc::FieldFlag::Auto) ? Hex(TEXT("8C8378")) : Ink);
		break;
	case K::KittenBlast:
	case K::AuraPulse:
	case K::CatnipBomb:
	case K::PrismBurst:
		Burst(E.X, E.Y, E.Value * 0.2, E.Value, 0.4, E.Kind == K::CatnipBomb ? Hex(TEXT("7BC96F")) : Hex(TEXT("B58CE0")));
		break;
	case K::PoopDropped:
		PoopRadius = E.Value;
		break;
	case K::BossSummoned:
		Burst(E.X, E.Y, 40.0, 220.0, 0.6, BossRed);
		break;
	case K::BossDefeated:
		Burst(E.X, E.Y, 60.0, 420.0, 0.9, Gold);
		break;
	case K::AddIntercepted:
		Burst(E.X, E.Y, 6.0, 30.0, 0.25, AddViolet);
		break;
	default:
		break;
	}
}

void APawRealmView::Draw()
{
	UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* Game = R ? R->GetGame() : nullptr;
	if (!R || !Game || !Renderer->IsSetUp())
	{
		return;
	}
	const double T0 = FPlatformTime::Seconds();
	const FPawRealmSim& Sim = R->GetSim();
	const Pc::PcField& F = Sim.Field;
	const Pc::PcGameData& D = Game->Data();
	const Pc::PcProgress& P = Game->Progress();
	UPawSpriteRenderer& Out = *Renderer;
	Out.BeginFrame(LogicalWidth, LogicalHeight);

	Out.Add(EPawLayer::Floor, LogicalWidth * 0.5, LogicalHeight * 0.5, LogicalWidth, LogicalHeight, Paper);

	double CatX = 0.0, CatY = 0.0;
	Sim.CatPos(CatX, CatY);
	// Ground: the aura, the poop, the damage zones.
	if (Pc::IsAuraOwned(P))
	{
		const double Radius = Pc::AuraRadius(D, P);
		Out.Add(EPawLayer::Zones, CatX, CatY, Radius * 2.0, Radius * 2.0, Hex(TEXT("B58CE0"), 0.22f));
	}
	if (F.bPoopActive && PoopRadius > 0.0)
	{
		Out.Add(EPawLayer::Zones, F.PoopX, F.PoopY, PoopRadius * 2.0, PoopRadius * 2.0, Hex(TEXT("98BE98"), 0.35f));
	}
	for (const Pc::PcDamageZone& Z : F.Zones)
	{
		FLinearColor C = ZoneColor(Z.Kind);
		C.A = 0.4f * static_cast<float>(Z.Duration > 0.0 ? FMath::Clamp(Z.Remaining / Z.Duration, 0.0, 1.0) : 1.0);
		Out.Add(EPawLayer::Zones, Z.X, Z.Y, Z.Radius * 2.0, Z.Radius * 2.0, C);
	}

	const FLinearColor Shadow(0.0f, 0.0f, 0.0f, 0.3f);
	auto Body = [&Out, &Shadow](double X, double Y, double Size, const FLinearColor& C)
	{
		// At the feet: centred low enough that the body leaves its lower half showing.
		Out.Add(EPawLayer::Shadows, X, Y + Size * 0.45, Size * 1.0, Size * 0.4, Shadow);
		Out.Add(EPawLayer::Bodies, X, Y, Size, Size, C);
	};

	// The mice, in their data's tint at their data's size.
	for (const int32 S : F.Live)
	{
		const size_t I = static_cast<size_t>(S);
		const Pc::PcEnemy& E = D.Enemies[static_cast<size_t>(F.Kind[I])];
		double X = 0.0, Y = 0.0;
		Sim.MousePos(S, X, Y);
		// The data's tint is a Godot modulate (sRGB) over a sprite; over a placeholder disc it modulates a mouse grey.
		auto Byte = [](double V) { return static_cast<uint8>(FMath::Clamp(V, 0.0, 1.0) * 255.0 + 0.5); };
		FLinearColor C = MouseGrey * FLinearColor(FColor(Byte(E.Tint[0]), Byte(E.Tint[1]), Byte(E.Tint[2])));
		C.A = 1.0f;
		if (E.bSparkle)
		{
			C = Gold;
		}
		else if (E.bIsSignature)
		{
			C = Signature;
		}
		if (F.Shielded[I])
		{
			C = FLinearColor::LerpUsingHSV(C, ShieldBlue, 0.6f);
		}
		Body(X, Y, E.SpriteTargetPx, C);
	}
	for (size_t K = 0; K < F.KActive.size(); ++K)
	{
		if (!F.KActive[K])
		{
			continue;
		}
		double X = 0.0, Y = 0.0;
		Sim.KittenPos(static_cast<int32>(K), X, Y);
		const FLinearColor C = F.KIllusory[K] ? FLinearColor::LerpUsingHSV(KittenOrange, Paper, 0.6f) : F.KGolden[K] ? Gold : KittenOrange;
		Body(X, Y, F.KLone[K] ? KittenSizePx * 1.25 : KittenSizePx, C);
	}
	if (F.bBossUp)
	{
		double X = 0.0, Y = 0.0;
		Sim.BossPos(X, Y);
		Body(X, Y, BossSizePx, F.bBossShielded ? FLinearColor::LerpUsingHSV(BossRed, ShieldBlue, 0.5f) : BossRed);
	}
	for (const Pc::PcBossAdd& A : F.Adds)
	{
		Body(A.X, A.Y, AddSizePx, AddViolet);
	}
	Body(CatX, CatY, CatSizePx, Ink);

	for (const FFx& Fx : Effects)
	{
		const double T = Fx.Age / Fx.Life;
		const double Radius = FMath::Lerp(Fx.From, Fx.To, T);
		FLinearColor C = Fx.Color;
		C.A = static_cast<float>(1.0 - T);
		Out.Add(EPawLayer::Fx, Fx.X, Fx.Y, Radius * 2.0, Radius * 2.0, C);
	}
	Out.EndFrame();
	DrawMs = (FPlatformTime::Seconds() - T0) * 1000.0;
}
