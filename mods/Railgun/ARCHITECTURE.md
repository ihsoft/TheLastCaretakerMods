# Railgun architecture

## Runtime contracts

- The weapon uses the game's module/fabricator path. Its actor, UI and runtime
  packages remain under `/Game/Mods/Railgun/...`; primary Item and Skill data
  assets use mod-unique names inside the current game's confirmed AssetManager
  scan roots. The release must not override stock Cyclone packages.
- The built shell is a `VoyageModuleActor` with tagged model components,
  placement collision, dynamic collision and an electric socket linked to its
  module component.
- Interaction acquisition requires a query component that explicitly blocks the
  `Interact` channel and an exact `InteractiveInterface` implementation. The
  action provider and input action are separate contracts.
- Operator control uses a dedicated child of the common Voyage vehicle pawn.
  Entry, exit, camera, input context, HUD selection and action hints are owned by
  that station; the physical weapon remains stationary.
- The GLB is imported by Unreal Interchange. Full hierarchy matrices determine
  ownership and transforms; node names identify roles but do not imply parentage.
- Yaw, pitch, muzzle, sight, entry, power-socket and charge-indicator roles are
  declared in the model manifest. Model revisions may change topology,
  materials and local offsets without changing generator code when those roles
  remain valid.
- Wide view is character-eye view. Scope view uses the barrel sight, optical
  mask, reticle and reduced sensitivity. The weapon aligns toward the character
  view target before scoped aiming.
- The weapon charges from the module electricity system, may fire only when its
  configured charge is full, resets charge after a shot, and applies a validated
  direct attack to the hit target. Projectile travel is represented visually;
  hit resolution is authoritative.
- Railgun settings and HUD use the game's displayed `KWh` scale. Voyage maps
  one displayed `KWh` to 1000 native electricity amount units; module demand
  remains expressed in W and is derived from the configured charge time in
  addition to standby demand.
- Station initialization resolves and caches the concrete module, binds the
  exact `OnModuleValueChanged` delegate, performs an initial charge snapshot,
  and removes the binding at EndPlay. The gameplay callback refreshes the
  cached charge and switches `SetCustomConsumption` only when the charging
  versus idle mode changes. Re-entry settings reload forces one demand refresh.
  The desired mode is cached before the native setter; synchronous nested
  notifications are coalesced into one final non-forced read, which does not
  repeat the setter when the mode is unchanged. The energy branch of the actor
  tick retains only time-based offline discharge and its supply checks; it no longer discovers
  the module, maintains demand, or samples unused charge-rate state. Capacity,
  units, the fresh pre-shot balance check, shot debit and bounded refund are
  unchanged.
- Loss of the module power connection discharges stored energy to zero at the
  configured `OfflineDischargeKW` rate. The default is `10` kW; zero disables
  offline discharge. This setting is independent of normal standby demand.
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
- Shot audio is cooked as a `SoundWave`; its volume multiplier is read from
  `Railgun.ini`. The accepted baseline is 600 percent.
- A successful shot has two independent recoil paths. The historical
  `CameraRecoilStrength` key now changes real station aim once per shot in a
  uniformly random yaw/pitch-plane direction, with a two-degree baseline and
  no return animation. Wide view updates eye aim before converging the weapon;
  scope view updates weapon aim directly. Existing yaw/pitch limits clamp the
  result, and neither path changes character or control rotation. Ship recoil
  walks at most 32 scene-component attachment parents from the real module root
  and applies one backward impulse to the first simulated primitive it finds.
  Both settings are bounded multipliers from zero through ten. Before parsing,
  the runtime initializes only these two settings to zero, so a missing INI,
  missing recoil key or invalid numeric value disables the corresponding
  recoil. Explicit valid INI values override that fallback; one remains the
  authored baseline. Missing/invalid-INI recoil fallback has static coverage
  only, not runtime coverage.
- Dismantling after exit is supported. The module owns a transient direct
  station reference; station safety disables acquisition, exits a controlled
  pawn, and destroys the detached station on a later pass.

## Primary assets and research

