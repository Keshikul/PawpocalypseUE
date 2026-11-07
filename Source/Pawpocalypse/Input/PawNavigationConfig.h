#pragma once

// Godot's GUI navigation (its ui_up / ui_down / ui_left / ui_right / ui_accept / ui_cancel actions, as the project overrides
// them) for Slate (docs/05 §3): the keys that move the selection, press it and back out are the map's, not Slate's defaults.
// Godot adds W / A / S / D to the arrows and the D-pad, and keypad Enter to Enter / Space / A; Slate alone has neither. The
// left stick past the ui_* dead zone navigates, as Slate's analog navigation does. Tab keeps moving the selection (Godot's
// built-in ui_focus_next).
//
// Read from the live bindings (PawInput::KeysOf), so a remap (slice 4) reaches the menus with a new config. Installed by the
// game instance in a game (FSlateApplication::SetNavigationConfig), never in the editor: it would change the editor's own UI.

#include "CoreMinimal.h"
#include "Framework/Application/NavigationConfig.h"

class PAWPOCALYPSE_API FPawNavigationConfig : public FNavigationConfig
{
public:
	FPawNavigationConfig();

	// What a key does in a menu (the tests read it): a direction, or Invalid.
	EUINavigation DirectionOf(const FKey& Key) const;
	// Accept, Back, or Invalid.
	EUINavigationAction ActionOf(const FKey& Key) const;
	float AnalogThreshold() const { return AnalogNavigationVerticalThreshold; }
	FKey AnalogX() const { return AnalogHorizontalKey; }
	FKey AnalogY() const { return AnalogVerticalKey; }
};
