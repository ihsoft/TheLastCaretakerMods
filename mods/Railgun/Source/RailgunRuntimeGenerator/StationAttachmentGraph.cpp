#include "StationAttachmentGraph.h"

#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
UEdGraphPin* StationMode(FGraph& G, UEdGraphPin* Movement)
{
    auto* Node = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName Name = GET_MEMBER_NAME_CHECKED(UCharacterMovementComponent, MovementMode);
    Node->VariableReference.SetExternalMember(Name, UCharacterMovementComponent::StaticClass()); G.Node(Node);
    G.Link(Movement, G.Pin(Node, P::FunctionTarget));
    return G.Pin(Node, Name);
}

void StationMerge(FGraph& G, const TArray<UEdGraphPin*>& Paths)
{
    auto* Join = G.Node(NewObject<UK2Node_IfThenElse>(G.Graph)); G.Default(Join, P::Condition, N::True);
    for (UEdGraphPin* Path : Paths) G.Link(Path, G.Pin(Join, P::Execute));
    G.Tail = G.Pin(Join, P::Then);
}
}
