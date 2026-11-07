#include "PcUnicorn.h"

#include "PcBackdropTones.h"
#include "PcRng.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		constexpr double Pi = 3.14159265358979323846;
		constexpr double Tau = 6.28318530717958647692;

		// What the timeline drives (PcTween ids).
		enum Prop : int { PDim, PRot, PScale, PRoot };
		enum Meth : int { MHop0 = 10, MHop1, MHop2 };
		enum Call : int { CNeigh = 20, CFiesta, CFinished };

		double Lerp(double A, double B, double T) { return A + (B - A) * T; }

		PcColor Tone(const PcTone& T)
		{
			return { T.R / 255.0, T.G / 255.0, T.B / 255.0, T.A / 255.0 };
		}
	}

	PcColor UnicornRainbow(int Band)
	{
		const uint8_t* B = Unicorn::RainbowBytes[((Band % 6) + 6) % 6];
		return { B[0] / 255.0, B[1] / 255.0, B[2] / 255.0, 1.0 };
	}

	double PcUnicornCeremony::Unit()
	{
		const uint64_t I = Draws++;
		if (bReplay)
		{
			return I < Recorded.size() ? Recorded[I] : 0.0;
		}
		return RngUnit(Seed, I, 0, Salt::UnicornFiesta);
	}

	double PcUnicornCeremony::Range(double From, double To)
	{
		// randf_range(from, to) = randd() * (to - from) + from; a recorded draw is the value itself.
		return bReplay ? Unit() : Unit() * (To - From) + From;
	}

	PcTweenHost PcUnicornCeremony::Host()
	{
		PcTweenHost H;
		H.Get = [this](int Id) -> double
		{
			switch (Id)
			{
			case PDim: return DimA;
			case PRot: return Rot;
			case PScale: return Scale;
			default: return RootA;
			}
		};
		H.Set = [this](int Id, double V)
		{
			switch (Id)
			{
			case PDim: DimA = V; break;
			case PRot: Rot = V; break;
			case PScale: Scale = V; break;
			case PRoot: RootA = V; break;
			default:
			{
				// A hop: from one third of the way to the next, an arc of a sine.
				const int I = Id - MHop0;
				const double FromX = Lerp(StartX, RestX, static_cast<double>(I) / Unicorn::Hops);
				const double ToX = Lerp(StartX, RestX, static_cast<double>(I + 1) / Unicorn::Hops);
				X = Lerp(FromX, ToX, V);
				Y = RestY - std::sin(V * Pi) * Side * Unicorn::HopShare;
				break;
			}
			}
		};
		H.Call = [this](int Id)
		{
			if (Id == CNeigh)
			{
				Sounds.push_back(PcUnicornSound::Neigh);
			}
			else if (Id == CFiesta)
			{
				Fiesta();
			}
			else
			{
				bFinished = true;
			}
		};
		return H;
	}

	void PcUnicornCeremony::Start(double W, double H)
	{
		using namespace Unicorn;
		ViewW = W;
		ViewH = H;
		Side = H * HeightShare;
		RestX = (W - Side) * 0.5;
		RestY = H * RestShare - Side;
		X = W + Side * EnterShare;
		Y = RestY;
		StartX = X;
		Sounds.push_back(PcUnicornSound::Neigh);
		Main = PcTween();
		Main.Property(PDim, DimAlpha, DimSec);
		for (int I = 0; I < Hops; ++I)
		{
			Main.Method(MHop0 + I, 0.0, 1.0, HopSec);
		}
		// The rear: back on its hind legs, bigger, and the second whinny.
		Main.Callback(CNeigh);
		Main.Property(PRot, RearDeg * Pi / 180.0, RearSec, PcTrans::Back);
		Main.Parallel();
		Main.Property(PScale, RearScale, RearSec);
		Main.Callback(CFiesta);
		Main.Property(PRot, 0.0, SettleSec, PcTrans::Elastic).Delay = SettleDelay;
		Main.Parallel();
		Main.Property(PScale, 1.0, SettleSec).Delay = SettleDelay;
	}

	void PcUnicornCeremony::Fiesta()
	{
		using namespace Unicorn;
		Sounds.push_back(PcUnicornSound::Fiesta);
		// From the cat's chest: the burst.
		const double Ox = X + Side * 0.5, Oy = Y + Side * 0.3;
		for (int I = 0; I < Confetti; ++I)
		{
			PcUnicornBit B;
			const double A = Range(-Pi * 0.95, -Pi * 0.05);
			const double Speed = Range(420.0, 980.0);
			B.X = Ox;
			B.Y = Oy;
			B.Vx = std::cos(A) * Speed;
			B.Vy = std::sin(A) * Speed;
			B.Spin = Range(-9.0, 9.0);
			B.Rot = Unit() * Tau;
			B.Sx = Range(8.0, 14.0);
			B.Sy = Range(4.0, 7.0);
			B.Colour = I % 6;
			Bits.push_back(B);
		}
		// A curtain falling across the whole screen, so the fiesta fills the frame.
		for (int I = 0; I < ConfettiRain; ++I)
		{
			PcUnicornBit B;
			B.X = Unit() * ViewW;
			B.Y = Range(-ViewH * 0.5, ViewH * 0.1);
			B.Vx = Range(-60.0, 60.0);
			B.Vy = Range(160.0, 360.0);
			B.Spin = Range(-7.0, 7.0);
			B.Rot = Unit() * Tau;
			B.Sx = Range(8.0, 14.0);
			B.Sy = Range(4.0, 7.0);
			B.Colour = I % 6;
			B.bRain = true;
			Bits.push_back(B);
		}
		for (int I = 0; I < Stars; ++I)
		{
			PcUnicornBit B;
			B.X = Unit() * ViewW;
			B.Y = Range(-ViewH * 0.5, -20.0);
			B.Vx = Range(-40.0, 40.0);
			B.Vy = Range(220.0, 420.0);
			B.Spin = Range(-3.0, 3.0);
			B.Rot = Unit() * Tau;
			B.Sx = B.Sy = Range(10.0, 20.0);
			B.bStar = true;
			Bits.push_back(B);
		}
	}

	bool PcUnicornCeremony::Input(PcUnicornInput What)
	{
		if (bDone || Clock < Unicorn::SkipAfter)
		{
			return false;
		}
		if (What == PcUnicornInput::MouseDown || What == PcUnicornInput::KeyDown || What == PcUnicornInput::PadDown)
		{
			End();
			return true;
		}
		return false;
	}

	void PcUnicornCeremony::End()
	{
		if (bDone)
		{
			return;
		}
		bDone = true;
		Fade = PcTween();
		Fade.Property(PRoot, 0.0, Unicorn::FadeSec);
		Fade.Callback(CFinished);
	}

	void PcUnicornCeremony::Step(double Dt)
	{
		// _process.
		Clock += Dt;
		if (Clock >= Unicorn::RainbowAt)
		{
			Grow = std::min(Grow + Dt / Unicorn::RainbowGrowSec, 1.0);
		}
		for (PcUnicornBit& B : Bits)
		{
			const double Fall = (B.bStar || B.bRain) ? 60.0 : 900.0;
			double Vx = B.Vx, Vy = B.Vy + Fall * Dt;
			// Confetti flutters: the rain keeps its speed and sways; the burst slows as it climbs and tumbles back.
			if (B.bRain)
			{
				Vx += std::sin(Clock * 5.0 + B.Rot) * 60.0 * Dt;
			}
			else if (!B.bStar)
			{
				Vx *= 1.0 - 1.6 * Dt;
				Vy *= 1.0 - 1.6 * Dt;
				Vx += std::sin(Clock * 7.0 + B.Rot) * 30.0 * Dt;
			}
			B.Vx = Vx;
			B.Vy = Vy;
			B.X += Vx * Dt;
			B.Y += Vy * Dt;
			B.Rot += B.Spin * Dt;
		}
		if (!bDone && Clock >= Unicorn::FadeAt)
		{
			End();
		}
		// Then the SceneTree steps the tweens, in the order they were made.
		const PcTweenHost H = Host();
		Main.Step(Dt, H);
		Fade.Step(Dt, H);
	}

	void UnicornArcs(const PcUnicornCeremony& C, std::vector<PcUnicornArc>& Out)
	{
		Out.clear();
		if (C.Grow <= 0.0)
		{
			return;
		}
		// Six bands behind the cat, sweeping up from the left.
		const double Cx = C.X + C.Side * 0.5, Cy = C.Y + C.Side * 0.95;
		const double Outer = C.Side * 0.95;
		const double Band = Outer * 0.075;
		const double Sweep = Pi * C.Grow;
		for (int I = 0; I < 6; ++I)
		{
			Out.push_back({ Cx, Cy, Outer - Band * I, Pi, Pi + Sweep, Band + 1.0, UnicornRainbow(I) });
		}
	}

	void UnicornBitShapes(const PcUnicornBit& B, std::vector<PcUnicornShape>& Out)
	{
		Out.clear();
		if (B.bStar)
		{
			// _star(): ten points, every other one at 0.45 of the radius.
			PcUnicornShape S;
			S.Color = Tone(Gold);
			for (int I = 0; I < 10; ++I)
			{
				const double A = B.Rot + Tau * static_cast<double>(I) / 10.0 - Pi / 2.0;
				const double R = I % 2 == 0 ? B.Sx : B.Sx * 0.45;
				S.Xs.push_back(B.X + std::cos(A) * R);
				S.Ys.push_back(B.Y + std::sin(A) * R);
			}
			PcUnicornShape Rim = S;
			Rim.bRim = true;
			Rim.Color = Tone(Ink);
			Rim.Xs.push_back(S.Xs[0]);
			Rim.Ys.push_back(S.Ys[0]);
			Out.push_back(S);
			Out.push_back(Rim);
			return;
		}
		// A quad turned by its rotation about its centre.
		PcUnicornShape S;
		S.Color = UnicornRainbow(B.Colour);
		const double Hx = B.Sx * 0.5, Hy = B.Sy * 0.5;
		const double Cs = std::cos(B.Rot), Sn = std::sin(B.Rot);
		const double Lx[4] = { -Hx, Hx, Hx, -Hx }, Ly[4] = { -Hy, -Hy, Hy, Hy };
		for (int I = 0; I < 4; ++I)
		{
			S.Xs.push_back(B.X + Lx[I] * Cs - Ly[I] * Sn);
			S.Ys.push_back(B.Y + Lx[I] * Sn + Ly[I] * Cs);
		}
		Out.push_back(S);
	}
}
