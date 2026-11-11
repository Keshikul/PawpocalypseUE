#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcBindings.h"
#include "PcControls.h"
#include "PcCsv.h"
#include "PcSettings.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The Controls page against the REAL controls_panel.gd in the real settings dialog (fixture `controls`): every row starts
// from a recorded state and makes 12 moves a player makes (a cell pressed, events while it listens, the clock, Reset, Reset
// all, a switch, a slider, the prompt style, a direct bind); after each, the status line, which cell listens, every cell's
// text, disabled state and the family its prompt draws, the Resets, the switches, the sliders and their lines, the
// controller column's family and the bindings as saved are compared. Whether a cell's prompt shows anything is the
// glyphs' (the engine test); here the cell's text is checked against it.

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

	// var_to_bytes(float) as hex: a 4-byte header (bit 16: a 64-bit double follows, else a 32-bit float).
	double VariantHex(const std::string& Hex)
	{
		std::vector<unsigned char> B;
		for (size_t I = 0; I + 1 < Hex.size(); I += 2)
		{
			B.push_back(static_cast<unsigned char>(std::strtoul(Hex.substr(I, 2).c_str(), nullptr, 16)));
		}
		REQUIRE(B.size() >= 8);
		const uint32_t Header = B[0] | (B[1] << 8) | (B[2] << 16) | (static_cast<uint32_t>(B[3]) << 24);
		if (Header & (1u << 16))
		{
			REQUIRE(B.size() >= 12);
			double D = 0.0;
			std::memcpy(&D, B.data() + 4, 8);
			return D;
		}
		// A 32-bit float's bits, read by arithmetic (PawCore holds no float): normal and zero values.
		const uint32_t Bits = B[4] | (B[5] << 8) | (B[6] << 16) | (static_cast<uint32_t>(B[7]) << 24);
		const int Exp = static_cast<int>((Bits >> 23) & 0xFF);
		const double Sign = (Bits >> 31) ? -1.0 : 1.0;
		const double Mantissa = static_cast<double>(Bits & 0x7FFFFF);
		if (Exp == 0)
		{
			return Sign * std::ldexp(Mantissa, -149);
		}
		return Sign * std::ldexp(Mantissa + 8388608.0, Exp - 150);
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

	// The fixture's saved text (action=ev,ev;...) as the settings hold it.
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
			Pc::PcSavedAction A;
			A.Action = Part.substr(0, Eq);
			for (const std::string& E : Split(Part.substr(Eq + 1), ','))
			{
				A.Events.push_back(Pc::BindingSerialize(ParseEv(E)));
			}
			Out.push_back(A);
		}
		return Out;
	}

	std::string SavedText(const Pc::PcSavedBindings& Saved)
	{
		std::string Out;
		for (size_t A = 0; A < Saved.size(); ++A)
		{
			std::string List;
			for (size_t I = 0; I < Saved[A].Events.size(); ++I)
			{
				List += (I ? "," : "") + Ev(Pc::BindingDeserialize(Saved[A].Events[I]));
			}
			Out += (A ? ";" : "") + Saved[A].Action + "=" + List;
		}
		return Out;
	}

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
			Pc::BindingAt(State, Row[static_cast<size_t>(CAction)]).push_back(Pc::BindingEventOf(Row[static_cast<size_t>(CEvent)], Code, Sign));
		}
		return Pc::BindingGodotOrder(State);
	}

	// The page's English words (ST_UI in the game).
	std::string Label(const std::string& Row)
	{
		const int I = Pc::BindingRowIndex(Row);
		REQUIRE(I >= 0);
		return Pc::BindingRowLabel(I);
	}

	std::string LineText(const Pc::PcControlsState& C)
	{
		switch (C.Line)
		{
		case Pc::PcControlsLine::None: return "";
		case Pc::PcControlsLine::Listening: return "Press a key or button for " + Label(C.LineRow) + ". Esc or B cancels.";
		case Pc::PcControlsLine::NothingPressed: return "Nothing pressed, nothing changed.";
		case Pc::PcControlsLine::Cancelled: return "Cancelled.";
		case Pc::PcControlsLine::Done: return "Done.";
		case Pc::PcControlsLine::Swapped: return "Swapped with " + Label(C.LineRow) + ".";
		case Pc::PcControlsLine::RefusedSystem: return "Esc, Start and B always open the menu and go back.";
		case Pc::PcControlsLine::Taken: return "That one is taken.";
		case Pc::PcControlsLine::Reset: return "Reset.";
		case Pc::PcControlsLine::ResetAll: return "Every control is back to its default.";
		}
		return "?";
	}

	const char* const CheckNames[] = { "show_prompts", "tutorial_tips", "swap_confirm_back", "swap_sticks", "invert_x", "invert_y" };

	bool LiveHas(const Pc::PcInputState& Live, const char* Action, const Pc::PcInputEvent& E)
	{
		if (const std::vector<Pc::PcInputEvent>* L = Pc::BindingFind(Live, Action))
		{
			for (const Pc::PcInputEvent& X : *L)
			{
				if (Pc::BindingSameEvent(X, E))
				{
					return true;
				}
			}
		}
		return false;
	}

	struct FResult
	{
		int Rows = 0;
		std::map<std::string, int> Ops;
		std::map<std::string, int> Lines;
		int PadColumnPinned = 0, PadColumnDetected = 0, PadColumnDefault = 0;
		int SliderMovesStill = 0;
		int StaleColumn = 0; // the controller column drawn from the last sync, not the pad detected since (0 since F37 is fixed)
		int NewPad = 0;      // moves in which the pad detected changed (F37's case: the page redraws now)
		int TimeoutAtZero = 0; // the clock landing exactly on 0 (Godot: <= 0 gives up)
	};

	FResult Run(const std::string& Path)
	{
		const Pc::PcInputState Def = Defaults();
		std::string Text;
		REQUIRE(Pc::ReadTextFile(Path, Text));
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		FResult R;
		Pc::PcSettings S;
		Pc::PcControlsState C;
		// The page redraws (sync) only when told: a control setting changed, the prompt family changed, a cell started or
		// stopped listening, and the pad detected changed (F37, a Godot fault the port does not copy, D16: Godot's page missed a
		// new pad while the prompts were pinned, and kept the old pad's buttons; the fixture is made with the fix).
		Pc::PcFamily SyncedColumn = Pc::PcFamily::Xbox;
		std::string LastFamily;
		std::string LastDetected;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> Cells = Split(Line, '\t');
			if (Header.empty())
			{
				Header = Cells;
				continue;
			}
			REQUIRE(Cells.size() == Header.size());
			std::map<std::string, std::string> V;
			for (size_t I = 0; I < Cells.size(); ++I)
			{
				V[Header[I]] = Cells[I];
			}
			const std::string& Op = V.at("op");
			const std::string& Arg = V.at("arg");
			INFO("row " << V.at("row") << " op " << Op << " arg " << Arg);
			++R.Ops[Op];
			const Pc::PcSettings Before = S;
			const Pc::PcControlsState CBefore = C;
			bool bSync = Op == "start" || Op == "listen" || Op == "reset_row" || Op == "reset_all" || Op == "bind";
			if (Op == "start")
			{
				++R.Rows;
				const std::vector<std::string> P = Split(Arg, '/');
				REQUIRE(P.size() == 7);
				S = Pc::PcSettings();
				S.Bindings = ParseSaved(P[0]);
				const std::vector<std::string> Flags = Split(P[1], ',');
				for (int I = 0; I < Pc::ControlsCheckCount; ++I)
				{
					Pc::ControlsSetCheck(S, static_cast<Pc::PcControlsCheck>(I), Flags[static_cast<size_t>(I)] == "1");
				}
				Pc::ControlsSetSlider(S, Pc::PcControlsSlider::CursorSpeed, VariantHex(P[2]));
				Pc::ControlsSetSlider(S, Pc::PcControlsSlider::StickDeadzone, VariantHex(P[3]));
				S.PromptStyle = P[4];
				C = Pc::PcControlsState();
			}
			else if (Op == "listen")
			{
				const std::vector<std::string> P = Split(Arg, '/');
				Pc::ControlsListen(C, P[0], P[1] == "pad" ? Pc::PcBindDevice::Pad : Pc::PcBindDevice::Keyboard);
			}
			else if (Op == "listen_disabled")
			{
				const std::vector<std::string> P = Split(Arg, '/');
				CHECK(Pc::BindingLocked(P[0], P[1] == "pad" ? Pc::PcBindDevice::Pad : Pc::PcBindDevice::Keyboard));
			}
			else if (Op == "event")
			{
				Pc::PcCapturedEvent E;
				bool bMotion = false;
				Pc::PcInputEvent Raw;
				if (Arg == "mm")
				{
					bMotion = true;
				}
				else if (Arg[0] == 'a')
				{
					const size_t Colon = Arg.find(':');
					E.Kind = Pc::PcInputKind::JoyAxis;
					E.Code = std::stoll(Arg.substr(1, Colon - 1));
					E.AxisValue = std::stod(Arg.substr(Colon + 1));
				}
				else
				{
					std::string Body = Arg;
					E.bPressed = Body.back() != 'u';
					E.bEcho = Body.back() == 'e';
					if (Body.back() == 'u' || Body.back() == 'e')
					{
						Body.pop_back();
					}
					Raw = ParseEv(Body);
					E.Kind = Raw.Kind;
					E.Code = Raw.Code;
				}
				// is_action_pressed("back") or ("ui_cancel"): a press (not an echo) of an event the LIVE map has there.
				const Pc::PcInputState Live = Pc::BindingApply(S.Bindings, S.bSwapSticks, S.bSwapConfirmBack, Def);
				const bool bBack = !bMotion && E.Kind != Pc::PcInputKind::JoyAxis && E.bPressed && !E.bEcho
					&& (LiveHas(Live, "back", Raw) || LiveHas(Live, "ui_cancel", Raw));
				const Pc::PcControlsInput Out = Pc::ControlsInput(C, S, Def, E, bMotion, bBack);
				++R.Ops["event_" + std::to_string(static_cast<int>(Out.Take))];
				bSync = bSync || Out.Take == Pc::PcControlsTake::Cancelled || Out.Take == Pc::PcControlsTake::Bound;
			}
			else if (Op == "tick")
			{
				const double Left = C.ListenLeft - std::strtod(Arg.c_str(), nullptr);
				bSync = Pc::ControlsTick(C, std::strtod(Arg.c_str(), nullptr));
				R.TimeoutAtZero += bSync && Left == 0.0 ? 1 : 0;
			}
			else if (Op == "reset_row")
			{
				Pc::ControlsResetRow(C, S, Def, Arg);
			}
			else if (Op == "reset_disabled")
			{
				CHECK_FALSE(Pc::ControlsResetEnabled(Arg));
			}
			else if (Op == "reset_all")
			{
				Pc::ControlsResetAll(C, S);
			}
			else if (Op == "check")
			{
				for (int I = 0; I < Pc::ControlsCheckCount; ++I)
				{
					if (Arg == CheckNames[I])
					{
						const Pc::PcControlsCheck Which = static_cast<Pc::PcControlsCheck>(I);
						Pc::ControlsSetCheck(S, Which, !Pc::ControlsCheckValue(S, Which));
					}
				}
			}
			else if (Op == "slider")
			{
				const size_t Eq = Arg.find('=');
				const Pc::PcControlsSlider Which = Arg.substr(0, Eq) == "cursor_speed" ? Pc::PcControlsSlider::CursorSpeed : Pc::PcControlsSlider::StickDeadzone;
				// The slider shows the stored value snapped; a move that lands where it is emits nothing.
				const double To = Pc::ControlsSliderSnap(Which, VariantHex(Arg.substr(Eq + 1)));
				if (To != Pc::ControlsSliderSnap(Which, Pc::ControlsSliderSetting(S, Which)))
				{
					Pc::ControlsSetSlider(S, Which, To);
				}
				else
				{
					++R.SliderMovesStill;
				}
			}
			else if (Op == "style")
			{
				S.PromptStyle = Arg;
			}
			else if (Op == "bind")
			{
				const size_t Slash = Arg.find('/');
				bool bSaved = false;
				const Pc::PcBindResult Result = Pc::ControlsBind(C, S, Def, Arg.substr(0, Slash), ParseEv(Arg.substr(Slash + 1)), bSaved);
				CHECK(std::to_string(static_cast<int>(Result)) == V.at("result"));
			}
			else if (Op != "bind_none")
			{
				FAIL("unknown op " << Op);
			}

			// controls_changed (a setting stored), prompt_style_changed, scheme_changed (the family moved): sync.
			bSync = bSync || !(S.Bindings == Before.Bindings) || S.PromptStyle != Before.PromptStyle || S.CursorSpeed != Before.CursorSpeed
				|| S.StickDeadzone != Before.StickDeadzone;
			for (int I = 0; I < Pc::ControlsCheckCount; ++I)
			{
				const Pc::PcControlsCheck Which = static_cast<Pc::PcControlsCheck>(I);
				bSync = bSync || Pc::ControlsCheckValue(S, Which) != Pc::ControlsCheckValue(Before, Which);
			}
			bSync = bSync || V.at("family") != LastFamily;
			LastFamily = V.at("family");
			R.NewPad += Op != "start" && V.at("detected") != LastDetected ? 1 : 0;
			bSync = bSync || V.at("detected") != LastDetected;
			LastDetected = V.at("detected");
			(void)CBefore;

			// --- what the page shows ---
			CHECK(LineText(C) == Unescape(V.at("status")));
			++R.Lines[std::to_string(static_cast<int>(C.Line))];
			CHECK((C.bListening ? C.ListenRow + "/" + (C.ListenDevice == Pc::PcBindDevice::Pad ? "pad" : "keyboard") : std::string("-")) == V.at("listening"));
			const Pc::PcFamily Family = static_cast<Pc::PcFamily>(std::stoi(V.at("family")));
			const Pc::PcFamily Detected = static_cast<Pc::PcFamily>(std::stoi(V.at("detected")));
			const Pc::PcFamily PadColumn = Pc::ControlsPadColumnFamily(S.PromptStyle, Family, Detected);
			CHECK(std::to_string(static_cast<int>(PadColumn)) == V.at("pad_column"));
			if (bSync)
			{
				SyncedColumn = PadColumn;
			}
			R.StaleColumn += SyncedColumn != PadColumn ? 1 : 0;
			R.PadColumnPinned += S.PromptStyle != "auto" && S.PromptStyle != "keyboard" ? 1 : 0;
			R.PadColumnDetected += (S.PromptStyle == "auto" || S.PromptStyle == "keyboard") && Detected != Pc::PcFamily::Keyboard ? 1 : 0;
			R.PadColumnDefault += PadColumn == Pc::PcFamily::Xbox && Detected == Pc::PcFamily::Keyboard && Family == Pc::PcFamily::Keyboard ? 1 : 0;
			const std::vector<std::string> CellTexts = Split(V.at("cells"), ';');
			REQUIRE(CellTexts.size() == 2 * Pc::BindingRowCount);
			for (int Row = 0; Row < Pc::BindingRowCount; ++Row)
			{
				for (int D = 0; D < 2; ++D)
				{
					const std::string Id = Pc::BindingRowId(Row);
					const Pc::PcBindDevice Device = D == 0 ? Pc::PcBindDevice::Keyboard : Pc::PcBindDevice::Pad;
					const std::vector<std::string> Cell = Split(CellTexts[static_cast<size_t>(2 * Row + D)], '|');
					REQUIRE(Cell.size() == 4);
					INFO("cell " << Id << "/" << D);
					const std::string Want = Pc::ControlsCellListening(C, Id, Device) ? "Press\xE2\x80\xA6" : (Cell[3] == "1" ? "" : "\xE2\x80\x94");
					CHECK(Unescape(Cell[0]) == Want);
					CHECK((Cell[1] == "1") == Pc::BindingLocked(Id, Device));
					CHECK(std::stoi(Cell[2]) == (D == 0 ? 0 : static_cast<int>(SyncedColumn)));
				}
			}
			const std::vector<std::string> Resets = Split(V.at("resets"), ',');
			for (int Row = 0; Row < Pc::BindingRowCount; ++Row)
			{
				CHECK((Resets[static_cast<size_t>(Row)] == "1") == !Pc::ControlsResetEnabled(Pc::BindingRowId(Row)));
			}
			const std::vector<std::string> Checks = Split(V.at("checks"), ',');
			for (int I = 0; I < Pc::ControlsCheckCount; ++I)
			{
				CHECK((Checks[static_cast<size_t>(I)] == "1") == Pc::ControlsCheckValue(S, static_cast<Pc::PcControlsCheck>(I)));
			}
			CHECK(std::strtod(V.at("cursor_speed").c_str(), nullptr) == S.CursorSpeed);
			CHECK(std::strtod(V.at("deadzone").c_str(), nullptr) == S.StickDeadzone);
			CHECK(std::strtod(V.at("cursor_slider").c_str(), nullptr) == Pc::ControlsSliderSnap(Pc::PcControlsSlider::CursorSpeed, S.CursorSpeed));
			CHECK(std::strtod(V.at("deadzone_slider").c_str(), nullptr) == Pc::ControlsSliderSnap(Pc::PcControlsSlider::StickDeadzone, S.StickDeadzone));
			CHECK(Unescape(V.at("cursor_line")) == "Cursor speed  " + std::to_string(Pc::ControlsPercent(S.CursorSpeed)) + "%");
			CHECK(Unescape(V.at("deadzone_line")) == "Stick dead zone  " + std::to_string(Pc::ControlsPercent(S.StickDeadzone)) + "%");
			CHECK(SavedText(S.Bindings) == V.at("saved"));
		}
		return R;
	}
}

