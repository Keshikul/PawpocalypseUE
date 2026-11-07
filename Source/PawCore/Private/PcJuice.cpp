#include "PcJuice.h"

#include "PcRng.h"

#include <cmath>

namespace Pc
{
	namespace
	{
		constexpr double Pi = 3.14159265358979323846;

		// scene/animation/easing_equations.h, each (t elapsed, b start, c change, d duration). Godot computes them in real_t
		// (float32) with float constants (0.3f, 1.70158f); the port in doubles, so the fixture is compared to a tolerance.
		double SineIn(double t, double b, double c, double d) { return -c * std::cos(t / d * (Pi / 2)) + c + b; }
		double SineOut(double t, double b, double c, double d) { return c * std::sin(t / d * (Pi / 2)) + b; }
		double SineInOut(double t, double b, double c, double d) { return -c / 2 * (std::cos(Pi * t / d) - 1) + b; }

		double QuadIn(double t, double b, double c, double d) { return c * std::pow(t / d, 2) + b; }
		double QuadOut(double t, double b, double c, double d)
		{
			t /= d;
			return -c * t * (t - 2) + b;
		}
		double QuadInOut(double t, double b, double c, double d)
		{
			t = t / d * 2;
			if (t < 1)
			{
				return c / 2 * std::pow(t, 2) + b;
			}
			return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
		}

		constexpr double BackS = 1.70158;
		double BackIn(double t, double b, double c, double d)
		{
			t /= d;
			return c * t * t * ((BackS + 1) * t - BackS) + b;
		}
		double BackOut(double t, double b, double c, double d)
		{
			t = t / d - 1;
			return c * (t * t * ((BackS + 1) * t + BackS) + 1) + b;
		}
		double BackInOut(double t, double b, double c, double d)
		{
			const double S = BackS * 1.525;
			t /= d / 2;
			if (t < 1)
			{
				return c / 2 * (t * t * ((S + 1) * t - S)) + b;
			}
			t -= 2;
			return c / 2 * (t * t * ((S + 1) * t + S) + 2) + b;
		}

		double ElasticIn(double t, double b, double c, double d)
		{
			if (t == 0)
			{
				return b;
			}
			t /= d;
			if (t == 1)
			{
				return b + c;
			}
			t -= 1;
			const double P = d * 0.3;
			const double A = c * std::pow(2.0, 10 * t);
			const double S = P / 4;
			return -(A * std::sin((t * d - S) * (2 * Pi) / P)) + b;
		}
		double ElasticOut(double t, double b, double c, double d)
		{
			if (t == 0)
			{
				return b;
			}
			t /= d;
			if (t == 1)
			{
				return b + c;
			}
			const double P = d * 0.3;
			const double S = P / 4;
			return c * std::pow(2.0, -10 * t) * std::sin((t * d - S) * (2 * Pi) / P) + c + b;
		}
		double ElasticInOut(double t, double b, double c, double d)
		{
			if (t == 0)
			{
				return b;
			}
			t /= d / 2;
			if (t == 2)
			{
				return b + c;
			}
			const double P = d * (0.3 * 1.5);
			double A = c;
			const double S = P / 4;
			if (t < 1)
			{
				t -= 1;
				A *= std::pow(2.0, 10 * t);
				return -0.5 * (A * std::sin((t * d - S) * (2 * Pi) / P)) + b;
			}
			t -= 1;
			A *= std::pow(2.0, -10 * t);
			return A * std::sin((t * d - S) * (2 * Pi) / P) * 0.5 + c + b;
		}

		using Equation = double (*)(double, double, double, double);

		// EASE_OUT_IN: the out curve over the first half, the in curve over the second (each namespace's out_in()).
		double OutIn(Equation Out, Equation In, double t, double b, double c, double d)
		{
			if (t < d / 2)
			{
				return Out(t * 2, b, c / 2, d);
			}
			const double H = c / 2;
			return In(t * 2 - d, b + H, H, d);
		}

		double Run(PcTrans Trans, PcEase Ease, double t, double b, double c, double d)
		{
			Equation In = nullptr;
			Equation Out = nullptr;
			Equation InOut = nullptr;
			switch (Trans)
			{
			case PcTrans::Linear: return c * t / d + b; // linear::in, whatever the ease
			case PcTrans::Sine: In = SineIn; Out = SineOut; InOut = SineInOut; break;
			case PcTrans::Quad: In = QuadIn; Out = QuadOut; InOut = QuadInOut; break;
			case PcTrans::Back: In = BackIn; Out = BackOut; InOut = BackInOut; break;
			case PcTrans::Elastic: In = ElasticIn; Out = ElasticOut; InOut = ElasticInOut; break;
			}
			switch (Ease)
			{
			case PcEase::In: return In(t, b, c, d);
			case PcEase::Out: return Out(t, b, c, d);
			case PcEase::InOut: return InOut(t, b, c, d);
			case PcEase::OutIn: return OutIn(Out, In, t, b, c, d);
			}
			return b;
		}

		PcJuiceTween OneLeg(double ToX, double ToY, double Duration, PcTrans Trans, PcEase Ease)
		{
			PcJuiceTween T;
			T.Legs[0] = { ToX, ToY, Duration, Trans, Ease };
			T.LegCount = 1;
			T.bLive = true;
			return T;
		}

		// _scale_to(): one tween on scale, EASE_OUT, the old one killed (_fresh).
		void ScaleTo(PcJuice& J, double To, double Sec, PcTrans Trans)
		{
			J.ScaleTween = OneLeg(To, To, Sec, Trans, PcEase::Out);
		}

