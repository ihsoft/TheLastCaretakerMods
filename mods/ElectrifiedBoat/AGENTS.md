# ElectrifiedBoat: non-obvious contracts

Repository rules in [../../AGENTS.md](../../AGENTS.md) apply. Read
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md) for the fingerprint and exact
game identities; [README.md](README.md) owns the build and installation steps.

- DML loads `/Game/Mods/ElectrifiedBoat/ModActor.ModActor_C`. Ship only this
  additive package, not the generator's stock Blueprint stubs or native mirrors.
  A physical `LogicMods` directory and `_P` suffix are not required; see the
  [DML contract](../../docs/voyage-cooked-asset-toolchain.md#dml-actor-identity-and-physical-installation).
- `OnActorAttached` handles construction **and save restoration**. Keep one
  synchronous handler, with no separate restore scan, Tick, or retry timer.
  A connected cable does not make a restored socket ineligible. The separate
  `OnActorRestored` signal is not needed here; see
  [lifecycle contracts](../../docs/vehicle-and-hud-modding-patterns.md#actor-attachment-and-save-restoration).
- Match the exact stock Electric Wall Socket class, not subclasses. Resolve
  its welded-root Boat once and use the first suitable stock electric port
  from that Boat's `OwnedActors`. Resolve the port's parent through stock
  `GetParentModule` (out parameter `Actor`) and native `GetModuleFromActor`,
  not the port view's possibly uninitialized `ModuleOwner`. No suitable port
  means a permitted nonmutating return, not a fatal error.
- `OwnedActors` is a **weak** actor array. Keep the by-reference Blueprint
  helper and weak/null/weak canary; a strong-array mirror or ordinary
  `Array_Get` is not an equivalent replacement. See
  [weak-array access](../../docs/voyage-cooked-asset-toolchain.md#reflected-weak-object-arrays).
- Boat power sharing uses `AddExternalSocket`, not secondary-group membership.
  Preserve the existing view and cable references, copy the complete live
  `Port` and five stock registration flags, and initialize the ID from the wall
  actor's own name. Do not copy the reference ID or add a collision scan.
  The exact flags and native behavior are in
  [external registration](../../docs/boat-resource-socket-architecture.md#external-socket-registration).
- `PairedModule` is saved by the game: clear both ends of an old pair and their
  `WallSocket` groups, but never put a Boat battery or mod object into that
  property. Stock `UnpairModule` alone does not perform symmetric cleanup.
- Keep EndPlay's recorded actor/socket/expected-owner association. Call
  `RemoveExternalSocket` only while the current owner still matches; removal
  does not clear the weak owner field. Never edit module socket arrays directly.
