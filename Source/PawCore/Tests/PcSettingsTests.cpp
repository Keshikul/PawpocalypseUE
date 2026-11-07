#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcRng.h"
#include "PcSettings.h"

#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The settings against the REAL settings_manager.gd and settings_dialog.tscn (fixture `settings`): Godot's own ConfigFile
// wrote each row's settings.cfg; PcSettings reads that very text and must hold what Godot's load_settings() held, then each
// change made on Godot's live dialog is made here and every setting and everything the dialog shows compared.

namespace
{
	std::string DataDir()
	{
		const char* Env = std::getenv("PAWCORE_DATA_DIR");
		return Env ? std::string(Env) : std::string("Data");
	}

	std::vector<std::string> Cells(const std::string& S)
	{
		std::vector<std::string> Out;
		size_t Start = 0;
		while (true)
		{
			const size_t At = S.find('\t', Start);
			Out.push_back(S.substr(Start, At == std::string::npos ? std::string::npos : At - Start));
			if (At == std::string::npos)
			{
				return Out;
			}
			Start = At + 1;
		}
	}

	std::string Unescape(const std::string& In)
	{
		std::string Out;
		for (size_t I = 0; I < In.size(); ++I)
		{
			if (In[I] == '\\' && I + 1 < In.size())
			{
				const char C = In[++I];
				Out.push_back(C == 'n' ? '\n' : C == 't' ? '\t' : C);
			}
			else
			{
				Out.push_back(In[I]);
			}
		}
		return Out;
	}

	double Hex(const std::string& V) { return std::strtod(V.c_str(), nullptr); }
	bool Flag(const std::string& V) { return V == "1"; }
	long long Int(const std::string& V) { return std::atoll(V.c_str()); }

	struct FResult
	{
		int Rows = 0, Changes = 0, SlidersMoved = 0, SlidersStill = 0, OutOfRange = 0, UnknownFps = 0, FileFloats = 0;
	};

	const char* SectionName(Pc::PcSettingsSection S)
	{
		return S == Pc::PcSettingsSection::Audio ? "Audio" : S == Pc::PcSettingsSection::Video ? "Video" : "Controls";
	}

