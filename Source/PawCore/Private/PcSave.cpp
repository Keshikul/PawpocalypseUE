#include "PcSave.h"

#include "PcAchievements.h"
#include "PcData.h"
#include "PcJobs.h"
#include "PcJson.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "PcTree.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <system_error>
#include <utility>

namespace Pc
{
	// --- save_game() -------------------------------------------------------------------------------------------------

	PcSaveState CaptureSave(const PcGameData& D, PcProgress& P, const PcSlotExtras& X, double NowUnix)
	{
		PcSaveState S;
		S.Version = PcGodotSaveVersion;
		S.Catnip = P.Catnip;
		S.Stardust = P.Stardust;
		S.ReawakenCount = P.ReawakenCount;
		for (const PcUpgrade& U : D.Upgrades)
		{
			S.UpgradeLevels[U.Id] = UpgradeLevel(P, U.Id);
		}
		for (const PcSkillBranch& B : D.Branches)
		{
			S.SkillBranchLevels[B.Id] = BranchLevel(P, B.Id);
		}
		S.bUnicornSplit = true;
		S.bFinalBossDefeated = P.bFinalBossDefeated;
		S.LifetimeCatnipEver = P.LifetimeCatnipEver;
		S.LifetimeCatnipThisCycle = P.LifetimeCatnipThisCycle;
		S.StardustGrantedEver = P.StardustGrantedEver;
		S.BossesDefeated = P.BossesDefeated;
		S.LifetimeKills = P.LifetimeKills;
		S.AchievementsEarned = P.AchievementsEarned;
		S.AchievementsClaimed = P.AchievementsClaimed;
		S.PlaySecondsThisCycle = P.PlaySecondsThisCycle;
		S.TotalPlaySeconds = P.TotalPlaySeconds;
		S.FinalBossSeconds = P.FinalBossSeconds;
		S.bAutoTravel = X.bAutoTravel;
		S.bAutoPickCard = X.bAutoPickCard;
		// The live counter into the map FIRST: the map is what is written, and the biome the player stands in has its kills
		// in TierKillCount, not yet in the map.
		BankTierKills(P);
		S.TierKillCounts = P.TierKillsByBiome;
		S.BaitSpent = P.BaitSpentByBiome;
		S.BossesClearedCycle = P.BossesClearedThisCycle;
		S.JobsHeld = P.JobsHeld;
		S.CurrentJob = P.CurrentJob;
		S.RunJob = P.RunJob;
		S.bRunJobChosen = P.bRunJobChosen;
		S.RunJobSecond = P.RunJobSecond;
		S.TutorialDone = X.TutorialDone;
		// The run's cards and XP (F24), a hand on screen with its contents.
		S.bRunCardsSaved = true;
		S.CardsOwned = P.CardLevels;
		S.CardsBanished = P.BanishedCards;
		S.CatchesLevel = P.CatchesLevel;
		S.CatchesPoints = P.CatchesPoints;
		S.Rerolls = P.Rerolls;
		S.ExtraChoices = P.ExtraChoices;
		S.Banishes = P.Banishes;
		S.FixationHands = P.FixationHands;
		S.Hand = P.Hand;
		S.Promised = P.PromisedCards;
		S.PrototypeCard = P.PrototypeCard;
		S.CardsTakenThisRun = P.CardsTakenThisRun;
		// Where the run stands (F26-F29).
		S.DeepestBiomeIndex = P.DeepestBiomeIndex;
		S.PendingSecondHelping = P.PendingSecondHelping;
		S.KillsThisCycle = P.KillsThisCycle;
		S.KillsSinceTithe = P.KillsSinceTithe;
		S.bReawakenInProgress = P.bReawakenInProgress;
		S.LastPlayedUnix = NowUnix;
		return S;
	}

	// --- load_game() -------------------------------------------------------------------------------------------------

	namespace
	{
		// GameState.apply_save_data(): only the upgrade ids it knows, from the levels it is handed.
		void ApplyGameState(const PcGameData& D, PcProgress& P, double Catnip, double Stardust, int64_t Reawakens,
			const std::map<std::string, int64_t>& Levels, bool bFinal, double Ever, double Granted,
			const std::set<std::string>& Bosses, double Cycle, double PlaySeconds)
		{
			P.BossesDefeated = Bosses;
			P.Catnip = Catnip;
			P.Stardust = Stardust;
			P.ReawakenCount = Reawakens;
			P.bFinalBossDefeated = bFinal;
			P.LifetimeCatnipEver = Ever;
			P.LifetimeCatnipThisCycle = Cycle;
			P.PlaySecondsThisCycle = std::max(PlaySeconds, 0.0);
			P.StardustGrantedEver = Granted;
			for (const auto& [Id, Level] : Levels)
			{
				if (D.FindUpgrade(Id))
				{
					// Never below 0 (F32); no top: the cap is on the effect, not the level.
					P.UpgradeLevels[Id] = std::max<int64_t>(Level, 0);
				}
			}
		}

