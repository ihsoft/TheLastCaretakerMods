#pragma once

// Builds optical view switching, reticle, and release graphs.
namespace OpticalProbeNames
{
inline const FName Camera(TEXT("StationOpticalCamera"));
inline const FName Active(TEXT("OwnsOpticalView"));
inline const FName PreviousView(TEXT("PreviousOpticalViewTarget"));
inline const FName BaselineFov(TEXT("BaselineOpticalFOV"));
inline const FName RequestedFov(TEXT("RequestedOpticalFOV"));
inline const FName CharacterHidden(TEXT("CharacterHiddenBeforeOptics"));
inline const FName Reticle(TEXT("OpticalReticle"));
inline constexpr TCHAR Half[] = TEXT("0.5");
inline constexpr TCHAR Twice[] = TEXT("2.0");
inline constexpr TCHAR Magnification[] = TEXT("5.0");
inline constexpr TCHAR MinimumFov[] = TEXT("10.0");
inline constexpr TCHAR MaximumFov[] = TEXT("150.0");
inline constexpr TCHAR Visible[] = TEXT("HitTestInvisible");
inline constexpr TCHAR Collapsed[] = TEXT("Collapsed");
}
namespace O = OpticalProbeNames;
namespace OP = OpticalCameraGraphNames;
void ReleaseStation(FGraph& G);

UEdGraphPin* OpticalSelf(FGraph& G)
{
    auto* Node = G.Node(NewObject<UK2Node_Self>(G.Graph)); return G.Pin(Node, P::FunctionTarget);
}
#include "StationAimNames.h"
#include "StationRangeGraph.h"
#include "StationHudNames.h"
#include "StationControlGraph.h"

UEdGraphPin* OpticalView(FGraph& G)
{
    return ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, GetViewTarget), G.Read(S::Controller));
}
void OpticalSwitchView(FGraph& G, UEdGraphPin* Target)
{
    auto* Call = G.Call(APlayerController::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerController, SetViewTargetWithBlend));
    G.Link(G.Read(S::Controller), G.Pin(Call, P::FunctionTarget));
    G.Link(Target, G.Pin(Call, OP::NewViewTarget)); G.Default(Call, OP::BlendTime, N::Zero); G.Exec(Call);
}

void OpticalReticle(FGraph& G, bool Visible)
{
    auto SetWidget = [&](FName Name, bool Show)
    {
    auto* Widget = NewObject<UK2Node_VariableGet>(G.Graph);
    Widget->VariableReference.SetExternalMember(Name, G.HudClass); G.Node(Widget);
    G.Link(G.Read(N::HudInstance), G.Pin(Widget, P::FunctionTarget));
    auto* Call = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
    G.Link(G.Pin(Widget, Name), G.Pin(Call, P::FunctionTarget));
    G.Default(Call, OP::Visibility, Show ? O::Visible : O::Collapsed); G.Exec(Call);
    };
    SetWidget(O::Reticle, Visible); SetWidget(H::ScopePanel, Visible);
}

void ReleaseOptics(FGraph& G)
{
    auto* Active = G.Branch(G.Read(O::Active)); G.Write(O::Active, nullptr, N::False);
    auto* ControllerValid = G.Branch(G.Valid(G.Read(S::Controller)));
    auto* Ours = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), OpticalView(G), OpticalSelf(G)));
    auto* PreviousValid = G.Branch(G.Valid(G.Read(O::PreviousView)));
    OpticalSwitchView(G, G.Read(O::PreviousView)); auto* Restored = G.Tail;
    G.Tail = G.Pin(PreviousValid, P::Else);
    auto* Pawn = ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn), G.Read(S::Controller));
    auto* PawnValid = G.Branch(G.Valid(Pawn)); OpticalSwitchView(G, Pawn);
    StationMerge(G, {G.Tail, G.Pin(PawnValid, P::Else), Restored, G.Pin(Ours, P::Else), G.Pin(ControllerValid, P::Else)});
    auto* CharacterValid = G.Branch(G.Valid(G.Read(N::OriginalPawn)));
    auto* Hidden = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorHiddenInGame));
    G.Link(G.Read(N::OriginalPawn), G.Pin(Hidden, P::FunctionTarget)); G.Link(G.Read(O::CharacterHidden), G.Pin(Hidden, OP::Hidden)); G.Exec(Hidden);
    StationMerge(G, {G.Tail, G.Pin(CharacterValid, P::Else)});
    auto* Detach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DetachFromActor));
    G.Link(OpticalSelf(G), G.Pin(Detach, P::FunctionTarget));
    G.Default(Detach, E::LocationRule, N::KeepWorld); G.Default(Detach, E::RotationRule, N::KeepWorld); G.Default(Detach, E::ScaleRule, N::KeepWorld); G.Exec(Detach);
    auto* HudValid = G.Branch(G.Valid(G.Read(N::HudInstance))); OpticalReticle(G, false);
    G.Text(Range::TargetName, N::EmptyText);
    G.Text(Range::TargetRange, N::EmptyText);
    StationMerge(G, {G.Tail, G.Pin(HudValid, P::Else), G.Pin(Active, P::Else)});
}
