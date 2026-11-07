#include "PcControls.h"

#include <algorithm>
#include <cmath>

namespace Pc
{
	namespace
	{
		bool& CheckRef(PcSettings& S, PcControlsCheck Check)
		{
			switch (Check)
			{
			case PcControlsCheck::ShowPrompts: return S.bShowPrompts;
			case PcControlsCheck::TutorialTips: return S.bTutorialTips;
			case PcControlsCheck::SwapConfirmBack: return S.bSwapConfirmBack;
			case PcControlsCheck::SwapSticks: return S.bSwapSticks;
			case PcControlsCheck::InvertX: return S.bInvertX;
			case PcControlsCheck::InvertY: break;
			}
			return S.bInvertY;
		}

		double& SliderRef(PcSettings& S, PcControlsSlider Which)
		{
			return Which == PcControlsSlider::CursorSpeed ? S.CursorSpeed : S.StickDeadzone;
		}

		void EndListen(PcControlsState& C, PcControlsLine Line, const std::string& Row)
		{
			C.bListening = false;
			C.ListenRow.clear();
			C.ListenLeft = 0.0;
			C.Line = Line;
			C.LineRow = Row;
		}
	}

	bool ControlsCheckValue(const PcSettings& S, PcControlsCheck Check)
	{
		return CheckRef(const_cast<PcSettings&>(S), Check);
	}

	bool ControlsSetCheck(PcSettings& S, PcControlsCheck Check, bool bOn)
	{
		bool& Held = CheckRef(S, Check);
		if (Held == bOn)
		{
			return false;
		}
		Held = bOn;
		return true;
	}

	double ControlsSliderMin(PcControlsSlider Which)
	{
		return Which == PcControlsSlider::CursorSpeed ? SettingsCursorSpeedMin : SettingsDeadzoneMin;
	}

	double ControlsSliderMax(PcControlsSlider Which)
	{
		return Which == PcControlsSlider::CursorSpeed ? SettingsCursorSpeedMax : SettingsDeadzoneMax;
	}

	double ControlsSliderStep(PcControlsSlider Which)
	{
		return Which == PcControlsSlider::CursorSpeed ? 0.05 : 0.01;
	}

	double ControlsSliderSnap(PcControlsSlider Which, double Value)
	{
		const double Min = ControlsSliderMin(Which);
		const double Step = ControlsSliderStep(Which);
		if (std::isfinite(Value))
		{
			Value = std::floor((Value - Min) / Step + 0.5) / (1.0 / Step) + Min;
		}
		return std::clamp(Value, Min, ControlsSliderMax(Which));
	}

	double ControlsSliderSetting(const PcSettings& S, PcControlsSlider Which)
	{
		return Which == PcControlsSlider::CursorSpeed ? S.CursorSpeed : S.StickDeadzone;
	}

	bool ControlsSetSlider(PcSettings& S, PcControlsSlider Which, double Value)
	{
		double& Held = SliderRef(S, Which);
		if (Held == Value)
		{
			return false;
		}
		Held = std::clamp(Value, ControlsSliderMin(Which), ControlsSliderMax(Which));
		return true;
	}

	int64_t ControlsPercent(double Value)
	{
		return static_cast<int64_t>(std::round(Value * 100.0));
	}

	void ControlsListen(PcControlsState& C, const std::string& Row, PcBindDevice Device)
	{
		C.bListening = true;
		C.ListenRow = Row;
		C.ListenDevice = Device;
		C.ListenLeft = BindingListenSec;
		C.Line = PcControlsLine::Listening;
		C.LineRow = Row;
	}

	bool ControlsTick(PcControlsState& C, double DeltaSeconds)
	{
		if (!C.bListening)
		{
			return false;
		}
		C.ListenLeft -= DeltaSeconds;
		if (C.ListenLeft <= 0.0)
		{
			EndListen(C, PcControlsLine::NothingPressed, "");
			return true;
		}
		return false;
	}

