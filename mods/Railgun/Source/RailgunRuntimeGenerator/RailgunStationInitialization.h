#pragma once
#include "AssetLoadingGraphNames.h"
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Shared array loop used by the generated Railgun graphs.
UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void PrepareRailgunStation(FGraph& G, UClass* StationClass, UEdGraphPin* Shell);

void AddRailgunStationInitializationFunction(UBlueprint* BP,
    UClass* StationClass);

bool ConfigureRailgunStationInitialization(UClass* StationClass);
}
