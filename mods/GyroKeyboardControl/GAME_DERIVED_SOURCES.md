# Game-derived source and build inputs

## Current fingerprint

- Steam build: 25191271
- Unreal parser target: 5.8
- VoyageSteam-Win64-Shipping.exe SHA-256:
  747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B

Run tools/Get-VoyageBuildFingerprint.ps1 before every generation or package
build. A mismatch invalidates all contracts below.

## Revalidated contracts

Current stock cooked assets establish:

- the vehicle package is /Game/Blueprints/Vehicles/BP_GyroCopter_Possessable;
- its native parent is /Script/Voyage.VoyageVehicleGyroCopter;
- /Game/Data/Assets/Modules/DA_Item_Module_GyroCopter selects the class for
  newly built Gyros through its DroppedActor soft class path;
- W maps to IA_GyroCopterTiltForward, S maps to
  IA_GyroCopterTiltBackward, and both feed the native float
  TiltForwardInput;
- Gyro Blueprint bytecode consumes TiltForwardInput for forward/back pitch,
  camera response, and total-throttle calculations;
- the game settings reader
  VoyageEditorBlueprintFunctionLibrary.LoadFileToArray(FString) retains the
  exact signature already validated by Railgun on this fingerprint.
- `/Game/Game/Input/Vehicle/IMC_GyroCopter_Keyboard` contains the stock
  mouse, E, T, Space, LeftControl, arrow, W/S/A/D, and F mappings and has no X
  mapping;
- stock `BP_GyroCopter_Possessable.GetProvidedActionsBP` overrides the native
  `VoyageVehiclePawn` declaration and returns the conditional F DropCargo
  action. The generated child calls that exact Blueprint parent function and
  appends the mod-owned X reset action.
- stock lift is `TiltControl.UpVector * PropellerVelocity * PropellerLift *
  (1 - max(Altitude, 0) * LiftReductionByHeight)`, with `PropellerLift=75` and
  forward/back target pitch `-10 degrees * TiltForwardInput`;
- stock `GetTotalThrottle` adds raw `abs(ThrottleInput)` on top of
  `CurrentThrottle`, so held Space can increase PhysicalEnergy production even
  when CurrentThrottle is already clamped to 1. Gameplay testing established
  that this extra power is required for useful climb performance, so the mod
  leaves the complete stock function unchanged.

The generated helper treats only exact native digital markers -1, 0, and 1 as
commands. It integrates W/S through DeltaSeconds, preserves the integrated
value when neither key is active, clamps mod output to +/-0.9999, and writes X
as immediate neutral. When the owning Gyro is not player controlled, helper
and native pitch state are reset to zero.

## Tracked reconstructed inputs

- Source/Voyage/VoyageVehiclePawn.h
- Source/Voyage/VoyageVehicleGyroCopter.h
- Source/Voyage/VoyageEditorBlueprintFunctionLibrary.h
- Source/Voyage/PlayerInputInterfaceAction.h
- Source/Voyage/VoyageInputAction.h
- Source/Voyage/VoyageGameUserSettings.h
- both generator commandlets and Build-InheritancePackage.ps1

The original Gyro item data asset, scriptobjects.bin, mappings, extracted JSON,
cooked packages, containers, inventories, and build logs are game-derived and
must remain below ignored artifacts paths.

## Validation status

The user validated the complete v1 package in Voyage on 2026-09-22: the
subclass-plus-DroppedActor architecture, enter/exit, pitch controls, X reset,
and the standard X HUD hint work in the real game. The stock parent action list
is preserved by construction. The first v2 lift formula was not sufficient in
game, while removing the held-Space contribution made useful climb impossible;
that throttle experiment is removed. A hard-coded `PropellerLift=300` probe
proved the helper's reflection write reaches the live physics field and is not
overwritten before force calculation. The later PropellerLift-scale candidate
  still produced no clearly visible height support in the user's test. The
  physical world-up-force experiment and both of its settings have since been
  removed. The current candidate uses kinematic full-throttle altitude hold
  with smooth positive-vertical-speed braking after Space release. On
  2026-09-24 the user confirmed that both remaining settings work in the real
  game: `PitchRampSeconds=3.0` and
  `AltitudeStabilizationVerticalDeceleration=100.0`.
