#include "RailgunStockInput.h"

#include "GraphCallHelpers.h"
#include "NativeVehicleGraphHelpers.h"
#include "RailgunInputNames.h"
#include "RailgunRuntimeGeneratorPrivate.h"
#include "RailgunStationInitialization.h"
#include "StationEntryGraph.h"
#include "StationOpticsGraph.h"
#include "EnhancedActionKeyMapping.h"
#include "InputMappingContext.h"
#include "VoyageInputContextAsset.h"
#include "VoyageInputControlsComponent.h"
#include "VoyageItemHandEquipment.h"

namespace Railgun::Runtime
{
namespace
{
UEdGraphPin* DuplicateObject(FGraph& G, UEdGraphPin* Source,
    UEdGraphPin* Outer, UClass* Class)
{
    auto* Duplicate = G.Call(UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageMiscBlueprintFunctionLibrary,
            DuplicateObjectVoyage));
    G.Link(Source, G.Pin(Duplicate, StockInput::Object));
    G.Link(Outer, G.Pin(Duplicate, StockInput::Outer));
    G.Exec(Duplicate);
    return RequireExitActionCast(G, G.Pin(Duplicate, P::ReturnValue), Class);
}

UEdGraphPin* ContextMap(FGraph& G, UEdGraphPin* Context)
{
    return ReadNativeInputField(G, Context,
        UVoyageInputContextAsset::StaticClass(), StockInput::ContextMap);
}

void AddMapEntry(FGraph& G, UEdGraphPin* Context, const TCHAR* Device,
    UEdGraphPin* Mapping)
{
    auto* Add = G.Call(UBlueprintMapLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Add));
    G.Link(ContextMap(G, Context), G.Pin(Add, StockInput::TargetMap));
    G.Default(Add, StockInput::Key, Device);
    G.Link(Mapping, G.Pin(Add, StockInput::Value));
    G.Exec(Add);
}

void ClearContextMap(FGraph& G, UEdGraphPin* Context)
{
    auto* Clear = G.Call(UBlueprintMapLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Clear));
    G.Link(ContextMap(G, Context), G.Pin(Clear, StockInput::TargetMap));
    G.Exec(Clear);
}

UEdGraphPin* Or(FGraph& G, UEdGraphPin* Left, UEdGraphPin* Right)
{
    return G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        Left, Right);
}

void RequireNoMappingProfileOverrides(FGraph& G, UEdGraphPin* Mapping)
{
    UEdGraphPin* Profiles = ReadNativeInputField(G, Mapping,
        UInputMappingContext::StaticClass(), StockInput::ProfileOverrides);
    auto* Length = G.Call(UBlueprintMapLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Length));
    G.Link(Profiles, G.Pin(Length, StockInput::TargetMap));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        EqualEqual_IntInt), G.Pin(Length, P::ReturnValue), StockInput::Zero));
}

void FilterMapping(FGraph& G, UEdGraphPin* Mapping, FName Snapshot,
    const TArray<UEdGraphPin*>& AllowedActions)
{
    RequireNoMappingProfileOverrides(G, Mapping);
    UEdGraphPin* Defaults = ReadNativeInputField(G, Mapping,
        UInputMappingContext::StaticClass(), StockInput::DefaultMappings);
    auto* BreakDefaults = NewObject<UK2Node_BreakStruct>(G.Graph);
    BreakDefaults->StructType = FInputMappingContextMappingData::StaticStruct();
    G.Node(BreakDefaults);
    UEdGraphPin* DefaultsInput = nullptr;
    for (UEdGraphPin* Pin : BreakDefaults->Pins)
        if (Pin->Direction == EGPD_Input) DefaultsInput = Pin;
    check(DefaultsInput);
    G.Link(Defaults, DefaultsInput);
    G.Write(Snapshot, G.Pin(BreakDefaults, StockInput::Mappings));

    auto* Loop = ContextLoop(G, G.Read(Snapshot));
    auto* BreakMapping = NewObject<UK2Node_BreakStruct>(G.Graph);
    BreakMapping->StructType = FEnhancedActionKeyMapping::StaticStruct();
    G.Node(BreakMapping);
    UEdGraphPin* MappingInput = nullptr;
    for (UEdGraphPin* Pin : BreakMapping->Pins)
        if (Pin->Direction == EGPD_Input) MappingInput = Pin;
    check(MappingInput);
    G.Link(G.Pin(Loop, CE::ArrayElement), MappingInput);
    UEdGraphPin* Action = G.Pin(BreakMapping, StockInput::Action);
    check(AllowedActions.Num() > 0);
    UEdGraphPin* Allowed = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        Action, AllowedActions[0]);
    for (int32 Index = 1; Index < AllowedActions.Num(); ++Index)
        Allowed = Or(G, Allowed, G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                EqualEqual_ObjectObject), Action, AllowedActions[Index]));
    auto* Keep = G.Branch(Allowed);
    G.Tail = G.Pin(Keep, P::Else);
    auto* Unmap = G.Call(UInputMappingContext::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UInputMappingContext,
            UnmapAllKeysFromAction));
    G.Link(Mapping, G.Pin(Unmap, P::FunctionTarget));
    G.Link(Action, G.Pin(Unmap, StockInput::Action));
    G.Exec(Unmap);
    G.Tail = G.Pin(Loop, CE::Completed);
}

