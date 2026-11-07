#include "UI/PawPauseMenu.h"

#include "Audio/PawAudioSubsystem.h"
#include "Input/PawInputMap.h"
#include "UI/PawSettingsDialog.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PcSave.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

const FName UPawPauseMenu::PauseHolder(TEXT("pause"));

namespace
{
	using namespace PawUi;

	const TCHAR* const TitleMap = TEXT("/Game/Pawpocalypse/Maps/L_Title");

	UPawButton* PageButton(UWidgetTree* T, UVerticalBox* Page, const TCHAR* Name, const TCHAR* Key, double Height, bool bFirst, double Gap)
	{
		UPawButton* B = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), Name);
		if (Key)
		{
			B->SetLabel(PawText::Ui(Key));
		}
		B->SetMinDimensions(0, static_cast<int32>(Height));
		UVerticalBoxSlot* S = Page->AddChildToVerticalBox(B);
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetPadding(FMargin(0.0f, bFirst ? 0.0f : static_cast<float>(Gap), 0.0f, 0.0f));
		return B;
	}
}

bool UPawPauseMenu::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		UOverlay* Root = T->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The scrim: the sky tone; it eats every click (the field behind is held).
		Scrim = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scrim"));
		Scrim->SetBrush(Rounded(PawTokens::SKY_BASE.CopyWithNewOpacity(static_cast<float>(PauseScrimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		Scrim->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* ScrimSlot = Root->AddChildToOverlay(Scrim);
		ScrimSlot->SetHorizontalAlignment(HAlign_Fill);
		ScrimSlot->SetVerticalAlignment(VAlign_Fill);

		// The window: 560 x 460 at the centre, paper, an ink frame (StyleBoxFlat_window).
		USizeBox* Size = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WindowSize"));
		Size->SetWidthOverride(static_cast<float>(PauseWindowWidth));
		Size->SetHeightOverride(static_cast<float>(PauseWindowHeight));
		UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
		SizeSlot->SetHorizontalAlignment(HAlign_Center);
		SizeSlot->SetVerticalAlignment(VAlign_Center);
		Window = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Window"));
		Window->SetBrush(Rounded(PawTokens::UI_PANEL, PawTokens::INK, static_cast<float>(PauseFrame), static_cast<float>(PauseRadius)));
		Window->SetPadding(FMargin(static_cast<float>(PausePadX), static_cast<float>(PausePadY)));
		Window->SetVisibility(ESlateVisibility::Visible);
		Size->SetContent(Window);
		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
		Window->SetContent(Body);

		Title = MakeText(T, LabelPx, PawTokens::INK, false, TEXT("Title"));
		Title->SetJustification(ETextJustify::Center);
		Title->SetText(PawText::Ui(TEXT("PAUSE_title")));
		Body->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);

		// The pages share one place (Godot's Pages control, 360 tall): one shows at a time.
		UOverlay* Pages = T->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Pages"));
		UVerticalBoxSlot* PagesSlot = Body->AddChildToVerticalBox(Pages);
		PagesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		PagesSlot->SetPadding(FMargin(0.0f, static_cast<float>(PauseBodySeparation), 0.0f, 0.0f));

		MainPage = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainPage"));
		UOverlaySlot* MS = Pages->AddChildToOverlay(MainPage);
		MS->SetHorizontalAlignment(HAlign_Fill);
		MS->SetVerticalAlignment(VAlign_Fill);
		const double Gap = PauseBodySeparation;
		ContinueButton = PageButton(T, MainPage, TEXT("ContinueButton"), TEXT("PAUSE_continue"), PauseButtonHeight, true, Gap);
		SaveButton = PageButton(T, MainPage, TEXT("SaveButton"), TEXT("PAUSE_save"), PauseButtonHeight, false, Gap);
		SettingsButton = PageButton(T, MainPage, TEXT("SettingsButton"), TEXT("PAUSE_settings"), PauseButtonHeight, false, Gap);
		MainMenuButton = PageButton(T, MainPage, TEXT("MainMenuButton"), TEXT("PAUSE_main_menu"), PauseButtonHeight, false, Gap);
		// Built beside the button it qualifies, shown only mid-decision.
		DiscardButton = PageButton(T, MainPage, TEXT("DiscardButton"), TEXT("PAUSE_discard"), 0.0, false, Gap);
		DiscardButton->SetVisibility(ESlateVisibility::Collapsed);
		StatusText = MakeText(T, LabelPx, PawTokens::UI_GOOD, false, TEXT("StatusText"));
		StatusText->SetJustification(ETextJustify::Center);
		StatusText->SetAutoWrapText(false);
		StatusText->SetWrapTextAt(static_cast<float>(PauseWindowWidth - 2.0 * PausePadX));
		MainPage->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, static_cast<float>(Gap), 0.0f, 0.0f));

		SlotPage = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotPage"));
		SlotPage->SetVisibility(ESlateVisibility::Collapsed);
		UOverlaySlot* SS = Pages->AddChildToOverlay(SlotPage);
		SS->SetHorizontalAlignment(HAlign_Fill);
		SS->SetVerticalAlignment(VAlign_Fill);
		SlotTitle = MakeText(T, LabelPx, PawTokens::UI_TEXT, false, TEXT("SlotTitle"));
		SlotTitle->SetJustification(ETextJustify::Center);
		SlotTitle->SetText(PawText::Ui(TEXT("PAUSE_slot_title")));
		SlotPage->AddChildToVerticalBox(SlotTitle)->SetHorizontalAlignment(HAlign_Center);
		SlotList = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotList"));
		UVerticalBoxSlot* ListSlot = SlotPage->AddChildToVerticalBox(SlotList);
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ListSlot->SetPadding(FMargin(0.0f, static_cast<float>(PauseSlotSeparation), 0.0f, 0.0f));
		for (int32 I = 0; I < SlotCount; ++I)
		{
			UPawButton* B = PageButton(T, SlotList, *PawUi::Numbered(TEXT("SlotButton"), I).ToString(), nullptr, PauseSlotHeight, I == 0,
				PauseSlotSeparation);
			B->SetWrapAt(static_cast<float>(PauseWindowWidth - 2.0 * PausePadX - 40.0));
			SlotButtons.Add(B);
		}
		SlotBackButton = PageButton(T, SlotPage, TEXT("SlotBackButton"), TEXT("PAUSE_back"), PauseSlotBackHeight, false, PauseSlotSeparation);
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		OnLayoutReady();
	}
	return bOk;
}

