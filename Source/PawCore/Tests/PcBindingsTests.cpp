#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcBindings.h"
#include "PcCsv.h"
#include "PcSettings.h"

#include <charconv>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Remapping against the REAL input_bindings.gd (fixture `input_bindings`): random rebinds (swaps, refusals, nothing changed),
// row resets, resets of everything and reloads through the saved diff, compared after each op (the result, the row swapped
// with, the diff as Godot saves it, the event every cell shows); apply() with random switches (every live event, the steer
// dead zone); binding_from() on raw events; and settings.cfg's [bindings] written and read by Godot's ConfigFile, hand edits
// included. The defaults are Data/InputMap.csv's, in Godot's InputMap order.

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	std::vector<std::string> Split(const std::string& S, char Sep)
	{
		std::vector<std::string> Out;
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find(Sep, Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	std::string Unescape(const std::string& In)
	{
		std::string Out;
		for (size_t I = 0; I < In.size(); ++I)
		{
			if (In[I] == '\\' && I + 1 < In.size())
			{
				const char C = In[++I];
				Out.push_back(C == 'n' ? '\n' : C == 't' ? '\t' : C);
			}
			else
			{
				Out.push_back(In[I]);
			}
		}
		return Out;
	}

	// The fixture's tables: comment lines, a header, rows.
	struct Table
	{
		std::vector<std::string> Header;
		std::vector<std::map<std::string, std::string>> Rows;
	};

	Table ReadTable(const std::string& Path)
	{
		Table T;
		std::string Text;
		REQUIRE(Pc::ReadTextFile(Path, Text));
		std::istringstream In(Text);
		std::string Line;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = Split(Line, '\t');
			if (T.Header.empty())
			{
				T.Header = Cells;
				continue;
			}
			REQUIRE(Cells.size() == T.Header.size());
			std::map<std::string, std::string> Row;
			for (size_t I = 0; I < Cells.size(); ++I)
			{
				Row[T.Header[I]] = Cells[I];
			}
			T.Rows.push_back(Row);
		}
		return T;
	}

	std::string Ev(const Pc::PcInputEvent& E)
	{
		switch (E.Kind)
		{
		case Pc::PcInputKind::Key: return "k" + std::to_string(E.Code);
		case Pc::PcInputKind::Mouse: return "m" + std::to_string(E.Code);
		case Pc::PcInputKind::JoyButton: return "b" + std::to_string(E.Code);
		case Pc::PcInputKind::JoyAxis: return "a" + std::to_string(E.Code) + (E.Sign > 0 ? "+" : E.Sign < 0 ? "-" : "0");
		case Pc::PcInputKind::None: break;
		}
		return "-";
	}

	Pc::PcInputEvent ParseEv(const std::string& S)
	{
		Pc::PcInputEvent E;
		if (S.empty() || S == "-")
		{
			return E;
		}
		std::string Digits = S.substr(1);
		switch (S[0])
		{
		case 'k': E.Kind = Pc::PcInputKind::Key; break;
		case 'm': E.Kind = Pc::PcInputKind::Mouse; break;
		case 'b': E.Kind = Pc::PcInputKind::JoyButton; break;
		case 'a':
			E.Kind = Pc::PcInputKind::JoyAxis;
			E.Sign = Digits.back() == '+' ? 1 : Digits.back() == '-' ? -1 : 0;
			Digits.pop_back();
			break;
		default: FAIL("unknown event " << S);
		}
		E.Code = std::stoll(Digits);
		return E;
	}

	std::string EventsText(const std::vector<Pc::PcInputEvent>& L)
	{
		std::string Out;
		for (size_t I = 0; I < L.size(); ++I)
		{
			Out += (I ? "," : "") + Ev(L[I]);
		}
		return Out;
	}

	// GDScript's str() of a float: "74.0", "2.5", "-1.0".
	std::string FloatText(double V)
	{
		char Buf[64];
		const auto R = std::to_chars(Buf, Buf + sizeof(Buf), V);
		std::string S(Buf, R.ptr);
		if (S.find_first_of(".e") == std::string::npos)
		{
			S += ".0";
		}
		return S;
	}

	std::string ValText(const Pc::PcCfgValue& V)
	{
		switch (V.Kind)
		{
		case Pc::PcCfgKind::Int: return "i" + std::to_string(V.I);
		case Pc::PcCfgKind::Float: return "f" + FloatText(V.F);
		case Pc::PcCfgKind::String: return "s" + V.S;
		case Pc::PcCfgKind::Bool: return V.B ? "b1" : "b0";
		}
		return "?";
	}

	std::string SavedText(const Pc::PcSavedBindings& Saved)
	{
		std::string Out;
		for (size_t A = 0; A < Saved.size(); ++A)
		{
			std::string List;
			for (size_t I = 0; I < Saved[A].Events.size(); ++I)
			{
				std::string Dict;
				for (size_t F = 0; F < Saved[A].Events[I].Fields.size(); ++F)
				{
					Dict += (F ? "|" : "") + Saved[A].Events[I].Fields[F].first + ":" + ValText(Saved[A].Events[I].Fields[F].second);
				}
				List += (I ? "," : "") + std::string("{") + Dict + "}";
			}
			Out += (A ? ";" : "") + Saved[A].Action + "=[" + List + "]";
		}
		return Out;
	}

	// The fixture's saved text back into bindings: action=[{key:<i|f|s|b>value|...},...];...
	Pc::PcSavedBindings ParseSaved(const std::string& Text)
	{
		Pc::PcSavedBindings Out;
		if (Text.empty())
		{
			return Out;
		}
		for (const std::string& Part : Split(Text, ';'))
		{
			const size_t Eq = Part.find('=');
			REQUIRE(Eq != std::string::npos);
			Pc::PcSavedAction A;
			A.Action = Part.substr(0, Eq);
			const std::string List = Part.substr(Eq + 2, Part.size() - Eq - 3); // inside [ ]
			size_t At = 0;
			while (At < List.size())
			{
				REQUIRE(List[At] == '{');
				const size_t Close = List.find('}', At);
				const std::string Body = List.substr(At + 1, Close - At - 1);
				Pc::PcSavedEvent E;
				if (!Body.empty())
				{
					for (const std::string& Field : Split(Body, '|'))
					{
						const size_t Colon = Field.find(':');
						const std::string Key = Field.substr(0, Colon);
						const char Kind = Field[Colon + 1];
						const std::string V = Field.substr(Colon + 2);
						Pc::PcCfgValue Value;
						switch (Kind)
						{
						case 'i': Value.Kind = Pc::PcCfgKind::Int; Value.I = std::stoll(V); break;
						case 'f': Value.Kind = Pc::PcCfgKind::Float; Value.F = std::stod(V); break;
						case 's': Value.Kind = Pc::PcCfgKind::String; Value.S = V; break;
						case 'b': Value.Kind = Pc::PcCfgKind::Bool; Value.B = V == "1"; break;
						default: FAIL("unknown value " << Field);
						}
						E.Fields.emplace_back(Key, Value);
					}
				}
				A.Events.push_back(E);
				At = Close + 1;
				if (At < List.size() && List[At] == ',')
				{
					++At;
				}
			}
			Out.push_back(A);
		}
		return Out;
	}

	std::string StateText(const Pc::PcInputState& State, const Pc::PcInputState& Order)
	{
		std::string Out;
		for (size_t I = 0; I < Order.size(); ++I)
		{
			const std::vector<Pc::PcInputEvent>* L = Pc::BindingFind(State, Order[I].Action);
			Out += (I ? ";" : "") + Order[I].Action + "=" + (L ? EventsText(*L) : std::string());
		}
		return Out;
	}

	std::string CellsText(const Pc::PcInputState& State)
	{
		std::string Out;
		for (int R = 0; R < Pc::BindingRowCount; ++R)
		{
			for (const Pc::PcBindDevice D : { Pc::PcBindDevice::Keyboard, Pc::PcBindDevice::Pad })
			{
				Out += (Out.empty() ? "" : ",") + Ev(Pc::BindingEventFor(Pc::BindingRowId(R), D, State));
			}
		}
		return Out;
	}

	// The defaults: Data/InputMap.csv, in Godot's InputMap order.
	Pc::PcInputState Defaults()
	{
		std::string Text;
		REQUIRE(Pc::ReadTextFile(DataDir() + "/InputMap.csv", Text));
		Pc::CsvTable Csv;
		std::string Error;
		REQUIRE(Pc::ParseCsv(Text, Csv, Error));
		const int CAction = Csv.Column("action"), CEvent = Csv.Column("godot_event"), CCode = Csv.Column("godot_code"),
			CSign = Csv.Column("godot_axis_sign");
		Pc::PcInputState State;
		for (const std::vector<std::string>& Row : Csv.Rows)
		{
			int64_t Code = 0, Sign = 0;
			REQUIRE(Pc::ParseInt(Row[static_cast<size_t>(CCode)], Code));
			if (!Row[static_cast<size_t>(CSign)].empty())
			{
				REQUIRE(Pc::ParseInt(Row[static_cast<size_t>(CSign)], Sign));
			}
			const Pc::PcInputEvent E = Pc::BindingEventOf(Row[static_cast<size_t>(CEvent)], Code, Sign);
			REQUIRE(!E.IsNone());
			Pc::BindingAt(State, Row[static_cast<size_t>(CAction)]).push_back(E);
		}
		return Pc::BindingGodotOrder(State);
	}

	struct FResult
	{
		int Rows = 0;
		std::map<std::string, int> Ops;
		int Applies = 0;
		int SwapSticks = 0;
		int SwapConfirm = 0;
		int FromCarry = 0;
		int Trimmed = 0;       // a start with a row bare on one device (a hand edit)
		int AddedToBare = 0;   // a rebind that ADDED a key to a row bare on that device
	};

	FResult RunOps(const std::string& Path)
	{
		const Pc::PcInputState Def = Defaults();
		const Table T = ReadTable(Path);
		FResult R;
		Pc::PcInputState State;
		Pc::PcSavedBindings Carry;
		for (const std::map<std::string, std::string>& V : T.Rows)
		{
			const std::string& Op = V.at("op");
			INFO("row " << V.at("row") << " op " << Op << " arg " << V.at("arg"));
			if (Op == "start")
			{
				++R.Rows;
				// The row began from its saved diff (the last row's, the defaults, or a hand edit of either).
				const Pc::PcSavedBindings Saved = ParseSaved(V.at("saved"));
				State = Pc::BindingWorkingState(Saved, Def);
				R.FromCarry += !Carry.empty() && Saved == Carry ? 1 : 0;
				const std::vector<std::string> Cells = Split(V.at("cells"), ',');
				for (size_t I = 0; I < Cells.size(); ++I)
				{
					if (Cells[I] == "-" && I != 16) // 16: Re-centre on the keyboard, bare by design
					{
						++R.Trimmed;
						break;
					}
				}
			}
			else if (Op == "rebind")
			{
				const size_t Slash = V.at("arg").find('/');
				const std::string Logical = V.at("arg").substr(0, Slash);
				const Pc::PcInputEvent Event = ParseEv(V.at("arg").substr(Slash + 1));
				const bool bBare = Pc::BindingEventFor(Logical, Pc::BindingDeviceOf(Event), State).IsNone();
				const Pc::PcBindOutcome Out = Pc::BindingRebind(State, Logical, Event);
				CHECK(std::to_string(static_cast<int>(Out.Result)) == V.at("result"));
				CHECK((Out.Other.empty() ? "-" : Out.Other) == V.at("other"));
				++R.Ops["rebind_" + V.at("result")];
				R.AddedToBare += bBare && (Out.Result == Pc::PcBindResult::Bound || Out.Result == Pc::PcBindResult::Swapped) ? 1 : 0;
			}
			else if (Op == "reset_row")
			{
				Pc::BindingResetRow(State, Def, V.at("arg"));
				++R.Ops[Op];
			}
			else if (Op == "reset_all")
			{
				State = Pc::BindingWorkingState({}, Def);
				++R.Ops[Op];
			}
			else if (Op == "reload")
			{
				State = Pc::BindingWorkingState(Pc::BindingDiff(State, Def), Def);
				++R.Ops[Op];
			}
			else if (Op == "apply")
			{
				Carry = Pc::BindingDiff(State, Def);
				CHECK(SavedText(Carry) == V.at("saved"));
				const bool bSticks = V.at("swap_sticks") == "1";
				const bool bConfirm = V.at("swap_confirm") == "1";
				const Pc::PcInputState Live = Pc::BindingApply(Carry, bSticks, bConfirm, Def);
				CHECK(StateText(Live, Def) == V.at("live"));
				CHECK(Pc::BindingSteerDeadZone(std::strtod(V.at("deadzone").c_str(), nullptr)) == std::strtod(V.at("steer_dz").c_str(), nullptr));
				++R.Applies;
				R.SwapSticks += bSticks ? 1 : 0;
				R.SwapConfirm += bConfirm ? 1 : 0;
				continue;
			}
			else
			{
				FAIL("unknown op " << Op);
			}
			CHECK(SavedText(Pc::BindingDiff(State, Def)) == V.at("saved"));
			CHECK(CellsText(State) == V.at("cells"));
		}
		return R;
	}
}

