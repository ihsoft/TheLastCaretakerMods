# ElectrifiedBoat

ElectrifiedBoat makes an exact stock Electric Wall Socket built on a Boat join
that Boat's electric network. Off-Boat electric sockets retain stock pairing,
and Fuel, Gas, Water, and subclass sockets are outside the mod's scope.

The asset-only mod owns one additive entry point:
`/Game/Mods/ElectrifiedBoat/ModActor.ModActor_C`. It ships no native runtime
DLL and does not override a stock package.

## Runtime contract

At BeginPlay the ModActor binds the persistent subsystem's
`OnActorAttached(Child, ParentComponent)` delegate. For an exact electric wall
socket on a welded-root Boat, the handler resolves the wall actor, its module,
and the Boat once. It then inspects only that Boat's directly owned actors and
uses the first suitable exact stock electric external-port actor. The stock
actor's `GetParentModule` result and native `GetModuleFromActor` identify the
Boat electric module.

After the complete preflight succeeds, the mod clears any existing wall-socket
pair symmetrically, copies the live reference port's complete `Port` value and
five registration flags, assigns the stock-derived socket ID, disables the
stock pair-search Tick, and calls `AddExternalSocket` for the existing wall
view. It never copies a reference ID, transform, cable reference, or mesh and
never joins a Boat secondary group. EndPlay cleanup is guarded by the exact
socket and expected owner recorded at registration.

If the resolved Boat has no suitable stock electric external-port reference,
the handler leaves the socket unchanged. The runtime has no actor Tick,
recurring timer, startup/world scan, diagnostic UI, or input binding.

The attachment event handles both newly built and restored sockets.
`OnActorRestored` is a separate reflected delegate, but this mod does not bind
it.

## Build and validation

From the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\mods\ElectrifiedBoat\Build-ElectrifiedBoat.ps1
```

The producer fingerprints the installed game, builds the editor-only mirror
and generator, creates and cooks the single additive ModActor package, packages
the IoStore triplet, and runs semantic validation. It writes a fresh result
below `Tmp/ElectrifiedBoat/` and does not install it.

To validate an existing candidate:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\mods\ElectrifiedBoat\Test-ElectrifiedBoatCandidate.ps1 `
  -PackageRoot .\Tmp\ElectrifiedBoat\build-<identity>\package
```

## Distribution

The distributable archive requires DML and contains
`ElectrifiedBoat.pak/.ucas/.utoc` at the archive root, without a `LogicMods`
wrapper directory, `_P` suffix, loader binaries, or an `.autoload` sidecar.
Copy the triplet together into `Voyage/Content/Paks` or a mounted user-mod
subfolder. If the mod is not already enabled, activate its virtual entry point
with `DML add ElectrifiedBoat`. The archive itself does not edit DML
configuration. Do not keep another version of ElectrifiedBoat enabled at the
same time; duplicate ModActors would process the same attachment events.

## Compatibility boundary

This source targets Steam build `25191271`, executable SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`,
game runtime UE `5.8.1`, generating editor UE `5.8.2`, and DML v0.6. Other
Boat layouts, dismantling edge cases, loading after removing the mod, and
absence of a suitable stock electric external port are unsupported or
unvalidated boundaries.
