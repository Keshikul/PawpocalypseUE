#include "UI/PawHudScreen.h"

#include "Audio/PawAudioSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/World.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcAchievements.h"
#include "PcCards.h"
#include "PcField.h"
#include "PcFieldRules.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/PawInputMap.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawIcons.h"
#include "UI/PawSettingsDialog.h"
#include "Save/PawSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Image.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawJobPicker.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawSkillTreeScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/PawPounceClock.h"
#include "UI/PawPrompts.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawRainbow.h"
#include "UI/PawVictoryScreen.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

namespace
{
	using Pc::PcDockTab;
	constexpr int32 TabCount = static_cast<int32>(PcDockTab::Count);

	// Survive the HUD being rebuilt, as Godot's statics survive travel: the buy multiplier, the open tab, the jobs the
	// player has seen (-1 until the session's first HUD seeds it). The dock's side and "ask before Reawakening" are the
	// settings' (UPawSettingsSubsystem: Settings > Controls > Menu side; the Reawaken page's box).
	int64 GBuyMultiplier = 1;
	PcDockTab GActiveTab = PcDockTab::Upgrades;
	int64 GJobsSeen = -1;
	// The automations (auto-travel, auto-pick) are the save slot's: UPawGameSubsystem::Slot().

	UPawSettingsSubsystem* SettingsOf(const UUserWidget* W)
	{
		const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	}

	bool DockOnLeft(const UUserWidget* W)
	{
		const UPawSettingsSubsystem* S = SettingsOf(W);
		return !S || S->IsDockOnLeft();
	}

	bool ConfirmReawaken(const UUserWidget* W)
	{
		const UPawSettingsSubsystem* S = SettingsOf(W);
		return !S || S->Get().bConfirmReawaken;
	}
	constexpr float PageWrap = 520.0f;   // the dock's inner width: wrapped page text is given a fixed width (KB U/09 §3)
	constexpr float ButtonWrap = 480.0f; // inside a button's padding
	constexpr double RealmRowHeight = 54.0; // hud.gd _build_travel_panel: custom_minimum_size.y
	constexpr double ReawakenButtonHeight = 76.0; // hud.tscn ReawakenButton
	constexpr double ReawakenButtonPx = 26.0;
	constexpr double ReawakenBarHeight = 30.0; // hud.tscn ReawakenBar
	constexpr double CardHeadingPx = 21.0;     // hud.gd _refresh_catches_page: the heading's font_size
	constexpr double ClaimWidth = 112.0, ClaimHeight = 42.0; // hud.gd _build_achievement_list
	constexpr double ButtonIconPx = 26.0;      // currency_icons.gd BUTTON_ICON_PX (icon_max_width)
	constexpr double QuickBarHeight = 66.0;    // hud.tscn QuickBar custom_minimum_size.y
	constexpr double QuickIconPx = 40.0;       // the TextureButtons' custom_minimum_size
	constexpr double QuickSeparation = 14.0;   // QuickBar separation
	constexpr double AchievementIconPx = 36.0; // hud.gd _build_achievement_list
	constexpr double JobIconPx = 40.0;         // hud.gd _build_job_page
	constexpr double JobButtonWidth = 104.0;   // hud.gd _build_job_page (42 high, as Claim)
	constexpr float RowTextWrap = 360.0f;      // a row's text beside its button
	constexpr float MarkSize = 14.0f;          // the achievement mark (★ ◆ ☆: the font has none until phase 6's art)

	// CardManager.REQUIREMENT_TEXT: one line per requirement, phrased as an instruction.
	const TCHAR* CardRequirementKey(const std::string& Requirement)
	{
		if (Requirement == "aura") return TEXT("CATCHES_hint_aura");
		if (Requirement == "pounce") return TEXT("CATCHES_hint_pounce");
		if (Requirement == "overkill") return TEXT("CATCHES_hint_overkill");
		if (Requirement == "poop") return TEXT("CATCHES_hint_poop");
		if (Requirement == "kitten") return TEXT("CATCHES_hint_kitten");
		if (Requirement == "kitten_pounce") return TEXT("CATCHES_hint_kitten_pounce");
		return TEXT("CATCHES_hint_other");
	}

	const TCHAR* TabKey(PcDockTab T)
	{
		static const TCHAR* Keys[] = { TEXT("TAB_upgrades"), TEXT("TAB_reawaken"), TEXT("TAB_realms"), TEXT("TAB_catches"), TEXT("TAB_awards"), TEXT("TAB_jobs") };
		return Keys[static_cast<int32>(T)];
	}
	const TCHAR* PageKey(PcDockTab T)
	{
		static const TCHAR* Keys[] = { TEXT("PAGE_upgrades"), TEXT("PAGE_reawaken"), TEXT("PAGE_realms"), TEXT("PAGE_catches"), TEXT("PAGE_awards"), TEXT("PAGE_jobs") };
		return Keys[static_cast<int32>(T)];
	}
	// hud.gd's tab ids (the icon is ui_tab_<id>), in PcDockTab order.
	FString TabIconId(PcDockTab T)
	{
		static const TCHAR* Ids[] = { TEXT("upgrades"), TEXT("reawaken"), TEXT("realms"), TEXT("catches"), TEXT("awards"), TEXT("jobs") };
		return Ids[FMath::Clamp(static_cast<int32>(T), 0, 5)];
	}
	const TCHAR* MultiplierKey(int64 M)
	{
		switch (M)
		{
		case 5: return TEXT("SHOP_mult_x5");
		case 10: return TEXT("SHOP_mult_x10");
		case -1: return TEXT("SHOP_mult_max");
		default: return TEXT("SHOP_mult_x1");
		}
	}

	bool HasFocus(const UWidget* W)
	{
		return W && (W->HasAnyUserFocus() || W->HasFocusedDescendants());
	}

	void SetShown(UWidget* W, bool bShow, ESlateVisibility Shown = ESlateVisibility::SelfHitTestInvisible)
	{
		if (W && W->GetVisibility() != (bShow ? Shown : ESlateVisibility::Collapsed))
		{
			W->SetVisibility(bShow ? Shown : ESlateVisibility::Collapsed);
		}
	}

	void SetText(UTextBlock* T, const FText& Text)
	{
		if (T && !T->GetText().IdenticalTo(Text) && !T->GetText().EqualTo(Text))
		{
			T->SetText(Text);
		}
	}

	// A currency's icon (hud.tscn CatnipIcon / StardustIcon: 34 x 34, keep aspect; phase 6 slice 5b).
	UImage* CurrencyIcon(UWidgetTree* Tree, const FString& Kind, FName Name)
	{
		UImage* I = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		I->SetBrush(PawUi::IconBrush(PawUi::CurrencyIconName(Kind), PawUi::CurrencyIconSize));
		I->SetVisibility(ESlateVisibility::HitTestInvisible);
		return I;
	}

	void SetText(UPawIconText* T, const FText& Text)
	{
		if (T)
		{
			T->SetText(Text);
		}
	}

	// A label with its currency icon before it (currency_icons.gd wrap_label: LABEL_ICON_PX 24, separation 8).
	UPawIconText* IconLine(UWidgetTree* Tree, const FString& Kind, double Px, const FLinearColor& Colour, float WrapAt, FName Name,
		bool bOverField = false)
	{
		UPawIconText* L = Tree->ConstructWidget<UPawIconText>(PawUi::Resolve<UPawIconText>(), Name);
		L->IconName = PawUi::CurrencyIconName(Kind);
		L->TextPx = static_cast<float>(Px);
		L->TextColour = Colour;
		L->WrapAt = WrapAt;
		L->bOverField = bOverField;
		L->SynchronizeProperties();
		return L;
	}
}

// --- build --------------------------------------------------------------------------------------------------------

bool UPawHudScreen::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // nothing over the field takes a click
		WidgetTree->RootWidget = Root;
		BuildOverlay(Root);
		BuildDock(Root); // the dock draws above the field's readouts
		// Last: the Reawaken ceremony over all of it (Godot's CanvasLayer 10 over the HUD's 1). It takes no input.
		Ceremony = WidgetTree->ConstructWidget<UPawReawakenCeremony>(PawUi::Resolve<UPawReawakenCeremony>(), TEXT("Ceremony"));
		UOverlaySlot* CeremonySlot = Root->AddChildToOverlay(Ceremony);
		CeremonySlot->SetHorizontalAlignment(HAlign_Fill);
		CeremonySlot->SetVerticalAlignment(VAlign_Fill);
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		OnLayoutReady();
	}
	return bOk;
}

