// The procedural mesh the backdrop's patches and puddles and the three effects bands draw with (M_BackdropFill).
//
// UE 5.8's UProceduralMeshComponent never tells the engine which materials it draws (it has no CollectPSOPrecacheData, as
// the static, instanced and skinned meshes do), so nothing precached its pipeline state and the packaged game compiled it
// at the realm's first frame, on the render thread: a hitch on a player's PC (`tools/packaged_pso.py`: "PSO PRECACHING MISS",
// ShadersOnly, M_BackdropFill, FLocalVertexFactory, "Material not precached yet"; HANDOFF-LOG (142)). This one names each
// of its materials with the local vertex factory the procedural mesh's proxy draws with (FProcMeshProxy). UMeshComponent's
// SetMaterial asks again for a new material, and while the compile runs the engine waits to create the proxy
// (r.PSOPrecache.ProxyCreationStrategy 1, the default) instead of stalling a frame. A material set before the first
// section counts (the renderer sets it as the mesh is made, so the request starts as the realm appears).
#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "PawFillMesh.generated.h"

UCLASS()
class UPawFillMeshComponent : public UProceduralMeshComponent
{
	GENERATED_BODY()

public:
	virtual void CollectPSOPrecacheData(const FPSOPrecacheParams& BasePrecachePSOParams, FMaterialInterfacePSOPrecacheParamsList& OutParams) override;
};
