# ElectrifiedBuildings architecture

## Controls and electrical ownership

The mod adds a cable socket to each supported control with exactly one resolved
building inlet. The inlet owns the electrical connection; the control owns the
socket's physical placement.

| Control | Discovery | Supply requirements |
| --- | --- | --- |
| Local light switch | `BP_Module_LightSwitch` | Location volumes containing the switch |
| Lift/transporter call button | `BP_PlatformControls` | Location volumes containing the button |
| Warehouse door lever | `BP_Module_Generic_Switch` attached to an actor whose exact class is `BP_WarehouseDoor` | Location volumes containing the door |

Other generic switches and door variants are outside this discovery contract.
Classes are loaded from the game's packages; discovery uses no POI or instance list.
Placement is defined per control mesh by the `EnsureControlOutlet` calls in
`RefreshBuildingSockets`.

`IndexLoadedInlets` indexes positive values from enabled `RequirementsFills`
blocks on loaded `BP_Module_RoomOutlet` modules. `ResolveInletCandidates` matches
them against positive requirements from enabled `VoyageLocationVolume` blocks
applicable to the supply host. Membership uses inclusion and overlap sets, with
explicit exclusions taking precedence. Matching inlet actors are deduplicated.

Creation requires exactly one inlet **for that control**. With zero or multiple
candidates, no socket is created; later discovery events can reconsider the
control. Separate building supplies are not joined. Power availability, cable
direction and transfer rates remain the game's responsibility.

## Lifecycle

DML loads `/Game/Mods/ElectrifiedBuildings/ModActor`. Its synchronous initial
scan handles controls already present. `VoyageModuleSubsystem.OnModuleRegistered`
and `OnModuleUnregistered` then trigger discovery and orphan cleanup.
`OnModuleGroupsChanged` subscriptions cover group initialization on relevant
modules: switches, call buttons, inlets and warehouse doors.

Event bursts queue one `SetTimerForNextTick` callback through `RefreshQueued`.
The callback clears the flag and refreshes the scene; it does not reschedule
itself. Existing sockets are reused. EndPlay cancels pending work, removes the
actor's delegate bindings and clears its watched-module list. There is no
recurring discovery timer or Blueprint Tick.

## Socket creation and saved identity

`BP_BuildingSocket` derives from native `VoyageVirtualActorSocket`. Its
`VoyageModuleSocketView` root uses the stock `DA_Socket_ElectricData` connector.
The game provides the mesh, interaction, cable controls and sounds.

`AttachOutlet` deferred-spawns the helper with the physical control as owner,
placing it in that control's level. Before finishing the spawn it reserves the
name `ElectrifiedBuildings_<host object name>_Power`, renames the actor, assigns
`SetSocketID` from the root component's object name, and attaches to the resolved
inlet with virtual group sharing enabled. Occupied actor names are collected
only for the first reservation in a refresh batch and reused within that batch.

The helper's BeginPlay calls `AddExternalSocket` on the inlet's module. After
spawn completion, the helper attaches to the physical control at its configured
offset and rotation. EndPlay calls `RemoveExternalSocket` on the registered
module if it is still valid. Failed spawn, root cast or rename paths leave no
usable helper; a later discovery event can retry the host.

Cable restoration uses the game's endpoint identity. Preserve the helper's
package/class, actor naming rule and native root-component name when changing
implementation or source layout. The mod has no separate cable-save format.

## Project dependencies

[Build/packages.txt](Build/packages.txt) owns the two-package release inventory.
The mod is additive and requires DML; editor support is excluded from the payload.

[RuntimeContracts.h](Project/EditorSupport/Source/Voyage/RuntimeContracts.h)
declares editor-only `/Script/Voyage` identities. Runtime implementations come
from the game. These contracts were reconstructed from native reflection,
reviewed mappings and stock asset properties on Steam build `25982825`, Unreal
`5.8`; [game-build.json](game-build.json) owns the source-build pin.

`FModuleRuntimeRequirements` retains its full reflected field order for Blueprint
struct operations. The serialized socket root also requires the native order of
`ModuleSocketComponent`, `VoyageModuleSocketViewComponent` and nested structs,
including zero-valued fields. Nonserialized module/volume declarations expose
only the members used by the graphs. Empty method bodies supply editor reflection,
not gameplay implementations.

The authored `Port.Type` is `ST_Any`; its distinct editor default `ST_Max` keeps
that zero-valued override in the unversioned cook. Preserve the complete Port
value and initialization flags. Collision-channel names in `DefaultEngine.ini`
match the stock socket's interaction responses. Reinspect affected declarations
when game changes or observed behavior contradict these contracts.

Shared contracts: [socket ownership](../../docs/boat-resource-socket-architecture-knowledge.md),
[port controls](../../docs/module-socket-runtime-control-knowledge.md),
and [saved endpoints](../../docs/mooring-cable-and-socket-knowledge.md).
