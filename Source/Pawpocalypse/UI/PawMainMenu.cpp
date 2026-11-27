#include "UI/PawMainMenu.h"
#include "UI/PawUiTime.h"

#include "Audio/PawAudioSubsystem.h"
#include "Input/PawInputMap.h"
#include "UI/PawSettingsDialog.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
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
#include "Platform/PawPlatformSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/DateTime.h"
#include "PcAchievements.h"
#include "PcSave.h"
#include "PcStats.h"
#include "PcTitle.h"
#include "Save/PawSaveSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "Engine/Texture2D.h"
#include "UI/PawBoba.h"
#include "UI/PawButton.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "UI/PawIcons.h"
#include "UI/PawPrompts.h"
#include "UI/PawHudScreen.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"
#include "UI/PawGarden.h"

// Where bugs go: the game's Steam discussions (playtest 2026-11-26, note 5; the first try's itch board before), and the
// game's Discord (an invite set never to expire). Links, not words: never translated. Opened in the Steam overlay under
// Steam (UPawPlatformSubsystem::OpenWebPage), else the browser.
const TCHAR* UPawMainMenu::BugReportUrl = TEXT("https://steamcommunity.com/app/5720010/discussions/");
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

	// The player's preferences (the language among them); null outside a game instance (the bake, a bare test widget).
	UPawSettingsSubsystem* SettingsOf(const UUserWidget* W)
	{
		const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UPawSettingsSubsystem>() : nullptr;
	}

	// D18 (S4): the title's cast, the UI concept's (tools/art/sticker_art.py write_title).
	const TCHAR* const TitleCatIcon = TEXT("ui_icon_title_cat");
	const TCHAR* const TitleMouseIcon = TEXT("ui_icon_title_mouse");
	// D18 (S4): a save card's cat, one picture per stage (tools/art/sticker_art.py write_slots).
	FString SlotCatIcon(int64 Stage) { return FString::Printf(TEXT("ui_icon_slot_cat_%lld"), static_cast<long long>(Stage)); }

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
	Layer->SetBrush(Rounded(PawTokens::INK.CopyWithNewOpacity(static_cast<float>(ScrimAlpha)), PawTokens::INK, 0.0f, 0.0f));
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

		// D18 (S4): the concept's cast on the garden, under everything else: the Tiny Kitten and three mice, each on its
		// rim (tools/art/sticker_art.py write_title), placed by its feet around the screen's middle. Pictures, never a click.
		auto AddFigure = [&](FName Name, const TCHAR* Stem, double Px, double X, double Y, bool bFlip)
		{
			UImage* Figure = T->ConstructWidget<UImage>(UImage::StaticClass(), Name);
			Figure->SetBrush(PawUi::IconBrush(Stem, Px));
			Figure->SetVisibility(ESlateVisibility::HitTestInvisible);
			Figure->SetRenderTransformPivot(FVector2D(0.5, 0.5));
			Figure->SetRenderScale(FVector2D(bFlip ? -1.0 : 1.0, 1.0));
			Place(Canvas, Figure, 0.5f, 0.5f, X - 0.5 * Px, Y - Px, X + 0.5 * Px, Y);
		};
		AddFigure(TEXT("TitleCat"), TitleCatIcon, TitleCatPx, TitleCatX, TitleCatY, false);
		for (int32 I = 0; I < 3; ++I)
		{
			AddFigure(PawUi::Numbered(TEXT("TitleMouse"), I), TitleMouseIcon, TitleMousePx, TitleMouseX[I], TitleMouseY[I], TitleMouseFlip[I]);
		}

		// The name, anchored top centre (main_menu.tscn Title: 1000 x 100 from 90 down, centred both ways; never translated).
		UBorder* NameBox = T->ConstructWidget<UBorder>();
		NameBox->SetBrush(NoBrush());
		NameBox->SetHorizontalAlignment(HAlign_Center);
		NameBox->SetVerticalAlignment(VAlign_Center);
		NameBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		// R4c: Butter, an ink outline and a solid shadow. D18 (S4): centred at the top and upright, the concept's sticker.
		NameText = Heavy(MakeText(T, R4NamePx, PawTokens::UI_SELECTED, true, TEXT("NameText")));
		{
			FSlateFontInfo Font = NameText->GetFont();
			Font.OutlineSettings.OutlineSize = static_cast<int32>(R4NameOutline);
			NameText->SetFont(Font);
			NameText->SetShadowOffset(FVector2D(0.0, R4NameShadow));
			SetTextColour(NameText, PawTokens::UI_SELECTED); // over the field, yet Butter (MakeText gives labels there Bone)
		}
		NameText->SetJustification(ETextJustify::Center);
		NameText->SetText(PawText::Ui(TEXT("TITLE_name")));
		NameBox->SetContent(NameText);
		NameBox->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		NameBox->SetRenderTransformAngle(static_cast<float>(TitleLogoAngle));
		// D18 (S4): the logo on its own at the top centre; the menu keeps R4h's room on the left (at 150 % UI it shrinks).
		UVerticalBox* LeftColumn = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LeftColumn"));
		USizeBox* LogoSize = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LogoSize"));
		LogoSize->SetWidthOverride(static_cast<float>(R4NameWidth));
		LogoSize->SetHeightOverride(static_cast<float>(R4NameHeight));
		LogoSize->SetContent(NameBox);
		Place(Canvas, LogoSize, 0.5f, 0.0f, -0.5 * R4NameWidth, TitleLogoTop, 0.5 * R4NameWidth, TitleLogoTop + R4NameHeight);
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
		MenuSlot->SetPadding(FMargin(0.0f, static_cast<float>(540.0 + R4PanelTop - R4NameTop), 0.0f, 0.0f)); // D18: no logo above it
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
		StatusText = Heavy(MakeText(T, LabelPx, PawTokens::UI_TEXT, false, TEXT("StatusText"))); // D18 (S4): ink, heavy (the concept's)
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
		// P4.3: her photo in the well (the first of DA_Boba; set when the title opens), fitted inside it.
		UImage* BobaImage = T->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BobaPhotoImage"));
		BobaImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		// Centred by its UBorderSlot: UBorder::SetHorizontalAlignment after SetContent moves only the live SBorder, and the
		// slot (what the asset saves, what BuildSlot applies) keeps Fill: the photo stretched to the square well.
		UBorderSlot* BobaSlot = Cast<UBorderSlot>(Well->SetContent(BobaImage));
		if (BobaSlot)
		{
			BobaSlot->SetHorizontalAlignment(HAlign_Center);
			BobaSlot->SetVerticalAlignment(VAlign_Center);
		}
		for (const TCHAR* Key : { TEXT("TITLE_boba_name"), TEXT("TITLE_boba_role") })
		{
			// The name heavy ink, the role muted: on the card, with no outline (D18; R4c's Butter on plum).
			const bool bName = FCString::Strcmp(Key, TEXT("TITLE_boba_name")) == 0;
			// Unnamed on purpose: the asset holds them as TextBlock_0 / _1, and naming them renumbers every later unnamed text
			// of the code layout (.Widgets.Parity compares texts by name).
			UTextBlock* Line = MakeText(T, bName ? R4BobaNamePx : BobaCaptionPx, bName ? PawTokens::UI_TEXT : PawTokens::UI_TEXT_MUTED);
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
			OutText->SetWrapTextAt(WindowInner - static_cast<float>(TitleScrollBarRoom));
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
		SlotsLayer->SetBrush(Rounded(PawTokens::INK.CopyWithNewOpacity(static_cast<float>(TitleSlotScrimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		SlotsLayer->SetPadding(FMargin(0.0f));
		SlotsLayer->SetHorizontalAlignment(HAlign_Center);
		SlotsLayer->SetVerticalAlignment(VAlign_Center);
		SlotsLayer->SetVisibility(ESlateVisibility::Collapsed);
		Fill(Canvas, SlotsLayer);
		USizeBox* SlotBodySize = T->ConstructWidget<USizeBox>();
		SlotBodySize->SetWidthOverride(static_cast<float>(TitleSlotBodyWidth));
		SlotBodySize->SetHeightOverride(static_cast<float>(TitleSlotBodyHeight));
		// Shrunk to the screen when it is narrower than the row (1280 x 800 at 150 % UI); never grown.
		UScaleBox* SlotFit = T->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("SlotFit"));
		SlotFit->SetStretch(EStretch::ScaleToFit);
		SlotFit->SetStretchDirection(EStretchDirection::DownOnly);
		SlotsLayer->SetContent(SlotFit);
		SlotFit->SetContent(SlotBodySize);
		UVerticalBox* SlotBody = T->ConstructWidget<UVerticalBox>();
		SlotBodySize->SetContent(SlotBody);
		// D18 (S4): CHOOSE A SLOT as the concept's sticker title, yellow on its ink outline.
		UTextBlock* SlotTitle = ButterTitle(MakeText(T, TitleSlotTitlePx, PawTokens::UI_SELECTED), ButterTitleOutline);
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
			// D18 (S4, the concept's slotCard): under a photo well at the card's top, where the save's cat stands.
			const float M = static_cast<float>(SlotTextMargin);
			UVerticalBox* Stack = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), PawUi::Numbered(TEXT("SlotStack"), I));
			Stack->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* StackSlot = Card->AddChildToOverlay(Stack);
			StackSlot->SetHorizontalAlignment(HAlign_Fill);
			StackSlot->SetVerticalAlignment(VAlign_Top);
			StackSlot->SetPadding(FMargin(M, M, M, static_cast<float>(SlotDeleteHeight) + M));
			UBorder* SlotWell = T->ConstructWidget<UBorder>(UBorder::StaticClass(), PawUi::Numbered(TEXT("SlotPhoto"), I));
			SlotWell->SetBrush(Rounded(PawTokens::UI_PHOTO, PawTokens::INK, static_cast<float>(SlotPhotoEdgePx), static_cast<float>(SlotPhotoRadius)));
			SlotWell->SetPadding(FMargin(0.0f, static_cast<float>(SlotPhotoPadY)));
			SlotWell->SetHorizontalAlignment(HAlign_Center);
			SlotWell->SetVerticalAlignment(VAlign_Center);
			Stack->AddChildToVerticalBox(SlotWell)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, static_cast<float>(SlotPhotoGap)));
			UImage* Cat = T->ConstructWidget<UImage>(UImage::StaticClass(), PawUi::Numbered(TEXT("SlotCat"), I));
			Cat->SetBrush(PawUi::IconBrush(SlotCatIcon(1), SlotCatPx));
			SlotWell->SetContent(Cat);
			UTextBlock* Info = MakeText(T, LabelPx, PawTokens::UI_TEXT, false, PawUi::Numbered(TEXT("SlotText"), I));
			Info->SetJustification(ETextJustify::Center);
			Info->SetAutoWrapText(false);
			Info->SetWrapTextAt(static_cast<float>(SlotCardWidth - 2.0 * SlotTextMargin));
			Info->SetVisibility(ESlateVisibility::HitTestInvisible);
			Stack->AddChildToVerticalBox(Info)->SetHorizontalAlignment(HAlign_Center);
			// Delete sits ON the card, at the bottom, and only once there is something to delete.
			UPawButton* Delete = T->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>(), PawUi::Numbered(TEXT("SlotDelete"), I));
			Delete->SetLabel(PawText::Ui(TEXT("TITLE_delete")));
			Delete->SetMinDimensions(0, static_cast<int32>(SlotDeleteButtonHeight));
			Delete->SetVisibility(ESlateVisibility::Collapsed);
			UOverlaySlot* DeleteSlot = Card->AddChildToOverlay(Delete);
			DeleteSlot->SetHorizontalAlignment(HAlign_Fill);
			DeleteSlot->SetVerticalAlignment(VAlign_Bottom);
			DeleteSlot->SetPadding(FMargin(M, 0.0f, M, static_cast<float>(SlotDeleteHeight - SlotDeleteButtonHeight)));
			// The slot NUMBER: D18 (S4), inside the card at its top, as the concept draws it (Godot: underneath, large; on
			// the scrim the ink did not read).
			UTextBlock* Number = Heavy(MakeText(T, SlotNumberPx, PawTokens::UI_TEXT, false, PawUi::Numbered(TEXT("SlotNumber"), I)));
			Number->SetJustification(ETextJustify::Center);
			Number->SetVisibility(ESlateVisibility::HitTestInvisible);
			FFormatNamedArguments Args;
			Args.Add(TEXT("n"), PawText::Count(I + 1));
			Number->SetText(PawText::Ui(TEXT("TITLE_slot_number"), Args));
			UVerticalBoxSlot* NumberSlot = Stack->InsertChildAt(0, Number) ? Cast<UVerticalBoxSlot>(Number->Slot) : nullptr;
			if (NumberSlot)
			{
				NumberSlot->SetHorizontalAlignment(HAlign_Center);
				NumberSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, static_cast<float>(SlotNumberGap)));
			}
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

		// The language list (phase 7; main_menu.gd _open_language_menu): a clear scrim over everything, the list's card at its
		// foot right, above the Language button (the cluster's last); its rows are made on every open (FillLanguageMenu).
		LanguageLayer = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LanguageLayer"));
		LanguageLayer->SetBrush(Rounded(PawTokens::INK.CopyWithNewOpacity(0.0f), PawTokens::INK, 0.0f, 0.0f));
		LanguageLayer->SetPadding(FMargin(static_cast<float>(TitleCornerMargin), static_cast<float>(TitleCornerMargin),
			static_cast<float>(TitleCornerMargin), static_cast<float>(TitleCornerMargin + ClusterButtonSize + LanguageMenuGap)));
		LanguageLayer->SetHorizontalAlignment(HAlign_Right);
		LanguageLayer->SetVerticalAlignment(VAlign_Bottom);
		LanguageLayer->SetVisibility(ESlateVisibility::Collapsed);
		Fill(Canvas, LanguageLayer);
		LanguageMenu = T->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LanguageMenu"));
		LanguageMenu->SetBrush(Chrome(EPawChrome::Panel));
		LanguageMenu->SetPadding(PlatePadding(FMargin(static_cast<float>(LanguageMenuPad)), EPawChrome::Panel));
		LanguageLayer->SetContent(LanguageMenu);
		USizeBox* LanguageWidth = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LanguageWidth"));
		LanguageWidth->SetMinDesiredWidth(static_cast<float>(LanguageRowWidth));
		LanguageMenu->SetContent(LanguageWidth);
		UScrollBox* LanguageScroll = T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("LanguageScroll"));
		LanguageWidth->SetContent(LanguageScroll);
		LanguageList = T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LanguageList"));
		LanguageScroll->AddChild(LanguageList);
	}
	const bool bOk = Super::Initialize();
	// The screen itself takes the keys when a click lands on nothing focusable (its scrim, a plate): Slate gives focus to
	// the first focusable widget under the click, and without this that was the game's viewport, so Back / Esc never
	// reached NativeOnPreviewKeyDown (playtest 2026-11-26, note 11; KB U/24).
	SetIsFocusable(true);
	if (bOk && PawUi::ShouldWire(this))
	{
		PawUi::UseAtlasIcons(this); // the asset's saved icon brushes: the atlas's cells (one batch)
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
	PawUi::CollectNamed(WidgetTree, TEXT("SlotPhoto"), SlotCount, SlotPhotos);
	PawUi::CollectNamed(WidgetTree, TEXT("SlotCat"), SlotCount, SlotCats);
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
	Click(MenuButtons[6], [this]() { AskQuit(); }); // asks first, as Back does (playtest 2026-11-26, note 43)
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
	// _on_language_changed: the settings say when the language shown changes; this screen's words follow.
	if (UPawSettingsSubsystem* S = SettingsOf(this))
	{
		S->OnChanged.AddWeakLambda(this, [this]() { OnSettingsChanged(); });
		ShownCulture = S->GetLanguageApplied().Culture;
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
		OnOpenUrl = [](const FString& Url) { UPawPlatformSubsystem::OpenWebPage(Url); };
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
	if (UPawBobaSubsystem* Boba = UPawBobaSubsystem::Of(this))
	{
		Boba->EnterMenu(TEXT("main_menu")); // P4.3
		ShowBobaPhoto();
	}
}

void UPawMainMenu::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	if (UPawBobaSubsystem* Boba = UPawBobaSubsystem::Of(this))
	{
		Boba->LeaveMenu(TEXT("main_menu"));
	}
}

