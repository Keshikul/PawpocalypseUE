// Pawpocalypse.UI.IconAtlas (phase 8, draw calls, 2026-11-23): every 128 px icon is a cell of the one UI atlas page and the
// page's texels there are the icon's own (tools/pack_ui_atlas.py, import_art.py icons); IconBrush draws the cell, named as the
// icon's texture; the larger pictures stay their own textures; every icon the HUD draws comes from the page (Slate batches by
// texture: each icon texture was a draw call of its own).
// Pawpocalypse.UI.Icons (phase 6 slice 5b): every icon Godot asks for by name (Art.texture: "ui_tab_%s", "achievement_%s",
// the currencies, the quick buttons, the title's plates, the tick) or by a data row's icon_path resolves in DA_Icons; and the
// HUD wears them where hud.gd does: the currencies, the wave line, the icon-only tabs and the open tab's pressed plate, the
// Reawaken page, the Achievements and Jobs rows, the Bait button and the realm rows that ask for Catnip.

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "PcHud.h"
#include "PcJobs.h"
#include "PcSave.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawGlyphs.h"
#include "UI/PawHudScreen.h"
#include "UI/PawIcons.h"
#include "UI/PawUiKit.h"
#include "UI/PawWidgets.h"

namespace
{
	struct FIconWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawHudScreen* Hud = nullptr;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			if (!T.TestNotNull(TEXT("the realm subsystem"), Realm) || !T.TestNotNull(TEXT("the game subsystem"), Game)
				|| !T.TestTrue(TEXT("the game data is ready"), Game->IsDataReady()))
			{
				return false;
			}
			Game->NewGame();
			Realm->SetViewSize(1920.0, 1080.0);
			if (!T.TestTrue(TEXT("the realm starts"), Realm->StartRealm(77)))
			{
				return false;
			}
			Hud = CreateWidget<UPawHudScreen>(GI, PawUi::Resolve<UPawHudScreen>());
			if (!T.TestNotNull(TEXT("the HUD is built"), Hud))
			{
				return false;
			}
			Hud->SelectTab(Pc::PcDockTab::Upgrades);
			Hud->RefreshNow();
			return true;
		}

		void Close()
		{
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

	FString Drawn(const UImage* I)
	{
		if (!I || I->GetVisibility() == ESlateVisibility::Collapsed)
		{
			return TEXT("(hidden)");
		}
		const UObject* Tex = I->GetBrush().GetResourceObject();
		return Tex ? Tex->GetName() : TEXT("(none)");
	}

	FString Utf(const std::string& S) { return UTF8_TO_TCHAR(S.c_str()); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawIconsTest, "Pawpocalypse.UI.Icons", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawIconsTest::RunTest(const FString& Parameters)
{
	using namespace PawUi;
	const UPawIconSet* Set = Icons();
	if (!TestNotNull(TEXT("DA_Icons (py tools/import_art.py --run icons)"), Set))
	{
		return true;
	}
	// Review CR11: an engine subsystem's property holds the set (released with the engine, never by a C++ static destructor
	// after it). No garbage collection here: the editor's keep flags spare a loaded asset whoever holds it, and a collection
	// without them would pull assets from under the other tests of this session.
	{
		const UPawIconCache* Cache = GEngine ? GEngine->GetEngineSubsystem<UPawIconCache>() : nullptr;
		if (TestNotNull(TEXT("the icon cache (an engine subsystem)"), Cache))
		{
			TestTrue(TEXT("it holds the set"), Cache->GetHeld() == Set);
		}
	}
	TestEqual(TEXT("one texture per name"), Set->Textures.Num(), Set->Names.Num());
	for (int32 I = 0; I < Set->Textures.Num(); ++I)
	{
		const UTexture2D* Tex = Set->Textures[I];
		const FString Name = Set->Names.IsValidIndex(I) ? Set->Names[I] : TEXT("?");
		// The UI concept's pictures (D18, S4) ride in the set at the size they are drawn: the Reawaken ceremony's rays at
		// 1920 x 1080 and its cat at 512, the title's cat at 512 and its mouse at 256, the save cards' seven cats at 256;
		// every icon is Godot's 128 px.
		static const TMap<FString, FIntPoint> Pictures = {
			{ TEXT("ui_icon_ceremony_rays"), FIntPoint(1920, 1080) }, { TEXT("ui_icon_unicorn_rays"), FIntPoint(1920, 1080) }, { TEXT("ui_icon_ceremony_cat"), FIntPoint(512, 512) },
			{ TEXT("ui_icon_title_cat"), FIntPoint(512, 512) }, { TEXT("ui_icon_title_mouse"), FIntPoint(256, 256) },
			{ TEXT("ui_icon_slot_cat_1"), FIntPoint(256, 256) }, { TEXT("ui_icon_slot_cat_2"), FIntPoint(256, 256) },
			{ TEXT("ui_icon_slot_cat_3"), FIntPoint(256, 256) }, { TEXT("ui_icon_slot_cat_4"), FIntPoint(256, 256) },
			{ TEXT("ui_icon_slot_cat_5"), FIntPoint(256, 256) }, { TEXT("ui_icon_slot_cat_6"), FIntPoint(256, 256) },
			{ TEXT("ui_icon_slot_cat_7"), FIntPoint(256, 256) } };
		// D18 (S4): the job picker's cats in their hats (ui_icon_job_cat_<job>, _none), 256 px like the slot cards'.
		const FIntPoint* Drawn = Pictures.Find(Name);
		static const FIntPoint JobCat(256, 256);
		if (!Drawn && Name.StartsWith(TEXT("ui_icon_job_cat_")))
		{
			Drawn = &JobCat;
		}
		if (Drawn)
		{
			TestTrue(FString::Printf(TEXT("%s: its picture's size"), *Name), Tex && Tex->Source.GetSizeX() == Drawn->X
				&& Tex->Source.GetSizeY() == Drawn->Y);
			continue;
		}
		TestTrue(FString::Printf(TEXT("%s: Godot's 128 px"), *Name), Tex && Tex->Source.GetSizeX() == 128 && Tex->Source.GetSizeY() == 128);
	}

	FIconWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return true;
	}
	const Pc::PcGameData& D = W.Game->Data();

	// 1. Every name Godot constructs, and every data icon_path, resolves.
	TArray<FString> Wanted;
	for (const TCHAR* Tab : { TEXT("upgrades"), TEXT("reawaken"), TEXT("realms"), TEXT("catches"), TEXT("awards"), TEXT("jobs") })
	{
		Wanted.Add(TabIconName(Tab));
	}
	for (const TCHAR* Kind : { TEXT("catnip"), TEXT("stardust"), TEXT("bait") })
	{
		Wanted.Add(CurrencyIconName(Kind));
	}
	for (const TCHAR* Stem : { TEXT("ui_quick_settings"), TEXT("ui_quick_skills"), TEXT("ui_quick_save"), TEXT("ui_quick_quit"),
		TEXT("ui_menu_bug"), TEXT("ui_menu_discord"), TEXT("ui_menu_language"), TEXT("ui_tick") })
	{
		Wanted.Add(Stem);
	}
	for (const Pc::PcAchievement& A : D.Achievements)
	{
		Wanted.Add(AchievementIconName(Utf(A.Id)));
	}
	for (const Pc::PcCard& C : D.Cards)
	{
		if (!C.IconPath.empty()) { Wanted.Add(Utf(C.IconPath)); }
	}
	for (const Pc::PcSkillBranch& B : D.Branches)
	{
		if (!B.IconPath.empty()) { Wanted.Add(Utf(B.IconPath)); }
	}
	for (const Pc::PcJob& J : D.Jobs)
	{
		if (!J.IconPath.empty()) { Wanted.Add(Utf(J.IconPath)); }
	}
	TestTrue(TEXT("the names Godot constructs: more than the UI's 17"), Wanted.Num() > 100);
	for (const FString& Name : Wanted)
	{
		TestNotNull(FString::Printf(TEXT("%s has its icon"), *Name), Icon(Name));
	}
	TestNull(TEXT("a name with no art is null (Art.texture's contract)"), Icon(TEXT("ui_tab_nothing")));

	// 2. The HUD wears them where hud.gd does.
	UPawHudScreen* H = W.Hud;
	TestEqual(TEXT("the Catnip icon"), Drawn(H->GetCurrencyIcon(false)), FString(TEXT("T_ui_icon_catnip")));
	TestEqual(TEXT("the Stardust icon"), Drawn(H->GetCurrencyIcon(true)), FString(TEXT("T_ui_icon_stardust")));
	for (int32 I = 0; I < static_cast<int32>(Pc::PcDockTab::Count); ++I)
	{
		const Pc::PcDockTab Tab = static_cast<Pc::PcDockTab>(I);
		const UPawButton* B = H->GetTabButton(Tab);
		if (!TestNotNull(TEXT("a tab"), B))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("tab %d: its icon, no words"), I), B->IsIconShown() && B->GetIconName().StartsWith(TEXT("ui_tab_"))
			&& B->GetLabel().IsEmpty());
		TestEqual(FString::Printf(TEXT("tab %d: the pressed plate only while open"), I), B->HasActiveLook(), Tab == Pc::PcDockTab::Upgrades);
	}
	H->SelectTab(Pc::PcDockTab::Realms);
	H->RefreshNow();
	TestTrue(TEXT("the open tab moves the pressed plate"), H->GetTabButton(Pc::PcDockTab::Realms)->HasActiveLook()
		&& !H->GetTabButton(Pc::PcDockTab::Upgrades)->HasActiveLook());
	TestTrue(TEXT("the Reawaken progress wears Catnip's"), H->GetReawakenProgressLine() && H->GetReawakenProgressLine()->IsIconShown()
		&& H->GetReawakenProgressLine()->IconName == CurrencyIconName(TEXT("catnip")));
	TestTrue(TEXT("the total wears Stardust's"), H->GetTotalLine() && H->GetTotalLine()->IsIconShown()
		&& H->GetTotalLine()->IconName == CurrencyIconName(TEXT("stardust")));
	TestEqual(TEXT("the payout wears Stardust's"), Drawn(H->GetPayoutIcon()), FString(TEXT("T_ui_icon_stardust")));
	// D18 (S4, the UI concept's scrReawaken): the Reawaken swirl on its button (Godot: Stardust's).
	TestEqual(TEXT("the Reawaken button wears the Reawaken swirl"), H->GetReawakenButton()->GetIconName(), TabIconName(TEXT("reawaken")));
	TestTrue(TEXT("the achievement summary wears Stardust's"), H->GetAchievementSummaryLine() && H->GetAchievementSummaryLine()->IsIconShown());
	TestEqual(TEXT("Claim all wears Stardust's"), H->GetClaimAllButton()->GetIconName(), CurrencyIconName(TEXT("stardust")));
	for (int32 I = 0; I < static_cast<int32>(D.Achievements.size()); ++I)
	{
		TestEqual(FString::Printf(TEXT("achievement row %d's icon"), I), Drawn(H->GetAchievementIcon(I)),
			TEXT("T_") + AchievementIconName(Utf(D.Achievements[static_cast<size_t>(I)].Id)));
	}
	const std::vector<const Pc::PcJob*> Jobs = Pc::AllJobs(D);
	for (int32 I = 0; I < static_cast<int32>(Jobs.size()); ++I)
	{
		TestEqual(FString::Printf(TEXT("job row %d's icon"), I), Drawn(H->GetJobIcon(I)),
			TEXT("T_") + FPaths::GetBaseFilename(Utf(Jobs[static_cast<size_t>(I)]->IconPath)));
	}
	const FString BaitIcon = H->GetBaitButton()->GetIconName();
	TestTrue(TEXT("the Bait button wears Bait's or (Catnip short) Catnip's"),
		BaitIcon == CurrencyIconName(TEXT("bait")) || BaitIcon == CurrencyIconName(TEXT("catnip")));
	bool bSawOwed = false;
	for (int32 I = 0; I < static_cast<int32>(D.Tiers.size()); ++I)
	{
		const Pc::PcRealmRow Row = Pc::RealmRow(D, W.Game->Progress(), I);
		const bool bOwed = Row.Kind == Pc::PcRealmRowKind::NeedsCatnip;
		bSawOwed = bSawOwed || bOwed;
		// D18 (S4, the UI concept's scrRealms): every row an icon; Catnip's only where it asks for Catnip, then the paw where
		// the cat is, the flag to travel, the lock where it cannot go yet.
		const FString Want = bOwed ? CurrencyIconName(TEXT("catnip"))
			: Row.Kind == Pc::PcRealmRowKind::Here ? TabIconName(TEXT("upgrades"))
			: (Row.Kind == Pc::PcRealmRowKind::Travel || Row.Kind == Pc::PcRealmRowKind::TravelBoss) ? TabIconName(TEXT("realms"))
			: FString(TEXT("ui_lock"));
		TestTrue(FString::Printf(TEXT("realm row %d: its icon (%s), drawn"), I, *Want),
			H->GetRealmButton(I)->IsIconShown() && H->GetRealmButton(I)->GetIconName() == Want);
	}
	TestTrue(TEXT("a new game has a realm row asking for Catnip (the case is seen)"), bSawOwed);

	// 3. The quick bar (hud.tscn QuickBar): four icon buttons, the mouse's; save says so; quit saves and leaves. D18 (S4):
	// each on its plate (the UI concept's dock; Godot: bare TextureButtons).
	const TCHAR* QuickIcons[] = { TEXT("ui_quick_settings"), TEXT("ui_quick_skills"), TEXT("ui_quick_save"), TEXT("ui_quick_quit") };
	for (int32 I = 0; I < 4; ++I)
	{
		const UPawButton* Q = H->GetQuickButton(I);
		if (!TestNotNull(FString::Printf(TEXT("quick button %d"), I), Q))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("quick %d: its icon (%s)"), I, QuickIcons[I]), Q->IsIconShown() && Q->GetIconName() == QuickIcons[I]);
		TestTrue(FString::Printf(TEXT("quick %d: on its plate (not bare)"), I), !Q->bBareLook && Q->GetStyle()
			&& !Q->GetStyle()->GetClass()->IsChildOf(UPawBareButtonStyle::StaticClass()));
		TestFalse(FString::Printf(TEXT("quick %d: never in the pad's order"), I), Q->GetIsFocusable());
		TestFalse(FString::Printf(TEXT("quick %d: its tooltip"), I), Q->GetToolTipText().IsEmpty());
	}
	TestTrue(TEXT("nothing under the quick bar before a save"), H->GetQuickStatusText().IsEmpty());
	// Review CR15: "Saved." only for a write that happens. This run has no save sandbox: saving is off, nothing is written.
	UPawSaveSubsystem* Save = W.GI->GetSubsystem<UPawSaveSubsystem>();
	TestFalse(TEXT("saving is off in this run"), Save->IsEnabled());
	H->PressQuickSave();
	TestEqual(TEXT("saving off: the quick save says it did not"), H->GetQuickStatusText().ToString(), FString(TEXT("Not saved.")));
	// A sandboxed disk: queued, then written, and it says so throughout.
	TSharedPtr<FPawMemorySaveStorage> Disk = MakeShared<FPawMemorySaveStorage>();
	Save->UseStorageForTest(Disk);
	H->PressQuickSave();
	TestEqual(TEXT("the quick save says so"), H->GetQuickStatusText().ToString(), FString(TEXT("Saved.")));
	Save->Flush();
	H->PollQuickSave();
	TestEqual(TEXT("written: still Saved."), H->GetQuickStatusText().ToString(), FString(TEXT("Saved.")));
	TestTrue(TEXT("the slot is on the disk"), Disk->Exists(UPawSaveSubsystem::SlotName(Save->GetActiveSlot())));
	// A newer build's save in the slot: the writer refuses it (and logs it), and "Saved." turns to "Not saved." when it says so.
	AddExpectedError(TEXT("the slot holds a newer build"), EAutomationExpectedErrorFlags::Contains, 1, false);
	{
		Pc::PcSaveState Future;
		Future.LifetimeCatnipEver = 1.0;
		std::string File = Pc::WriteSaveFile(Future, "future");
		File.replace(0, 9, "PAWSAVE 7");
		Disk->Put(UPawSaveSubsystem::SlotName(Save->GetActiveSlot()),
			TArray<uint8>(reinterpret_cast<const uint8*>(File.data()), static_cast<int32>(File.size())));
	}
	H->PressQuickSave();
	Save->Flush();
	H->Pump(1.0 / 60.0); // the HUD's frame reads the writer's word
	TestEqual(TEXT("a refused write: Not saved."), H->GetQuickStatusText().ToString(), FString(TEXT("Not saved.")));
	Save->UseStorageForTest(nullptr);
	bool bLeft = false;
	H->OnQuitToTitle = [&bLeft]() { bLeft = true; };
	H->PressQuickQuit();
	TestTrue(TEXT("the quick quit goes to the title"), bLeft);
	W.Close();
	return true;
}

