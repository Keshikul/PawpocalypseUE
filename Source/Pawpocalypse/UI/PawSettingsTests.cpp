// Pawpocalypse.UI.Settings.*: the settings popup (UPawSettingsDialog, what ships: WBP_SettingsDialog) over the settings
// subsystem, on a sandboxed storage (memory). Its rules are PcSettings, proven against the real settings_manager.gd and
// settings_dialog.tscn in PawCore (PcSettingsTests, fixture `settings`); these prove the dialog and the subsystem carry them:
//
// Pawpocalypse.UI.Settings.MatchesGodot  300 fixture rows: the settings.cfg GODOT wrote, read from the storage; every change
//                                        made on the widget; every setting, volume line, slider, box, list and page as Godot's.
// Pawpocalypse.UI.Settings.Look          every part wired; Godot's words; the pages' selection rows; Language disabled.
// Pawpocalypse.UI.Settings.FromPause     the pause menu steps aside for it and comes back (the field held throughout).
// Pawpocalypse.UI.Settings.FromTitle     over the title; closing brings the selection home to Settings.
// Pawpocalypse.UI.Settings.Persists      a change writes the file; a new game instance reads it back; an unreadable file is the
//                                        defaults; the last slot used follows saves and loads and is where the next run starts.
// Pawpocalypse.UI.Settings.Applies       the menu side moves the HUD's dock, the Reawaken box writes the setting, the video
//                                        settings reach the applier (never the editor's engine), Effects off is no budget.
//
// Negative tests: tools/sabotage_realm.py (`settings_*`).

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/PawAudioSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PcProgress.h"
#include "PcSettings.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "Save/PawSaveSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawButton.h"
#include "UI/PawHudScreen.h"
#include "UI/PawMainMenu.h"
#include "UI/PawPauseMenu.h"
#include "UI/PawRootLayout.h"
#include "UI/PawSettingsDialog.h"
#include "UI/PawWidgets.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/PawTextTesting.h"

#include <cstdlib>
#include <string>

namespace
{
	using Slider = Pc::PcSettingsSlider;

	struct FSettingsWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawGameSubsystem* Game = nullptr;
		UPawRealmSubsystem* Realm = nullptr;
		UPawSaveSubsystem* Save = nullptr;
		UPawSettingsSubsystem* Settings = nullptr;
		UPawAudioSubsystem* Audio = nullptr;
		TArray<FString> SystemDevices; // the system's sound devices, by name ("Default" is always there)
		TArray<FString> Swaps;         // what the engine would have been told
		UPawRootLayout* Root = nullptr;
		UPawHudScreen* Hud = nullptr;
		TSharedPtr<SWidget> RootSlate;
		TSharedPtr<FPawMemorySaveStorage> Disk;

