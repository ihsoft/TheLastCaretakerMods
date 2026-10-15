# Container lid labels through DML: feasibility research

## Scope and current decision

Research only, 2026-10-13. The accepted installed ContainerLidLabel v3-test
remains unchanged. Investigate the same name-and-side-icons behavior through an
additive DML entry actor without replacing the stock Large Blueprint package.
No implementation, build, installation or runtime experiment was performed.

Conclusion: a credible additive runtime design exists, but complete lifecycle
coverage and initialization ordering are not yet proven for Large containers.
This is a separate prospective architecture, not a validated replacement.

## Current evidence

Fresh fingerprint: Steam 25880952, executable SHA-256
16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44.
The public fingerprint, stock Blueprint summary, narrow full JSON readback and
mapping inspection tools were used. Temporary reports are below
`Tmp/container-lid-label/dml-*`.

- Stock BP_Inventory_PlayerBox bytecode subscribes to
  VoyageBaseInventoryComponent.OnInventoryNameUpdated and OnInventoryIconChanged
  through multicast delegates. Current mappings also expose Name,
  PrimaryIconIndex and SecondaryIconIndex. The Blueprint calls
  GetPrimaryIconIndex and GetSecondaryIconIndex.
- Current VoyagePersistentSubsystem mappings expose OnActorSpawned,
  OnActorDestroyed, OnActorRestored, OnActorUnloaded, OnActorAttached and
  OnActorDetached. Mapping property presence alone does not establish delegate
  signatures, Blueprint assignment flags, broadcast coverage or event order.
- The repository's existing DML runtime injection work establishes a loader-owned
  ModActor startup path. PalletContainerSnap's one-time data-asset property
  injection is not proof of per-container visual attachment or save lifecycle.
- The current lid mod already uses an independent widget and two planes, reads
  the inventory name and shares stock dynamic icon materials. Its visible layout
  and name-plus-icons policy were accepted in game. Currently its hooks are
  Blueprint overrides, which cannot simply be installed on an arbitrary live
  stock instance by starting a DML actor.

## Proposed architecture, not implementation

1. An additive `/Game/Mods/<ModName>/ModActor` subscribes to suitable world
   lifecycle events, filtered to the intended Large class. If loader startup can
   occur after objects exist, enumerate existing Large actors once after binding,
   with duplicate suppression. No recurring world scan is selected.
2. Add one owned visual helper per matching container. Attach its widget and
   icon planes to the actual PlayerBox_Lid and use the accepted local geometry.
   Keep original count, bar and side label untouched.
3. Initialize from current inventory state; subscribe to name/icon changes
   without removing stock subscribers. Keep the lower name visible independently.
   Share stock MIDs only after their readiness is established, or derive icon
   state from the same stock selection contract without relying on callback order.
4. Handle actor unload/destruction and reload without duplicate helpers or
   retained dead references. Treat helpers as transient presentation; verify the
   game's persistence behavior rather than assuming attached helpers are excluded.

This would ship only owned additive packages: no stock Large override, relocated
stock snapshot or stock widget replacement. DML itself remains a dependency;
'without patch' means this mod no longer contributes a stock-package override,
not that the complete loader environment contains no overrides.

## Unresolved discriminators

- Establish exact delegate signatures/assignment flags for the selected world
  events, and whether they cover restored, streamed and newly fabricated Large
  actors after DML startup. Attached-only coverage is insufficient proof.
- Establish readiness relative to stock BeginPlay, widget initialization and MID
  creation. Independent subscribers must not copy stale visibility or null MIDs.
- Verify one helper per actor after load/streaming, correct teardown and absence
  of persistent save changes.
- Prove the candidate against stock containers with the existing lid override
  removed under a recoverable migration. Running both would mask dependencies
  and can duplicate presentation; no migration is authorized by this research.

A later minimal discriminating test would observe lifecycle events and add one
marker/helper to an existing saved box and one newly created box, then reload.
Success supports the additive architecture; missed actors or premature callbacks
identify which lifecycle boundary needs work before building the full display.
This test is proposed only, not requested or implemented now.

## Tradeoff

Runtime assembly removes this mod's duplicate stock package and version-bound
relocated asset, reducing package-provider conflicts. It adds loader startup,
per-instance ownership, subscription and cleanup responsibilities. Game field
and event contracts remain fingerprint-sensitive. This is materially more work
than the completed small override, but it can reuse its proven visuals and
separate name/icon presentation. No measurable performance claim is established.
