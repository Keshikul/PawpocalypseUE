#include "Save/PawSettingsSubsystem.h"

#include "Core/PawLog.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Save/PawSaveStorage.h"
#include "UnrealEngine.h"
#include "Widgets/SWindow.h"

#include <string>

const TCHAR* const UPawSettingsSubsystem::EntryName = TEXT("settings");

namespace
{
	std::string Utf8(const FString& S) { return std::string(TCHAR_TO_UTF8(*S)); }
}

void UPawSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FString Why;
	Storage = PawStorageForThisRun(Why);
	UE_LOG(LogPaw, Log, TEXT("Settings: %s"), *Why);
	// A window named on the command line is the one this run wants (the bench, the tour, a developer's -ResX).
	const TCHAR* Cmd = FCommandLine::Get();
	FString Ignored;
	bWindowFromCommandLine = FParse::Value(Cmd, TEXT("ResX="), Ignored) || FParse::Value(Cmd, TEXT("ResY="), Ignored)
		|| FParse::Param(Cmd, TEXT("windowed")) || FParse::Param(Cmd, TEXT("fullscreen"));
	Reload();
	// _ready(): load_settings() then apply_all(). The window exists one frame later.
	StartupApply = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		StartupApply.Reset();
		ApplyVideo();
		return false;
	}));
}

void UPawSettingsSubsystem::Deinitialize()
{
	if (StartupApply.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(StartupApply);
		StartupApply.Reset();
	}
	Super::Deinitialize();
}

void UPawSettingsSubsystem::Reload()
{
	Settings = Pc::PcSettings();
	TArray<uint8> Bytes;
	if (Storage.IsValid() && Storage->Read(EntryName, Bytes))
	{
		const std::string Text(reinterpret_cast<const char*>(Bytes.GetData()), static_cast<size_t>(Bytes.Num()));
		if (!Pc::SettingsLoad(Text, Settings))
		{
			// ConfigFile.load() failing: the defaults, and the file is left for the player to fix or for the next save.
			UE_LOG(LogPaw, Warning, TEXT("Settings: the settings file is unreadable; the defaults are in use"));
			Settings = Pc::PcSettings();
		}
	}
	// Whoever already listens acts on what was read (at startup nobody does yet: the device subsystem reads them itself).
	OnChanged.Broadcast();
}

void UPawSettingsSubsystem::UseStorageForTest(TSharedPtr<IPawSaveStorage> InStorage)
{
	Storage = InStorage;
	Writes = 0;
	Video = FPawVideoApplied();
	Reload();
}

void UPawSettingsSubsystem::Save()
{
	// save_settings(): the whole file, every time (the one writer: a second writer would lose changes).
	if (!Storage.IsValid())
	{
		return;
	}
	const std::string Text = Pc::SettingsWrite(Settings);
	if (Storage->Write(EntryName, TArray<uint8>(reinterpret_cast<const uint8*>(Text.data()), static_cast<int32>(Text.size()))))
	{
		++Writes;
	}
	else
	{
		UE_LOG(LogPaw, Error, TEXT("Settings: failed to save the settings"));
	}
}

void UPawSettingsSubsystem::Changed(bool bVideo)
{
	Save();
	if (bVideo)
	{
		ApplyVideo();
	}
	OnChanged.Broadcast();
}

bool UPawSettingsSubsystem::MayApplyToEngine(bool bEditor, bool bStored, bool bViewport)
{
	return !bEditor && bStored && bViewport;
}

bool UPawSettingsSubsystem::WindowMonitor(const FDisplayMetrics& Metrics, const FVector2D& WindowCentre, FPlatformRect& OutDisplay,
	FPlatformRect& OutWork)
{
	// The monitor holding the window's centre (review CR20: fullscreen and the window's fit used the primary display wherever
	// the window was); off every monitor, the primary.
	for (const FMonitorInfo& M : Metrics.MonitorInfo)
	{
		const FPlatformRect& R = M.DisplayRect;
		if (WindowCentre.X >= R.Left && WindowCentre.X < R.Right && WindowCentre.Y >= R.Top && WindowCentre.Y < R.Bottom)
		{
			OutDisplay = R;
			OutWork = M.WorkArea;
			return true;
		}
	}
	OutDisplay = FPlatformRect(0, 0, Metrics.PrimaryDisplayWidth, Metrics.PrimaryDisplayHeight);
	OutWork = Metrics.PrimaryDisplayWorkAreaRect;
	return false;
}