		// bHud: the realm, the root layout and the HUD (the pause menu's way in). Disk: a storage to share with an earlier
		// world (a second run on the same machine).
		bool Open(FAutomationTestBase& T, bool bHud, TSharedPtr<FPawMemorySaveStorage> InDisk = nullptr)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			Game = GI->GetSubsystem<UPawGameSubsystem>();
			Save = GI->GetSubsystem<UPawSaveSubsystem>();
			Settings = GI->GetSubsystem<UPawSettingsSubsystem>();
			if (!T.TestNotNull(TEXT("the realm"), Realm) || !T.TestNotNull(TEXT("the game"), Game) || !T.TestNotNull(TEXT("the saves"), Save)
				|| !T.TestNotNull(TEXT("the settings"), Settings) || !T.TestTrue(TEXT("data"), Game->IsDataReady()))
			{
				return false;
			}
			T.TestFalse(TEXT("an automated run has no settings storage of its own (the sandbox)"), Settings->IsStored());
			// The system's sound devices: none but "Default" (Godot's fixture ran headless, its list is that), never this
			// machine's; no swap reaches the engine.
			Audio = GI->GetSubsystem<UPawAudioSubsystem>();
			if (!T.TestNotNull(TEXT("the audio"), Audio))
			{
				return false;
			}
			Audio->SystemDevices = [this]() { return SystemDevices; };
			Audio->SwapOutput = [this](const FString& Name) { Swaps.Add(Name); return true; };
			Disk = InDisk.IsValid() ? InDisk : MakeShared<FPawMemorySaveStorage>();
			Settings->UseStorageForTest(Disk); // the settings first: the saves start on their last slot
			Save->UseStorageForTest(Disk);
			Game->NewGame();
			Root = CreateWidget<UPawRootLayout>(GI, PawUi::Resolve<UPawRootLayout>());
			if (!T.TestNotNull(TEXT("root"), Root))
			{
				return false;
			}
			RootSlate = Root->TakeWidget();
			if (bHud)
			{
				Realm->SetViewSize(1920.0, 1080.0);
				if (!T.TestTrue(TEXT("the realm starts"), Realm->StartRealm(77)))
				{
					return false;
				}
				Hud = CreateWidget<UPawHudScreen>(GI, PawUi::Resolve<UPawHudScreen>());
				if (!T.TestNotNull(TEXT("HUD"), Hud))
				{
					return false;
				}
				Hud->SetMenuStack(Root->MenuStack);
				Hud->SetModalStack(Root->ModalStack);
				GFrameCounter += 2;
			}
			return true;
		}

		void Close()
		{
			RootSlate.Reset();
			if (Settings)
			{
				Settings->UseStorageForTest(nullptr);
			}
			if (Save)
			{
				Save->UseStorageForTest(nullptr);
			}
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

		void PutFile(const std::string& Text)
		{
			Disk->Put(UPawSettingsSubsystem::EntryName, TArray<uint8>(reinterpret_cast<const uint8*>(Text.data()), static_cast<int32>(Text.size())));
		}

		bool ReadFile(Pc::PcSettings& Out)
		{
			TArray<uint8> Bytes;
			if (!Disk->Read(UPawSettingsSubsystem::EntryName, Bytes))
			{
				return false;
			}
			return Pc::SettingsLoad(std::string(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<size_t>(Bytes.Num())), Out);
		}

		UPawSettingsDialog* OpenDialog(FAutomationTestBase& T)
		{
			UPawSettingsDialog* D = UPawSettingsDialog::Open(Root->ModalStack, nullptr);
			return T.TestTrue(TEXT("the dialog is up"), D && D->IsActivated()) ? D : nullptr;
		}
	};

	FString S(const FText& T) { return T.ToString(); }

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
	FString Flag(bool b) { return b ? TEXT("1") : TEXT("0"); }
	bool Ticked(const UPawButton* B) { return B && B->IsTicked(); }

	const TCHAR* SectionName(Pc::PcSettingsSection Sec)
	{
		return Sec == Pc::PcSettingsSection::Audio ? TEXT("Audio") : Sec == Pc::PcSettingsSection::Video ? TEXT("Video") : TEXT("Controls");
	}

	FString Options(const UComboBoxString* C)
	{
		FString Out;
		for (int32 I = 0; C && I < C->GetOptionCount(); ++I)
		{
			Out += (I ? TEXT("|") : TEXT("")) + C->GetOptionAtIndex(I);
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSettingsGodotTest, "Pawpocalypse.UI.Settings.MatchesGodot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSettingsGodotTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FString Text;
	const FString Path = FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data/settings_godot.tsv");
	if (!TestTrue(TEXT("the fixture"), FFileHelper::LoadFileToString(Text, *Path)))
	{
		return false;
	}
	FSettingsWorld W;
	if (!W.Open(*this, false))
	{
		W.Close();
		return false;
	}
	// One dialog for every row, as in the fixture (its section survives from row to row).
	UPawSettingsDialog* D = CreateWidget<UPawSettingsDialog>(W.GI, PawUi::Resolve<UPawSettingsDialog>());
	if (!TestNotNull(TEXT("the dialog"), D))
	{
		W.Close();
		return false;
	}
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines);
	TArray<FString> Header;
	int32 Rows = 0, Changes = 0, Mismatch = 0, WritesSeen = 0;
	for (const FString& Line : Lines)
	{
		if (Line.StartsWith(TEXT("#")))
		{
			continue;
		}
		TArray<FString> Cells;
		Line.ParseIntoArray(Cells, TEXT("\t"), false);
		if (Header.Num() == 0)
		{
			Header = Cells;
			continue;
		}
		TMap<FString, FString> V;
		for (int32 I = 0; I < Header.Num() && I < Cells.Num(); ++I)
		{
			V.Add(Header[I], Cells[I]);
		}
		const FString Op = V[TEXT("op")];
		if (Op == TEXT("load"))
		{
			if (Rows == 300)
			{
				break;
			}
			++Rows;
			// The file Godot wrote, as the subsystem finds it at startup; the dialog opens on it.
			if (D->IsActivated())
			{
				D->DeactivateWidget();
			}
			W.PutFile(std::string(TCHAR_TO_UTF8(*Unescape(V[TEXT("cfg")]))));
			W.Settings->Reload();
			D->ActivateWidget();
		}
		else
		{
			++Changes;
			const int32 Writes = W.Settings->GetWrites();
			const FString& Arg = V[TEXT("arg")];
			if (Op == TEXT("vol")) { D->MoveSlider(Slider::Master, Hex(Arg)); }
			else if (Op == TEXT("music")) { D->MoveSlider(Slider::Music, Hex(Arg)); }
			else if (Op == TEXT("sfx")) { D->MoveSlider(Slider::Sfx, Hex(Arg)); }
			else if (Op == TEXT("mute")) { D->ToggleMute(); }
			else if (Op == TEXT("effects")) { D->ToggleEffects(); }
			else if (Op == TEXT("quality")) { D->PickQuality(FCString::Atoi(*Arg)); }
			else if (Op == TEXT("fps")) { D->PickFps(FCString::Atoi(*Arg)); }
			else if (Op == TEXT("prompts")) { D->PickPrompts(FCString::Atoi(*Arg)); }
			else if (Op == TEXT("dock")) { D->PickDock(FCString::Atoi(*Arg)); }
			else if (Op == TEXT("section")) { D->StepSection(FCString::Atoi(*Arg)); }
			else if (Op == TEXT("reopen")) { D->DeactivateWidget(); D->ActivateWidget(); }
			WritesSeen += W.Settings->GetWrites() > Writes ? 1 : 0;
		}
		auto Same = [&](const TCHAR* What, const FString& Got, const FString& Want)
		{
			if (Got != Want && ++Mismatch <= 10)
			{
				AddError(FString::Printf(TEXT("row %s (%s %s) %s: '%s' vs Godot '%s'"), *V[TEXT("row")], *Op, *V[TEXT("arg")], What, *Got, *Want));
			}
		};
		const Pc::PcSettings& P = W.Settings->Get();
		auto H = [](double X) { return FString::Printf(TEXT("%.17g"), X); };
		Same(TEXT("master"), H(P.MasterVolume), H(Hex(V[TEXT("master")])));
		Same(TEXT("music"), H(P.MusicVolume), H(Hex(V[TEXT("music")])));
		Same(TEXT("sfx"), H(P.SfxVolume), H(Hex(V[TEXT("sfx")])));
		Same(TEXT("muted"), Flag(P.bMuted), V[TEXT("muted")]);
		Same(TEXT("output"), UTF8_TO_TCHAR(P.OutputDevice.c_str()), Unescape(V[TEXT("output")]));
		Same(TEXT("fullscreen"), Flag(P.bFullscreen), V[TEXT("fullscreen")]);
		Same(TEXT("vsync"), Flag(P.bVsync), V[TEXT("vsync")]);
		Same(TEXT("quality"), FString::Printf(TEXT("%lld"), static_cast<long long>(P.GraphicsQuality)), V[TEXT("quality")]);
		Same(TEXT("max_fps"), FString::Printf(TEXT("%lld"), static_cast<long long>(P.MaxFps)), V[TEXT("max_fps")]);
		Same(TEXT("effects"), Flag(P.bEffectsEnabled), V[TEXT("effects")]);
		Same(TEXT("last_slot"), FString::Printf(TEXT("%lld"), static_cast<long long>(P.LastSlot)), V[TEXT("last_slot")]);
		Same(TEXT("res_index"), FString::Printf(TEXT("%lld"), static_cast<long long>(P.ResolutionIndex)), V[TEXT("res_index")]);
		Same(TEXT("confirm"), Flag(P.bConfirmReawaken), V[TEXT("confirm")]);
		Same(TEXT("prompt"), UTF8_TO_TCHAR(P.PromptStyle.c_str()), V[TEXT("prompt")]);
		Same(TEXT("dock"), UTF8_TO_TCHAR(P.DockSide.c_str()), V[TEXT("dock")]);
		Same(TEXT("deadzone"), H(P.StickDeadzone), H(Hex(V[TEXT("deadzone")])));
		Same(TEXT("budget"), H(W.Settings->EffectBudget()), H(Hex(V[TEXT("budget")])));
		// The dialog, as drawn.
		Same(TEXT("volume line"), S(D->GetVolumeLine(Slider::Master)), V[TEXT("vol_label")]);
		Same(TEXT("music line"), S(D->GetVolumeLine(Slider::Music)), V[TEXT("music_label")]);
		Same(TEXT("sfx line"), S(D->GetVolumeLine(Slider::Sfx)), V[TEXT("sfx_label")]);
		// A USlider holds a float: Godot's double, as a float.
		Same(TEXT("volume slider"), H(D->GetSliderValue(Slider::Master)), H(static_cast<float>(Hex(V[TEXT("vol_slider")]))));
		Same(TEXT("music slider"), H(D->GetSliderValue(Slider::Music)), H(static_cast<float>(Hex(V[TEXT("music_slider")]))));
		Same(TEXT("sfx slider"), H(D->GetSliderValue(Slider::Sfx)), H(static_cast<float>(Hex(V[TEXT("sfx_slider")]))));
		Same(TEXT("mute box"), Flag(Ticked(D->GetCheck(TEXT("MuteCheck")))), V[TEXT("mute_check")]);
		Same(TEXT("fullscreen box"), Flag(Ticked(D->GetCheck(TEXT("FullscreenCheck")))), V[TEXT("full_check")]);
		Same(TEXT("vsync box"), Flag(Ticked(D->GetCheck(TEXT("VsyncCheck")))), V[TEXT("vsync_check")]);
		Same(TEXT("effects box"), Flag(Ticked(D->GetCheck(TEXT("EffectsCheck")))), V[TEXT("effects_check")]);
		const UComboBoxString* Res = D->GetOption(TEXT("ResolutionOption"));
		Same(TEXT("resolution"), Res ? FString::FromInt(Res->GetSelectedIndex()) : TEXT("?"), V[TEXT("res_sel")]);
		Same(TEXT("resolution text"), Res ? Res->GetSelectedOption() : TEXT("?"), V[TEXT("res_text")]);
		Same(TEXT("resolution disabled"), Flag(Res && !Res->GetIsEnabled()), V[TEXT("res_disabled")]);
		const UComboBoxString* Fps = D->GetOption(TEXT("MaxFpsOption"));
		Same(TEXT("fps"), Fps ? FString::FromInt(Fps->GetSelectedIndex()) : TEXT("?"), V[TEXT("fps_sel")]);
		Same(TEXT("fps text"), Fps ? Fps->GetSelectedOption() : TEXT("?"), V[TEXT("fps_text")]);
		const UComboBoxString* Quality = D->GetOption(TEXT("QualityOption"));
		Same(TEXT("quality list"), Quality ? FString::FromInt(Quality->GetSelectedIndex()) : TEXT("?"), V[TEXT("quality_sel")]);
		const UComboBoxString* Prompts = D->GetOption(TEXT("PromptsOption"));
		Same(TEXT("prompts list"), Prompts ? FString::FromInt(Prompts->GetSelectedIndex()) : TEXT("?"), V[TEXT("prompts_sel")]);
		const UComboBoxString* Dock = D->GetOption(TEXT("DockSideOption"));
		Same(TEXT("dock list"), Dock ? FString::FromInt(Dock->GetSelectedIndex()) : TEXT("?"), V[TEXT("dock_sel")]);
		const UComboBoxString* Output = D->GetOption(TEXT("OutputOption"));
		Same(TEXT("output items"), Options(Output), Unescape(V[TEXT("output_items")]));
		Same(TEXT("output"), Output ? FString::FromInt(Output->GetSelectedIndex()) : TEXT("?"), V[TEXT("output_sel")]);
		FString Shown;
		for (int32 I = 0; I < 3; ++I)
		{
			const Pc::PcSettingsSection Sec = static_cast<Pc::PcSettingsSection>(I);
			Shown += D->IsPageShown(Sec) ? SectionName(Sec) : TEXT("");
		}
		Same(TEXT("page"), Shown, V[TEXT("section")]);
	}
	// What the subsystem wrote is what it holds.
	Pc::PcSettings File;
	if (TestTrue(TEXT("the file is there"), W.ReadFile(File)))
	{
		TestEqual(TEXT("the file holds the volume"), File.MasterVolume, W.Settings->Get().MasterVolume);
		TestEqual(TEXT("and the menu side"), File.DockSide, W.Settings->Get().DockSide);
	}
	TestEqual(TEXT("rows"), Rows, 300);
	TestEqual(TEXT("every line as Godot's"), Mismatch, 0);
	TestTrue(TEXT("changes were written"), WritesSeen > 500);
	AddInfo(FString::Printf(TEXT("%d rows, %d changes, %d written"), Rows, Changes, WritesSeen));
	if (D->IsActivated())
	{
		D->DeactivateWidget();
	}
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSettingsLookTest, "Pawpocalypse.UI.Settings.Look", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSettingsLookTest::RunTest(const FString& Parameters)
{
	// Godot's English for this run (UI/PawTextTesting.h): it compares with Godot's words; the port's own English is
	// Pawpocalypse.UI.Text.PortWords' (Data/TextOverrides.csv).
	const PawTextTesting::FGodotWords GodotWords(*this);
	FSettingsWorld W;
	if (!W.Open(*this, false))
	{
		W.Close();
		return false;
	}
	UPawSettingsDialog* D = W.OpenDialog(*this);
	if (!D)
	{
		W.Close();
		return false;
	}
	const TCHAR* Parts[] = { TEXT("SectionAudio"), TEXT("SectionVideo"), TEXT("SectionControls"), TEXT("VolumeSlider"), TEXT("MusicSlider"),
		TEXT("SfxSlider"), TEXT("MuteCheck"), TEXT("OutputOption"), TEXT("LanguageOption"), TEXT("ResolutionOption"), TEXT("FullscreenCheck"),
		TEXT("VsyncCheck"), TEXT("QualityOption"), TEXT("MaxFpsOption"), TEXT("EffectsCheck"), TEXT("VideoNote"), TEXT("PromptsOption"),
		TEXT("PromptsNote"), TEXT("DockSideOption"), TEXT("DockSideNote"), TEXT("CloseButton") };
	for (const TCHAR* P : Parts)
	{
		if (!TestNotNull(FString::Printf(TEXT("%s is wired"), P), D->GetPart(P)))
		{
			W.Close();
			return false;
		}
	}
	TestEqual(TEXT("the title"), S(D->GetTitleText()), FString(TEXT("SETTINGS")));
	TestEqual(TEXT("Audio"), S(D->GetCheck(TEXT("SectionAudio"))->GetLabel()), FString(TEXT("Audio")));
	TestEqual(TEXT("Video"), S(D->GetCheck(TEXT("SectionVideo"))->GetLabel()), FString(TEXT("Video")));
	TestEqual(TEXT("Controls"), S(D->GetCheck(TEXT("SectionControls"))->GetLabel()), FString(TEXT("Controls")));
	TestEqual(TEXT("Close"), S(D->GetCheck(TEXT("CloseButton"))->GetLabel()), FString(TEXT("Close")));
	TestEqual(TEXT("the defaults: master"), S(D->GetVolumeLine(Slider::Master)), FString(TEXT("Master Volume: 100%")));
	TestEqual(TEXT("music"), S(D->GetVolumeLine(Slider::Music)), FString(TEXT("Music  45%")));
	TestEqual(TEXT("sound effects"), S(D->GetVolumeLine(Slider::Sfx)), FString(TEXT("Sound Effects  100%")));
	TestEqual(TEXT("the mute box"), S(D->GetCheck(TEXT("MuteCheck"))->GetLabel()), FString(TEXT("Mute all audio")));
	TestEqual(TEXT("the output"), Options(D->GetOption(TEXT("OutputOption"))), FString(TEXT("Follow the system")));
	TestEqual(TEXT("the resolutions"), Options(D->GetOption(TEXT("ResolutionOption"))),
		FString(TEXT("1280 x 720|1600 x 900|1920 x 1080|2560 x 1440|3840 x 2160")));
	TestEqual(TEXT("1920 x 1080 by default"), D->GetOption(TEXT("ResolutionOption"))->GetSelectedIndex(), 2);
	TestEqual(TEXT("the qualities"), Options(D->GetOption(TEXT("QualityOption"))), FString(TEXT("Low|Medium|High")));
	TestEqual(TEXT("the caps"), Options(D->GetOption(TEXT("MaxFpsOption"))), FString(TEXT("Unlimited|30 FPS|60 FPS|120 FPS|144 FPS|240 FPS")));
	TestEqual(TEXT("the prompts"), Options(D->GetOption(TEXT("PromptsOption"))),
		FString(TEXT("Auto|Keyboard and mouse|Xbox|PlayStation|Steam Deck")));
	TestEqual(TEXT("the sides"), Options(D->GetOption(TEXT("DockSideOption"))), FString(TEXT("Left|Right")));
	TestEqual(TEXT("the language: System, until the languages (phase 7)"), Options(D->GetOption(TEXT("LanguageOption"))), FString(TEXT("System")));
	TestFalse(TEXT("and disabled"), D->GetOption(TEXT("LanguageOption"))->GetIsEnabled());
	TestTrue(TEXT("V-Sync on by default"), Ticked(D->GetCheck(TEXT("VsyncCheck"))));
	TestTrue(TEXT("effects on by default"), Ticked(D->GetCheck(TEXT("EffectsCheck"))));
	// The selection: the section bar, the page's controls one per row, Close.
	TestTrue(TEXT("Audio first"), D->IsPageShown(Pc::PcSettingsSection::Audio) && !D->IsPageShown(Pc::PcSettingsSection::Video));
	TestEqual(TEXT("Audio: 3 sections, 3 sliders, the box, the output, Close"), D->GetWired().Num(), 9);
	TestTrue(TEXT("opens on the master slider"), D->GetDesiredFocusTarget() == D->GetPart(TEXT("VolumeSlider")));
	D->StepSection(1);
	TestTrue(TEXT("RB: Video"), D->IsPageShown(Pc::PcSettingsSection::Video));
	TestEqual(TEXT("Video without the disabled language: 3 + 6 + Close"), D->GetWired().Num(), 10);
	TestTrue(TEXT("opens on Resolution"), D->GetDesiredFocusTarget() == D->GetPart(TEXT("ResolutionOption")));
	D->StepSection(1);
	// The Controls panel's rows (controls_panel.gd focus_rows): 2 boxes, 10 rows x [keyboard, controller, Reset], Reset all,
	// 4 boxes, 2 sliders.
	TestEqual(TEXT("Controls: 3 + 2 + the panel's 39 + Close"), D->GetWired().Num(), 45);
	D->StepSection(1);
	TestTrue(TEXT("it wraps to Audio"), D->IsPageShown(Pc::PcSettingsSection::Audio));
	D->StepSection(-1);
	TestTrue(TEXT("LB: back to Controls"), D->IsPageShown(Pc::PcSettingsSection::Controls));
	D->PressSection(Pc::PcSettingsSection::Video);
	TestTrue(TEXT("a section button"), D->IsPageShown(Pc::PcSettingsSection::Video));
	// R4a: the open section is "the selected thing", on the Butter plate; the others are not.
	TestTrue(TEXT("the open section wears the call look"), D->GetCheck(TEXT("SectionVideo"))->HasCallLook()
		&& !D->GetCheck(TEXT("SectionAudio"))->HasCallLook() && !D->GetCheck(TEXT("SectionControls"))->HasCallLook());
	D->PressClose();
	TestFalse(TEXT("Close closes"), D->IsActivated());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSettingsOutputTest, "Pawpocalypse.UI.Settings.Output", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSettingsOutputTest::RunTest(const FString& Parameters)
{
	// Settings > Audio > Output (settings_dialog.gd _fill_output_option, settings_manager.gd set_output_device): the system's
	// devices after "Follow the system", refilled on every open; a pick plays there at once; a device gone since it was
	// chosen stays on the list, chosen, while the sound goes to the system's.
	FSettingsWorld W;
	if (!W.Open(*this, false))
	{
		W.Close();
		return false;
	}
	const FString Speakers = TEXT("Speakers (Realtek(R) Audio)"), Headset = TEXT("Headset Earphone (Wireless Controller)");
	W.SystemDevices = { Speakers, Headset };
	UPawSettingsDialog* D = W.OpenDialog(*this);
	if (!D)
	{
		W.Close();
		return false;
	}
	UComboBoxString* Output = D->GetOption(TEXT("OutputOption"));
	TestEqual(TEXT("the system's devices after Follow the system"), Options(Output),
		FString(TEXT("Follow the system|Speakers (Realtek(R) Audio)|Headset Earphone (Wireless Controller)")));
	TestEqual(TEXT("Follow the system by default"), Output->GetSelectedIndex(), 0);
	TestEqual(TEXT("nothing told at startup"), W.Swaps.Num(), 0);
	D->PickOutput(1);
	TestEqual(TEXT("stored by name"), FString(UTF8_TO_TCHAR(W.Settings->Get().OutputDevice.c_str())), Speakers);
	TestEqual(TEXT("and played there at once"), W.Swaps, TArray<FString>{ Speakers });
	TestEqual(TEXT("shown chosen"), Output->GetSelectedIndex(), 1);
	D->MoveSlider(Pc::PcSettingsSlider::Music, 0.3);
	TestEqual(TEXT("another setting (apply_audio again) tells the engine nothing new"), W.Swaps.Num(), 1);
	D->PressClose();
	// Unplugged: the next open still lists it, chosen (so plugging it back in finds it).
	W.SystemDevices = { Headset };
	W.Audio->ApplyOutputDevice();
	TestEqual(TEXT("gone: the system's device, never silence"), W.Audio->GetAppliedOutputDevice(), FString(TEXT("Default")));
	TestEqual(TEXT("the setting is kept"), FString(UTF8_TO_TCHAR(W.Settings->Get().OutputDevice.c_str())), Speakers);
	D = W.OpenDialog(*this);
	if (!D)
	{
		W.Close();
		return false;
	}
	Output = D->GetOption(TEXT("OutputOption"));
	TestEqual(TEXT("refilled: the gone device last"), Options(Output),
		FString(TEXT("Follow the system|Headset Earphone (Wireless Controller)|Speakers (Realtek(R) Audio)")));
	TestEqual(TEXT("and chosen"), Output->GetSelectedIndex(), 2);
	D->PickOutput(0);
	TestEqual(TEXT("Follow the system: stored as Default"), FString(UTF8_TO_TCHAR(W.Settings->Get().OutputDevice.c_str())), FString(TEXT("Default")));
	TestEqual(TEXT("already following it: nothing more told"), W.Swaps, (TArray<FString>{ Speakers, TEXT("Default") }));
	D->PressClose();
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSettingsFromPauseTest, "Pawpocalypse.UI.Settings.FromPause", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSettingsFromPauseTest::RunTest(const FString& Parameters)
{
	FSettingsWorld W;
	if (!W.Open(*this, true))
	{
		W.Close();
		return false;
	}
	if (!TestTrue(TEXT("the pause menu opens"), W.Hud->OpenPauseMenu()))
	{
		W.Close();
		return false;
	}
	UPawPauseMenu* Pause = W.Hud->GetPauseMenu();
	TestTrue(TEXT("Settings is live"), Pause->GetButton(TEXT("SettingsButton"))->GetIsEnabled());
	Pause->PressSettings();
	UPawSettingsDialog* D = Cast<UPawSettingsDialog>(W.Root->ModalStack->GetActiveWidget());
	if (!TestTrue(TEXT("the dialog is up on the Modal stack"), D && D->IsActivated()))
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("the pause menu stepped aside"), Pause->IsSettingsUp() && Pause->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("still up underneath"), Pause->IsActivated());
	TestTrue(TEXT("the field held"), W.Realm->IsPaused());
	TestFalse(TEXT("no second pause menu over it"), W.Hud->OpenPauseMenu());
	D->PressClose();
	TestFalse(TEXT("the dialog is gone"), D->IsActivated());
	TestFalse(TEXT("the pause menu is back"), Pause->IsSettingsUp() || Pause->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("on its main page"), Pause->IsActivated() && Pause->IsPageShown(false));
	TestTrue(TEXT("the field still held: the player did not press Continue"), W.Realm->IsPaused());
	Pause->PressContinue();
	TestFalse(TEXT("Continue lets it go"), W.Realm->IsPaused());
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSettingsFromTitleTest, "Pawpocalypse.UI.Settings.FromTitle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSettingsFromTitleTest::RunTest(const FString& Parameters)
{
	FSettingsWorld W;
	if (!W.Open(*this, false))
	{
		W.Close();
		return false;
	}
	UPawMainMenu* Menu = CreateWidget<UPawMainMenu>(W.GI, PawUi::Resolve<UPawMainMenu>());
	if (!TestNotNull(TEXT("the title"), Menu))
	{
		W.Close();
		return false;
	}
	Menu->SetModalStack(W.Root->ModalStack);
	Menu->Refresh();
	TestTrue(TEXT("Settings is live on the title"), Menu->GetMenuButton(4)->GetIsEnabled());
	Menu->PressSettings();
	UPawSettingsDialog* D = Cast<UPawSettingsDialog>(W.Root->ModalStack->GetActiveWidget());
	if (!TestTrue(TEXT("the dialog is up"), D && D->IsActivated()))
	{
		W.Close();
		return false;
	}
	D->PressClose();
	TestTrue(TEXT("closing brings the selection home to Settings"), Menu->GetTopDefault() == Menu->GetMenuButton(4));
	TestEqual(TEXT("the Modal stack is empty"), W.Root->ModalStack->GetNumWidgets(), 0);
	W.Close();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSettingsPersistsTest, "Pawpocalypse.UI.Settings.Persists", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSettingsPersistsTest::RunTest(const FString& Parameters)
{
	TSharedPtr<FPawMemorySaveStorage> Disk = MakeShared<FPawMemorySaveStorage>();
	{
		FSettingsWorld W;
		if (!W.Open(*this, false, Disk))
		{
			W.Close();
			return false;
		}
		TestFalse(TEXT("nothing written by opening"), Disk->Exists(UPawSettingsSubsystem::EntryName));
		UPawSettingsDialog* D = W.OpenDialog(*this);
		if (!D)
		{
			W.Close();
			return false;
		}
		TestFalse(TEXT("nor by the dialog opening (no change is the player's)"), Disk->Exists(UPawSettingsSubsystem::EntryName));
		D->MoveSlider(Slider::Music, 0.2);
		D->PickDock(1);
		D->PickFps(2);
		D->ToggleEffects();
		D->PressClose();
		Pc::PcSettings File;
		if (TestTrue(TEXT("a change writes the file"), W.ReadFile(File)))
		{
			TestEqual(TEXT("the music"), File.MusicVolume, 0.2);
			TestEqual(TEXT("the side"), File.DockSide, std::string("right"));
			TestEqual(TEXT("the cap"), File.MaxFps, static_cast<int64_t>(60));
			TestFalse(TEXT("the effects"), File.bEffectsEnabled);
		}
		// The last slot used: saving into a slot, and loading one, move it.
		W.Game->Progress().LifetimeCatnipEver = 10.0;
		W.Save->SaveGame(3);
		W.Save->Flush();
		TestEqual(TEXT("saving into slot 4 makes it the last slot used"), W.Settings->Get().LastSlot, static_cast<int64_t>(3));
		W.Save->SaveGame(1);
		W.Save->Flush();
		W.Save->LoadGame(3);
		TestEqual(TEXT("loading slot 4 too"), W.Settings->Get().LastSlot, static_cast<int64_t>(3));
		W.Close();
	}
	{
		// The next run on the same machine.
		FSettingsWorld W;
		if (!W.Open(*this, false, Disk))
		{
			W.Close();
			return false;
		}
		TestEqual(TEXT("the music comes back"), W.Settings->Get().MusicVolume, 0.2);
		TestFalse(TEXT("the dock's side"), W.Settings->IsDockOnLeft());
		TestEqual(TEXT("the effects budget"), W.Settings->EffectBudget(), 0.0);
		TestEqual(TEXT("the run starts on the last slot used"), W.Save->GetActiveSlot(), 3);
		UPawSettingsDialog* D = W.OpenDialog(*this);
		if (D)
		{
			TestEqual(TEXT("the dialog shows it"), S(D->GetVolumeLine(Slider::Music)), FString(TEXT("Music  20%")));
			TestEqual(TEXT("the cap's item"), D->GetOption(TEXT("MaxFpsOption"))->GetSelectedOption(), FString(TEXT("60 FPS")));
			D->PressClose();
		}
		// An unreadable file: the defaults (Godot's ConfigFile.load() failing), the file left as it is.
		const char Junk[] = "\x89PNG";
		Disk->Put(UPawSettingsSubsystem::EntryName, TArray<uint8>(reinterpret_cast<const uint8*>(Junk), 4));
		AddExpectedError(TEXT("the settings file is unreadable"), EAutomationExpectedErrorFlags::Contains, 1);
		W.Settings->Reload();
		TestEqual(TEXT("unreadable: the default music"), W.Settings->Get().MusicVolume, 0.45);
		TestTrue(TEXT("the file is not rewritten by reading it"), Disk->Exists(UPawSettingsSubsystem::EntryName));
		W.Close();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawSettingsAppliesTest, "Pawpocalypse.UI.Settings.Applies", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawSettingsAppliesTest::RunTest(const FString& Parameters)
{
	FSettingsWorld W;
	if (!W.Open(*this, true))
	{
		W.Close();
		return false;
	}
	UPawSettingsDialog* D = W.OpenDialog(*this);
	if (!D)
	{
		W.Close();
		return false;
	}
	TestTrue(TEXT("the dialog holds the field"), W.Realm->IsPaused());
	// The menu side: the HUD's dock follows at once.
	TestTrue(TEXT("the dock starts on the left"), W.Hud->IsDockOnLeft() && W.Hud->IsDockDrawnOnLeft());
	D->PickDock(1);
	TestFalse(TEXT("Menu side: Right moves the dock"), W.Hud->IsDockOnLeft() || W.Hud->IsDockDrawnOnLeft());
	D->PickDock(0);
	TestTrue(TEXT("and back"), W.Hud->IsDockOnLeft() && W.Hud->IsDockDrawnOnLeft());
	// The video settings reach the applier; in the editor never the engine.
	const int32 Before = W.Settings->GetVideoApplied().Count;
	D->PressSection(Pc::PcSettingsSection::Video);
	D->PickResolution(0);
	TestEqual(TEXT("a resolution is applied"), W.Settings->GetVideoApplied().Count, Before + 1);
	TestEqual(TEXT("1280 wide"), W.Settings->GetVideoApplied().Width, 1280);
	D->ToggleFullscreen();
	TestTrue(TEXT("fullscreen applied"), W.Settings->GetVideoApplied().bFullscreen);
	TestTrue(TEXT("Resolution stays live until the dialog opens again (as Godot)"), D->GetOption(TEXT("ResolutionOption"))->GetIsEnabled());
	D->PickFps(3);
	TestEqual(TEXT("the cap applied"), W.Settings->GetVideoApplied().MaxFps, static_cast<int64>(120));
	D->ToggleVsync();
	TestFalse(TEXT("V-Sync off applied"), W.Settings->GetVideoApplied().bVsync);
	TestFalse(TEXT("never the editor's own engine"), W.Settings->GetVideoApplied().bToEngine);
	TestTrue(TEXT("a packaged run with its settings and a window applies"), UPawSettingsSubsystem::MayApplyToEngine(false, true, true));
	TestFalse(TEXT("the editor never"), UPawSettingsSubsystem::MayApplyToEngine(true, true, true));
	TestFalse(TEXT("a run with no settings storage (the bench, the tour) never"), UPawSettingsSubsystem::MayApplyToEngine(false, false, true));
	TestFalse(TEXT("no window, nothing to apply to"), UPawSettingsSubsystem::MayApplyToEngine(false, true, false));
	const int32 Video = W.Settings->GetVideoApplied().Count;
	D->PickQuality(0);
	D->MoveSlider(Slider::Master, 0.5);
	TestEqual(TEXT("audio and quality do not touch the window"), W.Settings->GetVideoApplied().Count, Video);
	D->PressClose();
	TestFalse(TEXT("closing lets the field go"), W.Realm->IsPaused());
	D = W.OpenDialog(*this);
	if (D)
	{
		TestFalse(TEXT("reopened while fullscreen: Resolution greyed"), D->GetOption(TEXT("ResolutionOption"))->GetIsEnabled());
		D->PressClose();
	}
	// The Reawaken page's box is the setting.
	const bool bAsk = W.Settings->Get().bConfirmReawaken;
	W.Hud->ToggleAskBeforeReawaken();
	TestEqual(TEXT("the HUD's box writes the setting"), W.Settings->Get().bConfirmReawaken, !bAsk);
	TestEqual(TEXT("the box shows it"), W.Hud->GetAskReawakenButton()->IsTicked(), !bAsk);
	W.Hud->ToggleAskBeforeReawaken();
	// Effects off: the budget is 0.
	W.Settings->SetEffectsEnabled(false);
	TestEqual(TEXT("effects off: no budget"), W.Settings->EffectBudget(), 0.0);
	W.Settings->SetEffectsEnabled(true);
	W.Settings->SetGraphicsQuality(1);
	TestEqual(TEXT("Medium: 0.70"), W.Settings->EffectBudget(), 0.70);
	W.Close();
	return true;
}

#endif
