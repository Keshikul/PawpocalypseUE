#include "UI/PawPrompts.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Framework/Text/SlateTextRun.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	// prompt_icon.gd _keycap: paper, an ink frame of 2 with a 4 bottom edge, corners 5, the label 6 from either side, its
	// font half the icon (at least 10).
	constexpr float CapFrame = 2.0f;
	constexpr float CapFrameBottom = 4.0f;
	constexpr float CapRadius = 5.0f;
	constexpr float CapMarginX = 6.0f;
	constexpr int32 CapMinLabelPx = 10;

	UPawInputDeviceSubsystem* DeviceOf(const UUserWidget* W)
	{
		const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
	}

	UPawSettingsSubsystem* SettingsOf(const UUserWidget* W)
	{
		const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	}

	template <class T>
	void Listen(T* Widget, FDelegateHandle& FamilyHandle, FDelegateHandle& SettingsHandle)
	{
		if (UPawInputDeviceSubsystem* D = DeviceOf(Widget); D && !FamilyHandle.IsValid())
		{
			FamilyHandle = D->OnFamilyChanged.AddWeakLambda(Widget, [Widget](Pc::PcFamily) { Widget->Refresh(); });
		}
		// The settings as the device subsystem has applied them (the bindings first: a remap must reach the map before a prompt
		// reads it).
		if (UPawInputDeviceSubsystem* D = DeviceOf(Widget); D && !SettingsHandle.IsValid())
		{
			SettingsHandle = D->OnControlsChanged.AddWeakLambda(Widget, [Widget]() { Widget->Refresh(); });
		}
	}

	template <class T>
	void Unlisten(T* Widget, FDelegateHandle& FamilyHandle, FDelegateHandle& SettingsHandle)
	{
		if (UPawInputDeviceSubsystem* D = DeviceOf(Widget))
		{
			D->OnFamilyChanged.Remove(FamilyHandle);
			D->OnControlsChanged.Remove(SettingsHandle);
		}
		FamilyHandle.Reset();
		SettingsHandle.Reset();
	}
}

namespace PawPrompt
{
	Pc::PcFamily FamilyFor(const UUserWidget* W, int32 Override)
	{
		if (Override >= 0 && Override <= static_cast<int32>(Pc::PcFamily::Generic))
		{
			return static_cast<Pc::PcFamily>(Override);
		}
		const UPawInputDeviceSubsystem* D = DeviceOf(W);
		return D ? D->GetFamily() : Pc::PcFamily::Keyboard;
	}

	bool PromptsShown(const UUserWidget* W)
	{
		const UPawSettingsSubsystem* S = SettingsOf(W);
		return !S || S->Get().bShowPrompts;
	}
}

// --- UPawPromptIcon -----------------------------------------------------------------------------------------------------

bool UPawPromptIcon::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;
		Glyph = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Glyph"));
		Glyph->SetVisibility(ESlateVisibility::Collapsed);
		Root->AddChildToHorizontalBox(Glyph)->SetVerticalAlignment(VAlign_Center);
		KeySize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("KeySize"));
		KeySize->SetVisibility(ESlateVisibility::Collapsed);
		Root->AddChildToHorizontalBox(KeySize)->SetVerticalAlignment(VAlign_Center);
		Keycap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Keycap"));
		Keycap->SetBrush(PawUi::Rounded(PawTokens::UI_PANEL_FRAME, PawTokens::UI_PANEL_FRAME, 0.0f, CapRadius));
		Keycap->SetPadding(FMargin(CapFrame, CapFrame, CapFrame, CapFrameBottom));
		KeySize->SetContent(Keycap);
		KeyFace = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("KeyFace"));
		KeyFace->SetBrush(PawUi::Rounded(PawTokens::UI_BUTTON, PawTokens::UI_BUTTON, 0.0f, CapRadius - CapFrame));
		KeyFace->SetPadding(FMargin(CapMarginX - CapFrame, 0.0f));
		KeyFace->SetHorizontalAlignment(HAlign_Center);
		KeyFace->SetVerticalAlignment(VAlign_Center);
		Keycap->SetContent(KeyFace);
		KeyLabel = PawUi::MakeText(WidgetTree, 14.0, PawTokens::UI_TEXT, false, TEXT("KeyLabel"));
		KeyLabel->SetJustification(ETextJustify::Center);
		KeyFace->SetContent(KeyLabel);
	}
	const bool bOk = Super::Initialize();
	if (bOk)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return bOk;
}

void UPawPromptIcon::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (PawUi::ShouldWire(this))
	{
		Refresh();
	}
}

