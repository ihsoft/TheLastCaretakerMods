#pragma once

namespace StableGyroAssetNames
{
inline constexpr TCHAR ResetActionAssetName[] = TEXT("IAV_GyroPitchReset");
inline constexpr TCHAR ResetActionObjectPath[] = TEXT("/Game/Mods/StableGyro/IAV_GyroPitchReset.IAV_GyroPitchReset");
inline constexpr TCHAR ResetActionPackageName[] = TEXT("/Game/Mods/StableGyro/IAV_GyroPitchReset");
inline constexpr TCHAR ThrottleUpActionObjectPath[] =
    TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterThrottleUp.IA_GyroCopterThrottleUp");
inline constexpr TCHAR ThrottleDownActionObjectPath[] =
    TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterThrottleDown.IA_GyroCopterThrottleDown");
inline constexpr TCHAR ResetDisplayLabel[] = TEXT("Reset pitch");
inline constexpr TCHAR ResetMappingName[] = TEXT("GyroPitchReset");
inline constexpr TCHAR ResetRussianLabel[] = TEXT("Сбросить наклон");
inline constexpr TCHAR VehicleInputCategory[] = TEXT("Vehicle");
inline constexpr TCHAR DropCargoActionObjectPath[] = TEXT("/Game/Game/Input/Vehicle/IA_GyroCopterDropCargo.IA_GyroCopterDropCargo");
inline constexpr TCHAR DropCargoMappingName[] = TEXT("DropCargo");
inline constexpr TCHAR ResetActionWasActiveFieldName[] = TEXT("ResetActionWasActive");
}
