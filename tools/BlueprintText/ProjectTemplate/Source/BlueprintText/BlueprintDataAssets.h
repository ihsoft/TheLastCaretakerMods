// Existing DataAsset edits use Unreal's property codec, preserving owned objects.
#pragma once
#include "Engine/DataAsset.h"
#include "Misc/Base64.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/StructuredArchiveAdapters.h"

namespace BlueprintDataAssets
{
constexpr TCHAR ExportMode[]=TEXT("DataAssetExport");
constexpr TCHAR ApplyMode[]=TEXT("DataAssetApply");
constexpr TCHAR Source[]=TEXT("source");
constexpr TCHAR Preserved[]=TEXT("preserved");
constexpr TCHAR PropertyState[]=TEXT("propertyState");
constexpr TCHAR Asset[]=TEXT("asset");
constexpr TCHAR Class[]=TEXT("class");
constexpr TCHAR ObjectName[]=TEXT("objectName");
constexpr TCHAR ObjectsKey[]=TEXT("objects");
constexpr TCHAR Value[]=TEXT("value");
constexpr TCHAR Type[]=TEXT("type");
constexpr TCHAR EditableKey[]=TEXT("editable");
constexpr TCHAR Before[]=TEXT("before");
constexpr TCHAR After[]=TEXT("after");
constexpr TCHAR Schema[]=TEXT("schema");
constexpr TCHAR Kind[]=TEXT("kind");
constexpr TCHAR DataAssetKind[]=TEXT("dataAsset");
constexpr TCHAR Flags[]=TEXT("flags");
constexpr TCHAR Bytes[]=TEXT("bytes");
constexpr TCHAR Properties[]=TEXT("properties");
constexpr TCHAR Self[]=TEXT(".");
constexpr TCHAR GamePackagePrefix[]=TEXT("/Game/");

bool Fail(const FString& Reason)
{
    UE_LOG(LogTemp,Error,TEXT("DataAsset: %s"),*Reason);return false;
}
bool Persistent(FProperty* P) {return !P->HasAnyPropertyFlags(CPF_Transient|CPF_SkipSerialization);}
bool Editable(FProperty* P)
{
    return Persistent(P) && P->HasAnyPropertyFlags(CPF_Edit) &&
        !P->HasAnyPropertyFlags(CPF_EditConst|CPF_Deprecated);
}
bool Equal(const TSharedPtr<FJsonValue>& A,const TSharedPtr<FJsonValue>& B)
{
    return A.IsValid() && B.IsValid() && FJsonValue::CompareEqual(*A,*B);
}
bool Equal(const TSharedPtr<FJsonObject>& A,const TSharedPtr<FJsonObject>& B)
{
    return FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));
}
UDataAsset* Load(const FString& PackageName)
{
    if(!PackageName.StartsWith(GamePackagePrefix) || !FPackageName::IsValidLongPackageName(PackageName))
    {Fail(TEXT("Expected a /Game package identity"));return nullptr;}
    UPackage* Package=LoadPackage(nullptr,*PackageName,LOAD_None);
    if(!Package){Fail(TEXT("Package is unavailable: ")+PackageName);return nullptr;}
    Package->FullyLoad();
    TArray<UObject*> Objects;GetObjectsWithOuter(Package,Objects,false);
    UDataAsset* Found=nullptr;
    for(auto* Object:Objects)
    {
        auto* Candidate=Cast<UDataAsset>(Object);
        if(!Candidate || Found){Fail(TEXT("Expected exactly one DataAsset and its owned subobjects"));return nullptr;}
        Found=Candidate;
    }
    if(!Found)Fail(TEXT("Package has no DataAsset"));
    return Found;
}
FString Text(FProperty* P,const void* Data,UObject* Owner)
{
    FString Result;P->ExportTextItem_Direct(Result,Data,nullptr,Owner,PPF_Delimited);return Result;
}
TSharedPtr<FJsonValue> PropertyValue(FProperty* P,UObject* Owner)
{
    if(P->ArrayDim==1)return MakeShared<FJsonValueString>(Text(P,P->ContainerPtrToValuePtr<void>(Owner),Owner));
    TArray<TSharedPtr<FJsonValue>> Result;
    for(int32 I=0;I<P->ArrayDim;++I)Result.Add(MakeShared<FJsonValueString>(Text(P,P->ContainerPtrToValuePtr<void>(Owner,I),Owner)));
    return MakeShared<FJsonValueArray>(Result);
}