void UPawHudScreen::OnLayoutReady()
{
	// The parts, in either mode (a Widget Blueprint bound the single ones; the repeated ones are found by name).
	Remembered.SetNum(TabCount);
	PawUi::CollectNamed(WidgetTree, TEXT("Tab"), TabCount, Tabs);
	PawUi::CollectNamed(WidgetTree, TEXT("PageTitle"), TabCount, PageTitles);
	auto SlotOf = [](UWidget* W) { return W ? Cast<UOverlaySlot>(W->Slot) : nullptr; };
	DockSlot = SlotOf(DockBox);
	StatusSlot = SlotOf(StatusRow);
	XpSlot = SlotOf(XpRow);
	ClockSlot = SlotOf(PounceClock);
	PouncePromptSlot = SlotOf(PouncePrompt);
	BossSlot = SlotOf(WidgetTree->FindWidget(TEXT("BossBox")));
	if (ZoneReady)
	{
		ZoneReady->SetFollow(ZonePlate); // zone_rainbow.follow = zone_plate
	}
	auto Click = [this](UPawButton* B, TFunction<void()> Do)
	{
		if (B)
		{
			B->OnClicked().AddWeakLambda(this, MoveTemp(Do));
		}
	};
	for (int32 I = 0; I < TabCount; ++I)
	{
		if (Tabs[I])
		{
			Tabs[I]->Index = I;
			// hud.gd: icon-only tabs (expand_icon: the content height, 46 - 6 - 6); the name is the tooltip.
			Tabs[I]->SetIcon(PawUi::TabIconName(TabIconId(static_cast<PcDockTab>(I))), PawUi::TabHeight - 12.0);
			Tabs[I]->SetToolTipText(PawText::Ui(TabKey(static_cast<PcDockTab>(I)))); // btn.tooltip_text = spec.label
		}
		Click(Tabs[I], [this, I]() { SelectTab(static_cast<PcDockTab>(I)); });
	}
	// The quick bar (hud.gd _apply_style_icons, _bind_quick_tooltips; the actions at the end of hud.gd).
	struct FQuick { UPawButton* Button; const TCHAR* Icon; const TCHAR* Tip; void (UPawHudScreen::*Press)(); };
	for (const FQuick& Q : { FQuick{ QuickSettings, TEXT("ui_quick_settings"), TEXT("QUICK_settings"), &UPawHudScreen::PressQuickSettings },
		FQuick{ QuickSkills, TEXT("ui_quick_skills"), TEXT("QUICK_skills"), &UPawHudScreen::PressQuickSkills },
		FQuick{ QuickSave, TEXT("ui_quick_save"), TEXT("QUICK_save"), &UPawHudScreen::PressQuickSave },
		FQuick{ QuickQuit, TEXT("ui_quick_quit"), TEXT("QUICK_quit"), &UPawHudScreen::PressQuickQuit } })
	{
		if (Q.Button)
		{
			Q.Button->SetIcon(Q.Icon, QuickIconPx);
			Q.Button->SetToolTipText(PawText::Ui(Q.Tip));
			Q.Button->SetIsFocusable(false); // the mouse's: the dock's pad order never lands here
			Click(Q.Button, [this, Press = Q.Press]() { (this->*Press)(); });
		}
	}
	// hud.gd _apply_currency_icons, on ready: the icon carries the unit, never the word.
	for (UPawButton* B : { ReawakenButton.Get(), ClaimAllButton.Get() })
	{
		if (B)
		{
			B->SetIcon(PawUi::CurrencyIconName(TEXT("stardust")), ButtonIconPx);
		}
	}
	Click(MultiplierButton, [this]() { CycleMultiplier(); });
	Click(BaitButton, [this]() { PressBait(); });
	Click(AutoTravelButton, [this]() { ToggleAutoTravel(); });
	Click(ReawakenButton, [this]() { PressReawaken(); });
	Click(CancelButton, [this]() { CancelReawaken(); });
	Click(AskButton, [this]() { ToggleAskBeforeReawaken(); });
	Click(AutoPickButton, [this]() { ToggleAutoPick(); });
	Click(ClaimAllButton, [this]() { PressClaimAll(); });
	// The switches show the state they switch (the slot's automations, the confirm setting).
	if (AutoTravelButton)
	{
		AutoTravelButton->SetCheck(true, Game() && Game()->Slot().bAutoTravel);
	}
	if (AskButton)
	{
		AskButton->SetCheck(true, ConfirmReawaken(this));
	}
	if (AutoPickButton)
	{
		AutoPickButton->SetCheck(true, Game() && Game()->Slot().bAutoPickCard);
	}
	// The rows the data decides.
	BuildShopRows();
	BuildRealmRows();
	BuildCardRows();
	BuildAchievementRows();
	BuildJobRows();
	// UiJuice.hover (hud.gd): the upgrade rows, the job buttons, the tabs, the travel rows, the claim buttons. Not the quick
	// bar, the page buttons or Bait: Godot leaves those still.
	for (const TArray<TObjectPtr<UPawButton>>* Set : { &Rows, &JobButtons, &Tabs, &RealmRows, &ClaimButtons })
	{
		for (UPawButton* B : *Set)
		{
			Juice.Hover(B, this);
		}
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bLastLeft = DockOnLeft(this);
	ApplyDockSide();
	SelectTab(GActiveTab);
	// dock_side_changed: Settings > Controls > Menu side moves the dock at once; the Reawaken page's box follows too.
	if (UPawSettingsSubsystem* S = SettingsOf(this))
	{
		S->OnChanged.AddWeakLambda(this, [this]()
		{
			ApplyDockSide();
			if (AskButton)
			{
				AskButton->SetCheck(true, ConfirmReawaken(this));
			}
		});
	}
}

void UPawHudScreen::BuildOverlay(UOverlay* Root)
{
	UWidgetTree* T = WidgetTree;

	// The status row: the currencies, the wave line, the zone plate (hud.tscn StatusPanel; a 600-wide click-through row).
	StatusRow = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusRow"));
	StatusRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StatusSlot = Root->AddChildToOverlay(StatusRow);
	CurrencyBar = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CurrencyBar"));
	CurrencyBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StatusRow->AddChildToVerticalBox(CurrencyBar);
	// R4d: each currency a plum pill, its number heavy Bone on the plate (no outline: it no longer stands on the field).
	auto AddCurrency = [&](const TCHAR* Kind, const TCHAR* IconName, TObjectPtr<UImage>& Icon, TObjectPtr<UTextBlock>& Label, const TCHAR* Name,
		const TCHAR* PillName, bool bFirst)
	{
		UBorder* Pill = T->ConstructWidget<UBorder>(UBorder::StaticClass(), PillName);
		Pill->SetBrush(PawUi::Chrome(PawUi::EPawChrome::PlateSmall));
		Pill->SetPadding(PawUi::PlatePadding(FMargin(PawUi::R4PillPadX, PawUi::R4PillPadY), PawUi::EPawChrome::PlateSmall));
		Pill->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBoxSlot* PS = CurrencyBar->AddChildToHorizontalBox(Pill);
		PS->SetVerticalAlignment(VAlign_Center);
		PS->SetPadding(FMargin(bFirst ? 0.0f : static_cast<float>(PawUi::R4PillGap), 0.0f, 0.0f, 0.0f));
		// Named: an auto-name collides with one the asset already holds when the pill is grafted in (add:).
		UHorizontalBox* Line = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(FString(Kind) + TEXT("Line")));
		Pill->SetContent(Line);
		Icon = CurrencyIcon(T, Kind, IconName);
		UHorizontalBoxSlot* S = Line->AddChildToHorizontalBox(Icon);
		S->SetVerticalAlignment(VAlign_Center);
		S->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		USizeBox* Box = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(FString(Kind) + TEXT("Box")));
		Box->SetMinDesiredWidth(PawUi::CurrencyLabelWidth);
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		Label = PawUi::Heavy(PawUi::MakeText(T, PawUi::R4CurrencyPx, PawTokens::UI_TEXT, false, Name));
		Box->SetContent(Label);
		UHorizontalBoxSlot* LS = Line->AddChildToHorizontalBox(Box);
		LS->SetVerticalAlignment(VAlign_Center);
	};
	AddCurrency(TEXT("catnip"), TEXT("CatnipIcon"), CatnipIcon, CatnipText, TEXT("CatnipText"), TEXT("CatnipPill"), true);
	AddCurrency(TEXT("stardust"), TEXT("StardustIcon"), StardustIcon, StardustText, TEXT("StardustText"), TEXT("StardustPill"), false);

	// The wave line, above the zone plate: "is anything happening where I stand".
	// R4d: on its plum plate (it stood on the field, outlined, over the bushes).
	UBorder* WavePlate = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WavePlate"));
	WavePlate->SetBrush(PawUi::Chrome(PawUi::EPawChrome::PlateSmall));
	WavePlate->SetPadding(PawUi::PlatePadding(FMargin(PawUi::R4WavePadX, PawUi::R4WavePadY), PawUi::EPawChrome::PlateSmall));
	WavePlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	StatusRow->AddChildToVerticalBox(WavePlate)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	WaveBox = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WaveBox"));
	WaveBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WavePlate->SetContent(WaveBox);
	// hud.gd: the Bait icon before the wave line (wrap_label "bait").
	WaveLine = IconLine(T, TEXT("bait"), PawUi::SmallPx, PawTokens::UI_TEXT, 0.0f, TEXT("WaveLine"), false);
	WaveBox->AddChildToVerticalBox(WaveLine);
	WaveBar = PawUi::MakeThemeBar(T, TEXT("WaveBar")); // a theme ProgressBar (hud.gd: no override)
	USizeBox* WaveBarBox = T->ConstructWidget<USizeBox>();
	WaveBarBox->SetHeightOverride(PawUi::WaveBarHeight);
	WaveBarBox->SetContent(WaveBar);
	WaveBarBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	WaveBox->AddChildToVerticalBox(WaveBarBox)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));

	// The zone plate: "when do I get somewhere new" (the small plum plate since R3's shapes; Godot's was paper at 0.92).
	ZonePlate = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ZonePlate"));
	ZonePlate->SetBrush(PawUi::Chrome(PawUi::EPawChrome::PlateSmall));
	ZonePlate->SetPadding(PawUi::PlatePadding(FMargin(PawUi::ZonePlatePadX, PawUi::ZonePlatePadY), PawUi::EPawChrome::PlateSmall));
	ZonePlate->SetVisibility(ESlateVisibility::HitTestInvisible);
	StatusRow->AddChildToVerticalBox(ZonePlate)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	UVerticalBox* ZoneBody = T->ConstructWidget<UVerticalBox>();
	ZonePlate->SetContent(ZoneBody);
	ZoneText = PawUi::MakeText(T, PawUi::SmallPx, PawTokens::UI_TEXT, false, TEXT("ZoneText"));
	ZoneBody->AddChildToVerticalBox(ZoneText);
	ZoneBar = PawUi::MakeBar(T, PawTokens::UI_SLOT, PawTokens::GOLD, PawTokens::UI_PANEL_FRAME, PawUi::R4BarEdge, 4.0f, TEXT("ZoneBar"));
	USizeBox* ZoneBarBox = T->ConstructWidget<USizeBox>();
	ZoneBarBox->SetHeightOverride(PawUi::ZoneBarHeight);
	ZoneBarBox->SetContent(ZoneBar);
	ZoneBody->AddChildToVerticalBox(ZoneBarBox)->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));

	// The Catches XP bar along the field's bottom: level now, the bar, level next (hud.tscn XPBar).
	XpRow = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("XpRow"));
	XpRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	XpSlot = Root->AddChildToOverlay(XpRow);
	auto LevelBox = [&](TObjectPtr<UTextBlock>& Label, ETextJustify::Type Justify, const TCHAR* Name)
	{
		USizeBox* Box = T->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(PawUi::XpLevelWidth);
		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		Label = PawUi::Heavy(PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, true, Name));
		Label->SetJustification(Justify);
		Box->SetContent(Label);
		UHorizontalBoxSlot* S = XpRow->AddChildToHorizontalBox(Box);
		S->SetVerticalAlignment(VAlign_Center);
	};
	LevelBox(XpNow, ETextJustify::Right, TEXT("XpNow"));
	// R4d: a raised track with a Deep Plum edge, the fill Butter (the board's level bar).
	XpBar = PawUi::MakeBar(T, PawTokens::UI_BUTTON, PawTokens::UI_SELECTED, PawTokens::UI_PANEL_FRAME, 3.0f, 6.0f, TEXT("XpBar"));
	USizeBox* XpBarBox = T->ConstructWidget<USizeBox>();
	XpBarBox->SetHeightOverride(PawUi::XpBarHeight);
	XpBarBox->SetContent(XpBar);
	XpBarBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	UHorizontalBoxSlot* BarSlot = XpRow->AddChildToHorizontalBox(XpBarBox);
	BarSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	BarSlot->SetVerticalAlignment(VAlign_Center);
	BarSlot->SetPadding(FMargin(12.0f, 0.0f));
	LevelBox(XpNext, ETextJustify::Left, TEXT("XpNext"));

	// The Pounce clock: the field's bottom corner away from the dock, above the XP bar.
	PounceClock = T->ConstructWidget<UPawPounceClock>(PawUi::Resolve<UPawPounceClock>(), TEXT("PounceClock"));
	ClockSlot = Root->AddChildToOverlay(PounceClock);
	// Its fire button beside it, on the side towards the field (hud.gd _place_pounce_gauge).
	PouncePrompt = T->ConstructWidget<UPawPromptIcon>(PawUi::Resolve<UPawPromptIcon>(), TEXT("PouncePrompt"));
	PouncePrompt->Action = TEXT("pounce");
	PouncePrompt->IconPx = static_cast<int32>(PawUi::PouncePromptPx);
	PouncePromptSlot = Root->AddChildToOverlay(PouncePrompt);

	// The boss panel, centred on the field (tier_controller.gd _summon_boss: paper, ink frame, status / bar / clock).
	USizeBox* BossBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BossBox"));
	BossBox->SetWidthOverride(PawUi::BossBannerWidth);
	BossBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BossSlot = Root->AddChildToOverlay(BossBox);
	BossPanel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BossPanel"));
	BossPanel->SetBrush(PawUi::Chrome(PawUi::EPawChrome::Window)); // R3: the plum plate
	BossPanel->SetPadding(PawUi::PlatePadding(FMargin(14.0f, 10.0f), PawUi::EPawChrome::Window));
	BossPanel->SetVisibility(ESlateVisibility::Collapsed);
	BossBox->SetContent(BossPanel);
	UVerticalBox* BossBody = T->ConstructWidget<UVerticalBox>();
	BossPanel->SetContent(BossBody);
	BossText = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false, TEXT("BossText"));
	BossText->SetJustification(ETextJustify::Center);
	BossBody->AddChildToVerticalBox(BossText)->SetHorizontalAlignment(HAlign_Center);
	BossBar = PawUi::MakeBar(T, PawTokens::UI_SLOT, PawTokens::UI_DANGER, PawTokens::UI_SLOT, 0.0f, 4.0f, TEXT("BossBar"));
	USizeBox* BossBarBox = T->ConstructWidget<USizeBox>();
	BossBarBox->SetHeightOverride(PawUi::BossBarHeight);
	BossBarBox->SetContent(BossBar);
	BossBody->AddChildToVerticalBox(BossBarBox)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
	BossTimer = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false, TEXT("BossTimer"));
	BossTimer->SetJustification(ETextJustify::Center);
	BossTimer->SetVisibility(ESlateVisibility::Collapsed);
	BossBody->AddChildToVerticalBox(BossTimer)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));

	// The zone plate's rainbow outline (hud.gd _build_zone_progress: a SIBLING under Root that follows the plate from the
	// outside; a child of the plate's container would be laid out as a row). It fills the overlay and paints round the plate.
	ZoneReady = T->ConstructWidget<UPawRainbowFrame>(PawUi::Resolve<UPawRainbowFrame>(), TEXT("ZoneReady"));
	ZoneReady->Margin = Pc::RainbowFrame::ZoneMargin;
	ZoneReady->Corner = Pc::RainbowFrame::ZoneCorner;
	ZoneReady->LineWidth = Pc::RainbowFrame::ZoneWidth;
	UOverlaySlot* ReadySlot = Root->AddChildToOverlay(ZoneReady);
	ReadySlot->SetHorizontalAlignment(HAlign_Fill);
	ReadySlot->SetVerticalAlignment(VAlign_Fill);
}

void UPawHudScreen::BuildDock(UOverlay* Root)
{
	UWidgetTree* T = WidgetTree;
	DockBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DockBox"));
	DockBox->SetWidthOverride(PawUi::DockWidth);
	DockSlot = Root->AddChildToOverlay(DockBox);
	UBorder* Panel = T->ConstructWidget<UBorder>();
	Panel->SetBrush(PawUi::Chrome(PawUi::EPawChrome::Window)); // R3: the plum plate
	Panel->SetPadding(PawUi::PlatePadding(FMargin(18.0f, 14.0f, 16.0f, 14.0f), PawUi::EPawChrome::Window)); // DockPanel's margins
	DockBox->SetContent(Panel);
	UVerticalBox* Body = T->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Body);

	// The tab strip: six tabs, one row (icons with phase 6's art; their names until then).
	UHorizontalBox* Strip = T->ConstructWidget<UHorizontalBox>();
	Body->AddChildToVerticalBox(Strip);
	auto TabPrompt = [&](const TCHAR* Name, const TCHAR* Action, bool bNext)
	{
		UPawPromptIcon* P = T->ConstructWidget<UPawPromptIcon>(PawUi::Resolve<UPawPromptIcon>(), Name);
		P->Action = Action;
		P->IconPx = static_cast<int32>(PawUi::TabPromptPx);
		UHorizontalBoxSlot* S = Strip->AddChildToHorizontalBox(P);
		S->SetVerticalAlignment(VAlign_Center);
		const float Gap = static_cast<float>(PawUi::TabPromptGap);
		S->SetPadding(FMargin(bNext ? Gap : 0.0f, 0.0f, bNext ? 0.0f : Gap, 0.0f));
		return P;
	};
	TabPrevPrompt = TabPrompt(TEXT("TabPrevPrompt"), TEXT("tab_prev"), false);
	for (int32 I = 0; I < TabCount; ++I)
	{
		UPawButton* Tab = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), PawUi::Numbered(TEXT("Tab"), I));
		Tab->Index = I;
		Tab->SetMinDimensions(0, static_cast<int32>(PawUi::TabHeight));
		// Icon-only (OnLayoutReady puts the icon and the tooltip on, in either mode): no words.
		UHorizontalBoxSlot* S = Strip->AddChildToHorizontalBox(Tab);
		S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		S->SetPadding(FMargin(I == 0 ? 0.0f : 3.0f, 0.0f, I + 1 == TabCount ? 0.0f : 3.0f, 0.0f));
		Tabs.Add(Tab);
	}
	TabNextPrompt = TabPrompt(TEXT("TabNextPrompt"), TEXT("tab_next"), true);
	// The separator under the strip (hud.tscn TabSeparator): the theme's HSeparator, 4 px tall (phase 6 slice 5a; its line
	// never shows, as in Godot: finding F41).
	UBorder* TabRule = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TabSeparator"));
	TabRule->SetBrush(PawUi::Chrome(PawUi::EPawChrome::Separator));
	TabRule->SetPadding(FMargin(0.0f, 2.0f));
	Body->AddChildToVerticalBox(TabRule)->SetPadding(FMargin(0.0f, 8.0f));

	Pages = T->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Pages"));
	UVerticalBoxSlot* PagesSlot = Body->AddChildToVerticalBox(Pages);
	PagesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	// The quick bar (hud.tscn QuickSeparator, QuickBar: 66 tall, centred, 14 apart; four TextureButtons 40 x 40 at its foot):
	// settings, the skill tree, save, save and back to the title. The mouse's (dock_navigator.gd never visits it; a pad has
	// Start and Y for the same things). Phase 6 slice 5b.
	UBorder* QuickRule = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuickSeparator"));
	QuickRule->SetBrush(PawUi::Chrome(PawUi::EPawChrome::Separator));
	QuickRule->SetPadding(FMargin(0.0f, 2.0f));
	Body->AddChildToVerticalBox(QuickRule)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	USizeBox* QuickBox = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuickBarBox"));
	QuickBox->SetMinDesiredHeight(QuickBarHeight);
	QuickBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Body->AddChildToVerticalBox(QuickBox)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	QuickBar = T->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QuickBar"));
	QuickBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	QuickBox->SetContent(QuickBar);
	if (USizeBoxSlot* QS = Cast<USizeBoxSlot>(QuickBar->Slot))
	{
		QS->SetHorizontalAlignment(HAlign_Center);
		QS->SetVerticalAlignment(VAlign_Fill);
	}
	const TCHAR* QuickNames[] = { TEXT("QuickSettings"), TEXT("QuickSkills"), TEXT("QuickSave"), TEXT("QuickQuit") };
	for (int32 I = 0; I < 4; ++I)
	{
		UPawButton* Q = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), QuickNames[I]);
		TObjectPtr<UPawButton>* Parts[] = { &QuickSettings, &QuickSkills, &QuickSave, &QuickQuit };
		*Parts[I] = Q;
		Q->SetBare(true);
		Q->SetMinDimensions(static_cast<int32>(QuickIconPx), static_cast<int32>(QuickIconPx));
		UHorizontalBoxSlot* S = QuickBar->AddChildToHorizontalBox(Q);
		S->SetVerticalAlignment(VAlign_Bottom); // size_flags_vertical SHRINK_END
		S->SetPadding(FMargin(I == 0 ? 0.0f : static_cast<float>(QuickSeparation), 0.0f, 0.0f, 0.0f));
	}
	// A one-line legend at the foot of the dock (hud.gd: a PromptLabel, 14 px, prompts 22 px, centred).
	DockLegend = T->ConstructWidget<UPawPromptText>(PawUi::Resolve<UPawPromptText>(), TEXT("DockLegend"));
	DockLegend->IconPx = static_cast<int32>(PawUi::DockLegendPromptPx);
	DockLegend->TextPx = static_cast<float>(PawUi::DockLegendPx);
	DockLegend->TextColour = PawTokens::UI_TEXT;
	Body->AddChildToVerticalBox(DockLegend)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	// hud.tscn QuickStatus: "Saved." after the quick bar's save (centred, wrapping); empty, it takes no room.
	QuickStatus = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false, TEXT("QuickStatus"));
	QuickStatus->SetJustification(ETextJustify::Center);
	Body->AddChildToVerticalBox(QuickStatus)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	for (int32 I = 0; I < TabCount; ++I)
	{
		UVerticalBox* Page = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), PawUi::Numbered(TEXT("Page"), I));
		Pages->AddChild(Page);
		if (static_cast<PcDockTab>(I) == PcDockTab::Upgrades)
		{
			BuildUpgradesPage(Page);
			continue;
		}
		if (static_cast<PcDockTab>(I) == PcDockTab::Realms)
		{
			BuildRealmsPage(Page);
			continue;
		}
		switch (static_cast<PcDockTab>(I))
		{
		case PcDockTab::Reawaken: BuildReawakenPage(Page); break;
		case PcDockTab::Catches: BuildCatchesPage(Page); break;
		case PcDockTab::Achievements: BuildAchievementsPage(Page); break;
		default: BuildJobsPage(Page); break;
		}
	}
}

