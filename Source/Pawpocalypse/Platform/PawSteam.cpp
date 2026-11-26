#include "Platform/PawSteam.h"

#include "Containers/Ticker.h"
#include "Core/PawLog.h"
#include "HAL/PlatformMisc.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if PAW_WITH_STEAMWORKS
#include "SteamSharedModule.h"
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END
#endif

namespace
{
	TMulticastDelegate<void(bool)> GPawSteamOverlay;
	FSimpleMulticastDelegate GPawSteamStatsReady;

	// An automated run never talks to Steam (docs/09 §1 rule 4: the designer's account stays as it is). -PawNoSteam: a
	// developer's run of the Steam build without it.
	bool PawSteam_AutomatedRun()
	{
		return GIsAutomationTesting || FApp::IsUnattended() || FParse::Param(FCommandLine::Get(), TEXT("PawNoSteam"));
	}
}

#if PAW_WITH_STEAMWORKS

namespace
{
	// Holds the Steam callbacks; SteamAPI_RunCallbacks (the ticker below) delivers them on the game thread.
	class FPawSteamCallbacks
	{
	public:
		FPawSteamCallbacks() = default;

	private:
		STEAM_CALLBACK(FPawSteamCallbacks, HandleOverlay, GameOverlayActivated_t);
		STEAM_CALLBACK(FPawSteamCallbacks, HandleStats, UserStatsReceived_t);
	};

	void FPawSteamCallbacks::HandleOverlay(GameOverlayActivated_t* Data)
	{
		const bool bActive = Data && Data->m_bActive != 0;
		UE_LOG(LogPaw, Log, TEXT("Steam: overlay %s"), bActive ? TEXT("opened") : TEXT("closed"));
		GPawSteamOverlay.Broadcast(bActive);
	}

	void FPawSteamCallbacks::HandleStats(UserStatsReceived_t* Data)
	{
		if (!Data || Data->m_nGameID != static_cast<uint64>(PawSteam::AppId))
		{
			return;
		}
		UE_LOG(LogPaw, Log, TEXT("Steam: achievements loaded (%d)"), static_cast<int32>(Data->m_eResult));
		if (Data->m_eResult == k_EResultOK)
		{
			GPawSteamStatsReady.Broadcast();
		}
	}

	bool GPawSteamActive = false;
	TUniquePtr<FPawSteamCallbacks> GPawSteamCallbacks;
	FTSTicker::FDelegateHandle GPawSteamTicker;
	bool GPawSteamInputTried = false;
	bool GPawSteamInputReady = false;

	// steam_api is delay-loaded from the engine's ThirdParty folder: SteamShared loads it (a call before that would crash).
	// The SteamShared plugin's module loads at PreDefault, before the game module.
	bool PawSteam_DllsLoaded()
	{
		return FSteamSharedModule::IsAvailable() && FSteamSharedModule::Get().AreSteamDllsLoaded();
	}
}

bool PawSteam::IsSteamBuild()
{
	return PAW_STEAM_BUILD != 0;
}

bool PawSteam::RestartThroughSteamIfNeeded()
{
#if UE_BUILD_SHIPPING
	if (!IsSteamBuild() || PawSteam_AutomatedRun())
	{
		return false;
	}
	if (!PawSteam_DllsLoaded())
	{
		UE_LOG(LogPaw, Warning, TEXT("Steam: steam_api not loaded, cannot check how the game was started"));
		return false;
	}
	if (SteamAPI_RestartAppIfNecessary(AppId))
	{
		UE_LOG(LogPaw, Log, TEXT("Steam: started outside Steam, asking Steam to start app %u and exiting"), AppId);
		return true;
	}
#endif
	return false;
}

void PawSteam::Startup()
{
	if (GPawSteamActive || !IsSteamBuild())
	{
		return;
	}
	if (PawSteam_AutomatedRun())
	{
		UE_LOG(LogPaw, Log, TEXT("Steam: off (an automated run or -PawNoSteam)"));
		return;
	}
	if (!PawSteam_DllsLoaded())
	{
		UE_LOG(LogPaw, Warning, TEXT("Steam: steam_api not loaded"));
		return;
	}
#if !UE_BUILD_SHIPPING
	// Started from the exe, not by Steam: the API takes the App ID from here (no steam_appid.txt to forget in a package).
	FPlatformMisc::SetEnvironmentVar(TEXT("SteamAppId"), *FString::FromInt(static_cast<int32>(AppId)));
#endif
	SteamErrMsg Error;
	const ESteamAPIInitResult Result = SteamAPI_InitEx(&Error);
	if (Result != k_ESteamAPIInitResult_OK)
	{
		UE_LOG(LogPaw, Warning, TEXT("Steam: the API did not start (%d): %s"), static_cast<int32>(Result), UTF8_TO_TCHAR(Error));
		return;
	}
	GPawSteamActive = true;
	GPawSteamCallbacks = MakeUnique<FPawSteamCallbacks>();
	GPawSteamTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
	{
		if (GPawSteamActive)
		{
			SteamAPI_RunCallbacks();
		}
		return true;
	}));
	UE_LOG(LogPaw, Log, TEXT("Steam: on, app %u, user %s, language %s%s"), AppId, *SteamId64(), *GameLanguage(),
		IsOnSteamDeck() ? TEXT(", Steam Deck") : TEXT(""));
}

void PawSteam::Shutdown()
{
	if (!GPawSteamActive)
	{
		return;
	}
	FTSTicker::GetCoreTicker().RemoveTicker(GPawSteamTicker);
	GPawSteamCallbacks.Reset();
	if (GPawSteamInputReady && SteamInput())
	{
		SteamInput()->Shutdown();
	}
	GPawSteamActive = false;
	SteamAPI_Shutdown();
	UE_LOG(LogPaw, Log, TEXT("Steam: off"));
}

