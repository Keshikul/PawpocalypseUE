#include "Platform/PawPlatformSubsystem.h"

#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "PcStats.h"
#include "Platform/PawSteam.h"
#include "Realm/PawRealmSubsystem.h"

void UPawPlatformSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UPawGameSubsystem>();
	if (!IsActive())
	{
		return;
	}
	OverlayHandle = PawSteam::OnOverlay().AddWeakLambda(this, [this](bool bActive) { OnOverlay.Broadcast(bActive); });
	StatsHandle = PawSteam::OnStatsReady().AddWeakLambda(this, [this]() { SyncAchievements(true); });
	Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		Poll();
		return true;
	}), 1.0f);
}

void UPawPlatformSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(Ticker);
	PawSteam::OnOverlay().Remove(OverlayHandle);
	PawSteam::OnStatsReady().Remove(StatsHandle);
	Super::Deinitialize();
}

bool UPawPlatformSubsystem::IsActive()
{
	return PawSteam::IsActive();
}

FString UPawPlatformSubsystem::SaveUserFolder()
{
	return PawSteam::SteamId64();
}

void UPawPlatformSubsystem::OpenWebPage(const FString& Url)
{
	if (!PawSteam::OpenOverlayWebPage(Url))
	{
		FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
	}
}

FString UPawPlatformSubsystem::PlatformLanguage()
{
	return PawSteam::GameLanguage();
}

TOptional<Pc::PcFamily> UPawPlatformSubsystem::FamilyFromPlatform(FName Family)
{
	if (Family == FName(TEXT("PlayStation")))
	{
		return Pc::PcFamily::PlayStation;
	}
	if (Family == FName(TEXT("SteamDeck")))
	{
		return Pc::PcFamily::SteamDeck;
	}
	if (Family == FName(TEXT("Xbox")))
	{
		return Pc::PcFamily::Xbox;
	}
	if (Family.IsNone())
	{
		return TOptional<Pc::PcFamily>();
	}
	return Pc::PcFamily::Generic; // Switch, DirectInput pads, anything newer: Xbox's prompts (doc 05: an unknown pad draws as Xbox)
}

TOptional<Pc::PcFamily> UPawPlatformSubsystem::TranslatedPadFamily()
{
	if (!IsActive())
	{
		return TOptional<Pc::PcFamily>();
	}
	// Steam's pad list, read at most once a second (a pad can be plugged in, or Steam Input switched on, while playing).
	static double NextAsk = 0.0;
	static FName Last;
	const double Now = FPlatformTime::Seconds();
	if (Now >= NextAsk)
	{
		NextAsk = Now + 1.0;
		Last = PawSteam::TranslatedPadFamily();
	}
	return FamilyFromPlatform(Last);
}

FString UPawPlatformSubsystem::PresenceToken(bool bInRealm, const FString& BiomeId)
{
	return bInRealm && !BiomeId.IsEmpty() ? FString::Printf(TEXT("#Biome_%s"), *BiomeId) : FString();
}

void UPawPlatformSubsystem::Poll()
{
	SyncAchievements(false);
	SyncPresence();
}

void UPawPlatformSubsystem::SyncAchievements(bool bForce)
{
	const UPawGameSubsystem* G = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPawGameSubsystem>() : nullptr;
	if (!G || !G->IsDataReady())
	{
		return;
	}
	const Pc::PcProgress& P = G->Progress();
	const int64 Earned = static_cast<int64>(P.AchievementsEarned.size());
	// A new achievement, a load or new game (the epoch), Steam's achievements arriving, or a write that had to wait.
	if (!bForce && !bAchievementsRetry && Earned == SeenEarned && G->ProgressEpoch() == SeenEpoch)
	{
		return;
	}
	SeenEarned = Earned;
	SeenEpoch = G->ProgressEpoch();
	TArray<FString> Names;
	for (const std::string& Id : P.AchievementsEarned)
	{
		Names.Add(UTF8_TO_TCHAR(Id.c_str()));
	}
	bool bRetry = false;
	AchievementsSet += PawSteam::WriteAchievements(Names, bRetry);
	if (bRetry && !bAchievementsRetry)
	{
		UE_LOG(LogPaw, Log, TEXT("Steam: achievements not written yet (Steam has not loaded them, or an id is not on Steamworks); trying again"));
	}
	bAchievementsRetry = bRetry;
}

void UPawPlatformSubsystem::SyncPresence()
{
	const UGameInstance* GI = GetGameInstance();
	const UWorld* World = GI ? GI->GetWorld() : nullptr;
	const UPawRealmSubsystem* Realm = World ? World->GetSubsystem<UPawRealmSubsystem>() : nullptr;
	const UPawGameSubsystem* G = GI ? GI->GetSubsystem<UPawGameSubsystem>() : nullptr;
	const bool bInRealm = Realm && Realm->IsRunning() && G && G->IsDataReady();
	const FString Token = PresenceToken(bInRealm, bInRealm ? FString(UTF8_TO_TCHAR(G->Progress().CurrentBiomeId.c_str())) : FString());
	if (bPresenceSent && Token == Presence)
	{
		return;
	}
	Presence = Token;
	bPresenceSent = true;
	PawSteam::SetPresence(Token);
	UE_LOG(LogPaw, Log, TEXT("Steam: presence %s"), Token.IsEmpty() ? TEXT("(none)") : *Token);
}
