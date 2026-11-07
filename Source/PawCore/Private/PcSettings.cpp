#include "PcSettings.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <map>

namespace Pc
{
	namespace
	{
		const PcIntSize Resolutions[SettingsResolutionCount] = { { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 } };
		const int64_t FpsOptions[SettingsFpsOptionCount] = { 0, 30, 60, 120, 144, 240 };
		const char* const PromptStyles[SettingsPromptStyleCount] = { "auto", "keyboard", "xbox", "playstation", "steam_deck" };
		const char* const DockSides[SettingsDockSideCount] = { "left", "right" };

		// --- a ConfigFile value, as far as settings need one ----------------------------------------------------------
		enum class Kind : uint8_t { Int, Float, Bool, String, Array, Other };
		struct Value
		{
			Kind K = Kind::Other;
			int64_t I = 0;
			double F = 0.0;
			bool B = false;
			std::string S;
			std::vector<PcSavedEvent> Items; // an Array: a list of flat dictionaries (the [bindings] lists)
		};
		// A section's keys, and the order they first appeared in (ConfigFile keeps it: get_section_keys()).
		struct Section
		{
			std::map<std::string, Value> Values;
			std::vector<std::string> Order;
		};

		// Godot's String::to_float (built_in_strtod, Tcl's strtod): at most 18 mantissa digits, gathered as two 9-digit
		// integers joined in a double, then multiplied or divided by a power of ten built from 10, 100, 1e4, 1e8, 1e16 ...
		// NOT correctly rounded for long mantissas: "0.9500000000000001" reads as 0.95 (measured, fixture `settings`), where
		// a correctly rounded parse gives the next double up. A settings file Godot wrote must read as Godot read it.
		double GodotStrtod(const std::string& Text)
		{
			static const double PowersOf10[] = { 10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256 };
			constexpr int MaxExponent = 511;
			const char* P = Text.c_str();
			while (*P == ' ' || *P == '\t')
			{
				++P;
			}
			bool bNegative = false;
			if (*P == '-' || *P == '+')
			{
				bNegative = *P == '-';
				++P;
			}
			// The mantissa's digits (the point counted), and where the point is.
			int DecPt = -1;
			int MantSize = 0;
			for (;; ++MantSize)
			{
				const char C = P[MantSize];
				if (!std::isdigit(static_cast<unsigned char>(C)))
				{
					if (C != '.' || DecPt >= 0)
					{
						break;
					}
					DecPt = MantSize;
				}
			}
			const char* PExp = P + MantSize;
			if (DecPt < 0)
			{
				DecPt = MantSize;
			}
			else
			{
				MantSize -= 1;
			}
			int FracExp = 0;
			if (MantSize > 18)
			{
				FracExp = DecPt - 18;
				MantSize = 18;
			}
			else
			{
				FracExp = DecPt - MantSize;
			}
			if (MantSize == 0)
			{
				return bNegative ? -0.0 : 0.0;
			}
			int Frac1 = 0;
			for (; MantSize > 9; --MantSize)
			{
				char C = *P++;
				if (C == '.')
				{
					C = *P++;
				}
				Frac1 = 10 * Frac1 + (C - '0');
			}
			int Frac2 = 0;
			for (; MantSize > 0; --MantSize)
			{
				char C = *P++;
				if (C == '.')
				{
					C = *P++;
				}
				Frac2 = 10 * Frac2 + (C - '0');
			}
			double Fraction = (1.0e9 * Frac1) + Frac2;
			// The exponent.
			int Exp = 0;
			bool bExpNegative = false;
			P = PExp;
			if (*P == 'E' || *P == 'e')
			{
				++P;
				if (*P == '-' || *P == '+')
				{
					bExpNegative = *P == '-';
					++P;
				}
				while (std::isdigit(static_cast<unsigned char>(*P)))
				{
					Exp = Exp * 10 + (*P - '0');
					if (Exp > 100000)
					{
						break;
					}
					++P;
				}
			}
			Exp = bExpNegative ? FracExp - Exp : FracExp + Exp;
			const bool bDivide = Exp < 0;
			Exp = std::min(bDivide ? -Exp : Exp, MaxExponent);
			double DblExp = 1.0;
			for (const double* D = PowersOf10; Exp != 0; Exp >>= 1, ++D)
			{
				if (Exp & 1)
				{
					DblExp *= *D;
				}
			}
			Fraction = bDivide ? Fraction / DblExp : Fraction * DblExp;
			return bNegative ? -Fraction : Fraction;
		}

