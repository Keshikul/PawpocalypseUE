// DataTable parity (docs/06 §4, §8; docs/09 L3): the game's road to the data gives PawCore the same text as the CSVs.
//
// Pawpocalypse.Data.Parity  every DataTable, rebuilt by FPawDataAdapter, equals Data/<name>.csv cell for cell (header,
//                           row order, text), and the data built from the tables passes the validator exactly as the
//                           CSV-built data does. Because both routes feed the same text to the same parser
//                           (Pc::BuildGameData), equal text is equal numbers: this is the game's third of the mirror.
// Pawpocalypse.Data.ParityCatchesDrift  the comparison is not vacuous: a transient copy of a table with one changed
//                           cell, and one with an upper-case row name, are each reported.
//
// UnrealEditor-Cmd.exe Pawpocalypse.uproject -ExecCmds="Automation RunTests Pawpocalypse.Data;Quit"
//     -TestExit="Automation Test Queue Empty" -nullrhi -unattended -nop4 -SCCProvider=None -stdout   (editor closed)

#include "Core/PawLog.h"
#include "Data/PawDataAdapter.h"
#include "Data/PawDataRows.h"
#include "Engine/DataTable.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "PcData.h"
#include "PcValidate.h"

#include <algorithm>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// The first difference between two tables, or empty when they are equal.
	FString FirstDifference(const Pc::CsvTable& Csv, const Pc::CsvTable& Table)
	{
		if (Csv.Header != Table.Header)
		{
			return TEXT("the columns differ (regenerate PawDataRows.h: py tools/gen_paw_rows.py, then build and import)");
		}
		if (Csv.Rows.size() != Table.Rows.size())
		{
			return FString::Printf(TEXT("%d CSV rows, %d table rows"), static_cast<int32>(Csv.Rows.size()), static_cast<int32>(Table.Rows.size()));
		}
		for (size_t R = 0; R < Csv.Rows.size(); ++R)
		{
			for (size_t C = 0; C < Csv.Header.size(); ++C)
			{
				if (Csv.Rows[R][C] != Table.Rows[R][C])
				{
					return FString::Printf(TEXT("row %d (%s) column %s: CSV '%s', table '%s'"), static_cast<int32>(R),
						UTF8_TO_TCHAR(Csv.Rows[R][0].c_str()), UTF8_TO_TCHAR(Csv.Header[C].c_str()),
						UTF8_TO_TCHAR(Csv.Rows[R][C].c_str()), UTF8_TO_TCHAR(Table.Rows[R][C].c_str()));
				}
			}
		}
		return FString();
	}

	bool ReadCsv(const FString& Name, Pc::CsvTable& Out, FString& Error)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Data") / (Name + TEXT(".csv")));
		std::string Text, ParseError;
		if (!Pc::ReadTextFile(TCHAR_TO_UTF8(*Path), Text))
		{
			Error = TEXT("cannot read ") + Path;
			return false;
		}
		if (!Pc::ParseCsv(Text, Out, ParseError))
		{
			Error = Path + TEXT(": ") + UTF8_TO_TCHAR(ParseError.c_str());
			return false;
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawDataParityTest, "Pawpocalypse.Data.Parity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawDataParityTest::RunTest(const FString& Parameters)
{
	std::map<std::string, Pc::CsvTable> Tables;
	TArray<FString> Errors;
	const bool bLoaded = FPawDataAdapter::LoadTables(Tables, Errors);
	for (const FString& E : Errors)
	{
		AddError(E);
	}
	if (!bLoaded)
	{
		return false;
	}

	// Every table PawCore reads has a DataTable, and no DataTable is extra.
	const std::vector<std::string>& Names = Pc::GameDataTableNames();
	TestEqual(TEXT("DataTables vs PawCore's tables"), static_cast<int32>(Tables.size()), static_cast<int32>(Names.size()));
	int32 Cells = 0;
	for (const std::string& Name : Names)
	{
		const FString TableName = UTF8_TO_TCHAR(Name.c_str());
		const auto It = Tables.find(Name);
		if (It == Tables.end())
		{
			AddError(TableName + TEXT(": PawCore reads it, no DataTable holds it (PawDataRows.h)"));
			continue;
		}
		Pc::CsvTable Csv;
		FString Error;
		if (!ReadCsv(TableName, Csv, Error))
		{
			AddError(Error);
			continue;
		}
		const FString Difference = FirstDifference(Csv, It->second);
		if (!Difference.IsEmpty())
		{
			AddError(FString::Printf(TEXT("DT_%s differs from Data/%s.csv: %s (run py tools/import_data.py)"), *TableName, *TableName, *Difference));
		}
		Cells += static_cast<int32>(Csv.Rows.size() * Csv.Header.size());
	}

	// The two routes build data the validator judges identically (and the game's route reports no new failure).
	Pc::PcGameData FromCsv, FromTables;
	std::vector<std::string> LoadErrors;
	const std::string DataDir = TCHAR_TO_UTF8(*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Data")));
	TestTrue(TEXT("the CSVs load"), Pc::LoadGameData(DataDir, FromCsv, LoadErrors));
	Errors.Reset();
	TestTrue(TEXT("the DataTables build valid game data"), FPawDataAdapter::BuildGameData(FromTables, Errors));
	for (const FString& E : Errors)
	{
		AddError(E);
	}
	TestTrue(TEXT("the validator's verdict is the same on both routes"), Pc::ValidateGameData(FromCsv) == Pc::ValidateGameData(FromTables));
	// Spot values through the game's route: a row-ordered list (the deck), a code constant, a typed number.
	TestTrue(TEXT("deck order"), FromCsv.Cards.size() == FromTables.Cards.size()
		&& std::equal(FromCsv.Cards.begin(), FromCsv.Cards.end(), FromTables.Cards.begin(),
			[](const Pc::PcCard& A, const Pc::PcCard& B) { return A.Id == B.Id; }));
	TestTrue(TEXT("code constants"), FromTables.Code.BossRealms == FromCsv.Code.BossRealms
		&& FromTables.Code.MinSpawnInterval == FromCsv.Code.MinSpawnInterval);
	TestTrue(TEXT("a typed number"), FromTables.Combat.EvolutionDamagePerStage == FromCsv.Combat.EvolutionDamagePerStage);
	UE_LOG(LogPaw, Display, TEXT("PAW_DATA_PARITY %d tables, %d cells equal"), static_cast<int32>(Names.size()), Cells);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawDataParityCatchesDriftTest, "Pawpocalypse.Data.ParityCatchesDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawDataParityCatchesDriftTest::RunTest(const FString& Parameters)
{
	const UDataTable* Enemies = LoadObject<UDataTable>(nullptr, TEXT("/Game/Pawpocalypse/Data/DT_EnemyData.DT_EnemyData"));
	if (!TestNotNull(TEXT("DT_EnemyData"), Enemies))
	{
		return false;
	}
	Pc::CsvTable Csv;
	FString Error;
	if (!TestTrue(TEXT("EnemyData.csv reads"), ReadCsv(TEXT("EnemyData"), Csv, Error)))
	{
		return false;
	}
	TestTrue(TEXT("the real table matches"), FirstDifference(Csv, FPawDataAdapter::TableFromDataTable(*Enemies)).IsEmpty());

	// One changed cell, in a transient copy (the asset is never touched).
	UDataTable* Changed = DuplicateObject<UDataTable>(Enemies, GetTransientPackage());
	FPawEnemyRow* Row = Changed->FindRow<FPawEnemyRow>(FName(TEXT("alley_dog")), TEXT("parity drift test"));
	if (!TestNotNull(TEXT("alley_dog"), Row))
	{
		return false;
	}
	Row->hp += TEXT("1");
	TestFalse(TEXT("a changed cell is reported"), FirstDifference(Csv, FPawDataAdapter::TableFromDataTable(*Changed)).IsEmpty());

	// An upper-case row name still reads as its lower-case id (FName case is not stable, KB U/05 §3).
	UDataTable* Renamed = DuplicateObject<UDataTable>(Enemies, GetTransientPackage());
	const FPawEnemyRow Copy = *Renamed->FindRow<FPawEnemyRow>(FName(TEXT("alley_dog")), TEXT("parity drift test"));
	Renamed->RemoveRow(FName(TEXT("alley_dog")));
	Renamed->AddRow(FName(TEXT("ALLEY_DOG")), Copy);
	// RemoveRow + AddRow puts the row back in the SAME place (the row map reuses the freed slot, measured 5.8.3), so the
	// rebuilt table equals the CSV: the upper-case name reads as its lower-case id.
	const Pc::CsvTable Renamed2 = FPawDataAdapter::TableFromDataTable(*Renamed);
	TestTrue(TEXT("an upper-case row name reads as its lower-case id"), FirstDifference(Csv, Renamed2).IsEmpty());

	// A missing row is a difference.
	UDataTable* Shorter = DuplicateObject<UDataTable>(Enemies, GetTransientPackage());
	Shorter->RemoveRow(FName(TEXT("alley_dog")));
	TestFalse(TEXT("a missing row is reported"), FirstDifference(Csv, FPawDataAdapter::TableFromDataTable(*Shorter)).IsEmpty());
	return !HasAnyErrors();
}

#endif
