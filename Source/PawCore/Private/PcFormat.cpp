#include "PcFormat.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace Pc
{
	namespace
	{
		// int64 can hold every magnitude below 2^63; past it Godot's int() overflowed (unreachable in play: boss HP tops
		// out near 2e12), so the digits are printed instead of copying undefined behaviour.
		constexpr double Int64Limit = 9223372036854775807.0;

		std::string WholeNumber(double Magnitude)
		{
			if (Magnitude < Int64Limit)
			{
				// Truncation toward zero, as Godot's int() and floori() do for a non-negative value.
				return std::to_string(static_cast<int64_t>(Magnitude));
			}
			char Buf[400];
			std::snprintf(Buf, sizeof(Buf), "%.0f", std::floor(Magnitude));
			return Buf;
		}
	}

	namespace
	{
		// Godot's "%.1f" on Windows rounds like the legacy msvcrt printf its MinGW build links: the double is first written
		// to 17 significant decimal digits, then rounded HALF UP at the first decimal. So 1.25 -> "1.3", 1.45 (binary
		// 1.44999999999999995559 = "1.4500000000000000") -> "1.5", but 9.95 (binary 9.94999999999999928946 =
		// "9.9499999999999993") -> "9.9". C's correctly rounded printf gave 1.2/1.4/9.9 (67 mismatches) and rounding the
		// scaled double half away from zero gave 1.3/1.5/10.0 (43 mismatches); this matches all 4,402 Godot outputs in
		// Tests/Data/amounts_godot.tsv. Only called for 1 <= value < 1000.
		std::string OneDecimalLikeGodot(double Value, const std::string& Separator)
		{
			char Buf[64];
			std::snprintf(Buf, sizeof(Buf), "%.16e", Value); // d.dddddddddddddddde+XX: 17 correctly rounded digits
			std::string Digits;
			const char* P = Buf;
			for (; *P && *P != 'e'; ++P)
			{
				if (*P >= '0' && *P <= '9')
				{
					Digits += *P;
				}
			}
			const int Exponent = std::atoi(P + 1);
			// Digit i stands for 10^(Exponent - i): the tenths digit is i = Exponent + 1, the one deciding the rounding next.
			const size_t Cut = static_cast<size_t>(Exponent + 2);
			int64_t Tenths = 0;
			for (size_t I = 0; I < Cut; ++I)
			{
				Tenths = Tenths * 10 + (I < Digits.size() ? Digits[I] - '0' : 0);
			}
			if (Cut < Digits.size() && Digits[Cut] >= '5')
			{
				++Tenths;
			}
			return std::to_string(Tenths / 10) + Separator + std::to_string(Tenths % 10);
		}
	}

	std::string ShortAmount(double Value, const PcAmountStyle& Style)
	{
		const double Group = Style.Group;
		double Magnitude = std::fabs(Value);
		const std::string Sign = Value < 0.0 ? "-" : "";
		if (Magnitude < Group)
		{
			return Sign + WholeNumber(std::floor(Magnitude));
		}
		// The same divisions in the same order as Godot, so 1e23 lands on 99999.99... and truncates to "99999Qi" here too.
		size_t Tier = 0;
		while (Magnitude >= Group && Tier < Style.Suffixes.size())
		{
			Magnitude /= Group;
			++Tier;
		}
		std::string Text;
		if (Magnitude < Group / 10.0)
		{
			Text = OneDecimalLikeGodot(Magnitude, Style.DecimalSeparator);
		}
		else
		{
			Text = WholeNumber(Magnitude);
		}
		return Sign + Text + Style.Suffixes[Tier - 1];
	}

	std::string FixedLikeGodot(double Value, int Digits)
	{
		Digits = std::max(0, std::min(Digits, 6));
		const bool bNegative = Value < 0.0;
		const double Abs = std::fabs(Value);
		char Buf[64];
		std::snprintf(Buf, sizeof(Buf), "%.16e", Abs); // 17 correctly rounded significant digits
		std::string Sig;
		const char* P = Buf;
		for (; *P && *P != 'e'; ++P)
		{
			if (*P >= '0' && *P <= '9')
			{
				Sig += *P;
			}
		}
		const int Exponent = std::atoi(P + 1);
		// Digit i weighs 10^(Exponent - i): keep Exponent + 1 + Digits of them, the next one rounds half up.
		const int Keep = Exponent + 1 + Digits;
		if (Exponent > 15)
		{
			std::snprintf(Buf, sizeof(Buf), "%.*f", Digits, Value); // past exact int64 scaling: never reached by the game
			return Buf;
		}
		int64_t Scaled = 0;
		for (int I = 0; I < Keep; ++I)
		{
			Scaled = Scaled * 10 + (I < static_cast<int>(Sig.size()) ? Sig[static_cast<size_t>(I)] - '0' : 0);
		}
		if (Keep >= 0 && Keep < static_cast<int>(Sig.size()) && Sig[static_cast<size_t>(Keep)] >= '5')
		{
			++Scaled;
		}
		int64_t Unit = 1;
		for (int I = 0; I < Digits; ++I)
		{
			Unit *= 10;
		}
		std::string Out = (bNegative ? "-" : "") + std::to_string(Scaled / Unit); // printf keeps the sign of a negative that rounds to 0: "-0"
		if (Digits > 0)
		{
			std::string Frac = std::to_string(Scaled % Unit);
			Out += "." + std::string(static_cast<size_t>(Digits) - Frac.size(), '0') + Frac;
		}
		return Out;
	}

	std::string RoundiLikeGodot(double Value)
	{
		return std::to_string(static_cast<int64_t>(std::round(Value)));
	}

	std::string IntLikeGodot(double Value)
	{
		return std::to_string(static_cast<int64_t>(Value));
	}
}
