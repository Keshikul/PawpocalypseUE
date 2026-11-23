#include "UI/PawButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Components/Button.h"
#include "UI/PawIcons.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

namespace
{
	constexpr float Radius = 8.0f;
	constexpr float StarSize = 14.0f;
	constexpr float CheckSize = 26.0f;       // D18 (S4): the UI concept's toggle() (R3: 18)
	constexpr float CheckTickPx = 20.0f;     // its tick (the concept's check icon at .62 of 32)
	constexpr float CheckEdge = 2.5f;
	constexpr float CheckRadius = 7.0f;
	constexpr float IconGap = 8.0f; // currency_icons.gd decorate_button: h_separation 8
	constexpr float PriceTagInset = 10.0f; // D18 (S4): the price plate's gap to the row's right edge
	constexpr double PriceIconPx = 22.0;    // the leaf on it (the concept's .55 of the icon box)
}

// The theme's Button (phase 6 slice 5a): Godot's four nine-patch plates; the selection ring is painted over them
// (UPawButton::NativePaint), not part of a state.
UPawButtonStyle::UPawButtonStyle()
{
	using namespace PawUi;
	NormalBase = Chrome(EPawChrome::ButtonNormal);
	NormalHovered = Chrome(EPawChrome::ButtonHover);
	NormalPressed = Chrome(EPawChrome::ButtonPressed);
	// Selected = a toggle held down (SetActiveLook: the open tab): R4a's Butter plate, sunk like the pressed one, in every
	// state.
	SelectedBase = Chrome(EPawChrome::ButtonSelected);
	SelectedHovered = SelectedBase;
	SelectedPressed = SelectedBase;
	Disabled = Chrome(EPawChrome::ButtonDisabled);
	// theme Button content margins: 16 x 6; since R3's shapes the plate stands 4 px above its widget's bottom (its shadow),
	// so the label moves up 2 to sit on the plate's middle, the height unchanged.
	ButtonPadding = ButtonContentPadding();
	CustomPadding = FMargin(0.0f);
}

// A locked row is Godot's disabled Button: the disabled plate in every state (the ring shows the selection).
UPawLockedButtonStyle::UPawLockedButtonStyle()
{
	using namespace PawUi;
	NormalBase = Chrome(EPawChrome::ButtonDisabled);
	NormalHovered = NormalBase;
	NormalPressed = NormalBase;
	SelectedBase = NormalBase;
	SelectedHovered = NormalBase;
	SelectedPressed = NormalBase;
	Disabled = NormalBase;
	ButtonPadding = ButtonContentPadding();
	CustomPadding = FMargin(0.0f);
}

UPawBareButtonStyle::UPawBareButtonStyle()
{
	using namespace PawUi;
	NormalBase = NoBrush();
	NormalHovered = NoBrush();
	NormalPressed = NoBrush();
	SelectedBase = NoBrush();
	SelectedHovered = NoBrush();
	SelectedPressed = NoBrush();
	Disabled = NoBrush();
	ButtonPadding = FMargin(0.0f);
	CustomPadding = FMargin(0.0f);
}

UPawFlatButtonStyle::UPawFlatButtonStyle()
{
	using namespace PawUi;
	NormalBase = NoBrush();
	NormalHovered = Rounded(PawTokens::UI_BUTTON_HOVER, PawTokens::UI_BUTTON_HOVER, 0.0f, Radius);
	NormalPressed = NormalHovered;
	SelectedBase = NormalBase;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalHovered;
	Disabled = NormalBase;
	ButtonPadding = FMargin(10.0f, 4.0f);
	CustomPadding = FMargin(0.0f);
}

UPawButton::UPawButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The native look here; Initialize swaps in its Blueprint child (BP_ButtonStyle) once one is baked.
	SetStyle(UPawButtonStyle::StaticClass());
	LabelPx = static_cast<float>(PawUi::ButtonPx);
}