- The actor is `/Game/Mods/Railgun/Module/BP_Module_Railgun`. The shell owns an
  idempotent `InitializeRailgunStation` function and a transient direct station
  reference; no global actor discovery or autoload coordinator is packaged.
  `ReceiveBeginPlay` and the exact inherited persistent post-load event each
  schedule one next-tick initialization attempt. Initialization requires the
  model root tagged `Railgun.Model.Root` and an attached parent, so fabricator
  visuals and unattached ghosts cannot spawn a station. The node selected by
  `model-source.json`'s `nodes.root` is the shell's scene root and station
  anchor; no additional mount collider is needed. Physical collision remains
  on the configured `fabricatorCollision` mesh, and the dynamic-collision
  component retains auto-weld. An enclosing mount collider interferes with
  the attached power cable. Shell-owned initialization requires no
  VoyageAutoLoader. There is no polling or retry
  fallback. Its `VoyageModuleComponent.ItemAsset` points
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
- On Steam build `25191271`, the stock `DefaultGame.ini` scans `Skill` below
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

The primary icon is mod-owned. Drop configuration uses the owned merged mesh
`/Game/Mods/Railgun/Fabricator/AmmoCassette/SM_RailgunAmmoCassette` and the
native `BP_DynamicMeshActor`. `WeaponData`, all projectile subexports, bullet/case
fields, stock SFX/VFX, damage-type references, `SecondaryIcon` and `ScalePerItem`
are absent. Imports are limited to the native item class/CDO, ammo category,
recipe materials, owned icon and the owned drop-mesh dependencies. These
omissions are validated for fabrication and pickup, not for firing this item
through a stock sniper rifle.

`Assets/Fabricator/RailgunAmmoCassette.glb` is a user-authored rigid source for
one physical round. The dedicated Interchange adapter combines the source's
current mesh instances, preserves any imported materials and textures, applies
simple box collision, and records the resulting packages and mesh readback in
the build inventory. Validation requires the stable owned mesh identity,
nonempty render geometry, simple pickup collision, finite nondegenerate bounds,
and inclusion of every material and texture dependency actually imported from
the current source. It does not freeze a model revision, topology, decorative
nodes, former bounds, or material and texture counts. Build provenance hashes
the actual source and rejects changes made while a build is in progress.

