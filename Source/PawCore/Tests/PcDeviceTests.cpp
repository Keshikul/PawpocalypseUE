#if defined(PAWCORE_STANDALONE_TESTS)

#include "ThirdParty/doctest/doctest.h"
#include "PcCsv.h"
#include "PcDevice.h"

#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// The device rules against the REAL input_device.gd (fixture `input_device`): its observe() fed synthesized events with real
// time between some, every step replayed here at the clock it ran at, and the whole state compared after each. Each step
// started on a fresh millisecond; a step whose clock still ticked on while it ran (`after` != `now`, a couple in 7,000) is
// replayed at either reading and must match at one of them.

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

	// One string field of a flat JSON object as Godot wrote it ({"vendor_id":"1118","raw_name":"..."}); "" when absent.
	std::string JsonField(const std::string& J, const std::string& Key)
	{
		const std::string Tag = "\"" + Key + "\":\"";
		const size_t At = J.find(Tag);
		if (At == std::string::npos)
		{
			return std::string();
		}
		std::string Out;
		for (size_t I = At + Tag.size(); I < J.size() && J[I] != '"'; ++I)
		{
			if (J[I] == '\\' && I + 1 < J.size())
			{
				++I;
			}
			Out.push_back(J[I]);
		}
		return Out;
	}

	long long Int(const std::string& V) { return std::atoll(V.c_str()); }
	double Hex(const std::string& V) { return std::strtod(V.c_str(), nullptr); }
	bool Flag(const std::string& V) { return V == "1"; }

	const char* ToastText(Pc::PcDeviceToast T)
	{
		return T == Pc::PcDeviceToast::Connected ? "Controller connected" : T == Pc::PcDeviceToast::Disconnected ? "Controller disconnected" : "";
	}

	using FRow = std::map<std::string, std::string>;

	// One step of the fixture applied to S at the clock Now. Returns false for a guard that disagreed.
	bool Apply(Pc::PcDeviceState& S, const FRow& V, int64_t Now)
	{
		const std::string& Op = V.at("op");
		Pc::PcDeviceEvent E;
		if (Op == "reset")
		{
			S = Pc::PcDeviceState();
			S.PromptStyle = V.at("a");
			S.bSwapSticks = Flag(V.at("b"));
			S.BootMs = Int(V.at("c"));
			Pc::DeviceRefreshFamily(S);
		}
		else if (Op == "register")
		{
			const std::string J = Unescape(V.at("b"));
			const std::string Vid = JsonField(J, "vendor_id"), Pid = JsonField(J, "product_id");
			Pc::DeviceRegister(S, Int(V.at("a")), Pc::DeviceFamilyFromInfo(Vid.empty() ? 0 : Int(Vid), Pid.empty() ? 0 : Int(Pid),
				Unescape(V.at("c")), JsonField(J, "raw_name"), false));
		}
		else if (Op == "key")
		{
			E.Kind = Pc::PcDeviceEventKind::Key;
			E.bPressed = Flag(V.at("b"));
			E.bEcho = Flag(V.at("c"));
			E.bPressAction = Flag(V.at("e"));
			Pc::DeviceObserve(S, E, Now);
		}
		else if (Op == "click")
		{
			E.Kind = Pc::PcDeviceEventKind::MouseButton;
			E.bPressed = Flag(V.at("b"));
			Pc::DeviceObserve(S, E, Now);
		}
		else if (Op == "motion")
		{
			E.Kind = Pc::PcDeviceEventKind::MouseMotion;
			E.MotionLength = Hex(V.at("a"));
			Pc::DeviceObserve(S, E, Now);
		}
		else if (Op == "pad_button")
		{
			E.Kind = Pc::PcDeviceEventKind::PadButton;
			E.Device = Int(V.at("a"));
			E.bPressed = Flag(V.at("b"));
			E.bPressAction = Flag(V.at("e"));
			Pc::DeviceObserve(S, E, Now);
		}
		else if (Op == "pad_motion")
		{
			E.Kind = Pc::PcDeviceEventKind::PadMotion;
			E.Device = Int(V.at("a"));
			E.Axis = Int(V.at("b"));
			E.AxisValue = Hex(V.at("c"));
			E.bPressAction = V.at("e")[0] == '1';
			E.bPressPressed = V.at("e")[1] == '1';
			Pc::DeviceObserve(S, E, Now);
		}
		else if (Op == "warp") { Pc::DeviceWarp(S, Now); }
		else if (Op == "claim_stick") { Pc::DeviceClaimStick(S); }
		else if (Op == "joy") { Pc::DeviceConnection(S, Int(V.at("a")), Flag(V.at("b")), Pc::PcFamily::Generic, Now); }
		else if (Op == "style")
		{
			S.PromptStyle = V.at("a");
			Pc::DeviceRefreshFamily(S);
		}
		else if (Op == "guard")
		{
			return Pc::DevicePressCountsFor(S, Int(V.at("a")), Now) == Flag(V.at("b"));
		}
		return true;
	}

	bool Same(const Pc::PcDeviceState& S, const FRow& V)
	{
		return static_cast<int>(S.Family) == Int(V.at("family")) && static_cast<int>(S.Detected) == Int(V.at("detected"))
			&& static_cast<int>(S.Cursor) == Int(V.at("cursor")) && S.ActiveDevice == Int(V.at("active"))
			&& S.bLastWasPointer == Flag(V.at("pointer_last")) && S.bPointerHidden == Flag(V.at("hidden"))
			&& S.MouseTravel == Hex(V.at("travel")) && S.PressBeganMs == Int(V.at("press_began"))
			&& S.bPressAxisDown == Flag(V.at("press_axis")) && S.LastMotionMs == Int(V.at("last_motion"))
			&& S.WarpUntilMs == Int(V.at("warp_until")) && (S.Toast != Pc::PcDeviceToast::None) == Flag(V.at("toast"))
			&& std::string(ToastText(S.Toast)) == Unescape(V.at("toast_text"));
	}

	struct FResult
	{
		int Rows = 0, Steps = 0, StickClaims = 0, MouseClaims = 0, EchoesIgnored = 0, Toasts = 0, Guards = 0, Pinned = 0, LaterClock = 0;
	};

	FResult Replay(const std::string& Path)
	{
		FResult R;
		std::string Text;
		REQUIRE_MESSAGE(Pc::ReadTextFile(Path, Text), "missing " << Path);
		std::istringstream In(Text);
		std::string Line;
		std::vector<std::string> Header;
		Pc::PcDeviceState S;
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
			FRow V;
			for (size_t I = 0; I < C.size(); ++I)
			{
				V[Header[I]] = C[I];
			}
			const std::string& Op = V.at("op");
			const int64_t Now = Int(V.at("now"));
			const int64_t After = Int(V.at("after"));
			CAPTURE(V.at("row"));
			CAPTURE(V.at("step"));
			CAPTURE(Op);
			const Pc::PcCursorSource CursorBefore = S.Cursor;
			R.EchoesIgnored += (Op == "motion" && Now < S.WarpUntilMs) ? 1 : 0;
			Pc::PcDeviceState Next = S;
			bool bGuard = Apply(Next, V, Now);
			if (After != Now && !(bGuard && Same(Next, V)))
			{
				// The clock ticked on while Godot ran this step: it may have read the later millisecond.
				Pc::PcDeviceState Later = S;
				if (Apply(Later, V, After) && Same(Later, V))
				{
					Next = Later;
					bGuard = true;
					++R.LaterClock;
				}
			}
			S = Next;
			CHECK(bGuard);
			R.Rows += Op == "reset" ? 1 : 0;
			R.Pinned += (Op == "reset" && S.PromptStyle != "auto") ? 1 : 0;
			R.Steps += (Op != "reset" && Op != "register") ? 1 : 0;
			R.Guards += Op == "guard" ? 1 : 0;
			R.StickClaims += (CursorBefore != Pc::PcCursorSource::Stick && S.Cursor == Pc::PcCursorSource::Stick) ? 1 : 0;
			R.MouseClaims += (CursorBefore != Pc::PcCursorSource::Mouse && S.Cursor == Pc::PcCursorSource::Mouse) ? 1 : 0;
			R.Toasts += (Op == "joy" && S.Toast != Pc::PcDeviceToast::None) ? 1 : 0;
			CHECK(static_cast<int>(S.Family) == Int(V.at("family")));
			CHECK(static_cast<int>(S.Detected) == Int(V.at("detected")));
			CHECK(static_cast<int>(S.Cursor) == Int(V.at("cursor")));
			CHECK(S.ActiveDevice == Int(V.at("active")));
			CHECK(S.bLastWasPointer == Flag(V.at("pointer_last")));
			CHECK(S.bPointerHidden == Flag(V.at("hidden")));
			CHECK(S.MouseTravel == Hex(V.at("travel")));
			CHECK(S.PressBeganMs == Int(V.at("press_began")));
			CHECK(S.bPressAxisDown == Flag(V.at("press_axis")));
			CHECK(S.LastMotionMs == Int(V.at("last_motion")));
			CHECK(S.WarpUntilMs == Int(V.at("warp_until")));
			CHECK((S.Toast != Pc::PcDeviceToast::None) == Flag(V.at("toast")));
			CHECK(std::string(ToastText(S.Toast)) == Unescape(V.at("toast_text")));
		}
		return R;
	}
}

