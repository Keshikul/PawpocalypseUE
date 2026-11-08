#pragma once

// Every screen and piece is an editable Widget Blueprint (designer 2026-11-04; KB U/09 §2-2.1). Each stays a C++ class (its
// rules, its wiring, its tests) that can build its own layout in code; an editor-only baker (PawpocalypseEditor,
// UPawWidgetBaker) writes that layout ONCE into a Widget Blueprint child, WBP_<Name> under /Game/Pawpocalypse/UI/Widgets, and
// from then on THE WIDGET BLUEPRINT OWNS THE LAYOUT: it is never baked over (the designer's edits live there; a later layout
// fix is made in the asset). Button styles get the same: a Blueprint child BP_<Name> whose Class Defaults are the look.
//
// THE CONTRACT EVERY WIDGET CLASS FOLLOWS
//   Initialize(): BuildLayout() only when BuildsInCode(this) (a native class, not a Widget Blueprint), BEFORE Super::Initialize
//     (Common buttons wrap the content then); Super::Initialize (a Widget Blueprint's tree and its bound parts arrive here);
//     then, when ShouldWire(this), OnLayoutReady(): clicks, dynamic texts, state. Both modes run the same OnLayoutReady.
//   The parts code needs are UPROPERTY(meta=(BindWidgetOptional)) and the widgets carry those names (ConstructWidget's Name),
//     so the bake names them and the Widget Blueprint binds them. A repeated part is named <Prefix><n> and collected with
//     CollectNamed. Dynamic content (rows from data) stays code, added into a named container.
//   A piece's per-instance settings (a button's label, its size) are UPROPERTY(EditAnywhere) on the piece, applied in
//     SynchronizeProperties: a user widget's own tree is not saved inside its owner's asset, its properties are.
//   Pieces and screens are created through Resolve(), so the Widget Blueprint is used wherever it exists.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"

class UCommonButtonStyle;

namespace PawUi
{
	// Where the baked assets live, and their names: UPawMainMenu -> WBP_MainMenu; UPawButtonStyle -> BP_ButtonStyle.
	PAWPOCALYPSE_API const TCHAR* WidgetFolder();
	PAWPOCALYPSE_API FString AssetNameFor(const UClass* Native);
	PAWPOCALYPSE_API FString AssetPathFor(const UClass* Native); // the package: /Game/Pawpocalypse/UI/Widgets/WBP_MainMenu

	// The class to create for a native widget or button style class: its baked Blueprint when there is one (and the code layout
	// is not forced: paw.UI.CodeLayout 1), else the native class. Cached; the first ask for a class may load its asset.
	PAWPOCALYPSE_API UClass* Resolve(UClass* Native);
	template <class T>
	TSubclassOf<T> Resolve()
	{
		return Resolve(T::StaticClass());
	}
	// Forget the cache (the baker, after each asset; a test switching the code layout on or off).
	PAWPOCALYPSE_API void ResetResolver();
	PAWPOCALYPSE_API void SetCodeLayoutForced(bool bForced);
	PAWPOCALYPSE_API bool IsCodeLayoutForced();

	// The baker's flag: while set, OnLayoutReady does not run (the tree is saved as built, nothing wired or filled), and only
	// the class being baked builds its layout: a piece inside it stays bare, so only its properties go into the owner's asset.
	PAWPOCALYPSE_API bool IsBaking();
	PAWPOCALYPSE_API void SetBaking(bool bBaking, const UClass* Root = nullptr);

	// True for a native class, whose layout comes from code; false for a Widget Blueprint (its tree is the asset's).
	PAWPOCALYPSE_API bool BuildsInCode(const UUserWidget* Widget);
	// OnLayoutReady's gate: a live instance (not a class default, not the bake, not the designer's preview).
	PAWPOCALYPSE_API bool ShouldWire(const UUserWidget* Widget);

