#include "Realm/PawRealmSubsystem.h"

#include "Core/PawGameSubsystem.h"
#include "Core/PawLog.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "PcCardDraw.h"
#include "PcField.h"
#include "PcHud.h"

bool UPawRealmSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

UPawGameSubsystem* UPawRealmSubsystem::GetGame() const
{
	const UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UPawGameSubsystem>() : nullptr;
}

bool UPawRealmSubsystem::StartRealm(uint64 Seed)
{
	UPawGameSubsystem* Game = GetGame();
	if (!Game || !Game->IsDataReady())
	{
		UE_LOG(LogPaw, Error, TEXT("Realm: no game data, the field cannot start"));
		return false;
	}
	// A fresh sim: its travel count seeds each biome, so a second start must not inherit the first one's (the bounds are
	// the window's and stay).
	Sim = FPawRealmSim();
	Remeasure(); // the bounds belong to the window and the dock, not to the run
	// A session replays from its seed (bug reports quote it: the debug HUD shows it).
	Sim.BaseSeed = Seed != 0 ? Seed : static_cast<uint64>(FPlatformTime::Cycles64());
	// SettingsManager's defaults until phase 4's settings: auto-bait on, auto-travel and auto-pick off.
	Sim.Input.bAutoBaitSetting = true;
	Sim.bPaused = PauseHolds.Num() > 0; // a fresh sim must not run under a screen that holds the pause
	Sim.Start(Game->Data(), Game->Progress());
	Sim.Input.CatX = Sim.Field.CatX;
	Sim.Input.CatY = Sim.Field.CatY;
	bRunning = true;
	UE_LOG(LogPaw, Log, TEXT("Realm started in %hs (seed %llu, %.2f ms)"), Game->Progress().CurrentBiomeId.c_str(),
		static_cast<unsigned long long>(Sim.BaseSeed), Sim.LastTravelMs);
	return true;
}

void UPawRealmSubsystem::AdvanceFrame(double RealDelta)
{
	UPawGameSubsystem* Game = GetGame();
	if (!bRunning || !Game)
	{
		return;
	}
	// The slot's automations, every frame: a load or a new game changes them under a running field.
	Sim.Input.bAutoTravelSetting = Game->Slot().bAutoTravel;
	Sim.Input.bAutoPickSetting = Game->Slot().bAutoPickCard;
	Sim.Advance(Game->Data(), Game->Progress(), RealDelta);
}

void UPawRealmSubsystem::SetAutoTravel(bool bOn)
{
	if (UPawGameSubsystem* Game = GetGame())
	{
		Game->Slot().bAutoTravel = bOn;
	}
	Sim.Input.bAutoTravelSetting = bOn;
}

bool UPawRealmSubsystem::GetAutoTravel() const
{
	const UPawGameSubsystem* Game = GetGame();
	return Game ? Game->Slot().bAutoTravel : Sim.Input.bAutoTravelSetting;
}

void UPawRealmSubsystem::SetAutoPick(bool bOn)
{
	if (UPawGameSubsystem* Game = GetGame())
	{
		Game->Slot().bAutoPickCard = bOn;
	}
	Sim.Input.bAutoPickSetting = bOn;
}

bool UPawRealmSubsystem::GetAutoPick() const
{
	const UPawGameSubsystem* Game = GetGame();
	return Game ? Game->Slot().bAutoPickCard : Sim.Input.bAutoPickSetting;
}

void UPawRealmSubsystem::SetViewSize(double LogicalW, double LogicalH)
{
	ViewW = LogicalW;
	ViewH = LogicalH;
	Remeasure();
}

void UPawRealmSubsystem::SetDock(const Pc::PcRect& Dock, bool bDockOnLeft)
{
	DockRect = Dock;
	bDockLeft = bDockOnLeft;
	Remeasure();
}

void UPawRealmSubsystem::Remeasure()
{
	if (const UPawGameSubsystem* Game = GetGame(); Game && Game->IsDataReady())
	{
		Sim.SetBounds(Pc::PlayfieldRect(Game->Data(), ViewW, ViewH, DockRect, 0.0, bDockLeft));
	}
}

void UPawRealmSubsystem::SetCat(double X, double Y)
{
	Sim.Input.CatX = X;
	Sim.Input.CatY = Y;
}

bool UPawRealmSubsystem::Travel(int32 Index)
{
	UPawGameSubsystem* Game = GetGame();
	return bRunning && Game && Sim.Travel(Game->Data(), Game->Progress(), Index);
}