bool PawSteam::IsActive()
{
	return GPawSteamActive;
}

FString PawSteam::SteamId64()
{
	ISteamUser* User = GPawSteamActive ? SteamUser() : nullptr;
	return User ? FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(User->GetSteamID().ConvertToUint64())) : FString();
}

FString PawSteam::GameLanguage()
{
	ISteamApps* Apps = GPawSteamActive ? SteamApps() : nullptr;
	const char* Language = Apps ? Apps->GetCurrentGameLanguage() : nullptr;
	return Language ? FString(UTF8_TO_TCHAR(Language)) : FString();
}

bool PawSteam::IsOnSteamDeck()
{
	ISteamUtils* Utils = GPawSteamActive ? SteamUtils() : nullptr;
	return Utils && Utils->IsSteamRunningOnSteamDeck();
}

FName PawSteam::TranslatedPadFamily()
{
	ISteamInput* Input = GPawSteamActive ? SteamInput() : nullptr;
	if (!Input)
	{
		return NAME_None;
	}
	if (!GPawSteamInputTried)
	{
		// Init(false): SteamAPI_RunCallbacks runs ISteamInput::RunFrame. It does not move the game to the Steam Input API (that
		// needs an action manifest): pads keep coming in as XInput (KB S/03 "Glyphs through Steam Input").
		GPawSteamInputTried = true;
		GPawSteamInputReady = Input->Init(false);
	}
	if (!GPawSteamInputReady)
	{
		return NAME_None;
	}
	for (int32 Slot = 0; Slot < 4; ++Slot)
	{
		const InputHandle_t Handle = Input->GetControllerForGamepadIndex(Slot);
		if (Handle == 0)
		{
			continue;
		}
		switch (Input->GetInputTypeForHandle(Handle))
		{
		case k_ESteamInputType_PS3Controller:
		case k_ESteamInputType_PS4Controller:
		case k_ESteamInputType_PS5Controller:
			return FName(TEXT("PlayStation"));
		case k_ESteamInputType_SteamDeckController:
			return FName(TEXT("SteamDeck"));
		case k_ESteamInputType_SwitchProController:
			return FName(TEXT("Switch"));
		case k_ESteamInputType_XBox360Controller:
		case k_ESteamInputType_XBoxOneController:
			return FName(TEXT("Xbox"));
		default:
			return FName(TEXT("Generic"));
		}
	}
	return NAME_None;
}

int32 PawSteam::WriteAchievements(const TArray<FString>& ApiNames, bool& bOutRetry)
{
	bOutRetry = false;
	ISteamUserStats* Stats = GPawSteamActive ? SteamUserStats() : nullptr;
	if (!Stats)
	{
		return 0;
	}
	int32 Set = 0;
	for (const FString& Name : ApiNames)
	{
		const FTCHARToUTF8 Api(*Name);
		bool bHave = false;
		if (!Stats->GetAchievement(Api.Get(), &bHave))
		{
			// The account's achievements are not loaded yet, or Steamworks has no such API name.
			bOutRetry = true;
			continue;
		}
		if (!bHave && Stats->SetAchievement(Api.Get()))
		{
			UE_LOG(LogPaw, Log, TEXT("Steam: achievement %s set"), *Name);
			++Set;
		}
	}
	if (Set > 0 && !Stats->StoreStats())
	{
		UE_LOG(LogPaw, Warning, TEXT("Steam: storing %d achievements failed"), Set);
		bOutRetry = true;
	}
	return Set;
}

void PawSteam::SetPresence(const FString& Token)
{
	ISteamFriends* Friends = GPawSteamActive ? SteamFriends() : nullptr;
	if (!Friends)
	{
		return;
	}
	if (Token.IsEmpty())
	{
		Friends->ClearRichPresence();
		return;
	}
	Friends->SetRichPresence("steam_display", TCHAR_TO_UTF8(*Token));
}

bool PawSteam::OpenOverlayWebPage(const FString& Url)
{
	ISteamUtils* Utils = GPawSteamActive ? SteamUtils() : nullptr;
	ISteamFriends* Friends = GPawSteamActive ? SteamFriends() : nullptr;
	if (!Utils || !Friends || !Utils->IsOverlayEnabled())
	{
		return false;
	}
	Friends->ActivateGameOverlayToWebPage(TCHAR_TO_UTF8(*Url));
	return true;
}

#else // no Steamworks on this platform: no Steam

bool PawSteam::IsSteamBuild() { return false; }
bool PawSteam::RestartThroughSteamIfNeeded() { return false; }
void PawSteam::Startup() {}
void PawSteam::Shutdown() {}
bool PawSteam::IsActive() { return false; }
FString PawSteam::SteamId64() { return FString(); }
FString PawSteam::GameLanguage() { return FString(); }
bool PawSteam::IsOnSteamDeck() { return false; }
FName PawSteam::TranslatedPadFamily() { return NAME_None; }
int32 PawSteam::WriteAchievements(const TArray<FString>&, bool& bOutRetry) { bOutRetry = false; return 0; }
void PawSteam::SetPresence(const FString&) {}
bool PawSteam::OpenOverlayWebPage(const FString&) { return false; }

#endif

TMulticastDelegate<void(bool)>& PawSteam::OnOverlay()
{
	return GPawSteamOverlay;
}

FSimpleMulticastDelegate& PawSteam::OnStatsReady()
{
	return GPawSteamStatsReady;
}