TEST_CASE("Bindings: the defaults are Godot's InputMap, in its order")
{
	const Pc::PcInputState Def = Defaults();
	const Table T = ReadTable(DataDir() + "/../Source/PawCore/Tests/Data/input_bindings_defaults_godot.tsv");
	REQUIRE(T.Rows.size() == Def.size());
	for (size_t I = 0; I < Def.size(); ++I)
	{
		CHECK(Def[I].Action == T.Rows[I].at("action"));
		CHECK(EventsText(Def[I].Events) == T.Rows[I].at("events"));
	}
}

TEST_CASE("Bindings: every rebind, reset, reload and apply matches the real input_bindings.gd")
{
	FResult R = RunOps(DataDir() + "/../Source/PawCore/Tests/Data/input_bindings_godot.tsv");
	MESSAGE("rows " << R.Rows << ", from the last row's bindings " << R.FromCarry << ", ops: bound " << R.Ops["rebind_0"] << ", swapped "
		<< R.Ops["rebind_1"] << ", refused (system) " << R.Ops["rebind_2"] << ", refused (taken) " << R.Ops["rebind_3"]
		<< ", unchanged " << R.Ops["rebind_4"] << ", row resets " << R.Ops["reset_row"] << ", resets " << R.Ops["reset_all"]
		<< ", reloads " << R.Ops["reload"] << "; applies " << R.Applies << " (sticks swapped " << R.SwapSticks << ", confirm swapped "
		<< R.SwapConfirm << "); starts with a bare row " << R.Trimmed << ", keys added to a bare row " << R.AddedToBare);
	CHECK(R.Rows == 300);
	CHECK(R.FromCarry > 100);
	CHECK(R.Trimmed > 40);
	CHECK(R.AddedToBare > 5);
	for (const char* Op : { "rebind_0", "rebind_1", "rebind_2", "rebind_3", "reset_row", "reset_all", "reload" })
	{
		CHECK(R.Ops[Op] > 40);
	}
	CHECK(R.Ops["rebind_4"] > 10);
	CHECK(R.SwapSticks > 60);
	CHECK(R.SwapConfirm > 60);
}