TEST_CASE("Device: every step matches the real input_device.gd")
{
	const FResult R = Replay(DataDir() + "/../Source/PawCore/Tests/Data/input_device_godot.tsv");
	MESSAGE("rows " << R.Rows << ", steps " << R.Steps << ", stick claims " << R.StickClaims << ", mouse claims " << R.MouseClaims
		<< ", warp echoes ignored " << R.EchoesIgnored << ", joy steps with a toast " << R.Toasts << ", guards " << R.Guards
		<< ", rows with prompts pinned " << R.Pinned << ", steps matched at the later millisecond " << R.LaterClock);
	CHECK(R.Rows == 700);
	CHECK(R.StickClaims > 300);
	CHECK(R.MouseClaims > 100);
	CHECK(R.EchoesIgnored > 50);
	CHECK(R.Toasts > 50);
	CHECK(R.Guards > 100);
	CHECK(R.Pinned > 300);
	CHECK(R.LaterClock <= 10);
}

TEST_CASE("Device: family_from_info matches the real input_device.gd")
{
	std::string Text;
	REQUIRE(Pc::ReadTextFile(DataDir() + "/../Source/PawCore/Tests/Data/input_family_godot.tsv", Text));
	std::istringstream In(Text);
	std::string Line;
	bool bHeader = true;
	int Rows = 0;
	std::map<int, int> Seen;
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
		REQUIRE(C.size() == 6);
		CAPTURE(Line);
		const Pc::PcFamily F = Pc::DeviceFamilyFromInfo(Int(C[0]), Int(C[1]), Unescape(C[2]), Unescape(C[3]), Flag(C[4]));
		CHECK(static_cast<int>(F) == Int(C[5]));
		++Seen[static_cast<int>(F)];
		++Rows;
	}
	CHECK(Rows == 3000);
	for (int F = 1; F <= 4; ++F)
	{
		CAPTURE(F);
		CHECK(Seen[F] > 50); // every pad family reached (Xbox, PlayStation, Steam Deck, generic)
	}
}