		struct Reader
		{
			const std::string& T;
			size_t At = 0;

			bool End() const { return At >= T.size(); }
			char Peek() const { return End() ? '\0' : T[At]; }

			void SkipBlanks()
			{
				while (!End() && (T[At] == ' ' || T[At] == '\t' || T[At] == '\r'))
				{
					++At;
				}
			}

			void SkipLine()
			{
				while (!End() && T[At] != '\n')
				{
					++At;
				}
			}

			// A quoted string (VariantParser: the C escapes, \uXXXX; a newline inside the quotes is kept).
			bool ReadString(std::string& Out)
			{
				++At; // the opening quote
				while (!End())
				{
					const char C = T[At++];
					if (C == '"')
					{
						return true;
					}
					if (C != '\\' || End())
					{
						Out.push_back(C);
						continue;
					}
					const char E = T[At++];
					switch (E)
					{
					case 'b': Out.push_back('\b'); break;
					case 't': Out.push_back('\t'); break;
					case 'n': Out.push_back('\n'); break;
					case 'f': Out.push_back('\f'); break;
					case 'r': Out.push_back('\r'); break;
					case 'u':
					{
						uint32_t Code = 0;
						int Digits = 0;
						while (Digits < 4 && !End() && std::isxdigit(static_cast<unsigned char>(T[At])))
						{
							const char H = T[At++];
							Code = Code * 16 + static_cast<uint32_t>(std::isdigit(static_cast<unsigned char>(H)) ? H - '0' : (std::tolower(H) - 'a' + 10));
							++Digits;
						}
						// UTF-8 for the code point (settings strings are names: a BMP character is all they can need).
						if (Code < 0x80) { Out.push_back(static_cast<char>(Code)); }
						else if (Code < 0x800) { Out.push_back(static_cast<char>(0xC0 | (Code >> 6))); Out.push_back(static_cast<char>(0x80 | (Code & 0x3F))); }
						else
						{
							Out.push_back(static_cast<char>(0xE0 | (Code >> 12)));
							Out.push_back(static_cast<char>(0x80 | ((Code >> 6) & 0x3F)));
							Out.push_back(static_cast<char>(0x80 | (Code & 0x3F)));
						}
						break;
					}
					default: Out.push_back(E); break; // \" \\ and anything else: the character itself
					}
				}
				return false;
			}

			// Anything settings never hold (an array, a dictionary, Vector2(...), &"name"): skipped whole, brackets balanced
			// across lines, strings inside respected.
			void SkipOther()
			{
				int Depth = 0;
				while (!End())
				{
					const char C = T[At];
					if (C == '"')
					{
						std::string Ignored;
						ReadString(Ignored);
						if (Depth == 0)
						{
							return;
						}
						continue;
					}
					if (C == '[' || C == '{' || C == '(')
					{
						++Depth;
					}
					else if (C == ']' || C == '}' || C == ')')
					{
						--Depth;
						++At;
						if (Depth <= 0)
						{
							return;
						}
						continue;
					}
					else if (C == '\n' && Depth == 0)
					{
						return;
					}
					++At;
				}
			}

			void SkipSpace()
			{
				while (!End() && (T[At] == ' ' || T[At] == '\t' || T[At] == '\r' || T[At] == '\n'))
				{
					++At;
				}
			}

			// A scalar inside a dictionary: a string, a number, true / false. False for anything else.
			bool ReadScalar(PcCfgValue& Out)
			{
				if (Peek() == '"')
				{
					Out.Kind = PcCfgKind::String;
					return ReadString(Out.S);
				}
				const size_t Start = At;
				while (!End() && T[At] != ',' && T[At] != '}' && T[At] != ']' && T[At] != ' ' && T[At] != '\t' && T[At] != '\r' && T[At] != '\n')
				{
					++At;
				}
				const Value V = WordValue(T.substr(Start, At - Start));
				switch (V.K)
				{
				case Kind::Int: Out.Kind = PcCfgKind::Int; Out.I = V.I; return true;
				case Kind::Float: Out.Kind = PcCfgKind::Float; Out.F = V.F; return true;
				case Kind::Bool: Out.Kind = PcCfgKind::Bool; Out.B = V.B; return true;
				default: return false;
				}
			}

