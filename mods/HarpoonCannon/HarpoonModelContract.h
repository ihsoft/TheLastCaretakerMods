#pragma once
#include "CoreMinimal.h"

// Stable gameplay roles. Model object names and pivot offsets live in the
// model-owned GLB registry, not in station input code.
namespace HarpoonModelContract
{
inline const FName YawTag(TEXT("Harpoon.Model.Yaw"));
inline const FName PitchTag(TEXT("Harpoon.Model.Pitch"));
inline const FName MuzzleTag(TEXT("Harpoon.Model.Muzzle"));
inline const FName SightTag(TEXT("Harpoon.Model.Sight"));
inline const FName SightComponent(TEXT("HarpoonSight"));
}
