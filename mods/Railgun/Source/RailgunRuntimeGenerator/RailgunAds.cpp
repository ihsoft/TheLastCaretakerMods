#include "RailgunAds.h"

#include "NativeVehicleGraphHelpers.h"
#include "RailgunInputNames.h"
#include "RailgunRuntimeGeneratorPrivate.h"
#include "StationAttachmentGraph.h"
#include "StationOpticsGraph.h"
#include "VoyageEquipmentActor.h"
#include "VoyageItemHandEquipment.h"
#include "VoyageWeaponHolsterComponent.h"

namespace Railgun::Runtime
{
namespace
{
UEdGraphPin* Holster(FGraph& G, UClass* StationClass)
{
    return ReadNativeInputField(G, OpticalSelf(G), StationClass,
        RailgunAds::Holster);
}

UEdGraphPin* ActiveEquipment(FGraph& G, UClass* StationClass)
{
    auto* Active = G.Call(UVoyageWeaponHolsterComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageWeaponHolsterComponent,
            GetActiveSlotActor));
    G.Link(Holster(G, StationClass), G.Pin(Active, P::FunctionTarget));
    G.Default(Active, RailgunAds::Ducked, RailgunAds::False);
    return G.Pin(Active, P::ReturnValue);
}

UEdGraphPin* OverlayKey(FGraph& G)
{
    auto* Literal = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralName));
    G.Default(Literal, RailgunAds::LiteralValue,
        *RailgunAds::OverlayKey.ToString());
    return G.Pin(Literal, P::ReturnValue);
}
}

void SetRailgunAdsState(FGraph& G, UClass* StationClass, bool Enabled)
{
    UEdGraphPin* Equipment = ActiveEquipment(G, StationClass);
    G.Require(G.Valid(Equipment));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_ObjectObject), Equipment, G.Read(RailgunAds::Equipment)));
    auto* Set = G.Call(AVoyageEquipmentActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AVoyageEquipmentActor, SetInterpState));
    G.Link(Equipment, G.Pin(Set, P::FunctionTarget));
    G.Default(Set, RailgunAds::InType, RailgunAds::AdsEnum);
    G.Default(Set, RailgunAds::IsSet,
        Enabled ? RailgunAds::True : RailgunAds::False);
    G.Exec(Set);
}

UEdGraphPin* ReadRailgunAdsAmount(FGraph& G, UClass* StationClass)
{
    UEdGraphPin* Equipment = ActiveEquipment(G, StationClass);
    G.Require(G.Valid(Equipment));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_ObjectObject), Equipment, G.Read(RailgunAds::Equipment)));
    auto* Amount = G.Call(AVoyageEquipmentActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AVoyageEquipmentActor,
            GetInterpStateAmount));
    G.Link(Equipment, G.Pin(Amount, P::FunctionTarget));
    G.Default(Amount, RailgunAds::InType, RailgunAds::AdsEnum);
    return G.Pin(Amount, P::ReturnValue);
}

UEdGraphPin* ReadRailgunAdsActive(FGraph& G, UClass* StationClass)
{
    UEdGraphPin* Equipment = ActiveEquipment(G, StationClass);
    G.Require(G.Valid(Equipment));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_ObjectObject), Equipment, G.Read(RailgunAds::Equipment)));
    auto* Active = G.Call(AVoyageEquipmentActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AVoyageEquipmentActor, HasInterpState));
    G.Link(Equipment, G.Pin(Active, P::FunctionTarget));
    G.Default(Active, RailgunAds::InType, RailgunAds::AdsEnum);
    return G.Pin(Active, P::ReturnValue);
}

void SyncRailgunAdsService(FGraph& G, UClass* StationClass)
{
    G.Write(RailgunAds::Ready, nullptr, N::False);
    G.Write(RailgunAds::Equipment, nullptr);
    UClass* ServiceClass = LoadObject<UClass>(nullptr,
        RailgunInputNames::ServiceEquipmentActorClass);
    check(ServiceClass);
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = ServiceClass;
    Cast->SetPurity(false);
    G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute));
    G.Link(ActiveEquipment(G, StationClass), Cast->GetCastSourcePin());
    G.Tail = Cast->GetValidCastPin();
    G.Write(RailgunAds::Equipment, Cast->GetCastResultPin());
    G.Write(RailgunAds::Ready, nullptr, N::True);
}

