#include "Realm/PawVfx.h"

#include "PcBackdrop.h"
#include "PcBackdropTones.h"
#include "PcCards.h"
#include "PcData.h"
#include "PcFieldRules.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "Realm/PawFieldLook.h"
#include "Realm/PawFxLook.h"
#include "Realm/PawRealmSim.h"
#include "Render/PawSpriteRenderer.h"

namespace
{
	using FColorG = FLinearColor; // a Godot Color: sRGB floats, straight alpha
	constexpr double Tau = UE_DOUBLE_TWO_PI;
	const FColorG InkColor(Pc::Ink.R / 255.0f, Pc::Ink.G / 255.0f, Pc::Ink.B / 255.0f, 1.0f); // the palette's INK (PcBackdropTones.h)

	// tier_controller.gd
	const FColorG ExecuteColor(0.78f, 0.09f, 0.12f, 0.95f);
	const FColorG PiedPiperColor(0.84f, 0.62f, 0.46f, 0.85f);
	const FColorG HammerfallColor(1.0f, 0.62f, 0.48f, 0.9f);
	constexpr double AnvilEchoReach = 1.35;
	const FColorG ZoneTrail(0.55f, 0.82f, 1.0f, 1.0f);
	const FColorG ZoneCrater(1.0f, 0.52f, 0.24f, 1.0f);
	const FColorG ZoneEcho(0.62f, 0.88f, 0.94f, 1.0f);
	constexpr double DragTetherSec = 0.35;
	const FColorG DragTetherColor(0.98f, 0.80f, 0.52f, 0.65f);
	constexpr double RushPulseSec = 0.55;
	constexpr double RushVisibleAt = 0.15;
	constexpr double EdgeBurnBandPx = 90.0;
	constexpr double PrismBeamDrawInterval = 0.12;
	const FColorG PrismColor(0.95f, 0.75f, 1.0f, 0.95f);
	constexpr int32 MaxSplashBeamsPerFrame = 3;
	const FColorG OverkillSplashColor(1.0f, 0.55f, 0.42f, 0.85f);
	constexpr double BonusPopGapSec = 0.070;
	const FColorG PopBounty(1.0f, 0.84f, 0.3f, 1.0f);
	const FColorG PopToyed(1.0f, 0.5f, 0.75f, 1.0f);
	const FColorG PopSpectrum(0.45f, 0.95f, 0.45f, 1.0f);
	const FColorG GibletsRed(0.62f, 0.04f, 0.06f, 0.92f);
	const FColorG GibletsChunk(0.42f, 0.02f, 0.05f, 1.0f);
	constexpr double GibletsGapSec = 0.060;
	// vfx_burst.gd
	constexpr int32 BurstSegments = 48;
	// vfx_slam.gd
	constexpr double SlamDuration = 0.42, SlamSquash = 0.45, SlamRingWidth = 7.0, SlamInkEdge = 4.0;
	constexpr int32 SlamCracks = 7, SlamCrackJoints = 3;
	const FColorG SlamRingColor(1.0f, 0.92f, 0.62f, 1.0f);
	const FColorG SlamDustColor(0.78f, 0.68f, 0.52f, 0.9f);
	// vfx_swipe.gd
	constexpr int32 SwipeSegments = 20;
	constexpr double ArcSpan = 1.55, ReachFraction = 0.88, SwipeWidth = 9.0, SwipeDuration = 0.24, SwipeInkEdge = 4.0;
	constexpr int32 ClawCount = 3;
	constexpr double ClawLength = 26.0, ClawGap = 9.0, ClawWidth = 4.5, ClawAt = 0.35;
	const FColorG SwipeColor(0.996f, 0.965f, 0.918f, 1.0f);
	// Restyle D3 (2026-11-09): crits and earned crits in Butter (PawFxLook::Crit), not Godot's pinks (1, 0.42, 0.72, 0.95)
	// and (1, 0.62, 0.86): pink is the reward colour. The widths are Godot's: an earned crit stays the widest.
	constexpr double CritWidthScale = 1.5;
	constexpr double EarnedWidthScale = 2.0;
	// vfx_beam.gd
	constexpr double TipFraction = 0.28, MinLengthPx = 1.0;
	// aura_ring.gd
	constexpr int32 AuraSegments = 64;
	// Restyle D3 (2026-11-09): the plain aura is PawFxLook's (a white lift, a Bone ring with a Plum edge), not Godot's gold
	// ring (0.965, 0.761, 0.361, 0.72) 2.5 px and its 14 % fill. The Unicorn's rainbow keeps Godot's look.
	constexpr double RainbowRingWidth = 2.5;
	constexpr double RainbowHueCycle = 0.12, RainbowSaturation = 0.52, RainbowRingAlpha = 0.68, RainbowFillAlpha = 0.14;
	constexpr double RainbowArcSpan = 0.55;
	constexpr int32 RainbowArcCount = 12;
	constexpr double AuraPollSec = 0.15;
	// poop_zone_visual.gd
	constexpr int32 PoopSegments = 48, PoopBands = 5;
	constexpr double PoopHueCycle = 0.5, PoopRingWidth = 4.0;
	// damage_zone_field.gd
	constexpr int32 ZoneBands = 3, ZoneSegments = 28;
	constexpr double ZoneRingWidth = 3.0;
	// boss_telegraph.gd
	constexpr int32 TelegraphSegments = 72;
	constexpr double TelegraphRadius = 104.0, TelegraphWidth = 7.0, WarningSec = 0.8, PulseMinHz = 3.0, PulseMaxHz = 11.0;
	const FColorG OpenColor(1.0f, 0.82f, 0.32f, 1.0f);
	const FColorG ShieldedColor(0.45f, 0.85f, 1.0f, 1.0f);
	const FColorG TrackColor(1.0f, 1.0f, 1.0f, 0.10f);
	// golden_sparkle.gd
	constexpr int32 MoteCount = 5, HaloSegments = 24, TrailMax = 14;
	constexpr double SparkleHalo = 26.0, HaloBreathe = 4.0, OrbitMin = 16.0, OrbitMax = 30.0, MoteRadius = 2.6;
	constexpr double TrailInterval = 0.045, TrailLifetime = 0.55;
	const FColorG GoldCore(1.0f, 0.90f, 0.45f, 1.0f);
	const FColorG GoldEdge(1.0f, 0.72f, 0.18f, 1.0f);
	// vfx_edge_fire.gd
	constexpr double EdgeAlpha = 0.30, BreathPerSec = 0.55, BreathDepth = 0.22, EmberDriftPerSec = 18.0;
	constexpr int32 EmberCount = 26;
	const FColorG FireColor(1.0f, 0.584f, 0.482f, 1.0f);
	// player.gd JOB_VFX_COLORS
	FColorG JobColor(const std::string& Id)
	{
		if (Id == "town_crier") return FColorG(1.0f, 0.84f, 0.31f, 0.85f);
		if (Id == "blacksmith") return FColorG(1.0f, 0.45f, 0.20f, 0.85f);
		if (Id == "daredevil") return FColorG(1.0f, 0.30f, 0.30f, 0.85f);
		if (Id == "magician") return FColorG(0.78f, 0.45f, 1.0f, 0.85f);
		if (Id == "engineer") return FColorG(1.0f, 0.87f, 0.20f, 0.85f);
		if (Id == "unemployed") return FColorG(0.65f, 0.65f, 0.70f, 0.85f);
		return FColorG(1.0f, 0.9f, 0.5f, 0.8f);
	}

