# Railgun architecture

## Stock aiming and input integration boundaries

The following native contracts were inspected on an earlier game snapshot;
that observation is not a player-facing compatibility gate. The current
accepted source-build pin is in `game-build.json`.
Railgun uses these contracts for stock actions, mapping-context lifecycle and
ADS processing. Controller hardware behavior remains unverified. On a changed
Steam build, try the public candidate producer first; investigate these native
contracts if the build or game test fails. Reusable input integration rules
live in [Voyage input integration](../../docs/voyage-input-integration-knowledge.md).

Each shell clones the live character and vehicle context assets and their
device mappings. The character allowlist retains Primary Action, ADS, Reload
and both look axes; the vehicle allowlist retains Exit Vehicle. The current
filter requires empty mapping-profile overrides and no alternate
`SwitchGamepadMappingContext`; an unsupported shape rejects station preparation.
Entry uses the original character's `IAV_InteractTwo` through the interaction
provider, independently of the possessed station's contexts.

- `VoyageMiscBlueprintFunctionLibrary::CalculateCameraMovementValue` is a
  callable, non-pure function taking a Voyage player controller and a Vector2D.
  It applies stock camera settings. Its ADS input comes from the controller's
  current Pawn, that Pawn's `VoyageWeaponHolsterComponent`, and
  `GetActiveSlotActor(false)->GetInterpStateAmount(ADS)`. A station scope boolean
  alone does not supply this state. The helper does not expose an ADS-amount
  argument; character aiming also applies additional character multipliers.
- Holster initialization derives `OverlaySlotIndex` from the first entry in
  `HolsterSlots` with `EVoyageHolsterSlotType::Overlay`. Without such an entry,
  the index remains invalid and the overlay-slot application path returns.
  Treat the index as derived state, not a field to fabricate manually.
- The stock first-person character has five `HolsterSlots` (the fifth is
  Overlay) but only four ordinary `Slots` entries. Overlay does not require a
  matching `BarSlotConfig`. The native overlay path can insert an absent
  inventory slot record. Slot acceptance checks the holster configuration and,
  for hand equipment, requires `DesiredSlot` to match its holster slot type.
  A station-only overlay therefore uses `HolsterSlots=[Overlay]` and equipment
  `DesiredSlot=Overlay`; it must not fabricate internal slot indices. Inventory
  insertion also requires a nonempty item primary-asset identity.
- `AddOverlayState` accepts a key, `VoyageHolsterSlotOverride` and a force flag.
  The override holds a weak `VoyageItemBaseEquipment` reference and priority.
  This is not an API for inserting an arbitrary spawned actor. The inspected
  slot-actor creation path handles `VoyageItemHandEquipment` and
  `VoyageItemConsumable`, resolving their configured equipment actor class.
  A bare base-equipment definition is therefore insufficient for that path.
  `RemoveOverlayState(const FName& InKey)` is BlueprintCallable and returns
  bool; an absent key returns false. Reset the station equipment's ADS before
  removing its overlay on possession loss, including forced exit. Removal is
  not evidence of immediate actor destruction or completed slot transition.
- `VoyageInputControlsComponent` sets up input on its owner's pawn-restart
  notification and tears it down for the old controller on controller changes.
  Context refresh removes all mappings referenced by its `InputContextAsset`
  before adding the selected device context. Context lifetime must have a
  single owner; removing a shared stock context can affect another consumer.
- The station uses one stock action-hint widget for its complete provided-action
  list. Separate character and exit input contexts do not require separate
  renderers: two widgets render duplicate complete lists. Input-component
  ownership and hint rendering remain separate.
- The base component's device-change callback only stores the device type,
  but the gameplay `VoyagePlayerController` additionally enumerates all
  `VoyageInputControlsComponent` instances on its current Pawn and invokes
  each component's native context refresh with the new device type. The stock
  `/Game/Game/BP_VoyagePlayerController` inherits this controller. Mapping
  refresh therefore belongs to the gameplay controller, not the component's
  notification callback. Multiple such components participate in this path;
  no station-specific device polling or synthetic pawn restart is needed
  to reproduce this stock mapping-switch mechanism.
  `OnPawnRestarted` is reflected but is not BlueprintCallable in this build;
  an editor mirror must not invent callability to use it as a refresh API.
- `DrawFromHolster` checks the owner's animation instance and the equipment's
  draw animation. If either is absent, or playing the animation returns zero,
  it takes a native fallback branch. Its equipment-interface call sets
  `bEquipped` before the animation attempt. Absence of a character animation
  is not itself an unconditional rejection of drawing equipment.
