#include "Data/PawDataAdapter.h"

#include "Data/PawDataRows.h"
#include "Engine/DataTable.h"
#include "PcData.h"
#include "PcValidate.h"
#include "UObject/UnrealType.h"

#include <algorithm>

namespace
{
	std::string Utf8(const FString& Text)
	{
		const FTCHARToUTF8 Converted(*Text);
		return std::string(Converted.Get(), Converted.Length());
	}
}

Pc::CsvTable FPawDataAdapter::TableFromDataTable(const UDataTable& Table)
{
	Pc::CsvTable Out;
	const UScriptStruct* Struct = Table.GetRowStruct();
	TArray<const FStrProperty*> Columns;
	Out.Header.push_back("id");
	for (TFieldIterator<FStrProperty> It(Struct); It; ++It)
	{
		Columns.Add(*It);
		// Lower-cased: a property name is an FName too, and its case is not stable across builds (KB U/05 §3).
		const FString Name = It->GetName().ToLower();
		Out.Header.push_back(Name == TEXT("godot_file") ? std::string("_godot_file") : Utf8(Name));
	}
	for (const TPair<FName, uint8*>& Row : Table.GetRowMap())
	{
		std::vector<std::string> Cells;
		Cells.reserve(Out.Header.size());
		Cells.push_back(Utf8(Row.Key.ToString().ToLower()));
		for (const FStrProperty* Column : Columns)
		{
			Cells.push_back(Utf8(*Column->ContainerPtrToValuePtr<FString>(Row.Value)));
		}
		Out.Rows.push_back(MoveTemp(Cells));
	}
	return Out;
}

bool FPawDataAdapter::LoadTables(std::map<std::string, Pc::CsvTable>& Out, TArray<FString>& Errors)
{
	Out.clear();
	bool bOk = true;
	for (const FPawDataTableInfo& Info : PawDataTables())
	{
		const UDataTable* Table = LoadObject<UDataTable>(nullptr, Info.AssetPath);
		if (!Table)
		{
			Errors.Add(FString::Printf(TEXT("%s: no DataTable at %s (run py tools/import_data.py)"), Info.CsvName, Info.AssetPath));
			bOk = false;
			continue;
		}
		if (Table->GetRowStruct() != Info.RowStruct())
		{
			Errors.Add(FString::Printf(TEXT("%s: %s has row struct %s, expected %s"), Info.CsvName, Info.AssetPath,
				*GetNameSafe(Table->GetRowStruct()), *GetNameSafe(Info.RowStruct())));
			bOk = false;
			continue;
		}
		Out.emplace(Utf8(Info.CsvName), TableFromDataTable(*Table));
	}
	return bOk;
}

bool FPawDataAdapter::BuildGameData(Pc::PcGameData& Out, TArray<FString>& Errors)
{
	std::map<std::string, Pc::CsvTable> Tables;
	if (!LoadTables(Tables, Errors))
	{
		return false;
	}
	std::vector<std::string> LoadErrors;
	bool bOk = Pc::BuildGameData(Tables, Out, LoadErrors);
	for (const std::string& E : LoadErrors)
	{
		Errors.Add(UTF8_TO_TCHAR(E.c_str()));
	}
	const std::vector<std::string>& Known = Pc::KnownGodotDataIssues();
	for (const std::string& F : Pc::ValidateGameData(Out))
	{
		if (std::find(Known.begin(), Known.end(), F) == Known.end())
		{
			Errors.Add(UTF8_TO_TCHAR(F.c_str()));
			bOk = false;
		}
	}
	return bOk;
}
