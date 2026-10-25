# Game-derived source and build inputs

## Source-build gate

The current accepted Steam build pin is in `game-build.json`; the Unreal
parser target is `5.8`. The pin identifies exact build inputs, not the game's
player-facing Update or executable version.

The producer checks the Steam build ID while extracting current stock inputs.
A different build ID permits a candidate attempt with
`-IgnorePinnedGameVersion`; a successful build needs a game test. Investigate
the affected asset contract if either gate fails. Historical findings retain
only their explicitly named build.

## Game contracts

The public producer freshly extracts the stock cooked inputs and asserts the
asset contracts consumed by each candidate build. Current mapping metadata
independently revalidates the native owner and property types consumed by the
stabilization and calibration-prevention graphs. Previously inspected access
flags, CDO defaults and native details retain their explicit older fingerprints
unless independently revalidated below; they are not silently carried forward.

The native calculation chain in [altitude control](ALTITUDE-CONTROL.md) was
checked against Steam build `25982825`, UE `5.8.2`, Shipping executable SHA-256
`08228B72D4590F9B00E2171F252D618464C2E04B785237FE728DDDBF27D3B734`.
Its scope is input/collective/assist/calibration, rotor-speed governor, resource
drive, lift and base fuel request. Engine registration/thunk inspection
identifies the governor's virtual `GetMass` query and the module's
`GetResourceAmount(EModuleResourceType): double` getter. Formula-level evidence
and native entry addresses belong in that document; this does not promote
older reflected access flags or native CDO values into current observations.

Together, these sources establish:

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
- An earlier native snapshot established the protected Blueprint-readable
  SaveGame flags for
  `PropellerVelocity`, `CurrentThrottle`, `bEngineRunning`, `Collective`, and
  `HoverPitchScale`, the public Blueprint-readonly non-SaveGame flags for
  `bAutoHoverCollective` and `bAltitudeAssist`, and the stock CDO
  `HoverPitchScale` value `1`. Those flags and the CDO value were not
  independently revalidated for the current fingerprint;
- the prevention graph
  temporarily writes them through the current Engine
  `KismetSystemLibrary.SetBoolPropertyByName(UObject, FName, bool)` contract and
  restores their captured values; it does not alter their reflected flags;
- StableGyro leaves `Collective` untouched and, while altitude control is enabled,
  writes the entry-reloaded `HoverPitchScale` INI setting once on player entry.
  Its default and invalid-or-missing fallback are `2.5`, with accepted range
  `0.5..2.5`. This is mod tuning, not a claim about the native CDO default. The
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
- stock throttle actions are `/Game/Game/Input/Vehicle/IA_GyroCopterThrottleUp`
  and `IA_GyroCopterThrottleDown`, with player-mappable names
  `GyroCopterThrottleUp` and `GyroCopterThrottleDown`. The keyboard context's
  fourteen entries inherit their settings from their actions and have no
  mapping-owned player-mappable settings. Its MouseY Negate modifier and E/T
  Pressed triggers use default parameters. The generated context preserves
  these entries and appends `IAV_GyroPitchReset` on X with a Pressed trigger and
  player-mappable name `GyroPitchReset`. The stock gamepad context uses the same
  throttle actions on RightTriggerAxis/LeftTriggerAxis with no mapping triggers
  or modifiers;
- current stock `BP_GyroCopter_Possessable` has no Blueprint function override.
  The generated child overrides the inherited native
  `VoyageVehiclePawn.GetProvidedActionsBP`, calls that parent implementation to
  preserve its actions, and appends the mod-owned pitch-reset action. Steam build
  `25982825` native inspection establishes that the Gyro provider calls this
  base path first and only then appends its native cargo action when the
  reflected `DropCargoAction` object property is non-null. The child therefore
  preserves parent `ReceiveBeginPlay` and, only in four-port mode, clears that
  property through
  `KismetSystemLibrary.SetObjectPropertyByName`, and owns one replacement
  Enhanced Input binding and one provided-action record for the unchanged stock
  `/Game/Game/Input/Vehicle/IA_GyroCopterDropCargo` action. Current native
  inspection also establishes the owning `VoyageVehicleGyroCopter` float
  `DropCargoTriggerTime` at actor offset `0x72c`, its constructor default
  `1.0`, and the native provider's direct copy of that live value into the
  provided action's `TriggerTime`. The editor mirror appends this field only
  so the child can read it; the child does not serialize an override. Started
  arms a fresh press, Triggered releases only after its elapsed time reaches
  the same live field and only once per press, and Completed, Canceled, and
  controller changes clear the transient press state;
