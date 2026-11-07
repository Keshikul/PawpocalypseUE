// Pawpocalypse.UI.Pause.*: the pause menu (UPawPauseMenu, what ships: WBP_PauseMenu) on a sandboxed save (memory storage),
// over a running realm and the REAL root layout. Its rules are PcPause, proven against the real pause_menu.tscn in PawCore
// (PcPauseTests, fixture `pause`); these prove the screen says them in Godot's words, holds the field, saves, and leaves.
//
// Pawpocalypse.UI.Pause.MatchesGodot  300 fixture rows, every press on the widget: the page, the status line, the quit
//                                     button's caption, the discard, the five slot captions (read back from the slot files
//                                     the menu wrote), the field held while it is open.
// Pawpocalypse.UI.Pause.Look          every part wired; Godot's words; Settings live; what is wired.
// Pawpocalypse.UI.Pause.Opens         the HUD opens it (Menu stack, the field held) only when nothing else is up or holds
//                                     the field, and not on the frame after something else let go; Continue resumes.
// Pawpocalypse.UI.Pause.Leaves        Back to Main Menu arms, its second press saves and leaves; the discard leaves without
//                                     writing; closing disarms (reopened, one press never leaves).
// Pawpocalypse.UI.Pause.BackKeys      back on the Save page returns to the main page; pause (Start) closes from either.
// Pawpocalypse.UI.Pause.Overwrite     an occupied slot asks first (the file untouched), the second press writes it.
//
// Negative tests: tools/sabotage_realm.py (`pause_*`).

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcProgress.h"
#include "PcSave.h"
#include "PcStats.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include <cstdlib>
#include <string>

