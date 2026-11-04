#pragma once

// All of the game's data in plain structs (docs/03 §2 PcData), loaded from Data/*.csv. The same loader feeds the tests,
// the pawcore_sim CLI and (through the game module's adapter) the game; ids are lower-case strings, never FName.

#include "PcConstants.h"
#include "PcCore.h"
#include "PcDataRows.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace Pc
{
	struct PcGameData
	{
		std::vector<PcTier> Tiers; // in biome order: tier_1 .. tier_N
		std::vector<PcEnemy> Enemies;
		std::vector<PcBoss> Bosses;
		std::vector<PcUpgrade> Upgrades;
		std::vector<PcSkillBranch> Branches;
		std::vector<PcEvolution> Evolutions; // in stage order
		std::vector<PcCard> Cards; // in Godot's deck order: the sorted .tres file names
		std::vector<PcJob> Jobs;
		std::vector<PcAchievement> Achievements; // in Godot's roster order: the sorted .tres file names
		PcCombatConstants Combat;
		PcReawakenConstants Reawaken;
		PcCodeConstants Code; // the gameplay constants that live in Godot code (CodeConstants.csv)

		PAWCORE_API const PcEnemy* FindEnemy(const std::string& Id) const;
		PAWCORE_API const PcBoss* FindBoss(const std::string& Id) const;
		PAWCORE_API const PcUpgrade* FindUpgrade(const std::string& Id) const;
		PAWCORE_API const PcSkillBranch* FindBranch(const std::string& Id) const;
		PAWCORE_API const PcJob* FindJob(const std::string& Id) const;

		// Rebuilt by LoadGameData; call again after editing the vectors (tests do, to break data on purpose).
		PAWCORE_API void RebuildIndex();

	private:
		std::unordered_map<std::string, size_t> EnemyIndex, BossIndex, UpgradeIndex, BranchIndex, JobIndex;
	};

	// The tables the game data is built from, by CSV name without ".csv" (TierData, ..., CodeConstants). The game's
	// DataTables are exactly these (Source/Pawpocalypse/Data/PawDataRows.h; its parity test checks the two lists agree).
	PAWCORE_API const std::vector<std::string>& GameDataTableNames();

	// Builds the data from parsed tables keyed by those names: the one parser behind both routes, the CSV files
	// (LoadGameData) and the game's DataTables rebuilt as CSV text (FPawDataAdapter). False when a table is missing or a
	// cell does not parse; Errors says what and where. Building does not judge the numbers: that is the validator's job.
	PAWCORE_API bool BuildGameData(const std::map<std::string, CsvTable>& Tables, PcGameData& Out, std::vector<std::string>& Errors);

	// Reads every CSV of Dir (the repo's Data/) and builds from them.
	PAWCORE_API bool LoadGameData(const std::string& Dir, PcGameData& Out, std::vector<std::string>& Errors);
}
