// Pawpocalypse.UI.Layout.*: the screens fit (R4h, 2026-11-08; WORKLIST-RESTYLE R4's exit: "no overflow at 150 %").
// Each screen is laid out headless (PawLayout::Arrange: prepass, paint, which ticks, then arrange) at 1920 x 1080, at
// 1920 x 1080 with 150 % UI size (1280 x 720 of layout) and at 1280 x 800 (the Deck). Every painted part lies on the screen
// (a part a scroll box clips is judged by what shows), and the parts that must never cover each other do not.
//
// Pawpocalypse.UI.Layout.Title     the logo clear of the menu; Boba's card, the version, the cluster, the hint on screen
// Pawpocalypse.UI.Layout.Dock      the Upgrades page: no shop row shows under the quick bar or the legend
// Pawpocalypse.UI.Layout.Settings  each section: nothing on the page shows under Close; the window on screen
// Pawpocalypse.UI.Layout.Tree      the tree (every node) on screen and centred in the content area
// Pawpocalypse.UI.Layout.Languages every screen with words, in English and every translation, at 1280 x 800 and 150 % UI:
//                                  no text wider than its box, cut by a box that clips, or off the screen (phase 7)
// Pawpocalypse.UI.Layout.PseudoLocale  the same in the pseudo-locale (every line accented and ~40 % longer)
// Pawpocalypse.UI.Layout.TextMisses    the detector sees what it must (a text too wide, a line cut) and passes what fits
//
// Every run writes what it measured to Saved/Layout/<screen>_<W>x<H>_<scale>.txt. Sabotages: tools/sabotage_realm.py
// (`layout_*`, `shop_not_scrolling`, `settings_page_spills`, `tree_fit_stale_size`).