TEST_CASE("Controls: every move on the page matches the real controls_panel.gd")
{
	FResult R = Run(DataDir() + "/../Source/PawCore/Tests/Data/controls_godot.tsv");
	std::string Ops, Lines;
	for (const auto& O : R.Ops) { Ops += O.first + " " + std::to_string(O.second) + ", "; }
	for (const auto& L : R.Lines) { Lines += L.first + ":" + std::to_string(L.second) + " "; }
	MESSAGE("rows " << R.Rows << "; " << Ops << "lines " << Lines << "; controller column pinned " << R.PadColumnPinned
		<< ", detected " << R.PadColumnDetected << ", Xbox by default " << R.PadColumnDefault << "; still slider moves " << R.SliderMovesStill << "; a stale controller column " << R.StaleColumn << "; a new pad " << R.NewPad << "; timeouts exactly at 0 " << R.TimeoutAtZero);
	CHECK(R.Rows == 500);
	// Every take of an event, every line, every reason the controller column has.
	for (int T = 0; T <= 4; ++T)
	{
		CHECK(R.Ops["event_" + std::to_string(T)] > 5);
	}
	for (int L = 0; L <= 9; ++L)
	{
		CHECK(R.Lines[std::to_string(L)] > 5);
	}
	CHECK(R.PadColumnPinned > 100);
	CHECK(R.PadColumnDetected > 100);
	CHECK(R.PadColumnDefault > 20);
	CHECK(R.SliderMovesStill > 3);
	CHECK(R.StaleColumn == 0); // F37 fixed (D16)
	CHECK(R.NewPad > 5);       // and its case covered
	CHECK(R.TimeoutAtZero > 3);
}

TEST_CASE("Controls: the long suite runs all 1,500 rows" * doctest::test_suite("long") * doctest::skip())
{
	const FResult R = Run(DataDir() + "/../build/controls_godot_full.tsv");
	CHECK(R.Rows == 1500);
}

TEST_CASE("Controls: the page's words are Godot's")
{
	std::string Text;
	REQUIRE(Pc::ReadTextFile(DataDir() + "/../Source/PawCore/Tests/Data/controls_words_godot.tsv", Text));
	std::istringstream In(Text);
	std::string Line;
	int Rows = 0;
	while (std::getline(In, Line))
	{
		const std::vector<std::string> C = Split(Line, '\t');
		if (C.size() == 2 && C[0].rfind("row_", 0) == 0)
		{
			CHECK(Label(C[0].substr(4)) == C[1]);
			++Rows;
		}
	}
	CHECK(Rows == Pc::BindingRowCount);
}

#endif
