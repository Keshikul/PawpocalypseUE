#include "UI/PawGlyphs.h"

#include "Engine/Texture2D.h"
#include "Input/PawInputMap.h"
#include "Input/PawKeyCodes.h"
#include "UI/PawText.h"
#include "UI/PawUiAssets.h"

namespace PawGlyphs
{
	const TCHAR* const TextureDir = TEXT("/Game/Pawpocalypse/Art/UI/Input");

	namespace
	{
		// FOLDERS: GENERIC draws as Xbox (what most unknown pads imitate; Valve accepts it for Deck Verified).
		const TCHAR* Folder(Pc::PcFamily Family)
		{
			switch (Family)
			{
			case Pc::PcFamily::Keyboard: return TEXT("keyboard");
			case Pc::PcFamily::PlayStation: return TEXT("playstation");
			case Pc::PcFamily::SteamDeck: return TEXT("steam_deck");
			default: return TEXT("xbox");
			}
		}

		struct FPosition { FKey Key; const TCHAR* Name; };

		// BUTTON_POSITIONS and AXIS_POSITIONS (the guide button left out: the OS owns it). Both directions of a stick are the
		// stick; Godot binds axes, never Unreal's digital stand-ins, so those have no position.
		const TArray<FPosition>& PadPositions()
		{
			static const TArray<FPosition> P = {
				{ EKeys::Gamepad_FaceButton_Bottom, TEXT("face_down") }, { EKeys::Gamepad_FaceButton_Right, TEXT("face_right") },
				{ EKeys::Gamepad_FaceButton_Left, TEXT("face_left") }, { EKeys::Gamepad_FaceButton_Top, TEXT("face_up") },
				{ EKeys::Gamepad_Special_Left, TEXT("select") }, { EKeys::Gamepad_Special_Right, TEXT("start") },
				{ EKeys::Gamepad_LeftThumbstick, TEXT("stick_l_click") }, { EKeys::Gamepad_RightThumbstick, TEXT("stick_r_click") },
				{ EKeys::Gamepad_LeftShoulder, TEXT("shoulder_l") }, { EKeys::Gamepad_RightShoulder, TEXT("shoulder_r") },
				{ EKeys::Gamepad_DPad_Up, TEXT("dpad_up") }, { EKeys::Gamepad_DPad_Down, TEXT("dpad_down") },
				{ EKeys::Gamepad_DPad_Left, TEXT("dpad_left") }, { EKeys::Gamepad_DPad_Right, TEXT("dpad_right") },
				{ EKeys::Gamepad_LeftX, TEXT("stick_l") }, { EKeys::Gamepad_LeftY, TEXT("stick_l") },
				{ EKeys::Gamepad_RightX, TEXT("stick_r") }, { EKeys::Gamepad_RightY, TEXT("stick_r") },
				{ EKeys::Gamepad_LeftTriggerAxis, TEXT("trigger_l") }, { EKeys::Gamepad_RightTriggerAxis, TEXT("trigger_r") },
			};
			return P;
		}

		// MOUSE_POSITIONS and ARROW_KEYS: drawn from the keyboard folder whatever the family.
		const TArray<FPosition>& KeyboardPositions()
		{
			static const TArray<FPosition> P = {
				{ EKeys::LeftMouseButton, TEXT("mouse_left") }, { EKeys::RightMouseButton, TEXT("mouse_right") },
				{ EKeys::MiddleMouseButton, TEXT("mouse_middle") },
				{ EKeys::Up, TEXT("arrow_up") }, { EKeys::Down, TEXT("arrow_down") },
				{ EKeys::Left, TEXT("arrow_left") }, { EKeys::Right, TEXT("arrow_right") },
			};
			return P;
		}

		const TCHAR* PositionOf(const TArray<FPosition>& Table, const FKey& Key)
		{
			for (const FPosition& P : Table)
			{
				if (P.Key == Key)
				{
					return P.Name;
				}
			}
			return nullptr;
		}

		// KEY_NAMES (translated, context "key name"): the keys whose name is a word.
		const TCHAR* NamedKey(const FKey& Key)
		{
			struct FNamed { FKey Key; const TCHAR* StringKey; };
			static const FNamed Named[] = {
				{ EKeys::SpaceBar, TEXT("KEY_space") }, { EKeys::Enter, TEXT("KEY_enter") }, { EKeys::Escape, TEXT("KEY_escape") },
				{ EKeys::Tab, TEXT("KEY_tab") }, { EKeys::LeftShift, TEXT("KEY_shift") }, { EKeys::RightShift, TEXT("KEY_shift") },
				{ EKeys::LeftControl, TEXT("KEY_ctrl") }, { EKeys::RightControl, TEXT("KEY_ctrl") },
				{ EKeys::LeftAlt, TEXT("KEY_alt") }, { EKeys::RightAlt, TEXT("KEY_alt") }, { EKeys::BackSpace, TEXT("KEY_backspace") },
			};
			for (const FNamed& N : Named)
			{
				if (N.Key == Key)
				{
					return N.StringKey;
				}
			}
			return nullptr;
		}