namespace
{
	// Every UImage under Root, into the user widgets it holds (a button's icon is in the button's own tree).
	void ForEachImage(const UUserWidget* Root, TFunctionRef<void(const UImage&)> Visit, int32 Depth = 0)
	{
		if (!Root || !Root->WidgetTree || Depth > 16)
		{
			return;
		}
		Root->WidgetTree->ForEachWidget([&](UWidget* W)
		{
			if (const UImage* I = Cast<UImage>(W))
			{
				Visit(*I);
			}
			else if (const UUserWidget* U = Cast<UUserWidget>(W))
			{
				ForEachImage(U, Visit, Depth + 1);
			}
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawIconAtlasTest, "Pawpocalypse.UI.IconAtlas", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawIconAtlasTest::RunTest(const FString& Parameters)
{
	using namespace PawUi;
	// The game's way: the editor draws the textures (a transient cell must never be saved into an asset).
	TestNull(TEXT("in the editor, no cells unless asked"), IconCell(TEXT("ui_tick")));
	SetAtlasForTest(true);
	ON_SCOPE_EXIT { SetAtlasForTest(false); };
	const UPawIconSet* Set = Icons();
	if (!TestNotNull(TEXT("DA_Icons (py tools/import_art.py --run icons)"), Set))
	{
		return true;
	}
	TestTrue(TEXT("a page and the UVs for every cell"), Set->AtlasCellPage.Num() == Set->AtlasNames.Num()
		&& Set->AtlasUV.Num() == Set->AtlasNames.Num());
	TestEqual(TEXT("one atlas page"), Set->AtlasPages.Num(), 1);
	UTexture2D* Page = Set->AtlasPages.IsValidIndex(0) ? Set->AtlasPages[0].Get() : nullptr;
	if (!TestNotNull(TEXT("the page imported (py tools/pack_ui_atlas.py, then import_art.py --run icons)"), Page))
	{
		return true;
	}
	const int64 PageW = Page->Source.GetSizeX(), PageH = Page->Source.GetSizeY();
	TArray64<uint8> PageTexels;
	if (!TestTrue(TEXT("the page's texels read, BGRA8"), Page->Source.GetMipData(PageTexels, 0)
		&& Page->Source.GetFormat() == TSF_BGRA8 && PageTexels.Num() == PageW * PageH * 4))
	{
		return true;
	}
	int32 Small = 0, InAtlas = 0;
	for (int32 I = 0; I < Set->Names.Num(); ++I)
	{
		const FString& Name = Set->Names[I];
		UTexture2D* Tex = Set->Textures.IsValidIndex(I) ? Set->Textures[I].Get() : nullptr;
		const bool bSmall = Tex && Tex->Source.GetSizeX() == 128 && Tex->Source.GetSizeY() == 128;
		Small += bSmall ? 1 : 0;
		const UPawUiCell* Cell = IconCell(Name);
		const UObject* Drawn = IconBrush(Name, 24.0).GetResourceObject();
		if (!bSmall)
		{
			TestTrue(FString::Printf(TEXT("%s: a larger picture, drawn from its own texture"), *Name), !Cell && Drawn == Tex);
			continue;
		}
		const int32 A0 = Set->AtlasIndexOf(Name);
		if (!TestTrue(FString::Printf(TEXT("%s: in the atlas"), *Name), A0 != INDEX_NONE && Set->AtlasCellPage[A0] == 0 && Cell))
		{
			continue;
		}
		++InAtlas;
		const FVector4& UV = Set->AtlasUV[A0];
		const FSlateAtlasData A = Cell->GetSlateAtlasData();
		TestTrue(FString::Printf(TEXT("%s: the cell is its region of the page, 128 px"), *Name), A.AtlasTexture == Page
			&& A.StartUV.Equals(FVector2D(UV.X, UV.Y), 1e-7) && A.SizeUV.Equals(FVector2D(UV.Z - UV.X, UV.W - UV.Y), 1e-7)
			&& FVector2D(A.SizeUV.X * PageW, A.SizeUV.Y * PageH).Equals(FVector2D(128.0, 128.0), 0.01));
		TestTrue(FString::Printf(TEXT("%s: the brush draws the cell, named as the icon's texture"), *Name), Drawn == Cell
			&& Cell->GetName() == TEXT("T_") + Name);
		// The page's texels in the cell are the icon's own, byte for byte.
		TArray64<uint8> Own;
		const int64 X0 = FMath::RoundToInt64(UV.X * PageW), Y0 = FMath::RoundToInt64(UV.Y * PageH);
		bool bSame = Tex->Source.GetMipData(Own, 0) && Tex->Source.GetFormat() == TSF_BGRA8 && Own.Num() == 128 * 128 * 4
			&& X0 >= 0 && Y0 >= 0 && X0 + 128 <= PageW && Y0 + 128 <= PageH;
		for (int64 Row = 0; bSame && Row < 128; ++Row)
		{
			bSame = FMemory::Memcmp(PageTexels.GetData() + ((Y0 + Row) * PageW + X0) * 4, Own.GetData() + Row * 128 * 4, 128 * 4) == 0;
		}
		TestTrue(FString::Printf(TEXT("%s: the page's texels there are the icon's"), *Name), bSame);
	}
	TestTrue(TEXT("every 128 px icon is in the atlas"), Small > 150 && InAtlas == Small);
	// The input glyphs (56 px, the prompts' keys and buttons) and the chrome plates (the nine-patches, any size): each
	// cell the page's texels of its own texture, byte for byte.
	int32 Glyphs = 0, Plates = 0;
	for (int32 A0 = 0; A0 < Set->AtlasNames.Num(); ++A0)
	{
		const FString& Name = Set->AtlasNames[A0];
		if (Set->IndexOf(Name) != INDEX_NONE)
		{
			continue; // an icon: above
		}
		const bool bGlyph = Name.StartsWith(TEXT("input_"));
		Glyphs += bGlyph ? 1 : 0;
		Plates += bGlyph ? 0 : 1;
		UTexture2D* Tex = bGlyph ? PawGlyphs::Texture(Name) : LoadObject<UTexture2D>(nullptr,
			*FString::Printf(TEXT("/Game/Pawpocalypse/Art/UI/Chrome/T_%s.T_%s"), *Name, *Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		const UPawUiCell* Cell = IconCell(Name);
		const FVector4& UV = Set->AtlasUV[A0];
		const int64 X0 = FMath::RoundToInt64(UV.X * PageW), Y0 = FMath::RoundToInt64(UV.Y * PageH);
		const int64 TW = Tex ? Tex->Source.GetSizeX() : 0, TH = Tex ? Tex->Source.GetSizeY() : 0;
		TArray64<uint8> Own;
		bool bSame = Tex && Cell && Cell->GetSlateAtlasData().AtlasTexture == Page && Tex->Source.GetMipData(Own, 0)
			&& Tex->Source.GetFormat() == TSF_BGRA8 && Own.Num() == TW * TH * 4 && TW > 0 && X0 >= 0 && Y0 >= 0
			&& X0 + TW <= PageW && Y0 + TH <= PageH
			&& FMath::IsNearlyEqual((UV.Z - UV.X) * PageW, static_cast<double>(TW), 0.01) && FMath::IsNearlyEqual((UV.W - UV.Y) * PageH, static_cast<double>(TH), 0.01);
		for (int64 Row = 0; bSame && Row < TH; ++Row)
		{
			bSame = FMemory::Memcmp(PageTexels.GetData() + ((Y0 + Row) * PageW + X0) * 4, Own.GetData() + Row * TW * 4, TW * 4) == 0;
		}
		TestTrue(FString::Printf(TEXT("%s: a %s, its cell the page's texels of its own texture"), *Name, bGlyph ? TEXT("glyph") : TEXT("plate")), bSame);
	}
	TestTrue(FString::Printf(TEXT("the glyphs are in the atlas (%d)"), Glyphs), Glyphs >= 60);
	TestEqual(TEXT("every chrome plate is in the atlas"), Plates, static_cast<int32>(PawUi::EPawChrome::Count));
	// PawUi::Chrome draws its plate's cell in the game.
	const UPawUiCell* Panel = Cast<UPawUiCell>(PawUi::Chrome(PawUi::EPawChrome::Panel).GetResourceObject());
	TestTrue(TEXT("the panel plate from the atlas, its nine-patch kept"), Panel && Panel->GetName() == TEXT("T_ui_panel")
		&& PawUi::Chrome(PawUi::EPawChrome::Panel).DrawAs == ESlateBrushDrawType::Box);
	TestNull(TEXT("no cell for a name with no art"), IconCell(TEXT("ui_tab_nothing")));

	// The HUD on its Upgrades tab: every icon it draws comes from the page.
	FIconWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return true;
	}
	int32 FromPage = 0;
	TArray<FString> OwnTexture;
	ForEachImage(W.Hud, [&](const UImage& I)
	{
		if (I.GetVisibility() == ESlateVisibility::Collapsed || I.GetVisibility() == ESlateVisibility::Hidden)
		{
			return;
		}
		const UObject* R = I.GetBrush().GetResourceObject();
		if (const UPawUiCell* C = Cast<UPawUiCell>(R))
		{
			FromPage += C->Page == Page ? 1 : 0;
		}
		else if (R && R->GetName().StartsWith(TEXT("T_")) && IconCell(R->GetName().Mid(2)))
		{
			OwnTexture.Add(R->GetName());
		}
	});
	TestTrue(FString::Printf(TEXT("the HUD's icons come from the atlas page (%d)"), FromPage), FromPage >= 20);
	TestEqual(FString::Printf(TEXT("no HUD icon from its own texture when the atlas has it: %s"), *FString::Join(OwnTexture, TEXT(", "))),
		OwnTexture.Num(), 0);
	W.Close();
	return true;
}

#endif
