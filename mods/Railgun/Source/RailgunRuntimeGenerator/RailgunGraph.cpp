#include "RailgunGraph.h"

#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
FGraph::FGraph(UEdGraph* InGraph) : Graph(InGraph)
{
}

UEdGraphPin* FGraph::Pin(UEdGraphNode* In, FName Name)
{
    auto* Out = In->FindPin(Name);
    checkf(Out, TEXT("Missing pin %s on %s"), *Name.ToString(),
        *In->GetName());
    return Out;
}

void FGraph::Link(UEdGraphPin* From, UEdGraphPin* To)
{
    checkf(GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To),
        TEXT("Cannot link %s -> %s"), *From->PinName.ToString(),
        *To->PinName.ToString());
}

void FGraph::Default(UEdGraphNode* In, FName Name, const TCHAR* Value)
{
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Pin(In, Name), Value);
}

UK2Node_CallFunction* FGraph::Call(UClass* Owner, FName Function)
{
    auto* Fn = Owner->FindFunctionByName(Function);
    checkf(Fn, TEXT("Missing reflected function %s.%s"),
        *Owner->GetPathName(), *Function.ToString());
    checkf(Fn->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure),
        TEXT("Function is not Blueprint-callable: %s"), *Function.ToString());
    auto* Out = NewObject<UK2Node_CallFunction>(Graph);
    Out->SetFromFunction(Fn);
    return Node(Out);
}

UK2Node_CallArrayFunction* FGraph::ArrayCall(FName Function)
{
    auto* Fn = UKismetArrayLibrary::StaticClass()->FindFunctionByName(Function);
    checkf(Fn, TEXT("Missing reflected array function %s"),
        *Function.ToString());
    auto* Out = NewObject<UK2Node_CallArrayFunction>(Graph);
    Out->SetFromFunction(Fn);
    return Node(Out);
}

void FGraph::Exec(UEdGraphNode* In)
{
    Link(Tail, Pin(In, P::Execute));
    Tail = Pin(In, P::Then);
}

UEdGraphPin* FGraph::Read(FName Name)
{
    auto* Get = NewObject<UK2Node_VariableGet>(Graph);
    Get->VariableReference.SetSelfMember(Name);
    Node(Get);
    return Pin(Get, Name);
}

void FGraph::Write(FName Name, UEdGraphPin* Value, const TCHAR* Literal)
{
    auto* Set = NewObject<UK2Node_VariableSet>(Graph);
    Set->VariableReference.SetSelfMember(Name);
    Node(Set);
    if (Value)
        Link(Value, Pin(Set, Name));
    else
        Default(Set, Name, Literal);
    Exec(Set);
}

UK2Node_IfThenElse* FGraph::Branch(UEdGraphPin* Condition)
{
    auto* Out = Node(NewObject<UK2Node_IfThenElse>(Graph));
    Link(Tail, Pin(Out, P::Execute));
    Link(Condition, Pin(Out, P::Condition));
    Tail = Pin(Out, P::Then);
    return Out;
}

void FGraph::Require(UEdGraphPin* Condition)
{
    Branch(Condition);
}

UEdGraphPin* FGraph::ActorArray(UEdGraphPin* FirstActor,
    UEdGraphPin* SecondActor)
{
    auto* Array = Node(NewObject<UK2Node_MakeArray>(Graph));
    Array->AddInputPin();
    Link(FirstActor, Pin(Array, Array->GetPinName(0)));
    Link(SecondActor, Pin(Array, Array->GetPinName(1)));
    return Array->GetOutputPin();
}

UEdGraphPin* FGraph::Valid(UEdGraphPin* Object)
{
    auto* Fn = Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid));
    Link(Object, Pin(Fn, P::Object));
    return Pin(Fn, P::ReturnValue);
}

UEdGraphPin* FGraph::Compare(FName Function, UEdGraphPin* Value,
    const TCHAR* Other)
{
    auto* Fn = Call(UKismetMathLibrary::StaticClass(), Function);
    Link(Value, Pin(Fn, P::Binary::LeftOperand));
    Default(Fn, P::Binary::RightOperand, Other);
    return Pin(Fn, P::ReturnValue);
}

UEdGraphPin* FGraph::Binary(FName Function, UEdGraphPin* Left,
    UEdGraphPin* Right)
{
    auto* Fn = Call(UKismetMathLibrary::StaticClass(), Function);
    Link(Left, Pin(Fn, P::Binary::LeftOperand));
    Link(Right, Pin(Fn, P::Binary::RightOperand));
    return Pin(Fn, P::ReturnValue);
}

UEdGraphPin* FGraph::Transform(UEdGraphPin* Location, UEdGraphPin* Rotation)
{
    auto* Make = Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform));
    Link(Location, Pin(Make, E::Location));
    Link(Rotation, Pin(Make, E::Rotation));
    Default(Make, E::Scale, N::UnitScale);
    return Pin(Make, P::ReturnValue);
}

UEdGraphPin* FGraph::Offset(UEdGraphPin* TransformValue, const TCHAR* Value)
{
    auto* Fn = Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, TransformLocation));
    Link(TransformValue, Pin(Fn, E::Transform));
    Default(Fn, E::LocalPosition, Value);
    return Pin(Fn, P::ReturnValue);
}

void AddVariable(UBlueprint* BP, FName Name, FName Category, UObject* Type)
{
    FEdGraphPinType PinType;
    PinType.PinCategory = Category;
    PinType.PinSubCategoryObject = Type;
    if (Category == UEdGraphSchema_K2::PC_Real)
        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    check(FBlueprintEditorUtils::AddMemberVariable(BP, Name, PinType));
}

void MarkVariableTransient(UBlueprint* BP, FName Name)
{
    check(BP);
    FBPVariableDescription* Variable = BP->NewVariables.FindByPredicate(
        [Name](const FBPVariableDescription& Candidate)
        {
            return Candidate.VarName == Name;
        });
    check(Variable);
    Variable->PropertyFlags |= CPF_Transient;
}

void AddArrayVariable(UBlueprint* BP, FName Name, FName Category, UObject* Type)
{
    FEdGraphPinType PinType;
    PinType.PinCategory = Category;
    PinType.PinSubCategoryObject = Type;
    PinType.ContainerType = EPinContainerType::Array;
    if (Category == UEdGraphSchema_K2::PC_Real)
        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    check(FBlueprintEditorUtils::AddMemberVariable(BP, Name, PinType));
}
}
