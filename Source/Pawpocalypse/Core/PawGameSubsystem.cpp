#include "Core/PawGameSubsystem.h"

#include "Core/PawLog.h"
#include "Data/PawDataAdapter.h"
#include "PcData.h"
#include "PcProgress.h"
#include "PcStats.h"

UPawGameSubsystem::UPawGameSubsystem() = default;
UPawGameSubsystem::~UPawGameSubsystem() = default;

void UPawGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GameData = MakeUnique<Pc::PcGameData>();
	Model = MakeUnique<Pc::PcProgress>();
	Errors.Reset();
	bDataReady = FPawDataAdapter::BuildGameData(*GameData, Errors) && !GameData->Tiers.empty();
	// Loud in development, logged in Shipping (docs/03 §6): a game on broken data is a bug report waiting to happen.
	for (const FString& E : Errors)
	{
#if UE_BUILD_SHIPPING
		UE_LOG(LogPaw, Warning, TEXT("Game data: %s"), *E);
#else
		UE_LOG(LogPaw, Error, TEXT("Game data: %s"), *E);
#endif
	}
	if (bDataReady)
	{
		NewGame();
		UE_LOG(LogPaw, Log, TEXT("Game data ready: %d biomes, %d enemies"), static_cast<int32>(GameData->Tiers.size()),
			static_cast<int32>(GameData->Enemies.size()));
	}
}

void UPawGameSubsystem::NewGame()
{
	MarkProgressReplaced();
	if (bDataReady)
	{
		Pc::NewGame(*GameData, *Model);
	}
	SlotState = Pc::PcSlotExtras();
}
