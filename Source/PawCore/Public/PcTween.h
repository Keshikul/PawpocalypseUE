#pragma once

// Godot's Tween, the sequencing (phase 6 slice 6a; for the Unicorn ceremony, then Boba's cameos): steps run one after
// another, the tweeners of one step in parallel (`parallel()`, `set_parallel(true)`); a property tweener takes its start
// value when its step STARTS (or, delayed, when its delay ends) and lands exactly on its final value; a method tweener feeds
// from..to through its easing; a callback fires once its delay is over; an interval waits. Time left over when a step ends
// in the middle of a frame runs on into the next step in the same frame (scene/animation/tween.cpp Tween::step and the
// tweeners' step()). The easing is `TweenInterpolate` (PcJuice). Proven through the fixtures that use it (`unicorn`).
//
// The owner holds the values: a tweener names a property or a method by an int id, and the Host reads / writes / calls.

#include "PcCore.h"
#include "PcJuice.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace Pc
{
	struct PcTweener
	{
		enum class Kind : uint8_t { Property, Method, Callback, Interval };
		Kind What = Kind::Property;
		int Id = 0;                    // the property, method or callback
		double To = 0.0;               // a property's final value; a method's last
		double From = 0.0;             // a method's first value (a property's is read when it starts)
		double Duration = 0.0;
		double Delay = 0.0;
		PcTrans Trans = PcTrans::Linear;
		PcEase Ease = PcEase::InOut;   // Tween's default ease (EASE_IN_OUT; TRANS_LINEAR ignores it)

		// Running state.
		double Elapsed = 0.0;
		double Initial = 0.0;
		bool bFinished = false;
		bool bInitialPending = false;  // a delayed property reads its start when the delay ends
	};

	struct PcTweenHost
	{
		std::function<double(int)> Get;
		std::function<void(int, double)> Set;
		std::function<void(int)> Call;
	};

	struct PcTween
	{
		std::vector<std::vector<PcTweener>> Steps;
		size_t Current = 0;
		bool bStarted = false;
		bool bDead = false;

		// Building, as GDScript chains it.
		PcTweener& Property(int Id, double To, double Duration, PcTrans Trans = PcTrans::Linear, PcEase Ease = PcEase::InOut);
		PcTweener& Method(int Id, double From, double To, double Duration, PcTrans Trans = PcTrans::Linear, PcEase Ease = PcEase::InOut);
		PcTweener& Callback(int Id);
		PcTweener& Interval(double Duration);
		// The next tweener joins the last one's step (`parallel()`), or every next one does (`set_parallel(true)`).
		void Parallel() { bNextParallel = true; }
		void SetParallel(bool bOn) { bAllParallel = bOn; }

		// Tween::step(delta): false once dead (finished, or killed).
		PAWCORE_API bool Step(double Dt, const PcTweenHost& Host);
		void Kill() { bDead = true; }
		bool IsValid() const { return !bDead; }

	private:
		PcTweener& Add(const PcTweener& T);
		void StartStep(const PcTweenHost& Host);
		bool bNextParallel = false;
		bool bAllParallel = false;
	};
}
