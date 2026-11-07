#pragma once

// Number formatting that is a game rule (docs/03 §2 PcFormat): the port of Godot's Amounts.short. Plain strings; the
// game module supplies the per-language style (grouping, decimal separator, suffixes) from its string tables and wraps
// the result in FText.

#include "PcCore.h"

#include <array>
#include <string>

namespace Pc
{
	struct PcAmountStyle
	{
		// 1000 (English: 1.2M) or 10000 (Chinese, Japanese, Korean: 123.5万). Translators choose it.
		double Group = 1000.0;
		std::string DecimalSeparator = ".";
		// One per step of the grouping; with 10000 grouping the slots mean 10^4, 10^8, ... (translated).
		std::array<std::string, 6> Suffixes = { "K", "M", "B", "T", "Qa", "Qi" };
	};

	// `1234567` -> "1.2M" in English, "123,5万" with 10000 grouping and a comma. Below one group the whole number; one
	// decimal while the number is under a tenth of the group (1.4M, 99.9M); none above ("847M" beats "847.3M" in a HUD read
	// at a glance). The sign is kept. Matches Godot's Amounts.short bit for bit (tested against 4,403 Godot outputs) except
	// past the int64 range, where Godot printed an overflowed integer and this prints the digits, and for NaN / infinity,
	// where it prints "nan" / "inf" with the last suffix ("infQi") instead.
	PAWCORE_API std::string ShortAmount(double Value, const PcAmountStyle& Style = PcAmountStyle());

	// GDScript's "%.<Digits>f" % Value on Windows: 17 significant digits first, then rounded HALF UP at the last kept digit
	// (the msvcrt printf Godot's MinGW build links; KB G/03), zero-padded to Digits decimals, "." as the separator.
	// Digits 0..6. NaN and the infinities print "nan", "inf", "-inf", as Godot does.
	PAWCORE_API std::string FixedLikeGodot(double Value, int Digits);
	// GDScript's roundi(Value): half away from zero, as a whole number. NaN, the infinities and anything past int64 give
	// -9223372036854775808, as Godot does on x86-64.
	PAWCORE_API std::string RoundiLikeGodot(double Value);
	// GDScript's int(Value): toward zero; NaN, the infinities and past int64 as RoundiLikeGodot.
	PAWCORE_API std::string IntLikeGodot(double Value);
}