	FColorG WithAlpha(FColorG C, double A)
	{
		C.A = static_cast<float>(A);
		return C;
	}
	FColorG Scaled(FColorG C, double K)
	{
		C.A = static_cast<float>(C.A * K);
		return C;
	}
	// PawFxLook's strike paw at Tip, the drawing's up (its toes) turned to Motion, at opacity Alpha.
	void DrawStrikePaw(FPawFxCanvas& Out, const FVector2D& Tip, const FVector2D& Motion, double Alpha)
	{
		using namespace PawFxLook;
		const FVector2D Up = Motion.GetSafeNormal();
		if (Alpha <= 0.0 || Up.IsNearlyZero())
		{
			return;
		}
		// The rotation taking (0, -1) to Up (screen y down): x' = c x - s y, y' = s x + c y with s = Up.X, c = -Up.Y.
		const double Sn = Up.X, Cs = -Up.Y;
		const double K = StrikePawPx / StrikePawBox, Half = StrikePawBox * 0.5;
		TArray<FVector2D, TInlineAllocator<32>> Points;
		for (const FStrikePawPart& Part : StrikePawParts)
		{
			Points.Reset();
			for (int32 I = 0; I < Part.Count; ++I)
			{
				const double X = (StrikePawPoints[Part.First + I][0] - Half) * K, Y = (StrikePawPoints[Part.First + I][1] - Half) * K;
				Points.Add(Tip + FVector2D(Cs * X - Sn * Y, Sn * X + Cs * Y));
			}
			const FColorG Colour(Part.R / 255.0f, Part.G / 255.0f, Part.B / 255.0f, static_cast<float>(Alpha));
			if (Part.bOutline)
			{
				const FVector2D Close = Points[0]; // a copy: TArray refuses to add one of its own elements
				Points.Add(Close);
				Out.Polyline(Points, Colour, StrikePawStroke * K);
			}
			else
			{
				Out.Polygon(Points, Colour);
			}
		}
	}

	FVector2D Rotate(const FVector2D& V, double A)
	{
		const double C = FMath::Cos(A), S = FMath::Sin(A);
		return FVector2D(V.X * C - V.Y * S, V.X * S + V.Y * C);
	}
	double EaseOutCubic(double T)
	{
		const double U = 1.0 - T;
		return 1.0 - U * U * U;
	}
	double Fposmod(double X, double M)
	{
		const double R = FMath::Fmod(X, M);
		return R < 0.0 ? R + M : R;
	}
}

bool FPawVfx::Room(const FContext& C) const
{
	return C.Budget > 0.0 && Effects.Num() < MaxLive;
}

FPawVfx::FFx& FPawVfx::Add(EKind Kind)
{
	FFx& E = Effects.AddDefaulted_GetRef();
	E.Kind = Kind;
	E.Serial = NextSerial++;
	++SpawnedCount;
	return E;
}

int32 FPawVfx::LiveOf(EKind Kind) const
{
	int32 N = 0;
	for (const FFx& E : Effects)
	{
		N += E.Kind == Kind ? 1 : 0;
	}
	return N;
}

void FPawVfx::Clear()
{
	Effects.Reset();
	Delayed.Reset();
	SparkleState.Reset();
	PoopLeft = 0.0;
}

bool FPawVfx::Burst(const FContext& C, const FVector2D& At, const FLinearColor& Color, double EndRadius, double Duration, double Width,
	bool bOnCat)
{
	if (!Room(C))
	{
		return false;
	}
	FFx& E = Add(EKind::Burst);
	E.At = At;
	E.Color = Color;
	E.Radius = EndRadius;
	E.Life = FMath::Max(Duration, 1e-3);
	E.Width = Width;
	E.bA = bOnCat;
	return true;
}

bool FPawVfx::Slam(const FContext& C, const FVector2D& At, double Radius, bool bQuiet)
{
	if (!Room(C))
	{
		return false;
	}
	FFx& E = Add(EKind::Slam);
	E.At = At;
	E.Radius = Radius;
	E.Life = SlamDuration;
	E.bA = bQuiet;
	// _build_cracks: seven jagged lines, seeded by where the slam landed (Godot seeds by hash(global_position); the port by
	// the position's bits: the same place always cracks the same way, though not in Godot's exact shape).
	FRandomStream R(static_cast<int32>(GetTypeHash(At.X) * 31u + GetTypeHash(At.Y)));
	for (int32 I = 0; I < SlamCracks; ++I)
	{
		const double Angle = Tau * (I + R.FRandRange(-0.3f, 0.3f)) / SlamCracks;
		TArray<FVector2D>& Line = E.Cracks.AddDefaulted_GetRef();
		Line.Add(FVector2D::ZeroVector);
		FVector2D Pos = FVector2D::ZeroVector;
		const double Reach = R.FRandRange(0.45f, 0.8f);
		for (int32 J = 0; J < SlamCrackJoints; ++J)
		{
			const double Wobble = Angle + R.FRandRange(-0.45f, 0.45f);
			Pos += FVector2D(FMath::Cos(Wobble), FMath::Sin(Wobble) * SlamSquash) * (Reach / SlamCrackJoints);
			Line.Add(Pos);
		}
	}
	if (!bQuiet)
	{
		Shards(C, At, SlamDustColor, 14, Radius * 2.2, SlamDuration + 0.15, 900.0, 7.0);
	}
	return true;
}

