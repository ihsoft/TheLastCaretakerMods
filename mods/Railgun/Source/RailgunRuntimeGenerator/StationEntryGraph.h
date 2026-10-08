#pragma once
#include "ContextEntryNames.h"
#include "DedicatedStationNames.h"
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
// Builds station entry callbacks and the interaction interface graph.
namespace CE = ContextEntryNames;
namespace DS = DedicatedStationNames;
namespace StationLifecycle
{
inline const FName ShellOwner(TEXT("RailgunShellOwner"));
inline const FName EntryPending(TEXT("RailgunEntryPending"));
inline const FName ExitPending(TEXT("RailgunExitPending"));
inline const FName TeardownPending(TEXT("RailgunTeardownPending"));
inline const FName ActiveHud(TEXT("RailgunActiveHud"));
inline const FName BindShell(TEXT("BindRailgunShellLifecycle"));
inline const FName ShellEndPlayCallback(TEXT("OnRailgunShellEndPlay"));
inline const FName FinalizeTeardown(TEXT("FinalizeRailgunStationTeardown"));
inline constexpr TCHAR DestroyedReason[] = TEXT("0");
inline constexpr TCHAR RemovedFromWorldReason[] = TEXT("3");
}

void ContextSet(FGraph& G, UEdGraphPin* Target, UClass* Class, FName Field, UEdGraphPin* Value, const TCHAR* Literal = nullptr);

UEdGraphPin* ContextParentValid(FGraph& G);

struct FContextEntryEligibility
{
    UEdGraphPin* Character;
    UEdGraphPin* Movement;
};

FContextEntryEligibility RequireContextEntryEligibility(FGraph& G,
    UEdGraphPin* Pawn, UEdGraphPin* RejectedResult = nullptr,
    UEdGraphPin* InteractionComponent = nullptr);

void AddContextEntry(UBlueprint* BP);

FMulticastDelegateProperty* StationShellEndPlayDelegateProperty();

void RestoreDedicatedViewIfOwned(FGraph& G);

void AddStationLifecycleFunctions(UBlueprint* BP);

void AddStationLifecycleTeardown(UBlueprint* BP);
}
