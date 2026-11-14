# Railgun architecture

Railgun is an additive, single-player weapon mod. The persistent module owns the
physical gun and ammunition; a separate vehicle pawn owns station control.

| Asset below `/Game/Mods/Railgun/` | Responsibility |
| --- | --- |
| `Module/BP_Module_Railgun` | Persistent shell, native inventory, model components and station initialization. |
| `Station/BP_RailgunOperator` | Possession, aim, ADS, energy demand/debit, reload, firing and HUD notifications. |
| `Station/WBP_RailgunHUD` | Action hints, range/name, charge and ammunition presentation. |
| `Station/BP_RailgunTestShot` | Projectile movement, blocking-hit attack and water-crossing detection. |
| `Station/BP_RailgunTransientVfx` / `Station/BP_RailgunWaterWakeController` | Independently lived impact/fountain effects and bounded water wake. |
| `Fabricator/AmmoCassette/BP_RailgunAmmoCassette` | Native pickup carrier and composite cassette model. |
| `Inputs/BP_RailgunADSServiceEquipment` | Native equipment class with authored ADS interpolation defaults. |
| `Settings/BP_IniFileParser` | Mod-owned copy of the shared INI parser. |

The cassette and ADS equipment have no authored graphs. Their parents,
components, defaults and referenced class identities remain required. The
operator's Construction Script assigns its two input contexts.

The maintained project and synchronized BPT hold authored state. See
[BUILD.md](BUILD.md) for source layout, editor support, working copies,
DataAsset authoring, serialization and candidate validation. Native API
observations below are bounded to the inspected UE 5.8 contracts; the current
accepted source-build pin is [game-build.json](game-build.json).

## Stock aiming and input integration boundaries

The maintained character context retains Primary Action, ADS, Reload and both
look axes; the vehicle context retains Exit Vehicle. Mapping entries preserve
stock action identities and device settings. Empty mapping-profile overrides and
no alternate `SwitchGamepadMappingContext` are required by the current filter.
Entry uses the original character's `IAV_InteractTwo` through the interaction
provider, independently of the possessed station's contexts.

Two native input-controls components own those contexts. The gameplay controller
refreshes both on device changes; one stock hint widget renders the complete
provided-action list. Native input lifetime and camera processing follow [Voyage
input integration](../../docs/voyage-input-integration-knowledge.md).

