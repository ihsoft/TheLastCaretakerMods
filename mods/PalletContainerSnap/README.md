# PalletContainerSnap

PalletContainerSnap is a four-slot runtime canary for placing owned large
storage-container subclass on the unchanged stock Donk pallet through ordinary
Place. It keeps the stock large container's inventory and interaction behavior.

## Build

From the repository root, with the game closed only if a later manual install
is planned:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods\PalletContainerSnap\Build-PalletContainerSnap.ps1
```

The last pinned Steam build is in `game-build.json`. If the public builder
stops only at that comparison, retry it with `-IgnorePinnedGameVersion`.
Current reviewed mappings, stock inputs, and package validation remain required;
test a successful candidate in game before updating the pin.

That default remains the reproducible `0.6` owned-canary producer. The one-off
`0.7` same-path stock FortiHold experiment consumed the retained,
hash-validated installed `0.6` release
archive as its stable four-package/control-registry input, independently of
which version is currently active. It extracts the exact current-game stock
`DA_Item_PlayerBoxLarge`, proves a canonical no-op roundtrip, preserves its
unversioned header, versions, custom versions, original properties and imports,
and adds only the actual owned generator instance. The published UAssetAPI
writer requires an explicit absolute `-PowerShellHostPath` to the exact
PowerShell 7.6 / .NET 10 host because the reviewed game
mapping cannot contain the mod-authored generator class. Its owning writer adds
one validated in-memory class schema for that class only; it never writes or
changes the reviewed mapping. The resulting candidate contains the four
unchanged canary packages plus the one selected stock item and retains the
unchanged two-record canary registry. It does not add a stock registry record.
Its legacy unsuffixed build scope is now rejected because stock-package
overrides must use an `_P` container stem.

The experimental producer is intended to validate the installed game
fingerprint, generate and cook the four owned packages, write a two-record
plugin-local primary-asset registry, verify exact package and registry
readback, and prepare a schema-3 candidate below a fresh
`Tmp/PalletContainerSnap/build-*` directory. It does not install, launch the
game, or replace the global asset registry. Only a terminal `status=prepared`
result establishes that producer boundary; partial outputs are evidence for
their completed stages, not a release. The current source identity is
`0.6-canary-four-slot-snap-mode`. The user confirmed its positioning, four
slots, and pallet magnet functionality in the game. Pickup release,
save/reload, independent inventory persistence, and an independent `0.6`
occupied-slot check remain untested, so this is not yet a production placement
policy.

New stock-override builds do not use the legacy unsuffixed `StockLargeRoute`
scope. The byte-identical priority discriminator is:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods\PalletContainerSnap\Build-PalletContainerSnap.ps1 `
  -Scope StockLargePriority -ArtifactVersion 0.8-canary-stock-large-priority
