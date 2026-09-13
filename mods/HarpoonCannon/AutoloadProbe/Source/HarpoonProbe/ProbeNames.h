#pragma once
#include "CoreMinimal.h"

// HC13 Engine-only emission renewed for Steam25191271 / UE5.8 parser target,
// executable747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// See GAME_DERIVED_SOURCES.md; dormant Drone identities below remain historical.

// Mod-owned experiment tuning. Game identity revalidated using public stock
// JSON on Steam 25056839 / UE5.8.1, exe CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// Renew the class and loader contract on fingerprint change. No Voyage mirror.
namespace ProbeNames
{
inline const FName NativeHudRequested(TEXT("NativeHudCallbackObserved"));
inline constexpr TCHAR NativeHudYes[] = TEXT("HC18 HUD CALLBACK: YES | x5 | F8 EXIT");
inline constexpr TCHAR NativeHudNo[] = TEXT("HC18 HUD CALLBACK: NO | x5 | F8 EXIT");
inline constexpr TCHAR Package[] = TEXT("/Game/Mods/HarpoonCannonLifecycleProbe/ModActor");
inline constexpr TCHAR Asset[] = TEXT("ModActor");
inline constexpr TCHAR HudPackage[] = TEXT("/Game/Mods/HarpoonCannonLifecycleProbe/ProbeHUD");
inline constexpr TCHAR HudAsset[] = TEXT("ProbeHUD");
inline const FName HudInstance(TEXT("DiagnosticHUD"));
inline const FName HudTree(TEXT("WidgetTree"));
inline const FName HudCanvas(TEXT("DiagnosticCanvas"));
inline const FName HudBorder(TEXT("DiagnosticBorder"));
inline const FName HudRows(TEXT("DiagnosticRows"));
inline constexpr TCHAR HudZOrder[] = TEXT("20");
inline constexpr int32 HudFontSize = 14;
inline const FLinearColor HudForeground(0.1f, 1.0f, 1.0f, 1.0f);
inline const FLinearColor HudBackground(0.0f, 0.0f, 0.0f, 0.8f);
inline constexpr float HudPadding = 12.0f;
inline const FVector2D HudAnchor(1.0f, 0.0f);
inline const FVector2D HudOffset(-20.0f, 70.0f);
inline const FVector2D HudSize(620.0f, 440.0f);
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
inline const FName EntryAttempted(TEXT("MountAttempted"));
inline const FName FreezeHeld(TEXT("FreezeHeld"));
inline const FName MountBroken(TEXT("MountOverrideObserved"));
inline const FName OriginalSimulation(TEXT("OriginalSimulation"));
inline const FName Carrier(TEXT("CarrierComponent"));
inline const FName LocalAnchor(TEXT("CarrierLocalAnchor"));
inline const FName CarrierStart(TEXT("CarrierStartPosition"));
inline const FName CarrierNameText(TEXT("CarrierNameText"));
inline const FName AttachmentText(TEXT("AttachmentText"));
inline const FName LocalDriftText(TEXT("LocalDriftText"));
inline const FName CarrierTravelText(TEXT("CarrierTravelText"));
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
inline constexpr TCHAR MarkerText[] = TEXT("HC05 DRONE BELOW - DECK ATTACH TEST");
inline constexpr TCHAR TraceAbove[] = TEXT("0,0,30");
inline constexpr TCHAR TraceBelow[] = TEXT("0,0,-400");
inline constexpr TCHAR VisibilityTrace[] = TEXT("TraceTypeQuery1");
inline constexpr TCHAR KeepWorld[] = TEXT("KeepWorld");
inline constexpr TCHAR NoSupport[] = TEXT("NO SUPPORT: unchanged Drone; EXIT");
inline constexpr TCHAR ExistingParent[] = TEXT("ALREADY PARENTED: unchanged Drone; EXIT");
inline constexpr TCHAR AttachFailed[] = TEXT("ATTACH FAILED: physics restored; EXIT");
inline constexpr TCHAR CarrierLost[] = TEXT("SUPPORT LOST: EXIT now");
inline constexpr TCHAR AttachedYes[] = TEXT("Attached to sampled support: YES");
inline constexpr TCHAR AttachedNo[] = TEXT("Attached to sampled support: NO");
inline constexpr TCHAR LocalDriftPrefix[] = TEXT("Support-relative drift (cm): ");
inline constexpr TCHAR CarrierTravelPrefix[] = TEXT("Support world travel (cm): ");
inline constexpr TCHAR PhysicsYes[] = TEXT("Root physics simulating: YES");
inline constexpr TCHAR PhysicsNo[] = TEXT("Root physics simulating: NO");
inline constexpr TCHAR PhysicsUnknown[] = TEXT("Root physics: not sampled");
inline constexpr TCHAR OriginalPhysicsYes[] = TEXT("Physics before freeze: YES");
inline constexpr TCHAR OriginalPhysicsNo[] = TEXT("Physics before freeze: NO");
inline constexpr TCHAR MountDelay[] = TEXT("8.0");
inline constexpr TCHAR FreezeWaiting[] = TEXT("WAIT: settling before mount; DO NOT ENTER");
inline constexpr TCHAR EarlyEntry[] = TEXT("EXIT first: mount requires unoccupied Drone");
inline constexpr TCHAR FreezeApplied[] = TEXT("MOUNT READY: check outside, ENTER, then EXIT");
inline constexpr TCHAR MountOverridden[] = TEXT("MOUNT OVERRIDE OBSERVED: screenshot, EXIT");
inline constexpr TCHAR FreezeFailed[] = TEXT("Physics test FAILED: no primitive root; EXIT");
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
inline constexpr TCHAR WaitingText[] = TEXT("HC07 AUTOLOAD: waiting for player");
inline constexpr TCHAR RunningText[] = TEXT("HC07: ONE MOUNT ACROSS ENTRY / EXIT");
inline constexpr TCHAR LoadFailureText[] = TEXT("HC07 CLASS LOAD FAILED; no spawn");
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
