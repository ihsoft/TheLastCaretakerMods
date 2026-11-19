# PearFix architecture

PearFix restores missing accounting for existing full-size pears, allowing the
stock tree to resume growth and make its fruit collectible. It is an additive
DML actor at `/Game/Mods/PearFix/ModActor`; it does not override stock packages.

## Repair scope

The orchard is identified by the stock SeedInfo object path
`/Game/Data/Assets/Farming/DA_Seed_Coffee.DA_Seed_Coffee`, despite that asset's
internal name. Its `VoyageSeedFruitInfo_0` supplies the fruit-info reference.
Stock collision exclusion and ripening use the tree's `Fruits` map, not every
attached actor. An unregistered attached pear can therefore block growth while
remaining outside the update loop that would make it collectible.

`RepairTreeOnLoad` examines the tree's attached actors. Before reconstructing
an entry, `RestoreFruitRegistration` requires the expected orchard and fruit item,
a valid fruit directly attached to this tree, `Scale.X >= 1`, and absence from its
registered fruit actors. Exactly one valid, tree-owned fruit socket must match
within 25 cm, and its named map key must be free. This distance is a mod matching
policy, not a stock identity guarantee. Missing, ambiguous, immature, detached,
wrong-item and occupied-slot cases remain untouched.

The repair writes `VoyageSeedFruitGrowInfo` with the existing actor soft reference,
the stock fruit-info reference and `GrowTimer = 1`, the mature lower bound. After
reading back the registration, it removes only that slot's entries from
`RemovedFruits`, including duplicates. Other removal markers remain intact.
The existing actor, attachment and transform are preserved. Stock updates own
ripeness, pickup permission and the recalculated growth/space state; the mod
does not spawn replacement fruit or force those flags. Rechecking an already
registered pear makes no change. The repair does not change resource handling.

## Automatic lifecycle

BeginPlay binds `VoyagePersistentSubsystem.OnActorRestored` and `OnActorAttached`.
Handlers resolve the relevant tree from the supplied actor or its attachment
and add it uniquely to `PendingTrees`. One initial player-ready snapshot covers
trees that existed before subscription. Subsequent work comes from events;
there is no periodic world scan or location-wide polling.

The first pending tree schedules a non-looping five-second timer. Further events
join that batch without extending its deadline. `ProcessPendingTrees` drains the
queue and releases its references; it does not schedule another pass by itself.
A later event can create a new batch. The delay allows asynchronous tree setup
to progress, but is not proof that arbitrary asset loads have completed. Repair
guards still apply at execution time. EndPlay unbinds both handlers, cancels the
timer and clears queued references. See the shared
[persistence lifecycle](../../docs/vehicle-and-hud-modding-patterns-knowledge.md#actor-attachment-and-save-restoration)
for event ordering and readiness boundaries.

## Player feedback

Each successful fruit registration increments `PendingRepairCount`. A stock HUD
card displays `Pears fixed: N` for 20 seconds, with no body, under the key
`PearFix.Repaired`. N counts repaired fruits, including multiple fruits on one
tree; healthy checks produce no card. The count is cleared only when
`AddMessageBySlot` accepts delivery. If the HUD is unavailable, pending successes
accumulate and can be delivered once it is ready.

Tick handles player readiness and pending notification delivery, not tree
searches. There are no diagnostic hotkeys, audit history or diagnostic cards.
Text is English. The stock
[message-card interface](../../docs/voyage-hud-messages-knowledge.md) owns layout
and delivery semantics.

## Source and compatibility boundaries

### Editor input provenance

The maintained stock-path compilation inputs reflect Steam build `25982825`
and are authored with Unreal Engine 5.8.2. The `BP_DynamicMeshActor`,
`BP_DynamicMeshActor_Fruit` and `BP_FarmingSeed` editor stubs are reconstructed
from inspected stock reflection and Blueprint identities through BlueprintText;
their saved editor-role metadata marks them as stubs. `FarmingSeedFruitSocket` is an
editor-reconstructed user-defined struct preserving the stock struct and member
identities. Native declarations in `DiagnosticContracts.h` and
`PersistentContracts.h` mirror inspected types and delegate signatures; value
structs preserve field order. These are maintained compilation inputs, not
copies of stock gameplay implementations. Revalidate them against current
mappings and affected stock packages when those identities or layouts change.

The maintained Blueprint asset is the runtime source; BPT exports are temporary.
`Project/EditorSupport` contains editor-only native reflection declarations;
stock-path Blueprint stubs provide compile-time dependencies. Neither enters
the release container. The builder prepares dependencies from the game and
enforces the pin in `game-build.json`; only ModActor is packaged.

Gameplay acceptance covers automatic repair after save loading and streamed
arrival, restored collection/growth and the counted notification. It does not
establish the original cause of every missing map entry or promise to resolve
unrelated environmental growth obstructions. Recheck the map/struct identities,
socket matching and lifecycle when a game update changes these contracts.