bool FPawVfx::Swipe(const FContext& C, const FVector2D& Origin, const FVector2D& Target, double Reach, bool bCrit, bool bEarned)
{
	if (!Room(C))
	{
		return false;
	}
	FFx& E = Add(EKind::Swipe);
	const FVector2D To = Target - Origin;
	E.At = Origin;
	E.Angle = To.Size() > 0.001 ? FMath::Atan2(To.Y, To.X) : 0.0;
	E.Radius = FMath::Max(Reach, 8.0) * ReachFraction;
	E.Dist = To.Size();
	E.bA = bCrit;
	E.bB = bCrit && bEarned;
	E.Life = SwipeDuration;
	return true;
}

bool FPawVfx::Beam(const FContext& C, const FVector2D& From, const FVector2D& To, const FLinearColor& Color, double Width, double Duration)
{
	if (!Room(C))
	{
		return false;
	}
	FFx& E = Add(EKind::Beam);
	E.At = From;
	E.To = To;
	E.Color = Color;
	E.Width = Width;
	E.Life = FMath::Max(Duration, 1e-3);
	return true;
}

bool FPawVfx::Shards(const FContext& C, const FVector2D& At, const FLinearColor& Color, int32 Count, double Speed, double Duration,
	double Gravity, double Size, bool bTwinkle)
{
	if (!Room(C))
	{
		return false;
	}
	Count = FMath::Max(1, FMath::RoundToInt(Count * C.Budget));
	FFx& E = Add(EKind::Shards);
	E.At = At;
	E.Color = Color;
	E.Life = FMath::Max(Duration, 0.01);
	E.Gravity = Gravity;
	E.Size = Size;
	E.bA = bTwinkle;
	// rng.randomize(): every spray its own.
	for (int32 I = 0; I < Count; ++I)
	{
		const double Base = Tau * I / Count;
		const double Angle = Base + Random.FRandRange(-0.25f, 0.25f);
		FShard S;
		S.Dir = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		S.Speed = Speed * Random.FRandRange(0.55f, 1.0f);
		S.Spin = Random.FRandRange(-6.0f, 6.0f);
		S.Scale = Random.FRandRange(0.7f, 1.3f);
		S.Phase = Random.FRand() * Tau;
		E.Pieces.Add(S);
	}
	return true;
}

void FPawVfx::SeedEmbers(double Budget)
{
	// _seed_embers: Godot's own generator from its fixed seed, so the embers stand where Godot's do.
	Pc::PcGodotRng Rng(57454);
	Embers.Reset();
	const int32 Wanted = FMath::Max(1, FMath::RoundToInt(EmberCount * FMath::Max(Budget, 0.0)));
	for (int32 I = 0; I < Wanted; ++I)
	{
		FEmber E;
		E.T = Rng.Randf();
		E.Depth = Rng.Randf();
		E.Size = Rng.RandfRange(1.6f, 3.4f);
		E.Phase = static_cast<double>(Rng.Randf()) * Tau;
		Embers.Add(E);
	}
}