// Equal subvalues retain their hidden state, notably FText histories and flags.
bool Reconcile(FProperty* P,void* New,const void* Old,UObject* Owner)
{
    if(Old && Text(P,New,Owner)==Text(P,Old,Owner)){P->CopySingleValue(New,Old);return true;}
    if(auto* S=CastField<FStructProperty>(P))
    {
        for(TFieldIterator<FProperty> It(S->Struct);It;++It)
            for(int32 I=0;I<It->ArrayDim;++I)
                if(!Reconcile(*It,It->ContainerPtrToValuePtr<void>(New,I),Old?It->ContainerPtrToValuePtr<void>(Old,I):nullptr,Owner))return false;
    }
    else if(auto* A=CastField<FArrayProperty>(P))
    {
        FScriptArrayHelper N(A,New),O(A,Old);
        for(int32 I=0;I<N.Num();++I)
        {
            const void* Previous=Old && I<O.Num()?O.GetRawPtr(I):nullptr;
            // Preserve equal elements across insertions and reorderings as well.
            if(Old)for(int32 J=0;J<O.Num();++J)if(Text(A->Inner,N.GetRawPtr(I),Owner)==Text(A->Inner,O.GetRawPtr(J),Owner))
            {Previous=O.GetRawPtr(J);break;}
            if(!Reconcile(A->Inner,N.GetRawPtr(I),Previous,Owner))return false;
        }
    }
    else if(auto* M=CastField<FMapProperty>(P))
    {
        FScriptMapHelper N(M,New),O(M,Old);
        for(int32 I=0;I<N.GetMaxIndex();++I)if(N.IsValidIndex(I))
        {
            int32 Match=INDEX_NONE;
            if(Old)for(int32 J=0;J<O.GetMaxIndex();++J)if(O.IsValidIndex(J) && M->KeyProp->Identical(N.GetKeyPtr(I),O.GetKeyPtr(J))){Match=J;break;}
            if(!Reconcile(M->KeyProp,N.GetKeyPtr(I),Match==INDEX_NONE?nullptr:O.GetKeyPtr(Match),Owner) ||
               !Reconcile(M->ValueProp,N.GetValuePtr(I),Match==INDEX_NONE?nullptr:O.GetValuePtr(Match),Owner))return false;
        }
        N.Rehash();
    }
    else if(auto* Set=CastField<FSetProperty>(P))
    {
        FScriptSetHelper N(Set,New),O(Set,Old);
        for(int32 I=0;I<N.GetMaxIndex();++I)if(N.IsValidIndex(I))
        {
            const int32 Match=Old?O.FindElementIndex(N.GetElementPtr(I)):INDEX_NONE;
            if(!Reconcile(Set->ElementProp,N.GetElementPtr(I),Match==INDEX_NONE?nullptr:O.GetElementPtr(Match),Owner))return false;
        }
        N.Rehash();
    }
    else if(auto* R=CastField<FObjectPropertyBase>(P))
    {
        UObject* Target=nullptr;
        if(auto* Soft=CastField<FSoftObjectProperty>(P))
        {
            const auto Ref=Soft->GetPropertyValue(New).ToSoftObjectPath();
            if(!Ref.IsNull())
            {
                Target=Ref.TryLoad();
                if(!Ref.IsValid() || !Target)return Fail(TEXT("Missing reference target: ")+Ref.ToString());
            }
        }
        else Target=R->GetObjectPropertyValue(New);
        if(Target && !Target->IsA(R->PropertyClass))return Fail(TEXT("Incompatible object reference: ")+P->GetName());
        UClass* Required=nullptr;
        if(auto* C=CastField<FClassProperty>(P))Required=C->MetaClass;
        if(auto* C=CastField<FSoftClassProperty>(P))Required=C->MetaClass;
        if(Target && Required && (!Cast<UClass>(Target) || !Cast<UClass>(Target)->IsChildOf(Required)))return Fail(TEXT("Incompatible class reference: ")+P->GetName());
        if(P->HasAnyPropertyFlags(CPF_InstancedReference) && (!Old || Target!=R->GetObjectPropertyValue(Old)))
            return Fail(TEXT("Owned object creation/replacement requires the Unreal editor: ")+P->GetName());
    }
    return true;
}