		// SkillTreeManager.apply_save_data(): every known branch takes the saved level, or 0, clamped to 0..max (F32).
		void ApplyTree(const PcGameData& D, PcProgress& P, const std::map<std::string, int64_t>& Saved)
		{
			P.BranchLevels.clear();
			for (const PcSkillBranch& B : D.Branches)
			{
				const auto It = Saved.find(B.Id);
				const int64_t Level = It != Saved.end() ? std::clamp<int64_t>(It->second, 0, BranchMaxLevel(B)) : 0;
				if (Level != 0)
				{
					P.BranchLevels[B.Id] = Level;
				}
			}
		}

		// save_manager.gd's tables, in their dictionary order (the order the migration applies them in).
		const std::array<std::pair<const char*, const char*>, 4> BranchSplits = {{
			{"aura_power", "unlock_aura"}, {"pounce", "unlock_pounce"}, {"overkill", "unlock_overkill"}, {"unicorn", "unlock_unicorn"}}};
		const char* SplitLadder(const std::string& OldId)
		{
			return OldId == "aura_power" ? "aura_damage" : nullptr;
		}
		struct Merge
		{
			const char* NewId;
			std::array<const char*, 3> OldIds;
			int Count;
		};
		const std::array<Merge, 3> BranchMerges = {{
			{"claws", {{"ferocity", "mystique", nullptr}}, 2},
			{"fortune", {{"fortune_yield", "fortune_golden", nullptr}}, 2},
			{"legacy", {{"legacy_hoard", "legacy_memory", "legacy_territory"}}, 3}}};
		const std::pair<const char*, const char*> BranchRetired = {"reflex", "unlock_reflex"};

		bool IsCard(const PcGameData& D, const std::string& Id)
		{
			for (const PcCard& C : D.Cards)
			{
				if (C.Id == Id)
				{
					return true;
				}
			}
			return false;
		}

		// _cards_from_ids(): known cards only, each once, in order.
		std::vector<std::string> KnownCards(const PcGameData& D, const std::vector<std::string>& Ids)
		{
			std::vector<std::string> Out;
			for (const std::string& Id : Ids)
			{
				if (IsCard(D, Id) && std::find(Out.begin(), Out.end(), Id) == Out.end())
				{
					Out.push_back(Id);
				}
			}
			return Out;
		}

		// CardManager.apply_run_save(): the run's cards replaced WHOLE.
		void ApplyRunCards(const PcGameData& D, PcProgress& P, const PcSaveState& S)
		{
			P.CardLevels.clear();
			for (const auto& [Id, Level] : S.CardsOwned)
			{
				if (IsCard(D, Id))
				{
					P.CardLevels[Id] = Level;
				}
			}
			P.BanishedCards.clear();
			for (const std::string& Id : S.CardsBanished)
			{
				if (IsCard(D, Id))
				{
					P.BanishedCards.insert(Id);
				}
			}
			P.CatchesLevel = std::max<int64_t>(S.CatchesLevel, 0);
			P.CatchesPoints = std::max(S.CatchesPoints, 0.0);
			P.Rerolls = std::max<int64_t>(S.Rerolls, 0);
			P.ExtraChoices = std::max<int64_t>(S.ExtraChoices, 0);
			P.Banishes = std::max<int64_t>(S.Banishes, 0);
			P.FixationHands = std::max<int64_t>(S.FixationHands, 0);
			P.Hand = KnownCards(D, S.Hand);
			P.PromisedCards = KnownCards(D, S.Promised);
			// The mark only ever sits on a card in the hand.
			const bool bMarked = !S.PrototypeCard.empty() && IsCard(D, S.PrototypeCard)
				&& std::find(P.Hand.begin(), P.Hand.end(), S.PrototypeCard) != P.Hand.end();
			P.PrototypeCard = bMarked ? S.PrototypeCard : std::string();
		}

