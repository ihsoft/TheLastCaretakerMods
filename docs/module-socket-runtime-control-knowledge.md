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

## Resource groups and diagnostic values

`VoyageModuleComponent.GetModuleSecondaryGroups() -> TMap<FName, int32>` exposes
actual runtime secondary-group memberships. Use the native getter rather than
misdeclaring the underlying unsigned-ID map or converting IDs through floats.
Configured `ModuleGroupName` and `AdditionalGroupNames` do not enumerate those
runtime memberships. Multiple memberships alone do not establish independent
power inlets or transfer directions.

`CachedGridID` is a runtime-allocated module-grid key used by
`GetModulesInSameGrid`, not a stable identity or a complete electrical-resource
network description. The resource-amount query uses secondary groups through
a separate lookup. Equal cached grid IDs therefore do not establish equal power
availability, interchangeable inlets or bidirectional transfer.

`GetModuleGroupTotalResourceAmount(Module, ResourceType)` takes
`EModuleResourceType`, whose `Electricity` value is `0` in the inspected contract.
It does not take `EModuleResourceCategory`, whose `Electricity` value is `1`;
passing that numeric value queries `EModuleResourceType::Petrol`. Preserve the
actual reflected enum type instead of substituting a similarly named selector.
The result is a resource amount, not measured transfer power in watts.

For stock `BP_Module_Generic_Switch` actors, class and visible lever geometry
alone do not identify a building inlet: the class also serves door controls.
Inspect attachment parents, `SwitchType` and module configuration before
choosing an electrical owner. Group membership and visual proximity do not
replace that ownership evidence.

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
