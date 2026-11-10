// Included inside AssetText after the reflected property and object helpers.
#pragma once
constexpr TCHAR NativeDefaults[] = TEXT("nativeDefaults");
constexpr TCHAR CDOOwner[] = TEXT("$cdo");
constexpr TCHAR ValidateLegacyDefaultsMode[] = TEXT("ValidateLegacyDefaults");

bool NativeDefaultObjects(UObject* Root,TArray<UObject*>& NativeObjects)
{
    TArray<UObject*> Children;GetObjectsWithOuter(Root,Children,EGetObjectsFlags::None);
    for(UObject* Child:Children)
    {
        if(Child->HasAnyFlags(RF_Transient))continue;
        if(!Child->HasAnyFlags(RF_DefaultSubObject) || !Child->GetClass()->HasAnyClassFlags(CLASS_Native) ||
           !Child->GetArchetype() || Child->GetArchetype()->GetClass()!=Child->GetClass())
        {
            UE_LOG(LogTemp,Error,TEXT("Unsupported CDO-owned object %s: only native constructor default subobjects are covered."),*Child->GetPathName());
            return false;
        }
        NativeObjects.Add(Child);
        if(!NativeDefaultObjects(Child,NativeObjects))return false;
    }
    return true;
}
bool ExportNativeDefaults(UBlueprint* BP,Json& Data)
{
    UObject* CDO=BP->GeneratedClass->GetDefaultObject();
    TArray<UObject*> NativeObjects;if(!NativeDefaultObjects(CDO,NativeObjects))return false;
    NativeObjects.Sort([&](const UObject& A,const UObject& B){return ObjectKey(&A,BP)<ObjectKey(&B,BP);});
    Values Rows;
    for(UObject* Object:NativeObjects)
    {
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(ObjectId,ObjectKey(Object,BP));
        Row->SetStringField(ATNames::Valuename,Object->GetName());
        Row->SetStringField(ObjectClass,Object->GetClass()->GetPathName());
        Row->SetStringField(ObjectOuter,Object->GetOuter()==CDO?CDOOwner:ObjectKey(Object->GetOuter(),BP));
        Row->SetNumberField(ObjectFlags,uint32(Object->GetFlags()&StoredFlags));
        // The archetype is the inherited subobject, including native-constructor
        // initialization and parent Blueprint overrides, not the component class CDO.
        Row->SetObjectField(ATNames::Valueproperties,Properties(Object,Object->GetArchetype(),{},false,true));
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Data=MakeShared<FJsonObject>();Data->SetArrayField(TEXT("objects"),Rows);
    return true;
}
bool RestoreNativeDefaults(UBlueprint* BP,const FJsonObject& Root)
{
    UObject* CDO=BP->GeneratedClass->GetDefaultObject();
    TArray<UObject*> NativeObjects;if(!NativeDefaultObjects(CDO,NativeObjects))return false;
    if(!Root.HasField(NativeDefaults))
    {
        if(!NativeObjects.Num())return true;
        UE_LOG(LogTemp,Error,TEXT("Missing nativeDefaults for %s; export the intact asset with current BPT before editing."),*BP->GetPathName());return false;
    }
    const auto Data=Root.GetObjectField(NativeDefaults);
    if(!Keys(*Data,{TEXT("objects")}))return false;
    const auto& Rows=Data->GetArrayField(TEXT("objects"));
    if(Rows.Num()!=NativeObjects.Num())
    {UE_LOG(LogTemp,Error,TEXT("nativeDefaults must cover every constructor default subobject of %s."),*BP->GetPathName());return false;}
    TMap<FString,UObject*> ById;for(UObject* Object:NativeObjects)ById.Add(ObjectKey(Object,BP),Object);
    TSet<FString> Seen;
    for(const auto& Value:Rows)
    {
        auto Row=Value->AsObject();
        if(!Row || !Keys(*Row,{ObjectId,ATNames::Valuename,ObjectClass,ObjectOuter,ObjectFlags,ATNames::Valueproperties}))return false;
        const FString Id=Row->GetStringField(ObjectId);
        UObject* Object=ById.FindRef(Id);
        if(!Object || Seen.Contains(Id) || Object->GetName()!=Row->GetStringField(ATNames::Valuename) ||
           Object->GetClass()->GetPathName()!=Row->GetStringField(ObjectClass) ||
           (Object->GetOuter()==CDO?FString(CDOOwner):ObjectKey(Object->GetOuter(),BP))!=Row->GetStringField(ObjectOuter) ||
           uint32(Object->GetFlags()&StoredFlags)!=uint32(Row->GetIntegerField(ObjectFlags)))
        {UE_LOG(LogTemp,Error,TEXT("nativeDefaults identity must match the constructor-created object: %s"),*Id);return false;}
        Seen.Add(Id);
    }
    // Every final CDO/subobject identity now exists; restore references and values
    // without allocating duplicate native components or copying parent-owned pointers.
    for(const auto& Value:Rows)
    {
        auto Row=Value->AsObject();
        if(!Apply(ById.FindChecked(Row->GetStringField(ObjectId)),*Row->GetObjectField(ATNames::Valueproperties),{}))return false;
    }
    return true;
}
bool ValidateLegacyDefaults(const FString& Path)
{
    TSharedPtr<FJsonObject> Request;if(!Read(Path,Request))return false;
    for(const auto& Value:Request->GetArrayField(TEXT("assets")))
    {
        const auto Source=Value->AsObject();
        const FString Asset=Source->GetStringField(ATNames::Valueasset);
        UObject* CDO=nullptr;
        if(FPackageName::DoesPackageExist(Asset))
        {
            FString ObjectName=FPackageName::GetLongPackageAssetName(Asset);
            Source->TryGetStringField(ATNames::ValueobjectName,ObjectName);
            auto* BP=LoadObject<UBlueprint>(nullptr,*(Asset+TEXT(".")+ObjectName));
            if(!BP || !BP->GeneratedClass)return false;
            CDO=BP->GeneratedClass->GetDefaultObject();
        }
        else
        {
            UClass* Parent=LoadObject<UClass>(nullptr,*Source->GetStringField(ATNames::Valueparent));
            if(!Parent)return false;
            CDO=Parent->GetDefaultObject();
        }
        TArray<UObject*> NativeObjects;if(!NativeDefaultObjects(CDO,NativeObjects))return false;
        if(NativeObjects.Num())
        {UE_LOG(LogTemp,Error,TEXT("Legacy source omits nativeDefaults for %s. Export the intact asset into a fresh workspace; selected packages were not changed."),*Asset);return false;}
    }
    return true;
}