- the same current native inspection establishes
  `VoyageMiscBlueprintFunctionLibrary.DisconnectCableFromSocket(
  ModuleSocketComponent, bool)` as the stock disconnect helper and
  `ModuleSocketComponent.IsSocketConnectedOrHasCable()` as the stock cargo-hint
  predicate. The child applies them only to valid socket components whose
  `Port.ResourceType` is Mooring and passes `false` for cable destruction;
- StableGyro reads the reflected engine-running, current-throttle, and rotor
  velocity properties for its stabilization gate;
- the native Gyro primary tick is in `TG_PostPhysics`. The generated pitch
  helper therefore runs in `TG_PrePhysics` and is an explicit prerequisite of
  the Gyro, while the PostPhysics stabilization helper explicitly depends on
  the Gyro tick;
- the Blueprint property `MeshComponent` references a runtime component whose
  UObject name is `VehicleMesh`. Code that scans components through
  `GetObjectName` must compare the runtime UObject name, not the reflected
  property name;
- Steam build `25982825` stock Gyro JSON
  `BP_GyroCopter_Possessable.json` (SHA-256
  `51B0B5156265672FCD8AA0995F2F43897A81397ABFA4461E06C57527A0AFD5BB`)
  establishes the two inherited `VoyageModuleSocketViewComponent` mooring
  templates. Both attach to the native `VehicleMesh`, use
  `DA_Socket_MooringSmall`, `ST_Any`, Mooring resource/category,
  `DA_Cable_Hook`, integrated length `1000`, disabled auto-initialization and
  absolute scale. Their stock socket IDs are `1051774499` and `3437279300`.
  Their stock relative locations are
  `(75.481674, 54.50848, -33.890068)` and
  `(75.483, -54.360676, -33.705635)` respectively. The disabled four-port path
  leaves the inherited stock components and their instance transforms untouched.
  The generated child adds two distinct rear templates with the same socket
  contract and unique IDs `2056977161` and `2056977162`. It also adds four
  decorative plate components from
  two side-specific meshes derived from the current stock
  `/Game/AssetSets/Vehicles/Gyrocopter/Mesh/Gyro_MooringPlate` and reuses the
  stock `Gyrocopter_Seat_p1` material. The public producer exports the current
  decoded Nanite leaf geometry, requiring all four streaming pages, all 223
  clusters, all 111 leaf clusters, 13,790 faces and four UV channels without an
  ordinary-LOD fallback. The decoded source has no tangent stream, so both the
  source GLB and Blender-split halves omit tangents. The splitter explicitly
  copies corner normals, vertex colors, UV0..3 and material slots, and removes
  no faces. Independent raw-GLB readback verifies triangle positions, UV0..3,
  corner normals and vertex colors. Blender cannot
  faithfully retain some imported normals near degenerate geometry, so the
  bounded splitter restores the raw `NORMAL` accessor by complete
  triangle/corner position+UV identity after export. Source-absent tangents stay
  absent from the split GLB and Unreal derives them while retaining imported
  normals. Interchange enables Nanite on import and otherwise produces a reduced
  fallback mesh, so the generator explicitly disables Nanite and builds these
  two derived assets as ordinary static meshes. All 114
  exact-zero-area decoded faces reach the source GLB, though meter conversion
  leaves 88 bit-exact zero in Blender and Unreal's standard StaticMesh build may
  still discard coincident-position faces. Final cooked triangle counts and the
  resulting plate detail/material appearance remain explicit build and
  real-game visual validation boundaries. The child
  construction graph hides only the inherited `MooringPlate` object and moves
  `MooringSocket` and `MooringSocket1` along their respective frame tubes;
  socket configuration, rotations and identities remain unchanged. The six mod
  objects are retained as runtime-created Blueprint component templates outside
  the unconditional SCS node list. The two rear sockets are created in both
  modes; the four derived plates are created only in four-port mode. The internal
  package stage validates those
  identities after cook and proves a byte-identical unchanged writer roundtrip
  with the exact source-cook schema overlay and no `RawExport`; it no longer
  patches native-parent attachment metadata.

Steam build `25982825` native reflection identifies
`OnSocketConnectionChanged` as `FVoyageSocketConnectionDelegate`, with runtime
signature `VoyageSocketConnectionDelegate__DelegateSignature(bool
bSocketConnection)`. Bounded native inspection found the direct connect path
broadcasting `true` and disconnect broadcasting `false`. It also found the
`ConnectionCable` setter writing that field, updating the socket, then broadcasting
the validity of `ConnectedToSocket`; cable assignment can therefore broadcast
`false`. The disabled-mode graph treats either bool value as a cleanup request but
defers cleanup to the next tick so it does not disconnect reentrantly inside a
native callback. This establishes event shape and callback ordering, not the end
of save restoration or real-game cable cleanup behavior.

