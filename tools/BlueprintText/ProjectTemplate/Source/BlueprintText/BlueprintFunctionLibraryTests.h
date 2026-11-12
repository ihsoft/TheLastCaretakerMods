// Isolated function-library acceptance, with external class references and two Actor callers.
#pragma once
#include "Kismet/GameplayStatics.h"
namespace BlueprintFunctionLibraryTests
{
using namespace BlueprintObjectTests;
constexpr TCHAR LibraryPath[]=TEXT("/Game/BPTTypeLibrary");
FEdGraphPinType ActorType()
{
    FEdGraphPinType T;T.PinCategory=UEdGraphSchema_K2::PC_Object;T.PinSubCategoryObject=AActor::StaticClass();return T;
}
UK2Node_CallFunction* Call(UEdGraph* G,UClass* Class,FName Name)
{
    auto* N=Node<UK2Node_CallFunction>(G);N->SetFromFunction(Class->FindFunctionByName(Name));N->AllocateDefaultPins();return N;
}
bool Predicate(UBlueprint* BP,FName Name,UClass* Medium,UClass* Large,bool ExcludeLarge)
{
    auto* G=Function(BP,Name);auto* E=Entry(G);E->AddExtraFlags(FUNC_Static|FUNC_BlueprintPure);
    E->CreateUserDefinedPin(TEXT("Actor"),ActorType(),EGPD_Output);
    auto* R=Node<UK2Node_FunctionResult>(G);R->AllocateDefaultPins();
    FEdGraphPinType Bool;Bool.PinCategory=UEdGraphSchema_K2::PC_Boolean;
    R->CreateUserDefinedPin(TEXT("ReturnValue"),Bool,EGPD_Input);
    auto* Class=Call(G,UGameplayStatics::StaticClass(),TEXT("GetObjectClass"));
    auto* IsFamily=Call(G,UKismetMathLibrary::StaticClass(),TEXT("ClassIsChildOf"));
    IsFamily->FindPin(TEXT("ParentClass"))->DefaultObject=ExcludeLarge?Medium:Large;
    if(!Link(E->FindPin(TEXT("Actor")),Class->FindPin(TEXT("Object"))) ||
       !Link(Class->FindPin(TEXT("ReturnValue")),IsFamily->FindPin(TEXT("TestClass"))))return false;
    auto* Value=IsFamily->FindPin(TEXT("ReturnValue"));
    if(ExcludeLarge)
    {
        auto* IsLarge=Call(G,UKismetMathLibrary::StaticClass(),TEXT("ClassIsChildOf"));
        IsLarge->FindPin(TEXT("ParentClass"))->DefaultObject=Large;
        auto* Not=Call(G,UKismetMathLibrary::StaticClass(),TEXT("Not_PreBool"));
        auto* And=Call(G,UKismetMathLibrary::StaticClass(),TEXT("BooleanAND"));
        if(!Link(Class->FindPin(TEXT("ReturnValue")),IsLarge->FindPin(TEXT("TestClass"))) ||
           !Link(IsLarge->FindPin(TEXT("ReturnValue")),Not->FindPin(TEXT("A"))) ||
           !Link(Value,And->FindPin(TEXT("A"))) || !Link(Not->FindPin(TEXT("ReturnValue")),And->FindPin(TEXT("B"))))return false;
        Value=And->FindPin(TEXT("ReturnValue"));
    }
    return Link(Value,R->FindPin(TEXT("ReturnValue"))) && Link(E->FindPin(UEdGraphSchema_K2::PN_Then),R->FindPin(UEdGraphSchema_K2::PN_Execute));
}
bool Fixture()
{
    auto* Medium=Create(TEXT("/Game/BPTMedium"),AActor::StaticClass());if(!Medium || !Compile(Medium) || !Save(Medium))return false;
    auto* Large=Create(TEXT("/Game/BPTLarge"),Medium->GeneratedClass);if(!Large || !Compile(Large) || !Save(Large))return false;
    for(const auto& Pair:{TPair<const TCHAR*,UClass*>(TEXT("/Game/BPTMediumChild"),Medium->GeneratedClass),
                         TPair<const TCHAR*,UClass*>(TEXT("/Game/BPTLargeChild"),Large->GeneratedClass)})
    {auto* Child=Create(Pair.Key,Pair.Value);if(!Child || !Compile(Child) || !Save(Child))return false;}
    auto* Library=FKismetEditorUtilities::CreateBlueprint(UBlueprintFunctionLibrary::StaticClass(),CreatePackage(LibraryPath),
        TEXT("BPTTypeLibrary"),BPTYPE_FunctionLibrary);
    if(!Library || !Predicate(Library,TEXT("IsLarge"),Medium->GeneratedClass,Large->GeneratedClass,false) ||
       !Predicate(Library,TEXT("IsMedium"),Medium->GeneratedClass,Large->GeneratedClass,true) ||
       !BlueprintArgumentTests::Echo(Library,TEXT("Optional"),UEdGraphSchema_K2::PC_Int,TEXT("7")))return false;
    Entry(Library->FunctionGraphs.Last())->AddExtraFlags(FUNC_Static);
    // Match BlueprintDetailsCustomization::OnParamsChanged: after adding user
    // parameters, reconstruct entry/exit before compiling the final signature.
    // In particular __WorldContext belongs after the authored inputs.
    for(UEdGraph* G:Library->FunctionGraphs)for(UEdGraphNode* N:G->Nodes)
        if(auto* Editable=Cast<UK2Node_EditablePinBase>(N))Editable->ReconstructNode();
    if(!Compile(Library) || !Save(Library))return false;
    for(const TCHAR* Path:{TEXT("/Game/BPTLibraryCallerA"),TEXT("/Game/BPTLibraryCallerB")})
    {
        auto* BP=Create(Path,AActor::StaticClass());if(!BP)return false;
        auto* G=Function(BP,TEXT("Probe"));auto* E=Entry(G);E->AddExtraFlags(FUNC_BlueprintPure);
        E->CreateUserDefinedPin(TEXT("Actor"),ActorType(),EGPD_Output);
        auto* R=Node<UK2Node_FunctionResult>(G);R->AllocateDefaultPins();
        if(!Link(E->FindPin(UEdGraphSchema_K2::PN_Then),R->FindPin(UEdGraphSchema_K2::PN_Execute)))return false;
        for(const FName Name:{FName(TEXT("IsLarge")),FName(TEXT("IsMedium")),FName(TEXT("Optional"))})
        {
            auto* N=Node<UBlueprintTextCallFunction>(G);N->Rename(*Name.ToString(),G);
            N->SetFromFunction(Library->GeneratedClass->FindFunctionByName(Name));N->AllocateDefaultPins();
            if(Name!=TEXT("Optional") && !Link(E->FindPin(TEXT("Actor")),N->FindPin(TEXT("Actor"))))return false;
            FString Error;if(!N->RefreshArguments(Error))return false;
            R->CreateUserDefinedPin(Name,N->FindPin(TEXT("ReturnValue"))->PinType,EGPD_Input);
            if(!Link(N->FindPin(TEXT("ReturnValue")),R->FindPin(Name)))return false;
        }
        if(!Compile(BP) || !Save(BP))return false;
    }
    return true;
}
bool Check(bool Changed)
{
    FEditorScriptExecutionGuard ScriptGuard;
    auto* Library=LoadObject<UBlueprint>(nullptr,LibraryPath);
    if(!Library || !AssetText::IsFunctionLibrary(Library) || !AssetText::Supported(Library))
    {UE_LOG(LogTemp,Error,TEXT("Library fixture identity/state failed."));return false;}
    const FName LargeName=Changed?TEXT("IsLargeFamily"):TEXT("IsLarge");
    auto* LargeFunction=Library->GeneratedClass->FindFunctionByName(LargeName);
    if(!LargeFunction || !LargeFunction->HasAllFunctionFlags(FUNC_Static|FUNC_BlueprintPure) ||
       !FindFProperty<FObjectPropertyBase>(LargeFunction,Changed?TEXT("Candidate"):TEXT("Actor")))
    {UE_LOG(LogTemp,Error,TEXT("Library fixture signature/flags failed: %s flags=%x"),*GetPathNameSafe(LargeFunction),LargeFunction?uint32(LargeFunction->FunctionFlags):0);return false;}
    for(const TCHAR* Path:{TEXT("/Game/BPTLibraryCallerA"),TEXT("/Game/BPTLibraryCallerB")})
    {
        auto* BP=LoadObject<UBlueprint>(nullptr,Path);if(!BP || !AssetText::CheckCallArguments(BP))return false;
        for(UEdGraph* G:BP->FunctionGraphs)for(UEdGraphNode* Node:G->Nodes)if(auto* N=Cast<UBlueprintTextCallFunction>(Node))
        {
            auto* F=N->GetTargetFunction();auto* Self=N->FindPin(UEdGraphSchema_K2::PN_Self);
            // Editor nodes may resolve the same declaration through its skeleton class.
            if(!F || F->GetOwnerClass()->ClassGeneratedBy!=Library || !F->HasAllFunctionFlags(FUNC_Static|FUNC_BlueprintPure) ||
               !N->IsNodePure() || (Self && Self->LinkedTo.Num()))
            {UE_LOG(LogTemp,Error,TEXT("Library caller identity/flags failed: %s target=%s expected=%s flags=%x pure=%d"),*N->GetPathName(),*GetPathNameSafe(F),*Library->GeneratedClass->GetPathName(),F?uint32(F->FunctionFlags):0,N->IsNodePure());return false;}
        }
        auto* Probe=BP->GeneratedClass->FindFunctionByName(TEXT("Probe"));if(!Probe)return false;
        auto* Input=FindFProperty<FObjectPropertyBase>(Probe,TEXT("Actor"));
        auto* Large=FindFProperty<FBoolProperty>(Probe,TEXT("IsLarge"));
        auto* Medium=FindFProperty<FBoolProperty>(Probe,TEXT("IsMedium"));
        auto* Optional=FindFProperty<FIntProperty>(Probe,TEXT("Optional"));if(!Input || !Large || !Medium || !Optional)
        {UE_LOG(LogTemp,Error,TEXT("Library probe reflected parameters missing: %s input=%d large=%d medium=%d optional=%d"),Path,Input!=nullptr,Large!=nullptr,Medium!=nullptr,Optional!=nullptr);return false;}
        struct FCase {const TCHAR* Path;bool Large;bool Medium;};
        for(const FCase& C:{FCase{nullptr,false,false},FCase{TEXT("/Script/Engine.Actor"),false,false},
            FCase{TEXT("/Game/BPTMedium.BPTMedium_C"),false,true},FCase{TEXT("/Game/BPTLarge.BPTLarge_C"),true,false},
            FCase{TEXT("/Game/BPTMediumChild.BPTMediumChild_C"),false,true},FCase{TEXT("/Game/BPTLargeChild.BPTLargeChild_C"),true,false}})
        {
            UClass* Class=C.Path?LoadObject<UClass>(nullptr,C.Path):nullptr;if(C.Path && !Class)return false;
            FStructOnScope Args(Probe);Input->SetObjectPropertyValue_InContainer(Args.GetStructMemory(),Class?Class->GetDefaultObject():nullptr);
            BP->GeneratedClass->GetDefaultObject()->ProcessEvent(Probe,Args.GetStructMemory());
            if(Large->GetPropertyValue_InContainer(Args.GetStructMemory())!=C.Large ||
               Medium->GetPropertyValue_InContainer(Args.GetStructMemory())!=C.Medium ||
               Optional->GetPropertyValue_InContainer(Args.GetStructMemory())!=(Changed?9:7))
            {UE_LOG(LogTemp,Error,TEXT("Library execution failed: %s input=%s actual large=%d medium=%d optional=%d"),Path,C.Path?C.Path:TEXT("null"),Large->GetPropertyValue_InContainer(Args.GetStructMemory()),Medium->GetPropertyValue_InContainer(Args.GetStructMemory()),Optional->GetPropertyValue_InContainer(Args.GetStructMemory()));return false;}
        }
    }
    auto* E=Entry(Library->FunctionGraphs[0]);const int32 Flags=E->GetExtraFlags();
    for(const int32 Unsupported:{int32(FUNC_Static),int32(FUNC_BlueprintPure)})
    {
        E->ClearExtraFlags(Unsupported);
        const bool Rejected=AssetText::UnsupportedReason(Library).Contains(TEXT("pure static"));
        E->SetExtraFlags(Flags);
        if(!Rejected){UE_LOG(LogTemp,Error,TEXT("Unsupported library flags were accepted."));return false;}
    }
    return true;
}
}
