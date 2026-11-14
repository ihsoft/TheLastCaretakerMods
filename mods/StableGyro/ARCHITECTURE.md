# StableGyro architecture

The contracts below constrain implementation changes. Preserve the stated
ownership, lifecycle and saved identities. Changes to A/D integration, yaw,
camera behavior, possession, save behavior or other input/HUD behavior beyond
the features described here require a new user request.

The loader-free mod leaves the stock Gyro Blueprint at its original package
identity and uses a child selected through the stock item data asset's
`DroppedActor`. One helper handles pitch input before the stock physics tick;
another applies eligible altitude and horizontal stabilization after physics.
The child retains stock mappings and provided actions while adding an optional
pitch-reset action whose default key is X. Tick prerequisites are established at
BeginPlay and controller changes, not re-added every frame. The two flight helpers remain in
their respective PrePhysics and PostPhysics groups.
The vehicle HUD also shows the stock compass scale with its heading driven by
the player's camera.

## Project dependencies

The maintained project and supported commands are described in [BUILD.md](BUILD.md).
The gameplay Blueprints, settings parser and compass widgets are authored source.
Native declarations under `Project/EditorSupport/Source/Voyage/` and the shared
[item profile](../../tools/voyage-item-patch.md) provide editor dependencies,
preserving `/Script/Voyage` identities and Runtime classification for game
references. The project has no C++ modules or targets; native plugins do not ship.

