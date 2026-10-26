# StableGyro architecture

The loader-free mod leaves the stock Gyro Blueprint at its original package
identity and generates a child selected through the stock item data asset's
`DroppedActor`. One helper handles pitch input before the stock physics tick;
another applies eligible altitude and horizontal stabilization after physics.
The child retains stock mappings and provided actions while adding an optional
pitch-reset action whose default key is X. Tick prerequisites are established at
BeginPlay and controller changes, not re-added every frame. The two flight helpers remain in
their respective PrePhysics and PostPhysics groups.

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

The PrePhysics pitch helper derives reset from a rising edge using transient
`ResetActionWasActive`. It updates the latch after writing the integrated and
native pitch, so holding reset does not repeat the reset or block later pitch
integration. Disabled pitch, invalid ownership, controller changes and EndPlay
clear the latch. The existing pitch-enable gate also controls the reset hint.

## Independent flight-control settings

`EnableAltitudeControl` and `EnableHorizontalVelocityControl` both default to
true and are reread by the PostPhysics `ModActor` when the player enters a
Gyro. They are independent of each other and of `EnablePitchCorrection`.

When altitude control is false, PostPhysics clears all vertical lock, braking
and sample state and performs no mod-owned Z position or velocity correction.
The player-entry callback also leaves saved `HoverPitchScale` untouched. The
PrePhysics helper reads the altitude flag through its cached `ModActor`
reference, declines native altitude-assistance ownership, and restores any
captured `bAutoHoverCollective` and `bAltitudeAssist` values. W/S pitch
integration and eligible horizontal damping remain available.

When altitude control is true, the genuine player-entry callback writes the
entry-reloaded `HoverPitchScale` setting to the native Gyro. This advanced INI
setting defaults and falls back to `2.5`, accepts `0.5..2.5`, and is listed
under the warning against changing advanced settings. It is dimensionless and
is not polled or rewritten on tick.

When horizontal velocity control is false, PostPhysics branches before reading
or writing horizontal physics velocity, clears its prior XY sample, and
continues into the independent altitude path. Altitude control and W/S pitch
integration remain available.

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
The retired `FuelConsumptionPerMinute` key is ignored at runtime and removed
by the local installer, which adds the new default if its key is missing.

After reloading settings, `ApplyFuelSettings` applies them at helper BeginPlay
and on genuine player entry, before the altitude-control branch. Editing the
installed INI therefore takes effect on the next entry. Fuel settings are
independent of pitch correction, altitude control, horizontal damping and the
four-port option. There is no per-tick settings read or fuel write.

The function updates only the owned custom module's Petrol capacity entry and
the Gyro's base consumption property. It neither refills the tank nor removes
stored fuel when capacity is lowered; other resource entries remain intact.
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

### Port and base layout

Ports are `VoyageModuleSocketViewComponent` components attached to the native
`VehicleMesh`; placement does not require named static-mesh sockets. Copy the
Gyro's small mooring socket and Hook Cable configuration, rather than the ship's
Mooring Cable actor. Native socket registration and cable reeling enumerate the
actor's socket components, so the extra pair needs no separate winch loop.

The stock `MooringPlate` is a separate decorative component whose mesh contains
both sides together. It is not part of the frame mesh or two selectable material
sections. The producer splits its full decoded Nanite leaf geometry into two
side-specific meshes while preserving the original pivot. Four components reuse
those halves with the stock component material override and no collision. The
public producer delegates mesh import and cooking to the shared
[StaticMesh tools](../../tools/static-mesh-assets.md). The meshes use native
Nanite with all input triangles retained, zero trim error, position precision
`4`, normal precision `8`, and implicit tangents. A separate 25% triangle
fallback supplies the ray-tracing proxy. This reduces that representation's
geometry budget without reducing the full-detail Nanite input.
The inherited paired plate is hidden only in four-port mode.

The Blueprint generator uses valid editor-only reference meshes at the final
package identities. These dependencies are excluded from the mod cook; the
common producer's cooked meshes and every emitted bulk companion supply the
runtime packages. Staging checks their declared paths and sizes. Final-container
validation checks Nanite, fallback, ray tracing, source attributes and the stock
material reference before installation. The shared
[cooking knowledge](../../docs/static-mesh-cooking-knowledge.md) defines the
representation and validation boundaries.

Sockets and bases move together along the frame tubes: the original pair moves
5 cm forward and the added pair sits 40 cm behind the original positions. The
positive-Y tube direction is approximately
`(0.66015035, -0.72922620, 0.18008518)`, with Y mirrored on the other side.
This direction comes from frame geometry; a port normal alone cannot determine
it. Rotations and scales remain unchanged. Exact transforms and stable SocketIDs
are owned by `MooringLayout` in the inheritance generator.

### Drop Cargo and hints

Native reeling covers the extra sockets, but the stock Drop Cargo callback and
availability test use only the original pair. Four-port mode reuses the stock
Enhanced Input action and supplies one replacement binding and hint. It scans
valid Mooring sockets, uses the native connection predicate for availability,
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

`Assets/StableGyro.ini` is the packaged settings source, and
`Settings/StableGyro.settings.json` defines runtime bindings and value ranges.
[ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md) explains the native hover and
stabilization boundary. [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md)
identifies version-bound inputs; [mod rules](MOD-RULES.md) give the exact
behavior and settings constraints.
