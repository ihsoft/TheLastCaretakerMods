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
  to `/Game/Data/Assets/Modules/DA_Item_Module_RailgunCannonMk01`. The gun item
  is cloned from the current stock module donor, receives the owned icon,
  actor reference and temporary one-Alloy-Frame recipe, and has donor production
  metadata removed.
- One Tier-19 `Railgun` skill owns exactly two item references: the independent
  gun and the authored ammunition. The skill identity is
  `/Game/Data/Assets/Skill/Railgun/DA_Skill_Railgun`; its dedicated research icon
  is under `/Game/Mods/Railgun`.
- A complete current-game AssetRegistry with Railgun's primary entries is
  packaged. This is an accepted interim registry override, not a composable
  solution for multiple mods replacing the same registry.
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
The current item is generated from scratch as one `VoyageItemAmmo` export; it
is not a full Sniper Rod clone or a stock Rod package override. Sniper Rod is
still the fingerprint-matched donor for package serialization and stock drop
configuration.

The tested output contains exactly 16 top-level serialized properties:
`Caliber`, `Icon`, `Category`, `CategoryAsset`, `Quality`, `Weight`, `CraftTime`,
`CraftElectricityCost`, `CraftAmount`, `CraftFilter`, `Components`,
`DropVariations`, `DroppedActor`, `MaxDropCount`, `Name`, `Description`.
This is the smallest tested baseline, not proof that every remaining property
is indispensable.

| Setting | Current owned value |
| --- | --- |
| Name | Railgun Kinetic Rounds |
| Description | Armor-piercing kinetic rounds. No explosives, just mass and velocity. |
| Caliber / native tooltip | `45.0` / `45mm` |
| Quality | Common |
| Weight | `3.9` per round; six-round cassette displays `23.4 kg` |
| Craft amount / time | `6` rounds / `6` seconds |
| Craft electricity cost / filter | `5` native units / `3` |
| Recipe | Iron `2`, Copper `2`, Plastic `1` |
| MaxDropCount | `50`; the tested batch yields one box of six rounds |

The primary icon is mod-owned. Drop configuration uses the stock ammo box and
`BP_DynamicMeshActor`. `WeaponData`, all projectile subexports, bullet/case
fields, stock SFX/VFX, damage-type references, `SecondaryIcon` and `ScalePerItem`
are absent. Imports are limited to the native item class/CDO, ammo category,
recipe materials and owned icon. These omissions are validated for fabrication
and pickup, not for firing this item through a stock sniper rifle.

After cook, the shared patcher matches this one item's package serialization
to the exact stock Sniper Rod donor: unversioned property stream and matching
header/resolved custom versions (14 entries on this fingerprint). Tagged
output had registered and displayed but failed to produce a usable pickup.
The successful rewrite changed several metadata fields together; it does not
isolate one causal bit. See the shared
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
The mass budget is six times the item's actual weight (currently 23.4).
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

`build-20260929-061751` is the current firing, power-state, HUD-state and
model-structure checkpoint. Its model
passes the six distinct, disjoint and nonempty cassette-subtree contract with
all 36 render descendants covered by default hiding and propagated runtime
visibility, while the six holders remain outside those subtrees. User gameplay
confirmation covers the new model rendering, ammo visibility behavior, isolated
zero-ammo warning, one-round firing debit, rejection without ammunition or
charge, charge warning across offline, insufficient and ready states, 10 kW
offline discharge, and persistence. The earlier validated inventory and
fabrication contracts remain the foundation.
The earlier 16-property ammo baseline established fabrication and pickup of one
box with six rounds; gun/research validation remains separate.

- Steam build: `25191271`; parser profile: `UE5_8`.
- Executable SHA-256:
  `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
- Release evidence: `artifacts/railgun/build-20260929-061751/release-manifest.json`.
- Installation evidence:
  `artifacts/installations/Railgun/20260929-062140-build-20260929-061751-4e3ca500/install-manifest.json`.
- These ignored manifests identify the tested dirty-source artifact; they are
  evidence and rollback pointers, never required source inputs for a rebuild.

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
- A shared registry mod with hidden, typed placeholders and separate consumer
  overrides is deferred until the Railgun foundation is complete. The `Never`
  experiment does not validate cross-mod overriding, slot allocation or load
  precedence; do not implement that architecture as part of this checkpoint.
- Visual projectile effects, muzzle effects and richer audio can be added
  without changing hit resolution, provided the validated direct-attack path
  remains the authority.