void UPawMainMenu::ShowBobaPhoto()
{
	UImage* Image = Cast<UImage>(WidgetTree ? WidgetTree->FindWidget(TEXT("BobaPhotoImage")) : nullptr);
	const UPawBobaSubsystem* Boba = UPawBobaSubsystem::Of(this);
	UTexture2D* Tex = Boba ? Boba->GetPhoto(0) : nullptr;
	const FVector2D Size = UPawBobaSubsystem::PhotoSize(Tex);
	if (!Image || !Tex || Size.X <= 0.0 || Size.Y <= 0.0)
	{
		return;
	}
	// Fitted inside the well, its edge inside the frame (the frame's ink and a little air).
	const double Room = PawUi::R4BobaPhoto - 16.0;
	const double K = Room / FMath::Max(Size.X, Size.Y);
	FSlateBrush Brush;
	Brush.SetResourceObject(Tex);
	Brush.ImageSize = Size * K;
	Image->SetBrush(Brush);
}

bool UPawMainMenu::PressBoba()
{
	UPawBobaSubsystem* Boba = UPawBobaSubsystem::Of(this);
	return Boba && Boba->Force();
}

UImage* UPawMainMenu::GetBobaPhoto() const
{
	return Cast<UImage>(WidgetTree ? WidgetTree->FindWidget(TEXT("BobaPhotoImage")) : nullptr);
}

