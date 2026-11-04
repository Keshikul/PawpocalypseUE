#pragma once

// A small JSON reader and writer for the save file (docs/08 §2): objects, arrays, strings, booleans, null, and numbers
// kept as written (an integer stays an exact int64; anything else is a double). Pure C++17. The reader never throws and
// never recurses past MaxDepth (a hostile file must not crash the game: KB U/10 §1); the writer emits a fixed key order,
// so the same state always writes the same bytes.

#include "PcCore.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Pc
{
	struct PcJsonValue
	{
		enum class Kind : uint8_t { Null, Bool, Int, Double, String, Array, Object };
		Kind Type = Kind::Null;
		bool Bool = false;
		int64_t Int = 0;
		double Double = 0.0;
		std::string String;
		std::vector<PcJsonValue> Array;
		std::vector<std::pair<std::string, PcJsonValue>> Object; // in file order; the first of a repeated key wins

		// The member called Key, or nullptr (also when this is not an object).
		PAWCORE_API const PcJsonValue* Find(const std::string& Key) const;
		bool IsNumber() const { return Type == Kind::Int || Type == Kind::Double; }
		// A number as a double (Int converted); 0 for anything else.
		PAWCORE_API double AsDouble() const;
	};

	constexpr int PcJsonMaxDepth = 32;

	// Parses one JSON value filling the whole text (whitespace around it allowed). False with Error set on anything else.
	PAWCORE_API bool ParseJson(const std::string& Text, PcJsonValue& Out, std::string& Error);

	// The writer: appends to Out. Doubles are written as the shortest text that reads back as the same double, bit for bit
	// (std::to_chars), always with a '.' or an exponent so they read back as doubles, never as integers; a value that
	// is not finite is written as the string "inf", "-inf" or "nan" (JSON has no such numbers; JsonNumber reads them back).
	PAWCORE_API void JsonString(std::string& Out, const std::string& S);
	PAWCORE_API void JsonDouble(std::string& Out, double V);
	PAWCORE_API void JsonInt(std::string& Out, int64_t V);
	// A number written by JsonDouble (or any JSON number), or the strings it writes for non-finite values.
	PAWCORE_API bool JsonNumber(const PcJsonValue& V, double& Out);
}