		int64_t Get(const std::map<std::string, int64_t>& M, const std::string& Id)
		{
			const auto It = M.find(Id);
			return It == M.end() ? 0 : It->second;
		}
	}

	std::map<std::string, int64_t> MigrateBranchLevels(const PcGameData& D, const std::map<std::string, int64_t>& Saved,
		bool& bMigrated)
	{
		bMigrated = false;
		bool bNeeds = false;
		for (const auto& [OldId, Unlock] : BranchSplits)
		{
			(void)Unlock;
			if (Saved.count(OldId) && !D.FindBranch(OldId))
			{
				bNeeds = true;
			}
		}
		if (Saved.count(BranchRetired.first))
		{
			bNeeds = true;
		}
		for (const Merge& M : BranchMerges)
		{
			for (int I = 0; I < M.Count; ++I)
			{
				if (Saved.count(M.OldIds[static_cast<size_t>(I)]))
				{
					bNeeds = true;
				}
			}
		}
		if (!bNeeds)
		{
			return Saved;
		}

		std::map<std::string, int64_t> Out = Saved;
		// Splits: a ladder that kept its id keeps its key and loses one level to the unlock it spawns.
		for (const auto& [OldIdC, UnlockC] : BranchSplits)
		{
			const std::string OldId = OldIdC;
			const int64_t Level = Get(Out, OldId);
			if (Level <= 0)
			{
				continue;
			}
			Out[UnlockC] = 1;
			const char* Renamed = SplitLadder(OldId);
			const std::string LadderId = Renamed ? std::string(Renamed) : OldId;
			if (!D.FindBranch(LadderId))
			{
				Out.erase(OldId);
				continue;
			}
			if (LadderId == OldId)
			{
				Out[OldId] = Level - 1;
			}
			else
			{
				Out[LadderId] = std::max(Get(Out, LadderId), Level - 1);
				Out.erase(OldId);
			}
		}
		if (Get(Out, BranchRetired.first) > 0)
		{
			Out[BranchRetired.second] = 1;
		}
		Out.erase(BranchRetired.first);
		for (const Merge& M : BranchMerges)
		{
			int64_t Best = 0;
			bool bFound = false;
			for (int I = 0; I < M.Count; ++I)
			{
				const std::string OldId = M.OldIds[static_cast<size_t>(I)];
				const auto It = Out.find(OldId);
				if (It != Out.end())
				{
					bFound = true;
					Best = std::max(Best, It->second);
					Out.erase(It);
				}
			}
			if (bFound)
			{
				Out[M.NewId] = std::max(Get(Out, M.NewId), Best);
			}
		}
		// Clamp to what each branch can hold; an id with no branch goes.
		for (auto It = Out.begin(); It != Out.end();)
		{
			const PcSkillBranch* B = D.FindBranch(It->first);
			if (!B)
			{
				It = Out.erase(It);
				continue;
			}
			It->second = std::clamp<int64_t>(It->second, 0, BranchMaxLevel(*B));
			++It;
		}
		bMigrated = true;
		return Out;
	}

	std::map<std::string, int64_t> SplitUnicorn(const std::map<std::string, int64_t>& Levels, bool bAlreadySplit)
	{
		if (bAlreadySplit || Get(Levels, "unlock_unicorn") <= 0)
		{
			return Levels;
		}
		std::map<std::string, int64_t> Out = Levels;
		Out["unlock_poop"] = std::max<int64_t>(Get(Out, "unlock_poop"), 1);
		return Out;
	}

