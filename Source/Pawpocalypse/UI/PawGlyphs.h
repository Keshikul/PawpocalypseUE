#pragma once

// What a button prompt shows for an action (Godot InputGlyphs, scripts/systems/input_glyphs.gd; docs/05 §5): a glyph image or
// a keycap, resolved by ACTION and FAMILY from the live bindings (PawInput: the mapping context), never baked into a screen,
// so a remap or a different pad changes every prompt with nothing else edited.
//
// PAD GLYPHS ARE NAMED BY POSITION: face_down is the bottom face button on every pad (A on an Xbox, Cross on a PlayStation);
// a family is a folder of textures (T_input_<folder>_<position> under /Game/Pawpocalypse/Art/UI/Input, from Godot's SVGs via
// tools/raster_svg.py + tools/import_art.py). KEYS ARE NOT IMAGES: a keycap is drawn around the key's label; only the four
// arrows and the mouse buttons are images (the font has no arrows).
//
// Ground truth: the real InputGlyphs, fixture `input_glyphs` (every action x family; every key's label; the {token}
// replacement of the game's templates): the engine test Pawpocalypse.Input.Glyphs.

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "PcDevice.h"

class UTexture2D;

namespace PawGlyphs
{
	// What one prompt is.
	enum class EKind : uint8 { None, Glyph, Keycap, Blank }; // Blank: bound, but nothing to draw (the mouse wheel)

	struct FPrompt
	{
		EKind Kind = EKind::None;
		FString Stem;  // input_xbox_face_down (a Glyph; also set for a stem with no texture)
		FText Label;   // a Keycap's label
	};

	PAWPOCALYPSE_API extern const TCHAR* const TextureDir; // /Game/Pawpocalypse/Art/UI/Input

	// InputGlyphs.event_for: the action's first binding the family's device can press (a pad family: pad buttons and axes; the
	// keyboard: keys and mouse buttons). False when it has none (a pad-only action on the keyboard): show nothing.
	PAWPOCALYPSE_API bool BindingFor(const FString& Action, Pc::PcFamily Family, FKey& Out);
	// InputGlyphs.glyph_stem: "input_xbox_face_down", "input_keyboard_mouse_left"; "" for a key drawn as a keycap.
	PAWPOCALYPSE_API FString StemFor(const FKey& Key, Pc::PcFamily Family);
	// InputGlyphs.key_label: a named key is a word from ST_UI (KEY_*), a printable key its character upper-cased, anything
	// else Godot's name for it.
	PAWPOCALYPSE_API FText KeyLabel(const FKey& Key);
	// The whole decision (PromptIcon.refresh): a glyph whose texture exists, else a keycap for a key, else blank.
	PAWPOCALYPSE_API FPrompt PromptFor(const FString& Action, Pc::PcFamily Family);
	// The texture of a stem (loaded once, then cached); null if there is none.
	PAWPOCALYPSE_API UTexture2D* Texture(const FString& Stem);

	// InputGlyphs.bbcode, as parts: the text with every {action} token replaced by that action's prompt. Only names that ARE
	// actions are replaced ("{n}" passes through); an action with nothing to show on this device leaves nothing.
	struct FPart
	{
		EKind Kind = EKind::None; // None: plain text
		FString Text;             // the text, the stem, or the keycap's label
	};
	PAWPOCALYPSE_API TArray<FPart> Parts(const FString& Template, Pc::PcFamily Family);
	// The test form (as the fixture neutralises Godot's BBCode): <g:stem> for a glyph, <k:label> for a keycap.
	PAWPOCALYPSE_API FString Neutral(const TArray<FPart>& Parts);
	// Rich text markup for UPawPromptText: the text escaped, a prompt as <prompt g="stem" px="22"/> or <prompt k="Space"/>.
	PAWPOCALYPSE_API FString Markup(const TArray<FPart>& Parts, int32 IconPx);
	PAWPOCALYPSE_API FString EscapeMarkup(const FString& Text);
}