After cook, the reviewed UAssetGUI/UAssetAPI writer converts the three owned
JSON sources directly to staged `.uasset`/`.uexp` packages using UE 5.8.
`Assets/data-assets-contract.json` owns their individual package/native
identities plus one shared game, mapping, writer and revalidation gate for the
complete set. Every import carries explicit
`PackageName=None`; leaving
that field null is not equivalent under the reviewed filtered-import writer.
The build reopens the result and compares versions, custom versions, name map,
imports and complete export graph against the JSON used for that build before
packaging. The skill and gun item use the same direct writer/readback contract;
their current tagged/unversioned formats and object graphs are preserved by
their editable JSON rather than hard-coded count gates. See the shared
[serialization contract](../../docs/voyage-cooked-asset-toolchain.md#package-serialization-is-a-separate-compatibility-contract).

General consumer findings and limitations, including output grouping and
native caliber defaults, live in
[item fabrication and pickup](../../docs/voyage-item-fabrication-and-pickup.md).
The independent scan-to-vault unlock mechanism is documented in
[scanning and recipe unlocks](../../docs/voyage-scanning-and-recipe-unlocks.md).

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
[filtered inventory contract](../../docs/voyage-item-fabrication-and-pickup.md#filtered-module-inventories-refinery-reference).

The fire path treats the owned native inventory as authority; neither the
six-slot model state nor the HUD count cache can authorize a shot. It resolves
the exact accepted Railgun ammunition, captures a live occupied slot, verifies
that slot's exact item and positive count, and creates a deferred inactive shot
before spending resources. The input request is claimed before native calls
that may dispatch delegates. Native exact energy removal runs first, followed
without a latent gap by native `RemoveItem` for one round with notifications
enabled. Only a return value of one finishes the projectile, updates successful
shot charge state and plays audio. Empty inventory or insufficient charge
therefore produces no projectile, sound, damage, ammo debit or energy debit;
automatic charging remains independent of magazine state.

If the ammo debit rejects after energy was removed, the graph uses native
`AddResource` as bounded compensation, verifies both the accepted delta and the
live restored balance within a small double tolerance, then destroys the
deferred actor. A compensation mismatch permanently disables firing for that
operator instance and emits one diagnostic log message. This is explicit
fail-closed compensation, not a claim of transactional atomicity across the two
native APIs.

The wide-view HUD displays six persistent cartridge icons
above the existing charge text. It reads the module's event-maintained cached
count after one guarded initial synchronization; the HUD does not enumerate or
bind the inventory. The rightmost N icons use opaque white and the remaining
positions use the radial's faint bar color, matching the physical magazine
while empty slots remain visible. At zero rounds, all six positions use a
subtle red tint instead of the ordinary faint color. Ammo colors do not inherit
the dynamic charge-ring color.
The icon row and charge text form one centered block, and the existing optics
gate hides that whole block together with the charge radial. The source image,
crop, six-slot order, colors, cached-count path and absence of HUD inventory
polling are statically validated. Real-game validation confirms placement,
right-to-left updates, the zero-count tint and optics hiding.

The charge text and filled part of the radial ring use the same subtle red tint
whenever stored energy is below one shot. Shot-ready and invalid states use
opaque white; the radial background color is never changed by this warning.
When power is disconnected, configurable offline discharge eventually moves a
previously shot-ready gun into the red incomplete-charge state.

## Shot visual effects

The stock
`/Game/VFX/Environment/Interactive/NS_Explosion_SeaMine` system can be split
per spawned Niagara component without copying or modifying the stock asset.
Calling `UFXSystemComponent.SetEmitterEnable` before activation with
`dirt_main` disabled and `shockwave`, `main`, `spark`, `spark_l`, `refr`,
`project` and `puff` enabled produces the explosion without the water fountain.
The inverse mask produces only the fountain.

Production effects use a mod-owned transient Blueprint helper with an owned
inactive Niagara component. Each instance applies one of these masks,
activates independently and has a ten-second maximum actor lifetime. This
keeps an effect alive after its projectile stops while preventing one hit from
destroying or reconfiguring another hit's effect. The helper stores only a
runtime reference to the stock system; no stock VFX package is copied, patched
or registered by Railgun.

For Steam build `25191271`, the editor-only native mirror for water lookup is
`/Script/Voyage.VoyageMiscBlueprintFunctionLibrary`. Its confirmed signatures
are `GetActorWorld(AActor*) -> UWorld*` and
`GetWaterHeightAtLocation(UWorld*, FVector) -> float`. The latter may return
the `-100000 cm` sentinel, and its native implementation filters candidates by
water-body bounds and query results. A valid gameplay world, bounded segment
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

The explicit cook inventory passes through the bounded manifest adapter.
The three-record plugin-local registry is derived solely from owned package
readbacks through the shared Voyage registry profile and class-agnostic native
writer, then reopened and compared field-for-field.

- Steam build: `25191271`; parser profile: `UE5_8`.
- Executable SHA-256:
  `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
- Versioned release ZIPs live in `Voyage/Content/Paks/`. Ignored installation
  manifests under `artifacts/installations/Railgun/` identify exact installed
  hashes and rollback material; neither is a source dependency for rebuilding.
  Temporary release-manifest paths can disappear after build cleanup.

## Compatibility limits and deferred work

- Native mirrors, scan roots and reconstructed contracts are fingerprint-bound.
  A changed game fingerprint requires revalidation before cooking or making
  compatibility claims.
- Generated packages use tagged properties for partial mirrors. The ammo item
  is the sole bounded post-cook conversion exception; it is reopened before
  packaging. Do not convert unrelated native-child assets by analogy.
- The established runtime scope is single-player, including magazine
  save/reload. Multiplayer and exhaustive cable/operator-entry scenarios are
  outside that scope.
- Firing consumes one round and rejects insufficient ammunition or energy.
  The bounded energy-compensation failure branch has structural coverage only;
  exceptional native rejection is outside the established runtime coverage.
- Railgun neither requires nor uses a shared placeholder registry pool. Shared
  allocation rules, duplicate-ID ownership and precedence between conflicting
  records are outside its supported contract.
- Collision and water-entry visuals must remain observational: they do not
  change projectile movement, water physics or direct-attack authority. The
  separate above-water wake path owns fluid impulses.