	FResult RunSettingsFixture(const std::string& Path)
	{
		FResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		Pc::PcSettings S;
		Pc::PcSettingsDialog D;
		while (std::getline(In, Line))
		{
			if (Line.empty() || Line[0] == '#')
			{
				continue;
			}
			const std::vector<std::string> C = Cells(Line);
			if (Header.empty())
			{
				Header = C;
				continue;
			}
			REQUIRE(C.size() == Header.size());
			std::map<std::string, std::string> V;
			for (size_t I = 0; I < C.size(); ++I)
			{
				V[Header[I]] = C[I];
			}
			const std::string Op = V.at("op");
			CAPTURE(V.at("row"));
			CAPTURE(Op);
			if (Op == "load")
			{
				++R.Rows;
				S = Pc::PcSettings();
				const std::string Cfg = Unescape(V.at("cfg"));
				CHECK(Pc::SettingsLoad(Cfg, S));
				R.FileFloats += Cfg.find("_volume=0.") != std::string::npos ? 1 : 0;
				Pc::SettingsDialogSync(D, S);
				R.OutOfRange += (S.MasterVolume > 1.0 || S.MasterVolume < 0.0) ? 1 : 0;
				R.UnknownFps += Pc::SettingsFpsOption(Pc::SettingsFpsSelected(S)) != S.MaxFps ? 1 : 0;
				// Rewritten by the port and read back: every value Godot read from its own file survives.
				Pc::PcSettings Back;
				CHECK(Pc::SettingsLoad(Pc::SettingsWrite(S), Back));
				CHECK(Back.MasterVolume == S.MasterVolume);
				CHECK(Back.MusicVolume == S.MusicVolume);
				CHECK(Back.SfxVolume == S.SfxVolume);
				CHECK(Back.CursorSpeed == S.CursorSpeed);
				CHECK(Back.StickDeadzone == S.StickDeadzone);
				CHECK(Back.OutputDevice == S.OutputDevice);
				CHECK(Back.MaxFps == S.MaxFps);
			}
			else
			{
				++R.Changes;
				const std::string& Arg = V.at("arg");
				auto Slider = [&](Pc::PcSettingsSlider W)
				{
					if (Pc::SettingsDialogMoveSlider(D, S, W, Hex(Arg))) { ++R.SlidersMoved; } else { ++R.SlidersStill; }
				};
				if (Op == "vol") { Slider(Pc::PcSettingsSlider::Master); }
				else if (Op == "music") { Slider(Pc::PcSettingsSlider::Music); }
				else if (Op == "sfx") { Slider(Pc::PcSettingsSlider::Sfx); }
				else if (Op == "mute") { S.bMuted = !S.bMuted; }
				else if (Op == "effects") { S.bEffectsEnabled = !S.bEffectsEnabled; }
				else if (Op == "quality") { S.GraphicsQuality = Pc::SettingsClampQuality(Int(Arg)); }
				else if (Op == "fps") { S.MaxFps = Pc::SettingsClampFps(Pc::SettingsFpsOption(Int(Arg))); }
				else if (Op == "prompts") { S.PromptStyle = Pc::SettingsPromptStyle(Int(Arg)); }
				else if (Op == "dock") { S.DockSide = Pc::SettingsDockSide(Int(Arg)); }
				else if (Op == "section") { Pc::SettingsDialogStepSection(D, Int(Arg)); }
				else if (Op == "reopen") { Pc::SettingsDialogSync(D, S); }
				else { FAIL("unknown op " << Op); }
			}
			// Every setting, as Godot held it.
			CHECK(S.MasterVolume == Hex(V.at("master")));
			CHECK(S.MusicVolume == Hex(V.at("music")));
			CHECK(S.SfxVolume == Hex(V.at("sfx")));
			CHECK(S.bMuted == Flag(V.at("muted")));
			CHECK(S.OutputDevice == Unescape(V.at("output")));
			CHECK(S.bFullscreen == Flag(V.at("fullscreen")));
			CHECK(S.bVsync == Flag(V.at("vsync")));
			CHECK(S.GraphicsQuality == Int(V.at("quality")));
			CHECK(S.MaxFps == Int(V.at("max_fps")));
			CHECK(S.bEffectsEnabled == Flag(V.at("effects")));
			CHECK(S.Language == Unescape(V.at("language")));
			CHECK(S.LastSlot == Int(V.at("last_slot")));
			CHECK(S.ResolutionIndex == Int(V.at("res_index")));
			CHECK(S.bConfirmReawaken == Flag(V.at("confirm")));
			CHECK(S.bTutorialTips == Flag(V.at("tips")));
			CHECK(S.PromptStyle == V.at("prompt"));
			CHECK(S.DockSide == V.at("dock"));
			CHECK(S.bShowPrompts == Flag(V.at("show_prompts")));
			CHECK(S.bSwapConfirmBack == Flag(V.at("swap_cb")));
			CHECK(S.bSwapSticks == Flag(V.at("swap_sticks")));
			CHECK(S.bInvertX == Flag(V.at("invert_x")));
			CHECK(S.bInvertY == Flag(V.at("invert_y")));
			CHECK(S.CursorSpeed == Hex(V.at("cursor_speed")));
			CHECK(S.StickDeadzone == Hex(V.at("deadzone")));
			CHECK(Pc::EffectBudget(S) == Hex(V.at("budget")));
			// What the dialog shows.
			CHECK(V.at("vol_label") == "Master Volume: " + std::to_string(D.Percent[0]) + "%");
			CHECK(V.at("music_label") == "Music  " + std::to_string(D.Percent[1]) + "%");
			CHECK(V.at("sfx_label") == "Sound Effects  " + std::to_string(D.Percent[2]) + "%");
			CHECK(D.Slider[0] == Hex(V.at("vol_slider")));
			CHECK(D.Slider[1] == Hex(V.at("music_slider")));
			CHECK(D.Slider[2] == Hex(V.at("sfx_slider")));
			// == holds -0 equal to +0: the sign is checked too (a slider dragged below 0 rests at +0 in Godot).
			CHECK(std::signbit(D.Slider[0]) == std::signbit(Hex(V.at("vol_slider"))));
			CHECK(std::signbit(D.Slider[1]) == std::signbit(Hex(V.at("music_slider"))));
			CHECK(std::signbit(D.Slider[2]) == std::signbit(Hex(V.at("sfx_slider"))));
			CHECK(std::signbit(S.MasterVolume) == std::signbit(Hex(V.at("master"))));
			CHECK(std::signbit(S.MusicVolume) == std::signbit(Hex(V.at("music"))));
			CHECK(std::signbit(S.SfxVolume) == std::signbit(Hex(V.at("sfx"))));
			CHECK(Flag(V.at("mute_check")) == S.bMuted);
			CHECK(Flag(V.at("full_check")) == S.bFullscreen);
			CHECK(Flag(V.at("vsync_check")) == S.bVsync);
			CHECK(Int(V.at("res_sel")) == S.ResolutionIndex);
			CHECK(Flag(V.at("res_disabled")) == S.bFullscreen);
			const Pc::PcIntSize Res = Pc::SettingsResolution(S.ResolutionIndex);
			CHECK(V.at("res_text") == std::to_string(Res.X) + " x " + std::to_string(Res.Y));
			CHECK(Int(V.at("quality_sel")) == S.GraphicsQuality);
			const int64_t Fps = Pc::SettingsFpsSelected(S);
			CHECK(Int(V.at("fps_sel")) == Fps);
			CHECK(V.at("fps_text") == (Fps == 0 ? std::string("Unlimited") : std::to_string(Pc::SettingsFpsOption(Fps)) + " FPS"));
			CHECK(Flag(V.at("effects_check")) == S.bEffectsEnabled);
			CHECK(Int(V.at("prompts_sel")) == Pc::SettingsPromptsSelected(S));
			CHECK(Int(V.at("dock_sel")) == Pc::SettingsDockSelected(S));
			// Godot headless lists one device, "Default".
			const Pc::PcSettingsOutputList Out = Pc::SettingsOutputList({ "Default" }, S);
			std::string Items;
			for (size_t I = 0; I < Out.Devices.size(); ++I)
			{
				Items += (I ? "|" : "") + (Out.Devices[I] == "Default" ? std::string("Follow the system") : Out.Devices[I]);
			}
			CHECK(Unescape(V.at("output_items")) == Items);
			CHECK(Int(V.at("output_sel")) == Out.Selected);
			CHECK(V.at("section") == SectionName(D.Section));
		}
		return R;
	}
}

