#include "UI/PawSettingsDialog.h"

#include "Audio/PawAudioSubsystem.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Input/PawInputMap.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawControlsPanel.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "Rendering/DrawElements.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

const FName UPawSettingsDialog::PauseHolder(TEXT("settings"));

namespace
{
	using namespace PawUi;


	const TCHAR* const SectionKeys[] = { TEXT("SETTINGS_section_audio"), TEXT("SETTINGS_section_video"), TEXT("SETTINGS_section_controls") };

	FString Str(const FText& T) { return T.ToString(); }

	bool Shown(const UWidget* W)
	{
		return W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden;
	}

	// A player's pick, not the dialog filling or syncing a list (Godot's item_selected fires only for the player).
	bool ByPlayer(ESelectInfo::Type How) { return How != ESelectInfo::Direct; }

	UTextBlock* PageLabel(UWidgetTree* T, UVerticalBox* Page, const TCHAR* Name, const TCHAR* Key, bool bFirst)
	{
		UTextBlock* L = MakeText(T, LabelPx, PawTokens::UI_TEXT, false, Name);
		if (Key)
		{
			L->SetText(PawText::Ui(Key));
		}
		Page->AddChildToVerticalBox(L)->SetPadding(FMargin(0.0f, bFirst ? 0.0f : static_cast<float>(SettingsPageSeparation), 0.0f, 0.0f));
		return L;
	}

	UTextBlock* PageNote(UWidgetTree* T, UVerticalBox* Page, const TCHAR* Name, const TCHAR* Key)
	{
		UTextBlock* L = PageLabel(T, Page, Name, Key, false);
		L->SetAutoWrapText(false);
		L->SetWrapTextAt(static_cast<float>(SettingsWindowWidth - 2.0 * PausePadX)); // a fixed width (KB U/09 §3)
		return L;
	}

	USlider* PageSlider(UWidgetTree* T, UVerticalBox* Page, const TCHAR* Name, Pc::PcSettingsSlider Which)
	{
		USlider* S = T->ConstructWidget<USlider>(USlider::StaticClass(), Name);
		S->SetMinValue(0.0f);
		S->SetMaxValue(1.0f);
		S->SetStepSize(static_cast<float>(Pc::SettingsSliderStep(Which)));
		// Left / right move it at once (Godot's HSlider), no "press to lock" first; a drag lands on a step too.
		S->RequiresControllerLock = false;
		S->MouseUsesStep = true;
		S->IsFocusable = true;
		S->SetSliderBarColor(PawTokens::UI_TEXT_MUTED);
		S->SetSliderHandleColor(PawTokens::UI_BUTTON_FRAME);
		USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(FString(Name) + TEXT("Box"))));
		Box->SetMinDesiredHeight(static_cast<float>(SettingsSliderHeight));
		Box->SetContent(S);
		Page->AddChildToVerticalBox(Box)->SetPadding(FMargin(0.0f, static_cast<float>(SettingsPageSeparation), 0.0f, 0.0f));
		return S;
	}

	UPawButton* PageCheck(UWidgetTree* T, UVerticalBox* Page, const TCHAR* Name, const TCHAR* Key)
	{
		UPawButton* B = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), Name);
		B->SetLabel(PawText::Ui(Key));
		B->SetLabelSize(SettingsCheckPx);
		B->SetAlignLeft(true);
		B->SetFlat(true);
		B->SetCheck(true, false);
		B->SetMinDimensions(0, static_cast<int32>(SettingsRowHeight));
		UVerticalBoxSlot* S = Page->AddChildToVerticalBox(B);
		S->SetHorizontalAlignment(HAlign_Left);
		S->SetPadding(FMargin(0.0f, static_cast<float>(SettingsPageSeparation), 0.0f, 0.0f));
		return B;
	}

	UComboBoxString* PageOption(UWidgetTree* T, UVerticalBox* Page, const TCHAR* Name)
	{
		UComboBoxString* C = T->ConstructWidget<UPawOption>(UPawOption::StaticClass(), Name);
		USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(FString(Name) + TEXT("Box"))));
		Box->SetMinDesiredHeight(static_cast<float>(SettingsRowHeight));
		Box->SetContent(C);
		Page->AddChildToVerticalBox(Box)->SetPadding(FMargin(0.0f, static_cast<float>(SettingsPageSeparation), 0.0f, 0.0f));
		return C;
	}

	UVerticalBox* MakePage(UWidgetTree* T, UOverlay* Pages, const TCHAR* Name)
	{
		UVerticalBox* P = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
		UOverlaySlot* S = Pages->AddChildToOverlay(P);
		S->SetHorizontalAlignment(HAlign_Fill);
		S->SetVerticalAlignment(VAlign_Top);
		return P;
	}

	// Fills a list without it counting as the player's pick, and selects Index.
	void Fill(UComboBoxString* C, const TArray<FString>& Items, int32 Index)
	{
		if (!C)
		{
			return;
		}
		C->ClearOptions();
		for (const FString& I : Items)
		{
			C->AddOption(I);
		}
		C->SetSelectedIndex(Items.IsValidIndex(Index) ? Index : -1);
	}
}

