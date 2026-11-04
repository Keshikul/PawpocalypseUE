#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcJson.h"
#include "PcProgress.h"
#include "PcSave.h"
#include "PcStats.h"
#include "PcTree.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	const Pc::PcGameData& Game()
	{
		static Pc::PcGameData Data;
		static bool bLoaded = false;
		if (!bLoaded)
		{
			std::vector<std::string> Errors;
			REQUIRE(Pc::LoadGameData(DataDir(), Data, Errors));
			bLoaded = true;
		}
		return Data;
	}

	std::vector<std::string> SplitKeep(const std::string& S, char Sep)
	{
		std::vector<std::string> Out;
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find(Sep, Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	std::map<std::string, int64_t> Pairs(const std::string& S)
	{
		std::map<std::string, int64_t> Out;
		for (const std::string& P : SplitKeep(S, ';'))
		{
			const size_t Eq = P.find('=');
			if (Eq != std::string::npos)
			{
				Out[P.substr(0, Eq)] = std::atoll(P.c_str() + Eq + 1);
			}
		}
		return Out;
	}

	std::set<std::string> Ids(const std::string& S)
	{
		std::set<std::string> Out;
		for (const std::string& Id : SplitKeep(S, ';'))
		{
			if (!Id.empty())
			{
				Out.insert(Id);
			}
		}
		return Out;
	}

	std::vector<std::string> List(const std::string& S)
	{
		std::vector<std::string> Out;
		for (const std::string& Id : SplitKeep(S, ';'))
		{
			if (!Id.empty())
			{
				Out.push_back(Id);
			}
		}
		return Out;
	}

	std::string JoinList(const std::vector<std::string>& V)
	{
		std::string Out;
		for (const std::string& Id : V)
		{
			Out += (Out.empty() ? "" : ";") + Id;
		}
		return Out;
	}

	std::string Join(const std::set<std::string>& S)
	{
		std::string Out;
		for (const std::string& Id : S)
		{
			Out += (Out.empty() ? "" : ";") + Id;
		}
		return Out;
	}

	std::string PairsNz(const std::map<std::string, int64_t>& M)
	{
		std::string Out;
		for (const auto& [K, V] : M)
		{
			if (V != 0)
			{
				Out += (Out.empty() ? "" : ";") + K + "=" + std::to_string(V);
			}
		}
		return Out;
	}

	std::string PairsAll(const std::map<std::string, int64_t>& M)
	{
		std::string Out;
		for (const auto& [K, V] : M)
		{
			Out += (Out.empty() ? "" : ";") + K + "=" + std::to_string(V);
		}
		return Out;
	}

	double Hex(const std::string& S)
	{
		return std::strtod(S.c_str(), nullptr);
	}

	int64_t Int(const std::string& S)
	{
		return std::atoll(S.c_str());
	}

	const char* Flag(bool b)
	{
		return b ? "1" : "0";
	}

	// Bit for bit: a save must give back exactly the double it was handed (the "==" of a NaN would fail; none occur here).
	bool Same(double A, double B)
	{
		return std::memcmp(&A, &B, sizeof(double)) == 0;
	}

	using Row = std::map<std::string, std::string>;

	struct LiveState
	{
		Pc::PcProgress P;
		Pc::PcSlotExtras X;
	};

	LiveState ReadState(const Row& V, const std::string& Pre)
	{
		LiveState L;
		Pc::PcProgress& P = L.P;
		P.UpgradeLevels = Pairs(V.at(Pre + "upgrades"));
		P.BranchLevels = Pairs(V.at(Pre + "branches"));
		P.CardLevels = Pairs(V.at(Pre + "cards"));
		P.BanishedCards = Ids(V.at(Pre + "banished"));
		P.CatchesLevel = Int(V.at(Pre + "catches_level"));
		P.CatchesPoints = Hex(V.at(Pre + "catches_points"));
		P.ExtraChoices = Int(V.at(Pre + "extra_choices"));
		P.JobsHeld = Ids(V.at(Pre + "jobs_held"));
		P.CurrentJob = V.at(Pre + "current_job");
		P.RunJob = V.at(Pre + "run_job");
		P.RunJobSecond = V.at(Pre + "run_job_second");
		P.bRunJobChosen = V.at(Pre + "run_job_chosen") == "1";
		P.Catnip = Hex(V.at(Pre + "catnip"));
		P.Stardust = Hex(V.at(Pre + "stardust"));
		P.ReawakenCount = Int(V.at(Pre + "reawaken_count"));
		P.DeepestBiomeIndex = Int(V.at(Pre + "deepest"));
		P.BossesDefeated = Ids(V.at(Pre + "bosses"));
		P.bFinalBossDefeated = V.at(Pre + "final") == "1";
		P.StardustGrantedEver = Hex(V.at(Pre + "granted"));
		P.CurrentBiomeId = V.at(Pre + "biome");
		P.TierKillCount = Int(V.at(Pre + "tier_kills"));
		P.TierKillsByBiome = Pairs(V.at(Pre + "kills_by_biome"));
		P.BaitSpentByBiome = Pairs(V.at(Pre + "bait_spent"));
		P.BossesClearedThisCycle = Ids(V.at(Pre + "cleared"));
		P.CurrentWaveIndex = Int(V.at(Pre + "wave_index"));
		P.KillsThisWave = Int(V.at(Pre + "kills_this_wave"));
		P.LifetimeCatnipThisCycle = Hex(V.at(Pre + "catnip_cycle"));
		P.LifetimeCatnipEver = Hex(V.at(Pre + "catnip_ever"));
		P.LifetimeKills = Int(V.at(Pre + "lifetime_kills"));
		P.KillsSinceTithe = Int(V.at(Pre + "kills_since_tithe"));
		P.KillsThisCycle = Int(V.at(Pre + "kills_this_cycle"));
		P.WavesHere = Int(V.at(Pre + "waves_here"));
		P.CardsTakenThisRun = Int(V.at(Pre + "cards_taken"));
		P.InfestationBonus = Hex(V.at(Pre + "infestation"));
		P.PendingSecondHelping = Hex(V.at(Pre + "pending_second"));
		P.PlaySecondsThisCycle = Hex(V.at(Pre + "play_cycle"));
		P.TotalPlaySeconds = Hex(V.at(Pre + "play_total"));
		P.FinalBossSeconds = Hex(V.at(Pre + "final_seconds"));
		P.RealtimeInterestElapsed = Hex(V.at(Pre + "interest_elapsed"));
		P.AchievementsEarned = Ids(V.at(Pre + "earned"));
		P.AchievementsClaimed = Ids(V.at(Pre + "claimed"));
		L.X.bAutoTravel = V.at(Pre + "auto_travel") == "1";
		L.X.bAutoPickCard = V.at(Pre + "auto_pick") == "1";
		L.X.TutorialDone = Ids(V.at(Pre + "tutorial"));
		P.Rerolls = Int(V.at(Pre + "rerolls"));
		P.Banishes = Int(V.at(Pre + "banishes"));
		P.FixationHands = Int(V.at(Pre + "fixation"));
		P.Hand = List(V.at(Pre + "hand"));
		P.PromisedCards = List(V.at(Pre + "promised"));
		P.PrototypeCard = V.at(Pre + "prototype");
		return L;
	}

	void CheckState(const Pc::PcProgress& P, const Pc::PcSlotExtras& X, const Row& V, const std::string& Pre)
	{
		CHECK(PairsNz(P.UpgradeLevels) == V.at(Pre + "upgrades"));
		CHECK(PairsNz(P.BranchLevels) == V.at(Pre + "branches"));
		CHECK(PairsAll(P.CardLevels) == V.at(Pre + "cards"));
		CHECK(Join(P.BanishedCards) == V.at(Pre + "banished"));
		CHECK(P.CatchesLevel == Int(V.at(Pre + "catches_level")));
		CHECK(Same(P.CatchesPoints, Hex(V.at(Pre + "catches_points"))));
		CHECK(P.ExtraChoices == Int(V.at(Pre + "extra_choices")));
		CHECK(Join(P.JobsHeld) == V.at(Pre + "jobs_held"));
		CHECK(P.CurrentJob == V.at(Pre + "current_job"));
		CHECK(P.RunJob == V.at(Pre + "run_job"));
		CHECK(P.RunJobSecond == V.at(Pre + "run_job_second"));
		CHECK(Flag(P.bRunJobChosen) == V.at(Pre + "run_job_chosen"));
		CHECK(Same(P.Catnip, Hex(V.at(Pre + "catnip"))));
		CHECK(Same(P.Stardust, Hex(V.at(Pre + "stardust"))));
		CHECK(P.ReawakenCount == Int(V.at(Pre + "reawaken_count")));
		CHECK(P.DeepestBiomeIndex == Int(V.at(Pre + "deepest")));
		CHECK(Join(P.BossesDefeated) == V.at(Pre + "bosses"));
		CHECK(Flag(P.bFinalBossDefeated) == V.at(Pre + "final"));
		CHECK(Same(P.StardustGrantedEver, Hex(V.at(Pre + "granted"))));
		CHECK(P.CurrentBiomeId == V.at(Pre + "biome"));
		CHECK(P.TierKillCount == Int(V.at(Pre + "tier_kills")));
		CHECK(PairsAll(P.TierKillsByBiome) == V.at(Pre + "kills_by_biome"));
		CHECK(PairsAll(P.BaitSpentByBiome) == V.at(Pre + "bait_spent"));
		CHECK(Join(P.BossesClearedThisCycle) == V.at(Pre + "cleared"));
		CHECK(P.CurrentWaveIndex == Int(V.at(Pre + "wave_index")));
		CHECK(P.KillsThisWave == Int(V.at(Pre + "kills_this_wave")));
		CHECK(Same(P.LifetimeCatnipThisCycle, Hex(V.at(Pre + "catnip_cycle"))));
		CHECK(Same(P.LifetimeCatnipEver, Hex(V.at(Pre + "catnip_ever"))));
		CHECK(P.LifetimeKills == Int(V.at(Pre + "lifetime_kills")));
		CHECK(P.KillsSinceTithe == Int(V.at(Pre + "kills_since_tithe")));
		CHECK(P.KillsThisCycle == Int(V.at(Pre + "kills_this_cycle")));
		CHECK(P.WavesHere == Int(V.at(Pre + "waves_here")));
		CHECK(P.CardsTakenThisRun == Int(V.at(Pre + "cards_taken")));
		CHECK(Same(P.InfestationBonus, Hex(V.at(Pre + "infestation"))));
		CHECK(Same(P.PendingSecondHelping, Hex(V.at(Pre + "pending_second"))));
		CHECK(Same(P.PlaySecondsThisCycle, Hex(V.at(Pre + "play_cycle"))));
		CHECK(Same(P.TotalPlaySeconds, Hex(V.at(Pre + "play_total"))));
		CHECK(Same(P.FinalBossSeconds, Hex(V.at(Pre + "final_seconds"))));
		CHECK(Same(P.RealtimeInterestElapsed, Hex(V.at(Pre + "interest_elapsed"))));
		CHECK(Join(P.AchievementsEarned) == V.at(Pre + "earned"));
		CHECK(Join(P.AchievementsClaimed) == V.at(Pre + "claimed"));
		CHECK(Flag(X.bAutoTravel) == V.at(Pre + "auto_travel"));
		CHECK(Flag(X.bAutoPickCard) == V.at(Pre + "auto_pick"));
		CHECK(Join(X.TutorialDone) == V.at(Pre + "tutorial"));
		CHECK(P.Rerolls == Int(V.at(Pre + "rerolls")));
		CHECK(P.Banishes == Int(V.at(Pre + "banishes")));
		CHECK(P.FixationHands == Int(V.at(Pre + "fixation")));
		CHECK(JoinList(P.Hand) == V.at(Pre + "hand"));
		CHECK(JoinList(P.PromisedCards) == V.at(Pre + "promised"));
		CHECK(P.PrototypeCard == V.at(Pre + "prototype"));
	}

	Pc::PcSaveState ReadSave(const Row& V, const std::string& Pre)
	{
		Pc::PcSaveState S;
		S.Version = Int(V.at(Pre + "version"));
		S.Catnip = Hex(V.at(Pre + "catnip"));
		S.Stardust = Hex(V.at(Pre + "stardust"));
		S.ReawakenCount = Int(V.at(Pre + "reawaken_count"));
		S.UpgradeLevels = Pairs(V.at(Pre + "upgrades"));
		S.SkillBranchLevels = Pairs(V.at(Pre + "branches"));
		S.bFinalBossDefeated = V.at(Pre + "final") == "1";
		S.LifetimeCatnipEver = Hex(V.at(Pre + "catnip_ever"));
		S.LifetimeCatnipThisCycle = Hex(V.at(Pre + "catnip_cycle"));
		S.StardustGrantedEver = Hex(V.at(Pre + "granted"));
		S.BossesDefeated = Ids(V.at(Pre + "bosses"));
		S.LifetimeKills = Int(V.at(Pre + "lifetime_kills"));
		S.AchievementsEarned = Ids(V.at(Pre + "earned"));
		S.AchievementsClaimed = Ids(V.at(Pre + "claimed"));
		S.LastPlayedUnix = V.at(Pre + "stamped") == "1" ? 1.0 : 0.0;
		S.PlaySecondsThisCycle = Hex(V.at(Pre + "play_cycle"));
		S.TotalPlaySeconds = Hex(V.at(Pre + "play_total"));
		S.FinalBossSeconds = Hex(V.at(Pre + "final_seconds"));
		S.bAutoTravel = V.at(Pre + "auto_travel") == "1";
		S.bAutoPickCard = V.at(Pre + "auto_pick") == "1";
		S.TierKillCounts = Pairs(V.at(Pre + "kills_by_biome"));
		S.BaitSpent = Pairs(V.at(Pre + "bait_spent"));
		S.BossesClearedCycle = Ids(V.at(Pre + "cleared"));
		S.JobsHeld = Ids(V.at(Pre + "jobs_held"));
		S.CurrentJob = V.at(Pre + "current_job");
		S.RunJob = V.at(Pre + "run_job");
		S.bRunJobChosen = V.at(Pre + "run_job_chosen") == "1";
		S.RunJobSecond = V.at(Pre + "run_job_second");
		S.bUnicornSplit = V.at(Pre + "unicorn_split") == "1";
		S.TutorialDone = Ids(V.at(Pre + "tutorial"));
		S.bRunCardsSaved = V.at(Pre + "run_cards_saved") == "1";
		S.CardsOwned = Pairs(V.at(Pre + "cards"));
		S.CardsBanished = Ids(V.at(Pre + "banished"));
		S.CatchesLevel = Int(V.at(Pre + "catches_level"));
		S.CatchesPoints = Hex(V.at(Pre + "catches_points"));
		S.Rerolls = Int(V.at(Pre + "rerolls"));
		S.ExtraChoices = Int(V.at(Pre + "extra_choices"));
		S.Banishes = Int(V.at(Pre + "banishes"));
		S.FixationHands = Int(V.at(Pre + "fixation"));
		S.Hand = List(V.at(Pre + "hand"));
		S.Promised = List(V.at(Pre + "promised"));
		S.PrototypeCard = V.at(Pre + "prototype");
		S.CardsTakenThisRun = Int(V.at(Pre + "cards_taken"));
		return S;
	}

	// GODOT'S WRITER IS LOSSY: a text .tres keeps 16 significant digits, and a double that is exactly a float32 is written
	// with float32's digits (66126.65625 -> "66126.66"; up to ~6e-8 relative). The port's file keeps every double exactly
	// (docs/08 §2), so what Godot wrote is compared within that loss; everything else is exact.
	bool WrittenByGodot(double Port, double Godot)
	{
		if (Same(Port, Godot))
		{
			return true;
		}
		return std::fabs(Port - Godot) <= 1e-7 * std::fmax(std::fabs(Port), std::fabs(Godot));
	}

	void CheckSave(const Pc::PcSaveState& S, const Row& V, const std::string& Pre)
	{
		auto D = [&V, &Pre](const char* Col) { return Hex(V.at(Pre + Col)); };
		CHECK(S.Version == Int(V.at(Pre + "version")));
		CHECK(WrittenByGodot(S.Catnip, D("catnip")));
		CHECK(WrittenByGodot(S.Stardust, D("stardust")));
		CHECK(S.ReawakenCount == Int(V.at(Pre + "reawaken_count")));
		CHECK(PairsAll(S.UpgradeLevels) == V.at(Pre + "upgrades")); // whole: which ids a save names matters on load
		CHECK(PairsAll(S.SkillBranchLevels) == V.at(Pre + "branches"));
		CHECK(Flag(S.bFinalBossDefeated) == V.at(Pre + "final"));
		CHECK(WrittenByGodot(S.LifetimeCatnipEver, D("catnip_ever")));
		CHECK(WrittenByGodot(S.LifetimeCatnipThisCycle, D("catnip_cycle")));
		CHECK(WrittenByGodot(S.StardustGrantedEver, D("granted")));
		CHECK(Join(S.BossesDefeated) == V.at(Pre + "bosses"));
		CHECK(S.LifetimeKills == Int(V.at(Pre + "lifetime_kills")));
		CHECK(Join(S.AchievementsEarned) == V.at(Pre + "earned"));
		CHECK(Join(S.AchievementsClaimed) == V.at(Pre + "claimed"));
		CHECK(Flag(S.LastPlayedUnix > 0.0) == V.at(Pre + "stamped"));
		CHECK(WrittenByGodot(S.PlaySecondsThisCycle, D("play_cycle")));
		CHECK(WrittenByGodot(S.TotalPlaySeconds, D("play_total")));
		CHECK(WrittenByGodot(S.FinalBossSeconds, D("final_seconds")));
		CHECK(Flag(S.bAutoTravel) == V.at(Pre + "auto_travel"));
		CHECK(Flag(S.bAutoPickCard) == V.at(Pre + "auto_pick"));
		CHECK(PairsAll(S.TierKillCounts) == V.at(Pre + "kills_by_biome"));
		CHECK(PairsAll(S.BaitSpent) == V.at(Pre + "bait_spent"));
		CHECK(Join(S.BossesClearedCycle) == V.at(Pre + "cleared"));
		CHECK(Join(S.JobsHeld) == V.at(Pre + "jobs_held"));
		CHECK(S.CurrentJob == V.at(Pre + "current_job"));
		CHECK(S.RunJob == V.at(Pre + "run_job"));
		CHECK(Flag(S.bRunJobChosen) == V.at(Pre + "run_job_chosen"));
		CHECK(S.RunJobSecond == V.at(Pre + "run_job_second"));
		CHECK(Flag(S.bUnicornSplit) == V.at(Pre + "unicorn_split"));
		CHECK(Join(S.TutorialDone) == V.at(Pre + "tutorial"));
		CHECK(Flag(S.bRunCardsSaved) == V.at(Pre + "run_cards_saved"));
		CHECK(PairsAll(S.CardsOwned) == V.at(Pre + "cards"));
		CHECK(Join(S.CardsBanished) == V.at(Pre + "banished"));
		CHECK(S.CatchesLevel == Int(V.at(Pre + "catches_level")));
		CHECK(WrittenByGodot(S.CatchesPoints, D("catches_points")));
		CHECK(S.Rerolls == Int(V.at(Pre + "rerolls")));
		CHECK(S.ExtraChoices == Int(V.at(Pre + "extra_choices")));
		CHECK(S.Banishes == Int(V.at(Pre + "banishes")));
		CHECK(S.FixationHands == Int(V.at(Pre + "fixation")));
		CHECK(JoinList(S.Hand) == V.at(Pre + "hand"));
		CHECK(JoinList(S.Promised) == V.at(Pre + "promised"));
		CHECK(S.PrototypeCard == V.at(Pre + "prototype"));
		CHECK(S.CardsTakenThisRun == Int(V.at(Pre + "cards_taken")));
	}

	struct SaveResult
	{
		int Rows = 0;
		int OldVersion = 0;
		int TreeMigrated = 0;
		int UnicornSplit = 0;
		int JobRefused = 0;      // a save's current_job the load refused (not held)
		int RunJobSeeded = 0;    // run_job seeded from current_job
		int EarnedOnLoad = 0;    // the re-scan earned something
		int Banked = 0;          // the save moved live kills into the map
		int FileRoundTrips = 0;
		int RunCardsRestored = 0; // F24: a save holding the run's cards
		int RunCardsOld = 0;      // a save from before them: memory's kept
		int HandsReoffered = 0;   // a restored hand, up again
		int CardDropped = 0;      // a card id the deck no longer has, dropped
	};

	SaveResult RunSaveFixture(const std::string& Path)
	{
		SaveResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		const Pc::PcGameData& D = Game();
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = SplitKeep(Line, '\t');
			if (Header.empty())
			{
				Header = Cells;
				continue;
			}
			REQUIRE(Cells.size() == Header.size());
			Row V;
			for (size_t I = 0; I < Cells.size(); ++I)
			{
				V[Header[I]] = Cells[I];
			}
			++R.Rows;
			CAPTURE(R.Rows);

			// save_game() from state A.
			LiveState A = ReadState(V, "s0_");
			const std::map<std::string, int64_t> MapBefore = A.P.TierKillsByBiome;
			Pc::PcSaveState S = Pc::CaptureSave(D, A.P, A.X, 1.0);
			CheckSave(S, V, "sv_");
			CheckState(A.P, A.X, V, "s1_");
			R.Banked += A.P.TierKillsByBiome != MapBefore ? 1 : 0;
			// The file gives back exactly what it was handed.
			Pc::PcSaveState Back;
			std::string Why;
			const std::string File = Pc::WriteSaveFile(S, "test");
			REQUIRE_MESSAGE(Pc::ReadSaveFile(File, Back, &Why) == Pc::PcSaveReadError::None, Why);
			CHECK(Pc::WriteSaveFile(Back, "test") == File); // exactly what it was handed: every field, every bit
			++R.FileRoundTrips;

			// load_game() of save L over state B.
			LiveState B = ReadState(V, "b_");
			const Pc::PcSaveState L = ReadSave(V, "ld_");
			const Pc::PcLoadResult Res = Pc::ApplySave(D, B.P, L, B.X);
			if (Res.Path == Pc::PcLoadPath::OldVersion)
			{
				// Godot writes a save at once on this path; the write banks the live biome's kills.
				Pc::CaptureSave(D, B.P, Res.Extras, 1.0);
				++R.OldVersion;
			}
			else
			{
				R.TreeMigrated += Res.bTreeMigrated ? 1 : 0;
				R.UnicornSplit += (!L.bUnicornSplit && L.SkillBranchLevels.count("unlock_unicorn") &&
					L.SkillBranchLevels.at("unlock_unicorn") > 0) ? 1 : 0;
				R.JobRefused += (!L.CurrentJob.empty() && !L.JobsHeld.count(L.CurrentJob)) ? 1 : 0;
				R.RunJobSeeded += (L.RunJob.empty() && !L.CurrentJob.empty()) ? 1 : 0;
			}
			R.EarnedOnLoad += Res.NewlyEarned.empty() ? 0 : 1;
			CheckState(B.P, Res.Extras, V, "a_");
			if (Res.Path == Pc::PcLoadPath::Applied && L.bRunCardsSaved)
			{
				++R.RunCardsRestored;
				// Godot offers a restored hand again once (reoffer_after_load); the port's HUD opens the pick on any pending
				// hand, so the pending hand IS the offer.
				CHECK(V.at("reoffer1") == (B.P.Hand.empty() ? std::string("-") : JoinList(B.P.Hand)));
				CHECK(V.at("reoffer2") == "-");
				R.HandsReoffered += B.P.Hand.empty() ? 0 : 1;
				R.CardDropped += (L.CardsOwned.count("retired_card") || V.at("ld_hand").find("retired_card") != std::string::npos) ? 1 : 0;
			}
			else if (Res.Path == Pc::PcLoadPath::Applied)
			{
				++R.RunCardsOld;
			}
		}
		return R;
	}
}

