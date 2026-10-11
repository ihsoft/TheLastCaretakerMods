# Game-derived contract registry

The tracked mod contains hand-written generator source, editor-only identity
mirrors, scripts, and documentation. Extracted assets and raw inspection output
remain in ignored storage and are not source inputs.

## Version gate

- Steam build ID: `25191271`
- executable SHA-256:
  `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`
- Unreal family: `5.8`; generating editor: `5.8.2`
- revalidate all reflected identities, bytecode assumptions, and cooked imports
  after any fingerprint change

## Stock identities and lifecycle

- exact wall class:
  `/Game/Blueprints/Modules/Utility/Wireless/BP_WallSocket_Electric.BP_WallSocket_Electric_C`
- exact Boat reference actor:
  `/Game/Blueprints/Modules/BP_AttachmentVirtualSocketActor_Electricity.BP_AttachmentVirtualSocketActor_Electricity_C`
- generated-class wall member: `VoyageModuleSocketView`; the similarly named
  `_GEN_VARIABLE` export is an SCS template, not the runtime member
- `VoyagePersistentSubsystem.OnActorAttached` is Blueprint-assignable with
  `(AActor* Child, USceneComponent* ParentComponent)` and broadcasts after a
  successful native attachment
- `OnActorRestored(AActor* Actor)` is separately Blueprint-assignable but is
  not bound by ElectrifiedBoat
- the runtime binds `OnActorAttached` for both newly built and restored wall
  sockets; completeness across every Boat/save topology is not established

## Boat ownership and external registration

- `VoyageLevelInstanceComponent.OwnedActors` is a weak Actor array. The editor
  mirror preserves that layout. A generated Blueprint helper reads each element
  through `EX_ArrayGetByRef`; a weak/null/weak `ProcessEvent` canary runs before
  the asset is saved.
- The stock electricity external-port actor resolves its parent through
  `GetParentModule(AActor*& Actor)`. Native static
  `VoyageMiscBlueprintFunctionLibrary.GetModuleFromActor(AActor*)` returns the
  target `VoyageModuleComponent`.
- Stock `AddExternalSocket` removes the same socket from its prior owner's
  socket list, changes and initializes ownership, then adds it to the new owner.
  The inspected path does not reject an existing `ConnectionCable` or
  `ConnectedToSocket`; ElectrifiedBoat preserves those references.
- Stock wall post-load calls
  `SetSocketID(Conv_StringToName(GetObjectName(Self)))`. ElectrifiedBoat follows
  that path and does not add a cross-socket collision policy.
- The complete live `FModuleSocketIOData Port` and five reflected registration
  booleans are copied from the selected stock reference. Socket ID, transform,
  cable references, and visual mesh are not copied.
- The mod records each successful external registration and removes it at
  actor EndPlay only when the current owner is still the recorded module.

## Evidence ownership and runtime boundary

Detailed current-fingerprint evidence remains under ignored
`Tmp/electric-wall-socket-research/`; reusable stock asset snapshots remain in
the registered asset cache. Shared contracts and pitfalls are documented in:

- [Actor attachment and save restoration](../../docs/vehicle-and-hud-modding-patterns.md#actor-attachment-and-save-restoration)
- [External socket registration](../../docs/boat-resource-socket-architecture.md#external-socket-registration)
- [Reflected weak-object arrays](../../docs/voyage-cooked-asset-toolchain.md#reflected-weak-object-arrays)
- [DML actor identity and physical installation](../../docs/voyage-cooked-asset-toolchain.md#dml-actor-identity-and-physical-installation)
