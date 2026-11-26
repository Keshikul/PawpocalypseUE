#include "Save/PawSettingsSubsystem.h"

#include "Core/PawLog.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "PcLocales.h"
#include "Platform/PawPlatformSubsystem.h"
#include "Save/PawSaveStorage.h"
#include "UI/PawText.h"
#include "UnrealEngine.h"
#include "Widgets/SWindow.h"

#include <string>
#include <vector>

const TCHAR* const UPawSettingsSubsystem::EntryName = TEXT("settings");

namespace
{
	std::string Utf8(const FString& S) { return std::string(TCHAR_TO_UTF8(*S)); }

	// Q10 (designer, 2026-11-23): an idle game is left running behind other windows for hours; it need not draw at full
	// speed there. The fixed-step simulation keeps time exactly at any frame rate (PawRealmSim: up to 8 steps a frame).
	TAutoConsoleVariable<int32> CVarBackgroundFps(TEXT("paw.BackgroundFps"), 30,
		TEXT("The frame cap while the game's window is not active (unfocused or minimised); 0 = none. The player's own lower cap "
			"is kept."), ECVF_Default);

	// t.MaxFPS set at the priority it holds, or the game setting's when lower: a cap the command line set (the bench, a
	// tour) is lowered and given back, not refused for its priority.
	void SetFrameCap(IConsoleVariable* Cap, double Fps)
	{
		const uint32 Held = static_cast<uint32>(Cap->GetFlags()) & static_cast<uint32>(ECVF_SetByMask);
		const uint32 By = FMath::Max<uint32>(Held, static_cast<uint32>(ECVF_SetByGameSetting));
		Cap->Set(static_cast<float>(Fps), static_cast<EConsoleVariableFlags>(By));
	}
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
		// Started behind another window: in the background from the start.
		if (FSlateApplication::IsInitialized() && !FSlateApplication::Get().IsActive())
		{
			OnActivationChanged(false);
		}
		return false;
	}));
	if (FSlateApplication::IsInitialized())
	{
		ActivationHandle = FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this,
			&UPawSettingsSubsystem::OnActivationChanged);
	}
}

void UPawSettingsSubsystem::Deinitialize()
{
	if (ActivationHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationHandle);
	}
	ActivationHandle.Reset();
	OnActivationChanged(true); // the cap given back
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
	// apply_all()'s language: before anything listening rebuilds its text.
	ApplyLanguage();
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

void UPawSettingsSubsystem::OnActivationChanged(bool bActive)
{
	bAppActive = bActive;
	ApplyBackgroundCap();
}

void UPawSettingsSubsystem::ApplyBackgroundCap()
{
	const int64 BackgroundFps = CVarBackgroundFps.GetValueOnGameThread();
	const bool bWant = !bAppActive && BackgroundFps > 0;
	IConsoleVariable* Cap = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"));
	const bool bMay = Cap && MayCapInBackground(GIsEditor, GEngine && GEngine->GameViewport);
	if (bWant)
	{
		if (!Video.bBackground)
		{
			// The cap in force now: the one the engine holds (the player's, or the command line's).
			Video.CapBefore = Cap ? FMath::RoundToInt64(Cap->GetFloat()) : 0;
		}
		Video.bBackground = true;
		Video.BackgroundCap = Pc::BackgroundFrameCap(Video.CapBefore, BackgroundFps);
		Video.bBackgroundToEngine = bMay;
		if (bMay)
		{
			SetFrameCap(Cap, static_cast<double>(Video.BackgroundCap));
		}
	}
	else if (Video.bBackground)
	{
		Video.bBackground = false;
		if (bMay)
		{
			SetFrameCap(Cap, static_cast<double>(Video.CapBefore));
		}
	}
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
		if (Video.bBackground)
		{
			// The player changed the cap while the window was behind (a settings file edited, a second instance): the
			// new cap comes back with focus; the background cap follows it now.
			Video.bBackground = false;
			ApplyBackgroundCap();
		}
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

void UPawSettingsSubsystem::ApplyLanguage()
{
	// apply_language(): the setting when available, else the system's (Pc::LocaleResolve). A -culture= on the command line
	// stands in for the setting (the tour, the store shots, a developer); a culture with no translations is English.
	std::vector<std::string> Available;
	for (const FString& Code : PawText::Languages())
	{
		Available.push_back(Utf8(Code));
	}
	std::string Setting = Settings.Language;
	FString Wanted;
	if (FParse::Value(FCommandLine::Get(), TEXT("culture="), Wanted))
	{
		const Pc::PcLanguage* Asked = Pc::LanguageByCulture(Utf8(Wanted));
		Setting = Asked ? Asked->Code : Pc::LocaleEnglish;
	}
	// Under Steam, "system" is the language chosen for the game in Steam (doc 10 §5; empty in every other build).
	const std::string Code = Pc::LocaleResolve(Setting, Available, Utf8(FPlatformMisc::GetDefaultLocale()),
		Utf8(UPawPlatformSubsystem::PlatformLanguage()));
	const Pc::PcLanguage* Shown = Pc::LanguageByCode(Code);
	Language.Code = UTF8_TO_TCHAR(Code.c_str());
	Language.Culture = Shown ? UTF8_TO_TCHAR(Shown->Culture) : TEXT("en");
	++Language.Count;
	// Never in the editor: the designer's own editor keeps its language (the tests show a culture through PawText).
	Language.bToEngine = !GIsEditor;
	if (Language.bToEngine)
	{
		// A culture ICU has no data for is refused and the words stay as they were (a package staged without the
		// language's ICU data: InternationalizationPreset, docs/10 §1; paw.Languages checks every one in the package).
		if (!PawText::ShowCulture(Language.Culture))
		{
			UE_LOG(LogPaw, Error, TEXT("Settings: the engine has no culture '%s' (its ICU data not staged?); the words stay in '%s'"),
				*Language.Culture, *FInternationalization::Get().GetCurrentLanguage()->GetName());
		}
		UE_LOG(LogPaw, Log, TEXT("Settings: language %s (%s)"), *Language.Code, *Language.Culture);
	}
}

void UPawSettingsSubsystem::SetLanguage(const FString& Code)
{
	const std::string S = Utf8(Code);
	if (Settings.Language == S)
	{
		return;
	}
	Settings.Language = S;
	Save();
	ApplyLanguage();
	OnChanged.Broadcast();
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
