#include "StationOpticsGraph.h"

#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
UEdGraphPin* OpticalSelf(FGraph& G)
{
    auto* Node = G.Node(NewObject<UK2Node_Self>(G.Graph)); return G.Pin(Node, P::FunctionTarget);
}
}
