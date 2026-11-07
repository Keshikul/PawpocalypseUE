#include "UI/PawControlsPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Input/Events.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Input/PawInputMap.h"
#include "Input/PawKeyCodes.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawPrompts.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

#include <string>

namespace
{
	using namespace PawUi;

	const TCHAR* const CheckKeys[Pc::ControlsCheckCount] = { TEXT("CONTROLS_show_prompts"), TEXT("CONTROLS_tutorial_tips"),
		TEXT("CONTROLS_swap_confirm"), TEXT("CONTROLS_swap_sticks"), TEXT("CONTROLS_invert_x"), TEXT("CONTROLS_invert_y") };

	// The rows' names, in PcBindings' row order (InputBindings.LABELS).
	const TCHAR* const RowKeys[Pc::BindingRowCount] = { TEXT("CONTROLS_row_pounce"), TEXT("CONTROLS_row_tab_prev"),
		TEXT("CONTROLS_row_tab_next"), TEXT("CONTROLS_row_up"), TEXT("CONTROLS_row_down"), TEXT("CONTROLS_row_press"),
		TEXT("CONTROLS_row_buy_amount"), TEXT("CONTROLS_row_skill_tree"), TEXT("CONTROLS_row_recentre"), TEXT("CONTROLS_row_pause") };

	FText RowName(int32 Row)
	{
		return PawText::Ui(RowKeys[FMath::Clamp(Row, 0, Pc::BindingRowCount - 1)]);
	}

	int32 CellIndex(int32 Row, Pc::PcBindDevice Device)
	{
		return Row * 2 + (Device == Pc::PcBindDevice::Pad ? 1 : 0);
	}

	UPawButton* Check(UWidgetTree* T, UVerticalBox* Body, const TCHAR* Name, const TCHAR* Key)
	{
		UPawButton* B = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), Name);
		B->SetLabel(PawText::Ui(Key));
		B->SetLabelSize(SettingsCheckPx);
		B->SetAlignLeft(true);
		B->SetFlat(true);
		B->SetCheck(true, false);
		B->SetMinDimensions(0, static_cast<int32>(SettingsRowHeight));
		UVerticalBoxSlot* S = Body->AddChildToVerticalBox(B);
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetPadding(FMargin(0.0f, static_cast<float>(ControlsSeparation), 0.0f, 0.0f));
		return B;
	}

	USlider* Slider(UWidgetTree* T, UVerticalBox* Body, const TCHAR* Name, Pc::PcControlsSlider Which)
	{
		USlider* S = T->ConstructWidget<USlider>(USlider::StaticClass(), Name);
		S->SetMinValue(static_cast<float>(Pc::ControlsSliderMin(Which)));
		S->SetMaxValue(static_cast<float>(Pc::ControlsSliderMax(Which)));
		S->SetStepSize(static_cast<float>(Pc::ControlsSliderStep(Which)));
		S->RequiresControllerLock = false; // left / right move it at once, as Godot's HSlider
		S->MouseUsesStep = true;
		S->IsFocusable = true;
		S->SetSliderBarColor(PawTokens::UI_TEXT_MUTED);
		S->SetSliderHandleColor(PawTokens::UI_BUTTON_FRAME);
		USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(FString(Name) + TEXT("Box"))));
		Box->SetMinDesiredHeight(static_cast<float>(SettingsSliderHeight));
		Box->SetContent(S);
		Body->AddChildToVerticalBox(Box)->SetPadding(FMargin(0.0f, static_cast<float>(ControlsSeparation), 0.0f, 0.0f));
		return S;
	}

	UTextBlock* Line(UWidgetTree* T, UVerticalBox* Body, const TCHAR* Name, const FLinearColor& Colour)
	{
		UTextBlock* L = MakeText(T, LabelPx, Colour, false, Name);
		L->SetAutoWrapText(false);
		L->SetWrapTextAt(static_cast<float>(SettingsWindowWidth - 2.0 * PausePadX - 24.0)); // a fixed width (KB U/09 §3)
		Body->AddChildToVerticalBox(L)->SetPadding(FMargin(0.0f, static_cast<float>(ControlsSeparation), 0.0f, 0.0f));
		return L;
	}
}