UPawOption::UPawOption(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	using namespace PawUi;
	FSlateFontInfo LabelFont = GetFont();
	LabelFont.Size = FontPt(SettingsCheckPx);
	InitFont(LabelFont);
	InitForegroundColor(FSlateColor(PawTokens::UI_TEXT));
	// The theme's OptionButton: the theme button's plates (normal, hover, pressed, disabled; phase 6 slice 5a: the chrome) and
	// the theme's popup panel (PopupMenu: SBT_panel, its hover SBT_button_hover). The selection ring is the dialog's to paint
	// (UPawSettingsDialog::NativePaint): a combo box draws one brush per state.
	FComboBoxStyle Style = GetWidgetStyle();
	Style.ComboButtonStyle.ButtonStyle.Normal = Chrome(EPawChrome::ButtonNormal);
	Style.ComboButtonStyle.ButtonStyle.Hovered = Chrome(EPawChrome::ButtonHover);
	Style.ComboButtonStyle.ButtonStyle.Pressed = Chrome(EPawChrome::ButtonPressed);
	Style.ComboButtonStyle.ButtonStyle.Disabled = Chrome(EPawChrome::ButtonDisabled);
	Style.ComboButtonStyle.ButtonStyle.NormalForeground = FSlateColor(PawTokens::UI_TEXT);
	Style.ComboButtonStyle.ButtonStyle.HoveredForeground = FSlateColor(PawTokens::UI_TEXT);
	Style.ComboButtonStyle.ButtonStyle.PressedForeground = FSlateColor(PawTokens::UI_TEXT_MUTED);
	Style.ComboButtonStyle.ButtonStyle.DisabledForeground = FSlateColor(PawTokens::UI_TEXT_DISABLED);
	Style.ComboButtonStyle.MenuBorderBrush = Chrome(EPawChrome::Panel);
	Style.ComboButtonStyle.MenuBorderPadding = FMargin(14.0f, 12.0f); // SBT_panel content margins
	SetWidgetStyle(Style);
	SetContentPadding(FMargin(16.0f, 8.0f));
	FTableRowStyle Row = GetItemStyle();
	const FSlateBrush Plain = NoBrush();
	const FSlateBrush Lit = Chrome(EPawChrome::ButtonHover); // PopupMenu/styles/hover
	const FSlateBrush Picked = Rounded(PawTokens::UI_BUTTON_PRESSED, PawTokens::UI_FOCUS, 0.0f, 4.0f);
	Row.SetEvenRowBackgroundBrush(Plain).SetOddRowBackgroundBrush(Plain).SetEvenRowBackgroundHoveredBrush(Lit)
		.SetOddRowBackgroundHoveredBrush(Lit).SetActiveBrush(Picked).SetActiveHoveredBrush(Picked).SetInactiveBrush(Picked)
		.SetInactiveHoveredBrush(Picked).SetActiveHighlightedBrush(Lit).SetInactiveHighlightedBrush(Lit)
		.SetTextColor(FSlateColor(PawTokens::UI_TEXT)).SetSelectedTextColor(FSlateColor(PawTokens::UI_TEXT));
	SetItemStyle(Row);
	// Up / down leave the list for the next row (MenuFocus); A opens it, as Godot's OptionButton.
	SetEnableGamepadNavigationMode(true);
}

int32 UPawSettingsDialog::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	// The selection ring over a list (the theme's OptionButton focus: SBF_focus), as UPawButton paints its own.
	static const FSlateBrush Ring = PawUi::FocusRing();
	const FVector2f E(PawUi::FocusRingExpand, PawUi::FocusRingExpand);
	for (const UComboBoxString* List : { OutputOption.Get(), LanguageOption.Get(), ResolutionOption.Get(), QualityOption.Get(),
		MaxFpsOption.Get(), PromptsOption.Get(), DockSideOption.Get() })
	{
		if (OptionShowsRing(List))
		{
			const FGeometry& G = List->GetCachedGeometry();
			FSlateDrawElement::MakeBox(OutDrawElements, Layer + 1, G.ToPaintGeometry(G.GetLocalSize() + 2.0f * E, FSlateLayoutTransform(-E)), &Ring,
				ESlateDrawEffect::None, Ring.GetTint(InWidgetStyle));
		}
	}
	return Layer + 1;
}

bool UPawSettingsDialog::OptionShowsRing(const UComboBoxString* List) const
{
	if (!List)
	{
		return false;
	}
	const UGameInstance* GI = GetGameInstance();
	const UPawInputDeviceSubsystem* Device = GI ? GI->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
	return UPawButton::RingRule(List->IsHovered(), List->HasAnyUserFocus() || List->HasFocusedDescendants(), Device && Device->LastWasPointer());
}