		double NextLeanDeg(PcJuice& J)
		{
			const uint64_t I = J.Draws++;
			if (!J.RecordedLeansDeg.empty())
			{
				return I < J.RecordedLeansDeg.size() ? J.RecordedLeansDeg[I] : 0.0;
			}
			// randf_range(from, to) = randd() * (to - from) + from.
			return RngUnit(J.Seed, I, 0, Salt::Jitter) * (JuiceJitterDeg - -JuiceJitterDeg) + -JuiceJitterDeg;
		}

		// _jitter_step(): the next lean, or asleep when hidden.
		void JitterStep(PcJuice& J, bool bVisible)
		{
			if (!J.bJitter)
			{
				return;
			}
			if (!bVisible)
			{
				J.bJitter = false;
				J.Rotation = 0.0;
				return;
			}
			const double Lean = NextLeanDeg(J) * Pi / 180.0; // deg_to_rad
			J.RotTween = OneLeg(Lean, 0.0, JuiceJitterStepSec, PcTrans::Sine, PcEase::InOut);
			J.RotTween.bJitterStep = true;
		}

		// Tween::step over sequential tweeners (PropertyTweener::step): the leftover of a leg that ends runs the next one, which
		// starts from the value the last one left. Returns true when the tween finished during this step.
		bool StepTween(PcJuiceTween& T, double& X, double* Y, double Dt)
		{
			if (!T.bLive)
			{
				return false;
			}
			auto Start = [&]()
			{
				T.FromX = X;
				T.FromY = Y ? *Y : 0.0;
				T.Elapsed = 0.0;
				T.bStarted = true;
			};
			if (!T.bStarted)
			{
				Start();
			}
			double Rem = Dt;
			while (Rem > 0.0 && T.bLive)
			{
				const PcJuiceTween::Leg& L = T.Legs[T.At];
				T.Elapsed += Rem;
				if (T.Elapsed < L.Duration)
				{
					X = TweenInterpolate(T.FromX, L.ToX - T.FromX, T.Elapsed, L.Duration, L.Trans, L.Ease);
					if (Y)
					{
						*Y = TweenInterpolate(T.FromY, L.ToY - T.FromY, T.Elapsed, L.Duration, L.Trans, L.Ease);
					}
					Rem = 0.0;
				}
				else
				{
					X = L.ToX;
					if (Y)
					{
						*Y = L.ToY;
					}
					Rem = T.Elapsed - L.Duration;
					if (++T.At == T.LegCount)
					{
						T.bLive = false;
						return true;
					}
					Start();
				}
			}
			return false;
		}
	}

	double TweenInterpolate(double Initial, double Delta, double Elapsed, double Duration, PcTrans Trans, PcEase Ease)
	{
		// Tween::interpolate_variant: the equation from 0 by 1, then Initial lerped towards Initial + Delta.
		const double E = Run(Trans, Ease, Elapsed, 0.0, 1.0, Duration);
		return Initial + Delta * E;
	}

	void JuiceHoverEnter(PcJuice& J, bool bDisabled)
	{
		if (!bDisabled)
		{
			ScaleTo(J, JuiceHoverScale, JuiceHoverInSec, PcTrans::Back);
		}
	}

	void JuiceHoverExit(PcJuice& J)
	{
		ScaleTo(J, 1.0, JuiceHoverOutSec, PcTrans::Back);
	}

	void JuicePressDown(PcJuice& J)
	{
		ScaleTo(J, JuicePressScale, JuicePressSec, PcTrans::Quad);
	}

	void JuicePressUp(PcJuice& J, bool bHoveredEnabled)
	{
		ScaleTo(J, bHoveredEnabled ? JuiceHoverScale : 1.0, JuiceReleaseSec, PcTrans::Elastic);
	}

	void JuicePunch(PcJuice& J)
	{
		PcJuiceTween T;
		T.Legs[0] = { JuicePunchScaleX, JuicePunchScaleY, JuicePunchSec * 0.25, PcTrans::Quad, PcEase::Out };
		T.Legs[1] = { 1.0, 1.0, JuicePunchSec * 0.75, PcTrans::Elastic, PcEase::Out };
		T.LegCount = 2;
		T.bLive = true;
		J.ScaleTween = T;
	}

	void JuicePopIn(PcJuice& J)
	{
		J.ScaleX = JuicePopInFrom;
		J.ScaleY = JuicePopInFrom;
		J.ScaleTween = OneLeg(1.0, 1.0, JuicePopInSec, PcTrans::Back, PcEase::Out);
	}

	void JuiceSetJitter(PcJuice& J, bool bOn, bool bEffects, bool bVisible)
	{
		if (bOn && !bEffects)
		{
			bOn = false;
		}
		if (bOn == J.bJitter)
		{
			return;
		}
		J.bJitter = bOn;
		if (bOn)
		{
			JitterStep(J, bVisible);
		}
		else
		{
			J.RotTween = OneLeg(0.0, 0.0, JuiceJitterStepSec * 2.0, PcTrans::Sine, PcEase::Out);
		}
	}

	void JuiceStep(PcJuice& J, double Dt, bool bVisible)
	{
		StepTween(J.ScaleTween, J.ScaleX, &J.ScaleY, Dt);
		const bool bWasJitterStep = J.RotTween.bJitterStep;
		if (StepTween(J.RotTween, J.Rotation, nullptr, Dt) && bWasJitterStep)
		{
			// The step's `finished`: the next one starts on the next frame (a tween made during the SceneTree's tween pass is
			// not stepped in it), the leftover dropped.
			JitterStep(J, bVisible);
		}
	}

	bool JuiceAtRest(const PcJuice& J)
	{
		return !J.ScaleTween.bLive && !J.RotTween.bLive && !J.bJitter && J.ScaleX == 1.0 && J.ScaleY == 1.0 && J.Rotation == 0.0;
	}
}
