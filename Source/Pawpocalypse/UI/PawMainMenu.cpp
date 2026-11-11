#include "UI/PawMainMenu.h"

#include "Audio/PawAudioSubsystem.h"
#include "Input/PawInputMap.h"
#include "UI/PawSettingsDialog.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/DateTime.h"
#include "PcAchievements.h"
#include "PcSave.h"
#include "PcStats.h"
#include "PcTitle.h"
#include "Save/PawSaveSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawPrompts.h"
#include "UI/PawHudScreen.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "UI/PawGarden.h"

// main_menu.gd: where bugs go (the itch board; the Steam discussions once on Steam) and the game's Discord (an invite set
// never to expire). Links, not words: never translated.
const TCHAR* UPawMainMenu::BugReportUrl = TEXT("https://pugtatogames.itch.io/pawpocalypse/community");
const TCHAR* UPawMainMenu::DiscordUrl = TEXT("https://discord.gg/ETASJXDZgY");

namespace
{
	using namespace PawUi;

	const TCHAR* const RealmMap = TEXT("/Game/Pawpocalypse/Maps/L_Realm");

	bool Selected(const UWidget* W)
	{
		// HasAnyUserFocus is false for a Common button whose inner SCommonButton holds focus (KB U/08 §3.3).
		return W && W->GetVisibility() != ESlateVisibility::Collapsed && (W->HasAnyUserFocus() || W->HasFocusedDescendants());
	}

	bool Shown(const UWidget* W)
	{
		return W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden;
	}

	// A widget at a Godot anchor point with Godot's four offsets (left, top, right, bottom from that point).
	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* W, float AnchorX, float AnchorY, double Left, double Top, double Right,
		double Bottom)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(AnchorX, AnchorY));
		S->SetAlignment(FVector2D(0.0, 0.0));
		S->SetOffsets(FMargin(static_cast<float>(Left), static_cast<float>(Top), static_cast<float>(Right - Left),
			static_cast<float>(Bottom - Top)));
		return S;
	}

	// R4h: a room for a fixed-size piece: Width wide at Left (from the left edge, or from the right with AnchorX 1), from Top
	// down to Bottom above the screen's foot. The piece keeps its size while it fits and shrinks (never grows) when it does
	// not: at 150 % UI the screen is 720 tall and the menu and Boba's card no longer fit at their 1080p places.
	UScaleBox* Room(UWidgetTree* T, UCanvasPanel* Canvas, const TCHAR* Name, float AnchorX, double Left, double Width, double Top,
		double Bottom, EHorizontalAlignment H, EVerticalAlignment V, UWidget* Piece)
	{
		UScaleBox* Box = T->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), Name);
		Box->SetStretch(EStretch::ScaleToFit);
		Box->SetStretchDirection(EStretchDirection::DownOnly);
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(Box);
		S->SetAnchors(FAnchors(AnchorX, 0.0f, AnchorX, 1.0f));
		S->SetAlignment(FVector2D(0.0, 0.0));
		S->SetOffsets(FMargin(static_cast<float>(Left), static_cast<float>(Top), static_cast<float>(Width), static_cast<float>(Bottom)));
		if (UScaleBoxSlot* In = Cast<UScaleBoxSlot>(Box->AddChild(Piece)))
		{
			In->SetHorizontalAlignment(H);
			In->SetVerticalAlignment(V);
		}
		return Box;
	}

	UCanvasPanelSlot* Fill(UCanvasPanel* Canvas, UWidget* W)
	{
		UCanvasPanelSlot* S = Canvas->AddChildToCanvas(W);
		S->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		S->SetOffsets(FMargin(0.0f));
		return S;
	}

	// StyleBoxFlat_panel (the menu and the three windows): content margins 28 / 24; since R3's shapes the plum plate.
	FSlateBrush TitlePanelBrush()
	{
		return Chrome(EPawChrome::Window);
	}

	FMargin TitlePanelPadding()
	{
		return PlatePadding(FMargin(static_cast<float>(TitlePanelPadX), static_cast<float>(TitlePanelPadY)), EPawChrome::Window);
	}

	void SetEnabledLook(UPawButton* B, bool bEnabled)
	{
		if (!B)
		{
			return; // a part the designer removed from the Widget Blueprint: nothing to set
		}
		// Godot disables these (a door that cannot open): the disabled look, no press, and never selected (MenuFocus.wire
		// skips disabled rows).
		B->SetLocked(!bEnabled);
		B->SetIsEnabled(bEnabled);
	}
}

UPawPlateButtonStyle::UPawPlateButtonStyle()
{
	// R3's shapes: the theme's plum buttons (the pressed plate sinks onto its shadow), the content lifted off the shadow.
	NormalBase = Chrome(EPawChrome::ButtonNormal);
	NormalHovered = Chrome(EPawChrome::ButtonHover);
	NormalPressed = Chrome(EPawChrome::ButtonPressed);
	SelectedBase = NormalBase;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalPressed;
	Disabled = Chrome(EPawChrome::ButtonDisabled);
	const float Pad = static_cast<float>(ClusterPad);
	const float Half = 0.5f * static_cast<float>(ChromeSpec(EPawChrome::ButtonNormal).Shadow);
	ButtonPadding = FMargin(Pad, Pad - Half, Pad, Pad + Half);
	CustomPadding = FMargin(0.0f);
}

UPawButton* UPawMainMenu::MakeMenuButton(UVerticalBox* Box, int32 Index, const TCHAR* Key, double Height, double Px, bool bFirst)
{
	UPawButton* B = WidgetTree->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), PawUi::Numbered(TEXT("MenuButton"), Index));
	B->SetLabel(PawText::Ui(Key));
	B->SetLabelSize(Px);
	B->SetMinDimensions(0, static_cast<int32>(Height));
	UVerticalBoxSlot* S = Box->AddChildToVerticalBox(B);
	S->SetHorizontalAlignment(HAlign_Fill);
	S->SetPadding(FMargin(0.0f, bFirst ? 0.0f : static_cast<float>(R4MenuSeparation), 0.0f, 0.0f));
	return B;
}

UBorder* UPawMainMenu::MakeWindowLayer(UCanvasPanel* Root, FName Name, double Width, double Height, double ScrimAlpha,
	UVerticalBox*& OutBody)
{
	UWidgetTree* T = WidgetTree;
	// The Scrim: the sky tone, full screen; it eats every click (a modal layer).
	UBorder* Layer = T->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Layer->SetBrush(Rounded(PawTokens::SKY_BASE.CopyWithNewOpacity(static_cast<float>(ScrimAlpha)), PawTokens::INK, 0.0f, 0.0f));
	Layer->SetPadding(FMargin(0.0f));
	Layer->SetHorizontalAlignment(HAlign_Center);
	Layer->SetVerticalAlignment(VAlign_Center);
	Layer->SetVisibility(ESlateVisibility::Collapsed);
	Fill(Root, Layer);
	USizeBox* Size = T->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(static_cast<float>(Width));
	Size->SetHeightOverride(static_cast<float>(Height));
	Layer->SetContent(Size);
	UBorder* Window = T->ConstructWidget<UBorder>();
	Window->SetBrush(TitlePanelBrush());
	Window->SetPadding(TitlePanelPadding());
	Size->SetContent(Window);
	OutBody = T->ConstructWidget<UVerticalBox>();
	Window->SetContent(OutBody);
	return Layer;
}