	PcLoadResult ApplySave(const PcGameData& D, PcProgress& P, const PcSaveState& Saved, const PcSlotExtras& Current)
	{
		// SaveManager._finite_numbers() (F33): every number of the save that is not finite reads as 0, before anything.
		PcSaveState S = Saved;
		for (double* V : {&S.Catnip, &S.Stardust, &S.LifetimeCatnipEver, &S.LifetimeCatnipThisCycle, &S.StardustGrantedEver,
				&S.LastPlayedUnix, &S.PlaySecondsThisCycle, &S.TotalPlaySeconds, &S.FinalBossSeconds, &S.CatchesPoints,
				&S.PendingSecondHelping})
		{
			*V = std::isfinite(*V) ? *V : 0.0;
		}
		PcLoadResult R;
		R.Extras = Current;
		if (S.Version < PcGodotSaveVersion)
		{
			// Keep Catnip and the upgrade levels, reset the rest of the progression (v3 review decision #6); the caller
			// writes a save at once. Everything else the save holds is not read, and memory keeps it.
			R.Path = PcLoadPath::OldVersion;
			ApplyGameState(D, P, S.Catnip, 0.0, 0, S.UpgradeLevels, false, 0.0, 0.0, {}, 0.0, 0.0);
			ApplyTree(D, P, {});
			R.NewlyEarned = ApplyAchievementSave(D, P, {}, {});
			return R;
		}
		ApplyGameState(D, P, S.Catnip, S.Stardust, S.ReawakenCount, S.UpgradeLevels, S.bFinalBossDefeated,
			S.LifetimeCatnipEver, S.StardustGrantedEver, S.BossesDefeated, S.LifetimeCatnipThisCycle, S.PlaySecondsThisCycle);
		P.LifetimeKills = S.LifetimeKills;
		P.TotalPlaySeconds = S.TotalPlaySeconds;
		P.FinalBossSeconds = S.FinalBossSeconds;
		ApplyTree(D, P, SplitUnicorn(MigrateBranchLevels(D, S.SkillBranchLevels, R.bTreeMigrated), S.bUnicornSplit));
		// The re-scan runs HERE in Godot: before the jobs and the per-biome maps below are restored.
		R.NewlyEarned = ApplyAchievementSave(D, P, S.AchievementsEarned, S.AchievementsClaimed);
		R.Extras.bAutoTravel = S.bAutoTravel;
		R.Extras.bAutoPickCard = S.bAutoPickCard;
		P.TierKillsByBiome = S.TierKillCounts;
		P.BaitSpentByBiome = S.BaitSpent;
		P.BossesClearedThisCycle = S.BossesClearedCycle;
		// Forget where the player stood: the arriving realm's begin_biome() restores the live counter from the map.
		P.CurrentBiomeId.clear();
		P.JobsHeld = S.JobsHeld;
		// Through take_job(): a job not held is refused, and the preference stays "".
		P.CurrentJob.clear();
		TakeJob(P, S.CurrentJob);
		// A save from before the run lock: the job worn mid-run stays on rather than reopening the picker.
		P.RunJob = !S.RunJob.empty() ? S.RunJob : S.CurrentJob;
		P.bRunJobChosen = S.bRunJobChosen || !P.RunJob.empty();
		// Union Rules' second job (F23), stored as it was: it pays nothing once the node is not owned.
		P.RunJobSecond = S.RunJobSecond;
		R.Extras.TutorialDone = S.TutorialDone;
		// The run's cards and XP (F24). A save from before them leaves memory's, as it always did. A restored hand is up
		// again as soon as the HUD looks (it opens the pick on any pending hand: Godot's reoffer_after_load).
		if (S.bRunCardsSaved)
		{
			P.CardsTakenThisRun = std::max<int64_t>(S.CardsTakenThisRun, 0);
			ApplyRunCards(D, P, S);
		}
		// Where the run stands (F26-F29). An older save holds the defaults, which is what a restart always gave.
		P.DeepestBiomeIndex = std::clamp<int64_t>(S.DeepestBiomeIndex, 0, static_cast<int64_t>(D.Tiers.size()) - 1);
		P.PendingSecondHelping = std::max(S.PendingSecondHelping, 0.0);
		P.KillsThisCycle = std::max<int64_t>(S.KillsThisCycle, 0);
		P.KillsSinceTithe = std::max<int64_t>(S.KillsSinceTithe, 0);
		P.bReawakenInProgress = S.bReawakenInProgress;
		return R;
	}

	bool HasProgressToSave(const PcProgress& P)
	{
		return !(P.LifetimeCatnipEver <= 0.0 && P.ReawakenCount <= 0);
	}

	PcSlotSummary SlotSummary(const PcSaveState& S)
	{
		PcSlotSummary Out;
		Out.Reawakens = S.ReawakenCount;
		Out.Kills = S.LifetimeKills;
		Out.Catnip = S.LifetimeCatnipEver;
		Out.PlaySeconds = S.TotalPlaySeconds;
		Out.FinalBossSeconds = S.FinalBossSeconds;
		Out.Bosses = static_cast<int64_t>(S.BossesDefeated.size());
		Out.LastPlayedUnix = S.LastPlayedUnix;
		return Out;
	}

	// --- the file ----------------------------------------------------------------------------------------------------

