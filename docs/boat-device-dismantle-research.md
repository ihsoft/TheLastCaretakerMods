# Built-in ship devices: creation and removal

Research stopped at the user's request on 2026-09-13. Experimental mod,
reader, builds and task-specific diagnostic output were removed.

These findings were observed on Steam build 25191271, UE 5.8, executable SHA256
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
They require revalidation after a game update.

## Creation mechanism

`BP_Boat_Base.LevelInstance` is a `VoyageLevelInstanceComponent` whose
`LevelToLoad` points to `/Game/Maps/Boats/TestBoat`. The built-in Fabricator
(`BP_Module_Fabricator_C_1`) and Recycler (`BP_Module_Recycler_C_2`) are actors
in that map, not simply components of the ship Blueprint. They use the same
classes/item assets as player-built equipment.

At runtime the ship receives a separate level instance (observed package:
`TestBoat_LevelInstance_2`). Its devices acquire the boat as Owner, attach to
`VehicleMesh`, and receive `OwnedByLevelInstance`, `NoRecycle`, `NoDestruction`
and `PersistentRuntime` tags. The instance suffix is not a stable identifier.
The game has one ship; other vehicle types can have multiple instances.

## Removal limitations

- The stock recycler/dismantler check rejects `NoRecycle`. Removing that tag
  only from the two built-ins enabled normal dismantling; the user reported
  successful deletion without visible artifacts. Removing `NoDestruction`
  was unnecessary for that result.
- After save/load the devices returned. Persistent component IDs alone do not
  make removal of these level-owned actors persistent. The exact distinction
  between missing deletion records and loader behavior was not fully resolved.
- The native loader has a conversion path for actors with a primitive root,
  a simulating physics body and a persistent-actor component. It tracks source
  names in `SpawnedPersistentRuntimeActors`, marked SaveGame, and removes the
  source actors after conversion. The name lookup is inside that conditional
  path: it is not a universal deletion list for stationary built-in devices.
- A menu-only probe successfully moved the original map's Fabricator in memory.
  After entering gameplay, the ship's separate instance retained stock
  coordinates and lacked the probe marker; the modified source actors were
  no longer observed. This disproves that tested transfer method, not every
  possible early interception or forced-retention approach.
- Deleting devices after scene activation is insufficient for the requested
  behavior: player-built equipment may already occupy their locations.

A potential solution must prevent their activation at those positions, through
an exact map-instance edit or interception before initialization/collision.
No suitable early Blueprint hook, safe conversion API, or complete solution
was established. Broader quest, resource and save-integrity effects were not
validated. No further experiment is planned.
