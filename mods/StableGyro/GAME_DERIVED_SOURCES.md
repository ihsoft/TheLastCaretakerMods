# Game-derived source and build inputs

## Fingerprint gate

- Steam build: 25982825
- Unreal Editor: 5.8.2
- Unreal parser target: 5.8
- VoyageSteam-Win64-Shipping.exe SHA-256:
  08228B72D4590F9B00E2171F252D618464C2E04B785237FE728DDDBF27D3B734

Run tools/Get-VoyageBuildFingerprint.ps1 before every generation or package
build. A mismatch invalidates the current-fingerprint contracts below and
requires the producer to re-extract its stock inputs; historical findings retain
only their explicitly named fingerprint.

## Game contracts

The public producer freshly extracts the stock cooked inputs and asserts the
asset contracts consumed by each candidate build. Current mapping metadata
independently revalidates the native owner and property types consumed by the
stabilization and calibration-prevention graphs. Access flags, offsets, CDO
defaults and bounded native-code findings retain their explicit older
fingerprints below rather than being silently carried forward. Together, these
sources establish:

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
- current mapping metadata owns float `PropellerVelocity`, `CurrentThrottle`,
  `Collective`, and `HoverPitchScale`, plus bool `bEngineRunning`,
  `bAutoHoverCollective`, and `bAltitudeAssist`, on
  `/Script/Voyage.VoyageVehicleGyroCopter`;
- Steam build `25956314`, executable SHA-256
  `0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A`,
  established the protected Blueprint-readable SaveGame flags for
  `PropellerVelocity`, `CurrentThrottle`, `bEngineRunning`, `Collective`, and
  `HoverPitchScale`, the public Blueprint-readonly non-SaveGame flags for
  `bAutoHoverCollective` and `bAltitudeAssist`, and the stock CDO
  `HoverPitchScale` value `1`. Those flags and the CDO value were not
  independently revalidated for the current fingerprint;
- the prevention graph
  temporarily writes them through the current Engine
  `KismetSystemLibrary.SetBoolPropertyByName(UObject, FName, bool)` contract and
  restores their captured values; it does not alter their reflected flags;
- StableGyro leaves `Collective` untouched and resets `HoverPitchScale` to the
  user-selected target `1.0` once on player entry. That target uses the older
  proven stock default above; it is not a current-fingerprint CDO claim. The
  reset uses the Engine
  `KismetSystemLibrary.SetDoublePropertyByName(UObject, FName, double)` contract,
  whose implementation accepts a native float property fallback. The generated
  graph preserves the property's reflected readonly flags. The version-bound
  calibration formula, field offsets and reset boundary are documented in
  [ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md);
- the current stock `/Game/UI/Game/HUD/WBP_VoyageIngameGyroCopter` binds the
  `Value` property of its `Throttle` slider directly to
  `GyroCopter.Collective`. The widget has no `MinValue` or `MaxValue` override,
  so the UE 5.8.2 native Slider defaults `0..1` establish the displayed
  percentage as `Collective * 100`;
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
owner and field types consumed by the altitude graphs were revalidated from
current mapping metadata for the fingerprint above. Their mirror access flags
retain the explicitly bounded older evidence. The public producer
must freshly extract and assert every consumed stock asset contract for a
candidate build. Older native implementation details remain historical unless
stated otherwise. Build and static validation do not establish current runtime
compatibility.
