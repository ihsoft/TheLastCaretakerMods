# Railgun architecture

## Proven runtime contracts

- The weapon is constructed through the game's module/fabricator path by
  replacing the Cyclone leaf Blueprint and item-data packages. This stock
  virtual package identity is required even though all other owned packages use
  `/Game/Mods/Railgun/...`.
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
  500 kJ charge is full, resets charge after a shot, and applies a validated
  direct attack to the hit target. Projectile travel is represented visually;
  hit resolution is authoritative.
- Shot audio is cooked as a `SoundWave`; its volume multiplier is read from
  `Railgun.ini`. The accepted baseline is 600 percent.
- Dismantling after exit is supported and must not leave the coordinator with a
  stale actor reference.

## Compatibility limits

- Native mirrors and reconstructed Blueprint contracts are bound to the game
  fingerprint enforced by `Build-Railgun.ps1`. A changed fingerprint requires
  revalidation before cooking or compatibility claims.
- Generated packages use tagged properties. Unversioned native-child output is
  rejected because partial mirrors do not define a complete shipping ABI.
- The mod is currently single-player validated. Save persistence and multiplayer
  behavior are not established.

## Reasonable design hypotheses

- Ammunition instances in the GLB can later represent remaining rounds, but no
  ammunition inventory contract is implemented yet.
- Visual projectile effects, muzzle effects and richer audio can be added without
  changing hit resolution, provided the validated direct-attack path remains the
  authority.
