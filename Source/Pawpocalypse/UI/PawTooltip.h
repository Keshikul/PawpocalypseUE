#pragma once

// The shared tooltip card (P4.1; playtest 2026-11-26, notes 14 and 23; C1's approved tooltip look, its words TIP_* and
// UPG_*_desc approved the same day). One card for what the dock's rows can tell: the thing's icon, name and level; one line
// on what it does; Now → Next as two big numbers; a cost chip; one red chip only when something stops the buy. The HUD
// places it beside the dock on the field side, level with the row (Pc::PlaceTip). It takes no input.
//
// A WIDGET BLUEPRINT (WBP_Tooltip; PawWidgets.h): Panel, Icon, NameText, LevelText, LineText, Numbers, NowLabel, NowValue,
// Arrow, NextBox, NextLabel, NextValue, CostChip, CostIcon, CostText, BlockChip, BlockText bound by name.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "PawTooltip.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UVerticalBox;

UCLASS()
class PAWPOCALYPSE_API UPawTooltip : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	// The parts, each shown only when it has something to say (an empty text or bShow false collapses it).
	void SetHeader(const FString& IconStem, const FText& Name, const FText& Level);
	void SetLine(const FText& Line);
	// Now → Next; without a Next (a maxed line) only Now shows.
	void SetNumbers(bool bShow, const FText& NowValue, const FText& NextValue, bool bShowNext);
	// The price on its chip: the raised yellow when it can be paid, the off plate when not.
	void SetCost(bool bShow, const FText& Price, bool bCanPay, bool bLeaf = true);
	// The one thing that stops the buy ("Need 30 more"), on a red chip.
	void SetBlock(bool bShow, const FText& Why);

	// What the tests read (and only read).
	FText GetName() const;
	FText GetLevel() const;
	FText GetLine() const;
	FText GetNow() const;
	FText GetNext() const;
	FText GetCost() const;
	FText GetBlock() const;
	bool IsShown() const;
	bool IsNumbersShown() const;
	bool IsNextShown() const;
	bool IsCostShown() const;
	bool IsCostPayable() const { return bCostPayable; }
	bool IsBlockShown() const;

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Panel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Icon;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LineText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> Numbers;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NowLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NowValue;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Arrow;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> NextBox;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextLabel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NextValue;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CostChip;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CostIcon;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> BlockChip;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BlockText;

	bool bCostPayable = false;
	bool bCostStyled = false;
};
