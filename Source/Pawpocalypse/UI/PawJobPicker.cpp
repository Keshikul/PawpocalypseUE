#include "UI/PawJobPicker.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/World.h"
#include "PcHud.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawIcons.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

const FName UPawJobPicker::PauseHolder(TEXT("job_picker"));

namespace
{
	using namespace PawUi;

	// The panel's text is given a fixed wrap width (KB U/09 §3): the panel's width less its margins.
	constexpr float InnerWidth = static_cast<float>(JobPanelWidth - 2.0 * WindowPadX);
	constexpr float CardInner = static_cast<float>(JobCardWidth - 2.0 * JobCardPad);

	// D18 (S4): the cat in a job's hat (tools/art/sticker_art.py write_job_cats); "No job": the bare kitten.
	FString JobCatIcon(const FString& JobId)
	{
		return TEXT("ui_icon_job_cat_") + (JobId.IsEmpty() ? FString(TEXT("none")) : JobId);
	}

	bool Selected(const UWidget* W)
	{
		// HasAnyUserFocus is false for a Common button whose inner SCommonButton holds focus (KB U/08 §3.3).
		return W && W->GetVisibility() != ESlateVisibility::Collapsed && (W->HasAnyUserFocus() || W->HasFocusedDescendants());
	}
}

bool UPawJobPicker::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		UOverlay* Root = T->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// A dimmer, so the dock behind reads as out of play rather than still clickable; it eats every click (modal).
		Dim = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dim"));
		Dim->SetBrush(Rounded(PawTokens::INK.CopyWithNewOpacity(static_cast<float>(PickDimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		Dim->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);

		// The panel, centred, growing with its cards: the theme's WindowPanel (F21, designer 2026-11-03). D18 (S4): as wide
		// as five cards, in a box that shrinks it to fit the screen (never grows it).
		UScaleBox* Fit = T->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("Fit"));
		Fit->SetStretch(EStretch::ScaleToFit);
		Fit->SetStretchDirection(EStretchDirection::DownOnly);
		Fit->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* FitSlot = Root->AddChildToOverlay(Fit);
		FitSlot->SetHorizontalAlignment(HAlign_Fill);
		FitSlot->SetVerticalAlignment(VAlign_Fill);
		FitSlot->SetPadding(FMargin(static_cast<float>(JobFitMargin)));
		USizeBox* Size = T->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(static_cast<float>(JobPanelWidth));
		Size->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UScaleBoxSlot* SizeSlot = Cast<UScaleBoxSlot>(Fit->AddChild(Size)))
		{
			SizeSlot->SetHorizontalAlignment(HAlign_Center);
			SizeSlot->SetVerticalAlignment(VAlign_Center);
		}
		Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrush(WindowBrush());
		Panel->SetPadding(WindowPadding());
		Panel->SetVisibility(ESlateVisibility::Visible); // the plate stops clicks too
		Size->SetContent(Panel);

		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>();
		Body->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Panel->SetContent(Body);
		const FMargin Gap(0.0f, 0.0f, 0.0f, static_cast<float>(JobBodySeparation));

		Title = ButterTitle(MakeText(T, JobTitlePx, PawTokens::UI_TEXT, false, TEXT("Title")), ButterTitleOutline); // R4g
		Title->SetJustification(ETextJustify::Center);
		Title->SetText(PawText::Ui(TEXT("JOBPICK_title")));
		UVerticalBoxSlot* TitleSlot = Body->AddChildToVerticalBox(Title);
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(Gap);

		Note = MakeText(T, LabelPx, PawTokens::UI_TEXT, false, TEXT("Note")); // the theme's Label: ink, 17 px
		Note->SetJustification(ETextJustify::Center);
		Note->SetAutoWrapText(false);
		Note->SetWrapTextAt(InnerWidth);
		Note->SetText(PawText::Ui(TEXT("JOBPICK_note")));
		UVerticalBoxSlot* NoteSlot = Body->AddChildToVerticalBox(Note);
		NoteSlot->SetHorizontalAlignment(HAlign_Center);
		NoteSlot->SetPadding(Gap);

		List = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("List"));
		List->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Body->AddChildToVerticalBox(List);
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		OnLayoutReady();
	}
	return bOk;
}

