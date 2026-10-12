# Game-derived source and build inputs

## Fingerprint gate

- Steam build: 25827816
- Unreal Editor: 5.8.2
- Unreal parser target: 5.8
- VoyageSteam-Win64-Shipping.exe SHA-256:
  C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33

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
- A/D feed the native float `TiltInput`;
- current native reflection owns `CurrentThrottle` and `PropellerVelocity` as
  protected, Blueprint-readable float properties on
  `/Script/Voyage.VoyageVehicleGyroCopter`;
- the current stock Blueprint overrides its tilt limits to `-15..15` and its
  interpolation speed to `5`. StableGyro does not copy those values or replace
  the native flight implementation;
- the game settings reader
  VoyageEditorBlueprintFunctionLibrary.LoadFileToArray(FString) retains the
  exact signature already established by Railgun on this fingerprint;
- `/Game/Game/Input/Vehicle/IMC_GyroCopter_Keyboard` contains the stock
  mouse, E, T, Space, LeftControl, arrow, W/S/A/D, and F mappings and has no X
  mapping;
- current stock `BP_GyroCopter_Possessable` has no Blueprint function override.
  The generated child overrides the inherited native
  `VoyageVehiclePawn.GetProvidedActionsBP`, calls that parent implementation to
  preserve its actions, and appends the mod-owned X reset action;
- current native defaults retain `PropellerVelocityScale=50` and
  `PropellerLift=75`. The old Blueprint flight formula and derived percentage
  estimates are not treated as current contracts. StableGyro only reads the
  reflected rotor velocity and throttle values used by its existing gates;
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
must remain below ignored `Tmp` or registered `artifacts` paths. Current source
contracts were revalidated from native reflection metadata and stock assets for
the fingerprint above; build and static validation do not establish current
runtime compatibility.
