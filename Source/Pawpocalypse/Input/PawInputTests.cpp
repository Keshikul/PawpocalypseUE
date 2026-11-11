// Pawpocalypse.Input.*: Godot's input map as the port holds it (docs/05 §3).
//
// Pawpocalypse.Input.Map       the generated assets ARE Data/InputMap.csv (Godot's project.godot [input]): one IA_<Action> per
//                              Godot action of the right value type, every event one mapping of IMC_Pawpocalypse with its key and
//                              (for a stick or trigger) its half-axis modifier, nothing else; the Y axes flipped; every action on a
//                              keyboard or mouse AND a pad but the documented one-device list (adding to it is a design decision).
// Pawpocalypse.Input.HalfAxis  the modifiers IN the generated context give what Godot's own Input gives for 5,726 stick and
//                              trigger values (fixture `input_axis`): pressed exactly, strength exactly.
// Pawpocalypse.Input.Keys      what a key IS (PawInput::ActionsOfKey) equals the map, key for key, a stick or trigger's digital
//                              stand-in counting as its axis half.
// Pawpocalypse.Input.Navigation the menus' navigation (FPawNavigationConfig) is Godot's ui_* map: W / A / S / D beside the arrows
//                              and the D-pad, keypad Enter, the left stick at the ui_* dead zone; nothing else.
// Pawpocalypse.Input.Vector    Godot's Input.get_vector on the steer stick, 3,008 measured deflections (fixture `input_axis`).
// Pawpocalypse.Input.Screens   every key bound to a screen's actions does that action through the screen's own Slate widget
//                              (HUD, pause menu, settings, title, skill tree), and a key bound to none of them does nothing.
// Pawpocalypse.Input.Pounce    a click over the dock is the dock's, a trigger pounces anywhere.
// Pawpocalypse.Input.Velocity  player.gd stick_velocity on 2,000 measured deflections (fixture `input_device`).
// Pawpocalypse.Input.Device    the device subsystem on real Slate events (keys, pad buttons, sticks, the mouse, the warp echo,
//                              the prompt override and Swap sticks from Settings, connections and their toasts, the press
//                              guard, the hidden pointer): the rules are PcDevice's (L1, against input_device.gd); this is the
//                              translation from Slate's events into them.
// Pawpocalypse.Input.PointerOverField  the pointer hides over the open field, not over the dock or outside the playfield.
//
// Negative tests: tools/sabotage_realm.py (`input_*`).

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/PawAudioSubsystem.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Input/PawHalfAxis.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Input/PawInputMap.h"
#include "Input/PawNavigationConfig.h"
#include "Realm/PawRealmSubsystem.h"
#include "Realm/PawRealmView.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawHudScreen.h"
#include "UI/PawMainMenu.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawSkillTreeScreen.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/PawTextTesting.h"

#include <cstdlib>

namespace
{
	// Actions on one device only, by design (docs/05 §3; input_bindings.gd LOCKED / the Controls page): steering and re-centring
	// are the pad's (the mouse IS the keyboard player's cursor); the number keys and U are keyboard shortcuts.
	const TSet<FString> PadOnly = { TEXT("steer_left"), TEXT("steer_right"), TEXT("steer_up"), TEXT("steer_down"), TEXT("recentre") };
	const TSet<FString> KeyboardOnly = { TEXT("tab_1"), TEXT("tab_2"), TEXT("tab_3"), TEXT("tab_4"), TEXT("tab_5"), TEXT("tab_6"),
		TEXT("open_upgrades") };

	const FEnhancedActionKeyMapping* FindMapping(const UInputMappingContext* IMC, const UInputAction* Action, const FKey& Key)
	{
		for (const FEnhancedActionKeyMapping& M : IMC->GetMappings())
		{
			if (M.Action == Action && M.Key == Key)
			{
				return &M;
			}
		}
		return nullptr;
	}