TEST_CASE("Settings: every load and change matches the real settings_manager.gd and settings_dialog.tscn")
{
	const FResult R = RunSettingsFixture(DataDir() + "/../Source/PawCore/Tests/Data/settings_godot.tsv");
	MESSAGE("rows " << R.Rows << ", changes " << R.Changes << ", sliders moved " << R.SlidersMoved << " / still " << R.SlidersStill
		<< ", master out of 0..1 " << R.OutOfRange << ", a cap not on the list " << R.UnknownFps << ", files with float volumes "
		<< R.FileFloats);
	CHECK(R.Rows == 1000);
	CHECK(R.SlidersMoved > 1000);
	CHECK(R.SlidersStill > 20);
	CHECK(R.OutOfRange > 30);
	CHECK(R.UnknownFps > 30);
	CHECK(R.FileFloats > 300);
}

TEST_CASE("Settings: the long suite runs all 3,000 settings rows" * doctest::test_suite("long") * doctest::skip())
{
	const FResult R = RunSettingsFixture(DataDir() + "/../build/settings_godot_full.tsv");
	CHECK(R.Rows == 3000);
}

TEST_CASE("Settings: the window fits the screen as fit_to_screen and centred_position say")
{
	std::string Text;
	REQUIRE(Pc::ReadTextFile(DataDir() + "/../Source/PawCore/Tests/Data/settings_fit_godot.tsv", Text));
	std::istringstream In(Text);
	std::string Line;
	bool bHeader = true;
	int Rows = 0, Clamped = 0, NoScreen = 0;
	while (std::getline(In, Line))
	{
		if (Line.empty() || Line[0] == '#')
		{
			continue;
		}
		if (bHeader)
		{
			bHeader = false;
			continue;
		}
		const std::vector<std::string> C = Cells(Line);
		REQUIRE(C.size() == 14);
		auto N = [&](size_t I) { return static_cast<int64_t>(Int(C[I])); };
		const Pc::PcIntSize Fit = Pc::FitToScreen({ N(0), N(1) }, { N(2), N(3) }, { N(4), N(5) });
		CAPTURE(Line);
		CHECK(Fit.X == N(6));
		CHECK(Fit.Y == N(7));
		const Pc::PcIntSize At = Pc::CentredPosition({ N(8), N(9) }, { N(10), N(11) }, Fit);
		CHECK(At.X == N(12));
		CHECK(At.Y == N(13));
		++Rows;
		Clamped += (Fit.X != N(0) || Fit.Y != N(1)) ? 1 : 0;
		NoScreen += N(2) <= 0 ? 1 : 0;
	}
	CHECK(Rows == 3000);
	CHECK(Clamped > 500);
	CHECK(NoScreen > 50);
}