void UPawPauseMenu::OnLayoutReady()
{
	PawUi::CollectNamed(WidgetTree, TEXT("SlotButton"), SlotCount, SlotButtons);
	auto Click = [this](UPawButton* B, TFunction<void()> Do)
	{
		if (B)
		{
			B->OnClicked().AddWeakLambda(this, MoveTemp(Do));
		}
	};
	Click(ContinueButton, [this]() { PressContinue(); });
	Click(SaveButton, [this]() { PressSave(); });
	Click(MainMenuButton, [this]() { PressMainMenu(); });
	Click(DiscardButton, [this]() { PressDiscard(); });
	Click(SlotBackButton, [this]() { PressSlotsBack(); });
	for (int32 I = 0; I < SlotCount; ++I)
	{
		Click(GetSlotButton(I), [this, I]() { PressSlot(I); });
	}
	Click(SettingsButton, [this]() { PressSettings(); });
	if (!OnLeave)
	{
		OnLeave = [this]() { UGameplayStatics::OpenLevel(this, FName(TitleMap)); };
	}
}

UPawRealmSubsystem* UPawPauseMenu::Realm() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
}

UPawSaveSubsystem* UPawPauseMenu::Saves() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UPawSaveSubsystem>() : nullptr;
}

TOptional<FUIInputConfig> UPawPauseMenu::GetDesiredInputConfig() const
{
	// A modal over a held field: the game's input stays out; its buttons are Slate widgets.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

bool UPawPauseMenu::CanTakeFocus() const
{
	return IsActivated() && GetOwningLocalPlayer() != nullptr;
}

void UPawPauseMenu::NativeOnActivated()
{
	Super::NativeOnActivated();
	UPawAudioSubsystem::PlayFrom(this, TEXT("ui_click")); // pause_menu.gd open()
	ReturnFocus = nullptr;
	// open(): the main page, the status cleared, the tree paused (a holder: it resumes only when no one else holds it).
	Pc::PauseOpen(State);
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, true);
	}
	ShowPage();
	FocusPage(nullptr);
}

void UPawPauseMenu::NativeOnDeactivated()
{
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, false);
	}
	Super::NativeOnDeactivated();
}

// --- pages ---------------------------------------------------------------------------------------------------------------