bool UPawRealmSubsystem::SummonBoss()
{
	UPawGameSubsystem* Game = GetGame();
	if (!bRunning || !Game || !Pc::SummonFieldBoss(Sim.Field, Game->Data(), Game->Progress()))
	{
		return false;
	}
	Sim.EvaluateAchievements(Game->Data(), Game->Progress()); // the bait's Catnip: currency_changed
	return true;
}

void UPawRealmSubsystem::OnReawakened()
{
	if (UPawGameSubsystem* Game = GetGame(); bRunning && Game)
	{
		Sim.OnReawakened(Game->Data(), Game->Progress());
	}
}

bool UPawRealmSubsystem::ChooseCard(const FString& CardId)
{
	UPawGameSubsystem* Game = GetGame();
	if (!Game || !Game->IsDataReady())
	{
		return false;
	}
	const Pc::PcTake Take = Pc::ChooseCard(Game->Data(), Game->Progress(), std::string(TCHAR_TO_UTF8(*CardId)));
	if (!Take.bTaken)
	{
		return false;
	}
	if (bRunning)
	{
		Pc::RefreshFieldStats(Sim.Field, Game->Data(), Game->Progress());
		// A card that costs kittens (Lone Hunter) takes them at once (TierController on card_taken; F14, 2026-11-03).
		Pc::SyncFieldKittens(Sim.Field, Game->Data(), Game->Progress());
	}
	return true;
}

bool UPawRealmSubsystem::PickJob(const FString& JobId, bool bSecond)
{
	UPawGameSubsystem* Game = GetGame();
	if (!Game || !Game->IsDataReady())
	{
		return false;
	}
	const bool bAgain = Pc::PickJob(Game->Data(), Game->Progress(), bSecond, std::string(TCHAR_TO_UTF8(*JobId)));
	if (bRunning)
	{
		Pc::RefreshFieldStats(Sim.Field, Game->Data(), Game->Progress());
	}
	return bAgain;
}

bool UPawRealmSubsystem::RerollHand()
{
	UPawGameSubsystem* Game = GetGame();
	if (!Game || !Game->IsDataReady())
	{
		return false;
	}
	// Every deal is its own roll stream off the field's seed (PcRngCardRolls): a session replays from its seed.
	Pc::PcRngCardRolls Rolls(Sim.Field.Seed, ++Sim.Field.Deals);
	return Pc::RerollHand(Game->Data(), Game->Progress(), Rolls);
}

bool UPawRealmSubsystem::BanishFromHand(const FString& CardId)
{
	UPawGameSubsystem* Game = GetGame();
	if (!Game || !Game->IsDataReady())
	{
		return false;
	}
	Pc::PcRngCardRolls Rolls(Sim.Field.Seed, ++Sim.Field.Deals);
	return Pc::BanishFromHand(Game->Data(), Game->Progress(), std::string(TCHAR_TO_UTF8(*CardId)), Rolls);
}

void UPawRealmSubsystem::SetPauseHold(FName Holder, bool bHold)
{
	if (bHold)
	{
		PauseHolds.Add(Holder);
	}
	else
	{
		PauseHolds.Remove(Holder);
	}
	Sim.bPaused = PauseHolds.Num() > 0;
}

void UPawRealmSubsystem::SetPounceDown(bool bDown)
{
	if (!bDown)
	{
		// Let go: the next press is the player's own, once nothing holds the field.
		bPounceSwallowed = PauseHolds.Num() > 0 && bPounceSwallowed;
	}
	else if (PauseHolds.Num() > 0 || bPounceSwallowed)
	{
		bPounceSwallowed = true;
		bDown = false;
	}
	Sim.Input.bPounceDown = bDown;
}

void UPawRealmSubsystem::OnProgressChanged(bool bKittensMayHaveChanged)
{
	if (UPawGameSubsystem* Game = GetGame(); bRunning && Game)
	{
		if (bKittensMayHaveChanged)
		{
			Pc::SyncFieldKittens(Sim.Field, Game->Data(), Game->Progress());
		}
		Pc::RefreshFieldStats(Sim.Field, Game->Data(), Game->Progress());
	}
	if (UPawGameSubsystem* Game = GetGame(); Game && Game->IsDataReady())
	{
		Sim.EvaluateAchievements(Game->Data(), Game->Progress()); // upgrade_purchased / currency_changed / branch_leveled_up
	}
}
