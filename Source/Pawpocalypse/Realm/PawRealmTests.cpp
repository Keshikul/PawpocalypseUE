// The realm's engine half (docs/11 phase 2 slice 7; docs/09 L3). The rules are PawCore's and proved there; these prove the
// clock, the glue and the drawing the game puts around them.
//
// Pawpocalypse.Realm.FrameRateIndependence  the same steps give the same field at 30, 50, 60, 144 and 240 fps and at a
//                           jittered rate, bit for bit; a variable-step loop (the thing the fixed step prevents) and one
//                           extra step are each seen to differ.
// Pawpocalypse.Realm.SpiralGuard  a long frame runs at most 8 steps and drops the rest; a frame owing exactly 8 keeps its
//                           fraction of a step.
// Pawpocalypse.Realm.Interpolation  the presentation sits between the last two steps by the accumulator's fraction; a slot
//                           re-used by a new mouse is drawn where the new mouse is.
// Pawpocalypse.Realm.Travel  every biome, from a full field, in under 16 ms (docs/03 §4), nothing kept from the last one.
// Pawpocalypse.Realm.TravelRequested  the field's TravelRequested travels within the frame; without one nothing moves.
// Pawpocalypse.Realm.ReawakenTravel  after a Reawaken the boss is dismissed, and the realm itself never travels, with
//                           Territory or without: the HUD does, after the spend step (Territory wins, 2026-11-01).
// Pawpocalypse.Realm.View   a real world, the view and its renderer at 320 mice: what is drawn is what is live, the frame
//                           (advance + draw) inside the game thread's budget, travel with the redraw under 16 ms.
//
// UnrealEditor-Cmd.exe Pawpocalypse.uproject -ExecCmds="Automation RunTests Pawpocalypse.Realm;Quit"
//     -TestExit="Automation Test Queue Empty" -nullrhi -unattended -nop4 -SCCProvider=None -stdout   (editor closed)

#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Data/PawDataAdapter.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInterface.h"
#include "Realm/PawFieldLook.h"
#include "Render/PawSpriteAtlas.h"
#include "Audio/PawAudioSubsystem.h"
#include "Engine/Texture2D.h"
#include "PcBackdrop.h"
#include "Math/RandomStream.h"
#include "Misc/AutomationTest.h"
#include "PcCheats.h"
#include "PcData.h"
#include "PcEconomy.h"
#include "PcField.h"
#include "PcProgress.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawRealmSubsystem.h"
#include "Realm/PawRealmView.h"
#include "Render/PawSpriteRenderer.h"
#include "Save/PawSettingsSubsystem.h"

