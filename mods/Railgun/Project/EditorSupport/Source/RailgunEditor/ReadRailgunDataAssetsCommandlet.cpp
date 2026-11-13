#include "ReadRailgunDataAssetsCommandlet.h"
#include "VoyageItem.h"
#include "VoyageSkill.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace
{
using FNode = TSharedPtr<FJsonObject>;
FNode ReadValue(FProperty* Property, const void* Value, FString& Error, const void* DefaultValue = nullptr);

FNode ReadFields(UStruct* Type, const void* Container, const void* Defaults, FString& Error)
{
    auto Fields = MakeShared<FJsonObject>();
    for (TFieldIterator<FProperty> It(Type); It; ++It)
    {
        FProperty* Property = *It;
        if (Property->HasAnyPropertyFlags(CPF_Transient)) continue;
        if (Defaults && Property->Identical_InContainer(Container, Defaults)) continue;
        if (Property->ArrayDim != 1) { Error = TEXT("Static arrays unsupported"); return nullptr; }
        auto Node = ReadValue(Property, Property->ContainerPtrToValuePtr<void>(Container), Error,
            Defaults ? Property->ContainerPtrToValuePtr<void>(Defaults) : nullptr);
        if (!Node) { Error = Property->GetName() + TEXT(": ") + Error; return nullptr; }
        Fields->SetObjectField(Property->GetName(), Node);
    }
    return Fields;
}

FNode ReadValue(FProperty* Property, const void* Value, FString& Error, const void* DefaultValue)
{
    auto Result = MakeShared<FJsonObject>();
    if (auto* Enum = CastField<FEnumProperty>(Property))
    {
        Result->SetStringField(TEXT("enum"), Enum->GetEnum()->GetName());
        Result->SetStringField(TEXT("value"), Enum->GetEnum()->GetNameStringByValue(
            Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value)));
    }
    else if (auto* Soft = CastField<FSoftObjectProperty>(Property))
    {
        const FSoftObjectPath Path = Soft->GetPropertyValue(Value).ToSoftObjectPath();
        auto Top = MakeShared<FJsonObject>();
        Top->SetStringField(TEXT("PackageName"), Path.GetLongPackageName().IsEmpty() ? TEXT("None") : Path.GetLongPackageName());
        Top->SetStringField(TEXT("AssetName"), Path.GetAssetName().IsEmpty() ? TEXT("None") : Path.GetAssetName());
        auto Object = MakeShared<FJsonObject>();
        Object->SetObjectField(TEXT("AssetPath"), Top);
        if (Path.GetSubPathString().IsEmpty()) Object->SetField(TEXT("SubPathString"), MakeShared<FJsonValueNull>());
        else Object->SetStringField(TEXT("SubPathString"), Path.GetSubPathString());
        Result->SetStringField(TEXT("type"), TEXT("SoftObjectPropertyData"));
        Result->SetObjectField(TEXT("value"), Object);
    }
    else if (auto* Object = CastField<FObjectPropertyBase>(Property))
    {
        UObject* Target = Object->GetObjectPropertyValue(Value);
        if (Target) Result->SetStringField(TEXT("object"), Target->GetPathName());
        else Result->SetField(TEXT("object"), MakeShared<FJsonValueNull>());
    }
    else if (auto* TextProperty = CastField<FTextProperty>(Property))
    {
        const FText& Text = TextProperty->GetPropertyValue(Value);
        FString History;
        FTextStringHelper::WriteToBuffer(History, Text);
        FName TableId; FString TableKey;
        if (FTextInspector::GetTableIdAndKey(Text, TableId, TableKey))
        {
            if (!History.StartsWith(TEXT("LOCTABLE("))) { Error = TEXT("String-table history mismatch"); return nullptr; }
            auto State = MakeShared<FJsonObject>();
            State->SetStringField(TEXT("HistoryType"), TEXT("StringTableEntry"));
            State->SetNumberField(TEXT("Flags"), FTextInspector::GetFlags(Text));
            State->SetStringField(TEXT("TableId"), TableId.ToString());
            State->SetStringField(TEXT("Value"), TableKey);
            Result->SetObjectField(TEXT("text"), State);
            Result->SetStringField(TEXT("nativeHistory"), History);
            return Result;
        }
        // This migration supports Base histories only. Preserve the history-aware
        // native representation as evidence, rather than inferring it from display.
        if (!History.StartsWith(TEXT("NSLOCTEXT("))) { Error = TEXT("Unsupported text history: ") + History; return nullptr; }
        const auto Namespace = FTextInspector::GetNamespace(Text);
        const auto Key = FTextInspector::GetKey(Text);
        const FString* Source = FTextInspector::GetSourceString(Text);
        if (!Namespace.IsSet() || !Key.IsSet() || !Source) { Error = TEXT("Missing Base text identity"); return nullptr; }
        auto State = MakeShared<FJsonObject>();
        State->SetStringField(TEXT("HistoryType"), TEXT("Base"));
        State->SetNumberField(TEXT("Flags"), FTextInspector::GetFlags(Text));
        State->SetStringField(TEXT("Namespace"), Namespace.GetValue());
        State->SetStringField(TEXT("Value"), Key.GetValue());
        State->SetStringField(TEXT("CultureInvariantString"), *Source);
        Result->SetObjectField(TEXT("text"), State);
        Result->SetStringField(TEXT("nativeHistory"), History);
    }
    else if (auto* Array = CastField<FArrayProperty>(Property))
    {
        FScriptArrayHelper Helper(Array, Value);
        TArray<TSharedPtr<FJsonValue>> Values;
        for (int32 I = 0; I < Helper.Num(); ++I)
        {
            auto Node = ReadValue(Array->Inner, Helper.GetRawPtr(I), Error);
            if (!Node) return nullptr;
            Values.Add(MakeShared<FJsonValueObject>(Node));
        }
        Result->SetArrayField(TEXT("array"), Values);
    }
    else if (auto* Map = CastField<FMapProperty>(Property))
    {
        FScriptMapHelper Helper(Map, Value);
        TArray<TSharedPtr<FJsonValue>> Entries;
        for (int32 I = 0; I < Helper.GetMaxIndex(); ++I)
        {
            if (!Helper.IsValidIndex(I)) continue;
            auto Key = ReadValue(Map->KeyProp, Helper.GetKeyPtr(I), Error);
            auto Val = ReadValue(Map->ValueProp, Helper.GetValuePtr(I), Error);
            if (!Key || !Val) return nullptr;
            TArray<TSharedPtr<FJsonValue>> Pair{MakeShared<FJsonValueObject>(Key), MakeShared<FJsonValueObject>(Val)};
            Entries.Add(MakeShared<FJsonValueArray>(Pair));
        }
        Result->SetArrayField(TEXT("map"), Entries);
    }
    else if (auto* Struct = CastField<FStructProperty>(Property))
    {
        auto Fields = ReadFields(Struct->Struct, Value, DefaultValue, Error);
        if (!Fields) return nullptr;
        Result->SetStringField(TEXT("struct"), Struct->Struct->GetName());
        Result->SetObjectField(TEXT("fields"), Fields);
    }
    else if (auto* Boolean = CastField<FBoolProperty>(Property))
    {
        Result->SetStringField(TEXT("type"), TEXT("BoolPropertyData"));
        Result->SetBoolField(TEXT("value"), Boolean->GetPropertyValue(Value));
    }
    else if (auto* Name = CastField<FNameProperty>(Property))
    {
        Result->SetStringField(TEXT("type"), TEXT("NamePropertyData"));
        Result->SetStringField(TEXT("value"), Name->GetPropertyValue(Value).ToString());
    }
    else if (auto* Number = CastField<FNumericProperty>(Property))
    {
        Result->SetStringField(TEXT("type"), Property->GetClass()->GetName() + TEXT("Data"));
        Result->SetNumberField(TEXT("value"), Number->IsFloatingPoint()
            ? Number->GetFloatingPointPropertyValue(Value) : Number->GetSignedIntPropertyValue(Value));
    }
    else { Error = TEXT("Unsupported reflected property type: ") + Property->GetClass()->GetName(); return nullptr; }
    return Result;
}
}