bool UPawControlsPanel::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		// A box that scrolls, as Godot's ScrollContainer: the selection pulls the row it lands on into view (follow_focus).
		USizeBox* Size = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ScrollSize"));
		Size->SetHeightOverride(static_cast<float>(ControlsScrollHeight));
		WidgetTree->RootWidget = Size;
		Scroll = T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Scroll"));
		Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
		Size->SetContent(Scroll);
		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
		Scroll->AddChild(Body);

		ShowPromptsCheck = Check(T, Body, TEXT("ShowPromptsCheck"), CheckKeys[0]);
		TutorialTipsCheck = Check(T, Body, TEXT("TutorialTipsCheck"), CheckKeys[1]);

		// The table's head: the two columns' names over the cells, nothing over the labels or the Resets.
		UHorizontalBox* Head = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TableHead"));
		Body->AddChildToVerticalBox(Head)->SetPadding(FMargin(0.0f, static_cast<float>(ControlsSeparation), 0.0f, 0.0f));
		USpacer* HeadSpace = T->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("HeadSpace"));
		Head->AddChildToHorizontalBox(HeadSpace)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		auto HeadText = [T, Head](const TCHAR* Name, const TCHAR* Key)
		{
			UTextBlock* L = MakeText(T, ControlsSmallPx, PawTokens::UI_TEXT, false, Name);
			L->SetText(PawText::Ui(Key));
			L->SetJustification(ETextJustify::Center);
			L->SetAutoWrapText(false);
			L->SetWrapTextAt(static_cast<float>(ControlsCellWidth));
			USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(FString(Name) + TEXT("Box"))));
			Box->SetWidthOverride(static_cast<float>(ControlsCellWidth));
			Box->SetContent(L);
			Head->AddChildToHorizontalBox(Box)->SetPadding(FMargin(static_cast<float>(ControlsSeparation), 0.0f, 0.0f, 0.0f));
			return L;
		};
		KeyboardHead = HeadText(TEXT("KeyboardHead"), TEXT("CONTROLS_head_keyboard"));
		PadHead = HeadText(TEXT("PadHead"), TEXT("CONTROLS_head_pad"));
		USpacer* HeadPad = T->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("HeadPad"));
		HeadPad->SetSize(FVector2D(ControlsResetWidth, 0.0));
		Head->AddChildToHorizontalBox(HeadPad)->SetPadding(FMargin(static_cast<float>(ControlsSeparation), 0.0f, 0.0f, 0.0f));

		BindingTable = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BindingTable"));
		Body->AddChildToVerticalBox(BindingTable);

		Status = Line(T, Body, TEXT("Status"), PawTokens::UI_TEXT_MUTED);

		ResetAllButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("ResetAllButton"));
		ResetAllButton->SetLabel(PawText::Ui(TEXT("CONTROLS_reset_all")));
		ResetAllButton->SetMinDimensions(0, static_cast<int32>(SettingsRowHeight));
		Body->AddChildToVerticalBox(ResetAllButton)->SetPadding(FMargin(0.0f, static_cast<float>(ControlsSeparation), 0.0f, 0.0f));

		SwapConfirmCheck = Check(T, Body, TEXT("SwapConfirmCheck"), CheckKeys[2]);
		SwapSticksCheck = Check(T, Body, TEXT("SwapSticksCheck"), CheckKeys[3]);
		InvertXCheck = Check(T, Body, TEXT("InvertXCheck"), CheckKeys[4]);
		InvertYCheck = Check(T, Body, TEXT("InvertYCheck"), CheckKeys[5]);
		CursorSpeedLabel = Line(T, Body, TEXT("CursorSpeedLabel"), PawTokens::UI_TEXT);
		CursorSpeedSlider = Slider(T, Body, TEXT("CursorSpeedSlider"), Pc::PcControlsSlider::CursorSpeed);
		DeadzoneLabel = Line(T, Body, TEXT("DeadzoneLabel"), PawTokens::UI_TEXT);
		DeadzoneSlider = Slider(T, Body, TEXT("DeadzoneSlider"), Pc::PcControlsSlider::StickDeadzone);
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		OnLayoutReady();
	}
	return bOk;
}

