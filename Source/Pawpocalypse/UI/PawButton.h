#pragma once

// The game's one button (docs/05 §7): a Common button built in C++, its content made before Super::Initialize so the
// Common button wraps it (KB U/09 §3). The four states are the theme's nine-patch plates (phase 6 slice 5a, PawUi::Chrome),
// and focus (which is selection, docs/05 §6.1) is a ring in UI_FOCUS painted over them (NativePaint). A plate under the label lets a row warm up; an icon before it is Godot's Button.icon; a star in
// the top-left corner is the badge; a box before the label makes it a check box.
//
// LOCKED, not disabled: a row the player cannot press (unaffordable, MAX, "you are here") must still take the selection,
// because the player reads it (dock_navigator.gd: rows() includes disabled ones). Slate never focuses a disabled widget, so
// a locked button keeps Slate-enabled, turns Common UI's interaction off (presses ignored) and wears the locked style, which
// keeps the focus ring.
//
// A WIDGET BLUEPRINT (PawWidgets.h): WBP_Button is the baked layout, BP_ButtonStyle / BP_LockedButtonStyle / BP_FlatButtonStyle
// the looks (their Class Defaults). An instance's own settings (label, its size, wrap, alignment, flat) are properties, so a
// screen's Widget Blueprint keeps them and the designer edits them on the button.

#include "CoreMinimal.h"
#include "CommonButtonBase.h"

#include "PawButton.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class PAWPOCALYPSE_API UPawButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPawButtonStyle();
};

// A locked button: the disabled look in every state, the focus ring when selected.
UCLASS()
class PAWPOCALYPSE_API UPawLockedButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPawLockedButtonStyle();
};

// The bare look (phase 6 slice 5b): a TextureButton's, no plate in any state, no padding.
UCLASS()
class PAWPOCALYPSE_API UPawBareButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPawBareButtonStyle();
};

// A flat row (the Catches list: hud.gd's flat Button over each entry): no plate until hovered or selected, then the ring.
UCLASS()
class PAWPOCALYPSE_API UPawFlatButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UPawFlatButtonStyle();
};

UCLASS()
class PAWPOCALYPSE_API UPawButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPawButton(const FObjectInitializer& ObjectInitializer);
	virtual bool Initialize() override;
	virtual void SynchronizeProperties() override;

	// The instance's settings (the setters below write these; a screen's Widget Blueprint saves them).
	UPROPERTY(EditAnywhere, Category = "Paw")
	FText LabelText;
	UPROPERTY(EditAnywhere, Category = "Paw")
	float LabelPx = 16.0f;
	UPROPERTY(EditAnywhere, Category = "Paw")
	float WrapAt = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Paw")
	bool bAlignLeft = false;
	UPROPERTY(EditAnywhere, Category = "Paw")
	bool bFlatLook = false;

	void SetLabel(const FText& Text);
	FText GetLabel() const;
	// A fill drawn under the label (a tab's own colour); cleared with bShow = false.
	void SetPlate(bool bShow, const FLinearColor& Fill);
	// The badge: a star in the corner, with an optional count beside the label (Achievements).
	void SetBadge(bool bShow, int64 Count = 0);
	bool HasBadge() const { return bBadge; }
	void SetLabelSize(double GodotPx);
	// The label's colour while unlocked (a boss row is red, a cleared one Stardust); bOverride = false: the text colour.
	// bAlsoWhenLocked: the colour holds on a locked row too ("you are here" stays green: Godot's font_disabled_color).
	void SetLabelColour(bool bOverride, const FLinearColor& Colour, bool bAlsoWhenLocked = false);
	// Wraps the label at this width (0 = never): a fixed width, or a wrapped line overdraws the next (KB U/09 §3).
	void SetWrapAt(float Width);
	// Locked: selectable, never pressed, drawn disabled (see the class comment).
	void SetLocked(bool bInLocked);
	bool IsLocked() const { return bLockedState; }
	// A second line under the label, wrapped like it, in its own colour (an empty text hides it).
	void SetDetail(const FText& Text, const FLinearColor& Colour);
	FText GetDetail() const;
	// Left-aligned label and detail filling the row (a list entry), instead of a centred caption.
	void SetAlignLeft(bool bLeft);
	// The flat look (UPawFlatButtonStyle) while unlocked.
	void SetFlat(bool bInFlat);
	// The bare look (UPawBareButtonStyle): Godot's TextureButton, its icon and nothing else (the dock's quick bar).
	void SetBare(bool bInBare);
	UPROPERTY(EditAnywhere, Category = "Paw")
	bool bBareLook = false;
	// The game theme's look (not flat, not locked): what a screen's own buttons wear (the job picker's rows, OK).
	bool HasThemeLook() const { return !bFlatLook && !bLockedState; }
	// The icon before the label (Godot's Button.icon), Px square; an empty stem (or one with no art) hides it.
	void SetIcon(const FString& Stem, double Px);
	const FString& GetIconName() const { return IconName; }
	bool IsIconShown() const;
	// The theme's pressed plate held (a toggle Button's button_pressed: the open tab).
	void SetActiveLook(bool bActive);
	bool HasActiveLook() const { return bActiveLook; }
	// Whether the selection ring is painted: hovered, or focused by keys or a pad (RingRule).
	bool ShowsRing() const;
	static bool RingRule(bool bHovered, bool bFocused, bool bLastWasPointer);
	// A check box: the box before the label, ticked or not.
	void SetCheck(bool bShowBox, bool bTicked);
	bool IsTicked() const { return bTickedState; }

	// The shop's rows repeat while held (dock_navigator.gd: never a decision).
	bool bRepeats = false;
	// Free for the owner: which row or tab this is.
	int32 Index = INDEX_NONE;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void BuildLayout();
	void ApplySettings();
	void ApplyStyle();
	void ApplyLabelColour();
	void ApplyIcon();
	void ApplyLineLayout();
	FString IconName;
	float IconPx = 26.0f;
	bool bActiveLook = false;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Label;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Plate;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Star;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CheckBox;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage> Icon;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Detail;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UHorizontalBox> Line;
	UPROPERTY()
	TObjectPtr<class UOverlaySlot> LineSlot;
	bool bBadge = false;
	bool bLockedState = false;
	bool bTickedState = false;
	bool bColourOverride = false;
	bool bColourWhenLocked = false;
	FLinearColor OverrideColour = FLinearColor::Black;
};