TEST_CASE("Save: save_game() and load_game() match the real Godot SaveManager, field for field")
{
	const SaveResult R = RunSaveFixture(DataDir() + "/../Source/PawCore/Tests/Data/save_godot.tsv");
	MESSAGE("rows " << R.Rows << ", old versions " << R.OldVersion << ", tree migrated " << R.TreeMigrated
		<< ", unicorn split " << R.UnicornSplit << ", job refused " << R.JobRefused << ", run job seeded " << R.RunJobSeeded
		<< ", earned on load " << R.EarnedOnLoad << ", banked " << R.Banked << ", file round trips " << R.FileRoundTrips);
	CHECK(R.Rows == 1000);
	// Coverage: every path the load takes appears (a fixture that never takes one has not tested it).
	CHECK(R.OldVersion > 50);
	CHECK(R.TreeMigrated > 50);
	CHECK(R.UnicornSplit > 20);
	CHECK(R.JobRefused > 20);
	CHECK(R.RunJobSeeded > 20);
	CHECK(R.EarnedOnLoad > 20);
	CHECK(R.Banked > 100);
	MESSAGE("run cards restored " << R.RunCardsRestored << ", pre-F24 saves " << R.RunCardsOld << ", hands re-offered "
		<< R.HandsReoffered << ", retired cards dropped " << R.CardDropped);
	CHECK(R.RunCardsRestored > 300);
	CHECK(R.RunCardsOld > 50);
	CHECK(R.HandsReoffered > 100);
	CHECK(R.CardDropped > 20);
}