TEST_CASE("Device: a pad that brings a sound device says where to fix it (_watch_for_pad_audio)")
{
	const std::vector<std::string> Before = { "Default", "Speakers (Realtek(R) Audio)" };
	const std::vector<std::string> WithPad = { "Default", "Speakers (Realtek(R) Audio)", "Headset Earphone (Wireless Controller)" };
	CHECK(Pc::DevicePadAudioWaitMs == 2000);
	// A device appeared and the game follows the system: the toast.
	CHECK(Pc::DevicePadAudioToast(Before, WithPad, "Default"));
	// Wherever it appears in the list.
	CHECK(Pc::DevicePadAudioToast(Before, { "Headset Earphone (Wireless Controller)", "Default", "Speakers (Realtek(R) Audio)" }, "Default"));
	// The player chose a device: nothing can take the sound from it, so no toast (whichever device it is).
	CHECK_FALSE(Pc::DevicePadAudioToast(Before, WithPad, "Speakers (Realtek(R) Audio)"));
	CHECK_FALSE(Pc::DevicePadAudioToast(Before, WithPad, "Headset Earphone (Wireless Controller)"));
	CHECK_FALSE(Pc::DevicePadAudioToast(Before, WithPad, "Unplugged USB"));
	// Nothing new: a pad without audio, the same list, or one that only lost a device.
	CHECK_FALSE(Pc::DevicePadAudioToast(Before, Before, "Default"));
	CHECK_FALSE(Pc::DevicePadAudioToast(WithPad, Before, "Default"));
	CHECK_FALSE(Pc::DevicePadAudioToast(Before, { "Default" }, "Default"));
	// From nothing (no device at the connect) to one: new.
	CHECK(Pc::DevicePadAudioToast({}, { "Default" }, "Default"));
	CHECK_FALSE(Pc::DevicePadAudioToast({}, {}, "Default"));
}

#endif
