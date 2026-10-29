// Included after AssetText: discovery is engine-owned; rendering remains offline.
#pragma once
#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintPreviewNames.h"

namespace BlueprintPreview
{
bool Catalog(const FString& Input,const FString& Output)
{
    TSharedPtr<FJsonObject> Request;
    if(!AssetText::Read(Input,Request))return false;
    TSet<FString> Selected;
    for(const auto& Value:Request->GetArrayField(BlueprintPreviewNames::Selected))Selected.Add(Value->AsString());
    auto& Registry=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(BlueprintPreviewNames::RegistryModule).Get();
    TArray<FString> Mounted,Roots;
    FPackageName::QueryRootContentPaths(Mounted);
    const FString ProjectDir=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    for(const FString& Root:Mounted)
    {
        FString Directory;
        if(FPackageName::TryConvertLongPackageNameToFilename(Root,Directory) &&
           FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(Directory),ProjectDir))Roots.Add(Root);
    }
    if(!Roots.Contains(BlueprintPreviewNames::GameRoot))Roots.Add(BlueprintPreviewNames::GameRoot);
    for(FString& Root:Roots)Root.RemoveFromEnd(BlueprintPreviewNames::PathSeparator);
    Registry.ScanPathsSynchronous(Roots,true);
    TArray<FAssetData> Assets;
    for(const FString& Root:Roots)Registry.GetAssetsByPath(FName(*Root),Assets,true,true);
    Assets.Sort([](const FAssetData& A,const FAssetData& B){return A.PackageName.LexicalLess(B.PackageName);});
    TSet<FString> Packages;
    TArray<TSharedPtr<FJsonValue>> Rows;
    const FString Folder=FPaths::GetPath(Output);
    IFileManager::Get().MakeDirectory(*Folder,true);
    for(const auto& Data:Assets)
    {
        const FString Package=Data.PackageName.ToString();
        if(Packages.Contains(Package))
        {UE_LOG(LogTemp,Error,TEXT("Preview requires one primary asset per package: %s"),*Package);return false;}
        Packages.Add(Package);
        UObject* Object=Data.GetAsset();
        if(!Object){UE_LOG(LogTemp,Error,TEXT("Preview cannot load project asset: %s"),*Package);return false;}
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(BlueprintPreviewNames::Asset,Package);
        Row->SetStringField(BlueprintPreviewNames::AssetClass,Object->GetClass()->GetPathName());
        const FString Role=Object->GetOutermost()->GetMetaData().GetValue(Object,BlueprintPreviewNames::RoleMetadata);
        if(!Role.IsEmpty() && Role!=BlueprintPreviewNames::Stub)
        {UE_LOG(LogTemp,Error,TEXT("Unknown BlueprintText.Role on %s: %s"),*Package,*Role);return false;}
        Row->SetStringField(BlueprintPreviewNames::Role,Role.IsEmpty()?BlueprintPreviewNames::Unclassified:Role);
        TArray<FName> Dependencies;
        Registry.GetDependencies(Data.PackageName,Dependencies);
        // Explicitly retain inheritance even if registry gathering changes across engine versions.
        if(auto* BP=Cast<UBlueprint>(Object))if(BP->ParentClass)Dependencies.AddUnique(BP->ParentClass->GetOutermost()->GetFName());
        Dependencies.Sort(FNameLexicalLess());
        TArray<TSharedPtr<FJsonValue>> Refs;
        for(FName Dependency:Dependencies)
        {
            if(Dependency==Data.PackageName)continue;
            auto Ref=MakeShared<FJsonObject>();
            const FString Name=Dependency.ToString();
            Ref->SetStringField(BlueprintPreviewNames::Asset,Name);
            Ref->SetBoolField(BlueprintPreviewNames::Missing,!Name.StartsWith(BlueprintPreviewNames::ScriptRoot) && !FPackageName::DoesPackageExist(Name));
            Refs.Add(MakeShared<FJsonValueObject>(Ref));
        }
        Row->SetArrayField(BlueprintPreviewNames::Dependencies,Refs);
        if(auto* BP=Cast<UBlueprint>(Object))
        {
            FString Reason=AssetText::UnsupportedReason(BP);
            if(!BP->GeneratedClass)Reason=TEXT("Blueprint has no generated class.");
            if(!Reason.IsEmpty())Row->SetStringField(BlueprintPreviewNames::Reason,Reason);
            else if(!Selected.Contains(Package))
            {
                const FString Native=FPaths::Combine(Folder,FString::Printf(BlueprintPreviewNames::NativeFilePattern,Rows.Num()));
                const FString Resources=Cast<UWidgetBlueprint>(BP)?FPaths::ChangeExtension(Native,BlueprintPreviewNames::ResourcesExtension):FString();
                if(!AssetText::Export(Package,Native,Resources))return false;
                Row->SetStringField(BlueprintPreviewNames::Native,Native);
            }
        }
        else Row->SetStringField(BlueprintPreviewNames::Reason,TEXT("Non-Blueprint asset: dependency information only; no K2 graph preview."));
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    auto Result=MakeShared<FJsonObject>();
    Result->SetNumberField(BlueprintPreviewNames::Schema,1);
    Result->SetArrayField(BlueprintPreviewNames::Assets,Rows);
    return AssetText::Write(Output,Result);
}

bool MarkStubs(const FString& Input)
{
    TSharedPtr<FJsonObject> Request;
    if(!AssetText::Read(Input,Request))return false;
    TArray<UObject*> Objects;
    for(const auto& Value:Request->GetArrayField(BlueprintPreviewNames::Assets))
    {
        const FString Package=Value->AsString();
        UObject* Object=LoadObject<UObject>(nullptr,*(Package+BlueprintPreviewNames::ObjectSeparator+FPackageName::GetLongPackageAssetName(Package)));
        if(!Object)return false;
        Objects.Add(Object);
    }
    for(UObject* Object:Objects)
    {
        UPackage* Package=Object->GetOutermost();
        if(Package->GetMetaData().GetValue(Object,BlueprintPreviewNames::RoleMetadata)==BlueprintPreviewNames::Stub)continue;
        Package->GetMetaData().SetValue(Object,BlueprintPreviewNames::RoleMetadata,BlueprintPreviewNames::Stub);
        FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
        const FString Filename=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
        if(!UPackage::SavePackage(Package,Object,*Filename,Args))return false;
    }
    return true;
}
}