	uint32_t Crc32(const std::string& Bytes)
	{
		static const std::array<uint32_t, 256> Table = []()
		{
			std::array<uint32_t, 256> T{};
			for (uint32_t I = 0; I < 256; ++I)
			{
				uint32_t C = I;
				for (int K = 0; K < 8; ++K)
				{
					C = (C & 1u) ? (0xEDB88320u ^ (C >> 1)) : (C >> 1);
				}
				T[I] = C;
			}
			return T;
		}();
		uint32_t Crc = 0xFFFFFFFFu;
		for (const char Ch : Bytes)
		{
			Crc = Table[(Crc ^ static_cast<uint8_t>(Ch)) & 0xFFu] ^ (Crc >> 8);
		}
		return Crc ^ 0xFFFFFFFFu;
	}

	namespace
	{
		const char* const Magic = "PAWSAVE";

		void Key(std::string& Out, const char* Name, bool& bFirst)
		{
			Out += bFirst ? "\n  " : ",\n  ";
			bFirst = false;
			JsonString(Out, Name);
			Out += ": ";
		}

		void Levels(std::string& Out, const std::map<std::string, int64_t>& M)
		{
			Out += '{';
			bool bFirst = true;
			for (const auto& [K, V] : M)
			{
				Out += bFirst ? "" : ", ";
				bFirst = false;
				JsonString(Out, K);
				Out += ": ";
				JsonInt(Out, V);
			}
			Out += '}';
		}

		// A Godot "id -> true" dictionary.
		void Held(std::string& Out, const std::set<std::string>& S)
		{
			Out += '{';
			bool bFirst = true;
			for (const std::string& K : S)
			{
				Out += bFirst ? "" : ", ";
				bFirst = false;
				JsonString(Out, K);
				Out += ": true";
			}
			Out += '}';
		}

		void Strings(std::string& Out, const std::vector<std::string>& V)
		{
			Out += '[';
			for (size_t I = 0; I < V.size(); ++I)
			{
				Out += I == 0 ? "" : ", ";
				JsonString(Out, V[I]);
			}
			Out += ']';
		}

		// --- reading: each field takes its default when the key is missing (Godot's @export); a present key of the
		// wrong type fails the read (a file that says something else than we wrote is not trusted). ---
		struct Reader
		{
			const PcJsonValue& Body;
			std::string Error;

			bool Fail(const char* Name, const char* Want)
			{
				if (Error.empty())
				{
					Error = std::string(Name) + " is not " + Want;
				}
				return false;
			}

			void Double(const char* Name, double& Out)
			{
				if (const PcJsonValue* V = Body.Find(Name))
				{
					if (!JsonNumber(*V, Out))
					{
						Fail(Name, "a number");
					}
				}
			}

			void Int(const char* Name, int64_t& Out)
			{
				if (const PcJsonValue* V = Body.Find(Name))
				{
					if (V->Type == PcJsonValue::Kind::Int)
					{
						Out = V->Int;
					}
					else
					{
						Fail(Name, "an integer");
					}
				}
			}

			void Bool(const char* Name, bool& Out)
			{
				if (const PcJsonValue* V = Body.Find(Name))
				{
					if (V->Type == PcJsonValue::Kind::Bool)
					{
						Out = V->Bool;
					}
					else
					{
						Fail(Name, "true or false");
					}
				}
			}

			void String(const char* Name, std::string& Out)
			{
				if (const PcJsonValue* V = Body.Find(Name))
				{
					if (V->Type == PcJsonValue::Kind::String)
					{
						Out = V->String;
					}
					else
					{
						Fail(Name, "a string");
					}
				}
			}

			void LevelMap(const char* Name, std::map<std::string, int64_t>& Out)
			{
				const PcJsonValue* V = Body.Find(Name);
				if (!V)
				{
					return;
				}
				if (V->Type != PcJsonValue::Kind::Object)
				{
					Fail(Name, "an object");
					return;
				}
				for (const auto& [K, Item] : V->Object)
				{
					if (Item.Type != PcJsonValue::Kind::Int)
					{
						Fail(Name, "an object of integers");
						return;
					}
					Out[K] = Item.Int;
				}
			}

			void StringList(const char* Name, std::vector<std::string>& Out)
			{
				const PcJsonValue* V = Body.Find(Name);
				if (!V)
				{
					return;
				}
				if (V->Type != PcJsonValue::Kind::Array)
				{
					Fail(Name, "an array");
					return;
				}
				for (const PcJsonValue& Item : V->Array)
				{
					if (Item.Type != PcJsonValue::Kind::String)
					{
						Fail(Name, "an array of strings");
						return;
					}
					Out.push_back(Item.String);
				}
			}