	const UPawHalfAxis* HalfOf(const FEnhancedActionKeyMapping& M)
	{
		return M.Modifiers.Num() == 1 ? Cast<UPawHalfAxis>(M.Modifiers[0]) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputMapTest, "Pawpocalypse.Input.Map", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputMapTest::RunTest(const FString& Parameters)
{
	TArray<FPawInputMapRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("Data/InputMap.csv reads: ") + Error, PawInput::ReadInputMap(PawInput::InputMapCsv(), Rows, Error)))
	{
		return false;
	}
	UInputMappingContext* IMC = PawInput::LoadContext();
	if (!TestNotNull(TEXT("IMC_Pawpocalypse exists (py tools/create_input_assets.py --run)"), IMC))
	{
		return false;
	}
	TSet<FString> Actions;
	int32 Expected = 0;
	TMap<FString, TSet<FString>> Devices;
	for (const FPawInputMapRow& R : Rows)
	{
		Actions.Add(R.Action);
		Devices.FindOrAdd(R.Action).Add(R.Device == TEXT("pad") ? TEXT("pad") : TEXT("keyboard_mouse"));
		UInputAction* IA = PawInput::LoadAction(R.Action);
		if (!TestNotNull(FString::Printf(TEXT("%s exists"), *PawInput::ActionAssetName(R.Action)), IA))
		{
			continue;
		}
		const FKey Key(R.UeKey);
		TestTrue(FString::Printf(TEXT("%s: %s is a key"), *R.Id, *R.UeKey.ToString()), Key.IsValid());
		// Godot's axes point the other way on Y: up is -1 there, +1 here.
		if (R.GodotEvent == TEXT("pad_axis"))
		{
			const bool bY = R.UeKey == FName(TEXT("Gamepad_LeftY")) || R.UeKey == FName(TEXT("Gamepad_RightY"));
			TestEqual(FString::Printf(TEXT("%s: the sign on Unreal's axis"), *R.Id), R.UeAxisSign, bY ? -R.GodotAxisSign : R.GodotAxisSign);
		}
		const FEnhancedActionKeyMapping* M = FindMapping(IMC, IA, Key);
		if (!TestNotNull(FString::Printf(TEXT("%s: %s -> %s is mapped"), *R.Id, *R.UeKey.ToString(), *IA->GetName()), M))
		{
			continue;
		}
		if (R.bMerged)
		{
			continue; // the same key as another row of the action: that row's mapping
		}
		++Expected;
		if (R.GodotEvent == TEXT("pad_axis"))
		{
			const UPawHalfAxis* H = HalfOf(*M);
			if (TestNotNull(FString::Printf(TEXT("%s: one half-axis modifier"), *R.Id), H))
			{
				TestEqual(FString::Printf(TEXT("%s: its half"), *R.Id), H->Sign, static_cast<float>(R.UeAxisSign));
				TestEqual(FString::Printf(TEXT("%s: Godot's dead zone"), *R.Id), H->DeadZone, R.DeadZone);
			}
		}
		else
		{
			TestEqual(FString::Printf(TEXT("%s: a key or button has no modifier"), *R.Id), M->Modifiers.Num(), 0);
		}
	}
	TestEqual(TEXT("nothing mapped that Godot does not bind"), IMC->GetMappings().Num(), Expected);
	for (const FString& A : Actions)
	{
		const UInputAction* IA = PawInput::LoadAction(A);
		if (!IA)
		{
			continue;
		}
		const EInputActionValueType Want = PawInput::IsAxisAction(Rows, A) ? EInputActionValueType::Axis1D : EInputActionValueType::Boolean;
		TestEqual(FString::Printf(TEXT("%s: its value type"), *IA->GetName()), static_cast<int32>(IA->ValueType), static_cast<int32>(Want));
		const TSet<FString>& D = Devices[A];
		const bool bBoth = D.Contains(TEXT("pad")) && D.Contains(TEXT("keyboard_mouse"));
		if (PadOnly.Contains(A))
		{
			TestTrue(FString::Printf(TEXT("%s is the pad's alone (by design)"), *A), D.Num() == 1 && D.Contains(TEXT("pad")));
		}
		else if (KeyboardOnly.Contains(A))
		{
			TestTrue(FString::Printf(TEXT("%s is the keyboard's alone (by design)"), *A), D.Num() == 1 && D.Contains(TEXT("keyboard_mouse")));
		}
		else
		{
			TestTrue(FString::Printf(TEXT("%s has a keyboard or mouse binding AND a pad binding"), *A), bBoth);
		}
	}
	TestEqual(TEXT("Godot's 32 actions"), Actions.Num(), 32);
	AddInfo(FString::Printf(TEXT("%d actions, %d events, %d mappings"), Actions.Num(), Rows.Num(), IMC->GetMappings().Num()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputHalfAxisTest, "Pawpocalypse.Input.HalfAxis", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputHalfAxisTest::RunTest(const FString& Parameters)
{
	FString Text;
	const FString Path = FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/input_axis_godot.tsv");
	if (!TestTrue(TEXT("the fixture"), FFileHelper::LoadFileToString(Text, *Path)))
	{
		return false;
	}
	TArray<FPawInputMapRow> Rows;
	FString Error;
	UInputMappingContext* IMC = PawInput::LoadContext();
	if (!TestTrue(TEXT("the map reads"), PawInput::ReadInputMap(PawInput::InputMapCsv(), Rows, Error)) || !TestNotNull(TEXT("the context"), IMC))
	{
		return false;
	}
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	TArray<FString> Header;
	int32 Checked = 0, Mismatch = 0, PressedRows = 0, EdgeRows = 0;
	for (const FString& Line : Lines)
	{
		if (Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		TArray<FString> C;
		Line.ParseIntoArray(C, TEXT("\t"), false);
		if (Header.Num() == 0)
		{
			Header = C;
			continue;
		}
		TMap<FString, FString> V;
		for (int32 I = 0; I < Header.Num() && I < C.Num(); ++I)
		{
			V.Add(Header[I], C[I]);
		}
		const FString Action = V[TEXT("action")];
		const int64 Axis = FCString::Atoi64(*V[TEXT("axis")]);
		const int32 Bound = FCString::Atoi(*V[TEXT("bound_sign")]);
		const FPawInputMapRow* R = Rows.FindByPredicate([&](const FPawInputMapRow& X)
		{
			return X.Action == Action && X.GodotEvent == TEXT("pad_axis") && X.GodotCode == Axis && X.GodotAxisSign == Bound;
		});
		if (!TestNotNull(FString::Printf(TEXT("%s axis %lld is in the map"), *Action, Axis), R))
		{
			return false;
		}
		const FEnhancedActionKeyMapping* M = FindMapping(IMC, PawInput::LoadAction(Action), FKey(R->UeKey));
		const UPawHalfAxis* H = M ? HalfOf(*M) : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s: its modifier in the context"), *R->Id), H))
		{
			return false;
		}
		// The same physical stick position on Unreal's axis (Y flipped), through the asset's own modifier.
		const double Godot = std::strtod(TCHAR_TO_UTF8(*V[TEXT("value")]), nullptr);
		const float Flip = static_cast<float>(R->UeAxisSign * R->GodotAxisSign);
		const float Ue = static_cast<float>(Godot) * Flip;
		const FInputActionValue Out = H->ModifyRaw(nullptr, FInputActionValue(EInputActionValueType::Axis1D, FVector(Ue, 0.0, 0.0)), 0.0f);
		const float Strength = Out.Get<float>();
		const bool bPressedHere = Out.IsNonZero(); // what Enhanced Input calls actuated
		const bool bPressedGodot = V[TEXT("pressed")] == TEXT("1");
		const float StrengthGodot = static_cast<float>(std::strtod(TCHAR_TO_UTF8(*V[TEXT("strength")]), nullptr));
		// Pressed with a strength under Enhanced Input's actuation tolerance: it reads the floor (still pressed).
		const float StrengthHere = (bPressedGodot && StrengthGodot < UPawHalfAxis::PressedFloor && Strength == UPawHalfAxis::PressedFloor)
			? StrengthGodot : Strength;
		if ((bPressedHere != bPressedGodot || StrengthHere != StrengthGodot) && ++Mismatch <= 8)
		{
			AddError(FString::Printf(TEXT("%s value %.9g (Unreal %.9g): pressed %d strength %.9g vs Godot %d %.9g"), *Action, Godot, Ue,
				bPressedHere, StrengthHere, bPressedGodot, StrengthGodot));
		}
		++Checked;
		PressedRows += bPressedGodot ? 1 : 0;
		EdgeRows += (bPressedGodot && StrengthGodot < UPawHalfAxis::PressedFloor) ? 1 : 0;
	}
	TestEqual(TEXT("every value as Godot's"), Mismatch, 0);
	TestEqual(TEXT("rows"), Checked, 5726);
	PawInput::ResetBindings();
	TestTrue(TEXT("both pressed and released values"), PressedRows > 1000 && PressedRows < Checked - 1000);
	TestTrue(TEXT("the dead zone's edge (pressed, strength ~0)"), EdgeRows >= 14);
	AddInfo(FString::Printf(TEXT("%d values, %d pressed, %d on the edge"), Checked, PressedRows, EdgeRows));
	return true;
}

namespace
{
	TMap<FKey, TSet<FString>> ExpectedActions(const TArray<FPawInputMapRow>& Rows)
	{
		TMap<FKey, TSet<FString>> Out;
		for (const FPawInputMapRow& R : Rows)
		{
			const FKey Key(R.UeKey);
			Out.FindOrAdd(Key).Add(R.Action);
			FKey Digital;
			if (R.GodotEvent == TEXT("pad_axis") && PawInput::DigitalOfHalf(Key, R.UeAxisSign, Digital))
			{
				Out.FindOrAdd(Digital).Add(R.Action);
			}
		}
		return Out;
	}

	FKeyEvent KeyDown(const FKey& Key, bool bRepeat = false)
	{
		return FKeyEvent(Key, FModifierKeysState(), 0, bRepeat, 0, 0);
	}

	// A key through a widget's own Slate widget: the path Slate takes (OnPreviewKeyDown -> NativeOnPreviewKeyDown).
	bool SendKey(UUserWidget* W, const FKey& Key)
	{
		return W->TakeWidget()->OnPreviewKeyDown(FGeometry(), KeyDown(Key)).IsEventHandled();
	}

	void SendKeyUp(UUserWidget* W, const FKey& Key)
	{
		W->TakeWidget()->OnKeyUp(FGeometry(), KeyDown(Key));
	}

	void SendAnalog(UUserWidget* W, const FKey& Key, float Value)
	{
		W->TakeWidget()->OnAnalogValueChanged(FGeometry(), FAnalogInputEvent(Key, FModifierKeysState(), 0, false, 0, 0, Value));
	}

	// Keys no screen action is bound to: they must do nothing anywhere.
	const FKey Unbound[] = { EKeys::F7, EKeys::K, EKeys::Gamepad_LeftThumbstick };

	struct FInputWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawSettingsSubsystem* Settings = nullptr;
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
			Settings = GI->GetSubsystem<UPawSettingsSubsystem>();
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
			GFrameCounter += 2;
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

		// Clear of the pause menu's "not the frame after" guard, as a player's next press is.
		void NextFrames()
		{
			Hud->Pump(1.0 / 60.0);
			GFrameCounter += 2;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputKeysTest, "Pawpocalypse.Input.Keys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputKeysTest::RunTest(const FString& Parameters)
{
	TArray<FPawInputMapRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("the map reads"), PawInput::ReadInputMap(PawInput::InputMapCsv(), Rows, Error)))
	{
		return false;
	}
	PawInput::ResetBindings();
	const TMap<FKey, TSet<FString>> Want = ExpectedActions(Rows);
	for (const TPair<FKey, TSet<FString>>& W : Want)
	{
		const TSet<FString> Got(PawInput::ActionsOfKey(W.Key));
		TestTrue(FString::Printf(TEXT("%s is exactly its actions (%s)"), *W.Key.ToString(), *FString::Join(W.Value.Array(), TEXT(" "))),
			Got.Num() == W.Value.Num() && Got.Includes(W.Value));
	}
	for (const FKey& K : Unbound)
	{
		TestEqual(FString::Printf(TEXT("%s is no action"), *K.ToString()), PawInput::ActionsOfKey(K).Num(), 0);
	}
	// What Godot's own Esc, B and Start are: back AND pause (and the menus' cancel), back alone, pause alone.
	TestTrue(TEXT("Esc: back, pause, ui_cancel"), TSet<FString>(PawInput::ActionsOfKey(EKeys::Escape)).Num() == 3
		&& PawInput::KeyIs(EKeys::Escape, TEXT("back")) && PawInput::KeyIs(EKeys::Escape, TEXT("pause")));
	TestTrue(TEXT("B: back, not pause"), PawInput::KeyIs(EKeys::Gamepad_FaceButton_Right, TEXT("back"))
		&& !PawInput::KeyIs(EKeys::Gamepad_FaceButton_Right, TEXT("pause")));
	TestTrue(TEXT("Start: pause, not back"), PawInput::KeyIs(EKeys::Gamepad_Special_Right, TEXT("pause"))
		&& !PawInput::KeyIs(EKeys::Gamepad_Special_Right, TEXT("back")));
	TestTrue(TEXT("the right trigger's press is a pounce and a zoom"), PawInput::KeyIs(EKeys::Gamepad_RightTrigger, TEXT("pounce"))
		&& PawInput::KeyIs(EKeys::Gamepad_RightTrigger, TEXT("zoom_in")));
	TestTrue(TEXT("keypad Enter is Enter: ui_accept"), PawInput::KeyIs(EKeys::Enter, TEXT("ui_accept")));
	TestFalse(TEXT("a stand-in is left out when asked"), PawInput::KeysOf(TEXT("menu_up"), false).Contains(EKeys::Gamepad_LeftStick_Up));
	TestTrue(TEXT("and in otherwise"), PawInput::KeysOf(TEXT("menu_up")).Contains(EKeys::Gamepad_LeftStick_Up));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputNavigationTest, "Pawpocalypse.Input.Navigation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputNavigationTest::RunTest(const FString& Parameters)
{
	TArray<FPawInputMapRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("the map reads"), PawInput::ReadInputMap(PawInput::InputMapCsv(), Rows, Error)))
	{
		return false;
	}
	PawInput::ResetBindings();
	const FPawNavigationConfig Nav;
	TMap<FString, EUINavigation> Dir = { { TEXT("ui_up"), EUINavigation::Up }, { TEXT("ui_down"), EUINavigation::Down },
		{ TEXT("ui_left"), EUINavigation::Left }, { TEXT("ui_right"), EUINavigation::Right } };
	TSet<FKey> Navigating, Acting;
	for (const FPawInputMapRow& R : Rows)
	{
		const FKey Key(R.UeKey);
		if (const EUINavigation* D = Dir.Find(R.Action))
		{
			if (R.GodotEvent == TEXT("pad_axis"))
			{
				// The left stick: Slate's analog navigation, at the ui_* dead zone.
				TestTrue(FString::Printf(TEXT("%s: the stick navigates as analog"), *R.Id),
					(R.UeKey == FName(TEXT("Gamepad_LeftX")) && Nav.AnalogX() == Key) || (R.UeKey == FName(TEXT("Gamepad_LeftY")) && Nav.AnalogY() == Key));
				TestEqual(FString::Printf(TEXT("%s: at its dead zone"), *R.Id), Nav.AnalogThreshold(), R.DeadZone);
				continue;
			}
			TestEqual(FString::Printf(TEXT("%s: %s moves the selection"), *R.Id, *Key.ToString()), static_cast<int32>(Nav.DirectionOf(Key)),
				static_cast<int32>(*D));
			Navigating.Add(Key);
		}
		else if (R.Action == TEXT("ui_accept") || R.Action == TEXT("ui_cancel"))
		{
			const EUINavigationAction Want = R.Action == TEXT("ui_accept") ? EUINavigationAction::Accept : EUINavigationAction::Back;
			TestEqual(FString::Printf(TEXT("%s: %s"), *R.Id, *Key.ToString()), static_cast<int32>(Nav.ActionOf(Key)), static_cast<int32>(Want));
			Acting.Add(Key);
		}
	}
	// Godot's, beside Slate's arrows and D-pad.
	TestEqual(TEXT("W is up"), static_cast<int32>(Nav.DirectionOf(EKeys::W)), static_cast<int32>(EUINavigation::Up));
	TestEqual(TEXT("A is left"), static_cast<int32>(Nav.DirectionOf(EKeys::A)), static_cast<int32>(EUINavigation::Left));
	TestEqual(TEXT("S is down"), static_cast<int32>(Nav.DirectionOf(EKeys::S)), static_cast<int32>(EUINavigation::Down));
	TestEqual(TEXT("D is right"), static_cast<int32>(Nav.DirectionOf(EKeys::D)), static_cast<int32>(EUINavigation::Right));
	// Nothing else: a stand-in would move twice, Q is the buy amount, Tab is Slate's own (Godot's ui_focus_next).
	for (const FKey& K : { EKeys::Gamepad_LeftStick_Up, EKeys::Gamepad_LeftStick_Left, EKeys::Q, EKeys::T, EKeys::Gamepad_RightShoulder })
	{
		TestEqual(FString::Printf(TEXT("%s moves nothing"), *K.ToString()), static_cast<int32>(Nav.DirectionOf(K)), static_cast<int32>(EUINavigation::Invalid));
	}
	for (const FKey& K : { EKeys::Q, EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_Special_Right })
	{
		TestEqual(FString::Printf(TEXT("%s neither accepts nor backs out"), *K.ToString()), static_cast<int32>(Nav.ActionOf(K)),
			static_cast<int32>(EUINavigationAction::Invalid));
	}
	TestTrue(TEXT("Tab still moves the selection"), Nav.bTabNavigation);
	AddInfo(FString::Printf(TEXT("%d navigation keys, %d accept / back keys"), Navigating.Num(), Acting.Num()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputVectorTest, "Pawpocalypse.Input.Vector", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputVectorTest::RunTest(const FString& Parameters)
{
	FString Text;
	if (!TestTrue(TEXT("the fixture"), FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/input_vector_godot.tsv")))))
	{
		return false;
	}
	PawInput::ResetBindings();
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	int32 Rows = 0, Mismatch = 0, Zero = 0, Clamped = 0;
	bool bHeader = true;
	for (const FString& Line : Lines)
	{
		if (Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		if (bHeader)
		{
			bHeader = false;
			continue;
		}
		TArray<FString> C;
		Line.ParseIntoArray(C, TEXT("\t"), false);
		auto F = [&C](int32 I) { return static_cast<float>(std::strtod(TCHAR_TO_UTF8(*C[I]), nullptr)); };
		// Godot's axes (+Y down) to Unreal's (+Y up), through the map's steer keys.
		TMap<FKey, float> Axes;
		Axes.Add(EKeys::Gamepad_RightX, F(0));
		Axes.Add(EKeys::Gamepad_RightY, -F(1));
		const FVector2f V = PawInput::SteerVector(Axes);
		if ((V.X != F(2) || V.Y != F(3)) && ++Mismatch <= 8)
		{
			AddError(FString::Printf(TEXT("stick (%.9g, %.9g): (%.9g, %.9g) vs Godot (%.9g, %.9g)"), F(0), F(1), V.X, V.Y, F(2), F(3)));
		}
		++Rows;
		Zero += (F(2) == 0.0f && F(3) == 0.0f) ? 1 : 0;
		Clamped += FMath::Abs(FMath::Sqrt(F(0) * F(0) + F(1) * F(1))) > 1.0f ? 1 : 0;
	}
	TestEqual(TEXT("every vector as Godot's"), Mismatch, 0);
	TestEqual(TEXT("rows"), Rows, 3008);
	TestTrue(TEXT("inside the dead zone"), Zero > 300);
	TestTrue(TEXT("beyond the rim"), Clamped > 300);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputScreensTest, "Pawpocalypse.Input.Screens", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputScreensTest::RunTest(const FString& Parameters)
{
	FInputWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	PawInput::ResetBindings();
	UPawHudScreen* Hud = W.Hud;
	using Tab = Pc::PcDockTab;

	// --- the HUD: tabs, the buy amount, open_upgrades, the tree, pause ---
	for (const FKey& K : PawInput::KeysOf(TEXT("tab_next")))
	{
		Hud->SelectTab(Tab::Upgrades);
		const bool bHandled = SendKey(Hud, K);
		TestTrue(FString::Printf(TEXT("tab_next %s moves on a tab"), *K.ToString()), bHandled && Hud->GetActiveTab() != Tab::Upgrades);
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("tab_prev")))
	{
		Hud->SelectTab(Tab::Upgrades);
		SendKey(Hud, K);
		TestTrue(FString::Printf(TEXT("tab_prev %s wraps back a tab"), *K.ToString()), Hud->GetActiveTab() != Tab::Upgrades);
	}
	for (int32 I = 0; I < 6; ++I)
	{
		for (const FKey& K : PawInput::KeysOf(*FString::Printf(TEXT("tab_%d"), I + 1)))
		{
			Hud->SelectTab(Tab::Upgrades);
			SendKey(Hud, K);
			const Tab Want = Hud->IsTabShown(I) ? static_cast<Tab>(I) : Tab::Upgrades;
			TestEqual(FString::Printf(TEXT("tab_%d %s"), I + 1, *K.ToString()), static_cast<int32>(Hud->GetActiveTab()), static_cast<int32>(Want));
		}
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("open_upgrades")))
	{
		Hud->SelectTab(Tab::Realms);
		SendKey(Hud, K);
		TestEqual(FString::Printf(TEXT("open_upgrades %s jumps to Upgrades"), *K.ToString()), static_cast<int32>(Hud->GetActiveTab()),
			static_cast<int32>(Tab::Upgrades));
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("buy_amount")))
	{
		const int64 Before = Hud->GetBuyMultiplier();
		SendKey(Hud, K);
		TestTrue(FString::Printf(TEXT("buy_amount %s cycles the amount"), *K.ToString()), Hud->GetBuyMultiplier() != Before);
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("open_skill_tree")))
	{
		SendKey(Hud, K);
		UPawSkillTreeScreen* Tree = Hud->GetSkillTree();
		if (TestTrue(FString::Printf(TEXT("open_skill_tree %s opens the tree"), *K.ToString()), Tree && Tree->IsActivated()))
		{
			Tree->PressClose();
		}
		W.NextFrames();
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("pause")))
	{
		SendKey(Hud, K);
		UPawPauseMenu* Pause = Hud->GetPauseMenu();
		if (TestTrue(FString::Printf(TEXT("pause %s opens the pause menu"), *K.ToString()), Pause && Pause->IsActivated()))
		{
			Pause->PressContinue();
		}
		W.NextFrames();
	}
	for (const FKey& K : { EKeys::Gamepad_FaceButton_Right, EKeys::F7, EKeys::K })
	{
		const Tab Before = Hud->GetActiveTab();
		const int64 Amount = Hud->GetBuyMultiplier();
		SendKey(Hud, K);
		TestTrue(FString::Printf(TEXT("%s does nothing on the HUD"), *K.ToString()), !Hud->IsMenuUp() && Hud->GetActiveTab() == Before
			&& Hud->GetBuyMultiplier() == Amount);
	}
	TestFalse(TEXT("press with nothing selected presses nothing"), Hud->PressSelected());

	// --- the pause menu: back steps back, pause closes, anything else stays ---
	for (const FKey& K : PawInput::KeysOf(TEXT("back")))
	{
		if (!TestTrue(TEXT("the pause menu opens"), Hud->OpenPauseMenu()))
		{
			break;
		}
		UPawPauseMenu* Pause = Hud->GetPauseMenu();
		Pause->PressSave();
		SendKey(Pause, K);
		TestTrue(FString::Printf(TEXT("back %s on the Save page: the main page"), *K.ToString()), Pause->IsActivated() && Pause->IsPageShown(false));
		SendKey(Pause, K);
		TestFalse(FString::Printf(TEXT("back %s on the main page closes"), *K.ToString()), Pause->IsActivated());
		W.NextFrames();
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("pause")))
	{
		Hud->OpenPauseMenu();
		UPawPauseMenu* Pause = Hud->GetPauseMenu();
		Pause->PressSave();
		SendKey(Pause, K);
		const bool bBackToo = PawInput::KeyIs(K, TEXT("back"));
		// Esc is back as well: the Save page steps back first. Start (pause alone) closes from either page.
		TestEqual(FString::Printf(TEXT("pause %s on the Save page"), *K.ToString()), Pause->IsActivated(), bBackToo);
		if (Pause->IsActivated())
		{
			Pause->PressContinue();
		}
		W.NextFrames();
	}
	if (Hud->OpenPauseMenu())
	{
		UPawPauseMenu* Pause = Hud->GetPauseMenu();
		for (const FKey& K : Unbound)
		{
			SendKey(Pause, K);
		}
		TestTrue(TEXT("unbound keys leave the pause menu up"), Pause->IsActivated());
		Pause->PressContinue();
		W.NextFrames();
	}

	// --- the settings dialog: back closes, section_prev / section_next turn the page ---
	for (const FKey& K : PawInput::KeysOf(TEXT("section_next")))
	{
		UPawSettingsDialog* D = UPawSettingsDialog::Open(W.Root->ModalStack, nullptr);
		if (!TestNotNull(TEXT("the settings"), D))
		{
			break;
		}
		D->PressSection(Pc::PcSettingsSection::Audio);
		SendKey(D, K);
		TestTrue(FString::Printf(TEXT("section_next %s: Video"), *K.ToString()), D->IsPageShown(Pc::PcSettingsSection::Video));
		D->PressClose();
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("section_prev")))
	{
		UPawSettingsDialog* D = UPawSettingsDialog::Open(W.Root->ModalStack, nullptr);
		D->PressSection(Pc::PcSettingsSection::Audio);
		SendKey(D, K);
		TestTrue(FString::Printf(TEXT("section_prev %s: Controls (wraps)"), *K.ToString()), D->IsPageShown(Pc::PcSettingsSection::Controls));
		D->PressClose();
	}
	for (const FKey& K : PawInput::KeysOf(TEXT("back")))
	{
		UPawSettingsDialog* D = UPawSettingsDialog::Open(W.Root->ModalStack, nullptr);
		SendKey(D, K);
		TestFalse(FString::Printf(TEXT("back %s closes the settings"), *K.ToString()), D->IsActivated());
	}
	{
		UPawSettingsDialog* D = UPawSettingsDialog::Open(W.Root->ModalStack, nullptr);
		D->PressSection(Pc::PcSettingsSection::Audio); // the dialog keeps its section between openings (as Godot's)
		for (const FKey& K : Unbound)
		{
			SendKey(D, K);
		}
		TestTrue(TEXT("unbound keys leave the settings up, on their page"), D->IsActivated() && D->IsPageShown(Pc::PcSettingsSection::Audio));
		D->PressClose();
	}

	// --- the skill tree: zoom while held, the steer stick pans, back and open_skill_tree close ---
	if (TestTrue(TEXT("the tree opens"), Hud->OpenSkillTree(false)))
	{
		UPawSkillTreeScreen* Tree = Hud->GetSkillTree();
		for (const FKey& K : PawInput::KeysOf(TEXT("zoom_in"), false))
		{
			if (K.IsAnalog())
			{
				continue; // a trigger: its strength, below
			}
			SendKey(Tree, K);
			TestEqual(FString::Printf(TEXT("zoom_in %s held"), *K.ToString()), Tree->GetZoomHeld(), 1.0);
			SendKeyUp(Tree, K);
			TestEqual(FString::Printf(TEXT("zoom_in %s let go"), *K.ToString()), Tree->GetZoomHeld(), 0.0);
		}
		for (const FKey& K : PawInput::KeysOf(TEXT("zoom_out"), false))
		{
			if (K.IsAnalog())
			{
				continue;
			}
			SendKey(Tree, K);
			TestEqual(FString::Printf(TEXT("zoom_out %s held"), *K.ToString()), Tree->GetZoomHeld(), -1.0);
			SendKeyUp(Tree, K);
		}
		// The triggers by strength (Godot get_action_strength): past the 0.5 dead zone, rescaled; the stand-in key alone
		// changes nothing; a held key is strength 1 whatever the trigger says.
		SendAnalog(Tree, EKeys::Gamepad_RightTriggerAxis, 0.75f);
		TestEqual(TEXT("the right trigger three quarters in: zoom_in at half strength"), Tree->GetZoomHeld(), 0.5);
		SendKey(Tree, EKeys::Gamepad_RightTrigger);
		TestEqual(TEXT("its digital stand-in changes nothing"), Tree->GetZoomHeld(), 0.5);
		SendAnalog(Tree, EKeys::Gamepad_LeftTriggerAxis, 1.0f);
		TestEqual(TEXT("both triggers: in less out"), Tree->GetZoomHeld(), -0.5);
		SendAnalog(Tree, EKeys::Gamepad_LeftTriggerAxis, 0.3f);
		TestEqual(TEXT("a trigger inside its dead zone counts nothing"), Tree->GetZoomHeld(), 0.5);
		SendKey(Tree, EKeys::Equals);
		TestEqual(TEXT("a held key is strength 1"), Tree->GetZoomHeld(), 1.0);
		SendKeyUp(Tree, EKeys::Equals);
		SendAnalog(Tree, EKeys::Gamepad_RightTriggerAxis, 0.0f);
		SendAnalog(Tree, EKeys::Gamepad_LeftTriggerAxis, 0.0f);
		TestEqual(TEXT("let go: no zoom"), Tree->GetZoomHeld(), 0.0);
		// The steer stick (the right stick): Godot's get_vector (Settings > Invert X / Y join with the Controls page, slice 4).
		SendAnalog(Tree, EKeys::Gamepad_RightX, 0.8f);
		SendAnalog(Tree, EKeys::Gamepad_RightY, 0.0f);
		TMap<FKey, float> Axes;
		Axes.Add(EKeys::Gamepad_RightX, 0.8f);
		Axes.Add(EKeys::Gamepad_RightY, 0.0f);
		const FVector2f Want = PawInput::SteerVector(Axes);
		TestTrue(TEXT("the steer stick pans the tree as Godot's get_vector"), Tree->GetSteer().X > 0.0 && Tree->GetSteer().X == Want.X);
		SendAnalog(Tree, EKeys::Gamepad_RightY, 0.9f);
		TestTrue(TEXT("stick up is the screen's up"), Tree->GetSteer().Y < 0.0);
		SendAnalog(Tree, EKeys::Gamepad_RightX, 0.0f);
		SendAnalog(Tree, EKeys::Gamepad_RightY, 0.1f);
		TestTrue(TEXT("inside the dead zone: no pan"), Tree->GetSteer().IsZero());
		SendAnalog(Tree, EKeys::Gamepad_LeftX, 1.0f);
		TestTrue(TEXT("the left stick does not pan"), Tree->GetSteer().IsZero());
		for (const FKey& K : Unbound)
		{
			SendKey(Tree, K);
		}
		TestTrue(TEXT("unbound keys leave the tree up"), Tree->IsActivated());
		Tree->PressClose();
		W.NextFrames();
	}
	for (const TCHAR* Action : { TEXT("back"), TEXT("open_skill_tree") })
	{
		for (const FKey& K : PawInput::KeysOf(Action))
		{
			if (!Hud->OpenSkillTree(false))
			{
				AddError(TEXT("the tree did not open"));
				break;
			}
			SendKey(Hud->GetSkillTree(), K);
			TestFalse(FString::Printf(TEXT("%s %s closes the browsing tree"), Action, *K.ToString()), Hud->GetSkillTree()->IsActivated());
			W.NextFrames();
		}
	}

	// --- the title: back steps back (on the menu, it asks to quit) ---
	UPawMainMenu* Title = CreateWidget<UPawMainMenu>(W.GI, PawUi::Resolve<UPawMainMenu>());
	if (TestNotNull(TEXT("the title"), Title))
	{
		Title->OnQuit = []() {};
		Title->Refresh();
		for (const FKey& K : PawInput::KeysOf(TEXT("back")))
		{
			Title->PressCredits();
			SendKey(Title, K);
			TestTrue(FString::Printf(TEXT("back %s closes the credits"), *K.ToString()), Title->GetTopLayer() == UPawMainMenu::ELayer::Menu);
		}
		for (const FKey& K : Unbound)
		{
			SendKey(Title, K);
		}
		TestTrue(TEXT("unbound keys leave the title on its menu"), Title->GetTopLayer() == UPawMainMenu::ELayer::Menu);
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputPounceTest, "Pawpocalypse.Input.Pounce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputPounceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("a click on the field pounces"), APawRealmView::PounceHeld(true, true, false));
	TestFalse(TEXT("a click over the dock is the dock's"), APawRealmView::PounceHeld(true, true, true));
	TestTrue(TEXT("the trigger pounces with the cat under the dock"), APawRealmView::PounceHeld(true, false, true));
	TestFalse(TEXT("nothing held, nothing pounces"), APawRealmView::PounceHeld(false, false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputVelocityTest, "Pawpocalypse.Input.Velocity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputVelocityTest::RunTest(const FString& Parameters)
{
	FString Text;
	if (!TestTrue(TEXT("the fixture"), FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/input_velocity_godot.tsv")))))
	{
		return false;
	}
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	int32 Rows = 0, Mismatch = 0, Still = 0, Full = 0;
	bool bHeader = true;
	for (const FString& Line : Lines)
	{
		if (Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		if (bHeader)
		{
			bHeader = false;
			continue;
		}
		TArray<FString> C;
		Line.ParseIntoArray(C, TEXT("\t"), false);
		auto F = [&C](int32 I) { return static_cast<float>(std::strtod(TCHAR_TO_UTF8(*C[I]), nullptr)); };
		const FVector2f V = PawInput::StickVelocity(FVector2f(F(0), F(1)), F(2));
		if ((V.X != F(3) || V.Y != F(4)) && ++Mismatch <= 8)
		{
			AddError(FString::Printf(TEXT("input (%.9g, %.9g) width %.9g: (%.9g, %.9g) vs Godot (%.9g, %.9g)"), F(0), F(1), F(2), V.X, V.Y, F(3), F(4)));
		}
		++Rows;
		Still += (F(3) == 0.0f && F(4) == 0.0f) ? 1 : 0;
		Full += FMath::IsNearlyEqual(FMath::Sqrt(F(0) * F(0) + F(1) * F(1)), 1.0f, 1e-6f) ? 1 : 0;
	}
	TestEqual(TEXT("every velocity as Godot's"), Mismatch, 0);
	TestEqual(TEXT("rows"), Rows, 2000);
	TestTrue(TEXT("a stick at rest"), Still >= 50);
	TestTrue(TEXT("a full push"), Full >= 50);
	// Settings > Controls > stick dead zone (InputBindings.apply): the steer actions take it, clamped to 0.05 .. 0.9.
	PawInput::ResetBindings();
	PawInput::SetSteerDeadZone(0.4f);
	TestEqual(TEXT("the setting is the steer dead zone"), PawInput::DeadZoneOf(TEXT("steer_left")), 0.4f);
	TestEqual(TEXT("other actions keep theirs"), PawInput::DeadZoneOf(TEXT("menu_up")), 0.5f);
	TestFalse(TEXT("0.3 inside it: not pressed"), PawInput::IsPressed(TEXT("steer_right"), EKeys::Gamepad_RightX, 0.3f));
	TestTrue(TEXT("0.45 past it: pressed"), PawInput::IsPressed(TEXT("steer_right"), EKeys::Gamepad_RightX, 0.45f));
	PawInput::SetSteerDeadZone(0.95f);
	TestEqual(TEXT("clamped to 0.9"), PawInput::DeadZoneOf(TEXT("steer_up")), 0.9f);
	PawInput::SetSteerDeadZone(0.01f);
	TestEqual(TEXT("clamped to 0.05"), PawInput::DeadZoneOf(TEXT("steer_up")), 0.05f);
	PawInput::SetSteerDeadZone(-1.0f);
	TestEqual(TEXT("unset: the map's 0.2"), PawInput::DeadZoneOf(TEXT("steer_down")), 0.2f);
	return true;
}

namespace
{
	FPointerEvent MouseMove(float DX, float DY)
	{
		return FPointerEvent(0, FVector2D(100, 100), FVector2D(100 - DX, 100 - DY), TSet<FKey>(), EKeys::Invalid, 0.0f, FModifierKeysState());
	}

	FPointerEvent MouseClick()
	{
		return FPointerEvent(0, FVector2D(100, 100), FVector2D(100, 100), TSet<FKey>(), EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
	}

	FAnalogInputEvent Stick(const FKey& Key, float Value, int32 Device = 0)
	{
		return FAnalogInputEvent(Key, FModifierKeysState(), FInputDeviceId::CreateFromInternalId(Device), false, 0, 0, Value);
	}

	FString S(const FText& Text) { return Text.ToString(); }

	FKeyEvent PadKey(const FKey& Key, int32 Device = 0, bool bRepeat = false)
	{
		return FKeyEvent(Key, FModifierKeysState(), FInputDeviceId::CreateFromInternalId(Device), bRepeat, 0, 0);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputDeviceTest, "Pawpocalypse.Input.Device", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputDeviceTest::RunTest(const FString& Parameters)
{
	FInputWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	PawInput::ResetBindings();
	UPawInputDeviceSubsystem* D = W.GI->GetSubsystem<UPawInputDeviceSubsystem>();
	if (!TestNotNull(TEXT("the device subsystem"), D))
	{
		W.Close();
		return false;
	}
	int64 Clock = D->GetState().BootMs + 100000; // well past the boot window
	D->Clock = [&Clock]() { return Clock; };
	using Fam = Pc::PcFamily;
	int32 FamilyChanges = 0;
	D->OnFamilyChanged.AddLambda([&FamilyChanges](Fam) { ++FamilyChanges; });

	// A key: the keyboard's prompts; the pointer was not the last thing used.
	D->ObserveKey(KeyDown(EKeys::SpaceBar), true);
	TestEqual(TEXT("a key: keyboard prompts"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::Keyboard));
	TestFalse(TEXT("a key is not the pointer"), D->LastWasPointer());
	// A pad button: the pad's prompts (XInput: every pad is the Xbox family), the cursor untouched.
	D->ObserveKey(PadKey(EKeys::Gamepad_FaceButton_Bottom, 1), true);
	TestEqual(TEXT("pad A: Xbox prompts"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::Xbox));
	TestEqual(TEXT("the pad in hand"), D->GetState().ActiveDevice, static_cast<int64_t>(1));
	TestFalse(TEXT("a button does not take the cursor"), D->IsStickSteering());
	TestTrue(TEXT("the family changed and said so"), FamilyChanges >= 1);
	// The steer stick (the right stick) past AXIS_CLAIM takes the cursor and hides the pointer; the left one does not.
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftX, 0.9f, 1));
	TestFalse(TEXT("the left stick is not the steer stick"), D->IsStickSteering());
	D->ObserveAnalog(Stick(EKeys::Gamepad_RightY, 0.3f, 1));
	TestFalse(TEXT("under the claim: nothing"), D->IsStickSteering());
	D->ObserveAnalog(Stick(EKeys::Gamepad_RightY, -0.7f, 1));
	TestTrue(TEXT("the right stick steers"), D->IsStickSteering());
	TestTrue(TEXT("the pointer hides while the stick steers"), D->IsPointerHidden());
	// The cat warps the pointer: the motion that follows is the echo.
	D->NoteWarp();
	Clock += 50;
	D->ObserveMouseMove(MouseMove(40.0f, 0.0f));
	TestTrue(TEXT("the warp's echo does not take the cursor back"), D->IsStickSteering());
	Clock += 200;
	// A hand on the mouse: 6 px of travel within the quiet window.
	D->ObserveMouseMove(MouseMove(3.0f, 0.0f));
	TestTrue(TEXT("3 px: still the stick's"), D->IsStickSteering());
	Clock += 100;
	D->ObserveMouseMove(MouseMove(3.0f, 0.0f));
	TestFalse(TEXT("6 px within the quiet window: the mouse's"), D->IsStickSteering());
	TestEqual(TEXT("motion never flips the prompts"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::Xbox));
	// A click: the keyboard's prompts and the mouse's cursor.
	D->ObserveMouseButton(MouseClick(), true);
	TestEqual(TEXT("a click: keyboard prompts"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::Keyboard));
	TestTrue(TEXT("a click is the pointer"), D->LastWasPointer());
	// A stick's or trigger's digital stand-in is not an event Godot ever sees: it would claim the pad as a button would.
	D->ObserveKey(PadKey(EKeys::Gamepad_RightStick_Down, 1), true);
	D->ObserveKey(PadKey(EKeys::Gamepad_RightTrigger, 1), true);
	TestEqual(TEXT("stand-in keys change nothing"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::Keyboard));
	TestTrue(TEXT("and leave the pointer the last thing used"), D->LastWasPointer());
	// The press guard: a trigger past its dead zone is a press; it counts for a modal opened before it, after the guard.
	const int64 Opened = Clock;
	Clock += 10;
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftTriggerAxis, 0.6f, 1));
	TestEqual(TEXT("the trigger began a press"), D->GetState().PressBeganMs, static_cast<int64_t>(Clock));
	TestFalse(TEXT("too soon after the modal opened"), D->PressCountsFor(Opened));
	Clock += 400;
	TestTrue(TEXT("after the guard"), D->PressCountsFor(Opened));
	TestFalse(TEXT("not for a modal that opened after the press"), D->PressCountsFor(Clock - 5));
	// Settings > Controls > Button prompts pins the family; Swap sticks steers with the left.
	W.Settings->SetPromptStyle(TEXT("playstation"));
	TestEqual(TEXT("pinned: PlayStation"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::PlayStation));
	D->ObserveKey(KeyDown(EKeys::A), true);
	TestEqual(TEXT("a key does not unpin it"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::PlayStation));
	W.Settings->SetPromptStyle(TEXT("auto"));
	TestEqual(TEXT("auto: what was detected"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::Keyboard));
	// Connections: a toast after the boot window, the pad in hand gone back to the mouse and keys.
	D->ObserveAnalog(Stick(EKeys::Gamepad_RightX, 1.0f, 2));
	TestTrue(TEXT("pad 2 steers"), D->IsStickSteering());
	D->ObserveConnection(3, true);
	TestTrue(TEXT("a pad connected: the toast"), D->IsToastShown() && S(D->GetToastText()) == TEXT("Controller connected"));
	D->ObserveConnection(2, false);
	TestTrue(TEXT("the pad in hand gone: the toast"), S(D->GetToastText()) == TEXT("Controller disconnected"));
	TestFalse(TEXT("back to the mouse"), D->IsStickSteering());
	TestEqual(TEXT("and the keys' prompts"), static_cast<int32>(D->GetFamily()), static_cast<int32>(Fam::Keyboard));
	// What is HELD (Input.is_action_pressed, the dock's polling): a key while down; a trigger past its dead zone by its analog
	// value, never by Slate's digital stand-in; nothing after it is let go.
	D->ObserveKey(PadKey(EKeys::Gamepad_FaceButton_Bottom, 1), false); // pad A (press) went down above and was never let go
	D->ObserveKey(KeyDown(EKeys::SpaceBar), false);
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftTriggerAxis, 0.0f, 1)); // the press guard's trigger above, let go
	TestFalse(TEXT("nothing held"), D->IsActionHeld(TEXT("press")));
	D->ObserveKey(KeyDown(EKeys::SpaceBar), true);
	TestTrue(TEXT("Space held: press"), D->IsActionHeld(TEXT("press")));
	D->ObserveKey(KeyDown(EKeys::SpaceBar), false);
	TestFalse(TEXT("Space let go"), D->IsActionHeld(TEXT("press")));
	D->ObserveKey(PadKey(EKeys::Gamepad_LeftTrigger, 1), true);
	TestFalse(TEXT("the left trigger's stand-in alone is not a press"), D->IsActionHeld(TEXT("press")));
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftTriggerAxis, 0.3f, 1));
	TestFalse(TEXT("LT at 0.3: under its dead zone"), D->IsActionHeld(TEXT("press")));
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftTriggerAxis, 0.7f, 1));
	TestTrue(TEXT("LT at 0.7: held"), D->IsActionHeld(TEXT("press")) && D->IsActionHeld(TEXT("zoom_out")));
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftTriggerAxis, 0.0f, 1));
	D->ObserveKey(PadKey(EKeys::Gamepad_LeftTrigger, 1), false);
	TestFalse(TEXT("LT let go"), D->IsActionHeld(TEXT("press")));
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftY, -0.8f, 1));
	TestTrue(TEXT("the left stick down: menu_down held"), D->IsActionHeld(TEXT("menu_down")) && !D->IsActionHeld(TEXT("menu_up")));
	D->ObserveAnalog(Stick(EKeys::Gamepad_LeftY, 0.0f, 1));
	// The paw: closed while the left button is held, open again on its release; another button changes nothing. In the
	// editor the engine's cursor is never touched (the designer's own).
	TestTrue(TEXT("the click above left it closed"), D->IsPawClosed());
	D->ObserveMouseButton(MouseClick(), false);
	TestFalse(TEXT("released: open"), D->IsPawClosed());
	D->ObserveMouseButton(MouseClick(), true);
	TestTrue(TEXT("left button down: the paw closes"), D->IsPawClosed());
	D->ObserveMouseButton(FPointerEvent(0, FVector2D(100, 100), FVector2D(100, 100), TSet<FKey>(), EKeys::RightMouseButton, 0.0f, FModifierKeysState()), false);
	TestTrue(TEXT("the right button changes nothing"), D->IsPawClosed());
	D->ObserveMouseButton(MouseClick(), false);
	TestFalse(TEXT("left button up: open again"), D->IsPawClosed());
	TestEqual(TEXT("never applied in the editor"), D->GetPawApplied(), 0);
	for (const bool bClosed : { false, true })
	{
		const FString Png = FPaths::ProjectContentDir() / UPawInputDeviceSubsystem::PawCursorPath(bClosed).ToString() + TEXT(".png");
		TestTrue(FString::Printf(TEXT("the paw's PNG is there: %s"), *Png), FPaths::FileExists(Png));
	}
	// The pointer over the open field hides with the mouse; never while it is not over the field.
	D->SetPointerOverField(true);
	TestTrue(TEXT("over the field: hidden"), D->IsPointerHidden());
	D->SetPointerOverField(false);
	TestFalse(TEXT("over the dock: shown"), D->IsPointerHidden());
	D->Clock = nullptr;
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputBootQuietTest, "Pawpocalypse.Input.BootQuiet", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputBootQuietTest::RunTest(const FString& Parameters)
{
	FInputWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawInputDeviceSubsystem* D = W.GI->GetSubsystem<UPawInputDeviceSubsystem>();
	// Pads already plugged in at launch report a connection in the first moments: not news, no toast.
	const int64 Boot = D->GetState().BootMs;
	int64 Clock = Boot + 500;
	D->Clock = [&Clock]() { return Clock; };
	D->ObserveConnection(0, true);
	TestFalse(TEXT("a pad at launch: no toast"), D->IsToastShown());
	Clock = Boot + 2500;
	D->ObserveConnection(1, true);
	TestTrue(TEXT("a pad later: the toast"), D->IsToastShown());
	D->ObserveConnection(0, false);
	TestTrue(TEXT("a pad that is not the one in hand goes quietly (the last toast stands)"), S(D->GetToastText()) == TEXT("Controller connected"));
	D->Clock = nullptr;
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputPadAudioTest, "Pawpocalypse.Input.PadAudio", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputPadAudioTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	// input_device.gd _watch_for_pad_audio: a pad connected after the boot window, the output devices read then and 2 s later;
	// one that appeared while the game follows the system's: the toast says where to fix it (playtest 2026-10-01, #31).
	FInputWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawInputDeviceSubsystem* D = W.GI->GetSubsystem<UPawInputDeviceSubsystem>();
	UPawAudioSubsystem* A = W.GI->GetSubsystem<UPawAudioSubsystem>();
	if (!TestNotNull(TEXT("the audio"), A))
	{
		W.Close();
		return false;
	}
	const FString Speakers = TEXT("Speakers (Realtek(R) Audio)"), Headset = TEXT("Headset Earphone (Wireless Controller)");
	const FString Connected = TEXT("Controller connected");
	const FString NoSound = TEXT("The controller added a sound device. No sound? Pick your speakers in Settings, Audio, Output.");
	TArray<FString> System = { Speakers };
	A->SystemDevices = [&System]() { return System; };
	A->SwapOutput = [](const FString&) { return true; };
	const int64 Boot = D->GetState().BootMs;
	int64 Clock = Boot + 500;
	D->Clock = [&Clock]() { return Clock; };
	// Pads plugged in at launch: no toast, so no watch.
	D->ObserveConnection(0, true);
	TestEqual(TEXT("a pad at launch: no watch"), D->GetPadAudioWatches(), 0);
	// A pad later; Windows adds its headset a beat after the joypad.
	Clock = Boot + 5000;
	D->ObserveConnection(1, true);
	TestEqual(TEXT("a pad later: watched"), D->GetPadAudioWatches(), 1);
	TestEqual(TEXT("the connect toast first"), S(D->GetToastText()), Connected);
	System.Add(Headset);
	Clock += 1999;
	D->RunPadAudioWatches();
	TestEqual(TEXT("not before the wait"), D->GetPadAudioWatches(), 1);
	TestEqual(TEXT("still the connect toast"), S(D->GetToastText()), Connected);
	Clock += 1;
	D->RunPadAudioWatches();
	TestEqual(TEXT("the watch ends"), D->GetPadAudioWatches(), 0);
	TestTrue(TEXT("a sound device appeared: the toast"), D->IsToastShown() && S(D->GetToastText()) == NoSound);
	// A pad that brings no sound device: nothing more.
	D->ObserveConnection(2, true);
	Clock += 2000;
	D->RunPadAudioWatches();
	TestEqual(TEXT("no new device: only the connect toast"), S(D->GetToastText()), Connected);
	// A device chosen in Settings: nothing can take the sound from it.
	W.Settings->SetOutputDevice(Speakers);
	System = { Speakers };
	D->ObserveConnection(3, true);
	System.Add(Headset);
	Clock += 2000;
	D->RunPadAudioWatches();
	TestEqual(TEXT("a chosen device: no toast"), S(D->GetToastText()), Connected);
	// The setting is read when the look is taken, after the wait (Godot reads it after the await).
	W.Settings->SetOutputDevice(TEXT("Default"));
	System = { Speakers };
	D->ObserveConnection(4, true);
	System.Add(Headset);
	W.Settings->SetOutputDevice(Speakers);
	Clock += 2000;
	D->RunPadAudioWatches();
	TestEqual(TEXT("chosen during the wait: no toast"), S(D->GetToastText()), Connected);
	// Each connect watches on its own: the first's list, then the second's.
	W.Settings->SetOutputDevice(TEXT("Default"));
	System = { Speakers };
	D->ObserveConnection(5, true);
	System.Add(Headset);
	Clock += 1000;
	D->ObserveConnection(6, true);
	TestEqual(TEXT("two watches"), D->GetPadAudioWatches(), 2);
	Clock += 1000;
	D->RunPadAudioWatches();
	TestEqual(TEXT("the first is due: it saw the headset arrive"), S(D->GetToastText()), NoSound);
	D->ObserveConnection(7, false);
	Clock += 1000;
	D->RunPadAudioWatches();
	TestEqual(TEXT("both done"), D->GetPadAudioWatches(), 0);
	D->Clock = nullptr;
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputPointerFieldTest, "Pawpocalypse.Input.PointerOverField", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputPointerFieldTest::RunTest(const FString& Parameters)
{
	Pc::PcRect Field;
	Field.X = 568.0;
	Field.Y = 0.0;
	Field.W = 1352.0;
	Field.H = 1080.0;
	TestTrue(TEXT("on the field"), APawRealmView::PointerOverField(FVector2D(1000, 500), Field, false));
	TestFalse(TEXT("over the dock"), APawRealmView::PointerOverField(FVector2D(300, 500), Field, true));
	TestFalse(TEXT("a panel under the pointer inside the playfield (Godot: a control hovered)"),
		APawRealmView::PointerOverField(FVector2D(1000, 500), Field, true));
	TestFalse(TEXT("outside the playfield"), APawRealmView::PointerOverField(FVector2D(100, 500), Field, false));
	TestFalse(TEXT("the far edge is outside (Rect2.has_point)"), APawRealmView::PointerOverField(FVector2D(1920, 500), Field, false));
	// The stick's step: by velocity, clamped to the playfield, its far edges included.
	const FVector2D Step = APawRealmView::StickStep(FVector2D(1000, 500), FVector2f(600.0f, -120.0f), 0.5, Field);
	TestEqual(TEXT("moved by velocity x time"), Step, FVector2D(1300, 440));
	TestEqual(TEXT("clamped to the far edge"), APawRealmView::StickStep(FVector2D(1900, 1070), FVector2f(900.0f, 900.0f), 1.0, Field),
		FVector2D(1920, 1080));
	TestEqual(TEXT("never under the dock"), APawRealmView::StickStep(FVector2D(600, 10), FVector2f(-900.0f, -900.0f), 1.0, Field),
		FVector2D(568, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputSteerPausedTest, "Pawpocalypse.Input.SteerPaused", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputSteerPausedTest::RunTest(const FString& Parameters)
{
	using ESteer = APawRealmView::ESteer;
	// player.gd _process: R3's recentre first, else the stick while it has the cursor, else the pointer; the Cone of Shame
	// roots the cat (every path).
	TestTrue(TEXT("the stick steers"), APawRealmView::SteerBy(false, false, false, true, true) == ESteer::Stick);
	TestTrue(TEXT("R3 recentres first"), APawRealmView::SteerBy(false, false, true, true, true) == ESteer::Recentre);
	TestTrue(TEXT("else the pointer"), APawRealmView::SteerBy(false, false, false, false, true) == ESteer::Pointer);
	TestTrue(TEXT("nothing to steer with"), APawRealmView::SteerBy(false, false, false, false, false) == ESteer::Nothing);
	TestTrue(TEXT("rooted: nothing"), APawRealmView::SteerBy(false, true, true, true, true) == ESteer::Nothing);
	// Review CR6 (2026-11-10): a paused field (a card pick, the tree, a menu) is steered by nothing, as Godot's player stops
	// with the paused tree. The stick kept steering here: the pointer warped onto the cat under the menu every frame.
	TestTrue(TEXT("paused: the stick steers nothing"), APawRealmView::SteerBy(true, false, false, true, true) == ESteer::Nothing);
	TestTrue(TEXT("paused: R3 recentres nothing"), APawRealmView::SteerBy(true, false, true, true, true) == ESteer::Nothing);
	TestTrue(TEXT("paused: the pointer steers nothing"), APawRealmView::SteerBy(true, false, false, false, true) == ESteer::Nothing);
	return true;
}

#endif
