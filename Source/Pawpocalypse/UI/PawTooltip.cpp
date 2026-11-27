#include "UI/PawTooltip.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/PawIcons.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

namespace
{
	// The card's measures (C1's tooltip at the dock's scale).
	constexpr double TipPadX = 16.0;
	constexpr double TipPadY = 12.0;
	constexpr double TipIconPx = 44.0;
	constexpr double TipNamePx = 22.0;
	constexpr double TipLevelPx = 16.0;
	constexpr double TipLinePx = 17.0;
	constexpr double TipNumberPx = 30.0;
	constexpr double TipNumberLabelPx = 16.0;
	constexpr double TipChipPx = 18.0;
	constexpr double TipChipIconPx = 22.0;
	constexpr double TipGapY = 8.0;
	// The line wraps inside the card, 10 px short of its padding (a wrap at the box's width runs over: KB U/09).
	constexpr float TipLineWrap = static_cast<float>(PawUi::DockTipWidth - 2.0 * TipPadX - 16.0);

	void Show(UWidget* W, bool bShow)
	{
		if (W)
		{
			W->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	void Set(UTextBlock* T, const FText& Text)
	{
		if (T)
		{
			T->SetText(Text);
			Show(T, !Text.IsEmpty());
		}
	}

	FText Read(const UTextBlock* T)
	{
		return T ? T->GetText() : FText::GetEmpty();
	}

	bool Shown(const UWidget* W)
	{
		return W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden;
	}
}

bool UPawTooltip::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		using namespace PawUi;
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		USizeBox* Size = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TipBox"));
		Size->SetWidthOverride(static_cast<float>(DockTipWidth));
		WidgetTree->RootWidget = Size;

		Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrush(WindowBrush());
		Panel->SetPadding(FMargin(static_cast<float>(TipPadX), static_cast<float>(TipPadY)));
		Size->SetContent(Panel);
		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
		Panel->SetContent(Body);
		const FMargin Below(0.0f, 0.0f, 0.0f, static_cast<float>(TipGapY));

		// The head: the icon, then the name over the level.
		UHorizontalBox* Head = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Head"));
		Body->AddChildToVerticalBox(Head)->SetPadding(Below);
		Icon = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
		Icon->SetDesiredSizeOverride(FVector2D(TipIconPx, TipIconPx));
		UHorizontalBoxSlot* IconSlot = Head->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		UVerticalBox* Titles = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Titles"));
		UHorizontalBoxSlot* TitlesSlot = Head->AddChildToHorizontalBox(Titles);
		TitlesSlot->SetVerticalAlignment(VAlign_Center);
		TitlesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameText = Heavy(MakeText(T, TipNamePx, PawTokens::UI_TEXT, false, TEXT("NameText")));
		Titles->AddChildToVerticalBox(NameText);
		LevelText = MakeText(T, TipLevelPx, PawTokens::UI_TEXT_MUTED, false, TEXT("LevelText"));
		Titles->AddChildToVerticalBox(LevelText);

		// The one line.
		LineText = MakeText(T, TipLinePx, PawTokens::UI_TEXT, false, TEXT("LineText"));
		LineText->SetAutoWrapText(false);
		LineText->SetWrapTextAt(TipLineWrap);
		Body->AddChildToVerticalBox(LineText)->SetPadding(Below);

		// Now → Next, the numbers big, their words small above them.
		Numbers = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Numbers"));
		Body->AddChildToVerticalBox(Numbers)->SetPadding(Below);
		auto Box = [&](const TCHAR* Name, const TCHAR* LabelName, const TCHAR* ValueName, const TCHAR* Key, UTextBlock*& OutLabel,
			UTextBlock*& OutValue)
		{
			UVerticalBox* B = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
			OutLabel = MakeText(T, TipNumberLabelPx, PawTokens::UI_TEXT_MUTED, false, LabelName);
			OutLabel->SetText(PawText::Ui(Key));
			B->AddChildToVerticalBox(OutLabel);
			OutValue = Heavy(MakeText(T, TipNumberPx, PawTokens::UI_TEXT, false, ValueName));
			B->AddChildToVerticalBox(OutValue);
			return B;
		};
		UTextBlock* NowLabelText = nullptr;
		UTextBlock* NowValueText = nullptr;
		UVerticalBox* NowBox = Box(TEXT("NowBox"), TEXT("NowLabel"), TEXT("NowValue"), TEXT("TIP_now"), NowLabelText, NowValueText);
		NowLabel = NowLabelText;
		NowValue = NowValueText;
		Numbers->AddChildToHorizontalBox(NowBox)->SetVerticalAlignment(VAlign_Bottom);
		Arrow = Heavy(MakeText(T, TipNumberPx, PawTokens::UI_TEXT_MUTED, false, TEXT("Arrow")));
		Arrow->SetText(PawText::Verbatim(TEXT("→")));
		UHorizontalBoxSlot* ArrowSlot = Numbers->AddChildToHorizontalBox(Arrow);
		ArrowSlot->SetVerticalAlignment(VAlign_Bottom);
		ArrowSlot->SetPadding(FMargin(14.0f, 0.0f, 14.0f, 0.0f));
		UTextBlock* NextLabelText = nullptr;
		UTextBlock* NextValueText = nullptr;
		NextBox = Box(TEXT("NextBox"), TEXT("NextLabel"), TEXT("NextValue"), TEXT("TIP_next"), NextLabelText, NextValueText);
		NextLabel = NextLabelText;
		NextValue = NextValueText;
		Numbers->AddChildToHorizontalBox(NextBox)->SetVerticalAlignment(VAlign_Bottom);

		// The chips: the cost, then what stops the buy.
		UHorizontalBox* Chips = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Chips"));
		Body->AddChildToVerticalBox(Chips);
		CostChip = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CostChip"));
		CostChip->SetBrush(Chrome(EPawChrome::ButtonPrimary));
		CostChip->SetPadding(PlatePadding(FMargin(10.0f, 1.0f, 12.0f, 1.0f), EPawChrome::ButtonPrimary));
		UHorizontalBoxSlot* CostSlot = Chips->AddChildToHorizontalBox(CostChip);
		CostSlot->SetVerticalAlignment(VAlign_Center);
		CostSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		UHorizontalBox* CostLine = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CostLine"));
		CostChip->SetContent(CostLine);
		CostIcon = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CostIcon"));
		CostIcon->SetBrush(IconBrush(TEXT("ui_icon_catnip"), TipChipIconPx));
		UHorizontalBoxSlot* CostIconSlot = CostLine->AddChildToHorizontalBox(CostIcon);
		CostIconSlot->SetVerticalAlignment(VAlign_Center);
		CostIconSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		CostText = Heavy(MakeText(T, TipChipPx, PawTokens::UI_TEXT, false, TEXT("CostText")));
		CostLine->AddChildToHorizontalBox(CostText)->SetVerticalAlignment(VAlign_Center);
		BlockChip = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlockChip"));
		BlockChip->SetBrush(Rounded(PawTokens::UI_DANGER, PawTokens::INK, 2.5f, 10.0f));
		BlockChip->SetPadding(FMargin(10.0f, 3.0f));
		Chips->AddChildToHorizontalBox(BlockChip)->SetVerticalAlignment(VAlign_Center);
		BlockText = Heavy(MakeText(T, TipChipPx, PawTokens::UI_PANEL, false, TEXT("BlockText")));
		BlockChip->SetContent(BlockText);
	}
	const bool bOk = Super::Initialize();
	if (bOk)
	{
		SetVisibility(ESlateVisibility::Collapsed); // shown by whoever places it; it never takes a click
		if (PawUi::ShouldWire(this))
		{
			PawUi::UseAtlasIcons(this); // the asset's saved icon brushes: the atlas's cells (one batch)
		}
	}
	return bOk;
}