void UPawButton::BuildLayout()
{
	WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Plate"));
	Plate->SetBrush(PawUi::NoBrush());
	Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* PlateSlot = Root->AddChildToOverlay(Plate);
	PlateSlot->SetHorizontalAlignment(HAlign_Fill);
	PlateSlot->SetVerticalAlignment(VAlign_Fill);
	PlateSlot->SetPadding(PawUi::ButtonFacePadding()); // under the label, the plate's face exactly (R4a: inside its edge)

	Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Line"));
	Line->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* LS = Root->AddChildToOverlay(Line);
	LS->SetHorizontalAlignment(HAlign_Center);
	LS->SetVerticalAlignment(VAlign_Center);
	CheckBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CheckBox"));
	CheckBox->SetPadding(FMargin((CheckSize - CheckTickPx) * 0.5f));
	CheckBox->SetVisibility(ESlateVisibility::Collapsed);
	// D18 (S4): the tick inside (the concept's check icon); hidden, keeping its place, while unticked.
	CheckTick = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CheckTick"));
	CheckTick->SetBrush(PawUi::IconBrush(TEXT("ui_tick"), CheckTickPx));
	CheckTick->SetVisibility(ESlateVisibility::Hidden);
	CheckBox->SetContent(CheckTick);
	UHorizontalBoxSlot* CheckSlot = Line->AddChildToHorizontalBox(CheckBox);
	CheckSlot->SetVerticalAlignment(VAlign_Center);
	CheckSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	// The icon (Godot's Button.icon: a currency beside a price, a tab's picture; phase 6 slice 5b), collapsed until set.
	Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
	Icon->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* IconSlot = Line->AddChildToHorizontalBox(Icon);
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetPadding(FMargin(0.0f, 0.0f, IconGap, 0.0f));
	// The label, and under it the detail line (collapsed until a row sets one).
	UVerticalBox* Texts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Texts"));
	UHorizontalBoxSlot* TextsSlot = Line->AddChildToHorizontalBox(Texts);
	TextsSlot->SetVerticalAlignment(VAlign_Center);
	Label = PawUi::MakeText(WidgetTree, PawUi::ButtonPx, PawTokens::UI_TEXT, false, TEXT("Label"));
	Label->SetJustification(ETextJustify::Center);
	Texts->AddChildToVerticalBox(Label);
	Detail = PawUi::MakeText(WidgetTree, PawUi::LabelPx, PawTokens::UI_TEXT, false, TEXT("Detail")); // a Label under the heading (hud.gd)
	Detail->SetVisibility(ESlateVisibility::Collapsed);
	Texts->AddChildToVerticalBox(Detail);
	CountText = PawUi::MakeText(WidgetTree, PawUi::ButtonPx, PawTokens::UI_TEXT, false, TEXT("CountText"));
	CountText->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* CountSlot = Line->AddChildToHorizontalBox(CountText);
	CountSlot->SetVerticalAlignment(VAlign_Center);
	CountSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));

	// D18 (S4): the price on its own plate at a row's right end (the dock's shop), collapsed until a row sets one.
	PriceTag = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PriceTag"));
	PriceTag->SetBrush(PawUi::Chrome(PawUi::EPawChrome::ButtonPrimary));
	PriceTag->SetPadding(PawUi::PlatePadding(FMargin(10.0f, 1.0f, 12.0f, 1.0f), PawUi::EPawChrome::ButtonPrimary));
	PriceTag->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* TagSlot = Root->AddChildToOverlay(PriceTag);
	TagSlot->SetHorizontalAlignment(HAlign_Right);
	TagSlot->SetVerticalAlignment(VAlign_Center);
	TagSlot->SetPadding(FMargin(0.0f, 0.0f, PriceTagInset, 0.0f));
	UHorizontalBox* TagLine = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PriceLine"));
	PriceTag->SetContent(TagLine);
	PriceIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PriceIcon"));
	PriceIcon->SetVisibility(ESlateVisibility::Collapsed);
	UHorizontalBoxSlot* PriceIconSlot = TagLine->AddChildToHorizontalBox(PriceIcon);
	PriceIconSlot->SetVerticalAlignment(VAlign_Center);
	PriceIconSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	PriceText = PawUi::MakeText(WidgetTree, PawUi::ButtonPx, PawTokens::UI_TEXT, false, TEXT("PriceText"));
	TagLine->AddChildToHorizontalBox(PriceText)->SetVerticalAlignment(VAlign_Center);

	// The star sits ON the top-left border (hud.gd _set_tab_star: offset 1, -4). A drawn disc until the badge art
	// arrives with phase 6 (the font has no star glyph).
	Star = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Star"));
	Star->SetBrush(PawUi::Rounded(PawTokens::ALERT, PawTokens::INK, 2.0f, StarSize * 0.5f));
	Star->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* StarSlot = Root->AddChildToOverlay(Star);
	StarSlot->SetHorizontalAlignment(HAlign_Left);
	StarSlot->SetVerticalAlignment(VAlign_Top);
	StarSlot->SetPadding(FMargin(-15.0f, -10.0f, 0.0f, 0.0f));
	Star->SetPadding(FMargin(StarSize * 0.5f));
}

