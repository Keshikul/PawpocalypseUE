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
#include "Save/PawSettingsSubsystem.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "CommonInputSubsystem.h"
#include "EnhancedPlayerInput.h"
#include "Input/PawInputMap.h"
#include "InputAction.h"
#include "Engine/GameInstance.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "Realm/PawFieldLook.h"
#include "Render/PawSpriteAtlas.h"
#include "PcBackdropTones.h"
#include "Engine/Texture2D.h"

namespace
{
	// Placeholder colours until phase 6's art (sRGB hex from the Godot palette where one exists).
	FLinearColor Hex(const TCHAR* Code, float Alpha = 1.0f)
	{
		FLinearColor C = FLinearColor::FromSRGBColor(FColor::FromHex(Code));
		C.A = Alpha;
		return C;
	}

	// The floor and the effects' colours stay placeholders until phase 6's backdrop and VFX slices.
	const FLinearColor Paper = Hex(TEXT("FFF9E3"));

	FLinearColor TintOf(const std::array<double, 4>& T)
	{
		return FLinearColor(static_cast<float>(T[0]), static_cast<float>(T[1]), static_cast<float>(T[2]), static_cast<float>(T[3]));
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
	// The automatic ortho planes stay (spike S1). Measured 2026-11-05: with them, geometry at world Z 300 draws and at Z 400 is
	// clipped; explicit planes (near 0, far 2000) clipped everything above about Z 100. Every band therefore lies in Z 0..300
	// (UPawSpriteRenderer: floor 0, props 3-5, shadows 20, effects 15 / 25, bodies 100-220, top effects 300).
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
	if (!FieldAtlas || !SpriteMaterial || !SpriteSoftMaterial)
	{
		UE_LOG(LogPaw, Error, TEXT("Realm view: the field atlas or a sprite material is not set (py tools/create_maps.py)"));
	}
	Renderer->SetupSprites(FieldAtlas, SpriteMaterial, SpriteSoftMaterial);
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
	UPawInputDeviceSubsystem* Device = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
	const Pc::PcField& F = R->GetSim().Field;
	const float Dt = GetWorld()->GetDeltaSeconds();
	// The steer stick (player.gd _process): get_vector over the steer actions' axes, as the input map binds them.
	TMap<FKey, float> Axes;
	for (const TCHAR* Steer : { TEXT("steer_left"), TEXT("steer_right"), TEXT("steer_up"), TEXT("steer_down") })
	{
		for (const FKey& Key : PawInput::KeysOf(Steer, false))
		{
			Axes.Add(Key, PC->GetInputAnalogKeyState(Key));
		}
	}
	const UPawSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	const FVector2f Steer = PawInput::SteerVector(Axes);
	const bool bInvertX = Settings && Settings->Get().bInvertX;
	const bool bInvertY = Settings && Settings->Get().bInvertY;
	const FVector2f MoveInput(bInvertX ? -Steer.X : Steer.X, bInvertY ? -Steer.Y : Steer.Y);
	const FVector2f Velocity = PawInput::StickVelocity(MoveInput, static_cast<float>(F.Bounds.W))
		* static_cast<float>(Settings ? Settings->Get().CursorSpeed : 1.0);
	StickVelX = Velocity.X; // the cat faces by the stick alone (player.gd: SpriteLiveliness.face(_visual, velocity.x))
	// recentre: R3's press (its rising edge, Input.is_action_just_pressed).
	if (!RecentreAction)
	{
		RecentreAction = PawInput::LoadAction(TEXT("recentre"));
	}
	const UEnhancedPlayerInput* Input = Cast<UEnhancedPlayerInput>(PC->PlayerInput);
	const bool bRecentreDown = Input && RecentreAction && Input->GetActionValue(RecentreAction).IsNonZero();
	const bool bRecentre = bRecentreDown && !bRecentreWasDown;
	bRecentreWasDown = bRecentreDown;
	float MX = 0.0f, MY = 0.0f;
	const bool bMouse = PC->GetMousePosition(MX, MY) && LastViewport.Y > 0;
	const FVector2D Pointer = bMouse ? FieldFromViewport(FVector2D(MX, MY), LastViewport.Y) : FVector2D::ZeroVector;
	bool bOverDock = bMouse && R->IsOverDock(Pointer.X, Pointer.Y);
	// The Cone of Shame roots the cat after a Pounce: both paths freeze (the field ignores the input while it lasts).
	if (F.PounceLockLeft <= 0.0)
	{
		if (bRecentre)
		{
			// recentre(): the stick takes the cursor, the cat to the middle of the field, the pointer onto it.
			if (Device)
			{
				Device->ClaimStick();
			}
			MoveCat(PC, Device, F.Bounds.X + F.Bounds.W * 0.5, F.Bounds.Y + F.Bounds.H * 0.5);
		}
		else if (Device && Device->IsStickSteering())
		{
			// The stick path: move by velocity, stay put when released, clamped to the playfield; the pointer follows so
			// reaching for the mouse finds it under the cat.
			if (!Velocity.IsZero())
			{
				const FVector2D To = StickStep(FVector2D(F.CatX, F.CatY), Velocity, Dt, F.Bounds);
				MoveCat(PC, Device, To.X, To.Y);
			}
			bOverDock = false; // the stick's cat is never under the dock
		}
		else if (bMouse)
		{
			R->SetCat(Pointer.X, Pointer.Y); // wherever the pointer is, under the dock too (only the stick is clamped)
		}
	}
	// pointer_over_field(): inside the playfield, not over the dock, the field running (a paused game shows the pointer).
	if (Device)
	{
		Device->SetPointerOverField(bMouse && !R->IsPaused() && PointerOverField(Pointer, F.Bounds, bOverDock));
	}
	// Godot's `pounce` action, through the input map (IA_Pounce: the left mouse button, the right trigger past its dead zone).
	// A click on the dock is the dock's: the last input decides whether a press is the pointer's (Common UI's input type).
	if (!PounceAction)
	{
		PounceAction = PawInput::LoadAction(TEXT("pounce"));
	}
	const bool bAction = Input && PounceAction && Input->GetActionValue(PounceAction).IsNonZero();
	const UCommonInputSubsystem* Common = UCommonInputSubsystem::Get(PC->GetLocalPlayer());
	const bool bPointer = !Common || Common->GetCurrentInputType() == ECommonInputType::MouseAndKeyboard;
	R->SetPounceDown(PounceHeld(bAction, bPointer, bOverDock));
}

void APawRealmView::MoveCat(APlayerController* PC, UPawInputDeviceSubsystem* Device, double X, double Y)
{
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetCat(X, Y);
	}
	// _warp_pointer_here(): the OS pointer onto the cat, and the motion that warp reports is ignored (the warp echo).
	if (PC && LastViewport.Y > 0)
	{
		const double Scale = static_cast<double>(LastViewport.Y) / LogicalHeight;
		if (Device)
		{
			Device->NoteWarp();
		}
		PC->SetMouseLocation(FMath::RoundToInt(X * Scale), FMath::RoundToInt(Y * Scale));
	}
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
	if (FPawVfx::FContext C; MakeVfxContext(C, R->IsPaused() ? 0.0 : RealDelta))
	{
		Vfx.OnFrameEvents(C);
		Vfx.Tick(C);
	}
	TickLook(RealDelta, R->IsPaused());
	Draw();
}