		// OS.get_keycode_string for the keys Unreal names differently ("Page Up" is Godot's "PageUp"): Godot shows these
		// untranslated, as the engine names them.
		const TCHAR* GodotKeyName(const FKey& Key)
		{
			struct FName2 { FKey Key; const TCHAR* Name; };
			static const FName2 Names[] = {
				{ EKeys::PageUp, TEXT("PageUp") }, { EKeys::PageDown, TEXT("PageDown") }, { EKeys::CapsLock, TEXT("CapsLock") },
				{ EKeys::NumPadZero, TEXT("Kp 0") }, { EKeys::NumPadOne, TEXT("Kp 1") }, { EKeys::NumPadTwo, TEXT("Kp 2") },
				{ EKeys::NumPadThree, TEXT("Kp 3") }, { EKeys::NumPadFour, TEXT("Kp 4") }, { EKeys::NumPadFive, TEXT("Kp 5") },
				{ EKeys::NumPadSix, TEXT("Kp 6") }, { EKeys::NumPadSeven, TEXT("Kp 7") }, { EKeys::NumPadEight, TEXT("Kp 8") },
				{ EKeys::NumPadNine, TEXT("Kp 9") }, { EKeys::Add, TEXT("Kp Add") }, { EKeys::Subtract, TEXT("Kp Subtract") },
				{ EKeys::Multiply, TEXT("Kp Multiply") }, { EKeys::Divide, TEXT("Kp Divide") }, { EKeys::Decimal, TEXT("Kp Period") },
			};
			for (const FName2& N : Names)
			{
				if (N.Key == Key)
				{
					return N.Name;
				}
			}
			return nullptr;
		}

		int32 GTextureLoads = 0;
	}

	bool BindingFor(const FString& Action, Pc::PcFamily Family, FKey& Out)
	{
		// InputGlyphs.event_for: the FIRST of the action's events (the live map, Godot's events) that the family's device
		// presses. One Unreal has no key for (a pad's Guide button) draws nothing, as Godot has no picture for it: the next
		// event is not tried (measured: fixture `controls`).
		const Pc::PcBindDevice Device = Family != Pc::PcFamily::Keyboard ? Pc::PcBindDevice::Pad : Pc::PcBindDevice::Keyboard;
		if (const std::vector<Pc::PcInputEvent>* Events = Pc::BindingFind(PawInput::LiveState(), TCHAR_TO_UTF8(*Action)))
		{
			for (const Pc::PcInputEvent& E : *Events)
			{
				if (Pc::BindingDeviceOf(E) != Device)
				{
					continue;
				}
				int32 Sign = 0;
				const TArray<FKey> Keys = PawInput::KeysOfEvent(E, Sign);
				if (Keys.Num() == 0)
				{
					return false;
				}
				Out = Keys[0]; // an axis is its axis key, as the context maps it
				return true;
			}
		}
		return false;
	}

	FString StemFor(const FKey& Key, Pc::PcFamily Family)
	{
		const TCHAR* Position = nullptr;
		const TCHAR* InFolder = Folder(Family);
		if (Key.IsGamepadKey())
		{
			Position = PositionOf(PadPositions(), Key);
		}
		else
		{
			InFolder = TEXT("keyboard");
			Position = PositionOf(KeyboardPositions(), Key);
		}
		return Position ? FString::Printf(TEXT("input_%s_%s"), InFolder, Position) : FString();
	}

	FText KeyLabel(const FKey& Key)
	{
		if (const TCHAR* Named = NamedKey(Key))
		{
			return PawText::Ui(Named);
		}
		// A printable key is its CHARACTER ("-", "=", not "Hyphen"): Unreal's short display name of those keys.
		const FString Short = Key.GetDisplayName(false).ToString();
		if (Short.Len() == 1 && Short[0] > TEXT(' ') && Short[0] < 127)
		{
			return PawText::Verbatim(Short.ToUpper());
		}
		if (const TCHAR* Godot = GodotKeyName(Key))
		{
			return PawText::Verbatim(FString(Godot));
		}
		return Key.GetDisplayName(true);
	}