void UPawTooltip::SetHeader(const FString& IconStem, const FText& Name, const FText& Level)
{
	if (Icon)
	{
		const FSlateBrush Brush = IconStem.IsEmpty() ? FSlateBrush() : PawUi::IconBrush(IconStem, TipIconPx);
		const bool bHas = Brush.GetResourceObject() != nullptr;
		if (bHas)
		{
			Icon->SetBrush(Brush);
		}
		Show(Icon, bHas);
	}
	Set(NameText, Name);
	Set(LevelText, Level);
}

void UPawTooltip::SetLine(const FText& Line)
{
	Set(LineText, Line);
}

void UPawTooltip::SetNumbers(bool bShow, const FText& Now, const FText& Next, bool bShowNext)
{
	Show(Numbers, bShow);
	if (!bShow)
	{
		return;
	}
	if (NowValue)
	{
		NowValue->SetText(Now);
	}
	if (NextValue)
	{
		NextValue->SetText(Next);
	}
	Show(Arrow, bShowNext);
	Show(NextBox, bShowNext);
}

void UPawTooltip::SetCost(bool bShow, const FText& Price, bool bCanPay, bool bLeaf)
{
	Show(CostIcon, bShow && bLeaf);
	Show(CostChip, bShow);
	if (!bShow)
	{
		return;
	}
	if (CostChip && (!bCostStyled || bCostPayable != bCanPay))
	{
		CostChip->SetBrush(PawUi::Chrome(bCanPay ? PawUi::EPawChrome::ButtonPrimary : PawUi::EPawChrome::ButtonDisabled));
		bCostStyled = true;
	}
	bCostPayable = bCanPay;
	if (CostText)
	{
		CostText->SetText(Price);
		PawUi::SetTextColour(CostText, bCanPay ? PawTokens::UI_TEXT : PawTokens::UI_TEXT_DISABLED);
	}
}

void UPawTooltip::SetBlock(bool bShow, const FText& Why)
{
	Show(BlockChip, bShow && !Why.IsEmpty());
	if (BlockText)
	{
		BlockText->SetText(Why);
	}
}

FText UPawTooltip::GetName() const { return Read(NameText); }
FText UPawTooltip::GetLevel() const { return Read(LevelText); }
FText UPawTooltip::GetLine() const { return Read(LineText); }
FText UPawTooltip::GetNow() const { return Read(NowValue); }
FText UPawTooltip::GetNext() const { return Read(NextValue); }
FText UPawTooltip::GetCost() const { return Read(CostText); }
FText UPawTooltip::GetBlock() const { return Read(BlockText); }
bool UPawTooltip::IsShown() const { return Shown(this); }
bool UPawTooltip::IsNumbersShown() const { return Shown(Numbers); }
bool UPawTooltip::IsNextShown() const { return Shown(NextBox); }
bool UPawTooltip::IsCostShown() const { return Shown(CostChip); }
bool UPawTooltip::IsBlockShown() const { return Shown(BlockChip); }