void AddRailgunAdsSupport(UBlueprint* BP)
{
    auto* HolsterNode = BP->SimpleConstructionScript->CreateNode(
        UVoyageWeaponHolsterComponent::StaticClass(), RailgunAds::Holster);
    auto* HolsterTemplate = CastChecked<UVoyageWeaponHolsterComponent>(
        HolsterNode->ComponentTemplate);
    FVoyageHolsterSlotConfig OverlaySlot;
    OverlaySlot.Type = EVoyageHolsterSlotType::Overlay;
    HolsterTemplate->HolsterSlots = {OverlaySlot};
    HolsterTemplate->Slots.Reset();
    HolsterTemplate->SkeletonSocketTag = NAME_None;
    HolsterTemplate->EquipmentComponentTag = NAME_None;
    BP->SimpleConstructionScript->AddNode(HolsterNode);
    AddVariable(BP, RailgunAds::ServiceItem,
        UEdGraphSchema_K2::PC_Object, UVoyageItemHandEquipment::StaticClass());
    AddVariable(BP, RailgunAds::Equipment,
        UEdGraphSchema_K2::PC_Object, AVoyageEquipmentActor::StaticClass());
    AddVariable(BP, RailgunAds::Amount, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, RailgunAds::Ready, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, RailgunAds::Accepted, UEdGraphSchema_K2::PC_Boolean);
    MarkVariableTransient(BP, RailgunAds::ServiceItem);
    MarkVariableTransient(BP, RailgunAds::Equipment);
    MarkVariableTransient(BP, RailgunAds::Amount);
    MarkVariableTransient(BP, RailgunAds::Ready);
    MarkVariableTransient(BP, RailgunAds::Accepted);
    FKismetEditorUtilities::CompileBlueprint(BP);

    auto AddFunction = [&](FName Name)
    {
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, Name,
            UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
        FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
            static_cast<UClass*>(nullptr));
        UK2Node_FunctionEntry* Entry = nullptr;
        for (UEdGraphNode* Node : Graph->Nodes)
            if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
                Entry = Candidate;
        check(Entry);
        Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
        return TPair<UEdGraph*, UK2Node_FunctionEntry*>(Graph, Entry);
    };

    auto EnsureFunction = AddFunction(RailgunAds::Ensure);
    FGraph Ensure(EnsureFunction.Key);
    Ensure.Tail = Ensure.Pin(EnsureFunction.Value, P::Then);
    Ensure.Write(RailgunAds::Ready, nullptr, N::False);
    Ensure.Write(RailgunAds::Accepted, nullptr, N::False);
    Ensure.Branch(Ensure.Valid(Holster(Ensure, BP->GeneratedClass)));
    Ensure.Branch(Ensure.Valid(Ensure.Read(RailgunAds::ServiceItem)));
    auto* Override = NewObject<UK2Node_MakeStruct>(Ensure.Graph);
    Override->StructType = FVoyageHolsterSlotOverride::StaticStruct();
    Override->bMadeAfterOverridePinRemoval = true;
    Ensure.Node(Override);
    Ensure.Default(Override, RailgunAds::OverridePriority,
        RailgunAds::Priority);
    Ensure.Link(Ensure.Read(RailgunAds::ServiceItem),
        Ensure.Pin(Override, RailgunAds::OverrideItem));
    auto* Add = Ensure.Call(UVoyageWeaponHolsterComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageWeaponHolsterComponent,
            AddOverlayState));
    Ensure.Link(Holster(Ensure, BP->GeneratedClass),
        Ensure.Pin(Add, P::FunctionTarget));
    Ensure.Link(OverlayKey(Ensure), Ensure.Pin(Add, RailgunAds::InKey));
    UEdGraphPin* OverrideOut = nullptr;
    for (UEdGraphPin* Pin : Override->Pins)
        if (Pin->Direction == EGPD_Output) OverrideOut = Pin;
    check(OverrideOut);
    Ensure.Link(OverrideOut, Ensure.Pin(Add, RailgunAds::InOverride));
    Ensure.Default(Add, RailgunAds::ForceAdd, RailgunAds::True);
    Ensure.Exec(Add);
    Ensure.Write(RailgunAds::Accepted, Ensure.Pin(Add, P::ReturnValue));
    Ensure.Write(RailgunAds::Amount, nullptr, N::Zero);

    auto ResetFunction = AddFunction(RailgunAds::Reset);
    FGraph Reset(ResetFunction.Key);
    Reset.Tail = Reset.Pin(ResetFunction.Value, P::Then);
    auto* EquipmentValid = Reset.Branch(
        Reset.Valid(Reset.Read(RailgunAds::Equipment)));
    auto* Set = Reset.Call(AVoyageEquipmentActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AVoyageEquipmentActor, SetInterpState));
    Reset.Link(Reset.Read(RailgunAds::Equipment),
        Reset.Pin(Set, P::FunctionTarget));
    Reset.Default(Set, RailgunAds::InType, RailgunAds::AdsEnum);
    Reset.Default(Set, RailgunAds::IsSet, RailgunAds::False);
    Reset.Exec(Set);
    UEdGraphPin* ActorReset = Reset.Tail;
    Reset.Tail = Reset.Pin(EquipmentValid, P::Else);
    StationMerge(Reset, {ActorReset, Reset.Tail});
    auto* Remove = Reset.Call(UVoyageWeaponHolsterComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageWeaponHolsterComponent,
            RemoveOverlayState));
    Reset.Link(Holster(Reset, BP->GeneratedClass),
        Reset.Pin(Remove, P::FunctionTarget));
    Reset.Link(OverlayKey(Reset), Reset.Pin(Remove, RailgunAds::InKey));
    Reset.Exec(Remove);
    Reset.Write(RailgunAds::Equipment, nullptr);
    Reset.Write(RailgunAds::Amount, nullptr, N::Zero);
    Reset.Write(RailgunAds::Ready, nullptr, N::False);
    Reset.Write(RailgunAds::Accepted, nullptr, N::False);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(RailgunAds::Ensure) &&
        BP->GeneratedClass->FindFunctionByName(RailgunAds::Reset));
}
}