Stock-parent, HUD, crafting-generator, material, input and item dependency stubs
provide editor identities and required structure, not the game's implementations
or shaders. They are excluded from the release; runtime references resolve to
the game originals. Their reconstruction and revalidation boundaries are listed
under [editor-input provenance](#editor-input-provenance).

The stock parent remains `/Game/Blueprints/Vehicles/BP_GyroCopter_Possessable`,
with native parent `/Script/Voyage.VoyageVehicleGyroCopter`. Only
`/Game/Data/Assets/Modules/DA_Item_Module_GyroCopter.DroppedActor` selects the
mod child. Do not relocate or replace the stock parent implementation.

## Graph organization

The gameplay Blueprints retain their state and tick responsibilities. Local
collapsed graphs separate coordinating flow from calculations and side effects;
functions hold repeated complete operations. The shared
[graph design rule](../../docs/blueprint-graph-design-rule.md) owns the general
editing principles.

| Blueprint | Coordinating flow and implementation boundaries |
| --- | --- |
| `ModActor` | EventGraph retains control, propulsion and contact gates, then runs horizontal control before altitude control. Nested stages separate input intent, damping, acquisition, climb tracking, braking and swept hold. Shared full resets remain distinct from vertical-only resets. |
| `PitchInputActor` | PrePhysics retains assistance ownership and descent-cap updates before pitch processing. Eligibility, capture and application have separate local graphs; native flag restoration is shared. |
| `BP_GyroCopter_StablePilot` | Construction coordinates mesh caching, rear sockets, plate creation and stock-part layout. EventGraph coordinates cargo setup, controller binding, press/hold processing and disabled-port cleanup. Provided actions retain the parent result and append the enabled mod actions. |
| `WBP_StableGyroCompassHUD` | Inherits the stock vehicle HUD; initializes and places the camera compass in its root canvas. |
| `WBP_StableGyroCameraCompass` | Creates the compass display and caches its material, degree text and owning player's camera manager once. Widget Tick updates the scale and degree text from the camera. |
| `WBP_StableGyroCompassDisplay` | Inherits the stock compass tree and disables its native tick; it has no authored graphs. |

Native-assistance eligibility and held-descent-cap eligibility are intentionally
separate: the cap does not inherit the minimum Collective or neutral-input
requirements. Their calculations stay in the original tick phase. Physics reads,
additive writes and sample updates retain their ordering; decomposition does not
introduce a shared cached eligibility result.

`PitchInputActor.UpdateAltitudeAssistance` owns the common validity gate for
`CachedGyro` and `CachedStabilizationHelper`. With both references valid, it
updates the descent cap before native altitude ownership. Without the helper,
it restores native ownership without accessing the helper; with only the helper
valid, it clears the cap before restoration. Both update functions rely on this
caller gate. Pitch processing continues after every path. EndPlay retains its
separate helper validity check.

Component arrays preserve their concrete element types through storage and
ForEach. Construction requires `VehicleMesh` and both authored rear sockets;
their absence is a broken project contract, not an optional configuration.
Cargo availability reads freshly enumerated sockets without a per-element
validity branch. Cable disconnection and deferred socket cleanup retain their
validity gates because they cross mutation or scheduling boundaries.

`ResetCargoPressState` owns the common release, cancellation and controller-change
reset. `CleanupDisabledMooringSocket` owns per-socket cleanup while its caller
keeps the queued guard across both socket calls. `RefreshCargoAvailability` scans
the sockets before appending the action and retains the enclosing action-provider
local variable. The inherited action array crosses collapsed boundaries by
reference.

## Release serialization

The release includes eleven explicitly selected packages: three gameplay
Blueprints, the settings parser, three compass widgets, the reset input action,
keyboard mapping context, Gyro item DataAsset and one plate mesh. The stock item
and mapping context keep their override identities; the child remains under
`/Game/Mods/StableGyroCtrl/`. Filesystem moves must preserve these runtime paths.

The explicit item updater compares the cooked item against a freshly extracted
original, allowing only the `Build/Item.patch.json` replacement of `DroppedActor`.
Ordinary builds check the recorded source build and recipe before reuse. Plate
geometry, representations and material bindings are validated during explicit
plate updates. Cooked socket templates are checked during packaging.
Editor dependency stubs are excluded.
Source refresh and build commands belong to [BUILD.md](BUILD.md).

`WBP_StableGyroCompassDisplay` requires a separate cook without `-unversioned`.
Its inherited `TickFrequency=Never` must be saved with property names because
the editor's native HUD mirrors are partial declarations, not the full shipping
serialization schema. The other ten packages keep the ordinary unversioned
cook. Stage each batch before starting the next cook, which clears the project
cook directory. Final-container readback with the current game mappings must
resolve the display CDO's property as `TickFrequency=EWidgetTickFrequency::Never`.
Do not change the global `CanUseUnversionedPropertySerialization` setting to
achieve this; editor object duplication also needs that capability.

## Camera compass

For Steam build `25982825` / UE `5.8.2`, stock inspection establishes the
Gyro HUD's `RootCanvasPanel`, the compass material `/Game/UI/Compass/M_Compass`
and its `Direction` scalar. The binocular overlay
`/Game/UI/Tools/BP_ToolsOverlay_Binoculars` supplies the `0.4` presentation-scale
reference, not replacement payload. Stock compass update retains a pawn
reference; `bUseControllerDirection` does not substitute the active camera manager.

The reflected parent chains are
`VoyageIngameGenericVehicleWidget -> VoyageInGameVehicleWidget ->
VoyageInGameWidget -> VoyageBaseUserWidget -> UserWidget` and
`VoyageCompassWidget -> VoyageBaseUserWidget`.
`StableGyroHudContracts.h` mirrors only the parent identities and consumed widget
references, not the complete shipping CDO layout. Recheck these dependencies when
changed game data or serialization diagnostics give reason to doubt them.

The Gyro child's construction assigns `HUDOverrideWidgetClass` to
`WBP_StableGyroCompassHUD` through `KismetSystemLibrary.SetClassPropertyByName`.
This widget inherits `/Game/UI/Game/HUD/WBP_VoyageIngameGyroCopter` and leaves its
tree, vehicle Tick and visibility handling inherited. `OnInitialized` creates
one `WBP_StableGyroCameraCompass` with the owning player and attaches it to the
inherited `RootCanvasPanel`: top-center anchors, position `(0,24)`, auto size,
top-center pivot and render scale `(0.4,0.4)`. It is hit-test-invisible and
follows the stock HUD's visibility without an additional possession poll.

The camera widget creates `WBP_StableGyroCompassDisplay`, a child of
`/Game/UI/Game/World/WBP_CompassHUD`. Its empty authored tree preserves the
stock layout and material. The display's `TickFrequency=Never` prevents the
native character-based update from competing with the camera update.
Initialization retains the `CompassImage` dynamic material, `CompassAngleText`
and the owning controller's `PlayerCameraManager`; it does not rediscover them
each frame.

Widget Tick reads `PlayerCameraManager.GetCameraRotation` and uses
`heading = ClampAxis(cameraYaw + 90)`. It writes `heading / 360` to the material's
`Direction` scalar and `min(round(heading), 359)` to the degree text. The 90-degree
offset follows the game's compass convention; normalization handles negative
yaw and the north boundary. Camera heading is independent of the Gyro's hull
rotation and flight-control settings.

Only the three mod-owned widgets ship. The stock HUD and compass remain external
parents, with editor-only stubs at their original package identities. No stock
HUD, compass or binocular asset is overridden, and the feature adds no input
mapping, flight tick, loader or INI setting.

## Altitude control

The descend action (Ctrl by default) releases height support and ordinary
vertical braking. The PrePhysics
helper captures existing upward inertia; PostPhysics caps further positive-speed
growth while preserving natural decay and unrestricted descent, including below
the configured minimum Collective. Releasing the descend action clears the cap.
Once neutral
height control becomes eligible, it selects braking from the current physical
vertical-speed sign before acquiring exact hold. Delayed threshold recovery
therefore retains descent inertia for braking. [Altitude control](ALTITUDE-CONTROL.md)
owns the formulas, eligibility and reset boundaries.

## Flight action input

The altitude helpers read the stock `IA_GyroCopterThrottleUp` and
`IA_GyroCopterThrottleDown` actions. Pitch reset reads the mod-owned
`IAV_GyroPitchReset`. Space, Ctrl and X are default mappings; runtime decisions
use the owning Gyro's Enhanced Input values and therefore follow remapped keys.
Up and Down remain independent, with Down taking precedence when both are active.

The replacement's pure `GetFlightActionValues` graph contains three standard
action-value accessors. The Blueprint compiler generates their value bindings
on the Gyro's input component, including when it recompiles during cooking.
The helpers query `EnhancedInputLibrary.GetBoundActionValue` on that Gyro and
convert each value to a nonzero boolean. They do not create their own input
components, mapping contexts or consuming action events. Cargo keeps its
separate event bindings.

### Mapping and value-binding contracts

Preserve every stock keyboard mapping in
`/Game/Game/Input/Vehicle/IMC_GyroCopter_Keyboard`. For Steam build `25982825`,
its fourteen entries cover mouse, E, T, Space, LeftControl, arrows, W/S/A/D and F;
there is no stock X mapping. Entries inherit player-mappable settings from their
actions; MouseY Negate and E/T Pressed use default parameters. The only addition
is `IAV_GyroPitchReset` on X, with Pressed and player-mappable name `GyroPitchReset`.
The stock throttle action names remain `GyroCopterThrottleUp` and
`GyroCopterThrottleDown`; gamepad RightTriggerAxis/LeftTriggerAxis use the same
actions without mapping triggers or modifiers.

The shared [value-binding contract](../../docs/voyage-input-integration-knowledge.md#action-values-and-compiler-owned-bindings)
owns actor selection, compiler persistence and held-value semantics. Keep the
three connected action-value accessors in the Gyro graph; all three value
bindings must survive cook alongside cargo event bindings. Helpers query the
Gyro, never raw keys or their own input components.

### Pitch integration and reset

W/S use native `TiltForwardInput`; A/D use `TiltInput`. The native properties are
protected and Blueprint-readable; writable editor mirrors only permit the
established assignments and do not claim different game access flags. Stock
tilt limits `-15..15` and interpolation speed `5` remain owned by the stock parent.

When pitch correction is enabled, accept only exact native digital markers
`-1`, `0`, `1`: W increases integrated pitch, S decreases it, and neutral holds
it. Clamp to `-0.9999..0.9999`. Reset writes exact zero once per press.
The helper ticks in `TG_PrePhysics`, after the controller and before the stock
Gyro. Establish prerequisites at BeginPlay/controller change, not per frame.
Restore integrated pitch before every controlled Gyro tick, including neutral
input; reset helper and native pitch when the owner is not player-controlled.

Derive the reset rising edge through transient `ResetActionWasActive`; update
the latch only after both pitch writes, so pure-node reevaluation cannot consume
the edge prematurely. Clear it on invalid ownership, disabled/uncontrolled paths,
controller changes and EndPlay.

With `EnablePitchCorrection=false`, the pitch helper must neither read nor write
native `TiltForwardInput` on any tick or lifecycle path and must not append the
reset hint. Inherited actions pass through. The independent horizontal helper
may still read both native tilt axes as control intent. `GetProvidedActionsBP`
must call the inherited provider and append reset only while pitch is enabled.

### Horizontal stabilization

`ModActor` applies local two-axis control-intent damping in `TG_PostPhysics`,
explicitly after the stock Gyro tick and rotor/Chaos integration. Away from
external contact, neutral control rejects physics-injected horizontal speed
growth before decay. Apply only an additive XY velocity delta; never rewrite
the full velocity vector or its Z component. Preserve the reusable
[PostPhysics correction contract](../../docs/vehicle-and-hud-modding-patterns-knowledge.md#post-physics-kinematic-correction).
The shared propulsion/contact gates and their reset order are owned by
[altitude control](ALTITUDE-CONTROL.md#shipped-ownership-and-restoration).

## Independent flight-control settings

`EnableAltitudeControl` and `EnableHorizontalVelocityControl` both default to
true and are reread by the PostPhysics `ModActor` when the player enters a
Gyro. They are independent of each other and of `EnablePitchCorrection`.

When altitude control is false, PostPhysics clears all vertical lock, braking
and sample state (including exact hold, Space floor, both braking modes,
locked altitude and Ctrl cap) and performs no mod-owned Z position or velocity
correction. The altitude gate follows the independent horizontal path.
The player-entry callback also leaves saved `HoverPitchScale` untouched. The
PrePhysics helper reads the altitude flag through its cached `ModActor`
reference, declines native altitude-assistance ownership, and restores any
captured `bAutoHoverCollective` and `bAltitudeAssist` values. W/S pitch
integration and eligible horizontal damping remain available.

The enabled player-entry `HoverPitchScale` reset, its range and lifecycle are
owned by [entry reset](ALTITUDE-CONTROL.md#entry-reset-and-remaining-limits).

When horizontal velocity control is false, PostPhysics branches before reading
or writing horizontal physics velocity, clears its prior XY sample and validity flag, and
continues into the independent altitude path. Altitude control and W/S pitch
integration remain available.

## Settings parser

`Project/Content/Mods/StableGyro/Settings/BP_IniFileParser.uasset` is the mod-owned copy
of the reusable [BPT INI parser](../../tools/BlueprintText/Library/IniFileParser/AGENTS.md).
Each consumer's `ReloadSettings` constructs a parser with itself as Outer,
retains it in transient `IniParser`, and calls `Load` with
`ProjectContentDir() + "Paks/StableGyro.ini"`. It then assigns each setting once
from `GetDouble` or `GetBool`, supplying the schema fallback and numeric bounds.
There is no separate reset-before-read chain or duplicated parsing helper graph
in the consumers. Missing or invalid input assigns the getter's fallback.

The canonical INI owns distributed values, comments, ordering and formatting;
`Settings/StableGyro.settings.json` owns bindings, runtime fallbacks and ranges.
Out-of-range values use the fallback rather than clamping. Getter members,
arguments, fallbacks and consumers must match that schema.

| Consumer | Settings ownership and read lifetime |
| --- | --- |
| `ModActor` | Twelve PostPhysics settings, including both independent flight-control flags. Reload at BeginPlay and synchronously on genuine player entry through the Pawn controller-change delegate. |
| `PitchInputActor` | Owns only its `PitchRampSeconds` and `EnablePitchCorrection` members; same reload events. Read `EnableAltitudeControl` through the cached sibling, without duplicating that setting. |
| Replacement Gyro child | Alone owns `EnableFourMooringPorts`, using the exact replacement-class CDO snapshot before parent construction. All layout, cargo and action branches share it; helper loaders exclude it. See [four-port lifecycle](#four-mooring-ports). |

Each consumer constructs and retains its own parser snapshot; consuming graphs
read their own helper members, except the child's explicit CDO contract.
No parser reads occur on Tick. The parser ships as an ordinary mod asset,
without a separate native parser module. For Steam build `25982825`, the consumed
game API is `VoyageEditorBlueprintFunctionLibrary.LoadFileToArray(FString)`,
returning a string array. This integration is game-validated; parser grammar,
inline comments and getter edge cases belong to the
[shared INI library](../../tools/BlueprintText/Library/IniFileParser/integration.md).

## Fuel settings

The PostPhysics `ModActor` owns both fuel settings in the INI:

| Setting | Default and meaning | Inclusive range |
| --- | --- | --- |
| `FuelCapacity` | `400` native resource units of Petrol | `0.001..1000000000` |
| `FuelConsumption` | `250`, the stock native `PetrolConsumption` parameter | `0..1000000` |

Missing or invalid values use these defaults, which preserve the stock tank
capacity and base engine consumption. `FuelConsumption` is passed directly to
the native property without a time-unit conversion. It is not a scale factor
or a fixed number of liters per real minute. The native request is multiplied
by `CurrentThrottle`, not the displayed collective percentage; module processing
determines the actual tank decrease. Zero disables this base Petrol request.
The runtime reads `FuelConsumption`; the installed INI is a complete copy from
the selected package, without conversion from older keys.

After reloading settings, `ApplyFuelSettings` applies them at helper BeginPlay
and on genuine player entry, before the altitude-control branch. Editing the
installed INI therefore takes effect on the next entry. Fuel settings are
independent of pitch correction, altitude control, horizontal damping and the
four-port option. There is no per-tick settings read or fuel write.

The function updates only the owned custom module's Petrol capacity entry and
the Gyro's base consumption property. It neither refills the tank nor removes
stored fuel when capacity is lowered; other resource entries remain intact. Do not modify throttle
or workload consumption from this path. `VoyageModuleComponent.h` and
`VoyageCustomModuleComponent.h` provide editor-only identities for lookup, cast
and the external `MaxResources` getter; they are not ordinal-complete schemas
for authoring native component instances or defaults.
The native defaults, field identities and rate conversion are documented in
[altitude control](ALTITUDE-CONTROL.md#fuel-request-versus-observed-consumption).

## Four mooring ports

`EnableFourMooringPorts=true` adds two Hook Cable ports, giving four usable
connections. It defaults to true and requires a game restart after changes.
The replacement-class CDO reads that setting on first use and caches it for
the lifetime of the loaded class;
class unload and reload may read the file again. The child reads the snapshot
before its parent construction call. Both modes create the two rear socket
components from the same retained Blueprint component templates and attach them
to the native `VehicleMesh` before registration, preserving their names and
socket identities for saved connections. Four-port mode then creates the four
derived plates, runs the parent construction graph, and applies the established
four-port stock-part layout. Stock mode runs the parent construction graph with
inherited stock parts untouched. Item-data class selection applies to newly
created Gyros; it does not migrate saved stock actors into the mod subclass.

### Mooring native contracts

These native locations and stock identities are bounded to Steam build `25982825`.
Socket discovery at `0x1455468d0..0x145546c47` enumerates actor components without
a two-socket limit. Module-owner assignment at `0x145512c10` precedes the
`bAutoInitialize` check; false skips port-data initialization, not discovery.
The native reeling pass at `0x14549ed50` also enumerates actor socket components,
whereas Drop Cargo at `0x145486d20` uses cached `MooringSocketA/B`.

Preserve the Gyro's non-standalone socket configuration: `DA_Socket_MooringSmall`,
`ST_Any`, Mooring resource/category, `DA_Cable_Hook`, integrated length `1000 cm`,
disabled auto-initialization and absolute scale. Stock IDs are `1051774499` and
`3437279300`; added rear IDs are `2056977161` and `2056977162`. Create extra
components on the Gyro itself, with stable names and IDs in both setting modes.
The ship's `DA_Socket_Mooring` / `ST_Output` / `DA_Cable_Mooring` and `2000 cm`
configuration is a different consumer. Hook Cable uses `DA_Hook_Attachment`.
Do not substitute the ship actor or infer zero from absent overrides.

The reflected `MeshComponent` property points to the runtime UObject named
`VehicleMesh`. Component-name searches must use that UObject name. Stock instance
transforms and the authored rear templates remain in the child asset/BPT; disabled
four-port mode must leave the inherited transforms untouched.

`OnSocketConnectionChanged` has signature
`VoyageSocketConnectionDelegate__DelegateSignature(bool bSocketConnection)`.
Connect/disconnect broadcast true/false; the `ConnectionCable` setter also
broadcasts peer validity after updating the socket, so cable assignment can
broadcast false. Treat either value as a cleanup request and defer outside the
callback; it does not signal completion of save restoration. The general
[socket lifecycle](../../docs/mooring-cable-and-socket-knowledge.md) owns reusable
event/disconnect guidance; the [disabled-port path](#saved-connections-when-disabling-the-extra-pair)
owns this consumer's implementation.

### Port and base layout

Ports are `VoyageModuleSocketViewComponent` components attached to the native
`VehicleMesh`; placement does not require named static-mesh sockets. Copy the
Gyro's small mooring socket and Hook Cable configuration, rather than the ship's
Mooring Cable actor. Native socket registration and cable reeling enumerate the
actor's socket components, so the extra pair needs no separate winch loop.

The stock `MooringPlate` is a separate decorative component whose mesh contains
both sides together. It is not part of the frame mesh or two selectable material
sections. The separate updater splits its full decoded Nanite leaf geometry and
retains the positive-Y half as one complete editor mesh, preserving the original
pivot. The other half is temporary extraction evidence only. Four components reuse
the maintained mesh with the stock component material override and no collision.
The updater delegates mesh import to the shared
[StaticMesh tools](../../tools/static-mesh-assets.md) with `-OutputKind Uncooked`,
then replaces the complete package in the maintained project. The retained name
`StableGyroMooringPlatePositiveY` identifies the source half, not an instance side.
The mod's project cook supplies the runtime mesh. It uses native
Nanite with all input triangles retained, zero trim error, position precision
`4`, normal precision `8`, and implicit tangents. A separate 25% triangle
fallback supplies the ray-tracing proxy. This reduces that representation's
geometry budget without reducing the full-detail Nanite input.
The inherited paired plate is hidden only in four-port mode.

Preserve source corner normals, UV0..3, vertex colors and material bindings when
splitting the full decoded Nanite geometry. Do not delete zero-area faces or
invent source tangents. Blender can change imported normals near degenerate
geometry; the bounded splitter restores the raw `NORMAL` accessor by complete
triangle/corner position+UV identity after export. The source has no tangent
stream; the importer retains normals, while the native builders derive the
required fallback tangent basis. The stock mesh slot names `WorldGridMaterial`;
visible `Gyrocopter_Seat_p1` comes from the component override. Derived components
retain NoCollision and disabled overlap generation.

The material chain `Gyrocopter_Seat_p1 -> Gyrocopte_Master_Hull ->
/Game/ModulesAnimations/Common/Materials/M_Modules` has a connected
WorldPositionOffset output. Coincident positions need not have identical corner
attributes, so initial zero area is not permission to discard a face. This is
not evidence that every degenerate face contributes visibly. Native mesh builders
can discard coincident-position faces; the explicit updater must still prove
that no nondegenerate source face is lost. Exact source counts, decode assertions
and comparison tolerances belong to `Update-StableGyroMooringPlate.ps1`,
`Build/Split-MooringPlateGlb.py` and `Build/Validate-MooringPlateUpdate.ps1`.
Geometry/representation checks do not establish GPU cost or which representation
is used in an individual frame.

The four decorative components share one `StableGyroMooringPlate` template for
the mesh, material and collision settings. `CreateMooringPlates` calls
`CreateMooringPlate` four times with the required location and scale. Two use
`(1,1,1)` and the opposite pair use `(1,-1,1)` to mirror across local Y while
retaining their instance translations. The function creates the component with
its template mesh, attaches it to `VehicleMesh`
and finishes deferred creation. Its callers do not gate later plates on a
completion flag; failure of the internal cast ends that invocation. Decorative
instance names are generated uniquely from the common template. The two rear
socket templates remain separate with their stable endpoint names and SocketIDs.
Cook validation requires exactly those two socket templates and the shared plate.
It also verifies that the plate template references the packaged mesh identity.
Keep these runtime-created templates outside the unconditional SCS node list.
`Build/Validate-StableGyroMooringTemplates.ps1` validates with the exact source-cook
schema overlay, no `RawExport` and a byte-identical unchanged writer roundtrip;
it must not patch native-parent attachment metadata during packaging.

Replacement keeps package identities and Blueprint references, with a backup
of changed source files; it does not merge previous LOD, collision or socket data.
The project requires DX12/SM6 and ray tracing enabled to retain the requested
representations during cook. Preserve every emitted bulk companion. During an
explicit plate update, validate Nanite, fallback, ray tracing, the stock material
reference and correspondence to source geometry and attributes. Ordinary builds
reuse the accepted mesh without repeating these checks.
The shared
[cooking knowledge](../../docs/static-mesh-cooking-knowledge.md) defines the
representation and validation boundaries.

Sockets and bases move together along the frame tubes: the original pair moves
5 cm forward and the added pair sits 40 cm behind the original positions. The
positive-Y tube direction is approximately
`(0.66015035, -0.72922620, 0.18008518)`, with Y mirrored on the other side.
This direction comes from frame geometry; a port normal alone cannot determine
it. Rotations and scales remain unchanged. Exact transforms and stable SocketIDs
are stored in the maintained child Blueprint's component templates and
construction graph, represented in
`Project/BlueprintText/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot.json`.

### Drop Cargo and hints

For Steam build `25982825`, the stock Gyro Blueprint has no function override.
Its native action provider calls `VoyageVehiclePawn.GetProvidedActionsBP` and
then appends cargo when `DropCargoAction` is non-null. The child preserves parent
`ReceiveBeginPlay`; only in four-port mode it clears that property with
`KismetSystemLibrary.SetObjectPropertyByName` and owns one replacement binding
and provided-action record for `/Game/Game/Input/Vehicle/IA_GyroCopterDropCargo`.
The native float `DropCargoTriggerTime` belongs to `VoyageVehicleGyroCopter`
at offset `0x72c`, defaults to `1.0`, and is copied directly into the hint's
`TriggerTime`. The mirror exposes it for reads, never a serialized override.

Started arms a fresh press; Triggered checks elapsed time and releases once;
Completed, Canceled and controller change clear press state. Availability uses
`ModuleSocketComponent.IsSocketConnectedOrHasCable()` on Mooring ports, and
release calls `VoyageMiscBlueprintFunctionLibrary.DisconnectCableFromSocket(Socket, false)`.

Native reeling covers the extra sockets, but the stock Drop Cargo callback and
availability test use only the original pair. Four-port mode reuses the stock
Enhanced Input action and supplies one replacement binding and hint. It scans
Mooring socket components, uses the native connection predicate for availability,
and disconnects through the stock helper with `bDestroy=false`. Inherited
non-cargo actions remain available.

A short press does nothing. Holding F releases all connected mooring cables
when elapsed input time reaches the live native `DropCargoTriggerTime` (stock
default 1 second), once per press. The hint uses the same threshold. Release,
cancellation and controller changes clear the transient press state. Disabled
mode retains the native two-port action and hint.

### Saved connections when disabling the extra pair

With four-port mode disabled, BeginPlay binds each extra socket's
native connection event and queues an initial next-tick cleanup. Every later
connection event queues the same coalesced cleanup regardless of its bool payload,
avoiding synchronous disconnect inside the native callback. Cleanup keeps its
coalescing guard set through both sockets, uses the stock non-destroying helper,
hides and deactivates only components owned by this Gyro, and disables their
collision and overlap interaction. Each socket's descendant array is captured
once before traversal. Explicit socket visual references are also handled with
the same owner check; cable and remote endpoint objects are not traversed for
visual cleanup. Bindings remain active for connections restored later in loading.

The mooring path has no Tick, polling, repeating timer or arbitrary loading
delay. It uses one coalesced next-tick callback. The queued guard stays set until
both sockets are processed, preventing synchronous events from its own disconnects
from rescheduling cleanup. A later external event can queue another pass.

Keeping endpoints registered is part of save compatibility. Omitting them when
the option is false can leave saved cable references unresolved, delaying load
and leaving inaccessible attachment pads. Hiding or deactivating an endpoint
alone does not disconnect its saved cable; native disconnect must run first.

## Validation boundary

On Steam build `25982825`, game checks establish compass show/hide behavior
with the Gyro HUD and correct heading updates from the camera. The separate
display cook preserves the required inherited tick setting in the final
container; compilation alone does not establish that serialization contract.

On Steam build `25982825`, game checks cover four simultaneous connections,
the expanded hint and hold-to-release action, base placement and restored mesh
detail, and loading saved extra connections with the option disabled. That path
disconnects extra cables and leaves the extra sockets hidden and unusable without
the missing-endpoint loading delay. The removal of redundant Tick setup and
component traversal also passed a game check.

The fuel-capacity setting and the corrected direct `FuelConsumption=250.0`
default have passed game checks. This does not establish a fixed real-time
liters-per-minute conversion or explain every previously observed tank change.

These checks do not establish repair of orphan attachment pads left by older
packages, migration of saved stock Gyros, or compatibility after removing the
mod. Fixtures and cooked readback validate graph structure and component identities;
changed game versions still require game tests of restoration and disconnect.

The detailed native/altitude applicability is in
[ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md#compatibility-boundary).
Required acceptance scenarios are in [BUILD.md](BUILD.md#gameplay-validation).

## Editor-input provenance

The editor contracts below were inspected for Steam build `25982825`, UE `5.8.2`
(parser target `5.8`). Current source-build selection is owned by `game-build.json`
and the per-input source-state records. Mapping resolution alone does not
revalidate a stub's implementation or CDO layout. Recheck affected contracts when
changed stock data, observed behavior or tool diagnostics gives reason to doubt
them; retain the explicit older evidence boundaries in
[native field ownership](ALTITUDE-CONTROL.md#reflected-state-and-setter-route).

| Maintained inputs | Origin and role | Update or revalidation owner |
| --- | --- | --- |
| `Project/Content/Mods/StableGyro/Settings/BP_IniFileParser.uasset` | Authored shared INI library relocated into the mod, with synchronized BPT; shipped source. | Shared library relocation/BPT workflow; consumer lifetime remains [local](#settings-parser). |
| `Project/Content/Mods/StableGyro/ModActor.uasset`, `PitchInputActor.uasset`, and `Project/Content/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot.uasset` | Authored gameplay graphs using the inspected native contracts; synchronized BPT mirrors virtual paths. | BPT authoring; recheck consumed APIs on evidence of a contract change, never regenerate during build. |
| `Project/Content/Mods/StableGyro/UI/` | Three authored widgets inheriting stock parents and camera conventions. | [Compass dependencies](#camera-compass) and [serialization](#release-serialization). |
| `Project/Content/UI/Game/HUD/WBP_VoyageIngameGyroCopter.uasset`, `Project/Content/UI/Game/World/WBP_CompassHUD.uasset` and their BPT | Hand-reconstructed editor-only identities and required widget members, without stock implementations. | Check parent chains, names and consumed native fields when affected; exclude both from payload. |
| `Project/Content/Mods/StableGyro/IAV_GyroPitchReset.uasset`, `Project/Content/Game/Input/Vehicle/IMC_GyroCopter_Keyboard.uasset` | Authored reset action and stock keyboard-context reconstruction with that one addition. | [Mapping contract](#mapping-and-value-binding-contracts). |
| `Project/Content/Mods/StableGyro/StableGyroMooringPlatePositiveY/` | Positive-Y half derived from the stock Gyro plate through the mod splitter and shared StaticMesh producer. | `Build/MooringMeshes.source.json` owns source build/recipe/readiness; explicit [plate update](BUILD.md#source-updates). |
| `Project/Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset` and `Project/Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset` | Shared VoyageItem profile reconstruction and its owned generator stub; `Build/Item.patch.json` changes only `DroppedActor`. Only the item ships. | `Build/Item.source.json` owns applied build/recipe/readiness; explicit [item update](BUILD.md#source-updates), with current-stock comparison during that update. |
| Other `Project/Content/` dependencies | Reconstructed stock Gyro parent, material, icon, actions, item references and string-table editor stubs. No stock shaders or implementations are claimed. | Review required identities/layouts when affected; exclude from payload. Missing dependencies created by the shared item tool require separate review before source acceptance. |
| `Project/EditorSupport/Source/Voyage/` | Minimal native API mirrors from reflection/mappings; shared item-profile overlays declared by `Project/EditorSupport.json`. | Preserve `/Script/Voyage` identity, field order and consumed metadata. Rebuild ready support on source changes; never ship its binaries. |
