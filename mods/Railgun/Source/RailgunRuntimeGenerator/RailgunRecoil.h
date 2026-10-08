#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace RailgunRecoil
{
inline const FName CameraStrength(TEXT("RailgunCameraRecoilStrength"));
inline const FName ShipStrength(TEXT("RailgunShipRecoilStrength"));
inline const FName AimAzimuth(TEXT("RailgunRecoilAimAzimuth"));
inline const FName SearchComponent(TEXT("RailgunRecoilSearchComponent"));
inline const FName VisitedComponents(TEXT("RailgunRecoilVisitedComponents"));
inline const FName ShotDirection(TEXT("RailgunRecoilShotDirection"));
inline const FName ModuleLocation(TEXT("RailgunRecoilModuleLocation"));
inline const FName ItemToFindPin(TEXT("ItemToFind"));
inline const FName NewItemPin(TEXT("NewItem"));
inline const FName ImpulsePin(TEXT("Impulse"));
inline const FName LocationPin(TEXT("Location"));
inline const FName BoneNamePin(TEXT("BoneName"));
inline const FName VectorInputPin(TEXT("A"));
inline constexpr TCHAR None[] = TEXT("None");
inline constexpr TCHAR MaximumSearchIndex[] = TEXT("31");
inline constexpr TCHAR MinimumEnabledStrength[] = TEXT("0.0");
inline constexpr TCHAR MaximumAzimuthRadians[] =
    TEXT("6.2831853071795864769");
inline constexpr TCHAR BaselineAimKickDegrees[] = TEXT("2.0");
inline constexpr TCHAR BaselineVelocityChangeCmPerSecond[] = TEXT("-25.0");
inline constexpr TCHAR MaximumFiniteMass[] = TEXT("1000000000000.0");
}

void ApplyRailgunAimRecoil(FGraph& G);

void ClearRailgunRecoilVisited(FGraph& G);

UEdGraphPin* RailgunRecoilVisitedContains(FGraph& G, UEdGraphPin* Item);

void AddRailgunRecoilVisited(FGraph& G, UEdGraphPin* Item);

void ApplyRailgunShipRecoil(FGraph& G, UEdGraphPin* TypedRailgun);
}
