#pragma once

// The save (docs/08 §1-2): Godot's PawpocalypseSaveData field for field, the rules SaveManager.save_game() / load_game()
// apply around it, and the file the port writes it as. Proven against the REAL save_manager.gd (fixture `save`).
//
// WHAT A SAVE CARRIES IS WHAT GODOT'S CARRIES, nothing more (Godot is the spec until parity). Since 2026-11-03 (F24) that
// includes the run's cards, Catches level and XP and a hand on screen. Not in it, as in Godot: the live biome and its wave
// counters, the per-run kill counters; a load leaves those as they were in
// memory (the title screen resets a NEW game first).

#include "PcCore.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Pc
{
	struct PcGameData;
	struct PcProgress;

	// PawpocalypseSaveData.CURRENT_VERSION: the schema of the data (Godot's own number, kept so a Godot save imports 1:1).
	constexpr int64_t PcGodotSaveVersion = 7;

	// pawpocalypse_save_data.gd, field for field (the names in comments are the JSON keys, Godot's own).
	struct PcSaveState
	{
		int64_t Version = PcGodotSaveVersion;                  // version
		double Catnip = 0.0;                                   // catnip
		double Stardust = 0.0;                                 // stardust
		int64_t ReawakenCount = 0;                             // reawaken_count
		std::map<std::string, int64_t> UpgradeLevels;          // upgrade_levels {id: level}
		std::map<std::string, int64_t> SkillBranchLevels;      // skill_branch_levels {id: level}
		bool bFinalBossDefeated = false;                       // final_boss_defeated
		double LifetimeCatnipEver = 0.0;                       // lifetime_catnip_ever
		double LifetimeCatnipThisCycle = 0.0;                  // lifetime_catnip_this_cycle
		double StardustGrantedEver = 0.0;                      // stardust_granted_ever
		std::set<std::string> BossesDefeated;                  // bosses_defeated {biome: true}
		int64_t LifetimeKills = 0;                             // lifetime_kills
		std::set<std::string> AchievementsEarned;              // achievements_earned {id: true}
		std::set<std::string> AchievementsClaimed;             // achievements_claimed {id: true}
		double LastPlayedUnix = 0.0;                           // last_played_unix
		double PlaySecondsThisCycle = 0.0;                     // play_seconds_this_cycle
		double TotalPlaySeconds = 0.0;                         // total_play_seconds
		double FinalBossSeconds = 0.0;                         // final_boss_seconds
		bool bAutoTravel = false;                              // auto_travel (per slot, not per machine)
		bool bAutoPickCard = false;                            // auto_pick_card
		std::map<std::string, int64_t> TierKillCounts;         // tier_kill_counts {biome: kills}
		std::map<std::string, int64_t> BaitSpent;              // bait_spent {biome: bait}
		std::set<std::string> BossesClearedCycle;              // bosses_cleared_cycle {biome: true}
		std::set<std::string> JobsHeld;                        // jobs_held {id: true}
		std::string CurrentJob;                                // current_job
		std::string RunJob;                                    // run_job
		bool bRunJobChosen = false;                            // run_job_chosen
		std::string RunJobSecond;                              // run_job_second (Union Rules; F23, 2026-11-03)
		bool bUnicornSplit = false;                            // unicorn_split (every save the port writes: true)
		std::set<std::string> TutorialDone;                    // tutorial_done {step: true}
		// The run's cards and XP (designer 2026-11-03, F24; Godot first). bRunCardsSaved false = a save from before them:
		// the load leaves the run's cards as they are in memory, as it always did.
		bool bRunCardsSaved = false;                           // run_cards_saved
		std::map<std::string, int64_t> CardsOwned;             // cards_owned {card: level}
		std::set<std::string> CardsBanished;                   // cards_banished {card: true}
		int64_t CatchesLevel = 0;                              // catches_level
		double CatchesPoints = 0.0;                            // catches_points
		int64_t Rerolls = 0;                                   // rerolls
		int64_t ExtraChoices = 0;                              // extra_choices
		int64_t Banishes = 0;                                  // banishes
		int64_t FixationHands = 0;                             // fixation_hands
		std::vector<std::string> Hand;                         // hand [card ids]: a hand on screen, offered again on load
		std::vector<std::string> Promised;                     // promised [card ids]: Blueprint's cards for the next hand
		std::string PrototypeCard;                             // prototype_card
		int64_t CardsTakenThisRun = 0;                         // cards_taken_this_run (Grudge Ledger)
	};

	// What the slot carries that PcProgress does not hold: SettingsManager's per-slot pair and GameState.tutorial_done.
	struct PcSlotExtras
	{
		bool bAutoTravel = false;
		bool bAutoPickCard = false;
		std::set<std::string> TutorialDone;
	};

	// save_game(): banks the live biome's kills into the per-biome map first (GameState._bank_tier_kills(), which changes
	// the progress, as in Godot), then copies. NowUnix stamps last_played_unix. Every known upgrade and branch id is
	// written, zeros included, as Godot's duplicated dictionaries are: a load sets only the upgrade ids it finds, so a
	// missing zero would leave the level in memory standing.
	PAWCORE_API PcSaveState CaptureSave(const PcGameData& D, PcProgress& P, const PcSlotExtras& X, double NowUnix);

	enum class PcLoadPath : uint8_t
	{
		Applied,     // the save became the game
		OldVersion,  // version < 7: Godot's "progression reset" path (Catnip and upgrade levels kept); write a save at once
	};
	struct PcLoadResult
	{
		PcLoadPath Path = PcLoadPath::Applied;
		PcSlotExtras Extras;                     // the slot's toggles and tutorial after the load (unchanged on OldVersion)
		bool bTreeMigrated = false;              // the v6 skill-tree shape migration ran (Godot warns)
		std::vector<std::string> NewlyEarned;    // achievements the load's re-scan earned
	};
	// load_game() once the file has been read: GameState / SkillTreeManager / AchievementManager / SettingsManager in
	// Godot's order (the achievement re-scan runs before the jobs and the per-biome maps are restored, as there). Fields a
	// save does not carry keep their value in P. Current = the slot extras in memory before the load.
	PAWCORE_API PcLoadResult ApplySave(const PcGameData& D, PcProgress& P, const PcSaveState& S, const PcSlotExtras& Current);

	// _migrate_branch_levels(): the v6 tree shape (splits, merges, Reflex retired to an unlock), clamped to what each branch
	// holds; unchanged when no old id is present. bMigrated tells whether it ran.
	PAWCORE_API std::map<std::string, int64_t> MigrateBranchLevels(const PcGameData& D, const std::map<std::string, int64_t>& Saved,
		bool& bMigrated);
	// _split_unicorn(): a save from before the Unicorn split that owns the Unicorn is handed Rainbow Poop once.
	PAWCORE_API std::map<std::string, int64_t> SplitUnicorn(const std::map<std::string, int64_t>& Levels, bool bAlreadySplit);

	// autosave() and the title's Quit skip a game with nothing to keep (an idle fresh install never writes an empty slot).
	PAWCORE_API bool HasProgressToSave(const PcProgress& P);

	// slot_summary(): what the slot picker shows, without applying the save.
	struct PcSlotSummary
	{
		int64_t Reawakens = 0;
		int64_t Kills = 0;
		double Catnip = 0.0;              // lifetime_catnip_ever
		double PlaySeconds = 0.0;         // total_play_seconds
		double FinalBossSeconds = 0.0;
		int64_t Bosses = 0;               // bosses_defeated.size()
		double LastPlayedUnix = 0.0;
	};
	PAWCORE_API PcSlotSummary SlotSummary(const PcSaveState& S);

	// --- the file (docs/08 §2) -------------------------------------------------------------------------------------------
	// One header line, then the body as JSON with Godot's key names:
	//   PAWSAVE <format> <crc32 of the body, 8 hex digits> <body length in bytes>\n<body>
	// The format is the CONTAINER's version (a change to the file layout or a migration bumps it); the body's "version" is
	// Godot's schema. Unknown keys are ignored and missing keys take their default, as Godot does with an @export.
	constexpr int64_t PcSaveFormat = 1;

	PAWCORE_API std::string WriteSaveFile(const PcSaveState& S, const std::string& GameBuild);

	enum class PcSaveReadError : uint8_t
	{
		None,
		Empty,         // no bytes
		BadHeader,     // not a PAWSAVE file
		NewerFormat,   // written by a newer build: never loaded (and never overwritten by the backup logic's caller)
		BadLength,     // cut short or padded
		BadCrc,        // the body changed after it was written
		BadJson,       // the body does not parse, or a field has the wrong type
	};
	PAWCORE_API PcSaveReadError ReadSaveFile(const std::string& Bytes, PcSaveState& Out, std::string* Detail = nullptr);
	PAWCORE_API const char* SaveReadErrorName(PcSaveReadError E);

	// CRC-32 (IEEE 802.3, the zlib / PNG one).
	PAWCORE_API uint32_t Crc32(const std::string& Bytes);
}