bool APawRealmView::MakeVfxContext(FPawVfx::FContext& C, double Dt) const
{
	UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* Game = R ? R->GetGame() : nullptr;
	if (!R || !Game)
	{
		return false;
	}
	C.D = &Game->Data();
	C.P = &Game->Progress();
	C.Sim = &R->GetSim();
	double X = 0.0, Y = 0.0;
	R->GetSim().CatPos(X, Y);
	C.Cat = FVector2D(X, Y);
	const UGameInstance* GI = GetGameInstance();
	const UPawSettingsSubsystem* S = GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	C.Budget = S ? S->EffectBudget() : 1.0;
	C.Dt = Dt;
	return true;
}

void APawRealmView::SpawnEffects(const Pc::PcFieldEvent& E)
{
	using K = Pc::PcFieldEventKind;
	switch (E.Kind)
	{
	case K::Pounce:
		HopLeft = PawLook::HopSec; // Player.mark_pounced(): every Pounce hops, automatic or manual
		break;
	case K::BossDefeated:
		// Boss._play_death_sequence(): the body stays where it died, shudders, then collapses.
		if (const UPawRealmSubsystem* R = Realm(); R && R->GetGame())
		{
			const Pc::PcField& F = R->GetSim().Field;
			const Pc::PcGameData& D = R->GetGame()->Data();
			if (F.TierIndex >= 0 && static_cast<size_t>(F.TierIndex) < D.Tiers.size())
			{
				if (const Pc::PcBoss* B = D.FindBoss(D.Tiers[static_cast<size_t>(F.TierIndex)].BossData))
				{
					BossDeath.Cell = CellFor(UTF8_TO_TCHAR(B->TexturePath.c_str()));
				}
			}
			if (BossDeath.Cell != INDEX_NONE)
			{
				BossDeath.X = E.X;
				BossDeath.Y = E.Y;
				BossDeath.Size = Renderer->GetAtlas()->Cell(BossDeath.Cell).NativeWidth * PawLook::BossScale;
				BossDeath.Age = 0.0;
				DeathStart = Twitch;
				for (FVector2D& S : BossDeath.Shake)
				{
					S = FVector2D(LookRandom.FRandRange(-14.0, 14.0), LookRandom.FRandRange(-10.0, 10.0));
				}
			}
		}
		break;
	case K::BossHealed:
		Anger = 1.0; // Boss.heal(): visibly furious
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

	DrawBackdrop();

	DrawCreatures();

	// The effects (FPawVfx): three translucent bands in Godot's order among the sprites.
	if (FPawVfx::FContext C; MakeVfxContext(C, 0.0))
	{
		Vfx.Draw(C, FxGround, FxUnder, FxOver);
		Out.SetFx(EPawFxBand::Ground, FxGround, BackdropFillMaterial);
		Out.SetFx(EPawFxBand::Under, FxUnder, BackdropFillMaterial);
		Out.SetFx(EPawFxBand::Over, FxOver, BackdropFillMaterial);
	}
	Out.EndFrame();
	DrawMs = (FPlatformTime::Seconds() - T0) * 1000.0;
}

UTexture2D* APawRealmView::FloorTextureFor(const std::string& Stem) const
{
	const FString Want = FString(TEXT("T_")) + UTF8_TO_TCHAR(Stem.c_str());
	for (const TObjectPtr<UTexture2D>& T : FloorTextures)
	{
		if (T && T->GetName() == Want)
		{
			return T;
		}
	}
	return nullptr;
}

void APawRealmView::DrawBackdrop()
{
	UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* Game = R ? R->GetGame() : nullptr;
	UPawSpriteRenderer& Out = *Renderer;
	const Pc::PcField& F = R->GetSim().Field;
	const bool bBiome = Game && F.TierIndex >= 0 && static_cast<size_t>(F.TierIndex) < Game->Data().Tiers.size();
	const std::string Id = bBiome ? Game->Data().Tiers[static_cast<size_t>(F.TierIndex)].Id : std::string();
	const UPawSettingsSubsystem* Settings = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	const double Budget = Settings ? Settings->EffectBudget() : 1.0;
	// The budget is read when the backdrop is laid out, not watched: Godot's BiomeBackdrop lays out (and draws) on a biome and
	// a resize only, so a changed effects setting shows from the next travel or resize, as there.
	const FString Key = FString::Printf(TEXT("%hs|%.3f|%.3f"), Id.c_str(), LogicalWidth, LogicalHeight);
	if (Key != BackdropKey)
	{
		BackdropKey = Key;
		++BackdropBuildCount;
		Backdrop = bBiome ? Pc::BuildBackdrop(Id, LogicalWidth, LogicalHeight, Budget) : Pc::PcBackdropLayout();
		UTexture2D* FloorTex = Backdrop.Floor.empty() ? nullptr : FloorTextureFor(Backdrop.Floor);
		UE_CLOG(!Backdrop.Floor.empty() && !FloorTex, LogPaw, Error, TEXT("Realm view: no floor texture T_%hs (py tools/create_maps.py)"),
			Backdrop.Floor.c_str());
		Out.SetFloorTexture(FloorTex, FloorMaterial);
		BackdropCells.Reset();
		for (const Pc::PcBackdropProp& P : Backdrop.Props)
		{
			const int32 Cell = CellFor(UTF8_TO_TCHAR(P.Stem.c_str()));
			UE_CLOG(Cell == INDEX_NONE, LogPaw, Error, TEXT("Realm view: no atlas art for prop %hs"), P.Stem.c_str());
			BackdropCells.Add(Cell);
		}
		// The patches (patch tone at PATCH_ALPHA), then each puddle (its tone at PUDDLE_ALPHA) and its rim (the detail tone,
		// RimAlpha, PUDDLE_RIM_PX wide), as one mesh in Godot's draw order.
		const Pc::PcBackdropTones* Tones = nullptr;
		const Pc::PcBackdropTones* Water = nullptr;
		for (const Pc::PcBackdropTones& T : Pc::BackdropTones)
		{
			Tones = Backdrop.TonesId == T.Id ? &T : Tones;
			Water = Backdrop.WaterTonesId == T.Id ? &T : Water;
		}
		TArray<FVector2D> Points;
		TArray<FColor> Colors;
		TArray<int32> Tris;
		auto Byte = [](double A) { return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(A * 255.0), 0, 255)); };
		auto ToColor = [](const Pc::PcTone& T, uint8 A) { return FColor(T.R, T.G, T.B, A); };
		for (const Pc::PcBackdropBlob& B : Tones ? Backdrop.Blobs : std::vector<Pc::PcBackdropBlob>())
		{
			const FColor Fill = B.bPuddle ? ToColor(Water ? Water->Field : Tones->Puddle, Byte(Pc::Backdrop::PuddleAlpha))
				: ToColor(Tones->Patch, Byte(Pc::Backdrop::PatchAlpha));
			// A fan about the blob's centre: every outline is star-shaped about it (its radius never reaches zero).
			const int32 Base = Points.Num();
			Points.Add(FVector2D(B.AtX, B.AtY));
			Colors.Add(Fill);
			const int32 N = static_cast<int32>(B.Xs.size());
			for (int32 I = 0; I < N; ++I)
			{
				Points.Add(FVector2D(B.Xs[I], B.Ys[I]));
				Colors.Add(Fill);
			}
			for (int32 I = 0; I < N; ++I)
			{
				Tris.Append({ Base, Base + 1 + I, Base + 1 + (I + 1) % N });
			}
			if (!B.bPuddle)
			{
				continue;
			}
			// The rim: draw_polyline over the closed outline, a strip whose sides follow each point's averaged normal.
			const FColor Rim = ToColor(Tones->Detail, Byte(Pc::Backdrop::RimAlpha));
			const int32 RimBase = Points.Num();
			for (int32 I = 0; I < N; ++I)
			{
				const FVector2D P(B.Xs[I], B.Ys[I]);
				const FVector2D Prev(B.Xs[(I + N - 1) % N], B.Ys[(I + N - 1) % N]);
				const FVector2D Next(B.Xs[(I + 1) % N], B.Ys[(I + 1) % N]);
				const FVector2D A = (P - Prev).GetSafeNormal(), C = (Next - P).GetSafeNormal();
				const FVector2D Normal = (FVector2D(-A.Y, A.X) + FVector2D(-C.Y, C.X)).GetSafeNormal();
				const FVector2D Half = Normal * (Pc::Backdrop::PuddleRimPx * 0.5);
				Points.Add(P - Half);
				Points.Add(P + Half);
				Colors.Add(Rim);
				Colors.Add(Rim);
			}
			for (int32 I = 0; I < N; ++I)
			{
				const int32 A0 = RimBase + 2 * I, A1 = A0 + 1, B0 = RimBase + 2 * ((I + 1) % N), B1 = B0 + 1;
				Tris.Append({ A0, B0, A1, A1, B0, B1 });
			}
		}
		Out.SetBackdropFill(Points, Colors, Tris, BackdropFillMaterial);
	}
	if (Out.GetFloorTexture())
	{
		Out.AddFloor(LogicalWidth, LogicalHeight, 512.0 * Pc::Backdrop::TileScale);
	}
	else
	{
		Out.Add(EPawLayer::Floor, LogicalWidth * 0.5, LogicalHeight * 0.5, LogicalWidth, LogicalHeight, Paper);
	}
	for (int32 I = 0; I < static_cast<int32>(Backdrop.Props.size()); ++I)
	{
		const Pc::PcBackdropProp& P = Backdrop.Props[static_cast<size_t>(I)];
		const double W = FMath::Abs(P.W);
		FPawSprite S;
		S.Cell = BackdropCells.IsValidIndex(I) ? BackdropCells[I] : INDEX_NONE;
		S.X = P.X + W * 0.5; // a mirrored Rect2 keeps its footprint: the same place, the texture read right to left
		S.Y = P.Y + P.H * 0.5;
		S.Width = W;
		S.Height = P.H;
		S.bFlipX = P.W < 0.0;
		S.Order = I;
		Out.AddSprite(EPawLayer::Props, S);
	}
}

