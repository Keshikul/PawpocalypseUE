#pragma once

// How the field's creatures LOOK (phase 6): Godot's presentation constants and the small pure functions the realm view draws
// with, kept here so the tests can check each against the Godot code it came from. Presentation only (Data/CodeConstantsMap.csv
// routes every one of these to "presentation"): nothing here changes a rule.

#include "CoreMinimal.h"

namespace PawLook
{
	// sprite_liveliness.gd: art faces LEFT; a mover flips when its horizontal speed passes the dead zone. The breathing pulse is
	// VERTICAL only, 3 % of the height at 1.4 Hz.
	constexpr double FacingDeadzonePxPerSec = 8.0;
	constexpr double PulseAmplitude = 0.03;
	constexpr double PulseHz = 1.4;

	// enemy.gd
	constexpr double GlowScale = 1.24;        // the signature glow: a white silhouette this much larger, behind
	constexpr double GlowAlphaMin = 0.65;     // GLOW_ALPHA, breathing between the two
	constexpr double GlowAlphaMax = 1.0;
	constexpr double GlowHz = 1.1;
	inline const FLinearColor ShieldTint(0.55f, 0.9f, 1.0f, 0.6f); // a Godot modulate (sRGB space); also boss.gd's
	inline const FLinearColor SnareTint(0.46f, 0.64f, 0.95f, 1.0f);

	// ground_shadow.gd
	constexpr double ShadowAlpha = 0.7;
	constexpr double ShadowWidthShare = 0.8;
	constexpr double ShadowFeet = 0.46;
	constexpr double ShadowFlierWidth = 0.55;
	constexpr double ShadowFlierAlpha = 0.6;
	constexpr double ShadowFlierDrop = 0.18;

	// player.gd
	constexpr double CatPx = 175.0;           // SPRITE_TARGET_PX
	constexpr double HopSec = 0.42;
	constexpr double HopHeightPx = 26.0;
	constexpr double HopSquash = 0.16;
	constexpr double HopCrouchShare = 0.12;
	constexpr double HopLandShare = 0.12;
	constexpr int32 MountedFromStage = 4;     // MOUNTED_FROM_STAGE
	// In sprite sizes from the cat's centre (x left of centre is forward: the art faces left), and as a share of its width.
	constexpr double HatOffsetX = -0.10, HatOffsetY = -0.31, HatWidth = 0.62;
	constexpr double HatOffsetMountedX = 0.045, HatOffsetMountedY = -0.29, HatWidthMounted = 0.36;
	constexpr double WeaponOffsetX = -0.38, WeaponOffsetY = 0.14, WeaponWidth = 0.405;
	constexpr double WeaponOffsetMountedX = -0.14, WeaponOffsetMountedY = -0.26, WeaponWidthMounted = 0.352;

	// kitten.gd
	constexpr double KittenPx = 96.0;
	inline const FLinearColor GoldenKittenTint(1.30f, 1.06f, 0.40f, 1.0f); // overbright on purpose
	inline const FLinearColor IllusionTint(0.72f, 0.80f, 1.0f, 0.45f);
	inline const FLinearColor HaloColour(0.62f, 0.84f, 1.0f, 1.0f);
	constexpr double HaloRings[3][2] = { { 0.78, 0.10 }, { 0.62, 0.13 }, { 0.46, 0.16 } }; // radius share of KittenPx, alpha

	// boss.gd / boss.tscn
	constexpr double BossScale = 0.9;         // the scene's Visual.scale over the SVG's own size
	constexpr double TwitchInterval = 0.09;
	constexpr double TwitchIdlePx = 2.0;
	constexpr double TwitchAngryPx = 11.0;
	constexpr double AngerDecayPerSec = 1.6;
	constexpr double DeathShakeSec = 0.45;    // six random offsets, then 0.05 s back to the centre
	constexpr double DeathCollapseSec = 0.55; // scale to 0.05 (back ease-in) while fading out
	constexpr double DeathReturnSec = 0.05;

	// boss_add.gd: a texture drawn RADIUS x 1.5 x pulse from the centre, pulse 0.85 + 0.15 sin(age x 6).
	constexpr double AddRadius = 15.0;

