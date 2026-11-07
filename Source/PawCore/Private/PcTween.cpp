#include "PcTween.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	PcTweener& PcTween::Add(const PcTweener& T)
	{
		if (Steps.empty() || !(bNextParallel || bAllParallel))
		{
			Steps.emplace_back();
		}
		bNextParallel = false;
		Steps.back().push_back(T);
		return Steps.back().back();
	}

	PcTweener& PcTween::Property(int Id, double To, double Duration, PcTrans Trans, PcEase Ease)
	{
		PcTweener T;
		T.What = PcTweener::Kind::Property;
		T.Id = Id;
		T.To = To;
		T.Duration = Duration;
		T.Trans = Trans;
		T.Ease = Ease;
		return Add(T);
	}

	PcTweener& PcTween::Method(int Id, double From, double To, double Duration, PcTrans Trans, PcEase Ease)
	{
		PcTweener T;
		T.What = PcTweener::Kind::Method;
		T.Id = Id;
		T.From = From;
		T.To = To;
		T.Duration = Duration;
		T.Trans = Trans;
		T.Ease = Ease;
		return Add(T);
	}

	PcTweener& PcTween::Callback(int Id)
	{
		PcTweener T;
		T.What = PcTweener::Kind::Callback;
		T.Id = Id;
		return Add(T);
	}

	PcTweener& PcTween::Interval(double Duration)
	{
		PcTweener T;
		T.What = PcTweener::Kind::Interval;
		T.Duration = Duration;
		return Add(T);
	}

	void PcTween::StartStep(const PcTweenHost& Host)
	{
		// Tween::start_tweeners -> each Tweener::start(): the clock from zero; a property reads where it starts from now, or,
		// delayed, when its delay is over.
		for (PcTweener& T : Steps[Current])
		{
			T.Elapsed = 0.0;
			T.bFinished = false;
			if (T.What == PcTweener::Kind::Property)
			{
				if (T.Delay == 0.0)
				{
					T.Initial = Host.Get(T.Id);
				}
				else
				{
					T.bInitialPending = true;
				}
			}
		}
	}

	namespace
	{
		// One tweener's step(r_delta): true while it still runs; R is left as the time it did not use.
		bool StepTweener(PcTweener& T, double& R, const PcTweenHost& Host)
		{
			if (T.bFinished)
			{
				return false;
			}
			T.Elapsed += R;
			switch (T.What)
			{
			case PcTweener::Kind::Interval:
				if (T.Elapsed < T.Duration)
				{
					R = 0.0;
					return true;
				}
				T.bFinished = true;
				R = T.Elapsed - T.Duration;
				return false;
			case PcTweener::Kind::Callback:
				if (T.Elapsed < T.Delay)
				{
					R = 0.0;
					return true;
				}
				Host.Call(T.Id);
				T.bFinished = true;
				R = T.Elapsed - T.Delay;
				return false;
			default:
				break;
			}
			if (T.Elapsed < T.Delay)
			{
				R = 0.0;
				return true;
			}
			if (T.bInitialPending)
			{
				T.Initial = Host.Get(T.Id);
				T.bInitialPending = false;
			}
			const double Time = std::min(T.Elapsed - T.Delay, T.Duration);
			const double From = T.What == PcTweener::Kind::Method ? T.From : T.Initial;
			if (Time < T.Duration)
			{
				Host.Set(T.Id, TweenInterpolate(From, T.To - From, Time, T.Duration, T.Trans, T.Ease));
				R = 0.0;
				return true;
			}
			Host.Set(T.Id, T.To);
			T.bFinished = true;
			R = T.Elapsed - T.Delay - T.Duration;
			return false;
		}
	}

	bool PcTween::Step(double Dt, const PcTweenHost& Host)
	{
		if (bDead || Steps.empty())
		{
			return false;
		}
		if (!bStarted)
		{
			Current = 0;
			StartStep(Host);
			bStarted = true;
		}
		double Rem = Dt;
		while (Rem > 0.0 && !bDead)
		{
			double StepDelta = Rem;
			bool bActive = false;
			for (PcTweener& T : Steps[Current])
			{
				double Temp = Rem;
				bActive = StepTweener(T, Temp, Host) || bActive;
				StepDelta = std::min(Temp, StepDelta);
			}
			Rem = StepDelta;
			if (!bActive)
			{
				++Current;
				if (Current == Steps.size())
				{
					bDead = true; // finished (no loops)
					break;
				}
				StartStep(Host);
			}
		}
		return !bDead;
	}
}
