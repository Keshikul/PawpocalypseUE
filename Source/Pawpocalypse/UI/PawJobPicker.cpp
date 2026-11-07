#include "UI/PawJobPicker.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
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
		Dim->SetBrush(Rounded(PawTokens::SKY_BASE.CopyWithNewOpacity(static_cast<float>(PickDimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		Dim->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);

		// The panel, centred, 560 wide, growing with its list: the theme's WindowPanel (F21, designer 2026-11-03).
		USizeBox* Size = T->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(static_cast<float>(JobPanelWidth));
		Size->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
		SizeSlot->SetHorizontalAlignment(HAlign_Center);
		SizeSlot->SetVerticalAlignment(VAlign_Center);
		Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Panel->SetBrush(WindowBrush());
		Panel->SetPadding(WindowPadding());
		Panel->SetVisibility(ESlateVisibility::Visible); // the plate stops clicks too
		Size->SetContent(Panel);

		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>();
		Body->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Panel->SetContent(Body);
		const FMargin Gap(0.0f, 0.0f, 0.0f, static_cast<float>(JobBodySeparation));

		Title = MakeText(T, JobTitlePx, PawTokens::INK, false, TEXT("Title"));
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
		const FMargin Gap(0.0f, I == 0 ? 0.0f : static_cast<float>(JobListSeparation), 0.0f, 0.0f);
		UPawButton* Button = WidgetTree->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Button->SetMinDimensions(0, static_cast<int32>(JobRowHeight));
		Button->OnClicked().AddWeakLambda(this, [this, I]() { ClickRow(I); });
		List->AddChildToVerticalBox(Button)->SetPadding(Gap);
		UTextBlock* Blurb = MakeText(WidgetTree, JobBlurbPx, PawTokens::UI_TEXT_MUTED);
		Blurb->SetAutoWrapText(false);
		Blurb->SetWrapTextAt(InnerWidth);
		List->AddChildToVerticalBox(Blurb)->SetPadding(FMargin(0.0f, static_cast<float>(JobListSeparation), 0.0f, 0.0f));
		Buttons.Add(Button);
		Blurbs.Add(Blurb);
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
		Blurbs[I]->SetText(Blurb);
		Blurbs[I]->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	for (int32 I = RowIds.Num(); I < Buttons.Num(); ++I)
	{
		Buttons[I]->SetVisibility(ESlateVisibility::Collapsed);
		Blurbs[I]->SetVisibility(ESlateVisibility::Collapsed);
	}
	TArray<TArray<UWidget*>> Column;
	for (int32 I = 0; I < RowIds.Num(); ++I)
	{
		Column.Add({ Buttons[I].Get() });
	}
	Wired = WireRows(Column);
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
