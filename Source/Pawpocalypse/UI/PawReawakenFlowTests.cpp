// The Reawaken's screens around the spend step (docs/09 L3; docs/05 §6): the ceremony before it and the job picker after it,
// on the REAL root layout (its Slate built, so the stacks activate). What the picker offers is PcHud JobPicker, pinned in
// PawCore against the real job_picker.gd; these prove the screens and the order hud.gd runs them in.
//
// Pawpocalypse.UI.Ceremony.Tween     the three alphas over the sequence: sine in-out fades at Godot's durations, 0 after.
// Pawpocalypse.UI.Ceremony.Plays     a Reawaken plays it (REAWAKENING; "Reawaken 1 / +<payout> Stardust", the payout read
//                                    BEFORE the reset: Second Helping's deferred half included), the field runs under it,
//                                    a held field holds it, and its end opens the spend step (never before).
// Pawpocalypse.UI.Ceremony.Waits     a screen still on the Menu stack when it ends (one that does not hold the field): the
//                                    spend step waits for it, never travels without it.
// Pawpocalypse.UI.JobPicker.Asks     leaving the spend step with jobs held opens the picker on the Modal stack (the field
//                                    held, no travel yet): the held jobs by id, "(last run)" selected, "No job" last; the
//                                    guard; a press locks the run's job, closes, then travels once.
// Pawpocalypse.UI.JobPicker.Union    under Union Rules the first answer asks again in place: the primary not offered,
//                                    "At 50% strength." blurbs, "No second job"; the second answer closes and travels.
// Pawpocalypse.UI.JobPicker.NoJobs   nothing held: no picker, the spend step's end travels at once.
//
// Negative tests: tools/sabotage_realm.py (`ceremony_*`, `jobpick_*`).

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UI/PawWidgets.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcHud.h"
#include "PcJobs.h"
#include "PcProgress.h"
#include "PcSave.h"
#include "PcStats.h"
#include "PcTree.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawJobPicker.h"
#include "UI/PawReawakenCeremony.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawText.h"
#include "UI/PawTokens.h"
#include "UI/PawUiConstants.h"
#include "UI/PawUiKit.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/PawTextTesting.h"

