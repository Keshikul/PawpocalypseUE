#include "PcData.h"

#include <algorithm>
#include <utility>

namespace Pc
{
	namespace
	{
		// GodotFiles, when given, receives each row's `_godot_file` (the exporter's column: the .tres path the row came from).
		template <typename RowT>
		bool LoadTable(const std::map<std::string, CsvTable>& Tables, const char* Class, std::vector<RowT>& Out,
			std::vector<std::string>& Errors, std::vector<std::string>* GodotFiles = nullptr)
		{
			const auto It = Tables.find(Class);
			if (It == Tables.end())
			{
				Errors.push_back(std::string("missing table ") + Class);
				return false;
			}
			const CsvTable& Table = It->second;
			if (Table.Column("id") < 0)
			{
				Errors.push_back(std::string(Class) + ".csv: no id column");
				return false;
			}
			const size_t Before = Errors.size();
			const int FileColumn = Table.Column("_godot_file");
			Out.clear();
			for (size_t R = 0; R < Table.Rows.size(); ++R)
			{
				RowT Row;
				ReadRow(Table, R, Row, Errors);
				Out.push_back(Row);
				if (GodotFiles)
				{
					GodotFiles->push_back(FileColumn >= 0 ? Table.Rows[R][static_cast<size_t>(FileColumn)] : std::string());
				}
			}
			return Errors.size() == Before;
		}

		// "tier_12" -> 12; biome order is the number in the id (the CSV is sorted as text: tier_1, tier_10, ...).
		int64_t TrailingNumber(const std::string& Id)
		{
			const size_t P = Id.find_last_of('_');
			int64_t N = 0;
			return P != std::string::npos && ParseInt(Id.substr(P + 1), N) ? N : -1;
		}

		// Godot loads a Resource folder in sorted file-name order (ResourceDir.list()); the CSV is sorted by id. Rows whose ORDER
		// is a rule (the deck a hand is drawn from, the achievement list) are put back in Godot's order.
		template <typename RowT>
		void SortByGodotFile(std::vector<RowT>& Rows, const std::vector<std::string>& Files)
		{
			if (Files.size() != Rows.size())
			{
				return;
			}
			std::vector<size_t> Order(Rows.size());
			for (size_t I = 0; I < Order.size(); ++I)
			{
				Order[I] = I;
			}
			std::stable_sort(Order.begin(), Order.end(), [&Files](size_t A, size_t B) { return Files[A] < Files[B]; });
			std::vector<RowT> Sorted;
			Sorted.reserve(Order.size());
			for (const size_t I : Order)
			{
				Sorted.push_back(Rows[I]);
			}
			Rows = std::move(Sorted);
		}

		template <typename RowT>
		void Index(const std::vector<RowT>& Rows, std::unordered_map<std::string, size_t>& Map)
		{
			Map.clear();
			for (size_t I = 0; I < Rows.size(); ++I)
			{
				Map.emplace(Rows[I].Id, I);
			}
		}

		template <typename RowT>
		const RowT* Find(const std::vector<RowT>& Rows, const std::unordered_map<std::string, size_t>& Map, const std::string& Id)
		{
			const auto It = Map.find(Id);
			return It == Map.end() ? nullptr : &Rows[It->second];
		}
	}

	const PcEnemy* PcGameData::FindEnemy(const std::string& Id) const { return Find(Enemies, EnemyIndex, Id); }
	const PcBoss* PcGameData::FindBoss(const std::string& Id) const { return Find(Bosses, BossIndex, Id); }
	const PcUpgrade* PcGameData::FindUpgrade(const std::string& Id) const { return Find(Upgrades, UpgradeIndex, Id); }
	const PcSkillBranch* PcGameData::FindBranch(const std::string& Id) const { return Find(Branches, BranchIndex, Id); }
	const PcJob* PcGameData::FindJob(const std::string& Id) const { return Find(Jobs, JobIndex, Id); }

