#include "NativeVehicleGraphHelpers.h"

#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
UEdGraphPin* RequireExitActionCast(FGraph& G, UEdGraphPin* Object, UClass* Class)
{
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = Class; Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(Object, Cast->GetCastSourcePin());
    G.Tail = Cast->GetValidCastPin(); return Cast->GetCastResultPin();
}

UEdGraphPin* LoadStockInputReference(FGraph& G, const TCHAR* ObjectPath, UClass* Class)
{
    // Load stock data in preparation, not inside native possession/action collection.
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
    G.Default(Path, E::PathString, ObjectPath);
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Ref, AssetLoadingGraphNames::SoftObjectPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
    G.Link(G.Pin(Ref, P::ReturnValue), G.Pin(Load, AssetLoadingGraphNames::Asset)); G.Exec(Load);
    G.Require(G.Valid(G.Pin(Load, P::ReturnValue)));
    return RequireExitActionCast(G, G.Pin(Load, P::ReturnValue), Class);
}

UEdGraphPin* ReadNativeInputField(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field)
{
    auto* Get = NewObject<UK2Node_VariableGet>(G.Graph);
    Get->VariableReference.SetExternalMember(Field, Owner); G.Node(Get);
    G.Link(Target, G.Pin(Get, P::FunctionTarget)); return G.Pin(Get, Field);
}

void NativeInputFieldGuard(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field, FName Expected, bool Assign)
{
    G.Require(G.Valid(G.Read(Expected)));
    if (Assign)
    {
        auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
        Set->VariableReference.SetExternalMember(Field, Owner); G.Node(Set);
        G.Link(Target, G.Pin(Set, P::FunctionTarget)); G.Link(G.Read(Expected), G.Pin(Set, Field)); G.Exec(Set);
    }
    auto* Value = ReadNativeInputField(G, Target, Owner, Field);
    G.Require(G.Valid(Value));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Value, G.Read(Expected)));
}

void ConfigureCombinedOperatorPoint(FGraph& G)
{
    // This new actor's nonphysical root is the operator point, behind the
    // railgun at character height. Preserve the shell and native entry/exit.
    auto* Tags = NewObject<UK2Node_VariableSet>(G.Graph);
    Tags->VariableReference.SetExternalMember(CombinedVehicleNames::ComponentTags, UActorComponent::StaticClass());
    G.Node(Tags); G.Link(G.Read(V::Body), G.Pin(Tags, P::FunctionTarget));
    auto* Values = G.Node(NewObject<UK2Node_MakeArray>(G.Graph));
    G.Link(Values->GetOutputPin(), G.Pin(Tags, CombinedVehicleNames::ComponentTags));
    G.Default(Values, Values->GetPinName(0), CombinedVehicleNames::OperatorTag); G.Exec(Tags);
    auto* Tag = NewObject<UK2Node_VariableSet>(G.Graph);
    Tag->VariableReference.SetExternalMember(CombinedVehicleNames::ExitTag, AVoyageVehiclePawn::StaticClass());
    G.Node(Tag); G.Link(G.Read(V::Vehicle), G.Pin(Tag, P::FunctionTarget));
    G.Default(Tag, CombinedVehicleNames::ExitTag, CombinedVehicleNames::OperatorTag); G.Exec(Tag);
    auto* HasTag = G.Call(UActorComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UActorComponent, ComponentHasTag));
    G.Link(G.Read(V::Body), G.Pin(HasTag, P::FunctionTarget));
    // Engine tag argument is its own semantic identity, not an opaque operand.
    const FName TagArgument(TEXT("Tag"));
    G.Default(HasTag, TagArgument, CombinedVehicleNames::OperatorTag);
    G.Require(G.Pin(HasTag, P::ReturnValue));
}
}
