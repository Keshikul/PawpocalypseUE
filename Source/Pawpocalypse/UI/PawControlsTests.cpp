// Pawpocalypse.UI.Controls.*: Settings > Controls (UPawControlsPanel, what ships: WBP_ControlsPanel grafted into
// WBP_SettingsDialog). Its rules are PcControls, proven against the real controls_panel.gd in PawCore (PcControlsTests,
// fixture `controls`); these prove the widget and the dialog carry them, with Slate's own events:
//
// Pawpocalypse.UI.Controls.MatchesGodot  200 fixture rows replayed on the shipped widgets: a cell pressed, Godot's events sent
//                                        as Slate key, click and analog events THROUGH THE DIALOG, the clock, Reset, Reset
//                                        all, the switches, the sliders, the prompt style, a direct bind; after each, the
//                                        status line, every cell's words, lock, the family its prompt draws and whether it
//                                        shows anything, the Resets, the switches, the sliders and their lines, the settings
//                                        and the bindings as saved, as Godot's.
// Pawpocalypse.UI.Controls.Dialog        the page: every part bound, Godot's words; while a cell listens Esc cancels it and
//                                        the dialog stays open, LB does not turn the page, a click cancels a controller cell;
//                                        a key bound through the dialog reaches the game's map; the five-second timeout.
//
// Negative tests: tools/sabotage_realm.py (`controls_*`).

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Input/PawInputMap.h"
#include "Input/PawKeyCodes.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcBindings.h"
#include "PcControls.h"
#include "PcSettings.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawControlsPanel.h"
#include "UI/PawPrompts.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/PawTextTesting.h"

