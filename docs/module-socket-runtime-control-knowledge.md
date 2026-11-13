# Runtime control of resource sockets

These native contracts were inspected on Steam build 25982825, UE 5.8.
They apply to `ModuleSocketComponent` and `VoyageModuleComponent`. The inspected
build identifies the evidence, not a prerequisite for using these contracts;
recheck affected behavior when current evidence disagrees.

## Bandwidth and direction

`Port.Bandwidth.Input` and `Output` control the two transfer directions.
Zero is a closed direction; a negative bandwidth denotes an unlimited limit.
The native bandwidth getter combines socket and cable limits, preserving a
zero socket limit. Changing one socket's bandwidth is distinct from
`VoyageModuleComponent.SetCustomConsumption`, which changes module demand.

`Port.Type` constrains the socket's allowed direction. Existing connections
also have current direction state. `SetSocketDirectionType` updates that state
and the peer's direction only while both endpoints have `Port.Type = ST_Any`.
For an existing bidirectional connection, request the desired direction before
fixing the controlled endpoint's type. This setter is not a universal operation
for arbitrary fixed-direction peers.

Automatic socket initialization can replace bandwidth from the owning module.
A controller taking ownership of a socket's limits captures its original
values and `bAutoInitialize`, then disables automatic initialization for the
duration of that ownership. Restore captured state on cleanup when valid.

## Notify the network after a limit change

Writing `Port.Bandwidth` alone is insufficient for controlling an established
resource route. Native routing includes stored bandwidth values as well as
paths that read the current socket fields. After writing changed limits, call
the owning module's callable, parameterless `BroadcastConnectionsChanged()`.
It visits connected sockets and notifies their module owners through
`ConnectableModule.ConnectionsChanged`. This is different from merely invoking
the `OnModuleSocketConnectionChanged` multicast delegate.

Capture the old-versus-desired limit comparison before mutating the port.
Notify only for a real change. A listener that reapplies limits on every
connection notification and broadcasts unconditionally can create a feedback
loop. Coalesce connection handling to the next tick, and ensure unchanged
reconciliation does not broadcast again.

Editor tests can verify writes and notification counts. Native flow acceptance
requires an actual consumer on the controlled cable; neither a colored rim,
cable glow nor total battery discharge proves that this particular port is
passing or blocking power. Other ports may still supply energy.

## Reflection and state preservation

An editor-only mirror may expose the members needed to compile Blueprint
access without describing every member of the native Port struct. In that
case, use Set Members in Struct on the live property reference. Replacing the
whole struct with a value constructed from the partial mirror would risk
discarding resource filters and other native state. Full mirrored substructs,
such as the three-field `ModuleSocketBandwidth`, can be constructed separately
while preserving fields outside the controller's ownership.

For registration and module ownership, see
[boat resource sockets](boat-resource-socket-architecture-knowledge.md).
