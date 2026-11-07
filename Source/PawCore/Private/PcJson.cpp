#include "PcJson.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <system_error>
#include <unordered_set>

namespace Pc
{
	const PcJsonValue* PcJsonValue::Find(const std::string& Key) const
	{
		if (Type != Kind::Object)
		{
			return nullptr;
		}
		for (const auto& [K, V] : Object)
		{
			if (K == Key)
			{
				return &V;
			}
		}
		return nullptr;
	}

	double PcJsonValue::AsDouble() const
	{
		if (Type == Kind::Int)
		{
			return static_cast<double>(Int);
		}
		return Type == Kind::Double ? Double : 0.0;
	}

	namespace
	{
		// For a decimal number from_chars refused as out of range: does it overflow (true) or underflow (false)? The decimal
		// exponent of its first significant digit decides: "1e400" -> 400, "0.00012e-400" -> -404. The exponent saturates,
		// so "1e99999999999999999999" is still an overflow.
		bool OverflowsDouble(const char* P, const char* End)
		{
			if (P < End && *P == '-')
			{
				++P;
			}
			int64_t Leading = 0;   // significant digits before the point
			int64_t ZerosAfter = 0; // zeros after the point before the first significant digit
			bool bSeen = false;
			for (; P < End && *P >= '0' && *P <= '9'; ++P)
			{
				bSeen = bSeen || *P != '0';
				Leading += bSeen ? 1 : 0;
			}
			if (P < End && *P == '.')
			{
				for (++P; P < End && *P >= '0' && *P <= '9'; ++P)
				{
					if (!bSeen && *P == '0')
					{
						++ZerosAfter;
					}
					else
					{
						bSeen = true;
					}
				}
			}
			int64_t Exponent = 0;
			if (P < End && (*P == 'e' || *P == 'E'))
			{
				++P;
				const bool bMinus = P < End && *P == '-';
				if (P < End && (*P == '+' || *P == '-'))
				{
					++P;
				}
				for (; P < End && *P >= '0' && *P <= '9'; ++P)
				{
					Exponent = std::min<int64_t>(Exponent * 10 + (*P - '0'), 1000000000);
				}
				Exponent = bMinus ? -Exponent : Exponent;
			}
			const int64_t FirstDigit = Leading > 0 ? Leading - 1 : -(ZerosAfter + 1);
			return FirstDigit + Exponent > 0;
		}

		struct Parser
		{
			const char* P = nullptr;
			const char* End = nullptr;
			std::string Error;

			bool Fail(const char* What)
			{
				if (Error.empty())
				{
					Error = What;
				}
				return false;
			}

			void SkipSpace()
			{
				while (P < End && (*P == ' ' || *P == '\t' || *P == '\n' || *P == '\r'))
				{
					++P;
				}
			}

			bool Literal(const char* Word)
			{
				const size_t N = std::strlen(Word);
				if (static_cast<size_t>(End - P) < N || std::memcmp(P, Word, N) != 0)
				{
					return false;
				}
				P += N;
				return true;
			}

			static void AppendUtf8(std::string& Out, uint32_t C)
			{
				if (C < 0x80)
				{
					Out += static_cast<char>(C);
				}
				else if (C < 0x800)
				{
					Out += static_cast<char>(0xC0 | (C >> 6));
					Out += static_cast<char>(0x80 | (C & 0x3F));
				}
				else if (C < 0x10000)
				{
					Out += static_cast<char>(0xE0 | (C >> 12));
					Out += static_cast<char>(0x80 | ((C >> 6) & 0x3F));
					Out += static_cast<char>(0x80 | (C & 0x3F));
				}
				else
				{
					Out += static_cast<char>(0xF0 | (C >> 18));
					Out += static_cast<char>(0x80 | ((C >> 12) & 0x3F));
					Out += static_cast<char>(0x80 | ((C >> 6) & 0x3F));
					Out += static_cast<char>(0x80 | (C & 0x3F));
				}
			}

			bool Hex4(uint32_t& Out)
			{
				if (End - P < 4)
				{
					return Fail("a \\u escape is cut short");
				}
				Out = 0;
				for (int I = 0; I < 4; ++I)
				{
					const char C = *P++;
					Out <<= 4;
					if (C >= '0' && C <= '9')
					{
						Out |= static_cast<uint32_t>(C - '0');
					}
					else if (C >= 'a' && C <= 'f')
					{
						Out |= static_cast<uint32_t>(C - 'a' + 10);
					}
					else if (C >= 'A' && C <= 'F')
					{
						Out |= static_cast<uint32_t>(C - 'A' + 10);
					}
					else
					{
						return Fail("a \\u escape holds a non-hex digit");
					}
				}
				return true;
			}

