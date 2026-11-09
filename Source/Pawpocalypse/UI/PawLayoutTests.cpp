// Pawpocalypse.UI.Layout.*: the screens fit (R4h, 2026-11-08; WORKLIST-RESTYLE R4's exit: "no overflow at 150 %").
// Each screen is laid out headless (PawLayout::Arrange: prepass, paint, which ticks, then arrange) at 1920 x 1080, at
// 1920 x 1080 with 150 % UI size (1280 x 720 of layout) and at 1280 x 800 (the Deck). Every painted part lies on the screen
// (a part a scroll box clips is judged by what shows), and the parts that must never cover each other do not.
//
// Pawpocalypse.UI.Layout.Title     the logo clear of the menu; Boba's card, the version, the cluster, the hint on screen
// Pawpocalypse.UI.Layout.Dock      the Upgrades page: no shop row shows under the quick bar or the legend
// Pawpocalypse.UI.Layout.Settings  each section: nothing on the page shows under Close; the window on screen
// Pawpocalypse.UI.Layout.Tree      the tree (every node) on screen and centred in the content area
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
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawLayoutCheck.h"
#include "UI/PawMainMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawSkillTreeScreen.h"
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
	W.Close();
	return true;
}

#endif
