#pragma once
#include "AssetLoadingGraphNames.h"

namespace NativeVehicleNames
{
inline const FName Vehicle(TEXT("NativeStation"));
inline const FName Body(TEXT("NativeStationBody"));
inline const FName Occupied(TEXT("NativeEntryObserved"));
inline const FName Attempted(TEXT("NativeSpawnAttempted"));
inline const FName ExitSent(TEXT("NativeExitRequested"));
inline const FName PreparedFlag(TEXT("NativeStationPrepared"));
inline const FName NewPossessor(TEXT("NewPossessor"));
inline constexpr TCHAR Title[] = TEXT("HC19 NATIVE VEHICLE BASE / NO DRIVING / NO OPTICS");
inline constexpr TCHAR Ready[] = TEXT("HC19: aim at railgun base; F8 prepares station, next F8 enters");
inline constexpr TCHAR Prepared[] = TEXT("HC19 PREPARED: F8 enters native vehicle. Do not save.");
inline constexpr TCHAR Active[] = TEXT("HC19 INSIDE: F8 native exit (automatic attempt after20s). Do not save.");
inline constexpr TCHAR Returned[] = TEXT("HC19 RETURNED: check walking/actions; F8 repeats entry");
inline constexpr TCHAR Failed[] = TEXT("HC19 STOP: prerequisite/entry failed. Screenshot, quit without saving.");
inline constexpr TCHAR ExitFailed[] = TEXT("HC19 EXIT REQUESTED: if stuck, F8 retries. Do not save.");
inline constexpr TCHAR PhysicsYes[] = TEXT("Vehicle root simulates: YES - STOP TEST");
inline constexpr TCHAR PhysicsNo[] = TEXT("Vehicle root simulates: NO");
inline constexpr TCHAR ControlYes[] = TEXT("Controller pawn = native station: YES");
inline constexpr TCHAR ControlNo[] = TEXT("Controller pawn = native station: NO");
inline constexpr TCHAR AttachedYes[] = TEXT("Vehicle attached to railgun: YES");
inline constexpr TCHAR AttachedNo[] = TEXT("Vehicle attached to railgun: NO - STOP TEST");
inline constexpr TCHAR PlayerOnly[] = TEXT("Local player walking near a built railgun required");
inline constexpr TCHAR StationOffset[] = TEXT("X=0,Y=0,Z=140");
inline constexpr TCHAR Seconds[] = TEXT("Native control seconds (max20): ");
}
namespace V = NativeVehicleNames;

namespace CombinedVehicleNames
{
inline constexpr TCHAR Title[] = TEXT("HC24 NATIVE FIRST-PERSON REQUEST / x5 / HC23 CONTROLS");
inline constexpr TCHAR Ready[] = TEXT("HC24: aim at railgun base; F8 prepares, next F8 enters");
inline constexpr TCHAR Prepared[] = TEXT("HC24 PREPARED: camera/input references checked. F8 enters. Do not save.");
inline constexpr TCHAR Active[] = TEXT("HC24: check first-person/x5/mouse; E exits, F8 fallback (max20s).");
inline constexpr TCHAR Offset[] = TEXT("X=-170,Y=0,Z=140");
inline constexpr TCHAR OperatorTag[] = TEXT("HC21OperatorPoint");
inline const FName ExitTag = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, ExitComponentTag);
inline const FName ComponentTags = GET_MEMBER_NAME_CHECKED(UActorComponent, ComponentTags);
}

namespace ExitActionNames
{
inline const FName Expected(TEXT("ExpectedStockExitAction"));
inline const FName Field = GET_MEMBER_NAME_CHECKED(AVoyageVehicleForkliftPawn, ExitAction);
inline constexpr TCHAR Path[] = TEXT("/Game/Game/Input/Vehicle/IAV_VehicleExit.IAV_VehicleExit");
inline constexpr TCHAR Failed[] = TEXT("HC23 STOP: stock input reference load/type/readback failed. Entry blocked; screenshot and quit.");
}

UEdGraphPin* RequireExitActionCast(FGraph& G, UEdGraphPin* Object, UClass* Class)
{
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = Class; Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(Object, Cast->GetCastSourcePin());
    G.Tail = Cast->GetInvalidCastPin(); G.Text(N::FreezeStatus, ExitActionNames::Failed);
    G.Tail = Cast->GetValidCastPin(); return Cast->GetCastResultPin();
}