void UPawHudScreen::BuildUpgradesPage(UVerticalBox* Page)
{
	UWidgetTree* T = WidgetTree;
	UHorizontalBox* Header = T->ConstructWidget<UHorizontalBox>();
	Page->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	UTextBlock* Title = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false,
		PawUi::Numbered(TEXT("PageTitle"), static_cast<int32>(PcDockTab::Upgrades)));
	Title->SetText(PawText::Ui(PageKey(PcDockTab::Upgrades)));
	Title->SetJustification(ETextJustify::Center);
	UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Title);
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleSlot->SetVerticalAlignment(VAlign_Center);
	PageTitles.Add(Title);
	MultiplierButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("MultiplierButton"));
	MultiplierButton->SetMinDimensions(static_cast<int32>(PawUi::MultiplierButtonWidth), static_cast<int32>(PawUi::MultiplierButtonHeight));
	Header->AddChildToHorizontalBox(MultiplierButton)->SetVerticalAlignment(VAlign_Center);

	// The rows: BuildShopRows, at runtime, into this page (Page0).
}

namespace
{
	void PadScrollChild(UWidget* W); // below, with the pages
}

// --- the rows the data decides (runtime, into named containers: never baked into the Widget Blueprint) ---------------

void UPawHudScreen::BuildShopRows()
{
	UWidgetTree* T = WidgetTree;
	UVerticalBox* Page = Cast<UVerticalBox>(T->FindWidget(PawUi::Numbered(TEXT("Page"), static_cast<int32>(PcDockTab::Upgrades))));
	if (!Page || Rows.Num() > 0)
	{
		return;
	}
	const std::vector<std::string>& Order = Pc::ShopRowOrder();
	for (int32 I = 0; I < static_cast<int32>(Order.size()); ++I)
	{
		UPawButton* Row = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Row->Index = I;
		Row->bRepeats = true;
		Row->SetMinDimensions(0, static_cast<int32>(PawUi::R4RowHeight)); // R4e: the board's taller rows, words on the left
		Row->SetAlignLeft(true);
		Row->SetLabelSize(PawUi::R4RowPx);
		// A row buys on the PRESS (a held press repeats, dock_navigator.gd), not on the release.
		Row->SetPressMethod(EButtonPressMethod::ButtonPress);
		Row->SetClickMethod(EButtonClickMethod::MouseDown);
		Row->OnClicked().AddWeakLambda(this, [this, I]() { PressRow(I); });
		Page->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, PawUi::RowGap));
		Rows.Add(Row);
	}
}

void UPawHudScreen::BuildRealmRows()
{
	UWidgetTree* T = WidgetTree;
	UScrollBox* Scroll = Cast<UScrollBox>(T->FindWidget(TEXT("RealmList")));
	if (!Scroll || RealmRows.Num() > 0)
	{
		return;
	}
	const UPawGameSubsystem* G = nullptr;
	if (const UWorld* W = GetWorld())
	{
		if (const UPawRealmSubsystem* R = W->GetSubsystem<UPawRealmSubsystem>())
		{
			G = R->GetGame();
		}
	}
	const int32 Count = G && G->IsDataReady() ? static_cast<int32>(G->Data().Tiers.size()) : 12;
	for (int32 I = 0; I < Count; ++I)
	{
		UPawButton* Row = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Row->Index = I;
		Row->SetMinDimensions(0, static_cast<int32>(RealmRowHeight));
		Row->SetWrapAt(ButtonWrap);
		Row->OnClicked().AddWeakLambda(this, [this, I]() { PressRealm(I); });
		Scroll->AddChild(Row);
		if (UPanelSlot* S = Row->Slot)
		{
			if (UScrollBoxSlot* SS = Cast<UScrollBoxSlot>(S))
			{
				SS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, PawUi::RowGap));
			}
		}
		RealmRows.Add(Row);
	}
}

void UPawHudScreen::BuildCardRows()
{
	UWidgetTree* T = WidgetTree;
	UScrollBox* Scroll = Cast<UScrollBox>(T->FindWidget(TEXT("CardList")));
	if (!Scroll || CardRows.Num() > 0)
	{
		return;
	}
	const UPawGameSubsystem* G = Game();
	const int32 Count = G && G->IsDataReady() ? static_cast<int32>(G->Data().Cards.size()) : 0;
	for (int32 I = 0; I < Count; ++I)
	{
		UPawButton* Row = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Row->Index = I;
		Row->SetFlat(true);
		Row->SetAlignLeft(true);
		Row->SetLabelSize(CardHeadingPx);
		Row->SetWrapAt(ButtonWrap);
		Scroll->AddChild(Row);
		PadScrollChild(Row);
		CardRows.Add(Row);
	}
}

void UPawHudScreen::BuildAchievementRows()
{
	UWidgetTree* T = WidgetTree;
	UScrollBox* Scroll = Cast<UScrollBox>(T->FindWidget(TEXT("AchievementList")));
	if (!Scroll || ClaimButtons.Num() > 0)
	{
		return;
	}
	const UPawGameSubsystem* G = Game();
	const int32 Count = G && G->IsDataReady() ? static_cast<int32>(G->Data().Achievements.size()) : 0;
	for (int32 I = 0; I < Count; ++I)
	{
		UHorizontalBox* Line = T->ConstructWidget<UHorizontalBox>();
		// The icon, by constructed name (hud.gd: Art.texture("achievement_%s")), 36 px, 8 px before the text.
		UImage* Icon = T->ConstructWidget<UImage>();
		Icon->SetBrush(PawUi::IconBrush(PawUi::AchievementIconName(UTF8_TO_TCHAR(G->Data().Achievements[static_cast<size_t>(I)].Id.c_str())), AchievementIconPx));
		Icon->SetVisibility(Icon->GetBrush().GetResourceObject() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* IconSlot = Line->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		AchievementIcons.Add(Icon);
		UBorder* Mark = T->ConstructWidget<UBorder>();
		Mark->SetPadding(FMargin(MarkSize * 0.5f));
		Mark->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBoxSlot* MarkSlot = Line->AddChildToHorizontalBox(Mark);
		MarkSlot->SetVerticalAlignment(VAlign_Top);
		MarkSlot->SetPadding(FMargin(0.0f, 6.0f, 8.0f, 0.0f));
		UVerticalBox* Texts = T->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* TextsSlot = Line->AddChildToHorizontalBox(Texts);
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Name = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT);
		Name->SetAutoWrapText(false);
		Name->SetWrapTextAt(RowTextWrap);
		Texts->AddChildToVerticalBox(Name);
		UTextBlock* Line2 = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT);
		Line2->SetAutoWrapText(false);
		Line2->SetWrapTextAt(RowTextWrap);
		Texts->AddChildToVerticalBox(Line2)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
		UPawButton* Claim = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Claim->Index = I;
		Claim->SetMinDimensions(static_cast<int32>(ClaimWidth), static_cast<int32>(ClaimHeight));
		Claim->SetVisibility(ESlateVisibility::Collapsed);
		Claim->OnClicked().AddWeakLambda(this, [this, I]() { PressClaim(I); });
		UHorizontalBoxSlot* ClaimSlot = Line->AddChildToHorizontalBox(Claim);
		ClaimSlot->SetVerticalAlignment(VAlign_Center);
		ClaimSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		Scroll->AddChild(Line);
		PadScrollChild(Line);
		AchievementMarks.Add(Mark);
		AchievementNames.Add(Name);
		AchievementDetails.Add(Line2);
		ClaimButtons.Add(Claim);
		AchievementMarkColours.Add(PawTokens::UI_TEXT_MUTED);
	}
}

void UPawHudScreen::BuildJobRows()
{
	UWidgetTree* T = WidgetTree;
	UScrollBox* Scroll = Cast<UScrollBox>(T->FindWidget(TEXT("JobList")));
	if (!Scroll || JobButtons.Num() > 0)
	{
		return;
	}
	const UPawGameSubsystem* G = Game();
	const int32 Count = G && G->IsDataReady() ? static_cast<int32>(Pc::AllJobs(G->Data()).size()) : 0;
	for (int32 I = 0; I < Count; ++I)
	{
		UHorizontalBox* Line = T->ConstructWidget<UHorizontalBox>();
		// The job's icon (its icon_path), 40 px, 8 px before the text (hud.gd _build_job_page).
		UImage* Icon = T->ConstructWidget<UImage>();
		Icon->SetBrush(PawUi::IconBrush(UTF8_TO_TCHAR(Pc::AllJobs(G->Data())[static_cast<size_t>(I)]->IconPath.c_str()), JobIconPx));
		Icon->SetVisibility(Icon->GetBrush().GetResourceObject() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		UHorizontalBoxSlot* IconSlot = Line->AddChildToHorizontalBox(Icon);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		JobIcons.Add(Icon);
		UVerticalBox* Texts = T->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* TextsSlot = Line->AddChildToHorizontalBox(Texts);
		TextsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextsSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Name = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT);
		Name->SetAutoWrapText(false);
		Name->SetWrapTextAt(RowTextWrap);
		Texts->AddChildToVerticalBox(Name);
		UTextBlock* Line2 = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT);
		Line2->SetAutoWrapText(false);
		Line2->SetWrapTextAt(RowTextWrap);
		Texts->AddChildToVerticalBox(Line2);
		UPawButton* State = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		State->Index = I;
		State->SetMinDimensions(static_cast<int32>(JobButtonWidth), static_cast<int32>(ClaimHeight));
		State->SetLocked(true);
		UHorizontalBoxSlot* StateSlot = Line->AddChildToHorizontalBox(State);
		StateSlot->SetVerticalAlignment(VAlign_Center);
		StateSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		Scroll->AddChild(Line);
		PadScrollChild(Line);
		JobNames.Add(Name);
		JobDetails.Add(Line2);
		JobButtons.Add(State);
	}
}

// --- layout -------------------------------------------------------------------------------------------------------

void UPawHudScreen::SetDockOnLeft(bool bLeft)
{
	if (UPawSettingsSubsystem* S = SettingsOf(this))
	{
		S->SetDockSide(bLeft ? TEXT("left") : TEXT("right")); // OnChanged applies it
	}
	ApplyDockSide();
}

bool UPawHudScreen::IsDockOnLeft() const
{
	return DockOnLeft(this);
}

bool UPawHudScreen::IsDockDrawnOnLeft() const
{
	return DockSlot && DockSlot->GetHorizontalAlignment() == HAlign_Left;
}

void UPawHudScreen::ApplyDockSide()
{
	const bool bLeft = DockOnLeft(this);
	const float Dock = static_cast<float>(PawUi::DockWidth);
	const float Edge = static_cast<float>(PawUi::EdgeMargin);
	if (DockSlot)
	{
		DockSlot->SetHorizontalAlignment(bLeft ? HAlign_Left : HAlign_Right);
		DockSlot->SetVerticalAlignment(VAlign_Fill);
	}
	// The status readout: the far top corner of the field, growing away from the screen edge.
	if (StatusSlot)
	{
		StatusSlot->SetHorizontalAlignment(bLeft ? HAlign_Right : HAlign_Left);
		StatusSlot->SetVerticalAlignment(VAlign_Top);
		StatusSlot->SetPadding(FMargin(bLeft ? 0.0f : Edge, static_cast<float>(PawUi::StatusRowTop), bLeft ? Edge : 0.0f, 0.0f));
		for (UWidget* Child : StatusRow->GetAllChildren())
		{
			if (UVerticalBoxSlot* S = Cast<UVerticalBoxSlot>(Child->Slot))
			{
				S->SetHorizontalAlignment(bLeft ? HAlign_Right : HAlign_Left);
			}
		}
		if (UVerticalBoxSlot* S = WaveLine ? Cast<UVerticalBoxSlot>(WaveLine->Slot) : nullptr)
		{
			S->SetHorizontalAlignment(bLeft ? HAlign_Right : HAlign_Left);
		}
	}
	// The XP bar runs along the bottom of the FIELD, from the dock's edge.
	if (XpSlot)
	{
		const float Inset = Dock + static_cast<float>(PawUi::XpDockGap);
		XpSlot->SetHorizontalAlignment(HAlign_Fill);
		XpSlot->SetVerticalAlignment(VAlign_Bottom);
		XpSlot->SetPadding(FMargin(bLeft ? Inset : Edge, 0.0f, bLeft ? Edge : Inset, static_cast<float>(PawUi::XpBarBottom)));
	}
	// The Pounce clock: the field's bottom corner AWAY from the dock, above the XP bar (whose top is 54 from the bottom).
	if (ClockSlot)
	{
		const float Margin = static_cast<float>(PawUi::PounceMargin);
		const float Floor = static_cast<float>(PawUi::XpBarBottom + 36.0 + PawUi::PounceMargin);
		ClockSlot->SetHorizontalAlignment(bLeft ? HAlign_Right : HAlign_Left);
		ClockSlot->SetVerticalAlignment(VAlign_Bottom);
		ClockSlot->SetPadding(FMargin(bLeft ? 0.0f : Margin, 0.0f, bLeft ? Margin : 0.0f, Floor));
	}
	// The dock's rows draw their press prompt outside the row, on the field side.
	if (UPawInputDeviceSubsystem* D = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr)
	{
		D->SetDockArea(DockBox, bLeft);
	}
	// Its fire button: beside the clock on the field side, centred on it.
	if (PouncePromptSlot)
	{
		const float Margin = static_cast<float>(PawUi::PounceMargin);
		const float Floor = static_cast<float>(PawUi::XpBarBottom + 36.0 + PawUi::PounceMargin);
		const float Beside = Margin + static_cast<float>(PawUi::PounceDiameter + PawUi::PouncePromptGap);
		const float Lift = Floor + static_cast<float>((PawUi::PounceDiameter - PawUi::PouncePromptPx) * 0.5);
		PouncePromptSlot->SetHorizontalAlignment(bLeft ? HAlign_Right : HAlign_Left);
		PouncePromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PouncePromptSlot->SetPadding(FMargin(bLeft ? 0.0f : Beside, 0.0f, bLeft ? Beside : 0.0f, Lift));
	}
	// The boss panel centres on the FIELD, not the screen.
	if (BossSlot)
	{
		BossSlot->SetHorizontalAlignment(HAlign_Center);
		BossSlot->SetVerticalAlignment(VAlign_Top);
		BossSlot->SetPadding(FMargin(bLeft ? Dock : 0.0f, static_cast<float>(PawUi::BossBannerTop), bLeft ? 0.0f : Dock, 0.0f));
	}
}

void UPawHudScreen::ReportDockForSize(const FVector2D& Size)
{
	if (Size.Y <= 0.0 || (Size == LastSize && bLastLeft == DockOnLeft(this)))
	{
		return;
	}
	LastSize = Size;
	bLastLeft = DockOnLeft(this);
	// The field is 1080 logical units tall whatever the window; the UI's units follow its DPI curve.
	const double K = 1080.0 / Size.Y;
	const double W = PawUi::DockWidth * K;
	const double X = DockOnLeft(this) ? 0.0 : Size.X * K - W;
	ReportedDock = FVector4(X, 0.0, W, 1080.0);
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetDock(Pc::PcRect{ X, 0.0, W, 1080.0 }, DockOnLeft(this));
	}
}

// --- the dock: tabs, navigation, presses -----------------------------------------------------------------------------

UPawButton* UPawHudScreen::GetTabButton(Pc::PcDockTab Tab) const
{
	const int32 I = static_cast<int32>(Tab);
	return Tabs.IsValidIndex(I) ? Tabs[I].Get() : nullptr;
}