void UPawControlsPanel::OnLayoutReady()
{
	BuildRows();
	auto Click = [this](UPawButton* B, TFunction<void()> Do)
	{
		if (B)
		{
			B->OnClicked().AddWeakLambda(this, MoveTemp(Do));
		}
	};
	UPawButton* Checks[Pc::ControlsCheckCount] = { ShowPromptsCheck, TutorialTipsCheck, SwapConfirmCheck, SwapSticksCheck, InvertXCheck, InvertYCheck };
	for (int32 I = 0; I < Pc::ControlsCheckCount; ++I)
	{
		Click(Checks[I], [this, I]() { ToggleCheck(static_cast<Pc::PcControlsCheck>(I)); });
	}
	Click(ResetAllButton, [this]() { PressResetAll(); });
	if (CursorSpeedSlider) { CursorSpeedSlider->OnValueChanged.AddUniqueDynamic(this, &UPawControlsPanel::OnCursorSlider); }
	if (DeadzoneSlider) { DeadzoneSlider->OnValueChanged.AddUniqueDynamic(this, &UPawControlsPanel::OnDeadzoneSlider); }
	Sync();
}

void UPawControlsPanel::BuildRows()
{
	// _build_row(): the label (filling), the keyboard cell, the controller cell (each a button with its prompt over it), Reset.
	if (!BindingTable || !WidgetTree)
	{
		return;
	}
	BindingTable->ClearChildren();
	RowLabels.Reset();
	Cells.Reset();
	CellPrompts.Reset();
	Resets.Reset();
	UWidgetTree* T = WidgetTree;
	for (int32 Row = 0; Row < Pc::BindingRowCount; ++Row)
	{
		UHorizontalBox* Box = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		BindingTable->AddChildToVerticalBox(Box)->SetPadding(FMargin(0.0f, static_cast<float>(ControlsSeparation), 0.0f, 0.0f));
		UTextBlock* Label = MakeText(T, LabelPx, PawTokens::UI_TEXT);
		Label->SetText(RowName(Row));
		Label->SetAutoWrapText(true);
		UHorizontalBoxSlot* LabelSlot = Box->AddChildToHorizontalBox(Label);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		RowLabels.Add(Label);
		const std::string Primary = Pc::BindingMembers(Pc::BindingRowId(Row))[0];
		for (const Pc::PcBindDevice Device : { Pc::PcBindDevice::Keyboard, Pc::PcBindDevice::Pad })
		{
			UOverlay* Cell = T->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			UPawButton* Button = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
			Button->SetMinDimensions(static_cast<int32>(ControlsCellWidth), static_cast<int32>(ControlsCellHeight));
			UOverlaySlot* ButtonSlot = Cell->AddChildToOverlay(Button);
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
			UPawPromptIcon* Prompt = T->ConstructWidget<UPawPromptIcon>(PawUi::Resolve<UPawPromptIcon>());
			Prompt->bAlwaysShow = true; // the cells ARE the bindings: shown whatever Show button prompts says
			Prompt->SetIconPx(ControlsIconPx);
			Prompt->SetAction(UTF8_TO_TCHAR(Primary.c_str()));
			Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* PromptSlot = Cell->AddChildToOverlay(Prompt);
			PromptSlot->SetHorizontalAlignment(HAlign_Center);
			PromptSlot->SetVerticalAlignment(VAlign_Center);
			Box->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(static_cast<float>(ControlsSeparation), 0.0f, 0.0f, 0.0f));
			const int32 R = Row;
			Button->OnClicked().AddWeakLambda(this, [this, R, Device]() { PressCell(R, Device); });
			Cells.Add(Button);
			CellPrompts.Add(Prompt);
		}
		UPawButton* Reset = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Reset->SetLabel(PawText::Ui(TEXT("CONTROLS_reset")));
		Reset->SetLabelSize(ControlsSmallPx);
		Reset->SetMinDimensions(static_cast<int32>(ControlsResetWidth), static_cast<int32>(ControlsCellHeight));
		Box->AddChildToHorizontalBox(Reset)->SetPadding(FMargin(static_cast<float>(ControlsSeparation), 0.0f, 0.0f, 0.0f));
		const int32 R = Row;
		Reset->OnClicked().AddWeakLambda(this, [this, R]() { PressReset(R); });
		Resets.Add(Reset);
	}
}

void UPawControlsPanel::NativeConstruct()
{
	Super::NativeConstruct();
	// bindings_changed / prompt_style_changed / scheme_changed -> sync.
	if (UPawInputDeviceSubsystem* D = Device())
	{
		if (!ControlsHandle.IsValid())
		{
			ControlsHandle = D->OnControlsChanged.AddWeakLambda(this, [this]() { Sync(); });
		}
		if (!FamilyHandle.IsValid())
		{
			FamilyHandle = D->OnFamilyChanged.AddWeakLambda(this, [this](Pc::PcFamily) { Sync(); });
		}
	}
	Sync();
}

