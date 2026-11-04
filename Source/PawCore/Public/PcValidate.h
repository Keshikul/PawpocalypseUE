#pragma once

// The data validator (docs/06 §6): the intent of the Godot gates check_resources.gd (every rule), the data halves of
// check_damage.gd (evolution ladder) and check_hud.gd (stage gates, boss ladder, bait price), and doc 06's list
// (references resolve, skill-tree parents, achievement rewards). Shared by the tests, the pawcore_sim CLI and the game's
// adapter (development builds fail loudly, Shipping logs; docs/03 §6).
//
// Every failure starts with a stable rule tag, e.g. "[enemy.reward_scaling] enemy alley_dog: ...", so a negative test can
// assert WHICH rule fired. Every rule is negative-tested in Tests/PcValidateTests.cpp (the Godot rule: a gate that has
// never failed has never been tested).
//
// Art-file existence (Godot's ResourceLoader.exists on texture/icon/hat paths) is not PawCore's business: the art gates
// (Paw.Art.*) check the imported assets.

#include "PcCore.h"
#include "PcData.h"

#include <string>
#include <vector>

namespace Pc
{
	PAWCORE_API std::vector<std::string> ValidateGameData(const PcGameData& Data);

	// The rule tags ValidateGameData can emit, in the order they are checked (tests walk this list to prove each one
	// has a negative test).
	PAWCORE_API const std::vector<std::string>& ValidatorRules();

	// Failures the shipped data is KNOWN to produce: Godot data issues waiting on the designer (docs/01 §5). The tests
	// require exactly these on the real data; the game's adapter tolerates exactly these.
	PAWCORE_API const std::vector<std::string>& KnownGodotDataIssues();
}