Pc::PcDockTab UPawHudScreen::GetActiveTab() const
{
	return GActiveTab;
}

void UPawHudScreen::RefreshLegend()
{
	// hud.gd _refresh_dock_legend: Buy and Amount on Upgrades, Select elsewhere.
	if (DockLegend)
	{
		DockLegend->SetTemplate(PawText::Ui(GActiveTab == PcDockTab::Upgrades ? TEXT("HUD_legend_upgrades") : TEXT("HUD_legend_other")));
	}
}

UPawPromptIcon* UPawHudScreen::GetTabPrompt(bool bNext) const
{
	return bNext ? TabNextPrompt : TabPrevPrompt;
}

FString UPawHudScreen::GetDockLegend() const
{
	return DockLegend ? DockLegend->GetNeutral() : FString();
}

UWidget* UPawHudScreen::GetDockWidget() const
{
	return DockBox;
}

void UPawHudScreen::SelectTab(Pc::PcDockTab Tab)
{
	const int32 I = FMath::Clamp(static_cast<int32>(Tab), 0, TabCount - 1);
	UPawAudioSubsystem::PlayFrom(this, TEXT("ui_click")); // hud.gd show_tab
	RememberFocus();
	if (static_cast<PcDockTab>(I) != PcDockTab::Reawaken && bReawakenArmed)
	{
		bReawakenArmed = false; // hud.gd show_tab: a half-armed confirm cannot survive the player looking away
	}
	GActiveTab = static_cast<PcDockTab>(I);
	if (GActiveTab == PcDockTab::Jobs)
	{
		if (const UPawGameSubsystem* G = Game(); G && G->IsDataReady())
		{
			GJobsSeen = static_cast<int64>(G->Progress().JobsHeld.size());
		}
	}
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(I);
	}
	RefreshLegend();
	RefreshNow();
	if (IsActivated())
	{
		if (UWidget* Target = NativeGetDesiredFocusTarget())
		{
			Target->SetFocus();
		}
	}
}

void UPawHudScreen::PressRow(int32 Row)
{
	const std::vector<std::string>& Order = Pc::ShopRowOrder();
	UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady() || Row < 0 || Row >= static_cast<int32>(Order.size()))
	{
		return;
	}
	const std::string& Id = Order[static_cast<size_t>(Row)];
	if (Pc::PressShopRow(G->Data(), G->Progress(), Id, GBuyMultiplier) > 0)
	{
		// A Kitten Count purchase re-counts the kittens (Godot's upgrade_purchased); every purchase rebuilds the stats.
		if (UPawRealmSubsystem* R = Realm())
		{
			R->OnProgressChanged(Id == "kitten_count");
		}
		RefreshNow();
		Juice.Punch(Rows.IsValidIndex(Row) ? Rows[Row].Get() : nullptr); // _on_upgrade_purchased: the shared purchase punch
		return;
	}
	UPawAudioSubsystem::PlayFrom(this, TEXT("purchase_denied")); // _on_upgrade_button_pressed: nothing bought
	RefreshNow(); // anything the player causes answers on this frame
}

void UPawHudScreen::CycleMultiplier()
{
	GBuyMultiplier = Pc::NextBuyMultiplier(GBuyMultiplier);
	UPawAudioSubsystem::PlayFrom(this, TEXT("ui_click")); // _on_multiplier_button_pressed
	RefreshNow();
}

int64 UPawHudScreen::GetBuyMultiplier() const
{
	return GBuyMultiplier;
}

TArray<UWidget*> UPawHudScreen::PageColumn() const
{
	// Every shown button of the open page, top to bottom, LOCKED ONES INCLUDED: the player reads an unaffordable row as
	// much as an affordable one (dock_navigator.gd rows()).
	TArray<UWidget*> Column;
	auto AddShown = [&Column](UWidget* W)
	{
		if (W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden)
		{
			Column.Add(W);
		}
	};
	switch (GActiveTab)
	{
	case PcDockTab::Upgrades:
		AddShown(MultiplierButton);
		for (UPawButton* Row : Rows)
		{
			AddShown(Row);
		}
		break;
	case PcDockTab::Realms:
		AddShown(BaitButton);
		AddShown(AutoTravelButton);
		for (UPawButton* Row : RealmRows)
		{
			AddShown(Row);
		}
		break;
	case PcDockTab::Reawaken:
		AddShown(ReawakenButton);
		AddShown(CancelButton);
		AddShown(AskButton);
		break;
	case PcDockTab::Catches:
		AddShown(AutoPickButton);
		for (UPawButton* Row : CardRows)
		{
			AddShown(Row);
		}
		break;
	case PcDockTab::Achievements:
		AddShown(ClaimAllButton);
		for (UPawButton* Claim : ClaimButtons)
		{
			AddShown(Claim);
		}
		break;
	case PcDockTab::Jobs:
		for (UPawButton* Job : JobButtons)
		{
			AddShown(Job);
		}
		break;
	default:
		break;
	}
	return Column;
}

void UPawHudScreen::WriteNavigation()
{
	// Explicit navigation, the selection IS focus (docs/05 §6.1): tabs left/right among the visible ones; a tab down to the
	// page's column; the top of the column back up to the tab.
	TArray<UPawButton*> VisibleTabs;
	for (UPawButton* Tab : Tabs)
	{
		if (Tab->GetVisibility() != ESlateVisibility::Collapsed)
		{
			VisibleTabs.Add(Tab);
		}
	}
	const TArray<UWidget*> Column = PageColumn();
	UPawButton* Here = GetTabButton(GActiveTab);
	for (int32 I = 0; I < VisibleTabs.Num(); ++I)
	{
		UPawButton* Tab = VisibleTabs[I];
		Tab->SetNavigationRuleBase(EUINavigation::Up, EUINavigationRule::Stop);
		if (I > 0) { Tab->SetNavigationRuleExplicit(EUINavigation::Left, VisibleTabs[I - 1]); }
		else { Tab->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop); }
		if (I + 1 < VisibleTabs.Num()) { Tab->SetNavigationRuleExplicit(EUINavigation::Right, VisibleTabs[I + 1]); }
		else { Tab->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop); }
		if (Column.Num() > 0)
		{
			Tab->SetNavigationRuleExplicit(EUINavigation::Down, Column[0]);
		}
		else
		{
			Tab->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop);
		}
	}
	for (int32 I = 0; I < Column.Num(); ++I)
	{
		UWidget* W = Column[I];
		W->SetNavigationRuleExplicit(EUINavigation::Up, I > 0 ? Column[I - 1] : static_cast<UWidget*>(Here));
		if (I + 1 < Column.Num()) { W->SetNavigationRuleExplicit(EUINavigation::Down, Column[I + 1]); }
		else { W->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop); }
		W->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
		W->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop);
	}
}

void UPawHudScreen::RememberFocus()
{
	// Follows focus continuously: Common UI re-applies the focus target when the input device changes (spike S4).
	const int32 Tab = static_cast<int32>(GActiveTab);
	if (!Remembered.IsValidIndex(Tab))
	{
		return;
	}
	for (UWidget* W : PageColumn())
	{
		if (HasFocus(W))
		{
			Remembered[Tab] = W;
			return;
		}
	}
}

UWidget* UPawHudScreen::NativeGetDesiredFocusTarget() const
{
	// The remembered widget if it is still in the column, else the column's top, else the tab: never nothing (the pad dies).
	const int32 Tab = static_cast<int32>(GActiveTab);
	const TArray<UWidget*> Column = PageColumn();
	if (Remembered.IsValidIndex(Tab) && Remembered[Tab].IsValid() && Column.Contains(Remembered[Tab].Get()))
	{
		return Remembered[Tab].Get();
	}
	if (Column.Num() > 0)
	{
		// The Upgrades page opens on its first row, not on the multiplier (the shop is what the player came for).
		return GActiveTab == PcDockTab::Upgrades && Column.Num() > 1 ? Column[1] : Column[0];
	}
	return GetTabButton(GActiveTab);
}

TOptional<FUIInputConfig> UPawHudScreen::GetDesiredInputConfig() const
{
	// The free-cursor HUD (docs/05 §4; KB U/08 §3.2): capture during mouse down (with none, single clicks are lost; with
	// the "including initial mouse down" kinds pad A turns into a click), never locked, the cursor shown.
	return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::CaptureDuringMouseDown, EMouseLockMode::DoNotLock, false);
}

FReply UPawHudScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// dock_navigator.gd / hud.gd, through the input map (PawInput: the key's Godot actions): tab_prev / tab_next, tab_1..6,
	// buy_amount, open_upgrades, open_skill_tree, pause; press on a key the menus do not already take (the left trigger).
	// Rows move by Slate navigation (FPawNavigationConfig: Godot's ui_* keys, menu_up / menu_down's own keys among them).
	const FKey K = KeyEvent.GetKey();
	auto Is = [&K](const TCHAR* Action) { return PawInput::KeyIs(K, Action); };
	const int32 Here = static_cast<int32>(GActiveTab);
	auto Step = [this, Here](int32 Dir)
	{
		for (int32 N = 1; N < TabCount; ++N)
		{
			const int32 I = (Here + Dir * N + TabCount) % TabCount;
			if (Tabs[I] && Tabs[I]->GetVisibility() != ESlateVisibility::Collapsed)
			{
				SelectTab(static_cast<PcDockTab>(I));
				return;
			}
		}
	};
	// The dock takes its actions on the press that begins (Input.is_action_just_pressed): an echo is swallowed.
	const bool bDockKey = Is(TEXT("tab_next")) || Is(TEXT("tab_prev")) || Is(TEXT("buy_amount")) || Is(TEXT("open_upgrades"))
		|| Is(TEXT("menu_up")) || Is(TEXT("menu_down")) || Is(TEXT("press"));
	if (KeyEvent.IsRepeat() && bDockKey && DockActive())
	{
		return FReply::Handled();
	}
	if (Is(TEXT("tab_next")))
	{
		Step(1);
		return FReply::Handled();
	}
	if (Is(TEXT("tab_prev")))
	{
		Step(-1);
		return FReply::Handled();
	}
	if (Is(TEXT("buy_amount")))
	{
		CycleMultiplier();
		return FReply::Handled();
	}
	// pause (Esc, pad Start) opens the pause menu; back alone (pad B) has nothing to go back from here.
	if (Is(TEXT("pause")) && !KeyEvent.IsRepeat() && OpenPauseMenu())
	{
		return FReply::Handled();
	}
	if (Is(TEXT("open_skill_tree")) && !KeyEvent.IsRepeat())
	{
		OpenSkillTree(false); // browse, read-only
		return FReply::Handled();
	}
	// open_upgrades (hud.gd): jump to the Upgrades tab from wherever you are.
	if (Is(TEXT("open_upgrades")))
	{
		SelectTab(PcDockTab::Upgrades);
		return FReply::Handled();
	}
	for (int32 I = 0; I < TabCount; ++I)
	{
		if (Is(*FString::Printf(TEXT("tab_%d"), I + 1)) && Tabs[I] && Tabs[I]->GetVisibility() != ESlateVisibility::Collapsed)
		{
			SelectTab(static_cast<PcDockTab>(I));
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

bool UPawHudScreen::IsTabShown(int32 I) const
{
	return I >= 0 && I < TabCount && Tabs[I] && Tabs[I]->GetVisibility() != ESlateVisibility::Collapsed;
}

bool UPawHudScreen::PressSelected()
{
	UPawButton* Hit = nullptr;
	WidgetTree->ForEachWidget([&Hit](UWidget* W)
	{
		UPawButton* B = Cast<UPawButton>(W);
		if (!Hit && B && B->GetVisibility() != ESlateVisibility::Collapsed && (B->HasAnyUserFocus() || B->HasFocusedDescendants()))
		{
			Hit = B;
		}
	});
	if (!Hit || Hit->IsLocked() || !Hit->GetIsEnabled() || !Hit->IsInteractionEnabled())
	{
		return false;
	}
	Hit->OnClicked().Broadcast();
	return true;
}

void UPawHudScreen::ForgetJobsSeen()
{
	GJobsSeen = -1;
}

void UPawHudScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshNow();
	// The dock takes its keys before anything else sees them (dock_navigator.gd _input).
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPawInputDeviceSubsystem* D = GI->GetSubsystem<UPawInputDeviceSubsystem>())
		{
			TWeakObjectPtr<UPawHudScreen> Weak(this);
			D->ConsumeForDock = [Weak](const FKey& Key) { return Weak.IsValid() && Weak->DockTakesKey(Key); };
		}
	}
}

void UPawHudScreen::NativeDestruct()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPawInputDeviceSubsystem* D = GI->GetSubsystem<UPawInputDeviceSubsystem>())
		{
			D->ConsumeForDock = nullptr;
		}
	}
	Super::NativeDestruct();
}

// --- refresh --------------------------------------------------------------------------------------------------------

UPawRealmSubsystem* UPawHudScreen::Realm() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
}

UPawGameSubsystem* UPawHudScreen::Game() const
{
	const UPawRealmSubsystem* R = Realm();
	return R ? R->GetGame() : nullptr;
}

void UPawHudScreen::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	ReportDockForSize(FVector2D(Geometry.GetLocalSize()));
	Pump(DeltaSeconds);
}

void UPawHudScreen::Pump(double DeltaSeconds)
{
	ReadFieldEvents();
	{
		// _held_by_other_frame: anything but the pause menu holding the field, or up on a stack.
		const UPawRealmSubsystem* R = Realm();
		const bool bPauseUp = PauseMenu && PauseMenu->IsActivated();
		const bool bMenu = MenuStack.IsValid() && MenuStack->GetNumWidgets() > (bPauseUp ? 1 : 0);
		const bool bModal = ModalStack.IsValid() && ModalStack->GetNumWidgets() > 0;
		if (bMenu || bModal || (R && R->IsPaused() && !bPauseUp))
		{
			HeldByOtherFrame = GFrameCounter;
		}
	}
	OpenCardPickIfDealt();
	ResumeReawakenIfPending();
	// The ceremony's tween stops while anything holds the field (Godot: a pausable node's tween), so a hand dealt under the
	// flash freezes it until the card is taken.
	if (Ceremony && Ceremony->IsPlaying())
	{
		const UPawRealmSubsystem* R = Realm();
		if ((!R || !R->IsPaused()) && Ceremony->Step(DeltaSeconds))
		{
			bSpendPending = true; // finished -> _open_skill_tree_for_spending
		}
	}
	if (bSpendPending)
	{
		OpenSpendStepWhenFree();
	}
	RememberFocus();
	TickRepeat(static_cast<float>(DeltaSeconds));
	// The clock moves every frame (it is a clock); the rest is dirty work at 15 Hz.
	if (const UPawRealmSubsystem* R = Realm(); R && PounceClock)
	{
		if (const UPawGameSubsystem* G = Game(); G && G->IsDataReady())
		{
			const Pc::PcPounceClock C = Pc::PounceClock(R->GetSim().Field, G->Progress());
			PounceClock->SetFraction(C.Fraction);
		}
	}
	RefreshClock += DeltaSeconds;
	if (RefreshClock >= 1.0 / PawUi::HudRefreshHz)
	{
		RefreshClock = 0.0;
		RefreshNow();
	}
	TickReawakenGlow();
	Juice.Tick(DeltaSeconds);
}

