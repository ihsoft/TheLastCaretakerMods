#include "StationActionHints.h"

#include "GraphCallHelpers.h"
#include "StationOpticsGraph.h"
#include "RailgunRuntimeGeneratorPrivate.h"
#include "RailgunReload.h"
#include "K2Node_Literal.h"

namespace Railgun::Runtime
{
void AddStationActions(UBlueprint* BP)
{
    const FName Name = GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, GetProvidedActionsBP);
    check(AVoyageVehiclePawn::StaticClass()->FindFunctionByName(Name)->GetOuterUClass() == AVoyageVehiclePawn::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false, AVoyageVehiclePawn::StaticClass());
    UK2Node_FunctionEntry* Entry = nullptr; UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Found = Cast<UK2Node_FunctionEntry>(Node)) Entry = Found;
        if (auto* Found = Cast<UK2Node_FunctionResult>(Node)) Result = Found;
    }
    check(Entry && Result); FGraph G(Graph);
    const FName ActionDescription = GET_MEMBER_NAME_CHECKED(
        UInputAction, ActionDescription);
    auto ReadActionDescription = [&](UInputAction* InputAction)
    {
        auto* Literal = G.Node(NewObject<UK2Node_Literal>(Graph));
        Literal->SetObjectRef(InputAction);
        auto* Get = NewObject<UK2Node_VariableGet>(Graph);
        Get->VariableReference.SetExternalMember(ActionDescription,
            UInputAction::StaticClass());
        G.Node(Get);
        G.Link(Literal->GetValuePin(), G.Pin(Get, P::FunctionTarget));
        return G.Pin(Get, ActionDescription);
    };
    G.Pin(Entry, P::Then)->BreakAllPinLinks(); G.Pin(Result, P::Execute)->BreakAllPinLinks();
    G.Tail = G.Pin(Entry, P::Then);
    G.Link(G.Tail, G.Pin(Result, P::Execute));
    auto* Action = NewObject<UK2Node_MakeStruct>(Graph); Action->StructType = FPlayerInputInterfaceAction::StaticStruct();
    Action->bMadeAfterOverridePinRemoval = true; G.Node(Action);
    auto* ExitAction = LoadObject<UInputAction>(nullptr, RailgunInputNames::Exit); check(ExitAction);
    G.Pin(Action, Hint::InputAction)->DefaultObject = ExitAction;
    G.Default(Action, Hint::Name, Hint::ActionName); G.Default(Action, Hint::Category, Hint::ActionCategory);
    G.Link(ReadActionDescription(ExitAction), G.Pin(Action, Hint::Text));
    G.Link(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), OpticalSelf(G)), G.Pin(Action, Hint::Enabled));
    G.Default(Action, Hint::Type, Hint::Central);
    auto* Array = G.Node(NewObject<UK2Node_MakeArray>(Graph));
    UEdGraphPin* ActionOutput = nullptr; UEdGraphPin* FirstElement = nullptr;
    for (auto* Pin : Action->Pins) if (Pin->Direction == EGPD_Output) ActionOutput = Pin;
    for (auto* Pin : Array->Pins) if (Pin->Direction == EGPD_Input) { FirstElement = Pin; break; }
    check(ActionOutput && FirstElement); G.Link(ActionOutput, FirstElement);
    auto* ZoomAction = NewObject<UK2Node_MakeStruct>(Graph); ZoomAction->StructType = FPlayerInputInterfaceAction::StaticStruct();
    ZoomAction->bMadeAfterOverridePinRemoval = true; G.Node(ZoomAction);
    auto* ZoomInput = LoadObject<UInputAction>(nullptr, RailgunInputNames::Zoom); check(ZoomInput);
    G.Pin(ZoomAction, Hint::InputAction)->DefaultObject = ZoomInput;
    G.Default(ZoomAction, Hint::Name, Hint::ZoomName); G.Default(ZoomAction, Hint::Category, Hint::ActionCategory);
    G.Link(ReadActionDescription(ZoomInput), G.Pin(ZoomAction, Hint::Text));
    G.Link(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), OpticalSelf(G)), G.Pin(ZoomAction, Hint::Enabled));
    G.Default(ZoomAction, Hint::Type, Hint::Central);
    Array->AddInputPin();
    UEdGraphPin* LastElement = nullptr;
    for (auto* Pin : Array->Pins) if (Pin->Direction == EGPD_Input) LastElement = Pin;
    for (auto* Pin : ZoomAction->Pins) if (Pin->Direction == EGPD_Output) G.Link(Pin, LastElement);
    auto* FireAction = NewObject<UK2Node_MakeStruct>(Graph); FireAction->StructType = FPlayerInputInterfaceAction::StaticStruct();
    FireAction->bMadeAfterOverridePinRemoval = true; G.Node(FireAction);
    auto* FireInput = LoadObject<UInputAction>(nullptr, RailgunInputNames::Fire); check(FireInput);
    G.Pin(FireAction, Hint::InputAction)->DefaultObject = FireInput;
    G.Default(FireAction, Hint::Name, Hint::FireName); G.Default(FireAction, Hint::Category, Hint::ActionCategory);
    G.Link(ReadActionDescription(FireInput), G.Pin(FireAction, Hint::Text));
    G.Link(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), OpticalSelf(G)), G.Pin(FireAction, Hint::Enabled));
    G.Default(FireAction, Hint::Type, Hint::Central);
    Array->AddInputPin();
    for (auto* Pin : Array->Pins) if (Pin->Direction == EGPD_Input) LastElement = Pin;
    for (auto* Pin : FireAction->Pins) if (Pin->Direction == EGPD_Output) G.Link(Pin, LastElement);
    auto* ReloadAction = NewObject<UK2Node_MakeStruct>(Graph); ReloadAction->StructType = FPlayerInputInterfaceAction::StaticStruct();
    ReloadAction->bMadeAfterOverridePinRemoval = true; G.Node(ReloadAction);
    auto* ReloadInput = LoadObject<UInputAction>(nullptr, RailgunInputNames::Reload); check(ReloadInput);
    G.Pin(ReloadAction, Hint::InputAction)->DefaultObject = ReloadInput;
    G.Default(ReloadAction, Hint::Name, Hint::ReloadName); G.Default(ReloadAction, Hint::Category, Hint::ActionCategory);
    G.Link(ReadActionDescription(ReloadInput), G.Pin(ReloadAction, Hint::Text));
    G.Link(G.Read(Reload::Available), G.Pin(ReloadAction, Hint::Enabled));
    G.Default(ReloadAction, Hint::Type, Hint::Central);
    Array->AddInputPin();
    for (auto* Pin : Array->Pins) if (Pin->Direction == EGPD_Input) LastElement = Pin;
    for (auto* Pin : ReloadAction->Pins) if (Pin->Direction == EGPD_Output) G.Link(Pin, LastElement);
    G.Link(Array->GetOutputPin(), G.Pin(Result, P::ReturnValue));
}

}
