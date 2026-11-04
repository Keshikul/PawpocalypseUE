#pragma once

// CSV reading for PawCore: the format tools/export_godot_data.py writes (RFC 4180: quoted cells, doubled quotes,
// commas and newlines inside quotes), and the typed cell readers the generated PcDataRows.h calls.
// Errors are collected as messages, never thrown (PawCore builds without exceptions inside Unreal).

#include "PcCore.h"
#include "PcEnums.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Pc
{
	struct CsvTable
	{
		std::vector<std::string> Header;
		std::vector<std::vector<std::string>> Rows;

		// -1 when the column is absent (a field at its default in every row has no column, docs/06 §2).
		PAWCORE_API int Column(const std::string& Name) const;
	};

	// Parses the whole text. False + Error for malformed input (unterminated quote, ragged row).
	PAWCORE_API bool ParseCsv(const std::string& Text, CsvTable& Out, std::string& Error);
	PAWCORE_API bool ReadTextFile(const std::string& Path, std::string& Out);

	// Number parsing shared with the rest of PawCore: locale-independent, exact (std::from_chars), whole token, finite.
	PAWCORE_API bool ParseDouble(const std::string& Text, double& Out);
	PAWCORE_API bool ParseInt(const std::string& Text, int64_t& Out);

	// Reads typed cells of one row. An absent column or an empty cell leaves the value untouched (= the Godot default).
	class CsvRowReader
	{
	public:
		PAWCORE_API CsvRowReader(const CsvTable& InTable, size_t InRow, const char* InClass, std::vector<std::string>& InErrors);

		PAWCORE_API void ReadString(const char* Col, std::string& Out) const;
		PAWCORE_API void ReadDouble(const char* Col, double& Out) const;
		PAWCORE_API void ReadInt(const char* Col, int64_t& Out) const;
		PAWCORE_API void ReadBool(const char* Col, bool& Out) const;
		PAWCORE_API void ReadColor(const char* Col, std::array<double, 4>& Out) const;
		PAWCORE_API void ReadDoubleList(const char* Col, std::vector<double>& Out) const;
		PAWCORE_API void ReadStringList(const char* Col, std::vector<std::string>& Out) const;
		// SkillBranchData.extra_effects: a JSON object of name -> number array, e.g. {"mult":[1.06,1.12]}.
		PAWCORE_API void ReadLadderMap(const char* Col, std::map<std::string, std::vector<double>>& Out) const;

		template <typename EnumT>
		void ReadEnum(const char* Col, EnumT& Out) const
		{
			if (const std::string* Text = Cell(Col))
			{
				if (!FromName(*Text, Out))
				{
					Fail(Col, "unknown enum name '" + *Text + "'");
				}
			}
		}

	private:
		PAWCORE_API const std::string* Cell(const char* Col) const;
		PAWCORE_API void Fail(const char* Col, const std::string& Why) const;

		const CsvTable& Table;
		size_t Row;
		const char* Class;
		std::vector<std::string>& Errors;
	};
}
