# StableGyro architecture

The loader-free mod leaves the stock Gyro Blueprint at its original package
identity and generates a child selected through the stock item data asset's
`DroppedActor`. One helper handles pitch input before the stock physics tick;
another applies eligible altitude and horizontal stabilization after physics.
The child retains stock mappings and provided actions while adding an optional
X pitch-reset action. Tick prerequisites are established at BeginPlay and on
controller changes, not re-added every frame. The two flight helpers remain in
their respective PrePhysics and PostPhysics groups.

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

When horizontal velocity control is false, PostPhysics branches before reading
or writing horizontal physics velocity, clears its prior XY sample, and
continues into the independent altitude path. Altitude control and W/S pitch
integration remain available.

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
those halves with the stock component material override and no collision. They
are cooked as ordinary static meshes containing the extracted detailed geometry.
The inherited paired plate is hidden only in four-port mode.

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
