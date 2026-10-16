#pragma once
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace RailgunAds
{
inline const FName Holster(TEXT("RailgunADSServiceHolster"));
inline const FName ServiceItem(TEXT("RailgunADSServiceItem"));
inline const FName Equipment(TEXT("RailgunADSServiceActor"));
inline const FName Amount(TEXT("RailgunADSAmount"));
inline const FName Ready(TEXT("RailgunADSServiceReady"));
inline const FName Accepted(TEXT("RailgunADSOverlayAccepted"));
inline const FName Ensure(TEXT("EnsureRailgunADSService"));
inline const FName Reset(TEXT("ResetRailgunADSService"));
inline const FName OverlayKey(TEXT("RailgunADSService"));
inline const FName OverridePriority(TEXT("Priority"));
inline const FName OverrideItem(TEXT("Item"));
inline const FName InKey(TEXT("InKey"));
inline const FName LiteralValue(TEXT("Value"));
inline const FName InOverride(TEXT("InOverride"));
inline const FName ForceAdd(TEXT("bForceAdd"));
inline const FName Ducked(TEXT("bDucked"));
inline const FName InType(TEXT("InType"));
inline const FName IsSet(TEXT("bIsSet"));
inline constexpr TCHAR OverlayEnum[] = TEXT("EVoyageHolsterSlotType::Overlay");
inline constexpr TCHAR AdsEnum[] = TEXT("EVoyageEquipmentInterpStateType::ADS");
inline constexpr TCHAR Priority[] = TEXT("100");
inline constexpr TCHAR False[] = TEXT("false");
inline constexpr TCHAR True[] = TEXT("true");
inline constexpr TCHAR AcceptanceDiagnostic[] = TEXT("Railgun ADS overlay request was rejected");
}

void AddRailgunAdsSupport(UBlueprint* BP);
void SetRailgunAdsState(FGraph& G, UClass* StationClass, bool Enabled);
UEdGraphPin* ReadRailgunAdsAmount(FGraph& G, UClass* StationClass);
UEdGraphPin* ReadRailgunAdsActive(FGraph& G, UClass* StationClass);
void SyncRailgunAdsService(FGraph& G, UClass* StationClass);
}
