// Pawpocalypse.Input.Remap*: remapping in the game (docs/05 §3; Godot InputBindings.apply, InputDevice.apply_bindings). The
// rules are PawCore's PcBindings (L1, against the real input_bindings.gd); these check the translation into the engine.
//
// Pawpocalypse.Input.RemapKeys      every event of Godot's map turns into its Unreal key and axis half (Data/InputMap.csv), and
//                                   back; every key the glyph fixture names maps as the CSV does; the generated defaults
//                                   (PawInputDefaults.h) are the CSV's.
// Pawpocalypse.Input.RemapDefaults  with no bindings the live context IS IMC_Pawpocalypse, mapping for mapping.
// Pawpocalypse.Input.Remap          a player's remap through the settings reaches everything that reads the map: what a key is,
//                                   the menus' navigation, the prompts, the field's context; a swap moves both rows; Swap
//                                   confirm and back, Swap sticks and the stick dead zone; the file holds Godot's [bindings]
//                                   and a Godot-written settings.cfg is applied as read; Reset all controls puts it all back.
//
// Every test leaves the defaults applied (the live context is the run's). Negative tests: tools/sabotage_realm.py (`remap_*`).

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Input/PawHalfAxis.h"
#include "Input/PawInputDefaults.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Input/PawInputMap.h"
#include "Input/PawKeyCodes.h"
#include "Input/PawNavigationConfig.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawGlyphs.h"
#include "UI/PawPrompts.h"
#include "UI/PawWidgets.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "PcBindings.h"
#include "PcSettings.h"

#include <string>

namespace
{
	// One mapping as text: action, key, and an axis half's sign and dead zone.
	FString MappingText(const FEnhancedActionKeyMapping& M)
	{
		const UPawHalfAxis* Half = M.Modifiers.Num() == 1 ? Cast<UPawHalfAxis>(M.Modifiers[0]) : nullptr;
		return FString::Printf(TEXT("%s %s%s"), M.Action ? *M.Action->ActionDescription.ToString() : TEXT("?"), *M.Key.ToString(),
			Half ? *FString::Printf(TEXT(" %+.0f@%.3f"), Half->Sign, Half->DeadZone) : TEXT(""));
	}

	TArray<FString> MappingsOf(const UInputMappingContext* IMC)
	{
		TArray<FString> Out;
		for (const FEnhancedActionKeyMapping& M : IMC->GetMappings())
		{
			Out.Add(MappingText(M));
		}
		Out.Sort();
		return Out;
	}

	std::string Utf8(const FString& S) { return std::string(TCHAR_TO_UTF8(*S)); }

	Pc::PcInputEvent Key(int64 Code) { return Pc::PcInputEvent{ Pc::PcInputKind::Key, Code, 0 }; }

