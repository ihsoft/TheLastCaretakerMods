// Isolated executable fixture; no game or mod assets participate.
#pragma once
#include "K2Node_IfThenElse.h"
#include "K2Node_CustomEvent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/StructOnScope.h"

namespace BlueprintNestedGraphTests
{
using namespace BlueprintObjectTests;
constexpr TCHAR Path[]=TEXT("/Game/BPTNestedFixture");
FEdGraphPinType Type(FName Category){FEdGraphPinType T;T.PinCategory=Category;return T;}
UK2Node_Composite* Collapse(UEdGraph* G,const TCHAR* Name)
{
    auto* C=Node<UK2Node_Composite>(G);C->PostPlacedNewNode();
    C->BoundGraph->Rename(Name,C,REN_DontCreateRedirectors);
    return C;
}
UEdGraphPin* Input(UK2Node_Composite* C,const TCHAR* Name,FName Category)
{
    return CastChecked<UK2Node_Tunnel>(C->InputSinkNode)->CreateUserDefinedPin(Name,Type(Category),EGPD_Output);
}
UEdGraphPin* Output(UK2Node_Composite* C,const TCHAR* Name,FName Category)
{
    return CastChecked<UK2Node_Tunnel>(C->OutputSourceNode)->CreateUserDefinedPin(Name,Type(Category),EGPD_Input);
}
UK2Node_CallFunction* Math(UEdGraph* G,const TCHAR* Name)
{
    auto* N=Node<UK2Node_CallFunction>(G);
    N->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(Name));N->AllocateDefaultPins();return N;
}
UK2Node_VariableSet* Trace(UEdGraph* G,int32 Digit,bool Append)
{
    auto* Set=Node<UK2Node_VariableSet>(G);Set->VariableReference.SetSelfMember(TEXT("Trace"));Set->AllocateDefaultPins();
    if(!Append)Set->FindPin(TEXT("Trace"))->DefaultValue=FString::FromInt(Digit);
    else
    {
        auto* Get=Node<UK2Node_VariableGet>(G);Get->VariableReference.SetSelfMember(TEXT("Trace"));Get->AllocateDefaultPins();
        auto* Mul=Math(G,TEXT("Multiply_IntInt"));Mul->FindPin(TEXT("B"))->DefaultValue=TEXT("10");
        auto* Add=Math(G,TEXT("Add_IntInt"));Add->FindPin(TEXT("B"))->DefaultValue=FString::FromInt(Digit);
        if(!Link(Get->FindPin(TEXT("Trace")),Mul->FindPin(TEXT("A"))) ||
           !Link(Mul->FindPin(TEXT("ReturnValue")),Add->FindPin(TEXT("A"))) ||
           !Link(Add->FindPin(TEXT("ReturnValue")),Set->FindPin(TEXT("Trace"))))return nullptr;
    }
    return Set;
}
bool BuildFunction(UBlueprint* BP,const TCHAR* Name)
{
    auto* G=Function(BP,Name);auto* E=Entry(G);
    E->CreateUserDefinedPin(TEXT("Input"),Type(UEdGraphSchema_K2::PC_Int),EGPD_Output);
    E->CreateUserDefinedPin(TEXT("Choose"),Type(UEdGraphSchema_K2::PC_Boolean),EGPD_Output);
    auto* C=Collapse(G,TEXT("Details"));auto* Child=C->BoundGraph.Get();
    auto* Exec=Input(C,TEXT("execute"),UEdGraphSchema_K2::PC_Exec);
    auto* Value=Input(C,TEXT("Input"),UEdGraphSchema_K2::PC_Int);
    auto* Choose=Input(C,TEXT("Choose"),UEdGraphSchema_K2::PC_Boolean);
    auto* Yes=Output(C,TEXT("Yes"),UEdGraphSchema_K2::PC_Exec);
    auto* No=Output(C,TEXT("No"),UEdGraphSchema_K2::PC_Exec);
    auto* ValueOut=Output(C,TEXT("Result"),UEdGraphSchema_K2::PC_Int);
    auto* Inner=Collapse(Child,TEXT("Arithmetic"));
    auto* InnerIn=Input(Inner,TEXT("Input"),UEdGraphSchema_K2::PC_Int);
    auto* InnerOut=Output(Inner,TEXT("Result"),UEdGraphSchema_K2::PC_Int);
    auto* Add=Math(Inner->BoundGraph,TEXT("Add_IntInt"));Add->FindPin(TEXT("B"))->DefaultValue=TEXT("3");
    auto* Branch=Node<UK2Node_IfThenElse>(Child);Branch->AllocateDefaultPins();
    auto* TrueSet=Trace(Child,2,true);auto* FalseSet=Trace(Child,3,true);auto* Initial=Trace(G,1,false);
    if(!TrueSet || !FalseSet || !Initial ||
       !Link(E->FindPin(TEXT("then")),Initial->FindPin(TEXT("execute"))) ||
       !Link(Initial->FindPin(TEXT("then")),C->FindPin(TEXT("execute"))) ||
       !Link(E->FindPin(TEXT("Input")),C->FindPin(TEXT("Input"))) ||
       !Link(E->FindPin(TEXT("Choose")),C->FindPin(TEXT("Choose"))) ||
       !Link(Exec,Branch->FindPin(TEXT("execute"))) || !Link(Choose,Branch->FindPin(TEXT("Condition"))) ||
       !Link(Branch->FindPin(TEXT("then")),TrueSet->FindPin(TEXT("execute"))) ||
       !Link(Branch->FindPin(TEXT("else")),FalseSet->FindPin(TEXT("execute"))) ||
       !Link(TrueSet->FindPin(TEXT("then")),Yes) || !Link(FalseSet->FindPin(TEXT("then")),No) ||
       !Link(Value,Inner->FindPin(TEXT("Input"))) || !Link(Inner->FindPin(TEXT("Result")),ValueOut) ||
       !Link(InnerIn,Add->FindPin(TEXT("A"))) || !Link(Add->FindPin(TEXT("ReturnValue")),InnerOut))return false;
    for(bool Positive:{true,false})
    {
        auto* Final=Trace(G,Positive?4:5,true);
        auto* R=Node<UK2Node_FunctionResult>(G);R->AllocateDefaultPins();
        R->CreateUserDefinedPin(TEXT("ReturnValue"),Type(UEdGraphSchema_K2::PC_Int),EGPD_Input);
        auto* Sum=Math(G,TEXT("Add_IntInt"));Sum->FindPin(TEXT("B"))->DefaultValue=Positive?TEXT("10"):TEXT("-10");
        if(!Final || !Link(C->FindPin(Positive?TEXT("Yes"):TEXT("No")),Final->FindPin(TEXT("execute"))) ||
           !Link(Final->FindPin(TEXT("then")),R->FindPin(TEXT("execute"))) ||
           !Link(C->FindPin(TEXT("Result")),Sum->FindPin(TEXT("A"))) ||
           !Link(Sum->FindPin(TEXT("ReturnValue")),R->FindPin(TEXT("ReturnValue"))))return false;
    }
    return true;
}
bool Fixture()
{
    auto* BP=Create(Path,UObject::StaticClass());if(!BP)return false;
    if(!(FBlueprintEditorUtils::AddMemberVariable(BP,TEXT("Trace"),Type(UEdGraphSchema_K2::PC_Int)) &&
        BuildFunction(BP,TEXT("RunProbe")) && BuildFunction(BP,TEXT("RunOther")) && Compile(BP) && Save(BP)))return false;
    auto* Actor=Create(TEXT("/Game/BPTNestedActor"),AActor::StaticClass());
    if(!Actor || !FBlueprintEditorUtils::AddMemberVariable(Actor,TEXT("Trace"),Type(UEdGraphSchema_K2::PC_Int)))return false;
    auto* Construction=FBlueprintEditorUtils::FindUserConstructionScript(Actor);
    auto* Events=FBlueprintEditorUtils::FindEventGraph(Actor);
    if(!Construction || !Events)return false;
    auto* Event=Node<UK2Node_CustomEvent>(Events);Event->CustomFunctionName=TEXT("RunEvent");Event->AllocateDefaultPins();
    for(UEdGraph* G:{Construction,Events})
    {
        auto* C=Collapse(G,TEXT("Details"));
        auto* Exec=Input(C,TEXT("execute"),UEdGraphSchema_K2::PC_Exec);
        auto* Then=Output(C,TEXT("then"),UEdGraphSchema_K2::PC_Exec);
        auto* Set=Trace(C->BoundGraph,G==Construction?91:42,false);
        UEdGraphPin* Start=G==Construction?Entry(G)->FindPin(TEXT("then")):Event->FindPin(TEXT("then"));
        if(!Set || !Link(Start,C->FindPin(TEXT("execute"))) || !Link(Exec,Set->FindPin(TEXT("execute"))) ||
           !Link(Set->FindPin(TEXT("then")),Then))return false;
        auto* Tunnel=CastChecked<UK2Node_Tunnel>(C->InputSinkNode);
        auto Vector=Type(UEdGraphSchema_K2::PC_Struct);Vector.PinSubCategoryObject=TBaseStructure<FVector>::Get();
        Tunnel->CreateUserDefinedPin(TEXT("Direction"),Vector,EGPD_Output);
        C->FindPin(TEXT("Direction"))->DefaultValue=TEXT("1.000000,2.000000,3.000000");
        auto Array=Type(UEdGraphSchema_K2::PC_Int);Array.ContainerType=EPinContainerType::Array;
        Tunnel->CreateUserDefinedPin(TEXT("Values"),Array,EGPD_Output);
        auto Object=Type(UEdGraphSchema_K2::PC_Object);Object.PinSubCategoryObject=UStaticMesh::StaticClass();
        Tunnel->CreateUserDefinedPin(TEXT("Mesh"),Object,EGPD_Output);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));if(!Mesh)return false;
        GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(*C->FindPin(TEXT("Mesh")),Mesh);
    }
    return Compile(Actor) && Save(Actor);
}
bool Check(int32 Offset)
{
    auto* BP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/BPTNestedFixture.BPTNestedFixture"));
    if(!BP || !AssetText::Supported(BP))return false;
    auto* Instance=NewObject<UObject>(GetTransientPackage(),BP->GeneratedClass);
    auto* TraceProperty=FindFProperty<FIntProperty>(BP->GeneratedClass,TEXT("Trace"));
    for(const FName Name:{FName(TEXT("RunProbe")),FName(TEXT("RunOther"))})for(bool Choose:{false,true})
    {
        UFunction* F=Instance->FindFunction(Name);if(!F || !TraceProperty)return false;
        FStructOnScope Args(F);
        auto* In=FindFProperty<FIntProperty>(F,TEXT("Input"));
        auto* Choice=FindFProperty<FBoolProperty>(F,TEXT("Choose"));
        auto* Out=FindFProperty<FIntProperty>(F,TEXT("ReturnValue"));if(!In || !Choice || !Out)return false;
        In->SetPropertyValue_InContainer(Args.GetStructMemory(),5);
        Choice->SetPropertyValue_InContainer(Args.GetStructMemory(),Choose);
        Instance->ProcessEvent(F,Args.GetStructMemory());
        const int32 Expected=5+(Name==TEXT("RunProbe")?Offset:3)+(Choose?10:-10);
        if(Out->GetPropertyValue_InContainer(Args.GetStructMemory())!=Expected ||
           TraceProperty->GetPropertyValue_InContainer(Instance)!=(Choose?124:135))
        {
            UE_LOG(LogTemp,Error,TEXT("Nested fixture mismatch: %s choose=%d value=%d expected=%d trace=%d"),
                *Name.ToString(),Choose,Out->GetPropertyValue_InContainer(Args.GetStructMemory()),Expected,
                TraceProperty->GetPropertyValue_InContainer(Instance));return false;
        }
    }
    auto* Actor=LoadObject<UBlueprint>(nullptr,TEXT("/Game/BPTNestedActor.BPTNestedActor"));
    if(!Actor || !AssetText::Supported(Actor))return false;
    auto* CDO=Actor->GeneratedClass->GetDefaultObject();
    auto* ActorTrace=FindFProperty<FIntProperty>(Actor->GeneratedClass,TEXT("Trace"));if(!ActorTrace)return false;
    for(const FName Name:{UEdGraphSchema_K2::FN_UserConstructionScript,FName(TEXT("RunEvent"))})
    {
        auto* Function=CDO->FindFunction(Name);if(!Function)return false;
        ActorTrace->SetPropertyValue_InContainer(CDO,0);
        CDO->ProcessEvent(Function,nullptr);
        if(ActorTrace->GetPropertyValue_InContainer(CDO)!=(Name==UEdGraphSchema_K2::FN_UserConstructionScript?91:42))return false;
    }
    return true;
}
}