void UPawSettingsSubsystem::ApplyVideo()
{
	// apply_video(): V-Sync, the frame cap (max(cap, 0): a file's negative cap is no cap), then the window.
	Video.bVsync = Settings.bVsync;
	Video.MaxFps = FMath::Max<int64>(Settings.MaxFps, 0);
	Video.bFullscreen = Settings.bFullscreen;
	const Pc::PcIntSize Want = Pc::SettingsResolution(Settings.ResolutionIndex);
	Video.Width = static_cast<int32>(Want.X);
	Video.Height = static_cast<int32>(Want.Y);
	++Video.Count;
	Video.bToEngine = MayApplyToEngine(GIsEditor, Storage.IsValid(), GEngine && GEngine->GameViewport);
	if (!Video.bToEngine)
	{
		return;
	}
	if (IConsoleVariable* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync")))
	{
		VSync->Set(Settings.bVsync ? 1 : 0, ECVF_SetByGameSetting);
	}
	if (IConsoleVariable* Cap = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))
	{
		Cap->Set(static_cast<float>(Video.MaxFps), ECVF_SetByGameSetting);
	}
	if (bWindowFromCommandLine || !FSlateApplication::IsInitialized())
	{
		return;
	}
	FDisplayMetrics Metrics;
	FDisplayMetrics::RebuildDisplayMetrics(Metrics);
	TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow();
	FVector2D Centre = FVector2D::ZeroVector;
	if (Window.IsValid())
	{
		const FVector2D At = Window->GetPositionInScreen();
		const FVector2D Size = Window->GetSizeInScreen();
		Centre = At + Size * 0.5;
	}
	FPlatformRect Display, Work;
	WindowMonitor(Metrics, Centre, Display, Work);
	if (Settings.bFullscreen)
	{
		FSystemResolution::RequestResolutionChange(Display.Right - Display.Left, Display.Bottom - Display.Top, EWindowMode::WindowedFullscreen);
		return;
	}
	// The CLIENT area that fits on the screen once the title bar and borders are added, centred, never off the top or left.
	Pc::PcIntSize Decoration{ 0, 0 };
	if (Window.IsValid())
	{
		const FVector2D Client(static_cast<double>(Want.X), static_cast<double>(Want.Y));
		const FVector2D Whole = Window->GetWindowSizeFromClientSize(Client);
		Decoration = { static_cast<int64_t>(Whole.X - Client.X), static_cast<int64_t>(Whole.Y - Client.Y) };
	}
	const Pc::PcIntSize Usable{ Work.Right - Work.Left, Work.Bottom - Work.Top };
	const Pc::PcIntSize Size = Pc::FitToScreen(Want, Usable, Decoration);
	Video.Width = static_cast<int32>(Size.X);
	Video.Height = static_cast<int32>(Size.Y);
	FSystemResolution::RequestResolutionChange(Video.Width, Video.Height, EWindowMode::Windowed);
	if (Window.IsValid())
	{
		const Pc::PcIntSize At = Pc::CentredPosition({ Work.Left, Work.Top }, Usable, Size);
		Window->MoveWindowTo(FVector2D(static_cast<double>(At.X), static_cast<double>(At.Y)));
	}
}

// --- the setters ---------------------------------------------------------------------------------------------------------

void UPawSettingsSubsystem::SetMasterVolume(double V) { Settings.MasterVolume = Pc::SettingsClampVolume(V); Changed(false); }
void UPawSettingsSubsystem::SetMusicVolume(double V) { Settings.MusicVolume = Pc::SettingsClampVolume(V); Changed(false); }
void UPawSettingsSubsystem::SetSfxVolume(double V) { Settings.SfxVolume = Pc::SettingsClampVolume(V); Changed(false); }
void UPawSettingsSubsystem::SetMuted(bool b) { Settings.bMuted = b; Changed(false); }
void UPawSettingsSubsystem::SetOutputDevice(const FString& Name) { Settings.OutputDevice = Utf8(Name); Changed(false); }
void UPawSettingsSubsystem::SetFullscreen(bool b) { Settings.bFullscreen = b; Changed(true); }
void UPawSettingsSubsystem::SetVsync(bool b) { Settings.bVsync = b; Changed(true); }
void UPawSettingsSubsystem::SetResolutionIndex(int64 Index) { Settings.ResolutionIndex = Pc::SettingsClampResolution(Index); Changed(true); }
void UPawSettingsSubsystem::SetGraphicsQuality(int64 Quality) { Settings.GraphicsQuality = Pc::SettingsClampQuality(Quality); Changed(false); }
void UPawSettingsSubsystem::SetMaxFps(int64 Fps) { Settings.MaxFps = Pc::SettingsClampFps(Fps); Changed(true); }
void UPawSettingsSubsystem::SetConfirmReawaken(bool b) { Settings.bConfirmReawaken = b; Changed(false); }


