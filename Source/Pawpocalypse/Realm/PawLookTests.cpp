// Pawpocalypse.Realm.Look.*: how the field's creatures look (phase 6 slice 1). The pure maths of Realm/PawFieldLook.h against
// numbers worked from the Godot code by hand, and the field atlas against the data: every drawing the field can ask for is in it.

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PawGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcBackdrop.h"
#include "PcData.h"
#include "Realm/PawFieldLook.h"
#include "Render/PawSpriteAtlas.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLookRulesTest, "Pawpocalypse.Realm.Look.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLookRulesTest::RunTest(const FString& Parameters)
{
	using namespace PawLook;
	// GroundShadow.place(creature 64, ground): width 64 x 0.8 = 51.2; on the feet at 64 x 0.46 = 29.44; alpha 0.7. The shadow
	// texture is 256 x 96, so its height keeps that aspect.
	const FShadow G = Shadow(64.0, false, 96.0 / 256.0);
	TestEqual(TEXT("shadow width"), G.Width, 51.2, 1e-9);
	TestEqual(TEXT("shadow height"), G.Height, 19.2, 1e-9);
	TestEqual(TEXT("shadow on the feet"), G.OffsetY, 29.44, 1e-9);
	TestEqual(TEXT("shadow alpha"), G.Alpha, 0.7, 1e-9);
	// A flier: width x 0.55, alpha x 0.6, dropped another 0.18 of its size.
	const FShadow Fl = Shadow(64.0, true, 96.0 / 256.0);
	TestEqual(TEXT("flier shadow width"), Fl.Width, 28.16, 1e-9);
	TestEqual(TEXT("flier shadow drop"), Fl.OffsetY, 29.44 + 11.52, 1e-9);
	TestEqual(TEXT("flier shadow alpha"), Fl.Alpha, 0.42, 1e-9);

	// Player._apply_hop: halfway (p 0.5) the cat is at the top, 26 px up, stretched by 0.16 x 1 x 0.5 = 0.08.
	const FHop Top = Hop(HopSec * 0.5, true);
	TestEqual(TEXT("hop top lift"), Top.Lift, 26.0, 1e-9);
	TestEqual(TEXT("hop top narrower"), Top.ScaleX, 0.92, 1e-9);
	TestEqual(TEXT("hop top taller"), Top.ScaleY, 1.08, 1e-9);
	// The crouch at the start: p 0 -> stretch -0.16 (wider, shorter), no lift.
	const FHop Crouch = Hop(HopSec, true);
	TestEqual(TEXT("crouch: no lift"), Crouch.Lift, 0.0, 1e-9);
	TestEqual(TEXT("crouch: wider"), Crouch.ScaleX, 1.16, 1e-9);
	TestEqual(TEXT("crouch: shorter"), Crouch.ScaleY, 0.84, 1e-9);
	// The landing share: p 0.94 -> stretch -0.16 x (0.94 - 0.88) / 0.12 = -0.08.
	const FHop Land = Hop(HopSec * 0.06, true);
	TestEqual(TEXT("landing squash"), Land.ScaleY, 0.92, 1e-9);
	const FHop None = Hop(0.0, false);
	TestEqual(TEXT("no hop: no lift"), None.Lift, 0.0);
	TestEqual(TEXT("no hop: no stretch"), None.ScaleX, 1.0);

	// SpriteLiveliness: the pulse is 1 +- 0.03 at 1.4 Hz; at t = 0, phase 0.25 turns it is at its top.
	TestEqual(TEXT("pulse top"), PulseY(0.0, 0.25), 1.03, 1e-12);
	TestEqual(TEXT("pulse a period later"), PulseY(1.0 / PulseHz, 0.25), 1.03, 1e-9);
	TestEqual(TEXT("pulse bottom"), PulseY(0.0, 0.75), 0.97, 1e-12);
	// Facing: below 8 px/s nothing changes; moving right flips (the art faces left), moving left unflips.
	TestTrue(TEXT("slow drift keeps the facing"), Face(true, 7.9) && !Face(false, -7.9));
	TestTrue(TEXT("moving right flips"), Face(false, 8.0));
	TestFalse(TEXT("moving left faces left"), Face(true, -8.0));
	TestTrue(TEXT("a phase is in [0, 1)"), PhaseFor(123456789) >= 0.0 && PhaseFor(123456789) < 1.0);

	// The glow breathes between 0.65 and 1.0.
	TestEqual(TEXT("glow at its brightest"), GlowAlpha(0.25 / GlowHz, 0.0), 1.0, 1e-9);
	TestEqual(TEXT("glow at its faintest"), GlowAlpha(0.75 / GlowHz, 0.0), 0.65, 1e-9);

	// Godot's back ease-in: 0 at 0, 1 at 1, below zero early (it winds back first).
	TestEqual(TEXT("back ease 0"), BackEaseIn(0.0), 0.0, 1e-12);
	TestEqual(TEXT("back ease 1"), BackEaseIn(1.0), 1.0, 1e-12);
	TestTrue(TEXT("back ease winds back"), BackEaseIn(0.3) < 0.0);

	// The hat on a standing cat: (-0.10, -0.31) of 175 px from its centre, 0.62 of its width; mounted (0.045, -0.29), 0.36.
	const FOverlay Hat = CatOverlay(true, false, 1.0, 1.0);
	TestEqual(TEXT("hat x"), Hat.X, -17.5, 1e-9);
	TestEqual(TEXT("hat y"), Hat.Y, -54.25, 1e-9);
	TestEqual(TEXT("hat width"), Hat.Width, 108.5, 1e-9);
	const FOverlay Mounted = CatOverlay(true, true, 1.0, 1.0);
	TestEqual(TEXT("mounted hat x"), Mounted.X, 7.875, 1e-9);
	TestEqual(TEXT("mounted hat width"), Mounted.Width, 63.0, 1e-9);
	// Facing right, the hat and the weapon mirror with the cat, as one figure (F38, a Godot fault the port does not copy, D16:
	// Godot flipped only the cat's picture, so a cat steered right wore its hat behind its ears and held its weapon behind
	// its back).
	TestEqual(TEXT("facing left: the hat where it is drawn"), FacingOverlay(Hat, false).X, -17.5, 1e-9);
	TestEqual(TEXT("facing right: the hat mirrored onto the head"), FacingOverlay(Hat, true).X, 17.5, 1e-9);
	TestEqual(TEXT("facing right: the same height"), FacingOverlay(Hat, true).Y, Hat.Y, 1e-9);
	TestTrue(TEXT("the hat's drawing mirrors with the cat"), !HatFlipX(false) && HatFlipX(true));
	TestTrue(TEXT("the weapon (drawn leaning right) mirrored while the cat faces left, not right"), WeaponFlipX(false) && !WeaponFlipX(true));
	// The weapon (-0.38, 0.14), 0.405; it scales with the cat's breathing and hop.
	const FOverlay Weapon = CatOverlay(false, false, 1.0, 1.1);
	TestEqual(TEXT("weapon x"), Weapon.X, -66.5, 1e-9);
	TestEqual(TEXT("weapon y follows the cat's height"), Weapon.Y, 175.0 * 0.14 * 1.1, 1e-9);
	TestEqual(TEXT("weapon width"), Weapon.Width, 70.875, 1e-9);

	// A Godot modulate becomes a linear tint: white stays white, the shield's 0.6 alpha stays the opacity, 0.5 -> 0.5^2.2.
	TestEqual(TEXT("white"), Modulate(FLinearColor::White), FLinearColor::White);
	TestEqual(TEXT("shield alpha"), Modulate(ShieldTint).A, 0.6f);
	TestEqual(TEXT("half grey"), Modulate(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f)).R, FMath::Pow(0.5f, 2.2f), 1e-6f);
	TestTrue(TEXT("the golden kitten stays overbright"), Modulate(GoldenKittenTint).R > 1.0f);

	// Draw order: every z above every tree position of the z below.
	TestTrue(TEXT("z before the tree"), Order(ZKitten, 0.0) > Order(ZMice, TreeSpan - 1.0));
	TestTrue(TEXT("the cat over the kittens"), Order(ZCat, 0.0) > Order(ZKitten, 500.0));
	TestTrue(TEXT("the hat over the weapon over the cat"), Order(ZCat + 2, 0.0) > Order(ZCat + 1, 0.0)
		&& Order(ZCat + 1, 0.0) > Order(ZCat, 0.0));

	// The rig (R5): a swing is Amp sin, a lift Amp (1 - cos) / 2 (0 at rest, Amp at the half turn); the creature's own phase adds.
	TestEqual(TEXT("swing at rest"), RigAngleDeg(false, 14.0, 2.0, 0.0, 0.0, 0.0), 0.0, 1e-12);
	TestEqual(TEXT("swing a quarter turn on"), RigAngleDeg(false, 14.0, 2.0, 0.0, 0.0, 0.125), 14.0, 1e-9);
	TestEqual(TEXT("a pair half a turn apart"), RigAngleDeg(false, 14.0, 2.0, 0.5, 0.0, 0.125), -14.0, 1e-9);
	TestEqual(TEXT("lift at rest"), RigAngleDeg(true, -12.0, 0.8, 0.0, 0.0, 0.0), 0.0, 1e-12);
	TestEqual(TEXT("lift at the top"), RigAngleDeg(true, -12.0, 0.8, 0.0, 0.0, 0.625), -12.0, 1e-9);
	TestEqual(TEXT("the creature's phase"), RigAngleDeg(false, 14.0, 2.0, 0.0, 0.25, 0.0), 14.0, 1e-9);
	// A layer turns about its pivot: at 0 degrees it sits on the drawing; at any angle the pivot stays put. A 100 px drawing
	// at (500, 300), pivot (0.3, 0.8): the pivot is (-20, +30) from the centre, (480, 330).
	const FRigPose Rest = RigPose(500.0, 300.0, 100.0, 100.0, false, 0.3, 0.8, 0.0);
	TestTrue(TEXT("at rest the layer lies on the drawing"), FMath::IsNearlyEqual(Rest.X, 500.0, 1e-9)
		&& FMath::IsNearlyEqual(Rest.Y, 300.0, 1e-9) && Rest.Angle == 0.0);
	auto PivotOf = [](const FRigPose& P, double W, double H, double Ux, double Vy)
	{
		// the pivot's offset from the quad's centre, turned with the quad (clockwise on screen, y down)
		const double Ox = (Ux - 0.5) * W, Oy = (Vy - 0.5) * H;
		const double C = FMath::Cos(P.Angle), S = FMath::Sin(P.Angle);
		return FVector2D(P.X + Ox * C - Oy * S, P.Y + Ox * S + Oy * C);
	};
	const FRigPose Turned = RigPose(500.0, 300.0, 100.0, 100.0, false, 0.3, 0.8, 30.0);
	TestTrue(TEXT("the pivot stays put"), PivotOf(Turned, 100.0, 100.0, 0.3, 0.8).Equals(FVector2D(480.0, 330.0), 1e-9));
	TestEqual(TEXT("clockwise on screen as drawn"), Turned.Angle, FMath::DegreesToRadians(30.0), 1e-12);
	// Clockwise about a pivot below-left of the centre carries the centre right and down... (-20, +30) -> centre at
	// (480, 330) + R(30) (20, -30) = (480 + 17.32 + 15, 330 + 10 - 25.98).
	TestTrue(TEXT("the centre swings round the pivot"), FMath::IsNearlyEqual(Turned.X, 480.0 + 20.0 * FMath::Cos(UE_DOUBLE_PI / 6.0)
		+ 30.0 * FMath::Sin(UE_DOUBLE_PI / 6.0), 1e-9) && FMath::IsNearlyEqual(Turned.Y, 330.0 + 20.0 * FMath::Sin(UE_DOUBLE_PI / 6.0)
		- 30.0 * FMath::Cos(UE_DOUBLE_PI / 6.0), 1e-9));
	// Mirrored: the pivot is mirrored ((+20, +30): 520, 330) and the turn runs the other way.
	const FRigPose Flipped = RigPose(500.0, 300.0, 100.0, 100.0, true, 0.3, 0.8, 30.0);
	TestEqual(TEXT("mirrored, the turn reverses"), Flipped.Angle, -FMath::DegreesToRadians(30.0), 1e-12);
	TestTrue(TEXT("mirrored, the mirrored pivot stays put"), PivotOf(Flipped, 100.0, 100.0, 0.7, 0.8).Equals(FVector2D(520.0, 330.0), 1e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLookAtlasIndexTest, "Pawpocalypse.Realm.Look.AtlasIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLookAtlasIndexTest::RunTest(const FString& Parameters)
{
	// Review CR16 (2026-11-10): the atlas's lookup index counted as built while it held one entry per cell, so two cells with
	// one id (or two ids FString's hash takes as equal) rebuilt it, and the rigs, on every lookup: dozens of times a frame.
	UPawSpriteAtlas* Twice = NewObject<UPawSpriteAtlas>(GetTransientPackage());
	FPawAtlasCell Cell;
	Cell.Id = TEXT("twice");
	Twice->Cells.Add(Cell);
	Twice->Cells.Add(Cell);
	const int32 Builds = Twice->GetIndexBuilds();
	Twice->Find(TEXT("twice"));
	Twice->Find(TEXT("twice"));
	Twice->Rig(0);
	TestEqual(TEXT("three lookups build the index once"), Twice->GetIndexBuilds() - Builds, 1);
	// The shipped atlas has every id once (tools/import_art.py writes pack_atlas.py's manifest).
	const UPawSpriteAtlas* Atlas = LoadObject<UPawSpriteAtlas>(nullptr, TEXT("/Game/Pawpocalypse/Art/Field/DA_FieldAtlas.DA_FieldAtlas"));
	if (TestNotNull(TEXT("DA_FieldAtlas"), Atlas))
	{
		TSet<FString> Ids;
		TArray<FString> Twins;
		for (const FPawAtlasCell& C : Atlas->Cells)
		{
			bool bSeen = false;
			Ids.Add(C.Id, &bSeen);
			if (bSeen)
			{
				Twins.Add(C.Id);
			}
		}
		TestEqual(FString::Printf(TEXT("the field atlas: every id once (%s)"), *FString::Join(Twins, TEXT(", "))), Twins.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawLookAtlasTest, "Pawpocalypse.Realm.Look.Atlas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawLookAtlasTest::RunTest(const FString& Parameters)
{
	const UPawSpriteAtlas* Atlas = LoadObject<UPawSpriteAtlas>(nullptr, TEXT("/Game/Pawpocalypse/Art/Field/DA_FieldAtlas.DA_FieldAtlas"));
	if (!TestNotNull(TEXT("DA_FieldAtlas (py tools/import_art.py --run atlas)"), Atlas))
	{
		return false;
	}
	TestTrue(TEXT("the atlas has pages"), Atlas->Pages.Num() > 0);
	for (const TObjectPtr<UTexture2D>& Page : Atlas->Pages)
	{
		TestNotNull(TEXT("every page is a texture"), Page.Get());
	}
	for (const FPawAtlasCell& C : Atlas->Cells)
	{
		const bool bInside = C.U0 >= 0.0f && C.V0 >= 0.0f && C.U1 <= 1.0f && C.V1 <= 1.0f && C.U0 < C.U1 && C.V0 < C.V1;
		TestTrue(FString::Printf(TEXT("%s: a cell inside its page"), *C.Id), bInside && Atlas->Pages.IsValidIndex(C.Page));
		TestTrue(FString::Printf(TEXT("%s: its native size"), *C.Id), C.NativeWidth > 0.0f && C.NativeHeight > 0.0f);
	}
	TestEqual(TEXT("found by stem"), Atlas->Find(TEXT("ground_shadow")) != INDEX_NONE, true);
	TestEqual(TEXT("found by a Godot path"), Atlas->Find(TEXT("res://assets/art/player/kitten_basic.svg")),
		Atlas->Find(TEXT("kitten_basic")));
	TestEqual(TEXT("an unknown id"), Atlas->Find(TEXT("enemy_no_such_thing")), INDEX_NONE);
	const int32 Boss = Atlas->Find(TEXT("boss_generic"));
	TestTrue(TEXT("a boss is its SVG's 256 px"), Boss != INDEX_NONE && Atlas->Cell(Boss).NativeWidth == 256.0f);

	// The imported asset is the packer's latest manifest (a repack without the import would leave stale UVs): every row of
	// SourceArt/png/atlas/field.csv, when the pipeline has run on this machine, is a cell with the same page and UVs.
	const FString Manifest = FPaths::ProjectDir() / TEXT("SourceArt/png/atlas/field.csv");
	TArray<FString> Lines;
	if (FFileHelper::LoadFileToStringArray(Lines, *Manifest) && Lines.Num() > 1)
	{
		TestEqual(TEXT("as many cells as the manifest's rows"), Atlas->Cells.Num(), Lines.Num() - 1);
		for (int32 L = 1; L < Lines.Num(); ++L)
		{
			TArray<FString> Col;
			Lines[L].ParseIntoArray(Col, TEXT(","), false);
			const int32 I = Col.Num() >= 12 ? Atlas->Find(Col[0]) : INDEX_NONE;
			const bool bSame = I != INDEX_NONE && Atlas->Cell(I).Page == FCString::Atoi(*Col[1])
				&& FMath::IsNearlyEqual(Atlas->Cell(I).U0, FCString::Atof(*Col[6]), 1e-6f)
				&& FMath::IsNearlyEqual(Atlas->Cell(I).V0, FCString::Atof(*Col[7]), 1e-6f)
				&& FMath::IsNearlyEqual(Atlas->Cell(I).U1, FCString::Atof(*Col[8]), 1e-6f)
				&& FMath::IsNearlyEqual(Atlas->Cell(I).V1, FCString::Atof(*Col[9]), 1e-6f);
			TestTrue(FString::Printf(TEXT("%s is imported as packed (py tools/import_art.py --run atlas)"), Col.Num() ? *Col[0] : TEXT("?")), bSame);
			// The rig columns (pack_atlas.py RIG_COLUMNS: rig_of, rig_order, pivot_u, pivot_v, motion, amp, hz, phase).
			if (I != INDEX_NONE && Col.Num() >= 20)
			{
				const FPawAtlasCell& C = Atlas->Cell(I);
				const bool bRig = C.RigOf == Col[12] && (Col[12].IsEmpty() || (C.RigOrder == FCString::Atoi(*Col[13])
					&& C.Motion == Col[16] && (Col[16].IsEmpty() || (FMath::IsNearlyEqual(C.PivotU, FCString::Atof(*Col[14]), 1e-5f)
					&& FMath::IsNearlyEqual(C.PivotV, FCString::Atof(*Col[15]), 1e-5f)
					&& FMath::IsNearlyEqual(C.Amplitude, FCString::Atof(*Col[17]), 1e-4f)
					&& FMath::IsNearlyEqual(C.Hz, FCString::Atof(*Col[18]), 1e-4f)
					&& FMath::IsNearlyEqual(C.Phase, FCString::Atof(*Col[19]), 1e-4f)))));
				TestTrue(FString::Printf(TEXT("%s: its rig is imported as packed"), *Col[0]), bRig);
			}
		}
	}
	else
	{
		AddInfo(TEXT("no SourceArt/png/atlas/field.csv here (py tools/pack_atlas.py): the import's freshness is not checked"));
	}

	// Every rig is whole: one body layer, every layer of the drawing's own size, pivots inside the square, a known motion.
	int32 Rigged = 0;
	for (int32 I = 0; I < Atlas->Cells.Num(); ++I)
	{
		const TArray<FPawRigLayer>* Rig = Atlas->Rig(I);
		if (!Rig)
		{
			continue;
		}
		++Rigged;
		const FPawAtlasCell& D = Atlas->Cell(I);
		int32 Bodies = 0;
		for (const FPawRigLayer& L : *Rig)
		{
			const FPawAtlasCell& C = Atlas->Cell(L.Cell);
			Bodies += L.IsBody() ? 1 : 0;
			TestTrue(FString::Printf(TEXT("%s: the size of %s"), *C.Id, *D.Id), C.NativeWidth == D.NativeWidth
				&& FMath::IsNearlyEqual(C.U1 - C.U0, D.U1 - D.U0, 1e-6f));
			TestTrue(FString::Printf(TEXT("%s: a pivot in the square and a motion"), *C.Id), L.IsBody()
				|| (L.PivotU > 0.0 && L.PivotU < 1.0 && L.PivotV > 0.0 && L.PivotV < 1.0 && (C.Motion == TEXT("swing")
				|| C.Motion == TEXT("lift")) && L.Hz > 0.0 && L.AmplitudeDeg != 0.0));
		}
		TestEqual(FString::Printf(TEXT("%s: one body layer"), *D.Id), Bodies, 1);
		// The view spaces a creature's layers 0.12 of a draw place apart and keeps them inside +-0.36 (PawRealmView).
		TestTrue(FString::Printf(TEXT("%s: at most three back and three front layers"), *D.Id),
			(*Rig)[0].Order >= -3 && Rig->Last().Order <= 3);
		TestTrue(FString::Printf(TEXT("%s: a rig moves something"), *D.Id), Rig->Num() > 1);
	}
	// Masked sprites clip in the colour pass too (DefaultEngine.ini): with the engine default (1) an overlapping layer at
	// the same depth painted its transparent texels black over another (the R5 rig, 2026-11-09).
	const IConsoleVariable* MaskOnlyInEarlyZ = IConsoleManager::Get().FindConsoleVariable(TEXT("r.EarlyZPassOnlyMaterialMasking"));
	TestTrue(TEXT("r.EarlyZPassOnlyMaterialMasking is off"), MaskOnlyInEarlyZ && MaskOnlyInEarlyZ->GetInt() == 0);
	TestTrue(TEXT("the anatomy pass's drawings are rigged (R5)"), Rigged > 0 && Atlas->Rig(Atlas->Find(TEXT("enemy_rooftop_seagull"))));

	// Everything the data can put on the field.
	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	GI->InitializeStandalone();
	const UPawGameSubsystem* Game = GI->GetSubsystem<UPawGameSubsystem>();
	if (TestTrue(TEXT("the game data is ready"), Game && Game->IsDataReady()))
	{
		const Pc::PcGameData& D = Game->Data();
		auto Need = [&](const std::string& Path, const TCHAR* What)
		{
			TestTrue(FString::Printf(TEXT("%s %hs is in the atlas"), What, Path.c_str()), Atlas->Find(UTF8_TO_TCHAR(Path.c_str())) != INDEX_NONE);
		};
		for (const Pc::PcEnemy& E : D.Enemies)
		{
			Need(E.TexturePath, TEXT("enemy"));
		}
		for (const Pc::PcBoss& B : D.Bosses)
		{
			Need(B.TexturePath, TEXT("boss"));
		}
		for (const Pc::PcEvolution& E : D.Evolutions)
		{
			Need(E.TexturePath, TEXT("stage"));
			// Stage 1 is the bare paw (player.gd: no weapon on purpose); every other stage holds one.
			if (E.Stage > 1)
			{
				Need("player_weapon_stage_" + std::to_string(E.Stage), TEXT("weapon"));
			}
		}
		for (const Pc::PcJob& J : D.Jobs)
		{
			if (!J.HatPath.empty())
			{
				Need(J.HatPath, TEXT("hat"));
			}
		}
		Need("kitten_basic", TEXT("the kitten"));
		// Every prop the backdrop can draw (biome_backdrop.gd's tables, PcBackdrop): each biome and the title's garden, at the
		// narrowest and widest views, on high (every border row) and low.
		std::vector<std::string> Ids = { "menu" };
		for (const Pc::PcTier& T : D.Tiers)
		{
			Ids.push_back(T.Id);
		}
		for (const std::string& Id : Ids)
		{
			for (const double W : { 1280.0, 3840.0 })
			{
				for (const Pc::PcBackdropProp& P : Pc::BuildBackdrop(Id, W, 1080.0, 1.0).Props)
				{
					Need(P.Stem, TEXT("prop"));
				}
			}
		}
		Need("boss_add", TEXT("the add"));
	}
	UWorld* World = GI->GetWorld();
	GI->Shutdown();
	if (World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
	return true;
}

#endif