bool UPawMainMenu::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree && PawUi::BuildsInCode(this))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		Canvas = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		WidgetTree->RootWidget = Canvas;

		// The floor: paper, as the realm's own floor; the garden (BiomeBackdrop "menu", phase 6) covers it.
		UBorder* Floor = T->ConstructWidget<UBorder>();
		Floor->SetBrush(Rounded(PawTokens::UI_PANEL, PawTokens::INK, 0.0f, 0.0f));
		Floor->SetVisibility(ESlateVisibility::Visible);
		Fill(Canvas, Floor);

		// The garden (main_menu.gd _build_garden): over the paper, under everything else.
		UPawGarden* Garden = T->ConstructWidget<UPawGarden>(PawUi::Resolve<UPawGarden>(), TEXT("Garden"));
		Fill(Canvas, Garden);

		// The name, anchored top centre (main_menu.tscn Title: 1000 x 100 from 90 down, centred both ways; never translated).
		UBorder* NameBox = T->ConstructWidget<UBorder>();
		NameBox->SetBrush(NoBrush());
		NameBox->SetHorizontalAlignment(HAlign_Center);
		NameBox->SetVerticalAlignment(VAlign_Center);
		NameBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		// R4c: Butter, an ink outline and a solid shadow, top left and tilted (the board "Main menu (plum)").
		NameText = Heavy(MakeText(T, R4NamePx, PawTokens::UI_SELECTED, true, TEXT("NameText")));
		{
			FSlateFontInfo Font = NameText->GetFont();
			Font.OutlineSettings.OutlineSize = static_cast<int32>(R4NameOutline);
			NameText->SetFont(Font);
			NameText->SetShadowOffset(FVector2D(0.0, R4NameShadow));
			SetTextColour(NameText, PawTokens::UI_SELECTED); // over the field, yet Butter (MakeText gives labels there Bone)
		}
		NameText->SetJustification(ETextJustify::Left);
		NameText->SetText(PawText::Ui(TEXT("TITLE_name")));
		NameBox->SetHorizontalAlignment(HAlign_Left);
		NameBox->SetContent(NameText);
		NameBox->SetRenderTransformPivot(FVector2D(0.0, 0.5));
		NameBox->SetRenderTransformAngle(static_cast<float>(R4NameAngle));
		// R4h: the logo and the menu share one column, in one room from the logo's top to above the hint: at 1080p exactly
		// where R4c put them; at 150 % UI both shrink together (a menu shrunk alone came out smaller than at 100 %).
		UVerticalBox* LeftColumn = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftColumn"));
		USizeBox* LogoSize = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LogoSize"));
		LogoSize->SetWidthOverride(static_cast<float>(R4NameWidth));
		LogoSize->SetHeightOverride(static_cast<float>(R4NameHeight));
		LogoSize->SetContent(NameBox);
		LeftColumn->AddChildToVerticalBox(LogoSize)->SetHorizontalAlignment(HAlign_Left);
		Room(T, Canvas, TEXT("LeftFit"), 0.0f, R4TitleMargin, R4NameWidth, R4NameTop, R4MenuBottom, HAlign_Left, VAlign_Top, LeftColumn);

		// The version, bottom right.
		VersionText = MakeText(T, LabelPx, PawTokens::INK, false, TEXT("VersionText")); // R4c: on the lawn: ink (lilac vanished there)
		VersionText->SetJustification(ETextJustify::Right);
		VersionText->SetText(PawText::Version());
		VersionText->SetVisibility(ESlateVisibility::HitTestInvisible);
		const double VersionBottom = TitleCornerMargin + ClusterButtonSize + R4VersionAbove; // R4c: above the cluster
		Place(Canvas, VersionText, 1.0f, 1.0f, -TitleCornerMargin - TitleVersionWidth, -VersionBottom - TitleVersionHeight,
			-TitleCornerMargin, -VersionBottom);

		// The menu: a paper panel 460 x 440 whose top sits 110 above the middle.
		USizeBox* PanelSize = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MenuSize"));
		PanelSize->SetWidthOverride(static_cast<float>(R4PanelWidth));
		PanelSize->SetHeightOverride(static_cast<float>(R4PanelHeight));
		// R4c: on the left under the logo (Godot: centred, its top 110 above the middle). R4h: its top where 1080p puts it
		// (the middle less 290), in a room down to the hint: at 150 % UI it shrinks to fit there.
		UVerticalBoxSlot* MenuSlot = LeftColumn->AddChildToVerticalBox(PanelSize);
		MenuSlot->SetHorizontalAlignment(HAlign_Left);
		MenuSlot->SetPadding(FMargin(0.0f, static_cast<float>(540.0 + R4PanelTop - R4NameTop - R4NameHeight), 0.0f, 0.0f));
		UBorder* Panel = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
		Panel->SetBrush(TitlePanelBrush());
		Panel->SetPadding(TitlePanelPadding());
		Panel->SetVisibility(ESlateVisibility::Visible);
		PanelSize->SetContent(Panel);
		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBody"));
		Panel->SetContent(Body);
		UVerticalBox* Menu = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuList"));
		Body->AddChildToVerticalBox(Menu);
		const TCHAR* Keys[] = { TEXT("TITLE_continue"), TEXT("TITLE_new_game"), TEXT("TITLE_load"), TEXT("TITLE_leaderboard"),
			TEXT("TITLE_settings"), TEXT("TITLE_credits"), TEXT("TITLE_exit") };
		for (int32 I = 0; I < 7; ++I)
		{
			// R4c: the board's sizes, the words on the left; Continue the call (Refresh: Butter while it can continue).
			MenuButtons.Add(MakeMenuButton(Menu, I, Keys[I], I == 0 ? R4ContinueHeight : R4MenuButtonHeight,
				I == 0 ? R4ContinuePx : R4MenuButtonPx, I == 0));
			MenuButtons.Last()->SetAlignLeft(true);
		}
		StatusText = MakeText(T, LabelPx, PawTokens::UI_TEXT_MUTED, false, TEXT("StatusText"));
		StatusText->SetJustification(ETextJustify::Left);
		StatusText->SetAutoWrapText(false);
		StatusText->SetWrapTextAt(static_cast<float>(R4PanelWidth - 2.0 * TitlePanelPadX));
		Body->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, static_cast<float>(TitleBodySeparation), 0.0f, 0.0f));

		// The in-game keys as prompts (controller plan 9; main_menu.gd _build_hint: a PromptLabel, prompts 24 px).
		HintPrompts = T->ConstructWidget<UPawPromptText>(PawUi::Resolve<UPawPromptText>(), TEXT("HintPrompts"));
		HintPrompts->IconPx = static_cast<int32>(TitleHintPromptPx);
		HintPrompts->TextPx = static_cast<float>(LabelPx);
		HintPrompts->TextColour = PawTokens::UI_TEXT; // R4c: on its plum plate (until then ink on the lawn, crossing the bushes)
		UBorder* HintPlate = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HintPlate"));
		HintPlate->SetBrush(Chrome(EPawChrome::PlateSmall));
		HintPlate->SetPadding(PlatePadding(FMargin(static_cast<float>(R4HintPadX), static_cast<float>(R4HintPadY)), EPawChrome::PlateSmall));
		HintPlate->SetVisibility(ESlateVisibility::HitTestInvisible);
		HintPlate->SetContent(HintPrompts);
		UCanvasPanelSlot* HintSlot = Place(Canvas, HintPlate, 0.0f, 1.0f, TitleCornerMargin, -TitleCornerMargin - TitleHintHeight,
			TitleCornerMargin + TitleHintWidth, -TitleCornerMargin);
		HintSlot->SetAutoSize(true);
		HintSlot->SetAlignment(FVector2D(0.0, 1.0));

		// Boba's badge, bottom left: the frame and the caption at Godot's size; the photo (and the cameo a click summons)
		// arrive with the art (phase 6).
		UBorder* Badge = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BobaCard"));
		Badge->SetBrush(Chrome(EPawChrome::Window)); // R3: the plum plate
		Badge->SetPadding(PlatePadding(FMargin(static_cast<float>(BobaBadgePad)), EPawChrome::Window));
		Badge->SetToolTipText(PawText::Ui(TEXT("TITLE_boba_tooltip")));
		Badge->SetVisibility(ESlateVisibility::Visible);
		// R4c: on the right, tilted the other way (the board's pinned card).
		Badge->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		Badge->SetRenderTransformAngle(static_cast<float>(R4BobaAngle));
		// R4h: its bottom where 1080p puts it (the middle + 40 + 360 = 140 above the foot), in a room from under the logo's row:
		// at 150 % UI it rises (still above the version) instead of leaving the screen.
		USizeBox* BadgeSize = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BobaSize"));
		BadgeSize->SetWidthOverride(static_cast<float>(R4BobaWidth));
		BadgeSize->SetHeightOverride(static_cast<float>(R4BobaHeight));
		BadgeSize->SetContent(Badge);
		Room(T, Canvas, TEXT("BobaFit"), 1.0f, -R4BobaRight - R4BobaWidth, R4BobaWidth, R4BobaRoomTop, R4BobaBottom, HAlign_Right,
			VAlign_Bottom, BadgeSize);
		UVerticalBox* BadgeColumn = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BobaColumn"));
		Badge->SetContent(BadgeColumn);
		USizeBox* Photo = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BobaPhoto"));
		Photo->SetWidthOverride(static_cast<float>(R4BobaPhoto));
		Photo->SetHeightOverride(static_cast<float>(R4BobaPhoto));
		BadgeColumn->AddChildToVerticalBox(Photo)->SetHorizontalAlignment(HAlign_Center);
		// R4c: the photo's well (the board's raised frame), so the card waits for the photo instead of showing a hole.
		UBorder* Well = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BobaPhotoWell"));
		Well->SetBrush(Rounded(PawTokens::UI_BUTTON, PawTokens::UI_PANEL_FRAME, static_cast<float>(ChromeEdge), 14.0f));
		Well->SetVisibility(ESlateVisibility::HitTestInvisible);
		Photo->SetContent(Well);
		for (const TCHAR* Key : { TEXT("TITLE_boba_name"), TEXT("TITLE_boba_role") })
		{
			// R4c: the name a Butter title, the role lilac.
			const bool bName = FCString::Strcmp(Key, TEXT("TITLE_boba_name")) == 0;
			// Unnamed on purpose: the asset holds them as TextBlock_0 / _1, and naming them renumbers every later unnamed text
			// of the code layout (.Widgets.Parity compares texts by name).
			UTextBlock* Line = MakeText(T, bName ? R4BobaNamePx : BobaCaptionPx, bName ? PawTokens::UI_SELECTED : PawTokens::UI_TEXT_MUTED);
			if (bName)
			{
				Heavy(Line);
			}
			Line->SetJustification(ETextJustify::Center);
			Line->SetText(PawText::Ui(Key));
			BadgeColumn->AddChildToVerticalBox(Line)->SetHorizontalAlignment(HAlign_Center);
		}

		// The cluster: Report a bug, Discord, Language, square plates right of the badge, bottom-aligned with it. Mouse only.
		UHorizontalBox* Cluster = T->ConstructWidget<UHorizontalBox>();
		// R4c: bottom right (Godot: right of the badge, bottom left).
		const double ClusterWidth = 3.0 * ClusterButtonSize + 2.0 * ClusterGap;
		Place(Canvas, Cluster, 1.0f, 1.0f, -TitleCornerMargin - ClusterWidth, -TitleCornerMargin - ClusterButtonSize,
			-TitleCornerMargin, -TitleCornerMargin);
		const TCHAR* ClusterKeys[] = { TEXT("TITLE_bug"), TEXT("TITLE_discord"), TEXT("TITLE_language") };
		for (int32 I = 0; I < 3; ++I)
		{
			UPawButton* B = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), PawUi::Numbered(TEXT("ClusterButton"), I));
			B->SetStyle(PawUi::Resolve(UPawPlateButtonStyle::StaticClass()));
			B->SetMinDimensions(static_cast<int32>(ClusterButtonSize), static_cast<int32>(ClusterButtonSize));
			B->SetToolTipText(PawText::Ui(ClusterKeys[I]));
			B->SetIsFocusable(false);
			USizeBox* Square = T->ConstructWidget<USizeBox>();
			Square->SetWidthOverride(static_cast<float>(ClusterButtonSize));
			Square->SetHeightOverride(static_cast<float>(ClusterButtonSize));
			Square->SetContent(B);
			Cluster->AddChildToHorizontalBox(Square)->SetPadding(FMargin(I == 0 ? 0.0f : static_cast<float>(ClusterGap), 0.0f, 0.0f, 0.0f));
			ClusterButtons.Add(B);
		}

		// --- the layers, in Godot's order (the confirm on top) ---
		UVerticalBox* LBody = nullptr;
		LeaderboardLayer = MakeWindowLayer(Canvas, TEXT("LeaderboardLayer"), TitleWindowWidth, TitleLeaderboardHeight, TitleScrimAlpha, LBody);
		UVerticalBox* CBody = nullptr;
		CreditsLayer = MakeWindowLayer(Canvas, TEXT("CreditsLayer"), TitleWindowWidth, TitleCreditsHeight, TitleScrimAlpha, CBody);
		const float WindowInner = static_cast<float>(TitleWindowWidth - 2.0 * TitlePanelPadX);
		auto BuildWindow = [&](UVerticalBox* WBody, const TCHAR* TitleKey, const TCHAR* Part, UTextBlock*& OutText, UPawButton*& OutClose)
		{
			UTextBlock* Head = MakeText(T, TitleWindowTitlePx, PawTokens::UI_TEXT);
			Head->SetJustification(ETextJustify::Center);
			Head->SetText(PawText::Ui(TitleKey));
			WBody->AddChildToVerticalBox(Head)->SetHorizontalAlignment(HAlign_Center);
			UScrollBox* Scroll = T->ConstructWidget<UScrollBox>();
			UVerticalBoxSlot* ScrollSlot = WBody->AddChildToVerticalBox(Scroll);
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ScrollSlot->SetPadding(FMargin(0.0f, static_cast<float>(TitleBodySeparation), 0.0f, 0.0f));
			OutText = MakeText(T, LabelPx, PawTokens::UI_TEXT, false, FName(FString(Part) + TEXT("Text")));
			OutText->SetAutoWrapText(false);
			OutText->SetWrapTextAt(WindowInner);
			Scroll->AddChild(OutText);
			OutClose = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), FName(FString(Part) + TEXT("Close")));
			OutClose->SetLabel(PawText::Ui(TEXT("TITLE_close")));
			UVerticalBoxSlot* CloseSlot = WBody->AddChildToVerticalBox(OutClose);
			CloseSlot->SetHorizontalAlignment(HAlign_Fill);
			CloseSlot->SetPadding(FMargin(0.0f, static_cast<float>(TitleBodySeparation), 0.0f, 0.0f));
		};
		UTextBlock* LText = nullptr;
		UPawButton* LClose = nullptr;
		BuildWindow(LBody, TEXT("TITLE_leaderboard_title"), TEXT("Leaderboard"), LText, LClose);
		LeaderboardText = LText;
		LeaderboardClose = LClose;
		UTextBlock* CText = nullptr;
		UPawButton* CClose = nullptr;
		BuildWindow(CBody, TEXT("TITLE_credits_title"), TEXT("Credits"), CText, CClose);
		CreditsText = CText;
		CreditsClose = CClose;
		// ONE CREDIT, AND NOTHING ELSE (designer, 2026-09-23).
		CreditsText->SetText(PawText::Ui(TEXT("TITLE_credits_text")));

		// The slot screen: a darker scrim, the title, five cards, Back.
		SlotsLayer = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotsLayer"));
		SlotsLayer->SetBrush(Rounded(PawTokens::SKY_BASE.CopyWithNewOpacity(static_cast<float>(TitleSlotScrimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		SlotsLayer->SetPadding(FMargin(0.0f));
		SlotsLayer->SetHorizontalAlignment(HAlign_Center);
		SlotsLayer->SetVerticalAlignment(VAlign_Center);
		SlotsLayer->SetVisibility(ESlateVisibility::Collapsed);
		Fill(Canvas, SlotsLayer);
		USizeBox* SlotBodySize = T->ConstructWidget<USizeBox>();
		SlotBodySize->SetWidthOverride(static_cast<float>(TitleSlotBodyWidth));
		SlotBodySize->SetHeightOverride(static_cast<float>(TitleSlotBodyHeight));
		SlotsLayer->SetContent(SlotBodySize);
		UVerticalBox* SlotBody = T->ConstructWidget<UVerticalBox>();
		SlotBodySize->SetContent(SlotBody);
		UTextBlock* SlotTitle = MakeText(T, TitleSlotTitlePx, PawTokens::UI_TEXT);
		SlotTitle->SetJustification(ETextJustify::Center);
		SlotTitle->SetText(PawText::Ui(TEXT("TITLE_slots_title")));
		SlotBody->AddChildToVerticalBox(SlotTitle)->SetHorizontalAlignment(HAlign_Center);
		UHorizontalBox* Row = T->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* RowSlot = SlotBody->AddChildToVerticalBox(Row);
		RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f, static_cast<float>(TitleSlotSeparation), 0.0f, 0.0f));
		for (int32 I = 0; I < SlotCount; ++I)
		{
			USizeBox* Column = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), PawUi::Numbered(TEXT("SlotColumn"), I));
			Column->SetWidthOverride(static_cast<float>(SlotCardWidth));
			Row->AddChildToHorizontalBox(Column)->SetPadding(FMargin(I == 0 ? 0.0f : static_cast<float>(TitleSlotSeparation), 0.0f, 0.0f, 0.0f));
			UVerticalBox* ColumnBox = T->ConstructWidget<UVerticalBox>();
			Column->SetContent(ColumnBox);
			// EVERY SLOT IS THE SAME SIZE, FULL OR EMPTY: the text and the Delete live inside the card, which clips.
			USizeBox* CardSize = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), PawUi::Numbered(TEXT("SlotSize"), I));
			CardSize->SetWidthOverride(static_cast<float>(SlotCardWidth));
			CardSize->SetHeightOverride(static_cast<float>(SlotCardHeight));
			CardSize->SetClipping(EWidgetClipping::ClipToBounds);
			ColumnBox->AddChildToVerticalBox(CardSize)->SetHorizontalAlignment(HAlign_Center);
			UOverlay* Card = T->ConstructWidget<UOverlay>();
			CardSize->SetContent(Card);
			UPawButton* Face = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), PawUi::Numbered(TEXT("SlotCard"), I));
			UOverlaySlot* FaceSlot = Card->AddChildToOverlay(Face);
			FaceSlot->SetHorizontalAlignment(HAlign_Fill);
			FaceSlot->SetVerticalAlignment(VAlign_Fill);
			// The summary INSIDE the card, above the Delete strip; nothing decorative may eat the click that picks it.
			UTextBlock* Info = MakeText(T, LabelPx, PawTokens::UI_TEXT, false, PawUi::Numbered(TEXT("SlotText"), I));
			Info->SetJustification(ETextJustify::Center);
			Info->SetAutoWrapText(false);
			Info->SetWrapTextAt(static_cast<float>(SlotCardWidth - 2.0 * SlotTextMargin));
			Info->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* InfoSlot = Card->AddChildToOverlay(Info);
			InfoSlot->SetHorizontalAlignment(HAlign_Center);
			InfoSlot->SetVerticalAlignment(VAlign_Center);
			const float M = static_cast<float>(SlotTextMargin);
			InfoSlot->SetPadding(FMargin(M, M, M, static_cast<float>(SlotDeleteHeight) + M));
			// Delete sits ON the card, at the bottom, and only once there is something to delete.
			UPawButton* Delete = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), PawUi::Numbered(TEXT("SlotDelete"), I));
			Delete->SetLabel(PawText::Ui(TEXT("TITLE_delete")));
			Delete->SetMinDimensions(0, static_cast<int32>(SlotDeleteButtonHeight));
			Delete->SetVisibility(ESlateVisibility::Collapsed);
			UOverlaySlot* DeleteSlot = Card->AddChildToOverlay(Delete);
			DeleteSlot->SetHorizontalAlignment(HAlign_Fill);
			DeleteSlot->SetVerticalAlignment(VAlign_Bottom);
			DeleteSlot->SetPadding(FMargin(M, 0.0f, M, static_cast<float>(SlotDeleteHeight - SlotDeleteButtonHeight)));
			// The slot NUMBER underneath, large.
			UTextBlock* Number = Heavy(MakeText(T, SlotNumberPx, PawTokens::UI_TEXT, false, PawUi::Numbered(TEXT("SlotNumber"), I)));
			Number->SetJustification(ETextJustify::Center);
			FFormatNamedArguments Args;
			Args.Add(TEXT("n"), PawText::Count(I + 1));
			Number->SetText(PawText::Ui(TEXT("TITLE_slot_number"), Args));
			ColumnBox->AddChildToVerticalBox(Number)->SetPadding(FMargin(0.0f, static_cast<float>(SlotColumnSeparation), 0.0f, 0.0f));
			SlotColumns.Add(Column);
			SlotSizes.Add(CardSize);
			SlotCards.Add(Face);
			SlotTexts.Add(Info);
			SlotDeletes.Add(Delete);
			SlotNumbers.Add(Number);
		}
		SlotsBack = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), TEXT("SlotsBack"));
		SlotsBack->SetLabel(PawText::Ui(TEXT("TITLE_back")));
		SlotsBack->SetMinDimensions(0, static_cast<int32>(TitleSlotBackHeight));
		UVerticalBoxSlot* BackSlot = SlotBody->AddChildToVerticalBox(SlotsBack);
		BackSlot->SetHorizontalAlignment(HAlign_Center);
		BackSlot->SetPadding(FMargin(0.0f, static_cast<float>(TitleSlotSeparation), 0.0f, 0.0f));

		// The shared confirm (Godot's ConfirmPanel), on top of everything.
		UVerticalBox* KBody = nullptr;
		ConfirmLayer = MakeWindowLayer(Canvas, TEXT("ConfirmLayer"), TitleConfirmWidth, TitleConfirmHeight, TitleScrimAlpha, KBody);
		ConfirmText = MakeText(T, LabelPx, PawTokens::UI_TEXT, false, TEXT("ConfirmText"));
		ConfirmText->SetJustification(ETextJustify::Center);
		ConfirmText->SetAutoWrapText(false);
		ConfirmText->SetWrapTextAt(static_cast<float>(TitleConfirmWidth - 2.0 * TitlePanelPadX));
		UVerticalBoxSlot* PromptSlot = KBody->AddChildToVerticalBox(ConfirmText);
		PromptSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetVerticalAlignment(VAlign_Center);
		UHorizontalBox* Answers = T->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* AnswersSlot = KBody->AddChildToVerticalBox(Answers);
		AnswersSlot->SetHorizontalAlignment(HAlign_Center);
		AnswersSlot->SetPadding(FMargin(0.0f, static_cast<float>(TitleConfirmSeparation), 0.0f, 0.0f));
		auto Answer = [&](const TCHAR* Key, const TCHAR* Name, bool bFirst)
		{
			UPawButton* B = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), Name);
			B->SetLabel(PawText::Ui(Key));
			B->SetMinDimensions(static_cast<int32>(TitleConfirmButtonWidth), static_cast<int32>(TitleConfirmButtonHeight));
			Answers->AddChildToHorizontalBox(B)->SetPadding(FMargin(bFirst ? 0.0f : static_cast<float>(TitleConfirmRowSeparation), 0.0f, 0.0f, 0.0f));
			return B;
		};
		ConfirmYes = Answer(TEXT("TITLE_yes"), TEXT("ConfirmYes"), true);
		ConfirmNo = Answer(TEXT("TITLE_cancel"), TEXT("ConfirmNo"), false);
	}
	const bool bOk = Super::Initialize();
	if (bOk && PawUi::ShouldWire(this))
	{
		OnLayoutReady();
	}
	return bOk;
}

