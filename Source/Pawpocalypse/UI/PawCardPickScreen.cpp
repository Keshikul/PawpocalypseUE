#include "UI/PawCardPickScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/World.h"
#include "PcData.h"
#include "PcHud.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"

const FName UPawCardPickScreen::PauseHolder(TEXT("card_pick"));

namespace
{
	using namespace PawUi;

	// The text inside a card is given a fixed wrap width (KB U/09 §3): the card's width less its margins.
	constexpr float BandWrap = static_cast<float>(CardWidth - 2.0 * CardBandPadX - 2.0 * CardFramePx);
	constexpr float BodyWrap = static_cast<float>(CardWidth - 2.0 * CardInnerPadX - 2.0 * CardFramePx);
	constexpr float SubtitleWrap = 900.0f;

	// FLAVOUR_COLOURS: a hand of three reads as three KINDS of thing before any text is read (Stacklands' card families).
	FLinearColor FlavourColour(Pc::CardFlavour F)
	{
		switch (F)
		{
		case Pc::CardFlavour::AURA: return PawTokens::UI_CARD_AURA;
		case Pc::CardFlavour::KITTEN: return PawTokens::UI_TAB_UPGRADES;
		case Pc::CardFlavour::POUNCE: return PawTokens::UI_CARD_POUNCE;
		case Pc::CardFlavour::SWARM: return PawTokens::UI_CARD_SWARM;
		case Pc::CardFlavour::ZONE: return PawTokens::SLATE_HEADER;
		case Pc::CardFlavour::FORTUNE: return PawTokens::UI_CARD_DECK;
		case Pc::CardFlavour::DECK: return PawTokens::GREY_HEADER;
		default: return PawTokens::EYE; // Godot's Color.WHITE fallback
		}
	}

	// FLAVOUR_NAMES
	const TCHAR* FlavourKey(Pc::CardFlavour F)
	{
		switch (F)
		{
		case Pc::CardFlavour::AURA: return TEXT("PICK_flavour_aura");
		case Pc::CardFlavour::KITTEN: return TEXT("PICK_flavour_kitten");
		case Pc::CardFlavour::POUNCE: return TEXT("PICK_flavour_pounce");
		case Pc::CardFlavour::SWARM: return TEXT("PICK_flavour_swarm");
		case Pc::CardFlavour::ZONE: return TEXT("PICK_flavour_zone");
		case Pc::CardFlavour::FORTUNE: return TEXT("PICK_flavour_fortune");
		default: return TEXT("PICK_flavour_deck");
		}
	}

	bool Selected(const UWidget* W)
	{
		// HasAnyUserFocus is false for a Common button whose inner SCommonButton holds focus (KB U/08 §3.3).
		return W && W->GetVisibility() != ESlateVisibility::Collapsed && (W->HasAnyUserFocus() || W->HasFocusedDescendants());
	}

	UTextBlock* CardText(UWidgetTree* T, double Px, const FLinearColor& Colour, float Wrap)
	{
		UTextBlock* Text = MakeText(T, Px, Colour);
		Text->SetAutoWrapText(false);
		Text->SetWrapTextAt(Wrap);
		return Text;
	}
}

// --- the card --------------------------------------------------------------------------------------------------------

UPawCardFaceStyle::UPawCardFaceStyle()
{
	NormalBase = NoBrush();
	NormalHovered = NoBrush();
	NormalPressed = NoBrush();
	SelectedBase = NoBrush();
	SelectedHovered = NoBrush();
	SelectedPressed = NoBrush();
	Disabled = NoBrush();
	ButtonPadding = FMargin(0.0f);
	CustomPadding = FMargin(0.0f);
}

UPawCardFace::UPawCardFace(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetStyle(UPawCardFaceStyle::StaticClass());
}

