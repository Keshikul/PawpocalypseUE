#pragma once

// One question with two answers, on the Modal stack (P4.7; playtest 2026-11-26, notes 27 and 43; the words approved the same
// day). The pause menu's Quit game ("Save and quit?" [Save and quit] [Cancel]) and the skill tree's spend step ("You still
// have Stardust to spend." [Leave] [Stay]) ask with it. It opens with the SAFE answer (No) selected, Back is No, a click on
// the dim does nothing, and each answer runs once, after the dialog has closed. Whoever asked keeps holding the field.
//
// A WIDGET BLUEPRINT (WBP_ConfirmDialog; PawWidgets.h): Dim, Panel, PromptText, YesButton, NoButton bound by name.

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"

#include "PawConfirmDialog.generated.h"

class UBorder;
class UCommonActivatableWidgetStack;
class UPawButton;
class UTextBlock;

UCLASS()
class PAWPOCALYPSE_API UPawConfirmDialog : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	// Pushes a dialog on a Modal stack with its question and answers. Null without a stack (the caller decides what that
	// means). OnNo may be empty.
	static UPawConfirmDialog* Open(UCommonActivatableWidgetStack* Modal, const FText& Prompt, const FText& Yes, const FText& No,
		TFunction<void()> OnYes, TFunction<void()> OnNo = nullptr);

	// The question and its answers (Open calls it before activation).
	void Ask(const FText& Prompt, const FText& Yes, const FText& No, TFunction<void()> InOnYes, TFunction<void()> InOnNo);
	// The answers (the buttons call these; Back is PressNo; so do the tests and the drivers). The first one counts.
	void PressYes();
	void PressNo();

	// What the tests read (and only read).
	FText GetPrompt() const;
	FText GetYesLabel() const;
	FText GetNoLabel() const;
	UPawButton* GetYesButton() const { return YesButton; }
	UPawButton* GetNoButton() const { return NoButton; }
	bool IsAnswered() const { return bAnswered; }

protected:
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
	void OnLayoutReady();
	void Answer(bool bYes);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Dim;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Panel;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> YesButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPawButton> NoButton;

	TFunction<void()> OnYes;
	TFunction<void()> OnNo;
	bool bAnswered = false;
};
