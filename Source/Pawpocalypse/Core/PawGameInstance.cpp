#include "Core/PawGameInstance.h"

#include "Core/PawLog.h"
#include "PcCore.h"

void UPawGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogPaw, Log, TEXT("Pawpocalypse started; rules library: %hs"), Pc::LibraryName());
}