	// The widgets named <Prefix>0 .. <Prefix>(Count-1), in order (a missing one stays null: the parity test asserts none is).
	template <class T>
	void CollectNamed(const UWidgetTree* Tree, const TCHAR* Prefix, int32 Count, TArray<TObjectPtr<T>>& Out)
	{
		Out.Reset();
		for (int32 I = 0; I < Count; ++I)
		{
			Out.Add(Tree ? Cast<T>(Tree->FindWidget(FName(*FString::Printf(TEXT("%s%d"), Prefix, I)))) : nullptr);
		}
	}
	// Every class that has (or gets) a baked Blueprint, in bake order: the button styles, then the pieces (a piece used inside
	// another before it), then the screens. The baker writes them; the parity test checks each against its code layout.
	PAWPOCALYPSE_API TArray<UClass*> BakedClasses();
	// A repeated part's name: Name(TEXT("SlotCard"), 2) = "SlotCard2".
	PAWPOCALYPSE_API FName Numbered(const TCHAR* Prefix, int32 Index);
	// Every colour an object holds in its own properties (FLinearColor and FColor, inside structs and arrays of structs, e.g. a
	// text block's FSlateColor, a font's outline, a brush's tint; never through an object reference, never a Transient
	// property: not saved, so not the asset's look), as sRGB bytes with alpha.
	// Visit returns true to write its (changed) colour back; a linear colour keeps its own alpha. Returns how many it wrote.
	// The restyle's palette migration (tools/bake_widgets.py --recolour) and its test (Pawpocalypse.UI.Widgets.Palette).
	PAWPOCALYPSE_API int32 ForEachColour(UObject* Object, TFunctionRef<bool(FColor& Srgb)> Visit);
	// A colour's ROLE in a widget (restyle R3): one old colour may become several new ones, told apart by what it colours.
	// The plum flip turned ink TEXT bone and kept the ink surfaces; R3's shapes turn the ink EDGES of plates Deep Plum while
	// the plum panels (the same old value) stay (tools/bake_widgets.py --recolour; gen_paw_tokens.ui_role_pairs).
	enum class EPawColourRole : uint8
	{
		Surface, // a brush's fill, a shadow, a text's outline, an image's tint
		Text,    // under a text's colour (IsTextColour)
		Edge,    // a brush's outline (IsEdgeColour): a plate's or a button's frame
	};
	// The same walk, telling each colour's role.
	PAWPOCALYPSE_API int32 ForEachColourRole(UObject* Object, TFunctionRef<bool(FColor& Srgb, EPawColourRole Role)> Visit);
	// A property holding a text's colour: ColorAndOpacity on a text widget (UTextLayoutWidget) or an FTextBlockStyle, or a
	// piece's TextColour. Shadows and outlines are not (they stay with the surfaces).
	PAWPOCALYPSE_API bool IsTextColour(const UStruct* Owner, const FProperty* Property);
	// A property holding a brush's outline colour (FSlateBrushOutlineSettings::Color): an edge. A FONT's outline is not (the
	// ink outline of a label over the field stays a surface).
	PAWPOCALYPSE_API bool IsEdgeColour(const UStruct* Owner, const FProperty* Property);
	// Every font an object holds in its own properties (FSlateFontInfo, inside structs and arrays of structs: a text block's
	// font, a rich text's default style), the same walk. Visit returns true when it changed the font. Returns how many changed.
	// The restyle's font migration (tools/bake_widgets.py --refont) and its test (Pawpocalypse.UI.Widgets.Fonts).
	PAWPOCALYPSE_API int32 ForEachFont(UObject* Object, TFunctionRef<bool(FSlateFontInfo& Font)> Visit);
	// The game's font, F_Paw (tools/fonts.py; the composite import_fonts.py builds), loaded once. Null before its import.
	PAWPOCALYPSE_API class UFont* PawFont();
	// A font the restyle replaces: none, or one of the engine's (/Engine/..., Roboto: what UMG gave every text before F_Paw).
	PAWPOCALYPSE_API bool IsEngineFont(const UObject* FontObject);
	// R4b, THE TEXT FLOOR (docs/WORKLIST-RESTYLE-2026-11-07.md R4): nothing under 16 px at 1080p and 100 % UI size, so a
	// TV at 3 m reads it: a font at least PawUi::FontPt(16) = 12 pt, a piece's TextPx / LabelPx (Godot px) at least 16.
	// Exempt, by name: a keycap's label (UPawPromptIcon's KeyLabel: part of the glyph, sized to its cap).
	inline constexpr double TextFloorPx = 16.0;
	// What of Object is under the floor ("Name.Font 9.75 pt"); bRaise lifts each to the floor (the asset migration,
	// tools/bake_widgets.py --textfloor). Pawpocalypse.UI.Widgets.TextFloor walks every asset and code layout with it.
	PAWPOCALYPSE_API TArray<FString> UnderTextFloor(UObject* Object, bool bRaise);
	// The objects a baked asset saves its look in: a Widget Blueprint's tree widgets and its generated class's default object
	// (a style Blueprint: just the latter).
	PAWPOCALYPSE_API TArray<UObject*> LookObjects(UBlueprint* Asset);
}