			// [ {"key": scalar, ...}, ... ] (VariantWriter's layout, any spacing): false when it is anything else.
			bool ReadArray(std::vector<PcSavedEvent>& Out)
			{
				++At; // [
				SkipSpace();
				if (Peek() == ']')
				{
					++At;
					return true;
				}
				while (!End())
				{
					SkipSpace();
					if (Peek() != '{')
					{
						return false;
					}
					++At;
					PcSavedEvent E;
					SkipSpace();
					if (Peek() == '}')
					{
						++At;
					}
					else
					{
						while (true)
						{
							SkipSpace();
							std::string Key;
							if (Peek() != '"' || !ReadString(Key))
							{
								return false;
							}
							SkipSpace();
							if (Peek() != ':')
							{
								return false;
							}
							++At;
							SkipSpace();
							PcCfgValue V;
							if (!ReadScalar(V))
							{
								return false;
							}
							// A repeated key: the last value, in the first one's place (a Dictionary).
							auto Found = std::find_if(E.Fields.begin(), E.Fields.end(), [&](const std::pair<std::string, PcCfgValue>& F) { return F.first == Key; });
							if (Found != E.Fields.end())
							{
								Found->second = V;
							}
							else
							{
								E.Fields.emplace_back(Key, V);
							}
							SkipSpace();
							if (Peek() == ',')
							{
								++At;
								continue;
							}
							if (Peek() == '}')
							{
								++At;
								break;
							}
							return false;
						}
					}
					Out.push_back(E);
					SkipSpace();
					if (Peek() == ',')
					{
						++At;
						continue;
					}
					if (Peek() == ']')
					{
						++At;
						return true;
					}
					return false;
				}
				return false;
			}

			Value ReadValue()
			{
				Value V;
				SkipBlanks();
				const char C = Peek();
				if (C == '"')
				{
					V.K = ReadString(V.S) ? Kind::String : Kind::Other;
					return V;
				}
				if (C == '[')
				{
					const size_t Start = At;
					if (ReadArray(V.Items))
					{
						V.K = Kind::Array;
						return V;
					}
					V.Items.clear();
					At = Start;
					SkipOther();
					return V;
				}
				if (C == '{' || C == '&' || C == '^')
				{
					SkipOther();
					return V;
				}
				// A word or a number, to the end of the token.
				const size_t Start = At;
				while (!End() && T[At] != '\n' && T[At] != '\r' && T[At] != ' ' && T[At] != '\t' && T[At] != '(' && T[At] != ';')
				{
					++At;
				}
				const std::string Word = T.substr(Start, At - Start);
				if (Peek() == '(')
				{
					SkipOther(); // a constructor: Vector2(1, 2), Color(...)
					return V;
				}
				return WordValue(Word);
			}

			static Value WordValue(const std::string& Word)
			{
				Value V;
				if (Word == "true" || Word == "false")
				{
					V.K = Kind::Bool;
					V.B = Word == "true";
					return V;
				}
				if (Word == "inf" || Word == "-inf" || Word == "nan" || Word == "inf_neg")
				{
					V.K = Kind::Float;
					V.F = Word == "nan" ? std::nan("") : (Word == "inf" ? HUGE_VAL : -HUGE_VAL);
					return V;
				}
				if (Word.empty() || !(std::isdigit(static_cast<unsigned char>(Word[0])) || Word[0] == '-' || Word[0] == '+' || Word[0] == '.'))
				{
					return V; // null, or a word settings never hold
				}
				const bool bFloat = Word.find_first_of(".eE") != std::string::npos;
				const char* First = Word.data() + (Word[0] == '+' ? 1 : 0);
				const char* Last = Word.data() + Word.size();
				if (bFloat)
				{
					double D = 0.0;
					const auto R = std::from_chars(First, Last, D); // only to know the token is a number
					if (R.ec == std::errc() && R.ptr == Last)
					{
						V.K = Kind::Float;
						V.F = GodotStrtod(Word);
					}
				}
				else
				{
					int64_t I = 0;
					const auto R = std::from_chars(First, Last, I);
					if (R.ec == std::errc() && R.ptr == Last)
					{
						V.K = Kind::Int;
						V.I = I;
					}
				}
				return V;
			}

