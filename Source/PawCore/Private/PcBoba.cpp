#include "PcBoba.h"

#include "PcJuice.h"
#include "PcRng.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		constexpr double Pi = 3.14159265358979323846;
		constexpr uint64_t BobaSalt = 0xB0BA;

		double Uniform(PcBoba& B, double Lo, double Hi)
		{
			return Lo + (Hi - Lo) * RngUnit(B.Seed, B.Draws++, 0, BobaSalt);
		}

		void Schedule(PcBoba& B, double Lo, double Hi)
		{
			B.Timer = Uniform(B, Lo, Hi);
		}

		PcBobaKind PickKind(PcBoba& B)
		{
			double Total = 0.0;
			for (double W : BobaWeights)
			{
				Total += W;
			}
			double R = Uniform(B, 0.0, Total);
			for (int32_t K = 0; K < 5; ++K)
			{
				if (R < BobaWeights[K])
				{
					return static_cast<PcBobaKind>(K);
				}
				R -= BobaWeights[K];
			}
			return PcBobaKind::Drop;
		}

		// A photo, never the one before (unless there is only one).
		int32_t PickPhoto(PcBoba& B, int32_t Count)
		{
			if (Count <= 1)
			{
				return 0;
			}
			const bool bAvoid = B.LastPhoto >= 0 && B.LastPhoto < Count;
			const int32_t Choices = bAvoid ? Count - 1 : Count;
			int32_t P = std::min(Choices - 1, static_cast<int32_t>(std::floor(Uniform(B, 0.0, static_cast<double>(Choices)))));
			if (bAvoid && P >= B.LastPhoto)
			{
				++P;
			}
			return P;
		}

		void Start(PcBoba& B, PcBobaKind Kind, const std::vector<double>& Aspects, double H)
		{
			PcBobaCameo C;
			C.bLive = true;
			C.Kind = Kind;
			C.Photo = PickPhoto(B, static_cast<int32_t>(Aspects.size()));
			C.Aspect = Aspects[static_cast<size_t>(C.Photo)] > 0.0 ? Aspects[static_cast<size_t>(C.Photo)] : 1.0;
			C.Size = H * Uniform(B, BobaSizeMin, BobaSizeMax);
			switch (Kind)
			{
			case PcBobaKind::Flyby:
			case PcBobaKind::Spin:
			case PcBobaKind::Nyan:
				C.Duration = Kind == PcBobaKind::Nyan ? Uniform(B, 3.0, 4.2) : Uniform(B, 3.6, 5.6);
				C.LineY = H * Uniform(B, 0.25, 0.75);
				C.bRight = Uniform(B, 0.0, 1.0) < 0.5;
				C.Turns = Kind == PcBobaKind::Spin ? (Uniform(B, 0.0, 1.0) < 0.5 ? 1.0 : 2.0) : 0.0;
				break;
			case PcBobaKind::Peek:
				C.Edge = static_cast<PcBobaEdge>(std::min(3, static_cast<int32_t>(std::floor(Uniform(B, 0.0, 4.0)))));
				C.Along = Uniform(B, 0.2, 0.8);
				C.In = 0.7;
				C.Hold = Uniform(B, 1.4, 2.2);
				C.Out = 0.5;
				C.Duration = C.In + C.Hold + C.Out;
				break;
			case PcBobaKind::Drop:
				C.Along = Uniform(B, 0.2, 0.8);
				C.In = 1.3;
				C.Hold = Uniform(B, 1.0, 1.8);
				C.Out = 0.55;
				C.Duration = C.In + C.Hold + C.Out;
				break;
			}
			B.LastPhoto = C.Photo;
			B.Cameo = C;
		}

		bool CanStart(const PcBoba& B, bool bEffects, const std::vector<double>& Aspects)
		{
			return !B.Cameo.bLive && bEffects && !Aspects.empty() && B.Suppressors.empty();
		}

		// A hold's small wobble (the first try's jitter), degrees.
		double Wobble(double T)
		{
			return 3.0 * std::sin(2.0 * Pi * 6.0 * T);
		}
	}

	void BobaEnterMenu(PcBoba& B, const std::string& Tag)
	{
		const bool bFirst = B.Menus.empty();
		B.Menus.insert(Tag);
		if (bFirst && B.Timer < 0.0)
		{
			Schedule(B, BobaFirstMin, BobaFirstMax);
		}
	}

	void BobaLeaveMenu(PcBoba& B, const std::string& Tag)
	{
		B.Menus.erase(Tag);
		if (B.Menus.empty())
		{
			B.Timer = -1.0; // a cameo in flight finishes
		}
	}

	void BobaSuppress(PcBoba& B, const std::string& Tag, bool bOn)
	{
		if (bOn)
		{
			B.Suppressors.insert(Tag);
			B.Cameo.bLive = false; // never over the skill tree: cut, not finished
		}
		else
		{
			B.Suppressors.erase(Tag);
		}
	}

	bool BobaStep(PcBoba& B, double Dt, bool bEffects, const std::vector<double>& Aspects, double ScreenH)
	{
		if (B.Cameo.bLive)
		{
			B.Cameo.T += Dt;
			if (B.Cameo.T >= B.Cameo.Duration)
			{
				B.Cameo.bLive = false;
			}
		}
		if (B.Menus.empty() || B.Timer < 0.0)
		{
			return false;
		}
		B.Timer -= Dt;
		if (B.Timer > 0.0)
		{
			return false;
		}
		bool bStarted = false;
		if (CanStart(B, bEffects, Aspects))
		{
			Start(B, PickKind(B), Aspects, ScreenH);
			bStarted = true;
		}
		Schedule(B, BobaEveryMin, BobaEveryMax); // busy, off or held: only rescheduled
		return bStarted;
	}

	bool BobaForce(PcBoba& B, PcBobaKind Kind, bool bEffects, const std::vector<double>& Aspects, double ScreenH)
	{
		if (!CanStart(B, bEffects, Aspects))
		{
			return false;
		}
		Start(B, Kind, Aspects, ScreenH);
		return true;
	}

	bool BobaForceAny(PcBoba& B, bool bEffects, const std::vector<double>& Aspects, double ScreenH)
	{
		if (!CanStart(B, bEffects, Aspects))
		{
			return false;
		}
		Start(B, PickKind(B), Aspects, ScreenH);
		return true;
	}

	PcBobaPose BobaPose(const PcBobaCameo& C, double W, double H, double T)
	{
		if (T < 0.0)
		{
			T = C.T;
		}
		PcBobaPose P;
		P.bVisible = C.bLive && T >= 0.0 && T < C.Duration;
		const double HalfW = C.Size * C.Aspect * 0.5;
		const double HalfH = C.Size * 0.5;
		switch (C.Kind)
		{
		case PcBobaKind::Flyby:
		case PcBobaKind::Spin:
		case PcBobaKind::Nyan:
		{
			const double S = C.Duration > 0.0 ? std::clamp(T / C.Duration, 0.0, 1.0) : 1.0;
			const double X0 = C.bRight ? -HalfW : W + HalfW;
			const double X1 = C.bRight ? W + HalfW : -HalfW;
			P.X = X0 + (X1 - X0) * S;
			const double Bob = C.Size * (C.Kind == PcBobaKind::Nyan ? 0.05 : 0.08);
			P.Y = C.LineY + std::sin(2.0 * Pi * 2.2 * S) * Bob;
			if (C.Kind == PcBobaKind::Spin)
			{
				P.Rotation = TweenInterpolate(0.0, 360.0 * C.Turns * (C.bRight ? 1.0 : -1.0), std::min(T, C.Duration), C.Duration,
					PcTrans::Sine, PcEase::InOut);
			}
			else if (C.Kind == PcBobaKind::Nyan)
			{
				P.Rotation = 5.0 * std::sin(2.0 * Pi * 1.5 * T);
			}
			P.bFlip = C.bRight;
			break;
		}
		case PcBobaKind::Peek:
		{
			double K = 1.0;
			if (T < C.In)
			{
				K = TweenInterpolate(0.0, 1.0, T, C.In, PcTrans::Back, PcEase::Out);
			}
			else if (T >= C.In + C.Hold)
			{
				K = 1.0 - TweenInterpolate(0.0, 1.0, std::min(T - C.In - C.Hold, C.Out), C.Out, PcTrans::Quad, PcEase::In);
			}
			else
			{
				P.Rotation = Wobble(T - C.In);
			}
			const bool bAcross = C.Edge == PcBobaEdge::Left || C.Edge == PcBobaEdge::Right;
			const double Shown = K * BobaPeekShow * (bAcross ? 2.0 * HalfW : 2.0 * HalfH);
			switch (C.Edge)
			{
			case PcBobaEdge::Left:
				P.X = -HalfW + Shown;
				P.Y = H * C.Along;
				break;
			case PcBobaEdge::Right:
				P.X = W + HalfW - Shown;
				P.Y = H * C.Along;
				P.bFlip = true; // facing in
				break;
			case PcBobaEdge::Top:
				P.X = W * C.Along;
				P.Y = -HalfH + Shown;
				P.Rotation += 180.0; // upside down from the top
				break;
			case PcBobaEdge::Bottom:
				P.X = W * C.Along;
				P.Y = H + HalfH - Shown;
				break;
			}
			break;
		}
		case PcBobaKind::Drop:
		{
			P.X = W * C.Along;
			const double From = -HalfH;
			const double Land = H * BobaDropLand;
			const double Gone = H + HalfH;
			if (T < C.In)
			{
				P.Y = TweenInterpolate(From, Land - From, T, C.In, PcTrans::Bounce, PcEase::Out);
			}
			else if (T < C.In + C.Hold)
			{
				P.Y = Land;
				P.Rotation = Wobble(T - C.In);
			}
			else
			{
				P.Y = TweenInterpolate(Land, Gone - Land, std::min(T - C.In - C.Hold, C.Out), C.Out, PcTrans::Quad, PcEase::In);
			}
			break;
		}
		}
		return P;
	}

	std::vector<std::pair<double, double>> BobaTrail(const PcBobaCameo& C, double W, double H)
	{
		std::vector<std::pair<double, double>> Out;
		if (!C.bLive || C.Kind != PcBobaKind::Nyan || C.Duration <= 0.0)
		{
			return Out;
		}
		const double Across = W + C.Size * C.Aspect; // the whole line, off-screen to off-screen
		const double Speed = Across / C.Duration;
		const double Step = Speed > 0.0 ? BobaTrailStep / Speed : C.Duration;
		for (int32_t K = 0; K < BobaTrailPoints; ++K)
		{
			const double Tk = C.T - static_cast<double>(K) * Step;
			if (Tk < 0.0)
			{
				break;
			}
			const PcBobaPose P = BobaPose(C, W, H, Tk);
			Out.emplace_back(P.X, P.Y);
		}
		return Out;
	}
}
