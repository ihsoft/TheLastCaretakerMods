#include "GraphCallHelpers.h"

#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
UEdGraphPin* ObserveCall(FGraph& G, UClass* Owner, FName Function, UEdGraphPin* Target)
{
    auto* Call = G.Call(Owner, Function);
    G.Link(Target, G.Pin(Call, P::FunctionTarget));
    if (Call->FindPin(P::Execute)) G.Exec(Call);
    return G.Pin(Call, P::ReturnValue);
}
}
