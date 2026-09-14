#pragma once
#include "CoreMinimal.h"

// Stable gameplay roles. Model object names and pivot offsets live in the
// model-owned runtime descriptor, not in station input code.
namespace HarpoonModelContract
{
inline const FName YawTag(TEXT("Harpoon.Model.Yaw"));
inline const FName PitchTag(TEXT("Harpoon.Model.Pitch"));
inline const FName SightTag(TEXT("Harpoon.Model.Sight"));
inline const FName SightComponent(TEXT("HarpoonSight"));
inline const FVector FabricatorBoxSize(164.0, 164.0, 12.0);
inline const FVector FabricatorBoxCenter(0.0, 0.0, 6.0);
}