void UPawPauseMenu::ShowPage()
{
	if (MainPage)
	{
		MainPage->SetVisibility(State.bSlotsPage ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (SlotPage)
	{
		SlotPage->SetVisibility(State.bSlotsPage ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	RefreshMain();
}

void UPawPauseMenu::RefreshMain()
{
	if (MainMenuButton)
	{
		MainMenuButton->SetLabel(PawText::Ui(State.bExitPending ? TEXT("PAUSE_save_quit") : TEXT("PAUSE_main_menu")));
	}
	if (DiscardButton)
	{
		DiscardButton->SetVisibility(State.bExitPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (StatusText)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), PawText::Count(State.StatusSlot + 1));
		StatusText->SetText(State.Status == Pc::PcPauseStatus::None ? FText::GetEmpty()
			: PawText::Ui(State.Status == Pc::PcPauseStatus::Occupied ? TEXT("PAUSE_status_occupied") : TEXT("PAUSE_status_saved"), Args));
	}
}

void UPawPauseMenu::RefreshSlots()
{
	// _refresh_slots(): the labels are read again every time, a slot's contents change constantly while the game runs.
	UPawSaveSubsystem* S = Saves();
	const int64 Active = S ? S->GetActiveSlot() : 0;
	for (int32 I = 0; I < SlotCount; ++I)
	{
		UPawButton* B = GetSlotButton(I);
		if (!B)
		{
			continue;
		}
		Pc::PcSlotSummary Summary;
		const bool bRead = S && S->ReadSlotSummary(I, Summary);
		const Pc::PcPauseSlotLine L = Pc::PauseSlotLine(State, I, bRead, Active);
		// Godot splices " (current)" in code; here each form has a key with it and one without (word order is the translator's).
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), PawText::Count(I + 1));
		switch (L.Form)
		{
		case Pc::PcPauseSlotForm::Empty:
			B->SetLabel(PawText::Ui(L.bCurrent ? TEXT("PAUSE_slot_empty_current") : TEXT("PAUSE_slot_empty"), Args));
			break;
		case Pc::PcPauseSlotForm::Overwrite:
			B->SetLabel(PawText::Ui(L.bCurrent ? TEXT("PAUSE_slot_overwrite_current") : TEXT("PAUSE_slot_overwrite"), Args));
			break;
		default:
			Args.Add(TEXT("reawakens"), PawText::Count(Summary.Reawakens));
			Args.Add(TEXT("time"), PawText::SlotClock(Summary.PlaySeconds));
			Args.Add(TEXT("mice"), PawText::Mice(Summary.Kills));
			B->SetLabel(PawText::Ui(L.bCurrent ? TEXT("PAUSE_slot_summary_current") : TEXT("PAUSE_slot_summary"), Args));
			break;
		}
	}
}

void UPawPauseMenu::FocusPage(UWidget* On)
{
	TArray<TArray<UWidget*>> Rows;
	UWidget* Default = nullptr;
	if (State.bSlotsPage)
	{
		for (int32 I = 0; I < SlotCount; ++I)
		{
			Rows.Add({ GetSlotButton(I) });
		}
		Rows.Add({ SlotBackButton.Get() });
		const UPawSaveSubsystem* S = Saves();
		Default = GetSlotButton(static_cast<int32>(Pc::PauseSlotFocus(S ? S->GetActiveSlot() : 0, SlotCount)));
	}
	else
	{
		// A column opening on Continue; the discard joins it only while it shows.
		for (UPawButton* B : { ContinueButton.Get(), SaveButton.Get(), SettingsButton.Get(), MainMenuButton.Get(), DiscardButton.Get() })
		{
			if (B && B->GetIsEnabled())
			{
				Rows.Add({ B });
			}
		}
		Default = ContinueButton;
	}
	Wired = WireRows(Rows);
	if (CanTakeFocus())
	{
		if (UWidget* Target = On ? On : Default)
		{
			Target->SetFocus();
		}
	}
}

UWidget* UPawPauseMenu::NativeGetDesiredFocusTarget() const
{
	if (State.bSlotsPage)
	{
		const UPawSaveSubsystem* S = Saves();
		return GetSlotButton(static_cast<int32>(Pc::PauseSlotFocus(S ? S->GetActiveSlot() : 0, SlotCount)));
	}
	return ReturnFocus.IsValid() ? ReturnFocus.Get() : ContinueButton.Get();
}

// --- presses ---------------------------------------------------------------------------------------------------------------

void UPawPauseMenu::Close()
{
	Pc::PauseClose(State);
	RefreshMain();
	if (IsActivated())
	{
		DeactivateWidget();
	}
}

void UPawPauseMenu::SetModalStack(UCommonActivatableWidgetStack* Stack)
{
	ModalStack = Stack;
}

void UPawPauseMenu::PressSettings()
{
	// _on_settings_pressed(): steps ASIDE (hidden, still holding the field) for the dialog, and comes back when it closes,
	// the selection on the button that went there. Not dropped into the game: the player did not press Continue.
	UPawSettingsDialog* Dialog = UPawSettingsDialog::Open(ModalStack.Get(), [WeakThis = TWeakObjectPtr<UPawPauseMenu>(this)]()
	{
		if (UPawPauseMenu* Self = WeakThis.Get())
		{
			Self->bSettingsUp = false;
			Self->SetVisibility(Self->VisibleAs);
			Self->ReturnFocus = Self->SettingsButton;
			Self->FocusPage(Self->SettingsButton);
		}
	});
	if (!Dialog)
	{
		return;
	}
	if (!bSettingsUp)
	{
		VisibleAs = GetVisibility();
	}
	bSettingsUp = true;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPawPauseMenu::PressContinue()
{
	Close();
}

void UPawPauseMenu::PressSave()
{
	Pc::PauseShowSlots(State);
	RefreshSlots();
	ShowPage();
	FocusPage(nullptr);
}

void UPawPauseMenu::PressSlotsBack()
{
	Pc::PauseShowMain(State);
	ShowPage();
	FocusPage(nullptr);
}

void UPawPauseMenu::PressSlot(int32 SlotIndex)
{
	UPawSaveSubsystem* S = Saves();
	const Pc::PcPauseAction A = Pc::PausePressSlot(State, SlotIndex, S && S->HasSlot(SlotIndex));
	if (A == Pc::PcPauseAction::SaveToSlot)
	{
		if (S)
		{
			S->SaveGame(SlotIndex);
		}
		ShowPage();
		FocusPage(nullptr);
		return;
	}
	// Armed: the prompt is on the button itself (the status line sits on the main page); the selection stays there.
	RefreshSlots();
	RefreshMain();
	FocusPage(GetSlotButton(SlotIndex));
}

void UPawPauseMenu::PressMainMenu()
{
	if (Pc::PausePressQuit(State) == Pc::PcPauseAction::SaveAndLeave)
	{
		Leave(true);
		return;
	}
	// Armed: the selection STAYS on this button (its second press saves and quits, which loses nothing); the destructive
	// discard is the other control, never under the key.
	RefreshMain();
	FocusPage(MainMenuButton);
}

void UPawPauseMenu::PressDiscard()
{
	if (State.bExitPending && Pc::PausePressDiscard(State) == Pc::PcPauseAction::Leave)
	{
		Leave(false);
	}
}

void UPawPauseMenu::Leave(bool bSave)
{
	if (bSave)
	{
		if (UPawSaveSubsystem* S = Saves())
		{
			S->SaveGame();
			S->Flush();
		}
	}
	Pc::PauseLeave(State);
	RefreshMain();
	if (IsActivated())
	{
		DeactivateWidget();
	}
	if (OnLeave)
	{
		OnLeave();
	}
}

void UPawPauseMenu::PressBackOrPause(bool bBack)
{
	// Esc is both back and pause; on the Save page it means back. Start (pause alone) closes from either page.
	if (State.bSlotsPage && bBack)
	{
		PressSlotsBack();
		return;
	}
	Close();
}

FReply UPawPauseMenu::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// pause_menu.gd _process: back and pause, through the input map (Esc is both, pad B back, pad Start pause).
	const FKey K = KeyEvent.GetKey();
	const bool bBack = PawInput::KeyIs(K, TEXT("back"));
	const bool bPause = PawInput::KeyIs(K, TEXT("pause"));
	if (bBack || bPause)
	{
		if (!KeyEvent.IsRepeat())
		{
			PressBackOrPause(bBack);
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

// --- test views ------------------------------------------------------------------------------------------------------------

UPawButton* UPawPauseMenu::GetButton(const TCHAR* Name) const
{
	return WidgetTree ? Cast<UPawButton>(WidgetTree->FindWidget(FName(Name))) : nullptr;
}

UPawButton* UPawPauseMenu::GetSlotButton(int32 SlotIndex) const
{
	return SlotButtons.IsValidIndex(SlotIndex) ? SlotButtons[SlotIndex].Get() : nullptr;
}

FText UPawPauseMenu::GetStatusText() const { return StatusText ? StatusText->GetText() : FText::GetEmpty(); }
FText UPawPauseMenu::GetTitleText() const { return Title ? Title->GetText() : FText::GetEmpty(); }

bool UPawPauseMenu::IsPageShown(bool bSlots) const
{
	const UWidget* Page = bSlots ? static_cast<const UWidget*>(SlotPage.Get()) : static_cast<const UWidget*>(MainPage.Get());
	return Page && Page->GetVisibility() != ESlateVisibility::Collapsed && Page->GetVisibility() != ESlateVisibility::Hidden;
}