bool UPawSettingsDialog::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		UOverlay* Root = T->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The scrim (Color(SKY base, 0.8)): it eats every click.
		Scrim = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Scrim"));
		Scrim->SetBrush(Rounded(PawTokens::SKY_BASE.CopyWithNewOpacity(static_cast<float>(PauseScrimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		Scrim->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* ScrimSlot = Root->AddChildToOverlay(Scrim);
		ScrimSlot->SetHorizontalAlignment(HAlign_Fill);
		ScrimSlot->SetVerticalAlignment(VAlign_Fill);

		// The window: 1000 x 980 at the centre (StyleBoxFlat_window: paper, a 3 px ink frame, radius 10, margins 24 / 20).
		USizeBox* Size = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WindowSize"));
		Size->SetWidthOverride(static_cast<float>(SettingsWindowWidth));
		Size->SetHeightOverride(static_cast<float>(SettingsWindowHeight));
		UOverlaySlot* SizeSlot = Root->AddChildToOverlay(Size);
		SizeSlot->SetHorizontalAlignment(HAlign_Center);
		SizeSlot->SetVerticalAlignment(VAlign_Center);
		Window = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Window"));
		Window->SetBrush(Chrome(EPawChrome::Window)); // R3: the plum plate
		Window->SetPadding(PlatePadding(FMargin(static_cast<float>(PausePadX), static_cast<float>(PausePadY)), EPawChrome::Window));
		Window->SetVisibility(ESlateVisibility::Visible);
		Size->SetContent(Window);
		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Body"));
		Window->SetContent(Body);
		const float Gap = static_cast<float>(PauseBodySeparation);

		Title = Heavy(MakeText(T, LabelPx, PawTokens::UI_TEXT, false, TEXT("Title")));
		Title->SetJustification(ETextJustify::Center);
		Title->SetText(PawText::Ui(TEXT("SETTINGS_title")));
		Body->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);

		// The section bar: one toggle per section, sharing the width.
		UHorizontalBox* Bar = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SectionBar"));
		Body->AddChildToVerticalBox(Bar)->SetPadding(FMargin(0.0f, Gap, 0.0f, 0.0f));
		const TCHAR* SectionNames[] = { TEXT("SectionAudio"), TEXT("SectionVideo"), TEXT("SectionControls") };
		UPawButton* Sections[3] = {};
		for (int32 I = 0; I < 3; ++I)
		{
			UPawButton* B = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), SectionNames[I]);
			B->SetLabel(PawText::Ui(SectionKeys[I]));
			B->SetMinDimensions(0, static_cast<int32>(SettingsRowHeight));
			UHorizontalBoxSlot* S = Bar->AddChildToHorizontalBox(B);
			S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			S->SetPadding(FMargin(I == 0 ? 0.0f : static_cast<float>(SettingsSectionSeparation), 0.0f, 0.0f, 0.0f));
			Sections[I] = B;
		}
		SectionAudio = Sections[0];
		SectionVideo = Sections[1];
		SectionControls = Sections[2];

		// The HSeparator: the theme's separator (phase 6 slice 5a). Godot draws it 4 px tall, the texture's 2 + 2 px margins
		// and nothing between, so the line in the middle of ui_separator.svg never shows (finding F41).
		UBorder* Rule = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SectionSeparator"));
		Rule->SetBrush(Chrome(EPawChrome::Separator));
		USizeBox* RuleBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SectionSeparatorBox"));
		RuleBox->SetHeightOverride(static_cast<float>(SettingsRuleHeight));
		RuleBox->SetContent(Rule);
		Body->AddChildToVerticalBox(RuleBox)->SetPadding(FMargin(0.0f, Gap, 0.0f, 0.0f));

		// The pages share one place; one shows at a time.
		UOverlay* Pages = T->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Pages"));
		UVerticalBoxSlot* PagesSlot = Body->AddChildToVerticalBox(Pages);
		PagesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		PagesSlot->SetPadding(FMargin(0.0f, Gap, 0.0f, 0.0f));

		AudioPage = MakePage(T, Pages, TEXT("AudioPage"));
		VolumeLabel = PageLabel(T, AudioPage, TEXT("VolumeLabel"), nullptr, true);
		VolumeSlider = PageSlider(T, AudioPage, TEXT("VolumeSlider"), Pc::PcSettingsSlider::Master);
		MusicLabel = PageLabel(T, AudioPage, TEXT("MusicLabel"), nullptr, false);
		MusicSlider = PageSlider(T, AudioPage, TEXT("MusicSlider"), Pc::PcSettingsSlider::Music);
		SfxLabel = PageLabel(T, AudioPage, TEXT("SfxLabel"), nullptr, false);
		SfxSlider = PageSlider(T, AudioPage, TEXT("SfxSlider"), Pc::PcSettingsSlider::Sfx);
		MuteCheck = PageCheck(T, AudioPage, TEXT("MuteCheck"), TEXT("SETTINGS_mute"));
		OutputLabel = PageLabel(T, AudioPage, TEXT("OutputLabel"), TEXT("SETTINGS_output"), false);
		OutputOption = PageOption(T, AudioPage, TEXT("OutputOption"));

		VideoPage = MakePage(T, Pages, TEXT("VideoPage"));
		LanguageLabel = PageLabel(T, VideoPage, TEXT("LanguageLabel"), TEXT("SETTINGS_language"), true);
		LanguageOption = PageOption(T, VideoPage, TEXT("LanguageOption"));
		ResolutionLabel = PageLabel(T, VideoPage, TEXT("ResolutionLabel"), TEXT("SETTINGS_resolution"), false);
		ResolutionOption = PageOption(T, VideoPage, TEXT("ResolutionOption"));
		FullscreenCheck = PageCheck(T, VideoPage, TEXT("FullscreenCheck"), TEXT("SETTINGS_fullscreen"));
		VsyncCheck = PageCheck(T, VideoPage, TEXT("VsyncCheck"), TEXT("SETTINGS_vsync"));
		QualityLabel = PageLabel(T, VideoPage, TEXT("QualityLabel"), TEXT("SETTINGS_quality"), false);
		QualityOption = PageOption(T, VideoPage, TEXT("QualityOption"));
		MaxFpsLabel = PageLabel(T, VideoPage, TEXT("MaxFpsLabel"), TEXT("SETTINGS_max_fps"), false);
		MaxFpsOption = PageOption(T, VideoPage, TEXT("MaxFpsOption"));
		EffectsCheck = PageCheck(T, VideoPage, TEXT("EffectsCheck"), TEXT("SETTINGS_effects"));
		VideoNote = PageNote(T, VideoPage, TEXT("VideoNote"), TEXT("SETTINGS_video_note"));

		ControlsPage = MakePage(T, Pages, TEXT("ControlsPage"));
		PromptsLabel = PageLabel(T, ControlsPage, TEXT("PromptsLabel"), TEXT("SETTINGS_prompts"), true);
		PromptsOption = PageOption(T, ControlsPage, TEXT("PromptsOption"));
		PromptsNote = PageNote(T, ControlsPage, TEXT("PromptsNote"), TEXT("SETTINGS_prompts_note"));
		DockSideLabel = PageLabel(T, ControlsPage, TEXT("DockSideLabel"), TEXT("SETTINGS_dock"), false);
		DockSideOption = PageOption(T, ControlsPage, TEXT("DockSideOption"));
		DockSideNote = PageNote(T, ControlsPage, TEXT("DockSideNote"), TEXT("SETTINGS_dock_note"));
		// controls_panel.gd, below the two lists.
		ControlsPanel = T->ConstructWidget<UPawControlsPanel>(PawUi::Resolve<UPawControlsPanel>(), TEXT("ControlsPanel"));
		ControlsPage->AddChildToVerticalBox(ControlsPanel)->SetPadding(FMargin(0.0f, static_cast<float>(SettingsPageSeparation), 0.0f, 0.0f));

		CloseButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("CloseButton"));
		CloseButton->SetLabel(PawText::Ui(TEXT("SETTINGS_close")));
		CloseButton->SetMinDimensions(0, static_cast<int32>(SettingsRowHeight));
		UVerticalBoxSlot* CloseSlot = Body->AddChildToVerticalBox(CloseButton);
		CloseSlot->SetHorizontalAlignment(HAlign_Fill);
		CloseSlot->SetPadding(FMargin(0.0f, Gap, 0.0f, 0.0f));
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		OnLayoutReady();
	}
	return bOk;
}

