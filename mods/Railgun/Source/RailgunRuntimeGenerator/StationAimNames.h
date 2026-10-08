#pragma once
#include "CoreMinimal.h"

namespace Railgun::Runtime
{
// Names for station-local yaw and pitch state.
namespace StationAimNames
{
inline const FName Yaw(TEXT("RailgunLocalAimYaw"));
inline const FName Pitch(TEXT("RailgunLocalAimPitch"));
inline const FName YawComponent(TEXT("RailgunYawComponent"));
inline const FName PitchComponent(TEXT("RailgunPitchComponent"));
inline const FName ModelOwner(TEXT("RailgunAimModelOwner"));
inline const FName BindComponents(TEXT("BindRailgunAimComponents"));
inline const FName ClearReferences(TEXT("ClearRailgunAimReferences"));
}
namespace Aim = StationAimNames;

namespace EyeAim
{
inline const FName Yaw(TEXT("RailgunEyeYaw"));
inline const FName Pitch(TEXT("RailgunEyePitch"));
inline const FName Target(TEXT("RailgunEyeTarget"));
inline const FName FirstPersonCamera(TEXT("RailgunFirstPersonCamera"));
inline const FName FirstPersonCameraOwner(TEXT("RailgunFirstPersonCameraOwner"));
inline const FName EyeLocation(TEXT("OutLocation"));
inline const FName NewLocation(TEXT("NewLocation"));
inline const FName Direction(TEXT("Direction"));
inline const FName RotationVector(TEXT("InVec"));
inline constexpr TCHAR FirstPersonCameraName[] = TEXT("FirstPersonCamera");
}
}