```

It consumes a retained, manifest-validated control archive and emits the same
container bytes under `PalletContainerSnap_P`. It changes no cooked package,
registry, generator or item field. That historical build scope is an experiment;
new installation uses the current release manifest and the direct installation
path below. Never leave differently named duplicate container stems active.

Installed `0.8-canary-stock-large-priority` is the current runtime-confirmed
stock Large FortiHold route: the user reports that it now works, without
separately identifying a pre-existing saved instance versus a newly fabricated
instance. Runtime-confirmed
`0.6-canary-four-slot-snap-mode` remains its comparison control.
The user tested both a pre-existing and newly fabricated standard Large
FortiHold under `0.7`; both retained stock placement behavior. Exact release,
installation, runtime, and rollback boundaries are recorded in `BACKLOG.md`.

The current installed working runtime experiment was produced with
`-Scope RuntimeInjection -ArtifactVersion 0.9-canary-runtime-injection`. It
keeps the owned four-package control, adds one loader entry actor, and omits the
stock Large item package. On gameplay-world `ReceiveBeginPlay`, that actor
constructs the owned generator with exact stock Large as Outer and
unconditionally dispatches the UE generic object-property setter once. It does
not read the field, poll, scan, reassert, patch common Place, or replace the
stock item/default generator. Its `.autoload` sidecar uses the `_P` physical
stem while explicitly naming `/Game/Mods/PalletContainerSnap/ModActor.ModActor_C`.
This is a runtime diagnostic candidate, not a production conflict policy. Its
accepted evidence is under
`Tmp/PalletContainerSnap/build-20261009-runtime-injection-r3`; the result honestly
records post-cook verifier repair rather than a fresh full current-script rerun.
The user reports that its standard Large FortiHold placement works with the
Snap-on four-slot and Snap-off free-placement instructions. That confirms the
combined route for the recorded game fingerprint, not separately loaded versus
new items, save/reload, world travel, magnet behavior, occupied slots, or the
individual diagnostic markers.

The installed RuntimeInjection producer identity is
`0.10-canary-human-seed`. It retains the Large route and adds a separate owned
Human Seed generator plus two once-per-world property dispatches against exact
`DA_HumanSeed`: `bAllowPlacement=true`, then the generator object whose Outer is
that item. The Seed generator is a sibling directly under stock Default. With
Snap enabled it traces only the recognized pallet component along pallet up at
the nearest authored cell, preserves that cell's XY, and derives height from
the live placement component's transformed preview bounds. Snap disabled uses
the raw hit and normal with the same bounds-derived height. Missing preview
components or a missing exact-component surface hit preserves stock parent
behavior. Void setter dispatch, Place availability, height, validation and
magnet/save behavior remain runtime gates. The candidate has now been built and
structurally reviewed as an exact-six, no-stock-package release; its public
installer and installed-hash readback gates passed. It is installed but remains
runtime-not-run pending the gameplay test recorded in `BACKLOG.md`.

The next test-only producer identity is
`0.11-canary-human-seed-spawn`. It leaves the `0.10` placement and injection
contracts unchanged and added one manual test-spawn trigger to the
owned loader actor. With a valid local player controller and pawn, one exact
stock `BP_Item_HumanSeed` is requested 200 cm forward and 100 cm above the pawn
using `AdjustIfPossibleButDontSpawnIfColliding`; otherwise it only emits a
single failure diagnostic. The trigger does not consume input, repeat, poll,
retry, scan, or spawn automatically on world load. This is test acquisition,
not a production recipe or proof of native factory parity.

The bounded successor `0.12-canary-place-diagnostic` keeps both placement
generator packages canonical to `0.11`. F6 still spawns one stock Human Seed
and now retains that successful actor in a strong reflected reference. Manual
F7 reports whether it lacks `VoyageItemInterface`, returns a null
item, returns exact `DA_HumanSeed`, or returns another item, with available
actor/item object paths and friendly class display names. The object paths are
identity evidence; the class display names are explicitly non-identity labels.
Manual F9 is permitted only while the
user is holding that saved actor; after the same exact-item gate it dispatches
the stock first-person-character `Place(Source, Controller)` function. Its true
return is unconditional and means only dispatch; preview plus the existing
owned Seed generator callback are the downstream evidence.

The `0.13` owned Seed child proved its spawn path but crashed on pickup because
it implemented `GetItem` without a class-owned `VoyageItemInterface.GetItemName`.
Bounded minidump/executable evidence identifies the exact native GetItemName
thunk; the stock container control returns empty Text. Installed `0.14` adds
only the same empty-Text override. It does not add speculative
no-op implementations for the interface's other lifecycle methods and remains
runtime-unvalidated until F6 spawn plus pickup succeeds in the game.

These results use one owned, initially collapsed single-TextBlock UMG panel.
The panel does not take focus, change input mode or cursor, bind Tick, or add
buttons. `PrintString` is supplementary only: local UE 5.8 compiles it out in
Shipping/Test unless shipping logging is enabled, and that Voyage setting is
not established. No diagnostic claims a `bAllowPlacement` readback, injects an
action, confirms placement, scans, polls, or mutates Lazarus Pod.

For the diagnostic runtime test, press F7 before F6. The panel must
show the `no saved F6 seed` state; if it does not appear, stop because the
observation/input channel itself failed. Only after that control passes, use
F6, pick up that exact spawned Seed, use F7 for the provider result, and use F9
only when the result identifies exact `DA_HumanSeed`.

## DML activation layout

DML discovers logic mods below `Content/Paks/LogicMods` and loads the
conventional `/Game/Mods/<name>/ModActor.ModActor_C`. Prepare a current
PalletContainerSnap release manifest, then validate and install it directly:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods\PalletContainerSnap\Install-PalletContainerSnap.ps1 `
  -ReleaseManifest <current-release-manifest> -ValidateOnly

powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods\PalletContainerSnap\Install-PalletContainerSnap.ps1 `
  -ReleaseManifest <current-release-manifest>
```

The wrapper selects the shared installer's `LogicMods` destination. It checks
the closed game, release contents, installed hashes and the owned catalogue
descriptor. It leaves the ZIP outside the game. `-ReleaseCandidate` retains
the selected candidate under `artifacts/installations/PalletContainerSnap-V<N>`;
ordinary experiments stay in `Tmp/`. Reinstall the previous prepared package
from its release manifest if the current experiment fails. The mod does not
edit DML's saved `ModsToLoad` map; enable it through DML's own UI.