	void PcGameData::RebuildIndex()
	{
		Index(Enemies, EnemyIndex);
		Index(Bosses, BossIndex);
		Index(Upgrades, UpgradeIndex);
		Index(Branches, BranchIndex);
		Index(Jobs, JobIndex);
	}

	const std::vector<std::string>& GameDataTableNames()
	{
		static const std::vector<std::string> Names = {
			"AchievementData", "BossData", "CardData", "CodeConstants", "CombatConstantsData", "EnemyData", "EvolutionData",
			"JobData", "ReawakenConstantsData", "SkillBranchData", "TierData", "UpgradeData",
		};
		return Names;
	}

	bool LoadGameData(const std::string& Dir, PcGameData& Out, std::vector<std::string>& Errors)
	{
		std::map<std::string, CsvTable> Tables;
		bool bOk = true;
		for (const std::string& Name : GameDataTableNames())
		{
			std::string Text, Error;
			const std::string Path = Dir + "/" + Name + ".csv";
			if (!ReadTextFile(Path, Text))
			{
				Errors.push_back("missing " + Path);
				bOk = false;
				continue;
			}
			if (!ParseCsv(Text, Tables[Name], Error))
			{
				Errors.push_back(Name + ".csv: " + Error);
				Tables.erase(Name);
				bOk = false;
			}
		}
		// Built even when a file failed, so Errors lists every problem at once.
		return BuildGameData(Tables, Out, Errors) && bOk;
	}

	bool BuildGameData(const std::map<std::string, CsvTable>& Tables, PcGameData& Out, std::vector<std::string>& Errors)
	{
		Out = PcGameData();
		bool bOk = true;
		bOk &= LoadTable(Tables, "TierData", Out.Tiers, Errors);
		bOk &= LoadTable(Tables, "EnemyData", Out.Enemies, Errors);
		bOk &= LoadTable(Tables, "BossData", Out.Bosses, Errors);
		bOk &= LoadTable(Tables, "UpgradeData", Out.Upgrades, Errors);
		bOk &= LoadTable(Tables, "SkillBranchData", Out.Branches, Errors);
		bOk &= LoadTable(Tables, "EvolutionData", Out.Evolutions, Errors);
		std::vector<std::string> CardFiles;
		bOk &= LoadTable(Tables, "CardData", Out.Cards, Errors, &CardFiles);
		bOk &= LoadTable(Tables, "JobData", Out.Jobs, Errors);
		std::vector<std::string> AchievementFiles;
		bOk &= LoadTable(Tables, "AchievementData", Out.Achievements, Errors, &AchievementFiles);
		std::vector<PcCombatConstants> Combat;
		std::vector<PcReawakenConstants> Reawaken;
		bOk &= LoadTable(Tables, "CombatConstantsData", Combat, Errors);
		bOk &= LoadTable(Tables, "ReawakenConstantsData", Reawaken, Errors);
		if (Combat.size() != 1 || Reawaken.size() != 1)
		{
			Errors.push_back("CombatConstantsData and ReawakenConstantsData must have exactly one row each");
			bOk = false;
		}
		else
		{
			Out.Combat = Combat[0];
			Out.Reawaken = Reawaken[0];
		}
		if (const auto It = Tables.find("CodeConstants"); It != Tables.end())
		{
			bOk &= ReadCodeConstants(It->second, Out.Code, Errors);
		}
		else
		{
			Errors.push_back("missing table CodeConstants");
			bOk = false;
		}
		std::stable_sort(Out.Tiers.begin(), Out.Tiers.end(),
			[](const PcTier& A, const PcTier& B) { return TrailingNumber(A.Id) < TrailingNumber(B.Id); });
		std::stable_sort(Out.Evolutions.begin(), Out.Evolutions.end(),
			[](const PcEvolution& A, const PcEvolution& B) { return A.Stage < B.Stage; });
		// The deck (a hand is drawn from it in this order) and the achievement list (display order) as Godot loads them.
		SortByGodotFile(Out.Cards, CardFiles);
		SortByGodotFile(Out.Achievements, AchievementFiles);
		Out.RebuildIndex();
		return bOk;
	}
}
