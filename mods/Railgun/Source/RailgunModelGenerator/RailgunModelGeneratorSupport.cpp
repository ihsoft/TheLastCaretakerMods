#include "RailgunModelGeneratorPrivate.h"

namespace Railgun::ModelGenerator
{
bool SaveGeneratedAsset(UPackage* Package, UObject* Asset)
{
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
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
        UE_LOG(LogTemp, Error, TEXT("Compilation failed for %s"),
            *Blueprint->GetPathName());
        return false;
    }
    return true;
}

UVoyageItem* CreateLeafItemReferenceStub()
{
    UPackage* Package = CreatePackage(RailgunAssetNames::LeafItemPackageName);
    UVoyageItem* Item = NewObject<UVoyageItem>(Package,
        FName(RailgunAssetNames::LeafItemAssetName), RF_Public | RF_Standalone);
    if (!Item || !SaveGeneratedAsset(Package, Item))
    {
        UE_LOG(LogTemp, Error,
            TEXT("Failed to create the editor-only leaf-item reference stub"));
        return nullptr;
    }
    return Item;
}

USCS_Node* AddRootNode(USimpleConstructionScript* ConstructionScript,
    UClass* ComponentClass, const FName ComponentName)
{
    USCS_Node* Node = ConstructionScript->CreateNode(ComponentClass,
        ComponentName);
    ConstructionScript->AddNode(Node);
    return Node;
}

USCS_Node* AddChildNode(USimpleConstructionScript* ConstructionScript,
    USCS_Node* Parent, UClass* ComponentClass, const FName ComponentName)
{
    USCS_Node* Node = ConstructionScript->CreateNode(ComponentClass,
        ComponentName);
    Parent->AddChildNode(Node);
    return Node;
}
}
