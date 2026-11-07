#include "PcDevice.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace Pc
{
	namespace
	{
		constexpr int64_t VendorSony = 0x054C;
		constexpr int64_t VendorMicrosoft = 0x045E;
		constexpr int64_t VendorValve = 0x28DE;
		constexpr int64_t ProductDeck = 0x1205;
		constexpr int64_t ProductSteamVirtual = 0x11FF;

		// NAME_FAMILIES, searched in order (the specific names first; "wireless controller" last).
		struct FNameFamily { const char* Text; PcFamily Family; };
		const FNameFamily NameFamilies[] = {
			{ "dualsense", PcFamily::PlayStation }, { "dualshock", PcFamily::PlayStation }, { "ps5", PcFamily::PlayStation },
			{ "ps4", PcFamily::PlayStation }, { "playstation", PcFamily::PlayStation }, { "xbox", PcFamily::Xbox },
			{ "xinput", PcFamily::Xbox }, { "steam deck", PcFamily::SteamDeck }, { "wireless controller", PcFamily::PlayStation },
		};

		void SetCursor(PcDeviceState& S, PcCursorSource Value)
		{
			if (S.Cursor == Value)
			{
				return;
			}
			S.Cursor = Value;
			if (Value == PcCursorSource::Stick)
			{
				S.MouseTravel = 0.0;
			}
			// _hide_pointer(hidden): hidden or pointer_over_field
			S.bPointerHidden = Value == PcCursorSource::Stick || S.bPointerOverField;
		}

		void SetDetected(PcDeviceState& S, PcFamily Value)
		{
			S.Detected = Value;
			DeviceRefreshFamily(S);
		}

		void ClaimPad(PcDeviceState& S, int64_t Device)
		{
			S.bLastWasPointer = false;
			S.ActiveDevice = Device;
			SetDetected(S, DeviceFamilyOf(S, Device));
		}
	}

	PcFamily DeviceFamilyFromInfo(int64_t VendorId, int64_t ProductId, const std::string& Name, const std::string& RawName, bool bOnDeck)
	{
		if (VendorId == VendorValve && ProductId == ProductSteamVirtual)
		{
			// On a Deck the built-in pad is always behind Steam Input: the virtual pad IS the Deck. Anywhere else: anything.
			return bOnDeck ? PcFamily::SteamDeck : PcFamily::Generic;
		}
		if (VendorId == VendorValve && ProductId == ProductDeck)
		{
			return PcFamily::SteamDeck;
		}
		if (VendorId == VendorSony)
		{
			return PcFamily::PlayStation;
		}
		if (VendorId == VendorMicrosoft)
		{
			return PcFamily::Xbox;
		}
		std::string Names = Name + " " + RawName;
		std::transform(Names.begin(), Names.end(), Names.begin(), [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
		for (const FNameFamily& N : NameFamilies)
		{
			if (Names.find(N.Text) != std::string::npos)
			{
				return N.Family;
			}
		}
		return PcFamily::Generic;
	}

	void DeviceRegister(PcDeviceState& S, int64_t Device, PcFamily Family)
	{
		S.DeviceFamilies[Device] = Family;
	}

	PcFamily DeviceFamilyOf(PcDeviceState& S, int64_t Device)
	{
		const auto It = S.DeviceFamilies.find(Device);
		if (It != S.DeviceFamilies.end())
		{
			return It->second;
		}
		// register_device(device) with nothing to ask: family_from_info({}, "", on_deck) is Generic.
		S.DeviceFamilies[Device] = PcFamily::Generic;
		return PcFamily::Generic;
	}

	bool DeviceRefreshFamily(PcDeviceState& S)
	{
		// STYLE_FAMILY ("auto" is absent on purpose: it follows what was detected).
		PcFamily Next = S.Detected;
		if (S.PromptStyle == "keyboard") { Next = PcFamily::Keyboard; }
		else if (S.PromptStyle == "xbox") { Next = PcFamily::Xbox; }
		else if (S.PromptStyle == "playstation") { Next = PcFamily::PlayStation; }
		else if (S.PromptStyle == "steam_deck") { Next = PcFamily::SteamDeck; }
		if (Next == S.Family)
		{
			return false;
		}
		S.Family = Next;
		return true;
	}

	bool DeviceIsSteerAxis(const PcDeviceState& S, int64_t Axis)
	{
		return S.bSwapSticks ? (Axis == 0 || Axis == 1) : (Axis == 2 || Axis == 3);
	}

	void DeviceObserve(PcDeviceState& S, const PcDeviceEvent& E, int64_t NowMs)
	{
		// A trigger is an AXIS: every motion past the dead zone reads "pressed", so only its crossing from released counts.
		if (E.Kind == PcDeviceEventKind::PadMotion)
		{
			if (E.bPressAction)
			{
				if (E.bPressPressed && !S.bPressAxisDown)
				{
					S.PressBeganMs = NowMs;
				}
				S.bPressAxisDown = E.bPressPressed;
			}
		}
		else if (E.bPressAction && E.bPressed && !E.bEcho)
		{
			S.PressBeganMs = NowMs;
		}
		switch (E.Kind)
		{
		case PcDeviceEventKind::Key:
			if (E.bPressed && !E.bEcho)
			{
				S.bLastWasPointer = false;
				SetDetected(S, PcFamily::Keyboard);
			}
			break;
		case PcDeviceEventKind::MouseButton:
			if (E.bPressed)
			{
				S.bLastWasPointer = true;
				SetDetected(S, PcFamily::Keyboard);
				SetCursor(S, PcCursorSource::Mouse);
			}
			break;
		case PcDeviceEventKind::MouseMotion:
		{
			// Motion steers but never flips the prompts.
			if (NowMs < S.WarpUntilMs)
			{
				return;
			}
			if (NowMs - S.LastMotionMs > DeviceMouseQuietMs)
			{
				S.MouseTravel = 0.0;
			}
			S.LastMotionMs = NowMs;
			S.MouseTravel += E.MotionLength;
			if (S.MouseTravel >= DeviceMouseClaimPx)
			{
				S.MouseTravel = 0.0;
				S.bLastWasPointer = true;
				SetCursor(S, PcCursorSource::Mouse);
			}
			break;
		}
		case PcDeviceEventKind::PadButton:
			if (E.bPressed)
			{
				ClaimPad(S, E.Device);
			}
			break;
		case PcDeviceEventKind::PadMotion:
			if (std::fabs(E.AxisValue) >= DeviceAxisClaim)
			{
				ClaimPad(S, E.Device);
				if (DeviceIsSteerAxis(S, E.Axis))
				{
					SetCursor(S, PcCursorSource::Stick);
				}
			}
			break;
		}
	}

	void DeviceClaimStick(PcDeviceState& S)
	{
		SetCursor(S, PcCursorSource::Stick);
	}

	void DeviceWarp(PcDeviceState& S, int64_t NowMs)
	{
		S.WarpUntilMs = NowMs + DeviceWarpEchoMs;
	}

	bool DevicePressCountsFor(const PcDeviceState& S, int64_t OpenedMs, int64_t NowMs)
	{
		return S.PressBeganMs >= OpenedMs && NowMs - OpenedMs >= DeviceModalGuardMs;
	}

	void DeviceConnection(PcDeviceState& S, int64_t Device, bool bConnected, PcFamily Family, int64_t NowMs)
	{
		if (bConnected)
		{
			DeviceRegister(S, Device, Family);
			if (NowMs - S.BootMs > DeviceBootQuietMs)
			{
				S.Toast = PcDeviceToast::Connected;
			}
			return;
		}
		S.DeviceFamilies.erase(Device);
		if (Device != S.ActiveDevice)
		{
			return;
		}
		// The pad in the player's hands went away: back to the mouse and keys (no pause: an idle game has no fail state).
		S.ActiveDevice = -1;
		SetDetected(S, PcFamily::Keyboard);
		SetCursor(S, PcCursorSource::Mouse);
		S.Toast = PcDeviceToast::Disconnected;
	}

	bool DevicePadAudioToast(const std::vector<std::string>& Before, const std::vector<std::string>& After,
		const std::string& OutputDevice)
	{
		if (OutputDevice != "Default")
		{
			return false; // the player chose where the sound goes: a new device cannot take it
		}
		for (const std::string& Name : After)
		{
			if (std::find(Before.begin(), Before.end(), Name) == Before.end())
			{
				return true;
			}
		}
		return false;
	}

	void DeviceSetOverField(PcDeviceState& S, bool bOver)
	{
		// _process: over := cursor_source == MOUSE and ...; on a change, _hide_pointer(cursor_source == STICK).
		const bool bNow = bOver && S.Cursor == PcCursorSource::Mouse;
		if (bNow == S.bPointerOverField)
		{
			return;
		}
		S.bPointerOverField = bNow;
		S.bPointerHidden = S.Cursor == PcCursorSource::Stick || S.bPointerOverField;
	}
}
