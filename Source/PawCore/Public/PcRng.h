#pragma once

// Stateless, seeded randomness for the simulation (docs/03 §2, KB U/03 §4.2). A draw is a pure function of
// (seed, tick, entity, salt): no hidden state to save, a replay or a bug report reproduces exactly, and the order in which
// systems ask does not matter. The Godot game used the engine's global randf(); this is new for the port's fixed step.

#include "PcCore.h"

#include <cstdint>

namespace Pc
{
	// SplitMix64 finaliser over the four inputs: well mixed, fast, and the same on every compiler.
	PAWCORE_API uint64_t RngHash(uint64_t Seed, uint64_t Tick, uint64_t Entity, uint64_t Salt);

	// Uniform in [0, 1) with 53 random bits (every double in the range is equally likely on the 2^-53 grid).
	PAWCORE_API double RngUnit(uint64_t Seed, uint64_t Tick, uint64_t Entity, uint64_t Salt);

	// Uniform integer in [0, Count); Count > 0. Lemire's multiply-shift, so no modulo bias worth measuring.
	PAWCORE_API uint64_t RngBelow(uint64_t Count, uint64_t Seed, uint64_t Tick, uint64_t Entity, uint64_t Salt);

	// Named salts keep independent decisions independent (a crit roll must not correlate with a golden roll).
	namespace Salt
	{
		constexpr uint64_t Crit = 0x43524954ull;      // "CRIT"
		constexpr uint64_t Golden = 0x474F4C44ull;    // "GOLD"
		constexpr uint64_t Spawn = 0x5350574Eull;     // "SPWN"
		constexpr uint64_t SpawnPick = 0x5049434Bull; // "PICK"
		constexpr uint64_t Cards = 0x43415244ull;     // "CARD"
		// The field (PcField): one salt per independent decision of a spawn or a step.
		constexpr uint64_t SpawnEdge = 0x45444745ull; // "EDGE" which edge
		constexpr uint64_t SpawnAlong = 0x414C4E47ull; // "ALNG" where along it
		constexpr uint64_t SpawnTurn = 0x5455524Eull; // "TURN" the spread off dead centre
		constexpr uint64_t Funnel = 0x46554E4Cull;    // "FUNL" the fractional bounce
		constexpr uint64_t Wander = 0x574E4452ull;    // "WNDR" a wandering mouse's heading drift
		constexpr uint64_t KittenWake = 0x4B57414Bull; // "KWAK" a kitten's first roam heading timer on activation
		constexpr uint64_t KittenRoam = 0x4B524F4Dull; // "KROM" a roaming kitten's new heading
		constexpr uint64_t Riddle = 0x5244444Cull;     // "RDDL" a crit arming Riddle
		constexpr uint64_t PiedPiper = 0x50495045ull;  // "PIPE" where on the Pied Piper's ring a mouse is summoned
		constexpr uint64_t BossPlaceX = 0x42504C58ull; // "BPLX" where the boss relocates to, across
		constexpr uint64_t BossPlaceY = 0x42504C59ull; // "BPLY" and down
		constexpr uint64_t BossWobbleX = 0x42574258ull; // "BWBX" the boss's idle drift around its anchor, across
		constexpr uint64_t BossWobbleY = 0x42574259ull; // "BWBY" and down
		// The UI (PcJuice): a jittering control's next lean.
		constexpr uint64_t Jitter = 0x4A495454ull;     // "JITT"
		// The UI (PcRainbow): the rainbow bar's sparks (where, which way, which hue).
		constexpr uint64_t RainbowSpark = 0x53504B52ull; // "SPKR"
		// The UI (PcUnicorn): the Unicorn ceremony's confetti and stars.
		constexpr uint64_t UnicornFiesta = 0x46494553ull; // "FIES"
	}
}
