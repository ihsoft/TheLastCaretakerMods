// Exact reflected ordinary native call nodes; no project asset is saved here.
#pragma once
#include "K2Node_CallFunction.h"
#include "Misc/EngineVersion.h"

namespace BlueprintCallTemplates
{
constexpr TCHAR Mode[]=TEXT("CallTemplates");
constexpr TCHAR Operations[]=TEXT("operations");
constexpr TCHAR Parent[]=TEXT("contextParent");
constexpr TCHAR Entries[]=TEXT("entries");
constexpr TCHAR Identity[]=TEXT("identity");
constexpr TCHAR Insertion[]=TEXT("nativeInsertion");
constexpr TCHAR Pure[]=TEXT("pure");
constexpr TCHAR Required[]=TEXT("requiredConnections");
constexpr TCHAR EngineVersion[]=TEXT("engineVersion");
constexpr TCHAR Coverage[]=TEXT("coverage");
constexpr TCHAR Schema[]=TEXT("schema");
constexpr TCHAR Separator[]=TEXT("::");
constexpr TCHAR NativePrefix[]=TEXT("/Script/");
constexpr TCHAR ContextName[]=TEXT("BPTCallTemplateContext");
constexpr TCHAR ContextGraph[]=TEXT("CallTemplateGraph");
constexpr TCHAR InternalMetadata[]=TEXT("BlueprintInternalUseOnly");
constexpr TCHAR AllowedInternal[]=TEXT("/Script/Engine.Actor::AddComponentByClass");
constexpr TCHAR LatentMetadata[]=TEXT("Latent");
constexpr TCHAR ArrayMetadata[]=TEXT("ArrayParm");
constexpr TCHAR MapMetadata[]=TEXT("MapParam");
constexpr TCHAR SetMetadata[]=TEXT("SetParam");
constexpr TCHAR StructMetadata[]=TEXT("CustomStructureParam");
constexpr TCHAR EnumExecMetadata[]=TEXT("ExpandEnumAsExecs");
constexpr TCHAR BoolExecMetadata[]=TEXT("ExpandBoolAsExecs");
constexpr TCHAR DynamicTypeMetadata[]=TEXT("DeterminesOutputType");
constexpr TCHAR CoverageText[]=TEXT("Ordinary native CallFunction nodes with concrete pins in the selected parent context; no latent, wildcard, event or special-node synthesis. Internal allowlist: Actor.AddComponentByClass.");

bool Build(const FString& Input,const FString& Output)
{
    TSharedPtr<FJsonObject> Request;
    if(!AssetText::Read(Input,Request) || !AssetText::Keys(*Request,{Operations,Parent}))return false;
    auto* ParentClass=LoadObject<UClass>(nullptr,*Request->GetStringField(Parent));
    if(!ParentClass){UE_LOG(LogTemp,Error,TEXT("CallTemplates parent class is unavailable."));return false;}
    auto* BP=FKismetEditorUtilities::CreateBlueprint(ParentClass,GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(),UBlueprint::StaticClass(),ContextName),BPTYPE_Normal);
    if(!BP)return false;
    auto* Graph=FBlueprintEditorUtils::CreateNewGraph(BP,ContextGraph,UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP,Graph,false,static_cast<UFunction*>(nullptr));
    TArray<TSharedPtr<FJsonValue>> Result;
    TSet<FString> Seen;
    for(const auto& Value:Request->GetArrayField(Operations))
    {
        const FString Operation=Value->AsString();FString ClassPath,FunctionName;
        if(!Operation.Split(Separator,&ClassPath,&FunctionName) || !ClassPath.StartsWith(NativePrefix) || Seen.Contains(Operation))
        {UE_LOG(LogTemp,Error,TEXT("CallTemplates requires distinct /Script/Module.Class::Function identities: %s"),*Operation);return false;}
        Seen.Add(Operation);
        auto* Class=LoadObject<UClass>(nullptr,*ClassPath);
        auto* Function=Class?Class->FindFunctionByName(*FunctionName):nullptr;
        if(!Function || Function->GetOuterUClass()!=Class || !Function->HasAnyFunctionFlags(FUNC_Native) ||
           !Function->HasAnyFunctionFlags(FUNC_BlueprintCallable|FUNC_BlueprintPure) ||
           Function->HasAnyFunctionFlags(FUNC_Event|FUNC_Delegate|FUNC_Private|FUNC_Protected))
        {UE_LOG(LogTemp,Error,TEXT("CallTemplates requires an ordinary callable native function on its declaring class: %s"),*Operation);return false;}
        for(const TCHAR* Metadata:{LatentMetadata,ArrayMetadata,MapMetadata,SetMetadata,StructMetadata,
                                  EnumExecMetadata,BoolExecMetadata,DynamicTypeMetadata})
            if(Function->HasMetaData(Metadata))
            {UE_LOG(LogTemp,Error,TEXT("CallTemplates does not support %s metadata on %s."),Metadata,*Operation);return false;}
        if(Function->HasMetaData(InternalMetadata) && Operation!=AllowedInternal)
        {UE_LOG(LogTemp,Error,TEXT("CallTemplates internal function is outside the tested allowlist: %s"),*Operation);return false;}
        auto* Call=NewObject<UK2Node_CallFunction>(Graph);Graph->AddNode(Call);Call->CreateNewGuid();
        Call->SetFromFunction(Function);Call->AllocateDefaultPins();
        TArray<TSharedPtr<FJsonValue>> Connections;
        for(const auto* Pin:Call->Pins)
        {
            if(Pin->PinType.PinCategory==UEdGraphSchema_K2::PC_Wildcard)
            {UE_LOG(LogTemp,Error,TEXT("CallTemplates cannot specialize wildcard pin %s on %s."),*Pin->PinName.ToString(),*Operation);return false;}
            if(Pin->Direction==EGPD_Input && Pin->PinType.PinCategory!=UEdGraphSchema_K2::PC_Exec && Pin->bDefaultValueIsIgnored)
                Connections.Add(MakeShared<FJsonValueString>(Pin->PinName.ToString()));
        }
        TSet<UObject*> Selection;Selection.Add(Call);FString Native;
        FEdGraphUtilities::ExportNodesToText(Selection,Native);
        auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(Identity,Operation);
        Entry->SetStringField(Insertion,Native);Entry->SetBoolField(Pure,Call->IsNodePure());
        Entry->SetArrayField(Required,Connections);Result.Add(MakeShared<FJsonValueObject>(Entry));
    }
    if(Result.IsEmpty()){UE_LOG(LogTemp,Error,TEXT("CallTemplates requires at least one operation."));return false;}
    auto Root=MakeShared<FJsonObject>();Root->SetNumberField(Schema,1);
    Root->SetStringField(EngineVersion,FEngineVersion::Current().ToString());
    Root->SetStringField(Parent,Request->GetStringField(Parent));Root->SetStringField(Coverage,CoverageText);
    Root->SetArrayField(Entries,Result);return AssetText::Write(Output,Root);
}
}
