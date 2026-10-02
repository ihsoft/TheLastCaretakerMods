#pragma once

// Included after FGraph. Engine-only observation: NEVER writes player/boat state.
UEdGraphPin* ObserveCall(FGraph& G, UClass* Owner, FName Function, UEdGraphPin* Target)
{
    auto* Call = G.Call(Owner, Function);
    G.Link(Target, G.Pin(Call, P::FunctionTarget));
    if (Call->FindPin(P::Execute)) G.Exec(Call);
    return G.Pin(Call, P::ReturnValue);
}