			bool Parse(std::map<std::string, Section>& Out)
			{
				std::string Current;
				bool bAny = false;
				while (!End())
				{
					SkipBlanks();
					const char C = Peek();
					if (C == '\n')
					{
						++At;
						continue;
					}
					if (C == ';' || C == '#')
					{
						SkipLine();
						continue;
					}
					if (C == '[')
					{
						const size_t Close = T.find(']', At);
						if (Close == std::string::npos)
						{
							return bAny;
						}
						Current = T.substr(At + 1, Close - At - 1);
						Out[Current];
						bAny = true;
						At = Close + 1;
						SkipLine();
						continue;
					}
					const size_t Eq = T.find('=', At);
					const size_t Nl = T.find('\n', At);
					if (Eq == std::string::npos || (Nl != std::string::npos && Nl < Eq))
					{
						SkipLine(); // not a key line
						continue;
					}
					std::string Key = T.substr(At, Eq - At);
					while (!Key.empty() && (Key.back() == ' ' || Key.back() == '\t'))
					{
						Key.pop_back();
					}
					At = Eq + 1;
					Section& Sec = Out[Current];
					if (!Sec.Values.count(Key))
					{
						Sec.Order.push_back(Key);
					}
					Sec.Values[Key] = ReadValue();
					bAny = true;
					SkipLine();
				}
				return bAny;
			}
		};

		const Value* Find(const std::map<std::string, Section>& Cfg, const char* Sec, const char* Key)
		{
			const auto S = Cfg.find(Sec);
			if (S == Cfg.end())
			{
				return nullptr;
			}
			const auto K = S->second.Values.find(Key);
			return K == S->second.Values.end() ? nullptr : &K->second;
		}

		// The kinds of read load_settings() makes. A typed float takes an int or a float; float(x) the same.
		void ReadNumber(const std::map<std::string, Section>& Cfg, const char* Sec, const char* Key, double& Out)
		{
			if (const Value* V = Find(Cfg, Sec, Key))
			{
				if (V->K == Kind::Float) { Out = V->F; }
				else if (V->K == Kind::Int) { Out = static_cast<double>(V->I); }
			}
		}

		void ReadInt(const std::map<std::string, Section>& Cfg, const char* Sec, const char* Key, int64_t& Out)
		{
			if (const Value* V = Find(Cfg, Sec, Key))
			{
				if (V->K == Kind::Int) { Out = V->I; }
				else if (V->K == Kind::Float && std::isfinite(V->F)) { Out = static_cast<int64_t>(V->F); }
			}
		}

		// A typed bool takes a bool; bool(x) also an int or a float (non-zero is true).
		void ReadBool(const std::map<std::string, Section>& Cfg, const char* Sec, const char* Key, bool& Out, bool bConverted)
		{
			if (const Value* V = Find(Cfg, Sec, Key))
			{
				if (V->K == Kind::Bool) { Out = V->B; }
				else if (bConverted && V->K == Kind::Int) { Out = V->I != 0; }
				else if (bConverted && V->K == Kind::Float) { Out = V->F != 0.0; }
			}
		}

		void ReadString(const std::map<std::string, Section>& Cfg, const char* Sec, const char* Key, std::string& Out)
		{
			if (const Value* V = Find(Cfg, Sec, Key))
			{
				if (V->K == Kind::String) { Out = V->S; }
			}
		}