bool UPawCardFace::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		// Content before Super::Initialize: the Common button wraps it in its own button (docs/05 §2 rule 9).
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		USizeBox* Size = T->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(static_cast<float>(CardWidth));
		Size->SetMinDesiredHeight(static_cast<float>(CardHeight));
		WidgetTree->RootWidget = Size;

		// A STACKLANDS CARD: cream plate, a 3 px ink frame (the alert red when it costs you something), the flavour on a
		// coloured band across the top. The plate never takes the click itself: the button around it does.
		Plate = T->ConstructWidget<UBorder>();
		Plate->SetPadding(FMargin(static_cast<float>(CardFramePx), static_cast<float>(CardFramePx), static_cast<float>(CardFramePx),
			static_cast<float>(CardBottomPad)));
		Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Size->SetContent(Plate);
		UVerticalBox* Box = T->ConstructWidget<UVerticalBox>();
		Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Plate->SetContent(Box);

		// The band, ink-ruled underneath like a Stacklands card's name strip.
		Band = T->ConstructWidget<UBorder>();
		Band->SetPadding(FMargin(static_cast<float>(CardBandPadX), static_cast<float>(CardBandPadY)));
		Band->SetVisibility(ESlateVisibility::HitTestInvisible);
		Box->AddChildToVerticalBox(Band);
		UVerticalBox* BandBox = T->ConstructWidget<UVerticalBox>();
		Band->SetContent(BandBox);
		FlavourText = CardText(T, CardFlavourPx, PawTokens::INK, BandWrap);
		BandBox->AddChildToVerticalBox(FlavourText);
		NameText = CardText(T, CardNamePx, PawTokens::INK, BandWrap);
		BandBox->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		UBorder* Rule = T->ConstructWidget<UBorder>();
		Rule->SetBrush(Rounded(PawTokens::INK, PawTokens::INK, 0.0f, 0.0f));
		Rule->SetPadding(FMargin(0.0f, static_cast<float>(CardFramePx) * 0.5f));
		Rule->SetVisibility(ESlateVisibility::HitTestInvisible);
		Box->AddChildToVerticalBox(Rule);

		UVerticalBox* Body = T->ConstructWidget<UVerticalBox>();
		Body->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UVerticalBoxSlot* BodySlot = Box->AddChildToVerticalBox(Body);
		BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BodySlot->SetPadding(FMargin(static_cast<float>(CardInnerPadX), static_cast<float>(CardSeparation),
			static_cast<float>(CardInnerPadX), 0.0f));
		const FMargin Gap(0.0f, 0.0f, 0.0f, static_cast<float>(CardSeparation));
		// "NEW" versus "LEVEL 2 of 3" is the single most useful thing on the card (Godot).
		RankText = CardText(T, CardRankPx, PawTokens::UI_CARD_BANISH, BodyWrap);
		Body->AddChildToVerticalBox(RankText)->SetPadding(Gap);
		// (The card's picture goes here with phase 6's art; cards without one keep this layout, as in Godot.)
		BodyText = CardText(T, CardBodyPx, PawTokens::INK, BodyWrap);
		UVerticalBoxSlot* TextSlot = Body->AddChildToVerticalBox(BodyText);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetPadding(Gap);
		WarnText = CardText(T, CardWarnPx, PawTokens::UI_CARD_BANISH, BodyWrap);
		WarnText->SetText(PawText::Ui(TEXT("PICK_costs")));
		WarnText->SetVisibility(ESlateVisibility::Collapsed);
		Body->AddChildToVerticalBox(WarnText)->SetPadding(Gap);

		Take = T->ConstructWidget<UPawButton>(UPawButton::StaticClass());
		Take->SetLabel(PawText::Ui(TEXT("PICK_take")));
		Take->SetMinDimensions(0, static_cast<int32>(CardTakeHeight));
		Body->AddChildToVerticalBox(Take)->SetPadding(Gap);
		// BANISH sits under Take, smaller and quieter: destroying a card is the rarer intent, and it must not be what a fast
		// clicker hits. Shown only while a banish is held.
		Banish = T->ConstructWidget<UPawButton>(UPawButton::StaticClass());
		Banish->SetLabel(PawText::Ui(TEXT("PICK_banish")));
		Banish->SetLabelSize(CardBanishPx);
		Banish->SetLabelColour(true, PawTokens::UI_CARD_BANISH);
		Banish->SetMinDimensions(0, static_cast<int32>(CardBanishHeight));
		Banish->SetVisibility(ESlateVisibility::Collapsed);
		Body->AddChildToVerticalBox(Banish);
	}
	const bool bOk = Super::Initialize();
	// The mouse's whole-card target, not a stop for the keys: Take is the card's one selectable control, so a pad does
	// not land on the same card twice.
	SetIsFocusable(false);
	SetRenderTransformPivot(FVector2D(0.5, 0.5)); // grow from the middle
	return bOk;
}