void UPawMainMenu::OnLayoutReady()
{
	// The parts, in either mode (a Widget Blueprint bound the single ones; the repeated ones are found by name).
	PawUi::CollectNamed(WidgetTree, TEXT("MenuButton"), 7, MenuButtons);
	PawUi::CollectNamed(WidgetTree, TEXT("ClusterButton"), 3, ClusterButtons);
	// main_menu.gd CLUSTER_BUTTONS: the icon (Art.texture) fills the plate inside its 7 px margins; the name is the tooltip.
	const TCHAR* ClusterIcons[] = { TEXT("ui_menu_bug"), TEXT("ui_menu_discord"), TEXT("ui_menu_language") };
	for (int32 I = 0; I < ClusterButtons.Num(); ++I)
	{
		if (ClusterButtons[I])
		{
			ClusterButtons[I]->SetIcon(ClusterIcons[I], ClusterButtonSize - 2.0 * ClusterPad);
			Juice.Hover(ClusterButtons[I], this); // UiJuice.hover(btn)
		}
	}
	PawUi::CollectNamed(WidgetTree, TEXT("SlotCard"), SlotCount, SlotCards);
	PawUi::CollectNamed(WidgetTree, TEXT("SlotDelete"), SlotCount, SlotDeletes);
	PawUi::CollectNamed(WidgetTree, TEXT("SlotText"), SlotCount, SlotTexts);
	PawUi::CollectNamed(WidgetTree, TEXT("SlotNumber"), SlotCount, SlotNumbers);
	PawUi::CollectNamed(WidgetTree, TEXT("SlotSize"), SlotCount, SlotSizes);
	PawUi::CollectNamed(WidgetTree, TEXT("SlotColumn"), SlotCount, SlotColumns);
	SlotScale.Init(1.0f, SlotCount);
	auto Click = [this](UPawButton* B, TFunction<void()> Do)
	{
		if (B)
		{
			B->OnClicked().AddWeakLambda(this, MoveTemp(Do));
		}
	};
	Click(MenuButtons[0], [this]() { PressContinue(); });
	Click(MenuButtons[1], [this]() { PressNewGame(); });
	Click(MenuButtons[2], [this]() { PressLoad(); });
	Click(MenuButtons[3], [this]() { PressLeaderboard(); });
	Click(MenuButtons[4], [this]() { PressSettings(); });
	Click(MenuButtons[5], [this]() { PressCredits(); });
	Click(MenuButtons[6], [this]() { PressExit(); });
	for (int32 I = 0; I < 3; ++I)
	{
		Click(ClusterButtons[I], [this, I]() { PressCluster(I); });
	}
	for (int32 I = 0; I < SlotCount; ++I)
	{
		Click(SlotCards[I], [this, I]() { PressSlot(I); });
		Click(SlotDeletes[I], [this, I]() { PressDelete(I); });
		if (SlotNumbers[I])
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("n"), PawText::Count(I + 1));
			SlotNumbers[I]->SetText(PawText::Ui(TEXT("TITLE_slot_number"), Args));
		}
	}
	Click(LeaderboardClose, [this]() { PressClose(); });
	Click(CreditsClose, [this]() { PressClose(); });
	Click(SlotsBack, [this]() { PressSlotsBack(); });
	Click(ConfirmYes, [this]() { PressConfirmYes(); });
	Click(ConfirmNo, [this]() { PressConfirmNo(); });
	// The language list arrives with the languages (phase 7).
	if (ClusterButtons[2])
	{
		ClusterButtons[2]->SetIsEnabled(false);
	}
	// What is formatted, not baked: the version, the hint's template.
	if (VersionText)
	{
		VersionText->SetText(PawText::Version());
	}
	if (HintPrompts)
	{
		HintPrompts->SetTemplate(PawText::Ui(TEXT("TITLE_hint")));
	}
	if (!OnPlay)
	{
		OnPlay = [this]() { UGameplayStatics::OpenLevel(this, FName(RealmMap)); };
	}
	if (!OnOpenUrl)
	{
		OnOpenUrl = [](const FString& Url) { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); };
	}
	if (!OnQuit)
	{
		OnQuit = [this]() { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); };
	}
	if (!Now)
	{
		Now = []() { return static_cast<double>(FDateTime::UtcNow().ToUnixTimestamp()); };
	}
}

