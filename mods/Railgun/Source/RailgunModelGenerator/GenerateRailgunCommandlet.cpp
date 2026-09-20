// Editor-only GLB shell generator. Never shipped; native identities are gated
// by GAME_DERIVED_SOURCES.md. Operator generation lives in AutoloadProbe.

#include "GenerateRailgunCommandlet.h"
#include "RailgunAssetNames.h"
#include "../../RailgunModelContract.h"
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
#include "UObject/PackageFileSummary.h"
#include "UObject/UnrealType.h"
#include "VoyageCustomModuleComponent.h"
#include "VoyageDynamicCollisionComponent.h"
#include "VoyageItem.h"
#include "VoyageModuleActor.h"
#include "VoyageModuleComponent.h"
#include "VoyageModuleSocketViewComponent.h"
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
    UPackage* Package = CreatePackage(RailgunAssetNames::LeafItemPackageName);
    UVoyageItem* Item = NewObject<UVoyageItem>(
        Package,
        FName(RailgunAssetNames::LeafItemAssetName),
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

UGenerateRailgunCommandlet::UGenerateRailgunCommandlet()
{
    // Interchange material parameter discovery needs client-side material data,
    // matching the stock Python import commandlet, even with the NullRHI.
    IsClient = true;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateRailgunCommandlet::Main(const FString& Params)
{
    if (FParse::Param(*Params, RailgunAssetNames::VerifyTaggedParameter))
    {
        FString Json;
        TSharedPtr<FJsonObject> Inventory;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(FPaths::ProjectDir(), RailgunGlb::InventoryFile)) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Inventory)) return 1;
        for (const auto& Value : Inventory->GetArrayField(RailgunGlb::PackagesKey))
        {
            FString Relative = Value->AsString();
            if (!Relative.RemoveFromStart(RailgunAssetNames::GamePackagePrefix)) return 1;
            const FString File = FPaths::Combine(FPaths::ProjectDir(), RailgunAssetNames::CookedContentDirectory, Relative) + FPackageName::GetAssetPackageExtension();
            TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*File));
            if (!Reader) return 1;
            FPackageFileSummary Summary; *Reader << Summary;
            if (Reader->IsError() || (Summary.GetPackageFlags() & PKG_UnversionedProperties))
            {
                UE_LOG(LogTemp, Error, TEXT("Tagged property gate failed: %s"), *File);
                return 1;
            }
        }
        UE_LOG(LogTemp, Display, TEXT("All inventory packages use tagged properties"));
        return 0;
    }
    if (!FParse::Param(*Params, RailgunAssetNames::ShellOnlyParameter) ||
        RailgunAssetNames::IncludeBaseGameLoadedConnectorReference)
    {
        UE_LOG(LogTemp, Error, TEXT("Only explicit -ShellOnly with the mod-authored connector proxy is currently revalidated; historical operator generation is blocked"));
        return 1;
    }
    if (FPackageName::DoesPackageExist(RailgunAssetNames::PawnPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::HudPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::LeafProbePackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::BaseMeshPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::YawMeshPackageName) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::PitchMeshPackageName) ||
        (RailgunAssetNames::IncludeBaseGameLoadedConnectorReference &&
            FPackageName::DoesPackageExist(RailgunAssetNames::LoadedConnectorPackageName)) ||
        (RailgunAssetNames::UsesStockCameraDroneClass &&
            FPackageName::DoesPackageExist(RailgunAssetNames::CameraDronePackageName)) ||
        FPackageName::DoesPackageExist(RailgunAssetNames::LeafItemPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Railgun Railgun generated assets already exist"));
        return 1;
    }

    return RailgunGlb::Generate();}

#endif
