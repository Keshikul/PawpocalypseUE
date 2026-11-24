#include "Render/PawFillMesh.h"

#include "LocalVertexFactory.h"
#include "Materials/MaterialInterface.h"
#include "PSOPrecache.h"

void UPawFillMeshComponent::CollectPSOPrecacheData(const FPSOPrecacheParams& BasePrecachePSOParams, FMaterialInterfacePSOPrecacheParamsList& OutParams)
{
	FPSOPrecacheParams Params = BasePrecachePSOParams;
	Params.bCastShadow = CastShadow;
	// GetNumMaterials() counts mesh sections; a material set before the first section is an override only.
	const int32 Count = FMath::Max(GetNumMaterials(), GetNumOverrideMaterials());
	for (int32 I = 0; I < Count; ++I)
	{
		UMaterialInterface* Material = GetMaterial(I);
		if (!Material)
		{
			continue;
		}
		FMaterialInterfacePSOPrecacheParams Entry;
		Entry.MaterialInterface = Material;
		Entry.VertexFactoryDataList.Add(FPSOPrecacheVertexFactoryData(&FLocalVertexFactory::StaticType));
		Entry.PSOPrecacheParams = Params;
		AddMaterialInterfacePSOPrecacheParamsToList(Entry, OutParams);
	}
}