UEdGraphPin* CloneAndFilterMapping(FGraph& G, UEdGraphPin* Source,
    UEdGraphPin* Context, FName Snapshot,
    const TArray<UEdGraphPin*>& AllowedActions)
{
    UEdGraphPin* Clone = DuplicateObject(G, Source, Context,
        UInputMappingContext::StaticClass());
    FilterMapping(G, Clone, Snapshot, AllowedActions);
    return Clone;
}

void CaptureMapping(FGraph& G, UEdGraphPin* Context, const TCHAR* Device,
    FName Destination)
{
    auto* Find = G.Call(UBlueprintMapLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Find));
    G.Link(ContextMap(G, Context), G.Pin(Find, StockInput::TargetMap));
    G.Default(Find, StockInput::Key, Device);
    G.Require(G.Pin(Find, P::ReturnValue));
    G.Write(Destination, G.Pin(Find, StockInput::Value));
    G.Require(G.Valid(G.Read(Destination)));
}
}

void AddRailgunStockInputPreparation(UBlueprint* BP)
{
    for (const auto& Field : {
        TPair<FName, UClass*>(StockInput::CharacterContext,
            UVoyageInputContextAsset::StaticClass()),
        TPair<FName, UClass*>(StockInput::ExitContext,
            UVoyageInputContextAsset::StaticClass()),
        TPair<FName, UClass*>(StockInput::ServiceItem,
            UVoyageItemHandEquipment::StaticClass())})
    {
        AddVariable(BP, Field.Key, UEdGraphSchema_K2::PC_Object, Field.Value);
        MarkVariableTransient(BP, Field.Key);
    }
    AddArrayVariable(BP, StockInput::MappingSnapshot,
        UEdGraphSchema_K2::PC_Struct,
        FEnhancedActionKeyMapping::StaticStruct());
    MarkVariableTransient(BP, StockInput::MappingSnapshot);
    AddVariable(BP, StockInput::Ready, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, StockInput::FailureStage,
        UEdGraphSchema_K2::PC_String);
    MarkVariableTransient(BP, StockInput::Ready);
    MarkVariableTransient(BP, StockInput::FailureStage);
    for (FName Field : {StockInput::CharacterKeyboard,
        StockInput::CharacterGamepad, StockInput::ExitKeyboard,
        StockInput::ExitGamepad})
    {
        AddVariable(BP, Field, UEdGraphSchema_K2::PC_Object,
            UInputMappingContext::StaticClass());
        MarkVariableTransient(BP, Field);
    }
    FKismetEditorUtilities::CompileBlueprint(BP);

    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
        StockInput::Prepare, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    check(Entry);
    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph G(Graph);
    G.Tail = G.Pin(Entry, P::Then);
    G.Write(StockInput::Ready, nullptr, N::False);
    G.Write(StockInput::FailureStage, nullptr, StockInput::CloneStage);
    UEdGraphPin* Owner = OpticalSelf(G);
    G.Write(StockInput::CharacterContext, DuplicateObject(G,
        LoadStockInputReference(G, RailgunInputNames::StockCharacterContext,
            UVoyageInputContextAsset::StaticClass()), Owner,
        UVoyageInputContextAsset::StaticClass()));
    G.Write(StockInput::ExitContext, DuplicateObject(G,
        LoadStockInputReference(G, RailgunInputNames::StockVehicleContext,
            UVoyageInputContextAsset::StaticClass()), Owner,
        UVoyageInputContextAsset::StaticClass()));
    G.Write(StockInput::ServiceItem, LoadStockInputReference(G,
        RailgunInputNames::ServiceEquipmentObject,
        UVoyageItemHandEquipment::StaticClass()));

    UEdGraphPin* Character = G.Read(StockInput::CharacterContext);
    UEdGraphPin* Exit = G.Read(StockInput::ExitContext);
    G.Write(StockInput::FailureStage, nullptr, StockInput::SourceStage);
    CaptureMapping(G, Character, StockInput::MouseKeyboard,
        StockInput::CharacterKeyboard);
    CaptureMapping(G, Character, StockInput::Gamepad,
        StockInput::CharacterGamepad);
    CaptureMapping(G, Exit, StockInput::MouseKeyboard,
        StockInput::ExitKeyboard);
    CaptureMapping(G, Exit, StockInput::Gamepad,
        StockInput::ExitGamepad);
    for (UEdGraphPin* Context : {Character, Exit})
    {
        G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_BoolBool), G.Valid(ReadNativeInputField(G, Context,
                UVoyageInputContextAsset::StaticClass(),
                StockInput::SwitchGamepadContext)), N::False));
        ClearContextMap(G, Context);
    }
    const TArray<UEdGraphPin*> CharacterActions {
        LoadStockInputReference(G, RailgunInputNames::Fire,
            UInputAction::StaticClass()),
        LoadStockInputReference(G, RailgunInputNames::Zoom,
            UInputAction::StaticClass()),
        LoadStockInputReference(G, RailgunInputNames::Reload,
            UInputAction::StaticClass()),
        LoadStockInputReference(G, RailgunInputNames::LookYaw,
            UInputAction::StaticClass()),
        LoadStockInputReference(G, RailgunInputNames::LookPitch,
            UInputAction::StaticClass())};
    const TArray<UEdGraphPin*> ExitActions {
        LoadStockInputReference(G, RailgunInputNames::Exit,
            UInputAction::StaticClass())};
    G.Write(StockInput::FailureStage, nullptr, StockInput::FilterStage);
    AddMapEntry(G, Character, StockInput::MouseKeyboard,
        CloneAndFilterMapping(G, G.Read(StockInput::CharacterKeyboard),
            Character, StockInput::MappingSnapshot, CharacterActions));
    AddMapEntry(G, Character, StockInput::Gamepad,
        CloneAndFilterMapping(G, G.Read(StockInput::CharacterGamepad),
            Character, StockInput::MappingSnapshot, CharacterActions));
    AddMapEntry(G, Exit, StockInput::MouseKeyboard,
        CloneAndFilterMapping(G, G.Read(StockInput::ExitKeyboard),
            Exit, StockInput::MappingSnapshot, ExitActions));
    AddMapEntry(G, Exit, StockInput::Gamepad,
        CloneAndFilterMapping(G, G.Read(StockInput::ExitGamepad),
            Exit, StockInput::MappingSnapshot, ExitActions));
    G.Write(StockInput::FailureStage, nullptr, StockInput::NoFailure);
    G.Write(StockInput::Ready, nullptr, N::True);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(StockInput::Prepare));
}

