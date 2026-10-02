// Hand-written mod contracts.

#pragma once

#include "CoreMinimal.h"

namespace RailgunAssetNames
{
constexpr TCHAR ShellOnlyParameter[] = TEXT("ShellOnly");
constexpr TCHAR VerifyTaggedParameter[] = TEXT("VerifyTagged");
constexpr TCHAR GamePackagePrefix[] = TEXT("/Game/");
constexpr TCHAR CookedContentDirectory[] = TEXT("Saved/Cooked/Windows/Voyage/Content");
constexpr TCHAR PawnPackageName[] = TEXT("/Game/Mods/Railgun/BP_Railgun");
constexpr TCHAR HudPackageName[] = TEXT("/Game/Mods/Railgun/WBP_Railgun");
constexpr TCHAR LeafProbePackageName[] = TEXT("/Game/Mods/Railgun/Module/BP_Module_Railgun");
constexpr TCHAR LeafProbeAssetName[] = TEXT("BP_Module_Railgun");
constexpr TCHAR BaseMeshPackageName[] = TEXT("/Game/Mods/Railgun/SM_RailgunBase");
constexpr TCHAR YawMeshPackageName[] = TEXT("/Game/Mods/Railgun/SM_RailgunYawAssembly");
constexpr TCHAR PitchMeshPackageName[] = TEXT("/Game/Mods/Railgun/SM_RailgunPitchAssembly");
constexpr TCHAR LoadedConnectorPackageName[] = TEXT("/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out");
constexpr TCHAR LeafItemPackageName[] = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01");
constexpr TCHAR LeafItemAssetName[] = TEXT("DA_Item_Module_RailgunCannonMk01");
constexpr TCHAR ElectricSocketDataObjectPath[] = TEXT("/Game/Data/Assets/ModuleSockets/DA_Socket_ElectricData.DA_Socket_ElectricData");
constexpr TCHAR CameraDronePackageName[] = TEXT("/Game/Blueprints/Vehicles/BP_CameraDrone");

const FName GeneratorLeafProbeName(TEXT("GenerateRailgunLeafProbe"));
const FName ModuleMountRootName(TEXT("ModuleMountCollision"));
const FName DynamicCollisionName(TEXT("VoyageDynamicCollision"));
const FName ElectricComponentName(TEXT("Electric"));
const FName ElectricSocketName(TEXT("ElectricSocket"));
const FName BlockAllDynamicCollisionProfileName(TEXT("BlockAllDynamic"));
const FName NoCollisionProfileName(TEXT("NoCollision"));
constexpr float CollisionHalfWidthCentimeters = 75.0f;
constexpr float CollisionHalfHeightCentimeters = 50.0f;
constexpr bool ModuleMountGeneratesInteractionOverlaps = true;
constexpr double RailgunEnergyConsumptionOn = 1000.0;
constexpr double RailgunEnergyConsumptionStandby = 1000.0;
constexpr double RailgunIdleBufferKJ = 1.0;
constexpr uint32 ElectricSocketId = 2236302826u;
constexpr bool IncludeBaseGameLoadedConnectorReference = false;
constexpr bool UsesStockCameraDroneClass = true;
const FVector CollisionRelativeLocation(0.0, 0.0, 50.0);
}