UPawGameSubsystem* UPawMainMenu::Game() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UPawGameSubsystem>() : nullptr;
}

UPawSaveSubsystem* UPawMainMenu::Saves() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UPawSaveSubsystem>() : nullptr;
}

TOptional<FUIInputConfig> UPawMainMenu::GetDesiredInputConfig() const
{
	// A menu: its buttons are Slate widgets, the cursor free, nothing captured.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UPawMainMenu::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (UPawAudioSubsystem* Audio = UPawAudioSubsystem::Of(this))
	{
		Audio->PlayMusic(TEXT("menu")); // main_menu.gd _ready
	}
	Refresh();
	FocusTop();
}

bool UPawMainMenu::CanTakeFocus() const
{
	// Focus belongs to a local player: a screen built without one (the automation world) has none to give.
	return IsActivated() && GetOwningLocalPlayer() != nullptr;
}

// --- reading the game -----------------------------------------------------------------------------------------------

void UPawMainMenu::Refresh()
{
	const UPawGameSubsystem* G = Game();
	UPawSaveSubsystem* S = Saves();
	const bool bOnDisk = S && S->HasAnySave();
	Pc::PcProgress Empty;
	// The last load's outcome reaches the player (review CR2; F47): a newer build's slot, an unreadable one, the backup.
	const Pc::PcTitleNotice Notice = S ? S->GetTitleNotice() : Pc::PcTitleNotice::None;
	const Pc::PcTitleMenu M = Pc::TitleMenu(bOnDisk, G && G->IsDataReady() ? G->Progress() : Empty, Notice);
	bContinueOpensSlots = M.bContinueOpensSlots;
	SetEnabledLook(GetMenuButton(0), M.bContinue);
	if (UPawButton* Continue = GetMenuButton(0))
	{
		Continue->SetCallLook(M.bContinue); // R4c: the board's Butter Continue, while there is something to continue
	}
	SetEnabledLook(GetMenuButton(2), M.bLoad);
	if (M.Notice != Pc::PcTitleNotice::None)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), PawText::Count(S->GetTitleNoticeSlot() + 1));
		StatusText->SetText(PawText::Ui(M.Notice == Pc::PcTitleNotice::NewerBuild ? TEXT("TITLE_status_newer")
			: M.Notice == Pc::PcTitleNotice::Unreadable ? TEXT("TITLE_status_unreadable") : TEXT("TITLE_status_backup"), Args));
	}
	else if (M.bSaveFound)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), PawText::Count(M.Reawakens));
		Args.Add(TEXT("mice"), PawText::Count(M.Kills));
		StatusText->SetText(PawText::Ui(M.Reawakens == 1 ? TEXT("TITLE_status_found_one") : TEXT("TITLE_status_found"), Args));
	}
	else
	{
		StatusText->SetText(PawText::Ui(TEXT("TITLE_status_none")));
	}
	if (IsLayerShown(ELayer::Slots))
	{
		RefreshSlotCards();
	}
}