void UPawControlsPanel::NativeDestruct()
{
	if (UPawInputDeviceSubsystem* D = Device())
	{
		D->OnControlsChanged.Remove(ControlsHandle);
		D->OnFamilyChanged.Remove(FamilyHandle);
	}
	ControlsHandle.Reset();
	FamilyHandle.Reset();
	StopTicker();
	Super::NativeDestruct();
}

UPawSettingsSubsystem* UPawControlsPanel::Settings() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
}

UPawInputDeviceSubsystem* UPawControlsPanel::Device() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
}

Pc::PcSettings UPawControlsPanel::Current() const
{
	const UPawSettingsSubsystem* S = Settings();
	return S ? S->Get() : Pc::PcSettings();
}

// --- what the page shows -----------------------------------------------------------------------------------------------------

void UPawControlsPanel::Sync()
{
	const Pc::PcSettings S = Current();
	const UPawInputDeviceSubsystem* D = Device();
	const Pc::PcFamily Family = D ? D->GetFamily() : Pc::PcFamily::Keyboard;
	const Pc::PcFamily Detected = D ? D->GetState().Detected : Pc::PcFamily::Keyboard;
	DrawnPadColumn = Pc::ControlsPadColumnFamily(S.PromptStyle, Family, Detected);
	for (int32 Row = 0; Row < Pc::BindingRowCount && Row < RowLabels.Num(); ++Row)
	{
		const std::string Id = Pc::BindingRowId(Row);
		RowLabels[Row]->SetText(RowName(Row));
		for (const Pc::PcBindDevice Device : { Pc::PcBindDevice::Keyboard, Pc::PcBindDevice::Pad })
		{
			const int32 I = CellIndex(Row, Device);
			UPawButton* Cell = Cells[I];
			UPawPromptIcon* Prompt = CellPrompts[I];
			Prompt->SetFamilyOverride(static_cast<int32>(Device == Pc::PcBindDevice::Keyboard ? Pc::PcFamily::Keyboard : DrawnPadColumn));
			// A locked cell is Godot's disabled one: it reads, it cannot be pressed.
			Cell->SetLocked(Pc::BindingLocked(Id, Device));
			if (Pc::ControlsCellListening(State, Id, Device))
			{
				Prompt->SetVisibility(ESlateVisibility::Collapsed);
				Cell->SetLabel(PawText::Ui(TEXT("CONTROLS_listening_cell")));
			}
			else
			{
				Prompt->SetVisibility(ESlateVisibility::HitTestInvisible);
				Cell->SetLabel(Prompt->ShowsAnything() ? FText::GetEmpty() : PawText::Ui(TEXT("CONTROLS_no_binding")));
			}
		}
		Resets[Row]->SetLabel(PawText::Ui(TEXT("CONTROLS_reset")));
		Resets[Row]->SetLocked(!Pc::ControlsResetEnabled(Id));
	}
	for (int32 I = 0; I < Pc::ControlsCheckCount; ++I)
	{
		if (UPawButton* B = GetCheck(static_cast<Pc::PcControlsCheck>(I)))
		{
			B->SetCheck(true, Pc::ControlsCheckValue(S, static_cast<Pc::PcControlsCheck>(I)));
		}
	}
	bSyncing = true;
	for (const Pc::PcControlsSlider Which : { Pc::PcControlsSlider::CursorSpeed, Pc::PcControlsSlider::StickDeadzone })
	{
		if (USlider* Sl = GetSlider(Which))
		{
			Sl->SetValue(static_cast<float>(Pc::ControlsSliderSnap(Which, Pc::ControlsSliderSetting(S, Which))));
		}
		if (UTextBlock* L = Which == Pc::PcControlsSlider::CursorSpeed ? CursorSpeedLabel.Get() : DeadzoneLabel.Get())
		{
			L->SetText(GetSliderLine(Which));
		}
	}
	bSyncing = false;
	RefreshStatus();
}

FText UPawControlsPanel::GetSliderLine(Pc::PcControlsSlider Which) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("n"), PawText::Count(Pc::ControlsPercent(Pc::ControlsSliderSetting(Current(), Which))));
	return PawText::Ui(Which == Pc::PcControlsSlider::CursorSpeed ? TEXT("CONTROLS_cursor_speed") : TEXT("CONTROLS_deadzone"), Args);
}

