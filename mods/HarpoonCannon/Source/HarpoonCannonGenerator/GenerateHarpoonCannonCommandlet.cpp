// Editor-only GLB shell generator. Never shipped; native identities are gated
// by GAME_DERIVED_SOURCES.md. Operator generation lives in AutoloadProbe.

#include "GenerateHarpoonCannonCommandlet.h"
#include "CannonAssetNames.h"
#include "../../HarpoonModelContract.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "VoyageCustomModuleComponent.h"
#include "VoyageDynamicCollisionComponent.h"
#include "VoyageItem.h"
#include "VoyageModuleActor.h"
#include "VoyageModuleComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "InterchangeManager.h"
#include "Materials/MaterialInstanceConstant.h"

#if WITH_EDITOR
namespace
{
bool SaveGeneratedAsset(UPackage* Package, UObject* Asset)
{
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(),
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
}

bool CompileGeneratedBlueprint(UBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Compilation failed for %s"), *Blueprint->GetPathName());
        return false;
    }

    return true;
}

UVoyageItem* CreateLeafItemReferenceStub()
{
    UPackage* Package = CreatePackage(CannonAssetNames::LeafItemPackageName);
    UVoyageItem* Item = NewObject<UVoyageItem>(
        Package,
        FName(CannonAssetNames::LeafItemAssetName),
        RF_Public | RF_Standalone);
    if (!Item || !SaveGeneratedAsset(Package, Item))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the editor-only leaf-item reference stub"));
        return nullptr;
    }
    return Item;
}

USCS_Node* AddRootNode(
    USimpleConstructionScript* ConstructionScript,
    UClass* ComponentClass,
    const FName ComponentName)
{
    USCS_Node* Node = ConstructionScript->CreateNode(ComponentClass, ComponentName);
    ConstructionScript->AddNode(Node);
    return Node;
}

USCS_Node* AddChildNode(
    USimpleConstructionScript* ConstructionScript,
    USCS_Node* Parent,
    UClass* ComponentClass,
    const FName ComponentName)
{
    USCS_Node* Node = ConstructionScript->CreateNode(ComponentClass, ComponentName);
    Parent->AddChildNode(Node);
    return Node;
}
}

#include "GlbShell.h"

UGenerateHarpoonCannonCommandlet::UGenerateHarpoonCannonCommandlet()
{
    // Interchange material parameter discovery needs client-side material data,
    // matching the stock Python import commandlet, even with the NullRHI.
    IsClient = true;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateHarpoonCannonCommandlet::Main(const FString& Params)
{
    if (!FParse::Param(*Params, CannonAssetNames::ShellOnlyParameter) ||
        CannonAssetNames::IncludeBaseGameLoadedConnectorReference)
    {
        UE_LOG(LogTemp, Error, TEXT("Only explicit -ShellOnly with the mod-authored connector proxy is currently revalidated; historical operator generation is blocked"));
        return 1;
    }
    if (FPackageName::DoesPackageExist(CannonAssetNames::PawnPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::HudPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::LeafProbePackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::BaseMeshPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::YawMeshPackageName) ||
        FPackageName::DoesPackageExist(CannonAssetNames::PitchMeshPackageName) ||
        (CannonAssetNames::IncludeBaseGameLoadedConnectorReference &&
            FPackageName::DoesPackageExist(CannonAssetNames::LoadedConnectorPackageName)) ||
        (CannonAssetNames::UsesStockCameraDroneClass &&
            FPackageName::DoesPackageExist(CannonAssetNames::CameraDronePackageName)) ||
        FPackageName::DoesPackageExist(CannonAssetNames::LeafItemPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Harpoon Cannon generated assets already exist"));
        return 1;
    }

    return HarpoonGlb::Generate();}

#endif