void UPawCardFace::SetLook(const FLook& Look)
{
	FlavourText->SetText(Look.Flavour);
	NameText->SetText(Look.Name);
	RankText->SetText(Look.Rank);
	SetTextColour(RankText, Look.RankColour);
	BodyText->SetText(Look.Body);
	WarnText->SetVisibility(Look.bCosts ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	Banish->SetVisibility(Look.bBanish ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	// The trade is visible before the text is read: a card that costs you something wears the alert frame.
	Plate->SetBrush(Rounded(PawTokens::UI_CARD_PLATE, Look.bCosts ? PawTokens::UI_CARD_BANISH : PawTokens::INK,
		static_cast<float>(CardFramePx), static_cast<float>(CardRadius)));
	const float R = static_cast<float>(CardBandRadius);
	Band->SetBrush(RoundedCorners(Look.Band, Look.Band, 0.0f, FVector4(R, R, 0.0f, 0.0f)));
	// A pooled card starts flat again.
	LiftScale = 1.0f;
	SetRenderScale(FVector2D(1.0, 1.0));
}

bool UPawCardFace::IsLifted() const
{
	return IsHovered() || Selected(Take) || Selected(Banish);
}

void UPawCardFace::StepLift(double DeltaSeconds)
{
	// Scale, not size: drawn, not laid out, so the card grows over the gap instead of reflowing the hand under the cursor.
	const float Target = IsLifted() ? static_cast<float>(CardHoverScale) : 1.0f;
	const float Rate = static_cast<float>((CardHoverScale - 1.0) / CardHoverSec);
	const float Next = FMath::FInterpConstantTo(LiftScale, Target, static_cast<float>(DeltaSeconds), Rate);
	if (Next != LiftScale)
	{
		LiftScale = Next;
		SetRenderScale(FVector2D(LiftScale, LiftScale));
	}
}

void UPawCardFace::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	StepLift(DeltaSeconds);
}

FText UPawCardFace::GetFlavourText() const { return FlavourText ? FlavourText->GetText() : FText::GetEmpty(); }
FText UPawCardFace::GetNameText() const { return NameText ? NameText->GetText() : FText::GetEmpty(); }
FText UPawCardFace::GetRankText() const { return RankText ? RankText->GetText() : FText::GetEmpty(); }
FLinearColor UPawCardFace::GetRankColour() const { return RankText ? RankText->GetColorAndOpacity().GetSpecifiedColor() : FLinearColor::Black; }
FText UPawCardFace::GetBodyText() const { return BodyText ? BodyText->GetText() : FText::GetEmpty(); }
bool UPawCardFace::HasCostFrame() const { return Plate && Plate->Background.OutlineSettings.Color.GetSpecifiedColor() == PawTokens::UI_CARD_BANISH; }
FLinearColor UPawCardFace::GetBandColour() const { return Band ? Band->Background.TintColor.GetSpecifiedColor() : FLinearColor::Transparent; }
bool UPawCardFace::IsWarningShown() const { return WarnText && WarnText->GetVisibility() != ESlateVisibility::Collapsed; }

// --- the screen ------------------------------------------------------------------------------------------------------

bool UPawCardPickScreen::Initialize()
{
	if (!HasAnyFlags(RF_ClassDefaultObject) && !WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UWidgetTree* T = WidgetTree;
		UOverlay* Root = T->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		// Eats every click behind it: without it a Pounce fires through the modal (Godot's MOUSE_FILTER_STOP). The tree
		// page's violet, not black: the game's night with the lights down.
		Dim = T->ConstructWidget<UBorder>();
		Dim->SetBrush(Rounded(PawTokens::SKY_BASE.CopyWithNewOpacity(static_cast<float>(PickDimAlpha)), PawTokens::INK, 0.0f, 0.0f));
		Dim->SetVisibility(ESlateVisibility::Visible);
		UOverlaySlot* DimSlot = Root->AddChildToOverlay(Dim);
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* Column = T->ConstructWidget<UVerticalBox>();
		Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column);
		ColumnSlot->SetHorizontalAlignment(HAlign_Center);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
		const FMargin Gap(0.0f, 0.0f, 0.0f, static_cast<float>(PickColumnGap));

		// Paper text: the title sits on the dim, not on a card.
		Title = MakeText(T, PickTitlePx, PawTokens::UI_PANEL);
		Title->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(Title);
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(Gap);
		Subtitle = MakeText(T, PickSubtitlePx, PawTokens::DISABLED);
		Subtitle->SetJustification(ETextJustify::Center);
		Subtitle->SetAutoWrapText(false);
		Subtitle->SetWrapTextAt(SubtitleWrap);
		UVerticalBoxSlot* SubSlot = Column->AddChildToVerticalBox(Subtitle);
		SubSlot->SetHorizontalAlignment(HAlign_Center);
		SubSlot->SetPadding(Gap);

		Row = T->ConstructWidget<UHorizontalBox>();
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row);
		RowSlot->SetHorizontalAlignment(HAlign_Center);
		RowSlot->SetPadding(Gap);

		RerollButton = T->ConstructWidget<UPawButton>(UPawButton::StaticClass());
		RerollButton->SetMinDimensions(0, static_cast<int32>(RerollHeight));
		RerollButton->SetVisibility(ESlateVisibility::Collapsed);
		RerollButton->OnClicked().AddWeakLambda(this, [this]() { ClickReroll(); });
		Column->AddChildToVerticalBox(RerollButton)->SetHorizontalAlignment(HAlign_Center);
	}
	return Super::Initialize();
}