void FPawVfx::OnFrameEvents(const FContext& C)
{
	using K = Pc::PcFieldEventKind;
	namespace Flag = Pc::FieldFlag;
	const Pc::PcGameData& D = *C.D;
	const Pc::PcProgress& P = *C.P;
	const Pc::PcField& F = C.Sim->Field;
	int32 SplashBudget = MaxSplashBeamsPerFrame;
	bool bPrismChainDrawn = false;
	FVector2D PrismChainStart;
	bool bPrismDrawing = false;
	for (const Pc::PcFieldEvent& E : C.Sim->FrameEvents)
	{
		const FVector2D At(E.X, E.Y), To(E.ToX, E.ToY);
		switch (E.Kind)
		{
		case K::Spawned:
			if (Pc::PiedPiperRadius(D, P) > 0.0)
			{
				Burst(C, At, PiedPiperColor, 34.0, 0.30, 2.5);
			}
			break;
		case K::Strike:
			if (E.Flags & Flag::Boss)
			{
				Swipe(C, At, To, D.Combat.BasePawReach, (E.Flags & Flag::Crit) != 0, (E.Flags & Flag::Earned) != 0);
				break;
			}
			if (E.Flags & Flag::Riddle)
			{
				Burst(C, To, ExecuteColor, 52.0, 0.3, 4.0);
			}
			if (E.Flags & Flag::Threshold)
			{
				Shards(C, To, ExecuteColor, 10, 260.0, 0.45, 620.0, 9.0, false);
				Burst(C, To, ExecuteColor, 46.0, 0.28, 4.0);
			}
			Swipe(C, At, To, D.Combat.BasePawReach * ((E.Flags & Flag::Echo) ? AnvilEchoReach : 1.0), (E.Flags & Flag::Crit) != 0,
				(E.Flags & Flag::Earned) != 0);
			break;
		case K::Cleave:
			Beam(C, At, To, HammerfallColor, 9.0);
			break;
		case K::Splash:
			if (SplashBudget > 0 && C.Budget > 0.0)
			{
				--SplashBudget;
				Beam(C, At, To, OverkillSplashColor, 4.0);
			}
			break;
		case K::PrismBeam:
			// _tick_prism draws one step's chain every PRISM_BEAM_DRAW_INTERVAL: the first chain of a frame whose cooldown
			// has run out (a chain starts again where the cat is).
			if (!bPrismChainDrawn && !bPrismDrawing && PrismCooldown <= 0.0)
			{
				bPrismDrawing = true;
				PrismChainStart = At;
				PrismCooldown = PrismBeamDrawInterval;
			}
			else if (bPrismDrawing && At.Equals(PrismChainStart, 1e-9))
			{
				bPrismDrawing = false;
				bPrismChainDrawn = true;
			}
			if (bPrismDrawing)
			{
				Beam(C, At, To, PrismColor, 7.0);
			}
			break;
		case K::PrismBurst:
			Burst(C, At, PrismColor, E.Value, 0.45, 5.0);
			break;
		case K::AuraPulse:
			Burst(C, At, FColorG(0.55f, 0.82f, 1.0f, 0.9f), E.Value, 0.4, 4.0);
			break;
		case K::Pounce:
		{
			const bool bAuto = (E.Flags & Flag::Auto) != 0;
			// Player.lock_after_pounce: the Cone of Shame's ring, as long as the root, round the cat (F39 fixed: D16).
			const double Lock = Pc::JobPounceLockSec(D, P);
			if (Lock > 0.0)
			{
				Burst(C, C.Cat, FColorG(0.85f, 0.25f, 0.25f, 0.55f), 70.0, Lock, 2.0, true);
			}
			Burst(C, At, FColorG(1.0f, 0.85f, 0.4f, bAuto ? 0.45f : 0.9f), E.Value, 0.28, 4.0);
			Slam(C, At, E.Value, bAuto);
			break;
		}
		case K::Killed:
		{
			if (E.Flags & Flag::Golden)
			{
				Burst(C, At, FColorG(1.0f, 0.88f, 0.35f, 1.0f), 130.0, 0.55, 5.0);
				Burst(C, At, FColorG(1.0f, 0.72f, 0.18f, 0.8f), 78.0, 0.4, 3.0);
				Shards(C, At, FColorG(1.0f, 0.86f, 0.32f, 1.0f), 26, 340.0, 1.10, 0.0, 16.0, true);
			}
			const bool bSignature = (E.Flags & Flag::Signature) != 0;
			Burst(C, At, bSignature ? FColorG(1.0f, 0.82f, 0.35f, 0.95f) : FColorG(0.85f, 0.88f, 1.0f, 0.8f), bSignature ? 34.0 : 20.0,
				bSignature ? 0.3 : 0.22, 2.5);
			auto Pop = [&](const TCHAR* Kind, const FColorG& Colour)
			{
				double& Last = PopLast.FindOrAdd(Kind, -1000.0);
				if (Clock - Last < BonusPopGapSec)
				{
					return;
				}
				Last = Clock;
				Shards(C, At, Colour, 7, 190.0, 0.45, 0.0, 6.0, true);
			};
			if (E.Flags & Flag::Bounty)
			{
				Pop(TEXT("bounty"), PopBounty);
			}
			if (E.Flags & Flag::Toyed)
			{
				Pop(TEXT("toyed"), PopToyed);
			}
			if ((E.Flags & Flag::Beam) && Pc::PrismKillMultiplier(D, P) > 1.0)
			{
				Pop(TEXT("spectrum"), PopSpectrum);
			}
			break;
		}
		case K::CatnipBomb:
			Burst(C, At, FColorG(0.55f, 1.0f, 0.45f, 0.95f), E.Value, 0.45, 5.0);
			Shards(C, At, FColorG(0.45f, 0.95f, 0.38f, 0.95f), 16, E.Value * 1.6, 0.6, 480.0, 11.0, false);
			break;
		case K::KittenBlast:
			Burst(C, At, GibletsRed, E.Value, 0.3, 4.0);
			if (Clock - GibletsLast >= GibletsGapSec)
			{
				GibletsLast = Clock;
				Shards(C, At, GibletsRed, 14, E.Value * 2.4, 0.55, 1100.0, 6.0);
				Shards(C, At, GibletsChunk, 4, E.Value * 1.6, 0.6, 1500.0, 11.0);
			}
			break;
		case K::PoopDropped:
			PoopAt = At;
			PoopRadius = E.Value;
			PoopDuration = FMath::Max(E.ToX, 0.01);
			PoopLeft = PoopDuration;
			PoopTime = 0.0;
			break;
		case K::AddSpawned:
			Burst(C, At, FColorG(0.75f, 0.45f, 1.0f, 0.9f), 70.0, 0.35, 3.0);
			break;
		case K::AddIntercepted:
			Burst(C, At, FColorG(0.85f, 0.62f, 1.0f, 0.95f), 60.0, 0.3, 3.0);
			break;
		case K::BossHealed:
			if (F.bBossUp)
			{
				Burst(C, At, FColorG(0.55f, 1.0f, 0.6f, 0.95f), 200.0, 0.5, 6.0);
			}
			break;
		case K::BossWindow:
		{
			const bool bNowVulnerable = (E.Flags & Flag::Shielded) == 0;
			Burst(C, At, bNowVulnerable ? FColorG(1.0f, 0.82f, 0.32f, 0.9f) : FColorG(0.45f, 0.85f, 1.0f, 0.75f), 150.0, 0.32, 5.0);
			break;
		}
		case K::BossPhase:
			Burst(C, At, FColorG(0.95f, 0.4f, 0.45f, 0.95f), 260.0, 0.5, 5.0);
			break;
		case K::BossDefeated:
		{
			// _play_boss_death_vfx: three rings, 0.12 s apart, each wider.
			const FColorG Colours[3] = { FColorG(1.0f, 0.95f, 0.8f, 0.95f), FColorG(1.0f, 0.7f, 0.3f, 0.85f), FColorG(0.9f, 0.4f, 0.7f, 0.7f) };
			for (int32 I = 0; I < 3; ++I)
			{
				Delayed.Add({ Clock + 0.12 * I, At, Colours[I], 180.0 + 170.0 * I });
			}
			break;
		}
		default:
			break;
		}
	}
}

