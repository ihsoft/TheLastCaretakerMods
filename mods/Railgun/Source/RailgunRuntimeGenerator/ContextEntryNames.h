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
inline const FName QueryBox(TEXT("RailgunEntryQuery"));
inline const FName Interaction(TEXT("RailgunInteraction"));
inline const FName EntryAction(TEXT("RailgunEntryAction"));
inline const FName InitializeStation(TEXT("InitializeRailgunStation"));
inline const FName EnterFunction(TEXT("RailgunEnterFromAction"));
inline const FName Component(TEXT("Component"));
inline const FName MyCharacter(TEXT("MyCharacter"));
inline const FName OutActions(TEXT("OutActions"));
inline const FName ActionInstance(TEXT("InputAction"));
inline const FName Controller(TEXT("Controller"));
inline const FName OnTriggered(TEXT("OnTriggered"));
inline const FName Owner(TEXT("Owner"));
inline const FName Array(TEXT("Array"));
inline const FName ArrayElement(TEXT("Array Element"));
inline const FName LoopBody(TEXT("LoopBody"));
inline const FName Completed(TEXT("Completed"));
inline constexpr TCHAR One[] = TEXT("1.0");
// Secondary character interaction (default F), as used for drone entry.
// Keep primary interaction (default E) available for the electrical socket.
inline constexpr TCHAR InteractActionPath[] = TEXT("/Game/Game/Input/Character/IAV_InteractTwo.IAV_InteractTwo");
inline constexpr TCHAR ActionName[] = TEXT("RailgunEnter");
inline constexpr TCHAR Label[] = TEXT("Enter Railgun");
inline constexpr TCHAR LoopPackage[] = TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");
inline const FName LoopGraph(TEXT("ForEachLoop"));
// Runtime geometry is copied from the shell's manifest-authored entry reference.
inline const FVector BoxExtent(1.0f, 1.0f, 1.0f);
inline const FVector BoxOffset = FVector::ZeroVector;
}