FText UPawControlsPanel::GetStatusText() const
{
	auto Named = [this](const TCHAR* Key)
	{
		FFormatNamedArguments Args;
		const int32 Row = Pc::BindingRowIndex(State.LineRow);
		Args.Add(TEXT("action"), Row >= 0 ? RowName(Row) : FText::GetEmpty());
		return PawText::Ui(Key, Args);
	};
	switch (State.Line)
	{
	case Pc::PcControlsLine::None: return FText::GetEmpty();
	case Pc::PcControlsLine::Listening: return Named(TEXT("CONTROLS_line_listening"));
	case Pc::PcControlsLine::NothingPressed: return PawText::Ui(TEXT("CONTROLS_line_nothing"));
	case Pc::PcControlsLine::Cancelled: return PawText::Ui(TEXT("CONTROLS_line_cancelled"));
	case Pc::PcControlsLine::Done: return PawText::Ui(TEXT("CONTROLS_line_done"));
	case Pc::PcControlsLine::Swapped: return Named(TEXT("CONTROLS_line_swapped"));
	case Pc::PcControlsLine::RefusedSystem: return PawText::Ui(TEXT("CONTROLS_line_system"));
	case Pc::PcControlsLine::Taken: return PawText::Ui(TEXT("CONTROLS_line_taken"));
	case Pc::PcControlsLine::Reset: return PawText::Ui(TEXT("CONTROLS_line_reset"));
	case Pc::PcControlsLine::ResetAll: return PawText::Ui(TEXT("CONTROLS_line_reset_all"));
	}
	return FText::GetEmpty();
}

void UPawControlsPanel::RefreshStatus()
{
	if (Status)
	{
		Status->SetText(GetStatusText());
	}
}

TArray<TArray<UWidget*>> UPawControlsPanel::FocusRows() const
{
	TArray<TArray<UWidget*>> Rows;
	Rows.Add({ ShowPromptsCheck.Get() });
	Rows.Add({ TutorialTipsCheck.Get() });
	for (int32 Row = 0; Row < Pc::BindingRowCount && Row < Resets.Num(); ++Row)
	{
		Rows.Add({ Cells[CellIndex(Row, Pc::PcBindDevice::Keyboard)].Get(), Cells[CellIndex(Row, Pc::PcBindDevice::Pad)].Get(), Resets[Row].Get() });
	}
	Rows.Add({ ResetAllButton.Get() });
	for (UPawButton* B : { SwapConfirmCheck.Get(), SwapSticksCheck.Get(), InvertXCheck.Get(), InvertYCheck.Get() })
	{
		Rows.Add({ B });
	}
	Rows.Add({ CursorSpeedSlider.Get() });
	Rows.Add({ DeadzoneSlider.Get() });
	return Rows;
}

UPawButton* UPawControlsPanel::GetCell(int32 Row, Pc::PcBindDevice Device) const
{
	const int32 I = CellIndex(Row, Device);
	return Cells.IsValidIndex(I) ? Cells[I].Get() : nullptr;
}

UPawPromptIcon* UPawControlsPanel::GetCellPrompt(int32 Row, Pc::PcBindDevice Device) const
{
	const int32 I = CellIndex(Row, Device);
	return CellPrompts.IsValidIndex(I) ? CellPrompts[I].Get() : nullptr;
}

UPawButton* UPawControlsPanel::GetReset(int32 Row) const
{
	return Resets.IsValidIndex(Row) ? Resets[Row].Get() : nullptr;
}

UTextBlock* UPawControlsPanel::GetRowLabel(int32 Row) const
{
	return RowLabels.IsValidIndex(Row) ? RowLabels[Row].Get() : nullptr;
}

UPawButton* UPawControlsPanel::GetCheck(Pc::PcControlsCheck Check) const
{
	switch (Check)
	{
	case Pc::PcControlsCheck::ShowPrompts: return ShowPromptsCheck;
	case Pc::PcControlsCheck::TutorialTips: return TutorialTipsCheck;
	case Pc::PcControlsCheck::SwapConfirmBack: return SwapConfirmCheck;
	case Pc::PcControlsCheck::SwapSticks: return SwapSticksCheck;
	case Pc::PcControlsCheck::InvertX: return InvertXCheck;
	case Pc::PcControlsCheck::InvertY: return InvertYCheck;
	}
	return nullptr;
}

