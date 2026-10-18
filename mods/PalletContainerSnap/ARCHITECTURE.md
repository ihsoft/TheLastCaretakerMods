# PalletContainerSnap architecture

This paused investigation develops placement on four authored positions of
the stock Donk pallet. The owned placement generator derives a preview from
the stock Default generator's setup and validation path, then changes the
candidate transform only for the exact pallet. Its four transforms are authored
in `Contracts/pallet-container-snap-contract.json`.

Candidate routes include an owned container, a narrow stock Large item
override, and a once-per-world runtime assignment of an owned generator. They
have different installation and identity boundaries; none establishes full
pickup, occupancy, save/reload, or independent inventory behavior. The ignored
[BACKLOG.md](BACKLOG.md) records the current checkpoint and active gates.
[PLACEMENT-RESEARCH.md](PLACEMENT-RESEARCH.md) keeps the mod-local working
investigation. [Mod rules](MOD-RULES.md) retain stable boundaries.

## Four-slot placement contract

The four authored roots are `PlayerBox1` through `PlayerBox4` in
`Contracts/pallet-container-snap-contract.json`. They have identity rotation
and scale. In pallet-local Unreal centimeters, their source order is:

1. `(60.6917798519, 57.0609331131, 50.9266495705)`
2. `(60.6917798519, -59.5599830151, 50.9266495705)`
3. `(-59.2448532581, -59.5599830151, 50.9266495705)`
4. `(-59.2448532581, 57.0609331131, 50.9266495705)`

`SetupLocation` calls the exact stock Default implementation first. For an
exact stock Donk pallet hit with snapping enabled, it transforms the slots,
selects the strict nearest to the fresh `Out Hit.ImpactPoint` by squared world
distance, copies pallet rotation, and attaches to the hit component. Ties keep
source order; occupied slots are not skipped. With snapping disabled it uses
the parent `CalculateOffset` at the hit position and normal. Outside the exact
pallet class the stock result remains untouched.

The exact pallet support exception requires a fresh blocking hit whose
component is reference-equal to the attachment target. It preserves inherited
overlap, water, collision, surface, and attachment checks; it does not set
`CanPlace`. The owned canary's distinct item name and its preview/final
positions are the visible ways to identify this route. The current
experiment's unresolved runtime gates and its recovery checkpoint belong to
the local ignored backlog.