TEST_CASE("Settings: what the port writes reads back the same; anything else is not a settings file")
{
	auto Unit = [](int I, int Salt) { return Pc::RngUnit(20261104, static_cast<uint64_t>(I), 0, static_cast<uint64_t>(Salt)); };
	for (int I = 0; I < 500; ++I)
	{
		Pc::PcSettings S;
		Pc::PcSettingsDialog D;
		Pc::SettingsDialogMoveSlider(D, S, Pc::PcSettingsSlider::Master, Unit(I, 1));
		Pc::SettingsDialogMoveSlider(D, S, Pc::PcSettingsSlider::Music, Unit(I, 2));
		S.SfxVolume = static_cast<double>(I % 2);
		S.bMuted = I % 3 == 0;
		S.OutputDevice = I % 4 == 0 ? "Speakers \"Desk\" \\ 2\nline" : "Default";
		S.bFullscreen = I % 5 == 0;
		S.bVsync = I % 2 == 1;
		S.GraphicsQuality = I % 3;
		S.MaxFps = Pc::SettingsFpsOption(I % 6) + (I % 11 == 0 ? 15 : 0);
		S.bEffectsEnabled = I % 7 != 0;
		S.Language = I % 2 ? "system" : "de";
		S.ResolutionIndex = I % 5;
		S.LastSlot = I % 5;
		S.bConfirmReawaken = I % 2 == 0;
		S.bTutorialTips = I % 3 != 1;
		S.PromptStyle = Pc::SettingsPromptStyle(I % 5);
		S.DockSide = Pc::SettingsDockSide(I % 2);
		S.bShowPrompts = I % 4 != 0;
		S.bSwapConfirmBack = I % 6 == 0;
		S.bSwapSticks = I % 8 == 0;
		S.bInvertX = I % 9 == 0;
		S.bInvertY = I % 10 == 0;
		S.CursorSpeed = std::round(50.0 + Unit(I, 3) * 150.0) / 100.0;
		S.StickDeadzone = I % 13 == 0 ? Pc::SettingsDeadzoneMin : std::max(std::round(5.0 + Unit(I, 4) * 45.0) / 100.0, Pc::SettingsDeadzoneMin);
		const std::string Text = Pc::SettingsWrite(S);
		Pc::PcSettings Back;
		Back.bVsync = !S.bVsync; // nothing may survive from the defaults
		REQUIRE(Pc::SettingsLoad(Text, Back));
		CAPTURE(Text);
		CHECK(Back.MasterVolume == S.MasterVolume);
		CHECK(Back.MusicVolume == S.MusicVolume);
		CHECK(Back.SfxVolume == S.SfxVolume);
		CHECK(Back.bMuted == S.bMuted);
		CHECK(Back.OutputDevice == S.OutputDevice);
		CHECK(Back.bFullscreen == S.bFullscreen);
		CHECK(Back.bVsync == S.bVsync);
		CHECK(Back.GraphicsQuality == S.GraphicsQuality);
		CHECK(Back.MaxFps == S.MaxFps);
		CHECK(Back.bEffectsEnabled == S.bEffectsEnabled);
		CHECK(Back.Language == S.Language);
		CHECK(Back.ResolutionIndex == S.ResolutionIndex);
		CHECK(Back.LastSlot == S.LastSlot);
		CHECK(Back.bConfirmReawaken == S.bConfirmReawaken);
		CHECK(Back.bTutorialTips == S.bTutorialTips);
		CHECK(Back.PromptStyle == S.PromptStyle);
		CHECK(Back.DockSide == S.DockSide);
		CHECK(Back.bShowPrompts == S.bShowPrompts);
		CHECK(Back.bSwapConfirmBack == S.bSwapConfirmBack);
		CHECK(Back.bSwapSticks == S.bSwapSticks);
		CHECK(Back.bInvertX == S.bInvertX);
		CHECK(Back.bInvertY == S.bInvertY);
		CHECK(Back.CursorSpeed == S.CursorSpeed);
		CHECK(Back.StickDeadzone == S.StickDeadzone);
	}
	// Any double comes back within two units in the last place (measured 2026-11-04: 1,789 of 2,000 exactly, the rest one or two).
	// Godot's reader cannot be hit exactly from every decimal, nor can Godot's own files; no value the game holds is such a
	// double (the sliders hold steps; a Godot file's values come back exactly, checked in the replay above).
	int Exact = 0;
	for (int I = 0; I < 2000; ++I)
	{
		Pc::PcSettings S;
		S.MasterVolume = (Unit(I, 5) - 0.25) * 3.0;
		Pc::PcSettings Back;
		REQUIRE(Pc::SettingsLoad(Pc::SettingsWrite(S), Back));
		const double Ulp = std::nextafter(std::fabs(S.MasterVolume), 1e9) - std::fabs(S.MasterVolume);
		CHECK(std::fabs(Back.MasterVolume - S.MasterVolume) <= 2.0 * Ulp);
		Exact += Back.MasterVolume == S.MasterVolume ? 1 : 0;
	}
	MESSAGE("arbitrary doubles read back exactly: " << Exact << " of 2000");
	CHECK(Exact > 1700);
	Pc::PcSettings Kept;
	Kept.MusicVolume = 0.3;
	CHECK_FALSE(Pc::SettingsLoad("", Kept));
	CHECK_FALSE(Pc::SettingsLoad("\x89PNG\r\n\x1a\n", Kept));
	CHECK(Kept.MusicVolume == 0.3);
	// Godot's remapping and anything else settings never hold: skipped, brackets balanced across lines.
	Pc::PcSettings B;
	REQUIRE(Pc::SettingsLoad("[bindings]\n\npause=[\"key:4194305\", Object(InputEventKey,\"keycode\":4194305)]\nmap={\n\"a\": [1, 2],\n\"b\": \"]\"\n}\n"
		"[audio]\nmusic_volume=0.25 ; a comment\nsfx_volume=true\nmaster_volume=Vector2(1, 2)\n", B));
	CHECK(B.MusicVolume == 0.25);
	CHECK(B.SfxVolume == 1.0);    // the wrong kind keeps what was held
	CHECK(B.MasterVolume == 1.0);
}