namespace
{
	struct FPauseWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawSaveSubsystem* Save = nullptr;
		UPawRootLayout* Root = nullptr;
		UPawHudScreen* Hud = nullptr;
		TSharedPtr<SWidget> RootSlate;
		TSharedPtr<FPawMemorySaveStorage> Disk;
		int32 Leaves = 0;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			Save = GI->GetSubsystem<UPawSaveSubsystem>();
			if (!T.TestNotNull(TEXT("the realm"), Realm) || !T.TestNotNull(TEXT("the game"), Game) || !T.TestNotNull(TEXT("the saves"), Save)
				|| !T.TestTrue(TEXT("data"), Game->IsDataReady()))
			{
				return false;
			}
			Disk = MakeShared<FPawMemorySaveStorage>();
			Save->UseStorageForTest(Disk);
			Game->NewGame();
			Realm->SetViewSize(1920.0, 1080.0);
			if (!T.TestTrue(TEXT("the realm starts"), Realm->StartRealm(77)))
			{
				return false;
			}
			Root = CreateWidget<UPawRootLayout>(GI, PawUi::Resolve<UPawRootLayout>());
			Hud = CreateWidget<UPawHudScreen>(GI, PawUi::Resolve<UPawHudScreen>());
			if (!T.TestNotNull(TEXT("root"), Root) || !T.TestNotNull(TEXT("HUD"), Hud))
			{
				return false;
			}
			RootSlate = Root->TakeWidget();
			Hud->SetMenuStack(Root->MenuStack);
			Hud->SetModalStack(Root->ModalStack);
			GFrameCounter += 2; // clear of any earlier test's frame
			return true;
		}

		// A pause menu of its own, outside the stacks (one widget for a whole replay, as in the fixture).
		UPawPauseMenu* Standalone(FAutomationTestBase& T)
		{
			UPawPauseMenu* M = CreateWidget<UPawPauseMenu>(GI, PawUi::Resolve<UPawPauseMenu>());
			if (T.TestNotNull(TEXT("the pause menu"), M))
			{
				M->OnLeave = [this]() { ++Leaves; };
			}
			return M;
		}

		// The HUD's way in: Esc / Start.
		UPawPauseMenu* FromHud(FAutomationTestBase& T)
		{
			if (!T.TestTrue(TEXT("the HUD opens the pause menu"), Hud->OpenPauseMenu()))
			{
				return nullptr;
			}
			UPawPauseMenu* M = Hud->GetPauseMenu();
			if (T.TestTrue(TEXT("up and active"), M && M->IsActivated()))
			{
				M->OnLeave = [this]() { ++Leaves; };
				return M;
			}
			return nullptr;
		}

		void Close()
		{
			RootSlate.Reset();
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

		void PutSlot(int32 Slot, int64 Reawakens, int64 Kills, double Play)
		{
			Pc::PcSaveState S;
			S.ReawakenCount = Reawakens;
			S.LifetimeKills = Kills;
			S.TotalPlaySeconds = Play;
			S.LastPlayedUnix = 1.8e9;
			S.LifetimeCatnipEver = 10.0;
			const std::string File = Pc::WriteSaveFile(S, "test");
			Disk->Put(UPawSaveSubsystem::SlotName(Slot), TArray<uint8>(reinterpret_cast<const uint8*>(File.data()), static_cast<int32>(File.size())));
		}

		void PutJunk(int32 Slot)
		{
			const char Junk[] = "not a save";
			Disk->Put(UPawSaveSubsystem::SlotName(Slot), TArray<uint8>(reinterpret_cast<const uint8*>(Junk), 10));
		}

		// The slot files as listed (one character per slot), the active slot set without leaving a file behind.
		void SetSlots(const FString& Present, const TSet<int32>& Junk, int32 Active)
		{
			for (int32 I = 0; I < UPawSaveSubsystem::SlotCount; ++I)
			{
				Save->DeleteSlot(I);
			}
			Save->SaveGame(Active);
			Save->Flush();
			Save->DeleteSlot(Active);
			for (const TCHAR C : Present)
			{
				const int32 I = C - TEXT('0');
				if (Junk.Contains(I))
				{
					PutJunk(I);
				}
				else
				{
					PutSlot(I, 1, 1, 1.0);
				}
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

	FString Label(const UPawButton* B) { return B ? S(B->GetLabel()) : FString(TEXT("<missing>")); }
	bool Shows(const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden; }

	const FString Dash = FString(TEXT(" ")) + TCHAR(0x2014) + TEXT(" ");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPauseGodotTest, "Pawpocalypse.UI.Pause.MatchesGodot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// The real pause_menu.tscn's words, press for press (Tests/Data/pause_godot.tsv). One menu for every row, as in the fixture,
// so a caption not refreshed since an earlier row is stale in both. A slot file written before the row holds numbers the
// fixture does not record: its line is checked up to them; every slot the menu itself wrote is checked whole.
bool FPawPauseGodotTest::RunTest(const FString& Parameters)
{
	FString Text;
	const FString Path = FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/pause_godot.tsv");
	if (!TestTrue(TEXT("the fixture"), FFileHelper::LoadFileToString(Text, *Path)))
	{
		return false;
	}
	FPauseWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawPauseMenu* M = W.Standalone(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	// The rows, grouped.
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	TArray<FString> Header;
	TArray<TArray<TMap<FString, FString>>> Rows;
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
		TMap<FString, FString> V;
		for (int32 I = 0; I < Header.Num() && I < Cells.Num(); ++I)
		{
			V.Add(Header[I], Cells[I]);
		}
		if (Rows.Num() == 0 || Rows.Last()[0][TEXT("row")] != V[TEXT("row")])
		{
			if (Rows.Num() == 300)
			{
				break;
			}
			Rows.AddDefaulted();
		}
		Rows.Last().Add(MoveTemp(V));
	}
	if (!TestEqual(TEXT("rows"), Rows.Num(), 300))
	{
		W.Close();
		return false;
	}
	int32 Presses = 0, Mismatch = 0, Saved = 0, Armed = 0, Whole = 0;
	bool Exact[UPawPauseMenu::SlotCount] = { true, true, true, true, true };
	for (const TArray<TMap<FString, FString>>& Row : Rows)
	{
		const TMap<FString, FString>& First = Row[0];
		const FString Present = First[TEXT("present")];
		// A file Godot could not read: present, captioned empty on a fresh refresh before the menu wrote it.
		TSet<int32> Junk;
		{
			TSet<int32> Written;
			for (int32 L = 1; L < Row.Num(); ++L)
			{
				const TMap<FString, FString>& V = Row[L];
				if (V[TEXT("op")] == TEXT("slot") && V[TEXT("status")].StartsWith(TEXT("Saved")))
				{
					Written.Add(FCString::Atoi(*V[TEXT("arg")]));
				}
				// The captions are fresh after opening the Save page and after arming an overwrite.
				const bool bArmed = V[TEXT("op")] == TEXT("slot") && V[TEXT("pending")] != TEXT("-1");
				if (V[TEXT("op")] != TEXT("slots") && !bArmed)
				{
					continue;
				}
				for (int32 I = 0; I < UPawPauseMenu::SlotCount; ++I)
				{
					if (!Written.Contains(I) && Present.Contains(FString::FromInt(I)) && V[FString::Printf(TEXT("s%d"), I)].Contains(Dash + TEXT("empty")))
					{
						Junk.Add(I);
					}
				}
			}
		}
		if (M->IsActivated())
		{
			M->PressContinue();
		}
		W.SetSlots(Present, Junk, FCString::Atoi(*First[TEXT("active")]));
		W.P().ReawakenCount = FCString::Atoi64(*First[TEXT("game_reawakens")]);
		W.P().LifetimeKills = FCString::Atoi64(*First[TEXT("game_kills")]);
		W.P().TotalPlaySeconds = std::strtod(TCHAR_TO_UTF8(*First[TEXT("game_play")]), nullptr);
		W.P().LifetimeCatnipEver = 50.0;
		TSet<int32> SavedInRow;
		M->ActivateWidget();
		for (int32 L = 0; L < Row.Num(); ++L)
		{
			const TMap<FString, FString>& V = Row[L];
			const FString Op = V[TEXT("op")];
			if (L > 0)
			{
				++Presses;
				const int32 Arg = FCString::Atoi(*V[TEXT("arg")]);
				if (Op == TEXT("open")) { M->ActivateWidget(); }
				else if (Op == TEXT("close")) { M->PressContinue(); }
				else if (Op == TEXT("slots")) { M->PressSave(); }
				else if (Op == TEXT("main")) { M->PressSlotsBack(); }
				else if (Op == TEXT("quit1")) { M->PressMainMenu(); }
				else if (Op == TEXT("slot"))
				{
					M->PressSlot(Arg);
					if (M->GetState().PendingOverwrite >= 0)
					{
						++Armed;
					}
					else
					{
						++Saved;
						SavedInRow.Add(Arg);
					}
				}
				// A refresh: what each caption now says is whole unless it is a file from before the row.
				if (Op == TEXT("slots") || (Op == TEXT("slot") && M->GetState().PendingOverwrite >= 0))
				{
					for (int32 I = 0; I < UPawPauseMenu::SlotCount; ++I)
					{
						Exact[I] = SavedInRow.Contains(I) || !Present.Contains(FString::FromInt(I)) || Junk.Contains(I);
					}
				}
			}
			auto Same = [&](const TCHAR* What, const FString& Got, const FString& Want)
			{
				if (Got != Want && ++Mismatch <= 8)
				{
					AddError(FString::Printf(TEXT("row %s line %d (%s) %s: '%s' vs Godot '%s'"), *V[TEXT("row")], L, *Op, What, *Got, *Want));
				}
			};
			auto Flag = [](bool b) { return FString(b ? TEXT("1") : TEXT("0")); };
			Same(TEXT("open"), Flag(M->IsActivated()), V[TEXT("open")]);
			Same(TEXT("paused"), Flag(W.Realm->IsPaused()), V[TEXT("paused")]);
			Same(TEXT("main page"), Flag(M->IsPageShown(false)), V[TEXT("main_page")]);
			Same(TEXT("slot page"), Flag(M->IsPageShown(true)), V[TEXT("slot_page")]);
			Same(TEXT("status"), S(M->GetStatusText()), Unescape(V[TEXT("status")]));
			Same(TEXT("quit caption"), Label(M->GetButton(TEXT("MainMenuButton"))), Unescape(V[TEXT("quit_caption")]));
			Same(TEXT("discard"), Flag(Shows(M->GetButton(TEXT("DiscardButton")))), V[TEXT("discard")]);
			Same(TEXT("pending"), FString::Printf(TEXT("%lld"), static_cast<long long>(M->GetState().PendingOverwrite)), V[TEXT("pending")]);
			Same(TEXT("exit pending"), Flag(M->GetState().bExitPending), V[TEXT("exit_pending")]);
			Same(TEXT("active"), FString::FromInt(W.Save->GetActiveSlot()), V[TEXT("active")]);
			FString Have;
			for (int32 I = 0; I < UPawPauseMenu::SlotCount; ++I)
			{
				Have += W.Save->HasSlot(I) ? FString::FromInt(I) : FString();
			}
			Same(TEXT("slot files"), Have, V[TEXT("present")]);
			for (int32 I = 0; I < UPawPauseMenu::SlotCount; ++I)
			{
				const FString Want = Unescape(V[FString::Printf(TEXT("s%d"), I)]);
				const FString Got = Label(M->GetSlotButton(I));
				const FString Mark = Dash + TEXT("Reawaken ");
				const int32 At = Want.Find(Mark);
				if (!Exact[I] && At != INDEX_NONE)
				{
					Same(TEXT("slot line (to its numbers)"), Got.Left(At + Mark.Len()), Want.Left(At + Mark.Len()));
				}
				else
				{
					Whole += Want.Contains(Mark) ? 1 : 0;
					Same(TEXT("slot line"), Got, Want);
				}
			}
		}
	}
	TestEqual(TEXT("every line as Godot's"), Mismatch, 0);
	TestTrue(TEXT("overwrites armed"), Armed > 100);
	TestTrue(TEXT("saves written"), Saved > 100);
	TestTrue(TEXT("summary lines checked whole (the menu's own saves)"), Whole > 100);
	TestEqual(TEXT("never left"), W.Leaves, 0);
	AddInfo(FString::Printf(TEXT("%d rows, %d presses, %d armed, %d saved, %d whole summary lines"), Rows.Num(), Presses, Armed, Saved, Whole));
	if (M->IsActivated())
	{
		M->PressContinue();
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPauseLookTest, "Pawpocalypse.UI.Pause.Look", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPauseLookTest::RunTest(const FString& Parameters)
{
	FPauseWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawPauseMenu* M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	// Wired in what ships: every part found.
	const TCHAR* Names[] = { TEXT("ContinueButton"), TEXT("SaveButton"), TEXT("SettingsButton"), TEXT("MainMenuButton"), TEXT("DiscardButton"),
		TEXT("SlotBackButton") };
	const TCHAR* Words[] = { TEXT("Continue"), TEXT("Save"), TEXT("Settings"), TEXT("Back to Main Menu"), TEXT("Quit WITHOUT saving"), TEXT("Back") };
	for (int32 I = 0; I < 6; ++I)
	{
		if (!TestNotNull(FString::Printf(TEXT("%s is wired"), Names[I]), M->GetButton(Names[I])))
		{
			W.Close();
			return false;
		}
		TestEqual(TEXT("in Godot's words"), Label(M->GetButton(Names[I])), FString(Words[I]));
	}
	for (int32 I = 0; I < UPawPauseMenu::SlotCount; ++I)
	{
		TestNotNull(TEXT("a slot button is wired"), M->GetSlotButton(I));
	}
	TestEqual(TEXT("the title"), S(M->GetTitleText()), FString(TEXT("PAUSED")));
	TestTrue(TEXT("the main page"), M->IsPageShown(false) && !M->IsPageShown(true));
	TestTrue(TEXT("Settings opens the settings dialog"), M->GetButton(TEXT("SettingsButton"))->GetIsEnabled());
	TestFalse(TEXT("the discard hides until the quit is armed"), Shows(M->GetButton(TEXT("DiscardButton"))));
	TestEqual(TEXT("no status yet"), S(M->GetStatusText()), FString());
	TestEqual(TEXT("the main page: Continue, Save, Settings, Back to Main Menu"), M->GetWired().Num(), 4);
	TestTrue(TEXT("the selection opens on Continue"), M->GetDesiredFocusTarget() == M->GetButton(TEXT("ContinueButton")));
	M->PressMainMenu();
	TestEqual(TEXT("armed: the discard joins"), M->GetWired().Num(), 5);
	M->PressSave();
	TestTrue(TEXT("the Save page"), M->IsPageShown(true) && !M->IsPageShown(false));
	TestEqual(TEXT("five slots and Back"), M->GetWired().Num(), 6);
	TestEqual(TEXT("an empty slot, the active one"), Label(M->GetSlotButton(0)), FString(TEXT("Slot 1")) + Dash + TEXT("empty (current)"));
	TestEqual(TEXT("an empty slot"), Label(M->GetSlotButton(3)), FString(TEXT("Slot 4")) + Dash + TEXT("empty"));
	M->PressContinue();
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPauseOpensTest, "Pawpocalypse.UI.Pause.Opens", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPauseOpensTest::RunTest(const FString& Parameters)
{
	FPauseWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("the field runs"), W.Realm->IsPaused());
	UPawPauseMenu* M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("on the Menu stack"), W.Root->MenuStack->GetActiveWidget() == M);
	TestTrue(TEXT("the field holds"), W.Realm->IsPaused());
	TestFalse(TEXT("never a second one"), W.Hud->OpenPauseMenu());
	M->PressContinue();
	TestFalse(TEXT("Continue closes it"), M->IsActivated());
	TestFalse(TEXT("and the field runs"), W.Realm->IsPaused());
	W.Hud->Pump(1.0 / 60.0);
	// Something else holds the field: no pause menu over it.
	W.Realm->SetPaused(true);
	TestFalse(TEXT("not while another holds the field"), W.Hud->OpenPauseMenu());
	W.Realm->SetPaused(false);
	// A screen up that does NOT hold the field (every real one does, so the hold alone would hide a missing stack check):
	// never a pause menu stacked over it, on the Menu stack or the Modal one.
	for (UCommonActivatableWidgetStack* Stack : { W.Root->MenuStack.Get(), W.Root->ModalStack.Get() })
	{
		UCommonActivatableWidget* Other = Stack->AddWidget<UCommonActivatableWidget>(UCommonActivatableWidget::StaticClass());
		if (TestTrue(TEXT("a screen that does not hold the field"), Other && Other->IsActivated() && !W.Realm->IsPaused()))
		{
			TestFalse(TEXT("not over another screen"), W.Hud->OpenPauseMenu());
			Other->DeactivateWidget();
		}
		W.Hud->Pump(1.0 / 60.0);
		GFrameCounter += 2;
	}
	// The skill tree up: no. The Esc that closes it must not open this on the same or the next frame.
	if (TestTrue(TEXT("the tree opens"), W.Hud->OpenSkillTree(false)))
	{
		TestFalse(TEXT("not over the tree"), W.Hud->OpenPauseMenu());
		W.Hud->Pump(1.0 / 60.0);
		W.Hud->GetSkillTree()->DeactivateWidget();
		TestFalse(TEXT("the tree is gone"), W.Hud->IsMenuUp());
		TestFalse(TEXT("not on the frame the tree closed"), W.Hud->OpenPauseMenu());
		GFrameCounter += 1;
		TestFalse(TEXT("nor the next"), W.Hud->OpenPauseMenu());
		GFrameCounter += 1;
		TestTrue(TEXT("then it opens"), W.Hud->OpenPauseMenu());
		if (UPawPauseMenu* Again = W.Hud->GetPauseMenu())
		{
			TestTrue(TEXT("reopened on the main page, the status clear"), Again->IsPageShown(false) && S(Again->GetStatusText()).IsEmpty());
			Again->PressBackOrPause(false);
			TestFalse(TEXT("Start closes it"), Again->IsActivated());
		}
	}
	TestFalse(TEXT("the field runs at the end"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPauseLeavesTest, "Pawpocalypse.UI.Pause.Leaves", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPauseLeavesTest::RunTest(const FString& Parameters)
{
	FPauseWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().ReawakenCount = 6;
	W.P().LifetimeKills = 4321;
	W.P().LifetimeCatnipEver = 50.0;
	UPawPauseMenu* M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	// The first press arms; nothing is written, nothing leaves.
	M->PressMainMenu();
	TestEqual(TEXT("armed: the caption"), Label(M->GetButton(TEXT("MainMenuButton"))), FString(TEXT("Save and quit to menu")));
	TestTrue(TEXT("the discard shows"), Shows(M->GetButton(TEXT("DiscardButton"))));
	TestEqual(TEXT("no leaving yet"), W.Leaves, 0);
	TestFalse(TEXT("nothing written"), W.Save->HasAnySave());
	// Closing disarms: reopened, a single press only arms again.
	M->PressContinue();
	W.Hud->Pump(1.0 / 60.0);
	GFrameCounter += 2;
	M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	TestEqual(TEXT("disarmed on reopening"), Label(M->GetButton(TEXT("MainMenuButton"))), FString(TEXT("Back to Main Menu")));
	TestFalse(TEXT("no discard"), Shows(M->GetButton(TEXT("DiscardButton"))));
	M->PressMainMenu();
	TestEqual(TEXT("one press never leaves"), W.Leaves, 0);
	// The discard: leaves, writes nothing.
	M->PressDiscard();
	TestEqual(TEXT("the discard leaves"), W.Leaves, 1);
	TestFalse(TEXT("without writing"), W.Save->HasAnySave());
	TestFalse(TEXT("closed"), M->IsActivated());
	TestFalse(TEXT("the field let go"), W.Realm->IsPaused());
	TestFalse(TEXT("disarmed"), M->GetState().bExitPending);
	// Save and quit: the second press writes the active slot, then leaves.
	W.Hud->Pump(1.0 / 60.0);
	GFrameCounter += 2;
	M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	M->PressMainMenu();
	M->PressMainMenu();
	TestEqual(TEXT("the second press leaves"), W.Leaves, 2);
	Pc::PcSlotSummary Summary;
	if (TestTrue(TEXT("after writing the active slot"), W.Save->ReadSlotSummary(W.Save->GetActiveSlot(), Summary)))
	{
		TestEqual(TEXT("this game"), Summary.Kills, static_cast<int64_t>(4321));
	}
	TestFalse(TEXT("closed"), M->IsActivated());
	TestFalse(TEXT("the field let go"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPauseBackTest, "Pawpocalypse.UI.Pause.BackKeys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPauseBackTest::RunTest(const FString& Parameters)
{
	FPauseWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawPauseMenu* M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	M->PressSave();
	M->PressBackOrPause(true);
	TestTrue(TEXT("back on the Save page: the main page"), M->IsActivated() && M->IsPageShown(false));
	TestTrue(TEXT("the field still held"), W.Realm->IsPaused());
	M->PressBackOrPause(true);
	TestFalse(TEXT("back on the main page closes"), M->IsActivated());
	TestFalse(TEXT("the field runs"), W.Realm->IsPaused());
	W.Hud->Pump(1.0 / 60.0);
	GFrameCounter += 2;
	M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	M->PressSave();
	M->PressBackOrPause(false);
	TestFalse(TEXT("Start closes from the Save page"), M->IsActivated());
	TestFalse(TEXT("the field runs"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawPauseOverwriteTest, "Pawpocalypse.UI.Pause.Overwrite", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawPauseOverwriteTest::RunTest(const FString& Parameters)
{
	FPauseWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.PutSlot(2, 9, 12, 75.0);
	TArray<uint8> Before;
	W.Disk->Read(UPawSaveSubsystem::SlotName(2), Before);
	W.P().ReawakenCount = 3;
	W.P().LifetimeKills = 2500000;
	W.P().TotalPlaySeconds = 3700.0;
	W.P().LifetimeCatnipEver = 50.0;
	UPawPauseMenu* M = W.FromHud(*this);
	if (!M)
	{
		W.Close();
		return false;
	}
	M->PressSave();
	TestEqual(TEXT("the file's line"), Label(M->GetSlotButton(2)), FString(TEXT("Slot 3")) + Dash + TEXT("Reawaken 9 · 1m 15s · 12 mice"));
	M->PressSlot(2);
	TestEqual(TEXT("an occupied slot asks"), Label(M->GetSlotButton(2)), FString(TEXT("Slot 3")) + Dash + TEXT("OVERWRITE? press again"));
	TestEqual(TEXT("the status says so"), S(M->GetStatusText()), FString(TEXT("Slot 3 already has a game. Press it again to overwrite.")));
	TestTrue(TEXT("still the Save page"), M->IsPageShown(true));
	TArray<uint8> Mid;
	W.Disk->Read(UPawSaveSubsystem::SlotName(2), Mid);
	TestTrue(TEXT("the file untouched"), Before == Mid);
	M->PressSlot(2);
	TestEqual(TEXT("the second press writes"), S(M->GetStatusText()), FString(TEXT("Saved to slot 3.")));
	TestTrue(TEXT("back on the main page"), M->IsPageShown(false));
	TestEqual(TEXT("slot 3 is active"), W.Save->GetActiveSlot(), 2);
	M->PressSave();
	TestEqual(TEXT("this game's line"), Label(M->GetSlotButton(2)), FString(TEXT("Slot 3 (current)")) + Dash + TEXT("Reawaken 3 · 1h 01m · 2.5M mice"));
	// An empty slot saves at once.
	M->PressSlot(4);
	TestEqual(TEXT("an empty slot: no asking"), S(M->GetStatusText()), FString(TEXT("Saved to slot 5.")));
	TestTrue(TEXT("written"), W.Save->HasSlot(4));
	M->PressContinue();
	TestEqual(TEXT("never left"), W.Leaves, 0);
	W.Close();
	return true;
}

#endif