bool UPawButton::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		BuildLayout();
	}
	const bool bOk = Super::Initialize();
	LineSlot = Line ? Cast<UOverlaySlot>(Line->Slot) : nullptr;
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		ApplyStyle();
		ApplySettings();
		ApplyIcon();
	}
	if (bOk && PawUi::ShouldWire(this))
	{
		PawUi::UseAtlasIcons(this); // the asset's saved icon brushes: the atlas's cells (one batch)
	}
	return bOk;
}

int32 UPawButton::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	// The selection ring (SBF_focus) over whatever plate the state draws, FocusRingExpand px outside it. Common UI's hovered
	// state is the selection (a pad's focus, a mouse over it): docs/05 §6.1.
	if (ShowsRing())
	{
		static const FSlateBrush Ring = PawUi::FocusRing();
		const FVector2f E(PawUi::FocusRingExpand, PawUi::FocusRingExpand);
		FSlateDrawElement::MakeBox(OutDrawElements, ++Layer,
			AllottedGeometry.ToPaintGeometry(AllottedGeometry.GetLocalSize() + 2.0f * E, FSlateLayoutTransform(-E)), &Ring,
			ESlateDrawEffect::None, Ring.GetTint(InWidgetStyle)); // the brush's own (clear) fill: MakeBox does not apply it
	}
	return Layer;
}

bool UPawButton::ShowsRing() const
{
	const UGameInstance* GI = GetGameInstance();
	const UPawInputDeviceSubsystem* Device = GI ? GI->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
	return RingRule(IsHovered(), HasAnyUserFocus() || HasFocusedDescendants(), Device && Device->LastWasPointer());
}

bool UPawButton::RingRule(bool bHovered, bool bFocused, bool bLastWasPointer)
{
	// Godot rings the FOCUSED control and hides focus the mouse gave (4.6+); a locked row takes focus but never Common UI's
	// hover (its interaction is off), so focus is what shows a pad where it is. Hover rings too: docs/05 §6.1.
	return bHovered || (bFocused && !bLastWasPointer);
}

void UPawButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplySettings();
}

void UPawButton::ApplySettings()
{
	if (Label && !Label->GetText().EqualTo(LabelText))
	{
		Label->SetText(LabelText);
	}
	if (Label)
	{
		FSlateFontInfo Font = Label->GetFont();
		if (Font.Size != PawUi::FontPt(LabelPx))
		{
			Font.Size = PawUi::FontPt(LabelPx);
			Label->SetFont(Font);
		}
	}
	for (UTextBlock* T : { Label.Get(), Detail.Get() })
	{
		if (T)
		{
			T->SetJustification(bAlignLeft ? ETextJustify::Left : ETextJustify::Center);
		}
	}
	ApplyLineLayout();
}

