# Voyage item fabrication and pickup

## Evidence boundary

These findings come from user-tested authored `VoyageItemAmmo` packages in
Railgun, not a claim about every item class. The game fingerprint is Steam
build `25191271`, executable SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`, parser
profile `UE5_8`. Revalidate affected contracts when the fingerprint changes.
The owning [Railgun architecture](../mods/Railgun/ARCHITECTURE.md) records the
current item fields, values and release/install evidence.

## Distinct contracts

A working recipe requires more than a visible entry in the fabricator:

- Primary-asset registration and the game's configured scan roots establish
  manager identity/discovery. Research or another unlock path grants access;
  registration is not itself a recipe unlock.
- Package serialization must match the consumer's expectations. A registered
  and displayed item can still fail fabrication/pickup. The tested correction
  is described in the shared [serialization contract](voyage-cooked-asset-toolchain.md#package-serialization-is-a-separate-compatibility-contract).
- Item recipe and drop configuration determine the fabricated output. Validate
  the spawned box's contents and loot interaction, not just the completion
  sound or a visible physical mesh.
- Inventory ammunition consumption and weapon ballistics are separate
  consumers. Fabrication/pickup tests do not establish firing compatibility.

## Ammo item does not require a complete weapon object graph

One authored `VoyageItemAmmo` export with valid primary registration, working
serialization and retained recipe/drop metadata can fabricate and be looted.
In the tested setup, no projectile subexports, `WeaponData`, bullet/case fields,
stock SFX/VFX or damage-type references are required for those two operations.
The stock secondary icon and `ScalePerItem` are also unnecessary there.

This does not establish an absolute minimum schema or removal safety for other
ammo/weapon consumers. Omitted properties can inherit native/class defaults;
absence in serialized output must not be read as a zero or disabled value.

## Craft amount and drop grouping are different settings

For a six-round recipe using stock ammo-box drop configuration:

- Omitting both `MaxDropCount` and `ScalePerItem` produced six boxes with one
  round each.
- Restoring only `MaxDropCount=50`, with `ScalePerItem` still omitted, restored
  one box containing all six rounds.

The second test isolates a sufficient grouping correction for this setup.
Keep an explicit suitable `MaxDropCount` when preserving this behavior; do not
assume `CraftAmount` alone controls how many items share one drop. It does not
prove that exactly `50` is necessary, establish the behavior of every other
limit, or equate drop grouping with inventory stack capacity.

## Caliber is numeric, and omission does not hide it

`VoyageItemAmmo.Caliber` is a float. In the tested item, omission retained
functional fabrication/pickup but yielded the native/class default `9.0`,
displayed as `9mm`. Explicit `45.0` displayed as `45mm`.

Do not use removal as a tooltip-hiding mechanism or attempt to put a semantic
string such as `Railgun` into this numeric property. An item-specific text
override or hiding mechanism would require separate consumer research and
validation; changing a shared formatter would affect other items.

## Validation boundary

For a reduction or new item, separately check discovery/unlock, fabrication,
output count and grouping, loot transfer, and presentation. Save/load and
multiplayer require their own tests. Retain a known-good artifact before a
change; parser reopen and container verification remain lower gates than
these actual gameplay consumers.
