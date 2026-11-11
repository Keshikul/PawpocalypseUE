// Pawpocalypse.Realm.Vfx.*: the effects (phase 6 slice 3). Which effects each moment of the field makes, as tier_controller.gd
// and player.gd make them; the effects setting; the rate limits; the boss's delayed rings; the draw calls' tessellation; the
// depth bands staying where the camera sees them.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "PcData.h"
#include "PcField.h"
#include "Realm/PawFieldLook.h"
#include "Realm/PawRealmSim.h"
#include "Realm/PawVfx.h"
#include "Render/PawFxCanvas.h"
#include "Render/PawSpriteRenderer.h"

namespace
{
	using EKind = FPawVfx::EKind;

	struct FVfxWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		FPawRealmSim Sim;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			if (!T.TestTrue(TEXT("the game data"), Game && Game->IsDataReady()))
			{
				return false;
			}
			Game->NewGame();
			Sim.Start(Game->Data(), Game->Progress());
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
		FPawVfx::FContext Context(double Budget = 1.0, double Dt = 0.0)
		{
			FPawVfx::FContext C;
			C.D = &Game->Data();
			C.P = &Game->Progress();
			C.Sim = &Sim;
			C.Cat = FVector2D(900.0, 500.0);
			C.Budget = Budget;
			C.Dt = Dt;
			return C;
		}
		// One frame of these events.
		void Events(FPawVfx& V, std::initializer_list<Pc::PcFieldEvent> List, double Budget = 1.0)
		{
			Sim.FrameEvents.assign(List.begin(), List.end());
			V.OnFrameEvents(Context(Budget));
			Sim.FrameEvents.clear();
		}
	};

	Pc::PcFieldEvent Event(Pc::PcFieldEventKind Kind, uint32 Flags = 0, double Value = 0.0)
	{
		Pc::PcFieldEvent E;
		E.Kind = Kind;
		E.X = 900.0;
		E.Y = 500.0;
		E.ToX = 1000.0;
		E.ToY = 480.0;
		E.Value = Value;
		E.Flags = Flags;
		return E;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawVfxMappingTest, "Pawpocalypse.Realm.Vfx.Mapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawVfxMappingTest::RunTest(const FString& Parameters)
{
	FVfxWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	using K = Pc::PcFieldEventKind;
	namespace Flag = Pc::FieldFlag;
	{
		// A Pounce: a burst and the slam, the slam's dust; a Reflex pounce is quiet: no dust.
		FPawVfx V;
		W.Events(V, { Event(K::Pounce, 0, 160.0) });
		TestEqual(TEXT("pounce: one burst"), V.LiveOf(EKind::Burst), 1);
		TestEqual(TEXT("pounce: the slam"), V.LiveOf(EKind::Slam), 1);
		TestEqual(TEXT("pounce: the slam's dust"), V.LiveOf(EKind::Shards), 1);
		FPawVfx Q;
		W.Events(Q, { Event(K::Pounce, Flag::Auto, 160.0) });
		TestEqual(TEXT("a Reflex pounce raises no dust"), Q.LiveOf(EKind::Shards), 0);
		TestEqual(TEXT("a Reflex pounce still slams"), Q.LiveOf(EKind::Slam), 1);
	}
	{
		// A strike: the swipe; Riddle arming: its red burst; Execute's threshold: red shards and a burst.
		FPawVfx V;
		W.Events(V, { Event(K::Strike) });
		TestEqual(TEXT("a strike swipes"), V.LiveOf(EKind::Swipe), 1);
		TestEqual(TEXT("a plain strike makes nothing else"), V.Live(), 1);
		W.Events(V, { Event(K::Strike, Flag::Crit | Flag::Riddle) });
		TestEqual(TEXT("Riddle armed: a burst"), V.LiveOf(EKind::Burst), 1);
		W.Events(V, { Event(K::Strike, Flag::Threshold | Flag::Execute) });
		TestEqual(TEXT("the threshold: shards"), V.LiveOf(EKind::Shards), 1);
		TestEqual(TEXT("the threshold: a second burst"), V.LiveOf(EKind::Burst), 2);
		W.Events(V, { Event(K::Strike, Flag::Execute) });
		TestEqual(TEXT("Riddle's spend (Execute without the threshold) adds no shards"), V.LiveOf(EKind::Shards), 1);
	}
	{
		// A golden kill: two bursts and the gold shards, then the kill's pop; a plain kill: the pop alone.
		FPawVfx V;
		W.Events(V, { Event(K::Killed, Flag::Golden) });
		TestEqual(TEXT("golden kill: three rings"), V.LiveOf(EKind::Burst), 3);
		TestEqual(TEXT("golden kill: the gold spray"), V.LiveOf(EKind::Shards), 1);
		FPawVfx P;
		W.Events(P, { Event(K::Killed) });
		TestEqual(TEXT("a kill pops"), P.Live(), 1);
		// The bonus pops are rate-limited per kind (70 ms): two bounties in one frame, one spray.
		FPawVfx B;
		W.Events(B, { Event(K::Killed, Flag::Bounty), Event(K::Killed, Flag::Bounty), Event(K::Killed, Flag::Toyed) });
		TestEqual(TEXT("bounty and toyed: one spray each, the second bounty held back"), B.LiveOf(EKind::Shards), 2);
	}
	{
		// Overkill's splash beams: three a frame at most.
		FPawVfx V;
		W.Events(V, { Event(K::Splash), Event(K::Splash), Event(K::Splash), Event(K::Splash), Event(K::Splash) });
		TestEqual(TEXT("three splash beams a frame"), V.LiveOf(EKind::Beam), 3);
		W.Events(V, { Event(K::Splash) });
		TestEqual(TEXT("the next frame has its own three"), V.LiveOf(EKind::Beam), 4);
	}
	{
		// The boss: its death rings come 0.12 s apart.
		FPawVfx V;
		W.Events(V, { Event(K::BossDefeated) });
		TestEqual(TEXT("no ring on the kill's frame"), V.Live(), 0);
		V.Tick(W.Context(1.0, 0.01));
		TestEqual(TEXT("the first ring at once"), V.LiveOf(EKind::Burst), 1);
		V.Tick(W.Context(1.0, 0.25));
		TestEqual(TEXT("all three by a quarter second"), V.LiveOf(EKind::Burst), 3);
		W.Events(V, { Event(K::BossWindow), Event(K::BossPhase), Event(K::AddSpawned), Event(K::AddIntercepted) });
		TestEqual(TEXT("window, phase, add, intercept: a ring each"), V.LiveOf(EKind::Burst), 7);
	}
	{
		// The giblets, the catnip bomb, the Prism, the aura's pulse, Hammerfall.
		FPawVfx V;
		W.Events(V, { Event(K::KittenBlast, 0, 150.0), Event(K::KittenBlast, 0, 150.0) });
		TestEqual(TEXT("giblets: a burst each"), V.LiveOf(EKind::Burst), 2);
		TestEqual(TEXT("giblets: the gore once (60 ms apart)"), V.LiveOf(EKind::Shards), 2);
		FPawVfx M;
		W.Events(M, { Event(K::CatnipBomb, 0, 240.0), Event(K::PrismBurst, 0, 300.0), Event(K::AuraPulse, 0, 250.0), Event(K::Cleave) });
		TestEqual(TEXT("bomb, prism, pulse: bursts"), M.LiveOf(EKind::Burst), 3);
		TestEqual(TEXT("the bomb's spray"), M.LiveOf(EKind::Shards), 1);
		TestEqual(TEXT("Hammerfall's beam"), M.LiveOf(EKind::Beam), 1);
	}
	{
		// Effects off: nothing at all (every spawn checks the budget); low: the shards thinned.
		FPawVfx V;
		W.Events(V, { Event(K::Pounce, 0, 160.0), Event(K::Strike, Flag::Threshold), Event(K::Killed, Flag::Golden) }, 0.0);
		TestEqual(TEXT("effects off: none"), V.Live(), 0);
		TestEqual(TEXT("effects off: none counted"), static_cast<int64>(V.Spawned()), static_cast<int64>(0));
	}
	{
		// Lifetimes: a burst is gone after its duration, a swipe after 0.24 s.
		FPawVfx V;
		const FPawVfx::FContext C = W.Context();
		V.Burst(C, FVector2D(100.0, 100.0), FLinearColor::White, 50.0, 0.35);
		V.Swipe(C, FVector2D(0.0, 0.0), FVector2D(10.0, 0.0), 62.0, false, false);
		V.Tick(W.Context(1.0, 0.2));
		TestEqual(TEXT("both alive at 0.2 s"), V.Live(), 2);
		V.Tick(W.Context(1.0, 0.05));
		TestEqual(TEXT("the swipe gone at 0.25 s"), V.LiveOf(EKind::Swipe), 0);
		V.Tick(W.Context(1.0, 0.11));
		TestEqual(TEXT("the burst gone at 0.36 s"), V.Live(), 0);
		// A held field freezes them (a paused tree runs no tween).
		V.Burst(C, FVector2D(100.0, 100.0), FLinearColor::White, 50.0, 0.35);
		V.Tick(W.Context(1.0, 0.0));
		V.Tick(W.Context(1.0, 0.0));
		TestEqual(TEXT("paused: still there"), V.Live(), 1);
	}
	{
		// The poop zone shows for its duration (PoopDropped carries it in ToX).
		FPawVfx V;
		Pc::PcFieldEvent Poop = Event(K::PoopDropped, 0, 120.0);
		Poop.ToX = 3.0;
		W.Events(V, { Poop });
		TestTrue(TEXT("the poop shows"), V.IsPoopShowing());
		FPawFxCanvas G, U, O;
		V.Draw(W.Context(), G, U, O);
		TestTrue(TEXT("the poop draws under the creatures"), !U.IsEmpty());
		V.Tick(W.Context(1.0, 3.1));
		TestFalse(TEXT("gone after its duration"), V.IsPoopShowing());
	}
	W.Close();
	return true;
}

namespace
{
	FColor Rgb(uint8 R, uint8 G, uint8 B) { return FColor(R, G, B, 255); }
	// The vertices of a canvas whose colour (alpha ignored) is Colour.
	TArray<FVector2D> VertsOf(const FPawFxCanvas& C, const FColor& Colour)
	{
		TArray<FVector2D> Out;
		for (int32 I = 0; I < C.Colors.Num(); ++I)
		{
			if (C.Colors[I].R == Colour.R && C.Colors[I].G == Colour.G && C.Colors[I].B == Colour.B)
			{
				Out.Add(FVector2D(C.Verts[I].X, C.Verts[I].Y));
			}
		}
		return Out;
	}
	FVector2D Centroid(const TArray<FVector2D>& Points)
	{
		FVector2D Sum = FVector2D::ZeroVector;
		for (const FVector2D& P : Points)
		{
			Sum += P;
		}
		return Points.Num() > 0 ? Sum / Points.Num() : Sum;
	}
}

// The restyle's look of the aura and the strike (D3, designer 2026-11-09; tools/art/fx_look.py -> Realm/PawFxLook.h): the
// aura lifted white with a Bone ring on a Plum edge (not Godot's gold), the clutching paw at the arc's tip with its toes along
// the motion, crits in Butter (not Godot's pink). The palette's bytes are written out here, not read from PawFxLook.h, so a
// wrong token in the generated header fails too.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawVfxLookTest, "Pawpocalypse.Realm.Vfx.Look",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawVfxLookTest::RunTest(const FString& Parameters)
{
	FVfxWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	const FColor Bone = Rgb(0xF6, 0xEE, 0xF8), Ink = Rgb(0x2A, 0x1A, 0x33), Butter = Rgb(0xFF, 0xD2, 0x3F);
	const FColor GodotGold = Rgb(246, 194, 92), GodotPink = Rgb(255, 107, 184), GodotEarned = Rgb(255, 158, 219);
	const FColor Pad = Rgb(0xF8, 0xF1, 0xDE), Claw = Rgb(0xF6, 0xF6, 0xF6), SwipeBone = Rgb(254, 246, 234);
	{
		// The aura: owned, polled, drawn round the cat.
		FPawVfx V;
		W.Game->Progress().BranchLevels["unlock_aura"] = 1;
		V.Tick(W.Context(1.0, 0.2));
		FPawFxCanvas G, U, O;
		V.Draw(W.Context(), G, U, O);
		W.Game->Progress().BranchLevels["unlock_aura"] = 0;
		const TArray<FVector2D> Ring = VertsOf(O, Bone);
		TestTrue(TEXT("the aura's ring is Bone"), Ring.Num() > 0);
		TestTrue(TEXT("on a Plum edge"), VertsOf(O, Ink).Num() > 0);
		bool bLift = false;
		for (const FColor& C : O.Colors)
		{
			bLift |= C.R == 255 && C.G == 255 && C.B == 255 && C.A == 26;
		}
		TestTrue(TEXT("the inside lifted white at 10 %"), bLift);
		TestEqual(TEXT("no gold (Godot's ring)"), VertsOf(O, GodotGold).Num(), 0);
		const FVector2D Cat = W.Context().Cat;
		double Far = 0.0;
		for (const FVector2D& P : Ring)
		{
			Far = FMath::Max(Far, (P - Cat).Size());
		}
		TestTrue(TEXT("the ring round the cat at the aura's radius"), Far > 60.0 && Far < 300.0);
	}
	{
		// A plain strike at a quarter of its sweep: the arc, and the paw at its tip, toes along the motion.
		FPawVfx V;
		const FPawVfx::FContext C = W.Context();
		const FVector2D At(400.0, 400.0), To(460.0, 400.0); // the enemy straight right: the swipe's angle 0
		V.Swipe(C, At, To, 62.0, false, false);
		V.Tick(W.Context(1.0, 0.06));
		FPawFxCanvas G, U, O;
		V.Draw(W.Context(), G, U, O);
		const TArray<FVector2D> PadVerts = VertsOf(O, Pad), ClawVerts = VertsOf(O, Claw);
		TestTrue(TEXT("the paw's pads drawn"), PadVerts.Num() > 0);
		TestTrue(TEXT("its claws out"), ClawVerts.Num() > 0);
		TestTrue(TEXT("the arc Bone-white (Godot's)"), VertsOf(O, SwipeBone).Num() > 0);
		TestEqual(TEXT("no Butter on a plain strike"), VertsOf(O, Butter).Num(), 0);
		// vfx_swipe.gd: radius = reach x 0.88, the lead at -span/2 + span x sweep. Sweep 0.25.
		const double Sweep = 0.06 / 0.24, Lead = -1.55 * 0.5 + 1.55 * Sweep, R = 62.0 * 0.88;
		const FVector2D Tip = At + FVector2D(FMath::Cos(Lead), FMath::Sin(Lead)) * R;
		const FVector2D Motion(-FMath::Sin(Lead), FMath::Cos(Lead));
		const FVector2D PadAt = Centroid(PadVerts), ClawAt = Centroid(ClawVerts);
		TestTrue(TEXT("the paw at the arc's tip"), (PadAt - Tip).Size() < 12.0);
		TestTrue(TEXT("its claws ahead, along the motion"), FVector2D::DotProduct((ClawAt - PadAt).GetSafeNormal(), Motion) > 0.8);
		TestTrue(TEXT("the paw at its size (46 px box)"), (ClawAt - PadAt).Size() > 6.0 && (ClawAt - PadAt).Size() < 23.0);
	}
	{
		// Crits, luck's and earned: Butter, never Godot's pinks.
		for (const bool bEarned : { false, true })
		{
			FPawVfx V;
			V.Swipe(W.Context(), FVector2D(400.0, 400.0), FVector2D(460.0, 400.0), 62.0, true, bEarned);
			V.Tick(W.Context(1.0, 0.12));
			FPawFxCanvas G, U, O;
			V.Draw(W.Context(), G, U, O);
			TestTrue(bEarned ? TEXT("an earned crit is Butter") : TEXT("a crit is Butter"), VertsOf(O, Butter).Num() > 0);
			TestEqual(TEXT("no pink"), VertsOf(O, GodotPink).Num() + VertsOf(O, GodotEarned).Num(), 0);
		}
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawVfxRingOnCatTest, "Pawpocalypse.Realm.Vfx.RingOnCat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawVfxRingOnCatTest::RunTest(const FString& Parameters)
{
	// F39 (a Godot fault the port does not copy, D16): the job ring (and the Cone of Shame's) is a child of the cat, but Godot
	// placed it with global coordinates before it entered the tree, so it drew at the cat's position PLUS its spawn point:
	// twice the cat's position, usually off screen. Here it is round the cat, and it follows the cat.
	FVfxWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	FPawVfx V;
	FPawVfx::FContext C = W.Context();
	V.Tick(C); // the job worn is known: nothing yet
	Pc::PcProgress& P = W.Game->Progress();
	P.RunJob = W.Game->Data().Jobs.front().Id;
	V.Tick(C); // a job taken: its ring
	C.Dt = 0.1;
	C.Cat = FVector2D(950.0, 520.0); // the cat moves on
	V.Tick(C);
	FPawFxCanvas Ground, Under, Over;
	V.Draw(C, Ground, Under, Over);
	int32 Round = 0, Away = 0;
	for (const FPawFxCanvas* Band : { &Ground, &Under, &Over })
	{
		for (const FVector& Vert : Band->Verts)
		{
			const double Near = FVector2D::Distance(FVector2D(Vert.X, Vert.Y), C.Cat);
			Round += Near > 20.0 && Near < 110.0 ? 1 : 0;
			Away += FVector2D::Distance(FVector2D(Vert.X, Vert.Y), FVector2D(1850.0, 1020.0)) < 150.0 ? 1 : 0;
		}
	}
	TestTrue(FString::Printf(TEXT("the job ring is round the cat where it is now (%d points)"), Round), Round >= 32);
	TestEqual(TEXT("and nothing at twice the cat's position"), Away, 0);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawVfxCanvasTest, "Pawpocalypse.Realm.Vfx.Canvas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawVfxCanvasTest::RunTest(const FString& Parameters)
{
	FPawFxCanvas C;
	C.SetZ(42.0);
	C.Circle(FVector2D(0.0, 0.0), 10.0, FLinearColor(1.0f, 0.0f, 0.0f, 0.5f));
	TestEqual(TEXT("a circle: a fan of 64"), C.Indices.Num(), 64 * 3);
	TestEqual(TEXT("its depth"), C.Verts[0].Z, 42.0);
	TestEqual(TEXT("sRGB bytes kept as given"), C.Colors[0], FColor(255, 0, 0, 128));
	C.Reset();
	const FVector2D Line[4] = { FVector2D(0.0, 0.0), FVector2D(10.0, 0.0), FVector2D(10.0, 10.0), FVector2D(0.0, 10.0) };
	C.Polyline(Line, FLinearColor::White, 4.0);
	TestEqual(TEXT("a polyline: two triangles a segment"), C.Indices.Num(), 3 * 2 * 3);
	const double Width = (C.Verts[1] - C.Verts[0]).Size();
	TestEqual(TEXT("its width at an end"), Width, 4.0, 1e-6);
	C.Reset();
	C.Arc(FVector2D(0.0, 0.0), 50.0, 0.0, UE_DOUBLE_TWO_PI, 48, FLinearColor::White, 3.0);
	TestEqual(TEXT("an arc of 48 points: 47 segments"), C.Indices.Num(), 47 * 6);
	C.Reset();
	C.Circle(FVector2D(0.0, 0.0), 10.0, FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	TestTrue(TEXT("a transparent circle draws nothing"), C.IsEmpty());
	TestEqual(TEXT("from_hsv red"), FPawFxCanvas::FromHsv(0.0, 1.0, 1.0), FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
	TestEqual(TEXT("from_hsv a third"), FPawFxCanvas::FromHsv(1.0 / 3.0, 1.0, 1.0).G, 1.0f);

	// The bands: every effect and sprite depth inside what the camera's automatic ortho planes show (Z 400 is clipped).
	TestTrue(TEXT("the top effects at most Z 300"), UPawSpriteRenderer::TopFxZ <= 300.0);
	TestTrue(TEXT("over every body"), UPawSpriteRenderer::TopFxZ > UPawSpriteRenderer::BodyZ(PawLook::Order(5, PawLook::TreeSpan)));
	// The mice's band gives a rigged mouse's layers room (0.12 of a place = 0.048 world units; 0.0096 still drew black), and
	// every band keeps Godot's order: the boss over every mouse, z 1 above every place of z 0, the order rising throughout.
	TestTrue(TEXT("a rigged mouse's layers stay apart in depth"),
		UPawSpriteRenderer::BodyZ(PawLook::Order(0, 320.12)) - UPawSpriteRenderer::BodyZ(PawLook::Order(0, 320.0)) >= 0.04);
	TestTrue(TEXT("the boss over every mouse"), UPawSpriteRenderer::BodyZ(PawLook::Order(0, PawLook::TreeSpan - 1.0))
		> UPawSpriteRenderer::BodyZ(PawLook::Order(0, UPawSpriteRenderer::MicePlaces - 0.01)));
	bool bRising = true;
	for (double O = 0.0; O < 12000.0; O += 0.5)
	{
		bRising &= UPawSpriteRenderer::BodyZ(O + 0.5) > UPawSpriteRenderer::BodyZ(O);
	}
	TestTrue(TEXT("depth rises with the draw order everywhere"), bRising);
	TestTrue(TEXT("the kittens over the boss"), UPawSpriteRenderer::BodyZ(PawLook::Order(1, 0.0))
		> UPawSpriteRenderer::BodyZ(PawLook::Order(0, PawLook::TreeSpan - 1.0)));
	TestTrue(TEXT("the ground effects under the shadows"), UPawSpriteRenderer::GroundFxZ < 20.0);
	TestTrue(TEXT("the under effects over the shadows, under the bodies"), UPawSpriteRenderer::UnderFxZ > 20.0
		&& UPawSpriteRenderer::UnderFxZ < UPawSpriteRenderer::BodyZ(0.0));
	return true;
}

#endif
