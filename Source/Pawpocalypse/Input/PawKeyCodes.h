#pragma once

// Godot's input events <-> Unreal's keys (docs/05 §3, remapping): the live mapping context is built from Godot events
// (PawCore PcBindings), and the Controls page captures a Slate key as a Godot event (Pc::BindingFrom).
//
// Keys are by POSITION: a Godot physical keycode is the US-layout key at that place (Godot's KEY_* values, measured: fixture
// `input_glyphs`, input_keys_godot.tsv). UNREAL'S FKEY IS NOT A POSITION: on Windows it comes from the virtual-key code, which
// the keyboard layout picks (WindowsApplication.cpp: wParam, then MapVirtualKey(VK, MAPVK_VK_TO_CHAR) for the character), so
// on AZERTY the key in W's place is EKeys::Z. Every key that has a place (the letters, digits and punctuation) is therefore
// mapped through the ACTIVE LAYOUT: its US scan code -> the layout's virtual key -> the FKey Unreal makes of it (the engine's
// own path), and back for a capture. The others (arrows, F-keys, Enter, Space, the modifiers) are the same on every layout.
// Godot has ONE code for Shift, Ctrl, Alt and Meta (the side is a separate property): each is both of Unreal's (LeftShift and
// RightShift). Keypad Enter has no Unreal key of its own: it is Enter, and Enter is captured as Godot's ENTER. Godot's +Y on a
// stick is DOWN, Unreal's UP: the Y axes flip sign.

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "PcBindings.h"

namespace PawInput
{
	// A keyboard layout as Windows answers for it (MapVirtualKeyEx): scan code -> virtual key, virtual key -> scan code, virtual
	// key -> its character. The system's active layout unless a test sets one (and puts it back with null).
	struct FPawKeyboardLayout
	{
		TFunction<uint32(uint32)> ScanToVk;
		TFunction<uint32(uint32)> VkToScan;
		TFunction<uint32(uint32)> VkToChar;
	};
	PAWPOCALYPSE_API void SetLayoutForTest(const FPawKeyboardLayout* Layout);
	// The layouts the tests use: US English and French AZERTY (kbdfr), as Windows maps their letter keys.
	PAWPOCALYPSE_API FPawKeyboardLayout UsLayout();
	PAWPOCALYPSE_API FPawKeyboardLayout AzertyLayout();

	// The Unreal keys that press a Godot event, and for a pad axis the half of Unreal's axis that counts (OutUeSign: -1 / 1).
	// Empty when Unreal has no such key (a pad's Guide, the paddles, the wheel's sides).
	PAWPOCALYPSE_API TArray<FKey> KeysOfEvent(const Pc::PcInputEvent& E, int32& OutUeSign);
	// A Slate key as the Godot event it stands for (an axis key with the direction AxisValue gives it; a trigger's or stick's
	// digital stand-in as its axis at full push); None when Godot has no such key.
	PAWPOCALYPSE_API Pc::PcInputEvent EventOfKey(const FKey& Key, float AxisValue = 1.0f);
	// What the Controls page hands Pc::BindingFrom for a Slate key event: a key or button down / up (bRepeat: an echo), or an
	// analog value (bAnalog: AxisValue is the axis's, Unreal's sign).
	PAWPOCALYPSE_API Pc::PcCapturedEvent Captured(const FKey& Key, bool bPressed, bool bRepeat, bool bAnalog = false, float AxisValue = 0.0f);
}
