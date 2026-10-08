#pragma once

#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Invokes reflected Engine getters without writing player or boat state.
UEdGraphPin* ObserveCall(FGraph& G, UClass* Owner, FName Function, UEdGraphPin* Target);
}