UEdGraphPin* LoadStockInputReference(FGraph& G, const TCHAR* ObjectPath, UClass* Class)
{
    // Load stock data in preparation, not inside native possession/action collection.
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
    G.Default(Path, E::PathString, ObjectPath);
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Ref, AssetLoadingGraphNames::SoftObjectPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
    G.Link(G.Pin(Ref, P::ReturnValue), G.Pin(Load, AssetLoadingGraphNames::Asset)); G.Exec(Load);
    G.Require(G.Valid(G.Pin(Load, P::ReturnValue)), ExitActionNames::Failed);
    return RequireExitActionCast(G, G.Pin(Load, P::ReturnValue), Class);
}

void LoadStockExitAction(FGraph& G)
{
    G.Write(ExitActionNames::Expected, LoadStockInputReference(G, ExitActionNames::Path, UVoyageInputAction::StaticClass()));
}

void CheckStockExitAction(FGraph& G, bool Assign)
{
    G.Require(G.Valid(G.Read(ExitActionNames::Expected)), ExitActionNames::Failed);
    auto* Target = RequireExitActionCast(G, G.Read(V::Vehicle), AVoyageVehicleForkliftPawn::StaticClass());
    if (Assign)
    {
        auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
        Set->VariableReference.SetExternalMember(ExitActionNames::Field, AVoyageVehicleForkliftPawn::StaticClass());
        G.Node(Set); G.Link(Target, G.Pin(Set, P::FunctionTarget));
        G.Link(G.Read(ExitActionNames::Expected), G.Pin(Set, ExitActionNames::Field)); G.Exec(Set);
    }
    auto* Get = NewObject<UK2Node_VariableGet>(G.Graph);
    Get->VariableReference.SetExternalMember(ExitActionNames::Field, AVoyageVehicleForkliftPawn::StaticClass());
    G.Node(Get); G.Link(Target, G.Pin(Get, P::FunctionTarget));
    auto* Value = G.Pin(Get, ExitActionNames::Field);
    G.Require(G.Valid(Value), ExitActionNames::Failed);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Value, G.Read(ExitActionNames::Expected)), ExitActionNames::Failed);
}

namespace NativeInputNames
{
inline const FName Context(TEXT("ExpectedVehicleInputContext"));
inline const FName LookRight(TEXT("ExpectedVehicleLookRight"));
inline const FName LookUp(TEXT("ExpectedVehicleLookUp"));
inline const FName ControlsField = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, InputControls);
inline const FName ContextField = GET_MEMBER_NAME_CHECKED(UVoyageInputControlsComponent, InputContextAsset);
inline const FName RightField = GET_MEMBER_NAME_CHECKED(AVoyageVehicleForkliftPawn, LookRightInputAction);
inline const FName UpField = GET_MEMBER_NAME_CHECKED(AVoyageVehicleForkliftPawn, LookUpInputAction);
inline constexpr TCHAR ContextPath[] = TEXT("/Game/Game/Input/Vehicle/DA_Input_Context_Forklift.DA_Input_Context_Forklift");
inline constexpr TCHAR RightPath[] = TEXT("/Game/Game/Input/Character/IA_LookRight.IA_LookRight");
inline constexpr TCHAR UpPath[] = TEXT("/Game/Game/Input/Character/IA_LookUp.IA_LookUp");
}

void LoadNativeLookInputs(FGraph& G)
{
    G.Write(NativeInputNames::Context, LoadStockInputReference(G, NativeInputNames::ContextPath, UVoyageInputContextAsset::StaticClass()));
    G.Write(NativeInputNames::LookRight, LoadStockInputReference(G, NativeInputNames::RightPath, UInputAction::StaticClass()));
    G.Write(NativeInputNames::LookUp, LoadStockInputReference(G, NativeInputNames::UpPath, UInputAction::StaticClass()));
}

UEdGraphPin* ReadNativeInputField(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field)
{
    auto* Get = NewObject<UK2Node_VariableGet>(G.Graph);
    Get->VariableReference.SetExternalMember(Field, Owner); G.Node(Get);
    G.Link(Target, G.Pin(Get, P::FunctionTarget)); return G.Pin(Get, Field);
}