void FPawVfx::Tick(const FContext& C)
{
	const Pc::PcGameData& D = *C.D;
	const Pc::PcProgress& P = *C.P;
	const Pc::PcField& F = C.Sim->Field;
	const double Dt = C.Dt;
	// The job ring (player.gd _on_job_taken): whenever the job worn changes.
	const Pc::PcJob* Job = Pc::EquippedJob(D, P);
	const std::string JobId = Job ? Job->Id : std::string();
	if (bJobKnown && JobId != LastJob && !JobId.empty())
	{
		Burst(C, C.Cat, JobColor(JobId), 96.0, 0.30, 3.0, true);
	}
	LastJob = JobId;
	bJobKnown = true;
	if (EdgeTier != F.TierIndex)
	{
		// VfxEdgeFire._ready: each biome's scene seeds its embers once, at the effects budget of that moment.
		EdgeTier = F.TierIndex;
		SeedEmbers(C.Budget);
	}
	if (Dt <= 0.0)
	{
		return;
	}
	Clock += Dt;
	for (int32 I = Effects.Num() - 1; I >= 0; --I)
	{
		Effects[I].Age += Dt;
		if (Effects[I].Age >= Effects[I].Life)
		{
			Effects.RemoveAt(I, EAllowShrinking::No); // order kept: it is the draw order
		}
	}
	for (int32 I = Delayed.Num() - 1; I >= 0; --I)
	{
		if (Clock >= Delayed[I].At)
		{
			Burst(C, Delayed[I].Where, Delayed[I].Color, Delayed[I].Radius, 0.55, 6.0);
			Delayed.RemoveAt(I);
		}
	}
	PrismCooldown -= Dt;

	// _tick_spawn_foresight: a slow pulse where the next special arrives.
	if (F.bForeseen && Pc::SpawnForesight(D, P) && F.ForeseenKind >= 0 && static_cast<size_t>(F.ForeseenKind) < D.Enemies.size())
	{
		const Pc::PcEnemy& Next = D.Enemies[static_cast<size_t>(F.ForeseenKind)];
		if (Next.bIsSignature || Next.bSparkle)
		{
			ForesightPulse -= Dt;
			if (ForesightPulse <= 0.0)
			{
				ForesightPulse = 0.5;
				Burst(C, FVector2D(F.ForeseenX, F.ForeseenY), FColorG(1.0f, 0.82f, 0.35f, 0.75f), 30.0, 0.45, 3.0);
			}
		}
	}
	// _tick_kitten_drag: a tether from each kitten to its quarry, every DRAG_TETHER_SEC.
	if (F.KDragSpeed > 0.0)
	{
		DragTether -= Dt;
		if (DragTether <= 0.0)
		{
			DragTether = DragTetherSec;
			for (int32 K = 0; K < static_cast<int32>(F.KActive.size()); ++K)
			{
				const int32 T = F.KTarget[K];
				if (!F.KActive[K] || T < 0 || static_cast<size_t>(T) >= F.Active.size() || !F.Active[T] || F.Serial[T] != F.KTargetSerial[K])
				{
					continue;
				}
				double KX = 0.0, KY = 0.0, MX = 0.0, MY = 0.0;
				C.Sim->KittenPos(K, KX, KY);
				C.Sim->MousePos(T, MX, MY);
				Beam(C, FVector2D(KX, KY), FVector2D(MX, MY), DragTetherColor, 3.5);
			}
		}
	}
	// _tick_infestation_pulse: the rush's ring round the cat, faster as it builds.
	const double Rush = P.InfestationBonus;
	if (Rush < RushVisibleAt)
	{
		RushPulse = 0.0;
	}
	else if ((RushPulse -= Dt) <= 0.0)
	{
		RushPulse = FMath::Max(RushPulseSec / (1.0 + Rush), 0.18);
		const double Strength = D.Code.InfestationCeiling > 0.0 ? FMath::Clamp(Rush / D.Code.InfestationCeiling, 0.0, 1.0) : 0.0;
		Burst(C, C.Cat, FColorG(0.86f, 0.60f, 0.40f, static_cast<float>(0.30 + 0.45 * Strength)), 90.0 + 60.0 * Strength, 0.45, 3.0);
	}

	// AuraRing: polled, the rainbow's clock always running.
	AuraTime += Dt;
	AuraPoll -= Dt;
	if (AuraPoll <= 0.0)
	{
		AuraPoll = AuraPollSec;
		AuraRadius = Pc::AuraRadius(D, P);
		bAuraRainbow = Pc::IsUnicornOwned(P);
		bAuraOwned = Pc::IsAuraOwned(P);
	}
	if (PoopLeft > 0.0)
	{
		PoopTime += Dt;
		PoopLeft -= Dt;
	}
	TelegraphTime += Dt;
	EdgeTime += Dt;
	bEdgeFire = Pc::CardEffectValue(D, P, Pc::CardEffect::EDGE_BURN) > 0.0 && C.Budget > 0.0;

	// GoldenSparkle, one per golden mouse on the field (set_active on each activation: a new life starts afresh).
	TSet<int32> Golden;
	for (const int32 S : F.Live)
	{
		if (!D.Enemies[static_cast<size_t>(F.Kind[S])].bSparkle)
		{
			continue;
		}
		Golden.Add(S);
		FSparkle& Sp = SparkleState.FindOrAdd(S);
		if (!Sp.bSeen || Sp.Serial != F.Serial[S])
		{
			Sp = FSparkle();
			Sp.Serial = F.Serial[S];
			Sp.bSeen = true;
		}
		double X = 0.0, Y = 0.0;
		C.Sim->MousePos(S, X, Y);
		Sp.Time += Dt;
		for (int32 I = Sp.Trail.Num() - 1; I >= 0; --I)
		{
			Sp.Trail[I].Age += Dt;
			if (Sp.Trail[I].Age >= TrailLifetime)
			{
				Sp.Trail.RemoveAt(I);
			}
		}
		Sp.TrailTimer -= Dt;
		if (Sp.TrailTimer <= 0.0)
		{
			Sp.TrailTimer = TrailInterval;
			FSparkle::FFleck Fl;
			Fl.Pos = FVector2D(X + Random.FRandRange(-6.0f, 6.0f), Y + Random.FRandRange(-6.0f, 6.0f));
			Fl.Size = Random.FRandRange(1.4f, 2.8f);
			Sp.Trail.Add(Fl);
			if (Sp.Trail.Num() > TrailMax)
			{
				Sp.Trail.RemoveAt(0);
			}
		}
	}
	for (auto It = SparkleState.CreateIterator(); It; ++It)
	{
		if (!Golden.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

void FPawVfx::DrawEffect(const FFx& E, const FContext& C, FPawFxCanvas& Out) const
{
	const double T = FMath::Clamp(E.Age / E.Life, 0.0, 1.0);
	switch (E.Kind)
	{
	case EKind::Burst:
	{
		// TRANS_CUBIC / EASE_OUT on both the radius (0 -> end) and the alpha (1 -> 0).
		const double Ease = EaseOutCubic(T);
		const double R = E.Radius * Ease;
		if (R <= 0.0)
		{
			return;
		}
		// A ring on the cat is drawn round the cat where it is now: Godot's added its spawn point to the cat's position, so it
		// drew at twice the cat's position (F39, a Godot fault the port does not copy: D16).
		const FVector2D Centre = E.bA ? C.Cat : E.At;
		Out.Arc(Centre, R, 0.0, Tau, BurstSegments, Scaled(E.Color, 1.0 - Ease), E.Width);
		return;
	}
	case EKind::Slam:
	{
		const double Fade = 1.0 - T;
		const double BaseAlpha = E.bA ? 0.55 : 1.0;
		const double Grow = 1.0 - FMath::Pow(1.0 - T, 3.0);
		const double R = E.Radius * (0.25 + 0.75 * Grow);
		TArray<FVector2D, TInlineAllocator<49>> Ring;
		for (int32 I = 0; I < 49; ++I)
		{
			const double A = Tau * I / 48.0;
			Ring.Add(E.At + FVector2D(FMath::Cos(A) * R, FMath::Sin(A) * R * SlamSquash));
		}
		const double W = SlamRingWidth * (1.0 - 0.6 * T);
		Out.Polyline(Ring, WithAlpha(InkColor, 0.8 * Fade * BaseAlpha), W + SlamInkEdge * 2.0);
		Out.Polyline(Ring, WithAlpha(SlamRingColor, Fade * BaseAlpha), W);
		const double Reach = FMath::Clamp(T / 0.2, 0.0, 1.0);
		for (const TArray<FVector2D>& Crack : E.Cracks)
		{
			TArray<FVector2D, TInlineAllocator<8>> Pts;
			const int32 N = Crack.Num();
			const double Upto = 1.0 + (N - 1) * Reach;
			for (int32 I = 0; I < N && I <= Upto; ++I)
			{
				Pts.Add(E.At + Crack[I] * E.Radius);
			}
			if (Pts.Num() >= 2)
			{
				Out.Polyline(Pts, WithAlpha(InkColor, 0.85 * Fade * BaseAlpha), 3.5);
			}
		}
		return;
	}
	case EKind::Swipe:
	{
		const double Sweep = T, Alpha = 1.0 - T;
		if (Sweep <= 0.0 || Alpha <= 0.0)
		{
			return;
		}
		auto World = [&E](const FVector2D& Local) { return E.At + Rotate(Local, E.Angle); };
		const FColorG Stroke = E.bA ? PawFxLook::Crit : SwipeColor;
		const double Width = E.bB ? SwipeWidth * EarnedWidthScale : SwipeWidth * (E.bA ? CritWidthScale : 1.0);
		const double Lead = -ArcSpan * 0.5 + ArcSpan * Sweep;
		// Restyle D3: the arc is the trail of the paw at its tip, shorter than Godot's (PawFxLook::StrikeArcTrail).
		const double Span = ArcSpan * 0.55 * PawFxLook::StrikeArcTrail;
		const double Start = Lead - Span;
		TArray<FVector2D, TInlineAllocator<21>> Points, Echo;
		for (int32 I = 0; I <= SwipeSegments; ++I)
		{
			const double A = Start + Span * I / SwipeSegments;
			Points.Add(World(FVector2D(FMath::Cos(A), FMath::Sin(A)) * E.Radius));
			const double B = Start - Span * 0.30 + Span * I / SwipeSegments;
			Echo.Add(World(FVector2D(FMath::Cos(B), FMath::Sin(B)) * (E.Radius * 0.78)));
		}
		const FColorG Ink = InkColor;
		Out.Polyline(Points, WithAlpha(Ink, 0.85 * Alpha), Width + SwipeInkEdge * 2.0);
		Out.Polyline(Points, Scaled(Stroke, Alpha), Width);
		// _draw_claws: three slashes on the victim, growing in after CLAW_AT of the sweep.
		if (Sweep >= ClawAt && E.Dist > 0.0)
		{
			const double Grow = FMath::Clamp((Sweep - ClawAt) / 0.25, 0.0, 1.0);
			const FVector2D Centre(E.Dist, 0.0);
			const FVector2D Along = FVector2D(1.0, -1.0).GetSafeNormal();
			const FVector2D Across(Along.Y, -Along.X);
			const double ClawW = ClawWidth * Width / SwipeWidth;
			for (int32 I = 0; I < ClawCount; ++I)
			{
				const FVector2D Off = Across * ClawGap * (I - (ClawCount - 1) * 0.5);
				const FVector2D Half = Along * ClawLength * 0.5;
				const FVector2D A = Centre + Off - Half;
				const FVector2D B = A + Along * ClawLength * Grow;
				Out.Line(World(A), World(B), WithAlpha(Ink, 0.85 * Alpha), ClawW + SwipeInkEdge);
				Out.Line(World(A), World(B), Scaled(Stroke, Alpha), ClawW);
			}
		}
		Out.Polyline(Echo, Scaled(Stroke, Alpha * 0.45), Width * 0.55);
		// Restyle D3: the clutching paw (ui_cursor_paw_closed's drawing) at the arc's tip, its toes along the motion, on top.
		DrawStrikePaw(Out, World(FVector2D(FMath::Cos(Lead), FMath::Sin(Lead)) * E.Radius),
			Rotate(FVector2D(-FMath::Sin(Lead), FMath::Cos(Lead)), E.Angle), FMath::Min(1.0, Alpha * PawFxLook::StrikePawFadeGain));
		return;
	}
	case EKind::Beam:
	{
		const FVector2D Along = E.To - E.At;
		if (Along.Size() < MinLengthPx)
		{
			return;
		}
		const double Alpha = 1.0 - T;
		const FVector2D Side = FVector2D(Along.Y, -Along.X).GetSafeNormal(); // Vector2.orthogonal()
		const double HalfRoot = E.Width * 0.5, HalfTip = HalfRoot * TipFraction;
		const FVector2D Wide[4] = { E.At + Side * HalfRoot * 2.0, E.To + Side * HalfTip * 2.0, E.To - Side * HalfTip * 2.0,
			E.At - Side * HalfRoot * 2.0 };
		const FVector2D Quad[4] = { E.At + Side * HalfRoot, E.To + Side * HalfTip, E.To - Side * HalfTip, E.At - Side * HalfRoot };
		Out.Polygon(Wide, Scaled(E.Color, Alpha * 0.35));
		Out.Polygon(Quad, Scaled(E.Color, Alpha));
		return;
	}
	case EKind::Shards:
	{
		const double Elapsed = T * E.Life, Fade = 1.0 - T;
		for (const FShard& S : E.Pieces)
		{
			FVector2D Off = S.Dir * S.Speed * Elapsed;
			Off.Y += 0.5 * E.Gravity * Elapsed * Elapsed;
			double Alpha = Fade;
			if (E.bA)
			{
				Alpha *= 0.55 + 0.45 * FMath::Sin(S.Phase + Elapsed * 18.0);
			}
			const double Half = E.Size * S.Scale * 0.5;
			const double Turn = S.Spin * Elapsed;
			const FVector2D Pts[4] = { E.At + Off + Rotate(FVector2D(0.0, -Half), Turn), E.At + Off + Rotate(FVector2D(Half * 0.6, 0.0), Turn),
				E.At + Off + Rotate(FVector2D(0.0, Half), Turn), E.At + Off + Rotate(FVector2D(-Half * 0.6, 0.0), Turn) };
			Out.Polygon(Pts, Scaled(E.Color, FMath::Clamp(Alpha, 0.0, 1.0)));
		}
		return;
	}
	}
}

void FPawVfx::Draw(const FContext& C, FPawFxCanvas& Ground, FPawFxCanvas& Under, FPawFxCanvas& Over)
{
	const Pc::PcGameData& D = *C.D;
	const Pc::PcField& F = C.Sim->Field;
	Ground.Reset();
	Under.Reset();
	Over.Reset();

	// GROUND: Edge Burn (the scene's node, before the creatures in the tree: under their shadows).
	Ground.SetZ(UPawSpriteRenderer::GroundFxZ);
	const Pc::PcRect& B = F.Bounds;
	if (bEdgeFire && B.W > EdgeBurnBandPx * 2.0 && B.H > EdgeBurnBandPx * 2.0)
	{
		const double Breath = 1.0 + FMath::Sin(EdgeTime * Tau * BreathPerSec) * BreathDepth;
		const FColorG Hot = WithAlpha(FireColor, EdgeAlpha * Breath), Cold = WithAlpha(FireColor, 0.0);
		const double Band = EdgeBurnBandPx;
		const FVector2D O0(B.X, B.Y), O1(B.X + B.W, B.Y + B.H), I0(B.X + Band, B.Y + Band), I1(B.X + B.W - Band, B.Y + B.H - Band);
		const FColorG Cols[4] = { Hot, Hot, Cold, Cold };
		const FVector2D Strips[4][4] = {
			{ FVector2D(O0.X, O0.Y), FVector2D(O1.X, O0.Y), FVector2D(I1.X, I0.Y), FVector2D(I0.X, I0.Y) },
			{ FVector2D(O0.X, O1.Y), FVector2D(O1.X, O1.Y), FVector2D(I1.X, I1.Y), FVector2D(I0.X, I1.Y) },
			{ FVector2D(O0.X, O0.Y), FVector2D(O0.X, O1.Y), FVector2D(I0.X, I1.Y), FVector2D(I0.X, I0.Y) },
			{ FVector2D(O1.X, O0.Y), FVector2D(O1.X, O1.Y), FVector2D(I1.X, I1.Y), FVector2D(I1.X, I0.Y) },
		};
		for (const auto& S : Strips)
		{
			Ground.Polygon(S, Cols);
		}
		// The embers: drifting round the perimeter, set into the band by their depth.
		const double Perimeter = 2.0 * (B.W + B.H);
		const double Drift = Perimeter > 0.0 ? EmberDriftPerSec / Perimeter : 0.0;
		const FVector2D InnerCentre((I0.X + I1.X) * 0.5, (I0.Y + I1.Y) * 0.5);
		const int32 Shown = FMath::RoundToInt(Embers.Num() * C.Budget);
		for (int32 I = 0; I < Shown && I < Embers.Num(); ++I)
		{
			const FEmber& Em = Embers[I];
			double Dd = Fposmod(Em.T + EdgeTime * Drift, 1.0) * Perimeter;
			FVector2D At;
			if (Dd < B.W) { At = FVector2D(B.X + Dd, B.Y); }
			else if ((Dd -= B.W) < B.H) { At = FVector2D(B.X + B.W, B.Y + Dd); }
			else if ((Dd -= B.H) < B.W) { At = FVector2D(B.X + B.W - Dd, B.Y + B.H); }
			else { Dd -= B.W; At = FVector2D(B.X, B.Y + B.H - Dd); }
			At += (InnerCentre - At).GetSafeNormal() * (Band * Em.Depth * 0.8);
			const double Pulse = 0.75 + 0.25 * FMath::Sin(EdgeTime * 2.1 + Em.Phase);
			Ground.Circle(At, Em.Size * Pulse, WithAlpha(FireColor, 0.42 * Breath * (0.4 + 0.6 * (1.0 - Em.Depth))));
		}
	}

	// UNDER: the poop zone, then the damage zones (after the enemies in the tree: over their shadows).
	Under.SetZ(UPawSpriteRenderer::UnderFxZ);
	if (PoopLeft > 0.0 && PoopRadius > 0.0)
	{
		const double Life = FMath::Clamp(PoopLeft / PoopDuration, 0.0, 1.0);
		for (int32 I = 0; I < PoopBands; ++I)
		{
			const double T = static_cast<double>(I) / PoopBands;
			const double Hue = Fposmod(T + PoopTime * PoopHueCycle, 1.0);
			Under.Circle(PoopAt, PoopRadius * (1.0 - T * 0.18), WithAlpha(FPawFxCanvas::FromHsv(Hue, 0.65, 1.0), 0.10 * Life));
		}
		Under.Arc(PoopAt, PoopRadius, 0.0, Tau, PoopSegments,
			WithAlpha(FPawFxCanvas::FromHsv(Fposmod(PoopTime * PoopHueCycle, 1.0), 0.7, 1.0), 0.75 * Life), PoopRingWidth);
	}
	for (const Pc::PcDamageZone& Z : F.Zones)
	{
		const double Life = Z.Duration > 0.0 ? FMath::Clamp(Z.Remaining / Z.Duration, 0.0, 1.0) : 0.0;
		if (Z.Radius <= 0.0 || Life <= 0.0)
		{
			continue;
		}
		const FColorG Base = Z.Kind == Pc::PcZoneKind::Trail ? ZoneTrail : Z.Kind == Pc::PcZoneKind::Crater ? ZoneCrater : ZoneEcho;
		const FVector2D At(Z.X, Z.Y);
		Under.Circle(At, Z.Radius, Scaled(Base, 0.28 * Life));
		for (int32 I = 0; I < ZoneBands; ++I)
		{
			const double T = static_cast<double>(I + 1) / ZoneBands;
			Under.Arc(At, Z.Radius * T, 0.0, Tau, ZoneSegments, Scaled(Base, 0.85 * Life * T), ZoneRingWidth);
		}
	}

	// OVER, in Godot's z order. The Lone Hunter's halo (z 1, drawn before its kitten).
	using namespace PawLook;
	for (int32 K = 0; K < static_cast<int32>(F.KActive.size()); ++K)
	{
		if (!F.KActive[K] || !F.KLone[K])
		{
			continue;
		}
		double X = 0.0, Y = 0.0;
		C.Sim->KittenPos(K, X, Y);
		Over.SetZ(UPawSpriteRenderer::BodyZ(Order(ZKitten, K) - 0.5));
		for (const auto& Ring : HaloRings)
		{
			Over.Circle(FVector2D(X, Y), KittenPx * Ring[0], WithAlpha(HaloColour, Ring[1]));
		}
	}
	// The aura ring (z 2, behind the cat: show_behind_parent).
	if (bAuraOwned && AuraRadius > 0.0)
	{
		Over.SetZ(UPawSpriteRenderer::BodyZ(Order(ZCat, 0.0) - 0.5));
		if (bAuraRainbow)
		{
			const double BaseHue = Fposmod(AuraTime * RainbowHueCycle, 1.0);
			Over.Circle(C.Cat, AuraRadius, WithAlpha(FPawFxCanvas::FromHsv(BaseHue, RainbowSaturation, 1.0), RainbowFillAlpha));
			const double Step = Tau / RainbowArcCount;
			for (int32 I = 0; I < RainbowArcCount; ++I)
			{
				const double Hue = Fposmod(BaseHue + static_cast<double>(I) / RainbowArcCount * RainbowArcSpan, 1.0);
				Over.Arc(C.Cat, AuraRadius, Step * I, Step * I + Step * 1.05, FMath::Max(4, AuraSegments / RainbowArcCount),
					WithAlpha(FPawFxCanvas::FromHsv(Hue, RainbowSaturation, 1.0), RainbowRingAlpha), RainbowRingWidth);
			}
		}
		else
		{
			Over.Circle(C.Cat, AuraRadius, PawFxLook::AuraLift);
			Over.Arc(C.Cat, AuraRadius, 0.0, Tau, AuraSegments, WithAlpha(InkColor, PawFxLook::AuraInkAlpha),
				PawFxLook::AuraRingWidth + PawFxLook::AuraInkEdge * 2.0);
			Over.Arc(C.Cat, AuraRadius, 0.0, Tau, AuraSegments, PawFxLook::AuraRing, PawFxLook::AuraRingWidth);
		}
	}
	// The golden sparkles (z 2, children of the mice: after the cat in the tree), then the slams (z 2, spawned later still).
	Over.SetZ(UPawSpriteRenderer::BodyZ(Order(ZCat, 1000.0)));
	for (const TPair<int32, FSparkle>& Pair : SparkleState)
	{
		const FSparkle& Sp = Pair.Value;
		const int32 S = Pair.Key;
		// Only while that slot is still this golden mouse (a travel or a kill since the last tick leaves a stale entry).
		if (S < 0 || static_cast<size_t>(S) >= F.Active.size() || !F.Active[S] || F.Serial[S] != Sp.Serial)
		{
			continue;
		}
		double X = 0.0, Y = 0.0;
		C.Sim->MousePos(Pair.Key, X, Y);
		const FVector2D At(X, Y);
		for (const FSparkle::FFleck& Fl : Sp.Trail)
		{
			const double Life = 1.0 - Fl.Age / TrailLifetime;
			Over.Circle(Fl.Pos, Fl.Size * Life, WithAlpha(GoldCore, Life * 0.7));
		}
		const double Breathe = SparkleHalo + FMath::Sin(Sp.Time * 3.2) * HaloBreathe;
		Over.Circle(At, Breathe, WithAlpha(GoldEdge, 0.18 + 0.08 * FMath::Sin(Sp.Time * 3.2)));
		Over.Arc(At, Breathe, 0.0, Tau, HaloSegments, WithAlpha(GoldEdge, 0.55), 1.8);
		for (int32 I = 0; I < MoteCount; ++I)
		{
			const double Phase = static_cast<double>(I) / MoteCount * Tau;
			const double Speed = 1.6 + 0.35 * I;
			const double R = FMath::Lerp(OrbitMin, OrbitMax, static_cast<double>(I) / MoteCount);
			const double A = Phase + Sp.Time * Speed;
			const double Twinkle = 0.55 + 0.45 * FMath::Sin(Sp.Time * 7.0 + Phase);
			Over.Circle(At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R, MoteRadius * Twinkle, WithAlpha(GoldCore, Twinkle));
		}
	}
	Over.SetZ(UPawSpriteRenderer::BodyZ(Order(ZCat, 1500.0)));
	for (const FFx& E : Effects)
	{
		if (E.Kind == EKind::Slam)
		{
			DrawEffect(E, C, Over);
		}
	}
	// The boss's telegraph (z 5): the shield's countdown round the boss.
	if (F.bBossUp && F.TierIndex >= 0 && static_cast<size_t>(F.TierIndex) < D.Tiers.size())
	{
		if (const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(F.TierIndex)].BossData); Boss && !Boss->PhaseVulnerableSec.empty())
		{
			double X = 0.0, Y = 0.0;
			C.Sim->BossPos(X, Y);
			const FVector2D At(X, Y);
			const int32 Phase = FMath::Clamp(static_cast<int32>(F.BossPhase) - 1, 0, static_cast<int32>(Boss->PhaseVulnerableSec.size()) - 1);
			const double Window = F.bBossShielded
				? (static_cast<size_t>(Phase) < Boss->PhaseShieldSec.size() ? Boss->PhaseShieldSec[static_cast<size_t>(Phase)] : 0.0)
				: Boss->PhaseVulnerableSec[static_cast<size_t>(Phase)];
			const double Remaining = FMath::Max(0.0, Window - F.BossInvulnTimer);
			FColorG Colour = F.bBossShielded ? ShieldedColor : OpenColor;
			double Alpha = 0.9;
			if (Remaining <= WarningSec && Window > 0.0)
			{
				const double Urgency = 1.0 - FMath::Clamp(Remaining / WarningSec, 0.0, 1.0);
				const double Hz = FMath::Lerp(PulseMinHz, PulseMaxHz, Urgency);
				Alpha = 0.45 + 0.55 * FMath::Abs(FMath::Sin(TelegraphTime * Hz * UE_DOUBLE_PI));
			}
			Colour.A = static_cast<float>(Alpha);
			Over.SetZ(UPawSpriteRenderer::BodyZ(Order(5, 0.0)));
			Over.Arc(At, TelegraphRadius, 0.0, Tau, TelegraphSegments, TrackColor, TelegraphWidth);
			if (Window > 0.0)
			{
				const double Fraction = FMath::Clamp(Remaining / Window, 0.0, 1.0);
				const double Start = -UE_DOUBLE_PI / 2.0;
				Over.Arc(At, TelegraphRadius, Start, Start + Tau * Fraction, TelegraphSegments, Colour, TelegraphWidth);
			}
			Over.Arc(At, TelegraphRadius - TelegraphWidth * 2.0, 0.0, Tau, TelegraphSegments, WithAlpha(Colour, Alpha * 0.55), 2.0);
		}
	}
	// On top: the swipes and beams (z 45, in the order they came), the bursts (50), the shards (51), the cat's own rings (52).
	Over.SetZ(UPawSpriteRenderer::TopFxZ);
	for (const FFx& E : Effects)
	{
		if (E.Kind == EKind::Swipe || E.Kind == EKind::Beam)
		{
			DrawEffect(E, C, Over);
		}
	}
	for (const FFx& E : Effects)
	{
		if (E.Kind == EKind::Burst && !E.bA)
		{
			DrawEffect(E, C, Over);
		}
	}
	for (const FFx& E : Effects)
	{
		if (E.Kind == EKind::Shards)
		{
			DrawEffect(E, C, Over);
		}
	}
	for (const FFx& E : Effects)
	{
		if (E.Kind == EKind::Burst && E.bA)
		{
			DrawEffect(E, C, Over);
		}
	}
}
