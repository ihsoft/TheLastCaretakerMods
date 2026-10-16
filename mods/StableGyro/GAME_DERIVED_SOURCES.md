# Game-derived source and build inputs

## Fingerprint gate

- Steam build: 25956314
- Unreal Editor: 5.8.2
- Unreal parser target: 5.8
- VoyageSteam-Win64-Shipping.exe SHA-256:
  0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A

Run tools/Get-VoyageBuildFingerprint.ps1 before every generation or package
build. A mismatch invalidates the current-fingerprint contracts below and
requires the producer to re-extract its stock inputs; historical findings retain
only their explicitly named fingerprint.

## Game contracts

The public producer freshly extracts the stock cooked inputs and asserts the
asset contracts consumed by each candidate build. Current native reflection
metadata independently establishes the fields consumed by the stabilization
and calibration-prevention graphs. Earlier bounded native-code findings retain
their explicit historical fingerprint below rather than being silently carried
forward. Together, these sources establish:

- the vehicle package is /Game/Blueprints/Vehicles/BP_GyroCopter_Possessable;
- its native parent is /Script/Voyage.VoyageVehicleGyroCopter;
- /Game/Data/Assets/Modules/DA_Item_Module_GyroCopter selects the class for
  newly built Gyros through its DroppedActor soft class path;
- W maps to IA_GyroCopterTiltForward, S maps to
  IA_GyroCopterTiltBackward, and both feed the native float
  TiltForwardInput;
- A/D feed the native float `TiltInput`;
- native `TiltForwardInput` and `TiltInput` remain protected and
  Blueprint-readable. The editor-only mirror exposes them as writable only to
  emit the already established Blueprint assignments; it is not a native flag
  claim;
- current native reflection owns `PropellerVelocity` as a protected,
  Blueprint-readable float property on
  `/Script/Voyage.VoyageVehicleGyroCopter`;
- current native reflection owns `CurrentThrottle` and `bEngineRunning` as
  protected, Blueprint-readable SaveGame properties on that class. Native
  control flow updates throttle while the engine runs, decays it while stopped,
  and zeros both engine and throttle state on engine cut;
- current native reflection owns public Blueprint-readonly, non-SaveGame
  booleans `bAutoHoverCollective` and `bAltitudeAssist`. The prevention graph
  temporarily writes them through the current Engine
  `KismetSystemLibrary.SetBoolPropertyByName(UObject, FName, bool)` contract and
  restores their captured values; it does not alter their reflected flags;
- current native reflection owns protected BlueprintReadOnly SaveGame floats
  `Collective` and `HoverPitchScale`; the latter has native, stock Blueprint and
  mod-child CDO value `1`. StableGyro does not write or migrate either saved
  value. The version-bound calibration formula, field offsets and recovery
  limits are documented in [ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md);
- the current stock Blueprint overrides its tilt limits to `-15..15` and its
  interpolation speed to `5`. StableGyro does not copy those values or replace
  the native flight implementation;
- the game settings reader
  `VoyageEditorBlueprintFunctionLibrary.LoadFileToArray(FString)` has an
  independently revalidated current native owner and exact string-to-string-array
  signature;
- `/Game/Game/Input/Vehicle/IMC_GyroCopter_Keyboard` contains the stock
  mouse, E, T, Space, LeftControl, arrow, W/S/A/D, and F mappings and has no X
  mapping;
- current stock `BP_GyroCopter_Possessable` has no Blueprint function override.
  The generated child overrides the inherited native
  `VoyageVehiclePawn.GetProvidedActionsBP`, calls that parent implementation to
  preserve its actions, and appends the mod-owned X reset action;
- StableGyro reads the reflected engine-running, current-throttle, and rotor
  velocity properties for its stabilization gate;
- the native Gyro primary tick is in `TG_PostPhysics`. The generated pitch
  helper therefore runs in `TG_PrePhysics` and is an explicit prerequisite of
  the Gyro, while the PostPhysics stabilization helper explicitly depends on
  the Gyro tick;
- the Blueprint property `MeshComponent` references a runtime component whose
  UObject name is `VehicleMesh`. Code that scans components through
  `GetObjectName` must compare the runtime UObject name, not the reflected
  property name.

## Historical native context

Steam build `25880952`, executable SHA-256
`16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44`,
established that native forward input targets
`TiltForwardAngle * TiltForwardInput` in `CommandedTilt`; normalized rotation
interpolation uses `DeltaSeconds * TiltInterpSpeed`. The native control pass
then computes `speedRatio = clamp(PropellerVelocity / GovernedRotorSpeed, 0, 1)`
when `GovernedRotorSpeed > 0`, otherwise zero. It sets
`HullAttitudeTarget = CommandedTilt * speedRatio * HullAttitudeFollow` and
applies `CommandedTilt - HullAttitudeTarget` as the tilt component's relative
rotation. The native default `HullAttitudeFollow=0.7` had no stock Blueprint
override. At `speedRatio=1`, this left 30% of the commanded angle on the rotor
relative to the hull (4.5 degrees at full forward input), rather than halving
the input integration rate. That fingerprint also established native defaults
`PropellerVelocityScale=50` and `PropellerLift=75`. These details were not
revalidated for the current fingerprint and are not consumed by this build.

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
must remain below ignored `Tmp` or registered `artifacts` paths. The native
fields and setter consumed by the calibration-prevention graph were revalidated
from current reflection metadata for the fingerprint above. The public producer
must freshly extract and assert every consumed stock asset contract for a
candidate build. Older native implementation details remain historical unless
stated otherwise. Build and static validation do not establish current runtime
compatibility.