			// "id -> true"; an entry set to false is not held (Godot reads them with .get(id, false)).
			void HeldSet(const char* Name, std::set<std::string>& Out)
			{
				const PcJsonValue* V = Body.Find(Name);
				if (!V)
				{
					return;
				}
				if (V->Type != PcJsonValue::Kind::Object)
				{
					Fail(Name, "an object");
					return;
				}
				for (const auto& [K, Item] : V->Object)
				{
					if (Item.Type != PcJsonValue::Kind::Bool)
					{
						Fail(Name, "an object of true / false");
						return;
					}
					if (Item.Bool)
					{
						Out.insert(K);
					}
				}
			}
		};
	}

	std::string WriteSaveFile(const PcSaveState& S, const std::string& GameBuild)
	{
		std::string Body = "{";
		bool bFirst = true;
		Key(Body, "version", bFirst); JsonInt(Body, S.Version);
		Key(Body, "game_build", bFirst); JsonString(Body, GameBuild);
		Key(Body, "catnip", bFirst); JsonDouble(Body, S.Catnip);
		Key(Body, "stardust", bFirst); JsonDouble(Body, S.Stardust);
		Key(Body, "reawaken_count", bFirst); JsonInt(Body, S.ReawakenCount);
		Key(Body, "upgrade_levels", bFirst); Levels(Body, S.UpgradeLevels);
		Key(Body, "skill_branch_levels", bFirst); Levels(Body, S.SkillBranchLevels);
		Key(Body, "final_boss_defeated", bFirst); Body += S.bFinalBossDefeated ? "true" : "false";
		Key(Body, "lifetime_catnip_ever", bFirst); JsonDouble(Body, S.LifetimeCatnipEver);
		Key(Body, "lifetime_catnip_this_cycle", bFirst); JsonDouble(Body, S.LifetimeCatnipThisCycle);
		Key(Body, "stardust_granted_ever", bFirst); JsonDouble(Body, S.StardustGrantedEver);
		Key(Body, "bosses_defeated", bFirst); Held(Body, S.BossesDefeated);
		Key(Body, "lifetime_kills", bFirst); JsonInt(Body, S.LifetimeKills);
		Key(Body, "achievements_earned", bFirst); Held(Body, S.AchievementsEarned);
		Key(Body, "achievements_claimed", bFirst); Held(Body, S.AchievementsClaimed);
		Key(Body, "last_played_unix", bFirst); JsonDouble(Body, S.LastPlayedUnix);
		Key(Body, "play_seconds_this_cycle", bFirst); JsonDouble(Body, S.PlaySecondsThisCycle);
		Key(Body, "total_play_seconds", bFirst); JsonDouble(Body, S.TotalPlaySeconds);
		Key(Body, "final_boss_seconds", bFirst); JsonDouble(Body, S.FinalBossSeconds);
		Key(Body, "auto_travel", bFirst); Body += S.bAutoTravel ? "true" : "false";
		Key(Body, "auto_pick_card", bFirst); Body += S.bAutoPickCard ? "true" : "false";
		Key(Body, "tier_kill_counts", bFirst); Levels(Body, S.TierKillCounts);
		Key(Body, "bait_spent", bFirst); Levels(Body, S.BaitSpent);
		Key(Body, "bosses_cleared_cycle", bFirst); Held(Body, S.BossesClearedCycle);
		Key(Body, "jobs_held", bFirst); Held(Body, S.JobsHeld);
		Key(Body, "current_job", bFirst); JsonString(Body, S.CurrentJob);
		Key(Body, "run_job", bFirst); JsonString(Body, S.RunJob);
		Key(Body, "run_job_chosen", bFirst); Body += S.bRunJobChosen ? "true" : "false";
		Key(Body, "run_job_second", bFirst); JsonString(Body, S.RunJobSecond);
		Key(Body, "unicorn_split", bFirst); Body += S.bUnicornSplit ? "true" : "false";
		Key(Body, "tutorial_done", bFirst); Held(Body, S.TutorialDone);
		Key(Body, "run_cards_saved", bFirst); Body += S.bRunCardsSaved ? "true" : "false";
		Key(Body, "cards_owned", bFirst); Levels(Body, S.CardsOwned);
		Key(Body, "cards_banished", bFirst); Held(Body, S.CardsBanished);
		Key(Body, "catches_level", bFirst); JsonInt(Body, S.CatchesLevel);
		Key(Body, "catches_points", bFirst); JsonDouble(Body, S.CatchesPoints);
		Key(Body, "rerolls", bFirst); JsonInt(Body, S.Rerolls);
		Key(Body, "extra_choices", bFirst); JsonInt(Body, S.ExtraChoices);
		Key(Body, "banishes", bFirst); JsonInt(Body, S.Banishes);
		Key(Body, "fixation_hands", bFirst); JsonInt(Body, S.FixationHands);
		Key(Body, "hand", bFirst); Strings(Body, S.Hand);
		Key(Body, "promised", bFirst); Strings(Body, S.Promised);
		Key(Body, "prototype_card", bFirst); JsonString(Body, S.PrototypeCard);
		Key(Body, "cards_taken_this_run", bFirst); JsonInt(Body, S.CardsTakenThisRun);
		Key(Body, "deepest_biome_index", bFirst); JsonInt(Body, S.DeepestBiomeIndex);
		Key(Body, "pending_second_helping", bFirst); JsonDouble(Body, S.PendingSecondHelping);
		Key(Body, "kills_this_cycle", bFirst); JsonInt(Body, S.KillsThisCycle);
		Key(Body, "kills_since_tithe", bFirst); JsonInt(Body, S.KillsSinceTithe);
		Key(Body, "reawaken_in_progress", bFirst); Body += S.bReawakenInProgress ? "true" : "false";
		Body += "\n}\n";

		char Header[96];
		std::snprintf(Header, sizeof(Header), "%s %lld %08x %llu\n", Magic, static_cast<long long>(PcSaveFormat),
			static_cast<unsigned>(Crc32(Body)), static_cast<unsigned long long>(Body.size()));
		return std::string(Header) + Body;
	}

