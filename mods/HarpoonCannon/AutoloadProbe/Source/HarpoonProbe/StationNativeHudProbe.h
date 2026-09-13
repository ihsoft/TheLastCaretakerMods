#pragma once

// HC18 candidate02: prove native callback dispatch only. No native widget instance.
void AddNativeStationHudInterface(UBlueprint* BP, UClass* ReturnedHud = nullptr)
{
    UClass* Interface = UVoyageActorWidgetInterface::StaticClass();
    const FName FunctionName = GET_FUNCTION_NAME_CHECKED(IVoyageActorWidgetInterface, GetHUDOverrideWidget);
    check(Interface->FindFunctionByName(FunctionName)->GetOuterUClass() == Interface);
    check(FBlueprintEditorUtils::ImplementNewInterface(BP, Interface->GetClassPathName()));
    UEdGraph* Graph = nullptr;
    for (const auto& Description : BP->ImplementedInterfaces)
        if (Description.Interface == Interface)
            for (UEdGraph* Candidate : Description.Graphs)
                if (Candidate->GetFName() == FunctionName) Graph = Candidate;
    check(Graph);
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
        if (auto* Candidate = Cast<UK2Node_FunctionResult>(Node)) Result = Candidate;
    }
    check(Entry && Result);
    FGraph G(Graph, nullptr);
    G.Pin(Entry, P::Then)->BreakAllPinLinks(); G.Pin(Result, P::Execute)->BreakAllPinLinks();
    G.Tail = G.Pin(Entry, P::Then); G.Write(N::NativeHudRequested, nullptr, N::True);
    G.Link(G.Tail, G.Pin(Result, P::Execute));
    G.Pin(Result, P::ReturnValue)->DefaultObject = ReturnedHud;
}
