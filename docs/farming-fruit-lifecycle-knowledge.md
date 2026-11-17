# Farming fruit ownership, growth and collection

These contracts come from inspection of stock `BP_FarmingSeed` and its fruit
Blueprint on Steam build `25982825`, with in-game confirmation that restoring a
missing full-size pear record unblocks collection and growth. They support
future farming mods and diagnosis; they do not establish that every plant has
the same failure or identify why an original record was lost.

## Attached actors and registered fruit are different sets

`BP_FarmingSeed` at `/Game/Blueprints/Farming/New/BP_FarmingSeed` owns a `Fruits`
map keyed by fruit socket identity. Each `VoyageSeedFruitGrowInfo` contains an
actor soft reference, `GrowTimer` and a fruit-info reference. `GetFruitActors`
reads this map; it does not enumerate all actors attached to the tree.

The stock update loop requires a valid fruit-info reference, resolved actor and
attachment parent equal to the tree. It advances growth/scale and enables pickup
at the ripe threshold. Fruit interaction requires `IsRipe` or an unattached
actor; collection also checks `Scale.X >= 1`. A visible full-size attached fruit
therefore need not be collectible or participating in the growth loop.

## Two different space checks

`UpdateHasSpace` uses the seed octree and `SpaceRequirement` to check neighboring
plants. Separately, `UpdateGrowing` performs a physical sphere overlap around the
tree bounds. Its ignore list includes the tree, its immediate parent and
`GetFruitActors`. An attached fruit missing from the map is not excluded merely
because it belongs visually to the tree. With a blocking mesh collision it can
stop growth; the blocked path also skips the fruit update that would ripen it.

When diagnosing "Needs more empty space", distinguish plant spacing from this
physical overlap and compare the blocking actor with map membership. Disabling
collision or forcing pickup alone does not restore fruit accounting.

## Removal scratch state

`RemovedFruits` is a non-SaveGame scratch list used by `SpawnFruits`, not an
asynchronous removal-in-progress signal. The stock pass clears it, collects keys
whose records fail validation, and removes those keys from `Fruits`. The list
can remain populated until the next pass. Growth gates can prevent that next
pass. `DropFruit` detaches the actor synchronously before marking its key.

A recovery operation must distinguish this stale scratch state from a genuinely
detached or invalid fruit. A populated removal key alone is not a reason to
reject an otherwise valid attached-fruit recovery. Reconcile only the repaired
key after successful registration; repairing one entry does not establish that
other removal markers are stale.

## Setup readiness

The stock tree loads its mesh asynchronously before `GatherFruitSockets` for a
static mesh or `SetInstancedSkinnedMesh` for the skeletal path. Actor restoration
does not by itself establish that these dependencies are ready. Validate live
sockets before editing registration. For event selection and observation gaps,
see the
[persistence lifecycle contract](vehicle-and-hud-modding-patterns-knowledge.md#actor-attachment-and-save-restoration).
