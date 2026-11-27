#pragma once

// Boba, the designer's cat, in the menus (P4.3; playtest 2026-11-26, notes 2 and 3; CLAUDE.md: "Boba appears in menus only,
// never over the skill tree"). Her photos turn up now and then over the title, the pause menu, Settings and the victory
// screen: she flies across, spins across, crosses with a rainbow trail, peeks in from an edge, or drops in with a bounce.
// The first try's boba_cameo.gd is the design (WORKLIST-SLICE6 §6c); this is the game's own rule.
//
// The clock: the first menu to open schedules a cameo 6-14 s away, then every 25-60 s; the last menu to close stops the clock
// (a cameo in flight finishes). A suppressor (the skill tree) cuts a cameo in flight and holds the clock. When the clock
// fires while a cameo is in flight, the effects are off, there is no photo or something suppresses, it only reschedules.
// A forced cameo (the title card's click, the victory) obeys the same switches (the first try's force() ignored the
// effects switch: a finding, fixed here).
//
// Pure state stepped by its owner (UPawBobaSubsystem); the poses are functions of the cameo and its time, in screen pixels.

#include "PcCore.h"

#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Pc
{
	enum class PcBobaKind : uint8_t { Flyby, Spin, Nyan, Peek, Drop };
	enum class PcBobaEdge : uint8_t { Left, Right, Top, Bottom };

	struct PcBobaCameo
	{
		bool bLive = false;
		PcBobaKind Kind = PcBobaKind::Flyby;
		int32_t Photo = 0;
		double T = 0.0;          // seconds into it
		double Duration = 0.0;
		double Size = 0.0;       // her drawn height, px (BobaSizeMin..Max of the screen's)
		double Aspect = 1.0;     // the photo's width over its height
		// Flyby, Spin, Nyan: the line across.
		double LineY = 0.0;      // its height, px
		bool bRight = true;      // travelling right (drawn mirrored)
		double Turns = 0.0;      // Spin: whole turns
		// Peek, Drop: where, and the three phases (in, hold, out).
		PcBobaEdge Edge = PcBobaEdge::Left;
		double Along = 0.5;      // the share of the edge (Peek) or of the width (Drop)
		double In = 0.0;
		double Hold = 0.0;
		double Out = 0.0;
	};

	struct PcBobaPose
	{
		double X = 0.0;          // her centre, px
		double Y = 0.0;
		double Rotation = 0.0;   // degrees, clockwise
		bool bFlip = false;      // mirrored left-right
		bool bVisible = false;
	};

	struct PcBoba
	{
		std::set<std::string> Menus;       // the menus open now (their tags)
		std::set<std::string> Suppressors; // what holds her off (the skill tree)
		double Timer = -1.0;               // seconds to the next cameo; < 0: not scheduled
		PcBobaCameo Cameo;
		int32_t LastPhoto = -1;
		uint64_t Seed = 0;
		uint64_t Draws = 0;                // the RNG's counter (RngUnit(Seed, Draws++, ...))
	};

	// The clock's and the cameos' numbers (presentation).
	constexpr double BobaFirstMin = 6.0;
	constexpr double BobaFirstMax = 14.0;
	constexpr double BobaEveryMin = 25.0;
	constexpr double BobaEveryMax = 60.0;
	constexpr double BobaSizeMin = 0.24;      // her height over the screen's
	constexpr double BobaSizeMax = 0.34;
	constexpr double BobaPeekShow = 0.55;     // how much of her a peek shows
	constexpr double BobaDropLand = 0.58;     // where a drop lands, over the screen's height
	constexpr double BobaTrailStep = 7.0;     // the nyan trail: a point every 7 px of travel
	constexpr int32_t BobaTrailPoints = 60;
	// The kinds' weights: flyby, spin, nyan, peek, drop.
	constexpr double BobaWeights[5] = { 4.0, 1.0, 3.0, 4.0, 2.0 };

	PAWCORE_API void BobaEnterMenu(PcBoba& B, const std::string& Tag);
	PAWCORE_API void BobaLeaveMenu(PcBoba& B, const std::string& Tag);
	// The skill tree holds her off (bOn) and cuts a cameo in flight; released, the clock runs again.
	PAWCORE_API void BobaSuppress(PcBoba& B, const std::string& Tag, bool bOn);
	// Steps the clock and the cameo by Dt. True when a cameo started in this step (the owner plays boba_cameo).
	// Aspects: each photo's width over its height (their count is how many photos there are); ScreenH sizes her.
	PAWCORE_API bool BobaStep(PcBoba& B, double Dt, bool bEffects, const std::vector<double>& Aspects, double ScreenH);
	// One now, of this kind (the title card's click; the victory's nyan), under the clock's switches. True = started.
	PAWCORE_API bool BobaForce(PcBoba& B, PcBobaKind Kind, bool bEffects, const std::vector<double>& Aspects, double ScreenH);
	// One now of a kind by the weights (the title card's click), under the same switches.
	PAWCORE_API bool BobaForceAny(PcBoba& B, bool bEffects, const std::vector<double>& Aspects, double ScreenH);
	// Where she is T seconds into a cameo (C.T when T < 0).
	PAWCORE_API PcBobaPose BobaPose(const PcBobaCameo& C, double ScreenW, double ScreenH, double T = -1.0);
	// The nyan trail: the points she passed, newest first, every BobaTrailStep px, at most BobaTrailPoints.
	PAWCORE_API std::vector<std::pair<double, double>> BobaTrail(const PcBobaCameo& C, double ScreenW, double ScreenH);
}