	PcSaveReadError ReadSaveFile(const std::string& Bytes, PcSaveState& Out, std::string* Detail)
	{
		auto Say = [Detail](PcSaveReadError E, const std::string& Why)
		{
			if (Detail)
			{
				*Detail = Why;
			}
			return E;
		};
		Out = PcSaveState();
		if (Bytes.empty())
		{
			return Say(PcSaveReadError::Empty, "no bytes");
		}
		const size_t Eol = Bytes.find('\n');
		if (Eol == std::string::npos || Eol > 80)
		{
			return Say(PcSaveReadError::BadHeader, "no header line");
		}
		const std::string Header = Bytes.substr(0, Eol);
		// "PAWSAVE <format> <crc8> <length>"
		const size_t MagicLen = std::strlen(Magic);
		if (Header.compare(0, MagicLen, Magic) != 0 || Header.size() <= MagicLen || Header[MagicLen] != ' ')
		{
			return Say(PcSaveReadError::BadHeader, "not a PAWSAVE file");
		}
		const char* P = Header.data() + MagicLen + 1;
		const char* E = Header.data() + Header.size();
		int64_t Format = 0;
		std::from_chars_result R = std::from_chars(P, E, Format);
		if (R.ec != std::errc() || R.ptr >= E || *R.ptr != ' ')
		{
			return Say(PcSaveReadError::BadHeader, "the format is not a number");
		}
		if (Format > PcSaveFormat)
		{
			return Say(PcSaveReadError::NewerFormat, "written by a newer build (format " + std::to_string(Format) + ")");
		}
		if (Format < 1)
		{
			return Say(PcSaveReadError::BadHeader, "format " + std::to_string(Format));
		}
		P = R.ptr + 1;
		uint32_t Crc = 0;
		R = std::from_chars(P, E, Crc, 16);
		if (R.ec != std::errc() || R.ptr - P != 8 || R.ptr >= E || *R.ptr != ' ')
		{
			return Say(PcSaveReadError::BadHeader, "the checksum is not 8 hex digits");
		}
		P = R.ptr + 1;
		unsigned long long Length = 0;
		R = std::from_chars(P, E, Length);
		if (R.ec != std::errc() || R.ptr != E)
		{
			return Say(PcSaveReadError::BadHeader, "the length is not a number");
		}
		const std::string Body = Bytes.substr(Eol + 1);
		if (Body.size() != Length)
		{
			return Say(PcSaveReadError::BadLength, "the body is " + std::to_string(Body.size()) + " bytes, the header says " +
				std::to_string(Length));
		}
		if (Crc32(Body) != Crc)
		{
			return Say(PcSaveReadError::BadCrc, "the checksum does not match the body");
		}
		PcJsonValue Root;
		std::string JsonError;
		if (!ParseJson(Body, Root, JsonError))
		{
			return Say(PcSaveReadError::BadJson, JsonError);
		}
		if (Root.Type != PcJsonValue::Kind::Object)
		{
			return Say(PcSaveReadError::BadJson, "the body is not an object");
		}
		Reader Rd{Root, std::string()};
		PcSaveState S;
		Rd.Int("version", S.Version);
		Rd.Double("catnip", S.Catnip);
		Rd.Double("stardust", S.Stardust);
		Rd.Int("reawaken_count", S.ReawakenCount);
		Rd.LevelMap("upgrade_levels", S.UpgradeLevels);
		Rd.LevelMap("skill_branch_levels", S.SkillBranchLevels);
		Rd.Bool("final_boss_defeated", S.bFinalBossDefeated);
		Rd.Double("lifetime_catnip_ever", S.LifetimeCatnipEver);
		Rd.Double("lifetime_catnip_this_cycle", S.LifetimeCatnipThisCycle);
		Rd.Double("stardust_granted_ever", S.StardustGrantedEver);
		Rd.HeldSet("bosses_defeated", S.BossesDefeated);
		Rd.Int("lifetime_kills", S.LifetimeKills);
		Rd.HeldSet("achievements_earned", S.AchievementsEarned);
		Rd.HeldSet("achievements_claimed", S.AchievementsClaimed);
		Rd.Double("last_played_unix", S.LastPlayedUnix);
		Rd.Double("play_seconds_this_cycle", S.PlaySecondsThisCycle);
		Rd.Double("total_play_seconds", S.TotalPlaySeconds);
		Rd.Double("final_boss_seconds", S.FinalBossSeconds);
		Rd.Bool("auto_travel", S.bAutoTravel);
		Rd.Bool("auto_pick_card", S.bAutoPickCard);
		Rd.LevelMap("tier_kill_counts", S.TierKillCounts);
		Rd.LevelMap("bait_spent", S.BaitSpent);
		Rd.HeldSet("bosses_cleared_cycle", S.BossesClearedCycle);
		Rd.HeldSet("jobs_held", S.JobsHeld);
		Rd.String("current_job", S.CurrentJob);
		Rd.String("run_job", S.RunJob);
		Rd.Bool("run_job_chosen", S.bRunJobChosen);
		Rd.String("run_job_second", S.RunJobSecond);
		Rd.Bool("unicorn_split", S.bUnicornSplit);
		Rd.HeldSet("tutorial_done", S.TutorialDone);
		Rd.Bool("run_cards_saved", S.bRunCardsSaved);
		Rd.LevelMap("cards_owned", S.CardsOwned);
		Rd.HeldSet("cards_banished", S.CardsBanished);
		Rd.Int("catches_level", S.CatchesLevel);
		Rd.Double("catches_points", S.CatchesPoints);
		Rd.Int("rerolls", S.Rerolls);
		Rd.Int("extra_choices", S.ExtraChoices);
		Rd.Int("banishes", S.Banishes);
		Rd.Int("fixation_hands", S.FixationHands);
		Rd.StringList("hand", S.Hand);
		Rd.StringList("promised", S.Promised);
		Rd.String("prototype_card", S.PrototypeCard);
		Rd.Int("cards_taken_this_run", S.CardsTakenThisRun);
		Rd.Int("deepest_biome_index", S.DeepestBiomeIndex);
		Rd.Double("pending_second_helping", S.PendingSecondHelping);
		Rd.Int("kills_this_cycle", S.KillsThisCycle);
		Rd.Int("kills_since_tithe", S.KillsSinceTithe);
		Rd.Bool("reawaken_in_progress", S.bReawakenInProgress);
		if (!Rd.Error.empty())
		{
			return Say(PcSaveReadError::BadJson, Rd.Error);
		}
		Out = std::move(S);
		return PcSaveReadError::None;
	}

	const char* SaveReadErrorName(PcSaveReadError E)
	{
		switch (E)
		{
		case PcSaveReadError::None: return "none";
		case PcSaveReadError::Empty: return "empty";
		case PcSaveReadError::BadHeader: return "bad header";
		case PcSaveReadError::NewerFormat: return "newer format";
		case PcSaveReadError::BadLength: return "bad length";
		case PcSaveReadError::BadCrc: return "bad checksum";
		case PcSaveReadError::BadJson: return "bad body";
		}
		return "unknown";
	}
}