void UPawJobPicker::OnLayoutReady()
{
	// The rows are made from the jobs held, into List, when the picker opens (EnsureRow).
}

UPawRealmSubsystem* UPawJobPicker::Realm() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
}

void UPawJobPicker::EnsureRow(int32 Index)
{
	while (Buttons.Num() <= Index)
	{
		const int32 I = Buttons.Num();
		// D18 (S4): a card per job, in rows of up to five (each row a box in List, made as the cards need it).
		const int32 RowOfCards = I / JobCardsPerRow;
		while (CardRows.Num() <= RowOfCards)
		{
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
			Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			UVerticalBoxSlot* RowSlot = List->AddChildToVerticalBox(Row);
			RowSlot->SetHorizontalAlignment(HAlign_Center);
			RowSlot->SetPadding(FMargin(0.0f, CardRows.Num() == 0 ? 0.0f : static_cast<float>(JobCardGap), 0.0f, 0.0f));
			CardRows.Add(Row);
		}
		UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
		Card->SetBrush(Rounded(PawTokens::UI_PANEL, PawTokens::INK, static_cast<float>(CardFramePx), static_cast<float>(JobCardRadius)));
		Card->SetPadding(FMargin(static_cast<float>(JobCardPad)));
		Card->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UHorizontalBoxSlot* CardSlot = CardRows[RowOfCards]->AddChildToHorizontalBox(Card);
		CardSlot->SetVerticalAlignment(VAlign_Fill);
		CardSlot->SetPadding(FMargin(I % JobCardsPerRow == 0 ? 0.0f : static_cast<float>(JobCardGap), 0.0f, 0.0f, 0.0f));
		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetWidthOverride(CardInner);
		Card->SetContent(Width);
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Width->SetContent(Column);
		UBorder* Photo = WidgetTree->ConstructWidget<UBorder>();
		Photo->SetBrush(Rounded(PawTokens::UI_PHOTO, PawTokens::INK, static_cast<float>(SlotPhotoEdgePx), static_cast<float>(JobPhotoRadius)));
		Photo->SetPadding(FMargin(0.0f, static_cast<float>(JobPhotoPadY)));
		Photo->SetHorizontalAlignment(HAlign_Center);
		Photo->SetVisibility(ESlateVisibility::HitTestInvisible);
		Column->AddChildToVerticalBox(Photo)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, static_cast<float>(JobListSeparation)));
		UImage* Cat = WidgetTree->ConstructWidget<UImage>();
		Photo->SetContent(Cat);
		UPawButton* Button = WidgetTree->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Button->SetMinDimensions(0, static_cast<int32>(JobRowHeight));
		Button->SetWrapAt(CardInner - 2.0f * static_cast<float>(JobCardPad));
		Button->OnClicked().AddWeakLambda(this, [this, I]() { ClickRow(I); });
		Column->AddChildToVerticalBox(Button);
		UTextBlock* Blurb = MakeText(WidgetTree, JobBlurbPx, PawTokens::UI_TEXT_MUTED);
		Blurb->SetAutoWrapText(false);
		Blurb->SetWrapTextAt(CardInner);
		Column->AddChildToVerticalBox(Blurb)->SetPadding(FMargin(0.0f, static_cast<float>(JobListSeparation), 0.0f, 0.0f));
		Buttons.Add(Button);
		Blurbs.Add(Blurb);
		Cards.Add(Card);
		Photos.Add(Photo);
		Cats.Add(Cat);
	}
}

void UPawJobPicker::StepLook()
{
	// D18 (S4): the card the pointer or the pad is on has its button on the yellow plate, the thing to press.
	for (int32 I = 0; I < Buttons.Num() && I < RowIds.Num(); ++I)
	{
		UPawButton* B = Buttons[I];
		const bool bLifted = I == HeldRow || (B && (B->IsHovered() || Selected(B)));
		if (B && B->HasCallLook() != bLifted)
		{
			B->SetCallLook(bLifted);
		}
	}
}

