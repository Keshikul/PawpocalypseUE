#include "Input/PawKeyCodes.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/WindowsHWrapper.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace PawInput
{
	namespace
	{
		constexpr int64 Special = 4194304; // Godot's KEY_SPECIAL (1 << 22)

		// The keys that have a PLACE: Godot's physical keycode (the US key there) -> its scan code (set 1), and the US layout's
		// virtual key and character for it (what UsLayout() answers).
		struct FPlace
		{
			int64 Godot;
			uint32 Scan;
			uint32 UsVk;
			uint32 UsChar;
		};

		const TArray<FPlace>& Places()
		{
			static const TArray<FPlace> P = []()
			{
				TArray<FPlace> R = {
					{ 96, 0x29, 0xC0, '`' }, { 45, 0x0C, 0xBD, '-' }, { 61, 0x0D, 0xBB, '=' }, { 91, 0x1A, 0xDB, '[' },
					{ 93, 0x1B, 0xDD, ']' }, { 92, 0x2B, 0xDC, '\\' }, { 59, 0x27, 0xBA, ';' }, { 39, 0x28, 0xDE, '\'' },
					{ 44, 0x33, 0xBC, ',' }, { 46, 0x34, 0xBE, '.' }, { 47, 0x35, 0xBF, '/' },
				};
				// 1..9, 0: scan 0x02 .. 0x0B.
				for (int32 I = 0; I < 10; ++I)
				{
					const uint32 Digit = I == 9 ? '0' : static_cast<uint32>('1' + I);
					R.Add({ static_cast<int64>(Digit), static_cast<uint32>(0x02 + I), Digit, Digit });
				}
				// The letters, by row.
				const char* Rows[] = { "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };
				const uint32 First[] = { 0x10, 0x1E, 0x2C };
				for (int32 Row = 0; Row < 3; ++Row)
				{
					for (int32 I = 0; Rows[Row][I]; ++I)
					{
						const uint32 Letter = static_cast<uint32>(Rows[Row][I]);
						R.Add({ static_cast<int64>(Letter), First[Row] + static_cast<uint32>(I), Letter, Letter });
					}
				}
				return R;
			}();
			return P;
		}

		const FPlace* PlaceOfGodot(int64 Code)
		{
			return Places().FindByPredicate([Code](const FPlace& P) { return P.Godot == Code; });
		}

		const FPlace* PlaceOfScan(uint32 Scan)
		{
			return Places().FindByPredicate([Scan](const FPlace& P) { return P.Scan == Scan; });
		}

		const FPawKeyboardLayout* GTestLayout = nullptr;

		FPawKeyboardLayout SystemLayout()
		{
#if PLATFORM_WINDOWS
			// The calling thread's layout (the game thread's: the one Windows used to make the key messages).
			FPawKeyboardLayout L;
			L.ScanToVk = [](uint32 Scan) { return static_cast<uint32>(::MapVirtualKeyExW(Scan, MAPVK_VSC_TO_VK, ::GetKeyboardLayout(0))); };
			L.VkToScan = [](uint32 Vk) { return static_cast<uint32>(::MapVirtualKeyExW(Vk, MAPVK_VK_TO_VSC, ::GetKeyboardLayout(0))); };
			L.VkToChar = [](uint32 Vk) { return static_cast<uint32>(::MapVirtualKeyExW(Vk, MAPVK_VK_TO_CHAR, ::GetKeyboardLayout(0))); };
			return L;
#else
			return UsLayout();
#endif
		}

		// The layout in use: a test's; else US English for an automated run that asks for it (-PawUsKeyboard: the fixtures' key
		// labels are Godot's headless US ones); else the system's.
		const FPawKeyboardLayout& Current()
		{
			if (GTestLayout)
			{
				return *GTestLayout;
			}
			static const bool bUs = FParse::Param(FCommandLine::Get(), TEXT("PawUsKeyboard"));
			static const FPawKeyboardLayout Us = UsLayout();
			static const FPawKeyboardLayout System = SystemLayout();
			return bUs ? Us : System;
		}

		// The FKey Unreal makes of a key at Place under the active layout (FInputKeyManager::GetKeyFromCodes, as WindowsApplication
		// hands it the virtual key and its character).
		FKey KeyAtPlace(const FPlace& Place)
		{
			const FPawKeyboardLayout& L = Current();
			const uint32 Vk = L.ScanToVk(Place.Scan);
			if (Vk == 0)
			{
				return FKey();
			}
			return FInputKeyManager::Get().GetKeyFromCodes(Vk, L.VkToChar(Vk));
		}

		// The place of an Unreal key under the active layout, or null (a key with no place, or none on this layout).
		const FPlace* PlaceOfKey(const FKey& Key)
		{
			const uint32* KeyCode = nullptr;
			const uint32* CharCode = nullptr;
			FInputKeyManager::Get().GetCodesFromKey(Key, KeyCode, CharCode);
			const FPawKeyboardLayout& L = Current();
			// The virtual key: the engine's own for the key, else (a letter or digit, which Unreal names by its character) the
			// character itself, which IS the virtual key for those. Unreal's map of the other characters is built from the
			// system's layout at startup (on a Turkish Q keyboard VK_OEM_7 is EKeys::I), so they are matched last, by the key.
			// A letter or digit first: on a Turkish Q keyboard the engine even gives EKeys::I the key code VK_OEM_7.
			uint32 Vk = KeyCode ? *KeyCode : 0;
			if (CharCode && ((*CharCode >= 'A' && *CharCode <= 'Z') || (*CharCode >= '0' && *CharCode <= '9')))
			{
				Vk = *CharCode;
			}
			if (Vk)
			{
				if (const FPlace* P = PlaceOfScan(L.VkToScan(Vk)))
				{
					return P;
				}
			}
			// A key Unreal names by its character (a punctuation key): the place whose key makes it.
			if (CharCode)
			{
				for (const FPlace& P : Places())
				{
					if (KeyAtPlace(P) == Key)
					{
						return &P;
					}
				}
			}
			return nullptr;
		}

		struct FKeyCode
		{
			int64 Godot;
			FKey Key;
		};

		// Godot physical keycode -> Unreal key. A code may appear twice (both sides of a modifier); the FIRST row of a code is
		// the one a capture reports for that Unreal key... and every row of a code is a key that presses it.
		const TArray<FKeyCode>& KeyTable()
		{
			static const TArray<FKeyCode> T = []()
			{
				TArray<FKeyCode> R = {
					{ 32, EKeys::SpaceBar }, { 39, EKeys::Apostrophe }, { 44, EKeys::Comma }, { 45, EKeys::Hyphen }, { 46, EKeys::Period },
					{ 47, EKeys::Slash }, { 48, EKeys::Zero }, { 49, EKeys::One }, { 50, EKeys::Two }, { 51, EKeys::Three },
					{ 52, EKeys::Four }, { 53, EKeys::Five }, { 54, EKeys::Six }, { 55, EKeys::Seven }, { 56, EKeys::Eight },
					{ 57, EKeys::Nine }, { 59, EKeys::Semicolon }, { 61, EKeys::Equals }, { 91, EKeys::LeftBracket },
					{ 92, EKeys::Backslash }, { 93, EKeys::RightBracket }, { 96, EKeys::Tilde },
					{ Special + 0x01, EKeys::Escape }, { Special + 0x02, EKeys::Tab }, { Special + 0x04, EKeys::BackSpace },
					{ Special + 0x05, EKeys::Enter }, { Special + 0x06, EKeys::Enter }, // keypad Enter: Unreal's Enter
					{ Special + 0x07, EKeys::Insert }, { Special + 0x08, EKeys::Delete }, { Special + 0x09, EKeys::Pause },
					{ Special + 0x0D, EKeys::Home }, { Special + 0x0E, EKeys::End }, { Special + 0x0F, EKeys::Left },
					{ Special + 0x10, EKeys::Up }, { Special + 0x11, EKeys::Right }, { Special + 0x12, EKeys::Down },
					{ Special + 0x13, EKeys::PageUp }, { Special + 0x14, EKeys::PageDown },
					{ Special + 0x15, EKeys::LeftShift }, { Special + 0x15, EKeys::RightShift },
					{ Special + 0x16, EKeys::LeftControl }, { Special + 0x16, EKeys::RightControl },
					{ Special + 0x17, EKeys::LeftCommand }, { Special + 0x17, EKeys::RightCommand },
					{ Special + 0x18, EKeys::LeftAlt }, { Special + 0x18, EKeys::RightAlt },
					{ Special + 0x19, EKeys::CapsLock }, { Special + 0x1A, EKeys::NumLock }, { Special + 0x1B, EKeys::ScrollLock },
					{ Special + 0x81, EKeys::Multiply }, { Special + 0x82, EKeys::Divide }, { Special + 0x83, EKeys::Subtract },
					{ Special + 0x84, EKeys::Decimal }, { Special + 0x85, EKeys::Add },
				};
				const FKey Letters[] = { EKeys::A, EKeys::B, EKeys::C, EKeys::D, EKeys::E, EKeys::F, EKeys::G, EKeys::H, EKeys::I,
					EKeys::J, EKeys::K, EKeys::L, EKeys::M, EKeys::N, EKeys::O, EKeys::P, EKeys::Q, EKeys::R, EKeys::S, EKeys::T,
					EKeys::U, EKeys::V, EKeys::W, EKeys::X, EKeys::Y, EKeys::Z };
				for (int32 I = 0; I < UE_ARRAY_COUNT(Letters); ++I)
				{
					R.Add({ 65 + I, Letters[I] });
				}
				const FKey Fs[] = { EKeys::F1, EKeys::F2, EKeys::F3, EKeys::F4, EKeys::F5, EKeys::F6, EKeys::F7, EKeys::F8,
					EKeys::F9, EKeys::F10, EKeys::F11, EKeys::F12 };
				for (int32 I = 0; I < UE_ARRAY_COUNT(Fs); ++I)
				{
					R.Add({ Special + 0x1C + I, Fs[I] });
				}
				const FKey Pad[] = { EKeys::NumPadZero, EKeys::NumPadOne, EKeys::NumPadTwo, EKeys::NumPadThree, EKeys::NumPadFour,
					EKeys::NumPadFive, EKeys::NumPadSix, EKeys::NumPadSeven, EKeys::NumPadEight, EKeys::NumPadNine };
				for (int32 I = 0; I < UE_ARRAY_COUNT(Pad); ++I)
				{
					R.Add({ Special + 0x86 + I, Pad[I] });
				}
				return R;
			}();
			return T;
		}

		const TArray<FKeyCode>& MouseTable()
		{
			static const TArray<FKeyCode> T = {
				{ 1, EKeys::LeftMouseButton }, { 2, EKeys::RightMouseButton }, { 3, EKeys::MiddleMouseButton },
				{ 4, EKeys::MouseScrollUp }, { 5, EKeys::MouseScrollDown }, { 8, EKeys::ThumbMouseButton }, { 9, EKeys::ThumbMouseButton2 },
			};
			return T;
		}

		// Godot JoyButton -> Unreal (by position: A is the bottom face button on every pad). No Guide, Misc or paddles.
		const TArray<FKeyCode>& ButtonTable()
		{
			static const TArray<FKeyCode> T = {
				{ 0, EKeys::Gamepad_FaceButton_Bottom }, { 1, EKeys::Gamepad_FaceButton_Right }, { 2, EKeys::Gamepad_FaceButton_Left },
				{ 3, EKeys::Gamepad_FaceButton_Top }, { 4, EKeys::Gamepad_Special_Left }, { 6, EKeys::Gamepad_Special_Right },
				{ 7, EKeys::Gamepad_LeftThumbstick }, { 8, EKeys::Gamepad_RightThumbstick }, { 9, EKeys::Gamepad_LeftShoulder },
				{ 10, EKeys::Gamepad_RightShoulder }, { 11, EKeys::Gamepad_DPad_Up }, { 12, EKeys::Gamepad_DPad_Down },
				{ 13, EKeys::Gamepad_DPad_Left }, { 14, EKeys::Gamepad_DPad_Right },
			};
			return T;
		}

		struct FAxis
		{
			int64 Godot;
			FKey Key;
			int32 Flip; // Godot's sign * Flip = Unreal's sign
			FKey Positive; // the digital stand-in of Unreal's + half
			FKey Negative; // ... and of its - half (none for a trigger)
		};

		const TArray<FAxis>& AxisTable()
		{
			static const TArray<FAxis> T = {
				{ 0, EKeys::Gamepad_LeftX, 1, EKeys::Gamepad_LeftStick_Right, EKeys::Gamepad_LeftStick_Left },
				{ 1, EKeys::Gamepad_LeftY, -1, EKeys::Gamepad_LeftStick_Up, EKeys::Gamepad_LeftStick_Down },
				{ 2, EKeys::Gamepad_RightX, 1, EKeys::Gamepad_RightStick_Right, EKeys::Gamepad_RightStick_Left },
				{ 3, EKeys::Gamepad_RightY, -1, EKeys::Gamepad_RightStick_Up, EKeys::Gamepad_RightStick_Down },
				{ 4, EKeys::Gamepad_LeftTriggerAxis, 1, EKeys::Gamepad_LeftTrigger, FKey() },
				{ 5, EKeys::Gamepad_RightTriggerAxis, 1, EKeys::Gamepad_RightTrigger, FKey() },
			};
			return T;
		}

		const FKeyCode* FindByKey(const TArray<FKeyCode>& Table, const FKey& Key)
		{
			return Table.FindByPredicate([&Key](const FKeyCode& C) { return C.Key == Key; });
		}
	}

	TArray<FKey> KeysOfEvent(const Pc::PcInputEvent& E, int32& OutUeSign)
	{
		OutUeSign = 0;
		TArray<FKey> Out;
		auto Collect = [&Out, &E](const TArray<FKeyCode>& Table)
		{
			for (const FKeyCode& C : Table)
			{
				if (C.Godot == E.Code)
				{
					Out.AddUnique(C.Key);
				}
			}
		};
		switch (E.Kind)
		{
		case Pc::PcInputKind::Key:
			if (const FPlace* P = PlaceOfGodot(E.Code))
			{
				const FKey Key = KeyAtPlace(*P);
				if (Key.IsValid())
				{
					Out.Add(Key);
				}
				break;
			}
			Collect(KeyTable());
			break;
		case Pc::PcInputKind::Mouse: Collect(MouseTable()); break;
		case Pc::PcInputKind::JoyButton: Collect(ButtonTable()); break;
		case Pc::PcInputKind::JoyAxis:
			for (const FAxis& A : AxisTable())
			{
				if (A.Godot == E.Code)
				{
					Out.Add(A.Key);
					OutUeSign = static_cast<int32>(E.Sign) * A.Flip;
				}
			}
			break;
		case Pc::PcInputKind::None:
			break;
		}
		return Out;
	}

	Pc::PcInputEvent EventOfKey(const FKey& Key, float AxisValue)
	{
		Pc::PcInputEvent E;
		if (Key.IsValid() && !Key.IsGamepadKey() && !Key.IsMouseButton())
		{
			if (const FPlace* P = PlaceOfKey(Key))
			{
				E.Kind = Pc::PcInputKind::Key;
				E.Code = P->Godot;
				return E;
			}
		}
		auto From = [&E, &Key](const TArray<FKeyCode>& Table, Pc::PcInputKind Kind)
		{
			if (const FKeyCode* C = FindByKey(Table, Key))
			{
				E.Kind = Kind;
				E.Code = C->Godot;
				return true;
			}
			return false;
		};
		if (From(KeyTable(), Pc::PcInputKind::Key) || From(MouseTable(), Pc::PcInputKind::Mouse) || From(ButtonTable(), Pc::PcInputKind::JoyButton))
		{
			return E;
		}
		for (const FAxis& A : AxisTable())
		{
			int32 UeSign = 0;
			if (A.Key == Key)
			{
				UeSign = AxisValue > 0.0f ? 1 : (AxisValue < 0.0f ? -1 : 0);
			}
			else if (A.Positive == Key)
			{
				UeSign = 1;
			}
			else if (A.Negative.IsValid() && A.Negative == Key)
			{
				UeSign = -1;
			}
			else
			{
				continue;
			}
			E.Kind = Pc::PcInputKind::JoyAxis;
			E.Code = A.Godot;
			E.Sign = UeSign * A.Flip;
			return E;
		}
		return E;
	}

	Pc::PcCapturedEvent Captured(const FKey& Key, bool bPressed, bool bRepeat, bool bAnalog, float AxisValue)
	{
		Pc::PcCapturedEvent C;
		const Pc::PcInputEvent E = EventOfKey(Key, bAnalog ? AxisValue : 1.0f);
		C.Kind = E.Kind;
		C.Code = E.Code;
		C.bPressed = bPressed;
		C.bEcho = bRepeat;
		if (E.Kind == Pc::PcInputKind::JoyAxis)
		{
			// Godot's axis_value: the analog value in Godot's direction, or a digital stand-in at full push while held.
			const double Magnitude = bAnalog ? FMath::Abs(static_cast<double>(AxisValue)) : (bPressed ? 1.0 : 0.0);
			C.AxisValue = static_cast<double>(E.Sign) * Magnitude;
		}
		return C;
	}
}

namespace PawInput
{
	void SetLayoutForTest(const FPawKeyboardLayout* Layout)
	{
		static FPawKeyboardLayout Held;
		if (Layout)
		{
			Held = *Layout;
			GTestLayout = &Held;
		}
		else
		{
			GTestLayout = nullptr;
		}
	}

	FPawKeyboardLayout UsLayout()
	{
		FPawKeyboardLayout L;
		L.ScanToVk = [](uint32 Scan) { const FPlace* P = PlaceOfScan(Scan); return P ? P->UsVk : 0u; };
		L.VkToScan = [](uint32 Vk)
		{
			const FPlace* P = Places().FindByPredicate([Vk](const FPlace& X) { return X.UsVk == Vk; });
			return P ? P->Scan : 0u;
		};
		L.VkToChar = [](uint32 Vk)
		{
			const FPlace* P = Places().FindByPredicate([Vk](const FPlace& X) { return X.UsVk == Vk; });
			return P ? P->UsChar : 0u;
		};
		return L;
	}

	FPawKeyboardLayout AzertyLayout()
	{
		// kbdfr's letter keys (Microsoft's French layout): A and Q, Z and W trade places, M sits right of L, and the US M's
		// place is a comma. Every other key with a place keeps the US virtual key (the digit row's are VK_1..VK_0 on kbdfr).
		static const TMap<uint32, uint32> Moved = { { 0x10, 'A' }, { 0x11, 'Z' }, { 0x1E, 'Q' }, { 0x27, 'M' }, { 0x2C, 'W' }, { 0x32, 0xBC } };
		FPawKeyboardLayout Us = UsLayout();
		FPawKeyboardLayout L;
		L.ScanToVk = [Us](uint32 Scan) { const uint32* M = Moved.Find(Scan); return M ? *M : (Scan == 0x33 ? 0u : Us.ScanToVk(Scan)); };
		L.VkToScan = [Us](uint32 Vk)
		{
			for (const TPair<uint32, uint32>& M : Moved)
			{
				if (M.Value == Vk)
				{
					return M.Key;
				}
			}
			const uint32 Scan = Us.VkToScan(Vk);
			return Moved.Contains(Scan) ? 0u : Scan;
		};
		L.VkToChar = [Us](uint32 Vk) { return Vk >= 'A' && Vk <= 'Z' ? Vk : Us.VkToChar(Vk); };
		return L;
	}
}
