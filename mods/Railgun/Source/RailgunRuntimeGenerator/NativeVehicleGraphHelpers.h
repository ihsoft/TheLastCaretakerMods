#pragma once
#include "AssetLoadingGraphNames.h"
#include "VoyageInputControlsComponent.h"
#include "VoyageVehiclePawn.h"

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Builds native station vehicle setup and input-reference guards.
namespace NativeVehicleNames
{
inline const FName Vehicle(TEXT("NativeStation"));
inline const FName Body(TEXT("NativeStationBody"));
inline const FName NewPossessor(TEXT("NewPossessor"));
}
namespace V = NativeVehicleNames;

namespace CombinedVehicleNames
{
inline constexpr TCHAR Offset[] = TEXT("X=-170,Y=0,Z=140");
inline constexpr TCHAR OperatorTag[] = TEXT("HC21OperatorPoint");
inline const FName ExitTag = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, ExitComponentTag);
inline const FName ComponentTags = GET_MEMBER_NAME_CHECKED(UActorComponent, ComponentTags);
}
UEdGraphPin* RequireExitActionCast(FGraph& G, UEdGraphPin* Object, UClass* Class);
UEdGraphPin* LoadStockInputReference(FGraph& G, const TCHAR* ObjectPath, UClass* Class);

namespace NativeInputNames
{
inline const FName Context(TEXT("ExpectedVehicleInputContext"));
inline const FName ControlsField = GET_MEMBER_NAME_CHECKED(AVoyageVehiclePawn, InputControls);
inline const FName ContextField = GET_MEMBER_NAME_CHECKED(UVoyageInputControlsComponent, InputContextAsset);
}

UEdGraphPin* ReadNativeInputField(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field);

void NativeInputFieldGuard(FGraph& G, UEdGraphPin* Target, UClass* Owner, FName Field, FName Expected, bool Assign);

void ConfigureCombinedOperatorPoint(FGraph& G);
}