- `AttachEquipmentToHand` first considers the equipment's skeleton socket,
  then the holster's `SkeletonSocketTag`, then `EquipmentComponentTag`. When
  all are empty, it attaches to the owner's root component and applies the
  equipment attachment transform. This branch does not require a hand socket
  or skeletal mesh.
- `VoyageEquipmentActor::SetInterpState(InType, bIsSet)` is BlueprintCallable
  and returns void. `HasInterpState(InType)` is a const BlueprintPure boolean
  query of the live native state, separate from `GetInterpStateAmount(InType)`.
  Toggle commands and scope presentation can use that state instead of an
  independent station toggle. The equipment's native per-frame path updates interpolation
  state amounts using its configuration and delta time. The draw-interface
  receiver is an adjusted interface pointer; field offsets must be interpreted
  relative to the actor before assigning property identities. A station
  integration must use the native ADS state and updater rather than duplicating
  their transition formula or treating a setter call as completion evidence.

The station owns the overlay equipment actor without character animation or
hand sockets. Both input components participate in native device switching;
one HUD widget renders the complete provided-action list. Look values pass
through the native camera helper, then X is added to yaw and Y is subtracted
from direct rotator pitch before clamping, in both eye and scope modes.
This coordinate conversion does not replace the game's inversion setting.

## Source organization

- `RailgunRuntimeGenerator` keeps graph-building declarations and stable names
  in self-contained headers, with ordinary implementations in matching `.cpp`
  files. `FGraph` is the shared graph-construction interface; its template node
  factory remains in the header while non-template operations are out of line.
- `RailgunModelGenerator` imports the gun and ammunition visuals once, builds
  the shell and owned composite pickup from that shared scene, and keeps common
  editor-only helpers in its private support translation unit.
- `Assets/Railgun_Shot_Blast.wav` is the authored shot sound.
- Generator translation units include their own header first and build with
  unity disabled. Voyage mirror headers retain only trivial reflected native
  stubs inline; the nontrivial vehicle mirror constructor is out of line.

## Runtime contracts

- The weapon uses the game's module/fabricator path. Its actor, UI and runtime
  packages remain under `/Game/Mods/Railgun/...`; primary Item and Skill data
  assets use mod-unique names inside the current game's confirmed AssetManager
  scan roots.
- The built shell is a `VoyageModuleActor` with tagged model components,
  placement collision, dynamic collision and one electric socket. On the
  current fingerprint the socket explicitly serializes `bAutoInitialize=false`
  while retaining native `OwningModule=None` and `bStandalone=false` defaults;
  the removed module-level custom-target fields are neither mirrored nor
  serialized. Manual socket ownership and powered charging are confirmed in the
  real game for the current fingerprint.
- Interaction acquisition requires a query component that explicitly blocks the
  `Interact` channel and an exact `InteractiveInterface` implementation. The
  action provider and input action are separate contracts. Native pointer/query
  acquisition owns the spatial reach. The provider and entry callback share the
  live station, character and walking-state eligibility; the callback repeats
  those mutable checks before entry and does not add a separate actor-root
  distance threshold in a different coordinate space.
- Operator control uses a dedicated child of the common Voyage vehicle pawn.
  Entry, exit, camera, input context, HUD selection and action hints are owned by
  that station; the physical weapon remains stationary.