TEST_CASE("Bindings: the long suite runs all 3,000 rows" * doctest::test_suite("long") * doctest::skip())
{
	const FResult R = RunOps(DataDir() + "/../build/input_bindings_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Bindings: binding_from turns a raw event into a binding as Godot does")
{
	const Table T = ReadTable(DataDir() + "/../Source/PawCore/Tests/Data/input_binding_from_godot.tsv");
	std::map<std::string, int> Kept;
	for (const std::map<std::string, std::string>& V : T.Rows)
	{
		Pc::PcCapturedEvent E;
		const std::string& Kind = V.at("kind");
		E.Kind = Kind == "key" ? Pc::PcInputKind::Key : Kind == "mouse" ? Pc::PcInputKind::Mouse
			: Kind == "joy_button" ? Pc::PcInputKind::JoyButton : Pc::PcInputKind::JoyAxis;
		E.Code = std::stoll(V.at("code"));
		E.KeyCode = std::stoll(V.at("keycode"));
		E.bPressed = V.at("pressed") == "1";
		E.bEcho = V.at("echo") == "1";
		E.AxisValue = V.at("value") == "-" ? 0.0 : std::strtod(V.at("value").c_str(), nullptr);
		INFO(Kind << " " << V.at("code") << " pressed " << V.at("pressed") << " echo " << V.at("echo") << " value " << V.at("value"));
		CHECK(Ev(Pc::BindingFrom(E)) == V.at("out"));
		++Kept[Kind + (V.at("out") == "-" ? "_none" : "_kept")];
	}
	CHECK(T.Rows.size() == 3000);
	for (const char* K : { "key_kept", "key_none", "mouse_kept", "mouse_none", "joy_button_kept", "joy_button_none", "joy_axis_kept", "joy_axis_none" })
	{
		CHECK(Kept[K] > 50);
	}
}

TEST_CASE("Bindings: settings.cfg's [bindings] reads and writes as Godot's ConfigFile")
{
	const Pc::PcInputState Def = Defaults();
	const Table T = ReadTable(DataDir() + "/../Source/PawCore/Tests/Data/input_bindings_cfg_godot.tsv");
	int WithBindings = 0, Dropped = 0, Odd = 0;
	for (const std::map<std::string, std::string>& V : T.Rows)
	{
		INFO("row " << V.at("row"));
		const std::string Written = Unescape(V.at("written"));
		Pc::PcSettings S;
		S.Bindings.push_back({ "stale", {} }); // load_settings() starts from `bindings = {}`
		REQUIRE(Pc::SettingsLoad(Written, S));
		CHECK(SavedText(S.Bindings) == V.at("loaded"));
		CHECK(StateText(Pc::BindingWorkingState(S.Bindings, Def), Def) == V.at("work"));
		const std::string Rewritten = Unescape(V.at("rewritten"));
		CHECK(Pc::SettingsWrite(S) == Rewritten);
		// What the port writes reads back the same.
		Pc::PcSettings Again;
		REQUIRE(Pc::SettingsLoad(Pc::SettingsWrite(S), Again));
		CHECK(SavedText(Again.Bindings) == SavedText(S.Bindings));
		WithBindings += S.Bindings.empty() ? 0 : 1;
		Dropped += Written != Rewritten ? 1 : 0;
		Odd += V.at("loaded").find("|note:") != std::string::npos || V.at("loaded").find(":f") != std::string::npos ? 1 : 0;
	}
	MESSAGE("files " << T.Rows.size() << ", with bindings " << WithBindings << ", a value dropped on load " << Dropped
		<< ", hand edits kept " << Odd);
	CHECK(T.Rows.size() == 200);
	CHECK(WithBindings > 150);
	CHECK(Dropped > 3);
	CHECK(Odd > 10);
}

TEST_CASE("Bindings: the rules' own corners")
{
	const Pc::PcInputState Def = Defaults();
	Pc::PcInputState State = Def;
	auto Key = [](int64_t Code) { return Pc::PcInputEvent{ Pc::PcInputKind::Key, Code, 0 }; };
	// Esc, Start and B are system keys; Pause is locked on both devices; Re-centre on the keyboard only.
	CHECK(Pc::BindingRebind(State, "press", Key(Pc::GodotInput::KeyEscape)).Result == Pc::PcBindResult::RefusedSystem);
	CHECK(Pc::BindingRebind(State, "pause", Key('I')).Result == Pc::PcBindResult::RefusedSystem);
	CHECK(Pc::BindingRebind(State, "recentre", Key('I')).Result == Pc::PcBindResult::RefusedSystem);
	CHECK(Pc::BindingRebind(State, "recentre", { Pc::PcInputKind::JoyButton, 7, 0 }).Result == Pc::PcBindResult::Bound);
	// The number keys belong to tab_1..6: taken.
	CHECK(Pc::BindingRebind(State, "press", Key('1')).Result == Pc::PcBindResult::RefusedTaken);
	// Up onto S (Down's) swaps: Up gets S everywhere, Down gets W everywhere.
	const Pc::PcBindOutcome Out = Pc::BindingRebind(State, "up", Key('S'));
	CHECK(Out.Result == Pc::PcBindResult::Swapped);
	CHECK(Out.Other == "down");
	CHECK(EventsText(*Pc::BindingFind(State, "ui_up")) == "k4194320,k83,b11,a1-");
	CHECK(EventsText(*Pc::BindingFind(State, "menu_down")) == "k87,k4194322,b12,a1+");
	// Resetting Up resets Down too (they shared a key after the swap).
	Pc::BindingResetRow(State, Def, "up");
	CHECK(Pc::BindingDiff(State, Def).size() == 1); // only recentre's pad button stays
	CHECK(Pc::BindingDiff(State, Def)[0].Action == "recentre");
}

#endif
