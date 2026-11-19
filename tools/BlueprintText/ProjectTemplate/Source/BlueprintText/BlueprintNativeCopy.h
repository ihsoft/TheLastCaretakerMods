#pragma once
#include "AssetToolsModule.h"
#include "IAssetTools.h"

namespace BlueprintNativeCopy
{
bool Copy(const FString& Source,const FString& Destination)
{
    if(!Source.StartsWith(TEXT("/Game/")) || !Destination.StartsWith(TEXT("/Game/")) ||
       FPackageName::DoesPackageExist(Destination))return false;
    auto* Original=LoadObject<UBlueprint>(nullptr,*Source);
    if(!Original || (Original->ParentClass!=UObject::StaticClass() &&
       Original->ParentClass!=UBlueprintFunctionLibrary::StaticClass()) || !AssetText::Supported(Original))return false;
    const FString Name=FPackageName::GetLongPackageAssetName(Destination);
    auto& Assets=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    auto* Copy=Cast<UBlueprint>(Assets.DuplicateAsset(Name,FPackageName::GetLongPackagePath(Destination),Original));
    if(!Copy)return false;
    FKismetEditorUtilities::CompileBlueprint(Copy);
    if(Copy->Status==BS_Error || !Copy->GeneratedClass)return false;
    TArray<UEdGraph*> Graphs;Copy->GetAllGraphs(Graphs);
    for(UEdGraph* Graph:Graphs)for(UEdGraphNode* Node:Graph->Nodes)
        if(Node->IsA<UBlueprintTextCallFunction>())return false;
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    const FString File=FPackageName::LongPackageNameToFilename(Destination,FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Copy->GetOutermost(),Copy,*File,Args);
}
}