#include <cstring>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr double TravelBudgetMs = 16.0;   // docs/03 §4
	constexpr double FrameBudgetMs = 4.0;     // docs/04 §9: the game thread at peak on the dev PC

	const Pc::PcGameData* GameData(FAutomationTestBase& Test)
	{
		static Pc::PcGameData Data;
		static bool bTried = false, bOk = false;
		if (!bTried)
		{
			bTried = true;
			TArray<FString> Errors;
			bOk = FPawDataAdapter::BuildGameData(Data, Errors);
			for (const FString& E : Errors)
			{
				Test.AddError(E);
			}
		}
		return bOk ? &Data : nullptr;
	}

	// A busy first biome: kittens out, the paw strong enough to kill, so the steps change many things.
	Pc::PcProgress BusyProgress(const Pc::PcGameData& D)
	{
		Pc::PcProgress P;
		Pc::NewGame(D, P);
		P.UpgradeLevels["kitten_count"] = 6;
		P.UpgradeLevels["paw_damage"] = 30;
		P.InfestationBonus = 4.0;
		return P;
	}

	// FNV-1a over everything a step can change.
	struct FHash
	{
		uint64 H = 1469598103934665603ull;
		void Bytes(const void* Data, size_t N)
		{
			const uint8* B = static_cast<const uint8*>(Data);
			for (size_t I = 0; I < N; ++I)
			{
				H = (H ^ B[I]) * 1099511628211ull;
			}
		}
		template <typename T> void Value(const T& V) { Bytes(&V, sizeof(V)); }
		template <typename T> void Vec(const std::vector<T>& V) { if (!V.empty()) { Bytes(V.data(), V.size() * sizeof(T)); } }
	};

	uint64 HashOf(const Pc::PcField& F, const Pc::PcProgress& P)
	{
		FHash H;
		H.Value(F.StepCount);
		H.Value(F.TierIndex);
		H.Vec(F.Live);
		H.Vec(F.Serial);
		H.Vec(F.X);
		H.Vec(F.Y);
		H.Vec(F.Hp);
		H.Vec(F.KX);
		H.Vec(F.KY);
		H.Value(F.SpawnTimeLeft);
		H.Value(P.Catnip);
		H.Value(P.LifetimeKills);
		H.Value(P.TotalPlaySeconds);
		return H.H;
	}

	// Runs frames of the given deltas (cycled) until exactly Steps steps have run; the last frame is trimmed so no pattern
	// overshoots. Returns the field's hash there.
	uint64 RunTo(const Pc::PcGameData& D, int64 Steps, TFunctionRef<double(int32)> Delta)
	{
		Pc::PcProgress P = BusyProgress(D);
		FPawRealmSim Sim;
		Sim.BaseSeed = 42;
		Sim.Start(D, P);
		Sim.Input.CatX = Sim.Field.CatX + 90.0; // still: every pattern feeds every step the same input
		Sim.Input.CatY = Sim.Field.CatY - 40.0;
		int32 Frame = 0;
		while (static_cast<int64>(Sim.Field.StepCount) < Steps)
		{
			double Dt = Delta(Frame++);
			const double Remaining = static_cast<double>(Steps - static_cast<int64>(Sim.Field.StepCount)) * FPawRealmSim::Step - Sim.PendingTime();
			if (Dt > Remaining)
			{
				Dt = Remaining + FPawRealmSim::Step * 0.5;
			}
			Sim.Advance(D, P, Dt);
		}
		return Sim.Field.StepCount == static_cast<uint64>(Steps) ? HashOf(Sim.Field, P) : 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRealmFrameRateTest, "Pawpocalypse.Realm.FrameRateIndependence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRealmFrameRateTest::RunTest(const FString& Parameters)
{
	const Pc::PcGameData* D = GameData(*this);
	if (!D)
	{
		return false;
	}
	const int64 Steps = 60 * 40; // 40 game seconds
	const uint64 At60 = RunTo(*D, Steps, [](int32) { return 1.0 / 60.0; });
	TestNotEqual(TEXT("the run reached its step"), At60, uint64(0));
	struct FPattern { const TCHAR* Name; TFunction<double(int32)> Delta; };
	FRandomStream Jitter(7);
	TArray<double> Jittered;
	for (int32 I = 0; I < 4000; ++I)
	{
		Jittered.Add(Jitter.FRandRange(1.0 / 240.0, 1.0 / 20.0));
	}
	const FPattern Patterns[] = {
		{ TEXT("30 fps"), [](int32) { return 1.0 / 30.0; } },
		{ TEXT("50 fps"), [](int32) { return 1.0 / 50.0; } },
		{ TEXT("144 fps"), [](int32) { return 1.0 / 144.0; } },
		{ TEXT("240 fps"), [](int32) { return 1.0 / 240.0; } },
		{ TEXT("jittered 20-240 fps"), [&Jittered](int32 F) { return Jittered[F % Jittered.Num()]; } },
	};
	for (const FPattern& Pt : Patterns)
	{
		TestEqual(FString::Printf(TEXT("%s gives the 60 fps field bit for bit"), Pt.Name), RunTo(*D, Steps, Pt.Delta), At60);
	}

	// The comparison is not vacuous: one more step differs, and so does the loop the fixed step replaced (each frame's
	// real delta handed to the field as its step).
	TestNotEqual(TEXT("one extra step is seen"), RunTo(*D, Steps + 1, [](int32) { return 1.0 / 60.0; }), At60);
	{
		Pc::PcProgress P = BusyProgress(*D);
		FPawRealmSim Sim;
		Sim.BaseSeed = 42;
		Sim.Start(*D, P);
		Sim.Input.CatX = Sim.Field.CatX + 90.0;
		Sim.Input.CatY = Sim.Field.CatY - 40.0;
		const double Dt = 1.0 / 144.0;
		for (int64 F = 0; F < Steps * 144 / 60; ++F)
		{
			Pc::StepField(Sim.Field, *D, P, Sim.Input, Dt);
			Pc::TickPlayClock(P, Dt);
			Sim.Field.Events.clear();
		}
		Sim.Field.StepCount = static_cast<uint64>(Steps); // same count, so only the stepping differs
		TestNotEqual(TEXT("a variable-step loop at 144 fps is seen to differ"), HashOf(Sim.Field, P), At60);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRealmSpiralTest, "Pawpocalypse.Realm.SpiralGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRealmSpiralTest::RunTest(const FString& Parameters)
{
	const Pc::PcGameData* D = GameData(*this);
	if (!D)
	{
		return false;
	}
	Pc::PcProgress P = BusyProgress(*D);
	FPawRealmSim Sim;
	Sim.Start(*D, P);
	TestEqual(TEXT("a one-second hitch runs the cap"), Sim.Advance(*D, P, 1.0), FPawRealmSim::MaxStepsPerFrame);
	TestEqual(TEXT("and drops the rest"), Sim.StepsDropped, int64(60 - FPawRealmSim::MaxStepsPerFrame));
	TestTrue(TEXT("nothing is owed after it"), Sim.PendingTime() < FPawRealmSim::Step);
	TestEqual(TEXT("the next empty frame runs nothing"), Sim.Advance(*D, P, 0.0), 0);

	FPawRealmSim Exact;
	Exact.Start(*D, P);
	const double Clock = P.TotalPlaySeconds;
	TestEqual(TEXT("a frame owing 8.5 steps runs 8"), Exact.Advance(*D, P, 8.5 * FPawRealmSim::Step), 8);
	TestTrue(TEXT("the play clock counts the steps' time"), FMath::IsNearlyEqual(P.TotalPlaySeconds - Clock, 8.0 * FPawRealmSim::Step, 1e-9));
	TestEqual(TEXT("and drops none"), Exact.StepsDropped, int64(0));
	TestTrue(TEXT("its half step is kept"), FMath::IsNearlyEqual(Exact.PendingTime(), 0.5 * FPawRealmSim::Step, 1e-9));

	Exact.bPaused = true;
	TestEqual(TEXT("paused, a frame runs nothing"), Exact.Advance(*D, P, 1.0), 0);
	TestTrue(TEXT("and owes nothing for it"), FMath::IsNearlyEqual(Exact.PendingTime(), 0.5 * FPawRealmSim::Step, 1e-9));
	TestTrue(TEXT("nor counts it on the play clock"), FMath::IsNearlyEqual(P.TotalPlaySeconds - Clock, 8.0 * FPawRealmSim::Step, 1e-9));
	Exact.bPaused = false;
	Exact.GameSpeed = 5.0;
	TestEqual(TEXT("at 5x a 60 fps frame runs five steps (with the half step: still five)"), Exact.Advance(*D, P, 1.0 / 60.0), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRealmInterpolationTest, "Pawpocalypse.Realm.Interpolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRealmInterpolationTest::RunTest(const FString& Parameters)
{
	const Pc::PcGameData* D = GameData(*this);
	if (!D)
	{
		return false;
	}
	Pc::PcProgress P = BusyProgress(*D);
	FPawRealmSim Sim;
	Sim.Start(*D, P);
	const int32 Slot = Pc::SpawnEnemyAt(Sim.Field, *D, P, D->Enemies.front().Id, 400.0, 400.0, 1.0, 0.0);
	if (!TestTrue(TEXT("a mouse placed"), Slot >= 0))
	{
		return false;
	}
	Sim.Input.CatX = 1500.0; // far away: nothing touches it
	Sim.Input.CatY = 900.0;
	Sim.Advance(*D, P, FPawRealmSim::Step);
	const double X0 = Sim.Field.X[Slot], Y0 = Sim.Field.Y[Slot];
	Sim.Advance(*D, P, 1.5 * FPawRealmSim::Step); // one step, half of the next owed
	const double X1 = Sim.Field.X[Slot], Y1 = Sim.Field.Y[Slot];
	TestTrue(TEXT("the mouse moved in the step"), X1 != X0 || Y1 != Y0);
	TestTrue(TEXT("alpha is the half step owed"), FMath::IsNearlyEqual(Sim.Alpha(), 0.5, 1e-9));
	double X = 0.0, Y = 0.0;
	Sim.MousePos(Slot, X, Y);
	TestTrue(TEXT("drawn halfway between the last two steps"), FMath::IsNearlyEqual(X, (X0 + X1) * 0.5, 1e-9) && FMath::IsNearlyEqual(Y, (Y0 + Y1) * 0.5, 1e-9));

	// The slot taken by a new mouse since the snapshot: drawn where it is, never slid across from the old one.
	Sim.Field.Serial[Slot] += 1000;
	Sim.Field.X[Slot] = 100.0;
	Sim.MousePos(Slot, X, Y);
	TestEqual(TEXT("a re-used slot is drawn where its new mouse is"), X, 100.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRealmTravelTest, "Pawpocalypse.Realm.Travel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRealmTravelTest::RunTest(const FString& Parameters)
{
	const Pc::PcGameData* D = GameData(*this);
	if (!D)
	{
		return false;
	}
	Pc::PcProgress P = BusyProgress(*D);
	P.InfestationBonus = 1000.0; // the flood: a full field to leave
	FPawRealmSim Sim;
	Sim.Start(*D, P);
	const int32 N = static_cast<int32>(D->Tiers.size());
	double Worst = 0.0;
	for (int32 Round = 0; Round < 2; ++Round)
	{
		for (int32 I = 0; I < N; ++I)
		{
			const int32 Index = Round == 0 ? I : N - 1 - I;
			for (int32 F = 0; F < 90; ++F)
			{
				Sim.Advance(*D, P, FPawRealmSim::Step);
			}
			const int64 Before = Sim.Field.LiveCount();
			TestTrue(TEXT("travel accepted"), Sim.Travel(*D, P, Index));
			Worst = FMath::Max(Worst, Sim.LastTravelMs);
			TestTrue(FString::Printf(TEXT("biome %d: travel %.3f ms < %.0f ms (left %lld mice)"), Index + 1, Sim.LastTravelMs,
				TravelBudgetMs, Before), Sim.LastTravelMs < TravelBudgetMs);
			TestEqual(TEXT("the field is the new biome's"), Sim.Field.TierIndex, static_cast<int64_t>(Index));
			TestEqual(TEXT("the progress stands there"), FString(UTF8_TO_TCHAR(P.CurrentBiomeId.c_str())),
				FString(UTF8_TO_TCHAR(D->Tiers[static_cast<size_t>(Index)].Id.c_str())));
			TestEqual(TEXT("nothing kept from the last biome"), Sim.Field.LiveCount(), int64_t(0));
		}
	}
	TestTrue(TEXT("the timer measured something"), Worst > 0.0);
	TestFalse(TEXT("a biome outside the list is refused"), Sim.Travel(*D, P, N));
	UE_LOG(LogPaw, Display, TEXT("PAW_TRAVEL worst %.3f ms over %d travels"), Worst, 2 * N);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRealmTravelRequestTest, "Pawpocalypse.Realm.TravelRequested",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRealmTravelRequestTest::RunTest(const FString& Parameters)
{
	const Pc::PcGameData* D = GameData(*this);
	if (!D)
	{
		return false;
	}
	Pc::PcProgress P = BusyProgress(*D);
	FPawRealmSim Sim;
	Sim.Start(*D, P);
	for (int32 F = 0; F < 120; ++F)
	{
		Sim.Advance(*D, P, FPawRealmSim::Step);
	}
	TestEqual(TEXT("without a request the cat stays"), Sim.Field.TierIndex, int64_t(0));

	// The travel after a first clear: the field's own clock asks once the death sequence has played.
	Sim.Field.TravelDelay = 0.5 * FPawRealmSim::Step;
	Sim.Advance(*D, P, FPawRealmSim::Step);
	bool bAsked = false;
	for (const Pc::PcFieldEvent& E : Sim.FrameEvents)
	{
		bAsked = bAsked || E.Kind == Pc::PcFieldEventKind::TravelRequested;
	}
	TestTrue(TEXT("the field asked"), bAsked);
	TestEqual(TEXT("and the realm travelled in the same frame"), Sim.Field.TierIndex, int64_t(1));
	TestEqual(TEXT("the progress moved with it"), FString(UTF8_TO_TCHAR(P.CurrentBiomeId.c_str())),
		FString(UTF8_TO_TCHAR(D->Tiers[1].Id.c_str())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRealmReawakenTest, "Pawpocalypse.Realm.ReawakenTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRealmReawakenTest::RunTest(const FString& Parameters)
{
	const Pc::PcGameData* D = GameData(*this);
	if (!D)
	{
		return false;
	}
	// TierController._on_reawakened(): the boss is dismissed at once. Territory (Legacy's biome channel) used to travel
	// from here after the death sequence's beat; Godot's timer swapped the scene out from under the ceremony, so the
	// travel moved to the end of the spend step (designer 2026-11-01): the realm must not move, whatever Territory says.
	// In the first boss biome, mid-fight: a real summon (bait topped up as the cheat overlay would).
	const int32 BossIndex = static_cast<int32>(Pc::RealmIndex(*D, D->Code.BossRealms.front()));
	const Pc::PcBoss* Boss = D->FindBoss(D->Tiers[static_cast<size_t>(BossIndex)].BossData);
	if (!TestNotNull(TEXT("the first boss biome has a boss"), Boss))
	{
		return false;
	}
	for (const bool bTerritory : { true, false })
	{
		Pc::PcProgress P = BusyProgress(*D);
		Pc::CheatUnlockAllTiers(*D, P); // the ladder open: Territory is clamped to it
		P.BranchLevels["legacy"] = bTerritory ? 10 : 0; // level 10: start in biome 6
		FPawRealmSim Sim;
		Sim.Start(*D, P);
		Sim.Travel(*D, P, BossIndex);
		P.BaitKillsByBiome[P.CurrentBiomeId] += Pc::BaitShortfall(*D, P, *Boss) * Boss->KillsPerBait; // the Bait bank (D17 E2)
		P.Catnip = 1e15;
		TestTrue(TEXT("the boss is summoned"), Pc::SummonFieldBoss(Sim.Field, *D, P));
		Sim.Advance(*D, P, 10.0 * FPawRealmSim::Step);
		TestTrue(TEXT("the fight is on"), Sim.Field.bBossUp);
		Sim.OnReawakened(*D, P);
		TestFalse(TEXT("the boss is dismissed"), Sim.Field.bBossUp);
		const int32 Beat = static_cast<int32>(D->Code.BossDeathSequenceSec / FPawRealmSim::Step);
		for (int32 F = 0; F < 3 * Beat; ++F)
		{
			Sim.Advance(*D, P, FPawRealmSim::Step);
		}
		TestEqual(bTerritory ? TEXT("Territory: the realm stays put, three beats on") : TEXT("no Territory: still where it was"),
			Sim.Field.TierIndex, static_cast<int64_t>(BossIndex));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRealmViewTest, "Pawpocalypse.Realm.View",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRealmViewTest::RunTest(const FString& Parameters)
{
	UStaticMesh* Quad = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	const TCHAR* MaterialPaths[] = { TEXT("/Game/Pawpocalypse/Materials/M_FieldFlat.M_FieldFlat"),
		TEXT("/Game/Pawpocalypse/Materials/M_FieldSoft.M_FieldSoft"), TEXT("/Game/Pawpocalypse/Materials/M_FieldDisc.M_FieldDisc"),
		TEXT("/Game/Pawpocalypse/Materials/M_FieldRing.M_FieldRing") };
	UMaterialInterface* Materials[4] = {};
	for (int32 I = 0; I < 4; ++I)
	{
		Materials[I] = LoadObject<UMaterialInterface>(nullptr, MaterialPaths[I]);
		if (!Materials[I])
		{
			AddError(FString::Printf(TEXT("missing %s (py tools/create_field_materials.py)"), MaterialPaths[I]));
		}
	}
	UMaterialInterface* SpriteMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Pawpocalypse/Materials/M_Sprite.M_Sprite"));
	UMaterialInterface* SpriteSoft = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Pawpocalypse/Materials/M_SpriteSoft.M_SpriteSoft"));
	UPawSpriteAtlas* Atlas = LoadObject<UPawSpriteAtlas>(nullptr, TEXT("/Game/Pawpocalypse/Art/Field/DA_FieldAtlas.DA_FieldAtlas"));
	UMaterialInterface* FloorMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Pawpocalypse/Materials/M_Floor.M_Floor"));
	UMaterialInterface* FillMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Pawpocalypse/Materials/M_BackdropFill.M_BackdropFill"));
	TArray<TObjectPtr<UTexture2D>> Floors;
	for (int32 N = 1; N <= 12; ++N)
	{
		Floors.Add(LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Pawpocalypse/Art/Backdrops/T_backdrop_tier_%d.T_backdrop_tier_%d"), N, N)));
		if (!Floors.Last())
		{
			AddError(FString::Printf(TEXT("missing T_backdrop_tier_%d (py tools/import_art.py --run floors)"), N));
		}
	}
	if (!FloorMaterial || !FillMaterial)
	{
		AddError(TEXT("missing M_Floor or M_BackdropFill (py tools/create_field_materials.py)"));
	}
	if (!SpriteMaterial || !SpriteSoft || !Atlas)
	{
		AddError(TEXT("missing a sprite material or DA_FieldAtlas (py tools/create_field_materials.py; py tools/import_art.py --run atlas)"));
	}
	if (!Quad || HasAnyErrors())
	{
		return false;
	}

	// A standalone game instance: its own Game world, its subsystems (the game data loads from the DataTables).
	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	GI->InitializeStandalone();
	UWorld* World = GI->GetWorld();
	UPawRealmSubsystem* Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	UPawGameSubsystem* Game = GI->GetSubsystem<UPawGameSubsystem>();
	bool bOk = TestNotNull(TEXT("the realm subsystem exists in a game world"), Realm)
		&& TestNotNull(TEXT("the game subsystem exists"), Game) && TestTrue(TEXT("the game data is ready"), Game->IsDataReady());
	if (bOk)
	{
		FActorSpawnParameters Params;
		Params.bDeferConstruction = true;
		APawRealmView* View = World->SpawnActor<APawRealmView>(APawRealmView::StaticClass(), FTransform::Identity, Params);
		View->bStartRealmOnBeginPlay = false;
		View->bSampleInput = false;
		View->QuadMesh = Quad;
		View->FlatMaterial = Materials[0];
		View->SoftMaterial = Materials[1];
		View->DiscMaterial = Materials[2];
		View->RingMaterial = Materials[3];
		View->SpriteMaterial = SpriteMaterial;
		View->SpriteSoftMaterial = SpriteSoft;
		View->FieldAtlas = Atlas;
		View->FloorMaterial = FloorMaterial;
		View->BackdropFillMaterial = FillMaterial;
		View->FloorTextures = Floors;
		View->FinishSpawning(FTransform::Identity);
		View->InitView();

		Game->NewGame();
		Pc::PcProgress& P = Game->Progress();
		P.UpgradeLevels["kitten_count"] = 20;
		P.InfestationBonus = 1000.0;
		bOk = TestTrue(TEXT("the realm starts"), Realm->StartRealm());
		const Pc::PcGameData& D = Game->Data();
		// A mouse's draw place is 1 + its pool slot (the first free one, so under max_live_enemies): the renderer's roomy
		// mice band must cover every one (UPawSpriteRenderer::BodyZ; a rigged mouse's layers drew black when crowded in Z).
		TestTrue(TEXT("every live mouse's draw place is in the mice's depth band"),
			static_cast<double>(D.Code.MaxLiveEnemies) + 1.0 < UPawSpriteRenderer::MicePlaces);
		const Pc::PcField& F = Realm->GetSim().Field;
		for (int32 Frame = 0; bOk && Frame < 240 && F.LiveCount() < D.Code.MaxLiveEnemies - 5; ++Frame)
		{
			Realm->SetCat(F.Bounds.CentreX() + 300.0 * FMath::Cos(Frame * 0.05), F.Bounds.CentreY());
			View->RunFrame(FPawRealmSim::Step);
		}
		TestTrue(FString::Printf(TEXT("the field filled (%lld mice)"), F.LiveCount()), F.LiveCount() >= D.Code.MaxLiveEnemies - 5);

		double TotalMs = 0.0, WorstMs = 0.0;
		const int32 Frames = 300;
		int32 Kills = 0, Strikes = 0, Events = 0;
		const int64 SpawnedBefore = View->EffectsSpawned();
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			// The cat hunts the nearest mouse (the soak's cat): the paw strikes, kills come, the effects follow.
			double TX = F.CatX, TY = F.CatY, Best = 1e30;
			for (const int32 S : F.Live)
			{
				const double Dist = FMath::Square(F.X[S] - F.CatX) + FMath::Square(F.Y[S] - F.CatY);
				if (Dist < Best)
				{
					Best = Dist;
					TX = F.X[S];
					TY = F.Y[S];
				}
			}
			Realm->SetCat(TX, TY);
			const double T0 = FPlatformTime::Seconds();
			View->RunFrame(FPawRealmSim::Step);
			const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
			TotalMs += Ms;
			WorstMs = FMath::Max(WorstMs, Ms);
			for (const Pc::PcFieldEvent& E : Realm->GetSim().FrameEvents)
			{
				++Events;
				Kills += E.Kind == Pc::PcFieldEventKind::Killed ? 1 : 0;
				Strikes += E.Kind == Pc::PcFieldEventKind::Strike ? 1 : 0;
			}
		}
		const int64 Spawned = View->EffectsSpawned() - SpawnedBefore;
		UE_LOG(LogPaw, Display, TEXT("PAW_EVENTS %d events over %d frames: %d kills, %d strikes, %lld effects"), Events, Frames, Kills,
			Strikes, Spawned);
		const double MeanMs = TotalMs / Frames;
		UE_LOG(LogPaw, Display, TEXT("PAW_FRAME %lld mice, %lld kittens: advance + draw mean %.3f ms, worst %.3f ms (draw %.3f ms)"),
			F.LiveCount(), F.AwakeKittens(), MeanMs, WorstMs, View->LastDrawMs());
		TestTrue(FString::Printf(TEXT("advance + draw at 320 mice: mean %.3f ms < %.0f ms"), MeanMs, FrameBudgetMs), MeanMs < FrameBudgetMs);

		// What is drawn is what is live: every mouse, kitten, add, the boss and the cat; a shadow under each that stands (the
		// boss and its adds have none: boss.tscn, boss_add.gd).
		UPawSpriteRenderer* R = View->GetRenderer();
		const int32 Bodies = static_cast<int32>(F.LiveCount() + F.AwakeKittens() + static_cast<int64_t>(F.Adds.size())) + (F.bBossUp ? 1 : 0) + 1;
		// A rigged mouse (R5) is drawn as its layers: its body layer is its one body, its wings and legs are drawn beside it.
		int32 Limbs = 0, LimbsWanted = 0;
		for (const FPawSprite& S : R->LastSprites(EPawLayer::Bodies))
		{
			Limbs += Atlas->Cells.IsValidIndex(S.Cell) && !Atlas->Cell(S.Cell).RigOf.IsEmpty() && Atlas->Cell(S.Cell).RigOrder != 0 ? 1 : 0;
		}
		for (const int32 S : F.Live)
		{
			const Pc::PcEnemy& E = Game->Data().Enemies[static_cast<size_t>(F.Kind[static_cast<size_t>(S)])];
			const TArray<FPawRigLayer>* Rig = Atlas->Rig(Atlas->Find(UTF8_TO_TCHAR(E.TexturePath.c_str())));
			LimbsWanted += Rig ? Rig->Num() - 1 : 0;
		}
		TestEqual(TEXT("one body per live thing"), R->DrawnCount(EPawLayer::Bodies) - Limbs, Bodies);
		TestEqual(TEXT("every rigged mouse's wings and legs"), Limbs, LimbsWanted);
		TestTrue(TEXT("some rigged mice on this field"), LimbsWanted > 0);
		TestEqual(TEXT("one shadow per mouse, kitten and the cat"), R->DrawnCount(EPawLayer::Shadows),
			static_cast<int32>(F.LiveCount() + F.AwakeKittens()) + 1);
		// Every body wears its own art, and Godot's draw order holds: the mice (z 0) under the kittens (z 1) under the cat (z 2).
		double MiceTop = -1.0, KittenLow = 1e9, KittenTop = -1.0, CatOrder = -1.0;
		int32 Artless = 0;
		for (const FPawSprite& S : R->LastSprites(EPawLayer::Bodies))
		{
			Artless += S.Cell == INDEX_NONE ? 1 : 0;
			const FString Id = Atlas->Cells.IsValidIndex(S.Cell) ? Atlas->Cell(S.Cell).Id : FString();
			if (Id.StartsWith(TEXT("enemy_")))
			{
				MiceTop = FMath::Max(MiceTop, S.Order);
			}
			else if (Id == TEXT("kitten_basic"))
			{
				KittenLow = FMath::Min(KittenLow, S.Order);
				KittenTop = FMath::Max(KittenTop, S.Order);
				TestEqual(TEXT("a kitten is 96 px (Kitten.SPRITE_TARGET_PX)"), S.Width, PawLook::KittenPx);
			}
			else if (Id.StartsWith(TEXT("player_cat_stage_")))
			{
				CatOrder = S.Order;
			}
		}
		TestEqual(TEXT("every body has its atlas art"), Artless, 0);
		TestTrue(TEXT("mice drawn"), MiceTop >= 0.0);
		TestTrue(TEXT("the cat drawn in its stage's art"), CatOrder >= 0.0);
		TestTrue(TEXT("the kittens over every mouse"), F.AwakeKittens() == 0 || KittenLow > MiceTop);
		TestTrue(TEXT("the cat over every kitten and mouse"), CatOrder > KittenTop && CatOrder > MiceTop);
		for (const FPawSprite& S : R->LastSprites(EPawLayer::Shadows))
		{
			TestEqual(TEXT("a shadow is the ground shadow"), Atlas->Cells.IsValidIndex(S.Cell) ? Atlas->Cell(S.Cell).Id : FString(),
				FString(TEXT("ground_shadow")));
		}
		TestEqual(TEXT("one floor"), R->DrawnCount(EPawLayer::Floor), 1);
		// The biome's music (TierController._ready -> AudioManager.play_music(tier_data.id)), through the realm's frame.
		if (const UPawAudioSubsystem* Audio = GI->GetSubsystem<UPawAudioSubsystem>())
		{
			TestEqual(TEXT("the biome's music plays"), Audio->CurrentTrack(), Realm->CurrentTierId());
		}
		TestTrue(TEXT("the pool holds at least what is drawn"), R->PoolSize(EPawLayer::Bodies) >= Bodies);
		TestTrue(TEXT("the paw struck and killed"), Strikes > 0 && Kills > 0);
		TestTrue(TEXT("every strike and kill made its effect"), Spawned >= Strikes + Kills);

		// Review CR5 (2026-11-10): an empty effects band is cleared once, when it empties, not every frame: each clear (and each
		// create) rebuilds the mesh's collision body and its scene proxy, and three bands did it every frame, empty or not.
		{
			const FPawFxCanvas Empty{};
			for (const EPawFxBand B : { EPawFxBand::Ground, EPawFxBand::Under, EPawFxBand::Over })
			{
				R->SetFx(B, Empty, FillMaterial); // whatever the hunt left in them goes
			}
			const int32 Before = R->FxSectionRebuilds();
			for (int32 Frame = 0; Frame < 60; ++Frame)
			{
				for (const EPawFxBand B : { EPawFxBand::Ground, EPawFxBand::Under, EPawFxBand::Over })
				{
					R->SetFx(B, Empty, FillMaterial);
				}
			}
			TestEqual(TEXT("60 frames of empty effects bands rebuild no section"), R->FxSectionRebuilds() - Before, 0);
			FPawFxCanvas Ring;
			Ring.SetZ(UPawSpriteRenderer::GroundFxZ);
			Ring.Circle(FVector2D(900.0, 500.0), 40.0, FLinearColor(1.0f, 1.0f, 1.0f, 0.5f));
			R->SetFx(EPawFxBand::Ground, Ring, FillMaterial);
			TestEqual(TEXT("an effect drawn: one section made"), R->FxSectionRebuilds() - Before, 1);
			TestEqual(TEXT("and drawn: its triangles"), R->FxTriangles(EPawFxBand::Ground), 64);
			R->SetFx(EPawFxBand::Ground, Empty, FillMaterial);
			TestEqual(TEXT("the band empties: one clear"), R->FxSectionRebuilds() - Before, 2);
			TestEqual(TEXT("and draws nothing"), R->FxTriangles(EPawFxBand::Ground), 0);
		}

		// Review CR17: a steady frame looks no drawing up by name (the cells are kept while what they name stays), and a change
		// of what they name (here the job, so the hat) shows at once.
		if (const UPawSpriteAtlas* LookupAtlas = R->GetAtlas())
		{
			const std::string WasJob = P.RunJob;
			const int32 SmithHat = LookupAtlas->Find(TEXT("hat_blacksmith"));
			const int32 BurglarHat = LookupAtlas->Find(TEXT("hat_cat_burglar"));
			auto HatDrawn = [R](int32 Cell)
			{
				return Cell != INDEX_NONE && R->LastSprites(EPawLayer::Overlays).ContainsByPredicate([Cell](const FPawSprite& S) { return S.Cell == Cell; });
			};
			P.RunJob = "blacksmith";
			View->Draw(); // whatever changed since the last frame is looked up here, once
			const int32 Finds = LookupAtlas->FindCount();
			for (int32 Frame = 0; Frame < 30; ++Frame)
			{
				View->Draw();
			}
			TestEqual(TEXT("30 steady frames look up no drawing by name"), LookupAtlas->FindCount() - Finds, 0);
			TestTrue(TEXT("and still draw the cat"), R->DrawnCount(EPawLayer::Bodies) > 0);
			TestTrue(TEXT("and its job's hat"), HatDrawn(SmithHat));
			P.RunJob = "cat_burglar";
			View->Draw();
			TestTrue(TEXT("another job: its hat at once"), HatDrawn(BurglarHat) && !HatDrawn(SmithHat));
			P.RunJob = WasJob;
			View->Draw();
		}

		// Settings > Video > Visual effects off: the same hunt spawns none (vfx_burst.gd honours the effect budget).
		if (UPawSettingsSubsystem* Settings = GI->GetSubsystem<UPawSettingsSubsystem>())
		{
			Settings->SetEffectsEnabled(false);
			const int64 OffBefore = View->EffectsSpawned();
			int32 OffKills = 0;
			for (int32 Frame = 0; Frame < 120; ++Frame)
			{
				double TX = F.CatX, TY = F.CatY, Best = 1e30;
				for (const int32 S : F.Live)
				{
					const double Dist = FMath::Square(F.X[S] - F.CatX) + FMath::Square(F.Y[S] - F.CatY);
					if (Dist < Best)
					{
						Best = Dist;
						TX = F.X[S];
						TY = F.Y[S];
					}
				}
				Realm->SetCat(TX, TY);
				View->RunFrame(FPawRealmSim::Step);
				for (const Pc::PcFieldEvent& E : Realm->GetSim().FrameEvents)
				{
					OffKills += E.Kind == Pc::PcFieldEventKind::Killed ? 1 : 0;
				}
			}
			TestTrue(TEXT("effects off: the hunt still kills"), OffKills > 0);
			TestEqual(TEXT("effects off: no burst"), View->EffectsSpawned(), OffBefore);
			Settings->SetEffectsEnabled(true);
		}
		else
		{
			AddError(TEXT("no settings subsystem"));
		}

		// Travel with the redraw: the pool stays (parked, never removed), the next frame draws the empty field.
		const int32 Pool = R->PoolSize(EPawLayer::Bodies);
		double WorstTravel = 0.0;
		for (int32 I = 0; I < static_cast<int32>(D.Tiers.size()); ++I)
		{
			const double T0 = FPlatformTime::Seconds();
			Realm->Travel(I);
			View->RunFrame(0.0);
			WorstTravel = FMath::Max(WorstTravel, (FPlatformTime::Seconds() - T0) * 1000.0);
		}
		TestTrue(FString::Printf(TEXT("travel with the redraw: worst %.3f ms < %.0f ms"), WorstTravel, TravelBudgetMs), WorstTravel < TravelBudgetMs);
		// The backdrop's props share the sprite components, and each biome has its own count: the pool may grow, never shrink.
		TestTrue(TEXT("the pool is never shrunk"), R->PoolSize(EPawLayer::Bodies) >= Pool);
		// Every biome travelled to wore its own backdrop: the floor, the props PcBackdrop laid out, each with its art.
		for (int32 I = 0; I < static_cast<int32>(D.Tiers.size()); ++I)
		{
			Realm->Travel(I);
			View->RunFrame(0.0);
			const FString Tier = UTF8_TO_TCHAR(D.Tiers[static_cast<size_t>(I)].Id.c_str());
			const Pc::PcBackdropLayout& B = View->GetBackdrop();
			TestEqual(FString::Printf(TEXT("%s: the layout is this biome's"), *Tier), FString(UTF8_TO_TCHAR(B.Id.c_str())), Tier);
			TestTrue(FString::Printf(TEXT("%s: its floor texture"), *Tier),
				R->GetFloorTexture() && R->GetFloorTexture()->GetName() == TEXT("T_backdrop_") + Tier);
			TestEqual(FString::Printf(TEXT("%s: every prop drawn"), *Tier), R->DrawnCount(EPawLayer::Props), static_cast<int32>(B.Props.size()));
			TestTrue(FString::Printf(TEXT("%s: props laid out"), *Tier), B.Props.size() > 4);
			int32 PropsArtless = 0;
			for (const FPawSprite& S : R->LastSprites(EPawLayer::Props))
			{
				PropsArtless += Atlas->Cells.IsValidIndex(S.Cell) ? 0 : 1;
			}
			TestEqual(FString::Printf(TEXT("%s: every prop has its art"), *Tier), PropsArtless, 0);
			int32 Want = 0;
			for (const Pc::PcBackdropBlob& Blob : B.Blobs)
			{
				Want += static_cast<int32>(Blob.Xs.size()) * (Blob.bPuddle ? 3 : 1);
			}
			TestEqual(FString::Printf(TEXT("%s: the patches and puddles meshed"), *Tier), R->BackdropFillTriangles(), Want);
		}
		const int32 Builds = View->BackdropBuilds();
		View->RunFrame(0.0);
		View->RunFrame(0.0);
		TestEqual(TEXT("an unchanged biome is not laid out again"), View->BackdropBuilds(), Builds);
		TestEqual(TEXT("an empty field draws the cat and its kittens only"), R->DrawnCount(EPawLayer::Bodies),
			static_cast<int32>(F.AwakeKittens()) + 1);
		UE_LOG(LogPaw, Display, TEXT("PAW_VIEW_TRAVEL worst %.3f ms"), WorstTravel);
		View->Destroy();
	}
	GI->Shutdown();
	if (World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
	return bOk;
}

#endif
