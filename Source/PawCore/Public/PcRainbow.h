#pragma once

// The rainbow pieces (phase 6 slice 5d): Godot's rainbow_bar.gd (the Reawaken page's bar: a scrolling rainbow fill and the
// sparks thrown off its leading edge) and rainbow_frame.gd (the turning rainbow outline: the zone plate's when it shouts,
// Bait or a biome to go to). Pure state stepped by its owner's tick, and the shapes Godot's _draw() makes from it; the game
// paints them. Ground truth: the fixture `rainbow` (the real scripts compiled from their own source with only their draw
// calls renamed to recorders; the bar's random draws recorded so a test can replay them). Godot keeps positions in float32
// Vector2s, PawCore in doubles: compared to a tolerance.

#include "PcCore.h"
#include "PcFieldRules.h" // PcRect

#include <cstdint>
#include <vector>

namespace Pc
{
	// A Godot Color: straight alpha, sRGB channels 0..1.
	struct PcColor
	{
		double R = 0.0, G = 0.0, B = 0.0, A = 1.0;
	};

	// Color.from_hsv(h, s, v, a): h in turns (wrapped), the sextant and its fraction.
	PAWCORE_API PcColor GodotFromHsv(double H, double S, double V, double A = 1.0);
	// fposmod(x, y): the remainder with the sign of y.
	PAWCORE_API double GodotFposmod(double X, double Y);
	// Range.set_value(): snapped to the step from min (when step > 0), then held in [min, max] (no allow_greater / lesser).
	// A ProgressBar keeps its Range's default step, 0.01: the bars fill in hundredths.
	PAWCORE_API double GodotRangeValue(double Asked, double Min, double Max, double Step);
	constexpr double GodotRangeStep = 0.01;

	// rainbow_bar.gd's constants (Data/CodeConstantsMap.csv: presentation).
	namespace RainbowBar
	{
		constexpr double HueCyclePerSec = 0.22;   // HUE_CYCLE_PER_SEC
		constexpr double HueSpan = 0.75;          // HUE_SPAN
		constexpr int BandCount = 48;             // BAND_COUNT
		constexpr double BandSaturation = 0.72;   // the bands' Color.from_hsv(hue, 0.72, 1.0)
		constexpr double SparkSpawnPerSec = 14.0; // SPARK_SPAWN_PER_SEC
		constexpr double SparkLifetime = 0.7;     // SPARK_LIFETIME
		constexpr double SparkRisePx = 26.0;      // SPARK_RISE_PX
		constexpr double SparkDriftPx = 18.0;     // SPARK_DRIFT_PX
		constexpr double SparkMaxRadius = 2.6;    // SPARK_MAX_RADIUS
		constexpr double SparkJitterPx = 4.0;     // randf_range(-4.0, 4.0) about the leading edge
		constexpr double SparkMinRisePx = 6.0;    // randf_range(6.0, SPARK_RISE_PX)
		constexpr int SparkFullCap = 24;          // a full bar stops throwing sparks above this many
		constexpr double SparkHueCycle = 0.3;     // a spark's hue walks at _time * 0.3
		constexpr double SparkSaturation = 0.45;
	}

	struct PcRainbowSpark
	{
		double X = 0.0, Y = 0.0, Vx = 0.0, Vy = 0.0, Age = 0.0, Hue = 0.0;
	};
	// One band of the fill: a full-height rect from X, W wide (one px over, so the bands never leave a seam).
	struct PcRainbowBand
	{
		double X = 0.0, W = 0.0;
		PcColor Color;
	};
	// A filled circle (a spark, a star).
	struct PcRainbowDot
	{
		double X = 0.0, Y = 0.0, Radius = 0.0;
		PcColor Color;
	};

	// _fill_fraction(): where the value sits in the range, 0..1 (0 for an empty range).
	PAWCORE_API double RainbowBarFraction(double Value, double Min, double Max);

	// The bar's state: its clock, the sparks, the spawn accumulator. Its random draws come from PcRng (Seed, a counter), or,
	// for a test, from the draws Godot recorded (Recorded, in order; Godot's global randf_range / randf).
	struct PcRainbowBar
	{
		double Time = 0.0;
		double Accumulator = 0.0;
		std::vector<PcRainbowSpark> Sparks;
		uint64_t Seed = 0;
		uint64_t Draws = 0;
		bool bReplay = false;          // a test: draw from Recorded, not from PcRng
		std::vector<double> Recorded;  // Godot's draws, the values randf_range / randf returned, in order

		// _process(delta): the clock, the sparks aged and moved, new ones from the leading edge. W, H: the bar's size.
		PAWCORE_API void Step(double Dt, double W, double H, double Fraction);
		// _draw(): the bands (nothing for an empty bar), then the sparks.
		PAWCORE_API void Draw(double W, double H, double Fraction, std::vector<PcRainbowBand>& Bands, std::vector<PcRainbowDot>& Dots) const;

	private:
		double Range(double From, double To);
		double Unit();
	};

	// rainbow_frame.gd's defaults (presentation): the zone plate keeps them but its width; the Reawaken banner sets its own.
	namespace RainbowFrame
	{
		constexpr double Width = 4.0;         // width
		constexpr double Corner = 8.0;        // corner
		constexpr double TurnsPerSec = 0.12;  // turns_per_sec
		constexpr double Margin = 3.0;        // margin: how far outside the followed plate
		constexpr int CornerSteps = 8;        // each corner's quarter circle in 8 steps (9 points)
		constexpr double Saturation = 0.85;
		// hud.gd's zone plate: margin 2, corner 8, width 4.
		constexpr double ZoneMargin = 2.0, ZoneCorner = 8.0, ZoneWidth = 4.0;
	}

	// Following a plate (hud.gd: the frame is the plate's SIBLING, so a container cannot lay it out): the plate's rect grown by
	// Margin along the plate's own axes, so a leaning plate's frame leans with it. In the plate's local space.
	PAWCORE_API PcRect RainbowFrameAround(double PlateW, double PlateH, double Margin);

	// _process(): the hue turns only while the frame is active and shown.
	PAWCORE_API double RainbowFramePhase(double Phase, double Dt, double TurnsPerSec, bool bActiveAndShown);

	// _draw(): the rounded rectangle W x H traced from the top-right corner clockwise (on screen), closed (its first point
	// again at the end), each point's colour a hue turn along the loop plus Phase. Godot strokes it with
	// draw_polyline_colors(points, colours, width, antialiased = true).
	PAWCORE_API void RainbowFrameOutline(double W, double H, double Corner, double Phase, std::vector<double>& Xs,
		std::vector<double>& Ys, std::vector<PcColor>& Colors);
}
