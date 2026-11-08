// Pawpocalypse.Input.Glyphs   what a prompt shows is what Godot's InputGlyphs shows (fixture `input_glyphs`): every action in
//                             every family (glyph / keycap / nothing, the stem, the keycap's label), every bound key's label,
//                             and the {token} replacement of the game's prompt templates; every glyph texture imported as UI art.
// Pawpocalypse.Input.Prompts  the prompt widgets: an icon draws a glyph or a keycap and follows the device in hand without
//                             being told; a prompt line hides with Show button prompts off (a tooltip's line never does); the
//                             HUD's tab keys, legend and Pounce key; the title's hint; the press prompt's placement (in the
//                             dock: outside the row on the field side; elsewhere: inside the button's right edge).
//
// Negative tests: tools/sabotage_realm.py (`glyph_*`).

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/PawGameSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Input/PawInputDeviceSubsystem.h"
#include "Input/PawInputMap.h"
#include "Misc/AutomationTest.h"
#include "Components/Border.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Realm/PawRealmSubsystem.h"
#include "Save/PawSettingsSubsystem.h"
#include "UI/PawGlyphs.h"
#include "UI/PawHudScreen.h"
#include "UI/PawMainMenu.h"
#include "UI/PawPrompts.h"
#include "UI/PawWidgets.h"

namespace
{
	using Fam = Pc::PcFamily;

