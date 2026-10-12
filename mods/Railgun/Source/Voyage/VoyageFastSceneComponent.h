#pragma once
// Steam25827816 / UE5.8, full fingerprint in ../../ARCHITECTURE.md.
// Public HC26 mapping: StaticMeshComponent parent, ZERO own properties.
// Stock VehicleMesh identity and shared constructor evidence; no stock mesh.
#include "Components/StaticMeshComponent.h"
#include "VoyageFastSceneComponent.generated.h"
UCLASS()
class VOYAGE_API UVoyageFastSceneComponent : public UStaticMeshComponent
{
    GENERATED_BODY()
};