UReadRailgunDataAssetsCommandlet::UReadRailgunDataAssetsCommandlet()
{
    IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true;
}

int32 UReadRailgunDataAssetsCommandlet::Main(const FString& Params)
{
    FString Output, Error, Text;
    if (!FParse::Value(*Params, TEXT("Output="), Output)) return 1;
    FString Kind = TEXT("ammo"), Path;
    FParse::Value(*Params, TEXT("Asset="), Kind);
    if (Kind == TEXT("ammo")) Path = TEXT("/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod.DA_Ammo_Railgun_FullRod");
    else if (Kind == TEXT("gun")) Path = TEXT("/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01.DA_Item_Module_RailgunCannonMk01");
    else if (Kind == TEXT("skill")) Path = TEXT("/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun.DA_Skill_Railgun");
    else return 1;
    UObject* Ammo = LoadObject<UObject>(nullptr, *Path);
    if (!Ammo) return 1;
    auto Fields = ReadFields(Ammo->GetClass(), Ammo, Ammo->GetClass()->GetDefaultObject(), Error);
    if (!Fields) { UE_LOG(LogTemp, Error, TEXT("DataAsset readback: %s"), *Error); return 1; }
    auto Report = MakeShared<FJsonObject>();
    Report->SetObjectField(TEXT("fields"), Fields);
    Report->SetStringField(TEXT("class"), Ammo->GetClass()->GetPathName());
    if (FParse::Param(*Params, TEXT("Graph")))
    {
        auto Graph = MakeShared<FJsonObject>();
        TArray<UObject*> Objects; GetObjectsWithOuter(Ammo, Objects, EGetObjectsFlags::IncludeNestedObjects); Objects.Add(Ammo);
        for (UObject* Object : Objects)
        {
            if (Object->HasAnyFlags(RF_Transient)) continue;
            auto Data = ReadFields(Object->GetClass(), Object, Object->GetClass()->GetDefaultObject(), Error);
            if (!Data) { UE_LOG(LogTemp, Error, TEXT("Owned readback: %s"), *Error); return 1; }
            auto Entry = MakeShared<FJsonObject>();
            Entry->SetObjectField(TEXT("fields"), Data);
            Entry->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
            if (Object == Ammo) Entry->SetField(TEXT("outer"), MakeShared<FJsonValueNull>());
            else Entry->SetStringField(TEXT("outer"), Object->GetOuter()->GetPathName());
            if (Graph->HasField(Object->GetName())) return 1;
            Graph->SetObjectField(Object->GetName(), Entry);
        }
        Report = MakeShared<FJsonObject>(); Report->SetObjectField(TEXT("objects"), Graph);
    }
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text)) ||
        !FFileHelper::SaveStringToFile(Text, *Output)) return 1;
    return 0;
}