The `0.8` priority result supports provider precedence as the successful
single-variable change, but it is not a final compatibility claim. A newly
obtained stock FortiHold and a pre-existing saved FortiHold remain unseparated
runtime gates. The control canary remains available for comparison. Normal
parser readback of the unversioned stock package requires
the same exact owned-class schema supplement; `OtherAssetsFailedToAccess` for
the loose generator import is parser diagnostic metadata, not a serialized
item property, and the verified five-package container includes that generator.

The visible canary item is **Pallet Snap Canary FortiHold**. Its visible skill
is **Pallet Snap Canary Recipe**, declared as Tier requirement 11 / cost 0,
matching the stock FortiHold skill's tier requirement. The
recipe costs one stock Alloy Frame, takes one second, and consumes no
electricity. This is a source-declared initial-tier path; actual discovery and
fabrication are runtime gates.

The plugin-local catalogue uses an exact-asset allowlisted owned policy for
this canary item. The user selected `bIsDestructible=false`,
`bIsRepairable=false`, `Quality=Poor`, and `Tag=None` as canary design values;
they are not recovered stock-container defaults. The placed actor still
inherits the stock large-container Blueprint behavior and inventory, but that
inheritance does not promise destruction/repair metadata parity. Other
fallback tags retain the fingerprinted established shared Voyage registry
policy provenance recorded by the owning policy.

## Four-slot contract

The immutable source contract is
`Contracts/pallet-container-snap-contract.json`. Its four authored roots are
`PlayerBox1` through `PlayerBox4` from
`K:/Downloads/_NewConcept/SampleModels/Pallet-4x-LargeBoxes.glb`, SHA-256
`CA7DE5B5014F9A6E551EDB79C4E8DF2A1917E9068CD48D42A6FC0A1FB08AD472`.
All four have identity rotation and scale. The repository exporter convention
maps UE centimetres to glTF as `(UE.X, UE.Z, UE.Y) * 0.01`, giving these
pallet-local UE centimetre locations in deterministic source order:

- `(60.6917798519, 57.0609331131, 50.9266495705)`;
- `(60.6917798519, -59.5599830151, 50.9266495705)`;
- `(-59.2448532581, -59.5599830151, 50.9266495705)`;
- `(-59.2448532581, 57.0609331131, 50.9266495705)`.

A bounded comparison against fresh exact stock exports showed identical
canonical triangle positions, bounds, scale, and normalized transforms for the
pallet, container base, and lid. The authored container body AABB lower bound
is about 48.466 cm while the pallet AABB upper bound is about 49.792 cm. This
rough 1.33 cm AABB overlap is not exact contact geometry; the authored slot is
preserved unchanged so the stock support and overlap checks can discriminate it
at runtime.

`SetupLocation` calls the exact stock Default implementation first. When the
trace hit actor is exactly the stock pallet class and snapping is enabled, it
transforms all four slots, selects the strict nearest to fresh
`Out Hit.ImpactPoint` by squared world distance, copies pallet rotation, and
uses the hit component as attachment target. Ties keep source order and an
occupied nearest slot is not skipped. With snapping disabled it uses exact
parent `CalculateOffset(ImpactPoint, ImpactNormal, Offset)` without a
`TransformToGridFunc` position and preserves parent rotation. Both modes set
the validation origin to selected world placement plus pallet unit world-up
times live `CheckOffset`.
Outside that class, the stock result remains untouched. The inherited native
local overlap is only an occupancy hypothesis; the host exclusion must not
exclude neighboring containers.

Gameplay snapping is not stored as a literal BoolProperty. The exact placement
component's `ToggleSnapping` bytecode treats its owned Double
`AngularGranularity_Last == 0.0` as enabled. Disabled mode stores normal
granularities in the `*_Last` fields and uses active angular/linear values 0.1;
the canary preserves the parent rotation produced under that mode.

The `0.3-canary-surface-test` adds one guarded diagnostic variable to the owned
generator. Only when `AttachToComponent` is owned by the exact stock Donk
pallet, the owned `ValidateLocation` override snapshots
`IgnoreNoFabrication`, temporarily sets it true around the exact parent call,
then restores the snapshot before forwarding the parent's out value. Outside
that pallet it calls the parent unchanged. The stock flag bypasses both the
target-hierarchy and overlap-hierarchy `NoFabrication` tag-policy paths during
that call; it is not a host-only exemption. Skeletal-mesh overlap, cable
overlap, collision object/response filtering, physics collision, bottom
support, water, and surface-presence checks remain. `IgnoreActors` and
`IgnoreAttachParentCollision` are not changed.