## Enhanced Input value contracts

In UE `5.8.2`, `EnhancedInputLibrary.GetBoundActionValue(AActor*, const UInputAction*)`
reads the supplied actor's `EnhancedInputComponent`. The component requires an
action-value binding; an event binding alone does not populate this value.
`Conv_InputActionValueToBool` tests nonzero magnitude and accepts both boolean
and scalar action values. For the stock throttle actions, the keyboard and
gamepad mappings have no triggers or modifiers, and Enhanced Input's implicit
trigger uses the same nonzero test.

The Blueprint compiler rebuilds `DynamicBindingObjects` from compiled graph
nodes. A manually appended class binding is therefore not a persistent source
for cooking. Connected action-value accessor nodes generate the standard
`EnhancedInputActionValueBinding` entries and must remain in the source graph.
The generator uses the reflected editor class
`/Script/InputBlueprintNodes.K2Node_InputActionValueAccessor`, its `InputAction`
property and the actual getter signature. The packaged graph uses the Engine's
value bindings and library calls; these editor nodes add no runtime native API.

The UE `5.8.2` default for
`EnhancedInput.bAlwaysGetRealValueFromActionInstanceData` is enabled. Bound
values can remain active while held even when a mapping uses a Pressed trigger.
StableGyro therefore derives the reset edge explicitly rather than treating a
nonzero reset value as a pulse. It stores the previous-active flag after both
pitch writes so pure-node reevaluation cannot consume the edge prematurely.

## Mooring registration, reeling and geometry findings

The reusable socket/cable implications are summarized in
[mooring cable and socket lifecycle](../../docs/mooring-cable-and-socket-knowledge.md).
The exact Gyro producer/consumer evidence remains here.

The following native findings are bounded to Steam build `25982825`.
Module socket discovery at `0x1455468d0..0x145546c47` enumerates components on
the owning actor and assigns eligible sockets to the module. It does not cap the
list at two. `bAutoInitialize=false` does not exclude a socket: assignment at
`0x145512c10` establishes its module owner before checking that flag, which
controls initialization of port data. Preserve the stock non-standalone socket
configuration and create extra components on the Gyro itself.

The Gyro reeling pass at `0x14549ed50`, called from its native Tick, enumerates
all of the actor's socket components and processes connected mooring cables.
By contrast, the native Drop Cargo callback at `0x145486d20` uses only its cached
`MooringSocketA/B` references. This is why extending the component set is enough
for reeling, while both the action and its hint predicate need replacement.
The stock Gyro uses `DA_Socket_MooringSmall` and `DA_Cable_Hook` with a 1000 cm
integrated cable; the ship socket uses `DA_Socket_Mooring`, `ST_Output` and
`DA_Cable_Mooring` with 2000 cm. Hook Cable's attachment is `DA_Hook_Attachment`.
Do not substitute the ship actor or infer zero values from absent overrides.

Stock asset inspection establishes one `MooringPlate` component and one mesh
containing both separated sides, not an exporter merge of two components.
Its ordinary LOD0 has one material section and 3301 triangles; the decoded
Nanite leaf geometry has 13,790 faces. No triangle crosses the Y=0 split plane.
The mesh's material slot itself names `WorldGridMaterial`; the required visible
material comes from the component's `Gyrocopter_Seat_p1` override. Derived plate
components retain NoCollision and disabled overlap generation.

The source includes 114 zero-area triangles. The material chain
`Gyrocopter_Seat_p1` -> `Gyrocopte_Master_Hull` ->
`/Game/ModulesAnimations/Common/Materials/M_Modules` has a connected
WorldPositionOffset output (`PropertyConnectedMask=267747`, bit 12). Coincident
positions do not imply identical normals or UVs, so deleting those faces as
unconditionally irrelevant is not justified by their initial area alone.
This proves possible vertex deformation, not visible contribution from every
degenerate face. Extraction and splitting preserve all faces and corner data;
Unreal's ordinary mesh builder may still discard coincident-position faces.

These contracts can be revisited through the public
[asset inspection](../../tools/asset-inspection.md),
[executable inspection](../../tools/executable-inspection.md) and
[model export](../../tools/model-and-material-tools.md) entry points. The
exporter's bounded Nanite mode is documented in its
[guide](../../tools/model-export.md). Current behavior and the
scope of completed game checks are in [architecture](ARCHITECTURE.md).

## Historical native context

An earlier native snapshot established that native forward input targets
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
- Source/Voyage/VoyageModuleSocketViewComponent.h
- Source/Voyage/VoyageMiscBlueprintFunctionLibrary.h
- both generator commandlets and Build-InheritancePackage.ps1
- Validate-StableGyroMooringTemplates.ps1

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