void UPawMainMenu::RefreshSlotCards()
{
	UPawSaveSubsystem* S = Saves();
	const double Clock = Now ? Now() : 0.0;
	for (int32 I = 0; I < SlotCount; ++I)
	{
		Pc::PcSlotSummary Summary;
		const Pc::PcSlotState State = S ? S->SlotState(I, &Summary) : Pc::PcSlotState::Empty;
		const bool bRead = State == Pc::PcSlotState::Readable || State == Pc::PcSlotState::ReadableFromBackup;
		UTextBlock* Text = SlotTexts.IsValidIndex(I) ? SlotTexts[I].Get() : nullptr;
		if (UPawButton* Delete = GetSlotDelete(I))
		{
			Delete->SetVisibility(bRead ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
		if (!Text)
		{
			continue;
		}
		if (!bRead)
		{
			// F47 (D16): a file this build cannot read is never shown as an empty slot.
			Text->SetText(PawText::Ui(State == Pc::PcSlotState::NewerBuild ? TEXT("TITLE_slot_newer")
				: State == Pc::PcSlotState::Unreadable ? TEXT("TITLE_slot_unreadable") : TEXT("TITLE_slot_empty")));
			continue;
		}
		FFormatNamedArguments Args;
		Args.Add(TEXT("n"), PawText::Count(Summary.Reawakens));
		Args.Add(TEXT("time"), PawText::SlotClock(Summary.PlaySeconds));
		Args.Add(TEXT("mice"), PawText::Mice(Summary.Kills));
		Args.Add(TEXT("when"), PawText::When(Summary.LastPlayedUnix, Clock));
		Text->SetText(PawText::Ui(TEXT("TITLE_slot_info"), Args));
	}
}

void UPawMainMenu::RefreshLeaderboard()
{
	// Refreshed on open: final_boss_seconds can be stamped by a run that started from this very menu.
	const UPawGameSubsystem* G = Game();
	Pc::PcProgress Empty;
	const Pc::PcProgress& P = G && G->IsDataReady() ? G->Progress() : Empty;
	FText Last;
	switch (Pc::LeaderboardClear(P))
	{
	case Pc::PcClearForm::Time:
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("time"), PawText::Duration(P.FinalBossSeconds));
		Last = PawText::Ui(TEXT("TITLE_leaderboard_clear"), Args);
		break;
	}
	case Pc::PcClearForm::BeforeTheClock: Last = PawText::Ui(TEXT("TITLE_leaderboard_before_clock")); break;
	default: Last = PawText::Ui(TEXT("TITLE_leaderboard_not_yet")); break;
	}
	FFormatNamedArguments Args;
	Args.Add(TEXT("rank"), PawText::Ui(TEXT("TITLE_leaderboard_rank")));
	Args.Add(TEXT("offline"), PawText::Ui(TEXT("TITLE_leaderboard_offline")));
	Args.Add(TEXT("last"), Last);
	LeaderboardText->SetText(PawText::Ui(TEXT("TITLE_leaderboard_text"), Args));
}

