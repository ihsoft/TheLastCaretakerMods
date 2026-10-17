# Boat resource socket hover cards (observed asset contracts)

This document describes the Boat side connectors' resource producer and HUD
consumer. Game-derived details are bound to their stated Steam build.

## Data path

The asset contract below is bound to Steam build `25056839`.
`BP_Boat_Base` loads `/Game/Maps/Boats/TestBoat` through its
`VoyageLevelInstanceComponent`. The level contains a real
`BP_Module_Battery` and a real `BP_Module_Diesel_Container`; the large side
connectors are separate virtual socket actors attached to those modules.

`BP_AttachmentVirtualSocketActor_Electricity` derives from native
`VoyageVirtualActorSocket`. On begin play it walks its attachment parents,
resolves the first `VoyageModuleComponent`, and registers its socket through
`AddExternalSocket`; it unregisters through `RemoveExternalSocket` on end play.
The Petrol variant derives from that Blueprint and changes the socket data and
resource. In `TestBoat`, its per-instance port is overridden to
`ResourceType = Diesel` and `ResourceCategory = Fuel`. Therefore neither the
Blueprint class name nor its inherited Petrol default identifies the live
resource; `Port.ResourceType` is authoritative.

All surveyed Boat-side Electricity sockets attach to the built-in battery's
mesh. All surveyed Boat-side Diesel sockets attach to the built-in Diesel
container's mesh. The side connectors are consequently external sockets of
ordinary modules, not special sockets owned directly by `VoyageBoatPawn`.
The built-in Diesel module has local capacity `2000`, matching the stock card's
`1897 L / 2000 L`.

Native `ModuleSocketComponent::GetScanText` produces the middle line. It resolves
`ModuleOwner`, reads the port resource type, calls that component's virtual
`GetResourceAmount(type)` and `GetMaxResourceAmount(type)`, and writes the
formatted value into `VoyageScanText.Description`. It has no same-grid walk,
no `VoyageBoatPawn` call, and no Electricity-specific aggregation branch. The
interaction HUD is a downstream display consumer of already formatted
action/scan data.

`VoyageBoatPawn::GetModuleResourcesAvailable(type)` and
`GetModuleMaxResources(type)` are distinct aggregators. They obtain the Boat's
module grid, enumerate eligible modules in that grid, call the same two module
getters, and sum the results. This is the established aggregate operation; the
stock socket scan producer does not use it.

The socket cards show local module storage rather than whole-grid totals;
`6.0 kWh` is the local capacity of the built-in Boat battery. Battery and Diesel
container both own the same concrete `VoyageModuleComponent`; neither
Blueprint overrides the resource getters or initializes special resource
state in Blueprint `BeginPlay`. There is no resource-specific socket behavior
to fix.

The `kWh` suffix is a stock presentation convention, not a physical conversion
contract. The UI divides the native electricity amount by `1000`; runtime power
integration shows that the native stored amount behaves as kJ while module
demand is expressed in W. Code that controls energy consumption must therefore
use kJ and W directly and must not convert native amounts through Wh or apply a
factor of `3.6`. UI replacements may retain the stock `/1000` formatting when
visual parity is the goal.

The surveyed indicator creation path does not call
`VoyageBaseUserWidget::SetInteractiveSource` on its button container. A global
replacement consumer therefore cannot reliably recover the exact hovered
Diesel socket through `GetInteractiveSource()` and must not infer it from child
index, localized text, or rendered key labels.

`VoyageModuleResourceWidget` and `DA_Socket_LiquidData_Diesel` are false leads
for this value. The former had no active surveyed reference/call site; the
latter owns meshes, materials, sounds, rotation, and leak effects rather than
text or action data.

## External socket registration

This section applies to Steam build `25191271`.
It does not update the separate HUD contracts' version boundary above.

The Boat's `VoyageLevelInstanceComponent.OwnedActors` contains its built-in
actors, including battery modules and external ports. It is
`TArray<TWeakObjectPtr<AActor>>`, not a strong actor array. All 14 stock
Electricity external ports in `TestBoat` attach to the built-in battery's mesh.
Use the resolved Boat's owned references rather than a world-wide search.

`BP_AttachmentVirtualSocketActor_Electricity.GetParentModule` walks attachment
parents to an actor with a module. Its public Blueprint-callable/event signature
has one `AActor* Actor` **out parameter**, not a return value. Native static
`VoyageMiscBlueprintFunctionLibrary.GetModuleFromActor(AActor* Actor)` is
Blueprint-callable, not Blueprint-pure, and returns `VoyageModuleComponent*`.
Together these functions resolve the same owner used by a stock external port,
without depending on whether the port's own registration has already run.

The actor's instance member is `VoyageModuleSocketView`.
`VoyageModuleSocketView_GEN_VARIABLE` names its SCS template, not that member.
The view derives from `ModuleSocketComponent`; a cable endpoint must be matched
to the actual socket object, not an arbitrary socket from a module's list.

Native `VoyageModuleComponent.AddExternalSocket(Socket)` transfers the same
socket from the prior owner's socket list to the new owner's list, assigns
`ModuleOwner`, and initializes the socket, including `ModuleOwnerOriginal`.
It has no refusal based on an existing cable or connected peer. Preserve those
references during registration instead of treating an already connected socket
as ineligible. `RemoveExternalSocket` removes membership but does not clear the
weak owner reference; cleanup should confirm the socket is still owned by the
recorded owner. Do not edit either module's socket list manually.

To reproduce a stock electric external port's registration, copy its complete
`Port` value and `bAutoInitialize`, `bAddModuleRequirement`, `bIsVirtual`,
`bIsVirtualGrouped`, and `bIsVirtualShareToGroup`. Do not infer absent serialized
flags are false: inherited/native defaults still apply. Do not copy the reference
socket's identity, transform, mesh, or cable references. Stock wall-socket
post-load initializes its identity with
`SetSocketID(Conv_StringToName(GetObjectName(Self)))`; this path contains no
battery-wide collision scan.

External battery ownership supports Boat power sharing with stable cable flow.
Secondary-group membership alone is not equivalent to this ownership contract.
The five registration flags are a combined stock configuration; this does not
establish that any single flag independently determines flow stability.

Wall-socket `PairedModule` is a stock SaveGame property: never store a Boat
battery, Boat master, or mod object there. For an existing pair, clear both
ends and their `WallSocket` group membership. Stock `UnpairModule` does not
provide symmetric source-and-peer cleanup on its valid-pair branch.
`SecondaryModuleGroupIds` lacks the native SaveGame flag, which alone does not
exclude custom persistence. Runtime registration does not by itself establish
safe removal of a mod from a saved game.

See [attachment and restoration events](vehicle-and-hud-modding-patterns-knowledge.md#actor-attachment-and-save-restoration)
and [weak-array Blueprint access](voyage-cooked-asset-toolchain-knowledge.md#reflected-weak-object-arrays).

## Integration boundary

Network-wide Diesel and Electricity belong in the possessed-Boat HUD, while
socket scan text remains local to its module. Preserve the stock Actor, SCS
template, component class and global interaction-widget consumer. The
[research pitfalls](research-pitfalls-knowledge.md) document unsafe interception paths.