#include <algorithm>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FFlowWorld
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
			Hud->SetModalStack(Root->ModalStack);
			if (!Hud->GetAskReawakenButton()->IsTicked())
			{
				Hud->ToggleAskBeforeReawaken();
			}
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

		// A run worth Reawakening, then the HUD's two presses.
		bool Reawaken(FAutomationTestBase& T)
		{
			P().LifetimeCatnipEver = 1e12;
			P().LifetimeCatnipThisCycle = 1e12;
			P().Catnip = 1e12;
			Hud->RefreshNow();
			const int64_t Before = P().ReawakenCount;
			Hud->PressReawaken();
			Hud->PressReawaken();
			return T.TestEqual(TEXT("reawakened"), P().ReawakenCount, Before + 1);
		}

		// Frames of the HUD until the ceremony has run out (a guard against a ceremony that never ends).
		void PlayOutCeremony()
		{
			for (int32 I = 0; I < 400 && Hud->GetCeremony()->IsPlaying(); ++I)
			{
				Hud->Pump(1.0 / 60.0);
			}
			Hud->Pump(1.0 / 60.0);
		}

		// Through the ceremony and out of the spend step (its two-press leave).
		UPawSkillTreeScreen* LeaveSpendStep(FAutomationTestBase& T)
		{
			PlayOutCeremony();
			UPawSkillTreeScreen* Tree = Hud->GetSkillTree();
			if (!T.TestTrue(TEXT("the spend step is up"), Tree && Tree->IsActivated() && Tree->IsSpendMode()))
			{
				return nullptr;
			}
			Tree->PressClose();
			Tree->PressClose();
			T.TestFalse(TEXT("the spend step is left"), Tree->IsActivated());
			return Tree;
		}
	};

	FString S(const FText& T) { return T.ToString(); }

	std::vector<std::string> SomeJobs(const Pc::PcGameData& D, size_t N)
	{
		std::vector<std::string> Out;
		for (const Pc::PcJob& J : D.Jobs)
		{
			if (Out.size() < N)
			{
				Out.push_back(J.Id);
			}
		}
		std::sort(Out.begin(), Out.end());
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawCeremonyTweenTest, "Pawpocalypse.UI.Ceremony.Tween", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawCeremonyTweenTest::RunTest(const FString& Parameters)
{
	using A = UPawReawakenCeremony::FAlphas;
	const double In = PawUi::CeremonyFlashInSec, Pay = PawUi::CeremonyPayoutInSec, Hold = PawUi::CeremonyHoldSec, Out = PawUi::CeremonyFadeOutSec;
	TestEqual(TEXT("Godot's durations: 0.35 + 0.25 + 1.1 + 0.45"), UPawReawakenCeremony::Duration(), 2.15, 1e-12);
	const A Start = UPawReawakenCeremony::AlphasAt(0.0);
	TestTrue(TEXT("everything starts clear"), Start.Flash == 0.0f && Start.Headline == 0.0f && Start.Payout == 0.0f);
	// Sine in-out: half-way through a fade is exactly one half, a quarter of the way is (1 - cos(pi/4)) / 2.
	const A HalfIn = UPawReawakenCeremony::AlphasAt(In * 0.5);
	TestEqual(TEXT("the flash half in at half its time"), HalfIn.Flash, 0.5f, 1e-5f);
	TestEqual(TEXT("the headline with it"), HalfIn.Headline, HalfIn.Flash);
	TestEqual(TEXT("the payout waits"), HalfIn.Payout, 0.0f);
	const A Quarter = UPawReawakenCeremony::AlphasAt(In * 0.25);
	TestEqual(TEXT("eased, not linear"), Quarter.Flash, static_cast<float>(0.5 - 0.5 * FMath::Cos(PI * 0.25)), 1e-5f);
	const A PayHalf = UPawReawakenCeremony::AlphasAt(In + Pay * 0.5);
	TestTrue(TEXT("the flash is full while the payout fades in"), PayHalf.Flash == 1.0f && PayHalf.Headline == 1.0f);
	TestEqual(TEXT("the payout half in"), PayHalf.Payout, 0.5f, 1e-5f);
	const A Held = UPawReawakenCeremony::AlphasAt(In + Pay + Hold * 0.5);
	TestTrue(TEXT("the hold"), Held.Flash == 1.0f && Held.Headline == 1.0f && Held.Payout == 1.0f);
	const A Fading = UPawReawakenCeremony::AlphasAt(In + Pay + Hold + Out * 0.5);
	TestEqual(TEXT("all three fade out together"), Fading.Flash, 0.5f, 1e-5f);
	TestTrue(TEXT("together"), Fading.Headline == Fading.Flash && Fading.Payout == Fading.Flash);
	const A Late = UPawReawakenCeremony::AlphasAt(In + Pay + Hold + Out * 0.25);
	TestEqual(TEXT("the fade is eased too"), Late.Flash, static_cast<float>(0.5 + 0.5 * FMath::Cos(PI * 0.25)), 1e-5f);
	const A After = UPawReawakenCeremony::AlphasAt(UPawReawakenCeremony::Duration());
	TestTrue(TEXT("clear again at the end"), After.Flash == 0.0f && After.Headline == 0.0f && After.Payout == 0.0f);
	// Never above one, never below zero, and the flash never dips before the fade out.
	float Last = 0.0f;
	for (int32 I = 0; I <= 215; ++I)
	{
		const A At = UPawReawakenCeremony::AlphasAt(I * 0.01);
		if (At.Flash < 0.0f || At.Flash > 1.0f || At.Payout < 0.0f || At.Payout > 1.0f || (I * 0.01 < In + Pay + Hold && At.Flash < Last))
		{
			AddError(FString::Printf(TEXT("the tween leaves its track at %.2f s"), I * 0.01));
			break;
		}
		Last = At.Flash;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawCeremonyPlaysTest, "Pawpocalypse.UI.Ceremony.Plays", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawCeremonyPlaysTest::RunTest(const FString& Parameters)
{
	FFlowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	UPawReawakenCeremony* C = W.Hud->GetCeremony();
	if (!TestNotNull(TEXT("the HUD holds the ceremony"), C))
	{
		W.Close();
		return false;
	}
	TestEqual(TEXT("hidden until a Reawaken"), C->GetVisibility(), ESlateVisibility::Collapsed);
	// Second Helping defers half the grant: what landed is not what the Reawaken paid, and the ceremony states the PAYOUT.
	P.BranchLevels["second_helping"] = 1;
	P.LifetimeCatnipEver = 1e12;
	P.LifetimeCatnipThisCycle = 1e12;
	const double Payout = Pc::PendingStardust(W.D(), P);
	const double StardustBefore = P.Stardust;
	if (!W.Reawaken(*this))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("Second Helping held some back (the test can tell the two apart)"), FMath::FloorToDouble(P.Stardust - StardustBefore) != FMath::FloorToDouble(Payout));
	TestTrue(TEXT("the ceremony plays"), C->IsPlaying());
	TestEqual(TEXT("over the field, never taking a click"), C->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("the headline"), S(C->GetHeadlineText()), FString(TEXT("REAWAKENING")));
	FFormatNamedArguments Args;
	Args.Add(TEXT("n"), PawText::Count(1));
	Args.Add(TEXT("stardust"), PawText::Count(static_cast<int64>(FMath::FloorToDouble(Payout))));
	TestEqual(TEXT("the payout, read before the reset"), S(C->GetPayoutText()), S(PawText::Ui(TEXT("CEREMONY_payout"), Args)));
	TestTrue(TEXT("Reawaken 1, then the Stardust"), S(C->GetPayoutText()).StartsWith(TEXT("Reawaken 1\n+")));
	TestFalse(TEXT("no spend step yet"), W.Hud->IsMenuUp());
	TestFalse(TEXT("the field runs under the flash (Godot does not pause for it)"), W.Realm->IsPaused());

	// The HUD's frames drive the tween; what is drawn is the tween's value.
	for (int32 I = 0; I < 30; ++I)
	{
		W.Hud->Pump(1.0 / 60.0);
	}
	const UPawReawakenCeremony::FAlphas Want = UPawReawakenCeremony::AlphasAt(C->GetElapsed());
	const UPawReawakenCeremony::FAlphas Drawn = C->GetDrawnAlphas();
	TestEqual(TEXT("half a second in"), C->GetElapsed(), 0.5, 1e-9);
	TestTrue(TEXT("drawn as the tween says"), Drawn.Flash == Want.Flash && Drawn.Headline == Want.Headline && Drawn.Payout == Want.Payout);
	TestTrue(TEXT("the flash is up"), Drawn.Flash == 1.0f);

	// A held field holds the ceremony (Godot's tween on a pausable node).
	W.Realm->SetPaused(true);
	const double Held = C->GetElapsed();
	for (int32 I = 0; I < 300; ++I)
	{
		W.Hud->Pump(1.0 / 60.0);
	}
	TestEqual(TEXT("frozen while the field is held"), C->GetElapsed(), Held);
	TestTrue(TEXT("still playing"), C->IsPlaying());
	TestFalse(TEXT("and no spend step under a held field"), W.Hud->IsMenuUp());
	W.Realm->SetPaused(false);

	// Its end, and only its end, opens the spend step.
	while (C->IsPlaying() && C->GetElapsed() + 1.0 / 60.0 < UPawReawakenCeremony::Duration())
	{
		W.Hud->Pump(1.0 / 60.0);
		if (W.Hud->IsMenuUp())
		{
			AddError(TEXT("the spend step opened before the ceremony ended"));
			break;
		}
	}
	W.Hud->Pump(1.0 / 60.0);
	W.Hud->Pump(1.0 / 60.0);
	TestFalse(TEXT("the ceremony is over"), C->IsPlaying());
	TestEqual(TEXT("and hidden"), C->GetVisibility(), ESlateVisibility::Collapsed);
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	TestTrue(TEXT("the spend step follows it"), Tree && Tree->IsActivated() && Tree->IsSpendMode());
	TestFalse(TEXT("nothing left pending"), W.Hud->IsSpendStepPending());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawCeremonyWaitsTest, "Pawpocalypse.UI.Ceremony.Waits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawCeremonyWaitsTest::RunTest(const FString& Parameters)
{
	FFlowWorld W;
	if (!W.Open(*this) || !W.Reawaken(*this))
	{
		W.Close();
		return false;
	}
	// Every real screen on the Menu stack holds the field, and a held field holds the ceremony: a plain one does not.
	UCommonActivatableWidget* Other = W.Root->MenuStack->AddWidget<UCommonActivatableWidget>(UCommonActivatableWidget::StaticClass());
	if (!TestTrue(TEXT("a screen that does not hold the field"), Other && Other->IsActivated() && !W.Realm->IsPaused()))
	{
		W.Close();
		return false;
	}
	const int64 TravelsBefore = W.Realm->GetSim().Travels;
	W.PlayOutCeremony();
	TestFalse(TEXT("the ceremony ended"), W.Hud->GetCeremony()->IsPlaying());
	TestTrue(TEXT("the spend step waits"), W.Hud->IsSpendStepPending());
	TestFalse(TEXT("not opened under the other screen"), W.Hud->GetSkillTree() && W.Hud->GetSkillTree()->IsActivated());
	TestEqual(TEXT("and no travel without it"), W.Realm->GetSim().Travels, TravelsBefore);
	Other->DeactivateWidget();
	W.Hud->Pump(1.0 / 60.0);
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	TestTrue(TEXT("the spend step opens once the stack is clear"), Tree && Tree->IsActivated() && Tree->IsSpendMode());
	TestFalse(TEXT("nothing left pending"), W.Hud->IsSpendStepPending());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawJobPickerAsksTest, "Pawpocalypse.UI.JobPicker.Asks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawJobPickerAsksTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FFlowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	const std::vector<std::string> Jobs = SomeJobs(W.D(), 3);
	if (!TestEqual(TEXT("three jobs to hold"), static_cast<int32>(Jobs.size()), 3))
	{
		W.Close();
		return false;
	}
	for (const std::string& J : Jobs)
	{
		P.JobsHeld.insert(J);
	}
	P.CurrentJob = Jobs[1]; // last run's choice
	P.RunJob = Jobs[1];
	if (!W.Reawaken(*this))
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("the Reawaken asks again"), P.bRunJobChosen);
	const int64 TravelsBefore = W.Realm->GetSim().Travels;
	const int64 Where = Pc::ReawakenTravelIndex(W.D(), P); // read now: the travel spends it
	TestTrue(TEXT("a starting biome to go to"), Where >= 0);
	if (!W.LeaveSpendStep(*this))
	{
		W.Close();
		return false;
	}
	UPawJobPicker* Picker = W.Hud->GetJobPicker();
	if (!TestTrue(TEXT("leaving the spend step opens the job picker"), Picker && Picker->IsActivated()))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("on the Modal stack"), W.Root->ModalStack->GetWidgetList().Contains(Picker));
	TestTrue(TEXT("the field holds"), W.Realm->IsPaused());
	TestEqual(TEXT("no travel before the answer"), W.Realm->GetSim().Travels, TravelsBefore);
	TestEqual(TEXT("the title"), S(Picker->GetTitleText()), FString(TEXT("CHOOSE YOUR JOB")));
	TestEqual(TEXT("the note"), S(Picker->GetNoteText()), FString(TEXT("Locked for this run. You may change it at your next Reawaken.")));
	TestFalse(TEXT("the first ask"), Picker->IsAskingSecond());

	// What PawCore decides, drawn: the held jobs by id, "(last run)" on last run's, "No job" last.
	const Pc::PcJobPicker Want = Pc::JobPicker(W.D(), P, false);
	TestEqual(TEXT("one row per held job and the baseline"), Picker->GetRowCount(), 4);
	TestEqual(TEXT("as PawCore offers"), Picker->GetRowCount(), static_cast<int32>(Want.Rows.size()));
	TestEqual(TEXT("drawn: nothing pooled shows"), Picker->GetShownRowCount(), 4);
	for (int32 I = 0; I < 3; ++I)
	{
		TestEqual(TEXT("by id"), Picker->GetRowJobId(I), FString(UTF8_TO_TCHAR(Jobs[static_cast<size_t>(I)].c_str())));
		const FText Name = PawText::Data(TEXT("JOB"), Jobs[static_cast<size_t>(I)]);
		FFormatNamedArguments Args;
		Args.Add(TEXT("job"), Name);
		TestEqual(TEXT("the name, last run's marked"), S(Picker->GetRowLabel(I)), I == 1 ? S(PawText::Ui(TEXT("JOBPICK_last_run"), Args)) : S(Name));
		TestEqual(TEXT("the job's own blurb"), S(Picker->GetRowBlurb(I)), S(PawText::DataField(TEXT("JOB"), Jobs[static_cast<size_t>(I)], TEXT("desc"))));
	}
	TestTrue(TEXT("(last run) in English"), S(Picker->GetRowLabel(1)).EndsWith(TEXT("   (last run)")));
	// The game theme (F21, designer 2026-11-03): the theme's buttons, ink text, on the paper WindowPanel.
	for (int32 I = 0; I < Picker->GetRowCount(); ++I)
	{
		TestTrue(TEXT("the theme's button"), Picker->GetRowButton(I) && Picker->GetRowButton(I)->HasThemeLook());
	}
	TestTrue(TEXT("the note in ink"), Picker->GetNote()->GetColorAndOpacity().GetSpecifiedColor().Equals(PawTokens::UI_TEXT));
	{
		// The theme's WindowPanel: ui_window.svg's nine-patch (phase 6 slice 5a; Pawpocalypse.UI.Chrome checks every brush).
		const FSlateBrush& Plate = Picker->GetPanel()->Background;
		TestTrue(TEXT("on the window plate"), Plate.DrawAs == ESlateBrushDrawType::Box && Plate.GetResourceObject() != nullptr
			&& Plate.GetResourceObject() == PawUi::Chrome(PawUi::EPawChrome::Window).GetResourceObject());
	}
	TestEqual(TEXT("the baseline last"), Picker->GetRowJobId(3), FString());
	TestEqual(TEXT("No job"), S(Picker->GetRowLabel(3)), FString(TEXT("No job")));
	TestEqual(TEXT("its blurb"), S(Picker->GetRowBlurb(3)), FString(TEXT("A bare neck. Every trade in the list is measured against this.")));
	TestTrue(TEXT("the selection opens on last run's job"), Picker->GetDesiredFocusTarget() == Picker->GetRowButton(1));
	TestEqual(TEXT("one column, every row wired"), Picker->GetWired().Num(), 4);

	// The guard: nothing for 0.4 s; then a key press that began before the list opened still does nothing.
	Picker->NoteKeyDown(false); // a press that began as the list opened
	Picker->ClickRow(0);
	TestTrue(TEXT("a press at once does nothing"), Picker->IsActivated() && P.RunJob != Jobs[0]);
	Picker->AdvanceClock(PawUi::ModalGuardSec + 0.05);
	Picker->ClickRow(0);
	TestTrue(TEXT("nor the key that was already down"), Picker->IsActivated() && !P.bRunJobChosen);
	Picker->NoteKeyDown(false); // a fresh press
	Picker->ClickRow(0);
	TestFalse(TEXT("a fresh press answers"), Picker->IsActivated());
	TestEqual(TEXT("the run's job"), P.RunJob, Jobs[0]);
	TestEqual(TEXT("and the preference"), P.CurrentJob, Jobs[0]);
	TestTrue(TEXT("asked"), P.bRunJobChosen);
	TestEqual(TEXT("then the travel, once"), W.Realm->GetSim().Travels, TravelsBefore + 1);
	TestEqual(TEXT("to the starting biome"), static_cast<int64>(W.Realm->GetSim().Field.TierIndex), Where);
	TestFalse(TEXT("the field runs"), W.Realm->IsPaused());
	TestEqual(TEXT("nothing left on the Modal stack"), W.Root->ModalStack->GetNumWidgets(), 0);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawJobPickerUnionTest, "Pawpocalypse.UI.JobPicker.Union", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawJobPickerUnionTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FFlowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	const std::vector<std::string> Jobs = SomeJobs(W.D(), 2);
	for (const std::string& J : Jobs)
	{
		P.JobsHeld.insert(J);
	}
	P.BranchLevels["union_rules"] = 1;
	if (!TestTrue(TEXT("Union Rules owned"), Pc::HasUnionRules(W.D(), P)) || !W.Reawaken(*this))
	{
		W.Close();
		return false;
	}
	const int64 TravelsBefore = W.Realm->GetSim().Travels;
	if (!W.LeaveSpendStep(*this))
	{
		W.Close();
		return false;
	}
	UPawJobPicker* Picker = W.Hud->GetJobPicker();
	if (!TestTrue(TEXT("the picker is up"), Picker && Picker->IsActivated()))
	{
		W.Close();
		return false;
	}
	Picker->AdvanceClock(1.0); // a while on the first ask: the second must start its own guard
	TestTrue(TEXT("a first answer that leaves a job held asks again"), Picker->Pick(0));
	TestTrue(TEXT("in place"), Picker->IsActivated() && Picker->IsAskingSecond());
	TestEqual(TEXT("the primary is locked"), P.RunJob, Jobs[0]);
	TestFalse(TEXT("the run is not asked yet"), P.bRunJobChosen);
	TestEqual(TEXT("no travel between the two"), W.Realm->GetSim().Travels, TravelsBefore);
	TestEqual(TEXT("the primary is not offered twice"), Picker->GetRowCount(), 2);
	TestEqual(TEXT("the other job"), Picker->GetRowJobId(0), FString(UTF8_TO_TCHAR(Jobs[1].c_str())));
	TestEqual(TEXT("pooled rows beyond the list are hidden"), Picker->GetShownRowCount(), 2);
	const int64 Share = Pc::JobPicker(W.D(), P, true).SharePercent;
	TestEqual(TEXT("the shipped share"), Share, static_cast<int64>(50));
	TestTrue(TEXT("At 50% strength."), S(Picker->GetRowBlurb(0)).StartsWith(TEXT("At 50% strength. ")));
	TestTrue(TEXT("then the job's blurb"), S(Picker->GetRowBlurb(0)).EndsWith(S(PawText::DataField(TEXT("JOB"), Jobs[1], TEXT("desc")))));
	TestEqual(TEXT("No second job"), S(Picker->GetRowLabel(1)), FString(TEXT("No second job")));
	TestEqual(TEXT("its blurb"), S(Picker->GetRowBlurb(1)), FString(TEXT("Wear one and only one. Union Rules is an option, not an obligation.")));
	TestEqual(TEXT("rewired"), Picker->GetWired().Num(), 2);
	// The second ask restarts the guard.
	Picker->NoteMouseDown();
	Picker->ClickRow(0);
	TestTrue(TEXT("the guard runs again for the second ask"), Picker->IsActivated() && P.RunJobSecond.empty());
	Picker->AdvanceClock(PawUi::ModalGuardSec + 0.05);
	Picker->ClickRow(0);
	TestFalse(TEXT("the second answer closes"), Picker->IsActivated());
	TestEqual(TEXT("the second job"), P.RunJobSecond, Jobs[1]);
	TestTrue(TEXT("asked"), P.bRunJobChosen);
	TestEqual(TEXT("then the travel, once"), W.Realm->GetSim().Travels, TravelsBefore + 1);
	TestFalse(TEXT("the field runs"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawJobPickerNoJobsTest, "Pawpocalypse.UI.JobPicker.NoJobs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawJobPickerNoJobsTest::RunTest(const FString& Parameters)
{
	FFlowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	TestTrue(TEXT("a new game holds no job"), P.JobsHeld.empty());
	if (!W.Reawaken(*this))
	{
		W.Close();
		return false;
	}
	const int64 TravelsBefore = W.Realm->GetSim().Travels;
	if (!W.LeaveSpendStep(*this))
	{
		W.Close();
		return false;
	}
	TestFalse(TEXT("nothing held is not a choice: no picker"), W.Hud->GetJobPicker() && W.Hud->GetJobPicker()->IsActivated());
	TestEqual(TEXT("nothing on the Modal stack"), W.Root->ModalStack->GetNumWidgets(), 0);
	TestEqual(TEXT("the travel at once"), W.Realm->GetSim().Travels, TravelsBefore + 1);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawReawakenResumesTest, "Pawpocalypse.UI.Ceremony.Resumes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// F29 (designer 2026-11-04, Godot first): a Reawaken sets the flag and the save carries it; a load with it set opens the spend
// step (no ceremony), whose end travels and clears it; without it nothing opens.
bool FPawReawakenResumesTest::RunTest(const FString& Parameters)
{
	FFlowWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	Pc::PcProgress& P = W.P();
	W.Hud->Pump(1.0 / 60.0);
	TestFalse(TEXT("no Reawaken in progress: nothing opens"), W.Hud->GetSkillTree() && W.Hud->GetSkillTree()->IsActivated());
	if (!W.Reawaken(*this))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("a Reawaken is in progress"), P.bReawakenInProgress);
	TestTrue(TEXT("and the save carries it"), Pc::CaptureSave(W.D(), P, Pc::PcSlotExtras(), 1.0).bReawakenInProgress);
	W.LeaveSpendStep(*this);
	TestNull(TEXT("no job held: no picker"), W.Hud->GetJobPicker());
	TestFalse(TEXT("arrived: the Reawaken is over"), P.bReawakenInProgress);

	// A save made mid-Reawaken, loaded: the flag alone opens the spend step, without the ceremony.
	P.bReawakenInProgress = true;
	W.Hud->Pump(1.0 / 60.0);
	TestFalse(TEXT("no ceremony replayed"), W.Hud->GetCeremony()->IsPlaying());
	UPawSkillTreeScreen* Tree = W.Hud->GetSkillTree();
	if (!TestTrue(TEXT("the spend step resumes"), Tree && Tree->IsActivated() && Tree->IsSpendMode()))
	{
		W.Close();
		return false;
	}
	W.Hud->Pump(1.0 / 60.0);
	TestEqual(TEXT("opened once"), W.Root->MenuStack->GetNumWidgets(), 1);
	const int64 TravelsBefore = W.Realm->GetSim().Travels;
	Tree->PressClose();
	Tree->PressClose();
	TestFalse(TEXT("the spend step is left"), Tree->IsActivated());
	TestFalse(TEXT("then the run arrives and the flag clears"), P.bReawakenInProgress);
	TestTrue(TEXT("the travel ran (or the run was already there)"), W.Realm->GetSim().Travels >= TravelsBefore);
	W.Hud->Pump(1.0 / 60.0);
	TestFalse(TEXT("and nothing reopens"), W.Hud->GetSkillTree() && W.Hud->GetSkillTree()->IsActivated());
	W.Close();
	return true;
}

#endif
