#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// P4.3 (playtest 2026-11-26, notes 2 and 3): Boba in the menus. The clock's and the paths' rules are PawCore's (L1 "Boba:");
// these check the game's side: her photos, the menus' hooks, never over the tree, the effects switch, the title's card.

#include "Audio/PawAudioSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/BorderSlot.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawBoba.h"
#include "UI/PawMainMenu.h"
#include "UI/PawWidgets.h"

namespace
{
	struct FBobaWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawBobaSubsystem* Boba = nullptr;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Boba = GI->GetSubsystem<UPawBobaSubsystem>();
			if (!T.TestNotNull(TEXT("the Boba subsystem"), Boba))
			{
				return false;
			}
			Boba->SetSeed(42);
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
		void Effects(bool bOn)
		{
			if (UPawSettingsSubsystem* S = GI->GetSubsystem<UPawSettingsSubsystem>())
			{
				S->SetEffectsEnabled(bOn); // automated runs keep no settings file (PawStorageForThisRun)
			}
		}
		// Ticks until a cameo is up (or Limit seconds pass); the seconds it took, or -1.
		double Until(double Limit)
		{
			for (double T = 0.0; T < Limit; T += 0.1)
			{
				Boba->TickOnce(0.1f);
				if (Boba->IsShowing())
				{
					return T + 0.1;
				}
			}
			return -1.0;
		}
		int32 Plays() const
		{
			int32 N = 0;
			if (const UPawAudioSubsystem* A = GI->GetSubsystem<UPawAudioSubsystem>())
			{
				for (const UPawAudioSubsystem::FPlayed& P : A->Played())
				{
					N += P.Event == TEXT("boba_cameo") ? 1 : 0;
				}
			}
			return N;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawBobaPhotosTest, "Pawpocalypse.UI.Boba.Photos", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawBobaPhotosTest::RunTest(const FString& Parameters)
{
	FBobaWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	TestEqual(TEXT("her 14 photos (DA_Boba)"), W.Boba->PhotoCount(), 14);
	const UTexture2D* First = W.Boba->GetPhoto(0);
	TestTrue(TEXT("sorted: the first is boba_big_eyes"), First && First->GetName() == TEXT("T_boba_big_eyes"));
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawBobaMenusTest, "Pawpocalypse.UI.Boba.Menus", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawBobaMenusTest::RunTest(const FString& Parameters)
{
	FBobaWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Effects(true);
	TestTrue(TEXT("no menu, no Boba"), W.Until(120.0) < 0.0);
	W.Boba->EnterMenu(TEXT("main_menu"));
	const double First = W.Until(20.0);
	TestTrue(TEXT("the title: she comes 6-14 s in"), First >= Pc::BobaFirstMin - 0.11 && First <= Pc::BobaFirstMax + 0.11);
	UPawBobaLayer* Layer = W.Boba->GetLayer();
	if (TestNotNull(TEXT("her layer"), Layer))
	{
		TestTrue(TEXT("drawn"), Layer->IsPhotoShown());
		TestTrue(TEXT("one of her photos"), Layer->GetPhoto() == W.Boba->GetPhoto(W.Boba->GetState().Cameo.Photo));
	}
	TestEqual(TEXT("boba_cameo played"), W.Plays(), 1);
	// The title closes: she finishes, then never again.
	W.Boba->LeaveMenu(TEXT("main_menu"));
	TestTrue(TEXT("a cameo in flight finishes"), W.Boba->IsShowing());
	for (int32 I = 0; I < 80; ++I)
	{
		W.Boba->TickOnce(0.1f);
	}
	TestFalse(TEXT("finished"), W.Boba->IsShowing());
	TestTrue(TEXT("hidden"), Layer && !Layer->IsPhotoShown());
	TestTrue(TEXT("and none after"), W.Until(200.0) < 0.0);

	// The effects off: the clock runs, she does not come (and a forced one neither).
	W.Effects(false);
	W.Boba->EnterMenu(TEXT("pause"));
	TestTrue(TEXT("effects off: no Boba"), W.Until(200.0) < 0.0);
	TestFalse(TEXT("effects off: not even forced"), W.Boba->Force());
	W.Effects(true);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawBobaTreeTest, "Pawpocalypse.UI.Boba.NeverOverTree", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawBobaTreeTest::RunTest(const FString& Parameters)
{
	FBobaWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Effects(true);
	W.Boba->EnterMenu(TEXT("pause"));
	TestTrue(TEXT("forced"), W.Boba->ForceKind(Pc::PcBobaKind::Flyby));
	W.Boba->TickOnce(0.5f);
	TestTrue(TEXT("up"), W.Boba->GetLayer() && W.Boba->GetLayer()->IsPhotoShown());
	// The skill tree opens: she is cut at once, and stays away.
	W.Boba->Suppress(TEXT("tree"), true);
	TestFalse(TEXT("cut"), W.Boba->IsShowing());
	TestFalse(TEXT("cut on the layer at once"), W.Boba->GetLayer()->IsPhotoShown());
	TestFalse(TEXT("not forced over the tree"), W.Boba->Force());
	TestTrue(TEXT("not on the clock over the tree"), W.Until(200.0) < 0.0);
	W.Boba->Suppress(TEXT("tree"), false);
	TestTrue(TEXT("back once it closes"), W.Boba->Force());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawBobaVictoryTest, "Pawpocalypse.UI.Boba.Victory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawBobaVictoryTest::RunTest(const FString& Parameters)
{
	FBobaWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Effects(true);
	W.Boba->EnterMenu(TEXT("victory"));
	W.Boba->ForceLater(Pc::PcBobaKind::Nyan, 1.5, TEXT("victory"));
	W.Boba->TickOnce(1.4f);
	TestFalse(TEXT("not before 1.5 s"), W.Boba->IsShowing());
	W.Boba->TickOnce(0.2f);
	TestTrue(TEXT("the rainbow one at 1.5 s"), W.Boba->IsShowing() && W.Boba->GetState().Cameo.Kind == Pc::PcBobaKind::Nyan);
	// Left before it was due: nothing.
	FBobaWorld V;
	if (V.Open(*this))
	{
		V.Effects(true);
		V.Boba->EnterMenu(TEXT("victory"));
		V.Boba->ForceLater(Pc::PcBobaKind::Nyan, 1.5, TEXT("victory"));
		V.Boba->LeaveMenu(TEXT("victory"));
		V.Boba->TickOnce(2.0f);
		TestFalse(TEXT("the victory closed first: no cameo"), V.Boba->IsShowing());
	}
	V.Close();
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawBobaTitleTest, "Pawpocalypse.UI.Boba.Title", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawBobaTitleTest::RunTest(const FString& Parameters)
{
	FBobaWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.Effects(true);
	UPawMainMenu* Menu = CreateWidget<UPawMainMenu>(W.GI, PawUi::Resolve<UPawMainMenu>());
	if (!TestNotNull(TEXT("the title"), Menu))
	{
		W.Close();
		return false;
	}
	Menu->ShowBobaPhoto();
	const UImage* Photo = Menu->GetBobaPhoto();
	TestTrue(TEXT("her photo in the card's well: the first one"), Photo && Photo->GetBrush().GetResourceObject() == W.Boba->GetPhoto(0));
	// Centred by its UBorderSlot, at its own proportions (a Fill slot stretched her to the square well: KB U/09).
	const UBorderSlot* PhotoSlot = Photo ? Cast<UBorderSlot>(Photo->Slot) : nullptr;
	TestTrue(TEXT("the photo centred in its well, not stretched"), PhotoSlot && PhotoSlot->GetHorizontalAlignment() == HAlign_Center
		&& PhotoSlot->GetVerticalAlignment() == VAlign_Center);
	TestTrue(TEXT("at the photo's proportions"), Photo && FMath::IsNearlyEqual(Photo->GetBrush().GetImageSize().X / Photo->GetBrush().GetImageSize().Y,
		UPawBobaSubsystem::PhotoSize(W.Boba->GetPhoto(0)).X / UPawBobaSubsystem::PhotoSize(W.Boba->GetPhoto(0)).Y, 0.01));
	const UWidget* Card = Menu->GetBobaCard();
	TestTrue(TEXT("the card is straight (playtest note 3)"), Card && FMath::IsNearlyZero(Card->GetRenderTransformAngle()));
	TestTrue(TEXT("a click brings her"), Menu->PressBoba());
	TestTrue(TEXT("she is up"), W.Boba->IsShowing());
	TestEqual(TEXT("boba_cameo played"), W.Plays(), 1);
	TestFalse(TEXT("one at a time"), Menu->PressBoba());
	W.Close();
	return true;
}

#endif