void UPawHudScreen::TickReawakenGlow()
{
	const UPawGameSubsystem* G = Game();
	const int32 Index = static_cast<int32>(PcDockTab::Reawaken);
	UPawButton* Tab = Tabs.IsValidIndex(Index) ? Tabs[Index].Get() : nullptr;
	if (!Tab || !G || !G->IsDataReady())
	{
		return;
	}
	// The tab jitters as well as glows while a Reawaken is waiting: the one thing on the dock to do next. Not while its own
	// page is open (an invitation to a room you are in).
	const bool bSuggested = Pc::IsReawakenSuggested(G->Data(), G->Progress());
	Juice.Jitter(Tab, bSuggested && GActiveTab != PcDockTab::Reawaken, FPawJuice::EffectsOn(this));
	if (!bSuggested)
	{
		Tab->SetColorAndOpacity(FLinearColor::White); // restored, not left where the cycle stopped
		return;
	}
	const double Hue = FMath::Fmod(FPlatformTime::Seconds() * PawUi::ReawakenGlowHz, 1.0);
	Tab->SetColorAndOpacity(PawUi::FromHsv(Hue, PawUi::ReawakenGlowSaturation, 1.0));
}

void UPawHudScreen::SetMenuStack(UCommonActivatableWidgetStack* Stack)
{
	MenuStack = Stack;
}

void UPawHudScreen::SetModalStack(UCommonActivatableWidgetStack* Stack)
{
	ModalStack = Stack;
}

bool UPawHudScreen::IsCardPickUp() const
{
	return CardPick && CardPick->IsActivated();
}

bool UPawHudScreen::IsMenuUp() const
{
	const UCommonActivatableWidgetStack* Stack = MenuStack.Get();
	return Stack && Stack->GetNumWidgets() > 0;
}

bool UPawHudScreen::OpenSkillTree(bool bSpend)
{
	UCommonActivatableWidgetStack* Stack = MenuStack.Get();
	if (!Stack || Stack->GetNumWidgets() > 0)
	{
		return false;
	}
	SkillTree = Stack->AddWidget<UPawSkillTreeScreen>(PawUi::Resolve<UPawSkillTreeScreen>(), [this, bSpend](UPawSkillTreeScreen& Tree)
	{
		Tree.SetSpendMode(bSpend);
		// The tree calls it only when the spend step ends, never after a browse (skill_tree_ui.gd: `if was_spending`).
		Tree.OnSpendingFinished = [this]() { ReturnToStartingBiome(); };
	});
	return SkillTree != nullptr;
}

bool UPawHudScreen::OpenPauseMenu()
{
	UCommonActivatableWidgetStack* Stack = MenuStack.Get();
	const UPawRealmSubsystem* R = Realm();
	if (!Stack || IsMenuUp() || (ModalStack.IsValid() && ModalStack->GetNumWidgets() > 0) || (R && R->IsPaused()))
	{
		return false;
	}
	if (HeldByOtherFrame != 0 && GFrameCounter - HeldByOtherFrame <= 1)
	{
		return false;
	}
	PauseMenu = Stack->AddWidget<UPawPauseMenu>(PawUi::Resolve<UPawPauseMenu>(), [this](UPawPauseMenu& Menu)
	{
		Menu.SetModalStack(ModalStack.Get()); // its Settings
	});
	return PauseMenu != nullptr;
}

void UPawHudScreen::OpenCardPickIfDealt()
{
	UCommonActivatableWidgetStack* Stack = MenuStack.Get();
	const UPawGameSubsystem* G = Game();
	if (!Stack || !G || !G->IsDataReady() || G->Progress().Hand.empty())
	{
		return;
	}
	if (CardPick && Stack->GetWidgetList().Contains(CardPick.Get()))
	{
		return; // up, or on the stack and not yet active (covered, or mid-transition): never a second one
	}
	CardPick = Stack->AddWidget<UPawCardPickScreen>(PawUi::Resolve<UPawCardPickScreen>());
}

bool UPawHudScreen::DockActive() const
{
	// is_active(): the HUD up and nothing holding the field (a paused tree stops the dock: a menu, a modal, the pause menu).
	const UPawRealmSubsystem* R = Realm();
	return IsActivated() && !IsMenuUp() && !(ModalStack.IsValid() && ModalStack->GetNumWidgets() > 0) && !(R && R->IsPaused());
}

UWidget* UPawHudScreen::DockSelection(int32* OutIndex) const
{
	// has_selection(): a row of the open page has the focus.
	const TArray<UWidget*> Column = PageColumn();
	for (int32 I = 0; I < Column.Num(); ++I)
	{
		if (Column[I] && (Column[I]->HasAnyUserFocus() || Column[I]->HasFocusedDescendants()))
		{
			if (OutIndex)
			{
				*OutIndex = I;
			}
			return Column[I];
		}
	}
	return nullptr;
}

void UPawHudScreen::RestoreDockSelection()
{
	// restore_selection(): the page's remembered row.
	if (UWidget* Target = NativeGetDesiredFocusTarget(); Target && GetOwningLocalPlayer())
	{
		Target->SetFocus();
	}
}

void UPawHudScreen::StepDockRow(int32 Direction)
{
	// step_row(): wraps (the bottom of the shop is one press from the top); nothing selected selects the remembered row.
	const TArray<UWidget*> Column = PageColumn();
	int32 Index = INDEX_NONE;
	if (Column.Num() == 0)
	{
		return;
	}
	if (!DockSelection(&Index))
	{
		RestoreDockSelection();
		return;
	}
	const int32 Next = ((Index + Direction) % Column.Num() + Column.Num()) % Column.Num();
	if (GetOwningLocalPlayer())
	{
		Column[Next]->SetFocus();
	}
}

void UPawHudScreen::TickRepeat(float DeltaSeconds)
{
	// _tick_nav / _tick_press (PcDockRepeat, exact against dock_navigator.gd): the dock polls what is HELD, on its own clock,
	// for every device. Nothing while it is not active (a paused tree); the hold starts over after.
	const UGameInstance* GI = GetGameInstance();
	const UPawInputDeviceSubsystem* D = GI ? GI->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
	if (!D || !DockActive())
	{
		DockRepeat = Pc::PcDockRepeat();
		bDockPressWas = D && D->IsActionHeld(TEXT("press")); // a press held through a menu is not a new press after it
		return;
	}
	// A tap that came and went between two frames still counts (it is held for this poll).
	UPawInputDeviceSubsystem* DM = const_cast<UPawInputDeviceSubsystem*>(D);
	const bool bTapUp = DM->TakeActionDown(TEXT("menu_up"));
	const bool bTapDown = DM->TakeActionDown(TEXT("menu_down"));
	const bool bTapPress = DM->TakeActionDown(TEXT("press"));
	const int64 Dir = Pc::DockTickNav(DockRepeat, D->IsActionHeld(TEXT("menu_up")) || bTapUp, D->IsActionHeld(TEXT("menu_down")) || bTapDown,
		DeltaSeconds);
	if (Dir != 0)
	{
		StepDockRow(static_cast<int32>(Dir));
	}
	const bool bPress = D->IsActionHeld(TEXT("press")) || bTapPress;
	UWidget* Selected = DockSelection();
	const UPawButton* Row = Cast<UPawButton>(Selected);
	const Pc::PcDockPress P = Pc::DockTickPress(DockRepeat, (bPress && !bDockPressWas) || bTapPress, bPress, Selected != nullptr,
		Row && Row->bRepeats, DeltaSeconds);
	bDockPressWas = bPress;
	if (P == Pc::PcDockPress::ShowSelection)
	{
		RestoreDockSelection();
	}
	else if (P == Pc::PcDockPress::Press)
	{
		PressSelected();
	}
}

bool UPawHudScreen::DockTakesKey(const FKey& Key) const
{
	// dock_navigator.gd _input: press, menu_up and menu_down are the dock's while it is active (polled in TickRepeat: the
	// first press at once, then Godot's repeat), never Slate's navigation or a button's own press. The tab keys are not
	// (the HUD takes them on their own press, below); a key that is both (the D-pad's left / right) stays a tab key.
	if (!DockActive() || PawInput::KeyIs(Key, TEXT("tab_prev")) || PawInput::KeyIs(Key, TEXT("tab_next")))
	{
		return false;
	}
	return PawInput::KeyIs(Key, TEXT("press")) || PawInput::KeyIs(Key, TEXT("menu_up")) || PawInput::KeyIs(Key, TEXT("menu_down"));
}

void UPawHudScreen::ReadFieldEvents()
{
	const UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcField& F = R->GetSim().Field;
	if (F.StepCount == LastEventStep)
	{
		return; // a paused frame still holds the last advance's events
	}
	LastEventStep = F.StepCount;
	const Pc::PcGameData& D = G->Data();
	if (F.TierIndex < 0 || F.TierIndex >= static_cast<int64_t>(D.Tiers.size()))
	{
		return;
	}
	const FText Boss = PawText::Data(TEXT("BOSS"), D.Tiers[static_cast<size_t>(F.TierIndex)].BossData);
	for (const Pc::PcFieldEvent& E : R->GetSim().FrameEvents)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("boss"), Boss);
		switch (E.Kind)
		{
		case Pc::PcFieldEventKind::BossSummoned:
			BossMessage = FText::GetEmpty();
			break;
		case Pc::PcFieldEventKind::BossDefeated:
			if (E.Flags & Pc::FieldFlag::New)
			{
				// "New realms", plural: a boss clear opens a whole block of biomes.
				Args.Add(TEXT("n"), PawText::Count(Pc::EvolutionStage(D, G->Progress())));
				BossMessage = PawText::Ui(TEXT("BOSS_defeated_first"), Args);
			}
			else
			{
				// A rematch says what it paid: the only reason to stand in a realm banked hours ago.
				Args.Add(TEXT("amount"), PawText::Amount(E.Value));
				BossMessage = PawText::Ui(TEXT("BOSS_defeated_again"), Args);
			}
			BossMessageTier = F.TierIndex;
			if (E.Flags & Pc::FieldFlag::Victory)
			{
				// final_boss_seconds records the FIRST clear only: a rematch shows the time that actually counted.
				ShowVictory(G->Progress().FinalBossSeconds);
			}
			break;
		case Pc::PcFieldEventKind::BossTimeout:
			BossMessage = PawText::Ui(TEXT("BOSS_out_of_time"), Args);
			BossMessageTier = F.TierIndex;
			break;
		default:
			break;
		}
	}
}

void UPawHudScreen::RefreshNow()
{
	++Refreshes;
	RefreshStatus();
	RefreshTabs();
	RefreshField();
	// Hidden pages do nothing (hud.gd: measured, a hidden page refreshed cost 39 ms a second).
	switch (GActiveTab)
	{
	case PcDockTab::Upgrades: RefreshShop(); break;
	case PcDockTab::Realms: RefreshRealms(); break;
	case PcDockTab::Reawaken: RefreshReawaken(); break;
	case PcDockTab::Catches: RefreshCatches(); break;
	case PcDockTab::Achievements: RefreshAchievements(); break;
	case PcDockTab::Jobs: RefreshJobs(); break;
	default: break;
	}
	WriteNavigation();
}

void UPawHudScreen::RefreshShop()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcProgress& P = G->Progress();
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("multiplier"), PawText::Ui(MultiplierKey(GBuyMultiplier)));
		MultiplierButton->SetLabel(PawText::Ui(TEXT("SHOP_buy_multiplier"), Args));
	}
	const std::vector<std::string>& Order = Pc::ShopRowOrder();
	for (int32 I = 0; I < Rows.Num(); ++I)
	{
		UPawButton* Row = Rows[I];
		const std::string& Id = Order[static_cast<size_t>(I)];
		const Pc::PcShopRow S = Pc::ShopRow(D, P, Id, GBuyMultiplier);
		SetShown(Row, S.Kind != Pc::PcShopRowKind::Hidden, ESlateVisibility::Visible);
		if (S.Kind == Pc::PcShopRowKind::Hidden)
		{
			continue;
		}
		FFormatNamedArguments Args;
		Args.Add(TEXT("name"), PawText::Data(TEXT("UPG"), Id));
		Args.Add(TEXT("level"), PawText::Count(S.Level));
		switch (S.Kind)
		{
		case Pc::PcShopRowKind::KittenLocked:
			Row->SetLabel(PawText::Ui(Id == "kitten_power" ? TEXT("SHOP_kitten_power_locked") : TEXT("SHOP_kitten_speed_locked")));
			break;
		case Pc::PcShopRowKind::Max:
			Row->SetLabel(PawText::Ui(TEXT("SHOP_row_max"), Args));
			break;
		case Pc::PcShopRowKind::Bulk:
			Args.Add(TEXT("levels"), PawText::Count(S.Levels));
			Args.Add(TEXT("price"), PawText::Amount(S.Price));
			Row->SetLabel(PawText::Ui(TEXT("SHOP_row_bulk"), Args));
			break;
		default:
			Args.Add(TEXT("price"), PawText::Amount(S.Price));
			Row->SetLabel(PawText::Ui(TEXT("SHOP_row_price"), Args));
			break;
		}
		Row->SetLocked(!S.bEnabled); // a row it cannot pay stays selectable: the player reads it
	}
}

void UPawHudScreen::RefreshStatus()
{
	const UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcProgress& P = G->Progress();
	const Pc::PcField& F = R->GetSim().Field;

	SetText(CatnipText, PawText::Amount(P.Catnip));
	SetText(StardustText, PawText::Amount(P.Stardust));

	const Pc::PcCatchesBar Xp = Pc::CatchesBar(D, P);
	SetText(XpNow, PawText::Count(Xp.Level));
	SetText(XpNext, PawText::Count(Xp.Level + 1));
	XpKept = PawUi::SetBarValue(XpBar, Xp.Fraction, PawUi::XpBarStep, 0.0); // hud.tscn: step 0.0001, a flat fill

	const Pc::PcWaveLine Wave = Pc::WaveLine(D, P);
	SetShown(WaveShown(), Wave.bVisible); // R4d: the plate goes with its line (an empty plum pill otherwise)
	if (Wave.bVisible)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("wave"), PawText::Count(Wave.Wave));
		if (Wave.bBait)
		{
			Args.Add(TEXT("held"), PawText::Count(Wave.Held));
			Args.Add(TEXT("needed"), PawText::Count(Wave.Needed));
			SetText(WaveLine, PawText::Ui(TEXT("WAVE_bait"), Args));
		}
		else
		{
			Args.Add(TEXT("done"), PawText::Amount(Wave.Done));
			Args.Add(TEXT("cost"), PawText::Amount(Wave.Cost));
			SetText(WaveLine, PawText::Ui(TEXT("WAVE_plain"), Args));
		}
		PawUi::SetBarValue(WaveBar, Wave.Fraction, Pc::GodotRangeStep, PawUi::ThemeFillMinWidth()); // the theme's nine-patch
	}

	const Pc::PcZonePlate Zone = Pc::ZonePlate(F, D, P);
	SetShown(ZonePlate, Zone.Kind != Pc::PcZonePlateKind::Hidden, ESlateVisibility::HitTestInvisible);
	// The plate leans back and forth while it shouts: the same nervous "go on" the Reawaken tab does.
	Juice.Jitter(ZonePlate, Zone.Kind != Pc::PcZonePlateKind::Hidden && Zone.bRainbow, FPawJuice::EffectsOn(this));
	if (Zone.Kind != Pc::PcZonePlateKind::Hidden)
	{
		FFormatNamedArguments Args;
		switch (Zone.Kind)
		{
		case Pc::PcZonePlateKind::Bait:
			Args.Add(TEXT("boss"), PawText::Data(TEXT("BOSS"), Zone.BossId));
			SetText(ZoneText, PawText::Ui(TEXT("ZONE_bait"), Args));
			break;
		case Pc::PcZonePlateKind::Ready:
			SetText(ZoneText, PawText::Ui(TEXT("ZONE_ready")));
			break;
		default:
			Args.Add(TEXT("have"), PawText::Amount(Zone.Have));
			Args.Add(TEXT("need"), PawText::Amount(Zone.Need));
			SetText(ZoneText, PawText::Ui(TEXT("ZONE_progress"), Args));
			break;
		}
		ZoneKept = PawUi::SetBarValue(ZoneBar, Zone.Fraction, Pc::GodotRangeStep, 0.0); // a flat fill: no minimum width
	}
	// The plate's rainbow outline while it shouts (zone_rainbow.active = can_travel or bait); it shows only with the plate.
	if (ZoneReady)
	{
		ZoneReady->SetActive(Zone.bRainbow);
	}
}