			bool String(std::string& Out)
			{
				// P is on the opening quote.
				++P;
				while (true)
				{
					if (P >= End)
					{
						return Fail("a string never ends");
					}
					const char C = *P++;
					if (C == '"')
					{
						return true;
					}
					if (static_cast<unsigned char>(C) < 0x20)
					{
						return Fail("a raw control character in a string");
					}
					if (C != '\\')
					{
						Out += C;
						continue;
					}
					if (P >= End)
					{
						return Fail("an escape is cut short");
					}
					const char E = *P++;
					switch (E)
					{
					case '"': Out += '"'; break;
					case '\\': Out += '\\'; break;
					case '/': Out += '/'; break;
					case 'b': Out += '\b'; break;
					case 'f': Out += '\f'; break;
					case 'n': Out += '\n'; break;
					case 'r': Out += '\r'; break;
					case 't': Out += '\t'; break;
					case 'u':
					{
						uint32_t Code = 0;
						if (!Hex4(Code))
						{
							return false;
						}
						if (Code >= 0xD800 && Code <= 0xDBFF)
						{
							uint32_t Low = 0;
							if (End - P < 2 || P[0] != '\\' || P[1] != 'u')
							{
								return Fail("a lone high surrogate");
							}
							P += 2;
							if (!Hex4(Low))
							{
								return false;
							}
							if (Low < 0xDC00 || Low > 0xDFFF)
							{
								return Fail("a high surrogate without its low half");
							}
							Code = 0x10000 + ((Code - 0xD800) << 10) + (Low - 0xDC00);
						}
						else if (Code >= 0xDC00 && Code <= 0xDFFF)
						{
							return Fail("a lone low surrogate");
						}
						AppendUtf8(Out, Code);
						break;
					}
					default:
						return Fail("an unknown escape");
					}
				}
			}

			bool Number(PcJsonValue& Out)
			{
				const char* Start = P;
				if (P < End && *P == '-')
				{
					++P;
				}
				if (P >= End || !(*P >= '0' && *P <= '9'))
				{
					return Fail("a number without digits");
				}
				if (*P == '0' && P + 1 < End && P[1] >= '0' && P[1] <= '9')
				{
					return Fail("a number with a leading zero");
				}
				bool bInteger = true;
				while (P < End && *P >= '0' && *P <= '9')
				{
					++P;
				}
				if (P < End && *P == '.')
				{
					bInteger = false;
					++P;
					if (P >= End || !(*P >= '0' && *P <= '9'))
					{
						return Fail("a decimal point without digits after it");
					}
					while (P < End && *P >= '0' && *P <= '9')
					{
						++P;
					}
				}
				if (P < End && (*P == 'e' || *P == 'E'))
				{
					bInteger = false;
					++P;
					if (P < End && (*P == '+' || *P == '-'))
					{
						++P;
					}
					if (P >= End || !(*P >= '0' && *P <= '9'))
					{
						return Fail("an exponent without digits");
					}
					while (P < End && *P >= '0' && *P <= '9')
					{
						++P;
					}
				}
				if (bInteger)
				{
					int64_t V = 0;
					const std::from_chars_result R = std::from_chars(Start, P, V);
					if (R.ec == std::errc() && R.ptr == P)
					{
						Out.Type = PcJsonValue::Kind::Int;
						Out.Int = V;
						return true;
					}
					// Out of int64's range: kept as a double, like any other JSON reader.
				}
				double D = 0.0;
				const std::from_chars_result R = std::from_chars(Start, P, D);
				if (R.ptr != P || (R.ec != std::errc() && R.ec != std::errc::result_out_of_range))
				{
					return Fail("a number that does not read");
				}
				if (R.ec == std::errc::result_out_of_range)
				{
					// from_chars leaves D untouched here; every other JSON reader (Godot's, Python's) rounds to the
					// nearest double: an infinity for "1e400", zero for "1e-400". The sign stays.
					D = OverflowsDouble(Start, P) ? HUGE_VAL : 0.0;
					D = *Start == '-' ? -D : D;
				}
				Out.Type = PcJsonValue::Kind::Double;
				Out.Double = D;
				return true;
			}

