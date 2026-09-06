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
inline const FName State(TEXT("ProbeState"));
inline const FName Age(TEXT("ObservationSeconds"));
inline const FName Drone(TEXT("SpawnedDrone"));
inline constexpr TCHAR UnitScale[] = TEXT("1,1,1");
inline constexpr TCHAR BoardOffset[] = TEXT("180,-85,48");
inline constexpr TCHAR SpawnOffset[] = TEXT("350,140,70");
inline constexpr TCHAR True[] = TEXT("true");
inline constexpr TCHAR Zero[] = TEXT("0");
inline constexpr TCHAR Observing[] = TEXT("1");
inline constexpr TCHAR Finished[] = TEXT("2");
inline constexpr TCHAR ObserveSeconds[] = TEXT("60");
inline constexpr TCHAR MarkerOffset[] = TEXT("0,0,45");
inline constexpr TCHAR EmptyText[] = TEXT("");
inline constexpr TCHAR MarkerText[] = TEXT("HC02 DRONE BELOW - LOOK ONLY");
inline constexpr TCHAR HiddenYes[] = TEXT("Actor hidden: YES");
inline constexpr TCHAR HiddenNo[] = TEXT("Actor hidden: NO");
inline constexpr TCHAR HiddenUnknown[] = TEXT("Actor hidden: unavailable");
inline constexpr TCHAR CollisionYes[] = TEXT("Actor collision: ON");
inline constexpr TCHAR CollisionNo[] = TEXT("Actor collision: OFF");
inline constexpr TCHAR CollisionUnknown[] = TEXT("Actor collision: unavailable");
inline constexpr TCHAR AlwaysSpawn[] = TEXT("AlwaysSpawn");
inline constexpr TCHAR WaitingText[] = TEXT("HC02 AUTOLOAD: waiting for player");
inline constexpr TCHAR RunningText[] = TEXT("HC02: approach marker; DO NOT ENTER/LOOT");
inline constexpr TCHAR FinishedText[] = TEXT("HC02 DONE: final sample; cleanup requested");
inline constexpr TCHAR LoadFailureText[] = TEXT("HC02 CLASS LOAD FAILED; no spawn");
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