	UTexture2D* Texture(const FString& Stem)
	{
		if (Stem.IsEmpty())
		{
			return nullptr;
		}
		// Held for the engine's life, a stem with none included (review CR12: UPawUiAssets).
		UPawUiAssets* Held = UPawUiAssets::Get();
		if (Held)
		{
			if (const TObjectPtr<UTexture2D>* Found = Held->Glyphs.Find(Stem))
			{
				return *Found;
			}
		}
		const FString Name = TEXT("T_") + Stem;
		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *(FString(TextureDir) / Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		++GTextureLoads;
		if (Held)
		{
			Held->Glyphs.Add(Stem, Tex);
		}
		return Tex;
	}

	int32 TextureLoads()
	{
		return GTextureLoads;
	}

	FPrompt PromptFor(const FString& Action, Pc::PcFamily Family)
	{
		FPrompt P;
		FKey Key;
		if (!BindingFor(Action, Family, Key))
		{
			return P;
		}
		P.Stem = StemFor(Key, Family);
		if (Texture(P.Stem))
		{
			P.Kind = EKind::Glyph;
		}
		else if (!Key.IsGamepadKey() && !Key.IsMouseButton())
		{
			P.Kind = EKind::Keycap;
			P.Label = KeyLabel(Key);
		}
		else
		{
			P.Kind = EKind::Blank;
		}
		return P;
	}

	TArray<FPart> Parts(const FString& Template, Pc::PcFamily Family)
	{
		// \{([a-z_0-9]+)\}: only names that are actions; anything else is left as it is.
		TArray<FPart> Out;
		auto AddText = [&Out](const FString& S)
		{
			if (S.IsEmpty())
			{
				return;
			}
			if (Out.Num() > 0 && Out.Last().Kind == EKind::None)
			{
				Out.Last().Text += S;
			}
			else
			{
				Out.Add({ EKind::None, S });
			}
		};
		int32 At = 0;
		int32 I = 0;
		const int32 N = Template.Len();
		while (I < N)
		{
			if (Template[I] != TEXT('{'))
			{
				++I;
				continue;
			}
			int32 J = I + 1;
			while (J < N && (FChar::IsLower(Template[J]) || FChar::IsDigit(Template[J]) || Template[J] == TEXT('_'))
				&& Template[J] < 128)
			{
				++J;
			}
			if (J == I + 1 || J >= N || Template[J] != TEXT('}'))
			{
				++I;
				continue;
			}
			const FString Action = Template.Mid(I + 1, J - I - 1);
			if (!PawInput::HasAction(Action))
			{
				I = J + 1;
				continue;
			}
			AddText(Template.Mid(At, I - At));
			const FPrompt P = PromptFor(Action, Family);
			if (P.Kind == EKind::Glyph)
			{
				Out.Add({ EKind::Glyph, P.Stem });
			}
			else if (P.Kind == EKind::Keycap)
			{
				Out.Add({ EKind::Keycap, P.Label.ToString() });
			}
			I = At = J + 1;
		}
		AddText(Template.Mid(At));
		return Out;
	}

	FString Neutral(const TArray<FPart>& InParts)
	{
		FString S;
		for (const FPart& P : InParts)
		{
			switch (P.Kind)
			{
			case EKind::Glyph: S += TEXT("<g:") + P.Text + TEXT(">"); break;
			case EKind::Keycap: S += TEXT("<k:") + P.Text + TEXT(">"); break;
			default: S += P.Text; break;
			}
		}
		return S;
	}

	FString EscapeMarkup(const FString& Text)
	{
		FString S = Text.Replace(TEXT("&"), TEXT("&amp;"));
		S.ReplaceInline(TEXT("<"), TEXT("&lt;"));
		S.ReplaceInline(TEXT(">"), TEXT("&gt;"));
		S.ReplaceInline(TEXT("\""), TEXT("&quot;"));
		return S;
	}

	FString Markup(const TArray<FPart>& InParts, int32 IconPx)
	{
		FString S;
		for (const FPart& P : InParts)
		{
			switch (P.Kind)
			{
			case EKind::Glyph:
				S += FString::Printf(TEXT("<prompt g=\"%s\" px=\"%d\"/>"), *EscapeMarkup(P.Text), IconPx);
				break;
			case EKind::Keycap:
				S += FString::Printf(TEXT("<prompt k=\"%s\" px=\"%d\"/>"), *EscapeMarkup(P.Text), IconPx);
				break;
			default:
				S += EscapeMarkup(P.Text);
				break;
			}
		}
		return S;
	}
}