TEST_CASE("Save: the long suite runs all 3,000 save rows" * doctest::test_suite("long") * doctest::skip())
{
	const SaveResult R = RunSaveFixture(DataDir() + "/../build/save_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Save: the file round-trips every double bit for bit and every integer exactly")
{
	Pc::PcSaveState S;
	const double Odd[] = {0.0, -0.0, 1.0, 0.1, 1.0 / 3.0, 5e-324, 2.2250738585072014e-308, 1.7976931348623157e308,
		123456789.123456789, -987654321.5, 1e300 * 0.7, std::numeric_limits<double>::infinity(),
		-std::numeric_limits<double>::infinity()};
	int Checked = 0;
	for (const double V : Odd)
	{
		S.Catnip = V;
		S.Stardust = -V;
		S.ReawakenCount = std::numeric_limits<int64_t>::max();
		S.LifetimeKills = std::numeric_limits<int64_t>::min();
		S.UpgradeLevels["paw_damage"] = 9007199254740993; // 2^53 + 1: a double could not hold it
		S.CurrentJob = "quote\" back\\slash \n tab\t \x01 caf\xC3\xA9";
		Pc::PcSaveState Back;
		std::string Why;
		REQUIRE_MESSAGE(Pc::ReadSaveFile(Pc::WriteSaveFile(S, "b"), Back, &Why) == Pc::PcSaveReadError::None, Why);
		CHECK(Same(Back.Catnip, V));
		CHECK(Same(Back.Stardust, -V));
		CHECK(Back.ReawakenCount == S.ReawakenCount);
		CHECK(Back.LifetimeKills == S.LifetimeKills);
		CHECK(Back.UpgradeLevels.at("paw_damage") == 9007199254740993);
		CHECK(Back.CurrentJob == S.CurrentJob);
		++Checked;
	}
	CHECK(Checked == 13);
	// NaN round-trips as NaN.
	S.Catnip = std::nan("");
	Pc::PcSaveState Back;
	REQUIRE(Pc::ReadSaveFile(Pc::WriteSaveFile(S, "b"), Back) == Pc::PcSaveReadError::None);
	CHECK(std::isnan(Back.Catnip));
	// The same state writes the same bytes (a diff of two saves shows only what changed).
	S.Catnip = 1.5;
	CHECK(Pc::WriteSaveFile(S, "b") == Pc::WriteSaveFile(S, "b"));
}

TEST_CASE("Save: a damaged file is refused, never half-read")
{
	Pc::PcSaveState S;
	S.Catnip = 42.0;
	S.JobsHeld = {"bodyguard"};
	const std::string Good = Pc::WriteSaveFile(S, "b");
	Pc::PcSaveState Out;
	REQUIRE(Pc::ReadSaveFile(Good, Out) == Pc::PcSaveReadError::None);
	CHECK(Out.Catnip == 42.0);

	CHECK(Pc::ReadSaveFile("", Out) == Pc::PcSaveReadError::Empty);
	CHECK(Pc::ReadSaveFile("hello\n{}", Out) == Pc::PcSaveReadError::BadHeader);
	CHECK(Pc::ReadSaveFile(Good.substr(0, Good.size() - 5), Out) == Pc::PcSaveReadError::BadLength); // cut short (a crash)
	CHECK(Pc::ReadSaveFile(Good + "x", Out) == Pc::PcSaveReadError::BadLength);
	// One byte of the body changed, same length: the checksum catches it.
	std::string Flipped = Good;
	const size_t At = Flipped.find("42.0");
	REQUIRE(At != std::string::npos);
	Flipped[At] = '7';
	CHECK(Pc::ReadSaveFile(Flipped, Out) == Pc::PcSaveReadError::BadCrc);
	CHECK(Out.Catnip == 0.0); // refused: nothing of it leaks out
	// A newer build's file is never read (its meaning is unknown here).
	std::string Newer = Good;
	Newer.replace(0, std::strlen("PAWSAVE 1"), "PAWSAVE 9");
	CHECK(Pc::ReadSaveFile(Newer, Out) == Pc::PcSaveReadError::NewerFormat);
	// A body that parses but has the wrong type for a field.
	auto Wrap = [](const std::string& Body)
	{
		char Header[96];
		std::snprintf(Header, sizeof(Header), "PAWSAVE 1 %08x %zu\n", static_cast<unsigned>(Pc::Crc32(Body)), Body.size());
		return std::string(Header) + Body;
	};
	CHECK(Pc::ReadSaveFile(Wrap("{\"catnip\": \"lots\"}"), Out) == Pc::PcSaveReadError::BadJson);
	CHECK(Pc::ReadSaveFile(Wrap("{\"reawaken_count\": 1.5}"), Out) == Pc::PcSaveReadError::BadJson);
	CHECK(Pc::ReadSaveFile(Wrap("{\"jobs_held\": [\"bodyguard\"]}"), Out) == Pc::PcSaveReadError::BadJson);
	CHECK(Pc::ReadSaveFile(Wrap("[1, 2]"), Out) == Pc::PcSaveReadError::BadJson);
	CHECK(Pc::ReadSaveFile(Wrap("{\"catnip\": 1"), Out) == Pc::PcSaveReadError::BadJson);
	// Nested past the limit: refused, not a stack overflow.
	std::string Deep(200, '[');
	Deep += std::string(200, ']');
	CHECK(Pc::ReadSaveFile(Wrap("{\"x\": " + Deep + "}"), Out) == Pc::PcSaveReadError::BadJson);
	// Missing keys take their default and unknown keys are ignored (Godot's @export).
	REQUIRE(Pc::ReadSaveFile(Wrap("{\"catnip\": 3.5, \"some_future_field\": {\"a\": [1, 2]}}"), Out) == Pc::PcSaveReadError::None);
	CHECK(Out.Catnip == 3.5);
	CHECK(Out.Version == Pc::PcGodotSaveVersion);
	CHECK(Out.ReawakenCount == 0);
	CHECK_FALSE(Out.bUnicornSplit);
	// An "id: false" entry is not held.
	REQUIRE(Pc::ReadSaveFile(Wrap("{\"jobs_held\": {\"a\": true, \"b\": false}}"), Out) == Pc::PcSaveReadError::None);
	CHECK(Join(Out.JobsHeld) == "a");
}

TEST_CASE("Save: CRC-32 is the IEEE one (zlib, PNG)")
{
	CHECK(Pc::Crc32("") == 0u);
	CHECK(Pc::Crc32("123456789") == 0xCBF43926u); // the standard check value
	CHECK(Pc::Crc32("The quick brown fox jumps over the lazy dog") == 0x414FA339u);
}

TEST_CASE("Save: the JSON reader takes what JSON allows and refuses the rest")
{
	Pc::PcJsonValue V;
	std::string E;
	REQUIRE(Pc::ParseJson(" {\"a\": [1, -2, 3.5e2, true, false, null, \"\\u00e9\\ud83d\\ude00\"]} ", V, E));
	const Pc::PcJsonValue* A = V.Find("a");
	REQUIRE(A != nullptr);
	REQUIRE(A->Array.size() == 7);
	CHECK(A->Array[0].Type == Pc::PcJsonValue::Kind::Int);
	CHECK(A->Array[1].Int == -2);
	CHECK(A->Array[2].Type == Pc::PcJsonValue::Kind::Double);
	CHECK(A->Array[2].Double == 350.0);
	CHECK(A->Array[6].String == "\xC3\xA9\xF0\x9F\x98\x80");
	CHECK(Pc::ParseJson("99999999999999999999", V, E)); // past int64: kept as a double
	CHECK(V.Type == Pc::PcJsonValue::Kind::Double);
	const char* Bad[] = {"", "{", "[1,]", "{\"a\" 1}", "01", "1.", ".5", "-", "\"\\x\"", "\"\x01\"", "tru", "1 2",
		"\"\\ud83d\"", "{\"a\": 1,}", "[1 2]", "{\"a\": 1 \"b\": 2}"};
	int Refused = 0;
	for (const char* B : Bad)
	{
		CAPTURE(B);
		CHECK_FALSE(Pc::ParseJson(B, V, E));
		Refused += 1;
	}
	CHECK(Refused == 16);
}

TEST_CASE("Save: the v6 tree migration and the Unicorn split, case by case")
{
	const Pc::PcGameData& D = Game();
	bool bMigrated = false;
	// Nothing old: unchanged, and not reported.
	std::map<std::string, int64_t> Now = {{"claws", 3}, {"unlock_aura", 1}};
	CHECK(Pc::MigrateBranchLevels(D, Now, bMigrated) == Now);
	CHECK_FALSE(bMigrated);
	// aura_power 4: the unlock, and the renamed ladder gets the remaining 3.
	auto Out = Pc::MigrateBranchLevels(D, {{"aura_power", 4}}, bMigrated);
	CHECK(bMigrated);
	CHECK(Out.at("unlock_aura") == 1);
	CHECK(Out.at("aura_damage") == 3);
	CHECK(Out.count("aura_power") == 0);
	// Merges take the MAX, never the sum; Reflex becomes an unlock.
	Out = Pc::MigrateBranchLevels(D, {{"ferocity", 9}, {"mystique", 2}, {"reflex", 5}}, bMigrated);
	CHECK(Out.at("claws") == std::min<int64_t>(9, Pc::BranchMaxLevel(*D.FindBranch("claws"))));
	CHECK(Out.at("unlock_reflex") == 1);
	CHECK(Out.count("reflex") == 0);
	// An id no branch has goes; a level past the top is clamped.
	Out = Pc::MigrateBranchLevels(D, {{"reflex", 0}, {"not_a_branch", 3}, {"claws", 999}}, bMigrated);
	CHECK(Out.count("not_a_branch") == 0);
	CHECK(Out.at("claws") == Pc::BranchMaxLevel(*D.FindBranch("claws")));
	// The Unicorn split: once, only for an unsplit save owning the Unicorn.
	CHECK(Pc::SplitUnicorn({{"unlock_unicorn", 1}}, false).at("unlock_poop") == 1);
	CHECK(Pc::SplitUnicorn({{"unlock_unicorn", 1}}, true).count("unlock_poop") == 0);
	CHECK(Pc::SplitUnicorn({{"unlock_unicorn", 0}}, false).count("unlock_poop") == 0);
}

TEST_CASE("Save: an idle fresh game has nothing to save; any Catnip or Reawaken does")
{
	Pc::PcProgress P;
	CHECK_FALSE(Pc::HasProgressToSave(P));
	P.Catnip = 1e9; // spendable Catnip alone is not progress (it was never EARNED)
	CHECK_FALSE(Pc::HasProgressToSave(P));
	P.LifetimeCatnipEver = 0.5;
	CHECK(Pc::HasProgressToSave(P));
	P.LifetimeCatnipEver = 0.0;
	P.ReawakenCount = 1;
	CHECK(Pc::HasProgressToSave(P));
}

TEST_CASE("Save: the slot summary reads the save, not the game")
{
	Pc::PcSaveState S;
	S.ReawakenCount = 3;
	S.LifetimeKills = 1234;
	S.LifetimeCatnipEver = 5e6;
	S.TotalPlaySeconds = 3661.0;
	S.FinalBossSeconds = 3000.0;
	S.BossesDefeated = {"tier_3", "tier_6"};
	S.LastPlayedUnix = 1.8e9;
	const Pc::PcSlotSummary M = Pc::SlotSummary(S);
	CHECK(M.Reawakens == 3);
	CHECK(M.Kills == 1234);
	CHECK(M.Catnip == 5e6);
	CHECK(M.PlaySeconds == 3661.0);
	CHECK(M.FinalBossSeconds == 3000.0);
	CHECK(M.Bosses == 2);
	CHECK(M.LastPlayedUnix == 1.8e9);
}

#endif