int32 APawRealmView::CellFor(const FString& Path) const
{
	const UPawSpriteAtlas* A = Renderer ? Renderer->GetAtlas() : nullptr;
	return A && !Path.IsEmpty() ? A->Find(Path) : INDEX_NONE;
}

void APawRealmView::Face(FMover& M, uint64 Serial, double X, double Dt)
{
	// SpriteLiveliness.face from the distance actually travelled this frame (Godot: direction x pace, the same sign and
	// speed). A slot re-used by a new mouse has no last position yet; its sprite keeps the pooled one's facing, as Godot's.
	if (M.bSeen && M.Serial == Serial && Dt > 0.0)
	{
		M.bFlip = PawLook::Face(M.bFlip, (X - M.LastX) / Dt);
	}
	M.Serial = Serial;
	M.LastX = X;
	M.bSeen = true;
}

void APawRealmView::TickLook(double RealDelta, bool bPaused)
{
	// A paused Godot tree runs no _process: nothing breathes, hops or twitches while the pause menu, a card pick or the tree
	// is up.
	LookDt = bPaused ? 0.0 : RealDelta;
	if (bPaused)
	{
		return;
	}
	LookClock += RealDelta;
	// Player._apply_hop: time left is spent first, then the frame is drawn from what remains.
	bHopNow = HopLeft > 0.0;
	if (bHopNow)
	{
		HopLeft -= RealDelta;
	}
	// Boss._tick_movement / _tick_twitch: the anger cools, the sprite jumps to a new offset every TWITCH_INTERVAL.
	const UPawRealmSubsystem* R = Realm();
	if (R && R->GetSim().Field.bBossUp)
	{
		Anger = FMath::Max(0.0, Anger - RealDelta * PawLook::AngerDecayPerSec);
		TwitchTimer -= RealDelta;
		if (TwitchTimer <= 0.0)
		{
			TwitchTimer = PawLook::TwitchInterval;
			const double Amount = FMath::Lerp(PawLook::TwitchIdlePx, PawLook::TwitchAngryPx, Anger);
			Twitch = FVector2D(LookRandom.FRandRange(-Amount, Amount), LookRandom.FRandRange(-Amount, Amount));
		}
	}
	if (BossDeath.Cell != INDEX_NONE)
	{
		BossDeath.Age += RealDelta;
		if (BossDeath.Age >= PawLook::DeathShakeSec + PawLook::DeathReturnSec + PawLook::DeathCollapseSec)
		{
			BossDeath.Cell = INDEX_NONE; // Boss._finish_death(): hidden
			Twitch = FVector2D::ZeroVector;
		}
	}
}

