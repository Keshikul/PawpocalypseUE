#pragma once

// The Unicorn ceremony (unicorn_ceremony.gd, phase 6 slice 6a; designer 2026-10-01: "a horse sound is played then a unicorn
// hops into the frame, a large one, it rears and yells again and a rainbow is formed behind him and stars are falling and
// it's a fiesta with confetti"). Played once over the skill tree when the Unicorn node is bought. Pure state stepped by its
// owner: the dim, the mounted cat's hops / rear / settle (a PcTween timeline, Godot's own), the rainbow's growth, the
// confetti and stars (drag, sway, spin), the skip after 0.6 s, the fade at 3.4 s, and what it draws. Its random draws come
// from PcRng, or, for a test, from the draws Godot recorded. Ground truth: the fixture `unicorn` (the real script, its draw
// calls, random draws and sounds renamed to recorders, its tweens stepped by custom_step). Godot's Vector2s are float32,
// PawCore's doubles: compared to a tolerance.

#include "PcCore.h"
#include "PcRainbow.h" // PcColor
#include "PcTween.h"

#include <cstdint>
#include <vector>

namespace Pc
{
	// unicorn_ceremony.gd's constants (Data/CodeConstantsMap.csv: presentation).
	namespace Unicorn
	{
		constexpr double HeightShare = 0.55; // HEIGHT_SHARE: the cat's square, of the view's height
		constexpr double RestShare = 0.62;   // where its feet rest, of the view's height
		constexpr double EnterShare = 0.2;   // it starts this much of its side past the right edge
		constexpr double HopSec = 0.32;      // HOP_SEC
		constexpr int Hops = 3;              // HOPS
		constexpr double HopShare = 0.22;    // a hop's height, of its side
		constexpr double DimAlpha = 0.55, DimSec = 0.25;
		constexpr double RearDeg = -16.0, RearSec = 0.18, RearScale = 1.12;
		constexpr double SettleSec = 0.35, SettleDelay = 0.25;
		constexpr double RainbowAt = 1.25;   // RAINBOW_AT
		constexpr double RainbowGrowSec = 0.55;
		constexpr double FadeAt = 3.40;      // FADE_AT
		constexpr double FadeSec = 0.35;     // FADE_SEC
		constexpr double SkipAfter = 0.6;    // SKIP_AFTER
		constexpr int Confetti = 70;         // CONFETTI
		constexpr int ConfettiRain = 110;    // CONFETTI_RAIN
		constexpr int Stars = 26;            // STARS
		constexpr int ArcPoints = 64;
		constexpr double StarRimPx = 2.0;
		// RAINBOW: #E8453C #F39237 #F6D04D #6CC56B #4FA3E0 #9B6BD6.
		constexpr uint8_t RainbowBytes[6][3] = { { 0xE8, 0x45, 0x3C }, { 0xF3, 0x92, 0x37 }, { 0xF6, 0xD0, 0x4D },
			{ 0x6C, 0xC5, 0x6B }, { 0x4F, 0xA3, 0xE0 }, { 0x9B, 0x6B, 0xD6 } };
	}

	enum class PcUnicornSound : uint8_t { Neigh, Fiesta }; // unicorn_neigh, boss_defeated
	// What reached _input: only a mouse button down, a key down that is not an echo, or a pad button down skips.
	enum class PcUnicornInput : uint8_t { MouseDown, KeyDown, KeyEcho, KeyUp, PadDown, Other };

	struct PcUnicornBit
	{
		double X = 0.0, Y = 0.0, Vx = 0.0, Vy = 0.0, Rot = 0.0, Spin = 0.0, Sx = 0.0, Sy = 0.0;
		int Colour = 0;      // the RAINBOW band; a star is gold
		bool bStar = false;
		bool bRain = false;  // the curtain falling across the screen (keeps its speed, sways)
	};

	struct PcUnicornCeremony
	{
		// The view and the cat's square.
		double ViewW = 0.0, ViewH = 0.0, Side = 0.0, RestX = 0.0, RestY = 0.0, StartX = 0.0;
		// What is shown: the clock, the dim's alpha, the cat (top-left of its square, rotation and scale about the middle of
		// its foot), the whole layer's alpha, the rainbow's growth 0..1.
		double Clock = 0.0, DimA = 0.0, X = 0.0, Y = 0.0, Rot = 0.0, Scale = 1.0, RootA = 1.0, Grow = 0.0;
		bool bDone = false;      // fading out (or faded)
		bool bFinished = false;  // `finished`: hand the tree back
		std::vector<PcUnicornBit> Bits;
		std::vector<PcUnicornSound> Sounds; // played since the owner last cleared it

		uint64_t Seed = 0;
		uint64_t Draws = 0;
		bool bReplay = false;
		std::vector<double> Recorded;

		// _ready / _start: laid out for the view, the first whinny, the timeline.
		PAWCORE_API void Start(double W, double H);
		// _input: true when the input skips to the fade (and is eaten).
		PAWCORE_API bool Input(PcUnicornInput What);
		// One frame: _process (the clock, the rainbow, the bits, the fade at 3.4 s), then the tweens, as Godot steps them.
		PAWCORE_API void Step(double Dt);
		bool IsPlaying() const { return !bDone; }

	private:
		void End();
		void Fiesta();
		double Range(double From, double To);
		double Unit();
		PcTweenHost Host();
		PcTween Main, Fade;
		int HopIndex = 0;
	};

	struct PcUnicornArc
	{
		double Cx = 0.0, Cy = 0.0, Radius = 0.0, From = 0.0, To = 0.0, Width = 0.0;
		PcColor Color;
	};
	struct PcUnicornShape
	{
		bool bRim = false;      // a star's ink rim (a closed polyline, StarRimPx wide), else a filled polygon
		PcColor Color;
		std::vector<double> Xs, Ys;
	};

	// _draw_sky(): the rainbow's six arcs (none before it grows), then each bit: a confetti quad, or a star and its rim.
	PAWCORE_API void UnicornArcs(const PcUnicornCeremony& C, std::vector<PcUnicornArc>& Out);
	PAWCORE_API void UnicornBitShapes(const PcUnicornBit& B, std::vector<PcUnicornShape>& Out);
	PAWCORE_API PcColor UnicornRainbow(int Band);
}