void UPawPromptIcon::NativeConstruct()
{
	Super::NativeConstruct();
	Listen(this, FamilyHandle, SettingsHandle);
	Refresh();
}

void UPawPromptIcon::NativeDestruct()
{
	Unlisten(this, FamilyHandle, SettingsHandle);
	Super::NativeDestruct();
}

void UPawPromptIcon::SetAction(const FString& InAction)
{
	Action = InAction;
	Refresh();
}

void UPawPromptIcon::SetIconPx(int32 Px)
{
	IconPx = Px;
	Refresh();
}

void UPawPromptIcon::SetFamilyOverride(int32 Family)
{
	FamilyOverride = Family;
	Refresh();
}

void UPawPromptIcon::Refresh()
{
	Shown = PawGlyphs::FPrompt();
	if (!Action.IsEmpty() && (bAlwaysShow || PawPrompt::PromptsShown(this)))
	{
		Shown = PawGlyphs::PromptFor(Action, PawPrompt::FamilyFor(this, FamilyOverride));
	}
	const float Px = static_cast<float>(IconPx);
	if (Glyph)
	{
		UTexture2D* Tex = Shown.Kind == PawGlyphs::EKind::Glyph ? PawGlyphs::Texture(Shown.Stem) : nullptr;
		if (Tex)
		{
			Glyph->SetBrushFromTexture(Tex, false);
			Glyph->SetDesiredSizeOverride(FVector2D(Px, Px));
		}
		Glyph->SetVisibility(Tex ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const bool bCap = Shown.Kind == PawGlyphs::EKind::Keycap;
	if (KeySize)
	{
		KeySize->SetMinDesiredWidth(Px);
		KeySize->SetMinDesiredHeight(Px);
		KeySize->SetVisibility(bCap ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (KeyLabel && bCap)
	{
		KeyLabel->SetText(Shown.Label);
		FSlateFontInfo Font = KeyLabel->GetFont();
		Font.Size = PawUi::FontPt(FMath::Max(CapMinLabelPx, static_cast<int32>(IconPx * 0.5)));
		KeyLabel->SetFont(Font);
	}
}

FVector2D UPawPromptIcon::GetPromptSize() const
{
	if (Shown.Kind == PawGlyphs::EKind::Glyph)
	{
		return FVector2D(IconPx, IconPx);
	}
	if (Shown.Kind == PawGlyphs::EKind::Keycap && KeySize)
	{
		KeySize->ForceLayoutPrepass();
		return KeySize->GetDesiredSize();
	}
	return FVector2D::ZeroVector;
}

// --- UPawPromptText -----------------------------------------------------------------------------------------------------

bool UPawPromptText::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		Rich = WidgetTree->ConstructWidget<URichTextBlock>(URichTextBlock::StaticClass(), TEXT("Rich"));
		Rich->SetDecorators({ UPawPromptDecorator::StaticClass() });
		Rich->SetAutoWrapText(false);
		WidgetTree->RootWidget = Rich;
	}
	const bool bOk = Super::Initialize();
	if (bOk)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return bOk;
}

void UPawPromptText::ApplyStyle()
{
	if (!Rich)
	{
		return;
	}
	FTextBlockStyle Style = Rich->GetDefaultTextStyle();
	FSlateFontInfo Font = Style.Font;
	if (!Font.FontObject)
	{
		Font = FSlateFontInfo(PawUi::PawFont(), 10, TEXT("Regular")); // a rich text's default style names no font
	}
	Font.Size = PawUi::FontPt(TextPx);
	Style.SetFont(Font);
	Style.SetColorAndOpacity(FSlateColor(TextColour));
	Rich->SetDefaultTextStyle(Style);
	Rich->SetJustification(bCentred ? ETextJustify::Center : ETextJustify::Left);
	Rich->SetWrapTextAt(WrapAt);
}

void UPawPromptText::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplyStyle();
	if (PawUi::ShouldWire(this))
	{
		Refresh();
	}
}

void UPawPromptText::NativeConstruct()
{
	Super::NativeConstruct();
	Listen(this, FamilyHandle, SettingsHandle);
	Refresh();
}

void UPawPromptText::NativeDestruct()
{
	Unlisten(this, FamilyHandle, SettingsHandle);
	Super::NativeDestruct();
}

void UPawPromptText::SetTemplate(const FText& InTemplate)
{
	Template = InTemplate;
	Refresh();
}

void UPawPromptText::SetTextColour(const FLinearColor& Colour)
{
	if (!TextColour.Equals(Colour))
	{
		TextColour = Colour;
		ApplyStyle();
	}
}

FString UPawPromptText::GetMarkup() const
{
	return Rich ? Rich->GetText().ToString() : FString();
}

void UPawPromptText::Refresh()
{
	// PromptLabel.refresh: hidden whole with Show button prompts off ("Buy Amount Skills Pause" alone says nothing).
	// A tooltip's line is shown and hidden by its owner.
	if (bHideWithPrompts)
	{
		const bool bShown = PawPrompt::PromptsShown(this);
		SetVisibility(bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		// R4c/R4f: a line on its own plate (a UBorder holding only it: the title's hint, the tree's legend) takes the plate
		// with it, or an empty plum pill stays behind.
		if (UBorder* Plate = Cast<UBorder>(GetParent()))
		{
			Plate->SetVisibility(bShown ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	const TArray<PawGlyphs::FPart> Parts = PawGlyphs::Parts(Template.ToString(), PawPrompt::FamilyFor(this, -1));
	Neutral = PawGlyphs::Neutral(Parts);
	if (Rich)
	{
		Rich->SetText(PawText::Verbatim(PawGlyphs::Markup(Parts, IconPx)));
	}
}

// --- UPawPromptDecorator ------------------------------------------------------------------------------------------------

namespace
{
	class FPawPromptRun : public FRichTextDecorator
	{
	public:
		FPawPromptRun(URichTextBlock* InOwner, UPawPromptDecorator* InDecorator)
			: FRichTextDecorator(InOwner), Decorator(InDecorator)
		{
		}

		virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
		{
			return RunParseResult.Name == TEXT("prompt")
				&& (RunParseResult.MetaData.Contains(TEXT("g")) || RunParseResult.MetaData.Contains(TEXT("k")));
		}

	protected:
		virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& TextStyle) const override
		{
			UPawPromptDecorator* D = Decorator.Get();
			const FString* PxText = RunInfo.MetaData.Find(TEXT("px"));
			const int32 Px = FMath::Clamp(PxText ? FCString::Atoi(**PxText) : 28, 8, 128);
			if (const FString* Stem = RunInfo.MetaData.Find(TEXT("g")))
			{
				const FSlateBrush* Brush = D ? D->BrushFor(*Stem, Px) : nullptr;
				if (!Brush)
				{
					return SNullWidget::NullWidget;
				}
				return SNew(SBox).WidthOverride(static_cast<float>(Px)).HeightOverride(static_cast<float>(Px))
					.VAlign(VAlign_Center).HAlign(HAlign_Center)
					[
						SNew(SImage).Image(Brush)
					];
			}
			// InputGlyphs._prompt_bbcode: the recessed SLOT colour behind bold ink, a space either side (not paper: on a
			// paper panel a paper keycap is invisible and "Space Buy" reads as two words).
			const FString* Label = RunInfo.MetaData.Find(TEXT("k"));
			FSlateFontInfo Bold = TextStyle.Font;
			Bold.TypefaceFontName = TEXT("Bold");
			return SNew(SBorder).BorderImage(D ? D->KeycapBrush() : nullptr).Padding(0.0f)
				[
					SNew(STextBlock)
						.Text(PawText::Verbatim(TEXT(" ") + (Label ? *Label : FString()) + TEXT(" ")))
						.Font(Bold)
						.ColorAndOpacity(FSlateColor(PawTokens::UI_TEXT))
				];
		}

	private:
		TWeakObjectPtr<UPawPromptDecorator> Decorator;
	};
}

TSharedPtr<ITextDecorator> UPawPromptDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShared<FPawPromptRun>(InOwner, this);
}

const FSlateBrush* UPawPromptDecorator::BrushFor(const FString& Stem, int32 Px)
{
	const FString Key = FString::Printf(TEXT("%s@%d"), *Stem, Px);
	if (const TSharedPtr<FSlateBrush>* Found = Brushes.Find(Key))
	{
		return Found->Get();
	}
	UTexture2D* Tex = PawGlyphs::Texture(Stem);
	if (!Tex)
	{
		return nullptr;
	}
	Textures.AddUnique(Tex);
	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	Brush->SetResourceObject(Tex);
	Brush->ImageSize = FVector2D(Px, Px);
	Brush->DrawAs = ESlateBrushDrawType::Image;
	Brushes.Add(Key, Brush);
	return Brush.Get();
}

const FSlateBrush* UPawPromptDecorator::KeycapBrush()
{
	if (!Keycap.IsValid())
	{
		Keycap = MakeShared<FSlateBrush>(PawUi::Rounded(PawTokens::UI_SLOT, PawTokens::UI_SLOT, 0.0f, 0.0f));
	}
	return Keycap.Get();
}