void UPawSettingsDialog::OnLayoutReady()
{
	auto Click = [this](UPawButton* B, TFunction<void()> Do)
	{
		if (B)
		{
			B->OnClicked().AddWeakLambda(this, MoveTemp(Do));
		}
	};
	Click(SectionAudio, [this]() { PressSection(Pc::PcSettingsSection::Audio); });
	Click(SectionVideo, [this]() { PressSection(Pc::PcSettingsSection::Video); });
	Click(SectionControls, [this]() { PressSection(Pc::PcSettingsSection::Controls); });
	Click(CloseButton, [this]() { PressClose(); });
	Click(MuteCheck, [this]() { ToggleMute(); });
	Click(FullscreenCheck, [this]() { ToggleFullscreen(); });
	Click(VsyncCheck, [this]() { ToggleVsync(); });
	Click(EffectsCheck, [this]() { ToggleEffects(); });
	if (VolumeSlider) { VolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnMasterSlider); }
	if (MusicSlider) { MusicSlider->OnValueChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnMusicSlider); }
	if (SfxSlider) { SfxSlider->OnValueChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnSfxSlider); }
	if (OutputOption) { OutputOption->OnSelectionChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnOutputPicked); }
	if (ResolutionOption) { ResolutionOption->OnSelectionChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnResolutionPicked); }
	if (QualityOption) { QualityOption->OnSelectionChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnQualityPicked); }
	if (MaxFpsOption) { MaxFpsOption->OnSelectionChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnFpsPicked); }
	if (PromptsOption) { PromptsOption->OnSelectionChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnPromptsPicked); }
	if (DockSideOption) { DockSideOption->OnSelectionChanged.AddUniqueDynamic(this, &UPawSettingsDialog::OnDockPicked); }
	// The language list arrives with the languages (phase 7): "System", a door that cannot open yet.
	if (LanguageOption)
	{
		bSyncing = true;
		Fill(LanguageOption, { Str(PawText::Ui(TEXT("SETTINGS_language_system"))) }, 0);
		bSyncing = false;
		LanguageOption->SetIsEnabled(false);
	}
	Dialog.Section = Pc::PcSettingsSection::Audio; // _ready(): show_section("Audio")
	FillOptions();
	ShowSection();
}