- The HUD WidgetTree is `InvalidationBox -> Canvas ->` the exact stock
  `BP_DynamicPlayerInputHorizontalWidget_C` instance. The instance owns only
  the Railgun context, action-type filter and Canvas slot; it does not copy the
  stock widget's internal tree or create the outer hint widget at runtime. The
  stock class continues to own its internal action-row lifecycle. Generation
  uses an empty editor-only Blueprint with the exact stock package/class
  identity so the owned HUD can serialize that external class reference. The
  stand-in is excluded from cooking and release packaging, allowing the game to
  resolve its unchanged stock implementation.
  Reusable action/label contracts, evidence levels and the limits of attributing
  the layout fix to a single change live in
  [the shared HUD patterns](../../docs/vehicle-and-hud-modding-patterns-knowledge.md#evidence-levels-for-actions-and-hints).
- Station entry and exit follow the exact inherited Pawn possession events.
  Each transition schedules one next-tick continuation so Voyage's native
  possession and camera work completes first. Repeated events at the same
  callsite coalesce behind pending-state guards; there is no lifecycle timer or
  polling fallback. Actual entry reloads `Railgun.ini`, refreshes the cached
  energy state, restores canonical wide-mode camera/FOV/input state, notifies
  the bound HUD after that reset, and acquires the dedicated camera. Actual
  exit restores the previous view only when the station still owns the current
  view target.
- Installation preserves existing user settings. The packaged
  `Assets/Railgun.ini` owns current defaults;
  `Settings/Railgun.settings.json` owns bindings and ranges. At runtime a
  missing key uses its template default, and the next installation adds it to
  the installed INI.
- The GLB is imported by Unreal Interchange. Full hierarchy matrices determine
  ownership and transforms; node names identify roles but do not imply parentage.
- Yaw, pitch, muzzle, sight, entry, power-socket and charge-indicator roles are
  declared in the model manifest. Model revisions may change topology,
  materials and local offsets without changing generator code when those roles
  remain valid.
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
- The weapon charges from the module electricity system, may fire only when its
  configured charge is full, resets charge after a shot, and applies a validated
  direct attack to the hit target. Projectile travel is represented visually;
  hit resolution is authoritative.
- The projectile initializes its collision component at runtime as
  `WorldDynamic` with every channel blocked. Cooked serialization may omit an
  editor default that native runtime construction supplies differently.
- Railgun settings and HUD use the game's displayed `KWh` scale. Voyage maps
  one displayed `KWh` to 1000 native electricity amount units; module demand
  remains expressed in W. `IdleConsumptionKW` is converted to W and requested
  from the network whether the module is full or charging. A separate
  elapsed-time timer continuously debits that amount from the stored resource,
  independent of socket and power state. Native network input replenishes the
  same store: full supply offsets idle debit, limited supply leaves the net
  deficit, and missing supply pays the full idle cost. Charging requests the
  same idle amount in addition to the separately derived net charge power, so
  changing idle consumption does not change `FullChargeTimeSeconds` while the
  requested input is available. Native ON and standby consumption remain zero
  so this explicit debit is the only idle-loss path. Zero and fractional values
  are valid; the shared numeric parser supplies the `1` kW schema fallback for
  invalid or out-of-range input without clamping.
- Station initialization resolves and caches the concrete module, binds the
  exact value, socket-connection and power-state delegates, and performs
  initial charge and supply snapshots. The value callback refreshes cached
  charge and switches `SetCustomConsumption` only when the charging versus idle
  mode changes. The electric socket explicitly disables native automatic
  initialization and stays on the manual port configuration path, which is
  confirmed in the real game at the configured charging rate. On the currently
  supported Voyage fingerprint, an accepted
  socket-change notification can run before `HasSocketConnection` exposes the
  post-transition value. The guarded callback therefore records no
  authoritative supply state and coalesces one next-tick reconciliation for
  the same bound module; that reconciliation reads both supply getters after
  checking lifecycle, generation and object identity. The power callback
  consumes its exact `bHasPower` payload immediately.
  Re-entry settings reload forces one demand refresh and runtime-activity
  reconciliation. The desired demand mode is cached before the native setter;
  synchronous nested notifications are coalesced into one final non-forced
  read, which does not repeat the setter when the mode is unchanged. Rebinding
  first settles and stops the old module's idle-debit timer, invalidates pending
  supply reconciliation, removes old delegates and clears cached module,
  charge and supply state. A failed replacement cannot retain stale work.
  EndPlay performs the same timer and binding teardown.
- The station actor starts with Tick disabled and enables it only while the
  local station view is owned. Active optics/range work may remain per-frame
  while aiming. Idle debit is separate: positive cached charge and a positive
  idle rate start one looping active-only timer at one-third-second intervals.
  The timer is independent of cached supply state and never polls
  `HasSocketConnection` or `HasPower`; it integrates elapsed game
  time at the rate captured when the timer starts, advances its timestamp
  before native removal, and caps each debit by the live stored amount.
  Re-entrant value notifications therefore cannot debit the same interval.
  Rate change, rebind and teardown settle the final partial interval
  against the old module and old rate, then clear the handle. Empty and
  zero-rate stations own no debit timer; a native value callback restarts it
  after replenishment makes an empty store positive. Tick does not discover
  lifecycle or supply state, maintains demand, drains energy, or samples unused
  charge-rate state. Capacity, units, the fresh pre-shot balance check and exact
  shot debit remain independent of the idle-debit path. The extra one-unit
  service capacity absorbs the default one-third-second idle debit near full;
  unusually high configured idle rates may exceed that buffer and cause
  charging-mode transitions at the timer cadence.
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
  covered by the construction snapshot. Target name/range retain their
  per-frame optical trace. The hit component resolves through Voyage's shared
  destructible provider and `VoyageItemInterface.GetItemName`; the localized
  `FText` is published only when it changes. Unsupported hits use an empty name
  without exposing internal UObject identities, while misses and lifecycle
  reset publish the empty-name/`---` state.
  The HUD takes an unconditional initial range snapshot on construction and
  later calls `SetText` for those two widgets only when their `FText` changes.
  Widget Tick keeps only range presentation through the cached station
  reference. Status classification is event-driven from cached charge, socket
  and power fields and never calls the module resource or supply getters; the
  charging icon blinks through an authored looping UMG animation. The gameplay
  HUD has no diagnostic status text. The generated animation property and its
  inner `MovieScene` share one logical name because cooked UMG initialization
  resolves that property by the scene name. The animation keeps display rate
  separate from evaluation precision: authored phase times are converted to a
  high-resolution tick scale so ordinary widget frames accumulate playback.
  Fixed status opacity is `0.5` in each image's color alpha; animation render
  opacity remains an independent zero-or-one blink factor. Target-name,
  target-distance and charge-text layout/font contracts are compile-time
  presentation constants; only the required loaded font-object references are
  retained on the generated station. Target name uses offset `(0, 330)`,
  opacity `0.7`, size `20`, ShareTech Mono and Bold; target distance uses
  `(0, 185)`, `0.7`, size `25`, DSEG7 Classic Bold and Bold; charge text uses
  opacity `0.7`, size `14`, Noto Sans Regular and Regular.
- The model-authored charge-indicator render mesh keeps its GLB transform,
  ancestry, dimensions and UVs. The shell disables collision, overlap and shadow
  and tags it for runtime binding. That tagged component loads the stock
  `MI_PogressBar_Basic_LED` material at runtime. Each station instance caches it,
  an independent MID and its last written level. Station initialization binds
  idempotently to the concrete module's exact `OnModuleValueChanged` delegate:
  it removes a stale or duplicate binding, adds one callback and performs one
  initial snapshot. The visual-only callback reads Electricity directly from
  the reported module and updates `ProgressLevel` only when the clamped ratio
  to `RequiredEnergyAmount` changes. EndPlay removes the binding and clears the
  visual cache. The automatic-charge tick and successful-shot path do not write
  the model indicator, and there is no polling or timer fallback. This path
  does not use `MaxResourceAmount`, because capacity includes the extra service
  unit. Structural, source and cooked contracts are statically checked.
  Display-specific save/load and multiplayer behavior remain outside the
  established compatibility coverage.
- Shot audio is cooked as an owned `SoundWave` whose external class reference is
  the stock `/Game/Audio/Shares/SoundClasses/SC_SFX.SC_SFX`; the editor-only
  stand-in used to serialize that identity is excluded from cook and release.
  `SC_SFX` is parented to `SC_Master`, so native Master and SFX category volume
  processing remains intact. `ShotVolumePercent=600` preserves the established
  six-times pre-category baseline, while zero mutes the shot. The numeric
  baseline is unchanged; matching perceived loudness after stock SFX submix
  processing is not promised. The shot-volume setting and live Master/SFX
  response are confirmed in the real game. Restoration of saved audio settings
  during a new game startup was not separately verified.
- A successful shot has two independent recoil paths. The historical
  `CameraRecoilStrength` key now changes real station aim once per shot in a
  uniformly random yaw/pitch-plane direction, with a two-degree baseline and
  no return animation. Wide view updates eye aim before converging the weapon;
  scope view updates weapon aim directly. Existing yaw/pitch limits clamp the
  result, and neither path changes character or control rotation. Ship recoil
  walks at most 32 scene-component attachment parents from the real module root
  and applies one backward impulse to the first simulated primitive it finds.
  Both settings are bounded multipliers from zero through ten. A missing INI,
  missing recoil key or invalid numeric value uses the schema fallback: 0.5
  for camera recoil and 2 for ship recoil. Explicit valid INI values override
  those fallbacks; zero disables the corresponding effect. These fallback
  values still require a real-game check.
- The station binds the concrete shell owner's `OnEndPlay` delegate. Shell
  destruction or removal from the world immediately blocks acquisition and
  collision. An unoccupied station finalizes teardown directly; an occupied
  station requests Voyage's native exit and lets the resulting
  `ReceiveUnpossessed` continuation finalize it. Finalization is idempotent and
  refuses to destroy a player-controlled pawn or any pawn that still has a
  controller. World transition, editor shutdown and process quit rely on normal
  world teardown. Station EndPlay removes the shell delegate binding.

## Primary assets and research

- The actor is `/Game/Mods/Railgun/Module/BP_Module_Railgun`. The shell owns an
  idempotent `InitializeRailgunStation` function and a transient direct station
  reference.
  `ReceiveBeginPlay` and the exact inherited persistent post-load event each
  schedule one next-tick initialization attempt. Initialization requires a
  valid model root tagged `Railgun.Model.Root` and the owned entry component,
  but deliberately does not require the shell to have an external parent. The
  created station still attaches to the shell's tagged root, and its query box
  must retain the station interaction component as its parent. The node selected
  by
  `model-source.json`'s `nodes.root` is the shell's scene root and station
  anchor; no additional mount collider is needed. Physical collision remains
  on the configured `fabricatorCollision` mesh, and the dynamic-collision
  component retains auto-weld. An enclosing mount collider interferes with
  the attached power cable. Initialization uses no polling, retry or global
  discovery fallback.
  Because native fabrication helpers could still create a transient shell,
  preview open/cancel behavior remains an explicit runtime-test boundary along
  with a fresh build and save/load. Its
  `VoyageModuleComponent.ItemAsset` points
  to `/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01`. The complete
  gun item object graph is authored in
  `Assets/Fabricator/railgun-item.json` and written directly to the staged
  package; no stock item package is copied or patched.
- One Tier-19 `Railgun` skill owns exactly two item references: the independent
  gun and the authored ammunition. The skill identity is
  `/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun`; its dedicated research icon
  is under `/Game/Mods/Railgun`. Its complete serialized source is
  `Assets/Skill/railgun-skill.json`, not a generated mutation of a stock skill.
- The PAK carries an owned-entry registry with exactly the gun, ammunition and
  skill records at `Voyage/Mods/RailgunCatalogue/AssetRegistry.bin`. A loose
  content-only descriptor enables that plugin registry. The records are built
  from the owned UAssetAPI JSON readbacks through the shared Voyage registry
  profile and native writer; `Voyage/AssetRegistry.bin` is neither read nor
  packaged or replaced. Railgun owns no registry serializer or class defaults.
- The observed native Asset Manager settings scan `Skill` below
  `/Game/Data/Assets/Skill` and `Item` below `/Game/Data/Assets`. Registry
  membership alone is insufficient for discovery outside those roots.
- Research unlocks both recipes; neither recipe is available before research.
- `EVoyageSkillUnlockMethod::Never` hides a skill and leaves its recipes
  unavailable in a pre-research save while its assets remain registered and
  packaged. It is not an established mechanism for revoking existing unlocks
  or controlling their persistence. Railgun does not use this unlock method.

## Authored ammunition contract

The stable identity remains
`/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod`, despite the historical suffix.
The current item is authored as one `VoyageItemAmmo` export in
`Assets/Fabricator/railgun-ammo-item.json`; it is not a full Sniper Rod clone
or a stock Rod package override. The generator emits only an editor/cook
placeholder at the same identity so the research skill can reference it. The
packaged ammo object graph and values come only from the owned JSON.

The ammo item contains 16 top-level serialized properties:
`Caliber`, `Icon`, `Category`, `CategoryAsset`, `Quality`, `Weight`, `CraftTime`,
`CraftElectricityCost`, `CraftAmount`, `CraftFilter`, `Components`,
`DropVariations`, `DroppedActor`, `MaxDropCount`, `Name`, `Description`.
This is a current value snapshot, not a build schema or whitelist. Native
properties may be added to or removed from the owned JSON when required; the
producer validates serialization, identity and required runtime references.

| Setting | Current owned value |
| --- | --- |
| Name | Railgun Kinetic Rounds |
| Description | Armor-piercing kinetic rounds. No explosives, just mass and velocity. |
| Caliber / native tooltip | `45.0` / `45mm` |
| Quality | Common |
| Weight | `3.9` per round; the six-round magazine limit is `23.4 kg` |
| Craft amount / time | `1` round / `6` seconds |
| Craft electricity cost / filter | `5` native units / `3` |
| Recipe | Iron `1`, Copper `1`, Plastic `1` |
| MaxDropCount | `1`; each physical pickup represents one round |

The primary icon is mod-owned. The ammo JSON owns the stable drop RenderAsset
path and the owned composite actor
`/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette`. Its native
stock `MeshComponent` renders the carrier selected by
`nodes.ammoPickupCarrier`; direct SCS children reuse every other visible mesh,
material and source-relative transform below `nodes.ammoPickupRoot`. The same
imported assets remain in all six gun cassette slots. `WeaponData`, all projectile subexports, bullet/case
fields, stock SFX/VFX, damage-type references, `SecondaryIcon` and `ScalePerItem`
are absent. Imports are limited to the native item class/CDO, ammo category,
recipe materials, owned icon and the shared model dependencies. These
omissions are validated for fabrication and pickup, not for firing this item
through a stock sniper rifle.

The pickup generator transforms all eight local bounding-box corners of every
selected render mesh into carrier space and assigns exactly one simple box to
the shared carrier mesh. It records the selected roles, reused asset identities,
transforms and collision in the model inventory. Validation requires a nonempty
render subtree, finite transforms and bounds, the native physics carrier, and
the current shared identities; it does not freeze topology, decorative names,
counts, bounds or hashes. The stock `BP_DynamicMeshActor` stand-in exists only
to compile the owned child during the editor build and is never packaged.

After cook, the reviewed UAssetGUI/UAssetAPI writer converts the three owned
JSON sources directly to staged `.uasset`/`.uexp` packages using UE 5.8.
`Assets/data-assets-contract.json` owns their individual package/native
identities plus one shared game, mapping, writer and revalidation gate for the
complete set. Every import carries explicit
`PackageName=None`; leaving
that field null is not equivalent under the reviewed filtered-import writer.
The build reopens the result and compares versions, custom versions, name map,
imports and complete export graph against the JSON used for that build before
packaging. The ammo and gun items retain their unversioned formats; the skill
retains its tagged format. All three object graphs are preserved by their
editable JSON rather than hard-coded count gates. See the shared
[serialization contract](../../docs/voyage-cooked-asset-toolchain-knowledge.md#package-serialization-is-a-separate-compatibility-contract).

General consumer findings and limitations, including output grouping and
native caliber defaults, live in
[item fabrication and pickup](../../docs/voyage-item-fabrication-and-pickup-knowledge.md).
The independent scan-to-vault unlock mechanism is documented in
[scanning and recipe unlocks](../../docs/voyage-scanning-and-recipe-unlocks-knowledge.md).

## Magazine inventory

The persistent gun module owns `RailgunAmmoInventory`, a native
`VoyageInventoryWeightLimitedComponent`. Its actor validator accepts only a
valid reference to the owned ammo item; the validator's inventory argument may
be the player's source inventory and is not compared to the module inventory.
The mass budget is six times the item's current JSON-authored weight (currently
23.4 kg). Changing the ammo weight in JSON changes the generated magazine limit
without a C++ balance constant.
The existing `ReceiveBeginPlay` chain calls native `SetMaxWeightLimit` to
initialize effective capacity; serialized defaults alone were insufficient.

Manual reload uses the character captured as `OriginalPlayerPawn` on station
entry. After possession is confirmed, the operator casts that pawn to the
exact `VoyageBaseCharacter` owner and caches its `WeightInventory`; it never
uses the newly possessed player pawn or performs world discovery. The gun-side
target is the concrete module's native `GetInternalInventory()` result, and
the accepted item is the shell's exact `AcceptedRailgunAmmo` reference. Both
inventory references are bound to their native `OnInventoryChanged` delegate;
unpossession, EndPlay, and re-entry remove old bindings before clearing or
replacing the cached references.

One shared event-maintained predicate counts the exact accepted item across all
source and target slots. It enables reload only during valid local station
control, with a positive source count and fewer than six rounds in the target.
The standard action provider reads the cached result for its `bEnabled` field,
while the `R` handler repeats the live predicate and asks the shell's exact
`VoyageInventoryItemValidatorInterface.ValidateItem` implementation to accept
the configured ammunition. It snapshots the source slots for that item, then
revalidates each live slot before calling native
`target.TransferSlot(source, sourceSlot, -1, requestedAmount)`. The request is
positive and capped by the function-local remaining six-round budget; the
native return value must be positive and no larger than the request before the
loop continues. `-1` retains stock target-slot autoplacement, and the native
transaction owns capacity checks, weight checks, mutation, and inventory-change
notifications. Synchronous inventory callbacks may refresh the cached display
predicate, but cannot overwrite the transaction's local slot snapshot,
remaining amount, current slot, request, or moved count. Enhanced Input
`Started` is the sole execution path; the action-hint delegate does not also
perform the transfer.
Reload is independent of energy, scope mode, firing, and the existing
event-driven ammunition visuals, and has no tick or polling fallback.

The model binds six ordered cassette roots explicitly in `model-source.json`;
their render descendants are hidden by default. Each gun instance binds a
parameterless callback to its owned inventory's native `OnInventoryChanged`
multicast delegate. The callback sums `ItemCount` only for the exact Railgun
ammunition asset, clamps presentation to six, and updates the six cassette
subtrees only when that count changes. `ReceiveBeginPlay` performs an initial
forced synchronization after binding. The exact inherited persistent post-load
event performs another forced synchronization on the next tick, without
duplicating the binding. This presentation path does not mutate inventory
state, hide neighboring ammo-bin geometry, or depend on an operator entering
the gun.

The model's `AmmoMagazine_6Slot` anchor carries the reference, interaction
provider and attached query box specified by `model-source.json`. Part 100
opens the stock container overlay. The exact inherited
`InteractiveInterface.InteractGetInventory` override returns
`ModuleComponent.GetInternalInventory()`. Socket and operator interactions
remain separate. Temporary inventory diagnostic widgets and state are absent.
General evidence and restrictions live in the shared
[filtered inventory contract](../../docs/voyage-item-fabrication-and-pickup-knowledge.md#filtered-module-inventories-refinery-reference).

The fire path treats the owned native inventory as authority; neither the
six-slot model state nor the HUD count cache can authorize a shot. It resolves
the exact accepted Railgun ammunition, captures a live occupied slot, verifies
that slot's exact item and positive count, and creates a deferred inactive shot
before spending resources. The input request is claimed before native calls
that may dispatch delegates. Native exact energy removal runs first, followed
without a latent gap by native `RemoveItem` for one round with notifications
enabled. Its return value is not a control-flow gate: after the validated live
precheck and exact energy debit, the graph finishes the projectile and effects
without refund, retry, queue or persistent fault state. Empty inventory or
insufficient charge fails the precheck and produces no projectile, sound,
damage, ammo debit or energy debit; automatic charging remains independent of
magazine state.

The wide-view HUD displays six persistent cartridge icons
above the existing charge text. Construction performs one guarded initial
synchronization; later module inventory events update the event-maintained
cached count and notify the bound station/HUD directly. The HUD does not
enumerate or bind the inventory. The rightmost N icons use opaque white and the
remaining positions use the radial's faint bar color, matching the physical
magazine while empty slots remain visible. At zero rounds, all six positions
use a subtle red tint instead of the ordinary faint color. Ammo colors do not
inherit the dynamic charge-ring color.
The icon row and charge text form one centered block, and the existing optics
gate hides that whole block together with the charge radial. The source image,
crop, six-slot order, colors, cached-count path and absence of HUD inventory
polling follow the package contract. The runtime places the row, updates it
right-to-left, applies the zero-count tint and hides it with the optics gate.

The charge text and filled part of the radial ring use the same subtle red tint
whenever stored energy is below one shot. Shot-ready and invalid states use
opaque white; the radial background color is never changed by this warning.
Continuous idle debit can move a previously shot-ready gun into the red
incomplete-charge state whenever native network input does not fully replenish
the configured loss.

## Shot visual effects

The stock
`/Game/VFX/Environment/Interactive/NS_Explosion_SeaMine` system can be split
per spawned Niagara component without copying or modifying the stock asset.
Calling `UFXSystemComponent.SetEmitterEnable` before activation uses two
explicit masks from the current 14-emitter stock system. Fountain mode enables
`Drop_Tower`, `Tower`, `Spikes` and `Core_Drops`; impact mode enables
`FireExplode`, `spark`, `spark_l`, `shockwave`, `Spark001`, `Core_Smoke001`,
`SmallerPieces` and `BigPieces`. `BigPieceSplash` and `SmallPieceSplash` remain
disabled in both modes. A water miss uses only the fountain effect; a damaging
Talon-shark impact uses fire and smoke.

Production effects use a mod-owned transient Blueprint helper with an owned
inactive Niagara component. Each instance applies one of these masks,
activates independently and has a ten-second maximum actor lifetime. This
keeps an effect alive after its projectile stops while preventing one hit from
destroying or reconfiguring another hit's effect. The helper stores only a
runtime reference to the stock system; no stock VFX package is copied, patched
or registered by Railgun.

The editor-only native mirror for water lookup is
`/Script/Voyage.VoyageMiscBlueprintFunctionLibrary`. Current reflection
revalidation confirms `GetActorWorld(AActor*) -> UWorld*` and
`GetWaterHeightAtLocation(UWorld*, FVector) -> float`. Earlier native behavior
evidence established that the latter may return the `-100000 cm` sentinel and
filters candidates by water-body bounds and query results; its body was not
re-disassembled for this build. A valid gameplay world, bounded segment
sampling and a verified near-surface residual are therefore required; a
single endpoint value, zero height, or the function alone is not a universal
exact-surface contract. This API and its limits are fingerprint-bound.

The projectile applies the explosion on a real blocking hit and checks
only finite travelled projectile segments for the first accepted water-surface
crossing. Exhaustive collision ordering, submerged-target, save/load,
multiplayer and performance guarantees are outside the established coverage.

The above-water wake path uses the fingerprint-bound
`VoyageWeatherSubsystem.AddFluidImpulse(FVoyageFluidImpulse)` contract directly;
the subsystem derives from `TickableWorldSubsystem`, and the impulse contains
location, two-dimensional direction, radius and strength. One transient
controller per shot samples only already travelled segments over the first
500 metres, with two-metre spacing and cross-frame remainder carry. Its bounded
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
the first-crossing fountain and direct-hit attack paths. This is not an exact
reconstruction of the stock attack consumer. The 0.25-second hold is one part
of the complete wake path, not an isolated explanation of its visibility.
Exhaustive height/distance, obstacle-clipping, frame-rate and performance
guarantees are outside the established coverage.

For the fingerprint above, the native `AddFluidImpulse` entry appends the
48-byte impulse to the subsystem queue; it does not render or acknowledge a
visible effect. The stock weather Blueprint selects `RT_Foam` (1024 by 1024) and
`FluidWorldScale = 20000` cm. `BP_NinjaLive_Area_Water_Voyage` feeds that render
target to its FluidNinja component. Queue consumption, simulation-area placement
and the exact stock consumer lifecycle remain unresolved; the configured scale
alone does not establish which world points are represented.

## Build and compatibility identity

The explicit cook inventory passes through the bounded manifest adapter, which
checks package count and Core SHA-1 before the stock cook commandlet consumes
the list in one pass.
The three-record plugin-local registry is derived solely from owned package
readbacks through the shared Voyage registry profile and class-agnostic native
writer, then reopened and compared field-for-field.

- Parser profile: `UE5_8`; authoring editor: `5.8.2`. The exact accepted
  source-build pin is in `game-build.json`.
- Selected candidate ZIPs live under `artifacts/installations/Railgun-V<N>/`.
  Temporary builds and their transient installation state stay in `Tmp/`;
  neither is a source dependency for rebuilding.

## Compatibility limits and deferred work

- Native mirrors, scan roots and reconstructed contracts are fingerprint-bound.
  A changed game fingerprint requires revalidation before cooking or making
  compatibility claims.
- Native reflection, stock references, serialization and producer formats
  have separate structural checks. Controller hardware, exhaustive remapping,
  target-specific damage and effects, powered charging, registry discovery,
  research unlock and save restoration require their own runtime evidence;
  acceptance of keyboard controls does not establish all of these behaviors.
- Generated packages use tagged properties for partial mirrors. The ammo and
  gun items are the bounded post-cook unversioned conversion exceptions; both
  are reopened before packaging. Do not convert unrelated native-child assets
  by analogy.
- The established runtime scope is single-player, including magazine
  save/reload. Multiplayer and exhaustive cable/operator-entry scenarios are
  outside that scope.
- Firing consumes one round and rejects insufficient ammunition or energy.
  Exceptional native inventory mutation rejection after the live precheck is
  outside the established runtime coverage and does not lock later shots.
- Manual reload and action-hint availability/refresh are established runtime
  behavior. Exact input mapping, cached predicate, lifecycle bindings and the
  native transfer call also have static and installed-package readback coverage.
  Exhaustive partial and split-stack conservation across every source/target
  combination remains outside established runtime coverage.
- Railgun neither requires nor uses a shared placeholder registry pool. Shared
  allocation rules, duplicate-ID ownership and precedence between conflicting
  records are outside its supported contract.
- Collision and water-entry visuals must remain observational: they do not
  change projectile movement, water physics or direct-attack authority. The
  separate above-water wake path owns fluid impulses.