void UPawButton::ApplyLineLayout()
{
	// Godot's Button: icon_alignment LEFT by default (the icon at the content's left edge, the text centred in what is left);
	// an icon-only button (a tab, icon_alignment CENTER) and a plain caption centre the line; a list entry runs left.
	const bool bIconBesideWords = IsIconShown() && !LabelText.IsEmpty() && !bAlignLeft;
	if (LineSlot)
	{
		LineSlot->SetHorizontalAlignment(bAlignLeft ? HAlign_Left : bIconBesideWords ? HAlign_Fill : HAlign_Center);
	}
	if (UHorizontalBoxSlot* TextsSlot = Label && Label->GetParent() ? Cast<UHorizontalBoxSlot>(Label->GetParent()->Slot) : nullptr)
	{
		TextsSlot->SetSize(FSlateChildSize(bIconBesideWords ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
	}
	// WrapAt is the whole line inside the button: words beside an icon (or its kept place) get what the icon and its gap leave
	// (the language pass, 2026-11-20: a Catches row's card and a Realms row ran past the dock's edge in ten languages).
	const bool bIconSlot = Icon && Icon->GetVisibility() != ESlateVisibility::Collapsed && !LabelText.IsEmpty();
	const float Wrap = WrapAt > 0.0f ? FMath::Max(1.0f, WrapAt - (bIconSlot ? IconPx + IconGap : 0.0f)) : 0.0f;
	for (UTextBlock* T : { Label.Get(), Detail.Get() })
	{
		if (T && Wrap > 0.0f)
		{
			T->SetAutoWrapText(false);
			T->SetWrapTextAt(Wrap);
		}
	}
}

void UPawButton::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();
	// Common UI builds the button's style from its style class (the plates' own textures) and has just applied it to the
	// button it wraps the tree in (the tree's root): the same style, its plates the UI atlas's cells.
	UButton* Root = WidgetTree ? Cast<UButton>(WidgetTree->RootWidget) : nullptr;
	if (Root && PawUi::ShouldWire(this))
	{
		FButtonStyle Built = Root->GetStyle();
		bool bChanged = false;
		for (FSlateBrush* B : { &Built.Normal, &Built.Hovered, &Built.Pressed, &Built.Disabled })
		{
			bChanged |= PawUi::AtlasBrush(*B);
		}
		if (bChanged)
		{
			Root->SetStyle(Built);
		}
	}
}

void UPawButton::ApplyStyle()
{
	// The look in play: locked over flat over the theme's; each through its Blueprint child when one is baked.
	UClass* Native = bBareLook ? UPawBareButtonStyle::StaticClass() : bLockedState ? UPawLockedButtonStyle::StaticClass()
		: bFlatLook ? UPawFlatButtonStyle::StaticClass() : UPawButtonStyle::StaticClass();
	// A style the designer chose on the instance (not one of the three) is theirs: leave it.
	const UClass* Current = GetStyle() ? GetStyle()->GetClass() : nullptr;
	const bool bOurs = !Current || Current->IsChildOf(UPawButtonStyle::StaticClass()) || Current->IsChildOf(UPawLockedButtonStyle::StaticClass())
		|| Current->IsChildOf(UPawFlatButtonStyle::StaticClass()) || Current->IsChildOf(UPawBareButtonStyle::StaticClass());
	if (bOurs)
	{
		SetStyle(PawUi::Resolve(Native));
	}
}

void UPawButton::SetLabel(const FText& Text)
{
	LabelText = Text;
	if (Label && !Label->GetText().EqualTo(Text))
	{
		Label->SetText(Text);
	}
	ApplyIcon();
}

void UPawButton::SetIcon(const FString& Stem, double Px)
{
	IconName = Stem;
	IconPx = static_cast<float>(Px);
	ApplyIcon();
}

void UPawButton::ApplyIcon()
{
	if (!Icon)
	{
		return;
	}
	const FSlateBrush Brush = PawUi::IconBrush(IconName, IconPx);
	const bool bShow = !IconName.IsEmpty() && Brush.GetResourceObject() != nullptr;
	// A kept place (a list whose rows line up): an empty picture of the icon's size, drawn as nothing.
	Icon->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : bIconSlotKept ? ESlateVisibility::Hidden : ESlateVisibility::Collapsed);
	if (bShow)
	{
		Icon->SetBrush(Brush);
	}
	else if (bIconSlotKept)
	{
		FSlateBrush Empty = PawUi::NoBrush();
		Empty.ImageSize = FVector2D(IconPx, IconPx);
		Icon->SetBrush(Empty);
	}
	// An icon-only button (a tab) centres its picture: no gap after it.
	if (UHorizontalBoxSlot* S = Cast<UHorizontalBoxSlot>(Icon->Slot))
	{
		S->SetPadding(FMargin(0.0f, 0.0f, LabelText.IsEmpty() ? 0.0f : IconGap, 0.0f));
	}
	ApplyLineLayout();
}