UWidget* UPawMainMenu::GetBobaCard() const
{
	return WidgetTree ? WidgetTree->FindWidget(TEXT("BobaCard")) : nullptr;
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
		// D18 (S4): a save's cat at its stage on grass; an empty slot's well in card stock; one that cannot be read, off.
		const bool bEmpty = State == Pc::PcSlotState::Empty;
		if (UBorder* Photo = SlotPhotos.IsValidIndex(I) ? SlotPhotos[I].Get() : nullptr)
		{
			Photo->SetBrush(Rounded(bRead ? PawTokens::UI_PHOTO : bEmpty ? PawTokens::UI_PANEL_BAND : PawTokens::UI_BUTTON_DISABLED,
				bRead || bEmpty ? PawTokens::INK : PawTokens::UI_TEXT_DISABLED, static_cast<float>(SlotPhotoEdgePx),
				static_cast<float>(SlotPhotoRadius)));
		}
		if (UImage* Cat = SlotCats.IsValidIndex(I) ? SlotCats[I].Get() : nullptr)
		{
			const UPawGameSubsystem* G = Game();
			const int64 Stage = bRead && G && G->IsDataReady()
				? Pc::EvolutionStageFor(G->Data(), Summary.BossesDefeated, Summary.bFinalBossDefeated) : 0;
			// The well keeps its size empty: the picture is hidden, not collapsed.
			Cat->SetBrush(PawUi::IconBrush(SlotCatIcon(FMath::Max<int64>(Stage, 1)), SlotCatPx));
			Cat->SetVisibility(Stage > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
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
		: Layer == ELayer::Credits ? CreditsLayer.Get() : Layer == ELayer::Confirm ? ConfirmLayer.Get()
		: Layer == ELayer::Language ? LanguageLayer.Get() : nullptr;
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
	case ELayer::Language: return Shown(LanguageLayer);
	default: return true;
	}
}

UPawMainMenu::ELayer UPawMainMenu::GetTopLayer() const
{
	// _focus_top()'s order: the confirm, the credits, the leaderboard, the language list, the slots, the menu.
	for (const ELayer L : { ELayer::Confirm, ELayer::Credits, ELayer::Leaderboard, ELayer::Language, ELayer::Slots })
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
	case ELayer::Language:
	{
		// The language shown (its row's "selected" mark), else the first.
		const UPawSettingsSubsystem* S = SettingsOf(this);
		const int32 Current = S ? LanguageCodes.IndexOfByKey(S->GetLanguageApplied().Code) : INDEX_NONE;
		return LanguageRows.IsValidIndex(Current) ? LanguageRows[Current].Get() : LanguageRows.Num() > 0 ? LanguageRows[0].Get() : nullptr;
	}
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
	case ELayer::Language:
		for (UPawButton* Row : LanguageRows)
		{
			Rows.Add({ Row });
		}
		break;
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
	// The Jobs badge's count of jobs SEEN starts again with the game this press starts or loads (UPawUiMemory, review CR14:
	// HUD._jobs_seen = -1 in main_menu.gd _on_slot_chosen).
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
	else if (Index == 2)
	{
		// _toggle_language_menu()
		if (IsLayerShown(ELayer::Language))
		{
			CloseLanguageMenu();
		}
		else
		{
			OpenLanguageMenu();
		}
	}
}

// --- the language list ---------------------------------------------------------------------------------------------------

void UPawMainMenu::OpenLanguageMenu()
{
	FillLanguageMenu();
	Show(ELayer::Language, true);
	Juice.PopIn(LanguageMenu); // UiJuice.pop_in(language_menu)
	FocusTop();
}

void UPawMainMenu::CloseLanguageMenu()
{
	if (!IsLayerShown(ELayer::Language))
	{
		return;
	}
	Show(ELayer::Language, false);
	FocusTop();
}

void UPawMainMenu::FillLanguageMenu()
{
	if (!LanguageList || !WidgetTree)
	{
		return; // a part the designer removed from the Widget Blueprint
	}
	LanguageList->ClearChildren();
	LanguageRows.Reset();
	LanguageCodes = PawText::Languages();
	const UPawSettingsSubsystem* S = SettingsOf(this);
	const FString Current = S ? S->GetLanguageApplied().Code : FString(TEXT("en"));
	for (int32 I = 0; I < LanguageCodes.Num(); ++I)
	{
		const FString Code = LanguageCodes[I];
		// Unnamed: rebuilt on every open, and a name would still be held by the last open's row until it is collected.
		UPawButton* Row = WidgetTree->ConstructWidget<UPawButton>(PawUi::Resolve<UPawButton>());
		Row->Index = I;
		Row->SetMinDimensions(0, static_cast<int32>(LanguageRowHeight));
		Row->SetAlignLeft(true);
		Row->SetLabelSize(R4RowPx);
		Row->SetLabel(PawText::LanguageLabel(Code)); // its own name, never translated
		// The language shown: the yellow plate (the concept's "you are here") and the tick; the names line up either way.
		const bool bCurrent = Code == Current;
		Row->SetIconSlotKept(true);
		Row->SetIcon(bCurrent ? FString(TEXT("ui_tick")) : FString(), LanguageTickPx);
		Row->SetActiveLook(bCurrent);
		Row->OnClicked().AddWeakLambda(this, [this, Code]() { PressLanguage(Code); });
		UVerticalBoxSlot* RowSlot = LanguageList->AddChildToVerticalBox(Row);
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetPadding(FMargin(0.0f, I == 0 ? 0.0f : static_cast<float>(LanguageRowSeparation), 0.0f, 0.0f));
		LanguageRows.Add(Row);
	}
}

void UPawMainMenu::PressLanguage(const FString& Code)
{
	// _on_language_chosen(): saved and shown at once (UPawSettingsSubsystem::SetLanguage; this screen hears it), then closed.
	UPawAudioSubsystem::PlayFrom(this, TEXT("ui_click"));
	if (UPawSettingsSubsystem* S = SettingsOf(this))
	{
		S->SetLanguage(Code);
	}
	CloseLanguageMenu();
}

void UPawMainMenu::OnSettingsChanged()
{
	// _on_language_changed(): only a new language makes the words again (every other setting leaves this screen alone).
	const UPawSettingsSubsystem* S = SettingsOf(this);
	if (!S || S->GetLanguageApplied().Culture == ShownCulture)
	{
		return;
	}
	ShownCulture = S->GetLanguageApplied().Culture;
	// The string-table texts follow the culture by themselves; what was made from them once is made again.
	if (HintPrompts)
	{
		HintPrompts->SetTemplate(PawText::Ui(TEXT("TITLE_hint")));
	}
	Refresh();
	if (IsLayerShown(ELayer::Language))
	{
		FillLanguageMenu();
		FocusTop();
	}
}

void UPawMainMenu::Back()
{
	switch (GetTopLayer())
	{
	case ELayer::Confirm: PressConfirmNo(); return;
	case ELayer::Language: CloseLanguageMenu(); return;
	case ELayer::Credits:
	case ELayer::Leaderboard: PressClose(); return;
	case ELayer::Slots: PressSlotsBack(); return;
	default:
		AskQuit();
		return;
	}
}

void UPawMainMenu::AskQuit()
{
	// "Quit Pawpocalypse?": Exit and Back on the menu both ask (playtest 2026-11-26, note 43: "there should be are you sure you
	// want to exit type prompts when quitting"; Exit quit at once until then). Yes saves first (PressExit).
	RememberOpener();
	AskConfirm(PawText::Ui(TEXT("TITLE_quit_prompt")), [this]() { PressExit(); });
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

FReply UPawMainMenu::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	// Godot's LanguageScrim: a press that reaches the screen while the list is up (no row took it) closes the list, unless it
	// landed on the list's own card (its edge, its scroll bar).
	if (PressOffLanguageMenu(MouseEvent.GetScreenSpacePosition()))
	{
		return FReply::Handled();
	}
	// P4.3: a click on Boba's card brings her over (the card is a plate, not a button: mouse only, as the cluster is).
	if (const UWidget* Card = GetBobaCard(); Card && !IsLayerShown(ELayer::Slots) && !IsLayerShown(ELayer::Leaderboard) && !IsLayerShown(ELayer::Credits)
		&& !IsLayerShown(ELayer::Confirm) && Card->GetCachedGeometry().IsUnderLocation(MouseEvent.GetScreenSpacePosition()))
	{
		PressBoba();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(Geometry, MouseEvent);
}

bool UPawMainMenu::PressOffLanguageMenu(const FVector2D& ScreenPosition)
{
	if (!IsLayerShown(ELayer::Language))
	{
		return false;
	}
	if (!LanguageMenu || !LanguageMenu->GetCachedGeometry().IsUnderLocation(ScreenPosition))
	{
		CloseLanguageMenu();
	}
	return true;
}

// --- the slot cards' swell ---------------------------------------------------------------------------------------------

void UPawMainMenu::StepSwell(double DeltaSeconds)
{
	// HOVER MAKES ROOM (designer, 2026-09-11): the card and its column grow together, so the neighbours move rather than
	// the card overlapping them. The selected card swells too, so the keys and a pad see the same "this one" -- but only
	// as the ring shows it (UPawButton::RingRule): the focus the slots open with stays on slot 1 under a mouse, and it kept
	// slot 1 large while the pointer swelled another (playtest 2026-11-26, note 6).
	const float Rate = static_cast<float>((SlotHoverScale - 1.0) / SlotHoverSec);
	const UGameInstance* GI = GetGameInstance();
	const UPawInputDeviceSubsystem* Device = GI ? GI->GetSubsystem<UPawInputDeviceSubsystem>() : nullptr;
	const bool bPointer = Device && Device->LastWasPointer();
	for (int32 I = 0; I < SlotCards.Num(); ++I)
	{
		if (!SlotCards[I] || !SlotScale.IsValidIndex(I))
		{
			continue;
		}
		const bool bUp = IsLayerShown(ELayer::Slots) && SlotSwells(SlotCards[I]->IsHovered(), Selected(SlotCards[I]), bPointer);
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

bool UPawMainMenu::SlotSwells(bool bHovered, bool bSelected, bool bLastWasPointer)
{
	return UPawButton::RingRule(bHovered, bSelected, bLastWasPointer);
}

void UPawMainMenu::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	DeltaSeconds = PawUi::StepDelta(this, DeltaSeconds); // under a fixed step, that step once a frame (PawUiTime.h)
	StepSwell(DeltaSeconds);
	Juice.Tick(DeltaSeconds);
	// P4.3: the card's hover pop (the mouse over it, the effects on).
	if (UWidget* Card = GetBobaCard())
	{
		const float Want = Card->IsHovered() && FPawJuice::EffectsOn(this) ? static_cast<float>(Pc::JuiceHoverScale) : 1.0f;
		if (!FMath::IsNearlyEqual(Card->GetRenderTransform().Scale.X, Want))
		{
			Card->SetRenderScale(FVector2D(Want, Want));
		}
	}
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

FString UPawMainMenu::GetSlotCatName(int32 SlotIndex) const
{
	const UImage* Cat = SlotCats.IsValidIndex(SlotIndex) ? SlotCats[SlotIndex].Get() : nullptr;
	const UObject* Tex = Cat && Cat->GetVisibility() == ESlateVisibility::HitTestInvisible ? Cat->GetBrush().GetResourceObject() : nullptr;
	return Tex ? Tex->GetName() : FString();
}

FLinearColor UPawMainMenu::GetSlotPhotoColour(int32 SlotIndex) const
{
	const UBorder* Photo = SlotPhotos.IsValidIndex(SlotIndex) ? SlotPhotos[SlotIndex].Get() : nullptr;
	return Photo ? Photo->Background.TintColor.GetSpecifiedColor() : FLinearColor::Transparent;
}
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
