# ContainerLidLabel architecture

## Supported containers and package ownership

ContainerLidLabel is an additive DML mod. Its entry class is
`/Game/Mods/ContainerLidLabel/ModActor.ModActor_C`. DML creates the manager;
no stock container Blueprint is replaced, reparented, or serialized by the mod.

`BFL_ContainerTypes` compares `GetClass(Actor)` with exactly these stock classes
under `/Game/Blueprints/Interactives/Containers/`:

| Predicate | Blueprint class |
| --- | --- |
| IsLarge | BP_Inventory_PlayerBox_Large_C |
| IsMedium | BP_Inventory_PlayerBox_C |
| IsSmall | BP_Inventory_PlayerBox_Small_C |

Unknown descendants are intentionally excluded: inheritance alone does not
establish a compatible lid layout or capacity policy. The initial class query
may find them, but the shared handler leaves them unchanged.

The release contains four authored packages under `/Game/Mods/ContainerLidLabel/`:
`ModActor`, `BP_ContainerLidDisplay`, `BFL_ContainerTypes`, and
`Settings/BP_IniFileParser`. Stock text widgets, fonts, icon palettes, materials,
and container classes resolve from the game. Editor stand-ins and native DLLs
are excluded by the producer's exact package list.

## Discovery and lifetime

`ModActor.Start`, called by BeginPlay, reads settings, obtains
`VoyagePersistentSubsystem`, binds `OnActorRestored`, then synchronously processes
one `GetAllActorsOfClass(BP_Inventory_PlayerBox_C)` snapshot. In the game that base
query includes Large and Small. The same `HandleContainer` processes subsequent
Restored events immediately. There is no manager Tick, work queue, tracked set,
periodic scan, or per-container INI read.

On the supported game build, the inspected public spawn paths emit Restored
followed by Spawned for the same actor; persistence restoration also has a path
that emits Restored without Spawned. Subscribing only to Restored covers those
paths without a second handler for the public spawn pair. This is a bounded
lifecycle contract, not a generic deduplication service for arbitrary repeated
notifications or repeated manual Start calls.

`ConfigureContainer(Actor)` applies capacity and creates a display. Capacity
belongs to the manager: `GetConfiguredWeight(Actor)` selects its snapshot,
`SetMaxWeightLimit` updates the existing inventory, and the existing
`ResourceMeter` is refreshed from `GetPercentWeightUsed()`. Inventory stacks are
not recreated or trimmed.

The display initializes from only its target actor, attaches to `PlayerBox_Lid`,
and owns inventory-name/icon subscriptions and target EndPlay cleanup. It has
no manager reference or manager owner. Target EndPlay destroys the display;
display cleanup unbinds its inventory and target callbacks. Manager Stop only
unsubscribes its Restored handler. Recreating a saved container recreates its
display; the mod introduces no persistent container state.

## Settings ownership

One parser is constructed and loaded during manager initialization. The three
effective capacities are stored in that ModActor instance and reused for its
containers. There is no CDO cache or settings-loaded flag. GetFloat calls specify
Name, Default and Min=1; Max is omitted and follows the parser declaration.
The file path, fallback values and restart policy belong to
[configuration](CONFIGURATION.md).

## Lid presentation

The added engine WidgetComponent instantiates the stock `BP_PlayerBox_Text`.
It uses Voyage Roboto Bold, size 12, draw size 200x110, scale 0.12, no wrapping.
The stock count, capacity bar and original side label remain game-owned.

Positions below are centimetres in lid-local space. Icon planes use scale 0.05.

| Container | Name centre (X,Y,Z) | Icon centres (X,Y,Z) |
| --- | --- | --- |
| Large | (0,66.4,13.73) | (+/-17,62.27709,13.73) |
| Medium | (0,47.9,13.55) | (+/-17,40.88827,13.55) |
| Small | (0,39,13.1) | (+/-8,33.743553,13.4) |

Small icons sit 3 mm ahead of the label widget plane; preserve that separation
when adjusting the layout. Icons remain vertically aligned with the count and
follow lid motion. This placement is game-validated; geometry-only or null-RHI
checks do not establish absence of flicker.

Left icon planes share the corresponding stock dynamic material. Large/Medium
right icons do the same. Small destroys its stock RightIconPlane after parent
BeginPlay, and the stock updater guards that component before updating its
secondary material. Therefore Small's added right plane owns a separate MID,
created once from the static parent of the left material. Passing the left MID
itself to CreateDynamicMaterialInstance would reuse that MID.

On an icon event, Small reads the secondary palette soft reference, hides the
old right icon and loads the texture asynchronously. Completion compares the
loaded texture path with the current selection before assigning IconTexture
and showing the plane. Cleared, invalid or superseded selections stay hidden.
This loader does not poll or add a container-processing queue.

## Native code boundary

The only mod-owned native module is the editor support module named `Voyage`,
which supplies matching `/Script/Voyage` identities for compilation and tests.
It is not shipped. InventoryContracts.h and PersistentContracts.h hold the
minimal native API stand-ins; Voyage.cpp only registers the module.

Beyond reflected declarations, their implementations simulate file reading,
capacity writes and editor inventory state. Nine Test-prefixed methods drive
or inspect synthetic test behavior: rename/icons/bindings, Spawned/Restored,
BeginPlay/EndPlay/bindings, and latent icon-load completion. They are used only
by the Python editor suite, never by runtime Blueprints. There is no native
mod behavior, Blueprint generator, custom Blueprint node, or settings generator
in the mod. Shared BPT's own editor implementation is a tool dependency.

## Source and execution data

Project contains maintained assets, synchronized BPT, native stand-in sources
and test source. Content and any authored Config files are repository data.
Only Intermediate is ignored inside Project. Engine execution uses a disposable
copy under Tmp/ContainerLidLabel: preparation, tests and cook never write into
the maintained project. Test sources stay in Git; runtime fixtures, temporary
INI files and engine output stay with the run. See [build and validation](BUILD.md)
for working-copy creation, cleanup and explicit acceptance of authoring changes.
