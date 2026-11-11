#include "UI/PawIcons.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"

UTexture2D* UPawIconSet::Find(const FString& Stem) const
{
	if (ByName.Num() != Names.Num())
	{
		ByName.Reset();
		for (int32 I = 0; I < Names.Num(); ++I)
		{
			ByName.Add(Names[I], I);
		}
	}
	const int32* I = ByName.Find(Stem);
	return I && Textures.IsValidIndex(*I) ? Textures[*I].Get() : nullptr;
}

const UPawIconSet* UPawIconCache::Get()
{
	if (!Set)
	{
		Set = LoadObject<UPawIconSet>(nullptr, TEXT("/Game/Pawpocalypse/Art/UI/Icons/DA_Icons.DA_Icons"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	return Set;
}

void UPawIconCache::Deinitialize()
{
	Set = nullptr; // with the engine, not after it
	Super::Deinitialize();
}

namespace PawUi
{
	const UPawIconSet* Icons()
	{
		UPawIconCache* Cache = GEngine ? GEngine->GetEngineSubsystem<UPawIconCache>() : nullptr;
		return Cache ? Cache->Get() : nullptr;
	}

	UTexture2D* Icon(const FString& StemOrPath)
	{
		const UPawIconSet* Set = Icons();
		// art.gd path_for: the stem of a res:// path; a bare stem as it is.
		const FString Stem = StemOrPath.Contains(TEXT("/")) || StemOrPath.Contains(TEXT("."))
			? FPaths::GetBaseFilename(StemOrPath) : StemOrPath;
		return Set && !Stem.IsEmpty() ? Set->Find(Stem.ToLower()) : nullptr;
	}

	FSlateBrush IconBrush(const FString& StemOrPath, double Px)
	{
		FSlateBrush B;
		UTexture2D* Tex = Icon(StemOrPath);
		if (!Tex)
		{
			B.DrawAs = ESlateBrushDrawType::NoDrawType;
			return B;
		}
		B.DrawAs = ESlateBrushDrawType::Image;
		B.SetResourceObject(Tex);
		B.ImageSize = FVector2D(Px, Px);
		B.TintColor = FSlateColor(FLinearColor::White);
		return B;
	}

	FString TabIconName(const FString& TabId) { return TEXT("ui_tab_") + TabId; }

	FString CurrencyIconName(const FString& Kind)
	{
		// currency_icons.gd STEMS
		if (Kind == TEXT("catnip")) { return TEXT("ui_icon_catnip"); }
		if (Kind == TEXT("stardust")) { return TEXT("ui_icon_stardust"); }
		if (Kind == TEXT("bait")) { return TEXT("ui_icon_bait"); }
		return FString();
	}

	FString AchievementIconName(const FString& AchievementId) { return TEXT("achievement_") + AchievementId; }
}

bool UPawIconText::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		WidgetTree->RootWidget = Row;
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f)); // wrap_label's HBox separation
		Text = PawUi::MakeText(WidgetTree, 17.0, FLinearColor::Black, false, TEXT("Text"));
		UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Text);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}
	const bool bOk = Super::Initialize();
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Apply();
	}
	return bOk;
}

void UPawIconText::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	Apply();
}

void UPawIconText::Apply()
{
	if (Text)
	{
		FSlateFontInfo Font = Text->GetFont();
		// Over the live field: the light fill and the ink outline (PawUi::MakeText bOverField). Until R3 the flag was never
		// applied; ink text on the lawn read anyway, bone text does not.
		const int32 Outline = bOverField ? static_cast<int32>(PawUi::OverFieldOutline) : 0;
		if (Font.Size != PawUi::FontPt(TextPx) || Font.OutlineSettings.OutlineSize != Outline)
		{
			Font.Size = PawUi::FontPt(TextPx);
			Font.OutlineSettings.OutlineSize = Outline;
			Font.OutlineSettings.OutlineColor = PawTokens::INK;
			Text->SetFont(Font);
		}
		PawUi::SetTextColour(Text, bOverField ? PawTokens::UI_FIELD_LABEL : TextColour);
		if (WrapAt > 0.0f)
		{
			Text->SetAutoWrapText(false);
			Text->SetWrapTextAt(WrapAt);
		}
	}
	if (Icon)
	{
		const FSlateBrush Brush = PawUi::IconBrush(IconName, IconPx);
		const bool bShow = Brush.GetResourceObject() != nullptr;
		Icon->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShow)
		{
			Icon->SetBrush(Brush);
		}
	}
}

void UPawIconText::SetText(const FText& InText)
{
	if (Text && !Text->GetText().EqualTo(InText))
	{
		Text->SetText(InText);
	}
}

FText UPawIconText::GetText() const
{
	return Text ? Text->GetText() : FText::GetEmpty();
}

void UPawIconText::SetIcon(const FString& Stem)
{
	IconName = Stem;
	Apply();
}

bool UPawIconText::IsIconShown() const
{
	return Icon && Icon->GetVisibility() != ESlateVisibility::Collapsed;
}