The station's holster uses `HolsterSlots=[Overlay]` and a mod-owned
hand-equipment item with `DesiredSlot=Overlay` and a nonempty primary-asset
identity. The native holster creates the ADS equipment actor without character
animation or hand sockets. Scope control uses
`HasInterpState`/`SetInterpState(ADS)`; native interpolation supplies the amount
consumed by `CalculateCameraMovementValue`. On possession loss, reset ADS before
removing the overlay. Removal does not prove immediate actor destruction. The
supporting [native equipment
contract](../../docs/voyage-input-integration-knowledge.md#native-equipment-state-for-ads)
owns the slot and attachment details.

Processed look X is added to yaw and Y is subtracted from direct rotator pitch
before clamping in both eye and scope modes. This coordinate conversion does not
replace the game's inversion setting. Controller hardware and exhaustive
remapping remain separate validation boundaries.

## Settings parser

The operator uses its own `/Game/Mods/Railgun/Settings/BP_IniFileParser`,
imported from the shared [flat INI parser
library](../../tools/BlueprintText/Library/IniFileParser/AGENTS.md). There is no
external parser mod or runtime DLL dependency. `ReloadRailgunSettings`
constructs a fresh parser with the operator as Outer, retains it in transient
`RailgunIniParser`, and calls `Load` with `ProjectContentDir() +
"Paks/Railgun.ini"`. BeginPlay and confirmed station entry invoke this function;
there is no file polling.

Each setting is assigned once through `GetDouble`, using its schema fallback and
inclusive bounds. Parsing remains Float64. A missing file/key, invalid number or
out-of-range value uses the fallback without clamping or retaining the previous
snapshot's value. Exact key case, comments and duplicate-key behavior follow the
shared library's grammar. The operator contains no local line or number parser.

`Assets/Railgun.ini` owns distributed values and formatting;
`Settings/Railgun.settings.json` owns runtime bindings, fallbacks and bounds.
When adding a setting, update both files, its operator member, shared-parser
getter and consuming behavior. The loader integration test derives numeric
cases from the schema; extend its binding map or coverage only when the new
consumer contract needs it. The
getter defaults and bounds must agree with the schema. Fixed presentation values
that are not user options belong with their audio or HUD owner, not in the INI
or schema. [Build and test](BUILD.md#build-and-test) owns schema checks,
saved-loader tests and installation; the [shared settings
recipe](../../docs/runtime-configuration-patterns-knowledge.md) owns file
grammar and player-value preservation.

## Runtime contracts

### Station, model and possession

- The built shell is a `VoyageModuleActor` with tagged model components,
  placement collision, dynamic collision and one electric socket. The socket
  explicitly serializes `bAutoInitialize=false` and uses manual port
  configuration, retaining native `OwningModule=None` and `bStandalone=false`
  defaults.
- The actor is `/Game/Mods/Railgun/Module/BP_Module_Railgun`. The shell owns an
  idempotent `InitializeRailgunStation` function and a transient direct station
  reference. `ReceiveBeginPlay` and the exact inherited persistent post-load
  event each schedule one next-tick initialization attempt. Initialization
  requires a valid model root tagged `Railgun.Model.Root` and the owned entry
  component, but deliberately does not require the shell to have an external
  parent. The created station still attaches to the shell's tagged root, and its
  query box must retain the station interaction component as its parent. The
  node selected by `model-source.json`'s `nodes.root` is the shell's scene root
  and station anchor; no additional mount collider is needed. Physical collision
  remains on the configured `fabricatorCollision` mesh, and the
  dynamic-collision component retains auto-weld. An enclosing mount collider
  interferes with the attached power cable. Initialization uses no polling,
  retry or global discovery fallback. Because native fabrication helpers could
  still create a transient shell, preview open/cancel behavior remains an
  explicit runtime-test boundary along with a fresh build and save/load.
- Interaction acquisition requires a query component that explicitly blocks the
  `Interact` channel and an exact `InteractiveInterface` implementation. The
  action provider and input action are separate contracts. Native pointer/query
  acquisition owns the spatial reach. The provider and entry callback share the
  live station, character and walking-state eligibility; the callback repeats
  those mutable checks before entry and does not add a separate actor-root
  distance threshold in a different coordinate space.
- Operator control uses a dedicated child of the common Voyage vehicle pawn.
  Entry, exit, camera, input context, HUD selection and action hints are owned
  by that station; the physical weapon remains stationary.
- Station entry and exit follow the exact inherited Pawn possession events. Each
  transition schedules one next-tick continuation so Voyage's native possession
  and camera work completes first. Repeated events at the same callsite coalesce
  behind pending-state guards; there is no lifecycle timer or polling fallback.
  Actual entry reloads `Railgun.ini`, refreshes the cached energy state,
  restores canonical wide-mode camera/FOV/input state, notifies the bound HUD
  after that reset, and acquires the dedicated camera. Actual exit restores the
  previous view only when the station still owns the current view target.
- The GLB is imported by Unreal Interchange. Full hierarchy matrices determine
  ownership and transforms; node names identify roles but do not imply
  parentage.
- Yaw, pitch, muzzle, sight, entry, power-socket and charge-indicator roles are
  declared in `Assets/Model/model-source.json`, together with explicit collision
  and interaction boxes. Bind those roles rather than incidental Blender object
  order or decorative node names. Model revisions may change topology, materials
  and local offsets when those roles remain valid; the corresponding maintained
  project assets must be updated before building.
- Wide view is character-eye view. Scope view uses the barrel sight, optical
  mask, reticle and reduced sensitivity. The weapon aligns toward the character
  view target before scoped aiming.
- The station resolves the character's `FirstPersonCamera` once on each
  confirmed entry and resolves the model yaw/pitch roles once when the shell is
  bound. Tick, input, recoil and parallax convergence use transient owned
  references with validity and owner-identity guards; they never rediscover
  those components. Arbitrary replacement of camera or model components between
  the corresponding entry/shell lifecycle events is unsupported and does not
  trigger a polling fallback.
- The station binds the concrete shell owner's `OnEndPlay` delegate. Shell
  destruction or removal from the world immediately blocks acquisition and
  collision. An unoccupied station finalizes teardown directly; an occupied
  station requests Voyage's native exit and lets the resulting
  `ReceiveUnpossessed` continuation finalize it. Finalization is idempotent and
  refuses to destroy a player-controlled pawn or any pawn that still has a
  controller. World transition, editor shutdown and process quit rely on normal
  world teardown. Station EndPlay removes the shell delegate binding.

### Energy and supply

- Railgun settings and HUD use the game's displayed `KWh` scale. One shot
  requires `RailgunFullChargeEnergyKWh * 1000` native units. Module demand is in W;
  the [shared unit
  contract](../../docs/gameplay-observations-knowledge.md#electricity-units)
  explains the game's display convention. `IdleConsumptionKW` is converted to W
  and requested from the network whether the module is full or charging. A
  separate elapsed-time timer continuously debits that amount from the stored
  resource, independent of socket and power state. Native network input
  replenishes the same store: full supply offsets idle debit, limited supply
  leaves the net deficit, and missing supply pays the full idle cost. Charging
  requests the same idle amount in addition to the separately derived net charge
  power, so changing idle consumption does not change `FullChargeTimeSeconds`
  while the requested input is available. Native ON and standby consumption
  remain zero so this explicit debit is the only idle-loss path. Zero and
  fractional idle rates are valid; fallback and range values are owned by the
  settings schema.
- Station initialization resolves and caches the concrete module, binds the
  exact value, socket-connection and power-state delegates, and performs initial
  charge and supply snapshots. The value callback refreshes cached charge and
  switches `SetCustomConsumption` only when the charging versus idle mode
  changes. A socket-change notification can run before `HasSocketConnection`
  exposes the post-transition value. The guarded callback therefore records no
  authoritative supply state and coalesces one next-tick reconciliation for the
  same bound module; that reconciliation reads both supply getters after
  checking lifecycle, generation and object identity. The power callback
  consumes its exact `bHasPower` payload immediately. Re-entry settings reload
  forces one demand refresh and runtime-activity reconciliation. The desired
  demand mode is cached before the native setter; synchronous nested
  notifications are coalesced into one final non-forced read, which does not
  repeat the setter when the mode is unchanged. Rebinding first settles and
  stops the old module's idle-debit timer, invalidates pending supply
  reconciliation, removes old delegates and clears cached module, charge and
  supply state. A failed replacement cannot retain stale work. EndPlay performs
  the same timer and binding teardown.
- The station actor starts with Tick disabled and enables it only while the
  local station view is owned. Active optics/range work may remain per-frame
  while aiming. Idle debit is separate: positive cached charge and a positive
  idle rate start one looping active-only timer at one-third-second intervals.
  The timer is independent of cached supply state and never polls
  `HasSocketConnection` or `HasPower`; it integrates elapsed game time at the
  rate captured when the timer starts, advances its timestamp before native
  removal, and caps each debit by the live stored amount. Re-entrant value
  notifications therefore cannot debit the same interval. Rate change, rebind
  and teardown settle the final partial interval against the old module and old
  rate, then clear the handle. Empty and zero-rate stations own no debit timer;
  a native value callback restarts it after replenishment makes an empty store
  positive. Tick does not discover lifecycle or supply state, maintain demand or
  drain energy. Capacity, units, the fresh pre-shot balance check and exact shot
  debit remain independent of the idle-debit path. The extra one-unit service
  capacity absorbs the default one-third-second idle debit near full; unusually
  high configured idle rates may exceed that buffer and cause charging-mode
  transitions at the timer cadence.

### HUD and model indicators

- The HUD WidgetTree is `InvalidationBox -> Canvas ->` the exact stock
  `BP_DynamicPlayerInputHorizontalWidget_C` instance. The instance owns only the
  Railgun context, action-type filter and Canvas slot; it does not copy the
  stock widget's internal tree or create the outer hint widget at runtime. The
  stock class continues to own its internal action-row lifecycle. The project
  uses an empty editor-only Blueprint with the exact stock package/class
  identity so the owned HUD can serialize that external class reference. The
  stand-in is excluded from cooking and release packaging, allowing the game to
  resolve its unchanged stock implementation. Reusable action/label and
  stock-widget embedding contracts live in [the shared HUD
  patterns](../../docs/vehicle-and-hud-modding-patterns-knowledge.md#evidence-levels-for-actions-and-hints).
- The HUD resolves its exact owning station once from the owning player pawn in
  widget construction. It first identity-safely unregisters any prior station,
  stores reciprocal transient widget/station references, then takes initial
  charge, ammunition, style and view-mode snapshots. Missing or wrong ownership
  stops that binding path without retry, timer or actor discovery. Widget
  destruction clears the station reference only when it still points to that
  exact widget; station teardown also clears its HUD reference.
- Charge refresh follows the station's existing energy event path. The module's
  event-maintained ammunition synchronization notifies the station only after
  its cached count changes. Completed settings reads and zoom changes refresh
  style and mode directly. Notifications that precede widget construction are
  covered by the construction snapshot. Target name/range retain their per-frame
  optical trace. The hit component resolves through Voyage's shared destructible
  provider and `VoyageItemInterface.GetItemName`; the localized `FText` is
  published only when it changes. Unsupported hits use an empty name without
  exposing internal UObject identities, while misses and lifecycle reset publish
  the empty-name/`---` state. The HUD takes an unconditional initial range
  snapshot on construction and later calls `SetText` for those two widgets only
  when their `FText` changes. Widget Tick keeps only range presentation through
  the cached station reference. Status classification is event-driven from
  cached charge, socket and power fields and never calls the module resource or
  supply getters; the charging icon blinks through an authored looping UMG
  animation. Its animation identity/timing follows the [cooked UMG
  contract](../../docs/vehicle-and-hud-modding-patterns-knowledge.md#cooked-widget-animations).
  Fixed layout, fonts, colors and opacity are authored in the saved HUD and its
  `RefreshRailgunHudStyle` graph. The station retains the required font-object
  references. These style values are not runtime configuration options.
- The model-authored charge-indicator render mesh keeps its GLB transform,
  ancestry, dimensions and UVs. The shell disables collision, overlap and shadow
  and tags it for runtime binding. That tagged component loads the stock
  `MI_PogressBar_Basic_LED` material at runtime. Each station instance caches
  it, an independent MID and its last written level. Station initialization
  binds idempotently to the concrete module's exact `OnModuleValueChanged`
  delegate: it removes a stale or duplicate binding, adds one callback and
  performs one initial snapshot. The visual-only callback reads Electricity
  directly from the reported module and updates `ProgressLevel` only when the
  clamped ratio to the configured shot energy changes. EndPlay removes the binding
  and clears the visual cache. The successful-shot path does not write the model
  indicator; the indicator has no polling or timer fallback. This path does not
  use `MaxResourceAmount`, because capacity includes the extra service unit.
  Display-specific save/load and multiplayer remain separate validation
  boundaries.

### Ammunition and charge presentation

The wide-view HUD displays six persistent cartridge icons above the existing
charge text. Construction performs one guarded initial synchronization; later
module inventory events update the event-maintained cached count and notify the
bound station/HUD directly. The HUD does not enumerate or bind the inventory.
The rightmost N icons use opaque white and the remaining positions use the
radial's faint bar color, matching the physical magazine while empty slots
remain visible. At zero rounds, all six positions use a subtle red tint instead
of the ordinary faint color. Ammo colors do not inherit the dynamic charge-ring
color. The icon row and charge text form one centered block, and the existing
optics gate hides that whole block together with the charge radial. The charge
text and filled part of the radial ring use the same subtle red tint when the
energy module is valid and stored energy is below one shot. Shot-ready and
invalid-module states use opaque white; the radial background color is never
changed by this warning. Continuous idle debit can move a previously shot-ready
gun into the red incomplete-charge state whenever native network input does not
fully replenish the configured loss.

### Audio and recoil

- Shot audio is cooked as an owned `SoundWave` whose external class reference is
  the stock `/Game/Audio/Shares/SoundClasses/SC_SFX.SC_SFX`; the editor-only
  stand-in used to serialize that identity is excluded from cook and release.
  `SC_SFX` is parented to `SC_Master`, so native Master and SFX category volume
  processing remains intact. `ShotVolumePercent` scales shot volume; zero mutes
  it. Matching perceived loudness across stock submix processing and restoration
  of saved audio settings on startup are separate checks.
- A successful shot has two independent recoil paths. `CameraRecoilStrength`
  changes real station aim once per shot in a uniformly random yaw/pitch-plane
  direction, with a two-degree baseline and no return animation. Wide view
  updates eye aim before converging the weapon; scope view updates weapon aim
  directly. Existing yaw/pitch limits clamp the result, and neither path changes
  character or control rotation. Ship recoil walks at most 32 scene-component
  attachment parents from the real module root and applies one backward impulse
  to the first simulated primitive it finds. Both settings are bounded
  multipliers; zero disables the corresponding effect. Their defaults and bounds
  are owned by the settings schema.

## Primary assets and research

- The shell's `VoyageModuleComponent.ItemAsset` points to the mod-owned
  `/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01`. Its complete
  state is maintained in the project; no stock item package is overridden.
- A tier-unlocked `Railgun` skill owns exactly two item references: the independent
  gun and the authored ammunition. The skill identity is
  `/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun`; its dedicated research
  icon is under `/Game/Mods/Railgun`. Source paths for all three DataAssets are
  listed in [data-assets-contract.json](Build/data-assets-contract.json).
- The PAK carries an owned-entry registry with exactly the gun, ammunition and
  skill records at `Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`. A loose
  content-only descriptor enables that plugin registry. The records are built
  through the [build pipeline](BUILD.md#build-and-test).
  `Voyage/AssetRegistry.bin` is neither read nor packaged or replaced.
- The observed native Asset Manager settings scan `Skill` below
  `/Game/Data/Assets/Skill` and `Item` below `/Game/Data/Assets`. Registry
  membership alone is insufficient for discovery outside those roots.
- Research unlocks both recipes; neither recipe is available before research.

## Authored ammunition contract

`/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod` is the stable identity of the
owned Railgun Kinetic Rounds item. Its complete state lives in the maintained
`VoyageItemAmmo` DataAsset listed in
[data-assets-contract.json](Build/data-assets-contract.json). Presentation,
recipe and balance values are edited in that asset.

`CraftAmount=1` and `MaxDropCount=1` make each fabricated pickup represent one
round. The primary icon is mod-owned. The drop RenderAsset and
`Fabricator/AmmoCassette/BP_RailgunAmmoCassette` reference the shared model: the
inherited native `MeshComponent` renders `nodes.ammoPickupCarrier`, while direct
SCS children reuse the other visible meshes, materials and relative transforms
below `nodes.ammoPickupRoot`. All six gun cassette slots reuse those imported
assets. The native carrier owns one simple collision box enclosing the selected
render meshes in carrier space.

The ammo is an inventory/fabrication item; the operator and projectile Blueprint
own firing and damage. It needs no complete stock Sniper Rod weapon graph and
does not override a stock Rod package. Its reduced fields are not a contract for
firing this item through a stock sniper rifle. Preserve native carrier and owned
identities, usable geometry/collision and nonempty render subtrees; topology,
decorative names and material/texture counts may change.

Game consumer behavior belongs to [item fabrication and
pickup](../../docs/voyage-item-fabrication-and-pickup-knowledge.md).
Native-read, cook and converted-package comparison belong to [DataAsset
serialization](BUILD.md#primary-dataasset-serialization).

## Magazine inventory

The persistent gun module owns `RailgunAmmoInventory`, a native
`VoyageInventoryWeightLimitedComponent`. Its actor validator accepts only a
valid reference to the owned ammo item; the validator's inventory argument may
be the player's source inventory and is not compared to the module inventory.
The mass budget must remain six times the maintained ammo item's weight. An
ammo-weight change requires a matching update to the module's inventory
template; the build validates their agreement instead of regenerating that
Blueprint. The existing `ReceiveBeginPlay` chain calls native
`SetMaxWeightLimit` to initialize effective capacity; serialized defaults alone
were insufficient.

Manual reload uses the character captured as `OriginalPlayerPawn` on station
entry. After possession is confirmed, the operator casts that pawn to the exact
`VoyageBaseCharacter` owner and caches its `WeightInventory`; it never uses the
newly possessed player pawn or performs world discovery. The gun-side target is
the concrete module's native `GetInternalInventory()` result, and the accepted
item is the shell's exact `AcceptedRailgunAmmo` reference. Both inventory
references are bound to their native `OnInventoryChanged` delegate;
unpossession, EndPlay, and re-entry remove old bindings before clearing or
replacing the cached references.

One shared event-maintained predicate counts the exact accepted item across all
source and target slots. It enables reload only during valid local station
control, with a positive source count and fewer than six rounds in the target.
The standard action provider reads the cached result for its `bEnabled` field,
while the mapped Reload action repeats the live predicate and asks the shell's
exact `VoyageInventoryItemValidatorInterface.ValidateItem` implementation to
accept the configured ammunition. It snapshots the source slots for that item,
then revalidates each live slot before calling native
`target.TransferSlot(source, sourceSlot, -1, requestedAmount)`. The request is
positive and capped by the function-local remaining six-round budget; the native
return value must be positive and no larger than the request before the loop
continues. `-1` retains stock target-slot autoplacement, and the native
transaction owns capacity checks, weight checks, mutation, and inventory-change
notifications. Synchronous inventory callbacks may refresh the cached display
predicate, but cannot overwrite the transaction's local slot snapshot, remaining
amount, current slot, request, or moved count. Enhanced Input `Started` is the
sole execution path; the action-hint delegate does not also perform the
transfer. Reload is independent of energy, scope mode, firing, and the existing
event-driven ammunition visuals, and has no tick or polling fallback.

The model binds six distinct ordered cassette roots explicitly in
`model-source.json`; each must have a nonempty render subtree. Their render
descendants are hidden by default; internal mesh composition may change while
those roles remain valid. Each gun instance binds a parameterless callback to
its owned inventory's native `OnInventoryChanged` multicast delegate. The
callback sums `ItemCount` only for the exact Railgun ammunition asset, clamps
presentation to six, and updates the six cassette subtrees only when that count
changes. `ReceiveBeginPlay` performs an initial forced synchronization after
binding. The exact inherited persistent post-load event performs another forced
synchronization on the next tick, without duplicating the binding. This
presentation path does not mutate inventory state, hide neighboring ammo-bin
geometry, or depend on an operator entering the gun.

The model's `AmmoMagazine_6Slot` anchor carries the reference, interaction
provider and attached query box specified by `model-source.json`. Part 100 opens
the stock container overlay. The exact inherited
`InteractiveInterface.InteractGetInventory` override returns
`ModuleComponent.GetInternalInventory()`. Socket and operator interactions
remain separate. General evidence and restrictions live in the shared [filtered
inventory
contract](../../docs/voyage-item-fabrication-and-pickup-knowledge.md#filtered-module-inventories-refinery-reference).

### Fire transaction

The fire path treats the owned native inventory as authority; neither the
six-slot model state nor the HUD count cache can authorize a shot. It resolves
the exact accepted Railgun ammunition, captures a live occupied slot, verifies
that slot's exact item and positive count, and creates a deferred inactive shot
before spending resources. The input request is claimed before native calls that
may dispatch delegates. Native exact energy removal runs first, followed without
a latent gap by native `RemoveItem` for one round with notifications enabled.
Its return value is not a control-flow gate: after the validated live precheck
and exact energy debit, the graph finishes the projectile and effects without
refund, retry, queue or persistent fault state. Empty inventory or insufficient
charge fails the precheck and produces no projectile, sound, damage, ammo debit
or energy debit; automatic charging remains independent of magazine state. The
shot removes the configured required energy amount; it does not unconditionally
zero a store that also includes service capacity.

## Shot visual effects

`BP_RailgunTestShot` initializes collision at runtime as `WorldDynamic` with
every channel blocked; omitted editor defaults cannot be assumed to match native
runtime construction. The projectile's blocking-hit path registers the direct
attack. Visual effects do not own projectile motion or hit authority.

The stock `/Game/VFX/Environment/Interactive/NS_Explosion_SeaMine` system can be
split per spawned Niagara component without copying or modifying the stock
asset. Calling `UFXSystemComponent.SetEmitterEnable` before activation uses two
explicit masks from the current 14-emitter stock system. Fountain mode enables
`Drop_Tower`, `Tower`, `Spikes` and `Core_Drops`; impact mode enables
`FireExplode`, `spark`, `spark_l`, `shockwave`, `Spark001`, `Core_Smoke001`,
`SmallerPieces` and `BigPieces`. `BigPieceSplash` and `SmallPieceSplash` remain
disabled in both modes. A water miss uses only the fountain effect; a damaging
Talon-shark impact uses fire and smoke.

Production effects use a mod-owned transient Blueprint helper with an owned
inactive Niagara component. Each instance applies one of these masks, activates
independently and has a ten-second maximum actor lifetime. This keeps an effect
alive after its projectile stops while preventing one hit from destroying or
reconfiguring another hit's effect. The helper stores only a runtime reference
to the stock system; no stock VFX package is copied, patched or registered by
Railgun.

The editor-only native mirror for water lookup is
`/Script/Voyage.VoyageMiscBlueprintFunctionLibrary`. The consumed signatures are
`GetActorWorld(AActor*) -> UWorld*` and `GetWaterHeightAtLocation(UWorld*,
FVector) -> float`. Earlier native behavior inspection found the `-100000 cm`
sentinel and water-body bounds/query filtering; the signatures alone do not
establish that the current implementation has identical internals. A valid
gameplay world, bounded segment sampling and a verified near-surface residual
are therefore required; a single endpoint value, zero height, or the function
alone is not a universal exact-surface contract. This API and its limits are
fingerprint-bound.

The projectile applies the explosion on a real blocking hit and checks only
finite travelled projectile segments for the first accepted water-surface
crossing. Exhaustive collision ordering, submerged-target, save/load,
multiplayer and performance guarantees are outside the established coverage.

The above-water wake path uses the fingerprint-bound
`VoyageWeatherSubsystem.AddFluidImpulse(FVoyageFluidImpulse)` contract directly;
the subsystem derives from `TickableWorldSubsystem`, and the impulse contains
location, two-dimensional direction, radius and strength. One transient
controller per shot samples only already travelled segments over the first 500
metres, with two-metre spacing and cross-frame remainder carry. Its bounded
arrays hold at most 256 valid queried surface positions. Water lookup is made
eight metres below each trajectory point to satisfy the confirmed native
height-query bounds gate; only finite surfaces zero to eight metres below the
original trajectory are accepted.

Each accepted point submits a radius-two-metre impulse on every controller tick
for at most 0.25 seconds. Strength is held constant over that interval and is
bounded by `0.2`, with linear height falloff to zero at eight metres; it is not
scaled by delta time, velocity or projectile mass. The controller stops
accepting at the preview limit or when the shot finishes, destroys itself after
the last short tail, and has a one-second hard lifetime. It is independent of
the first-crossing fountain and direct-hit attack paths. It is not an exact
reconstruction of the stock attack consumer. Exhaustive height/distance,
obstacle-clipping, frame-rate and performance guarantees are outside the
established coverage.

`AddFluidImpulse` enqueues an impulse; it does not acknowledge visible output.
Stock FluidNinja queue consumption and simulation-area placement are not owned
by this mod. Queue-consumer behavior remains unestablished; the configured query
bounds do not guarantee a visible wake at every world position.

## Compatibility limits

The established runtime scope is single-player, including magazine save/reload.
Controller hardware, exhaustive remapping, multiplayer, cable/operator-entry
edge cases, all target-specific damage/effect combinations and all partial or
split-stack transfers are outside that scope. Native mirrors are partial editor
contracts, not a complete game API or shipping ABI.

The fire transaction does not branch on an unexpected native inventory-removal
rejection after its live precheck and energy debit; that failure case remains
outside the established runtime coverage and does not lock later shots.
Water/effect-specific limits are stated with their owners above.

For game updates, follow the public candidate and revalidation path in
[BUILD.md](BUILD.md#build-and-test) and the [shared release
rules](../../docs/release-and-installation-rule.md#game-update-and-candidate-builds).
An earlier native observation or a successful editor check does not establish
new runtime compatibility.
