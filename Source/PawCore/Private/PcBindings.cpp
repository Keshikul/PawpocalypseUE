#include "PcBindings.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace Pc
{
	namespace
	{
		struct Row
		{
			const char* Id;
			const char* Label;
			std::vector<std::string> Members;
		};

		const std::vector<Row>& Rows()
		{
			static const std::vector<Row> R = {
				{ "pounce", "Pounce", { "pounce" } },
				{ "tab_prev", "Previous tab", { "tab_prev", "ui_left", "section_prev" } },
				{ "tab_next", "Next tab", { "tab_next", "ui_right", "section_next" } },
				{ "up", "Up", { "menu_up", "ui_up" } },
				{ "down", "Down", { "menu_down", "ui_down" } },
				{ "press", "Press", { "press", "ui_accept" } },
				{ "buy_amount", "Buy amount", { "buy_amount" } },
				{ "skill_tree", "Skill tree", { "open_skill_tree" } },
				{ "recentre", "Re-centre", { "recentre" } },
				{ "pause", "Pause", { "pause" } },
			};
			return R;
		}

		// PROTECTED: actions whose keys nobody may take.
		const char* const Protected[] = { "open_upgrades", "tab_1", "tab_2", "tab_3", "tab_4", "tab_5", "tab_6", "zoom_in", "zoom_out",
			"steer_left", "steer_right", "steer_up", "steer_down" };
		const char* const ConfirmActions[] = { "ui_accept", "press" };
		const char* const BackActions[] = { "ui_cancel", "back" };

		// Godot's built-in actions, in the order InputMap registers them (the ones before the project's own; measured: fixture
		// `input_bindings`, input_bindings_defaults_godot.tsv).
		const char* const BuiltInOrder[] = { "ui_accept", "ui_select", "ui_cancel", "ui_close_dialog", "ui_focus_next", "ui_focus_prev",
			"ui_left", "ui_right", "ui_up", "ui_down", "ui_page_up", "ui_page_down", "ui_home", "ui_end" };

		const std::vector<PcInputEvent> Empty;

		const std::vector<PcInputEvent>& EventsOf(const PcInputState& State, const std::string& Action)
		{
			const std::vector<PcInputEvent>* L = BindingFind(State, Action);
			return L ? *L : Empty;
		}

		// String::to_int: the digits in order, a '-' before the first digit negates; anything else is skipped.
		int64_t GodotStringToInt(const std::string& S)
		{
			int64_t Value = 0;
			int64_t Sign = 1;
			for (const char C : S)
			{
				if (C >= '0' && C <= '9')
				{
					Value = Value * 10 + (C - '0');
				}
				else if (C == '-' && Value == 0)
				{
					Sign = -Sign;
				}
			}
			return Value * Sign;
		}

		// int(data.get(key, fallback)).
		int64_t GodotInt(const PcSavedEvent& E, const char* Key, int64_t Fallback)
		{
			const PcCfgValue* V = E.Get(Key);
			if (!V)
			{
				return Fallback;
			}
			switch (V->Kind)
			{
			case PcCfgKind::Int: return V->I;
			case PcCfgKind::Float: return std::isfinite(V->F) ? static_cast<int64_t>(V->F) : 0;
			case PcCfgKind::Bool: return V->B ? 1 : 0;
			case PcCfgKind::String: return GodotStringToInt(V->S);
			}
			return Fallback;
		}

		PcCfgValue IntValue(int64_t I)
		{
			PcCfgValue V;
			V.Kind = PcCfgKind::Int;
			V.I = I;
			return V;
		}

		PcCfgValue StringValue(const char* S)
		{
			PcCfgValue V;
			V.Kind = PcCfgKind::String;
			V.S = S;
			return V;
		}

		// `from` -> `to` in one action's list. `from` None ADDS `to`, to the row's first member only; `to` None REMOVES `from`.
		void Replace(PcInputState& State, const std::string& Action, const PcInputEvent& From, const PcInputEvent& To, bool bPrimary)
		{
			std::vector<PcInputEvent>& List = BindingAt(State, Action);
			if (From.IsNone())
			{
				if (!To.IsNone() && bPrimary)
				{
					List.push_back(To);
				}
				return;
			}
			for (size_t I = 0; I < List.size(); ++I)
			{
				if (BindingSameEvent(List[I], From))
				{
					if (To.IsNone())
					{
						List.erase(List.begin() + static_cast<std::ptrdiff_t>(I));
					}
					else
					{
						List[I] = To;
					}
					break;
				}
			}
		}

		bool RowsShareAnEvent(const PcInputState& State, const std::string& A, const std::string& B)
		{
			for (const std::string& ActionA : BindingMembers(A))
			{
				for (const PcInputEvent& EA : EventsOf(State, ActionA))
				{
					for (const std::string& ActionB : BindingMembers(B))
					{
						for (const PcInputEvent& EB : EventsOf(State, ActionB))
						{
							if (BindingSameEvent(EA, EB))
							{
								return true;
							}
						}
					}
				}
			}
			return false;
		}

		bool SameEvents(const std::vector<PcInputEvent>& A, const std::vector<PcInputEvent>& B)
		{
			if (A.size() != B.size())
			{
				return false;
			}
			for (size_t I = 0; I < A.size(); ++I)
			{
				if (!BindingSameEvent(A[I], B[I]))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool PcCfgValue::operator==(const PcCfgValue& O) const
	{
		if (Kind != O.Kind)
		{
			return false;
		}
		switch (Kind)
		{
		case PcCfgKind::Int: return I == O.I;
		case PcCfgKind::Float: return F == O.F || (std::isnan(F) && std::isnan(O.F));
		case PcCfgKind::String: return S == O.S;
		case PcCfgKind::Bool: return B == O.B;
		}
		return false;
	}

	const PcCfgValue* PcSavedEvent::Get(const std::string& Key) const
	{
		for (const auto& F : Fields)
		{
			if (F.first == Key)
			{
				return &F.second;
			}
		}
		return nullptr;
	}

	const std::vector<PcInputEvent>* BindingFind(const PcInputState& State, const std::string& Action)
	{
		for (const PcActionEvents& A : State)
		{
			if (A.Action == Action)
			{
				return &A.Events;
			}
		}
		return nullptr;
	}

	std::vector<PcInputEvent>& BindingAt(PcInputState& State, const std::string& Action)
	{
		for (PcActionEvents& A : State)
		{
			if (A.Action == Action)
			{
				return A.Events;
			}
		}
		State.push_back({ Action, {} });
		return State.back().Events;
	}

	PcInputEvent BindingEventOf(const std::string& GodotEvent, int64_t Code, int64_t AxisSign)
	{
		PcInputEvent E;
		E.Code = Code;
		if (GodotEvent == "key") { E.Kind = PcInputKind::Key; }
		else if (GodotEvent == "mouse_button") { E.Kind = PcInputKind::Mouse; }
		else if (GodotEvent == "pad_button") { E.Kind = PcInputKind::JoyButton; }
		else if (GodotEvent == "pad_axis") { E.Kind = PcInputKind::JoyAxis; E.Sign = AxisSign > 0 ? 1 : (AxisSign < 0 ? -1 : 0); }
		else { return PcInputEvent(); }
		return E;
	}

	PcInputState BindingGodotOrder(const PcInputState& State)
	{
		PcInputState Out;
		std::set<std::string> Taken;
		for (const char* Name : BuiltInOrder)
		{
			if (const std::vector<PcInputEvent>* L = BindingFind(State, Name))
			{
				Out.push_back({ Name, *L });
				Taken.insert(Name);
			}
		}
		for (const PcActionEvents& A : State)
		{
			if (!Taken.count(A.Action))
			{
				Out.push_back(A);
			}
		}
		return Out;
	}

	const char* BindingRowId(int Row)
	{
		return Rows()[static_cast<size_t>(std::clamp(Row, 0, BindingRowCount - 1))].Id;
	}

	const char* BindingRowLabel(int Row)
	{
		return Rows()[static_cast<size_t>(std::clamp(Row, 0, BindingRowCount - 1))].Label;
	}

	int BindingRowIndex(const std::string& Logical)
	{
		for (int I = 0; I < BindingRowCount; ++I)
		{
			if (Logical == Rows()[static_cast<size_t>(I)].Id)
			{
				return I;
			}
		}
		return -1;
	}

	std::vector<std::string> BindingMembers(const std::string& Logical)
	{
		const int I = BindingRowIndex(Logical);
		return I < 0 ? std::vector<std::string>() : Rows()[static_cast<size_t>(I)].Members;
	}

	bool BindingLocked(const std::string& Logical, PcBindDevice Device)
	{
		if (Logical == "pause")
		{
			return true;
		}
		return Logical == "recentre" && Device == PcBindDevice::Keyboard;
	}

	bool BindingSameEvent(const PcInputEvent& A, const PcInputEvent& B)
	{
		if (A.IsNone() || B.IsNone())
		{
			return A.IsNone() && B.IsNone();
		}
		if (A.Kind != B.Kind)
		{
			return false;
		}
		if (A.Kind == PcInputKind::JoyAxis)
		{
			return A.Code == B.Code && A.Sign == B.Sign;
		}
		return A.Code == B.Code;
	}

	bool BindingIsSystem(const PcInputEvent& E)
	{
		if (E.Kind == PcInputKind::Key)
		{
			return E.Code == GodotInput::KeyEscape;
		}
		if (E.Kind == PcInputKind::JoyButton)
		{
			return E.Code == GodotInput::JoyStart || E.Code == GodotInput::JoyB;
		}
		return false;
	}

	PcBindDevice BindingDeviceOf(const PcInputEvent& E)
	{
		return E.Kind == PcInputKind::JoyButton || E.Kind == PcInputKind::JoyAxis ? PcBindDevice::Pad : PcBindDevice::Keyboard;
	}

	PcInputEvent BindingEventFor(const std::string& Logical, PcBindDevice Device, const PcInputState& State)
	{
		const std::vector<std::string> Members = BindingMembers(Logical);
		if (Members.empty())
		{
			return PcInputEvent();
		}
		for (const PcInputEvent& E : EventsOf(State, Members[0]))
		{
			if (BindingDeviceOf(E) == Device)
			{
				return E;
			}
		}
		return PcInputEvent();
	}

	PcBindOutcome BindingRebind(PcInputState& State, const std::string& Logical, const PcInputEvent& Event)
	{
		const PcBindDevice Device = BindingDeviceOf(Event);
		if (BindingLocked(Logical, Device) || BindingIsSystem(Event))
		{
			return { PcBindResult::RefusedSystem, "" };
		}
		for (const char* Action : Protected)
		{
			for (const PcInputEvent& E : EventsOf(State, Action))
			{
				if (BindingSameEvent(E, Event))
				{
					return { PcBindResult::RefusedTaken, "" };
				}
			}
		}
		const PcInputEvent Old = BindingEventFor(Logical, Device, State);
		if (!Old.IsNone() && BindingSameEvent(Old, Event))
		{
			return { PcBindResult::Unchanged, "" };
		}
		// Who has it now? Every other row, every member - secondary keys too (the LAST row found, as Godot's loop leaves it).
		std::string Other;
		for (const Row& R : Rows())
		{
			if (Logical == R.Id)
			{
				continue;
			}
			for (const std::string& Action : R.Members)
			{
				for (const PcInputEvent& E : EventsOf(State, Action))
				{
					if (BindingSameEvent(E, Event))
					{
						Other = R.Id;
					}
				}
			}
		}
		if (!Other.empty() && BindingLocked(Other, Device))
		{
			return { PcBindResult::RefusedSystem, "" };
		}
		const std::vector<std::string> Mine = BindingMembers(Logical);
		for (size_t I = 0; I < Mine.size(); ++I)
		{
			Replace(State, Mine[I], Old, Event, I == 0);
		}
		if (!Other.empty())
		{
			const std::vector<std::string> Theirs = BindingMembers(Other);
			for (size_t I = 0; I < Theirs.size(); ++I)
			{
				Replace(State, Theirs[I], Event, Old, I == 0);
			}
			return { PcBindResult::Swapped, Other };
		}
		return { PcBindResult::Bound, "" };
	}

	void BindingResetRow(PcInputState& State, const PcInputState& Defaults, const std::string& Logical)
	{
		std::vector<std::string> Queue = { Logical }; // first in, first out (Godot's pop_front)
		std::set<std::string> Done;
		for (size_t Next = 0; Next < Queue.size(); ++Next)
		{
			const std::string Id = Queue[Next];
			if (Done.count(Id))
			{
				continue;
			}
			Done.insert(Id);
			for (const std::string& Action : BindingMembers(Id))
			{
				BindingAt(State, Action) = EventsOf(Defaults, Action);
			}
			for (const Row& R : Rows())
			{
				if (Done.count(R.Id))
				{
					continue;
				}
				if (RowsShareAnEvent(State, Id, R.Id))
				{
					Queue.push_back(R.Id);
				}
			}
		}
	}

	PcSavedEvent BindingSerialize(const PcInputEvent& E)
	{
		PcSavedEvent Out;
		switch (E.Kind)
		{
		case PcInputKind::Key:
			Out.Fields = { { "type", StringValue("key") }, { "physical_keycode", IntValue(E.Code) } };
			break;
		case PcInputKind::Mouse:
			Out.Fields = { { "type", StringValue("mouse") }, { "button", IntValue(E.Code) } };
			break;
		case PcInputKind::JoyButton:
			Out.Fields = { { "type", StringValue("joy_button") }, { "button", IntValue(E.Code) } };
			break;
		case PcInputKind::JoyAxis:
			Out.Fields = { { "type", StringValue("joy_axis") }, { "axis", IntValue(E.Code) }, { "sign", IntValue(E.Sign) } };
			break;
		case PcInputKind::None:
			break;
		}
		return Out;
	}

	PcInputEvent BindingDeserialize(const PcSavedEvent& E)
	{
		// String(data.get("type", "")): only a string can name a type.
		const PcCfgValue* Type = E.Get("type");
		const std::string T = Type && Type->Kind == PcCfgKind::String ? Type->S : std::string();
		PcInputEvent Out;
		if (T == "key")
		{
			Out.Kind = PcInputKind::Key;
			Out.Code = GodotInt(E, "physical_keycode", 0);
			return Out.Code != 0 ? Out : PcInputEvent();
		}
		if (T == "mouse")
		{
			Out.Kind = PcInputKind::Mouse;
			Out.Code = GodotInt(E, "button", 1);
			return Out;
		}
		if (T == "joy_button")
		{
			Out.Kind = PcInputKind::JoyButton;
			Out.Code = GodotInt(E, "button", 0);
			return Out;
		}
		if (T == "joy_axis")
		{
			Out.Kind = PcInputKind::JoyAxis;
			Out.Code = GodotInt(E, "axis", 0);
			Out.Sign = GodotInt(E, "sign", 1) >= 0 ? 1 : -1;
			return Out;
		}
		return PcInputEvent();
	}

	PcInputState BindingWorkingState(const PcSavedBindings& Saved, const PcInputState& Defaults)
	{
		PcInputState State = Defaults;
		for (const PcSavedAction& A : Saved)
		{
			if (!BindingFind(State, A.Action))
			{
				continue;
			}
			std::vector<PcInputEvent> List;
			for (const PcSavedEvent& Data : A.Events)
			{
				const PcInputEvent E = BindingDeserialize(Data);
				if (!E.IsNone())
				{
					List.push_back(E);
				}
			}
			if (!List.empty())
			{
				BindingAt(State, A.Action) = List;
			}
		}
		return State;
	}

	PcSavedBindings BindingDiff(const PcInputState& State, const PcInputState& Defaults)
	{
		PcSavedBindings Out;
		for (const PcActionEvents& A : State)
		{
			if (SameEvents(A.Events, EventsOf(Defaults, A.Action)))
			{
				continue;
			}
			PcSavedAction S;
			S.Action = A.Action;
			for (const PcInputEvent& E : A.Events)
			{
				S.Events.push_back(BindingSerialize(E));
			}
			Out.push_back(S);
		}
		return Out;
	}

	PcInputState BindingApply(const PcSavedBindings& Saved, bool bSwapSticks, bool bSwapConfirm, const PcInputState& Defaults)
	{
		PcInputState Live = BindingWorkingState(Saved, Defaults);
		// InputMap.action_add_event() skips an event the action already has (measured: a list a swap left with the same key
		// twice is live once). The rules' own state keeps both, as Godot's working state does.
		for (PcActionEvents& A : Live)
		{
			std::vector<PcInputEvent> Unique;
			for (const PcInputEvent& E : A.Events)
			{
				if (std::none_of(Unique.begin(), Unique.end(), [&E](const PcInputEvent& U) { return BindingSameEvent(U, E); }))
				{
					Unique.push_back(E);
				}
			}
			A.Events = Unique;
		}
		if (bSwapSticks)
		{
			for (PcActionEvents& A : Live)
			{
				for (PcInputEvent& E : A.Events)
				{
					if (E.Kind == PcInputKind::JoyAxis && E.Code >= GodotInput::AxisLeftX && E.Code <= GodotInput::AxisRightY)
					{
						E.Code = E.Code < GodotInput::AxisRightX ? E.Code + 2 : E.Code - 2;
					}
				}
			}
		}
		if (bSwapConfirm)
		{
			auto Swap = [&Live](const char* Action)
			{
				for (PcActionEvents& A : Live)
				{
					if (A.Action != Action)
					{
						continue;
					}
					for (PcInputEvent& E : A.Events)
					{
						if (E.Kind == PcInputKind::JoyButton && (E.Code == GodotInput::JoyA || E.Code == GodotInput::JoyB))
						{
							E.Code = E.Code == GodotInput::JoyA ? GodotInput::JoyB : GodotInput::JoyA;
						}
					}
				}
			};
			for (const char* Action : ConfirmActions)
			{
				Swap(Action);
			}
			for (const char* Action : BackActions)
			{
				Swap(Action);
			}
		}
		return Live;
	}

	double BindingSteerDeadZone(double DeadZone)
	{
		// Godot stores a dead zone as a 32-bit float: the nearest one, ties to even, by arithmetic (PawCore holds no float;
		// 0.05 .. 0.9 is far inside the normal range, so 24 bits of mantissa are the whole rule).
		const double V = std::clamp(DeadZone, 0.05, 0.9);
		int Exp = 0;
		const double Mantissa = std::frexp(V, &Exp);
		return std::ldexp(std::nearbyint(std::ldexp(Mantissa, 24)), Exp - 24);
	}

	PcInputEvent BindingFrom(const PcCapturedEvent& E)
	{
		PcInputEvent Out;
		switch (E.Kind)
		{
		case PcInputKind::Key:
			if (!E.bPressed || E.bEcho)
			{
				return Out;
			}
			Out.Kind = PcInputKind::Key;
			Out.Code = E.Code != 0 ? E.Code : E.KeyCode;
			return Out;
		case PcInputKind::Mouse:
			if (!E.bPressed || (E.Code >= GodotInput::MouseWheelUp && E.Code <= GodotInput::MouseWheelRight))
			{
				return Out;
			}
			Out.Kind = PcInputKind::Mouse;
			Out.Code = E.Code;
			return Out;
		case PcInputKind::JoyButton:
			if (!E.bPressed)
			{
				return Out;
			}
			Out.Kind = PcInputKind::JoyButton;
			Out.Code = E.Code;
			return Out;
		case PcInputKind::JoyAxis:
			// axis_value is a 32-bit float in Godot, and every value a pad sends is one already: compared as it comes.
			if ((E.Code != GodotInput::AxisTriggerLeft && E.Code != GodotInput::AxisTriggerRight) || std::fabs(E.AxisValue) < 0.5)
			{
				return Out;
			}
			Out.Kind = PcInputKind::JoyAxis;
			Out.Code = E.Code;
			Out.Sign = 1;
			return Out;
		case PcInputKind::None:
			break;
		}
		return Out;
	}
}
