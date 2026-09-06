#pragma once
#include "CoreMinimal.h"

// Mod-owned experiment tuning. Game identity revalidated using public stock
// JSON on Steam 25056839 / UE5.8.1, exe CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// Renew the class and loader contract on fingerprint change. No Voyage mirror.
namespace ProbeNames
{
inline constexpr TCHAR Package[] = TEXT("/Game/Mods/HarpoonCannonLifecycleProbe/ModActor");
inline constexpr TCHAR Asset[] = TEXT("ModActor");
inline constexpr TCHAR DroneClass[] = TEXT("/Game/Blueprints/Vehicles/BP_CameraDrone.BP_CameraDrone_C");
inline const FName Root(TEXT("ProbeRoot"));
inline const FName Title(TEXT("TitleText"));
inline const FName Spawn(TEXT("SpawnText"));
inline const FName Current(TEXT("CurrentText"));
inline const FName Clock(TEXT("ClockText"));
inline const FName Distance(TEXT("DistanceText"));
inline const FName Hidden(TEXT("HiddenText"));
inline const FName Collision(TEXT("CollisionText"));
inline const FName Marker(TEXT("DroneMarkerText"));
inline const FName Control(TEXT("ControlText"));
inline const FName Entered(TEXT("EnteredText"));
inline const FName Returned(TEXT("ReturnedText"));
inline const FName OriginalPawn(TEXT("OriginalPlayerPawn"));
inline const FName SawDroneControl(TEXT("SawDroneControl"));
inline const FName SawReturn(TEXT("SawReturnToOriginalPawn"));
inline const FName Physics(TEXT("PhysicsText"));
inline const FName OriginalPhysicsText(TEXT("OriginalPhysicsText"));
inline const FName Drift(TEXT("DriftText"));
inline const FName FreezeStatus(TEXT("FreezeStatusText"));
inline const FName PhysicsBody(TEXT("ObservedPhysicsBody"));
inline const FName EntryLocation(TEXT("FreezeEntryLocation"));
inline const FName EntryAttempted(TEXT("FreezeEntryAttempted"));
inline const FName FreezeHeld(TEXT("FreezeHeld"));
inline const FName OriginalSimulation(TEXT("OriginalSimulation"));
inline const FName State(TEXT("ProbeState"));
inline const FName Age(TEXT("ObservationSeconds"));
inline const FName Drone(TEXT("SpawnedDrone"));
inline constexpr TCHAR UnitScale[] = TEXT("1,1,1");
inline constexpr TCHAR BoardOffset[] = TEXT("180,-85,48");
inline constexpr TCHAR SpawnOffset[] = TEXT("350,140,70");
inline constexpr TCHAR True[] = TEXT("true");
inline constexpr TCHAR False[] = TEXT("false");
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR Observing[] = TEXT("1");
inline constexpr TCHAR Finished[] = TEXT("2");
inline constexpr TCHAR MarkerOffset[] = TEXT("0,0,45");
inline constexpr TCHAR EmptyText[] = TEXT("");
inline constexpr TCHAR MarkerText[] = TEXT("HC04 DRONE BELOW - PHYSICS TEST");
inline constexpr TCHAR PhysicsYes[] = TEXT("Root physics simulating: YES");
inline constexpr TCHAR PhysicsNo[] = TEXT("Root physics simulating: NO");
inline constexpr TCHAR PhysicsUnknown[] = TEXT("Root physics: not sampled");
inline constexpr TCHAR OriginalPhysicsYes[] = TEXT("Physics before freeze: YES");
inline constexpr TCHAR OriginalPhysicsNo[] = TEXT("Physics before freeze: NO");
inline constexpr TCHAR FreezeWaiting[] = TEXT("Physics test: waiting for entry");
inline constexpr TCHAR FreezeApplied[] = TEXT("Physics OFF requested once; test mouse/WASD");
inline constexpr TCHAR FreezeFailed[] = TEXT("Physics test FAILED: no primitive root; EXIT");
inline constexpr TCHAR FreezeRestored[] = TEXT("Original physics restored; exit observed");
inline constexpr TCHAR DriftPrefix[] = TEXT("World drift since freeze (cm): ");
inline constexpr TCHAR ControlYes[] = TEXT("Current player pawn = Drone: YES");
inline constexpr TCHAR ControlNo[] = TEXT("Current player pawn = Drone: NO");
inline constexpr TCHAR EnteredYes[] = TEXT("Drone control observed: YES");
inline constexpr TCHAR EnteredNo[] = TEXT("Drone control observed: NO");
inline constexpr TCHAR ReturnedYes[] = TEXT("Returned to original pawn: YES");
inline constexpr TCHAR ReturnedNo[] = TEXT("Returned to original pawn: NO");
inline constexpr TCHAR HiddenYes[] = TEXT("Actor hidden: YES");
inline constexpr TCHAR HiddenNo[] = TEXT("Actor hidden: NO");
inline constexpr TCHAR HiddenUnknown[] = TEXT("Actor hidden: unavailable");
inline constexpr TCHAR CollisionYes[] = TEXT("Actor collision: ON");
inline constexpr TCHAR CollisionNo[] = TEXT("Actor collision: OFF");
inline constexpr TCHAR CollisionUnknown[] = TEXT("Actor collision: unavailable");
inline constexpr TCHAR AlwaysSpawn[] = TEXT("AlwaysSpawn");
inline constexpr TCHAR WaitingText[] = TEXT("HC04 AUTOLOAD: waiting for player");
inline constexpr TCHAR RunningText[] = TEXT("HC04: PHYSICS ONLY; NOT A SHIP MOUNT");
inline constexpr TCHAR LoadFailureText[] = TEXT("HC04 CLASS LOAD FAILED; no spawn");
inline constexpr TCHAR SpawnWaitingText[] = TEXT("Initial spawn: not attempted");
inline constexpr TCHAR SpawnYes[] = TEXT("Initial spawn: VALID");
inline constexpr TCHAR SpawnNo[] = TEXT("Initial spawn: INVALID");
inline constexpr TCHAR CurrentWaitingText[] = TEXT("Last independent sample: waiting");
inline constexpr TCHAR CurrentYes[] = TEXT("Last independent sample: ALIVE");
inline constexpr TCHAR CurrentNo[] = TEXT("Last independent sample: INVALID");
inline constexpr TCHAR ClockPrefix[] = TEXT("Observation seconds: ");
inline constexpr TCHAR DistancePrefix[] = TEXT("Drone distance (cm): ");
inline constexpr TCHAR NoDistance[] = TEXT("Drone distance: unavailable");
inline constexpr float FontSize = 3.5f;
inline constexpr float RowSpacing = 5.0f;
inline constexpr float TickInterval = 0.2f;
}