UPawSettingsSubsystem* UPawSettingsDialog::Settings() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
}

UPawSettingsDialog* UPawSettingsDialog::Open(UCommonActivatableWidgetStack* Modal, TFunction<void()> InOnClosed)
{
	if (!Modal)
	{
		return nullptr;
	}
	return Modal->AddWidget<UPawSettingsDialog>(PawUi::Resolve<UPawSettingsDialog>(), [&InOnClosed](UPawSettingsDialog& D)
	{
		D.OnClosed = MoveTemp(InOnClosed);
	});
}

TOptional<FUIInputConfig> UPawSettingsDialog::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

bool UPawSettingsDialog::CanTakeFocus() const
{
	return IsActivated() && GetOwningLocalPlayer() != nullptr;
}

void UPawSettingsDialog::NativeOnActivated()
{
	Super::NativeOnActivated();
	// open(): the controls from the settings (without counting as changes), the tree paused, the page's first control.
	if (const UWorld* W = GetWorld())
	{
		if (UPawRealmSubsystem* R = W->GetSubsystem<UPawRealmSubsystem>())
		{
			R->SetPauseHold(PauseHolder, true);
		}
	}
	Sync();
	ShowSection();
}

void UPawSettingsDialog::NativeOnDeactivated()
{
	if (const UWorld* W = GetWorld())
	{
		if (UPawRealmSubsystem* R = W->GetSubsystem<UPawRealmSubsystem>())
		{
			R->SetPauseHold(PauseHolder, false);
		}
	}
	Super::NativeOnDeactivated();
	// closed: however it was dismissed, once.
	TFunction<void()> Then = MoveTemp(OnClosed);
	OnClosed = nullptr;
	if (Then)
	{
		Then();
	}
}

// --- the controls from the settings ----------------------------------------------------------------------------------------

void UPawSettingsDialog::FillOptions()
{
	// The lists' words (rebuilt on a language change in Godot; here on every open, which is the same while there is one).
	const UPawSettingsSubsystem* S = Settings();
	const Pc::PcSettings P = S ? S->Get() : Pc::PcSettings();
	bSyncing = true;
	TArray<FString> Items;
	for (int64 I = 0; I < Pc::SettingsResolutionCount; ++I)
	{
		const Pc::PcIntSize R = Pc::SettingsResolution(I);
		FFormatNamedArguments Args;
		Args.Add(TEXT("w"), PawText::Count(R.X));
		Args.Add(TEXT("h"), PawText::Count(R.Y));
		Items.Add(Str(PawText::Ui(TEXT("SETTINGS_resolution_size"), Args)));
	}
	Fill(ResolutionOption, Items, static_cast<int32>(P.ResolutionIndex));
	Fill(QualityOption, { Str(PawText::Ui(TEXT("SETTINGS_quality_low"))), Str(PawText::Ui(TEXT("SETTINGS_quality_medium"))),
		Str(PawText::Ui(TEXT("SETTINGS_quality_high"))) }, static_cast<int32>(P.GraphicsQuality));
	Items.Reset();
	for (int64 I = 0; I < Pc::SettingsFpsOptionCount; ++I)
	{
		const int64 Fps = Pc::SettingsFpsOption(I);
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), PawText::Count(Fps));
		Items.Add(Str(Fps == 0 ? PawText::Ui(TEXT("SETTINGS_fps_unlimited")) : PawText::Ui(TEXT("SETTINGS_fps"), Args)));
	}
	Fill(MaxFpsOption, Items, static_cast<int32>(Pc::SettingsFpsSelected(P)));
	Fill(PromptsOption, { Str(PawText::Ui(TEXT("SETTINGS_prompts_auto"))), Str(PawText::Ui(TEXT("SETTINGS_prompts_keyboard"))),
		Str(PawText::Ui(TEXT("SETTINGS_prompts_xbox"))), Str(PawText::Ui(TEXT("SETTINGS_prompts_playstation"))),
		Str(PawText::Ui(TEXT("SETTINGS_prompts_steam_deck"))) }, static_cast<int32>(Pc::SettingsPromptsSelected(P)));
	Fill(DockSideOption, { Str(PawText::Ui(TEXT("SETTINGS_dock_left"))), Str(PawText::Ui(TEXT("SETTINGS_dock_right"))) },
		static_cast<int32>(Pc::SettingsDockSelected(P)));
	// _fill_output_option(): refilled on every open (devices come and go): the system's devices, "Default" first.
	std::vector<std::string> System = { "Default" };
	if (const UPawAudioSubsystem* Audio = UPawAudioSubsystem::Of(this))
	{
		System.clear();
		for (const FString& Name : Audio->OutputDeviceList())
		{
			System.push_back(TCHAR_TO_UTF8(*Name));
		}
	}
	const Pc::PcSettingsOutputList Out = Pc::SettingsOutputList(System, P);
	OutputDevices.Reset();
	Items.Reset();
	for (const std::string& D : Out.Devices)
	{
		const FString Name = UTF8_TO_TCHAR(D.c_str());
		OutputDevices.Add(Name);
		Items.Add(D == "Default" ? Str(PawText::Ui(TEXT("SETTINGS_output_system"))) : Name);
	}
	Fill(OutputOption, Items, static_cast<int32>(Out.Selected));
	bSyncing = false;
}