		std::string Num(double V)
		{
			// The first spelling that Godot's reader (above) turns back into exactly V: the shortest, then 15, 16, 17 digits,
			// then the same in scientific form (Godot's reader counts a leading zero against its 18 digits: "0.0600...05"
			// loses its last digit, "6.00...05e-2" does not). A float always reads back as a float: 1.0, never 1.
			auto Spell = [](double X, int Digits)
			{
				char Buf[64];
				const auto R = Digits == 0 ? std::to_chars(Buf, Buf + sizeof(Buf), X)
					: Digits > 0 ? std::to_chars(Buf, Buf + sizeof(Buf), X, std::chars_format::general, Digits)
					: std::to_chars(Buf, Buf + sizeof(Buf), X, std::chars_format::scientific, -Digits);
				std::string S(Buf, R.ptr);
				if (std::isfinite(X) && S.find_first_of(".e") == std::string::npos)
				{
					S += ".0";
				}
				return S;
			};
			const std::string Shortest = Spell(V, 0);
			if (!std::isfinite(V))
			{
				return Shortest;
			}
			for (const int Digits : { 0, 15, 16, 17, -14, -15, -16 })
			{
				const std::string S = Digits == 0 ? Shortest : Spell(V, Digits);
				if (GodotStrtod(S) == V)
				{
					return S;
				}
			}
			return Shortest;
		}

		std::string Quote(const std::string& V)
		{
			// c_escape_multiline(): the backslash and the quote; a newline stays a newline.
			std::string Out = "\"";
			for (const char C : V)
			{
				if (C == '\\' || C == '"')
				{
					Out.push_back('\\');
				}
				Out.push_back(C);
			}
			return Out + "\"";
		}
	}

	PcIntSize SettingsResolution(int64_t Index) { return Resolutions[SettingsClampResolution(Index)]; }
	int64_t SettingsFpsOption(int64_t Index) { return FpsOptions[std::clamp<int64_t>(Index, 0, SettingsFpsOptionCount - 1)]; }
	const char* SettingsPromptStyle(int64_t Index) { return PromptStyles[std::clamp<int64_t>(Index, 0, SettingsPromptStyleCount - 1)]; }
	const char* SettingsDockSide(int64_t Index) { return DockSides[std::clamp<int64_t>(Index, 0, SettingsDockSideCount - 1)]; }
	PcIntSize SettingsMinWindow() { return { 1024, 600 }; }

	double SettingsClampVolume(double V) { return std::clamp(V, 0.0, 1.0); }
	int64_t SettingsClampQuality(int64_t V) { return std::clamp<int64_t>(V, 0, SettingsQualityCount - 1); }
	int64_t SettingsClampFps(int64_t V) { return std::max<int64_t>(V, 0); }
	int64_t SettingsClampSlot(int64_t V) { return std::clamp<int64_t>(V, 0, SettingsSlotCount - 1); }
	int64_t SettingsClampResolution(int64_t V) { return std::clamp<int64_t>(V, 0, SettingsResolutionCount - 1); }

	bool SettingsIsPromptStyle(const std::string& V)
	{
		return std::any_of(std::begin(PromptStyles), std::end(PromptStyles), [&](const char* P) { return V == P; });
	}

	bool SettingsIsDockSide(const std::string& V)
	{
		return std::any_of(std::begin(DockSides), std::end(DockSides), [&](const char* P) { return V == P; });
	}