USlider* UPawControlsPanel::GetSlider(Pc::PcControlsSlider Which) const
{
	return Which == Pc::PcControlsSlider::CursorSpeed ? CursorSpeedSlider.Get() : DeadzoneSlider.Get();
}

// --- what a player does --------------------------------------------------------------------------------------------------------

void UPawControlsPanel::PressCell(int32 Row, Pc::PcBindDevice Device)
{
	// A locked cell is disabled in Godot: a press does nothing.
	if (Row < 0 || Row >= Pc::BindingRowCount || Pc::BindingLocked(Pc::BindingRowId(Row), Device))
	{
		return;
	}
	Pc::ControlsListen(State, Pc::BindingRowId(Row), Device);
	StartTicker();
	Sync();
}

void UPawControlsPanel::SaveBindings(const Pc::PcSettings& S)
{
	// set_bindings(): saved, applied (the device subsystem), and the page redrawn on OnControlsChanged.
	if (UPawSettingsSubsystem* Sub = Settings())
	{
		Sub->SetBindings(S.Bindings);
	}
}

Pc::PcBindResult UPawControlsPanel::Bind(int32 Row, const Pc::PcInputEvent& Event)
{
	const int32 RowWas = Pc::BindingRowIndex(State.ListenRow);
	const Pc::PcBindDevice DeviceWas = State.ListenDevice;
	Pc::PcSettings S = Current();
	bool bSaved = false;
	const Pc::PcBindResult Result = Pc::ControlsBind(State, S, PawInput::DefaultState(), Pc::BindingRowId(Row), Event, bSaved);
	if (bSaved)
	{
		SaveBindings(S);
	}
	EndListen(RowWas, DeviceWas);
	return Result;
}

void UPawControlsPanel::EndListen(int32 RowWas, Pc::PcBindDevice DeviceWas)
{
	// _end_listen(): the line, a redraw, and the selection stays on the cell that listened.
	StopTicker();
	Sync();
	if (UPawButton* Cell = RowWas >= 0 ? GetCell(RowWas, DeviceWas) : nullptr)
	{
		if (GetOwningLocalPlayer() && Cell->GetCachedWidget().IsValid())
		{
			Cell->SetFocus();
		}
	}
}

void UPawControlsPanel::PressReset(int32 Row)
{
	if (Row < 0 || Row >= Pc::BindingRowCount || !Pc::ControlsResetEnabled(Pc::BindingRowId(Row)))
	{
		return;
	}
	Pc::PcSettings S = Current();
	Pc::ControlsResetRow(State, S, PawInput::DefaultState(), Pc::BindingRowId(Row));
	SaveBindings(S);
	RefreshStatus();
}

void UPawControlsPanel::PressResetAll()
{
	Pc::PcSettings Ignored = Current();
	Pc::ControlsResetAll(State, Ignored); // the line; the settings are the subsystem's to reset
	if (UPawSettingsSubsystem* Sub = Settings())
	{
		Sub->ResetControls();
	}
	RefreshStatus();
}

void UPawControlsPanel::ToggleCheck(Pc::PcControlsCheck Check)
{
	UPawSettingsSubsystem* Sub = Settings();
	if (!Sub)
	{
		return;
	}
	const bool bOn = !Pc::ControlsCheckValue(Sub->Get(), Check);
	switch (Check)
	{
	case Pc::PcControlsCheck::ShowPrompts: Sub->SetShowPrompts(bOn); break;
	case Pc::PcControlsCheck::TutorialTips: Sub->SetTutorialTips(bOn); break;
	case Pc::PcControlsCheck::SwapConfirmBack: Sub->SetSwapConfirmBack(bOn); break;
	case Pc::PcControlsCheck::SwapSticks: Sub->SetSwapSticks(bOn); break;
	case Pc::PcControlsCheck::InvertX: Sub->SetInvertX(bOn); break;
	case Pc::PcControlsCheck::InvertY: Sub->SetInvertY(bOn); break;
	}
}

