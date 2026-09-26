# Game-derived source and build inputs

## Fingerprint gate

- Steam build: 25191271
- Unreal parser target: 5.8
- VoyageSteam-Win64-Shipping.exe SHA-256:
  747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B

Run tools/Get-VoyageBuildFingerprint.ps1 before every generation or package
build. A mismatch invalidates all contracts below.

## Game contracts

The stock cooked assets establish:

- the vehicle package is /Game/Blueprints/Vehicles/BP_GyroCopter_Possessable;
- its native parent is /Script/Voyage.VoyageVehicleGyroCopter;
- /Game/Data/Assets/Modules/DA_Item_Module_GyroCopter selects the class for
  newly built Gyros through its DroppedActor soft class path;
- W maps to IA_GyroCopterTiltForward, S maps to
  IA_GyroCopterTiltBackward, and both feed the native float
  TiltForwardInput;
- A/D feed the native float `TiltInput`; the stock Blueprint combines it with
  `TiltForwardInput` when rotating the `TiltControl` scene component;
- the stock rotor target is pitch `-10 * TiltForwardInput`, roll
  `10 * TiltInput`; `TiltControl.RelativeRotation` approaches it through
  normalized `RLerp` with alpha `DeltaSeconds * 2`, so the live thrust vector
  changes smoothly after either axis changes;
- Gyro Blueprint bytecode consumes TiltForwardInput for forward/back pitch,
  camera response, and total-throttle calculations;
- the game settings reader
  VoyageEditorBlueprintFunctionLibrary.LoadFileToArray(FString) retains the
  exact signature already established by Railgun on this fingerprint;
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
- stock `UpdatePropeller` writes `PropellerVelocity` as the magnitude of the
  rotor's physical angular velocity in radians per second multiplied by `50`;
  the rotor body overrides its maximum angular velocity to `360000` degrees per
  second, but this is only a Chaos safety cap and is far above its normal
  operating speed. The stock physical root overrides its mass to `300 kg`.
  With `PropellerLift=75`, near-sea-level hover at ordinary 100%
  throttle corresponds approximately to
  `PropellerVelocity = 300 * 980 / 75 = 3920`. Assuming the stock near-linear
  steady-state response gives approximate references `392`, `784`, and `1176`
  at 10%, 20%, and 30% throttle. The stabilization gate intentionally consumes
  a direct `PropellerVelocity` threshold so its tuning does not depend on a
  guessed maximum;
- stock `GetTotalThrottle` adds raw `abs(ThrottleInput)` on top of
  `CurrentThrottle`, so held Space can increase PhysicalEnergy production even
  when CurrentThrottle is already clamped to 1. The extra power is required for
  useful climb performance, so the mod leaves the complete stock function
  unchanged;
- the Blueprint property `MeshComponent` references a runtime component whose
  UObject name is `VehicleMesh`. Code that scans components through
  `GetObjectName` must compare the runtime UObject name, not the reflected
  property name.

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
