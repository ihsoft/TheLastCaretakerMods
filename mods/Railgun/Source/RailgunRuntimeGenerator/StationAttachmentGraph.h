#pragma once

// Builds station attachment, movement/collision ownership, and release graphs.
namespace StationProbeNames
{
inline const FName Anchor(TEXT("StationAnchor"));
inline constexpr TCHAR WalkingByte[] = TEXT("1");
}
namespace S = StationProbeNames;
namespace SP = CharacterStationGraphNames;

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

#include "StationOpticsGraph.h"
