#pragma once

// Included after native field-reading helpers. HC24 changes the owned station's
// camera only; never possesses the observer or mutates the character's camera.
namespace NativeCameraNames
{
inline const FName Camera(TEXT("NativeStationCamera"));
inline const FName CameraField = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, CameraComponent);
inline const FName AllowField = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, bAllowFirstPersonCamera);
inline const FName ModeField = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, bFirstPersonCamera);
inline const FName ModeText(TEXT("NativeFirstPersonModeText"));
inline const FName RequestedText(TEXT("NativeRequestedFovText"));
inline const FVector2D PanelSize(730.0f, 290.0f);
inline constexpr TCHAR Refused[] = TEXT("HC24 CAMERA STOP: missing camera/flag/FOV prerequisite. Entry blocked.");
inline constexpr TCHAR ModeYes[] = TEXT("Station first-person flag: YES (verify viewpoint visually)");
inline constexpr TCHAR ModeNo[] = TEXT("Station first-person flag: NO (game changed requested mode)");
}

void SetNativeCameraFlag(FGraph& G, FName Field)
{
    auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
    Set->VariableReference.SetExternalMember(Field, AVoyageVehiclePawn::StaticClass()); G.Node(Set);
    G.Link(G.Read(V::Vehicle), G.Pin(Set, P::FunctionTarget));
    G.Default(Set, Field, N::True); G.Exec(Set);
}

void CalculateNativeOpticalFov(FGraph& G)
{
    auto* Manager = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Require(G.Valid(G.Pin(Manager, P::ReturnValue)), NativeCameraNames::Refused);
    G.Write(O::BaselineFov, ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetFOVAngle), G.Pin(Manager, P::ReturnValue)));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_DoubleDouble), G.Read(O::BaselineFov), O::MinimumFov),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble), G.Read(O::BaselineFov), O::MaximumFov)), NativeCameraNames::Refused);
    // Angular magnification: 2*atan(tan(on-foot FOV/2)/5), not FOV/5.
    auto* Half = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
    G.Link(G.Read(O::BaselineFov), G.Pin(Half, P::Binary::LeftOperand)); G.Default(Half, P::Binary::RightOperand, O::Half);
    auto* Tangent = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, DegTan));
    G.Link(G.Pin(Half, P::ReturnValue), G.Pin(Tangent, OP::AngleValue));
    auto* Divide = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(G.Pin(Tangent, P::ReturnValue), G.Pin(Divide, P::Binary::LeftOperand)); G.Default(Divide, P::Binary::RightOperand, O::Magnification);
    auto* Atan = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, DegAtan));
    G.Link(G.Pin(Divide, P::ReturnValue), G.Pin(Atan, OP::AngleValue));
    auto* Twice = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_DoubleDouble));
    G.Link(G.Pin(Atan, P::ReturnValue), G.Pin(Twice, P::Binary::LeftOperand)); G.Default(Twice, P::Binary::RightOperand, O::Twice);
    G.Write(O::RequestedFov, G.Pin(Twice, P::ReturnValue));
}

void CheckNativeOpticalCamera(FGraph& G)
{
    auto* Camera = ReadNativeInputField(G, G.Read(V::Vehicle), AVoyageVehiclePawn::StaticClass(), NativeCameraNames::CameraField);
    G.Require(G.Valid(Camera), NativeCameraNames::Refused);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Camera, G.Read(NativeCameraNames::Camera)), NativeCameraNames::Refused);
    G.Require(ReadNativeInputField(G, G.Read(V::Vehicle), AVoyageVehiclePawn::StaticClass(), NativeCameraNames::AllowField), NativeCameraNames::Refused);
    // Mode is also observed after entry; do not continually overwrite native state.
}

void ConfigureNativeOpticalCamera(FGraph& G)
{
    auto* Camera = ReadNativeInputField(G, G.Read(V::Vehicle), AVoyageVehiclePawn::StaticClass(), NativeCameraNames::CameraField);
    G.Require(G.Valid(Camera), NativeCameraNames::Refused); G.Write(NativeCameraNames::Camera, Camera);
    SetNativeCameraFlag(G, NativeCameraNames::AllowField);
    SetNativeCameraFlag(G, NativeCameraNames::ModeField);
    auto* Fov = G.Call(UCameraComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UCameraComponent, SetFieldOfView));
    G.Link(G.Read(NativeCameraNames::Camera), G.Pin(Fov, P::FunctionTarget)); G.Link(G.Read(O::RequestedFov), G.Pin(Fov, OP::FieldOfView)); G.Exec(Fov);
    CheckNativeOpticalCamera(G);
}

void ObserveNativeOpticalCamera(FGraph& G)
{
    G.BooleanText(NativeCameraNames::ModeText,
        ReadNativeInputField(G, G.Read(V::Vehicle), AVoyageVehiclePawn::StaticClass(), NativeCameraNames::ModeField), NativeCameraNames::ModeYes, NativeCameraNames::ModeNo);
    G.Number(NativeCameraNames::RequestedText, O::Requested, G.Read(O::RequestedFov));
    auto* Manager = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    auto* HasManager = G.Branch(G.Valid(G.Pin(Manager, P::ReturnValue)));
    G.Number(N::Current, O::Actual, ObserveCall(G, APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetFOVAngle), G.Pin(Manager, P::ReturnValue)));
    StationMerge(G, {G.Tail, G.Pin(HasManager, P::Else)}); // Diagnostics never block exit.
}
