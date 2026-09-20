#pragma once
#include "CoreMinimal.h"

// Stable gameplay roles. Model object names and pivot offsets live in the
// model-owned GLB registry, not in station input code.
namespace RailgunModelContract
{
inline const FName YawTag(TEXT("Railgun.Model.Yaw"));
inline const FName PitchTag(TEXT("Railgun.Model.Pitch"));
inline const FName MuzzleTag(TEXT("Railgun.Model.Muzzle"));
inline const FName SightTag(TEXT("Railgun.Model.Sight"));
inline const FName SightComponent(TEXT("RailgunSight"));
inline const FName EntryTag(TEXT("Railgun.Model.Entry"));
inline const FName EntryComponent(TEXT("RailgunEntryReference"));
}