#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
	struct FControlsWorld
	{
		UGameInstance* GI = nullptr;
		UPawSettingsSubsystem* Settings = nullptr;
		UPawInputDeviceSubsystem* Device = nullptr;
		UPawRootLayout* Root = nullptr;
		UPawSettingsDialog* Dialog = nullptr;
		UPawControlsPanel* Panel = nullptr;
		TSharedPtr<SWidget> RootSlate;
		TSharedPtr<SWidget> DialogSlate;
		TSharedPtr<FPawMemorySaveStorage> Disk;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			Settings = GI->GetSubsystem<UPawSettingsSubsystem>();
			Device = GI->GetSubsystem<UPawInputDeviceSubsystem>();
			if (!T.TestNotNull(TEXT("settings"), Settings) || !T.TestNotNull(TEXT("device"), Device))
			{
				return false;
			}
			Disk = MakeShared<FPawMemorySaveStorage>();
			Settings->UseStorageForTest(Disk);
			Root = CreateWidget<UPawRootLayout>(GI, PawUi::Resolve<UPawRootLayout>());
			if (!T.TestNotNull(TEXT("root"), Root))
			{
				return false;
			}
			RootSlate = Root->TakeWidget();
			Dialog = UPawSettingsDialog::Open(Root->ModalStack, nullptr);
			if (!T.TestTrue(TEXT("the dialog is up"), Dialog && Dialog->IsActivated()))
			{
				return false;
			}
			DialogSlate = Dialog->TakeWidget();
			Dialog->PressSection(Pc::PcSettingsSection::Controls);
			Panel = Dialog->GetControlsPanel();
			return T.TestNotNull(TEXT("the Controls panel"), Panel)
				&& T.TestTrue(TEXT("from its Widget Blueprint"), Panel->GetClass() != UPawControlsPanel::StaticClass());
		}

		void Close()
		{
			PawInput::ApplyBindings(Pc::PcSettings());
			DialogSlate.Reset();
			RootSlate.Reset();
			if (Settings)
			{
				Settings->UseStorageForTest(nullptr);
			}
			if (GI)
			{
				UWorld* World = GI->GetWorld();
				GI->Shutdown();
				if (World)
				{
					GEngine->DestroyWorldContext(World);
					World->DestroyWorld(false);
				}
			}
		}

		// Slate's events, through the dialog's own widget (the path Slate takes: the dialog previews before the cell).
		void Key(const FKey& K, bool bDown, bool bRepeat = false)
		{
			const FKeyEvent E(K, FModifierKeysState(), 0, bRepeat, 0, 0);
			if (bDown)
			{
				DialogSlate->OnPreviewKeyDown(FGeometry(), E);
			}
			else
			{
				DialogSlate->OnKeyUp(FGeometry(), E);
			}
		}

		void Click(const FKey& Button)
		{
			const FPointerEvent E(0, FVector2D(10, 10), FVector2D(10, 10), TSet<FKey>({ Button }), Button, 0.0f, FModifierKeysState());
			DialogSlate->OnPreviewMouseButtonDown(FGeometry(), E);
		}

		void Wheel(float Delta)
		{
			const FPointerEvent E(0, FVector2D(10, 10), FVector2D(10, 10), TSet<FKey>(), EKeys::Invalid, Delta, FModifierKeysState());
			DialogSlate->OnMouseWheel(FGeometry(), E);
		}

		void Analog(const FKey& K, float Value)
		{
			DialogSlate->OnAnalogValueChanged(FGeometry(), FAnalogInputEvent(K, FModifierKeysState(), 0, false, 0, 0, Value));
		}
	};

	FString Unescape(const FString& In)
	{
		FString Out;
		for (int32 I = 0; I < In.Len(); ++I)
		{
			if (In[I] == TEXT('\\') && I + 1 < In.Len())
			{
				const TCHAR C = In[++I];
				Out.AppendChar(C == TEXT('n') ? TEXT('\n') : C == TEXT('t') ? TEXT('\t') : C);
			}
			else
			{
				Out.AppendChar(In[I]);
			}
		}
		return Out;
	}

	double Hex(const FString& V) { return std::strtod(TCHAR_TO_UTF8(*V), nullptr); }

	// var_to_bytes(float) as hex (bit 16 of the header: a double follows, else a float).
	double VariantHex(const FString& Hex)
	{
		TArray<uint8> B;
		for (int32 I = 0; I + 1 < Hex.Len(); I += 2)
		{
			B.Add(static_cast<uint8>(FParse::HexDigit(Hex[I]) * 16 + FParse::HexDigit(Hex[I + 1])));
		}
		if (B.Num() < 8)
		{
			return 0.0;
		}
		const uint32 Header = B[0] | (B[1] << 8) | (B[2] << 16) | (static_cast<uint32>(B[3]) << 24);
		if ((Header & (1u << 16)) && B.Num() >= 12)
		{
			double D = 0.0;
			FMemory::Memcpy(&D, B.GetData() + 4, 8);
			return D;
		}
		float F = 0.0f;
		FMemory::Memcpy(&F, B.GetData() + 4, 4);
		return static_cast<double>(F);
	}

	Pc::PcInputEvent ParseEv(const FString& S)
	{
		Pc::PcInputEvent E;
		if (S.IsEmpty() || S == TEXT("-"))
		{
			return E;
		}
		FString Digits = S.Mid(1);
		switch (S[0])
		{
		case TEXT('k'): E.Kind = Pc::PcInputKind::Key; break;
		case TEXT('m'): E.Kind = Pc::PcInputKind::Mouse; break;
		case TEXT('b'): E.Kind = Pc::PcInputKind::JoyButton; break;
		case TEXT('a'):
			E.Kind = Pc::PcInputKind::JoyAxis;
			E.Sign = Digits.EndsWith(TEXT("+")) ? 1 : Digits.EndsWith(TEXT("-")) ? -1 : 0;
			Digits.LeftChopInline(1);
			break;
		default: break;
		}
		E.Code = FCString::Atoi64(*Digits);
		return E;
	}

	FString Ev(const Pc::PcInputEvent& E)
	{
		switch (E.Kind)
		{
		case Pc::PcInputKind::Key: return FString::Printf(TEXT("k%lld"), E.Code);
		case Pc::PcInputKind::Mouse: return FString::Printf(TEXT("m%lld"), E.Code);
		case Pc::PcInputKind::JoyButton: return FString::Printf(TEXT("b%lld"), E.Code);
		case Pc::PcInputKind::JoyAxis: return FString::Printf(TEXT("a%lld%s"), E.Code, E.Sign > 0 ? TEXT("+") : E.Sign < 0 ? TEXT("-") : TEXT("0"));
		case Pc::PcInputKind::None: break;
		}
		return TEXT("-");
	}

	Pc::PcSavedBindings ParseSaved(const FString& Text)
	{
		Pc::PcSavedBindings Out;
		TArray<FString> Parts;
		Text.ParseIntoArray(Parts, TEXT(";"), true);
		for (const FString& P : Parts)
		{
			FString Action, List;
			P.Split(TEXT("="), &Action, &List);
			Pc::PcSavedAction A;
			A.Action = TCHAR_TO_UTF8(*Action);
			TArray<FString> Evs;
			List.ParseIntoArray(Evs, TEXT(","), true);
			for (const FString& E : Evs)
			{
				A.Events.push_back(Pc::BindingSerialize(ParseEv(E)));
			}
			Out.push_back(A);
		}
		return Out;
	}

	FString SavedText(const Pc::PcSavedBindings& Saved)
	{
		FString Out;
		for (size_t A = 0; A < Saved.size(); ++A)
		{
			FString List;
			for (size_t I = 0; I < Saved[A].Events.size(); ++I)
			{
				List += (I ? TEXT(",") : TEXT("")) + Ev(Pc::BindingDeserialize(Saved[A].Events[I]));
			}
			Out += (A ? TEXT(";") : TEXT("")) + FString(UTF8_TO_TCHAR(Saved[A].Action.c_str())) + TEXT("=") + List;
		}
		return Out;
	}

	const TCHAR* const CheckNames[] = { TEXT("show_prompts"), TEXT("tutorial_tips"), TEXT("swap_confirm_back"), TEXT("swap_sticks"),
		TEXT("invert_x"), TEXT("invert_y") };

	void SetCheck(UPawSettingsSubsystem* S, int32 I, bool bOn)
	{
		switch (I)
		{
		case 0: S->SetShowPrompts(bOn); break;
		case 1: S->SetTutorialTips(bOn); break;
		case 2: S->SetSwapConfirmBack(bOn); break;
		case 3: S->SetSwapSticks(bOn); break;
		case 4: S->SetInvertX(bOn); break;
		default: S->SetInvertY(bOn); break;
		}
	}

	// A Godot event from the fixture, sent as Slate sends it.
	void Send(FControlsWorld& W, const FString& Arg)
	{
		if (Arg == TEXT("mm"))
		{
			return; // motion: the panel never takes it
		}
		if (Arg[0] == TEXT('a'))
		{
			FString Axis, Value;
			Arg.Mid(1).Split(TEXT(":"), &Axis, &Value);
			const double V = FCString::Atod(*Value);
			const Pc::PcInputEvent E{ Pc::PcInputKind::JoyAxis, FCString::Atoi64(*Axis), V > 0.0 ? 1 : (V < 0.0 ? -1 : 1) };
			int32 UeSign = 0;
			const TArray<FKey> Keys = PawInput::KeysOfEvent(E, UeSign);
			if (Keys.Num() > 0)
			{
				// Unreal's axis value: Godot's magnitude in Unreal's direction (the Y axes flip).
				W.Analog(Keys[0], static_cast<float>(FMath::Abs(V) * (V == 0.0 ? 0.0 : static_cast<double>(UeSign))));
			}
			return;
		}
		FString Body = Arg;
		const bool bUp = Body.EndsWith(TEXT("u"));
		const bool bEcho = Body.EndsWith(TEXT("e"));
		if (bUp || bEcho)
		{
			Body.LeftChopInline(1);
		}
		const Pc::PcInputEvent E = ParseEv(Body);
		int32 Sign = 0;
		const TArray<FKey> Keys = PawInput::KeysOfEvent(E, Sign);
		if (Keys.Num() == 0)
		{
			// Nothing Unreal can deliver (a pad's Guide button): Godot's event, given to the panel as a capture would.
			Pc::PcCapturedEvent C;
			C.Kind = E.Kind;
			C.Code = E.Code;
			C.bPressed = !bUp;
			C.bEcho = bEcho;
			W.Panel->CaptureEvent(C, false);
			return;
		}
		if (E.Kind == Pc::PcInputKind::Mouse)
		{
			// A release is nothing the panel keeps; the wheel is Slate's wheel event (Godot's button 4 / 5).
			if (!bUp)
			{
				if (Keys[0] == EKeys::MouseScrollUp || Keys[0] == EKeys::MouseScrollDown)
				{
					W.Wheel(Keys[0] == EKeys::MouseScrollUp ? 1.0f : -1.0f);
				}
				else
				{
					W.Click(Keys[0]);
				}
			}
			return;
		}
		W.Key(Keys[0], !bUp, bEcho);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawControlsGodotTest, "Pawpocalypse.UI.Controls.MatchesGodot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawControlsGodotTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FControlsWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	FString Text;
	const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/controls_godot.tsv"));
	if (!TestTrue(TEXT("the fixture reads"), FFileHelper::LoadFileToString(Text, *Path)))
	{
		W.Close();
		return false;
	}
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	TArray<FString> Header;
	constexpr int32 Rows = 200;
	int32 Moves = 0, Errors = 0;
	TMap<FString, int32> Ops;
	for (const FString& L : Lines)
	{
		if (L.StartsWith(TEXT("#")))
		{
			continue;
		}
		TArray<FString> C;
		L.ParseIntoArray(C, TEXT("\t"), false);
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
		if (FCString::Atoi(*V[TEXT("row")]) >= Rows)
		{
			break;
		}
		const FString& Op = V[TEXT("op")];
		const FString& Arg = V[TEXT("arg")];
		const FString Where = FString::Printf(TEXT("row %s %s %s"), *V[TEXT("row")], *Op, *Arg);
		++Ops.FindOrAdd(Op);
		// The device first: Godot observes an event before the panel acts on it (the families after the move, recorded).
		W.Device->SetDetectedForTest(static_cast<Pc::PcFamily>(FCString::Atoi(*V[TEXT("detected")])));
		if (Op == TEXT("start"))
		{
			TArray<FString> P;
			Arg.ParseIntoArray(P, TEXT("/"), false);
			W.Settings->ResetControls();
			W.Settings->SetBindings(ParseSaved(P[0]));
			TArray<FString> Flags;
			P[1].ParseIntoArray(Flags, TEXT(","), false);
			for (int32 I = 0; I < Pc::ControlsCheckCount; ++I)
			{
				SetCheck(W.Settings, I, Flags[I] == TEXT("1"));
			}
			W.Settings->SetCursorSpeed(VariantHex(P[2]));
			W.Settings->SetStickDeadzone(VariantHex(P[3]));
			W.Settings->SetPromptStyle(P[4]);
			W.Device->SetDetectedForTest(static_cast<Pc::PcFamily>(FCString::Atoi(*P[5])));
			W.Panel->ClearForTest();
		}
		else if (Op == TEXT("listen"))
		{
			FString Row, Dev;
			Arg.Split(TEXT("/"), &Row, &Dev);
			W.Panel->PressCell(Pc::BindingRowIndex(TCHAR_TO_UTF8(*Row)), Dev == TEXT("pad") ? Pc::PcBindDevice::Pad : Pc::PcBindDevice::Keyboard);
		}
		else if (Op == TEXT("event"))
		{
			// Godot's panel takes an event only while a cell listens (its dialog's own key polling is off in the fixture):
			// one that nothing listens for goes nowhere here either (an Esc would close this dialog).
			if (W.Panel->IsListening())
			{
				Send(W, Arg);
			}
		}
		else if (Op == TEXT("tick"))
		{
			W.Panel->Advance(Hex(Arg));
		}
		else if (Op == TEXT("reset_row"))
		{
			W.Panel->PressReset(Pc::BindingRowIndex(TCHAR_TO_UTF8(*Arg)));
		}
		else if (Op == TEXT("reset_all"))
		{
			W.Panel->PressResetAll();
		}
		else if (Op == TEXT("check"))
		{
			for (int32 I = 0; I < Pc::ControlsCheckCount; ++I)
			{
				if (Arg == CheckNames[I])
				{
					W.Panel->ToggleCheck(static_cast<Pc::PcControlsCheck>(I));
				}
			}
		}
		else if (Op == TEXT("slider"))
		{
			FString Key, Value;
			Arg.Split(TEXT("="), &Key, &Value);
			W.Panel->MoveSlider(Key == TEXT("cursor_speed") ? Pc::PcControlsSlider::CursorSpeed : Pc::PcControlsSlider::StickDeadzone, VariantHex(Value));
		}
		else if (Op == TEXT("style"))
		{
			W.Settings->SetPromptStyle(Arg);
		}
		else if (Op == TEXT("bind"))
		{
			FString Row, E;
			Arg.Split(TEXT("/"), &Row, &E);
			const Pc::PcBindResult R = W.Panel->Bind(Pc::BindingRowIndex(TCHAR_TO_UTF8(*Row)), ParseEv(E));
			Errors += TestEqual(Where + TEXT(": result"), FString::FromInt(static_cast<int32>(R)), V[TEXT("result")]) ? 0 : 1;
		}
		++Moves;

		// --- what the page shows, as Godot's ---
		auto Same = [this, &Errors, &Where](const TCHAR* What, const FString& Got, const FString& Want)
		{
			if (Got != Want)
			{
				++Errors;
				if (Errors <= 40)
				{
					AddError(FString::Printf(TEXT("%s: %s is '%s', Godot '%s'"), *Where, What, *Got, *Want));
				}
			}
		};
		const Pc::PcControlsState& St = W.Panel->GetState();
		Same(TEXT("the status line"), W.Panel->GetStatusText().ToString(), Unescape(V[TEXT("status")]));
		Same(TEXT("listening"), St.bListening ? FString(UTF8_TO_TCHAR(St.ListenRow.c_str())) + (St.ListenDevice == Pc::PcBindDevice::Pad ? TEXT("/pad") : TEXT("/keyboard")) : FString(TEXT("-")),
			V[TEXT("listening")]);
		TArray<FString> Cells;
		V[TEXT("cells")].ParseIntoArray(Cells, TEXT(";"), false);
		for (int32 Row = 0; Row < Pc::BindingRowCount; ++Row)
		{
			for (int32 D = 0; D < 2; ++D)
			{
				const Pc::PcBindDevice Device = D == 0 ? Pc::PcBindDevice::Keyboard : Pc::PcBindDevice::Pad;
				TArray<FString> Cell;
				Cells[Row * 2 + D].ParseIntoArray(Cell, TEXT("|"), false);
				const UPawButton* B = W.Panel->GetCell(Row, Device);
				const UPawPromptIcon* Icon = W.Panel->GetCellPrompt(Row, Device);
				const FString Name = FString::Printf(TEXT("cell %s/%d"), UTF8_TO_TCHAR(Pc::BindingRowId(Row)), D);
				Same(*(Name + TEXT(" words")), B->GetLabel().ToString(), Unescape(Cell[0]));
				Same(*(Name + TEXT(" disabled")), B->IsLocked() ? TEXT("1") : TEXT("0"), Cell[1]);
				Same(*(Name + TEXT(" family")), FString::FromInt(Icon->FamilyOverride), Cell[2]);
				Same(*(Name + TEXT(" shows")), Icon->ShowsAnything() ? TEXT("1") : TEXT("0"), Cell[3]);
			}
		}
		TArray<FString> Resets;
		V[TEXT("resets")].ParseIntoArray(Resets, TEXT(","), false);
		for (int32 Row = 0; Row < Pc::BindingRowCount; ++Row)
		{
			Same(TEXT("a Reset"), W.Panel->GetReset(Row)->IsLocked() ? TEXT("1") : TEXT("0"), Resets[Row]);
		}
		TArray<FString> Checks;
		V[TEXT("checks")].ParseIntoArray(Checks, TEXT(","), false);
		for (int32 I = 0; I < Pc::ControlsCheckCount; ++I)
		{
			Same(CheckNames[I], W.Panel->GetCheck(static_cast<Pc::PcControlsCheck>(I))->IsTicked() ? TEXT("1") : TEXT("0"), Checks[I]);
		}
		const Pc::PcSettings& S = W.Settings->Get();
		Same(TEXT("the cursor speed"), FString::SanitizeFloat(S.CursorSpeed, 17), FString::SanitizeFloat(Hex(V[TEXT("cursor_speed")]), 17));
		Same(TEXT("the dead zone"), FString::SanitizeFloat(S.StickDeadzone, 17), FString::SanitizeFloat(Hex(V[TEXT("deadzone")]), 17));
		Same(TEXT("the cursor slider"), FString::SanitizeFloat(W.Panel->GetSlider(Pc::PcControlsSlider::CursorSpeed)->GetValue()),
			FString::SanitizeFloat(static_cast<float>(Hex(V[TEXT("cursor_slider")]))));
		Same(TEXT("the dead zone slider"), FString::SanitizeFloat(W.Panel->GetSlider(Pc::PcControlsSlider::StickDeadzone)->GetValue()),
			FString::SanitizeFloat(static_cast<float>(Hex(V[TEXT("deadzone_slider")]))));
		Same(TEXT("the cursor line"), W.Panel->GetSliderLine(Pc::PcControlsSlider::CursorSpeed).ToString(), Unescape(V[TEXT("cursor_line")]));
		Same(TEXT("the dead zone line"), W.Panel->GetSliderLine(Pc::PcControlsSlider::StickDeadzone).ToString(), Unescape(V[TEXT("deadzone_line")]));
		Same(TEXT("the controller column"), FString::FromInt(static_cast<int32>(Pc::ControlsPadColumnFamily(S.PromptStyle, W.Device->GetFamily(),
			W.Device->GetState().Detected))), V[TEXT("pad_column")]);
		Same(TEXT("the family"), FString::FromInt(static_cast<int32>(W.Device->GetFamily())), V[TEXT("family")]);
		Same(TEXT("the bindings"), SavedText(S.Bindings), V[TEXT("saved")]);
	}
	AddInfo(FString::Printf(TEXT("%d moves over %d rows; %d differences"), Moves, Rows, Errors));
	TestEqual(TEXT("no difference from Godot"), Errors, 0);
	TestTrue(TEXT("every kind of move"), Ops.FindRef(TEXT("event")) > 200 && Ops.FindRef(TEXT("listen")) > 100 && Ops.FindRef(TEXT("bind")) > 50
		&& Ops.FindRef(TEXT("tick")) > 30 && Ops.FindRef(TEXT("reset_row")) > 20 && Ops.FindRef(TEXT("reset_all")) > 5
		&& Ops.FindRef(TEXT("slider")) > 20 && Ops.FindRef(TEXT("check")) > 20 && Ops.FindRef(TEXT("style")) > 10);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawControlsDialogTest, "Pawpocalypse.UI.Controls.Dialog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawControlsDialogTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FControlsWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	UPawControlsPanel* P = W.Panel;
	// Every part, Godot's words.
	for (int32 I = 0; I < Pc::ControlsCheckCount; ++I)
	{
		TestNotNull(TEXT("a switch"), P->GetCheck(static_cast<Pc::PcControlsCheck>(I)));
	}
	TestNotNull(TEXT("Reset all"), P->GetResetAll());
	TestNotNull(TEXT("the cursor slider"), P->GetSlider(Pc::PcControlsSlider::CursorSpeed));
	TestNotNull(TEXT("the dead zone slider"), P->GetSlider(Pc::PcControlsSlider::StickDeadzone));
	TestEqual(TEXT("a row's words"), P->GetRowLabel(1)->GetText().ToString(), FString(TEXT("Previous tab")));
	TestEqual(TEXT("Reset all's words"), P->GetResetAll()->GetLabel().ToString(), FString(TEXT("Reset all controls")));
	TestTrue(TEXT("Pause cannot be rebound"), P->GetCell(9, Pc::PcBindDevice::Keyboard)->IsLocked() && P->GetCell(9, Pc::PcBindDevice::Pad)->IsLocked()
		&& P->GetReset(9)->IsLocked());
	TestEqual(TEXT("Re-centre on the keyboard: nothing"), P->GetCell(8, Pc::PcBindDevice::Keyboard)->GetLabel().ToString(), FString(TEXT("—")));
	TestTrue(TEXT("Pounce on the keyboard is the mouse's glyph"), P->GetCellPrompt(0, Pc::PcBindDevice::Keyboard)->ShowsAnything());

	// Listening: Esc cancels it and the dialog stays; LB does not turn the page.
	P->PressCell(3, Pc::PcBindDevice::Keyboard);
	TestTrue(TEXT("Up listens"), P->IsListening());
	TestEqual(TEXT("the cell says so"), P->GetCell(3, Pc::PcBindDevice::Keyboard)->GetLabel().ToString(), FString(TEXT("Press…")));
	TestEqual(TEXT("and the line"), P->GetStatusText().ToString(), FString(TEXT("Press a key or button for Up. Esc or B cancels.")));
	W.Key(EKeys::Gamepad_LeftShoulder, true);
	TestTrue(TEXT("LB was swallowed: still the Controls page"), W.Dialog->IsPageShown(Pc::PcSettingsSection::Controls) && P->IsListening());
	W.Key(EKeys::Escape, true);
	TestFalse(TEXT("Esc cancelled"), P->IsListening());
	TestTrue(TEXT("and the dialog is still up"), W.Dialog->IsActivated());
	TestEqual(TEXT("Cancelled."), P->GetStatusText().ToString(), FString(TEXT("Cancelled.")));
	// A key bound through the dialog reaches the game's map.
	P->PressCell(3, Pc::PcBindDevice::Keyboard);
	W.Key(EKeys::I, true);
	TestEqual(TEXT("Done."), P->GetStatusText().ToString(), FString(TEXT("Done.")));
	TestTrue(TEXT("I is up now"), PawInput::KeyIs(EKeys::I, TEXT("menu_up")) && PawInput::KeyIs(EKeys::I, TEXT("ui_up")));
	TestEqual(TEXT("the cell draws I"), P->GetCellPrompt(3, Pc::PcBindDevice::Keyboard)->GetShown().Label.ToString(), FString(TEXT("I")));
	W.Key(EKeys::I, false); // its release: the page's no more
	// A click cancels a controller cell; a trigger binds past half way only.
	P->PressCell(0, Pc::PcBindDevice::Pad);
	W.Click(EKeys::LeftMouseButton);
	TestEqual(TEXT("a click cancels a controller cell"), P->GetStatusText().ToString(), FString(TEXT("Cancelled.")));
	P->PressCell(6, Pc::PcBindDevice::Pad);
	W.Key(EKeys::Gamepad_LeftTrigger, true); // the trigger's digital stand-in: swallowed
	W.Analog(EKeys::Gamepad_LeftTriggerAxis, 0.3f);
	TestTrue(TEXT("a light pull: still listening"), P->IsListening());
	W.Analog(EKeys::Gamepad_LeftTriggerAxis, 0.7f);
	TestEqual(TEXT("past half way: the zoom has it"), P->GetStatusText().ToString(), FString(TEXT("That one is taken.")));
	// Start is a system key.
	P->PressCell(5, Pc::PcBindDevice::Pad);
	W.Key(EKeys::Gamepad_Special_Right, true);
	TestEqual(TEXT("Start refused"), P->GetStatusText().ToString(), FString(TEXT("Esc, Start and B always open the menu and go back.")));
	// Five seconds of nothing.
	P->PressCell(2, Pc::PcBindDevice::Keyboard);
	P->Advance(4.9);
	TestTrue(TEXT("4.9 s: listening"), P->IsListening());
	P->Advance(0.2);
	TestEqual(TEXT("5 s: nothing changed"), P->GetStatusText().ToString(), FString(TEXT("Nothing pressed, nothing changed.")));
	// Reset all.
	P->PressResetAll();
	TestTrue(TEXT("Reset all: W is up again"), PawInput::KeyIs(EKeys::W, TEXT("menu_up")) && !PawInput::KeyIs(EKeys::I, TEXT("menu_up")));
	TestEqual(TEXT("its line"), P->GetStatusText().ToString(), FString(TEXT("Every control is back to its default.")));
	// Not listening: Esc closes the dialog as before.
	W.Key(EKeys::Escape, true);
	TestFalse(TEXT("Esc closes the dialog when nothing listens"), W.Dialog->IsActivated());
	W.Close();
	return true;
}

#endif