void UPawSettingsDialog::RefreshVolumeLines()
{
	UTextBlock* Lines[3] = { VolumeLabel.Get(), MusicLabel.Get(), SfxLabel.Get() };
	const TCHAR* Keys[3] = { TEXT("SETTINGS_master"), TEXT("SETTINGS_music"), TEXT("SETTINGS_sfx") };
	USlider* Sliders[3] = { VolumeSlider.Get(), MusicSlider.Get(), SfxSlider.Get() };
	bSyncing = true;
	for (int32 I = 0; I < 3; ++I)
	{
		if (Lines[I])
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("n"), PawText::Count(Dialog.Percent[I]));
			Lines[I]->SetText(PawText::Ui(Keys[I], Args));
		}
		if (Sliders[I])
		{
			Sliders[I]->SetValue(static_cast<float>(Dialog.Slider[I]));
		}
	}
	bSyncing = false;
}

void UPawSettingsDialog::Sync()
{
	// _sync_from_settings(): every control from the stored value, firing nothing.
	const UPawSettingsSubsystem* S = Settings();
	const Pc::PcSettings P = S ? S->Get() : Pc::PcSettings();
	Pc::SettingsDialogSync(Dialog, P);
	RefreshVolumeLines();
	FillOptions();
	if (MuteCheck) { MuteCheck->SetCheck(true, P.bMuted); }
	if (FullscreenCheck) { FullscreenCheck->SetCheck(true, P.bFullscreen); }
	if (VsyncCheck) { VsyncCheck->SetCheck(true, P.bVsync); }
	if (EffectsCheck) { EffectsCheck->SetCheck(true, P.bEffectsEnabled); }
	// open(): controls_panel.sync().
	if (ControlsPanel) { ControlsPanel->Sync(); }
	// Resolution is meaningless while fullscreen owns the window size.
	if (ResolutionOption)
	{
		ResolutionOption->SetIsEnabled(!P.bFullscreen);
	}
}

// --- sections and the selection ------------------------------------------------------------------------------------------

UVerticalBox* UPawSettingsDialog::PageOf(Pc::PcSettingsSection Section) const
{
	switch (Section)
	{
	case Pc::PcSettingsSection::Audio: return AudioPage;
	case Pc::PcSettingsSection::Video: return VideoPage;
	default: return ControlsPage;
	}
}

