#include "UI/PawUiMemory.h"

#include "Core/PawGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UPawUiMemory* UPawUiMemory::Of(const UObject* Context)
{
	const UWorld* World = Context ? Context->GetWorld() : nullptr;
	const UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (!GI)
	{
		GI = Cast<UGameInstance>(Context);
	}
	return GI ? GI->GetSubsystem<UPawUiMemory>() : nullptr;
}

int64& UPawUiMemory::JobsSeen()
{
	const UGameInstance* GI = GetGameInstance();
	const UPawGameSubsystem* G = GI ? GI->GetSubsystem<UPawGameSubsystem>() : nullptr;
	const int64 Epoch = G ? G->ProgressEpoch() : 0;
	if (Epoch != SeenEpoch)
	{
		JobsSeenValue = -1; // another game: its own jobs are not news (HUD._jobs_seen = -1)
		SeenEpoch = Epoch;
	}
	return JobsSeenValue;
}