UWidget* UPawCardPickScreen::GetDim() const { return Dim; }

int32 UPawCardPickScreen::GetShownCardCount() const
{
	int32 N = 0;
	for (const UPawCardFace* Face : Faces)
	{
		N += Face && Face->GetVisibility() != ESlateVisibility::Collapsed ? 1 : 0;
	}
	return N;
}
FText UPawCardPickScreen::GetTitleText() const { return Title ? Title->GetText() : FText::GetEmpty(); }
FText UPawCardPickScreen::GetSubtitleText() const { return Subtitle ? Subtitle->GetText() : FText::GetEmpty(); }

UPawRealmSubsystem* UPawCardPickScreen::Realm() const
{
	const UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UPawRealmSubsystem>() : nullptr;
}

UPawCardFace* UPawCardPickScreen::FaceAt(int32 Index)
{
	while (Faces.Num() <= Index)
	{
		const int32 I = Faces.Num();
		UPawCardFace* Face = WidgetTree->ConstructWidget<UPawCardFace>(UPawCardFace::StaticClass());
		Face->OnClicked().AddWeakLambda(this, [this, I]() { ClickTake(I); });
		Face->GetTake()->OnClicked().AddWeakLambda(this, [this, I]() { ClickTake(I); });
		Face->GetBanish()->OnClicked().AddWeakLambda(this, [this, I]() { ClickBanish(I); });
		UHorizontalBoxSlot* S = Row->AddChildToHorizontalBox(Face);
		S->SetVerticalAlignment(VAlign_Fill);
		S->SetPadding(FMargin(I == 0 ? 0.0f : static_cast<float>(CardGap), 0.0f, 0.0f, 0.0f));
		Faces.Add(Face);
	}
	return Faces[Index];
}

