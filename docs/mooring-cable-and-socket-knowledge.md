# Mooring cables and socket lifecycle (observed contracts)

These findings describe the shared Voyage socket/cable layer inspected on
Steam build `25982825`. Runtime validation covers the StableGyro implementation;
it does not establish every vehicle's reeling, release action or load ordering.
Recheck the relevant native and asset contracts for another game build or owner.

## Socket, cable and decoration are separate objects

`VoyageModuleSocketViewComponent` derives from `ModuleSocketComponent`. The
socket's transform places an endpoint; a named socket in a StaticMesh is not
required for the observed component-based arrangement. A visible mounting plate
can be a separate decorative mesh with independent material and collision.
Copying a plate does not create a functional cable port.

Copy the intended socket's complete configuration, including inherited defaults,
but assign distinct identities to new endpoints. `Port.ResourceType` identifies
the resource; class names or appearance are not sufficient. Do not copy live
peer/cable references from another socket.

Hook Cable and Mooring Cable share the `BP_ModuleCable_Base` hierarchy and have
`bMooringCable=true`, but their asset configurations differ:

| Observed configuration | Gyro Hook Cable socket | Ship Mooring Cable socket |
| --- | --- | --- |
| Socket data | `DA_Socket_MooringSmall` | `DA_Socket_Mooring` |
| Port direction | `ST_Any` | `ST_Output` |
| Integrated cable | `DA_Cable_Hook` | `DA_Cable_Mooring` |
| Integrated length | 1000 cm | 2000 cm |

Hook Cable specifies `DA_Hook_Attachment` as its attachment item. Absent strength,
stretch or other serialized overrides still inherit defaults; absence is not zero.
Do not substitute a ship socket configuration merely because both cables moor.

## Registration and vehicle consumers are different contracts

The inspected module discovery path enumerates eligible socket components on
its actor and assigns their module owner. `bAutoInitialize=false` does not exclude
a socket from this list: ownership assignment happens before the flag controls
port-data initialization. Discovery also considers standalone status, existing
ownership and `OwningModule`; copying only the mesh and transform is insufficient.

A port on a separate actor needs the corresponding external registration path.
The stock Boat resource-port pattern uses `AddExternalSocket` and
`RemoveExternalSocket`; see [external socket registration](boat-resource-socket-architecture-knowledge.md#external-socket-registration).
Membership in a module's socket list does not prove that a vehicle-specific
consumer will discover that external actor.

Inspect each consumer independently when adding ports. For example, native Gyro
reeling enumerates its actor's socket components, while its release callback and
hint predicate use two cached endpoints. Adding components therefore extends
reeling but does not automatically extend release or hints. This distinction is
reusable; the Gyro's enumeration scope is not a contract for every vehicle.

## Connection events do not mean only connected/disconnected

`OnSocketConnectionChanged` uses
`VoyageSocketConnectionDelegate__DelegateSignature(bool bSocketConnection)`.
The direct connect and disconnect paths broadcast true and false respectively.
However, the `ConnectionCable` setter also broadcasts after assigning the cable
and updating the socket; its bool reports validity of `ConnectedToSocket`.
Assigning a cable can consequently broadcast false.

For work that depends on the presence of a cable, process the event as a request
to inspect current state rather than filtering out false. The native
`IsSocketConnectedOrHasCable()` predicate accepts a valid peer or a valid cable;
these references need not become valid simultaneously during restoration.

`VoyageMiscBlueprintFunctionLibrary.DisconnectCableFromSocket(Socket, false)`
uses the socket's current cable and the native endpoint-disconnect path. The
boolean controls cable destruction; false is the observed stock Gyro release
argument. Hiding a mesh, disabling collision or deactivating a component does
not substitute for this operation. Remote attachment cleanup belongs to the
native cable/action behavior, not to a local visual traversal. Arbitrary cable
types and pre-existing orphan attachments need their own verification.

## Saved endpoint identity and disabled optional ports

When a saved connection refers to an optional endpoint, removing or omitting
that endpoint can leave an unresolved reference. StableGyro's game test exposed
a long load stall and leftover attachment pads with that approach. A working
alternative keeps endpoint names and SocketIDs stable in both modes, restores
the endpoint, disconnects it natively, then hides and disables local interaction.
This preserves reference resolution; it is not a general mod-removal migration.

For that implementation, BeginPlay queues an initial cleanup and binds connection
events for later restoration. Cleanup runs once on the next tick, outside the
native callback, with a queued flag held until all intended endpoints finish.
Synchronous events from its own disconnects cannot schedule another pass. New
external events can. This avoids polling and does not pretend that a fixed delay
marks the end of all save restoration.

Visual cleanup checks component ownership, disables collision and overlap, and
handles local descendants and explicit visual references. It does not follow
the cable to another actor. Snapshot a descendant array before a Blueprint
ForEach: a pure `GetChildrenComponents` feeding the loop can otherwise be
reevaluated for length and element access on every iteration.

## Reproduction and boundaries

Use the public [asset inspection](../tools/asset-inspection.md) and
[executable inspection](../tools/executable-inspection.md) tools for the exact
socket, cable and consumer. [StableGyro source provenance](../mods/StableGyro/GAME_DERIVED_SOURCES.md)
records the inspected owners and native locations;
[its architecture](../mods/StableGyro/ARCHITECTURE.md) records the implemented
pattern and game-test boundaries. For action identity, availability and hold
semantics, see [vehicle action patterns](vehicle-and-hud-modding-patterns-knowledge.md#action-execution-and-action-presentation-are-separate-contracts).