void UPawHudScreen::RefreshTabs()
{
	const UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcProgress& P = G->Progress();
	if (GJobsSeen < 0)
	{
		GJobsSeen = static_cast<int64>(P.JobsHeld.size()); // a loaded game does not badge jobs owned for days
	}
	bool bActiveShown = false;
	for (int32 I = 0; I < TabCount; ++I)
	{
		const PcDockTab Tab = static_cast<PcDockTab>(I);
		const Pc::PcTabState S = Pc::TabState(R->GetSim().Field, D, P, Tab, GActiveTab, GJobsSeen);
		UPawButton* Button = Tabs[I];
		SetShown(Button, S.bVisible, ESlateVisibility::Visible);
		Button->SetBadge(S.bBadge, S.Count);
		Button->SetActiveLook(Tab == GActiveTab); // btn.button_pressed: the theme's pressed plate
		bActiveShown = bActiveShown || (Tab == GActiveTab && S.bVisible);
	}
	// A tab can lock again: falling back beats a page with no tab lit.
	if (!bActiveShown && GActiveTab != PcDockTab::Upgrades)
	{
		SelectTab(PcDockTab::Upgrades);
	}
}

void UPawHudScreen::RefreshField()
{
	const UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcField& F = R->GetSim().Field;

	const Pc::PcPounceClock Clock = Pc::PounceClock(F, G->Progress());
	SetShown(PounceClock, Clock.bVisible, ESlateVisibility::HitTestInvisible);
	SetShown(PouncePrompt, Clock.bVisible, ESlateVisibility::HitTestInvisible);

	const Pc::PcBossPanel Panel = Pc::BossPanel(F, D);
	const bool bMessage = !BossMessage.IsEmpty() && BossMessageTier == F.TierIndex && !Panel.bVisible;
	SetShown(BossPanel, Panel.bVisible || bMessage, ESlateVisibility::HitTestInvisible);
	if (!Panel.bVisible)
	{
		if (bMessage)
		{
			SetText(BossText, BossMessage);
			BossBar->SetPercent(0.0f);
			SetShown(BossTimer, false);
		}
		return;
	}
	FFormatNamedArguments Args;
	Args.Add(TEXT("boss"), PawText::Data(TEXT("BOSS"), D.Tiers[static_cast<size_t>(F.TierIndex)].BossData));
	Args.Add(TEXT("phase"), PawText::Count(Panel.Phase));
	Args.Add(TEXT("hp"), PawText::Fixed(Panel.Fraction * 100.0, 1)); // "%.1f"
	if (Panel.bShielded)
	{
		FFormatNamedArguments Note;
		Note.Add(TEXT("note"), PawText::Ui(TEXT("BOSS_shielded")));
		Args.Add(TEXT("shield"), PawText::Ui(TEXT("BOSS_shield_suffix"), Note));
	}
	else
	{
		Args.Add(TEXT("shield"), FText::GetEmpty());
	}
	SetText(BossText, PawText::Ui(TEXT("BOSS_status"), Args));
	BossBar->SetPercent(static_cast<float>(Panel.Fraction));
	// Recolour rather than a second indicator: the bar is where the eye is during the fight.
	PawUi::SetBarFill(BossBar, Panel.bShielded ? PawTokens::STARDUST : PawTokens::UI_DANGER, 4.0f);
	SetShown(BossTimer, Panel.bTimerVisible, ESlateVisibility::HitTestInvisible);
	if (Panel.bTimerVisible)
	{
		const int64 Whole = static_cast<int64>(Panel.Remaining);
		FFormatNamedArguments T;
		T.Add(TEXT("m"), PawText::Count(Whole / 60));
		T.Add(TEXT("s"), PawText::TwoDigits(Whole % 60));
		SetText(BossTimer, PawText::Ui(TEXT("BOSS_timer"), T));
		PawUi::SetTextColour(BossTimer, Panel.bTimerDanger ? PawTokens::UI_DANGER : PawTokens::UI_TEXT);
	}
}

// --- the Realms and Reawaken pages ------------------------------------------------------------------------------------

UTextBlock* UPawHudScreen::PageText(UVerticalBox* Page, const FLinearColor& Colour, float WrapAt, FName Name)
{
	UTextBlock* T = PawUi::MakeText(WidgetTree, PawUi::LabelPx, Colour, false, Name);
	if (WrapAt > 0.0f)
	{
		T->SetAutoWrapText(false);
		T->SetWrapTextAt(WrapAt);
	}
	Page->AddChildToVerticalBox(T)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	return T;
}

void UPawHudScreen::BuildRealmsPage(UVerticalBox* Page)
{
	UWidgetTree* T = WidgetTree;
	UTextBlock* Title = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false,
		PawUi::Numbered(TEXT("PageTitle"), static_cast<int32>(PcDockTab::Realms)));
	Title->SetText(PawText::Ui(PageKey(PcDockTab::Realms)));
	Page->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
	PageTitles.Add(Title);
	StageText = PageText(Page, PawTokens::UI_TEXT_MUTED, PageWrap, TEXT("StageText"));
	UTextBlock* Hint = PageText(Page, PawTokens::UI_TEXT, PageWrap);
	Hint->SetText(PawText::Ui(TEXT("REALMS_hint")));

	BaitButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("BaitButton"));
	BaitButton->SetMinDimensions(0, static_cast<int32>(RealmRowHeight));
	BaitButton->SetWrapAt(ButtonWrap);
	Page->AddChildToVerticalBox(BaitButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	AutoTravelButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("AutoTravelButton"));
	AutoTravelButton->SetLabel(PawText::Ui(TEXT("REALMS_auto_travel")));
	AutoTravelButton->SetCheck(true, false);
	Page->AddChildToVerticalBox(AutoTravelButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	// Twelve rows: the list is a map (hud.gd), scrolled when it is taller than the dock.
	UScrollBox* Scroll = T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RealmList"));
	UVerticalBoxSlot* ScrollSlot = Page->AddChildToVerticalBox(Scroll);
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
	// The rows: BuildRealmRows, at runtime, into RealmList.
}

void UPawHudScreen::BuildReawakenPage(UVerticalBox* Page)
{
	UWidgetTree* T = WidgetTree;
	UTextBlock* Title = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false,
		PawUi::Numbered(TEXT("PageTitle"), static_cast<int32>(PcDockTab::Reawaken)));
	Title->SetText(PawText::Ui(PageKey(PcDockTab::Reawaken)));
	Page->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
	PageTitles.Add(Title);
	PageText(Page, PawTokens::UI_TEXT, PageWrap)->SetText(PawText::Ui(TEXT("REAWAKEN_blurb")));
	ReawakenProgressLine = IconLine(T, TEXT("catnip"), PawUi::LabelPx, PawTokens::UI_TEXT, PageWrap - 32.0f, TEXT("ReawakenProgressLine"));
	Page->AddChildToVerticalBox(ReawakenProgressLine)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	// hud.tscn ReawakenBar: rainbow_bar.gd, the theme's track under a scrolling rainbow and its sparks (phase 6 slice 5d).
	ReawakenRainbow = T->ConstructWidget<UPawRainbowBar>(PawUi::Resolve<UPawRainbowBar>(), TEXT("ReawakenRainbow"));
	USizeBox* BarBox = T->ConstructWidget<USizeBox>();
	BarBox->SetHeightOverride(ReawakenBarHeight);
	BarBox->SetContent(ReawakenRainbow);
	Page->AddChildToVerticalBox(BarBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	PageText(Page, PawTokens::UI_TEXT, 0.0f)->SetText(PawText::Ui(TEXT("REAWAKEN_payout_header")));
	// The payout: the Stardust disc, then "+n" (the icon, not the word).
	UHorizontalBox* Payout = T->ConstructWidget<UHorizontalBox>();
	Page->AddChildToVerticalBox(Payout)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	PayoutIcon = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PayoutIcon"));
	PayoutIcon->SetBrush(PawUi::IconBrush(PawUi::CurrencyIconName(TEXT("stardust")), 24.0)); // LABEL_ICON_PX
	PayoutIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
	Payout->AddChildToHorizontalBox(PayoutIcon)->SetVerticalAlignment(VAlign_Center);
	PayoutText = PawUi::Heavy(PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false, TEXT("PayoutText")));
	UHorizontalBoxSlot* PayoutSlot = Payout->AddChildToHorizontalBox(PayoutText);
	PayoutSlot->SetVerticalAlignment(VAlign_Center);
	PayoutSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
	TotalLine = IconLine(T, TEXT("stardust"), PawUi::LabelPx, PawTokens::UI_TEXT, 0.0f, TEXT("TotalLine"));
	Page->AddChildToVerticalBox(TotalLine)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	MultiplierText = PageText(Page, PawTokens::UI_TEXT, PageWrap, TEXT("MultiplierText"));
	USpacer* Spacer = T->ConstructWidget<USpacer>();
	Page->AddChildToVerticalBox(Spacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UnicornText = PageText(Page, PawTokens::UI_TEXT, PageWrap, TEXT("UnicornText"));
	UnicornText->SetText(PawText::Ui(TEXT("REAWAKEN_unicorn")));

	ReawakenButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("ReawakenButton"));
	ReawakenButton->SetMinDimensions(0, static_cast<int32>(ReawakenButtonHeight));
	ReawakenButton->SetLabelSize(ReawakenButtonPx);
	ReawakenButton->SetWrapAt(ButtonWrap);
	Page->AddChildToVerticalBox(ReawakenButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	CancelButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("CancelButton"));
	CancelButton->SetLabel(PawText::Ui(TEXT("REAWAKEN_cancel")));
	CancelButton->SetVisibility(ESlateVisibility::Collapsed);
	Page->AddChildToVerticalBox(CancelButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	AskButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("AskButton"));
	AskButton->SetLabel(PawText::Ui(TEXT("REAWAKEN_ask")));
	AskButton->SetCheck(true, true);
	Page->AddChildToVerticalBox(AskButton);
}

void UPawHudScreen::RefreshRealms()
{
	const UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcProgress& P = G->Progress();

	// The stage line: naming the boss in the way turns a readout into a direction to walk in.
	const Pc::PcStageLine Stage = Pc::StageLine(D, P);
	FFormatNamedArguments StageArgs;
	StageArgs.Add(TEXT("n"), PawText::Count(Stage.Stage));
	if (Stage.bFullyEvolved)
	{
		SetText(StageText, PawText::Ui(TEXT("REALMS_stage_evolved"), StageArgs));
	}
	else
	{
		FText Boss = PawText::Ui(TEXT("REALMS_next_boss"));
		if (!Stage.NextBossTier.empty())
		{
			const int64 Index = Pc::RealmIndex(D, Stage.NextBossTier);
			FFormatNamedArguments In;
			In.Add(TEXT("boss"), PawText::Data(TEXT("BOSS"), D.Tiers[static_cast<size_t>(Index)].BossData));
			In.Add(TEXT("biome"), PawText::Data(TEXT("TIER"), Stage.NextBossTier));
			Boss = PawText::Ui(TEXT("REALMS_boss_in"), In);
		}
		StageArgs.Add(TEXT("boss"), Boss);
		SetText(StageText, PawText::Ui(TEXT("REALMS_stage_next"), StageArgs));
	}

	// The Bait button: shown in every biome until every boss has fallen this run. _refresh_bait_button stops its lean first
	// and starts it again at its end when the bait calls: so, as in Godot, the lean restarts on every refresh (finding F42).
	Juice.Jitter(BaitButton, false, true);
	const Pc::PcBaitButton Bait = Pc::BaitButton(R->GetSim().Field, D, P);
	SetShown(BaitButton, Bait.Kind != Pc::PcBaitKind::Hidden, ESlateVisibility::Visible);
	if (Bait.Kind != Pc::PcBaitKind::Hidden)
	{
		FFormatNamedArguments Args;
		if (!Bait.BossId.empty())
		{
			Args.Add(TEXT("boss"), PawText::Data(TEXT("BOSS"), Bait.BossId));
		}
		switch (Bait.Kind)
		{
		case Pc::PcBaitKind::Elsewhere:
			Args.Add(TEXT("biome"), PawText::Data(TEXT("TIER"), Bait.NextBossTier));
			BaitButton->SetLabel(PawText::Ui(TEXT("BAIT_elsewhere"), Args));
			break;
		case Pc::PcBaitKind::BossHere:
			BaitButton->SetLabel(PawText::Ui(TEXT("BAIT_here"), Args));
			break;
		case Pc::PcBaitKind::BaitOwed:
			Args.Add(TEXT("n"), PawText::Count(Bait.BaitOwed));
			BaitButton->SetLabel(PawText::Ui(TEXT("BAIT_owed"), Args));
			break;
		case Pc::PcBaitKind::CatnipShort:
			Args.Add(TEXT("amount"), PawText::Amount(Bait.CatnipShort));
			BaitButton->SetLabel(PawText::Ui(TEXT("BAIT_catnip"), Args));
			break;
		default:
			BaitButton->SetLabel(PawText::Ui(TEXT("BAIT_ready"), Args));
			break;
		}
		// hud.gd: the Bait icon, or Catnip's where Catnip is what is missing (decorate_button: 26 px).
		BaitButton->SetIcon(PawUi::CurrencyIconName(Bait.Kind == Pc::PcBaitKind::CatnipShort ? TEXT("catnip") : TEXT("bait")), ButtonIconPx);
		BaitButton->SetLocked(!Bait.bEnabled);
		// Ready, and the boss still stands in the way: it calls (Godot warms it GOLD at 45 %; R4a: the Butter call plate).
		BaitButton->SetCallLook(Bait.bCalling);
		Juice.Jitter(BaitButton, Bait.bCalling, FPawJuice::EffectsOn(this)); // _bait_is_calling()
	}
	AutoTravelButton->SetCheck(true, Game() && Game()->Slot().bAutoTravel);
	// Hidden until the skill tree hands the automation over (hud.gd: a switch for something the player cannot do yet reads
	// as a broken switch).
	SetShown(AutoTravelButton, Pc::OwnsAutoTravel(D, P), ESlateVisibility::Visible);

	for (int32 I = 0; I < RealmRows.Num(); ++I)
	{
		UPawButton* Row = RealmRows[I];
		const Pc::PcRealmRow S = Pc::RealmRow(D, P, I);
		const std::string& Id = D.Tiers[static_cast<size_t>(I)].Id;
		FFormatNamedArguments Args;
		Args.Add(TEXT("biome"), PawText::Data(TEXT("TIER"), Id));
		FText Line;
		bool bColour = false, bColourLocked = false;
		FLinearColor Colour = PawTokens::UI_TEXT;
		switch (S.Kind)
		{
		case Pc::PcRealmRowKind::NeedsCatnip:
			Args.Add(TEXT("amount"), PawText::Amount(S.Owed));
			Line = PawText::Ui(TEXT("REALMS_row_owed"), Args);
			break;
		case Pc::PcRealmRowKind::Here:
			Line = PawText::Ui(TEXT("REALMS_row_here"), Args);
			bColour = bColourLocked = true; // "here" must not read like "locked": the two states a map most needs apart
			Colour = PawTokens::UI_GOOD;
			break;
		case Pc::PcRealmRowKind::TravelBoss:
			Args.Add(TEXT("boss"), PawText::Data(TEXT("BOSS"), S.BossId));
			Line = PawText::Ui(S.bCleared ? TEXT("REALMS_row_boss_cleared") : TEXT("REALMS_row_boss"), Args);
			bColour = true;
			Colour = S.bCleared ? PawTokens::STARDUST : PawTokens::UI_DANGER;
			break;
		case Pc::PcRealmRowKind::Travel:
			Line = PawText::Ui(TEXT("REALMS_row_travel"), Args);
			break;
		default:
			Line = PawText::Ui(TEXT("REALMS_row_locked"), Args);
			break;
		}
		if (S.bShowReward && (S.Kind == Pc::PcRealmRowKind::Travel || S.Kind == Pc::PcRealmRowKind::TravelBoss))
		{
			FFormatNamedArguments Reward;
			Reward.Add(TEXT("row"), Line);
			Reward.Add(TEXT("multiplier"), PawText::Fixed(S.RewardMultiplier, 1)); // "%.1f"
			Line = PawText::Ui(TEXT("REALMS_row_reward"), Reward);
		}
		Row->SetLabel(Line);
		// The amount's unit on a row that asks for Catnip; none where the row asks for nothing (hud.gd).
		Row->SetIcon(S.Kind == Pc::PcRealmRowKind::NeedsCatnip ? PawUi::CurrencyIconName(TEXT("catnip")) : FString(), ButtonIconPx);
		Row->SetLocked(!S.bEnabled);
		Row->SetLabelColour(bColour, Colour, bColourLocked);
		// A biome that has JUST become reachable warms up (BIOME_READY_TINT): exactly one thing on screen does this. R4a: the
		// Butter call plate with ink words (a 45 % GOLD tint over plum read khaki).
		Row->SetCallLook(S.bNew);
		Juice.Jitter(Row, S.bNew, FPawJuice::EffectsOn(this));
	}
}

