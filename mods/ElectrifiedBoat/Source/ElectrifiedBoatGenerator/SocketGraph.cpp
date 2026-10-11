#include "SocketGraph.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"

namespace ElectrifiedBoat
{
namespace Pins
{
const FName Execute(UEdGraphSchema_K2::PN_Execute);
const FName Then(UEdGraphSchema_K2::PN_Then);
const FName Condition(UEdGraphSchema_K2::PN_Condition);
const FName Object(UEdGraphSchema_K2::PN_ObjectToCast);
const FName ReturnValue(UEdGraphSchema_K2::PN_ReturnValue);
const FName Self(UEdGraphSchema_K2::PN_Self);
}

FSocketGraph::FSocketGraph(UEdGraph* InGraph) : Graph(InGraph)
{
    check(Graph);
}

UEdGraphPin* FSocketGraph::Pin(UEdGraphNode* InNode, const FName Name) const
{
    UEdGraphPin* Result = InNode->FindPin(Name);
    checkf(Result, TEXT("Missing pin '%s' on '%s'"),
        *Name.ToString(), *InNode->GetName());
    return Result;
}

void FSocketGraph::Link(UEdGraphPin* From, UEdGraphPin* To) const
{
    check(From && To);
    checkf(GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To),
        TEXT("Cannot link %s.%s to %s.%s"),
        *From->GetOwningNode()->GetName(), *From->PinName.ToString(),
        *To->GetOwningNode()->GetName(), *To->PinName.ToString());
}

void FSocketGraph::Default(
    UEdGraphNode* InNode,
    const FName Name,
    const FString& Value) const
{
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(
        *Pin(InNode, Name), Value);
}

void FSocketGraph::DefaultObject(
    UEdGraphNode* InNode,
    const FName Name,
    UObject* Value) const
{
    UEdGraphPin* TargetPin = Pin(InNode, Name);
    TargetPin->DefaultObject = Value;
    TargetPin->DefaultValue.Reset();
}

UK2Node_CallFunction* FSocketGraph::Call(
    UClass* Owner,
    const FName FunctionName,
    const int32 Y)
{
    UFunction* Function = Owner->FindFunctionByName(FunctionName);
    checkf(Function, TEXT("Missing reflected function %s.%s"),
        *Owner->GetPathName(), *FunctionName.ToString());
    checkf(Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure),
        TEXT("Function is not Blueprint-callable: %s"), *FunctionName.ToString());
    UK2Node_CallFunction* Result = NewObject<UK2Node_CallFunction>(Graph);
    Result->SetFromFunction(Function);
    return Node(Result, Y);
}

UK2Node_CallArrayFunction* FSocketGraph::ArrayCall(
    const FName FunctionName,
    const int32 Y)
{
    UFunction* Function = UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        FunctionName);
    checkf(Function, TEXT("Missing reflected array function %s"),
        *FunctionName.ToString());
    UK2Node_CallArrayFunction* Result =
        NewObject<UK2Node_CallArrayFunction>(Graph);
    Result->SetFromFunction(Function);
    return Node(Result, Y);
}

void FSocketGraph::Exec(UEdGraphNode* InNode)
{
    Link(Tail, Pin(InNode, Pins::Execute));
    Tail = Pin(InNode, Pins::Then);
}

UEdGraphPin* FSocketGraph::Read(const FName Name, const int32 Y)
{
    UK2Node_VariableGet* Get = NewObject<UK2Node_VariableGet>(Graph);
    Get->VariableReference.SetSelfMember(Name);
    Node(Get, Y);
    return Get->GetValuePin();
}

UK2Node_VariableSet* FSocketGraph::Write(
    const FName Name,
    UEdGraphPin* Value,
    const FString& Literal,
    const int32 Y)
{
    UK2Node_VariableSet* Set = NewObject<UK2Node_VariableSet>(Graph);
    Set->VariableReference.SetSelfMember(Name);
    Node(Set, Y);
    if (Value)
    {
        Link(Value, Pin(Set, Name));
    }
    else
    {
        Default(Set, Name, Literal);
    }
    Exec(Set);
    return Set;
}

UEdGraphPin* FSocketGraph::ReadExternal(
    const FName Name,
    UClass* Owner,
    UEdGraphPin* Object,
    const int32 Y)
{
    UK2Node_VariableGet* Get = NewObject<UK2Node_VariableGet>(Graph);
    Get->VariableReference.SetExternalMember(Name, Owner);
    Node(Get, Y);
    Link(Object, Pin(Get, Pins::Self));
    return Get->GetValuePin();
}

UK2Node_VariableSet* FSocketGraph::WriteExternal(
    const FName Name,
    UClass* Owner,
    UEdGraphPin* Object,
    UEdGraphPin* Value,
    const FString& Literal,
    const int32 Y)
{
    UK2Node_VariableSet* Set = NewObject<UK2Node_VariableSet>(Graph);
    Set->VariableReference.SetExternalMember(Name, Owner);
    Node(Set, Y);
    Link(Object, Pin(Set, Pins::Self));
    if (Value)
    {
        Link(Value, Pin(Set, Name));
    }
    else
    {
        Default(Set, Name, Literal);
    }
    Exec(Set);
    return Set;
}

UK2Node_IfThenElse* FSocketGraph::Branch(
    UEdGraphPin* Condition,
    const int32 Y)
{
    UK2Node_IfThenElse* Result = Node(NewObject<UK2Node_IfThenElse>(Graph), Y);
    Link(Tail, Pin(Result, Pins::Execute));
    Link(Condition, Pin(Result, Pins::Condition));
    Tail = Pin(Result, Pins::Then);
    return Result;
}

UEdGraphPin* FSocketGraph::Valid(UEdGraphPin* Object, const int32 Y)
{
    UK2Node_CallFunction* Function = Call(
        UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid),
        Y);
    Link(Object, Pin(Function, Pins::Object));
    return Pin(Function, Pins::ReturnValue);
}

void AddTransientObjectArrayVariable(
    UBlueprint* Blueprint,
    const FName Name,
    UClass* ObjectClass)
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Object;
    Type.PinSubCategoryObject = ObjectClass;
    Type.ContainerType = EPinContainerType::Array;
    check(FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, Type));
    FBPVariableDescription* Variable = Blueprint->NewVariables.FindByPredicate(
        [Name](const FBPVariableDescription& Candidate)
        {
            return Candidate.VarName == Name;
        });
    check(Variable);
    Variable->PropertyFlags |= CPF_Transient;
}

void AddTransientVariable(
    UBlueprint* Blueprint,
    const FName Name,
    const FEdGraphPinType& Type)
{
    check(FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, Type));
    FBPVariableDescription* Variable = Blueprint->NewVariables.FindByPredicate(
        [Name](const FBPVariableDescription& Candidate)
        {
            return Candidate.VarName == Name;
        });
    check(Variable);
    Variable->PropertyFlags |= CPF_Transient;
}
}
