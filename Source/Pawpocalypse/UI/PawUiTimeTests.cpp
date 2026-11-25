// Pawpocalypse.UI.StepDelta (2026-11-25): the time a widget steps its own motion by (UI/PawUiTime.h). In the game as players
// run it, Slate's delta untouched; under the engine's fixed step (a store-clip recording), that step, once per engine frame,
// whatever Slate hands over and however often the window is painted; and a ceremony stepped that way lasts its own length in
// frames.
//
// Negative tests: tools/sabotage_realm.py (`stepdelta_*`).

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "UI/PawUiTime.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawStepDeltaTest, "Pawpocalypse.UI.StepDelta", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawStepDeltaTest::RunTest(const FString& Parameters)
{
	const bool bWasFixed = FApp::UseFixedTimeStep();
	const double WasStep = FApp::GetFixedDeltaTime();
	const UObject* A = GetTransientPackage();
	const UObject* B = UPackage::StaticClass();

	// The game as players run it: Slate's delta, every call.
	FApp::SetUseFixedTimeStep(false);
	TestEqual(TEXT("no fixed step: Slate's delta"), PawUi::StepDelta(A, 0.25f), 0.25f);
	TestEqual(TEXT("no fixed step: Slate's delta again in the same frame"), PawUi::StepDelta(A, 0.25f), 0.25f);

	// A recording: the fixed step, once per engine frame and widget.
	FApp::SetUseFixedTimeStep(true);
	FApp::SetFixedDeltaTime(1.0 / 60.0);
	GFrameCounter += 7; // a frame no widget has stepped in yet
	TestEqual(TEXT("fixed step: the step, not Slate's real delta"), PawUi::StepDelta(A, 0.11f), static_cast<float>(1.0 / 60.0));
	TestEqual(TEXT("fixed step: a second paint in the frame steps nothing"), PawUi::StepDelta(A, 0.11f), 0.0f);
	TestEqual(TEXT("fixed step: another widget steps in the same frame"), PawUi::StepDelta(B, 0.11f), static_cast<float>(1.0 / 60.0));
	double Total = 0.0;
	for (int32 Frame = 0; Frame < 225; ++Frame) // the Unicorn ceremony's 3.75 s at 60 frames a second
	{
		++GFrameCounter;
		Total += PawUi::StepDelta(A, 0.11f);
		Total += PawUi::StepDelta(A, 0.11f); // the screenshot's second paint
	}
	TestTrue(TEXT("fixed step: 225 frames painted twice are 3.75 s"), FMath::IsNearlyEqual(Total, 3.75, 1e-3));

	FApp::SetUseFixedTimeStep(bWasFixed);
	FApp::SetFixedDeltaTime(WasStep);
	return true;
}

#endif