void UPawHudScreen::RefreshReawaken()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcReawakenPage Page = Pc::ReawakenPage(G->Data(), G->Progress(), bReawakenArmed);
	FFormatNamedArguments Progress;
	Progress.Add(TEXT("earned"), PawText::Amount(Page.Earned));
	Progress.Add(TEXT("required"), PawText::Amount(Page.Required));
	// After the first Reawaken the bar is a SUGGESTION: the wording stops implying a lock.
	SetText(ReawakenProgressLine, PawText::Ui(Page.bFirst ? TEXT("REAWAKEN_progress_first") : TEXT("REAWAKEN_progress_later"), Progress));
	if (ReawakenRainbow)
	{
		ReawakenRainbow->SetValue(Page.Fraction); // reawaken_bar.value = clampf(earned / maxf(required, 1.0), 0.0, 1.0)
	}
	FFormatNamedArguments Pay;
	Pay.Add(TEXT("amount"), PawText::Amount(Page.Pending));
	SetText(PayoutText, PawText::Ui(TEXT("REAWAKEN_payout"), Pay));
	FFormatNamedArguments Total;
	Total.Add(TEXT("amount"), PawText::Amount(Page.TotalAfter));
	SetText(TotalLine, PawText::Ui(TEXT("REAWAKEN_total"), Total));
	FFormatNamedArguments Mult;
	Mult.Add(TEXT("now"), PawText::Fixed(Page.MultiplierNow, 2)); // "%.2f"
	Mult.Add(TEXT("after"), PawText::Fixed(Page.MultiplierAfter, 2));
	SetText(MultiplierText, PawText::Ui(TEXT("REAWAKEN_multiplier"), Mult));
	SetShown(UnicornText, Page.bUnicorn, ESlateVisibility::HitTestInvisible);

	switch (Page.Button)
	{
	case Pc::PcReawakenButtonKind::Confirm:
		ReawakenButton->SetLabel(PawText::Ui(TEXT("REAWAKEN_button_confirm")));
		break;
	case Pc::PcReawakenButtonKind::Early:
		ReawakenButton->SetLabel(PawText::Ui(TEXT("REAWAKEN_button_early"), Pay));
		break;
	case Pc::PcReawakenButtonKind::Reawaken:
	{
		FFormatNamedArguments N;
		N.Add(TEXT("n"), PawText::Count(Page.Count));
		ReawakenButton->SetLabel(PawText::Ui(TEXT("REAWAKEN_button"), N));
		break;
	}
	default:
		ReawakenButton->SetLabel(PawText::Ui(TEXT("REAWAKEN_button_not_yet"), Pay));
		break;
	}
	ReawakenButton->SetLocked(!Page.bReady);
	SetShown(CancelButton, bReawakenArmed, ESlateVisibility::Visible);
	AskButton->SetCheck(true, ConfirmReawaken(this));
}

void UPawHudScreen::PressRealm(int32 Index)
{
	UPawRealmSubsystem* R = Realm();
	const UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcRealmRow Row = Pc::RealmRow(G->Data(), G->Progress(), Index);
	if (Row.bEnabled)
	{
		R->Travel(Index); // nothing is loaded: the realm swaps its field (docs/03 §4)
	}
	RefreshNow();
}

void UPawHudScreen::PressBait()
{
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SummonBoss(); // all or nothing: a refused summon keeps the bait
	}
	RefreshNow();
}

void UPawHudScreen::ToggleAutoTravel()
{
	if (UPawGameSubsystem* G = Game())
	{
		G->Slot().bAutoTravel = !G->Slot().bAutoTravel; // the realm reads it every frame
	}
	RefreshNow();
}

void UPawHudScreen::ToggleAskBeforeReawaken()
{
	// SettingsManager.set_confirm_reawaken(): a machine preference, saved with the settings.
	if (UPawSettingsSubsystem* S = SettingsOf(this))
	{
		S->SetConfirmReawaken(!S->Get().bConfirmReawaken);
	}
	RefreshNow();
}

// --- the Catches, Achievements and Jobs pages -------------------------------------------------------------------------

UScrollBox* UPawHudScreen::PageScroll(UVerticalBox* Page, FName Name)
{
	// The page's ONLY expanding child (hud.gd: two would halve the list against whatever else claimed the space).
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), Name);
	Page->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
	return Scroll;
}

namespace
{
	void PadScrollChild(UWidget* W)
	{
		if (UScrollBoxSlot* SS = W ? Cast<UScrollBoxSlot>(W->Slot) : nullptr)
		{
			SS->SetPadding(FMargin(0.0f, 0.0f, 0.0f, PawUi::RowGap));
		}
	}
}

void UPawHudScreen::BuildCatchesPage(UVerticalBox* Page)
{
	UWidgetTree* T = WidgetTree;
	UTextBlock* Title = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false,
		PawUi::Numbered(TEXT("PageTitle"), static_cast<int32>(PcDockTab::Catches)));
	Title->SetText(PawText::Ui(PageKey(PcDockTab::Catches)));
	Page->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
	PageTitles.Add(Title);
	CatchesSummary = PageText(Page, PawTokens::UI_TEXT, PageWrap, TEXT("CatchesSummary"));
	AutoPickButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("AutoPickButton"));
	AutoPickButton->SetLabel(PawText::Ui(TEXT("CATCHES_auto_pick")));
	AutoPickButton->SetCheck(true, false);
	AutoPickButton->SetVisibility(ESlateVisibility::Collapsed); // until Auto-Card is owned: a switch for nothing reads broken
	Page->AddChildToVerticalBox(AutoPickButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	// EVERY CARD, not only the taken ones (designer, 2026-09-11): a list of what you have cannot say how to get the rest.
	// Each row is a flat button so a pad can select it and read it (controller plan phase 3); pressing does nothing.
	PageScroll(Page, TEXT("CardList"));
	// The rows: BuildCardRows, at runtime, into CardList.
}

void UPawHudScreen::BuildAchievementsPage(UVerticalBox* Page)
{
	UWidgetTree* T = WidgetTree;
	UTextBlock* Title = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false,
		PawUi::Numbered(TEXT("PageTitle"), static_cast<int32>(PcDockTab::Achievements)));
	Title->SetText(PawText::Ui(PageKey(PcDockTab::Achievements)));
	Page->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
	PageTitles.Add(Title);
	AchievementSummaryLine = IconLine(T, TEXT("stardust"), PawUi::LabelPx, PawTokens::UI_TEXT, PageWrap - 32.0f, TEXT("AchievementSummaryLine"));
	Page->AddChildToVerticalBox(AchievementSummaryLine)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	ClaimAllButton = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("ClaimAllButton"));
	ClaimAllButton->SetVisibility(ESlateVisibility::Collapsed);
	Page->AddChildToVerticalBox(ClaimAllButton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));

	// One row per achievement: the mark, the text, a Claim button built once and hidden (hud.gd: forty-one rows churning
	// nodes on every kill is slower and leaves freed buttons connected).
	PageScroll(Page, TEXT("AchievementList"));
	// The rows: BuildAchievementRows, at runtime, into AchievementList.
}

void UPawHudScreen::BuildJobsPage(UVerticalBox* Page)
{
	UWidgetTree* T = WidgetTree;
	UTextBlock* Title = PawUi::MakeText(T, PawUi::LabelPx, PawTokens::UI_TEXT, false,
		PawUi::Numbered(TEXT("PageTitle"), static_cast<int32>(PcDockTab::Jobs)));
	Title->SetText(PawText::Ui(PageKey(PcDockTab::Jobs)));
	Page->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
	PageTitles.Add(Title);
	PageText(Page, PawTokens::UI_TEXT, PageWrap)->SetText(PawText::Ui(TEXT("JOBS_blurb")));

	// One row per job: the text, and a button that is a STATEMENT, not a control (Worn / Owned / Locked): swapping mid-run
	// is what the lock prevents. Locked, never disabled, so the pad still reads every row (UPawButton).
	PageScroll(Page, TEXT("JobList"));
	// The rows: BuildJobRows, at runtime, into JobList.
}

void UPawHudScreen::RefreshCatches()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcCatchesPage Page = Pc::CatchesPage(D, G->Progress(), G->Slot().bAutoPickCard);
	FText Summary;
	if (Page.Taken == 0)
	{
		Summary = PawText::Ui(TEXT("CATCHES_none"));
	}
	else
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), PawText::Count(Page.Taken));
		Summary = PawText::Ui(Page.Taken == 1 ? TEXT("CATCHES_taken_one") : TEXT("CATCHES_taken"), Args);
	}
	if (Page.bPickingAuto)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("summary"), Summary);
		Summary = PawText::Ui(TEXT("CATCHES_auto_suffix"), Args);
	}
	SetText(CatchesSummary, Summary);
	SetShown(AutoPickButton, Page.bAutoPickShown, ESlateVisibility::Visible);
	AutoPickButton->SetCheck(true, G->Slot().bAutoPickCard);

	for (int32 I = 0; I < CardRows.Num() && I < static_cast<int32>(Page.Rows.size()); ++I)
	{
		const Pc::PcCatchesRow& S = Page.Rows[static_cast<size_t>(I)];
		UPawButton* Row = CardRows[I];
		const FText Name = PawText::Data(TEXT("CARD"), S.CardId);
		if (S.bHeld)
		{
			FFormatNamedArguments Heading;
			Heading.Add(TEXT("name"), Name);
			Heading.Add(TEXT("level"), PawText::Count(S.Level));
			Heading.Add(TEXT("max"), PawText::Count(S.MaxLevel));
			Row->SetLabel(PawText::Ui(TEXT("CATCHES_held"), Heading));
			FFormatNamedArguments Values;
			Values.Add(TEXT("v"), PawText::CardNumber(S.Value));
			Values.Add(TEXT("c"), PawText::CardNumber(S.Cost));
			Row->SetDetail(FText::Format(PawText::DataField(TEXT("CARD"), S.CardId, TEXT("desc")), Values), PawTokens::UI_TEXT);
		}
		else
		{
			Row->SetLabel(Name);
			FText Hint;
			switch (S.Hint.Kind)
			{
			case Pc::PcCardHintKind::Banished: Hint = PawText::Ui(TEXT("CATCHES_hint_banished")); break;
			case Pc::PcCardHintKind::MaxLevel: Hint = PawText::Ui(TEXT("CATCHES_hint_max")); break;
			case Pc::PcCardHintKind::Requirement: Hint = PawText::Ui(CardRequirementKey(S.Hint.Requirement)); break;
			case Pc::PcCardHintKind::NotEnoughKittens: Hint = PawText::Ui(TEXT("CATCHES_hint_kittens")); break;
			default: Hint = PawText::Ui(TEXT("CATCHES_not_taken")); break;
			}
			Row->SetDetail(Hint, PawTokens::UI_TEXT_MUTED);
		}
		// Gold for what you have, grey for what you do not: the list is scanned far more than it is read.
		Row->SetLabelColour(true, S.bHeld ? PawTokens::UI_GOOD : PawTokens::UI_TEXT_MUTED);
	}
}

void UPawHudScreen::RefreshAchievements()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const Pc::PcAchievementsPage Page = Pc::AchievementsPage(D, G->Progress());
	for (int32 I = 0; I < ClaimButtons.Num() && I < static_cast<int32>(Page.Rows.size()); ++I)
	{
		const Pc::PcAchievementRow& S = Page.Rows[static_cast<size_t>(I)];
		const FText Name = PawText::Data(TEXT("ACH"), S.Id);
		FText Second;
		FLinearColor Colour = PawTokens::UI_TEXT_MUTED;
		switch (S.Kind)
		{
		case Pc::PcAchievementRowKind::Claimed:
		case Pc::PcAchievementRowKind::Earned:
			SetText(AchievementNames[I], Name);
			Second = PawText::DataField(TEXT("ACH"), S.Id, TEXT("desc"));
			Colour = S.Kind == Pc::PcAchievementRowKind::Claimed ? PawTokens::UI_TAB_UPGRADES : PawTokens::UI_GOOD;
			break;
		case Pc::PcAchievementRowKind::Secret:
			SetText(AchievementNames[I], PawText::Ui(TEXT("AWARDS_secret")));
			Colour = PawTokens::UI_TEXT_DISABLED;
			break;
		default:
		{
			SetText(AchievementNames[I], Name);
			FFormatNamedArguments Args;
			Args.Add(TEXT("value"), PawText::Amount(S.Value));
			Args.Add(TEXT("threshold"), PawText::Amount(S.Threshold));
			Second = PawText::Ui(TEXT("AWARDS_progress"), Args);
			break;
		}
		}
		SetText(AchievementDetails[I], Second);
		SetShown(AchievementDetails[I], !Second.IsEmpty());
		PawUi::SetTextColour(AchievementNames[I], Colour);
		PawUi::SetTextColour(AchievementDetails[I], Colour);
		// The mark: filled for claimed and earned, hollow for the rest (★ ◆ ☆).
		const bool bDone = S.Kind == Pc::PcAchievementRowKind::Claimed || S.Kind == Pc::PcAchievementRowKind::Earned;
		AchievementMarks[I]->SetBrush(PawUi::Rounded(bDone ? Colour : PawTokens::UI_PANEL, Colour, 2.0f, MarkSize * 0.5f));
		AchievementMarkColours[I] = Colour;
		// The amount is ON the button: "Claim" alone does not say whether it is worth the reach.
		UPawButton* Claim = ClaimButtons[I];
		SetShown(Claim, S.bClaimable, ESlateVisibility::Visible);
		if (S.bClaimable)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("amount"), PawText::Amount(S.Reward));
			Claim->SetLabel(PawText::Ui(TEXT("AWARDS_claim"), Args));
			Claim->SetIcon(PawUi::CurrencyIconName(TEXT("stardust")), ButtonIconPx); // the icon carries the unit
		}
	}
	FFormatNamedArguments Args;
	Args.Add(TEXT("earned"), PawText::Count(Page.Earned));
	Args.Add(TEXT("total"), PawText::Count(Page.Total));
	if (Page.Waiting > 0)
	{
		Args.Add(TEXT("n"), PawText::Count(Page.Waiting));
		Args.Add(TEXT("amount"), PawText::Amount(Page.WaitingStardust));
		SetText(AchievementSummaryLine, PawText::Ui(Page.Waiting == 1 ? TEXT("AWARDS_summary_waiting_one") : TEXT("AWARDS_summary_waiting"), Args));
		FFormatNamedArguments All;
		All.Add(TEXT("amount"), PawText::Amount(Page.WaitingStardust));
		ClaimAllButton->SetLabel(PawText::Ui(TEXT("AWARDS_claim_all"), All));
	}
	else
	{
		SetText(AchievementSummaryLine, PawText::Ui(TEXT("AWARDS_summary"), Args));
	}
	SetShown(ClaimAllButton, Page.Waiting > 0, ESlateVisibility::Visible);
}