	bool SettingsLoad(const std::string& Text, PcSettings& S)
	{
		std::map<std::string, Section> Cfg;
		Reader R{ Text };
		if (!R.Parse(Cfg))
		{
			return false;
		}
		ReadNumber(Cfg, "audio", "master_volume", S.MasterVolume);
		ReadNumber(Cfg, "audio", "music_volume", S.MusicVolume);
		ReadNumber(Cfg, "audio", "sfx_volume", S.SfxVolume);
		ReadBool(Cfg, "audio", "muted", S.bMuted, false);
		ReadString(Cfg, "audio", "output_device", S.OutputDevice);
		ReadBool(Cfg, "video", "fullscreen", S.bFullscreen, false);
		ReadBool(Cfg, "video", "vsync", S.bVsync, false);
		ReadInt(Cfg, "video", "graphics_quality", S.GraphicsQuality);
		S.GraphicsQuality = SettingsClampQuality(S.GraphicsQuality);
		ReadInt(Cfg, "video", "max_fps", S.MaxFps); // not clamped on load: applying uses max(cap, 0)
		ReadBool(Cfg, "video", "effects_enabled", S.bEffectsEnabled, false);
		ReadString(Cfg, "game", "language", S.Language);
		ReadInt(Cfg, "game", "last_slot", S.LastSlot);
		S.LastSlot = SettingsClampSlot(S.LastSlot);
		ReadInt(Cfg, "video", "resolution_index", S.ResolutionIndex);
		S.ResolutionIndex = SettingsClampResolution(S.ResolutionIndex);
		ReadBool(Cfg, "gameplay", "confirm_reawaken", S.bConfirmReawaken, false);
		ReadBool(Cfg, "gameplay", "tutorial_tips", S.bTutorialTips, true);
		ReadString(Cfg, "controls", "prompt_style", S.PromptStyle);
		if (!SettingsIsPromptStyle(S.PromptStyle))
		{
			S.PromptStyle = "auto";
		}
		ReadString(Cfg, "controls", "dock_side", S.DockSide);
		if (!SettingsIsDockSide(S.DockSide))
		{
			S.DockSide = "left";
		}
		ReadBool(Cfg, "controls", "show_prompts", S.bShowPrompts, true);
		ReadBool(Cfg, "controls", "swap_confirm_back", S.bSwapConfirmBack, true);
		ReadBool(Cfg, "controls", "swap_sticks", S.bSwapSticks, true);
		ReadBool(Cfg, "controls", "invert_x", S.bInvertX, true);
		ReadBool(Cfg, "controls", "invert_y", S.bInvertY, true);
		ReadNumber(Cfg, "controls", "cursor_speed", S.CursorSpeed);
		S.CursorSpeed = std::clamp(S.CursorSpeed, SettingsCursorSpeedMin, SettingsCursorSpeedMax);
		ReadNumber(Cfg, "controls", "stick_deadzone", S.StickDeadzone);
		S.StickDeadzone = std::clamp(S.StickDeadzone, SettingsDeadzoneMin, SettingsDeadzoneMax);
		// `bindings = {}`, then every key of [bindings] that holds a list, in the file's order.
		S.Bindings.clear();
		const auto Bindings = Cfg.find("bindings");
		if (Bindings != Cfg.end())
		{
			for (const std::string& Action : Bindings->second.Order)
			{
				const Value& V = Bindings->second.Values.at(Action);
				if (V.K == Kind::Array)
				{
					S.Bindings.push_back({ Action, V.Items });
				}
			}
		}
		return true;
	}