bool UPawButton::IsIconShown() const
{
	// A picture drawn: not a kept, empty place (Hidden: SetIconSlotKept with no art), which this once counted as shown.
	return Icon && Icon->GetVisibility() != ESlateVisibility::Collapsed && Icon->GetVisibility() != ESlateVisibility::Hidden;
}

void UPawButton::SetActiveLook(bool bActive)
{
	// A toggle Button's button_pressed: the theme's pressed plate while it stays down (the open tab).
	if (bActiveLook == bActive)
	{
		return;
	}
	bActiveLook = bActive;
	ApplyLabelColour(); // R4a: ink on the Butter plate
	// Still pressable and hoverable while held down (Common UI would switch a selected button's interaction off).
	SetIsInteractableWhenSelected(true);
	SetIsSelectable(bActive);
	if (bActive)
	{
		SetIsSelected(true, false);
	}
}

FText UPawButton::GetLabel() const
{
	return Label ? Label->GetText() : FText::GetEmpty();
}

void UPawButton::SetLabelSize(double GodotPx)
{
	LabelPx = static_cast<float>(GodotPx);
	ApplySettings();
}

void UPawButton::SetLabelColour(bool bOverride, const FLinearColor& Colour, bool bAlsoWhenLocked)
{
	if (bColourOverride == bOverride && bColourWhenLocked == bAlsoWhenLocked && (!bOverride || OverrideColour == Colour))
	{
		return;
	}
	bColourOverride = bOverride;
	bColourWhenLocked = bAlsoWhenLocked;
	OverrideColour = Colour;
	ApplyLabelColour();
}

float UPawButton::WidthFor(const FText& Text) const
{
	float Words = 0.0f;
	if (Label && FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
	{
		Words = static_cast<float>(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Label->GetFont()).X);
	}
	const FMargin Pad = PawUi::ButtonContentPadding();
	const bool bIconSlot = Icon && Icon->GetVisibility() != ESlateVisibility::Collapsed && !Text.IsEmpty();
	return Pad.Left + Pad.Right + Words + (bIconSlot ? IconPx + IconGap : 0.0f);
}

void UPawButton::SetWrapAt(float Width)
{
	WrapAt = Width;
	ApplySettings();
}

void UPawButton::SetIconSlotKept(bool bKept)
{
	bIconSlotKept = bKept;
	ApplyIcon();
}

void UPawButton::SetPriceTag(bool bShow, const FText& Text, bool bCanPay, bool bLeaf)
{
	if (!PriceTag)
	{
		return;
	}
	PriceTag->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bShow)
	{
		return;
	}
	if (!bTagStyled || bTagPayable != bCanPay)
	{
		// The raised yellow when it can be paid; the off plate, its words and its leaf faded, when not (the concept's).
		PriceTag->SetBrush(PawUi::Chrome(bCanPay ? PawUi::EPawChrome::ButtonPrimary : PawUi::EPawChrome::ButtonDisabled));
		bTagPayable = bCanPay;
		bTagStyled = true;
	}
	if (PriceText)
	{
		if (!PriceText->GetText().EqualTo(Text))
		{
			PriceText->SetText(Text);
		}
		PawUi::SetTextColour(PriceText, bCanPay ? PawTokens::UI_TEXT : PawTokens::UI_TEXT_DISABLED);
	}
	if (PriceIcon)
	{
		const FSlateBrush Leaf = bLeaf ? PawUi::IconBrush(TEXT("ui_icon_catnip"), PriceIconPx) : FSlateBrush();
		const bool bShowLeaf = bLeaf && Leaf.GetResourceObject() != nullptr;
		PriceIcon->SetVisibility(bShowLeaf ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShowLeaf)
		{
			PriceIcon->SetBrush(Leaf);
		}
		PriceIcon->SetRenderOpacity(bCanPay ? 1.0f : 0.5f);
	}
}

bool UPawButton::IsPriceTagShown() const
{
	return PriceTag && PriceTag->GetVisibility() != ESlateVisibility::Collapsed;
}