	// Godot's z_index of each thing on the field (scene files and scripts); the order inside a z is the scene tree's.
	constexpr int32 ZMice = 0;     // enemy.tscn; the boss is a later child of the same node
	constexpr int32 ZKitten = 1;   // kitten.tscn
	constexpr int32 ZCat = 2;      // player.tscn; its weapon at +1, its hat at +2 (relative)
	constexpr int32 ZAdd = 3;      // boss_add.gd _ready
	constexpr double TreeSpan = 2000.0;

	// Draw order for the renderer (FPawSprite::Order): z first, then the position in the tree.
	inline double Order(int32 GodotZ, double TreePosition) { return GodotZ * TreeSpan + FMath::Clamp(TreePosition, 0.0, TreeSpan - 1.0); }

	// A Godot modulate (multiplied with the texture in sRGB space, values over 1 allowed) as the linear tint the sprite
	// material multiplies a LINEAR texel by: (tex x m)^2.2 = tex^2.2 x m^2.2. Alpha is opacity, unchanged.
	inline FLinearColor Modulate(const FLinearColor& M)
	{
		auto L = [](float V) { return V <= 0.0f ? 0.0f : FMath::Pow(V, 2.2f); };
		return FLinearColor(L(M.R), L(M.G), L(M.B), M.A);
	}
	inline FLinearColor Multiply(const FLinearColor& A, const FLinearColor& B) { return A * B; }

	// SpriteLiveliness.pulsed_scale: the vertical factor at time T (seconds) for a phase in turns.
	inline double PulseY(double T, double Phase) { return 1.0 + PulseAmplitude * FMath::Sin((T * PulseHz + Phase) * UE_DOUBLE_TWO_PI); }
	// SpriteLiveliness.phase_for: stable per instance, in [0, 1).
	inline double PhaseFor(uint64 Key) { return static_cast<double>((Key * 2654435761ull) % 1000ull) / 1000.0; }
	// The rig (restyle R5, 2026-11-09, the port's own look: Godot has none): a layer's turn in degrees, clockwise on screen as
	// drawn, at time T (seconds) for a creature whose own phase is Own (turns). Swing: Amp sin; lift: Amp (1 - cos) / 2, so 0
	// at rest. tools/art/review_rig.py draws with the same formula.
	inline double RigAngleDeg(bool bLift, double Amp, double Hz, double Phase, double Own, double T)
	{
		const double A = (Hz * T + Phase + Own) * UE_DOUBLE_TWO_PI;
		return bLift ? Amp * (1.0 - FMath::Cos(A)) * 0.5 : Amp * FMath::Sin(A);
	}
	// Where a rig layer's quad goes: the drawing is centred at (X, Y), W x H, mirrored when bFlip; the layer turns Deg degrees
	// (as drawn) about its pivot (U, V across the square). The pivot stays put; the quad's centre swings round it.
	struct FRigPose
	{
		double X = 0.0, Y = 0.0;
		double Angle = 0.0;  // radians, clockwise on screen (FPawSprite::Angle)
	};
	inline FRigPose RigPose(double X, double Y, double W, double H, bool bFlip, double U, double V, double Deg)
	{
		double Px = (U - 0.5) * W;
		const double Py = (V - 0.5) * H;
		double A = FMath::DegreesToRadians(Deg);
		if (bFlip)
		{
			Px = -Px;
			A = -A;
		}
		// The centre sits at -P from the pivot; clockwise on screen with y down is (x cos - y sin, x sin + y cos).
		const double C = FMath::Cos(A), S = FMath::Sin(A);
		return { X + Px - Px * C + Py * S, Y + Py - Px * S - Py * C, A };
	}

	// SpriteLiveliness.face: the new flip state given the old one and a horizontal speed.
	inline bool Face(bool bFlipped, double Dx) { return FMath::Abs(Dx) < FacingDeadzonePxPerSec ? bFlipped : Dx > 0.0; }