	// A rebind as the Controls page makes it: on the working state of the settings' bindings, then saved as the diff.
	Pc::PcBindResult Rebind(UPawSettingsSubsystem* Settings, const char* Logical, const Pc::PcInputEvent& E)
	{
		Pc::PcInputState State = Pc::BindingWorkingState(Settings->Get().Bindings, PawInput::DefaultState());
		const Pc::PcBindOutcome Out = Pc::BindingRebind(State, Logical, E);
		if (Out.Result == Pc::PcBindResult::Bound || Out.Result == Pc::PcBindResult::Swapped)
		{
			Settings->SetBindings(Pc::BindingDiff(State, PawInput::DefaultState()));
		}
		return Out.Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRemapKeysTest, "Pawpocalypse.Input.RemapKeys", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRemapKeysTest::RunTest(const FString& Parameters)
{
	TArray<FPawInputMapRow> Rows;
	FString Error;
	if (!TestTrue(TEXT("the map reads"), PawInput::ReadInputMap(PawInput::InputMapCsv(), Rows, Error)))
	{
		return false;
	}
	// Every event of the map: its Unreal key and half, and the key back to the event (keypad Enter comes back as Enter).
	for (const FPawInputMapRow& R : Rows)
	{
		const Pc::PcInputEvent E = Pc::BindingEventOf(Utf8(R.GodotEvent), R.GodotCode, R.GodotAxisSign);
		int32 Sign = 0;
		const TArray<FKey> Keys = PawInput::KeysOfEvent(E, Sign);
		TestTrue(FString::Printf(TEXT("%s: %s"), *R.Id, *R.UeKey.ToString()), Keys.Contains(FKey(R.UeKey)));
		if (R.GodotEvent == TEXT("pad_axis"))
		{
			TestEqual(FString::Printf(TEXT("%s: Unreal's half"), *R.Id), Sign, R.UeAxisSign);
		}
		if (!R.bMerged)
		{
			TestTrue(FString::Printf(TEXT("%s: the key is the event"), *R.Id),
				PawInput::EventOfKey(FKey(R.UeKey), static_cast<float>(R.UeAxisSign == 0 ? 1 : R.UeAxisSign)) == E);
		}
	}
	// The keys the glyph fixture names (Godot's keycodes, measured) map as the CSV maps them.
	FString Text;
	const FString Keys = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/input_keys_godot.tsv"));
	if (TestTrue(TEXT("input_keys_godot.tsv reads"), FFileHelper::LoadFileToString(Text, *Keys)))
	{
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines);
		int32 Checked = 0;
		for (const FString& L : Lines)
		{
			TArray<FString> C;
			L.ParseIntoArray(C, TEXT("\t"), false);
			if (L.StartsWith(TEXT("#")) || C.Num() != 3 || C[0] == TEXT("keycode") || C[2].IsEmpty())
			{
				continue;
			}
			int32 Sign = 0;
			TestTrue(FString::Printf(TEXT("Godot key %s is %s"), *C[0], *C[2]),
				PawInput::KeysOfEvent(Key(FCString::Atoi64(*C[0])), Sign).Contains(FKey(*C[2])));
			++Checked;
		}
		TestTrue(TEXT("the fixture's mapped keys checked"), Checked >= 40);
	}
	// Godot's modifiers are one code for both sides; a capture gives Godot's code for either.
	int32 Sign = 0;
	TestEqual(TEXT("Shift is both shifts"), PawInput::KeysOfEvent(Key(4194325), Sign).Num(), 2);
	TestTrue(TEXT("right Ctrl is Godot's Ctrl"), PawInput::EventOfKey(EKeys::RightControl) == Key(4194326));
	// What a capture hands binding_from: a trigger's stand-in at full push, a stick's never a binding, an echo none.
	TestTrue(TEXT("the right trigger captured"), Pc::BindingFrom(PawInput::Captured(EKeys::Gamepad_RightTrigger, true, false))
		== Pc::PcInputEvent{ Pc::PcInputKind::JoyAxis, 5, 1 });
	TestTrue(TEXT("a trigger half pushed"), Pc::BindingFrom(PawInput::Captured(EKeys::Gamepad_LeftTriggerAxis, true, false, true, 0.6f))
		== Pc::PcInputEvent{ Pc::PcInputKind::JoyAxis, 4, 1 });
	TestTrue(TEXT("a trigger barely touched: nothing"), Pc::BindingFrom(PawInput::Captured(EKeys::Gamepad_LeftTriggerAxis, true, false, true, 0.3f)).IsNone());
	TestTrue(TEXT("a stick: nothing"), Pc::BindingFrom(PawInput::Captured(EKeys::Gamepad_LeftStick_Up, true, false)).IsNone());
	TestTrue(TEXT("an echo: nothing"), Pc::BindingFrom(PawInput::Captured(EKeys::J, true, true)).IsNone());
	TestTrue(TEXT("the wheel: nothing"), Pc::BindingFrom(PawInput::Captured(EKeys::MouseScrollUp, true, false)).IsNone());
	TestTrue(TEXT("pad Y"), Pc::BindingFrom(PawInput::Captured(EKeys::Gamepad_FaceButton_Top, true, false)) == Pc::PcInputEvent{ Pc::PcInputKind::JoyButton, 3, 0 });
	// The generated defaults are the CSV's, in Godot's InputMap order.
	Pc::PcInputState FromCsv;
	for (const FPawInputMapRow& R : Rows)
	{
		Pc::BindingAt(FromCsv, Utf8(R.Action)).push_back(Pc::BindingEventOf(Utf8(R.GodotEvent), R.GodotCode, R.GodotAxisSign));
	}
	FromCsv = Pc::BindingGodotOrder(FromCsv);
	const Pc::PcInputState& Defaults = PawInput::DefaultState();
	bool bSame = Defaults.size() == FromCsv.size();
	for (size_t I = 0; bSame && I < Defaults.size(); ++I)
	{
		bSame = Defaults[I].Action == FromCsv[I].Action && Defaults[I].Events == FromCsv[I].Events;
	}
	TestTrue(TEXT("PawInputDefaults.h is Data/InputMap.csv (py tools/gen_input_defaults.py)"), bSame);
	for (const FPawInputMapRow& R : Rows)
	{
		TestEqual(FString::Printf(TEXT("%s: the action's dead zone"), *R.Id), PawInput::DefaultDeadZone(R.Action), R.DeadZone);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRemapDefaultsTest, "Pawpocalypse.Input.RemapDefaults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRemapDefaultsTest::RunTest(const FString& Parameters)
{
	const UInputMappingContext* Asset = PawInput::LoadContext();
	if (!TestNotNull(TEXT("IMC_Pawpocalypse"), Asset))
	{
		return false;
	}
	PawInput::ApplyBindings(Pc::PcSettings());
	const TArray<FString> Live = MappingsOf(PawInput::LiveContext());
	const TArray<FString> Want = MappingsOf(Asset);
	TestEqual(TEXT("as many mappings"), Live.Num(), Want.Num());
	for (const FString& M : Want)
	{
		TestTrue(FString::Printf(TEXT("live has %s"), *M), Live.Contains(M));
	}
	for (const FString& M : Live)
	{
		TestTrue(FString::Printf(TEXT("the asset has %s"), *M), Want.Contains(M));
	}
	TestFalse(TEXT("the live context is not the asset"), PawInput::LiveContext() == Asset);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawRemapTest, "Pawpocalypse.Input.Remap", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawRemapTest::RunTest(const FString& Parameters)
{
	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	GI->InitializeStandalone();
	UPawSettingsSubsystem* Settings = GI->GetSubsystem<UPawSettingsSubsystem>();
	UPawInputDeviceSubsystem* Device = GI->GetSubsystem<UPawInputDeviceSubsystem>();
	ON_SCOPE_EXIT
	{
		PawInput::ApplyBindings(Pc::PcSettings());
		GI->Shutdown();
	};
	if (!TestNotNull(TEXT("settings"), Settings) || !TestNotNull(TEXT("device"), Device))
	{
		return false;
	}
	const TSharedPtr<FPawMemorySaveStorage> Storage = MakeShared<FPawMemorySaveStorage>();
	Settings->UseStorageForTest(Storage);
	int32 Told = 0;
	Device->OnControlsChanged.AddLambda([&Told]() { ++Told; });
	const int32 Applied = Device->GetBindingsApplied();
	auto Stored = [&Storage]()
	{
		TArray<uint8> Bytes;
		Storage->Read(UPawSettingsSubsystem::EntryName, Bytes);
		return FString(UTF8_TO_TCHAR(std::string(reinterpret_cast<const char*>(Bytes.GetData()), Bytes.Num()).c_str()));
	};

	// A prompt on screen for Up (the dock's legend draws menu_up): it shows W, and must redraw itself on the remap.
	UPawPromptIcon* Icon = CreateWidget<UPawPromptIcon>(GI, PawUi::Resolve<UPawPromptIcon>());
	if (!TestNotNull(TEXT("a prompt icon"), Icon))
	{
		return false;
	}
	const TSharedRef<SWidget> HeldIcon = Icon->TakeWidget();
	Icon->SetFamilyOverride(static_cast<int32>(Pc::PcFamily::Keyboard));
	Icon->SetAction(TEXT("menu_up"));
	TestEqual(TEXT("the prompt shows W"), Icon->GetShown().Label.ToString(), FString(TEXT("W")));

	// "Up" onto I: menu_up AND ui_up take it in W's place; W is nothing now.
	TestEqual(TEXT("Up onto I: bound"), static_cast<int32>(Rebind(Settings, "up", Key('I'))), static_cast<int32>(Pc::PcBindResult::Bound));
	TestEqual(TEXT("applied once more"), Device->GetBindingsApplied(), Applied + 1);
	TestTrue(TEXT("the prompts were told"), Told >= 1);
	TestEqual(TEXT("the prompt on screen redrew: I"), Icon->GetShown().Label.ToString(), FString(TEXT("I")));
	TestTrue(TEXT("I is menu_up"), PawInput::KeyIs(EKeys::I, TEXT("menu_up")));
	TestTrue(TEXT("I is ui_up"), PawInput::KeyIs(EKeys::I, TEXT("ui_up")));
	TestEqual(TEXT("W is nothing"), PawInput::ActionsOfKey(EKeys::W).Num(), 0);
	TestTrue(TEXT("the live context maps I"), MappingsOf(PawInput::LiveContext()).Contains(TEXT("menu_up I")));
	{
		const FPawNavigationConfig Nav;
		TestEqual(TEXT("the menus: I moves up"), static_cast<int32>(Nav.DirectionOf(EKeys::I)), static_cast<int32>(EUINavigation::Up));
		TestEqual(TEXT("the menus: W moves nothing"), static_cast<int32>(Nav.DirectionOf(EKeys::W)), static_cast<int32>(EUINavigation::Invalid));
	}
	const PawGlyphs::FPrompt Prompt = PawGlyphs::PromptFor(TEXT("menu_up"), Pc::PcFamily::Keyboard);
	TestTrue(TEXT("the prompt draws I"), Prompt.Kind == PawGlyphs::EKind::Keycap && Prompt.Label.ToString() == TEXT("I"));
	TestTrue(TEXT("the file holds Godot's [bindings]"), Stored().Contains(TEXT("[bindings]")) && Stored().Contains(TEXT("menu_up=[{\n\"physical_keycode\": 73,\n\"type\": \"key\"\n}")));

	// "Up" onto S (Down's): a swap, both rows move.
	TestEqual(TEXT("Up onto S: swapped"), static_cast<int32>(Rebind(Settings, "up", Key('S'))), static_cast<int32>(Pc::PcBindResult::Swapped));
	TestTrue(TEXT("S is up"), PawInput::KeyIs(EKeys::S, TEXT("menu_up")) && !PawInput::KeyIs(EKeys::S, TEXT("menu_down")));
	TestTrue(TEXT("I went to Down"), PawInput::KeyIs(EKeys::I, TEXT("menu_down")) && PawInput::KeyIs(EKeys::I, TEXT("ui_down")));
	// Esc, Start, B and the number keys are refused, and nothing is applied for a refusal.
	const int32 Before = Device->GetBindingsApplied();
	TestEqual(TEXT("Esc refused"), static_cast<int32>(Rebind(Settings, "press", Key(Pc::GodotInput::KeyEscape))), static_cast<int32>(Pc::PcBindResult::RefusedSystem));
	TestEqual(TEXT("1 is taken"), static_cast<int32>(Rebind(Settings, "press", Key('1'))), static_cast<int32>(Pc::PcBindResult::RefusedTaken));
	TestEqual(TEXT("a refusal applies nothing"), Device->GetBindingsApplied(), Before);

	// Swap confirm and back: pad A backs out, B confirms; the menus too.
	Settings->SetSwapConfirmBack(true);
	TestTrue(TEXT("B is ui_accept and press"), PawInput::KeyIs(EKeys::Gamepad_FaceButton_Right, TEXT("ui_accept")) && PawInput::KeyIs(EKeys::Gamepad_FaceButton_Right, TEXT("press")));
	TestTrue(TEXT("A is back"), PawInput::KeyIs(EKeys::Gamepad_FaceButton_Bottom, TEXT("back")) && !PawInput::KeyIs(EKeys::Gamepad_FaceButton_Bottom, TEXT("press")));
	{
		const FPawNavigationConfig Nav;
		TestEqual(TEXT("the menus: A backs out"), static_cast<int32>(Nav.ActionOf(EKeys::Gamepad_FaceButton_Bottom)), static_cast<int32>(EUINavigationAction::Back));
	}
	// Swap sticks: the left stick steers, the right one navigates.
	Settings->SetSwapSticks(true);
	TestTrue(TEXT("the left stick steers"), PawInput::IsSteerAxis(EKeys::Gamepad_LeftX) && !PawInput::IsSteerAxis(EKeys::Gamepad_RightX));
	TestTrue(TEXT("the right stick is menu_up"), PawInput::KeyIs(EKeys::Gamepad_RightStick_Up, TEXT("menu_up")));
	{
		const FPawNavigationConfig Nav;
		TestEqual(TEXT("the menus navigate on the right stick"), Nav.AnalogX(), EKeys::Gamepad_RightX);
	}
	TestTrue(TEXT("the device subsystem steers with the left"), Device->GetState().bSwapSticks);
	// The stick dead zone: the steer halves, and the presses read from it.
	Settings->SetStickDeadzone(0.4);
	TestEqual(TEXT("steer dead zone 0.4"), PawInput::DeadZoneOf(TEXT("steer_left")), 0.4f);
	TestTrue(TEXT("the live steer half is at 0.4"), MappingsOf(PawInput::LiveContext()).Contains(TEXT("steer_left Gamepad_LeftX -1@0.400")));
	TestFalse(TEXT("0.35 does not steer"), PawInput::IsPressed(TEXT("steer_left"), EKeys::Gamepad_LeftX, -0.35f));
	TestTrue(TEXT("0.45 does"), PawInput::IsPressed(TEXT("steer_left"), EKeys::Gamepad_LeftX, -0.45f));

	// A settings.cfg Godot wrote (fixture `input_bindings`, the cfg table's first file with bindings) is applied as read.
	FString Cfg;
	const FString CfgPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/input_bindings_cfg_godot.tsv"));
	if (TestTrue(TEXT("the cfg fixture reads"), FFileHelper::LoadFileToString(Cfg, *CfgPath)))
	{
		TArray<FString> Lines;
		Cfg.ParseIntoArrayLines(Lines);
		FString Written;
		for (const FString& L : Lines)
		{
			TArray<FString> C;
			L.ParseIntoArray(C, TEXT("\t"), false);
			if (!L.StartsWith(TEXT("#")) && C.Num() == 5 && C[0] != TEXT("row") && C[1].Contains(TEXT("[bindings]")) && !C[1].Contains(TEXT("=5")))
			{
				// The fixture escapes \\, \n and \t.
				for (int32 I = 0; I < C[1].Len(); ++I)
				{
					if (C[1][I] == TEXT('\\') && I + 1 < C[1].Len())
					{
						const TCHAR E = C[1][++I];
						Written.AppendChar(E == TEXT('n') ? TEXT('\n') : E == TEXT('t') ? TEXT('\t') : E);
						continue;
					}
					Written.AppendChar(C[1][I]);
				}
				break;
			}
		}
		if (TestFalse(TEXT("a file with bindings"), Written.IsEmpty()))
		{
			const std::string Bytes = Utf8(Written);
			Storage->Put(UPawSettingsSubsystem::EntryName, TArray<uint8>(reinterpret_cast<const uint8*>(Bytes.data()), static_cast<int32>(Bytes.size())));
			Settings->Reload();
			Pc::PcSettings Want;
			TestTrue(TEXT("PawCore reads it"), Pc::SettingsLoad(Bytes, Want));
			const Pc::PcInputState Live = Pc::BindingApply(Want.Bindings, Want.bSwapSticks, Want.bSwapConfirmBack, PawInput::DefaultState());
			bool bSame = Live.size() == PawInput::LiveState().size();
			for (size_t I = 0; bSame && I < Live.size(); ++I)
			{
				bSame = Live[I].Action == PawInput::LiveState()[I].Action && Live[I].Events == PawInput::LiveState()[I].Events;
			}
			TestTrue(TEXT("the live map is the file's"), bSame && !Want.Bindings.empty());
		}
	}

	// Reset all controls: no bindings, every switch back; the live context is the asset's again.
	Settings->ResetControls();
	const Pc::PcSettings& S = Settings->Get();
	TestTrue(TEXT("reset: no bindings"), S.Bindings.empty());
	TestTrue(TEXT("reset: the help switches on, the swaps and inversions off"), S.bShowPrompts && S.bTutorialTips && !S.bSwapConfirmBack
		&& !S.bSwapSticks && !S.bInvertX && !S.bInvertY);
	TestTrue(TEXT("reset: cursor speed 1, dead zone 0.2"), S.CursorSpeed == 1.0 && S.StickDeadzone == 0.2);
	TestTrue(TEXT("reset: W is up again"), PawInput::KeyIs(EKeys::W, TEXT("menu_up")) && PawInput::ActionsOfKey(EKeys::I).Num() == 0);
	TestTrue(TEXT("reset: the live context is the asset's"), MappingsOf(PawInput::LiveContext()) == MappingsOf(PawInput::LoadContext()));
	TestFalse(TEXT("reset: the file has no [bindings]"), Stored().Contains(TEXT("[bindings]")));
	// set_control returns early on an equal value: no write.
	const int32 Writes = Settings->GetWrites();
	Settings->SetSwapSticks(false);
	Settings->SetCursorSpeed(1.0);
	TestEqual(TEXT("an equal value writes nothing"), Settings->GetWrites(), Writes);
	Settings->SetCursorSpeed(5.0);
	TestEqual(TEXT("the cursor speed is held to 2"), Settings->Get().CursorSpeed, 2.0);
	return true;
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

// Pawpocalypse.Input.Azerty: keys by POSITION on any layout (Godot binds physical_keycode). Unreal's FKey is the layout's
// virtual key, so the map goes through the layout: on French AZERTY the key in W's place (labelled Z) is Up, Q's place
// (labelled A) is the buy amount, A's place (labelled Q) is Previous tab; the prompts draw the labels the player sees; a
// capture reports the place. US English gives the map's own keys. Layouts injected (UsLayout / AzertyLayout).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawAzertyTest, "Pawpocalypse.Input.Azerty", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawAzertyTest::RunTest(const FString& Parameters)
{
	ON_SCOPE_EXIT
	{
		PawInput::SetLayoutForTest(nullptr);
		PawInput::ApplyBindings(Pc::PcSettings());
	};
	const PawInput::FPawKeyboardLayout Us = PawInput::UsLayout();
	PawInput::SetLayoutForTest(&Us);
	PawInput::ApplyBindings(Pc::PcSettings());
	TestTrue(TEXT("US: W is up, A is previous tab, Q the buy amount"), PawInput::KeyIs(EKeys::W, TEXT("menu_up"))
		&& PawInput::KeyIs(EKeys::A, TEXT("tab_prev")) && PawInput::KeyIs(EKeys::Q, TEXT("buy_amount")));
	TestTrue(TEXT("US: the live context is the asset's"), MappingsOf(PawInput::LiveContext()) == MappingsOf(PawInput::LoadContext()));

	const PawInput::FPawKeyboardLayout Fr = PawInput::AzertyLayout();
	PawInput::SetLayoutForTest(&Fr);
	PawInput::ApplyBindings(Pc::PcSettings());
	TestTrue(TEXT("AZERTY: Z (W's place) is up, on the menus too"), PawInput::KeyIs(EKeys::Z, TEXT("menu_up")) && PawInput::KeyIs(EKeys::Z, TEXT("ui_up")));
	TestTrue(TEXT("AZERTY: Q (A's place) is previous tab"), PawInput::KeyIs(EKeys::Q, TEXT("tab_prev")));
	TestTrue(TEXT("AZERTY: S and D stay"), PawInput::KeyIs(EKeys::S, TEXT("menu_down")) && PawInput::KeyIs(EKeys::D, TEXT("tab_next")));
	TestTrue(TEXT("AZERTY: A (Q's place) is the buy amount"), PawInput::KeyIs(EKeys::A, TEXT("buy_amount")));
	TestFalse(TEXT("AZERTY: W is nothing"), PawInput::KeyIs(EKeys::W, TEXT("menu_up")) || PawInput::ActionsOfKey(EKeys::W).Num() > 0);
	TestTrue(TEXT("AZERTY: the number keys stay"), PawInput::KeyIs(EKeys::One, TEXT("tab_1")));
	{
		const FPawNavigationConfig Nav;
		TestEqual(TEXT("AZERTY: the menus move up on Z"), static_cast<int32>(Nav.DirectionOf(EKeys::Z)), static_cast<int32>(EUINavigation::Up));
		TestEqual(TEXT("AZERTY: and left on Q"), static_cast<int32>(Nav.DirectionOf(EKeys::Q)), static_cast<int32>(EUINavigation::Left));
	}
	const PawGlyphs::FPrompt Up = PawGlyphs::PromptFor(TEXT("menu_up"), Pc::PcFamily::Keyboard);
	TestTrue(TEXT("AZERTY: the Up prompt reads Z"), Up.Kind == PawGlyphs::EKind::Keycap && Up.Label.ToString() == TEXT("Z"));
	TestTrue(TEXT("AZERTY: a capture of Z is W's place"), PawInput::EventOfKey(EKeys::Z) == Key('W'));
	TestTrue(TEXT("AZERTY: a capture of A is Q's place"), PawInput::EventOfKey(EKeys::A) == Key('Q'));
	TestTrue(TEXT("AZERTY: the arrows are the arrows"), PawInput::KeyIs(EKeys::Up, TEXT("menu_up")));
	return true;
}

#endif
