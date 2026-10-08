#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Builds the Voyage actor-widget interface graph for station HUD dispatch.
void AddNativeStationHudInterface(UBlueprint* BP, UClass* ReturnedHud = nullptr);
}
