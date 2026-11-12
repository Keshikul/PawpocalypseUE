#include "Render/PawSpriteAtlas.h"

FString UPawSpriteAtlas::StemOf(const FString& Path)
{
	return FPaths::GetBaseFilename(Path);
}

void UPawSpriteAtlas::Index() const
{
	// Built once for the cells as they are (review CR16: counting entries against cells rebuilt it, and the rigs, on every
	// lookup when two cells shared an id). Loading or editing the asset builds it again.
	if (bIndexed && IndexedCells == Cells.Num())
	{
		return;
	}
	bIndexed = true;
	IndexedCells = Cells.Num();
	++IndexBuilds;
	ById.Reset();
	Rigs.Reset();
	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		ById.Add(Cells[I].Id, I);
	}
	for (int32 I = 0; I < Cells.Num(); ++I)
	{
		const FPawAtlasCell& C = Cells[I];
		const int32* Drawing = C.RigOf.IsEmpty() ? nullptr : ById.Find(C.RigOf);
		if (!Drawing)
		{
			continue;
		}
		FPawRigLayer L;
		L.Cell = I;
		L.Order = C.RigOrder;
		L.PivotU = C.PivotU;
		L.PivotV = C.PivotV;
		L.bLift = C.Motion == TEXT("lift");
		L.AmplitudeDeg = C.Amplitude;
		L.Hz = C.Hz;
		L.Phase = C.Phase;
		Rigs.FindOrAdd(*Drawing).Add(L);
	}
	for (TPair<int32, TArray<FPawRigLayer>>& R : Rigs)
	{
		R.Value.Sort([](const FPawRigLayer& A, const FPawRigLayer& B) { return A.Order < B.Order; });
	}
}

void UPawSpriteAtlas::PostLoad()
{
	Super::PostLoad();
	bIndexed = false;
}

#if WITH_EDITOR
void UPawSpriteAtlas::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	Super::PostEditChangeProperty(Event);
	bIndexed = false;
}
#endif

int32 UPawSpriteAtlas::Find(const FString& Id) const
{
	++Finds;
	Index();
	const FString Key = Id.Contains(TEXT("/")) || Id.Contains(TEXT(".")) ? StemOf(Id) : Id;
	const int32* Found = ById.Find(Key);
	return Found ? *Found : INDEX_NONE;
}

const TArray<FPawRigLayer>* UPawSpriteAtlas::Rig(int32 DrawingCell) const
{
	Index();
	return Rigs.Find(DrawingCell);
}
