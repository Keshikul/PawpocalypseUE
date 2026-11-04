#if defined(PAWCORE_SIM_CLI)

// pawcore_sim: PawCore's half of the mirror gate (docs/06 §8). tools/check_mirror.py writes fixtures (upgrade and branch
// LEVELS, never multipliers: a multiplier handed to both sides would skip /data and test only the last line of arithmetic),
// this prints what PawCore makes of each, and the script builds the same state in pawsim and diffs the two at 1e-9.
// Built by tools/build_pawcore_tests.ps1 next to the test exe; UBT compiles this file empty.
//
//   pawcore_sim --data <Data dir> --fixtures <file> [--out <file>]
//   pawcore_sim --data <Data dir> --validate      (tools/fuzz_validator.py: "LOAD <error>" / "RULE <failure>" lines;
//                                                  exit 0 clean, 1 rejected)
//
// Fixture file: one fixture per line, `#` comments. Fields separated by `|`:
//   name=<tag>|upgrades=id:level,id:level|branches=id:level|bosses=<n cleared in ladder order>|reawakens=<n>|deepest=<0-based>|stardust=<hex or decimal>
//   [|catnip_ever=<hex or decimal>]
// Output: a header line, then one tab-separated row per fixture; doubles as C hex floats (%a), so nothing is lost on the way.
// After the fixtures it prints every upgrade's price at levels 0..80 and every branch's price at each level.

#include "PcCombat.h"
#include "PcCsv.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcStats.h"
#include "PcValidate.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	std::vector<std::string> Split(const std::string& S, char Sep)
	{
		std::vector<std::string> Out;
		std::string Cur;
		std::istringstream In(S);
		while (std::getline(In, Cur, Sep))
		{
			Out.push_back(Cur);
		}
		return Out;
	}

	bool ParseLevels(const std::string& S, std::map<std::string, int64_t>& Out)
	{
		for (const std::string& Pair : Split(S, ','))
		{
			if (Pair.empty())
			{
				continue;
			}
			const size_t Colon = Pair.find(':');
			int64_t Level = 0;
			if (Colon == std::string::npos || !Pc::ParseInt(Pair.substr(Colon + 1), Level))
			{
				return false;
			}
			Out[Pair.substr(0, Colon)] = Level;
		}
		return true;
	}

	std::string Hex(double V)
	{
		char Buf[64];
		std::snprintf(Buf, sizeof(Buf), "%a", V);
		return Buf;
	}

	int Fail(const std::string& Message)
	{
		std::fprintf(stderr, "pawcore_sim: %s\n", Message.c_str());
		return 2;
	}
}

