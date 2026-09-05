#pragma once

#include "BlueprintGraphNames.h"

// Reflected engine identities for build-time registry discovery graphs.
namespace RegistryDiscoveryGraphNames
{
inline const FName GetAssetsByPath(TEXT("GetAssetsByPath"));
inline const FName GetAllAssets(TEXT("GetAllAssets"));
inline const FName InSoftObjectPath(TEXT("InSoftObjectPath"));
inline const FName PathString(TEXT("PathString"));
inline const FName SourceString(TEXT("SourceString"));
inline const FName InPrefix(TEXT("InPrefix"));
inline const FName SoftClassPath(TEXT("SoftClassPath"));
inline const FName AssetClass(TEXT("AssetClass"));
inline const FName ScanPathsSynchronous(TEXT("ScanPathsSynchronous"));
inline const FName IsLoadingAssets(TEXT("IsLoadingAssets"));
inline const FName InBool(TEXT("InBool"));
inline const FName InPaths(TEXT("InPaths"));
inline const FName ForceRescan(TEXT("bForceRescan"));
inline const FName IgnoreDenyList(TEXT("bIgnoreDenyListScanFilters"));
inline const FName OnlyOnDisk(TEXT("bIncludeOnlyOnDiskAssets"));
inline const FName OutAssetData(TEXT("OutAssetData"));
inline const FName InAssetData(TEXT("InAssetData"));
inline const FName ForEachLoop(TEXT("ForEachLoop"));
inline const FName Array(TEXT("Array"));
inline const FName ArrayElement(TEXT("Array Element"));
inline const FName NewItem(TEXT("NewItem"));
}