// --- layers and the selection ----------------------------------------------------------------------------------------

void UPawMainMenu::Show(ELayer Layer, bool bShow)
{
	UBorder* W = Layer == ELayer::Slots ? SlotsLayer.Get() : Layer == ELayer::Leaderboard ? LeaderboardLayer.Get()
		: Layer == ELayer::Credits ? CreditsLayer.Get() : Layer == ELayer::Confirm ? ConfirmLayer.Get() : nullptr;
	if (W)
	{
		W->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

bool UPawMainMenu::IsLayerShown(ELayer Layer) const
{
	switch (Layer)
	{
	case ELayer::Slots: return Shown(SlotsLayer);
	case ELayer::Leaderboard: return Shown(LeaderboardLayer);
	case ELayer::Credits: return Shown(CreditsLayer);
	case ELayer::Confirm: return Shown(ConfirmLayer);
	default: return true;
	}
}

UPawMainMenu::ELayer UPawMainMenu::GetTopLayer() const
{
	// _focus_top()'s order: the confirm, the credits, the leaderboard, the slots, the menu.
	for (const ELayer L : { ELayer::Confirm, ELayer::Credits, ELayer::Leaderboard, ELayer::Slots })
	{
		if (IsLayerShown(L))
		{
			return L;
		}
	}
	return ELayer::Menu;
}

UWidget* UPawMainMenu::NativeGetDesiredFocusTarget() const
{
	switch (GetTopLayer())
	{
	case ELayer::Confirm: return ConfirmNo; // the safe answer is under the key: deleting a slot, or quitting
	case ELayer::Credits: return CreditsClose;
	case ELayer::Leaderboard: return LeaderboardClose;
	case ELayer::Slots:
	{
		const UPawSaveSubsystem* S = Saves();
		const int64 Last = Pc::SlotScreenFocus(S ? S->GetActiveSlot() : 0, SlotCount);
		return GetSlotCard(static_cast<int32>(Last));
	}
	default:
	{
		// The button that opened what was on top comes home; else Continue, else New Game.
		UWidget* Home = MenuReturn.Get();
		if (Home && Shown(Home) && Home->GetIsEnabled())
		{
			return Home;
		}
		UPawButton* Continue = GetMenuButton(0);
		return Continue && Continue->GetIsEnabled() ? Continue : GetMenuButton(1);
	}
	}
}

void UPawMainMenu::FocusTop()
{
	TArray<TArray<UWidget*>> Rows;
	switch (GetTopLayer())
	{
	case ELayer::Confirm: Rows.Add({ ConfirmYes.Get(), ConfirmNo.Get() }); break;
	case ELayer::Credits: Rows.Add({ CreditsClose.Get() }); break;
	case ELayer::Leaderboard: Rows.Add({ LeaderboardClose.Get() }); break;
	case ELayer::Slots:
	{
		TArray<UWidget*> Cards;
		TArray<UWidget*> Deletes;
		for (int32 I = 0; I < SlotCount; ++I)
		{
			Cards.Add(GetSlotCard(I));
			Deletes.Add(GetSlotDelete(I));
		}
		Rows = { Cards, Deletes, { SlotsBack.Get() } };
		break;
	}
	default:
		// Disabled rows are skipped: a title menu is a list of doors, and a door that cannot open is a dead stop for a pad.
		for (UPawButton* B : MenuButtons)
		{
			if (B->GetIsEnabled())
			{
				Rows.Add({ B });
			}
		}
		break;
	}
	Wired = WireRows(Rows);
	if (CanTakeFocus())
	{
		if (UWidget* Target = NativeGetDesiredFocusTarget())
		{
			Target->SetFocus();
		}
	}
}

void UPawMainMenu::RememberOpener()
{
	for (UPawButton* B : MenuButtons)
	{
		if (Selected(B))
		{
			MenuReturn = B;
			return;
		}
	}
}

// --- presses ---------------------------------------------------------------------------------------------------------

void UPawMainMenu::PressContinue()
{
	if (!GetMenuButton(0) || !GetMenuButton(0)->GetIsEnabled())
	{
		return;
	}
	if (bContinueOpensSlots)
	{
		// F47: the active slot cannot be read; memory holds a fresh game, never played over it unasked. The player chooses.
		OpenSlots();
		return;
	}
	if (UPawSaveSubsystem* S = Saves())
	{
		S->ClearTitleNotice(); // said once
	}
	if (OnPlay)
	{
		OnPlay();
	}
}

void UPawMainMenu::OpenSlots()
{
	// New Game and Load open the same picker (2026-08-20): it already tells "empty, start here" from "occupied, resume".
	RememberOpener();
	RefreshSlotCards();
	Show(ELayer::Slots, true);
	FocusTop();
}

void UPawMainMenu::PressNewGame() { OpenSlots(); }

void UPawMainMenu::PressLoad()
{
	if (GetMenuButton(2) && GetMenuButton(2)->GetIsEnabled())
	{
		OpenSlots();
	}
}

void UPawMainMenu::SetModalStack(UCommonActivatableWidgetStack* Stack)
{
	ModalStack = Stack;
}

void UPawMainMenu::PressSettings()
{
	// _on_settings(): the dialog over the title; when it closes the selection comes home to Settings (_focus_top).
	MenuReturn = GetMenuButton(4);
	UPawSettingsDialog::Open(ModalStack.Get(), [WeakThis = TWeakObjectPtr<UPawMainMenu>(this)]()
	{
		if (UPawMainMenu* Self = WeakThis.Get())
		{
			Self->FocusTop();
		}
	});
}

void UPawMainMenu::PressLeaderboard()
{
	RefreshLeaderboard();
	Show(ELayer::Leaderboard, true);
	FocusTop();
}

void UPawMainMenu::PressCredits()
{
	Show(ELayer::Credits, true);
	FocusTop();
}

void UPawMainMenu::PressExit()
{
	// Only worth saving if there is something to save; a fresh launch that goes straight to Exit writes no file.
	UPawGameSubsystem* G = Game();
	UPawSaveSubsystem* S = Saves();
	if (S && G && G->IsDataReady() && Pc::HasProgressToSave(G->Progress()))
	{
		S->SaveGame();
		S->Flush();
	}
	if (OnQuit)
	{
		OnQuit();
	}
}

void UPawMainMenu::PressSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= SlotCount)
	{
		return;
	}
	UPawGameSubsystem* G = Game();
	UPawSaveSubsystem* S = Saves();
	const Pc::PcSlotState State = S ? S->SlotState(SlotIndex) : Pc::PcSlotState::Empty;
	if (State == Pc::PcSlotState::NewerBuild)
	{
		// A newer build's save: never played here, never written over (CR2). The card says why; the slots stay up.
		return;
	}
	Show(ELayer::Slots, false);
	// The Jobs badge counts jobs SEEN, in a static that outlives the HUD; another game's count must not carry into this one.
	UPawHudScreen::ForgetJobsSeen();
	if (State == Pc::PcSlotState::Readable || State == Pc::PcSlotState::ReadableFromBackup)
	{
		// Occupied: it loads and resumes. No confirm: nothing is destroyed by loading. A load that fails after all (the
		// file changed since the card was drawn) plays nothing: the slots come back with the slot's real state (CR2).
		const EPawLoadOutcome Outcome = S->LoadGame(SlotIndex);
		if (Outcome != EPawLoadOutcome::Loaded && Outcome != EPawLoadOutcome::LoadedFromBackup)
		{
			RefreshSlotCards();
			Show(ELayer::Slots, true);
			FocusTop();
			return;
		}
		S->ClearTitleNotice();
	}
	else if (G && G->IsDataReady())
	{
		// Empty, or a file that cannot be read (F47: its card said so; the writer keeps the old file as slot_N_corrupt): a
		// fresh game starts there, with both automations off (UPawGameSubsystem::NewGame resets the slot's toggles) and no
		// achievements (AchievementManager.apply_save_data({})). Writing at once is what CREATES the slot, and doing it
		// this frame keeps the empty game out of the slot that was active (SaveGame reseeds the digest).
		G->NewGame();
		Pc::ApplyAchievementSave(G->Data(), G->Progress(), {}, {});
		if (S)
		{
			S->SaveGame(SlotIndex);
		}
	}
	if (OnPlay)
	{
		OnPlay();
	}
}