FText UPawButton::GetPriceTag() const
{
	return IsPriceTagShown() && PriceText ? PriceText->GetText() : FText::GetEmpty();
}

void UPawButton::SetDetail(const FText& Text, const FLinearColor& Colour)
{
	if (!Detail)
	{
		return;
	}
	const bool bShow = !Text.IsEmpty();
	Detail->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Detail->GetText().EqualTo(Text))
	{
		Detail->SetText(Text);
	}
	PawUi::SetTextColour(Detail, Colour);
}

FText UPawButton::GetDetail() const
{
	return Detail && Detail->GetVisibility() != ESlateVisibility::Collapsed ? Detail->GetText() : FText::GetEmpty();
}

void UPawButton::SetAlignLeft(bool bLeft)
{
	bAlignLeft = bLeft;
	ApplySettings();
}

void UPawButton::SetFlat(bool bInFlat)
{
	bFlatLook = bInFlat;
	if (!bLockedState)
	{
		ApplyStyle();
	}
}

void UPawButton::SetBare(bool bInBare)
{
	bBareLook = bInBare;
	ApplyStyle();
}

void UPawButton::SetLocked(bool bInLocked)
{
	if (bLockedState == bInLocked)
	{
		return;
	}
	bLockedState = bInLocked;
	SetIsInteractionEnabled(!bInLocked);
	ApplyStyle();
	ApplyLabelColour();
}

void UPawButton::ApplyLabelColour()
{
	// A locked row reads disabled; a coloured one keeps its colour when it can be pressed.
	// R4a: on the Butter plate (the open tab, a call) every word is ink, whatever colour the row asked for.
	const bool bOnSelected = bActiveLook || bCallLook;
	const bool bOverrideShows = bColourOverride && (!bLockedState || bColourWhenLocked);
	PawUi::SetTextColour(Label, bOnSelected ? PawTokens::UI_SELECTED_TEXT
		: bOverrideShows ? OverrideColour : bLockedState ? PawTokens::UI_TEXT_DISABLED : PawTokens::UI_TEXT);
	PawUi::SetTextColour(CountText, bOnSelected ? PawTokens::UI_SELECTED_TEXT : PawTokens::UI_TEXT);
}

void UPawButton::SetCheck(bool bShowBox, bool bTicked)
{
	bTickedState = bTicked;
	if (CheckBox)
	{
		CheckBox->SetVisibility(bShowBox ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// D18 (S4, the UI concept's toggle()): yellow with the tick when on, cream when off, on an ink edge.
		CheckBox->SetBrush(PawUi::Rounded(bTicked ? PawTokens::UI_SELECTED : PawTokens::UI_PANEL, PawTokens::INK, CheckEdge, CheckRadius));
	}
	if (CheckTick)
	{
		CheckTick->SetVisibility(bTicked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

FLinearColor UPawButton::GetCheckFill() const
{
	return CheckBox ? CheckBox->Background.TintColor.GetSpecifiedColor() : FLinearColor::Transparent;
}

bool UPawButton::IsTickShown() const
{
	return CheckTick && CheckTick->GetVisibility() != ESlateVisibility::Hidden && CheckTick->GetVisibility() != ESlateVisibility::Collapsed
		&& CheckTick->GetBrush().GetResourceObject() != nullptr;
}

void UPawButton::SetPlate(bool bShow, const FLinearColor& Fill)
{
	if (Plate)
	{
		Plate->SetBrush(bShow ? PawUi::Rounded(Fill, Fill, 0.0f, PawUi::ButtonFaceRadius) : PawUi::NoBrush());
	}
}

void UPawButton::SetCallLook(bool bCall)
{
	if (bCallLook == bCall)
	{
		return;
	}
	bCallLook = bCall;
	SetPlate(bCall, PawTokens::UI_SELECTED);
	ApplyLabelColour();
}

void UPawButton::SetBadge(bool bShow, int64 Count)
{
	bBadge = bShow;
	if (Star)
	{
		Star->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (CountText)
	{
		CountText->SetVisibility(Count > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Count > 0)
		{
			CountText->SetText(PawText::Count(Count));
		}
	}
}
