// Pawpocalypse.UI.Icons (phase 6 slice 5b): every icon Godot asks for by name (Art.texture: "ui_tab_%s", "achievement_%s",
// the currencies, the quick buttons, the title's plates, the tick) or by a data row's icon_path resolves in DA_Icons; and the
// HUD wears them where hud.gd does: the currencies, the wave line, the icon-only tabs and the open tab's pressed plate, the
// Reawaken page, the Achievements and Jobs rows, the Bait button and the realm rows that ask for Catnip.

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

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
#include "UI/PawHudScreen.h"
#include "UI/PawIcons.h"
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
	TestEqual(TEXT("one texture per name"), Set->Textures.Num(), Set->Names.Num());
	for (int32 I = 0; I < Set->Textures.Num(); ++I)
	{
		const UTexture2D* Tex = Set->Textures[I];
		TestTrue(FString::Printf(TEXT("%s: Godot's 128 px"), Set->Names.IsValidIndex(I) ? *Set->Names[I] : TEXT("?")),
			Tex && Tex->Source.GetSizeX() == 128 && Tex->Source.GetSizeY() == 128);
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
	TestTrue(TEXT("the wave line wears Bait's"), H->GetWaveLine() && H->GetWaveLine()->IconName == CurrencyIconName(TEXT("bait"))
		&& H->GetWaveLine()->IsIconShown());
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
	TestEqual(TEXT("the Reawaken button wears Stardust's"), H->GetReawakenButton()->GetIconName(), CurrencyIconName(TEXT("stardust")));
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
		TestEqual(FString::Printf(TEXT("realm row %d: Catnip's icon only when it asks for Catnip"), I),
			H->GetRealmButton(I)->IsIconShown(), bOwed);
	}
	TestTrue(TEXT("a new game has a realm row asking for Catnip (the case is seen)"), bSawOwed);

	// 3. The quick bar (hud.tscn QuickBar): four bare icon buttons, the mouse's; save says so; quit saves and leaves.
	const TCHAR* QuickIcons[] = { TEXT("ui_quick_settings"), TEXT("ui_quick_skills"), TEXT("ui_quick_save"), TEXT("ui_quick_quit") };
	for (int32 I = 0; I < 4; ++I)
	{
		const UPawButton* Q = H->GetQuickButton(I);
		if (!TestNotNull(FString::Printf(TEXT("quick button %d"), I), Q))
		{
			continue;
		}
		TestTrue(FString::Printf(TEXT("quick %d: its icon (%s), 40 px"), I, QuickIcons[I]), Q->IsIconShown() && Q->GetIconName() == QuickIcons[I]);
		TestTrue(FString::Printf(TEXT("quick %d: bare (a TextureButton: no plate)"), I), Q->bBareLook && Q->GetStyle()
			&& Q->GetStyle()->GetClass()->IsChildOf(UPawBareButtonStyle::StaticClass()));
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

#endif