void UPawJobPicker::ShowRows(bool bInSecond, bool bRefocus)
{
	bSecond = bInSecond;
	RowIds.Reset();
	const UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
	Pc::PcJobPicker Picker;
	Picker.Rows.push_back(Pc::PcJobPickRow()); // no data: the baseline alone, so the player is never trapped
	if (G && G->IsDataReady())
	{
		Picker = Pc::JobPicker(G->Data(), G->Progress(), bSecond);
	}
	FocusRow = static_cast<int32>(Picker.Focus);
	for (int32 I = 0; I < static_cast<int32>(Picker.Rows.size()); ++I)
	{
		const Pc::PcJobPickRow& Row = Picker.Rows[static_cast<size_t>(I)];
		RowIds.Add(UTF8_TO_TCHAR(Row.JobId.c_str()));
		FText Label;
		FText Blurb;
		if (Row.JobId.empty())
		{
			// The baseline. Always last, always available.
			Label = PawText::Ui(bSecond ? TEXT("JOBPICK_no_second") : TEXT("JOBPICK_none"));
			Blurb = PawText::Ui(bSecond ? TEXT("JOBPICK_no_second_blurb") : TEXT("JOBPICK_none_blurb"));
		}
		else
		{
			Label = PawText::Data(TEXT("JOB"), Row.JobId);
			Blurb = PawText::DataField(TEXT("JOB"), Row.JobId, TEXT("desc"));
			if (bSecond)
			{
				FFormatNamedArguments Args;
				Args.Add(TEXT("n"), PawText::Count(Picker.SharePercent));
				Args.Add(TEXT("description"), Blurb);
				Blurb = PawText::Ui(TEXT("JOBPICK_share"), Args);
			}
		}
		if (Row.bLastRun)
		{
			// The previous choice is offered as the default: the same job every run is one press.
			FFormatNamedArguments Args;
			Args.Add(TEXT("job"), Label);
			Label = PawText::Ui(TEXT("JOBPICK_last_run"), Args);
		}
		EnsureRow(I);
		Buttons[I]->SetLabel(Label);
		Buttons[I]->SetVisibility(ESlateVisibility::Visible);
		Buttons[I]->SetCallLook(false);
		Blurbs[I]->SetText(Blurb);
		Blurbs[I]->SetVisibility(ESlateVisibility::HitTestInvisible);
		Cards[I]->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Cats[I]->SetBrush(PawUi::IconBrush(JobCatIcon(RowIds[I]), JobCatPx));
	}
	for (int32 I = RowIds.Num(); I < Buttons.Num(); ++I)
	{
		Buttons[I]->SetVisibility(ESlateVisibility::Collapsed);
		Blurbs[I]->SetVisibility(ESlateVisibility::Collapsed);
		Cards[I]->SetVisibility(ESlateVisibility::Collapsed);
	}
	// The cards in rows of up to five: left and right along a row, up and down between rows.
	TArray<TArray<UWidget*>> Grid;
	for (int32 I = 0; I < RowIds.Num(); ++I)
	{
		if (I % JobCardsPerRow == 0)
		{
			Grid.AddDefaulted();
		}
		Grid.Last().Add(Buttons[I].Get());
	}
	Wired = WireRows(Grid);
	// The guard's clock starts with every open, the second ask's included.
	OpenedAt = Clock;
	if (bRefocus && CanTakeFocus())
	{
		if (UWidget* Target = NativeGetDesiredFocusTarget())
		{
			Target->SetFocus();
		}
	}
}

UWidget* UPawJobPicker::NativeGetDesiredFocusTarget() const
{
	// _focus_list(): the default row (last run's job), else the first. Never nothing (the pad dies).
	if (Buttons.IsValidIndex(FocusRow) && FocusRow < RowIds.Num())
	{
		return Buttons[FocusRow];
	}
	return Buttons.Num() > 0 ? Buttons[0].Get() : nullptr;
}

