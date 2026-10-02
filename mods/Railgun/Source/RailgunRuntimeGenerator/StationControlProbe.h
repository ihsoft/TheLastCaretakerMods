#pragma once

// HC17: the autoload optical object is an Engine Pawn, not a physical vehicle.
namespace StationControlNames
{
inline const FName Owned(TEXT("OwnsStationPossession"));
inline const FName ReturnPending(TEXT("StationReturnNeedsRetry"));
inline constexpr TCHAR ReturnFailed[] = TEXT("HC17 RETURN FAILED: F8 retries; do not save. Report this.");
}
namespace Control = StationControlNames;
void OpticalReticle(FGraph& G, bool Visible);

UEdGraphPin* StationControlledPawn(FGraph& G)
{
    return ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn), G.Read(S::Controller));
}

void StationPossess(FGraph& G, UEdGraphPin* Pawn)
{
    auto* Call = G.Call(AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, Possess));
    G.Link(G.Read(S::Controller), G.Pin(Call, P::FunctionTarget));
    G.Link(Pawn, G.Pin(Call, SP::PossessionTarget)); G.Exec(Call);
}

void ReleaseStationControl(FGraph& G)
{
    auto* Owned = G.Branch(G.Read(Control::Owned));
    auto* ControllerValid = G.Branch(G.Valid(G.Read(S::Controller)));
    auto* CanReturn = G.Branch(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), StationControlledPawn(G), OpticalSelf(G)),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Valid(StationControlledPawn(G)), N::False)));
    // Stop this execution path on return failure: retain fixation and the proxy.
    // A latched failure allows only a deliberate F8 retry, not repeated Possess.
    G.Write(Control::ReturnPending, nullptr, N::True);
    OpticalReticle(G, false); G.Text(N::FreezeStatus, Control::ReturnFailed);
    G.Require(G.Valid(G.Read(N::OriginalPawn)), Control::ReturnFailed);
    auto* OriginalController = ObserveCall(G, APawn::StaticClass(), BlueprintGraphNames::ActorFunctions::GetController, G.Read(N::OriginalPawn));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Valid(OriginalController), N::False),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), OriginalController, G.Read(S::Controller))), Control::ReturnFailed);
    StationPossess(G, G.Read(N::OriginalPawn));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), StationControlledPawn(G), G.Read(N::OriginalPawn)), Control::ReturnFailed);
    StationMerge(G, {G.Tail, G.Pin(CanReturn, P::Else), G.Pin(ControllerValid, P::Else)});
    G.Write(Control::Owned, nullptr, N::False); G.Write(Control::ReturnPending, nullptr, N::False);
    StationMerge(G, {G.Tail, G.Pin(Owned, P::Else)});
}