	std::string SettingsWrite(const PcSettings& S)
	{
		// save_settings()'s order: a section is placed where its first key was set (resolution_index joins [video] last).
		auto B = [](bool V) { return std::string(V ? "true" : "false"); };
		auto I = [](int64_t V) { return std::to_string(V); };
		std::string Out;
		Out += "[audio]\n\n";
		Out += "master_volume=" + Num(S.MasterVolume) + "\n";
		Out += "music_volume=" + Num(S.MusicVolume) + "\n";
		Out += "sfx_volume=" + Num(S.SfxVolume) + "\n";
		Out += "muted=" + B(S.bMuted) + "\n";
		Out += "output_device=" + Quote(S.OutputDevice) + "\n";
		Out += "\n[video]\n\n";
		Out += "fullscreen=" + B(S.bFullscreen) + "\n";
		Out += "vsync=" + B(S.bVsync) + "\n";
		Out += "graphics_quality=" + I(S.GraphicsQuality) + "\n";
		Out += "max_fps=" + I(S.MaxFps) + "\n";
		Out += "effects_enabled=" + B(S.bEffectsEnabled) + "\n";
		Out += "resolution_index=" + I(S.ResolutionIndex) + "\n";
		Out += "\n[game]\n\n";
		Out += "language=" + Quote(S.Language) + "\n";
		Out += "last_slot=" + I(S.LastSlot) + "\n";
		Out += "\n[gameplay]\n\n";
		Out += "confirm_reawaken=" + B(S.bConfirmReawaken) + "\n";
		Out += "tutorial_tips=" + B(S.bTutorialTips) + "\n";
		Out += "\n[controls]\n\n";
		Out += "prompt_style=" + Quote(S.PromptStyle) + "\n";
		Out += "dock_side=" + Quote(S.DockSide) + "\n";
		Out += "show_prompts=" + B(S.bShowPrompts) + "\n";
		Out += "swap_confirm_back=" + B(S.bSwapConfirmBack) + "\n";
		Out += "swap_sticks=" + B(S.bSwapSticks) + "\n";
		Out += "invert_x=" + B(S.bInvertX) + "\n";
		Out += "invert_y=" + B(S.bInvertY) + "\n";
		Out += "cursor_speed=" + Num(S.CursorSpeed) + "\n";
		Out += "stick_deadzone=" + Num(S.StickDeadzone) + "\n";
		if (!S.Bindings.empty())
		{
			// VariantWriter: an Array is [a, b]; a Dictionary {} or {\n"key": value,\n...\n}, its keys SORTED.
			auto Scalar = [](const PcCfgValue& V)
			{
				switch (V.Kind)
				{
				case PcCfgKind::Int: return std::to_string(V.I);
				case PcCfgKind::Float: return Num(V.F);
				case PcCfgKind::String: return Quote(V.S);
				case PcCfgKind::Bool: return std::string(V.B ? "true" : "false");
				}
				return std::string();
			};
			Out += "\n[bindings]\n\n";
			for (const PcSavedAction& A : S.Bindings)
			{
				std::string List = "[";
				for (size_t Ev = 0; Ev < A.Events.size(); ++Ev)
				{
					std::vector<std::pair<std::string, PcCfgValue>> Fields = A.Events[Ev].Fields;
					std::stable_sort(Fields.begin(), Fields.end(),
						[](const std::pair<std::string, PcCfgValue>& X, const std::pair<std::string, PcCfgValue>& Y) { return X.first < Y.first; });
					std::string Dict = "{";
					for (size_t F = 0; F < Fields.size(); ++F)
					{
						Dict += (F == 0 ? "\n" : ",\n") + Quote(Fields[F].first) + ": " + Scalar(Fields[F].second);
					}
					Dict += Fields.empty() ? "}" : "\n}";
					List += (Ev == 0 ? "" : ", ") + Dict;
				}
				Out += A.Action + "=" + List + "]\n";
			}
		}
		return Out;
	}

	double EffectBudget(const PcSettings& S)
	{
		if (!S.bEffectsEnabled)
		{
			return 0.0;
		}
		switch (S.GraphicsQuality)
		{
		case 0: return 0.35;
		case 1: return 0.70;
		default: return 1.0;
		}
	}

	PcIntSize FitToScreen(PcIntSize Target, PcIntSize Usable, PcIntSize Decoration)
	{
		if (Usable.X <= 0 || Usable.Y <= 0)
		{
			return Target; // no screen to measure against: leave it alone
		}
		const PcIntSize Min = SettingsMinWindow();
		const PcIntSize Room{ Usable.X - std::max<int64_t>(Decoration.X, 0), Usable.Y - std::max<int64_t>(Decoration.Y, 0) };
		return { std::clamp<int64_t>(Target.X, Min.X, std::max(Room.X, Min.X)), std::clamp<int64_t>(Target.Y, Min.Y, std::max(Room.Y, Min.Y)) };
	}

	PcIntSize CentredPosition(PcIntSize ScreenPos, PcIntSize ScreenSize, PcIntSize Size)
	{
		// Vector2i division truncates toward zero, as C++ does.
		const PcIntSize At{ ScreenPos.X + (ScreenSize.X - Size.X) / 2, ScreenPos.Y + (ScreenSize.Y - Size.Y) / 2 };
		return { std::max(At.X, ScreenPos.X), std::max(At.Y, ScreenPos.Y) };
	}

	// --- the dialog --------------------------------------------------------------------------------------------------

	double SettingsSliderStep(PcSettingsSlider Which)
	{
		return Which == PcSettingsSlider::Master ? 0.05 : 0.01;
	}