void NativeInputFieldGuard(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field, FName Expected, bool Assign)
{
    G.Require(G.Valid(G.Read(Expected)), ExitActionNames::Failed);
    if (Assign)
    {
        auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
        Set->VariableReference.SetExternalMember(Field, Owner); G.Node(Set);
        G.Link(Target, G.Pin(Set, P::FunctionTarget)); G.Link(G.Read(Expected), G.Pin(Set, Field)); G.Exec(Set);
    }
    auto* Value = ReadNativeInputField(G, Target, Owner, Field);
    G.Require(G.Valid(Value), ExitActionNames::Failed);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Value, G.Read(Expected)), ExitActionNames::Failed);
}

void CheckNativeLookInputs(FGraph& G, bool Assign)
{
    auto* Controls = ReadNativeInputField(G, G.Read(V::Vehicle), AVoyageVehiclePawn::StaticClass(), NativeInputNames::ControlsField);
    G.Require(G.Valid(Controls), ExitActionNames::Failed);
    NativeInputFieldGuard(G, Controls, UVoyageInputControlsComponent::StaticClass(), NativeInputNames::ContextField, NativeInputNames::Context, Assign);
    auto* Forklift = RequireExitActionCast(G, G.Read(V::Vehicle), AVoyageVehicleForkliftPawn::StaticClass());
    NativeInputFieldGuard(G, Forklift, AVoyageVehicleForkliftPawn::StaticClass(), NativeInputNames::RightField, NativeInputNames::LookRight, Assign);
    NativeInputFieldGuard(G, Forklift, AVoyageVehicleForkliftPawn::StaticClass(), NativeInputNames::UpField, NativeInputNames::LookUp, Assign);
}

#include "NativeVehicleCameraProbe.h"

void ConfigureCombinedOperatorPoint(FGraph& G)
{
    // This new actor's nonphysical root is the operator point, behind the
    // railgun at character height. Preserve the shell and native entry/exit.
    auto* Tags = NewObject<UK2Node_VariableSet>(G.Graph);
    Tags->VariableReference.SetExternalMember(CombinedVehicleNames::ComponentTags, UActorComponent::StaticClass());
    G.Node(Tags); G.Link(G.Read(V::Body), G.Pin(Tags, P::FunctionTarget));
    auto* Values = G.Node(NewObject<UK2Node_MakeArray>(G.Graph));
    G.Link(Values->GetOutputPin(), G.Pin(Tags, CombinedVehicleNames::ComponentTags));
    G.Default(Values, Values->GetPinName(0), CombinedVehicleNames::OperatorTag); G.Exec(Tags);
    auto* Tag = NewObject<UK2Node_VariableSet>(G.Graph);
    Tag->VariableReference.SetExternalMember(CombinedVehicleNames::ExitTag, AVoyageVehiclePawn::StaticClass());
    G.Node(Tag); G.Link(G.Read(V::Vehicle), G.Pin(Tag, P::FunctionTarget));
    G.Default(Tag, CombinedVehicleNames::ExitTag, CombinedVehicleNames::OperatorTag); G.Exec(Tag);
    auto* HasTag = G.Call(UActorComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UActorComponent, ComponentHasTag));
    G.Link(G.Read(V::Body), G.Pin(HasTag, P::FunctionTarget));
    // Engine tag argument is its own semantic identity, not an opaque operand.
    const FName TagArgument(TEXT("Tag"));
    G.Default(HasTag, TagArgument, CombinedVehicleNames::OperatorTag);
    G.Require(G.Pin(HasTag, P::ReturnValue), V::Failed);
}

UEdGraphPin* NativeControlled(FGraph& G)
{
    return ObserveCall(G, AController::StaticClass(), GET_FUNCTION_NAME_CHECKED(AController, K2_GetPawn), G.Read(S::Controller));
}

void NativeExit(FGraph& G)
{
    // No manual Possess, character transform, movement, collision or HUD repair.
    G.Write(V::ExitSent, nullptr, N::True);
    auto* Call = G.Call(AVoyageVehiclePawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, OnExitVehicle));
    G.Link(G.Read(V::Vehicle), G.Pin(Call, P::FunctionTarget)); G.Exec(Call);
    G.Text(N::FreezeStatus, V::ExitFailed);
}