void UPawMainMenu::AskConfirm(const FText& Prompt, TFunction<void()> Then)
{
	ConfirmText->SetText(Prompt);
	// One-shot: a stale action would make the NEXT confirm run both.
	ConfirmThen = MoveTemp(Then);
	Show(ELayer::Confirm, true);
	FocusTop();
}

void UPawMainMenu::PressDelete(int32 SlotIndex)
{
	UPawSaveSubsystem* S = Saves();
	Pc::PcSlotSummary Summary;
	if (!S || !S->ReadSlotSummary(SlotIndex, Summary))
	{
		return;
	}
	FFormatNamedArguments Args;
	Args.Add(TEXT("slot"), PawText::Count(SlotIndex + 1));
	Args.Add(TEXT("n"), PawText::Count(Summary.Reawakens));
	Args.Add(TEXT("time"), PawText::SlotClock(Summary.PlaySeconds));
	AskConfirm(PawText::Ui(TEXT("TITLE_delete_prompt"), Args), [this, SlotIndex]()
	{
		if (UPawSaveSubsystem* Saved = Saves())
		{
			Saved->DeleteSlot(SlotIndex);
		}
		RefreshSlotCards();
	});
}

void UPawMainMenu::PressConfirmYes()
{
	Show(ELayer::Confirm, false);
	TFunction<void()> Then = MoveTemp(ConfirmThen);
	ConfirmThen = nullptr;
	if (Then)
	{
		Then();
	}
	FocusTop();
}

