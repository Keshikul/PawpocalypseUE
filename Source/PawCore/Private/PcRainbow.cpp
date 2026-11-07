#include "PcRainbow.h"

#include "PcRng.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		constexpr double Pi = 3.14159265358979323846;
	}

	PcColor GodotFromHsv(double H, double S, double V, double A)
	{
		// Color::set_hsv (core/math/color.cpp).
		PcColor C;
		C.A = A;
		if (S == 0.0)
		{
			C.R = C.G = C.B = V;
			return C;
		}
		H *= 6.0;
		H = std::fmod(H, 6.0);
		const int I = static_cast<int>(std::floor(H));
		const double F = H - I;
		const double P = V * (1.0 - S);
		const double Q = V * (1.0 - S * F);
		const double T = V * (1.0 - S * (1.0 - F));
		switch (I)
		{
		case 0: C.R = V; C.G = T; C.B = P; break;
		case 1: C.R = Q; C.G = V; C.B = P; break;
		case 2: C.R = P; C.G = V; C.B = T; break;
		case 3: C.R = P; C.G = Q; C.B = V; break;
		case 4: C.R = T; C.G = P; C.B = V; break;
		default: C.R = V; C.G = P; C.B = Q; break;
		}
		return C;
	}

	double GodotFposmod(double X, double Y)
	{
		// Math::fposmod: fmod, then moved into Y's sign.
		double Value = std::fmod(X, Y);
		if ((Value < 0.0 && Y > 0.0) || (Value > 0.0 && Y < 0.0))
		{
			Value += Y;
		}
		return Value + 0.0;
	}

	double GodotRangeValue(double Asked, double Min, double Max, double Step)
	{
		// Range::_set_value_no_signal: the step from min, then max (page 0), then min.
		double V = Asked;
		if (Step > 0.0)
		{
			V = std::round((V - Min) / Step) * Step + Min;
		}
		if (V > Max)
		{
			V = Max;
		}
		if (V < Min)
		{
			V = Min;
		}
		return V;
	}

	double RainbowBarFraction(double Value, double Min, double Max)
	{
		const double Span = Max - Min;
		if (Span <= 0.0)
		{
			return 0.0;
		}
		return std::clamp((Value - Min) / Span, 0.0, 1.0);
	}

	double PcRainbowBar::Unit()
	{
		const uint64_t I = Draws++;
		if (bReplay)
		{
			return I < Recorded.size() ? Recorded[I] : 0.0;
		}
		return RngUnit(Seed, I, 0, Salt::RainbowSpark);
	}

	double PcRainbowBar::Range(double From, double To)
	{
		// randf_range(from, to) = randd() * (to - from) + from; a recorded draw is the value itself.
		if (bReplay)
		{
			return Unit();
		}
		return Unit() * (To - From) + From;
	}

	void PcRainbowBar::Step(double Dt, double W, double H, double Fraction)
	{
		using namespace RainbowBar;
		Time += Dt;
		// _advance_sparks: aged, the old ones gone, the rest moved.
		std::vector<PcRainbowSpark> Alive;
		Alive.reserve(Sparks.size());
		for (PcRainbowSpark S : Sparks)
		{
			S.Age += Dt;
			if (S.Age >= SparkLifetime)
			{
				continue;
			}
			S.X += S.Vx * Dt;
			S.Y += S.Vy * Dt;
			Alive.push_back(S);
		}
		Sparks = std::move(Alive);
		// _spawn_sparks: none from an empty bar, nor from a full one already throwing plenty (Godot's `a or b and c`).
		if (Fraction <= 0.0 || (Fraction >= 1.0 && static_cast<int>(Sparks.size()) > SparkFullCap))
		{
			return;
		}
		Accumulator += SparkSpawnPerSec * Dt;
		while (Accumulator >= 1.0)
		{
			Accumulator -= 1.0;
			PcRainbowSpark S;
			S.X = W * Fraction + Range(-SparkJitterPx, SparkJitterPx);
			S.Y = Range(0.0, H);
			S.Vx = Range(-SparkDriftPx, SparkDriftPx);
			S.Vy = -Range(SparkMinRisePx, SparkRisePx);
			S.Hue = Unit();
			Sparks.push_back(S);
		}
	}

	void PcRainbowBar::Draw(double W, double H, double Fraction, std::vector<PcRainbowBand>& Bands, std::vector<PcRainbowDot>& Dots) const
	{
		using namespace RainbowBar;
		Bands.clear();
		Dots.clear();
		(void)H;
		const double FillWidth = W * Fraction;
		if (FillWidth > 0.0)
		{
			const double BandWidth = FillWidth / BandCount;
			for (int I = 0; I < BandCount; ++I)
			{
				const double Along = static_cast<double>(I) / BandCount;
				// The hue scrolls against time, so the pattern travels along the bar.
				const double Hue = GodotFposmod(Along * HueSpan - Time * HueCyclePerSec, 1.0);
				Bands.push_back({ BandWidth * I, BandWidth + 1.0, GodotFromHsv(Hue, BandSaturation, 1.0) });
			}
		}
		for (const PcRainbowSpark& S : Sparks)
		{
			const double Life = 1.0 - S.Age / SparkLifetime;
			Dots.push_back({ S.X, S.Y, SparkMaxRadius * Life,
				GodotFromHsv(GodotFposmod(S.Hue + Time * SparkHueCycle, 1.0), SparkSaturation, 1.0, Life) });
		}
	}

	PcRect RainbowFrameAround(double PlateW, double PlateH, double Margin)
	{
		return { -Margin, -Margin, PlateW + 2.0 * Margin, PlateH + 2.0 * Margin };
	}

	double RainbowFramePhase(double Phase, double Dt, double TurnsPerSec, bool bActiveAndShown)
	{
		return bActiveAndShown ? GodotFposmod(Phase + Dt * TurnsPerSec, 1.0) : Phase;
	}

	void RainbowFrameOutline(double W, double H, double Corner, double Phase, std::vector<double>& Xs, std::vector<double>& Ys,
		std::vector<PcColor>& Colors)
	{
		Xs.clear();
		Ys.clear();
		Colors.clear();
		const double HalfX = W / 2.0, HalfY = H / 2.0;
		const double R = std::min(Corner, std::min(HalfX, HalfY));
		// The four corners' centres and where each quarter starts: top-right, bottom-right, bottom-left, top-left.
		const double Cx[4] = { HalfX + (HalfX - R), HalfX + (HalfX - R), HalfX + (-HalfX + R), HalfX + (-HalfX + R) };
		const double Cy[4] = { HalfY + (-HalfY + R), HalfY + (HalfY - R), HalfY + (HalfY - R), HalfY + (-HalfY + R) };
		const double A0[4] = { -Pi / 2.0, 0.0, Pi / 2.0, Pi };
		for (int C = 0; C < 4; ++C)
		{
			for (int I = 0; I <= RainbowFrame::CornerSteps; ++I)
			{
				const double Ang = A0[C] + (Pi / 2.0) * static_cast<double>(I) / RainbowFrame::CornerSteps;
				Xs.push_back(Cx[C] + std::cos(Ang) * R);
				Ys.push_back(Cy[C] + std::sin(Ang) * R);
			}
		}
		Xs.push_back(Xs[0]);
		Ys.push_back(Ys[0]);
		const size_t N = Xs.size();
		for (size_t I = 0; I < N; ++I)
		{
			Colors.push_back(GodotFromHsv(GodotFposmod(static_cast<double>(I) / static_cast<double>(N - 1) + Phase, 1.0),
				RainbowFrame::Saturation, 1.0));
		}
	}
}
