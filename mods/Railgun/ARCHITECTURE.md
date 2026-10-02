# Railgun architecture

## Proven runtime contracts

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
- Yaw, pitch, muzzle, sight, entry and power-socket roles are declared in the
  model manifest. Model revisions may change topology, materials and local
  offsets without changing generator code when those roles remain valid.
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
- Loss of the module power connection discharges stored energy to zero at the
  configured `OfflineDischargeKW` rate. The default is `10` kW; zero disables
  offline discharge. This setting is independent of normal standby demand.
- Shot audio is cooked as a `SoundWave`; its volume multiplier is read from
  `Railgun.ini`. The accepted baseline is 600 percent.
- Dismantling after exit is supported and must not leave the coordinator with a
  stale actor reference.

## Primary assets and research

- The actor is `/Game/Mods/Railgun/Module/BP_Module_Railgun`; the coordinator
  discovers that exact owned class. Its `VoyageModuleComponent.ItemAsset` points
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
  from the owned UAssetAPI JSON readbacks plus the explicit class/deployment
  policy in `Assets/data-assets-contract.json`; `Voyage/AssetRegistry.bin` is
  neither read nor packaged or replaced.
- On Steam build `25191271`, the stock `DefaultGame.ini` scans `Skill` below
  `/Game/Data/Assets/Skill` and `Item` below `/Game/Data/Assets`. Registry
  membership alone is insufficient for discovery outside those roots.
- User validation of `build-20260926-083231` confirmed discovery, research
  unlocking both recipes, and independent gun construction, firing and
  dismantling. Pre-research recipe absence was also tested.
- `EVoyageSkillUnlockMethod::Never` hid the skill and left its recipes unavailable
  in the tested pre-research save while the assets remained registered and
  packaged. Revoking existing unlocks and persistence under `Never` were not
  tested. The hiding experiment is no longer enabled.

## Authored ammunition contract

The stable identity remains
`/Game/Data/Assets/Ammo/DA_Ammo_Railgun_FullRod`, despite the historical suffix.
The current item is authored as one `VoyageItemAmmo` export in
`Assets/Fabricator/railgun-ammo-item.json`; it is not a full Sniper Rod clone
or a stock Rod package override. The generator emits only an editor/cook
placeholder at the same identity so the research skill can reference it. The
packaged ammo object graph and values come only from the owned JSON.

The current game-validated ammo baseline contains 16 top-level serialized properties:
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
one physical round. Its sibling `ammo-cassette-source.json` binds the current
source hash to an inspection snapshot: eight nodes, six mesh instances, three
mesh definitions, 1508 triangles, two materials and six embedded images, with
no skin or animation. Those counts describe this source revision rather than a
permanent topology contract. The dedicated Interchange adapter combines all
current instances, preserves imported materials and textures, applies a simple
box collision, and records the resulting packages and mesh readback in the
build inventory. The current Unreal readback is 1504 triangles with bounds
`6.56 x 6.56 x 54.65952 cm`; the source audit and imported readback are recorded
separately because mesh building may remove degenerate source triangles. A
future source revision must refresh the sibling contract and pass the same
source-to-import checks; it need not retain decorative node names or counts.

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

## Current game-validated checkpoint

`build-20261002-donor-free-03` is the current source-built checkpoint. Its
three-record plugin-local registry is derived solely from owned package
readbacks and the explicit registry policy, then reopened and compared
field-for-field. The user tested the installed artifact and reported that
everything works; this general confirmation is not evidence of additional
individually enumerated scenarios.

- Steam build: `25191271`; parser profile: `UE5_8`.
- Executable SHA-256:
  `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
- Release evidence:
  `artifacts/railgun/build-20261002-donor-free-03/release-manifest.json`.
- Installation evidence:
  `artifacts/installations/Railgun/20261002-233550-build-20261002-donor-free-03-91616adb/install-manifest.json`.
- These ignored manifests identify the tested dirty-source artifact; they are
  evidence and rollback pointers, never required source inputs for a rebuild.
- The user also confirmed this plugin-local registry remains additive while
  `VoyageAssetPool_P` supplies a global registry override. This removes that
  mod as an isolation requirement for Railgun, but does not establish safety
  for arbitrary registry overrides or duplicate primary-asset identities.

The preceding `build-20260930-221701` remains scoped behavioral evidence for
the six distinct cassette subtrees and their 36 covered render descendants,
model rendering, ammo visibility, isolated zero-ammo warning, one-round firing
debit, ordinary ammunition and charge rejection, charge warnings, 10 kW offline
discharge, persistence, single-round fabrication and the owned physical ammo
cassette. Those observations were not separately repeated or itemized for the
current checkpoint.

## Compatibility limits and deferred work

- Native mirrors, scan roots and reconstructed contracts are fingerprint-bound.
  A changed game fingerprint requires revalidation before cooking or making
  compatibility claims.
- Generated packages use tagged properties for partial mirrors. The ammo item
  is the sole bounded post-cook conversion exception; it is reopened before
  packaging. Do not convert unrelated native-child assets by analogy.
- The mod is single-player validated, including magazine save/reload.
  Multiplayer and distinct cable or operator-entry scenarios remain unvalidated.
- Native one-round consumption and ordinary ammunition/energy rejection gates
  are game-validated. The bounded energy-compensation failure branch is
  structurally validated; its exceptional native rejection path has not been
  induced in game.
- Railgun neither requires nor uses a shared placeholder registry pool. Shared
  allocation rules, duplicate-ID ownership and precedence between conflicting
  records are outside its supported contract.
- Visual projectile effects, muzzle effects and richer audio can be added
  without changing hit resolution, provided the validated direct-attack path
  remains the authority.