#include "Audio/PawAudioSubsystem.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcSettings.h"
#include "PcCheats.h"
#include "PcEconomy.h"
#include "PcData.h"
#include "PcField.h"
#include "PcHud.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/RichTextBlock.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TextWidgetTypes.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "PcSave.h"
#include "UI/PawButton.h"
#include "UI/PawCardPickScreen.h"
#include "UI/PawHudScreen.h"
#include "UI/PawJobPicker.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawVictoryScreen.h"
#include "UI/PawLayoutCheck.h"
#include "UI/PawMainMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawText.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FSize
	{
		FVector2D Screen;
		float UiSize;
		const TCHAR* Name;
		float Scale() const { return PawLayout::GameScale(Screen, UiSize); } // the DPI curve (800 tall: 0.74) times the UI size
	};
	const FSize Sizes[] = {
		{ FVector2D(1920.0, 1080.0), 1.0f, TEXT("1920x1080 100 %") },
		{ FVector2D(1920.0, 1080.0), 1.5f, TEXT("1920x1080 150 %") },
		{ FVector2D(1280.0, 800.0), 1.0f, TEXT("1280x800 100 %") },
		{ FVector2D(1280.0, 800.0), 1.5f, TEXT("1280x800 150 %") },
	};

	// A sandboxed game (settings and saves in memory), the realm running, the root layout and the HUD.
	struct FLayoutWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawSaveSubsystem* Save = nullptr;
		UPawSettingsSubsystem* Settings = nullptr;
		UPawRootLayout* Root = nullptr;
		UPawHudScreen* Hud = nullptr;
		TSharedPtr<SWidget> RootSlate;
		TSharedPtr<FPawMemorySaveStorage> Disk;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			Save = GI->GetSubsystem<UPawSaveSubsystem>();
			Settings = GI->GetSubsystem<UPawSettingsSubsystem>();
			if (!T.TestNotNull(TEXT("the realm"), Realm) || !T.TestNotNull(TEXT("the game"), Game) || !T.TestNotNull(TEXT("the saves"), Save)
				|| !T.TestNotNull(TEXT("the settings"), Settings) || !T.TestTrue(TEXT("data"), Game->IsDataReady()))
			{
				return false;
			}
			if (UPawAudioSubsystem* Audio = GI->GetSubsystem<UPawAudioSubsystem>())
			{
				Audio->SystemDevices = []() { return TArray<FString>{ TEXT("Default") }; };
				Audio->SwapOutput = [](const FString&) { return true; };
			}
			Disk = MakeShared<FPawMemorySaveStorage>();
			Settings->UseStorageForTest(Disk);
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
			GFrameCounter += 2;
			return true;
		}

		void Close()
		{
			RootSlate.Reset();
			if (Settings)
			{
				Settings->UseStorageForTest(nullptr);
			}
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
	};

	FString LastName(const FString& Path)
	{
		int32 Slash = INDEX_NONE;
		return Path.FindLastChar(TEXT('/'), Slash) ? Path.Mid(Slash + 1) : Path;
	}

	// Lays the widget out, writes what it saw, and checks every painted part shows on the screen.
	TArray<PawLayout::FPart> Measure(FAutomationTestBase& T, UUserWidget* W, const FSize& Size, const TCHAR* Screen)
	{
		TArray<PawLayout::FPart> Parts = PawLayout::Arrange(W, Size.Screen, Size.Scale());
		T.TestTrue(FString::Printf(TEXT("%s %s: laid out"), Screen, Size.Name), Parts.Num() > 10);
		FString Dump;
		int32 Off = 0;
		for (const PawLayout::FPart& P : Parts)
		{
			const bool bOn = !P.bShown || PawLayout::OnScreen(PawLayout::Footprint(P), Size.Screen);
			Dump += FString::Printf(TEXT("%s%s\t%s\tshown %s\n"), bOn ? TEXT("") : TEXT("OFF "), *P.Path, *PawLayout::Describe(P.Rect),
				P.bShown ? *PawLayout::Describe(P.Shown) : TEXT("none"));
			if (!bOn && Off++ < 12)
			{
				T.AddError(FString::Printf(TEXT("%s %s: %s leaves the screen at %s"), Screen, Size.Name, *P.Path, *PawLayout::Describe(PawLayout::Footprint(P))));
			}
		}
		T.TestEqual(FString::Printf(TEXT("%s %s: parts off the screen"), Screen, Size.Name), Off, 0);
		const FString File = FPaths::ProjectSavedDir() / TEXT("Layout") / FString::Printf(TEXT("%s_%.0fx%.0f_%.0f.txt"), Screen,
			Size.Screen.X, Size.Screen.Y, Size.UiSize * 100.0f);
		FFileHelper::SaveStringToFile(Dump, *File);
		return Parts;
	}

	// No shown part from Movers (by last name prefix) covers any of Fixed (by last name prefix). Returns the clashes.
	int32 Clashes(FAutomationTestBase& T, const TArray<PawLayout::FPart>& Parts, const TArray<FString>& A, const TArray<FString>& B,
		const FString& Where)
	{
		int32 Count = 0;
		for (const PawLayout::FPart& P : Parts)
		{
			if (!P.bShown || !A.ContainsByPredicate([&P](const FString& N) { return LastName(P.Path).StartsWith(N); }))
			{
				continue;
			}
			for (const PawLayout::FPart& Q : Parts)
			{
				if (&P != &Q && Q.bShown && B.ContainsByPredicate([&Q](const FString& N) { return LastName(Q.Path).StartsWith(N); })
					&& PawLayout::Overlap(PawLayout::Footprint(P), PawLayout::Footprint(Q)))
				{
					if (Count++ < 8)
					{
						T.AddError(FString::Printf(TEXT("%s: %s %s covers %s %s"), *Where, *P.Path, *PawLayout::Describe(PawLayout::Footprint(P)),
							*Q.Path, *PawLayout::Describe(PawLayout::Footprint(Q))));
					}
				}
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutTitleTest, "Pawpocalypse.UI.Layout.Title", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutTitleTest::RunTest(const FString& Parameters)
{
	FLayoutWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawMainMenu* Menu = CreateWidget<UPawMainMenu>(W.GI, PawUi::Resolve<UPawMainMenu>());
	if (TestNotNull(TEXT("the title screen"), Menu))
	{
		Menu->Refresh();
		for (const FSize& Size : Sizes)
		{
			const TArray<PawLayout::FPart> Parts = Measure(*this, Menu, Size, TEXT("title"));
			const FString Where = FString::Printf(TEXT("title %s"), Size.Name);
			TestTrue(Where + TEXT(": the logo shows"), PawLayout::Find(Parts, TEXT("NameText")) != nullptr);
			TestTrue(Where + TEXT(": Boba's card shows"), PawLayout::Find(Parts, TEXT("BobaCard")) != nullptr);
			TestTrue(Where + TEXT(": the menu shows"), PawLayout::Find(Parts, TEXT("MenuPanel")) != nullptr);
			// The logo clear of the menu; the hint clear of the menu; Boba's card clear of the version, the cluster, the menu
			// and the logo.
			TestEqual(Where + TEXT(": the logo over the menu"), Clashes(*this, Parts, { TEXT("NameText") }, { TEXT("MenuPanel") }, Where), 0);
			TestEqual(Where + TEXT(": the hint over the menu"), Clashes(*this, Parts, { TEXT("HintPlate") }, { TEXT("MenuPanel") }, Where), 0);
			TestEqual(Where + TEXT(": Boba's card over the corner"), Clashes(*this, Parts, { TEXT("BobaCard") },
				{ TEXT("VersionText"), TEXT("ClusterButton"), TEXT("MenuPanel"), TEXT("NameText") }, Where), 0);
		}
		// Phase 7: the language list at its longest (all 23 languages): on the screen, above the cluster, clear of it.
		PawText::SetShowUnfinishedLanguagesForTest(true);
		Menu->PressCluster(2);
		TestEqual(TEXT("the list's 23 rows"), Menu->GetLanguageRows().Num(), 23);
		for (const FSize& Size : Sizes)
		{
			const TArray<PawLayout::FPart> Parts = Measure(*this, Menu, Size, TEXT("title_language"));
			const FString Where = FString::Printf(TEXT("title, the language list, %s"), Size.Name);
			TestTrue(Where + TEXT(": the list shows"), PawLayout::Find(Parts, TEXT("LanguageMenu")) != nullptr);
			TestEqual(Where + TEXT(": the list over the cluster"), Clashes(*this, Parts, { TEXT("LanguageMenu") }, { TEXT("ClusterButton") }, Where), 0);
		}
		Menu->CloseLanguageMenu();
		PawText::SetShowUnfinishedLanguagesForTest(false);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutDockTest, "Pawpocalypse.UI.Layout.Dock", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutDockTest::RunTest(const FString& Parameters)
{
	FLayoutWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Hud->SetDockOnLeft(true);
	W.Hud->SelectTab(Pc::PcDockTab::Upgrades);
	W.Hud->RefreshNow();
	for (const FSize& Size : Sizes)
	{
		const TArray<PawLayout::FPart> Parts = Measure(*this, W.Hud, Size, TEXT("dock"));
		const FString Where = FString::Printf(TEXT("dock %s"), Size.Name);
		const PawLayout::FPart* Quick = PawLayout::Find(Parts, TEXT("QuickBarBox"));
		const PawLayout::FPart* Legend = PawLayout::Find(Parts, TEXT("DockLegend"));
		if (!TestNotNull(Where + TEXT(": the quick bar"), Quick) || !TestNotNull(Where + TEXT(": the legend"), Legend))
		{
			continue;
		}
		int32 Rows = 0, Under = 0;
		for (int32 I = 0; UPawButton* Row = W.Hud->GetRowButton(I); ++I)
		{
			const PawLayout::FPart* P = PawLayout::FindWidget(Parts, Row);
			if (!P || !P->bShown)
			{
				continue; // scrolled out of view
			}
			++Rows;
			if (PawLayout::Overlap(P->Shown, Quick->Rect) || PawLayout::Overlap(P->Shown, Legend->Rect))
			{
				if (Under++ < 4)
				{
					AddError(FString::Printf(TEXT("%s: shop row %d shows at %s, under the quick bar %s / legend %s"), *Where, I,
						*PawLayout::Describe(P->Shown), *PawLayout::Describe(Quick->Rect), *PawLayout::Describe(Legend->Rect)));
				}
			}
		}
		TestTrue(Where + TEXT(": shop rows show"), Rows >= 4);
		TestEqual(Where + TEXT(": shop rows under the quick bar"), Under, 0);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutBossTest, "Pawpocalypse.UI.Layout.Boss", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutBossTest::RunTest(const FString& Parameters)
{
	// D18 (S4, the UI concept's scrBoss): the boss panel never covers the pills, the status panel or the dock, at every
	// size and on either side; at 1920 x 1080 it sits BESIDE the status panel (the concept's place).
	FLayoutWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.Game->Data();
	Pc::PcProgress& P = W.Game->Progress();
	const int32 BossIndex = static_cast<int32>(Pc::RealmIndex(D, D.Code.BossRealms.front()));
	const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(BossIndex)].BossData);
	Pc::CheatUnlockAllTiers(D, P);
	TestTrue(TEXT("travel to the boss biome"), W.Realm->Travel(BossIndex));
	if (!TestNotNull(TEXT("the boss"), Boss))
	{
		W.Close();
		return false;
	}
	P.BaitKillsByBiome[P.CurrentBiomeId] += Pc::BaitShortfall(D, P, *Boss) * Boss->KillsPerBait;
	P.Catnip = 1e15;
	TestTrue(TEXT("summoned"), W.Realm->SummonBoss());
	W.Realm->AdvanceFrame(FPawRealmSim::Step);
	W.Hud->Pump(1.0);
	TestTrue(TEXT("the panel shows"), W.Hud->IsBossPanelVisible());
	for (const bool bLeft : { false, true })
	{
		W.Hud->SetDockOnLeft(bLeft);
		for (const FSize& Size : Sizes)
		{
			const FString Where = FString::Printf(TEXT("boss %s, dock %s"), Size.Name, bLeft ? TEXT("left") : TEXT("right"));
			const TArray<PawLayout::FPart> Parts = Measure(*this, W.Hud, Size, TEXT("boss"));
			TestEqual(Where + TEXT(": the boss panel covers nothing"), Clashes(*this, Parts, { TEXT("BossPanel") },
				{ TEXT("CatnipPill"), TEXT("StardustPill"), TEXT("FieldPlate"), TEXT("DockBox") }, Where), 0);
			const PawLayout::FPart* Panel = PawLayout::Find(Parts, TEXT("BossPanel"));
			const PawLayout::FPart* Status = PawLayout::Find(Parts, TEXT("FieldPlate"));
			if (Size.Screen.X == 1920.0 && Size.UiSize == 1.0f && TestNotNull(Where + TEXT(": the panel"), Panel)
				&& TestNotNull(Where + TEXT(": the status panel"), Status))
			{
				TestTrue(Where + TEXT(": beside the status panel (the concept's place)"), Panel->Rect.Top < Status->Rect.Bottom);
			}
		}
	}
	W.Hud->SetDockOnLeft(false);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutSettingsTest, "Pawpocalypse.UI.Layout.Settings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutSettingsTest::RunTest(const FString& Parameters)
{
	FLayoutWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawSettingsDialog* D = UPawSettingsDialog::Open(W.Root->ModalStack, nullptr);
	if (TestNotNull(TEXT("the settings dialog"), D))
	{
		const Pc::PcSettingsSection Sections[] = { Pc::PcSettingsSection::Audio, Pc::PcSettingsSection::Video, Pc::PcSettingsSection::Controls };
		const TCHAR* SectionNames[] = { TEXT("audio"), TEXT("video"), TEXT("controls") };
		for (int32 S = 0; S < 3; ++S)
		{
			D->PressSection(Sections[S]);
			for (const FSize& Size : Sizes)
			{
				const FString Screen = FString::Printf(TEXT("settings_%s"), SectionNames[S]);
				const TArray<PawLayout::FPart> Parts = Measure(*this, D, Size, *Screen);
				const FString Where = FString::Printf(TEXT("settings %s %s"), SectionNames[S], Size.Name);
				const PawLayout::FPart* Close = PawLayout::Find(Parts, TEXT("CloseButton"));
				if (!TestNotNull(Where + TEXT(": Close"), Close))
				{
					continue;
				}
				TestTrue(Where + TEXT(": Close on screen"), PawLayout::OnScreen(Close->Rect, Size.Screen));
				int32 Under = 0;
				for (int32 I = 0; I < Parts.Num(); ++I)
				{
					const PawLayout::FPart& P = Parts[I];
					if (P.bShown && PawLayout::IsUnder(Parts, I, TEXT("Pages")) && PawLayout::Overlap(P.Shown, Close->Rect))
					{
						if (Under++ < 4)
						{
							AddError(FString::Printf(TEXT("%s: %s shows at %s, under Close %s"), *Where, *P.Path, *PawLayout::Describe(P.Shown),
								*PawLayout::Describe(Close->Rect)));
						}
					}
				}
				TestEqual(Where + TEXT(": page parts under Close"), Under, 0);
			}
		}
		D->PressSection(Pc::PcSettingsSection::Audio);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutTreeTest, "Pawpocalypse.UI.Layout.Tree", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutTreeTest::RunTest(const FString& Parameters)
{
	FLayoutWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	for (const FSize& Size : Sizes)
	{
		// Opened fresh at each size: the fit happens on the open, and again when the real size first arrives.
		if (!TestTrue(TEXT("the tree opens"), W.Hud->OpenSkillTree(false)))
		{
			break;
		}
		UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
		const TArray<PawLayout::FPart> Parts = Measure(*this, Tree, Size, TEXT("tree"));
		const FString Where = FString::Printf(TEXT("tree %s"), Size.Name);
		FSlateRect All(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
		int32 Nodes = 0;
		for (const PawLayout::FPart& P : Parts)
		{
			if (Cast<UPawTreeNode>(P.Widget.Get()))
			{
				++Nodes;
				All = FSlateRect(FMath::Min(All.Left, P.Rect.Left), FMath::Min(All.Top, P.Rect.Top), FMath::Max(All.Right, P.Rect.Right),
					FMath::Max(All.Bottom, P.Rect.Bottom));
			}
		}
		TestTrue(Where + TEXT(": nodes laid out"), Nodes > 40);
		TestTrue(Where + FString::Printf(TEXT(": every node on screen %s"), *PawLayout::Describe(All)), PawLayout::OnScreen(All, Size.Screen));
		// The content area: under the header (130 of layout), the whole width.
		const FVector2D Centre(Size.Screen.X * 0.5, (130.0 * Size.Scale() + Size.Screen.Y) * 0.5);
		const FVector2D Drawn((All.Left + All.Right) * 0.5, (All.Top + All.Bottom) * 0.5);
		// The fit's bounds include the captions under the nodes (28 of layout, scaled by the zoom): a few px of grace.
		const double Grace = 24.0 * Size.Scale();
		TestTrue(Where + FString::Printf(TEXT(": centred (drawn at %.0f, %.0f; the middle %.0f, %.0f)"), Drawn.X, Drawn.Y, Centre.X, Centre.Y),
			FMath::Abs(Drawn.X - Centre.X) <= Grace && FMath::Abs(Drawn.Y - Centre.Y) <= Grace);
		Tree->PressClose();
	}
	// The spend step (D18, S4): Continue, big and in the middle, never over the legend (it stood on it at every size).
	for (const FSize& Size : Sizes)
	{
		if (!TestTrue(TEXT("the spend step opens"), W.Hud->OpenSkillTree(true)))
		{
			break;
		}
		UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
		const FString Where = FString::Printf(TEXT("spend %s"), Size.Name);
		const TArray<PawLayout::FPart> Parts = Measure(*this, Tree, Size, TEXT("spend"));
		TestEqual(Where + TEXT(": Continue covers no legend"), Clashes(*this, Parts, { TEXT("CloseButton") }, { TEXT("LegendPlate") }, Where), 0);
		Tree->PressClose();
		Tree->PressClose();
	}
	W.Close();
	return true;
}

// --- The language pass (phase 7, 2026-11-20; WORKLIST-LOC step 4, doc 11's exit: no overflow at 1280 x 800 and 150 % UI) ---
// Every screen with words, in every language the game has and in the pseudo-locale, laid out at its tightest (1280 x 800 at
// 150 % UI: 1153 x 720 of layout), and every text checked against its box (PawLayout::TextMisses). What it finds goes to
// Saved/Layout/languages.txt, one line each: culture, screen, part, why, the words.

namespace
{
	const FSize Tight = { FVector2D(1280.0, 800.0), 1.5f, TEXT("1280x800 150 %") };

	struct FMisses
	{
		TArray<FString> Lines;
		void Check(UUserWidget* W, const FString& Culture, const FString& Screen)
		{
			if (!W)
			{
				Lines.Add(FString::Printf(TEXT("%s\t%s\t-\tthe screen did not open\t-"), *Culture, *Screen));
				return;
			}
			const TArray<PawLayout::FPart> Parts = PawLayout::Arrange(W, Tight.Screen, Tight.Scale());
			// The skill tree's map pans and zooms (fitted on a tick, which this does not run): a node past the screen's edge is
			// not a text cut short. Its words are still held to their boxes.
			TSet<FString> Panned;
			if (Cast<UPawSkillTreeScreen>(W))
			{
				for (int32 I = 0; I < Parts.Num(); ++I)
				{
					if (PawLayout::IsUnder(Parts, I, TEXT("Canvas")))
					{
						Panned.Add(Parts[I].Path);
					}
				}
			}
			for (const PawLayout::FTextMiss& M : PawLayout::TextMisses(Parts))
			{
				if (Panned.Contains(M.Path) && M.Why.StartsWith(TEXT("cut sideways")))
				{
					continue;
				}
				Lines.Add(FString::Printf(TEXT("%s\t%s\t%s\t%s\t%s"), *Culture, *Screen, *M.Path, *M.Why, *M.Text));
			}
			for (const PawLayout::FPart& P : Parts)
			{
				if (P.bShown && !Panned.Contains(P.Path) && !PawLayout::OnScreen(PawLayout::Footprint(P), Tight.Screen)
					&& Cast<UTextLayoutWidget>(P.Widget.Get()))
				{
					Lines.Add(FString::Printf(TEXT("%s\t%s\t%s\toff the screen at %s\t-"), *Culture, *Screen, *P.Path,
						*PawLayout::Describe(PawLayout::Footprint(P))));
				}
			}
		}
	};

	void PutSave(FLayoutWorld& W, int32 Slot, int64 Reawakens, int64 Kills, double Play)
	{
		Pc::PcSaveState S;
		S.ReawakenCount = Reawakens;
		S.LifetimeKills = Kills;
		S.TotalPlaySeconds = Play;
		S.LastPlayedUnix = 1.8e9;
		S.LifetimeCatnipEver = 10.0;
		const std::string File = Pc::WriteSaveFile(S, "test");
		W.Disk->Put(UPawSaveSubsystem::SlotName(Slot), TArray<uint8>(reinterpret_cast<const uint8*>(File.data()), static_cast<int32>(File.size())));
	}

	// What a tour does besides its own checks (Pawpocalypse.UI.Language.Switch): Opened, once the game and the HUD are up
	// (before the first screen); Screen, at every screen in place of the fit check (a screen that did not open: nullptr).
	struct FTourHooks
	{
		TFunction<void(FLayoutWorld&)> Opened;
		TFunction<void(FLayoutWorld&, UUserWidget*, const FString&)> Screen;
	};

	// Every screen with words, in the language shown now. Without hooks, each one's texts are held to their boxes (M).
	void TourScreens(FAutomationTestBase& T, const FString& Culture, FMisses& M, const FTourHooks* Hooks = nullptr)
	{
		FLayoutWorld W;
		if (!W.Open(T))
		{
			W.Close();
			return;
		}
		if (Hooks && Hooks->Opened)
		{
			Hooks->Opened(W);
		}
		auto Visit = [&](UUserWidget* Shown, const FString& Screen)
		{
			if (Hooks && Hooks->Screen)
			{
				Hooks->Screen(W, Shown, Screen);
			}
			else
			{
				M.Check(Shown, Culture, Screen);
			}
		};
		const Pc::PcGameData& D = W.Game->Data();
		Pc::PcProgress& P = W.Game->Progress();

		// The title: the menu, the language list, the slots (a game, an unreadable file, empty ones), the delete prompt,
		// the leaderboard, the credits, the quit prompt.
		PutSave(W, 0, 7, 4321987, 3700.0);
		W.Disk->Put(UPawSaveSubsystem::SlotName(1), TArray<uint8>(reinterpret_cast<const uint8*>("not a save"), 10));
		if (UPawMainMenu* Menu = CreateWidget<UPawMainMenu>(W.GI, PawUi::Resolve<UPawMainMenu>()))
		{
			Menu->Refresh();
			Visit(Menu, TEXT("title"));
			Menu->PressCluster(2);
			Visit(Menu, TEXT("title, language list"));
			Menu->CloseLanguageMenu();
			Menu->PressLoad();
			Visit(Menu, TEXT("title, slots"));
			Menu->PressDelete(0);
			Visit(Menu, TEXT("title, delete prompt"));
			Menu->PressConfirmNo();
			Menu->PressSlotsBack();
			Menu->PressLeaderboard();
			Visit(Menu, TEXT("title, leaderboard"));
			Menu->PressClose();
			Menu->PressCredits();
			Visit(Menu, TEXT("title, credits"));
			Menu->PressClose();
			Menu->PressExit();
			Visit(Menu, TEXT("title, quit prompt"));
			Menu->PressConfirmNo();
		}

		// The dock, fresh (the kitten rows locked), then with a run under way: every page.
		W.Hud->SelectTab(Pc::PcDockTab::Upgrades);
		W.Hud->RefreshNow();
		Visit(W.Hud, TEXT("dock, upgrades (new game)"));
		Pc::CheatUnlockAllTiers(D, P);
		P.Catnip = 1e12; // a run worth Reawakening (the flow tests' amount): the Reawaken below reaches the spend step
		P.LifetimeCatnipEver = 1e12;
		P.LifetimeCatnipThisCycle = 1e12;
		P.ReawakenCount = 2;
		P.Rerolls = 3;
		P.Banishes = 2;
		int32 Held = 0;
		for (const Pc::PcCard& C : D.Cards)
		{
			if (Held++ % 3 == 0)
			{
				P.CardLevels[C.Id] = 1;
			}
		}
		for (size_t A = 0; A < D.Achievements.size(); A += 4)
		{
			P.AchievementsEarned.insert(D.Achievements[A].Id);
		}
		for (size_t J = 0; J < D.Jobs.size(); J += 2)
		{
			P.JobsHeld.insert(D.Jobs[J].Id);
		}
		const Pc::PcDockTab Tabs[] = { Pc::PcDockTab::Upgrades, Pc::PcDockTab::Reawaken, Pc::PcDockTab::Realms, Pc::PcDockTab::Catches,
			Pc::PcDockTab::Achievements, Pc::PcDockTab::Jobs };
		const TCHAR* TabNames[] = { TEXT("upgrades"), TEXT("reawaken"), TEXT("realms"), TEXT("catches"), TEXT("achievements"), TEXT("jobs") };
		// On screen first, as in the game: a job row's words wrap by its button's measured width, which a refresh takes from
		// the drawn button (the game refreshes 10 times a second; this refreshes once).
		W.Hud->TakeWidget();
		for (int32 I = 0; I < 6; ++I)
		{
			W.Hud->SelectTab(Tabs[I]);
			W.Hud->RefreshNow();
			Visit(W.Hud, FString::Printf(TEXT("dock, %s"), TabNames[I]));
		}
		W.Hud->SelectTab(Pc::PcDockTab::Upgrades);

		// The card pick: every card, three a hand.
		for (size_t C = 0; C < D.Cards.size(); C += 3)
		{
			std::vector<std::string> Hand;
			for (size_t K = C; K < FMath::Min(C + 3, D.Cards.size()); ++K)
			{
				Hand.push_back(D.Cards[K].Id);
			}
			P.Hand = Hand;
			P.CatchesLevel = 4;
			W.Hud->Pump(0.0);
			Visit(W.Hud->GetCardPick(), FString::Printf(TEXT("card pick, %s"), UTF8_TO_TCHAR(Hand.front().c_str())));
		}
		// Out of the pick the player's way (a take closes it): left open, it holds back the Reawaken's spend step below.
		for (int32 I = 0; I < 10 && W.Hud->IsCardPickUp(); ++I)
		{
			// The hand the screen shows is the one held (the tour set the progress's hand under it).
			UPawCardPickScreen* Pick = W.Hud->GetCardPick();
			P.Hand.clear();
			for (int32 K = 0; K < Pick->GetCardCount(); ++K)
			{
				P.Hand.push_back(TCHAR_TO_UTF8(*Pick->GetCardId(K)));
			}
			Pick->Take(0);
			W.Hud->Pump(0.0);
		}
		P.Hand.clear();
		W.Hud->Pump(0.0);

		// The skill tree: browsing, with the tip on the nodes whose tips say the most in this language; the spend step.
		if (W.Hud->OpenSkillTree(false))
		{
			UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
			TArray<TPair<int32, FString>> Longest;
			for (const std::string& Id : Tree->GetLayout().Order)
			{
				Tree->NodeFocused(UTF8_TO_TCHAR(Id.c_str()));
				int32 Len = Tree->GetTipTitle().ToString().Len();
				for (const FText& Line : Tree->GetTipLines())
				{
					Len += Line.ToString().Len();
				}
				Longest.Add({ Len, UTF8_TO_TCHAR(Id.c_str()) });
			}
			if (Hooks)
			{
				// The same nodes in every language: the switch test compares a screen in one language with it in another.
				Longest.Sort([](const TPair<int32, FString>& A, const TPair<int32, FString>& B) { return A.Value < B.Value; });
			}
			else
			{
				Longest.Sort([](const TPair<int32, FString>& A, const TPair<int32, FString>& B) { return A.Key > B.Key; });
			}
			Visit(Tree, TEXT("tree"));
			for (int32 I = 0; I < FMath::Min(4, Longest.Num()); ++I)
			{
				Tree->NodeFocused(Longest[I].Value);
				Visit(Tree, FString::Printf(TEXT("tree, tip %s"), *Longest[I].Value));
			}
			Tree->PressClose();
		}
		else
		{
			Visit(nullptr, TEXT("tree"));
		}

		// The pause menu: its page, the armed main menu, the slots page. A frame on: the tree held the field this one.
		W.Hud->Pump(1.0 / 60.0);
		GFrameCounter += 2;
		if (W.Hud->OpenPauseMenu())
		{
			UPawPauseMenu* Pause = W.Hud->GetPauseMenu();
			Visit(Pause, TEXT("pause"));
			Pause->PressMainMenu();
			Visit(Pause, TEXT("pause, armed"));
			Pause->PressSave();
			Visit(Pause, TEXT("pause, slots"));
			Pause->PressSlotsBack();
			Pause->PressContinue();
		}
		else
		{
			Visit(nullptr, TEXT("pause"));
		}

		// Settings: each section.
		if (UPawSettingsDialog* S = UPawSettingsDialog::Open(W.Root->ModalStack, nullptr))
		{
			const Pc::PcSettingsSection Sections[] = { Pc::PcSettingsSection::Audio, Pc::PcSettingsSection::Video, Pc::PcSettingsSection::Controls };
			const TCHAR* SectionNames[] = { TEXT("audio"), TEXT("video"), TEXT("controls") };
			for (int32 I = 0; I < 3; ++I)
			{
				S->PressSection(Sections[I]);
				Visit(S, FString::Printf(TEXT("settings, %s"), SectionNames[I]));
			}
			S->PressClose();
		}

		// A Reawaken: the ceremony, the spend step, the job picker.
		if (!W.Hud->GetAskReawakenButton()->IsTicked())
		{
			W.Hud->ToggleAskBeforeReawaken();
		}
		W.Hud->RefreshNow();
		W.Hud->PressReawaken();
		W.Hud->PressReawaken();
		for (int32 I = 0; I < 60; ++I)
		{
			W.Hud->Pump(1.0 / 60.0);
		}
		Visit(W.Hud->GetCeremony(), TEXT("ceremony"));
		for (int32 I = 0; I < 400 && W.Hud->GetCeremony() && W.Hud->GetCeremony()->IsPlaying(); ++I)
		{
			W.Hud->Pump(1.0 / 60.0);
		}
		W.Hud->Pump(1.0 / 60.0);
		if (UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree(); Tree && Tree->IsActivated() && Tree->IsSpendMode())
		{
			Visit(Tree, TEXT("spend step"));
			Tree->PressClose();
			Visit(Tree, TEXT("spend step, armed"));
			Tree->PressClose();
			W.Hud->Pump(1.0 / 60.0);
			Visit(W.Hud->GetJobPicker(), TEXT("job picker"));
		}
		else
		{
			const UPawSkillTreeScreen* Shown = W.Hud->GetSkillTree();
			const int32 Reawakens = static_cast<int32>(P.ReawakenCount);
			const int32 bMade = Shown ? 1 : 0, bActive = Shown && Shown->IsActivated() ? 1 : 0, bSpend = Shown && Shown->IsSpendMode() ? 1 : 0;
			const int32 bPick = W.Hud->GetCardPick() && W.Hud->GetCardPick()->IsActivated() ? 1 : 0;
			M.Lines.Add(FString::Printf(TEXT("%s\tspend step\t-\tthe Reawaken did not reach the spend step (Reawakens %d, tree %d, activated %d, spend %d, pick %d)\t-"),
				*Culture, Reawakens, bMade, bActive, bSpend, bPick));
		}

		// The ending.
		W.Hud->ShowVictory(5025.0);
		Visit(W.Hud->GetVictory(), TEXT("victory"));
		W.Close();
	}

	struct FCultureShown
	{
		FString Was;
		FCultureShown() : Was(FInternationalization::Get().GetCurrentCulture()->GetName()) {}
		~FCultureShown()
		{
			PawText::SetPseudoLocale(false);
			PawText::ShowCulture(Was);
		}
	};

	void WriteMisses(const FMisses& M, const FString& Name)
	{
		const FString File = FPaths::ProjectSavedDir() / TEXT("Layout") / Name;
		FFileHelper::SaveStringToFile(TEXT("culture\tscreen\tpart\twhy\twords\n") + FString::Join(M.Lines, TEXT("\n")) + TEXT("\n"), *File,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutLanguagesTest, "Pawpocalypse.UI.Layout.Languages", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutLanguagesTest::RunTest(const FString& Parameters)
{
	PawText::RegisterTables();
	PawText::RegisterTranslations();
	const FCultureShown Restore;
	TArray<FString> Cultures = PawText::LoadedCultures().Array();
	Cultures.Sort();
	Cultures.Insert(TEXT("en"), 0);
	TestEqual(TEXT("English and the 22 translations"), Cultures.Num(), 23);
	FMisses M;
	for (const FString& Culture : Cultures)
	{
		if (!TestTrue(FString::Printf(TEXT("%s shown"), *Culture), PawText::ShowCulture(Culture)))
		{
			continue;
		}
		TourScreens(*this, Culture, M);
	}
	WriteMisses(M, TEXT("languages.txt"));
	for (int32 I = 0; I < FMath::Min(20, M.Lines.Num()); ++I)
	{
		AddError(M.Lines[I]);
	}
	TestEqual(TEXT("texts that do not fit, in every language (Saved/Layout/languages.txt)"), M.Lines.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutPseudoTest, "Pawpocalypse.UI.Layout.PseudoLocale", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutPseudoTest::RunTest(const FString& Parameters)
{
	// Every line accented and about 40 % longer: what fits only in today's languages shows here.
	PawText::RegisterTables();
	PawText::RegisterTranslations();
	const FCultureShown Restore;
	PawText::ShowCulture(TEXT("en"));
	PawText::SetPseudoLocale(true);
	TestTrue(TEXT("the pseudo-locale on"), PawText::IsPseudoLocale());
	TestTrue(TEXT("a line made pseudo"), PawText::Ui(TEXT("TAB_upgrades")).ToString().StartsWith(TEXT("[")));
	// The rule itself: every letter accented, a {placeholder} and a multiplier kept, about 40 % more in short words.
	TestEqual(TEXT("pseudo: accents, placeholder, padding"), PawText::Pseudo(TEXT("Upgrades {n} x5")), FString(TEXT("[Üpgrådës {n} x5 ëñëñ]")));
	TestEqual(TEXT("pseudo: no letters, no change"), PawText::Pseudo(TEXT("{value} / {threshold}")), FString(TEXT("{value} / {threshold}")));
	{
		TArray<FString> Words;
		PawText::Pseudo(TEXT("Pounce damage x4. Pounce lock: you cannot move for 3s after each Pounce.")).ParseIntoArray(Words, TEXT(" "));
		int32 Longest = 0;
		for (const FString& Word : Words)
		{
			Longest = FMath::Max(Longest, Word.Len());
		}
		TestTrue(FString::Printf(TEXT("pseudo: no word longer than the English's (%d letters)"), Longest), Longest <= 7);
	}
	FMisses M;
	TourScreens(*this, TEXT("pseudo"), M);
	PawText::SetPseudoLocale(false);
	TestEqual(TEXT("English back"), PawText::Ui(TEXT("TAB_upgrades")).ToString(), FString(TEXT("Upgrades")));
	WriteMisses(M, TEXT("pseudo.txt"));
	for (int32 I = 0; I < FMath::Min(20, M.Lines.Num()); ++I)
	{
		AddError(M.Lines[I]);
	}
	TestEqual(TEXT("texts that do not fit in the pseudo-locale (Saved/Layout/pseudo.txt)"), M.Lines.Num(), 0);
	return true;
}

// The detector itself (PawLayout::TextMisses): a pass that finds nothing could be one that sees nothing. It must see a
// text wider than its box and a whole line cut by a box that clips, and pass one that fits.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLayoutTextMissesTest, "Pawpocalypse.UI.Layout.TextMisses", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLayoutTextMissesTest::RunTest(const FString& Parameters)
{
	UWidgetTree* T = NewObject<UWidgetTree>(GetTransientPackage(), NAME_None, RF_Transient);
	UCanvasPanel* Canvas = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	T->RootWidget = Canvas;
	auto Words = [T](const TCHAR* Name, const TCHAR* Text)
	{
		UTextBlock* X = T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		X->SetText(FText::FromString(Text));
		return X;
	};
	auto Put = [](UCanvasPanel* Into, UWidget* What, double X, double Y)
	{
		UCanvasPanelSlot* S = Into->AddChildToCanvas(What);
		S->SetPosition(FVector2D(X, Y));
		S->SetAutoSize(true);
	};
	// Wider than its box: an unwrapped line in a 60 px size box.
	USizeBox* Narrow = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Narrow"));
	Narrow->SetWidthOverride(60.0f);
	Narrow->SetContent(Words(TEXT("TooWide"), TEXT("A line much wider than its box")));
	Put(Canvas, Narrow, 10.0, 10.0);
	// Cut sideways: a whole line placed past the right edge of a box that clips.
	USizeBox* Clip = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Clip"));
	Clip->SetWidthOverride(100.0f);
	Clip->SetHeightOverride(60.0f);
	Clip->SetClipping(EWidgetClipping::ClipToBounds);
	UCanvasPanel* Inner = T->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Inner"));
	Clip->SetContent(Inner);
	Put(Inner, Words(TEXT("Clipped"), TEXT("Past the edge")), 60.0, 0.0);
	Put(Canvas, Clip, 10.0, 80.0);
	// Fits: a short word in a wide box.
	USizeBox* Wide = T->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Wide"));
	Wide->SetWidthOverride(300.0f);
	Wide->SetContent(Words(TEXT("Fits"), TEXT("Fits")));
	Put(Canvas, Wide, 10.0, 170.0);

	const TArray<PawLayout::FTextMiss> Misses = PawLayout::TextMisses(PawLayout::ArrangeTree(T, FVector2D(480.0, 320.0), 1.0f));
	auto Why = [&Misses](const TCHAR* Path)
	{
		const PawLayout::FTextMiss* M = Misses.FindByPredicate([Path](const PawLayout::FTextMiss& X) { return X.Path == Path; });
		return M ? M->Why : FString();
	};
	TestTrue(TEXT("a text wider than its box is seen (") + Why(TEXT("TooWide")) + TEXT(")"), Why(TEXT("TooWide")).StartsWith(TEXT("needs")));
	TestTrue(TEXT("a line cut by a box that clips is seen (") + Why(TEXT("Clipped")) + TEXT(")"),
		Why(TEXT("Clipped")).StartsWith(TEXT("cut sideways")));
	TestTrue(TEXT("a text that fits passes (") + Why(TEXT("Fits")) + TEXT(")"), Why(TEXT("Fits")).IsEmpty());
	TestEqual(TEXT("nothing else is seen"), Misses.Num(), 2);
	return true;
}

// Pawpocalypse.UI.Language.Switch: the language changed while a screen is up (phase 7, 2026-11-23). Godot rebuilds every
// code-built string on SettingsManager.language_changed (hud.gd, skill_tree_ui.gd, main_menu.gd, settings_dialog.gd,
// controls_panel.gd, prompt_label.gd); string-table text follows a culture change by itself here, words a screen turned into
// a plain string do not. The tour three times, the same steps each time; at every screen the language is changed to the
// other one and back, as the game changes it (the culture shown, then the setting saved and announced, OnChanged):
//   fresh en   built in English; at each screen: English, German live, English again
//   fresh de   built in German; at each screen: German, English live, German again
//   late de    built in English, German chosen once the HUD is up (before the first screen), then as fresh de
// The tour's screens, then the HUD in a boss fight (its own short tour). Every look must say what the screen built fresh in
// its language says (fresh en's and fresh de's first looks): the words of every laid-out Slate text (a combo box's choice
// too) and every tooltip, in view or not, compared as a multiset (a list rebuilt on the change names its rows anew).
// Writes Saved/Layout/language_switch.txt. Sabotages: tools/sabotage_realm.py `switch_*`.

namespace
{
	struct FSwitchLook
	{
		FString Run;
		FString Screen;
		FString Step;
		FString Culture;
		TArray<TPair<FString, FString>> Words; // the part's path, what it says
	};

	// Every laid-out text's words, in view or not (the tree's map pans as it ticks: what it scrolls past is still the screen's
	// text), UMG's or Slate's own (a combo box's choice), and the tooltips.
	TArray<TPair<FString, FString>> ShownWords(UUserWidget* W)
	{
		TArray<TPair<FString, FString>> Out;
		for (const PawLayout::FWords& Words : PawLayout::ArrangeWords(W, Tight.Screen, Tight.Scale()))
		{
			Out.Add({ Words.Path, Words.Words });
		}
		return Out;
	}

	// As the game changes it: UPawSettingsSubsystem::SetLanguage shows the culture (never in the editor: the test shows it,
	// as Title.Language does), saves the setting and broadcasts OnChanged. English and German: Godot's code is the culture.
	void ShowLanguage(FLayoutWorld& W, const FString& Code)
	{
		PawText::ShowCulture(Code);
		W.Settings->SetLanguage(Code);
	}

	// The HUD in a boss fight (the boss panel; not on the language pass's tour), raised as Layout.Boss raises it.
	void TourBoss(FAutomationTestBase& T, const FTourHooks& Hooks)
	{
		FLayoutWorld W;
		if (!W.Open(T))
		{
			W.Close();
			return;
		}
		if (Hooks.Opened)
		{
			Hooks.Opened(W);
		}
		const Pc::PcGameData& D = W.Game->Data();
		Pc::PcProgress& P = W.Game->Progress();
		const int32 BossIndex = static_cast<int32>(Pc::RealmIndex(D, D.Code.BossRealms.front()));
		const Pc::PcBoss* Boss = D.FindBoss(D.Tiers[static_cast<size_t>(BossIndex)].BossData);
		Pc::CheatUnlockAllTiers(D, P);
		bool bUp = Boss && W.Realm->Travel(BossIndex);
		if (bUp)
		{
			P.BaitKillsByBiome[P.CurrentBiomeId] += Pc::BaitShortfall(D, P, *Boss) * Boss->KillsPerBait;
			P.Catnip = 1e15;
			bUp = W.Realm->SummonBoss();
			W.Realm->AdvanceFrame(FPawRealmSim::Step);
			W.Hud->Pump(1.0);
		}
		Hooks.Screen(W, bUp && W.Hud->IsBossPanelVisible() ? W.Hud : nullptr, TEXT("boss fight"));
		W.Close();
	}

	TMap<FString, int32> Counted(const TArray<TPair<FString, FString>>& Words)
	{
		TMap<FString, int32> Out;
		for (const TPair<FString, FString>& P : Words)
		{
			++Out.FindOrAdd(P.Value);
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLanguageSwitchTest, "Pawpocalypse.UI.Language.Switch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLanguageSwitchTest::RunTest(const FString& Parameters)
{
	PawText::RegisterTables();
	PawText::RegisterTranslations();
	const FCultureShown Restore;
	TestTrue(TEXT("German is complete (offered to players)"), PawText::Languages(false).Contains(TEXT("de")));

	TArray<FSwitchLook> Looks;
	TArray<FString> Fails;
	struct FRun
	{
		const TCHAR* Name;
		const TCHAR* Built;  // the language the game starts in
		const TCHAR* Chosen; // the language chosen once the HUD is up
		const TCHAR* Other;  // the one each screen is switched to and back from
	};
	const FRun Runs[] = {
		{ TEXT("fresh en"), TEXT("en"), TEXT("en"), TEXT("de") },
		{ TEXT("fresh de"), TEXT("de"), TEXT("de"), TEXT("en") },
		{ TEXT("late de"), TEXT("en"), TEXT("de"), TEXT("en") },
	};
	for (const FRun& Run : Runs)
	{
		PawText::ShowCulture(Run.Built);
		FTourHooks Hooks;
		Hooks.Opened = [&Run](FLayoutWorld& W) { ShowLanguage(W, Run.Chosen); };
		Hooks.Screen = [&](FLayoutWorld& W, UUserWidget* Shown, const FString& Screen)
		{
			if (!Shown)
			{
				Fails.Add(FString::Printf(TEXT("%s\t%s\tthe screen did not open"), Run.Name, *Screen));
				return;
			}
			Looks.Add({ Run.Name, Screen, TEXT("first"), Run.Chosen, ShownWords(Shown) });
			ShowLanguage(W, Run.Other);
			Looks.Add({ Run.Name, Screen, TEXT("switched"), Run.Other, ShownWords(Shown) });
			ShowLanguage(W, Run.Chosen);
			Looks.Add({ Run.Name, Screen, TEXT("back"), Run.Chosen, ShownWords(Shown) });
		};
		FMisses M;
		TourScreens(*this, Run.Name, M, &Hooks);
		TourBoss(*this, Hooks);
		for (const FString& Line : M.Lines)
		{
			Fails.Add(FString::Printf(TEXT("%s\t%s"), Run.Name, *Line));
		}
	}

	// The references: each screen built fresh in each language, its first look.
	TMap<FString, const FSwitchLook*> Fresh;
	TArray<FString> Screens;
	for (const FSwitchLook& L : Looks)
	{
		if (L.Step == TEXT("first") && L.Run == TEXT("fresh ") + L.Culture)
		{
			Fresh.Add(L.Culture + TEXT("|") + L.Screen, &L);
			if (L.Culture == TEXT("en"))
			{
				Screens.Add(L.Screen);
			}
		}
	}
	TestTrue(FString::Printf(TEXT("the tour saw the screens (%d)"), Screens.Num()), Screens.Num() >= 20);

	// Not a pass by seeing nothing: every screen has words, and German ones.
	int32 Compared = 0;
	for (const FString& Screen : Screens)
	{
		const FSwitchLook* const* En = Fresh.Find(TEXT("en|") + Screen);
		const FSwitchLook* const* De = Fresh.Find(TEXT("de|") + Screen);
		if (!TestTrue(FString::Printf(TEXT("%s: built in both languages"), *Screen), En && De))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: has words"), *Screen), (*En)->Words.Num() > 0);
		TestFalse(FString::Printf(TEXT("%s: says something else in German"), *Screen), Counted((*En)->Words).OrderIndependentCompareEqual(Counted((*De)->Words)));
	}

	// Every look against the screen built fresh in its language.
	TArray<FString> Lines;
	for (const FSwitchLook& L : Looks)
	{
		const FSwitchLook* const* Ref = Fresh.Find(L.Culture + TEXT("|") + L.Screen);
		if (!Ref || *Ref == &L)
		{
			if (!Ref)
			{
				Lines.Add(FString::Printf(TEXT("%s\t%s\t%s\t%s\t-\tno screen built fresh in %s to compare with"), *L.Run, *L.Screen, *L.Step, *L.Culture, *L.Culture));
			}
			continue;
		}
		++Compared;
		TMap<FString, int32> Want = Counted((*Ref)->Words);
		TMap<FString, int32> Seen = Counted(L.Words);
		for (const TPair<FString, FString>& P : L.Words)
		{
			int32* Left = Want.Find(P.Value);
			if (Left && *Left > 0)
			{
				--*Left;
			}
			else
			{
				Lines.Add(FString::Printf(TEXT("%s\t%s\t%s\t%s\t%s\tshows \"%s\""), *L.Run, *L.Screen, *L.Step, *L.Culture, *P.Key,
					*P.Value.Replace(TEXT("\n"), TEXT("\\n"))));
			}
		}
		for (const TPair<FString, FString>& P : (*Ref)->Words)
		{
			int32* Has = Seen.Find(P.Value);
			if (Has && *Has > 0)
			{
				--*Has;
			}
			else
			{
				Lines.Add(FString::Printf(TEXT("%s\t%s\t%s\t%s\t%s\tlacks \"%s\" (built fresh)"), *L.Run, *L.Screen, *L.Step, *L.Culture, *P.Key,
					*P.Value.Replace(TEXT("\n"), TEXT("\\n"))));
			}
		}
	}
	TestTrue(FString::Printf(TEXT("looks compared (%d)"), Compared), Compared >= 5 * Screens.Num());

	const FString File = FPaths::ProjectSavedDir() / TEXT("Layout") / TEXT("language_switch.txt");
	FFileHelper::SaveStringToFile(TEXT("run\tscreen\tstep\tculture\tpart\twhat\n") + FString::Join(Fails, TEXT("\n")) + (Fails.IsEmpty() ? TEXT("") : TEXT("\n"))
		+ FString::Join(Lines, TEXT("\n")) + TEXT("\n"), *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	for (int32 I = 0; I < FMath::Min(10, Fails.Num()); ++I)
	{
		AddError(Fails[I]);
	}
	for (int32 I = 0; I < FMath::Min(25, Lines.Num()); ++I)
	{
		AddError(Lines[I]);
	}
	TestEqual(TEXT("screens that did not open"), Fails.Num(), 0);
	TestEqual(TEXT("words that did not follow the language (Saved/Layout/language_switch.txt)"), Lines.Num(), 0);
	return true;
}

#endif