	// A fixture table: its header's columns by name, then the rows.
	bool ReadTsv(FAutomationTestBase& T, const TCHAR* Name, TArray<TMap<FString, FString>>& Out)
	{
		FString Text;
		if (!T.TestTrue(FString::Printf(TEXT("the fixture %s"), Name),
			FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Source/PawCore/Tests/Data") / Name))))
		{
			return false;
		}
		TArray<FString> Lines;
		Text.ParseIntoArrayLines(Lines, false);
		TArray<FString> Header;
		for (const FString& Line : Lines)
		{
			if (Line.IsEmpty() || Line.StartsWith(TEXT("#")))
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
			TMap<FString, FString>& Row = Out.AddDefaulted_GetRef();
			for (int32 I = 0; I < Header.Num(); ++I)
			{
				Row.Add(Header[I], I < C.Num() ? C[I] : FString());
			}
		}
		return true;
	}

	struct FPromptWorld
	{
		UGameInstance* GI = nullptr;
		UWorld* World = nullptr;
		UPawSettingsSubsystem* Settings = nullptr;
		UPawInputDeviceSubsystem* Device = nullptr;

		bool Open(FAutomationTestBase& T)
		{
			GI = NewObject<UGameInstance>(GEngine);
			GI->InitializeStandalone();
			World = GI->GetWorld();
			Settings = GI->GetSubsystem<UPawSettingsSubsystem>();
			Device = GI->GetSubsystem<UPawInputDeviceSubsystem>();
			UPawGameSubsystem* Game = GI->GetSubsystem<UPawGameSubsystem>();
			UPawRealmSubsystem* Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
			if (!T.TestNotNull(TEXT("settings"), Settings) || !T.TestNotNull(TEXT("devices"), Device) || !T.TestNotNull(TEXT("game"), Game)
				|| !T.TestNotNull(TEXT("realm"), Realm) || !T.TestTrue(TEXT("data"), Game->IsDataReady()))
			{
				return false;
			}
			T.TestFalse(TEXT("an automated run has no settings storage (the designer's file untouched)"), Settings->IsStored());
			Game->NewGame();
			Realm->SetViewSize(1920.0, 1080.0);
			Realm->StartRealm(77);
			Device->Clock = [this]() { return Device->GetState().BootMs + 100000; };
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

		// The hand on the keyboard, or on pad 1 (an Xbox: XInput).
		void UseKeyboard() { Device->ObserveKey(FKeyEvent(EKeys::SpaceBar, FModifierKeysState(), 0, false, 0, 0), true); }
		void UsePad()
		{
			Device->ObserveKey(FKeyEvent(EKeys::Gamepad_FaceButton_Bottom, FModifierKeysState(), FInputDeviceId::CreateFromInternalId(1),
				false, 0, 0), true);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputGlyphsTest, "Pawpocalypse.Input.Glyphs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputGlyphsTest::RunTest(const FString& Parameters)
{
	PawInput::ResetBindings();
	// 1. Every action, every family: the same prompt as Godot's.
	TArray<TMap<FString, FString>> Rows;
	if (!ReadTsv(*this, TEXT("input_glyphs_godot.tsv"), Rows))
	{
		return false;
	}
	int32 Checked = 0, Wrong = 0, Glyphs = 0, Keycaps = 0, Nones = 0;
	for (const TMap<FString, FString>& R : Rows)
	{
		const FString& Action = R[TEXT("action")];
		const Fam Family = static_cast<Fam>(FCString::Atoi(*R[TEXT("family")]));
		const FString& Kind = R[TEXT("kind")];
		const PawGlyphs::FPrompt P = PawGlyphs::PromptFor(Action, Family);
		bool bOk = false;
		if (Kind == TEXT("glyph"))
		{
			bOk = P.Kind == PawGlyphs::EKind::Glyph && P.Stem == R[TEXT("stem")];
			++Glyphs;
		}
		else if (Kind == TEXT("keycap"))
		{
			bOk = P.Kind == PawGlyphs::EKind::Keycap && P.Label.ToString() == R[TEXT("label")];
			++Keycaps;
		}
		else if (Kind == TEXT("none"))
		{
			bOk = P.Kind == PawGlyphs::EKind::None;
			++Nones;
		}
		else
		{
			bOk = P.Kind == PawGlyphs::EKind::Blank;
		}
		++Checked;
		if (!bOk)
		{
			if (++Wrong <= 12)
			{
				AddError(FString::Printf(TEXT("%s family %d: Godot %s %s %s; the port %d %s %s"), *Action, static_cast<int32>(Family), *Kind,
					*R[TEXT("stem")], *R[TEXT("label")], static_cast<int32>(P.Kind), *P.Stem, *P.Label.ToString()));
			}
		}
	}
	TestEqual(TEXT("every action x family compared"), Checked, 160);
	TestTrue(TEXT("glyphs, keycaps and actions with nothing on a device are all in it"), Glyphs > 90 && Keycaps > 15 && Nones > 20);
	TestEqual(TEXT("and each is Godot's"), Wrong, 0);

	// 2. Every bound key's label (Godot: a printable key is its character, a named one a word, the rest its engine name).
	TArray<TMap<FString, FString>> Keys;
	if (!ReadTsv(*this, TEXT("input_keys_godot.tsv"), Keys))
	{
		return false;
	}
	int32 Labels = 0;
	for (const TMap<FString, FString>& R : Keys)
	{
		if (R[TEXT("ue_key")].IsEmpty())
		{
			continue;
		}
		const FKey Key(*R[TEXT("ue_key")]);
		TestTrue(FString::Printf(TEXT("%s is a key"), *R[TEXT("ue_key")]), Key.IsValid());
		TestEqual(FString::Printf(TEXT("the label of %s"), *R[TEXT("ue_key")]), PawGlyphs::KeyLabel(Key).ToString(), R[TEXT("label")]);
		++Labels;
	}
	TestTrue(TEXT("every key the port binds"), Labels >= 45);

	// 3. The templates: tokens of actions replaced, any other placeholder left alone, an action with nothing on a device gone.
	TArray<TMap<FString, FString>> Prompts;
	if (!ReadTsv(*this, TEXT("input_prompts_godot.tsv"), Prompts))
	{
		return false;
	}
	for (const TMap<FString, FString>& R : Prompts)
	{
		const Fam Family = static_cast<Fam>(FCString::Atoi(*R[TEXT("family")]));
		TestEqual(FString::Printf(TEXT("%s, family %d"), *R[TEXT("id")], static_cast<int32>(Family)),
			PawGlyphs::Neutral(PawGlyphs::Parts(R[TEXT("template")], Family)), R[TEXT("prompt")]);
	}
	TestEqual(TEXT("every template x family"), Prompts.Num(), 50);
	// The markup the rich text reads: words escaped, prompts as tags.
	const FString Markup = PawGlyphs::Markup(PawGlyphs::Parts(TEXT("<b> & {press}"), Fam::Xbox), 22);
	TestEqual(TEXT("the markup"), Markup, FString(TEXT("&lt;b&gt; &amp; <prompt g=\"input_xbox_face_down\" px=\"22\"/>")));

	// 4. Every glyph imported, as UI art (UserInterface2D, no mips, never streamed).
	int32 Textures = 0;
	TSet<FString> Stems;
	for (const TMap<FString, FString>& R : Rows)
	{
		if (R[TEXT("exists")] == TEXT("1"))
		{
			Stems.Add(R[TEXT("stem")]);
		}
	}
	for (const FString& Stem : Stems)
	{
		UTexture2D* Tex = PawGlyphs::Texture(Stem);
		if (!TestNotNull(*FString::Printf(TEXT("T_%s"), *Stem), Tex))
		{
			continue;
		}
#if WITH_EDITORONLY_DATA // the import's source and its mip setting are editor data (the game target has neither)
		TestEqual(FString::Printf(TEXT("%s: 56 px (2x the largest prompt)"), *Stem), Tex->Source.GetSizeX(), 56); // the source: no RHI in a headless run
		TestTrue(FString::Printf(TEXT("%s: no mips"), *Stem), Tex->MipGenSettings == TMGS_NoMipmaps);
#endif
		TestTrue(FString::Printf(TEXT("%s: UI art"), *Stem), Tex->CompressionSettings == TC_EditorIcon && Tex->NeverStream && Tex->SRGB);
		++Textures;
	}
	TestTrue(TEXT("the glyphs Godot draws"), Textures >= 40);
	TestNull(TEXT("no texture for a stem with none"), PawGlyphs::Texture(TEXT("input_xbox_nothing")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPawInputPromptsTest, "Pawpocalypse.Input.Prompts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPawInputPromptsTest::RunTest(const FString& Parameters)
{
	PawInput::ResetBindings();
	FPromptWorld W;
	if (!W.Open(*this))
	{
		W.Close();
		return false;
	}
	W.UseKeyboard();

	// An icon: a keycap on the keys, a glyph on a pad, redrawn when the hand moves (nobody calls Refresh).
	UPawPromptIcon* Icon = CreateWidget<UPawPromptIcon>(W.GI, PawUi::Resolve<UPawPromptIcon>());
	if (!TestNotNull(TEXT("an icon"), Icon) || !TestTrue(TEXT("from its Widget Blueprint"), Icon->GetClass() != UPawPromptIcon::StaticClass()))
	{
		W.Close();
		return false;
	}
	// Held as a screen holds it: a Slate widget let go destructs its user widget, which stops listening.
	TArray<TSharedRef<SWidget>> Held;
	Held.Add(Icon->TakeWidget());
	Icon->SetAction(TEXT("press"));
	TestTrue(TEXT("the keys: a keycap"), Icon->GetShown().Kind == PawGlyphs::EKind::Keycap);
	TestEqual(TEXT("labelled Space"), Icon->GetShown().Label.ToString(), FString(TEXT("Space")));
	W.UsePad();
	TestTrue(TEXT("a pad in hand: the glyph, unasked"), Icon->GetShown().Kind == PawGlyphs::EKind::Glyph);
	TestEqual(TEXT("A on an Xbox pad"), Icon->GetShown().Stem, FString(TEXT("input_xbox_face_down")));
	TestEqual(TEXT("its size"), Icon->GetPromptSize(), FVector2D(28.0, 28.0));
	W.Settings->SetPromptStyle(TEXT("playstation"));
	TestEqual(TEXT("Button prompts: PlayStation"), Icon->GetShown().Stem, FString(TEXT("input_playstation_face_down")));
	W.Settings->SetPromptStyle(TEXT("auto"));
	Icon->SetFamilyOverride(static_cast<int32>(Fam::SteamDeck));
	TestEqual(TEXT("a family pinned (the Controls page's columns)"), Icon->GetShown().Stem, FString(TEXT("input_steam_deck_face_down")));
	Icon->SetFamilyOverride(-1);
	Icon->SetAction(TEXT("steer_left"));
	W.UseKeyboard();
	TestFalse(TEXT("a pad-only action on the keys: nothing"), Icon->ShowsAnything());
	Icon->SetAction(TEXT("pounce"));
	TestEqual(TEXT("pounce on the keys is the mouse button"), Icon->GetShown().Stem, FString(TEXT("input_keyboard_mouse_left")));
	// Show button prompts off: nothing, unless it is always shown.
	W.Settings->SetShowPrompts(false);
	TestFalse(TEXT("prompts off: nothing"), Icon->ShowsAnything());
	Icon->bAlwaysShow = true;
	Icon->Refresh();
	TestTrue(TEXT("but an always-shown one"), Icon->ShowsAnything());
	Icon->bAlwaysShow = false;

	// A prompt line: hidden whole with the setting off; a tooltip's line is not.
	UPawPromptText* Line = CreateWidget<UPawPromptText>(W.GI, PawUi::Resolve<UPawPromptText>());
	UPawPromptText* Tip = CreateWidget<UPawPromptText>(W.GI, PawUi::Resolve<UPawPromptText>());
	if (!TestNotNull(TEXT("a line"), Line) || !TestNotNull(TEXT("a tip line"), Tip))
	{
		W.Close();
		return false;
	}
	Tip->bHideWithPrompts = false; // set where it is built, as the tree's code layout and asset do
	// R4c/R4f: a line on its own plum plate (the title's hint, the tree's legend) takes the plate with it.
	UBorder* Plate = NewObject<UBorder>(Line->GetOuter());
	Plate->SetContent(Line);
	Held.Add(Line->TakeWidget());
	Held.Add(Tip->TakeWidget());
	Line->SetTemplate(FText::FromString(TEXT("{press} Buy")));
	Tip->SetTemplate(FText::FromString(TEXT("{press} Buy")));
	TestTrue(TEXT("prompts off: the line hidden"), Line->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("prompts off: its plate with it, no empty pill"), Plate->GetVisibility() == ESlateVisibility::Collapsed);
	TestTrue(TEXT("a tooltip's line is its owner's to hide"), Tip->GetVisibility() != ESlateVisibility::Collapsed);
	W.Settings->SetShowPrompts(true);
	TestTrue(TEXT("prompts on: shown"), Line->GetVisibility() != ESlateVisibility::Collapsed);
	TestTrue(TEXT("prompts on: its plate too"), Plate->GetVisibility() != ESlateVisibility::Collapsed);
	TestEqual(TEXT("the keys"), Line->GetNeutral(), FString(TEXT("<k:Space> Buy")));
	W.UsePad();
	TestEqual(TEXT("a pad: redrawn unasked"), Line->GetNeutral(), FString(TEXT("<g:input_xbox_face_down> Buy")));
	TestTrue(TEXT("its rich text holds the prompt"), Line->GetMarkup().Contains(TEXT("<prompt g=\"input_xbox_face_down\"")));

	// The HUD: the tab keys flank the tabs, the legend follows the page, the Pounce key by its clock.
	W.UseKeyboard();
	UPawHudScreen* Hud = CreateWidget<UPawHudScreen>(W.GI, PawUi::Resolve<UPawHudScreen>());
	if (TestNotNull(TEXT("the HUD"), Hud))
	{
		Held.Add(Hud->TakeWidget());
		UPawPromptIcon* Prev = Hud->GetTabPrompt(false);
		UPawPromptIcon* Next = Hud->GetTabPrompt(true);
		if (TestNotNull(TEXT("the previous-tab key"), Prev) && TestNotNull(TEXT("the next-tab key"), Next))
		{
			TestEqual(TEXT("A"), Prev->GetShown().Label.ToString(), FString(TEXT("A")));
			TestEqual(TEXT("D"), Next->GetShown().Label.ToString(), FString(TEXT("D")));
			TestEqual(TEXT("26 px"), Prev->IconPx, 26);
		}
		Hud->SelectTab(Pc::PcDockTab::Upgrades);
		TestEqual(TEXT("the Upgrades legend (Godot hud_upgrades)"), Hud->GetDockLegend(),
			FString(TEXT("<k:Space> Buy   <k:Q> Amount   <k:T> Skills   <k:Esc> Pause")));
		Hud->SelectTab(Pc::PcDockTab::Realms);
		TestEqual(TEXT("elsewhere (Godot hud_other)"), Hud->GetDockLegend(), FString(TEXT("<k:Space> Select   <k:T> Skills   <k:Esc> Pause")));
		W.UsePad();
		TestEqual(TEXT("on a pad"), Hud->GetDockLegend(),
			FString(TEXT("<g:input_xbox_face_down> Select   <g:input_xbox_face_up> Skills   <g:input_xbox_start> Pause")));
		if (TestNotNull(TEXT("the Pounce key"), Hud->GetPouncePrompt()))
		{
			TestEqual(TEXT("the right trigger"), Hud->GetPouncePrompt()->GetShown().Stem, FString(TEXT("input_xbox_trigger_r")));
		}
	}

	// The press prompt's place (input_device.gd _place_focus_prompt; dock_navigator.gd _place_row_prompt).
	const FVector2D Row(1300, 200), RowSize(560, 46), P(26, 26);
	TestEqual(TEXT("in the dock on the right: left of the row, gap 10"),
		UPawInputDeviceSubsystem::FocusPromptAt(Row, RowSize, P, true, false), FVector2D(1300 - 10 - 26, 200 + 10));
	TestEqual(TEXT("in the dock on the left: right of the row"),
		UPawInputDeviceSubsystem::FocusPromptAt(Row, RowSize, P, true, true), FVector2D(1300 + 560 + 10, 210));
	TestEqual(TEXT("elsewhere: inside the right edge, inset 10"),
		UPawInputDeviceSubsystem::FocusPromptAt(Row, RowSize, FVector2D(24, 24), false, false), FVector2D(1300 + 560 - 24 - 10, 211));
	TestFalse(TEXT("not for the pointer"), UPawInputDeviceSubsystem::FocusPromptShown(true, true));
	TestTrue(TEXT("for keys and pads"), UPawInputDeviceSubsystem::FocusPromptShown(true, false));
	Held.Reset();
	W.Close();
	return true;
}

#endif
