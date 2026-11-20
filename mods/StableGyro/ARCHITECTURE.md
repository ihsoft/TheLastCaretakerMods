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

The saved item is the authority for its state. Export its full DataAsset state
through BPT into a temporary workspace for editing and review.
Use [item authoring](BUILD.md#item-authoring) for ordinary
edits. `Build/Item.patch.json` retains the intended stock-to-mod replacement for
explicit game-data refresh; it is not applied by ordinary builds.

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
| `WBP_StableGyroCompassHUD` | Inherits the stock vehicle HUD; initializes the camera compass, minimap and one horizontal map-action row in its root canvas. `PlaceMapHints` places the owned row. |
| `WBP_StableGyroMapHints` | Owns an auto-sized HorizontalBox; initializes one proven single-action stock container per hint, with Auto child slots and no additional outer padding. |
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

The release inventory in `Build-StableGyro.ps1` explicitly selects the gameplay
Blueprints, settings parser, compass and map UI, map overlay data and composite
material, reset input action, keyboard mapping context, Gyro item DataAsset
and one plate mesh. The stock item
and mapping context keep their override identities; the child remains under
`/Game/Mods/StableGyroCtrl/`. Filesystem moves must preserve these runtime paths.

The explicit item updater compares the cooked item against a freshly extracted
original, allowing only the `Build/Item.patch.json` replacement of `DroppedActor`.
Ordinary builds check the recorded source build before reuse; recipe changes
must be applied explicitly by their author. Plate
extraction, splitting and import are checked during explicit plate updates.
The shared importer reopens the saved mesh and resolves its material bindings.
Editor dependency stubs are excluded.
Source refresh and build commands belong to [BUILD.md](BUILD.md).

`WBP_StableGyroCompassDisplay`, `WBP_StableGyroMinimapDisplay`,
`WBP_StableGyroMapScreen` and `DA_StableGyroMap` use tagged properties.
Their inherited overrides must be saved with property names because
the editor's native HUD mirrors are partial declarations, not the full shipping
serialization schema. Other packages use the ordinary unversioned cook.
The shared producer owns separate staging of these batches. Final-container
readback with the current game mappings must
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

Only the three mod-owned compass widgets ship for this feature. The stock HUD and compass remain external
parents, with editor-only stubs at their original package identities. No stock
HUD, compass or binocular asset is overridden, and the feature adds no input
mapping, flight tick, loader or INI setting.

## Minimap and interactive map

`WBP_StableGyroCompassHUD.OnInitialized` calls `ReloadMapSettings` once before
creating its map children. It passes coverage and opacity to the minimap before
Slate attachment. `MinimapOpacity` defaults to 0.6; zero disables the minimap
for that HUD lifetime by skipping its creation, rather than rendering it invisible.
The HUD still creates the Open Map hint and supports the interactive map.

`WBP_StableGyroMinimap` owns the passive view in the top-right gyro HUD. Its
ordinary UUserWidget Tick uses a cached owning pawn and map reference to write
the pawn's world XY through `VoyageMinimapWidget.SetCenterLocation`. The pawn is
checked for validity; no per-frame object search, file read, forced Tick flag
or diagnostic UI is needed. The display inherits the stock `BP_MinimapWidget`
tree and its native rendering behavior, keeping the full-map locator mode.
`TypeExcludeFilters` is empty so the native Player locator and icon remain visible.
Transparent, zero-thickness PathLine and ForwardLine settings suppress ship
trajectories and course lines. `ClipToBoundsAlways` confines other map drawing.

The map's native world dimensions are 1,000,000 cm per side. Its projected world
coverage is dimensions multiplied by zoom, so `MinimapSizeKm / 10` is the native
zoom: 6 means a 6 x 6 km square, approximately 3 km to each edge. The native zoom
range .01..1 bounds the supported setting to .1..10 km. The INI owns shipped
values and comments; the settings schema owns bounds and fallback values.

The stock map's inner RetainerBox uses an opaque terminal material; changing
outer widget RenderOpacity alone does not uniformly fade its background.
The owned outer RetainerBox composites the complete map with
`M_StableGyroMinimapComposite`, a UI AlphaComposite material. It multiplies both
sampled RGB and alpha by `MapOpacity`, with texture parameter `MapTexture`.
Construct applies the supplied zoom, obtains the effect material after Slate
creation and sets opacity; the minimap does not read the INI itself.
Tick does not update the material. The material must ship with inline shader
code as described in [BUILD.md](BUILD.md#candidate-build).

The third action opens `DA_StableGyroMap` through
`VoyagePlayerController.ShowOverlayWidget` in native Overlay input mode.
`WBP_StableGyroMapScreen` inherits the complete stock
`BP_VoyageMapOverlayWidget`, including its `ScreenWidget`, action hints,
cardinal directions, cursor coordinates/distance and marker controls.
Initialization supplies the owning pawn as Source, sets center once and starts
at zoom .6. It does not continuously recenter, allowing native pan/zoom and
marker interaction. Full-screen opacity is independent of the minimap setting.
The stock overlay handles Escape and restores gameplay input/HUD on close.
V-close is not added: the vehicle input context is inactive during Overlay mode.
No stock map package is overridden and no additional gamepad mapping is claimed.

## Map action hints and minimap visibility

The gyro HUD creates the owned `WBP_StableGyroMapHints` with its owning player.
`PlaceMapHints` attaches it to inherited `RootCanvasPanel`, bottom-center at
`(200,-16)`, alignment `(0,1)`, CanvasSlot auto-size and hit-test-invisible.

The owned row has an ordinary UMG HorizontalBox root. HUD setup explicitly calls
`InitializeHints(EnableMinimap)` once. It creates Toggle minimap only when the
effective opacity is positive, and always creates Open Map. Each hint uses a
stock `BP_ButtonInfo_Vertical_Container` instance with the owning player. For
each it assigns stock `BP_ButtonInfoDetailed_Vertical` through
`SetClassPropertyByName`, then calls `VoyageButtonInfoContainerWidget.SetActions`
with `DA_Input_Context_GyroCopter`, that card's non-null InputAction, one explicit
matching override record, and bRecreateWidget=true: Inspect / Toggle minimap, then InteractThree /
Open Map. The stock cards retain key glyphs, native mapping updates and internal
centering. The row's own TickFrequency=Never does not change the child widgets.

Each created container is added with HorizontalBox.AddChildToHorizontalBox.
HorizontalBoxSlot defaults to Automatic size (verified in UE 5.8 source);
there are no Fill weights or manually calculated card widths. The first slot
uses default zero padding, as does the second. The stock card's root content
already has 2 logical pixels of padding on each side. A wider label or remapped
key changes the row's desired width and moves the next card rather than using
an independent canvas X position. The HUD owns overlay visibility and cleanup;
no polling, Tick or reattachment is added.

Stock Steam 25982825 `WBP_VoyageIngameGyroCopter` positions its bottom control row
at `(16,-16)`, bottom-left aligned with CanvasSlot auto-size, and
`BP_DynamicPlayerInputHorizontalWidget.ContextInputActionsRoot` is a HorizontalBox.
This supplies the layout pattern, not a reason to replace the working input
container contract.

`GetProvidedActionsBP` does not append these two map records to the vehicle's
general action row. The existing third-action Started binding opens the
full-screen map even when the minimap is hidden or disabled in the INI.
`ToggleMinimap` checks the optional minimap reference before setting its visibility
to `Collapsed` or `HitTestInvisible`. With zero opacity there is no widget to
toggle, so Inspect cannot restore it. Ordinary toggling does not change either
hint's visibility. The transient visibility choice resets when the HUD is
recreated and is not persisted to the INI.

The stock global `BP_PlayingWidget.PawnOverlay` owns health and battery. Its adjacent
`ActionRoot` is occupied by experience, consumption and combat notifications, not
an empty vehicle action row. The map-action row therefore uses the gyro root canvas
beside that block; it does not modify the global widget. Check spacing against
those notifications and UI scaling in game.

The possessed gyro handles `IAV_Inspect.Triggered`. Its keyboard mapping uses
the stock Inspect default G with `InputTriggerPressed` and inherited player
settings, so holding the key does not repeatedly toggle and remapping remains
owned by Inspect. On that event only, the gyro obtains its controller's
`GetPlayerFocusWidget`, casts it to `WBP_StableGyroCompassHUD` and calls
`ToggleMinimap`; absent/different focus widgets terminate at the cast. There is
no Tick lookup. The native function declaration is the reviewed SmartOutlet
`VoyagePlayerController.GetPlayerFocusWidget` contract (Steam 25982825), returning
`VoyageBaseUserWidget`; the user confirmed the gyro-specific dispatch in game.

`StableGyroHintContracts.h` supplies the native identities and SetActions
signature reviewed for SmartOutlet (Steam 25982825). Editor-only
assets for the card, containers and gyro context are excluded from the
release inventory; the game supplies their implementations. Recheck their
native parents, signature and named class property if game data changes.

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
actions; MouseY Negate and E/T Pressed use default parameters. Additions are
`IAV_GyroPitchReset` on X (Pressed, player-mappable name `GyroPitchReset`),
`IAV_InteractThree` on V for the full-screen map, and stock `IAV_Inspect` on G
(Pressed, inherited player-mappable name `Inspect`) for minimap visibility.
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
Each consumer's settings loader constructs a parser with itself as Outer,
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
| `WBP_StableGyroCompassHUD` | Owns `MinimapSizeKm` and `MinimapOpacity`; `ReloadMapSettings` runs once at OnInitialized, before optional minimap creation and hint initialization. Children consume the supplied values. Recreate the HUD/restart after settings edits. |

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
transforms and the authored rear templates remain in the child asset; disabled
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
sections. The separate updater selects whole faces from its full decoded Nanite
leaf geometry through the [shared GLB selector](../../tools/glb-tools.md#select-whole-triangles):
positive Unreal Y, corresponding to mesh-local glTF positive Z. It retains that
half as one complete editor mesh, preserving the original pivot. Four components reuse
the maintained mesh with the stock component material override and no collision.
The updater delegates mesh import to the shared
[StaticMesh tools](../../tools/static-mesh-assets.md) with `-OutputKind Uncooked -ProjectFile <working-project>`,
then accepts the complete package in the maintained project. The retained name
`StableGyroMooringPlatePositiveY` identifies the source half, not an instance side.
The mod's project cook supplies the runtime mesh. It uses native
Nanite with all input triangles retained, zero trim error, position precision
`4`, normal precision `8`, and implicit tangents. A separate 25% triangle
fallback supplies the ray-tracing proxy. This reduces that representation's
geometry budget without reducing the full-detail Nanite input.
The inherited paired plate is hidden only in four-port mode.

Preserve source corner normals, UV0..3, vertex colors and material bindings when
splitting the full decoded Nanite geometry. Do not delete zero-area faces or
invent source tangents. The shared selector copies the selected corner attributes
directly, without a Blender roundtrip or normal recalculation. The source has no tangent
stream; the importer retains normals, while the native builders derive the
required fallback tangent basis. The stock mesh slot names `WorldGridMaterial`;
visible `Gyrocopter_Seat_p1` comes from the component override. Derived components
retain NoCollision and disabled overlap generation.

The material chain `Gyrocopter_Seat_p1 -> Gyrocopte_Master_Hull ->
/Game/ModulesAnimations/Common/Materials/M_Modules` has a connected
WorldPositionOffset output. Coincident positions need not have identical corner
attributes, so initial zero area is not permission to discard a face. This is
not evidence that every degenerate face contributes visibly. Native mesh builders
can discard coincident-position faces. The selector preserves attributes while
separating the disconnected halves; the shared importer owns basic package and
material checks. No fixed triangle counts or cooked-to-GLB comparison are part
of source acceptance. These checks do not establish GPU cost or which
representation is used in an individual frame.

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
The plate template must reference the maintained mesh identity described above.
Keep these runtime-created templates outside the unconditional SCS node list:
SCS would create them even when the corresponding option is disabled. Retain
them as component templates owned by stock `K2Node_AddComponent` nodes so
ordinary editor compilation retains them and the deferred creation functions
can copy the authored mesh, material, collision and socket settings at runtime.

Replacement keeps package identities and Blueprint references, with a backup
of changed source files; it does not merge previous LOD, collision or socket data.
The project requires DX12/SM6 and ray tracing enabled to retain the requested
representations during cook. Preserve every emitted bulk companion. Ordinary
builds cook the accepted editor mesh without a separate representation validator.
The shared [cooking knowledge](../../docs/static-mesh-cooking-knowledge.md)
describes representation boundaries when a rendering investigation needs them.

Sockets and bases move together along the frame tubes: the original pair moves
5 cm forward and the added pair sits 40 cm behind the original positions. The
positive-Y tube direction is approximately
`(0.66015035, -0.72922620, 0.18008518)`, with Y mirrored on the other side.
This direction comes from frame geometry; a port normal alone cannot determine
it. Rotations and scales remain unchanged. Exact transforms and stable SocketIDs
are stored in the maintained child Blueprint's component templates and
construction graph in
`Project/Content/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot.uasset`.

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

Game checks also establish player-centered minimap tracking, POIs/player marker,
coverage, composite background/icon opacity, native full-screen interaction and
compact map-action hints. With zero opacity the minimap and Toggle hint are absent
while Open Map remains available. These checks do not establish every remapping,
gamepad, UI-scale or save/load combination; the affected scenarios remain in
[gameplay validation](BUILD.md#gameplay-validation).

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
mod. Structural inspection alone does not establish runtime restoration and
disconnect behavior; those changes require the affected game checks.

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
| `Project/Content/Mods/StableGyro/Settings/BP_IniFileParser.uasset` | Native copy of the authored shared INI library in the mod's namespace; shipped source. | Shared library CopyAsset/BPT workflow; consumer lifetime remains [local](#settings-parser). |
| `Project/Content/Mods/StableGyro/ModActor.uasset`, `PitchInputActor.uasset`, and `Project/Content/Mods/StableGyroCtrl/BP_GyroCopter_StablePilot.uasset` | Authored gameplay graphs using the inspected native contracts. | BPT authoring; recheck consumed APIs on evidence of a contract change, never regenerate during build. |
| `Project/Content/Mods/StableGyro/UI/` | Authored compass/map widgets, owned overlay data and UI composite material. | [Compass](#camera-compass), [map contracts](#minimap-and-interactive-map) and [serialization](#release-serialization). |
| `Project/Content/UI/Game/HUD/WBP_VoyageIngameGyroCopter.uasset`, `Project/Content/UI/Game/World/WBP_CompassHUD.uasset` | Hand-reconstructed editor-only identities and required widget members, without stock implementations. | Check parent chains, names and consumed native fields when affected; exclude both from payload. |
| `Project/Content/Mods/StableGyro/IAV_GyroPitchReset.uasset`, `Project/Content/Game/Input/Vehicle/IMC_GyroCopter_Keyboard.uasset` | Authored reset action and stock keyboard-context reconstruction with reset, map and minimap-toggle additions. | [Mapping contract](#mapping-and-value-binding-contracts). |
| `Project/Content/Game/Input/Character/IAV_Inspect.uasset` | Editor-only `VoyageInputAction` identity created with the native DataAsset factory. Stock Steam 25982825 action and character keyboard mapping establish Inspect/G, Pressed and inherited settings; the game supplies the action properties. | Exclude the stub from the release; recheck identity and mapping semantics when affected. |
| `Project/Content/Game/Input/Character/IAV_InteractThree.uasset`, `Game/Input/Vehicle/DA_Input_Context_GyroCopter.uasset`, `UI/Game/Inputs/`, `UI/Game/Locator/`, `UI/Screen/` | Editor-only identities for stock map parents, input context, action and hint cards; native map and input behavior remains game-owned. | Recheck consumed parents, fields and API signatures when affected; exclude these stubs from the release inventory. |
| `Project/Content/Mods/StableGyro/StableGyroMooringPlatePositiveY/` | Positive-Y half derived from the stock Gyro plate through shared GLB selection and StaticMesh import, with the mod's side, material and representation settings. | `Build/MooringMeshes.source.json` records the source game build; explicit [plate update](BUILD.md#source-updates). |
| `Project/Content/Data/Assets/Modules/DA_Item_Module_GyroCopter.uasset` and `Project/Content/Blueprints/Fabrication/BP_CraftingLocationGenerator_Default.uasset` | Shared VoyageItem profile reconstruction and its owned generator stub; temporary BPT exports expose the item's properties and owned-object state. `Build/Item.patch.json` preserves the `DroppedActor` replacement during refresh. Only the item ships. | `Build/Item.source.json` records the source game build; explicit [item update](BUILD.md#source-updates), with current-stock comparison. |
| Other `Project/Content/` dependencies | Reconstructed stock Gyro parent, material, icon, actions, item references and string-table editor stubs. No stock shaders or implementations are claimed. | Review required identities/layouts when affected; exclude from payload. Missing dependencies created by the shared item tool require separate review before source acceptance. |
| `Project/EditorSupport/Source/Voyage/` | Minimal native API mirrors from reflection/mappings; shared item-profile overlays declared by `Project/EditorSupport.json`. | Preserve `/Script/Voyage` identity, field order and consumed metadata. Rebuild ready support on source changes; never ship its binaries. |