	PcBindResult ControlsBind(PcControlsState& C, PcSettings& S, const PcInputState& Defaults, const std::string& Row,
		const PcInputEvent& Event, bool& bSaved)
	{
		PcInputState State = BindingWorkingState(S.Bindings, Defaults);
		const PcBindOutcome Out = BindingRebind(State, Row, Event);
		PcControlsLine Line = PcControlsLine::None;
		std::string LineRow;
		switch (Out.Result)
		{
		case PcBindResult::Bound: Line = PcControlsLine::Done; break;
		case PcBindResult::Swapped: Line = PcControlsLine::Swapped; LineRow = Out.Other; break;
		case PcBindResult::RefusedSystem: Line = PcControlsLine::RefusedSystem; break;
		case PcBindResult::RefusedTaken: Line = PcControlsLine::Taken; break;
		case PcBindResult::Unchanged: break; // match has no case for it: the line goes blank
		}
		bSaved = Out.Result == PcBindResult::Bound || Out.Result == PcBindResult::Swapped;
		if (bSaved)
		{
			S.Bindings = BindingDiff(State, Defaults);
		}
		EndListen(C, Line, LineRow);
		return Out.Result;
	}

	PcControlsInput ControlsInput(PcControlsState& C, PcSettings& S, const PcInputState& Defaults, const PcCapturedEvent& E,
		bool bMotion, bool bBack)
	{
		PcControlsInput Out;
		if (!C.bListening)
		{
			return Out;
		}
		if (bMotion)
		{
			Out.Take = PcControlsTake::Passed;
			return Out;
		}
		Out.Take = PcControlsTake::Swallowed;
		// Back cancels, on whichever device.
		if (bBack)
		{
			EndListen(C, PcControlsLine::Cancelled, "");
			Out.Take = PcControlsTake::Cancelled;
			return Out;
		}
		// A click while a CONTROLLER cell listens is the mouse player changing their mind, not a binding.
		if (C.ListenDevice == PcBindDevice::Pad && E.Kind == PcInputKind::Mouse && E.bPressed)
		{
			EndListen(C, PcControlsLine::Cancelled, "");
			Out.Take = PcControlsTake::Cancelled;
			return Out;
		}
		const PcInputEvent Binding = BindingFrom(E);
		if (Binding.IsNone() || BindingDeviceOf(Binding) != C.ListenDevice)
		{
			return Out;
		}
		const std::string Row = C.ListenRow;
		Out.Result = ControlsBind(C, S, Defaults, Row, Binding, Out.bSaved);
		Out.Take = PcControlsTake::Bound;
		return Out;
	}

	void ControlsResetRow(PcControlsState& C, PcSettings& S, const PcInputState& Defaults, const std::string& Row)
	{
		PcInputState State = BindingWorkingState(S.Bindings, Defaults);
		BindingResetRow(State, Defaults, Row);
		S.Bindings = BindingDiff(State, Defaults);
		C.Line = PcControlsLine::Reset; // listening, if it was, goes on
		C.LineRow.clear();
	}

	void ControlsResetAll(PcControlsState& C, PcSettings& S)
	{
		S.Bindings.clear();
		S.bShowPrompts = true;
		S.bTutorialTips = true;
		S.bSwapConfirmBack = false;
		S.bSwapSticks = false;
		S.bInvertX = false;
		S.bInvertY = false;
		S.CursorSpeed = 1.0;
		S.StickDeadzone = 0.2;
		C.Line = PcControlsLine::ResetAll;
		C.LineRow.clear();
	}

	bool ControlsCellListening(const PcControlsState& C, const std::string& Row, PcBindDevice Device)
	{
		return C.bListening && C.ListenRow == Row && C.ListenDevice == Device;
	}

	bool ControlsResetEnabled(const std::string& Row)
	{
		return Row != "pause";
	}

	PcFamily ControlsPadColumnFamily(const std::string& PromptStyle, PcFamily Family, PcFamily Detected)
	{
		if (PromptStyle == "xbox") { return PcFamily::Xbox; }
		if (PromptStyle == "playstation") { return PcFamily::PlayStation; }
		if (PromptStyle == "steam_deck") { return PcFamily::SteamDeck; }
		if (Family != PcFamily::Keyboard)
		{
			return Family;
		}
		if (Detected != PcFamily::Keyboard)
		{
			return Detected;
		}
		return PcFamily::Xbox;
	}
}
