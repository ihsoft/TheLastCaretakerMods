# StableGyro architecture

The loader-free mod leaves the stock Gyro Blueprint at its original package
identity and generates a child selected through the stock item data asset's
`DroppedActor`. One helper handles pitch input before the stock physics tick;
another applies eligible altitude and horizontal stabilization after physics.
The child retains stock mappings and provided actions while adding an optional
X pitch-reset action. It also owns the restart-applied
`EnableFourMooringPorts` layout switch. The replacement-class CDO reads that
one setting on first use and caches it for the lifetime of the loaded class;
class unload and reload may read the file again. The child reads the snapshot
before its parent construction call. Both modes create the two rear socket
components from the same retained Blueprint component templates and attach them
to the native `VehicleMesh` before registration, preserving their names and
socket identities for saved connections. Four-port mode then creates the four
derived plates, runs the parent construction graph, and applies the established
four-port stock-part layout. Stock mode runs the parent construction graph with
inherited stock parts untouched. After BeginPlay it binds each extra socket's
native connection event and queues an initial next-tick cleanup. Every later
connection event queues the same coalesced cleanup regardless of its bool payload,
avoiding synchronous disconnect inside the native callback. Cleanup keeps its
coalescing guard set through both sockets, uses the stock non-destroying helper,
hides and deactivates only components owned by this Gyro, and disables their
collision and overlap interaction. The bindings remain active for connections
restored later in the load lifecycle. The native helper's real cable cleanup,
load ordering, and save compatibility remain game-validation boundaries.

`Assets/StableGyro.ini` is the packaged settings source, and
`Settings/StableGyro.settings.json` defines runtime bindings and value ranges.
[ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md) explains the native hover and
stabilization boundary. [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md)
identifies version-bound inputs; [mod rules](MOD-RULES.md) give the exact
behavior and settings constraints.
