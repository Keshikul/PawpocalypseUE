#pragma once

// The UI's motion on one screen (phase 6 slice 5c): Godot's UiJuice (scripts/systems/ui_juice.gd) over Pc::PcJuice. A screen
// owns one FPawJuice, wires the controls Godot wires (Hover, Punch, PopIn, Jitter at the same call sites) and ticks it every
// frame; the motion is applied as the widget's render transform about its centre, so it never changes layout (UiJuice's
// first rule). Hover owns the scale, jitter the rotation, as in Godot. The effects setting turns the idle jitter off; hover
// and press stay (they answer the player's own input).
//
// The hover is the MOUSE's (Godot: mouse_entered): Common UI fires OnHovered only for the pointer and only while the button
// is enabled and interactable, which is _on_enter's disabled check. Pad and keys move the selection ring instead (5a).

#include "CoreMinimal.h"
#include "PcJuice.h"

class UCommonButtonBase;
class UWidget;

class PAWPOCALYPSE_API FPawJuice
{
public:
	// UiJuice.hover(): the hover pop and the press squash, bound to the button's events for as long as Owner lives.
	// Idempotent (a pooled row wired twice is wired once).
	void Hover(UCommonButtonBase* Button, UObject* Owner);
	// Godot's `disabled` where the port keeps a button interactable (the tree's nodes stay focusable for their tips): a
	// disabled button does not grow on hover, and its release settles to rest.
	void SetHoverOff(UCommonButtonBase* Button, bool bOff);
	// UiJuice.punch(): the purchase squash-and-stretch.
	void Punch(UWidget* W);
	// UiJuice.pop_in(): a just-shown panel scales up from POP_IN_FROM.
	void PopIn(UWidget* W);
	// UiJuice.jitter(): start or stop the idle lean (off eases back to upright). bEffects: the effects setting.
	void Jitter(UWidget* W, bool bOn, bool bEffects);
	// One frame: every moving widget steps and wears its new transform; a widget come to rest is let go.
	void Tick(double DeltaSeconds);

	// The handlers Hover binds (the tests call them: no pointer in a headless run).
	void HandleHovered(UCommonButtonBase* B);
	void HandleUnhovered(UCommonButtonBase* B);
	void HandlePressed(UCommonButtonBase* B);
	void HandleReleased(UCommonButtonBase* B);

	// For the tests: W's motion (null when it is at rest and untouched), and whether it leans.
	const Pc::PcJuice* Find(const UWidget* W) const;
	bool IsJittering(const UWidget* W) const;
	bool IsHoverWired(const UCommonButtonBase* B) const;

	// The effects setting, read from W's game instance (true without one).
	static bool EffectsOn(const UWidget* W);
	// is_visible_in_tree(): W and every panel and widget above it shown.
	static bool IsShownInTree(const UWidget* W);

private:
	Pc::PcJuice& At(UWidget* W);
	bool Disabled(const UCommonButtonBase* B) const;
	static void Apply(UWidget* W, const Pc::PcJuice& J);

	TMap<TWeakObjectPtr<UWidget>, Pc::PcJuice> Moving;
	TSet<TWeakObjectPtr<UCommonButtonBase>> Wired;
	TSet<TWeakObjectPtr<UCommonButtonBase>> HoverOff;
	uint64 NextSeed = 0x5C5C;
};