void UPawSettingsDialog::ShowSection()
{
	// show_section(): the page shown, its toggle pressed (the theme's pressed plate).
	UPawButton* Toggles[3] = { SectionAudio.Get(), SectionVideo.Get(), SectionControls.Get() };
	for (int32 I = 0; I < 3; ++I)
	{
		const Pc::PcSettingsSection S = static_cast<Pc::PcSettingsSection>(I);
		if (UVerticalBox* Page = PageOf(S))
		{
			Page->SetVisibility(S == Dialog.Section ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (Toggles[I])
		{
			Toggles[I]->SetCallLook(S == Dialog.Section); // R4a: the open section on the Butter plate
		}
	}
	FocusPage();
}

void UPawSettingsDialog::FocusPage()
{
	// _focus_page(): the section bar on top, then every live control of the page, one per row, then Close; rewired on every
	// section change (a hidden page's controls must not be in the loop). Opens on the page's first control.
	TArray<TArray<UWidget*>> Rows;
	TArray<UWidget*> Bar;
	for (UPawButton* B : { SectionAudio.Get(), SectionVideo.Get(), SectionControls.Get() })
	{
		if (B)
		{
			Bar.Add(B);
		}
	}
	Rows.Add(Bar);
	UWidget* First = nullptr;
	TArray<UWidget*> Controls;
	switch (Dialog.Section)
	{
	case Pc::PcSettingsSection::Audio:
		Controls = { VolumeSlider.Get(), MusicSlider.Get(), SfxSlider.Get(), MuteCheck.Get(), OutputOption.Get() };
		break;
	case Pc::PcSettingsSection::Video:
		Controls = { LanguageOption.Get(), ResolutionOption.Get(), FullscreenCheck.Get(), VsyncCheck.Get(), QualityOption.Get(),
			MaxFpsOption.Get(), EffectsCheck.Get() };
		break;
	default:
		Controls = { PromptsOption.Get(), DockSideOption.Get() };
		break;
	}
	for (UWidget* C : Controls)
	{
		// A door that cannot open (a disabled list) is a dead stop for a pad: MenuFocus.interactive() leaves it out.
		if (C && C->GetIsEnabled())
		{
			Rows.Add({ C });
			First = First ? First : C;
		}
	}
	// The binding table is a table: keyboard, controller and Reset side by side, so left and right move along a row.
	if (Dialog.Section == Pc::PcSettingsSection::Controls && ControlsPanel)
	{
		Rows.Append(ControlsPanel->FocusRows());
	}
	Rows.Add({ CloseButton.Get() });
	Wired = WireRows(Rows);
	if (CanTakeFocus())
	{
		if (UWidget* Target = First ? First : CloseButton.Get())
		{
			Target->SetFocus();
		}
	}
}

UWidget* UPawSettingsDialog::NativeGetDesiredFocusTarget() const
{
	return Wired.Num() > 3 ? Wired[3] : CloseButton.Get();
}

void UPawSettingsDialog::PressSection(Pc::PcSettingsSection Section)
{
	Dialog.Section = Section;
	ShowSection();
}

void UPawSettingsDialog::StepSection(int64 Direction)
{
	Pc::SettingsDialogStepSection(Dialog, Direction);
	ShowSection();
}

void UPawSettingsDialog::PressClose()
{
	if (IsActivated())
	{
		DeactivateWidget();
	}
}

FReply UPawSettingsDialog::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// settings_dialog.gd _process, through the input map: back closes; section_prev / section_next turn the page.
	// A key being BOUND is not a command: the Controls panel takes it first (Esc cancels the listening, not the dialog).
	if (PanelListening())
	{
		return ControlsPanel->CaptureKey(KeyEvent, true) ? FReply::Handled() : FReply::Unhandled();
	}
	const FKey K = KeyEvent.GetKey();
	if (!KeyEvent.IsRepeat())
	{
		if (PawInput::KeyIs(K, TEXT("back")))
		{
			PressClose();
			return FReply::Handled();
		}
		if (PawInput::KeyIs(K, TEXT("section_prev")))
		{
			StepSection(-1);
			return FReply::Handled();
		}
		if (PawInput::KeyIs(K, TEXT("section_next")))
		{
			StepSection(1);
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

bool UPawSettingsDialog::PanelListening() const
{
	return ControlsPanel && ControlsPanel->IsListening() && Dialog.Section == Pc::PcSettingsSection::Controls;
}

FReply UPawSettingsDialog::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (PanelListening())
	{
		return ControlsPanel->CaptureKey(KeyEvent, false) ? FReply::Handled() : FReply::Unhandled();
	}
	return Super::NativeOnKeyUp(Geometry, KeyEvent);
}

FReply UPawSettingsDialog::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	// A click while a cell listens is the panel's (a mouse button bound, or a controller cell's listening cancelled).
	if (PanelListening() && ControlsPanel->CaptureMouse(MouseEvent.GetEffectingButton(), true))
	{
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(Geometry, MouseEvent);
}

FReply UPawSettingsDialog::NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	// Godot's wheel is a mouse BUTTON (4 up, 5 down): it cancels a controller cell's listening, and is never a binding.
	if (PanelListening() && MouseEvent.GetWheelDelta() != 0.0f
		&& ControlsPanel->CaptureMouse(MouseEvent.GetWheelDelta() > 0.0f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown, true))
	{
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(Geometry, MouseEvent);
}

FReply UPawSettingsDialog::NativeOnAnalogValueChanged(const FGeometry& Geometry, const FAnalogInputEvent& Event)
{
	// A trigger being bound: its analog value decides (past half way).
	if (PanelListening() && ControlsPanel->CaptureAnalog(Event))
	{
		return FReply::Handled();
	}
	return Super::NativeOnAnalogValueChanged(Geometry, Event);
}

// --- the changes -----------------------------------------------------------------------------------------------------------

void UPawSettingsDialog::MoveSlider(Pc::PcSettingsSlider Which, double Value)
{
	UPawSettingsSubsystem* S = Settings();
	Pc::PcSettings P = S ? S->Get() : Pc::PcSettings();
	const bool bMoved = Pc::SettingsDialogMoveSlider(Dialog, P, Which, Value);
	if (bMoved && S)
	{
		switch (Which)
		{
		case Pc::PcSettingsSlider::Master: S->SetMasterVolume(P.MasterVolume); break;
		case Pc::PcSettingsSlider::Music: S->SetMusicVolume(P.MusicVolume); break;
		default: S->SetSfxVolume(P.SfxVolume); break;
		}
	}
	// The slider sits on its step either way (a drag between steps lands on one).
	RefreshVolumeLines();
}

void UPawSettingsDialog::OnMasterSlider(float Value) { if (!bSyncing) { MoveSlider(Pc::PcSettingsSlider::Master, Value); } }
void UPawSettingsDialog::OnMusicSlider(float Value) { if (!bSyncing) { MoveSlider(Pc::PcSettingsSlider::Music, Value); } }
void UPawSettingsDialog::OnSfxSlider(float Value) { if (!bSyncing) { MoveSlider(Pc::PcSettingsSlider::Sfx, Value); } }

void UPawSettingsDialog::ToggleMute()
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetMuted(!S->Get().bMuted);
		if (MuteCheck) { MuteCheck->SetCheck(true, S->Get().bMuted); }
	}
}

void UPawSettingsDialog::ToggleFullscreen()
{
	// Resolution is NOT greyed here: Godot only does that as the dialog opens.
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetFullscreen(!S->Get().bFullscreen);
		if (FullscreenCheck) { FullscreenCheck->SetCheck(true, S->Get().bFullscreen); }
	}
}

void UPawSettingsDialog::ToggleVsync()
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetVsync(!S->Get().bVsync);
		if (VsyncCheck) { VsyncCheck->SetCheck(true, S->Get().bVsync); }
	}
}