TOptional<FUIInputConfig> UPawJobPicker::GetDesiredInputConfig() const
{
	// A modal over a held field: the game's input stays out; its buttons are Slate widgets (no viewport capture).
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UPawJobPicker::NativeOnActivated()
{
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, true);
	}
	// open(): the first ask. Pooled: everything re-read here (docs/05 §2 rule 5); Common UI focuses the target after this.
	ShowRows(false, false);
	Super::NativeOnActivated();
}

void UPawJobPicker::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, false);
	}
}

void UPawJobPicker::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	AdvanceClock(DeltaSeconds);
	StepLook();
}

FReply UPawJobPicker::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	NoteKeyDown(KeyEvent.IsRepeat());
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

FReply UPawJobPicker::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	NoteMouseDown();
	return Super::NativeOnPreviewMouseButtonDown(Geometry, MouseEvent);
}

void UPawJobPicker::NoteKeyDown(bool bRepeat)
{
	bLastWasPointer = false;
	if (!bRepeat)
	{
		PressBeganAt = Clock;
	}
}

void UPawJobPicker::NoteMouseDown()
{
	bLastWasPointer = true;
}

bool UPawJobPicker::IsGuardOpen() const
{
	return UPawCardPickScreen::GuardAllows(OpenedAt, Clock, bLastWasPointer, PressBeganAt);
}

void UPawJobPicker::ClickRow(int32 Row)
{
	if (IsGuardOpen())
	{
		Pick(Row);
	}
}

bool UPawJobPicker::Pick(int32 Row)
{
	UPawRealmSubsystem* R = Realm();
	if (!R || !RowIds.IsValidIndex(Row))
	{
		return false;
	}
	if (R->PickJob(RowIds[Row], bSecond))
	{
		// UNION RULES: ask again, in place, for the second slot.
		ShowRows(true, true);
		return true;
	}
	bSecond = false;
	TFunction<void()> Then = MoveTemp(OnChosen);
	OnChosen = nullptr;
	if (IsActivated())
	{
		DeactivateWidget();
	}
	if (Then)
	{
		Then();
	}
	return true;
}

bool UPawJobPicker::CanTakeFocus() const
{
	return IsActivated() && GetOwningLocalPlayer() != nullptr;
}

UPawButton* UPawJobPicker::GetRowButton(int32 Row) const
{
	return Row >= 0 && Row < RowIds.Num() && Buttons.IsValidIndex(Row) ? Buttons[Row].Get() : nullptr;
}

FText UPawJobPicker::GetRowLabel(int32 Row) const
{
	const UPawButton* B = GetRowButton(Row);
	return B ? B->GetLabel() : FText::GetEmpty();
}

FText UPawJobPicker::GetRowBlurb(int32 Row) const
{
	return Row >= 0 && Row < RowIds.Num() && Blurbs.IsValidIndex(Row) ? Blurbs[Row]->GetText() : FText::GetEmpty();
}

FString UPawJobPicker::GetRowCatName(int32 Row) const
{
	const UImage* Cat = Row >= 0 && Row < RowIds.Num() && Cats.IsValidIndex(Row) ? Cats[Row].Get() : nullptr;
	const UObject* Tex = Cat ? Cat->GetBrush().GetResourceObject() : nullptr;
	return Tex ? Tex->GetName() : FString();
}

FLinearColor UPawJobPicker::GetRowPhotoColour(int32 Row) const
{
	const UBorder* Photo = Row >= 0 && Row < RowIds.Num() && Photos.IsValidIndex(Row) ? Photos[Row].Get() : nullptr;
	return Photo ? Photo->Background.TintColor.GetSpecifiedColor() : FLinearColor::Transparent;
}

int32 UPawJobPicker::GetShownRowCount() const
{
	int32 N = 0;
	for (const UPawButton* B : Buttons)
	{
		N += B && B->GetVisibility() != ESlateVisibility::Collapsed ? 1 : 0;
	}
	return N;
}

FText UPawJobPicker::GetTitleText() const { return Title ? Title->GetText() : FText::GetEmpty(); }
FText UPawJobPicker::GetNoteText() const { return Note ? Note->GetText() : FText::GetEmpty(); }
UWidget* UPawJobPicker::GetDim() const { return Dim; }
