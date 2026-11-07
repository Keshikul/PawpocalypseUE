#include "Render/PawSpriteAtlas.h"

FString UPawSpriteAtlas::StemOf(const FString& Path)
{
	return FPaths::GetBaseFilename(Path);
}

int32 UPawSpriteAtlas::Find(const FString& Id) const
{
	if (ById.Num() != Cells.Num())
	{
		ById.Reset();
		for (int32 I = 0; I < Cells.Num(); ++I)
		{
			ById.Add(Cells[I].Id, I);
		}
	}
	const FString Key = Id.Contains(TEXT("/")) || Id.Contains(TEXT(".")) ? StemOf(Id) : Id;
	const int32* Found = ById.Find(Key);
	return Found ? *Found : INDEX_NONE;
}