void UPawHudScreen::RefreshJobs()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	const std::vector<Pc::PcJobRow> JobStates = Pc::JobRows(D, G->Progress());
	for (int32 I = 0; I < JobButtons.Num() && I < static_cast<int32>(JobStates.size()); ++I)
	{
		const Pc::PcJobRow& S = JobStates[static_cast<size_t>(I)];
		SetText(JobNames[I], PawText::Data(TEXT("JOB"), S.JobId));
		FText Second;
		FFormatNamedArguments Args;
		const TCHAR* State = TEXT("JOBS_locked");
		FLinearColor Colour = PawTokens::UI_TEXT_DISABLED;
		switch (S.Kind)
		{
		case Pc::PcJobRowKind::Worn:
		case Pc::PcJobRowKind::Owned:
			Second = PawText::DataField(TEXT("JOB"), S.JobId, TEXT("desc"));
			State = S.Kind == Pc::PcJobRowKind::Worn ? TEXT("JOBS_worn") : TEXT("JOBS_owned");
			Colour = S.Kind == Pc::PcJobRowKind::Worn ? PawTokens::UI_TEXT : PawTokens::UI_TEXT_MUTED;
			break;
		case Pc::PcJobRowKind::NoNode:
			Second = PawText::Ui(TEXT("JOBS_no_node"));
			break;
		case Pc::PcJobRowKind::Closed:
			Args.Add(TEXT("branch"), PawText::Data(TEXT("SKILL"), S.RivalBranch));
			Second = PawText::Ui(TEXT("JOBS_closed"), Args);
			break;
		case Pc::PcJobRowKind::BossGate:
			Args.Add(TEXT("biome"), PawText::Data(TEXT("TIER"), S.BossTier));
			Second = PawText::Ui(TEXT("JOBS_boss_gate"), Args);
			break;
		case Pc::PcJobRowKind::ReawakenGate:
			Args.Add(TEXT("n"), PawText::Count(S.Reawakens));
			Second = PawText::Ui(S.Reawakens == 1 ? TEXT("JOBS_reawaken_once") : TEXT("JOBS_reawaken"), Args);
			break;
		default:
			Args.Add(TEXT("stardust"), PawText::Count(static_cast<int64>(S.Cost)));
			Second = PawText::Ui(TEXT("JOBS_ready"), Args);
			break;
		}
		SetText(JobDetails[I], Second);
		PawUi::SetTextColour(JobNames[I], Colour);
		PawUi::SetTextColour(JobDetails[I], Colour);
		JobButtons[I]->SetLabel(PawText::Ui(State));
		// "You are wearing this" must not render in the same grey as "locked" (hud.gd's font_disabled_color note).
		const bool bOwned = S.Kind == Pc::PcJobRowKind::Worn || S.Kind == Pc::PcJobRowKind::Owned;
		JobButtons[I]->SetLabelColour(bOwned, PawTokens::UI_TEXT, true);
	}
}

void UPawHudScreen::ToggleAutoPick()
{
	if (UPawGameSubsystem* G = Game())
	{
		G->Slot().bAutoPickCard = !G->Slot().bAutoPickCard; // the realm reads it every frame
	}
	RefreshNow();
}

void UPawHudScreen::PressClaim(int32 Index)
{
	UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady() || Index < 0 || Index >= static_cast<int32>(G->Data().Achievements.size()))
	{
		return;
	}
	// Taking a reward moves Stardust, which the Reawaken page and the status row report: everything refreshes.
	Pc::ClaimAchievement(G->Data(), G->Progress(), G->Data().Achievements[static_cast<size_t>(Index)].Id);
	RefreshNow();
	if (IsActivated() && ClaimButtons.IsValidIndex(Index) && ClaimButtons[Index]->GetVisibility() == ESlateVisibility::Collapsed)
	{
		// The pressed button went away: the selection moves to what is now first in the column, never into nothing.
		if (UWidget* Target = NativeGetDesiredFocusTarget())
		{
			Target->SetFocus();
		}
	}
}

void UPawHudScreen::PressClaimAll()
{
	UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady())
	{
		return;
	}
	Pc::ClaimAllAchievements(G->Data(), G->Progress());
	RefreshNow();
	if (IsActivated())
	{
		if (UWidget* Target = NativeGetDesiredFocusTarget())
		{
			Target->SetFocus();
		}
	}
}

FText UPawHudScreen::GetCatchesSummaryText() const { return CatchesSummary ? CatchesSummary->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetAchievementSummaryText() const { return AchievementSummaryLine ? AchievementSummaryLine->GetText() : FText::GetEmpty(); }

FText UPawHudScreen::GetAchievementText(int32 Index, bool bDetail) const
{
	const TArray<TObjectPtr<UTextBlock>>& From = bDetail ? AchievementDetails : AchievementNames;
	if (!From.IsValidIndex(Index) || (bDetail && From[Index]->GetVisibility() == ESlateVisibility::Collapsed))
	{
		return FText::GetEmpty();
	}
	return From[Index]->GetText();
}

FLinearColor UPawHudScreen::GetAchievementMark(int32 Index) const
{
	return AchievementMarkColours.IsValidIndex(Index) ? AchievementMarkColours[Index] : FLinearColor::Transparent;
}

FText UPawHudScreen::GetJobText(int32 Index, bool bDetail) const
{
	const TArray<TObjectPtr<UTextBlock>>& From = bDetail ? JobDetails : JobNames;
	return From.IsValidIndex(Index) ? From[Index]->GetText() : FText::GetEmpty();
}

void UPawHudScreen::PressReawaken()
{
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady() || !Pc::CanReawaken(G->Data(), G->Progress()))
	{
		return;
	}
	// Two presses, because Reawakening throws away every base upgrade and there is no undo (skippable: the box).
	if (ConfirmReawaken(this) && !bReawakenArmed)
	{
		bReawakenArmed = true;
		RefreshNow();
		// THE SELECTION MOVES TO CANCEL: the confirm is the SAME button, so with the selection left on it, Space or pad A
		// would confirm a reset nobody looked at twice (docs/05 §3).
		Remembered[static_cast<int32>(PcDockTab::Reawaken)] = CancelButton;
		if (IsActivated())
		{
			CancelButton->SetFocus();
		}
		return;
	}
	bReawakenArmed = false;
	DoReawaken();
}

void UPawHudScreen::CancelReawaken()
{
	bReawakenArmed = false;
	Remembered[static_cast<int32>(PcDockTab::Reawaken)] = ReawakenButton;
	RefreshNow();
	if (IsActivated())
	{
		ReawakenButton->SetFocus();
	}
}

void UPawHudScreen::DoReawaken()
{
	UPawRealmSubsystem* R = Realm();
	UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcGameData& D = G->Data();
	Pc::PcProgress& P = G->Progress();
	// The payout is read BEFORE the reset: afterwards the grant is folded into the totals (and Second Helping defers half
	// of it), so the ceremony could not report it.
	const double Payout = Pc::PendingStardust(D, P);
	// From here until the run lands in its starting biome (TravelToStartingBiome): a save in between resumes at the spend
	// step (F29, designer 2026-11-04). Set before the reset, as hud.gd sets it before reawaken() (whose signal saves).
	P.bReawakenInProgress = true;
	if (!Pc::Reawaken(D, P).bDone)
	{
		P.bReawakenInProgress = false;
		return;
	}
	// TierController._on_reawakened: kittens, stats, the boss dismissed.
	R->OnReawakened();
	// Then the ceremony; its end opens the spend step (Pump), whose end asks for the job and travels to the starting biome
	// (the first, or Territory's: designer 2026-11-01).
	bSpendPending = false;
	if (Ceremony)
	{
		Ceremony->Play(Payout, P.ReawakenCount);
	}
	else
	{
		bSpendPending = true; // no ceremony: still the spend step, never skipped silently
		OpenSpendStepWhenFree();
	}
	RefreshNow();
}

void UPawHudScreen::ResumeReawakenIfPending()
{
	// hud.gd _resume_reawaken() (F29, designer 2026-11-04). A save written between a Reawaken and the run's arrival in its
	// starting biome loads with the flag set: the spend step opens, then the job picker and the travel follow as the
	// ceremony's would. The ceremony is not replayed. Godot checks once, when the realm scene opens; here every Pump, so a
	// load from the title is covered too, which is why it waits while any part of the Reawaken is on screen: the ceremony,
	// the spend step (Menu stack) and the job picker (Modal stack) all run with the flag set.
	const UPawGameSubsystem* G = Game();
	if (!G || !G->IsDataReady() || !G->Progress().bReawakenInProgress || bSpendPending)
	{
		return;
	}
	if ((Ceremony && Ceremony->IsPlaying()) || IsMenuUp() || (ModalStack.IsValid() && ModalStack->GetNumWidgets() > 0))
	{
		return;
	}
	bSpendPending = true;
	OpenSpendStepWhenFree();
}

void UPawHudScreen::OpenSpendStepWhenFree()
{
	// _open_skill_tree_for_spending(). With no Menu stack there is no spend step to wait for (Godot with no tree in the
	// scene): the relocation that would follow it happens now.
	if (!MenuStack.IsValid())
	{
		bSpendPending = false;
		ReturnToStartingBiome();
		return;
	}
	if (IsMenuUp())
	{
		return; // the next Pump, once the screen on it is gone
	}
	bSpendPending = false;
	if (!OpenSkillTree(true))
	{
		ReturnToStartingBiome();
	}
}

void UPawHudScreen::ReturnToStartingBiome()
{
	// ORDER MATTERS (hud.gd): ask which job, THEN go. The choice is recorded before the travel.
	UPawGameSubsystem* G = Game();
	UCommonActivatableWidgetStack* Stack = ModalStack.Get();
	if (G && G->IsDataReady() && Stack && Pc::ShouldAskForJob(G->Progress()))
	{
		JobPicker = Stack->AddWidget<UPawJobPicker>(PawUi::Resolve<UPawJobPicker>(), [this](UPawJobPicker& Picker)
		{
			Picker.OnChosen = [this]()
			{
				RefreshJobs();
				TravelToStartingBiome();
			};
		});
		if (JobPicker)
		{
			return;
		}
	}
	TravelToStartingBiome();
}

void UPawHudScreen::ShowVictory(double Seconds)
{
	UCommonActivatableWidgetStack* Stack = ModalStack.Get();
	if (!Stack || (Victory && Stack->GetWidgetList().Contains(Victory.Get())))
	{
		return;
	}
	Victory = Stack->AddWidget<UPawVictoryScreen>(PawUi::Resolve<UPawVictoryScreen>(), [Seconds](UPawVictoryScreen& Screen)
	{
		Screen.SetSeconds(Seconds);
	});
}

void UPawHudScreen::TravelToStartingBiome()
{
	UPawRealmSubsystem* R = Realm();
	UPawGameSubsystem* G = Game();
	if (!R || !G || !G->IsDataReady())
	{
		return;
	}
	// The Reawaken is over once the run is on its way (or already there): hud.gd clears the flag here too (F29).
	G->Progress().bReawakenInProgress = false;
	// The reset cleared the current biome, so this runs from inside it too, as Godot's _travel_to_starting_biome reloads it.
	const int64 Where = Pc::ReawakenTravelIndex(G->Data(), G->Progress());
	if (Where >= 0)
	{
		R->Travel(static_cast<int32>(Where));
	}
	RefreshNow();
}

// --- test views -----------------------------------------------------------------------------------------------------

UWidget* UPawHudScreen::GetStatusRow() const { return StatusRow; }
UWidget* UPawHudScreen::GetDock() const { return DockBox; }
UWidget* UPawHudScreen::GetBossPanel() const { return BossPanel; }
FText UPawHudScreen::GetStageText() const { return StageText ? StageText->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetReawakenProgressText() const { return ReawakenProgressLine ? ReawakenProgressLine->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetPayoutText() const { return PayoutText ? PayoutText->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetTotalText() const { return TotalLine ? TotalLine->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetMultiplierText() const { return MultiplierText ? MultiplierText->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetCatnipText() const { return CatnipText ? CatnipText->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetStardustText() const { return StardustText ? StardustText->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetZoneText() const { return ZoneText ? ZoneText->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetWaveText() const { return WaveLine ? WaveLine->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetBossText() const { return BossText ? BossText->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetBossTimerText() const { return BossTimer ? BossTimer->GetText() : FText::GetEmpty(); }
FText UPawHudScreen::GetXpText(bool bNext) const
{
	const UTextBlock* T = bNext ? XpNext.Get() : XpNow.Get();
	return T ? T->GetText() : FText::GetEmpty();
}
bool UPawHudScreen::IsZoneVisible() const { return ZonePlate && ZonePlate->GetVisibility() != ESlateVisibility::Collapsed; }
UWidget* UPawHudScreen::GetZonePlate() const { return ZonePlate; }
UWidget* UPawHudScreen::WaveShown() const
{
	// R4d: the wave line sits on WavePlate; an asset the plate was never grafted into shows WaveBox itself.
	UPanelWidget* Parent = WaveBox ? WaveBox->GetParent() : nullptr;
	return Parent && Parent->GetFName() == TEXT("WavePlate") ? static_cast<UWidget*>(Parent) : static_cast<UWidget*>(WaveBox.Get());
}

bool UPawHudScreen::IsWaveVisible() const
{
	const UWidget* Shown = WaveShown();
	return Shown && Shown->GetVisibility() != ESlateVisibility::Collapsed;
}
bool UPawHudScreen::IsBossPanelVisible() const { return BossPanel && BossPanel->GetVisibility() != ESlateVisibility::Collapsed; }
bool UPawHudScreen::IsBossTimerVisible() const { return BossTimer && BossTimer->GetVisibility() != ESlateVisibility::Collapsed; }

// --- the quick bar (hud.gd: _on_settings_icon_pressed, _on_skill_tree_button_pressed, _on_save_icon_pressed,
// _on_quit_icon_pressed) ------------------------------------------------------------------------------------------------

void UPawHudScreen::PressQuickSettings()
{
	if (IsMenuUp() || (ModalStack.IsValid() && ModalStack->GetNumWidgets() > 0))
	{
		return;
	}
	UPawSettingsDialog::Open(ModalStack.Get(), []() {});
}

void UPawHudScreen::PressQuickSkills()
{
	OpenSkillTree(false); // browse, as T
}

void UPawHudScreen::PressQuickSave()
{
	if (UPawSaveSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSaveSubsystem>() : nullptr)
	{
		S->SaveGame();
	}
	if (QuickStatus)
	{
		QuickStatus->SetText(PawText::Ui(TEXT("QUICK_saved")));
	}
}

void UPawHudScreen::PressQuickQuit()
{
	// Saves first (Godot's change_scene raises no close request), then the title, not the desktop.
	if (UPawSaveSubsystem* S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawSaveSubsystem>() : nullptr)
	{
		S->SaveGame();
		S->Flush();
	}
	if (OnQuitToTitle)
	{
		OnQuitToTitle();
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Pawpocalypse/Maps/L_Title")));
}

FText UPawHudScreen::GetQuickStatusText() const { return QuickStatus ? QuickStatus->GetText() : FText::GetEmpty(); }