This experiment asks one question: if the pallet preview becomes green and can
be confirmed, tag policy was at least one contributor. If it remains red, one
or more other checks coexist; that does not prove the tag policy was irrelevant.

The `0.4-canary-checkorigin-test` keeps that `0.3` tag-policy diagnostic and
changes one additional input only for the exact pallet: `CheckLocation` no
longer carries the parent trace's hover-normal/grid delta. The authored slot,
rotation, attachment target, and inherited support, overlap, water, surface,
collision-filter, and ignore-list behavior remain unchanged. If
`Requires flat surface` remains, the authored slot/support geometry remains a
cause. If `Blocked output` becomes independent of the hovered area, the stale
check origin caused that variation. Neither outcome alone is a production fix.

The `0.5-canary-pallet-support-test` keeps both earlier diagnostics and changes
the support rule only for the owned authored slot. When the fresh Setup hit is
blocking, its actor class is exactly the stock Donk pallet, and its valid hit
component is reference-equal to `AttachToComponent`, the owned
`ValidateBottomAlignment` override preserves that complete hit and treats the
authored pallet-local slot as supported. Every other path calls the exact parent
and retains all bottom-vertex traces. This is an explicit specialized-slot
policy, including a pallet at its live inclination; it is not evidence that all
container bottom vertices geometrically contact a flat surface. It does not set
`CanPlace`, enable unsupported placement globally, or bypass overlap, water,
surface-policy, occupancy, or attachment checks.

The `0.6-canary-four-slot-snap-mode` retains that exact-pallet support rule but
broadens its owned candidate policy to the four authored slots and requested
hit-based free mode. The support exception applies only to the same fresh
blocking exact-pallet hit with a valid hit component equal to the attachment
target. It does not write `CanPlace`; physical overlap, tag policy, water,
surface policy, collision filters, and ignore lists remain inherited. The
nearest slot is deliberately selected even when occupied, so stock local
physical overlap remains the rejection mechanism rather than a second state
table.

Runtime `0.5` proved green preview and confirmation, but lifting the pallet made
the placed container fall while ordinary wall placement stays attached. No
stock magnet sound/delay was observed. Native pallet magnet ownership remains
unknown. The user reports that magnet functionality works in `0.6`; no magnet
code was added, so no native investigation or explicit invocation is needed for
the accepted current result. This does not explain the differing `0.5` result
or establish an exact sound, delay, or movement sequence. `0.6` does not replay
`AttachActor`, call `OnAttach`, or treat `OnFabricate` as a Place callback.

No per-tick runtime log is emitted. The distinct item name and the preview/final
snap location are the user-visible discriminators for generator selection and
target recognition; successful confirmation is distinguished by the placed
owned container with its independent inventory.

## Runtime gates

Compile, cook, container integrity, tagged-property and readback checks do not
establish gameplay behavior. A real-game canary must still verify:

1. the item and skill are discoverable and fabrication yields the owned item;
2. the preview snaps to the same final transform;
3. a second container is rejected while the slot is occupied;
4. pickup frees the slot;
5. moving the pallet keeps the attached container aligned; and
6. save/reload preserves attachment and each container's independent inventory.

### Real-game canary procedure

After a separately approved installation, use the normal fabrication and Place
UI only:

For `0.6-canary-four-slot-snap-mode`, use these bounded checks:

1. Find **Pallet Snap Canary Recipe** at Tier 11 and fabricate one
   **Pallet Snap Canary FortiHold** from one Alloy Frame. Missing skill/item or
   a different cost rejects the accessibility contract.
2. With snapping enabled, aim near each of the pallet's four authored box
   positions. Verify the preview chooses the nearest distinct slot and remains
   green; ties intentionally retain authored array order.
3. Disable snapping through the ordinary placement control. Aim at two
   different points on the pallet and verify placement follows the hit-based
   free position rather than a four-slot grid while retaining normal player
   rotation behavior.
4. Confirm one snapped placement. Then aim a second canary at that occupied
   slot. Rejection supports inherited physical-overlap rejection; it does not
   prove an occupancy-aware slot selector because nearest-slot selection does
   not skip occupied positions.

Pickup release, save/reload, and inventory persistence remain later production
gates. The same-slot rejection observed in `0.5` is not an independent `0.6`
occupancy proof. No further gameplay test is currently requested.
