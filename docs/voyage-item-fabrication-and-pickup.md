# Voyage item fabrication and pickup

## Evidence boundary

This document separates item serialization, inventory ownership, transfer and
notifications. General mechanisms are not tied to a particular mod or game
build; static facts, runtime coverage and inferences are distinguished below.

Where a section explicitly refers to a fingerprint, its binary/schema evidence
was obtained on Steam
build `25191271`, executable SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`, parser
profile `UE5_8`. This provenance gates reuse of the corresponding extracted
inputs and reconstructed binary contracts, not the general ownership and event
patterns. A game update alone is not a reason to repeat the whole investigation.
An optional implementation example is the
[Railgun architecture](../mods/Railgun/ARCHITECTURE.md), which owns its item
values and release evidence, not the general contracts described here.

## Distinct contracts

A working recipe requires more than a visible entry in the fabricator:

- Primary-asset registration and the game's configured scan roots establish
  manager identity/discovery. Research or another unlock path grants access;
  registration is not itself a recipe unlock.
- Package serialization must match the consumer's expectations. A registered
  and displayed item can still fail fabrication/pickup. The tested correction
  is described in the shared [serialization contract](voyage-cooked-asset-toolchain.md#package-serialization-is-a-separate-compatibility-contract).
- Item recipe and drop configuration determine the fabricated output. Validate
  the spawned box's contents and loot interaction, not just the completion
  sound or a visible physical mesh.
- Inventory ammunition consumption and weapon ballistics are separate
  consumers. Fabrication/pickup tests do not establish firing compatibility.

## Ammo item does not require a complete weapon object graph

One authored `VoyageItemAmmo` export with valid primary registration, working
serialization and retained recipe/drop metadata can fabricate and be looted.
In the tested setup, no projectile subexports, `WeaponData`, bullet/case fields,
stock SFX/VFX or damage-type references are required for those two operations.
The stock secondary icon and `ScalePerItem` are also unnecessary there.

This does not establish an absolute minimum schema or removal safety for other
ammo/weapon consumers. Omitted properties can inherit native/class defaults;
absence in serialized output must not be read as a zero or disabled value.

## Craft amount and drop grouping are different settings

For a six-round recipe using stock ammo-box drop configuration:

- Omitting both `MaxDropCount` and `ScalePerItem` produced six boxes with one
  round each.
- Restoring only `MaxDropCount=50`, with `ScalePerItem` still omitted, restored
  one box containing all six rounds.

The second test isolates a sufficient grouping correction for this setup.
Keep an explicit suitable `MaxDropCount` when preserving this behavior; do not
assume `CraftAmount` alone controls how many items share one drop. It does not
prove that exactly `50` is necessary, establish the behavior of every other
limit, or equate drop grouping with inventory stack capacity.

## Caliber is numeric, and omission does not hide it

`VoyageItemAmmo.Caliber` is a float. In the tested item, omission retained
functional fabrication/pickup but yielded the native/class default `9.0`,
displayed as `9mm`. Explicit `45.0` displayed as `45mm`.

Do not use removal as a tooltip-hiding mechanism or attempt to put a semantic
string such as `Railgun` into this numeric property. An item-specific text
override or hiding mechanism would require separate consumer research and
validation; changing a shared formatter would affect other items.

## Validation boundary

For a reduction or new item, separately check discovery/unlock, fabrication,
output count and grouping, loot transfer, and presentation. Save/load and
multiplayer require their own tests. Retain a known-good artifact before a
change; parser reopen and container verification remain lower gates than
these actual gameplay consumers.

## JSON authoring boundary

For the tested Railgun setup, the complete owned UAssetAPI/UAssetGUI JSON is
the source of truth for the ammo item's object graph and values. The reviewed
[UAssetAPI `21c982f` writer](voyage-json-save-checkpoint.md#serialization-contract)
writes it directly to loose `.uasset/.uexp` with reviewed mappings and an
explicit `UE5_8` serialization hint; no stock donor or post-write patcher is
used. The writer does not add hidden balance overrides. Stored object and
custom versions remain part of the authored package-format contract rather
than a runtime dependency on another item.

The producer reopens the package and compares serialization metadata, imports,
name map and the complete export graph against the current JSON, rather than
enforcing historical property or import counts. This direct-JSON route is
game-validated for the tested setup recorded in the owning
[Railgun architecture](../mods/Railgun/ARCHITECTURE.md#current-game-validated-checkpoint),
not for arbitrary future edits; packaging, reference resolution and affected
gameplay consumers retain their own gates.

For this filtered UE5.8 import layout, encode an unset import `PackageName` as
the explicit FName `"None"`, not JSON `null`. In the reviewed writer, a null
FName can serialize as name-map index zero, which is not guaranteed to resolve
to `None`. A repeated stable binary roundtrip can therefore preserve an
incorrect first write; semantic readback must compare against the authored
JSON. Do not generalize this to byte-identical `.uasset` JSON roundtrips: fields
such as name-map hashes can be recomputed by the writer.

## Filtered module inventories: refinery reference

### Inventory change notifications

On the fingerprint above, `VoyageBaseInventoryComponent` declares reflected
multicast delegates including `OnInventoryChanged`, `OnInventoryAdd`,
`OnInventoryRemove` and `OnInventorySlotChanged`. These are not absent merely
because a mod's partial editor mirror does not declare them.
The stock `BP_Module_DieselGenerator` Blueprint obtains the internal inventory,
checks validity, and adds its parameterless `Sequencer inventory changed`
callback to `OnInventoryChanged`. That initialization path also explicitly
calls `Update Visualizations`. Thus a stock Blueprint subscription exists;
periodic polling is not required merely to bridge native-to-Blueprint access.
The mapping alone does not establish the delegate's signature type name or
every broadcast site. A consumer must still verify quantity-change coverage,
initial synchronization and save/load lifecycle, and avoid duplicate bindings.

The stock serialized delegate reference resolves to
`/Script/Voyage.InventoryDelegate__DelegateSignature`; its callback has no
parameters. The delegate property owner is
`/Script/Voyage.VoyageBaseInventoryComponent`. In the Diesel Refinery,
`ReceiveBeginPlay` performs the binding and initial visualization update.
The exact inherited `/Script/Voyage.PersistentInterface:OnPersistentActorPostLoad`
event (integer `Version` parameter) defers a visualization update with
`DelayUntilNextTick`; that post-load branch does not itself rebind the delegate.
Railgun subsequently validated this event-driven pattern in the game for
stack additions/removals, deposit-all, independent gun instances, and empty
or partial inventories across save/load. The owning
[checkpoint](../mods/Railgun/ARCHITECTURE.md#current-game-validated-checkpoint)
records the release and installation evidence. These tested paths do not
establish every native broadcast site or other modules' lifecycle contracts.

### Manual transfer between character and module inventories

**Static facts** describe the reflected API and ownership path; **runtime
facts** cover normal character-to-module transfer, not every inventory subclass
or combination of stacks. The mechanism does not depend on a particular mod.

**Static facts.** For a character-to-module action, the source is the character's
`VoyageBaseCharacter.WeightInventory`; the target is the physical module's
`GetInternalInventory()` result. Capture the character before possession and
retain that owned reference: querying the possessed pawn afterwards selects
the operator station, not the character. Do not find these owners by periodic
world scans or infer them from whichever inventory window is open.

The native slot transfer is called on the **destination**:

```text
moved = destination.TransferSlot(source, sourceSlot, -1, requestedAmount)
```

`Source`/`SourceSlot` identify the live source entry; `TargetSlot=-1` requests
native destination-slot placement. The return value is the amount moved, not
the remaining count. Receiver and argument direction are part of the contract;
do not infer them from another transfer method's name. Using the native transfer
avoids implementing a separate remove/add transaction or synthesizing slot data.

The established implementation pattern obtains indices using
`source.FindSlotsByItem(exactItem)`, snapshots them in a function-local array,
then reads each current slot with `GetSlot` before transferring. Recheck item
identity and quantity, cap each positive request by the remaining transfer
budget, and check that the returned count is positive and no greater than the
request before continuing.
Keep the snapshot, remaining budget, current slot and moved amount local to
this invocation. Inventory callbacks can re-enter availability logic during a
mutation; transaction scratch state must not share the actor members that
those callbacks update.

Eligibility is checked before mutation and again on execution, not only when
rendering the action hint. The module's item validator must accept the exact
item; native transfer remains responsible for its inventory transaction and
capacity handling. A count budget is distinct from native mass/slot limits.
See the [validator argument caveat](#component-and-interaction-contract): its
inventory argument is not guaranteed to be the module's destination inventory.

**Runtime facts.** Character-to-module slot transfer updates the displayed
count and action availability and respects the consumer's item-count limit.
Both source and target
`OnInventoryChanged` subscriptions keep the availability cache current without
a new polling timer. Initialization explicitly refreshes it; exit/EndPlay and
re-entry unbind old subscriptions. The action's `Started` handler is the single
mutation entry point; its provided-action delegate does not duplicate it.

**Limits and reasonable inference.** The earlier symptoms of reversed transfer
and occupied weight with no visible contents demonstrate that plausible
inventory calls are not enough to validate a transaction. They do not prove
that every other transfer API is broken, or uniquely identify the cause of
every inconsistent state. Native slot transfer is the established choice here;
exhaustive partial/split-stack conservation, failure paths and multiplayer
remain outside confirmed runtime coverage. Function-local scratch state is a
defence against synchronous callbacks, not proof that all delegates broadcast
synchronously on every native path.

See [action/hint ownership](vehicle-and-hud-modding-patterns.md#action-execution-and-action-presentation-are-separate-contracts).
Optional implementation examples (Railgun):
[transfer and lifecycle](../mods/Railgun/Source/RailgunRuntimeGenerator/RailgunReload.cpp)
and [native inventory mirror](../mods/Railgun/Source/Voyage/VoyageBaseInventoryComponent.h).

### Native removal and compensating resource additions

Bounded native inspection on the fingerprint above establishes that
`VoyageBaseInventoryComponent.RemoveItem` returns the number actually removed,
not the remaining inventory or the unfulfilled request. The weight-limited
subclass uses the same implementation. For a request of one item, return `1`
means success and `0` means nothing was removed. The requested slot is preferred,
not exclusive: removal can continue through other matching-item slots. Preserve
the notification-enabled call path when visual consumers subscribe to inventory
changes; do not directly mutate the serialized item map or a HUD count.

`VoyageModuleComponent.AddResource` returns the accepted resource delta, not
the new balance. Its native adjustment path can clamp the resulting resource
amount to a capacity-derived limit (unless its bypass branch applies).
Consequently, adding back a debit is not an unconditional rollback contract:
check the returned delta and the actual resource balance against the captured
pre-debit value, with an appropriate numeric tolerance.

These are static native contracts, not proof of an atomic inventory/energy
transaction or of a complete weapon firing path. A consumer must validate
prerequisites before spending, prevent reentrant double spending, handle failed
debits and compensation explicitly, and obtain gameplay validation. Do not
transplant executable addresses into runtime code.

Reproduce by locating the reflected native registration with
`Invoke-VoyageExecutableInspector.ps1`, then following the bounded thunk,
constructor/vtable and implementation with `Inspect-VoyageNativeMemberAccess.py`.
Static evidence is retained below ignored
`artifacts/railgun/ammo-fire-native-20260928/`; revalidate on a changed fingerprint.

### Component and interaction contract

Static inspection on the fingerprint above identifies the displayed Diesel
Refinery as `/Game/Blueprints/Modules/Generators/BP_Module_DieselGenerator`;
Portable Petrol Refinery is `BP_Module_PetrolGenerator_Portable` in the same
directory. These are inventory references, not donors for fuel-conversion
behavior in an ammunition container.

Both have an SCS `VoyageInventoryWeightLimitedComponent` with these serialized
settings: `Type=Container`, `Access=ReadWrite`, `bAllowBeyondWeightLimit=false`,
`AcceptedItemCategories=[Organic]`, `bAllowFiltering=false`,
`DepositAllCategoryFilter=[DA_ItemCategory_Organic]`,
`bAllowNearbyQueries=false`, and `bAutoCloseHudWhenEmpty=false`.
The weight component derives from `VoyageBaseInventoryComponent` and exposes
float `MaxWeightLimit` separately from the base's integer `Capacity`.
Do not mistake an inventory slot limit for an item-count or mass limit.

The Diesel Refinery actor also implements the native
`/Script/Voyage.VoyageInventoryItemValidatorInterface`. Its Blueprint event is
`ValidateItem(VoyageBaseInventoryComponent* Inventory, VoyageItem* Item,
bool& bIsValid)`; the stock body returns whether `Item.Category` is Organic.
This provides a concrete candidate for item-identity filtering in another
module. `DepositAllCategoryFilter` must not be assumed to be an exact-item
acceptance rule; the inspected stock object uses both category settings and
the validator interface.

A subsequent Railgun runtime probe established an important argument boundary:
the actor's `ValidateItem` was called with the player's `WeightInventory` and
the exact accepted ammo object. An added `Inventory == module-owned inventory`
condition returned false and rejected that call even though item identity
matched. Do not assume this argument is always the destination container.
For an actor-specific item whitelist, compare the item identity without that
destination-pointer assumption, as the stock refinery predicate also ignores
the inventory argument. This observation does not establish every native call
site or prove that changing the predicate alone fixes all transfer failures.

Diagnostic state must also be owned explicitly: creating a new overlay on every
validator call covers earlier observations with fresh default text. An apparent
unsampled open event in that topmost widget is not evidence that the event never
ran. Reuse bounded diagnostic UI, preserve independent event samples, and never
make the gameplay return path depend on successful diagnostic widget creation.

The Diesel Refinery's `InteractiveObjectComponent` uses `WidgetOverlay`,
`PartId=100`, and `/Game/Data/UI/OverlayWidgets/DA_Widget_Container`.
Its SCS node owns a child `BoxComponent` (`Box1`): the serialized collision
profile is `Interactive`, object type is `ECC_GameTraceChannel2`, and the
`Interact` response is `Block`. The scene interaction component is not itself
a ray-hit shape. Preserve both the acquisition shape and its attachment to
the interaction provider; an overlay reference and part ID alone are not a
complete stock interaction assembly.
`InteractGetInventory` returns `ModuleComponent.GetInternalInventory()` for
that part. The inventory is a component of the persistent module actor, not
of an operator pawn. In its setup path, when over-limit storage is disabled,
the actor calls `SetMaxWeightLimit` with the component's `MaxWeightLimit`.

Runtime Railgun evidence subsequently distinguished object identity from
effective capacity: the returned and authored inventories were the same live
`VoyageInventoryWeightLimitedComponent`, its direct `MaxWeightLimit` read was
`23.400002`, but the UI showed `100 kg` and accepted more than six 3.9-unit
items. Item-only validation had restored successful transfer. Therefore neither
the wrong-inventory hypothesis nor float display noise explains that entire
capacity discrepancy.

Bounded native inspection on the same fingerprint confirms that
`SetMaxWeightLimit` stores the base field and then runs a separate recomputation:
it initializes another internal value from that field and adds entries from a
modifier collection. Direct serialized property assignment does not execute
that setter path. The stock setup call is consequently meaningful and must not
be dropped as an apparent redundant assignment. User gameplay confirmation of
Railgun `build-20260927-093742` established that calling the native setter from
the module actor's `ReceiveBeginPlay` fixes the prior 100-kg/effective-capacity
problem. This implementation reuses the existing event path and neither clears
nor recreates the inventory. It does not implement a separate post-load hook.
The UI/admission consumers of the derived value have not been independently
traced here. Do not write native offsets or destroy excess inventory contents
when applying a lower limit.

The diagnostic-free implementation was subsequently game-validated, including
inventory persistence through save/load. Its release evidence is
`artifacts/railgun/build-20260927-234904/release-manifest.json`; installation
evidence is
`artifacts/installations/Railgun/20260927-235200-build-20260927-234904-9680093e/install-manifest.json`.
The coder relayed user confirmation of magazine behavior, the capacity limit,
diagnostic removal and persistence. This does not enumerate every possible
batch/single-transfer boundary or validate other modules by analogy.

The stock assembly and bounded Railgun runtime results do not validate every
replacement or deposit route. A mod must still verify native inventory
discovery, validator dispatch for every supported deposit route, interaction
selection, and save/load of the actual instance. A weight budget calculated as
six times the item's actual weight (23.4 for six 3.9-unit items) needs boundary
tests: six at once, six one by one, 5+1, rejection of a seventh, removal and
refill. Floating-point rounding and native partial-transfer behavior have not
been established by this static inspection. Never silently accept extra items
or discard rejected ones to compensate for a failed boundary test.

Reproduce with `Get-VoyageAssetSummary.ps1` for the exact refinery's components
and `ValidateItem` function, `Get-VoyageAssetJson.ps1` for component defaults and
implemented interfaces, and `Inspect-VoyageAsset.ps1` for bounded pseudocode and
the `VoyageBaseInventoryComponent` / `VoyageInventoryWeightLimitedComponent`
mapping queries. Raw evidence remains under ignored artifacts.