void UPawSettingsDialog::ToggleEffects()
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetEffectsEnabled(!S->Get().bEffectsEnabled);
		if (EffectsCheck) { EffectsCheck->SetCheck(true, S->Get().bEffectsEnabled); }
	}
}

void UPawSettingsDialog::ShowPicked(UComboBoxString* List, int64 Index)
{
	// The list shows the item picked (whoever picked it: a click, a key, a test), never counted as a second pick.
	if (List && List->GetSelectedIndex() != static_cast<int32>(Index))
	{
		bSyncing = true;
		List->SetSelectedIndex(static_cast<int32>(Index));
		bSyncing = false;
	}
}

void UPawSettingsDialog::PickResolution(int32 Index)
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetResolutionIndex(Index);
		ShowPicked(ResolutionOption, S->Get().ResolutionIndex);
	}
}

void UPawSettingsDialog::PickQuality(int32 Index)
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetGraphicsQuality(Index);
		ShowPicked(QualityOption, S->Get().GraphicsQuality);
	}
}

void UPawSettingsDialog::PickFps(int32 Index)
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetMaxFps(Pc::SettingsFpsOption(Index));
		ShowPicked(MaxFpsOption, Pc::SettingsFpsSelected(S->Get()));
	}
}

void UPawSettingsDialog::PickPrompts(int32 Index)
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetPromptStyle(UTF8_TO_TCHAR(Pc::SettingsPromptStyle(Index)));
		ShowPicked(PromptsOption, Pc::SettingsPromptsSelected(S->Get()));
	}
}

void UPawSettingsDialog::PickDock(int32 Index)
{
	if (UPawSettingsSubsystem* S = Settings())
	{
		S->SetDockSide(UTF8_TO_TCHAR(Pc::SettingsDockSide(Index)));
		ShowPicked(DockSideOption, Pc::SettingsDockSelected(S->Get()));
	}
}

void UPawSettingsDialog::PickOutput(int32 Index)
{
	if (UPawSettingsSubsystem* S = Settings(); S && OutputDevices.IsValidIndex(Index))
	{
		S->SetOutputDevice(OutputDevices[Index]);
		ShowPicked(OutputOption, Index);
	}
}

void UPawSettingsDialog::OnOutputPicked(FString, ESelectInfo::Type How)
{
	if (!bSyncing && ByPlayer(How) && OutputOption) { PickOutput(OutputOption->GetSelectedIndex()); }
}
void UPawSettingsDialog::OnResolutionPicked(FString, ESelectInfo::Type How)
{
	if (!bSyncing && ByPlayer(How) && ResolutionOption) { PickResolution(ResolutionOption->GetSelectedIndex()); }
}
void UPawSettingsDialog::OnQualityPicked(FString, ESelectInfo::Type How)
{
	if (!bSyncing && ByPlayer(How) && QualityOption) { PickQuality(QualityOption->GetSelectedIndex()); }
}
void UPawSettingsDialog::OnFpsPicked(FString, ESelectInfo::Type How)
{
	if (!bSyncing && ByPlayer(How) && MaxFpsOption) { PickFps(MaxFpsOption->GetSelectedIndex()); }
}
void UPawSettingsDialog::OnPromptsPicked(FString, ESelectInfo::Type How)
{
	if (!bSyncing && ByPlayer(How) && PromptsOption) { PickPrompts(PromptsOption->GetSelectedIndex()); }
}
void UPawSettingsDialog::OnDockPicked(FString, ESelectInfo::Type How)
{
	if (!bSyncing && ByPlayer(How) && DockSideOption) { PickDock(DockSideOption->GetSelectedIndex()); }
}

// --- test views ------------------------------------------------------------------------------------------------------------

FText UPawSettingsDialog::GetVolumeLine(Pc::PcSettingsSlider Which) const
{
	const UTextBlock* L = Which == Pc::PcSettingsSlider::Master ? VolumeLabel.Get() : Which == Pc::PcSettingsSlider::Music ? MusicLabel.Get() : SfxLabel.Get();
	return L ? L->GetText() : FText::GetEmpty();
}

double UPawSettingsDialog::GetSliderValue(Pc::PcSettingsSlider Which) const
{
	const USlider* S = Which == Pc::PcSettingsSlider::Master ? VolumeSlider.Get() : Which == Pc::PcSettingsSlider::Music ? MusicSlider.Get() : SfxSlider.Get();
	return S ? S->GetValue() : -1.0;
}

bool UPawSettingsDialog::IsPageShown(Pc::PcSettingsSection Section) const
{
	return Shown(PageOf(Section));
}

UWidget* UPawSettingsDialog::GetPart(const TCHAR* Name) const
{
	return WidgetTree ? WidgetTree->FindWidget(FName(Name)) : nullptr;
}

UPawButton* UPawSettingsDialog::GetCheck(const TCHAR* Name) const { return Cast<UPawButton>(GetPart(Name)); }
UComboBoxString* UPawSettingsDialog::GetOption(const TCHAR* Name) const { return Cast<UComboBoxString>(GetPart(Name)); }
FText UPawSettingsDialog::GetTitleText() const { return Title ? Title->GetText() : FText::GetEmpty(); }
