#include "VoyageItemPatchEditorCommandlet.h"
#include "ItemPatchNames.h"
#include "VoyageItemMirrors.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Texture2D.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CustomEvent.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "UObject/MetaData.h"
#include "UObject/UnrealType.h"

UVoyageItemPatchEditorCommandlet::UVoyageItemPatchEditorCommandlet() { IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true; }
static void Save(UObject* Object)
{
    UPackage* Package=Object->GetOutermost();
    FString Filename=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
    checkf(UPackage::SavePackage(Package,Object,*Filename,Args),TEXT("Save failed: %s"),*Filename);
}
static UObject* NewAsset(UClass* Class, const FString& Path)
{
    FString PackageName,Name; check(Path.Split(ItemPatchNames::ObjectSeparator,&PackageName,&Name));
    checkf(!FPackageName::DoesPackageExist(PackageName),TEXT("Refuse existing asset during replacement: %s"),*Path);
    UObject* Object=NewObject<UObject>(CreatePackage(*PackageName),Class,*Name,RF_Public|RF_Standalone|RF_Transactional);
    FAssetRegistryModule::AssetCreated(Object); return Object;
}
static void Import(UObject* Object, const TSharedPtr<FJsonObject>& Props)
{
    for (const auto& Pair:Props->Values)
    {
        FProperty* Prop=FindFProperty<FProperty>(Object->GetClass(),*Pair.Key);
        checkf(Prop,TEXT("Missing property %s"),*Pair.Key);
        FString Text=Pair.Value->AsString();
        const TCHAR* End=Prop->ImportText_Direct(*Text,Prop->ContainerPtrToValuePtr<void>(Object),Object,PPF_None,GWarn);
        checkf(End && !*End,TEXT("Import failed or incomplete: %s = %s"),*Pair.Key,*Text);
    }
}
static TSharedPtr<FJsonObject> Read(UObject* Object,const TSharedPtr<FJsonObject>& Props)
{
    auto Result=MakeShared<FJsonObject>();
    for(const auto& Pair:Props->Values)
    {
        FProperty* Prop=FindFProperty<FProperty>(Object->GetClass(),*Pair.Key); check(Prop);
        FString Text; Prop->ExportText_InContainer(0,Text,Object,nullptr,Object,PPF_None);
        Result->SetStringField(Pair.Key,Text);
    }
    return Result;
}
static FText ImportTextHistory(const TSharedPtr<FJsonObject>& Json)
{
    const FString History=Json->GetStringField(ItemPatchNames::HistoryType);
    FText Text;
    if(History==ItemPatchNames::Base)
    {
        check(Json->GetStringField(ItemPatchNames::Flags)==ItemPatchNames::Immutable);
        Text=FText::AsLocalizable_Advanced_LocText(*Json->GetStringField(ItemPatchNames::Namespace),*Json->GetStringField(ItemPatchNames::Value),*Json->GetStringField(ItemPatchNames::CultureInvariantString));
    }
    else if(History==ItemPatchNames::StringTableEntry)
    {
        check(Json->GetIntegerField(ItemPatchNames::Flags)==0);
        Text=FText::FromStringTable(*Json->GetStringField(ItemPatchNames::TableId),Json->GetStringField(ItemPatchNames::Value));
    }
    else if(History==ItemPatchNames::OrderedFormat)
    {
        check(Json->GetIntegerField(ItemPatchNames::Flags)==0);
        FFormatOrderedArguments Arguments;
        for(const auto& Entry:Json->GetArrayField(ItemPatchNames::Arguments))
        {
            check(Entry->AsObject()->GetStringField(ItemPatchNames::Type)==ItemPatchNames::Text);
            Arguments.Add(ImportTextHistory(Entry->AsObject()->GetObjectField(ItemPatchNames::Value)));
        }
        Text=FText::Format(ImportTextHistory(Json->GetObjectField(ItemPatchNames::SourceFmt)),Arguments);
    }
    else checkf(false,TEXT("Unsupported FText history %s"),*History);
    return Text;
}
static void ImportTexts(UObject* Object,const TSharedPtr<FJsonObject>& Props)
{
    for(const auto& Pair:Props->Values)
    {
        auto* Prop=FindFProperty<FTextProperty>(Object->GetClass(),*Pair.Key); check(Prop);
        Prop->SetPropertyValue_InContainer(Object,ImportTextHistory(Pair.Value->AsObject()));
    }
}
int32 UVoyageItemPatchEditorCommandlet::Main(const FString& Params)
{
    FString SpecPath,ReportPath;
    check(FParse::Value(*Params,ItemPatchNames::SpecArgument,SpecPath));
    check(FParse::Value(*Params,ItemPatchNames::ReportArgument,ReportPath));
    FString Input; check(FFileHelper::LoadFileToString(Input,*SpecPath));
    TSharedPtr<FJsonObject> Spec; check(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Input),Spec));
    FString Package=Spec->GetStringField(ItemPatchNames::Package);
    FString BlueprintPath=Spec->GetStringField(ItemPatchNames::Blueprint);
    FString Name=FPackageName::GetShortName(Package);
    const auto& Properties=Spec->GetArrayField(ItemPatchNames::Properties);
    if(FParse::Param(*Params,ItemPatchNames::Preflight))
    {
        if(UBlueprint* Existing=LoadObject<UBlueprint>(nullptr,*BlueprintPath,nullptr,LOAD_NoWarn))
        {
            if(Existing->ParentClass!=UVoyageCraftingLocationGenerator::StaticClass() ||
               Existing->GetOutermost()->GetMetaData().GetValue(Existing,ItemPatchNames::EditorRoleKey)!=FString(ItemPatchNames::StubRole))
            { UE_LOG(LogTemp,Error,TEXT("Refuse replacing a generator that is not an owned editor stub")); return 2; }
        }
        if(UObject* Existing=LoadObject<UObject>(nullptr,*(Package+ItemPatchNames::ObjectSeparator+Name),nullptr,LOAD_NoWarn))
        {
            if(Existing->GetClass()!=UVoyageItem::StaticClass())
            { UE_LOG(LogTemp,Error,TEXT("Item class conflict")); return 2; }
        }
        for(const auto& Entry:Spec->GetArrayField(ItemPatchNames::Dependencies))
        {
            auto D=Entry->AsObject(); const FString ClassName=D->GetStringField(ItemPatchNames::Class);
            UClass* Class=ClassName==ItemPatchNames::Texture2D ? UTexture2D::StaticClass() : LoadObject<UClass>(nullptr,*(ItemPatchNames::VoyageScriptPrefix+ClassName));
            if(!Class) return 2;
            if(UObject* Existing=LoadObject<UObject>(nullptr,*D->GetStringField(ItemPatchNames::Path),nullptr,LOAD_NoWarn))
                if(Existing->GetClass()!=Class) { UE_LOG(LogTemp,Error,TEXT("Dependency class conflict")); return 2; }
        }
        for(const auto& Table:Spec->GetObjectField(ItemPatchNames::Tables)->Values)
            if(UObject* Existing=LoadObject<UObject>(nullptr,*Table.Key,nullptr,LOAD_NoWarn))
                if(Existing->GetClass()!=UStringTable::StaticClass()) { UE_LOG(LogTemp,Error,TEXT("String table class conflict")); return 2; }
        return 0;
    }
    UObject* Item=nullptr; UObject* Generator=nullptr; UBlueprint* BP=nullptr;
    if(FParse::Param(*Params,ItemPatchNames::Reopen))
    {
        BP=LoadObject<UBlueprint>(nullptr,*BlueprintPath); check(BP);
        Item=LoadObject<UObject>(nullptr,*(Package+ItemPatchNames::ObjectSeparator+Name)); check(Item);
        auto* Prop=FindFProperty<FObjectPropertyBase>(Item->GetClass(),ItemPatchNames::CraftingLocationGenerator); check(Prop);
        Generator=Prop->GetObjectPropertyValue_InContainer(Item); check(Generator);
    }
    else
    {
        for(const auto& Entry:Spec->GetArrayField(ItemPatchNames::Dependencies))
        {
            auto D=Entry->AsObject(); FString ClassName=D->GetStringField(ItemPatchNames::Class);
            UClass* Class=ClassName==ItemPatchNames::Texture2D ? UTexture2D::StaticClass() : LoadObject<UClass>(nullptr,*(ItemPatchNames::VoyageScriptPrefix+ClassName));
            check(Class);
            const FString Path=D->GetStringField(ItemPatchNames::Path);
            if(UObject* Existing=LoadObject<UObject>(nullptr,*Path,nullptr,LOAD_NoWarn))
            {
                checkf(Existing->GetClass()==Class,TEXT("Dependency class mismatch: %s"),*Path);
            }
            else
            {
                UObject* Stub=NewAsset(Class,Path);
                Stub->GetOutermost()->GetMetaData().SetValue(Stub,ItemPatchNames::EditorRoleKey,ItemPatchNames::StubRole);
                Save(Stub);
            }
        }
        for(const auto& Table:Spec->GetObjectField(ItemPatchNames::Tables)->Values)
        {
            if(LoadObject<UStringTable>(nullptr,*FString(Table.Key),nullptr,LOAD_NoWarn)) continue;
            auto* Object=CastChecked<UStringTable>(NewAsset(UStringTable::StaticClass(),FString(Table.Key)));
            Object->GetOutermost()->GetMetaData().SetValue(Object,ItemPatchNames::EditorRoleKey,ItemPatchNames::StubRole);
            for(const auto& Entry:Table.Value->AsObject()->Values) Object->GetMutableStringTable()->SetSourceString(FTextKey(FString(Entry.Key)),Entry.Value->AsString(),ItemPatchNames::Empty);
            Save(Object);
        }
        check(!FPackageName::DoesPackageExist(BlueprintPath));
        BP=FKismetEditorUtilities::CreateBlueprint(UVoyageCraftingLocationGenerator::StaticClass(),CreatePackage(*BlueprintPath),*FPackageName::GetShortName(BlueprintPath),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
        check(BP);
        BP->GetOutermost()->GetMetaData().SetValue(BP,ItemPatchNames::EditorRoleKey,ItemPatchNames::StubRole);
        for(const auto& Pair:Spec->GetObjectField(ItemPatchNames::Variables)->Values)
        {
            if(Pair.Key==ItemPatchNames::UberGraphFrame) continue;
            FString Type=Pair.Value->AsString(); FEdGraphPinType Pin;
            if(Type==ItemPatchNames::BoolProperty) Pin.PinCategory=UEdGraphSchema_K2::PC_Boolean;
            else if(Type==ItemPatchNames::DoubleProperty) { Pin.PinCategory=UEdGraphSchema_K2::PC_Real; Pin.PinSubCategory=UEdGraphSchema_K2::PC_Double; }
            else if(Type==ItemPatchNames::ObjectProperty) { Pin.PinCategory=UEdGraphSchema_K2::PC_Object; Pin.PinSubCategoryObject=UObject::StaticClass(); }
            else if(Type==ItemPatchNames::TextProperty) Pin.PinCategory=UEdGraphSchema_K2::PC_Text;
            else if(Type.StartsWith(ItemPatchNames::StructPropertyPrefix))
            {
                FString StructName=Type.Mid(15,Type.Len()-16);
                Pin.PinCategory=UEdGraphSchema_K2::PC_Struct;
                Pin.PinSubCategoryObject=LoadObject<UScriptStruct>(nullptr,*((StructName==ItemPatchNames::HitResult?ItemPatchNames::EngineScriptPrefix:ItemPatchNames::CoreScriptPrefix)+StructName));
                check(Pin.PinSubCategoryObject.IsValid());
            }
            else { UE_LOG(LogTemp,Error,TEXT("Unsupported variable type %s"),*Type); return 2; }
            check(FBlueprintEditorUtils::AddMemberVariable(BP,*Pair.Key,Pin,Pair.Key==ItemPatchNames::AllowWithWater?ItemPatchNames::TrueValue:ItemPatchNames::Empty));
        }
        UEdGraph* Graph=FBlueprintEditorUtils::FindEventGraph(BP); check(Graph);
        auto* Event=NewObject<UK2Node_CustomEvent>(Graph);
        Event->CustomFunctionName=ItemPatchNames::EditorStubEmptyEvent; Graph->AddNode(Event); Event->CreateNewGuid(); Event->AllocateDefaultPins();
        FKismetEditorUtilities::CompileBlueprint(BP); check(BP->Status!=BS_Error);
        Save(BP);
        Item=NewAsset(UVoyageItem::StaticClass(),Package+ItemPatchNames::ObjectSeparator+Name);
        Generator=NewObject<UObject>(Item,BP->GeneratedClass,*Spec->GetStringField(ItemPatchNames::InnerName),RF_Public|RF_Transactional);
        Import(Generator,Properties[0]->AsObject());
        Import(Item,Properties[1]->AsObject());
        const auto& Texts=Spec->GetArrayField(ItemPatchNames::TextProperties);
        ImportTexts(Generator,Texts[0]->AsObject()); ImportTexts(Item,Texts[1]->AsObject());
        Save(Item);
    }
    check(Generator->GetOuter()==Item);
    auto Report=MakeShared<FJsonObject>();
    Report->SetObjectField(ItemPatchNames::Generator,Read(Generator,Properties[0]->AsObject()));
    Report->SetObjectField(ItemPatchNames::Item,Read(Item,Properties[1]->AsObject()));
    TArray<TSharedPtr<FJsonValue>> Fields;
    for(TFieldIterator<FProperty> It(BP->GeneratedClass);It;++It) Fields.Add(MakeShared<FJsonValueString>(It->GetName()));
    Report->SetArrayField(ItemPatchNames::GeneratorSchema,Fields);
    Report->SetStringField(ItemPatchNames::Status,ItemPatchNames::Passed);
    FString Output; check(FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Output)));
    check(FFileHelper::SaveStringToFile(Output,*ReportPath));
    return 0;
}
