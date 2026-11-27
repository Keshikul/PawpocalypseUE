// Pawpocalypse.UI.Title.*: the title screen (UPawMainMenu) on a sandboxed save (memory storage). The rules are proven against
// the real main_menu.tscn in PawCore (PcTitleTests, fixture `title`); these prove the screen says them in Godot's words
// (the string tables, the line breaks), and the flows: a new game, a load, a delete, quitting, Back, the links.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GeneralProjectSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcAchievements.h"
#include "PcData.h"
#include "PcProgress.h"
#include "PcSave.h"
#include "PcStats.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "PcSettings.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "UI/PawButton.h"
#include "UI/PawMainMenu.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawWidgets.h"
#include "UI/PawGarden.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "Components/PanelWidget.h"
#include "PcBackdrop.h"
#include "UI/PawTextTesting.h"

#include <string>

namespace
{
	struct FTitleWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawSaveSubsystem* Save = nullptr;
		UPawMainMenu* Menu = nullptr;
		TSharedPtr<FPawMemorySaveStorage> Disk;
		int32 Plays = 0;
		int32 Quits = 0;
		TArray<FString> Opened;
		double Clock = 1.8e9;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			Save = GI->GetSubsystem<UPawSaveSubsystem>();
			if (!T.TestNotNull(TEXT("the game"), Game) || !T.TestNotNull(TEXT("the saves"), Save) || !T.TestTrue(TEXT("data"), Game->IsDataReady()))
			{
				return false;
			}
			Game->NewGame();
			Disk = MakeShared<FPawMemorySaveStorage>();
			Save->UseStorageForTest(Disk);
			// What ships: the Widget Blueprint (Pawpocalypse.UI.Widgets.* checks it is one and matches the code layout).
			Menu = CreateWidget<UPawMainMenu>(GI, PawUi::Resolve<UPawMainMenu>());
			if (!T.TestNotNull(TEXT("the title screen"), Menu))
			{
				return false;
			}
			Menu->OnPlay = [this]() { ++Plays; };
			Menu->OnQuit = [this]() { ++Quits; };
			Menu->OnOpenUrl = [this](const FString& Url) { Opened.Add(Url); };
			Menu->Now = [this]() { return Clock; };
			Menu->Refresh();
			return true;
		}

		void Close()
		{
			if (Save)
			{
				Save->UseStorageForTest(nullptr);
			}
			if (GI)
			{
				GI->Shutdown();
			}
			if (World)
			{
				GEngine->DestroyWorldContext(World);
				World->DestroyWorld(false);
			}
		}

		Pc::PcProgress& P() { return Game->Progress(); }
		const Pc::PcGameData& D() { return Game->Data(); }

		void PutSlot(int32 Slot, int64 Reawakens, int64 Kills, double Play, double Unix, std::set<std::string> Bosses = {},
			bool bFinalBoss = false)
		{
			Pc::PcSaveState S;
			S.BossesDefeated = std::move(Bosses);
			S.bFinalBossDefeated = bFinalBoss;
			S.ReawakenCount = Reawakens;
			S.LifetimeKills = Kills;
			S.TotalPlaySeconds = Play;
			S.LastPlayedUnix = Unix;
			S.LifetimeCatnipEver = 10.0;
			const std::string File = Pc::WriteSaveFile(S, "test");
			Disk->Put(UPawSaveSubsystem::SlotName(Slot), TArray<uint8>(reinterpret_cast<const uint8*>(File.data()), static_cast<int32>(File.size())));
		}

		void PutJunk(int32 Slot)
		{
			const char Junk[] = "not a save";
			Disk->Put(UPawSaveSubsystem::SlotName(Slot), TArray<uint8>(reinterpret_cast<const uint8*>(Junk), 10));
		}

		// A save a newer build wrote (its container format past this build's).
		void PutNewer(int32 Slot, int64 Reawakens)
		{
			Pc::PcSaveState S;
			S.ReawakenCount = Reawakens;
			S.LifetimeCatnipEver = 10.0;
			std::string File = Pc::WriteSaveFile(S, "future");
			File.replace(0, 9, "PAWSAVE 7");
			Disk->Put(UPawSaveSubsystem::SlotName(Slot), TArray<uint8>(reinterpret_cast<const uint8*>(File.data()), static_cast<int32>(File.size())));
		}

		TArray<uint8> Bytes(const FString& Name)
		{
			TArray<uint8> Out;
			Disk->Read(Name, Out);
			return Out;
		}

		void Wipe()
		{
			for (int32 I = 0; I < UPawMainMenu::SlotCount; ++I)
			{
				Save->DeleteSlot(I);
			}
		}
	};

	FString S(const FText& T) { return T.ToString(); }

	FString Unescape(const FString& In)
	{
		FString Out;
		for (int32 I = 0; I < In.Len(); ++I)
		{
			if (In[I] == TEXT('\\') && I + 1 < In.Len())
			{
				const TCHAR C = In[++I];
				Out.AppendChar(C == TEXT('n') ? TEXT('\n') : C == TEXT('t') ? TEXT('\t') : C);
			}
			else
			{
				Out.AppendChar(In[I]);
			}
		}
		return Out;
	}

	double Hex(const FString& V) { return std::strtod(TCHAR_TO_UTF8(*V), nullptr); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleGodotTest, "Pawpocalypse.UI.Title.MatchesGodot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The real main_menu.tscn's words, row for row (Tests/Data/title_godot.tsv): the status line, Continue / Load, every card
// (the summary read from a real save file, a missing or unreadable slot reading Empty), the leaderboard.
bool FPawTitleGodotTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FString Text;
	const FString Path = FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/title_godot.tsv");
	if (!TestTrue(TEXT("the fixture"), FFileHelper::LoadFileToString(Text, *Path)))
	{
		return false;
	}
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	TArray<FString> Header;
	int32 Rows = 0, Cards = 0, Mismatch = 0;
	for (const FString& Line : Lines)
	{
		if (Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		TArray<FString> Cells;
		Line.ParseIntoArray(Cells, TEXT("\t"), false);
		if (Header.Num() == 0)
		{
			Header = Cells;
			continue;
		}
		if (Rows >= 300)
		{
			break;
		}
		TMap<FString, FString> V;
		for (int32 I = 0; I < Header.Num() && I < Cells.Num(); ++I)
		{
			V.Add(Header[I], Cells[I]);
		}
		++Rows;
		Pc::PcProgress& P = W.P();
		P.LifetimeCatnipEver = Hex(V[TEXT("catnip_ever")]);
		P.ReawakenCount = FCString::Atoi64(*V[TEXT("reawakens")]);
		P.LifetimeKills = FCString::Atoi64(*V[TEXT("kills")]);
		P.FinalBossSeconds = Hex(V[TEXT("final_seconds")]);
		P.bFinalBossDefeated = V[TEXT("final_defeated")] == TEXT("1");
		W.Wipe();
		const FString Present = V[TEXT("has_slots")];
		const int32 Unreadable = FCString::Atoi(*V[TEXT("unreadable")]);
		for (int32 Si = 0; Si < UPawMainMenu::SlotCount; ++Si)
		{
			const FString Pre = FString::Printf(TEXT("s%d_"), Si);
			if (Si == Unreadable)
			{
				W.PutJunk(Si);
			}
			else if (FCString::Atoi64(*V[Pre + TEXT("reawakens")]) >= 0)
			{
				W.PutSlot(Si, FCString::Atoi64(*V[Pre + TEXT("reawakens")]), FCString::Atoi64(*V[Pre + TEXT("kills")]),
					Hex(V[Pre + TEXT("play")]), Hex(V[Pre + TEXT("unix")]));
			}
		}
		W.Clock = Hex(V[TEXT("s0_now")]);
		W.Menu->Refresh();
		W.Menu->PressLeaderboard();
		W.Menu->PressClose();
		W.Menu->PressNewGame();
		auto Same = [&](const TCHAR* What, const FString& Got, const FString& Want)
		{
			if (Got != Want)
			{
				++Mismatch;
				if (Mismatch <= 8)
				{
					AddError(FString::Printf(TEXT("row %d %s: '%s' vs Godot '%s'"), Rows, What, *Got.ReplaceCharWithEscapedChar(), *Want.ReplaceCharWithEscapedChar()));
				}
			}
		};
		Same(TEXT("status"), S(W.Menu->GetStatusText()), Unescape(V[TEXT("status")]));
		Same(TEXT("continue"), W.Menu->GetMenuButton(0)->GetIsEnabled() ? TEXT("0") : TEXT("1"), V[TEXT("continue_disabled")]);
		Same(TEXT("load"), W.Menu->GetMenuButton(2)->GetIsEnabled() ? TEXT("0") : TEXT("1"), V[TEXT("load_disabled")]);
		Same(TEXT("leaderboard"), S(W.Menu->GetLeaderboardText()), Unescape(V[TEXT("leaderboard")]));
		for (int32 Si = 0; Si < UPawMainMenu::SlotCount; ++Si)
		{
			const FString Pre = FString::Printf(TEXT("s%d_"), Si);
			Same(TEXT("card"), S(W.Menu->GetSlotText(Si)), Unescape(V[Pre + TEXT("info")]));
			Same(TEXT("delete"), W.Menu->GetSlotDelete(Si)->GetVisibility() == ESlateVisibility::Collapsed ? TEXT("0") : TEXT("1"), V[Pre + TEXT("delete")]);
			++Cards;
		}
		W.Menu->PressSlotsBack();
	}
	TestEqual(TEXT("rows"), Rows, 300);
	TestEqual(TEXT("every line as Godot's"), Mismatch, 0);
	AddInfo(FString::Printf(TEXT("%d rows, %d cards"), Rows, Cards));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleLookTest, "Pawpocalypse.UI.Title.Look", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The fixed words and the doors that cannot open yet: the name, the five slot numbers, the hint's keys, the credits, the
// version line; Settings and Language disabled until their slices, Bug / Discord live.
bool FPawTitleLookTest::RunTest(const FString& Parameters)
{
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawMainMenu* M = W.Menu;
	// Wired in what ships (the Widget Blueprint): every part found and connected.
	for (int32 I = 0; I < 7; ++I)
	{
		if (!TestNotNull(FString::Printf(TEXT("menu button %d is wired"), I), M->GetMenuButton(I)))
		{
			W.Close();
			return false;
		}
	}
	for (int32 I = 0; I < UPawMainMenu::SlotCount; ++I)
	{
		TestNotNull(TEXT("a slot card is wired"), M->GetSlotCard(I));
	}
	TestEqual(TEXT("the name"), S(M->GetNameText()), FString(TEXT("PAWPOCALYPSE")));
	// input_prompts_godot.tsv title_hint, family 0: keycaps around the keys' labels.
	TestEqual(TEXT("the hint, Godot's keys"), M->GetHintText(), FString(TEXT("In game:  <k:A> <k:D> menu tabs    <k:T> skill tree    <k:Esc> pause")));
	TestEqual(TEXT("the credits"), S(M->GetCreditsText()), FString(TEXT("created by keshi")));
	// The version line: "v" and the project's version, 0.MINOR.PATCH (the designer, 2026-11-23: "Use minor type version
	// numbers"; Config/DefaultGame.ini; tools/package.py --bump raises the last number).
	const FString Version = GetDefault<UGeneralProjectSettings>()->ProjectVersion;
	TestEqual(TEXT("the version line"), S(M->GetVersionText()), TEXT("v") + Version);
	TArray<FString> VersionParts;
	Version.ParseIntoArray(VersionParts, TEXT("."), false);
	TestTrue(TEXT("the version is 0.MINOR.PATCH"), VersionParts.Num() == 3 && VersionParts[0] == TEXT("0") && VersionParts[1].IsNumeric()
		&& VersionParts[2].IsNumeric());
	for (int32 I = 0; I < UPawMainMenu::SlotCount; ++I)
	{
		TestEqual(TEXT("the slot number"), S(M->GetSlotNumberText(I)), FString::Printf(TEXT("Slot %d"), I + 1));
	}
	const TCHAR* Labels[] = { TEXT("Continue"), TEXT("New Game"), TEXT("Load"), TEXT("Leaderboard"), TEXT("Settings"), TEXT("Credits"), TEXT("Exit") };
	for (int32 I = 0; I < 7; ++I)
	{
		TestEqual(TEXT("the menu, Godot's order"), S(M->GetMenuButton(I)->GetLabel()), FString(Labels[I]));
	}
	TestTrue(TEXT("Settings opens the settings dialog"), M->GetMenuButton(4)->GetIsEnabled());
	TestTrue(TEXT("Report a bug is live"), M->GetClusterButton(0)->GetIsEnabled());
	TestTrue(TEXT("Discord is live"), M->GetClusterButton(1)->GetIsEnabled());
	TestTrue(TEXT("Language opens the language list (phase 7)"), M->GetClusterButton(2)->GetIsEnabled());
	TestFalse(TEXT("the cluster is mouse only"), M->GetClusterButton(0)->GetIsFocusable());
	// Phase 6 slice 5b: the plates wear main_menu.gd's icons (Art.texture), no words; the name is the tooltip.
	const TCHAR* ClusterIcons[] = { TEXT("ui_menu_bug"), TEXT("ui_menu_discord"), TEXT("ui_menu_language") };
	for (int32 I = 0; I < 3; ++I)
	{
		const UPawButton* B = M->GetClusterButton(I);
		TestTrue(FString::Printf(TEXT("plate %d wears %s, no words"), I, ClusterIcons[I]),
			B && B->IsIconShown() && B->GetIconName() == ClusterIcons[I] && B->GetLabel().IsEmpty() && !B->GetToolTipText().IsEmpty());
	}
	// A fresh install: no save, no progress. Continue and Load cannot open; the selection opens on New Game.
	TestFalse(TEXT("Continue is a door that cannot open"), M->GetMenuButton(0)->GetIsEnabled());
	TestFalse(TEXT("Load too"), M->GetMenuButton(2)->GetIsEnabled());
	TestTrue(TEXT("New Game has the selection"), M->GetTopDefault() == M->GetMenuButton(1));
	M->PressNewGame();
	TestEqual(TEXT("the slots are wired: five cards, no Delete, Back"), M->GetWired().Num(), 6);
	M->PressSlotsBack();
	TestEqual(TEXT("the menu is wired without the disabled rows (New Game, Leaderboard, Settings, Credits, Exit)"), M->GetWired().Num(), 5);
	M->PressContinue();
	TestEqual(TEXT("a disabled Continue plays nothing"), W.Plays, 0);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitlePlumTest, "Pawpocalypse.UI.Title.Plum", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Restyle R4c: the title after the canvas board "Main menu (plum)", in what ships (the Widget Blueprint, grafted) and in the
// code layout it was grafted from: the logo Butter, big and tilted at the top left; the menu on the left, its words on the
// left at the board's sizes; the hint on its plum plate; Continue the Butter call only while there is something to continue.
bool FPawTitlePlumTest::RunTest(const FString& Parameters)
{
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	auto CanvasSlotOf = [](UWidget* Wd) -> UCanvasPanelSlot*
	{
		for (UWidget* At = Wd; At; At = At->GetParent())
		{
			if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(At->Slot))
			{
				return S;
			}
		}
		return nullptr;
	};
	UPawMainMenu* FromCode = nullptr;
	PawUi::SetCodeLayoutForced(true);
	FromCode = CreateWidget<UPawMainMenu>(W.GI, UPawMainMenu::StaticClass());
	PawUi::SetCodeLayoutForced(false);
	for (UPawMainMenu* M : { W.Menu, FromCode })
	{
		if (!TestNotNull(TEXT("a title"), M))
		{
			continue;
		}
		const FString Where = M == W.Menu ? TEXT("WBP_MainMenu") : TEXT("the code layout");
		const UTextBlock* Name = Cast<UTextBlock>(M->WidgetTree->FindWidget(TEXT("NameText")));
		if (TestNotNull(Where + TEXT(": the logo"), Name))
		{
			TestTrue(Where + TEXT(": the logo is Butter"), Name->GetColorAndOpacity().GetSpecifiedColor().Equals(PawTokens::UI_SELECTED));
			TestEqual(Where + TEXT(": the logo's size"), Name->GetFont().Size, PawUi::FontPt(PawUi::R4NamePx));
			// D18 (S4, the UI concept's scrTitle): upright and centred at the top (R4c had it tilted at the top left).
			const UWidget* Box = Name->GetParent();
			TestTrue(Where + TEXT(": the logo upright"), Box && FMath::IsNearlyEqual(Box->GetRenderTransform().Angle, static_cast<float>(PawUi::TitleLogoAngle)));
			const UBorderSlot* InBox = Cast<UBorderSlot>(Name->Slot);
			TestTrue(Where + TEXT(": the logo centred in its box"), InBox && InBox->GetHorizontalAlignment() == HAlign_Center);
			const UCanvasPanelSlot* S = CanvasSlotOf(const_cast<UTextBlock*>(Name));
			TestTrue(Where + TEXT(": the logo at the top centre"), S && S->GetAnchors().Minimum == FVector2D(0.5, 0.0)
				&& FMath::IsNearlyEqual(S->GetOffsets().Left, static_cast<float>(-0.5 * PawUi::R4NameWidth))
				&& FMath::IsNearlyEqual(S->GetOffsets().Top, static_cast<float>(PawUi::TitleLogoTop)));
		}
		UPawButton* Continue = M->GetMenuButton(0);
		if (TestNotNull(Where + TEXT(": Continue"), Continue))
		{
			const UCanvasPanelSlot* S = CanvasSlotOf(Continue);
			// R4h: in its room (left edge, top to bottom; Pawpocalypse.UI.Layout.Title proves it fits at 150 %).
			TestTrue(Where + TEXT(": the menu on the left"), S && S->GetAnchors().Minimum == FVector2D(0.0, 0.0)
				&& S->GetAnchors().Maximum == FVector2D(0.0, 1.0) && FMath::IsNearlyEqual(S->GetOffsets().Left, static_cast<float>(PawUi::R4TitleMargin)));
			// The rooms' ends, the constants' (the asset and the code agree; a constant moved alone shows here).
			TestTrue(Where + TEXT(": the menu's room ends above the hint"), S && FMath::IsNearlyEqual(S->GetOffsets().Bottom, static_cast<float>(PawUi::R4MenuBottom)));
			const UWidget* Card = M->WidgetTree->FindWidget(TEXT("BobaCard"));
			const UCanvasPanelSlot* C = Card ? CanvasSlotOf(const_cast<UWidget*>(Card)) : nullptr;
			TestTrue(Where + TEXT(": Boba's room ends above the version"), C && C->GetAnchors().Maximum == FVector2D(1.0, 1.0)
				&& FMath::IsNearlyEqual(C->GetOffsets().Bottom, static_cast<float>(PawUi::R4BobaBottom))
				&& FMath::IsNearlyEqual(C->GetOffsets().Top, static_cast<float>(PawUi::R4BobaRoomTop)));
			for (int32 I = 0; I < 7; ++I)
			{
				const UPawButton* B = M->GetMenuButton(I);
				TestTrue(FString::Printf(TEXT("%s: menu button %d's words on the left, at the board's size"), *Where, I), B && B->bAlignLeft
					&& FMath::IsNearlyEqual(B->LabelPx, static_cast<float>(I == 0 ? PawUi::R4ContinuePx : PawUi::R4MenuButtonPx)));
			}
		}
		// D18 (S4): the concept's cast on the garden, the Tiny Kitten and three mice, two of them mirrored; pictures only.
		auto Picture = [](const UWidget* W) -> FString
		{
			const UImage* I = Cast<UImage>(W);
			const UObject* Tex = I ? I->GetBrush().GetResourceObject() : nullptr;
			return Tex ? Tex->GetName() : FString();
		};
		const UWidget* Cat = M->WidgetTree->FindWidget(TEXT("TitleCat"));
		TestEqual(Where + TEXT(": the Tiny Kitten on the garden"), Picture(Cat), FString(TEXT("T_ui_icon_title_cat")));
		TestTrue(Where + TEXT(": a picture, never a click"), Cat && Cat->GetVisibility() == ESlateVisibility::HitTestInvisible);
		int32 Mice = 0, Mirrored = 0;
		for (int32 I = 0; I < 3; ++I)
		{
			const UWidget* Mouse = M->WidgetTree->FindWidget(PawUi::Numbered(TEXT("TitleMouse"), I));
			Mice += Picture(Mouse) == TEXT("T_ui_icon_title_mouse") ? 1 : 0;
			Mirrored += Mouse && Mouse->GetRenderTransform().Scale.X < 0.0 ? 1 : 0;
		}
		TestEqual(Where + TEXT(": three mice"), Mice, 3);
		TestEqual(Where + TEXT(": two mirrored, as the concept draws them"), Mirrored, 2);
		const UWidget* Number = M->WidgetTree->FindWidget(TEXT("SlotNumber0"));
		TestTrue(Where + TEXT(": the slot's number inside its card, at its top (D18)"), Number && Number->GetParent()
			&& Number->GetParent()->GetFName() == TEXT("SlotStack0") && Number->GetParent()->GetChildIndex(Number) == 0);
		const UTextBlock* Status = Cast<UTextBlock>(M->WidgetTree->FindWidget(TEXT("StatusText")));
		TestTrue(Where + TEXT(": the status line in ink (D18)"), Status && Status->GetColorAndOpacity().GetSpecifiedColor().Equals(PawTokens::UI_TEXT));
		const UBorder* Hint = Cast<UBorder>(M->WidgetTree->FindWidget(TEXT("HintPlate")));
		TestTrue(Where + TEXT(": the hint on the small plum plate"), Hint && Hint->Background.GetResourceObject()
			== PawUi::Chrome(PawUi::EPawChrome::PlateSmall).GetResourceObject()
			&& Hint->GetContent() && Hint->GetContent()->GetFName() == TEXT("HintPrompts"));
	}
	// A fresh install: Continue cannot open, and is not the call.
	TestFalse(TEXT("no save: Continue is not the call"), W.Menu->GetMenuButton(0)->HasCallLook());
	W.Game->Progress().ReawakenCount = 1;
	W.Save->SaveGame(0);
	W.Save->Flush();
	W.Menu->Refresh();
	TestTrue(TEXT("a save: Continue opens"), W.Menu->GetMenuButton(0)->GetIsEnabled());
	TestTrue(TEXT("a save: Continue is the Butter call"), W.Menu->GetMenuButton(0)->HasCallLook());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleGardenTest, "Pawpocalypse.UI.Title.Garden", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The garden behind the menu (main_menu.gd _build_garden): in what ships, right over the paper floor and under everything else,
// with its art; laid out as the "menu" biome (PcBackdrop, exact against Godot): five trees, puddles, the frame of bushes.
bool FPawTitleGardenTest::RunTest(const FString& Parameters)
{
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawGarden* G = W.Menu->WidgetTree ? Cast<UPawGarden>(W.Menu->WidgetTree->FindWidget(TEXT("Garden"))) : nullptr;
	if (!TestNotNull(TEXT("the title has its garden"), G))
	{
		W.Close();
		return false;
	}
	const UPanelWidget* Canvas = G->GetParent();
	TestTrue(TEXT("the garden is the second layer: over the floor, under everything else"), Canvas && Canvas->GetChildIndex(G) == 1);
	TestTrue(TEXT("the floor under it is the paper"), Canvas && Canvas->GetChildAt(0) && Canvas->GetChildAt(0)->IsA<UBorder>());
	TestTrue(TEXT("the garden has its floor and its atlas"), G->HasArt());
	TestEqual(TEXT("the garden takes no input"), G->GetVisibility(), ESlateVisibility::HitTestInvisible);
	G->LayOutFor(FVector2D(1920.0, 1080.0));
	int32 Trees = 0, Swaying = 0;
	for (const Pc::PcBackdropProp& P : G->GetScatter().Props)
	{
		Trees += P.Kind == Pc::PcBackdropPropKind::Landmark ? 1 : 0;
		Swaying += P.bSway ? 1 : 0;
	}
	TestEqual(TEXT("five trees stand in the garden"), Trees, 5);
	TestTrue(TEXT("the plants sway"), Swaying > 0);
	int32 Puddles = 0;
	for (const Pc::PcBackdropBlob& B : G->GetFrame().Blobs)
	{
		Puddles += B.bPuddle ? 1 : 0;
	}
	TestTrue(TEXT("puddles (the Rooftops' sky)"), Puddles > 0);
	TestEqual(TEXT("the frame is the menu biome's"), static_cast<int32>(G->GetFrame().Props.size()),
		static_cast<int32>(Pc::BuildBackdrop("menu", 1920.0, 1080.0, 1.0).Props.size()));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleNewGameTest, "Pawpocalypse.UI.Title.NewGame", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// An empty slot starts a new game THERE, written at once, and the slot that was active keeps its game (HANDOFF-LOG (34):
// the end-of-frame save must not write the empty game over it).
bool FPawTitleNewGameTest::RunTest(const FString& Parameters)
{
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	// A game in slot 0, loaded (as at startup).
	Pc::PcProgress& P = W.P();
	P.LifetimeCatnipEver = 5e6;
	P.Catnip = 1234.0;
	P.ReawakenCount = 3;
	P.LifetimeKills = 999;
	P.AchievementsEarned.insert("some_achievement");
	W.Game->Slot().bAutoTravel = true;
	W.Save->SaveGame(0);
	W.Save->Flush();
	TArray<uint8> Before;
	W.Disk->Read(UPawSaveSubsystem::SlotName(0), Before);
	W.Menu->Refresh();
	TestTrue(TEXT("Continue opens"), W.Menu->GetMenuButton(0)->GetIsEnabled());
	TestTrue(TEXT("Continue has the selection"), W.Menu->GetTopDefault() == W.Menu->GetMenuButton(0));
	W.Menu->PressNewGame();
	TestTrue(TEXT("the slot screen"), W.Menu->GetTopLayer() == UPawMainMenu::ELayer::Slots);
	TestTrue(TEXT("on the active slot's card"), W.Menu->GetTopDefault() == W.Menu->GetSlotCard(0));
	TestFalse(TEXT("an empty slot has no Delete"), W.Menu->GetSlotDelete(2)->GetVisibility() != ESlateVisibility::Collapsed);
	TestTrue(TEXT("an occupied one has"), W.Menu->GetSlotDelete(0)->GetVisibility() != ESlateVisibility::Collapsed);
	W.Menu->PressSlot(2);
	TestEqual(TEXT("then the game opens"), W.Plays, 1);
	TestEqual(TEXT("a new game: no Catnip"), P.Catnip, 0.0);
	TestEqual(TEXT("no Reawakens"), P.ReawakenCount, static_cast<int64_t>(0));
	TestTrue(TEXT("no achievements"), P.AchievementsEarned.empty());
	TestFalse(TEXT("both automations off"), W.Game->Slot().bAutoTravel);
	TestEqual(TEXT("slot 3 is the active one"), W.Save->GetActiveSlot(), 2);
	TestTrue(TEXT("and it exists at once"), W.Save->HasSlot(2));
	// The end-of-frame save, then the autosave: nothing may land in slot 0.
	W.Save->Tick(1.0f / 60.0f);
	W.Save->Autosave();
	W.Save->Flush();
	TArray<uint8> After;
	W.Disk->Read(UPawSaveSubsystem::SlotName(0), After);
	TestTrue(TEXT("slot 1 still holds its game, byte for byte"), Before == After);
	Pc::PcSlotSummary Old;
	TestTrue(TEXT("slot 1 reads"), W.Save->ReadSlotSummary(0, Old));
	TestEqual(TEXT("with its Reawakens"), Old.Reawakens, static_cast<int64_t>(3));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleLoadTest, "Pawpocalypse.UI.Title.Load", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// An occupied slot loads, no confirm, and the game opens on it; the loaded slot becomes the active one.
bool FPawTitleLoadTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.PutSlot(4, 7, 4321, 3700.0, W.Clock - 120.0);
	W.Menu->Refresh();
	TestTrue(TEXT("a save on disk: Load opens"), W.Menu->GetMenuButton(2)->GetIsEnabled());
	W.Menu->PressLoad();
	TestEqual(TEXT("its card"), S(W.Menu->GetSlotText(4)), FString(TEXT("Reawaken 7\n1h 01m played\n4.3K mice\n\nLast saved\n2 min ago")));
	W.Menu->PressSlot(4);
	TestEqual(TEXT("the game opens"), W.Plays, 1);
	TestEqual(TEXT("on the loaded game"), W.P().ReawakenCount, static_cast<int64_t>(7));
	TestEqual(TEXT("its kills"), W.P().LifetimeKills, static_cast<int64_t>(4321));
	TestEqual(TEXT("slot 5 is active"), W.Save->GetActiveSlot(), 4);
	TestFalse(TEXT("the slot screen is gone"), W.Menu->IsLayerShown(UPawMainMenu::ELayer::Slots));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleLoadOutcomesTest, "Pawpocalypse.UI.Title.LoadOutcomes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Review CR2 and F47 (a Godot fault the port fixes, D16): every load outcome reaches the player through the title, and
// nothing plays over a slot this build cannot read. A newer build's slot: said, Continue off, its card refuses. An
// unreadable one: said, Continue opens the slots, its card starts a new game there and the old file is kept aside. A game
// from the backup: said once.
bool FPawTitleLoadOutcomesTest::RunTest(const FString& Parameters)
{
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	// The loads below that meet a broken file say so in the log (the unreadable one, the two backups).
	AddExpectedError(TEXT("is corrupt"), EAutomationExpectedErrorFlags::Contains, 3, false);
	AddExpectedError(TEXT("and its backup are unreadable"), EAutomationExpectedErrorFlags::Contains, 1, false);
	const int32 A = W.Save->GetActiveSlot();
	const int32 Other = (A + 2) % UPawMainMenu::SlotCount;
	const FString Name = UPawSaveSubsystem::SlotName(A);
	auto Status = [&W]() { return S(W.Menu->GetStatusText()); };

	// 1. A newer build's save in the active slot, as at startup.
	W.PutNewer(A, 7);
	const TArray<uint8> NewerBytes = W.Bytes(Name);
	W.Game->NewGame();
	TestTrue(TEXT("newer: refused"), W.Save->LoadGame(A) == EPawLoadOutcome::NewerBuild);
	W.Menu->Refresh();
	TestEqual(TEXT("newer: the status says why"), Status(),
		FString::Printf(TEXT("Slot %d is from a newer version. Update the game to play it."), A + 1));
	TestFalse(TEXT("newer: Continue is off (a session there would never be saved)"), W.Menu->GetMenuButton(0)->GetIsEnabled());
	TestTrue(TEXT("newer: Load is on"), W.Menu->GetMenuButton(2)->GetIsEnabled());
	TestTrue(TEXT("newer: New Game has the selection"), W.Menu->GetTopDefault() == W.Menu->GetMenuButton(1));
	W.Menu->PressContinue();
	TestEqual(TEXT("newer: Continue plays nothing"), W.Plays, 0);
	W.Menu->PressLoad();
	TestEqual(TEXT("newer: its card"), S(W.Menu->GetSlotText(A)), FString(TEXT("Newer version\n\nUpdate the game to play this save.")));
	TestTrue(TEXT("newer: no Delete"), W.Menu->GetSlotDelete(A)->GetVisibility() == ESlateVisibility::Collapsed);
	W.Menu->PressSlot(A);
	TestEqual(TEXT("newer: its card plays nothing"), W.Plays, 0);
	TestTrue(TEXT("newer: the slots stay up"), W.Menu->GetTopLayer() == UPawMainMenu::ELayer::Slots);
	TestTrue(TEXT("newer: the file untouched"), W.Bytes(Name) == NewerBytes);
	// Another slot, empty: a new game there, and the title is plain again.
	W.Menu->PressSlot(Other);
	TestEqual(TEXT("newer: an empty slot plays"), W.Plays, 1);
	TestEqual(TEXT("newer: in that slot"), W.Save->GetActiveSlot(), Other);
	W.Save->Flush();
	TestTrue(TEXT("newer: still untouched"), W.Bytes(Name) == NewerBytes);
	W.Menu->Refresh();
	TestFalse(TEXT("newer: the notice is gone once a game is in a slot"), Status().StartsWith(TEXT("Slot ")));
	TestTrue(TEXT("newer: Continue is back"), W.Menu->GetMenuButton(0)->GetIsEnabled());

	// 2. The active slot and its backup unreadable (F47): Godot played a fresh game over it without a word.
	W.Wipe();
	W.Save->SaveGame(A); // A active again (the game in memory: nothing worth keeping)
	W.Save->Flush();
	W.PutJunk(A);
	W.Disk->Put(Name + TEXT("_bak"), TArray<uint8>({ 1, 2, 3 }));
	const TArray<uint8> JunkBytes = W.Bytes(Name);
	W.Game->NewGame();
	W.Plays = 0;
	TestTrue(TEXT("unreadable: refused"), W.Save->LoadGame(A) == EPawLoadOutcome::Corrupt);
	W.Menu->Refresh();
	TestEqual(TEXT("unreadable: the status says so"), Status(), FString::Printf(TEXT("Slot %d could not be read. Choose a slot."), A + 1));
	TestTrue(TEXT("unreadable: Continue is on"), W.Menu->GetMenuButton(0)->GetIsEnabled());
	W.Menu->PressContinue();
	TestEqual(TEXT("unreadable: Continue plays no fresh game over it"), W.Plays, 0);
	TestTrue(TEXT("unreadable: it opens the slots"), W.Menu->GetTopLayer() == UPawMainMenu::ELayer::Slots);
	TestTrue(TEXT("unreadable: on that slot's card"), W.Menu->GetTopDefault() == W.Menu->GetSlotCard(A));
	TestEqual(TEXT("unreadable: its card"), S(W.Menu->GetSlotText(A)), FString(TEXT("Cannot be read\n\nStart a new game here.")));
	TestTrue(TEXT("unreadable: no Delete"), W.Menu->GetSlotDelete(A)->GetVisibility() == ESlateVisibility::Collapsed);
	TestFalse(TEXT("unreadable: nothing kept aside yet"), W.Disk->Exists(Name + TEXT("_corrupt")));
	W.Menu->PressSlot(A);
	TestEqual(TEXT("unreadable: its card starts a game there"), W.Plays, 1);
	W.Save->Flush();
	TestTrue(TEXT("unreadable: the slot holds the new game"), W.Save->SlotState(A) == Pc::PcSlotState::Readable);
	TestTrue(TEXT("unreadable: the old file kept aside"), W.Bytes(Name + TEXT("_corrupt")) == JunkBytes);
	W.Menu->Refresh();
	TestFalse(TEXT("unreadable: the notice is gone"), Status().StartsWith(TEXT("Slot ")));

	// 3. A game loaded from the backup: said once (until the player plays).
	W.Wipe();
	W.PutSlot(A, 4, 40, 60.0, W.Clock - 30.0);
	W.Disk->Put(Name + TEXT("_bak"), W.Bytes(Name));
	W.PutJunk(A);
	W.Game->NewGame();
	W.Plays = 0;
	TestTrue(TEXT("backup: loaded from it"), W.Save->LoadGame(A) == EPawLoadOutcome::LoadedFromBackup);
	TestEqual(TEXT("backup: its game"), W.P().ReawakenCount, static_cast<int64_t>(4));
	W.Menu->Refresh();
	TestEqual(TEXT("backup: the status says so"), Status(), FString::Printf(TEXT("Slot %d was damaged. Its backup was loaded."), A + 1));
	TestTrue(TEXT("backup: Continue is on"), W.Menu->GetMenuButton(0)->GetIsEnabled());
	W.Menu->PressContinue();
	TestEqual(TEXT("backup: Continue plays it"), W.Plays, 1);
	W.Menu->Refresh();
	TestFalse(TEXT("backup: said once"), Status().StartsWith(TEXT("Slot ")));
	// The same when the startup recovery is what put the backup back (the slot broken before the game started).
	W.Wipe();
	W.PutSlot(A, 5, 50, 60.0, W.Clock - 30.0);
	W.Disk->Put(Name + TEXT("_bak"), W.Bytes(Name));
	W.PutJunk(A);
	W.Save->RecoverInterruptedSaves();
	TestTrue(TEXT("recovered: the slot reads again"), W.Save->SlotState(A) == Pc::PcSlotState::Readable);
	TestTrue(TEXT("recovered: the load says it came from the backup"), W.Save->LoadGame(A) == EPawLoadOutcome::LoadedFromBackup);
	W.Menu->Refresh();
	TestEqual(TEXT("recovered: the status says so"), Status(), FString::Printf(TEXT("Slot %d was damaged. Its backup was loaded."), A + 1));
	TestTrue(TEXT("recovered: a second load is plain"), W.Save->LoadGame(A) == EPawLoadOutcome::Loaded);
	// A broken slot whose backup reads shows the backup's game on its card, and its card loads it.
	W.Wipe();
	W.PutSlot(Other, 6, 60, 60.0, W.Clock - 30.0);
	W.Disk->Put(UPawSaveSubsystem::SlotName(Other) + TEXT("_bak"), W.Bytes(UPawSaveSubsystem::SlotName(Other)));
	W.PutJunk(Other);
	W.Plays = 0;
	W.Menu->Refresh();
	W.Menu->PressLoad();
	TestTrue(TEXT("backup card: the backup's game"), S(W.Menu->GetSlotText(Other)).StartsWith(TEXT("Reawaken 6\n")));
	TestTrue(TEXT("backup card: Delete offered"), W.Menu->GetSlotDelete(Other)->GetVisibility() != ESlateVisibility::Collapsed);
	W.Menu->PressSlot(Other);
	TestEqual(TEXT("backup card: it plays"), W.Plays, 1);
	TestEqual(TEXT("backup card: that game"), W.P().ReawakenCount, static_cast<int64_t>(6));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleSlotCatsTest, "Pawpocalypse.UI.Title.SlotCats", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// D18 (S4, the UI concept's slotCard): each save card shows the save's cat at its stage (the game's rule, read from the
// file: the ladder's bosses and the ending) on grass; an empty slot's well is card stock and holds no cat; a slot that
// cannot be read has the off well and no cat.
bool FPawTitleSlotCatsTest::RunTest(const FString& Parameters)
{
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const std::vector<std::string>& Ladder = W.Game->Data().Code.BossRealms;
	if (!TestTrue(TEXT("the boss ladder"), Ladder.size() >= 2))
	{
		W.Close();
		return false;
	}
	W.PutSlot(0, 1, 100, 600.0, W.Clock - 60.0, { Ladder[0] });                                       // one boss: stage 2
	W.PutSlot(2, 9, 9000, 9000.0, W.Clock - 60.0, std::set<std::string>(Ladder.begin(), Ladder.end()), true); // the end: 7
	W.PutSlot(3, 2, 50, 300.0, W.Clock - 60.0, { Ladder[1] });                                        // out of order: 1
	W.PutJunk(4);
	W.Menu->Refresh();
	W.Menu->PressLoad();
	TestTrue(TEXT("the slots are up"), W.Menu->IsLayerShown(UPawMainMenu::ELayer::Slots));
	TestEqual(TEXT("one boss beaten: the Armored Tomcat"), W.Menu->GetSlotCatName(0), FString(TEXT("T_ui_icon_slot_cat_2")));
	TestEqual(TEXT("the game finished: The Last Unicorn"), W.Menu->GetSlotCatName(2), FString(TEXT("T_ui_icon_slot_cat_7")));
	TestEqual(TEXT("a later boss alone: still the Tiny Kitten (the ladder in order)"), W.Menu->GetSlotCatName(3),
		FString(TEXT("T_ui_icon_slot_cat_1")));
	TestEqual(TEXT("a save stands on grass"), W.Menu->GetSlotPhotoColour(0), PawTokens::UI_PHOTO);
	TestEqual(TEXT("an empty slot: no cat"), W.Menu->GetSlotCatName(1), FString());
	TestEqual(TEXT("its well in card stock"), W.Menu->GetSlotPhotoColour(1), PawTokens::UI_PANEL_BAND);
	TestEqual(TEXT("unreadable: no cat"), W.Menu->GetSlotCatName(4), FString());
	TestEqual(TEXT("its well off"), W.Menu->GetSlotPhotoColour(4), PawTokens::UI_BUTTON_DISABLED);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleDeleteTest, "Pawpocalypse.UI.Title.Delete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Delete asks first, with the safe answer under the key; Cancel keeps the save, Yes removes it (and its backup).
bool FPawTitleDeleteTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.PutSlot(1, 2, 50, 75.0, W.Clock - 30.0);
	W.Disk->Put(UPawSaveSubsystem::SlotName(1) + TEXT("_bak"), TArray<uint8>({ 1, 2, 3 }));
	W.Menu->Refresh();
	W.Menu->PressNewGame();
	W.Menu->PressDelete(1);
	TestTrue(TEXT("the confirm is on top"), W.Menu->GetTopLayer() == UPawMainMenu::ELayer::Confirm);
	TestEqual(TEXT("it says what goes"), S(W.Menu->GetConfirmText()),
		FString(TEXT("Delete slot 2?\n\nReawaken 2, 1m 15s played.\nThis cannot be undone.")));
	TestTrue(TEXT("Cancel has the selection"), W.Menu->GetTopDefault() == W.Menu->GetConfirmNo());
	TestEqual(TEXT("Yes and Cancel are wired"), W.Menu->GetWired().Num(), 2);
	W.Menu->Back();
	TestTrue(TEXT("Back is Cancel: the slots again"), W.Menu->GetTopLayer() == UPawMainMenu::ELayer::Slots);
	TestTrue(TEXT("the save is kept"), W.Save->HasSlot(1));
	W.Menu->PressDelete(1);
	W.Menu->PressConfirmYes();
	TestFalse(TEXT("Yes deletes it"), W.Save->HasSlot(1));
	TestFalse(TEXT("and its backup"), W.Disk->Exists(UPawSaveSubsystem::SlotName(1) + TEXT("_bak")));
	TestEqual(TEXT("the card is empty"), S(W.Menu->GetSlotText(1)), FString(TEXT("Empty\n\nStart a new game here.")));
	TestTrue(TEXT("with no Delete"), W.Menu->GetSlotDelete(1)->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("back on the slots"), W.Menu->GetTopLayer() == UPawMainMenu::ELayer::Slots);
	// A second Yes after the confirm closed does nothing (one-shot).
	W.PutSlot(1, 2, 50, 75.0, W.Clock - 30.0);
	W.Menu->PressConfirmYes();
	TestTrue(TEXT("a stale Yes deletes nothing"), W.Save->HasSlot(1));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleBackTest, "Pawpocalypse.UI.Title.BackAndQuit", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Back closes the top layer; on the menu it asks to quit. Exit saves a game worth saving, then quits; the links open.
bool FPawTitleBackTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawMainMenu* M = W.Menu;
	M->PressCredits();
	TestTrue(TEXT("the credits"), M->GetTopLayer() == UPawMainMenu::ELayer::Credits);
	TestTrue(TEXT("Close has the selection"), M->GetTopDefault() == M->GetCloseButton(UPawMainMenu::ELayer::Credits));
	M->Back();
	TestTrue(TEXT("Back closes them"), M->GetTopLayer() == UPawMainMenu::ELayer::Menu);
	M->PressLeaderboard();
	TestEqual(TEXT("not cleared yet"), S(M->GetLeaderboardText()).EndsWith(TEXT("You have not cleared the final boss yet.")), true);
	M->Back();
	TestTrue(TEXT("and the leaderboard"), M->GetTopLayer() == UPawMainMenu::ELayer::Menu);
	M->PressNewGame();
	M->Back();
	TestTrue(TEXT("and the slots"), M->GetTopLayer() == UPawMainMenu::ELayer::Menu);
	M->Back();
	TestTrue(TEXT("on the menu, Back asks to quit"), M->GetTopLayer() == UPawMainMenu::ELayer::Confirm);
	TestEqual(TEXT("in Godot's words"), S(M->GetConfirmText()), FString(TEXT("Quit Pawpocalypse?")));
	M->PressConfirmNo();
	TestEqual(TEXT("Cancel stays"), W.Quits, 0);
	// Playtest 2026-11-26 (note 43): the Exit button asks first, as Back does.
	M->GetMenuButton(6)->OnClicked().Broadcast();
	TestTrue(TEXT("Exit asks before quitting"), M->GetTopLayer() == UPawMainMenu::ELayer::Confirm && W.Quits == 0);
	TestEqual(TEXT("the same question"), S(M->GetConfirmText()), FString(TEXT("Quit Pawpocalypse?")));
	M->PressConfirmNo();
	TestEqual(TEXT("Cancel stays"), W.Quits, 0);
	// Nothing to keep: Exit writes nothing.
	M->PressExit();
	TestEqual(TEXT("Exit quits"), W.Quits, 1);
	TestFalse(TEXT("an idle fresh game writes no slot"), W.Save->HasAnySave());
	// Something to keep: Back, Yes saves first.
	W.P().LifetimeCatnipEver = 10.0;
	M->Back();
	M->PressConfirmYes();
	TestEqual(TEXT("Yes quits"), W.Quits, 2);
	TestTrue(TEXT("after saving the game"), W.Save->HasSlot(W.Save->GetActiveSlot()));
	M->PressCluster(0);
	M->PressCluster(1);
	M->PressCluster(2);
	TestEqual(TEXT("two links opened"), W.Opened.Num(), 2);
	TestEqual(TEXT("the bug board"), W.Opened.IsValidIndex(0) ? W.Opened[0] : FString(), FString(UPawMainMenu::BugReportUrl));
	// Playtest 2026-11-26 (note 5): "the report a bug should redirect to steam discussions" (this game's App ID, 5720010).
	TestEqual(TEXT("the bug board is the game's Steam discussions"), FString(UPawMainMenu::BugReportUrl),
		FString(TEXT("https://steamcommunity.com/app/5720010/discussions/")));
	TestEqual(TEXT("the Discord"), W.Opened.IsValidIndex(1) ? W.Opened[1] : FString(), FString(UPawMainMenu::DiscordUrl));
	TestTrue(TEXT("Language opens its list, no link"), M->GetTopLayer() == UPawMainMenu::ELayer::Language);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleLanguageTest, "Pawpocalypse.UI.Title.Language", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The cluster's Language (phase 7; main_menu.gd _open_language_menu): the languages offered, each in its own name, the one
// shown ticked on the yellow plate; a layer whose rows have the keys while it is up, starting on the language shown; a row
// saves and shows its language and closes the list; Back, the button again and a press off the list close it; the
// screen's words follow the new language.
bool FPawTitleLanguageTest::RunTest(const FString& Parameters)
{
	using ELayer = UPawMainMenu::ELayer;
	FTitleWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSettingsSubsystem* Settings = W.GI->GetSubsystem<UPawSettingsSubsystem>();
	if (!TestNotNull(TEXT("the settings"), Settings))
	{
		W.Close();
		return false;
	}
	Settings->UseStorageForTest(W.Disk); // the setting is written to the sandbox, "system" to start with
	const FString Was = FInternationalization::Get().GetCurrentCulture()->GetName();
	UPawMainMenu* M = W.Menu;

	// Only complete languages are offered (2026-11-20: English and German). The button toggles the list.
	M->PressCluster(2);
	TestTrue(TEXT("the list is up, on top"), M->GetTopLayer() == ELayer::Language);
	TestEqual(TEXT("complete languages only"), FString::Join(M->GetLanguageCodes(), TEXT(",")), FString::Join(PawText::Languages(false), TEXT(",")));
	M->PressCluster(2);
	TestTrue(TEXT("the button again closes it"), M->GetTopLayer() == ELayer::Menu);

	// The review's list (-PawDraftLanguages): every language with a line in use, in the picker's order.
	PawText::SetShowUnfinishedLanguagesForTest(true);
	M->PressCluster(2);
	const TArray<FString> Codes = M->GetLanguageCodes();
	TestEqual(TEXT("English and the 22 translated"), Codes.Num(), 23);
	TestEqual(TEXT("in the picker's order"), FString::Join(Codes, TEXT(",")), FString::Join(PawText::Languages(true), TEXT(",")));
	const TArray<TObjectPtr<UPawButton>>& Rows = M->GetLanguageRows();
	TestEqual(TEXT("one row each"), Rows.Num(), Codes.Num());
	const int32 En = Codes.IndexOfByKey(TEXT("en")), De = Codes.IndexOfByKey(TEXT("de"));
	if (!TestTrue(TEXT("English and German listed"), Rows.IsValidIndex(En) && Rows.IsValidIndex(De)))
	{
		PawText::SetShowUnfinishedLanguagesForTest(false);
		Settings->UseStorageForTest(nullptr);
		W.Close();
		return false;
	}
	TestEqual(TEXT("each in its own name, no (draft) (D20)"), S(Rows[De]->GetLabel()), FString(TEXT("Deutsch")));
	TestEqual(TEXT("English"), S(Rows[En]->GetLabel()), FString(TEXT("English")));
	TestTrue(TEXT("the language shown: the yellow plate and the tick"), Rows[En]->HasActiveLook() && Rows[En]->GetIconName() == TEXT("ui_tick"));
	TestTrue(TEXT("the others: neither"), !Rows[De]->HasActiveLook() && Rows[De]->GetIconName().IsEmpty());
	TestTrue(TEXT("the keys start on the language shown"), M->GetTopDefault() == Rows[En]);
	TestEqual(TEXT("its rows are the keys' rows"), M->GetWired().Num(), Codes.Num());
	M->Back();
	TestTrue(TEXT("Back closes it"), M->GetTopLayer() == ELayer::Menu);

	// A press off the list closes it (Godot's scrim); with the list down it is not the list's.
	M->PressCluster(2);
	TestTrue(TEXT("a press off the card closes it"), M->PressOffLanguageMenu(FVector2D(-5000.0, -5000.0)) && M->GetTopLayer() == ELayer::Menu);
	TestFalse(TEXT("with the list down, a press is the screen's"), M->PressOffLanguageMenu(FVector2D(-5000.0, -5000.0)));

	// A row: the language saved and shown, the list closed, the screen's words in it. (The editor never shows a language
	// itself: the test shows German as the game would.)
	const FString EnglishHint = M->GetHintText();
	PawText::ShowCulture(TEXT("de"));
	M->PressCluster(2);
	M->PressLanguage(TEXT("de"));
	Pc::PcSettings File;
	TArray<uint8> Bytes;
	TestTrue(TEXT("saved: de"), W.Disk->Read(UPawSettingsSubsystem::EntryName, Bytes)
		&& Pc::SettingsLoad(std::string(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<size_t>(Bytes.Num())), File)
		&& File.Language == "de");
	TestEqual(TEXT("applied: German"), Settings->GetLanguageApplied().Culture, FString(TEXT("de")));
	TestTrue(TEXT("the list closed"), M->GetTopLayer() == ELayer::Menu);
	TestTrue(TEXT("the hint made again, in German"), M->GetHintText() != EnglishHint && M->GetHintText().StartsWith(TEXT("Im Spiel:")));
	TestEqual(TEXT("the status line too"), S(M->GetStatusText()), FString(TEXT("Noch kein Spielstand. Beginne ein neues Spiel.")));
	M->PressCluster(2);
	TestTrue(TEXT("reopened: German is the one shown"), M->GetLanguageRows().IsValidIndex(De) && M->GetLanguageRows()[De]->HasActiveLook()
		&& M->GetTopDefault() == M->GetLanguageRows()[De]);
	M->PressLanguage(TEXT("en"));
	PawText::ShowCulture(Was);

	PawText::SetShowUnfinishedLanguagesForTest(false);
	Settings->UseStorageForTest(nullptr);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawTitleSwellTest, "Pawpocalypse.UI.Title.SlotSwell", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawTitleSwellTest::RunTest(const FString& Parameters)
{
	// Playtest 2026-11-26 (note 6): "slot 1 is large. the rest increase in size as you hover but slot 1 is always large". The
	// slots open with focus on the active slot's card, and a mouse never moves focus: the swell must read focus only as the
	// ring does (the keys and a pad), never the focus a mouse player cannot see.
	TestTrue(TEXT("the hovered card swells"), UPawMainMenu::SlotSwells(true, false, true));
	TestFalse(TEXT("the slots' opening focus does not swell slot 1 under a mouse"), UPawMainMenu::SlotSwells(false, true, true));
	TestTrue(TEXT("the keys' or a pad's selection swells"), UPawMainMenu::SlotSwells(false, true, false));
	TestFalse(TEXT("neither: flat"), UPawMainMenu::SlotSwells(false, false, false));
	return true;
}

#endif
