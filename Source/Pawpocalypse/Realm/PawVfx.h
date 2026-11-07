#pragma once

// The realm's effects (phase 6 slice 3), ported from Godot's vfx_*.gd, aura_ring.gd, poop_zone_visual.gd,
// damage_zone_field.gd, boss_telegraph.gd, golden_sparkle.gd and the Lone Hunter's halo (kitten.gd), with tier_controller.gd's
// and player.gd's choice of which effect each moment of the field gets. Presentation only: the field's events and state in,
// FPawFxCanvas draw calls out (UPawSpriteRenderer draws them in three bands). Every effect honours the effects setting as
// Godot's do (spawn nothing at a budget of 0; the shards' count scaled by it).
//
// Time: a paused Godot tree runs no tween and no _process, so every effect freezes while the field is held (the view passes
// a delta of 0 then).
//
// Kept from Godot on purpose (findings, the port copies them): the Cone of Shame's ring and the job ring are children of the
// cat placed with global coordinates before they enter the tree, so they draw at the cat's position PLUS where they were
// spawned (F39, usually off screen).

#include "CoreMinimal.h"
#include "PcField.h"
#include "Render/PawFxCanvas.h"

#include <string>

struct FPawRealmSim;
namespace Pc
{
	struct PcGameData;
	struct PcProgress;
}

class PAWPOCALYPSE_API FPawVfx
{
public:
	struct FContext
	{
		const Pc::PcGameData* D = nullptr;
		const Pc::PcProgress* P = nullptr;
		const FPawRealmSim* Sim = nullptr;
		FVector2D Cat = FVector2D::ZeroVector; // the cat, interpolated
		double Budget = 1.0;                   // SettingsManager.effect_budget()
		double Dt = 0.0;                       // this frame's seconds of unpaused play
	};

	enum class EKind : uint8
	{
		Burst,
		Slam,
		Swipe,
		Beam,
		Shards,
	};

	// The field's events of a frame (FPawRealmSim::FrameEvents, in order).
	void OnFrameEvents(const FContext& C);
	// Ages everything, runs the timers (foresight, the drag tether, the rush pulse, the boss's death bursts, the sparkles).
	void Tick(const FContext& C);
	// Draws into the three bands.
	void Draw(const FContext& C, FPawFxCanvas& Ground, FPawFxCanvas& Under, FPawFxCanvas& Over);
	void Clear();

	// VfxBurst.spawn etc., as tier_controller.gd calls them. False when the effects setting spawned nothing.
	bool Burst(const FContext& C, const FVector2D& At, const FLinearColor& Color, double EndRadius, double Duration = 0.35,
		double Width = 3.0, bool bOnCat = false);
	bool Slam(const FContext& C, const FVector2D& At, double Radius, bool bQuiet);
	bool Swipe(const FContext& C, const FVector2D& Origin, const FVector2D& Target, double Reach, bool bCrit, bool bEarned);
	bool Beam(const FContext& C, const FVector2D& From, const FVector2D& To, const FLinearColor& Color, double Width = 6.0,
		double Duration = 0.18);
	bool Shards(const FContext& C, const FVector2D& At, const FLinearColor& Color, int32 Count, double Speed, double Duration = 0.5,
		double Gravity = 0.0, double Size = 6.0, bool bTwinkle = false);

	// What is live (tests, the debug HUD).
	int32 Live() const { return Effects.Num(); }
	int32 LiveOf(EKind Kind) const;
	int64 Spawned() const { return SpawnedCount; }
	bool IsPoopShowing() const { return PoopLeft > 0.0; }
	bool AreEdgeFiresBurning() const { return bEdgeFire; }
	int32 Sparkles() const { return SparkleState.Num(); }

	// A cap on live effects for safety under a flood (Godot had none; at its measured kill rates this never binds).
	static constexpr int32 MaxLive = 2048;

private:
	struct FShard
	{
		FVector2D Dir;
		double Speed = 0.0, Spin = 0.0, Scale = 1.0, Phase = 0.0;
	};
	struct FFx
	{
		EKind Kind = EKind::Burst;
		int64 Serial = 0;
		double Age = 0.0, Life = 0.35;
		FVector2D At = FVector2D::ZeroVector, To = FVector2D::ZeroVector;
		FLinearColor Color = FLinearColor::White;
		double Radius = 0.0, Width = 3.0, Angle = 0.0, Dist = 0.0;
		bool bA = false, bB = false; // burst: on the cat; slam: quiet; swipe: crit, earned; shards: twinkle
		double Gravity = 0.0, Size = 6.0;
		TArray<FShard> Pieces;
		TArray<TArray<FVector2D>> Cracks;
	};
	struct FSparkle
	{
		uint64 Serial = 0;
		double Time = 0.0, TrailTimer = 0.0;
		struct FFleck
		{
			FVector2D Pos;
			double Age = 0.0, Size = 2.0;
		};
		TArray<FFleck> Trail;
		bool bSeen = false;
	};
	struct FEmber
	{
		double T = 0.0, Depth = 0.0, Size = 2.0, Phase = 0.0;
	};

	bool Room(const FContext& C) const;
	FFx& Add(EKind Kind);
	void DrawEffect(const FFx& E, const FContext& C, FPawFxCanvas& Out) const;
	void SeedEmbers(double Budget);

	TArray<FFx> Effects;
	int64 SpawnedCount = 0;
	int64 NextSerial = 0;
	FRandomStream Random{ 0x0f0f };

	// Persistent visuals.
	double AuraTime = 0.0, AuraPoll = 0.0, AuraRadius = 0.0;
	bool bAuraRainbow = false, bAuraOwned = false;
	FVector2D PoopAt = FVector2D::ZeroVector;
	double PoopRadius = 0.0, PoopDuration = 1.0, PoopLeft = 0.0, PoopTime = 0.0;
	double TelegraphTime = 0.0;
	double EdgeTime = 0.0;
	bool bEdgeFire = false;
	int64 EdgeTier = -2;
	TArray<FEmber> Embers;
	TMap<int32, FSparkle> SparkleState; // per mouse slot

	// Timers of tier_controller.gd.
	double ForesightPulse = 0.0;
	double DragTether = 0.0;
	double RushPulse = 0.0;
	double PrismCooldown = 0.0;
	TMap<FString, double> PopLast;      // _bonus_pop's rate limit per kind (seconds of play)
	double GibletsLast = -1000.0;
	double Clock = 0.0;
	struct FDelayed
	{
		double At = 0.0;
		FVector2D Where;
		FLinearColor Color;
		double Radius = 0.0;
	};
	TArray<FDelayed> Delayed;            // the boss's death bursts (0.12 s apart)
	std::string LastJob;
	bool bJobKnown = false;
};
