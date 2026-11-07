// Pawpocalypse.UI.Juice (phase 6 slice 5c): the UI's motion where Godot's UiJuice puts it. The HUD: the hover pop on the upgrade
// rows, the job buttons, the tabs, the travel rows and the claim buttons (and on nothing else), not on a locked row, the press
// squash, the purchase punch; the Reawaken tab's lean and glow while a Reawaken is suggested (not while its page is open, not
// with the effects off); a newly open biome's row and the zone plate lean. The tree: nodes pop only when they can be bought,
// READY nodes lean in the spend step only, a bought node punches, the leaving plate pops in. The title: the cluster pops.
// The motion is a render transform about the centre (layout never moves); the curves are PcJuice's (L1, fixture `juice`).

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Components/Border.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "PcEconomy.h"
#include "PcHud.h"
#include "PcTreeLayout.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawJuice.h"
#include "UI/PawMainMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawWidgets.h"

namespace
{
	struct FJuiceWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawRootLayout* Root = nullptr;
		UPawHudScreen* Hud = nullptr;
		TSharedPtr<SWidget> RootSlate;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			if (!T.TestNotNull(TEXT("the realm"), Realm) || !T.TestNotNull(TEXT("the game"), Game) || !T.TestTrue(TEXT("data"), Game->IsDataReady()))
			{
				return false;
			}
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
			Hud->SelectTab(Pc::PcDockTab::Upgrades);
			Hud->RefreshNow();
			return true;
		}

		void Close()
		{
			RootSlate.Reset();
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
		UPawSettingsSubsystem* Settings() { return GI->GetSubsystem<UPawSettingsSubsystem>(); }
	};

	bool Near(double A, double B) { return FMath::Abs(A - B) < 1e-4; }
	double ScaleX(const UWidget* W) { return W->GetRenderTransform().Scale.X; }
	double ScaleY(const UWidget* W) { return W->GetRenderTransform().Scale.Y; }
	double Angle(const UWidget* W) { return W->GetRenderTransform().Angle; }
	bool Upright(const UWidget* W) { return Near(ScaleX(W), 1.0) && Near(ScaleY(W), 1.0) && Near(Angle(W), 0.0); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawJuiceHudTest, "Pawpocalypse.UI.Juice.Hud", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawJuiceHudTest::RunTest(const FString& Parameters)
{
	FJuiceWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawHudScreen* Hud = W.Hud;
	FPawJuice& J = Hud->GetJuice();

	// Wired where hud.gd calls UiJuice.hover, and nowhere else.
	int32 Wired = 0;
	int32 Expected = 0;
	for (int32 I = 0; Hud->GetRowButton(I); ++I, ++Expected) { Wired += J.IsHoverWired(Hud->GetRowButton(I)) ? 1 : 0; }
	for (int32 I = 0; Hud->GetJobButton(I); ++I, ++Expected) { Wired += J.IsHoverWired(Hud->GetJobButton(I)) ? 1 : 0; }
	for (int32 I = 0; Hud->GetRealmButton(I); ++I, ++Expected) { Wired += J.IsHoverWired(Hud->GetRealmButton(I)) ? 1 : 0; }
	for (int32 I = 0; Hud->GetClaimButton(I); ++I, ++Expected) { Wired += J.IsHoverWired(Hud->GetClaimButton(I)) ? 1 : 0; }
	for (int32 I = 0; I < 6; ++I, ++Expected) { Wired += J.IsHoverWired(Hud->GetTabButton(static_cast<Pc::PcDockTab>(I))) ? 1 : 0; }
	TestTrue(TEXT("every upgrade row, job button, travel row and claim button is there"), Expected > 6 + 20);
	TestEqual(TEXT("each of them pops on hover (UiJuice.hover)"), Wired, Expected);
	TestFalse(TEXT("not Bait"), J.IsHoverWired(Hud->GetBaitButton()));
	TestFalse(TEXT("not the multiplier"), J.IsHoverWired(Hud->GetMultiplierButton()));
	TestFalse(TEXT("not the quick bar"), J.IsHoverWired(Hud->GetQuickButton(0)));
	TestFalse(TEXT("not Claim all"), J.IsHoverWired(Hud->GetClaimAllButton()));

	// The hover pop through the button's own events: 1.06 about the centre, back to 1 and let go.
	W.P().Catnip = 1e12;
	Hud->RefreshNow();
	UPawButton* Row = Hud->GetRowButton(0);
	TestFalse(TEXT("an affordable row is live"), Row->IsLocked());
	Row->OnHovered().Broadcast();
	Hud->Pump(0.05);
	TestTrue(TEXT("growing"), ScaleX(Row) > 1.0 && ScaleX(Row) < Pc::JuiceHoverScale + 0.05);
	Hud->Pump(1.0);
	TestTrue(TEXT("hovered: HOVER_SCALE"), Near(ScaleX(Row), Pc::JuiceHoverScale) && Near(ScaleY(Row), Pc::JuiceHoverScale));
	TestTrue(TEXT("about the centre"), Row->GetRenderTransformPivot().Equals(FVector2D(0.5, 0.5)));
	Row->OnPressed().Broadcast();
	Hud->Pump(1.0);
	TestTrue(TEXT("pressed: PRESS_SCALE"), Near(ScaleX(Row), Pc::JuicePressScale));
	Row->OnReleased().Broadcast();
	Hud->Pump(1.0);
	TestTrue(TEXT("released (no pointer on it): rest"), Near(ScaleX(Row), 1.0));
	Row->OnHovered().Broadcast();
	Hud->Pump(1.0);
	Row->OnUnhovered().Broadcast();
	Hud->Pump(1.0);
	TestTrue(TEXT("unhovered: rest"), Upright(Row));
	TestNull(TEXT("and let go once at rest"), J.Find(Row));

	// A locked row does not grow (_on_enter: a disabled button).
	W.P().Catnip = 0.0;
	Hud->RefreshNow();
	TestTrue(TEXT("a row nothing can buy is locked"), Row->IsLocked());
	Row->OnHovered().Broadcast();
	Hud->Pump(1.0);
	TestTrue(TEXT("a locked row stays still"), Upright(Row));
	Row->OnUnhovered().Broadcast();
	Hud->Pump(1.0);

	// The purchase punch: wide and short, then elastically back.
	W.P().Catnip = 1e12;
	Hud->RefreshNow();
	Hud->PressRow(0);
	Hud->Pump(0.05);
	TestTrue(TEXT("a purchase punches the row: wider"), ScaleX(Row) > 1.02);
	TestTrue(TEXT("and shorter"), ScaleY(Row) < 0.98);
	Hud->Pump(1.0);
	TestTrue(TEXT("and settles"), Upright(Row));
	W.P().Catnip = 0.0;
	Hud->PressRow(0);
	Hud->Pump(0.05);
	TestTrue(TEXT("nothing bought: no punch"), Upright(Row));

	// The Reawaken tab leans and glows while a Reawaken is suggested; not while its page is open; no lean with the effects off.
	UPawButton* Tab = Hud->GetTabButton(Pc::PcDockTab::Reawaken);
	Hud->Pump(0.02);
	TestFalse(TEXT("no suggestion: the tab is still"), J.IsJittering(Tab));
	TestTrue(TEXT("and white"), Tab->GetColorAndOpacity().Equals(FLinearColor::White));
	W.P().LifetimeCatnipEver = 1e14;
	TestTrue(TEXT("a Reawaken is suggested"), Pc::IsReawakenSuggested(W.D(), W.P()));
	Hud->RefreshNow();
	Hud->Pump(0.02);
	TestTrue(TEXT("the Reawaken tab leans"), J.IsJittering(Tab));
	TestFalse(TEXT("and glows (not white)"), Tab->GetColorAndOpacity().Equals(FLinearColor::White));
	Hud->Pump(0.2);
	TestTrue(TEXT("leaning within JITTER_DEG"), !Near(Angle(Tab), 0.0) && FMath::Abs(Angle(Tab)) <= Pc::JuiceJitterDeg + 1e-3);
	TestTrue(TEXT("the render angle is the lean, in degrees"), J.Find(Tab) && Near(Angle(Tab), FMath::RadiansToDegrees(J.Find(Tab)->Rotation)));
	Hud->SelectTab(Pc::PcDockTab::Reawaken);
	Hud->Pump(0.02);
	TestFalse(TEXT("its page open: no lean"), J.IsJittering(Tab));
	Hud->Pump(1.0);
	TestTrue(TEXT("eased back upright"), Near(Angle(Tab), 0.0));
	Hud->SelectTab(Pc::PcDockTab::Upgrades);
	W.Settings()->SetEffectsEnabled(false);
	Hud->Pump(0.02);
	TestFalse(TEXT("effects off: no lean"), J.IsJittering(Tab));
	TestFalse(TEXT("the glow stays (a colour, not motion)"), Tab->GetColorAndOpacity().Equals(FLinearColor::White));
	W.Settings()->SetEffectsEnabled(true);
	Hud->Pump(0.02);
	TestTrue(TEXT("effects on: it leans again"), J.IsJittering(Tab));
	W.P().LifetimeCatnipEver = 0.0;
	Hud->Pump(0.02);
	TestFalse(TEXT("no suggestion: still"), J.IsJittering(Tab));
	TestTrue(TEXT("and white again"), Tab->GetColorAndOpacity().Equals(FLinearColor::White));

	// A biome just become reachable: its row and the zone plate lean (one thing at a time on a page).
	Hud->SelectTab(Pc::PcDockTab::Realms);
	Hud->RefreshNow();
	UPawButton* Next = Hud->GetRealmButton(1);
	TestFalse(TEXT("the next biome is shut: still"), J.IsJittering(Next));
	W.P().LifetimeCatnipThisCycle += W.D().Tiers[1].CatnipToUnlock + 1.0;
	Hud->RefreshNow();
	TestTrue(TEXT("the newly open biome's row leans"), J.IsJittering(Next));
	TestFalse(TEXT("the row it stands beside does not"), J.IsJittering(Hud->GetRealmButton(0)));
	TestTrue(TEXT("the zone plate leans while it shouts"), J.IsJittering(Hud->GetZonePlate()));
	Hud->Pump(0.2);
	TestTrue(TEXT("the row is leaning"), !Near(Angle(Next), 0.0));
	TestTrue(TEXT("its size untouched (scale is the hover's)"), Near(ScaleX(Next), 1.0));

	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawJuiceTreeTest, "Pawpocalypse.UI.Juice.Tree", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawJuiceTreeTest::RunTest(const FString& Parameters)
{
	// Browsing: nothing leans and nothing pops (every node is `disabled` outside the spend step).
	{
		FJuiceWorld W;
		if (!W.Open(*this))
		{
			W.Close();
			return false;
		}
		W.P().Stardust = 1e12;
		TestTrue(TEXT("the tree opens to browse"), W.Hud->OpenSkillTree(false));
		UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
		UPawTreeNode* Node = Tree ? Tree->GetNode(TEXT("instinct")) : nullptr;
		if (TestNotNull(TEXT("the instinct node"), Node))
		{
			TestTrue(TEXT("it reads READY"), Node->GetState() == Pc::PcTreeNodeState::Ready);
			TestTrue(TEXT("wired for the hover pop"), Tree->GetJuice().IsHoverWired(Node));
			TestFalse(TEXT("but a browse does not lean"), Tree->GetJuice().IsJittering(Node));
			Tree->GetJuice().HandleHovered(Node);
			Tree->GetJuice().Tick(1.0);
			TestTrue(TEXT("nor pop on hover"), Upright(Node));
		}
		W.Close();
	}
	// The spend step: READY nodes lean, a buyable one pops, a bought one punches, leaving asks with a popped-in plate.
	FJuiceWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.P().Stardust = 1e12;
	TestTrue(TEXT("the tree opens to spend"), W.Hud->OpenSkillTree(true));
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	if (!TestTrue(TEXT("in the spend step"), Tree && Tree->IsSpendMode()))
	{
		W.Close();
		return false;
	}
	FPawJuice& J = Tree->GetJuice();
	int32 Ready = 0;
	int32 Leaning = 0;
	int32 Other = 0;
	int32 OtherLeaning = 0;
	for (const Pc::PcSkillBranch& B : W.D().Branches)
	{
		const UPawTreeNode* N = Tree->GetNode(UTF8_TO_TCHAR(B.Id.c_str()));
		const bool bReady = N && N->GetState() == Pc::PcTreeNodeState::Ready;
		Ready += bReady ? 1 : 0;
		Leaning += bReady && J.IsJittering(N) ? 1 : 0;
		Other += !bReady ? 1 : 0;
		OtherLeaning += !bReady && J.IsJittering(N) ? 1 : 0;
	}
	TestTrue(TEXT("some nodes are READY"), Ready > 0);
	TestEqual(TEXT("every READY node leans"), Leaning, Ready);
	TestTrue(TEXT("some are not"), Other > 0);
	TestEqual(TEXT("no other node does"), OtherLeaning, 0);

	UPawTreeNode* Node = Tree->GetNode(TEXT("instinct"));
	J.HandleHovered(Node);
	J.Tick(1.0);
	TestTrue(TEXT("a buyable node pops on hover"), Near(ScaleX(Node), Pc::JuiceHoverScale));
	J.HandleUnhovered(Node);
	J.Tick(1.0);
	// One not buyable now (its parent unbought): no pop.
	UPawTreeNode* Shut = nullptr;
	for (const Pc::PcSkillBranch& B : W.D().Branches)
	{
		if (!Pc::TreeCanBuyNow(W.D(), W.P(), B.Id))
		{
			Shut = Tree->GetNode(UTF8_TO_TCHAR(B.Id.c_str()));
			break;
		}
	}
	if (TestNotNull(TEXT("a node that cannot be bought now"), Shut))
	{
		J.HandleHovered(Shut);
		J.Tick(1.0);
		TestTrue(TEXT("does not pop"), Near(ScaleX(Shut), 1.0));
	}

	TestTrue(TEXT("instinct is bought"), Tree->PressNode(TEXT("instinct")));
	J.Tick(0.05);
	TestTrue(TEXT("the bought node punches: wider"), ScaleX(Node) > 1.02);
	TestTrue(TEXT("and shorter"), ScaleY(Node) < 0.98);
	J.Tick(1.0);
	TestTrue(TEXT("and settles"), Near(ScaleX(Node), 1.0) && Near(ScaleY(Node), 1.0));

	UBorder* Plate = Tree->GetWarningPlate();
	TestTrue(TEXT("no leaving plate yet"), Plate && Plate->GetVisibility() == ESlateVisibility::Collapsed);
	Tree->PressClose();
	TestTrue(TEXT("leaving asks"), Plate && Plate->GetVisibility() != ESlateVisibility::Collapsed);
	TestTrue(TEXT("the plate pops in from POP_IN_FROM"), Plate && Near(ScaleX(Plate), Pc::JuicePopInFrom));
	J.Tick(1.0);
	TestTrue(TEXT("to full size"), Plate && Near(ScaleX(Plate), 1.0));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawJuiceTitleTest, "Pawpocalypse.UI.Juice.Title", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawJuiceTitleTest::RunTest(const FString& Parameters)
{
	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	UPawMainMenu* Menu = CreateWidget<UPawMainMenu>(GI, PawUi::Resolve<UPawMainMenu>());
	if (TestNotNull(TEXT("the title screen"), Menu))
	{
		for (int32 I = 0; I < 3; ++I)
		{
			TestTrue(FString::Printf(TEXT("cluster plate %d pops on hover"), I), Menu->GetJuice().IsHoverWired(Menu->GetClusterButton(I)));
		}
		TestFalse(TEXT("the menu's own buttons do not (main_menu.gd)"), Menu->GetJuice().IsHoverWired(Menu->GetMenuButton(0)));
		UPawButton* B = Menu->GetClusterButton(0);
		B->OnHovered().Broadcast();
		Menu->GetJuice().Tick(1.0);
		TestTrue(TEXT("hovered: HOVER_SCALE"), Near(ScaleX(B), Pc::JuiceHoverScale));
	}
	GI->Shutdown();
	if (World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
	return true;
}

#endif