TEST_CASE("Settings: the device played to is the stored one while the system lists it, else the system's (_apply_output_device)")
{
	// Godot's AudioServer.get_output_device_list() always begins with "Default"; the port's list does too.
	const std::vector<std::string> System = { "Default", "Speakers (Realtek(R) Audio)", "Headphones (Wireless Controller)" };
	Pc::PcSettings S;
	CHECK(Pc::SettingsOutputWant(System, S) == "Default");
	S.OutputDevice = "Speakers (Realtek(R) Audio)";
	CHECK(Pc::SettingsOutputWant(System, S) == "Speakers (Realtek(R) Audio)");
	S.OutputDevice = "Headphones (Wireless Controller)";
	CHECK(Pc::SettingsOutputWant(System, S) == "Headphones (Wireless Controller)");
	// Unplugged since it was saved: the system's device, never silence; the setting is kept (the list still shows it).
	S.OutputDevice = "USB Speakers";
	CHECK(Pc::SettingsOutputWant(System, S) == "Default");
	CHECK(S.OutputDevice == "USB Speakers");
	const Pc::PcSettingsOutputList L = Pc::SettingsOutputList(System, S);
	CHECK(L.Devices.size() == 4);
	CHECK(L.Selected == 3);
	// A name is matched whole: a prefix or another case is another device.
	S.OutputDevice = "Speakers";
	CHECK(Pc::SettingsOutputWant(System, S) == "Default");
	S.OutputDevice = "speakers (realtek(r) audio)";
	CHECK(Pc::SettingsOutputWant(System, S) == "Default");
	// No device at all (a headless run): "Default" still.
	S.OutputDevice = "Speakers (Realtek(R) Audio)";
	CHECK(Pc::SettingsOutputWant({ "Default" }, S) == "Default");
	CHECK(Pc::SettingsOutputWant({}, S) == "Default");
}

#endif
