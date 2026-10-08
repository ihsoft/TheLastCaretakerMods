#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
namespace RailgunChargeIndicator
{
inline const FName Owner(TEXT("RailgunChargeIndicatorOwner"));
inline const FName Component(TEXT("RailgunChargeIndicatorComponent"));
inline const FName Material(TEXT("RailgunChargeIndicatorMaterial"));
inline const FName LastLevel(TEXT("RailgunChargeIndicatorLastLevel"));
inline const FName BoundModule(TEXT("RailgunChargeIndicatorModule"));
inline const FName BindFunction(TEXT("BindRailgunChargeIndicator"));
inline const FName RefreshFunction(TEXT("RefreshRailgunChargeIndicator"));
inline const FName CallbackFunction(
    TEXT("OnRailgunChargeIndicatorModuleValueChanged"));
inline const FName ElementIndexPin(TEXT("ElementIndex"));
inline const FName OptionalNamePin(TEXT("OptionalName"));
inline const FName SourceMaterialPin(TEXT("SourceMaterial"));
inline const FName ParameterNamePin(TEXT("ParameterName"));
inline const FName ValuePin(TEXT("Value"));
inline const FName DoubleInputPin(TEXT("InDouble"));
inline const FName DynamicMaterialName(TEXT("RailgunChargeIndicatorMID"));
inline constexpr TCHAR FullLevel[] = TEXT("1.0");
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void SetRailgunChargeIndicatorLevel(FGraph& G, UEdGraphPin* Level);

void EnsureRailgunChargeIndicator(FGraph& G);

void UpdateRailgunChargeIndicator(FGraph& G, UEdGraphPin* StoredEnergy);

UEdGraphPin* ReadRailgunChargeIndicatorEnergy(FGraph& G,
    UEdGraphPin* Module);

void AddRailgunChargeIndicatorFunctions(UBlueprint* BP);

void AddRailgunChargeIndicatorTeardown(UBlueprint* BP);
}
