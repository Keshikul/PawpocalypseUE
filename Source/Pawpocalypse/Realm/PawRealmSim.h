#pragma once

// The realm's clock and its glue to PawCore, without the engine (docs/03 §5): the fixed 60 Hz step over a real frame
// delta, the snapshot the presentation interpolates from, the events of a frame, travel, and what TierController did
// on the game's signals. UPawRealmSubsystem owns one; the Paw.Realm tests drive one directly, so the frame-rate
// independence test runs the very loop the game runs.
//
// Ownership: the data and the progress belong to UPawGameSubsystem and are passed in; the sim owns the field.

#include "CoreMinimal.h"

#include "PcField.h"

#include <vector>

namespace Pc
{
	struct PcGameData;
	struct PcProgress;
}

struct PAWPOCALYPSE_API FPawRealmSim
{
	static constexpr double Step = 1.0 / 60.0;
	// The spiral-of-death guard: a frame never runs more steps than this; the time beyond is dropped (docs/03 §5).
	static constexpr int32 MaxStepsPerFrame = 8;

	Pc::PcField Field;
	Pc::PcFieldInput Input;          // the cat and the settings, written by the game once per frame
	Pc::PcRect Bounds{ 0.0, 0.0, 1920.0, 1080.0 };
	double GameSpeed = 1.0;          // 1 in play; the cheat's 2x / 5x; never World Time Dilation
	bool bPaused = false;            // the pause menu, the skill tree, a card pick: the accumulator stops
	uint64 BaseSeed = 1;             // each biome begun takes BaseSeed + its ordinal: a session replays from its seed

	// Every event of the last Advance, oldest first (the steps' drains in order).
	std::vector<Pc::PcFieldEvent> FrameEvents;
	// Achievements earned and not yet drained, in earn order (AchievementManager.achievement_earned). Godot re-scans on every
	// kill, currency change, purchase, Reawaken, boss and branch level; the realm re-scans after every fixed step, which
	// sees all of those within the step, and the subsystem after every progress change made outside the field.
	std::vector<std::string> EarnedAchievements;
	void EvaluateAchievements(const Pc::PcGameData& D, Pc::PcProgress& P);

	// Measurements (the debug HUD; the travel test).
	int32 StepsLastFrame = 0;
	int64 StepsDropped = 0;          // steps the guard threw away, ever
	double LastTravelMs = 0.0;
	double WorstTravelMs = 0.0;
	double LastStepsMs = 0.0;        // the last frame's steps, all of them
	int64 Travels = 0;

	// The current biome again (a fresh field there): the realm's first frame, and a New Game.
	void Start(const Pc::PcGameData& D, Pc::PcProgress& P);
	// Travel to biome Index (into PcGameData::Tiers): the progress's begin_biome, then a fresh field (docs/03 §4).
	// False for an index outside the list.
	bool Travel(const Pc::PcGameData& D, Pc::PcProgress& P, int32 Index);
	// The playfield moved (a resize, the dock's side): the field and everything that reads its bounds.
	void SetBounds(const Pc::PcRect& NewBounds);
	// TierController._on_reawakened(): kittens re-counted, stats rebuilt, the boss dismissed. No travel: the HUD takes
	// the player to the starting biome after the spend step (Territory wins, designer 2026-11-01).
	void OnReawakened(const Pc::PcGameData& D, Pc::PcProgress& P);

	// One real frame: the steps it owes (at most MaxStepsPerFrame), each followed by the play clock and the drain of its
	// events. A TravelRequested from the field travels at once; the remaining steps run in the new biome. Returns the
	// steps run.
	int32 Advance(const Pc::PcGameData& D, Pc::PcProgress& P, double RealDelta);

	// How far the presentation is between the last step and the next (0..1).
	double Alpha() const { return Accumulator / Step; }
	double PendingTime() const { return Accumulator; }

	// Interpolated positions for the presentation: between the state before the last step and the current one. A slot
	// re-used by a new mouse since then, or a kitten that was asleep, is drawn where it is now.
	void MousePos(int32 Slot, double& OutX, double& OutY) const;
	void KittenPos(int32 K, double& OutX, double& OutY) const;
	void CatPos(double& OutX, double& OutY) const;
	void BossPos(double& OutX, double& OutY) const;

private:
	void Snapshot();
	void BeginHere(const Pc::PcGameData& D, Pc::PcProgress& P, const std::string& TierId);

	double Accumulator = 0.0;

	std::vector<double> PrevX, PrevY;
	std::vector<uint64_t> PrevSerial;
	std::vector<uint8_t> PrevActive;
	std::vector<double> PrevKX, PrevKY;
	std::vector<uint8_t> PrevKActive;
	double PrevCatX = 0.0, PrevCatY = 0.0, PrevBossX = 0.0, PrevBossY = 0.0;
	bool bPrevBossUp = false;
};