	double SettingsSliderSnap(double Value, double Step)
	{
		if (Step > 0.0)
		{
			// The k-th multiple of the step as k / (1 / step): the nearest double to the decimal (k / 20, k / 100). Measured
			// against Godot's sliders (fixture `settings`): every one of 4,912 moves; k * step was a bit off on 700 of them.
			// k is floor(x + 0.5), not round(x): a drag just below 0 gives +0 in Godot, where round() gives -0.
			Value = std::floor(Value / Step + 0.5) / (1.0 / Step);
		}
		return std::clamp(Value, 0.0, 1.0);
	}

	int64_t SettingsPercent(double Value)
	{
		return static_cast<int64_t>(std::round(Value * 100.0));
	}

	namespace
	{
		double& Stored(PcSettings& S, PcSettingsSlider Which)
		{
			return Which == PcSettingsSlider::Master ? S.MasterVolume : Which == PcSettingsSlider::Music ? S.MusicVolume : S.SfxVolume;
		}

		double StoredValue(const PcSettings& S, PcSettingsSlider Which)
		{
			return Which == PcSettingsSlider::Master ? S.MasterVolume : Which == PcSettingsSlider::Music ? S.MusicVolume : S.SfxVolume;
		}
	}

	void SettingsDialogSync(PcSettingsDialog& D, const PcSettings& S)
	{
		for (int I = 0; I < 3; ++I)
		{
			const PcSettingsSlider W = static_cast<PcSettingsSlider>(I);
			const double V = StoredValue(S, W);
			if (std::isfinite(V))
			{
				D.Slider[I] = SettingsSliderSnap(V, SettingsSliderStep(W));
			}
			D.Percent[I] = SettingsPercent(V);
		}
	}

	bool SettingsDialogMoveSlider(PcSettingsDialog& D, PcSettings& S, PcSettingsSlider Which, double Value)
	{
		if (!std::isfinite(Value))
		{
			return false;
		}
		const int I = static_cast<int>(Which);
		const double Snapped = SettingsSliderSnap(Value, SettingsSliderStep(Which));
		if (Snapped == D.Slider[I])
		{
			return false;
		}
		D.Slider[I] = Snapped;
		D.Percent[I] = SettingsPercent(Snapped);
		Stored(S, Which) = SettingsClampVolume(Snapped);
		return true;
	}

	void SettingsDialogStepSection(PcSettingsDialog& D, int64_t Direction)
	{
		const int64_t At = static_cast<int64_t>(D.Section) + Direction;
		D.Section = static_cast<PcSettingsSection>(((At % SettingsSectionCount) + SettingsSectionCount) % SettingsSectionCount);
	}

	int64_t SettingsFpsSelected(const PcSettings& S)
	{
		for (int64_t I = 0; I < SettingsFpsOptionCount; ++I)
		{
			if (FpsOptions[I] == S.MaxFps)
			{
				return I;
			}
		}
		return 0;
	}

	int64_t SettingsPromptsSelected(const PcSettings& S)
	{
		for (int64_t I = 0; I < SettingsPromptStyleCount; ++I)
		{
			if (S.PromptStyle == PromptStyles[I])
			{
				return I;
			}
		}
		return 0;
	}

	int64_t SettingsDockSelected(const PcSettings& S)
	{
		return S.DockSide == DockSides[1] ? 1 : 0;
	}

	PcSettingsOutputList SettingsOutputList(const std::vector<std::string>& SystemDevices, const PcSettings& S)
	{
		PcSettingsOutputList Out;
		Out.Devices = SystemDevices;
		if (std::find(Out.Devices.begin(), Out.Devices.end(), S.OutputDevice) == Out.Devices.end())
		{
			Out.Devices.push_back(S.OutputDevice);
		}
		for (size_t I = 0; I < Out.Devices.size(); ++I)
		{
			if (Out.Devices[I] == S.OutputDevice)
			{
				Out.Selected = static_cast<int64_t>(I);
			}
		}
		return Out;
	}

	std::string SettingsOutputWant(const std::vector<std::string>& SystemDevices, const PcSettings& S)
	{
		const bool bListed = std::find(SystemDevices.begin(), SystemDevices.end(), S.OutputDevice) != SystemDevices.end();
		return bListed ? S.OutputDevice : std::string("Default");
	}
}
