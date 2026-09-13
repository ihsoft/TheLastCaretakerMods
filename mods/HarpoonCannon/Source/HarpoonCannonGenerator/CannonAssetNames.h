// HAND-WRITTEN MOD CONTRACTS plus current-build shark identities.
// Shark paths are version-bound to Steam build 23962331 and must be
// re-inventoried after a game fingerprint change.

#pragma once

#include "CoreMinimal.h"

namespace CannonAssetNames
{
constexpr TCHAR ShellOnlyParameter[] = TEXT("ShellOnly");
enum class EOperatorLifecycleProbe
{
    NativeActor,
    MinimalVehicleBlueprint,
    PassiveTransformBlueprint,
    InteractiveComponentBlueprint,
    InteractionBoxBlueprint,
    InteractionInterfaceBlueprint,
    StockCameraDroneBlueprint,
    StockCameraDroneActivatedBlueprint,
    StockCameraDroneOffsetBlueprint,
    StockCameraDroneSpawnValidityMarkerBlueprint,
    StockCameraDroneDelayedValidityMarkerBlueprint,
    FullOperatorBlueprint
};

constexpr TCHAR PawnPackageName[] = TEXT("/Game/Mods/HarpoonCannon/BP_HarpoonCannon");
constexpr TCHAR PawnAssetName[] = TEXT("BP_HarpoonCannon");
constexpr TCHAR HudPackageName[] = TEXT("/Game/Mods/HarpoonCannon/WBP_HarpoonCannon");
constexpr TCHAR HudAssetName[] = TEXT("WBP_HarpoonCannon");
constexpr TCHAR LeafProbePackageName[] = TEXT("/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New");
constexpr TCHAR LeafProbeAssetName[] = TEXT("BP_Module_WindTurbine_Medium_New");
constexpr TCHAR BaseMeshPackageName[] = TEXT("/Game/Mods/HarpoonCannon/SM_HarpoonCannonBase");
constexpr TCHAR BaseMeshAssetName[] = TEXT("SM_HarpoonCannonBase");
constexpr TCHAR YawMeshPackageName[] = TEXT("/Game/Mods/HarpoonCannon/SM_HarpoonCannonYawAssembly");
constexpr TCHAR YawMeshAssetName[] = TEXT("SM_HarpoonCannonYawAssembly");
constexpr TCHAR PitchMeshPackageName[] = TEXT("/Game/Mods/HarpoonCannon/SM_HarpoonCannonPitchAssembly");
constexpr TCHAR PitchMeshAssetName[] = TEXT("SM_HarpoonCannonPitchAssembly");
constexpr TCHAR LoadedConnectorPackageName[] = TEXT("/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out");
constexpr TCHAR LoadedConnectorAssetName[] = TEXT("SM_Mooring_CableSocket_Out");
constexpr TCHAR VisualSourceRelativePath[] = TEXT("SourceAssets/harpoon_cannon_blockout.obj");
constexpr TCHAR LeafItemPackageName[] = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium");
constexpr TCHAR LeafItemAssetName[] = TEXT("DA_Item_Module_WindTurbineMedium");
constexpr TCHAR LeafItemObjectPath[] = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.DA_Item_Module_WindTurbineMedium");
constexpr TCHAR CameraDronePackageName[] = TEXT("/Game/Blueprints/Vehicles/BP_CameraDrone");
constexpr TCHAR CameraDroneAssetName[] = TEXT("BP_CameraDrone");

const FName GeneratorBlueprintName(TEXT("GenerateHarpoonCannonPawn"));
const FName GeneratorWidgetName(TEXT("GenerateHarpoonCannonHud"));
const FName GeneratorLeafProbeName(TEXT("GenerateHarpoonCannonLeafProbe"));
const FName GeneratorCameraDroneReferenceName(TEXT("GenerateCameraDroneReferenceStub"));
const FName VisualFallbackMaterialSlotName(TEXT("VisualFallbackMaterial"));
const FName ModuleComponentName(TEXT("ModuleComponent"));
const FName ModuleMountRootName(TEXT("ModuleMountCollision"));
const FName MountRootName(TEXT("MountRoot"));
const FName CollisionProbeName(TEXT("PlacementCollisionProbe"));
const FName DynamicCollisionName(TEXT("VoyageDynamicCollision"));
const FName BaseBodyName(TEXT("HarpoonBaseBody"));
const FName TurretBodyName(TEXT("HarpoonTurretBody"));
const FName BarrelBodyName(TEXT("HarpoonBarrelBody"));
const FName LoadedConnectorName(TEXT("HarpoonLoadedConnector"));
const FName YawPivotName(TEXT("YawPivot"));
const FName PitchPivotName(TEXT("PitchPivot"));
const FName OpticalAxisName(TEXT("OpticalAxis"));
const FName OpticalCameraName(TEXT("OpticalCamera"));
const FName OperatorInteractionBoxName(TEXT("OperatorInteractionBox"));
const FName OperatorInteractionComponentName(TEXT("OperatorInteraction"));
const FName OperatorCharacterLocationName(TEXT("OperatorCharacterLocation"));
const FName OperatorActorVariableName(TEXT("OperatorActor"));
const FName HudRootName(TEXT("OpticalHudRoot"));
const FName ReticleName(TEXT("OpticalReticle"));
const FName DistanceTextName(TEXT("TargetDistanceText"));
const FName BlockAllDynamicCollisionProfileName(TEXT("BlockAllDynamic"));
const FName NoCollisionProfileName(TEXT("NoCollision"));
const FName InteractiveCollisionProfileName(TEXT("Interactive"));
const FName FabricatedActorTag(TEXT("Fabricated"));
const FName ExitComponentTag(TEXT("ExitComponentTag"));
const FName InteractActionFunctionName(TEXT("InteractAction"));
const FName SetDroneActiveFunctionName(TEXT("SetDroneActive"));
const FName SetVisibilityFunctionName(TEXT("SetVisibility"));
const FName DroneActivePinName(TEXT("Active"));
const FName DroneUpdateDeployPinName(TEXT("UpdateDeploy"));
const FName NewRelativeLocationPinName(TEXT("NewRelativeLocation"));
const FName SweepPinName(TEXT("bSweep"));
const FName TeleportPinName(TEXT("bTeleport"));
const FName NewVisibilityPinName(TEXT("bNewVisibility"));
const FName PropagateToChildrenPinName(TEXT("bPropagateToChildren"));
const FName DelayDurationPinName(TEXT("Duration"));
const FName EventCharacterPinName(TEXT("MyCharacter"));
const FName EventPartIdPinName(TEXT("PartId"));
const FName EventInteractIndexPinName(TEXT("InteractIndex"));
const FName PlayerControllerPinName(TEXT("PlayerController"));
const FName CollisionHandlingOverridePinName(TEXT("CollisionHandlingOverride"));
const FName LocationRulePinName(TEXT("LocationRule"));
const FName RotationRulePinName(TEXT("RotationRule"));
const FName ScaleRulePinName(TEXT("ScaleRule"));
const FName WeldSimulatedBodiesPinName(TEXT("bWeldSimulatedBodies"));
const FName TickEnabledPinName(TEXT("bEnabled"));
constexpr TCHAR ReferenceStubObjectName[] = TEXT("ReferenceStub");

constexpr TCHAR ObjVertexPrefix[] = TEXT("v ");
constexpr TCHAR ObjObjectPrefix[] = TEXT("o ");
constexpr TCHAR ObjMaterialPrefix[] = TEXT("usemtl ");
constexpr TCHAR ObjFacePrefix[] = TEXT("f ");
constexpr TCHAR ObjIndexSeparator[] = TEXT("/");

constexpr const TCHAR* StaticMountObjects[] = {
    TEXT("HC_StaticMount")
};
constexpr const TCHAR* YawAssemblyObjects[] = {
    TEXT("HC_RotatingBase"),
    TEXT("HC_RotationRing"),
    TEXT("HC_Yoke_Left"),
    TEXT("HC_Yoke_Right"),
    TEXT("HC_StockMooring_BaseProxy"),
    TEXT("HC_StockMooring_RecessProxy"),
    TEXT("HC_StockMooring_LipProxy"),
    TEXT("HC_FairleadRoller"),
    TEXT("HC_CableProxy_00"),
    TEXT("HC_CableProxy_01"),
    TEXT("HC_CableProxy_02"),
    TEXT("HC_CableProxy_03"),
    TEXT("HC_CableProxy_04"),
    TEXT("HC_CableProxy_05"),
    TEXT("HC_ControlGrip"),
    TEXT("HC_StatusLight")
};
constexpr const TCHAR* PitchAssemblyObjects[] = {
    TEXT("HC_Trunnion"),
    TEXT("HC_ElevationGear_Left"),
    TEXT("HC_ElevationGear_Right"),
    TEXT("HC_PressureChamber"),
    TEXT("HC_PressureBand_Rear"),
    TEXT("HC_PressureBand_Front"),
    TEXT("HC_LinearRam"),
    TEXT("HC_Guide_Bottom"),
    TEXT("HC_Guide_Left"),
    TEXT("HC_Guide_Right"),
    TEXT("HC_MuzzleBrace_Left"),
    TEXT("HC_MuzzleBrace_Right"),
    TEXT("HC_SightPost"),
    TEXT("HC_SightAperture"),
    TEXT("HC_ConnectorProxy_Body"),
    TEXT("HC_ConnectorProxy_Collar"),
    TEXT("HC_ConnectorProxy_Handle_Left"),
    TEXT("HC_ConnectorProxy_Handle_Right"),
    TEXT("HC_ConnectorProxy_Handle_Top")
};

constexpr double FullYawDegrees = 360.0;
constexpr double MinimumPitchDegrees = -50.0;
constexpr double MaximumPitchDegrees = 10.0;
constexpr double BaseHorizontalFovDegrees = 90.0;
constexpr double OpticalMagnification = 5.0;
constexpr float OpticalHorizontalFovDegrees = 22.619865f;
constexpr float CollisionHalfWidthCentimeters = 75.0f;
constexpr float CollisionHalfHeightCentimeters = 50.0f;
constexpr bool ModuleMountGeneratesInteractionOverlaps = true;
constexpr bool BaseMeshBuildsFabricatorInteractionCollision = true;
constexpr bool MovingMeshBuildsFabricatorInteractionCollision = false;
constexpr bool IncludeBaseGameLoadedConnectorReference = false;
constexpr EOperatorLifecycleProbe OperatorLifecycleProbe =
    EOperatorLifecycleProbe::StockCameraDroneDelayedValidityMarkerBlueprint;
constexpr bool UsesStockCameraDroneClass =
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneActivatedBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneOffsetBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneSpawnValidityMarkerBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneDelayedValidityMarkerBlueprint;
constexpr bool ActivatesStockCameraDrone =
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneActivatedBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneOffsetBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneSpawnValidityMarkerBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneDelayedValidityMarkerBlueprint;
constexpr bool OffsetsStockCameraDrone =
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneOffsetBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneSpawnValidityMarkerBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneDelayedValidityMarkerBlueprint;
constexpr bool MarksStockCameraDroneSpawnValidity =
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneSpawnValidityMarkerBlueprint ||
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneDelayedValidityMarkerBlueprint;
constexpr bool ChecksStockCameraDroneDelayedValidity =
    OperatorLifecycleProbe == EOperatorLifecycleProbe::StockCameraDroneDelayedValidityMarkerBlueprint;
const FVector CollisionRelativeLocation(0.0, 0.0, 50.0);
const FVector VisualMeshRelativeLocation(0.0, 0.0, 0.0);
const FVector VisualMeshRelativeScale(1.0, 1.0, 1.0);
const FVector PitchPivotRelativeLocation(-5.0, 0.0, 94.0);
const FVector LoadedConnectorRelativeLocation(101.0, 0.0, 12.0);
const FRotator LoadedConnectorRelativeRotation(0.0, 0.0, 90.0);
const FVector LoadedConnectorRelativeScale(2.0, 2.0, 2.0);
const FVector OpticalAxisRelativeLocation(305.0, 0.0, 31.0);
const FVector OperatorInteractionRelativeLocation(-35.0, -70.0, 105.0);
const FVector OperatorInteractionBoxExtent(115.0, 95.0, 105.0);
const FVector OperatorCharacterRelativeLocation(-120.0, -100.0, 20.0);
constexpr TCHAR StockCameraDroneProbeRelativeLocationDefault[] =
    TEXT("(X=0.000000,Y=-250.000000,Z=250.000000)");
constexpr TCHAR StockCameraDroneDelayedValiditySecondsDefault[] = TEXT("2.000000");

constexpr int32 DefaultInteractivePartId = 0;
constexpr int32 DefaultInteractiveIndex = 0;
constexpr TCHAR DirectExitMarkerKeyName[] = TEXT("E");
constexpr TCHAR BooleanTrueDefault[] = TEXT("true");
constexpr TCHAR BooleanFalseDefault[] = TEXT("false");
constexpr TCHAR AlwaysSpawnCollisionHandling[] = TEXT("AlwaysSpawn");
constexpr TCHAR SnapToTargetAttachmentRule[] = TEXT("SnapToTarget");

constexpr TCHAR ReticleText[] = TEXT("+");
constexpr TCHAR DistancePlaceholderText[] = TEXT("TARGET 0000 m");

constexpr TCHAR OrdinarySharkDataObjectPath[] = TEXT("/Game/Data/NPCData/DA_NPC_Shark.DA_NPC_Shark");
constexpr TCHAR RamSharkDataObjectPath[] = TEXT("/Game/Data/NPCData/DA_NPC_RamShark.DA_NPC_RamShark");
constexpr TCHAR LaserSharkDataObjectPath[] = TEXT("/Game/Data/NPCData/DA_NPC_LaserShark.DA_NPC_LaserShark");
}
