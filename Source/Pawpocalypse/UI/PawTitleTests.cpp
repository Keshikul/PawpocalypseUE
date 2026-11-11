// Pawpocalypse.UI.Title.*: the title screen (UPawMainMenu) on a sandboxed save (memory storage). The rules are proven against
// the real main_menu.tscn in PawCore (PcTitleTests, fixture `title`); these prove the screen says them in Godot's words
// (the string tables, the line breaks), and the flows: a new game, a load, a delete, quitting, Back, the links.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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

		void PutSlot(int32 Slot, int64 Reawakens, int64 Kills, double Play, double Unix)
		{
			Pc::PcSaveState S;
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
	TestTrue(TEXT("the version line"), S(M->GetVersionText()).StartsWith(TEXT("v")) && S(M->GetVersionText()).Len() > 1);
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
	TestFalse(TEXT("Language waits for the languages"), M->GetClusterButton(2)->GetIsEnabled());
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
			const UWidget* Box = Name->GetParent();
			TestTrue(Where + TEXT(": the logo tilted"), Box && FMath::IsNearlyEqual(Box->GetRenderTransform().Angle, static_cast<float>(PawUi::R4NameAngle)));
			const UBorderSlot* InBox = Cast<UBorderSlot>(Name->Slot);
			TestTrue(Where + TEXT(": the logo starts at its box's left edge (graft slot:NameText)"), InBox && InBox->GetHorizontalAlignment() == HAlign_Left);
			const UCanvasPanelSlot* S = CanvasSlotOf(const_cast<UTextBlock*>(Name));
			TestTrue(Where + TEXT(": the logo at the top left"), S && S->GetAnchors().Minimum == FVector2D(0.0, 0.0)
				&& FMath::IsNearlyEqual(S->GetOffsets().Left, static_cast<float>(PawUi::R4TitleMargin)));
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
	TestEqual(TEXT("the Discord"), W.Opened.IsValidIndex(1) ? W.Opened[1] : FString(), FString(UPawMainMenu::DiscordUrl));
	W.Close();
	return true;
}

#endif
