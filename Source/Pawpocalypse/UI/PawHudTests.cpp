// The HUD's engine half (docs/09 L3; docs/11 phase 3). PcHud's decisions are pinned in PawCore; these prove what the game
// builds around them: the string tables load and say Godot's English, the widgets show what the progress says, a press buys
// and tells the realm, the dock's real rect measures the field, nothing over the field takes a click, and the dock's
// navigation is written for every row.
//
// Pawpocalypse.UI.Strings        both tables registered; keys resolve to their English; a two-line value keeps its line.
// Pawpocalypse.UI.Hud.Readouts   currencies, rows (hidden, locked, priced, bulk), the multiplier, the zone and wave lines,
//                                the XP bar, all as the English a player reads.
// Pawpocalypse.UI.Hud.Press      a row press buys, answers on the frame, and a Kitten Count press wakes a kitten.
// Pawpocalypse.UI.Hud.Dock       the dock's rect, on either side, measures the field; a click over it is not a Pounce.
// Pawpocalypse.UI.Hud.HitTest    everything over the field is hit-test invisible.
// Pawpocalypse.UI.Hud.Navigation tabs and rows have explicit rules; the focus target is a shown row; badges.
// Pawpocalypse.UI.Hud.Boss       the boss panel during a fight, and its after-word when the boss dies.
// Pawpocalypse.UI.Hud.Realms     the stage line, the Bait button, every row as the player reads it, a press travels.
// Pawpocalypse.UI.Hud.Reawaken   the payout lines, the two-press confirm (focus to Cancel), Cancel, the box, the reset.
// Pawpocalypse.UI.Hud.Catches    every card as read (taken, or what stands in the way), the summary, the auto-pick box.
// Pawpocalypse.UI.Hud.Achievements  rows, marks, Claim and Claim all each pay once, the summary, the column.
// Pawpocalypse.UI.Hud.Jobs       locked by a boss, by Reawakens, closed by a rival, ready; owned and worn; the column.
//
// Negative tests: tools/sabotage_realm.py (`hud_*`).