void UPawCardPickScreen::ShowHand(bool bRefocus)
{
	UPawRealmSubsystem* R = Realm();
	UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
	if (!G || !G->IsDataReady())
	{
		Close();
		return;
	}
	const Pc::PcCardPick Pick = Pc::CardPick(G->Data(), G->Progress());
	if (!Pick.bOpen || Pick.Cards.empty())
	{
		HandIds.Reset();
		Close();
		return;
	}

	FFormatNamedArguments TitleArgs;
	TitleArgs.Add(TEXT("n"), PawText::Count(Pick.Level));
	Title->SetText(PawText::Ui(TEXT("PICK_title"), TitleArgs));
	FText Sub = PawText::Ui(TEXT("PICK_subtitle"));
	// BLUEPRINT: the promised cards, on a line under the subtitle.
	if (!Pick.Foresight.empty())
	{
		TArray<FText> Names;
		for (const std::string& Id : Pick.Foresight)
		{
			Names.Add(PawText::Data(TEXT("CARD"), Id));
		}
		FFormatNamedArguments Args;
		Args.Add(TEXT("subtitle"), Sub);
		Args.Add(TEXT("cards"), FText::Join(PawText::Ui(TEXT("PICK_list_sep")), Names));
		Sub = PawText::Ui(TEXT("PICK_foresight"), Args);
	}
	Subtitle->SetText(Sub);

	HandIds.Reset();
	for (int32 I = 0; I < static_cast<int32>(Pick.Cards.size()); ++I)
	{
		const Pc::PcPickCard& C = Pick.Cards[static_cast<size_t>(I)];
		HandIds.Add(UTF8_TO_TCHAR(C.CardId.c_str()));
		UPawCardFace::FLook Look;
		Look.Flavour = PawText::Ui(FlavourKey(C.Flavour));
		Look.Band = FlavourColour(C.Flavour);
		Look.Name = PawText::Data(TEXT("CARD"), C.CardId);
		if (C.bNew)
		{
			Look.Rank = PawText::Ui(TEXT("PICK_new"));
			Look.RankColour = PawTokens::UI_CARD_BANISH;
		}
		else
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("level"), PawText::Count(C.Level));
			Args.Add(TEXT("max"), PawText::Count(C.MaxLevel));
			Look.Rank = PawText::Ui(TEXT("PICK_level"), Args);
			Look.RankColour = PawTokens::UI_TEXT_MUTED;
		}
		FFormatNamedArguments Values;
		Values.Add(TEXT("v"), PawText::CardNumber(C.Value));
		Values.Add(TEXT("c"), PawText::CardNumber(C.Cost));
		Look.Body = FText::Format(PawText::DataField(TEXT("CARD"), C.CardId, TEXT("desc")), Values);
		Look.bCosts = C.bCosts;
		Look.bBanish = Pick.bBanish;
		UPawCardFace* Face = FaceAt(I);
		Face->SetLook(Look);
		Face->SetVisibility(ESlateVisibility::Visible);
	}
	for (int32 I = HandIds.Num(); I < Faces.Num(); ++I)
	{
		Faces[I]->SetVisibility(ESlateVisibility::Collapsed);
	}

	ShowReroll(Pick.Rerolls);
	// The guard's clock starts with every deal, a reroll's included.
	DealtAt = Clock;
	if (bRefocus && CanTakeFocus())
	{
		if (UWidget* Target = NativeGetDesiredFocusTarget())
		{
			Target->SetFocus();
		}
	}
}

UWidget* UPawCardPickScreen::NativeGetDesiredFocusTarget() const
{
	// The first card's Take; with no hand (never shown so) the Reroll button: never nothing (the pad dies).
	if (HandIds.Num() > 0 && Faces.Num() > 0 && Faces[0])
	{
		return Faces[0]->GetTake();
	}
	return RerollButton;
}

TOptional<FUIInputConfig> UPawCardPickScreen::GetDesiredInputConfig() const
{
	// A menu over a held field: Common UI keeps the game's input out while it is up. Its buttons are Slate widgets, so a
	// free cursor clicks them without any viewport capture (the capture rule of KB U/08 §3.2 is for clicks on the field).
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UPawCardPickScreen::NativeOnActivated()
{
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, true);
	}
	// Pooled: everything is re-read here, never kept from the last hand (docs/05 §2 rule 5). Common UI focuses the desired
	// target after this: exactly one Set Focus per activation.
	ShowHand(false);
	Super::NativeOnActivated();
}

void UPawCardPickScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	// Only this screen's hold: the field resumes only when nothing else holds it (the tree, the ceremony, the pause menu).
	if (UPawRealmSubsystem* R = Realm())
	{
		R->SetPauseHold(PauseHolder, false);
	}
}

void UPawCardPickScreen::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	AdvanceClock(DeltaSeconds);
}

FReply UPawCardPickScreen::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	NoteKeyDown(KeyEvent.IsRepeat());
	return Super::NativeOnPreviewKeyDown(Geometry, KeyEvent);
}

FReply UPawCardPickScreen::NativeOnPreviewMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	NoteMouseDown();
	return Super::NativeOnPreviewMouseButtonDown(Geometry, MouseEvent);
}

void UPawCardPickScreen::NoteKeyDown(bool bRepeat)
{
	bLastWasPointer = false;
	if (!bRepeat)
	{
		PressBeganAt = Clock; // a held key's repeats are the same press: it began when it began
	}
}