namespace
{
	// set_control(): an equal value changes nothing (no save, no signal); any other is stored as given (Clamp: the sliders).
	template <typename T>
	bool SetControl(T& Held, T Value)
	{
		if (Held == Value)
		{
			return false;
		}
		Held = Value;
		return true;
	}
}

void UPawSettingsSubsystem::SetShowPrompts(bool b) { if (SetControl(Settings.bShowPrompts, b)) { Changed(false); } }
void UPawSettingsSubsystem::SetTutorialTips(bool b) { if (SetControl(Settings.bTutorialTips, b)) { Changed(false); } }
void UPawSettingsSubsystem::SetSwapConfirmBack(bool b) { if (SetControl(Settings.bSwapConfirmBack, b)) { Changed(false); } }
void UPawSettingsSubsystem::SetSwapSticks(bool b) { if (SetControl(Settings.bSwapSticks, b)) { Changed(false); } }
void UPawSettingsSubsystem::SetInvertX(bool b) { if (SetControl(Settings.bInvertX, b)) { Changed(false); } }
void UPawSettingsSubsystem::SetInvertY(bool b) { if (SetControl(Settings.bInvertY, b)) { Changed(false); } }

void UPawSettingsSubsystem::SetCursorSpeed(double V)
{
	// The early return compares the value GIVEN, before the clamp (Godot's order).
	if (Settings.CursorSpeed != V)
	{
		Settings.CursorSpeed = FMath::Clamp(V, Pc::SettingsCursorSpeedMin, Pc::SettingsCursorSpeedMax);
		Changed(false);
	}
}

void UPawSettingsSubsystem::SetStickDeadzone(double V)
{
	if (Settings.StickDeadzone != V)
	{
		Settings.StickDeadzone = FMath::Clamp(V, Pc::SettingsDeadzoneMin, Pc::SettingsDeadzoneMax);
		Changed(false);
	}
}

void UPawSettingsSubsystem::SetBindings(const Pc::PcSavedBindings& Bindings)
{
	Settings.Bindings = Bindings;
	Changed(false);
}

void UPawSettingsSubsystem::ResetControls()
{
	Settings.Bindings.clear();
	Settings.bShowPrompts = true;
	Settings.bTutorialTips = true;
	Settings.bSwapConfirmBack = false;
	Settings.bSwapSticks = false;
	Settings.bInvertX = false;
	Settings.bInvertY = false;
	Settings.CursorSpeed = 1.0;
	Settings.StickDeadzone = 0.2;
	Changed(false);
}

void UPawSettingsSubsystem::SetEffectsEnabled(bool b)
{
	if (Settings.bEffectsEnabled != b)
	{
		Settings.bEffectsEnabled = b;
		Changed(false);
	}
}

void UPawSettingsSubsystem::SetLastSlot(int64 Slot)
{
	if (Settings.LastSlot != Slot)
	{
		Settings.LastSlot = Pc::SettingsClampSlot(Slot);
		Changed(false);
	}
}

void UPawSettingsSubsystem::SetPromptStyle(const FString& Style)
{
	const std::string S = Utf8(Style);
	if (Pc::SettingsIsPromptStyle(S) && Settings.PromptStyle != S)
	{
		Settings.PromptStyle = S;
		Changed(false);
	}
}

void UPawSettingsSubsystem::SetDockSide(const FString& Side)
{
	const std::string S = Utf8(Side);
	if (Pc::SettingsIsDockSide(S) && Settings.DockSide != S)
	{
		Settings.DockSide = S;
		Changed(false);
	}
}