void UPawControlsPanel::MoveSlider(Pc::PcControlsSlider Which, double Value)
{
	// Range.set_value: snapped from the minimum; value_changed only when the slider moved, then set_control.
	const Pc::PcSettings S = Current();
	const double To = Pc::ControlsSliderSnap(Which, Value);
	if (To == Pc::ControlsSliderSnap(Which, Pc::ControlsSliderSetting(S, Which)))
	{
		bSyncing = true;
		if (USlider* Sl = GetSlider(Which))
		{
			Sl->SetValue(static_cast<float>(To));
		}
		bSyncing = false;
		return;
	}
	if (UPawSettingsSubsystem* Sub = Settings())
	{
		if (Which == Pc::PcControlsSlider::CursorSpeed)
		{
			Sub->SetCursorSpeed(To);
		}
		else
		{
			Sub->SetStickDeadzone(To);
		}
	}
}

void UPawControlsPanel::OnCursorSlider(float Value)
{
	if (!bSyncing)
	{
		MoveSlider(Pc::PcControlsSlider::CursorSpeed, Value);
	}
}

void UPawControlsPanel::OnDeadzoneSlider(float Value)
{
	if (!bSyncing)
	{
		MoveSlider(Pc::PcControlsSlider::StickDeadzone, Value);
	}
}

void UPawControlsPanel::ClearForTest()
{
	StopTicker();
	State = Pc::PcControlsState();
	Sync();
}

// --- listening ---------------------------------------------------------------------------------------------------------------

void UPawControlsPanel::StartTicker()
{
	if (!Ticker.IsValid())
	{
		// A core ticker: the dialog pauses the game (KB: anything moving while paused uses FTSTicker).
		Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float Dt)
		{
			Advance(Dt);
			return true;
		}));
	}
}

void UPawControlsPanel::StopTicker()
{
	if (Ticker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
		Ticker.Reset();
	}
}

void UPawControlsPanel::Advance(double DeltaSeconds)
{
	const int32 RowWas = Pc::BindingRowIndex(State.ListenRow);
	const Pc::PcBindDevice DeviceWas = State.ListenDevice;
	if (Pc::ControlsTick(State, DeltaSeconds))
	{
		EndListen(RowWas, DeviceWas);
	}
}

bool UPawControlsPanel::CaptureEvent(const Pc::PcCapturedEvent& Event, bool bBack)
{
	const int32 RowWas = Pc::BindingRowIndex(State.ListenRow);
	const Pc::PcBindDevice DeviceWas = State.ListenDevice;
	Pc::PcSettings S = Current();
	const Pc::PcControlsInput Out = Pc::ControlsInput(State, S, PawInput::DefaultState(), Event, false, bBack);
	switch (Out.Take)
	{
	case Pc::PcControlsTake::NotListening:
	case Pc::PcControlsTake::Passed:
		return false;
	case Pc::PcControlsTake::Swallowed:
		return true;
	case Pc::PcControlsTake::Cancelled:
	case Pc::PcControlsTake::Bound:
		if (Out.bSaved)
		{
			SaveBindings(S);
		}
		EndListen(RowWas, DeviceWas);
		return true;
	}
	return true;
}

bool UPawControlsPanel::CaptureKey(const FKeyEvent& Event, bool bDown)
{
	if (!State.bListening)
	{
		return false;
	}
	const FKey Key = Event.GetKey();
	// A stick's or a trigger's digital stand-in: swallowed; the analog value is what binds (or not).
	const Pc::PcInputEvent AsGodot = PawInput::EventOfKey(Key);
	if (AsGodot.Kind == Pc::PcInputKind::JoyAxis)
	{
		return true;
	}
	// event.is_action_pressed("back") or ("ui_cancel"), in the live map.
	const bool bBack = bDown && !Event.IsRepeat() && (PawInput::KeyIs(Key, TEXT("back")) || PawInput::KeyIs(Key, TEXT("ui_cancel")));
	return CaptureEvent(PawInput::Captured(Key, bDown, Event.IsRepeat()), bBack);
}

bool UPawControlsPanel::CaptureMouse(const FKey& Button, bool bDown)
{
	if (!State.bListening)
	{
		return false;
	}
	const bool bBack = bDown && (PawInput::KeyIs(Button, TEXT("back")) || PawInput::KeyIs(Button, TEXT("ui_cancel")));
	return CaptureEvent(PawInput::Captured(Button, bDown, false), bBack);
}

bool UPawControlsPanel::CaptureAnalog(const FAnalogInputEvent& Event)
{
	if (!State.bListening)
	{
		return false;
	}
	return CaptureEvent(PawInput::Captured(Event.GetKey(), true, false, true, Event.GetAnalogValue()), false);
}
