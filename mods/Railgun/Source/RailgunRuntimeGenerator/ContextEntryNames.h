#pragma once
#include "CoreMinimal.h"
namespace ContextEntryNames
{
inline const FName Ready(TEXT("RailgunEntryReady"));
inline const FName ModelEntry(TEXT("RailgunModelEntryReference"));
inline const FName BoxExtentPin(TEXT("InBoxExtent"));
inline const FName InteractBlocks(TEXT("RailgunEntryInteractBlocks"));
inline const FName CollisionChannelPin(TEXT("Channel"));
inline constexpr TCHAR InteractChannelValue[] = TEXT("ECC_GameTraceChannel1");
inline constexpr TCHAR BlockResponseValue[] = TEXT("2");
inline const FName ProviderSeen(TEXT("RailgunEntryProviderSeen"));
inline const FName CallbackSeen(TEXT("RailgunEntryCallbackSeen"));
inline const FName QueryBox(TEXT("RailgunEntryQuery"));
inline const FName Interaction(TEXT("RailgunInteraction"));
inline const FName EntryAction(TEXT("RailgunEntryAction"));
inline const FName EnterFunction(TEXT("RailgunEnterFromAction"));
inline const FName PrepareFunction(TEXT("RailgunPrepareStation"));
inline const FName FoundPair(TEXT("RailgunFoundPair"));
inline const FName ShellClass(TEXT("RailgunShellClass"));
inline const FName Shell(TEXT("RailgunShell"));
inline const FName Component(TEXT("Component"));
inline const FName MyCharacter(TEXT("MyCharacter"));
inline const FName OutActions(TEXT("OutActions"));
inline const FName ActionInstance(TEXT("InputAction"));
inline const FName CallbackText(TEXT("Text"));
inline const FName Controller(TEXT("Controller"));
inline const FName OnTriggered(TEXT("OnTriggered"));
inline const FName Owner(TEXT("Owner"));
inline const FName Array(TEXT("Array"));
inline const FName ArrayElement(TEXT("Array Element"));
inline const FName LoopBody(TEXT("LoopBody"));
inline const FName Completed(TEXT("Completed"));
inline const FName OutActors(TEXT("OutActors"));
inline const FName ClassAsset(TEXT("Asset"));
inline const FName SoftClassPath(TEXT("PathString"));
inline const FName SoftClassReferencePath(TEXT("SoftClassPath"));
inline constexpr TCHAR One[] = TEXT("1.0");
inline const FName ChildActors(TEXT("OutActors"));
inline const FName Recurse(TEXT("bRecursivelyIncludeAttachedActors"));
inline const FName Spawned(TEXT("RailgunPreparedCount"));
inline const FName WasOccupied(TEXT("RailgunWasOccupied"));
// Secondary character interaction (default F), as used for drone entry.
// Keep primary interaction (default E) available for the electrical socket.
inline constexpr TCHAR InteractActionPath[] = TEXT("/Game/Game/Input/Character/IAV_InteractTwo.IAV_InteractTwo");
inline constexpr TCHAR ActionName[] = TEXT("RailgunEnter");
inline constexpr TCHAR Label[] = TEXT("Enter Railgun");
inline constexpr TCHAR LoopPackage[] = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");
inline const FName LoopGraph(TEXT("ForEachLoop"));
inline constexpr TCHAR PreparedStatus[] = TEXT("Enter; check breech sight, E exit. Defaults: mouse40%, yaw +/-80. No saving.");
inline constexpr TCHAR CountLabel[] = TEXT("Prepared stations: ");
inline constexpr TCHAR ProviderYes[] = TEXT("Entry provider observed (ANY, sticky): YES");
inline constexpr TCHAR ProviderNo[] = TEXT("Entry provider observed (ANY, sticky): NO");
inline constexpr TCHAR CallbackYes[] = TEXT("Entry callback observed (ANY, sticky): YES");
inline constexpr TCHAR CallbackNo[] = TEXT("Entry callback observed (ANY, sticky): NO");
inline constexpr float ScanInterval = 1.0f;
// Runtime geometry is copied from the shell's manifest-authored entry reference.
inline const FVector BoxExtent(1.0f, 1.0f, 1.0f);
inline const FVector BoxOffset = FVector::ZeroVector;
}