			bool Value(PcJsonValue& Out, int Depth)
			{
				if (Depth > PcJsonMaxDepth)
				{
					return Fail("nested too deep");
				}
				SkipSpace();
				if (P >= End)
				{
					return Fail("a value is missing");
				}
				const char C = *P;
				if (C == '{')
				{
					++P;
					Out.Type = PcJsonValue::Kind::Object;
					SkipSpace();
					if (P < End && *P == '}')
					{
						++P;
						return true;
					}
					// The keys already read: a duplicate keeps the first value, and finding it must not cost a scan of the
					// whole object per key (a hostile save with 100,000 keys).
					std::unordered_set<std::string> Seen;
					while (true)
					{
						SkipSpace();
						if (P >= End || *P != '"')
						{
							return Fail("an object key is not a string");
						}
						std::string Key;
						if (!String(Key))
						{
							return false;
						}
						SkipSpace();
						if (P >= End || *P != ':')
						{
							return Fail("a ':' is missing after a key");
						}
						++P;
						PcJsonValue Member;
						if (!Value(Member, Depth + 1))
						{
							return false;
						}
						if (Seen.insert(Key).second)
						{
							Out.Object.emplace_back(std::move(Key), std::move(Member));
						}
						SkipSpace();
						if (P < End && *P == ',')
						{
							++P;
							continue;
						}
						if (P < End && *P == '}')
						{
							++P;
							return true;
						}
						return Fail("an object is not closed");
					}
				}
				if (C == '[')
				{
					++P;
					Out.Type = PcJsonValue::Kind::Array;
					SkipSpace();
					if (P < End && *P == ']')
					{
						++P;
						return true;
					}
					while (true)
					{
						PcJsonValue Item;
						if (!Value(Item, Depth + 1))
						{
							return false;
						}
						Out.Array.push_back(std::move(Item));
						SkipSpace();
						if (P < End && *P == ',')
						{
							++P;
							continue;
						}
						if (P < End && *P == ']')
						{
							++P;
							return true;
						}
						return Fail("an array is not closed");
					}
				}
				if (C == '"')
				{
					Out.Type = PcJsonValue::Kind::String;
					return String(Out.String);
				}
				if (Literal("true"))
				{
					Out.Type = PcJsonValue::Kind::Bool;
					Out.Bool = true;
					return true;
				}
				if (Literal("false"))
				{
					Out.Type = PcJsonValue::Kind::Bool;
					Out.Bool = false;
					return true;
				}
				if (Literal("null"))
				{
					Out.Type = PcJsonValue::Kind::Null;
					return true;
				}
				return Number(Out);
			}
		};
	}

	bool ParseJson(const std::string& Text, PcJsonValue& Out, std::string& Error)
	{
		Parser Ps;
		Ps.P = Text.data();
		Ps.End = Text.data() + Text.size();
		Out = PcJsonValue();
		if (!Ps.Value(Out, 0))
		{
			Error = Ps.Error;
			Out = PcJsonValue();
			return false;
		}
		Ps.SkipSpace();
		if (Ps.P != Ps.End)
		{
			Error = "text after the value";
			Out = PcJsonValue();
			return false;
		}
		return true;
	}

	void JsonString(std::string& Out, const std::string& S)
	{
		Out += '"';
		for (const char C : S)
		{
			switch (C)
			{
			case '"': Out += "\\\""; break;
			case '\\': Out += "\\\\"; break;
			case '\n': Out += "\\n"; break;
			case '\r': Out += "\\r"; break;
			case '\t': Out += "\\t"; break;
			default:
				if (static_cast<unsigned char>(C) < 0x20)
				{
					char Buf[8];
					std::snprintf(Buf, sizeof(Buf), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(C)));
					Out += Buf;
				}
				else
				{
					Out += C; // UTF-8 bytes pass through
				}
			}
		}
		Out += '"';
	}

	void JsonDouble(std::string& Out, double V)
	{
		if (std::isnan(V))
		{
			Out += "\"nan\"";
			return;
		}
		if (std::isinf(V))
		{
			Out += V > 0.0 ? "\"inf\"" : "\"-inf\"";
			return;
		}
		// The shortest text that reads back as exactly this double (std::to_chars without a precision), locale-free.
		char Buf[64];
		const std::to_chars_result R = std::to_chars(Buf, Buf + sizeof(Buf), V);
		std::string Text(Buf, R.ptr);
		// Keep it a double when read back: "5" would read as an integer, and "-0" would lose its sign.
		if (Text.find_first_of(".eE") == std::string::npos)
		{
			Text += ".0";
		}
		Out += Text;
	}

	void JsonInt(std::string& Out, int64_t V)
	{
		char Buf[32];
		const std::to_chars_result R = std::to_chars(Buf, Buf + sizeof(Buf), V);
		Out.append(Buf, R.ptr);
	}

	bool JsonNumber(const PcJsonValue& V, double& Out)
	{
		if (V.IsNumber())
		{
			Out = V.AsDouble();
			return true;
		}
		if (V.Type == PcJsonValue::Kind::String)
		{
			if (V.String == "inf")
			{
				Out = HUGE_VAL;
				return true;
			}
			if (V.String == "-inf")
			{
				Out = -HUGE_VAL;
				return true;
			}
			if (V.String == "nan")
			{
				Out = std::nan("");
				return true;
			}
		}
		return false;
	}
}