TSharedPtr<FJsonObject> Snapshot(UDataAsset* Root)
{
    auto Result=MakeShared<FJsonObject>(),Document=MakeShared<FJsonObject>(),Rows=MakeShared<FJsonObject>();
    auto Protected=MakeShared<FJsonObject>(),FieldBytes=MakeShared<FJsonObject>();
    Document->SetNumberField(Schema,2);Document->SetStringField(Kind,DataAssetKind);
    Document->SetStringField(Asset,Root->GetOutermost()->GetName());
    Document->SetStringField(ObjectName,Root->GetName());Document->SetStringField(Class,Root->GetClass()->GetPathName());
    TArray<UObject*> Objects;GetObjectsWithOuter(Root,Objects,true);Objects.Add(Root);
    Objects.Sort([](const UObject& A,const UObject& B){return A.GetPathName()<B.GetPathName();});bool Ok=true;
    for(auto* Object:Objects)
    {
        const FString Key=Object==Root?Self:Object->GetPathName(Root);
        auto Row=MakeShared<FJsonObject>(),Props=MakeShared<FJsonObject>(),Serialized=MakeShared<FJsonObject>();
        Row->SetStringField(Class,Object->GetClass()->GetPathName());
        for(TFieldIterator<FProperty> It(Object->GetClass());It;++It)
        {
            auto* P=*It;if(!Persistent(P))continue;
            auto Field=MakeShared<FJsonObject>();FString Extended;
            FString NativeType=P->GetCPPType(&Extended)+Extended;
            if(P->ArrayDim>1)NativeType+=FString::Printf(TEXT("[%d]"),P->ArrayDim);
            Field->SetStringField(Type,NativeType);Field->SetBoolField(EditableKey,Editable(P));
            Field->SetField(Value,PropertyValue(P,Object));Props->SetObjectField(P->GetName(),Field);
            TArray<uint8> Data;FMemoryWriter Writer(Data,true);FObjectAndNameAsStringProxyArchive Archive(Writer,false);Archive.ArNoDelta=true;
            for(int32 I=0;I<P->ArrayDim;++I)P->SerializeItem(FStructuredArchiveFromArchive(Archive).GetSlot(),P->ContainerPtrToValuePtr<void>(Object,I));
            Ok&=!Archive.IsError();Serialized->SetStringField(P->GetName(),FBase64::Encode(Data));
        }
        Row->SetObjectField(Properties,Props);Rows->SetObjectField(Key,Row);FieldBytes->SetObjectField(Key,Serialized);
    }
    // Mask editable fields on all objects together, including values read by
    // custom Serialize implementations. Everything else remains protected.
    struct Saved {FProperty* P;UObject* Owner;void* Data;};TArray<Saved> Values;
    for(auto* Object:Objects)for(TFieldIterator<FProperty> It(Object->GetClass());It;++It)if(Editable(*It))
    {
        auto* P=*It;void* Data=P->AllocateAndInitializeValue();void* Empty=P->AllocateAndInitializeValue();
        P->CopyCompleteValue(Data,P->ContainerPtrToValuePtr<void>(Object));
        P->CopyCompleteValue(P->ContainerPtrToValuePtr<void>(Object),Empty);P->DestroyAndFreeValue(Empty);Values.Add({P,Object,Data});
    }
    for(auto* Object:Objects)
    {
        TArray<uint8> Data;FMemoryWriter Writer(Data,true);FObjectAndNameAsStringProxyArchive Archive(Writer,false);Archive.ArNoDelta=true;
        Object->Serialize(Archive);Ok&=!Archive.IsError();
        auto Row=MakeShared<FJsonObject>();Row->SetStringField(Class,Object->GetClass()->GetPathName());
        Row->SetNumberField(Flags,uint32(Object->GetFlags()&(RF_Public|RF_Standalone|RF_ArchetypeObject|RF_DefaultSubObject)));
        Row->SetStringField(Bytes,FBase64::Encode(Data));Protected->SetObjectField(Object==Root?Self:Object->GetPathName(Root),Row);
    }
    for(const auto& V:Values){V.P->CopyCompleteValue(V.P->ContainerPtrToValuePtr<void>(V.Owner),V.Data);V.P->DestroyAndFreeValue(V.Data);}
    if(!Ok){Fail(TEXT("Cannot capture serialization preservation state"));return nullptr;}
    Document->SetObjectField(ObjectsKey,Rows);Result->SetObjectField(Source,Document);
    Result->SetObjectField(Preserved,Protected);Result->SetObjectField(PropertyState,FieldBytes);return Result;
}
bool Export(const FString& PackageName,const FString& Output)
{
    auto* Object=Load(PackageName);if(!Object)return false;
    const auto State=Snapshot(Object);return State.IsValid() && AssetText::Write(Output,State.ToSharedRef());
}
bool Apply(const FString& Input,const FString& Output)
{
    TSharedPtr<FJsonObject> Request;
    if(Output.IsEmpty() || !AssetText::Read(Input,Request) || !AssetText::Keys(*Request,{Before,After}))return false;
    const auto Original=Request->GetObjectField(Before),Desired=Request->GetObjectField(After);
    const auto OriginalSource=Original->GetObjectField(Source);
    auto* Root=Load(OriginalSource->GetStringField(Asset));if(!Root)return false;
    const auto Current=Snapshot(Root);if(!Current)return false;
    if(!Equal(Current,Original))return Fail(TEXT("Saved DataAsset no longer matches the accepted snapshot"));
    const auto OldObjects=OriginalSource->GetObjectField(ObjectsKey),NewObjects=Desired->GetObjectField(ObjectsKey);
    auto Identity=MakeShared<FJsonObject>(*Desired);Identity->SetObjectField(ObjectsKey,OldObjects);
    if(!Equal(Identity,OriginalSource) || OldObjects->Values.Num()!=NewObjects->Values.Num())return Fail(TEXT("DataAsset identity or owned object inventory changed"));
    TArray<UObject*> Objects;GetObjectsWithOuter(Root,Objects,true);Objects.Add(Root);
    for(auto* Object:Objects)
    {
        const FString Key=Object==Root?Self:Object->GetPathName(Root);
        if(!NewObjects->HasTypedField<EJson::Object>(Key))return Fail(TEXT("Missing owned object: ")+Key);
        const auto Old=OldObjects->GetObjectField(Key),New=NewObjects->GetObjectField(Key);
        auto ObjectIdentity=MakeShared<FJsonObject>(*New);ObjectIdentity->SetObjectField(Properties,Old->GetObjectField(Properties));
        if(!Equal(ObjectIdentity,Old))return Fail(TEXT("Owned object class or identity changed: ")+Key);
        const auto OldProps=Old->GetObjectField(Properties),NewProps=New->GetObjectField(Properties);
        if(OldProps->Values.Num()!=NewProps->Values.Num())return Fail(TEXT("Property inventory changed: ")+Key);
        for(TFieldIterator<FProperty> It(Object->GetClass());It;++It)
        {
            auto* P=*It;if(!Persistent(P))continue;
            if(!NewProps->HasTypedField<EJson::Object>(P->GetName()))return Fail(TEXT("Missing property: ")+P->GetName());
            const auto OldRow=OldProps->GetObjectField(P->GetName()),NewRow=NewProps->GetObjectField(P->GetName());
            auto Metadata=MakeShared<FJsonObject>(*NewRow);Metadata->SetField(Value,OldRow->Values[Value]);
            if(!Equal(Metadata,OldRow))return Fail(TEXT("Property metadata is read-only: ")+P->GetName());
            if(Equal(OldRow->Values[Value],NewRow->Values.FindRef(Value)))continue;
            if(!Editable(P))return Fail(TEXT("Property is read-only: ")+P->GetName());
            TArray<TSharedPtr<FJsonValue>> Items;
            if(P->ArrayDim==1)Items.Add(NewRow->Values.FindRef(Value));
            else if(NewRow->HasTypedField<EJson::Array>(Value))Items=NewRow->GetArrayField(Value);
            if(Items.Num()!=P->ArrayDim)return Fail(TEXT("Fixed array length changed: ")+P->GetName());
            void* Candidate=P->AllocateAndInitializeValue();
            P->CopyCompleteValue(Candidate,P->ContainerPtrToValuePtr<void>(Object));bool Ok=true;
            for(int32 I=0;I<P->ArrayDim && Ok;++I)
            {
                FString InputText;Ok=Items[I].IsValid() && Items[I]->TryGetString(InputText);if(!Ok)break;
                void* Dest=static_cast<uint8*>(Candidate)+I*P->ElementSize;const void* OldData=P->ContainerPtrToValuePtr<void>(Object,I);
                if(InputText==Text(P,OldData,Object))continue;
                const TCHAR* End=P->ImportText_Direct(*InputText,Dest,Object,PPF_Delimited,GWarn);
                Ok=End && !*End && Reconcile(P,Dest,OldData,Object) && Text(P,Dest,Object)==InputText;
            }
            if(Ok)P->CopyCompleteValue(P->ContainerPtrToValuePtr<void>(Object),Candidate);
            P->DestroyAndFreeValue(Candidate);
            if(!Ok)return Fail(TEXT("Invalid, unresolved or non-canonical property value: ")+Key+TEXT(" / ")+P->GetName());
        }
    }
    const auto Changed=Snapshot(Root);if(!Changed)return false;
    if(!Equal(Changed->GetObjectField(Source),Desired) || !Equal(Changed->Values[Preserved],Original->Values[Preserved]))
        return Fail(TEXT("DataAsset edit changed protected state; package not saved"));
    for(const auto& Pair:OldObjects->Values)
    {
        const auto OldProps=Pair.Value->AsObject()->GetObjectField(Properties),NewProps=NewObjects->GetObjectField(Pair.Key)->GetObjectField(Properties);
        const auto OldBytes=Original->GetObjectField(PropertyState)->GetObjectField(Pair.Key),NewBytes=Changed->GetObjectField(PropertyState)->GetObjectField(Pair.Key);
        for(const auto& Field:OldProps->Values)if(Equal(Field.Value,NewProps->Values[Field.Key]) && !Equal(OldBytes->Values[Field.Key],NewBytes->Values[Field.Key]))
            return Fail(FString(TEXT("Untouched property serialization changed: "))+FString(Pair.Key)+TEXT(" / ")+FString(Field.Key));
    }
    if(!AssetText::Write(Output,Changed.ToSharedRef()))return false;
    FString Filename=FPackageName::LongPackageNameToFilename(Root->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    if(!UPackage::SavePackage(Root->GetOutermost(),Root,*Filename,Args))return Fail(TEXT("Cannot save DataAsset"));
    return true;
}
}
