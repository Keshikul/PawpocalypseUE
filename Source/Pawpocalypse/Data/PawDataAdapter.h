#pragma once

// The game's road into PawCore's data (docs/03 §6): the DataTables imported from Data/*.csv (tools/import_data.py) are
// turned back into the CSV text they hold and parsed by PawCore's one loader (Pc::BuildGameData), then judged by the
// same validator the tests and pawcore_sim run. The tables' row structs keep every cell as text (PawDataRows.h), so the
// DataTable route and the CSV route meet at the same characters and cannot read a number differently.

#include "CoreMinimal.h"

#include "PcCsv.h"

#include <map>
#include <string>

namespace Pc
{
	struct PcGameData;
}

class UDataTable;

struct PAWPOCALYPSE_API FPawDataAdapter
{
	// One table back as PawCore's CsvTable: header `id` + the row struct's columns in declaration order (= the CSV's),
	// row names as ids (lower-cased: FName keeps whichever case it met first, KB U/05 §3), `godot_file` as `_godot_file`.
	static Pc::CsvTable TableFromDataTable(const UDataTable& Table);

	// Every table of PawDataTables(), loaded and converted, keyed by its CSV name. False (with Errors) if one is missing
	// or has another row struct.
	static bool LoadTables(std::map<std::string, Pc::CsvTable>& Out, TArray<FString>& Errors);

	// The tables -> Pc::BuildGameData -> Pc::ValidateGameData. Validator failures are reported in Errors and make it
	// false, except the Godot data issues PawCore's tests list as known (docs/01 §5); callers decide how loud to be
	// (development fails loudly, Shipping logs: docs/03 §6).
	static bool BuildGameData(Pc::PcGameData& Out, TArray<FString>& Errors);
};
