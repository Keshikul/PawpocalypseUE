#pragma once

// Button prompts (docs/05 §5; Godot PromptIcon, PromptLabel): what an action is pressed with, drawn for the device in hand.
//   UPawPromptIcon (WBP_PromptIcon): ONE action's prompt: a glyph (a pad or mouse button, an arrow key) or a keycap drawn at
//     run time around the key's label (paper, ink frame, a deeper bottom edge: what makes a rectangle read as a KEY).
//   UPawPromptText (WBP_PromptText): text with prompts inline ("{press} Buy"): a TRANSLATED template whose {action} tokens
//     become glyphs or keycaps (a slot-coloured cap, as Godot's BBCode draws one). Hidden whole with Settings > Controls >
//     Show button prompts off: its words are captions for the glyphs.
// Both redraw on a device family change (UPawInputDeviceSubsystem::OnFamilyChanged) and on a settings change (the Button
// prompts style, Show button prompts; the language with phase 7, a remap with slice 4). Neither takes the mouse.
//
// WIDGET BLUEPRINTS (PawWidgets.h): the per-instance settings (action, icon size, text size and colour) are properties,
// applied in SynchronizeProperties, so a screen's asset keeps them.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlockDecorator.h"

#include "UI/PawGlyphs.h"

#include "PawPrompts.generated.h"

class UBorder;
class UImage;
class URichTextBlock;
class USizeBox;
class UTextBlock;
class UTexture2D;

namespace PawPrompt
{
	// The family a prompt draws: the override when set (>= 0), else the device in hand (the keyboard without a game).
	PAWPOCALYPSE_API Pc::PcFamily FamilyFor(const UUserWidget* W, int32 Override);
	// Settings > Controls > Show button prompts (on without a game).
	PAWPOCALYPSE_API bool PromptsShown(const UUserWidget* W);
}

UCLASS()
class PAWPOCALYPSE_API UPawPromptIcon : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void SynchronizeProperties() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// The Godot action ("press"). Lower-case, as Godot names it.
	UPROPERTY(EditAnywhere, Category = "Paw")
	FString Action;
	// The glyph's height in pixels; a keycap matches it (Godot icon_px).
	UPROPERTY(EditAnywhere, Category = "Paw")
	int32 IconPx = 28;
	// Shown even with Show button prompts off (the Controls page's cells ARE the bindings).
	UPROPERTY(EditAnywhere, Category = "Paw")
	bool bAlwaysShow = false;
	// Draw this family whatever is in hand (0 keyboard .. 4 generic); -1 follows the device.
	UPROPERTY(EditAnywhere, Category = "Paw")
	int32 FamilyOverride = -1;

	void SetAction(const FString& InAction);
	void SetIconPx(int32 Px);
	void SetFamilyOverride(int32 Family);
	void Refresh();

	// What it shows (the gates; a caller hiding a row whose action has no binding on this device).
	bool ShowsAnything() const { return Shown.Kind == PawGlyphs::EKind::Glyph || Shown.Kind == PawGlyphs::EKind::Keycap; }
	const PawGlyphs::FPrompt& GetShown() const { return Shown; }
	// The size it takes (glyph px square, or the keycap's): a prompt placed by hand beside something is placed by this.
	FVector2D GetPromptSize() const;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Glyph;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> KeySize;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Keycap;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> KeyFace;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KeyLabel;

	PawGlyphs::FPrompt Shown;
	FDelegateHandle FamilyHandle;
	FDelegateHandle SettingsHandle;
};

UCLASS()
class PAWPOCALYPSE_API UPawPromptText : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual void SynchronizeProperties() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// The prompts' height in pixels (Godot icon_px).
	UPROPERTY(EditAnywhere, Category = "Paw")
	int32 IconPx = 28;
	// The words' size in Godot pixels and their colour.
	UPROPERTY(EditAnywhere, Category = "Paw")
	float TextPx = 17.0f;
	UPROPERTY(EditAnywhere, Category = "Paw")
	FLinearColor TextColour = FLinearColor::Black;
	UPROPERTY(EditAnywhere, Category = "Paw")
	bool bCentred = true;
	// A legend (PromptLabel) hides with Show button prompts off; a tooltip's line (InputGlyphs.bbcode) never does.
	UPROPERTY(EditAnywhere, Category = "Paw")
	bool bHideWithPrompts = true;
	// Wrap at this width (0: never), as a tooltip's lines do.
	UPROPERTY(EditAnywhere, Category = "Paw")
	float WrapAt = 0.0f;

	// The translated template ("{press} Buy"). Fill ordinary placeholders BEFORE setting it.
	void SetTemplate(const FText& InTemplate);
	const FText& GetTemplate() const { return Template; }
	void SetTextColour(const FLinearColor& Colour);
	void Refresh();
	// The test form of what it shows (PawGlyphs::Neutral), and the markup handed to the rich text.
	const FString& GetNeutral() const { return Neutral; }
	FString GetMarkup() const;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URichTextBlock> Rich;

	FText Template;
	FString Neutral;
	FDelegateHandle FamilyHandle;
	FDelegateHandle SettingsHandle;
	void ApplyStyle();
};

// The rich text decorator behind UPawPromptText: <prompt g="stem" px="22"/> is a glyph, <prompt k="Space" px="22"/> a keycap.
UCLASS()
class PAWPOCALYPSE_API UPawPromptDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;

	// The brush for a stem at a size, made once and kept with its texture (a brush does not hold its texture alive).
	const FSlateBrush* BrushFor(const FString& Stem, int32 Px);
	const FSlateBrush* KeycapBrush();

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> Textures;
	TMap<FString, TSharedPtr<FSlateBrush>> Brushes;
	TSharedPtr<FSlateBrush> Keycap;
};
