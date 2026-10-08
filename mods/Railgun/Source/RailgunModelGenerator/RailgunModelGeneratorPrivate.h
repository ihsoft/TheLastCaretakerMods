#pragma once

#include "RailgunAssetNames.h"
#include "../../RailgunModelContract.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "InterchangeGenericAnimationPipeline.h"
#include "InterchangeGenericAssetsPipeline.h"
#include "InterchangeGenericMaterialPipeline.h"
#include "InterchangeGenericMeshPipeline.h"
#include "InterchangeGenericTexturePipeline.h"
#include "InterchangeManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "UObject/PackageFileSummary.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "VoyageCustomModuleComponent.h"
#include "VoyageDynamicCollisionComponent.h"
#include "VoyageItem.h"
#include "VoyageModuleActor.h"
#include "VoyageModuleComponent.h"
#include "VoyageModuleSocketViewComponent.h"

namespace Railgun::ModelGenerator
{
bool SaveGeneratedAsset(UPackage* Package, UObject* Asset);
bool CompileGeneratedBlueprint(UBlueprint* Blueprint);
UVoyageItem* CreateLeafItemReferenceStub();
USCS_Node* AddRootNode(USimpleConstructionScript* ConstructionScript,
    UClass* ComponentClass, FName ComponentName);
USCS_Node* AddChildNode(USimpleConstructionScript* ConstructionScript,
    USCS_Node* Parent, UClass* ComponentClass, FName ComponentName);
}