int main(int Argc, char** Argv)
{
	std::string DataDir = "Data";
	std::string FixturePath;
	std::string OutPath;
	bool bValidate = false;
	for (int I = 1; I < Argc; ++I)
	{
		const std::string Arg = Argv[I];
		if (I + 1 < Argc && Arg == "--data")
		{
			DataDir = Argv[++I];
		}
		else if (I + 1 < Argc && Arg == "--fixtures")
		{
			FixturePath = Argv[++I];
		}
		else if (Arg == "--validate")
		{
			bValidate = true;
		}
		else if (I + 1 < Argc && Arg == "--out")
		{
			OutPath = Argv[++I];
		}
		else
		{
			return Fail("unknown argument " + Arg);
		}
	}
	if (bValidate)
	{
		// The loader's errors first (a cell that does not parse never reaches the validator), then every rule failure.
		Pc::PcGameData D;
		std::vector<std::string> Errors;
		const bool bLoaded = Pc::LoadGameData(DataDir, D, Errors);
		for (const std::string& E : Errors)
		{
			std::printf("LOAD %s\n", E.c_str());
		}
		if (!bLoaded)
		{
			return 1;
		}
		const std::vector<std::string> Failures = Pc::ValidateGameData(D);
		for (const std::string& F : Failures)
		{
			std::printf("RULE %s\n", F.c_str());
		}
		return Failures.empty() && Errors.empty() ? 0 : 1;
	}
	if (FixturePath.empty())
	{
		return Fail("--fixtures is required");
	}

	Pc::PcGameData D;
	std::vector<std::string> Errors;
	if (!Pc::LoadGameData(DataDir, D, Errors))
	{
		for (const std::string& E : Errors)
		{
			std::fprintf(stderr, "%s\n", E.c_str());
		}
		return Fail("could not load " + DataDir);
	}
	std::string Text;
	if (!Pc::ReadTextFile(FixturePath, Text))
	{
		return Fail("could not read " + FixturePath);
	}

	std::ostringstream Out;
	Out << "name\tevolution_stage\tpaw_dps\taura_dps\tplayer_dps\tkitten_dps_each\tkitten_count\tcurrency_find\treawaken_multiplier\tpoop_dps"
		<< "\tpending_stardust\treawaken_requirement\tcatnip_carryover\tstarting_level\tstarting_biome\tfortune_yield\tgolden\n";
	std::istringstream In(Text);
	std::string Line;
	int Rows = 0;
	while (std::getline(In, Line))
	{
		if (!Line.empty() && Line.back() == '\r')
		{
			Line.pop_back();
		}
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		std::string Name;
		Pc::PcProgress P;
		for (const std::string& Field : Split(Line, '|'))
		{
			const size_t Eq = Field.find('=');
			if (Eq == std::string::npos)
			{
				return Fail("bad field '" + Field + "'");
			}
			const std::string Key = Field.substr(0, Eq);
			const std::string Value = Field.substr(Eq + 1);
			bool bOk = true;
			if (Key == "name")
			{
				Name = Value;
			}
			else if (Key == "upgrades")
			{
				bOk = ParseLevels(Value, P.UpgradeLevels);
			}
			else if (Key == "branches")
			{
				bOk = ParseLevels(Value, P.BranchLevels);
			}
			else if (Key == "bosses")
			{
				int64_t Cleared = 0;
				bOk = Pc::ParseInt(Value, Cleared) && Cleared >= 0 && Cleared <= static_cast<int64_t>(D.Code.BossRealms.size());
				for (int64_t B = 0; bOk && B < Cleared; ++B)
				{
					P.BossesDefeated.insert(D.Code.BossRealms[static_cast<size_t>(B)]);
				}
			}
			else if (Key == "reawakens")
			{
				bOk = Pc::ParseInt(Value, P.ReawakenCount);
			}
			else if (Key == "deepest")
			{
				bOk = Pc::ParseInt(Value, P.DeepestBiomeIndex);
			}
			else if (Key == "catnip_ever")
			{
				char* End = nullptr;
				P.LifetimeCatnipEver = std::strtod(Value.c_str(), &End);
				bOk = End && *End == '\0' && !Value.empty();
			}
			else if (Key == "stardust")
			{
				char* End = nullptr;
				P.StardustGrantedEver = std::strtod(Value.c_str(), &End);
				bOk = End && *End == '\0' && !Value.empty();
			}
			else
			{
				bOk = false;
			}
			if (!bOk)
			{
				return Fail("bad field '" + Field + "' in: " + Line);
			}
		}
		for (const auto& [Id, Level] : P.UpgradeLevels)
		{
			if (!D.FindUpgrade(Id))
			{
				return Fail("unknown upgrade " + Id);
			}
		}
		for (const auto& [Id, Level] : P.BranchLevels)
		{
			if (!D.FindBranch(Id))
			{
				return Fail("unknown branch " + Id);
			}
		}

		const Pc::PcCombatStats S = Pc::RebuildStats(D, P);
		Out << Name << '\t' << Pc::EvolutionStage(D, P) << '\t' << Hex(Pc::PawDps(D.Combat, S)) << '\t' << Hex(Pc::AuraDps(D.Combat, S))
			<< '\t' << Hex(Pc::PlayerDps(D.Combat, S)) << '\t' << Hex(Pc::KittenDps(D.Combat, S)) << '\t' << Pc::KittenCount(D, P)
			<< '\t' << Hex(Pc::CurrencyFindMultiplier(D, P)) << '\t' << Hex(S.ReawakenMultiplier) << '\t' << Hex(Pc::PoopDps(D.Combat, S))
			<< '\t' << Hex(Pc::PendingStardust(D, P)) << '\t' << Hex(Pc::ReawakenRequirement(D, P)) << '\t' << Hex(Pc::CatnipCarryover(D, P))
			<< '\t' << Pc::StartingUpgradeLevel(D, P) << '\t' << Pc::StartingBiomeIndex(D, P) << '\t' << Hex(Pc::FortuneYieldMultiplier(D, P))
			<< '\t';
		for (size_t T = 0; T < D.Tiers.size(); ++T)
		{
			Out << (T ? ";" : "") << Hex(Pc::GoldenChanceFor(D, P, D.Tiers[T]));
		}
		Out << '\n';
		++Rows;
	}
	if (Rows == 0)
	{
		return Fail("no fixtures in " + FixturePath); // an empty report must fail, never compare nothing and pass
	}

	Out << "#prices\nkind\tid\tlevel\tcost\n";
	for (const Pc::PcUpgrade& U : D.Upgrades)
	{
		for (int64_t Level = 0; Level <= 80; ++Level)
		{
			Out << "upgrade\t" << U.Id << '\t' << Level << '\t' << Hex(Pc::UpgradeCost(U, Level)) << '\n';
		}
	}
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		for (int64_t Level = 0; Level <= static_cast<int64_t>(B.LevelCosts.size()); ++Level)
		{
			Out << "branch\t" << B.Id << '\t' << Level << '\t' << Hex(Pc::BranchCost(B, Level)) << '\n';
		}
	}

	const std::string Report = Out.str();
	if (OutPath.empty())
	{
		std::fwrite(Report.data(), 1, Report.size(), stdout);
	}
	else
	{
		FILE* F = std::fopen(OutPath.c_str(), "wb");
		if (!F)
		{
			return Fail("could not write " + OutPath);
		}
		std::fwrite(Report.data(), 1, Report.size(), F);
		std::fclose(F);
	}
	return 0;
}

#endif
