#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcFormat.h"

#include <cstdlib>
#include <sstream>
#include <string>

namespace
{
	// Tests/Data next to this file; the build script runs the exe from the repo root.
	std::string FixturePath()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		const std::string Data = Env ? Env : "Data";
		return Data + "/../Source/PawCore/Tests/Data/amounts_godot.tsv";
	}
}

TEST_CASE("Amounts.short: the cases check_hud.gd asserts")
{
	CHECK(Pc::ShortAmount(999.0) == "999");
	CHECK(Pc::ShortAmount(1234.0) == "1.2K");
	CHECK(Pc::ShortAmount(1234567.0) == "1.2M");
	CHECK(Pc::ShortAmount(847300000.0) == "847M");
	CHECK(Pc::ShortAmount(-2500.0) == "-2.5K");
	CHECK(Pc::ShortAmount(1.5e12) == "1.5T");
	CHECK(Pc::ShortAmount(3.0e18) == "3.0Qi");
	Pc::PcAmountStyle Zz;
	Zz.Group = 10000.0;
	Zz.DecimalSeparator = ",";
	Zz.Suffixes = { "\xE4\xB8\x87", "\xE4\xBA\xBF", "B", "T", "Qa", "Qi" }; // 万 亿
	CHECK(Pc::ShortAmount(9999.0, Zz) == "9999");
	CHECK(Pc::ShortAmount(1234567.0, Zz) == "123,5\xE4\xB8\x87");
	CHECK(Pc::ShortAmount(250000000.0, Zz) == "2,5\xE4\xBA\xBF");
	CHECK(Pc::ShortAmount(1234.0, Zz) == "1234");
}

TEST_CASE("Amounts.short matches Godot 4.7.1 on 4,403 values, bit for bit")
{
	std::string Text;
	REQUIRE_MESSAGE(Pc::ReadTextFile(FixturePath(), Text), "missing " << FixturePath());
	Pc::PcAmountStyle En;
	Pc::PcAmountStyle Zz;
	Zz.Group = 10000.0;
	Zz.DecimalSeparator = ",";
	Zz.Suffixes = { "W", "Y", "Z", "J", "G", "X" };
	std::istringstream Lines(Text);
	std::string Line;
	int Rows = 0, Skipped = 0;
	while (std::getline(Lines, Line))
	{
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		const size_t A = Line.find('\t');
		const size_t B = Line.find('\t', A + 1);
		const double Value = std::strtod(Line.substr(0, A).c_str(), nullptr);
		const std::string WantEn = Line.substr(A + 1, B - A - 1);
		const std::string WantZz = Line.substr(B + 1);
		// Godot's int() overflowed past the int64 range (1e300 -> "-9223372036854775808Qi"); the port prints the digits.
		if (WantEn.find("9223372036854775808") != std::string::npos)
		{
			++Skipped;
			continue;
		}
		CAPTURE(Line);
		CHECK(Pc::ShortAmount(Value, En) == WantEn);
		CHECK(Pc::ShortAmount(Value, Zz) == WantZz);
		++Rows;
	}
	CHECK(Rows == 4402);
	CHECK(Skipped == 1);
}

TEST_CASE("Amounts.short past the int64 range prints digits, never an overflowed number")
{
	const std::string S = Pc::ShortAmount(1e300);
	CHECK(S.find('-') == std::string::npos);
	CHECK(S.substr(S.size() - 2) == "Qi");
	CHECK(S.rfind("1000000000", 0) == 0);
}

#endif