	// Player._apply_hop with HopLeft already decremented this frame: the lift (px up) and the stretch (x, y factors).
	struct FHop
	{
		double Lift = 0.0;
		double ScaleX = 1.0, ScaleY = 1.0;
	};
	inline FHop Hop(double HopLeft, bool bHopping)
	{
		FHop H;
		if (!bHopping)
		{
			return H;
		}
		const double P = FMath::Clamp(1.0 - HopLeft / HopSec, 0.0, 1.0);
		const double Lift = FMath::Sin(P * UE_DOUBLE_PI);
		H.Lift = Lift * HopHeightPx;
		double Stretch = HopSquash * Lift * 0.5;
		if (P < HopCrouchShare)
		{
			Stretch = -HopSquash * (1.0 - P / HopCrouchShare);
		}
		else if (P > 1.0 - HopLandShare)
		{
			Stretch = -HopSquash * ((P - (1.0 - HopLandShare)) / HopLandShare);
		}
		H.ScaleX = 1.0 - Stretch;
		H.ScaleY = 1.0 + Stretch;
		return H;
	}

	// GroundShadow.place: the shadow under a creature CreaturePx tall standing at its centre, for a shadow texture of the
	// given aspect (height / width). Out: the shadow's centre offset below the creature, its size, its alpha.
	struct FShadow
	{
		double OffsetY = 0.0, Width = 0.0, Height = 0.0, Alpha = 0.0;
	};
	inline FShadow Shadow(double CreaturePx, bool bFlier, double Aspect)
	{
		FShadow S;
		S.Width = CreaturePx * ShadowWidthShare * (bFlier ? ShadowFlierWidth : 1.0);
		S.Height = S.Width * Aspect;
		S.OffsetY = CreaturePx * ShadowFeet + (bFlier ? CreaturePx * ShadowFlierDrop : 0.0);
		S.Alpha = ShadowAlpha * (bFlier ? ShadowFlierAlpha : 1.0);
		return S;
	}

	// The signature glow's alpha (enemy.gd _tick_glow) at a glow clock in seconds.
	inline double GlowAlpha(double Clock, double Phase)
	{
		const double T = 0.5 + 0.5 * FMath::Sin(Clock * UE_DOUBLE_TWO_PI * GlowHz + Phase);
		return FMath::Lerp(GlowAlphaMin, GlowAlphaMax, T);
	}

	// Godot's Tween TRANS_BACK + EASE_IN (s = 1.70158).
	inline double BackEaseIn(double T)
	{
		constexpr double S = 1.70158;
		return T * T * ((S + 1.0) * T - S);
	}

	// The cat's hat or weapon (player.gd _apply_hat / _apply_weapon): offset from the cat's centre and width, in field px,
	// for the cat's current scale factors relative to its fit (the breathing and the hop). Children of the sprite: they scale
	// with it but do NOT mirror when it flips (flip_h is the texture's, not the node's).
	struct FOverlay
	{
		double X = 0.0, Y = 0.0, Width = 0.0, Height = 0.0;
	};
	inline FOverlay CatOverlay(bool bHat, bool bMounted, double RelX, double RelY)
	{
		const double OX = bHat ? (bMounted ? HatOffsetMountedX : HatOffsetX) : (bMounted ? WeaponOffsetMountedX : WeaponOffsetX);
		const double OY = bHat ? (bMounted ? HatOffsetMountedY : HatOffsetY) : (bMounted ? WeaponOffsetMountedY : WeaponOffsetY);
		const double W = bHat ? (bMounted ? HatWidthMounted : HatWidth) : (bMounted ? WeaponWidthMounted : WeaponWidth);
		FOverlay O;
		O.X = CatPx * OX * RelX;
		O.Y = CatPx * OY * RelY;
		O.Width = CatPx * W * RelX;
		O.Height = CatPx * W * RelY; // hats and weapons are square drawings
		return O;
	}
	// Where an overlay is drawn for the cat facing right (bFlip) or left (its drawing's own way): the hat and the weapon mirror
	// with the cat, as one figure (F38, a Godot fault the port does not copy, D16: Godot flipped only the cat's picture, so a
	// cat steered right wore its hat behind its ears and held its weapon behind its back).
	inline FOverlay FacingOverlay(const FOverlay& O, bool bFlip)
	{
		FOverlay M = O;
		M.X = bFlip ? -O.X : O.X;
		return M;
	}
	// The weapon is drawn leaning right: mirrored while the cat faces left. The hat mirrors as the cat does.
	inline bool WeaponFlipX(bool bFlip) { return !bFlip; }
	inline bool HatFlipX(bool bFlip) { return bFlip; }
}