void APawRealmView::DrawCreatures()
{
	UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* Game = R ? R->GetGame() : nullptr;
	const UPawSpriteAtlas* Atlas = Renderer->GetAtlas();
	if (!Game || !Atlas)
	{
		return;
	}
	using namespace PawLook;
	const FPawRealmSim& Sim = R->GetSim();
	const Pc::PcField& F = Sim.Field;
	const Pc::PcGameData& D = Game->Data();
	const Pc::PcProgress& P = Game->Progress();
	UPawSpriteRenderer& Out = *Renderer;

	if (EnemyCells.Num() != static_cast<int32>(D.Enemies.size()))
	{
		EnemyCells.Reset();
		for (const Pc::PcEnemy& E : D.Enemies)
		{
			const int32 Cell = CellFor(UTF8_TO_TCHAR(E.TexturePath.c_str()));
			UE_CLOG(Cell == INDEX_NONE, LogPaw, Error, TEXT("Realm view: no atlas art for enemy %hs (%hs)"), E.Id.c_str(),
				E.TexturePath.c_str());
			EnemyCells.Add(Cell);
		}
	}
	const int32 ShadowCell = Atlas->Find(TEXT("ground_shadow"));
	const double ShadowAspect = ShadowCell != INDEX_NONE && Atlas->Cell(ShadowCell).NativeWidth > 0.0f
		? Atlas->Cell(ShadowCell).NativeHeight / Atlas->Cell(ShadowCell).NativeWidth : 0.375;
	auto AddShadow = [&](double X, double Y, double CreaturePx, bool bFlier)
	{
		const FShadow S = Shadow(CreaturePx, bFlier, ShadowAspect);
		FPawSprite Sp;
		Sp.Cell = ShadowCell;
		Sp.X = X;
		Sp.Y = Y + S.OffsetY;
		Sp.Width = S.Width;
		Sp.Height = S.Height;
		Sp.Tint = FLinearColor(1.0f, 1.0f, 1.0f, static_cast<float>(S.Alpha));
		Out.AddSprite(EPawLayer::Shadows, Sp);
	};

	// The mice (Enemy: a Sprite2D fitted to sprite_target_px, z 0, in pool order).
	if (Mice.Num() < static_cast<int32>(F.Kind.size()))
	{
		Mice.SetNum(static_cast<int32>(F.Kind.size()));
	}
	for (const int32 S : F.Live)
	{
		const size_t I = static_cast<size_t>(S);
		const int32 KindIndex = F.Kind[I];
		const Pc::PcEnemy& E = D.Enemies[static_cast<size_t>(KindIndex)];
		double X = 0.0, Y = 0.0;
		Sim.MousePos(S, X, Y);
		FMover& M = Mice[S];
		Face(M, F.Serial[I], X, LookDt);
		const double Size = E.SpriteTargetPx;
		const bool bFlier = E.Movement == Pc::EnemyMovementKind::FLIER;
		AddShadow(X, Y, Size, bFlier);
		// Enemy._apply_tint: data.tint x the shield's x the snare's, multiplied.
		FLinearColor Mod = TintOf(E.Tint);
		if (F.Shielded[I])
		{
			Mod = Mod * ShieldTint;
		}
		if (F.SlowMult[I] < 1.0)
		{
			Mod = Mod * SnareTint;
		}
		const double Phase = PhaseFor(F.Serial[I]);
		const double Height = Size * PulseY(LookClock, Phase);
		const double Order = PawLook::Order(ZMice, 1.0 + S);
		if (E.bIsSignature)
		{
			// The glow: a white silhouette (its shader ignores the modulate's colour, keeps its alpha) 1.24x, right behind.
			FPawSprite G;
			G.Cell = EnemyCells[KindIndex];
			G.X = X;
			G.Y = Y;
			G.Width = Size * GlowScale;
			G.Height = Height * GlowScale;
			G.bFlipX = M.bFlip;
			G.Whiten = 1.0f;
			G.Tint = FLinearColor(1.0f, 1.0f, 1.0f, Mod.A * static_cast<float>(GlowAlpha(LookClock, Phase)));
			G.Order = Order - 0.5;
			Out.AddSprite(EPawLayer::Overlays, G);
		}
		FPawSprite B;
		B.Cell = EnemyCells[KindIndex];
		B.X = X;
		B.Y = Y;
		B.Width = Size;
		B.Height = Height;
		B.bFlipX = M.bFlip;
		B.Tint = Modulate(Mod);
		B.Order = Order;
		Out.AddSprite(EPawLayer::Bodies, B);
	}

	// The kittens (Kitten: the kitten drawing at 96 px, z 1, gold and illusory tints multiplied, the Lone Hunter halo).
	const int32 KittenCell = Atlas->Find(TEXT("kitten_basic"));
	if (Kittens.Num() < static_cast<int32>(F.KActive.size()))
	{
		Kittens.SetNum(static_cast<int32>(F.KActive.size()));
	}
	for (size_t K = 0; K < F.KActive.size(); ++K)
	{
		if (!F.KActive[K])
		{
			Kittens[static_cast<int32>(K)].bSeen = false;
			continue;
		}
		double X = 0.0, Y = 0.0;
		Sim.KittenPos(static_cast<int32>(K), X, Y);
		FMover& M = Kittens[static_cast<int32>(K)];
		Face(M, 0, X, LookDt);
		AddShadow(X, Y, KittenPx, false);
		// The Lone Hunter's halo is an effect (FPawVfx::Draw): behind this kitten, over the mice.
		FLinearColor Mod = FLinearColor::White;
		if (F.KGolden[K])
		{
			Mod = Mod * GoldenKittenTint;
		}
		if (F.KIllusory[K])
		{
			Mod = Mod * IllusionTint;
		}
		FPawSprite B;
		B.Cell = KittenCell;
		B.X = X;
		B.Y = Y;
		B.Width = KittenPx;
		B.Height = KittenPx * PulseY(LookClock, PhaseFor(1000 + K));
		B.bFlipX = M.bFlip;
		B.Tint = Modulate(Mod);
		B.Order = PawLook::Order(ZKitten, static_cast<double>(K));
		Out.AddSprite(EPawLayer::Bodies, B);
	}

	// The boss (Boss: its drawing at its own size x 0.9, z 0 after the pool, the twitch on the sprite, the shield tint).
	if (F.bBossUp && F.TierIndex >= 0 && static_cast<size_t>(F.TierIndex) < D.Tiers.size())
	{
		if (const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(F.TierIndex)].BossData))
		{
			const int32 Cell = CellFor(UTF8_TO_TCHAR(Boss->TexturePath.c_str()));
			if (Cell != INDEX_NONE)
			{
				double X = 0.0, Y = 0.0;
				Sim.BossPos(X, Y);
				const double Size = Atlas->Cell(Cell).NativeWidth * BossScale;
				FPawSprite B;
				B.Cell = Cell;
				B.X = X + Twitch.X;
				B.Y = Y + Twitch.Y;
				B.Width = Size;
				B.Height = Size;
				B.Tint = Modulate(F.bBossShielded ? ShieldTint : FLinearColor::White);
				B.Order = PawLook::Order(ZMice, TreeSpan - 1.0);
				Out.AddSprite(EPawLayer::Bodies, B);
			}
		}
	}
	else if (BossDeath.Cell != INDEX_NONE)
	{
		// _play_death_sequence: six shudders (each tweened from the last offset), back to the centre, then the collapse.
		const double Step = DeathShakeSec / 6.0;
		const double Age = BossDeath.Age;
		FVector2D Offset = FVector2D::ZeroVector;
		double Scale = 1.0, Alpha = 1.0;
		if (Age < DeathShakeSec)
		{
			const int32 I = FMath::Min(5, static_cast<int32>(Age / Step));
			const FVector2D From = I == 0 ? DeathStart : BossDeath.Shake[I - 1];
			Offset = FMath::Lerp(From, BossDeath.Shake[I], (Age - I * Step) / Step);
		}
		else if (Age < DeathShakeSec + DeathReturnSec)
		{
			Offset = FMath::Lerp(BossDeath.Shake[5], FVector2D::ZeroVector, (Age - DeathShakeSec) / DeathReturnSec);
		}
		else
		{
			const double T = FMath::Clamp((Age - DeathShakeSec - DeathReturnSec) / DeathCollapseSec, 0.0, 1.0);
			Scale = 1.0 + (0.05 - 1.0) * BackEaseIn(T);
			Alpha = 1.0 - T;
		}
		FPawSprite B;
		B.Cell = BossDeath.Cell;
		B.X = BossDeath.X + Offset.X;
		B.Y = BossDeath.Y + Offset.Y;
		B.Width = BossDeath.Size * Scale;
		B.Height = BossDeath.Size * Scale;
		B.Tint = FLinearColor(1.0f, 1.0f, 1.0f, static_cast<float>(Alpha));
		B.Order = PawLook::Order(ZMice, TreeSpan - 1.0);
		Out.AddSprite(EPawLayer::Bodies, B);
	}

	// The adds (BossAdd: boss_add drawn RADIUS x 1.5 x pulse from the centre, z 3, in launch order).
	const int32 AddCell = Atlas->Find(TEXT("boss_add"));
	TSet<uint64> AddsNow;
	for (int32 A = 0; A < static_cast<int32>(F.Adds.size()); ++A)
	{
		const Pc::PcBossAdd& Add = F.Adds[static_cast<size_t>(A)];
		AddsNow.Add(Add.Serial);
		const double Born = AddBorn.FindOrAdd(Add.Serial, LookClock);
		const double Diameter = AddRadius * 1.5 * (0.85 + 0.15 * FMath::Sin((LookClock - Born) * 6.0)) * 2.0;
		FPawSprite B;
		B.Cell = AddCell;
		B.X = Add.X;
		B.Y = Add.Y;
		B.Width = Diameter;
		B.Height = Diameter;
		B.Order = PawLook::Order(ZAdd, 1.0 + A);
		Out.AddSprite(EPawLayer::Bodies, B);
	}
	for (auto It = AddBorn.CreateIterator(); It; ++It)
	{
		if (!AddsNow.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}

	// The cat (Player: its visual stage's drawing at 175 px, z 2; the weapon at +1 and the hat at +2, children of the sprite:
	// they scale with it but do not mirror. The hop lifts and squashes the sprite; the shadow stays on the floor).
	double CatX = 0.0, CatY = 0.0;
	Sim.CatPos(CatX, CatY);
	bCatFlip = PawLook::Face(bCatFlip, LookDt > 0.0 ? StickVelX : 0.0);
	AddShadow(CatX, CatY, CatPx, false);
	const int64 Stage = Pc::VisualStage(D, P);
	const Pc::PcEvolution* Evo = nullptr;
	for (const Pc::PcEvolution& E : D.Evolutions)
	{
		if (E.Stage == Stage)
		{
			Evo = &E;
		}
	}
	const int32 CatCell = Evo ? CellFor(UTF8_TO_TCHAR(Evo->TexturePath.c_str())) : INDEX_NONE;
	const FHop H = Hop(HopLeft, bHopNow);
	const double RelX = H.ScaleX;
	const double RelY = PulseY(LookClock, PhaseFor(0)) * H.ScaleY;
	const double BodyY = CatY - H.Lift;
	FPawSprite Cat;
	Cat.Cell = CatCell;
	Cat.X = CatX;
	Cat.Y = BodyY;
	Cat.Width = CatPx * RelX;
	Cat.Height = CatPx * RelY;
	Cat.bFlipX = bCatFlip;
	Cat.Tint = Modulate(Evo ? TintOf(Evo->Tint) : FLinearColor::White);
	Cat.Order = PawLook::Order(ZCat, 0.0);
	Out.AddSprite(EPawLayer::Bodies, Cat);
	const bool bMounted = Stage >= MountedFromStage;
	// The weapon: player_weapon_stage_<visual stage> (none for the bare paw), always mirrored (it is drawn leaning right).
	const int32 WeaponCell = CellFor(FString::Printf(TEXT("player_weapon_stage_%lld"), Stage));
	if (WeaponCell != INDEX_NONE)
	{
		const FOverlay O = CatOverlay(false, bMounted, RelX, RelY);
		FPawSprite W;
		W.Cell = WeaponCell;
		W.X = CatX + O.X;
		W.Y = BodyY + O.Y;
		W.Width = O.Width;
		W.Height = O.Height;
		W.bFlipX = true;
		W.Tint = Cat.Tint;
		W.Order = PawLook::Order(ZCat + 1, 0.0);
		Out.AddSprite(EPawLayer::Overlays, W);
	}
	const Pc::PcJob* Job = Pc::EquippedJob(D, P);
	const int32 HatCell = Job ? CellFor(UTF8_TO_TCHAR(Job->HatPath.c_str())) : INDEX_NONE;
	if (HatCell != INDEX_NONE)
	{
		const FOverlay O = CatOverlay(true, bMounted, RelX, RelY);
		FPawSprite Hat;
		Hat.Cell = HatCell;
		Hat.X = CatX + O.X;
		Hat.Y = BodyY + O.Y;
		Hat.Width = O.Width;
		Hat.Height = O.Height;
		Hat.Tint = Cat.Tint;
		Hat.Order = PawLook::Order(ZCat + 2, 0.0);
		Out.AddSprite(EPawLayer::Overlays, Hat);
	}
}
