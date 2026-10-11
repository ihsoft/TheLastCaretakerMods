# ElectrifiedBoat rules

These rules apply to `mods/ElectrifiedBoat`. Repository rules in
`../../AGENTS.md` also apply.

- The mod is additive and owns only
  `/Game/Mods/ElectrifiedBoat/ModActor`. Never override the stock wall-socket
  Blueprint.
- Process only exact instances of the stock Electric Wall Socket class. Fuel,
  Gas, Water, and mod subclasses keep stock behavior.
- Bind `VoyagePersistentSubsystem.OnActorAttached` once and register a socket
  synchronously from that event. The same event handles newly built and
  restored sockets. `OnActorRestored` exists as a separate reflected signal
  but is not bound by this mod.
- Resolve the wall actor, module, and welded-root Boat once. Enumerate only the
  resolved Boat's `VoyageLevelInstanceComponent.OwnedActors`, stop at the first
  suitable exact stock electric external-port actor, and resolve its parent
  module through stock `GetParentModule` plus native `GetModuleFromActor`.
  Never scan the world or poll for ownership or lifecycle state.
- If a resolved Boat has no suitable stock electric external-port reference,
  return without mutating the wall socket. This is the explicit boundary for
  an unsupported Boat topology.
- Keep the runtime actor free of Tick, timers, startup discovery, diagnostic
  UI, and input bindings. Bounded component and owned-actor iteration is
  allowed only during one attachment event.
- Never write a Boat or mod object into the stock `PairedModule` SaveGame
  property. Clear both ends of an existing pair before external registration;
  do not join a Boat secondary group.
- Copy only the reference port's complete `Port` value and five registration
  booleans. Initialize the wall socket through
  `SetSocketID(GetObjectName(WallActor))`, then transfer its existing view with
  `AddExternalSocket`. Do not copy IDs, transforms, cable references, or mesh.
- Retain the transient actor/socket/expected-owner registry until actor
  EndPlay. Call `RemoveExternalSocket` only if the registered view is still
  owned by the recorded module; never edit module socket arrays manually.
- The editor mirror must preserve native identities exactly. In particular,
  `OwnedActors` remains a weak Actor array and is read through the validated
  by-reference Blueprint opcode and generator canary.
- Static validation must prove one owned package at the exact ElectrifiedBoat
  virtual entry class, the lifecycle/registration calls, disabled Tick, and
  absence of recurring diagnostics. Compile/cook/container validation is not
  gameplay validation.
- Distribution packaging must contain only the unsuffixed IoStore triplet at
  the archive root. Do not add a `LogicMods` wrapper, bundle a loader, modify
  DML settings, or include obsolete sidecars. Activation is the separate
  `DML add ElectrifiedBoat` user step when the entry is not already enabled.
