#pragma once
// Steam25191271 / UE5.8, full fingerprint in GAME_DERIVED_SOURCES.md.
// Public HC26 mapping: StaticMeshComponent parent, ZERO own properties.
// Stock VehicleMesh identity and shared constructor evidence; no stock mesh.
#include "Components/StaticMeshComponent.h"
#include "VoyageFastSceneComponent.generated.h"
UCLASS()
class VOYAGE_API UVoyageFastSceneComponent : public UStaticMeshComponent
{
    GENERATED_BODY()
};
