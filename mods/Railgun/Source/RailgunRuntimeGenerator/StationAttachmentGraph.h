#pragma once

// Builds station attachment, movement/collision ownership, and release graphs.
namespace StationProbeNames
{
inline const FName Held(TEXT("OwnsStationFixation"));
inline const FName Controller(TEXT("StationController"));
inline const FName Movement(TEXT("StationMovement"));
inline const FName Root(TEXT("StationCharacterRoot"));
inline const FName Anchor(TEXT("StationAnchor"));
inline const FName CollisionBefore(TEXT("StationCollisionBefore"));
inline const FName RotationBefore(TEXT("StationRotationBefore"));
inline const FName EntryLocal(TEXT("StationEntryLocal"));
inline const FName AnchorStart(TEXT("StationAnchorStart"));
inline constexpr TCHAR ClassPath[] = TEXT("/Game/Mods/Railgun/Module/BP_Module_Railgun.BP_Module_Railgun_C");
inline constexpr TCHAR RootName[] = TEXT("ModuleMountCollision");
inline constexpr TCHAR Range[] = TEXT("300.0");
inline constexpr TCHAR Walking[] = TEXT("MOVE_Walking");
inline constexpr TCHAR WalkingByte[] = TEXT("1");
}
namespace S = StationProbeNames;
namespace SP = CharacterStationGraphNames;

UEdGraphPin* StationMode(FGraph& G)
{
    auto* Node = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName Name = GET_MEMBER_NAME_CHECKED(UCharacterMovementComponent, MovementMode);
    Node->VariableReference.SetExternalMember(Name, UCharacterMovementComponent::StaticClass()); G.Node(Node);
    G.Link(G.Read(S::Movement), G.Pin(Node, P::FunctionTarget)); return G.Pin(Node, Name);
}

UEdGraphPin* StationParent(FGraph& G)
{
    return ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), G.Read(S::Root));
}

void StationSetMode(FGraph& G, const TCHAR* Mode)
{
    auto* Call = G.Call(UCharacterMovementComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCharacterMovementComponent, SetMovementMode));
    G.Link(G.Read(S::Movement), G.Pin(Call, P::FunctionTarget)); G.Default(Call, SP::NewMovementMode, Mode); G.Exec(Call);
}

void StationCollision(FGraph& G, bool Restore)
{
    auto* Call = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Call, P::FunctionTarget));
    if (Restore) G.Link(G.Read(S::CollisionBefore), G.Pin(Call, SP::CollisionEnabled));
    else G.Default(Call, SP::CollisionEnabled, N::False);
    G.Exec(Call);
}

void StationMerge(FGraph& G, const TArray<UEdGraphPin*>& Paths)
{
    auto* Join = G.Node(NewObject<UK2Node_IfThenElse>(G.Graph)); G.Default(Join, P::Condition, N::True);
    for (UEdGraphPin* Path : Paths) G.Link(Path, G.Pin(Join, P::Execute));
    G.Tail = G.Pin(Join, P::Then);
}

#include "StationOpticsGraph.h"

void ReleaseStation(FGraph& G)
{
    ReleaseStationControl(G);
    ReleaseOptics(G);
    auto* Held = G.Branch(G.Read(S::Held)); G.Write(S::Held, nullptr, N::False);
    auto* CharacterValid = G.Branch(G.Valid(G.Read(N::OriginalPawn)));
    auto* RootValid = G.Branch(G.Valid(G.Read(S::Root)));
    auto* Ours = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), StationParent(G), G.Read(S::Anchor)));
    auto* Detach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DetachFromActor));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Detach, P::FunctionTarget));
    G.Default(Detach, E::LocationRule, N::KeepWorld); G.Default(Detach, E::RotationRule, N::KeepWorld);
    G.Default(Detach, E::ScaleRule, N::KeepWorld); G.Exec(Detach);
    StationMerge(G, {G.Tail, G.Pin(Ours, P::Else)});
    // Do not detach a replacement owner's parent or force its movement mode.
    auto* Unparented = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Valid(StationParent(G)), N::False));
    auto* CurrentRotation = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorRotation), G.Read(N::OriginalPawn));
    auto* CurrentParts = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakRotator));
    G.Link(CurrentRotation, G.Pin(CurrentParts, SP::RotationValue));
    auto* OldParts = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakRotator));
    G.Link(G.Read(S::RotationBefore), G.Pin(OldParts, SP::RotationValue));
    auto* Rotation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
    G.Link(G.Pin(CurrentParts, SP::Yaw), G.Pin(Rotation, SP::Yaw));
    G.Link(G.Pin(OldParts, SP::Pitch), G.Pin(Rotation, SP::Pitch)); G.Link(G.Pin(OldParts, SP::Roll), G.Pin(Rotation, SP::Roll));
    auto* SetRotation = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorRotation));
    G.Link(G.Read(N::OriginalPawn), G.Pin(SetRotation, P::FunctionTarget));
    G.Link(G.Pin(Rotation, P::ReturnValue), G.Pin(SetRotation, SP::NewRotation)); G.Default(SetRotation, SP::TeleportPhysics, N::False); G.Exec(SetRotation);
    auto* FreeTail = G.Tail;
    // Restore our collision flag on both paths. External takeover is unsupported;
    // do not silently leave a character non-colliding after relinquishing our gate.
    G.Tail = G.Pin(Unparented, P::Else); StationCollision(G, true); auto* ExternalTail = G.Tail;
    G.Tail = FreeTail; StationCollision(G, true);
    auto* MovementValid = G.Branch(G.Valid(G.Read(S::Movement)));
    auto* StillNone = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), StationMode(G), N::Zero));
    StationSetMode(G, S::Walking); // Entry admits Walking only; native floor reacquisition.
    StationMerge(G, {G.Tail, G.Pin(StillNone, P::Else), G.Pin(MovementValid, P::Else), ExternalTail,
        G.Pin(RootValid, P::Else), G.Pin(CharacterValid, P::Else), G.Pin(Held, P::Else)});
}