void AddRailgunStockInputComponents(UBlueprint* BP)
{
    auto* ExitContext = LoadObject<UVoyageInputContextAsset>(nullptr,
        RailgunInputNames::ExitContextObject);
    check(ExitContext);
    auto* ExitControls = BP->SimpleConstructionScript->CreateNode(
        UVoyageInputControlsComponent::StaticClass(), StockInput::ExitControls);
    CastChecked<UVoyageInputControlsComponent>(
        ExitControls->ComponentTemplate)->InputContextAsset = ExitContext;
    BP->SimpleConstructionScript->AddNode(ExitControls);
    for (const auto& Field : {
        TPair<FName, UClass*>(StockInput::CharacterContext,
            UVoyageInputContextAsset::StaticClass()),
        TPair<FName, UClass*>(StockInput::ExitContext,
            UVoyageInputContextAsset::StaticClass())})
    {
        AddVariable(BP, Field.Key, UEdGraphSchema_K2::PC_Object, Field.Value);
        MarkVariableTransient(BP, Field.Key);
    }
    FKismetEditorUtilities::CompileBlueprint(BP);
    UEdGraph* Construction =
        FBlueprintEditorUtils::FindUserConstructionScript(BP);
    check(Construction);
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Construction->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    check(Entry);
    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph G(Construction);
    G.Tail = G.Pin(Entry, P::Then);
    UEdGraphPin* Controls = ReadNativeInputField(G, OpticalSelf(G),
        AVoyageVehiclePawn::StaticClass(), NativeInputNames::ControlsField);
    ContextSet(G, Controls, UVoyageInputControlsComponent::StaticClass(),
        NativeInputNames::ContextField, G.Read(StockInput::CharacterContext));
    UEdGraphPin* ExitComponent = G.Read(StockInput::ExitControls);
    ContextSet(G, ExitComponent, UVoyageInputControlsComponent::StaticClass(),
        NativeInputNames::ContextField, G.Read(StockInput::ExitContext));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
}
}