void UPawMainMenu::PressConfirmNo()
{
	Show(ELayer::Confirm, false);
	ConfirmThen = nullptr;
	FocusTop();
}

void UPawMainMenu::PressClose()
{
	Show(ELayer::Credits, false);
	Show(ELayer::Leaderboard, false);
	FocusTop();
}

void UPawMainMenu::PressSlotsBack()
{
	Show(ELayer::Slots, false);
	Refresh();
	FocusTop();
}

void UPawMainMenu::PressCluster(int32 Index)
{
	if (!GetClusterButton(Index) || !GetClusterButton(Index)->GetIsEnabled())
	{
		return;
	}
	UPawAudioSubsystem::PlayFrom(this, TEXT("ui_click")); // main_menu.gd _on_cluster_pressed
	if (Index == 0 && OnOpenUrl)
	{
		OnOpenUrl(BugReportUrl);
	}
	else if (Index == 1 && OnOpenUrl)
	{
		OnOpenUrl(DiscordUrl);
	}
}

void UPawMainMenu::Back()
{
	switch (GetTopLayer())
	{
	case ELayer::Confirm: PressConfirmNo(); return;
	case ELayer::Credits:
	case ELayer::Leaderboard: PressClose(); return;
	case ELayer::Slots: PressSlotsBack(); return;
	default:
		RememberOpener();
		AskConfirm(PawText::Ui(TEXT("TITLE_quit_prompt")), [this]() { PressExit(); });
		return;
	}
}

FReply UPawMainMenu::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	// main_menu.gd _unhandled_input: back, through the input map.
	const FKey K = KeyEvent.GetKey();
	if (PawInput::KeyIs(K, TEXT("back")))
	{
		if (!KeyEvent.IsRepeat())
		{
			Back();
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

// --- the slot cards' swell ---------------------------------------------------------------------------------------------

void UPawMainMenu::StepSwell(double DeltaSeconds)
{
	// HOVER MAKES ROOM (designer, 2026-09-11): the card and its column grow together, so the neighbours move rather than
	// the card overlapping them. The selected card swells too, so the keys and a pad see the same "this one".
	const float Rate = static_cast<float>((SlotHoverScale - 1.0) / SlotHoverSec);
	for (int32 I = 0; I < SlotCards.Num(); ++I)
	{
		if (!SlotCards[I] || !SlotScale.IsValidIndex(I))
		{
			continue;
		}
		const bool bUp = IsLayerShown(ELayer::Slots) && (SlotCards[I]->IsHovered() || Selected(SlotCards[I]));
		const float Target = bUp ? static_cast<float>(SlotHoverScale) : 1.0f;
		const float Next = FMath::FInterpConstantTo(SlotScale[I], Target, static_cast<float>(DeltaSeconds), Rate);
		if (Next != SlotScale[I])
		{
			SlotScale[I] = Next;
			if (SlotSizes.IsValidIndex(I) && SlotSizes[I])
			{
				SlotSizes[I]->SetWidthOverride(static_cast<float>(SlotCardWidth) * Next);
				SlotSizes[I]->SetHeightOverride(static_cast<float>(SlotCardHeight) * Next);
			}
			if (SlotColumns.IsValidIndex(I) && SlotColumns[I])
			{
				SlotColumns[I]->SetWidthOverride(static_cast<float>(SlotCardWidth) * Next);
			}
		}
	}
}

void UPawMainMenu::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	StepSwell(DeltaSeconds);
	Juice.Tick(DeltaSeconds);
}

// --- test views ----------------------------------------------------------------------------------------------------------

UPawButton* UPawMainMenu::GetMenuButton(int32 Index) const { return MenuButtons.IsValidIndex(Index) ? MenuButtons[Index].Get() : nullptr; }
UPawButton* UPawMainMenu::GetSlotCard(int32 SlotIndex) const { return SlotCards.IsValidIndex(SlotIndex) ? SlotCards[SlotIndex].Get() : nullptr; }
UPawButton* UPawMainMenu::GetSlotDelete(int32 SlotIndex) const { return SlotDeletes.IsValidIndex(SlotIndex) ? SlotDeletes[SlotIndex].Get() : nullptr; }
UPawButton* UPawMainMenu::GetClusterButton(int32 Index) const { return ClusterButtons.IsValidIndex(Index) ? ClusterButtons[Index].Get() : nullptr; }

UPawButton* UPawMainMenu::GetCloseButton(ELayer Layer) const
{
	return Layer == ELayer::Credits ? CreditsClose.Get() : Layer == ELayer::Leaderboard ? LeaderboardClose.Get() : nullptr;
}

FText UPawMainMenu::GetStatusText() const { return StatusText ? StatusText->GetText() : FText::GetEmpty(); }
FText UPawMainMenu::GetSlotText(int32 SlotIndex) const { return SlotTexts.IsValidIndex(SlotIndex) ? SlotTexts[SlotIndex]->GetText() : FText::GetEmpty(); }
FText UPawMainMenu::GetSlotNumberText(int32 SlotIndex) const { return SlotNumbers.IsValidIndex(SlotIndex) ? SlotNumbers[SlotIndex]->GetText() : FText::GetEmpty(); }
FText UPawMainMenu::GetLeaderboardText() const { return LeaderboardText ? LeaderboardText->GetText() : FText::GetEmpty(); }
FText UPawMainMenu::GetConfirmText() const { return ConfirmText ? ConfirmText->GetText() : FText::GetEmpty(); }
FText UPawMainMenu::GetVersionText() const { return VersionText ? VersionText->GetText() : FText::GetEmpty(); }
FString UPawMainMenu::GetHintText() const { return HintPrompts ? HintPrompts->GetNeutral() : FString(); }
FText UPawMainMenu::GetNameText() const { return NameText ? NameText->GetText() : FText::GetEmpty(); }
FText UPawMainMenu::GetCreditsText() const { return CreditsText ? CreditsText->GetText() : FText::GetEmpty(); }

float UPawMainMenu::GetSlotCardWidth(int32 SlotIndex) const
{
	return SlotSizes.IsValidIndex(SlotIndex) ? SlotSizes[SlotIndex]->GetWidthOverride() : 0.0f;
}
