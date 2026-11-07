#pragma once

// The UI's motion (phase 6 slice 5c): Godot's Tween easing and scripts/systems/ui_juice.gd (UiJuice), the hover pop, the
// press squash, the purchase punch, the idle jitter and the pop-in. Pure state stepped by its owner's tick; the owner applies
// Scale / Rotation as a render transform about the centre (UiJuice's rule: motion never changes layout). Ground truth: the
// fixture `juice` (the real Tween.interpolate_value at every easing UiJuice uses, and the real UiJuice on a real Button).

#include "PcCore.h"

#include <cstdint>
#include <vector>

namespace Pc
{
	// Godot's Tween.TransitionType / EaseType, the ones UiJuice uses (and LINEAR).
	enum class PcTrans : uint8_t { Linear, Sine, Quad, Back, Elastic };
	enum class PcEase : uint8_t { In, Out, InOut, OutIn };

	// Tween.interpolate_value(initial, delta, elapsed, duration, trans, ease): the value a tween from Initial by Delta shows
	// Elapsed seconds into Duration (scene/animation/easing_equations.h, in doubles; Godot's are float32).
	PAWCORE_API double TweenInterpolate(double Initial, double Delta, double Elapsed, double Duration, PcTrans Trans, PcEase Ease);

	// ui_juice.gd's constants (Data/CodeConstantsMap.csv: presentation).
	constexpr double JuiceHoverScale = 1.06;    // HOVER_SCALE
	constexpr double JuicePressScale = 0.94;    // PRESS_SCALE
	constexpr double JuiceHoverInSec = 0.16;    // HOVER_IN_SEC
	constexpr double JuiceHoverOutSec = 0.28;   // HOVER_OUT_SEC
	constexpr double JuicePressSec = 0.06;      // PRESS_SEC
	constexpr double JuiceReleaseSec = 0.32;    // RELEASE_SEC
	constexpr double JuicePunchScaleX = 1.14;   // PUNCH_SCALE.x
	constexpr double JuicePunchScaleY = 0.90;   // PUNCH_SCALE.y
	constexpr double JuicePunchSec = 0.42;      // PUNCH_SEC (a quarter out, three quarters back)
	constexpr double JuiceJitterDeg = 2.2;      // JITTER_DEG
	constexpr double JuiceJitterStepSec = 0.11; // JITTER_STEP_SEC
	constexpr double JuicePopInFrom = 0.86;     // POP_IN_FROM
	constexpr double JuicePopInSec = 0.18;      // POP_IN_SEC

	// One live Tween on one property (UiJuice keeps one per property per control: a new one kills the old). Its tweeners run
	// in sequence (the punch has two); each starts from the property's value at the moment it starts, as tween_property does.
	struct PcJuiceTween
	{
		struct Leg
		{
			double ToX = 0.0;
			double ToY = 0.0;
			double Duration = 0.0;
			PcTrans Trans = PcTrans::Linear;
			PcEase Ease = PcEase::Out;
		};
		Leg Legs[2];
		int32_t LegCount = 0;
		int32_t At = 0;           // the leg running
		bool bLive = false;
		bool bStarted = false;   // the current leg has captured its start (on the first step after it was made)
		bool bJitterStep = false; // a jitter step: its end chains the next one
		double FromX = 0.0;
		double FromY = 0.0;
		double Elapsed = 0.0;
	};

	struct PcJuice
	{
		double ScaleX = 1.0;
		double ScaleY = 1.0;
		double Rotation = 0.0; // radians
		bool bJitter = false;  // META_JITTER
		PcJuiceTween ScaleTween;
		PcJuiceTween RotTween;
		// The jitter's leans: drawn from Seed (randf_range(-JITTER_DEG, JITTER_DEG)); a test replays a recorded run's instead.
		uint64_t Seed = 0;
		uint64_t Draws = 0;
		std::vector<double> RecordedLeansDeg;
	};

	// The events UiJuice listens to (hover() wires them; a Button gets the press ones too).
	PAWCORE_API void JuiceHoverEnter(PcJuice& J, bool bDisabled);   // _on_enter: a disabled button does not grow
	PAWCORE_API void JuiceHoverExit(PcJuice& J);                    // _on_exit
	PAWCORE_API void JuicePressDown(PcJuice& J);                    // button_down
	PAWCORE_API void JuicePressUp(PcJuice& J, bool bHoveredEnabled); // _on_release: back to the hover size if still on it
	PAWCORE_API void JuicePunch(PcJuice& J);                        // punch(): the purchase squash-and-stretch
	PAWCORE_API void JuicePopIn(PcJuice& J);                        // pop_in(): from POP_IN_FROM with a small overshoot
	// jitter(control, on): bEffects is the effects setting (off: no idle jitter); bVisible: is_visible_in_tree().
	PAWCORE_API void JuiceSetJitter(PcJuice& J, bool bOn, bool bEffects, bool bVisible);
	// One frame of both tweens. bVisible: a jitter step that ends while hidden stops the jitter at rest (_jitter_step's sleep).
	PAWCORE_API void JuiceStep(PcJuice& J, double Dt, bool bVisible);
	// Nothing moving and at rest (the owner may stop ticking).
	PAWCORE_API bool JuiceAtRest(const PcJuice& J);
}