#include "PcRainbow.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetNavigation.h"
#include "Components/PanelWidget.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Misc/AutomationTest.h"
#include "UI/PawWidgets.h"
#include "PcAchievements.h"
#include "PcCards.h"
#include "PcCheats.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcHud.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcStats.h"
#include "PcTree.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawUiConstants.h"
#include "UI/PawHudScreen.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawPounceClock.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawTextTesting.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// A standalone game instance with its world and subsystems, a realm started on a fixed seed, and a HUD over it.
	struct FHudWorld
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
			Hud->SetDockOnLeft(true);
			Hud->SelectTab(Pc::PcDockTab::Upgrades);
			while (Hud->GetBuyMultiplier() != 1)
			{
				Hud->CycleMultiplier();
			}
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

		Pc::PcProgress& P() { return Game->Progress(); }
		const Pc::PcGameData& D() { return Game->Data(); }
		int32 RowOf(const char* Id) const
		{
			const std::vector<std::string>& O = Pc::ShopRowOrder();
			for (int32 I = 0; I < static_cast<int32>(O.size()); ++I)
			{
				if (O[static_cast<size_t>(I)] == Id)
				{
					return I;
				}
			}
			return INDEX_NONE;
		}
	};

	FString S(const FText& T) { return T.ToString(); }

	bool Shown(const UWidget* W) { return W && W->GetVisibility() != ESlateVisibility::Collapsed && W->GetVisibility() != ESlateVisibility::Hidden; }

	// Every widget under Root (Root included) that would take a click.
	void ClickTakers(UWidget* Root, TArray<UWidget*>& Out)
	{
		if (!Root || !Shown(Root))
		{
			return;
		}
		const ESlateVisibility V = Root->GetVisibility();
		if (V == ESlateVisibility::Visible)
		{
			Out.Add(Root);
		}
		if (V == ESlateVisibility::HitTestInvisible)
		{
			return; // nothing under it can be hit
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
		{
			for (int32 I = 0; I < Panel->GetChildrenCount(); ++I)
			{
				ClickTakers(Panel->GetChildAt(I), Out);
			}
		}
		if (UUserWidget* User = Cast<UUserWidget>(Root); User && User->WidgetTree)
		{
			ClickTakers(User->WidgetTree->RootWidget, Out);
		}
	}

	EUINavigationRule Rule(const UWidget* W, EUINavigation Dir, UWidget** OutTarget = nullptr)
	{
		const UWidgetNavigation* N = W ? W->Navigation.Get() : nullptr;
		if (!N)
		{
			return EUINavigationRule::Escape;
		}
		const FWidgetNavigationData& Data = Dir == EUINavigation::Up ? N->Up : Dir == EUINavigation::Down ? N->Down
			: Dir == EUINavigation::Left ? N->Left : N->Right;
		if (OutTarget)
		{
			*OutTarget = Data.Widget.Get();
		}
		return Data.Rule;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawUiStringsTest, "Pawpocalypse.UI.Strings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawUiStringsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("ST_UI has its keys"), PawText::HasUi(TEXT("TAB_upgrades")));
	TestEqual(TEXT("a key says Godot's English"), S(PawText::Ui(TEXT("TAB_awards"))), FString(TEXT("Achievements")));
	TestEqual(TEXT("a data name by convention"), S(PawText::Data(TEXT("UPG"), "pounce_cooldown")), FString(TEXT("Pounce Recovery")));
	TestEqual(TEXT("a biome name"), S(PawText::Data(TEXT("TIER"), "tier_1")), FString(TEXT("Backyard")));
	const FString TwoLines = S(PawText::Ui(TEXT("SHOP_kitten_power_locked")));
	TestTrue(TEXT("a two-line value keeps its line break"), TwoLines.Contains(TEXT("\n")) && TwoLines.StartsWith(TEXT("Kitten Power")));
	TestFalse(TEXT("a missing key is missing"), PawText::HasUi(TEXT("NO_such_key")));
	FFormatNamedArguments Args;
	Args.Add(TEXT("name"), FText::AsCultureInvariant(TEXT("Shred")));
	Args.Add(TEXT("level"), PawText::Count(3));
	Args.Add(TEXT("price"), PawText::Amount(1234.0));
	TestEqual(TEXT("named arguments fill"), S(PawText::Ui(TEXT("SHOP_row_price"), Args)),
		FString::Printf(TEXT("Shred (Lv 3) — %s"), *S(PawText::Amount(1234.0))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudReadoutsTest, "Pawpocalypse.UI.Hud.Readouts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudReadoutsTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	P.Catnip = 300.0;
	W.Hud->RefreshNow();
	TestEqual(TEXT("Catnip"), S(W.Hud->GetCatnipText()), S(PawText::Amount(300.0)));
	TestEqual(TEXT("Stardust"), S(W.Hud->GetStardustText()), S(PawText::Amount(0.0)));

	// Rows, as a player reads them (Godot's English, Amounts.short prices).
	const int32 Paw = W.RowOf("paw_damage");
	const double PawCost = Pc::UpgradeCostById(W.D(), P, "paw_damage");
	TestEqual(TEXT("a priced row"), S(W.Hud->GetRowButton(Paw)->GetLabel()), FString::Printf(TEXT("Paw Damage (Lv 0) — %s"), *S(PawText::Amount(PawCost))));
	TestFalse(TEXT("affordable: not locked"), W.Hud->GetRowButton(Paw)->IsLocked());
	const int32 Exec = W.RowOf("execute");
	TestTrue(TEXT("unaffordable: locked"), Pc::UpgradeCostById(W.D(), P, "execute") > P.Catnip && W.Hud->GetRowButton(Exec)->IsLocked());
	TestTrue(TEXT("a locked row is still focusable (Slate-enabled)"), W.Hud->GetRowButton(Exec)->GetIsEnabled());
	// R4e: the board's rows: the words on the left, a little bigger, taller.
	TestTrue(TEXT("a shop row: words left, 18 px"), W.Hud->GetRowButton(Paw)->bAlignLeft
		&& FMath::IsNearlyEqual(W.Hud->GetRowButton(Paw)->LabelPx, static_cast<float>(PawUi::R4RowPx)));
	// R4d: the wave line's plate is what shows and hides with it.
	TestTrue(TEXT("the wave line sits on its plum plate"), W.Hud->WaveShown() && W.Hud->WaveShown()->GetFName() == TEXT("WavePlate"));
	const int32 Power = W.RowOf("kitten_power");
	TestTrue(TEXT("Kitten Power waits for a kitten"), S(W.Hud->GetRowButton(Power)->GetLabel()).StartsWith(TEXT("Kitten Power — buy a Kitten first")));
	TestTrue(TEXT("and is locked"), W.Hud->GetRowButton(Power)->IsLocked());
	for (int32 I = 0; I < static_cast<int32>(Pc::ShopRowOrder().size()); ++I)
	{
		const Pc::PcShopRow Row = Pc::ShopRow(W.D(), P, Pc::ShopRowOrder()[static_cast<size_t>(I)], 1);
		TestEqual(FString::Printf(TEXT("row %d shown as PcHud says"), I), Shown(W.Hud->GetRowButton(I)), Row.Kind != Pc::PcShopRowKind::Hidden);
	}

	// The multiplier: Buy x5 prices the walked sum.
	W.Hud->CycleMultiplier();
	TestEqual(TEXT("the multiplier says x5"), S(W.Hud->GetMultiplierButton()->GetLabel()), FString(TEXT("Buy x5")));
	P.Catnip = 1e9;
	W.Hud->RefreshNow();
	const Pc::PcShopRow Bulk = Pc::ShopRow(W.D(), P, "paw_damage", 5);
	TestEqual(TEXT("a bulk row"), S(W.Hud->GetRowButton(Paw)->GetLabel()),
		FString::Printf(TEXT("Paw Damage (Lv 0) — +5 for %s"), *S(PawText::Amount(Bulk.Price))));
	W.Hud->CycleMultiplier();
	W.Hud->CycleMultiplier();
	TestEqual(TEXT("then Max"), S(W.Hud->GetMultiplierButton()->GetLabel()), FString(TEXT("Buy Max")));
	W.Hud->CycleMultiplier();
	TestEqual(TEXT("and round to x1"), W.Hud->GetBuyMultiplier(), int64(1));

	// The zone plate and the wave line.
	P.Catnip = 300.0;
	W.Hud->RefreshNow();
	const Pc::PcZonePlate Zone = Pc::ZonePlate(W.Realm->GetSim().Field, W.D(), P);
	TestEqual(TEXT("the zone plate's kind"), Zone.Kind, Pc::PcZonePlateKind::Progress);
	TestEqual(TEXT("the zone plate"), S(W.Hud->GetZoneText()),
		FString::Printf(TEXT("Next biome  —  %s / %s"), *S(PawText::Amount(Zone.Have)), *S(PawText::Amount(Zone.Need))));
	// The bar keeps what Godot's Range keeps: the fraction snapped to 0.01.
	TestTrue(TEXT("its bar"), FMath::IsNearlyEqual(W.Hud->GetZoneFill(),
		static_cast<float>(Pc::GodotRangeValue(Zone.Fraction, 0.0, 1.0, Pc::GodotRangeStep)), 1e-6f));
	const Pc::PcWaveLine Wave = Pc::WaveLine(W.D(), P);
	TestEqual(TEXT("the wave line shown as PcHud says"), W.Hud->IsWaveVisible(), Wave.bVisible);
	if (Wave.bVisible && !Wave.bBait)
	{
		TestTrue(TEXT("the wave line"), S(W.Hud->GetWaveText()).StartsWith(FString::Printf(TEXT("Wave %lld  —  "), Wave.Wave)));
	}

	// The XP bar: within the level.
	P.CatchesLevel = 2;
	P.CatchesPoints = (Pc::TotalCatchesForLevel(W.D(), 2) + Pc::TotalCatchesForLevel(W.D(), 3)) * 0.5;
	W.Hud->RefreshNow();
	TestEqual(TEXT("XP: this level"), S(W.Hud->GetXpText(false)), FString(TEXT("2")));
	TestEqual(TEXT("XP: the next"), S(W.Hud->GetXpText(true)), FString(TEXT("3")));
	TestTrue(TEXT("XP: half full"), FMath::IsNearlyEqual(W.Hud->GetXpFill(), 0.5f, 1e-4f));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudPressTest, "Pawpocalypse.UI.Hud.Press", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudPressTest::RunTest(const FString& Parameters)
{
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	const int32 Paw = W.RowOf("paw_damage");
	const double Cost = Pc::UpgradeCostById(W.D(), P, "paw_damage");
	P.Catnip = Cost;
	W.Hud->RefreshNow();
	W.Hud->PressRow(Paw);
	TestEqual(TEXT("the press bought a level"), Pc::UpgradeLevel(P, "paw_damage"), int64_t(1));
	TestEqual(TEXT("and paid for it"), P.Catnip, 0.0);
	TestTrue(TEXT("the row answers on the frame"), S(W.Hud->GetRowButton(Paw)->GetLabel()).StartsWith(TEXT("Paw Damage (Lv 1)")));
	TestEqual(TEXT("Catnip answers too"), S(W.Hud->GetCatnipText()), S(PawText::Amount(0.0)));
	// A press it cannot pay buys nothing, and still answers on the frame: Catnip changed behind the HUD's back (no tick
	// between), so only the press's own refresh shows the new amount (2026-11-10: this half was unasserted, and the
	// hud_press_waits_for_refresh sabotage, which drops that refresh, was MISSED in the nightly).
	P.Catnip = Cost * 0.5;
	W.Hud->PressRow(Paw);
	TestEqual(TEXT("a press it cannot pay buys nothing"), Pc::UpgradeLevel(P, "paw_damage"), int64_t(1));
	TestEqual(TEXT("a denied press answers on the frame too"), S(W.Hud->GetCatnipText()), S(PawText::Amount(Cost * 0.5)));

	// A Kitten Count press re-counts the realm's kittens (Godot's upgrade_purchased -> _sync_kitten_count).
	const int64_t Before = W.Realm->GetSim().Field.AwakeKittens();
	P.Catnip = 1e9;
	W.Hud->PressRow(W.RowOf("kitten_count"));
	TestEqual(TEXT("Kitten Count bought"), Pc::UpgradeLevel(P, "kitten_count"), int64_t(1));
	TestEqual(TEXT("a kitten woke in the realm"), W.Realm->GetSim().Field.AwakeKittens(), Before + 1);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudDockTest, "Pawpocalypse.UI.Hud.Dock", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudDockTest::RunTest(const FString& Parameters)
{
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcField& F = W.Realm->GetSim().Field;
	TestEqual(TEXT("without a dock the field is the view"), F.Bounds.X, 0.0);
	// A HUD 1920 x 1080 UI units (scale 1), dock on the left.
	W.Hud->ReportDockForSize(FVector2D(1920.0, 1080.0));
	TestEqual(TEXT("left: the dock's rect"), W.Realm->GetDock().W, 568.0);
	TestTrue(TEXT("left: the field starts at the dock's edge"), FMath::IsNearlyEqual(F.Bounds.X, 568.0, 1e-9));
	TestTrue(TEXT("left: and runs to the right edge"), FMath::IsNearlyEqual(F.Bounds.X + F.Bounds.W, 1920.0, 1e-9));
	TestTrue(TEXT("a click over the dock is the dock's"), W.Realm->IsOverDock(100.0, 500.0));
	TestFalse(TEXT("a click on the field is a Pounce"), W.Realm->IsOverDock(1000.0, 500.0));
	// Flipped: the dock on the right.
	W.Hud->SetDockOnLeft(false);
	W.Hud->ReportDockForSize(FVector2D(1920.0, 1080.0));
	TestTrue(TEXT("right: the dock's rect"), FMath::IsNearlyEqual(W.Realm->GetDock().X, 1920.0 - 568.0, 1e-9));
	TestTrue(TEXT("right: the field starts at 0"), FMath::IsNearlyEqual(F.Bounds.X, 0.0, 1e-9));
	TestTrue(TEXT("right: and stops at the dock"), FMath::IsNearlyEqual(F.Bounds.W, 1920.0 - 568.0, 1e-9));
	TestTrue(TEXT("right: the dock takes its clicks"), W.Realm->IsOverDock(1800.0, 500.0));
	// A window of another size: the UI's units scale to the field's 1080.
	W.Hud->SetDockOnLeft(true);
	W.Hud->ReportDockForSize(FVector2D(1280.0, 800.0));
	TestTrue(TEXT("scaled: the dock in field units"), FMath::IsNearlyEqual(W.Realm->GetDock().W, 568.0 * 1080.0 / 800.0, 1e-9));
	W.Hud->SetDockOnLeft(true);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudDockConsumerTest, "Pawpocalypse.UI.Hud.DockConsumer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudDockConsumerTest::RunTest(const FString& Parameters)
{
	// Review CR13 (2026-11-10): the dock takes its keys first through the device subsystem's consumer, which the HUD sets when
	// it activates and cleared, whoever had set it, when it was destructed. On a travel a new HUD can activate before the old
	// one's Slate goes: the old one's destruct then took the dock's keys from the new one, and pad A pressed a dock row twice
	// again (the bug fixed in HANDOFF-LOG (49)).
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawInputDeviceSubsystem* D = W.GI->GetSubsystem<UPawInputDeviceSubsystem>();
	UPawHudScreen* Newer = CreateWidget<UPawHudScreen>(W.GI, PawUi::Resolve<UPawHudScreen>());
	if (!TestNotNull(TEXT("the device subsystem"), D) || !TestNotNull(TEXT("a second HUD"), Newer))
	{
		W.Close();
		return false;
	}
	TSharedPtr<SWidget> OldSlate = W.Hud->TakeWidget();
	TSharedPtr<SWidget> NewSlate = Newer->TakeWidget();
	W.Hud->ActivateWidget();
	Newer->ActivateWidget();
	TestTrue(TEXT("the newer HUD's dock takes its keys"), D->HasDockConsumer());
	OldSlate.Reset(); // the old HUD's Slate goes: its NativeDestruct
	TestTrue(TEXT("the old HUD's destruct leaves the newer HUD's consumer"), D->HasDockConsumer());
	Newer->DeactivateWidget();
	NewSlate.Reset();
	TestFalse(TEXT("the newer HUD's own destruct clears it"), D->HasDockConsumer());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudHitTestTest, "Pawpocalypse.UI.Hud.HitTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudHitTestTest::RunTest(const FString& Parameters)
{
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	// Over the field nothing may eat the click that Pounce needs (Godot's MOUSE_FILTER_STOP lesson, docs/05 §6.1).
	TestEqual(TEXT("the HUD's root lets clicks through"), W.Hud->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
	for (UWidget* Part : { W.Hud->GetStatusRow(), static_cast<UWidget*>(W.Hud->GetPounceClock()), W.Hud->GetBossPanel() })
	{
		TArray<UWidget*> Takers;
		ClickTakers(Part, Takers);
		TestEqual(FString::Printf(TEXT("%s takes no click"), Part ? *Part->GetName() : TEXT("(null)")), Takers.Num(), 0);
	}
	// The dock does take them (its buttons).
	TArray<UWidget*> DockTakers;
	ClickTakers(W.Hud->GetDock(), DockTakers);
	TestTrue(TEXT("the dock's buttons take clicks"), DockTakers.Num() > 10);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudNavigationTest, "Pawpocalypse.UI.Hud.Navigation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudNavigationTest::RunTest(const FString& Parameters)
{
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Hud->RefreshNow();
	TArray<UWidget*> Column = { W.Hud->GetMultiplierButton() };
	for (int32 I = 0; I < static_cast<int32>(Pc::ShopRowOrder().size()); ++I)
	{
		if (Shown(W.Hud->GetRowButton(I)))
		{
			Column.Add(W.Hud->GetRowButton(I));
		}
	}
	TestTrue(TEXT("rows are shown"), Column.Num() > 5);
	UWidget* Tab = W.Hud->GetTabButton(Pc::PcDockTab::Upgrades);
	UWidget* Target = nullptr;
	TestEqual(TEXT("the open tab leads down to the page"), Rule(Tab, EUINavigation::Down, &Target), EUINavigationRule::Explicit);
	TestTrue(TEXT("to the multiplier"), Target == Column[0]);
	for (int32 I = 0; I < Column.Num(); ++I)
	{
		Target = nullptr;
		TestEqual(FString::Printf(TEXT("item %d up is explicit"), I), Rule(Column[I], EUINavigation::Up, &Target), EUINavigationRule::Explicit);
		TestTrue(FString::Printf(TEXT("item %d up goes to the one above"), I), Target == (I == 0 ? Tab : Column[I - 1]));
		if (I + 1 < Column.Num())
		{
			Target = nullptr;
			TestEqual(FString::Printf(TEXT("item %d down is explicit"), I), Rule(Column[I], EUINavigation::Down, &Target), EUINavigationRule::Explicit);
			TestTrue(FString::Printf(TEXT("item %d down skips hidden rows"), I), Target == Column[I + 1]);
		}
		else
		{
			TestEqual(TEXT("the last row stops"), Rule(Column[I], EUINavigation::Down), EUINavigationRule::Stop);
		}
	}
	// Tabs walk left and right among the shown ones.
	UWidget* Next = nullptr;
	TestEqual(TEXT("a tab's right is explicit"), Rule(Tab, EUINavigation::Right, &Next), EUINavigationRule::Explicit);
	TestTrue(TEXT("to the next tab"), Next == W.Hud->GetTabButton(Pc::PcDockTab::Reawaken));
	TestFalse(TEXT("Jobs is hidden with no job"), Shown(W.Hud->GetTabButton(Pc::PcDockTab::Jobs)));

	// The focus target is a shown row (never nothing: the pad would die).
	UWidget* Focus = W.Hud->GetDesiredFocusTarget();
	TestTrue(TEXT("a focus target exists and is shown"), Focus && Shown(Focus));

	// Badges: Upgrades, affordable, starred when another tab is open, never on itself.
	W.P().Catnip = 1e9;
	W.Hud->SelectTab(Pc::PcDockTab::Realms);
	TestTrue(TEXT("the Upgrades star while Realms is open"), W.Hud->GetTabButton(Pc::PcDockTab::Upgrades)->HasBadge());
	W.Hud->SelectTab(Pc::PcDockTab::Upgrades);
	TestFalse(TEXT("no star on the open tab"), W.Hud->GetTabButton(Pc::PcDockTab::Upgrades)->HasBadge());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudBossTest, "Pawpocalypse.UI.Hud.Boss", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudBossTest::RunTest(const FString& Parameters)
{
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();
	TestFalse(TEXT("no panel without a boss"), W.Hud->IsBossPanelVisible());
	const int32 BossIndex = static_cast<int32>(Pc::RealmIndex(D, D.Code.BossRealms.front()));
	const Pc::PcTier& Tier = D.Tiers[static_cast<size_t>(BossIndex)];
	const Pc::PcBoss* Boss = D.FindBoss(Tier.BossData);
	if (!TestNotNull(TEXT("the first boss"), Boss))
	{
		W.Close();
		return false;
	}
	Pc::CheatUnlockAllTiers(D, P);
	TestTrue(TEXT("travel to the boss biome"), W.Realm->Travel(BossIndex));
	P.TierKillCount += Pc::BaitShortfall(D, P, *Boss) * Boss->KillsPerBait;
	P.Catnip = 1e15;
	TestTrue(TEXT("summoned"), W.Realm->SummonBoss());
	W.Realm->AdvanceFrame(FPawRealmSim::Step);
	W.Hud->Pump(1.0);
	TestTrue(TEXT("the panel during the fight"), W.Hud->IsBossPanelVisible());
	const FString Name = S(PawText::Data(TEXT("BOSS"), Tier.BossData));
	TestTrue(TEXT("names the boss and its phase"), S(W.Hud->GetBossText()).StartsWith(FString::Printf(TEXT("%s  —  Phase 1  —  "), *Name)));

	// Killed: the after-word stays, the bar empties.
	Pc::PcField& F = W.Realm->GetSim().Field;
	Pc::DamageFieldBoss(F, D, P, W.Realm->GetSim().Input, F.BossHp * 10.0, 1.0);
	F.bBossShielded = false;
	if (F.bBossUp)
	{
		F.BossInvulnTimer = 0.0;
		Pc::DamageFieldBoss(F, D, P, W.Realm->GetSim().Input, 1e30, 1.0);
	}
	W.Realm->AdvanceFrame(FPawRealmSim::Step);
	W.Hud->Pump(1.0);
	TestFalse(TEXT("the boss is down"), F.bBossUp);
	TestTrue(TEXT("the panel stays for its after-word"), W.Hud->IsBossPanelVisible());
	TestTrue(TEXT("it says the boss is defeated"), S(W.Hud->GetBossText()).StartsWith(FString::Printf(TEXT("%s DEFEATED"), *Name)));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudRealmsTest, "Pawpocalypse.UI.Hud.Realms", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudRealmsTest::RunTest(const FString& Parameters)
{
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();
	W.Hud->SelectTab(Pc::PcDockTab::Realms);
	const FString Here = S(PawText::Data(TEXT("TIER"), D.Tiers[0].Id));
	const FString Next = S(PawText::Data(TEXT("TIER"), D.Tiers[1].Id));

	// The stage line names the boss in the way and where it lives.
	const FString FirstBossBiome = S(PawText::Data(TEXT("TIER"), D.Code.BossRealms.front()));
	const FString FirstBoss = S(PawText::Data(TEXT("BOSS"), D.Tiers[static_cast<size_t>(Pc::RealmIndex(D, D.Code.BossRealms.front()))].BossData));
	TestEqual(TEXT("the stage line"), S(W.Hud->GetStageText()),
		FString::Printf(TEXT("Evolution Stage 1 — next: defeat %s in %s"), *FirstBoss, *FirstBossBiome));
	// No boss here: the Bait button points at the next one, and cannot be pressed.
	TestEqual(TEXT("the Bait button points elsewhere"), S(W.Hud->GetBaitButton()->GetLabel()),
		FString::Printf(TEXT("Bait — the next boss is in %s"), *FirstBossBiome));
	TestTrue(TEXT("and is locked"), W.Hud->GetBaitButton()->IsLocked());

	// The rows, as read.
	TestEqual(TEXT("here"), S(W.Hud->GetRealmButton(0)->GetLabel()), FString::Printf(TEXT("%s — you are here"), *Here));
	TestTrue(TEXT("here is locked"), W.Hud->GetRealmButton(0)->IsLocked());
	const double Owed = Pc::CatnipUntilUnlocked(P, D.Tiers[1]);
	TestEqual(TEXT("the next biome names the Catnip it still wants"), S(W.Hud->GetRealmButton(1)->GetLabel()),
		FString::Printf(TEXT("%s — earn %s more"), *Next, *S(PawText::Amount(Owed))));
	TestTrue(TEXT("and is locked"), W.Hud->GetRealmButton(1)->IsLocked());
	P.LifetimeCatnipThisCycle += D.Tiers[1].CatnipToUnlock + 1.0;
	W.Hud->RefreshNow();
	TestEqual(TEXT("open: travel"), S(W.Hud->GetRealmButton(1)->GetLabel()), FString::Printf(TEXT("Travel: %s"), *Next));
	TestFalse(TEXT("and pressable"), W.Hud->GetRealmButton(1)->IsLocked());
	int32 Gated = INDEX_NONE;
	for (int32 I = 0; I < static_cast<int32>(D.Tiers.size()) && Gated == INDEX_NONE; ++I)
	{
		if (!Pc::IsTierUnlocked(D, P, D.Tiers[static_cast<size_t>(I)].RequiredEvolutionStage))
		{
			Gated = I;
		}
	}
	if (TestTrue(TEXT("a stage-gated biome exists"), Gated != INDEX_NONE))
	{
		TestTrue(TEXT("a stage-gated row says locked"), S(W.Hud->GetRealmButton(Gated)->GetLabel()).EndsWith(TEXT(" — locked")));
	}

	// The press travels: the realm swaps its field, and the rows follow.
	W.Hud->PressRealm(1);
	TestEqual(TEXT("travelled"), W.Realm->GetSim().Field.TierIndex, int64_t(1));
	TestEqual(TEXT("the progress is there"), FString(UTF8_TO_TCHAR(P.CurrentBiomeId.c_str())), FString(UTF8_TO_TCHAR(D.Tiers[1].Id.c_str())));
	TestTrue(TEXT("the row now says here"), S(W.Hud->GetRealmButton(1)->GetLabel()).EndsWith(TEXT(" — you are here")));
	W.Hud->PressRealm(Gated);
	TestEqual(TEXT("a locked row does not travel"), W.Realm->GetSim().Field.TierIndex, int64_t(1));

	// The auto-travel box: hidden until the tree's Auto-Travel node is owned (hud.gd), then it writes the realm's setting.
	TestEqual(TEXT("no auto-travel box before the node"), W.Hud->GetAutoTravelButton()->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("and not in the column"), W.Hud->PageColumn().Contains(W.Hud->GetAutoTravelButton()));
	P.BranchLevels["auto_travel"] = 1;
	W.Hud->RefreshNow();
	TestEqual(TEXT("the box once the node is owned"), W.Hud->GetAutoTravelButton()->GetVisibility(), ESlateVisibility::Visible);
	TestTrue(TEXT("and in the column"), W.Hud->PageColumn().Contains(W.Hud->GetAutoTravelButton()));
	const bool bBefore = W.Realm->GetAutoTravel();
	W.Hud->ToggleAutoTravel();
	TestEqual(TEXT("auto-travel toggled into the realm"), W.Realm->GetAutoTravel(), !bBefore);
	TestEqual(TEXT("and the box shows it"), W.Hud->GetAutoTravelButton()->IsTicked(), !bBefore);
	// The running field gets it: the slot's value reaches the field's input on the next frame (the field reads only that).
	W.Realm->AdvanceFrame(FPawRealmSim::Step);
	TestEqual(TEXT("the field runs with the toggled value"), W.Realm->GetSim().Input.bAutoTravelSetting, !bBefore);
	W.Hud->ToggleAutoTravel();

	// Navigation walks the page's column, locked rows included (the player reads them).
	const TArray<UWidget*> Column = W.Hud->PageColumn();
	TestTrue(TEXT("the column holds every row, locked ones too"), Column.Contains(W.Hud->GetRealmButton(Gated)) && Column.Contains(W.Hud->GetRealmButton(1)));
	UWidget* Target = nullptr;
	const int32 At = Column.IndexOfByKey(W.Hud->GetRealmButton(Gated));
	if (At > 0)
	{
		TestEqual(TEXT("a locked row's up is explicit"), Rule(Column[At], EUINavigation::Up, &Target), EUINavigationRule::Explicit);
		TestTrue(TEXT("to the row above"), Target == Column[At - 1]);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudReawakenTest, "Pawpocalypse.UI.Hud.Reawaken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudReawakenTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();
	if (W.Hud->GetAskReawakenButton()->IsTicked() == false)
	{
		W.Hud->ToggleAskBeforeReawaken(); // the default: ask
	}
	W.Hud->SelectTab(Pc::PcDockTab::Reawaken);
	TestEqual(TEXT("a fresh game is not worth it"), S(W.Hud->GetReawakenButton()->GetLabel()),
		FString::Printf(TEXT("Not worth it yet — only +%s"), *S(PawText::Amount(Pc::PendingStardust(D, P)))));
	TestTrue(TEXT("and is locked"), W.Hud->GetReawakenButton()->IsLocked());
	TestTrue(TEXT("the progress line counts toward the first"), S(W.Hud->GetReawakenProgressText()).EndsWith(TEXT("earned toward your first Reawaken")));
	W.Hud->PressReawaken();
	TestEqual(TEXT("a locked press does nothing"), P.ReawakenCount, int64_t(0));

	// Worth it: the payout and the multiplier, then the button.
	P.LifetimeCatnipEver = 1e12;
	P.LifetimeCatnipThisCycle = 1e12;
	P.Catnip = 1e12;
	P.UpgradeLevels["paw_damage"] = 7;
	const double Pending = Pc::PendingStardust(D, P);
	P.Stardust = Pending; // as big as the payout, so the total reads twice it (a few Stardust would vanish in "1.0M")
	W.Hud->RefreshNow();
	TestEqual(TEXT("the payout"), S(W.Hud->GetPayoutText()), FString::Printf(TEXT("+%s"), *S(PawText::Amount(Pending))));
	TestEqual(TEXT("the total after it"), S(W.Hud->GetTotalText()), FString::Printf(TEXT("Total: %s"), *S(PawText::Amount(2.0 * Pending))));
	TestNotEqual(TEXT("which differs from the payout"), S(W.Hud->GetTotalText()), FString::Printf(TEXT("Total: %s"), *S(PawText::Amount(Pending))));
	TestTrue(TEXT("the multiplier now and after"), S(W.Hud->GetMultiplierText()).StartsWith(TEXT("Damage multiplier 1.00x  →  ")));
	TestEqual(TEXT("the button"), S(W.Hud->GetReawakenButton()->GetLabel()), FString(TEXT("Reawaken (Reawaken 0)")));
	TestFalse(TEXT("unlocked"), W.Hud->GetReawakenButton()->IsLocked());

	// The first press arms; the selection moves to Cancel; nothing has happened yet.
	W.Hud->PressReawaken();
	TestTrue(TEXT("armed"), W.Hud->IsReawakenArmed());
	TestEqual(TEXT("the button asks again"), S(W.Hud->GetReawakenButton()->GetLabel()), FString(TEXT("Confirm — this resets your life")));
	TestTrue(TEXT("Cancel appears"), Shown(W.Hud->GetCancelReawakenButton()));
	TestTrue(TEXT("the selection is on Cancel, not under the key"), W.Hud->GetDesiredFocusTarget() == W.Hud->GetCancelReawakenButton());
	TestEqual(TEXT("not reawakened yet"), P.ReawakenCount, int64_t(0));
	W.Hud->CancelReawaken();
	TestFalse(TEXT("Cancel disarms"), W.Hud->IsReawakenArmed());
	TestFalse(TEXT("and hides itself"), Shown(W.Hud->GetCancelReawakenButton()));
	// Looking away disarms too.
	W.Hud->PressReawaken();
	W.Hud->SelectTab(Pc::PcDockTab::Upgrades);
	W.Hud->SelectTab(Pc::PcDockTab::Reawaken);
	TestFalse(TEXT("a tab change disarms"), W.Hud->IsReawakenArmed());

	// Travelled away first, so the reset has somewhere to bring the cat back from.
	P.LifetimeCatnipThisCycle = 1e12;
	TestTrue(TEXT("travel to the second biome"), W.Realm->Travel(1));
	const double StardustBefore = P.Stardust;
	TestFalse(TEXT("Nine Lives not earned before"), Pc::IsAchievementEarned(P, "reawaken_1"));
	// The ceremony plays first (Pawpocalypse.UI.Ceremony.*); this world has no Menu stack, so its end travels at once.
	auto PlayOutCeremony = [&W]()
	{
		for (int32 F = 0; F < 400 && W.Hud->GetCeremony()->IsPlaying(); ++F)
		{
			W.Hud->Pump(1.0 / 60.0);
		}
	};
	W.Hud->PressReawaken();
	W.Hud->PressReawaken();
	TestEqual(TEXT("the second press reawakens"), P.ReawakenCount, int64_t(1));
	TestEqual(TEXT("no travel while the ceremony plays"), W.Realm->GetSim().Field.TierIndex, int64_t(1));
	PlayOutCeremony();
	// The realm is told (TierController._on_reawakened, the `reawakened` signal): the travel that follows rebuilds the
	// field, so what only the signal does is earn the Reawaken's achievements, at once, before any step.
	TestTrue(TEXT("Nine Lives earned by the Reawaken itself"), Pc::IsAchievementEarned(P, "reawaken_1"));
	TestTrue(TEXT("the Stardust landed"), P.Stardust > StardustBefore);
	TestEqual(TEXT("the base upgrades reset"), Pc::UpgradeLevel(P, "paw_damage"), Pc::StartingUpgradeLevel(D, P));
	TestEqual(TEXT("back in the first biome"), W.Realm->GetSim().Field.TierIndex, int64_t(0));
	TestFalse(TEXT("disarmed after"), W.Hud->IsReawakenArmed());

	// "Ask before Reawakening" off: one press does it.
	W.Hud->ToggleAskBeforeReawaken();
	TestFalse(TEXT("the box unticks"), W.Hud->GetAskReawakenButton()->IsTicked());
	P.LifetimeCatnipEver = 1e14;
	// Kittens bought this life follow the reset. (The reset clears the current biome, so the return to the first biome
	// always runs, as Godot's _travel_to_first_biome does from inside it too: that travel re-counts them.)
	P.UpgradeLevels["kitten_count"] = 5;
	W.Realm->OnProgressChanged(true);
	TestEqual(TEXT("five kittens out"), W.Realm->GetSim().Field.AwakeKittens(), static_cast<int64_t>(Pc::KittenCount(D, P)));
	W.Hud->RefreshNow();
	W.Hud->PressReawaken();
	TestEqual(TEXT("one press with the box off"), P.ReawakenCount, int64_t(2));
	TestEqual(TEXT("the kittens follow the reset"), W.Realm->GetSim().Field.AwakeKittens(), static_cast<int64_t>(Pc::KittenCount(D, P)));
	TestTrue(TEXT("fewer than before it"), Pc::KittenCount(D, P) < 5);

	// Territory wins (designer 2026-11-01): the next run opens in the starting biome, whether the Reawaken happened
	// deeper than it or shallower, and never in the first biome. (With no Menu stack there is no spend step, so the HUD
	// travels when the ceremony ends, as Godot's _travel_to_starting_biome does with no tree in the scene;
	// tools/check_godot_reawaken_travel.py.)
	Pc::CheatUnlockAllTiers(D, P);
	P.BranchLevels["legacy"] = 10; // Legacy level 10: start in biome 6
	const int64 Start = Pc::StartingBiomeIndex(D, P) - 1;
	TestEqual(TEXT("Territory asks for the sixth biome"), Start, int64(5));
	int64 Count = P.ReawakenCount;
	for (const int32 From : { 7, 1, 5 }) // deeper, shallower, already there
	{
		P.LifetimeCatnipThisCycle = 1e30;
		TestTrue(TEXT("travel before the Reawaken"), W.Realm->Travel(From));
		P.LifetimeCatnipEver *= 100.0;
		W.Hud->RefreshNow();
		W.Hud->PressReawaken();
		TestEqual(TEXT("reawakened with Territory"), P.ReawakenCount, ++Count);
		PlayOutCeremony();
		TestEqual(FString::Printf(TEXT("from biome %d: in the sixth biome at once"), From + 1),
			W.Realm->GetSim().Field.TierIndex, Start);
		TestEqual(TEXT("and the progress says so"), FString(UTF8_TO_TCHAR(P.CurrentBiomeId.c_str())),
			FString(UTF8_TO_TCHAR(D.Tiers[static_cast<size_t>(Start)].Id.c_str())));
		for (int32 F = 0; F < 200; ++F)
		{
			W.Realm->AdvanceFrame(FPawRealmSim::Step);
		}
		TestEqual(TEXT("and stays there (no late travel)"), W.Realm->GetSim().Field.TierIndex, Start);
	}
	W.Hud->ToggleAskBeforeReawaken(); // back to the default for the next test
	W.Close();
	return true;
}

namespace
{
	int32 CardIndex(const Pc::PcGameData& D, const char* Id)
	{
		for (int32 I = 0; I < static_cast<int32>(D.Cards.size()); ++I)
		{
			if (D.Cards[static_cast<size_t>(I)].Id == Id)
			{
				return I;
			}
		}
		return INDEX_NONE;
	}

	int32 JobIndex(const Pc::PcGameData& D, const char* Id)
	{
		const std::vector<const Pc::PcJob*> All = Pc::AllJobs(D);
		for (int32 I = 0; I < static_cast<int32>(All.size()); ++I)
		{
			if (All[static_cast<size_t>(I)]->Id == Id)
			{
				return I;
			}
		}
		return INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudCatchesTest, "Pawpocalypse.UI.Hud.Catches", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudCatchesTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();
	W.Hud->SelectTab(Pc::PcDockTab::Catches);
	TestEqual(TEXT("nothing taken"), S(W.Hud->GetCatchesSummaryText()), FString(TEXT("No cards taken yet this life. Catch mice to fill Catches.")));
	TestFalse(TEXT("no auto-pick box without Auto-Card"), Shown(W.Hud->GetAutoPickButton()));
	TestNotNull(TEXT("a row for the last card"), W.Hud->GetCardRow(static_cast<int32>(D.Cards.size()) - 1));
	TestNull(TEXT("and no more"), W.Hud->GetCardRow(static_cast<int32>(D.Cards.size())));

	// Every card, not only the taken ones: what stands in the way, as an instruction.
	const int32 Bounty = CardIndex(D, "bounty");
	const int32 Blood = CardIndex(D, "bloodlust");
	const int32 Close = CardIndex(D, "close_quarters");
	const int32 Lone = CardIndex(D, "lone_hunter");
	if (!TestTrue(TEXT("the cards are in the data"), Bounty >= 0 && Blood >= 0 && Close >= 0 && Lone >= 0))
	{
		W.Close();
		return false;
	}
	TestEqual(TEXT("a card not taken: its name"), S(W.Hud->GetCardRow(Bounty)->GetLabel()), FString(TEXT("Bounty")));
	TestEqual(TEXT("nothing in its way"), S(W.Hud->GetCardRow(Bounty)->GetDetail()), FString(TEXT("Not taken yet.")));
	TestEqual(TEXT("a Pounce card names the branch"), S(W.Hud->GetCardRow(Blood)->GetDetail()), FString(TEXT("Buy the Pounce branch in the skill tree.")));
	TestEqual(TEXT("a kitten card names the kitten"), S(W.Hud->GetCardRow(Lone)->GetDetail()), FString(TEXT("Have at least one kitten.")));

	// Taken: the heading carries the level, the body what it does at that level ({v} and {c} filled, no trailing zeros).
	P.CardLevels["close_quarters"] = 1;
	W.Hud->RefreshNow();
	TestEqual(TEXT("one card"), S(W.Hud->GetCatchesSummaryText()), FString(TEXT("1 card taken this life.")));
	TestEqual(TEXT("a held card's heading"), S(W.Hud->GetCardRow(Close)->GetLabel()), FString(TEXT("Close Quarters  (Lv 1/2)")));
	TestEqual(TEXT("a held card's body"), S(W.Hud->GetCardRow(Close)->GetDetail()),
		FString(TEXT("Your aura detonates for 5 seconds of damage, but shrinks to 0.7x its radius.")));
	P.CardLevels["bounty"] = 2;
	P.BanishedCards.insert("spite");
	W.Hud->RefreshNow();
	TestEqual(TEXT("two cards"), S(W.Hud->GetCatchesSummaryText()), FString(TEXT("2 cards taken this life.")));
	TestEqual(TEXT("level two of three"), S(W.Hud->GetCardRow(Bounty)->GetLabel()), FString(TEXT("Bounty  (Lv 2/3)")));
	TestTrue(TEXT("its value at level two"), S(W.Hud->GetCardRow(Bounty)->GetDetail()).Contains(TEXT("3")));
	TestEqual(TEXT("a banished card says so"), S(W.Hud->GetCardRow(CardIndex(D, "spite"))->GetDetail()), FString(TEXT("Removed from this run.")));

	// Auto-Card owned: the box appears; it writes the realm's setting and the summary says so.
	P.BranchLevels["auto_card"] = 1;
	W.Hud->RefreshNow();
	TestTrue(TEXT("the auto-pick box with Auto-Card"), Shown(W.Hud->GetAutoPickButton()));
	const bool bBefore = W.Realm->GetAutoPick();
	W.Hud->ToggleAutoPick();
	TestEqual(TEXT("auto-pick toggled into the realm"), W.Realm->GetAutoPick(), !bBefore);
	TestEqual(TEXT("and the box shows it"), W.Hud->GetAutoPickButton()->IsTicked(), !bBefore);
	TestEqual(TEXT("the summary says it picks"), S(W.Hud->GetCatchesSummaryText()), FString(TEXT("2 cards taken this life. Picking automatically.")));
	W.Hud->ToggleAutoPick();
	TestFalse(TEXT("and stops saying it"), S(W.Hud->GetCatchesSummaryText()).Contains(TEXT("Picking")));

	// The pad reads every row: the box, then every card, none of them locked away.
	const TArray<UWidget*> Column = W.Hud->PageColumn();
	TestEqual(TEXT("the column: the box and every card"), Column.Num(), static_cast<int32>(D.Cards.size()) + 1);
	TestTrue(TEXT("the box first"), Column.Num() > 0 && Column[0] == W.Hud->GetAutoPickButton());
	TestTrue(TEXT("a card not taken is in it"), Column.Contains(W.Hud->GetCardRow(Blood)));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudAchievementsTest, "Pawpocalypse.UI.Hud.Achievements", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudAchievementsTest::RunTest(const FString& Parameters)
{
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();
	W.Hud->SelectTab(Pc::PcDockTab::Achievements);
	const int32 Total = static_cast<int32>(D.Achievements.size());
	TestEqual(TEXT("nothing earned"), S(W.Hud->GetAchievementSummaryText()), FString::Printf(TEXT("0 / %d earned"), Total));
	TestFalse(TEXT("no Claim all"), Shown(W.Hud->GetClaimAllButton()));
	const Pc::PcAchievement& A = D.Achievements[0];
	const Pc::PcAchievement& B = D.Achievements[1];
	TestEqual(TEXT("an unearned row: its name"), S(W.Hud->GetAchievementText(0, false)), S(PawText::Data(TEXT("ACH"), A.Id)));
	TestEqual(TEXT("and its progress"), S(W.Hud->GetAchievementText(0, true)),
		FString::Printf(TEXT("%s / %s"), *S(PawText::Amount(Pc::AchievementProgress(D, P, A))), *S(PawText::Amount(A.Threshold))));
	TestFalse(TEXT("no Claim on it"), Shown(W.Hud->GetClaimButton(0)));
	TestTrue(TEXT("muted"), W.Hud->GetAchievementMark(0).Equals(PawTokens::UI_TEXT_MUTED));
	TestEqual(TEXT("an empty column falls back to the tab"), W.Hud->GetDesiredFocusTarget(), static_cast<UWidget*>(W.Hud->GetTabButton(Pc::PcDockTab::Achievements)));

	// Earned: the description, the good colour, a Claim with its amount; the summary counts what waits.
	P.AchievementsEarned.insert(A.Id);
	P.AchievementsEarned.insert(B.Id);
	W.Hud->RefreshNow();
	const double Both = A.StardustReward + B.StardustReward;
	TestEqual(TEXT("two waiting"), S(W.Hud->GetAchievementSummaryText()),
		FString::Printf(TEXT("2 / %d earned — 2 rewards waiting, %s"), Total, *S(PawText::Amount(Both))));
	TestEqual(TEXT("Claim all with the sum"), S(W.Hud->GetClaimAllButton()->GetLabel()), FString::Printf(TEXT("Claim all   %s"), *S(PawText::Amount(Both))));
	TestTrue(TEXT("a Claim on the earned row"), Shown(W.Hud->GetClaimButton(0)));
	TestEqual(TEXT("with its amount"), S(W.Hud->GetClaimButton(0)->GetLabel()), FString::Printf(TEXT("Claim %s"), *S(PawText::Amount(A.StardustReward))));
	TestEqual(TEXT("the description"), S(W.Hud->GetAchievementText(0, true)), S(PawText::DataField(TEXT("ACH"), A.Id, TEXT("desc"))));
	TestTrue(TEXT("the good colour"), W.Hud->GetAchievementMark(0).Equals(PawTokens::UI_GOOD));
	TArray<UWidget*> Column = W.Hud->PageColumn();
	TestEqual(TEXT("the column: Claim all and both Claims"), Column.Num(), 3);
	TestTrue(TEXT("Claim all first"), Column.Num() > 0 && Column[0] == W.Hud->GetClaimAllButton());

	// One Claim pays exactly once.
	const double Before = P.Stardust;
	W.Hud->PressClaim(0);
	TestEqual(TEXT("the Stardust landed"), P.Stardust, Before + A.StardustReward);
	W.Hud->PressClaim(0);
	TestEqual(TEXT("a second press pays nothing"), P.Stardust, Before + A.StardustReward);
	TestFalse(TEXT("its Claim is gone"), Shown(W.Hud->GetClaimButton(0)));
	TestTrue(TEXT("claimed: the Upgrades colour"), W.Hud->GetAchievementMark(0).Equals(PawTokens::UI_TAB_UPGRADES));
	TestEqual(TEXT("one waiting"), S(W.Hud->GetAchievementSummaryText()),
		FString::Printf(TEXT("2 / %d earned — 1 reward waiting, %s"), Total, *S(PawText::Amount(B.StardustReward))));
	TestEqual(TEXT("the Stardust readout follows"), S(W.Hud->GetStardustText()), S(PawText::Amount(P.Stardust)));

	// Claim all takes the rest, once; the page settles.
	W.Hud->PressClaimAll();
	TestEqual(TEXT("Claim all paid the rest"), P.Stardust, Before + Both);
	W.Hud->PressClaimAll();
	TestEqual(TEXT("and nothing more"), P.Stardust, Before + Both);
	TestFalse(TEXT("Claim all goes"), Shown(W.Hud->GetClaimAllButton()));
	TestEqual(TEXT("nothing waits"), S(W.Hud->GetAchievementSummaryText()), FString::Printf(TEXT("2 / %d earned"), Total));
	TestEqual(TEXT("the column is empty again"), W.Hud->PageColumn().Num(), 0);

	// Earned by the game, not only by the test: a counter past its threshold is earned by the realm's next step (Godot
	// re-scans on every kill and currency change), and by a progress change while the field is still (a purchase while
	// paused). Each lands once, in the realm's earn list.
	const Pc::PcAchievement* ByCatnip = nullptr;
	const Pc::PcAchievement* ByLevel = nullptr;
	for (const Pc::PcAchievement& X : D.Achievements)
	{
		if (!ByCatnip && X.Metric == Pc::AchievementMetric::LIFETIME_CATNIP && !Pc::IsAchievementEarned(P, X.Id))
		{
			ByCatnip = &X;
		}
		if (!ByLevel && X.Metric == Pc::AchievementMetric::UPGRADE_LEVEL && !Pc::IsAchievementEarned(P, X.Id))
		{
			ByLevel = &X;
		}
	}
	if (TestNotNull(TEXT("a Catnip achievement"), ByCatnip) && TestNotNull(TEXT("an upgrade-level achievement"), ByLevel))
	{
		const size_t EarnedBefore = W.Realm->GetSim().EarnedAchievements.size();
		P.LifetimeCatnipEver = std::max(P.LifetimeCatnipEver, ByCatnip->Threshold);
		TestFalse(TEXT("not before the game looks"), Pc::IsAchievementEarned(P, ByCatnip->Id));
		W.Realm->AdvanceFrame(FPawRealmSim::Step);
		TestTrue(TEXT("earned by the next step"), Pc::IsAchievementEarned(P, ByCatnip->Id));
		W.Realm->SetPaused(true);
		P.UpgradeLevels[ByLevel->UpgradeId] = static_cast<int64>(ByLevel->Threshold);
		W.Realm->AdvanceFrame(FPawRealmSim::Step);
		TestFalse(TEXT("a paused field does not step"), Pc::IsAchievementEarned(P, ByLevel->Id));
		W.Realm->OnProgressChanged(false);
		TestTrue(TEXT("a purchase while paused earns at once"), Pc::IsAchievementEarned(P, ByLevel->Id));
		W.Realm->SetPaused(false);
		W.Realm->AdvanceFrame(FPawRealmSim::Step);
		TestEqual(TEXT("each earned once"), W.Realm->GetSim().EarnedAchievements.size(), EarnedBefore + 2);
		W.Hud->RefreshNow();
		TestTrue(TEXT("and the page offers its Claim"), Shown(W.Hud->GetClaimAllButton()));
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawHudJobsTest, "Pawpocalypse.UI.Hud.Jobs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawHudJobsTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FHudWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const Pc::PcGameData& D = W.D();
	Pc::PcProgress& P = W.P();
	// The tab shows once a job is held (hud.gd _build_tabs): one the rows below do not read.
	P.JobsHeld.insert("town_crier");
	W.Hud->RefreshNow();
	W.Hud->SelectTab(Pc::PcDockTab::Jobs);
	TestTrue(TEXT("the Jobs page is open"), W.Hud->GetActiveTab() == Pc::PcDockTab::Jobs);
	const int32 Smith = JobIndex(D, "blacksmith");
	const int32 Magic = JobIndex(D, "magician");
	const int32 Idle = JobIndex(D, "unemployed");
	const int32 Dare = JobIndex(D, "daredevil");
	if (!TestTrue(TEXT("the jobs are in the data"), Smith >= 0 && Magic >= 0 && Idle >= 0 && Dare >= 0))
	{
		W.Close();
		return false;
	}
	TestEqual(TEXT("a job's name"), S(W.Hud->GetJobText(Smith, false)), FString(TEXT("Blacksmith")));
	TestEqual(TEXT("gated by a boss"), S(W.Hud->GetJobText(Smith, true)),
		FString::Printf(TEXT("Locked — beat the boss in %s, then buy it in the skill tree."), *S(PawText::Data(TEXT("TIER"), "tier_6"))));
	TestEqual(TEXT("gated by one Reawaken"), S(W.Hud->GetJobText(Magic, true)), FString(TEXT("Locked — Reawaken once, then buy it in the skill tree.")));
	TestEqual(TEXT("gated by three"), S(W.Hud->GetJobText(Idle, true)), FString(TEXT("Locked — Reawaken 3 times, then buy it in the skill tree.")));
	TestEqual(TEXT("the state says locked"), S(W.Hud->GetJobButton(Smith)->GetLabel()), FString(TEXT("Locked")));
	TestTrue(TEXT("a statement, never a control"), W.Hud->GetJobButton(Smith)->IsLocked());

	// The boss down: ready, at the node's price.
	P.BossesDefeated.insert("tier_6");
	W.Hud->RefreshNow();
	TestEqual(TEXT("ready at its price"), S(W.Hud->GetJobText(Smith, true)),
		FString::Printf(TEXT("Ready — buy it in the skill tree for %s Stardust."),
			*S(PawText::Count(static_cast<int64>(Pc::NextBranchCost(D, P, "job_blacksmith"))))));
	// The rival bought closes it.
	P.BranchLevels["job_daredevil"] = 1;
	W.Hud->RefreshNow();
	TestEqual(TEXT("closed by the rival"), S(W.Hud->GetJobText(Smith, true)),
		FString::Printf(TEXT("Closed — you took %s instead."), *S(PawText::Data(TEXT("SKILL"), "job_daredevil"))));

	// Held: the description; worn when it is the run's job.
	P.JobsHeld.insert("daredevil");
	P.CurrentJob = "daredevil";
	W.Hud->RefreshNow();
	TestEqual(TEXT("held: its description"), S(W.Hud->GetJobText(Dare, true)), S(PawText::DataField(TEXT("JOB"), "daredevil", TEXT("desc"))));
	TestEqual(TEXT("owned, not worn (only preferred)"), S(W.Hud->GetJobButton(Dare)->GetLabel()), FString(TEXT("Owned")));
	P.RunJob = "daredevil";
	W.Hud->RefreshNow();
	TestEqual(TEXT("worn: locked in for this run"), S(W.Hud->GetJobButton(Dare)->GetLabel()), FString(TEXT("Worn")));
	TestTrue(TEXT("still a statement"), W.Hud->GetJobButton(Dare)->IsLocked());

	// The pad reads every row, locked ones included.
	const TArray<UWidget*> Column = W.Hud->PageColumn();
	TestEqual(TEXT("the column: every job"), Column.Num(), static_cast<int32>(Pc::AllJobs(D).size()));
	TestTrue(TEXT("a locked job is in it"), Column.Contains(W.Hud->GetJobButton(Magic)));
	W.Close();
	return true;
}

#endif
