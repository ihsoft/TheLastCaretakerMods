// Read-only preview resources from editor assets; never reads import-source files.
#pragma once
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Styling/SlateBrush.h"
#include "HAL/FileManager.h"

namespace WidgetPreviewResources
{
constexpr TCHAR ResourcesArgument[] = TEXT("PreviewResources=");
constexpr TCHAR ManifestName[] = TEXT("manifest.json");
constexpr TCHAR SchemaField[] = TEXT("schema");
constexpr TCHAR AssetField[] = TEXT("asset");
constexpr TCHAR EngineField[] = TEXT("engine");
constexpr TCHAR TexturesField[] = TEXT("textures");
constexpr TCHAR UnsupportedField[] = TEXT("unsupported");
constexpr TCHAR ResourceField[] = TEXT("resource");
constexpr TCHAR ReasonField[] = TEXT("reason");
constexpr TCHAR FileField[] = TEXT("file");
constexpr TCHAR WidthField[] = TEXT("width");
constexpr TCHAR HeightField[] = TEXT("height");

bool Export(UBlueprint* BP,const FString& Folder)
{
    auto* Widget=Cast<UWidgetBlueprint>(BP);
    if(!Widget || !Widget->WidgetTree)
    {UE_LOG(LogTemp,Error,TEXT("Preview resources require a Widget Blueprint."));return false;}
    TSet<UObject*> Resources;
    TSet<FString> NamedResources;
    // Visit reflected brushes, including brushes nested in styles and containers.
    // Do not follow external object graphs (materials/custom widgets are not rendered).
    TFunction<void(FProperty*,const void*)> Visit;
    Visit=[&](FProperty* Property,const void* Value)
    {
        if(auto* Struct=CastField<FStructProperty>(Property))
        {
            if(Struct->Struct==TBaseStructure<FSlateBrush>::Get())
            {
                const auto* Brush=static_cast<const FSlateBrush*>(Value);
                if(UObject* Resource=Brush->GetResourceObject())Resources.Add(Resource);
                else if(!Brush->GetResourceName().IsNone())NamedResources.Add(Brush->GetResourceName().ToString());
            }
            else for(TFieldIterator<FProperty> It(Struct->Struct);It;++It)
                for(int32 I=0;I<It->ArrayDim;++I)Visit(*It,It->ContainerPtrToValuePtr<void>(Value,I));
        }
        else if(auto* Array=CastField<FArrayProperty>(Property))
        {
            FScriptArrayHelper Helper(Array,Value);
            for(int32 I=0;I<Helper.Num();++I)Visit(Array->Inner,Helper.GetRawPtr(I));
        }
        else if(auto* Map=CastField<FMapProperty>(Property))
        {
            FScriptMapHelper Helper(Map,Value);
            for(int32 I=0;I<Helper.GetMaxIndex();++I)if(Helper.IsValidIndex(I))
            {Visit(Map->KeyProp,Helper.GetKeyPtr(I));Visit(Map->ValueProp,Helper.GetValuePtr(I));}
        }
        else if(auto* Set=CastField<FSetProperty>(Property))
        {
            FScriptSetHelper Helper(Set,Value);
            for(int32 I=0;I<Helper.GetMaxIndex();++I)if(Helper.IsValidIndex(I))Visit(Set->ElementProp,Helper.GetElementPtr(I));
        }
    };
    TArray<UObject*> Objects;
    GetObjectsWithOuter(Widget->WidgetTree,Objects,EGetObjectsFlags::IncludeNestedObjects);
    for(UObject* Object:Objects)if(!Object->HasAnyFlags(RF_Transient))
        for(TFieldIterator<FProperty> It(Object->GetClass());It;++It)if(!It->HasAnyPropertyFlags(CPF_Transient))
            for(int32 I=0;I<It->ArrayDim;++I)Visit(*It,It->ContainerPtrToValuePtr<void>(Object,I));

    auto Manifest=MakeShared<FJsonObject>();auto Textures=MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Unsupported;
    auto Diagnose=[&](const FString& Resource,const FString& Reason)
    {
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(ResourceField,Resource);Row->SetStringField(ReasonField,Reason);
        Unsupported.Add(MakeShared<FJsonValueObject>(Row));
        UE_LOG(LogTemp,Warning,TEXT("HUD preview resource %s: %s"),*Resource,*Reason);
    };
    if(!IFileManager::Get().MakeDirectory(*Folder,true))return false;
    TArray<UObject*> Sorted=Resources.Array();Sorted.Sort([](const UObject& A,const UObject& B){return A.GetPathName()<B.GetPathName();});
    int32 Index=0;
    for(UObject* Resource:Sorted)
    {
        auto* Texture=Cast<UTexture2D>(Resource);
        if(!Texture){Diagnose(Resource->GetPathName(),TEXT("Only Texture2D source images are supported; materials and render targets need rendering."));continue;}
        if(Texture->Source.GetNumBlocks()!=1 || Texture->Source.GetNumLayers()!=1 || Texture->Source.GetNumSlices()!=1)
        {Diagnose(Resource->GetPathName(),TEXT("Multi-block, layered or sliced texture source is not supported."));continue;}
        FImage Image;
        if(!FImageUtils::GetTexture2DSourceImage(Texture,Image))
        {Diagnose(Resource->GetPathName(),TEXT("No readable editor texture source; cooked platform pixels are not exported."));continue;}
        const FString File=FString::Printf(TEXT("texture-%04d.png"),Index++);
        if(!FImageUtils::SaveImageByExtension(*(Folder/File),Image))
        {UE_LOG(LogTemp,Error,TEXT("Cannot write HUD texture: %s"),*(Folder/File));return false;}
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(FileField,File);
        Row->SetNumberField(WidthField,Image.SizeX);Row->SetNumberField(HeightField,Image.SizeY);
        Textures->SetObjectField(Resource->GetPathName(),Row);
    }
    TArray<FString> Names=NamedResources.Array();Names.Sort();
    for(const FString& Name:Names)Diagnose(Name,TEXT("Named brush resource has no loaded asset; file/style resources are not exported."));
    Manifest->SetNumberField(SchemaField,1);Manifest->SetStringField(AssetField,BP->GetOutermost()->GetName());
    Manifest->SetStringField(EngineField,FEngineVersion::Current().ToString());
    Manifest->SetObjectField(TexturesField,Textures);Manifest->SetArrayField(UnsupportedField,Unsupported);
    FString Text;
    return FJsonSerializer::Serialize(Manifest,TJsonWriterFactory<TCHAR,TPrettyJsonPrintPolicy<TCHAR>>::Create(&Text)) &&
        FFileHelper::SaveStringToFile(Text,*(Folder/ManifestName),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}