void UPawCardPickScreen::NoteMouseDown()
{
	bLastWasPointer = true;
}

bool UPawCardPickScreen::GuardAllows(double InDealtAt, double Now, bool bInLastWasPointer, double InPressBeganAt)
{
	if (Now - InDealtAt < ModalGuardSec)
	{
		return false;
	}
	// Strictly after the deal: the press that dealt this hand (Reroll, Banish) began at the deal's own instant, and must not
	// take a card from the hand it dealt (Godot compares milliseconds with >=, where the same press can land in the same
	// millisecond; the intent, its comment says, is this).
	return bInLastWasPointer || InPressBeganAt > InDealtAt;
}

bool UPawCardPickScreen::IsGuardOpen() const
{
	return GuardAllows(DealtAt, Clock, bLastWasPointer, PressBeganAt);
}

void UPawCardPickScreen::ClickTake(int32 Index)
{
	if (IsGuardOpen())
	{
		Take(Index);
	}
}

void UPawCardPickScreen::ClickBanish(int32 Index)
{
	if (IsGuardOpen())
	{
		Banish(Index);
	}
}

void UPawCardPickScreen::ClickReroll()
{
	if (IsGuardOpen())
	{
		Reroll();
	}
}

bool UPawCardPickScreen::Take(int32 Index)
{
	UPawRealmSubsystem* R = Realm();
	bool bTaken = false;
	if (R && HandIds.IsValidIndex(Index))
	{
		bTaken = R->ChooseCard(HandIds[Index]);
	}
	if (!bTaken)
	{
		// choose() refuses anything not in the live hand: the screen is showing a stale hand. Close rather than leave the
		// player clicking a card that will never be granted (Godot).
		UE_LOG(LogPaw, Error, TEXT("Card pick: %s was not in the offered hand"), HandIds.IsValidIndex(Index) ? *HandIds[Index] : TEXT("(no card)"));
	}
	Close();
	return bTaken;
}

bool UPawCardPickScreen::Banish(int32 Index)
{
	UPawRealmSubsystem* R = Realm();
	if (!R || !HandIds.IsValidIndex(Index) || !R->BanishFromHand(HandIds[Index]))
	{
		return false;
	}
	// A re-deal, or the deck banished dry: ShowHand closes when no hand came back.
	ShowHand(true);
	return true;
}

bool UPawCardPickScreen::Reroll()
{
	UPawRealmSubsystem* R = Realm();
	if (!R || !R->RerollHand())
	{
		// The button's count was stale: redraw only the button (Godot _refresh_reroll); not a deal, the guard runs on.
		UPawGameSubsystem* G = R ? R->GetGame() : nullptr;
		if (G && G->IsDataReady())
		{
			const bool bHadFocus = Selected(RerollButton);
			const Pc::PcCardPick Pick = Pc::CardPick(G->Data(), G->Progress());
			ShowReroll(Pick.Rerolls);
			if (bHadFocus && Pick.Rerolls <= 0 && CanTakeFocus())
			{
				if (UWidget* Target = NativeGetDesiredFocusTarget())
				{
					Target->SetFocus();
				}
			}
		}
		return false;
	}
	ShowHand(true);
	return true;
}

void UPawCardPickScreen::ShowReroll(int64 Rerolls)
{
	// _refresh_reroll(), then the rows wired again (a Reroll that comes or goes changes them): Take / Banish / Reroll.
	FFormatNamedArguments Args;
	Args.Add(TEXT("n"), PawText::Count(Rerolls));
	RerollButton->SetLabel(PawText::Ui(TEXT("PICK_reroll"), Args));
	RerollButton->SetVisibility(Rerolls > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	TArray<UWidget*> Takes, Banishes;
	for (int32 I = 0; I < HandIds.Num(); ++I)
	{
		Takes.Add(Faces[I]->GetTake());
		Banishes.Add(Faces[I]->GetBanish());
	}
	Wired = WireRows({ Takes, Banishes, { RerollButton.Get() } });
}

bool UPawCardPickScreen::CanTakeFocus() const
{
	// Focus belongs to a local player: a screen built without one (the automation world) has none to give.
	return IsActivated() && GetOwningLocalPlayer() != nullptr;
}

void UPawCardPickScreen::Close()
{
	if (IsActivated())
	{
		DeactivateWidget();
	}
}
